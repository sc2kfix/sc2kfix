// sc2kfix hooks/hook_guzzardo.cpp: cheat handler replacement
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
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

#define NUM_CHEATS 15
#define NUM_CHEAT_MAXCHARS 9

typedef struct {
	int iIndex;          // Cheat index, match multiple cheats to the same index.
	const char* pEntry;  // Code entry
	int iPos;            // Position within the array. (Only set when there's a match)
} cheat_t;

enum {
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
static cheat_t arrScrambledCheats[NUM_CHEATS] = {
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
static int arrCheatScrambleKey[NUM_CHEATS][NUM_CHEAT_MAXCHARS] = {
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
static BOOL bCheatMultipleDetections = FALSE;
static const char* szIlonasHouseLabel = "Ilona's House";
int iChurchVirus = -1;

// Adds our new functionality to the priscilla debug menu.
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
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_FIX_BAD_TERRAIN, "Fix Bad Terrain") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_HIGHLIGHT_BAD_TERRAIN, "Highlight Bad Terrain") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_LABEL_CLEAR_ORPHANS, "Clear Orphaned Labels") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_LABEL_LIST_ORPHANS, "List Orphaned Labels") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #6 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_MLDEPLOY, "Delete Military Deploy 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #7 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_FRDEPLOY, "Delete Fire Deploy 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #8 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_PLDEPLOY, "Delete Police Deploy 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #9 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_TORNADO, "Delete Tornado 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #10 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_MONSTER, "Delete Monster 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #11 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_HERO, "Delete MaxisMan 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #12 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_TRAINS, "Delete Train 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #13 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_SAILBOATS, "Delete Sailboat 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #14 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_SHIPS, "Delete CargoShip 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #15 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_COPTERS, "Delete Helicopter 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #16 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_STRING, IDM_DEBUG_THING_CLEAN_PLANES, "Delete Airplane 'things'") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #17 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator after "Graph Kludge"
		if (!InsertMenu(hDebugPopup, 12, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #18 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		// Separator between the disasters and internal debugging functions.
		if (!InsertMenu(hDebugPopup, 11, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #19 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between grants and disasters
		if (!InsertMenu(hDebugPopup, 4, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #20 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between the version option and grants
		if (!InsertMenu(hDebugPopup, 1, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #21 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		// Insert in reverse order.
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_SPRITE_DISPLAY, "Browse Sprites") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #22 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #23 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_MISSILESILOS, "Propose Missile Silos") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #24 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_NAVALYARD, "Propose Naval Yard") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #25 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_ARMYBASE, "Propose Army Base") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #26 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_AIRFORCE, "Propose Air Force Base") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #27 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION | MF_STRING, IDM_DEBUG_MILITARY_DECLINED, "Stop Military Spawning") && guzzardo_debug & GUZZARDO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #28 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		if (guzzardo_debug & GUZZARDO_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated debug menu.\n");
	}
}

// Call for enabling and attaching the debug menu.
void EnableDebugMenu(CSimcityAppPrimary *pSCApp, HWND hWnd) {
	HMENU hMenu, hDebugMenu;
	CMFC3XMenu *pMenu, *pDebugMenu;
	int iSCMenuPos;

	if (!pSCApp->bSCAPriscillaActivated) {
		hMenu = GetMenu(hWnd);
		pMenu = GameMain_Menu_FromHandle(hMenu);
		pDebugMenu = new CMFC3XMenu();
		if (pDebugMenu)
			pDebugMenu->m_hMenu = 0;
		hDebugMenu = LoadMenuA(hGameModule, (LPCSTR)223);
		AdjustDebugMenu(hDebugMenu);
		GameMain_Menu_Attach(pDebugMenu, hDebugMenu);
		iSCMenuPos = Game_GetSimcityViewMenuPos(6);
		InsertMenuA(pMenu->m_hMenu, iSCMenuPos + 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR)pDebugMenu->m_hMenu, szNewItem);
		Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
		DrawMenuBar(hWnd);
		pSCApp->bSCAPriscillaActivated = TRUE;
	}
}

// Attempts to locate the XLAB entry for Ilona's House. Returns the XLAB entry ID if found or -1
// if not found.
static int FindTheHouseLabel() {
	const char *pLabel;
	for (int i = MIN_USER_TEXT_ENTRIES; i <= MAX_USER_TEXT_ENTRIES; ++i) {
		pLabel = GetXLABEntry(i);
		if (pLabel && _stricmp(pLabel, szIlonasHouseLabel) == 0) {
			return i;
		}
	}
	return -1;
}

// Allocates an XLAB entry for Ilona's House and creates a sign at the position requested.
static void SetTheHouseLabel(int iX, int iY) {
	BYTE iLabelIdx;

	if (XTXTGetTextOverlayID(iX, iY)) {
		if (XTXTGetTextOverlayID(iX, iY) > MAX_USER_TEXT_ENTRIES)
			return;
	}
	iLabelIdx = Game_PrepareLabel();
	if (iLabelIdx) {
		XTXTSetTextOverlayID(iX, iY, iLabelIdx);
		SetXLABEntry(iLabelIdx, szIlonasHouseLabel);
	}
}

// Attempts to find Ilona's House and returns whether or not it was successful.
static BOOL FindTheHouse() {
	__int16 xPos, yPos, xWindPos, ySignPos;
	__int16 iLength, iDepth, iLabelIdx;

	xPos = -1;
	yPos = -1;
	ySignPos = -1;
	for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
		for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
			if (GetTileID(iLength, iDepth) == TILE_COMMERCIAL_1X1_BEDANDBREAKFAST) {
				if (XZONReturnZone(iLength, iDepth) == ZONE_NONE) {
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
		Game_ItemPlacementCheck(xWindPos, yPos, TILE_POWERPLANT_WIND, AREA_1x1);
		Game_CenterOnTileCoords(xPos, yPos);
		return TRUE;
	}
	if (iLabelIdx >= MIN_USER_TEXT_ENTRIES && iLabelIdx <= MAX_USER_TEXT_ENTRIES) {
		for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
			for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
				if (XTXTGetTextOverlayID(iLength, iDepth) == iLabelIdx) {
					Game_RemoveLabel(iLabelIdx);
					XTXTSetTextOverlayID(iLength, iDepth, 0);
					break;
				}
			}
		}
	}
	return FALSE;
}

// Attempts to build Ilona's House and returns whether or not it was successful. Effectively,
// this chooses a random location to attempt to place a "bed and breakfast" tile with no zone and
// a solitary wind power plant next to it.
static BOOL BuildTheHouse() {
	int iAttempts;
	__int16 xPos;
	__int16 yPos;
	__int16 xWindPos;
	__int16 ySignPos;

	iAttempts = 0;
	while (TRUE) {
	RETRY:
		xPos = Game_RandomWordLFSRMod128();
		yPos = Game_RandomWordLFSRMod128();
		xWindPos = xPos - 1;
		ySignPos = yPos - 1;
		if (xWindPos < 0 || ySignPos < 0)
			goto RETRY;
		if (GetTileID(xPos, yPos) < TILE_SMALLPARK) {
			if (GetTileID(xPos, ySignPos) < TILE_SMALLPARK &&
				GetTileID(xWindPos, yPos) < TILE_SMALLPARK) {
				if (!GetTerrainTileID(xPos, yPos) &&
					!GetTerrainTileID(xPos, ySignPos) &&
					!GetTerrainTileID(xWindPos, yPos) &&
					(xPos < 0 || yPos >= GAME_MAP_SIZE || !XBITReturnIsWater(xPos, yPos)) &&
					(xPos >= GAME_MAP_SIZE || ySignPos >= GAME_MAP_SIZE || !XBITReturnIsWater(xPos, ySignPos)) &&
					(xWindPos >= GAME_MAP_SIZE || yPos >= GAME_MAP_SIZE || !XBITReturnIsWater(xWindPos, yPos))) {
					if (ALTMReturnLandAltitude(xPos, yPos) == ALTMReturnLandAltitude(xPos, ySignPos) &&
						ALTMReturnLandAltitude(xPos, yPos) == ALTMReturnLandAltitude(xWindPos, yPos)) {
						if (L_ItemPlacementCheck(xPos, yPos, TILE_COMMERCIAL_1X1_BEDANDBREAKFAST, AREA_1x1, false)) {
							SetTheHouseLabel(xPos, ySignPos);
							L_ItemPlacementCheck(xWindPos, yPos, TILE_POWERPLANT_WIND, AREA_1x1, false);
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

// Removes all churches on the map.
static void ChangeChurchZone() {
	__int16 iLength, iDepth;

	// Iterate through the map, replacing every church with random rubble and re-zone them as
	// dense residental. Note that we're *not* unsetting the powered/powerable bits, so
	// consequently once the tiles are replaced and re-zoned they will immediately grow.
	for (iLength = 0; iLength < GAME_MAP_SIZE; ++iLength) {
		for (iDepth = 0; iDepth < GAME_MAP_SIZE; ++iDepth) {
			if (GetTileID(iLength, iDepth) == TILE_INFRASTRUCTURE_CHURCH) {
				if (XZONReturnZone(iLength, iDepth) == ZONE_NONE) {
					Game_PlaceTile(iLength, iDepth, GetRubbleTileID());
					XZONSetNewZone(iLength, iDepth, ZONE_DENSE_RESIDENTIAL);
				}
			}
		}
	}
}

static BOOL CheatInputReturn_SC2K1996() {
	if (iCheatEntry == -1)
		iCheatExpectedCharPos = 0;
	return TRUE;
}

void ResetCheatInput_SC2K1996() {
	if (iCheatEntry != -1 || iCheatExpectedCharPos > 0)
		if (guzzardo_debug & GUZZARDO_DEBUG_OTHER)
			ConsoleLog(LOG_DEBUG, "GUZZ: Resetting cheat input state.\n");
	
	iCheatEntry = -1;
	CheatInputReturn_SC2K1996();
}

// Reimplemented and extended vanilla SC2K cheat handler.
// Replacement for CMainFrame::OnChar.
extern "C" void __stdcall Hook_MainFrame_OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	CMainFrame* pThis;

	__asm mov[pThis], ecx

	char nLowerChar;
	BOOL bComplete, bFail;
	int i, j;
	int nCurrPos;
	int* nCodeArr;
	int nCodePos;
	char nCodeChar;
	cheat_t* strCheatEntry;
	CSimcityAppPrimary *pSCApp;
	CSimcityView* pSCView;
	CJokeDialog jokeDlg;

	pSCApp = &pCSimcityAppThis;

	nLowerChar = tolower(nChar);
TRYAGAIN:
	bComplete = bFail = FALSE;
	if (iCheatEntry != -1) {
		strCheatEntry = &arrScrambledCheats[iCheatEntry]; // Cheat entry
		nCodeArr = arrCheatScrambleKey[iCheatEntry]; // Target character position reference array
		nCodePos = nCodeArr[iCheatExpectedCharPos];
		nCodeChar = strCheatEntry->pEntry[nCodePos];
		if (nCodeChar == nLowerChar) {
			nCurrPos = iCheatExpectedCharPos + 1;
			iCheatExpectedCharPos = nCurrPos;
			nCodePos = nCodeArr[nCurrPos];
			if (nCurrPos != NUM_CHEAT_MAXCHARS && nCodePos != -1) {
				if (CheatInputReturn_SC2K1996())
					return;
			}
		}
		else if (bCheatMultipleDetections) {
			for (i = 0; i < NUM_CHEATS; ++i) {
				if (i == iCheatEntry)
					continue;
				j = arrScrambledCheats[i].iPos;
				if (j >= 0) {
					strCheatEntry = &arrScrambledCheats[j];
					nCodeArr = arrCheatScrambleKey[j];
					nCodePos = nCodeArr[iCheatExpectedCharPos];
					nCodeChar = strCheatEntry->pEntry[nCodePos];
					if (nCodeChar == nLowerChar) {
						iCheatEntry = j;
						goto TRYAGAIN;
					}
				}
			}
			bFail = TRUE;
		}
		else
			bFail = TRUE;

		if (!bFail) {
			switch (strCheatEntry->iIndex) {
			case CHEAT_FUND:
				Game_DoFund(25);
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_CASS:
				if (!Game_RandomWordLFSRMod(16)) {
					wSetTriggerDisasterType = DISASTER_FIRESTORM;
					Game_SetCPoint(&disasterPoint, wCityCenterX, wCityCenterY);
				}
				dwCityFunds += 250;
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_THEWORKS:
				pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
				if (pSCView)
					Game_SimcityView_DebugGrantAllGifts(pSCView);
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_MAJORFLOOD:
				wSetTriggerDisasterType = DISASTER_MASSFLOODS;
				Game_SetCPoint(&disasterPoint, wCityCenterX, wCityCenterY);
				break;
			case CHEAT_PARTTHESEA:
				// An extrapolation of 'moses' from the Windows 3.1 game.
				// Once the code is activated it takes a moment for the
				// flood/wind to halt.
				if (dwDisasterActive) {
					if (wCurrentDisasterType == DISASTER_FLOOD ||
						wCurrentDisasterType == DISASTER_HURRICANE ||
						wCurrentDisasterType == DISASTER_MASSFLOODS) {
						if (wDisasterFloodArea > 0)
							wDisasterFloodArea = 0;
						if (wDisasterWindy > 0)
							wDisasterWindy = 0;
					}
				}
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_FIRESTORM:
				wSetTriggerDisasterType = DISASTER_FIRESTORM;
				Game_SetCPoint(&disasterPoint, wCityCenterX, wCityCenterY);
				break;
			case CHEAT_DEBUG:
				EnableDebugMenu(pSCApp, pThis->m_hWnd);
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_MILITARY:
				Game_SimulationProposeMilitaryBase();
				jsonXFIX["meta"]["porntipsguzzardo"] = true;
				break;
			case CHEAT_JOKE:
				Game_JokeDialog_Construct(&jokeDlg, 0);
				ToggleFloatingStatusDialog(FALSE);
				GameMain_Dialog_DoModal(&jokeDlg);
				ToggleFloatingStatusDialog(TRUE);
				Game_JokeDialog_Destruct(&jokeDlg); // Function name references "A" dialog rather than anything specific.
				break;
			case CHEAT_WEBB:
				if (!FindTheHouse()) {
					if (!BuildTheHouse())
						L_MessageBoxA(pThis->m_hWnd, "Sorry, no room to build Ilona's house!", gamePrimaryKey, MB_ICONINFORMATION | MB_OK);
				}
				break;
			case CHEAT_OOPS:
				L_MessageBoxA(pThis->m_hWnd, "Same to you, buddy!", "Hey!", MB_ICONEXCLAMATION | MB_OK);
				if (iChurchVirus < 0)
					iChurchVirus = 0; // Warning
				else if (iChurchVirus == 0)
					iChurchVirus = 1; // You asked for it!
				break;
			case CHEAT_REPENT:
				if (iChurchVirus > 0) {
					if (L_MessageBoxA(pThis->m_hWnd, "Tea Father?", gamePrimaryKey, MB_ICONINFORMATION | MB_YESNO) == IDYES) {
						iChurchVirus = 0; // Set it back to 0 rather than -1; the next execution of the related cheats will result in immediate action.
						ChangeChurchZone();
						jsonXFIX["meta"]["porntipsguzzardo"] = true;
					}
					else
						goto NO;
				}
				else {
					if (iChurchVirus == 0)
						iChurchVirus = -1; // Set back to -1 if executed once more.
				NO:
					L_MessageBoxA(pThis->m_hWnd, "Oh go on..", gamePrimaryKey, MB_ICONEXCLAMATION | MB_OK);
				}
				break;
			default:
				break;
			}
		}
		bComplete = TRUE;
	}

	iCheatEntry = -1;
	if (!bComplete) {
		iCheatExpectedCharPos = 0;

		bCheatMultipleDetections = FALSE;
		for (i = 0; i < NUM_CHEATS; ++i) {
			strCheatEntry = &arrScrambledCheats[i];
			if (strCheatEntry) {
				strCheatEntry->iPos = -1;
				if (*strCheatEntry->pEntry == nLowerChar) {
					strCheatEntry->iPos = i;
					if (iCheatEntry < 0) {
						iCheatExpectedCharPos = 1;
						iCheatEntry = strCheatEntry->iPos;
					}
					else
						bCheatMultipleDetections = TRUE;
				}
			}
		}
	}
	CheatInputReturn_SC2K1996();
}

// Call your cousin Vinnie!
void PorntipsGuzzardo(void) {
	// This case only occurs if the debug menu has been loaded
	// from the original non-hooked CMainFrame::OnChar function.
	hDebugMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(223));
	AdjustDebugMenu(hDebugMenu);

	// Hook for CMainFrame::OnChar
	SafeVirtualProtect((LPVOID)0x4029E1, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4029E1, Hook_MainFrame_OnChar);
}
