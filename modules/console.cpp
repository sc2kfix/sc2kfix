// sc2kfix modules/console.cpp: sc2kfix console and SX2 interpreter
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Notes: 2025-03-01 (@araxestroy)
//
// This might be getting a bit out of hand. We're now implementing a quick-and-dirty scripting
// language in this console using regexes and C++ strings. It's not great but it seems to work
// well enough to actually be useful in a few scenarios. I still hate it though.
//
// I think I'd rather have a proper Lua implementation but this is good enough for now. It helps
// with the reverse engineering process and that's what matters to me.
//
// I'd say I remember when I used to have standards, but I never have.

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <io.h>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#ifdef CONSOLE_ENABLED
BOOL bConsoleEnabled = TRUE;
#else 
BOOL bConsoleEnabled = FALSE;
#endif

typedef struct {
	int iType;
	DWORD dwContents;
} script_variable_t;

HANDLE hConsoleThread;
char szCmdBuf[256] = { 0 };
BOOL bConsoleUndocumentedMode = FALSE;
int iConsoleScriptNest = 0;
std::map<std::string, script_variable_t> mapVariables;

static BOOL ConsoleCmdShowTest(const char* szCommand, const char* szArguments);

console_command_t fpConsoleCommands[] = {
	{ "?", ConsoleCmdHelp, CONSOLE_COMMAND_ALIAS, "" },
	{ "clear", ConsoleCmdClear, CONSOLE_COMMAND_DOCUMENTED, "Clear all variables" },
	{ "echo", ConsoleCmdEcho, CONSOLE_COMMAND_DOCUMENTED, "Print to console" },
	{ "echo!", ConsoleCmdEcho, CONSOLE_COMMAND_UNDOCUMENTED, "Print to console without newline" },
	{ "help", ConsoleCmdHelp, CONSOLE_COMMAND_DOCUMENTED, "Display this help" },
	//{ "label", ConsoleCmdLabel, CONSOLE_COMMAND_SCRIPTONLY, "Script label" },
	{ "run", ConsoleCmdRun, CONSOLE_COMMAND_DOCUMENTED, "Run console script" },
	{ "set", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "show", ConsoleCmdShow, CONSOLE_COMMAND_DOCUMENTED, "Display various game and plugin information" },
	{ "unset", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "wait", ConsoleCmdWait, CONSOLE_COMMAND_UNDOCUMENTED, "Wait for a number of milliseconds" },
};

void ConsoleScriptSleep(DWORD dwMilliseconds) {
	int i = dwMilliseconds / 100;
	do {
		if (!iConsoleScriptNest)
			return;
		Sleep(100);
		i--;
	} while (i);
}

BOOL ConsoleCmdLabel(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments)
		return FALSE;
}

// COMMAND: run ...

BOOL ConsoleCmdRun(const char* szCommand, const char* szArguments) {
	std::string strPossibleScriptName;
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf("  run <filename>   Executes a file as a series of console commands\n");
		return TRUE;
	}

	// Try to find the script we want
	strPossibleScriptName = szArguments;
	if (!FileExists(strPossibleScriptName.c_str())) {
		strPossibleScriptName += ".sx2";
		if (!FileExists(strPossibleScriptName.c_str())) {
			ConsoleLog(LOG_ERROR, "CORE: Couldn't find script %s.\n", szArguments);
			return TRUE;
		}
	}

	// Iterate through the script and run lines until they fail
	std::ifstream fsScriptFile(strPossibleScriptName);
	std::string strScriptLine;
	int i = 1;
	iConsoleScriptNest++;
	while (std::getline(fsScriptFile, strScriptLine)) {
		if (!ConsoleEvaluateCommand(strScriptLine.c_str(), FALSE) || !iConsoleScriptNest) {
			ConsoleLog(LOG_ERROR, "CORE: Script %s aborted on line %d.\n", strPossibleScriptName.c_str(), i);
			return TRUE;
		}

		// We survived!
		i++;
	}

	// Decrement the nesting count and break	
	iConsoleScriptNest--;
	return TRUE;
}

BOOL ConsoleCmdClear(const char* szCommand, const char* szArguments) {
	mapVariables.clear();
	if (!iConsoleScriptNest)
		printf("Variable map cleared.\n");
	return TRUE;
}

BOOL ConsoleCmdEcho(const char* szCommand, const char* szArguments) {
	printf("%s%s", szArguments, (!strcmp(szCommand, "echo!") ? "" : "\n"));
	return TRUE;
}

