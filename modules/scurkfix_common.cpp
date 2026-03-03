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

static BYTE DOSMacPalTable[256];

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

	pCmdLineStr = R_SCURK_WRP_GetTAppInitCmdLine();

	bValidFileEntry = FALSE;
	if (nArgs >= 2)
		pRet = L_SCURK_ProcessCmdLine(pArgs[0], pCmdLineStr->p->array, &bValidFileEntry);
	if (!bValidFileEntry)
		pRet = NULL;

	_chdir(szGamePath);

	return pRet;
}

// PlaceTileListDlg functions

extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis) {
	__int16 *wTileObjects;
	char szTileStr[80 + 1];
	int nItem, nMax;
	int nIdx;
	int iCXHScroll, imainRight, imainBottom, ilbCX, ilbCY;
	TBC45XRect mainRect;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_SetupWindow(0x%06X)\n", _ReturnAddress(), pThis);

	strcpy_s(szTileStr, sizeof(szTileStr) - 1, "Tile");
	R_BOR_WRP_Dialog_SetupWindow(pThis);

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

	R_BOR_WRP_Window_HandleMessage(pThis->pListBox, LB_SETCOLUMNWIDTH, pThis->nMaxHitArea, 0);

	wTileObjects = R_SCURK_WRP_GetwTileObjects();

	nMax = wTileObjects[3 * pThis->mNumTiles] + wTileObjects[3 * pThis->mNumTiles + 1] - 1;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->mNumTiles(%d), nMax(%d), pThis->nTileRow(%d)\n", pThis->mNumTiles, nMax, pThis->nTileRow);
	for (nItem = wTileObjects[3 * pThis->mNumTiles]; nMax > nItem; nItem += pThis->nTileRow) {
		sprintf_s(szTileStr, sizeof(szTileStr) - 1, "Tile%04d%04d", nItem, nItem + pThis->nTileRow - 1);
		nIdx = R_BOR_WRP_ListBox_AddString(pThis->pListBox, szTileStr);
		if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
			ConsoleLog(LOG_DEBUG, "nItem(%d), szTileStr[%s], nIdx(%d)\n", nItem, szTileStr, nIdx);
		R_BOR_WRP_ListBox_SetItemData(pThis->pListBox, nIdx, nItem);
	}
}

extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis) {
	int nCurSelRowIdx;
	int nPosOne, nPosTwo;
	char szBuf[80 + 1];
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLButtonDblClk(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = R_BOR_WRP_ListBox_GetSelIndex(pThis->pListBox);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	GetCursorPos(&curPt);
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	pThis->nXPos = (curPt.x - lbRect.left) / pThis->nPosWidth;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nXPos(%d)\n", pThis->nXPos);

	R_BOR_WRP_ListBox_GetString(pThis->pListBox, szBuf, nCurSelRowIdx);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d)\n", nPosOne, nPosTwo);
	pThis->nCurPos = pThis->nXPos + nPosOne;
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nCurPos(%d)\n", pThis->nCurPos);
}

extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis) {
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

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLBNSelChange(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = R_BOR_WRP_ListBox_GetSelIndex(pThis->pListBox);
	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	R_BOR_WRP_ListBox_GetString(pThis->pListBox, szBuf, nCurSelRowIdx);
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

	wTileObjects = R_SCURK_WRP_GetwTileObjects();
	if (nValTwo >= wTileObjects[3 * pThis->mNumTiles + 1] + wTileObjects[3 * pThis->mNumTiles]) {
		R_SCURK_WRP_winscurkApp_ScurkSound(pSCApp, 3);
		pThis->nSelected = 0;
	}
	else {
		pThis->nXPos = nValOne;
		pThis->nCurPos = nValTwo;
		pThis->nSelected = 1;
		pLongTileName = R_SCURK_WRP_EditableTileSet_GetLongName(pSCApp->mWorkingTiles, pThis->nCurPos);
		R_BOR_WRP_Dialog_SetCaption(pThis, pLongTileName);
		wtoolValue = R_SCURK_WRP_GetwToolValue();
		*wtoolValue = 8;
		wtoolNum = R_SCURK_WRP_GetwToolNum();
		wtoolNum[*wtoolValue] = pThis->nCurPos;
		InvalidateRect(pThis->pWnd->HWindow, 0, 0);
		pWindow = R_SCURK_WRP_winscurkApp_GetPlaceWindow(pSCApp);
		R_SCURK_WRP_PlaceWindow_ClearCurrentTool(pWindow);
		R_BOR_WRP_Window_SetCursor(pWindow->__wndHead.pWnd, pThis->pWnd->Module, (const char *)30006);
		R_SCURK_WRP_winscurkApp_ScurkSound(pSCApp, 1);
	}
}

// TEncodeDib functions

extern "C" void __cdecl Hook_SCURK_EncodeDib_mFillAt(TEncodeDib *pThis, TBC45XPoint *pt) {
	BYTE Bit, *pShapeBuf;
	int nMaxHeight, nBitPos, bNext;
	LONG x, y;
	TBC45XPoint ptPos;

	memset(pThis->mShapeBuf, 0, 0x7FFF);
	nMaxHeight = (pThis->H - 1);
	nBitPos = pt->x + pThis->W * (nMaxHeight - pt->y); // Vertical calculation used to be (pThis->H - pt->y - 2)
	Bit = pThis->Bits[nBitPos];
	pThis->mShapeBuf[pt->x + pThis->W * pt->y] = 1;
	do {
		bNext = 0;
		pShapeBuf = pThis->mShapeBuf;
		for (y = 0; y < pThis->H; ++y) {
			for (x = 0; x < pThis->W; ++x) {
				if (*pShapeBuf == 1) {
					ptPos.x = x;
					ptPos.y = y;
					R_SCURK_WRP_EncodeDib_mFillLine(pThis, &ptPos, Bit);
					bNext = 1;
				}
				++pShapeBuf;
			}
		}
		pShapeBuf = pShapeBuf - 1;
		for (y = pThis->H - 1; y >= 0; --y) {
			for (x = pThis->W - 1; x >= 0; --x) {
				if (*pShapeBuf == 1) {
					ptPos.x = x;
					ptPos.y = y;
					R_SCURK_WRP_EncodeDib_mFillLine(pThis, &ptPos, Bit);
					bNext = 1;
				}
				--pShapeBuf;
			}
		}
	} while (bNext);
}

extern "C" void __cdecl Hook_SCURK_EncodeDib_mFillLine(TEncodeDib *pThis, TBC45XPoint *pt, BYTE Bit) {
	int nMaxHeight, nBitPos;
	BYTE *pBitLine, *pShapeLines, *pShapeLine, *pBit;
	bool bIsValidMinVerticalPos, bIsValidMaxVerticalPos;
	LONG x;

	nMaxHeight = (pThis->H - 1);
	nBitPos = pThis->W * (nMaxHeight - pt->y); // Vertical calculation used to be (pThis->H - (pt->y + 2))
	pBitLine = (BYTE *)&pThis->Bits[nBitPos];
	pShapeLines = &pThis->mShapeBuf[pThis->W * pt->y];
	bIsValidMinVerticalPos = pt->y >= 0; // This was > 0
	bIsValidMaxVerticalPos = pt->y < pThis->H; // This was < pThis->H - 1
	x = pt->x;
	pShapeLine = &pShapeLines[pt->x];
	for (pBit = &pBitLine[pt->x]; x < pThis->W; ++pBit)
	{
		if (Bit != *pBit)
			break;
		*pShapeLine = 2;
		if (bIsValidMinVerticalPos && Bit == pBit[pThis->W] && !pShapeLines[x - pThis->W])
			pShapeLines[x - pThis->W] = 1;
		if (bIsValidMaxVerticalPos && Bit == pBitLine[x - pThis->W] && !pShapeLine[pThis->W])
			pShapeLine[pThis->W] = 1;
		++x;
		++pShapeLine;
	}
	x = pt->x;
	pShapeLine = &pShapeLines[pt->x];
	for (pBit = &pBitLine[pt->x]; x >= 0; --pBit)
	{
		if (Bit != *pBit)
			break;
		*pShapeLine = 2;
		if (bIsValidMinVerticalPos && Bit == pBit[pThis->W] && !pShapeLines[x - pThis->W])
			pShapeLines[x - pThis->W] = 1;
		if (bIsValidMaxVerticalPos && Bit == pBitLine[x - pThis->W] && !pShapeLine[pThis->W])
			pShapeLine[pThis->W] = 1;
		--x;
		--pShapeLine;
	}
}

extern "C" int __cdecl Hook_SCURK_EncodeDib_mDetermineShapeHeight(TEncodeDib *pThis) {
	BYTE *pStartBits, *pEndBits;
	int W, H, nMaxHeight;

	pStartBits = (BYTE *)pThis->Bits;
	H = pThis->H;
	W = pThis->W;
	nMaxHeight = (H - 1); // This was -2
	pEndBits = &pStartBits[W * nMaxHeight];
	if (pStartBits > pEndBits) {
		pThis->mHeight = 0;
		return 0;
	}
	while (*pEndBits == 0xFC) {
		if (W > 0)
			--W;
		else {
			W = pThis->W;
			--H;
		}
		if (pStartBits > --pEndBits) {
			pThis->mHeight = 0;
			return 0;
		}
	}
	pThis->mHeight = H;
	return H;
}

extern "C" void __cdecl Hook_SCURK_EncodeDib_mShrink(TEncodeDib *pThis, TBC45XDib *pInDib, int nScaleBy) {
	int nCurrHeight, nCurrWidth;
	BYTE *pInBits, *pOutNearBits, *pOutFarBits;

	memset(pThis->Bits, 0xFC, 0x8000);
	pInBits = (BYTE *)pInDib->Bits;
	pOutFarBits = (BYTE *)pThis->Bits;
	for (nCurrHeight = 0; nCurrHeight < pThis->H / nScaleBy; ++nCurrHeight) {
		nCurrWidth = 0;
		pOutNearBits = pOutFarBits;
		while (nCurrWidth < 64 / nScaleBy) {
			pOutNearBits[nCurrHeight * pThis->W + 64] = pInBits[nCurrWidth * nScaleBy + 64 + nCurrHeight * nScaleBy * pThis->W];
			pOutFarBits[64 - nCurrWidth + nCurrHeight * pThis->W] = pInBits[64 - nCurrWidth * nScaleBy + nCurrHeight * nScaleBy * pThis->W];
			++nCurrWidth;
			++pOutNearBits;
		}
		// Added to account for the "run-out" that was occurring on the near/far-edges of 4x4 objects.
		// (See the Plymouth Arcology and look at its horizontal edges)
		pOutNearBits[nCurrHeight * pThis->W + 64] = pInBits[nCurrWidth * nScaleBy + 64 + nCurrHeight * nScaleBy * pThis->W];
		pOutFarBits[64 - nCurrWidth + nCurrHeight * pThis->W] = pInBits[64 - nCurrWidth * nScaleBy + nCurrHeight * nScaleBy * pThis->W];
		++pOutNearBits;
	}
}

extern "C" void __cdecl Hook_SCURK_EncodeDib_mEncodeShape(TEncodeDib *pThis, WORD shapeHeight, WORD shapeWidth, WORD nOffSet) {
	BYTE *pThisBits, *pShapeBuf;
	int nHeight, nWidth;
	BYTE pTileRowOffset;
	BYTE *pShapePrevRowBits;
	int nCurrHeight, nCurrWidth;
	BYTE *pStartBit;
	int nCurrWidthSize;
	BYTE *pCurrBit, *pTileBuf;

	// With 'pShapePrevRowBits' this is typically set to
	// 'pShapeBits' when pShapeBits the ChunkMode is
	// MIF_CM_NEWROWSTART; pTileRowOffset is then set to
	// the nCount attribute for pShapePrevRowBits before it
	// is then set to the current pShapeBits.
	// Outside of the for loop when MIF_CM_ENDOFSPRITE is
	// set, then nCount for pShapePrevRowBits is set to 2 (or
	// rather the then current nCount).

	pThisBits = (BYTE *)pThis->Bits;
	nHeight = 0;
	nWidth = pThis->W;
	pShapeBuf = pThis->mShapeBuf;
	SPRITEDATA(pShapeBuf)->nCount = 0;
	SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_NEWROWSTART;
	pShapePrevRowBits = pShapeBuf;
	pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
	pTileRowOffset = 0;
	pShapeBuf = pTileBuf;
	for (nCurrHeight = 0; nCurrHeight < shapeHeight; ++nCurrHeight) {
		// Calculation changed from nWidth * (shapeHeight - nCurrHeight - 1) + nOffSet
		nHeight = (shapeHeight - nCurrHeight) - 1;
		pStartBit = &pThisBits[nOffSet + nWidth * nHeight];
		nCurrWidth = 0;
		while (nCurrWidth < shapeWidth) {
			if (*pStartBit == 0xFC) {
				nCurrWidthSize = 0;
				for (pCurrBit = pStartBit; nCurrWidth + nCurrWidthSize < shapeWidth && *pCurrBit == 0xFC; ++pCurrBit) {
					++nCurrWidthSize;
				}
				nCurrWidth += nCurrWidthSize;
				pStartBit = pCurrBit;
				if (nCurrWidth < shapeWidth) {
					SPRITEDATA(pShapeBuf)->nCount = nCurrWidthSize;
					SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_SKIPPIXELS;
					pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
					pShapeBuf = pTileBuf;
					pTileRowOffset += 2;
				}
			}
			else {
				nCurrWidthSize = 0;
				for (pCurrBit = pStartBit; nCurrWidth + nCurrWidthSize < shapeWidth && *pCurrBit != 0xFC; ++pCurrBit) {
					++nCurrWidthSize;
				}
				SPRITEDATA(pShapeBuf)->nCount = nCurrWidthSize;
				SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_PROCPIXELS;
				pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
				memcpy(pTileBuf, pStartBit, nCurrWidthSize);
				pShapeBuf = &pTileBuf[nCurrWidthSize];
				pTileRowOffset += nCurrWidthSize + 2;
				pStartBit = pCurrBit;
				nCurrWidth += nCurrWidthSize;
				if (!IsEvenUnsigned(nCurrWidthSize)) {
					*pShapeBuf++ = 0;
					++pTileRowOffset;
				}
			}
		}
		SPRITEDATA(pShapePrevRowBits)->nCount = pTileRowOffset;
		// Only encode a new row if nCurrHeight is less than shapeHeight - 1
		// otherwise it'll be the endofsprite marker.
		if (nCurrHeight < shapeHeight - 1) {
			SPRITEDATA(pShapeBuf)->nCount = 0;
			SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_NEWROWSTART;
			pShapePrevRowBits = pShapeBuf;
			pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
			pShapeBuf = pTileBuf;
		}
		pTileRowOffset = 0;
	}
	SPRITEDATA(pShapeBuf)->nCount = 2;
	SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_ENDOFSPRITE;
	pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
	pThis->mLength = pTileBuf - pThis->mShapeBuf;
}

// winscurkMDIClient functions

extern "C" void __cdecl Hook_SCURK_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	winscurkApp *pSCApp;
	TBC45XPalette *pPal;
	TBC45XClientDC clDC;
	WORD *wColFastCnt, *wColSlowCnt;
	TBC45XMDIChild *pMDIChild;
	HWND hWndTargetOne, hWndTargetTwo, hWndTargetThree;
	unsigned uFlags;
	BOOL bRedraw, bNoChildren;

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();

	bRedraw = FALSE;
	if (!IsIconic(pThis->pWnd->HWindow)) {
		pPal = R_SCURK_WRP_winscurkApp_GetPalette(pSCApp);
		R_BOR_WRP_ClientDC_Construct(&clDC, pThis->pWnd->HWindow);
		R_BOR_WRP_DC_SelectObjectPalette(&clDC, pPal, 0);
		wColFastCnt = R_SCURK_WRP_GetwColFastCnt();
		wColSlowCnt = R_SCURK_WRP_GetwColSlowCnt();
		if (*wColFastCnt == 5) {
			R_SCURK_WRP_winscurkMDIClient_RotateColors(pThis, 1);
			AnimatePalette((HPALETTE)pPal->Handle, 0xAB, 0x31, pThis->mFastColors);
			*wColFastCnt = 0;
			bRedraw = TRUE;
		}
		if (*wColSlowCnt == 30) {
			R_SCURK_WRP_winscurkMDIClient_RotateColors(pThis, 0);
			AnimatePalette((HPALETTE)pPal->Handle, 0xE0, 0x10, pThis->mSlowColors);
			*wColSlowCnt = 0;
			bRedraw = TRUE;
		}
		++*wColFastCnt;
		++*wColSlowCnt;
		R_BOR_WRP_WindowDC_Destruct(&clDC, 0);

		// Only call redraw if the given MDIChild is active, rather than
		// refreshing all windows from pThis->pWnd->HWindow downwards.
		//
		// This reduces "a bit" of the flickering that was otherwise occurring
		// across all windows; at this stage it is only limited to the active
		// MDI Child.
		if (bRedraw) {
			pMDIChild = R_BOR_WRP_MDIClient_GetActiveMDIChild(pThis);
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
													R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
												pThis->mTiles[nDBID] = (uint8_t *)R_BOR_WRP_gAllocBlock(dwSize_Shap);
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

extern "C" LONG __cdecl Hook_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName) {
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
		pThis->mTileSet = (sprite_archive_t *)R_BOR_WRP_gAllocBlock(10 * pThis->mNumTiles + 2);
	if (!pThis->mTileSet) {
		fclose(f);
		return 0;
	}
	pThis->mTileSet->nSprites = pThis->mNumTiles;
	R_BOR_WRP_gUpdateWaitWindow();
	fread(pThis->mTileSet->pData, 1, 10 * pThis->mNumTiles, f);
	fread(pThis->mTileSizeTable, 1, 4 * pThis->mNumTiles, f);
	R_BOR_WRP_gUpdateWaitWindow();
	pThis->mStartPos = 0;
	nIdx = SPRITE_SMALL_START;
	do {
		if (pThis->mTiles[nIdx])
			R_BOR_WRP_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)R_BOR_WRP_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		// Offset by 1 to avoid the default set from floating off the base.
		if (pThis->mTileSet->pData[nIdx].nSprNum != SPRITE_SMALL_RESIDENTIAL_3X3_LARGEAPARTMENTS1 &&
			pThis->mTileSet->pData[nIdx].nSprNum != SPRITE_MEDIUM_RESIDENTIAL_3X3_LARGEAPARTMENTS1) {
			if (pThis->mTileSet->pData[nIdx].sprHeader.wHeight > 1)
				pThis->mTileSet->pData[nIdx].sprHeader.wHeight -= 1;
		}
		if ((nIdx % 100) == 0)
			R_BOR_WRP_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < SPRITE_LARGE_START);
	R_BOR_WRP_gUpdateWaitWindow();
	nIdx = SPRITE_LARGE_START;
	do {
		if (pThis->mTiles[nIdx])
			R_BOR_WRP_gFreeBlock(pThis->mTiles[nIdx]);
		pThis->mTiles[nIdx] = (uint8_t *)R_BOR_WRP_gAllocBlock(pThis->mTileSizeTable[nIdx]);
		fread(pThis->mTiles[nIdx], 1, pThis->mTileSizeTable[nIdx], f);
		// Offset by 1 to avoid the default set from floating off the base.
		if (pThis->mTileSet->pData[nIdx].nSprNum != SPRITE_LARGE_RESIDENTIAL_3X3_LARGEAPARTMENTS1) {
			if (pThis->mTileSet->pData[nIdx].sprHeader.wHeight > 1)
				pThis->mTileSet->pData[nIdx].sprHeader.wHeight -= 1;
		}
		pThis->mDBIndexFromShapeNum[pThis->mTileSet->pData[nIdx].nSprNum % SPRITE_COUNT] = nIdx;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_MEDIUM_START) % SPRITE_COUNT] = nIdx - SPRITE_MEDIUM_START;
		pThis->mDBIndexFromShapeNum[(pThis->mTileSet->pData[nIdx].nSprNum - SPRITE_LARGE_START) % SPRITE_COUNT] = nIdx - SPRITE_LARGE_START;
		if ((nIdx % 100) == 0)
			R_BOR_WRP_gUpdateWaitWindow();
		++nIdx;
	} while (nIdx < pThis->mTileSet->nSprites);
	R_BOR_WRP_gUpdateWaitWindow();
	pThis->mTileSet[514].pData[0].sprHeader.wWidth += 4;
	pThis->mTileSet[98].pData[0].nSprNum += 2;
	pThis->mTileSet[543].pData[0].nSprNum += 4;
	pThis->mTileSet[126].pData[0].sprHeader.sprOffset.sprLong += 2;
	fclose(f);
	if (!bDisableFixedTiles) {
		if (nRes) {
			L_SCURK_LoadFixedLargeSpritesRsrc(pThis);
			R_BOR_WRP_gUpdateWaitWindow();
		}
	}
	return nRes;
}

