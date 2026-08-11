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

static bool L_PreCheckForExistingFile(char *lpFileName, const char *pExt) {
	char szTempFile[MAX_PATH + 1], szTempExt[16 + 1], szTempOnlyFile[MAX_PATH + 1];
	int nLen;

	strcpy_s(szTempFile, lpFileName);
	strcpy_s(szTempOnlyFile, szTempFile);
	PathStripPathA(szTempOnlyFile);
	PathRemoveExtensionA(szTempOnlyFile);
	PathRemoveExtensionA(szTempFile);
	nLen = strlen(szTempOnlyFile);
	// nLen above 0.
	if (nLen > 0) {
		// Under this circumstance only do the file extension
		// validation and replacement if an extension has been
		// set, otherwise only do the length check and return
		// false if the stripped filename didn't contain anything
		// but the extension.
		if (pExt && strlen(pExt) > 0) {
			strcpy_s(szTempExt, pExt);
			_strlwr_s(szTempExt);
			// Empty the filename string and rebuild it.
			memset(lpFileName, 0, MAX_PATH + 1);
			sprintf_s(lpFileName, MAX_PATH, "%s.%s", szTempFile, szTempExt);
		}
		return true;
	}
	return false;
}

BOOL CALLBACK FileHookProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hWndParent;
	RECT mainRect, itemRect;
	//SIZE dlgSZ;
	int nItemHorzOffset;
	int nPartHeight;
	DWORD nFlags;
	bool bHasSaveExt;
	char szTempStr[MAX_PATH + 1], szTempPath[MAX_PATH + 1], szTempFile[MAX_PATH + 1];
	char szErrStr[1024 + 1];
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
					// Horizontal positional offset observed beyond Windows NT 6.1
					// (Observed on Windows 10 and 11; 8 or 8.1 is not known but accounted for).
					nItemHorzOffset = (dwOSVersion > 0x00060001) ? 8 : 4;

					nFlags &= ~SWP_NOMOVE;
					GetWindowRect(GetDlgItem(hWndParent, stc2), &itemRect); // The "File Types" static label
					SetDlgItemTextA(hWnd, IDC_CUST_STATIC1, "&City name:");
					SetWindowPos(GetDlgItem(hWnd, IDC_CUST_STATIC1), HWND_TOP, itemRect.left - nItemHorzOffset, 2, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top, SWP_NOZORDER | SWP_SHOWWINDOW);
					GetWindowRect(GetDlgItem(hWndParent, cmb1), &itemRect); // The "File Types" ComboBox
					memset(szTempStr, 0, sizeof(szTempStr));
					memcpy(szTempStr, pExtDlg->szCityName, sizeof(pExtDlg->szCityName));
					SetDlgItemTextA(hWnd, IDC_CUST_EDIT1, szTempStr);
					SendMessage(GetDlgItem(hWnd, IDC_CUST_EDIT1), EM_SETLIMITTEXT, CITY_NAME_LEN, 0);
					SetWindowPos(GetDlgItem(hWnd, IDC_CUST_EDIT1), HWND_TOP, itemRect.left - nItemHorzOffset, 0, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top, SWP_NOZORDER | SWP_SHOWWINDOW);
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
			switch (pOfNotify->hdr.code) {
				case CDN_FILEOK:
					if (pExtDlg && pExtDlg->nExtType == FEXT_TYPE_SAVECITYNAME) {
						bHasSaveExt = (pExtDlg->pSaveExt && strlen(pExtDlg->pSaveExt) > 0) ? true : false;
						memset(szTempStr, 0, sizeof(szTempStr));
						memset(szTempPath, 0, sizeof(szTempPath));
						memset(szTempFile, 0, sizeof(szTempFile));
						GetDlgItemTextA(hWndParent, cmb13, szTempStr, sizeof(szTempStr) - 1);
						SendMessageA(hWndParent, CDM_GETFILEPATH, MAX_PATH, (LPARAM)szTempPath);
						if (L_PreCheckForExistingFile(szTempPath, pExtDlg->pSaveExt)) {
							strcpy_s(szTempFile, szTempPath);
							PathStripPathA(szTempFile);
							// Just in case the extension wasn't defined, make use of
							// the default city save match case, otherwise warn and abort.
							if (!bHasSaveExt) {
								if (!PathMatchSpecA(szTempPath, CITY_DEFAULT_SAVE_MATCH)) {
									sprintf_s(szErrStr, "'%s' does not have a valid extension set. You will need to set it to: '%s'", szTempFile, CITY_DEFAULT_APPEND_EXTENSION);
									MessageBoxA(hWndParent, szErrStr, "Error", MB_ICONERROR);
									SetWindowLongA(hWnd, DWL_MSGRESULT, 1);
									return TRUE;
								}
							}
							if (PathFileExistsA(szTempPath)) {
								if (_stricmp(szTempStr, szTempFile) != 0) {
									if (!bHasSaveExt)
										sprintf_s(szErrStr, "WARNING: %s already exists.\nDo you want to replace it?", szTempFile);
									else
										sprintf_s(szErrStr, "WARNING: The file extension for '%s' has been set to: '.%s'\n\n%s already exists.\nDo you want to replace it?",
											szTempStr, pExtDlg->pSaveExt, szTempFile);
									if (MessageBoxA(hWndParent, szErrStr, "Confirm Save As", MB_ICONWARNING | MB_YESNO) != IDYES) {
										SetWindowLongA(hWnd, DWL_MSGRESULT, 1);
										return TRUE;
									}
								}
							}
						}
						else {
							// This is to detect whether the entered
							// filename was just 'a' file extension,
							// in which case after it has been removed
							// if the length is 0.. don't close the dialog.
							SetWindowLongA(hWnd, DWL_MSGRESULT, 1);
							return TRUE;
						}

						strcpy_s(pExtDlg->szAdjustedFile, szTempPath);

						memset(szTempStr, 0, sizeof(szTempStr));
						GetDlgItemTextA(hWnd, IDC_CUST_EDIT1, szTempStr, sizeof(szTempStr) - 1);
						nLen = strlen(szTempStr);
						if (nLen < 1 || nLen > CITY_NAME_LEN) {
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
