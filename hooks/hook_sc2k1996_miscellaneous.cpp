// sc2kfix hooks/hook_sc2k1996_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is where I test a bunch of stuff live to cross reference what I think is going on in the
// game engine based on decompiling things in IDA and following the code paths. As a result,
// there's a lot of experimental stuff in here. Comments will probably be unhelpful. Godspeed.

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

#define MISCHOOK_DEBUG_OTHER 1
#define MISCHOOK_DEBUG_MILITARY 2
#define MISCHOOK_DEBUG_MENU 4
#define MISCHOOK_DEBUG_SAVES 8
#define MISCHOOK_DEBUG_WINDOW 16
#define MISCHOOK_DEBUG_DISASTERS 32
#define MISCHOOK_DEBUG_MOVIES 64
#define MISCHOOK_DEBUG_SMACK 128
#define MISCHOOK_DEBUG_CHEAT 256

#define MISCHOOK_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define MAX_USER_LABELS 51

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

AFX_MSGMAP_ENTRY afxMessageMapMainMenu[9];
DLGPROC lpNewCityAfxProc = NULL;
char szTempMayorName[24] = { 0 };

static BOOL bOverrideTickPlacementHighlight = FALSE;

static int iChurchVirus = -1;

static const char *theHouse = "Ilona's House";

// Override some strings that have egregiously bad grammar/capitalization.
// Maxis fail English? That's unpossible!
extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	if (hInstance == hSC2KAppModule && bSettingsUseNewStrings) {
		switch (uID) {
		case 97:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Hydroelectric Dam"))
				return strlen(lpBuffer);
			break;
		case 108:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Hydroelectric dams can only be placed on waterfall tiles."))
				return strlen(lpBuffer);
			break;
		case 111:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would intersect an existing tunnel."))
				return strlen(lpBuffer);
			break;
		case 112:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would leave the city limits."))
				return strlen(lpBuffer);
			break;
		case 113:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would be too deep in the terrain."))
				return strlen(lpBuffer);
			break;
		case 114:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as the exit terrain is unstable."))
				return strlen(lpBuffer);
			break;
		case 115:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"An existing subway or sewer line is blocking construction."))
				return strlen(lpBuffer);
			break;
		case 116:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel entrances must be placed on a hillside."))
				return strlen(lpBuffer);
			break;
		case 129:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Nuclear Power"))
				return strlen(lpBuffer);
			break;
		case 132:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Microwave Power"))
				return strlen(lpBuffer);
			break;
		case 133:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Fusion Power"))
				return strlen(lpBuffer);
			break;
		case 240:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Your nation's military is interested in building a base on your city's soil. "
				"This could mean extra revenue. It could also raise new problems. "
				"Do you wish to grant land to the military?"))
				return strlen(lpBuffer);
			break;
		case 289:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Current rates are %d%%.\r\n"
				"Do you wish to issue the bond?"))
				return strlen(lpBuffer);
			break;
		case 290:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"You need $10,000 in cash to repay an outstanding bond."))
				return strlen(lpBuffer);
			break;
		case 291:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"The oldest outstanding bond rate is %d%%.\r\n"
				"Do you wish to repay this bond?"))
				return strlen(lpBuffer);
			break;
		case 346:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Engineers report that tunnel construction costs will be %s.\r\n"
				"Do you wish to construct the tunnel?"))
				return strlen(lpBuffer);
			break;
		case 640:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Grocery store"))
				return strlen(lpBuffer);
			break;
		case 745:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Launch Arcology"))
				return strlen(lpBuffer);
			break;
		case 4002:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"SimCity 2000 City (*.SC2)|*.SC2|SimCity Classic City (*.CTY)|*.CTY||"))
				return strlen(lpBuffer);
			break;
		case 4004:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"SimCity 2000 Tilesets (*.mif)|*.mif||"))
				return strlen(lpBuffer);
			break;
		case 32921:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Saves city every 5 years"))
				return strlen(lpBuffer);
			break;
		default:
			return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
		}
	}
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

#pragma warning(disable : 6387)
// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 3 && hGameMenu)
		return hGameMenu;
	if ((DWORD)lpMenuName == 223 && hDebugMenu)
		return hDebugMenu;
	return LoadMenuA(hInstance, lpMenuName);
}
#pragma warning(default : 6387)

// Make sure our own menu items get enabled instead of disabled
extern "C" BOOL __stdcall Hook_EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable) {
	// XXX - There's gotta be a better way to do this.
	if (uIDEnableItem == 5 && uEnable == 0x403)
		return EnableMenuItem(hMenu, uIDEnableItem, MF_BYPOSITION | MF_ENABLED);
	if (uIDEnableItem == 6 && uEnable == 0x403)
		return EnableMenuItem(hMenu, uIDEnableItem, MF_BYPOSITION | MF_ENABLED);
	return EnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

extern "C" BOOL __stdcall Hook_ShowWindow(HWND hWnd, int nCmdShow) {
	if (mischook_debug & MISCHOOK_DEBUG_WINDOW)
		ConsoleLog(LOG_DEBUG, "WND:  0x%08X -> ShowWindow(0x%08X, %i)\n", _ReturnAddress(), hWnd, nCmdShow);

	HWND hWndStatusBar = (HWND)((DWORD*)pCWndRootWindow)[68];
	if (hWnd == hWndStatusBar && bSettingsUseStatusDialog) {
		if (hStatusDialog)
			ShowWindow(hStatusDialog, SW_SHOW);
		return ShowWindow(hWnd, SW_HIDE);
	}

	// Workaround for the game window not showing if started by a launcher process
	if (nCmdShow == 11 && (DWORD)_ReturnAddress() == 0x40586C)
		return ShowWindow(hWnd, SW_MAXIMIZE);

	return ShowWindow(hWnd, nCmdShow);
}

extern "C" DWORD __cdecl Hook_SmackOpen(LPCSTR lpFileName, uint32_t uFlags, int32_t iExBuf) {
	if (mischook_debug & MISCHOOK_DEBUG_SMACK)
		ConsoleLog(LOG_DEBUG, "SMK:  0x%08X -> _SmackOpen(%s, %u, %i)\n", _ReturnAddress(), lpFileName, uFlags, iExBuf);

	if (!smk_enabled || bSkipIntro || bSettingsAlwaysSkipIntro)
		if (strrchr(lpFileName, '\\'))
			if (!strcmp(strrchr(lpFileName, '\\'), "\\INTROA.SMK") || !strcmp(strrchr(lpFileName, '\\'), "\\INTROB.SMK"))
				return NULL;

	char buf[MAX_PATH + 1];

	memset(buf, 0, sizeof(buf));

	return SMKOpenProc(AdjustSource(buf, lpFileName), uFlags, iExBuf);
}

extern "C" DWORD __cdecl Hook_MovieCheck(char *sMovStr) {
	if (sMovStr && strncmp(sMovStr, "INTRO", 5) == 0) {
		if (!smk_enabled || bSkipIntro || bSettingsAlwaysSkipIntro)
			return 1;
	}

	return Game_Direct_MovieCheck(sMovStr);
}

extern "C" void __stdcall Hook_ApparentExit(void) {
	DWORD pThis;

	__asm mov [pThis], ecx

	// 0x405FCF - GameDoIdleUpKeep() - toolmenu item from prior to the game starting. (This one can be ignored)
	// 0x4A26D6 - DispatchCmdMsg() (library function) - this one is hit when you use the 'Exit' menu item.
	// 0x40A6C8 - This one is hit when you press the close gadget or goto close in the "Main Window" top-level menu. (or if you press Alt + F4)
	// 0x481EC6 - This one is hit when you goto close in the "Game Window" top-level menu.

	int iReqRet;
	int iSource;
	DWORD dwOldVal1, dwOldVal2;

	iSource = *((DWORD *)pThis + 206);
	dwOldVal1 = *((DWORD *)pThis + 64);
	dwOldVal2 = *((DWORD *)pThis + 63); // If this is '1' by default this appears to indicate 'Quit' was clicked from the pregame menu dialog.

	// One of the two (or both) suspend the simulation.
	*((DWORD *)pThis + 64) = 1;
	*((DWORD *)pThis + 63) = 0;
	iReqRet = Game_ExitRequester((void *)pThis, iSource);
	if (iReqRet != IDCANCEL) {
		if (iReqRet == IDYES)
			Game_DoSaveCity((void *)pThis);
		Game_PreGameMenuDialogToggle(*((void **)pThis + 7), 0);
		Game_CWinApp_OnAppExit((void *)pThis);
		return;
	}
	// This case is hit when you click "Cancel", you then want to restore both old values in order
	// for the simulation to properly resume (based on current tests).
	*((DWORD *)pThis + 64) = dwOldVal1;
	*((DWORD *)pThis + 63) = dwOldVal2;
}

std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_Before;
std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_After;

extern "C" void __stdcall Hook_GameDoIdleUpkeep(void) {
	DWORD pThis;
	__asm mov [pThis], ecx
	for (const auto& hook : stHooks_Hook_GameDoIdleUpkeep_Before) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*) = (void(*)(void*))hook.pFunction;
			fnHook((void*)pThis);
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

	__asm {
		mov ecx, [pThis]
		mov edi, 0x405AB0
		call edi
	}

	for (const auto& hook : stHooks_Hook_GameDoIdleUpkeep_After) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*) = (void(*)(void*))hook.pFunction;
			fnHook((void*)pThis);
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

BAIL:
	return;
}

// Fix up a specific setting of the GameDoIdleUpkeep state
void __declspec(naked) Hook_4062AD(void) {
	__asm {
		mov dword ptr [ecx+0x14C], 1
		mov dword ptr [edi], 3
		push 0x4062BD
		retn
	}
}

std::vector<hook_function_t> stHooks_Hook_OnNewCity_Before;

static BOOL CALLBACK Hook_NewCityDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, 150, szSettingsMayorName);
		break;
	case WM_DESTROY:
		// XXX - there's probably a better window message to use here.
		memset(szTempMayorName, 0, 24);
		if (!GetDlgItemText(hwndDlg, 150, szTempMayorName, 24))
			strcpy_s(szTempMayorName, 24, szSettingsMayorName);

		strcpy_s(dwMapXLAB[0][0].szLabel, 24, szTempMayorName);

		// XXX - this should probably be moved to a separate proper hook into the game itself
		for (const auto& hook : stHooks_Hook_OnNewCity_Before) {
			bHookStopProcessing = FALSE;
			if (hook.iType == HOOKFN_TYPE_NATIVE) {
				void (*fnHook)(void) = (void(*)(void))hook.pFunction;
				fnHook();
			}
		}
		break;
	}

	return lpNewCityAfxProc(hwndDlg, message, wParam, lParam);
}

#pragma warning(disable : 6387)
// Load our own version of the main menu and the New City dialog when called
extern "C" INT_PTR __stdcall Hook_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	switch ((DWORD)lpTemplateName) {
	case 101:
		lpNewCityAfxProc = lpDialogFunc;
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, Hook_NewCityDialogProc, dwInitParam);
	case 102:
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	case 103:
		if (bUpdateAvailable)
			return DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(103), hWndParent, lpDialogFunc, dwInitParam);
		return DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(20104), hWndParent, lpDialogFunc, dwInitParam);
	default:
		return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}
}
#pragma warning(default : 6387)

// Fix rail and highway border connections not loading properly
extern "C" void __stdcall Hook_LoadNeighborConnections1500(void) {
	short* wCityNeighborConnections1500 = (short*)0x4CA3F0;
	*wCityNeighborConnections1500 = 0;
	*(DWORD*)0x4C85A0 = 0;

	for (int x = 0; x < GAME_MAP_SIZE; x++) {
		for (int y = 0; y < GAME_MAP_SIZE; y++) {
			if (dwMapXTXT[x][y].bTextOverlay == 0xFA) {
				BYTE iTileID = dwMapXBLD[x][y].iTileID;
				if (iTileID >= TILE_RAIL_LR && iTileID < TILE_TUNNEL_T
					|| iTileID >= TILE_CROSSOVER_ROADLR_RAILTB && iTileID < TILE_SUSPENSION_BRIDGE_START_B
					|| iTileID >= TILE_HIGHWAY_HTB && iTileID < TILE_REINFORCED_BRIDGE_PYLON)
					++*wCityNeighborConnections1500;
			}
		}
	}

	if (mischook_debug & MISCHOOK_DEBUG_SAVES)
		ConsoleLog(LOG_DEBUG, "SAVE: Loaded %d $1500 neighbor connections.\n", *wCityNeighborConnections1500);
}

extern "C" int __cdecl Hook_PlacePowerLinesAtCoordinates(__int16 x, __int16 y) {
	__int16 iY;
	int iResult;
	unsigned int iTileID;

	iY = y;
	if (x < 0) {
TOBEGINNING:
		iTileID = (unsigned int)dwMapXBLD[x];
		P_LOBYTE(iTileID) = ((map_XBLD_t *)(iTileID))[iY].iTileID;
		iResult = iTileID & 0xFFFF00FF;
		if ((__int16)iResult < TILE_POWERLINES_LR) {
			Game_PlaceTileWithMilitaryCheck(x, iY, TILE_POWERLINES_LR);
TOTHISPART:
			if (x < GAME_MAP_SIZE && iY < GAME_MAP_SIZE)
				dwMapXBIT[x][iY].b.iPowerable = 1;
			iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x, iY);
			if (x > 0)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x - 1, iY);
			if (x < GAME_MAP_SIZE-1)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x + 1, iY);
			if (iY > 0)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x, iY - 1);
			if (iY < GAME_MAP_SIZE-1)
				return Game_CheckAdjustTerrainAndPlacePowerLines(x, iY + 1);
		}
		else {
			switch ((__int16)iResult) {
				case TILE_ROAD_LR:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_POWERTB_ROADLR);
					goto TOTHISPART;
				case TILE_ROAD_TB:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_POWERLR_ROADTB);
					goto TOTHISPART;
				case TILE_RAIL_LR:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_POWERTB_RAILLR);
					goto TOTHISPART;
				case TILE_RAIL_TB:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_POWERLR_RAILTB);
					goto TOTHISPART;
				case TILE_HIGHWAY_LR:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_HIGHWAYLR_POWERTB);
					goto TOTHISPART;
				case TILE_HIGHWAY_TB:
					Game_PlaceTileWithMilitaryCheck(x, iY, TILE_CROSSOVER_HIGHWAYTB_POWERLR);
					goto TOTHISPART;
				default:
					return iResult;
			}
		}
		return iResult;
	}
	if (x >= GAME_MAP_SIZE)
		goto TOBEGINNING;
	if (y >= GAME_MAP_SIZE)
		goto TOBEGINNING;
	iResult = x;
	if (*(BYTE *)&dwMapXBIT[x][y].b >= 0)
		goto TOBEGINNING;
	return iResult;
}

