// sc2kfix hooks/hook_movies.cpp: these are the movie-related hooks.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <time.h>

#include <sc2kfix.h>

#define MOVIE_DEBUG_CALLS 1

#define MOVIE_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MOVIE_DEBUG
#define MOVIE_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mov_debug = MOVIE_DEBUG;

static DWORD dwDummy;

BOOL bSkipIntro = FALSE;
static HMODULE hMod_SMK = 0;

extern "C" BOOL __cdecl Hook_MovieOpen(char* sMovStr) {
	CMFC3XString movStr;
	int nWidth, nHeight;
	int nBufWidth, nBufHeight;
	void *pMovBuf;
	BYTE iRev;
	MSG Msg;
	BOOL bBreakout;

	if (sMovStr && strncmp(sMovStr, "INTRO", 5) == 0)
		if (bSkipIntro || bSettingsAlwaysSkipIntro)
			return TRUE;

	MovieWndInitFinish = FALSE;
	MovieWndExit = FALSE;
	GameMain_SmackSoundUseDirectSound(hWndMovie);

	GameMain_String_Cons(&movStr);
	GameMain_String_Format(&movStr, "%s\\Movies\\%s", szGamePath, sMovStr);

	if (mov_debug & MOVIE_DEBUG_CALLS)
		ConsoleLog(LOG_DEBUG, "0x%06X -> MovieOpen(%s): [%s]\n", _ReturnAddress(), sMovStr, movStr.m_pchData);

	smkOpenRet = (DWORD *)GameMain_SmackOpen(movStr.m_pchData, 0xFE000, -1);
	if (smkOpenRet) {
		hWndMovieCap = SetCapture(hWndMovie);
		nWidth = smkOpenRet[1];
		nHeight = smkOpenRet[2];
		smkBufOpenRet = (DWORD *)GameMain_SmackBufferOpen(hWndMovie, 0, nWidth, nHeight, nWidth, nHeight);
		if (smkBufOpenRet) {
			nBufWidth = smkBufOpenRet[4];
			nBufHeight = smkBufOpenRet[5];
			pMovBuf = (void *)smkBufOpenRet[271];
			iRev = *(BYTE *)smkBufOpenRet;
			GameMain_SmackToBuffer(smkOpenRet,
				(nBufWidth - nWidth) >> 1,
				(nBufHeight - nHeight) >> 1,
				nBufWidth,
				nBufHeight,
				pMovBuf,
				iRev);
			bBreakout = FALSE;
			do {
				while (PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE)) {
					if (Msg.message == WM_QUIT) {
						bBreakout = TRUE;
						break;
					}
					TranslateMessage(&Msg);
					DispatchMessageA(&Msg);
				}
				if (bBreakout)
					break;
			} while (GameMain_SmackWait(smkOpenRet) || Game_MoviePlay(hWndMovie));
			GameMain_SmackBufferClose(smkBufOpenRet);
		}
		GameMain_SmackClose(smkOpenRet);
		if (hWndMovieCap)
			SetCapture(hWndMovieCap);
		smkBufOpenRet = 0;
		GameMain_String_Dest(&movStr);
		return MovieWndExit;
	}
	else {
		GameMain_String_Dest(&movStr);
		return FALSE;
	}
}

extern "C" BOOL __cdecl Hook_MovieCheck(char* sMovStr) {
	CMFC3XString movStr;
	CMFC3XFileStatus fileStat;
	BOOL bRet;

	bRet = FALSE;
	if (sMovStr && strncmp(sMovStr, "INTRO", 5) == 0)
		if (bSkipIntro || bSettingsAlwaysSkipIntro)
			return TRUE;

	GameMain_String_Cons(&movStr);
	GameMain_String_Format(&movStr, "%s\\Movies\\%s", szGamePath, sMovStr);
	
	if (mov_debug & MOVIE_DEBUG_CALLS)
		ConsoleLog(LOG_DEBUG, "0x%06X -> MovieCheck(%s): [%s]\n", _ReturnAddress(), sMovStr, movStr.m_pchData);
	
	bRet = GameMain_File_GetStatusWithString(movStr.m_pchData, &fileStat);
	GameMain_String_Dest(&movStr);

	return bRet;
}

void InstallMovieHooks(void) {
	ConsoleLog(LOG_INFO, "MOV:  Loaded Movie Hooks.\n");

	// Hook into the movie opening function.
	VirtualProtect((LPVOID)0x401104, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401104, Hook_MovieOpen);

	// Hook into the movie checking function.
	VirtualProtect((LPVOID)0x402360, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402360, Hook_MovieCheck);
}
