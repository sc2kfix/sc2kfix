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

static int nRevType = REV_WIN;

std::vector<sprite_ids_t> spriteIDs;

static void L_ConvertDOSSprite(tileConv_t *pObjSet, WORD nSpriteID, BYTE *pDOSTileBuf, int nConvRepl) {
	BYTE *pDOSTileBits, *pTileBitsBuf, *pTileBits;
	int nTileSize;
	BOOL bDone;
	BYTE pDOSTileChunkMode, pDOSTileBitCount;
	WORD pDOSTileRemainingBitCount;

	if (pObjSet->pObjects[nSpriteID])
		free(pObjSet->pObjects[nSpriteID]);
	pObjSet->pObjects[nSpriteID] = (BYTE *)malloc(0xFFFF);
	memset(pObjSet->pObjects[nSpriteID], 0, 0xFFFF);
	pDOSTileBits = pDOSTileBuf;
	pTileBitsBuf = pObjSet->pObjects[nSpriteID];
	nTileSize = 0;
	pTileBits = 0;
	if (SPRITEDOSDATA(pDOSTileBuf)->nChunkMode != TIL_CM_NEWROWSTART) {
		SPRITEDATA(pTileBitsBuf)->nCount = 0;
		SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_NEWROWSTART;
		pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
		pTileBits = pTileBitsBuf;
		nTileSize = 2;
		++pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wHeight;
	}
	bDone = 0;
	while (!bDone) {
		pDOSTileChunkMode = SPRITEDOSDATA(pDOSTileBits)->nChunkMode;
		pDOSTileBitCount = SPRITEDOSDATA(pDOSTileBits)->nCount;
		pDOSTileBits = (BYTE *)&SPRITEDOSDATA(pDOSTileBits)->pBuf;
		switch (pDOSTileChunkMode) {
		case TIL_CM_SKIPPIXELS:
			SPRITEDATA(pTileBitsBuf)->nCount = pDOSTileBitCount;
			SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_SKIPPIXELS;
			pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
			nTileSize += 2;
			break;
		case TIL_CM_PROCPIXELS:
			SPRITEDATA(pTileBitsBuf)->nCount = pDOSTileBitCount;
			SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_PROCPIXELS;
			pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
			pDOSTileRemainingBitCount = pDOSTileBitCount;
			for (nTileSize += 2; pDOSTileRemainingBitCount--; ++nTileSize)
				*pTileBitsBuf++ = L_GetTranslatedDOSMacPaletteIdx(*pDOSTileBits++, nConvRepl);
			if (!IsEvenUnsigned(pDOSTileBitCount) && pTileBits) {
				++*pTileBits;
				++pTileBitsBuf;
				++nTileSize;
			}
			break;
		case TIL_CM_NEWROWSTART:
			pTileBits = pTileBitsBuf;
			SPRITEDATA(pTileBitsBuf)->nCount = pDOSTileBitCount - 1;
			SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_NEWROWSTART;
			pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
			nTileSize += 2;
			break;
		default:
			SPRITEDATA(pTileBitsBuf)->nCount = pDOSTileBitCount;
			SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_ENDOFSPRITE;
			pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
			nTileSize += 2;
			bDone = 1;
			break;
		}
	}
	pObjSet->pObjectSetSize[nSpriteID] = nTileSize;
	pObjSet->pObjects[nSpriteID] = (BYTE *)realloc(pObjSet->pObjects[nSpriteID], nTileSize);
}

static void L_ConvertSprite(WORD nWidth, WORD nHeight, BYTE *pBits, int nConvType, int nConvReplPal) {
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, pBit;
	WORD nShapeWidth, nShapeHeight, nCurrWidth;
	BOOL bDone;
	WORD pTileRemainingBitCount;

	nCurrWidth = 0;
	pTileBits = pBits;
	if (pTileBits) {
		nShapeHeight = nHeight;
		nShapeWidth = nWidth;

		bDone = 0;
		while (!bDone) {
			pTileBitCount = SPRITEDATA(pTileBits)->nCount;
			pTileChunkMode = SPRITEDATA(pTileBits)->nChunkMode;
			pTileBits = (BYTE *)&SPRITEDATA(pTileBits)->pBuf;
			switch (pTileChunkMode) {
			case MIF_CM_EMPTY:
				continue;
			case MIF_CM_NEWROWSTART:
				nCurrWidth = 0;
				bDone = nShapeHeight == 0;
				--nShapeHeight;
				break;
			case MIF_CM_ENDOFSPRITE:
				bDone = 1;
				break;
			case MIF_CM_SKIPPIXELS:
				nCurrWidth += pTileBitCount;
				break;
			case MIF_CM_PROCPIXELS:
				for (pTileRemainingBitCount = pTileBitCount; pTileRemainingBitCount; ++nCurrWidth) {
					--pTileRemainingBitCount;
					// *pTileBits here in this case is nPixelIndex (colour lookup palette index)
					//
					// 'if (nCurrWidth < nShapeWidth)' removed
					// to avoid certain columns of pixels being
					// missed during palette processing.
					{
						if (*pTileBits == 0xFC)
							pBit = 0x61;
						else {
							// Exception needed in this case.
							// Under DOS you want the bit to be 0xFF/White
							// while under Mac you want it to be 0x00/Black.
							if (nConvType == REV_DOSMAC)
								pBit = (*pTileBits == 0xFF) ? 0x00 : L_GetTranslatedDOSMacPaletteIdx(*pTileBits, nConvReplPal);
							else if (nConvType == REV_WIN)
								pBit = L_GetAdjustedPaletteIdx(*pTileBits, nConvReplPal);
							else
								pBit = *pTileBits;
						}
						*pTileBits = pBit;
					}
					++pTileBits;
				}
				if (!IsEvenUnsigned(pTileBitCount)) {
					++pTileBits;
					++nCurrWidth;
				}
				break;
			default:
				bDone = 1;
				break;
			}
		}
	}
}

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

