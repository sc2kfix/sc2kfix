// sc2kfix hooks/hook_sc2k1995_miscellaneous.cpp: 1995 CD Collection hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Hooks for the 1995 CD Collection - these are minimal cases to avoid certain
// interface-breaking conditions and other chosen cases.
//
// The 1995 CD Collection is still deprecated, we'd strongly recommend the 1996 Special Edition!

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

#define MISCHOOK_1995_DEBUG_OTHER 1
#define MISCHOOK_1995_DEBUG_MENU 2

#define MISCHOOK_1995_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_1995_DEBUG
#define MISCHOOK_1995_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_1995_debug = MISCHOOK_1995_DEBUG;

static DWORD dwDummy;

extern "C" int __stdcall Hook_1995_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	if (hInstance == hSC2KAppModule) {
		switch (uID) {
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
		default:
			break;
		}
	}
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

#pragma warning(disable : 6387)
// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_1995_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 2 && hMainMenu)
		return hMainMenu;
	return LoadMenuA(hInstance, lpMenuName);
}
#pragma warning(default : 6387)

void __declspec(naked) Hook_1995_LoadCityCancelFix(void) {
	CMainFrame *pThis;

	__asm mov [pThis], ecx

	if (pThis) {
		Game_MainFrame_ToggleToolBars_1995(pThis, TRUE);
		Game_UpdateSectionsAndResetWindowMenu_1995();
	}

	__asm mov ecx, [pThis]
	GAMEJMP(0x42E73C)
}

static void L_ProcessCmdLine_1995(CSimcityAppPrimary *pSCApp) {
	char szFileArg[MAX_PATH + 1], szFileExt[16 + 1];
	std::string str;
	int iArgc;
	LPWSTR *pArgv;

	memset(szFileArg, 0, sizeof(szFileArg));
	memset(szFileExt, 0, sizeof(szFileExt));

	str = pSCApp->m_lpCmdLine;
	std::wstring wStr(str.begin(), str.end());

	pArgv = CommandLineToArgvW(wStr.c_str(), &iArgc);
	if (pArgv) {
		// When a drag-and-drop occurs (over the main program or a shortcut), the file argument
		// is always at the very end; the processing will only accept that detail as well.
		WideCharToMultiByte(CP_UTF8, 0, pArgv[iArgc - 1], -1, szFileArg, MAX_PATH, NULL, NULL);
		_strlwr_s(szFileArg, sizeof(szFileArg) - 1);

		free(pArgv);
	}

	if (strlen(szFileArg) > 0) {
		if (L_IsPathValid(szFileArg)) {
			// We only need the file extension in this case.
			_splitpath_s(szFileArg, NULL, 0, NULL, 0, NULL, 0, szFileExt, sizeof(szFileExt) - 1);

			if (strlen(szFileExt) > 0) {
				if (_stricmp(szFileExt, ".sc2") == 0) {
					pSCApp->dwSCACMDLineLoadMode = GAME_MODE_CITY;
					GameMain_String_OperatorSet_1995(&pSCApp->dwSCACStringTargetTypePath, szFileArg);
				}
				if (_stricmp(szFileExt, ".scn") == 0) {
					pSCApp->dwSCACMDLineLoadMode = GAME_MODE_DISASTER;
					GameMain_String_OperatorSet_1995(&pSCApp->dwSCACStringTargetTypePath, szFileArg);
				}
			}
		}
	}
}

void __declspec(naked) Hook_1995_SimcityApp_InitInstanceFix() {
	CSimcityAppPrimary *pThis;

	__asm {
		mov ecx, ebx
		mov [pThis], ecx
	}

	// Originally 'SW_MAXIMIZE' was (pThis->m_nCmdShow | SW_MAXIMIZE)
	// resulting in a value of 11.
	// m_nCmdShow by default appeared to have been set to
	// SW_SHOWNA (8).

	pThis->m_nCmdShow = SW_MAXIMIZE;
	ShowWindow(pThis->m_pMainWnd->m_hWnd, pThis->m_nCmdShow);
	UpdateWindow(pThis->m_pMainWnd->m_hWnd);
	DragAcceptFiles(pThis->m_pMainWnd->m_hWnd, TRUE);
	GameMain_WinApp_EnableShellOpen_1995(pThis);

	// The exact purposes of these are unclear.
	// It seems as if they're only used here
	// and/or during a case of "documents" being
	// freed (whether these were for debugging
	// or leftover cases aren't clear).
	dwUnknownInitVarOne_1995 = 0;
	bCSimcityDocSC2InUse_1995 = FALSE;
	bCSimcityDocSCNInUse_1995 = FALSE;

	L_ProcessCmdLine_1995(pThis);

	__asm {
		mov ecx, [pThis]
		mov ebx, ecx
	}
	GAMEJMP(0x405965)
}