BOOL ConsoleCmdWait(const char* szCommand, const char* szArguments) {
	int iMilliseconds = 0;

	// Function only valid in script mode
	if (!iConsoleScriptNest)
		return FALSE;

	// Sleep for N milliseconds
	if (!sscanf_s(szArguments, "%d", &iMilliseconds)) {
		ConsoleLog(LOG_ERROR, "CORE: Bad argument to `wait`.\n");
		return FALSE;
	}
	ConsoleScriptSleep(iMilliseconds);
	return TRUE;
}

// COMMMAND: help / ?

BOOL ConsoleCmdHelp(const char* szCommand, const char* szArguments) {
	// Iterate through the current context command list. We start at 1 to hide the implicit initial "?" pseudo-command.
	size_t uLongestCommand = 0;

	for (size_t i = 0; i < sizeof(fpConsoleCommands) / sizeof(console_command_t); i++)
		if (strlen(fpConsoleCommands[i].szCommand) > uLongestCommand && (fpConsoleCommands[i].iUndocumented || (bConsoleUndocumentedMode && fpConsoleCommands[i].iUndocumented < CONSOLE_COMMAND_ALIAS)))
			uLongestCommand = strlen(fpConsoleCommands[i].szCommand);

	uLongestCommand += 6;

	for (size_t i = 0; i < sizeof(fpConsoleCommands) / sizeof(console_command_t); i++)
		if (!fpConsoleCommands[i].iUndocumented || (bConsoleUndocumentedMode && fpConsoleCommands[i].iUndocumented < CONSOLE_COMMAND_ALIAS))
			printf(" %c%-*s%s\n", (fpConsoleCommands[i].iUndocumented == CONSOLE_COMMAND_UNDOCUMENTED ? '*' : ' '), (int)uLongestCommand, fpConsoleCommands[i].szCommand, fpConsoleCommands[i].szDescription);

	return TRUE;
}

// COMMAND: show [...]

BOOL ConsoleCmdShow(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  show debug          Display enabled debugging options\n"
			"  show memory ...     Display memory contents\n"
			"  show microsim ...   Display microsim info\n"
			"  show sound          Display sound info\n"
			"  show tile ...       Display tile info\n"
			"  show version        Display sc2kfix version info\n");
		return TRUE;
	}

	if (!strcmp(szArguments, "debug"))
		return ConsoleCmdShowDebug(szCommand, szArguments);

	if (!strcmp(szArguments, "memory") || !strncmp(szArguments, "memory ", 7))
		return ConsoleCmdShowMemory(szCommand, szArguments);

	if (!strcmp(szArguments, "microsim") || !strncmp(szArguments, "microsim ", 9))
		return ConsoleCmdShowMicrosim(szCommand, szArguments);

	if (!strcmp(szArguments, "sound") || !strncmp(szArguments, "sound ", 6))
		return ConsoleCmdShowSound(szCommand, szArguments);

	if (!strcmp(szArguments, "test"))
		return ConsoleCmdShowTest(szCommand, szArguments);

	if (!strcmp(szArguments, "tile") || !strncmp(szArguments, "tile ", 5))
		return ConsoleCmdShowTile(szCommand, szArguments);

	if (!strcmp(szArguments, "version"))
		return ConsoleCmdShowVersion(szCommand, szArguments);

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdShowDebug(const char* szCommand, const char* szArguments) {
	printf("Debugging labels enabled: ");
	if (mci_debug) {
		printf("MCI=0x%08X ", mci_debug);
	}
	if (snd_debug) {
		printf("SND=0x%08X ", snd_debug);
	}
	if (timer_debug) {
		printf("TIMER=0x%08X ", timer_debug);
	}
	if (mischook_debug) {
		printf("MISC=0x%08X ", mischook_debug);
	}
	printf("\n");

	return TRUE;
}