static void AllocateAndLoadSprites1996(FILE *pFile, sprite_archive_t *lpBuf, WORD nSpriteSet) {
	WORD nPos, nID;
	sprite_ids_t *pSprEnt;
	BYTE *pSpriteData;

	fseek(pFile, lpBuf->pData[0].sprHeader.sprOffset.sprLong, SEEK_SET);
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
					if (fread(pSpriteData, 1, pSprEnt->nSize, pFile) == pSprEnt->nSize) {
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
						L_ConvertSprite(pSprEnt->wWidth, pSprEnt->wHeight, pSpriteData, REV_WIN, 3);
						pArrSpriteHeaders[nID].sprOffset.sprPtr = pSpriteData;
						pArrSpriteHeaders[nID].wHeight = pSprEnt->wHeight;
						pArrSpriteHeaders[nID].wWidth = pSprEnt->wWidth;

						Cache_Sprite(nID, pSpriteData, pSprEnt->nSize, pSprEnt->wHeight, pSprEnt->wWidth);
					}
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_LoadSpriteDataArchive1996(WORD nSpriteSet) {
	FILE *f;
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

	f = old_fopen(retStrPath.m_pchData, "rb");
	if (f) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		fseek(f, 0, SEEK_END);
		nFlen = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(&nSpriteCnt, 2, 1, f);
		nSpriteCnt = _byteswap_ushort(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (fread(&lpBuf->pData, 1, nBufSize, f) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = _byteswap_ushort(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;

					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = _byteswap_ulong(pSprtHead->sprOffset.sprLong);

					pArrSpriteHeaders[nID] = *pSprtHead;
					pArrSpriteHeaders[nID].sprOffset.sprPtr = 0;
					pArrSpriteHeaders[nID].wHeight = _byteswap_ushort(pArrSpriteHeaders[nID].wHeight);
					pArrSpriteHeaders[nID].wWidth = _byteswap_ushort(pArrSpriteHeaders[nID].wWidth);

					nNextPos = (nPos >= nSpriteCnt - 1) ? -1 : nPos + 1;
					nNextID = (nNextPos >=0) ? nID : -1;

					if (nNextPos >= 0) {
						// The next position hasn't yet been processed, do so here so
						// we can get the file size.
						sprNextOffset = _byteswap_ulong(lpBuf->pData[nNextPos].sprHeader.sprOffset.sprLong);
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

				AllocateAndLoadSprites1996(f, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		fclose(f);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
}

static void ResetCustomTileNames() {
	for (int i = 0; i < SPRITE_MEDIUM_START; ++i) {
		if (pTileNames[i])
			Game_FreeDataEntry(pTileNames[i]);
		pTileNames[i] = 0;
	}
}

static void ReloadSpriteDataArchive1996(WORD nSpriteSet) {
	FILE *f;
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

	f = old_fopen(retStrPath.m_pchData, "rb");
	if (f) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		fseek(f, 0, SEEK_END);
		nFlen = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(&nSpriteCnt, 2, 1, f);
		nSpriteCnt = _byteswap_ushort(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (fread(&lpBuf->pData, 1, nBufSize, f) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = _byteswap_ushort(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;
					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = _byteswap_ulong(pSprtHead->sprOffset.sprLong);
					pSprtHead->wHeight = _byteswap_ushort(pSprtHead->wHeight);
					pSprtHead->wWidth = _byteswap_ushort(pSprtHead->wWidth);
				}

				AllocateAndLoadSprites1996(f, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		fclose(f);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
}

static void L_LoadFixedLargeSpritesRsrc_SC2K1996(int nTileSet) {
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
	char *pTileDat, *pTileType;
	char *pBuf;
	tilesetMainHeader_t *pTileHeader;
	tilesetHeadInfo_t *pTileInfo;
	tilesetHeadInfo_t *pTileTiles;
	tilesetMem_t *pTileMem;
	tileMem_t *pTileContents;
	tileShap_t *pTileShap;
	tileName_t *pTileName;
	int iReplacementsLoaded = 0;

	dwOffset = 0;
	hTileSetHandle = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(nTileSet), "TSET");
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
					dwSize = _byteswap_ulong(pTileHeader->dwSize);
					dwOffset += sizeof(tilesetMainHeader_t);
					pTileInfo = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
					if (pTileInfo && memcmp(pTileInfo->szHead, "INFO", 4) == 0) {
						dwSize = _byteswap_ulong(pTileInfo->dwSize);
						pTileType = (char *)(pTileDat + dwOffset + sizeof(tilesetHeadInfo_t));
						if (pTileType) {
							nRevType = REV_WIN;
							if (memcmp(pTileType, "_MAC", 4) == 0)
								goto DONOTPROCEED;
							else if (memcmp(pTileType, "00W_", 4) == 0)
								nRevType = REV_W00;
						}
						dwOffset += sizeof(tilesetHeadInfo_t) + dwSize;
						pTileTiles = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
						if (pTileTiles && memcmp(pTileTiles->szHead, "TILE", 4) == 0) {
							dwSize = _byteswap_ulong(pTileTiles->dwSize);
							dwOffset += sizeof(tilesetHeadInfo_t);
							pTileMem = (tilesetMem_t *)(pTileDat + dwOffset);
							if (pTileMem) {
								pTileMem->nMaxChunks = _byteswap_ushort(pTileMem->nMaxChunks);
								pTileContents = &pTileMem->tileMem;
								if (pTileContents) {
									for (nChunk = 0; pTileMem->nMaxChunks > nChunk; ++nChunk) {
										memcpy(szHead, pTileContents->szHead, 4);
										dwSize = _byteswap_ulong(pTileContents->dwSize);
										pBuf = &pTileContents->pBuf;

										bGotShap = bGotName = bResize = FALSE;
										if (memcmp(szHead, "SHAP", 4) == 0) {
											pTileShap = (tileShap_t *)pBuf;
											nSpriteID = _byteswap_ushort(pTileShap->nSpriteID);
											nWidth = _byteswap_ushort(pTileShap->nWidth);
											nHeight = _byteswap_ushort(pTileShap->nHeight);
											dwSize_Shap = _byteswap_ulong(pTileShap->dwSize);
											// In this case we ONLY want to load the large sprites.
											if (nSpriteID < SPRITE_LARGE_START)
												bGotShap = TRUE;
											else
												bGotShap = (nHeight > 1) ? Game_ChangeTileSpriteEntry(nSpriteID, nWidth, nHeight, dwSize_Shap, &pTileShap->pBuf) : TRUE;

											if (bGotShap && nHeight > 1 && nSpriteID >= SPRITE_LARGE_START) {
												iReplacementsLoaded++;
												if (sprite_debug & SPRITE_DEBUG_TILESETS)
													ConsoleLog(LOG_DEBUG, "TILE: Loaded (%d) replacement large sprite for: %s\n", nTileSet, szSpriteNames[nSpriteID - SPRITE_LARGE_START]);
											}
										}
										else if (memcmp(szHead, "NAME", 4) == 0) {
											pTileName = (tileName_t *)pBuf;
											nTileNameID = _byteswap_ushort(pTileName->nTileNameID);
											nNameLength = _byteswap_ushort(pTileName->nNameLength);
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
				DONOTPROCEED:
				free(pTileDat);
			}
		}
		FreeResource(hTileSetGlobal);
	}

	if (iReplacementsLoaded && sprite_debug & SPRITE_DEBUG_TILESETS)
		ConsoleLog(LOG_DEBUG, "TILE: Loaded %i replacement default large sprite resources.\n", iReplacementsLoaded);
}

void ReloadDefaultTileSet_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to reload the base game tile set?", gamePrimaryKey, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDYES)
		return;

	GameMain_CmdTarget_BeginWaitCursor(pSCApp);
	Init_SpriteCache(true);

	nRevType = REV_WIN;

	ResetCustomTileNames();
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	if (!bDisableFixedTiles) {
		if (dwFixedTileMask & FIXTIL_MASK_HORZOFF)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HORZOFF);
		if (dwFixedTileMask & FIXTIL_MASK_VERTOFF)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_VERTOFF);
		if (dwFixedTileMask & FIXTIL_MASK_BADPALIDX)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_BADPALIDX);
		if (dwFixedTileMask & FIXTIL_MASK_MISSPIXELS)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_MISSPIXELS);
		if (dwFixedTileMask & FIXTIL_MASK_OOBPALIDX)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_OOBPALIDX);

		if (dwFixedTileMask & FIXTIL_MASK_HANGARANIM)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGARANIM);
		else if (dwFixedTileMask & FIXTIL_MASK_HANGARSHUT)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGARSHUT);
		else if (dwFixedTileMask & FIXTIL_MASK_HANGAROPEN)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGAROPEN);
	}
	GameMain_CmdTarget_EndWaitCursor(pSCApp);

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __declspec(naked) __stdcall Hook_LoadSpriteArchives1996() {
	Init_SpriteCache(false);

	nRevType = REV_WIN;

	Game_LoadDataArchive(TILEDAT_DEFS_SPECIAL);
	Game_LoadDataArchive(TILEDAT_DEFS_LARGE);
	Game_LoadDataArchive(TILEDAT_DEFS_SMALLMED);

	if (!bDisableFixedTiles) {
		if (dwFixedTileMask & FIXTIL_MASK_HORZOFF)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HORZOFF);
		if (dwFixedTileMask & FIXTIL_MASK_VERTOFF)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_VERTOFF);
		if (dwFixedTileMask & FIXTIL_MASK_BADPALIDX)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_BADPALIDX);
		if (dwFixedTileMask & FIXTIL_MASK_MISSPIXELS)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_MISSPIXELS);
		if (dwFixedTileMask & FIXTIL_MASK_OOBPALIDX)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_OOBPALIDX);

		if (dwFixedTileMask & FIXTIL_MASK_HANGARANIM)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGARANIM);
		else if (dwFixedTileMask & FIXTIL_MASK_HANGARSHUT)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGARSHUT);
		else if (dwFixedTileMask & FIXTIL_MASK_HANGAROPEN)
			L_LoadFixedLargeSpritesRsrc_SC2K1996(IDR_TSET_FIXTIL_HANGAROPEN);
	}
	GAMEJMP(0x42C332)
}

