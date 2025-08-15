// sc2kfix hooks/hook_sc2k1996_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is where I test a bunch of stuff live to cross reference what I think is going on in the
// game engine based on decompiling things in IDA and following the code paths. As a result,
// there's a lot of experimental stuff in here. Comments will probably be unhelpful. Godspeed.

// !!! HIC SUNT EVEN MORE DRACONES !!!
// 2025-08-04 (araxestroy): oh MAN this file sucks. I need to go through it all and do a full
// rework to match the KNF-esque style I usually use when writing code for this project. It also
// has a dearth of comments because a lot of it is reimplemented from decompilation, so even after
// writing and/or approving a lot of the code here I don't know what half of the code in this
// fucking file does.
//
// I am not a religious man, but if anyone reading this is, please pray for me.

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define GUZZARDO_DEBUG_OTHER 1
#define GUZZARDO_DEBUG_MENU 2

#define GUZZARDO_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef GUZZARDO_DEBUG
#define GUZZARDO_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT guzzardo_debug = GUZZARDO_DEBUG;

static DWORD dwDummy; 

#define NUM_CHEATS 15
#define NUM_CHEAT_MAXCHARS 9
#define MAX_USER_LABELS 51

typedef struct {
	int iIndex;          // Cheat index, match multiple cheats to the same index.
	const char* pEntry;  // Code entry
	int iPos;            // Position within the array. (Only set when there's a match)
} cheat_t;

static enum {
	CHEAT_FUND,
	CHEAT_CASS,
	CHEAT_THEWORKS,
	CHEAT_MAJORFLOOD,
	CHEAT_PARTTHESEA,
	CHEAT_FIRESTORM,
	CHEAT_DEBUG,
	CHEAT_MILITARY,
	CHEAT_JOKE,
	CHEAT_WEBB,
	CHEAT_OOPS,
	CHEAT_REPENT
};

// Some the codes here have been randomised once more.
static cheat_t cheatStrArray[NUM_CHEATS] = {
	{CHEAT_FUND,       "fund",      -1},
	{CHEAT_CASS,       "cass",      -1},
	{CHEAT_THEWORKS,   "ithecama",  -1},
	{CHEAT_MAJORFLOOD, "nhoa",      -1},
	{CHEAT_PARTTHESEA, "msseo",     -1},
	{CHEAT_FIRESTORM,  "nwsueheo",  -1},
	{CHEAT_FIRESTORM,  "mlayrosre", -1},
	{CHEAT_DEBUG,      "psiclaril", -1},
	{CHEAT_MILITARY,   "gnarlimit", -1},
	{CHEAT_JOKE,       "joke",      -1},
	{CHEAT_WEBB,       "webb",      -1},    // From the Interactive Demo
	{CHEAT_OOPS,       "damn",      -1},    // DOS
	{CHEAT_OOPS,       "darn",      -1},    // DOS
	{CHEAT_OOPS,       "heck",      -1},    // DOS
	{CHEAT_REPENT,     "mylrosde",  -1}     // Custom
};

// In the game itself it uses an array of 72 entries
// (the original 8 cheat entries * 9 potential characters + current position).
// For the custom version it has been adjusted to a multi-dimensional array.
static int cheatCharPos[NUM_CHEATS][NUM_CHEAT_MAXCHARS] = {
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  6,  5,  4,  2,  3,  7,  1, -1},
	{0,  2,  3,  1, -1, -1, -1, -1, -1},
	{0,  4,  2,  3,  1, -1, -1, -1, -1},
	{0,  4,  1,  5,  7,  3,  2,  6, -1},
	{0,  4,  6,  5,  1,  8,  2,  7,  3},
	{0,  6,  2,  1,  3,  7,  4,  8,  5},
	{0,  7,  4,  6,  2,  3,  8,  5,  1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  1,  2,  3, -1, -1, -1, -1, -1},
	{0,  3,  5,  6,  4,  1,  2,  7, -1}
};

// This is set if there are multiple cheats detected matching the first character.
static BOOL cheatMultipleDetections = FALSE;;
static const char* theHouse = "Ilona's House";
int iChurchVirus = -1;

