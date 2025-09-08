// sc2kfix modules/status_dialog.cpp: recreation of the DOS/Mac version status dialog
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <intrin.h>
#include <string>
#include <regex>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define GOTO_BTN_TEXT   0
#define GOTO_BTN_IMAGE  1

#define GOTO_STYLE_3D   0
#define GOTO_STYLE_FLAT 1

#define GOTO_IMG_POS    12

static DWORD dwDummy;

HANDLE hWeatherBitmaps[13];
HANDLE hCompassBitmaps[4];
BOOL bStatusDialogMoving = FALSE;

static HWND hStatusDialog = NULL;
static HWND hGotoButton = NULL;
static int iGotoButtonType = GOTO_BTN_TEXT;
static int iGotoButtonStyle = GOTO_STYLE_3D;
static POINT ptFloatNew;
static POINT ptFloatMoving;
static POINT ptFloat;
static SIZE szFloat;
static WNDPROC OldGotoButtonWndProc;

BOOL CanUseFloatingStatusDialog() {
	return (bSettingsUseStatusDialog && hStatusDialog) ? TRUE : FALSE;
}

void ToggleFloatingStatusDialog(BOOL bEnable) {
	if (hStatusDialog)
		EnableWindow(hStatusDialog, bEnable);
}

void ToggleGotoButton(HWND hWndBut, BOOL bEnable) {
	EnableWindow(hWndBut, bEnable);
	if (hGotoButton)
		EnableWindow(hGotoButton, bEnable);
}

