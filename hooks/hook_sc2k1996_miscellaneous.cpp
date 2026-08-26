// sc2kfix hooks/hook_sc2k1996_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

// 2026-08-18 (@araxestroy): This file is going to definitely be split up into several components
// as part of the Big Cleanup. I don't think I could live with myself if I touched pretty much
// every other part of the project but left this file in the state that it's in.

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
#define MISCHOOK_DEBUG_BUILDSUBFRAMES 128
#define MISCHOOK_DEBUG_CHEAT 256

#define MISCHOOK_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

DLGPROC lpNewCityAfxProc = NULL;
char szTempCityName[32] = { 0 };
char szTempMayorName[MAX_LABEL_LEN + 1] = { 0 };

DLGPROC lpMainDialogAfxProc = NULL;
HWND hwndMainDialog_SC2K1996 = NULL;
BOOL bMainDialogUpdateState = FALSE;

// Used for newspaper testing commands
extern bool bForceNewspaperDisplay;
extern int iForceNewspaperArg0;
extern int iForceNewspaperArg1;

// Override some strings that have egregiously bad grammar/capitalization.
// Maxis fail English? That's unpossible!
extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	return L_LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

#pragma warning(disable : 6387)
// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 2 && hMainMenu)
		return hMainMenu;
	if ((DWORD)lpMenuName == 3 && hGameMenu)
		return hGameMenu;
	if ((DWORD)lpMenuName == 223 && hDebugMenu)
		return hDebugMenu;
	return LoadMenuA(hInstance, lpMenuName);
}
#pragma warning(default : 6387)

extern "C" int __stdcall Hook_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	return L_MessageBoxA(hWnd, lpText, lpCaption, uType);
}

int L_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	HWND MBhWnd;
	int ret;

	// This has been added so if a '0' parameter was passed for hWnd it'll
	// be set for the active window, otherwise the message box could end up
	// behind the window and re-focusing would become a bit of a pain.
	MBhWnd = (hWnd) ? hWnd : GetActiveWindow();
	
	ToggleFloatingStatusDialog(FALSE);
	ret = MessageBoxA(MBhWnd, lpText, lpCaption, uType);
	ToggleFloatingStatusDialog(TRUE);

	return ret;
}

extern "C" INT_PTR __stdcall Hook_GameDialog_DoModal() {
	CGameDialog *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	BOOL bQueryDialog = ((DWORD)_ReturnAddress() == 0x44D2C3 || (DWORD)_ReturnAddress() == 0x4719E3) ? TRUE : FALSE;
	INT_PTR ret;

	// If we're entering a query dialog, make sure that colour cycling is active in the background
	if (bQueryDialog)
		pSCApp->dwSCABackgroundColourCyclingActive = TRUE;
	ret = GameMain_GameDialog_DoModal(pThis);
	if (bQueryDialog)
		pSCApp->dwSCABackgroundColourCyclingActive = FALSE;

	return ret;
}

extern "C" void __stdcall Hook_GameDialog_OnDestroy() {
	CGameDialog *pThis;

	__asm mov [pThis], ecx

	// Some extra cleanup just to be safe
	if (hWndExt)
		hWndExt = 0;
	GameMain_GameDialog_OnDestroy(pThis);
}

// Process the command line for the game itself
static void L_ProcessCmdLine_1996(CSimcityAppPrimary *pSCApp) {
	char szFileArg[MAX_PATH + 1], szFileExt[16 + 1];
	std::string str;
	int iArgc;
	LPWSTR *pArgv;

	memset(szFileArg, 0, sizeof(szFileArg));
	memset(szFileExt, 0, sizeof(szFileExt));

	// Grab the command line and argv-ize it for parsing
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

	// Explicitly null-terminate szFileArg to be safe
	szFileArg[sizeof(szFileArg) - 1] = 0;

	// Figure out if a city or scenario was passed to the game and mark it to be loaded if so
	if (strlen(szFileArg) > 0) {
		if (L_IsPathValid(szFileArg)) {
			// We only need the file extension in this case.
			_splitpath_s(szFileArg, NULL, 0, NULL, 0, NULL, 0, szFileExt, sizeof(szFileExt) - 1);

			if (strlen(szFileExt) > 0) {
				if (_stricmp(szFileExt, ".sc2") == 0) {
					// Load the city in mayor mode
					pSCApp->dwSCACMDLineLoadMode = GAME_MODE_CITY;
					GameMain_String_OperatorSet(&pSCApp->dwSCACStringTargetTypePath, szFileArg);
				}
				if (_stricmp(szFileExt, ".scn") == 0) {
					// Load the city in disaster mode. If there's no disaster in the save data,
					// the game will automatically flip back to mayor mode.
					pSCApp->dwSCACMDLineLoadMode = GAME_MODE_DISASTER;
					GameMain_String_OperatorSet(&pSCApp->dwSCACStringTargetTypePath, szFileArg);
				}
			}
		}
	}
}

// Hook to fix a few bugs in InitInstance
void __declspec(naked) Hook_SimcityApp_InitInstanceFix() {
	CSimcityAppPrimary *pThis;

	// `this` is in ebx at the point where we're injecting our hook, and ecx is clobberable
	// XXX (araxestroy): do we *need* to clobber ecx here?
	__asm {
		mov ecx, ebx
		mov [pThis], ecx
	}

	// Originally, m_nCmdShow by default appeared to have been set to 8 (SW_SHOWNA), and was being
	// set to (pThis->m_nCmdShow | SW_MAXIMIZE), resulting in a value of 11 (SW_FORCEMINIMIZE).
	pThis->m_nCmdShow = SW_MAXIMIZE;
	ShowWindow(pThis->m_pMainWnd->m_hWnd, pThis->m_nCmdShow);
	UpdateWindow(pThis->m_pMainWnd->m_hWnd);
	DragAcceptFiles(pThis->m_pMainWnd->m_hWnd, TRUE);
	GameMain_WinApp_EnableShellOpen(pThis);

	// The exact purposes of these are unclear. It seems as if they're only used here and/or
	// during a case of "documents" being freed (whether these were for debugging or leftover
	// cases aren't clear).
	dwUnknownInitVarOne = 0;
	bCSimcityDocSC2InUse = FALSE;
	bCSimcityDocSCNInUse = FALSE;

	// Process the command line from the game's perspective for drag-and-drop city loading before
	// returning to the original control path
	L_ProcessCmdLine_1996(pThis);

	__asm {
		mov ecx, [pThis]
		mov ebx, ecx
	}
	GAMEJMP(0x405996)
}