extern "C" void __stdcall Hook_ResetGameVars(void) {
	void(__stdcall *H_ResetGameVars)(void) = (void(__stdcall *)(void))0x4348E0;
	int(__thiscall *H_RotateAntiClockwise)(DWORD *) = (int(__thiscall *)(DWORD *))0x401A73;

	BOOL bMapEditor, bNewGame;

	bMapEditor = ((DWORD)_ReturnAddress() == 0x42DF13);
	bNewGame = ((DWORD)_ReturnAddress() == 0x42E482);
	if (bMapEditor || bNewGame) {
		DWORD *pThis;

		pThis = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);

		if (((__int16)wCityMode < 0 && bNewGame) || bMapEditor) {
			if (wViewRotation != VIEWROTATION_NORTH) {
				do
					H_RotateAntiClockwise(pThis);
				while (wViewRotation != VIEWROTATION_NORTH);
				UpdateWindow((HWND)pThis[7]); // This would be pThis->m_hWnd if the structs were present.
			}
		}
	}

	H_ResetGameVars();
}

extern "C" int __cdecl Hook_SimulationGrowthTick(signed __int16 iStep, signed __int16 iSubStep) {
#if 1
	DWORD *pThis;
	int iAttributes;
	int iResult;
	__int16 iX;
	__int16 iY;
	__int16 i;
	__int16 iXMM;
	__int16 iYMM;
	BOOL bPlaceChurch;
	int iXPos;
	map_XZON_attribs_t maXZON;
	__int16 iCurrZoneType;
	int iPosAttributes;
	__int16 iTileID;
	__int16 iBuildingCount;
	signed __int16 iFundingPercent;
	__int16 iBuildingPopLevel;
	char iRandSelectOne;
	__int16 iCalculateResDemand;
	__int16 iRemainderResDemand;
	__int16 iMapValPerhaps;
	__int16 iReplaceTile;
	__int16 iNextX;
	__int16 iNextY;

	pThis = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
	iAttributes = dwCityPopulation;
	iX = iStep;
	if (iChurchVirus > 0)
		bPlaceChurch = 1;
	else
		bPlaceChurch = 2500u * (__int16)dwTileCount[TILE_INFRASTRUCTURE_CHURCH] < (unsigned int)dwCityPopulation;
	wCurrentAngle = wPositionAngle[wViewRotation];
	iResult = iStep / 2;
	iXMM = iStep / 2;
	while (iX < GAME_MAP_SIZE) {
		iY = iSubStep;
		for (i = iSubStep / 2; ; i = iY / 2) {
			iYMM = i;
			if (iY >= GAME_MAP_SIZE)
				break;
			iXPos = iX;
			maXZON = dwMapXZON[iXPos][iY].b;
			iCurrZoneType = maXZON.iZoneType;
			iPosAttributes = iXPos * 4;
			P_LOBYTE(iPosAttributes) = dwMapXBLD[iXPos][iY].iTileID;
			P_LOBYTE(iAttributes) = iPosAttributes;
			iAttributes &= 0xFFFF00FF;
			if (maXZON.iZoneType != ZONE_NONE) {
				if (maXZON.iZoneType > ZONE_DENSE_INDUSTRIAL) {
					switch (iCurrZoneType) {
						case ZONE_MILITARY:
							// (I / N): For the most part the divisor is the number of tiles that the
							// given building-type occupies, so it divides by that to get the actual
							// number of buildings present.
							//
							// As far as I can tell when it comes to the 'MILITARYTILE_MHANGAR1' case...
							// 12 of those type could be classified as a unit, so if iAttributes is greater
							// than that unit, place more hangars.
							// (old method - iAttribtues, new method - iBuildingCount)
							//
							// That crops up the most on Army and Naval cases.
							switch (bMilitaryBaseType) {
								case MILITARY_BASE_ARMY:
									if ((rand() & 3) == 0) {
#if 0
										P_LOWORD(iAttributes) = (__int16)dwMilitaryTiles[MILITARYTILE_MPARKINGLOT] / 4;
										iTileID = TILE_MILITARY_PARKINGLOT;
										if ((__int16)iAttributes > (__int16)dwMilitaryTiles[MILITARYTILE_MHANGAR1] / 12)
											iTileID = TILE_MILITARY_HANGAR1;
#else
										iBuildingCount = (__int16)dwMilitaryTiles[MILITARYTILE_MPARKINGLOT] / 4;
										if ((__int16)dwMilitaryTiles[MILITARYTILE_TOPSECRET] / 4 < iBuildingCount) {
											iTileID = TILE_MILITARY_HANGAR1;
											if ((__int16)dwMilitaryTiles[MILITARYTILE_MHANGAR1] / 12 >= iBuildingCount)
												iTileID = TILE_MILITARY_TOPSECRET;
										}
										else
											iTileID = TILE_MILITARY_PARKINGLOT;
#endif
										if (!Game_SimulationGrowSpecificZone(iX, iY, iTileID, ZONE_MILITARY))
											Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_HANGAR1, ZONE_MILITARY);
									}
									break;
								case MILITARY_BASE_AIR_FORCE:
									if ((rand() & 3) == 0) {
										iAttributes = 5;
										iBuildingCount = ((__int16)dwMilitaryTiles[MILITARYTILE_RUNWAY] + (__int16)dwMilitaryTiles[MILITARYTILE_RUNWAYCROSS]) / 5;
										if ((__int16)dwMilitaryTiles[MILITARYTILE_MPARKINGLOT] / 4 < iBuildingCount) {
											if (2 * (__int16)dwMilitaryTiles[MILITARYTILE_MCONTROLTOWER] >= iBuildingCount) {
												if (2 * (__int16)dwMilitaryTiles[MILITARYTILE_MRADAR] >= iBuildingCount) {
													if ((__int16)dwMilitaryTiles[MILITARYTILE_F15B] >= iBuildingCount) {
														if ((__int16)dwMilitaryTiles[MILITARYTILE_BUILDING1] / 2 >= iBuildingCount) {
															if ((__int16)dwMilitaryTiles[MILITARYTILE_BUILDING2] / 2 >= iBuildingCount) {
																iTileID = TILE_INFRASTRUCTURE_HANGAR2;
																if ((__int16)dwMilitaryTiles[MILITARYTILE_HANGAR2] / 4 >= iBuildingCount)
																	iTileID = TILE_MILITARY_PARKINGLOT;
															}
															else
																iTileID = TILE_INFRASTRUCTURE_BUILDING2;
														}
														else
															iTileID = TILE_INFRASTRUCTURE_BUILDING1;
													}
													else
														iTileID = TILE_MILITARY_F15B;
												}
												else
													iTileID = TILE_MILITARY_RADAR;
											}
											else
												iTileID = TILE_MILITARY_CONTROLTOWER;
										}
										else
											iTileID = TILE_INFRASTRUCTURE_RUNWAY;
										goto GOSPAWNAIRFIELD;
									}
									break;
								case MILITARY_BASE_NAVY:
									if ((rand() & 3) == 0) {
										iBuildingCount = dwMilitaryTiles[MILITARYTILE_CRANE];
										if ((__int16)dwMilitaryTiles[MILITARYTILE_CARGOYARD] / 4 < iBuildingCount) {
											if ((__int16)dwMilitaryTiles[MILITARYTILE_TOPSECRET] / 4 >= iBuildingCount) {
												BYTE(iAttributes) = 0;
												iTileID = TILE_MILITARY_WAREHOUSE;
												if ((__int16)dwMilitaryTiles[MILITARYTILE_MWAREHOUSE] / 3 >= iBuildingCount)
													iTileID = TILE_INFRASTRUCTURE_CARGOYARD;
											}
											else
												iTileID = TILE_MILITARY_TOPSECRET;
										}
										else
											iTileID = TILE_INFRASTRUCTURE_CRANE;
										if (!Game_SimulationGrowSpecificZone(iX, iY, iTileID, ZONE_MILITARY))
											goto GOSPAWNSEAYARD;
									}
									break;
								case MILITARY_BASE_MISSILE_SILOS:
									if ((BYTE)iPosAttributes != TILE_MILITARY_MISSILESILO)
										Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_MISSILESILO, ZONE_MILITARY);
									break;
								default:
									goto GOUNDCHECKTHENYINCREASE;
							}
							break;
						case ZONE_SEAPORT:
							if ((rand() & 3) != 0) {
								if ((WORD)iAttributes == TILE_INFRASTRUCTURE_CRANE && (rand() & 3) == 0)
									Game_SpawnShip(iX, iY);
							}
							else {
								iBuildingCount = dwTileCount[TILE_INFRASTRUCTURE_CRANE];
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_CARGOYARD] / 4 < iBuildingCount) {
									if ((__int16)dwTileCount[TILE_MILITARY_LOADINGBAY] / 4 >= iBuildingCount) {
										BYTE(iAttributes) = 0;
										iTileID = TILE_MILITARY_WAREHOUSE;
										if ((__int16)dwTileCount[TILE_MILITARY_WAREHOUSE] / 3 >= iBuildingCount)
											iTileID = TILE_INFRASTRUCTURE_CARGOYARD;
									}
									else
										iTileID = TILE_MILITARY_LOADINGBAY;
								}
								else
									iTileID = TILE_INFRASTRUCTURE_CRANE;
								if (!Game_SimulationGrowSpecificZone(iX, iY, iTileID, ZONE_SEAPORT)) {
GOSPAWNSEAYARD:
									Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_WAREHOUSE, iCurrZoneType);
								}
							}
							break;
						case ZONE_AIRPORT:
							if ((rand() & 3) != 0) {
								if ((WORD)iAttributes == TILE_INFRASTRUCTURE_RUNWAY &&
									!(rand() % 30) &&
									iX < GAME_MAP_SIZE &&
									iY < GAME_MAP_SIZE) {
									iAttributes = (int)&dwMapXBIT[iXPos];
									if (dwMapXBIT[iX][iY].b.iPowered != 0) {
										if (rand() % 10 < 4) {
											Game_SpawnHelicopter(iX, iY);
											break;
										}
										if ((wViewRotation & 1) != 0) {
											if (iX < GAME_MAP_SIZE &&
												iY < GAME_MAP_SIZE &&
												((map_XBIT_bits_t *)iAttributes + iY)->iRotated != 0)
												goto AIRFIELDSKIPAHEAD;
										}
										else if (iX >= GAME_MAP_SIZE ||
											iY >= GAME_MAP_SIZE ||
											((map_XBIT_bits_t *)iAttributes + iY)->iRotated == 0) {
AIRFIELDSKIPAHEAD:
											Game_SpawnAeroplane(iX, iY, 0);
											break;
										}
										Game_SpawnAeroplane(iX, iY, 2);
									}
								}
							}
							else {
								iAttributes = 5;
								iBuildingCount = ((__int16)dwTileCount[TILE_INFRASTRUCTURE_RUNWAY] + (__int16)dwTileCount[TILE_INFRASTRUCTURE_RUNWAYCROSS]) / 5;
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_PARKINGLOT] / 4 < iBuildingCount) {
									if (2 * (__int16)dwTileCount[TILE_INFRASTRUCTURE_CONTROLTOWER_CIV] >= iBuildingCount) {
										if (2 * (__int16)dwTileCount[TILE_MILITARY_RADAR] >= iBuildingCount) {
											if ((__int16)dwTileCount[TILE_MILITARY_TARMAC] >= iBuildingCount) {
												if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_BUILDING1] / 2 >= iBuildingCount) {
													if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_BUILDING2] / 2 >= iBuildingCount) {
														iTileID = TILE_INFRASTRUCTURE_HANGAR2;
														if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_HANGAR2] / 4 >= iBuildingCount)
															iTileID = TILE_INFRASTRUCTURE_PARKINGLOT;
													}
													else
														iTileID = TILE_INFRASTRUCTURE_BUILDING2;
												}
												else
													iTileID = TILE_INFRASTRUCTURE_BUILDING1;
											}
											else
												iTileID = TILE_MILITARY_TARMAC;
										}
										else
											iTileID = TILE_MILITARY_RADAR;
									}
									else
										iTileID = TILE_INFRASTRUCTURE_CONTROLTOWER_CIV;
								}
								else
									iTileID = TILE_INFRASTRUCTURE_RUNWAY;
GOSPAWNAIRFIELD:
								Game_SimulationGrowSpecificZone(iX, iY, iTileID, iCurrZoneType);
							}
							break;
						default:
							break;
					}
				}
				else {
					if ((__int16)iAttributes >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
						if ((*(BYTE *)&maXZON & 0xF0 & wCurrentAngle) == 0) {
							// This case appears to be hit with >= 2x2 zoned items.
							goto GOUNDCHECKTHENYINCREASE;
						}
						P_LOWORD(iAttributes) = iAttributes - TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1;
						iBuildingPopLevel = wBuildingPopLevel[(__int16)iAttributes];
					}
					else {
						if ((__int16)iAttributes >= TILE_ROAD_LR || !Game_IsValidTransitItems(iX, iY)) {
							goto GOUNDCHECKTHENYINCREASE;
						}
						P_LOWORD(iAttributes) = 0;
						iBuildingPopLevel = 0;
					}
					if (Game_IsZonedTilePowered(iX, iY)) {
						if (Game_UpdateDisasterAndTransitStats(iX, iY, iCurrZoneType, iBuildingPopLevel, 100)) {
							iCalculateResDemand = wCityResidentialDemand[(__int16)((iCurrZoneType - 1) / 2)] + 2000;
							iRemainderResDemand = 4000 - iCalculateResDemand;
						}
						else {
							iCalculateResDemand = 0;
							iRemainderResDemand = 4000;
						}
					}
					else {
						iCalculateResDemand = 0;
						iRemainderResDemand = 4000;
					}
					// This block is encountered when a given area is not "under construction" and not "abandonded".
					// A building is then randomly selected in 
					if (iBuildingPopLevel > 0 && !bAreaState[(__int16)iAttributes]) {
						pZonePops[iCurrZoneType] += wBuildingPopulation[iBuildingPopLevel]; // Values appear to be: 1[1], 8[2], 12[3], 36[4] (wBuildingPopulation[iBuildingPopLevel] format.
						if ((unsigned __int16)rand() < (__int16)(iRemainderResDemand / iBuildingPopLevel)) {
							iRandSelectOne = rand() & 1;
							Game_PerhapsGeneralZoneChangeBuilding(iX, iY, iBuildingPopLevel, iRandSelectOne);
							goto GOUNDCHECKTHENYINCREASE;
						}
					}
					iAttributes = (int)&bAreaState[(__int16)iAttributes];
					if (*(BYTE *)iAttributes == 1 && (unsigned __int16)rand() < 0x4000 / iBuildingPopLevel) {
						if (bPlaceChurch && (iBuildingPopLevel & 2) != 0 && iCurrZoneType < ZONE_LIGHT_COMMERCIAL) {
							Game_PlaceChurch(iX, iY);
							goto GOUNDCHECKTHENYINCREASE;
						}
GOGENERALZONEITEMPLACE:
						Game_PerhapsGeneralZoneChooseAndPlaceBuilding(iX, iY, iBuildingPopLevel, (iCurrZoneType - 1) / 2);
						goto GOUNDCHECKTHENYINCREASE;
					}
					if (*(BYTE *)iAttributes == 2) {
						// Abandoned buildings.
						iAttributes = iBuildingPopLevel;
						pZonePops[ZONEPOP_ABANDONED] += wBuildingPopulation[iBuildingPopLevel];
						if ((unsigned __int16)rand() >= 15 * iCalculateResDemand / iBuildingPopLevel) {
							goto GOUNDCHECKTHENYINCREASE;
						}
						goto GOGENERALZONEITEMPLACE;
					}
					// This block is where construction will start.
					if (iBuildingPopLevel != 4 &&
						((iCurrZoneType & 1) == 0 || iBuildingPopLevel <= 0) &&
						(iCurrZoneType >= 5 ||
						(iBuildingPopLevel != 1 || dwMapXVAL[iXMM][iYMM].bBlock >= 0x20u) &&
							(iBuildingPopLevel != 2 || dwMapXVAL[iXMM][iYMM].bBlock >= 0x60u) &&
							(iBuildingPopLevel != 3 || dwMapXVAL[iXMM][iYMM].bBlock >= 0xC0u))) {
						iAttributes = 3 * iCalculateResDemand / (iBuildingPopLevel + 1);
						if (iAttributes > (unsigned __int16)rand()) {
							Game_PerhapsGeneralZoneStartBuilding(iX, iY, iBuildingPopLevel, iCurrZoneType);
						}
					}
				}
			}
			else {
				if ((__int16)iAttributes < TILE_ROAD_LR)
					goto GOUNDCHECKTHENYINCREASE;
				if ((unsigned __int16)Game_RandomWordLFSRMod128())
					goto GOAFTERSETXBIT;
				if ((__int16)iAttributes >= TILE_ROAD_LR && (__int16)iAttributes < TILE_RAIL_LR ||
					(__int16)iAttributes >= TILE_CROSSOVER_POWERTB_ROADLR && (__int16)iAttributes < TILE_CROSSOVER_POWERTB_RAILLR ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYLR_ROADTB ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYTB_ROADLR ||
					(__int16)iAttributes >= TILE_ONRAMP_TL && (__int16)iAttributes < TILE_HIGHWAY_HTB) {
					// Transportation budget, roads - if below 100% related tiles will be replaced with rubble.
					iFundingPercent = pBudgetArr[BUDGET_ROAD].iFundingPercent;
					if (iFundingPercent != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iFundingPercent) {
						iRandSelectOne = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iRandSelectOne);
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE) {
							goto GOAFTERSETXBIT;
						}
						goto GOBEFORESETXBIT;
					}
				}
				else if ((__int16)iAttributes >= TILE_RAIL_LR && (__int16)iAttributes < TILE_TUNNEL_T ||
					(__int16)iAttributes >= TILE_CROSSOVER_ROADLR_RAILTB && (__int16)iAttributes < TILE_HIGHWAY_LR ||
					(__int16)iAttributes >= TILE_SUBTORAIL_T && (__int16)iAttributes < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYLR_RAILTB ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYTB_RAILLR) {
					// Transportation budget, rails - if below 100% related tiles will be replaced with rubble.
					iFundingPercent = pBudgetArr[BUDGET_RAIL].iFundingPercent;
					if (iFundingPercent != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iFundingPercent) {
						iRandSelectOne = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iRandSelectOne);
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE) {
							goto GOAFTERSETXBIT;
						}
