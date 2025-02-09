// sc2kfix sc2kfix.h: globals that need to be used elsewhere
// (c) 2025 github.com/araxestroy - released under the MIT license

#pragma once

#include <windows.h>

// Turning this on enables every debugging option. You have been warned.
// #define DEBUGALL

// Turning this on enables experimental features. You have been warned.
#define CONSOLE_ENABLED

#define SC2KVERSION_UNKNOWN 0
#define SC2KVERSION_1995    1
#define SC2KVERSION_1996    2

#define SC2KFIX_VERSION	"0.5-dev"

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

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoRegistryCheckAndInstall(void);
const char* HexPls(UINT uNumber);
void ConsoleLog(int iLogLevel, const char* fmt, ...);

DWORD WINAPI ConsoleThread(LPVOID lpParameter);

extern HMODULE hRealWinMM;
extern HMODULE hSC2KAppModule;
extern HMODULE hSC2KFixModule;
extern HANDLE hConsoleThread;
extern FARPROC fpWinMMHookList[180];
extern DWORD dwDetectedVersion;
extern DWORD dwSC2KAppTimestamp;
extern const char* szSC2KFixVersion;

// Debugging settings

extern UINT mci_debug;