// Fix for the simulation freezing when cancelling out of the save without quitting dialog
extern "C" void __stdcall Hook_SimcityApp_OnQuit(void) {
	CSimcityAppPrimary *pThis;
	__asm mov [pThis], ecx

	int iReqRet;

	// While 'dwSCAMainFrameDestroyVar' is set to 1 various simulation and update
	// aspects are suspended. Originally when "Cancel" was clicked in order to avoid
	// quitting it wouldn't unset the var and consequently the game simulation would
	// remain suspended.
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
	BOOL bOnOverride;

	if (pThis->m_pMenu != NULL) {
		if (pThis->m_pSubMenu != NULL)
			return;

		// Added this here to account for the items we've added.
		// We can override them accordingly here.
		bOnOverride = bOn;
		if (pThis->m_nID == IDM_GAME_WINDOWS_SCENARIOGOALS)
			bOnOverride = (bInScenario) ? TRUE : FALSE;
		else if ((pThis->m_nID >= IDM_DEBUG_MILITARY_DECLINED && pThis->m_nID <= IDM_DEBUG_MILITARY_MISSILESILOS) ||
			(pThis->m_nID >= IDM_DEBUG_THING_CLEAN_PLANES && pThis->m_nID <= IDM_DEBUG_THING_CLEAN_MLDEPLOY))
			bOnOverride = (wCityMode > GAME_MODE_TERRAIN_EDIT) ? TRUE : FALSE;
		else if (pThis->m_nID == IDM_GAME_OPTIONS_SC2KFIXSETTINGS ||
			pThis->m_nID == IDM_GAME_OPTIONS_MODCONFIG ||
			pThis->m_nID == IDM_GAME_FILE_RELOADDEFAULTTILESET ||
			pThis->m_nID == IDM_MAIN_FILE_OPENMAINDIALOG ||
			pThis->m_nID == IDM_DEBUG_SPRITE_DISPLAY ||
			pThis->m_nID == IDM_DEBUG_LABEL_LIST_ORPHANS ||
			pThis->m_nID == IDM_DEBUG_LABEL_CLEAR_ORPHANS)
			bOnOverride = TRUE;
		
		EnableMenuItem(pThis->m_pMenu->m_hMenu, pThis->m_nIndex, MF_BYPOSITION |
			(bOnOverride ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
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
}

// Fix to make sure the main menu can't time out, disappear, and never come back
static void OpenMainDialog_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
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

// Called from CMainFrame::OnCreate at 0x40A2B7
extern "C" void __stdcall Hook_SimcityApp_GetCapabilities(CMainFrame* pMainFrm) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	// XXX (araxestroy): do we even need this anymore? sc2kfix isn't compatible with anything less
	// than Windows 7, and certainly won't even get to this point.
#pragma warning(disable : 4996)
#pragma warning(disable : 28159)
	__int16 wVersion = (WORD)GetVersion();
#pragma warning(default : 28159)
#pragma warning(default : 4996)
	if (LOBYTE(wVersion) <= 3 && (LOBYTE(wVersion) != 3 || HIBYTE(wVersion) < 51)) {
		Game_FailRadio(238);
		pThis->wSCAGameSpeedLOW = GAME_SPEED_PAUSED;
		Game_SimcityApp_OnQuit(pThis);
	}

	// Save the screen resolution and seed the two original non-libc RNGs
	HDC hDC = GetDC(pMainFrm->m_hWnd);
	pThis->iSCAGDCHorzRes = GetDeviceCaps(hDC, HORZRES);
	pThis->iSCAGDCVertRes = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(pMainFrm->m_hWnd, hDC);
	Game_SeedRandomLFSR(GetTickCount32() | 1);
	Game_SeedRandomLCG(GetTickCount32() | 1);

	// Originally there was a call here to trigger
	// the music, however this was causing problems
	// with other music engines during intro video
	// playback (this was also the root cause of
	// track 10001 not being played after the intro
	// or in-flight dialogue).
	pThis->iSCAActiveCursor = GAMECURSOR_ARROW;
	Game_SimcityApp_SetGameCursor(pThis, GAMECURSOR_ARROW, FALSE);
}

// Function prototype: HOOKCB void Hook_SimcityApp_BuildSubFrames_Before(CSimcityAppPrimary *pThis)
// Ignored if bHookStopProcessing == TRUE.
// SPECIAL NOTE: Ignoring this hook on callback results in the game effectively hanging. You have
//   been warned!
std::vector<hook_function_t> stHooks_Hook_SimcityApp_BuildSubFrames_Before;

// Function prototype: HOOKCB void Hook_SimcityApp_BuildSubFrames_GameStartup_After(CSimcityAppPrimary *pThis)
// Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimcityApp_BuildSubFrames_GameStartup_After;

// Function prototype: HOOKCB void Hook_SimcityApp_BuildSubFrames_After(CSimcityAppPrimary *pThis)
// Ignored if bHookStopProcessing == TRUE.
std::vector<hook_function_t> stHooks_Hook_SimcityApp_BuildSubFrames_After;

// Called from CSimcityApp::OnIdle; functions as the basic state machine for the game window.
extern "C" void __stdcall Hook_SimcityApp_BuildSubFrames(void) {
	CSimcityAppPrimary *pThis;
	__asm mov [pThis], ecx

	CMainFrame* pMainFrm = (CMainFrame*)pThis->m_pMainWnd;
	CMainFrame* pMDIFrm = (CMainFrame*)GameMain_MDIFrameWnd_MDIGetActive(pMainFrm, 0);
	CSimcityView* pSCView = NULL;
	CSimcityDoc* pSCDoc = NULL;
	BOOL bOffCycle = FALSE;
	CMFC3XPalette* pActivePal;
	CMovieDialog movDlg;
	BOOL bValidInitialDialogStep;
	static int iReportLimit = 0;
	static BOOL bDialogLooping = FALSE;

	// Process "before" hook
	for (const auto& hook : stHooks_Hook_SimcityApp_BuildSubFrames_Before) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*) = (void(*)(CSimcityAppPrimary*))hook.pFunction;
			fnHook(pThis);
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

	// Fill in pointers to the SCV and SCD classes
	if (pMDIFrm) {
		pSCView = (CSimcityView *)GameMain_FrameWnd_GetActiveView(pMDIFrm);
		pSCDoc = (CSimcityDoc *)GameMain_FrameWnd_GetActiveDocument(pMDIFrm);
	}

	// Toggle animation colour cycling
	if (pThis->dwSCAAnimationOffCycle) {
		pActivePal = Game_SimcityApp_GetActivePalette(pThis);
		Game_ToggleColorCycling(pActivePal, FALSE);
		pThis->dwSCAAnimationOffCycle = FALSE;
		bOffCycle = TRUE;
	}
	if (pThis->dwSCAAnimationOnCycle) {
		pActivePal = Game_SimcityApp_GetActivePalette(pThis);
		Game_ToggleColorCycling(pActivePal, TRUE);
		pThis->dwSCAAnimationOnCycle = FALSE;
	}

	// Run the main state machine for the program step
	bValidInitialDialogStep = (pThis->iSCAMenuDialogStep != ONIDLE_INITIALDIALOG_NONE && pThis->iSCAMenuDialogStep != ONIDLE_INITIALDIALOG_COUNT) ? TRUE : FALSE;
	switch (pThis->iSCAProgramStep) {
		// State 0: Edit New Map ("god mode")
		case ONIDLE_STATE_MAPMODE:
			// All we need to do here is handle the basic cursor stuff, so hand that back off to
			// the game engine to take care of.
			if (pSCView) {
				Game_SimcityView_MaintainCursor(pSCView);
				if (pSCDoc) {
					if (bFrequentUpdates || (!bFrequentUpdates && bOffCycle))
						GameMain_Document_UpdateAllViews(pSCDoc, NULL, SCD_UPDATE_VIEW_UPDATE_DOCURSOR, NULL);
					GameMain_Document_UpdateAllViews(pSCDoc, NULL, SCD_UPDATE_VIEW_CHECKCURSOR, NULL);
				}
				UpdateWindow(pSCView->m_hWnd);
			}
			break;
		
		// State 1: Display "Maxis Presents" logo
		case ONIDLE_STATE_DISPLAYMAXIS:
			// Load the graphic for display
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_DISPLAYMAXIS\n");
				pThis->dwSCASetNextStep = FALSE;
				pThis->dwSCADoStepSkip = FALSE;
				if (!Game_MainFrame_LoadGraphic(pMainFrm, aPresentsBmp))
					GameMain_AfxAbort();
				pThis->dwSCALastTick = GetTickCount32();
			}

			// Set a timer to wait five seconds (interruptable by the user) before moving on
			if (pThis->dwSCADoStepSkip || (GetTickCount32() - pThis->dwSCALastTick) > 5000) {
				pThis->iSCAProgramStep = ONIDLE_STATE_WAITMAXIS;
				pThis->dwSCASetNextStep = TRUE;
			}
			break;

		// State 2: Clean up the Maxis Presents logo
		case ONIDLE_STATE_WAITMAXIS:
			// Unload the graphic and move on to the next step
			if (!Game_MainFrame_DeleteGraphic(pMainFrm, FALSE))
				GameMain_AfxAbort();
			if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
				ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_WAITMAXIS\n");
			pThis->iSCAProgramStep = ONIDLE_STATE_DISPLAYTITLE;
			pThis->dwSCASetNextStep = TRUE;
			break;

		// State 3: Display the SC2K title screen
		case ONIDLE_STATE_DISPLAYTITLE:
			// Load the graphic for display
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_DISPLAYTITLE\n");
				pThis->dwSCASetNextStep = FALSE;
				pThis->dwSCADoStepSkip = FALSE;

				// Attempt an extra cleanup of any leftover MainFrame graphic before loading the
				// title graphic into the buffer
				Game_MainFrame_DeleteGraphic(pMainFrm, FALSE);
				if (!L_LoadAnimatedGraphic_SC2K1996(pMainFrm, aTitlescrBmp))
					GameMain_AfxAbort();
				bCSAMainFrameDirectReleaseCapture = FALSE;
				Game_SimcityApp_SetGameCursor(pThis, 0, FALSE);
				pThis->dwSCALastTick = GetTickCount32();
			}

			// Set a timer to wait five seconds (interruptable by the user) before moving on
			if (pThis->dwSCADoStepSkip || (GetTickCount32() - pThis->dwSCALastTick) > 5000) {
				pThis->iSCAProgramStep = ONIDLE_STATE_DISPLAYREGISTRATION;
				pThis->dwSCASetNextStep = TRUE;
			}
			
			// XXX (araxestroy): needed? return value discarded, no side effects
			if (pMainFrm->dwMFCGraphicsOne)
				Game_SimcityApp_GetActivePalette(pThis);
			break;

		// State 4: "dialog finish"
		case ONIDLE_STATE_DIALOGFINISH:
			// Clean up the title screen graphic
			if (!L_DeleteAnimatedGraphic_SC2K1996(pMainFrm, TRUE))
				GameMain_AfxAbort();
			if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES) {
				if (iReportLimit <= 1 || pThis->wSCAInitDialogFinishLastProgramStep != ONIDLE_STATE_MENUDIALOG)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_DIALOGFINISH: wSCAInitDialogFinishLastProgramStep[%s] bDialogLooping(%c)\n", GetOnIdleStateEnumName(pThis->wSCAInitDialogFinishLastProgramStep), (bDialogLooping && !bValidInitialDialogStep) ? 'Y' : 'N');
			}

			pThis->iSCAProgramStep = pThis->wSCAInitDialogFinishLastProgramStep;
			pThis->wSCAInitDialogFinishLastProgramStep = ONIDLE_STATE_MAPMODE; // Value of 0 - reset it.
			pThis->dwSCASetNextStep = TRUE;
			break;
		
		// State 5: Display registration info
		case ONIDLE_STATE_DISPLAYREGISTRATION:
			// Display the registration info dialog
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_DISPLAYREGISTRATION: wSCAInitDialogFinishLastProgramStep[%s]\n", GetOnIdleStateEnumName(pThis->wSCAInitDialogFinishLastProgramStep));
				pThis->dwSCASetNextStep = FALSE;
				pThis->dwSCADoStepSkip = FALSE;
				if (!Game_MainFrame_LoadOwnerInformation(pMainFrm))
					GameMain_AfxAbort();
				pThis->dwSCALastTick = GetTickCount32();
			}

			// Set a timer to wait five seconds (interruptable by the user) before moving on
			if (pThis->dwSCADoStepSkip || (GetTickCount32() - pThis->dwSCALastTick) > 5000) {
				pThis->iSCAProgramStep = ONIDLE_STATE_CLOSEREGISTRATION;
				pThis->dwSCASetNextStep = TRUE;
			}

			// XXX (araxestroy): needed? return value discarded, no side effects
			if (pMainFrm->dwMFCGraphicsOne)
				Game_SimcityApp_GetActivePalette(pThis);
			break;

		// State 6: Stop displaying registration info
		case ONIDLE_STATE_CLOSEREGISTRATION:
			// Clean up the registration info dialog
			if (!Game_MainFrame_CloseOwnerInformation(pMainFrm))
				GameMain_AfxAbort();
			if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
				ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_CLOSEREGISTRATION\n");

			// Set up the main menu to be the next step
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
			break;

		// State 7: Main menu
		case ONIDLE_STATE_PENDINGACTION:
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES) {
					if (iReportLimit <= 1 || bValidInitialDialogStep)
						ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_PENDINGACTION: bDialogLooping(%c)\n", (bDialogLooping && !bValidInitialDialogStep) ? 'Y' : 'N');
				}

				// Check to see if we have a command-line argument telling us to load a specific
				// saved game or scenario, and set the next state appropriately if so
				if (pThis->dwSCACMDLineLoadMode) {
					if (pThis->dwSCACMDLineLoadMode == GAME_MODE_CITY || pThis->dwSCACMDLineLoadMode == GAME_MODE_DISASTER) {
						pThis->iSCAProgramStep = ONIDLE_STATE_FROMCMDLINE;
						if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
							ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_PENDINGACTION: dwSCACMDLineLoadMode(%u)\n", pThis->dwSCACMDLineLoadMode);
					}
				}
				else {
					// Let's avoid trying to reload the same graphic if it gets stuck in a loop.
					if (pThis->wSCAInitDialogFinishLastProgramStep != ONIDLE_STATE_PENDINGACTION) {
						if (!pMainFrm->dwMFCGraphicsOne && !L_LoadAnimatedGraphic_SC2K1996(pMainFrm, aTitlescrBmp))
							GameMain_AfxAbort();
					}

					// Display the main menu and wait for a response
					pThis->iSCAMenuDialogStep = Game_MainFrame_DoInitialDialog(pMainFrm);
					pThis->iSCAProgramStep = ONIDLE_STATE_MENUDIALOG;
				}
			}
			pThis->dwSCASetNextStep = TRUE;
			break;

		// States 10-13: intermediate states returning from a specific option
		case ONIDLE_STATE_LOADCITY_RETURN:
		case ONIDLE_STATE_NEWCITY_RETURN:
		case ONIDLE_STATE_EDITNEWMAP_RETURN:
		case ONIDLE_STATE_LOADSCENARIO_RETURN:
			if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
				ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_ - : iSCAProgramStep[%s]\n", GetOnIdleStateEnumName(pThis->iSCAProgramStep));

			break;

		// State 14: Return from choosing a main menu option
		case ONIDLE_STATE_MENUDIALOG:
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES) {
					if (iReportLimit <= 1 || bValidInitialDialogStep)
						ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_MENUDIALOG: [%s] bDialogLooping(%c)\n", GetOnIdleInitialDialogEnumName(pThis->iSCAMenuDialogStep), (bDialogLooping && !bValidInitialDialogStep) ? 'Y' : 'N');
				}

				// Determine which button was chosen and act accordingly
				bDialogLooping = FALSE;
				pThis->dwSCASetNextStep = FALSE;
				switch (pThis->iSCAMenuDialogStep) {
					case ONIDLE_INITIALDIALOG_LOADCITY:
						pThis->dwSCAOnInitToggleToolBar = FALSE;
						pThis->iSCAProgramStep = ONIDLE_STATE_LOADCITY_RETURN;
						Game_SimcityApp_LoadCity(pThis);
						break;
					case ONIDLE_INITIALDIALOG_NEWCITY:
						pThis->iSCAProgramStep = ONIDLE_STATE_NEWCITY_RETURN;
						Game_SimcityApp_NewCity(pThis);
						break;
					case ONIDLE_INITIALDIALOG_EDITNEWMAP:
						pThis->iSCAProgramStep = ONIDLE_STATE_EDITNEWMAP_RETURN;
						Game_SimcityApp_EditNewMap(pThis);
						pThis->iSCAProgramStep = ONIDLE_STATE_INGAME;
						break;
					case ONIDLE_INITIALDIALOG_LOADSCENARIO:
						pThis->dwSCAOnInitToggleToolBar = FALSE;
						pThis->iSCAProgramStep = ONIDLE_STATE_LOADSCENARIO_RETURN;
						Game_SimcityApp_LoadScenario(pThis);
						break;
					case ONIDLE_INITIALDIALOG_ONQUIT:
						pThis->dwSCAOnQuitSuspendSim = TRUE;
						Game_SimcityApp_OnQuit(pThis);
						break;
					case ONIDLE_INITIALDIALOG_LOADTILESET:
						Game_SimcityApp_LoadTileset(pThis);
						pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
						break;
					case ONIDLE_INITIALDIALOG_MOVIES:
						// If the "MaxisMovie" window class has been registered successfully in
						// CSimcityApp::InitInstance, open the movie dialog
						if (dwMovieClassRegistered) {
							bKeepPalette = TRUE;
							Game_MovieDialog_Cons(&movDlg, 0);
							GameMain_Dialog_DoModal(&movDlg);
							bKeepPalette = FALSE;
							Game_MovieDialog_Dest(&movDlg);
						}

						pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
						break;
					case ONIDLE_INITIALDIALOG_SC2KFIXSETTINGS:
						ShowSettingsDialog();
						pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
						break;
					default:
						// Under this circumstance we want to open the dialogue
						// again, otherwise it will hit ONIDLE_STATE_DIALOGFINISH
						// and then that's it until the program's restarted.
						bDialogLooping = TRUE;
						pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
						break;
				}

				// Transfer the program step accordingly
				pThis->wSCAInitDialogFinishLastProgramStep = pThis->iSCAProgramStep;
				if (pThis->iSCAProgramStep != ONIDLE_STATE_PENDINGACTION)
					pThis->iSCAProgramStep = ONIDLE_STATE_DIALOGFINISH;
				else
					pThis->dwSCASetNextStep = TRUE;
				
				// Avoid spamming the logs with redundant debug reports when looping
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES) {
					if (bDialogLooping) {
						if (iReportLimit <= 1)
							iReportLimit++;
					}
					else {
						if (iReportLimit > 0)
							iReportLimit = 0;
					}
				}
			}
			break;

		// State 15: Handle command-line loading
		case ONIDLE_STATE_FROMCMDLINE:
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_FROMCMDLINE: dwSCACMDLineLoadMode(%u)\n", pThis->dwSCACMDLineLoadMode);
				pThis->dwSCASetNextStep = FALSE;
				bCSAMainFrameDirectReleaseCapture = FALSE;
				Game_SimcityApp_SetGameCursor(pThis, pThis->iSCAActiveCursor, TRUE);

				SetCapture(pMainFrm->m_hWnd);
				ReleaseCapture();

				// Determine whether we need to load a city or a scenario and call the appropriate
				// function in the game engine
				pThis->dwSCAOnInitToggleToolBar = FALSE;
				if (pThis->dwSCACMDLineLoadMode == GAME_MODE_CITY) {
					pThis->iSCAProgramStep = ONIDLE_STATE_LOADCITY_RETURN;
					L_SimcityApp_LoadCityFromCMDLine(pThis, pThis->dwSCACStringTargetTypePath.m_pchData);
				}
				else {
					pThis->iSCAProgramStep = ONIDLE_STATE_LOADSCENARIO_RETURN;
					L_SimcityApp_LoadScenarioFromCMDLine(pThis, pThis->dwSCACStringTargetTypePath.m_pchData);
				}
			}
			break;

		// State 16: Set up the intro video if possible
		case ONIDLE_STATE_INTROVIDEO:
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_INTROVIDEO\n");
				pThis->dwSCASetNextStep = FALSE;
				pThis->dwSCADoStepSkip = FALSE;
				pThis->dwSCALastTick = GetTickCount32();
				pThis->iSCAProgramStep = ONIDLE_STATE_DISPLAYMAXIS;

				// Check that we aren't set to skip the intro video
				if (!bSkipIntro && !jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO].ToBool()) {
					// Attempt to play the intro video(s), otherwise set the next state to the
					// "skipping in-flight sequence" dialog 
					if (Game_MovieCheck(aIntroASmk) && Game_MovieCheck(aIntroBSmk)) {
						if (!Game_Sound_IsMusicPlaying(pThis->SCASNDLayer))
							Game_SimcityApp_MusicStop(pThis);
						if (Game_MovieCreateWindow()) {
							if (!Game_MovieOpen(aIntroASmk))
								Game_MovieOpen(aIntroBSmk);
							Game_MovieDestroyWindow();
						}
					}
					else
						pThis->iSCAProgramStep = ONIDLE_STATE_DISPLAYINFLIGHT;
				}

				// Wrangle the music engine while this is going on
				if (!Game_Sound_IsMusicPlaying(pThis->SCASNDLayer))
					Game_SimcityApp_MusicPlayNextRefocusSong(pThis);
				pThis->dwSCASetNextStep = TRUE;
			}
			break;

		// State 17: Display the "skipping in-flight sequence" dialog
		case ONIDLE_STATE_DISPLAYINFLIGHT:
			if (pThis->dwSCASetNextStep) {
				if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
					ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_DISPLAYINFLIGHT\n");
				pThis->dwSCASetNextStep = FALSE;
				pThis->dwSCADoStepSkip = FALSE;
				if (!Game_MainFrame_DoInflightDialog(pMainFrm))
					GameMain_AfxAbort();
				pThis->dwSCALastTick = GetTickCount32();
			}

			// Set a timer to wait five seconds (interruptable by the user) before moving on
			if (pThis->dwSCADoStepSkip || (GetTickCount32() - pThis->dwSCALastTick) > 5000) {
				pThis->iSCAProgramStep = ONIDLE_STATE_CLOSEINFLIGHT;
				pThis->dwSCASetNextStep = TRUE;
			}

			// XXX (araxestroy): needed? return value discarded, no side effects
			if (pMainFrm->dwMFCGraphicsOne)
				Game_SimcityApp_GetActivePalette(pThis);
			break;

		// State 18: Close the "skipping in-flight sequence" dialog
		case ONIDLE_STATE_CLOSEINFLIGHT:
			if (!Game_MainFrame_CloseInflightDialog(pMainFrm))
				GameMain_AfxAbort();
			if (mischook_debug & MISCHOOK_DEBUG_BUILDSUBFRAMES)
				ConsoleLog(LOG_DEBUG, "ONIDLE_STATE_CLOSEINFLIGHT\n");
			pThis->iSCAProgramStep = ONIDLE_STATE_DISPLAYMAXIS;
			pThis->dwSCASetNextStep = TRUE;
			break;

		// State -1: Main game loop
		default:
			// Break out of the state machine entirely (thus pausing the game) if the mouse is
			// being dragged with the left mouse button down somewhere
			if (pThis->dwSCADragSuspendSim)
				break;

			// This is where the actual game stuff happens
			if (pSCView) {
				// Run the Thing and disaster ticks if needed and update subtick intervals
				if (bOffCycle)
					Game_SimcityApp_UpdateTick(pThis);

				// Run a simulation tick if we're in city mode and the gmae isn't paused
				if (wCityMode == GAME_MODE_CITY) {
					if (pThis->wSCAGameSpeedLOW != GAME_SPEED_PAUSED) {
						if (pMDIFrm) {
							if (pThis->dwSCASimulationTicking) {
								// Process the actual simulation tick, including calendar advance,
								// budget updates, construction, etc. based on the in-game date
								if (pSCDoc && pSCDoc->pSimEngine)
									Simulation_ProcessTick();

								// Start a disaster if we've been told to do so
								if (wSetTriggerDisasterType)
									Game_SimulationStartDisaster();

								// Clean up any disaster-specific deployments that may have become
								// orphaned for one reason or another if we don't have an ongoing
								// disaster
								DeleteAllDisasterDeploys_SC2K1996();

								// Set the flag to wait before firing off the next simulation tick
								// if we're not on African Swallow speed
								if (pThis->wSCAGameSpeedLOW != GAME_SPEED_AFRICAN_SWALLOW)
									pThis->dwSCASimulationTicking = FALSE;
							}
						}
					}
				}

				// Run a disaster tick if we're in disaster mode
				else if (wCityMode == GAME_MODE_DISASTER) {
					if (pThis->wSCAGameSpeedLOW != GAME_SPEED_PAUSED) {
						if (pThis->dwSCASimulationTicking) {
							Game_SimulationStartDisaster();
							++wIdleCount;
							if (pThis->wSCAGameSpeedLOW != GAME_SPEED_AFRICAN_SWALLOW)
								pThis->dwSCASimulationTicking = FALSE;
						}
					}
				}

				// Run title bar and view updates
				if (pSCDoc && pSCDoc->pSimEngine) {
					if (pThis->wSCAGameSpeedLOW != GAME_SPEED_PAUSED) {
						BOOL bUpdateGameView = (wCityMode == GAME_MODE_DISASTER && !bOffCycle) ? FALSE : TRUE;
						// Moved the title and view update calls out of the SimulationProcessTick()
						// function so they always occur at the end to ensure everything is updated
						// and to account for the new mode '3' for preserving the "placement preview"
						// active cursor during granular updates.
						//
						// When disaster mode is active we only want to instigate an update when
						// 'OffCycle' is FALSE in order to not have a negative effect on the disaster
						// animations (Without this being done the animations are rather fast).
						//
						// When "Frequent Updates" are disabled, only have it send a view update when
						// bOffCycle is TRUE.
						if (bFrequentUpdates && bUpdateGameView)
							Game_SimcityDoc_UpdateDocumentTitle(pSCDoc);
						if ((bFrequentUpdates && bUpdateGameView) || (!bFrequentUpdates && bOffCycle))
							GameMain_Document_UpdateAllViews(pSCDoc, NULL, SCD_UPDATE_VIEW_UPDATE_DOCURSOR, NULL);
					}
					GameMain_Document_UpdateAllViews(pSCDoc, NULL, SCD_UPDATE_VIEW_CHECKCURSOR, NULL);
				}

				// Call for a window update
				UpdateWindow(pSCView->m_hWnd);
			}
			break;
	}

	// Process "after" hook
	for (const auto& hook : stHooks_Hook_SimcityApp_BuildSubFrames_After) {
		bHookStopProcessing = FALSE;
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*) = (void(*)(CSimcityAppPrimary*))hook.pFunction;
			fnHook(pThis);
		}
		if (bHookStopProcessing)
			goto BAIL;
	}

