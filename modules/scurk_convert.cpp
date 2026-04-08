// sc2kfix modules/scurk_covert.cpp: conversion-related calls and dialogues for SCURK
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

//
// Calls and functions here are strictly to do with the new conversion implementation
// (not the original limited translation calls).
//
// At the moment it deals with doing a comprehensive conversion from Macintosh-type
// MIF and DOS TIL to Windows MIF to include ALL shapes - not just the original
// limited EDNUM range.
//
// Code related to the loading of the "fixed" set and merging of (parts of it) said
// set are also present here.

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include <sc2kfix.h>
#include "../resource.h"

// Functions to do with loading and referencing the "fixed"
// large tiles.

typedef struct {
	__int16 nSpriteID;
	__int16 nDBID;
	WORD nWidth;
	WORD nHeight;
	int nSize;
	BOOL bInclude; // Only relevant during merging.
	BYTE *pTileDat;
} recordedTiles_t;

std::vector<recordedTiles_t> fixedTiles;

recordedTiles_t *L_SCURK_GetRecordedTileBySpriteID(__int16 nSpriteID) {
	for (unsigned i = 0; i < fixedTiles.size(); ++i) {
		recordedTiles_t *pFixedTile = &fixedTiles[i];
		if (pFixedTile && pFixedTile->nSpriteID == nSpriteID)
			return pFixedTile;
	}
	return NULL;
}

static void L_SCURK_ClearFixedTiles() {
	for (unsigned i = 0; i < fixedTiles.size(); ++i) {
		recordedTiles_t *pFixedTile = &fixedTiles[i];
		if (pFixedTile) {
			if (pFixedTile->pTileDat) {
				if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_FIXEDTILES)
					ConsoleLog(LOG_DEBUG, "Clearing (%u)\n", pFixedTile->nSpriteID);
				R_SCURK_WRP_gFreeBlock(pFixedTile->pTileDat);
				pFixedTile->pTileDat = 0;
			}
		}
	}

	fixedTiles.clear();
}

void L_SCURK_LoadFixedLargeSpritesRsrc(cEditableTileSet *pThis) {
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
	recordedTiles_t fixedEnt;

	L_SCURK_ClearFixedTiles();

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
											nDBID = pThis->mDBIndexFromShapeNum[nSpriteID];

											// Only replace sprites with a height above 1 (similar to the main game
											// under these circumstances).
											if (nHeight > 1) {
												if (pThis->mTiles[nDBID])
													R_SCURK_WRP_gFreeBlock(pThis->mTiles[nDBID]);
												pThis->mTiles[nDBID] = (uint8_t *)R_SCURK_WRP_gAllocBlock(dwSize_Shap);
												if (pThis->mTiles[nDBID]) {
													memset(pThis->mTiles[nDBID], 0, dwSize_Shap);
													memcpy(pThis->mTiles[nDBID], &pTileShap->pBuf, dwSize_Shap);
													pThis->mTileSet->pData[nDBID].sprHeader.wWidth = nWidth;
													pThis->mTileSet->pData[nDBID].sprHeader.wHeight = nHeight;
													pThis->mTileSizeTable[nDBID] = dwSize_Shap;
												}
												bGotShap = (pThis->mTiles[nDBID]) ? TRUE : FALSE;
											}
											else
												bGotShap = TRUE;

											if (bGotShap && nHeight > 1 && nSpriteID >= SPRITE_LARGE_START) {
												fixedEnt.nSpriteID = nSpriteID;
												fixedEnt.nDBID = nDBID;
												fixedEnt.nHeight = nHeight;
												fixedEnt.nWidth = nWidth;
												fixedEnt.nSize = dwSize_Shap;
												fixedEnt.bInclude = FALSE;
												fixedEnt.pTileDat = (BYTE *)R_SCURK_WRP_gAllocBlock(dwSize_Shap);
												memcpy(fixedEnt.pTileDat, pThis->mTiles[nDBID], dwSize_Shap);
												fixedTiles.push_back(fixedEnt);

												if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_FIXEDTILES)
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
}

// The dialogue for merging "fixed" object(s) into
// (what you deem to be) applicable destination
// sets

static void InsertObjectEntryViewRow(HWND hDlgListView, int iRow, const char *pObjectID, const char *pObjectName) {
	ListView_InsertItemText(hDlgListView, iRow);
	ListView_SetItemText(hDlgListView, iRow, 0, "");
	ListView_SetItemText(hDlgListView, iRow, 1, (char *)pObjectID);
	ListView_SetItemText(hDlgListView, iRow, 2, (char *)pObjectName);
}

static void PopulateObjectEntryList(HWND hDlgListView) {
	int nIdx = 0;
	char szObjectID[16 + 1], szObjectName[64 + 1];

	ListView_DeleteAllItems(hDlgListView);
	for (unsigned i = 0; i < fixedTiles.size(); i++) {
		recordedTiles_t *pRecordedTile = &fixedTiles[i];
		if (pRecordedTile && pRecordedTile->pTileDat) {
			memset(szObjectID, 0, sizeof(szObjectID));
			memset(szObjectName, 0, sizeof(szObjectName));

			sprintf_s(szObjectID, "%d", pRecordedTile->nSpriteID);
			sprintf_s(szObjectName, "(%s) %s", (
					(pRecordedTile->nSpriteID < SPRITE_LARGE_START) ? "Small" : 
					(pRecordedTile->nSpriteID < SPRITE_MEDIUM_START) ? "Tiny" : "Large"
					),
					(
					(pRecordedTile->nSpriteID < SPRITE_LARGE_START) ? szSpriteNames[pRecordedTile->nSpriteID - SPRITE_MEDIUM_START] : 
					(pRecordedTile->nSpriteID < SPRITE_MEDIUM_START) ? szSpriteNames[pRecordedTile->nSpriteID] : szSpriteNames[pRecordedTile->nSpriteID - SPRITE_LARGE_START]
					)
				);
			InsertObjectEntryViewRow(hDlgListView, nIdx, szObjectID, szObjectName);
			++nIdx;
		}
	}
}

