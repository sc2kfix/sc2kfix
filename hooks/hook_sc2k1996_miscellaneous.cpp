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
#include <windowsx.h>
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
//#define MISCHOOK_DEBUG_MILITARY 2 // can be re-used
#define MISCHOOK_DEBUG_MENU 4
//#define MISCHOOK_DEBUG_SAVES 8 // can be re-used
#define MISCHOOK_DEBUG_WINDOW 16
#define MISCHOOK_DEBUG_DISASTERS 32
//#define MISCHOOK_DEBUG_MOVIES 64 // can be re-used
//#define MISCHOOK_DEBUG_SPRITE 128 // can be re-used
#define MISCHOOK_DEBUG_CHEAT 256

#define MISCHOOK_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

DLGPROC lpNewCityAfxProc = NULL;
char szTempMayorName[24] = { 0 };

DLGPROC lpMainDialogAfxProc = NULL;
HWND hwndMainDialog_SC2K1996 = NULL;
BOOL bMainDialogUpdateState = FALSE;

static BOOL bOverrideTickPlacementHighlight = FALSE;

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

extern "C" BOOL __stdcall Hook_ShowWindow(HWND hWnd, int nCmdShow) {
	if (mischook_debug & MISCHOOK_DEBUG_WINDOW)
		ConsoleLog(LOG_DEBUG, "WND:  0x%08X -> ShowWindow(0x%08X, %i)\n", _ReturnAddress(), hWnd, nCmdShow);

	// Workaround for the game window not showing if started by a launcher process
	if (nCmdShow == 11 && (DWORD)_ReturnAddress() == 0x40586C)
		return ShowWindow(hWnd, SW_MAXIMIZE);

	return ShowWindow(hWnd, nCmdShow);
}

int L_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	int ret;

	ToggleFloatingStatusDialog(FALSE);
	ret = MessageBoxA(hWnd, lpText, lpCaption, uType);
	ToggleFloatingStatusDialog(TRUE);

	return ret;
}

int __stdcall Hook_AfxMessageBoxStr(LPCTSTR lpszPrompt, UINT nType, UINT nIDHelp) {
	int ret;

	ToggleFloatingStatusDialog(FALSE);
	ret = GameMain_WinApp_DoMessageBox(game_AfxCoreState.m_pCurrentWinApp, lpszPrompt, nType, nIDHelp);
	ToggleFloatingStatusDialog(TRUE);

	return ret;
}

int __stdcall Hook_AfxMessageBoxID(UINT nIDPrompt, UINT nType, UINT nIDHelp) {
	CMFC3XString cStr;
	UINT nID;
	int ret;

	GameMain_String_Cons(&cStr);
	GameMain_String_LoadStringA(&cStr, nIDPrompt);
	nID = nIDHelp;
	if (nIDHelp == -1)
		nID = nIDPrompt;

	ToggleFloatingStatusDialog(FALSE);
	ret = GameMain_WinApp_DoMessageBox(game_AfxCoreState.m_pCurrentWinApp, cStr.m_pchData, nType, nIDHelp);
	ToggleFloatingStatusDialog(TRUE);

	GameMain_String_Dest(&cStr);
	return ret;
}

extern "C" int __stdcall Hook_FileDialog_DoModal() {
	CMFC3XFileDialog *pThis;

	__asm mov [pThis], ecx

	HWND hWndOwner;
	bool bIsReserved;
	int iRet;
	int nPathLen, nFileLen, nNewLen;
	char szPath[MAX_PATH + 1];
	OPENFILENAMEA* pOfn;

	memset(szPath, 0, sizeof(szPath));

	ToggleFloatingStatusDialog(FALSE);

	hWndOwner = GameMain_Dialog_PreModal(pThis);
	bIsReserved = pThis->m_ofn.pvReserved == 0;
	pThis->m_ofn.hwndOwner = hWndOwner;
	pOfn = &pThis->m_ofn;

	if (bIsReserved)
		iRet = GameMain_GetSaveFileNameA(pOfn);
	else
		iRet = GameMain_GetLoadFileNameA(pOfn);
	GameMain_Dialog_PostModal(pThis);
	if (!iRet)
		iRet = IDCANCEL;

	if (iRet != IDCANCEL) {
		nPathLen = strlen(pOfn->lpstrFile);
		nFileLen = strlen(pOfn->lpstrFileTitle);
		if (nPathLen > 0 && nFileLen > 0) {
			nNewLen = nPathLen - nFileLen;
			if (nNewLen > 0) {
				strncpy_s(szPath, sizeof(szPath)-1, pOfn->lpstrFile, nNewLen);
				if (L_IsPathValid(szPath)) {
					if ((DWORD)_ReturnAddress() == 0x42EB82 ||
						(DWORD)_ReturnAddress() == 0x42FDCE) // From 'LoadCity' or 'SaveCityAs'
						strcpy_s(szLastStoredCityPath, sizeof(szLastStoredCityPath) - 1, szPath);
					else if ((DWORD)_ReturnAddress() == 0x42F312) // From 'LoadTileSet'
						strcpy_s(szLastStoredTileSetPath, sizeof(szLastStoredTileSetPath) - 1, szPath);
				}
			}
		}
	}

	ToggleFloatingStatusDialog(TRUE);

	return iRet;
}

extern "C" void __stdcall Hook_SimcityApp_OnQuit(void) {
	CSimcityAppPrimary *pThis;

	__asm mov [pThis], ecx

	// pThis[63] = dwSCAOnQuitSuspendSim
	// pThis[64] = dwSCAMainFrameDestroyVar
	// pThis[206] = dwSCASysCmdOnQuitVar
	//
	// While 'dwSCAMainFrameDestroyVar' is set to 1 various simulation and update
	// aspects are suspended. Originally when "Cancel" was clicked in order to avoid
	// quitting it wouldn't unset the var and consequently the game simulation would
	// remain suspended.

	int iReqRet;

	pThis->dwSCAMainFrameDestroyVar = 1;
	pThis->dwSCAOnQuitSuspendSim = 0;
	iReqRet = Game_SimcityApp_ExitRequester(pThis, pThis->dwSCASysCmdOnQuitVar);
	if (iReqRet != IDCANCEL) {
		if (iReqRet == IDYES)
			Game_SimcityApp_SaveCity(pThis);
		Game_MainFrame_ToggleToolBars((CMainFrame *)pThis->m_pMainWnd, 0);
		GameMain_WinApp_OnAppExit(pThis);
		return;
	}
	pThis->dwSCAMainFrameDestroyVar = 0;
	pThis->dwSCAOnQuitSuspendSim = 0;
}

// Hook CCmdUI::Enable so we can programmatically enable and disable menu items reliably
extern "C" void __stdcall Hook_CmdUI_Enable(BOOL bOn) {
	CMFC3XCmdUI *pThis;
	__asm mov [pThis], ecx

	HWND hWndParent;
	CMFC3XWnd *pWndParent;
	HWND hNextDlgTabItem;
	CMFC3XWnd *pNextDlgTabItem;
	HWND hWndFocus;

	if (pThis->m_pMenu != NULL) {
		if (pThis->m_pSubMenu != NULL)
			return;

		EnableMenuItem(pThis->m_pMenu->m_hMenu, pThis->m_nIndex, MF_BYPOSITION |
			(bOn ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else {
		if (!bOn && (GetFocus() == pThis->m_pOther->m_hWnd)) {
			hWndParent = GetParent(pThis->m_pOther->m_hWnd);
			pWndParent = GameMain_Wnd_FromHandle(hWndParent);
			hNextDlgTabItem = GetNextDlgTabItem(pWndParent->m_hWnd, pThis->m_pOther->m_hWnd, 0);
			pNextDlgTabItem = GameMain_Wnd_FromHandle(hNextDlgTabItem);
			hWndFocus = SetFocus(pNextDlgTabItem->m_hWnd);
			GameMain_Wnd_FromHandle(hWndFocus);
		}
		EnableWindow(pThis->m_pOther->m_hWnd, bOn);
	}
	pThis->m_bEnableChanged = TRUE;

	// This section has been added to account for menu items that aren't handled
	// natively (yet).

	// Ensure that the new 'Reload Default Tile Set' item is always enabled.
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_GAME_FILE_RELOADDEFAULTTILESET, MF_BYCOMMAND | MF_ENABLED);

	// Ensure the main config menu options are always enabled
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_GAME_OPTIONS_SC2KFIXSETTINGS, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_GAME_OPTIONS_MODCONFIG, MF_BYCOMMAND | MF_ENABLED);

	// Only enable the Scenario Goals option if we need it
	if (bInScenario)
		EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_GAME_WINDOWS_SCENARIOGOALS, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_GAME_WINDOWS_SCENARIOGOALS, MF_BYCOMMAND | MF_GRAYED);

	// Ensure that the debug military options are always enabled.
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_DEBUG_MILITARY_DECLINED, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_DEBUG_MILITARY_AIRFORCE, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_DEBUG_MILITARY_ARMYBASE, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_DEBUG_MILITARY_NAVALYARD, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(GameGetRootWindowHandle()), IDM_DEBUG_MILITARY_MISSILESILOS, MF_BYCOMMAND | MF_ENABLED);
}

