// sc2kfix include/sc2kfix.h: globals that need to be used elsewhere
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once
#pragma warning(disable : 4200)
#pragma warning(disable : 4733)

#include <windows.h>
#include <windowsx.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <random>

#include <mfc3xhelp.h>
#include <sc2kclasses.h>
#include <sc2k_1995.h>
#include <sc2k_1996.h>
#include <sc2k_demo.h>
#include <bc45xhelp.h>
#include <scurkclasses.h>
#include <scurk_1996.h>
#include <scurk_primary.h>
#include <music.h>

// Turning this on enables every debugging option. You have been warned.
// #define DEBUGALL

// Turning this on forces the console to be enabled, as if -console was passed to SIMCITY.EXE.
// #define CONSOLE_ENABLED

#define SC2KFIX_CORE

#define VERSION_PROG_UNKNOWN  0
#define VERSION_SC2K_1996     1
#define VERSION_SC2K_1995     2
#define VERSION_SC2K_DEMO     3
#define VERSION_SCURK_PRIMARY 4
#define VERSION_SCURK_1996    5

#define SC2KFIX_MODE_UNKNOWN  0
#define SC2KFIX_MODE_SC2K     1
#define SC2KFIX_MODE_SC2KDEMO 2
#define SC2KFIX_MODE_SCURK    3

#define SC2KFIX_VERSION			"0.10a-dev"
#define SC2KFIX_VERSION_MAJOR	0
#define SC2KFIX_VERSION_MINOR	10
#define SC2KFIX_VERSION_PATCH	1
#define SC2KFIX_RELEASE_TAG		"r10"

#define SC2KFIX_INIFILE		"sc2kfix.ini"
#define SC2KFIX_MODSFOLDER	"mods"

#define HOOKEXT extern "C" __declspec(dllexport)
#define HOOKEXT_CPP __declspec(dllexport)

#define WM_SC2KFIX_UPDATE 37241

#define UPDATE_STRING "A new version of sc2kfix is available for download from the GitHub releases page."

#include <json.hpp>

#ifdef __cplusplus
#include <sstream>
template <typename T> std::string to_string_precision(const T value, const int prec) {
	std::ostringstream out;
	out.precision(prec);
	out << std::fixed << value;
	return std::move(out).str();
}
#endif

#define countof(x) (sizeof(x)/sizeof(*(x)))
#define lengthof(s) (countof(s)-1)

#define SIZE_OFFSETOF(sz, ty, el) ((sz)&(((ty *)0)->el))

#define DWORD_OFFSETOF(ty, el) SIZE_OFFSETOF(DWORD, ty, el)
#define WORD_OFFSETOF(ty, el)  SIZE_OFFSETOF(WORD, ty, el)
#define BYTE_OFFSETOF(ty, el)  SIZE_OFFSETOF(BYTE, ty, el)

#define IFF_HEAD(a, b, c, d) ((DWORD)d << 24 | (DWORD)c << 16 | (DWORD)b << 8 | (DWORD)a)
#define DWORD_NTOHL_CHECK(x) (bBigEndian ? ntohl(x) : x)
#define DWORD_HTONL_CHECK(x) (bBigEndian ? htonl(x) : x)

#define RELATIVE_OFFSET(from, to) *(DWORD*)((DWORD)(from)) = (DWORD)(to) - (DWORD)(from) - 4;
#define NEWCALL(from, to) *(BYTE*)(from) = 0xE8; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJMP(from, to) *(BYTE*)(from) = 0xE9; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x82; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x83; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x84; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x85; RELATIVE_OFFSET((DWORD)(from)+2, to)

#define DEBUG_FLAGS_NONE		0
#define DEBUG_FLAGS_EVERYTHING	0xFFFFFFFF

#if !NOKUROKO
#define WM_KUROKO_REPL	WM_APP+0x100
#define WM_KUROKO_FILE	WM_APP+0x101
#define WM_CONSOLE_REPL	WM_APP+0x200
#endif

#define MUSIC_TRACKS 19
#define SOUND_ENTRIES 31

// It should be noted that with these values
// they're referencing the min/max for the
// user and sim label entries but NOT the total
// maximum for all which is 256 (0 - 255).
// Index 0 is used for the city-base mayor name.
#define MIN_USER_TEXT_ENTRIES 1
#define MAX_USER_TEXT_ENTRIES 50
#define MIN_SIM_TEXT_ENTRIES (MAX_USER_TEXT_ENTRIES + 1)
#define MAX_SIM_TEXT_ENTRIES 200