BOOL ConsoleCmdShowMemory(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != SC2KVERSION_1996) {
		printf("Command only available when attached to 1996 Special Edition.\n");
		return TRUE;
	}

	if (*(szArguments + 6) == '\0' || *(szArguments + 7) == '\0' || !strcmp(szArguments + 7, "?")) {
		printf(
			"Usage:\n"
			"  show memory <address> [operand_size] [range_size]\n"
			"    <address>: Address in hexadecimal\n"
			"    [operand_size]: Optional, one of: { byte, word, dword, range } (default dword)\n"
			"    [range_size]: Size of range if operand_size is \"range\" (default 16)\n");
		return TRUE;
	}

	DWORD dwAddress = NULL;
	char szOperandSize[6] = { 0 };
	int iRange = 16;
	sscanf_s(szArguments + 7, "%X %s %i", &dwAddress, szOperandSize, sizeof(szOperandSize), &iRange);

	__try {
		if (!*szOperandSize || !strcmp(szOperandSize, "dword"))
			printf("0x%08X: (dword) 0x%08X\n", dwAddress, *(DWORD*)dwAddress);
		else if (!strcmp(szOperandSize, "word"))
			printf("0x%08X: (word) 0x%04X\n", dwAddress, *(WORD*)dwAddress);
		else if (!strcmp(szOperandSize, "byte"))
			printf("0x%08X: (byte) 0x%02X\n", dwAddress, *(BYTE*)dwAddress);
		else if (!strcmp(szOperandSize, "range")) {
			if (iRange == 0)
				iRange = 16;

			printf("0x%08X: ", dwAddress);

			for (int i = 0; i < iRange; i++)
				printf("%02X ", *(BYTE*)(dwAddress + i));

			printf("\n");
		} else {
			printf("Invalid argument.\n");
		}
		return TRUE;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		return TRUE;
	}

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdShowMicrosim(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != SC2KVERSION_1996) {
		printf("Command only available when attached to 1996 Special Edition.\n");
		return TRUE;
	}

	if (*(szArguments + 8) == '\0' || *(szArguments + 9) == '\0' || !strcmp(szArguments + 9, "?")) {
		printf(
			"  show microsim <id>   Show specific microsim data\n"
			"  show microsim list   Show list of provisioned microsims\n");
		return TRUE;
	}

	if (!strcmp(szArguments + 9, "list")) {
		printf("Provisioned microsims:\n");
		for (int i = 0; i < 150; i++)
			if (pMicrosimArr[i].bTileID != TILE_CLEAR)
				printf("  %i: bTileID = %u\n", i, pMicrosimArr[i].bTileID);
		printf("\n");
		return TRUE;
	}

	int iMicrosimID = 0;
	if (!strcmp(szArguments + 9, "bus")) {
		iMicrosimID = 1;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "rail")) {
		iMicrosimID = 2;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "subway")) {
		iMicrosimID = 3;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "wind")) {
		iMicrosimID = 4;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "hydro")) {
		iMicrosimID = 5;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "bigpark")) {
		iMicrosimID = 6;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "museum")) {
		iMicrosimID = 7;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "library")) {
		iMicrosimID = 8;
		goto skipscanf;
	}
	if (!strcmp(szArguments + 9, "marina")) {
		iMicrosimID = 9;
		goto skipscanf;
	}

	if (sscanf_s(szArguments + 9, "%i", &iMicrosimID)) {
		if (iMicrosimID >= 0 && iMicrosimID < 150) {
skipscanf:
			int iTileID = pMicrosimArr[iMicrosimID].bTileID;
			printf(
				"Microsim %i:\n"
				"  Tile/Building: %s (%i / 0x%02X)\n"
				"  Data (Byte):   %i\n"
				"  Data (Word 1): %i\n"
				"  Data (Word 2): %i\n"
				"  Data (Word 3): %i\n", iMicrosimID, szTileNames[iTileID], iTileID, iTileID, pMicrosimArr[iMicrosimID].bMicrosimData[0],
				*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[1]), *(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[3]), *(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[5]));
			return TRUE;
		}
	}

	printf("Invalid argument.\n");
	return TRUE;
}

static const char* GetMidiDeviceTechnologyString(WORD wTechnology) {
	switch (wTechnology) {
	case MOD_MIDIPORT:
		return "Hardware MIDI port";
	case MOD_SYNTH:
		return "Hardware synthesizer";
	case MOD_SQSYNTH:
		return "Square wave synthesizer";
	case MOD_FMSYNTH:
		return "FM synthesizer";
	case MOD_MAPPER:
		return "Microsoft MIDI mapper";
	case MOD_WAVETABLE:
		return "Wavetable synthesizer";
	case MOD_SWSYNTH:
		return "Software synthesizer";
	default:
		return "Unknown";
	}
}

