// sc2kfix modules/status_dialog.cpp: recreation of the DOS/Mac version status dialog
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
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

static DWORD dwDummy;

HANDLE hWeatherBitmaps[13];
HANDLE hCompassBitmaps[4];

static tagPOINT ptFloat;
static tagSIZE szFloat;
static tagRECT rGoTo;

static void RemoveGotoButtonFocus(HWND hWnd) {
	HWND hGotoBut;
	UINT nStyle;

	hGotoBut = GetDlgItem(hWnd, 120);
	nStyle = GetWindowLongA(hGotoBut, GWL_STYLE);
	nStyle &= ~BS_DEFPUSHBUTTON;
	SetWindowLongA(hGotoBut, GWL_STYLE, nStyle);
}

static void AdjustGotoButton(HWND hWnd) {
	HWND hWndGoto;
	tagRECT r;
	LONG x, y, cx, cy;
	UINT nStyle, nExStyle;

	hWndGoto = GetDlgItem(hWnd, 120);
	if (hWndGoto) {
		nStyle = GetWindowLongA(hWndGoto, GWL_STYLE);
		nExStyle = GetWindowLongA(hWndGoto, GWL_EXSTYLE);
		if (bSettingsUseStatusDialog) {
			nStyle |= BS_FLAT | BS_BITMAP | BS_NOTIFY;
			if (nStyle & WS_DISABLED)
				nStyle &= ~WS_DISABLED;
			if (nStyle & BS_DEFPUSHBUTTON)
				nStyle &= ~BS_DEFPUSHBUTTON;
			nStyle |= WS_VISIBLE;
			nExStyle |= WS_EX_TOOLWINDOW;
			GetClientRect(hWnd, &r);
			x = r.right - 40;
			y = r.top;
			cx = cy = 40;
		}
		else {
			if (nStyle & BS_FLAT)
				nStyle &= ~BS_FLAT;
			if (nStyle & BS_BITMAP)
				nStyle &= ~BS_BITMAP;
			if (dwDisasterActive)
				nStyle &= ~WS_DISABLED;
			else
				nStyle |= WS_DISABLED;
			nStyle |= BS_DEFPUSHBUTTON;
			if (nExStyle & WS_EX_TOOLWINDOW)
				nExStyle &= ~WS_EX_TOOLWINDOW;
			SendMessage(hWndGoto, BM_SETIMAGE, IMAGE_BITMAP, 0);
			x = rGoTo.left;
			y = rGoTo.top;
			cx = rGoTo.right - rGoTo.left;
			cy = rGoTo.bottom - rGoTo.top;
		}
		SetWindowLongA(hWndGoto, GWL_STYLE, nStyle);
		SetWindowLongA(hWndGoto, GWL_EXSTYLE, nExStyle);
		SetWindowPos(hWndGoto, HWND_TOP, x, y, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

extern "C" BOOL __stdcall Hook_StatusControlBarCreateStatusBar_SC2K1996() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	int(__thiscall *H_CDialogBarCreate)(void *, void *, LPCTSTR, UINT, UINT) = (int(__thiscall *)(void *, void *, LPCTSTR, UINT, UINT))0x4B5801;

	int ret;

	ret = H_CDialogBarCreate(pThis, pCWndRootWindow, (LPCSTR)255, (DS_SETFONT | 0x8000 | 0x0200), 0x6F);
	if (ret) {
		if (!bFontsInitialized)
			InitializeFonts();
		ptFloat.x = 360;
		ptFloat.y = 160;
		szFloat.cx = 250;
		szFloat.cy = 40;
		// Record the original GoTo button position.
		GetClientRect(GetDlgItem((HWND)pThis[7], 120), &rGoTo);
		UpdateStatus_SC2K1996(-1);
		return ret;
	}
	else {
		ConsoleLog(LOG_DEBUG, "Failed to create Status Bar.\n");
	}

	return 0;
}

extern "C" BOOL __stdcall Hook_MainFrameDoStatusControlBarSize_SC2K1996() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	BOOL(__thiscall *H_MainFrameDoStatusControlBarSize)(void *) = (BOOL(__thiscall *)(void *))0x40C670;

	return (bSettingsUseStatusDialog) ? 0 : H_MainFrameDoStatusControlBarSize(pThis);
}

extern "C" BOOL __stdcall Hook_StatusControlBarMoveGotoButton_SC2K1996() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	HWND hDlgItem, hWnd, hWndFromHandle;
	DWORD *pWnd;
	tagRECT r1;
	tagRECT r2;

	if (!bSettingsUseStatusDialog) {
		hWnd = (HWND)pThis[7];
		hDlgItem = GetDlgItem(hWnd, 120); // "GoTo" button.
		pWnd = Game_CWnd_FromHandle(hDlgItem);
		hWndFromHandle = (HWND)pWnd[7];
		GetClientRect(hWnd, &r1);
		GetWindowRect(hWndFromHandle, &r2);
		ScreenToClient(hWnd, (LPPOINT)&r2.left);
		ScreenToClient(hWnd, (LPPOINT)&r2.right);
		return MoveWindow(hWndFromHandle,
			r2.left - r2.right + r1.right - 5,
			r2.top,
			r2.right - r2.left,
			r2.bottom - r2.top,
			TRUE);
	}
}

