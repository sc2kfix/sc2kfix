// sc2kfix mods/sc2kfix.h: main include file for mods
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once
#pragma warning(disable : 4200)

#define HOOKEXT	extern "C" __declspec(dllimport)
#define HOOKCB	extern "C" __declspec(dllexport)

#define DEBUG_FLAGS_NONE		0
#define DEBUG_FLAGS_EVERYTHING	0xFFFFFFFF

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
} sc2kfix_mod_info_t;

typedef struct {
	const char* szHookName;
	int iHookPriority;
} sc2kfix_mod_hook_t;

typedef struct {
	int iHookCount;
	sc2kfix_mod_hook_t stHooks[];
} sc2kfix_mod_hooklist_t;

HOOKEXT HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText);
HOOKEXT const char* HexPls(UINT uNumber, int width);
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...);
