// sc2kfix utility.cpp: utility functions to save me from reinventing the wheel
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include "resource.h"

void CenterDialogBox(HWND hwndDlg) {
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

HWND CreateTooltip(HWND hDlg, HWND hControl, const char* szText) {
	if (!hDlg || !hControl || !szText)
		return NULL;

	HWND hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, hDlg, NULL, hSC2KFixModule, NULL);
	if (!hTooltip)
		return NULL;

	char* lpszText = _strdup(szText);
	if (!lpszText)
		return NULL;

	SendMessage(hTooltip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);

	TOOLINFO tooltipInfo = { 0 };
	tooltipInfo.cbSize = sizeof(TOOLINFO);
	tooltipInfo.hwnd = hDlg;
	tooltipInfo.uId = (UINT_PTR)hControl;
	tooltipInfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	tooltipInfo.lpszText = lpszText;
	SendMessage(hTooltip, TTM_ADDTOOL, NULL, (LPARAM)&tooltipInfo);

	return hTooltip;
}

const char* HexPls(UINT uNumber, int width) {
	thread_local char szRet[16] = { 0 };
	sprintf_s(szRet, 16, "0x%0*X", width, uNumber);
	return szRet;
}

extern FILE* fdLog;

void ConsoleLog(int iLogLevel, const char* fmt, ...) {
	va_list args;
	int len;
	char* buf;
	const char* prefix;

	switch (iLogLevel) {
	case LOG_EMERGENCY:
		prefix = "[EMERG] ";
		break;
	case LOG_ALERT:
		prefix = "[ALERT] ";
		break;
	case LOG_CRITICAL:
		prefix = "[CRIT ] ";
		break;
	case LOG_ERROR:
		prefix = "[ERROR] ";
		break;
	case LOG_WARNING:
		prefix = "[WARN ] ";
		break;
	case LOG_NOTICE:
		prefix = "[NOTE ] ";
		break;
	case LOG_INFO:
		prefix = "[INFO ] ";
		break;
	case LOG_DEBUG:
		prefix = "[DEBUG] ";
		break;
	case LOG_NONE:
	default:                            // XXX - can this be a constexpr error?
		prefix = "";
		break;
	}

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		if (fdLog) {
			fprintf(fdLog, "%s%s", prefix, buf);
			fflush(fdLog);
		}

		if (bConsoleEnabled)
			printf("%s%s", prefix, buf);

		free(buf);
	}

	va_end(args);
}

int GetTileID(int iTileX, int iTileY) {
	if (iTileX >= 0 && iTileX < 128 && iTileY >= 0 && iTileY < 128)
		return dwMapXBLD[iTileX]->iTileID[iTileY];
	else
		return -1;
}

const char* GetLowHighScale(BYTE bScale) {
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

BOOL FileExists(const char* name) {
	FILE* fdTest;
	if (!fopen_s(&fdTest, name, "r")) {
		fclose(fdTest);
		return TRUE;
	}
	return FALSE;
}

BOOL WritePrivateProfileIntA(const char *section, const char *name, int value, const char *ini_name) {
	char szBuf[128 + 1];

	memset(szBuf, 0, sizeof(szBuf));

	sprintf_s(szBuf, sizeof(szBuf) - 1, "%d", value);
	return WritePrivateProfileStringA(section, name, szBuf, ini_name);
}

void MigrateRegStringValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, char *szOutBuf, DWORD dwLen) {
	DWORD dwOutBufLen = dwLen;
	RegGetValueA(hKey, lpSubKey, lpValueName, RRF_RT_REG_SZ, NULL, szOutBuf, &dwOutBufLen);
}

void MigrateRegDWORDValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, DWORD *dwOut, DWORD dwSize) {
	DWORD dwOutSize = dwSize;
	RegGetValueA(hKey, lpSubKey, lpValueName, RRF_RT_REG_DWORD, NULL, dwOut, &dwOutSize);
}

void MigrateRegBOOLValue(HKEY hKey, const char *lpSubKey, const char *lpValueName, BOOL *bOut) {
	DWORD dwOutSize = sizeof(BOOL);
	RegGetValueA(hKey, lpSubKey, lpValueName, RRF_RT_REG_DWORD, NULL, bOut, &dwOutSize);
}
