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

#if !NOKUROKO
#include <kuroko/kuroko.h>
#include <kuroko/util.h>
#endif

#ifdef CONSOLE_ENABLED
BOOL bConsoleEnabled = TRUE;
#else 
BOOL bConsoleEnabled = FALSE;
#endif

HANDLE hConsoleThread;
#if !NOKUROKO
DWORD dwConsoleThreadID;
#endif
char szCmdBuf[256] = { 0 };
BOOL bConsoleUndocumentedMode = FALSE;

static BOOL ConsoleCmdShowTest(const char* szCommand, const char* szArguments);

console_command_t fpConsoleCommands[] = {
	{ "?", ConsoleCmdHelp, CONSOLE_COMMAND_ALIAS, "" },
	{ "clear", ConsoleCmdClear, CONSOLE_COMMAND_DOCUMENTED, "Clear screen" },
	{ "echo", ConsoleCmdEcho, CONSOLE_COMMAND_DOCUMENTED, "Print to console" },
	{ "echo!", ConsoleCmdEcho, CONSOLE_COMMAND_UNDOCUMENTED, "Print to console without newline" },
	{ "help", ConsoleCmdHelp, CONSOLE_COMMAND_DOCUMENTED, "Display this help" },
#if !NOKUROKO
	{ "run", ConsoleCmdRun, CONSOLE_COMMAND_DOCUMENTED, "Run Kuroko code" },
#endif
	{ "set", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "show", ConsoleCmdShow, CONSOLE_COMMAND_DOCUMENTED, "Display various game and plugin information" },
	{ "unset", ConsoleCmdSet, CONSOLE_COMMAND_DOCUMENTED, "Modify game and plugin behaviour" },
	{ "wait", ConsoleCmdWait, CONSOLE_COMMAND_UNDOCUMENTED, "Wait for a number of milliseconds" },
};

void ConsoleScriptSleep(DWORD dwMilliseconds) {
	int i = dwMilliseconds / 100;
	do
		Sleep(100);
	while (--i);
}

// COMMAND: run ...

#if !NOKUROKO
BOOL ConsoleCmdRun(const char* szCommand, const char* szArguments) {
	MSG msg;
	std::string strPossibleScriptName;
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  run kuroko       Enters the Kuroko REPL\n"
			"  run <filename>   Executes a file as a Kuroko module\n");
		return TRUE;
	}

	// Start the Kuroko REPL if requested
	if (!strcmp(szArguments, "kuroko")) {
		if (bKurokoVMInitialized) {
			printf("\n");
			PostThreadMessage(dwKurokoThreadID, WM_KUROKO_REPL, NULL, NULL);
			GetMessage(&msg, NULL, 0, 0);
			printf("\nKuroko REPL exited, returning control to console thread.\n");
		}
		return TRUE;
	}

	// Try to find the script we want
	strPossibleScriptName = szArguments;
	if (!FileExists(strPossibleScriptName.c_str())) {
		strPossibleScriptName += ".krk";
		if (!FileExists(strPossibleScriptName.c_str())) {
			ConsoleLog(LOG_ERROR, "CORE: Couldn't find script %s.\n", szArguments);
			return TRUE;
		}
	}

	// Execute through Kuroko
	if (bKurokoVMInitialized) {
		char* szFilename = (char*)malloc(strPossibleScriptName.length() + 1);
		if (!szFilename)
			return TRUE;

		strcpy_s(szFilename, strPossibleScriptName.length() + 1, strPossibleScriptName.c_str());
		PostThreadMessage(dwKurokoThreadID, WM_KUROKO_FILE, (WPARAM)szFilename, NULL);
		GetMessage(&msg, NULL, 0, 0);
		free(szFilename);
	}
	return TRUE;
}
#endif

BOOL ConsoleCmdClear(const char* szCommand, const char* szArguments) {
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "\x1b[2J\x1b[0;0H", sizeof("\x1b[2J\x1b[0;0H"), NULL, NULL);
	return TRUE;
}

BOOL ConsoleCmdEcho(const char* szCommand, const char* szArguments) {
	printf("%s%s", szArguments, (!strcmp(szCommand, "echo!") ? "" : "\n"));
	return TRUE;
}