BAIL:
	return;
}

static void RandomizeCityTerrainVariables(void) {
	if (GetAsyncKeyState(VK_SHIFT)) {
		// Fully random slider values
		bCityHasRiver = rand() & 1;
		bCityHasOcean = rand() & 1;
		wCityTerrainSliderHills = rand() % 48;
		wCityTerrainSliderWater = rand() % 48;
		wCityTerrainSliderTrees = rand() % 48;
	}
	else {
		// Weighted towards more usable landmass, but more varied than default
		bCityHasRiver = (rand() & 3) == 3 ? 0 : 1;		// 75% chance of having a river
		bCityHasOcean = rand() & 1;						// 50% chance of having an ocean
		wCityTerrainSliderHills = rand() % 25 + 3;		// 3-27 out of 47
		wCityTerrainSliderWater = rand() % 32 + 1;		// 1-32 out of 47
		wCityTerrainSliderTrees = rand() % 36 + 7;		// 7-42 out of 47
	}
}

// Function prototype: HOOKCB void Hook_OnNewCity_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: I cannot for the life of me remember why I added this. It will almost certainly
//   be replaced within the next three months (comment added 2025-08-15).
std::vector<hook_function_t> stHooks_Hook_OnNewCity_Before;

static BOOL CALLBACK Hook_NewCityDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	CSimcityView *pSCView;
	static bool bAborting = false;
	RECT rc;

	switch (message) {
	case WM_INITDIALOG:
		bAborting = false;
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis);
		SendMessage(GetDlgItem(hwndDlg, 119), WM_SETFONT, (WPARAM)hFontMSSansSerifRegular8, TRUE);
		SetFocus(GetDlgItem(hwndDlg, 1));

		// Set WS_EX_LAYERED on our window object, since we need that for transparency and can't
		// do that in the MFC dialog creation function
		SetWindowLong(hwndDlg, GWL_EXSTYLE, GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwndDlg, 0, 255, LWA_ALPHA);

		// Clear out the tooltip cache for this dialog
		DestroyStoredTooltips(storedToolTips, hwndDlg);
		
		// Button tooltips
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 1),
			"Finalizes your city settings and starts the game.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 20),
			"Generates a new map for your city. A larger variety of terrain is available than in the vanilla SimCity 2000 start game dialog.\n\n"
			"Hold Shift while clicking this to increase the depth of the splines being reticulated.");

		// Difficulty selection tooltips
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 109),
			"Start a game on Easy difficulty.\n"
			"Modifiers:\n"
			" - $20,000 starting cash\n"
			" - Slightly increased industrial demand\n"
			" - Eight years before disasters can occur, and reduced chance of disasters");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 1001),
			"Start a game on Easy difficulty.\n"
			"Modifiers:\n"
			" - $20,000 starting cash\n"
			" - Slightly increased industrial demand\n"
			" - Eight years before disasters can occur, and reduced chance of disasters");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 110),
			"Start a game on Medium difficulty.\n"
			"Modifiers:\n"
			" - $10,000 starting cash\n"
			" - Baseline industrial demand\n"
			" - Five years before disasters can occur, and a moderate chance of disasters");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 1002),
			"Start a game on Medium difficulty.\n"
			"Modifiers:\n"
			" - $10,000 starting cash\n"
			" - Baseline industrial demand\n"
			" - Five years before disasters can occur, and a moderate chance of disasters");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 111),
			"Start a game on Hard difficulty.\n"
			"Modifiers:\n"
			" - $10,000 bond at 3% APR\n"
			" - Slightly decreased industrial demand\n"
			" - Two and a half years before disasters can occur, and an increased chance of disasters");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 1003),
			"Start a game on Hard difficulty.\n"
			"Modifiers:\n"
			" - $10,000 bond at 3% APR\n"
			" - Slightly decreased industrial demand\n"
			" - Two and a half years before disasters can occur, and an increased chance of disasters");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 1010),
			"Hover over a date to see the difference between starting years.");

		// Year selection tooltips
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 104),
			"Start the game in 1900.\n"
			"Modifiers:\n"
			" - No forced unlocks.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 105),
			"Start the game in 1950.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment plants unlocked.\n"
			" - 50% chance of natural gas power plants being unlocked.\n"
			" - 5% chance of nuclear power plants being unlocked.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 106),
			"Start the game in 2000.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment and desalination plants unlocked.\n"
			" - Natural gas, nuclear, wind, and solar power plants unlocked.\n"
			" - 50% chance of Plymouth arcologies being unlocked.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, 107),
			"Start the game in 2050.\n"
			"Modifiers:\n"
			" - Subways, buses, highways, and airports unlocked.\n"
			" - Water treatment and desalination plants unlocked.\n"
			" - Natural gas, nuclear, wind, solar, and micorwave power plants unlocked.\n"
			" - 5% chance of fusion power plants being unlocked.\n"
			" - Plymouth arcologies unlocked.\n"
			" - 50% chance of Forest arcologies being unlocked.");

		// Limit the City name to 30 characters (not 31 - this avoids a rather
		// nasty overrun that can occur if the old limit is hit).
		SendMessage(GetDlgItem(hwndDlg, 101), EM_SETLIMITTEXT, CITY_NAME_LEN, 0);
		SendMessage(GetDlgItem(hwndDlg, 150), EM_SETLIMITTEXT, MAX_LABEL_LEN, 0);

		// Set the default options.
		SetDlgItemText(hwndDlg, 101, "New City");
		SetDlgItemText(hwndDlg, 150, jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME].ToString().c_str());

		Button_SetCheck(GetDlgItem(hwndDlg, 109), BST_CHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, 104), BST_CHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, 108), BST_CHECKED);
		iTerrainCosmeticMode = TERRAIN_COSMETIC_NONE;

		if (!bLegacyTerrainMode) {
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TERRAINCOSMETIC].ToInt() > TERRAIN_COSMETIC_NONE)
				SetWindowText(GetDlgItem(hwndDlg, 117), "WARNING: A specific 'Forced Terrain Mode' is set. Once the city has started, the selected 'Terrain Type' will be saved but not applied.");
		}

		if (pSCView) {
			Game_SimcityView_DrawHouse(pSCView);
			RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}
		break;
	case WM_DESTROY:
		if (bAborting)
			return 0;

		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis);

		// Set the city name, defaulting to "New City" in case the player didn't enter one
		memset(szTempCityName, 0, sizeof(szTempCityName));
		if (!GetDlgItemText(hwndDlg, 101, szTempCityName, sizeof(szTempCityName)))
			strcpy_s(szTempCityName, sizeof(szTempCityName), "New City");
		GameMain_String_Cons(&pszCityName);
		GameMain_String_OperatorSet(&pszCityName, szTempCityName);

		// Set the XLAB entry for the mayor name, falling back to the default from settings.json
		memset(szTempMayorName, 0, sizeof(szTempMayorName));
		if (!GetDlgItemText(hwndDlg, 150, szTempMayorName, sizeof(szTempMayorName)))
			strcpy_s(szTempMayorName, sizeof(szTempMayorName), jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME].ToString().c_str());
		SetXLABEntry(0, szTempMayorName);

		// Set the difficulty and starting year
		wNationalFedRate = 3;

		if (Button_GetCheck(GetDlgItem(hwndDlg, 109)) == BST_CHECKED) {
			wCityDifficulty = GAME_DIFFICULTY_EASY;
			dwCityFunds = 20000;
		} else if (Button_GetCheck(GetDlgItem(hwndDlg, 110)) == BST_CHECKED) {
			wCityDifficulty = GAME_DIFFICULTY_MEDIUM;
			dwCityFunds = 10000;
		} else if (Button_GetCheck(GetDlgItem(hwndDlg, 111)) == BST_CHECKED) {
			wCityDifficulty = GAME_DIFFICULTY_HARD;
			dwCityFunds = 10000;

			// Manually "issue" a starting bond
			wArrBondData[dwCityBonds++] = wNationalFedRate;
			dwInterestRateSum = wNationalFedRate;
			pBudgetArr[BUDGET_BOND].iCurrentCosts = dwCityBonds;
			pBudgetArr[BUDGET_BOND].iCountMonth[0] = 1;
			pBudgetArr[BUDGET_BOND].iFundingPercent = 10000 * dwInterestRateSum / dwCityBonds;
			pBudgetArr[BUDGET_BOND].iFundMonth[0] = pBudgetArr[BUDGET_BOND].iFundingPercent;
			pBudgetArr[BUDGET_BOND].iYearToDateCost = pBudgetArr[BUDGET_BOND].iFundingPercent * pBudgetArr[BUDGET_BOND].iCurrentCosts;
			pBudgetArr[BUDGET_BOND].iEstimatedCost = 12 * pBudgetArr[BUDGET_BOND].iYearToDateCost;
		}

		wNationalEconomyTrend = wCityDifficulty - 1;

		if (Button_GetCheck(GetDlgItem(hwndDlg, 104)) == BST_CHECKED) {
			wCityStartYear = 1900;
			dwNationalPopulation = 10000;
		} else if (Button_GetCheck(GetDlgItem(hwndDlg, 105)) == BST_CHECKED) {
			wCityStartYear = 1950;
			dwNationalPopulation = 25000;
		} else if (Button_GetCheck(GetDlgItem(hwndDlg, 106)) == BST_CHECKED) {
			wCityStartYear = 2000;
			dwNationalPopulation = 60000;
		} else if (Button_GetCheck(GetDlgItem(hwndDlg, 107)) == BST_CHECKED) {
			wCityStartYear = 2050;
			dwNationalPopulation = 150000;
		}

		dwMapXGRP[GRP_GNP][0] = 3;
		dwMapXGRP[GRP_NATIONALPOP][0] = dwNationalPopulation;

		// Partially randomize the invention/innovation years, then update the toolbar accordingly
		for (int i = 0; i < 17; i++) {
			int iInventionYear = rand() % 20 + iInventionBaseYears[i];
			wCityInventionYears[i] = wCityStartYear;
			if (wCityStartYear < wCityStartYear)
				wCityInventionYears[i] = 0;
		}
		Game_ToolMenuUpdate();

		// Set the label for the ocean if we have one
		if (bCityHasOcean) {
			wNeighborNameIdx[0] = 0;
			strcpy(stNeighborCities, "Ocean");
			dwNeighborPopulation[0] = 0;
			dwNeighborValue[0] = 0;
			dwNeighborFame[0] = 0;
		}

		// Update the titlebar and set up the initial newspaper
		Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
		Game_NewspaperStoryGenerator(NEWSPAPER_TYPE_FOUNDING, 0);

		// Get the selected terrain setting (or randomize it if requested)
		if (Button_GetCheck(GetDlgItem(hwndDlg, 108)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = TERRAIN_COSMETIC_NONE;
		else if (Button_GetCheck(GetDlgItem(hwndDlg, 112)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = TERRAIN_COSMETIC_GREY;
		else if (Button_GetCheck(GetDlgItem(hwndDlg, 113)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = TERRAIN_COSMETIC_GREEN;
		else if (Button_GetCheck(GetDlgItem(hwndDlg, 114)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = TERRAIN_COSMETIC_COLD;
		else if (Button_GetCheck(GetDlgItem(hwndDlg, 115)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = TERRAIN_COSMETIC_HOT;
		else if (Button_GetCheck(GetDlgItem(hwndDlg, 116)) == BST_CHECKED)
			jsonXFIX["map"]["terrain_cosmetic_mode"] = rand() % 5;

		bUseMapTerrainCosmeticMode = true;

		iTerrainCosmeticMode = GetXFIXTerrainMode();

		if (pSCView) {
			Game_SimcityView_DrawHouse(pSCView);
			RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}

		// Clean up window tooltips
		DestroyStoredTooltips(storedToolTips, hwndDlg);

		// TODO: this should probably be moved to a separate proper hook into the game itself
		for (const auto& hook : stHooks_Hook_OnNewCity_Before) {
			bHookStopProcessing = FALSE;
			if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
				void (*fnHook)(void) = (void(*)(void))hook.pFunction;
				fnHook();
			}
		}

		break;

	// Apply transparency when the right mouse button is held so the player can peek at the map
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		SetLayeredWindowAttributes(hwndDlg, NULL, 32, LWA_ALPHA);

		// Zoom while peeking if the window area is big enough to hold it (more or less) but not
		// so big we're already zoomed in once
		GetWindowRect(pCSimcityAppThis.m_pMainWnd->m_hWnd, &rc);
		if ((rc.right - rc.left > 1600 && rc.bottom - rc.top > 900) &&
			(rc.right - rc.left <= 2000 && rc.bottom - rc.top <= 1080))
			Game_SimcityView_ScaleIn(Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis));
		break;
	case WM_RBUTTONUP:
		SetLayeredWindowAttributes(hwndDlg, NULL, 255, LWA_ALPHA);
		GetWindowRect(pCSimcityAppThis.m_pMainWnd->m_hWnd, &rc);
		if ((rc.right - rc.left > 1600 && rc.bottom - rc.top > 900) &&
			(rc.right - rc.left <= 2000 && rc.bottom - rc.top <= 1080))
			Game_SimcityView_ScaleOut(Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis));
		break;

	case WM_COMMAND:
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis);
		if (HIWORD(wParam) == BN_CLICKED) {
			// Preview the selected terrain type
			switch (LOWORD(wParam)) {
			case IDOK:
				EndDialog(hwndDlg, TRUE);
				break;
			case IDCANCEL:
				bAborting = true;
				EndDialog(hwndDlg, FALSE);
				break;
			case 20:
				// Reticulate some splines
				Game_SimcityDoc_PrepareMap();
				RandomizeCityTerrainVariables();
				Game_SimcityDoc_PrepareData(pCSimcityDoc);
				Game_SimcityView_MakeTerrain(pSCView, bCityHasOcean, bCityHasRiver, wCityTerrainSliderHills, wCityTerrainSliderWater, wCityTerrainSliderTrees);
				// Use zoomlevel 2 if the window area is big enough to hold it
				RECT rc;
				GetWindowRect(pCSimcityAppThis.m_pMainWnd->m_hWnd, &rc);
				if (rc.right - rc.left > 1920 && rc.bottom - rc.top > 1080)
					Game_SimcityView_ScaleIn(pSCView);

				Game_SimcityView_DrawHouse(pSCView);
				RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				break;
			case 108:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_NONE;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			case 112:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_GREY;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			case 113:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_GREEN;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			case 114:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_COLD;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			case 115:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_HOT;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			case 116:
				iTerrainCosmeticMode = TERRAIN_COSMETIC_NONE;
				if (pSCView) {
					Game_SimcityView_DrawHouse(pSCView);
					RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
				break;
			}
		}
	}

	return 0;
}

// Local replacement for CSimcityApp::NewCity
extern "C" void __stdcall L_SimcityApp_NewCity(void) {
	CSimcityAppPrimary* pThis;
	__asm mov [pThis], ecx
	
	// Do some checks to make sure we come out of map edit mode properly
	pThis->dwSCAOnQuitSuspendSim = 0;
	if (dwMapEditingMode)
		Game_MainFrame_ToggleToolBars((CMainFrame*)pThis->m_pMainWnd, 0);
	else if (Game_SimcityApp_CheckActiveGame(pThis) == 2) {
		if (pThis->iSCAProgramStep != ONIDLE_STATE_INGAME) {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = 1;
		}
		return;
	}

	// Start up the initial game state
	pThis->iSCAProgramStep = ONIDLE_STATE_NEWCITY_RETURN;
	pThis->dwSCASetNextStep = 1;
	Game_PrepareGame();
	wCityMode = -1;
	Game_ShowViewControls();
	Game_StartCleanGame();

	// Use zoomlevel 2 if the window area is big enough to hold it
	RECT rc;
	GetWindowRect(pCSimcityAppThis.m_pMainWnd->m_hWnd, &rc);
	if (rc.right - rc.left > 1920 && rc.bottom - rc.top > 1080)
		Game_SimcityView_ScaleIn(Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis));

	// Display the dialog and break out back to the menu loop if it's cancelled
	if (!DialogBoxParam(hSC2KFixModule, (LPCSTR)101, pThis->m_pMainWnd->m_hWnd, Hook_NewCityDialogProc, 0)) {
		pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
		pThis->dwSCASetNextStep = 1;
		return;
	}

	pThis->iSCAProgramStep = ONIDLE_STATE_INGAME;
	pThis->dwSCASetNextStep = 1;
	Game_SimcityApp_AdjustMenus(pThis, GAME_MODE_CITY);
	dwMapEditingMode = 0;
	pThis->dwSCAGameStarted = 1;
	pThis->dwSCAGenerateFirstTimeMap = 0;
}

// Function to resize the main menu dialog to add the "update available" notice
static void SetMainDialogUpdateState(HWND hwndDlg) {
	RECT quitRect, statRect, dlgRect;
	HWND hdlgStaticItem, hdlgQuitItem;
	LONG addCY, dlgCY;

	bMainDialogUpdateState = TRUE;
	if (bMainDialogUpdateState) {
		hdlgQuitItem = GetDlgItem(hwndDlg, 115);
		GetWindowRect(hdlgQuitItem, &quitRect);
		ScreenToClient(hwndDlg, (LPPOINT)&quitRect);
		ScreenToClient(hwndDlg, (LPPOINT)&quitRect.right);
		addCY = quitRect.top;

		hdlgStaticItem = GetDlgItem(hwndDlg, IDC_STATIC_UPDATENOTICE);
		GetWindowRect(hdlgStaticItem, &statRect);
		ScreenToClient(hwndDlg, (LPPOINT)&statRect);
		ScreenToClient(hwndDlg, (LPPOINT)&statRect.right);
		addCY = (((statRect.top * 2) - addCY) + 21) / 3;

		statRect.top += addCY;
		statRect.bottom += addCY;

		SetWindowPos(hdlgStaticItem, HWND_TOP, statRect.left, statRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

		GetWindowRect(hwndDlg, &dlgRect);
		dlgCY = addCY + dlgRect.bottom - dlgRect.top;
		dlgRect.bottom += addCY;
		SetWindowPos(hwndDlg, HWND_TOP, 0, 0, dlgRect.right - dlgRect.left, dlgCY, SWP_NOMOVE | SWP_NOACTIVATE);

		SetDlgItemText(hwndDlg, IDC_STATIC_UPDATENOTICE, UPDATE_STRING);
		ShowWindow(hdlgStaticItem, SW_SHOW);
	}
}

// Hook for the main menu dialog to ensure the "update available" notice is displayed
static BOOL CALLBACK Hook_MainDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		hwndMainDialog_SC2K1996 = hwndDlg;

		if (bUpdateAvailable)
			SetMainDialogUpdateState(hwndDlg);
		break;
	case WM_SC2KFIX_UPDATE:
		if (!bMainDialogUpdateState) {
			if (lParam == 1)
				SetMainDialogUpdateState(hwndDlg);
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
	case 103:
		lpMainDialogAfxProc = lpDialogFunc;
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, Hook_MainDialogProc, dwInitParam);
	case 102:
	case 113:
	case 142:
	case 154:
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	default:
		return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}
}
#pragma warning(default : 6387)

// Hook for the default window procedure to allow additional mouse bindings to function
extern "C" LRESULT __stdcall Hook_DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	CSimcityAppPrimary* pSCApp;
	CSimcityView* pSCView;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);

	// Handle additional mouse buttons. Windows usually sends WM_MBUTTONDOWN for MOUSE3/middle
	// mouse button and WM_XBUTTONDOWN for MOUSE4/MOUSE5, but both can have all different mouse
	// button flags in them.
	if (Msg == WM_MBUTTONDOWN || Msg == WM_XBUTTONDOWN) {
		if (pSCView && hWnd == pSCView->m_hWnd) {
			POINT pt;

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (wParam && MK_MBUTTON)
				GetKeyButtonBinding_SC2K1996(B_KEY_MOUSE_MBUTTON, FALSE, &pt);
			else if (wParam && MK_XBUTTON1)
				GetKeyButtonBinding_SC2K1996(B_KEY_MOUSE_MOUSE4BUTTON, FALSE, &pt);
			else if (wParam && MK_XBUTTON2)
				GetKeyButtonBinding_SC2K1996(B_KEY_MOUSE_MOUSE5BUTTON, FALSE, &pt);
			return TRUE;
		}
	}
	// Handle mousewheel keybinds.
	else if (Msg == WM_MOUSEWHEEL) {
		if (pSCView && hWnd == pSCView->m_hWnd) {
			int nDelta;

			nDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (nDelta < 0)
				GetKeyBinding_SC2K1996(B_KEY_MOUSE_WHEELDOWN, FALSE);
			else if (nDelta > 0)
				GetKeyBinding_SC2K1996(B_KEY_MOUSE_WHEELUP, FALSE);
		}
	}

	return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

extern int iChurchVirus;

// Function prototype: HOOKCB void Hook_PrepareGame_Before(void)
// Called before the vanilla PrepareGame function is called. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_PrepareGame_Before;

// Function prototype: HOOKCB void Hook_PrepareGame_After(void)
// Called after the vanilla PrepareGame function is called. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_PrepareGame_After;

extern "C" void __stdcall Hook_PrepareGame(void) {
	for (const auto& hook : stHooks_Hook_PrepareGame_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)() = (void(*)())hook.pFunction;
			fnHook();
		}
	}

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSimcityView *pSCView;
	CMainFrame *pMainFrm;
	bool bDrawHouse;

	if (!pCSimcityDoc) {
		// This case is hit at program start when the city doc/view first needs to be initialized.
		CMFC3XMultiDocTemplate **pMultiDoc = (CMFC3XMultiDocTemplate **)pSCApp->m_templateList.m_pNodeHead;
		pActiveSimDoc = (CSimcityDoc *)GameMain_MultiDocTemplate_OpenDocumentFile(pMultiDoc[2], NULL, TRUE);
		GameMain_Document_SetTitle(pActiveSimDoc, pStartEngineStr);
	}
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	Game_SimcityView_ResetScreenArea(pSCView);
	bDrawHouse = false;
	switch (pSCApp->iSCAProgramStep) {
		case ONIDLE_STATE_NEWCITY_RETURN:
			Game_SimcityDoc_NewGame(pCSimcityDoc);
			if (!dwMapEditingMode) {
				// Randomize the terrain variation slightly if the setting for that is set. Otherwise, use the
				// defaults.
				if (jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_NEWCITYVARIETY].ToBool())
					RandomizeCityTerrainVariables();
				Game_SimcityView_MakeTerrain(pSCView, bCityHasOcean, bCityHasRiver, wCityTerrainSliderHills, wCityTerrainSliderWater, wCityTerrainSliderTrees);
			}
			break;
		case ONIDLE_STATE_EDITNEWMAP_RETURN:
			if (!pSCApp->dwSCAGenerateFirstTimeMap) {
				if (pSCView) {
					// Rotate the map 90 degrees at a time until wViewRotation is north. We need to do
					// this because SimCity 2000 was programmed by madmen and rotating the viewport is
					// actually accomplished by rotating all the map data in memory.
					if (wViewRotation != VIEWROTATION_NORTH) {
						do
							Game_SimcityView_RotateAntiClockwise(pSCView);
						while (wViewRotation != VIEWROTATION_NORTH);
					}
				}
			}
			Game_StartCleanGame();
			Game_SimcityDoc_PrepareData(pCSimcityDoc);
			if (pSCApp->dwSCAGenerateFirstTimeMap)
				Game_SimcityView_MakeTerrain(pSCView, bCityHasOcean, bCityHasRiver, wCityTerrainSliderHills, wCityTerrainSliderWater, wCityTerrainSliderTrees);
			else
				bDrawHouse = true;
			break;
		case ONIDLE_STATE_LOADCITY_RETURN:
			Game_SimcityDoc_NewGame(pCSimcityDoc);
			break;
		case ONIDLE_STATE_LOADSCENARIO_RETURN:
			bInScenario = TRUE;
			Game_SimcityDoc_NewGame(pCSimcityDoc);
			break;
		default:
			Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
			break;
	}
	if (bDrawHouse)
		Game_SimcityView_DrawHouse(pSCView);
	if (!pSCApp->dwSCASCURK) {
		if (dwShowSCURK) {
			HMENU hMenu = GetMenu(pMainFrm->m_hWnd);
			if (hMenu) {
				HMENU hSubMenu = GetSubMenu(hMenu, 1);
				if (hSubMenu) {
					DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
					DeleteMenu(hSubMenu, 4, MF_BYPOSITION);
				}
			}
			dwShowSCURK = 0;
		}
	}
	Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_BULLDOZER], &cityToolGroupStrings[CITY_MENUTOOL_POS(BULLDOZER_DEMOLISH, CITYTOOL_GROUP_BULLDOZER)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_NATURE], &cityToolGroupStrings[CITY_MENUTOOL_POS(NATURE_TREES, CITYTOOL_GROUP_NATURE)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_DISPATCH], &cityToolGroupStrings[CITY_MENUTOOL_POS(DISPATCH_POLICE, CITYTOOL_GROUP_DISPATCH)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_POWER], &cityToolGroupStrings[CITY_MENUTOOL_POS(POWER_WIRES, CITYTOOL_GROUP_POWER)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_WATER], &cityToolGroupStrings[CITY_MENUTOOL_POS(WATER_PIPES, CITYTOOL_GROUP_WATER)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_REWARDS], &cityToolGroupStrings[CITY_MENUTOOL_POS(REWARDS_MAYORSHOUSE, CITYTOOL_GROUP_REWARDS)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_ROADS], &cityToolGroupStrings[CITY_MENUTOOL_POS(ROADS_ROAD, CITYTOOL_GROUP_ROADS)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_RAIL], &cityToolGroupStrings[CITY_MENUTOOL_POS(RAILS_RAIL, CITYTOOL_GROUP_RAIL)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_PORTS], &cityToolGroupStrings[CITY_MENUTOOL_POS(PORTS_SEAPORT, CITYTOOL_GROUP_PORTS)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_RESIDENTIAL], &cityToolGroupStrings[CITY_MENUTOOL_POS(ZONES_HIGH, CITYTOOL_GROUP_RESIDENTIAL)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_COMMERCIAL], &cityToolGroupStrings[CITY_MENUTOOL_POS(ZONES_HIGH, CITYTOOL_GROUP_COMMERCIAL)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_INDUSTRIAL], &cityToolGroupStrings[CITY_MENUTOOL_POS(ZONES_HIGH, CITYTOOL_GROUP_INDUSTRIAL)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_EDUCATION], &cityToolGroupStrings[CITY_MENUTOOL_POS(EDUCATION_SCHOOL, CITYTOOL_GROUP_EDUCATION)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_SERVICES], &cityToolGroupStrings[CITY_MENUTOOL_POS(SERVICES_POLICE, CITYTOOL_GROUP_SERVICES)]);
	GameMain_String_OperatorCopy(&pMainFrm->dwMFCityToolBar.dwCTBString[CITYTOOL_GROUP_PARKS], &cityToolGroupStrings[CITY_MENUTOOL_POS(PARKS_SMALLPARK, CITYTOOL_GROUP_PARKS)]);
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_BULLDOZER] = BULLDOZER_DEMOLISH;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_NATURE] = NATURE_TREES;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_DISPATCH] = DISPATCH_POLICE;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_POWER] = POWER_WIRES;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_WATER] = WATER_PIPES;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_REWARDS] = REWARDS_MAYORSHOUSE;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_ROADS] = ROADS_ROAD;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_RAIL] = RAILS_RAIL;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_PORTS] = PORTS_SEAPORT;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_RESIDENTIAL] = ZONES_HIGH;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_COMMERCIAL] = ZONES_HIGH;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_INDUSTRIAL] = ZONES_HIGH;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_EDUCATION] = EDUCATION_SCHOOL;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_SERVICES] = SERVICES_POLICE;
	pMainFrm->dwMFCityToolBar.dwCTToolSelection[CITYTOOL_GROUP_PARKS] = PARKS_SMALLPARK;
	Game_CityToolBar_RefreshToolBar(&pMainFrm->dwMFCityToolBar);

	for (const auto& hook : stHooks_Hook_PrepareGame_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)() = (void(*)())hook.pFunction;
			fnHook();
		}
	}
}