static void OnDrawGotoButton(LPDRAWITEMSTRUCT lpDIS) {
	HDC hDC, hDCMem;
	HBITMAP hBitmap, hBitmapMem;
	HANDLE hOld;
	RECT r;
	UINT nState;
	HWND hWndItem;
	LONG iWidth, iHeight;
	COLORREF colBtnShadow, colBtnFace, colBtnHighlight, colWinFrame;
	HBRUSH hBrush;
	BITMAP bm;
	char szBuf[128 + 1];

	memset(szBuf, 0, sizeof(szBuf));

	if (!lpDIS)
		return;

	hDC = lpDIS->hDC;
	r = lpDIS->rcItem;
	nState = lpDIS->itemState;
	hWndItem = lpDIS->hwndItem;

	iWidth = lpDIS->rcItem.right - lpDIS->rcItem.left;
	iHeight = lpDIS->rcItem.bottom - lpDIS->rcItem.top;

	hDCMem = CreateCompatibleDC(hDC);
	hBitmapMem = CreateCompatibleBitmap(hDC, iWidth, iHeight);
	hOld = SelectObject(hDCMem, hBitmapMem);

	colBtnShadow = GetSysColor(COLOR_BTNSHADOW);
	colBtnFace = GetSysColor(COLOR_BTNFACE);
	colBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
	colWinFrame = GetSysColor(COLOR_WINDOWFRAME);

	if ((nState & ODS_SELECTED) != 0) {
		SetBkColor(hDCMem, colBtnShadow);
		hBrush = CreateSolidBrush(colBtnShadow);
		FillRect(hDCMem, &r, hBrush);
		DeleteObject(hBrush);
	}
	else {
		SetBkColor(hDCMem, colBtnFace);
		hBrush = CreateSolidBrush(colBtnFace);
		FillRect(hDCMem, &r, hBrush);
		DeleteObject(hBrush);
	}

	if ((nState & ODS_SELECTED) != 0) {
		hBrush = CreateSolidBrush(colBtnHighlight);
		if (iGotoButtonStyle == GOTO_STYLE_FLAT)
			FrameRect(hDCMem, &r, hBrush);
		else
			DrawEdge(hDCMem, &r, EDGE_SUNKEN, BF_RECT);
		DeleteObject(hBrush);
	}
	else {
		hBrush = CreateSolidBrush(colWinFrame);
		if (iGotoButtonStyle == GOTO_STYLE_FLAT)
			FrameRect(hDCMem, &r, hBrush);
		else
			DrawEdge(hDCMem, &r, EDGE_RAISED, BF_RECT);
		DeleteObject(hBrush);
	}

	if (iGotoButtonType == GOTO_BTN_TEXT) {
		GetWindowText(hWndItem, szBuf, 128);

		// Opaque background to start with.
		SetBkMode(hDCMem, OPAQUE);
		if ((nState & ODS_DISABLED) != 0) {
			// White text for the 3D effect.
			SetTextColor(hDCMem, RGB(255, 255, 255));
			// Offset by 1.
			OffsetRect(&r, 1, 1);
			// Draw it.
			DrawTextA(hDCMem, szBuf, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
			// Offset by -1.
			OffsetRect(&r, -1, -1);
			// Background mode to transparent to preserve the above white text.
			SetBkMode(hDCMem, TRANSPARENT);
			// Draw over it with the gray text to replicate the disabled 3D look.
			SetTextColor(hDCMem, GetSysColor(COLOR_GRAYTEXT));
		}
		else
			SetTextColor(hDCMem, RGB(0, 0, 0));
		DrawTextA(hDCMem, szBuf, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	}
	else if (iGotoButtonType == GOTO_BTN_IMAGE) {
		HDC memDC;

		memDC = CreateCompatibleDC(hDCMem);
		hBitmap = (HBITMAP)hWeatherBitmaps[(dwDisasterActive) ? GOTO_IMG_POS : bWeatherTrend];
		GetObject(hBitmap, sizeof(tagBITMAP), &bm);
		SelectObject(memDC, hBitmap);

		// Loaded image size is 32x32, offset by 4 in order for it to be centred.
		OffsetRect(&r, 4, 4);
		// Blit.
		BitBlt(hDCMem, r.left, r.top, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);
		DeleteDC(memDC);
	}
	BitBlt(hDC, 0, 0, iWidth, iHeight, hDCMem, 0, 0, SRCCOPY);

	SelectObject(hDCMem, hOld);

	DeleteObject(hBitmapMem);
	DeleteDC(hDCMem);
}

LRESULT CALLBACK NewGotoButtonWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hParentWindow;
	HWND hGameStatusBar;

	switch (uMsg) {
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_KEYUP:
			if (hWnd == hGotoButton) {
				hParentWindow = GameGetRootWindowHandle();
				if (hParentWindow) {
					if (uMsg != WM_KEYUP) {
						/// '111' - uID set in SC2K for the StatusControlBar.
						hGameStatusBar = GetDlgItem(hParentWindow, 111);
						if (hGameStatusBar)
							SendMessage(GetDlgItem(hGameStatusBar, 120), BM_CLICK, 0, 0);
					}
					SetFocus(hParentWindow);
				}
				return FALSE;
			}
			break;
	}
	return CallWindowProc(OldGotoButtonWndProc, hWnd, uMsg, wParam, lParam);
}

static void SetGotoButtonAttributes(HWND hWnd) {
	RECT wndRect;

	if (hGotoButton)
		return;

	GetClientRect(hWnd, &wndRect);

	hGotoButton = GetDlgItem(hWnd, IDC_BUTTON_GOTO);

	OldGotoButtonWndProc = SubclassWindow(hGotoButton, (WNDPROC)NewGotoButtonWndProc);

	iGotoButtonType = GOTO_BTN_IMAGE;
	iGotoButtonStyle = GOTO_STYLE_FLAT;
	
	SetWindowPos(hGotoButton, HWND_TOP, wndRect.right - 40, wndRect.top, 40, 40, SWP_NOZORDER|SWP_NOACTIVATE);
}

static void OnPaintFloatingStatusBar(HWND hWnd, HDC hDC) {
	LPCSTR pStringCtrl;
	LPCSTR pStringNotif;
	COLORREF crCtrl;
	COLORREF crNotif;
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
	RECT r;
	HDC hDCBits;
	BITMAP bm;
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CStatusControlBar *pStatusBar;

	// We're using the brushes and colours from the main program.
	CMFC3XBrush *MainBrushFace = (CMFC3XBrush *)0x4CAA48;
	CMFC3XBrush *MainBrushBorder = (CMFC3XBrush *)0x4CB1B0;
	COLORREF &colBtnFace = *(COLORREF *)0x4CB3FC;

	if (hWnd && IsWindowVisible(hWnd)) {

		GetClientRect(hWnd, &r);

		FillRect(hDC, &r, (HBRUSH)MainBrushFace->m_hObject);
		FrameRect(hDC, &r, (HBRUSH)MainBrushBorder->m_hObject);

		pSCApp = Game_GetSimcityAppClassPointer();
		pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
		pStatusBar = &pMainFrm->dwMFStatusControlBar;
		if (pStatusBar) {
			pStringCtrl = pStatusBar->dwSCBCStringCtrlSelection.m_pchData;
			pStringNotif = pStatusBar->dwSCBCStringNotification.m_pchData;
			crCtrl = pStatusBar->dwSCBColorCtrlSelection;
			crNotif = pStatusBar->dwSCBColorNotification;

			left = 0;
			top = 3;
			right = 174;
			bottom = 15;

			// Top line.
			SelectFont(hDC, hFontMSSansSerifBold8);
			SetRect(&r, left, top, right, bottom);
			OffsetRect(&r, 5, 2);
			FillRect(hDC, &r, (HBRUSH)MainBrushFace->m_hObject);
			SetTextAlign(hDC, 8);
			SetBkColor(hDC, colBtnFace);
			SetTextColor(hDC, crCtrl);
			ExtTextOutA(hDC, r.left, r.bottom, ETO_CLIPPED, &r, pStringCtrl, lstrlenA(pStringCtrl), 0);

			top += 18;
			bottom += 18;

			// Bottom line.
			SelectFont(hDC, hFontMSSansSerifRegular8);
			SetRect(&r, left, top, right, bottom);
			OffsetRect(&r, 5, 2);
			FillRect(hDC, &r, (HBRUSH)MainBrushFace->m_hObject);
			SetTextAlign(hDC, 8);
			SetBkColor(hDC, colBtnFace);
			SetTextColor(hDC, crNotif);
			ExtTextOutA(hDC, r.left, r.bottom, ETO_CLIPPED, &r, pStringNotif, lstrlenA(pStringNotif), 0);

			// The following values are to vertically centre a 38x38
			// sized image within the borders of a the widget with a height of 40.
			// The left/right values will blit the image just prior to the
			// weather/goto indicator button.
			left = 182;
			top = 1;
			right = 220;
			bottom = 39;

			// Compass image.
			SetRect(&r, left, top, right, bottom);
			FillRect(hDC, &r, (HBRUSH)MainBrushFace->m_hObject);
			hDCBits = CreateCompatibleDC(hDC);
			GetObject(hCompassBitmaps[wViewRotation], sizeof(tagBITMAP), &bm);
			SelectObject(hDCBits, hCompassBitmaps[wViewRotation]);
			BitBlt(hDC, r.left, r.top, bm.bmWidth, bm.bmHeight, hDCBits, 0, 0, SRCCOPY);
			DeleteDC(hDCBits);
		}
	}
}

void MoveAndBlitStatusWidget(HWND hWnd, int x, int y) {
	HDC hDC;
	RECT r;

	GetWindowRect(hWnd, &r);
	hDC = GetDC(0);
	PatBlt(hDC, x - ptFloatNew.x, y - ptFloatNew.y, r.right - r.left, 2, PATINVERT);
	PatBlt(hDC, 
		x + r.right - ptFloatNew.x - r.left,
		y - ptFloatNew.y,
		2,
		r.bottom - r.top,
		PATINVERT);
	PatBlt(hDC,
		x - ptFloatNew.x,
		y + r.bottom - ptFloatNew.y - r.top,
		r.right - r.left + 2,
		2,
		PATINVERT);
	PatBlt(hDC, x - ptFloatNew.x, y - ptFloatNew.y + 2, 2, r.bottom - r.top - 2, PATINVERT);
	ReleaseDC(0, hDC);
}

BOOL CALLBACK StatusDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	CSimcityAppPrimary *pSCApp;
	LPDRAWITEMSTRUCT lpDIS;
	PAINTSTRUCT ps;
	HDC hDC, hDCMem;
	HANDLE hOld;
	HBITMAP hBitmapMem;
	POINT pt;

	switch (message) {
		case WM_INITDIALOG:
			if (!bFontsInitialized)
				InitializeFonts();

			SetWindowPos(hwndDlg, HWND_TOP, ptFloat.x, ptFloat.y, szFloat.cx, szFloat.cy, SWP_NOACTIVATE|SWP_HIDEWINDOW);

			SetGotoButtonAttributes(hwndDlg);
			return TRUE;

		case WM_DRAWITEM:
			lpDIS = (LPDRAWITEMSTRUCT)lParam;
			if (lpDIS->hwndItem == hGotoButton)
				OnDrawGotoButton(lpDIS);
			return FALSE;

		case WM_PAINT:
			hDC = BeginPaint(hwndDlg, &ps);
			hDCMem = CreateCompatibleDC(hDC);
			hBitmapMem = CreateCompatibleBitmap(hDC, szFloat.cx, szFloat.cy);
			hOld = SelectObject(hDCMem, hBitmapMem);
			OnPaintFloatingStatusBar(hwndDlg, hDCMem);
			BitBlt(hDC, 0, 0, szFloat.cx, szFloat.cy, hDCMem, 0, 0, SRCCOPY);
			SelectObject(hDCMem, hOld);
			DeleteObject(hBitmapMem);
			DeleteDC(hDCMem);
			EndPaint(hwndDlg, &ps);
			return FALSE;

		case WM_LBUTTONDOWN:
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);

			pSCApp = Game_GetSimcityAppClassPointer();

			pSCApp->dwSCADragSuspendSim = 1;
			bStatusDialogMoving = TRUE;
			ptFloatNew.x = pt.x;
			ptFloatNew.y = pt.y;
			SetCapture(hwndDlg);
			ClientToScreen(hwndDlg, &pt);
			MoveAndBlitStatusWidget(hwndDlg, pt.x, pt.y);
			ptFloatMoving.x = pt.x;
			ptFloatMoving.y = pt.y;
			break;

		case WM_MOUSEMOVE:
			if (bStatusDialogMoving) {
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);

				ClientToScreen(hwndDlg, &pt);
				MoveAndBlitStatusWidget(hwndDlg, ptFloatMoving.x, ptFloatMoving.y);
				ptFloatMoving.x = pt.x;
				ptFloatMoving.y = pt.y;
				MoveAndBlitStatusWidget(hwndDlg, pt.x, pt.y);
			}
			break;

		// Onbutton/key release set the focus back to the main game
		// window.
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_KEYUP:
			if (message == WM_LBUTTONUP) {
				if (bStatusDialogMoving) {
					pt.x = GET_X_LPARAM(lParam);
					pt.y = GET_Y_LPARAM(lParam);

					pSCApp = Game_GetSimcityAppClassPointer();

					bStatusDialogMoving = FALSE;
					ReleaseCapture();
					MoveAndBlitStatusWidget(hwndDlg, ptFloatMoving.x, ptFloatMoving.y);
					ClientToScreen(hwndDlg, &pt);
					SetWindowPos(hwndDlg, HWND_TOP, pt.x - ptFloatNew.x, pt.y - ptFloatNew.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);

					// Record the new position.
					ptFloat.x = pt.x - ptFloatNew.x;
					ptFloat.y = pt.y - ptFloatNew.y;
					pSCApp->dwSCADragSuspendSim = 0;
				}
			}
			SetFocus(GameGetRootWindowHandle());
			break;

		// This will allow for cheat execution if the status widget
		// has keyboard focus at the time.
		case WM_CHAR:
			SendMessage(GameGetRootWindowHandle(), WM_CHAR, wParam, lParam);
			return TRUE;

		// This will reset the pointer to the default arrow while
		// while the cursor hovers over the status bar.
		case WM_SETCURSOR:
			pSCApp = Game_GetSimcityAppClassPointer();
			Game_SimcityAppSetGameCursor(pSCApp, 0, 0);
			pSCApp->dwSCACursorGameHit = 4;
			return TRUE;
	}

	return FALSE;
}