#define MAX_LABEL_TEXT_ENTRY_RANGE 128

#define MICROSIMID_MIN 0
#define MICROSIMID_MAX (MAX_SIM_TEXT_ENTRIES - MIN_SIM_TEXT_ENTRIES)
#define MICROSIMID_ENTRY(x) (x - MIN_SIM_TEXT_ENTRIES)

// These "appear" to be related to XTHG cases
// based on the named sailboat case.
#define MIN_XTHG_TEXT_ENTRIES (MAX_SIM_TEXT_ENTRIES + 1)
#define MAX_XTHG_TEXT_ENTRIES 240
#define XTHGID_ENTRY(x) (x - MIN_XTHG_TEXT_ENTRIES)

#define MIN_PLEASECHECK_TEXT_ENTRIES (MAX_XTHG_TEXT_ENTRIES + 1)
#define MAX_PLEASECHECK_TEXT_ENTRIES 249
#define PLEASECHECKID_ENTRY(x) (x - MIN_PLEASECHECK_TEXT_ENTRIES)

#define NGHBR_CONNECTION_TEXT_ENTRY (MAX_PLEASECHECK_TEXT_ENTRIES + 1)
#define NGHBRCONNECTID_ENTRY(x) (x - NGHBR_CONNECTION_TEXT_ENTRY)

#define MIN_DISASTER_TEXT_ENTRIES (NGHBR_CONNECTION_TEXT_ENTRY + 1)
#define MAX_DISASTER_TEXT_ENTRIES 255
#define DISASTERID_ENTRY(x) (x - MIN_DISASTER_TEXT_ENTRIES)

#define PIER_MAXTILES 4
#define RUNWAYSTRIP_MAXTILES 5

#define MARINA_TILES_ALLDRY 0
#define MARINA_TILES_ALLWET 9

#define INI_GAME_SPEED_SETTING(x) (x - 1)

#define HALVECOORD(x) (x >> 1)

// Struct defining an injected hook from a loaded mod and its nested call priority.
typedef struct {
	const char* szHookName;
	int iHookPriority;
} sc2kfix_mod_hook_t;

// Struct defining a mod in its entirety, including its version info, sc2kfix/OC2K version
// requirements, basic descriptions, and what hooks it injects code into.
typedef struct {
	int iModInfoVersion;				// Mandatory

	int iModVersionMajor;				// Mandatory
	int iModVersionMinor;				// Mandatory
	int iModVersionPatch;				// Mandatory
	int iMinimumVersionMajor;			// Mandatory
	int iMinimumVersionMinor;			// Mandatory
	int iMinimumVersionPatch;			// Mandatory

	const char* szModName;				// Mandatory
	const char* szModShortName;			// Mandatory
	const char* szModAuthor;			// Optional, but recommended
	const char* szModDescription;		// Optional, but recommended

	int iHookCount;						// Mandatory
	sc2kfix_mod_hook_t* stHooks;		// Mandatory
} sc2kfix_mod_info_t;

// Enum for mod hook types
enum {
	HOOKFN_TYPE_NONE,
	HOOKFN_TYPE_NATIVE,
	HOOKFN_TYPE_KUROKO
};

// Function pointers (native and otherwise) for hooks
typedef struct {
	int iPriority;
	int iType;
	void* pFunction;
	BOOL bEnabled;
} hook_function_t;

#include <hooklists.h>

typedef BOOL (*console_cmdproc_t)(const char* szCommand, const char* szArguments);

// Struct defining a core console command.
typedef struct {
	const char* szCommand;
	console_cmdproc_t fpProc;
	int iUndocumented;
	const char* szDescription;
} console_command_t;

// Struct defining debugging information for sound buffers.
typedef struct {
	int iSoundID;
	int iReloadCount;
} soundbufferinfo_t;

// Struct defining a sound to be replaced in hook_sndPlaySound.cpp. I genuinely don't have any
// better way to describe this one.
typedef struct {
	BYTE* bBuffer;
	DWORD nBufSize;
} sound_replacement_t;

