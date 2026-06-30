// sc2kfix hooks/hook_querydialog.cpp: hook for new query dialog features
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// XXX: This code is Not Good and has done some bad stuff on certain versions of Windows 10. I'm
// not entirely sure which versions are afflicted with Problems exacerbated by it but there are at
// least two different reports of machines (out of a few hundred users as of writing) where the
// game crashes immediately on launch with a nonsensical stack trace if the advanced query dialog
// is enabled. Rewriting it is extremely low priority though since it's mostly for save file/map
// debugging, and it works on my machine, so it's been changed as of Release 9c to be an opt-in
// hook via the `-advquery` command-line option.

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

extern BOOL PrepareDialogSpriteGraphic_SC2K1996(CGraphics *pGraphic, HWND hWnd, sprite_header_t *pSprHead, __int16 nSpriteID, CMFC3XRect *pDlgRect, __int16 isFlipped = 0, __int16 doInvert = 0, int nType = PALCACHE_TYPE_NONE);
extern void ShowCurrentDialogSpriteGraphic_SC2K1996(CGraphics *pGraphic, HWND hWnd, sprite_header_t *pSprHead, __int16 nSpriteID, CMFC3XRect *pDlgRect, BOOL bSpriteFail, __int16 isFlipped = 0, __int16 doInvert = 0, int nType = PALCACHE_TYPE_NONE);

typedef struct {
	WORD iTileX;
	WORD iTileY;
} query_coords_info;

static int GetQueriedSpriteIDFromCoords(WORD x, WORD y) {
	__int16 iTileID, iTextOverlay;
	int iSpriteID = -1;
	
	iTileID = GetTileID(x, y);
	if (!iTileID) {
		iTileID = GetTerrainTileID(x, y);
		iTileID = nXTERTileIDs[iTileID];
	}
	//else {
	//	if (iTileID >= TILE_ARCOLOGY_PLYMOUTH) {
	//		// Positioning falls into the "medium" range.
	//		iSpriteID = iTileID + SPRITE_MEDIUM_START;
	//	}
	//}	

	if (iSpriteID < 0) {
		// Positioning falls into the "large" range.
		iSpriteID = iTileID + SPRITE_LARGE_START;
	}

	iTextOverlay = XTXTGetTextOverlayID(x, y);
	if (iTextOverlay) {
		if (iTextOverlay >= MIN_XTHG_TEXT_ENTRIES &&
			iTextOverlay <= MAX_XTHG_TEXT_ENTRIES &&
			XTHGGetType(XTHGID_ENTRY(iTextOverlay)) == XTHG_SAILBOAT)
			iSpriteID = SPRITE_LARGE_SAILBOAT_NE;
	}

	return iSpriteID;
}

static BYTE GetPertinentRsrcIDOffset(WORD x, WORD y) {
	BYTE iRsrcOffset, iTextOverlay;

	// Initially iRsrcOffset is the XBLD TileID.
	// Based on the initial building (or not) it will
	// then calculate the Resource ID offset that is
	// then passed passed to LoadNamedEntryFromRsrcOffset().

	iRsrcOffset = GetTileID(x, y);
	if (iRsrcOffset >= TILE_COMMERCIAL_1X1_GASSTATION1)
		iRsrcOffset -= 102;
	else {
		BOOL bNoUpdate = FALSE;
		BYTE nInfraTile = 0;
		while (bInfraTile[nInfraTile] <= iRsrcOffset) {
			if (++nInfraTile >= 22) {
				bNoUpdate = TRUE;
				break;
			}
		}
		if (!bNoUpdate)
			iRsrcOffset = nInfraTile;
	}

	if (!iRsrcOffset) {
		iRsrcOffset = 158;
		if (x < GAME_MAP_SIZE &&
			y < GAME_MAP_SIZE &&
			XBITReturnIsWater(x, y)) {
			iRsrcOffset = 160;
			if (!XBITReturnIsSaltWater(x, y))
				iRsrcOffset = 159;
			iTextOverlay = XTXTGetTextOverlayID(x, y);
			if (iTextOverlay) {
				if (iTextOverlay >= MIN_XTHG_TEXT_ENTRIES &&
					iTextOverlay <= MAX_XTHG_TEXT_ENTRIES &&
					XTHGGetType(XTHGID_ENTRY(iTextOverlay)) == XTHG_SAILBOAT)
					iRsrcOffset = 161;
			}
		}
	}
	return iRsrcOffset;
}