GOBEFORESETXBIT:
						*(BYTE *)&dwMapXBIT[iX][iY].b &= ~0x80u;
GOAFTERSETXBIT:
						if ((__int16)iAttributes >= TILE_INFRASTRUCTURE_RAILSTATION) {
							if ((WORD)iAttributes == TILE_INFRASTRUCTURE_RAILSTATION &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iY].b.iPowered != 0 &&
								!(unsigned __int16)Game_RandomWordLFSRMod4()) {
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_RAILSTATION] / 4 > wActiveTrains) {
									Game_SpawnTrain(iX, iY);
								}
							}
							else if ((WORD)iAttributes == TILE_INFRASTRUCTURE_MARINA &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iY].b.iPowered != 0 &&
								!(unsigned __int16)Game_RandomWordLFSRMod4()) {
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_MARINA] / 9 > wSailingBoats) {
									Game_SpawnSailBoat(iX, iY);
								}
							}
							else if ((__int16)iAttributes >= TILE_ARCOLOGY_PLYMOUTH &&
								(__int16)iAttributes <= TILE_ARCOLOGY_LAUNCH &&
								(*(BYTE *)&dwMapXZON[iX][iY].b & 0xF0) == 0x80) {
								__int16 iTempTileID = (__int16)iAttributes;
								P_LOBYTE(iAttributes) = dwMapXTXT[iX][iY].bTextOverlay;
								iAttributes &= 0xFFFF00FF;
								if ((__int16)iAttributes >= 51 &&
									(__int16)iAttributes < 201 &&
									pMicrosimArr[(__int16)iAttributes - 51].bTileID >= 251 &&
									pMicrosimArr[(__int16)iAttributes - 51].bTileID != 255) {
									P_LOWORD(iAttributes) = iAttributes - 51;
									iMapValPerhaps = (*(BYTE *)&dwMapXVAL[iX >> 1][iY >> 1].bBlock >> 5)
										- (*(BYTE *)&dwMapXCRM[iX >> 1][iY >> 1].bBlock >> 5)
										- (*(BYTE *)&dwMapXPLT[iX >> 1][iY >> 1].bBlock >> 5)
										+ 12;
									if (iX >= GAME_MAP_SIZE ||
										iY >= GAME_MAP_SIZE ||
										dwMapXBIT[iX][iY].b.iPowered == 0)
										iMapValPerhaps /= 2;
									if (iX >= GAME_MAP_SIZE ||
										iY >= GAME_MAP_SIZE ||
										dwMapXBIT[iX][iY].b.iWatered == 0)
										iMapValPerhaps /= 2;
									if (iMapValPerhaps < 0)
										iMapValPerhaps = 0;
									if (iMapValPerhaps > 12)
										P_LOBYTE(iMapValPerhaps) = 12;
									//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - iMapValPerhaps(%d), (BYTE)iMapValPerhaps(%u), Item(%s)\n", iStep, iSubStep, iMapValPerhaps, (BYTE)iMapValPerhaps, szTileNames[iTempTileID]);
									pMicrosimArr[(__int16)iAttributes].bMicrosimData[0] = (BYTE)iMapValPerhaps;
								}
							}
						}
					}
				}
				else if ((__int16)iAttributes >= TILE_SUSPENSION_BRIDGE_START_B && (__int16)iAttributes < TILE_ONRAMP_TL ||
					(WORD)iAttributes == TILE_REINFORCED_BRIDGE_PYLON ||
					(WORD)iAttributes == TILE_REINFORCED_BRIDGE) {
					iFundingPercent = pBudgetArr[BUDGET_BRIDGE].iFundingPercent;
					// Transportation budget, bridges - if below 100% and the weather isn't favourable, there's a chance of destruction.
					if (iFundingPercent != 100 && (int)((unsigned __int8)bWeatherWind + (unsigned __int16)rand() % 50u) >= iFundingPercent) {
						//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Bridge. Weather Vulnerable\n", iStep, iSubStep);
						Game_CenterOnTileCoords(iX, iY);
						Game_DestroyStructure(pThis, iX, iY, 1);
						Game_NewspaperStoryGenerator(39, 0);
						goto GOAFTERSETXBIT;
					}
				}
				else if ((__int16)iAttributes < TILE_TUNNEL_T || (__int16)iAttributes >= TILE_CROSSOVER_POWERTB_ROADLR) {
					if (((__int16)iAttributes < TILE_HIGHWAY_HTB || (__int16)iAttributes >= TILE_SUBTORAIL_T) &&
						((__int16)iAttributes < TILE_HIGHWAY_LR || (__int16)iAttributes >= TILE_SUSPENSION_BRIDGE_START_B))
						goto GOAFTERSETXBIT;
					if ((iX & 1) == 0 && (iY & 1) == 0) {
						iFundingPercent = pBudgetArr[BUDGET_HIGHWAY].iFundingPercent;
						if (iFundingPercent != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iFundingPercent) {
							if (iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iY].b.iWater != 0) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #1. Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX, iY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #1 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iRandSelect, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX, iY, iReplaceTile);
							}
							iNextX = iX + 1;
							if ((iX + 1) >= 0 &&
								iNextX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iNextX][iY].b.iWater != 0) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #2. Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #2 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iRandSelect, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY, iReplaceTile);
							}
							iNextY = iY + 1;
							if (iX < GAME_MAP_SIZE &&
								(iY + 1) >= 0 &&
								iNextY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iNextY].b.iWater != 0) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #3. Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX, iY + 1, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #3 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iRandSelect, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX, iY + 1, iReplaceTile);
							}
							if ((iX + 1) < GAME_MAP_SIZE &&
								(iY + 1) < GAME_MAP_SIZE &&
								dwMapXBIT[(iX + 1)][(iY + 1)].b.iWater != 0) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #4. Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY + 1, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #4 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iRandSelect, szTileNames[(__int16)iAttributes]);
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY + 1, iReplaceTile);
							}
							goto GOAFTERSETXBIT;
						}
					}
				}
				else if ((__int16)iAttributes >= TILE_TUNNEL_T && (__int16)iAttributes <= TILE_TUNNEL_L) {
					iFundingPercent = pBudgetArr[BUDGET_TUNNEL].iFundingPercent;
					if (iFundingPercent != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iFundingPercent) {
						//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Tunnel. Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes]);
						Game_CenterOnTileCoords(iX, iY);
						Game_DestroyStructure(pThis, iX, iY, 1);
						goto GOAFTERSETXBIT;
					}
				}
			}
GOUNDCHECKTHENYINCREASE:
			if (!(unsigned __int16)Game_RandomWordLFSRMod128()) {
				P_LOBYTE(iAttributes) = dwMapXUND[iX][iY].iTileID;
				iAttributes &= 0xFFFF00FF;
				if ((__int16)iAttributes >= TILE_RUBBLE1 && (__int16)iAttributes < TILE_POWERLINES_HTB ||
					(WORD)iAttributes == TILE_ROAD_BR ||
					(WORD)iAttributes == TILE_ROAD_HTB ||
					(WORD)iAttributes == TILE_ROAD_LHR) {
					iFundingPercent = pBudgetArr[BUDGET_SUBWAY].iFundingPercent;
					if (iFundingPercent != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iFundingPercent) {
						//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Subway. Item(%s) / Underground Item(%s)\n", iStep, iSubStep, szTileNames[(__int16)iAttributes], ((__int16)iAttributes > 35) ? "** Unknown **" : szUndergroundNames[(__int16)iAttributes]);
						if ((WORD)iAttributes == TILE_ROAD_BR)
							Game_DestroyStructure(pThis, iX, iY, 0);
						else {
							if ((WORD)iAttributes == TILE_ROAD_HTB)
								iReplaceTile = UNDER_TILE_PIPES_TB;
							else if ((WORD)iAttributes == TILE_ROAD_LHR)
								iReplaceTile = UNDER_TILE_PIPES_LR;
							else
								iReplaceTile = UNDER_TILE_CLEAR;
							Game_PlaceUndergroundTiles(iX, iY, iReplaceTile);
						}
						// There's no 'goto' in this case.
					}
				}
			}
			iY += 4;
		}
		iX += 4;
		iResult = iX / 2;
		iXMM = iX / 2;
	}
	rcDst.top = -1000;
	return iResult;
#else
	int(__cdecl *H_SimulationGrowthTick)(signed __int16, signed __int16) = (int(__cdecl *)(signed __int16, signed __int16))0x4358B0;

	int ret = H_SimulationGrowthTick(iStep, iSubStep);
	//ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> SimulationGrowthTick(%d, %d) = %d\n", _ReturnAddress(), iStep, iSubStep, ret);

	return ret;
#endif
}

