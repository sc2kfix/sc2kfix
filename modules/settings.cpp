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

json::JSON jsonSettingsCore;
json::JSON jsonSettingsMods;

char szGamePath[MAX_PATH];

char szSettingsMayorName[64];
char szSettingsCompanyName[64];

BOOL bSettingsMusicInBackground = TRUE;
BOOL bSettingsUseSoundReplacements = TRUE;
BOOL bSettingsShuffleMusic = FALSE;
BOOL bSettingsUseMultithreadedMusic = TRUE;
BOOL bSettingsFrequentCityRefresh = TRUE;
BOOL bSettingsUseMP3Music = FALSE;
BOOL bSettingsAlwaysPlayMusic = FALSE;

BOOL bSettingsAlwaysConsole = FALSE;
BOOL bSettingsCheckForUpdates = TRUE;
BOOL bSettingsDontLoadMods = FALSE;

BOOL bSettingsUseStatusDialog = FALSE;
BOOL bSettingsTitleCalendar = TRUE;
BOOL bSettingsUseNewStrings = TRUE;
BOOL bSettingsAlwaysSkipIntro = FALSE;

UINT iSettingsMusicEngineOutput = MUSIC_ENGINE_SEQUENCER;
char szSettingsFluidSynthSoundfont[MAX_PATH + 1];

char szSettingsMIDITrackPath[MUSIC_TRACKS][MAX_PATH + 1];
char szSettingsMP3TrackPath[MUSIC_TRACKS][MAX_PATH + 1];

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