BOOL CALLBACK AdvancedQueryDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	query_coords_info *qci;
	WORD iTileX, iTileY;
	std::string strTileHeader;
	std::string strTileInfo;
	BYTE iTextOverlay, iRsrcOffset, iTileID;
	CMFC3XRect dlgRect;
	PAINTSTRUCT ps;
	sprite_header_t *pSprHead;
	const char *pSrc;
	char szBuf[80 + 1];
	int x, y;
	static int nSpriteID = -1;
	static CGraphics *pQueriedTileImage = NULL;
	static BOOL bSpriteFail = FALSE;

	switch (message) {
	case WM_INITDIALOG:
		hWndExt = hwndDlg;

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		qci = (query_coords_info *)lParam;

		GetWindowRect(hwndDlg, &dlgRect);

		pQueriedTileImage = new CGraphics();

		ScreenToClient(hwndDlg, (LPPOINT)&dlgRect);
		ScreenToClient(hwndDlg, (LPPOINT)&dlgRect.right);

		iTileX = qci->iTileX;
		iTileY = qci->iTileY;

		iTextOverlay = XTXTGetTextOverlayID(iTileX, iTileY);
		if (iTextOverlay == 111)
			Game_RecalculateMayorsHouseStats();

		memset(szBuf, 0, sizeof(szBuf));

		nSpriteID = GetQueriedSpriteIDFromCoords(iTileX, iTileY);
		if (nSpriteID >= 0) {
			iRsrcOffset = GetPertinentRsrcIDOffset(iTileX, iTileY);
			pSrc = pCustomTileNamesFromSpriteID[nSpriteID];
			if (pSrc)
				strcpy_s(szBuf, sizeof(szBuf) - 1, pSrc);
			else
				Game_LoadNamedEntryFromRsrcOffset(szBuf, 2000, iRsrcOffset);
			bSpriteFail = PrepareDialogSpriteGraphic_SC2K1996(pQueriedTileImage, hwndDlg, &pArrSpriteHeaders[nSpriteID], nSpriteID, &dlgRect);
		}

		if (iTextOverlay >= MIN_SIM_TEXT_ENTRIES && iTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
			iTileID = GetMicroSimulatorTileID(MICROSIMID_ENTRY(iTextOverlay));
			if (iTileID) {
				pSrc = GetXLABEntry(iTextOverlay);
				if (pSrc)
					strcpy_s(szBuf, sizeof(szBuf) - 1, pSrc);
			}
		}

		// Build the header string
		iTileID = GetTileID(iTileX, iTileY);
		if (iTileID == TILE_CLEAR) {
			if (XBITReturnIsWater(iTileX, iTileY) && XBITReturnIsSaltWater(iTileX ,iTileY))
				strTileHeader = "Salt water";
			else if (XBITReturnIsWater(iTileX, iTileY))
				strTileHeader = "Fresh water";
		} else
			strTileHeader = szTileNames[iTileID];

		strTileHeader += "  (iTileID ";
		strTileHeader += std::to_string(iTileID);
		strTileHeader += " / ";
		strTileHeader += HexPls(iTileID, 2);
		strTileHeader += " )\n";

		if (szBuf[0] != 0) {
			strTileHeader += szBuf;
			strTileHeader += "  ";
		}
			
		strTileHeader += "(nSpriteID ";
		strTileHeader += std::to_string(nSpriteID);
		strTileHeader += " / ";
		strTileHeader += HexPls(nSpriteID, 4);
		strTileHeader += " )\nCoordinates:   X=";
		strTileHeader += std::to_string(iTileX);
		strTileHeader += "  Y=";
		strTileHeader += std::to_string(iTileY);

		// Build the data string
		strTileInfo =  GetZoneName(XZONReturnZone(iTileX, iTileY));
		strTileInfo += "\n";
		
		// Altitude/depth
		if (XBITReturnIsWater(iTileX, iTileY) && ALTMReturnLandAltitude(iTileX, iTileY) < wWaterLevel)
			strTileInfo += std::to_string(100 * (wWaterLevel - ALTMReturnLandAltitude(iTileX, iTileY)) - 50);
		else if (GetTerrainTileID(iTileX, iTileY) && GetTerrainTileID(iTileX, iTileY) < SUBMERGED_00)
			strTileInfo += std::to_string(25 * (4 * (ALTMReturnLandAltitude(iTileX, iTileY) - wWaterLevel) + 4));
		else
			strTileInfo += std::to_string(100 * (ALTMReturnLandAltitude(iTileX, iTileY) - wWaterLevel) + 50);
		strTileInfo += " feet ";
		if (XBITReturnIsWater(iTileX, iTileY) && ALTMReturnLandAltitude(iTileX, iTileY) < wWaterLevel)
			strTileInfo += "deep ";
		strTileInfo += "(ALTM: ";
		strTileInfo += HexPls(ALTMReturnMask(iTileX, iTileY), 4);
		strTileInfo += ")\n";

		// Land value
		strTileInfo += "$";
		strTileInfo += std::to_string(GetXVALByteDataWithNormalCoordinates(iTileX, iTileY) + 1);
		strTileInfo += ",000/acre\n";

		// Crime
		strTileInfo += GetLowHighScale(GetXCRMByteDataWithNormalCoordinates(iTileX, iTileY));
		strTileInfo += " (XCRM: ";
		strTileInfo += std::to_string(GetXCRMByteDataWithNormalCoordinates(iTileX, iTileY));
		strTileInfo += ")\n";

		// Pollution
		strTileInfo += GetLowHighScale(GetXPLTByteDataWithNormalCoordinates(iTileX, iTileY));
		strTileInfo += " (XPLT: ";
		strTileInfo += std::to_string(GetXPLTByteDataWithNormalCoordinates(iTileX, iTileY));
		strTileInfo += ")\n\n";

		// Raw XZON data
		switch (XZONReturnCornerMask(iTileX, iTileY)) {
		case CORNER_NONE:
			strTileInfo += "No corners";
			break;
		case CORNER_BLEFT:
			strTileInfo += "Bottom-left corner";
			break;
		case CORNER_BRIGHT:
			strTileInfo += "Bottom-right corner";
			break;
		case CORNER_TLEFT:
			strTileInfo += "Top-left corner";
			break;
		case CORNER_TRIGHT:
			strTileInfo += "Top-right corner";
			break;
		case CORNER_ALL:
			strTileInfo += "All four corners";
			break;
		}
		strTileInfo += ", iZoneID ";
		strTileInfo += HexPls(XZONReturnZone(iTileX, iTileY), 1);
		strTileInfo += "\n";

		// XBIT
		if (!XBITReturnMask(iTileX, iTileY))
			strTileInfo += "none (XBIT: 0x00)\n\n";
		else {
			// XXX - this code sucks, more so than the rest of this function
			int i = 0;

			if (XBITReturnIsPowerable(iTileX, iTileY)) {
				i++;
				strTileInfo += "powerable ";
			}
			if (XBITReturnIsPowered(iTileX, iTileY)) {
				i++;
				strTileInfo += "powered ";
			}
			if (XBITReturnIsPiped(iTileX, iTileY)) {
				i++;
				strTileInfo += "piped ";
			}
			if (XBITReturnIsWatered(iTileX, iTileY)) {
				i++;
				strTileInfo += "watered ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (XBITReturnIsMark(iTileX, iTileY)) {
				i++;
				strTileInfo += "mark ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (XBITReturnIsWater(iTileX, iTileY)) {
				i++;
				strTileInfo += "water ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (XBITReturnIsFlipped(iTileX, iTileY)) {
				i++;
				strTileInfo += "flipped ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (XBITReturnIsSaltWater(iTileX, iTileY)) {
				i++;
				strTileInfo += "saltwater ";
				if (i == 5)
					strTileInfo += "\n";
			}
			strTileInfo += "(XBIT: ";
			strTileInfo += HexPls(XBITReturnMask(iTileX, iTileY), 1);
			strTileInfo += ")\n";
			if (i < 5)
				strTileInfo += "\n";
		}

		// XUND
		if (GetUndergroundTileID(iTileX, iTileY) > UNDER_TILE_SUBWAYENTRANCE)
			strTileInfo += "Unknown";
		else
			strTileInfo += szUndergroundNames[GetUndergroundTileID(iTileX, iTileY)];
		strTileInfo += " (XUND: ";
		strTileInfo += HexPls(GetUndergroundTileID(iTileX, iTileY), 2);
		strTileInfo += ")\n";

		// Microsim info
		BYTE bTextOverlay;
		
		bTextOverlay = XTXTGetTextOverlayID(iTileX, iTileY); // Entries >= MIN_SIM_TEXT_ENTRIES aren't user-modifiable label/text entries.
		if (bTextOverlay >= MIN_SIM_TEXT_ENTRIES && bTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
			BYTE iMicrosimID = MICROSIMID_ENTRY(bTextOverlay); // The MicrosimID being calculated from entry MIN_SIM_TEXT_ENTRIES and beyond but subtracted by the non-user modifiable starting value.
			strTileInfo += GetXLABEntry(bTextOverlay);
			strTileInfo += " (iMicrosimID " + std::to_string(iMicrosimID) + " / " + HexPls(iMicrosimID, 2) + ")\n";
			strTileInfo += std::to_string(GetMicroSimulatorStat0(iMicrosimID)) + " / " + HexPls(GetMicroSimulatorStat0(iMicrosimID), 2) + "\n";
			strTileInfo += std::to_string(GetMicroSimulatorStat1(iMicrosimID)) + " / " + HexPls(GetMicroSimulatorStat1(iMicrosimID), 2) + "\n";
			strTileInfo += std::to_string(GetMicroSimulatorStat2(iMicrosimID)) + " / " + HexPls(GetMicroSimulatorStat2(iMicrosimID), 2) + "\n";
			strTileInfo += std::to_string(GetMicroSimulatorStat3(iMicrosimID)) + " / " + HexPls(GetMicroSimulatorStat3(iMicrosimID), 2);
		}
		else
			strTileInfo += "None\nN/A\nN/A\nN/A\nN/A";

		SetDlgItemText(hwndDlg, IDC_STATIC_TILENAME, strTileHeader.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_TILEDATA, strTileInfo.c_str());

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_PAINT:
		BeginPaint(hwndDlg, &ps);
		GetWindowRect(hwndDlg, &dlgRect);
		ScreenToClient(hwndDlg, (LPPOINT)&dlgRect);
		ScreenToClient(hwndDlg, (LPPOINT)&dlgRect.right);

		if (nSpriteID >= 0) {
			pSprHead = &pArrSpriteHeaders[nSpriteID];
			if (pSprHead) {
				x = dlgRect.right - pSprHead->wWidth - 20;
				y = (dlgRect.bottom - pSprHead->wHeight) / 2;

				ShowCurrentDialogSpriteGraphic_SC2K1996(pQueriedTileImage, hwndDlg, &pArrSpriteHeaders[nSpriteID], nSpriteID, &dlgRect, bSpriteFail);
				if (pQueriedTileImage && !bSpriteFail) {
					Game_Graphics_SetColorTableFromApplicationPalette(pQueriedTileImage);
					Game_Graphics_Paint(pQueriedTileImage, ps.hdc, x, y);
				}
			}
		}

		EndPaint(hwndDlg, &ps);
		return FALSE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}

	case WM_DESTROY:
		hWndExt = 0;
		nSpriteID = -1;
		if (pQueriedTileImage) {
			pQueriedTileImage->DeleteStored_SC2K1996();
			delete pQueriedTileImage;
			pQueriedTileImage = NULL;
		}
		break;
	}
	return FALSE;
}