static void AdjustDebugMenu(HMENU hDebugMenu) {
	if (hDebugMenu) {
		HMENU hDebugPopup;
		MENUITEMINFO miiDebugPopup;
		miiDebugPopup.cbSize = sizeof(MENUITEMINFO);
		miiDebugPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hDebugMenu, 0, TRUE, &miiDebugPopup) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		hDebugPopup = miiDebugPopup.hSubMenu;

		// Insert in reverse order.
		// Separator between the disasters and internal debugging functions.
		if (!InsertMenu(hDebugPopup, 11, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between grants and disasters
		if (!InsertMenu(hDebugPopup, 4, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between the version option and grants
		if (!InsertMenu(hDebugPopup, 1, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		// Insert in reverse order.
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_MISSILESILOS, "Propose Missile Silos") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_NAVALYARD, "Propose Naval Yard") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_ARMYBASE, "Propose Army Base") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #6 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_AIRFORCE, "Propose Air Force Base") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #7 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_DECLINED, "Stop Military Spawning") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #8 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		// Add the message map entries
		AddNewSCVMessageMapEntry(WM_COMMAND, 0, IDM_DEBUG_MILITARY_DECLINED, IDM_DEBUG_MILITARY_DECLINED, 0x0A, ProposeMilitaryBaseDecline);
		AddNewSCVMessageMapEntry(WM_COMMAND, 0, IDM_DEBUG_MILITARY_AIRFORCE, IDM_DEBUG_MILITARY_AIRFORCE, 0x0A, ProposeMilitaryBaseMissileSilos);
		AddNewSCVMessageMapEntry(WM_COMMAND, 0, IDM_DEBUG_MILITARY_ARMYBASE, IDM_DEBUG_MILITARY_ARMYBASE, 0x0A, ProposeMilitaryBaseAirForceBase);
		AddNewSCVMessageMapEntry(WM_COMMAND, 0, IDM_DEBUG_MILITARY_NAVALYARD, IDM_DEBUG_MILITARY_NAVALYARD, 0x0A, ProposeMilitaryBaseNavalYard);
		AddNewSCVMessageMapEntry(WM_COMMAND, 0, IDM_DEBUG_MILITARY_MISSILESILOS, IDM_DEBUG_MILITARY_MISSILESILOS, 0x0A, ProposeMilitaryBaseMissileSilos);

		if (guzzardo_debug & GUZZARDO_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated debug menu.\n");
	}
}

static int FindTheHouseLabel() {
	for (int i = 1; i < MAX_USER_LABELS; ++i) {
		if (dwMapXLAB[0][i].szLabel && _stricmp(dwMapXLAB[0][i].szLabel, theHouse) == 0) {
			return i;
		}
	}
	return -1;
}

static void SetTheHouseLabel(int xPos, int ySignPos) {
	__int16 iLabelIdx;
	WORD iTextLen;

	char(__stdcall * H_PrepareLabel)() = (char(__stdcall*)())0x402D56;

	if (dwMapXTXT[xPos][ySignPos].bTextOverlay) {
		if (dwMapXTXT[xPos][ySignPos].bTextOverlay >= MAX_USER_LABELS)
			return;
	}
	iLabelIdx = H_PrepareLabel();
	if (iLabelIdx) {
		dwMapXTXT[xPos][ySignPos].bTextOverlay = (BYTE)iLabelIdx;
		iTextLen = (WORD)strlen(theHouse);
		memcpy(&dwMapXLAB[0][(int)iLabelIdx], theHouse, iTextLen);
		dwMapXLAB[0][iLabelIdx].szLabel[iTextLen] = 0;
	}
}

static BOOL FindTheHouse() {
	__int16 xPos, yPos, xWindPos, ySignPos;
	__int16 iLength, iDepth, iLabelIdx;

	void(__cdecl * H_RemoveLabel)(__int16) = (void(__cdecl*)(__int16))0x401DCA;

	xPos = -1;
	yPos = -1;
	ySignPos = -1;
	for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
		for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
			if (dwMapXBLD[iLength][iDepth].iTileID == TILE_COMMERCIAL_1X1_BEDANDBREAKFAST) {
				if (dwMapXZON[iLength][iDepth].b.iZoneType == ZONE_NONE) {
					xPos = iLength;
					yPos = iDepth;
					xWindPos = xPos - 1;
					ySignPos = yPos - 1;
					break;
				}
			}
		}
	}
	iLabelIdx = FindTheHouseLabel();
	if (xPos != -1 && yPos != -1) {
		// Set the sign if it is missing.
		if (iLabelIdx < 0)
			SetTheHouseLabel(xPos, ySignPos);
		// Set the Wind PowerPlant if it's not present
		// (assuming the spot is still available).
		Game_ItemPlacementCheck(xWindPos, yPos, TILE_POWERPLANT_WIND, 1);
		Game_CenterOnTileCoords(xPos, yPos);
		return TRUE;
	}
	if (iLabelIdx > 0 && iLabelIdx < MAX_USER_LABELS) {
		for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
			for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
				if (dwMapXTXT[iLength][iDepth].bTextOverlay == iLabelIdx) {
					H_RemoveLabel(iLabelIdx);
					dwMapXTXT[iLength][iDepth].bTextOverlay = 0;
					break;
				}
			}
		}
	}
	return FALSE;
}

