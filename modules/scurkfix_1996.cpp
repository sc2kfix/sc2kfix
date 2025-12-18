// sc2kfix modules/scurkfix_1996.cpp: fixes for SCURK - Network Edition (1996) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

#define MISCHOOK_SCURK1996_DEBUG_INTERNAL 1
#define MISCHOOK_SCURK1996_DEBUG_PICKANDPLACE 2
#define MISCHOOK_SCURK1996_DEBUG_PLACEANDCOPY 4

#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURK1996_DEBUG
#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_scurk1996_debug = MISCHOOK_SCURK1996_DEBUG;

extern "C" void Hook_SCURK1996_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	TBC45XPalette *pPal;
	TBC45XClientDC clDC;
	TBC45XMDIChild *pMDIChild;
	HWND hWndChild, hWndObjSelect;
	BOOL bRedraw;

	bRedraw = FALSE;
	if (!IsIconic(pThis->pWnd->HWindow)) {
		pPal = GameMain_winscurkApp_GetPalette_SCURK1996(gScurkApplication_SCURK1996);
		GameMain_BCClientDC_Construct_SCURK1996(&clDC, pThis->pWnd->HWindow);
		GameMain_BCDC_SelectObjectPalette_SCURK1996(&clDC, pPal, 0);
		if (wColFastCnt_SCURK1996 == 5) {
			GameMain_winscurkMDIClient_RotateColors_SCURK1996(pThis, 1);
			AnimatePalette((HPALETTE)pPal->Handle, 0xAB, 0x31, pThis->mFastColors);
			wColFastCnt_SCURK1996 = 0;
			bRedraw = TRUE;
		}
		if (wColSlowCnt_SCURK1996 == 30) {
			GameMain_winscurkMDIClient_RotateColors_SCURK1996(pThis, 0);
			AnimatePalette((HPALETTE)pPal->Handle, 0xE0, 0x10, pThis->mSlowColors);
			wColSlowCnt_SCURK1996 = 0;
			bRedraw = TRUE;
		}
		++wColFastCnt_SCURK1996;
		++wColSlowCnt_SCURK1996;
		GameMain_BCWindowDC_Destruct_SCURK1996(&clDC, 0);

		// Only call redraw if the given MDIChild is active, rather than
		// refreshing all windows from pThis->pWnd->HWindow downwards.
		//
		// This reduces "a bit" of the flickering that was otherwise occurring
		// across all windows; at this stage it is only limited to the active
		// MDI Child.
		if (bRedraw) {
			pMDIChild = GameMain_BCMDIClient_GetActiveMDIChild_SCURK1996(pThis);
			if (pMDIChild) {
				hWndChild = 0;
				hWndObjSelect = 0;
				if (pMDIChild == (TBC45XMDIChild *)pThis->mPlaceWindow) {
					hWndChild = pThis->mPlaceWindow->__wndHead.pWnd->HWindow;
					hWndObjSelect = pThis->mPlaceWindow->pPlaceTileListDlg->pWnd->HWindow;
				}
				else if (pMDIChild == (TBC45XMDIChild *)pThis->mMoverWindow)
					hWndChild = pThis->mMoverWindow->__wndHead.pWnd->HWindow;
				else if (pMDIChild == (TBC45XMDIChild *)pThis->mEditWindow)
					hWndChild = ((winscurkParMDIChild *)pThis->mEditWindow)->__wndHead.pWnd->HWindow;

				if (hWndChild)
					RedrawWindow(hWndChild, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
				if (hWndObjSelect)
					RedrawWindow(hWndObjSelect, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
			}
		}
	}
}

extern "C" void __cdecl Hook_SCURK1996_DebugOut(char const *fmt, ...) {
	va_list args;
	int len;
	char* buf;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_INTERNAL) == 0)
		return;

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		ConsoleLog(LOG_DEBUG, "0x%06X -> gDebugOut(): %s", _ReturnAddress(), buf);

		free(buf);
	}

	va_end(args);
}

void InstallFixes_SCURK1996(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk1996_debug = DEBUG_FLAGS_EVERYTHING;

	InstallRegistryPathingHooks_SCURK1996();

	// Hook for palette animation fix
	VirtualProtect((LPVOID)0x449800, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x449800, Hook_SCURK1996_winscurkMDIClient_CycleColors);
	ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix for SCURK.\n");

	// Add back the internal debug notices for tracing purposes.
	VirtualProtect((LPVOID)0x4132E8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4132E8, Hook_SCURK1996_DebugOut);
}
