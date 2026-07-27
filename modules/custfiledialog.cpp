// sc2kfix modules/custfiledialog.cpp: Custom calls for the file dialog
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// This is to mostly account for certain shortcomings with the default
// file dialog initialization and extended functionality in other contexts.

#undef UNICODE
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

// !!!! TODO: Perform a check during the "FileOK" situation against expected
//            results that would be encountered during file extension replacement
//            situations - check to see whether the file exists and prompt accordingly
//            if so, etc.

BOOL CALLBACK FileHookProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hWndParent;
	RECT mainRect, itemRect;
	//SIZE dlgSZ;
	int nPartHeight;
	DWORD nFlags;
	char szTempStr[MAX_PATH + 1];
	int nLen;
	OPENFILENAMEA *pOfn;
	extFileDlg_t *pExtDlg;
	OFNOTIFY *pOfNotify;

	switch (message) {
		case WM_INITDIALOG:
			SetWindowLong(hWnd, GWL_USERDATA, lParam);
			pOfn = (OPENFILENAMEA *)lParam;
			pExtDlg = (extFileDlg_t *)pOfn->lCustData;
			
			hWndParent = GetParent(hWnd);
			GetWindowRect(hWndParent, &mainRect);

			nPartHeight = 0;
			nFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW;
			if (pExtDlg) {
				if (pExtDlg->nExtType != FEXT_TYPE_NONE) {
					if (nFlags & SWP_HIDEWINDOW)
						nFlags &= ~SWP_HIDEWINDOW;
					if ((nFlags & SWP_SHOWWINDOW) == 0)
						nFlags |= SWP_SHOWWINDOW;
					if (pExtDlg->nExtType == FEXT_TYPE_SAVECITYNAME)
						nPartHeight = 30;
				}
			}

			// Alter the additional dialog portion.
			SetWindowPos(hWnd, HWND_TOP, 0, 0, mainRect.right - mainRect.left, nPartHeight, nFlags);

			// Handle the adjustment of custom items here.
			if (pExtDlg) {
				if (pExtDlg->nExtType == FEXT_TYPE_SAVECITYNAME) {
					nFlags &= ~SWP_NOMOVE;
					GetWindowRect(GetDlgItem(hWndParent, stc2), &itemRect); // The "File Types" static label
					SetDlgItemTextA(hWnd, IDC_CUST_STATIC1, "&City name:");
					SetWindowPos(GetDlgItem(hWnd, IDC_CUST_STATIC1), HWND_TOP, itemRect.left - 4, 2, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top, SWP_NOZORDER | SWP_SHOWWINDOW);
					GetWindowRect(GetDlgItem(hWndParent, cmb1), &itemRect); // The "File Types" ComboBox
					memset(szTempStr, 0, sizeof(szTempStr));
					memcpy(szTempStr, pExtDlg->szCityName, sizeof(pExtDlg->szCityName));
					SetDlgItemTextA(hWnd, IDC_CUST_EDIT1, szTempStr);
					SendMessage(GetDlgItem(hWnd, IDC_CUST_EDIT1), EM_SETLIMITTEXT, CITY_NAME_LEN, 0);
					SetWindowPos(GetDlgItem(hWnd, IDC_CUST_EDIT1), HWND_TOP, itemRect.left - 4, 0, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top, SWP_NOZORDER | SWP_SHOWWINDOW);
				}
			}

			CenterDialogBox(hWndParent);
			return TRUE;

		case WM_SIZE:
			hWndParent = GetParent(hWnd);
			//dlgSZ.cx = LOWORD(lParam);
			//dlgSZ.cy = HIWORD(lParam);
			//ConsoleLog(LOG_DEBUG, "dlgSZ(%d, %d)\n", dlgSZ.cx, dlgSZ.cy);

			GetWindowRect(GetDlgItem(hWndParent, cmb1), &itemRect); // The "File Types" ComboBox
			SetWindowPos(GetDlgItem(hWnd, IDC_CUST_EDIT1), HWND_TOP, 0, 0, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
			pOfNotify = (OFNOTIFY *)lParam;
			pExtDlg = (extFileDlg_t *)pOfNotify->lpOFN->lCustData;
			hWndParent = GetParent(hWnd);
			//ConsoleLog(LOG_DEBUG, "(%u) (%u - 0x%08X)\n", pOfNotify->hdr.code, pOfNotify->hdr.idFrom, pOfNotify->hdr.idFrom);
			switch (pOfNotify->hdr.code) {
				case CDN_FILEOK:
					memset(szTempStr, 0, sizeof(szTempStr));
					GetDlgItemTextA(hWndParent, cmb13, szTempStr, sizeof(szTempStr) - 1);
					PathRemoveExtensionA(szTempStr);
					if (strlen(szTempStr) == 0) {
						// This is to detect whether the entered
						// filename was just 'a' file extension,
						// in which case after it has been removed
						// if the length is 0.. don't close the dialog.
						SetWindowLongA(hWnd, DWL_MSGRESULT, 1);
						return TRUE;
					}
					//ConsoleLog(LOG_DEBUG, "CDN_FILEOK: [%s]\n", szTempStr);
					if (pExtDlg && pExtDlg->nExtType == FEXT_TYPE_SAVECITYNAME) {
						memset(szTempStr, 0, sizeof(szTempStr));
						GetDlgItemTextA(hWnd, IDC_CUST_EDIT1, szTempStr, sizeof(szTempStr) - 1);
						nLen = strlen(szTempStr);
						if (nLen < 1 || nLen > CITY_NAME_LEN) {
							char szErrStr[256 + 1];
							if (nLen < 1)
								strcpy_s(szErrStr, "You must enter a city name.");
							else
								sprintf_s(szErrStr, "Your city name cannot exceed %d characters.", CITY_NAME_LEN);
							MessageBoxA(hWndParent, szErrStr, gamePrimaryKey, MB_ICONERROR);
							SetWindowLongA(hWnd, DWL_MSGRESULT, 1);
							return TRUE;
						}
						if (memcmp(pExtDlg->szCityName, szTempStr, CITY_NAME_LEN) != 0) {
							memset(pExtDlg->szCityName, 0, sizeof(pExtDlg->szCityName));
							memcpy(pExtDlg->szCityName, szTempStr, CITY_NAME_LEN);
							nLen = strlen(pExtDlg->szCityName);
							pExtDlg->szCityName[nLen] = 0;
							pExtDlg->bCityNameChanged = true;
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}