extern "C" int __cdecl Hook_SimulationGrowSpecificZone(__int16 iX, __int16 iY, __int16 iTileID, __int16 iZoneType) {
	// Variable names subject to change
	// during the demystification process.
	__int16 x, y;
	__int16 iCurrX, iCurrY;
	__int16 iNextX, iNextY;
	__int16 iBuildingCount[2];
	__int16 iMoveX, iMoveY;
	__int16 iRotate;
	__int16 iTileRotated;
	int i;
	__int16 iLengthWays;
	__int16 iDepthWays;
	__int16 iPierPathTileCount;
	__int16 iPierLength;
	map_XBIT_t *mXBIT;
	BYTE mXBBits;
	map_XBLD_t *mXBLDOne, *mXBLDTwo;
	BYTE mXBuilding[4];
	map_XZON_t *mXZONOne, *mXZONTwo;
	BYTE *pZone;

	x = iX;
	y = iY;
	if (iZoneType != ZONE_MILITARY) {
		if (!Game_IsZonedTilePowered(iX, iY))
			return 0;
	}
	switch (iTileID) {
		case TILE_INFRASTRUCTURE_RUNWAY:
			iMoveX = 0;
			iMoveY = 0;
			if ((dwTileCount[TILE_INFRASTRUCTURE_RUNWAY] & 1) == 0) {
				if ((x & 1) != 0) {
					iMoveX = 1;
					goto PROCEEDFURTHER;
				}
				if ((y & 1) == 0) {
					return 0;
				}
				goto PROCEEDAHEAD;
			}
			if ((y & 1) != 0) {
PROCEEDAHEAD:
				iMoveY = 1;
				goto PROCEEDFURTHER;
			}
			if ((x & 1) == 0) {
				return 0;
			}
			iMoveX = 1;
PROCEEDFURTHER:
			iCurrX = x;
			iCurrY = y;
			iBuildingCount[0] = 0;
			while (iCurrX < GAME_MAP_SIZE && iCurrY < GAME_MAP_SIZE) {
				if (dwMapXZON[iCurrX][iCurrY].b.iZoneType != iZoneType)
					return 0;
				mXBuilding[0] = dwMapXBLD[iCurrX][iCurrY].iTileID;
				if (iZoneType == ZONE_MILITARY) {
					if ((mXBuilding[0] >= TILE_ROAD_LR && mXBuilding[0] <= TILE_ROAD_LTBR) ||
						mXBuilding[0] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[0] == TILE_MILITARY_MISSILESILO)
						return 0;
					if (dwMapXTER[iCurrX][iCurrY].iTileID)
						return 0;
					if (dwMapXUND[iCurrX][iCurrY].iTileID)
						return 0;
				}
				if (mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAYCROSS)
					--iBuildingCount[0];
				iCurrX += iMoveY;
				++iBuildingCount[0];
				iCurrY += iMoveX;
				if (iBuildingCount[0] >= 5) {
					if (!iMoveY) 
						goto SKIPFIRSTROTATIONCHECK;
					if ((wViewRotation & 1) != 0) {
						if (!iMoveY) {
SKIPFIRSTROTATIONCHECK:
							if ((wViewRotation & 1) != 0)
								goto SKIPSECONDROTATIONCHECK;
						}
						iRotate = 0;
					}
					else {
SKIPSECONDROTATIONCHECK:
						iRotate = 1;
					}
					iBuildingCount[1] = 0;
					while (2) {
						mXBuilding[1] = dwMapXBLD[x][y].iTileID;
						if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAYCROSS) {
							--iBuildingCount[1];
							if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY) {
								iTileRotated = x < GAME_MAP_SIZE &&
									y < GAME_MAP_SIZE &&
									dwMapXBIT[x][y].b.iRotated;
								if (iTileRotated != iRotate) {
									Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAYCROSS);
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										*(BYTE *)&dwMapXZON[x][y].b |= 0xF0u;
									if (iZoneType != ZONE_MILITARY) {
										if (x <= -1)
											goto RUNWAY_GETOUT;
										if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
											*(BYTE *)&dwMapXBIT[x][y].b |= 0xC0u;
									}
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
										mXBIT = dwMapXBIT[x];
										mXBBits = (*(BYTE *)&mXBIT[y].b & 0xFD);
RUNWAY_GOBACK:
										*(BYTE *)&mXBIT[y].b = mXBBits;
									}
								}
							}
						}
						else {
							if (iZoneType == ZONE_MILITARY) {
								if ((mXBuilding[1] >= TILE_ROAD_LR && mXBuilding[1] <= TILE_ROAD_LTBR) ||
									mXBuilding[1] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[1] == TILE_MILITARY_MISSILESILO)
									return 0;
								if (dwMapXTER[x][y].iTileID)
									return 0;
								if (dwMapXUND[x][y].iTileID)
									return 0;
							}
							if (dwMapXBLD[x][y].iTileID >= TILE_SMALLPARK)
								Game_ZonedBuildingTileDeletion(x, y);
							Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAY);
							if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								*(BYTE *)&dwMapXZON[x][y].b |= 0xF0u;
							if (iZoneType != ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								*(BYTE *)&dwMapXBIT[x][y].b |= 0xC0u;
							if (iRotate && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
								mXBIT = dwMapXBIT[x];
								mXBBits = (*(BYTE *)&mXBIT[y].b | 2);
								goto RUNWAY_GOBACK;
							}
						}
RUNWAY_GETOUT:
						x += iMoveY;
						y += iMoveX;
						if (++iBuildingCount[1] >= 5)
							return 1;
						continue;
					}
				}
			}
			return 1;
		case TILE_INFRASTRUCTURE_CRANE:
			for (i = 0; i < 4; i++) {
				iLengthWays = x + wSomePierLengthWays[i];
				if (iLengthWays < GAME_MAP_SIZE) {
					iDepthWays = y + wSomePierDepthWays[i];
					if (iDepthWays < GAME_MAP_SIZE && dwMapXBIT[iLengthWays][iDepthWays].b.iWater != 0)
						break;
				}
			}
			if (i == 4)
				return 0;
			iDepthWays = wSomePierDepthWays[i];
			if (iDepthWays && (x & 1) != 0)
				return 0;
			iLengthWays = wSomePierLengthWays[i];
			if (iLengthWays && (y & 1) != 0)
				return 0;
			iPierPathTileCount = 0;
			iNextX = x;
			iNextY = y;
			do {
				iNextX += iLengthWays;
				iNextY += iDepthWays;
				if (iNextX >= GAME_MAP_SIZE || iNextY >= GAME_MAP_SIZE)
					return 0;
				if (iNextX >= GAME_MAP_SIZE ||
					iNextY >= GAME_MAP_SIZE ||
					dwMapXBIT[iNextX][iNextY].b.iWater == 0)
					return 0;
				if (dwMapXBLD[iNextX][iNextY].iTileID)
					return 0;
				++iPierPathTileCount;
			} while (iPierPathTileCount < 5);
			if ((*(WORD *)&dwMapALTM[iNextX][iNextY].w & 0x3E0) >> 5 < (*(WORD *)&dwMapALTM[iNextX][iNextY].w & 0x1F) + 2)
				return 0;
			if (dwMapXBLD[x][y].iTileID >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(x, y);
			Game_ItemPlacementCheck(x, y, TILE_INFRASTRUCTURE_CRANE, 1);
			if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
				pZone = (BYTE *)&dwMapXZON[x][y].b;
				*pZone ^= (*pZone ^ iZoneType) & 0xF;
			}
			if (iZoneType == ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				*(BYTE *)&dwMapXBIT[x][y].b &= 0xFu;
			iLengthWays = wSomePierLengthWays[i];
			if (!iLengthWays)
				goto PIER_GOTOONE;
			if ((wViewRotation & 1) == 0)
				goto PIER_GOTOTWO;
			if (iLengthWays)
				goto PIER_GOTOTHREE;
PIER_GOTOONE:
			if ((wViewRotation & 1) != 0) {
PIER_GOTOTWO:
				iRotate = 1;
			}
			else {
PIER_GOTOTHREE:
				iRotate = 0;
			}
			iPierLength = 4;
			do {
				x += wSomePierLengthWays[i];
				y += wSomePierDepthWays[i];
				Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_PIER);
				if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXZON[x][y].b |= 0xF0u;
				if (iRotate) {
					if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
						*(BYTE *)&dwMapXBIT[x][y].b |= 2u;
				}
				--iPierLength;
			} while (iPierLength);
			return 1;
		case TILE_INFRASTRUCTURE_CONTROLTOWER_CIV:
		case TILE_MILITARY_CONTROLTOWER:
		case TILE_MILITARY_WAREHOUSE:
		case TILE_INFRASTRUCTURE_BUILDING1:
		case TILE_INFRASTRUCTURE_BUILDING2:
		case TILE_MILITARY_TARMAC:
		case TILE_MILITARY_F15B:
		case TILE_MILITARY_HANGAR1:
		case TILE_MILITARY_RADAR:
			if (dwMapXBLD[x][y].iTileID < TILE_SMALLPARK) {
				Game_ItemPlacementCheck(x, y, iTileID, 1);
				if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXZON[x][y].b ^= (*(BYTE *)&dwMapXZON[x][y].b ^ iZoneType) & 0xF;
				if (iZoneType == ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXBIT[x][y].b &= 0xFu;
			}
			return 1;
		case TILE_INFRASTRUCTURE_PARKINGLOT:
		case TILE_MILITARY_PARKINGLOT:
		case TILE_MILITARY_LOADINGBAY:
		case TILE_MILITARY_TOPSECRET:
		case TILE_INFRASTRUCTURE_CARGOYARD:
		case TILE_INFRASTRUCTURE_HANGAR2:
			signed __int16 iSX;
			iSX = x & 0xFFFE; // If absent you will get bizarre overlap cases (this could be a part of the 0x402603 function investigation).
			P_LOBYTE(y) = y & 0xFE; // If absent you will get bizarre overlap cases (this could be a part of the 0x402603 function investigation).
			iNextX = (__int16)(iSX + 1);
			iNextY = (__int16)(y + 1);
			mXBLDOne = dwMapXBLD[iSX];
			mXBuilding[0] = mXBLDOne[y].iTileID;
			if (mXBuilding[0] >= TILE_INFRASTRUCTURE_WATERTOWER)
				return 0;
			if (mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[0] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[0] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBLDTwo = dwMapXBLD[iNextX];
			mXBuilding[1] = mXBLDTwo[y].iTileID;
			if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[1] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[1] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBuilding[2] = mXBLDOne[iNextY].iTileID;
			if (mXBuilding[2] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[2] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[2] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[2] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBuilding[3] = mXBLDTwo[iNextY].iTileID;
			if (mXBuilding[3] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[3] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[3] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[3] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXZONOne = dwMapXZON[iSX];
			if (mXZONOne[y].b.iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONOne[y].b.iZoneType == ZONE_MILITARY) {
					if (mXBuilding[0] >= TILE_ROAD_LR && mXBuilding[0] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iSX][y].iTileID)
					return 0;
			}
			mXZONTwo = dwMapXZON[iNextX];
			if (mXZONTwo[y].b.iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONTwo[y].b.iZoneType == ZONE_MILITARY) {
					if (mXBuilding[1] >= TILE_ROAD_LR && mXBuilding[1] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iNextX][y].iTileID)
					return 0;
			}
			if (mXZONOne[iNextY].b.iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONOne[iNextY].b.iZoneType == ZONE_MILITARY) {
					if (mXBuilding[2] >= TILE_ROAD_LR && mXBuilding[2] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iSX][iNextY].iTileID)
					return 0;
			}
			if (mXZONTwo[iNextY].b.iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONTwo[iNextY].b.iZoneType == ZONE_MILITARY) {
					if (mXBuilding[3] >= TILE_ROAD_LR && mXBuilding[3] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iNextX][iNextY].iTileID)
					return 0;
			}
			if (mXBuilding[0] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iSX, y);
			if (dwMapXBLD[iNextX][y].iTileID >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iNextX, y);
			if (dwMapXBLD[iSX][iNextY].iTileID >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iSX, iNextY);
			if (dwMapXBLD[iNextX][iNextY].iTileID >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iNextX, iNextY);
			Game_ItemPlacementCheck(iSX, y, iTileID, 2);
			if (iSX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				*(BYTE *)&dwMapXZON[iSX][y].b ^= (*(BYTE *)&dwMapXZON[iSX][y].b ^ iZoneType) & 0xF;
			if (iNextX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				*(BYTE *)&dwMapXZON[iNextX][y].b ^= (*(BYTE *)&dwMapXZON[iNextX][y].b ^ iZoneType) & 0xF;
			if (iSX < GAME_MAP_SIZE && iNextY < GAME_MAP_SIZE)
				*(BYTE *)&dwMapXZON[iSX][iNextY].b ^= (*(BYTE *)&dwMapXZON[iSX][iNextY].b ^ iZoneType) & 0xF;
			if (iNextX < GAME_MAP_SIZE && iNextY < GAME_MAP_SIZE)
				*(BYTE *)&dwMapXZON[iNextX][iNextY].b ^= (*(BYTE *)&dwMapXZON[iNextX][iNextY].b ^ iZoneType) & 0xF;
			if (iZoneType == ZONE_MILITARY) {
				if (iSX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXBIT[iSX][y].b &= 0xFu;
				if (iNextX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXBIT[iNextX][y].b &= 0xFu;
				if (iSX < GAME_MAP_SIZE && iNextY < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXBIT[iSX][iNextY].b &= 0xFu;
				if (iNextX < GAME_MAP_SIZE && iNextY < GAME_MAP_SIZE)
					*(BYTE *)&dwMapXBIT[iNextX][iNextY].b &= 0xFu;
			}
			return 1;
		case TILE_MILITARY_MISSILESILO:
			PlaceMissileSilo(x, y);
			return 1;
		default:
			return 1;
	}
}

extern "C" int __cdecl Hook_ItemPlacementCheck(unsigned __int16 m_x, int m_y, __int16 iTileID, __int16 iTileArea) {
	__int16 x;
	__int16 y;
	__int16 iArea;
	__int16 iMarinaCount;
	__int16 iX;
	__int16 iY;
	__int16 iTile;
	BYTE iBuilding;
	__int16 iItemWidth;
	__int16 iItemLength;
	__int16 iItemDepth;
	__int16 iMapBit;
	__int16 iCorner[3];
	char cMSimBit;

	x = (__int16)m_x;
	y = P_LOWORD(m_y);

	iArea = iTileArea - 1;
	if (iArea > 1) {
		--x;
		--y;
	}
	iMarinaCount = 0;
	iX = x;
	iItemWidth = x + iArea;
	if (iItemWidth >= x) {
		iTile = iTileID;
		iItemLength = iArea + y;
		while (1) {
			iY = y;
			if (iItemLength >= y)
				break;
		GOBACK:
			if (++iX > iItemWidth)
				goto GOFORWARD;
		}
		while (1) {
			if (iArea <= 0) {
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					return 0;
			}
			else if (iX < 1 || iY < 1 || iX > GAME_MAP_SIZE-2 || iY > GAME_MAP_SIZE-2) {
				// Added this due to legacy military plot drops, this allows > 1x1 type buildings
				// to develop if the plot is on the edge of the map.
				if (dwMapXZON[iX][iY].b.iZoneType == ZONE_MILITARY) {
					if (iX < 0 || iY < 0 || iX > GAME_MAP_SIZE - 1 || iY > GAME_MAP_SIZE - 1) {
						return 0;
					}
				}
				else {
					return 0;
				}
			}
			iBuilding = dwMapXBLD[iX][iY].iTileID;
			if (iBuilding >= TILE_ROAD_LR) {
				return 0;
			}
			if (iBuilding == TILE_RADIOACTIVITY) {
				return 0;
			}
			if (iBuilding == TILE_SMALLPARK) {
				return 0;
			}
			if (dwMapXZON[iX][iY].b.iZoneType == ZONE_MILITARY) {
				if (iBuilding == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
					iBuilding == TILE_ROAD_LR ||
					iBuilding == TILE_ROAD_TB)
					return 0;
			}
			if (iTileID == TILE_INFRASTRUCTURE_MARINA) {
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					dwMapXBIT[iX][iY].b.iWater != 0) {
					++iMarinaCount;
					goto GOSKIP;
				}
				if (dwMapXTER[iX][iY].iTileID) {
					return 0;
				}
			}
			if (dwMapXTER[iX][iY].iTileID) {
				return 0;
			}
			if (iX < GAME_MAP_SIZE &&
				iY < GAME_MAP_SIZE &&
				dwMapXBIT[iX][iY].b.iWater != 0) {
				return 0;
			}
		GOSKIP:
			if (++iY > iItemLength) {
				goto GOBACK;
			}
		}
	}
	iTile = iTileID;
GOFORWARD:
	if (iTile == TILE_INFRASTRUCTURE_MARINA && (!iMarinaCount || iMarinaCount == 9)) {
		Game_AfxMessageBox(107, 0, -1);
		return 0;
	}
	else {
		if (iTile == TILE_SERVICES_BIGPARK || (iMapBit = -32, iTile == TILE_SMALLPARK)) { // The initial setting of iMapBit to -32 isn't present in the DOS version.
			iMapBit = 32;
		}
		else {
			iMapBit = 224; // Present in the DOS version.
		}
		if (iTile == TILE_SMALLPARK && dwMapXBLD[x][y].iTileID > TILE_SMALLPARK) {
			return 0;
		}
		else {
			__int16 iCurrXPos = x;
			cMSimBit = Game_SimulationProvisionMicrosim(x, y, iTile); // The 'y' variable is '__int16' whereas that argument is an 'int' (it was previously the latter), noting just in case.
			if (iItemWidth >= x) {
				iItemDepth = y + iArea;
				do {
					for (__int16 iCurrYPos = y; iCurrYPos <= iItemDepth; ++iCurrYPos) {
						if (iCurrXPos > -1) {
							if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE) {
								*(BYTE *)&dwMapXBIT[iCurrXPos][iCurrYPos].b &= 0x1Fu;
							}
							if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE) {
								*(BYTE *)&dwMapXBIT[iCurrXPos][iCurrYPos].b |= iMapBit;
							}
						}
						Game_PlaceTileWithMilitaryCheck(iCurrXPos, iCurrYPos, iTile);
						if (iCurrXPos > -1) {
							if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE) {
								*(BYTE *)&dwMapXZON[iCurrXPos][iCurrYPos].b &= 0xF0u;
							}
							if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE) {
								*(BYTE *)&dwMapXZON[iCurrXPos][iCurrYPos].b &= 0xFu;
							}
						}
						if (cMSimBit) {
							*(BYTE *)&dwMapXTXT[iCurrXPos][iCurrYPos].bTextOverlay = cMSimBit;
						}
					}
					++iCurrXPos;
				} while (iCurrXPos <= iItemWidth);
			}
			if (iArea) {
				if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
					dwMapXZON[x][y].b.iCorners = wTileAreaBottomLeftCorner[4 * wViewRotation] >> 4;
				}
				iCorner[0] = iArea + x;
				if ((iArea + x) > -1 && iCorner[0] < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
					dwMapXZON[iCorner[0]][y].b.iCorners = wTileAreaBottomRightCorner[4 * wViewRotation] >> 4;
				}
				if (iCorner[0] < GAME_MAP_SIZE) {
					iCorner[1] = y + iArea;
					if ((y + iArea) > -1 && iCorner[1] < GAME_MAP_SIZE) {
						dwMapXZON[iCorner[0]][iCorner[1]].b.iCorners = wTileAreaTopLeftCorner[4 * wViewRotation] >> 4;
					}
				}
				if (x < GAME_MAP_SIZE) {
					iCorner[2] = iArea + y;
					if ((iArea + y) > -1 && iCorner[2] < GAME_MAP_SIZE) {
						dwMapXZON[x][iCorner[2]].b.iCorners = wTileAreaTopRightCorner[4 * wViewRotation] >> 4;
					}
				}
			}
			else if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE) {
				*(BYTE *)&dwMapXZON[x][y].b |= 0xF0u;
			}
			Game_SpawnItem(x, y + iArea);
			return 1;
		}
	}
}

