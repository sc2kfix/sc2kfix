// sc2kfix hooks/hook_spritesandtilesets.cpp: sprite and tileset handling
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SPRITE_DEBUG_OTHER 1
#define SPRITE_DEBUG_SPRITES 2
#define SPRITE_DEBUG_TILESETS 4

#define SPRITE_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SPRITE_DEBUG
#define SPRITE_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT sprite_debug = SPRITE_DEBUG;

static DWORD dwDummy; 

#define CUSTOM_TILENAME_MAXNUM 500

std::vector<sprite_ids_t> spriteIDs;

static BOOL CheckForExistingID(WORD nID) {
	WORD nSkipHit;
	sprite_ids_t *pSprEnt;

	nSkipHit = 0;
	for (int i = 0; i < (int)spriteIDs.size(); i++) {
		pSprEnt = &spriteIDs[i];
		if (pSprEnt && pSprEnt->nID == nID) {
			if (sprite_debug & SPRITE_DEBUG_SPRITES)
				ConsoleLog(LOG_DEBUG, "CheckForExistingID(%u): (%u, %u, 0x%06X, %u) ID already exists.\n", nID, pSprEnt->nArcID, pSprEnt->nID, pSprEnt->dwOffset, pSprEnt->dwSize);
			pSprEnt->bMultiple = TRUE;
			nSkipHit = ++pSprEnt->nSkipHit;
		}
	}
	return (nSkipHit) ? TRUE : FALSE;
}