extern "C" void __stdcall Hook_StartCleanGame(void) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMainFrame *pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;

	// Clear any information stored for the scenario goals dialogue.
	L_ClearScenarioDetails();

	// Clean up the game state and start the new game/map
	iChurchVirus = -1;
	ResetThingCleanupState_SC2K1996();
	CreateDefaultXFIX();
	iTerrainCosmeticMode = GetXFIXTerrainMode();

	wSetTriggerDisasterType = DISASTER_NONE;
	bNoDisasters = 0;
	bOptionsAutoBudget = 0;
	bOptionsAutoGoto = 1;
	GameMain_String_Empty(&pszCityName);
	GameMain_String_Empty(&strCityFilename);
	GameMain_String_Empty(&strUnusedString);
	wCityProgression = 0;
	dwCityDays = 0;
	wUnknownGameVarOne = 0; // This one doesn't "appear" to be used elsewhere.
	bMilitaryBaseType = MILITARY_BASE_NONE;
	wCityStartYear = 1900;
	wViewRotation = VIEWROTATION_NORTH;
	wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
	wCurrentMapToolGroup = MAPTOOL_GROUP_RAISETERRAIN;
	EditData = 0;
	wClipXlow = 1000;
	wClipYlow = 1000;
	wClipXhigh = 0;
	wClipYhigh = 0;
	LastCursorX = -1;
	dirtyRect.left = -1000;
	dirtyRect.top = -1000;
	dirtyRect.right = -1;
	dirtyRect.bottom = -1;
	bYearEndFlag = 0;
	wNewspaperChoice = 0;
	wCityCenterX = 64;
	wCityCenterY = 64;
	dwCityOldResPopulation = 0;
	switch (GameMain_WinApp_GetProfileIntA(pSCApp, "OPTIONS", "SPEED", CONF_GAME_SPEED_SETTING(GAME_SPEED_PAUSED))) {
		case CONF_GAME_SPEED_SETTING(GAME_SPEED_PAUSED):
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_PAUSED;
			break;
		case CONF_GAME_SPEED_SETTING(GAME_SPEED_TURTLE):
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_TURTLE;
			break;
		case CONF_GAME_SPEED_SETTING(GAME_SPEED_LLAMA):
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_LLAMA;
			break;
		case CONF_GAME_SPEED_SETTING(GAME_SPEED_CHEETAH):
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_CHEETAH;
			break;
		case CONF_GAME_SPEED_SETTING(GAME_SPEED_AFRICAN_SWALLOW):
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_AFRICAN_SWALLOW;
			break;
		default:
			pSCApp->wSCAGameSpeedLOW = GAME_SPEED_PAUSED;
			break;
	}
	if (pMainFrm)
		Game_CityToolBar_UpdateControls(&pMainFrm->dwMFCityToolBar, TRUE);
	// This nested loop here sets TunnelLvls to 0.
	// Originally it was: dwMapALTM[x][y].w &= ~0xFC00; (0x7C00 being ALTM_TUNNELLVLS_BOUNDARY)
	for (int x = 0; x < GAME_MAP_SIZE; ++x) {
		for (int y = 0; y < GAME_MAP_SIZE; ++y) {
			ALTMSetTunnelLevels(x, y, 0);
		}
	}
}