static int L_GetTilesetFileType(const char *pFilePath) {
	int nRet = 0;
	char szMIFHeader[4], szLargeData[9];
	FILE *f;

	f = old_fopen(pFilePath, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		int nFlen = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (nFlen >= 16) {
			fread(szMIFHeader, 1, 4, f);
			fseek(f, 0, SEEK_SET);
			fread(szLargeData, 1, 9, f);
			fseek(f, 0, SEEK_SET);
			if (memcmp(szMIFHeader, "MIFF", 4) == 0)
				nRet = 1;
			else if (memcmp(szLargeData, "LARGE.DAT", 9) == 0)
				nRet = 2;
		}
		fclose(f);
	}
	return nRet;
}

static void L_ChangeDOSTileSpriteEntry(tileConv_t *pObjSet, WORD nSpriteID, BYTE *pBuf, bool bReadOnly) {
	if (pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wHeight <= 1)
		return;
	int nConvRepl = 0;
	if (bReadOnly) {
		if (GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_MILITARY_HANGAR1)) {
			if (nHangar1Mode == HANGAR1_ANIM)
				nConvRepl = 1;
			else if (nHangar1Mode == HANGAR1_OPEN)
				nConvRepl = 2;
		}
	}
	L_ConvertDOSSprite(pObjSet, nSpriteID, pBuf, nConvRepl);
	BYTE *pDst = (BYTE *)Game_AllocateDataEntry(pObjSet->pObjectSetSize[nSpriteID]);
	if (pDst) {
		memset(pDst, 0, pObjSet->pObjectSetSize[nSpriteID]);
		if (pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr) {
			Game_FreeDataEntry(pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr);
			pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = 0;
		}
		memcpy(pDst, pObjSet->pObjects[nSpriteID], pObjSet->pObjectSetSize[nSpriteID]);
		pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = pDst;
		pArrSpriteHeaders[nSpriteID].wWidth = pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wWidth;
		pArrSpriteHeaders[nSpriteID].wHeight = pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wHeight;
		Cache_Sprite(nSpriteID, pDst, pObjSet->pObjectSetSize[nSpriteID], pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wHeight, pObjSet->pObjectSet->pData[nSpriteID].sprHeader.wWidth);
	}
}