static void AllocateAndLoadSprites1996(CMFC3XFile *pFile, sprite_archive_t *lpBuf, WORD nSpriteSet) {
	WORD nPos, nID;
	sprite_ids_t *pSprEnt;
	BYTE *pSpriteData;

	GameMain_File_Seek(pFile, lpBuf->pData[0].sprHeader.dwAddress.sprLong, 0);
	for (nPos = 0; nPos < spriteIDs.size(); ++nPos) {
		pSprEnt = &spriteIDs[nPos];
		if (pSprEnt && pSprEnt->nArcID == nSpriteSet) {
			nID = pSprEnt->nID;
			if (pSprEnt->bMultiple)
				if (sprite_debug & SPRITE_DEBUG_SPRITES)
					ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): Multiple sprites with the same ID Detected (%u, 0x%06X, %u) (%u)\n", nSpriteSet, nID, pSprEnt->dwOffset, pSprEnt->dwSize, pSprEnt->nSkipHit);
			if (pSprEnt->dwSize > 0) {
				pSpriteData = (BYTE *)Game_AllocateDataEntry(pSprEnt->dwSize);
				if (pSpriteData) {
					if (GameMain_File_Read(pFile, pSpriteData, pSprEnt->dwSize) == pSprEnt->dwSize) {
						if (pSprEnt->bMultiple && pSprEnt->nSkipHit > 0) {
							if (sprite_debug & SPRITE_DEBUG_SPRITES)
								ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): discarding skipped sprite with ID (%u, 0x%06X, %u).\n", nSpriteSet, nID, pSprEnt->dwOffset, pSprEnt->dwSize);
							Game_FreeDataEntry(pSpriteData);
							continue;
						}
						if (pArrSpriteHeaders[nID].dwAddress.sprLong) {
							Game_FreeDataEntry(pArrSpriteHeaders[nID].dwAddress.sprPtr);
							pArrSpriteHeaders[nID].dwAddress.sprPtr = 0;
						}
						pArrSpriteHeaders[nID].dwAddress.sprPtr = pSpriteData;
						pArrSpriteHeaders[nID].wHeight = pSprEnt->wHeight;
						pArrSpriteHeaders[nID].wWidth = pSprEnt->wWidth;
					}
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_LoadSpriteDataArchive1996(WORD nSpriteSet) {
	CMFC3XFile datArchive;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nPos, nNextPos;
	__int16 nArcFileCnt;
	WORD nID, nNextID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;
	DWORD dwNextAddress;
	sprite_ids_t spriteEnt;
	int nSize;

	GameMain_File_Cons(&datArchive);
	GameMain_String_Cons(&retString);
	GameMain_String_Cons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> LoadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	Game_SimcityApp_GetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	GameMain_String_Format(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	if (GameMain_File_Open(&datArchive, retStrPath.m_pchData, 0, 0)) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		nFlen = GameMain_File_GetLength(&datArchive);
		GameMain_File_Read(&datArchive, &nArcFileCnt, 2);
		nArcFileCnt = Game_FlipShortBytes(nArcFileCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nArcFileCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nArcFileCnt;
		if (lpBuf) {
			if (GameMain_File_Read(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nArcFileCnt; ++nPos) {
					nID = Game_FlipShortBytes(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;

					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->dwAddress.sprLong = Game_FlipLongBytePortions(pSprtHead->dwAddress.sprLong);

					pArrSpriteHeaders[nID] = *pSprtHead;
					pArrSpriteHeaders[nID].dwAddress.sprPtr = 0;
					pArrSpriteHeaders[nID].wHeight = Game_FlipShortBytes(pArrSpriteHeaders[nID].wHeight);
					pArrSpriteHeaders[nID].wWidth = Game_FlipShortBytes(pArrSpriteHeaders[nID].wWidth);

					nNextPos = (nPos >= nArcFileCnt - 1) ? -1 : nPos + 1;
					nNextID = (nNextPos >=0) ? nID : -1;

					if (nNextPos >= 0) {
						// The next position hasn't yet been processed, do so here so
						// we can get the file size.
						dwNextAddress = Game_FlipLongBytePortions(lpBuf->pData[nNextPos].sprHeader.dwAddress.sprLong);
						nSize = (dwNextAddress - pSprtHead->dwAddress.sprLong);
					}
					else
						nSize = (nFlen - pSprtHead->dwAddress.sprLong);

					if (nSize > 0) {
						spriteEnt.nArcID = nSpriteSet;
						spriteEnt.nID = nID;
						spriteEnt.dwOffset = pSprtHead->dwAddress.sprLong;
						spriteEnt.dwSize = nSize;
						spriteEnt.wHeight = pArrSpriteHeaders[nID].wHeight;
						spriteEnt.wWidth = pArrSpriteHeaders[nID].wWidth;
						spriteEnt.nSkipHit = 0;
						spriteEnt.bMultiple = (CheckForExistingID(nID)) ? TRUE : FALSE;
						spriteIDs.push_back(spriteEnt);
					}
				}

				AllocateAndLoadSprites1996(&datArchive, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		GameMain_File_Close(&datArchive);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
	GameMain_File_Dest(&datArchive);
}

static void ResetCustomTileNames() {
	for (int i = 0; i < CUSTOM_TILENAME_MAXNUM; ++i) {
		if (pTileNames[i])
			Game_FreeDataEntry(pTileNames[i]);
		pTileNames[i] = 0;
	}
}

static void ReloadSpriteDataArchive1996(WORD nSpriteSet) {
	CMFC3XFile datArchive;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nArcFileCnt;
	WORD nPos;
	WORD nID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;

	GameMain_File_Cons(&datArchive);
	GameMain_String_Cons(&retString);
	GameMain_String_Cons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> ReloadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	Game_SimcityApp_GetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	GameMain_String_Format(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	if (GameMain_File_Open(&datArchive, retStrPath.m_pchData, 0, 0)) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		nFlen = GameMain_File_GetLength(&datArchive);
		GameMain_File_Read(&datArchive, &nArcFileCnt, 2);
		nArcFileCnt = Game_FlipShortBytes(nArcFileCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nArcFileCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nArcFileCnt;
		if (lpBuf) {
			if (GameMain_File_Read(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nArcFileCnt; ++nPos) {
					nID = Game_FlipShortBytes(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;
					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->dwAddress.sprLong = Game_FlipLongBytePortions(pSprtHead->dwAddress.sprLong);
					pSprtHead->wHeight = Game_FlipShortBytes(pSprtHead->wHeight);
					pSprtHead->wWidth = Game_FlipShortBytes(pSprtHead->wWidth);
				}

				AllocateAndLoadSprites1996(&datArchive, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		GameMain_File_Close(&datArchive);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
	GameMain_File_Dest(&datArchive);
}

void ReloadDefaultTileSet_SC2K1996() {
	CSimcityAppPrimary *pSCApp;

	pSCApp = &pCSimcityAppThis;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to reload the base game tile set?", gamePrimaryKey, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDYES)
		return;

	GameMain_CmdTarget_BeginWaitCursor(pSCApp);
	ResetCustomTileNames();
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	GameMain_CmdTarget_EndWaitCursor(pSCApp);
}

void InstallSpriteAndTileSetHooks_SC2K1996(void) {
	// Hook LoadSpriteDataArchive
	VirtualProtect((LPVOID)0x4029B4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4029B4, Hook_LoadSpriteDataArchive1996);
}