// Hook for updating the game titlebar. Still kind of rough from recompilation.
extern "C" void __stdcall Hook_SimcityDoc_UpdateDocumentTitle() {
	CSimcityDoc *pThis;

	__asm mov [pThis], ecx

	CMFC3XString cStr;
	CSimcityAppPrimary *pSCApp;
	int iCityDayMon;
	int iCityMonth;
	int iCityYear;
	char *pFundStr = NULL;

	GameMain_String_Cons(&cStr);

	pSCApp = &pCSimcityAppThis;
	if (!pSCApp->dwSCAMainFrameDestroyVar) {
		if (!wCityMode)
			GameMain_String_LoadStringA(&cStr, 413); // "Editing Terrain..."
		else {
			if (!pszCityName.m_nDataLength)
				goto GETOUT;
			iCityDayMon = dwCityDays % 25 + 1;
			iCityMonth = dwCityDays / 25 % 12;
			iCityYear = wCityStartYear + dwCityDays / 300;
			if (IsIconic(pSCApp->m_pMainWnd->m_hWnd)) {
				if (dwDisasterActive) {
					if (wCurrentDisasterType <= DISASTER_HURRICANE)
						GameMain_String_LoadStringA(&cStr, dwDisasterStringIndex[wCurrentDisasterType]);
					else
						GameMain_String_Empty(&cStr);
				}
				else
					GameMain_String_Format(&cStr, "%s%s%d", pszCityName.m_pchData, gameStrHyphen, iCityYear);
			}
			else {
				GameMain_String_Empty(&cStr);
				pFundStr = L_GetCurrencyString_SC2K1996(dwCityFunds);
				if (!pFundStr)
					goto GETOUT;
				if (jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND].ToBool() && bFrequentUpdates)
					GameMain_String_Format(&cStr, "%s %d %4d <%s> %s", pSCApp->dwSCApCStringLongMonths[iCityMonth].m_pchData, iCityDayMon, iCityYear, pszCityName.m_pchData, pFundStr);
				else
					GameMain_String_Format(&cStr, "%s %4d <%s> %s", pSCApp->dwSCApCStringShortMonths[iCityMonth].m_pchData, iCityYear, pszCityName.m_pchData, pFundStr);
				free(pFundStr);
				pFundStr = 0;
			}
		}
		GameMain_Document_UpdateAllViews(pThis, NULL, SCD_UPDATE_VIEW_TITLE, (CMFC3XObject *)&cStr);
	}
GETOUT:
	GameMain_String_Dest(&cStr);
}

// Helper function for updating simulation date variables
static void UpdateCityDateAndSeason(BOOL bIncrement) {
	if (bIncrement)
		++dwCityDays;
	wCityCurrentMonth = (dwCityDays / 25) % 12;
	wCityCurrentSeason = (wCityCurrentMonth + 1) % 12 / 3;
	wCityElapsedYears = (WORD)(dwCityDays / 300);
}

// Function prototype: HOOKCB void Hook_SimCalendarAdvance_Before(void)
// Called before the vanilla SimCalendar day simulation. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_Before;

// Function prototype: HOOKCB void Hook_SimCalendarDay23_Before(void)
// Called before the vanilla day 23 code. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarDay23_Before;

// Function prototype: HOOKCB BOOL Hook_ScenarioSuccessCheck(void)
// Cannot be ignored.
// Return value: TRUE if the mod's scenario requirements have been met, FALSE if not.
std::vector<hook_function_t> stHooks_Hook_ScenarioSuccessCheck;

// Function prototype: HOOKCB void Hook_SimCalendarDay23_After(void)
// Called after the vanilla day 23 code. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarDay23_After;

// Function prototype: HOOKCB void Hook_SimCalendarAdvance_After(void)
// Called after the vanilla SimCalendar day simulation. Cannot be ignored.
std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_After;


LARGE_INTEGER SPT_uTickStart, SPT_uTickEnd, SPT_uTicksPerSecond;

NEWENGINE void Simulation_ProcessTick(void) {
	int i;
	DWORD dwMonDay;
	int iStep, iSubStep;
	DWORD dwCityProgressionRequirement;
	BYTE iPaperVal;
	BOOL bScenarioSuccess;
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CNewspaperDialog newsDialog;
	if (!QueryPerformanceFrequency(&SPT_uTicksPerSecond))
		ConsoleLog(LOG_WARNING, "CORE: WTF? QueryPerformanceFrequency errored out 0x%08X.\n", GetLastError());

	pSCApp = &pCSimcityAppThis;
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	UpdateCityDateAndSeason(TRUE);
	dwMonDay = (dwCityDays % 25);
	if (pSCApp->dwSCAGameAutoSave > 0 &&
		!((dwCityDays / 300) % pSCApp->dwSCAGameAutoSave) &&
		!wCityCurrentMonth &&
		!dwMonDay) {
		Game_SimcityApp_CallAutoSave(pSCApp);
	}

	// Call mods for daily processing tasks - before update
	// XXX - should mods be able to entirely override SimCalendar days? Perhaps this is more a
	// theological discussion to be held...
	for (const auto& hook : stHooks_Hook_SimCalendarAdvance_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void(*fnHook)() = (void(*)())hook.pFunction;
			fnHook();
		}
	}

	// Bugfix: recalculate city valuation every day.
	Game_RecalculateCityValue();

	// Force a newspaper if requested (used in console commands)
	if (bForceNewspaperDisplay) {
		// Build the newspaper (also invoking it if it's type 2-5 or 36)
		SafeVirtualProtect((LPVOID)0x47B69C, 1, PAGE_EXECUTE_READWRITE);
		Game_NewspaperStoryGenerator(iForceNewspaperArg0, iForceNewspaperArg1);

		switch (iForceNewspaperArg0) {
		case 2:			// founding
		case 3:			// progression		arg1 = progression level
		case 4:			// invention		arg1 = invention type
		case 5:			// innovation		arg1 = innovation type
		case 36:		// power plant EOL	arg1 = power plant type
			break;
		default:
			// Manually invoke the newspaper as if it's a subscription issue
			Game_NewspaperDialog_Construct(&newsDialog, NULL);
			newsDialog.dwNDPaperChoice = wNewspaperChoice;
			Game_GameDialog_DoModal(&newsDialog);
			Game_NewspaperDialog_Destruct(&newsDialog);
			break;
		}

		// Unset the force options
		bForceNewspaperDisplay = false;
	}

	// Advance the simulation for the current SimCalendar day
	switch (dwMonDay) {
		case 0:
			QueryPerformanceCounter(&SPT_uTickStart);

			// If we're not using the real-time renderer, then update the titlebar on the first of
			// the month (vanilla behaviour)
			if (!bFrequentUpdates)
				Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);

			// Happy new year; here's how broke you are
			if (bYearEndFlag) {
				if (!bOptionsAutoBudget) {
					if (!IsIconic(pMainFrm->m_hWnd))
						Game_Sound_StopSound(pSCApp->SCASNDLayer);
				}
				Game_SimulationPrepareBudgetDialog(0);
			}

			// Calculate any budget updates before any newspapers get displayed
			Game_UpdateBudgetInformation();

			// If the newspaper subscription is selected, craft a newspaper on April 1 and August 1
			// TODO (araxestroy): should be scaled based on some kind of newspaper frequency slider
			// that also ties into game speed
			if (bNewspaperSubscription) {
				if (wCityCurrentMonth == 3 || wCityCurrentMonth == 7) {
					Game_NewspaperDialog_Construct(&newsDialog, NULL);
					newsDialog.dwNDPaperChoice = wNewspaperChoice; // CNewspaperDialog -> CGameDialog -> CDialog; struct position 39 - paperchoice dword var.
					Game_GameDialog_DoModal(&newsDialog);
					Game_NewspaperDialog_Destruct(&newsDialog);
				}
			}

			// Update some variables
			UpdateCityDateAndSeason(FALSE);
			for (i = 0; i < ZONEPOP_COUNT; ++i)
				pZonePops[i] = 0;
			break;
		case 1:
			// Update the city-wide power consumption stats and map/graph data
			Game_SimulationUpdatePowerConsumption();
			break;
		case 2:
			// XXX (araxestroy): document exactly what goes on here once we finish RE
			Game_SimulationPollutionTerrainAndLandValueScan();
			break;
		// NOTE: cases for days 3-18 are in the 'default' case
		case 19:
			// Update traffic stats and map/graph data
			Game_SimulationUpdateMonthlyTrafficData();
			break;
		case 20:
			// Update the city-wide water consumption stats and map/graph data
			Game_SimulationUpdateWaterConsumption();
			break;
		case 21:
			Game_SimulationRCIDemandUpdates();
			Game_SimulationEQ_LE_Processing();
			Game_UpdateGraphData();
			break;
		case 22:
			// Pre-call for Day 23
			for (const auto& hook : stHooks_Hook_SimCalendarDay23_Before) {
				if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
					void(*fnHook)() = (void(*)())hook.pFunction;
					fnHook();
				}
			}
			
			// Check against city milestone progression requirements and grant new milestones
			dwCityProgressionRequirement = dwCityProgressionRequirements[wCityProgression];
			if (dwCityProgressionRequirement) {
				if (dwCityProgressionRequirement < dwCityPopulation) {
					Game_SimcityApp_SetGameCursor(pSCApp, 24, 0);
					// There are only 7 (0-6) progression levels, cast the warning away.
					iPaperVal = (BYTE)wCityProgression++;
					Game_NewspaperStoryGenerator(NEWSPAPER_TYPE_GROWTH, iPaperVal);
					Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
					if (wCityProgression >= 4) {
						if (wCityProgression == 4)
							Game_SimulationProposeMilitaryBase();
						else if (wCityProgression == 5)
							Game_SimulationToggleGrantReward(3, TRUE);
					}
					else
						Game_SimulationToggleGrantReward(wCityProgression - 1, TRUE);
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
				if (scenarioAttrib.dwCitySize > dwCityPopulation && scenarioAttrib.dwCitySize)
					bScenarioSuccess = FALSE;
				if (pBudgetArr[BUDGET_RESFUND].iCurrentCosts < scenarioAttrib.dwResPop)
					bScenarioSuccess = FALSE;
				if (pBudgetArr[BUDGET_COMFUND].iCurrentCosts < scenarioAttrib.dwComPop)
					bScenarioSuccess = FALSE;
				if (pBudgetArr[BUDGET_INDFUND].iCurrentCosts < scenarioAttrib.dwIndPop)
					bScenarioSuccess = FALSE;
				if (dwCityFunds - dwCityBonds < scenarioAttrib.dwCashGoal)
					bScenarioSuccess = FALSE;
				if (dwCityLandValue < scenarioAttrib.dwLandValueGoal)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.wLEGoal > dwCityWorkforceLE)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.wEQGoal > dwCityWorkforceEQ)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.dwPollutionLimit > 0 && dwCityPollution > scenarioAttrib.dwPollutionLimit)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.dwCrimeLimit > 0 && dwCityCrime > scenarioAttrib.dwCrimeLimit)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.dwTrafficLimit > 0 && dwCityTrafficCount > scenarioAttrib.dwTrafficLimit)
					bScenarioSuccess = FALSE;
				if (scenarioAttrib.bFirstBuilding) {
					if (wTileCount[scenarioAttrib.bFirstBuilding] < scenarioAttrib.wFirstBuildTileCnt)
						bScenarioSuccess = FALSE;
				}
				if (scenarioAttrib.bSecondBuilding) {
					if (wTileCount[scenarioAttrib.bSecondBuilding] < scenarioAttrib.wSecondBuildTileCnt)
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
				else if (!--scenarioAttrib.wTimeLimit)
					Game_EventScenarioNotification(GAMEOVER_SCENARIO_FAILURE);
			}

			// Check if the city is bankrupt and impeach the mayor if so
			if (dwCityFunds < -100000)
				Game_EventScenarioNotification(GAMEOVER_BANKRUPT);

			// Post-call for Day 23
			for (const auto& hook : stHooks_Hook_SimCalendarDay23_After) {
				if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
					void(*fnHook)() = (void(*)())hook.pFunction;
					fnHook();
				}
			}

			break;
		case 23:
			// Mode '3' also used here for "placement preview" preservation and blink mitigation.
			if (!bFrequentUpdates)
				GameMain_Document_UpdateAllViews(pCSimcityDoc, NULL, SCD_UPDATE_VIEW_UPDATE_DOCURSOR, NULL);

			// Run updates for various stats subdialogs
			Game_UpdatePopulationDialog();
			Game_UpdateIndustryDialog();
			Game_UpdateGraphDialog();
			break;
		case 24:
			Game_MainFrame_UpdateCityToolBar((CMainFrame *)pSCApp->m_pMainWnd);
			Game_UpdateCityMap();
			Game_UpdateSimNationDialog();
			Game_UpdateWeatherOrDisasterState();

			// Update the performance counters for PERFMON_WHOLEMONTH
			QueryPerformanceCounter(&SPT_uTickEnd);
			if (dwPerfMonEnabled & PERFMON_WHOLEMONTH) {
				if (SPT_uTicksPerSecond.QuadPart && SPT_uTickStart.QuadPart) {
					if (((SPT_uTickEnd.QuadPart - SPT_uTickStart.QuadPart) * 1000000 / SPT_uTicksPerSecond.QuadPart) > 1000)
						ConsoleLog(LOG_INFO, "PERFMON: %s %d - Month took %llu microseconds.\n",
							pSCApp->dwSCApCStringLongMonths[dwCityDays / 25 % 12].m_pchData,
							wCityStartYear + dwCityDays / 300,
							((SPT_uTickEnd.QuadPart - SPT_uTickStart.QuadPart) * 1000000 / SPT_uTicksPerSecond.QuadPart));
				} else {
					ConsoleLog(LOG_INFO, "PERFMON: %s %d - Incomplete month, performance data not available.\n");
				}
			}

			break;
		default:
			// Handle a growth tick day
			if (dwMonDay >= 3 && dwMonDay <= 18) {
				// Update the season on the 13th of the month
				if (dwMonDay == 12)
					UpdateCityDateAndSeason(FALSE);

				// Calculate the step/substep for the growth tick and run the simulation for the
				// day. The step/substep pair is added to the X/Y coordinates in the iterator for
				// each growth tick so the city changes relatively evenly over the month.
				iStep = ((dwMonDay - 3) / 4 % 4);
				iSubStep = (dwMonDay + 1) % 4;
				Simulation_DoGrowthTick(iStep, iSubStep);
				break;
			}
			return;
	}

	// Call mods for daily processing tasks - after update
	for (const auto& hook : stHooks_Hook_SimCalendarAdvance_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void(*fnHook)() = (void(*)())hook.pFunction;
			fnHook();
		}
	}
}