extern "C" void __stdcall Hook_1995_SimcityApp_OnQuit(void) {
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
	iReqRet = Game_SimcityApp_ExitRequester_1995(pThis, pThis->dwSCASysCmdOnQuitVar);
	if (iReqRet != IDCANCEL) {
		if (iReqRet == IDYES)
			Game_SimcityApp_SaveCity_1995(pThis);
		Game_MainFrame_ToggleToolBars_1995((CMainFrame *)pThis->m_pMainWnd, 0);
		GameMain_WinApp_OnAppExit_1995(pThis);
		return;
	}
	pThis->dwSCAMainFrameDestroyVar = 0;
	pThis->dwSCAOnQuitSuspendSim = 0;
}

static void OpenMainDialog_SC2K1995() {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis_1995;
	if (pSCApp) {
		pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass_1995(pSCApp);
		// Let's not allow this or any trickery if the main view window is valid.
		if (!pSCView) {
			if (pMainFrm) {
				// Adjust the program step so it will re-launch the main dialogue.
				pSCApp->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
				pSCApp->wSCAInitDialogFinishLastProgramStep = ONIDLE_STATE_MAPMODE; // Value of 0 - reset it.
				pSCApp->dwSCASetNextStep = TRUE;
			}
		}
	}
}

static BOOL L_OnCmdMsg_1995(CMFC3XWnd *pThis, UINT nID, int nCode, void *pExtra, void *pHandler, void *dwRetAddr) {
	// Normally internally there'd be the class hierarchy regarding inheritence
	// (which isn't present here).
	//
	// 0x4B7F74 - with CFrameWnd - use CFrameWnd::OnCmdMsg
	//
	// All other flagged address references have thus far gracefully
	// gone to CCmdTarget::OnCmdMsg (which is the non-overridden virtual call).
	//
	// If others also require specific handling, checkout the returned address
	// and see where it specifically happens to originate.
	if ((DWORD)dwRetAddr == 0x4B7F74) {
		if (nCode == _CN_COMMAND) {
			switch (nID) {
			case IDM_MAIN_FILE_OPENMAINDIALOG:
				OpenMainDialog_SC2K1995();
				return TRUE;
			}
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
		}
		else if (nCode == _CN_COMMAND_UI) {
			// As far as potential handling here goes - tread carefully;
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - _CN_COMMAND_UI\n", pThis, nID, nCode, pExtra, pHandler);
		}
		return GameMain_FrameWnd_OnCmdMsg_1995((CMFC3XFrameWnd *)pThis, nID, nCode, pExtra, pHandler);
	}
	else if ((DWORD)dwRetAddr == 0x4A3AA6) {
		// This is the equivalent of 0x4A4BB2 from the 1996 function area.
		// Any WM_COMMAND message that would end up calling CWnd::WindowProc().
		// Useful to handle additions in various dialogues.
		// For any standard cases it falls-through as normal to CCmdTarget::OnCmdMsg().
		if (nCode == _CN_COMMAND) {
			//ConsoleLog(LOG_DEBUG, "::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
		}
	}
	else {
		// Leaving this particular debug notice enabled without any flags.
		// It is particularly important that this is picked up if any
		// strange cases appear. Thus far it hasn't.. but you never know.
		ConsoleLog(LOG_DEBUG, "?::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
	}

	return GameMain_CmdTarget_OnCmdMsg_1995(pThis, nID, nCode, pExtra, pHandler);
}

extern "C" BOOL __stdcall Hook_1995_Wnd_OnCommand(WPARAM wParam, LPARAM lParam) {
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
		GameMain_TestCmdUI_Construct_1995(&testCmd);
		testCmd.m_nID = nID;
		L_OnCmdMsg_1995(pThis, nID, _CN_COMMAND_UI, &testCmd, 0, _ReturnAddress());
		if (!testCmd.m_bEnabled)
			return TRUE;
		nCode = _CN_COMMAND;
	}
	else {
		if (GameMain_AfxGetThreadState_1995()->m_hLockoutNotifyWindow == pThis->m_hWnd)
			return TRUE;

		pWndHandle = GameMain_Wnd_FromHandlePermanent_1995(hWndCtrl);
		if (pWndHandle != NULL && GameMain_Wnd_SendChildNotifyLastMsg_1995(pWndHandle, 0))
			return TRUE;
	}

	return L_OnCmdMsg_1995(pThis, nID, nCode, 0, 0, _ReturnAddress());
}

