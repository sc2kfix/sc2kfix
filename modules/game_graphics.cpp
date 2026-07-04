// sc2kfix modules/game_graphics.cpp: game graphics class
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Library-side calls to certain critical calls that would otherwise fail
// or cause anomalous behaviour if the equivalent remote-calls were used in the
// wrong context.

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

extern bool CycleIndexCheck(BYTE palIdx);
extern BYTE CyclePaletteIdx(BYTE colIdx, int cIdx);

extern __int16 nCycleIdx;

static BOOL L_hWndBeginProcessObject_SC2K1996(HWND hWnd, void *vBits, int x, int y, RECT *r) {
	CMFC3XRect clRect;

	GetClientRect(hWnd, &clRect);
	if (IsRectEmpty(r))
		currWndClientRect = clRect;
	else if (!IntersectRect(&currWndClientRect, r, &clRect))
		return FALSE;
	if (currWndClientRect.top > 1)
		--currWndClientRect.top;
	Game_SetSpriteForDrawing(vBits, pArrSpriteHeaders, x, (__int16)y, &currWndClientRect);
	return TRUE;
}

BOOL PrepareDialogSpriteGraphic_SC2K1996(CGraphics *pGraphic, HWND hWnd, sprite_header_t *pSprHead, __int16 nSpriteID, CMFC3XRect *pDlgRect, __int16 isFlipped, __int16 doInvert, int nType) {
	CMFC3XPoint sprPt;
	CMFC3XRect sprRect;
	BYTE *pSprBits;
	WORD nWidthOffset;
	CMFC3XDC *pDC;
	BOOL bSpriteFail = TRUE;

	if (pSprHead) {
		if (pSprHead->wHeight > 1) {
			nWidthOffset = (nType >= PALCACHE_TYPE_CYCLE) ? 10 : 7;
			sprPt.x = (pSprHead->wWidth + nWidthOffset) & ~7;
			sprPt.y = (pSprHead->wHeight + 8) & ~7;

			if (pGraphic) {
				pGraphic = Game_Graphics_Cons(pGraphic);
				if (pGraphic) {
					sprRect.left = 0;
					sprRect.top = 0;
					sprRect.right = sprPt.x;
					sprRect.bottom = sprPt.y;

					bSpriteFail = (!pGraphic->CreateWithPalette_SC2K1996(sprPt.x, sprPt.y)) ? TRUE : FALSE;
					if (!bSpriteFail) {
						pSprBits = Game_Graphics_LockDIBBits(pGraphic);
						pDC = pGraphic->GetDC_SC2K1996();
						if (pDC) {
							FillRect(pDC->m_hDC, &sprRect, (HBRUSH)MainBrushFace->m_hObject);
							pGraphic->ReleaseDC_SC2K1996(pDC);

							L_hWndBeginProcessObject_SC2K1996(hWnd, pSprBits, sprPt.x, sprPt.y, pDlgRect);
							if (nType >= PALCACHE_TYPE_CYCLE)
								L_drawShapeSpecific_SC2K1996(nSpriteID, 0, 0, isFlipped, doInvert, nType);
							else
								L_drawShapeDialog_SC2K1996(nSpriteID, 0, 0, 0, 0);
							Game_FinishProcessObjects();
						}
						Game_Graphics_UnlockDIBBits(pGraphic);
					}
				}
			}
		}
	}

	return bSpriteFail;
}