static void ToggleAllObjectItems(HWND hDlgListView, BOOL bEnable) {
	for (int i = 0; i < ListView_GetItemCount(hDlgListView); i++)
		ListView_SetCheckState(hDlgListView, i, bEnable);
}

static void GetIncludedObjects(HWND hDlgListView) {
	char szObjectID[16 + 1];
	__int16 nObjectID;
	recordedTiles_t *pRecordedTile;

	for (int i = 0; i < ListView_GetItemCount(hDlgListView); i++) {
		memset(szObjectID, 0, sizeof(szObjectID));

		ListView_GetItemText(hDlgListView, i, 1, szObjectID, countof(szObjectID) - 1);
		nObjectID = (__int16)atoi(szObjectID);

		pRecordedTile = L_SCURK_GetRecordedTileBySpriteID(nObjectID);
		if (pRecordedTile)
			pRecordedTile->bInclude = (ListView_GetCheckState(hDlgListView, i) == TRUE) ? TRUE : FALSE;
	}
}

static void ClearIncludedObjects() {
	for (unsigned i = 0; i < fixedTiles.size(); i++) {
		recordedTiles_t *pRecordedTile = &fixedTiles[i];
		if (pRecordedTile && pRecordedTile->pTileDat)
			pRecordedTile->bInclude = FALSE;
	}
}

BOOL CALLBACK SelectFixedObjectsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hDlgListView;
	int colOrder[] = { 1, 2, 0 };

	switch (message) {
		case WM_INITDIALOG:
			hDlgListView = GetDlgItem(hwndDlg, IDC_OBJECTLIST);
			ListView_SetExtendedListViewStyle(hDlgListView, LVS_EX_CHECKBOXES);

			// In order for the checkbox column to be at the end, it must be
			// defined in this fashion.
			ListView_InsertColumnEntry(hDlgListView, 0, "Enable", 50, (LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM), LVCFMT_LEFT);
			ListView_InsertColumnEntry(hDlgListView, 1, "Object ID", 100, (LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM), LVCFMT_LEFT);
			ListView_InsertColumnEntry(hDlgListView, 2, "Object Name", 250, (LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM), LVCFMT_LEFT);

			ListView_SetColumnOrderArray(hDlgListView, 3, colOrder);

			PopulateObjectEntryList(hDlgListView);

			CenterDialogBox(hwndDlg);
			return TRUE;

		case WM_COMMAND:
			hDlgListView = GetDlgItem(hwndDlg, IDC_OBJECTLIST);
			switch (GET_WM_COMMAND_ID(wParam, lParam)) {
				case IDC_ENBALLBUT:
					ToggleAllObjectItems(hDlgListView, TRUE);
					return TRUE;
				case IDC_DISALLBUT:
					ToggleAllObjectItems(hDlgListView, FALSE);
					return TRUE;
				case IDOK:
					GetIncludedObjects(hDlgListView);
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDCANCEL:
					// We always want to ensure all items
					// are properly cleared if the dialogue
					// is just cancelled; there's to be NO
					// persistence between conversion attempts.
					ClearIncludedObjects();
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
	}
	return FALSE;
}

static void L_SCURK_DoSelectFixedObjects(winscurkMDIClient *pThis) {
	// Only load the dialogue if the fixed tiles
	// have been recorded.
	if (fixedTiles.size() > 0)
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_CONVERT_SELFIXOBJECTS), pThis->pWnd->HWindow, SelectFixedObjectsDialogProc);
}

// Functions to do with conversion

static void GetFileDirectory(char *pLoadPath) {
	char *p;
	int nLen;

	nLen = strlen(pLoadPath);
	for (p = &pLoadPath[nLen]; nLen > 0 && *p != '\\'; --p)
		--nLen;
	pLoadPath[nLen] = 0;
}

static BOOL L_SCURK_DirectConvert_WorkingSetCheck(winscurkMDIClient *pThis, int nLoad) {
	if (nLoad != CONVSAVEAS_LOADWRK)
		return TRUE;
	int nRet = R_SCURK_WRP_gScurkMessage(29005, 29003, MB_YESNOCANCEL);
	if (nRet) {
		if (nRet != IDCANCEL) {
			if (nRet == IDYES) {
				R_SCURK_WRP_winscurkMDIClient_CmFileSaveWorking(pThis);
				int *pSaveSucceeded = R_SCURK_WRP_GetgSaveSucceeded();
				if (*pSaveSucceeded)
					return TRUE;
			}
			else if (nRet == IDNO)
				return TRUE;
		}
	}
	return FALSE;
}