// This structure is explicitly used in the settings dialogue.
// Once EndDialog is called (with TRUE set is the result)
// have it apply the variables back to their equivalent
// globals and save. If the EndDialog passed result is FALSE
// it insulates the primary globals from being modified.
typedef struct {
	// These are the primary settings.
	char szSettingsMayorName[64];
	char szSettingsCompanyName[64];

	BOOL bSettingsMusicInBackground;
	BOOL bSettingsUseSoundReplacements;
	BOOL bSettingsShuffleMusic;
	BOOL bSettingsFrequentCityRefresh;
	BOOL bSettingsUseMP3Music;
	BOOL bSettingsAlwaysPlayMusic;
	BOOL bSettingsAlwaysConsole;
	BOOL bSettingsCheckForUpdates;
	BOOL bSettingsDontLoadMods;
	BOOL bSettingsUseStatusDialog;
	BOOL bSettingsTitleCalendar;
	BOOL bSettingsUseNewStrings;
	BOOL bSettingsAlwaysSkipIntro;

	UINT iSettingsMusicEngineOutput;
	char szSettingsFluidSynthSoundfont[MAX_PATH + 1];

	char szSettingsMIDITrackPath[MUSIC_TRACKS][MAX_PATH + 1];
	char szSettingsMP3TrackPath[MUSIC_TRACKS][MAX_PATH + 1];

	// Attributes that the settings dialogue needs to know before and after.
	BOOL bActiveTrackChanged;
	BOOL bActiveMusicEngineTouched;

	UINT iCurrentMusicEngineOutput;
	char szCurrentFluidSynthSoundfont[MAX_PATH + 1];
} settings_t;

// Enum for console command visibility in inline help. Documented commands always appear in inline
// help, undocumented commands only appear if `set undocumented` has been activated. Commands
// tagged as aliases never appear. Commands tagged as script-only return an error in interactive
// mode but function in script mode.
enum {
	CONSOLE_COMMAND_DOCUMENTED = 0,
	CONSOLE_COMMAND_UNDOCUMENTED,
	CONSOLE_COMMAND_ALIAS,
	CONSOLE_COMMAND_SCRIPTONLY
};

// Enum for logging functionality. Roughly equates to syslog levels (see RFC 5424).
enum {
	LOG_NONE = -1,
	LOG_EMERGENCY,
	LOG_ALERT,
	LOG_CRITICAL,
	LOG_ERROR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG
};

// Game path global

extern char szGamePath[MAX_PATH];

// Settings globals

extern json::JSON jsonSettingsCore;
extern json::JSON jsonSettingsMods;

extern char szSettingsMayorName[64];
extern char szSettingsCompanyName[64];

extern UINT iSettingsMusicEngineOutput;
extern BOOL bSettingsMusicInBackground;
extern BOOL bSettingsUseSoundReplacements;
extern BOOL bSettingsShuffleMusic;
extern BOOL bSettingsUseMultithreadedMusic;
extern BOOL bSettingsFrequentCityRefresh;
extern BOOL bSettingsUseMP3Music;
extern BOOL bSettingsAlwaysPlayMusic;

extern BOOL bSettingsAlwaysConsole;
extern BOOL bSettingsCheckForUpdates;
extern BOOL bSettingsDontLoadMods;

extern BOOL bSettingsUseStatusDialog;
extern BOOL bSettingsTitleCalendar;
extern BOOL bSettingsUseNewStrings;
extern BOOL bSettingsAlwaysSkipIntro;

// Music track aliases

extern char szSettingsMIDITrackPath[MUSIC_TRACKS][MAX_PATH + 1];
extern char szSettingsMP3TrackPath[MUSIC_TRACKS][MAX_PATH + 1];

// Scenario state on-load information

extern const char* scScenarioDescription;
extern DWORD dwScenarioStartDays;
extern DWORD dwScenarioStartPopulation;
extern WORD wScenarioStartXVALTiles;
extern DWORD dwScenarioStartTrafficDivisor;

// Command line globals

extern int iForcedBits;

// Path adjustment (from registry_pathing area)

BOOL L_IsPathValid(const char *pStr);
BOOL L_IsDirectoryPathValid(const char *pStr);

// Utility functions