void ShowCurrentDialogSpriteGraphic_SC2K1996(CGraphics *pGraphic, HWND hWnd, sprite_header_t *pSprHead, __int16 nSpriteID, CMFC3XRect *pDlgRect, BOOL bSpriteFail, __int16 isFlipped, __int16 doInvert, int nType) {
	CMFC3XPoint sprPt;
	CMFC3XRect sprRect;
	BYTE *pSprBits;
	WORD nWidthOffset;
	CMFC3XDC *pDC;

	if (pSprHead) {
		if (pSprHead->wHeight > 1) {
			nWidthOffset = (nType >= PALCACHE_TYPE_CYCLE) ? 10 : 7;
			sprPt.x = (pSprHead->wWidth + nWidthOffset) & ~7;
			sprPt.y = (pSprHead->wHeight + 8) & ~7;
			if (pGraphic && !bSpriteFail) {
				sprRect.left = 0;
				sprRect.top = 0;
				sprRect.right = sprPt.x;
				sprRect.bottom = sprPt.y;

				pSprBits = Game_Graphics_LockDIBBits(pGraphic);
				pDC = pGraphic->GetDC_SC2K1996();
				if (pDC) {
					FillRect(pDC->m_hDC, &sprRect, (HBRUSH)MainBrushFace->m_hObject);
					pGraphic->ReleaseDC_SC2K1996(pDC);

					L_hWndBeginProcessObject_SC2K1996(hWnd, pSprBits, sprPt.x, sprPt.y, pDlgRect);
					if (nType >= PALCACHE_TYPE_CYCLE)
						L_drawShapeSpecific_SC2K1996(nSpriteID, 0, 0, isFlipped, doInvert, nType);
					else
						L_drawShapeDialog_SC2K1996(nSpriteID, 0, 0, 0, 0);
					Game_FinishProcessObjects();
				}
				Game_Graphics_UnlockDIBBits(pGraphic);
			}
		}
	}
}

void CGraphics::DeleteStored_SC2K1996() {
	if (GRBitmap)
		::DeleteObject(GRBitmap);
	GRBitmap = 0;
	if (GRBitmapLoColor)
		::DeleteObject(GRBitmapLoColor);
	GRBitmapLoColor;
	if (GRpBitmapInfo) {
		free(GRpBitmapInfo);
		GRpBitmapInfo = 0;
	}
}

int CGraphics::CreateWithPalette_SC2K1996(LONG ibiWidth, LONG ibiHeight) {
	CSimcityAppPrimary *pSCApp;
	void *ppvBits;
	void *ppvBitsLoColor;
	BITMAPINFO *pBmi;
	CMFC3XPalette *pActPal;
	PALETTEENTRY palEnts[HICOLORCNT];
	BYTE *ippvBits;

	pSCApp = &pCSimcityAppThis;
	if (GRBitmap)
		Game_Graphics_DeleteObject(this);
	if (GRpBitmapInfo) {
		free(GRpBitmapInfo);
		GRpBitmapInfo = 0;
	}
	pBmi = (BITMAPINFO *)malloc(0x428);
	if (!pBmi) {
		ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): pBmi - memory allocation has failed.\n", ibiWidth, ibiHeight);
		return 0;
	}
	pBmi->bmiHeader.biSize = 40;
	pBmi->bmiHeader.biPlanes = 1;
	pBmi->bmiHeader.biBitCount = 8;
	pBmi->bmiHeader.biCompression = 0;
	pBmi->bmiHeader.biSizeImage = 0;
	pBmi->bmiHeader.biClrUsed = 0;
	pBmi->bmiHeader.biClrImportant = 0;
	pBmi->bmiHeader.biWidth = ibiWidth;
	pBmi->bmiHeader.biHeight = ibiHeight * GRorient;
	pActPal = Game_SimcityApp_GetActivePalette(pSCApp);
	GetPaletteEntries((HPALETTE)pActPal->m_hObject, 0, HICOLORCNT, palEnts);
	for (int i=0; i<HICOLORCNT; ++i) {
		pBmi->bmiColors[i].rgbRed = palEnts[i].peRed;
		pBmi->bmiColors[i].rgbGreen = palEnts[i].peGreen;
		pBmi->bmiColors[i].rgbBlue = palEnts[i].peBlue;
		pBmi->bmiColors[i].rgbReserved = 0;
	}
	ppvBits = 0;
	ppvBitsLoColor = 0;
	GRBitmap = CreateDIBSection(hDC_Global, pBmi, DIB_RGB_COLORS, &ppvBits, 0, 0);
	if (!GRBitmap) {
		ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): !GRBitmap - CreateDIBSection has failed.\n", ibiWidth, ibiHeight);
		free(pBmi);
		return 0;
	}
	if (bLoColor) {
		GRBitmapLoColor = CreateDIBSection(hDC_Global, pBmi, DIB_RGB_COLORS, &ppvBitsLoColor, 0, 0);
		if (!GRBitmapLoColor) {
			ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): !GRBitmapLoColor - CreateDIBSection has failed.\n", ibiWidth, ibiHeight);
			::DeleteObject(GRBitmap);
			free(pBmi);
			return 0;
		}
		GRpBitsLoColor = (BYTE *)ppvBitsLoColor;
		Game_Graphics_Set16ColorTable(this);
	}
	GRwidth = ibiWidth;
	GRheight = ibiHeight;
	GRpBits = (BYTE *)ppvBits;
	GRpAppPalette = Game_SimcityApp_GetActivePalette(pSCApp);
	ippvBits = (BYTE *)ppvBits;
	for (DWORD biSizeImage = 0; pBmi->bmiHeader.biSizeImage > biSizeImage; ++biSizeImage)
		*ippvBits++ = 0;
	GRpBitmapInfo = pBmi;
	if (GRpBitmapInfo->bmiHeader.biHeight < 0)
		GRpBitmapInfo->bmiHeader.biHeight = -GRpBitmapInfo->bmiHeader.biHeight;
	return 1;
}

