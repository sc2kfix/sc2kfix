// sc2kfix console.cpp: experiment console
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "sc2kfix.h"

HANDLE hConsoleThread;
char szCmdBuf[256] = { 0 };

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		// TODO - reset the console input handler somehow
		return TRUE;
	}
	return FALSE;
}

void CmdShowDebug(void) {
	printf("Debugging labels enabled: ");
	if (mci_debug) {
		printf("MCI");
	}
	printf("\n");
}

DWORD WINAPI ConsoleThread(LPVOID lpParameter) {
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	for (;;) {
		gets_s(szCmdBuf, 256);

		// TODO - add actual console processing here

		printf("> ");
	}
}