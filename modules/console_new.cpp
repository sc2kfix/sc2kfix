// sc2kfix modules/console_new.cpp: sc2kfix console 2.0
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Notes: 2026-04-21 (@araxestroy)
//
// I'm very, very sorry.


#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <io.h>
#include <signal.h>
#include <conio.h>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include <lua_glue.h>
#include <commandtree.hpp>
#include "../resource.h"

#define CLI_DEBUG 0

#if CLI_DEBUG == 0
#define debug_printf drop_args
#else
#define debug_printf printf
#endif

#define printf_red(s, ...) printf(VT100_COLOUR_RED s VT100_DEFAULT, __VA_ARGS__)
#define printf_lightred(s, ...) printf(VT100_COLOUR_BRIGHT_RED s VT100_DEFAULT, __VA_ARGS__)
#define printf_yellow(s, ...) printf(VT100_COLOUR_YELLOW s VT100_DEFAULT, __VA_ARGS__)
#define printf_lightblue(s, ...) printf(VT100_COLOUR_BRIGHT_BLUE s VT100_DEFAULT, __VA_ARGS__)

void drop_args(...) {
	return;
}

bool bConsoleInLuaREPL = false;
bool bConsoleElevatedMode = false;
bool bConsoleKeepCommandBuffer = false;
console::CommandTree treeConsoleCommands;

static BYTE AttemptSafeReadByte(BYTE* address) {
	__try {
		return *address;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		throw std::out_of_range("segfault");
		return 0;
	}
}

static WORD AttemptSafeReadWord(WORD* address) {
	__try {
		return *address;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		throw std::out_of_range("segfault");
		return 0;
	}
}

static DWORD AttemptSafeReadDword(DWORD* address) {
	__try {
		return *address;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		throw std::out_of_range("segfault");
		return 0;
	}
}

static uint64_t AttemptSafeReadQword(uint64_t* address) {
	__try {
		return *address;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		throw std::out_of_range("segfault");
		return 0;
	}
}

static void AttemptSafeMemcpy(BYTE* dest, BYTE* src, size_t bytes) {
	__try {
		while (bytes--)
			*dest++ = *src++;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught. Don't do that again.\n");
		throw std::out_of_range("segfault");
		return;
	}
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

void PrintAlignedStringMap(std::map<std::string, std::string> mapStr, int iPrefixSpaces = 3) {
	std::map<std::string, std::string> mapOutput;
	std::string strPrefixSpaces(iPrefixSpaces, ' ');
	size_t iLeftAlign = 0;

	for (auto s : mapStr) {
		mapOutput[s.first] = s.second;
		if (iLeftAlign < s.first.size())
			iLeftAlign = s.first.size();
	}

	for (auto s : mapOutput)
		printf("%s%s%s%s\n", strPrefixSpaces.c_str(), s.first.c_str(), std::string(3 + iLeftAlign - s.first.size(), ' ').c_str(), s.second.c_str());
}

bool ConsoleCommandClear(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	// It is the year 2026. Don't let anyone tell you Windows doesn't support VT100 control codes.
	printf("\x1b[2J\x1b[0;0H");

	return true;
}

bool ConsoleCommandFixupThingsClear(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	int iIndex = 0;

	if (dwDetectedVersion != VERSION_SC2K_1996) {
		printf_yellow("Command only available when attached to 1996 Special Edition.\n");
		return true;
	}
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"all", "Remove every Thing entity on the map (potentially dangerous!)"},
				{"index <index>", "XTHG index of Thing to remove"},
				{"type <type>", "Remove all Things of a specific type"},
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	// Arguments are required for this command
	if (args.size() == 0) {
		bConsoleKeepCommandBuffer = true;
		return false;
	}

	// Parse arguments
	for (size_t i = 0; i < args.size(); i++) {
		// all
		if (args[i] == "all") {
			// Danger, Will Robinson!
			DeleteMapThingByIdx_SC2K1996(-1);
			ConsoleLog(LOG_INFO, "Cleared all things by console command.\n");
			continue;
		}

		// index <index>
		if (args[i] == "index") {
			if (++i >= args.size())
				return false;

			if (!sscanf_s(args[i].c_str(), "%u", &iIndex))
				return false;

			if (iIndex >= MIN_THING_IDX && iIndex <= MAX_THING_IDX) {
				DeleteMapThingByIdx_SC2K1996(iIndex);
				ConsoleLog(LOG_INFO, "Cleared thing index %d by console command.\n", iIndex);
			} else
				return false;

			continue;
		}
		// type <type>
		if (args[i] == "type") {
			if (++i >= args.size())
				return false;

			if (args[i] == "plane") {
				DeleteAllPlanes_SC2K1996();
				continue;
			} else if (args[i] == "helicopter") {
				DeleteAllCopters_SC2K1996();
				continue;
			} else if (args[i] == "cargoship") {
				DeleteAllShips_SC2K1996();
				continue;
			} else if (args[i] == "sailboat") {
				DeleteAllSailboats_SC2K1996();
				continue;
			} else if (args[i] == "train") {
				DeleteAllTrains_SC2K1996();
				continue;
			} else if (args[i] == "maxisman") {
				DeleteAllMaxisMen_SC2K1996();
				continue;
			} else if (args[i] == "monster") {
				DeleteAllMonsters_SC2K1996();
				continue;
			} else if (args[i] == "tornado") {
				DeleteAllTornadoes_SC2K1996();
				continue;
			} else if (args[i] == "policedeploy") {
				DeleteAllPoliceDeploys_SC2K1996();
				continue;
			} else if (args[i] == "firedeploy") {
				DeleteAllFireDeploys_SC2K1996();
				continue;
			} else if (args[i] == "militarydeploy") {
				DeleteAllMilitaryDeploys_SC2K1996();
				continue;
			} else
				return false;
		}

		// Invalid arugment, bail out
		return false;
	}

	return true;
}