void CGraphics::PaintNormalAndStretch(HDC hDC, int x, int y, int sX, int sY, int nFactor) {
	CSimcityAppPrimary *pSCApp;
	HGDIOBJ hObj;
	CMFC3XPalette *pActPal;
	HPALETTE hActPal;
	HPALETTE hSelPal;
	BOOL bForceBkgd = FALSE;

	pSCApp = &pCSimcityAppThis;
	if (GRBitmap) {
		if (bLoColor) {
			Game_Graphics_RemapTo16ColorsMain(this);
			hObj = ::SelectObject(hDC_Global, GRBitmapLoColor);
			hActPal = hLoColor;
			bForceBkgd = FALSE;
		}
		else {
			hObj = ::SelectObject(hDC_Global, GRBitmap);
			pActPal = Game_SimcityApp_GetActivePalette(pSCApp);
			hActPal = (HPALETTE)pActPal->m_hObject;
			bForceBkgd = pSCApp->dwSCAbForceBkgd;
		}
		hSelPal = SelectPalette(hDC, hActPal, bForceBkgd);
		RealizePalette(hDC);
		::BitBlt(hDC, x, y, GRwidth, GRheight, hDC_Global, 0, 0, SRCCOPY);
		::StretchBlt(hDC, sX, sY, GRwidth * nFactor, GRheight * nFactor, hDC_Global, 0, 0, GRwidth, GRheight, SRCCOPY);
		SelectPalette(hDC, hSelPal, 0);
		::SelectObject(hDC_Global, hObj);
	}
}

CMFC3XDC *CGraphics::GetDC_SC2K1996() {
	CMFC3XDC *pDC;

	if (GRBitmap)
		g_hBitmapOld = (HBITMAP)SelectObject(hDC_Global, GRBitmap);

	pDC = new CMFC3XDC();
	if (pDC)
		pDC = GameMain_DC_Cons(pDC);
	GameMain_DC_Attach(pDC, hDC_Global);
	return pDC;
}

void CGraphics::ReleaseDC_SC2K1996(CMFC3XDC *pDC) {
	GameMain_DC_Detach(pDC);
	if (GRBitmap)
		SelectObject(hDC_Global, g_hBitmapOld);
	if (pDC) {
		GameMain_DC_Destruct(pDC);
		delete pDC;
		pDC = NULL;
	}
	g_hBitmapOld = 0;
}

void L_SetRectBackground_SC2K1996(HDC hDC, LONG left, LONG top, LONG right, LONG bottom, COLORREF cr) {
	RECT r;

	SetBkColor(hDC, cr);
	SetRect(&r, left, top, right + left, top + bottom);
	ExtTextOutA(hDC, 0, 0, OPAQUE, &r, 0, 0, NULL);
}

