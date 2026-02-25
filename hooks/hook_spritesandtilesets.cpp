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
				ConsoleLog(LOG_DEBUG, "CheckForExistingID(%u): (%u, %u, 0x%06X, %d) ID already exists.\n", nID, pSprEnt->nArcID, pSprEnt->nID, pSprEnt->sprOffset, pSprEnt->nSize);
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

	GameMain_File_Seek(pFile, lpBuf->pData[0].sprHeader.sprOffset.sprLong, 0);
	for (nPos = 0; nPos < spriteIDs.size(); ++nPos) {
		pSprEnt = &spriteIDs[nPos];
		if (pSprEnt && pSprEnt->nArcID == nSpriteSet) {
			nID = pSprEnt->nID;
			if (pSprEnt->bMultiple)
				if (sprite_debug & SPRITE_DEBUG_SPRITES)
					ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): Multiple sprites with the same ID Detected (%u, 0x%06X, %d) (%u)\n", nSpriteSet, nID, pSprEnt->sprOffset, pSprEnt->nSize, pSprEnt->nSkipHit);
			if (pSprEnt->nSize > 0) {
				pSpriteData = (BYTE *)Game_AllocateDataEntry(pSprEnt->nSize);
				if (pSpriteData) {
					if (GameMain_File_Read(pFile, pSpriteData, pSprEnt->nSize) == pSprEnt->nSize) {
						if (pSprEnt->bMultiple && pSprEnt->nSkipHit > 0) {
							if (sprite_debug & SPRITE_DEBUG_SPRITES)
								ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): discarding skipped sprite with ID (%u, 0x%06X, %d).\n", nSpriteSet, nID, pSprEnt->sprOffset, pSprEnt->nSize);
							Game_FreeDataEntry(pSpriteData);
							continue;
						}
						if (pArrSpriteHeaders[nID].sprOffset.sprPtr) {
							Game_FreeDataEntry(pArrSpriteHeaders[nID].sprOffset.sprPtr);
							pArrSpriteHeaders[nID].sprOffset.sprPtr = 0;
						}
						pArrSpriteHeaders[nID].sprOffset.sprPtr = pSpriteData;
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
	__int16 nSpriteCnt;
	WORD nID, nNextID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;
	int32_t sprNextOffset;
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
		GameMain_File_Read(&datArchive, &nSpriteCnt, 2);
		nSpriteCnt = Game_FlipShortBytes(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (GameMain_File_Read(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = Game_FlipShortBytes(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;

					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = Game_FlipLongBytePortions(pSprtHead->sprOffset.sprLong);

					pArrSpriteHeaders[nID] = *pSprtHead;
					pArrSpriteHeaders[nID].sprOffset.sprPtr = 0;
					pArrSpriteHeaders[nID].wHeight = Game_FlipShortBytes(pArrSpriteHeaders[nID].wHeight);
					pArrSpriteHeaders[nID].wWidth = Game_FlipShortBytes(pArrSpriteHeaders[nID].wWidth);

					nNextPos = (nPos >= nSpriteCnt - 1) ? -1 : nPos + 1;
					nNextID = (nNextPos >=0) ? nID : -1;

					if (nNextPos >= 0) {
						// The next position hasn't yet been processed, do so here so
						// we can get the file size.
						sprNextOffset = Game_FlipLongBytePortions(lpBuf->pData[nNextPos].sprHeader.sprOffset.sprLong);
						nSize = (sprNextOffset - pSprtHead->sprOffset.sprLong);
					}
					else
						nSize = (nFlen - pSprtHead->sprOffset.sprLong);

					if (nSize > 0) {
						spriteEnt.nArcID = nSpriteSet;
						spriteEnt.nID = nID;
						spriteEnt.sprOffset = pSprtHead->sprOffset.sprLong;
						spriteEnt.nSize = nSize;
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
	__int16 nSpriteCnt;
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
		GameMain_File_Read(&datArchive, &nSpriteCnt, 2);
		nSpriteCnt = Game_FlipShortBytes(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (GameMain_File_Read(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = Game_FlipShortBytes(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;
					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = Game_FlipLongBytePortions(pSprtHead->sprOffset.sprLong);
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

static void L_LoadFixedLargeSpritesRsrc_SC2K1996() {
	HRSRC hTileSetHandle;
	HGLOBAL hTileSetGlobal;
	DWORD dwTileDatSz;
	DWORD dwOffset;
	WORD nChunk;
	char *szHead[4];
	DWORD dwSize;
	__int16 nSpriteID;
	WORD nWidth, nHeight;
	DWORD dwSize_Shap;
	WORD nTileNameID, nNameLength;
	BOOL bGotShap, bGotName, bResize;
	char *pRsrcDat;
	char *pTileDat;
	char *pBuf;
	tilesetMainHeader_t *pTileHeader;
	tilesetHeadInfo_t *pTileInfo;
	tilesetHeadInfo_t *pTileTiles;
	tilesetMem_t *pTileMem;
	tileMem_t *pTileContents;
	tileShap_t *pTileShap;
	tileName_t *pTileName;

	dwOffset = 0;
	hTileSetHandle = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_TSET_FIXED), "TSET");
	if (hTileSetHandle) {
		hTileSetGlobal = LoadResource(hSC2KFixModule, hTileSetHandle);
		dwTileDatSz = SizeofResource(hSC2KFixModule, hTileSetHandle);
		pRsrcDat = (char *)LockResource(hTileSetGlobal);
		if (pRsrcDat) {
			pTileDat = (char *)malloc(dwTileDatSz);
			if (pTileDat) {
				memcpy(pTileDat, pRsrcDat, dwTileDatSz);
				pTileHeader = (tilesetMainHeader_t *)pTileDat;
				if (memcmp(pTileHeader->szTypeHead, "MIFF", 4) == 0 &&
					memcmp(pTileHeader->szSC2KHead, "SC2K", 4) == 0) {
					dwSize = L_FlipLongBytePortions(pTileHeader->dwSize);
					dwOffset += sizeof(tilesetMainHeader_t);
					pTileInfo = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
					if (pTileInfo && memcmp(pTileInfo->szHead, "INFO", 4) == 0) {
						dwSize = L_FlipLongBytePortions(pTileInfo->dwSize);
						dwOffset += sizeof(tilesetHeadInfo_t) + dwSize;
						pTileTiles = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
						if (pTileTiles && memcmp(pTileTiles->szHead, "TILE", 4) == 0) {
							dwSize = L_FlipLongBytePortions(pTileTiles->dwSize);
							dwOffset += sizeof(tilesetHeadInfo_t);
							pTileMem = (tilesetMem_t *)(pTileDat + dwOffset);
							if (pTileMem) {
								pTileMem->nMaxChunks = L_FlipShortBytes(pTileMem->nMaxChunks);
								pTileContents = &pTileMem->tileMem;
								if (pTileContents) {
									for (nChunk = 0; pTileMem->nMaxChunks > nChunk; ++nChunk) {
										memcpy(szHead, pTileContents->szHead, 4);
										dwSize = L_FlipLongBytePortions(pTileContents->dwSize);
										pBuf = &pTileContents->pBuf;

										bGotShap = bGotName = bResize = FALSE;
										if (memcmp(szHead, "SHAP", 4) == 0) {
											pTileShap = (tileShap_t *)pBuf;
											nSpriteID = L_FlipShortBytes(pTileShap->nSpriteID);
											nWidth = L_FlipShortBytes(pTileShap->nWidth);
											nHeight = L_FlipShortBytes(pTileShap->nHeight);
											dwSize_Shap = L_FlipLongBytePortions(pTileShap->dwSize);
											// In this case we ONLY want to load the large sprites.
											if (nSpriteID < SPRITE_LARGE_START)
												bGotShap = TRUE;
											else
												bGotShap = (nHeight > 1) ? Game_ChangeTileSpriteEntry(nSpriteID, nWidth, nHeight, dwSize_Shap, &pTileShap->pBuf) : TRUE;

											if (bGotShap && nHeight > 1 && nSpriteID >= SPRITE_LARGE_START)
												ConsoleLog(LOG_INFO, "Loaded replacement large sprite for: %s\n", szSpriteNames[nSpriteID - SPRITE_LARGE_START]);
										}
										else if (memcmp(szHead, "NAME", 4) == 0) {
											pTileName = (tileName_t *)pBuf;
											nTileNameID = L_FlipShortBytes(pTileName->nTileNameID);
											nNameLength = L_FlipShortBytes(pTileName->nNameLength);
											// Although we process the above we leave the
											// names alone here.
											bGotName = TRUE;
										}

										// Added. If this is set to true it stands to reason
										// you'd then want to break out of the loop.
										if (bTilesetLoadOutOfMemory)
											break;

										if (bGotShap || bGotName || bResize) {
											bResize = FALSE;
											pTileContents = (tileMem_t *)&pBuf[dwSize];
											continue;
										}

										bResize = TRUE;
										pTileContents = (tileMem_t *)Game_ReallocateDataEntry((char *)pTileMem, pBuf);
									}
								}
							}
						}
					}
				}
				free(pTileDat);
			}
		}
		FreeResource(hTileSetGlobal);
	}
	ConsoleLog(LOG_INFO, "Load Replacement Default Large Sprite Resources.\n");
}

void ReloadDefaultTileSet_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to reload the base game tile set?", gamePrimaryKey, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDYES)
		return;

	GameMain_CmdTarget_BeginWaitCursor(pSCApp);
	ResetCustomTileNames();
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GameMain_CmdTarget_EndWaitCursor(pSCApp);

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __declspec(naked) __stdcall Hook_LoadSpriteArchives1996() {
	Game_LoadDataArchive(TILEDAT_DEFS_SPECIAL);
	Game_LoadDataArchive(TILEDAT_DEFS_LARGE);
	Game_LoadDataArchive(TILEDAT_DEFS_SMALLMED);
	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GAMEJMP(0x42C332)
}

void InstallSpriteAndTileSetHooks_SC2K1996(void) {
	// Hook LoadSpriteDataArchive
	VirtualProtect((LPVOID)0x4029B4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4029B4, Hook_LoadSpriteDataArchive1996);

	// Hook into InitializeDataColorsFonts - move actual
	// sprite loading into external call.
	VirtualProtect((LPVOID)0x42C314, 30, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x42C314, 0x90, 30);
	NEWJMP((LPVOID)0x42C314, Hook_LoadSpriteArchives1996);
}
