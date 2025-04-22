// sc2kfix hooks/hook_miscellaneous.cpp: miscellaneous hooks to be injected
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

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

AFX_MSGMAP_ENTRY afxMessageMapMainMenu[9];
DLGPROC lpNewCityAfxProc = NULL;
char szTempMayorName[24] = { 0 };
char szCurrentMonthDay[24] = { 0 };
const char* szMonthNames[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

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

		strcpy_s(dwMapXLAB[0]->szLabel, 24, szTempMayorName);

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

	for (int x = 0; x < 128; x++) {
		for (int y = 0; y < 128; y++) {
			if (dwMapXTXT[x]->bTextOverlay[y] == 0xFA) {
				BYTE iTileID = dwMapXBLD[x]->iTileID[y];
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
		iTileID = (unsigned int)dwMapXBLD[x]->iTileID;
		P_LOBYTE(iTileID) = *(BYTE *)(iTileID + iY);
		iResult = iTileID & 0xFFFF00FF;
		if ((__int16)iResult < TILE_POWERLINES_LR) {
			Game_PlaceTileWithMilitaryCheck(x, iY, TILE_POWERLINES_LR);
TOTHISPART:
			if (x < 0x80 && iY < 0x80)
				*(BYTE *)&dwMapXBIT[x]->b[iY] |= 0x80u;
			iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x, iY);
			if (x > 0)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x - 1, iY);
			if (x < 127)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x + 1, iY);
			if (iY > 0)
				iResult = Game_CheckAdjustTerrainAndPlacePowerLines(x, iY - 1);
			if (iY < 127)
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
	if (x >= 0x80)
		goto TOBEGINNING;
	if (y >= 0x80)
		goto TOBEGINNING;
	iResult = x;
	if (*(BYTE *)&dwMapXBIT[x]->b[y] >= 0)
		goto TOBEGINNING;
	return iResult;
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
	signed __int16 iBudgetItemOne;
	__int16 iSomeValOne;
	char iRandSelectOne;
	__int16 iCalculateResDemand;
	__int16 iRemainderResDemand;
	__int16 iMapValPerhaps;
	__int16 iRandSelect;
	__int16 iNextX;
	__int16 iNextY;
	WORD &word_4C7CF8 = *(WORD *)0x4C7CF8;
	WORD *word_4DC4C8 = &*(WORD *)0x4DC4C8;
	WORD *word_4E7458 = &*(WORD *)0x4E7458;
	BYTE *byte_4E7508 = &*(BYTE *)0x4E7508;
	WORD *word_4E75B0 = &*(WORD *)0x4E75B0;
	WORD &word_4E99D8 = *(WORD *)0x4E99D8;
	WORD &word_4E99D0 = *(WORD *)0x4E99D0;
	DWORD *dword_4CB3EC = &*(DWORD *)0x4CB3EC;

	void(__cdecl *H_SpawnShip)(__int16, __int16) = (void(__cdecl *)(__int16, __int16))0x402829;
	int(__cdecl *H_SpawnHelicopter)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x402478;
	int(__cdecl *H_SpawnAeroplane)(__int16, __int16, __int16) = (int(__cdecl *)(__int16, __int16, __int16))0x4014CE;
	int(__fastcall *H_402B3F)(int) = (int(__fastcall *)(int))0x402B3F;
	unsigned int(__thiscall *H_402211)(DWORD *, int, int, int) = (unsigned int(__thiscall *)(DWORD *, int, int, int))0x402211;
	int(__cdecl *H_401E38)(__int16, __int16, __int16) = (int(__cdecl *)(__int16, __int16, __int16))0x401E38;
	int(__cdecl *H_4023B0)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x4023B0;
	int(__cdecl *H_401B40)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x401B40;
	int(__cdecl *H_40281F)(__int16, __int16, __int16, __int16, __int16) = (int(__cdecl *)(__int16, __int16, __int16, __int16, __int16))0x40281F;
	char(__cdecl *H_PerhapsGeneralZoneBuilding)(__int16, __int16, __int16, int) = (char(__cdecl *)(__int16, __int16, __int16, int))0x4024FA;
	int(__cdecl *H_4016F9)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x4016F9;
	char(__cdecl *H_PerhapsGeneralZoneSpawning)(__int16, __int16, __int16, __int16) = (char(__cdecl *)(__int16, __int16, __int16, __int16))0x402289;
	char(__cdecl *H_PerhapsGeneralZoneAreaBuilding)(signed __int16, signed __int16, __int16, __int16) = (char(__cdecl *)(signed __int16, signed __int16, __int16, __int16))0x401A3C;
	int(*H_402261)(void) = (int(*)(void))0x402261;
	__int16(__cdecl *H_40209F)(__int16, __int16) = (__int16(__cdecl *)(__int16, __int16))0x40209F;
	int(__cdecl *H_402978)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x402978;
	// Newspaper....
	int(__cdecl *H_402900)(__int16, char) = (int(__cdecl *)(__int16, char))0x402900;

	pThis = (DWORD *)Game_PointerToCSimcityViewClass(pCWinAppThis);
	iAttributes = dwCityPopulation;
	iX = iStep;
	bPlaceChurch = 2500u * (__int16)dwTileCount[TILE_INFRASTRUCTURE_CHURCH] < (unsigned int)dwCityPopulation;
	word_4C7CF8 = word_4DC4C8[wViewRotation];
	iResult = iStep / 2;
	iXMM = iStep / 2;
	while (iX < 0x80) {
		iY = iSubStep;
		for (i = iSubStep / 2; ; i = iY / 2) {
			iYMM = i;
			if (iY >= 0x80)
				break;
			iXPos = iX;
			maXZON = dwMapXZON[iXPos]->b[iY];
			iCurrZoneType = maXZON.iZoneType;
			iPosAttributes = iXPos * 4;
			P_LOBYTE(iPosAttributes) = dwMapXBLD[iXPos]->iTileID[iY];
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
										iAttributes = 5;
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
											if ((__int16)dwMilitaryTiles[MILITARYTILE_TOPSECRET] / 4 < iBuildingCount) {
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
									goto GOIYINCREASE;
							}
							break;
						case ZONE_SEAPORT:
							if ((rand() & 3) != 0) {
								if ((WORD)iAttributes == TILE_INFRASTRUCTURE_CRANE && (rand() & 3) == 0)
									H_SpawnShip(iX, iY);
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
									iX < 0x80 &&
									iY < 0x80) {
									iAttributes = (int)&dwMapXBIT[iXPos];
									if (dwMapXBIT[iX]->b[iY].iPowered != 0) {
										if (rand() % 10 < 4) {
											H_SpawnHelicopter(iX, iY);
											break;
										}
										if ((wViewRotation & 1) != 0) {
											if (iX < 0x80 &&
												iY < 0x80 &&
												(*(BYTE *)(*(DWORD *)iAttributes + iY) & 2) != 0)
												goto AIRFIELDSKIPAHEAD;
										}
										else if (iX >= 0x80 ||
											iY >= 0x80 ||
											(*(BYTE *)(*(DWORD *)iAttributes + iY) & 2) == 0) {
AIRFIELDSKIPAHEAD:
											H_SpawnAeroplane(iX, iY, 0);
											break;
										}
										H_SpawnAeroplane(iX, iY, 2);
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
							if (!(unsigned __int16)H_402B3F(iPosAttributes)) {
								P_LOBYTE(iAttributes) = dwMapXUND[iX]->iTileID[iY];
								iAttributes &= 0xFFFF00FF;
								if ((__int16)iAttributes >= TILE_RUBBLE1 && (__int16)iAttributes < TILE_POWERLINES_HTB ||
									(WORD)iAttributes == TILE_ROAD_BR ||
									(WORD)iAttributes == TILE_ROAD_HTB ||
									(WORD)iAttributes == TILE_ROAD_LHR ||
									(WORD)iAttributes == TILE_ROAD_HLR) {
									iBudgetItemOne = *((WORD *)dwBudgetArr + 834);
									ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - (WORD)iAttributes(%u), iBudgetItemOne(%d)\n", iStep, iSubStep, (WORD)iAttributes, iBudgetItemOne);
									if (iBudgetItemOne != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iBudgetItemOne) {
										if ((WORD)iAttributes == TILE_ROAD_BR)
											H_402211(pThis, iX, iY, 0);
										else
											H_401E38(iX, iY, 0);
									}
								}
							}
							break;
					}
				}
				else {
					if ((__int16)iAttributes >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
						if ((*(BYTE *)&maXZON & 0xF0 & (char)word_4C7CF8) == 0) {
							goto GOIYINCREASE;
						}
						P_LOWORD(iAttributes) = iAttributes - TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1;
						iSomeValOne = word_4E7458[(__int16)iAttributes];
					}
					else {
						if ((__int16)iAttributes >= TILE_ROAD_LR || !H_4023B0(iX, iY)) {
							goto GOIYINCREASE;
						}
						P_LOWORD(iAttributes) = 0;
						iSomeValOne = 0;
					}
					if (H_401B40(iX, iY)) {
						if (H_40281F(iX, iY, iCurrZoneType, iSomeValOne, 100)) {
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
					if (iSomeValOne > 0 && !byte_4E7508[(__int16)iAttributes]) {
						pZonePops[iCurrZoneType] += word_4E75B0[iSomeValOne];
						if ((unsigned __int16)rand() < (__int16)(iRemainderResDemand / iSomeValOne)) {
							iRandSelectOne = rand() & 1;
							ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - iSomeValOne(%d), iRandSelect(%d)\n", iX, iY, iSomeValOne, iRandSelectOne);
							H_PerhapsGeneralZoneBuilding(iX, iY, iSomeValOne, iRandSelectOne);
							goto GOIYINCREASE;
						}
					}
					iAttributes = (int)&byte_4E7508[(__int16)iAttributes];
					if (*(BYTE *)iAttributes == 1 && (unsigned __int16)rand() < 0x4000 / iSomeValOne) {
						if (bPlaceChurch && (iSomeValOne & 2) != 0 && iCurrZoneType < ZONE_LIGHT_COMMERCIAL) {
							H_4016F9(iX, iY);
							goto GOIYINCREASE;
						}
GOGENERALZONESPAWNING:
						H_PerhapsGeneralZoneSpawning(iX, iY, iSomeValOne, (iCurrZoneType - 1) / 2);
						goto GOIYINCREASE;
					}
					if (*(BYTE *)iAttributes == 2) {
						iAttributes = iSomeValOne;
						pZonePops[7] += word_4E75B0[iSomeValOne];
						if ((unsigned __int16)rand() >= 15 * iCalculateResDemand / iSomeValOne) {
							goto GOIYINCREASE;
						}
						goto GOGENERALZONESPAWNING;
					}
					if (iSomeValOne != 4 &&
						((iCurrZoneType & 1) == 0 || iSomeValOne <= 0) &&
						(iCurrZoneType >= 5 ||
						(iSomeValOne != 1 || dwMapXVAL[iXMM]->bBlock[iYMM] >= 0x20u) &&
							(iSomeValOne != 2 || dwMapXVAL[iXMM]->bBlock[iYMM] >= 0x60u) &&
							(iSomeValOne != 3 || dwMapXVAL[iXMM]->bBlock[iYMM] >= 0xC0u))) {
						iAttributes = 3 * iCalculateResDemand / (iSomeValOne + 1);
						if (iAttributes > (unsigned __int16)rand()) {
							H_PerhapsGeneralZoneAreaBuilding(iX, iY, iSomeValOne, iCurrZoneType);
						}
					}
				}
			}
			else {
				if ((__int16)iAttributes < TILE_ROAD_LR)
					goto GOIYINCREASE;
				if ((unsigned __int16)H_402B3F(iPosAttributes))
					goto GOAFTERSETXBIT;
				if ((__int16)iAttributes >= TILE_ROAD_LR && (__int16)iAttributes < TILE_RAIL_LR ||
					(__int16)iAttributes >= TILE_TUNNEL_T && (__int16)iAttributes < TILE_CROSSOVER_POWERTB_RAILLR ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYLR_ROADTB ||
					(WORD)iAttributes == TILE_CROSSOVER_HIGHWAYTB_ROADLR ||
					(__int16)iAttributes >= TILE_ONRAMP_TL && (__int16)iAttributes < TILE_HIGHWAY_HTB) {
					iBudgetItemOne = *((WORD *)dwBudgetArr + 610);
					if (iBudgetItemOne != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iBudgetItemOne) {
						iRandSelectOne = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iRandSelectOne);
						if (iX >= 0x80 || iY >= 0x80) {
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
					iBudgetItemOne = *((WORD *)dwBudgetArr + 778);
					if (iBudgetItemOne != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iBudgetItemOne) {
						iRandSelectOne = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iRandSelectOne);
						if (iX >= 0x80 || iY >= 0x80) {
							goto GOAFTERSETXBIT;
						}
GOBEFORESETXBIT:
						*(BYTE *)&dwMapXBIT[iX]->b[iY] &= ~0x80u;
GOAFTERSETXBIT:
						if ((__int16)iAttributes >= TILE_INFRASTRUCTURE_RAILSTATION) {
							if ((WORD)iAttributes == TILE_INFRASTRUCTURE_RAILSTATION &&
								iX < 0x80 &&
								iY < 0x80 &&
								dwMapXBIT[iX]->b[iY].iPowered != 0 &&
								!(unsigned __int16)H_402261()) {
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_RAILSTATION] / 4 > word_4E99D8)
									H_40209F(iX, iY);
							}
						}
						else if ((WORD)iAttributes == TILE_INFRASTRUCTURE_MARINA &&
							iX < 0x80 &&
							iY < 0x80 &&
							dwMapXBIT[iX]->b[iY].iPowered != 0 &&
							!(unsigned __int16)H_402261()) {
							if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_MARINA] / 9 > word_4E99D0)
								H_402978(iX, iY);
						}
						else if ((__int16)iAttributes >= TILE_ARCOLOGY_PLYMOUTH &&
							(__int16)iAttributes <= TILE_ARCOLOGY_LAUNCH &&
							(*(BYTE *)&dwMapXZON[iX]->b[iY] & 0xF0) == 0x80) {
							P_LOBYTE(iAttributes) = dwMapXTXT[iX]->bTextOverlay[iY];
							iAttributes &= 0xFFFF00FF;
							if ((__int16)iAttributes >= 51 &&
								(__int16)iAttributes < 201 &&
								LOBYTE(dword_4CB3EC[2 * (__int16)iAttributes - 102]) >= 0xFBu &&
								LOBYTE(dword_4CB3EC[2 * (__int16)iAttributes - 102]) != 255) {
								P_LOWORD(iAttributes) = iAttributes - 51;
								iMapValPerhaps = (*(BYTE *)&dwMapXVAL[2 * (__int16)(iX & 0xFFFE)]->bBlock[iY >> 1] >> 5)
									- (*(BYTE *)&dwMapXCRM[2 * (__int16)(iX & 0xFFFE)]->bBlock[iY >> 1] >> 5)
									- (*(BYTE *)&dwMapXPLT[2 * (__int16)(iX & 0xFFFE)]->bBlock[iY >> 1] >> 5)
									+ 12;
								if (iX >= 0x80 ||
									iY >= 0x80 ||
									dwMapXBIT[iX]->b[iY].iPowered == 0)
									iMapValPerhaps /= 2;
								if (iX >= 0x80 ||
									iY >= 0x80 ||
									dwMapXBIT[iX]->b[iY].iWatered == 0)
									iMapValPerhaps /= 2;
								if (iMapValPerhaps < 0)
									iMapValPerhaps = 0;
								if (iMapValPerhaps > 12)
									P_LOBYTE(iMapValPerhaps) = 12;
								*((BYTE *)dword_4CB3EC + 8 * (__int16)iAttributes + 1) = (BYTE)iMapValPerhaps;
							}
						}
					}
				}
				else if ((__int16)iAttributes >= TILE_SUSPENSION_BRIDGE_START_B && (__int16)iAttributes < TILE_ONRAMP_TL ||
					(WORD)iAttributes == TILE_REINFORCED_BRIDGE_PYLON ||
					(WORD)iAttributes == TILE_REINFORCED_BRIDGE) {
					iBudgetItemOne = *((WORD *)dwBudgetArr + 722);
					if (iBudgetItemOne != 100 && (int)((unsigned __int8)bWeatherWind + (unsigned __int16)rand() % 50u) >= iBudgetItemOne) {
						Game_CenterOnTileCoords(iX, iY);
						H_402211(pThis, iX, iY, 1);
						H_402900(39, 0);
						goto GOAFTERSETXBIT;
					}
				}
				else if ((__int16)iAttributes < TILE_TUNNEL_T || (__int16)iAttributes >= TILE_CROSSOVER_POWERTB_ROADLR) {
					if (((__int16)iAttributes < TILE_HIGHWAY_HTB || (__int16)iAttributes >= TILE_SUBTORAIL_T) &&
						((__int16)iAttributes < TILE_HIGHWAY_LR || (__int16)iAttributes >= TILE_SUSPENSION_BRIDGE_START_B))
						goto GOAFTERSETXBIT;
					if ((iX & 1) == 0 && (iY & 1) == 0) {
						iBudgetItemOne = *((WORD *)dwBudgetArr + 666);
						if (iBudgetItemOne != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iBudgetItemOne) {
							if (iX < 0x80 &&
								iY < 0x80 &&
								dwMapXBIT[iX]->b[iY].iWater != 0)
								Game_PlaceTileWithMilitaryCheck(iX, iY, 0);
							else {
								iRandSelect = (rand() & 3) + 1;
								Game_PlaceTileWithMilitaryCheck(iX, iY, iRandSelect);
							}
							iNextX = iX + 1;
							if ((iX + 1) >= 0 &&
								iNextX < 0x80 &&
								iY < 0x80 &&
								dwMapXBIT[iNextX]->b[iY].iWater != 0)
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY, 0);
							else {
								iRandSelect = (rand() & 3) + 1;
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY, iRandSelect);
							}
							iNextY = iY + 1;
							if (iX < 0x80 &&
								(iY + 1) >= 0 &&
								iNextY < 0x80 &&
								dwMapXBIT[iX]->b[iNextY].iWater != 0)
								Game_PlaceTileWithMilitaryCheck(iX, iY + 1, 0);
							else {
								iRandSelect = (rand() & 3) + 1;
								Game_PlaceTileWithMilitaryCheck(iX, iY + 1, iRandSelect);
							}
							if ((iX + 1) < 0x80 &&
								(iY + 1) < 0x80 &&
								dwMapXBIT[(iX + 1)]->b[(iY + 1)].iWater != 0)
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY + 1, 0);
							else {
								iRandSelect = (rand() & 3) + 1;
								Game_PlaceTileWithMilitaryCheck(iX + 1, iY + 1, iRandSelect);
							}
							goto GOAFTERSETXBIT;
						}
					}
				}
				else {
					iBudgetItemOne = *((WORD *)dwBudgetArr + 890);
					if (iBudgetItemOne != 100 && (unsigned __int16)((unsigned __int16)rand() % 100u) >= iBudgetItemOne) {
						Game_CenterOnTileCoords(iX, iY);
						H_402211(pThis, iX, iY, 1);
						goto GOAFTERSETXBIT;
					}
				}
			}