#define SETDEBUGOP(keyword, var, description) \
	if (args[i] == keyword || bSetAll) { \
		var ## _debug = dwOperation; \
		printf("%sabled " description " debugging.\n", (dwOperation ? "En" : "Dis")); \
		bSuccess = true; \
	}

bool ConsoleCommandSetDebug(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	DWORD dwOperation = DEBUG_FLAGS_NONE;
	bool bSetAll = false;

	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"guzzardo", "Enable/disable cousin Vinnie debugging"},
				{"mci", "Enable/disable MCI debugging"},
				{"military", "Enable/disable military base algorithm debugging"},
				{"mischook", "Enable/disable miscellaneous debugging"},
				{"modloader", "Enable/disable native code mod loader debugging"},
				{"mus", "Enable/disable music engine debugging"},
				{"registry", "Enable/disable registry override hooks debugging"},
				{"sc2x", "Enable/disable SC2X format and load/save debugging"},
				{"snd", "Enable/disable sound hook debugging"},
				{"sprite", "Enable/disable sprite and tileset hook debugging"},
				{"timer", "Enable/disable timer hook debugging"},
				{"update", "Enable/disable update notifier debugging"}
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	std::string& strRootName = *((std::string*)iOptParam);
	if (strRootName == "set")
		dwOperation = DEBUG_FLAGS_EVERYTHING;
	else
		dwOperation = DEBUG_FLAGS_NONE;

	// Arguments are required for this command
	if (args.size() == 0) {
		bConsoleKeepCommandBuffer = true;
		return false;
	}

	// Parse arguments
	// To save on typing this one works a bit differently than other commands. A better example
	// of how to parse arguments would be the "show memory" or "show microsim" commands.
	for (size_t i = 0; i < args.size(); i++) {
		bool bSuccess = false;
		if (args[i] == "all") {
			bSetAll = true;
			i = args.size();
			bSuccess = true;
		}

		SETDEBUGOP("guzzardo", guzzardo, "cousin Vinnie")
		SETDEBUGOP("mci", mci, "MCI")
		SETDEBUGOP("military", military, "military base algorithm")
		SETDEBUGOP("mischook", mischook, "miscellaneous")
		SETDEBUGOP("modloader", modloader, "native code mod loader")
		SETDEBUGOP("mus", mus, "music engine")
		SETDEBUGOP("registry", registry, "registry override hooks")
		SETDEBUGOP("sc2x", sc2x, "SC2X format and load/save")
		SETDEBUGOP("snd", snd, "sound hook")
		SETDEBUGOP("sprite", sprite, "sprite and tileset hook")
		SETDEBUGOP("timer", timer, "timer hook")
		SETDEBUGOP("update", updatenotifier, "update notifier")

		// Invalid arugment, bail out
		if (!bSuccess)
			return false;
	}

	return true;
}

bool ConsoleCommandSetUndocumented(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	std::string& strRootName = *((std::string*)iOptParam);
	if (strRootName == "set")
		bConsoleElevatedMode = true;
	else
		bConsoleElevatedMode = false;
	return true;
}

bool ConsoleCommandShowAudioBuffers(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	int i = 0;
	for (const auto& iter : mapSoundBuffers)
		printf("  %i: <0x%08X>   %i.wav   (reloads: %i)\n", i++, iter.first, iter.second.iSoundID, iter.second.iReloadCount);
	return true;
}

bool ConsoleCommandShowAudioEngine(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	printf(
		"Audio engine: %s\n"
		"Music driver: %s\n",
		"old", jsonSettingsCore["sc2kfix"]["audio"]["music_driver"].ToString().c_str());
	return true;
}

bool ConsoleCommandShowAudioMidi(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

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
	return true;
}

bool ConsoleCommandShowAudioSongs(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	extern int iCurrentSong;
	printf("Current playlist: ");
	for (int i = 0; i < (int)vectorRandomSongIDs.size(); i++)
		printf("%i%s ", vectorRandomSongIDs[i], (i == iCurrentSong ? "*" : ""));
	printf("\n");
	return true;
}

bool ConsoleCommandRunLua(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"<[Enter]>", "Switches to the Lua REPL"},
				{"<filename>", "Executes a file as a standalone Lua script"},
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;
	
	if (args.size() == 0) {
		bConsoleInLuaREPL = true;
		LuaRunREPL();
		bConsoleInLuaREPL = false;
	} else if (args.size() > 1)
		return false;
	else {
		std::string strPossibleScriptName = args[0];
		if (!FileExists(strPossibleScriptName.c_str())) {
			strPossibleScriptName += ".lua";
			if (!FileExists(strPossibleScriptName.c_str())) {
				ConsoleLog(LOG_ERROR, "CORE: Couldn't find Lua script %s.\n", strPossibleScriptName.c_str());
				return true;
			}
		}

		// Spin up a new Lua VM, set up the glue logic, load the file, and run it.
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		luaL_dostring(L,
			"mod_info = {}\n"
			"mod_info.name = \"sc2kfix Lua REPL\"\n"
			"mod_info.shortname = \"repl\"\n");
		LuaGlueSetupState(L);
		luaL_dofile(L, strPossibleScriptName.c_str());
		lua_close(L);
	}
	
	return true;
}

bool ConsoleCommandRunTest(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	return true;
}

