// sc2kfix modules/settings.cpp: settings dialog code and configurator
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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
#include <keybindings.h>
#include "../resource.h"

static DWORD dwDummy;

json::JSON jsonSettingsCore;
json::JSON jsonSettingsMods;

char szGamePath[MAX_PATH];

// No longer actually used for settings, but as temporary buffers
char szSettingsMayorName[64];
char szSettingsCompanyName[64];
char szSettingsMIDITrackPath[MUSIC_TRACKS][MAX_PATH + 1];
char szSettingsMP3TrackPath[MUSIC_TRACKS][MAX_PATH + 1];
static char szSettingsFluidSynthSoundfont[MAX_PATH + 1];

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

const char* GetSettingsJsonPath() {
	static char szJsonPath[MAX_PATH];

	sprintf_s(szJsonPath, MAX_PATH, "%s\\%s", szGamePath, SC2KFIX_COREJSON);
	return szJsonPath;
}

void DefaultSettingsSC2K(json::JSON& jsonSettings) {
	jsonSettings["SimCity2000"]["Registration"]["Mayor Name"] = "Marvin Maxis";
	jsonSettings["SimCity2000"]["Registration"]["Company Name"] = "Q37 Space Modulator Mfg.";

	jsonSettings["SimCity2000"]["Version"]["SCURK"] = 256;
	jsonSettings["SimCity2000"]["Version"]["SimCity 2000"] = 256;

	jsonSettings["SimCity2000"]["Localize"]["Language"] = "USA";

	jsonSettings["SimCity2000"]["Options"]["Disasters"] = 1;
	jsonSettings["SimCity2000"]["Options"]["Music"] = 1;
	jsonSettings["SimCity2000"]["Options"]["Sound"] = 1;
	jsonSettings["SimCity2000"]["Options"]["AutoGoto"] = 1;
	jsonSettings["SimCity2000"]["Options"]["AutoBudget"] = 0;
	jsonSettings["SimCity2000"]["Options"]["AutoSave"] = 0;
	jsonSettings["SimCity2000"]["Options"]["Speed"] = INI_GAME_SPEED_SETTING(GAME_SPEED_LLAMA);

	jsonSettings["SimCity2000"]["SCURK"]["CycleColors"] = 1;
	jsonSettings["SimCity2000"]["SCURK"]["GridHeight"] = 2;
	jsonSettings["SimCity2000"]["SCURK"]["GridWidth"] = 2;
	jsonSettings["SimCity2000"]["SCURK"]["ShowClipRegion"] = 0;
	jsonSettings["SimCity2000"]["SCURK"]["ShowDrawGrid"] = 0;
	jsonSettings["SimCity2000"]["SCURK"]["SnapToGrid"] = 0;
	jsonSettings["SimCity2000"]["SCURK"]["Sound"] = 1;

	jsonSettings["SimCity2000"]["Windows"]["Last Color Depth"] = 32;
}

void DefaultSettingsSC2KFixCore(json::JSON& jsonSettings) {
	jsonSettings["sc2kfix"]["core"]["installed"] = false;
	jsonSettings["sc2kfix"]["core"]["force_console"] = false;
	jsonSettings["sc2kfix"]["core"]["check_for_updates"] = true;
	jsonSettings["sc2kfix"]["core"]["skip_mods"] = false;

	jsonSettings["sc2kfix"]["audio"]["music_in_background"] = true;
	jsonSettings["sc2kfix"]["audio"]["use_sound_replacements"] = true;
	jsonSettings["sc2kfix"]["audio"]["shuffle_music"] = false;
	jsonSettings["sc2kfix"]["audio"]["music_driver"] = "fluidsynth";
	jsonSettings["sc2kfix"]["audio"]["soundfont"] = "C:\\Windows\\System32\\drivers\\gm.dls";
	jsonSettings["sc2kfix"]["audio"]["always_play_music"] = false;
	jsonSettings["sc2kfix"]["audio"]["master_volume"] = 1.0;
	jsonSettings["sc2kfix"]["audio"]["music_volume"] = 1.0;
	jsonSettings["sc2kfix"]["audio"]["sound_volume"] = 1.0;

	jsonSettings["sc2kfix"]["paths"]["cities"] = std::string(szGamePath) + "\\CITIES\\";
	jsonSettings["sc2kfix"]["paths"]["tilesets"] = std::string(szGamePath) + "\\SCURKART\\";

	jsonSettings["sc2kfix"]["qol"]["frequent_updates"] = true;
	jsonSettings["sc2kfix"]["qol"]["dark_underground"] = false;
	jsonSettings["sc2kfix"]["qol"]["skip_intro"] = false;
	jsonSettings["sc2kfix"]["qol"]["use_new_strings"] = true;
	jsonSettings["sc2kfix"]["qol"]["use_floating_status"] = false;
	jsonSettings["sc2kfix"]["qol"]["title_calendar"] = true;
	
	for (int i = 10000; i <= 10019; i++) {
		jsonSettings["sc2kfix"]["music_midi"][std::to_string(i)] = "";
		jsonSettings["sc2kfix"]["music_mp3"][std::to_string(i)] = "";
	}
}

