// sc2kfix registry_install.cpp: faux installer to create missing registry entries
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include "../resource.h"

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		// These both come from the game themselves.
		// I don't know if they're used anywhere, but they're there.
		SetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, "Marvin Maxis");
		SetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, "Q37 Space Modulator Mfg.");

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_INSTALL_OK:
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, szSettingsMayorName, 63))
				strcpy_s(szSettingsMayorName, 64, "Marvin Maxis");
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, szSettingsCompanyName, 63))
				strcpy_s(szSettingsCompanyName, 64, "Q37 Space Modulator Mfg.");
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

static void InstallSC2KDefaults(void) {
	const char *ini_file = GetIniPath();
	const char *section;

	section = "Portable";
	if (GetPrivateProfileIntA(section, "Installed", 0, ini_file) == 1) {
		return;
	}

	WritePrivateProfileIntA(section, "Installed", 1, ini_file);

	// Prompt the user for the mayor and company names
	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_INSTALL), NULL, InstallDialogProc);

	// Write version info
	DWORD dwSC2KVersion = 0x00000100;

	section = "Version";
	WritePrivateProfileIntA(section, "SCURK", dwSC2KVersion, ini_file);
	WritePrivateProfileIntA(section, "SimCity 2000", dwSC2KVersion, ini_file);

	// Write language info
	section = "Localize";
	WritePrivateProfileStringA(section, "Language", "USA", ini_file);

	// Write default options
	section = "Options";
	WritePrivateProfileIntA(section, "Disasters", TRUE, ini_file);
	WritePrivateProfileIntA(section, "Music", TRUE, ini_file);
	WritePrivateProfileIntA(section, "Sound", TRUE, ini_file);
	WritePrivateProfileIntA(section, "AutoGoto", TRUE, ini_file);
	WritePrivateProfileIntA(section, "AutoBudget", FALSE, ini_file);
	WritePrivateProfileIntA(section, "AutoSave", FALSE, ini_file);
	WritePrivateProfileIntA(section, "Speed", FALSE, ini_file);

	// Write default SCURK options
	section = "SCURK";
	WritePrivateProfileIntA(section, "CycleColors", 1, ini_file);
	WritePrivateProfileIntA(section, "GridHeight", 2, ini_file);
	WritePrivateProfileIntA(section, "GridWidth", 2, ini_file);
	WritePrivateProfileIntA(section, "ShowClipRegion", 0, ini_file);
	WritePrivateProfileIntA(section, "ShowDrawGrid", 0, ini_file);
	WritePrivateProfileIntA(section, "SnapToGrid", 0, ini_file);
	WritePrivateProfileIntA(section, "Sound", 1, ini_file);

	SaveSettings(TRUE);
}

static void MigrateSC2KRegistration(HKEY hKeySC2KReg) {
	const char *ini_file = GetIniPath();
	const char *section = "Registration";

	MigrateRegStringValue(hKeySC2KReg, NULL, "Mayor Name", szSettingsMayorName, sizeof(szSettingsMayorName));
	WritePrivateProfileStringA(section, "Mayor Name", szSettingsMayorName, ini_file);

	MigrateRegStringValue(hKeySC2KReg, NULL, "Company Name", szSettingsCompanyName, sizeof(szSettingsCompanyName));
	WritePrivateProfileStringA(section, "Company Name", szSettingsCompanyName, ini_file);
}

static void MigrateSC2KVersion(void) {
	DWORD dwOut;
	const char *ini_file = GetIniPath();
	const char *section = "Version";

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Version", "SCURK", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SCURK", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Version", "SimCity 2000", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SimCity 2000", dwOut, ini_file);
}

static void MigrateSC2KLocalize(void) {
	const char *ini_file = GetIniPath();
	const char *section = "Localize";

	char szOutBuf[16];
	MigrateRegStringValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Localize", "Language", szOutBuf, sizeof(szOutBuf));
	WritePrivateProfileStringA(section, "Language", szOutBuf, ini_file);
}

static void MigrateSC2KOptions(void) {
	DWORD dwOut;
	const char *ini_file = GetIniPath();
	const char *section = "Options";

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "Disasters", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Disasters", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "Music", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Music", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "Sound", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Sound", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "AutoGoto", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoGoto", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "AutoBudget", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoBudget", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "AutoSave", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoSave", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", "Speed", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Speed", dwOut, ini_file);
}

static void MigrateSC2KSCURK(void) {
	DWORD dwOut;
	const char *ini_file = GetIniPath();
	const char *section = "SCURK";

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "CycleColors", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "CycleColors", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "GridHeight", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "GridHeight", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "GridWidth", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "GridWidth", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "ShowClipRegion", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "ShowClipRegion", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "ShowDrawGrid", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "ShowDrawGrid", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "SnapToGrid", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SnapToGrid", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\SCURK", "Sound", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "Sound", dwOut, ini_file);
}

static void MigrateFinalize(void) {
	const char *ini_file = GetIniPath();
	const char *section = "Portable";

	WritePrivateProfileIntA(section, "Installed", 1, ini_file);
}

int DoRegistryCheckAndInstall(void) {
	HKEY hKeySC2KRegistration;
	LSTATUS lResultRegistration = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Registration", NULL, KEY_ALL_ACCESS, &hKeySC2KRegistration);
	if (lResultRegistration != ERROR_SUCCESS) {
		// Let's install.
		InstallSC2KDefaults();
		return 0;
	}

	if (szSettingsMayorName[0] == 0 ||
		szSettingsCompanyName[0] == 0) {
		if (RegQueryValueEx(hKeySC2KRegistration, "Mayor Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND ||
			RegQueryValueEx(hKeySC2KRegistration, "Company Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND) {

			// Fake an install.

			InstallSC2KDefaults();

			RegCloseKey(hKeySC2KRegistration);

			// Signal that we had to fake an install.
			return 2;
		}
		else {

			// Migrate from registry to ini.
			MigrateSC2KRegistration(hKeySC2KRegistration);
			RegCloseKey(hKeySC2KRegistration);

			MigrateSC2KVersion();
			MigrateSC2KLocalize();
			MigrateSC2KOptions();
			MigrateSC2KSCURK();
			MigrateFinalize();

			SaveSettings(TRUE);
			return 1;
		}
	}
	return 0;
}