// Function prototype: HOOKCB void Hook_GameDoIdleUpkeep_Before(void)
// Ignored if bHookStopProcessing == TRUE.
// SPECIAL NOTE: Ignoring this hook on callback results in the game effectively hanging. You have
//   been warned!
std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_Before;

// Function prototype: HOOKCB void Hook_GameDoIdleUpkeep_After(void)
// Ignored if bHookStopProcessing == TRUE.
std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_After;

extern "C" void __stdcall Hook_GameDoIdleUpkeep(void) {
	DWORD *pThis;
	__asm mov [pThis], ecx
	for (const auto& hook : stHooks_Hook_GameDoIdleUpkeep_Before) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(void*) = (void(*)(void*))hook.pFunction;
			fnHook(pThis);
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
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(void*) = (void(*)(void*))hook.pFunction;
			fnHook(pThis);
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

BAIL:
	return;
}

// Fix the missing "Maxis Presents" slide
void __declspec(naked) Hook_4062AD(void) {
	__asm {
		mov dword ptr [ecx+0x14C], 1
		mov dword ptr [edi], 3
		push 0x4062BD
		retn
	}
}

// Function prototype: HOOKCB void Hook_OnNewCity_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: I cannot for the life of me remember why I added this. It will almost certainly
//   be replaced within the next three months (comment added 2025-08-15).
std::vector<hook_function_t> stHooks_Hook_OnNewCity_Before;

static BOOL CALLBACK Hook_NewCityDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// Difficulty selection tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 109),
			"Start a game on Easy difficulty.\n"
			"Modifiers:\n"
			" - $20,000 starting cash\n"
			" - Slightly increased industrial demand\n"
			" - Four months before disasters can occur");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 1001),
			"Start a game on Easy difficulty.\n"
			"Modifiers:\n"
			" - $20,000 starting cash\n"
			" - Slightly increased industrial demand\n"
			" - Four months before disasters can occur");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 110),
			"Start a game on Medium difficulty.\n"
			"Modifiers:\n"
			" - $10,000 starting cash\n"
			" - Baseline industrial demand\n"
			" - Two months before disasters can occur");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 1002),
			"Start a game on Medium difficulty.\n"
			"Modifiers:\n"
			" - $10,000 starting cash\n"
			" - Baseline industrial demand\n"
			" - Two months before disasters can occur");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 111),
			"Start a game on Hard difficulty.\n"
			"Modifiers:\n"
			" - $10,000 bond at 3% APR\n"
			" - Slightly decreased industrial demand\n"
			" - One month before disasters can occur");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 1003),
			"Start a game on Hard difficulty.\n"
			"Modifiers:\n"
			" - $10,000 bond at 3% APR\n"
			" - Slightly decreased industrial demand\n"
			" - One month before disasters can occur");

		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 1010),
			"Hover over a date to see the difference between starting years.");

		// Year selection tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 104),
			"Start the game in 1900.\n"
			"Modifiers:\n"
			" - No forced unlocks.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 105),
			"Start the game in 1950.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment plants unlocked.\n"
			" - 50% chance of natural gas power plants being unlocked.\n"
			" - 5% chance of nuclear power plants being unlocked.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 106),
			"Start the game in 2000.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment and desalination plants unlocked.\n"
			" - Natural gas, nuclear, wind, and solar power plants unlocked.\n"
			" - 50% chance of Plymouth arcologies being unlocked.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, 107),
			"Start the game in 2050.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment and desalination plants unlocked.\n"
			" - Natural gas, nuclear, wind, solar, and micorwave power plants unlocked.\n"
			" - 5% chance of fusion power plants being unlocked.\n"
			" - Plymouth arcologies unlocked.\n"
			" - 50% chance of Forest arcologies being unlocked.");

		// Set the default mayor name.
		SetDlgItemText(hwndDlg, 150, szSettingsMayorName);
		break;
	case WM_DESTROY:
		// XXX - there's probably a better window message to use here.
		memset(szTempMayorName, 0, 24);
		if (!GetDlgItemText(hwndDlg, 150, szTempMayorName, 24))
			strcpy_s(szTempMayorName, 24, szSettingsMayorName);

		SetXLABEntry(0, szTempMayorName);

		// XXX - this should probably be moved to a separate proper hook into the game itself
		for (const auto& hook : stHooks_Hook_OnNewCity_Before) {
			bHookStopProcessing = FALSE;
			if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
				void (*fnHook)(void) = (void(*)(void))hook.pFunction;
				fnHook();
			}
		}

		break;
	}

	return lpNewCityAfxProc(hwndDlg, message, wParam, lParam);
}

static BOOL CALLBACK Hook_MainDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	std::string strInfo;

	switch (message) {
	case WM_INITDIALOG:
		hwndMainDialog_SC2K1996 = hwndDlg;

		if (bUpdateAvailable) {
			strInfo = UPDATE_STRING;
			bMainDialogUpdateState = TRUE;
		}
		else {
			// Set the version string.
			strInfo = "Running\nsc2kfix\nVersion\n";
			strInfo += szSC2KFixVersion;
			strInfo += " (";
			strInfo += szSC2KFixReleaseTag;
			strInfo += ")";
		}

		SetDlgItemText(hwndDlg, IDC_STATIC_UPDATENOTICE, strInfo.c_str());
		break;
	case WM_SC2KFIX_UPDATE:
		if (!bMainDialogUpdateState) {
			if (lParam == 1) {
				strInfo = UPDATE_STRING;
				bMainDialogUpdateState = TRUE;

				SetDlgItemText(hwndDlg, IDC_STATIC_UPDATENOTICE, strInfo.c_str());
			}
		}
		break;
	case WM_DESTROY:
		hwndMainDialog_SC2K1996 = NULL;
		break;
	}
	return lpMainDialogAfxProc(hwndDlg, message, wParam, lParam);
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
		lpMainDialogAfxProc = lpDialogFunc;
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, Hook_MainDialogProc, dwInitParam);
	default:
		return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}
}
#pragma warning(default : 6387)

// Game area Middle Mouse Button Down handler.
static void DoOnMButtonDown(CSimcityAppPrimary *pSCApp, UINT nFlags, POINT pt) {
	__int16 wTileCoords = 0;
	BYTE bTileX = 0, bTileY = 0;
	wTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
	bTileX = LOBYTE(wTileCoords);
	bTileY = HIBYTE(wTileCoords);

	if (wTileCoords & 0x8000)
		return;
	else {
		if (nFlags & MK_CONTROL)
			;
		else if (nFlags & MK_SHIFT)
			;
		else if (GetAsyncKeyState(VK_MENU) < 0) {
			// useful for tests
		} else {
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
			Game_CenterOnTileCoords(bTileX, bTileY);
		}
	}
}

extern "C" LRESULT __stdcall Hook_DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (Msg == WM_MBUTTONDOWN) {
		if (pSCView && hWnd == pSCView->m_hWnd) {
			POINT pt;

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			DoOnMButtonDown(pSCApp, (UINT)wParam, pt);
			return TRUE;
		}
	}
	return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

