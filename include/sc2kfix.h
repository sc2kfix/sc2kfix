// sc2kfix include/sc2kfix.h: globals that need to be used elsewhere
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once
#pragma warning(disable : 4200)

#include <windows.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <random>

#include <smk.h>
#include <sc2k_1996.h>
#include <music.h>
#include <json.hpp>

// Turning this on enables every debugging option. You have been warned.
// #define DEBUGALL

// Turning this on forces the console to be enabled, as if -console was passed to SIMCITY.EXE.
// #define CONSOLE_ENABLED

#define SC2KVERSION_UNKNOWN 0
#define SC2KVERSION_1995    1
#define SC2KVERSION_1996    2

#define SC2KFIX_VERSION			"0.10-dev"
#define SC2KFIX_VERSION_MAJOR	0
#define SC2KFIX_VERSION_MINOR	10
#define SC2KFIX_VERSION_PATCH	0
#define SC2KFIX_RELEASE_TAG		"r9c"

#define SC2KFIX_INIFILE		"sc2kfix.ini"
#define SC2KFIX_MODSFOLDER	"mods"

#define HOOKEXT extern "C" __declspec(dllexport)

#define countof(x) (sizeof(x)/sizeof(*(x)))
#define lengthof(s) (countof(s)-1)

// The nearest equivalent of LOWORD() from IDA.
#define P_LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define P_LOW_IND(x,part_type)   P_LAST_IND(x,part_type)
#  define P_HIGH_IND(x,part_type)  0
#else
#  define P_HIGH_IND(x,part_type)  P_LAST_IND(x,part_type)
#  define P_LOW_IND(x,part_type)   0
#endif
#define P_BYTEn(x, n) (*((BYTE*)&(x)+n))
#define P_LOBYTE(x) P_BYTEn(x,P_LOW_IND(x,BYTE))
#define P_LOWORD(x) (*((uint16_t*)&(x)))
#define P_HIWORD(x) (*((uint16_t*)&(x)+1))
// Signed
#define P_SHIWORD(x) (*((int16_t*)&(x)+1))

#define IFF_HEAD(a, b, c, d) ((DWORD)d << 24 | (DWORD)c << 16 | (DWORD)b << 8 | (DWORD)a)

#define RELATIVE_OFFSET(from, to) *(DWORD*)((DWORD)(from)) = (DWORD)(to) - (DWORD)(from) - 4;
#define NEWCALL(from, to) *(BYTE*)(from) = 0xE8; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJMP(from, to) *(BYTE*)(from) = 0xE9; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x82; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x83; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x84; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x85; RELATIVE_OFFSET((DWORD)(from)+2, to)

#define DEBUG_FLAGS_NONE		0
#define DEBUG_FLAGS_EVERYTHING	0xFFFFFFFF

#define WM_KUROKO_REPL	WM_APP+0x10
#define WM_KUROKO_FILE	WM_APP+0x11
#define WM_CONSOLE_REPL	WM_APP+0x20

typedef struct {
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	UINT_PTR nSig;
	void* pfn;
} AFX_MSGMAP_ENTRY;

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
} sc2kfix_mod_info_t;

typedef struct {
	const char* szHookName;
	int iHookPriority;
} sc2kfix_mod_hook_t;

typedef struct {
	int iHookCount;
	sc2kfix_mod_hook_t stHooks[];
} sc2kfix_mod_hooklist_t;

typedef struct {
	int iPriority;
	void* pFunction;
} hook_function_t;

#include <hooklists.h>

typedef BOOL (*console_cmdproc_t)(const char* szCommand, const char* szArguments);

typedef struct {
	const char* szCommand;
	console_cmdproc_t fpProc;
	int iUndocumented;
	const char* szDescription;
} console_command_t;

typedef struct {
	int iSoundID;
	int iReloadCount;
} soundbufferinfo_t;

typedef struct {
	BYTE* bBuffer;
	DWORD nBufSize;
} sound_replacement_t;

enum {
	CONSOLE_COMMAND_DOCUMENTED = 0,
	CONSOLE_COMMAND_UNDOCUMENTED,
	CONSOLE_COMMAND_ALIAS,
	CONSOLE_COMMAND_SCRIPTONLY
};

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

extern char szSettingsMayorName[64];
extern char szSettingsCompanyName[64];

extern BOOL bSettingsMusicInBackground;
extern BOOL bSettingsUseSoundReplacements;
extern BOOL bSettingsShuffleMusic;
extern BOOL bSettingsUseMultithreadedMusic;
extern BOOL bSettingsFrequentCityRefresh;
extern BOOL bSettingsUseMP3Music;

extern BOOL bSettingsAlwaysConsole;
extern BOOL bSettingsCheckForUpdates;
extern BOOL bSettingsDontLoadMods;

extern BOOL bSettingsUseStatusDialog;
extern BOOL bSettingsTitleCalendar;
extern BOOL bSettingsUseNewStrings;
extern BOOL bSettingsAlwaysSkipIntro;

// Path adjustment (from registry_pathing area)

const char *AdjustSource(char *buf, const char *path);

// Utility functions

