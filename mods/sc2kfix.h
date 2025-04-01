// sc2kfix mods/sc2kfix.h: main include file for mods
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

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
	const char* szModName;
	const char* szModShortName;
	const char* szModAuthor;
	const char* szModDescription;
} sc2kfix_mod_info_t;

HOOKEXT HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText);
HOOKEXT const char* HexPls(UINT uNumber, int width);
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...);
