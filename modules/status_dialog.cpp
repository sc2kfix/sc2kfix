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

HWND hStatusDialog = NULL;
HANDLE hWeatherBitmaps[13];
HANDLE hCompassBitmaps[4];
static HCURSOR hDefaultCursor = NULL;
static HWND hwndDesktop;
static RECT rcTemp, rcDlg, rcDesktop;

static COLORREF crToolColor = RGB(0, 0, 0);
static COLORREF crStatusColor = RGB(0, 0, 0);
static HBRUSH hBrushBkg = NULL;

extern "C" int __stdcall Hook_402793(int iStatic, char* szText, int iMaybeAlways1, COLORREF crColor) {
	__asm push ecx

	if (hStatusDialog) {
		SendMessage(GetDlgItem(hStatusDialog, IDC_STATIC_COMPASS), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hCompassBitmaps[wViewRotation]);
		InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_COMPASS), NULL, TRUE);

		if (iStatic == 0) {
			char szCurrentText[200];
			GetDlgItemText(hStatusDialog, IDC_STATIC_SELECTEDTOOL, szCurrentText, 200);
			if (crColor != crToolColor || strcmp(szText, szCurrentText)) {
				SetDlgItemText(hStatusDialog, IDC_STATIC_SELECTEDTOOL, szText);
				crToolColor = crColor;
				InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_SELECTEDTOOL), NULL, TRUE);
			}
		} else if (iStatic == 1) {
			char szCurrentText[200];
			GetDlgItemText(hStatusDialog, IDC_STATIC_STATUSSTRING, szCurrentText, 200);

			std::string strText = szText;
			strText = std::regex_replace(strText, std::regex("&"), "&&");
			if (crColor != crStatusColor || strcmp(strText.c_str(), szCurrentText)) {
				SetDlgItemText(hStatusDialog, IDC_STATIC_STATUSSTRING, strText.c_str());
				crStatusColor = crColor;
				InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_STATUSSTRING), NULL, TRUE);
			}
		} else if (iStatic == 2) {
			if (crStatusColor == RGB(255, 0, 0)) {
				ShowWindow(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), SW_HIDE);
				ShowWindow(GetDlgItem(hStatusDialog, IDC_BUTTON_GOTO), SW_SHOW);
				SendMessage(GetDlgItem(hStatusDialog, IDC_BUTTON_GOTO), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[12]);
			}
			else {
				ShowWindow(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), SW_SHOW);
				ShowWindow(GetDlgItem(hStatusDialog, IDC_BUTTON_GOTO), SW_HIDE);
				SendMessage(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hWeatherBitmaps[bWeatherTrend]);
			}
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_BUTTON_WEATHERICON), NULL, TRUE);
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_BUTTON_GOTO), NULL, TRUE);
		} else
			InvalidateRect(hStatusDialog, NULL, FALSE);
	}

	__asm {
		pop ecx
		push crColor
		push iMaybeAlways1
		push szText
		push iStatic
		mov edi, 0x40BD50
		call edi
	}
}

extern "C" int __stdcall Hook_4021A8(HWND iShow) {
	HWND* iThis;
	__asm mov [iThis], ecx

	HWND iActualShow = iShow;

	if (bSettingsUseStatusDialog)
		iActualShow = 0;

	if (hStatusDialog) {
		int iCmdShow;

		if (!wCityMode)
			iCmdShow = SW_HIDE;
		else
			iCmdShow = (iShow) ? SW_SHOW : SW_HIDE;

		ShowWindow(hStatusDialog, iCmdShow);
	}
	else if (bSettingsUseStatusDialog)
		ShowStatusDialog();

	Game_CFrameWnd_ShowStatusBar(iThis, iActualShow);
}

extern "C" int __stdcall Hook_40103C(int iShow) {
	__asm push ecx

	if (hStatusDialog) {
		int iCmdShow;
		
		if (!wCityMode)
			iCmdShow = SW_HIDE;
		else
			iCmdShow = (iShow) ? SW_SHOW : SW_HIDE;

		ShowWindow(hStatusDialog, iCmdShow);
	}

	__asm {
		pop ecx
		push iShow
		mov edi, 0x40B9E0
		call edi
	}
}

BOOL CALLBACK StatusDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		if (!bFontsInitialized)
			InitializeFonts();
		SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL), WM_SETFONT, (WPARAM)hFontMSSansSerifBold10, TRUE);
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL))
			SetTextColor((HDC)wParam, crToolColor);
		else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_STATUSSTRING))
			SetTextColor((HDC)wParam, crStatusColor);
		
		SetBkColor((HDC)wParam, RGB(240, 240, 240));
		if (!hBrushBkg)
			hBrushBkg = CreateSolidBrush(RGB(240, 240, 240));
		return (LONG)hBrushBkg;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
			case IDC_BUTTON_GOTO:
				if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED) {
					HWND phWnd = GetParent(hwndDlg);
					if (phWnd) {
						HWND dhWnd = GetDlgItem(phWnd, 111);
						if (dhWnd)
							SendMessage(GetDlgItem(dhWnd, 120), BM_CLICK, 0, 0);
					}
					return FALSE;
				}
				break;
		}
		break;

	case WM_LBUTTONDOWN:
		// With the following you'll be able to click the client area
		// of the dialogue and drag it around.
		// The only detail is that the pointer reverts to the one set
		// by Windows not the one that's currently active in the game.
		SendMessage(hwndDlg, WM_SYSCOMMAND, (SC_MOVE | HTCAPTION), 0);
		return FALSE;

	case WM_SETCURSOR:
		Game_SimcityAppSetGameCursor(&pCSimcityAppThis, 0, 0);
		dwCursorGameHit = 4;
		return 1;
	}

	return FALSE;
}