static void L_SCURK_GetIncludedFixedShape(tileConv_t *pObjSet, int nShapNum, int nDBID, recordedTiles_t *pFixedOut) {
	for (unsigned i = 0; i < fixedTiles.size(); ++i) {
		recordedTiles_t *pFixedEnt = &fixedTiles[i];
		if (pFixedEnt && pFixedEnt->pTileDat && pFixedEnt->nDBID == nDBID) {
			if (!pFixedEnt->bInclude)
				break;
			if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_FIXEDTILES)
				ConsoleLog(LOG_DEBUG, "Include Fixed Shape: (%d, %d) [%s] (%u, %u) (%d)\n", nShapNum, nDBID, szInternalSpriteName[nShapNum], pFixedEnt->nHeight, pFixedEnt->nWidth, pFixedEnt->nSize);
			pFixedOut->nSpriteID = pFixedEnt->nSpriteID;
			pFixedOut->nDBID = pFixedEnt->nDBID;
			pFixedOut->nWidth = pFixedEnt->nWidth;
			pFixedOut->nHeight = pFixedEnt->nHeight;
			pFixedOut->nSize = pFixedEnt->nSize;
			pFixedOut->bInclude = TRUE;
			pFixedOut->pTileDat = pFixedEnt->pTileDat;
			return;
		}
	}
	pFixedOut->nSpriteID = nShapNum;
	pFixedOut->nDBID = nDBID;
	pFixedOut->nWidth = pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth;
	pFixedOut->nHeight = pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight;
	pFixedOut->nSize = pObjSet->pObjectSetSize[nDBID];
	pFixedOut->bInclude = FALSE;
	pFixedOut->pTileDat = pObjSet->pObjects[nDBID];
}

static int L_SCURK_SaveConvertedSet(tileConv_t *pObjSet, cEditableTileSet *pWorkSet, const char *pSaveFile, int *nFixCnt) {
	int nRes, nSize;
	FILE *f;
	DWORD dwBuf, dwContLen;
	WORD wBuf, nWidth, nHeight;
	int nShapNum, nDBID;
	BYTE bBuf[4], *pShapDat;
	char szHeadName[4];
	char szInfoPortion[114];
	recordedTiles_t recordedEnt;

	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CONVERT)
		ConsoleLog(LOG_DEBUG, "SaveConvertedSet(%s): (%u)\n", pSaveFile, pObjSet->pObjectSet->nSprites);

	nRes = 0;
	// File backup call - but only if the specified
	// 'original' file already exists.
	L_SCURK_BackupFile(pSaveFile);
	f = old_fopen(pSaveFile, "wb");
	if (f) {
		// Length of file calculation - starting.
		dwBuf = 0x2758;
		dwContLen = 0;
		for (nShapNum = 0; nShapNum < pObjSet->pObjectSet->nSprites; ++nShapNum) {
			R_SCURK_WRP_gUpdateWaitWindow();
			nDBID = pWorkSet->mDBIndexFromShapeNum[nShapNum];
			if (nDBID == SPRITE_SMALL_START) {
				if (nShapNum != SPRITE_SMALL_START)
					continue;
			}
			else if (nDBID == SPRITE_MEDIUM_START) {
				if (nShapNum != SPRITE_MEDIUM_START)
					continue;
			}
			else if (nDBID == SPRITE_LARGE_START) {
				if (nShapNum != SPRITE_LARGE_START)
					continue;
			}
			memset(&recordedEnt, 0, sizeof(recordedTiles_t));
			L_SCURK_GetIncludedFixedShape(pObjSet, nShapNum, nDBID, &recordedEnt);
			nSize = recordedEnt.nSize;
			if (nSize <= 0)
				continue;
			dwContLen += nSize;
		}
		dwBuf = _byteswap_ulong(dwBuf + dwContLen);

		// Initial part of the header:
		// MIFF
		// Length of File
		// SC2K
		// INFO
		memcpy(szHeadName, "MIFF", 4);
		fwrite(szHeadName, 1, sizeof(szHeadName), f);
		fwrite(&dwBuf, sizeof(dwBuf), 1, f);
		memcpy(szHeadName, "SC2K", 4);
		fwrite(szHeadName, 1, sizeof(szHeadName), f);
		memcpy(szHeadName, "INFO", 4);
		fwrite(szHeadName, 1, sizeof(szHeadName), f);

		// Next part:
		// Length of tileset information (00 00 00 72) (114)
		// * It should be noted that the above length "doesn't"
		//   have to be 114, however it is a chosen default for
		//   the Windows tilesets. (It's not present in the built-in
		//   Macintosh tilesets)
		//
		// szInfoPortion
		// * It otherwise comprises (or used to comprise) of:
		//   - Platform header: _MAC or NIW_ (_WIN backwards) (Always present)
		//   - Filename plus full path (This was only recorded in the Win 3.1 version of SCURK - or other non-public / pre-release builds)
		//   - Program of record: winSCURK (This is always present and used regardless of whether the save is performed with the Windows or Macintosh versions)
		//
		//   The rest otherwise "appears" to be junk.
		//   (Let's now zero it by default)
		memset(szInfoPortion, 0, sizeof(szInfoPortion));

		memset(bBuf, 0, sizeof(bBuf));
		bBuf[3] = sizeof(szInfoPortion);
		fwrite(bBuf, 1, sizeof(bBuf), f);
		// Ordered in this fashion since originally it would record
		// the filename plus the full path, then it would be truncated
		// if somehow it impinged on the program of record.
		// The first part hasn't been set since the WIN3.1 edition
		// of the program; the line is left here for original reference
		// purposes.
		// szInfoPortion[54] = mTileFileName[0]; // Remote var unused.
		old_strcpy(&szInfoPortion[94], "winSCURK");
		*(DWORD *)szInfoPortion = '_WIN'; // This gets reversed
		fwrite(szInfoPortion, 1, sizeof(szInfoPortion), f);

		// Next part:
		// TILE
		// Length of Content
		// Chunk Count
		memcpy(szHeadName, "TILE", 4);
		fwrite(szHeadName, 1, sizeof(szHeadName), f);
		dwBuf = _byteswap_ulong(dwContLen + CONV_TILE_LOC(pObjSet->pObjectSet->nSprites));
		fwrite(&dwBuf, sizeof(dwBuf), 1, f);
		wBuf = _byteswap_ushort(pObjSet->pObjectSet->nSprites);
		fwrite(&wBuf, sizeof(wBuf), 1, f);

		// The main body of SHAP entries.
		for (nShapNum = 0; nShapNum < pObjSet->pObjectSet->nSprites; ++nShapNum) {
			R_SCURK_WRP_gUpdateWaitWindow();
			// SHAP
			// Length of Content
			// Shape ID
			// Width
			// Height
			// Length of Content (minus header)
			// Content
			nDBID = pWorkSet->mDBIndexFromShapeNum[nShapNum];
			if (nDBID == SPRITE_SMALL_START) {
				if (nShapNum != SPRITE_SMALL_START)
					continue;
			}
			else if (nDBID == SPRITE_MEDIUM_START) {
				if (nShapNum != SPRITE_MEDIUM_START)
					continue;
			}
			else if (nDBID == SPRITE_LARGE_START) {
				if (nShapNum != SPRITE_LARGE_START)
					continue;
			}
			memset(&recordedEnt, 0, sizeof(recordedTiles_t));
			L_SCURK_GetIncludedFixedShape(pObjSet, nShapNum, nDBID, &recordedEnt);
			nSize = recordedEnt.nSize;
			if (nSize <= 0)
				continue;

			nWidth = recordedEnt.nWidth;
			nHeight = recordedEnt.nHeight;
			if (nHeight < 1)
				nHeight = 1;
			pShapDat = recordedEnt.pTileDat;

			memcpy(szHeadName, "SHAP", 4);
			fwrite(szHeadName, 1, sizeof(szHeadName), f);
			dwBuf = _byteswap_ulong(nSize + sizeof(tilesetShapHeader_t));
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			wBuf = _byteswap_ushort(nShapNum);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _byteswap_ushort(nWidth);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _byteswap_ushort(nHeight);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			dwBuf = _byteswap_ulong(nSize);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			fwrite(pShapDat, nSize, 1, f);

			if (recordedEnt.bInclude)
				++*nFixCnt;
		}
		fclose(f);
		nRes = 1;
	}

	return nRes;
}

