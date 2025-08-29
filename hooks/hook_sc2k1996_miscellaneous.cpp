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

extern "C" int __stdcall Hook_AfxMessageBoxStr(LPCTSTR lpszPrompt, UINT nType, UINT nIDHelp) {
	int(__thiscall *H_CWinAppDoMessageBox)(void *, LPCTSTR, UINT, UINT) = (int(__thiscall *)(void *, LPCTSTR, UINT, UINT))0x4B2206;

	DWORD &game_AfxCoreState = *(DWORD *)0x4CE8C0;

	int ret;

	ToggleFloatingStatusDialog(FALSE);
	ret = H_CWinAppDoMessageBox((DWORD *)game_AfxCoreState, lpszPrompt, nType, nIDHelp);
	ToggleFloatingStatusDialog(TRUE);

	return ret;
}

extern "C" int __stdcall Hook_AfxMessageBoxID(UINT nIDPrompt, UINT nType, UINT nIDHelp) {
	CMFC3XString *(__thiscall *H_CStringCons)(CMFC3XString *) = (CMFC3XString *(__thiscall *)(CMFC3XString *))0x4A2C28;
	void(__thiscall *H_CStringDest)(CMFC3XString *) = (void(__thiscall *)(CMFC3XString *))0x4A2CB0;
	BOOL(__thiscall *H_CStringLoadStringA)(CMFC3XString *, unsigned int) = (BOOL(__thiscall *)(CMFC3XString *, unsigned int))0x4A3453;
	int(__thiscall *H_CWinAppDoMessageBox)(void *, LPCTSTR, UINT, UINT) = (int(__thiscall *)(void *, LPCTSTR, UINT, UINT))0x4B2206;

	DWORD &game_AfxCoreState = *(DWORD *)0x4CE8C0;

	CMFC3XString cStr;
	UINT nID;
	int ret;

	H_CStringCons(&cStr);
	H_CStringLoadStringA(&cStr, nIDPrompt);
	nID = nIDHelp;
	if (nIDHelp == -1)
		nID = nIDPrompt;

	ToggleFloatingStatusDialog(FALSE);
	ret = H_CWinAppDoMessageBox((DWORD *)game_AfxCoreState, cStr.m_pchData, nType, nIDHelp);
	ToggleFloatingStatusDialog(TRUE);

	H_CStringDest(&cStr);
	return ret;
}