HWND ShowStatusDialog(void) {
	if (hStatusDialog)
		return hStatusDialog;

	hStatusDialog = CreateDialogParam(hSC2KFixModule, MAKEINTRESOURCE(IDD_SIMULATIONSTATUS), GameGetRootWindowHandle(), StatusDialogProc, NULL);
	if (!hStatusDialog) {
		ConsoleLog(LOG_ERROR, "CORE: Couldn't create status dialog: 0x%08X\n", GetLastError());
		return NULL;
	}
	hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &rcDesktop);
	SetWindowPos(hStatusDialog, HWND_TOP, rcDesktop.left + (rcDesktop.right - rcDesktop.left) / 8 + 128, rcDesktop.top + (rcDesktop.bottom - rcDesktop.top) / 8, 0, 0, SWP_NOSIZE);
	ShowWindow(hStatusDialog, SW_HIDE);
	return hStatusDialog;
}

static tagPOINT ptFloat;
static tagSIZE szFloat;
static tagRECT rGoTo;
static HWND hWndSTextCtrl, hWndSTextStat;


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
		ptFloat.x = 300;
		ptFloat.y = 100;
		szFloat.cx = 250;
		szFloat.cy = 40;
		GetClientRect(GetDlgItem((HWND)pThis[7], 120), &rGoTo);
		ConsoleLog(LOG_DEBUG, "(%d, %d, %d, %d)\n", rGoTo.left, rGoTo.top, rGoTo.right, rGoTo.bottom);
		ConsoleLog(LOG_DEBUG, "Create Status Bar: %d\n", ret);
		//hWndSTextCtrl = CreateWindowA("STATIC", "", 0, 0, 0, 0, 0, (HWND)pThis[7], 0, gamehInstance, NULL);
		//if (hWndSTextCtrl)
		//	ConsoleLog(LOG_DEBUG, "Create Status Bar StaticCtrlText: %d\n", hWndSTextCtrl);
		//hWndSTextStat = CreateWindowA("STATIC", "", 0, 0, 0, 0, 0, (HWND)pThis[7], 0, gamehInstance, NULL);
		//if (hWndSTextStat)
		//	ConsoleLog(LOG_DEBUG, "Create Status Bar StaticStatText: %d\n", hWndSTextStat);
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

extern "C" void __stdcall Hook_StatusControlBarOnPaint_SC2K1996() {
	DWORD *pThis;

	__asm mov [pThis], ecx

	void(__thiscall *H_StatusControlBarOnPaint_SC2K1996)(void *) = (void(__thiscall *)(void *))0x489F20;
	void(__thiscall *H_ControlBarOnPaint)(void *) = (void(__thiscall *)(void *))0x4B4E7E;

	if (bSettingsUseStatusDialog)
		H_ControlBarOnPaint(pThis);
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

	GetClientRect((HWND)pThis[7], &r);
	if (pThis == pStatusBar && bSettingsUseStatusDialog) {
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
		SendMessageA((HWND)pStatusBar[7], WM_SYSCOMMAND, (SC_MOVE | HTCAPTION), 0);
		
		// Record the new position.
		GetWindowRect((HWND)pStatusBar[7], &r);
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
	UINT wPFlags;
	tagRECT r;

	bShow = (wCityMode && iShow != 0) ? TRUE : FALSE;
	pStatusBar = &((DWORD *)pCWndRootWindow)[61];
	wPFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW;
	if (bSettingsUseStatusDialog) {
		wPFlags = 0;
		if (iShow > 0 && bShow)
			wPFlags = SWP_NOMOVE | SWP_SHOWWINDOW;
		else if (!iShow || !wCityMode)
			wPFlags = SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE;
		ConsoleLog(LOG_DEBUG, "0x%06X (%d, %d, %u)\n", wPFlags, bShow, iShow, wCityMode);
		SetWindowLongA((HWND)pStatusBar[7], GWL_STYLE, DS_SETFONT | WS_POPUP);
		SetWindowLongA((HWND)pStatusBar[7], GWL_EXSTYLE, WS_EX_TOOLWINDOW);
		SetParent((HWND)pStatusBar[7], NULL);
		SetWindowPos((HWND)pStatusBar[7], HWND_TOPMOST, ptFloat.x, ptFloat.y, szFloat.cx, szFloat.cy, wPFlags);
		GetClientRect((HWND)pStatusBar[7], &r);
		MoveWindow(GetDlgItem((HWND)pStatusBar[7], 120), r.right - 40, r.bottom - 40, 40, 40, TRUE);
	}
	else {
		SetWindowLongA((HWND)pStatusBar[7], GWL_STYLE, DS_SETFONT | WS_CHILD | WS_CLIPSIBLINGS | 0x8000 | 0x0200);
		SetParent((HWND)pStatusBar[7], GameGetRootWindowHandle());
		SetWindowPos((HWND)pStatusBar[7], HWND_TOP, 0, 0, 0, 0, wPFlags);
		MoveWindow(GetDlgItem((HWND)pStatusBar[7], 120), rGoTo.left, rGoTo.top, rGoTo.right - rGoTo.left, rGoTo.bottom - rGoTo.top, TRUE);
	}
	if (pCWndRootWindow && pStatusBar) {
		H_CFrameWndShowControlBar(pCWndRootWindow, pStatusBar, 0, 0);
		H_CFrameWndShowControlBar(pCWndRootWindow, pStatusBar, bShow, 0);
	}
}
