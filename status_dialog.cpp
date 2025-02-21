// sc2kfix statsu_dialog.cpp: recreation of the DOS/Mac version status dialog
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <intrin.h>

#include <sc2kfix.h>
#include "resource.h"

HWND hStatusDialog = NULL;
HFONT hStatusDialogBoldFont = NULL;
static HCURSOR hDefaultCursor = NULL;
static HWND hwndDesktop;
static RECT rcTemp, rcDlg, rcDesktop;

static COLORREF crToolColor = RGB(0, 0, 0);
static COLORREF crStatusColor = RGB(0, 0, 0);

extern "C" int __stdcall Hook_402793(int iStatic, char* szText, int iMaybeAlways1, COLORREF crColor) {
	__asm {
		push ecx
	}
	if (hStatusDialog) {
		if (iStatic == 0) {
			SetDlgItemText(hStatusDialog, IDC_STATIC_SELECTEDTOOL, szText);
			crToolColor = crColor;
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_SELECTEDTOOL), NULL, TRUE);
		} else if (iStatic == 1) {
			SetDlgItemText(hStatusDialog, IDC_STATIC_STATUSSTRING, szText);
			crStatusColor = crColor;
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_STATUSSTRING), NULL, TRUE);
		} else if (iStatic == 2) {
			HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_WEATHER0 + bWeatherTrend), IMAGE_BITMAP, 40, 40, LR_SHARED);
			if (GetLastError())
				ConsoleLog(LOG_ERROR, "LoadImage failed, %u\n", GetLastError());
			SendMessage(GetDlgItem(hStatusDialog, IDC_STATIC_WEATHERICON), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
			InvalidateRect(GetDlgItem(hStatusDialog, IDC_STATIC_WEATHERICON), NULL, TRUE);
		}
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

BOOL CALLBACK StatusDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT hit = 0;
	switch (message) {
	case WM_INITDIALOG:
		hStatusDialogBoldFont = CreateFont(-MulDiv(10, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
		SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL), WM_SETFONT, (WPARAM)hStatusDialogBoldFont, TRUE);
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SELECTEDTOOL))
			SetTextColor((HDC)wParam, crToolColor);
		else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_STATUSSTRING))
			SetTextColor((HDC)wParam, crStatusColor);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LONG)GetStockObject(NULL_BRUSH);

	case WM_NCHITTEST:
		return HTCAPTION;
	}
	return FALSE;
}

HWND ShowStatusDialog(void) {
	DWORD* CWndMainWindow = (DWORD*)*(DWORD*)0x4C702C;	// god this is awful
	hStatusDialog = CreateDialogParam(hSC2KFixModule, MAKEINTRESOURCE(IDD_SIMULATIONSTATUS), (HWND)(CWndMainWindow[7]), StatusDialogProc, NULL);
	if (!hStatusDialog) {
		ConsoleLog(LOG_ERROR, "Couldn't create statuss dialog: 0x%08X\n", GetLastError());
		return NULL;
	}
	hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
	hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &rcDesktop);
	GetWindowRect(hwndDesktop, &rcTemp);
	GetWindowRect(hStatusDialog, &rcDlg);
	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
	OffsetRect(&rcTemp, -rcDesktop.left, -rcDesktop.top);
	OffsetRect(&rcTemp, -rcDlg.right, -rcDlg.bottom);
	SetWindowPos(hStatusDialog, HWND_TOP, rcDesktop.left + (rcTemp.right / 2), rcDesktop.top + (rcTemp.bottom / 2), 0, 0, SWP_NOSIZE);
	return hStatusDialog;
}