extern "C" BOOL __stdcall Hook_StatusControlBarUpdateStatusBar_SC2K1996(int iEntry, char *szText, int iArgUnknown, COLORREF newColor) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	BOOL(__thiscall *H_CStatusControlBarUpdateStatusBar)(void *, int, char *, int, COLORREF) = (BOOL(__thiscall *)(void *, int, char *, int, COLORREF))0x489D50;
	CMFC3XString *(__thiscall *H_CStringOperatorSet)(CMFC3XString *, char *) = (CMFC3XString *(__thiscall *)(CMFC3XString *, char *))0x4A2E6A;

	if (bSettingsUseStatusDialog) {
		if (iEntry) {
			if (iEntry == 1) {
				H_CStringOperatorSet((CMFC3XString *)&pThis[31], szText);
				pThis[38] = (DWORD)newColor;
			}
			else if (iEntry == 2) {
				if (dwDisasterActive) {
					SendMessage(GetDlgItem((HWND)pThis[7], 120), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[12]);
				}
				else {
					if (!IsWindowEnabled(GetDlgItem((HWND)pThis[7], 120)))
						EnableWindow(GetDlgItem((HWND)pThis[7], 120), TRUE);
					RemoveGotoButtonFocus((HWND)pThis[7]);
					SendMessage(GetDlgItem((HWND)pThis[7], 120), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[bWeatherTrend]);
				}
				return InvalidateRect(GetDlgItem((HWND)pThis[7], 120), 0, TRUE);
			}
		}
		else {
			H_CStringOperatorSet((CMFC3XString *)&pThis[28], szText);
			pThis[37] = (DWORD)newColor;
		}
		return InvalidateRect((HWND)pThis[7], 0, TRUE);
	}
	else
		return H_CStatusControlBarUpdateStatusBar(pThis, iEntry, szText, iArgUnknown, newColor);
}

