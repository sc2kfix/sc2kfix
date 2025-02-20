// sc2kfix console.cpp: exactly what it says on the tin
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <mmsystem.h>

#include <sc2kfix.h>
#include "resource.h"

#ifdef CONSOLE_ENABLED
BOOL bConsoleEnabled = TRUE;
#else 
BOOL bConsoleEnabled = FALSE;
#endif

HANDLE hConsoleThread;
char szCmdBuf[256] = { 0 };
BOOL bConsoleUndocumentedMode = FALSE;

static BOOL ConsoleCmdShowTest(const char* szCommand, const char* szArguments);
BOOL ConsoleCmdSettings(const char* szCommand, const char* szArguments);

console_command_t fpConsoleCommands[] = {
	{ "?", ConsoleCmdHelp, CONSOLE_COMMAND_ALIAS, "" },
	{ "help", ConsoleCmdHelp, CONSOLE_COMMAND_DOCUMENTED, "Display this help" },
	{ "set", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "settings", ConsoleCmdSettings, CONSOLE_COMMAND_DOCUMENTED, "Open sc2kfix settings dialog" },
	{ "show", ConsoleCmdShow, CONSOLE_COMMAND_DOCUMENTED, "Display various game and plugin information" },
	{ "unset", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
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

// COMMAND: settings

BOOL ConsoleCmdSettings(const char* szCommand, const char* szArguments) {
	DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS), NULL, SettingsDialogProc);
	return TRUE;
}

BOOL ConsoleCmdShow(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  show debug        Display enabled debugging options\n"
			"  show memory ...   Display memory contents\n"
			"  show sound        Display sound info\n"
			"  show version      Display sc2kfix version info\n");
		return TRUE;
	}

	if (!strcmp(szArguments, "debug"))
		return ConsoleCmdShowDebug(szCommand, szArguments);

	if (!strcmp(szArguments, "memory") || !strncmp(szArguments, "memory ", 7))
		return ConsoleCmdShowMemory(szCommand, szArguments);

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
			"\n"
			"    <address>: Address in hexadecimal\n"
			"    [operand_size]: Optional, one of: { byte, word, dword, range }\n"
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
		ConsoleLog(LOG_ERROR, "Segmentation fault caught. Don't do that again.\n");
		return TRUE;
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
	extern WORD wMilitaryBaseX, wMilitaryBaseY;
	if (bMilitaryBaseType > 1)
		printf("Military base located at (%u, %u)\n", wMilitaryBaseX, wMilitaryBaseY);
	else
		printf("Military base not built in city.\n");

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

		printf(
			"Tile (%i, %i):\n"
			"  iTileID: %i\n"
			"  Zone: %s\n"
			"  XBIT: 0x%02X\n", iTileX, iTileY, iTileID, GetZoneName(dwMapXZON[iTileX]->b[iTileY].iZoneType), dwMapXBIT[iTileX]->b[iTileY]);
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
		"sc2kfix version %s - https://github.com/araxestroy/sc2kfix\n"
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