extern "C" void __stdcall Hook_StartCleanGame(void) {
	BOOL bMapEditor, bNewGame;

	bMapEditor = ((DWORD)_ReturnAddress() == 0x42DF13);
	bNewGame = ((DWORD)_ReturnAddress() == 0x42E482);
	if (bMapEditor || bNewGame) {
		CSimcityAppPrimary *pSCApp;
		CSimcityView *pThis;

		pSCApp = &pCSimcityAppThis;
		pThis = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);

		if (((__int16)wCityMode < 0 && bNewGame) || bMapEditor) {
			if (wViewRotation != VIEWROTATION_NORTH) {
				do
					Game_SimcityView_RotateAntiClockwise(pThis);
				while (wViewRotation != VIEWROTATION_NORTH);
				UpdateWindow(pThis->m_hWnd); // This would be pThis->m_hWnd if the structs were present.
			}
		}
	}

	GameMain_StartCleanGame();
}

extern "C" void __stdcall Hook_SimcityDoc_UpdateDocumentTitle() {
	CSimcityDoc *pThis;

	__asm mov [pThis], ecx

	CMFC3XString cStr;
	CSimcityAppPrimary *pSCApp;
	int iCityDayMon;
	int iCityMonth;
	int iCityYear;
	const char *pCurrStr;
	CSimString *pFundStr;

	GameMain_String_Cons(&cStr);

	pSCApp = &pCSimcityAppThis;
	if (!pSCApp->dwSCAMainFrameDestroyVar) {
		if (!wCityMode) {
			GameMain_String_LoadStringA(&cStr, 0x19D); // "Editing Terrain..."
			goto GOFORWARD;
		}
		if (!pszCityName.m_nDataLength)
			goto GETOUT;
		iCityDayMon = dwCityDays % 25 + 1;
		iCityMonth = dwCityDays / 25 % 12;
		iCityYear = wCityStartYear + dwCityDays / 300;
		if (GameMain_IsIconic(GameGetRootWindowHandle())) {
			if (dwDisasterActive) {
				if (wCurrentDisasterID <= DISASTER_HURRICANE)
					GameMain_String_LoadStringA(&cStr, dwDisasterStringIndex[wCurrentDisasterID]);
				else
					GameMain_String_Empty(&cStr);
			}
			else
				GameMain_String_Format(&cStr, "%s%s%d", pszCityName.m_pchData, gameStrHyphen, iCityYear);
			goto GOFORWARD;
		}
		GameMain_String_Empty(&cStr);
		if (strcmp(pSCApp->dwSCACStringLang.m_pchData, gameLangFrench) != 0) {
			if (strcmp(pSCApp->dwSCACStringLang.m_pchData, gameLangGerman) != 0)
				pCurrStr = gameCurrDollar;
			else
				pCurrStr = gameCurrDM;
		}
		else
			pCurrStr = gameCurrFF;
		pFundStr = new CSimString();
		if (pFundStr)
			pFundStr = Game_SimString_SetString(pFundStr, pCurrStr, 20, (double)dwCityFunds);
		else
			goto GETOUT;
		Game_SimString_TruncateAtSpace(pFundStr);
		if (bSettingsTitleCalendar)
			GameMain_String_Format(&cStr, "%s %d %4d <%s> %s", pSCApp->dwSCApCStringLongMonths[iCityMonth].m_pchData, iCityDayMon, iCityYear, pszCityName.m_pchData, pFundStr->pStr);
		else
			GameMain_String_Format(&cStr, "%s %4d <%s> %s", pSCApp->dwSCApCStringShortMonths[iCityMonth].m_pchData, iCityYear, pszCityName.m_pchData, pFundStr->pStr);
		if (pFundStr) {
			Game_SimString_Dest(pFundStr);
			operator delete(pFundStr);
		}
GOFORWARD:
		GameMain_Document_UpdateAllViews(pThis, 0, 1, (CMFC3XObject *)&cStr);
	}
GETOUT:
	GameMain_String_Dest(&cStr);
}

// Local TileHightlightUpdate function.
// This is for attempts at mitigating some of
// the oddities that come with either:
// 1) African Swallow mode during non-granular updates (batch).
// 2) Granular updates on all speed levels. (more so for African Swallow and Cheetah)
static void L_TileHighlightUpdate(CSimcityView *pThis) {
	BYTE *vBits;
	LONG bottom;
	LONG x;
	__int16 y;

	if (wTileHighlightActive) {
		vBits = Game_Graphics_LockDIBBits(pThis->dwSCVCGraphics);
		if (vBits || Game_SimcityView_CheckOrLoadGraphic(pThis)) {
			x = Game_Graphics_Width(pThis->dwSCVCGraphics);
			y = Game_Graphics_Height(pThis->dwSCVCGraphics);
			if (!bOverrideTickPlacementHighlight) {
				Game_BeginProcessObjects(pThis, vBits, x, y, &pThis->dwSCVRECTOne);
				Game_SimcityView_DrawSquareHighlight(pThis, wHighlightedTileX1, wHighlightedTileY1, wHighlightedTileX2, wHighlightedTileY2);
				Game_FinishProcessObjects();
			}
			Game_Graphics_UnlockDIBBits(pThis->dwSCVCGraphics);
			bottom = ++rcDst.bottom;
			if (pThis->dwSCVIsZoomed) {
				rcDst.bottom = bottom + 2;
				++rcDst.right;
			}
			// As it turns out this if case is necessary here.. otherwise it results in breakage when
			// it comes to the pollution clouds (entire view window update rather than just the
			// "dirty" area).
			// ^ Unclear - the pollution case still expresses itself even with this case implemented.
			// Tests performed in the 'Interactive Demo' (of which don't have any of these hooks) have
			// also resulted in similar intermittent encounters.
			if (pThis == (CSimcityView *)&pSomeWnd)
				Game_SimcityView_MainWindowUpdate(pThis, 0, 1);
			else
				Game_SimcityView_MainWindowUpdate(pThis, &rcDst, 1);
			if (bOverrideTickPlacementHighlight)
				wTileHighlightActive = 0;
		}
	}
}

static void UpdateCityDateAndSeason(BOOL bIncrement) {
	if (bIncrement)
		++dwCityDays;
	wCityCurrentMonth = (dwCityDays / 25) % 12;
	wCityCurrentSeason = (wCityCurrentMonth + 1) % 12 / 3;
	wCityElapsedYears = (dwCityDays / 300);
}

// Function prototype: HOOKCB void Hook_SimCalendarAdvance_Before(void)
// Called before the vanilla SimCalendar day simulation. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_Before;

// Function prototype: HOOKCB BOOL Hook_ScenarioSuccessCheck(void)
// Cannot be ignored.
// Return value: TRUE if the mod's scenario requirements have been met, FALSE if not.
std::vector<hook_function_t> stHooks_Hook_ScenarioSuccessCheck;

// Function prototype: HOOKCB void Hook_SimCalendarAdvance_After(void)
// Called after the vanilla SimCalendar day simulation. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_After;

