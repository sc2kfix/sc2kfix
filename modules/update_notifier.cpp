// sc2kfix modules/update_notifier.cpp: exactly what it says on the tin
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include "../resource.h"

#define UPDATENOTIFIER_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef UPDATENOTIFIER_DEBUG
#define UPDATENOTIFIER_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT updatenotifier_debug = UPDATENOTIFIER_DEBUG;

const char* szGitHubRepoReleases = "https://api.github.com/repos/sc2kfix/sc2kfix/releases?per_page=1";
const char* szGitHubAPIType[] = { "Accept: application/vnd.github+json", NULL };
char szLatestRelease[24] = { 0 };
BOOL bUpdateAvailable = FALSE;

DWORD WINAPI UpdaterThread(LPVOID lpParameter) {
	// Give the main thread time to finish DLL init then check for updates and return
	Sleep(200);
	bUpdateAvailable = UpdaterCheckForUpdates();
	return EXIT_SUCCESS;
}

#define FETCHBUF_SIZE 4096
#define BUF_SIZE (FETCHBUF_SIZE * 4)

BOOL UpdaterCheckForUpdates(void) {
	DWORD dwContext;
	HINTERNET hInet = InternetOpen("sc2kfix Update Notifier", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
	if (!hInet) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  Couldn't open hInet.\n");
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetOpen()\n");

	DWORD dwTimeoutMilliseconds = 3000;
	InternetSetOption(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeoutMilliseconds, sizeof(DWORD));

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetSetOption()\n");

#pragma warning(push)
#pragma warning(disable:6385)
	HINTERNET hHttpRequest = InternetOpenUrl(hInet, szGitHubRepoReleases, "X-GitHub-Api-Version: 2022-11-28\r\n", -1L, INTERNET_FLAG_SECURE, (DWORD_PTR)&dwContext);
#pragma warning(pop)
	if (!hHttpRequest) {
		if (GetLastError() == 12002)
			ConsoleLog(LOG_WARNING, "UPD:  Update notifier timed out after 3000 milliseconds.\n");
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  InternetOpenUrl failed, 0x%08X.\n", GetLastError());
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetOpenUrl()\n");

	DWORD dwBytesGrabbed = 0;
	DWORD dwBytesReceived = 0;
	char szTempBuf[FETCHBUF_SIZE + 1];
	char* szBuffer = (char*)malloc(BUF_SIZE + 1);
	if (!szBuffer) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  Couldn't allocate szBuffer.\n");
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  malloc()\n");

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Bytes to grab: %u\n", BUF_SIZE);

	// Fetch in FETCHBUF_SIZE chunks.
	while (InternetReadFile(hHttpRequest, szTempBuf, sizeof(szTempBuf) - 1, &dwBytesGrabbed)) {
		if (dwBytesGrabbed == 0 || (dwBytesReceived + dwBytesGrabbed) >= BUF_SIZE)
			break;
		
		dwBytesReceived += dwBytesGrabbed;
		strcat_s(szBuffer, BUF_SIZE, szTempBuf);
		memset(szTempBuf, 0, sizeof(szTempBuf));
	}

	szBuffer[BUF_SIZE + 1] = 0;

	// This was previously in the prior InternetReadFile 'if' block (when it was checking
	// for it to return 0); leave this here for review.
	//
	//if (updatenotifier_debug)
	//	ConsoleLog(LOG_ERROR, "UPD:  InternetReadFile failed, 0x%08X.\n", GetLastError());
	//goto FAIL;

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetReadFile()\n");

	InternetCloseHandle(hHttpRequest);
	InternetCloseHandle(hInet);

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  InternetCloseHandle()\n");

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Bytes received: %u\n", dwBytesReceived);

	char* szTagString = strstr(szBuffer, "\"tag_name\"");
	if (!szTagString) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  No release tag name found (1).\n");
		free(szBuffer);
		return FALSE;
	}

	char* szQuoteComma = strstr(szTagString, "\",");
	char szThrowaway[64] = { 0 };
	strncpy_s(szThrowaway, 64, szTagString, szQuoteComma - szTagString);

	int n = sscanf_s(szThrowaway, "\"tag_name\":\"%s\"", szLatestRelease, 23);
	if (!*szLatestRelease) {
		if (updatenotifier_debug)
			ConsoleLog(LOG_ERROR, "UPD:  No release tag name found (2) - scanned %i fields.\n", n);
		free(szBuffer);
		return FALSE;
	}

	if (updatenotifier_debug)
		ConsoleLog(LOG_DEBUG, "UPD:  Latest release: %s\n", szLatestRelease);

	if (strcmp(szSC2KFixReleaseTag, szLatestRelease)) {
		ConsoleLog(LOG_INFO, "UPD:  New release available: %s (currently running %s)\n", szLatestRelease, szSC2KFixReleaseTag);
		if (dwDetectedVersion == VERSION_SC2K_1996)
			PostMessage(hwndMainDialog_SC2K1996, WM_SC2KFIX_UPDATE, 0, 1);
		free(szBuffer);
		return TRUE;
	}
	
	free(szBuffer);
	return FALSE;
}
