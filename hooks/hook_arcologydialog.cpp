// sc2kfix hooks/hook_arcologydialog.cpp: hooks for the arcology dialog
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

DWORD dwArcologyBtnIDs[ARCOLOGY_COUNT] = {
	120,
	121,
	122,
	129
};

DWORD dwArcologyInfoBtnIDs[ARCOLOGY_COUNT] = {
	3,
	4,
	5,
	12
};

extern "C" int __stdcall Hook_SelectArcologyDialog_OnInitDialog() {
	CSelectArcologyDialog *pThis;

	__asm mov [pThis], ecx

	__int16 nItem;
	HFONT hFont;
	HWND hDlgItem;
	RECT dlgWndRect, dlgClientRect, borderRect, cancelRect, dlgRect, sectRect;
	int nPadding, nWidth;
	HBRUSH hBrush;
	CGraphics *pGraphics;
	CMFC3XDC *pDC;
	void *vBits;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	SetWindowTextA(pThis->m_hWnd, "Select Arcology");
	pThis->dwSADArcologySelection = -1;
	for (nItem = 0; nItem < ARCOLOGY_COUNT; ++nItem)
		pThis->dwSADCGraphicsOne[nItem] = 0;
	hFont = CreateFont(8, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	pThis->dwSADCFont.m_hObject =  hFont;
	for (nItem = 0; nItem < wGrantedArcologies; ++nItem) {
		ShowWindow(GetDlgItem(pThis->m_hWnd, dwArcologyBtnIDs[nItem]), SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(pThis->m_hWnd, dwArcologyInfoBtnIDs[nItem]), SW_SHOWNORMAL);
	}
	hDlgItem = GetDlgItem(pThis->m_hWnd, 331);
	GetWindowRect(hDlgItem, &borderRect);
	hDlgItem = GetDlgItem(pThis->m_hWnd, IDCANCEL);
	GetWindowRect(hDlgItem, &cancelRect);
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	nPadding = (wGrantedArcologies < ARCOLOGY_COUNT) ? wGrantedArcologies : wGrantedArcologies - 1;
	nWidth = dlgWndRect.right +
		dlgClientRect.left - dlgClientRect.right -
		dlgWndRect.left +
		(((borderRect.right - borderRect.left) + nPadding) * wGrantedArcologies);
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
	for (nItem = 0; nItem < wGrantedArcologies; ++nItem) {
		POINT pt;
		int nSpriteID = dwArcologySpriteIDs[nItem];

		pt.x = pArrSpriteHeaders[nSpriteID].wWidth;
		pt.y = pArrSpriteHeaders[nSpriteID].wHeight + 1;

		pGraphics = new CGraphics();
		if (pGraphics)
			pGraphics = Game_Graphics_Cons(pGraphics);
		else
			pGraphics = 0;
		pThis->dwSADCGraphicsOne[nItem] = pGraphics;
		pThis->dwSADCGraphicsOne[nItem]->CreateWithPalette_SC2K1996(pt.x, pt.y);

		pt.x = Game_Graphics_WidthBytes(pThis->dwSADCGraphicsOne[nItem]);
		pt.y = Game_Graphics_Height(pThis->dwSADCGraphicsOne[nItem]);

		pThis->dwSADPoints[nItem].x = pt.x;
		pThis->dwSADPoints[nItem].y = pt.y;

		pDC = Game_Graphics_GetDC(pThis->dwSADCGraphicsOne[nItem]);
		vBits = Game_Graphics_LockDIBBits(pThis->dwSADCGraphicsOne[nItem]);
		SetRect(&sectRect, 0, 0, pt.x, pt.y);
		FillRect(pDC->m_hDC, &sectRect, hBrush);

		Game_BeginProcessObjects(pThis, vBits, pt.x, (__int16)pt.y, &dlgClientRect);
		L_drawShapeDialog_SC2K1996(nSpriteID, 0, 0, 0, 0);
		Game_FinishProcessObjects();
		Game_Graphics_ReleaseDC(pThis->dwSADCGraphicsOne[nItem], pDC);

		Game_Graphics_RemapBitmapColorsFromSimcityAppPalette(pThis->dwSADCGraphicsOne[nItem]);
		pThis->dwSADBtnState[nItem] = 0;
	}
	DeleteObject(hBrush);
	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);
	return 1;
}