extern "C" void __stdcall Hook_StatusControlBarOnPaint_SC2K1996() {
	DWORD *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_StatusControlBarOnPaint_SC2K1996)(void *) = (void(__thiscall *)(void *))0x489F20;
	void(__thiscall *H_ControlBarOnPaint)(void *) = (void(__thiscall *)(void *))0x4B4E7E;

	COLORREF &colBtnFace = *(COLORREF *)0x4CB3FC;
	DWORD *MainBrushFace = (DWORD *)0x4CAA48;

	LPCSTR pStringOne;
	LPCSTR pStringTwo;
	COLORREF crOne;
	COLORREF crTwo;
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
	tagRECT r;
	HDC hDC, hDCBits;
	tagBITMAP bm;

	if (bSettingsUseStatusDialog) {
		pStringOne = (LPCSTR)pThis[28];
		pStringTwo = (LPCSTR)pThis[31];
		crOne = (COLORREF)pThis[37];
		crTwo = (COLORREF)pThis[38];
		H_ControlBarOnPaint(pThis);

		left = 0;
		top = 3;
		right = 180;
		bottom = 15;

		hDC = GetDC((HWND)pThis[7]);
		SelectFont(hDC, hFontMSSansSerifBold8);
		SetRect(&r, left, top, right, bottom);
		OffsetRect(&r, 5, 2);
		FillRect(hDC, &r, (HBRUSH)MainBrushFace[1]);
		SetTextAlign(hDC, 8);
		SetBkColor(hDC, colBtnFace);
		SetTextColor(hDC, crOne);
		ExtTextOutA(hDC, r.left, r.bottom, ETO_CLIPPED, &r, pStringOne, lstrlenA(pStringOne), 0);

		top += 18;
		bottom += 18;

		SelectFont(hDC, hFontMSSansSerifRegular8);
		SetRect(&r, left, top, right, bottom);
		OffsetRect(&r, 5, 2);
		FillRect(hDC, &r, (HBRUSH)MainBrushFace[1]);
		SetTextAlign(hDC, 8);
		SetBkColor(hDC, colBtnFace);
		SetTextColor(hDC, crTwo);
		ExtTextOutA(hDC, r.left, r.bottom, ETO_CLIPPED, &r, pStringTwo, lstrlenA(pStringTwo), 0);

		left = 168;
		top = 0;
		right = 208;
		bottom = 38;

		SetRect(&r, left, top, right, bottom);
		OffsetRect(&r, 5, 1);
		FillRect(hDC, &r, (HBRUSH)MainBrushFace[1]);
		hDCBits = CreateCompatibleDC(hDC);
		GetObject(hCompassBitmaps[wViewRotation], sizeof(tagBITMAP), &bm);
		SelectObject(hDCBits, hCompassBitmaps[wViewRotation]);
		BitBlt(hDC, r.left, r.top, bm.bmWidth, bm.bmHeight, hDCBits, 0, 0, SRCCOPY);
		DeleteDC(hDCBits);
		ReleaseDC((HWND)pThis[7], hDC);
	}
	else
		H_StatusControlBarOnPaint_SC2K1996(pThis);
}

extern "C" void __stdcall Hook_MainFrameToggleStatusControlBar_SC2K1996(BOOL bShow) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	DWORD *pStatusBar;
	HWND hStatusBar;

	pStatusBar = &pThis[61];
	hStatusBar = (HWND)pStatusBar[7];
	if (pThis[101] != bShow) {
		if (bShow) {
			ShowWindow(hStatusBar, SW_SHOW);
			pThis[101] = TRUE;
		}
		else {
			ShowWindow(hStatusBar, SW_HIDE);
			pThis[101] = FALSE;
		}
		UpdateStatus_SC2K1996(bShow);
	}
}

extern "C" void __stdcall Hook_MainFrameToggleToolBars_SC2K1996(BOOL bShow) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_CFrameWndRecalcLayout)(void *, int) = (void(__thiscall *)(void *, int))0x4BB23A;

	BOOL bToolBarsCreated;
	DWORD *pCityToolBar;
	DWORD *pMapToolBar;
	DWORD *pStatusBar;

	BOOL &bMainFrameInactive = *(BOOL *)0x4CAD14;

	bToolBarsCreated = pThis[289];
	pCityToolBar = &pThis[102];
	pMapToolBar = &pThis[233];
	pStatusBar = &pThis[61];
	if (bToolBarsCreated && Game_PointerToCSimcityViewClass(&pCSimcityAppThis)) {
		if (bShow) {
			if (bMainFrameInactive)
				return;
			if (wCityMode) {
				if (bSettingsUseStatusDialog)
					ShowWindow((HWND)pStatusBar[7], SW_SHOWNORMAL);
				if (ShowWindow((HWND)pCityToolBar[7], SW_SHOWNORMAL))
					return;
				UpdateWindow((HWND)pCityToolBar[7]);
			}
			else {
				if (bSettingsUseStatusDialog)
					ShowWindow((HWND)pStatusBar[7], SW_HIDE);
				if (ShowWindow((HWND)pMapToolBar[7], SW_SHOWNORMAL))
					return;
				UpdateWindow((HWND)pMapToolBar[7]);
			}
		}
		else if (wCityMode) {
			if (!ShowWindow((HWND)pCityToolBar[7], SW_HIDE))
				return;
		}
		else if (!ShowWindow((HWND)pMapToolBar[7], SW_HIDE)) {
			return;
		}
		if (!bShow) {
			if (bSettingsUseStatusDialog)
				ShowWindow((HWND)pStatusBar[7], SW_HIDE);
			H_CFrameWndRecalcLayout(pThis, 1);
			InvalidateRect((HWND)pThis[7], 0, 1);
			UpdateWindow((HWND)pThis[7]);
		}
	}
}

