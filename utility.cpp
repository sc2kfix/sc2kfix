// sc2kfix utility.cpp: utility functions to save me from reinventing the wheel
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include <commandtree.hpp>
#include "resource.h"

BOOL bFontsInitialized = FALSE;
HFONT hFontMSSansSerifRegular8;
HFONT hFontMSSansSerifBold8;
HFONT hFontMSSansSerifRegular10;
HFONT hFontMSSansSerifBold10;
HFONT hFontArialRegular10;
HFONT hFontArialBold10;
HFONT hFontArialBold16;
HFONT hSystemRegular12;

static BYTE DOSMacPalTable[256];

void InitializeFonts(void) {
	if (bFontsInitialized)
		return;

	HDC hDC = GetDC(0);
	int iDPI = GetDeviceCaps(hDC, LOGPIXELSY);
	hFontMSSansSerifRegular8 = CreateFont(-MulDiv(8, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifBold8 = CreateFont(-MulDiv(8, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifRegular10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontMSSansSerifBold10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hFontArialRegular10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	hFontArialBold10 = CreateFont(-MulDiv(10, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
	hFontArialBold16 = CreateFont(-MulDiv(16, iDPI, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial");
	hSystemRegular12 = CreateFont(-MulDiv(12, iDPI, 72), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "System");
	bFontsInitialized = TRUE;
}

#pragma warning(push)
#pragma warning(disable : 28159)
// Wrapper for GetTickCount that shuts up MSVC warnings.
HOOKEXT DWORD __stdcall GetTickCount32(void) {
	return GetTickCount();
}
#pragma warning(pop)

// Pop up a crash error for a missing DLL
void __declspec(noreturn) MessageBoxCrash(std::string strComponent, DWORD dwErrorCode) {
	std::string strErrorMessage;

	if (strComponent == "SafeVirtualProtect" || strComponent == "exception")
		;	// do nothing
	else if (string_ends_with(strComponent, ".dll"))
		ConsoleLog(LOG_EMERGENCY, "CORE: Couldn't load %s; error code 0x%08X.\n", strComponent.c_str(), dwErrorCode);
	else
		ConsoleLog(LOG_EMERGENCY, "CORE: Fatal error in component %s detected.\n", strComponent.c_str());

	if (string_ends_with(strComponent, ".dll")) {
		strErrorMessage = string_format(
			"sc2kfix has encountered a fatal error when trying to load the %s library required "
			"by the plugin to function. This may be due to the %s file supplied with the plugin "
			"not being present. Please ensure that you have extracted all DLLs in the root of the sc2kfix "
			"release ZIP alongside winmm.dll.\n\n"

			"If you have all four DLLs present, please submit a crash report to the sc2kfix developers "
			"either via the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues -- "
			"preferred) or via the sc2kfix Discord server (https://sc2kfix.net/discord). In order for "
			"us to best assist with the crash, please make a copy of the %s file after closing "
			"this dialog and before you re-open %s. Submit this copy of the log file along with "
			"your crash report, and we will do our best to investigate.\n\n"

			"Clicking the OK button will immediately terminate %s. Any unsaved progress "
			"will be lost.",
			strComponent.c_str(), strComponent.c_str()),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "sc2kfix-scurk.log" : "sc2kfix.log"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000");
	} else if (strComponent == "SafeVirtualProtect") {
		strErrorMessage = string_format(
			"sc2kfix has encountered a fatal error when trying to set up critical hooks into the "
			"%s game engine. Initialization of the game cannot continue. This may be due "
			"to system security configuration, misbehaving antivirus software, or running the game "
			"alongside another game with aggressive anti-cheat functionality.\n\n"

			"Please submit a crash report to the sc2kfix developers either via the sc2kfix GitHub "
			"issues page (https://github.com/sc2kfix/sc2kfix/issues -- preferred) or via the sc2kfix "
			"Discord server (https://sc2kfix.net/discord). In order for us to best assist with the "
			"crash, please make a copy of the %s file after closing this dialog and before "
			"you re-open %s. Submit this copy of the log file along with your crash report, "
			"and we will do our best to investigate.\n\n"

			"Clicking the OK button will immediately terminate %s. Any unsaved progress "
			"will be lost.",
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "sc2kfix-scurk.log" : "sc2kfix.log"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000")
		);
	} else if (strComponent == "exception") {
		strErrorMessage = string_format(
			"sc2kfix has detected an unhandled top-level exception in %s. If you have the "
			"console open, check the console for details. Fault information has been logged to the "
			"console and to sc2kfix.log.\n\n"

			"Please submit a crash report to the sc2kfix developers either via the sc2kfix GitHub "
			"issues page (https://github.com/sc2kfix/sc2kfix/issues -- preferred) or via the sc2kfix "
			"Discord server (https://sc2kfix.net/discord). In order for us to best assist with the "
			"crash, please make a copy of the %s file after closing this dialog and before "
			"you re-open %s. Submit this copy of the log file along with your crash report, "
			"and we will do our best to investigate.\n\n"

			"Clicking the OK button will immediately terminate %s. Any unsaved progress "
			"will be lost.",
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "sc2kfix-scurk.log" : "sc2kfix.log"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000")
		);
	} else
		strErrorMessage = string_format(
			"sc2kfix has detected an unspecified fatal error and cannot continue. This SHOULD NOT happen; "
			"please inform a developer immediately by submitting a crash report to either via the sc2kfix GitHub "
			"issues page (https://github.com/sc2kfix/sc2kfix/issues -- preferred) or via the sc2kfix "
			"Discord server (https://sc2kfix.net/discord). In order for us to best assist with the "
			"crash, please make a copy of the %s file after closing this dialog and before "
			"you re-open %s. Submit this copy of the log file along with your crash report, "
			"and we will do our best to investigate.\n\n"

			"Clicking the OK button will immediately terminate %s. Any unsaved progress "
			"will be lost.",
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "sc2kfix-scurk.log" : "sc2kfix.log"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000"),
			(dwSC2KFixMode == SC2KFIX_MODE_SCURK ? "SCURK" : "SimCity 2000")
		);

	MessageBox(GetActiveWindow(), strErrorMessage.c_str(), "sc2kfix fatal error", MB_OK | MB_ICONSTOP);

	if (strComponent != "exception")
		abort();
}

// Wrapper for VirtualProtect that throws a fatal error if it fails
bool SafeVirtualProtectEx(void* lpAddress, size_t dwSize, DWORD flNewProtect, const char* szFile, int iLine, const char* szFunction) {
	DWORD dwDummy;
	bool bSuccess = VirtualProtect(lpAddress, dwSize, flNewProtect, &dwDummy);

	if (bSuccess)
		return bSuccess;

	DWORD dwError = GetLastError();

	ConsoleLog(LOG_EMERGENCY, "CORE: SafeVirtualProtect(0x%08X, %d, 0x%08X) failed at %s:%d in function %s(); error code 0x%08X.\n", lpAddress, dwSize, flNewProtect, szFile, iLine, szFunction, dwError);
	MessageBoxCrash("SafeVirtualProtect", NULL);
}

HOOKEXT void CenterDialogBox(HWND hwndDlg) {
	HWND hwndDesktop;
	RECT rcTemp, rcDlg, rcDesktop;

	hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &rcDesktop);
	GetWindowRect(hwndDesktop, &rcTemp);
	GetWindowRect(hwndDlg, &rcDlg);
	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
	OffsetRect(&rcTemp, -rcDesktop.left, -rcDesktop.top);
	OffsetRect(&rcTemp, -rcDlg.right, -rcDlg.bottom);
	SetWindowPos(hwndDlg, HWND_TOP, rcDesktop.left + (rcTemp.right / 2), rcDesktop.top + (rcTemp.bottom / 2), 0, 0, SWP_NOSIZE);
}

// Creates a Win32 common controls tooltip and assigns it to a given control in a given window.
// XXX - Technically leaks a small amount of memory, as _strdup() is called on each invocation.
static HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText) {
	if (!hDlg || !hControl || !szText)
		return NULL;

	HWND hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, hDlg, NULL, hSC2KFixModule, NULL);
	if (!hTooltip)
		return NULL;

	SendMessage(hTooltip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);

	TOOLINFO tooltipInfo = { 0 };
	tooltipInfo.cbSize = sizeof(TOOLINFO);
	tooltipInfo.hwnd = hDlg;
	tooltipInfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	tooltipInfo.uId = (UINT_PTR)hControl;
	tooltipInfo.lpszText = (LPSTR) szText;
	SendMessage(hTooltip, TTM_ADDTOOL, NULL, (LPARAM)&tooltipInfo);

	return hTooltip;
}

// Create and store tooltip for later destruction.
HOOKEXT void StoreTooltip(std::vector<tooltip_store_t> &tt_s, HWND hParent, HWND hControl, const char *szText) {
	HWND hToolTip;
	tooltip_store_t tt_item;

	hToolTip = CreateTooltip(hParent, hControl, szText);
	if (hToolTip) {
		tt_item.hParent = hParent;
		tt_item.hControl = hControl;
		tt_item.hToolTip = hToolTip;
		tt_s.push_back(tt_item);
	}
}

// Destroys the stored tooltip and frees the string.
static void DeleteTooltip(HWND hDlg, HWND hControl, HWND hTooltip) {
	if (!hDlg || !hControl || !hTooltip)
		return;

	TOOLINFO tooltipInfo = { 0 };
	tooltipInfo.cbSize = sizeof(TOOLINFO);
	tooltipInfo.hwnd = hDlg;
	tooltipInfo.uFlags = TTF_IDISHWND;
	tooltipInfo.uId = (UINT_PTR)hControl;
	SendMessage(hTooltip, TTM_DELTOOL, 0, (LPARAM)&tooltipInfo);

	DestroyWindow(hTooltip);
}

// Destroy associated tooltip entries.
HOOKEXT void DestroyStoredTooltips(std::vector<tooltip_store_t> &tt_s, HWND hParent) {
	for (std::vector<tooltip_store_t>::iterator it = tt_s.begin(); it != tt_s.end();) {
		if (it->hParent == hParent) {
			DeleteTooltip(it->hParent, it->hControl, it->hToolTip);
			it = tt_s.erase(it);
		}
		else
			++it;
	}
}

// Formats a hexadecimal number in a very temporary C string.
// Please don't use this function if you can avoid it.
HOOKEXT const char* HexPls(UINT uNumber, int width) {
	thread_local char szRet[16] = { 0 };
	sprintf_s(szRet, 16, "0x%0*X", width, uNumber);
	return szRet;
}

// Formats an internal sc2kfix version as a static char[]
HOOKEXT const char* FormatVersion(int iMajor, int iMinor, int iPatch) {
	static char szRet[16] = { 0 };
	if (!iPatch)
		sprintf_s(szRet, 16, "%d.%d", iMajor, iMinor);
	else
		sprintf_s(szRet, 16, "%d.%d%c", iMajor, iMinor, iPatch - 1 + 'a');
	return szRet;
}

// Transforms a std::string into another std::string with a max width of iMaxWidth and an optional
// indentation on each wrap.
HOOKEXT_CPP std::string WordWrap(std::string strInput, size_t iMaxWidth, size_t iIndentWidth) {
	std::istringstream is(strInput);
	std::ostringstream os;
	std::string strWord;
	size_t iCurrentPos = iIndentWidth;

	while (is >> strWord) {
		if (strWord.size() + iCurrentPos > iMaxWidth) {
			os << "\n" + std::string(iIndentWidth, ' ');
			iCurrentPos = iIndentWidth;
		}

		os << strWord + ' ';
		iCurrentPos += strWord.size() + 1;
	}
	
	return os.str();
}

extern FILE* fdLog;

// Writes a message to the console and the running log file, with a colour-coded log level prefix
// for the message displayed on the console.
HOOKEXT void ConsoleLog(int iLogLevel, const char* fmt, ...) {
	va_list args;
	int len;
	char* buf;
	const char* prefix;
	const char* colour;

	switch (iLogLevel) {
	case LOG_EMERGENCY:
		colour = VT100_BGCOLOUR_BRIGHT_RED VT100_COLOUR_BLACK;
		prefix = "[EMERG]";
		break;
	case LOG_ALERT:
		colour = VT100_BGCOLOUR_RED VT100_COLOUR_BLACK;
		prefix = "[ALERT]";
		break;
	case LOG_CRITICAL:
		colour = VT100_COLOUR_BRIGHT_RED;
		prefix = "[CRIT ]";
		break;
	case LOG_ERROR:
		colour = VT100_COLOUR_RED;
		prefix = "[ERROR]";
		break;
	case LOG_WARNING:
		colour = VT100_COLOUR_YELLOW;
		prefix = "[WARN ]";
		break;
	case LOG_NOTICE:
		colour = VT100_COLOUR_GREEN;
		prefix = "[NOTE ]";
		break;
	case LOG_INFO:
		colour = VT100_COLOUR_CYAN;
		prefix = "[INFO ]";
		break;
	case LOG_DEBUG:
		colour = "";
		prefix = "[DEBUG]";
		break;
	case LOG_NONE:
	default:
		colour = "";
		prefix = "";
		break;
	}

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		if (fdLog) {
			fprintf(fdLog, "%s %s", prefix, buf);
			fflush(fdLog);
		}

		if (bConsoleEnabled)
			printf("%s%s%s %s", colour, prefix, VT100_DEFAULT, buf);

		free(buf);
	}

	va_end(args);
}

HOOKEXT const char* GetLowHighScale(BYTE bScale) {
	if (!bScale)
		return "None";
	if (bScale < 60)
		return "Low";
	if (bScale < 120)
		return "Medium";
	if (bScale < 180)
		return "High";
	return "Very High";
}

HOOKEXT BOOL FileExists(const char* name) {
	FILE* fdTest;
	fdTest = old_fopen(name, "r");
	if (fdTest) {
		fclose(fdTest);
		return TRUE;
	}
	return FALSE;
}

HOOKEXT const char* GetFileBaseName(const char* szPath) {
	char szName[MAX_PATH] = { 0 };
	char szExt[MAX_PATH] = { 0 };
	char* szBaseName = (char*)malloc(MAX_PATH);
	if (!szBaseName)
		return NULL;

	_splitpath_s(szPath, NULL, 0, NULL, 0, szName, MAX_PATH, szExt, MAX_PATH);
	sprintf_s(szBaseName, MAX_PATH, "%s%s", szName, szExt);
	return szBaseName;
}

HOOKEXT const char* GetModsFolderPath(void) {
	static char szModsFolderPath[MAX_PATH];

	sprintf_s(szModsFolderPath, MAX_PATH, "%s\\%s", szGamePath, SC2KFIX_MODSFOLDER);
	return szModsFolderPath;
}

HOOKEXT BOOL WritePrivateProfileIntA(const char *section, const char *name, int value, const char *ini_name) {
	char szBuf[128 + 1];

	memset(szBuf, 0, sizeof(szBuf));

	sprintf_s(szBuf, sizeof(szBuf) - 1, "%d", value);
	return WritePrivateProfileStringA(section, name, szBuf, ini_name);
}

HOOKEXT const char* GetOnIdleStateEnumName(int iState) {
	if (iState < ONIDLE_STATE_INGAME || iState >= ONIDLE_STATE_COUNT)
		return "(invalid iState)";
	return szOnIdleStateEnums[iState + 1];
}

HOOKEXT const char* GetOnIdleInitialDialogEnumName(int iInitialDialogState) {
	if (iInitialDialogState < ONIDLE_INITIALDIALOG_NONE || iInitialDialogState >= ONIDLE_INITIALDIALOG_COUNT)
		return "(invalid iInitialDialogState)";
	return szOnIdleInitialDialogEnums[iInitialDialogState];
}

static BOOL IsBadFileCharacter(char c) {
	// Note: This takes out the most common
	// invalid filename character cases.
	if (c >= 0x00 && c <= 0x1F)
		return TRUE;
	if (c == '<' || c == '>' ||
		c == ':' || c == '"' ||
		c == '/' || c == '\\' ||
		c == '|' || c == '?' ||
		c == '*' || c == 0x7F)
		return TRUE;
	return FALSE;
}

HOOKEXT BOOL IsFileNameValid(const char *pName) {
	if (!pName)
		return FALSE;

	const char *pTemp = pName;
	for (; *pTemp; pTemp++)
		if (IsBadFileCharacter(*pTemp))
			return FALSE;
	return TRUE;
}

BOOL CopyReplacementString(char *pDest, rsize_t SizeInBytes, const char *pSrc) {
	if (!strcpy_s(pDest, SizeInBytes, pSrc) && jsonSettingsCore[C_SC2KFIX][S_FIX_QOL][I_FIX_QOL_USENEWSTRINGS].ToBool())
		return TRUE;
	return FALSE;
}

char *ConvertFileTypeFilterString(const char *pInStr) {
	const char *startChar;
	char *currChar;
	static char szOutStr[1024 + 1];

	memset(szOutStr, 0, sizeof(szOutStr));
	strcpy_s(szOutStr, pInStr);
	for (startChar = szOutStr;; startChar = currChar + 1) {
		currChar = (char *)strchr(startChar, '|');
		if (!currChar)
			break;
		*currChar = 0;
	}

	return szOutStr;
}

// This is done deliberately in order
// to allow for a log file to be opened
// while the program is active - fopen_s
// doesn't allow for this while fopen does.
#pragma warning(disable:4996)
FILE* log_fopen(const char* fname, const char* mode) {
	return fopen(fname, mode);
}
#pragma warning(default:4996)

FILE* old_fopen(const char* fname, const char* mode) {
	FILE* f;
	if (!fopen_s(&f, fname, mode))
		return f;
	return NULL;
}

void *__cdecl L_ReallocateDataEntry(char *pDest, char *pSrc) {
	DWORD dwCurr;
	DWORD dwDiff;
	char *pDestPtr;
	DWORD dwSize;
	DWORD dwPos;
	void *pNew;

	dwCurr = GlobalSize(pDest);
	dwDiff = 0;
	if (pSrc != pDest) {
		do
			pDestPtr = &pDest[++dwDiff];
		while (pDestPtr != pSrc);
	}
	dwSize = dwCurr - dwDiff;
	for (dwPos = 0; dwSize > dwPos; ++dwPos)
		pDest[dwPos] = pSrc[dwPos];
	pNew = realloc(pDest, dwSize);
	return (pNew) ? pNew : pDest;
}

void SetRGBEntry(RGBQUAD *pRGB, BYTE r, BYTE g, BYTE b) {
	pRGB->rgbRed = r;
	pRGB->rgbGreen = g;
	pRGB->rgbBlue = b;
	pRGB->rgbReserved = 0;
}

int L_GetTranslatedDOSMacPaletteIdx(BYTE palIdx, int nType) {
	// In the tiny/small equivalents of the
	// "Hangar1", "Loading Bay" and "Crane" tiles
	// instead of index 0xB3 it uses 0x34 - consider
	// this as a conversion option during translation.
	if (palIdx == 0xE8) {
		if (nType == 0)
			return 0x34;
		else if (nType == 2)
			return 0x00;
	}
	return DOSMacPalTable[palIdx];
}

int L_GetAdjustedPaletteIdx(BYTE palIdx, int nType) {
	// In the tiny/small equivalents of the
	// "Hangar1", "Loading Bay" and "Crane" tiles
	// instead of index 0xB3 it uses 0x34 - consider
	// this as a conversion option during translation.
	if (nType == 3) {
		if ((palIdx >= 0x0A && palIdx <= 0x0F) ||
			(palIdx >= 0xE8 && palIdx <= 0xF5))
			return 0x00;
	}
	if (palIdx == 0xE8) {
		if (nType == 1)
			return 0xB3;
		else if (nType == 2)
			return 0x00;
		else
			return 0x34;
	}
	return palIdx;
}

void L_InitDOSMacPaletteIdxTable() {
	int i;

	for (i = 0; i < 256; ++i) {
		if (i >= 0 && i < 204)
			DOSMacPalTable[i] = i + 16;
		else if (i >= 224 && i < 239)
			DOSMacPalTable[i] = i;
		else
			DOSMacPalTable[i] = 0;
	}

	// The values set below are the previously unavailable indices
	// that have now been re-introduced into the main game palette
	// on the Windows version of the game.
	DOSMacPalTable[1] =   0x00; // This was previously 0x11 - which would "pick" within the animated range - not desirable.
	DOSMacPalTable[204] = 0x0A;
	DOSMacPalTable[205] = 0x0B;
	DOSMacPalTable[206] = 0x0C;
	DOSMacPalTable[207] = 0x0D;
	DOSMacPalTable[208] = 0x0E;
	DOSMacPalTable[209] = 0x0F;
	DOSMacPalTable[210] = 0xE8;
	DOSMacPalTable[211] = 0xE9;
	DOSMacPalTable[212] = 0xEA;
	DOSMacPalTable[213] = 0xEB;
	DOSMacPalTable[214] = 0xEC;
	DOSMacPalTable[215] = 0xED;
	DOSMacPalTable[216] = 0xEE;
	DOSMacPalTable[217] = 0xEF;
	DOSMacPalTable[218] = 0xF0;
	DOSMacPalTable[219] = 0xF1;
	DOSMacPalTable[220] = 0xF2;
	DOSMacPalTable[221] = 0xF3;
	DOSMacPalTable[222] = 0xF4;
	DOSMacPalTable[223] = 0xF5;
	for (i = 0; i < 8; ++i)
		DOSMacPalTable[232 + i] = 0xB3 + i;
	DOSMacPalTable[255] = 0xFF; // Only used during DOS conversion, a bad idea for Mac.

	ConsoleLog(LOG_INFO, "Initialize DOS/Mac -> Windows Palette Index Table.\n");
}

int L_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	if (hInstance == hSC2KAppModule) {
		switch (uID) {
		case 97:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Hydroelectric Dam"))
				return strlen(lpBuffer);
			break;
#if MAP_EDGE_BUILDING == 2
		case 105:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Sorry, you cannot\r\nplace items off\r\nthe edge of the map."))
				return strlen(lpBuffer);
			break;
#endif
		case 108:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Hydroelectric dams can only be placed on waterfall tiles."))
				return strlen(lpBuffer);
			break;
		case 111:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would intersect an existing tunnel."))
				return strlen(lpBuffer);
			break;
		case 112:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would leave the city limits."))
				return strlen(lpBuffer);
			break;
		case 113:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would be too deep in the terrain."))
				return strlen(lpBuffer);
			break;
		case 114:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as the exit terrain is unstable."))
				return strlen(lpBuffer);
			break;
		case 115:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"An existing subway or sewer line is blocking construction."))
				return strlen(lpBuffer);
			break;
		case 116:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Tunnel entrances must be placed on a hillside."))
				return strlen(lpBuffer);
			break;
		case 129:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Nuclear Power"))
				return strlen(lpBuffer);
			break;
		case 132:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Microwave Power"))
				return strlen(lpBuffer);
			break;
		case 133:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Fusion Power"))
				return strlen(lpBuffer);
			break;
		case 240:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Your nation's military is interested in building a base on your city's soil. "
				"This could mean extra revenue. It could also raise new problems. "
				"Do you wish to grant land to the military?"))
				return strlen(lpBuffer);
			break;
		case 289:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Current rates are %d%%.\r\n"
				"Do you wish to issue the bond?"))
				return strlen(lpBuffer);
			break;
		case 290:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"You need $10,000 in cash to repay an outstanding bond."))
				return strlen(lpBuffer);
			break;
		case 291:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"The oldest outstanding bond rate is %d%%.\r\n"
				"Do you wish to repay this bond?"))
				return strlen(lpBuffer);
			break;
		case 346:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Engineers report that tunnel construction costs will be %s.\r\n"
				"Do you wish to construct the tunnel?"))
				return strlen(lpBuffer);
			break;
		case 640:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Grocery store"))
				return strlen(lpBuffer);
			break;
		case 745:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Launch Arcology"))
				return strlen(lpBuffer);
			break;
		case 4002:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"SimCity 2000 City (*.SC2)|*.SC2|SimCity Classic City (*.CTY)|*.CTY||"))
				return strlen(lpBuffer);
			break;
		case 4004:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"SimCity 2000 Tilesets (*.MIF, *.TIL)|*.MIF;*.TIL|SimCity 2000 Win/Mac Tilesets (*.MIF)|*.MIF|SimCity 2000 DOS Tilesets (*.TIL)|*.TIL||"))
				return strlen(lpBuffer);
			break;
		case 32921:
			if (CopyReplacementString(lpBuffer, cchBufferMax,
				"Saves city every 5 years"))
				return strlen(lpBuffer);
			break;
		default:
			break;
		}
	}
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

