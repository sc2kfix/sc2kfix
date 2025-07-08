// sc2kfix modules/registry_config.cpp: registry and pathing hooks and configuration file handling
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define MISCHOOK_DEBUG_REGISTRY 32768
#define MISCHOOK_DEBUG_PATHING 65536

#define REG_KEY_BASE 0x80000040UL

enum redirected_keys_t {
	enMaxisKey,
	enSC2KKey,
	enPathsKey,
	enWindowsKey,
	enVersionKey,
	enOptionsKey,
	enLocalizeKey,
	enRegistrationKey,
	enSCURKKey,

	enCountKey
};

enum regPathVersion {
	REGPATH_UNKNOWN,
	REGPATH_SC2K1996,
	REGPATH_SC2K1995,
	REGPATH_SC2KDEMO,
	REGPATH_SCURK1996
};

static int iRegPathHookMode = REGPATH_UNKNOWN;

const char *gamePrimaryKey = "SimCity 2000";

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
	const char* ini_file = GetIniPath();
	const char* section;

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
	WritePrivateProfileIntA(section, "Speed", 2, ini_file);

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
	const char* ini_file = GetIniPath();
	const char* section = "Registration";

	MigrateRegStringValue(hKeySC2KReg, NULL, "Mayor Name", szSettingsMayorName, sizeof(szSettingsMayorName));
	WritePrivateProfileStringA(section, "Mayor Name", szSettingsMayorName, ini_file);

	MigrateRegStringValue(hKeySC2KReg, NULL, "Company Name", szSettingsCompanyName, sizeof(szSettingsCompanyName));
	WritePrivateProfileStringA(section, "Company Name", szSettingsCompanyName, ini_file);
}

static void MigrateSC2KVersion(void) {
	DWORD dwOut;
	const char* ini_file = GetIniPath();
	const char* section = "Version";
	char szKeyName[128+1];

	memset(szKeyName, 0, sizeof(szKeyName));

	sprintf_s(szKeyName, sizeof(szKeyName)-1, "Software\\Maxis\\%s\\%s", gamePrimaryKey, section);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "SCURK", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SCURK", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "SimCity 2000", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SimCity 2000", dwOut, ini_file);
}

static void MigrateSC2KLocalize(void) {
	const char* ini_file = GetIniPath();
	const char* section = "Localize";
	char szKeyName[128+1];

	memset(szKeyName, 0, sizeof(szKeyName));

	sprintf_s(szKeyName, sizeof(szKeyName)-1, "Software\\Maxis\\%s\\%s", gamePrimaryKey, section);

	char szOutBuf[16];
	MigrateRegStringValue(HKEY_CURRENT_USER, szKeyName, "Language", szOutBuf, sizeof(szOutBuf));
	WritePrivateProfileStringA(section, "Language", szOutBuf, ini_file);
}

static void MigrateSC2KOptions(void) {
	DWORD dwOut;
	const char* ini_file = GetIniPath();
	const char* section = "Options";
	char szKeyName[128+1];

	memset(szKeyName, 0, sizeof(szKeyName));

	sprintf_s(szKeyName, sizeof(szKeyName)-1, "Software\\Maxis\\%s\\%s", gamePrimaryKey, section);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "Disasters", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Disasters", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "Music", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Music", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "Sound", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Sound", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "AutoGoto", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoGoto", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "AutoBudget", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoBudget", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "AutoSave", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "AutoSave", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "Speed", &dwOut, sizeof(BOOL));
	WritePrivateProfileIntA(section, "Speed", dwOut, ini_file);
}

static void MigrateSC2KSCURK(void) {
	DWORD dwOut;
	const char* ini_file = GetIniPath();
	const char* section = "SCURK";
	char szKeyName[128+1];

	memset(szKeyName, 0, sizeof(szKeyName));

	sprintf_s(szKeyName, sizeof(szKeyName)-1, "Software\\Maxis\\%s\\%s", gamePrimaryKey, section);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "CycleColors", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "CycleColors", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "GridHeight", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "GridHeight", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "GridWidth", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "GridWidth", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "ShowClipRegion", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "ShowClipRegion", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "ShowDrawGrid", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "ShowDrawGrid", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "SnapToGrid", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "SnapToGrid", dwOut, ini_file);

	MigrateRegDWORDValue(HKEY_CURRENT_USER, szKeyName, "Sound", &dwOut, sizeof(DWORD));
	WritePrivateProfileIntA(section, "Sound", dwOut, ini_file);
}