extern "C" LRESULT __stdcall Hook_ControlBarWindowProc_SC2K1996(UINT Msg, WPARAM wParam, LPARAM lParam) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	LRESULT(__thiscall *H_CWndWindowProc)(void *, UINT, WPARAM, LPARAM) = (LRESULT(__thiscall *)(void *, UINT, WPARAM, LPARAM))0x4A4B70;

	DWORD *pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	tagRECT r;
	HWND m_hWndOwner;
	DWORD *pWnd;

	if (Msg < WM_DRAWITEM || Msg > WM_CHARTOITEM && Msg != WM_COMPAREITEM && Msg != WM_COMMAND) {
		if (pThis == pStatusBar && bSettingsUseStatusDialog) {
			if (Msg == WM_SETCURSOR) {
				if (bSettingsUseStatusDialog) {
					Game_SimcityAppSetGameCursor(&pCSimcityAppThis, 0, 0);
					dwCursorGameHit = 4;
				}
				return TRUE;
			}
			else if (Msg == WM_LBUTTONDOWN) {
				RemoveGotoButtonFocus((HWND)pThis[7]);
				SendMessageA((HWND)pThis[7], WM_SYSCOMMAND, (SC_MOVE | HTCAPTION), 0);

				// Record the new position.
				GetWindowRect((HWND)pThis[7], &r);
				ptFloat.x = r.left;
				ptFloat.y = r.top;

				if (!dwDisasterActive)
					SetFocus(NULL);
				return TRUE;
			}
		}
		return H_CWndWindowProc(pThis, Msg, wParam, lParam);
	}
	if (pThis == pStatusBar && bSettingsUseStatusDialog) {
		// No parent, the GoTo button goes straight to the CMainFrame.
		if (Msg == WM_COMMAND) {
			if (GET_WM_COMMAND_ID(wParam, lParam) == 120) {
				switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
				case BN_CLICKED:
					RemoveGotoButtonFocus((HWND)pThis[7]);
					if (!dwDisasterActive) {
						SetFocus(NULL);
						return TRUE;
					}
					break;
				default:
					break;
				}
			}
		}
		return SendMessageA(GameGetRootWindowHandle(), Msg, wParam, lParam);
	}
	m_hWndOwner = (HWND)pThis[8];
	if (!m_hWndOwner)
		m_hWndOwner = GetParent((HWND)pThis[7]);
	pWnd = Game_CWnd_FromHandle(m_hWndOwner);
	return SendMessageA((HWND)pWnd[7], Msg, wParam, lParam);
}

extern "C" void __stdcall Hook_ControlBarDoPaint_SC2K1996(DWORD *pDC) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_ControlBarDrawBorders)(void *, DWORD *, RECT *) = (void(__thiscall *)(void *, DWORD *, RECT *))0x4B53EB;

	DWORD *MainBrushBorder = (DWORD *)0x4CB1B0;

	DWORD *pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	tagRECT r;
	LONG iHeight;

	GetClientRect((HWND)pThis[7], &r);
	if (pThis == pStatusBar && bSettingsUseStatusDialog) {
		iHeight = r.bottom - r.top;
		if (iHeight > 40) {
			r.bottom = 40;
			FrameRect((HDC)pDC[1], &r, (HBRUSH)MainBrushBorder[1]);
			r.bottom = r.top + iHeight;
		}
		FrameRect((HDC)pDC[1], &r, (HBRUSH)MainBrushBorder[1]);
	}
	else {
		H_ControlBarDrawBorders(pThis, pDC, &r);
	}
}