const char *GetFixedTileType(int nTileSet) {
	switch (nTileSet) {
		case IDR_TSET_FIXTIL_HORZOFF:
			return "Horizontal Offset";
		case IDR_TSET_FIXTIL_VERTOFF:
			return "Vertical Offset";
		case IDR_TSET_FIXTIL_BADPALIDX:
			return "Bad Palette Index";
		case IDR_TSET_FIXTIL_MISSPIXELS:
			return "Missing Pixels";
		case IDR_TSET_FIXTIL_OOBPALIDX:
			return "Out-of-bounds Palette Index";
		case IDR_TSET_FIXTIL_HANGAROPEN:
			return "Hangar1 Open (Black)";
		case IDR_TSET_FIXTIL_HANGARANIM:
			return "Hangar1 Anim (Grey)";
		case IDR_TSET_FIXTIL_HANGARSHUT:
			return "Hangar1 Shut (Yellow)";
		default:
			break;
	}
	return "";
}

int L_byteswap_longlabel(char *pBuf) {
	return _byteswap_ulong(*(DWORD *)pBuf);
}

void L_byteswap_buffer(DWORD *pBuf, int nCount) {
	for (int nPos = 0; int(nCount / 4) > nPos; ++nPos)
		pBuf[nPos] = _byteswap_ulong(pBuf[nPos]);
}

