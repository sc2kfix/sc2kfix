// sc2kfix settings.cpp: settings dialog code and configurator
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>
#include "resource.h"

static DWORD dwDummy;

char szSettingsMayorName[64];
char szSettingsCompanyName[64];
BOOL bSettingsMusicInBackground = FALSE;
BOOL bSettingsUseNewStrings = TRUE;
BOOL bSettingsUseSoundReplacements = TRUE;
BOOL bSettingsMilitaryBaseRevenue = FALSE;	// NYI
BOOL bSettingsShuffleMusic = FALSE;
BOOL bSettingsAlwaysConsole = FALSE;

static HWND hwndDesktop;
static RECT rcTemp, rcDlg, rcDesktop;

HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText) {
	if (!hDlg || !hControl || !szText)
		return NULL;

	HWND hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, hDlg, NULL, hSC2KFixModule, NULL);
	if (!hTooltip)
		return NULL;

	char* lpszText = _strdup(szText);
	if (!lpszText)
		return NULL;

	SendMessage(hTooltip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);

	TOOLINFO tooltipInfo = { 0 };
	tooltipInfo.cbSize = sizeof(TOOLINFO);
	tooltipInfo.hwnd = hDlg;
	tooltipInfo.uId = (UINT_PTR)hControl;
	tooltipInfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	tooltipInfo.lpszText = lpszText;
	SendMessage(hTooltip, TTM_ADDTOOL, NULL, (LPARAM)&tooltipInfo);

	return hTooltip;
}

void LoadSettings(void) {
	HKEY hkeySC2KRegistration;
	LSTATUS lResultRegistration = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Registration", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KRegistration, NULL);
	if (lResultRegistration != ERROR_SUCCESS) {
		MessageBox(NULL, "Couldn't open registry keys for editing", "sc2kfix error", MB_OK | MB_ICONEXCLAMATION);
		ConsoleLog(LOG_ERROR, "Couldn't open registry keys for settings load, error = 0x%08X\n", lResultRegistration);
		return;
	}

	DWORD dwMayorNameSize = 64;
	DWORD dwCompanyNameSize = 64;
	LSTATUS retval = ERROR_SUCCESS;

	retval = RegGetValue(hkeySC2KRegistration, NULL, "Mayor Name", RRF_RT_REG_SZ, NULL, szSettingsMayorName, &dwMayorNameSize);
	switch (retval) {
	case ERROR_SUCCESS:
		break;
	default:
		strcpy_s(szSettingsMayorName, "Marvin Maxis");
		char* buf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, retval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
		ConsoleLog(LOG_WARNING, "Error %s loading mayor name; resetting to default.\n", buf);
		break;
	}

	retval = RegGetValue(hkeySC2KRegistration, NULL, "Company Name", RRF_RT_REG_SZ, NULL, szSettingsCompanyName, &dwCompanyNameSize);
	switch (retval) {
	case ERROR_SUCCESS:
		break;
	default:
		strcpy_s(szSettingsCompanyName, "Q37 Space Modulator Mfg.");
		char* buf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, retval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
		ConsoleLog(LOG_WARNING, "Error %s loading company name; resetting to default.\n", buf);
		break;
	}

	HKEY hkeySC2KFix;
	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KFix, NULL);

	DWORD dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsMusicInBackground", RRF_RT_REG_DWORD, NULL, &bSettingsMusicInBackground, &dwSizeofBool) != ERROR_SUCCESS) {
		bSettingsMusicInBackground = FALSE;
		RegSetValueEx(hkeySC2KFix, "bSettingsMusicInBackground", NULL, REG_DWORD, (BYTE*)&bSettingsMusicInBackground, sizeof(BOOL));
	}

	dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsUseNewStrings", RRF_RT_REG_DWORD, NULL, &bSettingsUseNewStrings, &dwSizeofBool)) {
		bSettingsUseNewStrings = TRUE;
		RegSetValueEx(hkeySC2KFix, "bSettingsUseNewStrings", NULL, REG_DWORD, (BYTE*)&bSettingsUseNewStrings, sizeof(BOOL));
	}

	dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsUseSoundReplacements", RRF_RT_REG_DWORD, NULL, &bSettingsUseSoundReplacements, &dwSizeofBool)) {
		bSettingsUseSoundReplacements = TRUE;
		RegSetValueEx(hkeySC2KFix, "bSettingsUseSoundReplacements", NULL, REG_DWORD, (BYTE*)&bSettingsUseSoundReplacements, sizeof(BOOL));
	}

	dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsMilitaryBaseRevenue", RRF_RT_REG_DWORD, NULL, &bSettingsMilitaryBaseRevenue, &dwSizeofBool)) {
		bSettingsMilitaryBaseRevenue = FALSE;
		RegSetValueEx(hkeySC2KFix, "bSettingsMilitaryBaseRevenue", NULL, REG_DWORD, (BYTE*)&bSettingsMilitaryBaseRevenue, sizeof(BOOL));
	}

	dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsShuffleMusic", RRF_RT_REG_DWORD, NULL, &bSettingsShuffleMusic, &dwSizeofBool)) {
		bSettingsShuffleMusic = FALSE;
		RegSetValueEx(hkeySC2KFix, "bSettingsShuffleMusic", NULL, REG_DWORD, (BYTE*)&bSettingsShuffleMusic, sizeof(BOOL));
	}

	dwSizeofBool = sizeof(BOOL);
	if (RegGetValue(hkeySC2KFix, NULL, "bSettingsAlwaysConsole", RRF_RT_REG_DWORD, NULL, &bSettingsAlwaysConsole, &dwSizeofBool)) {
		bSettingsAlwaysConsole = FALSE;
		RegSetValueEx(hkeySC2KFix, "bSettingsAlwaysConsole", NULL, REG_DWORD, (BYTE*)&bSettingsAlwaysConsole, sizeof(BOOL));
	}
}

