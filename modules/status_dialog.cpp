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

extern "C" BOOL __stdcall Hook_StatusControlBarCreateStatusBar_SC2K1996() {
	DWORD *pThis;

	__asm mov[pThis], ecx

	int(__thiscall *H_CDialogBarCreate)(void *, void *, LPCTSTR, UINT, UINT) = (int(__thiscall *)(void *, void *, LPCTSTR, UINT, UINT))0x4B5801;

	HINSTANCE &gamehInstance = *(HINSTANCE *)0x4CE8C4;

	UINT nFlags;
	int ret;

	nFlags = (DS_SETFONT | 0x8000 | 0x0200);

	ret = H_CDialogBarCreate(pThis, pCWndRootWindow, (LPCSTR)255, nFlags, 0x6F);
	if (ret) {
		if (!bFontsInitialized)
			InitializeFonts();
		ptFloat.x = 360;
		ptFloat.y = 160;
		szFloat.cx = 250;
		szFloat.cy = 40;
		// Record the original GoTo button position.
		GetClientRect(GetDlgItem((HWND)pThis[7], 120), &rGoTo);
		//ConsoleLog(LOG_DEBUG, "(%d, %d, %d, %d)\n", rGoTo.left, rGoTo.top, rGoTo.right, rGoTo.bottom);
		ConsoleLog(LOG_DEBUG, "Create Status Bar: %d\n", ret);
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

	DWORD *(__stdcall *H_CWndFromHandle)(HWND) = (DWORD *(__stdcall *)(HWND))0x4A3BDF;

	HWND hDlgItem, hWnd, hWndFromHandle;
	DWORD *pWnd;
	tagRECT r1;
	tagRECT r2;

	if (!bSettingsUseStatusDialog) {
		hWnd = (HWND)pThis[7];
		hDlgItem = GetDlgItem(hWnd, 120); // "GoTo" button.
		pWnd = H_CWndFromHandle(hDlgItem);
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
				(COLORREF)pThis[38] = newColor;
			}
			else if (iEntry == 2) {
				if (dwDisasterActive) {
					SendMessage(GetDlgItem((HWND)pThis[7], 120), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[12]);
				}
				else {
					if (!IsWindowEnabled(GetDlgItem((HWND)pThis[7], 120)))
						EnableWindow(GetDlgItem((HWND)pThis[7], 120), TRUE);
					SendMessage(GetDlgItem((HWND)pThis[7], 120), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[bWeatherTrend]);
				}
				InvalidateRect(GetDlgItem((HWND)pThis[7], 120), NULL, TRUE);
			}
		}
		else {
			H_CStringOperatorSet((CMFC3XString *)&pThis[28], szText);
			(COLORREF)pThis[37] = newColor;
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
		//ConsoleLog(LOG_DEBUG, "[%s] [%s] (%d, %d, %d, %d) (%d, %d)\n", pStringOne, pStringTwo, cR.left, cR.top, cR.right, cR.bottom, cR.right - cR.left, cR.bottom - cR.top);
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

	void(__thiscall *H_CFrameWndShowControlBar)(void *, void *, BOOL, int) = (void(__thiscall *)(void *, void *, BOOL, int))0x4BA3A0;

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

	DWORD *(__stdcall *H_CWndFromHandle)(HWND) = (DWORD *(__stdcall *)(HWND))0x4A3BDF;
	LRESULT(__thiscall *H_CWndWindowProc)(void *, UINT, WPARAM, LPARAM) = (LRESULT(__thiscall *)(void *, UINT, WPARAM, LPARAM))0x4A4B70;

	DWORD *pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	HWND m_hWndOwner;
	DWORD *pWnd;

	if (Msg < WM_DRAWITEM || Msg > WM_CHARTOITEM && Msg != WM_COMPAREITEM && Msg != WM_COMMAND)
		return H_CWndWindowProc(pThis, Msg, wParam, lParam);
	if (pThis == pStatusBar && bSettingsUseStatusDialog) {
		// No parent, the GoTo button goes straight to the CMainFrame.
		return SendMessageA(GameGetRootWindowHandle(), Msg, wParam, lParam);
	}
	m_hWndOwner = (HWND)pThis[8];
	if (!m_hWndOwner)
		m_hWndOwner = GetParent((HWND)pThis[7]);
	pWnd = H_CWndFromHandle(m_hWndOwner);
	return SendMessageA((HWND)pWnd[7], Msg, wParam, lParam);
}

extern "C" SIZE *__stdcall Hook_DialogBarCalcFixedLayout_SC2K1996(SIZE *pSZ, BOOL bStretch, BOOL bHorz) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	LONG lX;
	LONG lY;

	if (bStretch) {
		if (bSettingsUseStatusDialog) {
			lX = szFloat.cx;
			lY = szFloat.cy;
		}
		else {
			lY = 0x7FFF;
			if (bHorz) {
				lY = pThis[27];
				lX = 0x7FFF;
			}
			else {
				lX = pThis[26];
			}
		}
		pSZ->cx = lX;
		pSZ->cy = lY;
	}
	else {
		pSZ->cx = pThis[26];
		pSZ->cy = pThis[27];
	}
	return pSZ;
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

extern "C" void __stdcall Hook_CControlBarOnLButtonDown_SC2K1996(UINT nFlags, tagPOINT pt) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	long(__thiscall *H_CWndDefault)(void *) = (long(__thiscall *)(void *))0x4A3B2F;
	void(__thiscall *H_CDockContextStartDrag)(void *, tagPOINT) = (void(__thiscall *)(void *, tagPOINT))0x4B5C25;

	tagRECT r;

	DWORD *pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	if (pThis == pStatusBar && bSettingsUseStatusDialog) {
		SendMessageA((HWND)pThis[7], WM_SYSCOMMAND, (SC_MOVE | HTCAPTION), 0);
		
		// Record the new position.
		GetWindowRect((HWND)pThis[7], &r);
		ptFloat.x = r.left;
		ptFloat.y = r.top;
	}
	else {
		H_CWndDefault(pThis);
		if (pThis[24]) {
			ClientToScreen((HWND)pThis[7], &pt);
			H_CDockContextStartDrag(&pThis[25], pt);
		}
	}
}