void InitializeFonts(void);
HOOKEXT void CenterDialogBox(HWND hwndDlg);
HOOKEXT HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText);
HOOKEXT const char* HexPls(UINT uNumber, int width);
HOOKEXT const char* FormatVersion(int iMajor, int iMinor, int iPatch);
HOOKEXT_CPP std::string WordWrap(std::string strInput, size_t iMaxWidth, size_t iIndentWidth);
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...);
HOOKEXT const char* GetLowHighScale(BYTE bScale);
HOOKEXT BOOL FileExists(const char* name);
HOOKEXT const char* GetFileBaseName(const char* szPath);
HOOKEXT const char* GetModsFolderPath(void);
HOOKEXT const char* GetOnIdleStateEnumName(int iState);
HOOKEXT const char* GetOnIdleInitialDialogEnumName(int iInitialDialogState);
//HBITMAP CreateSpriteBitmap(int iSpriteID);
HOOKEXT BOOL IsFileNameValid(const char *pName);
HOOKEXT BOOL WritePrivateProfileIntA(const char *section, const char *name, int value, const char *ini_name);
int MaxisDecompress(BYTE* pBuffer, size_t iBufSize, BYTE* pCompressedData, int iCompressedSize);
HOOKEXT_CPP std::string Base64Encode(const unsigned char* pSrcData, size_t iSrcCount);
HOOKEXT_CPP size_t Base64Decode(BYTE* pBuffer, size_t iBufSize, const unsigned char* pSrcData, size_t iSrcCount);
HOOKEXT_CPP json::JSON EncodeDWORDArray(DWORD* dwArray, size_t iCount, BOOL bBigEndian);
HOOKEXT_CPP json::JSON EncodeBudgetArray(DWORD* dwBudgetArray, BOOL bBigEndian);
HOOKEXT_CPP void DecodeDWORDArray(DWORD* dwArray, json::JSON jsonArray, size_t iCount, BOOL bBigEndian);
void PorntipsGuzzardo(void);

// Globals etc.

LONG WINAPI CrashHandler(LPEXCEPTION_POINTERS lpExceptions);
BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
void LoadStoredPaths(void);
void SaveStoredPaths(void);
int DoCheckAndInstall(void);
void ResetFileAssociations(void);
void SetGamePath(void);
const char *GetIniPath(void);
void InitializeSettings(void);
void LoadSettings(void);
void SaveSettings(BOOL onload);
void ShowSettingsDialog(void);
void ShowModSettingsDialog(void);
void ShowScenarioStatusDialog(void);
void ShowSpriteBrowseDialog(void);
BOOL CanUseFloatingStatusDialog();
void ToggleFloatingStatusDialog(BOOL bEnable);
void ToggleGotoButton(HWND hWndBut, BOOL bEnable);
void InstallSoundEngineHooks_SC2K1996(void);
BOOL UpdaterCheckForUpdates(void);
DWORD WINAPI UpdaterThread(LPVOID lpParameter);
const char *GetGameSoundPath();
int GetCurrentActiveSongID();
BOOL MusicLoadFluidSynth(void);
void DoMusicPlay(int iSongID, BOOL bInterrupt);
BOOL DoConfigureMusicTracks(settings_t *st, HWND hDlg, BOOL bMP3);

BOOL CopyReplacementString(char *pDest, rsize_t SizeInBytes, const char *pSrc);