extern "C" void __stdcall Hook_SelectArcologyDialog_OnDrawItem(int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	CSelectArcologyDialog *pThis;

	__asm mov [pThis], ecx

	int nPos = -1;

	if (wGrantedArcologies > 0) {
		for (__int16 nItem = 0; nItem < ARCOLOGY_COUNT; ++nItem) {
			if (dwArcologyBtnIDs[nItem] == nCtlID) {
				nPos = nItem;
				break;
			}
		}
	}
	if (nPos >= 0) {
		if ((lpDIS->itemAction & ODA_DRAWENTIRE) != 0) {
			Game_SelectArcologyDialog_OnDrawEntire(pThis, nPos, nCtlID, lpDIS);
			if ((lpDIS->itemState & ODS_SELECTED) != 0)
				Game_SelectArcologyDialog_OnDrawState(lpDIS);
			if ((lpDIS->itemState & ODS_FOCUS) != 0)
				Game_SelectArcologyDialog_OnDrawFocus(lpDIS);
		}
		else {
			if ((lpDIS->itemAction & ODA_SELECT) != 0)
				Game_SelectArcologyDialog_OnDrawState(lpDIS);
			else if ((lpDIS->itemAction & ODA_FOCUS) != 0)
				Game_SelectArcologyDialog_OnDrawFocus(lpDIS);
		}
	}
}

extern "C" void __stdcall Hook_SelectArcologyDialog_OnDrawEntire(int nPos, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	CSelectArcologyDialog *pThis;

	__asm mov [pThis], ecx

	HWND hDlgItem;
	HDC hDlgDC;
	CMFC3XPaintDC *pDC;
	RECT rcDest;
	DWORD dwState;
	int nOuterWidth, nOuterHeight;
	int nOuterHalfWidth;
	int nInnerWidth, nInnerHeight;
	int nCost;
	COLORREF cr, crTextOld;
	HFONT hFont;
	char szBuf[255 + 1];
	SIZE textSz;
	POINT pt;
	int nY;
	const char *pTileStr;
	int nImageX, nImageY;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	CopyRect(&rcDest, &lpDIS->rcItem);
	dwState = pThis->dwSADBtnState[nPos];
	nOuterWidth = rcDest.right - rcDest.left;
	nOuterHeight = rcDest.bottom - rcDest.top;
	nOuterHalfWidth = nOuterWidth / 2;
	nInnerWidth = nOuterWidth - 2;
	nInnerHeight = nOuterHeight - 2;
	cr = (dwState == TBBS_CHECKED) ? crDlgColBtnShadow : crDlgColBtnFace;
	L_SetRectBackground_SC2K1996(lpDIS->hDC, 1, 1, nInnerWidth, nInnerHeight, cr);
	SetTextColor(pDC->m_hDC, crDlgColBtnText);
	SetBkColor(pDC->m_hDC, crDlgColBtnFace);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	hFont = SelectFont(pDC->m_hDC, pThis->dwSADCFont.m_hObject);
	LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, dwArcologyPopStrIDs[nPos], szBuf, sizeof(szBuf) - 1);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nOuterHeight - textSz.cy - 4;
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	nCost = costFromSubTool[CITY_MENUTOOL_POS(nPos + REWARDS_ARCOLOGIES_PLYMOUTH, CITYTOOL_GROUP_REWARDS)];
	// One oddity here with the currency string is
	// that it'll move horizontally (observed prior
	// to any changes as well). This issue is greatly
	// mitigated by it now being on a separate line,
	// so it'll no longer overlap with the MW read-out.
	textSz.cx = Game_GetAvailableFunds(pDC, nCost);
	nY = nY - textSz.cy;
	crTextOld = crDlgColBtnText;
	if (nCost > dwCityFunds)
		crTextOld = SetTextColor(pDC->m_hDC, RGB(255, 0, 0));
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	Game_DisplayItemCost(pDC, nCost);
	SetTextColor(pDC->m_hDC, crTextOld);
	pTileStr = pTileNames[nPos + TILE_ARCOLOGY_PLYMOUTH];
	if (!pTileStr)
		strcpy_s(szBuf, cityToolGroupStrings[CITY_MENUTOOL_POS(nPos + REWARDS_ARCOLOGIES_PLYMOUTH, CITYTOOL_GROUP_REWARDS)].m_pchData);
	else
		strcpy_s(szBuf, pTileStr);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nY - textSz.cy;
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	SetTextAlign(pDC->m_hDC, TA_NOUPDATECP);
	nImageX = (nInnerWidth - pThis->dwSADPoints[nPos].x - 1) >> 1;
	nImageY = (nInnerHeight - pThis->dwSADPoints[nPos].y) >> 1;
	Game_Graphics_SetColorTableFromApplicationPalette(pThis->dwSADCGraphicsOne[nPos]);
	Game_Graphics_BitBlit(pThis->dwSADCGraphicsOne[nPos], lpDIS->hDC, nImageX + 1, nImageY - 12, pThis->dwSADPoints[nPos].x, pThis->dwSADPoints[nPos].y, 0, 0);
	L_SetButtonShape_SC2K1996(lpDIS->hDC, nInnerWidth, nInnerHeight, dwState);
	SelectFont(pDC->m_hDC, hFont);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