bool ConsoleCommandShowMemory(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	DWORD dwAddress = NULL;
	int iElementsCount = 1;
	bool bAddressScanned = false;
	int iBase = 16;

	BYTE b;
	WORD w;
	DWORD dw;
	uint64_t qw;

	// iOptParam is the element size (+1 for floating point when size == 4 or 8)
	bool bFloat = (iOptParam == 5 || iOptParam == 9);
	if (bFloat)
		iOptParam--;

	if (dwDetectedVersion != VERSION_SC2K_1996) {
		printf_yellow("Command only available when attached to 1996 Special Edition.\n");
		return true;
	}
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"<address>", "Address to examine in hexadecimal"},
				{"[count <count>]", "Number of elements to show (default 1)"},
				{"[decimal]", "Display integers in decimal instead of hex"},
				{"[octal]", "Display integers in octal instead of hex"},
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	// Arguments are required for this command
	if (args.size() == 0) {
		bConsoleKeepCommandBuffer = true;
		return false;
	}

	// Parse arguments
	for (size_t i = 0; i < args.size(); i++) {
		// [count <count>]
		if (args[i] == "count") {
			if (++i >= args.size())
				return false;

			if (!sscanf_s(args[i].c_str(), "%u", &iElementsCount))
				return false;

			continue;
		}

		// [decimal]
		if (args[i] == "decimal" && iBase == 16) {
			iBase = 10;
			continue;
		}

		// [octal]
		if (args[i] == "octal" && iBase == 16) {
			iBase = 8;
			continue;
		}

		// <address>
		if (!bAddressScanned && sscanf_s(args[i].c_str(), "%X", &dwAddress)) {
			bAddressScanned = true;
			continue;
		}

		// Invalid arugment, bail out
		return false;
	}

	// Do some quick sanity checking
	if (iElementsCount < 1 || !dwAddress)
		return false;

	try {
		// Display single items in both hexadecimal and decimal by default
		if (iElementsCount == 1) {
			printf("0x%08X: ", dwAddress);
			switch (iOptParam) {
			case 1:
				b = AttemptSafeReadByte((BYTE*)dwAddress);
				if (iBase == 10)
					printf("(byte) %u / %d\n", b, b);
				else if (iBase == 8)
					printf("(byte) %03o\n", b);
				else
					printf("(byte) 0x%02X / %d\n", b, b);
				break;
			case 2:
				w = AttemptSafeReadWord((WORD*)dwAddress);
				if (iBase == 10)
					printf("(word) %u / %d\n", w, w);
				else if (iBase == 8)
					printf("(word) %06o\n", w);
				else
					printf("(word) 0x%04X / %d\n", w, w);
				break;
			case 4:
				dw = AttemptSafeReadDword((DWORD*)dwAddress);
				if (bFloat)
					printf("(float) %f\n", *(float*)&dw);
				else {
					if (iBase == 10)
						printf("(dword) %u / %d\n", dw, dw);
					else if (iBase == 8)
						printf("(dword) %011o\n", dw);
					else
						printf("(dword) 0x%08X / %d\n", dw, dw);
				}
				break;
			case 8:
				qw = AttemptSafeReadQword((uint64_t*)dwAddress);
				if (bFloat)
					printf("(double) %lf\n", *(double*)&qw);
				else {
					if (iBase == 10)
						printf("(qword) %llu / %lld\n", qw, qw);
					else if (iBase == 8)
						printf("(qword) %022llo\n", qw);
					else
						printf("(qword) 0x%016llX / %lld\n", qw, qw);
				}
				break;
			}
		} else {
			// Display multiple elements (in hexadecimal for integers by default)
			while (iElementsCount > 0) {
				printf("0x%08X:", dwAddress);
				size_t uNextElements = (iElementsCount * iOptParam > 16 ? 16 / iOptParam : iElementsCount);
				iElementsCount -= 16 / iOptParam;

				BYTE buf[16];
				AttemptSafeMemcpy(buf, (BYTE*)dwAddress, 16);
				for (int i = 0; i < uNextElements; i++) {
					switch (iOptParam) {
					case 1:
						if (iBase == 10)
							printf(" %3u", buf[i]);
						else if (iBase == 8)
							printf(" %03o", buf[i]);
						else
							printf(" %02X", buf[i]);
						break;
					case 2:
						if (iBase == 10)
							printf(" %5u", *(WORD*)&buf[i * iOptParam]);
						else if (iBase == 8)
							printf(" %06o", *(WORD*)&buf[i * iOptParam]);
						else
							printf(" %04X", *(WORD*)&buf[i * iOptParam]);
						break;
					case 4:
						if (bFloat)
							printf(" %f", *(float*)&buf[i * iOptParam]);
						else {
							if (iBase == 10)
								printf(" %10u", *(DWORD*)&buf[i * iOptParam]);
							else if (iBase == 8)
								printf(" %011o", *(DWORD*)&buf[i * iOptParam]);
							else
								printf(" %08X", *(DWORD*)&buf[i * iOptParam]);
						}
						break;
					case 8:
						if (bFloat)
							printf(" %lf", *(double*)&buf[i * iOptParam]);
						else {
							if (iBase == 10)
								printf(" %20llu", *(uint64_t*)&buf[i * iOptParam]);
							else if (iBase == 8)
								printf(" %022llo", *(uint64_t*)&buf[i * iOptParam]);
							else
								printf(" %016llX", *(uint64_t*)&buf[i * iOptParam]);
						}
						break;
					}
				}
				printf("\n");
				dwAddress += 16;
			}
		}
	}
	catch (...) {
		return true;
	}

	return true;
}