static BOOL DoAdvancedQuery(__int16 x, __int16 y) {
	CSimcityAppPrimary *pSCApp;
	CMainFrame *pMainFrm;
	CCityToolBar *pCityToolBar;
	query_coords_info qci;

	pSCApp = &pCSimcityAppThis;
	pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	pCityToolBar = &pMainFrm->dwMFCityToolBar;

	if (bUseAdvancedQuery) {
		if (GetAsyncKeyState(VK_MENU) < 0) {
			memset(&qci, 0, sizeof(qci));
			qci.iTileX = x;
			qci.iTileY = y;

			pSCApp->dwSCABackgroundColourCyclingActive = TRUE;
			Game_CityToolBar_ToolMenuDisable(pCityToolBar);
			DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_ADVANCEDQUERY), GameGetRootWindowHandle(), AdvancedQueryDialogProc, (LPARAM)&qci);
			Game_CityToolBar_ToolMenuEnable(pCityToolBar);
			pSCApp->dwSCABackgroundColourCyclingActive = FALSE;
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL bSoundPlayed = FALSE;

extern "C" void __cdecl Hook_QuerySpecificItem(__int16 x, __int16 y) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSimcityView *pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	bSoundPlayed = FALSE;
	if (pSCView && wCursorActive)
		Game_SimcityView_KillCursor(pSCView);
	if (!DoAdvancedQuery(x, y))
		GameMain_QuerySpecificItem(x, y);

}

extern "C" void __cdecl Hook_QueryGeneralItem(__int16 x, __int16 y) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSimcityView *pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView && wCursorActive)
		Game_SimcityView_KillCursor(pSCView);
	if (!DoAdvancedQuery(x, y))
		GameMain_QueryGeneralItem(x, y);
}

