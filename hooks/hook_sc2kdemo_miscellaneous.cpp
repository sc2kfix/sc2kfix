// sc2kfix hooks/hook_sc2kdemo_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Hooks for the Interactive Demo... AAAA!!

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

#define MISCHOOK_DEMO_DEBUG_OTHER 1
#define MISCHOOK_DEMO_DEBUG_MENU 2

#define MISCHOOK_DEMO_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_DEMO_DEBUG
#define MISCHOOK_DEMO_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_demo_debug = MISCHOOK_DEMO_DEBUG;

static DWORD dwDummy;

#pragma warning(disable : 6387)
// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_Demo_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 2 && hMainMenu)
		return hMainMenu;
	return LoadMenuA(hInstance, lpMenuName);
}
#pragma warning(default : 6387)

void __declspec(naked) Hook_Demo_SimcityApp_InitInstanceFix() {
	CSimcityAppDemo *pThis;

	__asm mov [pThis], ecx

	// Originally 'SW_MAXIMIZE' was (pThis->m_nCmdShow | SW_MAXIMIZE)
	// resulting in a value of 11.
	// m_nCmdShow by default appeared to have been set to
	// SW_SHOWNA (8).

	pThis->m_nCmdShow = SW_MAXIMIZE;
	ShowWindow(pThis->m_pMainWnd->m_hWnd, pThis->m_nCmdShow);
	UpdateWindow(pThis->m_pMainWnd->m_hWnd);
	DragAcceptFiles(pThis->m_pMainWnd->m_hWnd, TRUE);
	GameMain_WinApp_EnableShellOpen_Demo(pThis);

	// The exact purposes of these are unclear.
	// It seems as if they're only used here
	// and/or during a case of "documents" being
	// freed (whether these were for debugging
	// or leftover cases aren't clear).
	dwUnknownInitVarOne_Demo = 0;
	bCSimcityDocSC2InUse_Demo = FALSE;
	bCSimcityDocSCNInUse_Demo = FALSE;

	// In the full version the equivalent hooks
	// would have the command line processing here.

	__asm mov ecx, [pThis]
	GAMEJMP(0x4762E2)
}

static void OpenMainDialog_SC2KDemo() {
	CSimcityAppDemo *pSCApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis_Demo;
	if (pSCApp) {
		pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass_Demo(pSCApp);
		// Let's not allow this or any trickery if the main view window is valid.
		if (!pSCView) {
			if (pMainFrm) {
				// Adjust the program step so it will re-launch the main dialogue.
				pSCApp->iSCAProgramStep = DEMO_ONIDLE_STATE_PENDINGACTION;
				pSCApp->wSCAInitDialogFinishLastProgramStep = DEMO_ONIDLE_STATE_MAPMODE; // Value of 0 - reset it.
				pSCApp->dwSCASetNextStep = TRUE;
			}
		}
	}
}

static BOOL L_OnCmdMsg_Demo(CMFC3XWnd *pThis, UINT nID, int nCode, void *pExtra, void *pHandler, void *dwRetAddr) {
	// Normally internally there'd be the class hierarchy regarding inheritence
	// (which isn't present here).
	//
	// 0x4A15ED - with CFrameWnd - use CFrameWnd::OnCmdMsg
	//
	// All other flagged address references have thus far gracefully
	// gone to CCmdTarget::OnCmdMsg (which is the non-overridden virtual call).
	//
	// If others also require specific handling, checkout the returned address
	// and see where it specifically happens to originate.
	if ((DWORD)dwRetAddr == 0x4A15ED) {
		if (nCode == _CN_COMMAND) {
			switch (nID) {
			case IDM_MAIN_FILE_OPENMAINDIALOG:
				OpenMainDialog_SC2KDemo();
				return TRUE;
			}
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
		}
		else if (nCode == _CN_COMMAND_UI) {
			// As far as potential handling here goes - tread carefully;
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - _CN_COMMAND_UI\n", pThis, nID, nCode, pExtra, pHandler);
		}
		return GameMain_FrameWnd_OnCmdMsg_Demo((CMFC3XFrameWnd *)pThis, nID, nCode, pExtra, pHandler);
	}
	else if ((DWORD)dwRetAddr == 0x48CEE7) {
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

	return GameMain_CmdTarget_OnCmdMsg_Demo(pThis, nID, nCode, pExtra, pHandler);
}

extern "C" BOOL __stdcall Hook_Demo_Wnd_OnCommand(WPARAM wParam, LPARAM lParam) {
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
		GameMain_TestCmdUI_Construct_Demo(&testCmd);
		testCmd.m_nID = nID;
		L_OnCmdMsg_Demo(pThis, nID, _CN_COMMAND_UI, &testCmd, 0, _ReturnAddress());
		if (!testCmd.m_bEnabled)
			return TRUE;
		nCode = _CN_COMMAND;
	}
	else {
		if (GameMain_AfxGetThreadState_Demo()->m_hLockoutNotifyWindow == pThis->m_hWnd)
			return TRUE;

		pWndHandle = GameMain_Wnd_FromHandlePermanent_Demo(hWndCtrl);
		if (pWndHandle != NULL && GameMain_Wnd_SendChildNotifyLastMsg_Demo(pWndHandle, 0))
			return TRUE;
	}

	return L_OnCmdMsg_Demo(pThis, nID, nCode, 0, 0, _ReturnAddress());
}

