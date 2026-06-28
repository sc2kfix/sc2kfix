// sc2kfix hooks/hook_bridgedialog.cpp: hooks for the bridge dialog
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

extern int nOwnDrwDlg;
extern CMFC3XWnd *pStoredWnd;

static POINT dwBSDPoints[BRIDGE_COUNT];
static DWORD dwBSDBtnState[BRIDGE_COUNT];
static HFONT hBSDFont = NULL;

static DWORD dwBridgeGroupBoxIDs[BRIDGE_COUNT] = {
	331,
	332,
	333,
	335,
	336,
	337,
	338
};

static DWORD dwBridgeSelectBtnIDs[BRIDGE_COUNT] = {
	120,
	121,
	122,
	123,
	124,
	125,
	126
};

static DWORD dwBridgeInfoBtnIDs[BRIDGE_COUNT] = {
	3,
	4,
	5,
	6,
	7,
	8,
	9
};

extern "C" void __stdcall Hook_drawBridgeShape(__int16 nWidth, __int16 nHeight, __int16 nSpriteID) {
	L_drawShapeDialog_SC2K1996(SPRITE_MEDIUM_WATER_TRBL, nWidth, nHeight - pArrSpriteHeaders[SPRITE_MEDIUM_WATER_TRBL].wHeight, 0, 0);
	L_drawShapeDialog_SC2K1996(nSpriteID, nWidth, nHeight - pArrSpriteHeaders[nSpriteID].wHeight, 0, 0);
}

// New call for the Highway Reinforced Bridge
// in-order to include the water sprites.
void L_drawHighwayReinforcedBridgeShape_SC2K1996(__int16 nWidth, __int16 nHeight, __int16 nSpriteID) {
	L_drawShapeDialog_SC2K1996(SPRITE_MEDIUM_WATER_TRBL, nWidth + 8, nHeight - 8 - pArrSpriteHeaders[SPRITE_MEDIUM_WATER_TRBL].wHeight, 0, 0);
	L_drawShapeDialog_SC2K1996(SPRITE_MEDIUM_WATER_TRBL, nWidth + 8, nHeight - pArrSpriteHeaders[SPRITE_MEDIUM_WATER_TRBL].wHeight, 0, 0);
	L_drawShapeDialog_SC2K1996(SPRITE_MEDIUM_WATER_TRBL, nWidth + 16, nHeight - 4 - pArrSpriteHeaders[SPRITE_MEDIUM_WATER_TRBL].wHeight, 0, 0);
	L_drawShapeDialog_SC2K1996(SPRITE_MEDIUM_WATER_TRBL, nWidth, nHeight - 4 - pArrSpriteHeaders[SPRITE_MEDIUM_WATER_TRBL].wHeight, 0, 0);
	L_drawShapeDialog_SC2K1996(nSpriteID, nWidth, nHeight - pArrSpriteHeaders[nSpriteID].wHeight, 0, 0);
}