static void MigrateFinalize(void) {
	const char* ini_file = GetIniPath();
	const char* section = "Portable";

	WritePrivateProfileIntA(section, "Installed", 1, ini_file);
}

int DoRegistryCheckAndInstall(void) {
	int ret;
	char szKeyName[128+1];

	memset(szKeyName, 0, sizeof(szKeyName));

	sprintf_s(szKeyName, sizeof(szKeyName)-1, "Software\\Maxis\\%s\\%s", gamePrimaryKey, "Registration");

	HKEY hKeySC2KRegistration;
	LSTATUS lResultRegistration = RegOpenKeyExA(HKEY_CURRENT_USER, szKeyName, NULL, KEY_ALL_ACCESS, &hKeySC2KRegistration);
	if (lResultRegistration != ERROR_SUCCESS) {
		// Let's install.
		InstallSC2KDefaults();
		return 0;
	}

	ret = 0;
	if (szSettingsMayorName[0] == 0 ||
		szSettingsCompanyName[0] == 0) {
		if (RegQueryValueEx(hKeySC2KRegistration, "Mayor Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND ||
			RegQueryValueEx(hKeySC2KRegistration, "Company Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND) {

			// Fake an install.

			InstallSC2KDefaults();

			// Signal that we had to fake an install.
			ret = 2;
		}
		else {

			// Migrate from registry to ini.
			MigrateSC2KRegistration(hKeySC2KRegistration);

			MigrateSC2KVersion();
			MigrateSC2KLocalize();
			MigrateSC2KOptions();
			MigrateSC2KSCURK();
			MigrateFinalize();

			SaveSettings(TRUE);
			ret = 1;
		}
	}

	RegCloseKey(hKeySC2KRegistration);
	return ret;
}

static BOOL IsRegKey(HKEY hKey, int rkVal) {
	if (rkVal < enMaxisKey || rkVal >= enCountKey)
		return FALSE;

	if (hKey == (HKEY)(REG_KEY_BASE + (rkVal)))
		return TRUE;

	return FALSE;
}

static BOOL IsFakeRegKey(unsigned long ulKey) {
	if ((ulKey) >= (REG_KEY_BASE + enMaxisKey) && (ulKey) < (REG_KEY_BASE + enCountKey))
		return TRUE;

	return FALSE;
}

static int GetRedirectKey(HKEY hKey) {
	for (int i = 0; i < enCountKey; i++)
		if (IsRegKey(hKey, i))
			return i;

	return enCountKey;
}

static BOOL RegLookup(const char *lpSubKey, unsigned long *ulKey) {
	BOOL ret;

	ret = FALSE;
	if (strcmp(lpSubKey, "Maxis") == 0) {
		*ulKey = enMaxisKey;
		ret = TRUE;
	}
	else if (strcmp(lpSubKey, gamePrimaryKey) == 0) {
		*ulKey = enSC2KKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Paths") == 0) {
		*ulKey = enPathsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Windows") == 0) {
		*ulKey = enWindowsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Version") == 0) {
		*ulKey = enVersionKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Options") == 0) {
		*ulKey = enOptionsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Localize") == 0) {
		*ulKey = enLocalizeKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Registration") == 0) {
		*ulKey = enRegistrationKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "SCURK") == 0) {
		*ulKey = enSCURKKey;
		ret = TRUE;
	}
	return ret;
}

static const char *SectionLookup(HKEY hKey) {
	switch (GetRedirectKey(hKey)) {
		case enWindowsKey:
			return "Windows";
		case enVersionKey:
			return "Version";
		case enOptionsKey:
			return "Options";
		case enLocalizeKey:
			return "Localize";
		case enSCURKKey:
			return "SCURK";
		default:
			break;
	}
	return NULL;
}

static void GetOutString(const char *sString, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL)
		*lpcbData = strlen(sString) + 1;
	else
		memcpy(lpData, sString, *lpcbData);
}

static void GetOutDWORD(DWORD dwValue, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL)
		*lpcbData = sizeof(DWORD);
	else
		memcpy(lpData, &dwValue, sizeof(DWORD));
}