void L_SetButtonShape_SC2K1996(HDC hDC, int nInnerWidth, int nInnerHeight, DWORD dwState) {
	L_SetRectBackground_SC2K1996(hDC, 1, 0, nInnerWidth, 1, crDlgColWndFrame);
	L_SetRectBackground_SC2K1996(hDC, 1, nInnerHeight + 1, nInnerWidth, 1, crDlgColWndFrame);
	L_SetRectBackground_SC2K1996(hDC, 0, 1, 1, nInnerHeight, crDlgColWndFrame);
	L_SetRectBackground_SC2K1996(hDC, nInnerWidth + 1, 1, 1, nInnerHeight, crDlgColWndFrame);
	if ((dwState & (TBBS_CHECKED | TBBS_PRESSED)) != 0) {
		L_SetRectBackground_SC2K1996(hDC, 1, 1, 1, nInnerHeight, crDlgColBtnShadow);
		L_SetRectBackground_SC2K1996(hDC, 1, 1, nInnerWidth, 1, crDlgColBtnShadow);
	}
	else {
		L_SetRectBackground_SC2K1996(hDC, 1, 1, 1, nInnerHeight - 1, crDlgColBtnHighlight);
		L_SetRectBackground_SC2K1996(hDC, 1, 1, nInnerWidth - 1, 1, crDlgColBtnHighlight);
		L_SetRectBackground_SC2K1996(hDC, nInnerWidth, 1, 1, nInnerHeight, crDlgColBtnShadow);
		L_SetRectBackground_SC2K1996(hDC, 1, nInnerHeight, nInnerWidth, 1, crDlgColBtnShadow);
		L_SetRectBackground_SC2K1996(hDC, nInnerWidth - 1, 2, 1, nInnerHeight - 2, crDlgColBtnShadow);
		L_SetRectBackground_SC2K1996(hDC, 2, nInnerHeight - 1, nInnerWidth - 2, 1, crDlgColBtnShadow);
	}
}

typedef struct {
	DWORD nFrID;
	int nHeight;
	int nWidth;
	DWORD nSize;
	BYTE  *pBuf;
} imageFrame_t;

static std::vector<imageFrame_t> imageCache;

static void Create_ImageNew(BYTE *pImageBuf, DWORD nSize, int nHeight, int nWidth, int nFrm) {
	imageFrame_t imgFrame;

	memset(&imgFrame, 0, sizeof(imgFrame));
	imgFrame.nFrID = nFrm;
	imgFrame.nHeight = nHeight;
	imgFrame.nWidth = nWidth;
	imgFrame.nSize = nSize;
	imgFrame.pBuf = (BYTE *)malloc(nSize);
	if (imgFrame.pBuf) {
		memcpy(imgFrame.pBuf, pImageBuf, nSize);
		for (DWORD nBit = 0; nBit < nSize; ++nBit) {
			imgFrame.pBuf[nBit] = CyclePaletteIdx(imgFrame.pBuf[nBit], nFrm);
		}
	}
	else {
		ConsoleLog(LOG_ERROR, "Create_ImageNew(%u, %d, %d, %d): Allocation failed for image frame.\n", nSize, nHeight, nWidth, nFrm);
		imgFrame.pBuf = 0;
	}
	// Push even if allocation fails.
	imageCache.push_back(imgFrame);
}

int L_LoadAnimatedGraphic_SC2K1996(CMainFrame *pMainFrm, const char *pStr) {
	int ret = Game_MainFrame_LoadGraphic(pMainFrm, pStr);
	if (ret && !bLoColor) {
		if (pMainFrm->dwMFCGraphicsOne) {
			if (pMainFrm->dwMFCGraphicsOne->GRpBits) {
				BOOL bCycling = FALSE;
				for (DWORD nBit = 0; nBit < pMainFrm->dwMFCGraphicsOne->GRpBitmapInfo->bmiHeader.biSizeImage; ++nBit) {
					if (CycleIndexCheck(pMainFrm->dwMFCGraphicsOne->GRpBits[nBit])) {
						bCycling = TRUE;
						break;
					}
				}
				if (bCycling) {
					BYTE *pBits = Game_Graphics_LockDIBBits(pMainFrm->dwMFCGraphicsOne);
					if (pBits) {
						for (int nFrm = 0; nFrm < CACHED_FRAMES; ++nFrm)
							Create_ImageNew(pBits, pMainFrm->dwMFCGraphicsOne->GRpBitmapInfo->bmiHeader.biSizeImage, pMainFrm->dwMFCGraphicsOne->GRheight, pMainFrm->dwMFCGraphicsOne->GRwidth, nFrm);
						Game_Graphics_UnlockDIBBits(pMainFrm->dwMFCGraphicsOne);
					}
				}
			}
		}
	}
	return ret;
}

int L_DeleteAnimatedGraphic_SC2K1996(CMainFrame *pMainFrm, BOOL bUnused) {
	if (pMainFrm && pMainFrm->dwMFCGraphicsOne && !bLoColor) {
		for (std::vector<imageFrame_t>::iterator itFr = imageCache.begin(); itFr != imageCache.end();) {
			if (itFr->pBuf) {
				free(itFr->pBuf);
				itFr->pBuf = 0;
			}
			itFr = imageCache.erase(itFr);
		}

		imageCache.clear();
	}
	return Game_MainFrame_DeleteGraphic(pMainFrm, bUnused);
}

