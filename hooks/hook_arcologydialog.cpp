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

extern "C" int __stdcall Hook_SelectArcologyDialog_OnInitDialog() {
	CSelectArcologyDialog *pThis;

	__asm mov [pThis], ecx

	__int16 nItem;
	HFONT hFont;
	HWND hDlgItem;
	RECT dlgWndRect, dlgClientRect, cancelRect, sectRect;
	int nWidth, nCancelX, nCancelY;
	HBRUSH hBrush;
	CGraphics *pGraphics;
	CMFC3XDC *pDC;
	void *vBits;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	pThis->dwSADArcologySelection = -1;
	for (nItem = 0; nItem < ARCOLOGY_COUNT; ++nItem)
		pThis->dwSADCGraphicsOne[nItem] = 0;
	hFont = CreateFont(8, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	pThis->dwSADCFont.m_hObject =  hFont;
	for (nItem = 0; nItem < wGrantedArcologies; ++nItem) {
		ShowWindow(GetDlgItem(pThis->m_hWnd, dwArcologyBtnIDs[nItem]), SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(pThis->m_hWnd, dwArcologyInfoBtnIDs[nItem]), SW_SHOWNORMAL);
	}
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	nWidth = wGrantedArcologies * ((dlgClientRect.right - dlgClientRect.left) / ARCOLOGY_COUNT) + 
		dlgClientRect.left + dlgWndRect.right -
		dlgWndRect.left - dlgClientRect.right;
	SetWindowPos(pThis->m_hWnd, 0, 0, 0, nWidth, dlgWndRect.bottom - dlgWndRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hDlgItem = GetDlgItem(pThis->m_hWnd, IDCANCEL);
	GetWindowRect(hDlgItem, &cancelRect);
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	nCancelX = ((cancelRect.left + dlgWndRect.right - dlgWndRect.left - cancelRect.right) / 2) - GetSystemMetrics(SM_CXDLGFRAME);
	nCancelY = cancelRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME) - dlgWndRect.top;
	SetWindowPos(hDlgItem, 0, nCancelX, nCancelY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	crArcologyColBtnFace = GetSysColor(COLOR_BTNFACE);
	crArcologyColBtnShadow = GetSysColor(COLOR_BTNSHADOW);
	crArcologyColBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
	crArcologyColBtnText = GetSysColor(COLOR_BTNTEXT);
	crArcologyColWndFrame = GetSysColor(COLOR_WINDOWFRAME);
	hBrush = CreateSolidBrush(crArcologyColBtnFace);
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
	// Hook CSelectArcologyDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x402572, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402572, Hook_SelectArcologyDialog_OnInitDialog);

	// Hook CSelectArcologyDialog::SetCursorDeleteGraphics
	SafeVirtualProtect((LPVOID)0x401EAB, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401EAB, Hook_SelectArcologyDialog_SetCursorDeleteGraphics);
}