void InstallStatusHooks_SC2K1996(void) {
	// Hook for CStatusControlBar::CreateStatusBar
	VirtualProtect((LPVOID)0x40173A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40173A, Hook_StatusControlBarCreateStatusBar_SC2K1996);

	// Hook for CMainFrame::DoStatusControlBarSize
	VirtualProtect((LPVOID)0x40285B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40285B, Hook_MainFrameDoStatusControlBarSize_SC2K1996);

	// Hook for CStatusControlBar::MoveGotoButton
	VirtualProtect((LPVOID)0x40126C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40126C, Hook_StatusControlBarMoveGotoButton_SC2K1996);

	// Hook for CStatusControlBar::UpdateStatusBar
	VirtualProtect((LPVOID)0x40204F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40204F, Hook_StatusControlBarUpdateStatusBar_SC2K1996);

	// Hook for CStatusControlBar::OnPaint
	VirtualProtect((LPVOID)0x402B67, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B67, Hook_StatusControlBarOnPaint_SC2K1996);

	// Hook for CMainFrame::ToggleStatusControlBar
	VirtualProtect((LPVOID)0x4021A8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021A8, Hook_MainFrameToggleStatusControlBar_SC2K1996);

	// Hook for CMainFrame::ToggleToolBars
	VirtualProtect((LPVOID)0x40103C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40103C, Hook_MainFrameToggleToolBars_SC2K1996);

	// Hook for CControlBar::WindowProc
	// Added additional checks for the following:
	// 1) LButtonDown Dragging.
	// 2) Removing the BS_DEFPUSHBUTTON attribute (and killing focus while
	//    no disaster is active).
	// 3) Change to the normal 'Arrow' cursor while the pointer is within
	//    the dimensions of the status widget.
	VirtualProtect((LPVOID)0x4B4B46, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B4B46, Hook_ControlBarWindowProc_SC2K1996);

	// Hook for CControlBar::DoPaint
	VirtualProtect((LPVOID)0x4B53B6, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B53B6, Hook_ControlBarDoPaint_SC2K1996);
}

void UpdateStatus_SC2K1996(int iShow) {
	void(__thiscall *H_CFrameWndShowControlBar)(void *, void *, BOOL, int) = (void(__thiscall *)(void *, void *, BOOL, int))0x4BA3A0;

	DWORD *pStatusBar;
	DWORD *pSCView;
	BOOL bShow;
	HWND hWndStatusBar;
	UINT wPFlags;

	pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	pSCView = Game_PointerToCSimcityViewClass(&pCSimcityAppThis);
	bShow = (pSCView && wCityMode > 0 && iShow != 0) ? TRUE : FALSE;
	hWndStatusBar = (HWND)pStatusBar[7];
	wPFlags = SWP_NOZORDER | SWP_NOACTIVATE;
	if (bShow)
		wPFlags |= SWP_SHOWWINDOW;
	if (bSettingsUseStatusDialog) {
		wPFlags &= ~SWP_NOZORDER;
		SetWindowLongA(hWndStatusBar, GWL_STYLE, DS_SETFONT | WS_POPUP);
		SetWindowLongA(hWndStatusBar, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
		SetParent(hWndStatusBar, NULL);
		SetWindowPos(hWndStatusBar, HWND_TOPMOST, ptFloat.x, ptFloat.y, szFloat.cx, szFloat.cy, wPFlags);
		if (bShow)
			AdjustGotoButton(hWndStatusBar);
	}
	else {
		SetWindowLongA(hWndStatusBar, GWL_STYLE, DS_SETFONT | WS_CHILD | WS_CLIPSIBLINGS | 0x8000 | 0x0200);
		SetParent(hWndStatusBar, GameGetRootWindowHandle());
		if (bShow)
			AdjustGotoButton(hWndStatusBar);
		SetWindowPos(hWndStatusBar, HWND_TOP, 0, 0, 0, 0, wPFlags);
	}
	if (pSCView && pCWndRootWindow && pStatusBar)
		H_CFrameWndShowControlBar(pCWndRootWindow, pStatusBar, bShow, 0);
}