void InitializeSettings(void) {
	// Initialize JSON objects
	jsonSettingsCore = json::Object();
	jsonSettingsMods = json::Object();

	// Set up original SC2K settings
	jsonSettingsCore["SimCity2000"]["Version"]["SCURK"] = 256;
	jsonSettingsCore["SimCity2000"]["Version"]["SimCity 2000"] = 256;
	jsonSettingsCore["SimCity2000"]["Localize"]["Language"] = "USA";

	jsonSettingsCore["SimCity2000"]["Options"]["Disasters"] = 1;
	jsonSettingsCore["SimCity2000"]["Options"]["Music"] = 1;
	jsonSettingsCore["SimCity2000"]["Options"]["Sound"] = 1;
	jsonSettingsCore["SimCity2000"]["Options"]["AutoGoto"] = 1;
	jsonSettingsCore["SimCity2000"]["Options"]["AutoBudget"] = 0;
	jsonSettingsCore["SimCity2000"]["Options"]["AutoSave"] = 0;
	jsonSettingsCore["SimCity2000"]["Options"]["Speed"] = 0;

	jsonSettingsCore["SimCity2000"]["SCURK"]["CycleColors"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["GridHeight"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["GridWidth"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["ShowClipRegion"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["ShowDrawGrid"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["SnapToGrid"] = 0;
	jsonSettingsCore["SimCity2000"]["SCURK"]["Sound"] = 0;

	jsonSettingsCore["SimCity2000"]["Windows"]["Last Color Depth"] = 32;
}

static void MigrateSC2KFixSettings(void) {
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsMusicInBackground", &bSettingsMusicInBackground);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseSoundReplacements", &bSettingsUseSoundReplacements);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsShuffleMusic", &bSettingsShuffleMusic);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsFrequentCityRefresh", &bSettingsFrequentCityRefresh);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseMP3Music", &bSettingsUseMP3Music);

	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsAlwaysConsole", &bSettingsAlwaysConsole);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsCheckForUpdates", &bSettingsCheckForUpdates);

	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseStatusDialog", &bSettingsUseStatusDialog);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsTitleCalendar", &bSettingsTitleCalendar);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsUseNewStrings", &bSettingsUseNewStrings);
	MigrateRegBOOLValue(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\sc2kfix", "bSettingsAlwaysSkipIntro", &bSettingsAlwaysSkipIntro);
}

const char* SettingsSaveMusicEngine(UINT iMusicEngine) {
	switch (iMusicEngine) {
	case MUSIC_ENGINE_NONE:
		return "none";
	case MUSIC_ENGINE_SEQUENCER:
		return "sequencer";
	case MUSIC_ENGINE_FLUIDSYNTH:
		return "fluidsynth";
	case MUSIC_ENGINE_MP3:
		return "mp3";
	default:
		return "sequencer";
	}
}

UINT SettingsLoadMusicEngine(const char* szMusicEngine) {
	if (!strcmp(szMusicEngine, "none"))
		return MUSIC_ENGINE_NONE;
	if (!strcmp(szMusicEngine, "sequencer"))
		return MUSIC_ENGINE_SEQUENCER;
	if (!strcmp(szMusicEngine, "fluidsynth"))
		return MUSIC_ENGINE_FLUIDSYNTH;
	if (!strcmp(szMusicEngine, "mp3"))
		return MUSIC_ENGINE_MP3;
	return MUSIC_ENGINE_SEQUENCER;
}

void LoadSettings(void) {
	const char *ini_file = GetIniPath();
	const char *section = "Registration";
	GetPrivateProfileStringA(section, "Mayor Name", "", szSettingsMayorName, sizeof(szSettingsMayorName)-1, ini_file);
	GetPrivateProfileStringA(section, "Company Name", "", szSettingsCompanyName, sizeof(szSettingsCompanyName)-1, ini_file);

	section = "sc2kfix";
	char szSectionBuf[32];

	memset(szSettingsMIDITrackPath, 0, sizeof(szSettingsMIDITrackPath));
	memset(szSettingsMP3TrackPath, 0, sizeof(szSettingsMP3TrackPath));

	// Check for the section presence.
	if (!GetPrivateProfileSectionA(section, szSectionBuf, sizeof(szSectionBuf) - 1, ini_file)) {
		// Check to see whether the values existed in the registry so they can be migrated.
		MigrateSC2KFixSettings();
	}

	// QoL/performance settings
	char szSettingsMusicEngineOutput[32];
	GetPrivateProfileStringA(section, "szSettingsMusicEngineOutput", "sequencer", szSettingsMusicEngineOutput, 31, ini_file);
	iSettingsMusicEngineOutput = SettingsLoadMusicEngine(szSettingsMusicEngineOutput);
	if (!hmodFluidSynth && iSettingsMusicEngineOutput == MUSIC_ENGINE_FLUIDSYNTH) {
		ConsoleLog(LOG_ERROR, "CORE: FluidSynth music engine selected but library not available; falling back to MIDI sequencer.\n");
		iSettingsMusicEngineOutput = MUSIC_ENGINE_SEQUENCER;
	}
	GetPrivateProfileStringA(section, "szSettingsFluidSynthSoundfont", "", szSettingsFluidSynthSoundfont, MAX_PATH, ini_file);

	bSettingsMusicInBackground = GetPrivateProfileIntA(section, "bSettingsMusicInBackground", bSettingsMusicInBackground, ini_file);
	bSettingsUseSoundReplacements = GetPrivateProfileIntA(section, "bSettingsUseSoundReplacements", bSettingsUseSoundReplacements, ini_file);
	bSettingsShuffleMusic = GetPrivateProfileIntA(section, "bSettingsShuffleMusic", bSettingsShuffleMusic, ini_file);
	bSettingsFrequentCityRefresh = GetPrivateProfileIntA(section, "bSettingsFrequentCityRefresh", bSettingsFrequentCityRefresh, ini_file);
	bSettingsUseMP3Music = GetPrivateProfileIntA(section, "bSettingsUseMP3Music", bSettingsUseMP3Music, ini_file);
	bSettingsAlwaysPlayMusic = GetPrivateProfileIntA(section, "bSettingsAlwaysPlayMusic", bSettingsAlwaysPlayMusic, ini_file);

	// Internal settings
	bSettingsAlwaysConsole = GetPrivateProfileIntA(section, "bSettingsAlwaysConsole", bSettingsAlwaysConsole, ini_file);
	bSettingsCheckForUpdates = GetPrivateProfileIntA(section, "bSettingsCheckForUpdates", bSettingsCheckForUpdates, ini_file);
	bSettingsDontLoadMods = GetPrivateProfileIntA(section, "bSettingsDontLoadMods", bSettingsDontLoadMods, ini_file);

	// Interface settings
	bSettingsUseStatusDialog = GetPrivateProfileIntA(section, "bSettingsUseStatusDialog", bSettingsUseStatusDialog, ini_file);
	bSettingsTitleCalendar = GetPrivateProfileIntA(section, "bSettingsTitleCalendar", bSettingsTitleCalendar, ini_file);
	bSettingsUseNewStrings = GetPrivateProfileIntA(section, "bSettingsUseNewStrings", bSettingsUseNewStrings, ini_file);
	bSettingsAlwaysSkipIntro = GetPrivateProfileIntA(section, "bSettingsAlwaysSkipIntro", bSettingsAlwaysSkipIntro, ini_file);

	// FluidSynth settings (experimental -- not exposed to GUI yet)
	if (iSettingsMusicEngineOutput == MUSIC_ENGINE_FLUIDSYNTH) {
		ConsoleLog(LOG_INFO, "CORE: FluidSynth music engine enabled.\n");

		if (!strcmp(szSettingsFluidSynthSoundfont, "") || !FileExists(szSettingsFluidSynthSoundfont)) {
			ConsoleLog(LOG_ERROR, "CORE: FluidSynth soundfont not specified or does not exist; falling back to MIDI sequencer.\n");
			iSettingsMusicEngineOutput = MUSIC_ENGINE_SEQUENCER;
		} else
			ConsoleLog(LOG_INFO, "CORE: Using \"%s\" as FluidSynth soundfont.\n", szSettingsFluidSynthSoundfont);
	}

	char szKeyBuf[32];

	section = "sc2kfix.music.MIDI";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMIDITrackPath[i], sizeof(szSettingsMIDITrackPath[i]) - 1, ini_file);
	}

	section = "sc2kfix.music.MP3";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMP3TrackPath[i], sizeof(szSettingsMP3TrackPath[i]) - 1, ini_file);
	}

	SaveSettings(TRUE);
}

