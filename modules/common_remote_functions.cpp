// sc2kfix modules/common_remote_functions.cpp: wrapper remote functions
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE: The defined remote functions only account for certain cases.
// If you attempt to use any of these remote functions out-of-context
// (ie, they don't have a targeted remote call) then they will fail.
//
// Be careful.

// Keys for remote framework cases:
// BOR - Borland wrapper functions.
// MFC - MFC wrapper/replacement functions.
// SCURK - SCURK-only.
// SC2K - SC2K-only.

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>
#include "../resource.h"

__int16 __cdecl L_FlipShortBytes(__int16 nVal) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SC2K) {
		if (dwDetectedVersion == VERSION_SC2K_1996)
			return Game_FlipShortBytes(nVal);
	}
	else if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_FlipShortBytes_SCURKPrimary(nVal);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_FlipShortBytes_SCURK1996(nVal);
	}
	return -1;
}

int __cdecl L_FlipLongBytePortions(int nVal) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SC2K) {
		if (dwDetectedVersion == VERSION_SC2K_1996)
			return Game_FlipLongBytePortions(nVal);
	}
	else if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_FlipLongBytePortions_SCURKPrimary(nVal);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_FlipLongBytePortions_SCURK1996(nVal);
	}
	return -1;
}

void *__cdecl L_BOR_gAllocBlock(size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gAllocBlock_SCURKPrimary(nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gAllocBlock_SCURK1996(nSz);
	}
	return NULL;
}

void __cdecl L_BOR_gFreeBlock(void *pBlock) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_gFreeBlock_SCURKPrimary(pBlock);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_gFreeBlock_SCURK1996(pBlock);
	}
}

int __stdcall L_BOR_gUpdateWaitWindow() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gUpdateWaitWindow_SCURKPrimary();
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gUpdateWaitWindow_SCURK1996();
	}
	return 0;
}

static void L_SCURK_LoadFixedLargeSpritesRsrc(cEditableTileSet *pThis) {
	HRSRC hTileSetHandle;
	HGLOBAL hTileSetGlobal;
	DWORD dwTileDatSz;
	DWORD dwOffset;
	WORD nChunk;
	char *szHead[4];
	DWORD dwSize;
	__int16 nSpriteID;
	__int16 nDBID;
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
											nDBID = pThis->mDBIndexFromShapeNum[nSpriteID];

											// Only replace sprites with a height above 1 (similar to the main game
											// under these circumstances).
											if (nHeight > 1) {
												if (pThis->mTiles[nDBID])
													L_BOR_gFreeBlock(pThis->mTiles[nDBID]);
												pThis->mTiles[nDBID] = (uint8_t *)L_BOR_gAllocBlock(dwSize_Shap);
												if (pThis->mTiles[nDBID]) {
													memcpy(pThis->mTiles[nDBID], &pTileShap->pBuf, dwSize_Shap);
													pThis->mTileSet->pData[nDBID].sprHeader.wWidth = nWidth;
													pThis->mTileSet->pData[nDBID].sprHeader.wHeight = nHeight;
													pThis->mTileSizeTable[nDBID] = dwSize_Shap;
												}
												bGotShap = (pThis->mTiles[nDBID]) ? TRUE : FALSE;
											}
											else
												bGotShap = TRUE;

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

										if (bGotShap || bGotName || bResize) {
											bResize = FALSE;
											pTileContents = (tileMem_t *)&pBuf[dwSize];
											continue;
										}

										bResize = TRUE;
										pTileContents = (tileMem_t *)L_ReallocateDataEntry((char *)pTileMem, pBuf);
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
	ConsoleLog(LOG_INFO, "Load Replacement Default Sprite Resources.\n");
}

LONG __cdecl L_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName) {
	FILE *f;
	int nRes;
	int nIdx;

	f = old_fopen(lpPathName, "rb");
	if (!f)
		return 0;
	fseek(f, 0, SEEK_END);
	nRes = ftell(f);
	fseek(f, 0, SEEK_SET);
	fread(&pThis->mNumTiles, 2, 1, f);
	if (!pThis->mTileSet)
		pThis->mTileSet = (sprite_archive_t *)L_BOR_gAllocBlock(10 * pThis->mNumTiles + 2);
	if (!pThis->mTileSet) {
		fclose(f);
		return 0;
	}
	pThis->mTileSet->nSprites = pThis->mNumTiles;
	L_BOR_gUpdateWaitWindow();
	fread(pThis->mTileSet->pData, 1, 10 * pThis->mNumTiles, f);
	fread(pThis->mTileSizeTable, 1, 4 * pThis->mNumTiles, f);
	L_BOR_gUpdateWaitWindow();
	pThis->mStartPos = 0;
	nIdx = SPRITE_SMALL_START;
	do {
		if (pThis->mTiles[nIdx])
			L_BOR_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)L_BOR_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		if ((nIdx % 100) == 0)
			L_BOR_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < SPRITE_LARGE_START);
	L_BOR_gUpdateWaitWindow();
	nIdx = SPRITE_LARGE_START;
	do {
		if (pThis->mTiles[nIdx])
			L_BOR_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)L_BOR_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		pThis->mDBIndexFromShapeNum[pThis->mTileSet->pData[nIdx].nSprNum % SPRITE_COUNT] = nIdx;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_MEDIUM_START) % SPRITE_COUNT] = nIdx - SPRITE_MEDIUM_START;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_LARGE_START) % SPRITE_COUNT] = nIdx - SPRITE_LARGE_START;
		if ((nIdx % 100) == 0)
			L_BOR_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < pThis->mTileSet->nSprites);
	L_BOR_gUpdateWaitWindow();
	pThis->mTileSet[514].pData[0].sprHeader.wWidth += 4;
	pThis->mTileSet[98].pData[0].nSprNum += 2;
	pThis->mTileSet[543].pData[0].nSprNum += 4;
	pThis->mTileSet[126].pData[0].sprHeader.sprOffset.sprLong += 2;
	fclose(f);
	if (!bDisableFixedTiles) {
		if (nRes) {
			L_SCURK_LoadFixedLargeSpritesRsrc(pThis);
			L_BOR_gUpdateWaitWindow();
		}
	}
	return nRes;
}
