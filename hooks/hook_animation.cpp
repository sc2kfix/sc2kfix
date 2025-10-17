// sc2kfix hooks/hook_animation.cpp: hooks to do with animations across the various
// supported versions of SimCity 2000.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

extern "C" void __cdecl Hook_ToggleColorCycling_SC2K1996(CMFC3XPalette *pPalette, int iToggle) {
	CSimcityAppPrimary *pApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	BOOL bCityViewAnim;

	// Only redraw the relevant windows during:
	// 1) Titlescreen image animation.
	// 2) While the CSimcityView window is active and the toolbars aren't being dragged.
	// 3) None of the additional redraw calls in LoColor mode.

	GameMain_ToggleColorCycling(pPalette, iToggle);

	pApp = &pCSimcityAppThis;
	if (pApp) {
		if (!bLoColor) {
			pMainFrm = (CMainFrame *)pApp->m_pMainWnd;
			wSimSpeed = pApp->wSCAGameSpeedLOW;
			dwTitleScreenAnimation = pApp->dwSCAToggleTitleScreenAnimation;
			iProgramStep = pApp->iSCAProgramStep;
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = &pMainFrm->dwMFMapToolBar;
					pCityToolBar = &pMainFrm->dwMFCityToolBar;

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					bCityViewAnim = TRUE;

					if (pCityToolBar && pCityToolBar->dwCTBToolBarTitleDrag)
						bCityViewAnim = FALSE;
					
					if (pMapToolBar && pMapToolBar->dwMTBToolBarTitleDrag)
						bCityViewAnim = FALSE;

					if (CanUseFloatingStatusDialog() && bStatusDialogMoving)
						bCityViewAnim = FALSE;

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pApp);
					if (!pSCView)
						RedrawWindow(pMainFrm->m_hWnd, NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_ToggleColorCycling_SC2K1995(CMFC3XPalette *pPalette, int iToggle) {
	CSimcityAppPrimary *pApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	BOOL bCityViewAnim;

	CSimcityAppPrimary &pCSimcityAppThis1995 = *(CSimcityAppPrimary *)0x4C6010;
	BOOL &bLoColor1995 = *(BOOL *)0x4E903C;

	CSimcityView *(__thiscall *H_SimcityApp_PointerToCSimcityViewClass1995)(CSimcityAppPrimary *) = (CSimcityView *(__thiscall *)(CSimcityAppPrimary *))0x4026D0;
	void(__cdecl *H_ToggleColorCycling1995)(CMFC3XPalette *, int) = (void(__cdecl *)(CMFC3XPalette *, int))0x456A60;

	H_ToggleColorCycling1995(pPalette, iToggle);
	
	pApp = &pCSimcityAppThis1995;
	if (pApp) {
		if (!bLoColor1995) {
			pMainFrm = (CMainFrame *)pApp->m_pMainWnd; // m_pMainWnd
			wSimSpeed = pApp->wSCAGameSpeedLOW;
			dwTitleScreenAnimation = pApp->dwSCAToggleTitleScreenAnimation;
			iProgramStep = pApp->iSCAProgramStep;
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = &pMainFrm->dwMFMapToolBar;
					pCityToolBar = &pMainFrm->dwMFCityToolBar;

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					bCityViewAnim = TRUE;

					if (pCityToolBar && pCityToolBar->dwCTBToolBarTitleDrag)
						bCityViewAnim = FALSE;

					if (pMapToolBar && pMapToolBar->dwMTBToolBarTitleDrag)
						bCityViewAnim = FALSE;

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = H_SimcityApp_PointerToCSimcityViewClass1995(pApp);
					if (!pSCView)
						RedrawWindow(pMainFrm->m_hWnd, NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_ToggleColorCycling_SC2KDemo(CMFC3XPalette *pPalette, int iToggle) {
	CSimcityAppDemo *pApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	BOOL bCityViewAnim;

	CSimcityAppDemo &pCSimcityAppThisDemo = *(CSimcityAppDemo *)0x4B6A70;
	BOOL &bLoColorDemo = *(BOOL *)0x4D1EDC;

	CSimcityView *(__thiscall *H_SimcityApp_PointerToCSimcityViewClassDemo)(CSimcityAppDemo *) = (CSimcityView *(__thiscall *)(CSimcityAppDemo *))0x402725;
	void(__cdecl *H_ToggleColorCyclingDemo)(CMFC3XPalette *, int) = (void(__cdecl *)(CMFC3XPalette *, int))0x44890F;

	H_ToggleColorCyclingDemo(pPalette, iToggle);

	pApp = &pCSimcityAppThisDemo;
	if (pApp) {
		if (!bLoColorDemo) {
			pMainFrm = (CMainFrame *)pApp->m_pMainWnd; // m_pMainWnd
			wSimSpeed = pApp->wSCAGameSpeedLOW;
			dwTitleScreenAnimation = pApp->dwSCAToggleTitleScreenAnimation;
			iProgramStep = pApp->iSCAProgramStep;
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = &pMainFrm->dwMFMapToolBar;
					pCityToolBar = &pMainFrm->dwMFCityToolBar;

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					bCityViewAnim = TRUE;

					if (pCityToolBar && pCityToolBar->dwCTBToolBarTitleDrag)
						bCityViewAnim = FALSE;

					if (pMapToolBar && pMapToolBar->dwMTBToolBarTitleDrag)
						bCityViewAnim = FALSE;

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = H_SimcityApp_PointerToCSimcityViewClassDemo(pApp);
					if (!pSCView)
						RedrawWindow(pMainFrm->m_hWnd, NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

void InstallAnimationHooks_SC2K1996(void) {
	VirtualProtect((LPVOID)0x4023D3, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4023D3, Hook_ToggleColorCycling_SC2K1996);
}

void InstallAnimationHooks_SC2K1995(void) {
	VirtualProtect((LPVOID)0x402405, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402405, Hook_ToggleColorCycling_SC2K1995);
}

void InstallAnimationHooks_SC2KDemo(void) {
	VirtualProtect((LPVOID)0x402473, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402473, Hook_ToggleColorCycling_SC2KDemo);
}