// Hook CCmdUI::Enable so we can programmatically enable and disable menu items reliably
extern "C" void __stdcall Hook_1995_CmdUI_Enable(BOOL bOn) {
	CMFC3XCmdUI *pThis;
	__asm mov [pThis], ecx

	HWND hWndParent;
	CMFC3XWnd *pWndParent;
	HWND hNextDlgTabItem;
	CMFC3XWnd *pNextDlgTabItem;
	HWND hWndFocus;
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;

	if (pThis->m_pMenu != NULL) {
		if (pThis->m_pSubMenu != NULL)
			return;

		EnableMenuItem(pThis->m_pMenu->m_hMenu, pThis->m_nIndex, MF_BYPOSITION |
			(bOn ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else {
		if (!bOn && (GetFocus() == pThis->m_pOther->m_hWnd)) {
			hWndParent = GetParent(pThis->m_pOther->m_hWnd);
			pWndParent = GameMain_Wnd_FromHandle_1995(hWndParent);
			hNextDlgTabItem = GetNextDlgTabItem(pWndParent->m_hWnd, pThis->m_pOther->m_hWnd, 0);
			pNextDlgTabItem = GameMain_Wnd_FromHandle_1995(hNextDlgTabItem);
			hWndFocus = SetFocus(pNextDlgTabItem->m_hWnd);
			GameMain_Wnd_FromHandle_1995(hWndFocus);
		}
		EnableWindow(pThis->m_pOther->m_hWnd, bOn);
	}
	pThis->m_bEnableChanged = TRUE;

	pSCApp = &pCSimcityAppThis_1995;
	if (pSCApp) {
		pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
		if (pMainFrm) {
			// This section has been added to account for menu items that aren't handled
			// natively (yet).

			// Ensure that the "Open Main Dialog" item is always enabled.
			EnableMenuItem(GetMenu(pMainFrm->m_hWnd), IDM_MAIN_FILE_OPENMAINDIALOG, MF_BYCOMMAND | MF_ENABLED);
		}
	}
}

void InstallMiscHooks_SC2K1995(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_1995_debug = DEBUG_FLAGS_EVERYTHING;

	// Install critical Windows API hooks
	*(DWORD*)(0x4EEC54) = (DWORD)Hook_1995_LoadStringA;
	*(DWORD*)(0x4EED5C) = (DWORD)Hook_1995_LoadMenuA;

	InstallRegistryPathingHooks_SC2K1995();

	// Wipe out the call to UpdateSectionsAndResetWindowMenu() here
	// otherwise it results in a crash if you cancel the LoadCity dialogue
	// prior to starting any game.
	VirtualProtect((LPVOID)0x42E746, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x42E746, 0x90, 5);

	// Wipe out the 'push'
	VirtualProtect((LPVOID)0x42E732, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x42E732, 0x90, 2);
	// Detour and add the UpdateSectionsAndResetWindowMenu() call
	// within this function (this replicates the behaviour in the
	// 1996SE version).
	VirtualProtect((LPVOID)0x42E737, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x42E737, Hook_1995_LoadCityCancelFix);

	// Fix the 'Arial" font
	VirtualProtect((LPVOID)0x4E6234, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E6234, 0, 6);
	memcpy_s((LPVOID)0x4E6234, 6, "Arial", 6);
	VirtualProtect((LPVOID)0x44D5C2, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44D5C2 = 5;
	VirtualProtect((LPVOID)0x44D5CF, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44D5CF = 10;

	// Hook for CSimcityApp::InitInstance to bypass and fix:
	// a) Set m_nCmdShow to 'SW_MAXIMIZE' by default rather than
	//    'SW_SHOWNA' - while adding the 'SW_MAXIMIZE' bit during
	//     the ShowWindow() call - this resolves the lack of a main
	//     window when the program was executed via a launcher or
	//     the command line.
	// b) The command line processing
	//    (Win9x ShellOpen path conversion to DOS-type, of which didn't
	//    occur from NT 5.0 and beyond).
	// (This also accounts for the initial ShowWindow case)
	VirtualProtect((LPVOID)0x405813, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x405813, Hook_1995_SimcityApp_InitInstanceFix);

	// Hook CSimcityApp::OnQuit
	VirtualProtect((LPVOID)0x401749, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401749, Hook_1995_SimcityApp_OnQuit);

	// Set the initial program state to ONIDLE_STATE_DISPLAYMAXIS
	VirtualProtect((LPVOID)0x4051DD, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4051DD = ONIDLE_STATE_DISPLAYMAXIS;

	// Fix the Maxis Presents logo not being shown
	VirtualProtect((LPVOID)0x4E5120, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E5120, 0, 13);
	memcpy_s((LPVOID)0x4E5120, 13, "presnts.bmp", 13);

	// Hook CWnd::OnCommand
	VirtualProtect((LPVOID)0x4A4246, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A4246, Hook_1995_Wnd_OnCommand);

	// Hook for CCmdUI::Enable
	VirtualProtect((LPVOID)0x4A185E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4A185E, Hook_1995_CmdUI_Enable);

	// Add more buttons to SC2K's menus
	hMainMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(2));
	if (hMainMenu) {
		// File menu -> Open Main Dialog
		HMENU hFilePopup;
		MENUITEMINFO miiFilePopup;
		miiFilePopup.cbSize = sizeof(MENUITEMINFO);
		miiFilePopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hMainMenu, 0, TRUE, &miiFilePopup) && mischook_1995_debug & MISCHOOK_1995_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		hFilePopup = miiFilePopup.hSubMenu;
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_1995_debug & MISCHOOK_1995_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_STRING, IDM_MAIN_FILE_OPENMAINDIALOG, "&Open Main Dialog") && mischook_1995_debug & MISCHOOK_1995_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}

		if (mischook_1995_debug & MISCHOOK_1995_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated main menu.\n");
	}

skipmainmenu:
	
	// Adjust the Save File dialog type criterion
	VirtualProtect((LPVOID)0x4E6314, 32, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E6314, 0, 32);
	memcpy_s((LPVOID)0x4E6314, 32, "SimCity Files (*.sc2)|*.sc2||", 30);
}