BOOL ConsoleCmdShowSound(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != SC2KVERSION_1996) {
		printf("Command only available when attached to 1996 Special Edition.\n");
		return TRUE;
	}

	if (*(szArguments + 5) == '\0' || *(szArguments + 6) == '\0' || !strcmp(szArguments + 6, "?")) {
		printf(
			"  show sound buffers   Dump loaded WAV buffers\n"
			"  show sound midi      Show all MIDI devices\n"
			"  show sound songs     Show the current song list\n");
		return TRUE;
	}

	if (!strcmp(szArguments + 6, "buffers")) {
		printf("Loaded WAV buffers:\n");
		int i = 0;
		for (const auto& iter : mapSoundBuffers)
			printf("  %i: <0x%08X>   %i.wav   (reloads: %i)\n", i++, iter.first, iter.second.iSoundID, iter.second.iReloadCount);
		return TRUE;
	}

	if (!strcmp(szArguments + 6, "midi")) {
		printf("MIDI devices (max %u):\n", midiOutGetNumDevs());
		int maxdevs = midiOutGetNumDevs();
		for (int i = -1; i < maxdevs; i++) {
			MIDIOUTCAPS moc;
			midiOutGetDevCaps(i, &moc, sizeof(MIDIOUTCAPS));
			printf(
				"  Device %i:\n"
				"    Product ID:   %04X:%04X\n"
				"    Name:         %s\n"
				"    Technology:   %s\n", i, moc.wMid, moc.wPid, moc.szPname, GetMidiDeviceTechnologyString(moc.wTechnology));
		}
		return TRUE;
	}

	if (!strcmp(szArguments + 6, "songs")) {
		extern int iCurrentSong;
		printf("Current song list: ");
		for (int i = 0; i < (int)vectorRandomSongIDs.size(); i++)
			printf("%i%s ", vectorRandomSongIDs[i], (i == iCurrentSong ? "*" : ""));
		printf("\n");
		return TRUE;
	}
	
	printf("Invalid argument.\n");
	return TRUE;
}

static BOOL ConsoleCmdShowTest(const char* szCommand, const char* szArguments) {
	return TRUE;
}

BOOL ConsoleCmdShowTile(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != SC2KVERSION_1996) {
		printf("Command only available when attached to 1996 Special Edition.\n");
		return TRUE;
	}

	if (*(szArguments + 4) == '\0' || *(szArguments + 5) == '\0' || !strcmp(szArguments + 5, "?")) {
		printf(
			"Usage:\n"
			"  show tile <x> <y>\n");
		return TRUE;
	}

	int iTileX = -1, iTileY = -1;
	sscanf_s(szArguments + 5, "%i %i", &iTileX, &iTileY);

	if (iTileX >= 0 && iTileX < 128 && iTileY >= 0 && iTileY < 128) {
		int iTileID = dwMapXBLD[iTileX]->iTileID[iTileY];

		char szXBITFormatted[256] = { 0 };
		if (dwMapXBIT[iTileX]->b[iTileY].iPowerable)
			strcat_s(szXBITFormatted, 256, "powerable ");
		if (dwMapXBIT[iTileX]->b[iTileY].iPowered)
			strcat_s(szXBITFormatted, 256, "powered ");
		if (dwMapXBIT[iTileX]->b[iTileY].iPiped)
			strcat_s(szXBITFormatted, 256, "piped ");
		if (dwMapXBIT[iTileX]->b[iTileY].iWatered)
			strcat_s(szXBITFormatted, 256, "watered ");
		if (dwMapXBIT[iTileX]->b[iTileY].iXVALMask)
			strcat_s(szXBITFormatted, 256, "xvalmask ");
		if (dwMapXBIT[iTileX]->b[iTileY].iWater)
			strcat_s(szXBITFormatted, 256, "water ");
		if (dwMapXBIT[iTileX]->b[iTileY].iRotated)
			strcat_s(szXBITFormatted, 256, "rotated ");
		if (dwMapXBIT[iTileX]->b[iTileY].iSaltWater)
			strcat_s(szXBITFormatted, 256, "saltwater ");
		if (szXBITFormatted[0] == '\0')
			strcpy_s(szXBITFormatted, 256, "none");
		if (szXBITFormatted[strlen(szXBITFormatted) - 1] == ' ')
			szXBITFormatted[strlen(szXBITFormatted) - 1] = '\0';

		printf(
			"Tile (%i, %i):\n"
			"  iTileID: %s (%i / 0x%02X)\n"
			"  Zone:    %s\n"
			"  XBIT:    0x%02X (%s)\n", iTileX, iTileY, szTileNames[iTileID], iTileID, iTileID, GetZoneName(dwMapXZON[iTileX]->b[iTileY].iZoneType), dwMapXBIT[iTileX]->b[iTileY], szXBITFormatted);
		return TRUE;
	}

	printf("Invalid argument.\n");
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
		"sc2kfix version %s - https://sc2kfix.net\n"
		"Plugin build info: %s\n"
		"SimCity 2000 version: %s\n"
		"Plugin loaded at 0x%08X\n"
		"City days: %u\n", szSC2KFixVersion, szSC2KFixBuildInfo, szSC2KVersion, (DWORD)hSC2KFixModule, dwCityDays);
	printf("\n");

	return TRUE;
}