void InitializeFonts(void);
void CenterDialogBox(HWND hwndDlg);
HOOKEXT HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText);
HOOKEXT const char* HexPls(UINT uNumber, int width);
HOOKEXT const char* FormatVersion(int iMajor, int iMinor, int iPatch);
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...);
int GetTileID(int iTileX, int iTileY);
const char* GetZoneName(int iZoneID);
const char* GetLowHighScale(BYTE bScale);
HOOKEXT BOOL FileExists(const char* name);
HOOKEXT const char* GetModsFolderPath(void);
HBITMAP CreateSpriteBitmap(int iSpriteID);
BOOL WritePrivateProfileIntA(const char *section, const char *name, int value, const char *ini_name);
void MigrateRegStringValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, char *szOutBuf, DWORD dwLen);
void MigrateRegDWORDValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, DWORD *dwOut, DWORD dwSize);
void MigrateRegBOOLValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, BOOL *bOut);
int MaxisDecompress(BYTE* pBuffer, size_t iBufSize, BYTE* pCompressedData, int iCompressedSize);
std::string Base64Encode(const unsigned char* pSrcData, size_t iSrcCount);
size_t Base64Decode(BYTE* pBuffer, size_t iBufSize, const unsigned char* pSrcData, size_t iSrcCount);
json::JSON EncodeDWORDArray(DWORD* dwArray, size_t iCount, BOOL bBigEndian);
void DecodeDWORDArray(DWORD* dwArray, json::JSON jsonArray, size_t iCount, BOOL bBigEndian);

// Globals etc.

LONG WINAPI CrashHandler(LPEXCEPTION_POINTERS lpExceptions);
BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
int DoRegistryCheckAndInstall(void);
void SetGamePath(void);
const char *GetIniPath();
void LoadSettings(void);
void SaveSettings(BOOL onload);
void ShowSettingsDialog(void);
HWND ShowStatusDialog(void);
void LoadReplacementSounds(void);
BOOL UpdaterCheckForUpdates(void);
DWORD WINAPI UpdaterThread(LPVOID lpParameter);

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
BOOL ConsoleCmdShowSprite(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowTile(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowVersion(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSet(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetTile(const char* szCommand, const char* szArguments);

void LoadNativeCodeMods(void);

DWORD WINAPI KurokoThread(LPVOID lpParameter);

extern BOOL bGameDead;
extern HMODULE hRealWinMM;
extern HMODULE hSC2KAppModule;
extern HMODULE hSC2KFixModule;
extern HANDLE hConsoleThread;
extern HMENU hGameMenu;
extern FARPROC fpWinMMHookList[180];
extern DWORD dwDetectedVersion;
extern DWORD dwSC2KAppTimestamp;
extern DWORD dwSC2KFixVersion;
extern const char* szSC2KFixVersion;
extern const char* szSC2KFixReleaseTag;
extern const char* szSC2KFixBuildInfo;
extern BOOL bInSCURK;
extern BOOL bConsoleEnabled;
extern BOOL bSkipIntro;
extern BOOL bUseAdvancedQuery;
extern BOOL bKurokoVMInitialized;
extern DWORD dwConsoleThreadID;
extern DWORD dwKurokoThreadID;

extern BOOL bFontsInitialized;
extern HFONT hFontMSSansSerifRegular8;
extern HFONT hFontMSSansSerifBold8;
extern HFONT hFontMSSansSerifRegular10;
extern HFONT hFontMSSansSerifBold10;
extern HFONT hFontArialRegular10;
extern HFONT hFontArialBold10;
extern HFONT hSystemRegular12;

extern std::map<HMODULE, sc2kfix_mod_info_t> mapLoadedNativeMods;
extern std::map<HMODULE, std::vector<sc2kfix_mod_hook_t>> mapLoadedNativeModHooks;
extern std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
extern std::vector<int> vectorRandomSongIDs;
extern std::random_device rdRandomDevice;
extern std::mt19937 mtMersenneTwister;

extern HWND hStatusDialog;
extern HANDLE hWeatherBitmaps[13];
extern HANDLE hCompassBitmaps[4];

extern char szLatestRelease[24];
extern BOOL bUpdateAvailable;

// Hooks to inject in dllmain.cpp

void InstallMiscHooks(void);
void UpdateMiscHooks(void);
void InstallQueryHooks(void);
void InstallMilitaryHooks(void);
extern "C" void __stdcall Hook_LoadSoundBuffer(int iSoundID, void* lpBuffer);
extern "C" int __stdcall Hook_MusicPlay(int iSongID);
extern "C" int __stdcall Hook_MusicStop(void);
extern "C" int __stdcall Hook_MusicPlayNextRefocusSong(void);
extern "C" int __stdcall Hook_402793(int iStatic, char* szText, int iMaybeAlways1, COLORREF crColor);
extern "C" int __stdcall Hook_4021A8(HWND iShow);
extern "C" int __stdcall Hook_40103C(int iShow);

// Registry hooks
void InstallRegistryPathingHooks_SC2K1996(void);
void InstallRegistryPathingHooks_SC2K1995(void);
void InstallRegistryPathingHooks_SCURK1996(void);

// Debugging settings

extern UINT mci_debug;
extern UINT military_debug;
extern UINT mischook_debug;
extern UINT modloader_debug;
extern UINT mus_debug;
extern UINT snd_debug;
extern UINT timer_debug;
extern UINT updatenotifier_debug;

// SCURK specific stuff

BOOL InjectSCURKFix(void);