static void CreateFloatingStatusDialog(void) {
	if (hStatusDialog)
		return;

	hStatusDialog = CreateDialogParam(hSC2KFixModule, MAKEINTRESOURCE(IDD_SIMULATIONSTATUS), GameGetRootWindowHandle(), StatusDialogProc, NULL);
	if (!hStatusDialog) {
		ConsoleLog(LOG_ERROR, "CORE: Couldn't create status dialog: 0x%08X\n", GetLastError());
		return;
	}
}

static void DestroyFloatingStatusDialog(void) {
	if (!hStatusDialog)
		return;

	if (hGotoButton && OldGotoButtonWndProc && (WNDPROC)GetWindowLong(hGotoButton, GWL_WNDPROC) == NewGotoButtonWndProc)
		SubclassWindow(hGotoButton, OldGotoButtonWndProc);

	DestroyWindow(hStatusDialog);
}

extern "C" BOOL __stdcall Hook_StatusControlBarCreateStatusBar_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CStatusControlBar *pThis;

	__asm mov[pThis], ecx

	int(__thiscall *H_CDialogBarCreate)(CMFC3XDialogBar *, CMFC3XWnd *, LPCTSTR, UINT, UINT) = (int(__thiscall *)(CMFC3XDialogBar *, CMFC3XWnd *, LPCTSTR, UINT, UINT))0x4B5801;

	int ret;

	// It's necessary to call CDialogBar::Create directly rather than
	// the CStatusControlBar::CreateStatusBar call in order to avoid a crash.
	pSCApp = Game_GetSimcityAppClassPointer();
	ret = H_CDialogBarCreate(pThis, pSCApp->m_pMainWnd, (LPCSTR)255, (0x8000 | 0x0200), 111);
	if (ret) {
		ptFloat.x = 360;
		ptFloat.y = 160;
		szFloat.cx = 260;
		szFloat.cy = 40;
		CreateFloatingStatusDialog();
		UpdateStatus_SC2K1996(-1);
	}
	return ret;
}

