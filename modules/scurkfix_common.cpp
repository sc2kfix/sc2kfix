// sc2kfix modules/scurkfix_common.cpp: fixes for SCURK that are common between primary and 1996 versions
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

//
// This source file contains common calls that are then used
// in the primary and 1996 English versions of WinSCURK.
//

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>
#include "../resource.h"

UINT mischook_scurk_debug = MISCHOOK_SCURK_DEBUG;

// SCURK-side debug output function

void L_SCURK_gDebugOut(const char *fmt, va_list args) {
	int len;
	char* buf;

	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		ConsoleLog(LOG_DEBUG, "0x%06X -> gDebugOut(): %s", _ReturnAddress(), buf);

		free(buf);
	}
}

// OwlMainCommandLine-specific workaround

static char *L_SCURK_ProcessCmdLine(char *pMainPath, char *pCmdLineParms, BOOL *bValidFileEntry) {
	int iArgc;
	std::string str;
	static char szFileArg[MAX_PATH + 1];

	memset(szFileArg, 0, sizeof(szFileArg));

	str = "\"";
	str += pMainPath;
	str += "\" ";
	str += pCmdLineParms;

	std::wstring wStr(str.begin(), str.end());
	LPWSTR *pArgv;

	pArgv = CommandLineToArgvW(wStr.c_str(), &iArgc);
	if (pArgv) {
		// When a drag-and-drop occurs (over the main program or a shortcut), the file argument
		// is always at the very end; the processing will only accept that detail as well.
		WideCharToMultiByte(CP_UTF8, 0, pArgv[iArgc - 1], -1, szFileArg, MAX_PATH, NULL, NULL);
		_strlwr_s(szFileArg, sizeof(szFileArg) - 1);

		free(pArgv);
	}

	*bValidFileEntry = FALSE;
	if (iArgc > 1) {
		if (strlen(szFileArg) > 0) {
			if (L_IsPathValid(szFileArg))
				*bValidFileEntry = TRUE;
		}
	}

	return szFileArg;
}

char *L_SCURK_OwlMainCommandLineFix(char **pArgs, int nArgs) {
	BC45Xstring *pCmdLineStr;
	BOOL bValidFileEntry;
	char *pRet = NULL;

	pCmdLineStr = L_SCURK_WRP_GetTAppInitCmdLine();

	bValidFileEntry = FALSE;
	if (nArgs >= 2)
		pRet = L_SCURK_ProcessCmdLine(pArgs[0], pCmdLineStr->p->array, &bValidFileEntry);
	if (!bValidFileEntry)
		pRet = NULL;

	_chdir(szGamePath);

	return pRet;
}

// PlaceTileListDlg functions

void L_SCURK_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis) {
	__int16 *wTileObjects;
	char szTileStr[80 + 1];
	int nItem, nMax;
	int nIdx;
	int iCXHScroll, imainRight, imainBottom, ilbCX, ilbCY;
	TBC45XRect mainRect;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_SetupWindow(0x%06X)\n", _ReturnAddress(), pThis);

	strcpy_s(szTileStr, sizeof(szTileStr) - 1, "Tile");
	L_BOR_WRP_Dialog_SetupWindow(pThis);

	iCXHScroll = GetSystemMetrics(SM_CXHSCROLL);

	// First resize the dialogue.
	GetClientRect(pThis->pWnd->HWindow, &mainRect);
	imainRight = pThis->nMaxHitArea + iCXHScroll - mainRect.right;
	imainBottom = pThis->nLBButtonWidth - mainRect.bottom;
	GetWindowRect(pThis->pWnd->HWindow, &mainRect);
	mainRect.right += imainRight + 8;
	mainRect.bottom += imainBottom + 8;
	SetWindowPos(pThis->pWnd->HWindow, HWND_TOP, mainRect.left, mainRect.top, mainRect.right - mainRect.left, mainRect.bottom - mainRect.top, SWP_NOZORDER | SWP_NOMOVE);

	// Then resize the listbox control.
	// If it is done in the wrong order it will fail "hard"
	// on Windows 11 24H2+.
	// Adjust the width and height slightly as well...
	// otherwise it will still fail "hard".
	ilbCX = (mainRect.right - mainRect.left) - 8;
	ilbCY = (mainRect.bottom - mainRect.top) - 8;
	GetClientRect(pThis->pWnd->HWindow, &mainRect);
	SetWindowPos(pThis->pListBox->HWindow, HWND_TOP, mainRect.left + 3, mainRect.top + 3, ilbCX - 4, ilbCY + 2, SWP_NOZORDER);

	L_BOR_WRP_Window_HandleMessage(pThis->pListBox, LB_SETCOLUMNWIDTH, pThis->nMaxHitArea, 0);

	wTileObjects = L_SCURK_WRP_GetwTileObjects();

	nMax = wTileObjects[3 * pThis->mNumTiles] + wTileObjects[3 * pThis->mNumTiles + 1] - 1;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->mNumTiles(%d), nMax(%d), pThis->nTileRow(%d)\n", pThis->mNumTiles, nMax, pThis->nTileRow);
	for (nItem = wTileObjects[3 * pThis->mNumTiles]; nMax > nItem; nItem += pThis->nTileRow) {
		sprintf_s(szTileStr, sizeof(szTileStr) - 1, "Tile%04d%04d", nItem, nItem + pThis->nTileRow - 1);
		nIdx = L_BOR_WRP_ListBox_AddString(pThis->pListBox, szTileStr);
		if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
			ConsoleLog(LOG_DEBUG, "nItem(%d), szTileStr[%s], nIdx(%d)\n", nItem, szTileStr, nIdx);
		L_BOR_WRP_ListBox_SetItemData(pThis->pListBox, nIdx, nItem);
	}
}