static void L_ReadDOSTilesetFile(const char *pFilePath) {
	FILE *f;
	tileConv_t convObjectSet;
	BYTE *lpBuffer;
	tilMainStruct_t Buffer;
	bool bReadOnly;
	tilHeader_t *lpLargeShapeBuf, *lpSmallShapeBuf, *lpOtherShapeBuf;
	DWORD dwLargeSize, dwLargeOffset, dwSmallSize, dwSmallOffset, dwOtherSize, dwOtherOffset;
	WORD nSpriteID;
	BOOL bValid;
	tilesetShapVerify_t validTiles[SPRITE_COUNT];

	f = old_fopen(pFilePath, "rb");
	if (f) {
		memset(&convObjectSet, 0, sizeof(convObjectSet));

		convObjectSet.nObjectNum = SPRITE_COUNT;
		int nSize = (SPRITE_COUNT + 10) * 4;
		convObjectSet.pObjectSetSize = (int *)malloc(nSize);
		memset(convObjectSet.pObjectSetSize, 0, nSize);
		nSize = sizeof(sprite_file_header_t) * convObjectSet.nObjectNum + sizeof(__int16);
		convObjectSet.pObjectSet = (sprite_archive_t *)malloc(nSize);
		memset(convObjectSet.pObjectSet, 0, nSize);
		convObjectSet.pObjectSet->nSprites = convObjectSet.nObjectNum;

		lpBuffer = (BYTE *)malloc(0xFFFF);
		fread(&Buffer, 1, 0x80, f);
		bReadOnly = (memcmp(Buffer.readOnlyFile, "READONLY.XXX", 12) == 0) ? true : false;
		lpLargeShapeBuf = (tilHeader_t *)malloc(0x2EE0);
		fseek(f, Buffer.dwLargeOffset, SEEK_SET);
		fread(lpLargeShapeBuf, 1, 0x2EE0, f);
		dwLargeSize = Buffer.dwLargeSize;
		for (nSpriteID = SPRITE_LARGE_START; nSpriteID < SPRITE_COUNT; ++nSpriteID) {
			dwLargeOffset = lpLargeShapeBuf[nSpriteID].dwOffset;
			validTiles[nSpriteID].nSpriteID = nSpriteID;
			bValid = TRUE;
			if (lpLargeShapeBuf[nSpriteID].height < 2)
				bValid = FALSE;
			if (dwLargeOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nSpriteID].nHeight = lpLargeShapeBuf[nSpriteID].height;
					validTiles[nSpriteID].nWidth = lpLargeShapeBuf[nSpriteID].width;
				}
			}
			validTiles[nSpriteID].nValidated = (dwLargeOffset != 0xFFFFFFFF && bValid) ? 1 : 0;
			if (validTiles[nSpriteID].nValidated != 2) {
				if (validTiles[nSpriteID].nValidated == 1) {
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wHeight = validTiles[nSpriteID].nHeight;
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wWidth = validTiles[nSpriteID].nWidth;
				}
			}
			fseek(f, dwLargeSize + dwLargeOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			if (validTiles[nSpriteID].nValidated == 1)
				L_ChangeDOSTileSpriteEntry(&convObjectSet, nSpriteID, lpBuffer, bReadOnly);
		}

		lpOtherShapeBuf = (tilHeader_t *)malloc(0x2EE0);
		fseek(f, Buffer.dwOtherOffset, SEEK_SET);
		fread(lpOtherShapeBuf, 1, 0x2EE0, f);
		dwOtherSize = Buffer.dwOtherSize;
		for (nSpriteID = SPRITE_MEDIUM_START; nSpriteID < SPRITE_LARGE_START; ++nSpriteID) {
			dwOtherOffset = lpOtherShapeBuf[nSpriteID].dwOffset;
			validTiles[nSpriteID].nSpriteID = nSpriteID;
			bValid = TRUE;
			if (lpOtherShapeBuf[nSpriteID].height < 2)
				bValid = FALSE;
			if (dwOtherOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nSpriteID].nHeight = lpOtherShapeBuf[nSpriteID].height;
					validTiles[nSpriteID].nWidth = lpOtherShapeBuf[nSpriteID].width;
				}
			}
			validTiles[nSpriteID].nValidated = (dwOtherOffset != 0xFFFFFFFF && bValid) ? 1 : 0;
			if (validTiles[nSpriteID].nValidated != 2) {
				if (validTiles[nSpriteID].nValidated == 1) {
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wHeight = validTiles[nSpriteID].nHeight;
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wWidth = validTiles[nSpriteID].nWidth;
				}
			}
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			if (validTiles[nSpriteID].nValidated == 1)
				L_ChangeDOSTileSpriteEntry(&convObjectSet, nSpriteID, lpBuffer, bReadOnly);
		}

		lpSmallShapeBuf = (tilHeader_t *)malloc(0x2EE0);
		fseek(f, Buffer.dwSmallOffset, SEEK_SET);
		fread(lpSmallShapeBuf, 1, 0x2EE0, f);
		dwSmallSize = Buffer.dwSmallSize;
		for (nSpriteID = SPRITE_SMALL_START; nSpriteID < SPRITE_LARGE_START; ++nSpriteID) {
			dwSmallOffset = lpSmallShapeBuf[nSpriteID].dwOffset;
			validTiles[nSpriteID].nSpriteID = nSpriteID;
			bValid = TRUE;
			if (lpSmallShapeBuf[nSpriteID].height < 2)
				bValid = FALSE;
			if (dwSmallOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nSpriteID].nHeight = lpSmallShapeBuf[nSpriteID].height;
					validTiles[nSpriteID].nWidth = lpSmallShapeBuf[nSpriteID].width;
					validTiles[nSpriteID].nValidated = 1;
				}
				else
					validTiles[nSpriteID].nValidated = 2;
			}
			else if (validTiles[nSpriteID].nValidated == 1)
				validTiles[nSpriteID].nValidated = 2;
			if (validTiles[nSpriteID].nValidated != 2) {
				if (validTiles[nSpriteID].nValidated == 1) {
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wHeight = validTiles[nSpriteID].nHeight;
					convObjectSet.pObjectSet->pData[nSpriteID].sprHeader.wWidth = validTiles[nSpriteID].nWidth;
				}
			}
			fseek(f, dwSmallSize + dwSmallOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			// Only free/(replace) if:
			// a) the tile has been successfully validated once here and now.
			// b) the tile isn't valid (and wasn't previously valid - ie from a prior archive - it's a skip case)
			if (validTiles[nSpriteID].nValidated != 2)
				L_ChangeDOSTileSpriteEntry(&convObjectSet, nSpriteID, lpBuffer, bReadOnly);
		}

		free(lpSmallShapeBuf);
		free(lpOtherShapeBuf);
		free(lpLargeShapeBuf);

		free(lpBuffer);

		if (convObjectSet.pObjectSet) {
			free(convObjectSet.pObjectSet);
			convObjectSet.pObjectSet = 0;
		}
		if (convObjectSet.pObjectSetSize) {
			free(convObjectSet.pObjectSetSize);
			convObjectSet.pObjectSetSize = 0;
		}
		fclose(f);
	}
}