static void L_SCURK_ConvertFromMac(tileConv_t *pObjSet, WORD nDBID) {
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, pBit;
	WORD nShapeWidth, nShapeHeight, nCurrWidth;
	BOOL bDone;
	WORD pTileRemainingBitCount;

	if (pObjSet->pObjectSet) {
		nCurrWidth = 0;
		pTileBits = pObjSet->pObjects[nDBID];
		if (pTileBits) {
			nShapeHeight = pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight;
			nShapeWidth = pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth;

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
								pBit = (*pTileBits == 0xFF) ? 0x00 : DOSMacPalTable[*pTileBits];
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
}

static int L_SCURK_ConvertMacMIFAndSave(tileConv_t *pObjSet, const char *pLoadFile, const char *pSaveFile, int *nFixCnt) {
	winscurkApp *pSCApp;
	cEditableTileSet *pWorkSet;
	int nRes;
	FILE *f;
	tilesetMainHeader_t mainHeader;
	tilesetHeadInfo_t infoHeader;
	char szHeader[4];
	tilesetTileInfo_t tileHeader;
	tilesetChunkHeader_t chunkHeader;
	DWORD dwSize;
	tilesetShapHeader_t shapHeader;
	WORD nSpriteID, nWidth, nHeight;
	WORD nDBID;
	BOOL bSave;

	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CONVERT)
		ConsoleLog(LOG_DEBUG, "ConvertMacMIFAndSave(%s, %s): (%u)\n", pLoadFile, pSaveFile, pObjSet->pObjectSet->nSprites);

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	// This is so we can get valid nDBID and nEdNum values.
	pWorkSet = pSCApp->mWorkingTiles;

	nRes = 0;
	bSave = FALSE;
	f = old_fopen(pLoadFile, "rb");
	if (f) {
		fread(&mainHeader, sizeof(tilesetMainHeader_t), 1, f);
		if (memcmp(mainHeader.szTypeHead, "MIFF", 4) == 0 &&
			memcmp(mainHeader.szSC2KHead, "SC2K", 4) == 0) {
			fread(&infoHeader, sizeof(tilesetHeadInfo_t), 1, f);
			if (memcmp(infoHeader.szHead, "INFO", 4) == 0) {
				fread(szHeader, 1, 4, f);
				fseek(f, -4, SEEK_CUR);
				// Set nRes to -1 here by default.
				// -1 indicates that the detected header
				// is "not" Macintosh.
				nRes = -1;
				if (memcmp(szHeader, "_MAC", 4) == 0) {
					// Set nRes back to 0 once we're sure.
					nRes = 0;
					mainHeader.dwSize = _byteswap_ulong(mainHeader.dwSize);
					infoHeader.dwSize = _byteswap_ulong(infoHeader.dwSize);

					fseek(f, infoHeader.dwSize, SEEK_CUR);

					fread(&tileHeader, sizeof(tilesetTileInfo_t), 1, f);
					if (memcmp(tileHeader.szHead, "TILE", 4) == 0) {
						tileHeader.dwSize = _byteswap_ulong(tileHeader.dwSize);
						tileHeader.nMaxChunks = _byteswap_ushort(tileHeader.nMaxChunks);

						for (WORD nChunk = 0; nChunk < tileHeader.nMaxChunks; ++nChunk) {
							R_SCURK_WRP_gUpdateWaitWindow();
							memset(&chunkHeader, 0, sizeof(tilesetChunkHeader_t));
							fread(chunkHeader.szHead, 1, 4, f);
							if (feof(f))
								break;
							fread(&dwSize, 4, 1, f);
							chunkHeader.dwSize = _byteswap_ulong(dwSize);

							if (memcmp(chunkHeader.szHead, "SHAP", 4) == 0) {
								// This check is present to avoid a misalignment
								// situation that was originally occurring while
								// processing the reconstituted Macintosh-specific
								// MIF tilesets. It is necessary as a result of the
								// empty SHAP entries only containing the size + 4
								// (The size of the SHAP header).
								fread(szHeader, 1, 4, f);
								fseek(f, -4, SEEK_CUR);
								if (memcmp(szHeader, "SHAP", 4) == 0 && !chunkHeader.dwSize)
									continue;

								memset(&shapHeader, 0, sizeof(tilesetShapHeader_t));

								fread(&nSpriteID, 2, 1, f);
								fread(&nWidth, 2, 1, f);
								fread(&nHeight, 2, 1, f);
								fread(&dwSize, 4, 1, f);

								shapHeader.nSpriteID = _byteswap_ushort(nSpriteID);
								shapHeader.nWidth = _byteswap_ushort(nWidth);
								shapHeader.nHeight = _byteswap_ushort(nHeight);
								shapHeader.dwSize = _byteswap_ulong(dwSize);

								nDBID = pWorkSet->mDBIndexFromShapeNum[shapHeader.nSpriteID];

								if (pObjSet->pObjects[nDBID])
									R_SCURK_WRP_gFreeBlock(pObjSet->pObjects[nDBID]);
								pObjSet->pObjects[nDBID] = (BYTE *)R_SCURK_WRP_gAllocBlock(shapHeader.dwSize);
								memset(pObjSet->pObjects[nDBID], 0, shapHeader.dwSize);
								pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight = shapHeader.nHeight;
								pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth = shapHeader.nWidth;
								pObjSet->pObjectSetSize[nDBID] = shapHeader.dwSize;

								fread(pObjSet->pObjects[nDBID], shapHeader.dwSize, 1, f);

								L_SCURK_ConvertFromMac(pObjSet, nDBID);
							}
							else
								fseek(f, chunkHeader.dwSize, SEEK_CUR);

							bSave = TRUE;
						}
					}
				}
			}
		}
		fclose(f);
	}

	if (bSave)
		nRes = L_SCURK_SaveConvertedSet(pObjSet, pWorkSet, pSaveFile, nFixCnt);

	return nRes;
}

static void L_SCURK_ConvertFromDOS(tileConv_t *pObjSet, WORD nDBID, BYTE *pDOSTileBuf) {
	BYTE *pDOSTileBits, *pTileBitsBuf, *pTileBits;
	int nTileSize;
	BOOL bDone;
	BYTE pDOSTileChunkMode, pDOSTileBitCount;
	WORD pDOSTileRemainingBitCount;

	if (pObjSet->pObjects[nDBID])
		R_SCURK_WRP_gFreeBlock(pObjSet->pObjects[nDBID]);
	pObjSet->pObjects[nDBID] = (BYTE *)R_SCURK_WRP_gAllocBlock(0xFFFF);
	memset(pObjSet->pObjects[nDBID], 0, 0xFFFF);
	pDOSTileBits = pDOSTileBuf;
	pTileBitsBuf = pObjSet->pObjects[nDBID];
	nTileSize = 0;
	pTileBits = 0;
	if (SPRITEDOSDATA(pDOSTileBuf)->nChunkMode != TIL_CM_NEWROWSTART) {
		SPRITEDATA(pTileBitsBuf)->nCount = 0;
		SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_NEWROWSTART;
		pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
		pTileBits = pTileBitsBuf;
		nTileSize = 2;
		++pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight;
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
				*pTileBitsBuf++ = DOSMacPalTable[*pDOSTileBits++];
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
	pObjSet->pObjectSetSize[nDBID] = nTileSize;
	pObjSet->pObjects[nDBID] = (BYTE *)R_SCURK_WRP_gResizeBlock(pObjSet->pObjects[nDBID], nTileSize);
}

static int L_SCURK_ConvertDOSTILAndSave(tileConv_t *pObjSet, const char *pLoadFile, const char *pSaveFile, int *nFixCnt) {
	winscurkApp *pSCApp;
	cEditableTileSet *pWorkSet;
	int nRes;
	FILE *f;
	BYTE *lpBuffer;
	tilMainStruct_t Buffer;
	tilHeader_t *lpLargeShapeBuf, *lpSmallShapeBuf, *lpOtherShapeBuf;
	DWORD dwLargeSize, dwLargeOffset, dwSmallSize, dwSmallOffset, dwOtherSize, dwOtherOffset;
	WORD nShapNum, nDBID;
	__int16 nEdNum;
	BOOL bValid, bSave;
	tilesetShapVerify_t validTiles[SPRITE_COUNT];

	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CONVERT)
		ConsoleLog(LOG_DEBUG, "ConvertDOSTILAndSave(%s, %s): (%u)\n", pLoadFile, pSaveFile, pObjSet->pObjectSet->nSprites);

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	// This is so we can get valid nDBID and nEdNum values.
	pWorkSet = pSCApp->mWorkingTiles;

	nRes = 0;

	memset(validTiles, 0, sizeof(validTiles));

	bSave = FALSE;
	f = old_fopen(pLoadFile, "rb");
	if (f) {
		lpBuffer = (BYTE *)R_SCURK_WRP_gAllocBlock(0xFFFF);
		fread(&Buffer, 1, 0x80, f);
		lpLargeShapeBuf = (tilHeader_t *)R_SCURK_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwLargeOffset, SEEK_SET);
		fread(lpLargeShapeBuf, 1, 0x2EE0, f);
		dwLargeSize = Buffer.dwLargeSize;
		for (nShapNum = SPRITE_LARGE_START; nShapNum < SPRITE_COUNT; ++nShapNum) {
			R_SCURK_WRP_gUpdateWaitWindow();
			nEdNum = R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(pWorkSet, nShapNum - SPRITE_LARGE_START);
			nDBID = pWorkSet->mDBIndexFromShapeNum[nShapNum];
			dwLargeOffset = lpLargeShapeBuf[nShapNum].dwOffset;
			validTiles[nShapNum].nSpriteID = nShapNum;
			bValid = TRUE;
			if (nEdNum < 0) {
				if (lpLargeShapeBuf[nShapNum].height < 2)
					bValid = FALSE;
			}
			if (dwLargeOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nShapNum].nHeight = lpLargeShapeBuf[nShapNum].height;
					validTiles[nShapNum].nWidth = lpLargeShapeBuf[nShapNum].width;
				}
			}
			validTiles[nShapNum].nValidated = (dwLargeOffset != 0xFFFFFFFF && bValid) ? 1 : 0;
			if (validTiles[nShapNum].nValidated != 2) {
				if (validTiles[nShapNum].nValidated == 1) {
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight = validTiles[nShapNum].nHeight;
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth = validTiles[nShapNum].nWidth;
				}
			}
			fseek(f, dwLargeSize + dwLargeOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			if (validTiles[nShapNum].nValidated == 1)
				L_SCURK_ConvertFromDOS(pObjSet, nDBID, lpBuffer);
		}

		lpOtherShapeBuf = (tilHeader_t *)R_SCURK_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwOtherOffset, SEEK_SET);
		fread(lpOtherShapeBuf, 1, 0x2EE0, f);
		dwOtherSize = Buffer.dwOtherSize;
		for (nShapNum = SPRITE_MEDIUM_START; nShapNum < SPRITE_LARGE_START; ++nShapNum) {
			R_SCURK_WRP_gUpdateWaitWindow();
			nEdNum = R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(pWorkSet, nShapNum - SPRITE_MEDIUM_START);
			nDBID = pWorkSet->mDBIndexFromShapeNum[nShapNum];
			dwOtherOffset = lpOtherShapeBuf[nShapNum].dwOffset;
			validTiles[nShapNum].nSpriteID = nShapNum;
			bValid = TRUE;
			if (nEdNum < 0) {
				if (lpOtherShapeBuf[nShapNum].height < 2)
					bValid = FALSE;
			}
			if (dwOtherOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nShapNum].nHeight = lpOtherShapeBuf[nShapNum].height;
					validTiles[nShapNum].nWidth = lpOtherShapeBuf[nShapNum].width;
				}
			}
			validTiles[nShapNum].nValidated = (dwOtherOffset != 0xFFFFFFFF && bValid) ? 1 : 0;
			if (validTiles[nShapNum].nValidated != 2) {
				if (validTiles[nShapNum].nValidated == 1) {
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight = validTiles[nShapNum].nHeight;
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth = validTiles[nShapNum].nWidth;
				}
			}
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			if (validTiles[nShapNum].nValidated == 1)
				L_SCURK_ConvertFromDOS(pObjSet, nDBID, lpBuffer);
		}

		lpSmallShapeBuf = (tilHeader_t *)R_SCURK_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwSmallOffset, SEEK_SET);
		fread(lpSmallShapeBuf, 1, 0x2EE0, f);
		dwSmallSize = Buffer.dwSmallSize;
		for (nShapNum = SPRITE_SMALL_START; nShapNum < SPRITE_LARGE_START; ++nShapNum) {
			R_SCURK_WRP_gUpdateWaitWindow();
			if (nShapNum < SPRITE_MEDIUM_START)
				nEdNum = R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(pWorkSet, nShapNum);
			else
				nEdNum = R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(pWorkSet, nShapNum - SPRITE_MEDIUM_START);
			nDBID = pWorkSet->mDBIndexFromShapeNum[nShapNum];
			dwSmallOffset = lpSmallShapeBuf[nShapNum].dwOffset;
			validTiles[nShapNum].nSpriteID = nShapNum;
			bValid = TRUE;
			if (nEdNum < 0) {
				if (lpSmallShapeBuf[nShapNum].height < 2)
					bValid = FALSE;
			}
			if (dwSmallOffset != 0xFFFFFFFF) {
				if (bValid) {
					validTiles[nShapNum].nHeight = lpSmallShapeBuf[nShapNum].height;
					validTiles[nShapNum].nWidth = lpSmallShapeBuf[nShapNum].width;
					validTiles[nShapNum].nValidated = 1;
				}
				else
					validTiles[nShapNum].nValidated = 2;
			}
			else if (validTiles[nShapNum].nValidated == 1)
				validTiles[nShapNum].nValidated = 2;
			if (validTiles[nShapNum].nValidated != 2) {
				if (validTiles[nShapNum].nValidated == 1) {
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wHeight = validTiles[nShapNum].nHeight;
					pObjSet->pObjectSet->pData[nDBID].sprHeader.wWidth = validTiles[nShapNum].nWidth;
				}
			}
			fseek(f, dwSmallSize + dwSmallOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			// Only free/(replace) if:
			// a) the tile has been successfully validated once here and now.
			// b) the tile isn't valid (and wasn't previously valid - ie from a prior archive - it's a skip case)
			if (validTiles[nShapNum].nValidated != 2)
				L_SCURK_ConvertFromDOS(pObjSet, nDBID, lpBuffer);
		}

		R_SCURK_WRP_gFreeBlock(lpOtherShapeBuf);
		R_SCURK_WRP_gFreeBlock(lpSmallShapeBuf);
		R_SCURK_WRP_gFreeBlock(lpLargeShapeBuf);

		R_SCURK_WRP_gFreeBlock(lpBuffer);
		fclose(f);

		bSave = TRUE;
	}

	if (bSave)
		nRes = L_SCURK_SaveConvertedSet(pObjSet, pWorkSet, pSaveFile, nFixCnt);

	return nRes;
}