static BOOL BuildTheHouse() {
	int iAttempts;
	__int16 xPos;
	__int16 yPos;
	__int16 xWindPos;
	__int16 ySignPos;

	map_XTER_t** dwMapXTERPrevX = (map_XTER_t**)0x4C9F54;

	iAttempts = 0;
	while (TRUE) {
	RETRY:
		xPos = Game_RandomWordLFSRMod128();
		yPos = Game_RandomWordLFSRMod128();
		xWindPos = xPos - 1;
		ySignPos = yPos - 1;
		if (xWindPos < 0 || ySignPos < 0)
			goto RETRY;
		if (dwMapXBLD[xPos][yPos].iTileID < TILE_SMALLPARK) {
			if (dwMapXBLD[xPos][ySignPos].iTileID < TILE_SMALLPARK &&
				dwMapXBLD[xWindPos][yPos].iTileID < TILE_SMALLPARK) {
				if (!dwMapXTER[xPos][yPos].iTileID &&
					!dwMapXTER[xPos][ySignPos].iTileID &&
					!dwMapXTERPrevX[xPos][yPos].iTileID &&
					(xPos < 0 || yPos >= GAME_MAP_SIZE || !dwMapXBIT[xPos][yPos].b.iWater) &&
					(xPos >= GAME_MAP_SIZE || ySignPos >= GAME_MAP_SIZE || !dwMapXBIT[xPos][ySignPos].b.iWater) &&
					(xWindPos >= GAME_MAP_SIZE || yPos >= GAME_MAP_SIZE || !dwMapXBIT[xWindPos][yPos].b.iWater)) {
					if (dwMapALTM[xPos][yPos].w.iLandAltitude == dwMapALTM[xPos][ySignPos].w.iLandAltitude &&
						dwMapALTM[xPos][yPos].w.iLandAltitude == dwMapALTM[xWindPos][yPos].w.iLandAltitude) {
						if (Game_ItemPlacementCheck(xPos, yPos, TILE_COMMERCIAL_1X1_BEDANDBREAKFAST, 1)) {
							SetTheHouseLabel(xPos, ySignPos);
							Game_ItemPlacementCheck(xWindPos, yPos, TILE_POWERPLANT_WIND, 1);
							Game_CenterOnTileCoords(xPos, yPos);
							return TRUE;
						}
					}
				}
			}
		}

		if (++iAttempts >= 100)
			break;
	}
	return FALSE;
}

static void ChangeChurchZone() {
	__int16 iReplaceTile, iLength, iDepth;

	for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
		for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
			if (dwMapXBLD[iLength][iDepth].iTileID == TILE_INFRASTRUCTURE_CHURCH) {
				if (dwMapXZON[iLength][iDepth].b.iZoneType == ZONE_NONE) {
					iReplaceTile = (rand() & 3) + 1; // Random rubble.
					Game_PlaceTileWithMilitaryCheck(iLength, iDepth, iReplaceTile); // Replace
					dwMapXZON[iLength][iDepth].b.iZoneType = ZONE_DENSE_RESIDENTIAL; // Re-zone
				}
			}
		}
	}
}

