// sc2kfix modules/tileconv_config.cpp: Tile Conversion/Default configuration
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// The purpose of this dialogue is as follows:
// - To select which 'fixed' tile types are loaded when the default sprite set is (re)loaded
// - To select which 'Hangar1' type is used when:
//   - The default sprite set is reloaded
//   - When the 'ORIGINAL'/built-in tileset from the DOS/Macintosh version is loaded

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define GET_CHECKBOX(h, src) (Button_GetCheck(GetDlgItem(h, src)) == BST_CHECKED) ? true : false
#define SET_CHECKBOX(h, src, dest) Button_SetCheck(GetDlgItem(h, dest), src ? BST_CHECKED : BST_UNCHECKED)

typedef struct {
	DWORD dwTileMask;
	int nHangarMode;
} tileconvconf_t;

static void SetTileTypeSelection(HWND hWnd, DWORD dwTileMask) {
	SET_CHECKBOX(hWnd, (dwTileMask & FIXTIL_MASK_HORZOFF), IDC_TILECONV_CHECK_HORZOFF);
	SET_CHECKBOX(hWnd, (dwTileMask & FIXTIL_MASK_VERTOFF), IDC_TILECONV_CHECK_VERTOFF);
	SET_CHECKBOX(hWnd, (dwTileMask & FIXTIL_MASK_BADPALIDX), IDC_TILECONV_CHECK_BADPALIDX);
	SET_CHECKBOX(hWnd, (dwTileMask & FIXTIL_MASK_MISSPIXELS), IDC_TILECONV_CHECK_MISSPIXELS);
	SET_CHECKBOX(hWnd, (dwTileMask & FIXTIL_MASK_OOBPALIDX), IDC_TILECONV_CHECK_OOBPALIDX);
}

static void GetUpdatedTileMaskAndMode(HWND hWnd, tileconvconf_t *cvt) {
	cvt->dwTileMask = 0;
	if (GET_CHECKBOX(hWnd, IDC_TILECONV_CHECK_HORZOFF))
		cvt->dwTileMask |= FIXTIL_MASK_HORZOFF;
	if (GET_CHECKBOX(hWnd, IDC_TILECONV_CHECK_VERTOFF))
		cvt->dwTileMask |= FIXTIL_MASK_VERTOFF;
	if (GET_CHECKBOX(hWnd, IDC_TILECONV_CHECK_BADPALIDX))
		cvt->dwTileMask |= FIXTIL_MASK_BADPALIDX;
	if (GET_CHECKBOX(hWnd, IDC_TILECONV_CHECK_MISSPIXELS))
		cvt->dwTileMask |= FIXTIL_MASK_MISSPIXELS;
	if (GET_CHECKBOX(hWnd, IDC_TILECONV_CHECK_OOBPALIDX))
		cvt->dwTileMask |= FIXTIL_MASK_OOBPALIDX;
	cvt->nHangarMode = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_TILECONV_COMBO));
}

BOOL CALLBACK ConfTileConvDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hComboBox;
	RECT cmdRect;
	tileconvconf_t *cvt;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		cvt = (tileconvconf_t *)lParam;

		DestroyStoredTooltips(storedToolTips, hwndDlg);

		hComboBox = GetDlgItem(hwndDlg, IDC_TILECONV_COMBO);
		GetWindowRect(hComboBox, &cmdRect);
		SetWindowRedraw(hComboBox, FALSE);
		ComboBox_AddString(hComboBox, "(Ignore)");
		ComboBox_AddString(hComboBox, "Shut");
		ComboBox_AddString(hComboBox, "Anim (Grey)");
		ComboBox_AddString(hComboBox, "Open");
		SetWindowRedraw(hComboBox, TRUE);

		ComboBox_SetCurSel(hComboBox, cvt->nHangarMode);

		SetWindowPos(hComboBox, HWND_TOP, 0, 0, cmdRect.right - cmdRect.left, 100, SWP_NOZORDER | SWP_NOMOVE);

		SetTileTypeSelection(hwndDlg, cvt->dwTileMask);

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_COMBO),
			"Select the 'Hangar1' default type you want to have applied during:\n"
			" - Default object set (re)loading.\n"
			" - The on-the-fly conversion and loading of the 'ORIGINAL'/default DOS/Macintosh object set.\n\n"
			" Types:\n"
			" - Shut (Yellow door - extrapolated from the tiny/small view)\n"
			" - Anim (Grey door - DOS/Macintosh pre-release to version 1.1 cycling effect)\n"
			" - Open (Black internal area - default from DOS/Macintosh 1.2 and all Windows editions)\n\n"
			"** If set to '(Ignore)' it won't attempt to use the associated fixed tile or adjust the palette index.");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_CHECK_HORZOFF),
			"Objects with horizontal offset fixes:\n"
			" - Drive-In Theater\n"
			" - Gas Power Plant\n"
			" - Oil Power Plant\n"
			" - Nuclear Power Plant");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_CHECK_VERTOFF),
			"Objects with vertical offset fixes:\n"
			" - Large Apartments 1");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_CHECK_BADPALIDX),
			"Objects with bad palette index fixes:\n"
			" - Theater Square\n"
			" - Chemical Processing 1 (3x3)\n"
			" - Hospital\n"
			" - Marina\n"
			" - Seaport Warehouse\n"
			" - Control Tower (Military)\n"
			" - Dustcloud/explosion (1-4)");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_CHECK_MISSPIXELS),
			"Objects with missing pixel fixes:\n"
			" - Plymouth Arcology");

		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_TILECONV_CHECK_OOBPALIDX),
			"Objects with out-of-bounds palette index fixes:\n"
			" - Crane\n"
			" - Loading Bay");

		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_DESTROY:
		DestroyStoredTooltips(storedToolTips, hwndDlg);
		return TRUE;

	case WM_COMMAND:
		cvt = (tileconvconf_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			GetUpdatedTileMaskAndMode(hwndDlg, cvt);
			EndDialog(hwndDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL DoConfigureTileConv(HWND hWnd) {
	BOOL bRet;
	tileconvconf_t cvt;

	memset(&cvt, 0, sizeof(cvt));
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		cvt.dwTileMask = (DWORD)jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SCURK_FIXTILMSK].ToInt();
		cvt.nHangarMode = jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SCURK_HANGARCNV].ToInt();
	}
	else {
		cvt.dwTileMask = (DWORD)jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SC2K_FIXTILMSK].ToInt();
		cvt.nHangarMode = jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SC2K_HANGARCNV].ToInt();
	}

	bRet = DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_TILECONV), hWnd, ConfTileConvDialogProc, (LPARAM)&cvt);
	if (bRet == TRUE) {
		if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
			jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SCURK_FIXTILMSK] = cvt.dwTileMask;
			jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SCURK_HANGARCNV] = cvt.nHangarMode;

			// Save the settings JSON file and update hooks
			SaveJSONSettings();
		}
		else {
			jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SC2K_FIXTILMSK] = cvt.dwTileMask;
			jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SC2K_HANGARCNV] = cvt.nHangarMode;
		}
	}
	return bRet;
}