static void L_SCURK_BackupFile(LPCSTR lpPathName) {
	time_t t;
	tm *pTM;
	char szStamp[14 + 1], szFileName[MAX_PATH + 1];

	if (FileExists(lpPathName)) {
		t = time(NULL);
		pTM = localtime(&t);

		sprintf_s(szStamp, "%04d%02d%02d%02d%02d%02d", pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday, pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
		for (int i = 0; i <= 10; ++i) {
			sprintf_s(szFileName, "%s.bak.%s%02d", lpPathName, szStamp, i);
			if (!FileExists(szFileName)) {
				if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_CREATEBAK)
					ConsoleLog(LOG_DEBUG, "File Exists: %s - Creating Backup: %s\n", lpPathName, szFileName);
				CopyFile(lpPathName, szFileName, TRUE);
				break;
			}
		}
	}
}

extern "C" int __cdecl Hook_SCURK_EditableTileSet_mWriteToMIFFFile(cEditableTileSet *pThis, LPCSTR lpPathName) {
	int ret;
	FILE *f;
	DWORD dwBuf, dwContLen;
	WORD wNameChunkCount, wBuf;
	int nEdNum, nShapNum, nDBID, nNameLen;
	BYTE bBuf[4];
	char szHeadName[4];
	char szInfoPortion[114];

	ret = 0;
	// File backup call - but only if the specified
	// 'original' file already exists.
	L_SCURK_BackupFile(lpPathName);
	f = old_fopen(lpPathName, "wb");
	if (f) {
		// Length of file calculation - starting.
		dwBuf = 0x2758;
		dwContLen = 0;
		wNameChunkCount = 0;
		for (nEdNum = 0; nEdNum < MAX_EDNUM; ++nEdNum) {
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum];
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];

			dwContLen += pThis->mTileSizeTable[nDBID - SPRITE_LARGE_START] + pThis->mTileSizeTable[nDBID - SPRITE_MEDIUM_START] + pThis->mTileSizeTable[nDBID];
			if (pThis->mTileIsRenamed[nEdNum]) {
				nNameLen = strlen(pThis->mTileNames[nEdNum]);
				pThis->mTileNames[nNameLen] = 0;
				dwContLen += (nNameLen & 1) + nNameLen + 12;
				++wNameChunkCount;
			}
		}
		dwBuf = _BS_LONG(dwBuf + dwContLen);

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
		strcpy(&szInfoPortion[94], "winSCURK");
		*(DWORD *)szInfoPortion = '_WIN'; // This gets reversed
		fwrite(szInfoPortion, 1, sizeof(szInfoPortion), f);

		// Next part:
		// TILE
		// Length of Content
		// Chunk Count
		memcpy(szHeadName, "TILE", 4);
		fwrite(szHeadName, 1, sizeof(szHeadName), f);
		dwBuf = _BS_LONG_LOC(dwContLen);
		fwrite(&dwBuf, sizeof(dwBuf), 1, f);
		wBuf = _BS_SHORT_CC(wNameChunkCount);
		fwrite(&wBuf, sizeof(wBuf), 1, f);

		// The main body of first NAME and SHAP entries.
		// NAME always comes first
		// SHAP - Large, Small and Tiny for each iteration.
		for (nEdNum = 0; nEdNum < MAX_EDNUM; ++nEdNum) {
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum];
			if (pThis->mTileIsRenamed[nEdNum]) {
				// NAME:
				// Lengh of Content
				// Shape ID (Tiny - default)
				// Name Length
				// String
				memcpy(szHeadName, "NAME", 4);
				fwrite(szHeadName, 1, sizeof(szHeadName), f);
				nNameLen = strlen(pThis->mTileNames[nEdNum]);
				dwBuf = _BS_LONG_NAME_LOC(nNameLen);
				fwrite(&dwBuf, sizeof(dwBuf), 1, f);
				wBuf = _BS_SHORT_SHAP_TINY(nShapNum);
				fwrite(&wBuf, sizeof(wBuf), 1, f);
				wBuf = _BS_SHORT_NAME_LEN(nNameLen);
				fwrite(&wBuf, sizeof(wBuf), 1, f);
				fwrite(pThis->mTileNames[nEdNum], (nNameLen & 1) + nNameLen, 1, f);
			}

			// SHAP
			// Length of Content
			// Shape ID
			// Width
			// Height
			// Length of Content (minus header)
			// Content

			// Large
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			memcpy(szHeadName, "SHAP", 4);
			fwrite(szHeadName, 1, sizeof(szHeadName), f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID] + 10);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			wBuf = _BS_SHORT_SHAP_LARGE(nShapNum);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wWidth);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wHeight);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID]);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			fwrite(pThis->mTiles[nDBID], pThis->mTileSizeTable[nDBID], 1, f);

			// Small
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum] - SPRITE_MEDIUM_START;
			memcpy(szHeadName, "SHAP", 4);
			fwrite(szHeadName, 1, sizeof(szHeadName), f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID] + 10);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			wBuf = _BS_SHORT_SHAP_SMALL(nShapNum);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wWidth);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wHeight);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID]);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			fwrite(pThis->mTiles[nDBID], pThis->mTileSizeTable[nDBID], 1, f);

			// Tiny
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum] - SPRITE_LARGE_START;
			memcpy(szHeadName, "SHAP", 4);
			fwrite(szHeadName, 1, sizeof(szHeadName), f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID] + 10);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			wBuf = _BS_SHORT_SHAP_TINY(nShapNum);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wWidth);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			wBuf = _BS_SHORT(pThis->mTileSet->pData[nDBID].sprHeader.wHeight);
			fwrite(&wBuf, sizeof(wBuf), 1, f);
			dwBuf = _BS_LONG(pThis->mTileSizeTable[nDBID]);
			fwrite(&dwBuf, sizeof(dwBuf), 1, f);
			fwrite(pThis->mTiles[nDBID], pThis->mTileSizeTable[nDBID], 1, f);
		}
		fclose(f);
		ret = 1;
	}

	return ret;
}

