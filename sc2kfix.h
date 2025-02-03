// sc2kfix sc2kfix.h: globals that need to be used elsewhere
// (c) 2025 github.com/araxestroy - released under the MIT license

#pragma once

#include <windows.h>

// Turning this on spits out a bunch of debug dialog boxes. You have been warned.
// #define DEBUG

// Turning this on enables experimental features. You have been warned.
#define HOOK_ENABLED

#define VERSION_UNKNOWN 0
#define VERSION_1995    1
#define VERSION_1996    2

#define SC2KFIX_VERSION	"0.5-dev"

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoRegistryCheckAndInstall(void);
void DebugOutput(const char* fmt, ...);

DWORD WINAPI ConsoleThread(LPVOID lpParameter);

extern HMODULE hRealWinMM;
extern HMODULE hSC2KAppModule;
extern HMODULE hSC2KFixModule;
extern HANDLE hConsoleThread;
extern DWORD dwDetectedVersion;
extern DWORD dwSC2KAppTimestamp;