extern "C" int __stdcall Hook_QuerySpecificDialog_OnInitDialog() {
	CQuerySpecificDialog *pThis;

	__asm mov [pThis], ecx

	CMFC3XPaintDC paintDC;
	CMFC3XWnd *pWnd;
	RECT btnTextRect;
	char szBuf[255 + 1];
	SIZE txtSz;
	int nWidth;
	int nSpriteID;

	GameMain_PaintDC_Cons(&paintDC, pThis);
	GameMain_Dialog_OnInitDialog(pThis);
	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);
	EnableWindow(pThis->dwQSDCEdit.m_hWnd, FALSE);

	// Moved here from OnInitDialog().
	GetWindowRect(pThis->dwQSDCButton.m_hWnd, &btnTextRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&btnTextRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&btnTextRect.right);

	BOOL nBtnCmdShow = SW_HIDE;
	if (pThis->dwQSDTileID == TILE_SERVICES_CITYHALL || pThis->dwQSDTileID == TILE_INFRASTRUCTURE_LIBRARY) {
		if (pThis->dwQSDTileID == TILE_SERVICES_CITYHALL)
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 810, szBuf, sizeof(szBuf) - 1);
		else
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 811, szBuf, sizeof(szBuf) - 1);
		SetWindowText(pThis->dwQSDCButton.m_hWnd, szBuf);
		GetTextExtentPointA(paintDC.m_hAttribDC, szBuf, strlen(szBuf), &txtSz);
		nWidth = txtSz.cx - btnTextRect.right + btnTextRect.left + 8;
		btnTextRect.left -= nWidth / 2;
		btnTextRect.right += nWidth / 2;
		MoveWindow(pThis->dwQSDCButton.m_hWnd, btnTextRect.left, btnTextRect.top, btnTextRect.right - btnTextRect.left, btnTextRect.bottom - btnTextRect.top, TRUE);
		nBtnCmdShow = SW_SHOW;
	}
	ShowWindow(pThis->dwQSDCButton.m_hWnd, nBtnCmdShow);

	// Originally there was a check whereas tiles
	// prior to the arcologies were displayed in
	// their large size, whereas the rest were
	// medium - no longer.
	nSpriteID = pThis->dwQSDTileID + SPRITE_LARGE_START;

	pThis->dwQSDCGraphicsOne = new CGraphics();
	if (pThis->dwQSDCGraphicsOne)
		pThis->dwQSDCGraphicsOne = Game_Graphics_Cons(pThis->dwQSDCGraphicsOne);
	else
		pThis->dwQSDCGraphicsOne = 0;

	pThis->dwQSDPointOne.x = (pArrSpriteHeaders[nSpriteID].wWidth + 7) & ~7;
	pThis->dwQSDPointOne.y = (pArrSpriteHeaders[nSpriteID].wHeight + 8) & ~7;
	pThis->dwQSDCGraphicsOne->CreateWithPalette_SC2K1996(pThis->dwQSDPointOne.x, pThis->dwQSDPointOne.y);

	hWndExt = pThis->m_hWnd;
	GameMain_PaintDC_Dest(&paintDC);
	return 1;
}

extern "C" void __stdcall Hook_QuerySpecificDialog_SetCursorAndDeleteGraphics() {
	CQuerySpecificDialog *pThis;

	__asm mov [pThis], ecx

	Game_GameDialog_SetCursor(pThis);
	if (pThis->dwQSDCGraphicsOne) {
		pThis->dwQSDCGraphicsOne->DeleteStored_SC2K1996();
		delete pThis->dwQSDCGraphicsOne;
		pThis->dwQSDCGraphicsOne = 0;
	}
}

extern "C" int __stdcall Hook_QueryGeneralDialog_OnInitDialog() {
	CQueryGeneralDialog *pThis;

	__asm mov [pThis], ecx

	int ret;

	ret = GameMain_QueryGeneralDialog_OnInitDialog(pThis);
	if (ret)
		hWndExt = pThis->m_hWnd;

	return ret;
}