// Hook CCmdUI::Enable so we can programmatically enable and disable menu items reliably
extern "C" void __stdcall Hook_Demo_CmdUI_Enable(BOOL bOn) {
	CMFC3XCmdUI *pThis;
	__asm mov [pThis], ecx

	HWND hWndParent;
	CMFC3XWnd *pWndParent;
	HWND hNextDlgTabItem;
	CMFC3XWnd *pNextDlgTabItem;
	HWND hWndFocus;
	CSimcityAppDemo *pSCApp;
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
			pWndParent = GameMain_Wnd_FromHandle_Demo(hWndParent);
			hNextDlgTabItem = GetNextDlgTabItem(pWndParent->m_hWnd, pThis->m_pOther->m_hWnd, 0);
			pNextDlgTabItem = GameMain_Wnd_FromHandle_Demo(hNextDlgTabItem);
			hWndFocus = SetFocus(pNextDlgTabItem->m_hWnd);
			GameMain_Wnd_FromHandle_Demo(hWndFocus);
		}
		EnableWindow(pThis->m_pOther->m_hWnd, bOn);
	}
	pThis->m_bEnableChanged = TRUE;

	pSCApp = &pCSimcityAppThis_Demo;
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

void InstallMiscHooks_SC2KDemo(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_demo_debug = DEBUG_FLAGS_EVERYTHING;

	// Install critical Windows API hooks
	*(DWORD*)(0x4D7D2C) = (DWORD)Hook_Demo_LoadMenuA;

	InstallRegistryPathingHooks_SC2KDemo();

	// Fix the 'Arial" font
	VirtualProtect((LPVOID)0x4CF130, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4CF130, 0, 6);
	memcpy_s((LPVOID)0x4CF130, 6, "Arial", 6);
	VirtualProtect((LPVOID)0x4403A3, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4403A3 = 5;
	VirtualProtect((LPVOID)0x4403AE, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4403AE = 10;

	// Hook for CSimcityApp::InitInstance to bypass and fix:
	// - Set m_nCmdShow to 'SW_MAXIMIZE' by default rather than
	//    'SW_SHOWNA' - while adding the 'SW_MAXIMIZE' bit during
	//     the ShowWindow() call - this resolves the lack of a main
	//     window when the program was executed via a launcher or
	//     the command line.
	// (This also accounts for the initial ShowWindow case)
	VirtualProtect((LPVOID)0x476256, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x476256, Hook_Demo_SimcityApp_InitInstanceFix);

	// Set the initial program state to DEMO_ONIDLE_STATE_DISPLAYMAXIS
	VirtualProtect((LPVOID)0x475C18, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x475C18 = DEMO_ONIDLE_STATE_DISPLAYMAXIS;

	// Fix the Maxis Presents logo not being shown
	VirtualProtect((LPVOID)0x4D2984, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4D2984, 0, 13);
	memcpy_s((LPVOID)0x4D2984, 13, "presnts.bmp", 13);

	// Hook CWnd::OnCommand
	VirtualProtect((LPVOID)0x48D687, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x48D687, Hook_Demo_Wnd_OnCommand);

	// Hook for CCmdUI::Enable
	VirtualProtect((LPVOID)0x48F1C7, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x48F1C7, Hook_Demo_CmdUI_Enable);

	// Add more buttons to SC2K's menus
	hMainMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(2));
	if (hMainMenu) {
		// File menu -> Open Main Dialog
		HMENU hFilePopup;
		MENUITEMINFO miiFilePopup;
		miiFilePopup.cbSize = sizeof(MENUITEMINFO);
		miiFilePopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hMainMenu, 0, TRUE, &miiFilePopup) && mischook_demo_debug & MISCHOOK_DEMO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		hFilePopup = miiFilePopup.hSubMenu;
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_demo_debug & MISCHOOK_DEMO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_STRING, IDM_MAIN_FILE_OPENMAINDIALOG, "&Open Main Dialog") && mischook_demo_debug & MISCHOOK_DEMO_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}

		if (mischook_demo_debug & MISCHOOK_DEMO_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated main menu.\n");
	}

skipmainmenu:
	;
	// Experiment with nullifying the timer during the first load.
	//VirtualProtect((LPVOID)0x47685E, 10, PAGE_EXECUTE_READWRITE, &dwDummy);
	//BYTE bTimePatch[10] = { 0xC7, 0x05, 0x68, 0x6A, 0x4B, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
	//memcpy((LPVOID)0x47685E, bTimePatch, 10);
}