extern "C" void __stdcall Hook_MainFrameOnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	DWORD* pThis;

	__asm mov[pThis], ecx

	char nLowerChar;
	int i, j;
	int nCurrPos;
	int* nCodeArr;
	int nCodePos;
	char nCodeChar;
	cheat_t* strCheatEntry;
	HWND hWnd;
	DWORD* pSCView;
	HMENU hMenu, hDebugMenu;
	DWORD* pMenu, * pDebugMenu;
	int iSCMenuPos;
	DWORD jokeDlg[27];

	void(__cdecl * H_DoFund)(__int16) = (void(__cdecl*)(__int16))0x40191F;
	void(__thiscall * H_SimcityViewDebugGrantAllGifts)(DWORD*) = (void(__thiscall*)(DWORD*))0x401C0D;
	int(__thiscall * H_ADialogDestruct)(void*) = (int(__thiscall*)(void*))0x401D7A;
	void(__thiscall * H_SimcityAppAdjustNewspaperMenu)(void*) = (void(__thiscall*)(void*))0x40210D;
	DWORD* (__thiscall * H_JokeDialogConstruct)(void*, void*) = (DWORD * (__thiscall*)(void*, void*))0x4024E6;
	int(__stdcall * H_GetSimcityViewMenuPos)(int iPos) = (int(__stdcall*)(int))0x402EFA;
	void(__stdcall * H_SimulationProposeMilitaryBase)() = (void(__stdcall*)())0x403017;
	INT_PTR(__thiscall * H_DialogDoModal)(void*) = (INT_PTR(__thiscall*)(void*))0x4A7196;
	DWORD* (__stdcall * H_CMenuFromHandle)(HMENU) = (DWORD * (__stdcall*)(HMENU))0x4A7427;
	int(__thiscall * H_CMenuAttach)(DWORD*, HMENU) = (int(__thiscall*)(DWORD*, HMENU))0x4A7483;

	HINSTANCE& game_hModule = *(HINSTANCE*)0x4CE8C8;
	int& iCheatEntry = *(int*)0x4E6520;
	int& iCheatExpectedCharPos = *(int*)0x4E6524;
	char* szNewItem = (char*)0x4E66EC;

	hWnd = (HWND)pThis[7];

	// "Insert" key - only relevant in the demo but pressing it advances
	// the timer.
	if (nChar == 45) {
		// Does nothing here - could be useful for other test cases.
	}

	nLowerChar = tolower(nChar);