void ConvertSettingsToJSON(void) {
	const char* szSettingsIniPath = GetIniPath();
	char szKeyBuf[32];
	char szSettingsMusicEngineOutput[32];

	// [Registration]
	const char* section = "Registration";
	GetPrivateProfileStringA(section, "Mayor Name", "", szSettingsMayorName, sizeof(szSettingsMayorName) - 1, szSettingsIniPath);
	GetPrivateProfileStringA(section, "Company Name", "", szSettingsCompanyName, sizeof(szSettingsCompanyName) - 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Registration"]["Mayor Name"] = szSettingsMayorName;
	jsonSettingsCore["SimCity2000"]["Registration"]["Company Name"] = szSettingsCompanyName;

	// [Options]
	section = "Options";
	jsonSettingsCore["SimCity2000"]["Options"]["Disasters"] = GetPrivateProfileIntA(section, "Disasters", 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["Music"] = GetPrivateProfileIntA(section, "Music", 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["Sound"] = GetPrivateProfileIntA(section, "Sound", 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["AutoGoto"] = GetPrivateProfileIntA(section, "AutoGoto", 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["AutoBudget"] = GetPrivateProfileIntA(section, "AutoBudget", 0, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["AutoSave"] = GetPrivateProfileIntA(section, "AutoSave", 0, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["Options"]["Speed"] = GetPrivateProfileIntA(section, "Speed", INI_GAME_SPEED_SETTING(GAME_SPEED_LLAMA), szSettingsIniPath);

	// [Options]
	section = "SCURK";
	jsonSettingsCore["SimCity2000"]["SCURK"]["CycleColors"] = GetPrivateProfileIntA(section, "CycleColors", 1, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["GridHeight"] = GetPrivateProfileIntA(section, "GridHeight", 2, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["GridWidth"] = GetPrivateProfileIntA(section, "GridWidth", 2, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["ShowClipRegion"] = GetPrivateProfileIntA(section, "ShowClipRegion", 0, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["ShowDrawGrid"] = GetPrivateProfileIntA(section, "ShowDrawGrid", 0, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["SnapToGrid"] = GetPrivateProfileIntA(section, "SnapToGrid", 0, szSettingsIniPath);
	jsonSettingsCore["SimCity2000"]["SCURK"]["Sound"] = GetPrivateProfileIntA(section, "Sound", 1, szSettingsIniPath);
	
	// [sc2kfix]
	section = "sc2kfix";

	jsonSettingsCore["sc2kfix"]["audio"]["music_in_background"] = (bool)GetPrivateProfileIntA(section, "bSettingsMusicInBackground", jsonSettingsCore["sc2kfix"]["audio"]["music_in_background"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["audio"]["use_sound_replacements"] = (bool)GetPrivateProfileIntA(section, "bSettingsUseSoundReplacements", jsonSettingsCore["sc2kfix"]["audio"]["use_sound_replacements"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["audio"]["shuffle_music"] = (bool)GetPrivateProfileIntA(section, "bSettingsShuffleMusic", jsonSettingsCore["sc2kfix"]["audio"]["shuffle_music"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["audio"]["always_play_music"] = (bool)GetPrivateProfileIntA(section, "bSettingsAlwaysPlayMusic", jsonSettingsCore["sc2kfix"]["audio"]["always_play_music"].ToBool(), szSettingsIniPath);

	// Core settings
	jsonSettingsCore["sc2kfix"]["core"]["installed"] = true;
	jsonSettingsCore["sc2kfix"]["core"]["force_console"] = (bool)GetPrivateProfileIntA(section, "bSettingsAlwaysConsole", jsonSettingsCore["sc2kfix"]["core"]["force_console"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["core"]["check_for_updates"] = (bool)GetPrivateProfileIntA(section, "bSettingsCheckForUpdates", jsonSettingsCore["sc2kfix"]["core"]["check_for_updates"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["core"]["skip_mods"] = (bool)GetPrivateProfileIntA(section, "bSettingsDontLoadMods", jsonSettingsCore["sc2kfix"]["core"]["skip_mods"].ToBool(), szSettingsIniPath);

	// QOL settings
	jsonSettingsCore["sc2kfix"]["qol"]["frequent_updates"] = (bool)GetPrivateProfileIntA(section, "bSettingsFrequentCityRefresh", jsonSettingsCore["sc2kfix"]["qol"]["frequent_updates"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["qol"]["use_floating_status"] = (bool)GetPrivateProfileIntA(section, "bSettingsUseStatusDialog", jsonSettingsCore["sc2kfix"]["qol"]["use_floating_status"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["qol"]["title_calendar"] = (bool)GetPrivateProfileIntA(section, "bSettingsTitleCalendar", jsonSettingsCore["sc2kfix"]["qol"]["title_calendar"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["qol"]["use_new_strings"] = (bool)GetPrivateProfileIntA(section, "bSettingsUseNewStrings", jsonSettingsCore["sc2kfix"]["qol"]["use_new_strings"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["qol"]["skip_intro"] = (bool)GetPrivateProfileIntA(section, "bSettingsAlwaysSkipIntro", jsonSettingsCore["sc2kfix"]["qol"]["skip_intro"].ToBool(), szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["qol"]["dark_underground"] = (bool)GetPrivateProfileIntA(section, "bSettingsDarkUndergroundBkgnd", jsonSettingsCore["sc2kfix"]["qol"]["dark_underground"].ToBool(), szSettingsIniPath);

	// Audio settings
	memset(szSettingsMIDITrackPath, 0, sizeof(szSettingsMIDITrackPath));
	memset(szSettingsMP3TrackPath, 0, sizeof(szSettingsMP3TrackPath));

	GetPrivateProfileStringA(section, "szSettingsMusicEngineOutput", "fluidsynth", szSettingsMusicEngineOutput, 31, szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["audio"]["music_driver"] = szSettingsMusicEngineOutput;

	GetPrivateProfileStringA(section, "szSettingsFluidSynthSoundfont", "C:\\Windows\\System32\\drivers\\gm.dls", szSettingsFluidSynthSoundfont, MAX_PATH, szSettingsIniPath);
	jsonSettingsCore["sc2kfix"]["audio"]["soundfont"] = szSettingsFluidSynthSoundfont;

	section = "sc2kfix.music.MIDI";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMIDITrackPath[i], sizeof(szSettingsMIDITrackPath[i]) - 1, szSettingsIniPath);
		StrTrimA(szSettingsMIDITrackPath[i], " \t\r\n");
		jsonSettingsCore["sc2kfix"]["music_midi"][std::string(szKeyBuf)] = szSettingsMIDITrackPath[i];
	}

	section = "sc2kfix.music.MP3";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMP3TrackPath[i], sizeof(szSettingsMP3TrackPath[i]) - 1, szSettingsIniPath);
		StrTrimA(szSettingsMP3TrackPath[i], " \t\r\n");
		jsonSettingsCore["sc2kfix"]["music_mp3"][std::string(szKeyBuf)] = szSettingsMP3TrackPath[i];
	}

	// Rename the old INI, save the JSON, and print a notice in the console
	MoveFile(szSettingsIniPath, (std::string(szSettingsIniPath) + ".old").c_str());
	SaveJSONSettings();
	ConsoleLog(LOG_INFO, "CORE: Converted existing " SC2KFIX_INIFILE " to " SC2KFIX_COREJSON ".\n");
}

void InitializeJSONSettings(void) {
	// Initialize JSON objects
	jsonSettingsCore = json::Object();
	jsonSettingsMods = json::Object();

	// Set up original SC2K settings
	DefaultSettingsSC2K(jsonSettingsCore);

	// Set up default sc2kfix settings
	DefaultSettingsSC2KFixCore(jsonSettingsCore);

	// Copy over default bindings into JSON
	SaveJSONBindings(jsonSettingsCore);

	// Convert existing settings INI if needed
	if (!FileExists(GetSettingsJsonPath()) && FileExists(GetIniPath()))
		ConvertSettingsToJSON();
}

void LoadJSONSettings(void) {
	// XXX: this is a bit inefficient; maybe do a std::string with a pre-allocation using
	// std::filesystem::file_size() instead?
	std::ifstream fSettingsJSON(GetSettingsJsonPath());
	std::stringstream strLoadedJSONDump;
	strLoadedJSONDump << fSettingsJSON.rdbuf();
	jsonSettingsCore.merge(jsonSettingsCore.Load(strLoadedJSONDump.str()));
}

void SaveJSONSettings(void) {
	jsonSettingsCore["sc2kfix"]["core"]["settings_save_time"] = std::to_string(time(NULL));
	std::ofstream fSettingsJSON(GetSettingsJsonPath(), std::ios::out | std::ios::trunc);
	fSettingsJSON << jsonSettingsCore.dump();

	if (dwDetectedVersion == VERSION_SC2K_1996) {
		UpdateMiscHooks_SC2K1996();
		UpdateStatus_SC2K1996(-1);
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
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG), NULL, 0, 0, 0, 0, uFlags);

	// Quality of Life / Performance Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC), NULL, 0, 0, 0, 0, uFlags);

	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_CONFKEYBINDINGS), NULL, 0, 0, 0, 0, uFlags);

	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_CONFMP3TRACKS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_CONFMIDTRACKS), NULL, 0, 0, 0, 0, uFlags);

	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), NULL, 0, 0, 0, 0, uFlags);

	// Game Settings
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_COMPANY), NULL, 0, 0, 0, 0, uFlags);
	SetWindowPos(GetDlgItem(hwndDlg, IDC_SETTINGS_MAYOR), NULL, 0, 0, 0, 0, uFlags);
}

json::JSON jsonSettingsCoreWorkingCopy;

#define GET_CHECKBOX(dest, src) dest = (bool)Button_GetCheck(GetDlgItem(hwndDlg, src))
#define SET_CHECKBOX(src, dest) Button_SetCheck(GetDlgItem(hwndDlg, dest), src.ToBool() ? BST_CHECKED : BST_UNCHECKED)

BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	std::string strVersionInfo;
	settings_t *st;
	char szTempRegistrationNameBuffer[64] = { 0 };

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
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		st = (settings_t *)lParam;
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		// Set the tab order for the dialog controls
		SetSettingsTabOrdering(hwndDlg);

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
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS),
			"Resets the file association entries in the registry so that .sc2 and .scn files will automatically open in SimCity 2000.");

		// QoL/Performance settings
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT),
			"Selects the music output driver. Uses Windows MIDI as a fallback option.\n\n"
			""
			"None: Disables music playback independent of the per-game music option.\n"
			"Windows MIDI: Uses the native Windows MIDI sequencer.\n"
			"FluidSynth: Uses the FluidSynth software synth, if available (default).\n"
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
			"This setting controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
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
			"This setting checks to see if there's a newer release of sc2kfix available when the game starts.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
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
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND),
			"When enabled the underground layer background will be dark.");

		// Set the version string.
		strVersionInfo = "sc2kfix ";
		strVersionInfo += szSC2KFixVersion;
		strVersionInfo += " (";
		strVersionInfo += szSC2KFixReleaseTag;
		strVersionInfo += ")";
		SetDlgItemText(hwndDlg, IDC_STATIC_VERSIONINFO, strVersionInfo.c_str());

		// Load the existing settings into the dialog
		SetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Mayor Name"].ToString().c_str());
		SetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Company Name"].ToString().c_str());

		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), MusicEngineStringToInt(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"].ToString().c_str()));

		{
			const char* szSoundFontBaseName = GetFileBaseName(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"].ToString().c_str());
			SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, szSoundFontBaseName);
			free((void*)szSoundFontBaseName);
		}

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"], IDC_SETTINGS_CHECK_BKGDMUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"], IDC_SETTINGS_CHECK_REFRESH_RATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"], IDC_SETTINGS_CHECK_CONSOLE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["floating_status"], IDC_SETTINGS_CHECK_STATUS_DIALOG);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"], IDC_SETTINGS_CHECK_TITLE_DATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"], IDC_SETTINGS_CHECK_NEW_STRINGS);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"], IDC_SETTINGS_CHECK_SKIP_INTRO);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_background"], IDC_SETTINGS_CHECK_DARK_UNDGRND);

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	// Close without saving if the dialog is closed via the menu bar close button or Alt+F4
	case WM_CLOSE:
		EndDialog(hwndDlg, FALSE);
		break;

	case WM_COMMAND:
		st = (settings_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case ID_SETTINGS_OK:
			// Grab settings from the dialog controls
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szTempRegistrationNameBuffer, 63))
				jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Mayor Name"] = szTempRegistrationNameBuffer;
			if (!GetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szTempRegistrationNameBuffer, 63))
				jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Company Name"] = szTempRegistrationNameBuffer;

			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"] = MusicEngineIntToString(ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT)));

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"], IDC_SETTINGS_CHECK_BKGDMUSIC);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"], IDC_SETTINGS_CHECK_REFRESH_RATE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"], IDC_SETTINGS_CHECK_CONSOLE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["floating_status"], IDC_SETTINGS_CHECK_STATUS_DIALOG);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"], IDC_SETTINGS_CHECK_TITLE_DATE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"], IDC_SETTINGS_CHECK_NEW_STRINGS);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"], IDC_SETTINGS_CHECK_SKIP_INTRO);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_background"], IDC_SETTINGS_CHECK_DARK_UNDGRND);

			EndDialog(hwndDlg, TRUE);
			break;
		case ID_SETTINGS_CANCEL:
			EndDialog(hwndDlg, FALSE);
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
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND), BST_UNCHECKED);
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
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND), BST_UNCHECKED);
			break;
		case IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS:
			ResetFileAssociations();
			MessageBox(hwndDlg, ".sc2 and .scn file associations reset!", "It Works!", MB_OK | MB_ICONINFORMATION);
			break;
		case IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE:
			if (GetOpenFileName(&stOFNFluidSynth)) {
				if (mus_debug & 8)
					ConsoleLog(LOG_DEBUG, "CORE: SoundFont setting changed; new soundfont is %s\n", szFluidSynthSettingPath);

				jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"] = szFluidSynthSettingPath;

				{
					const char* szSoundFontBaseName = GetFileBaseName(szFluidSynthSettingPath);
					SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, szSoundFontBaseName);
					free((void*)szSoundFontBaseName);
				}
			}
			break;
		case IDC_SETTINGS_BUTTON_CONFMIDTRACKS:
			return DoConfigureMusicTracks(st, hwndDlg, FALSE);
		case IDC_SETTINGS_BUTTON_CONFMP3TRACKS:
			return DoConfigureMusicTracks(st, hwndDlg, TRUE);
		case IDC_SETTINGS_BUTTON_CONFKEYBINDINGS:
			return DoConfigureKeyBindings(st, hwndDlg);
		}
		return TRUE;
	}
	return FALSE;
}