// COMMAND: set [...]

BOOL ConsoleCmdSet(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  [un]set debug [...]   Enable debugging output\n"
			"  [un]set tile [...]    Modify tile parameters\n");
		return TRUE;
	}

	BOOL bOperation = TRUE;
	if (!strcmp(szCommand, "unset"))
		bOperation = FALSE;

	if (!strncmp(szArguments, "debug ", 6))
		return ConsoleCmdSetDebug(szCommand, szArguments + 6);

	if (!strcmp(szArguments, "tile") || !strncmp(szArguments, "tile ", 5))
		return ConsoleCmdSetTile(szCommand, szArguments + 5);

	if (!strcmp(szArguments, "undocumented")) {
		bConsoleUndocumentedMode = bOperation;
		return TRUE;
	}

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  [un]set debug misc    Enable miscellaneous hook debugging\n"
			"  [un]set debug mci     Enable MCI debugging\n"
			"  [un]set debug snd     Enable WAV debugging\n"
			"  [un]set debug timer   Enable timer debugging\n");
		return TRUE;
	}

	// FIRE IN THE HOLE
	DWORD bOperation = DEBUG_FLAGS_EVERYTHING;
	if (!strcmp(szCommand, "unset"))
		bOperation = FALSE;
	
	if (!strcmp(szArguments, "mci")) {
		mci_debug = bOperation;
		printf("%sabled MCI debugging.\n", (bOperation ? "En" : "Dis"));
	} else if (!strcmp(szArguments, "snd")) {
		snd_debug = bOperation;
		printf("%sabled WAV debugging.\n", (bOperation ? "En" : "Dis"));
	} else if (!strcmp(szArguments, "timer")) {
		timer_debug = bOperation;
		printf("%sabled timer debugging.\n", (bOperation ? "En" : "Dis"));
	} else if (!strcmp(szArguments, "misc")) {
		mischook_debug = bOperation;
		printf("%sabled misc hook debugging.\n", (bOperation ? "En" : "Dis"));
	} else {
		printf("Invalid argument.\n");
	}
	return TRUE;
}

BOOL ConsoleCmdSetTile(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  [un]set tile <x> <y> rotate    Enable rotate flag on tile\n");
		return TRUE;
	}

	DWORD bOperation = TRUE;
	if (!strcmp(szCommand, "unset"))
		bOperation = FALSE;

	char szTileOperation[12] = { 0 };
	int iTileX = -1, iTileY = -1;
	sscanf_s(szArguments, "%i %i %s", &iTileX, &iTileY, szTileOperation, sizeof(szTileOperation));

	if (iTileX >= 0 && iTileX < 128 && iTileY >= 0 && iTileY < 128) {
		if (!strcmp(szTileOperation, "rotate")) {
			dwMapXBIT[iTileX]->b[iTileY].iRotated = bOperation;
			return TRUE;
		}
	}
	
	printf("Invalid argument.\n");
	return TRUE;
}

// CONSOLE THREAD

DWORD WINAPI ConsoleThread(LPVOID lpParameter) {
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	for (;;) {
		if (bConsoleUndocumentedMode)
			printf("# ");
		else
			printf("> ");

		gets_s(szCmdBuf, 256);
		if (!ConsoleEvaluateCommand(szCmdBuf, TRUE))
			printf("Invalid command.\n");
	}
}

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		if (iConsoleScriptNest)
			printf("\nScript halted due to Control-C interrupt.\n\n");
		iConsoleScriptNest = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL ConsoleEvaluateCommand(const char* szCommandLine, BOOL bInteractive) {
	if (*szCommandLine == '\r' || *szCommandLine == '\n' || !*szCommandLine)
		return TRUE;

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
			if (bInteractive && fpConsoleCommands[i].iUndocumented == CONSOLE_COMMAND_SCRIPTONLY)
				return FALSE;

			BOOL bRetval = fpConsoleCommands[i].fpProc(fpConsoleCommands[i].szCommand, szArguments);
			return bRetval;
		}
	}

	return FALSE;
}