#define NUM_CHEATS 15
#define NUM_CHEAT_MAXCHARS 9

typedef struct {
	int iIndex;          // Cheat index, match multiple cheats to the same index.
	const char *pEntry;  // Code entry
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
static BOOL cheatMultipleDetections = FALSE;

static void AdjustDebugMenu(HMENU hDebugMenu) {
	if (hDebugMenu) {
		AFX_MSGMAP_ENTRY afxMessageMapEntry[5];
		HMENU hDebugPopup;
		MENUITEMINFO miiDebugPopup;
		miiDebugPopup.cbSize = sizeof(MENUITEMINFO);
		miiDebugPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hDebugMenu, 0, TRUE, &miiDebugPopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		hDebugPopup = miiDebugPopup.hSubMenu;

		// Insert in reverse order.
		// Separator between the disasters and internal debugging functions.
		if (!InsertMenu(hDebugPopup, 11, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between grants and disasters
		if (!InsertMenu(hDebugPopup, 4, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		// Separator between the version option and grants
		if (!InsertMenu(hDebugPopup, 1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		// Insert in reverse order.
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_MISSILESILOS, "Propose Missile Silos") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_NAVALYARD, "Propose Naval Yard") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_ARMYBASE, "Propose Army Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #6 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_AIRFORCE, "Propose Air Force Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #7 failed, error = 0x%08X.\n", GetLastError());
			return;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_DECLINED, "Stop Military Spawning") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #8 failed, error = 0x%08X.\n", GetLastError());
			return;
		}

		afxMessageMapEntry[0] = {
			WM_COMMAND,
			0,
			IDM_DEBUG_MILITARY_DECLINED,
			IDM_DEBUG_MILITARY_DECLINED,
			0x0A,
			ProposeMilitaryBaseDecline,
		};

		afxMessageMapEntry[1] = {
			WM_COMMAND,
			0,
			IDM_DEBUG_MILITARY_AIRFORCE,
			IDM_DEBUG_MILITARY_AIRFORCE,
			0x0A,
			ProposeMilitaryBaseAirForceBase,
		};

		afxMessageMapEntry[2] = {
			WM_COMMAND,
			0,
			IDM_DEBUG_MILITARY_ARMYBASE,
			IDM_DEBUG_MILITARY_ARMYBASE,
			0x0A,
			ProposeMilitaryBaseArmyBase,
		};

		afxMessageMapEntry[3] = {
			WM_COMMAND,
			0,
			IDM_DEBUG_MILITARY_NAVALYARD,
			IDM_DEBUG_MILITARY_NAVALYARD,
			0x0A,
			ProposeMilitaryBaseNavalYard,
		};

		afxMessageMapEntry[4] = {
			WM_COMMAND,
			0,
			IDM_DEBUG_MILITARY_MISSILESILOS,
			IDM_DEBUG_MILITARY_MISSILESILOS,
			0x0A,
			ProposeMilitaryBaseMissileSilos,
		};

		VirtualProtect((LPVOID)0x4D4608, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy_s((LPVOID)0x4D4608, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));

		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated debug menu.\n");
	}
}

static int FindTheHouseLabel() {
	for (int i = 1; i < MAX_USER_LABELS; ++i) {
		if (dwMapXLAB[0][i].szLabel && _stricmp(dwMapXLAB[0][i].szLabel, theHouse)==0) {
			return i;
		}
	}
	return -1;
}

static void SetTheHouseLabel(int xPos, int ySignPos) {
	__int16 iLabelIdx;
	WORD iTextLen;

	char(__stdcall *H_PrepareLabel)() = (char(__stdcall *)())0x402D56;

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

	void(__cdecl *H_RemoveLabel)(__int16) = (void(__cdecl *)(__int16))0x401DCA;

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

	map_XTER_t **dwMapXTERPrevX = (map_XTER_t **)0x4C9F54;

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

extern "C" void __stdcall Hook_MainFrameOnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	DWORD pThis;

	__asm mov [pThis], ecx

	char nLowerChar;
	int i, j;
	int nCurrPos;
	int *nCodeArr;
	int nCodePos;
	char nCodeChar;
	cheat_t *strCheatEntry;
	HWND hWnd;
	DWORD *pSCView;
	HMENU hMenu, hDebugMenu;
	DWORD *pMenu, *pDebugMenu;
	int iSCMenuPos;
	DWORD jokeDlg[27];

	void(__cdecl *H_DoFund)(__int16) = (void(__cdecl *)(__int16))0x40191F;
	void(__thiscall *H_SimcityViewDebugGrantAllGifts)(DWORD *) = (void(__thiscall *)(DWORD *))0x401C0D;
	int(__thiscall *H_ADialogDestruct)(void *) = (int(__thiscall *)(void *))0x401D7A;
	void(__thiscall *H_SimcityAppAdjustNewspaperMenu)(void *) = (void(__thiscall *)(void *))0x40210D;
	DWORD *(__thiscall *H_JokeDialogConstruct)(void *, void *) = (DWORD *(__thiscall *)(void *, void *))0x4024E6;
	int(__stdcall *H_GetSimcityViewMenuPos)(int iPos) = (int(__stdcall *)(int))0x402EFA;
	void(__stdcall *H_SimulationProposeMilitaryBase)() = (void(__stdcall *)())0x403017;
	INT_PTR(__thiscall *H_DialogDoModal)(void *) = (INT_PTR(__thiscall *)(void *))0x4A7196;
	DWORD *(__stdcall *H_CMenuFromHandle)(HMENU) = (DWORD *(__stdcall *)(HMENU))0x4A7427;
	int(__thiscall *H_CMenuAttach)(DWORD *, HMENU) = (int(__thiscall *)(DWORD *, HMENU))0x4A7483;

	HINSTANCE &game_hModule = *(HINSTANCE *)0x4CE8C8;
	int &iCheatEntry = *(int *)0x4E6520;
	int &iCheatExpectedCharPos = *(int *)0x4E6524;
	char *szNewItem = (char *)0x4E66EC;

	hWnd = (HWND)((DWORD *)pThis)[7];

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
				pDebugMenu = (DWORD *)operator new(8); // This would be CMenu().
				if (pDebugMenu)
					pDebugMenu[1] = 0;
				hDebugMenu = LoadMenuA(game_hModule, (LPCSTR)223);
				AdjustDebugMenu(hDebugMenu);
				H_CMenuAttach(pDebugMenu, hDebugMenu);
				iSCMenuPos = H_GetSimcityViewMenuPos(6);
				InsertMenuA((HMENU)pMenu[1], iSCMenuPos + 6, MF_BYPOSITION|MF_POPUP, pDebugMenu[1], szNewItem);
				H_SimcityAppAdjustNewspaperMenu(&pCSimcityAppThis);
				DrawMenuBar(hWnd);
				bPriscillaActivated = 1;
				break;
			case CHEAT_MILITARY:
				H_SimulationProposeMilitaryBase();
				break;
			case CHEAT_JOKE:
				H_JokeDialogConstruct((void *)&jokeDlg, 0);
				H_DialogDoModal((void *)&jokeDlg);
				H_ADialogDestruct((void *)&jokeDlg); // Function name references "A" dialog rather than anything specific.
				break;
			case CHEAT_WEBB:
				if (!FindTheHouse()) {
					if (!BuildTheHouse())
						MessageBoxA(hWnd, "Sorry, no room to build Ilona's house!", gamePrimaryKey, MB_ICONINFORMATION | MB_OK);
				}
				break;
			case CHEAT_OOPS:
				MessageBoxA(hWnd, "Same to you, buddy!", "Hey!", MB_ICONEXCLAMATION | MB_OK);
				if (iChurchVirus < 0)
					iChurchVirus = 0; // Warning
				else if (iChurchVirus == 0)
					iChurchVirus = 1; // You asked for it!
				break;
			case CHEAT_REPENT:
				if (iChurchVirus > 0) {
					if (MessageBoxA(hWnd, "Tea Father?", gamePrimaryKey, MB_ICONINFORMATION | MB_YESNO) == IDYES)
						iChurchVirus = 0; // Set it back to 0 rather than -1; the next execution of the related cheats will result in immediate action.
					else
						goto NO;
				}
				else {
					if (iChurchVirus == 0)
						iChurchVirus = -1; // Set back to -1 if executed once more.
NO:
					MessageBoxA(hWnd, "Oh go on..", gamePrimaryKey, MB_ICONEXCLAMATION | MB_OK);
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

extern "C" void __stdcall Hook_SimcityDocUpdateDocumentTitle() {
	DWORD pThis;

	__asm mov [pThis], ecx

	CMFC3XString cStr;
	int iCityDayMon;
	int iCityMonth;
	int iCityYear;
	const char *pCurrStr;
	CSimString *pFundStr;

	CSimString *(__thiscall *H_SimStringSetString)(CSimString *, const char *pSrc, int iSize, double idAmount) = (CSimString *(__thiscall *)(CSimString *, const char *pSrc, int iSize, double idAmount))0x4015CD;
	void(__thiscall *H_SimStringTruncateAtSpace)(CSimString *) = (void(__thiscall *)(CSimString *))0x4019B5;
	void(__thiscall *H_SimStringDest)(CSimString *) = (void(__thiscall *)(CSimString *))0x40242D;
	void(__cdecl *H_CStringFormat)(CMFC3XString *, char const *Ptr, ...) = (void(__cdecl *)(CMFC3XString *, char const *Ptr, ...))0x49EBD3;
	CMFC3XString *(__thiscall *H_CStringCons)(CMFC3XString *) = (CMFC3XString *(__thiscall *)(CMFC3XString *))0x4A2C28;
	void(__thiscall *H_CStringEmpty)(CMFC3XString *) = (void(__thiscall *)(CMFC3XString *))0x4A2C95;
	void(__thiscall *H_CStringDest)(CMFC3XString *) = (void(__thiscall *)(CMFC3XString *))0x4A2CB0;
	BOOL(__thiscall *H_CStringLoadStringA)(CMFC3XString *, unsigned int) = (BOOL(__thiscall *)(CMFC3XString *, unsigned int))0x4A3453;
	BOOL(__stdcall *H_IsIconic)(HWND hWnd) = (BOOL(__stdcall *)(HWND hWnd))0x49BCF4;

	DWORD &MainFrmDest = *(DWORD *)0x4C7110;
	CMFC3XString &SCAStringLang = *(CMFC3XString *)0x4C7148;
	CMFC3XString *SCApCStringArrLongMonths = (CMFC3XString *)0x4C71F8;
	CMFC3XString *SCApCStringArrShortMonths = (CMFC3XString *)0x4C7288;
	const char *gameCurrDollar = (const char *)0x4E6168;
	const char *gameCurrDM = (const char *)0x4E6180;
	const char *gameLangGerman = (const char *)0x4E6198;
	const char *gameCurrFF = (const char *)0x4E619C;
	const char *gameLangFrench = (const char *)0x4E61B4;
	const char *gameStrHyphen = (const char *)0x4E6804;

	H_CStringCons(&cStr);

	if (!MainFrmDest) {
		if (!wCityMode) {
			H_CStringLoadStringA(&cStr, 0x19D); // "Starting SimEngine..."
			goto GETOUT;
		}
		if (!pszCityName.m_nDataLength)
			goto GETOUT;
		iCityDayMon = dwCityDays % 25 + 1;
		iCityMonth = dwCityDays / 25 % 12;
		iCityYear = wCityStartYear + dwCityDays / 300;
		if (H_IsIconic(GameGetRootWindowHandle())) {
			if (dwDisasterActive) {
				if (wCurrentDisasterID <= DISASTER_HURRICANE)
					H_CStringLoadStringA(&cStr, dwDisasterStringIndex[wCurrentDisasterID]);
				else
					H_CStringEmpty(&cStr);
			}
			else
				H_CStringFormat(&cStr, "%s%s%d", pszCityName.m_pchData, gameStrHyphen, iCityYear);
			goto GOFORWARD;
		}
		H_CStringEmpty(&cStr);
		if (wcscmp((const wchar_t *)SCAStringLang.m_pchData, (const wchar_t *)gameLangFrench) != 0) {
			if (wcscmp((const wchar_t *)SCAStringLang.m_pchData, (const wchar_t *)gameLangGerman) != 0)
				pCurrStr = gameCurrDollar;
			else
				pCurrStr = gameCurrDM;
		}
		else
			pCurrStr = gameCurrFF;
		pFundStr = new CSimString();
		if (pFundStr)
			pFundStr = H_SimStringSetString(pFundStr, pCurrStr, 20, (double)dwCityFunds);
		else
			goto GETOUT;
		H_SimStringTruncateAtSpace(pFundStr);
		if (bSettingsTitleCalendar)
			H_CStringFormat(&cStr, "%s %d %4d <%s> %s", SCApCStringArrLongMonths[iCityMonth].m_pchData, iCityDayMon, iCityYear, pszCityName.m_pchData, pFundStr->pStr);
		else
			H_CStringFormat(&cStr, "%s %4d <%s> %s", SCApCStringArrShortMonths[iCityMonth].m_pchData, iCityYear, pszCityName.m_pchData, pFundStr->pStr);
		if (pFundStr) {
			H_SimStringDest(pFundStr);
			operator delete(pFundStr);
		}
GOFORWARD:
		Game_CDocument_UpdateAllViews((void *)pThis, 0, 1, &cStr);
	}
GETOUT:
	H_CStringDest(&cStr);
}

// Local TileHightlightUpdate function.
// This is for attempts at mitigating some of
// the oddities that come with either:
// 1) African Swallow mode during non-granular updates (batch).
// 2) Granular updates on all speed levels. (more so for African Swallow and Cheetah)
static void L_TileHighlightUpdate(DWORD *pThis) {
	BYTE *vBits;
	LONG bottom;
	LONG x;
	__int16 y;

	int(__cdecl *H_BeginObject)(void *, void *, int, __int16, RECT *) = (int(__cdecl *)(void *, void *, int, __int16, RECT *))0x401226;
	BOOL(__thiscall *H_SimcityViewMainWindowUpdate)(void *, RECT *, BOOL) = (BOOL(__thiscall *)(void *, RECT *, BOOL))0x40152D;
	void(__thiscall *H_GraphicsUnlockDIBBits)(void *) = (void(__thiscall *)(void *))0x401BE5;
	int(__thiscall *H_GraphicsHeight)(void *) = (int(__thiscall *)(void *))0x40216C;
	LONG(__thiscall *H_GraphicsWidth)(void *) = (LONG(__thiscall *)(void *))0x402419;
	int(__thiscall *H_SimcityViewCheckOrLoadGraphic)(void *) = (int(__thiscall *)(void *))0x40297D;
	BOOL(__stdcall *H_FinishObject)() = (BOOL(__stdcall *)())0x402B7B;
	BYTE *(__thiscall *H_GraphicsLockDIBBits)(void *) = (BYTE *(__thiscall *)(void *))0x402DA1;

	DWORD *pSomeWnd = (DWORD *)0x4CAC18;
	tagRECT &dRect = *(tagRECT *)0x4CAD48;

	if (wTileHighlightActive) {
		vBits = H_GraphicsLockDIBBits((void *)pThis[13]);
		if (vBits || H_SimcityViewCheckOrLoadGraphic(pThis)) {
			x = H_GraphicsWidth((void *)pThis[13]);
			y = H_GraphicsHeight((void *)pThis[13]);
			if (!bOverrideTickPlacementHighlight) {
				H_BeginObject(pThis, vBits, x, y, (RECT *)pThis + 19);
				Game_DrawSquareHighlight(pThis, wHighlightedTileX1, wHighlightedTileY1, wHighlightedTileX2, wHighlightedTileY2);
				H_FinishObject();
			}
			H_GraphicsUnlockDIBBits((void *)pThis[13]);
			bottom = ++dRect.bottom;
			if (*(DWORD *)((char *)pThis + 322)) {
				dRect.bottom = bottom + 2;
				++dRect.right;
			}
			H_SimcityViewMainWindowUpdate(pThis, &dRect, 1);
			if (bOverrideTickPlacementHighlight)
				wTileHighlightActive = 0;
		}
	}
}

extern "C" void __stdcall Hook_SimulationProcessTick() {
	int i;
	DWORD dwMonDay;
	DWORD newsDialog[156];
	__int16 iStep, iSubStep;
	DWORD dwCityProgressionRequirement;
	BYTE iPaperVal;
	BOOL bScenarioSuccess;
	BOOL bDoTileHighlightUpdate;
	DWORD *pSCApp;
	DWORD *pSCView;

	void(__stdcall *H_UpdateGraphDialog)() = (void(__stdcall *)())0x4010A5;
	void(__stdcall *H_SimulationPollutionTerrainAndLandValueScan)() = (void(__stdcall *)())0x401154;
	void(__stdcall *H_SimulationEQ_LE_Processing)() = (void(__stdcall *)())0x401262;
	void(__cdecl *H_UpdateSimNationDialog)() = (void(__cdecl *)())0x4012FD;
	void(__stdcall *H_UpdateIndustryDialog)() = (void(__stdcall *)())0x40142E;
	void(__cdecl *H_SimulationPrepareBudgetDialog)(int) = (void(__cdecl *)(int))0x4015E6;
	void(__cdecl *H_SimulationGrantReward)(__int16 iReward, int iToggle) = (void(__cdecl *)(__int16 iReward, int iToggle))0x401672;
	void(__stdcall *H_UpdatePopulationDialog)() = (void(__stdcall *)())0x40169F;
	void(__thiscall *H_SimcityAppCallAutoSave)(void *) = (void(__thiscall *)(void *))0x4016A9;
	void(__thiscall *H_SimcityViewMaintainCursor)(void *) = (void(__thiscall *)(void *))0x401A96;
	void(__stdcall *H_SimulationUpdateWaterConsumption)() = (void(__stdcall *)())0x401CA8;
	void(__stdcall *H_UpdateWeatherOrDisasterState)() = (void(__stdcall *)())0x401E65;
	DWORD *(__thiscall *H_NewspaperConstruct)(void *) = (DWORD *(__thiscall *)(void *))0x401F23;
	void(__stdcall *H_UpdateGraphData)() = (void(__stdcall *)())0x402022;
	void(__thiscall *H_SimcityAppAdjustNewspaperMenu)(void *) = (void(__thiscall *)(void *))0x40210D;
	void(__stdcall *H_SimulationRCIDemandUpdates)() = (void(__stdcall *)())0x40217B;
	int(__thiscall *H_GameDialogDoModal)(void *) = (int(__thiscall *)(void *))0x40219E;
	void(__cdecl *H_SimulationGrowthTick)(__int16 iStep, __int16 iSubStep) = (void(__cdecl *)(__int16, __int16))0x4022FC;
	void(__cdecl *H_UpdateCityMap)() = (void(__cdecl *)())0x40239C;
	void(__stdcall *H_ToolMenuUpdate)() = (void(__stdcall *)())0x4023EC;
	void(__cdecl *H_EventScenarioNotification)(__int16 iEvent) = (void(__cdecl *)(__int16 iEvent))0x402487;
	void(__thiscall *H_NewspaperDestruct)(void *) = (void(__thiscall *)(void *))0x4025B3;
	void(__stdcall *H_SimulationUpdatePowerConsumption)() = (void(__stdcall *)())0x4026F8;
	void(__stdcall *H_NewspaperStoryGenerator)(__int16 iPaperType, BYTE iPaperVal) = (void(__stdcall *)(__int16 iPaperType, BYTE iPaperVal))0x402900;
	void(__stdcall *H_UpdateBudgetInformation)() = (void(__stdcall *)())0x402D2E;
	void(__stdcall *H_SimulationUpdateMonthlyTrafficData)() = (void(__stdcall *)())0x402D51;
	void(__thiscall *H_MainFrameUpdateCityToolBar)(void *) = (void(__thiscall *)(void *))0x402F18;
	void(__stdcall *H_SimulationProposeMilitaryBase)() = (void(__stdcall *)())0x403017;

	wCityCurrentMonth = ++dwCityDays / 25 % 12;
	wCityCurrentSeason = (dwCityDays / 25 % 12 + 1) % 12 / 3;
	wCityElapsedYears = dwCityDays / 300;
	if (dwSCAGameAutoSave > 0 &&
		!((dwCityDays / 300) % dwSCAGameAutoSave) &&
		!wCityCurrentMonth &&
		!(dwCityDays & 25)) {
		H_SimcityAppCallAutoSave(&pCSimcityAppThis);
	}

	if (bSettingsFrequentCityRefresh) {
		Game_RefreshTitleBar(pCDocumentMainWindow);
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
	}

	dwMonDay = (dwCityDays % 25);
	switch (dwMonDay) {
		case 0:
			if (!bSettingsFrequentCityRefresh)
				Game_RefreshTitleBar(pCDocumentMainWindow);
			if (bYearEndFlag)
				H_SimulationPrepareBudgetDialog(0);
			H_UpdateBudgetInformation();
			if (bNewspaperSubscription) {
				if (wCityCurrentMonth == 3 || wCityCurrentMonth == 7) {
					H_NewspaperConstruct((void *)&newsDialog);
					newsDialog[39] = wNewspaperChoice; // CNewspaperDialog -> CGameDialog -> CDialog; struct position 39 - paperchoice dword var.
					H_GameDialogDoModal(&newsDialog);
					H_NewspaperDestruct(&newsDialog);
				}
			}
			wCityCurrentMonth = dwCityDays / 25 % 12;
			wCityCurrentSeason = (dwCityDays / 25 % 12 + 1) % 12 / 3;
			wCityElapsedYears = dwCityDays / 300;
			for (i = 0; i < 8; pZonePops[i - 1] = 0)
				++i;
			break;
		case 1:
			H_SimulationUpdatePowerConsumption();
			break;
		case 2:
			H_SimulationPollutionTerrainAndLandValueScan();
			break;
		// Switch cases 3-18 have been moved to 'default' as
		// if (dwMonDay >= 3 && dwMonDay <= 18).
		case 19:
			H_SimulationUpdateMonthlyTrafficData();
			break;
		case 20:
			H_SimulationUpdateWaterConsumption();
			break;
		case 21:
			H_SimulationRCIDemandUpdates();
			H_SimulationEQ_LE_Processing();
			H_UpdateGraphData();
			break;
		case 22:
			dwCityProgressionRequirement = dwCityProgressionRequirements[wCityProgression];
			if (dwCityProgressionRequirement) {
				if (dwCityProgressionRequirement < dwCityPopulation) {
					Game_SimcityAppSetGameCursor(&pCSimcityAppThis, 24, 0);
					iPaperVal = wCityProgression++;
					H_NewspaperStoryGenerator(3, iPaperVal);
					H_SimcityAppAdjustNewspaperMenu(&pCSimcityAppThis);
					if (wCityProgression >= 4) {
						if (wCityProgression == 4)
							H_SimulationProposeMilitaryBase();
						else if (wCityProgression == 5)
							H_SimulationGrantReward(3, 1);
					}
					else
						H_SimulationGrantReward(wCityProgression - 1, 1);
					H_ToolMenuUpdate();
					H_SimcityAppAdjustNewspaperMenu(&pCSimcityAppThis);
					Game_SimcityAppSetGameCursor(&pCSimcityAppThis, 0, 0);
				}
			}
			if (bInScenario) {
				bScenarioSuccess = dwScenarioCitySize <= dwCityPopulation;
				if (pBudgetArr[BUDGET_RESFUND].iCurrentCosts < (int)dwScenarioResPopulation)
					bScenarioSuccess = FALSE;
				if (pBudgetArr[BUDGET_COMFUND].iCurrentCosts < (int)dwScenarioComPopulation)
					bScenarioSuccess = FALSE;
				if (pBudgetArr[BUDGET_INDFUND].iCurrentCosts < (int)dwScenarioIndPopulation)
					bScenarioSuccess = FALSE;
				if (dwCityFunds - dwCityBonds < (int)dwScenarioCashGoal)
					bScenarioSuccess = FALSE;
				if (dwCityLandValue < (int)dwScenarioLandValueGoal)
					bScenarioSuccess = FALSE;
				if (wScenarioLEGoal > dwCityWorkforceLE)
					bScenarioSuccess = FALSE;
				if (wScenarioEQGoal > dwCityWorkforceEQ)
					bScenarioSuccess = FALSE;
				if (dwScenarioPollutionLimit > 0 && dwCityPollution > dwScenarioPollutionLimit)
					bScenarioSuccess = FALSE;
				if (dwScenarioCrimeLimit > 0 && dwCityCrime > dwScenarioCrimeLimit)
					bScenarioSuccess = FALSE;
				if (dwScenarioTrafficLimit > 0 && dwCityTrafficUnknown > dwScenarioTrafficLimit)
					bScenarioSuccess = FALSE;
				if (bScenarioBuildingGoal1) {
					if (dwTileCount[bScenarioBuildingGoal1] < wScenarioBuildingGoal1Count)
						bScenarioSuccess = FALSE;
				}
				if (bScenarioBuildingGoal2) {
					if (dwTileCount[bScenarioBuildingGoal2] < wScenarioBuildingGoal2Count)
						bScenarioSuccess = FALSE;
				}
				if (bScenarioSuccess)
					H_EventScenarioNotification(1);
				else if (!--wScenarioTimeLimitMonths)
					H_EventScenarioNotification(0);
			}
			if (dwCityFunds < -100000)
				H_EventScenarioNotification(2);
			break;
		case 23:
			if (!bSettingsFrequentCityRefresh)
				Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
			H_UpdatePopulationDialog();
			H_UpdateIndustryDialog();
			H_UpdateGraphDialog();
			break;
		case 24:
			H_MainFrameUpdateCityToolBar(pCWndRootWindow);
			H_UpdateCityMap();
			H_UpdateSimNationDialog();
			H_UpdateWeatherOrDisasterState();
			break;
		default:
			// Moved here rather than the prior list of cases that were
			// specific to the growth tick function.
			if (dwMonDay >= 3 && dwMonDay <= 18) {
				if (dwMonDay == 12) {
					wCityCurrentMonth = dwCityDays / 25 % 12;
					wCityCurrentSeason = (dwCityDays / 25 % 12 + 1) % 12 / 3;
					wCityElapsedYears = dwCityDays / 300;
				}
				iStep = ((dwMonDay - 3) / 4 % 4); // Steps 0 - 3 in groups of 4.
				iSubStep = (dwMonDay + 1) % 4; // SubSteps 0-3 for each group of 4.
				H_SimulationGrowthTick(iStep, iSubStep);
				break;
			}
			return;
	}

	// Explanation:
	// !bSettingsFrequentCityRefresh - It will do the tile highlight update if:
	// 1) wSimulationSpeed is set to African Swallow
	// 2) pSCApp[198] is true (AnimationOffCycle) or it is game day 21 - CDocument::UpdateAllViews case.
	//
	// bSettingsFrequentCityRefresh - Tile highlight updates only occur if wSimulationSpeed
	// isn't set to paused.

	bDoTileHighlightUpdate = FALSE;
	pSCApp = &pCSimcityAppThis;
	if (!bSettingsFrequentCityRefresh) {
		if (wSimulationSpeed == GAME_SPEED_AFRICAN_SWALLOW) {
			if (pSCApp[198] || dwMonDay == 21)
				bDoTileHighlightUpdate = TRUE;
		}
	}
	else {
		if (wSimulationSpeed != GAME_SPEED_PAUSED) {
			bDoTileHighlightUpdate = TRUE;
		}
	}

	if (bDoTileHighlightUpdate) {
		pSCView = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
		if (pSCView) {
			if (wCityMode) {
				// It should be noted that the highlight will only appear with a valid selected tool.
				// If you attempt to press Shift or Control (for the bulldozer or query) while an
				// invalid tool is selected, there'll be no placement highlighted (this matches the
				// behaviour in the normal game as well).
				if (wCurrentCityToolGroup != TOOL_GROUP_CENTERINGTOOL) {
					if (wTileCoordinateX < 0 || wTileCoordinateX >= GAME_MAP_SIZE ||
						wTileCoordinateY < 0 || wTileCoordinateY >= GAME_MAP_SIZE) {
						wTileHighlightActive = 0;
					}
					else {
						wTileHighlightActive = 1;
						L_TileHighlightUpdate(pSCView);
					}
					H_SimcityViewMaintainCursor(pSCView);
				}
			}
		}
	}
}

extern "C" void __stdcall Hook_SimulationStartDisaster(void) {
	void(__stdcall *H_SimulationStartDisaster)() = (void(__stdcall *)())0x45CF10;

	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationStartDisaster(), wDisasterType = %u.\n", _ReturnAddress(), wSetTriggerDisasterType);

	H_SimulationStartDisaster();
}

extern "C" int __stdcall Hook_AddAllInventions(void) {
	if (mischook_debug & MISCHOOK_DEBUG_CHEAT)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> AddAllInventions()\n", _ReturnAddress());

	memset(wCityInventionYears, 0, sizeof(WORD)*MAX_CITY_INVENTION_YEARS);
	Game_ToolMenuUpdate();
	Game_SoundPlaySound(&pCSimcityAppThis, SOUND_ZAP);

	return 0;
}

// Hook the middle mouse button as a centering tool shortcut
extern "C" int __stdcall Hook_CSimcityView_WM_MBUTTONDOWN(WPARAM wMouseKeys, POINT pt) {
	__int16 wTileCoords = 0;
	BYTE bTileX = 0, bTileY = 0;
	wTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
	bTileX = LOBYTE(wTileCoords);
	bTileY = HIBYTE(wTileCoords);

	if (wTileCoords & 0x8000)
		return wTileCoords;
	else {
		if (wMouseKeys & MK_CONTROL)
			;
		else if (wMouseKeys & MK_SHIFT)
			;
		else if (GetAsyncKeyState(VK_MENU) < 0) {
			// useful for tests
		} else {
			Game_SoundPlaySound(&pCSimcityAppThis, SOUND_CLICK);
			Game_CenterOnTileCoords(bTileX, bTileY);
		}
	}
	return wTileCoords;
}

extern "C" __int16 __stdcall Hook_CSimcityView_WM_LBUTTONDOWN(WPARAM iMouseKeys, POINT pt) {
	DWORD pThis;

	__asm mov [pThis], ecx

	int ret;
	HWND hWnd;
	int iYVar;
	tagRECT Rect;

	ret = *(DWORD *)(pThis + 268);
	if (ret)
		*(DWORD *)(pThis + 268) = 0;
	else {
		ret = PtInRect((const RECT *)(pThis + 232), pt);
		if (!ret) {
			Game_GetScreenAreaInfo(pThis, &Rect);
			if (PtInRect((const RECT *)(pThis + 88), pt)) {
				iYVar = *(DWORD *)(pThis + 76);
				// CWnd::OnVScroll:
				// - Second argument:
				//   - 0 - Up Key or left-click the top arrow on the vertical scrollbar
				//   - 1 - Down Key or left-click the bottom arrow on the vertical scrollbar
				//   - 2 - Left-click on the vertical scrollbar above the 'thumb'
				//   - 3 - Left-click on the vertical scrollbar below the 'thumb'
				//   - 4 - Release the 'thumb' (new 'thumb' release position reflected on screen)
				//   - 5 - Left-click on the vertical scrollbar 'thumb' hold and drag (return result from function is 0 - 'default' case hit)
				//   - 6 - Directly to bottom (trigger currently unknown)
				//   - 7 - Directly to top (trigger currently unknown)
				//   - 8 - Release either arrow on the vertical scrollbar (return result from function is 0 - 'default' case hit)
				if (PtInRect((const RECT *)(pThis + 120), pt))
					P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_LINEDOWN, 0, iYVar);
				else if (PtInRect((const RECT *)(pThis + 104), pt))
					P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_LINEUP, 0, iYVar);
				else if (PtInRect((const RECT *)(pThis + 136), pt))
					P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_THUMBTRACK, (__int16)pt.y, iYVar);
				else {
					if (*(DWORD *)(pThis + 140) >= (ULONG)pt.y)
						P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_PAGEUP, 0, iYVar);
					else
						P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_PAGEDOWN, 0, iYVar);
				}
			}
			else {
				ret = *(DWORD *)(pThis + 252);
				if (!ret) {
					bOverrideTickPlacementHighlight = TRUE;
					hWnd = SetCapture(*(HWND *)(pThis + 28));
					Game_CWnd_FromHandle(hWnd);
					P_LOWORD(ret) = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
					wCurrentTileCoordinates = ret;
					if ((__int16)ret >= 0) {
						wTileCoordinateX = (uint8_t)ret;
						wPreviousTileCoordinateX = (uint8_t)ret;
						wTileCoordinateY = wCurrentTileCoordinates >> 8;
						wPreviousTileCoordinateY = wCurrentTileCoordinates >> 8;
						wGameScreenAreaX = (WORD)pt.x;
						wGameScreenAreaY = (WORD)pt.y;
						*(DWORD *)(pThis + 252) = 1;
						*(DWORD *)(pThis + 248) = 1;
						if (wCityMode)
							P_LOWORD(ret) = Game_CityToolMenuAction(iMouseKeys, pt);
						else
							P_LOWORD(ret) = Game_MapToolMenuAction(iMouseKeys, pt);
					}
				}
			}
		}
	}

	return ret;
}