extern bool bFallbackSound;

extern "C" void __stdcall Hook_SimulationStartDisaster(void) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSound *pSound = pSCApp->SCASNDLayer;
	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationStartDisaster(), wDisasterType = %u.\n", _ReturnAddress(), wSetTriggerDisasterType);

	// Check added here to allow for the siren sound to play
	// once more if it ends up being overridden by the
	// bulldozer - an exception is present for if bFallbackSound
	// is set to true (which defaults to the bulldozer sound).
	//
	// An additional check for whether the ActionThingSound is
	// currently SOUND_SIREN but pSound->bSndPlaySound is 0
	// (which will happen if sound was turned off and on again),
	// in which case it gets restarted as well.
	if (dwDisasterActive) {
		if (pSCApp && pSound) {
			if ((pSound->iSNDActionThingSoundID != SOUND_SIREN && !bFallbackSound) || 
				(pSound->iSNDActionThingSoundID == SOUND_SIREN && !pSound->bSNDPlaySound)) {
				if (pSound->iSNDActionThingSoundID == SOUND_BULLDOZER || 
					(pSCApp->dwSCAGameSound && !pSound->bSNDPlaySound)) {
					Game_SimcityApp_StopSounds(pSCApp);
					Game_SimcityApp_SoundPlayActionThingSound(pSCApp, SOUND_SIREN, 5);
				}
			}
		}
	}
	GameMain_SimulationStartDisaster();
	// Added this here so that the siren sound will stop
	// if it (continues to) play erroneously.
	if (!dwDisasterActive)
		Game_SimcityApp_StopSounds(pSCApp);
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

CGraphics *pBaseGraphics = NULL;
LONG nBaseGraphicWidth = 0;
LONG nBaseGraphicHeight = 0;
BYTE *pBaseGraphicLockDIBRes = NULL;

extern "C" CSimcityView *__stdcall Hook_SimcityView_Cons() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMainFrame *pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;

	pThis = GameMain_SimcityView_Cons(pThis);
	if (pThis) {
		pBaseGraphics = new CGraphics();
		if (pBaseGraphics) {
			pBaseGraphics = Game_Graphics_Cons(pBaseGraphics);
			if (pBaseGraphics) {
				pBaseGraphics->CreateWithPalette_SC2K1996(pMainFrm->iMFbiWidth, pMainFrm->iMFbiHeight);
				nBaseGraphicWidth = Game_Graphics_Width(pBaseGraphics);
				nBaseGraphicHeight = Game_Graphics_Height(pBaseGraphics);
				pBaseGraphicLockDIBRes = Game_Graphics_LockDIBBits(pBaseGraphics);
			}
			else {
				delete pBaseGraphics;
				pBaseGraphics = NULL;
			}
		}
	}

	return pThis;
}

extern "C" void __stdcall Hook_SimcityView_ResetScrollViewsAndDeleteGraphics() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	if (pThis) {
		if (pBaseGraphics) {
			pBaseGraphics->DeleteStored_SC2K1996();
			delete pBaseGraphics;
			pBaseGraphics = NULL;
		}
	}

	GameMain_SimcityView_ResetScrollViewsAndDeleteGraphics(pThis);
}

extern "C" void __stdcall Hook_SimcityView_OnUpdate(CMFC3XView *pSender, LPARAM lHint, CMFC3XObject *pHint) {
	CSimcityView *pThis;
	LARGE_INTEGER uTickStart, uTickEnd, uTicksPerSecond;

	__asm mov [pThis], ecx

	char *pBuf;
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	QueryPerformanceFrequency(&uTicksPerSecond);
	QueryPerformanceCounter(&uTickStart);

	if (!pSCApp->dwSCAMainFrameDestroyVar) {
		if (lHint == SCD_UPDATE_VIEW_TITLE) {
			pBuf = GameMain_String_GetBuffer((CMFC3XString *)pHint, 1);
			GameMain_Document_SetTitle(pThis->m_pDocument, pBuf);
		}
		else {
			if (lHint == SCD_UPDATE_VIEW_CHECKCURSOR)
				L_CheckCursor_SC2K1996(pThis);
			else if (lHint == SCD_UPDATE_VIEW_UPDATE_DOCURSOR)
				L_DrawHouse_SC2K1996(pThis, TRUE);
			else if (lHint == SCD_UPDATE_VIEW_UPDATE)
				Game_SimcityView_DrawHouse(pThis);
			else
				Game_SimcityView_MainWindowUpdate(pThis, 0, 0);
			UpdateWindow(pThis->m_hWnd);
		}
	}
	QueryPerformanceCounter(&uTickEnd);
	if (dwPerfMonEnabled & PERFMON_ONUPDATE) {
		if (((uTickEnd.QuadPart - uTickStart.QuadPart) * 1000000 / uTicksPerSecond.QuadPart) > 1000)
			ConsoleLog(LOG_INFO, "PERFMON: %s %2d, %d - OnUpdate took %llu microseconds.\n",
				pSCApp->dwSCApCStringLongMonths[dwCityDays / 25 % 12].m_pchData,
				dwCityDays % 25 + 1,
				wCityStartYear + dwCityDays / 300,
				((uTickEnd.QuadPart - uTickStart.QuadPart) * 1000000 / uTicksPerSecond.QuadPart));
	}
}

