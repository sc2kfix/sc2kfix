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
BOOL bConsoleUndocumentedMode = FALSE;

console_command_t fpConsoleCommands[] = {
	{ "?", ConsoleCmdHelp, CONSOLE_COMMAND_ALIAS, "" },
	{ "help", ConsoleCmdHelp, CONSOLE_COMMAND_DOCUMENTED, "Display this help" },
	{ "set", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "show", ConsoleCmdShow, CONSOLE_COMMAND_DOCUMENTED, "Display various game and plugin information" }
};

// COMMMAND: help / ?

BOOL ConsoleCmdHelp(const char* szCommand, const char* szArguments) {
	// Iterate through the current context command list. We start at 1 to hide the implicit initial "?" pseudo-command.
	size_t uLongestCommand = 0;

	for (size_t i = 0; i < sizeof(fpConsoleCommands) / sizeof(console_command_t); i++)
		if (strlen(fpConsoleCommands[i].szCommand) > uLongestCommand && (fpConsoleCommands[i].iUndocumented || (bConsoleUndocumentedMode && fpConsoleCommands[i].iUndocumented != CONSOLE_COMMAND_ALIAS)))
			uLongestCommand = strlen(fpConsoleCommands[i].szCommand);

	uLongestCommand += 6;

	for (size_t i = 0; i < sizeof(fpConsoleCommands) / sizeof(console_command_t); i++)
		if (!fpConsoleCommands[i].iUndocumented || (bConsoleUndocumentedMode && fpConsoleCommands[i].iUndocumented != CONSOLE_COMMAND_ALIAS))
			printf(" %c%-*s%s\n", (fpConsoleCommands[i].iUndocumented == CONSOLE_COMMAND_UNDOCUMENTED ? '*' : ' '), (int)uLongestCommand, fpConsoleCommands[i].szCommand, fpConsoleCommands[i].szDescription);

	return TRUE;
}

// COMMAND: show [...]

BOOL ConsoleCmdShow(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  show debug     Display enabled debugging options\n"
			"  show version   Display sc2kfix version info\n");
		return TRUE;
	}

	if (!strcmp(szArguments, "debug"))
		return ConsoleCmdShowDebug(szCommand, szArguments);

	if (!strcmp(szArguments, "version"))
		return ConsoleCmdShowVersion(szCommand, szArguments);

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdShowDebug(const char* szCommand, const char* szArguments) {
	printf("Debugging labels enabled: ");
	if (mci_debug) {
		printf("MCI");
	}
	printf("\n");

	return TRUE;
}

BOOL ConsoleCmdShowVersion(const char* szCommand, const char* szArguments) {
	const char* szSC2KVersion = "unknown";
	switch (dwDetectedVersion) {
	case SC2KVERSION_1995:
		szSC2KVersion = "1995 CD Collection";
	case SC2KVERSION_1996:
		szSC2KVersion = "1996 Special Edition";
	}

	printf(
		"sc2kfix version %s - https://github.com/araxestroy/sc2kfix\n"
		"Plugin build info: %s\n"
		"SimCity 2000 version: %s\n"
		"Plugin loaded at 0x%08X\n", szSC2KFixVersion, szSC2KFixBuildInfo, szSC2KVersion, (DWORD)hSC2KFixModule);
	printf("\n");

	return TRUE;
}

// COMMAND: set [...]

BOOL ConsoleCmdSet(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  set debug [...]   Enable debugging output\n");
		return TRUE;
	}

	if (!strncmp(szArguments, "debug ", 6))
		return ConsoleCmdSetDebug(szCommand, szArguments + 6);

	if (!strcmp(szArguments, "undocumented")) {
		bConsoleUndocumentedMode = TRUE;
		return TRUE;
	}

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  set debug mci   Enable MCI debugging\n");
		return TRUE;
	}
	
	if (!strcmp(szArguments, "mci")) {
		mci_debug = TRUE;
		return TRUE;
	}
}

// CONSOLE THREAD

DWORD WINAPI ConsoleThread(LPVOID lpParameter) {
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	for (;;) {
		gets_s(szCmdBuf, 256);
		if (!ConsoleEvaluateCommand(szCmdBuf))
			printf("Invalid command.\n");

		if (bConsoleUndocumentedMode)
			printf("# ");
		else
			printf("> ");
	}
}

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		// TODO - reset the console input handler somehow
		return TRUE;
	}
	return FALSE;
}

BOOL ConsoleEvaluateCommand(const char* szCommandLine) {
	size_t uCmdLen = strchr(szCommandLine, ' ') - szCommandLine;
	const char* szArguments = "";
	if (!strchr(szCommandLine, ' '))
		uCmdLen = strlen(szCommandLine);
	else
		szArguments = strchr(szCommandLine, ' ') + 1;

	for (size_t i = 0; i < sizeof(fpConsoleCommands) / sizeof(console_command_t); i++) {
		if (uCmdLen != strlen(fpConsoleCommands[i].szCommand))
			continue;
		if (!memcmp(szCommandLine, fpConsoleCommands[i].szCommand, uCmdLen)) {
			BOOL bRetval = fpConsoleCommands[i].fpProc(fpConsoleCommands[i].szCommand, szArguments);
			return bRetval;
		}
	}

	if (!*szCommandLine)
		return TRUE;

	return FALSE;
}
