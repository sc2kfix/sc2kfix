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

HOOKEXT_CPP json::JSON jsonSettingsCore;
HOOKEXT_CPP json::JSON jsonSettingsMods;

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
	jsonSettings[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME] = DEF_SIM_REG_MAYOR_NAME;
	jsonSettings[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME] = DEF_SIM_REG_COMPANY_NAME;

	jsonSettings[C_SIMCITY2000][S_SIM_VER][I_SIM_VER_SC2K] = DEF_SIM_VER_PROGS;
	jsonSettings[C_SIMCITY2000][S_SIM_VER][I_SIM_VER_SCURK] = DEF_SIM_VER_PROGS;

	jsonSettings[C_SIMCITY2000][S_SIM_LOCALIZE][I_SIM_LOC_LANG] = DEF_SIM_LOC_LANGUAGE;

	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_DISASTERS] = DEF_SIM_OPT_DISASTERS;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_MUSIC] = DEF_SIM_OPT_MUSIC;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SOUND] = DEF_SIM_OPT_SOUND;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOGOTO] = DEF_SIM_OPT_AUTOGOTO;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOBUDGET] = DEF_SIM_OPT_AUTOBUDGET;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOSAVE] = DEF_SIM_OPT_AUTOSAVE;
	jsonSettings[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SPEED] = DEF_SIM_OPT_SPEED;

	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_CYCLECOLORS] = DEF_SIM_SCRK_CYCLECOLORS;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDHEIGHT] = DEF_SIM_SCRK_GRIDHEIGHT;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDWIDTH] = DEF_SIM_SCRK_GRIDWIDTH;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWCLIPREG] = DEF_SIM_SCRK_SHOWCLIPREG;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWDRAWGRID] = DEF_SIM_SCRK_SHOWDRAWGRID;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SNAPTOGRID] = DEF_SIM_SCRK_SNAPTOGRID;
	jsonSettings[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SOUND] = DEF_SIM_SCRK_SOUND;

	jsonSettings[C_SIMCITY2000][S_SIM_WIN][I_SIM_WIN_LASTCOLDEPTH] = DEF_SIM_WIN_LASTCOLDEPTH;
}

void DefaultSettingsSC2KFixCore(json::JSON& jsonSettings) {
	jsonSettings[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_INSTALLED] = DEF_FIX_CORE_INSTALLED;
	jsonSettings[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_FORCECON] = DEF_FIX_CORE_FORCECON;
	jsonSettings[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_CHECKFORUPD] = DEF_FIX_CORE_CHECKFORUPD;
	jsonSettings[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SKIPMODS] = DEF_FIX_CORE_SKIPMODS;

	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICINBKGRND] = DEF_FIX_AUD_MUSICINBKGRND;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE] = DEF_FIX_AUD_USESNDREPLACE;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC] = DEF_FIX_AUD_SHUFFLEMUSIC;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER] = DEF_FIX_AUD_MUSICDRIVER;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT] = DEF_FIX_AUD_SOUNDFONT;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC] = DEF_FIX_AUD_ALWAYSPLAYMUSIC;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MASTERVOLUME] = DEF_FIX_AUD_MASTERVOLUME;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICVOLUME] = DEF_FIX_AUD_MUSICVOLUME;
	jsonSettings[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDVOLUME] = DEF_FIX_AUD_SOUNDVOLUME;

	jsonSettings[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES] = std::string(szGamePath) + "\\" + DEF_SIM_PATHS_CITIES + "\\";
	jsonSettings[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS] = std::string(szGamePath) + "\\" + DEF_SIM_PATHS_TILESETS + "\\";

	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_FREQUPDATES] = DEF_FIX_QOL_FREQUPDATES;
	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_DARKUNDGRND] = DEF_FIX_QOL_DARKUNDGRND;
	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO] = DEF_FIX_QOL_SKIPINTRO;
	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS] = DEF_FIX_QOL_USENEWSTRINGS;
	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USEFLTSTATUS] = DEF_FIX_QOL_USEFLTSTATUS;
	jsonSettings[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND] = DEF_FIX_QOL_TITLECALEND;
	
	for (int i = 10000; i < 10019; i++) {
		jsonSettings[C_SC2KFIX][S_FIX_MUSMID][std::to_string(i)] = "";
		jsonSettings[C_SC2KFIX][S_FIX_MUSMP3][std::to_string(i)] = "";
	}
}

#define GETBOOLVAL(x) (x) ? true : false