// Hook for left mouse button (MOUSE1) handling in the game view
extern "C" void __stdcall Hook_SimcityView_OnLButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CSimcityView *pThis;
	__asm mov [pThis], ecx

	RECT r;

	// It should be noted that the following variable is only unset outside of this function when
	// the menu-specific DoCenterOnPoint() call is made. For the rest of the actions it isn't 
	// unset, which is why after bulldozing or querying a tile in that manner won't result in any
	// action handling from the active toolbar (in the base game).
	if (pThis->dwSCVRightClickMenuOpen)
		pThis->dwSCVRightClickMenuOpen = 0;
	else if (!PtInRect(&pThis->SCVStaticRect, pt)) {
		Game_SimcityView_GetScreenAreaInfo(pThis, &r);
		if (!pThis->dwSCVLeftMouseDownInGameArea) {
			// Set mouse capture and calculate the tile coordinates for the click
			HWND hWnd = SetCapture(pThis->m_hWnd);
			GameMain_Wnd_FromHandle(hWnd);
			wCurrentTileCoordinates = Game_PointToTile((__int16)pt.x, (__int16)pt.y);

			// Ensure the tile coordinates are valid before sending them through to the game
			if (wCurrentTileCoordinates >= 0) {
				// Update the internal coordinate variables
				wTileCoordinateX = LOBYTE(wCurrentTileCoordinates);
				wTileCoordinateY = HIBYTE(wCurrentTileCoordinates);
				wPreviousTileCoordinateX = wTileCoordinateX;
				wPreviousTileCoordinateY = wTileCoordinateY;
				wGameScreenAreaX = (WORD)pt.x;
				wGameScreenAreaY = (WORD)pt.y;
				
				// Flag the mouse button action as in progress and call the appropiate function
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

// Hook for mouse movement in the game view
extern "C" void __stdcall Hook_SimcityView_OnMouseMove(UINT nFlags, CMFC3XPoint pt) {
	CSimcityView *pThis;
	__asm mov [pThis], ecx

	pThis->SCVMousePoint = pt;
	if (pThis->dwSCVLeftMouseDownInGameArea) {
		wCurrentTileCoordinates = Game_PointToTile((__int16)pt.x, (__int16)pt.y);
		if (wCurrentTileCoordinates >= 0) {
			wTileCoordinateX = LOBYTE(wCurrentTileCoordinates);
			wTileCoordinateY = HIBYTE(wCurrentTileCoordinates);
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
}

// Hook for right mouse button (MOUSE2) handling in the game view
extern "C" void __stdcall Hook_SimcityView_OnRButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CSimcityView *pThis;
	__asm mov [pThis], ecx

	GetKeyButtonBinding_SC2K1996(B_KEY_MOUSE_RBUTTON, FALSE, &pt);
}

extern "C" void __stdcall Hook_SimcityView_DoBudget() {
	CSimcityView *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	BOOL bSaveAutoBudget;

	bSaveAutoBudget = bOptionsAutoBudget;
	bOptionsAutoBudget = 0;
	Game_Sound_StopSound(pSCApp->SCASNDLayer);
	Game_SimulationPrepareBudgetDialog(0);
	bOptionsAutoBudget = bSaveAutoBudget;
}

extern "C" void __stdcall Hook_NewspaperDialog_OnInitDialog() {
	CNewspaperDialog *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;

	// Always attempt to stop the sound just prior to
	// the Newspaper dialogue being initialized.
	if (pSCApp)
		Game_SimcityApp_StopSounds(pSCApp);

	GameMain_NewspaperDialog_OnInitDialog(pThis);
}

// Hook to fix cursor weirdness on some high resolution monitors
extern "C" void __stdcall Hook_SimcityApp_LoadCursorResources() {
	CSimcityAppPrimary *pThis;
	__asm mov [pThis], ecx

	// Inform SC2K of the screen resolution before loading cursors so it gets it right
	HDC hDC = GetDC(0);
	pThis->iSCAGDCHorzRes = GetDeviceCaps(hDC, HORZRES);
	ReleaseDC(0, hDC);

	// Fall through to the game engine
	GameMain_SimcityApp_LoadCursorResources(pThis);
}

// Hook for game graphics initialization
// TODO: comment this
extern "C" int __stdcall Hook_StartupGraphics() {
	HDC hDC;
	int iPlanes, iBitsPixel, iBitRate;
	PALETTEENTRY *p_pEnt;
	colStruct *pCol;
	DWORD pvIn;
	DWORD pvOut;
	LOGPAL plPal;

	plPal.wVersion = 0x300;
	plPal.wNumPalEnts = LOCOLORCNT;
	memset(plPal.pPalEnts, 0, sizeof(plPal.pPalEnts));
	hDC = GetDC(0);
	if (!hDC_Global) {
		hDC_Global = CreateCompatibleDC(hDC);
	}

	iPlanes = GetDeviceCaps(hDC, PLANES);
	iBitsPixel = GetDeviceCaps(hDC, BITSPIXEL);
	iBitRate = (iForcedBits > 0) ? iForcedBits : iBitsPixel * iPlanes;

	bHiColor = TRUE;
	if (iBitRate < 16) {
		bHiColor = FALSE;
		if (iBitRate <= 4) {
			bLoColor = TRUE;
			pvIn = SETCOLORTABLE;
			if (Escape(hDC, QUERYESCSUPPORT, 4, (LPCSTR)&pvIn, 0)) {
				p_pEnt = plPal.pPalEnts;
				pCol = rgbLoColor;
				do {
					colTable cT;

					memset(&cT, 0, sizeof(colTable));
					cT.Index = pCol->wPos;
					cT.rgb = RGB(pCol->pe.peRed, pCol->pe.peGreen, pCol->pe.peBlue);

					Escape(hDC, SETCOLORTABLE, 6, (LPCSTR)&cT, &pvOut);
					p_pEnt[pCol->wPos].peRed = pCol->pe.peRed;
					p_pEnt[pCol->wPos].peGreen = pCol->pe.peGreen;
					p_pEnt[pCol->wPos].peBlue = pCol->pe.peBlue;
					p_pEnt[pCol->wPos].peFlags = 1;
					pCol++;
				} while ( pCol->wPos < LOCOLORCNT );
				bPaletteSet = TRUE;
				SendMessageA(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
			}
			else {
				p_pEnt = plPal.pPalEnts;
				pCol = rgbNormalColor;
				do {
					p_pEnt[pCol->wPos].peRed = pCol->pe.peRed;
					p_pEnt[pCol->wPos].peGreen = pCol->pe.peGreen;
					p_pEnt[pCol->wPos].peBlue = pCol->pe.peBlue;
					p_pEnt[pCol->wPos].peFlags = 1;
					pCol++;
				} while ( pCol->wPos < LOCOLORCNT );
				bPaletteSet = FALSE;
			}
			hLoColor = CreatePalette((const LOGPALETTE *)&plPal);
		}
	}

	return ReleaseDC(0, hDC);
}

// Hook for when the game window regains focus
extern "C" void __stdcall Hook_MainFrame_OnActivateApp(BOOL bActive, HTASK hTask) {
	CMainFrame *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSimcityView *pSCView;
	HWND hWndCapt;

	if (!pSCApp->dwSCAMainFrameDestroyVar) {
		GameMain_Wnd_Default(pThis);
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		hWndCapt = SetCapture(pThis->m_hWnd);
		GameMain_Wnd_FromHandle(hWndCapt);
		bMainFrameInactive = !bActive;
		if (!bActive || pSCApp->dwSCAbForceBkgd) {
			if (!bActive && pSCApp->dwSCAbForceBkgd) {
				pSCApp->dwSCAbForceBkgd = FALSE;
				Game_MainFrame_ToggleToolBars(pThis, FALSE);
				// Only call this function if the setting to play
				// music in background is not enabled.
				if (!bBackgroundMusic)
					Game_SimcityApp_MusicStop(pSCApp);
				L_PlaySound_SC2K1996(0, 0);
				InvalidateRect(pThis->m_hWnd, 0, TRUE);
				if (pSCView)
					Game_SimcityView_MainWindowUpdate(pSCView, 0, FALSE);
			}
		}
		else {
			pSCApp->dwSCAbForceBkgd = TRUE;
			if (!IsIconic(pThis->m_hWnd)) {
				if (!bKeepPalette)
					Game_MainFrame_OnQueryNewPalette(pThis);
				InvalidateRect(pThis->m_hWnd, 0, TRUE);
				Game_MainFrame_ToggleToolBars(pThis, TRUE);
				if (pSCView) {
					if (!Game_Sound_IsMusicPlaying(pSCApp->SCASNDLayer))
						Game_SimcityApp_MusicPlayNextRefocusSong(pSCApp);
					// Added to get things going again.
					bSoundKickstart = true;
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_SILENT);
				}
			}
			Game_SimcityApp_SetGameCursor(pSCApp, 0, TRUE);
		}
		if (!bCSAMainFrameDirectReleaseCapture)
			ReleaseCapture();
	}
}

// Hook for when the game window changes size
extern "C" void __stdcall Hook_MainFrame_OnSize(UINT nType, int cx, int cy) {
	CMainFrame *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;

	pSCApp = &pCSimcityAppThis;
	if (nType == 1) {
		if (!bBackgroundMusic)
			Game_SimcityApp_MusicStop(pSCApp);

		L_PlaySound_SC2K1996(0, 0);
	}
	Game_MainFrame_OnQueryNewPalette(pThis);
	InvalidateRect(pThis->m_hWnd, 0, TRUE);
	Game_MainFrame_MoveStatusGoToButton(pThis);
	GameMain_MDIFrameWnd_OnSize(pThis, nType, cx, cy);
}

// Hook for when ShowWindow is called on the main game window
extern "C" void __stdcall Hook_MainFrame_OnShowWindow(BOOL bShow, BOOL nStatus) {
	CMainFrame *pThis;
	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	GameMain_Wnd_Default(pThis);
	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (bShow) {
		Game_MainFrame_OnQueryNewPalette(pThis);
		InvalidateRect(pThis->m_hWnd, 0, TRUE);
		Game_MainFrame_ToggleToolBars(pThis, TRUE);
		if (pSCView) {
			if (!Game_Sound_IsMusicPlaying(pSCApp->SCASNDLayer))
				Game_SimcityApp_MusicPlayNextRefocusSong(pSCApp);
			// Added to get things going again.
			bSoundKickstart = true;
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_SILENT);
		}
	}
	else {
		if (!bBackgroundMusic)
			Game_SimcityApp_MusicStop(pSCApp);

		L_PlaySound_SC2K1996(0, 0);
	}
	Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
}

// Hook for keyboard handling
extern "C" void __stdcall Hook_MainFrame_OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
	CMainFrame *pThis;
	__asm mov [pThis], ecx

	//ConsoleLog(LOG_DEBUG, "0x%06X -> CMainFrame::OnKeyDown(0x%06X, 0x%06X, 0x%06X)\n", _ReturnAddress(), nChar, nRepCnt, nFlags);

	GetKeyBinding_SC2K1996(CharToKey(nChar), FALSE);

	int nValidChar = _isctype((char)nChar, (_ALPHA|_DIGIT));
	if (nValidChar)
		Game_MainFrame_OnChar(pThis, nChar, nRepCnt, nFlags);
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
	pSCVScrollBarHorz = pSCView->SCVScrollBarHorz;
	pSCVScrollBarVert = pSCView->SCVScrollBarVert;
	pSCVStatic = pSCView->SCVStaticOne;
	if (!bRedraw) {
		bRedraw = TRUE;
		if (pSCApp->iSCAProgramStep == ONIDLE_STATE_EDITNEWMAP_RETURN || !wCityMode)
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
	__asm mov [pThis], ecx

	int iCityToolBarButton;
	UINT ButtonStyle, nStyle;
	int nLayer, nIndex, nPos;
	HMENU hSubMenu;
	CMFC3XMenu *pSubMenu;
	int nMenuItemCount;
	int nSubMenuItemCount;
	char szString[80*MAX_CITY_MENUTOOLS];
	char *pString;
	char *pTargString;
	DWORD uIDs[MAX_CITY_MENUTOOLS];
	DWORD *pUID;
	int nGranted;
	int nReward;
	unsigned nRewardBit;
	CMFC3XString *citySubToolStrings;

	HWND hDlgItem = GetDlgItem(pThis->dwMFStatusControlBar.m_hWnd, 120); // Status - GoTo button.
	CMapToolBar* pMapToolBar = &pThis->dwMFMapToolBar;
	if (!wCityMode)
		Game_MapToolBar_ResetControls(pMapToolBar);

	CCityToolBar* pCityToolBar = &pThis->dwMFCityToolBar;
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
	CMFC3XMenu* pMenu = &pCityToolBar->dwCTBMenuOne;
	GameMain_Menu_DestroyMenu(pMenu);
	HMENU hMenu = LoadMenuA(hGameModule, (LPCSTR)136);
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
				citySubToolStrings = &cityToolGroupStrings[CITY_MENUTOOL_COUNT(CITYTOOL_GROUP_REWARDS)];
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

extern "C" void __cdecl Hook_RemoveLabel(__int16 nTextOverlayID) {
	bool bZeroLabel = false;

	if (nTextOverlayID) {
		if (nTextOverlayID <= MAX_SIM_TEXT_ENTRIES) {
			if (nTextOverlayID <= MAX_USER_TEXT_ENTRIES) {
				if (nTextOverlayID >= MIN_USER_TEXT_ENTRIES)
					bZeroLabel = true;
			}
			else if (nTextOverlayID >= MIN_MICROSIM_LABEL_ENTRIES) {
				pMicrosimArr[MICROSIMID_ENTRY(nTextOverlayID)].bTileID = TILE_CLEAR;
				bZeroLabel = true;
			}
		}
		else if (nTextOverlayID <= MAX_XTHG_TEXT_ENTRIES)
			dwMapXTHG[0][XTHGID_ENTRY(nTextOverlayID)].bLabel = 0;
	}

	if (bZeroLabel)
		memset(&dwMapXLAB[0][nTextOverlayID], 0, sizeof(map_XLAB_t));
}

static void DoOrphanLabel_SC2K1996(bool bRemove) {
	int nCnt;
	std::string str;
	const char *pLbl;

	str = "Orphaned labels";
	str += (bRemove) ? " removed:\n\n" : " detected:\n\n";

	nCnt = 0;
	for (int i = 1; i <= MAX_USER_TEXT_ENTRIES; ++i) {
		pLbl = GetXLABEntry(i);
		if (pLbl) {
			// Only report on populated labels.
			if (strlen(pLbl) > 0) {
				if (mischook_debug & MISCHOOK_DEBUG_OTHER)
					ConsoleLog(LOG_DEBUG, "MISC: Found label '%s' at position '%d'.\n", pLbl, i);
			}
			bool bLabelFound = false;
			for (int iX = 0; iX < GAME_MAP_SIZE; ++iX) {
				for (int iY = 0; iY < GAME_MAP_SIZE; ++iY) {
					if (XTXTGetTextOverlayID(iX, iY) == i) {
						bLabelFound = true;
						break;
					}
				}
				if (bLabelFound)
					break;
			}
			if (!bLabelFound) {
				// Only report on populated labels, otherwise
				// go through the other user entries for
				// sanitisation purposes.
				if (strlen(pLbl) > 0) {
					str += string_format("%d - '%s'\n", i, pLbl);
					if (mischook_debug & MISCHOOK_DEBUG_OTHER)
						ConsoleLog(LOG_DEBUG, "MISC: Found orphaned label '%s' at position '%d'%s.\n", pLbl, i, ((bRemove) ? "; removing" : ""));
					++nCnt;
				}
				if (bRemove)
					Game_RemoveLabel(i);
			}
		}
	}

	if (nCnt > 0)
		str += string_format("\nTotal count: %d", nCnt);
	else
		str = "No orphaned labels detected.";

	L_MessageBoxA(GameGetRootWindowHandle(), str.c_str(), gamePrimaryKey, MB_ICONINFORMATION);
}

// Hook for a couple different CWnd::OnCmdMessage derivatives
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
				ReloadDefaultTileSet_SC2K1996(false);
				return TRUE;

			case IDM_MAIN_FILE_OPENMAINDIALOG:
				OpenMainDialog_SC2K1996();
				return TRUE;

			case IDM_DEBUG_SPRITE_DISPLAY:
				ShowSpriteBrowseDialog();
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_PLANES:
				DoThingClean_SC2K1996(THING_CLEAN_PLANES);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_COPTERS:
				DoThingClean_SC2K1996(THING_CLEAN_COPTERS);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_SHIPS:
				DoThingClean_SC2K1996(THING_CLEAN_SHIPS);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_SAILBOATS:
				DoThingClean_SC2K1996(THING_CLEAN_SAILBOATS);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_TRAINS:
				DoThingClean_SC2K1996(THING_CLEAN_TRAINS);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_HERO:
				DoThingClean_SC2K1996(THING_CLEAN_HERO);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_MONSTER:
				DoThingClean_SC2K1996(THING_CLEAN_MONSTER);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_TORNADO:
				DoThingClean_SC2K1996(THING_CLEAN_TORNADO);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_PLDEPLOY:
				DoThingClean_SC2K1996(THING_CLEAN_PLDEPLOY);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_FRDEPLOY:
				DoThingClean_SC2K1996(THING_CLEAN_FRDEPLOY);
				return TRUE;

			case IDM_DEBUG_THING_CLEAN_MLDEPLOY:
				DoThingClean_SC2K1996(THING_CLEAN_MLDEPLOY);
				return TRUE;

			case IDM_DEBUG_LABEL_LIST_ORPHANS:
				DoOrphanLabel_SC2K1996(false);
				return TRUE;

			case IDM_DEBUG_LABEL_CLEAR_ORPHANS:
				if (L_MessageBoxA(pThis->m_hWnd, "WARNING: You are about to clear out any detected orphaned labels, this action cannot be undone.\n\nDo you want to proceed?", gamePrimaryKey, MB_ICONWARNING|MB_YESNO) == IDYES)
					DoOrphanLabel_SC2K1996(true);
				return TRUE;
			}
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
		}
		else if (nCode == _CN_COMMAND_UI) {
			// As far as potential handling here goes - tread carefully;
			//ConsoleLog(LOG_DEBUG, "CFrameWnd::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - _CN_COMMAND_UI\n", pThis, nID, nCode, pExtra, pHandler);
		}
		return GameMain_FrameWnd_OnCmdMsg((CMFC3XFrameWnd *)pThis, nID, nCode, pExtra, pHandler);
	}
	else if ((DWORD)dwRetAddr == 0x4A4BB2) {
		if (nCode == _CN_COMMAND) {
			switch (nID) {
			// This is the 'sc2kfix Settings' enddialog return code for the main dialog to
			// execution from the BuildSubFrames section.
			case IDC_GAME_MAIN_SC2KFIXSETTINGS:
				return EndDialog(pThis->m_hWnd, ONIDLE_INITIALDIALOG_SC2KFIXSETTINGS);
			}
			//ConsoleLog(LOG_DEBUG, "::OnCmdMsg(0x%06X, %u, %d, 0x%06X, 0x%06X) - 0x%06X\n", pThis, nID, nCode, pExtra, pHandler, dwRetAddr);
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
	__asm mov [pThis], ecx

	CMFC3XTestCmdUI testCmd;
	UINT nID = LOWORD(wParam);
	HWND hWndCtrl = (HWND)lParam;
	int nCode = HIWORD(wParam);

	// If we didn't actually get a command, bail out
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

		CMFC3XWnd* pWndHandle = GameMain_Wnd_FromHandlePermanent(hWndCtrl);
		if (pWndHandle != NULL && GameMain_Wnd_SendChildNotifyLastMsg(pWndHandle, 0))
			return TRUE;
	}

	return L_OnCmdMsg(pThis, nID, nCode, 0, 0, _ReturnAddress());
}

int nOwnDrwDlg = OWNDRW_DLG_NONE;
CMFC3XWnd *pStoredWnd = NULL;

extern "C" void __stdcall Hook_Wnd_OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS) {
	CMFC3XWnd *pThis;
	__asm mov [pThis], ecx

	if (pStoredWnd == pThis) {
		if (nOwnDrwDlg == OWNDRW_DLG_BRIDGE) {
			L_BridgeSelectDialog_OnDrawItem_SC2K1996((CBridgeSelectDialog *)pThis, nIDCtl, lpDIS);
			return;
		}
	}

	if (lpDIS->CtlType == ODT_MENU) {
		CMFC3XMenu* pMenu = GameMain_Menu_FromHandlePermanent((HMENU)lpDIS->hwndItem);
		if (pMenu != NULL) {
			GameMain_Menu_DrawItem(pMenu, lpDIS);
			return;
		}
	}
	else {
		CMFC3XWnd* pChild = GameMain_Wnd_FromHandlePermanent(lpDIS->hwndItem);
		if (pChild != NULL && GameMain_Wnd_SendChildNotifyLastMsg(pChild, NULL))
			return;
	}
	GameMain_Wnd_Default(pThis);
}

