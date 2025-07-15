// sc2kfix modules/settings.cpp: settings dialog code and configurator
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

static DWORD dwDummy;

char szGamePath[MAX_PATH];

char szSettingsMayorName[64];
char szSettingsCompanyName[64];

BOOL bSettingsMusicInBackground = TRUE;
BOOL bSettingsUseSoundReplacements = TRUE;
BOOL bSettingsShuffleMusic = FALSE;
BOOL bSettingsUseMultithreadedMusic = TRUE;
BOOL bSettingsFrequentCityRefresh = TRUE;
BOOL bSettingsUseMP3Music = FALSE;

BOOL bSettingsAlwaysConsole = FALSE;
BOOL bSettingsCheckForUpdates = TRUE;
BOOL bSettingsDontLoadMods = FALSE;

BOOL bSettingsUseStatusDialog = FALSE;
BOOL bSettingsTitleCalendar = TRUE;
BOOL bSettingsUseNewStrings = TRUE;
BOOL bSettingsAlwaysSkipIntro = FALSE;

void SetGamePath(void) {
	char szModulePathName[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), NULL, szModulePathName, MAX_PATH);

	PathRemoveFileSpecA(szModulePathName);
	strcpy_s(szGamePath, MAX_PATH, szModulePathName);
}

const char *GetIniPath() {
	static char szIniPath[MAX_PATH];

	sprintf_s(szIniPath, MAX_PATH, "%s\\%s", szGamePath, SC2KFIX_INIFILE);
	return szIniPath;
}

static void MigrateSC2KFixSettings(void) {
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsMusicInBackground", &bSettingsMusicInBackground);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseSoundReplacements", &bSettingsUseSoundReplacements);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsShuffleMusic", &bSettingsShuffleMusic);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseMultithreadedMusic", &bSettingsUseMultithreadedMusic);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsFrequentCityRefresh", &bSettingsFrequentCityRefresh);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseMP3Music", &bSettingsUseMP3Music);

	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsAlwaysConsole", &bSettingsAlwaysConsole);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsCheckForUpdates", &bSettingsCheckForUpdates);

	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseStatusDialog", &bSettingsUseStatusDialog);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsTitleCalendar", &bSettingsTitleCalendar);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseNewStrings", &bSettingsUseNewStrings);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsAlwaysSkipIntro", &bSettingsAlwaysSkipIntro);
}

void LoadSettings(void) {
	const char *ini_file = GetIniPath();
	const char *section = "Registration";
	GetPrivateProfileStringA(section, "Mayor Name", "", szSettingsMayorName, sizeof(szSettingsMayorName)-1, ini_file);
	GetPrivateProfileStringA(section, "Company Name", "", szSettingsCompanyName, sizeof(szSettingsCompanyName)-1, ini_file);

	section = "sc2kfix";
	char szSectionBuf[32];

	// Check for the section presence.
	if (!GetPrivateProfileSectionA(section, szSectionBuf, sizeof(szSectionBuf) - 1, ini_file)) {
		// Check to see whether the values existed in the registry so they can be migrated.
		MigrateSC2KFixSettings();
	}

	// QoL/performance settings
	bSettingsMusicInBackground = GetPrivateProfileIntA(section, "bSettingsMusicInBackground", bSettingsMusicInBackground, ini_file);
	bSettingsUseSoundReplacements = GetPrivateProfileIntA(section, "bSettingsUseSoundReplacements", bSettingsUseSoundReplacements, ini_file);
	bSettingsShuffleMusic = GetPrivateProfileIntA(section, "bSettingsShuffleMusic", bSettingsShuffleMusic, ini_file);
	bSettingsUseMultithreadedMusic = GetPrivateProfileIntA(section, "bSettingsUseMultithreadedMusic", bSettingsUseMultithreadedMusic, ini_file);
	bSettingsFrequentCityRefresh = GetPrivateProfileIntA(section, "bSettingsFrequentCityRefresh", bSettingsFrequentCityRefresh, ini_file);
	bSettingsUseMP3Music = GetPrivateProfileIntA(section, "bSettingsUseMP3Music", bSettingsUseMP3Music, ini_file);

	// Internal settings
	bSettingsAlwaysConsole = GetPrivateProfileIntA(section, "bSettingsAlwaysConsole", bSettingsAlwaysConsole, ini_file);
	bSettingsCheckForUpdates = GetPrivateProfileIntA(section, "bSettingsCheckForUpdates", bSettingsCheckForUpdates, ini_file);
	bSettingsDontLoadMods = GetPrivateProfileIntA(section, "bSettingsDontLoadMods", bSettingsDontLoadMods, ini_file);

	// Interface settings
	bSettingsUseStatusDialog = GetPrivateProfileIntA(section, "bSettingsUseStatusDialog", bSettingsUseStatusDialog, ini_file);
	bSettingsTitleCalendar = GetPrivateProfileIntA(section, "bSettingsTitleCalendar", bSettingsTitleCalendar, ini_file);
	bSettingsUseNewStrings = GetPrivateProfileIntA(section, "bSettingsUseNewStrings", bSettingsUseNewStrings, ini_file);
	bSettingsAlwaysSkipIntro = GetPrivateProfileIntA(section, "bSettingsAlwaysSkipIntro", bSettingsAlwaysSkipIntro, ini_file);

	SaveSettings(TRUE);
}