extern "C" void __stdcall Hook_QuerySpecificDialog_OnPaint() {
	CQuerySpecificDialog *pThis;

	__asm mov [pThis], ecx

	COLORREF crSysColor, crColor;
	CMFC3XPaintDC paintDC;
	RECT dlgRect;
	CSimcityAppPrimary *pSCApp;
	__int16 iTextOverlay;
	int nSpriteID = -1;
	BYTE *pBits;
	CMFC3XDC *pDC;
	BYTE iMicrosimID;
	__int16 nLine;
	int x, y, nHeight;
	POINT pt;
	BYTE nRating;

	GameMain_PaintDC_Cons(&paintDC, pThis);

	pSCApp = &pCSimcityAppThis;
	Game_SimcityApp_GetActivePalette(pSCApp);

	GetWindowRect(pThis->m_hWnd, &dlgRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&dlgRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&dlgRect.right);

	iTextOverlay = XTXTGetTextOverlayID((WORD)pThis->dwQSDMapX, (WORD)pThis->dwQSDMapY);
	crSysColor = GetSysColor(COLOR_BTNFACE);
	SetBkColor(paintDC.m_hDC, crSysColor);
	nSpriteID = pThis->dwQSDTileID + SPRITE_LARGE_START;
	x = (pArrSpriteHeaders[nSpriteID].wWidth + 7) & ~7;
	y = (pArrSpriteHeaders[nSpriteID].wHeight + 8) & ~7;
	pBits = Game_Graphics_LockDIBBits(pThis->dwQSDCGraphicsOne);
	if (pBits) {
		pDC = Game_Graphics_GetDC(pThis->dwQSDCGraphicsOne);
		if (pDC) {
			FillRect(pDC->m_hDC, &dlgRect, (HBRUSH)MainBrushFace->m_hObject);
			Game_Graphics_ReleaseDC(pThis->dwQSDCGraphicsOne, pDC);
			Game_BeginProcessObjects(pThis, pBits, x, y, &dlgRect);
			L_drawShapeDialog_SC2K1996(nSpriteID, 0, 0, 0, 0);
			Game_FinishProcessObjects();
		}
		Game_Graphics_UnlockDIBBits(pThis->dwQSDCGraphicsOne);
	}
	x = dlgRect.right - pArrSpriteHeaders[nSpriteID].wWidth - 20;
	y = (dlgRect.bottom - pArrSpriteHeaders[nSpriteID].wHeight) / 2;
	Game_Graphics_SetColorTableFromApplicationPalette(pThis->dwQSDCGraphicsOne);
	Game_Graphics_Paint(pThis->dwQSDCGraphicsOne, paintDC.m_hDC, x, y);
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);
	crColor = GetSysColor(COLOR_BTNTEXT);
	nHeight = 20 + 40;
	iMicrosimID = MICROSIMID_ENTRY(iTextOverlay);
	for (nLine = 0; nLine < 5; ++nLine) {
		MoveToEx(paintDC.m_hDC, 12, nHeight + 20 * nLine, &pt);
		Game_CalculateGrade(&paintDC, iMicrosimID, nLine);
	}
	if (!bSoundPlayed) {
		switch (pThis->dwQSDTileID) {
			case TILE_POWERPLANT_HYDRO1:
			case TILE_POWERPLANT_HYDRO2:
			case TILE_POWERPLANT_WIND:
			case TILE_POWERPLANT_GAS:
			case TILE_POWERPLANT_OIL:
			case TILE_POWERPLANT_NUCLEAR:
			case TILE_POWERPLANT_SOLAR:
			case TILE_POWERPLANT_MICROWAVE:
			case TILE_POWERPLANT_FUSION:
			case TILE_POWERPLANT_COAL:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_ZAP);
				break;
			case TILE_SERVICES_CITYHALL:
			case TILE_SERVICES_BIGPARK:
			case TILE_SERVICES_STADIUM:
			case TILE_SERVICES_STATUE:
			case TILE_INFRASTRUCTURE_MAYORSHOUSE:
			case TILE_OTHER_BRAUNLLAMADOME:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CHEERS);
				break;
			case TILE_SERVICES_HOSPITAL:
			case TILE_SERVICES_POLICE:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_POLICE);
				break;
			case TILE_SERVICES_FIRE:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_FIRETRUCK);
				break;
			case TILE_SERVICES_SCHOOL:
			case TILE_SERVICES_COLLEGE:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_SCHOOL);
				break;
			case TILE_SERVICES_PRISON:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_PRISON);
				break;
			case TILE_SERVICES_ZOO:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_MONSTER);
				break;
			case TILE_INFRASTRUCTURE_BUSDEPOT:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_HORNS);
				break;
			case TILE_INFRASTRUCTURE_RAILSTATION:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_TRAIN);
				break;
			case TILE_INFRASTRUCTURE_MARINA:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_FLOOD);
				break;
			case TILE_ARCOLOGY_PLYMOUTH:
			case TILE_ARCOLOGY_FOREST:
			case TILE_ARCOLOGY_DARCO:
			case TILE_ARCOLOGY_LAUNCH:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_ARCO);
				nRating = GetMicroSimulatorStat0(iMicrosimID);
				if (nRating > 9)
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CHEERS);
				if (nRating < 4)
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_BOOS);
				break;
			default:
				break;
		}
		bSoundPlayed = TRUE;
	}
	GameMain_PaintDC_Dest(&paintDC);
}

#define LINEHEIGHT 20
#define QG_LINE(x) x * LINEHEIGHT