extern "C" int __stdcall Hook_BridgeSelectDialog_OnInitDialog() {
	CBridgeSelectDialog *pThis;

	__asm mov [pThis], ecx

	__int16 nItem;
	HWND hDlgItem;
	RECT imageRect, dlgWndRect, dlgClientRect, cancelRect, borderRect, dlgRect, sectRect;
	int nPadding, nWidth;
	__int16 nBridgeTypeBit;
	HBRUSH hBrush;
	CGraphics *pGraphic;
	CMFC3XDC *pDC;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	pStoredWnd = pThis;
	nOwnDrwDlg = OWNDRW_DLG_BRIDGE;
	SetWindowTextA(pThis->m_hWnd, "Select Bridge");
	for (nItem = 0; nItem < BRIDGE_COUNT; ++nItem)
		pThis->dwBSDCGraphicsOne[nItem] = 0;
	hBSDFont = CreateFont(8, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeSelectBtnIDs[0]);
	GetWindowRect(hDlgItem, &imageRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.right);
	pThis->dwBSDWidth = (imageRect.right - imageRect.left);
	pThis->dwBSDHeight = (imageRect.bottom - imageRect.top);
	nItem = 0;
	wDlgNumAvailableBridges = 0;
	for (nBridgeTypeBit = 1; nBridgeTypeBit <= 64; nBridgeTypeBit *= 2) {
		if ((nBridgeTypeBit & pThis->dwBSDBridgeType) != 0) {
			wDlgAvailableBridges[wDlgNumAvailableBridges] = nItem;
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeGroupBoxIDs[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOWNORMAL);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeSelectBtnIDs[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOWNORMAL);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeInfoBtnIDs[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOWNORMAL);

			++wDlgNumAvailableBridges;
		}
		++nItem;
	}
	hDlgItem = GetDlgItem(pThis->m_hWnd, 331);
	GetWindowRect(hDlgItem, &borderRect);
	hDlgItem = GetDlgItem(pThis->m_hWnd, IDCANCEL);
	GetWindowRect(hDlgItem, &cancelRect);
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	nPadding = wDlgNumAvailableBridges & 3;
	nWidth = dlgWndRect.right +
		dlgClientRect.left - dlgClientRect.right -
		dlgWndRect.left +
		(((borderRect.right - borderRect.left) + nPadding) * wDlgNumAvailableBridges);
	SetRect(&dlgRect,
		0,
		0,
		nWidth,
		(cancelRect.bottom + dlgWndRect.bottom +
			dlgClientRect.top - dlgClientRect.bottom -
			dlgWndRect.top +
			(borderRect.bottom - borderRect.top) -
			cancelRect.top));
	SetWindowPos(pThis->m_hWnd, 0, 0, 0, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, SWP_NOZORDER | SWP_NOACTIVATE);
	crDlgColBtnFace = GetSysColor(COLOR_BTNFACE);
	crDlgColBtnShadow = GetSysColor(COLOR_BTNSHADOW);
	crDlgColBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
	crDlgColBtnText = GetSysColor(COLOR_BTNTEXT);
	crDlgColWndFrame = GetSysColor(COLOR_WINDOWFRAME);
	hBrush = CreateSolidBrush(crDlgColBtnFace);
	for (nItem = 0; nItem < wDlgNumAvailableBridges; ++nItem) {
		pGraphic = new CGraphics();
		if (pGraphic)
			pGraphic = Game_Graphics_Cons(pGraphic);
		else
			pGraphic = 0;
		pThis->dwBSDCGraphicsOne[nItem] = pGraphic;
		pThis->dwBSDCGraphicsOne[nItem]->CreateWithPalette_SC2K1996(pThis->dwBSDWidth, pThis->dwBSDHeight);

		dwBSDPoints[nItem].x = pThis->dwBSDWidth;
		dwBSDPoints[nItem].y = pThis->dwBSDHeight;
		pDC = Game_Graphics_GetDC(pThis->dwBSDCGraphicsOne[nItem]);
		vBridgeBits = Game_Graphics_LockDIBBits(pThis->dwBSDCGraphicsOne[nItem]);
		SetRect(&sectRect, 0, 0, pThis->dwBSDWidth,  pThis->dwBSDHeight);
		FillRect(pDC->m_hDC, &sectRect, hBrush);

		Game_BeginProcessObjects(pThis, vBridgeBits, pThis->dwBSDWidth, (__int16)pThis->dwBSDHeight, &dlgClientRect);
		__int16 nImageWidth = (__int16)pThis->dwBSDWidth;
		__int16 nImageHeight = (__int16)pThis->dwBSDHeight - 72;
		switch (wDlgAvailableBridges[nItem]) {
			case BRIDGE_WIRE:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_POWERLINES_BR);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_ELEVATED_POWERLINES);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_ELEVATED_POWERLINES);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_ELEVATED_POWERLINES);
				Game_drawBridgeShape(nImageWidth / 2 - 20, nImageHeight + 16, SPRITE_MEDIUM_POWERLINES_LHR);
				break;
			case BRIDGE_RAIL:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_RAIL_BRIDGE);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_RAIL_BRIDGE_PYLON);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_RAIL_BRIDGE);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_RAIL_BRIDGE_PYLON);
				Game_drawBridgeShape(nImageWidth / 2 - 20, nImageHeight + 16, SPRITE_MEDIUM_RAIL_BRIDGE);
				break;
			case BRIDGE_ROAD_CAUSEWAY:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_RAISING_BRIDGE_LOWERED);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_CAUSEWAY_PYLON);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_RAISING_BRIDGE_LOWERED);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_CAUSEWAY_PYLON);
				Game_drawBridgeShape(nImageWidth / 2 - 20, nImageHeight + 16, SPRITE_MEDIUM_RAISING_BRIDGE_LOWERED);
				break;
			case BRIDGE_ROAD_RAISING:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_RAISING_BRIDGE_LOWERED);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_RAISING_BRIDGE_TOWER);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_RAISING_BRIDGE_RAISED);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_RAISING_BRIDGE_TOWER);
				Game_drawBridgeShape(nImageWidth / 2 - 20, nImageHeight + 16, SPRITE_MEDIUM_RAISING_BRIDGE_LOWERED);
				break;
			case BRIDGE_ROAD_SUSPENSION:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_SUSPENSION_BRIDGE_END_T);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_SUSPENSION_BRIDGE_MIDDLE_T);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_SUSPENSION_BRIDGE_CENTER_B);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_SUSPENSION_BRIDGE_MIDDLE_B);
				Game_drawBridgeShape(nImageWidth / 2 - 20, nImageHeight + 16, SPRITE_MEDIUM_SUSPENSION_BRIDGE_START_B);
				break;
			case BRIDGE_HIGHWAY:
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 + 20, nImageHeight + 4, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 4, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 + 12, nImageHeight + 8, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 8, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 + 4, nImageHeight + 12, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 - 12, nImageHeight + 12, SPRITE_MEDIUM_HIGHWAY_LR);
				Game_drawBridgeShape(nImageWidth / 2 - 4, nImageHeight + 16, SPRITE_MEDIUM_HIGHWAY_LR);
				break;
			case BRIDGE_HIGHWAY_REINFORCED:
				L_drawHighwayReinforcedBridgeShape_SC2K1996(nImageWidth / 2 + 4, nImageHeight, SPRITE_MEDIUM_REINFORCED_BRIDGE_PYLON);
				L_drawHighwayReinforcedBridgeShape_SC2K1996(nImageWidth / 2 - 12, nImageHeight + 8, SPRITE_MEDIUM_REINFORCED_BRIDGE);
				L_drawHighwayReinforcedBridgeShape_SC2K1996(nImageWidth / 2 - 28, nImageHeight + 16, SPRITE_MEDIUM_REINFORCED_BRIDGE_PYLON);
				break;
			default:
				break;
		}
		Game_Graphics_UnlockDIBBits(pThis->dwBSDCGraphicsOne[nItem]);
		Game_FinishProcessObjects();
		Game_Graphics_ReleaseDC(pThis->dwBSDCGraphicsOne[nItem], pDC);

		Game_Graphics_RemapBitmapColorsFromSimcityAppPalette(pThis->dwBSDCGraphicsOne[nItem]);
		dwBSDBtnState[nItem] = 0;
	}
	DeleteObject(hBrush);
	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);
	return 1;
}

