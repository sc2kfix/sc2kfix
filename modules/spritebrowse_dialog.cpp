// sc2kfix modules/spritebrowse_dialog.cpp: sprite browsing dialog.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

extern BOOL PrepareDialogSpriteGraphic_SC2K1996(CGraphics *pGraphic, HWND hWnd, sprite_header_t *pSprHead, __int16 nSpriteID, CMFC3XRect *pDlgRect);

extern HWND hWndExt;

BOOL CALLBACK SpriteBrowserDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hWndCombo;
	CMFC3XRect cmdRect, dlgRect, paintRect;
	__int16 nSpriteID;
	char szSprIDEnt[80 + 1];
	int nSel;
	sprite_header_t *pSprHead;
	PAINTSTRUCT ps;
	std::string str;
	int x, y;
	static __int16 nBaseSpriteID = -1;
	static CGraphics *pQueriedTileImageSmall = NULL;
	static CGraphics *pQueriedTileImageMedium = NULL;
	static CGraphics *pQueriedTileImageLarge = NULL;
	static BOOL bSpriteFailSmall = FALSE;
	static BOOL bSpriteFailMedium = FALSE;
	static BOOL bSpriteFailLarge = FALSE;

	switch (message) {
		case WM_INITDIALOG:
			hWndCombo = GetDlgItem(hwndDlg, IDC_SPRITEBROWSER_COMBOCTRL);
			GetWindowRect(hWndCombo, &cmdRect);

			pQueriedTileImageSmall = new CGraphics();
			pQueriedTileImageMedium = new CGraphics();
			pQueriedTileImageLarge = new CGraphics();

			ComboBox_AddString(hWndCombo, "(Select Sprite ID)");
			for (unsigned i = 1; i < SPRITE_ENTRY_COUNT; i++) {
				memset(szSprIDEnt, 0, sizeof(szSprIDEnt));

				sprintf_s(szSprIDEnt, sizeof(szSprIDEnt) - 1, "%d - %s", i, szSpriteNames[i]);

				ComboBox_AddString(hWndCombo, szSprIDEnt);
			}

			ComboBox_SetCurSel(hWndCombo, 0);

			SetWindowPos(hWndCombo, HWND_TOP, 0, 0, cmdRect.right - cmdRect.left, 200, SWP_NOZORDER | SWP_NOMOVE);

			CenterDialogBox(hwndDlg);
			return TRUE;

		case WM_PAINT:
			BeginPaint(hwndDlg, &ps);
			GetClientRect(hwndDlg, &dlgRect);

			paintRect.left = 10;
			paintRect.top = 160;
			paintRect.bottom = 180;
			paintRect.right = 180;

			SelectFont(ps.hdc, hFontMSSansSerifRegular8);
			SetTextAlign(ps.hdc, 8);
			SetBkColor(ps.hdc, colBtnFace);
			SetTextColor(ps.hdc, RGB(0, 0, 0));

			if (nBaseSpriteID > 0) {
				nSpriteID = nBaseSpriteID + SPRITE_SMALL_START;

				str = "Small Sprite: ";
				str += szInternalSpriteName[nSpriteID];
				str += " / ";
				str += std::to_string(nSpriteID);
				str += " (";
				str += HexPls(nSpriteID, 4);
				str += ")";
				if (bSpriteFailSmall)
					str += " - Not found.";
				FillRect(ps.hdc, &paintRect, (HBRUSH)MainBrushFace->m_hObject);
				ExtTextOutA(ps.hdc, paintRect.left, paintRect.top, ETO_OPAQUE, &paintRect, str.c_str(), str.length(), 0);

				pSprHead = &pArrSpriteHeaders[nSpriteID];
				if (pSprHead) {
					x = dlgRect.right - 40;
					y = (dlgRect.bottom - pSprHead->wHeight) - 20;

					if (pQueriedTileImageSmall && !bSpriteFailSmall) {
						Game_Graphics_SetColorTableFromApplicationPalette(pQueriedTileImageSmall);
						Game_Graphics_Paint(pQueriedTileImageSmall, ps.hdc, x, y);
					}
				}

				nSpriteID = nBaseSpriteID + SPRITE_MEDIUM_START;

				str = "Medium Sprite: ";
				str += szInternalSpriteName[nSpriteID];
				str += " / ";
				str += std::to_string(nSpriteID);
				str += " (";
				str += HexPls(nSpriteID, 4);
				str += ")";
				if (bSpriteFailMedium)
					str += " - Not found.";
				SetRect(&paintRect, paintRect.left, paintRect.top + 40, paintRect.right, paintRect.bottom + 40);
				FillRect(ps.hdc, &paintRect, (HBRUSH)MainBrushFace->m_hObject);
				ExtTextOutA(ps.hdc, paintRect.left, paintRect.top, ETO_OPAQUE, &paintRect, str.c_str(), str.length(), 0);

				pSprHead = &pArrSpriteHeaders[nSpriteID];
				if (pSprHead) {
					x -= (40 * 2);
					y = (dlgRect.bottom - pSprHead->wHeight) - 20;

					if (pQueriedTileImageMedium && !bSpriteFailMedium) {
						Game_Graphics_SetColorTableFromApplicationPalette(pQueriedTileImageMedium);
						Game_Graphics_Paint(pQueriedTileImageMedium, ps.hdc, x, y);
					}
				}

				nSpriteID = nBaseSpriteID + SPRITE_LARGE_START;

				str = "Large Sprite: ";
				str += szInternalSpriteName[nSpriteID];
				str += " / ";
				str += std::to_string(nSpriteID);
				str += " (";
				str += HexPls(nSpriteID, 4);
				str += ")";
				if (bSpriteFailLarge)
					str += " - Not found.";
				SetRect(&paintRect, paintRect.left, paintRect.top + 40, paintRect.right, paintRect.bottom + 40);
				FillRect(ps.hdc, &paintRect, (HBRUSH)MainBrushFace->m_hObject);
				ExtTextOutA(ps.hdc, paintRect.left, paintRect.top, ETO_OPAQUE, &paintRect, str.c_str(), str.length(), 0);

				pSprHead = &pArrSpriteHeaders[nSpriteID];
				if (pSprHead) {
					x -= (40 * 4);
					y = (dlgRect.bottom - pSprHead->wHeight) - 20;

					if (pQueriedTileImageLarge && !bSpriteFailLarge) {
						Game_Graphics_SetColorTableFromApplicationPalette(pQueriedTileImageLarge);
						Game_Graphics_Paint(pQueriedTileImageLarge, ps.hdc, x, y);
					}
				}
			}

			EndPaint(hwndDlg, &ps);
			return FALSE;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam)) {
				case IDC_SPRITEBROWSER_COMBOCTRL:
					if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_KILLFOCUS ||
						GET_WM_COMMAND_CMD(wParam, lParam) == CBN_CLOSEUP ||
						GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDOK ||
						GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDCANCEL) {
						UpdateWindow(hwndDlg);
						// Set again in-order for palette animation/cycling redrawing
						// to resume.
						if (!hWndExt)
							hWndExt = hwndDlg;
						return TRUE;
					}
					else if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_DROPDOWN) {
						// Temporarily unset in-order to avoid the palette
						// animation/cycling redraw.
						hWndExt = 0;
					}
					return FALSE;
				case IDC_SPRITEBROWSER_SELBUT:
					if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED) {
						if (!hWndExt)
							hWndExt = hwndDlg;

						hWndCombo = GetDlgItem(hwndDlg, IDC_SPRITEBROWSER_COMBOCTRL);

						memset(szSprIDEnt, 0, sizeof(szSprIDEnt));

						if (pQueriedTileImageSmall)
							pQueriedTileImageSmall->DeleteStored_SC2K1996();
						if (pQueriedTileImageMedium)
							pQueriedTileImageMedium->DeleteStored_SC2K1996();
						if (pQueriedTileImageLarge)
							pQueriedTileImageLarge->DeleteStored_SC2K1996();
						nBaseSpriteID = -1;

						nSel = ComboBox_GetCurSel(hWndCombo);
						if (nSel > 0) {
							GetWindowRect(hwndDlg, &dlgRect);
							ScreenToClient(hwndDlg, (LPPOINT)&dlgRect);
							ScreenToClient(hwndDlg, (LPPOINT)&dlgRect.right);

							GetWindowText(hWndCombo, szSprIDEnt, 5);
							nBaseSpriteID = atoi(szSprIDEnt);

							if (nBaseSpriteID > 0) {
								nSpriteID = nBaseSpriteID + SPRITE_SMALL_START;
								bSpriteFailSmall = PrepareDialogSpriteGraphic_SC2K1996(pQueriedTileImageSmall, hwndDlg, &pArrSpriteHeaders[nSpriteID], nSpriteID, &dlgRect);

								nSpriteID = nBaseSpriteID + SPRITE_MEDIUM_START;
								bSpriteFailMedium = PrepareDialogSpriteGraphic_SC2K1996(pQueriedTileImageMedium, hwndDlg, &pArrSpriteHeaders[nSpriteID], nSpriteID, &dlgRect);

								nSpriteID = nBaseSpriteID + SPRITE_LARGE_START;
								bSpriteFailLarge = PrepareDialogSpriteGraphic_SC2K1996(pQueriedTileImageLarge, hwndDlg, &pArrSpriteHeaders[nSpriteID], nSpriteID, &dlgRect);
							}
						}
						InvalidateRect(hwndDlg, 0, TRUE);
						UpdateWindow(hwndDlg);
						return TRUE;
					}
					break;
				case IDOK:
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
			}

			case WM_DESTROY:
				hWndExt = 0;
				if (pQueriedTileImageSmall) {
					pQueriedTileImageSmall->DeleteStored_SC2K1996();
					delete pQueriedTileImageSmall;
					pQueriedTileImageSmall = NULL;
				}
				if (pQueriedTileImageMedium) {
					pQueriedTileImageMedium->DeleteStored_SC2K1996();
					delete pQueriedTileImageMedium;
					pQueriedTileImageMedium = NULL;
				}
				if (pQueriedTileImageLarge) {
					pQueriedTileImageLarge->DeleteStored_SC2K1996();
					delete pQueriedTileImageLarge;
					pQueriedTileImageLarge = NULL;
				}
				nBaseSpriteID = -1;
				break;
	}
	return FALSE;
}

void ShowSpriteBrowseDialog(void) {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CCityToolBar *pCityToolBar;

	pSCApp = &pCSimcityAppThis;
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	pCityToolBar = &pMainFrm->dwMFCityToolBar;

	pSCApp->dwSCABackgroundColourCyclingActive = TRUE;
	Game_CityToolBar_ToolMenuDisable(pCityToolBar);
	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SPRITEBROWSER), GameGetRootWindowHandle(), SpriteBrowserDialogProc);
	Game_CityToolBar_ToolMenuEnable(pCityToolBar);
	pSCApp->dwSCABackgroundColourCyclingActive = FALSE;
}
