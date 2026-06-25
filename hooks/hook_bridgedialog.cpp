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
	RECT imageRect, sectRect, dlgRect;
	__int16 nBridgeTypeBit;
	CGraphics *pGraphic;
	CMFC3XDC *pDC;
	CMFC3XWnd *pWnd;

	GameMain_Dialog_OnInitDialog(pThis);
	for (nItem = 0; nItem < BRIDGE_COUNT; ++nItem)
		pThis->dwBSDCGraphicsOne[nItem] = 0;
	hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeImageSurfaceID[0]);
	GetWindowRect(hDlgItem, &imageRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.right);
	pThis->dwBSDWidth = (imageRect.right - imageRect.left + 7) & ~7;
	pThis->dwBSDHeight = (imageRect.bottom - imageRect.top + 7) & ~7;
	nItem = 0;
	wDlgNumAvailableBridges = 0;
	for (nBridgeTypeBit = 1; nBridgeTypeBit <= 64; nBridgeTypeBit *= 2) {
		if ((nBridgeTypeBit & pThis->dwBSDBridgeType) != 0) {
			wDlgAvailableBridges[wDlgNumAvailableBridges] = nItem;
			pGraphic = new CGraphics();
			if (pGraphic)
				pGraphic = Game_Graphics_Cons(pGraphic);
			else
				pGraphic = 0;
			pThis->dwBSDCGraphicsOne[wDlgNumAvailableBridges] = pGraphic;
			pThis->dwBSDCGraphicsOne[wDlgNumAvailableBridges]->CreateWithPalette_SC2K1996(pThis->dwBSDWidth, pThis->dwBSDHeight);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeImageSurfaceID[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOW);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeSelectBtnID[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOW);
			hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeInfoBtnID[wDlgNumAvailableBridges]);
			ShowWindow(hDlgItem, SW_SHOW);

			++wDlgNumAvailableBridges;
		}
		++nItem;
	}
	GetWindowRect(pThis->m_hWnd, &sectRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&sectRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&sectRect.right);
	if (wDlgNumAvailableBridges >= 3)
		SetRect(&dlgRect, 0, 0, sectRect.right - sectRect.left, (imageRect.bottom - imageRect.top) * ((wDlgNumAvailableBridges + 2) / 3) + 64);
	else
		SetRect(&dlgRect, 0, 0, wDlgNumAvailableBridges * (imageRect.right - imageRect.left) + 34, (imageRect.bottom - imageRect.top) * ((wDlgNumAvailableBridges + 2) / 3) + 64);
	SetWindowPos(pThis->m_hWnd, 0, dlgRect.top, dlgRect.left, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, SWP_NOZORDER|SWP_NOACTIVATE);
	for (nItem = 0; nItem < wDlgNumAvailableBridges; ++nItem) {
		vBridgeBits = Game_Graphics_LockDIBBits(pThis->dwBSDCGraphicsOne[nItem]);
		pDC = Game_Graphics_GetDC(pThis->dwBSDCGraphicsOne[nItem]);
		imageRect.left = 0;
		imageRect.top = 0;
		imageRect.right = pThis->dwBSDWidth;
		imageRect.bottom = pThis->dwBSDHeight;
		FillRect(pDC->m_hDC, &imageRect, (HBRUSH)MainBrushFace->m_hObject);
		Game_Graphics_ReleaseDC(pThis->dwBSDCGraphicsOne[nItem], pDC);
		Game_BeginProcessObjects(pThis, vBridgeBits, pThis->dwBSDWidth, (__int16)pThis->dwBSDHeight, &dlgRect);
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
	}

	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);
	return 1;
}

extern "C" void __stdcall Hook_BridgeSelectDialog_OnPaint() {
	CBridgeSelectDialog *pThis;

	__asm mov [pThis], ecx

	CMFC3XPaintDC paintDC;
	HWND hDlgItem;
	RECT imageRect;
	CGraphics *pGraphic;
	CMFC3XDC *pDC;
	char szResBuf[255 + 1];
	POINT pt;

	GameMain_PaintDC_Cons(&paintDC, pThis);
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);
	SetBkColor(paintDC.m_hDC, GetSysColor(COLOR_BTNFACE));
	SetTextColor(paintDC.m_hDC, GetSysColor(COLOR_BTNTEXT));
	SelectFont(paintDC.m_hDC, hFontMSSansSerifRegular10);
	for (__int16 nItem = 0; nItem < wDlgNumAvailableBridges; ++nItem) {
		__int16 nAvailableBridge = wDlgAvailableBridges[nItem];
		hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeImageSurfaceID[nItem]);
		GetWindowRect(hDlgItem, &imageRect);
		ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.left);
		ScreenToClient(pThis->m_hWnd, (LPPOINT)&imageRect.right);
		pGraphic = pThis->dwBSDCGraphicsOne[nItem];
		pDC = Game_Graphics_GetDC(pGraphic);
		Game_Graphics_SetColorTableFromApplicationPalette(pGraphic);
		Game_Graphics_Paint(pGraphic, paintDC.m_hDC, imageRect.left, imageRect.top);
		Game_Graphics_ReleaseDC(pGraphic, pDC);
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, dwBridgeStringID[nAvailableBridge], szResBuf, sizeof(szResBuf) - 1);
		hDlgItem = GetDlgItem(pThis->m_hWnd, dwBridgeSelectBtnID[nItem]);
		SetWindowTextA(hDlgItem, szResBuf);
		MoveToEx(paintDC.m_hDC, imageRect.left + 20, imageRect.bottom - 42, &pt);
		Game_DisplayItemCost(&paintDC, pThis->dwBSDNumTiles * dwBridgeBaseCost[nAvailableBridge]);
	}
	SetTextAlign(paintDC.m_hDC, TA_NOUPDATECP);
	GameMain_PaintDC_Dest(&paintDC);
}

extern "C" void __stdcall Hook_BridgeSelectDialog_SetCursorAndDeleteGraphics() {
	CBridgeSelectDialog *pThis;

	__asm mov [pThis], ecx

	int nItem;

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