BOOL ConsoleCmdWait(const char* szCommand, const char* szArguments) {
	int iMilliseconds = 0;
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
			"  show mods           Display loaded mods\n"
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

	if (!strcmp(szArguments, "mods") || !strncmp(szArguments, "mods ", 5))
		return ConsoleCmdShowMods(szCommand, szArguments);

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

	if (guzzardo_debug)
		printf("GUZZ=0x%08X ", guzzardo_debug);
	if (mci_debug)
		printf("MCI=0x%08X ", mci_debug);
	if (military_debug)
		printf("MIL=0x%08X ", military_debug);
	if (mischook_debug)
		printf("MISC=0x%08X ", mischook_debug);
	if (modloader_debug)
		printf("MODS=0x%08X ", modloader_debug);
	if (mov_debug)
		printf("MOV=0x%08X ", mov_debug);
	if (mus_debug)
		printf("MUS=0x%08X ", mus_debug);
	if (registry_debug)
		printf("REG=0x%08X ", registry_debug);
	if (sc2x_debug)
		printf("SC2X=0x%08X ", sc2x_debug);
	if (snd_debug)
		printf("SND=0x%08X ", snd_debug);
	if (sprite_debug)
		printf("SPR=0x%08X ", sprite_debug);
	if (timer_debug)
		printf("TIMER=0x%08X ", timer_debug);
	if (updatenotifier_debug)
		printf("UPD=0x%08X ", updatenotifier_debug);

	printf("\n");

	return TRUE;
}

