// sc2kfix modules/scurk_objmove.cpp: PaintWindow object movement within working area.
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

// The dialogue for selecting both the
// horizontal and vertical shunting
// values.

typedef struct {
	int nHorzMove;
	int nHorzMax;
	int nVertMove;
	int nVertMax;
	const char *pTileDesc;
} shunt_info;

static int GetMoveFromPos(HWND hWndTrackbar, int nDirMax) {
	int nTrackPos, nDirMove;

	nDirMove = 0;
	if (hWndTrackbar) {
		nTrackPos = SendMessage(hWndTrackbar, TBM_GETPOS, 0, 0);
		nDirMove = (nTrackPos == nDirMax) ? 0 : nTrackPos - nDirMax;
		if (nDirMove > 0) {
			if (nDirMove > nDirMax)
				nDirMove = nDirMax;
		}
		else if (nDirMove < 0) {
			if (nDirMove < -nDirMax)
				nDirMove = -nDirMax;
		}
	}
	return nDirMove;
}

BOOL CALLBACK ShuntDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char szDlgTitle[128 + 1], szTextLabel[64 + 1];
	HWND hWndStatic, hWndTrackbar, hWndHotTrackbar;
	int nTrackPos;
	shunt_info *si;

	switch (message) {
		case WM_INITDIALOG:
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			si = (shunt_info *)lParam;

			sprintf_s(szDlgTitle, "Shunt Object - %s", si->pTileDesc);
			SetWindowTextA(hwndDlg, szDlgTitle);

			sprintf_s(szTextLabel, "Horizontal:    %d", si->nHorzMove);
			hWndStatic = GetDlgItem(hwndDlg, IDC_SHUNT_TXTHORZ);
			SetWindowTextA(hWndStatic, szTextLabel);

			sprintf_s(szTextLabel, "Vertical:    %d", si->nVertMove);
			hWndStatic = GetDlgItem(hwndDlg, IDC_SHUNT_TXTVERT);
			SetWindowTextA(hWndStatic, szTextLabel);

			hWndTrackbar = GetDlgItem(hwndDlg, IDC_SHUNT_SLIDHORZ);
			SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, si->nHorzMax * 2);
			nTrackPos = (si->nHorzMove == 0) ? si->nHorzMax : si->nHorzMax + si->nHorzMove;
			SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, nTrackPos);

			hWndTrackbar = GetDlgItem(hwndDlg, IDC_SHUNT_SLIDVERT);
			SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, si->nVertMax * 2);
			nTrackPos = (si->nVertMove == 0) ? si->nVertMax : si->nVertMax + si->nVertMove;
			SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, nTrackPos);

			CenterDialogBox(hwndDlg);
			return TRUE;

		case WM_COMMAND:
			si = (shunt_info *)GetWindowLong(hwndDlg, GWL_USERDATA);
			switch (GET_WM_COMMAND_ID(wParam, lParam)) {
				case IDOK:
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDCANCEL:
					si->nHorzMove = 0;
					si->nVertMove = 0;
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
			break;

		case WM_HSCROLL:
			si = (shunt_info *)GetWindowLong(hwndDlg, GWL_USERDATA);
			switch (GET_WM_HSCROLL_CODE(wParam, lParam)) {
				case TB_ENDTRACK:
					hWndHotTrackbar = GET_WM_HSCROLL_HWND(wParam, lParam);
					if (hWndHotTrackbar == GetDlgItem(hwndDlg, IDC_SHUNT_SLIDHORZ)) {
						si->nHorzMove = GetMoveFromPos(hWndHotTrackbar, si->nHorzMax);

						sprintf_s(szTextLabel, "Horizontal:    %d", si->nHorzMove);
						hWndStatic = GetDlgItem(hwndDlg, IDC_SHUNT_TXTHORZ);
						SetWindowTextA(hWndStatic, szTextLabel);
					}
					else if (hWndHotTrackbar == GetDlgItem(hwndDlg, IDC_SHUNT_SLIDVERT)) {
						si->nVertMove = GetMoveFromPos(hWndHotTrackbar, si->nVertMax);

						sprintf_s(szTextLabel, "Vertical:     %d", si->nVertMove);
						hWndStatic = GetDlgItem(hwndDlg, IDC_SHUNT_TXTVERT);
						SetWindowTextA(hWndStatic, szTextLabel);
					}
					break;
			}
			break;
	}
	return FALSE;
}