static void L_BridgeSelectDialog_OnDrawEntire_SC2K1996(CBridgeSelectDialog *pThis, int nPos, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	HWND hDlgItem;
	HDC hDlgDC;
	CMFC3XPaintDC *pDC;
	RECT rcDest;
	DWORD dwState;
	int nOuterWidth, nOuterHeight;
	int nOuterHalfWidth;
	int nInnerWidth, nInnerHeight;
	__int16 nAvailableBridge;
	int nCost;
	COLORREF cr, crTextOld;
	HFONT hFont;
	char szBuf[255 + 1];
	SIZE textSz;
	POINT pt;
	int nY;
	int nImageX, nImageY;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	CopyRect(&rcDest, &lpDIS->rcItem);
	dwState = dwBSDBtnState[nPos];
	nOuterWidth = rcDest.right - rcDest.left;
	nOuterHeight = rcDest.bottom - rcDest.top;
	nOuterHalfWidth = nOuterWidth / 2;
	nInnerWidth = nOuterWidth - 2;
	nInnerHeight = nOuterHeight - 2;
	nAvailableBridge = wDlgAvailableBridges[nPos];
	cr = (dwState == TBBS_CHECKED) ? crDlgColBtnShadow : crDlgColBtnFace;
	L_SetRectBackground_SC2K1996(lpDIS->hDC, 1, 1, nInnerWidth, nInnerHeight, cr);
	SetTextColor(pDC->m_hDC, crDlgColBtnText);
	SetBkColor(pDC->m_hDC, crDlgColBtnFace);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	hFont = SelectFont(pDC->m_hDC, hBSDFont);
	nCost = pThis->dwBSDNumTiles * dwBridgeBaseCost[nAvailableBridge];
	strcpy_s(szBuf, "Placeholder");
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	// One oddity here with the currency string is
	// that it'll move horizontally (observed prior
	// to any changes as well). This issue is greatly
	// mitigated by it now being on a separate line,
	// so it'll no longer overlap with the MW read-out.
	textSz.cx = Game_GetAvailableFunds(pDC, nCost);
	nY = nOuterHeight - textSz.cy - 4;
	crTextOld = crDlgColBtnText;
	if (nCost > dwCityFunds)
		crTextOld = SetTextColor(pDC->m_hDC, RGB(255, 0, 0));
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	Game_DisplayItemCost(pDC, nCost);
	SetTextColor(pDC->m_hDC, crTextOld);
	LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, dwBridgeStringIDs[nAvailableBridge], szBuf, sizeof(szBuf) - 1);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nY - textSz.cy;
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	nImageX = (nInnerWidth - dwBSDPoints[nPos].x - 1) >> 1;
	nImageY = (nInnerHeight - dwBSDPoints[nPos].y) >> 1;
	Game_Graphics_SetColorTableFromApplicationPalette(pThis->dwBSDCGraphicsOne[nPos]);
	Game_Graphics_BitBlit(pThis->dwBSDCGraphicsOne[nPos], lpDIS->hDC, nImageX + 1, nImageY - 40, dwBSDPoints[nPos].x, dwBSDPoints[nPos].y, 0, 0);
	if (dwState == TBBS_CHECKED)
		InvertRect(pDC->m_hDC, &rcDest);
	L_SetButtonShape_SC2K1996(lpDIS->hDC, nInnerWidth, nInnerHeight, dwState);
	SelectFont(pDC->m_hDC, hFont);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

