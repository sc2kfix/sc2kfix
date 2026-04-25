// sc2kfix mods/sc2kfix.h: main include file for mods
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once
#pragma warning(disable : 4200)

#include <string>

#define HOOKEXT		extern "C" __declspec(dllimport)
#define HOOKEXT_CPP	__declspec(dllimport)
#define HOOKCB		extern "C" __declspec(dllexport)

#include "../include/json.hpp"
#include "../include/commandtree.hpp"
#include "../include/mfc3xhelp.h"
#include "../include/commonhelp.h"
#include "../include/sc2kclasses.h"
#include "../include/sc2k_1996.h"
#include "../include/vt100.h"

#define IFF_HEAD(a, b, c, d) ((DWORD)d << 24 | (DWORD)c << 16 | (DWORD)b << 8 | (DWORD)a)

#define RELATIVE_OFFSET(from, to) *(DWORD*)((DWORD)(from)) = (DWORD)(to) - (DWORD)(from) - 4;
#define NEWCALL(from, to) *(BYTE*)(from) = 0xE8; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJMP(from, to) *(BYTE*)(from) = 0xE9; RELATIVE_OFFSET((DWORD)(from)+1, to)
#define NEWJB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x82; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNB(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x83; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x84; RELATIVE_OFFSET((DWORD)(from)+2, to)
#define NEWJNZ(from, to) *(BYTE*)(from) = 0x0F; *(BYTE*)((DWORD)(from)+1) = 0x85; RELATIVE_OFFSET((DWORD)(from)+2, to)

#define DEBUG_FLAGS_NONE		0
#define DEBUG_FLAGS_EVERYTHING	0xFFFFFFFF

#define LOG(level, fmt, ...) ConsoleLog(level, "MODS: (%s) " fmt, stModInfo.szModShortName, __VA_ARGS__)

#define GAME_MAP_SIZE 128u

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

typedef struct {
	const char* szHookName;
	int iHookPriority;
} sc2kfix_mod_hook_t;

typedef struct {
	int iModInfoVersion;				// Mandatory

	int iModVersionMajor;				// Mandatory
	int iModVersionMinor;				// Mandatory
	int iModVersionPatch;				// Mandatory
	int iMinimumVersionMajor;			// Mandatory
	int iMinimumVersionMinor;			// Mandatory
	int iMinimumVersionPatch;			// Mandatory

	const char* szModName;				// Mandatory
	const char* szModShortName;			// Mandatory
	const char* szModAuthor;			// Optional, but recommended
	const char* szModDescription;		// Optional, but recommended

	int iHookCount;						// Mandatory
	sc2kfix_mod_hook_t* pstHooks;		// Mandatory
} sc2kfix_mod_info_t;

#define CTRL(c) (c - 64)

#define COMMAND_TYPE_UNKNOWN		0	// undefined
#define COMMAND_TYPE_BRANCH			1	// fake "command" used to add documentation to trees
#define COMMAND_TYPE_DOCUMENTED		2	// command will show up in ? without set undoc
#define COMMAND_TYPE_UNDOCUMENTED	3	// command requires set undoc to show up in ?
#define COMMAND_TYPE_HIDDEN			4	// command does not show up in ? or autocomplete

#define COMMAND_OPTPARAM_NONE		0	// don't pass anything special in iOptParam (default)
#define COMMAND_OPTPARAM_ROOTNAME	1	// pass a pointer to the entered root command name
#define COMMAND_OPTPARAM_TREE		2	// pass a pointer to std::string vecSplit

#define BREAKOUT_NONE		0
#define BREAKOUT_RETURN		1
#define BREAKOUT_TAB		2
#define BREAKOUT_QUESTION	3
#define BREAKOUT_INTERRUPT	4
#define BREAKOUT_SPACE		5

#define printf_red(s, ...) printf(VT100_COLOUR_RED s VT100_DEFAULT, __VA_ARGS__)
#define printf_lightred(s, ...) printf(VT100_COLOUR_BRIGHT_RED s VT100_DEFAULT, __VA_ARGS__)
#define printf_yellow(s, ...) printf(VT100_COLOUR_YELLOW s VT100_DEFAULT, __VA_ARGS__)
#define printf_lightblue(s, ...) printf(VT100_COLOUR_BRIGHT_BLUE s VT100_DEFAULT, __VA_ARGS__)

#define HOOKS_COUNT(st) (sizeof(st) / sizeof(sc2kfix_mod_hook_t))

HOOKEXT bool bConsoleInLuaREPL;
HOOKEXT bool bConsoleElevatedMode;
HOOKEXT bool bConsoleKeepCommandBuffer;
HOOKEXT_CPP console::CommandTree treeConsoleCommands;

HOOKEXT void CenterDialogBox(HWND hwndDlg);
HOOKEXT HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText);
HOOKEXT const char* HexPls(UINT uNumber, int width);
HOOKEXT const char* FormatVersion(int iMajor, int iMinor, int iPatch);
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...);
HOOKEXT const char* GetLowHighScale(BYTE bScale);
HOOKEXT BOOL FileExists(const char* name);
HOOKEXT const char* GetModsFolderPath(void);
HOOKEXT const char* GetOnIdleStateEnumName(int iState);
HOOKEXT const char* GetOnIdleInitialDialogEnumName(int iInitialDialogState);
HOOKEXT BOOL WritePrivateProfileIntA(const char* section, const char* name, int value, const char* ini_name);

HOOKEXT_CPP std::string Base64Encode(const unsigned char* pSrcData, size_t iSrcCount);
HOOKEXT_CPP size_t Base64Decode(BYTE* pBuffer, size_t iBufSize, const unsigned char* pSrcData, size_t iSrcCount);
HOOKEXT_CPP json::JSON EncodeDWORDArray(DWORD* dwArray, size_t iCount, BOOL bBigEndian);
HOOKEXT_CPP void DecodeDWORDArray(DWORD* dwArray, json::JSON jsonArray, size_t iCount, BOOL bBigEndian);
HOOKEXT_CPP void PrintAlignedStringMap(std::map<std::string, std::string> mapStr, int iPrefixSpaces = 3);
