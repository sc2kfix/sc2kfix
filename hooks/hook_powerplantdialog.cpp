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

extern "C" int __stdcall Hook_PowerPlantDialog_OnInitDialog() {
	CPowerPlantDialog *pThis;

	__asm mov [pThis], ecx

	__int16 nItem;
	HFONT hFont;
	HWND hDlgItem;
	RECT infoRect, cancelRect, dlgWndRect, dlgClientRect, dlgRect, sectRect;
	HBRUSH hBrush;
	CGraphics *pGraphics;
	CMFC3XDC *pDC;
	void *vBits;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	pThis->dwPPDSelectedPowerPlant = -1;
	for (nItem = 0; nItem < POWERPLANT_COUNT; ++nItem)
		pThis->dwPPDCGraphics[nItem] = 0;
	hFont = CreateFont(8, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "MS Sans Serif");
	pThis->dwPPDCFont.m_hObject =  hFont;
	wDlgNumAvailablePlants = 0;
	for (nItem = 0; nItem < POWERPLANT_COUNT; ++nItem) {
		if (((1 << nItem) & wGrantedPowerPlants) != 0) {
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwPowerPlantBtnIDs[wDlgNumAvailablePlants]);
			ShowWindow(hDlgItem, SW_NORMAL);

			hDlgItem = GetDlgItem(pThis->m_hWnd, dwPowerPlantInfoBtnIDs[wDlgNumAvailablePlants]);
			ShowWindow(hDlgItem, SW_NORMAL);
			wDlgAvailablePlants[wDlgNumAvailablePlants] = nItem;
			++wDlgNumAvailablePlants;
		}
		//ConsoleLog(LOG_DEBUG, "(%d) (%d, %d)\n", nItem, pThis->dwPPDPoint[nItem].x, pThis->dwPPDPoint[nItem].y);
 	}
	GetWindowRect(pThis->dwPPDButtonInfo.m_hWnd, &infoRect);
	GetWindowRect(pThis->dwPPDButtonCancel.m_hWnd, &cancelRect);
	GetWindowRect(pThis->m_hWnd, &dlgWndRect);
	GetClientRect(pThis->m_hWnd, &dlgClientRect);
	SetRect(&dlgRect,
		0,
		0,
		dlgWndRect.right - dlgWndRect.left,
		(cancelRect.bottom + dlgWndRect.bottom +
		dlgClientRect.top - dlgClientRect.bottom -
		dlgWndRect.top +
		(infoRect.bottom - infoRect.top) * ((wDlgNumAvailablePlants + 2) / 3) -
		cancelRect.top));
	SetWindowPos(pThis->m_hWnd, 0, dlgRect.top, dlgRect.left, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, SWP_NOZORDER|SWP_NOACTIVATE);

	crPowerPlantColBtnFace = GetSysColor(COLOR_BTNFACE);
	crPowerPlantColBtnShadow = GetSysColor(COLOR_BTNSHADOW);
	crPowerPlantColBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
	crPowerPlantColBtnText = GetSysColor(COLOR_BTNTEXT);
	crPowerPlantColWndFrame = GetSysColor(COLOR_WINDOWFRAME);
	hBrush = CreateSolidBrush(crPowerPlantColBtnFace);

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

extern "C" void __stdcall Hook_PowerPlantDialog_SetCursorDeleteGraphics() {
	CPowerPlantDialog *pThis;

	__asm mov [pThis], ecx

	Game_GameDialog_SetCursor(pThis);
	for (__int16 nItems = 0; nItems < wDlgNumAvailablePlants; ++nItems) {
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

	// Hook CPowerPlantDialog::SetCursorDeleteGraphics
	SafeVirtualProtect((LPVOID)0x403008, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403008, Hook_PowerPlantDialog_SetCursorDeleteGraphics);
}