void L_byteswap_micro(WORD *pBuf, unsigned int nCount) {
	for (int nPos = 0; nPos < int(nCount >> 3); ++nPos) {
		pBuf[1] = _byteswap_ushort(pBuf[1]);
		pBuf[2] = _byteswap_ushort(pBuf[2]);
		pBuf[3] = _byteswap_ushort(pBuf[3]);
		pBuf += 4;
	}
}

void L_byteswap_ushorts(WORD *pBuf, int nCount) {
	for (int nPos = 0; int(nCount / 2) > nPos; ++nPos)
		pBuf[nPos] = _byteswap_ushort(pBuf[nPos]);
}

void L_CharStringToPascalString(const char *pInStr, char *pOutStr, int nMaxSize, bool bFixedSize) {
	int nAbsMaxSize, nLen, nDiffLen, nStoredSize;

	if (!pInStr)
		return;
	nAbsMaxSize = nMaxSize;
	if (nAbsMaxSize > 255)
		nAbsMaxSize = 255;
	nLen = strlen(pInStr);
	if (nLen > nAbsMaxSize)
		nLen = nAbsMaxSize;
	if (nLen > 255)
		nLen = 255;
	nDiffLen = nAbsMaxSize - nLen;
	memcpy(pOutStr, pInStr, nLen);
	// Only zero the remainder in a
	// fixed size situation.
	if (bFixedSize)
		memset(&pOutStr[nLen], 0, nDiffLen);
	for (int nPos = nLen - 1; nPos >= 0; --nPos)
		pOutStr[nPos + 1] = pOutStr[nPos];
	nStoredSize = (bFixedSize) ? nAbsMaxSize : nLen;
	memset(&pOutStr[0], nStoredSize, 1);
}

