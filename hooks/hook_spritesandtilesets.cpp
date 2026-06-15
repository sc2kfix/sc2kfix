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
	int iReplacementsLoaded = 0;

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
					dwSize = _byteswap_ulong(pTileHeader->dwSize);
					dwOffset += sizeof(tilesetMainHeader_t);
					pTileInfo = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
					if (pTileInfo && memcmp(pTileInfo->szHead, "INFO", 4) == 0) {
						dwSize = _byteswap_ulong(pTileInfo->dwSize);
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
													ConsoleLog(LOG_DEBUG, "TILE: Loaded replacement large sprite for: %s\n", szSpriteNames[nSpriteID - SPRITE_LARGE_START]);
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

	ResetCustomTileNames();
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GameMain_CmdTarget_EndWaitCursor(pSCApp);

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __declspec(naked) __stdcall Hook_LoadSpriteArchives1996() {
	Init_SpriteCache(false);

	Game_LoadDataArchive(TILEDAT_DEFS_SPECIAL);
	Game_LoadDataArchive(TILEDAT_DEFS_LARGE);
	Game_LoadDataArchive(TILEDAT_DEFS_SMALLMED);

	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GAMEJMP(0x42C332)
}

extern "C" void __stdcall Hook_SimcityApp_LoadTileset1996() {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	CMFC3XString strFileTypes;
	CMFC3XString strCaption;
	CMFC3XFileDialog fileDialog;
	CMFC3XString strFilePath;
	int nPathLen, nFileLen, nNewLen;
	char szPath[MAX_PATH + 1];
	OPENFILENAMEA* pOfn;

	memset(szPath, 0, sizeof(szPath));

	GameMain_String_Cons(&strFileTypes);
	GameMain_String_Cons(&strCaption);

	GameMain_String_LoadStringA(&strFileTypes, 4004);
	GameMain_String_LoadStringA(&strCaption, 4005);

	GameMain_FileDialog_Cons(&fileDialog, (void *)1, "mif", "*.mif", OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, strFileTypes.m_pchData, 0);

	Game_SimcityApp_GetValueStringA(pThis, &strFilePath, aPaths, aTilesets);

	strcpy_s(fileDialog.m_szFileName, strFilePath.m_pchData);
	pOfn = &fileDialog.m_ofn;
	pOfn->lpstrInitialDir = fileDialog.m_szFileName;
	pOfn->lpstrTitle = strCaption.m_pchData;

	if (GameMain_FileDialog_DoModal(&fileDialog) == 1) {
		GameMain_CmdTarget_BeginWaitCursor(pThis);
		Game_ReadTilesetFile(pOfn->lpstrFile);

		nNewLen = 0;
		nPathLen = strlen(pOfn->lpstrFile);
		nFileLen = strlen(pOfn->lpstrFileTitle);
		if (nPathLen > 0 && nFileLen > 0) {
			nNewLen = nPathLen - nFileLen;
			if (nNewLen > 0) {
				strncpy_s(szPath, sizeof(szPath) - 1, pOfn->lpstrFile, nNewLen);
				if (L_IsPathValid(szPath))
					jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS] = szPath;
			}
		}

		GameMain_CmdTarget_EndWaitCursor(pThis);
	}

	GameMain_String_Dest(&strFilePath);

	Game_GameFileDialog_Dest(&fileDialog);

	GameMain_String_Dest(&strCaption);
	GameMain_String_Dest(&strFileTypes);
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
	char *pBuf;
	tilesetHeadInfo_t tilesetInfo;
	tilesetChunkHeader_t tilesetChunkHeader;
	CSimcityView *pSCView;

	// Seek and process 'Info' portion.
	fseek(f, sizeof(tilesetMainHeader_t), SEEK_SET);
	fread(tilesetInfo.szHead, 4, 1, f);
	if (memcmp(tilesetInfo.szHead, "INFO", 4) != 0)
		return;

	fread(&tilesetInfo.dwSize, 4, 1, f);
	tilesetInfo.dwSize = _byteswap_ulong(tilesetInfo.dwSize);
	fseek(f, tilesetInfo.dwSize, SEEK_CUR);

	// Process 'Tile' portion.
	memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
	fread(tilesetChunkHeader.szHead, 4, 1, f);
	if (memcmp(tilesetChunkHeader.szHead, "TILE", 4) != 0)
		return;

	fread(&tilesetChunkHeader.dwSize, 4, 1, f);
	tilesetChunkHeader.dwSize = _byteswap_ulong(tilesetChunkHeader.dwSize);
	
	pBuf = (char *)malloc(tilesetChunkHeader.dwSize);
	if (!pBuf)
		Game_LoadTilesFromFile(f);
	else {
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
	char *pBuf;
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
			memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
			fread(tilesetChunkHeader.szHead, 1, 4, f);
			if (feof(f))
				break;
			fread(&dwSize, 4, 1, f);
			bSomeBool = (dwSize & 0x1000000) == 0;
			tilesetChunkHeader.dwSize = _byteswap_ulong(dwSize);
			if (!bSomeBool)
				tilesetChunkHeader.dwSize += 1;
			fread(pBuf, 1, tilesetChunkHeader.dwSize, f);
			if (memcmp(tilesetChunkHeader.szHead, "SHAP", 4) == 0) {
				if (!Game_ReadTileShapInformation((tileShap_t *)pBuf) && !bTilesetLoadOutOfMemory)
					bTilesetLoadOutOfMemory = TRUE;
			}
			else if (memcmp(tilesetChunkHeader.szHead, "NAME", 4) == 0)
				Game_ReadTileNameInformation((tileName_t *)pBuf);

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
		if (pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr) {
			Game_FreeDataEntry(pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr);
			pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = 0;
		}
		memcpy(pDst, pBuf, dwSize);
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