bool ConsoleCommandShowMemoryByte(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 1);
}

bool ConsoleCommandShowMemoryWord(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 2);
}

bool ConsoleCommandShowMemoryDword(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 4);
}

bool ConsoleCommandShowMemoryQword(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 8);
}

bool ConsoleCommandShowMemoryFloat(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 5);
}

bool ConsoleCommandShowMemoryDouble(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	return ConsoleCommandShowMemory(args, iBreakoutState, 9);
}

bool ConsoleCommandShowMicrosim(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	DWORD dwAddress = NULL;
	int iMicrosimID = 0;
	bool bListAllMicrosims = false;

	if (dwDetectedVersion != VERSION_SC2K_1996) {
		printf_yellow("Command only available when attached to 1996 Special Edition.\n");
		return true;
	}
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"all", "Display list of provisioned microsims"},
				{"bigpark", "Display microsim data for large parks"},
				{"bus", "Display microsim data for bus depots"},
				{"hydro", "Display microsim data for hydroelectric dams"},
				{"id <id>", "ID of specific microsim to examine"},
				{"library", "Display microsim data for libraries"},
				{"marina", "Display microsim data for marinas"},
				{"museum", "Display microsim data for museums"},
				{"rail", "Display microsim data for rail stations"},
				{"subway", "Display microsim data for subway stations"},
				{"wind", "Display microsim data for wind power plants"},
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	// Arguments are required for this command
	if (args.size() == 0) {
		bConsoleKeepCommandBuffer = true;
		return false;
	}

	// Parse arguments
	for (size_t i = 0; i < args.size(); i++) {
		// [id <id>]
		if (args[i] == "id") {
			if (++i >= args.size())
				return false;

			if (!sscanf_s(args[i].c_str(), "%u", &iMicrosimID))
				return false;

			continue;
		}

		// all
		if (args[i] == "all") {
			bListAllMicrosims = true;
			continue;
		}

		// bigpark
		if (args[i] == "bigpark") {
			iMicrosimID = 6;
			continue;
		}

		// bus
		if (args[i] == "bus") {
			iMicrosimID = 1;
			continue;
		}

		// hydro
		if (args[i] == "hydro") {
			iMicrosimID = 5;
			continue;
		}

		// library
		if (args[i] == "library") {
			iMicrosimID = 8;
			continue;
		}

		// marina
		if (args[i] == "marina") {
			iMicrosimID = 9;
			continue;
		}

		// museum
		if (args[i] == "museum") {
			iMicrosimID = 7;
			continue;
		}

		// rail
		if (args[i] == "rail") {
			iMicrosimID = 2;
			continue;
		}

		// subway
		if (args[i] == "subway") {
			iMicrosimID = 3;
			continue;
		}

		// Invalid arugment, bail out
		return false;
	}

	// Show list if requested
	if (bListAllMicrosims) {
		printf("Provisioned microsims:\n");
		for (int i = 0; i <= MICROSIMID_MAX; i++)
			if (GetMicroSimulatorTileID(i) != TILE_CLEAR)
				printf("   %i: bTileID = %u\n", i, GetMicroSimulatorTileID(i));
	} else {
		if (iMicrosimID >= MICROSIMID_MIN && iMicrosimID <= MICROSIMID_MAX) {
			BYTE iTileID = GetMicroSimulatorTileID(iMicrosimID);
			printf(
				"Microsim %i:\n"
				"   Tile/Building:       %s (%u / 0x%02X)\n"
				"   Data Stat0 (Byte):   %u\n"
				"   Data Stat1 (Word 1): %u\n"
				"   Data Stat2 (Word 2): %u\n"
				"   Data Stat3 (Word 3): %u\n", iMicrosimID, szTileNames[iTileID], iTileID, iTileID, GetMicroSimulatorStat0(iMicrosimID),
				GetMicroSimulatorStat1(iMicrosimID), GetMicroSimulatorStat2(iMicrosimID), GetMicroSimulatorStat3(iMicrosimID));
			return true;
		}
		return false;
	}

	return true;
}

bool ConsoleCommandShowMods(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	bool bDetailed = false;
	bool bShowLuaMods = true;
	bool bShowNativeMods = true;

	if (dwDetectedVersion != VERSION_SC2K_1996) {
		printf_yellow("Command only available when attached to 1996 Special Edition.\n");
		return true;
	}

	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap(
			{
				{"<[Enter]>", "Executes this command"},
				{"[detail]", "Shows verbose information about loaded mods"},
				{"[lua]", "Only shows information about Lua mods"},
				{"[native]", "Only shows information about native code mods"},
			});
		bConsoleKeepCommandBuffer = true;
		return true;
	}

	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	// Parse arguments
	for (size_t i = 0; i < args.size(); i++) {
		// [detail]
		if (args[i] == "detail") {
			bDetailed = true;
			continue;
		}

		// [lua]
		if (args[i] == "lua") {
			bShowLuaMods = true;
			bShowNativeMods = false;
			continue;
		}

		// [native]
		if (args[i] == "native") {
			bShowLuaMods = false;
			bShowNativeMods = true;
			continue;
		}

		// Invalid argument, bail out
		return false;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	if (bShowNativeMods) {
		printf("%d native code mods loaded:\n", mapLoadedNativeMods.size());
		for (auto stNativeMod : mapLoadedNativeMods) {

			const char* szModVersion = _strdup(FormatVersion(stNativeMod.second.iModVersionMajor, stNativeMod.second.iModVersionMinor, stNativeMod.second.iModVersionPatch));
			const char* szModMinimumVersion = _strdup(FormatVersion(stNativeMod.second.iMinimumVersionMajor, stNativeMod.second.iMinimumVersionMinor, stNativeMod.second.iMinimumVersionPatch));

			printf(
				"   %s version %s (0x%08X)\n"
				"      Mod Name:             %s\n"
				"      Author:               %s\n"
				"      Req. sc2kfix version: %s\n"
				"      Description:          %s\n",
				stNativeMod.second.szModShortName, szModVersion, (INT_PTR)stNativeMod.first,
				stNativeMod.second.szModName,
				stNativeMod.second.szModAuthor,
				szModMinimumVersion,
				WordWrap(stNativeMod.second.szModDescription, csbi.dwSize.X, 28).c_str());
			if (bDetailed) {
				printf("      Hooks:\n");
				for (auto stHook : mapLoadedNativeModHooks[stNativeMod.first])
					printf("         %s (pri %d)\n", stHook.szHookName, stHook.iHookPriority);
			}
			printf("\n");
		}
	}

	return true;
}