static int L_SCURK_ConvertAndSaveSet(const char *pLoadFile, const char *pSaveFile, int nType, int *nFixCnt) {
	int nSize, nRes;
	tileConv_t convObjectSet;
	char szMeterStr[128 + 1];
	BC45Xstring titleStr;

	memset(&convObjectSet, 0, sizeof(convObjectSet));

	if (nType < CONVTYPE_MACMIF || nType > CONVTYPE_DOSTIL)
		return 0;

	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CONVERT)
		ConsoleLog(LOG_DEBUG, "ConvertAndSaveSet(%s, %s, %d)\n", pLoadFile, pSaveFile, nType);

	convObjectSet.nObjectNum = SPRITE_COUNT;
	nSize = 6040;
	convObjectSet.pObjectSetSize = (int *)R_SCURK_WRP_gAllocBlock(nSize);
	memset(convObjectSet.pObjectSetSize, 0, nSize);
	nSize = sizeof(sprite_file_header_t) * convObjectSet.nObjectNum + sizeof(__int16);
	convObjectSet.pObjectSet = (sprite_archive_t *)R_SCURK_WRP_gAllocBlock(nSize);
	memset(convObjectSet.pObjectSet, 0, nSize);
	convObjectSet.pObjectSet->nSprites = convObjectSet.nObjectNum;

	for (int i = 0; i < SPRITE_COUNT + 10; ++i)
		convObjectSet.pObjects[i] = 0;

	R_SCURK_WRP_gScurkLoadString(&titleStr, 29003);
	strcpy_s(szMeterStr, (nType == CONVTYPE_MACMIF) ? "Converting Macintosh Set" : "Converting DOS Set");

	R_SCURK_WRP_gBeginWaitWindow(686, szMeterStr, (TBC45XModule *)titleStr.p->array);
	if (nType == CONVTYPE_MACMIF)
		nRes = L_SCURK_ConvertMacMIFAndSave(&convObjectSet, pLoadFile, pSaveFile, nFixCnt);
	else
		nRes = L_SCURK_ConvertDOSTILAndSave(&convObjectSet, pLoadFile, pSaveFile, nFixCnt);
	R_SCURK_WRP_gEndWaitWindow();

	R_BOR_String_Destruct(&titleStr, 2);

	if (convObjectSet.pObjectSet) {
		R_SCURK_WRP_gFreeBlock(convObjectSet.pObjectSet);
		convObjectSet.pObjectSet = 0;
	}
	if (convObjectSet.pObjectSetSize) {
		R_SCURK_WRP_gFreeBlock(convObjectSet.pObjectSetSize);
		convObjectSet.pObjectSetSize = 0;
	}

	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CONVERT)
		ConsoleLog(LOG_DEBUG, "ConvertAndSaveSet(%s, %s, %d): nRes(%d)\n", pLoadFile, pSaveFile, nType, nRes);

	return nRes;
}

