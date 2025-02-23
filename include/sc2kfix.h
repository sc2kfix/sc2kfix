// sc2kfix sc2kfix.h: globals that need to be used elsewhere
// (c) 2025 github.com/araxestroy - released under the MIT license

#pragma once

#include <windows.h>
#include <map>
#include <vector>
#include <algorithm>
#include <random>

#include <sc2k_1996.h>

// Turning this on enables every debugging option. You have been warned.
// #define DEBUGALL

// Turning this on forces the console to be enabled, as if -console was passed to SIMCITY.EXE.
// #define CONSOLE_ENABLED

#define SC2KVERSION_UNKNOWN 0
#define SC2KVERSION_1995    1
#define SC2KVERSION_1996    2

#define SC2KFIX_VERSION		"0.8-dev"
#define SC2KFIX_RELEASE_TAG	"r7a"

#define RELATIVE_OFFSET(from, to) *(DWORD*)((DWORD)(from)) = (DWORD)(to) - (DWORD)(from) - 4;
#define NEWCALL(from, to) *(BYTE*)(from) = 0xE8; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJMP(from, to) *(BYTE*)(from) = 0xE9; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x82; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x83; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x84; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x85; RELATIVE_OFFSET((DWORD)(from)+2, to)

#define DEBUG_FLAGS_EVERYTHING 0xFFFFFFFF

typedef struct {
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	UINT_PTR nSig;
	void* pfn;
} AFX_MSGMAP_ENTRY;

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
	CONSOLE_COMMAND_ALIAS
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

// Settings globals
extern char szSettingsMayorName[64];
extern char szSettingsCompanyName[64];
extern BOOL bSettingsMusicInBackground;
extern BOOL bSettingsUseNewStrings;
extern BOOL bSettingsUseSoundReplacements;
extern BOOL bSettingsMilitaryBaseRevenue;
extern BOOL bSettingsShuffleMusic;
extern BOOL bSettingsAlwaysConsole;

// Globals etc.

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SettingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoRegistryCheckAndInstall(void);
void LoadSettings(void);
void SaveSettings(void);
void ShowSettingsDialog(void);
void LoadReplacementSounds(void);
void MusicShufflePlaylist(int iLastSongPlayed);
const char* HexPls(UINT uNumber);
void ConsoleLog(int iLogLevel, const char* fmt, ...);

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType);
DWORD WINAPI ConsoleThread(LPVOID lpParameter);
BOOL ConsoleEvaluateCommand(const char* szCommandLine);
BOOL ConsoleCmdHelp(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShow(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowDebug(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowMemory(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowMicrosim(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowSound(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowTile(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdShowVersion(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSet(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSetTile(const char* szCommand, const char* szArguments);

extern HMODULE hRealWinMM;
extern HMODULE hSC2KAppModule;
extern HMODULE hSC2KFixModule;
extern HANDLE hConsoleThread;
extern HMENU hGameMenu;
extern FARPROC fpWinMMHookList[180];
extern DWORD dwDetectedVersion;
extern DWORD dwSC2KAppTimestamp;
extern const char* szSC2KFixVersion;
extern const char* szSC2KFixReleaseTag;
extern const char* szSC2KFixBuildInfo;
extern BOOL bInSCURK;
extern BOOL bConsoleEnabled;

extern std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
extern std::vector<int> vectorRandomSongIDs;
extern std::random_device rdRandomDevice;
extern std::mt19937 mtMersenneTwister;

extern HWND hStatusDialog;
extern HFONT hStatusDialogBoldFont;
extern HANDLE hWeatherBitmaps[12];

// Hooks to inject in dllmain.cpp

void InstallMiscHooks(void);
void UpdateMiscHooks(void);
extern "C" void __stdcall Hook_LoadSoundBuffer(int iSoundID, void* lpBuffer);
extern "C" int __stdcall Hook_MusicPlayNextRefocusSong(void);
extern "C" int __stdcall Hook_402793(int iStatic, char* szText, int iMaybeAlways1, COLORREF crColor);

// Debugging settings

extern UINT mci_debug;
extern UINT snd_debug;
extern UINT timer_debug;
extern UINT mischook_debug;

// SCURK specific stuff

BOOL InjectSCURKFix(void);