extern "C" __int16 __stdcall Hook_CSimcityView_WM_MOUSEMOVE(WPARAM iMouseKeys, POINT pt) {
	DWORD pThis;

	__asm mov [pThis], ecx

	int iTileCoords;
	int iLeftMousDownInGameArea;

	P_LOWORD(iTileCoords) = (WORD)pt.x;
	iLeftMousDownInGameArea = *(DWORD *)(pThis + 252);
	*(struct tagPOINT *)(pThis + 260) = pt; // Placement position.
	if (iLeftMousDownInGameArea) {
		P_LOWORD(iTileCoords) = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		wCurrentTileCoordinates = iTileCoords;
		if ((__int16)iTileCoords >= 0) {
			wTileCoordinateX = (unsigned __int8)iTileCoords;
			P_LOWORD(iTileCoords) = wCurrentTileCoordinates >> 8;
			wTileCoordinateY = wCurrentTileCoordinates >> 8;
			if ( wPreviousTileCoordinateX != wTileCoordinateX || wPreviousTileCoordinateY != (WORD)iTileCoords) {
				if ( (int)abs(wGameScreenAreaX - pt.x) > 1 || (iTileCoords = abs(wGameScreenAreaY - pt.y), iTileCoords > 1) ) {
					*(DWORD *)(pThis + 256) = 1;
					if ((iMouseKeys & MK_LBUTTON) != 0) {
						if (*(DWORD *)(pThis + 248)) {
							if (wCityMode) {
								if ((wCurrentCityToolGroup != 17) || GetAsyncKeyState(VK_MENU) & 0x8000)
									Game_CityToolMenuAction(iMouseKeys, pt);
							}
							else {
								if ((wCurrentMapToolGroup == 9 && GetAsyncKeyState(VK_MENU) & 0x8000) || // 'Center Tool' selected with either 'Alt' key pressed.
									(wCurrentMapToolGroup != 9 && (iMouseKeys & MK_CONTROL) == 0) || // Other tool selected with 'ctrl' not pressed.
									(wCurrentMapToolGroup != 9 && (iMouseKeys & MK_CONTROL) != 0 && GetAsyncKeyState(VK_MENU) & 0x8000)) // Other tool with 'ctrl' pressed (Center Tool) and 'Alt'.
									Game_MapToolMenuAction(iMouseKeys, pt);
							}
						}
					}
					P_LOWORD(iTileCoords) = wTileCoordinateX;
					wPreviousTileCoordinateX = wTileCoordinateX;
					wPreviousTileCoordinateY = wTileCoordinateY;
					wGameScreenAreaX = (WORD)pt.x;
					wGameScreenAreaY = (WORD)pt.y;
				}
			}
		}
	}
	else
		bOverrideTickPlacementHighlight = FALSE;

	return iTileCoords;
}