void L_SCURK_DirectConvert(winscurkMDIClient *pThis, int nLoad) {
	winscurkApp *pSCApp;
	OPENFILENAMEA *pLoadOfn, *pSaveOfn;
	char szLoadPath[MAX_PATH + 1], szSourcePath[MAX_PATH + 1], szDefSaveExt[3 + 1],
		szMsg[1024 + 1], szFixMsg[128 + 1];
	int *pSaveSucceeded, nRes, nType, nFixCnt;
	DWORD mType;
	cEditableTileSet *pEdTileSet;

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();

	if (!L_SCURK_DirectConvert_WorkingSetCheck(pThis, nLoad))
		return;

	pSaveSucceeded = R_SCURK_WRP_GetgSaveSucceeded();

	memset(szMsg, 0, sizeof(szMsg));
	nType = CONVTYPE_NONE;
	nFixCnt = 0;
	mType = MB_OK;
	pLoadOfn = R_SCURK_WRP_winscurkMDIClient_mGetOpenFileName(pThis);
	pLoadOfn->lpstrFilter = ConvertFileTypeFilterString("Macintosh MIF File (*.mif)|*.mif|DOS TIL File (*.til)|*.til|All Files|*.*|");
	pLoadOfn->lpstrInitialDir = R_SCURK_WRP_winscurkApp_mGetMiffPath(pSCApp);
	if (GetOpenFileNameA(pLoadOfn)) {
		nRes = R_SCURK_WRP_winscurkApp_mGetFileType(pSCApp, pLoadOfn->lpstrFile) - 1;
		if (nRes) {
			// MIF
			if (nRes == 1)
				nType = CONVTYPE_MACMIF;
			else {
				mType = MB_ICONERROR;
				sprintf_s(szMsg, "Not a valid Object Set '%s'", pLoadOfn->lpstrFile);
				R_SCURK_WRP_gScurkMessage_Str(szMsg, 29003, mType);
				return;
			}
		}
		else 
			nType = CONVTYPE_DOSTIL;

		if (nType > CONVTYPE_NONE) {
			strcpy_s(szSourcePath, pLoadOfn->lpstrFile);
			strcpy_s(szLoadPath, pLoadOfn->lpstrFile);
			GetFileDirectory(szLoadPath);
			R_SCURK_WRP_winscurkApp_mSetMiffPath(pSCApp, szLoadPath);

			strcpy_s(szDefSaveExt, "mif");

			pSaveOfn = R_SCURK_WRP_winscurkMDIClient_mGetOpenFileName(pThis);
			pSaveOfn->Flags |= OFN_OVERWRITEPROMPT;
			pSaveOfn->lpstrFilter = ConvertFileTypeFilterString("MIFF File (*.mif)|*.mif||");
			pSaveOfn->lpstrDefExt = szDefSaveExt;
			if (GetSaveFileNameA(pSaveOfn)) {
				R_SCURK_WRP_CheckExtension(pSaveOfn->lpstrFile, szDefSaveExt);

				L_SCURK_DoSelectFixedObjects(pThis);

				R_SCURK_WRP_BeginWaitCursor();
				nRes = L_SCURK_ConvertAndSaveSet(szSourcePath, pSaveOfn->lpstrFile, nType, &nFixCnt);
				R_SCURK_WRP_EndWaitCursor();
				if (nRes > 0) {
					if (nType == CONVTYPE_MACMIF)
						sprintf_s(szMsg, "Macintosh MIF Object Set converted and saved '%s' -> '%s'", szSourcePath, pSaveOfn->lpstrFile);
					else
						sprintf_s(szMsg, "DOS TIL Object Set converted and saved as '%s' -> '%s'", szSourcePath, pSaveOfn->lpstrFile);
					if (nFixCnt > 0) {
						sprintf_s(szFixMsg, "\n\n%d selected 'fixed' objects merged.", nFixCnt);
						strcat_s(szMsg, szFixMsg);
					}
					if (nLoad > CONVSAVEAS_ONLY) {
						if (nLoad == CONVSAVEAS_LOADSRC)
							strcat_s(szMsg, "\n\nLoading as the Source Object Set.");
						else
							strcat_s(szMsg, "\n\nLoading as the Working Object Set.");
					}
					*pSaveSucceeded = 1;
				}
				else {
					// Fail message
					// -1 - Not a Macintosh MIF
					//  0 - Normal fail
					mType = MB_ICONERROR;
					if (nRes < 0) {
						if (nRes == -1)
							sprintf_s(szMsg, "Not a Macintosh MIF Object Set '%s'", szSourcePath);
					}
					else
						sprintf_s(szMsg, "Failed to convert and save Object Set '%s' -> '%s'", szSourcePath, pSaveOfn->lpstrFile);
					*pSaveSucceeded = 0;
				}
				strcpy_s(szLoadPath, pSaveOfn->lpstrFile);
				GetFileDirectory(szLoadPath);
				R_SCURK_WRP_winscurkApp_mSetMiffPath(pSCApp, szLoadPath);
				R_SCURK_WRP_gScurkMessage_Str(szMsg, 29003, mType);

				if (*pSaveSucceeded) {
					if (nLoad > CONVSAVEAS_ONLY) {
						if (nLoad == CONVSAVEAS_LOADSRC)
							pEdTileSet = pSCApp->mSourceTiles;
						else
							pEdTileSet = pSCApp->mWorkingTiles;

						R_SCURK_WRP_winscurkMDIClient_mReadFromMIFFile(pThis, pEdTileSet, pSaveOfn->lpstrFile);

						if (nLoad == CONVSAVEAS_LOADWRK) {
							R_SCURK_WRP_PlaceWindow_DrawHouse(pThis->mPlaceWindow, 0);
							InvalidateRect(pThis->mPlaceWindow->__wndHead.pWnd->HWindow, 0, 0);
						}
						InvalidateRect(pThis->mMoverWindow->__wndHead.pWnd->HWindow, 0, 0);
					}
				}
			}
		}
	}
}