void ConvertSettingsToJSON(void) {
	const char* szSettingsIniPath = GetIniPath();
	char szKeyBuf[32];
	char szSettingsMusicEngineOutput[32];
	char szLastStoredCityPath[MAX_PATH + 1];
	char szLastStoredTileSetPath[MAX_PATH + 1];

	// [Registration]
	const char* section = S_SIM_REG;
	GetPrivateProfileStringA(section, I_SIM_REG_MAYORNAME, "", szSettingsMayorName, sizeof(szSettingsMayorName) - 1, szSettingsIniPath);
	GetPrivateProfileStringA(section, I_SIM_REG_COMPANYNAME, "", szSettingsCompanyName, sizeof(szSettingsCompanyName) - 1, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME] = szSettingsMayorName;
	jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME] = szSettingsCompanyName;

	// [Options]
	section = S_SIM_OPTIONS;
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_DISASTERS] = GetPrivateProfileIntA(section, I_SIM_OPT_DISASTERS, DEF_SIM_OPT_DISASTERS, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_MUSIC] = GetPrivateProfileIntA(section, I_SIM_OPT_MUSIC, DEF_SIM_OPT_MUSIC, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SOUND] = GetPrivateProfileIntA(section, I_SIM_OPT_SOUND, DEF_SIM_OPT_SOUND, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOGOTO] = GetPrivateProfileIntA(section, I_SIM_OPT_AUTOGOTO, DEF_SIM_OPT_AUTOGOTO, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOBUDGET] = GetPrivateProfileIntA(section, I_SIM_OPT_AUTOBUDGET, DEF_SIM_OPT_AUTOBUDGET, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOSAVE] = GetPrivateProfileIntA(section, I_SIM_OPT_AUTOSAVE, DEF_SIM_OPT_AUTOSAVE, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SPEED] = GetPrivateProfileIntA(section, I_SIM_OPT_SPEED, DEF_SIM_OPT_SPEED, szSettingsIniPath);

	// [Options]
	section = S_SIM_SCURK;
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_CYCLECOLORS] = GetPrivateProfileIntA(section, I_SIM_SCRK_CYCLECOLORS, DEF_SIM_SCRK_CYCLECOLORS, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDHEIGHT] = GetPrivateProfileIntA(section, I_SIM_SCRK_GRIDHEIGHT, DEF_SIM_SCRK_GRIDHEIGHT, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDWIDTH] = GetPrivateProfileIntA(section, I_SIM_SCRK_GRIDWIDTH, DEF_SIM_SCRK_GRIDWIDTH, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWCLIPREG] = GetPrivateProfileIntA(section, I_SIM_SCRK_SHOWCLIPREG, DEF_SIM_SCRK_SHOWCLIPREG, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWDRAWGRID] = GetPrivateProfileIntA(section, I_SIM_SCRK_SHOWDRAWGRID, DEF_SIM_SCRK_SHOWDRAWGRID, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SNAPTOGRID] = GetPrivateProfileIntA(section, I_SIM_SCRK_SNAPTOGRID, DEF_SIM_SCRK_SNAPTOGRID, szSettingsIniPath);
	jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SOUND] = GetPrivateProfileIntA(section, I_SIM_SCRK_SOUND, DEF_SIM_SCRK_SOUND, szSettingsIniPath);
	
	// [sc2kfix]
	section = C_SC2KFIX;

	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICINBKGRND] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsMusicInBackground", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICINBKGRND].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsUseSoundReplacements", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsShuffleMusic", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsAlwaysPlayMusic", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC].ToBool(), szSettingsIniPath));

	// Core settings
	jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_INSTALLED] = true;
	jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_FORCECON] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsAlwaysConsole", jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_FORCECON].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_CHECKFORUPD] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsCheckForUpdates", jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_CHECKFORUPD].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SKIPMODS] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsDontLoadMods", jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SKIPMODS].ToBool(), szSettingsIniPath));

	// QOL settings
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_FREQUPDATES] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsFrequentCityRefresh", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_FREQUPDATES].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USEFLTSTATUS] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsUseStatusDialog", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USEFLTSTATUS].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsTitleCalendar", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsUseNewStrings", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsAlwaysSkipIntro", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO].ToBool(), szSettingsIniPath));
	jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_DARKUNDGRND] = GETBOOLVAL(GetPrivateProfileIntA(section, "bSettingsDarkUndergroundBkgnd", jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_DARKUNDGRND].ToBool(), szSettingsIniPath));

	// Audio settings
	memset(szSettingsMIDITrackPath, 0, sizeof(szSettingsMIDITrackPath));
	memset(szSettingsMP3TrackPath, 0, sizeof(szSettingsMP3TrackPath));

	GetPrivateProfileStringA(section, "szSettingsMusicEngineOutput", DEF_FIX_AUD_MUSICDRIVER, szSettingsMusicEngineOutput, 31, szSettingsIniPath);
	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER] = szSettingsMusicEngineOutput;

	GetPrivateProfileStringA(section, "szSettingsFluidSynthSoundfont", DEF_FIX_AUD_SOUNDFONT, szSettingsFluidSynthSoundfont, MAX_PATH, szSettingsIniPath);
	jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT] = szSettingsFluidSynthSoundfont;

	section = "sc2kfix.music.MIDI";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMIDITrackPath[i], sizeof(szSettingsMIDITrackPath[i]) - 1, szSettingsIniPath);
		StrTrimA(szSettingsMIDITrackPath[i], " \t\r\n");
		jsonSettingsCore[C_SC2KFIX][S_FIX_MUSMID][std::string(szKeyBuf)] = szSettingsMIDITrackPath[i];
	}

	section = "sc2kfix.music.MP3";
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		sprintf_s(szKeyBuf, sizeof(szKeyBuf), "100%02d", i);
		GetPrivateProfileStringA(section, szKeyBuf, "", szSettingsMP3TrackPath[i], sizeof(szSettingsMP3TrackPath[i]) - 1, szSettingsIniPath);
		StrTrimA(szSettingsMP3TrackPath[i], " \t\r\n");
		jsonSettingsCore[C_SC2KFIX][S_FIX_MUSMP3][std::string(szKeyBuf)] = szSettingsMP3TrackPath[i];
	}

	// Keybinding settings
	section = "sc2kfix.Bindings";
	LoadLegacyStoredBindings(jsonSettingsCore, section, szSettingsIniPath);

	// Last Stored Paths
	section = "LastAccessedPaths";
	GetPrivateProfileStringA(section, "szLastStoredCityPath", "", szLastStoredCityPath, sizeof(szLastStoredCityPath) - 1, szSettingsIniPath);
	jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES] = szLastStoredCityPath;
	GetPrivateProfileStringA(section, "szLastStoredTileSetPath", "", szLastStoredTileSetPath, sizeof(szLastStoredTileSetPath) - 1, szSettingsIniPath);
	jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS] = szLastStoredTileSetPath;

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
	LoadJSONBindings(jsonSettingsCore);
}