static void L_BridgeSelectDialog_OnDrawState_SC2K1996(LPDRAWITEMSTRUCT lpDIS) {
	InvertRect(lpDIS->hDC, &lpDIS->rcItem);
}

static void L_BridgeSelectDialog_OnDrawFocus_SC2K1996(LPDRAWITEMSTRUCT lpDIS) {
	DrawFocusRect(lpDIS->hDC, &lpDIS->rcItem);
}

void L_BridgeSelectDialog_OnDrawItem_SC2K1996(CBridgeSelectDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	int nPos = -1;

	for (__int16 nItem = 0; nItem < ARCOLOGY_COUNT; ++nItem) {
		if (dwBridgeSelectBtnIDs[nItem] == nCtlID) {
			nPos = nItem;
			break;
		}
	}
	if (nPos >= 0) {
		if ((lpDIS->itemAction & ODA_DRAWENTIRE) != 0) {
			L_BridgeSelectDialog_OnDrawEntire_SC2K1996(pThis, nPos, nCtlID, lpDIS);
			if ((lpDIS->itemState & ODS_SELECTED) != 0)
				L_BridgeSelectDialog_OnDrawState_SC2K1996(lpDIS);
			if ((lpDIS->itemState & ODS_FOCUS) != 0)
				L_BridgeSelectDialog_OnDrawFocus_SC2K1996(lpDIS);
		}
		else {
			if ((lpDIS->itemAction & ODA_SELECT) != 0)
				L_BridgeSelectDialog_OnDrawState_SC2K1996(lpDIS);
			else if ((lpDIS->itemAction & ODA_FOCUS) != 0)
				L_BridgeSelectDialog_OnDrawFocus_SC2K1996(lpDIS);
		}
	}
}