void L_SCURK_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis) {
	int nCurSelRowIdx;
	int nPosOne, nPosTwo;
	char szBuf[80 + 1];
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLButtonDblClk(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = L_BOR_WRP_ListBox_GetSelIndex(pThis->pListBox);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	GetCursorPos(&curPt);
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	pThis->nXPos = (curPt.x - lbRect.left) / pThis->nPosWidth;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nXPos(%d)\n", pThis->nXPos);

	L_BOR_WRP_ListBox_GetString(pThis->pListBox, szBuf, nCurSelRowIdx);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d)\n", nPosOne, nPosTwo);
	pThis->nCurPos = pThis->nXPos + nPosOne;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nCurPos(%d)\n", pThis->nCurPos);
}

void L_SCURK_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis) {
	int nCurSelRowIdx;
	int nPosOne, nPosTwo;
	int nValOne, nValTwo;
	__int16 *wTileObjects, *wtoolValue, *wtoolNum;
	char szBuf[80 + 1];
	char *pLongTileName;
	winscurkApp *pSCApp;
	winscurkPlaceWindow *pWindow;
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	pSCApp = L_SCURK_WRP_winscurkApp_GetPointerToClass();

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLBNSelChange(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = L_BOR_WRP_ListBox_GetSelIndex(pThis->pListBox);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	L_BOR_WRP_ListBox_GetString(pThis->pListBox, szBuf, nCurSelRowIdx);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	// These 3 lines have been added since in Windows 11 24H2-onwards
	// it seems as if pThis[18] is not being set correctly.
	// The following code is partially from the EvLButtonDblClk() call.
	GetCursorPos(&curPt);
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	pThis->nChldHndlorX = (curPt.x - lbRect.left);

	nValOne = pThis->nChldHndlorX / pThis->nPosWidth;
	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	nValTwo = nValOne + nPosOne;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d), nValOne(%d), nValTwo(%d)\n", nPosOne, nPosTwo, nValOne, nValTwo);

	wTileObjects = L_SCURK_WRP_GetwTileObjects();
	if (nValTwo >= wTileObjects[3 * pThis->mNumTiles + 1] + wTileObjects[3 * pThis->mNumTiles]) {
		L_SCURK_WRP_winscurkApp_ScurkSound(pSCApp, 3);
		pThis->nSelected = 0;
	}
	else {
		pThis->nXPos = nValOne;
		pThis->nCurPos = nValTwo;
		pThis->nSelected = 1;
		pLongTileName = L_SCURK_WRP_EditableTileSet_GetLongName(pSCApp->mWorkingTiles, pThis->nCurPos);
		L_BOR_WRP_Dialog_SetCaption(pThis, pLongTileName);
		wtoolValue = L_SCURK_WRP_GetwToolValue();
		*wtoolValue = 8;
		wtoolNum = L_SCURK_WRP_GetwToolNum();
		wtoolNum[*wtoolValue] = pThis->nCurPos;
		InvalidateRect(pThis->pWnd->HWindow, 0, 0);
		pWindow = L_SCURK_WRP_winscurkApp_GetPlaceWindow(pSCApp);
		L_SCURK_WRP_winscurkPlaceWindow_ClearCurrentTool(pWindow);
		L_BOR_WRP_Window_SetCursor(pWindow->__wndHead.pWnd, pThis->pWnd->Module, (const char *)30006);
		L_SCURK_WRP_winscurkApp_ScurkSound(pSCApp, 1);
	}
}