extern "C" void __stdcall Hook_StatusControlBarDestructStatusBar_SC2K1996() {
	CStatusControlBar *pThis;

	__asm mov[pThis], ecx

	void(__thiscall *H_StatusControlBarDestructStatusBar)(CStatusControlBar *) = (void(__thiscall *)(CStatusControlBar *))0x489C80;

	DestroyFloatingStatusDialog();

	H_StatusControlBarDestructStatusBar(pThis);
}

extern "C" void __stdcall Hook_StatusControlBarUpdateStatusBar_SC2K1996(int iEntry, char *szText, int iArgUnknown, COLORREF newColor) {
	CStatusControlBar *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_CStatusControlBarUpdateStatusBar)(CStatusControlBar *, int, char *, int, COLORREF) = (void(__thiscall *)(CStatusControlBar *, int, char *, int, COLORREF))0x489D50;
	CMFC3XString *(__thiscall *H_CStringOperatorSet)(CMFC3XString *, char *) = (CMFC3XString *(__thiscall *)(CMFC3XString *, char *))0x4A2E6A;

	if (CanUseFloatingStatusDialog()) {
		if (iEntry) {
			if (iEntry == 1) {
				H_CStringOperatorSet(&pThis->dwSCBCStringNotification, szText);
				pThis->dwSCBColorNotification = newColor;
			}
			/*
			 * 'iEntry == 2' is excluded in this case since the 'GoTo' button image changing is handled elsewhere.
			 */
		}
		else {
			H_CStringOperatorSet(&pThis->dwSCBCStringCtrlSelection, szText);
			pThis->dwSCBColorCtrlSelection = newColor;
		}
		RedrawWindow(hStatusDialog, 0, 0, RDW_INVALIDATE);
	}
	else
		H_CStatusControlBarUpdateStatusBar(pThis, iEntry, szText, iArgUnknown, newColor);
}

