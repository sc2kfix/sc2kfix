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

extern "C" BOOL __stdcall Hook_DestroyMenu(HMENU hMenu) {
	ConsoleLog(LOG_DEBUG, "MENU: 0x%06X -> DestroyMenu(0x%06X)\n", hMenu);
	return DestroyMenu(hMenu);
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

extern "C" int __cdecl Hook_SimulationGrowSpecificZone(__int16 iX, __int16 iY, __int16 iTileID, __int16 iZoneType) {
#if 1
	// Variable names subject to change
	// during the demystification process.
	__int16 x, y;
	__int16 iCurrX, iCurrY;
	__int16 iBuildingCountOne;
	__int16 iBuildingOne;
	__int16 iMoveX, iMoveY;
	__int16 iOppositeFacing;
	__int16 iBuildingCountTwo;
	__int16 iBuildingTwo;
	__int16 iTileSouthFacing;
	map_XBIT_t *mXBIT;
	BYTE mXBBits;

	// It looks like this is to do with the given tiles being powered. Though this check
	// is only hit on non-Military zones.
	int(__cdecl *H_IsZonedTilePowered)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x401B40;
	// This could be a check to make sure certain building types aren't overwritten during zone growth.
	__int16(__cdecl *H_402603)(__int16, __int16) = (__int16(__cdecl *)(__int16, __int16))0x402603;

	x = iX;
	y = iY;
	if (iZoneType != ZONE_MILITARY) {
		if (!H_IsZonedTilePowered(iX, iY))
			return 0;
		//ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> SimulationGrowSpecificZone(%d, %d, %d, %d) (Non-MZ - IsPowered)\n", _ReturnAddress(), iX, iY, iTileID, iZoneType);
	}
	//ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> SimulationGrowSpecificZone(%d, %d, %d, %d)\n", _ReturnAddress(), iX, iY, iTileID, iZoneType);
	switch (iTileID) {
		case TILE_INFRASTRUCTURE_RUNWAY:
			iMoveX = 0;
			iMoveY = 0;
			if ((dwTileCount[TILE_INFRASTRUCTURE_RUNWAY] & 1) == 0) {
				ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> SimulationGrowSpecificZone(%d, %d, %d, %d) - dwTileCount[TILE_INTRASTRUCTURE_RUNWAY] = %u\n", _ReturnAddress(), iX, iY, iTileID, iZoneType, dwTileCount[TILE_INFRASTRUCTURE_RUNWAY]);
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
			iBuildingCountOne = 0;
			while (iCurrX < 0x80 && iCurrY < 0x80) {
				if (dwMapXZON[iCurrX]->b[iCurrY].iZoneType != iZoneType)
					return 0;
				// Positional marker - see whether a given if/check for military-zoned
				// roads will need to be placed here.
				iBuildingOne = dwMapXBLD[iCurrX]->iTileID[iCurrY];
				if (iBuildingOne == TILE_INFRASTRUCTURE_RUNWAY || iBuildingOne == TILE_INFRASTRUCTURE_RUNWAYCROSS)
					--iBuildingCountOne;
				iCurrX += iMoveY;
				++iBuildingCountOne;
				iCurrY += iMoveX;
				if (iBuildingCountOne >= 5) {
					if (!iMoveY) 
						goto SKIPFIRSTROTATIONCHECK;
					if ((wViewRotation & 1) != 0) {
						if (!iMoveY) {
SKIPFIRSTROTATIONCHECK:
							if ((wViewRotation & 1) != 0)
								goto SKIPSECONDROTATIONCHECK;
						}
						iOppositeFacing = 0;
					}
					else {
SKIPSECONDROTATIONCHECK:
						iOppositeFacing = 1;
					}
					iBuildingCountTwo = 0;
					while (2) {
						iBuildingTwo = dwMapXBLD[x]->iTileID[y];
						if (iBuildingTwo == TILE_INFRASTRUCTURE_RUNWAY || iBuildingTwo == TILE_INFRASTRUCTURE_RUNWAYCROSS) {
							--iBuildingCountTwo;
							if (iBuildingTwo == TILE_INFRASTRUCTURE_RUNWAY) {
								iTileSouthFacing = x < 0x80 &&
									y < 0x80 &&
									dwMapXBIT[x]->b[y].iRotated == VIEWROTATION_SOUTH;
								if (iTileSouthFacing != iOppositeFacing) {
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
							if (dwMapXBLD[x]->iTileID[y] >= TILE_SMALLPARK)
								H_402603(x, y);
							Game_PlaceTileWithMilitaryCheck(x, y, 221);
							if (x < 0x80 && y < 0x80)
								*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
							if (iZoneType != ZONE_MILITARY && x < 0x80 && y < 0x80)
								*(BYTE *)&dwMapXBIT[x]->b[y] |= 0xC0u;
							if (iOppositeFacing && x < 0x80 && y < 0x80) {
								mXBIT = dwMapXBIT[x];
								mXBBits = (*(BYTE *)&mXBIT->b[y] | 2);
								goto RUNWAY_GOBACK;
							}
						}
RUNWAY_GETOUT:
						x += iMoveY;
						y += iMoveX;
						if (++iBuildingCountTwo >= 5)
							return 1;
						continue;
					}
				}
			}
			return 0;
		case TILE_MILITARY_MISSILESILO:
			PlaceMissileSilo(x, y);
			return 1;
		default:
			return 1;
	}
#else
	int(__cdecl *H_SimulationGrowSpecificZone)(__int16, __int16, __int16, __int16) = (int(__cdecl *)(__int16, __int16, __int16, __int16))0x4382B0;

	int ret = H_SimulationGrowSpecificZone(iX, iY, iTileID, iZoneType);

	return ret;
#endif
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
					if ((__int16)(y + iArea) > -1 && iSection[1] < 0x80) {
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

	pThis = (DWORD *)Game_PointerToCSimcityView(pCWinAppThis);	// TODO: is this necessary or can we just dereference pCSimcityView?
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
	*(DWORD*)(0x4EFBF0) = (DWORD)Hook_DestroyMenu;
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

	// Hook into the SimulationGrowSpecificZone function
	VirtualProtect((LPVOID)0x4026B2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026B2, Hook_SimulationGrowSpecificZone);

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
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_NAVALYARD, "Propose Naval Yard") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_ARMYBASE, "Propose Army Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #6 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_AIRFORCE, "Propose Air Force Base") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #7 failed, error = 0x%08X.\n", GetLastError());
			goto skipdebugmenu;
		}
		if (!InsertMenu(hDebugPopup, 5, MF_BYPOSITION|MF_STRING, IDM_DEBUG_MILITARY_DECLINED, "Stop Military Spawning") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #8 failed, error = 0x%08X.\n", GetLastError());
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