void ShowSettingsDialog(void) {
	settings_t st;

	// Copy the active settings structure into the working copy
	jsonSettingsCoreWorkingCopy = jsonSettingsCore;

	// Save the original settings here just prior to saving.
	std::string strOriginalMusicDriver = jsonSettingsCore["sc2kfix"]["audio"]["music_driver"].ToString();
	std::string strOriginalSoundfont = jsonSettingsCore["sc2kfix"]["audio"]["soundfont"].ToString();

	InitializeTempBindings();

	ToggleFloatingStatusDialog(FALSE);

	if (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS), GameGetRootWindowHandle(), SettingsDialogProc, (LPARAM)&st) == TRUE) {
		// Copy back all our settings into the active structure
		jsonSettingsCore = jsonSettingsCoreWorkingCopy;

		// Update any keybindings if needed
		if (st.bKeyBindingsChanged) {
			UpdateKeyBindings();
			SaveJSONBindings(jsonSettingsCore);
		}

		// Save the settings JSON file and update hooks
		SaveJSONSettings();

		// See if we need to reset the music engine.
		if (dwMusicThreadID && (strOriginalMusicDriver != jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"].ToString() ||
			strOriginalSoundfont != jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"].ToString()) ||
			(st.bActiveMusicDriverTouched && st.bActiveTrackChanged))
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_RESET, NULL, NULL);
	}

	ToggleFloatingStatusDialog(TRUE);
}