extern "C" __int16 __cdecl Hook_MapToolMenuAction(int iMouseKeys, POINT pt) {
	DWORD *pThis;
	int ret;
	__int16 iCurrToolGroupA, iCurrToolGroupB;
	__int16 iTileStartX, iTileStartY;
	__int16 iTileTargetX, iTileTargetY;
	WORD wNewScreenPointX, wNewScreenPointY;
	HWND hWnd;

	// pThis[62] - When this is set to 0, you remain within the do/while loop until you
	//             release the left mouse button.
	//             If it is set to 1 while the left mouse button is pressed (Shift key is
	//             pressed and the iCurrToolGroupA is not 7 or 8 (trees or forest respectively)
	//             it will break out of the loop and then you end up within the WM_MOUSEMOVE
	//             call (if mouse movement is taking place).
	//
	// The change in this case is to only set pThis[62] to 0 when the iCurrToolGroupA is not
	// 'Center Tool', this will then allow it to pass-through to the WM_MOUSEMOVE call.

	pThis = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);	// TODO: is this necessary or can we just dereference pCSimcityView?
	Game_TileHighlightUpdate(pThis);
	iCurrToolGroupA = wCurrentMapToolGroup;
	iTileStartX = 400;
	iTileStartY = 400;
	iCurrToolGroupB = wCurrentMapToolGroup;
	if ((iMouseKeys & MK_CONTROL) != 0)
		iCurrToolGroupA = MAPTOOL_GROUP_CENTERINGTOOL;
	if (iCurrToolGroupA != MAPTOOL_GROUP_CENTERINGTOOL)
		pThis[62] = 0;
	do {
		P_LOWORD(ret) = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		if ((__int16)ret < 0)
			break;
		iTileTargetX = ret & (GAME_MAP_SIZE-1);
		iTileTargetY = (__int16)ret >> 8;
		if ((unsigned __int16)iTileTargetX >= GAME_MAP_SIZE || iTileTargetY < 0)
			break;
		if ((iMouseKeys & MK_SHIFT) != 0 && iCurrToolGroupA != MAPTOOL_GROUP_TREES && iCurrToolGroupA != MAPTOOL_GROUP_FOREST) {
			pThis[62] = 1;
			break;
		}
		if (iTileStartX != iTileTargetX || iTileStartY != iTileTargetY) {
			switch (iCurrToolGroupA) {
			case MAPTOOL_GROUP_BULLDOZER: // Bulldozing, only relevant in the CityToolMenuAction code it seems.
				Game_UseBulldozer(iTileTargetX, iTileTargetY);
				Game_UpdateAreaPortionFill(pThis);
				break;
			case MAPTOOL_GROUP_RAISETERRAIN: // Raise Terrain
				P_LOWORD(ret) = Game_MapToolRaiseTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_LOWERTERRAIN: // Lower Terrain
				P_LOWORD(ret) = Game_MapToolLowerTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_STRETCHTERRAIN: // Stretch Terrain (Drag vertically)
				P_LOWORD(ret) = Game_MapToolStretchTerrain(iTileTargetX, iTileTargetY, (__int16)pt.y);
				break;
			case MAPTOOL_GROUP_LEVELTERRAIN: // Level Terrain
				P_LOWORD(ret) = Game_MapToolLevelTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_WATER: // Place Water
			case MAPTOOL_GROUP_STREAM: // Place Stream
				if (iCurrToolGroupA == MAPTOOL_GROUP_WATER) {
					if (!Game_MapToolPlaceWater(iTileTargetX, iTileTargetY) || Game_MapToolSoundTrigger(dwAudioHandle))
						break;
				}
				else {
					Game_MapToolPlaceStream(iTileTargetX, iTileTargetY, 100);
					if (Game_MapToolSoundTrigger(dwAudioHandle))
						break;
				}
				Game_SoundPlaySound(&pCSimcityAppThis, SOUND_FLOOD);
				break;
			case MAPTOOL_GROUP_TREES: // Place Tree
			case MAPTOOL_GROUP_FOREST: // Place Forest
				if (!Game_MapToolSoundTrigger(dwAudioHandle))
					Game_SoundPlaySound(&pCSimcityAppThis, SOUND_PLOP);
				if (iCurrToolGroupA == 7)
					Game_MapToolPlaceTree(iTileTargetX, iTileTargetY);
				else
					Game_MapToolPlaceForest(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_CENTERINGTOOL: // Center Tool
				Game_GetScreenCoordsFromTileCoords(iTileTargetX, iTileTargetY, &wNewScreenPointX, &wNewScreenPointY);
				Game_SoundPlaySound(&pCSimcityAppThis, SOUND_CLICK);
				if (*(DWORD *)((char *)pThis + 322))
					Game_CenterOnNewScreenCoordinates(pThis, wScreenPointX - (wNewScreenPointX >> 1), wScreenPointY - (wNewScreenPointY >> 1));
				else
					Game_CenterOnNewScreenCoordinates(pThis, wScreenPointX - wNewScreenPointX, wScreenPointY - wNewScreenPointY);
				break;
			default:
				break;
			}
		}
		if (iCurrToolGroupA >= MAPTOOL_GROUP_RAISETERRAIN && iCurrToolGroupA <= MAPTOOL_GROUP_LEVELTERRAIN)
			break;
		else if (iCurrToolGroupA == MAPTOOL_GROUP_CENTERINGTOOL) {
			Game_UpdateAreaCompleteColorFill(pThis);
			hWnd = (HWND)pThis[7];
			UpdateWindow(hWnd);
			break;
		}
		Game_UpdateAreaPortionFill(pThis);
		iTileStartX = iTileTargetX;
		hWnd = (HWND)pThis[7];
		iTileStartY = iTileTargetY;
		UpdateWindow(hWnd);
		ret = Game_CSimcityViewMouseMoveOrLeftClick(pThis, &pt);
	} while (ret);
	if (iCurrToolGroupB != iCurrToolGroupA) {
		P_LOWORD(ret) = iCurrToolGroupB;
		wCurrentCityToolGroup = iCurrToolGroupB;
	}
	return ret;
}

extern "C" void __stdcall Hook_LoadCursorResources() {
	DWORD pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_LoadCursorResources)(void *) = (void(__thiscall *)(void *))0x4255A0;

	HDC hDC;

	hDC = GetDC(0);
	((DWORD *)pThis)[57] = GetDeviceCaps(hDC, HORZRES);
	ReleaseDC(0, hDC);
	H_LoadCursorResources((void *)pThis);
}