extern "C" int __stdcall Hook_FileDialogDoModal() {
	DWORD *pThis;

	__asm mov [pThis], ecx

	HWND(__thiscall *H_DialogPreModal)(void *) = (HWND(__thiscall *)(void *))0x4A710B;
	BOOL(__stdcall *H_GetLoadFileNameA)(LPOPENFILENAMEA) = (BOOL(__stdcall *)(LPOPENFILENAMEA))0x49C35A;
	BOOL(__stdcall *H_GetSaveFileNameA)(LPOPENFILENAMEA) = (BOOL(__stdcall *)(LPOPENFILENAMEA))0x49C354;
	void(__thiscall *H_DialogPostModal)(void *) = (void(__thiscall *)(void *))0x4A7154;

	HWND hWndOwner;
	bool bIsReserved;
	int iRet;
	int nPathLen, nFileLen, nNewLen;
	char szPath[MAX_PATH + 1];
	OPENFILENAMEA* pOfn;

	memset(szPath, 0, sizeof(szPath));

	ToggleFloatingStatusDialog(FALSE);

	hWndOwner = H_DialogPreModal(pThis);
	bIsReserved = pThis[36] == 0;
	pThis[18] = (DWORD)hWndOwner;
	pOfn = (OPENFILENAMEA*)(pThis + 17);

	if (bIsReserved)
		iRet = H_GetSaveFileNameA(pOfn);
	else
		iRet = H_GetLoadFileNameA(pOfn);
	H_DialogPostModal(pThis);
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

extern "C" void __stdcall Hook_SimcityAppOnQuit(void) {
	DWORD *pThis;

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

	pThis[64] = 1;
	pThis[63] = 0;
	iReqRet = Game_ExitRequester((void *)pThis, pThis[206]);
	if (iReqRet != IDCANCEL) {
		if (iReqRet == IDYES)
			Game_DoSaveCity(pThis);
		Game_PreGameMenuDialogToggle((void *)pThis[7], 0);
		Game_CWinApp_OnAppExit(pThis);
		return;
	}
	pThis[64] = 0;
	pThis[63] = 0;
}

// Hook CCmdUI::Enable so we can programmatically enable and disable menu items reliably
extern "C" void __stdcall Hook_CCmdUI_Enable(BOOL bOn) {
	CMFC3XCmdUI *pThis;
	__asm mov [pThis], ecx

	HWND hWndParent;
	DWORD *pWndParent;
	HWND hNextDlgTabItem;
	DWORD *pNextDlgTabItem;
	HWND hWndFocus;

	if (pThis->m_pMenu != NULL) {
		if (pThis->m_pSubMenu != NULL)
			return;

		EnableMenuItem(pThis->m_pMenu->m_hMenu, pThis->m_nIndex, MF_BYPOSITION |
			(bOn ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else {
		if (!bOn && (GetFocus() == (HWND)pThis->m_pOther[7])) {
			hWndParent = GetParent((HWND)pThis->m_pOther[7]);
			pWndParent = Game_CWnd_FromHandle(hWndParent);
			hNextDlgTabItem = GetNextDlgTabItem((HWND)pWndParent[7], (HWND)pThis->m_pOther[7], 0);
			pNextDlgTabItem = Game_CWnd_FromHandle(hNextDlgTabItem);
			hWndFocus = SetFocus((HWND)pNextDlgTabItem[7]);
			Game_CWnd_FromHandle(hWndFocus);
		}
		EnableWindow((HWND)pThis->m_pOther[7], bOn);
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

		strcpy_s(dwMapXLAB[0][0].szLabel, 24, szTempMayorName);

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

// CSimcityView Middle Mouse Button Down handler.
static void SimcityViewOnMButtonDown(UINT nFlags, POINT pt) {
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
			Game_SoundPlaySound(&pCSimcityAppThis, SOUND_CLICK);
			Game_CenterOnTileCoords(bTileX, bTileY);
		}
	}
}

extern "C" LRESULT __stdcall Hook_DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	DWORD *pSCView;

	pSCView = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
	if (Msg == WM_MBUTTONDOWN) {
		if (pSCView && hWnd == (HWND)pSCView[7]) {
			POINT pt;

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			SimcityViewOnMButtonDown((UINT)wParam, pt);
			return TRUE;
		}
	}
	return DefWindowProcA(hWnd, Msg, wParam, lParam);
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

extern int iChurchVirus;

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
	__int16 iCurrentDemand;
	__int16 iRemainderDemand;
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
						if (Game_RunTripGenerator(iX, iY, iCurrZoneType, iBuildingPopLevel, 100)) {
							iCurrentDemand = wCityDemand[(__int16)((iCurrZoneType - 1) / 2)] + 2000;
							iRemainderDemand = 4000 - iCurrentDemand;
						}
						else {
							iCurrentDemand = 0;
							iRemainderDemand = 4000;
						}
					}
					else {
						iCurrentDemand = 0;
						iRemainderDemand = 4000;
					}
					// This block is encountered when a given area is not "under construction" and not "abandonded".
					// A building is then randomly selected in 
					if (iBuildingPopLevel > 0 && !bAreaState[(__int16)iAttributes]) {
						pZonePops[iCurrZoneType] += wBuildingPopulation[iBuildingPopLevel]; // Values appear to be: 1[1], 8[2], 12[3], 36[4] (wBuildingPopulation[iBuildingPopLevel] format.
						if ((unsigned __int16)rand() < (__int16)(iRemainderDemand / iBuildingPopLevel)) {
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
						if ((unsigned __int16)rand() >= 15 * iCurrentDemand / iBuildingPopLevel)
							goto GOUNDCHECKTHENYINCREASE;
						goto GOGENERALZONEITEMPLACE;
					}
					// This block is where construction will start.
					if (iBuildingPopLevel != 4 &&
						((iCurrZoneType & 1) == 0 || iBuildingPopLevel <= 0) &&
						(iCurrZoneType >= 5 ||
						(iBuildingPopLevel != 1 || dwMapXVAL[iXMM][iYMM].bBlock >= 0x20u) &&
							(iBuildingPopLevel != 2 || dwMapXVAL[iXMM][iYMM].bBlock >= 0x60u) &&
							(iBuildingPopLevel != 3 || dwMapXVAL[iXMM][iYMM].bBlock >= 0xC0u))) {
						iAttributes = 3 * iCurrentDemand / (iBuildingPopLevel + 1);
						if (iAttributes > (unsigned __int16)rand())
							Game_PerhapsGeneralZoneStartBuilding(iX, iY, iBuildingPopLevel, iCurrZoneType);
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
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							goto GOAFTERSETXBIT;
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
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							goto GOAFTERSETXBIT;
GOBEFORESETXBIT:
						*(BYTE *)&dwMapXBIT[iX][iY].b &= ~0x80u;
GOAFTERSETXBIT:
						if ((__int16)iAttributes >= TILE_INFRASTRUCTURE_RAILSTATION) {
							if ((WORD)iAttributes == TILE_INFRASTRUCTURE_RAILSTATION &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iY].b.iPowered != 0 &&
								!(unsigned __int16)Game_RandomWordLFSRMod4()) {
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_RAILSTATION] / 4 > wActiveTrains)
									Game_SpawnTrain(iX, iY);
							}
							else if ((WORD)iAttributes == TILE_INFRASTRUCTURE_MARINA &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								dwMapXBIT[iX][iY].b.iPowered != 0 &&
								!(unsigned __int16)Game_RandomWordLFSRMod4()) {
								if ((__int16)dwTileCount[TILE_INFRASTRUCTURE_MARINA] / 9 > wSailingBoats)
									Game_SpawnSailBoat(iX, iY);
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
	if (iZoneType != ZONE_MILITARY)
		if (!Game_IsZonedTilePowered(iX, iY))
			return 0;
	
	switch (iTileID) {
		case TILE_INFRASTRUCTURE_RUNWAY:
			iMoveX = 0;
			iMoveY = 0;
			if ((dwTileCount[TILE_INFRASTRUCTURE_RUNWAY] & 1) == 0) {
				if ((x & 1) != 0) {
					iMoveX = 1;
					goto PROCEEDFURTHER;
				}
				if ((y & 1) == 0)
					return 0;

				goto PROCEEDAHEAD;
			}
			if ((y & 1) != 0) {
PROCEEDAHEAD:
				iMoveY = 1;
				goto PROCEEDFURTHER;
			}
			if ((x & 1) == 0)
				return 0;
			
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
			if (iBuilding >= TILE_ROAD_LR)
				return 0;
			
			if (iBuilding == TILE_RADIOACTIVITY)
				return 0;
			
			if (iBuilding == TILE_SMALLPARK)
				return 0;
			
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

			if (dwMapXTER[iX][iY].iTileID)
				return 0;
			
			if (iX < GAME_MAP_SIZE &&
				iY < GAME_MAP_SIZE &&
				dwMapXBIT[iX][iY].b.iWater != 0) {
				return 0;
			}

		GOSKIP:
			if (++iY > iItemLength)
				goto GOBACK;
			
		}
	}

	iTile = iTileID;

GOFORWARD:
	if (iTile == TILE_INFRASTRUCTURE_MARINA && (!iMarinaCount || iMarinaCount == 9)) {
		Game_AfxMessageBoxID(107, 0, -1);
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

extern "C" void __stdcall Hook_SimcityDocUpdateDocumentTitle() {
	DWORD *pThis;

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
			H_CStringLoadStringA(&cStr, 0x19D); // "Editing Terrain..."
			goto GOFORWARD;
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
		Game_CDocument_UpdateAllViews(pThis, 0, 1, &cStr);
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

	DWORD &pSomeWnd = *(DWORD *)0x4CAC18; // Perhaps this is the active view window? (unclear - but this is referenced in the native TileHighlightUpdate function)
	RECT &dRect = *(RECT *)0x4CAD48;

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
			// As it turns out this if case is necessary here.. otherwise it results in breakage when
			// it comes to the pollution clouds (entire view window update rather than just the
			// "dirty" area).
			// ^ Unclear - the pollution case still expresses itself even with this case implemented.
			// Tests performed in the 'Interactive Demo' (of which don't have any of these hooks) have
			// also resulted in similar intermittent encounters.
			if (pThis == &pSomeWnd)
				H_SimcityViewMainWindowUpdate(pThis, 0, 1);
			else
				H_SimcityViewMainWindowUpdate(pThis, &dRect, 1);
			if (bOverrideTickPlacementHighlight)
				wTileHighlightActive = 0;
		}
	}
}

static void UpdateCityDateAndSeason(BOOL bIncrement) {
	if (bIncrement)
		++dwCityDays;
	wCityCurrentMonth = dwCityDays / 25 % 12;
	wCityCurrentSeason = (dwCityDays / 25 % 12 + 1) % 12 / 3;
	wCityElapsedYears = dwCityDays / 300;
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

	UpdateCityDateAndSeason(TRUE);
	dwMonDay = (dwCityDays % 25);
	if (dwSCAGameAutoSave > 0 &&
		!((dwCityDays / 300) % dwSCAGameAutoSave) &&
		!wCityCurrentMonth &&
		!dwMonDay) {
		H_SimcityAppCallAutoSave(&pCSimcityAppThis);
	}

	if (bSettingsFrequentCityRefresh) {
		Game_RefreshTitleBar(pCDocumentMainWindow);
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
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
			UpdateCityDateAndSeason(FALSE);
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
			// Check against city milestone progression requirements and grant new milestones
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
					H_EventScenarioNotification(GAMEOVER_SCENARIO_VICTORY);
				else if (!--wScenarioTimeLimitMonths)
					H_EventScenarioNotification(GAMEOVER_SCENARIO_FAILURE);
			}

			// Check if the city is bankrupt and impeach the mayor if so
			if (dwCityFunds < -100000)
				H_EventScenarioNotification(GAMEOVER_BANKRUPT);
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
				if (dwMonDay == 12)
					UpdateCityDateAndSeason(FALSE);
				iStep = ((dwMonDay - 3) / 4 % 4); // Steps 0 - 3 in groups of 4.
				iSubStep = (dwMonDay + 1) % 4; // SubSteps 0-3 for each group of 4.
				H_SimulationGrowthTick(iStep, iSubStep);
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

extern "C" void __stdcall Hook_CSimcityView_WM_LBUTTONDOWN(UINT nFlags, POINT pt) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	HWND hWnd;
	RECT r;
	const RECT *SCVScrollPosVertRect;

	// pThis[19] = SCVScrollBarVert
	// pThis[22] = SCVScrollBarVertRectOne
	// pThis[26] = SCVScrollBarVertRectTwo
	// pThis[30] = SCVScrollBarVertRectThree
	// pThis[34] = SCVScrollPosVertRect
	// pThis[58] = SCVStaticRect
	// pThis[62] = dwSCVLeftMouseButtonDown
	// pThis[63] = dwSCVLeftMouseDownInGameArea
	// pThis[67] = dwSCVRightClickMenuOpen

	if (pThis[67])
		pThis[67] = 0;
	else if (!PtInRect((const RECT *)&pThis[58], pt)) {
		Game_GetScreenAreaInfo(pThis, &r);
		if (PtInRect((const RECT *)&pThis[22], pt)) {
			if (PtInRect((const RECT *)&pThis[30], pt))
				Game_CSimCityView_OnVScroll(pThis, SB_LINEDOWN, 0, (DWORD *)pThis[19]);
			else if (PtInRect((const RECT *)&pThis[26], pt))
				Game_CSimCityView_OnVScroll(pThis, SB_LINEUP, 0, (DWORD *)pThis[19]);
			else if (PtInRect((const RECT *)&pThis[34], pt))
				Game_CSimCityView_OnVScroll(pThis, SB_THUMBTRACK, (__int16)pt.y, (DWORD *)pThis[19]);
			else {
				// This part appears to be non-functional, pressing "Page Down" will rotate the map;
				// "Page Up" doesn't do anything.
				SCVScrollPosVertRect = (const RECT *)&pThis[34];
				if (SCVScrollPosVertRect->top >= pt.y)
					Game_CSimCityView_OnVScroll(pThis, SB_PAGEUP, 0, (DWORD *)pThis[19]);
				else
					Game_CSimCityView_OnVScroll(pThis, SB_PAGEDOWN, 0, (DWORD *)pThis[19]);
			}
		}
		else if (!pThis[63]) {
			bOverrideTickPlacementHighlight = TRUE;
			hWnd = SetCapture((HWND)pThis[7]);
			Game_CWnd_FromHandle(hWnd);
			wCurrentTileCoordinates = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);;
			if (wCurrentTileCoordinates >= 0) {
				wTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
				wPreviousTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
				wTileCoordinateY = wCurrentTileCoordinates >> 8;
				wPreviousTileCoordinateY = wCurrentTileCoordinates >> 8;
				wGameScreenAreaX = (WORD)pt.x;
				wGameScreenAreaY = (WORD)pt.y;
				pThis[63] = 1;
				pThis[62] = 1;
				if (wCityMode)
					Game_CityToolMenuAction(nFlags, pt);
				else
					Game_MapToolMenuAction(nFlags, pt);
			}
		}
	}
}

extern "C" void __stdcall Hook_CSimcityView_WM_MOUSEMOVE(UINT nFlags, POINT pt) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	// pThis[62] = dwSCVLeftMouseButtonDown
	// pThis[63] = dwSCVLeftMouseDownInGameArea
	// pThis[64] = dwSCVCursorInGameArea
	// pThis[65] = SCVMousePoint

	*(POINT *)&pThis[65] = pt;
	if (pThis[63]) {
		wCurrentTileCoordinates = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		if (wCurrentTileCoordinates >= 0) {
			wTileCoordinateX = (uint8_t)wCurrentTileCoordinates;
			wTileCoordinateY = wCurrentTileCoordinates >> 8;
			if (wPreviousTileCoordinateX != wTileCoordinateX ||
				wPreviousTileCoordinateY != wTileCoordinateY) {
				if ((int)abs(wGameScreenAreaX - pt.x) > 1 ||
					((int)abs(wGameScreenAreaY - pt.y) > 1)) {
					pThis[64] = 1;
					if ((nFlags & MK_LBUTTON) != 0) {
						if (pThis[62]) {
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
	DWORD *pThis;
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

	pThis = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);	// TODO: is this necessary or can we just dereference pCSimcityView?
	Game_TileHighlightUpdate(pThis);
	iTileStartX = 400;
	iTileStartY = 400;
	iCurrMapToolGroupNoHotKey = wCurrentMapToolGroup;
	iCurrMapToolGroupWithHotKey = iCurrMapToolGroupNoHotKey;
	if ((nFlags & MK_CONTROL) != 0)
		iCurrMapToolGroupWithHotKey = MAPTOOL_GROUP_CENTERINGTOOL;
	if (iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_CENTERINGTOOL)
		pThis[62] = 0;
	do {
		iTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
		if (iTileCoords < 0)
			break;
		iTileTargetX = (uint8_t)iTileCoords;
		iTileTargetY = iTileCoords >> 8;
		if (iTileTargetX >= GAME_MAP_SIZE || iTileTargetY < 0)
			break;
		if ((nFlags & MK_SHIFT) != 0 && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_TREES && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_FOREST) {
			pThis[62] = 1;
			break;
		}
		if (iTileStartX != iTileTargetX || iTileStartY != iTileTargetY) {
			switch (iCurrMapToolGroupWithHotKey) {
			case MAPTOOL_GROUP_BULLDOZER: // Bulldozing, only relevant in the CityToolMenuAction code it seems.
				Game_UseBulldozer(iTileTargetX, iTileTargetY);
				Game_UpdateAreaPortionFill(pThis);
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
				if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_TREES)
					Game_MapToolPlaceTree(iTileTargetX, iTileTargetY);
				else
					Game_MapToolPlaceForest(iTileTargetX, iTileTargetY);
				break;
			case MAPTOOL_GROUP_CENTERINGTOOL: // Center Tool
				Game_GetScreenCoordsFromTileCoords(iTileTargetX, iTileTargetY, &wNewScreenPointX, &wNewScreenPointY);
				Game_SoundPlaySound(&pCSimcityAppThis, SOUND_CLICK);
				dwIsZoomed = *(DWORD *)((char *)pThis + 322);
				if (dwIsZoomed)
					Game_CenterOnNewScreenCoordinates(pThis, wScreenPointX - (wNewScreenPointX >> 1), wScreenPointY - (wNewScreenPointY >> 1));
				else
					Game_CenterOnNewScreenCoordinates(pThis, wScreenPointX - wNewScreenPointX, wScreenPointY - wNewScreenPointY);
				break;
			default:
				break;
			}
		}
		if (iCurrMapToolGroupWithHotKey >= MAPTOOL_GROUP_RAISETERRAIN && iCurrMapToolGroupWithHotKey <= MAPTOOL_GROUP_LEVELTERRAIN)
			break;
		else if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_CENTERINGTOOL) {
			Game_UpdateAreaCompleteColorFill(pThis);
			hWnd = (HWND)pThis[7];
			UpdateWindow(hWnd);
			break;
		}
		Game_UpdateAreaPortionFill(pThis);
		iTileStartX = iTileTargetX;
		iTileStartY = iTileTargetY;
		hWnd = (HWND)pThis[7];
		UpdateWindow(hWnd);
	} while (Game_CSimcityViewMouseMoveOrLeftClick(pThis, &pt));
	if (iCurrMapToolGroupNoHotKey != iCurrMapToolGroupWithHotKey) {
		wCurrentCityToolGroup = iCurrMapToolGroupNoHotKey;
	}
}

extern "C" void __stdcall Hook_LoadCursorResources() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_LoadCursorResources)(void *) = (void(__thiscall *)(void *))0x4255A0;

	HDC hDC;

	hDC = GetDC(0);
	pThis[57] = GetDeviceCaps(hDC, HORZRES);
	ReleaseDC(0, hDC);
	H_LoadCursorResources(pThis);
}

extern "C" int __stdcall Hook_StartupGraphics() {
	HDC hDC_One, hDC_Two;
	int iPlanes, iBitsPixel, iBitRate;
	PALETTEENTRY *p_pEnt;
	colStruct *pCol;
	DWORD pvIn;
	DWORD pvOut;
	LOGPAL plPal;

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

extern "C" void __stdcall Hook_CityToolBarToolMenuDisable() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_CityToolBarToolMenuDisable)(void *) = (void(__thiscall *)(void *))0x4237F0;

	ToggleFloatingStatusDialog(FALSE);

	H_CityToolBarToolMenuDisable(pThis);
}