BOOL ConsoleCmdShowMemory(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != VERSION_SC2K_1996) {
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

	if (!dwAddress) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		return TRUE;
	}

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
	if (dwDetectedVersion != VERSION_SC2K_1996) {
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
		for (int i = 0; i <= MICROSIMID_MAX; i++)
			if (GetMicroSimulatorTileID(i) != TILE_CLEAR)
				printf("  %i: bTileID = %u\n", i, GetMicroSimulatorTileID(i));
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
		if (iMicrosimID >= MICROSIMID_MIN && iMicrosimID <= MICROSIMID_MAX) {
skipscanf:
			BYTE iTileID = GetMicroSimulatorTileID(iMicrosimID);
			printf(
				"Microsim %i:\n"
				"  Tile/Building: %s (%u / 0x%02X)\n"
				"  Data Stat0 (Byte):   %u\n"
				"  Data Stat1 (Word 1): %u\n"
				"  Data Stat2 (Word 2): %u\n"
				"  Data Stat3 (Word 3): %u\n", iMicrosimID, szTileNames[iTileID], iTileID, iTileID, GetMicroSimulatorStat0(iMicrosimID),
				GetMicroSimulatorStat1(iMicrosimID), GetMicroSimulatorStat2(iMicrosimID), GetMicroSimulatorStat3(iMicrosimID));
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

BOOL ConsoleCmdShowMods(const char* szCommand, const char* szArguments) {
	BOOL bDetail = FALSE;
	if (*(szArguments + 4) == '\0' || *(szArguments + 5) == '\0')
		bDetail = FALSE;
	else if (!strcmp(szArguments + 5, "detail"))
		bDetail = TRUE;
	else if (!strcmp(szArguments + 5, "?")) {
		printf(
			"  show mods          Show native code mod list\n"
			"  show mods detail   Show native code mod list verbosely\n");
		return TRUE;
	} else
		return FALSE;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	printf("%d native code mods loaded:\n", mapLoadedNativeMods.size());
	for (auto stNativeMod : mapLoadedNativeMods) {

		const char* szModVersion = _strdup(FormatVersion(stNativeMod.second.iModVersionMajor, stNativeMod.second.iModVersionMinor, stNativeMod.second.iModVersionPatch));
		const char* szModMinimumVersion = _strdup(FormatVersion(stNativeMod.second.iMinimumVersionMajor, stNativeMod.second.iMinimumVersionMinor, stNativeMod.second.iMinimumVersionPatch));

		printf(
			"  %s version %s (0x%08X)\n"
			"    Mod Name:             %s\n"
			"    Author:               %s\n"
			"    Req. sc2kfix version: %s\n"
			"    Description:          %s\n",
			stNativeMod.second.szModShortName, szModVersion, (INT_PTR)stNativeMod.first,
			stNativeMod.second.szModName,
			stNativeMod.second.szModAuthor,
			szModMinimumVersion,
			WordWrap(stNativeMod.second.szModDescription, csbi.dwSize.X, 26));
		if (bDetail) {
			printf("    Hooks:\n");
			for (auto stHook : mapLoadedNativeModHooks[stNativeMod.first])
				printf("      %s (pri %d)\n", stHook.szHookName, stHook.iHookPriority);
		}
		printf("\n");
	}
	return TRUE;
}

BOOL ConsoleCmdShowSound(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != VERSION_SC2K_1996) {
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
	printf("%s\n", jsonSettingsCore.dump().c_str());
	return TRUE;
}

BOOL ConsoleCmdShowTile(const char* szCommand, const char* szArguments) {
	if (dwDetectedVersion != VERSION_SC2K_1996) {
		printf("Command only available when attached to 1996 Special Edition.\n");
		return TRUE;
	}

	if (*(szArguments + 4) == '\0' || *(szArguments + 5) == '\0' || !strcmp(szArguments + 5, "?")) {
		printf(
			"Usage:\n"
			"  show tile <x> <y>\n");
		return TRUE;
	}

	__int16 iTileX = -1, iTileY = -1;
	sscanf_s(szArguments + 5, "%hi %hi", &iTileX, &iTileY);

	if (iTileX >= 0 && iTileX < GAME_MAP_SIZE && iTileY >= 0 && iTileY < GAME_MAP_SIZE) {
		BYTE iTileID = GetTileID(iTileX, iTileY);

		char szXBITFormatted[256] = { 0 };
		if (XBITReturnIsPowerable(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "powerable ");
		if (XBITReturnIsPowered(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "powered ");
		if (XBITReturnIsPiped(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "piped ");
		if (XBITReturnIsWatered(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "watered ");
		if (XBITReturnIsMark(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "mark ");
		if (XBITReturnIsWater(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "water ");
		if (XBITReturnIsFlipped(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "flipped ");
		if (XBITReturnIsSaltWater(iTileX, iTileY))
			strcat_s(szXBITFormatted, 256, "saltwater ");
		if (szXBITFormatted[0] == '\0')
			strcpy_s(szXBITFormatted, 256, "none");
		if (szXBITFormatted[strlen(szXBITFormatted) - 1] == ' ')
			szXBITFormatted[strlen(szXBITFormatted) - 1] = '\0';

		printf(
			"Tile (%i, %i):\n"
			"  iTileID: %s (%i / 0x%02X)\n"
			"  Zone:    %s\n"
			"  XBIT:    0x%02X (%s)\n", iTileX, iTileY, szTileNames[iTileID], iTileID, iTileID, GetZoneName(XZONReturnZone(iTileX, iTileY)), XBITReturnMask(iTileX, iTileY), szXBITFormatted);
		return TRUE;
	}

	printf("Invalid argument.\n");
	return TRUE;
}

BOOL ConsoleCmdShowVersion(const char* szCommand, const char* szArguments) {
	const char* szProgramName = "SimCity 2000";
	const char* szProgramVersion = "unknown";
	if (dwSC2kFixMode == SC2KFIX_MODE_SC2K) {
		switch (dwDetectedVersion) {
		case VERSION_SC2K_1995:
			szProgramVersion = "1995 CD Collection";
			break;
		case VERSION_SC2K_1996:
			szProgramVersion = "1996 Special Edition";
			break;
		}
	}
	else if (dwSC2kFixMode == SC2KFIX_MODE_SC2KDEMO) {
		if (dwDetectedVersion == VERSION_SC2K_DEMO)
			szProgramVersion = "Interactive Demo";
	}
	else if (dwSC2kFixMode == SC2KFIX_MODE_SCURK) {
		szProgramName = "SCURK";
		switch (dwDetectedVersion) {
		case VERSION_SCURK_1996SE:
			szProgramVersion = "1996 Special Edition";
			break;
		}
	}
	else
		szProgramName = "Unknown";

#if !NOKUROKO
	KrkValue kuroko_version;
	krk_tableGet_fast(&vm.system->fields, S("version"), &kuroko_version);
#endif

	// AF - the separation comma positioning in this case is deliberate
	// in order to be a bit more "friendly" concerning missing or varied
	// end-arguments depending on the build.
	printf(
		"sc2kfix version %s - https://sc2kfix.net\n"
		"Plugin build info: %s\n"
		"%s version: %s\n"
		"Plugin loaded at 0x%08X\n"
#if !NOKUROKO
		"Kuroko version: Kuroko %s\n" 
#endif
		,szSC2KFixVersion 
		,szSC2KFixBuildInfo
		,szProgramName
		,szProgramVersion
		,(DWORD)hSC2KFixModule 
#if !NOKUROKO
		,AS_CSTRING(kuroko_version)
#endif
	);

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

#define SETDEBUGOP(keyword, var, description) \
	if (!strcmp(szArguments, keyword)) { \
		var ## _debug = bOperation; \
		printf("%sabled " description " debugging.\n", (bOperation ? "En" : "Dis")); \
	}

BOOL ConsoleCmdSetDebug(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  [un]set debug guzzardo    Enable cousin Vinnie debugging\n"
			"  [un]set debug mci         Enable MCI debugging\n"
			"  [un]set debug military    Enable military base algorithm debugging\n"
			"  [un]set debug mischook    Enable miscellaneous debugging\n"
			"  [un]set debug modloader   Enable native code mod loader debugging\n"
			"  [un]set debug mus         Enable new music engine debugging\n"
			"  [un]set debug registry    Enable registry override hooks debugging\n"
			"  [un]set debug sc2x        Enable SC2X format and load/save debugging\n"
			"  [un]set debug snd         Enable sound hook debugging\n"
			"  [un]set debug sprite      Enable sprite and tileset hook debugging\n"
			"  [un]set debug timer       Enable timer hook debugging\n"
			"  [un]set debug update      Enable update notifier debugging\n");
		return TRUE;
	}

	// FIRE IN THE HOLE
	DWORD bOperation = DEBUG_FLAGS_EVERYTHING;
	if (!strcmp(szCommand, "unset"))
		bOperation = FALSE;

	SETDEBUGOP("guzzardo", guzzardo, "cousin Vinnie")
	else SETDEBUGOP("mci", mci, "MCI")
	else SETDEBUGOP("military", military, "military base algorithm")
	else SETDEBUGOP("mischook", mischook, "miscellaneous")
	else SETDEBUGOP("modloader", modloader, "native code mod loader")
	else SETDEBUGOP("mus", mus, "new music engine")
	else SETDEBUGOP("registry", registry, "registry override hooks")
	else SETDEBUGOP("sc2x", sc2x, "SC2X format and load/save")
	else SETDEBUGOP("snd", snd, "sound hook")
	else SETDEBUGOP("sprite", sprite, "sprite and tileset hook")
	else SETDEBUGOP("timer", timer, "timer hook")
	else SETDEBUGOP("update", updatenotifier, "update notifier")
	else
		printf("Invalid argument.\n");

	return TRUE;
}

BOOL ConsoleCmdSetTile(const char* szCommand, const char* szArguments) {
	if (!szArguments || !*szArguments || !strcmp(szArguments, "?")) {
		printf(
			"  [un]set tile <x> <y> flip    Enable flip flag on tile\n");
		return TRUE;
	}

	DWORD bOperation = TRUE;
	if (!strcmp(szCommand, "unset"))
		bOperation = FALSE;

	char szTileOperation[12] = { 0 };
	__int16 iTileX = -1, iTileY = -1;
	sscanf_s(szArguments, "%hi %hi %s", &iTileX, &iTileY, szTileOperation, sizeof(szTileOperation));

	if (iTileX >= 0 && iTileX < GAME_MAP_SIZE && iTileY >= 0 && iTileY < GAME_MAP_SIZE) {
		if (!strcmp(szTileOperation, "flip")) {
			if (bOperation)
				XBITSetBits(iTileX, iTileY, XBIT_FLIPPED);
			else
				XBITClearBits(iTileX, iTileY, XBIT_FLIPPED);
			return TRUE;
		}
	}
	
	printf("Invalid argument.\n");
	return TRUE;
}

// CONSOLE THREAD

DWORD WINAPI ConsoleThread(LPVOID lpParameter) {
	Sleep(200);
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