extern "C" void __stdcall Hook_BridgeSelectDialog_OnPaint() {
	CBridgeSelectDialog *pThis;

	__asm mov [pThis], ecx

	CMFC3XPaintDC paintDC;

	GameMain_PaintDC_Cons(&paintDC, pThis);
	GameMain_PaintDC_Dest(&paintDC);
}

extern "C" void __stdcall Hook_BridgeSelectDialog_SetCursorAndDeleteGraphics() {
	CBridgeSelectDialog *pThis;

	__asm mov [pThis], ecx

	int nItem;

	nOwnDrwDlg = OWNDRW_DLG_NONE;
	pStoredWnd = NULL;
	Game_GameDialog_SetCursor(pThis);
	for (nItem = 0; nItem < BRIDGE_COUNT; ++nItem) {
		if (pThis->dwBSDCGraphicsOne[nItem]) {
			pThis->dwBSDCGraphicsOne[nItem]->DeleteStored_SC2K1996();
			delete pThis->dwBSDCGraphicsOne[nItem];
			pThis->dwBSDCGraphicsOne[nItem] = 0;
		}
	}
}

void InstallBridgeDialogHooks_SC2K1996(void) {
	// Adjust the dialog resource ID (113 - 0x71)
	SafeVirtualProtect((LPVOID)0x426333, 1, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x426333, 0x71, 1);

	// Adjust the message map referenced IDs.
	SafeVirtualProtect((LPVOID)0x4D8C18, 216, PAGE_EXECUTE_READWRITE);
	MFC3X_AFX_MSGMAP_ENTRY *pBMM = (MFC3X_AFX_MSGMAP_ENTRY *)(0x4D8C18);
	pBMM[0].nID = pBMM[0].nLastID = dwBridgeSelectBtnIDs[0];
	pBMM[1].nID = pBMM[1].nLastID = dwBridgeSelectBtnIDs[1];
	pBMM[2].nID = pBMM[2].nLastID = dwBridgeSelectBtnIDs[2];
	pBMM[3].nID = pBMM[3].nLastID = dwBridgeInfoBtnIDs[0];
	pBMM[4].nID = pBMM[4].nLastID = dwBridgeInfoBtnIDs[1];
	pBMM[5].nID = pBMM[5].nLastID = dwBridgeInfoBtnIDs[2];

	// Hook for drawBridgeShape
	// (placed here due to it being only
	// used in this context).
	SafeVirtualProtect((LPVOID)0x402941, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402941, Hook_drawBridgeShape);

	// Hook CBridgeSelectDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x4021DA, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4021DA, Hook_BridgeSelectDialog_OnInitDialog);

	// Hook CBridgeSelectDialog::OnPaint
	SafeVirtualProtect((LPVOID)0x4010C3, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4010C3, Hook_BridgeSelectDialog_OnPaint);

	// Hook CBridgeSelectDialog::SetCursorAndDeleteGraphics
	SafeVirtualProtect((LPVOID)0x401B27, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401B27, Hook_BridgeSelectDialog_SetCursorAndDeleteGraphics);
}