static BYTE *Get_ImageFrame_Buffer(BYTE *pImageBuf, DWORD nFrmIdx) {
	if (nFrmIdx >= 0 && imageCache.size() > 0) {
		for (std::vector<imageFrame_t>::reverse_iterator itFr = imageCache.rbegin(); itFr != imageCache.rend();) {
			if (itFr->nFrID <= nFrmIdx) {
				if (itFr->pBuf)
					return itFr->pBuf;
			}
			++itFr;
		}
	}
	return pImageBuf;
}

void L_NextAnimatedImageFrame_SC2K1996(CGraphics *pGraphic) {
	// Only used if:
	// 1) the buffer isn't NULL
	// 2) Not in LoColor mode
	// 3) the cache has more than 1 frame.
	if (pGraphic && pGraphic->GRpBits && !bLoColor) {
		if (imageCache.size() > 1) {
			BYTE *pBits = Game_Graphics_LockDIBBits(pGraphic);
			if (pBits) {
				int nFrmIdx = nCycleIdx % CACHED_FRAMES;
				if (nFrmIdx < 0)
					nFrmIdx = -nFrmIdx;
				BYTE *pNewBits = Get_ImageFrame_Buffer(pBits, nFrmIdx);
				if (pNewBits)
					memcpy(pBits, pNewBits, imageCache[0].nSize);
				Game_Graphics_UnlockDIBBits(pGraphic);
			}
		}
	}
}

extern "C" void __stdcall Hook_SimcityWnd_OnEraseBkgnd(CMFC3XDC *pDC) {
	CSimcityWnd *pThis;

	__asm mov[pThis], ecx

	RECT r;
	POINT pt;
	HBRUSH hBrush;
	HGDIOBJ hOldObj;
	int iGrHeight, iGrWidth, iHeight, iWidth;

	GetClientRect(pThis->m_hWnd, &r);
	hBrush = CreateSolidBrush(0);
	SetBrushOrgEx(pDC->m_hDC, 0, 0, &pt);
	hOldObj = SelectObject(pDC->m_hDC, hBrush);
	if (pThis->m_pSCWGraphics) {
		iGrHeight = Game_Graphics_Height(pThis->m_pSCWGraphics);
		iGrWidth = Game_Graphics_Width(pThis->m_pSCWGraphics);
		iWidth = (r.right - r.left - iGrWidth) / 2;
		iHeight = (r.bottom - r.top - iGrHeight) / 2;
		PatBlt(pDC->m_hDC, r.left, r.top, r.right - r.left, iHeight, PATCOPY);
		PatBlt(pDC->m_hDC, r.left, iHeight, iWidth, iGrHeight, PATCOPY);
		PatBlt(pDC->m_hDC, iWidth + iGrWidth, iHeight, iWidth, iGrHeight, PATCOPY);
		PatBlt(pDC->m_hDC, r.left, iHeight + iGrHeight, r.right - r.left, iHeight, PATCOPY);
		L_NextAnimatedImageFrame_SC2K1996(pThis->m_pSCWGraphics);
		Game_Graphics_SetColorTableFromApplicationPalette(pThis->m_pSCWGraphics);
		Game_Graphics_Paint(pThis->m_pSCWGraphics, pDC->m_hDC, iWidth, iHeight);
	}
	else
		PatBlt(pDC->m_hDC, r.left, r.top, r.right - r.left, r.bottom - r.top, PATCOPY);
	SelectObject(pDC->m_hDC, hOldObj);
	DeleteObject(hBrush);
}

void InstallGraphicHooks_SC2K1996(void) {
	// Hook for CSimcityWnd::OnEraseBkgnd
	SafeVirtualProtect((LPVOID)0x401D75, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401D75, Hook_SimcityWnd_OnEraseBkgnd);

	// Fix the black <-> white palette index swap
	// that occurs within CGraphics::RemapBitmapColors(BOOL)
	// eax rather than ecx.
	SafeVirtualProtect((LPVOID)0x475F5F, 1, PAGE_EXECUTE_READWRITE);
	*(BYTE*)0x475F5F = 0x84; // This was 0x8C
}
