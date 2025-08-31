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

typedef struct {
	WORD iTileX;
	WORD iTileY;
} query_coords_info;

BOOL CALLBACK AdvancedQueryDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	query_coords_info *qci;
	WORD iTileX, iTileY;
	std::string strTileHeader;
	std::string strTileInfo;
	BYTE iTileID;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		qci = (query_coords_info *)lParam;

		iTileX = qci->iTileX;
		iTileY = qci->iTileY;
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
		strTileHeader += ")\nCoordinates:   X=";
		strTileHeader += std::to_string(iTileX);
		strTileHeader += "  Y=";
		strTileHeader += std::to_string(iTileY);

		// Build the data string
		strTileInfo =  GetZoneName(XZONReturnZone(iTileX, iTileY));
		strTileInfo += "\n";
		
		// Altitude/depth
		if (XBITReturnIsWater(iTileX, iTileY) && ALTMReturnLandAltitude(iTileX, iTileY) < wWaterLevel)
			strTileInfo += std::to_string(100 * (wWaterLevel - ALTMReturnLandAltitude(iTileX, iTileY)) - 50);
		else if (dwMapXTER[iTileX][iTileY].iTileID && dwMapXTER[iTileX][iTileY].iTileID < SUBMERGED_00)
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
		strTileInfo += std::to_string(dwMapXVAL[iTileX >> 1][iTileY >> 1].bBlock + 1);
		strTileInfo += ",000/acre\n";

		// Crime
		strTileInfo += GetLowHighScale(dwMapXCRM[iTileX >> 1][iTileY >> 1].bBlock);
		strTileInfo += " (XCRM: ";
		strTileInfo += std::to_string(dwMapXCRM[iTileX >> 1][iTileY >> 1].bBlock);
		strTileInfo += ")\n";

		// Pollution
		strTileInfo += GetLowHighScale(dwMapXPLT[iTileX >> 1][iTileY >> 1].bBlock);
		strTileInfo += " (XPLT: ";
		strTileInfo += std::to_string(dwMapXPLT[iTileX >> 1][iTileY >> 1].bBlock);
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
		if (dwMapXTXT[iTileX][iTileY].bTextOverlay <= MIN_SIM_TEXT_ENTRIES || dwMapXTXT[iTileX][iTileY].bTextOverlay > MAX_SIM_TEXT_ENTRIES)
			strTileInfo += "None\nN/A\nN/A\nN/A\nN/A";
		else {
			BYTE bTextOverlay = dwMapXTXT[iTileX][iTileY].bTextOverlay; // Entries > 51 aren't user-modifiable label/text entries.
			BYTE iMicrosimID = bTextOverlay - MIN_SIM_TEXT_ENTRIES; // The MicrosimID being calculated from entry 52 and beyond but subtracted by the non-user modifiable starting value.
			strTileInfo += GetXLABEntry(bTextOverlay);
			strTileInfo += " (iMicrosimID " + std::to_string(iMicrosimID) + " / " + HexPls(iMicrosimID, 2) + ")\n";
			strTileInfo += std::to_string(pMicrosimArr[iMicrosimID].bMicrosimDataStat0) + " / " + HexPls(pMicrosimArr[iMicrosimID].bMicrosimDataStat0, 2) + "\n";
			strTileInfo += std::to_string(pMicrosimArr[iMicrosimID].iMicrosimDataStat1) + " / " + HexPls(pMicrosimArr[iMicrosimID].iMicrosimDataStat1, 2) + "\n";
			strTileInfo += std::to_string(pMicrosimArr[iMicrosimID].iMicrosimDataStat2) + " / " + HexPls(pMicrosimArr[iMicrosimID].iMicrosimDataStat2, 2) + "\n";
			strTileInfo += std::to_string(pMicrosimArr[iMicrosimID].iMicrosimDataStat3) + " / " + HexPls(pMicrosimArr[iMicrosimID].iMicrosimDataStat3, 2);
		}

		SetDlgItemText(hwndDlg, IDC_STATIC_TILENAME, strTileHeader.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_TILEDATA, strTileInfo.c_str());

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL DoAdvancedQuery(__int16 x, __int16 y) {
	DWORD *pCityToolBar;
	query_coords_info qci;

	pCityToolBar = &((DWORD *)pCWndRootWindow)[102];

	if (bUseAdvancedQuery) {
		if (GetAsyncKeyState(VK_MENU) < 0) {
			memset(&qci, 0, sizeof(qci));
			qci.iTileX = x;
			qci.iTileY = y;

			Game_ToolMenuDisable(pCityToolBar);
			DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_ADVANCEDQUERY), GameGetRootWindowHandle(), AdvancedQueryDialogProc, (LPARAM)&qci);
			Game_ToolMenuEnable(pCityToolBar);
			return TRUE;
		}
	}
	return FALSE;
}

extern "C" void __cdecl Hook_QuerySpecificItem(__int16 x, __int16 y) {
	void(__cdecl *H_QuerySpecificItem)(__int16, __int16) = (void(__cdecl *)(__int16, __int16))0x44D1B0;

	if (!DoAdvancedQuery(x, y))
		H_QuerySpecificItem(x, y);
}

extern "C" void __cdecl Hook_QueryGeneralItem(__int16 x, __int16 y) {
	void(__cdecl *H_QueryGeneralItem)(__int16, __int16) = (void(__cdecl *)(__int16, __int16))0x4719A0;

	if (!DoAdvancedQuery(x, y))
		H_QueryGeneralItem(x, y);
}

void InstallQueryHooks_SC2K1996(void) {
	ConsoleLog(LOG_DEBUG, "MISC: Installing Query Hooks\n");

	VirtualProtect((LPVOID)0x401CFD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401CFD, Hook_QuerySpecificItem);

	VirtualProtect((LPVOID)0x402E19, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402E19, Hook_QueryGeneralItem);
}
