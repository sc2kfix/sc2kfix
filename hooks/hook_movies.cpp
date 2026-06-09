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

#define MAX_MOVBUT 5

#define MOVIE_DEBUG_CALLS 1

#define MOVIE_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MOVIE_DEBUG
#define MOVIE_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mov_debug = MOVIE_DEBUG;

static DWORD dwDummy;

extern int nMovZoomFactor;

BOOL bSkipIntro = FALSE;

extern "C" BOOL __cdecl Hook_MovieOpen(char* sMovStr) {
	CMFC3XString movStr;
	int nWidth, nHeight;
	int nBufWidth, nBufHeight;
	void *pMovBuf;
	BYTE iRev;
	MSG Msg;
	BOOL bLoaded, bBreakout;

	MovieWndInitFinish = FALSE;
	MovieWndExit = FALSE;
	GameMain_SmackSoundUseDirectSound(hWndMovie);

	GameMain_String_Cons(&movStr);
	GameMain_String_Format(&movStr, "%s\\Movies\\%s", szGamePath, sMovStr);

	if (mov_debug & MOVIE_DEBUG_CALLS)
		ConsoleLog(LOG_DEBUG, "0x%06X -> MovieOpen(%s): [%s]\n", _ReturnAddress(), sMovStr, movStr.m_pchData);

	bLoaded = FALSE;
	smkOpenRet = (DWORD *)GameMain_SmackOpen(movStr.m_pchData, 0xFE000, -1);
	if (smkOpenRet) {
		bLoaded = TRUE;
		hWndMovieCap = SetCapture(hWndMovie);
		nWidth = smkOpenRet[1] * nMovZoomFactor;
		nHeight = smkOpenRet[2] * nMovZoomFactor;
		smkBufOpenRet = (DWORD *)GameMain_SmackBufferOpen(hWndMovie, 0, nWidth, nHeight, nWidth, nHeight);
		if (smkBufOpenRet) {
			smkBufOpenRet[7] *= nMovZoomFactor;
			smkBufOpenRet[8] *= nMovZoomFactor;

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
	}

	GameMain_String_Dest(&movStr);
	return (bLoaded) ? MovieWndExit : FALSE;
}

extern "C" BOOL __cdecl Hook_MovieCheck(char* sMovStr) {
	CMFC3XString movStr;
	CMFC3XFileStatus fileStat;
	BOOL bRet;

	bRet = FALSE;
	
	GameMain_String_Cons(&movStr);
	GameMain_String_Format(&movStr, "%s\\Movies\\%s", szGamePath, sMovStr);
	
	if (mov_debug & MOVIE_DEBUG_CALLS)
		ConsoleLog(LOG_DEBUG, "0x%06X -> MovieCheck(%s): [%s]\n", _ReturnAddress(), sMovStr, movStr.m_pchData);
	
	bRet = GameMain_File_GetStatusWithString(movStr.m_pchData, &fileStat);
	GameMain_String_Dest(&movStr);

	return bRet;
}

#define PIECE_AREA 70

enum {
	MOVBUT_QUIT,
	MOVBUT_TOPLEFT,
	MOVBUT_TOPRIGHT,
	MOVBUT_BTMLEFT,
	MOVBUT_BTMRIGHT
};

enum {
	MOVACT_NONE,
	MOVACT_FIRST,
	MOVACT_SECOND,
	MOVACT_THIRD,
	MOVACT_FOURTH,
	MOVACT_QUIT
};

static int GetZoomedPieceArea() {
	return PIECE_AREA * nMovZoomFactor;
}

extern "C" BOOL Hook_MovieDialog_OnInitDialog() {
	CMovieDialog *pThis;

	__asm mov [pThis], ecx

	HINSTANCE hModule;
	HRSRC hResInfo, hResInfoButUp, hResInfoButDown;
	HGLOBAL hResData, hResDataButUp, hResDataButDown;
	LOGPAL movPal;
	HPALETTE hPal;
	int nCX, nWndCX, nCY, nWndCY, nPieceArea;
	RECT wndRect, movQuitBut, movTopLeftBut, movTopRightBut, movBottomLeftBut, movBottomRightBut;

	GameMain_Dialog_OnInitDialog(pThis);

	hModule = (HINSTANCE)GetWindowLongA(pThis->m_hWnd, GWL_HINSTANCE);
	if (hModule) {
		hResInfo = FindResourceA(hModule, MAKEINTRESOURCEA(261), RT_BITMAP);
		if (hResInfo) {
			hResData = LoadResource(hModule, hResInfo);
			if (hResData) {
				pThis->pOWMainBitmapInfo = (BITMAPINFO *)LockResource(hResData);
				movPal.wVersion = 768;
				movPal.wNumPalEnts = HICOLORCNT;
				memset(movPal.pPalEnts, 0, sizeof(movPal.pPalEnts));
				for (int nCol = 0; nCol < HICOLORCNT; ++nCol) {
					movPal.pPalEnts[nCol].peRed = pThis->pOWMainBitmapInfo->bmiColors[nCol].rgbRed;
					movPal.pPalEnts[nCol].peGreen = pThis->pOWMainBitmapInfo->bmiColors[nCol].rgbGreen;
					movPal.pPalEnts[nCol].peBlue = pThis->pOWMainBitmapInfo->bmiColors[nCol].rgbBlue;
					movPal.pPalEnts[nCol].peFlags = 0;
				}
				hPal = CreatePalette((const LOGPALETTE *)&movPal);
				GameMain_GdiObject_Attach(&pThis->MovPalette, hPal);
			}
			FreeResource(hResData);
		}

		nCX = GetSystemMetrics(SM_CXSCREEN);
		nCY = GetSystemMetrics(SM_CYSCREEN);

		SetWindowPos(pThis->m_hWnd, 0, 0, 0, nCX, nCY, SWP_NOZORDER | SWP_NOMOVE);

		WORD nBut = 0;
		for (int i = 0; i < MAX_MOVBUT; i++) {
			hResInfoButUp = FindResourceA(hModule, MAKEINTRESOURCEA(wMovButtonsUp[nBut]), RT_BITMAP);
			if (hResInfoButUp) {
				hResDataButUp = LoadResource(hModule, hResInfoButUp);
				if (hResDataButUp)
					pThis->pOWButtonBitmapInfo[i] = (BITMAPINFO *)LockResource(hResDataButUp);
				FreeResource(hResDataButUp);
			}
			hResInfoButDown = FindResourceA(hModule, MAKEINTRESOURCEA(wMovButtonsDown[nBut]), RT_BITMAP);
			if (hResInfoButDown) {
				hResDataButDown = LoadResource(hModule, hResInfoButDown);
				if (hResDataButDown)
					pThis->pOWButtonBitmapInfo[MAX_MOVBUT + i] = (BITMAPINFO *)LockResource(hResDataButDown);
				FreeResource(hResDataButDown);
			}
			nBut += 2;
		}

		GetWindowRect(pThis->m_hWnd, &wndRect);

		nWndCX = (wndRect.right - wndRect.left) / 2;
		nWndCY = (wndRect.bottom - wndRect.top);
		nPieceArea = GetZoomedPieceArea();

		movQuitBut.left = nWndCX - nPieceArea;
		movQuitBut.top = (nWndCY + nPieceArea) / 2;
		movQuitBut.bottom = movQuitBut.top + nPieceArea;
		movQuitBut.right = nWndCX + nPieceArea;

		// Quit button
		CopyRect(&pThis->MovButRECT[MOVBUT_QUIT], &movQuitBut);

		movTopLeftBut.left = movQuitBut.left;
		movTopLeftBut.top = (nWndCY - (nPieceArea * 3)) / 2;
		movTopLeftBut.bottom = movTopLeftBut.top + nPieceArea;
		movTopLeftBut.right = nWndCX;

		// Movie button top-left
		CopyRect(&pThis->MovButRECT[MOVBUT_TOPLEFT], &movTopLeftBut);
		
		movTopRightBut.left = nWndCX;
		movTopRightBut.top = movTopLeftBut.top;
		movTopRightBut.bottom = movTopRightBut.top + nPieceArea;
		movTopRightBut.right = movQuitBut.right;

		// Movie button top-right
		CopyRect(&pThis->MovButRECT[MOVBUT_TOPRIGHT], &movTopRightBut);

		movBottomLeftBut.left = movQuitBut.left;
		movBottomLeftBut.top = (nWndCY - nPieceArea) / 2;
		movBottomLeftBut.bottom = movBottomLeftBut.top + nPieceArea;
		movBottomLeftBut.right = nWndCX;

		// Movie button bottom-left
		CopyRect(&pThis->MovButRECT[MOVBUT_BTMLEFT], &movBottomLeftBut);

		movBottomRightBut.left = nWndCX;
		movBottomRightBut.top = movBottomLeftBut.top;
		movBottomRightBut.bottom = movBottomRightBut.top + nPieceArea;
		movBottomRightBut.right = movQuitBut.right;

		// Movie button bottom-right
		CopyRect(&pThis->MovButRECT[MOVBUT_BTMRIGHT], &movBottomRightBut);
	}
	else	
		EndDialog(pThis->m_hWnd, MOVACT_QUIT);
	return TRUE;
}

void InstallMovieHooks(void) {
	if (mov_debug)
		ConsoleLog(LOG_DEBUG, "MOV:  Loaded movie hooks.\n");

	// Hook into the movie opening function.
	SafeVirtualProtect((LPVOID)0x401104, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401104, Hook_MovieOpen);

	// Hook into the movie checking function.
	SafeVirtualProtect((LPVOID)0x402360, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402360, Hook_MovieCheck);

	// Hook CMovieDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x4018ED, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4018ED, Hook_MovieDialog_OnInitDialog);
}