static BOOL L_SCURK_DoShuntBy(winscurkMDIClient *pParWnd, const char *pTileName, int *nHorzMove, int *nVertMove, int nHorzMax, int nVertMax) {
	BOOL bRes;
	shunt_info si;

	memset(&si, 0, sizeof(si));
	si.nHorzMove = *nHorzMove;
	si.nHorzMax = nHorzMax;
	si.nVertMove = *nVertMove;
	si.nVertMax = nVertMax;
	si.pTileDesc = pTileName;

	bRes = (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_SHUNT), pParWnd->pWnd->HWindow, ShuntDialogProc, (LPARAM)&si) == 1);
	*nHorzMove = si.nHorzMove;
	*nVertMove = si.nVertMove;
	return bRes;
}

// Functions to do with shunting

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

static int L_SCURK_GetTileBase(cEditableTileSet *pEdTileSet, int nEdNum, BOOL bInvert) {
	int nShapeWidth, nTileBase;

	nShapeWidth = R_SCURK_WRP_EditableTileSet_mGetShapeWidth(pEdTileSet, nEdNum) - SINGLE_TILE_WIDTH;
	nTileBase = (bInvert) ? TILE_BASE_1x1 : TILE_BASE_4x4;
	if (nShapeWidth) {
		nTileBase = (bInvert) ? TILE_BASE_2x2 : TILE_BASE_3x3;
		nShapeWidth -= SINGLE_TILE_WIDTH;
		if (nShapeWidth) {
			nTileBase = (bInvert) ? TILE_BASE_3x3 : TILE_BASE_2x2;
			nShapeWidth -= SINGLE_TILE_WIDTH;
			if (nShapeWidth) {
				nTileBase = (bInvert) ? TILE_BASE_4x4 : TILE_BASE_1x1;
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
	R_SCURK_WRP_gFreeBlock(pEdTileSet->mTiles[nDBID]);
	pEdTileSet->mTiles[nDBID] = (BYTE *)R_SCURK_WRP_gAllocBlock(pThis->pEncodeDib->mLength);
	memcpy(pEdTileSet->mTiles[nDBID], pThis->pEncodeDib->mShapeBuf, pThis->pEncodeDib->mLength);
	pEdTileSet->mTileSizeTable[nDBID] = pThis->pEncodeDib->mLength;
	pEdTileSet->mTileSet->pData[nDBID].sprHeader.wHeight = LOWORD(pThis->pEncodeDib->mHeight);

	nTileBase = L_SCURK_GetTileBase(pEdTileSet, nEdNum, FALSE);

	// Clear and refresh the displayed shape.
	R_SCURK_WRP_PaintWindow_mClearTile(pThis, nTileBase);
	R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Dib(pEdTileSet, pThis->pEncodeDib, nEdNum);
	R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Graphic(pEdTileSet, pThis->pGraphic, nEdNum);
	InvalidateRect(pThis->HWindow, 0, 0);
}

void L_SCURK_MoveDIB(winscurkMDIClient *pThis) {
	winscurkApp *pSCApp;
	int nHorzDir, nHorzMove, nHorzMax, 
		nVertDir, nVertMove, nVertMax, 
		nTileBase, nTileBaseInv, nMove;
	const char *pLongName;
	char szTileName[128 + 1];
	TEncodeDib *pEncDib;
	__int32 nXOffset;
	WORD shapeWidth, shapeHeight;
	cPaintWindow *pPaintWnd;

	pSCApp = R_SCURK_WRP_winscurkApp_GetPointerToClass();
	if (pThis->mEditWindow) {
		pPaintWnd = pThis->mEditWindow->pPaintWindow;

		nTileBase = L_SCURK_GetTileBase(pSCApp->mWorkingTiles, pPaintWnd->pScurkEditParent->nEdNum, FALSE);
		pLongName = R_SCURK_WRP_EditableTileSet_mGetLongName(pSCApp->mWorkingTiles, pPaintWnd->pScurkEditParent->nEdNum);

		nTileBaseInv = L_SCURK_GetTileBase(pSCApp->mWorkingTiles, pPaintWnd->pScurkEditParent->nEdNum, TRUE);

		nHorzDir = SHUNT_NONE;
		nHorzMove = 0;
		nHorzMax = 16;
		if (nTileBaseInv == TILE_BASE_4x4)
			nHorzMax = 64;
		else if (nTileBaseInv == TILE_BASE_3x3)
			nHorzMax = 48;
		else if (nTileBaseInv == TILE_BASE_2x2)
			nHorzMax = 32;

		nVertDir = SHUNT_NONE;
		nVertMove = 0;
		nVertMax = 64;

		sprintf(szTileName, "%s (%dx%d)", pLongName, nTileBase, nTileBase);
		if (!L_SCURK_DoShuntBy(pThis, szTileName, &nHorzMove, &nVertMove, nHorzMax, nVertMax))
			return;

		if (!nHorzMove && !nVertMove)
			return;

		if (nHorzMove < 0) {
			nHorzDir = SHUNT_LEFT;
			nHorzMove = -nHorzMove;
		}
		else if (nHorzMove > 0) {
			nHorzDir = SHUNT_RIGHT;
			nHorzMove = nHorzMove;
		}

		if (nVertMove < 0) {
			nVertDir = SHUNT_DOWN;
			nVertMove = -nVertMove;
		}
		else if (nVertMove > 0) {
			nVertDir = SHUNT_UP;
			nVertMove = nVertMove;
		}

		pEncDib = (TEncodeDib *)R_BOR_Op_New(sizeof(TEncodeDib));
		if (!pEncDib) {
			ConsoleLog(LOG_DEBUG, "L_SCURK_MoveDIB(): !pEncDib allocation has failed.\n");
			return;
		}

		pEncDib = R_SCURK_WRP_EncodeDib_Construct_Dimens(pEncDib, 128, 256, 0x100, DIB_PAL_COLORS);
		if (pEncDib) {
			// First set the Undo Buffer
			// (Placed outside the brackets deliberately
			// just in case we want to shunt by more
			// than 1).
			R_SCURK_WRP_PaintWindow_mPreserveToUndoBuffer(pPaintWnd);
			if (nHorzMove > 0) {
				for (nMove = 0; nMove < nHorzMove; ++nMove) {
					nXOffset = 64 - ((int)(WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent) >> 1);
					R_SCURK_WRP_EncodeDib_mShrink(pEncDib, pPaintWnd->pEncodeDib, 1);
					shapeWidth = (WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent);
					shapeHeight = R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(pEncDib);
					L_SCURK_EncodeWithShunt(pEncDib, shapeHeight, shapeWidth, nXOffset, nHorzDir);
					R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(pPaintWnd->pEncodeDib, pEncDib);

					L_SCURK_RefreshTile(pPaintWnd, pSCApp->mWorkingTiles);
				}
			}

			if (nVertMove > 0) {
				for (nMove = 0; nMove < nVertMove; ++nMove) {
					nXOffset = 64 - ((int)(WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent) >> 1);
					R_SCURK_WRP_EncodeDib_mShrink(pEncDib, pPaintWnd->pEncodeDib, 1);
					shapeWidth = (WORD)R_SCURK_WRP_EditWindow_mGetShapeWidth(pPaintWnd->pScurkEditParent);
					shapeHeight = R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(pEncDib);
					L_SCURK_EncodeWithShunt(pEncDib, shapeHeight, shapeWidth, nXOffset, nVertDir);
					R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(pPaintWnd->pEncodeDib, pEncDib);

					L_SCURK_RefreshTile(pPaintWnd, pSCApp->mWorkingTiles);
				}
			}

			R_SCURK_WRP_EncodeDib_Destruct(pEncDib, 3);
		}
	}
}