extern "C" void __stdcall Hook_Engine_SimulationProcessTick() {
	int i;
	DWORD dwMonDay;
	CNewspaperDialog newsDialog;
	__int16 iStep, iSubStep;
	DWORD dwCityProgressionRequirement;
	BYTE iPaperVal;
	BOOL bScenarioSuccess;
	BOOL bDoTileHighlightUpdate;
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	UpdateCityDateAndSeason(TRUE);
	dwMonDay = (dwCityDays % 25);
	if (pSCApp->dwSCAGameAutoSave > 0 &&
		!((dwCityDays / 300) % pSCApp->dwSCAGameAutoSave) &&
		!wCityCurrentMonth &&
		!dwMonDay) {
		Game_SimcityApp_CallAutoSave(pSCApp);
	}

	if (bSettingsFrequentCityRefresh) {
		Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
		GameMain_Document_UpdateAllViews(pCSimcityDoc, NULL, 2, NULL);
	}

	// Call mods for daily processing tasks - before update
	// XXX - should mods be able to entirely override SimCalendar days? Perhaps this is more a
	// theological discussion to be held...
	for (const auto& hook : stHooks_Hook_SimCalendarAdvance_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			BOOL(*fnHook)() = (BOOL(*)())hook.pFunction;
			fnHook();
		}
	}

	// Advance the simulation for the current SimCalendar day
	switch (dwMonDay) {
		case 0:
			if (!bSettingsFrequentCityRefresh)
				Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
			if (bYearEndFlag)
				Game_SimulationPrepareBudgetDialog(0);
			Game_UpdateBudgetInformation();
			if (bNewspaperSubscription) {
				if (wCityCurrentMonth == 3 || wCityCurrentMonth == 7) {
					Game_NewspaperDialog_Construct(&newsDialog);
					newsDialog.dwNDPaperChoice = wNewspaperChoice; // CNewspaperDialog -> CGameDialog -> CDialog; struct position 39 - paperchoice dword var.
					Game_GameDialog_DoModal(&newsDialog);
					Game_NewspaperDialog_Destruct(&newsDialog);
				}
			}
			UpdateCityDateAndSeason(FALSE);
			for (i = 0; i < ZONEPOP_COUNT; ++i)
				pZonePops[i] = 0;
			break;
		case 1:
			Game_SimulationUpdatePowerConsumption();
			break;
		case 2:
			Game_SimulationPollutionTerrainAndLandValueScan();
			break;
		// Switch cases 3-18 have been moved to 'default' as
		// if (dwMonDay >= 3 && dwMonDay <= 18).
		case 19:
			Game_SimulationUpdateMonthlyTrafficData();
			break;
		case 20:
			Game_SimulationUpdateWaterConsumption();
			break;
		case 21:
			Game_SimulationRCIDemandUpdates();
			Game_SimulationEQ_LE_Processing();
			Game_UpdateGraphData();
			break;
		case 22:
			// Check against city milestone progression requirements and grant new milestones
			dwCityProgressionRequirement = dwCityProgressionRequirements[wCityProgression];
			if (dwCityProgressionRequirement) {
				if (dwCityProgressionRequirement < dwCityPopulation) {
					Game_SimcityApp_SetGameCursor(pSCApp, 24, 0);
					// There are only 7 (0-6) progression levels, cast the warning away.
					iPaperVal = (BYTE)wCityProgression++;
					Game_NewspaperStoryGenerator(3, iPaperVal);
					Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
					if (wCityProgression >= 4) {
						if (wCityProgression == 4)
							Game_SimulationProposeMilitaryBase();
						else if (wCityProgression == 5)
							Game_SimulationGrantReward(3, 1);
					}
					else
						Game_SimulationGrantReward(wCityProgression - 1, 1);
					Game_ToolMenuUpdate();
					Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
					Game_SimcityApp_SetGameCursor(pSCApp, 0, 0);
				}
			}

			if (bInScenario) {
				// Set our default scenario success state to true; we'll be picking off individual
				// success/fail requirements from here and marking as false if they're not met
				bScenarioSuccess = TRUE;

				// Iterate through possible the vanilla scenario requirements
				if (dwScenarioCitySize > dwCityPopulation && dwScenarioCitySize)
					bScenarioSuccess = FALSE;
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

				// Iterate through mod-based scenario goals
				for (const auto& hook : stHooks_Hook_ScenarioSuccessCheck) {
					if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
						BOOL(*fnHook)() = (BOOL(*)())hook.pFunction;
						if (!fnHook())
							bScenarioSuccess = FALSE;
					}
				}

				// Declare victory if the player has met the requirements, or tick down towards
				// failure if they haven't
				if (bScenarioSuccess)
					Game_EventScenarioNotification(GAMEOVER_SCENARIO_VICTORY);
				else if (!--wScenarioTimeLimitMonths)
					Game_EventScenarioNotification(GAMEOVER_SCENARIO_FAILURE);
			}

			// Check if the city is bankrupt and impeach the mayor if so
			if (dwCityFunds < -100000)
				Game_EventScenarioNotification(GAMEOVER_BANKRUPT);
			break;
		case 23:
			if (!bSettingsFrequentCityRefresh)
				GameMain_Document_UpdateAllViews(pCSimcityDoc, NULL, 2, NULL);
			Game_UpdatePopulationDialog();
			Game_UpdateIndustryDialog();
			Game_UpdateGraphDialog();
			break;
		case 24:
			Game_MainFrame_UpdateCityToolBar((CMainFrame *)pSCApp->m_pMainWnd);
			Game_UpdateCityMap();
			Game_UpdateSimNationDialog();
			Game_UpdateWeatherOrDisasterState();
			break;
		default:
			// Moved here rather than the prior list of cases that were
			// specific to the growth tick function.
			if (dwMonDay >= 3 && dwMonDay <= 18) {
				if (dwMonDay == 12)
					UpdateCityDateAndSeason(FALSE);
				iStep = ((dwMonDay - 3) / 4 % 4); // Steps 0 - 3 in groups of 4.
				iSubStep = (dwMonDay + 1) % 4; // SubSteps 0-3 for each group of 4.
				Game_SimulationGrowthTick(iStep, iSubStep);
				break;
			}
			return;
	}

	// Call mods for daily processing tasks - after update
	for (const auto& hook : stHooks_Hook_SimCalendarAdvance_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			BOOL(*fnHook)() = (BOOL(*)())hook.pFunction;
			fnHook();
		}
	}

	// Explanation:
	// !bSettingsFrequentCityRefresh - It will do the tile highlight update if:
	// 1) pSCApp->wSCAGameSpeedLOW is set to African Swallow
	// 2) pSCApp[198] is true (AnimationOffCycle) or it is game day 21 - CDocument::UpdateAllViews case.
	//
	// bSettingsFrequentCityRefresh - Tile highlight updates only occur if pSCApp->wSCAGameSpeedLOW
	// isn't set to paused.

	bDoTileHighlightUpdate = FALSE;
	if (!bSettingsFrequentCityRefresh) {
		if (pSCApp->wSCAGameSpeedLOW == GAME_SPEED_AFRICAN_SWALLOW) {
			if (pSCApp->dwSCAAnimationOffCycle || dwMonDay == 21)
				bDoTileHighlightUpdate = TRUE;
		}
	}
	else {
		if (pSCApp->wSCAGameSpeedLOW != GAME_SPEED_PAUSED) {
			bDoTileHighlightUpdate = TRUE;
		}
	}

	if (bDoTileHighlightUpdate) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			if (wCityMode) {
				// It should be noted that the highlight will only appear with a valid selected tool.
				// If you attempt to press Shift or Control (for the bulldozer or query) while an
				// invalid tool is selected, there'll be no placement highlighted (this matches the
				// behaviour in the normal game as well).
				if (wCurrentCityToolGroup != CITYTOOL_GROUP_CENTERINGTOOL) {
					if (wTileCoordinateX < 0 || wTileCoordinateX >= GAME_MAP_SIZE ||
						wTileCoordinateY < 0 || wTileCoordinateY >= GAME_MAP_SIZE ||
						(wCurrentCityToolGroup == CITYTOOL_GROUP_REWARDS && wSelectedSubtool[wCurrentCityToolGroup] == REWARDS_ARCOLOGIES_WAITING)) {
						wTileHighlightActive = 0;
					}
					else {
						wTileHighlightActive = 1;
						L_TileHighlightUpdate(pSCView);
					}
					Game_SimcityView_MaintainCursor(pSCView);
				}
			}
		}
	}
}

extern "C" void __stdcall Hook_SimulationStartDisaster(void) {
	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationStartDisaster(), wDisasterType = %u.\n", _ReturnAddress(), wSetTriggerDisasterType);

	GameMain_SimulationStartDisaster();
}

extern "C" int __stdcall Hook_AddAllInventions(void) {
	CSimcityAppPrimary *pSCApp;

	pSCApp = &pCSimcityAppThis;
	if (mischook_debug & MISCHOOK_DEBUG_CHEAT)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> AddAllInventions()\n", _ReturnAddress());

	memset(wCityInventionYears, 0, sizeof(WORD)*MAX_CITY_INVENTION_YEARS);
	Game_ToolMenuUpdate();
	Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_ZAP);

	return 0;
}