extern "C" void __stdcall Hook_QueryGeneralDialog_OnPaint() {
	CQueryGeneralDialog *pThis;

	__asm mov [pThis], ecx

	CMFC3XPaintDC paintDC;
	RECT rcDest;
	BYTE nLevels[5];
	coords_w_t tileCoords;
	CSimcityAppPrimary *pSCApp;
	int nGerman, nFrench, nOffsetX;
	BOOL bZoned;
	int nOffsetY;
	BOOL bWetTile;
	DWORD nWidthBytes;
	int nTextAreaWidth, nHeight, x, y;
	WORD nWidth;
	RECT shapeAreaRect;
	CMFC3XDC *pDC;
	const char *pSrcString;
	char szDest[80];
	SIZE txtSz;
	POINT pt;
	BYTE wZone;
	char szResBuf[255 + 1], szTextBuf[511 + 1];
	BYTE iTerrainTileID;
	int nLandAlt, nFeet;
	BYTE nCrimeVal;
	__int16 nThreshold;
	BYTE nPollutionVal;
	COLORREF crOldCol;
	__int16 nCurrentPointX, nCurrentPointY;

	GameMain_PaintDC_Cons(&paintDC, pThis);
	CopyRect(&rcDest, &paintDC.m_ps.rcPaint);

	nLevels[0] = 0;    // None
	nLevels[1] = 1;    // Low
	nLevels[2] = 60;   // Medium
	nLevels[3] = 120;  // High
	nLevels[4] = 180;  // Very High

	tileCoords.x = (__int16)pThis->dwQGPointOne.x;
	tileCoords.y = (__int16)pThis->dwQGPointOne.y;

	pSCApp = &pCSimcityAppThis;

	// Determine language in order
	// for attribute and offset
	// adjustments.
	nGerman = 0;
	nFrench = 0;
	nOffsetX = 0;
	if (_stricmp(pSCApp->dwSCACStringLang.m_pchData, "ger") == 0) {
		nGerman = 1;
		nOffsetX = 50;
	}
	else if (_stricmp(pSCApp->dwSCACStringLang.m_pchData, "fre") == 0) {
		nFrench = 1;
		nOffsetX = 1;
	}

	// The shape.
	SetBkColor(paintDC.m_hDC, GetSysColor(COLOR_BTNFACE));
	SetTextColor(paintDC.m_hDC, GetSysColor(COLOR_BTNTEXT));
	bWetTile = FALSE;
	nWidthBytes = Game_Graphics_WidthBytes(pThis->pQGDGraphic);
	nHeight = pArrSpriteHeaders[wQuerySpriteID].wHeight;
	x = nWidthBytes;
	y = nHeight;
	shapeAreaRect.left = 0;
	shapeAreaRect.top = 0;
	shapeAreaRect.right = nWidthBytes;
	shapeAreaRect.bottom = nHeight;
	pDC = Game_Graphics_GetDC(pThis->pQGDGraphic);
	SetBkColor(pDC->m_hDC, GetSysColor(COLOR_BTNFACE));
	++shapeAreaRect.bottom;
	ExtTextOutA(pDC->m_hDC, 0, 0, OPAQUE, &shapeAreaRect, "    ", 0, 0);
	--shapeAreaRect.bottom;
	Game_Graphics_ReleaseDC(pThis->pQGDGraphic, pDC);
	Game_BeginProcessObjects(pThis, pQuerySpriteBits, x, y, &rcDest);
	L_drawShapeDialog_SC2K1996(wQuerySpriteID, 0, 0, 0, 0);
	Game_FinishProcessObjects();
	if (nGerman || nFrench)
		nTextAreaWidth = 4 * (rcDest.right - rcDest.left) / 5;
	else
		nTextAreaWidth = 3 * (rcDest.right - rcDest.left) / 4;
	nWidth = pArrSpriteHeaders[wQuerySpriteID].wWidth;
	x = nTextAreaWidth - (nWidth >> 1);
	y = (rcDest.bottom - rcDest.top) / 2 - (nHeight >> 1);
	Game_Graphics_SetColorTableFromApplicationPalette(pThis->pQGDGraphic);
	Game_Graphics_Paint(pThis->pQGDGraphic, paintDC.m_hDC, x, y);

	// Tile Name
	wQueryTileID = GetPertinentRsrcIDOffset(tileCoords.x, tileCoords.y);
	pSrcString = pCustomTileNamesFromSpriteID[wQuerySpriteID];
	if (pSrcString)
		strcpy_s(szDest, pSrcString);
	else
		Game_LoadNamedEntryFromRsrcOffset(szDest, 2000, wQueryTileID);
	GetTextExtentPointA(paintDC.m_hAttribDC, szDest, strlen(szDest), &txtSz);
	x = (rcDest.right - txtSz.cx - rcDest.left) / 2;
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);
	MoveToEx(paintDC.m_hDC, x, 16, &pt);
	TextOutA(paintDC.m_hDC, 0, 0, szDest, strlen(szDest));

	// Main section
	// bZoned check added to account for
	// Military-zoned road tiles (Army Base case)
	// in-order for the traffic label to be displayed
	// and subsequent rows to be offset.
	bZoned = FALSE;
	nOffsetY = 0;
	wZone = XZONReturnZone(tileCoords.x, tileCoords.y);
	wQueryTileID = GetTileID(tileCoords.x, tileCoords.y);
	if (wZone) {
		// Zone Label
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 812, szResBuf, sizeof(szResBuf) - 1);
		SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
		MoveToEx(paintDC.m_hDC, nOffsetX + 100, 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

		SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

		// Zone Name
		Game_LoadNamedEntryFromRsrcOffset(szDest, 2100, wZone);
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szDest, strlen(szDest));

		// Zone Density
		Game_LoadNamedEntryFromRsrcOffset(szDest, 2100, wZone + 16);
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, QG_LINE(1) + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szDest, strlen(szDest));
		bZoned = TRUE;
	}
	if (GET_TILE_RANGE(wQueryTileID, TILE_ROAD_LR, TILE_ROAD_LTBR) ||
		GET_TILE_RANGE(wQueryTileID, TILE_TUNNEL_T, TILE_TUNNEL_L) ||
		GET_TILE_RANGE(wQueryTileID, TILE_CROSSOVER_HIGHWAYLR_ROADTB, TILE_CROSSOVER_HIGHWAYTB_ROADLR) ||
		GET_TILE_RANGE(wQueryTileID, TILE_ONRAMP_TL, TILE_ONRAMP_BR) ||
		GET_TILE_RANGE(wQueryTileID, TILE_HIGHWAY_HTB, TILE_HIGHWAY_LTBR) ||
		GET_TILE_RANGE(wQueryTileID, TILE_HIGHWAY_LR, TILE_CROSSOVER_HIGHWAYTB_POWERLR) ||
		GET_TILE_RANGE(wQueryTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_RAIL_BRIDGE) ||
		GET_TILE_RANGE(wQueryTileID, TILE_REINFORCED_BRIDGE_PYLON, TILE_REINFORCED_BRIDGE)) {
		// Traffic Label
		if (bZoned)
			nOffsetY = QG_LINE(2);
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 813, szResBuf, sizeof(szResBuf) - 1);
		SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
		MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

		SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

		// Traffic - Originally for the MAP_EDGE_MAX (127) case
		// there was a bug whereas it was doing a "less than"
		// on GAME_MAP_SIZE (128), which failed when it attempted
		// the axis + 1 case.
		// cars/minute Label
		__int16 nTraffic = 0;
		if (tileCoords.x > MAP_EDGE_MIN)
			nTraffic = GetXTRFByteDataWithNormalCoordinates(tileCoords.x - 1, tileCoords.y);
		if (tileCoords.y > MAP_EDGE_MIN)
			nTraffic += GetXTRFByteDataWithNormalCoordinates(tileCoords.x, tileCoords.y - 1);
		if (tileCoords.x < MAP_EDGE_MAX)
			nTraffic += GetXTRFByteDataWithNormalCoordinates(tileCoords.x + 1, tileCoords.y);
		if (tileCoords.y < MAP_EDGE_MAX)
			nTraffic += GetXTRFByteDataWithNormalCoordinates(tileCoords.x, tileCoords.y + 1);
		if (GET_TILE_RANGE(wQueryTileID, TILE_HIGHWAY_HTB, TILE_REINFORCED_BRIDGE) ||
			GET_TILE_RANGE(wQueryTileID, TILE_HIGHWAY_LR, TILE_CROSSOVER_HIGHWAYTB_POWERLR))
			nTraffic *= 2;
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 814, szResBuf, sizeof(szResBuf) - 1);
		sprintf_s(szTextBuf, "%ld%s", nTraffic / 4 / 2, szResBuf);
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));
	}
	else
		bZoned = FALSE;

	// Altitude Label
	nOffsetY = (bZoned) ? QG_LINE(3) : QG_LINE(2);
	LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 827, szResBuf, sizeof(szResBuf) - 1);
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
	MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
	TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

	// Altitude
	MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
	iTerrainTileID = GetTerrainTileID(tileCoords.x, tileCoords.y);
	nLandAlt = ALTMReturnLandAltitude(tileCoords.x, tileCoords.y);
	if (nLandAlt < wWaterLevel) {
		// feet deep Label
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 815, szResBuf, sizeof(szResBuf) - 1);
		nFeet = 100 * (wWaterLevel - nLandAlt) - 50;
		bWetTile = TRUE;
	}
	else {
		// feet Label
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 824, szResBuf, sizeof(szResBuf) - 1);
		if (!iTerrainTileID || iTerrainTileID >= SUBMERGED_00) {
			if (iTerrainTileID >= SUBMERGED_00)
				bWetTile = TRUE;
			nFeet = 100 * (nLandAlt - wWaterLevel) + 50;
		}
		else
			nFeet = 25 * (4 * (nLandAlt - wWaterLevel) + 4);
	}
	sprintf_s(szTextBuf, "%ld %s", nFeet, szResBuf);
	TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));

	if (!bWetTile || nLandAlt < wWaterLevel) {
		// Land Value Label
		nOffsetY = (bZoned) ? QG_LINE(4) : QG_LINE(3);
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 816, szResBuf, sizeof(szResBuf) - 1);
		SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
		MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

		SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

		// Land Value and thousand acre label
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 825, szResBuf, sizeof(szResBuf) - 1);
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
		sprintf_s(szTextBuf, "%ld%s", GetXVALByteDataWithNormalCoordinates(tileCoords.x, tileCoords.y) + 1, szResBuf);
		TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));
	}

	// Crime Label
	nOffsetY = (bZoned) ? QG_LINE(5) : QG_LINE(4);
	LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 817, szResBuf, sizeof(szResBuf) - 1);
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
	MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
	TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

	// Crime value Label
	MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
	nCrimeVal = GetXCRMByteDataWithNormalCoordinates(tileCoords.x, tileCoords.y);
	for (nThreshold = 1; nThreshold < 5; ++nThreshold) {
		if (nCrimeVal <= nLevels[nThreshold])
			break;
	}
	Game_LoadNamedEntryFromRsrcOffset(szDest, 2200, nThreshold);
	TextOutA(paintDC.m_hDC, 0, 0, szDest, strlen(szDest));

	// Pollution Label
	nOffsetY = (bZoned) ? QG_LINE(6) : QG_LINE(5);
	LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 818, szResBuf, sizeof(szResBuf) - 1);
	SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
	MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
	TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

	SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

	// Pollution value Label
	MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
	nPollutionVal = GetXPLTByteDataWithNormalCoordinates(tileCoords.x, tileCoords.y);
	for (nThreshold = 1; nThreshold < 5; ++nThreshold) {
		if (nPollutionVal <= nLevels[nThreshold])
			break;
	}
	Game_LoadNamedEntryFromRsrcOffset(szDest, 2200, nThreshold);
	TextOutA(paintDC.m_hDC, 0, 0, szDest, strlen(szDest));

	// Infrastructure/building section
	if (wQueryTileID >= TILE_SMALLPARK && wZone != ZONE_MILITARY && !bWetTile) {
		// Powered Label
		nOffsetY = (bZoned) ? QG_LINE(7) : QG_LINE(6);
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 826, szResBuf, sizeof(szResBuf) - 1);
		SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
		MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

		SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

		// Powered indicator label
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
		if (tileCoords.x < GAME_MAP_SIZE &&
			tileCoords.y < GAME_MAP_SIZE &&
			XBITReturnIsPowered(tileCoords.x, tileCoords.y)) {
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 819, szResBuf, sizeof(szResBuf) - 1);
			TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));
		}
		else {
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 820, szResBuf, sizeof(szResBuf) - 1);
			crOldCol = SetTextColor(paintDC.m_hDC, RGB(255, 0, 0));
			TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));
			SetTextColor(paintDC.m_hDC, crOldCol);
		}

		// Watered Label
		nOffsetY = (bZoned) ? QG_LINE(8) : QG_LINE(7);
		LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 821, szResBuf, sizeof(szResBuf) - 1);
		SetTextAlign(paintDC.m_hDC, TA_UPDATECP | TA_RIGHT);
		MoveToEx(paintDC.m_hDC, nOffsetX + 100, nOffsetY + 40, &pt);
		TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));

		SetTextAlign(paintDC.m_hDC, TA_UPDATECP);

		// Watered indicator label
		MoveToEx(paintDC.m_hDC, nOffsetX + 110, nOffsetY + 40, &pt);
		if (wQueryTileID == TILE_INFRASTRUCTURE_WATERPUMP) {
			// Gallons Per Month value Label
			__int16 nGallonsPerMonth = 0;
			if (tileCoords.x < GAME_MAP_SIZE &&
				tileCoords.y < GAME_MAP_SIZE &&
				XBITReturnIsPowered(tileCoords.x, tileCoords.y)) {
				nGallonsPerMonth = 5 * wWaterLevel + (bWeatherRain >> 1);
				for (nCurrentPointX = tileCoords.x - 1; nCurrentPointX <= tileCoords.x + 1; ++nCurrentPointX) {
					if (nCurrentPointX < GAME_MAP_SIZE) {
						for (nCurrentPointY = tileCoords.y - 1; nCurrentPointY <= tileCoords.y + 1; ++nCurrentPointY) {
							if (nCurrentPointY < GAME_MAP_SIZE) {
								if ((XBITReturnMask(nCurrentPointX, nCurrentPointY) & (XBIT_SALTWATER | XBIT_WATER)) == XBIT_WATER)
									nGallonsPerMonth += 10;
							}
						}
					}
				}
			}
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 822, szResBuf, sizeof(szResBuf) - 1);
			sprintf_s(szTextBuf, "%ld %s", 720 * nGallonsPerMonth, szResBuf);
			TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));
		}
		else if (wQueryTileID == TILE_INFRASTRUCTURE_WATERTOWER) {
			// Stored Gallons value Label
			__int16 nStoredGallons = 0;
			__int16 nDominantCornerPointX = tileCoords.x;
			__int16 nDominantCornerPointY = tileCoords.y;
			Game_FindCorner(&nDominantCornerPointX, &nDominantCornerPointY, wQueryTileID);
			nCurrentPointX = nDominantCornerPointX + 1;
			nCurrentPointY = nDominantCornerPointY - 1;
			if (nDominantCornerPointX < GAME_MAP_SIZE &&
				nDominantCornerPointY < GAME_MAP_SIZE &&
				XBITReturnIsWatered(nDominantCornerPointX, nDominantCornerPointY))
				++nStoredGallons;
			if (nCurrentPointX >= MAP_EDGE_MIN &&
				nCurrentPointX < GAME_MAP_SIZE &&
				nDominantCornerPointY < GAME_MAP_SIZE &&
				XBITReturnIsWatered(nCurrentPointX, nDominantCornerPointY))
				++nStoredGallons;
			if (nDominantCornerPointX < GAME_MAP_SIZE &&
				nCurrentPointY >= MAP_EDGE_MIN &&
				nCurrentPointY < GAME_MAP_SIZE &&
				XBITReturnIsWatered(nDominantCornerPointX, nCurrentPointY))
				++nStoredGallons;
			if (nCurrentPointX < GAME_MAP_SIZE && 
				nCurrentPointY >= MAP_EDGE_MIN &&
				nCurrentPointY < GAME_MAP_SIZE &&
				XBITReturnIsWatered(nCurrentPointX, nCurrentPointY))
				++nStoredGallons;
			LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 823, szResBuf, sizeof(szResBuf) - 1);
			sprintf_s(szTextBuf, "%ld %s", 10000 * nStoredGallons, szResBuf);
			TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));
		}
		else {
			if (tileCoords.x < GAME_MAP_SIZE &&
				tileCoords.y < GAME_MAP_SIZE &&
				XBITReturnIsWatered(tileCoords.x, tileCoords.y)) {
				LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 819, szResBuf, sizeof(szResBuf) - 1);
				TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));
			}
			else {
				LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 820, szResBuf, sizeof(szResBuf) - 1);
				crOldCol = SetTextColor(paintDC.m_hDC, RGB(255, 0, 0));
				TextOutA(paintDC.m_hDC, 0, 0, szResBuf, strlen(szResBuf));
				SetTextColor(paintDC.m_hDC, crOldCol); // Restore the old colour (not present originally)
			}
		}
	}

	// Debug section
	if (GetAsyncKeyState(VK_MENU) < 0) {
		MoveToEx(paintDC.m_hDC, rcDest.left + 10, rcDest.bottom - 30, &pt);
		sprintf_s(szResBuf, "X=%ld Y=%ld", tileCoords.x, tileCoords.y);
		if (tileCoords.x < GAME_MAP_SIZE &&
			tileCoords.y < GAME_MAP_SIZE &&
			XBITReturnIsFlipped(tileCoords.x, tileCoords.y))
			strcat_s(szResBuf, " flip");
		sprintf_s(szTextBuf, "%s Txt=%ld", szResBuf, XTXTGetTextOverlayID(tileCoords.x, tileCoords.y));
		TextOutA(paintDC.m_hDC, 0, 0, szTextBuf, strlen(szTextBuf));
	}

	SetTextAlign(paintDC.m_hDC, TA_NOUPDATECP);

	GameMain_PaintDC_Dest(&paintDC);
}