bool ConsoleCommandShowDebug(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({{"<[Enter]>", "Execute this command"}});
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

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
	return true;
}

bool ConsoleCommandShowSettingsJson(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	printf("jsonSettingsCore:\n%s\n\n", jsonSettingsCore.dump().c_str());
	printf("jsonSettingsMods:\n%s\n", jsonSettingsMods.dump().c_str());
	return true;
}

bool ConsoleCommandShowVersion(std::vector<std::string> args, int iBreakoutState, intptr_t iOptParam) {
	std::string strProgramName = "SimCity 2000";
	std::string strProgramVersion = "unknown";

	// No arguments allowed
	if (iBreakoutState == BREAKOUT_QUESTION) {
		PrintAlignedStringMap({ {"<[Enter]>", "Execute this command"} });
		bConsoleKeepCommandBuffer = true;
		return true;
	}
	if (iBreakoutState != BREAKOUT_RETURN)
		return false;

	if (dwSC2KFixMode == SC2KFIX_MODE_SC2K) {
		switch (dwDetectedVersion) {
		case VERSION_SC2K_1995:
			strProgramVersion = "1995 CD Collection";
			break;
		case VERSION_SC2K_1996:
			strProgramVersion = "1996 Special Edition";
			break;
		}
	}
	else if (dwSC2KFixMode == SC2KFIX_MODE_SC2KDEMO) {
		if (dwDetectedVersion == VERSION_SC2K_DEMO)
			strProgramVersion = "Interactive Demo";
	}
	else if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		strProgramName = "SCURK";
		switch (dwDetectedVersion) {
		case VERSION_SCURK_PRIMARY:
			strProgramVersion = "1995 (Primary version)";
			break;
		case VERSION_SCURK_1996:
			strProgramVersion = "1996 Network Edition";
			break;
		}
	}
	else
		strProgramName = "Unknown";

	// AF - the separation comma positioning in this case is deliberate
	// in order to be a bit more "friendly" concerning missing or varied
	// end-arguments depending on the build.
	//
	// Reworked this significantly due to some legal advice (araxestroy)
	printf(
		VT100_COLOUR_BRIGHT_WHITE "sc2kfix version %s - (c) 2025-2026 sc2kfix Project (https://sc2kfix.net)\n" VT100_DEFAULT
		"Build info: %s\n"
		"%s version: %s\n"
		"Lua version: %s\n"
		"Plugin loaded at 0x%08X\n\n"

		"This program includes code from the following projects:\n"
		"  Lua is (c) 1994-2025 Lua.org, PUC-Rio; made available under the MIT License\n"
		"  FluidSynth is (c) FluidSynth; made available under the LGPL 2.1 license\n\n"

		"Licenses for third-party code can be found in the sc2kfix repository and distribution under\n"
		"the `thirdparty` directory. The sc2kfix plugin, example mods, and frameworks are released\n"
		"under the terms of the MIT license unless otherwise stated.\n\n"

		"SimCity 2000 is a registered trademark of Electronic Arts, Inc.\n",
		szSC2KFixVersion,
		szSC2KFixBuildInfo,
		strProgramName.c_str(),
		strProgramVersion.c_str(),
		LUA_VERSION,
		(DWORD)hSC2KFixModule
	);

	return true;
}