FILE *old_fopen(const char *fname, const char *mode);

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType);
DWORD WINAPI ConsoleThread(LPVOID lpParameter);
BOOL ConsoleEvaluateCommand(const char* szCommandLine, BOOL bInteractive);
BOOL ConsoleCmdClear(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdEcho(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdRun(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdWait(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdHelp(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShow(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowDebug(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowMemory(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowMicrosim(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowMods(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowSound(const char* szCommand, const char* szArguments);
//BOOL ConsoleCmdShowSprite(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowTile(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowVersion(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSet(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetTile(const char* szCommand, const char* szArguments);

void LoadNativeCodeMods(void);

#if !NOKUROKO
DWORD WINAPI KurokoThread(LPVOID lpParameter);
#endif

extern const char *gamePrimaryKey;

extern char szLastStoredCityPath[MAX_PATH + 1];
extern char szLastStoredTileSetPath[MAX_PATH + 1];

extern BOOL bGameDead;
extern HMODULE hRealWinMM;
extern HMODULE hSC2KAppModule;
extern HMODULE hSC2KFixModule;
extern HMODULE hmodFluidSynth;
extern HANDLE hConsoleThread;
extern HMENU hMainMenu;
extern HMENU hGameMenu;
extern HMENU hDebugMenu;
extern FARPROC fpWinMMHookList[180];
extern DWORD dwDetectedVersion;
extern DWORD dwSC2KFixMode;
extern DWORD dwDetectedAppTimestamp;
extern DWORD dwSC2KFixVersion;
extern const char* szSC2KFixVersion;
extern const char* szSC2KFixReleaseTag;
extern const char* szSC2KFixBuildInfo;
extern BOOL bConsoleEnabled;
extern BOOL bSkipIntro;
extern BOOL bUseAdvancedQuery;
#if !NOKUROKO
extern BOOL bKurokoVMInitialized;
extern DWORD dwConsoleThreadID;
extern DWORD dwKurokoThreadID;
#endif

extern BOOL bFontsInitialized;
extern HFONT hFontMSSansSerifRegular8;
extern HFONT hFontMSSansSerifBold8;
extern HFONT hFontMSSansSerifRegular10;
extern HFONT hFontMSSansSerifBold10;
extern HFONT hFontArialRegular10;
extern HFONT hFontArialBold10;
extern HFONT hFontArialBold16;
extern HFONT hSystemRegular12;

extern std::map<HMODULE, sc2kfix_mod_info_t> mapLoadedNativeMods;
extern std::map<HMODULE, std::vector<sc2kfix_mod_hook_t>> mapLoadedNativeModHooks;
extern std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
extern std::vector<int> vectorRandomSongIDs;
extern std::random_device rdRandomDevice;
extern std::mt19937 mtMersenneTwister;

extern HANDLE hWeatherBitmaps[13];
extern HANDLE hCompassBitmaps[4];
extern BOOL bStatusDialogMoving;

extern char szLatestRelease[24];
extern BOOL bUpdateAvailable;

HOOKEXT BOOL bHookStopProcessing;

// Hooks to inject in dllmain.cpp

void InstallAnimationHooks_SC2K1996(void);
void InstallAnimationHooks_SC2K1995(void);
void InstallAnimationHooks_SC2KDemo(void);
void InstallSpriteAndTileSetHooks_SC2K1996(void);
void InstallTileGrowthOrPlacementHandlingHooks_SC2K1996(void);
void InstallToolBarHooks_SC2K1996(void);
void InstallMiscHooks_SC2K1996(void);
void UpdateMiscHooks_SC2K1996(void);
void InstallMiscHooks_SC2K1995(void);
void InstallMiscHooks_SC2KDemo(void);
void InstallStatusHooks_SC2K1996(void);
void UpdateStatus_SC2K1996(int iShow);
void InstallQueryHooks_SC2K1996(void);
void InstallMilitaryHooks_SC2K1996(void);
void InstallSaveHooks_SC2K1996(void);
extern "C" int __stdcall Hook_LoadSoundIntoBuffer(int iSoundID, void* lpBuffer);
int L_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
void ReloadDefaultTileSet_SC2K1996();
int IsValidSiloPosCheck(__int16 m_x, __int16 m_y);
void ProposeMilitaryBaseDecline(void);
void ProposeMilitaryBaseMissileSilos(void);
void ProposeMilitaryBaseAirForceBase(void);
void ProposeMilitaryBaseArmyBase(void);
void ProposeMilitaryBaseNavalYard(void);

// Registry hooks
void InstallRegistryPathingHooks_SC2K1996(void);
void InstallRegistryPathingHooks_SC2K1995(void);
void InstallRegistryPathingHooks_SC2KDemo(void);
void InstallRegistryPathingHooks_SCURKPrimary(void);
void InstallRegistryPathingHooks_SCURK1996(void);

// Movie hook
void InstallMovieHooks(void);

// Debugging settings

extern UINT guzzardo_debug;
extern UINT mci_debug;
extern UINT military_debug;
extern UINT mischook_debug;
extern UINT modloader_debug;
extern UINT mov_debug;
extern UINT mus_debug;
extern UINT registry_debug;
extern UINT sc2x_debug;
extern UINT snd_debug;
extern UINT sprite_debug;
extern UINT timer_debug;
extern UINT updatenotifier_debug;

// SCURK specific stuff

void InstallFixes_SCURKPrimary(void);
void InstallFixes_SCURK1996(void);