void InstallQueryHooks_SC2K1996(void) {
	//ConsoleLog(LOG_DEBUG, "MISC: Installing query hooks.\n");

	SafeVirtualProtect((LPVOID)0x401CFD, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401CFD, Hook_QuerySpecificItem);

	SafeVirtualProtect((LPVOID)0x402E19, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402E19, Hook_QueryGeneralItem);

	// Hook into the CQuerySpecificDialog::OnInitDialog function
	// 1) The original drawing call
	// 2) To move the button text adjustment, movement and showing
	//    to the paint call (otherwise it crashes in quite the
	//    spectacular manner when locally implemented).
	SafeVirtualProtect((LPVOID)0x4019C9, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4019C9, Hook_QuerySpecificDialog_OnInitDialog);

	// Hook the CQuerySpecificDialog::SetCursorAndDeleteGraphics
	SafeVirtualProtect((LPVOID)0x402D06, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402D06, Hook_QuerySpecificDialog_SetCursorAndDeleteGraphics);

	// Hook CQueryGeneralDialog::OnInitDialog in order to:
	SafeVirtualProtect((LPVOID)0x402C89, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402C89, Hook_QueryGeneralDialog_OnInitDialog);

	// Hook CQuerySpecificDialog::OnPaint in order to:
	// 1) Handle the button text adjustment, movement and show
	// 2) Enable the new cycling and other effects
	// 3) Avoid sound playing over and over due to the redraw for (2)
	SafeVirtualProtect((LPVOID)0x402C16, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402C16, Hook_QuerySpecificDialog_OnPaint);

	// Hook CQueryGeneralDialog::OnPaint
	SafeVirtualProtect((LPVOID)0x4017DF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4017DF, Hook_QueryGeneralDialog_OnPaint);
}
