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

extern "C" void __cdecl Hook_AnimationFunctionSimCity1996(HPALETTE *hP1, int iToggle) {
	DWORD *pApp;
	DWORD *pMainFrm;
	DWORD *pSCView;
	DWORD *pMapToolBar;
	DWORD *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	DWORD dwTBControlsDisabled;
	DWORD dwTBToolBarTitleDrag;
	BOOL bCityViewAnim;

	BOOL &bLoColor = *(BOOL *)0x4EA04C;

	void(__cdecl *H_AnimationFunction1996)(HPALETTE *, int) = (void(__cdecl *)(HPALETTE *, int))0x457110;

	// Only redraw the relevant windows during:
	// 1) Titlescreen image animation.
	// 2) While the CSimcityView window is active and the toolbars aren't being dragged.
	// 3) None of the additional redraw calls in LoColor mode.

	H_AnimationFunction1996(hP1, iToggle);

	pApp = &pCSimcityAppThis;
	if (pApp) {
		if (!bLoColor) {
			pMainFrm = (DWORD *)pApp[7]; // m_pMainWnd
			wSimSpeed = ((WORD *)pApp)[388];
			dwTitleScreenAnimation = pApp[199];
			iProgramStep = pApp[201];
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = (DWORD *)&pMainFrm[233];
					pCityToolBar = (DWORD *)&pMainFrm[102];

					// For the toolbars, they're using vars from the parent MyToolBar class
					// 46 and 47 are dwMyTBControlsDisabled and dwMyTBToolBarTitleDrag respectively.
					dwTBControlsDisabled = pMapToolBar[46];
					dwTBToolBarTitleDrag = pCityToolBar[47];

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					if (wCityMode)
						bCityViewAnim = (pCityToolBar && !dwTBToolBarTitleDrag);
					else
						bCityViewAnim = (pMapToolBar && !dwTBControlsDisabled);

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = Game_PointerToCSimcityViewClass(pApp);
					if (!pSCView)
						RedrawWindow((HWND)pMainFrm[7], NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow((HWND)pSCView[7], NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_AnimationFunctionSimCity1995(HPALETTE *hP1, int iToggle) {
	DWORD *pApp;
	DWORD *pMainFrm;
	DWORD *pSCView;
	DWORD *pMapToolBar;
	DWORD *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	DWORD dwTBControlsDisabled;
	DWORD dwTBToolBarTitleDrag;
	BOOL bCityViewAnim;

	DWORD &pCSimcityAppThis1995 = *(DWORD *)0x4C6010;
	WORD &wCityMode1995 = *(WORD *)0x4C9444;
	BOOL &bLoColor1995 = *(BOOL *)0x4E903C;

	DWORD *(__thiscall *H_PointerToCSimcityViewClass1995)(void *) = (DWORD *(__thiscall *)(void *))0x4026D0;
	void(__cdecl *H_AnimationFunction1995)(HPALETTE *, int) = (void(__cdecl *)(HPALETTE *, int))0x456A60;

	H_AnimationFunction1995(hP1, iToggle);
	
	pApp = &pCSimcityAppThis1995;
	if (pApp) {
		if (!bLoColor1995) {
			pMainFrm = (DWORD *)pApp[7]; // m_pMainWnd
			wSimSpeed = ((WORD *)pApp)[388];
			dwTitleScreenAnimation = pApp[199];
			iProgramStep = pApp[201];
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = (DWORD *)&pMainFrm[233];
					pCityToolBar = (DWORD *)&pMainFrm[102];

					// For the toolbars, they're using vars from the parent MyToolBar class
					// 46 and 47 are dwMyTBControlsDisabled and dwMyTBToolBarTitleDrag respectively.
					dwTBControlsDisabled = pMapToolBar[46];
					dwTBToolBarTitleDrag = pCityToolBar[47];

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					if (wCityMode1995)
						bCityViewAnim = (pCityToolBar && !dwTBToolBarTitleDrag);
					else
						bCityViewAnim = (pMapToolBar && !dwTBControlsDisabled);

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = H_PointerToCSimcityViewClass1995(pApp);
					if (!pSCView)
						RedrawWindow((HWND)pMainFrm[7], NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow((HWND)pSCView[7], NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_AnimationFunctionSimCityDemo(HPALETTE *hP1, int iToggle) {
	DWORD *pApp;
	DWORD *pMainFrm;
	DWORD *pSCView;
	DWORD *pMapToolBar;
	DWORD *pCityToolBar;
	WORD wSimSpeed;
	DWORD dwTitleScreenAnimation;
	int iProgramStep;
	DWORD dwTBControlsDisabled;
	DWORD dwTBToolBarTitleDrag;
	BOOL bCityViewAnim;

	DWORD &pCSimcityAppThisDemo = *(DWORD *)0x4B6A70;
	WORD &wCityModeDemo = *(WORD *)0x4B302C;
	BOOL &bLoColorDemo = *(BOOL *)0x4D1EDC;

	DWORD *(__thiscall *H_PointerToCSimcityViewClassDemo)(void *) = (DWORD *(__thiscall *)(void *))0x402725;
	void(__cdecl *H_AnimationFunctionDemo)(HPALETTE *, int) = (void(__cdecl *)(HPALETTE *, int))0x44890F;

	H_AnimationFunctionDemo(hP1, iToggle);

	pApp = &pCSimcityAppThisDemo;
	if (pApp) {
		if (!bLoColorDemo) {
			pMainFrm = (DWORD *)pApp[7]; // m_pMainWnd
			wSimSpeed = ((WORD *)pApp)[386];
			dwTitleScreenAnimation = pApp[198];
			iProgramStep = pApp[200];
			if (wSimSpeed != GAME_SPEED_PAUSED || dwTitleScreenAnimation || iProgramStep != ONIDLE_STATE_INGAME) {
				if (pMainFrm) {
					pMapToolBar = (DWORD *)&pMainFrm[233];
					pCityToolBar = (DWORD *)&pMainFrm[102];

					// For the toolbars, they're using vars from the parent MyToolBar class
					// 46 and 47 are dwMyTBControlsDisabled and dwMyTBToolBarTitleDrag respectively.
					dwTBControlsDisabled = pMapToolBar[46];
					dwTBToolBarTitleDrag = pCityToolBar[47];

					// With this check, the redraw calls won't be made if either toolbar is being dragged.
					// This avoids any bleeding that may occur as a result of the blitted border that will
					// appear during this time.
					//
					// NOTE: The only side-effect is that during toolbar dragging no palette animation will
					//       occur; this won't have any effect on the simulation itself since that is temporarily
					//       suspended during city toolbar dragging (and it doesn't matter during the map toolbar
					//       dragging case).
					if (wCityModeDemo)
						bCityViewAnim = (pCityToolBar && !dwTBToolBarTitleDrag);
					else
						bCityViewAnim = (pMapToolBar && !dwTBControlsDisabled);

					// CMainFrame m_hWnd - only call this specific redraw function before CSimcityView has been created.
					// (ie, before any game has been started - palette animation on the image is disabled once the
					// game window has been created)
					pSCView = H_PointerToCSimcityViewClassDemo(pApp);
					if (!pSCView)
						RedrawWindow((HWND)pMainFrm[7], NULL, NULL, RDW_INVALIDATE);
					else if (pSCView && bCityViewAnim)
						RedrawWindow((HWND)pSCView[7], NULL, NULL, RDW_INVALIDATE);
				}
			}
		}
	}
}

void InstallAnimationSimCity1996Hooks(void) {
	VirtualProtect((LPVOID)0x4023D3, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4023D3, Hook_AnimationFunctionSimCity1996);
}

void InstallAnimationSimCity1995Hooks(void) {
	VirtualProtect((LPVOID)0x402405, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402405, Hook_AnimationFunctionSimCity1995);
}

void InstallAnimationSimCityDemoHooks(void) {
	VirtualProtect((LPVOID)0x402473, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402473, Hook_AnimationFunctionSimCityDemo);
}