GOIYINCREASE:
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

	int ret = H_SimulationGrowthTick(x, y);
	ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> SimulationGrowthTick(%d, %d) = %d\n", _ReturnAddress(), x, y, ret);

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
			while (iCurrX < 0x80 && iCurrY < 0x80) {
				if (dwMapXZON[iCurrX]->b[iCurrY].iZoneType != iZoneType)
					return 0;
				mXBuilding[0] = dwMapXBLD[iCurrX]->iTileID[iCurrY];
				if (iZoneType == ZONE_MILITARY) {
					if ((mXBuilding[0] >= TILE_ROAD_LR && mXBuilding[0] <= TILE_ROAD_LTBR) ||
						mXBuilding[0] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[0] == TILE_MILITARY_MISSILESILO)
						return 0;
					if (dwMapXTER[iCurrX]->iTileID[iCurrY])
						return 0;
					if (dwMapXUND[iCurrX]->iTileID[iCurrY])
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
						mXBuilding[1] = dwMapXBLD[x]->iTileID[y];
						if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAYCROSS) {
							--iBuildingCount[1];
							if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY) {
								iTileRotated = x < 0x80 &&
									y < 0x80 &&
									dwMapXBIT[x]->b[y].iRotated;
								if (iTileRotated != iRotate) {
									Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAYCROSS);
									if (x < 0x80 && y < 0x80)
										*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
									if (iZoneType != ZONE_MILITARY) {
										if (x <= -1)
											goto RUNWAY_GETOUT;
										if (x < 0x80 && y < 0x80)
											*(BYTE *)&dwMapXBIT[x]->b[y] |= 0xC0u;
									}
									if (x < 0x80 && y < 0x80) {
										mXBIT = dwMapXBIT[x];
										mXBBits = (*(BYTE *)&mXBIT->b[y] & 0xFD);
RUNWAY_GOBACK:
										*(BYTE *)&mXBIT->b[y] = mXBBits;
									}
								}
							}
						}
						else {
							if (iZoneType == ZONE_MILITARY) {
								if ((mXBuilding[1] >= TILE_ROAD_LR && mXBuilding[1] <= TILE_ROAD_LTBR) ||
									mXBuilding[1] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[1] == TILE_MILITARY_MISSILESILO)
									return 0;
								if (dwMapXTER[x]->iTileID[y])
									return 0;
								if (dwMapXUND[x]->iTileID[y])
									return 0;
							}
							if (dwMapXBLD[x]->iTileID[y] >= TILE_SMALLPARK)
								Game_ZonedBuildingTileDeletion(x, y);
							Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAY);
							if (x < 0x80 && y < 0x80)
								*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
							if (iZoneType != ZONE_MILITARY && x < 0x80 && y < 0x80)
								*(BYTE *)&dwMapXBIT[x]->b[y] |= 0xC0u;
							if (iRotate && x < 0x80 && y < 0x80) {
								mXBIT = dwMapXBIT[x];
								mXBBits = (*(BYTE *)&mXBIT->b[y] | 2);
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
				if (iLengthWays < 0x80) {
					iDepthWays = y + wSomePierDepthWays[i];
					if (iDepthWays < 0x80 && dwMapXBIT[iLengthWays]->b[iDepthWays].iWater != 0)
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
				if (iNextX >= 0x80 || iNextY >= 0x80)
					return 0;
				if (iNextX >= 0x80 ||
					iNextY >= 0x80 ||
					dwMapXBIT[iNextX]->b[iNextY].iWater == 0)
					return 0;
				if (dwMapXBLD[iNextX]->iTileID[iNextY])
					return 0;
				++iPierPathTileCount;
			} while (iPierPathTileCount < 5);
			if ((*(WORD *)&dwMapALTM[iNextX]->w[iNextY] & 0x3E0) >> 5 < (*(WORD *)&dwMapALTM[iNextX]->w[iNextY] & 0x1F) + 2)
				return 0;
			if (dwMapXBLD[x]->iTileID[y] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(x, y);
			Game_ItemPlacementCheck(x, y, TILE_INFRASTRUCTURE_CRANE, 1);
			if (x < 0x80 && y < 0x80) {
				pZone = (BYTE *)&dwMapXZON[x]->b[y];
				*pZone ^= (*pZone ^ iZoneType) & 0xF;
			}
			if (iZoneType == ZONE_MILITARY && x < 0x80 && y < 0x80)
				*(BYTE *)&dwMapXBIT[x]->b[y] &= 0xFu;
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
				if (x < 0x80 && y < 0x80)
					*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
				if (iRotate) {
					if (x < 0x80 && y < 0x80)
						*(BYTE *)&dwMapXBIT[x]->b[y] |= 2u;
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
			if (dwMapXBLD[x]->iTileID[y] < TILE_SMALLPARK) {
				Game_ItemPlacementCheck(x, y, iTileID, 1);
				if (x < 0x80 && y < 0x80)
					*(BYTE *)&dwMapXZON[x]->b[y] ^= (*(BYTE *)&dwMapXZON[x]->b[y] ^ iZoneType) & 0xF;
				if (iZoneType == ZONE_MILITARY && x < 0x80 && y < 0x80)
					*(BYTE *)&dwMapXBIT[x]->b[y] &= 0xFu;
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
			mXBuilding[0] = mXBLDOne->iTileID[y];
			if (mXBuilding[0] >= TILE_INFRASTRUCTURE_WATERTOWER)
				return 0;
			if (mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[0] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[0] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[0] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBLDTwo = dwMapXBLD[iNextX];
			mXBuilding[1] = mXBLDTwo->iTileID[y];
			if (mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[1] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[1] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[1] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBuilding[2] = mXBLDOne->iTileID[iNextY];
			if (mXBuilding[2] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[2] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[2] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[2] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXBuilding[3] = mXBLDTwo->iTileID[iNextY];
			if (mXBuilding[3] == TILE_INFRASTRUCTURE_RUNWAY || mXBuilding[3] == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
				mXBuilding[3] == TILE_INFRASTRUCTURE_CRANE || mXBuilding[3] == TILE_MILITARY_MISSILESILO)
				return 0;
			mXZONOne = dwMapXZON[iSX];
			if (mXZONOne->b[y].iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONOne->b[y].iZoneType == ZONE_MILITARY) {
					if (mXBuilding[0] >= TILE_ROAD_LR && mXBuilding[0] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iSX]->iTileID[y])
					return 0;
			}
			mXZONTwo = dwMapXZON[iNextX];
			if (mXZONTwo->b[y].iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONTwo->b[y].iZoneType == ZONE_MILITARY) {
					if (mXBuilding[1] >= TILE_ROAD_LR && mXBuilding[1] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iNextX]->iTileID[y])
					return 0;
			}
			if (mXZONOne->b[iNextY].iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONOne->b[iNextY].iZoneType == ZONE_MILITARY) {
					if (mXBuilding[2] >= TILE_ROAD_LR && mXBuilding[2] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iSX]->iTileID[iNextY])
					return 0;
			}
			if (mXZONTwo->b[iNextY].iZoneType != iZoneType)
				return 0;
			if (iZoneType == ZONE_MILITARY) {
				if (mXZONTwo->b[iNextY].iZoneType == ZONE_MILITARY) {
					if (mXBuilding[3] >= TILE_ROAD_LR && mXBuilding[3] <= TILE_ROAD_LTBR)
						return 0;
				}
				if (dwMapXUND[iNextX]->iTileID[iNextY])
					return 0;
			}
			if (mXBuilding[0] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iSX, y);
			if (dwMapXBLD[iNextX]->iTileID[y] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iNextX, y);
			if (dwMapXBLD[iSX]->iTileID[iNextY] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iSX, iNextY);
			if (dwMapXBLD[iNextX]->iTileID[iNextY] >= TILE_SMALLPARK)
				Game_ZonedBuildingTileDeletion(iNextX, iNextY);
			Game_ItemPlacementCheck(iSX, y, iTileID, 2);
			if (iSX < 0x80 && y < 0x80)
				*(BYTE *)&dwMapXZON[iSX]->b[y] ^= (*(BYTE *)&dwMapXZON[iSX]->b[y] ^ iZoneType) & 0xF;
			if (iNextX < 0x80 && y < 0x80)
				*(BYTE *)&dwMapXZON[iNextX]->b[y] ^= (*(BYTE *)&dwMapXZON[iNextX]->b[y] ^ iZoneType) & 0xF;
			if (iSX < 0x80 && iNextY < 0x80)
				*(BYTE *)&dwMapXZON[iSX]->b[iNextY] ^= (*(BYTE *)&dwMapXZON[iSX]->b[iNextY] ^ iZoneType) & 0xF;
			if (iNextX < 0x80 && iNextY < 0x80)
				*(BYTE *)&dwMapXZON[iNextX]->b[iNextY] ^= (*(BYTE *)&dwMapXZON[iNextX]->b[iNextY] ^ iZoneType) & 0xF;
			if (iZoneType == ZONE_MILITARY) {
				if (iSX < 0x80 && y < 0x80)
					*(BYTE *)&dwMapXBIT[iSX]->b[y] &= 0xFu;
				if (iNextX < 0x80 && y < 0x80)
					*(BYTE *)&dwMapXBIT[iNextX]->b[y] &= 0xFu;
				if (iSX < 0x80 && iNextY < 0x80)
					*(BYTE *)&dwMapXBIT[iSX]->b[iNextY] &= 0xFu;
				if (iNextX < 0x80 && iNextY < 0x80)
					*(BYTE *)&dwMapXBIT[iNextX]->b[iNextY] &= 0xFu;
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
	__int16 iSection[3];
	char cMSimBit;
	BYTE *pZone;

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
				if (iX >= 0x80 || iY >= 0x80)
					return 0;
			}
			else if (iX < 1 || iY < 1 || iX > 126 || iY > 126) {
				return 0;
			}
			iBuilding = dwMapXBLD[iX]->iTileID[iY];
			if (iBuilding >= TILE_ROAD_LR) {
				return 0;
			}
			if (iBuilding == TILE_RADIOACTIVITY) {
				return 0;
			}
			if (iBuilding == TILE_SMALLPARK) {
				return 0;
			}
			if (dwMapXZON[iX]->b[iY].iZoneType == ZONE_MILITARY) {
				if (iBuilding == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
					iBuilding == TILE_ROAD_LR ||
					iBuilding == TILE_ROAD_TB)
					return 0;
			}
			if (iTileID == TILE_INFRASTRUCTURE_MARINA) {
				if (iX < 0x80 &&
					iY < 0x80 &&
					dwMapXBIT[iX]->b[iY].iWater != 0) {
					++iMarinaCount;
					goto GOSKIP;
				}
				if (dwMapXTER[iX]->iTileID[iY]) {
					return 0;
				}
			}
			if (dwMapXTER[iX]->iTileID[iY]) {
				return 0;
			}
			if (iX < 0x80 &&
				iY < 0x80 &&
				dwMapXBIT[iX]->b[iY].iWater != 0) {
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
		if (iTile == TILE_SMALLPARK && dwMapXBLD[x]->iTileID[y] > TILE_SMALLPARK) {
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
							if (iCurrXPos < 0x80 && iCurrYPos < 0x80) {
								*(BYTE *)&dwMapXBIT[iCurrXPos]->b[iCurrYPos] &= 0x1Fu;
							}
							if (iCurrXPos < 0x80 && iCurrYPos < 0x80) {
								*(BYTE *)&dwMapXBIT[iCurrXPos]->b[iCurrYPos] |= iMapBit;
							}
						}
						Game_PlaceTileWithMilitaryCheck(iCurrXPos, iCurrYPos, iTile);
						if (iCurrXPos > -1) {
							if (iCurrXPos < 0x80 && iCurrYPos < 0x80) {
								*(BYTE *)&dwMapXZON[iCurrXPos]->b[iCurrYPos] &= 0xF0u;
							}
							if (iCurrXPos < 0x80 && iCurrYPos < 0x80) {
								*(BYTE *)&dwMapXZON[iCurrXPos]->b[iCurrYPos] &= 0xFu;
							}
						}
						if (cMSimBit) {
							*(BYTE *)&dwMapXTXT[iCurrXPos]->bTextOverlay[iCurrYPos] = cMSimBit;
						}
					}
					++iCurrXPos;
				} while (iCurrXPos <= iItemWidth);
			}
			if (iArea) {
				if (x < 0x80 && y < 0x80) {
					pZone = (BYTE *)&dwMapXZON[x]->b[y];
					*pZone = LOBYTE(wSomePositionalAngleOne[4 * wViewRotation]) | *pZone & 0xF;
				}
				iSection[0] = iArea + x;
				if ((iArea + x) > -1 && iSection[0] < 0x80 && y < 0x80) {
					pZone = (BYTE *)&dwMapXZON[iSection[0]]->b[y];
					*pZone = LOBYTE(wSomePositionalAngleTwo[4 * wViewRotation]) | *pZone & 0xF;
				}
				if (iSection[0] < 0x80) {
					iSection[1] = y + iArea;
					if ((y + iArea) > -1 && iSection[1] < 0x80) {
						pZone = (BYTE *)&dwMapXZON[iSection[0]]->b[iSection[1]];
						*pZone = LOBYTE(wSomePositionalAngleThree[4 * wViewRotation]) | *pZone & 0xF;
					}
				}
				if (x < 0x80) {
					iSection[2] = iArea + y;
					if ((iArea + y) > -1 && iSection[2] < 0x80) {
						pZone = (BYTE *)&dwMapXZON[x]->b[iSection[2]];
						*pZone = LOBYTE(wSomePositionalAngleFour[4 * wViewRotation]) | *pZone & 0xF;
					}
				}
			}
			else if (x < 0x80 && y < 0x80) {
				*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
			}
			Game_SpawnItem(x, y + iArea);
			return 1;
		}
	}
}