// Reference and inspiration for this comes from the separate
// 'simcity-noinstall' project.
const char *AdjustSource(char *buf, const char *path) {
	static char def_data_path[] = "A:\\DATA\\";

	def_data_path[0] = szGamePath[0];

	int plen = strlen(path);
	int flen = strlen(def_data_path);
	if (plen <= flen || _strnicmp(def_data_path, path, flen) != 0) {
		return path;
	}

	char temp[MAX_PATH + 1];

	memset(temp, 0, sizeof(temp));

	strcpy_s(temp, MAX_PATH, path + (flen - 1));

	strcpy_s(buf, MAX_PATH, szGamePath);
	strcat_s(buf, MAX_PATH, "\\Movies");
	strcat_s(buf, MAX_PATH, temp);

	return buf;
}

static void GamePathAdjust(const char *szBasePath, const char *target, LPBYTE lpData, LPDWORD lpcbData) {
	char szTarget[MAX_PATH];

	sprintf_s(szTarget, MAX_PATH, "%s\\%s", szBasePath, target);
	GetOutString(szTarget, lpData, lpcbData);
}

static void GetIniOutString(const char *section, const char *key, const char *sValue, LPBYTE lpData, LPDWORD lpcbData) {
	const char *ini_file = GetIniPath();

	char szBuf[64];
	GetPrivateProfileStringA(section, key, sValue, szBuf, sizeof(szBuf) - 1, ini_file);
	GetOutString(szBuf, lpData, lpcbData);
}

