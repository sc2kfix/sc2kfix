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

static DWORD dwDummy;

extern HWND hWndExt;

typedef struct {
	WORD iTileX;
	WORD iTileY;
} query_coords_info;

static CGraphics *pQueriedTileImage = NULL;
static int nSpriteID = -1;

static int GetQueriedSpriteIDFromCoords(WORD x, WORD y) {
	__int16 iTileID, iTextOverlay;
	int iSpriteID = -1;
	
	iTileID = GetTileID(x, y);
	if (!iTileID) {
		iTileID = GetTerrainTileID(x, y);
		iTileID = wXTERToSpriteIDMap[iTileID];
	}
	else {
		if (iTileID >= TILE_ARCOLOGY_PLYMOUTH) {
			// Positioning falls into the "medium" range.
			iSpriteID = iTileID + SPRITE_MEDIUM_START;
		}
	}	

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

BOOL __cdecl L_BeingProcessObjectOnHwnd(HWND hWnd, void *vBits, int x, int y, RECT *r) {
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

BOOL CALLBACK AdvancedQueryDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	query_coords_info *qci;
	WORD iTileX, iTileY;
	std::string strTileHeader;
	std::string strTileInfo;
	BYTE iTextOverlay, iRsrcOffset, iTileID;
	CMFC3XRect dlgRect, sprRect;
	void *pSprBits;
	CMFC3XDC *pDC;
	PAINTSTRUCT ps;
	sprite_header_t *pSprHead;
	CMFC3XPoint sprPt;
	const char *pSrc;
	char szBuf[80 + 1];
	int x, y;

	switch (message) {
	case WM_INITDIALOG:
		hWndExt = hwndDlg;

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		qci = (query_coords_info *)lParam;

		GetWindowRect(hwndDlg, &dlgRect);
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
			pSprHead = &pArrSpriteHeaders[nSpriteID];
			if (pSprHead) {
				sprPt.x = (pSprHead->wWidth + 7) & ~7;
				sprPt.y = (pSprHead->wHeight + 8) & ~7;

				pQueriedTileImage = new CGraphics();
				if (pQueriedTileImage) {
					pQueriedTileImage = Game_Graphics_Cons(pQueriedTileImage);
					if (pQueriedTileImage) {

						sprRect.left = 0;
						sprRect.top = 0;
						sprRect.right = sprPt.x;
						sprRect.bottom = sprPt.y;

						pQueriedTileImage->CreateWithPalette_SC2K1996(sprPt.x, sprPt.y);
						pSprBits = Game_Graphics_LockDIBBits(pQueriedTileImage);
						pDC = new CMFC3XDC();
						pDC = Game_Graphics_GetDC(pQueriedTileImage);
						if (pDC) {
							FillRect(pDC->m_hDC, &sprRect, (HBRUSH)MainBrushFace->m_hObject);
							Game_Graphics_ReleaseDC(pQueriedTileImage, pDC);
							pDC = NULL;

							L_BeingProcessObjectOnHwnd(hwndDlg, pSprBits, sprPt.x, sprPt.y, &dlgRect);
							Game_DrawProcessObject(nSpriteID, 0, 0, 0, 0);
							Game_FinishProcessObjects();
							Game_Graphics_UnlockDIBBits(pQueriedTileImage);
						}
					}
				}
			}
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
		GetClientRect(hwndDlg, &dlgRect);

		if (nSpriteID >= 0) {
			pSprHead = &pArrSpriteHeaders[nSpriteID];
			if (pSprHead) {
				x = dlgRect.right - pSprHead->wWidth - 20;
				y = (dlgRect.bottom - pSprHead->wHeight) / 2;

				if (pQueriedTileImage) {
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
		if (pQueriedTileImage) {
			pQueriedTileImage->DeleteStored_SC2K1996();
			delete pQueriedTileImage;
			pQueriedTileImage = NULL;
		}
		nSpriteID = -1;
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

extern "C" void __cdecl Hook_QuerySpecificItem(__int16 x, __int16 y) {
	if (!DoAdvancedQuery(x, y))
		GameMain_QuerySpecificItem(x, y);
}

extern "C" void __cdecl Hook_QueryGeneralItem(__int16 x, __int16 y) {
	if (!DoAdvancedQuery(x, y))
		GameMain_QueryGeneralItem(x, y);
}

void InstallQueryHooks_SC2K1996(void) {
	//ConsoleLog(LOG_DEBUG, "MISC: Installing query hooks.\n");

	VirtualProtect((LPVOID)0x401CFD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401CFD, Hook_QuerySpecificItem);

	VirtualProtect((LPVOID)0x402E19, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402E19, Hook_QueryGeneralItem);

	// Move the alt+query bottom text to not be blocked by the OK button
	VirtualProtect((LPVOID)0x428FB1, 3, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x428FB1 = 0x83;
	*(BYTE*)0x428FB2 = 0xE8;
	*(BYTE*)0x428FB3 = 0x32;

	// Patch the maximum so it's reduced from GAME_MAP_SIZE to GAME_MAP_SIZE - 1
	// otherwise a failure was occurring as it was attempting to fetch the XTRF
	// values which halted all subsequent painting.
	// Even though this issue only cropped up when the X coordinate was 127
	// it has been adjusted for both X and Y cases.
	VirtualProtect((LPVOID)0x4284F3, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4284F3 = GAME_MAP_SIZE - 1;
	VirtualProtect((LPVOID)0x42851D, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x42851D = GAME_MAP_SIZE - 1;
}
