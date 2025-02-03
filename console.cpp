// sc2kfix console.cpp: experiment console
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "sc2kfix.h"

HANDLE hConsoleThread;

DWORD WINAPI ConsoleThread(LPVOID lpParameter) {
	char szCmdBuf[256] = { 0 };
	for (;;) {
		printf("> ");
		gets_s(szCmdBuf, 256);
	}
}