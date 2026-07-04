// sc2kfix hooks/hook_powerplantdialog.cpp: hooks for the powerplant dialog
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

static char *pPowerPlantCostStrs[POWERPLANT_COUNT];

extern "C" int __stdcall Hook_PowerPlantDialog_OnInitDialog() {
	CPowerPlantDialog *pThis;

	__asm mov [pThis], ecx

	__int16 nItem;
	int nCost;
	HFONT hFont;
	HWND hDlgItem;
	RECT borderRect, cancelRect, dlgWndRect, dlgClientRect, dlgRect, sectRect;
	int nPadding, nWidth;
	HBRUSH hBrush;
	CGraphics *pGraphics;
	CMFC3XDC *pDC;
	void *vBits;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	pThis->dwPPDSelectedPowerPlant = -1;
	for (nItem = 0; nItem < POWERPLANT_COUNT; ++nItem) {
		pThis->dwPPDCGraphics[nItem] = 0;
		pPowerPlantCostStrs[nItem] = 0;
	}
	hFont = CreateFont(8, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	pThis->dwPPDCFont.m_hObject =  hFont;
	wDlgNumAvailablePlants = 0;
	for (nItem = 0; nItem < POWERPLANT_COUNT; ++nItem) {
		if (((1 << nItem) & wGrantedPowerPlants) != 0) {
			wDlgAvailablePlants[wDlgNumAvailablePlants] = nItem;
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwPowerPlantBtnIDs[wDlgNumAvailablePlants]);
			ShowWindow(hDlgItem, SW_NORMAL);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwPowerPlantInfoBtnIDs[wDlgNumAvailablePlants]);
			ShowWindow(hDlgItem, SW_NORMAL);

			nCost = costFromSubTool[CITY_MENUTOOL_POS(nItem + POWER_PLANTS_COAL, CITYTOOL_GROUP_POWER)];
			pPowerPlantCostStrs[nItem] = L_GetCurrencyString_SC2K1996(nCost);

			++wDlgNumAvailablePlants;
		}
 	}
	GetWindowRect(pThis->dwPPDButtonBorder.m_hWnd, &borderRect);
	GetWindowRect(pThis->dwPPDButtonCancel.m_hWnd, &cancelRect);
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	nPadding = 3;
	nWidth = dlgWndRect.right +
		dlgClientRect.left - dlgClientRect.right -
		dlgWndRect.left +
		(((borderRect.right - borderRect.left) + nPadding) * nPadding);
	SetRect(&dlgRect,
		0,
		0,
		nWidth,
		(cancelRect.bottom + dlgWndRect.bottom +
		dlgClientRect.top - dlgClientRect.bottom -
		dlgWndRect.top +
		(borderRect.bottom - borderRect.top) * ((wDlgNumAvailablePlants + 2) / 3) -
		cancelRect.top));
	SetWindowPos(pThis->m_hWnd, 0, dlgRect.top, dlgRect.left, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, SWP_NOZORDER|SWP_NOACTIVATE);

	crDlgColBtnFace = GetSysColor(COLOR_BTNFACE);
	crDlgColBtnShadow = GetSysColor(COLOR_BTNSHADOW);
	crDlgColBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
	crDlgColBtnText = GetSysColor(COLOR_BTNTEXT);
	crDlgColWndFrame = GetSysColor(COLOR_WINDOWFRAME);
	hBrush = CreateSolidBrush(crDlgColBtnFace);

	for (nItem = 0; nItem < wDlgNumAvailablePlants; ++nItem) {
		POINT pt;
		__int16 wCurrPlant = wDlgAvailablePlants[nItem];
		__int16 nSpriteID = wPowerPlantSpriteIDs[wCurrPlant];

		pThis->dwPPDBtnsID[nItem] = dwPowerPlantBtnIDs[wCurrPlant];
		pt.x = pArrSpriteHeaders[nSpriteID].wWidth;
		pt.y = pArrSpriteHeaders[nSpriteID].wHeight + 1;

		pGraphics = new CGraphics();
		if (pGraphics)
			pGraphics = Game_Graphics_Cons(pGraphics);
		else
			pGraphics = 0;
		pThis->dwPPDCGraphics[nItem] = pGraphics;
		pThis->dwPPDCGraphics[nItem]->CreateWithPalette_SC2K1996(pt.x, pt.y);

		pt.x = Game_Graphics_WidthBytes(pThis->dwPPDCGraphics[nItem]);
		pt.y = Game_Graphics_Height(pThis->dwPPDCGraphics[nItem]);

		pThis->dwPPDPoint[nItem].x = pt.x;
		pThis->dwPPDPoint[nItem].y = pt.y;

		pDC = Game_Graphics_GetDC(pThis->dwPPDCGraphics[nItem]);
		vBits = Game_Graphics_LockDIBBits(pThis->dwPPDCGraphics[nItem]);
		SetRect(&sectRect, 0, 0, pt.x, pt.y);
		FillRect(pDC->m_hDC, &sectRect, hBrush);

		Game_BeginProcessObjects(pThis, vBits, pt.x, (__int16)pt.y, &dlgClientRect);
		L_drawShapeDialog_SC2K1996(nSpriteID, 0, 0, 0, 0);
		Game_FinishProcessObjects();
		Game_Graphics_ReleaseDC(pThis->dwPPDCGraphics[nItem], pDC);

		Game_Graphics_RemapBitmapColorsFromSimcityAppPalette(pThis->dwPPDCGraphics[nItem]);
		pThis->dwPPDBtnsState[nItem] = 0;
	}
	DeleteObject(hBrush);

	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);
	return 1;
}