static void GetIniOutDWORD(const char *section, const char *key, DWORD dvVal, LPBYTE lpData, LPDWORD lpcbData) {
	const char *ini_file = GetIniPath();

	GetOutDWORD(GetPrivateProfileIntA(section, key, dvVal, ini_file), lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD dwReserved, DWORD dwType, const BYTE *lpData, DWORD cbData) {
	const char *section = SectionLookup(hKey);

	if (section) {
		const char *ini_file;

		ini_file = GetIniPath();
		if (dwType == REG_DWORD || (dwType == REG_BINARY && cbData == sizeof(DWORD))) {
			WritePrivateProfileIntA(section, lpValueName, *(const DWORD*)lpData, ini_file);
		}
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegSetValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			dwReserved, dwType, *lpData, cbData);

	return RegSetValueExA(hKey, lpValueName, dwReserved, dwType, lpData, cbData);
}

extern "C" LSTATUS __stdcall Hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	if (IsRegKey(hKey, enPathsKey)) {
		char szTargetPath[MAX_PATH];

		strcpy_s(szTargetPath, MAX_PATH, szGamePath);
		if (_stricmp(lpValueName, "Goodies") == 0) {
			if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
				ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> Query Adjustment - %s -> %s\n", _ReturnAddress(), lpValueName, "MOVIES");
			GamePathAdjust(szTargetPath, "Movies", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Cities") == 0 ||
			_stricmp(lpValueName, "SaveGame") == 0) {
			GamePathAdjust(szTargetPath, "Cities", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Data") == 0) {
			GamePathAdjust(szTargetPath, "Data", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Graphics") == 0) {
			GamePathAdjust(szTargetPath, "Bitmaps", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Home") == 0) {
			GetOutString(szTargetPath, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Music") == 0) {
			GamePathAdjust(szTargetPath, "Sounds", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Scenarios") == 0) {
			GamePathAdjust(szTargetPath, "Scenario", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "TileSets") == 0) {
			GamePathAdjust(szTargetPath, "ScurkArt", lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enWindowsKey)) {
		if (_stricmp(lpValueName, "Display") == 0) {
			GetIniOutString("Windows", lpValueName, "8 1", lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Color Check") == 0) {
			GetIniOutDWORD("Windows", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Last Color Depth") == 0) {
			GetIniOutDWORD("Windows", lpValueName, 20, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enVersionKey)) {
		if (_stricmp(lpValueName, "SimCity 2000") == 0) {
			GetIniOutDWORD("Version", lpValueName, 256, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "SCURK") == 0) {
			GetIniOutDWORD("Version", lpValueName, 256, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enOptionsKey)) {
		if (_stricmp(lpValueName, "Disasters") == 0) {
			GetIniOutDWORD("Options", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Music") == 0) {
			GetIniOutDWORD("Options", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Sound") == 0) {
			GetIniOutDWORD("Options", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "AutoGoto") == 0) {
			GetIniOutDWORD("Options", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "AutoBudget") == 0) {
			GetIniOutDWORD("Options", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "AutoSave") == 0) {
			GetIniOutDWORD("Options", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Speed") == 0) {
			GetIniOutDWORD("Options", lpValueName, 2, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enLocalizeKey)) {
		if (_stricmp(lpValueName, "Language") == 0) {
			GetIniOutString("Localize", lpValueName, "USA", lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enRegistrationKey)) {
		if (_stricmp(lpValueName, "Mayor Name") == 0) {
			GetIniOutString("Registration", lpValueName, szSettingsMayorName, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Company Name") == 0) {
			GetIniOutString("Registration", lpValueName, szSettingsCompanyName, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enSCURKKey)) {
		if (_stricmp(lpValueName, "CycleColors") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 1, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "GridHeight") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 2, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "GridWidth") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 2, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "ShowClipRegion") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "ShowDrawGrid") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "SnapToGrid") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 0, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, "Sound") == 0) {
			GetIniOutDWORD("SCURK", lpValueName, 1, lpData, lpcbData);
		}
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegQueryValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			lpReserved, *lpType, lpData, lpcbData);

	return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD dwReserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
	const LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition) {
	unsigned long ulKey;

	ulKey = 0;
	if (RegLookup(lpSubKey, &ulKey)) {
		*phkResult = (HKEY)(REG_KEY_BASE + ulKey);
		return ERROR_SUCCESS;
	}

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCreateKeyExA(0x%08x, %s, ...)\n", _ReturnAddress(), hKey, lpSubKey);

	return RegCreateKeyExA(hKey, lpSubKey, dwReserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}

extern "C" LSTATUS __stdcall Hook_RegCloseKey(HKEY hKey) {
	if (IsFakeRegKey((unsigned long)hKey))
		return ERROR_SUCCESS;

	if (mischook_debug & MISCHOOK_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCloseKey(0x%08x)\n", _ReturnAddress(), hKey);

	return RegCloseKey(hKey);
}

extern "C" HANDLE __stdcall Hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	if (mischook_debug & MISCHOOK_DEBUG_PATHING)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), lpFileName,
			dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (iRegPathHookMode == REGPATH_SC2K1996) {
		if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
			(DWORD)_ReturnAddress() == 0x48A810) {
			char buf[MAX_PATH + 1];

			memset(buf, 0, sizeof(buf));

			HANDLE hFileHandle = CreateFileA(AdjustSource(buf, lpFileName), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			if (mischook_debug & MISCHOOK_DEBUG_PATHING)
				ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X) (0x%08x)\n", _ReturnAddress(), lpFileName,
					dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, hFileHandle);
			return hFileHandle;
		}
	}
	return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" HANDLE __stdcall Hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
	if (mischook_debug & MISCHOOK_DEBUG_PATHING)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> FindFirstFileA(%s, 0x%08X)\n", _ReturnAddress(), lpFileName, lpFindFileData);

	if (iRegPathHookMode == REGPATH_SC2K1996) {
		if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
			(DWORD)_ReturnAddress() == 0x48A810) {
			char buf[MAX_PATH + 1];

			memset(buf, 0, sizeof(buf));

			HANDLE hFileHandle = FindFirstFileA(AdjustSource(buf, lpFileName), lpFindFileData);
			if (mischook_debug & MISCHOOK_DEBUG_PATHING)
				ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> FindFirstFileA(%s, 0x%08X) (0x%08x)\n", _ReturnAddress(), buf, lpFindFileData, hFileHandle);
			return hFileHandle;
		}
	}
	return FindFirstFileA(lpFileName, lpFindFileData);
}

void InstallRegistryPathingHooks_SC2K1996(void) {
	iRegPathHookMode = REGPATH_SC2K1996;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4EF7F8) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EF800) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EF80C) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EF810) = (DWORD)Hook_RegCloseKey;

	// Install CreateFileA hook
	*(DWORD*)(0x4EFADC) = (DWORD)Hook_CreateFileA;

	// Install FindFirstFileA hook
	*(DWORD*)(0x4EFB8C) = (DWORD)Hook_FindFirstFileA;
}

void InstallRegistryPathingHooks_SC2K1995(void) {
	iRegPathHookMode = REGPATH_SC2K1995;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4EE7A8) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EE7A4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EE7A0) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EE7AC) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SC2KDemo(void) {
	iRegPathHookMode = REGPATH_SC2KDEMO;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4D7768) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4D7760) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4D776C) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4D7770) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SCURK1996(void) {
	iRegPathHookMode = REGPATH_SCURK1996;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4B05F0) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4B05E4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4B05E8) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4B05EC) = (DWORD)Hook_RegCloseKey;
}