TRYAGAIN:
	if (iCheatEntry != -1) {
		strCheatEntry = &cheatStrArray[iCheatEntry]; // Cheat entry
		nCodeArr = cheatCharPos[iCheatEntry]; // Target character position reference array
		nCodePos = nCodeArr[iCheatExpectedCharPos];
		nCodeChar = strCheatEntry->pEntry[nCodePos];
		if (nCodeChar == nLowerChar) {
			nCurrPos = iCheatExpectedCharPos + 1;
			iCheatExpectedCharPos = nCurrPos;
			nCodePos = nCodeArr[nCurrPos];
			if (nCurrPos != NUM_CHEAT_MAXCHARS && nCodePos != -1) {
			GOBACK:
				if (iCheatEntry != -1)
					return;
				goto GETOUT;
			}
		}
		else if (cheatMultipleDetections) {
			for (i = 0; i < NUM_CHEATS; ++i) {
				if (i == iCheatEntry)
					continue;
				j = cheatStrArray[i].iPos;
				if (j >= 0) {
					strCheatEntry = &cheatStrArray[j];
					nCodeArr = cheatCharPos[j];
					nCodePos = nCodeArr[iCheatExpectedCharPos];
					nCodeChar = strCheatEntry->pEntry[nCodePos];
					if (nCodeChar == nLowerChar) {
						iCheatEntry = j;
						goto TRYAGAIN;
					}
				}
			}
			iCheatEntry = -1;
			goto GOBACK;
		}
		else {
			iCheatEntry = -1;
			goto GOBACK;
		}
		switch (strCheatEntry->iIndex) {
		case CHEAT_FUND:
			H_DoFund(25);
			break;
		case CHEAT_CASS:
			if (!Game_RandomWordLFSRMod(16)) {
				wSetTriggerDisasterType = DISASTER_FIRESTORM;
				Game_SimulationPrepareDiasterCoordinates(&dwDisasterPoint, wCityCenterX, wCityCenterY);
			}
			dwCityFunds += 250;
			break;
		case CHEAT_THEWORKS:
			pSCView = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
			if (pSCView)
				H_SimcityViewDebugGrantAllGifts(pSCView);
			break;
		case CHEAT_MAJORFLOOD:
			wSetTriggerDisasterType = DISASTER_MASSFLOODS;
			Game_SimulationPrepareDiasterCoordinates(&dwDisasterPoint, wCityCenterX, wCityCenterY);
			break;
		case CHEAT_PARTTHESEA:
			// An extrapolation of 'moses' from the Windows 3.1 game.
			// Once the code is activated it takes a moment for the
			// flood/wind to halt.
			if (dwDisasterActive) {
				if (wCurrentDisasterID == DISASTER_FLOOD ||
					wCurrentDisasterID == DISASTER_HURRICANE ||
					wCurrentDisasterID == DISASTER_MASSFLOODS) {
					if (wDisasterFloodArea > 0)
						wDisasterFloodArea = 0;
					if (wDisasterWindy > 0)
						wDisasterWindy = 0;
				}
			}
			break;
		case CHEAT_FIRESTORM:
			wSetTriggerDisasterType = DISASTER_FIRESTORM;
			Game_SimulationPrepareDiasterCoordinates(&dwDisasterPoint, wCityCenterX, wCityCenterY);
			break;
		case CHEAT_DEBUG:
			if (bPriscillaActivated)
				return;
			hMenu = GetMenu(hWnd);
			pMenu = H_CMenuFromHandle(hMenu);
			pDebugMenu = (DWORD*)operator new(8); // This would be CMenu().
			if (pDebugMenu)
				pDebugMenu[1] = 0;
			hDebugMenu = LoadMenuA(game_hModule, (LPCSTR)223);
			AdjustDebugMenu(hDebugMenu);
			H_CMenuAttach(pDebugMenu, hDebugMenu);
			iSCMenuPos = H_GetSimcityViewMenuPos(6);
			InsertMenuA((HMENU)pMenu[1], iSCMenuPos + 6, MF_BYPOSITION | MF_POPUP, pDebugMenu[1], szNewItem);
			H_SimcityAppAdjustNewspaperMenu(&pCSimcityAppThis);
			DrawMenuBar(hWnd);
			bPriscillaActivated = 1;
			break;
		case CHEAT_MILITARY:
			H_SimulationProposeMilitaryBase();
			break;
		case CHEAT_JOKE:
			H_JokeDialogConstruct((void*)&jokeDlg, 0);
			H_DialogDoModal((void*)&jokeDlg);
			H_ADialogDestruct((void*)&jokeDlg); // Function name references "A" dialog rather than anything specific.
			break;
		case CHEAT_WEBB:
			if (!FindTheHouse()) {
				if (!BuildTheHouse())
					L_MessageBoxA(hWnd, "Sorry, no room to build Ilona's house!", gamePrimaryKey, MB_ICONINFORMATION | MB_OK);
			}
			break;
		case CHEAT_OOPS:
			L_MessageBoxA(hWnd, "Same to you, buddy!", "Hey!", MB_ICONEXCLAMATION | MB_OK);
			if (iChurchVirus < 0)
				iChurchVirus = 0; // Warning
			else if (iChurchVirus == 0)
				iChurchVirus = 1; // You asked for it!
			break;
		case CHEAT_REPENT:
			if (iChurchVirus > 0) {
				if (L_MessageBoxA(hWnd, "Tea Father?", gamePrimaryKey, MB_ICONINFORMATION | MB_YESNO) == IDYES) {
					iChurchVirus = 0; // Set it back to 0 rather than -1; the next execution of the related cheats will result in immediate action.
					ChangeChurchZone();
				}
				else
					goto NO;
			}
			else {
				if (iChurchVirus == 0)
					iChurchVirus = -1; // Set back to -1 if executed once more.
			NO:
				L_MessageBoxA(hWnd, "Oh go on..", gamePrimaryKey, MB_ICONEXCLAMATION | MB_OK);
			}
			break;
		default:
			break;
		}
		iCheatEntry = -1;
		iCheatExpectedCharPos = 0;
		goto GOBACK;
	}

	iCheatEntry = -1;
	iCheatExpectedCharPos = 0;

	cheatMultipleDetections = FALSE;
	for (i = 0; i < NUM_CHEATS; ++i) {
		strCheatEntry = &cheatStrArray[i];
		if (strCheatEntry) {
			strCheatEntry->iPos = -1;
			if (*strCheatEntry->pEntry == nLowerChar) {
				strCheatEntry->iPos = i;
				if (iCheatEntry < 0) {
					iCheatExpectedCharPos = 1;
					iCheatEntry = strCheatEntry->iPos;
				}
				else
					cheatMultipleDetections = TRUE;
			}
		}
	}

GETOUT:
	if (iCheatEntry == -1)
		iCheatExpectedCharPos = 0;
}

// Call your cousin Vinnie!
void PorntipsGuzzardo(void) {
	// This case only occurs if the debug menu has been loaded
	// from the original non-hooked CMainFrame::OnChar function.
	hDebugMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(223));
	AdjustDebugMenu(hDebugMenu);

	// Hook for CMainFrame::OnChar
	VirtualProtect((LPVOID)0x4029E1, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4029E1, Hook_MainFrameOnChar);
}