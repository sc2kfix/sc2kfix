// sc2kfix hooks/hook_querydialog.cpp: hook for new query dialog features
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

static DWORD dwDummy;

static WORD iGlobalTileX, iGlobalTileY;

BOOL CALLBACK AdvancedQueryDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	WORD iTileX = iGlobalTileX;
	WORD iTileY = iGlobalTileY;
	std::string strTileHeader;
	std::string strTileInfo;
	int iTileID;

	switch (message) {
	case WM_INITDIALOG:
		// Build the header string
		iTileID = GetTileID(iGlobalTileX, iGlobalTileY);
		if (iTileID == 0) {
			if (dwMapXBIT[iTileX][iTileY].b.iWater && dwMapXBIT[iTileX][iTileY].b.iSaltWater)
				strTileHeader = "Salt water";
			else if (dwMapXBIT[iTileX][iTileY].b.iWater)
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
		strTileInfo =  GetZoneName(dwMapXZON[iTileX][iTileY].b.iZoneType);
		strTileInfo += "\n";
		
		// Altitude/depth
		if (dwMapXBIT[iTileX][iTileY].b.iWater && dwMapALTM[iTileX][iTileY].w.iLandAltitude < wWaterLevel)
			strTileInfo += std::to_string(100 * (wWaterLevel - dwMapALTM[iTileX][iTileY].w.iLandAltitude) - 50);
		else if (dwMapXTER[iTileX][iTileY].iTileID && dwMapXTER[iTileX][iTileY].iTileID < 0x10)
			strTileInfo += std::to_string(25 * (4 * (dwMapALTM[iTileX][iTileY].w.iLandAltitude - wWaterLevel) + 4));
		else
			strTileInfo += std::to_string(100 * (dwMapALTM[iTileX][iTileY].w.iLandAltitude - wWaterLevel) + 50);
		strTileInfo += " feet ";
		if (dwMapXBIT[iTileX][iTileY].b.iWater && dwMapALTM[iTileX][iTileY].w.iLandAltitude < wWaterLevel)
			strTileInfo += "deep ";
		strTileInfo += "(ALTM: ";
		strTileInfo += HexPls(*(WORD*)(&dwMapALTM[iTileX][iTileY].w), 4);
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
		switch (dwMapXZON[iTileX][iTileY].b.iCorners) {
		case 0:
			strTileInfo += "No corners";
			break;
		case 1:
			strTileInfo += "Bottom-left corner";
			break;
		case 2:
			strTileInfo += "Bottom-right corner";
			break;
		case 4:
			strTileInfo += "Top-left corner";
			break;
		case 8:
			strTileInfo += "Top-right corner";
			break;
		case 15:
			strTileInfo += "All four corners";
			break;
		}
		strTileInfo += ", iZoneID ";
		strTileInfo += HexPls(dwMapXZON[iTileX][iTileY].b.iZoneType, 1);
		strTileInfo += "\n";

		// XBIT
		if (!*(BYTE*)(&dwMapXBIT[iTileX][iTileY].b))
			strTileInfo += "none (XBIT: 0x00)\n";
		else {
			// XXX - this code sucks, more so than the rest of this function
			int i = 0;

			if (dwMapXBIT[iTileX][iTileY].b.iPowerable) {
				i++;
				strTileInfo += "powerable ";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iPowered) {
				i++;
				strTileInfo += "powered ";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iPiped) {
				i++;
				strTileInfo += "piped ";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iWatered) {
				i++;
				strTileInfo += "watered ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iXVALMask) {
				i++;
				strTileInfo += "xvalmask ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iWater) {
				i++;
				strTileInfo += "water ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iRotated) {
				i++;
				strTileInfo += "rotated ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX][iTileY].b.iSaltWater) {
				i++;
				strTileInfo += "saltwater ";
				if (i == 5)
					strTileInfo += "\n";
			}
			strTileInfo += "(XBIT: ";
			strTileInfo += HexPls(*(BYTE*)(&dwMapXBIT[iTileX][iTileY].b), 1);
			strTileInfo += ")\n";
			if (i < 5)
				strTileInfo += "\n";
		}

		// XUND
		if (dwMapXUND[iTileX][iTileY].iTileID > 35)
			strTileInfo += "Unknown";
		else
			strTileInfo += szUndergroundNames[dwMapXUND[iTileX][iTileY].iTileID];
		strTileInfo += " (XUND: ";
		strTileInfo += HexPls(dwMapXUND[iTileX][iTileY].iTileID, 2);
		strTileInfo += ")\n";

		// Microsim info
		if (dwMapXTXT[iTileX][iTileY].bTextOverlay < 0x34 || dwMapXTXT[iTileX][iTileY].bTextOverlay > 0xC8)
			strTileInfo += "None\nN/A\nN/A\nN/A\nN/A";
		else {
			int iMicrosimID = dwMapXTXT[iTileX][iTileY].bTextOverlay - 0x33;
			strTileInfo += GetXLABEntry(iMicrosimID + 0x33);
			strTileInfo += " (iMicrosimID " + std::to_string(iMicrosimID) + " / " + HexPls(iMicrosimID, 2) + ")\n";
			strTileInfo += std::to_string(pMicrosimArr[iMicrosimID].bMicrosimData[0]) + " / " + HexPls(pMicrosimArr[iMicrosimID].bMicrosimData[0], 2) + "\n";
			strTileInfo += std::to_string(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[1])) + " / " + HexPls(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[1]), 2) + "\n";
			strTileInfo += std::to_string(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[3])) + " / " + HexPls(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[3]), 2) + "\n";
			strTileInfo += std::to_string(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[5])) + " / " + HexPls(*(WORD*)(&pMicrosimArr[iMicrosimID].bMicrosimData[5]), 2);
		}

		SetDlgItemText(hwndDlg, IDC_STATIC_TILENAME, strTileHeader.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_TILEDATA, strTileInfo.c_str());

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

// Inserted into jump table at 0x43F924 in place of `dd offset loc_43F80C`
extern "C" void _declspec(naked) Hook_QueryJumpTable(void) {
	__asm {
		// Save all registers because this is one of those "hic sunt dracones" moments
		pusha
		mov [iGlobalTileX], bx
		mov [iGlobalTileY], bp
	}

	// See if we need to intercept
	if (GetAsyncKeyState(VK_MENU) < 0) {
		Game_ToolMenuDisable((char*)pCWndRootWindow + 408);
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_ADVANCEDQUERY), NULL, AdvancedQueryDialogProc);
		Game_ToolMenuEnable((char*)pCWndRootWindow + 408);
		__asm popa
		GAMEJMP(0x43F837)
	}

	// Go onto the regular call otherwise
	__asm popa
	GAMEJMP(0x43F80C)
}

void InstallQueryHooks(void) {
	// Install the query hook into the jump table
	VirtualProtect((LPVOID)0x43F924, 4, PAGE_READWRITE, &dwDummy);
	*(DWORD*)0x43F924 = (DWORD)Hook_QueryJumpTable;
}