extern "C" void __stdcall Hook_SelectArcologyDialog_SetCursorDeleteGraphics() {
	CSelectArcologyDialog *pThis;

	__asm mov [pThis], ecx

	Game_GameDialog_SetCursor(pThis);
	for (__int16 nItem = 0; nItem < ARCOLOGY_COUNT; ++nItem) {
		if (pThis->dwSADCGraphicsOne[nItem]) {
			pThis->dwSADCGraphicsOne[nItem]->DeleteStored_SC2K1996();
			delete pThis->dwSADCGraphicsOne[nItem];
			pThis->dwSADCGraphicsOne[nItem] = 0;
		}
	}
}

void InstallArcologyDialogHooks_SC2K1996(void) {
	// Adjust the dialog resource ID (113 - 0x71)
	SafeVirtualProtect((LPVOID)0x47B9A5, 1, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x47B9A5, 0x71, 1);

	// Adjust the message map referenced IDs.
	SafeVirtualProtect((LPVOID)0x4DD0B0, 360, PAGE_EXECUTE_READWRITE);
	MFC3X_AFX_MSGMAP_ENTRY *pAMM = (MFC3X_AFX_MSGMAP_ENTRY *)(0x4DD0B0);
	pAMM[4].nID = pAMM[4].nLastID = dwArcologyBtnIDs[2];
	pAMM[5].nID = pAMM[5].nLastID = dwArcologyInfoBtnIDs[2];
	pAMM[6].nID = pAMM[6].nLastID = dwArcologyBtnIDs[1];
	pAMM[7].nID = pAMM[7].nLastID = dwArcologyInfoBtnIDs[1];
	pAMM[8].nID = pAMM[8].nLastID = dwArcologyBtnIDs[3];
	pAMM[9].nID = pAMM[9].nLastID = dwArcologyInfoBtnIDs[3];
	pAMM[10].nID = pAMM[10].nLastID = dwArcologyBtnIDs[0];
	pAMM[11].nID = pAMM[11].nLastID = dwArcologyInfoBtnIDs[0];

	// Hook CSelectArcologyDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x402572, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402572, Hook_SelectArcologyDialog_OnInitDialog);

	// Hook CSelectArcologyDialog::OnDrawItem
	SafeVirtualProtect((LPVOID)0x401703, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401703, Hook_SelectArcologyDialog_OnDrawItem);

	// Hook CSelectArcologyDialog::OnDrawEntire
	SafeVirtualProtect((LPVOID)0x401901, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401901, Hook_SelectArcologyDialog_OnDrawEntire);

	// Hook CSelectArcologyDialog::SetCursorDeleteGraphics
	SafeVirtualProtect((LPVOID)0x401EAB, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401EAB, Hook_SelectArcologyDialog_SetCursorDeleteGraphics);
}
