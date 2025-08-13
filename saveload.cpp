// sc2kfix modules/saveload.cpp: hooks for fixing save/load bugs
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE: This file is a pared down version of sc2x.cpp from the main branch. It does not need to
// be ported back into main if/when r9_patches wraps up.

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define SC2X_DEBUG_LOAD 1
#define SC2X_DEBUG_SAVE 2
#define SC2X_DEBUG_VANILLA_LOAD 4
#define SC2X_DEBUG_VANILLA_SAVE 8
#define SC2X_DEBUG_JSON_LOAD 16
#define SC2X_DEBUG_JSON_SAVE 32

#define SC2X_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SC2X_DEBUG
#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define BAILOUT(s, ...) do { \
	ConsoleLog(LOG_ERROR, "SAVE: " s, __VA_ARGS__); \
	return 0; \
} while (0)

UINT sc2x_debug = SC2X_DEBUG;

static char* szLoadFileName = NULL;
static int iCorruptedFixupSize = 0;
static DWORD dwDummy;


extern "C" DWORD __stdcall Hook_LoadGame(void* pFile, char* src) {
	DWORD(__thiscall * H_SimcityAppDoLoadGame)(void*, void*, char*) = (DWORD(__thiscall*)(void*, void*, char*))0x4302E0;
	DWORD* pThis;
	DWORD ret;

	__asm mov [pThis], ecx

	szLoadFileName = src;
	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: Loading saved game \"%s\".\n", szLoadFileName);

	std::ifstream infile(szLoadFileName, std::ios::binary | std::ios::ate);
	if (infile.is_open()) {
		iCorruptedFixupSize = infile.tellg();
		iCorruptedFixupSize -= 8;
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "LOAD: Saved game iCorruptedFixupSize is %d bytes.\n", iCorruptedFixupSize);
		infile.close();
	}
	else {
		ConsoleLog(LOG_WARNING, "LOAD: Couldn't open saved game \"%s\" to determine iCorruptedFixupSize.\n", szLoadFileName);
		ConsoleLog(LOG_WARNING, "LOAD: If this save is corrupted, sc2kfix will not be able to attempt to fix it.\n");
		iCorruptedFixupSize = 0;
	}

	// Make sure it's an .sc2 file and attempt to load if so.
	if (std::regex_search(szLoadFileName, std::regex("\\.[Ss][Cc]2$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "LOAD: Saved game is a vanilla SC2 file. Passing control to SC2K.\n");

		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
	}
	else if (std::regex_search(szLoadFileName, std::regex("\\.[Ss][Cc][Nn]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "LOAD: Saved game is a vanilla SCN file. Passing control to SC2K.\n");

		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
	}
	else if (std::regex_search(szLoadFileName, std::regex("\\.[Cc][Tt][Yy]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "LOAD: Saved game is a SimCity Classic file. Passing control to SC2K.\n");

		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
	}

	return ret;
}

extern "C" DWORD __stdcall Hook_SaveGame(CMFC3XString* lpFileName) {
	DWORD(__thiscall * H_SimcityAppDoSaveGame)(void*, CMFC3XString*) = (DWORD(__thiscall*)(void*, CMFC3XString*))0x432180;
	DWORD* pThis;
	DWORD ret;

	__asm mov [pThis], ecx

	ret = H_SimcityAppDoSaveGame(pThis, lpFileName);
	return ret;
}

// Assembly language hook to try to fix up corrupted save file headers.
void __declspec(naked) Hook_431212(void) {
	// Replace the code we're clobbering to inject ourselves
	__asm {
		// Original call flow
		mov eax, 0x401429
		call eax
		add esp, 4

		// Check if eax is 0; skip otherwise
		push ebp
		mov ebp, esp
		cmp eax, 0
		jne skip
	}

	// If we don't have a fixup size, inform the user that their game is about to crash
	if (!iCorruptedFixupSize) {
		MessageBox(GetActiveWindow(),
			"sc2kfix has detected a corrupted save file but was unable to recover enough information to "
			"attempt to fix it. Your game is likely to crash after closing this dialog box. Please file"
			"a save corruption report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues).\n\n"

			"Developer info:\n"
			"Save header corrupted (FORM header chunk size 0)\n"
			"Failed to load iCorruptedFixupSize in Hook_LoadGame", "sc2kfix error", MB_OK | MB_ICONERROR);

		__asm jmp skip
	}

	// Log that we're attempting a fixup
	ConsoleLog(LOG_NOTICE, "LOAD: Detected possible corrupted save \"%s\".\n", szLoadFileName);
	ConsoleLog(LOG_NOTICE, "LOAD: Attempting to fix up corrupted save header, new size = %d.\n", iCorruptedFixupSize);

	// Inform the user about what's going on
	MessageBox(GetActiveWindow(),
		"sc2kfix has detected a corrupted save file and will try to restore it. If your city loads "
		"successfully, you should save it to a new save game file as soon as possible, restart "
		"SimCity 2000, and load the new save.\n\n"

		"If the game crashes after closing this dialog box or after reloading the new save file, "
		"please file a report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues).\n\n"

		"Developer info:\n"
		"Save header corrupted (FORM header chunk size 0)", "sc2kfix warning", MB_OK | MB_ICONWARNING);

	// Inject the right (or, close enough) size back into the original code path
	__asm {
		mov eax, [iCorruptedFixupSize]
	skip:
		pop ebp
		push 0x43121A		// jump back to original control flow
		retn
	}
}

void InstallSaveHooks(void) {
	// Load game hook
	VirtualProtect((LPVOID)0x4025A4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4025A4, Hook_LoadGame);

	// Patch to stop CFile::CFile() from being called in exclusive mode when loading a game
	VirtualProtect((LPVOID)0x430118, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x430118 = 0x8040;

	// Patch to attempt to fix loading partially corrupted saves
	VirtualProtect((LPVOID)0x431212, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x431212, Hook_431212);

	// Save game hook
	VirtualProtect((LPVOID)0x401870, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401870, Hook_SaveGame);
}