void NewConsoleInitializeCommands(console::CommandTree& treeCommands) {
	treeCommands["clear"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandClear, "Clear the console");
	treeCommands["fixup"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Fix up engine gremlins");
	treeCommands["fixup"]["things"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Fixup commands for Thing entities");
	treeCommands["fixup"]["things"]["clear"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandFixupThingsClear, "Clear (specific) Thing entities");
	treeCommands["run"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Run Lua REPL or scripts");
	treeCommands["run"]["lua"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandRunLua, "Run Lua REPL or scripts");
	treeCommands["run"]["test"] = ConsoleCommand(COMMAND_TYPE_UNDOCUMENTED, ConsoleCommandRunTest, "Test command");
	treeCommands["set"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Modify game and plugin behaviour");
	treeCommands["set"]["debug"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandSetDebug, "Enable/disable debugging options", COMMAND_OPTPARAM_ROOTNAME);
	treeCommands["set"]["undocumented"] = ConsoleCommand(COMMAND_TYPE_UNDOCUMENTED, ConsoleCommandSetUndocumented, "Enable/disable display of special commands", COMMAND_OPTPARAM_ROOTNAME);
	treeCommands["show"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Display various game and plugin information");
	treeCommands["show"]["audio"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Show audio engine info");
	treeCommands["show"]["audio"]["buffers"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowAudioBuffers, "Dump loaded WAV buffer metadata");
	treeCommands["show"]["audio"]["engine"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowAudioEngine, "Display audio engine information");
	treeCommands["show"]["audio"]["midi"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowAudioMidi, "Dump all discovered MIDI devices");
	treeCommands["show"]["audio"]["songs"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowAudioSongs, "Dump the current song playlist");
	treeCommands["show"]["debug"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowDebug, "Display enabled debugging options");
	treeCommands["show"]["memory"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Display memory contents");
	treeCommands["show"]["memory"]["byte"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryByte, "Display byte-sized elements");
	treeCommands["show"]["memory"]["word"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryWord, "Display word-sized elements");
	treeCommands["show"]["memory"]["dword"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryDword, "Display double word-sized elements");
	treeCommands["show"]["memory"]["qword"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryQword, "Display quad word-sized elements");
	treeCommands["show"]["memory"]["float"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryFloat, "Display single precision floating point elements");
	treeCommands["show"]["memory"]["double"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMemoryDouble, "Display double precision floating point elements");
	treeCommands["show"]["microsim"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMicrosim, "Show microsim data");
	treeCommands["show"]["mods"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowMods, "Show loaded mods");
	treeCommands["show"]["settings"][""] = ConsoleCommand(COMMAND_TYPE_BRANCH, NULL, "Show settings info");
	treeCommands["show"]["settings"]["json"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowSettingsJson, "Dump JSON settings structure");
	treeCommands["show"]["version"] = ConsoleCommand(COMMAND_TYPE_DOCUMENTED, ConsoleCommandShowVersion, "Show sc2kfix and library versions");

	// Aliases
	treeCommands["show"]["sound"] = treeCommands["show"]["audio"];
	treeCommands["unset"] = treeCommands["set"];
}

DWORD WINAPI NewConsoleThread(LPVOID lpParameter) {
	std::string strCommand;
	int breakout = BREAKOUT_NONE;
	bool bFullCommand = false;
	bool bDoneQuestionOut = false;
	bool bDoJuniperStyle = true;
	bConsoleKeepCommandBuffer = false;

	// Initialize the console
	Sleep(200);
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	NewConsoleInitializeCommands(treeConsoleCommands);

	// HIC SUNT DRACONES
	while (true) {
		if (bConsoleElevatedMode)
			printf(VT100_COLOUR_BRIGHT_WHITE "sc2kfix# " VT100_DEFAULT "%s", strCommand.c_str());
		else
			printf(VT100_COLOUR_BRIGHT_WHITE "sc2kfix> " VT100_DEFAULT "%s", strCommand.c_str());
		breakout = BREAKOUT_NONE;
		bDoneQuestionOut = false;
		while (!breakout) {
			int c = _getch();
			switch (c) {
			case CTRL('C'):
				printf("\n");
				breakout = BREAKOUT_INTERRUPT;
				break;
			case '\r':
			case '\n':
				printf("\n");
				breakout = BREAKOUT_RETURN;
				break;
			case '?':
				printf("?\n");
				breakout = BREAKOUT_QUESTION;
				break;
			case '\t':
				printf("\n");
				breakout = BREAKOUT_TAB;
				break;
			case ' ':
				if (strCommand == "") {
					printf("\a");
					continue;
				}
				else if (bDoJuniperStyle) {
					breakout = BREAKOUT_SPACE;
					break;
				}
				else {
					putc(c, stdout);
					strCommand.push_back(c);
					continue;
				}
			case '\b':
				if (strCommand == "") {
					printf("\a");
					continue;
				}
				strCommand.pop_back();
				printf("\b \b");
				continue;
			default:
				putc(c, stdout);
				strCommand.push_back(c);
				continue;
			}
		}

		strCommand.erase(std::find_if(strCommand.rbegin(), strCommand.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), strCommand.end());

		std::vector<std::string> vecSplit;
		if (!string_split(strCommand, vecSplit)) {
			printf_yellow("Unmatched quote.\n");
			continue;
		};

		debug_printf("vecSplit.size: %d\n", vecSplit.size());

		if ((bDoJuniperStyle && breakout == BREAKOUT_SPACE) || breakout == BREAKOUT_QUESTION || breakout == BREAKOUT_TAB) {
			size_t iDepth = 0;
			console::CommandTree* treePointer = &treeConsoleCommands;
			while (iDepth < vecSplit.size()) {
				if (treePointer->hasKey(vecSplit[iDepth])) {
					//printf("%d: found key\n", iDepth);
					if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Object) {
						//printf("%d: found object\n", iDepth);
						treePointer = &(*treePointer)[vecSplit[iDepth++]];

						if (vecSplit.size() == iDepth && !treePointer->hasKey(""))
							break;

						if (vecSplit.size() == iDepth && !treePointer->hasKey(vecSplit[iDepth - 1])) {
							debug_printf("bFC 1\n");
							bFullCommand = true;
						}

						if (vecSplit.size() == iDepth && treePointer->hasKey("")) {
							//printf("%d: found nested command\n", iDepth);
							if ((*treePointer)[""].ToCommand().iType == COMMAND_TYPE_BRANCH && !treePointer->hasKey(vecSplit[iDepth - 1])) {
								debug_printf("bFC 2 -- \"%s\"\n", vecSplit[iDepth - 1].c_str());
								bFullCommand = true;
								break;
							}
						}
						else if (!treePointer->hasKey(vecSplit[iDepth]) && treePointer->hasKey("")) {
							if ((*treePointer)[""].ToCommand().iType == COMMAND_TYPE_BRANCH && !treePointer->hasKey(vecSplit[iDepth])) {
								debug_printf("bFC 3 -- \"%s\"\n", vecSplit[iDepth - 1].c_str());
								bFullCommand = false;
								break;
							}
						}
						else if (treePointer->hasKey(vecSplit[iDepth])) {
							debug_printf("bFC 4 -- \"%s\"\n", vecSplit[iDepth].c_str());
							bFullCommand = true;
							break;
						}
						else {
							bFullCommand = false;
							break;
						}

						//printf("%d: continue\n", iDepth);
						continue;
					}
					else if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Command) {
						//printf("%d: found command\n", iDepth);
						debug_printf("bFC 5\n");
						bFullCommand = true;
						break;
					}
				}
				else {
					bFullCommand = false;
					break;
				}
			}
		}

		//for (int i = 0; i < vecSplit.size(); i++)
			//printf("%d: \"%s\"\n", i, vecSplit[i].c_str());

		/*if (breakout == BREAKOUT_SPACE && bFullCommand) {
			putc(' ', stdout);
			strCommand.push_back(' ');
		}*/

		if (bFullCommand && (breakout == BREAKOUT_TAB || breakout == BREAKOUT_SPACE)) {
			debug_printf("bFullCommand\n");
			strCommand.push_back(' ');

			if (breakout == BREAKOUT_SPACE)
				printf("\r");
				//printf("\x1B[1K\r");
		}

		if (breakout == BREAKOUT_QUESTION || breakout == BREAKOUT_TAB || (breakout == BREAKOUT_SPACE)) {
			size_t iDepth = 0;
			console::CommandTree* treePointer = &treeConsoleCommands;

			std::map<std::string, std::string> mapOutput;
			size_t iLongestCommand = 0;

			if (vecSplit.size() == 0) {
				for (auto s : treePointer->ObjectRange()) {
					if ((*treePointer)[s.first].ObjectType() == console::CommandTree::Class::Command) {
						mapOutput[s.first] = s.second.ToCommand().szDescription;
						if (iLongestCommand < s.first.size())
							iLongestCommand = s.first.size();
					}
					else if ((*treePointer)[s.first].ObjectType() == console::CommandTree::Class::Object && s.second.hasKey("")) {
						mapOutput[s.first + " ..."] = s.second[""].ToCommand().szDescription;
						if (iLongestCommand < s.first.size() + 4)
							iLongestCommand = s.first.size() + 4;
					}
				}

				if (breakout == BREAKOUT_SPACE)
					printf("\n");

				for (auto s : mapOutput)
					printf("   %s%s%s\n", s.first.c_str(), std::string(3 + iLongestCommand - s.first.size(), ' ').c_str(), s.second.c_str());

				bDoneQuestionOut = true;
				continue;
			}

			bool bGo = false;

			while (iDepth < vecSplit.size()) {
				if (treePointer->hasKey(vecSplit[iDepth])) {
					//printf("%d: found key\n", iDepth);
					if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Object) {
						//printf("%d: found object\n", iDepth);
						treePointer = &(*treePointer)[vecSplit[iDepth++]];

						if (vecSplit.size() == iDepth && treePointer->hasKey("")) {
							//printf("%d: found nested command\n", iDepth);
							bGo = true;
							break;
						}

						//printf("%d: continue\n", iDepth);
						continue;
					}
					else if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Command) {
						if (breakout != BREAKOUT_QUESTION)
							bGo = true;
						break;
					}
				}
				else {
					bGo = true;
					break;
				}
			}

			if (iDepth == vecSplit.size()) {
				if (strCommand[strCommand.size() - 1] != ' ')
					strCommand.push_back(' ');
				vecSplit.push_back("");
			}

			if (bGo) {
				int i = 0;
				for (auto s : treePointer->ObjectRange()) {
					std::string s2 = s.first;
					if ((*treePointer)[s.first].ObjectType() == console::CommandTree::Class::Command) {
						if (!string_starts_with(s2, vecSplit[iDepth].c_str()))
							continue;
						if (!bConsoleElevatedMode && s.second.ToCommand().iType == COMMAND_TYPE_UNDOCUMENTED ||
							s.second.ToCommand().iType == COMMAND_TYPE_HIDDEN)
							continue;

						if (s.first == "" && s.second.ToCommand().iType != COMMAND_TYPE_BRANCH) {
							mapOutput["..."] = s.second.ToCommand().szDescription;
							if (iLongestCommand < 3)
								iLongestCommand = 3;

						}
						else {
							mapOutput[s.first] = s.second.ToCommand().szDescription;
							if (iLongestCommand < s.first.size())
								iLongestCommand = s.first.size();
						}
					}
					else if ((*treePointer)[s.first].ObjectType() == console::CommandTree::Class::Object && s.second.hasKey("")) {
						if (!string_starts_with(s2, vecSplit[iDepth].c_str()))
							continue;
						mapOutput[s.first + " ..."] = s.second[""].ToCommand().szDescription;
						if (iLongestCommand < s.first.size() + 4)
							iLongestCommand = s.first.size() + 4;
					}
				}

				//printf("%d: itsame\n", iDepth);

				strCommand.erase(std::find_if(strCommand.rbegin(), strCommand.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), strCommand.end());

				if (breakout == BREAKOUT_TAB || breakout == BREAKOUT_SPACE) {
					size_t i = 0;
					std::map<std::string, std::string> mapSort = mapOutput;

					for (auto s : mapSort) {
						std::string s2 = s.first;

						if (string_ends_with(s2, " ...")) {
							s2.pop_back();
							s2.pop_back();
							s2.pop_back();
							s2.pop_back();
						}

						if (s2.size() > i)
							i = s2.size();
					}
					for (auto s : mapSort) {
						std::string s2 = s.first;

						if (string_ends_with(s2, " ...")) {
							s2.pop_back();
							s2.pop_back();
							s2.pop_back();
							s2.pop_back();
						}
						if (s2.size() < i)
							i = s2.size();
					}

					//printf("shortest command: %d\n", i);

					bool local_breakout = false;
					size_t j;
					for (j = vecSplit[iDepth].size(); j < i; j++) {
						char c = mapOutput.begin()->first[j];
						for (auto s : mapOutput) {
							if (s.first[j] != c) {
								local_breakout = true;
								break;
							}
						}
						if (local_breakout)
							break;
						vecSplit[iDepth].push_back(c);
						strCommand.push_back(c);
					}
					if (i == j) {
						bFullCommand = true;
						debug_printf("autocompleted whole command\n");
					}
				}

				if (mapOutput.size() == 0) {
					printf_yellow("%sInvalid argument.\n", breakout == BREAKOUT_SPACE ? "\n" : "");
					bDoneQuestionOut = true;
				}
				else if (!(mapOutput.size() == 1 && breakout == BREAKOUT_SPACE)) {
					bDoneQuestionOut = true;

					if (mapOutput.size() != 1 && breakout == BREAKOUT_SPACE && !bFullCommand)
						printf("\n");

					for (auto s : mapOutput) {
						if (s.first != "" && (breakout == BREAKOUT_TAB || breakout == BREAKOUT_QUESTION || (breakout == BREAKOUT_SPACE && !bFullCommand)))
							printf("   %s%s%s\n", s.first.c_str(), std::string(3 + iLongestCommand - s.first.size(), ' ').c_str(), s.second.c_str());
					}

					if (bFullCommand)
						strCommand.push_back(' ');
				}

				if (mapOutput.size() == 1) {
					if (breakout == BREAKOUT_SPACE) {
						if (bFullCommand) {
							strCommand.push_back(' ');
							printf("\r");
							//printf("\x1B[1K\r");
						}
					}
				}
			}
		}

		if (breakout == BREAKOUT_RETURN || breakout == BREAKOUT_INTERRUPT || (breakout == BREAKOUT_QUESTION && !bDoneQuestionOut)) {
			if (breakout == BREAKOUT_INTERRUPT)
				continue;

			strCommand.erase(std::find_if(strCommand.rbegin(), strCommand.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), strCommand.end());

			size_t iDepth = 0;
			console::CommandTree* treePointer = &treeConsoleCommands;
			while (iDepth < vecSplit.size()) {
				if (treePointer->hasKey(vecSplit[iDepth])) {
					debug_printf("%d: found key\n", iDepth);
					if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Object) {
						debug_printf("%d: found object\n", iDepth);
						treePointer = &(*treePointer)[vecSplit[iDepth++]];
						if (breakout == BREAKOUT_QUESTION)
							continue;

						if (vecSplit.size() == iDepth && !treePointer->hasKey("")) {
							printf_yellow("Invalid argument.\n");
							break;
						}

						if ((vecSplit.size() == iDepth && treePointer->hasKey("") ||
							!treePointer->hasKey(vecSplit[iDepth]) && treePointer->hasKey("")) &&
							breakout != BREAKOUT_QUESTION) {
							debug_printf("%d: found nested command\n", iDepth);
							if ((*treePointer)[""].ToCommand().iType == COMMAND_TYPE_BRANCH) {
								printf_yellow("Invalid argument.\n");
								break;
							}

							std::vector<std::string> vecArgs;
							for (size_t i = iDepth + 1; i < vecSplit.size(); i++)
								vecArgs.push_back(vecSplit[i]);

							intptr_t iOptParam = NULL;
							if ((*treePointer)[""].ToCommand().iOptParam == COMMAND_OPTPARAM_ROOTNAME)
								iOptParam = (intptr_t)&vecSplit[0];
							else if ((*treePointer)[""].ToCommand().iOptParam == COMMAND_OPTPARAM_TREE)
								iOptParam = (intptr_t)&vecSplit;

							if (!(*treePointer)[""].ToCommand().pCommand(vecArgs, breakout, iOptParam))
								printf_yellow("Invalid argument.\n");
							break;
						}

						debug_printf("%d: continue\n", iDepth);
						continue;
					}
					else if ((*treePointer)[vecSplit[iDepth]].ObjectType() == console::CommandTree::Class::Command) {
						debug_printf("%d: found command\n", iDepth);
						std::vector<std::string> vecArgs;
						for (size_t i = iDepth + 1; i < vecSplit.size(); i++)
							vecArgs.push_back(vecSplit[i]);

						intptr_t iOptParam = NULL;
						if ((*treePointer)[vecSplit[iDepth]].ToCommand().iOptParam == COMMAND_OPTPARAM_ROOTNAME)
							iOptParam = (intptr_t)&vecSplit[0];
						else if ((*treePointer)[vecSplit[iDepth]].ToCommand().iOptParam == COMMAND_OPTPARAM_TREE)
							iOptParam = (intptr_t)&vecSplit;

						if (!((*treePointer)[vecSplit[iDepth]].ToCommand().pCommand(vecArgs, breakout, iOptParam)))
							printf_yellow("Invalid argument.\n");
						break;
					}
				}
				else {
					printf_yellow("Invalid argument.\n");
					break;
				}
			}

			if (!bConsoleKeepCommandBuffer && !bDoneQuestionOut)
				strCommand = "";

			bConsoleKeepCommandBuffer = false;
		}
	}
}