extern "C" void __stdcall Hook_MainFrameToggleStatusControlBar_SC2K1996(BOOL bShow) {
	CMainFrame *pThis;

	__asm mov [pThis], ecx

	CStatusControlBar *pStatusBar;

	pStatusBar = &pThis->dwMFStatusControlBar;
	if (pThis->bMFShowStatusBar != bShow) {
		if (!CanUseFloatingStatusDialog())
			ShowWindow(pStatusBar->m_hWnd, (bShow) ? SW_SHOW : SW_HIDE);
		pThis->bMFShowStatusBar = (bShow) ? TRUE: FALSE;
		UpdateStatus_SC2K1996(bShow);
	}
}

extern "C" void __stdcall Hook_MainFrameToggleToolBars_SC2K1996(BOOL bShow) {
	CMainFrame *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_CFrameWndRecalcLayout)(CMFC3XFrameWnd *, int) = (void(__thiscall *)(CMFC3XFrameWnd *, int))0x4BB23A;

	BOOL bToolBarsCreated;
	CCityToolBar *pCityToolBar;
	CMapToolBar *pMapToolBar;

	BOOL &bMainFrameInactive = *(BOOL *)0x4CAD14;

	bToolBarsCreated = pThis->dwMFToolBarsCreated;
	pCityToolBar = &pThis->dwMFCityToolBar;
	pMapToolBar = &pThis->dwMFMapToolBar;
	if (bToolBarsCreated && Game_PointerToCSimcityViewClass(&pCSimcityAppThis)) {
		if (bShow) {
			if (bMainFrameInactive)
				return;
			if (wCityMode) {
				if (CanUseFloatingStatusDialog())
					ShowWindow(hStatusDialog, SW_SHOWNORMAL);
				if (ShowWindow(pCityToolBar->m_hWnd, SW_SHOWNORMAL))
					return;
				UpdateWindow(pCityToolBar->m_hWnd);
			}
			else {
				if (CanUseFloatingStatusDialog())
					ShowWindow(hStatusDialog, SW_HIDE);
				if (ShowWindow(pMapToolBar->m_hWnd, SW_SHOWNORMAL))
					return;
				UpdateWindow(pMapToolBar->m_hWnd);
			}
		}
		else if (wCityMode) {
			if (!ShowWindow(pCityToolBar->m_hWnd, SW_HIDE))
				return;
		}
		else if (!ShowWindow(pMapToolBar->m_hWnd, SW_HIDE)) {
			return;
		}
		if (!bShow) {
			if (CanUseFloatingStatusDialog())
				ShowWindow(hStatusDialog, SW_HIDE);
			H_CFrameWndRecalcLayout(pThis, 1);
			InvalidateRect(pThis->m_hWnd, 0, 1);
			UpdateWindow(pThis->m_hWnd);
		}
	}
}