extern "C" void __stdcall Hook_CityToolBarToolMenuEnable() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_CityToolBarToolMenuEnable)(void *) = (void(__thiscall *)(void *))0x423860;

	ToggleFloatingStatusDialog(TRUE);

	H_CityToolBarToolMenuEnable(pThis);
}

extern "C" void __stdcall Hook_ShowViewControls() {
	void(__thiscall *H_MainFrameToggleStatusControlBar)(void *, BOOL) = (void(__thiscall *)(void *, BOOL))0x4021A8;
	void(__thiscall *H_CFrameWndRecalcLayout)(void *, int) = (void(__thiscall *)(void *, int))0x4BB23A;

	int &iSCAProgramStep = *(int *)0x4C7334;
	BOOL &bRedraw = *(BOOL *)0x4E62B4;

	DWORD *pMainFrm;
	DWORD *pSCView;
	DWORD *pSCVScrollBarHorz;
	DWORD *pSCVScrollBarVert;
	DWORD *pSCVStatic;

	pMainFrm = (DWORD *)pCWndRootWindow;
	pSCView = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
	pSCVScrollBarHorz = (DWORD *)pSCView[20];
	pSCVScrollBarVert = (DWORD *)pSCView[19];
	pSCVStatic = (DWORD *)pSCView[21];
	if (!bRedraw) {
		bRedraw = TRUE;
		if (iSCAProgramStep == ONIDLE_STATE_RETURN_12 || !wCityMode)
			H_MainFrameToggleStatusControlBar(pMainFrm, FALSE);
		else {
			if (!CanUseFloatingStatusDialog())
				H_MainFrameToggleStatusControlBar(pMainFrm, TRUE);
		}
		H_CFrameWndRecalcLayout(pMainFrm, TRUE);
		ShowWindow((HWND)pSCVScrollBarHorz[7], SW_SHOWNORMAL);
		ShowWindow((HWND)pSCVScrollBarVert[7], SW_SHOWNORMAL);
		ShowWindow((HWND)pSCVStatic[7], SW_SHOWNORMAL);
	}
}

