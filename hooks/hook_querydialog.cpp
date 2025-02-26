// sc2kfix hooks/hook_querydialog.cpp: hook for new query dialog features
// (c) 2025 github.com/araxestroy - released under the MIT license

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
			if (dwMapXBIT[iTileX]->b[iTileY].iWater && dwMapXBIT[iTileX]->b[iTileY].iSaltWater)
				strTileHeader = "Salt water";
			else if (dwMapXBIT[iTileX]->b[iTileY].iWater)
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
		strTileInfo =  GetZoneName(dwMapXZON[iTileX]->b[iTileY].iZoneType);
		strTileInfo += "\n";
		
		// Altitude/depth
		if (dwMapXBIT[iTileX]->b[iTileY].iWater && dwMapALTM[iTileX]->w[iTileY].iLandAltitude < wWaterLevel)
			strTileInfo += std::to_string(100 * (wWaterLevel - dwMapALTM[iTileX]->w[iTileY].iLandAltitude) - 50);
		else if (dwMapXTER[iTileX]->iTileID[iTileY] && dwMapXTER[iTileX]->iTileID[iTileY] < 0x10)
			strTileInfo += std::to_string(25 * (4 * (dwMapALTM[iTileX]->w[iTileY].iLandAltitude - wWaterLevel) + 4));
		else
			strTileInfo += std::to_string(100 * (dwMapALTM[iTileX]->w[iTileY].iLandAltitude - wWaterLevel) + 50);
		strTileInfo += " feet ";
		if (dwMapXBIT[iTileX]->b[iTileY].iWater && dwMapALTM[iTileX]->w[iTileY].iLandAltitude < wWaterLevel)
			strTileInfo += "deep ";
		strTileInfo += "(ALTM: ";
		strTileInfo += HexPls(*(WORD*)(&dwMapALTM[iTileX]->w[iTileY]), 4);
		strTileInfo += ")\n";

		// Land value
		strTileInfo += "$";
		strTileInfo += std::to_string(dwMapXVAL[iTileX >> 1]->bBlock[iTileY >> 1] + 1);
		strTileInfo += ",000/acre\n";

		// Crime
		strTileInfo += GetLowHighScale(dwMapXCRM[iTileX >> 1]->bBlock[iTileY >> 1]);
		strTileInfo += " (XCRM: ";
		strTileInfo += std::to_string(dwMapXCRM[iTileX >> 1]->bBlock[iTileY >> 1]);
		strTileInfo += ")\n";

		// Pollution
		strTileInfo += GetLowHighScale(dwMapXPLT[iTileX >> 1]->bBlock[iTileY >> 1]);
		strTileInfo += " (XPLT: ";
		strTileInfo += std::to_string(dwMapXPLT[iTileX >> 1]->bBlock[iTileY >> 1]);
		strTileInfo += ")\n\n";

		// Raw XZON data
		switch (dwMapXZON[iTileX]->b[iTileY].iCorners) {
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
		strTileInfo += HexPls(dwMapXZON[iTileX]->b[iTileY].iZoneType, 1);
		strTileInfo += "\n";

		// XBIT
		if (!*(BYTE*)(&dwMapXBIT[iTileX]->b[iTileY]))
			strTileInfo += "none (XBIT: 0x00)\n";
		else {
			// XXX - this code sucks, more so than the rest of this function
			int i = 0;

			if (dwMapXBIT[iTileX]->b[iTileY].iPowerable) {
				i++;
				strTileInfo += "powerable ";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iPowered) {
				i++;
				strTileInfo += "powered ";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iPiped) {
				i++;
				strTileInfo += "piped ";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iWatered) {
				i++;
				strTileInfo += "watered ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iXVALMask) {
				i++;
				strTileInfo += "xvalmask ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iWater) {
				i++;
				strTileInfo += "water ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iRotated) {
				i++;
				strTileInfo += "rotated ";
				if (i == 5)
					strTileInfo += "\n";
			}
			if (dwMapXBIT[iTileX]->b[iTileY].iSaltWater) {
				i++;
				strTileInfo += "saltwater ";
				if (i == 5)
					strTileInfo += "\n";
			}
			strTileInfo += "(XBIT: ";
			strTileInfo += HexPls(*(BYTE*)(&dwMapXBIT[iTileX]->b[iTileY]), 1);
			strTileInfo += ")\n";
			if (i < 5)
				strTileInfo += "\n";
		}

		// XUND
		if (dwMapXUND[iTileX]->iTileID[iTileY] > 35)
			strTileInfo += "Unknown";
		else
			strTileInfo += szUndergroundNames[dwMapXUND[iTileX]->iTileID[iTileY]];
		strTileInfo += " (XUND: ";
		strTileInfo += HexPls(dwMapXUND[iTileX]->iTileID[iTileY], 2);
		strTileInfo += ")\n";

		// Microsim info
		if (dwMapXTXT[iTileX]->bTextOverlay[iTileY] < 0x34 || dwMapXTXT[iTileX]->bTextOverlay[iTileY] > 0xC8)
			strTileInfo += "None\nN/A\nN/A\nN/A\nN/A";
		else {
			int iMicrosimID = dwMapXTXT[iTileX]->bTextOverlay[iTileY] - 0x33;
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
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_ADVANCEDQUERY), NULL, AdvancedQueryDialogProc);
		__asm {
			// Skip the regular call (and its stack cleanup)
			popa
			push 0x43F837
			retn
		}
	}

	__asm {
		// Go onto the regular call otherwise
		popa
		push 0x43F80C
		retn
	}
}

void InstallQueryHooks(void) {
	// Install the query hook into the jump table
	VirtualProtect((LPVOID)0x43F924, 4, PAGE_READWRITE, &dwDummy);
	*(DWORD*)0x43F924 = (DWORD)Hook_QueryJumpTable;
}