void SaveJSONSettings(void) {
	jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SETSAVETIME] = std::to_string(time(NULL));
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

#define GET_CHECKBOX(dest, src) dest = (Button_GetCheck(GetDlgItem(hwndDlg, src)) == BST_CHECKED) ? true : false
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

		DestroyStoredTooltips(storedToolTips, hwndDlg);

		// Create tooltips.
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_OK),
			"Saves the currently selected settings and closes the settings dialog.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_CANCEL),
			"Discards changed settings and closes the settings dialog.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_DEFAULTS),
			"Changes the settings to the default sc2kfix experience but does not save settings or close the dialog.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, ID_SETTINGS_VANILLA),
			"Changes the settings to disable all quality of life, interface and gameplay enhancements but does not save settings or close the dialog.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS),
			"Resets the file association entries in the registry so that .sc2 and .scn files will automatically open in SimCity 2000.");

		// QoL/Performance settings
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT),
			"Selects the music output driver. Uses Windows MIDI as a fallback option.\n\n"
			""
			"None: Disables music playback independent of the per-game music option.\n"
			"Windows MIDI: Uses the native Windows MIDI sequencer.\n"
			"FluidSynth: Uses the FluidSynth software synth, if available (default).\n"
			"MP3 Playback: Uses MP3 files for playback, if available.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT),
			"FluidSynth requires a soundfont for playback that contains the samples and synthesis data required to play back MIDI files. Any SoundFont 2 standard soundfont\n\n"
			""
			"Selecting a new soundfont will reset the music engine and restart the currently playing song.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE),
			"Opens a file browser to select a soundfont for FluidSynth. Not needed for Windows MIDI or MP3 playback drivers.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC),
			"By default, SimCity 2000 stops the currently-playing song when the game window loses focus. This setting continues playing music in the background until the end of the track, "
			"after which a new song will be selected when the game window regains focus.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS),
			"Certain versions of SimCity 2000 had higher quality sounds than the Windows 95 versions. "
			"This setting controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC),
			"By default, SimCity 2000 selects \"random\" music by playing the next track in a looping playlist of songs. "
			"This setting controls whether or not to shuffle the playlist when the game starts and when the end of the playlist is reached.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE),
			"SimCity 2000 was designed to spend more CPU time on simulation than on rendering by only updating the city's growth when the display moves or on the 24th day of the month. "
			"Enabling this setting allows the game to refresh the city display in real-time instead of batching display updates.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC),
			"Enabling this setting will result in the next random music selection being played after the current song finishes.");

		// sc2kfix core settings
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE),
			"sc2kfix has a debugging console that can be activated by passing the -console argument to SimCity 2000's command line. "
			"This setting forces sc2kfix to always start the console along with the game, even if the -console argument is not passed.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES),
			"This setting checks to see if there's a newer release of sc2kfix available when the game starts.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS),
			"Enabling this setting forces sc2kfix to skip loading any installed mods on startup.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");

		// Interface settings
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG),
			"The DOS and Mac versions of SimCity 2000 used a movable floating dialog to show the current tool, status line, and weather instead of a fixed bar at the bottom of the game window. "
			"Enabling this setting will use the floating status dialog instead of the bottom status bar.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE),
			"By default the title bar only displays the month and year. Enabling this setting will display the full in-game date instead.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS),
			"Certain strings in the game have typos, grammatical issues, and/or ambiguous wording. This setting loads corrected strings in memory in place of the affected originals.");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO),
			"Once enabled the introduction videos will be skipped on startup (This will only apply if the videos have been detected, otherwise the standard warning will be displayed).");
		StoreTooltip(storedToolTips, hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND),
			"When enabled the underground layer background will be dark.");

		// Set the version string.
		strVersionInfo = "sc2kfix ";
		strVersionInfo += szSC2KFixVersion;
		strVersionInfo += " (";
		strVersionInfo += szSC2KFixReleaseTag;
		strVersionInfo += ")";
		SetDlgItemText(hwndDlg, IDC_STATIC_VERSIONINFO, strVersionInfo.c_str());

		// Load the existing settings into the dialog
		SetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, jsonSettingsCoreWorkingCopy[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME].ToString().c_str());
		SetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, jsonSettingsCoreWorkingCopy[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME].ToString().c_str());

		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), MusicEngineStringToInt(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString().c_str()));

		{
			const char* szSoundFontBaseName = GetFileBaseName(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString().c_str());
			SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, szSoundFontBaseName);
			free((void*)szSoundFontBaseName);
		}

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICINBKGRND], IDC_SETTINGS_CHECK_BKGDMUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_FREQUPDATES], IDC_SETTINGS_CHECK_REFRESH_RATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_FORCECON], IDC_SETTINGS_CHECK_CONSOLE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_CHECKFORUPD], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SKIPMODS], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USEFLTSTATUS], IDC_SETTINGS_CHECK_STATUS_DIALOG);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND], IDC_SETTINGS_CHECK_TITLE_DATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS], IDC_SETTINGS_CHECK_NEW_STRINGS);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO], IDC_SETTINGS_CHECK_SKIP_INTRO);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_DARKUNDGRND], IDC_SETTINGS_CHECK_DARK_UNDGRND);

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
			if (GetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szTempRegistrationNameBuffer, 63))
				jsonSettingsCoreWorkingCopy[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME] = szTempRegistrationNameBuffer;
			if (GetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szTempRegistrationNameBuffer, 63))
				jsonSettingsCoreWorkingCopy[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME] = szTempRegistrationNameBuffer;

			jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER] = MusicEngineIntToString(ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT)));

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICINBKGRND], IDC_SETTINGS_CHECK_BKGDMUSIC);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_FREQUPDATES], IDC_SETTINGS_CHECK_REFRESH_RATE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_FORCECON], IDC_SETTINGS_CHECK_CONSOLE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_CHECKFORUPD], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_SKIPMODS], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);

			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USEFLTSTATUS], IDC_SETTINGS_CHECK_STATUS_DIALOG);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_TITLECALEND], IDC_SETTINGS_CHECK_TITLE_DATE);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS], IDC_SETTINGS_CHECK_NEW_STRINGS);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_SKIPINTRO], IDC_SETTINGS_CHECK_SKIP_INTRO);
			GET_CHECKBOX(jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_DARKUNDGRND], IDC_SETTINGS_CHECK_DARK_UNDGRND);

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

				jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT] = szFluidSynthSettingPath;

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

	case WM_DESTROY:
		DestroyStoredTooltips(storedToolTips, hwndDlg);

		break;
	}
	return FALSE;
}

void ShowSettingsDialog(void) {
	settings_t st;

	// Copy the active settings structure into the working copy
	jsonSettingsCoreWorkingCopy = jsonSettingsCore;

	// Save the original settings here just prior to saving.
	std::string strOriginalMusicDriver = jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString();
	std::string strOriginalSoundfont = jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString();

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
		if (dwMusicThreadID && (strOriginalMusicDriver != jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() ||
			strOriginalSoundfont != jsonSettingsCoreWorkingCopy[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString()) ||
			(st.bActiveMusicDriverTouched && st.bActiveTrackChanged))
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_RESET, NULL, NULL);
	}

	ToggleFloatingStatusDialog(TRUE);
}