extern "C" void __stdcall Hook_SimcityApp_LoadTileset1996() {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	char szFileTypes[255 + 1], szCaption[255 + 1];
	CMFC3XString strFilePath;
	int nPathLen, nFileLen, nNewLen;
	char szFilePath[MAX_PATH + 1], szPath[MAX_PATH + 1];
	OPENFILENAMEA m_ofn;

	memset(szPath, 0, sizeof(szPath));

	L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 4004, szFileTypes, sizeof(szFileTypes) - 1);
	L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 4005, szCaption, sizeof(szCaption) - 1);

	Game_SimcityApp_GetValueStringA(pThis, &strFilePath, aPaths, aTilesets);

	strcpy_s(szFilePath, sizeof(szFilePath) - 1, strFilePath.m_pchData);

	memset(&m_ofn, 0, sizeof(OPENFILENAMEA));
	m_ofn.lStructSize = sizeof(OPENFILENAMEA);
	m_ofn.hwndOwner = pThis->m_pMainWnd->m_hWnd;
	m_ofn.hInstance = pThis->m_hInstance;
	m_ofn.lpstrFilter = ConvertFileTypeFilterString(szFileTypes);
	m_ofn.lpstrInitialDir = szFilePath;
	m_ofn.lpstrTitle = szCaption;
	m_ofn.nMaxFile = _countof(szPath);
	m_ofn.nMaxFileTitle = _countof(szPath);
	m_ofn.nFilterIndex = 1;
	m_ofn.lpstrDefExt = "mif";
	m_ofn.lpstrFile = szPath;
	m_ofn.lpstrFileTitle = szFilePath;
	m_ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
	ToggleFloatingStatusDialog(FALSE);
	if (GetOpenFileNameA(&m_ofn)) {
		GameMain_CmdTarget_BeginWaitCursor(pThis);
		int nRet = L_GetTilesetFileType(m_ofn.lpstrFile);
		if (nRet > 0) {
			if (nRet == 1)
				Game_ReadTilesetFile(m_ofn.lpstrFile);
			else
				L_ReadDOSTilesetFile(m_ofn.lpstrFile);
			nNewLen = 0;
			nPathLen = strlen(m_ofn.lpstrFile);
			nFileLen = strlen(m_ofn.lpstrFileTitle);
			if (nPathLen > 0 && nFileLen > 0) {
				nNewLen = nPathLen - nFileLen;
				if (nNewLen > 0) {
					strncpy_s(szPath, sizeof(szPath) - 1, m_ofn.lpstrFile, nNewLen);
					if (L_IsPathValid(szPath))
						jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS] = szPath;
				}
			}
		}
		else {
			char szError[512 + 1];
			sprintf_s(szError, "Invalid tileset file '%s'", m_ofn.lpstrFileTitle);
			MessageBoxA(pThis->m_pMainWnd->m_hWnd, szError, gamePrimaryKey, MB_ICONERROR);
		}
		GameMain_CmdTarget_EndWaitCursor(pThis);
	}
	ToggleFloatingStatusDialog(TRUE);
	GameMain_String_Dest(&strFilePath);
}