void SaveSettings(BOOL onload) {
	const char *ini_file = GetIniPath();
	const char *section = "Registration";

	WritePrivateProfileStringA(section, "Mayor Name", szSettingsMayorName, ini_file);
	WritePrivateProfileStringA(section, "Company Name", szSettingsCompanyName, ini_file);

	section = "sc2kfix";

	// QoL/performance settings
	WritePrivateProfileStringA(section, "szSettingsMusicEngineOutput", SettingsSaveMusicEngine(iSettingsMusicEngineOutput), ini_file);
	WritePrivateProfileStringA(section, "szSettingsFluidSynthSoundfont", szSettingsFluidSynthSoundfont, ini_file);
	WritePrivateProfileIntA(section, "bSettingsMusicInBackground", bSettingsMusicInBackground, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseSoundReplacements", bSettingsUseSoundReplacements, ini_file);
	WritePrivateProfileIntA(section, "bSettingsShuffleMusic", bSettingsShuffleMusic, ini_file);
	WritePrivateProfileIntA(section, "bSettingsFrequentCityRefresh", bSettingsFrequentCityRefresh, ini_file);
	WritePrivateProfileIntA(section, "bSettingsUseMP3Music", bSettingsUseMP3Music, ini_file);
	WritePrivateProfileIntA(section, "bSettingsAlwaysPlayMusic", bSettingsAlwaysPlayMusic, ini_file);

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

	char szKeyBuf[32];

	section = "sc2kfix.music.MIDI";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		WritePrivateProfileStringA(section, szKeyBuf, szSettingsMIDITrackPath[i], ini_file);
	}

	section = "sc2kfix.music.MP3";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		WritePrivateProfileStringA(section, szKeyBuf, szSettingsMP3TrackPath[i], ini_file);
	}

	if (!onload) {
		// Update any hooks we need to.
		if (dwDetectedVersion == SC2KVERSION_1996) {
			UpdateMiscHooks_SC2K1996();
			UpdateStatus_SC2K1996(-1);
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
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC), NULL, 0, 0, 0, 0, uFlags);
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

int iOriginalSettingsMusicEngineOutput;
char* szOriginalSettingsFluidSynthSoundfont;

BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	std::string strVersionInfo;

	char szFluidSynthSettingPath[MAX_PATH] = { 0 };
	OPENFILENAMEA stOFNFluidSynth = {
		sizeof(OPENFILENAMEA), hwndDlg, NULL,
		"SoundFont2 Files (*.sf2)\0*.sf2\0",
		NULL, NULL, NULL,
		szFluidSynthSettingPath, MAX_PATH - 1,
		NULL, NULL, NULL,
		"Select a FluidSynth SoundFont",
	};

	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		// Set the tab order for the dialog controls
		SetSettingsTabOrdering(hwndDlg);
		
		// Save some of the music engine settings for later
		iOriginalSettingsMusicEngineOutput = iSettingsMusicEngineOutput;
		szOriginalSettingsFluidSynthSoundfont = _strdup(szSettingsFluidSynthSoundfont);

		// Inject the music engine crap
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "None");			// MUSIC_ENGINE_NONE
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "Windows MIDI");	// MUSIC_ENGINE_SEQUENCER
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "FluidSynth");		// MUSIC_ENGINE_FLUIDSYNTH
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "MP3 Playback");	// MUSIC_ENGINE_MP3
		ComboBox_SetMinVisible(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), 4);

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
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT),
			"Selects the music output driver. Uses Windows MIDI as a fallback option.\n\n"
			""
			"None: Disables music playback independent of the per-game music option.\n"
			"Windows MIDI: Uses the native Windows MIDI sequencer.\n"
			"FluidSynth: Uses the FluidSynth software synth, if available.\n"
			"MP3 Playback: Uses MP3 files for playback, if available.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT),
			"FluidSynth requires a soundfont for playback that contains the samples and synthesis data required to play back MIDI files. Any SoundFont 2 standard soundfont\n\n"
			""
			"Selecting a new soundfont will reset the music engine and restart the currently playing song.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE),
			"Opens a file browser to select a soundfont for FluidSynth. Not needed for Windows MIDI or MP3 playback drivers.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC),
			"By default, SimCity 2000 stops the currently-playing song when the game window loses focus. This setting continues playing music in the background until the end of the track, "
			"after which a new song will be selected when the game window regains focus.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS),
			"Certain versions of SimCity 2000 had higher quality sounds than the Windows 95 versions. "
			"This setting controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC),
			"By default, SimCity 2000 selects \"random\" music by playing the next track in a looping playlist of songs. "
			"This setting controls whether or not to shuffle the playlist when the game starts and when the end of the playlist is reached.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE),
			"SimCity 2000 was designed to spend more CPU time on simulation than on rendering by only updating the city's growth when the display moves or on the 24th day of the month. "
			"Enabling this setting allows the game to refresh the city display in real-time instead of batching display updates.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC),
			"Enabling this setting will result in the next random music selection being played after the current song finishes.");

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
			"Enabling this setting will use the floating status dialog instead of the bottom status bar.");
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

		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), iSettingsMusicEngineOutput);

		{
			const char* szSoundFontBaseName = GetFileBaseName(szSettingsFluidSynthSoundfont);
			SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, szSoundFontBaseName);
			free((void*)szSoundFontBaseName);
		}

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), bSettingsMusicInBackground ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), bSettingsUseSoundReplacements ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), bSettingsShuffleMusic ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), bSettingsFrequentCityRefresh ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC), bSettingsAlwaysPlayMusic ? BST_CHECKED : BST_UNCHECKED);

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

	// Close without saving if the dialog is closed via the menu bar close button or Alt+F4
	case WM_CLOSE:
		free(szOriginalSettingsFluidSynthSoundfont);
		EndDialog(hwndDlg, wParam);
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case ID_SETTINGS_OK:
			// Grab settings from the dialog controls
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szSettingsMayorName, 63))
				strcpy_s(szSettingsMayorName, 64, "Marvin Maxis");
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szSettingsCompanyName, 63))
				strcpy_s(szSettingsCompanyName, 64, "Q37 Space Modulator Mfg.");

			iSettingsMusicEngineOutput = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT));

			bSettingsMusicInBackground = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC));
			bSettingsUseSoundReplacements = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS));
			bSettingsShuffleMusic = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC));
			bSettingsFrequentCityRefresh = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE));
			bSettingsAlwaysPlayMusic = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC));

			bSettingsAlwaysConsole = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE));
			bSettingsCheckForUpdates = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES));
			bSettingsDontLoadMods = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS));

			bSettingsUseStatusDialog = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG));
			bSettingsTitleCalendar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE));
			bSettingsUseNewStrings = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS));
			bSettingsAlwaysSkipIntro = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO));

			// Save the settings
			SaveSettings(FALSE);

			// See if we need to reset the music engine.
			if (dwMusicThreadID && (iSettingsMusicEngineOutput != iOriginalSettingsMusicEngineOutput ||
				strcmp(szOriginalSettingsFluidSynthSoundfont, szSettingsFluidSynthSoundfont)))
				PostThreadMessage(dwMusicThreadID, WM_MUSIC_RESET, NULL, NULL);

			free(szOriginalSettingsFluidSynthSoundfont);
			EndDialog(hwndDlg, wParam);
			break;
		case ID_SETTINGS_CANCEL:
			free(szOriginalSettingsFluidSynthSoundfont);
			EndDialog(hwndDlg, wParam);
			break;
		case ID_SETTINGS_DEFAULTS:
			// Set all the checkboxes to the defaults.
			ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), MUSIC_ENGINE_SEQUENCER);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC), BST_UNCHECKED);

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
			ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), MUSIC_ENGINE_SEQUENCER);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC), BST_UNCHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS), BST_CHECKED);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), BST_UNCHECKED);
			break;
		case IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE:
			if (GetOpenFileName(&stOFNFluidSynth)) {
				if (mus_debug & 8)
					ConsoleLog(LOG_DEBUG, "CORE: SoundFont setting changed; new soundfont is %s\n", szFluidSynthSettingPath);
				strncpy_s(szSettingsFluidSynthSoundfont, 261, szFluidSynthSettingPath, 260);

				{
					const char* szSoundFontBaseName = GetFileBaseName(szSettingsFluidSynthSoundfont);
					SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, szSoundFontBaseName);
					free((void*)szSoundFontBaseName);
				}
			}
			break;
		}
		return TRUE;
	}
	return FALSE;
}

void ShowSettingsDialog(void) {
	ToggleFloatingStatusDialog(FALSE);

	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS), GameGetRootWindowHandle(), SettingsDialogProc);

	ToggleFloatingStatusDialog(TRUE);
}