extern "C" void __stdcall Hook_MainFrameUpdateSections() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_CCityToolBar_RefreshToolBar)(void *) = (void(__thiscall *)(void *))0x401000;
	void(__thiscall *H_CMapToolBarResetControls)(void *) = (void(__thiscall *)(void *))0x401140;
	UINT(__thiscall *H_CMyToolBarGetButtonStyle)(void *, int) = (UINT(__thiscall *)(void *, int))0x401235;
	void(__thiscall *H_CMainFrameDisableCityToolBarButton)(void *, int) = (void(__thiscall *)(void *, int))0x4016DB;
	void(__thiscall *H_CMyToolBarSetButtonStyle)(void *, int nIndex, UINT nStyle) = (void(__thiscall *)(void *, int, UINT))0x402306;
	void(__thiscall *H_CMyToolBarInvalidateButton)(void *, int) = (void(__thiscall *)(void *, int))0x4029C8;
	void(__thiscall *H_CCityToolBarUpdateControls)(void *, BOOL) = (void(__thiscall *)(void *, BOOL))0x402A68;
	CMFC3XString *(__thiscall *H_CStringOperatorSet)(CMFC3XString *, char *) = (CMFC3XString *(__thiscall *)(CMFC3XString *, char *))0x4A2E6A;
	CMFC3XMenu *(__stdcall *H_CMenuFromHandle)(HMENU) = (CMFC3XMenu *(__stdcall *)(HMENU))0x4A7427;
	int(__thiscall *H_CMenuAttach)(CMFC3XMenu *, HMENU) = (int(__thiscall *)(CMFC3XMenu *, HMENU))0x4A7483;
	BOOL(__thiscall *H_CMenuDestroyMenu)(CMFC3XMenu *) = (BOOL(__thiscall *)(CMFC3XMenu *))0x4A74FB;

	CMFC3XString *cityToolGroupStrings = (CMFC3XString *)0x4C94C8;
	HINSTANCE &hGameModule = *(HINSTANCE *)0x4CE8C8;
	int *dwGrantedItems = (int *)0x4E9A10;
	DWORD *DisplayLayer = (DWORD *)0x4E9E48;

	HWND hDlgItem;
	DWORD *pMapToolBar;
	DWORD *pCityToolBar;
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
	CMFC3XString *cityToolString;
	CMFC3XString *pTargMFCString;

	hDlgItem = GetDlgItem((HWND)pThis[68], 120); // Status - GoTo button.
	pMapToolBar = &pThis[233];
	if (!wCityMode)
		H_CMapToolBarResetControls(pMapToolBar);
	pCityToolBar = &pThis[102];
	H_CCityToolBarUpdateControls(pCityToolBar, FALSE);
	ToggleGotoButton(hDlgItem, FALSE);
	if (wCityMode == GAME_MODE_CITY) {
		if (wCurrentCityToolGroup == CITYTOOL_GROUP_DISPATCH) {
			wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
			H_CCityToolBarUpdateControls(pCityToolBar, FALSE);
		}
		H_CMainFrameDisableCityToolBarButton(pThis, CITYTOOL_BUTTON_DISPATCH);
		H_CMyToolBarInvalidateButton(pCityToolBar, CITYTOOL_BUTTON_DISPATCH);
	}
	else if (wCityMode != GAME_MODE_DISASTER)
		goto REFRESHMENUGRANTS;
	if (wCityMode == GAME_MODE_DISASTER)
		ToggleGotoButton(hDlgItem, TRUE);
	if (!dwGrantedItems[CITYTOOL_GROUP_REWARDS]) {
		H_CMainFrameDisableCityToolBarButton(pThis, CITYTOOL_BUTTON_REWARDS);
		H_CMyToolBarInvalidateButton(pCityToolBar, CITYTOOL_BUTTON_REWARDS);
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
	ButtonStyle = H_CMyToolBarGetButtonStyle(pCityToolBar, iCityToolBarButton);
	H_CMyToolBarSetButtonStyle(pCityToolBar, iCityToolBarButton, ButtonStyle | 0x100);
	for (nLayer = LAYER_UNDERGROUND; nLayer < LAYER_COUNT; ++nLayer) {
		if (DisplayLayer[nLayer])
			nStyle = 0x102;
		else
			nStyle = 2;
		nIndex = CITYTOOL_BUTTON_DISPLAYUNDERGROUND - nLayer;
		H_CMyToolBarSetButtonStyle(pCityToolBar, nIndex, nStyle);
	}
REFRESHMENUGRANTS:
	pMenu = (CMFC3XMenu *)&pCityToolBar[57];
	H_CMenuDestroyMenu(pMenu);
	hMenu = LoadMenuA(hGameModule, (LPCSTR)136);
	H_CMenuAttach(pMenu, hMenu);
	for (nPos = CITYTOOL_BUTTON_BULLDOZER; nPos < CITYTOOL_BUTTON_SIGNS; ++nPos) {
		if (dwGrantedItems[nPos]) {
			hSubMenu = GetSubMenu(pMenu->m_hMenu, nPos);
			pSubMenu = H_CMenuFromHandle(hSubMenu);
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
				// calculation here is citytoolbuttongroup * maxsubtools (12 per group), this sets it to TOOL_GROUP_REWARDS.
				cityToolString = &cityToolGroupStrings[CITYTOOL_GROUP_REWARDS*MAX_CITY_SUBTOOLS];
				do {
					// (1 << nReward) bit-shifted result of the nReward count.
					nRewardBit = (1 << nReward);
					if ((nRewardBit & dwGrantedItems[nPos]) != 0) {
						if (nPos == CITYTOOL_BUTTON_REWARDS && !nGranted) {
							pThis[220] = nReward;
							nGranted = 1;
							pTargMFCString = (CMFC3XString *)&pCityToolBar[74];
							H_CStringOperatorSet(pTargMFCString, cityToolString[nReward].m_pchData);
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
	H_CCityToolBar_RefreshToolBar(pCityToolBar);
}

// Hook for the scenario description popup
__declspec(naked) void Hook_402B4E(const char* szDescription, int a2, void* cWnd) {
	__asm push ecx

	if (szDescription && strlen(szDescription))
		scScenarioDescription = szDescription;
	dwScenarioStartDays = dwCityDays;
	dwScenarioStartPopulation = dwCityPopulation;
	wScenarioStartXVALTiles = *(WORD*)0x4C93B4;		// XXX - needs variable declaration in sc2k_1996.h
	dwScenarioStartTrafficDivisor = pBudgetArr[10].iCurrentCosts + pBudgetArr[11].iCurrentCosts + pBudgetArr[12].iCurrentCosts + 1;		// XXX - this should be a descriptive macro

	__asm pop ecx
	GAMEJMP(0x42DC20);
}

static BOOL L_OnCmdMsg(void *pThis, UINT nID, int nCode, void *pExtra, void *pHandler, void *dwRetAddr) {
	BOOL(__thiscall *H_CCmdTargetOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4A280C;
	BOOL(__thiscall *H_CDialogOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4A6C8E;
	BOOL(__thiscall *H_CDocumentOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4AE16C;
	BOOL(__thiscall *H_CViewOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4AE83A;
	BOOL(__thiscall *H_CMDIFrameWndOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4B780A;
	BOOL(__thiscall *H_CFrameWndOnCmdMsg)(void *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo) = (BOOL(__thiscall *)(void *, UINT, int, void *, void *))0x4B9C9A;

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
				ReloadDefaultTileSet1996();
				return TRUE;
			}
		}
		else if (nCode == _CN_COMMAND_UI) {
			// As far as potential handling here goes - tread carefully;
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - _CN_COMMAND_UI\n", pThis, nID, nCode, pExtra, pHandler);
		}
		return H_CFrameWndOnCmdMsg(pThis, nID, nCode, pExtra, pHandler);
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

	return H_CCmdTargetOnCmdMsg(pThis, nID, nCode, pExtra, pHandler);
}

extern "C" BOOL __stdcall Hook_WndOnCommand(WPARAM wParam, LPARAM lParam) {
	DWORD *pThis;

	__asm mov[pThis], ecx

	DWORD *(__stdcall *H_CWndFromHandlePermanent)(HWND) = (DWORD *(__stdcall *)(HWND))0x4A3BFD;
	CMFC3XTestCmdUI *(__thiscall *H_CTestCmdUIConstruct)(void *) = (CMFC3XTestCmdUI *(__thiscall *)(void *))0x4A5315;
	BOOL(__thiscall *H_CWndSendChildNotifyLastMsg)(void *, LRESULT *) = (BOOL(__thiscall *)(void *, LRESULT *))0x4A6091;
	DWORD *(__stdcall *H_AfxGetThreadState)() = (DWORD *(__stdcall *)())0x4C0730;

	DWORD *pWndHandle;
	CMFC3XTestCmdUI testCmd;

	// AFX_THREAD_STATE -> DWORD:
	// var[40] -> m_hLockoutNotifyWindow

	UINT nID = LOWORD(wParam);
	HWND hWndCtrl = (HWND)lParam;
	int nCode = HIWORD(wParam);

	if (nID == 0)
		return FALSE;

	if (hWndCtrl == NULL) {
		H_CTestCmdUIConstruct(&testCmd);
		testCmd.m_nID = nID;
		L_OnCmdMsg(pThis, nID, _CN_COMMAND_UI, &testCmd, 0, _ReturnAddress());
		if (!testCmd.m_bEnabled)
			return TRUE;
		nCode = _CN_COMMAND;
	}
	else {
		if ((HWND)H_AfxGetThreadState()[40] == (HWND)pThis[7])
			return TRUE;

		pWndHandle = H_CWndFromHandlePermanent(hWndCtrl);
		if (pWndHandle != NULL && H_CWndSendChildNotifyLastMsg(pWndHandle, 0))
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
	NEWJMP((LPVOID)0x49FE18, Hook_FileDialogDoModal);

	// Fix the sign fonts
	VirtualProtect((LPVOID)0x4E7267, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4E7267 = 'a';
	VirtualProtect((LPVOID)0x44DC42, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC42 = 5;
	VirtualProtect((LPVOID)0x44DC4F, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC4F = 10;

	// Hook CSimcityApp::OnQuit
	VirtualProtect((LPVOID)0x401753, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401753, Hook_SimcityAppOnQuit);

	InstallSpriteAndTileSetSimCity1996Hooks();

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

	// Expand sound buffers and load higher quality sounds from DLL resources
	LoadReplacementSounds();

	// Install music engine hooks
	InstallMusicEngineHooks();

	// Hook status bar updates for the status dialog implementation
	InstallStatusHooks_SC2K1996();

	// Hooks for CCityToolBar::ToolMenuDisable and CCityToolBar::ToolMenuEnable
	// Both of which are called when a modal CGameDialog is opened.
	// The purpose in this case will be to temporarily alter the parent
	// of the status widget (if it is in floating mode) so interaction is disabled.
	VirtualProtect((LPVOID)0x402937, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402937, Hook_CityToolBarToolMenuDisable);
	VirtualProtect((LPVOID)0x401519, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401519, Hook_CityToolBarToolMenuEnable);

	// Hook for ShowViewControls
	VirtualProtect((LPVOID)0x4021D5, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021D5, Hook_ShowViewControls);

	// Hook for CMainFrame::UpdateSections
	VirtualProtect((LPVOID)0x40131B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40131B, Hook_MainFrameUpdateSections);

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

	// Hook CWnd::OnCommand
	VirtualProtect((LPVOID)0x4A5352, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A5352, Hook_WndOnCommand);

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

	// Hook for CCmdUI::Enable
	VirtualProtect((LPVOID)0x4A296A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A296A, Hook_CCmdUI_Enable);

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