extern "C" int __stdcall Hook_StartupGraphics() {
	HDC hDC_One, hDC_Two;
	int iPlanes, iBitsPixel, iBitRate;
	PALETTEENTRY *p_pEnt;
	colStruct *pCol;
	DWORD pvIn;
	DWORD pvOut;
	tagLOGPAL plPal;

	HDC &hDC_Global = *(HDC *)0x4EA03C;
	HPALETTE &hLoColor = *(HPALETTE *)0x4EA044;
	BOOL &bHiColor = *(BOOL *)0x4EA048;
	BOOL &bLoColor = *(BOOL *)0x4EA04C;
	BOOL &bPaletteSet = *(BOOL *)0x4EA050;
	testColStruct *rgbLoColor = (testColStruct *)0x4EA058;
	testColStruct *rgbNormalColor = (testColStruct *)0x4EA0B8;

	plPal.wVersion = 0x300;
	plPal.wNumPalEnts = LOCOLORCNT;
	memset(plPal.pPalEnts, 0, sizeof(plPal.pPalEnts));
	hDC_One = 0;
	if (!hDC_Global) {
		hDC_One = GetDC(0);
		hDC_Global = CreateCompatibleDC(hDC_One);
	}

	hDC_Two = GetDC(0);
	iPlanes = GetDeviceCaps(hDC_Two, PLANES);
	iBitsPixel = GetDeviceCaps(hDC_Two, BITSPIXEL);
	if (iForcedBits > 0)
		iBitRate = iForcedBits;
	else
		iBitRate = iBitsPixel * iPlanes;

	if (iBitRate < 16) {
		if (iBitRate <= 4) {
			bLoColor = TRUE;
			pvIn = 4;
			if (Escape(hDC_Two, QUERYESCSUPPORT, 4, (LPCSTR)&pvIn, 0)) {
				p_pEnt = plPal.pPalEnts;
				pCol = rgbLoColor;
				do {
					Escape(hDC_Two, SETCOLORTABLE, 6, (LPCSTR)pCol, &pvOut);
					p_pEnt[pCol->wPos].peRed = pCol->pe.peRed;
					p_pEnt[pCol->wPos].peGreen = pCol->pe.peGreen;
					p_pEnt[pCol->wPos].peBlue = pCol->pe.peBlue;
					p_pEnt[pCol->wPos].peFlags = 1;
					pCol++;
				} while ( pCol->wPos < LOCOLORCNT );
				bPaletteSet = 1;
				SendMessageA(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
			}
			else {
				p_pEnt = plPal.pPalEnts;
				pCol = rgbNormalColor;
				bPaletteSet = 0;
				do {
					p_pEnt[pCol->wPos].peRed = pCol->pe.peRed;
					p_pEnt[pCol->wPos].peGreen = pCol->pe.peGreen;
					p_pEnt[pCol->wPos].peBlue = pCol->pe.peBlue;
					p_pEnt[pCol->wPos].peFlags = 1;
					pCol++;
				} while ( pCol->wPos < LOCOLORCNT );
			}
			hLoColor = CreatePalette((const LOGPALETTE *)&plPal);
		}
	}
	else {
		bHiColor = TRUE;
	}

	return ReleaseDC(0, hDC_Two);
}

// Placeholder.
void ShowModSettingsDialog(void) {
	MessageBox(NULL, "The mod settings dialog has not yet been implemented. Check back later.", "sc2fix", MB_OK);
}

// Install hooks and run code that we only want to do for the 1996 Special Edition SIMCITY.EXE.
// This should probably have a better name. And maybe be broken out into smaller functions.
void InstallMiscHooks_SC2K1996(void) {
	InstallRegistryPathingHooks_SC2K1996();

	// Install LoadStringA hook
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;

	// Install LoadMenuA hook
	*(DWORD*)(0x4EFDCC) = (DWORD)Hook_LoadMenuA;
	*(DWORD*)(0x4EFE58) = (DWORD)Hook_EnableMenuItem;
	*(DWORD*)(0x4EFC64) = (DWORD)Hook_DialogBoxParamA;

	// Install ShowWindow hook
	*(DWORD*)(0x4EFE70) = (DWORD)Hook_ShowWindow;

	// Only install this hook if SMK is enabled.
	if (smk_enabled) {
		// Install Smacker function hooks
		*(DWORD*)(0x4EFF00) = (DWORD)Hook_SmackOpen;
	}

	// Hook into the movie checking function.
	VirtualProtect((LPVOID)0x402360, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402360, Hook_MovieCheck);

	// Fix the sign fonts
	VirtualProtect((LPVOID)0x4E7267, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4E7267 = 'a';
	VirtualProtect((LPVOID)0x44DC42, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC42 = 5;
	VirtualProtect((LPVOID)0x44DC4F, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC4F = 10;

	// Hook what appears to be the exit function
	VirtualProtect((LPVOID)0x401753, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401753, Hook_ApparentExit);

	// Hook GameDoIdleUpkeep
	VirtualProtect((LPVOID)0x402A3B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402A3B, Hook_GameDoIdleUpkeep);

	// Fix the Maxis Presents logo not being shown
	VirtualProtect((LPVOID)0x4062B9, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x4062B9 = 1;
	VirtualProtect((LPVOID)0x4062AD, 12, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4062AD, Hook_4062AD);
	VirtualProtect((LPVOID)0x4E6130, 12, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy_s((LPVOID)0x4E6130, 12, "presnts.bmp", 12);

	// Fix power and water grid updates slowing down after the population hits 50,000
	VirtualProtect((LPVOID)0x440943, 4, PAGE_EXECUTE_READWRITE, &dwDummy); // 0x440170 <- CityToolMenuAction
	*(DWORD*)0x440943 = 50000000; // Power
	VirtualProtect((LPVOID)0x440987, 4, PAGE_EXECUTE_READWRITE, &dwDummy); // 0x440170 <- CityToolMenuAction
	*(DWORD*)0x440987 = 50000000; // Water
	VirtualProtect((LPVOID)0x43F429, 4, PAGE_EXECUTE_READWRITE, &dwDummy); // CityToolMenuAction
	*(DWORD*)0x43F429 = 50000000; // Water
	VirtualProtect((LPVOID)0x43F3A4, 4, PAGE_EXECUTE_READWRITE, &dwDummy); // CityToolMenuAction
	*(DWORD*)0x43F3A4 = 50000000; // Power
	
	// Fix city name being overwritten by filename on save
	BYTE bFilenamePatch[6] = { 0xB9, 0xA0, 0xA1, 0x4C, 0x00, 0x51 };
	VirtualProtect((LPVOID)0x42FE62, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE62, bFilenamePatch, 6);
	VirtualProtect((LPVOID)0x42FE99, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE99, bFilenamePatch, 6);

	// Adjust the Save File dialog type criterion
	VirtualProtect((LPVOID)0x4E7344, 32, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E7344, 0, 32);
	memcpy_s((LPVOID)0x4E7344, 32, "Simcity files (*.sc2)|*.sc2||", 32);

	// Fix save filenames going wonky 
	VirtualProtect((LPVOID)0x4321B9, 8, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4321B9, 0x90, 8);

	// Fix $1500 neighbor connections on game load
	VirtualProtect((LPVOID)0x434BEA, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x434BEA, Hook_LoadNeighborConnections1500);
	*(BYTE*)0x434BEF = 0x90;

	// Install hooks for the SC2X save format
	InstallSaveHooks();

	// Hook into the ResetGameVars function.
	VirtualProtect((LPVOID)0x401F05, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401F05, Hook_ResetGameVars);

	// Hook into the SimulationGrowthTick function
	VirtualProtect((LPVOID)0x4022FC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4022FC, Hook_SimulationGrowthTick);

	// Hook into the SimulationGrowSpecificZone function
	VirtualProtect((LPVOID)0x4026B2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026B2, Hook_SimulationGrowSpecificZone);

	// Hook into the PlacePowerLines function
	VirtualProtect((LPVOID)0x402725, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402725, Hook_PlacePowerLinesAtCoordinates);

	// Hook into what appears to be one of the item placement checking functions
	VirtualProtect((LPVOID)0x4027F2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4027F2, Hook_ItemPlacementCheck);

	// Military base hooks
	InstallMilitaryHooks();

	// Move the alt+query bottom text to not be blocked by the OK button
	VirtualProtect((LPVOID)0x428FB1, 3, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x428FB1 = 0x83;
	*(BYTE*)0x428FB2 = 0xE8;
	*(BYTE*)0x428FB3 = 0x32;
	
	// Install the advanced query hook
	if (bUseAdvancedQuery)
		InstallQueryHooks();

	// Fix the broken cheat
	// *** Only effective when the 'CMainFrame::OnChar' below is disabled. ***
	UINT uCheatPatch[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	memcpy_s((LPVOID)0x4E65C8, 10, "mrsoleary", 10);
	memcpy_s((LPVOID)0x4E6490, sizeof(uCheatPatch), uCheatPatch, sizeof(uCheatPatch));

	// Hook for CMainFrame::OnChar
	VirtualProtect((LPVOID)0x4029E1, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4029E1, Hook_MainFrameOnChar);

	// Increase sound buffer sizes to 256K each
	VirtualProtect((LPVOID)0x480C2B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C2B = 262144;
	VirtualProtect((LPVOID)0x480C4B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C4B = 262144;
	VirtualProtect((LPVOID)0x480C5B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C5B = 262144;
	VirtualProtect((LPVOID)0x480C6B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C6B = 262144;
	VirtualProtect((LPVOID)0x480C7B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C7B = 262144;

	// Load higher quality sounds from DLL resources
	LoadReplacementSounds();

	// Hook sound buffer loading
	VirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401F9B, Hook_LoadSoundBuffer);

	// Install music engine hooks
	InstallMusicEngineHooks();

	// Load weather icons
	for (int i = 0; i < 13; i++) {
		HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_WEATHER0 + i), IMAGE_BITMAP, 40, 40, NULL);
		if (hBitmap)
			hWeatherBitmaps[i] = hBitmap;
		else
			ConsoleLog(LOG_ERROR, "MISC: Couldn't load weather bitmap IDB_WEATHER%i: 0x%08X\n", i, GetLastError());
	}

	// Load compass icons
	for (int i = 0; i < 4; i++) {
		HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_COMPASS0 + i), IMAGE_BITMAP, 40, 40, NULL);
		if (hBitmap)
			hCompassBitmaps[i] = hBitmap;
		else
			ConsoleLog(LOG_ERROR, "MISC: Couldn't load compass bitmap IDB_COMPASS%i: 0x%08X\n", i, GetLastError());
	}

	// Hook status bar updates for the status dialog implementation
	VirtualProtect((LPVOID)0x402793, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402793, Hook_402793);
	VirtualProtect((LPVOID)0x4021A8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021A8, Hook_4021A8);
	VirtualProtect((LPVOID)0x40103C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40103C, Hook_40103C);

	// New hooks for CSimcityDoc::UpdateDocumentTitle and
	// SimulationProcessTick - these account for:
	// 1) Including the day of the month in the window title.
	// 2) The fine-grained simulation updates.
	VirtualProtect((LPVOID)0x4017B2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017B2, Hook_SimcityDocUpdateDocumentTitle);
	VirtualProtect((LPVOID)0x401820, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401820, Hook_SimulationProcessTick);

	// Hook SimulationStartDisaster
	VirtualProtect((LPVOID)0x402527, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402527, Hook_SimulationStartDisaster);

	// Hook AddAllInventions
	VirtualProtect((LPVOID)0x402388, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402388, Hook_AddAllInventions);

	// Add settings buttons to SC2K's menus
	hGameMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(3));
	if (hGameMenu) {
		AFX_MSGMAP_ENTRY afxMessageMapEntry[2];
		HMENU hOptionsPopup;
		MENUITEMINFO miiOptionsPopup;
		miiOptionsPopup.cbSize = sizeof(MENUITEMINFO);
		miiOptionsPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hGameMenu, 2, TRUE, &miiOptionsPopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		hOptionsPopup = miiOptionsPopup.hSubMenu;
		if (!InsertMenu(hOptionsPopup, -1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		if (!InsertMenu(hOptionsPopup, -1, MF_BYPOSITION|MF_STRING,  IDM_GAME_OPTIONS_SC2KFIXSETTINGS, "sc2kfix &Settings...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		if (!InsertMenu(hOptionsPopup, -1, MF_BYPOSITION|MF_STRING, IDM_GAME_OPTIONS_MODCONFIG, "Mod &Configuration...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}

		EnableMenuItem(hOptionsPopup, 5, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(hOptionsPopup, 6, MF_BYPOSITION | MF_ENABLED);

		afxMessageMapEntry[0] = {
			WM_COMMAND,
			0,
			IDM_GAME_OPTIONS_SC2KFIXSETTINGS,
			IDM_GAME_OPTIONS_SC2KFIXSETTINGS,
			0x0A,
			ShowSettingsDialog,
		};

		afxMessageMapEntry[1] = {
			WM_COMMAND,
			0,
			IDM_GAME_OPTIONS_MODCONFIG,
			IDM_GAME_OPTIONS_MODCONFIG,
			0x0A,
			ShowModSettingsDialog
		};

		VirtualProtect((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy_s((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));

		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated game menu.\n");
	}

skipgamemenu:

	// This case only occurs if the debug menu has been loaded
	// from the original non-hooked CMainFrame::OnChar function.
	hDebugMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(223));
	AdjustDebugMenu(hDebugMenu);

	// Hook for the game area leftmousebuttondown call.
	VirtualProtect((LPVOID)0x401523, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401523, Hook_CSimcityView_WM_LBUTTONDOWN);

	// Hook for the game area mouse movement call.
	VirtualProtect((LPVOID)0x4016EA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4016EA, Hook_CSimcityView_WM_MOUSEMOVE);

	// Hook for the MapToolMenuAction call.
	VirtualProtect((LPVOID)0x402B44, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B44, Hook_MapToolMenuAction);

	// Hook for CSimcityApp::LoadCursorResources
	VirtualProtect((LPVOID)0x402234, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402234, Hook_LoadCursorResources);

	// Hook for StartupGraphics
	VirtualProtect((LPVOID)0x4014DD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4014DD, Hook_StartupGraphics);

	// Add hook to center with the middle mouse button
	AFX_MSGMAP_ENTRY afxMessageMapEntrySimCityView = {
		WM_MBUTTONDOWN,
		0,
		0,
		0,
		0x2A,
		Hook_CSimcityView_WM_MBUTTONDOWN
	};
	VirtualProtect((LPVOID)0x4D45F0, sizeof(afxMessageMapEntrySimCityView), PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy_s((LPVOID)0x4D45F0, sizeof(afxMessageMapEntrySimCityView), &afxMessageMapEntrySimCityView, sizeof(afxMessageMapEntrySimCityView));

	// Copy the main menu's message map and update the runtime class to use it
	VirtualProtect((LPVOID)0x4D513C, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy_s(afxMessageMapMainMenu, sizeof(afxMessageMapMainMenu), (LPVOID)0x4D5140, sizeof(AFX_MSGMAP_ENTRY) * 8);
	afxMessageMapMainMenu[7] = { WM_COMMAND, 0, 118, 118, 0x0A, ShowSettingsDialog };
	afxMessageMapMainMenu[8] = { 0 };
	*(DWORD*)0x4D513C = (DWORD)afxMessageMapMainMenu;

	// Skip over the strange bit of code that re-arranges the original main menu.
	// 
	// For some reason the main menu dialog resource in simcity.exe has the Load Tile Set button
	// at the exact same coordinates as the Quit button. The code we're skipping (because we're
	// using our own dialog resource for the main menu) programatically resizes the dialog and
	// rearranges the buttons to fit on it. Why they didn't just fix the button coordinates in
	// the dialog resource instead is beyond me.
	VirtualProtect((LPVOID)0x41503F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP(0x41503F, 0x415161);
	*(BYTE*)0x415044 = 0x90;

	// Part two!
	UpdateMiscHooks_SC2K1996();
}

// The difference between InstallMiscHooks and UpdateMiscHooks is that UpdateMiscHooks can be run
// again at runtime because it can patch back in original game code. It's used for small stuff.
void UpdateMiscHooks_SC2K1996(void) {
	// Music in background
	VirtualProtect((LPVOID)0x40BFDA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	if (bSettingsMusicInBackground)
		memset((LPVOID)0x40BFDA, 0x90, 5);
	else {
		BYTE bOriginalCode[5] = { 0xE8, 0xFD, 0x50, 0xFF, 0xFF };
		memcpy_s((LPVOID)0x40BFDA, 5, bOriginalCode, 5);
	}
}