static void L_SCURK_TranslateFromMac(cEditableTileSet *pThis, WORD nDBID) {
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, pBit;
	WORD nShapeWidth, nShapeHeight, nCurrWidth;
	BOOL bDone;
	WORD pTileRemainingBitCount;

	if (pThis->mTileSet) {
		nCurrWidth = 0;
		pTileBits = pThis->mTiles[nDBID];
		if (pTileBits) {
			nShapeHeight = pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			nShapeWidth = pThis->mTileSet->pData[nDBID].sprHeader.wWidth;

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
							else
								pBit = DOSMacPalTable[*pTileBits];
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

extern "C" int __cdecl Hook_SCURK_EditableTileSet_mReadFromMIFFFile(cEditableTileSet *pThis, LPCSTR lpPathName) {
	int ret;
	FILE *f;
	tilesetMainHeader_t mainHeader;
	tilesetHeadInfo_t infoHeader;
	char szHeader[4];
	BOOL bMac;
	tilesetTileInfo_t tileHeader;
	tilesetChunkHeader_t chunkHeader;
	DWORD dwSize;
	tilesetShapHeader_t shapHeader;
	WORD nSpriteID, nWidth, nHeight;
	WORD nDBID;
	__int16 nEdNum;
	tilesetNameHeader_t nameHeader;

	// This function now checks for '_MAC' within the informational
	// portion of the header; if it is detected then it will
	// convert the DOS/Macintosh palette indices.

	ret = 0;
	bMac = FALSE;
	f = fopen(lpPathName, "rb");
	if (f) {
		for (int i = 0; i < MAX_EDNUM; ++i) {
			pThis->mTileIsRenamed[i] = 0;
		}

		fread(&mainHeader, sizeof(tilesetMainHeader_t), 1, f);
		if (memcmp(mainHeader.szTypeHead, "MIFF", 4) == 0 &&
			memcmp(mainHeader.szSC2KHead, "SC2K", 4) == 0) {
			fread(&infoHeader, sizeof(tilesetHeadInfo_t), 1, f);
			if (memcmp(infoHeader.szHead, "INFO", 4) == 0) {
				fread(szHeader, 1, 4, f);
				fseek(f, -4, SEEK_CUR);
				if (memcmp(szHeader, "_MAC", 4) == 0)
					bMac = TRUE;

				mainHeader.dwSize = _byteswap_ulong(mainHeader.dwSize);
				infoHeader.dwSize = _byteswap_ulong(infoHeader.dwSize);

				fseek(f, infoHeader.dwSize, SEEK_CUR);

				fread(&tileHeader, sizeof(tilesetTileInfo_t), 1, f);
				if (memcmp(tileHeader.szHead, "TILE", 4) == 0) {
					tileHeader.dwSize = _byteswap_ulong(tileHeader.dwSize);
					tileHeader.nMaxChunks = _byteswap_ushort(tileHeader.nMaxChunks);

					for (WORD nChunk = 0; nChunk < tileHeader.nMaxChunks; ++nChunk) {
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
							// MIF tilesets. // REVISIT FURTHER DOWN THE LINE
							if (bMac) {
								fread(szHeader, 1, 4, f);
								fseek(f, -4, SEEK_CUR);
								if (memcmp(szHeader, "SHAP", 4) == 0 && !chunkHeader.dwSize)
									continue;
							}
							memset(&shapHeader, 0, sizeof(tilesetShapHeader_t));

							fread(&nSpriteID, 2, 1, f);
							fread(&nWidth, 2, 1, f);
							fread(&nHeight, 2, 1, f);
							fread(&dwSize, 4, 1, f);

							shapHeader.nSpriteID = _byteswap_ushort(nSpriteID);
							shapHeader.nWidth = _byteswap_ushort(nWidth);
							shapHeader.nHeight = _byteswap_ushort(nHeight);
							shapHeader.dwSize = _byteswap_ulong(dwSize);

							nDBID = pThis->mDBIndexFromShapeNum[shapHeader.nSpriteID];

							if (pThis->mTiles[nDBID])
								R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
							pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(shapHeader.dwSize);
							pThis->mTileSet->pData[nDBID].sprHeader.wHeight = shapHeader.nHeight;
							pThis->mTileSet->pData[nDBID].sprHeader.wWidth = shapHeader.nWidth;
							pThis->mTileSizeTable[nDBID] = shapHeader.dwSize;

							fread(pThis->mTiles[nDBID], shapHeader.dwSize, 1, f);

							if (bMac)
								L_SCURK_TranslateFromMac(pThis, nDBID);
						}
						else if (memcmp(chunkHeader.szHead, "NAME", 4) == 0) {
							fread(&nameHeader, sizeof(tilesetNameHeader_t), 1, f);
							nameHeader.nShapNum = _byteswap_ushort(nameHeader.nShapNum);
							nameHeader.nNameLength = _byteswap_ushort(nameHeader.nNameLength);

							nEdNum = R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(pThis, nameHeader.nShapNum);

							if (nEdNum != -1) {
								fread(pThis->mTileNames[nEdNum], nameHeader.nNameLength, 1, f);
								pThis->mTileNames[nEdNum][nameHeader.nNameLength] = 0;
								pThis->mTileIsRenamed[nEdNum] = 1;
							}
							else
								fseek(f, nameHeader.nNameLength, SEEK_CUR);
						}
						else
							fseek(f, chunkHeader.dwSize, SEEK_CUR);

						ret = 1;
					}
				}
			}
		}
		fclose(f);
	}
	return ret;
}

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadShapeFromDib_PostBuild(cEditableTileSet *pThis, int nDBID, TEncodeDib *pEncDib) {
	int nDBIDStart;
	int shapeWidth;

	// Originally when pThis->mTileSet->pData[nDBID].sprHeader.wHeight was set to
	// mDetermineShapeHeight() it would increment it by 1; this has now been removed.
	// This case is only encountered during TIL -> MIF conversion, specifically when
	// originally it was downscaling the large tiles to their small and tiny equivalents.
	// NOTE: This now only comes into play if the 'COMPREHENSIVE_DOS_LOAD' define is set
	//       to 0.

	shapeWidth = pThis->mTileSet->pData[nDBID].sprHeader.wWidth;
	pThis->mTileSet->pData[nDBID].sprHeader.wHeight = R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(pEncDib);
	R_SCURK_WRP_EncodeDib_mEncodeShape(pEncDib, pThis->mTileSet->pData[nDBID].sprHeader.wHeight, shapeWidth, 64 - shapeWidth / 2);
	R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
	pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(pEncDib->mLength);
	memcpy(pThis->mTiles[nDBID], pEncDib->mShapeBuf, pEncDib->mLength);
	pThis->mTileSizeTable[nDBID] = pEncDib->mLength;
	for (nDBIDStart = nDBID; nDBIDStart < SPRITE_LARGE_START; nDBIDStart += SPRITE_MEDIUM_START)
		;
	if (nDBID < SPRITE_LARGE_START)
		pThis->mTileSet->pData[nDBID].nSprNum = pThis->mTileSet->pData[nDBIDStart].nSprNum - (nDBIDStart - nDBID);
}

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadShapeFromDib_Paint(cEditableTileSet *pThis, int nEdNum, cPaintWindow *pPaintWnd) {
	int nDBID;

	// Originally when pThis->mTileSet->pData[nDBID].sprHeader.wHeight was set to the
	// encoded dib's height it would increment it by 1; this has now been removed.
	// As a result in both the Dib and Graphic mRenderDbShapeToDIB calls the original
	// (nHeight - 1) calculation is now just nHeight.
	// A number of vertical off-by-one cases have been avoided as a result of 
	// adjustments to the mFill, mFillAt, mDetermineShapeHeight, mFillLine and
	// mEncodeShape calls that:
	// - Remove the need for the previously mentioned '+ 1" case
	// - Avoid positional off-by-one cases that were occurring when the fill tool
	//   was used.

	if (pThis->mTileSet) {
		R_SCURK_WRP_PaintWindow_mEncodeShape(pPaintWnd, 0);
		nDBID = pThis->mDBIndexFromShapeNum[pThis->mShapeNumFromEditableNum[nEdNum % 184]];
		R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
		pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(pPaintWnd->pEncodeDib->mLength);
		memcpy(pThis->mTiles[nDBID], pPaintWnd->pEncodeDib->mShapeBuf, pPaintWnd->pEncodeDib->mLength);
		pThis->mTileSizeTable[nDBID] = pPaintWnd->pEncodeDib->mLength;
		pThis->mTileSet->pData[nDBID].sprHeader.wHeight = LOWORD(pPaintWnd->pEncodeDib->mHeight); // This was + 1

		R_SCURK_WRP_PaintWindow_mEncodeShape(pPaintWnd, 1);
		nDBID = pThis->mDBIndexFromShapeNum[pThis->mShapeNumFromEditableNum[nEdNum % 184]] - 500;
		R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
		pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(pPaintWnd->pEncodeDib->mLength);
		memcpy(pThis->mTiles[nDBID], pPaintWnd->pEncodeDib->mShapeBuf, pPaintWnd->pEncodeDib->mLength);
		pThis->mTileSizeTable[nDBID] = pPaintWnd->pEncodeDib->mLength;
		pThis->mTileSet->pData[nDBID].sprHeader.wHeight = LOWORD(pPaintWnd->pEncodeDib->mHeight); // This was + 1

		R_SCURK_WRP_PaintWindow_mEncodeShape(pPaintWnd, 2);
		nDBID = pThis->mDBIndexFromShapeNum[pThis->mShapeNumFromEditableNum[nEdNum % 184]] - 1000;
		R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
		pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(pPaintWnd->pEncodeDib->mLength);
		memcpy(pThis->mTiles[nDBID], pPaintWnd->pEncodeDib->mShapeBuf, pPaintWnd->pEncodeDib->mLength);
		pThis->mTileSizeTable[nDBID] = pPaintWnd->pEncodeDib->mLength;
		pThis->mTileSet->pData[nDBID].sprHeader.wHeight = LOWORD(pPaintWnd->pEncodeDib->mHeight); // This was + 1
	}
}

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Dib(cEditableTileSet *pThis, TBC45XDib *pDib, int nDBID) {
	int nDibWidth, nDibHeight;
	BYTE *pDibBitsLine, *pDibBitsLinePrev;
	int nDibCurrWidth, nShapeHeight;
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, palBit;
	WORD pTileRemainingBitCount;
	int nWidth, nHeight;
	BOOL bDone;

	if (pThis->mTileSet) {
		nDibWidth = pDib->W;
		nDibHeight = pDib->H;
		pDibBitsLine = (BYTE *)&pDib->Bits[nDibWidth * nDibHeight]; // This was (nDibHeight - 1)
		pDibBitsLinePrev = pDibBitsLine;
		nDibCurrWidth = 0;
		nShapeHeight = nDibHeight;
		pTileBits = pThis->mTiles[nDBID];
		if (pTileBits) {
			nHeight = nDibHeight - pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			nWidth = (nDibWidth - pThis->mTileSet->pData[nDBID].sprHeader.wWidth) >> 1;
			if (nHeight > 0) {
				pDibBitsLine -= nHeight * nDibWidth;
				pDibBitsLinePrev -= nHeight * nDibWidth;
				nShapeHeight = pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			}
			if (nWidth > 0) {
				pDibBitsLine += nWidth;
				pDibBitsLinePrev += nWidth;
			}
			bDone = 0;
			while (!bDone) {
				pTileBitCount = SPRITEDATA(pTileBits)->nCount;
				pTileChunkMode = SPRITEDATA(pTileBits)->nChunkMode;
				pTileBits = (BYTE *)&SPRITEDATA(pTileBits)->pBuf;
				switch (pTileChunkMode) {
				case MIF_CM_EMPTY:
					continue;
				case MIF_CM_NEWROWSTART:
					pDibBitsLinePrev -= nDibWidth;
					pDibBitsLine = pDibBitsLinePrev;
					nDibCurrWidth = 0;
					bDone = nShapeHeight == 0;
					--nShapeHeight; // Decrement used to be part of the above check;
					break;
				case MIF_CM_ENDOFSPRITE:
					bDone = 1;
					break;
				case MIF_CM_SKIPPIXELS:
					pDibBitsLine += pTileBitCount;
					nDibCurrWidth += pTileBitCount;
					break;
				case MIF_CM_PROCPIXELS:
					for (pTileRemainingBitCount = pTileBitCount; pTileRemainingBitCount; ++nDibCurrWidth) {
						--pTileRemainingBitCount;
						// *pTileBits here in this case is nPixelIndex (colour lookup palette index)
						//
						// 'if (nDibWidth < nDibCurrWidth)' removed
						// to avoid certain columns of pixels being
						// missed during palette processing.
						{
							if (*pTileBits == 0xFC)
								palBit = 0x61;
							else
								palBit = *pTileBits;
							*pDibBitsLine++ = palBit;
						}
						++pTileBits;
					}
					if (!IsEvenUnsigned(pTileBitCount)) {
						++pTileBits;
						++nDibCurrWidth;
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

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Graphic(cEditableTileSet *pThis, CWinGBitmap *pGraphic, int nDBID) {
	int nGraphicWidth, nGraphicHeight;
	BYTE *pGraphicBitsLine, *pGraphicBitsLinePrev;
	int nGraphicCurrWidth, nShapeHeight;
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, palBit;
	WORD pTileRemainingBitCount;
	int nWidth, nHeight;
	BOOL bDone;

	if (pThis->mTileSet) {
		nGraphicWidth = R_SCURK_WRP_WinGBitmap_Width(pGraphic);
		nGraphicHeight = R_SCURK_WRP_WinGBitmap_Height(pGraphic);
		pGraphicBitsLine = &pGraphic->GRpBits[nGraphicWidth * nGraphicHeight]; // This used to be (nGraphicHeight - 1)
		pGraphicBitsLinePrev = pGraphicBitsLine;
		nGraphicCurrWidth = 0;
		nShapeHeight = nGraphicHeight;
		pTileBits = pThis->mTiles[nDBID];
		if (pTileBits) {
			nHeight = nGraphicHeight - pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			nWidth = (nGraphicWidth - pThis->mTileSet->pData[nDBID].sprHeader.wWidth) >> 1;
			if (nHeight > 0) {
				pGraphicBitsLine -= nHeight * nGraphicWidth;
				pGraphicBitsLinePrev -= nHeight * nGraphicWidth;
				nShapeHeight = pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			}
			if (nWidth > 0) {
				pGraphicBitsLine += nWidth;
				pGraphicBitsLinePrev += nWidth;
			}
			bDone = 0;
			while (!bDone) {
				pTileBitCount = SPRITEDATA(pTileBits)->nCount;
				pTileChunkMode = SPRITEDATA(pTileBits)->nChunkMode;
				pTileBits = (BYTE *)&SPRITEDATA(pTileBits)->pBuf;
				switch (pTileChunkMode) {
				case MIF_CM_EMPTY:
					continue;
				case MIF_CM_NEWROWSTART:
					pGraphicBitsLinePrev -= nGraphicWidth;
					pGraphicBitsLine = pGraphicBitsLinePrev;
					nGraphicCurrWidth = 0;
					bDone = nShapeHeight == 0;
					--nShapeHeight; // Decrement used to be part of the above check;
					break;
				case MIF_CM_ENDOFSPRITE:
					bDone = 1;
					break;
				case MIF_CM_SKIPPIXELS:
					pGraphicBitsLine += pTileBitCount;
					nGraphicCurrWidth += pTileBitCount;
					break;
				case MIF_CM_PROCPIXELS:
					for (pTileRemainingBitCount = pTileBitCount; pTileRemainingBitCount; ++nGraphicCurrWidth) {
						--pTileRemainingBitCount;
						// *pTileBits here in this case is nPixelIndex (colour lookup palette index)
						//
						// 'if (nGraphicCurrWidth < nGraphicWidth)' removed
						// to avoid certain columns of pixels being
						// missed during palette processing.
						{
							if (*pTileBits == 0xFC)
								palBit = 0x61;
							else
								palBit = *pTileBits;
							*pGraphicBitsLine++ = palBit;
						}
						++pTileBits;
					}
					if (!IsEvenUnsigned(pTileBitCount)) {
						++pTileBits;
						++nGraphicCurrWidth;
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

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderShapeToTile(cEditableTileSet *pThis, TBC45XDib *pDib, int nEdNum) {
	int nDibWidth, nDibHeight;
	TBC45XDib *mTileBack;
	BYTE *pDibBitsLine, *pDibBitsLinePrev;
	int nDibCurrWidth, nShapeHeight, nDBID;
	BYTE *pTileBits, pTileBitCount, pTileChunkMode, palBit;
	WORD pTileRemainingBitCount;
	int nWidth, nHeight;
	BOOL bDone;

	if (pThis->mTileSet) {
		nDibWidth = pDib->W;
		nDibHeight = pDib->H;
		mTileBack = R_SCURK_WRP_mTileBack();
		memcpy(pDib->Bits, mTileBack->Bits, 0x1000);
		pDibBitsLine = (BYTE *)&pDib->Bits[nDibWidth * nDibHeight]; // This was (nDibHeight - 1)
		pDibBitsLinePrev = pDibBitsLine;
		nDibCurrWidth = 0;
		nShapeHeight = nDibHeight;
		nDBID = pThis->mDBIndexFromShapeNum[pThis->mShapeNumFromEditableNum[nEdNum % MAX_EDNUM]] - SPRITE_MEDIUM_START;
		pTileBits = pThis->mTiles[nDBID];
		if (pTileBits) {
			nHeight = nDibHeight - pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			nWidth = (nDibWidth - pThis->mTileSet->pData[nDBID].sprHeader.wWidth) / 2;
			if (nHeight > 0) {
				pDibBitsLine -= nHeight * nDibWidth;
				pDibBitsLinePrev -= nHeight * nDibWidth;
				nShapeHeight = pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
			}
			else {
				// In the Tile buttons this is to account for objects
				// that have a height taller than the visible area,
				// in order to keep it vertically aligned with the
				// base.
				for (; nHeight < 0; ++nHeight)
					pTileBits += SPRITEDATA(pTileBits)->nCount + 2;
			}
			if (nWidth > 0) {
				pDibBitsLine += nWidth;
				pDibBitsLinePrev += nWidth;
			}
			bDone = 0;
			while (!bDone) {
				pTileBitCount = SPRITEDATA(pTileBits)->nCount;
				pTileChunkMode = SPRITEDATA(pTileBits)->nChunkMode;
				pTileBits = (BYTE *)&SPRITEDATA(pTileBits)->pBuf;
				switch (pTileChunkMode) {
				case MIF_CM_EMPTY:
					continue;
				case MIF_CM_NEWROWSTART:
					pDibBitsLinePrev -= nDibWidth;
					pDibBitsLine = pDibBitsLinePrev;
					nDibCurrWidth = 0;
					bDone = nShapeHeight == 0;
					--nShapeHeight; // Decrement used to be part of the above check;
					break;
				case MIF_CM_ENDOFSPRITE:
					bDone = 1;
					break;
				case MIF_CM_SKIPPIXELS:
					pDibBitsLine += pTileBitCount;
					nDibCurrWidth += pTileBitCount;
					break;
				case MIF_CM_PROCPIXELS:
					for (pTileRemainingBitCount = pTileBitCount; pTileRemainingBitCount; ++nDibCurrWidth) {
						--pTileRemainingBitCount;
						// *pTileBits here in this case is nPixelIndex (colour lookup palette index)
						//
						// 'if (nDibWidth < nDibCurrWidth)' removed
						// to avoid certain columns of pixels being
						// missed during palette processing.
						{
							if (*pTileBits == 0xFC)
								palBit = 0x61;
							else
								palBit = *pTileBits;
							*pDibBitsLine++ = palBit;
						}
						++pTileBits;
					}
					if (!IsEvenUnsigned(pTileBitCount)) {
						++pTileBits;
						++nDibCurrWidth;
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

static void L_SCURK_TranslateFromDOS(cEditableTileSet *pThis, WORD nDBID, BYTE *pDOSTileBuf) {
	BYTE *pDOSTileBits, *pTileBitsBuf, *pTileBits;
	int nTileSize;
	BOOL bDone;
	BYTE pDOSTileChunkMode, pDOSTileBitCount;
	WORD pDOSTileRemainingBitCount;

	if (pThis->mTiles[nDBID])
		R_BOR_WRP_gFreeBlock(pThis->mTiles[nDBID]);
	pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(0xFFFF);
	pDOSTileBits = pDOSTileBuf;
	pTileBitsBuf = pThis->mTiles[nDBID];
	nTileSize = 0;
	pTileBits = 0;
	if (SPRITEDOSDATA(pDOSTileBuf)->nChunkMode != TIL_CM_NEWROWSTART) {
		SPRITEDATA(pTileBitsBuf)->nCount = 0;
		SPRITEDATA(pTileBitsBuf)->nChunkMode = MIF_CM_NEWROWSTART;
		pTileBitsBuf = (BYTE *)&SPRITEDATA(pTileBitsBuf)->pBuf;
		pTileBits = pTileBitsBuf;
		nTileSize = 2;
		++pThis->mTileSet->pData[nDBID].sprHeader.wHeight;
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
	pThis->mTileSizeTable[nDBID] = nTileSize;
	pThis->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gResizeBlock(pThis->mTiles[nDBID], nTileSize);
}

extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadFromDOSFile(cEditableTileSet *pThis, LPCSTR lpPathName) {
	FILE *f;
	BYTE *lpBuffer;
	tilMainStruct_t Buffer;
	tilHeader_t *lpShapeBuf;
#if COMPREHENSIVE_DOS_LOAD
	tilHeader_t *lpOtherShapeBuf;
	DWORD dwOtherSize, dwOtherOffset;
	int nSkipNum;
#endif
	DWORD dwSize, dwOffset;
	WORD nEdNum, nShapNum, nDBID;

	f = old_fopen(lpPathName, "rb");
	if (f) {
		lpBuffer = (BYTE *)R_BOR_WRP_gAllocBlock(0xFFFF);
		fread(&Buffer, 1, 0x80, f);
		lpShapeBuf = (tilHeader_t *)R_BOR_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwLargeOffset, SEEK_SET);
		fread(lpShapeBuf, 1, 0x2EE0, f);
		dwSize = Buffer.dwLargeSize;
		for (nEdNum = 0; nEdNum < MAX_EDNUM; ++nEdNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum];
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpShapeBuf[nShapNum].height;
			pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpShapeBuf[nShapNum].width;
			dwOffset = lpShapeBuf[nShapNum].dwOffset;
			fseek(f, dwSize + dwOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}
#if !COMPREHENSIVE_DOS_LOAD
		R_SCURK_WRP_EditableTileSet_mBuildSmallMedTiles(pThis);
#endif
		R_BOR_WRP_gFreeBlock(lpShapeBuf);

#if COMPREHENSIVE_DOS_LOAD
		lpOtherShapeBuf = (tilHeader_t *)R_BOR_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwOtherOffset, SEEK_SET);
		fread(lpOtherShapeBuf, 1, 0x2EE0, f);

		lpShapeBuf = (tilHeader_t *)R_BOR_WRP_gAllocBlock(0x2EE0);
		fseek(f, Buffer.dwSmallOffset, SEEK_SET);
		fread(lpShapeBuf, 1, 0x2EE0, f);
		dwSize = Buffer.dwSmallSize;
		dwOffset = 0;
		// Main Small processing.
		for (nEdNum = 0; nEdNum < MAX_EDNUM; ++nEdNum) {
			if ((nEdNum >= 70 && nEdNum <= 74) ||
				(nEdNum >= 96 && nEdNum <= 105))
				continue;
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum] - SPRITE_MEDIUM_START;
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpShapeBuf[nShapNum].height;
			pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpShapeBuf[nShapNum].width;
			dwOffset = lpShapeBuf[nShapNum].dwOffset;
			fseek(f, dwSize + dwOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}

		// When processing the "small" tiles
		// some are present within the OTHER.DAT
		// rather than SMALL.DAT, and the offsets
		// vary. This was determined based on asset
		// ordering within the Windows Sprite DAT archives
		// (rather than their stored ID).

		dwOtherSize = Buffer.dwOtherSize;
		dwOtherOffset = 0;
		// Initial skip from other #0
		for (nSkipNum = 0; nSkipNum < 42; ++nSkipNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			dwOtherOffset = lpOtherShapeBuf[nSkipNum].dwOffset;
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
		}

		// Small from other #1 - Arcologies and Braun
		for (nEdNum = 70; nEdNum <= 74; ++nEdNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum] - SPRITE_MEDIUM_START;
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			if (nEdNum >= 70 && nEdNum <= 74) {
				pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpOtherShapeBuf[nShapNum].height;
				pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpOtherShapeBuf[nShapNum].width;
			}
			dwOtherOffset = lpOtherShapeBuf[nShapNum].dwOffset;
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}

		// Small from other #2 - Hydro and Wind
		for (nEdNum = 96; nEdNum <= 98; ++nEdNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum] - SPRITE_MEDIUM_START;
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpOtherShapeBuf[nShapNum].height;
			pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpOtherShapeBuf[nShapNum].width;
			dwOtherOffset = lpOtherShapeBuf[nShapNum].dwOffset;
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}

		// Small from other #3 - Gas to Coal
		dwOtherOffset = 0;
		for (nEdNum = 99; nEdNum <= 105; ++nEdNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum] - SPRITE_MEDIUM_START;
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpOtherShapeBuf[nShapNum].height;
			pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpOtherShapeBuf[nShapNum].width;
			dwOtherOffset = lpOtherShapeBuf[nShapNum].dwOffset;
			fseek(f, dwOtherSize + dwOtherOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}

		// Tiny Processing
		for (nEdNum = 0; nEdNum < MAX_EDNUM; ++nEdNum) {
			R_BOR_WRP_gUpdateWaitWindow();
			nShapNum = pThis->mShapeNumFromEditableNum[nEdNum] - SPRITE_LARGE_START;
			nDBID = pThis->mDBIndexFromShapeNum[nShapNum];
			pThis->mTileSet->pData[nDBID].sprHeader.wHeight = lpShapeBuf[nShapNum].height;
			pThis->mTileSet->pData[nDBID].sprHeader.wWidth = lpShapeBuf[nShapNum].width;
			dwOffset = lpShapeBuf[nShapNum].dwOffset;
			fseek(f, dwSize + dwOffset, SEEK_SET);
			fread(lpBuffer, 1, 0xFFFF, f);
			L_SCURK_TranslateFromDOS(pThis, nDBID, lpBuffer);
		}
		R_BOR_WRP_gFreeBlock(lpShapeBuf);

		R_BOR_WRP_gFreeBlock(lpOtherShapeBuf);
#endif

		R_BOR_WRP_gFreeBlock(lpBuffer);
		fclose(f);
	}
}

void L_SCURK_InitDOSMacPaletteIdxTable() {
	int i;

	for (i = 0; i < 256; ++i) {
		if (i >= 0 && i < 204)
			DOSMacPalTable[i] = i + 16;
		else if (i >= 224 && i < 239)
			DOSMacPalTable[i] = i;
		else
			DOSMacPalTable[i] = 0;
	}

	// This fixes certain colours being
	// lost during the DOS/Mac -> Windows
	// translation process.
	// It must be noted that these cases
	// were manually determined during
	// the comprehensive examination of
	// certain tiles and their prior
	// pre-conversion palette indices.
	// // CHECK AS NEEDED - these values
	//    aren't fatal regardless, previously
	//    they were just being set to 0x00 - 
	//    which caused a noticeable negative
	//    effect on certain tiles (see the
	//    Marina, Seaport Warehouse,
	//    Seaport Loading Bar, Army Hangar,
	//    Military Control Tower)
	DOSMacPalTable[211] = 0x22;
	DOSMacPalTable[212] = 0x7E;
	DOSMacPalTable[213] = 0x7C;
	DOSMacPalTable[222] = 0x79;

	ConsoleLog(LOG_INFO, "Initialize DOS/Mac -> Windows Palette Index Table.\n");

	//for (int i = 0; i < 256; ++i) {
	//	ConsoleLog(LOG_DEBUG, "0x%02X (%d) (%u, %u) | 0x%02X (%d) (%u, %u) -> 0x%02X (%u) (%u, %u)\n", i, i, HIGHNIBBLE(i), LOWNIBBLE(i), PAL_IDX(i), PAL_IDX(i), HIGHNIBBLE(PAL_IDX(i)), LOWNIBBLE(PAL_IDX(i)), DOSMacPalTable[i], DOSMacPalTable[i], HIGHNIBBLE(DOSMacPalTable[i]), LOWNIBBLE(DOSMacPalTable[i]));
	//}
}

// cPaintWindow functions

extern "C" void __cdecl Hook_SCURK_PaintWindow_mFill(cPaintWindow *pThis, TBC45XPoint *pPoint) {
	TBC45XPoint pt;
	DWORD dwBitPoint;
	LONG H, W;
	BYTE backColor, foreColor;
	TBC45XRect rect;

	R_SCURK_WRP_PaintWindow_mScreenToDib(&pt, pThis, pPoint);
	R_SCURK_WRP_EncodeDib_mFillAt(pThis->pEncodeDib, &pt);
	pThis->pBits = (BYTE *)R_SCURK_WRP_EditWindow_mGetForegroundPattern(pThis->pScurkEditParent)->Bits;
	dwBitPoint = 0;
	// This used to be H = 1, now adjusted since the original
	// issue concerning the top pixel row going unprocessed now
	// being corrected.
	for (H = 0; H < pThis->pEncodeDib->H; ++H) {
		for (W = 0; W < pThis->pEncodeDib->W; ++W) {
			if (pThis->pEncodeDib->mShapeBuf[dwBitPoint]) {
				backColor = R_SCURK_WRP_EditWindow_mGetBackgroundColor(pThis->pScurkEditParent);
				foreColor = R_SCURK_WRP_EditWindow_mGetForegroundColor(pThis->pScurkEditParent);
				rect.bottom = H;
				rect.right = W;
				rect.left = W;
				rect.top = H;
				R_SCURK_WRP_PaintWindow_mPutPixel(pThis, &rect, foreColor, backColor);
			}
			++dwBitPoint;
		}
	}
	R_SCURK_WRP_PaintWindow_mApplyTileToScreen(pThis);
	R_SCURK_WRP_PaintWindow_mClipDrawing(pThis);
	InvalidateRect(pThis->HWindow, 0, TRUE);
}

extern "C" void __cdecl Hook_SCURK_PaintWindow_mClipDrawing(cPaintWindow *pThis) {
	int32_t lWidth, lMaxWidth, lWidthPos;
	int32_t lHeight, lMaxHeight;
	uint16_t nNearBit, nFarBit;
	BYTE *lSrcBits, *lDestBits1, *lDestBits2;
	BYTE *pEncodedBit, *pGraphicBit, *pDibBit;
	BYTE *pCurrGraphicBit, *pCurrEncodedBit, *pCurrWidthBit;

	if (pThis->dwTouching) {
		pThis->dwTouching = 0;
		lMaxWidth = 64 - (R_SCURK_WRP_EditWindow_mGetShapeWidth(pThis->pScurkEditParent) >> 1);
		lWidth = 63;
		lSrcBits = (BYTE *)pThis->pDibEmptyTile->Bits;
		lDestBits1 = pThis->pGraphic->GRpBits;
		lDestBits2 = (BYTE *)pThis->pEncodeDib->Bits;
		lMaxHeight = 32; // 4x4 objects, avoids any clipping beyond the bounds of the tilebase.
		if (R_SCURK_WRP_EditWindow_mGetShapeWidth(pThis->pScurkEditParent) != 128)
			lMaxHeight = 256;
		for (lHeight = 0; lHeight < lMaxHeight; ++lHeight) {
			nNearBit = (lHeight << 7); // Up to 32640
			nFarBit = nNearBit + 127;  // Up to 32767
			lWidthPos = 0;
			pEncodedBit = &lDestBits2[nFarBit];
			pGraphicBit = &lDestBits1[nFarBit];
			pDibBit = &lSrcBits[nFarBit];
			pCurrEncodedBit = &lDestBits2[nNearBit];
			pCurrGraphicBit = &lDestBits1[nNearBit];
			for (pCurrWidthBit = &lSrcBits[nNearBit]; lWidthPos < lWidth; ++lWidthPos) {
				*pCurrGraphicBit = *pCurrWidthBit;
				*pCurrEncodedBit = 0xFC;
				*pGraphicBit = *pDibBit;
				*pEncodedBit = 0xFC;
				++pCurrEncodedBit;
				++pCurrGraphicBit;
				++pCurrWidthBit;
				--nFarBit;
				--pEncodedBit;
				--pGraphicBit;
				--pDibBit;
			}
			// Only account for this one at the widest position so it falls
			// within the absolute clipping bounds.
			if (lWidth == lMaxWidth - 1) {
				*pCurrGraphicBit = lSrcBits[nFarBit];
				*pCurrEncodedBit = 0xFC;
				*pGraphicBit = lSrcBits[nFarBit];
				*pEncodedBit = 0xFC;
			}
			if (lWidth > lMaxWidth)
				lWidth -= 2;
		}
		R_SCURK_WRP_PaintWindow_mDraw(pThis);
		InvalidateRect(pThis->pScurkEditParent->__wndHead.pWnd->HWindow, 0, 0);
	}
}

extern "C" void __cdecl Hook_SCURK_PaintWindow_mEncodeShape(cPaintWindow *pThis, int nZoomLevel) {
	TEncodeDib *pEncDib;
	int nScaleFactor, nShiftVal;
	int nXOffset;
	WORD shapeWidth, shapeHeight;

	pEncDib = (TEncodeDib *)R_BOR_Op_New(sizeof(TEncodeDib));
	if (!pEncDib) {
		ConsoleLog(LOG_DEBUG, "0x%06X -> cPaintWindow::mEncodeShape(%d): !pEncDib allocation has failed.\n", _ReturnAddress(), nZoomLevel);
		return;
	}

	pEncDib = R_SCURK_WRP_EncodeDib_Construct_Dimens(pEncDib, 128, 256, 0x100, DIB_PAL_COLORS);

	nScaleFactor = (nZoomLevel) ? nZoomLevel * 2 : 1;
	nShiftVal = (nZoomLevel) ? nZoomLevel + 1 : 1;
	nXOffset = 64 - ((int)(WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pThis->pScurkEditParent) >> nShiftVal);
	R_SCURK_WRP_EncodeDib_mShrink(pEncDib, pThis->pEncodeDib, nScaleFactor);
	shapeWidth = (WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pThis->pScurkEditParent) / nScaleFactor;
	shapeHeight = R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(pEncDib);
	R_SCURK_WRP_EncodeDib_mEncodeShape(pEncDib, shapeHeight, shapeWidth, nXOffset);
	R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(pThis->pEncodeDib, pEncDib);
	if (pEncDib)
		R_SCURK_WRP_EncodeDib_Destruct(pEncDib, 3);
}

// winscurkMDIFrame functions

extern "C" int __cdecl Hook_SCURK_winscurkMDIFrame_AssignMenu(winscurkMDIFrame *pThis, TBC45XResId menuResID) {
	winscurkApp *pSCApp;
	HMENU hDefaultMenu, hMenu;

	if (pThis->__wndHead.pWnd->Attr.Menu.Id != menuResID.Id) {
		if (HIWORD(pThis->__wndHead.pWnd->Attr.Menu.Id))
			free((void*)pThis->__wndHead.pWnd->Attr.Menu.Id);

		pThis->__wndHead.pWnd->Attr.Menu.Id = (HIWORD(menuResID.Id)) ? (const char *)R_BOR_strnewdup((char *)menuResID.Id, 0) : menuResID.Id;
	}
	if (!pThis->__wndHead.pWnd->HWindow)
		return 1;
	hDefaultMenu = GetMenu(pThis->__wndHead.pWnd->HWindow);
	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	if (pSCApp->mLocalModule)
		pSCApp = (winscurkApp *)pSCApp->mLocalModule;
	// When it comes to altering the menu
	// it must be done here as a result
	// of the menu being loaded and destroyed
	// each time the MDI Child Window
	// is switched.
	hMenu = LoadMenuA(pSCApp->HInstance, pThis->__wndHead.pWnd->Attr.Menu.Id);
	if (hMenu && pThis->__wndHead.pWnd->Attr.Menu.Id == (const char *)400) {
		if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "0x%06X -> winscurkMDIFrame::AssignMenu(%u): - EditWindow Menu\n", _ReturnAddress(), (DWORD)menuResID.Id);
		HMENU hEditPopup;
		MENUITEMINFO miiEditPopup;
		miiEditPopup.cbSize = sizeof(MENUITEMINFO);
		miiEditPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hMenu, 1, TRUE, &miiEditPopup) && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		hEditPopup = miiEditPopup.hSubMenu;
		if (!InsertMenu(hEditPopup, 9, MF_BYPOSITION|MF_STRING, IDM_SCRK_EW_EDIT_MOVE_RIGHT, "Move Object Right") && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit InsertMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hEditPopup, 9, MF_BYPOSITION|MF_STRING, IDM_SCRK_EW_EDIT_MOVE_LEFT, "Move Object Left") && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit InsertMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hEditPopup, 9, MF_BYPOSITION|MF_STRING, IDM_SCRK_EW_EDIT_MOVE_DOWN, "Move Object Down") && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit InsertMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hEditPopup, 9, MF_BYPOSITION|MF_STRING, IDM_SCRK_EW_EDIT_MOVE_UP, "Move Object Up") && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit InsertMenuA #4 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}
		if (!InsertMenu(hEditPopup, 9, MF_BYPOSITION|MF_SEPARATOR, NULL, NULL) && mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: Edit InsertMenuA #5 failed, error = 0x%08X.\n", GetLastError());
			goto skipmainmenu;
		}

		if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated EditWindow edit menu.\n");
	}
skipmainmenu:
	if (!R_BOR_MDIFrame_SetMenu((TBC45XMDIFrame *)pThis, hMenu))
		return 0;
	if (hDefaultMenu)
		DestroyMenu(hDefaultMenu);
	return 1;
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

extern "C" void __cdecl Hook_SCURK_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi) {
	LONG nCXScreen, x, y;

	R_BOR_WRP_Window_DefaultProcessing(pThis->__wndHead.pWnd);
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

// TCommandEnabler functions

extern "C" void __cdecl Hook_SCURK_CommandEnabler_Enable(TBC45XCommandEnabler *pThis) {
	pThis->Handled = 1;

	// Added here to allow for the new menu
	// items to be enabled.
	winscurkApp *pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	winscurkMDIClient *pclWnd = pSCApp->mdiClient;
	if (pclWnd) {
		EnableMenuItem(GetMenu(pclWnd->pWnd->Parent->HWindow), IDM_SCRK_EW_EDIT_MOVE_UP, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(GetMenu(pclWnd->pWnd->Parent->HWindow), IDM_SCRK_EW_EDIT_MOVE_DOWN, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(GetMenu(pclWnd->pWnd->Parent->HWindow), IDM_SCRK_EW_EDIT_MOVE_LEFT, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(GetMenu(pclWnd->pWnd->Parent->HWindow), IDM_SCRK_EW_EDIT_MOVE_RIGHT, MF_BYCOMMAND | MF_ENABLED);
	}
}

// TDialog functions

extern "C" void __cdecl Hook_SCURK_BCDialog_CmCancel(TBC45XDialog *pThis) {
	winscurkApp *pSCApp;
	winscurkPlaceWindow *pWindow;

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();

	// We really don't want to close the Place&Pick object selection
	// dialogue by pressing escape...
	pWindow = R_SCURK_WRP_winscurkApp_GetPlaceWindow(pSCApp);
	if (pWindow && pWindow->pPlaceTileListDlg && pWindow->pPlaceTileListDlg == (TPlaceTileListDlg *)pThis)
		return;

	R_BOR_WRP_Dialog_EvClose(pThis);
}

// TFrameWindow functions

static void  L_SCURK_EncodeWithShunt(TEncodeDib *pThis, WORD shapeHeight, WORD shapeWidth, WORD nOffSet, int nDir) {
	BYTE *pThisBits, *pShapeBuf;
	int nHeight, nWidth;
	BYTE pTileRowOffset;
	BYTE *pShapePrevRowBits;
	int nCurrHeight, nCurrWidth;
	BYTE *pStartBit;
	int nCurrWidthSize;
	BYTE *pCurrBit, *pTileBuf;

	// With 'pShapePrevRowBits' this is typically set to
	// 'pShapeBits' when pShapeBits the ChunkMode is
	// MIF_CM_NEWROWSTART; pTileRowOffset is then set to
	// the nCount attribute for pShapePrevRowBits before it
	// is then set to the current pShapeBits.
	// Outside of the for loop when MIF_CM_ENDOFSPRITE is
	// set, then nCount for pShapePrevRowBits is set to 2 (or
	// rather the then current nCount).

	pThisBits = (BYTE *)pThis->Bits;
	nWidth = pThis->W;
	pShapeBuf = pThis->mShapeBuf;
	SPRITEDATA(pShapeBuf)->nCount = 0;
	SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_NEWROWSTART;
	pShapePrevRowBits = pShapeBuf;
	pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
	pTileRowOffset = 0;
	pShapeBuf = pTileBuf;

	nCurrHeight = 0;
	if (nDir == SHUNT_UP)
		++nCurrHeight;
	else if (nDir == SHUNT_DOWN) {
		SPRITEDATA(pShapeBuf)->nCount = 0;
		SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_NEWROWSTART;
		pShapePrevRowBits = pShapeBuf;
		pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
		pShapeBuf = pTileBuf;
	}

	for (; nCurrHeight < shapeHeight; ++nCurrHeight) {
		// Calculation changed from nWidth * (shapeHeight - nCurrHeight - 1) + nOffSet
		nHeight = (shapeHeight - nCurrHeight) - 1;
		pStartBit = &pThisBits[nOffSet + nWidth * nHeight];
		BYTE pCheckStartBit = *pStartBit;
		if (nCurrHeight > shapeHeight) {
			if (nDir == SHUNT_DOWN)
				continue;
		}
		nCurrWidth = 0;
		if (nDir == SHUNT_LEFT) {
			++nCurrWidth;
			++pStartBit;
		}
		while (nCurrWidth < shapeWidth) {
			BYTE pCheckStartWidthBit = *pStartBit;
			nCurrWidthSize = 0;
			if (nDir == SHUNT_RIGHT) {
				if (nCurrWidth == 0) {
					pCheckStartWidthBit = 0xFC;
					++nCurrWidthSize;
				}
			}
			if (pCheckStartWidthBit == 0xFC) {
				for (pCurrBit = pStartBit; nCurrWidth + nCurrWidthSize < shapeWidth && *pCurrBit == 0xFC; ++pCurrBit) {
					++nCurrWidthSize;
				}
				nCurrWidth += nCurrWidthSize;
				pStartBit = pCurrBit;
				if (nCurrWidth < shapeWidth) {
					SPRITEDATA(pShapeBuf)->nCount = nCurrWidthSize;
					SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_SKIPPIXELS;
					pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
					pShapeBuf = pTileBuf;
					pTileRowOffset += 2;
				}
			}
			else {
				for (pCurrBit = pStartBit; nCurrWidth + nCurrWidthSize < shapeWidth && *pCurrBit != 0xFC; ++pCurrBit) {
					++nCurrWidthSize;
				}
				SPRITEDATA(pShapeBuf)->nCount = nCurrWidthSize;
				SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_PROCPIXELS;
				pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
				memcpy(pTileBuf, pStartBit, nCurrWidthSize);
				pShapeBuf = &pTileBuf[nCurrWidthSize];
				pTileRowOffset += nCurrWidthSize + 2;
				pStartBit = pCurrBit;
				nCurrWidth += nCurrWidthSize;
				if (!IsEvenUnsigned(nCurrWidthSize)) {
					*pShapeBuf++ = 0;
					++pTileRowOffset;
				}
			}
		}
		SPRITEDATA(pShapePrevRowBits)->nCount = pTileRowOffset;
		// Only encode a new row if nCurrHeight is less than shapeHeight - 1
		// otherwise it'll be the endofsprite marker.
		if (nCurrHeight < shapeHeight - 1) {
			SPRITEDATA(pShapeBuf)->nCount = 0;
			SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_NEWROWSTART;
			pShapePrevRowBits = pShapeBuf;
			pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
			pShapeBuf = pTileBuf;
		}
		pTileRowOffset = 0;
	}
	SPRITEDATA(pShapeBuf)->nCount = 2;
	SPRITEDATA(pShapeBuf)->nChunkMode = MIF_CM_ENDOFSPRITE;
	pTileBuf = (BYTE *)&SPRITEDATA(pShapeBuf)->pBuf;
	pThis->mLength = pTileBuf - pThis->mShapeBuf;
}

static int L_SCURK_GetTileBase(cEditableTileSet *pEdTileSet, int nEdNum) {
	int nShapeWidth, nTileBase;

	nShapeWidth = R_SCURK_WRP_EditableTileSet_mGetShapeWidth(pEdTileSet, nEdNum) - SINGLE_TILE_WIDTH;
	nTileBase = TILE_BASE_4x4;
	if (nShapeWidth) {
		nTileBase = TILE_BASE_3x3;
		nShapeWidth -= SINGLE_TILE_WIDTH;
		if (nShapeWidth) {
			nTileBase = TILE_BASE_2x2;
			nShapeWidth -= SINGLE_TILE_WIDTH;
			if (nShapeWidth) {
				nTileBase = TILE_BASE_1x1;
				if (nShapeWidth < SINGLE_TILE_WIDTH)
					nTileBase = TILE_BASE_INVALID;
			}
		}
	}

	return nTileBase;
}

static void L_SCURK_RefreshTile(cPaintWindow *pThis, cEditableTileSet *pEdTileSet) {
	int nEdNum, nDBID;
	int nTileBase;

	// Store the adjusted Shape information
	// (Only large is needed here).
	nEdNum = pThis->pScurkEditParent->nEdNum;
	nDBID = pEdTileSet->mDBIndexFromShapeNum[pEdTileSet->mShapeNumFromEditableNum[nEdNum]];
	R_BOR_WRP_gFreeBlock(pEdTileSet->mTiles[nDBID]);
	pEdTileSet->mTiles[nDBID] = (BYTE *)R_BOR_WRP_gAllocBlock(pThis->pEncodeDib->mLength);
	memcpy(pEdTileSet->mTiles[nDBID], pThis->pEncodeDib->mShapeBuf, pThis->pEncodeDib->mLength);
	pEdTileSet->mTileSizeTable[nDBID] = pThis->pEncodeDib->mLength;
	pEdTileSet->mTileSet->pData[nDBID].sprHeader.wHeight = LOWORD(pThis->pEncodeDib->mHeight);

	nTileBase = L_SCURK_GetTileBase(pEdTileSet, nEdNum);

	// Clear and refresh the displayed shape.
	R_SCURK_WRP_PaintWindow_mClearTile(pThis, nTileBase);
	R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Dib(pEdTileSet, pThis->pEncodeDib, nEdNum);
	R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Graphic(pEdTileSet, pThis->pGraphic, nEdNum);
	InvalidateRect(pThis->HWindow, 0, 0);
}

static void L_SCURK_MoveDIB(winscurkMDIClient *pThis, int nDir) {
	winscurkApp *pSCApp;
	TEncodeDib *pEncDib;
	__int32 nXOffset;
	WORD shapeWidth, shapeHeight;
	cPaintWindow *pPaintWnd;

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	if (pThis->mEditWindow) {
		pPaintWnd = pThis->mEditWindow->pPaintWindow;

		pEncDib = (TEncodeDib *)R_BOR_Op_New(sizeof(TEncodeDib));
		if (!pEncDib) {
			ConsoleLog(LOG_DEBUG, "L_SCURK_MoveDIB(%d): !pEncDib allocation has failed.\n", nDir);
			return;
		}

		pEncDib = R_SCURK_WRP_EncodeDib_Construct_Dimens(pEncDib, 128, 256, 0x100, DIB_PAL_COLORS);
		if (pEncDib) {
			// First set the Undo Buffer
			// (Placed outside the brackets deliberately
			// just in case we want to shunt by more
			// than 1).
			R_SCURK_WRP_PaintWindow_mPreserveToUndoBuffer(pPaintWnd);
			{
				nXOffset = 64 - ((int)(WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent) >> 1);
				R_SCURK_WRP_EncodeDib_mShrink(pEncDib, pPaintWnd->pEncodeDib, 1);
				shapeWidth = (WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent);
				shapeHeight = R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(pEncDib);
				L_SCURK_EncodeWithShunt(pEncDib, shapeHeight, shapeWidth, nXOffset, nDir);
				R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(pPaintWnd->pEncodeDib, pEncDib);

				L_SCURK_RefreshTile(pPaintWnd, pSCApp->mWorkingTiles);
			}

			R_SCURK_WRP_EncodeDib_Destruct(pEncDib, 3);
		}
	}
}

extern "C" LRESULT __cdecl Hook_SCURK_FrameWindow_EvCommand(TBC45XFrameWindow *pThis, DWORD id, HWND hWndCtl, DWORD notifyCode) {
	if (mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_ONCMD)
		ConsoleLog(LOG_DEBUG, "0x%06X -> TFrameWindow::EvCommand(0x%06X, %u, 0x%06X, %u)\n", _ReturnAddress(), pThis, id, hWndCtl, notifyCode);

	winscurkApp *pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	DWORD dwDecoFrmEvCmdAddr = R_SCURK_ADDR_FrameWindow_EvCommand_To_TDecoratedFrame_EvCommand();
	if (dwDecoFrmEvCmdAddr && (DWORD)_ReturnAddress() == dwDecoFrmEvCmdAddr) {
		if (pThis == (TBC45XFrameWindow *)pSCApp->mdiClient->mParent->pFrameWnd) {
			switch (id) {
			case IDM_SCRK_EW_EDIT_MOVE_UP:
				L_SCURK_MoveDIB(pSCApp->mdiClient, SHUNT_UP);
				return TRUE;
			case IDM_SCRK_EW_EDIT_MOVE_DOWN:
				L_SCURK_MoveDIB(pSCApp->mdiClient, SHUNT_DOWN);
				return TRUE;
			case IDM_SCRK_EW_EDIT_MOVE_LEFT:
				L_SCURK_MoveDIB(pSCApp->mdiClient, SHUNT_LEFT);
				return TRUE;
			case IDM_SCRK_EW_EDIT_MOVE_RIGHT:
				L_SCURK_MoveDIB(pSCApp->mdiClient, SHUNT_RIGHT);
				return TRUE;
			default:
				break;
			}
		}
	}

	if (hWndCtl == 0) {
		HWND hCmdTarget = R_BOR_WRP_FrameWindow_GetCommandTarget(pThis);

		while (hCmdTarget && hCmdTarget != pThis->pWnd->HWindow) {
			TBC45XWindow *cmdTarget = R_BOR_WRP_GetWindowPtr(hCmdTarget, 0);

			if (cmdTarget)
				return R_BOR_WRP_Window_EvCommand(cmdTarget, id, hWndCtl, notifyCode);

			hCmdTarget = GetParent(hCmdTarget);
		}
	}

	return R_BOR_WRP_Window_EvCommand(pThis->pWnd, id, hWndCtl, notifyCode);
}