// Placeholder.
void ShowModSettingsDialog(void) {
	L_MessageBoxA(GameGetRootWindowHandle(), "The mod settings dialog has not yet been implemented. Check back later.", "sc2fix", MB_OK);
}

// Hook to do startup tasks before InitInstance
extern "C" int __stdcall Hook_SimcityApp_InitInstance() {
	CSimcityAppPrimary* pThis;
	__asm mov [pThis], ecx

	SoundEngineInitialize();
	return GameMain_SimcityApp_InitInstance(pThis);
}

extern bool bStopAudioThread;

extern HANDLE hMusicHandle;
extern HANDLE hFSMIDIHandle;
extern HANDLE hSDLSoundHandle;
extern HANDLE hSDLSongHandle;

// Hook to do cleanups at the start of ExitInstance
extern "C" void __stdcall Hook_SimcityApp_ExitInstance() {
	CSimcityAppPrimary* pThis;
	__asm mov [pThis], ecx

	DWORD dwWaitRes;

	// First set the flag and then send WM_QUIT to the various
	// sound/music threads.
	bStopAudioThread = true;
	if (dwSDLSongThreadID)
		PostThreadMessage(dwSDLSongThreadID, WM_QUIT, NULL, NULL);
	if (dwSDLSoundThreadID)
		PostThreadMessage(dwSDLSoundThreadID, WM_QUIT, NULL, NULL);

	// Shut down the music threads
	if (dwFSMIDIThreadID)
		PostThreadMessage(dwFSMIDIThreadID, WM_QUIT, NULL, NULL);
	if (dwMusicThreadID)
		PostThreadMessage(dwMusicThreadID, WM_QUIT, NULL, NULL);

	// Then wait for the objects here first for 140ms.
	// If dwWaitRes returns 0, zero the handle.
	if (hSDLSongHandle) {
		dwWaitRes = WaitForSingleObject(hSDLSongHandle, THREAD_WAIT_TIME);
		if (!dwWaitRes)
			hSDLSongHandle = 0;
	}
	if (hSDLSoundHandle) {
		dwWaitRes = WaitForSingleObject(hSDLSoundHandle, THREAD_WAIT_TIME);
		if (!dwWaitRes)
			hSDLSoundHandle = 0;
	}
	if (hFSMIDIHandle) {
		dwWaitRes = WaitForSingleObject(hFSMIDIHandle, THREAD_WAIT_TIME);
		if (!dwWaitRes)
			hFSMIDIHandle = 0;
	}
	if (hMusicHandle) {
		dwWaitRes = WaitForSingleObject(hMusicHandle, THREAD_WAIT_TIME);
		if (!dwWaitRes)
			hMusicHandle = 0;
	}

	Game_PerhapsFreeDocumentsLibraryAndStrings();
	FreeLibrary(pThis->dwSCAhModule);
	SoundEngineDestroy();
	GameMain_WinApp_ExitInstance(pThis);
}

// Evil no good quick and dirty hack to fix bond issuance
// TODO (araxestroy): Properly reimplement CBudgetFundDialog::IssueBond
extern "C" uint32_t Hook_TemporaryBondCreditRatingFix(void) {
	return (25000 * dwCityBonds / (unsigned int)(dwCityValue + 1));
}

// Install hooks and run code that we only want to do for the 1996 Special Edition SIMCITY.EXE.
// This should probably have a better name. And maybe be broken out into smaller functions.
//
// UPDATE 2025-08-15 (araxestroy): Working on breaking this out nicely. It's not going well.
void InstallMiscHooks_SC2K1996(void) {
	// Install early startup hook before CSimcityApp::InitInstance
	SafeVirtualProtect((LPVOID)0x4016B3, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4016B3, Hook_SimcityApp_InitInstance);

	// Hook CSimcityApp::ExitInstance
	SafeVirtualProtect((LPVOID)0x402E5F, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402E5F, Hook_SimcityApp_ExitInstance);
	
	// Install critical Windows API hooks
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;
	*(DWORD*)(0x4EFDCC) = (DWORD)Hook_LoadMenuA;
	*(DWORD*)(0x4EFDE4) = (DWORD)Hook_MessageBoxA;
	*(DWORD*)(0x4EFC64) = (DWORD)Hook_DialogBoxParamA;
	*(DWORD*)(0x4EFCE8) = (DWORD)Hook_DefWindowProcA;

	// Install registry pathing hooks
	InstallRegistryPathingHooks_SC2K1996();

	// Install Movie hooks
	InstallMovieHooks();

	// Hook into the CGameDialog::DoModal function
	SafeVirtualProtect((LPVOID)0x40219E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40219E, Hook_GameDialog_DoModal);

	// Hook into the CGameDialog::OnDestroy function
	SafeVirtualProtect((LPVOID)0x401532, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401532, Hook_GameDialog_OnDestroy);

	InstallGraphicHooks_SC2K1996();

	// Fix the sign fonts
	SafeVirtualProtect((LPVOID)0x4E7267, 1, PAGE_EXECUTE_READWRITE);
	*(BYTE*)0x4E7267 = 'a';

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
	SafeVirtualProtect((LPVOID)0x405859, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x405859, Hook_SimcityApp_InitInstanceFix);

	// Hook CSimcityApp::OnQuit
	SafeVirtualProtect((LPVOID)0x401753, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401753, Hook_SimcityApp_OnQuit);

	InstallSpriteAndTileSetHooks_SC2K1996();
	
	// CSimcityApp::GetCapabilities
	SafeVirtualProtect((LPVOID)0x401E15, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E15, Hook_SimcityApp_GetCapabilities);

	// Hook CSimcityApp::BuildSubFrames
	SafeVirtualProtect((LPVOID)0x402A3B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402A3B, Hook_SimcityApp_BuildSubFrames);

	// Fix the Maxis Presents logo not being shown
	SafeVirtualProtect((LPVOID)0x4E6130, 13, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x4E6130, 0, 13);
	memcpy_s((LPVOID)0x4E6130, 12, "presnts.bmp", 12);

	// Change the referenced variable to 'EditData' instead.
	// This restores the "city center" marker when the
	// "Land Value" tab is selected.
	// Originally it was comparing against an orphaned variable.
	SafeVirtualProtect((LPVOID)0x487B37, 8, PAGE_EXECUTE_READWRITE);
	*(BYTE *)0x487B3B = 0xA4;
	*(BYTE *)0x487B3A = 0x04;

	InstallScenarioHooks_SC2K1996();

	// Install hooks for saving and loading
	InstallSaveHooks_SC2K1996();

	// Hook into the PrepareGame function.
	SafeVirtualProtect((LPVOID)0x401578, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401578, Hook_PrepareGame);

	// Hook into the StartCleanGame function.
	SafeVirtualProtect((LPVOID)0x401F05, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401F05, Hook_StartCleanGame);
	
	// Hook replacing CSimcityApp::NewCity
	SafeVirtualProtect((LPVOID)0x40144C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40144C, L_SimcityApp_NewCity);

	InstallDrawingHooks_SC2K1996();

	InstallCurrencyStringHooks_SC2K1996();

	InstallThingHooks_SC2K1996();

	InstallTileGrowthOrPlacementHandlingHooks_SC2K1996();

	InstallGraphsScanningStatsHandlingHooks_SC2K1996();
	
	// Install the advanced query hook
	InstallQueryHooks_SC2K1996();

	InstallArcologyDialogHooks_SC2K1996();

	InstallPowerPlantDialogHooks_SC2K1996();

	InstallBridgeDialogHooks_SC2K1996();

	// Expand sound buffers and load higher quality sounds from DLL resources
	InstallSoundEngineHooks_SC2K1996();

	// Install music engine hooks
	InstallMusicEngineHooks();

	// Hook for CMainFrame::OnActivateApp
	SafeVirtualProtect((LPVOID)0x40203B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40203B, Hook_MainFrame_OnActivateApp);

	// Hook for CMainFrame::OnSize
	SafeVirtualProtect((LPVOID)0x4027DE, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4027DE, Hook_MainFrame_OnSize);

	// Hook for CMainFrame::OnShowWindow
	SafeVirtualProtect((LPVOID)0x401A50, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401A50, Hook_MainFrame_OnShowWindow);

	// Hook for CMainFrame::OnKeyDown
	SafeVirtualProtect((LPVOID)0x402D8D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402D8D, Hook_MainFrame_OnKeyDown);

	// Hook status bar updates for the status dialog implementation
	InstallStatusHooks_SC2K1996();

	// Hook for ShowViewControls
	SafeVirtualProtect((LPVOID)0x4021D5, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4021D5, Hook_ShowViewControls);

	// Hook for CMainFrame::UpdateSections
	SafeVirtualProtect((LPVOID)0x40131B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40131B, Hook_MainFrame_UpdateSections);

	// nop out "StopSound" call in SimulationPrepareBudgetDialog()
	// this allows for the "click" to be played when executed from
	// the city toolbar.
	SafeVirtualProtect((LPVOID)0x473230, 10, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x473230, 0x90, 10);

	InstallToolBarHooks_SC2K1996();

	// New hooks for CSimcityDoc::UpdateDocumentTitle and
	// SimulationProcessTick - these account for:
	// 1) Including the day of the month in the window title.
	// 2) The fine-grained simulation updates.
	SafeVirtualProtect((LPVOID)0x4017B2, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4017B2, Hook_SimcityDoc_UpdateDocumentTitle);

	// DEPRECATED -- now local function
	SafeVirtualProtect((LPVOID)0x401820, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401820, Simulation_ProcessTick);

	// Hook SimulationStartDisaster
	SafeVirtualProtect((LPVOID)0x402527, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402527, Hook_SimulationStartDisaster);

	// Hook AddAllInventions
	SafeVirtualProtect((LPVOID)0x402388, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402388, Hook_AddAllInventions);

	// Hook CWnd::OnCommand
	SafeVirtualProtect((LPVOID)0x4A5352, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4A5352, Hook_Wnd_OnCommand);

	// Hook into CWnd::OnDrawItem
	SafeVirtualProtect((LPVOID)0x4A468C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4A468C, Hook_Wnd_OnDrawItem);

	// Add more buttons to SC2K's menus
	hMainMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(2));
	if (hMainMenu) {
		// File menu -> Open Main Dialog
		HMENU hFilePopup;
		MENUITEMINFO miiFilePopup;
		miiFilePopup.cbSize = sizeof(MENUITEMINFO);
		miiFilePopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hMainMenu, 0, TRUE, &miiFilePopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		hFilePopup = miiFilePopup.hSubMenu;
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hFilePopup, 0, MF_BYPOSITION|MF_STRING, IDM_MAIN_FILE_OPENMAINDIALOG, "&Open Main Dialog") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Main InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}

		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated main menu.\n");
	}

	skipmainmenu:

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
	// Hook for CSimcityView::CSimcityView
	SafeVirtualProtect((LPVOID)0x402E28, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402E28, Hook_SimcityView_Cons);

	// Hook for CSimcityView::ResetScrollViewsAndDeleteGraphics
	SafeVirtualProtect((LPVOID)0x402B62, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402B62, Hook_SimcityView_ResetScrollViewsAndDeleteGraphics);

	// Hook for CSimcityView::OnUpdate
	SafeVirtualProtect((LPVOID)0x4024E1, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4024E1, Hook_SimcityView_OnUpdate);

	// Hook for CSimcityView::OnLButtonDown
	SafeVirtualProtect((LPVOID)0x401523, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401523, Hook_SimcityView_OnLButtonDown);

	// Hook for CSimcityView::OnMouseMove
	SafeVirtualProtect((LPVOID)0x4016EA, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4016EA, Hook_SimcityView_OnMouseMove);

	// Hook for CSimcityView::OnRButtonDown
	SafeVirtualProtect((LPVOID)0x401C9E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401C9E, Hook_SimcityView_OnRButtonDown);

	// Hook for CSimcityView::DoBudget
	SafeVirtualProtect((LPVOID)0x4020AE, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4020AE, Hook_SimcityView_DoBudget);

	// Hook for CNewspaperDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x401E2E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E2E, Hook_NewspaperDialog_OnInitDialog);

	// Hook for CSimcityApp::LoadCursorResources
	SafeVirtualProtect((LPVOID)0x402234, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402234, Hook_SimcityApp_LoadCursorResources);

	// Hook for StartupGraphics
	SafeVirtualProtect((LPVOID)0x4014DD, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4014DD, Hook_StartupGraphics);

	// Hook for CCmdUI::Enable
	SafeVirtualProtect((LPVOID)0x4A296A, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4A296A, Hook_CmdUI_Enable);

	// Skip over the strange bit of code that re-arranges the original main menu.
	// 
	// For some reason the main menu dialog resource in simcity.exe has the Load Tile Set button
	// at the exact same coordinates as the Quit button. The code we're skipping (because we're
	// using our own dialog resource for the main menu) programatically resizes the dialog and
	// rearranges the buttons to fit on it. Why they didn't just fix the button coordinates in
	// the dialog resource instead is beyond me.
	SafeVirtualProtect((LPVOID)0x41503F, 6, PAGE_EXECUTE_READWRITE);
	NEWJMP(0x41503F, 0x415161);
	*(BYTE*)0x415044 = 0x90;

	// nop out this bit of code for when the sign dialogue is Cancelled.
	// You otherwise end up with orphaned labels (the XTXT entry is removed
	// but not the XLAB entry).
	SafeVirtualProtect((LPVOID)0x44D893, 6, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x44D893, 0x90, 6);

	// Hook RemoveLabel in-order to facilitate the complete wiping
	// of a removed label entry.
	SafeVirtualProtect((LPVOID)0x401DCA, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401DCA, Hook_RemoveLabel);

	// Temporary hack to fix bond issuance not being properly affected by credit rating.
	SafeVirtualProtect((LPVOID)0x41B279, 31, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x41B279, 0x90, 31);
	NEWCALL((LPVOID)0x41B279, Hook_TemporaryBondCreditRatingFix);

	// Call your cousin Vinnie!
	PorntipsGuzzardo();

	// Part two!
	UpdateMiscHooks_SC2K1996();
}

// The difference between InstallMiscHooks and UpdateMiscHooks is that UpdateMiscHooks can be run
// again at runtime because it can patch back in original game code. It's used for small stuff.
void UpdateMiscHooks_SC2K1996(void) {
	UpdateDrawingHooks_SC2K1996();
}