void InstallStatusHooks_SC2K1996(void) {
	// Load weather icons
	for (int i = 0; i < 13; i++) {
		HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_WEATHER0 + i), IMAGE_BITMAP, 32, 32, NULL);
		if (hBitmap)
			hWeatherBitmaps[i] = hBitmap;
		else
			ConsoleLog(LOG_ERROR, "MISC: Couldn't load weather bitmap IDB_WEATHER%i: 0x%08X\n", i, GetLastError());
	}

	// Load compass icons
	for (int i = 0; i < 4; i++) {
		HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_COMPASS0 + i), IMAGE_BITMAP, 38, 38, NULL);
		if (hBitmap)
			hCompassBitmaps[i] = hBitmap;
		else
			ConsoleLog(LOG_ERROR, "MISC: Couldn't load compass bitmap IDB_COMPASS%i: 0x%08X\n", i, GetLastError());
	}

	// Hook for CStatusControlBar::CreateStatusBar
	VirtualProtect((LPVOID)0x40173A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40173A, Hook_StatusControlBarCreateStatusBar_SC2K1996);

	// Hook for CStatusControlBar::~CStatusControlBar
	VirtualProtect((LPVOID)0x40240F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40240F, Hook_StatusControlBarDestructStatusBar_SC2K1996);

	// Hook for CStatusControlBar::UpdateStatusBar
	VirtualProtect((LPVOID)0x40204F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40204F, Hook_StatusControlBarUpdateStatusBar_SC2K1996);

	// Hook for CMainFrame::ToggleStatusControlBar
	VirtualProtect((LPVOID)0x4021A8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021A8, Hook_MainFrameToggleStatusControlBar_SC2K1996);

	// Hook for CMainFrame::ToggleToolBars
	VirtualProtect((LPVOID)0x40103C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40103C, Hook_MainFrameToggleToolBars_SC2K1996);
}

void UpdateStatus_SC2K1996(int iShow) {
	void(__thiscall *H_CFrameWndShowControlBar)(CMFC3XFrameWnd *, CMFC3XControlBar *, BOOL, int) = (void(__thiscall *)(CMFC3XFrameWnd *, CMFC3XControlBar *, BOOL, int))0x4BA3A0;

	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CStatusControlBar *pStatusBar;
	CSimcityView *pSCView;
	BOOL bShow;
	UINT wPFlags;

	pSCApp = Game_GetSimcityAppClassPointer();
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	pStatusBar = &pMainFrm->dwMFStatusControlBar;
	pSCView = Game_PointerToCSimcityViewClass(pSCApp);
	bShow = (pSCView && wCityMode > 0 && iShow != 0) ? TRUE : FALSE;
	wPFlags = 0;
	if (CanUseFloatingStatusDialog()) {
		if (iShow == -1)
			wPFlags |= SWP_NOZORDER;
		wPFlags |= SWP_NOACTIVATE;
		if (bShow)
			wPFlags |= SWP_SHOWWINDOW;
	}
	else
		wPFlags |= SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW;
	if (hStatusDialog)
		SetWindowPos(hStatusDialog, HWND_TOP, ptFloat.x, ptFloat.y, szFloat.cx, szFloat.cy, wPFlags);
	if (pMainFrm && pStatusBar) {
		ShowWindow(pStatusBar->m_hWnd, (bShow && !CanUseFloatingStatusDialog()) ? SW_SHOW : SW_HIDE);
		H_CFrameWndShowControlBar(pMainFrm, pStatusBar, ((CanUseFloatingStatusDialog()) ? 0 : bShow), 0);
	}
}