extern "C" BOOL __stdcall Hook_StatusOnSetCursor(UINT nHitTest, UINT Message) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	if (bSettingsUseStatusDialog) {
		Game_SimcityAppSetGameCursor(&pCSimcityAppThis, 0, 0);
		dwCursorGameHit = 4;
	}
	return TRUE;
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
	VirtualProtect((LPVOID)0x4B4B46, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B4B46, Hook_ControlBarWindowProc_SC2K1996);

	// Hook for CDialogBar::CalcFixedLayout
	VirtualProtect((LPVOID)0x4B5973, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B5973, Hook_DialogBarCalcFixedLayout_SC2K1996);

	// Hook for CControlBar::DoPaint
	VirtualProtect((LPVOID)0x4B53B6, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B53B6, Hook_ControlBarDoPaint_SC2K1996);

	// Hook for CControlBar::OnLButtonDown
	// Added an additional check in the hook to account
	// for a match with the StatusControlBar in order to allow for
	// dragging.
	VirtualProtect((LPVOID)0x4B4F8A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4B4F8A, Hook_CControlBarOnLButtonDown_SC2K1996);

	// Only enough space to replace WM_NULL with WM_SETCURSOR.
	AFX_MSGMAP_ENTRY afxMessageMapEntry;

	afxMessageMapEntry = {
		WM_SETCURSOR,
		0,
		0,
		0,
		3,
		Hook_StatusOnSetCursor,
	};

	VirtualProtect((LPVOID)0x4DEAF8, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy_s((LPVOID)0x4DEAF8, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));
}

void UpdateStatus_SC2K1996(int iShow) {
	void(__thiscall *H_CFrameWndShowControlBar)(void *, void *, BOOL, int) = (void(__thiscall *)(void *, void *, BOOL, int))0x4BA3A0;

	BOOL bShow;
	DWORD *pStatusBar;
	HWND hwndStatusBar, hWndGoto;
	UINT wPFlags, butFlags;
	tagRECT r;

	HINSTANCE &gamehInstance = *(HINSTANCE *)0x4CE8C4;

	bShow = (wCityMode > 0 && iShow != 0) ? TRUE : FALSE;
	pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	hwndStatusBar = (HWND)pStatusBar[7];
	hWndGoto = GetDlgItem(hwndStatusBar, 120);
	wPFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW;
	if (bSettingsUseStatusDialog) {
		wPFlags = SWP_HIDEWINDOW;
		if (iShow > 0 && bShow)
			wPFlags = SWP_SHOWWINDOW;
		else if (!iShow || wCityMode <= 0)
			wPFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW;
		SetWindowLongA(hwndStatusBar, GWL_STYLE, DS_SETFONT | WS_POPUP);
		SetWindowLongA(hwndStatusBar, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
		SetParent(hwndStatusBar, NULL);
		SetWindowPos(hwndStatusBar, HWND_TOPMOST, ptFloat.x, ptFloat.y, szFloat.cx, szFloat.cy, wPFlags);
		if (iShow == -1) {
			if (hWndGoto) {
				GetClientRect(hwndStatusBar, &r);
				DestroyWindow(hWndGoto);
				butFlags = WS_CHILD | BS_FLAT | BS_BITMAP | WS_VISIBLE;
				CreateWindowA("Button", "", butFlags, r.right - 40, r.top, 40, 40, hwndStatusBar, (HMENU)120, gamehInstance, NULL);
			}
		}
	}
	else {
		SetWindowLongA(hwndStatusBar, GWL_STYLE, DS_SETFONT | WS_CHILD | WS_CLIPSIBLINGS | 0x8000 | 0x0200);
		SetParent(hwndStatusBar, GameGetRootWindowHandle());
		if (iShow == -1) {
			if (hWndGoto) {
				DestroyWindow(hWndGoto);
				butFlags = WS_CHILD | WS_VISIBLE | WS_DISABLED;
				if (dwDisasterActive)
					butFlags &= ~WS_DISABLED;
				hWndGoto = CreateWindowA("Button", "GoTo", butFlags, rGoTo.left, rGoTo.top, rGoTo.right - rGoTo.left, rGoTo.bottom - rGoTo.top, hwndStatusBar, (HMENU)120, gamehInstance, NULL);
				SendMessageA(hWndGoto, WM_SETFONT, (WPARAM)hFontMSSansSerifRegular8, 1);
			}
		}
		SetWindowPos(hwndStatusBar, HWND_TOP, 0, 0, 0, 0, wPFlags);
	}
	if (pCWndRootWindow && pStatusBar) {
		H_CFrameWndShowControlBar(pCWndRootWindow, pStatusBar, 0, 0);
		H_CFrameWndShowControlBar(pCWndRootWindow, pStatusBar, bShow, 0);
	}
}