void SaveSettings(BOOL onload) {
	const char *ini_file = GetIniPath();
	const char *section = "Registration";

	WritePrivateProfileStringA(section, "Mayor Name", szSettingsMayorName, ini_file);
	WritePrivateProfileStringA(section, "Company Name", szSettingsCompanyName, ini_file);

	section = "sc2kfix";

	// QoL/performance settings
	WritePrivateProfileIntA(section, "bSettingsMusicInBackground", bSettingsMusicInBackground, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseSoundReplacements", bSettingsUseSoundReplacements, ini_file);
	WritePrivateProfileIntA(section, "bSettingsShuffleMusic", bSettingsShuffleMusic, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseMultithreadedMusic", bSettingsUseMultithreadedMusic, ini_file);
	WritePrivateProfileIntA(section, "bSettingsFrequentCityRefresh", bSettingsFrequentCityRefresh, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseMP3Music", bSettingsUseMP3Music, ini_file);

	// Internal settings
	WritePrivateProfileIntA(section, "bSettingsAlwaysConsole", bSettingsAlwaysConsole, ini_file);
	WritePrivateProfileIntA(section, "bSettingsCheckForUpdates", bSettingsCheckForUpdates, ini_file);
	WritePrivateProfileIntA(section, "bSettingsDontLoadMods", bSettingsDontLoadMods, ini_file);

	// Interface settings
	WritePrivateProfileIntA(section, "bSettingsUseStatusDialog", bSettingsUseStatusDialog, ini_file);
	WritePrivateProfileIntA(section, "bSettingsTitleCalendar", bSettingsTitleCalendar, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseNewStrings", bSettingsUseNewStrings, ini_file);
	WritePrivateProfileIntA(section, "bSettingsAlwaysSkipIntro", bSettingsAlwaysSkipIntro, ini_file);

	ConsoleLog(LOG_INFO, "CORE: Saved sc2kfix settings.\n");

	if (!onload) {
		// Update any hooks we need to.
		if (dwDetectedVersion == SC2KVERSION_1996) {
			UpdateMiscHooks_SC2K1996();
			UpdateStatus_SC2K1996();
		}
	}
}

static void SetSettingsTabOrdering(HWND hwndDlg) {
	UINT uFlags = (SWP_NOMOVE | SWP_NOSIZE);

	// Entries are defined in reverse order.

	// Bottom Buttons
	SetWindowPos(GetDlgItem(hwndDlg, ID_SETTINGS_VANILLA), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, ID_SETTINGS_DEFAULTS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, ID_SETTINGS_CANCEL), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, ID_SETTINGS_OK), NULL, 0, 0, 0, 0, uFlags);

	// sc2kfix Core Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), NULL, 0, 0, 0, 0, uFlags);

	// Interface Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), NULL, 0, 0, 0, 0, uFlags);

	// Quality of Life / Performance Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), NULL, 0, 0, 0, 0, uFlags);

	// Game Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_COMPANY), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_MAYOR), NULL, 0, 0, 0, 0, uFlags);
}

BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	std::string strVersionInfo;
	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		SetSettingsTabOrdering(hwndDlg);

		// Create tooltips.
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_OK),
			"Saves the currently selected settings and closes the settings dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_CANCEL),
			"Discards changed settings and closes the settings dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_DEFAULTS),
			"Changes the settings to the default sc2kfix experience but does not save settings or close the dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_VANILLA),
			"Changes the settings to disable all quality of life, interface and gameplay enhancements but does not save settings or close the dialog.");

		// QoL/Performance settings
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC),
			"By default, SimCity 2000 stops the currently-playing song when the game window loses focus. This setting continues playing music in the background until the end of the track, "
			"after which a new song will be selected when the game window regains focus.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS),
			"Certain versions of SimCity 2000 had higher quality sounds than the Windows 95 versions. "
			"This setting controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC),
			"By default, SimCity 2000 selects \"random\" music by playing the next track in a looping playlist of songs. "
			"This setting controls whether or not to shuffle the playlist when the game starts and when the end of the playlist is reached.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC),
			"This setting enables the multithreaded music engine introduced in Release 8, which reduces game freezes when a new random song is loaded or a song is interrupted by an event-specific song.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE),
			"SimCity 2000 was designed to spend more CPU time on simulation than on rendering by only updating the city's growth when the display moves or on the 24th day of the month. "
			"Enabling this setting allows the game to refresh the city display in real-time instead of batching display updates.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC),
			"Enabling this setting will play music from MP3s instead of MIDI files if you have previously-rendered MP3 versions of the game's soundtrack in the SOUNDS directory.");

		// sc2kfix core settings
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE),
			"sc2kfix has a debugging console that can be activated by passing the -console argument to SimCity 2000's command line. "
			"This setting forces sc2kfix to always start the console along with the game, even if the -console argument is not passed.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES),
			"This setting checks to see if there's a newer release of sc2kfix available when the game starts.\n\n");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS),
			"Enabling this setting forces sc2kfix to skip loading any installed mods on startup.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");

		// Interface settings
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG),
			"The DOS and Mac versions of SimCity 2000 used a movable floating dialog to show the current tool, status line, and weather instead of a fixed bar at the bottom of the game window. "
			"Enabling this setting will use the floating status dialog instead of the bottom status bar.\n\n"
			
			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE),
			"By default the title bar only displays the month and year. Enabling this setting will display the full in-game date instead.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS),
			"Certain strings in the game have typos, grammatical issues, and/or ambiguous wording. This setting loads corrected strings in memory in place of the affected originals.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO),
			"Once enabled the introduction videos will be skipped on startup (This will only apply if the videos have been detected, otherwise the standard warning will be displayed).");

		// Set the version string.
		strVersionInfo = "sc2kfix ";
		strVersionInfo += szSC2KFixVersion;
		strVersionInfo += " (";
		strVersionInfo += szSC2KFixReleaseTag;
		strVersionInfo += ")";
		SetDlgItemText(hwndDlg, IDC_STATIC_VERSIONINFO, strVersionInfo.c_str());

		// Load the existing settings into the dialog
		SetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szSettingsMayorName);
		SetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szSettingsCompanyName);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), bSettingsMusicInBackground ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), bSettingsUseSoundReplacements ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), bSettingsShuffleMusic ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC), bSettingsUseMultithreadedMusic ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), bSettingsFrequentCityRefresh ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC), bSettingsUseMP3Music ? BST_CHECKED : BST_UNCHECKED);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), bSettingsAlwaysConsole ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES), bSettingsCheckForUpdates ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS), bSettingsDontLoadMods ? BST_CHECKED : BST_UNCHECKED);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), bSettingsUseStatusDialog ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), bSettingsTitleCalendar ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), bSettingsUseNewStrings ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), bSettingsAlwaysSkipIntro ? BST_CHECKED : BST_UNCHECKED);

		// Center the dialog box
		CenterDialogBox(hwndDlg);
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
			bSettingsUseSoundReplacements = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS));
			bSettingsShuffleMusic = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC));
			bSettingsUseMultithreadedMusic = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC));
			bSettingsFrequentCityRefresh = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE));
			bSettingsUseMP3Music = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC));

			bSettingsAlwaysConsole = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE));
			bSettingsCheckForUpdates = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES));
			bSettingsDontLoadMods = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS));

			bSettingsUseStatusDialog = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG));
			bSettingsTitleCalendar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE));
			bSettingsUseNewStrings = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS));
			bSettingsAlwaysSkipIntro = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO));

			// Save the settings
			SaveSettings(FALSE);
			EndDialog(hwndDlg, wParam);
			break;
		case ID_SETTINGS_CANCEL:
			EndDialog(hwndDlg, wParam);
			break;
		case ID_SETTINGS_DEFAULTS:
			// Set all the checkboxes to the defaults.
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC), BST_UNCHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS), BST_UNCHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), BST_UNCHECKED);
			break;
		case ID_SETTINGS_VANILLA:
			// Clear all checkboxes except for the update checker.
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MULTITHREADED_MUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_MP3_MUSIC), BST_UNCHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS), BST_CHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), BST_UNCHECKED);
			break;
		}
		return TRUE;
	}
	return FALSE;
}

void ShowSettingsDialog(void) {
	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS), NULL, SettingsDialogProc);
}