extern "C" void __stdcall Hook_SimcityView_OnLButtonDown(UINT nFlags, POINT pt) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	HWND hWnd;
	RECT r;

	// pThis[19] = SCVScrollBarVert
	// pThis[22] = SCVScrollBarVertRectOne
	// pThis[26] = SCVScrollBarVertRectTwo
	// pThis[30] = SCVScrollBarVertRectThree
	// pThis[34] = SCVScrollPosVertRect
	// pThis[58] = SCVStaticRect
	// pThis[62] = dwSCVLeftMouseButtonDown
	// pThis[63] = dwSCVLeftMouseDownInGameArea
	// pThis[67] = dwSCVRightClickMenuOpen

	if (pThis->dwSCVRightClickMenuOpen)
		pThis->dwSCVRightClickMenuOpen = 0;
	else if (!PtInRect(&pThis->dwSCVStaticRect, pt)) {
		Game_SimcityView_GetScreenAreaInfo(pThis, &r);
		if (PtInRect(&pThis->dwSCVScrollBarVertRectOne, pt)) {
			if (PtInRect(&pThis->dwSCVScrollBarVertRectThree, pt))
				Game_SimCityView_OnVScroll(pThis, SB_LINEDOWN, 0, pThis->dwSCVScrollBarVert);
			else if (PtInRect(&pThis->dwSCVScrollBarVertRectTwo, pt))
				Game_SimCityView_OnVScroll(pThis, SB_LINEUP, 0, pThis->dwSCVScrollBarVert);
			else if (PtInRect(&pThis->dwSCVScrollPosVertRect, pt))
				Game_SimCityView_OnVScroll(pThis, SB_THUMBTRACK, (__int16)pt.y, pThis->dwSCVScrollBarVert);
			else {
				// This part appears to be non-functional, pressing "Page Down" will rotate the map;
				// "Page Up" doesn't do anything.
				if (pThis->dwSCVScrollPosVertRect.top >= pt.y)
					Game_SimCityView_OnVScroll(pThis, SB_PAGEUP, 0, pThis->dwSCVScrollBarVert);
				else
					Game_SimCityView_OnVScroll(pThis, SB_PAGEDOWN, 0, pThis->dwSCVScrollBarVert);
			}
		}
		else if (!pThis->dwSCVLeftMouseDownInGameArea) {
			bOverrideTickPlacementHighlight = TRUE;
			hWnd = SetCapture(pThis->m_hWnd);
			GameMain_Wnd_FromHandle(hWnd);
			wCurrentTileCoordinates = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);;
			if (wCurrentTileCoordinates >= 0) {
				wTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
				wPreviousTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
				wTileCoordinateY = wCurrentTileCoordinates >> 8;
				wPreviousTileCoordinateY = wCurrentTileCoordinates >> 8;
				wGameScreenAreaX = (WORD)pt.x;
				wGameScreenAreaY = (WORD)pt.y;
				pThis->dwSCVLeftMouseDownInGameArea = 1;
				pThis->dwSCVLeftMouseButtonDown = 1;
				if (wCityMode)
					Game_CityToolMenuAction(nFlags, pt);
				else
					Game_MapToolMenuAction(nFlags, pt);
			}
		}
	}
}