// Window title hook, part 1
extern "C" char* __stdcall Hook_40D67D(void) {
	if (bSettingsTitleCalendar)
		sprintf_s(szCurrentMonthDay, 24, "%s %d,", szMonthNames[dwCityDays / 25 % 12], dwCityDays % 25 + 1);
	else
		sprintf_s(szCurrentMonthDay, 24, "%s", szMonthNames[dwCityDays / 25 % 12]);
	return szCurrentMonthDay;
}

std::vector<hook_function_t> stHooks_Hook_SimulationProcessTickDaySwitch_Before;

// Window title hook, part 2 and refresh hook
// TODO: Clean this hook up to be as pure C/C++ as possible. I'm sure we can make it nice and
// clean, I just need more time to fiddle with it.
extern "C" void _declspec(naked) Hook_SimulationProcessTickDaySwitch(void) {
	__asm push edx

	for (const auto& hook : stHooks_Hook_SimulationProcessTickDaySwitch_Before) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void) = (void(*)(void))hook.pFunction;
			fnHook();
		}
		if (bHookStopProcessing)
			__asm jmp def;
	}

	Game_RefreshTitleBar(pCDocumentMainWindow);

	if (bSettingsFrequentCityRefresh) {
		POINT pt;
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
		GetCursorPos(&pt);
		if (wCityMode && wCurrentCityToolGroup != TOOL_GROUP_CENTERINGTOOL && Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y) < 0x8000) {
			Game_DrawSquareHighlight(wHighlightedTileX1, wHighlightedTileY1, wHighlightedTileX2, wHighlightedTileY2);
			wTileHighlightActive = 1;
			RedrawWindow(GameGetRootWindowHandle(), NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
		}
	}