bool L_PascalStringToCharString(const char *pInStr, char *pOutStr) {
	int nPos;
	char c;

	if (!pInStr || strlen(pInStr) == 0)
		return false;
	c = *pInStr;
	for (nPos = 0; c > nPos; ++nPos)
		pOutStr[nPos] = pInStr[nPos + 1];
	pOutStr[nPos] = 0;
	return true;
}

// start of base64 code
/*
* Base64 encoding/decoding (RFC1341)
* Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
*
* This software may be distributed under the terms of the BSD license.
*/

static const unsigned char base64_encodetable[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_decodetable[256] = {
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	62, 128, 128, 128, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
	61, 128, 128, 128, 0, 128, 128, 128, 0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 128, 128, 128, 128, 128, 128, 26, 27,
	28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128
};

// Encodes a block of memory as base64 and returns it as a std::string.
HOOKEXT_CPP std::string Base64Encode(const unsigned char* pSrcData, size_t iSrcCount) {
	unsigned char* out, * pos;
	const unsigned char* end, * in;

	size_t olen;

	olen = 4 * ((iSrcCount + 2) / 3); /* 3-byte blocks to 4-byte */

	if (olen < iSrcCount)
		return std::string(); /* integer overflow */

	std::string outStr;
	outStr.resize(olen);
	out = (unsigned char*)&outStr[0];

	end = pSrcData + iSrcCount;
	in = pSrcData;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base64_encodetable[in[0] >> 2];
		*pos++ = base64_encodetable[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_encodetable[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_encodetable[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_encodetable[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_encodetable[(in[0] & 0x03) << 4];
			*pos++ = '=';
		}
		else {
			*pos++ = base64_encodetable[((in[0] & 0x03) << 4) |
				(in[1] >> 4)];
			*pos++ = base64_encodetable[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	return outStr;
}

// Decodes a base64 string into a memory buffer. Returns the number of bytes actually written.
HOOKEXT_CPP size_t Base64Decode(BYTE* pBuffer, size_t iBufSize, const unsigned char* pSrcData, size_t iSrcCount) {
	unsigned char* pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	count = 0;
	for (i = 0; i < iSrcCount; i++) {
		if (base64_decodetable[pSrcData[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return 0;

	olen = count / 4 * 3;
	if (olen > iBufSize) {
		return 0;
	}
	pos = pBuffer;
	if (pBuffer == NULL) {
		return 0;
	}

	count = 0;
	for (i = 0; i < iSrcCount; i++) {
		tmp = base64_decodetable[pSrcData[i]];
		if (tmp == 0x80)
			continue;

		if (pSrcData[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					return 0;
				}
				break;
			}
		}
	}

	return pos - pBuffer;
}

// end of base64 code

// Decompresses a MaxisRLE blob into a buffer
int MaxisDecompress(BYTE* pBuffer, size_t iBufSize, BYTE* pCompressedData, int iCompressedSize) {
	int i = 0, j = 0;

	for (; i < iCompressedSize && j < iBufSize;) {
		if (pCompressedData[i] < 128) {
			memcpy(pBuffer + j, pCompressedData + i + 1, pCompressedData[i]);
			j += pCompressedData[i];
			i += pCompressedData[i] + 1;
		}
		else if (pCompressedData[i] > 128) {
			memset(pBuffer + j, pCompressedData[i + 1], pCompressedData[i] - 127);
			j += pCompressedData[i] - 127;
			i += 2;
		}
		else
			ConsoleLog(LOG_WARNING, "LOAD: Unexpected 0x80 in MaxisDecompress. This should never happen.\n");
	}

	if (sc2x_debug & 4)
		ConsoleLog(LOG_DEBUG, "LOAD: Uncompressed %d bytes into %d bytes.\n", i, j);
	return j;
}

HOOKEXT_CPP json::JSON json::Array() {
	return std::move(json::JSON::Make(json::JSON::Class::Array));
}

HOOKEXT_CPP json::JSON json::Object() {
	return std::move(JSON::Make(JSON::Class::Object));
}

HOOKEXT_CPP std::ostream& json::operator<<(std::ostream& os, const json::JSON& json) {
	os << json.dump();
	return os;
}

HOOKEXT_CPP json::JSON json::JSON::Load(const string& str) {
	size_t offset = 0;
	return std::move(parse_next(str, offset));
}

console::CommandTree console::Object() {
	return std::move(CommandTree::Make(CommandTree::Class::Object));
}

// Transforms an array of 32-bit integers into a JSON array, including endian swapping if needed
HOOKEXT_CPP json::JSON EncodeDWORDArray(DWORD* dwArray, size_t iCount, BOOL bBigEndian) {
	json::JSON jsonArray = json::Array();
	for (size_t i = 0; i < iCount; i++) {
		if (bBigEndian)
			jsonArray.append<DWORD>(SwapDWORD(dwArray[i]));
		else
			jsonArray.append<DWORD>(dwArray[i]);
	}
	return jsonArray;
}

// Transforms a budget array into a JSON array, including endian swapping if needed
HOOKEXT_CPP json::JSON EncodeBudgetArray(DWORD* dwBudgetArray, BOOL bBigEndian) {
	json::JSON jsonObject = json::Object();
	jsonObject["iCurrentCosts"] = DWORD_NTOHL_CHECK(dwBudgetArray[0]);
	jsonObject["iFundingPercent"] = DWORD_NTOHL_CHECK(dwBudgetArray[1]);
	jsonObject["iYearToDateCost"] = DWORD_NTOHL_CHECK(dwBudgetArray[2]);

	jsonObject["iCountMonth"] = json::Array<DWORD>(
		DWORD_NTOHL_CHECK(dwBudgetArray[3]), DWORD_NTOHL_CHECK(dwBudgetArray[5]), DWORD_NTOHL_CHECK(dwBudgetArray[7]),
		DWORD_NTOHL_CHECK(dwBudgetArray[9]), DWORD_NTOHL_CHECK(dwBudgetArray[11]), DWORD_NTOHL_CHECK(dwBudgetArray[13]),
		DWORD_NTOHL_CHECK(dwBudgetArray[15]), DWORD_NTOHL_CHECK(dwBudgetArray[17]), DWORD_NTOHL_CHECK(dwBudgetArray[19]),
		DWORD_NTOHL_CHECK(dwBudgetArray[21]), DWORD_NTOHL_CHECK(dwBudgetArray[23]), DWORD_NTOHL_CHECK(dwBudgetArray[25]));
	jsonObject["iFundMonth"] = json::Array<DWORD>(
		DWORD_NTOHL_CHECK(dwBudgetArray[4]), DWORD_NTOHL_CHECK(dwBudgetArray[6]), DWORD_NTOHL_CHECK(dwBudgetArray[8]),
		DWORD_NTOHL_CHECK(dwBudgetArray[10]), DWORD_NTOHL_CHECK(dwBudgetArray[12]), DWORD_NTOHL_CHECK(dwBudgetArray[14]),
		DWORD_NTOHL_CHECK(dwBudgetArray[16]), DWORD_NTOHL_CHECK(dwBudgetArray[18]), DWORD_NTOHL_CHECK(dwBudgetArray[20]),
		DWORD_NTOHL_CHECK(dwBudgetArray[22]), DWORD_NTOHL_CHECK(dwBudgetArray[24]), DWORD_NTOHL_CHECK(dwBudgetArray[26]));
	return jsonObject;
}

// Transforms a JSON array of integers into an array of 32-bit integers at a memory location.
// WARNING: Be very sure you know what you're doing with this function, as it will happily over-
// flow a buffer if you specify an iCount higher than the number of elements in the target array.
HOOKEXT_CPP void DecodeDWORDArray(DWORD* dwArray, json::JSON jsonArray, size_t iCount, BOOL bBigEndian) {
	for (size_t i = 0; i < iCount; i++)
		dwArray[i] = (bBigEndian ? SwapDWORD(jsonArray[i].ToInt()) : jsonArray[i].ToInt());
}

// Returns a std::string that's a clone of the parameter but entirely lowercase
HOOKEXT_CPP std::string string_tolower(std::string& str) {
	std::string strNew = str;
	std::transform(strNew.begin(), strNew.end(), strNew.begin(), std::tolower);
	return strNew;
}

// Returns a std::string that's a clone of the parameter but entirely uppercase
HOOKEXT_CPP std::string string_toupper(std::string& str) {
	std::string strNew = str;
	std::transform(strNew.begin(), strNew.end(), strNew.begin(), std::toupper);
	return strNew;
}

// Similar to std::string::starts_with in C++20
HOOKEXT_CPP bool string_starts_with(std::string& str, const char* prefix) {
	return (str.rfind(prefix, 0) != std::string::npos);
}

// Similar to std::string::ends_with in C++20
HOOKEXT_CPP bool string_ends_with(std::string& str, const char* suffix) {
	return (str.find(suffix, str.length() - strlen(suffix)) != std::string::npos);
}

// Similar to std::string::contains in C++20
HOOKEXT_CPP bool string_contains(std::string& str, const char* substr) {
	return (str.find(substr) != std::string::npos);
}

// C++ string wrapper for vsprintf_s
HOOKEXT_CPP std::string string_format(const char* fmt, ...) {
	va_list args;
	int len;
	char* buf;

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	std::string str(len, '\0');
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);
		str = buf;
		free(buf);
	}

	va_end(args);
	return str;
}

// Splits a std::string into a vector array of strings for CLI parsing
HOOKEXT_CPP bool string_split(std::string str, std::vector<std::string>& qargs) {
	int len = str.length();
	bool qot = false, sqot = false;
	int arglen;
	qargs.clear();

	for (int i = 0; i < len; i++) {
		int start = i;
		if (str[i] == '\"')
			qot = true;

		else if (str[i] == '\'') sqot = true;

		if (qot) {
			i++;
			start++;
			while (i < len && str[i] != '\"')
				i++;
			if (i < len)
				qot = false;
			arglen = i - start;
			i++;
		}
		else if (sqot) {
			i++;
			start++;
			while (i < len && str[i] != '\'')
				i++;
			if (i < len)
				sqot = false;
			arglen = i - start;
			i++;
		}
		else {
			while (i < len && str[i] != ' ')
				i++;
			arglen = i - start;
		}
		qargs.push_back(str.substr(start, arglen));
	}

	// Return false if there's a syntax error, true if not
	if (qot || sqot)
		return false;
	return true;
}