extern "C" void __stdcall Hook_SimcityView_OnMouseMove(UINT nFlags, CMFC3XPoint pt) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	// pThis[62] = dwSCVLeftMouseButtonDown
	// pThis[63] = dwSCVLeftMouseDownInGameArea
	// pThis[64] = dwSCVCursorInGameArea
	// pThis[65] = SCVMousePoint

	pThis->dwSCVMousePoint = pt;
	if (pThis->dwSCVLeftMouseDownInGameArea) {
		wCurrentTileCoordinates = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		if (wCurrentTileCoordinates >= 0) {
			wTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
			wTileCoordinateY = wCurrentTileCoordinates >> 8;
			if (wPreviousTileCoordinateX != wTileCoordinateX ||
				wPreviousTileCoordinateY != wTileCoordinateY) {
				if ((int)abs(wGameScreenAreaX - pt.x) > 1 ||
					((int)abs(wGameScreenAreaY - pt.y) > 1)) {
					pThis->dwSCVCursorInGameArea = 1;
					if ((nFlags & MK_LBUTTON) != 0) {
						if (pThis->dwSCVLeftMouseButtonDown) {
							if (wCityMode) {
								if ((wCurrentCityToolGroup != CITYTOOL_GROUP_CENTERINGTOOL) || GetAsyncKeyState(VK_MENU) & 0x8000)
									Game_CityToolMenuAction(nFlags, pt);
							}
							else {
								if ((wCurrentMapToolGroup == MAPTOOL_GROUP_CENTERINGTOOL && GetAsyncKeyState(VK_MENU) & 0x8000) || // 'Center Tool' selected with either 'Alt' key pressed.
									(wCurrentMapToolGroup != MAPTOOL_GROUP_CENTERINGTOOL && (nFlags & MK_CONTROL) == 0) || // Other tool selected with 'ctrl' not pressed.
									(wCurrentMapToolGroup != MAPTOOL_GROUP_CENTERINGTOOL && (nFlags & MK_CONTROL) != 0 && GetAsyncKeyState(VK_MENU) & 0x8000)) // Other tool with 'ctrl' pressed (Center Tool) and 'Alt'.
									Game_MapToolMenuAction(nFlags, pt);
							}
						}
					}
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
}

extern "C" void __cdecl Hook_MapToolMenuAction(UINT nFlags, POINT pt) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pThis;
	__int16 iTileCoords;
	__int16 iCurrMapToolGroupWithHotKey, iCurrMapToolGroupNoHotKey;
	__int16 iTileStartX, iTileStartY;
	__int16 iTileTargetX, iTileTargetY;
	WORD wNewScreenPointX, wNewScreenPointY;
	DWORD dwIsZoomed;
	HWND hWnd;

	// pThis[62] = dwSCVLeftMouseButtonDown
	// *(DWORD *)((char *)pThis + 322) = SCVIsZoomed (This is referenced as a DWORD internally - structure alignment between it and the prior WORD at (WORD)pThis[160])

	// pThis[62] - When this is set to 0, you remain within the do/while loop until you
	//             release the left mouse button.
	//             If it is set to 1 while the left mouse button is pressed (Shift key is
	//             pressed and the iCurrToolGroupA is not 7 or 8 (trees or forest respectively)
	//             it will break out of the loop and then you end up within the WM_MOUSEMOVE
	//             call (if mouse movement is taking place).
	//
	// The change in this case is to only set pThis[62] to 0 when the iCurrToolGroupA is not
	// 'Center Tool', this will then allow it to pass-through to the WM_MOUSEMOVE call.

	pSCApp = &pCSimcityAppThis;
	pThis = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);	// TODO: is this necessary or can we just dereference pCSimcityView?
	Game_SimcityView_TileHighlightUpdate(pThis);
	iTileStartX = 400;
	iTileStartY = 400;
	iCurrMapToolGroupNoHotKey = wCurrentMapToolGroup;
	iCurrMapToolGroupWithHotKey = iCurrMapToolGroupNoHotKey;
	if ((nFlags & MK_CONTROL) != 0)
		iCurrMapToolGroupWithHotKey = MAPTOOL_GROUP_CENTERINGTOOL;
	if (iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_CENTERINGTOOL)
		pThis->dwSCVLeftMouseButtonDown = 0;
	do {
		iTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		if (iTileCoords < 0)
			break;
		iTileTargetX = (uint8_t)iTileCoords;
		iTileTargetY = iTileCoords >> 8;
		if (iTileTargetX >= GAME_MAP_SIZE || iTileTargetY < 0)
			break;
		if ((nFlags & MK_SHIFT) != 0 && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_TREES && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_FOREST) {
			pThis->dwSCVLeftMouseButtonDown = 1;
			break;
		}
		if (iTileStartX != iTileTargetX || iTileStartY != iTileTargetY) {
			switch (iCurrMapToolGroupWithHotKey) {
			case MAPTOOL_GROUP_BULLDOZER: // Bulldozing, only relevant in the CityToolMenuAction code it seems.
				Game_UseBulldozer(iTileTargetX, iTileTargetY);
				Game_SimcityView_UpdateAreaPortionFill(pThis);
				break;
			case MAPTOOL_GROUP_RAISETERRAIN: // Raise Terrain
				Game_MapToolRaiseTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_LOWERTERRAIN: // Lower Terrain
				Game_MapToolLowerTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_STRETCHTERRAIN: // Stretch Terrain (Drag vertically)
				Game_MapToolStretchTerrain(iTileTargetX, iTileTargetY, (__int16)pt.y);
				break;
			case MAPTOOL_GROUP_LEVELTERRAIN: // Level Terrain
				Game_MapToolLevelTerrain(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_WATER: // Place Water
			case MAPTOOL_GROUP_STREAM: // Place Stream
				if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_WATER) {
					if (!Game_MapToolPlaceWater(iTileTargetX, iTileTargetY) || Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
						break;
				}
				else {
					Game_MapToolPlaceStream(iTileTargetX, iTileTargetY, 100);
					if (Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
						break;
				}
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_FLOOD);
				break;
			case MAPTOOL_GROUP_TREES: // Place Tree
			case MAPTOOL_GROUP_FOREST: // Place Forest
				if (!Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_PLOP);
				if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_TREES)
					Game_MapToolPlaceTree(iTileTargetX, iTileTargetY);
				else
					Game_MapToolPlaceForest(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_CENTERINGTOOL: // Center Tool
				Game_GetScreenCoordsFromTileCoords(iTileTargetX, iTileTargetY, &wNewScreenPointX, &wNewScreenPointY);
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
				dwIsZoomed = pThis->dwSCVIsZoomed;
				if (dwIsZoomed)
					Game_SimcityView_CenterOnNewScreenCoordinates(pThis, wScreenPointX - (wNewScreenPointX >> 1), wScreenPointY - (wNewScreenPointY >> 1));
				else
					Game_SimcityView_CenterOnNewScreenCoordinates(pThis, wScreenPointX - wNewScreenPointX, wScreenPointY - wNewScreenPointY);
				break;
			default:
				break;
			}
		}
		if (iCurrMapToolGroupWithHotKey >= MAPTOOL_GROUP_RAISETERRAIN && iCurrMapToolGroupWithHotKey <= MAPTOOL_GROUP_LEVELTERRAIN)
			break;
		else if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_CENTERINGTOOL) {
			Game_SimcityView_UpdateAreaCompleteColorFill(pThis);
			hWnd = pThis->m_hWnd;
			UpdateWindow(hWnd);
			break;
		}
		Game_SimcityView_UpdateAreaPortionFill(pThis);
		iTileStartX = iTileTargetX;
		iTileStartY = iTileTargetY;
		hWnd = pThis->m_hWnd;
		UpdateWindow(hWnd);
	} while (Game_GetGameAreaMouseActivity(pThis, &pt));
	if (iCurrMapToolGroupNoHotKey != iCurrMapToolGroupWithHotKey)
		wCurrentCityToolGroup = iCurrMapToolGroupNoHotKey;
}

extern "C" void __stdcall Hook_SimcityApp_LoadCursorResources() {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	HDC hDC;

	hDC = GetDC(0);
	pThis->iSCAGDCHorzRes = GetDeviceCaps(hDC, HORZRES);
	ReleaseDC(0, hDC);
	GameMain_SimcityApp_LoadCursorResources(pThis);
}

extern "C" int __stdcall Hook_StartupGraphics() {
	HDC hDC_One, hDC_Two;
	int iPlanes, iBitsPixel, iBitRate;
	PALETTEENTRY *p_pEnt;
	colStruct *pCol;
	DWORD pvIn;
	DWORD pvOut;
	LOGPAL plPal;

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
			pvIn = SETCOLORTABLE;
			if (Escape(hDC_Two, QUERYESCSUPPORT, 4, (LPCSTR)&pvIn, 0)) {
				p_pEnt = plPal.pPalEnts;
				pCol = rgbLoColor;
				do {
					colTable cT;

					memset(&cT, 0, sizeof(colTable));
					cT.Index = pCol->wPos;
					cT.rgb = RGB(pCol->pe.peRed, pCol->pe.peGreen, pCol->pe.peBlue);

					Escape(hDC_Two, SETCOLORTABLE, 6, (LPCSTR)&cT, &pvOut);
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

extern "C" void __stdcall Hook_ShowViewControls() {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMFC3XScrollBar *pSCVScrollBarHorz;
	CMFC3XScrollBar *pSCVScrollBarVert;
	CMFC3XStatic *pSCVStatic;

	pSCApp = &pCSimcityAppThis;
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	pSCVScrollBarHorz = pSCView->dwSCVScrollBarHorz;
	pSCVScrollBarVert = pSCView->dwSCVScrollBarVert;
	pSCVStatic = pSCView->dwSCVStaticOne;
	if (!bRedraw) {
		bRedraw = TRUE;
		if (pSCApp->iSCAProgramStep == ONIDLE_STATE_RETURN_12 || !wCityMode)
			Game_MainFrame_ToggleStatusControlBar(pMainFrm, FALSE);
		else {
			if (!CanUseFloatingStatusDialog())
				Game_MainFrame_ToggleStatusControlBar(pMainFrm, TRUE);
		}
		GameMain_FrameWnd_RecalcLayout(pMainFrm, TRUE);
		ShowWindow(pSCVScrollBarHorz->m_hWnd, SW_SHOWNORMAL);
		ShowWindow(pSCVScrollBarVert->m_hWnd, SW_SHOWNORMAL);
		ShowWindow(pSCVStatic->m_hWnd, SW_SHOWNORMAL);
	}
}

extern "C" void __stdcall Hook_MainFrame_UpdateSections() {
	CMainFrame *pThis;

	__asm mov[pThis], ecx

	HWND hDlgItem;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	int iCityToolBarButton;
	UINT ButtonStyle;
	int nLayer;
	UINT nStyle;
	int nIndex;
	HMENU hMenu;
	CMFC3XMenu *pMenu;
	int nPos;
	HMENU hSubMenu;
	CMFC3XMenu *pSubMenu;
	int nMenuItemCount;
	int nSubMenuItemCount;
	char szString[960];
	char *pString;
	char *pTargString;
	DWORD uIDs[12];
	DWORD *pUID;
	int nGranted;
	int nReward;
	unsigned nRewardBit;
	CMFC3XString *citySubToolStrings;

	hDlgItem = GetDlgItem(pThis->dwMFStatusControlBar.m_hWnd, 120); // Status - GoTo button.
	pMapToolBar = &pThis->dwMFMapToolBar;
	if (!wCityMode)
		Game_MapToolBar_ResetControls(pMapToolBar);
	pCityToolBar = &pThis->dwMFCityToolBar;
	Game_CityToolBar_UpdateControls(pCityToolBar, FALSE);
	ToggleGotoButton(hDlgItem, FALSE);
	if (wCityMode == GAME_MODE_CITY) {
		if (wCurrentCityToolGroup == CITYTOOL_GROUP_DISPATCH) {
			wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
			Game_CityToolBar_UpdateControls(pCityToolBar, FALSE);
		}
		Game_MainFrame_DisableCityToolBarButton(pThis, CITYTOOL_BUTTON_DISPATCH);
		Game_MyToolBar_InvalidateButton(pCityToolBar, CITYTOOL_BUTTON_DISPATCH);
	}
	else if (wCityMode != GAME_MODE_DISASTER)
		goto REFRESHMENUGRANTS;
	if (wCityMode == GAME_MODE_DISASTER)
		ToggleGotoButton(hDlgItem, TRUE);
	if (!dwGrantedItems[CITYTOOL_GROUP_REWARDS]) {
		Game_MainFrame_DisableCityToolBarButton(pThis, CITYTOOL_BUTTON_REWARDS);
		Game_MyToolBar_InvalidateButton(pCityToolBar, CITYTOOL_BUTTON_REWARDS);
	}
	iCityToolBarButton = wCurrentCityToolGroup;
	// Adjust here; this used to check to see whether
	// wCurrentCityToolGroup is greater than CITYTOOL_GROUP_SIGNS
	// however during disaster cases.. if the query button was selected
	// and the disaster ended, it would end up highlighting the zoom in button.
	// 
	// This behaviour has been confirmed in the base game and interactive demo
	// in order to confirm the apparent buggy nature (unless one wanted to
	// highlight the zoom in button for whatever reason without changing the
	// underlying tool).
	if (wCurrentCityToolGroup > CITYTOOL_GROUP_QUERY)
		iCityToolBarButton = wCurrentCityToolGroup + 4;
	ButtonStyle = Game_MyToolBar_GetButtonStyle(pCityToolBar, iCityToolBarButton);
	Game_MyToolBar_SetButtonStyle(pCityToolBar, iCityToolBarButton, ButtonStyle | TBBS_CHECKED);
	for (nLayer = LAYER_UNDERGROUND; nLayer < LAYER_COUNT; ++nLayer) {
		if (DisplayLayer[nLayer])
			nStyle = (TBBS_CHECKED|TBBS_CHECKBOX);
		else
			nStyle = TBBS_CHECKBOX;
		nIndex = CITYTOOL_BUTTON_DISPLAYUNDERGROUND - nLayer;
		Game_MyToolBar_SetButtonStyle(pCityToolBar, nIndex, nStyle);
	}
REFRESHMENUGRANTS:
	pMenu = &pCityToolBar->dwCTBMenuOne;
	GameMain_Menu_DestroyMenu(pMenu);
	hMenu = LoadMenuA(hGameModule, (LPCSTR)136);
	GameMain_Menu_Attach(pMenu, hMenu);
	for (nPos = CITYTOOL_BUTTON_BULLDOZER; nPos < CITYTOOL_BUTTON_SIGNS; ++nPos) {
		if (dwGrantedItems[nPos]) {
			hSubMenu = GetSubMenu(pMenu->m_hMenu, nPos);
			pSubMenu = GameMain_Menu_FromHandle(hSubMenu);
			nMenuItemCount = GetMenuItemCount(pSubMenu->m_hMenu);
			nSubMenuItemCount = nMenuItemCount;
			if (nMenuItemCount > 0) {
				pString = szString;
				pUID = uIDs;
				do {
					*pUID++ = GetMenuItemID(pSubMenu->m_hMenu, 0);
					pTargString = pString;
					pString += 80;
					GetMenuStringA(pSubMenu->m_hMenu, 0, pTargString, 80, MF_BYPOSITION);
					DeleteMenu(pSubMenu->m_hMenu, 0, MF_BYPOSITION);
					--nSubMenuItemCount;
				} while (nSubMenuItemCount);
			}
			nGranted = 0;
			nReward = 0;
			if (nMenuItemCount > 0) {
				pUID = uIDs;
				pString = szString;
				// calculation here is citytoolbuttongroup * maxmenutools, this sets it to CITYTOOL_GROUP_REWARDS.
				citySubToolStrings = &cityToolGroupStrings[CITYTOOL_GROUP_REWARDS*MAX_CITY_MENUTOOLS];
				do {
					// (1 << nReward) bit-shifted result of the nReward count.
					nRewardBit = (1 << nReward);
					if ((nRewardBit & dwGrantedItems[nPos]) != 0) {
						if (nPos == CITYTOOL_BUTTON_REWARDS && !nGranted) {
							pCityToolBar->dwCTToolSelection[CITYTOOL_GROUP_REWARDS] = nReward;
							nGranted = 1;
							GameMain_String_OperatorCopy(&pCityToolBar->dwCTBString[CITYTOOL_GROUP_REWARDS], &citySubToolStrings[nReward]);
						}
						AppendMenuA(pSubMenu->m_hMenu, 0, *pUID, pString);
					}
					++pUID;
					pString += 80;
					++nReward;
				} while (nMenuItemCount > nReward);
			}
		}
	}
	Game_CityToolBar_RefreshToolBar(pCityToolBar);
}

// Hook for the scenario description popup
__declspec(naked) void Hook_402B4E(const char* szDescription, int a2, void* cWnd) {
	__asm push ecx

	if (szDescription && strlen(szDescription))
		scScenarioDescription = szDescription;
	dwScenarioStartDays = dwCityDays;
	dwScenarioStartPopulation = dwCityPopulation;
	wScenarioStartXVALTiles = wCityDevelopedTiles;
	dwScenarioStartTrafficDivisor = pBudgetArr[10].iCurrentCosts + pBudgetArr[11].iCurrentCosts + pBudgetArr[12].iCurrentCosts + 1;		// XXX - this should be a descriptive macro

	__asm pop ecx
	GAMEJMP(0x42DC20);
}

static BOOL L_OnCmdMsg(CMFC3XWnd *pThis, UINT nID, int nCode, void *pExtra, void *pHandler, void *dwRetAddr) {
	// Normally internally there'd be the class hierarchy regarding inheritence
	// (which isn't present here).
	//
	// 0x4B9080 - with CFrameWnd - use CFrameWnd::OnCmdMsg
	//
	// All other flagged address references have thus far gracefully
	// gone to CCmdTarget::OnCmdMsg (which is the non-overridden virtual call).
	//
	// If others also require specific handling, checkout the returned address
	// and see where it specifically happens to originate.
	if ((DWORD)dwRetAddr == 0x4B9080) {
		if (nCode == _CN_COMMAND) {
			switch (nID) {
			case IDM_GAME_OPTIONS_SC2KFIXSETTINGS:
				ShowSettingsDialog();
				return TRUE;

			case IDM_GAME_OPTIONS_MODCONFIG:
				ShowModSettingsDialog();
				return TRUE;

			case IDM_DEBUG_MILITARY_DECLINED:
				ProposeMilitaryBaseDecline();
				return TRUE;

			case IDM_DEBUG_MILITARY_AIRFORCE:
				ProposeMilitaryBaseAirForceBase();
				return TRUE;

			case IDM_DEBUG_MILITARY_ARMYBASE:
				ProposeMilitaryBaseArmyBase();
				return TRUE;

			case IDM_DEBUG_MILITARY_NAVALYARD:
				ProposeMilitaryBaseNavalYard();
				return TRUE;

			case IDM_DEBUG_MILITARY_MISSILESILOS:
				ProposeMilitaryBaseMissileSilos();
				return TRUE;

			case IDM_GAME_WINDOWS_SCENARIOGOALS:
				ShowScenarioStatusDialog();
				return TRUE;

			case IDM_GAME_FILE_RELOADDEFAULTTILESET:
				ReloadDefaultTileSet_SC2K1996();
				return TRUE;
			}
		}
		else if (nCode == _CN_COMMAND_UI) {
			// As far as potential handling here goes - tread carefully;
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - _CN_COMMAND_UI\n", pThis, nID, nCode, pExtra, pHandler);
		}
		return GameMain_FrameWnd_OnCmdMsg((CMFC3XFrameWnd *)pThis, nID, nCode, pExtra, pHandler);
	}
	if ((DWORD)dwRetAddr == 0x4A4BB2) {
		if (nCode == _CN_COMMAND) {
			switch (nID) {
				// This is the 'sc2kfix Settings' entry in the main dialog.
			case IDC_GAME_MAIN_SC2KFIXSETTINGS:
				ShowSettingsDialog();
				return TRUE;
			}
		}
	}
	else {
		// Leaving this particular debug notice enabled without any flags.
		// It is particularly important that this is picked up if any
		// strange cases appear. Thus far it hasn't.. but you never know.
		ConsoleLog(LOG_DEBUG, "?::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
	}

	return GameMain_CmdTarget_OnCmdMsg(pThis, nID, nCode, pExtra, pHandler);
}

extern "C" BOOL __stdcall Hook_Wnd_OnCommand(WPARAM wParam, LPARAM lParam) {
	CMFC3XWnd *pThis;

	__asm mov[pThis], ecx

	CMFC3XWnd *pWndHandle;
	CMFC3XTestCmdUI testCmd;

	// AFX_THREAD_STATE -> DWORD:
	// var[40] -> m_hLockoutNotifyWindow

	UINT nID = LOWORD(wParam);
	HWND hWndCtrl = (HWND)lParam;
	int nCode = HIWORD(wParam);

	if (nID == 0)
		return FALSE;

	if (hWndCtrl == NULL) {
		GameMain_TestCmdUI_Construct(&testCmd);
		testCmd.m_nID = nID;
		L_OnCmdMsg(pThis, nID, _CN_COMMAND_UI, &testCmd, 0, _ReturnAddress());
		if (!testCmd.m_bEnabled)
			return TRUE;
		nCode = _CN_COMMAND;
	}
	else {
		if (GameMain_AfxGetThreadState()->m_hLockoutNotifyWindow == pThis->m_hWnd)
			return TRUE;

		pWndHandle = GameMain_Wnd_FromHandlePermanent(hWndCtrl);
		if (pWndHandle != NULL && GameMain_Wnd_SendChildNotifyLastMsg(pWndHandle, 0))
			return TRUE;
	}

	return L_OnCmdMsg(pThis, nID, nCode, 0, 0, _ReturnAddress());
}

// Placeholder.
void ShowModSettingsDialog(void) {
	L_MessageBoxA(GameGetRootWindowHandle(), "The mod settings dialog has not yet been implemented. Check back later.", "sc2fix", MB_OK);
}

// Install hooks and run code that we only want to do for the 1996 Special Edition SIMCITY.EXE.
// This should probably have a better name. And maybe be broken out into smaller functions.
//
// UPDATE 2025-08-15 (araxestroy): Working on breaking this out nicely. It's not going well.
void InstallMiscHooks_SC2K1996(void) {
	// Install critical Windows API hooks
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;
	*(DWORD*)(0x4EFDCC) = (DWORD)Hook_LoadMenuA;
	*(DWORD*)(0x4EFC64) = (DWORD)Hook_DialogBoxParamA;
	*(DWORD*)(0x4EFE70) = (DWORD)Hook_ShowWindow;
	*(DWORD*)(0x4EFCE8) = (DWORD)Hook_DefWindowProcA;

	// Install Smacker hooks
	GetSMKFuncs();

	// Install registry pathing hooks
	InstallRegistryPathingHooks_SC2K1996();

	// Hook into both AfxMessageBox functions
	VirtualProtect((LPVOID)0x4B232F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B232F, Hook_AfxMessageBoxStr);
	VirtualProtect((LPVOID)0x4B234F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B234F, Hook_AfxMessageBoxID);

	// Hook into the CFileDialog::DoModal function
	VirtualProtect((LPVOID)0x49FE18, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x49FE18, Hook_FileDialog_DoModal);

	// Fix the sign fonts
	VirtualProtect((LPVOID)0x4E7267, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4E7267 = 'a';
	VirtualProtect((LPVOID)0x44DC42, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC42 = 5;
	VirtualProtect((LPVOID)0x44DC4F, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC4F = 10;

	// Hook CSimcityApp::OnQuit
	VirtualProtect((LPVOID)0x401753, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401753, Hook_SimcityApp_OnQuit);

	InstallSpriteAndTileSetHooks_SC2K1996();

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

	// Fix the pipe tool not refreshing properly at max zoom
	VirtualProtect((LPVOID)0x43F447, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x43F447, 0x402810);		// CSimcityView::UpdateAreaCompleteColorFill

	// Install hooks for saving and loading
	InstallSaveHooks_SC2K1996();

	// Hook into the StartCleanGame function.
	VirtualProtect((LPVOID)0x401F05, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401F05, Hook_StartCleanGame);

	InstallTileGrowthOrPlacementHandlingHooks_SC2K1996();

	// Move the alt+query bottom text to not be blocked by the OK button
	VirtualProtect((LPVOID)0x428FB1, 3, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x428FB1 = 0x83;
	*(BYTE*)0x428FB2 = 0xE8;
	*(BYTE*)0x428FB3 = 0x32;
	
	// Install the advanced query hook
	if (bUseAdvancedQuery)
		InstallQueryHooks_SC2K1996();

	// Expand sound buffers and load higher quality sounds from DLL resources
	LoadReplacementSounds();

	// Install music engine hooks
	InstallMusicEngineHooks();

	// Hook status bar updates for the status dialog implementation
	InstallStatusHooks_SC2K1996();

	// Hook for ShowViewControls
	VirtualProtect((LPVOID)0x4021D5, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021D5, Hook_ShowViewControls);

	// Hook for CMainFrame::UpdateSections
	VirtualProtect((LPVOID)0x40131B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40131B, Hook_MainFrame_UpdateSections);

	InstallToolBarHooks_SC2K1996();

	// New hooks for CSimcityDoc::UpdateDocumentTitle and
	// SimulationProcessTick - these account for:
	// 1) Including the day of the month in the window title.
	// 2) The fine-grained simulation updates.
	VirtualProtect((LPVOID)0x4017B2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017B2, Hook_SimcityDoc_UpdateDocumentTitle);
	VirtualProtect((LPVOID)0x401820, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401820, Hook_Engine_SimulationProcessTick);

	// Hook SimulationStartDisaster
	VirtualProtect((LPVOID)0x402527, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402527, Hook_SimulationStartDisaster);

	// Hook AddAllInventions
	VirtualProtect((LPVOID)0x402388, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402388, Hook_AddAllInventions);

	// Hook CWnd::OnCommand
	VirtualProtect((LPVOID)0x4A5352, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A5352, Hook_Wnd_OnCommand);

	// Add more buttons to SC2K's menus
	// TODO: write a much cleaner and more programmatic way of doing this
	hGameMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(3));
	if (hGameMenu) {
		// File menu -> Reload Default Tileset
		HMENU hFilePopup;
		MENUITEMINFO miiFilePopup;
		miiFilePopup.cbSize = sizeof(MENUITEMINFO);
		miiFilePopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hGameMenu, 0, TRUE, &miiFilePopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		hFilePopup = miiFilePopup.hSubMenu;
		if (!InsertMenu(hFilePopup, 6, MF_BYPOSITION|MF_STRING, IDM_GAME_FILE_RELOADDEFAULTTILESET, "Reload &Default Tile Set") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}

		// Options menu -> add sc2kfix Settings... and Mod Configuration...
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

		// Windows menu -> add Show Scenario Goals...
		HMENU hMenuWindowsPopup;
		MENUITEMINFO miiWindowsPopup;
		miiWindowsPopup.cbSize = sizeof(MENUITEMINFO);
		miiWindowsPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hGameMenu, 4, TRUE, &miiWindowsPopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		hMenuWindowsPopup = miiWindowsPopup.hSubMenu;
		if (!InsertMenu(hMenuWindowsPopup, -1, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}
		if (!InsertMenu(hMenuWindowsPopup, -1, MF_BYPOSITION | MF_STRING, IDM_GAME_WINDOWS_SCENARIOGOALS, "Show &Scenario Goals...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Game InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipgamemenu;
		}

		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated game menu.\n");
	}

skipgamemenu:
	// Hook for the game area leftmousebuttondown call.
	VirtualProtect((LPVOID)0x401523, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401523, Hook_SimcityView_OnLButtonDown);

	// Hook for the game area mouse movement call.
	VirtualProtect((LPVOID)0x4016EA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4016EA, Hook_SimcityView_OnMouseMove);

	// Hook for the MapToolMenuAction call.
	VirtualProtect((LPVOID)0x402B44, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B44, Hook_MapToolMenuAction);

	// Hook for CSimcityApp::LoadCursorResources
	VirtualProtect((LPVOID)0x402234, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402234, Hook_SimcityApp_LoadCursorResources);

	// Hook for StartupGraphics
	VirtualProtect((LPVOID)0x4014DD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4014DD, Hook_StartupGraphics);

	// Hook for CCmdUI::Enable
	VirtualProtect((LPVOID)0x4A296A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A296A, Hook_CmdUI_Enable);

	// Hook the scenario start dialog so we can save the description
	VirtualProtect((LPVOID)0x402B4E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B4E, Hook_402B4E);

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

	// Call your cousin Vinnie!
	PorntipsGuzzardo();

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

