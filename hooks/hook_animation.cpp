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

extern "C" void __cdecl Hook_ToggleColorCycling_SC2K1996(CMFC3XPalette *pPalette, int bToggle) {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	HDC hDC;
	CMFC3XDC *pDC;
	CMFC3XPalette *pSelPal;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	BOOL bRedraw, bCityViewAnim;

	// Only redraw the relevant windows during:
	// 1) Titlescreen image animation.
	// 2) While the CSimcityView window is active and the toolbars aren't being dragged.
	// 3) None of the additional redraw calls in LoColor mode.

	bRedraw = FALSE;
	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		if (pSCApp->wSCAGameSpeedLOW != GAME_SPEED_PAUSED || pSCApp->dwSCAToggleTitleScreenAnimation || pSCApp->iSCAProgramStep != ONIDLE_STATE_INGAME) {
			if (!bLoColor) {
				pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
				if (pMainFrm) {
					GetPaletteEntries((HPALETTE)pPalette->m_hObject, 0, 0x100, pPalAnimMain);
					hDC = GetDC(pMainFrm->m_hWnd);
					pDC = GameMain_DC_FromHandle(hDC);
					pSelPal = GameMain_DC_SelectPalette(pDC, pPalette, FALSE);
					if (bToggle) {
						Game_SwapCycle(0);
						AnimatePalette((HPALETTE)pPalette->m_hObject, 224, 16, pPalOffCycle);
						bRedraw = TRUE;
					}
					else {
						Game_SwapCycle(1);
						AnimatePalette((HPALETTE)pPalette->m_hObject, 171, 49, pPalOnCycle);
						bRedraw = TRUE;
					}
					GameMain_DC_SelectPalette(pDC, pSelPal, FALSE);
					ReleaseDC(pMainFrm->m_hWnd, pDC->m_hDC);

					if (bRedraw) {
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
						pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
						if (!pSCView)
							RedrawWindow(pMainFrm->m_hWnd, NULL, NULL, RDW_INVALIDATE);
						else if (pSCView && bCityViewAnim)
							RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
					}
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_ToggleColorCycling_SC2K1995(CMFC3XPalette *pPalette, BOOL bToggle) {
	CSimcityAppPrimary *pApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	BOOL bCityViewAnim;

	GameMain_ToggleColorCycling_1995(pPalette, bToggle);
	
	pApp = &pCSimcityAppThis_1995;
	if (pApp) {
		if (!bLoColor_1995) {
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
					pSCView = Game_SimcityApp_PointerToCSimcityViewClass_1995(pApp);
					if (!pSCView)
						RedrawWindow(pMainFrm->m_hWnd, NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow(pSCView->m_hWnd, NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_ToggleColorCycling_SC2KDemo(CMFC3XPalette *pPalette, BOOL bToggle) {
	CSimcityAppDemo *pApp;
	CMainFrame *pMainFrm;
	CSimcityView *pSCView;
	CMapToolBar *pMapToolBar;
	CCityToolBar *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	BOOL bCityViewAnim;

	GameMain_ToggleColorCycling_Demo(pPalette, bToggle);

	pApp = &pCSimcityAppThis_Demo;
	if (pApp) {
		if (!bLoColor_Demo) {
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
					pSCView = Game_SimcityApp_PointerToCSimcityViewClass_Demo(pApp);
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