extern "C" void __cdecl Hook_ReadTilesetFile1996(char *pFilePath) {
	CSimcityAppPrimary *pSCApp;
	FILE *f;
	DWORD nLen;
	char currChar;
	const char *pFileName;
	char *pBuf;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	bTilesetLoadOutOfMemory = FALSE;
	f = old_fopen(pFilePath, "rb");
	if (f) {
		if (Game_CheckTilesetFileHeader(f)) {
			// There was a prior null function prior to
			// actual main tileset loading; it most likely
			// could have been to debug the main body of
			// the file prior to actual loading.
			Game_VerifyAndLoadNewTiles(f);
			if (bTilesetLoadOutOfMemory) {
				GameMain_String_LoadStringA(&reqCaption, 4007);
				GameMain_String_LoadStringA(&reqText, 4008);
				nLen = strlen(pFilePath);
				do
					currChar = pFilePath[--nLen];
				while (currChar != '\\' && nLen > 0);
				pFileName = &pFilePath[nLen + 1];
				pBuf = (char *)Game_AllocateDataEntry(reqText.m_nDataLength + 2 * (strlen(pFileName) + 1) - 2);
				if (pBuf) {
					wsprintfA(pBuf, reqText.m_pchData, pFileName, pFileName);
					GameMain_CmdTarget_EndWaitCursor(pSCApp);
					L_MessageBoxA(0, pBuf, reqCaption.m_pchData, MB_OK);
					pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
					if (pSCView)
						UpdateWindow(pSCView->m_hWnd);
					Game_FreeDataEntry(pBuf);
				}
				GameMain_String_ReleaseBuffer(&reqCaption, 0);
				GameMain_String_ReleaseBuffer(&reqText, 0);
			}
		}
		fclose(f);
	}
}

extern "C" BOOL __cdecl Hook_CheckTilesetFileHeader1996(FILE *f) {
	tilesetMainHeader_t tileHead;

	if (!f)
		return FALSE;

	// The original program-native calls are required otherwise
	// the crash is spectacular.
	fseek(f, 0, SEEK_SET);
	fread(&tileHead, sizeof(tilesetMainHeader_t), 1, f);

	return memcmp(tileHead.szTypeHead, "MIFF", 4) == 0 && memcmp(tileHead.szSC2KHead, "SC2K", 4) == 0;
}