__asm {
		pop edx
		cmp edx, 24
		ja def

		push 0x4135DB
		retn

def:
		push 0x413ABF
		retn
	}
}

std::vector<hook_function_t> stHooks_Hook_SimulationProcessTickDaySwitch_After;

extern "C" void _declspec(naked) Hook_SimulationProcessTickDaySwitch_After(void) {
	for (const auto& hook : stHooks_Hook_SimulationProcessTickDaySwitch_After) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void) = (void(*)(void))hook.pFunction;
			fnHook();
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

BAIL:
	// Original cleanup from 0x413ABF
	__asm {
		mov eax, [ebp-0x0C]
		pop esi
		mov fs:0, eax
		pop ebx
		mov esp, ebp
		pop ebp
		retn
	}
}

extern "C" void _declspec(naked) Hook_SimulationStartDisaster(void) {
	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationStartDisaster(), wDisasterType = %u.\n", _ReturnAddress(), wDisasterType);

	GAMEJMP(0x45CF10)
}

extern "C" int __cdecl Hook_SimulationPrepareDisaster(DWORD* a1, __int16 a2, __int16 a3) {
	//if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
	//	ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationPrepareDisaster(0x%08X, %i, %i).\n", _ReturnAddress(), a1, a2, a3);

	a1[0] = a2;
	a1[1] = a3;

	return a2;
}

