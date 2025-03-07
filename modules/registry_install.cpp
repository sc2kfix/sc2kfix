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

char szSC2KPath[MAX_PATH];
char szSC2KGoodiesPath[MAX_PATH];
char szSC2KMoviesPath[MAX_PATH];

const char *GetSetMoviesPath() {
	return szSC2KMoviesPath;
}

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

BOOL DoRegistryCheckAndInstall(void) {
	HKEY hkeySC2KRegistration;
	LSTATUS lResultRegistration = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Registration", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KRegistration, NULL);
	if (lResultRegistration != ERROR_SUCCESS) {
		MessageBox(NULL, "Couldn't open registry keys for editing", "sc2kfix error", MB_OK | MB_ICONEXCLAMATION);
		ConsoleLog(LOG_ERROR, "CORE: Couldn't open registry keys for registry check, error = 0x%08X\n", lResultRegistration);
		return FALSE;
	}

	if (RegQueryValueEx(hkeySC2KRegistration, "Mayor Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND ||
		RegQueryValueEx(hkeySC2KRegistration, "Company Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND) {

		// Fake an install.
		
		// Prompt the user for the mayor and company names
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_INSTALL), NULL, InstallDialogProc);

		// Write registration strings
		RegSetValueEx(hkeySC2KRegistration, "Mayor Name", NULL, REG_SZ, (BYTE*)szSettingsMayorName, strlen(szSettingsMayorName) + 1);
		RegSetValueEx(hkeySC2KRegistration, "Company Name", NULL, REG_SZ, (BYTE*)szSettingsCompanyName, strlen(szSettingsCompanyName) + 1);

		// Generate paths
		char szSC2KExePath[MAX_PATH] = { 0 };
		char szSC2KPaths[10][MAX_PATH];
		GetModuleFileNameEx(GetCurrentProcess(), NULL, szSC2KExePath, MAX_PATH);
		PathRemoveFileSpecA(szSC2KExePath);
		
		for (int i = 0; i < 10; i++)
			strcpy_s(szSC2KPaths[i], MAX_PATH, szSC2KExePath);

		strcat_s(szSC2KPaths[0], MAX_PATH, "\\CITIES");
		strcat_s(szSC2KPaths[1], MAX_PATH, "\\DATA");
		strcat_s(szSC2KPaths[2], MAX_PATH, "\\GOODIES");
		strcat_s(szSC2KPaths[3], MAX_PATH, "\\BITMAPS");
		strcat_s(szSC2KPaths[4], MAX_PATH, "");
		strcat_s(szSC2KPaths[5], MAX_PATH, "\\SOUNDS");
		strcat_s(szSC2KPaths[6], MAX_PATH, "\\CITIES");
		strcat_s(szSC2KPaths[7], MAX_PATH, "\\SCENARIO");
		strcat_s(szSC2KPaths[8], MAX_PATH, "\\SCURKART");
		strcat_s(szSC2KPaths[9], MAX_PATH, "\\Movies");

		strcpy_s(szSC2KMoviesPath, MAX_PATH, szSC2KPaths[9]);

		// Write paths
		HKEY hkeySC2KPaths;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Paths", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KPaths, NULL);
		RegSetValueEx(hkeySC2KPaths, "Cities", NULL, REG_SZ, (BYTE*)szSC2KPaths[0], strlen(szSC2KPaths[0]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Data", NULL, REG_SZ, (BYTE*)szSC2KPaths[1], strlen(szSC2KPaths[1]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Goodies", NULL, REG_SZ, (BYTE*)szSC2KPaths[2], strlen(szSC2KPaths[2]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Graphics", NULL, REG_SZ, (BYTE*)szSC2KPaths[3], strlen(szSC2KPaths[3]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Home", NULL, REG_SZ, (BYTE*)szSC2KPaths[4], strlen(szSC2KPaths[4]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Music", NULL, REG_SZ, (BYTE*)szSC2KPaths[5], strlen(szSC2KPaths[5]) + 1);
		RegSetValueEx(hkeySC2KPaths, "SaveGame", NULL, REG_SZ, (BYTE*)szSC2KPaths[6], strlen(szSC2KPaths[6]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Scenarios", NULL, REG_SZ, (BYTE*)szSC2KPaths[7], strlen(szSC2KPaths[7]) + 1);
		RegSetValueEx(hkeySC2KPaths, "TileSets", NULL, REG_SZ, (BYTE*)szSC2KPaths[8], strlen(szSC2KPaths[8]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Movies", NULL, REG_SZ, (BYTE*)szSC2KPaths[9], strlen(szSC2KPaths[9]) + 1);

		// Write version info
		HKEY hkeySC2KVersion;
		DWORD dwSC2KVersion = 0x00000100;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Version", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KVersion, NULL);
		RegSetValueEx(hkeySC2KVersion, "SCURK", NULL, REG_DWORD, ((BYTE*)&dwSC2KVersion), sizeof(DWORD));
		RegSetValueEx(hkeySC2KVersion, "SimCity 2000", NULL, REG_DWORD, ((BYTE*)&dwSC2KVersion), sizeof(DWORD));

		// Write language info
		HKEY hkeySC2KLocalize;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Localize", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KLocalize, NULL);
		RegSetValueEx(hkeySC2KLocalize, "Language", NULL, REG_SZ, ((BYTE*)"USA"), sizeof("USA"));

		// Write default settings
		HKEY hkeySC2KOptions;
		DWORD dwTrue = 0x00000001;
		DWORD dwFalse = 0x00000000;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KOptions, NULL);
		RegSetValueEx(hkeySC2KOptions, "Disasters", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Music", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Sound", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoGoto", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoBudget", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoSave", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Speed", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));

		// Signal that we had to fake an install.
		return TRUE;
	} else {
		HKEY hkeySC2KPaths;
		LRESULT lResultPaths = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Paths", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KPaths, NULL);
		if (lResultPaths != ERROR_SUCCESS) {
			MessageBox(NULL, "Couldn't open path registry keys for editing", "sc2kfix error", MB_OK | MB_ICONEXCLAMATION);
			ConsoleLog(LOG_ERROR, "CORE: Couldn't open path registry keys for load, error = 0x%08X\n", lResultPaths);
			return FALSE;
		}

		DWORD dwSC2KMoviesPathSize = MAX_PATH;
		LSTATUS retval = RegGetValue(hkeySC2KPaths, NULL, "Movies", RRF_RT_REG_SZ, NULL, szSC2KMoviesPath, &dwSC2KMoviesPathSize);
		switch (retval) {
		case ERROR_SUCCESS:
			break;
		default:
			// Generate paths
			char szSC2KExePath[MAX_PATH] = { 0 };
			GetModuleFileNameEx(GetCurrentProcess(), NULL, szSC2KExePath, MAX_PATH);
			PathRemoveFileSpecA(szSC2KExePath);

			strcpy_s(szSC2KMoviesPath, szSC2KExePath);
			strcat_s(szSC2KMoviesPath, MAX_PATH, "\\MOVIES");

			RegSetValueEx(hkeySC2KPaths, "Movies", NULL, REG_SZ, (BYTE*)szSC2KMoviesPath, strlen(szSC2KMoviesPath) + 1);

			char* buf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, retval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
			ConsoleLog(LOG_WARNING, "CORE: Error loading 'Goodies' path; resetting to default. Reg: %s", buf); // The lack of the newline is deliberate.

			break;
		}
	}
	return FALSE;
}
