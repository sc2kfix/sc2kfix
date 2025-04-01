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

// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 3 && hGameMenu)
		return hGameMenu;
	return LoadMenuA(hInstance, lpMenuName);
}

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

	if (bSkipIntro || bSettingsAlwaysSkipIntro)
		if (strrchr(lpFileName, '\\'))
			if (!strcmp(strrchr(lpFileName, '\\'), "\\INTROA.SMK") || !strcmp(strrchr(lpFileName, '\\'), "\\INTROB.SMK"))
				return NULL;

	char buf[MAX_PATH + 1];

	memset(buf, 0, sizeof(buf));

	return SMKOpenProc(AdjustSource(buf, lpFileName), uFlags, iExBuf);
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
		break;
	}

	return lpNewCityAfxProc(hwndDlg, message, wParam, lParam);
}

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

	for (auto hook : stHooks_Hook_SimulationProcessTickDaySwitch_Before) {
		void (*fnHook)(void) = (void(*)(void))hook.pFunction;
		fnHook();
	}

	Game_RefreshTitleBar(pCDocumentMainWindow);

	if (bSettingsFrequentCityRefresh) {
		POINT pt;
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
		GetCursorPos(&pt);
		if (wCityMode && wCurrentCityToolGroup != TOOL_GROUP_CENTERINGTOOL && Game_GetTileCoordsFromScreenCoords(pt.x, pt.y) < 0x8000) {
			Game_DrawSquareHighlight(*(WORD*)0x4CDB68, *(WORD*)0x4CDB70, *(WORD*)0x4CDB6C, *(WORD*)0x4CDB74);
			*(WORD*)0x4EA7F0 = 1;		// TODO - figure out exactly what this should be called
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
	wTileCoords = Game_GetTileCoordsFromScreenCoords(pt.x, pt.y);
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
					P_LOWORD(ret) = Game_CSimCityView_OnVScroll(pThis, SB_THUMBTRACK, pt.y, iYVar);
				else {
					if (*(DWORD *)(pThis + 140) >= pt.y)
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
					P_LOWORD(ret) = Game_GetTileCoordsFromScreenCoords(pt.x, pt.y);
					wCurrentTileCoordinates = ret;
					if ((__int16)ret >= 0) {
						wTileCoordinateX = (uint8_t)ret;
						wPreviousTileCoordinateX = (uint8_t)ret;
						wTileCoordinateY = wCurrentTileCoordinates >> 8;
						wPreviousTileCoordinateY = wCurrentTileCoordinates >> 8;
						wGameScreenAreaX = pt.x;
						wGameScreenAreaY = pt.y;
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

	P_LOWORD(iTileCoords) = pt.x;
	iThisSomething = *(DWORD *)(pThis + 252);
	*(struct tagPOINT *)(pThis + 260) = pt; // Placement position.
	if (iThisSomething) {
		P_LOWORD(iTileCoords) = Game_GetTileCoordsFromScreenCoords(pt.x, pt.y);
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
					wGameScreenAreaX = pt.x;
					wGameScreenAreaY = pt.y;
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
		P_LOWORD(ret) = Game_GetTileCoordsFromScreenCoords(pt.x, pt.y);
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
				P_LOWORD(ret) = Game_MapToolStretchTerrain(iTileTargetX, iTileTargetY, pt.y);
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
	ConsoleLog(LOG_DEBUG, "FUCK");
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

	// Window title calendar
	VirtualProtect((LPVOID)0x40D67D, 10, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x40D67D, Hook_40D67D);
	memset((LPVOID)0x40D682, 0x90, 5);
	VirtualProtect((LPVOID)0x4135D2, 9, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4135D2, Hook_SimulationProcessTickDaySwitch);
	memset((LPVOID)0x4135D7, 0x90, 4);

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
			ConsoleLog(LOG_DEBUG, "MISC: GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		hOptionsPopup = miiOptionsPopup.hSubMenu;
		if (!AppendMenu(hOptionsPopup, MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		if (!AppendMenu(hOptionsPopup, MF_STRING, 40000, "sc2kfix &Settings...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		if (!AppendMenu(hOptionsPopup, MF_STRING, 40001, "Mod &Configuration...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}

		afxMessageMapEntry[0] = {
			WM_COMMAND,
			0,
			40000,
			40000,
			0x0A,
			ShowSettingsDialog,
		};

		afxMessageMapEntry[1] = {
			WM_COMMAND,
			0,
			40001,
			40001,
			0x0A,
			ShowModSettingsDialog
		};

		VirtualProtect((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy_s((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));

		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated game menu.\n");
	}

skipmenu:

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