extern "C" void __cdecl Hook_VerifyAndLoadNewTiles1996(FILE *f) {
	DWORD dwMainSize;
	char *pBuf, szHeader[4];
	tilesetHeadInfo_t tilesetInfo;
	tilesetChunkHeader_t tilesetChunkHeader;
	CSimcityView *pSCView;

	// Seek and process 'Info' portion.
	fseek(f, 4, SEEK_SET);
	fread(&dwMainSize, 1, 4, f);
	dwMainSize = _byteswap_ulong(dwMainSize);
	fseek(f, 4, SEEK_CUR);
	fread(tilesetInfo.szHead, 4, 1, f);
	if (memcmp(tilesetInfo.szHead, "INFO", 4) != 0)
		return;

	fread(&tilesetInfo.dwSize, 4, 1, f);
	tilesetInfo.dwSize = _byteswap_ulong(tilesetInfo.dwSize);

	fread(szHeader, 1, 4, f);
	fseek(f, -4, SEEK_CUR);

	nRevType = REV_WIN;
	if (memcmp(szHeader, "_MAC", 4) == 0) {
		nRevType = REV_DOSMAC;
		// the following only applies to the very rare (and specific) Macintosh tilesets:
		// subtract the main header, the info header, the platform tag, and the 'TILE' chunk header
		// which then just leaves the number of tiles and subsequent SHAP/NAME lumps.
		dwMainSize -= sizeof(tilesetMainHeader_t) - sizeof(tilesetHeadInfo_t) - tilesetInfo.dwSize - sizeof(tilesetChunkHeader_t);
	}
	else if (memcmp(szHeader, "00W_", 4) == 0)
		nRevType = REV_W00;

	fseek(f, tilesetInfo.dwSize, SEEK_CUR);

	// Process 'Tile' portion.
	memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
	fread(tilesetChunkHeader.szHead, 4, 1, f);
	if (memcmp(tilesetChunkHeader.szHead, "TILE", 4) != 0)
		return;

	fread(&tilesetChunkHeader.dwSize, 4, 1, f);
	tilesetChunkHeader.dwSize = (nRevType != REV_DOSMAC) ? _byteswap_ulong(tilesetChunkHeader.dwSize) : dwMainSize;

	pBuf = (char *)malloc(tilesetChunkHeader.dwSize);
	if (!pBuf)
		Game_LoadTilesFromFile(f);
	else {
		memset(pBuf, 0, tilesetChunkHeader.dwSize);
		Game_GetAndLoadNextTileFileChunkToMemory(f, pBuf, tilesetChunkHeader.dwSize);
		Game_LoadTilesFromMemory(pBuf);
		free(pBuf);
	}

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis);
	if (pSCView) {
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __stdcall Hook_GetAndLoadNextTileFileChunkToMemory1996(FILE *f, char *pBuf, DWORD dwSize) {
	DWORD dwRemainingSize, dwChunkSize, dwFetchedSize;

	dwRemainingSize = dwSize;
	dwChunkSize = 0x8000;
	dwFetchedSize = 0;
	while (dwRemainingSize > 0) {
		if (dwChunkSize > dwRemainingSize)
			dwChunkSize = dwRemainingSize;
		dwFetchedSize = fread(pBuf, 1, dwChunkSize, f);
		if (dwFetchedSize == 0)
			break;
		dwRemainingSize -= dwFetchedSize;
		pBuf += dwFetchedSize;
	}
}

extern "C" void *__cdecl Hook_ReallocateDataEntry1996(char *pDest, char *pSrc) {
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

extern "C" void __cdecl Hook_LoadTilesFromMemory1996(tilesetMem_t *pTileMem) {
	WORD nMaxChunks, nChunk;
	tileMem_t *pTileMemEntry;
	tilesetChunkHeader_t tilesetChunkHeader;
	char *pBuf;
	BOOL bGotShap, bGotName, bResize;

	nMaxChunks = _byteswap_ushort(pTileMem->nMaxChunks);
	pTileMemEntry = &pTileMem->tileMem;
	for (nChunk = 0; nMaxChunks > nChunk; ++nChunk) {
		memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
		memcpy(tilesetChunkHeader.szHead, pTileMemEntry->szHead, 4);
		tilesetChunkHeader.dwSize = _byteswap_ulong(pTileMemEntry->dwSize);
		pBuf = &pTileMemEntry->pBuf;

		bGotShap = bGotName = FALSE;
		if (memcmp(tilesetChunkHeader.szHead, "SHAP", 4) == 0) {
			// This check is present to avoid a misalignment
			// situation that was originally occurring while
			// processing the reconstituted Macintosh-specific
			// MIF tilesets. It is necessary as a result of the
			// empty SHAP entries only containing the size + 4
			// (The size of the SHAP header).
			if (nRevType == REV_DOSMAC) {
				if (memcmp(pBuf, "SHAP", 4) == 0 && !tilesetChunkHeader.dwSize) {
					pTileMemEntry = (tileMem_t *)&pTileMemEntry->pBuf;
					continue;
				}
			}
			bGotShap = Game_ReadTileShapInformation((tileShap_t *)pBuf);
			if (!bGotShap) {
				if (!bTilesetLoadOutOfMemory && bResize) {
					bTilesetLoadOutOfMemory = TRUE;
					bResize = FALSE;
				}
			}
		}
		else if (memcmp(tilesetChunkHeader.szHead, "NAME", 4) == 0)
			bGotName = Game_ReadTileNameInformation((tileName_t *)pBuf);

		// Added. If this is set to true it stands to reason
		// you'd then want to break out of the loop.
		if (bTilesetLoadOutOfMemory)
			break;

		if (bGotShap || bGotName || bResize) {
			bResize = FALSE;
			pTileMemEntry = (tileMem_t *)&pBuf[tilesetChunkHeader.dwSize];
			continue;
		}

		bResize = TRUE;
		pTileMemEntry = (tileMem_t *)Game_ReallocateDataEntry((char *)pTileMem, pBuf);
	}
}

extern "C" void __cdecl Hook_LoadTilesFromFile1996(FILE *f) {
	char *pBuf, szHeader[4];
	WORD nMaxChunks, nChunk;
	DWORD dwSize;
	tilesetChunkHeader_t tilesetChunkHeader;
	BOOL bSomeBool;

	bTilesetLoadOutOfMemory = FALSE;
	pBuf = (char *)malloc(0x10000);
	if (pBuf) {
		fread(&nMaxChunks, 2, 1, f);
		nMaxChunks = _byteswap_ushort(nMaxChunks);
		for (nChunk = 0; nMaxChunks > nChunk; ++nChunk) {
			memset(pBuf, 0, 0x10000);
			memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
			fread(tilesetChunkHeader.szHead, 1, 4, f);
			if (feof(f))
				break;
			fread(&dwSize, 4, 1, f);
			bSomeBool = (dwSize & 0x1000000) == 0;
			tilesetChunkHeader.dwSize = _byteswap_ulong(dwSize);
			if (!bSomeBool)
				tilesetChunkHeader.dwSize += 1;

			if (memcmp(tilesetChunkHeader.szHead, "SHAP", 4) == 0) {
				// This check is present to avoid a misalignment
				// situation that was originally occurring while
				// processing the reconstituted Macintosh-specific
				// MIF tilesets. It is necessary as a result of the
				// empty SHAP entries only containing the size + 4
				// (The size of the SHAP header).
				if (nRevType == REV_DOSMAC) {
					fread(szHeader, 1, 4, f);
					fseek(f, -4, SEEK_CUR);
					if (memcmp(szHeader, "SHAP", 4) == 0 && !tilesetChunkHeader.dwSize)
						continue;
				}
				fread(pBuf, 1, tilesetChunkHeader.dwSize, f);
				if (!Game_ReadTileShapInformation((tileShap_t *)pBuf) && !bTilesetLoadOutOfMemory)
					bTilesetLoadOutOfMemory = TRUE;
			}
			else if (memcmp(tilesetChunkHeader.szHead, "NAME", 4) == 0) {
				fread(pBuf, 1, tilesetChunkHeader.dwSize, f);
				Game_ReadTileNameInformation((tileName_t *)pBuf);
			}
			else
				fseek(f, tilesetChunkHeader.dwSize, SEEK_CUR);

			// Added. If this is set to true it stands to reason
			// you'd then want to break out of the loop.
			if (bTilesetLoadOutOfMemory)
				break;
		}
		free(pBuf);
	}
	else
		bTilesetLoadOutOfMemory = TRUE;
}

extern "C" BOOL __cdecl Hook_ReadTileShapInformation1996(tileShap_t *pTileShap) {
	__int16 nSpriteID;
	WORD nWidth, nHeight;
	DWORD dwSize;

	// if 'nHeight' > 1 then change the tile sprite entry, otherwise just return TRUE.
	// This check avoids zero'ing a building that may not be contained within the
	// tileset that's being loaded.

	nSpriteID = _byteswap_ushort(pTileShap->nSpriteID);
	nWidth = _byteswap_ushort(pTileShap->nWidth);
	nHeight = _byteswap_ushort(pTileShap->nHeight);
	dwSize = _byteswap_ulong(pTileShap->dwSize);
	return (nHeight > 1) ? Game_ChangeTileSpriteEntry(nSpriteID, nWidth, nHeight, dwSize, &pTileShap->pBuf) : TRUE;
}

extern "C" BOOL __cdecl Hook_ReadTileNameInformation1996(tileName_t *pTileName) {
	WORD nTileNameID, nNameLength;
	char *pNewTileName;

	nTileNameID = _byteswap_ushort(pTileName->nTileNameID);
	nNameLength = _byteswap_ushort(pTileName->nNameLength);
	pNewTileName = (char *)Game_AllocateDataEntry(nNameLength + 1);
	if (pNewTileName) {
		memcpy(pNewTileName, &pTileName->pBuf, nNameLength);
		pNewTileName[nNameLength] = 0;
		if (pTileNames[nTileNameID]) {
			Game_FreeDataEntry(pTileNames[nTileNameID]);
			pTileNames[nTileNameID] = 0;
		}
		pTileNames[nTileNameID] = pNewTileName;
	}
	return TRUE;
}

extern "C" BOOL __cdecl Hook_ChangeTileSpriteEntry1996(int nSpriteID, WORD nWidth, WORD nHeight, DWORD dwSize, void *pBuf) {
	BYTE *pDst;

	pDst = (BYTE *)Game_AllocateDataEntry(dwSize);
	if (pDst) {
		memset(pDst, 0, dwSize);
		if (pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr) {
			Game_FreeDataEntry(pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr);
			pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = 0;
		}
		memcpy(pDst, pBuf, dwSize);
		if (nRevType >= REV_WIN && nRevType <= REV_DOSMAC) {
			int nConvRepl = 0;
			if (nRevType == REV_DOSMAC) {
				if (GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_MILITARY_HANGAR1)) {
					if (nHangar1Mode == HANGAR1_ANIM)
						nConvRepl = 1;
					else if (nHangar1Mode == HANGAR1_OPEN)
						nConvRepl = 2;
				}
			}
			else if (nRevType == REV_WIN)
				nConvRepl = 3;
			L_ConvertSprite(nWidth, nHeight, pDst, nRevType, nConvRepl);
		}
		pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = pDst;
		pArrSpriteHeaders[nSpriteID].wWidth = nWidth;
		pArrSpriteHeaders[nSpriteID].wHeight = nHeight;
		Cache_Sprite(nSpriteID, pDst, dwSize, nHeight, nWidth);
	}
	return (pDst) ? TRUE : FALSE;
}

void InstallSpriteAndTileSetHooks_SC2K1996(void) {
	// Hook LoadSpriteDataArchive
	SafeVirtualProtect((LPVOID)0x4029B4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4029B4, Hook_LoadSpriteDataArchive1996);

	// Hook into InitializeDataColorsFonts - move actual sprite loading into external call.
	SafeVirtualProtect((LPVOID)0x42C314, 30, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x42C314, 0x90, 30);
	NEWJMP((LPVOID)0x42C314, Hook_LoadSpriteArchives1996);

	// Hook CSimcityApp:LoadTileset
	SafeVirtualProtect((LPVOID)0x401E29, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E29, Hook_SimcityApp_LoadTileset1996);

	// Hook ReadTilesetFile
	SafeVirtualProtect((LPVOID)0x4021F8, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4021F8, Hook_ReadTilesetFile1996);

	// Hook CheckTilesetFileHeader
	SafeVirtualProtect((LPVOID)0x401280, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401280, Hook_CheckTilesetFileHeader1996);

	// Hook VerifyAndLoadNewTiles
	SafeVirtualProtect((LPVOID)0x4019F6, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4019F6, Hook_VerifyAndLoadNewTiles1996);

	// Hook GetAndLoadNextTileFileChunkToMemory
	SafeVirtualProtect((LPVOID)0x402739, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402739, Hook_GetAndLoadNextTileFileChunkToMemory1996);

	// Hook ReallocateDataEntry
	SafeVirtualProtect((LPVOID)0x40264E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40264E, Hook_ReallocateDataEntry1996);

	// Hook LoadTilesFromMemory
	SafeVirtualProtect((LPVOID)0x401654, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401654, Hook_LoadTilesFromMemory1996);

	// Hook LoadTilesFromFile
	SafeVirtualProtect((LPVOID)0x401C35, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401C35, Hook_LoadTilesFromFile1996);

	// Hook ReadTileShapInformation
	SafeVirtualProtect((LPVOID)0x403044, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403044, Hook_ReadTileShapInformation1996);

	// Hook ReadTileNameInformation
	SafeVirtualProtect((LPVOID)0x40260D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40260D, Hook_ReadTileNameInformation1996);

	// Hook ChangeTileSpriteEntry
	SafeVirtualProtect((LPVOID)0x4013E3, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4013E3, Hook_ChangeTileSpriteEntry1996);
}