extern "C" void __stdcall Hook_PowerPlantDialog_OnDrawEntire(int nPos, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	CPowerPlantDialog *pThis;

	__asm mov [pThis], ecx

	HWND hDlgItem;
	HDC hDlgDC;
	CMFC3XPaintDC *pDC;
	RECT rcDest;
	DWORD dwState;
	int nOuterWidth, nOuterHeight;
	int nOuterHalfWidth;
	int nInnerWidth, nInnerHeight;
	int nAvailablePlant;
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
	dwState = pThis->dwPPDBtnsState[nPos];
	nOuterWidth = rcDest.right - rcDest.left;
	nOuterHeight = rcDest.bottom - rcDest.top;
	nOuterHalfWidth = nOuterWidth / 2;
	nInnerWidth = nOuterWidth - 2;
	nInnerHeight = nOuterHeight - 2;
	nAvailablePlant = wDlgAvailablePlants[nPos];
	cr = (dwState == TBBS_CHECKED) ? crDlgColBtnShadow : crDlgColBtnFace;
	L_SetRectBackground_SC2K1996(lpDIS->hDC, 1, 1, nInnerWidth, nInnerHeight, cr);
	SetTextColor(pDC->m_hDC, crDlgColBtnText);
	SetBkColor(pDC->m_hDC, crDlgColBtnFace);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	hFont = SelectFont(pDC->m_hDC, pThis->dwPPDCFont.m_hObject);
	sprintf_s(szBuf, "%d%s", wPowerPlantMWs[nAvailablePlant], aMw);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nOuterHeight - textSz.cy - 4;
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	nCost = costFromSubTool[CITY_MENUTOOL_POS(nAvailablePlant + POWER_PLANTS_COAL, CITYTOOL_GROUP_POWER)];
	memset(szBuf, 0, sizeof(szBuf));
	if (pPowerPlantCostStrs[nAvailablePlant])
		strcpy_s(szBuf, pPowerPlantCostStrs[nAvailablePlant]);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nY - textSz.cy;
	crTextOld = crDlgColBtnText;
	if (nCost > dwCityFunds)
		crTextOld = SetTextColor(pDC->m_hDC, RGB(255, 0, 0));
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	SetTextColor(pDC->m_hDC, crTextOld);
	strcpy_s(szBuf, cityToolGroupStrings[CITY_MENUTOOL_POS(nAvailablePlant + POWER_PLANTS_COAL, CITYTOOL_GROUP_POWER)].m_pchData);
	GetTextExtentPointA(pDC->m_hAttribDC, szBuf, strlen(szBuf), &textSz);
	nY = nY - textSz.cy;
	MoveToEx(pDC->m_hDC, nOuterHalfWidth - textSz.cx / 2, nY, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szBuf, strlen(szBuf));
	SetTextAlign(pDC->m_hDC, TA_NOUPDATECP);
	nImageX = (nInnerWidth - pThis->dwPPDPoint[nPos].x - 1) >> 1;
	nImageY = (nInnerHeight - pThis->dwPPDPoint[nPos].y) >> 1;
	Game_Graphics_SetColorTableFromApplicationPalette(pThis->dwPPDCGraphics[nPos]);
	Game_Graphics_BitBlit(pThis->dwPPDCGraphics[nPos], lpDIS->hDC, nImageX + 1, nImageY - 12, pThis->dwPPDPoint[nPos].x, pThis->dwPPDPoint[nPos].y, 0, 0);
	if (dwState == TBBS_CHECKED)
		InvertRect(pDC->m_hDC, &rcDest);
	L_SetButtonShape_SC2K1996(lpDIS->hDC, nInnerWidth, nInnerHeight, dwState);
	SelectFont(pDC->m_hDC, hFont);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

extern "C" void __stdcall Hook_PowerPlantDialog_SetCursorDeleteGraphics() {
	CPowerPlantDialog *pThis;

	__asm mov [pThis], ecx

	Game_GameDialog_SetCursor(pThis);
	for (__int16 nItems = 0; nItems < wDlgNumAvailablePlants; ++nItems) {
		if (pPowerPlantCostStrs[nItems]) {
			free(pPowerPlantCostStrs[nItems]);
			pPowerPlantCostStrs[nItems] = 0;
		}
		if (pThis->dwPPDCGraphics[nItems]) {
			pThis->dwPPDCGraphics[nItems]->DeleteStored_SC2K1996();
			delete pThis->dwPPDCGraphics[nItems];
			pThis->dwPPDCGraphics[nItems] = 0;
		}
	}
}

void InstallPowerPlantDialogHooks_SC2K1996(void) {
	// MW
	SafeVirtualProtect((LPVOID)0x4E6E96, 1, PAGE_EXECUTE_READWRITE);
	*(BYTE*)0x4E6E96 = 'W';

	// Hook CPowerPlantDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x401889, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401889, Hook_PowerPlantDialog_OnInitDialog);

	// Hook CPowerPlantDialog::OnDrawEntire
	SafeVirtualProtect((LPVOID)0x402194, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402194, Hook_PowerPlantDialog_OnDrawEntire);

	// Hook CPowerPlantDialog::SetCursorDeleteGraphics
	SafeVirtualProtect((LPVOID)0x403008, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403008, Hook_PowerPlantDialog_SetCursorDeleteGraphics);
}