void SaveSettings(void) {
	HKEY hkeySC2KRegistration;
	LSTATUS lResultRegistration = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Registration", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KRegistration, NULL);
	if (lResultRegistration != ERROR_SUCCESS) {
		MessageBox(NULL, "Couldn't open registry keys for editing", "sc2kfix error", MB_OK | MB_ICONEXCLAMATION);
		ConsoleLog(LOG_ERROR, "Couldn't open registry keys for registry check, error = 0x%08X\n", lResultRegistration);
		return;
	}

	// Write registration strings
	RegSetValueEx(hkeySC2KRegistration, "Mayor Name", NULL, REG_SZ, (BYTE*)szSettingsMayorName, strlen(szSettingsMayorName) + 1);
	RegSetValueEx(hkeySC2KRegistration, "Company Name", NULL, REG_SZ, (BYTE*)szSettingsCompanyName, strlen(szSettingsCompanyName) + 1);

	HKEY hkeySC2KFix;
	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KFix, NULL);

	// Write sc2kfix settings
	RegSetValueEx(hkeySC2KFix, "bSettingsMusicInBackground", NULL, REG_DWORD, (BYTE*)&bSettingsMusicInBackground, sizeof(BOOL));
	RegSetValueEx(hkeySC2KFix, "bSettingsUseNewStrings", NULL, REG_DWORD, (BYTE*)&bSettingsUseNewStrings, sizeof(BOOL));
	RegSetValueEx(hkeySC2KFix, "bSettingsUseSoundReplacements", NULL, REG_DWORD, (BYTE*)&bSettingsUseSoundReplacements, sizeof(BOOL));
	RegSetValueEx(hkeySC2KFix, "bSettingsMilitaryBaseRevenue", NULL, REG_DWORD, (BYTE*)&bSettingsMilitaryBaseRevenue, sizeof(BOOL));
	RegSetValueEx(hkeySC2KFix, "bSettingsShuffleMusic", NULL, REG_DWORD, (BYTE*)&bSettingsShuffleMusic, sizeof(BOOL));
	RegSetValueEx(hkeySC2KFix, "bSettingsAlwaysConsole", NULL, REG_DWORD, (BYTE*)&bSettingsAlwaysConsole, sizeof(BOOL));
	ConsoleLog(LOG_INFO, "Saved sc2kfix settings.\n");

	// Update any hooks we need to.
	UpdateMiscHooks();
}

BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KAppModule, MAKEINTRESOURCE(1)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KAppModule, MAKEINTRESOURCE(2)));

		// Create tooltips.
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS),
			"Certain versions of SimCity 2000 had higher quality sounds than the Windows 95 versions. "
			"This option controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MILITARY_REVENUE),
			"Military bases were originally intended to increase commercial demand and generate city revenue. "
			"This was never implemented, though the negative aspects of military base ownership were. "
			"This makes military bases in vanilla SimCity 2000 a rather poor mayoral decision.\n\n"

			"Enabling this option will allow military bases to generate a population based annual income in the form of a stipend from the federal government, "
			"as well as increased commercial demand, both of which scale with the size and type of the base, and may fluctuate depending on military needs and "
			"base redevelopment efforts.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC),
			"By default, SimCity 2000 selects \"random\" music by playing the next track in a looping playlist of songs. "
			"This option controls whether or not to shuffle the playlist when the game starts and when the end of the playlist is reached.");

		// Load the existing settings into the dialog
		SetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szSettingsMayorName);
		SetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szSettingsCompanyName);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), bSettingsMusicInBackground ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), bSettingsUseNewStrings ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), bSettingsUseNewStrings ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MILITARY_REVENUE), bSettingsMilitaryBaseRevenue ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), bSettingsShuffleMusic ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), bSettingsAlwaysConsole ? BST_CHECKED : BST_UNCHECKED);

		// Center the dialog box
		hwndDesktop = GetDesktopWindow();
		GetWindowRect(hwndDesktop, &rcDesktop);
		GetWindowRect(hwndDesktop, &rcTemp);
		GetWindowRect(hwndDlg, &rcDlg);
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rcTemp, -rcDesktop.left, -rcDesktop.top);
		OffsetRect(&rcTemp, -rcDlg.right, -rcDlg.bottom);
		SetWindowPos(hwndDlg, HWND_TOP, rcDesktop.left + (rcTemp.right / 2), rcDesktop.top + (rcTemp.bottom / 2), 0, 0, SWP_NOSIZE);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_SETTINGS_OK:
			// Grab settings from the dialog controls
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szSettingsMayorName, 63))
				strcpy_s(szSettingsMayorName, 64, "Marvin Maxis");
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szSettingsCompanyName, 63))
				strcpy_s(szSettingsCompanyName, 64, "Q37 Space Modulator Mfg.");
			bSettingsMusicInBackground = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC));
			bSettingsMilitaryBaseRevenue = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MILITARY_REVENUE));
			bSettingsAlwaysConsole = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE));

			// Save the settings
			SaveSettings();

			// Fall through
		case ID_SETTINGS_CANCEL:
			EndDialog(hwndDlg, wParam);
		}
		return TRUE;
	}
	return FALSE;
}

void ShowSettingsDialog(void) {
	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS), NULL, SettingsDialogProc);
}