// winscurkMDIClient functions

void L_SCURK_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	winscurkApp *pSCApp;
	TBC45XPalette *pPal;
	TBC45XClientDC clDC;
	WORD *wColFastCnt, *wColSlowCnt;
	TBC45XMDIChild *pMDIChild;
	HWND hWndTargetOne, hWndTargetTwo, hWndTargetThree;
	unsigned uFlags;
	BOOL bRedraw, bNoChildren;

	pSCApp = L_SCURK_WRP_winscurkApp_GetPointerToClass();

	bRedraw = FALSE;
	if (!IsIconic(pThis->pWnd->HWindow)) {
		pPal = L_SCURK_WRP_winscurkApp_GetPalette(pSCApp);
		L_BOR_WRP_ClientDC_Construct(&clDC, pThis->pWnd->HWindow);
		L_BOR_WRP_DC_SelectObjectPalette(&clDC, pPal, 0);
		wColFastCnt = L_SCURK_WRP_GetwColFastCnt();
		wColSlowCnt = L_SCURK_WRP_GetwColSlowCnt();
		if (*wColFastCnt == 5) {
			L_SCURK_WRP_winscurkMDIClient_RotateColors(pThis, 1);
			AnimatePalette((HPALETTE)pPal->Handle, 0xAB, 0x31, pThis->mFastColors);
			*wColFastCnt = 0;
			bRedraw = TRUE;
		}
		if (*wColSlowCnt == 30) {
			L_SCURK_WRP_winscurkMDIClient_RotateColors(pThis, 0);
			AnimatePalette((HPALETTE)pPal->Handle, 0xE0, 0x10, pThis->mSlowColors);
			*wColSlowCnt = 0;
			bRedraw = TRUE;
		}
		++*wColFastCnt;
		++*wColSlowCnt;
		L_BOR_WRP_WindowDC_Destruct(&clDC, 0);

		// Only call redraw if the given MDIChild is active, rather than
		// refreshing all windows from pThis->pWnd->HWindow downwards.
		//
		// This reduces "a bit" of the flickering that was otherwise occurring
		// across all windows; at this stage it is only limited to the active
		// MDI Child.
		if (bRedraw) {
			pMDIChild = L_BOR_WRP_MDIClient_GetActiveMDIChild(pThis);
			if (pMDIChild) {
				bNoChildren = FALSE;
				hWndTargetOne = 0;
				hWndTargetTwo = 0;
				hWndTargetThree = 0;
				if (pMDIChild == (TBC45XMDIChild *)pThis->mPlaceWindow) {
					hWndTargetOne = pThis->mPlaceWindow->__wndHead.pWnd->HWindow;
					if (pThis->mPlaceWindow->pPlaceTileListDlg && pThis->mPlaceWindow->pPlaceTileListDlg->pListBox)
						hWndTargetTwo = pThis->mPlaceWindow->pPlaceTileListDlg->pListBox->HWindow;
				}
				else if (pMDIChild == (TBC45XMDIChild *)pThis->mMoverWindow) {
					if (pThis->mMoverWindow->pTileSourceWindow)
						hWndTargetOne = pThis->mMoverWindow->pTileSourceWindow->HWindow;
					if (pThis->mMoverWindow->pTileWorkingWindow)
						hWndTargetTwo = pThis->mMoverWindow->pTileWorkingWindow->HWindow;
				}
				else if (pMDIChild == (TBC45XMDIChild *)pThis->mEditWindow) {
					bNoChildren = TRUE;
					hWndTargetOne = pThis->mEditWindow->__wndHead.pWnd->HWindow;
					if (pThis->mEditWindow->pPaintWindow)
						hWndTargetTwo = pThis->mEditWindow->pPaintWindow->HWindow;
					if (pThis->mEditWindow->pPaletteWindow)
						hWndTargetThree = pThis->mEditWindow->pPaletteWindow->HWindow;
				}

				uFlags = RDW_INVALIDATE;
				if (!bNoChildren)
					uFlags |= RDW_ALLCHILDREN;
				if (hWndTargetOne)
					RedrawWindow(hWndTargetOne, 0, 0, uFlags);
				if (hWndTargetTwo)
					RedrawWindow(hWndTargetTwo, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
				if (hWndTargetThree)
					RedrawWindow(hWndTargetThree, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
			}
		}
	}
}

// cEditableTileSet functions

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
													L_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
												pThis->mTiles[nDBID] = (uint8_t *)L_BOR_WRP_gAllocBlock(dwSize_Shap);
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
												ConsoleLog(LOG_DEBUG, "TILE: Loaded replacement large sprite for: %s\n", szSpriteNames[nSpriteID - SPRITE_LARGE_START]);
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

LONG L_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName) {
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
		pThis->mTileSet = (sprite_archive_t *)L_BOR_WRP_gAllocBlock(10 * pThis->mNumTiles + 2);
	if (!pThis->mTileSet) {
		fclose(f);
		return 0;
	}
	pThis->mTileSet->nSprites = pThis->mNumTiles;
	L_BOR_WRP_gUpdateWaitWindow();
	fread(pThis->mTileSet->pData, 1, 10 * pThis->mNumTiles, f);
	fread(pThis->mTileSizeTable, 1, 4 * pThis->mNumTiles, f);
	L_BOR_WRP_gUpdateWaitWindow();
	pThis->mStartPos = 0;
	nIdx = SPRITE_SMALL_START;
	do {
		if (pThis->mTiles[nIdx])
			L_BOR_WRP_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)L_BOR_WRP_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		if ((nIdx % 100) == 0)
			L_BOR_WRP_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < SPRITE_LARGE_START);
	L_BOR_WRP_gUpdateWaitWindow();
	nIdx = SPRITE_LARGE_START;
	do {
		if (pThis->mTiles[nIdx])
			L_BOR_WRP_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)L_BOR_WRP_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		pThis->mDBIndexFromShapeNum[pThis->mTileSet->pData[nIdx].nSprNum % SPRITE_COUNT] = nIdx;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_MEDIUM_START) % SPRITE_COUNT] = nIdx - SPRITE_MEDIUM_START;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_LARGE_START) % SPRITE_COUNT] = nIdx - SPRITE_LARGE_START;
		if ((nIdx % 100) == 0)
			L_BOR_WRP_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < pThis->mTileSet->nSprites);
	L_BOR_WRP_gUpdateWaitWindow();
	pThis->mTileSet[514].pData[0].sprHeader.wWidth += 4;
	pThis->mTileSet[98].pData[0].nSprNum += 2;
	pThis->mTileSet[543].pData[0].nSprNum += 4;
	pThis->mTileSet[126].pData[0].sprHeader.sprOffset.sprLong += 2;
	fclose(f);
	if (!bDisableFixedTiles) {
		if (nRes) {
			L_SCURK_LoadFixedLargeSpritesRsrc(pThis);
			L_BOR_WRP_gUpdateWaitWindow();
		}
	}
	return nRes;
}