extern "C" int __stdcall Hook_AddAllInventions(void) {
	if (mischook_debug & MISCHOOK_DEBUG_CHEAT)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> AddAllInventions()\n", _ReturnAddress());

	memset(wCityInventionYears, 0, sizeof(WORD)*MAX_CITY_INVENTION_YEARS);
	Game_ToolMenuUpdate();
	Game_SoundPlaySound(pCWinAppThis, SOUND_ZAP);

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
			Game_SoundPlaySound(pCWinAppThis, SOUND_CLICK);
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
	int iThisSomething;

	P_LOWORD(iTileCoords) = (WORD)pt.x;
	iThisSomething = *(DWORD *)(pThis + 252);
	*(struct tagPOINT *)(pThis + 260) = pt; // Placement position.
	if (iThisSomething) {
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

	pThis = (DWORD *)Game_PointerToCSimcityViewClass(pCWinAppThis);	// TODO: is this necessary or can we just dereference pCSimcityView?
	Game_TileHighlightUpdate((int)pThis);
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
		iTileTargetX = ret & 0x7F;
		iTileTargetY = (__int16)ret >> 8;
		if ((unsigned __int16)iTileTargetX >= 0x80u || iTileTargetY < 0)
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
				Game_SoundPlaySound(pCWinAppThis, SOUND_FLOOD);
				break;
			case MAPTOOL_GROUP_TREES: // Place Tree
			case MAPTOOL_GROUP_FOREST: // Place Forest
				if (!Game_MapToolSoundTrigger(dwAudioHandle))
					Game_SoundPlaySound(pCWinAppThis, SOUND_PLOP);
				if (iCurrToolGroupA == 7)
					Game_MapToolPlaceTree(iTileTargetX, iTileTargetY);
				else
					Game_MapToolPlaceForest(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_CENTERINGTOOL: // Center Tool
				Game_GetScreenCoordsFromTileCoords(iTileTargetX, iTileTargetY, &wNewScreenPointX, &wNewScreenPointY);
				Game_SoundPlaySound(pCWinAppThis, SOUND_CLICK);
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

// Placeholder.
void ShowModSettingsDialog(void) {
	MessageBox(NULL, "The mod settings dialog has not yet been implemented. Check back later.", "sc2fix", MB_OK);
}

// Install hooks and run code that we only want to do for the 1996 Special Edition SIMCITY.EXE.
// This should probably have a better name. And maybe be broken out into smaller functions.
void InstallMiscHooks(void) {
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
	VirtualProtect((LPVOID)0x440943, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440943 = 50000000;
	VirtualProtect((LPVOID)0x440987, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440987 = 50000000;
	VirtualProtect((LPVOID)0x43F429, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F429 = 50000000;
	VirtualProtect((LPVOID)0x43F3A4, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F3A4 = 50000000;
	
	// Fix city name being overwritten by filename on save
	BYTE bFilenamePatch[6] = { 0xB9, 0xA0, 0xA1, 0x4C, 0x00, 0x51 };
	VirtualProtect((LPVOID)0x42FE62, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE62, bFilenamePatch, 6);
	VirtualProtect((LPVOID)0x42FE99, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE99, bFilenamePatch, 6);

	// Fix save filenames going wonky 
	VirtualProtect((LPVOID)0x4321B9, 8, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4321B9, 0x90, 8);

	// Fix $1500 neighbor connections on game load
	VirtualProtect((LPVOID)0x434BEA, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x434BEA, Hook_LoadNeighborConnections1500);
	*(BYTE*)0x434BEF = 0x90;

	// Install hooks for the SC2X save format
	InstallSaveHooks();

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
	UINT uCheatPatch[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	memcpy_s((LPVOID)0x4E65C8, 10, "mrsoleary", 10);
	memcpy_s((LPVOID)0x4E6490, sizeof(uCheatPatch), uCheatPatch, sizeof(uCheatPatch));

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

	// Window title calendar and modding hooks for SimulationProcessTick
	VirtualProtect((LPVOID)0x40D67D, 10, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x40D67D, Hook_40D67D);
	memset((LPVOID)0x40D682, 0x90, 5);
	VirtualProtect((LPVOID)0x4135D2, 9, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4135D2, Hook_SimulationProcessTickDaySwitch);
	memset((LPVOID)0x4135D7, 0x90, 4);
	VirtualProtect((LPVOID)0x413ABF, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x413ABF, Hook_SimulationProcessTickDaySwitch_After);

	// Hook SimulationStartDisaster
	VirtualProtect((LPVOID)0x402527, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402527, Hook_SimulationStartDisaster);

	// Hook SimulationPrepareDisaster
	VirtualProtect((LPVOID)0x40174E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40174E, Hook_SimulationPrepareDisaster);

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

	hDebugMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(223));
	if (hDebugMenu) {
		AFX_MSGMAP_ENTRY afxMessageMapEntry[5];
		HMENU hDebugPopup;
		MENUITEMINFO miiDebugPopup;
		miiDebugPopup.cbSize = sizeof(MENUITEMINFO);
		miiDebugPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hDebugMenu, 0, TRUE, &miiDebugPopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		hDebugPopup = miiDebugPopup.hSubMenu;

		// Insert in reverse order.
		// Separator between the disasters and internal debugging functions.
		if (!InsertMenu(hDebugPopup, 11, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		// Separator between grants and disasters
		if (!InsertMenu(hDebugPopup, 4, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		// Separator between the version option and grants
		if (!InsertMenu(hDebugPopup, 1, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		
		// Insert in reverse order.
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_MISSILESILOS, "Propose Missile Silos") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_NAVALYARD, "Propose Naval Yard") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_ARMYBASE, "Propose Army Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #6 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_AIRFORCE, "Propose Air Force Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #7 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_DECLINED, "Stop Military Spawning") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Debug InsertMenuA #8 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
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

skipdebugmenu:

	// Hook for the game area leftmousebuttondown call.
	VirtualProtect((LPVOID)0x401523, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401523, Hook_CSimcityView_WM_LBUTTONDOWN);

	// Hook for the game area mouse movement call.
	VirtualProtect((LPVOID)0x4016EA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4016EA, Hook_CSimcityView_WM_MOUSEMOVE);

	// Hook for the MapToolMenuAction call.
	VirtualProtect((LPVOID)0x402B44, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B44, Hook_MapToolMenuAction);

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
	UpdateMiscHooks();
}

// The difference between InstallMiscHooks and UpdateMiscHooks is that UpdateMiscHooks can be run
// again at runtime because it can patch back in original game code. It's used for small stuff.
void UpdateMiscHooks(void) {
	// Music in background
	VirtualProtect((LPVOID)0x40BFDA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	if (bSettingsMusicInBackground)
		memset((LPVOID)0x40BFDA, 0x90, 5);
	else {
		BYTE bOriginalCode[5] = { 0xE8, 0xFD, 0x50, 0xFF, 0xFF };
		memcpy_s((LPVOID)0x40BFDA, 5, bOriginalCode, 5);
	}
}