// winscurkMoverWindow functions

TBC45XWindow *L_SCURK_MoverWindow_DisableMaximizeBox(TBC45XWindow *pThis) {
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PLACEANDCOPY) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> DisableMaximizeBox()\n", _ReturnAddress());

	if (GetSystemMetrics(SM_CXSCREEN) > 700)
		pThis->Attr.Style &= ~WS_MAXIMIZEBOX;
	else
		pThis->Attr.Style |= WS_MAXIMIZE;

	return pThis;
}

void L_SCURK_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi) {
	LONG nCXScreen, x, y;

	L_BOR_WRP_Window_DefaultProcessing(pThis->__wndHead.pWnd);
	nCXScreen = GetSystemMetrics(SM_CXSCREEN);
	if (nCXScreen <= 640) {
		x = 512;
		y = 256;
	}
	else {
		x = 640;
		y = 480;
	}

	pMmi->ptMinTrackSize.x = x;
	pMmi->ptMinTrackSize.y = y;

	pMmi->ptMaxPosition.x = 0;
	pMmi->ptMaxPosition.y = 0;
	pMmi->ptMaxSize.x = x;
	pMmi->ptMaxSize.y = y;

	pMmi->ptMaxTrackSize.x = x;
	pMmi->ptMaxTrackSize.y = y;
}

// TDialog functions

void L_SCURK_BCDialog_CmCancel(TBC45XDialog *pThis) {
	winscurkApp *pSCApp;
	winscurkPlaceWindow *pWindow;

	pSCApp = L_SCURK_WRP_winscurkApp_GetPointerToClass();

	// We really don't want to close the Place&Pick object selection
	// dialogue by pressing escape...
	pWindow = L_SCURK_WRP_winscurkApp_GetPlaceWindow(pSCApp);
	if (pWindow && pWindow->pPlaceTileListDlg && pWindow->pPlaceTileListDlg == (TPlaceTileListDlg *)pThis)
		return;

	L_BOR_WRP_Dialog_EvClose(pThis);
}
