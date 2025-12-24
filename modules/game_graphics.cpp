// sc2kfix modules/game_graphics.cpp: game graphics class
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Library-side calls to certain critical calls that would otherwise fail
// or cause anomalous behaviour if the equivalent remote-calls were used in the
// wrong context.

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

void CGraphics::DeleteStored_SC2K1996() {
	if (GRBitmap)
		::DeleteObject(GRBitmap);
	GRBitmap = 0;
	if (GRBitmapLoColor)
		::DeleteObject(GRBitmapLoColor);
	GRBitmapLoColor;
	if (GRpBitmapInfo) {
		free(GRpBitmapInfo);
		GRpBitmapInfo = 0;
	}
}

int CGraphics::CreateWithPalette_SC2K1996(LONG ibiWidth, LONG ibiHeight) {
	CSimcityAppPrimary *pSCApp;
	void *ppvBits;
	void *ppvBitsLoColor;
	BITMAPINFO *pBmi;
	CMFC3XPalette *pActPal;
	PALETTEENTRY palEnts[HICOLORCNT];
	BYTE *ippvBits;

	pSCApp = &pCSimcityAppThis;
	if (GRBitmap)
		Game_Graphics_DeleteObject(this);
	if (GRpBitmapInfo) {
		free(GRpBitmapInfo);
		GRpBitmapInfo = 0;
	}
	pBmi = (BITMAPINFO *)malloc(0x428);
	if (!pBmi) {
		ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): pBmi - memory allocation has failed.\n", ibiWidth, ibiHeight);
		return 0;
	}
	pBmi->bmiHeader.biSize = 40;
	pBmi->bmiHeader.biPlanes = 1;
	pBmi->bmiHeader.biBitCount = 8;
	pBmi->bmiHeader.biCompression = 0;
	pBmi->bmiHeader.biSizeImage = 0;
	pBmi->bmiHeader.biClrUsed = 0;
	pBmi->bmiHeader.biClrImportant = 0;
	pBmi->bmiHeader.biWidth = ibiWidth;
	pBmi->bmiHeader.biHeight = ibiHeight * GRorient;
	pActPal = Game_SimcityApp_GetActivePalette(pSCApp);
	GetPaletteEntries((HPALETTE)pActPal->m_hObject, 0, HICOLORCNT, palEnts);
	for (int i=0; i<HICOLORCNT; ++i) {
		pBmi->bmiColors[i].rgbRed = palEnts[i].peRed;
		pBmi->bmiColors[i].rgbGreen = palEnts[i].peGreen;
		pBmi->bmiColors[i].rgbBlue = palEnts[i].peBlue;
		pBmi->bmiColors[i].rgbReserved = 0;
	}
	ppvBits = 0;
	ppvBitsLoColor = 0;
	GRBitmap = CreateDIBSection(hDC_Global, pBmi, DIB_RGB_COLORS, &ppvBits, 0, 0);
	if (!GRBitmap) {
		ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): !GRBitmap - CreateDIBSection has failed.\n", ibiWidth, ibiHeight);
		free(pBmi);
		return 0;
	}
	if (bLoColor) {
		GRBitmapLoColor = CreateDIBSection(hDC_Global, pBmi, DIB_RGB_COLORS, &ppvBitsLoColor, 0, 0);
		if (!GRBitmapLoColor) {
			ConsoleLog(LOG_DEBUG, "CGraphics::CreateWithPalette_SC2K1996(%d, %d): !GRBitmapLoColor - CreateDIBSection has failed.\n", ibiWidth, ibiHeight);
			::DeleteObject(GRBitmap);
			free(pBmi);
			return 0;
		}
		GRpBitsLoColor = (BYTE *)ppvBitsLoColor;
		Game_Graphics_Set16ColorTable(this);
	}
	GRwidth = ibiWidth;
	GRheight = ibiHeight;
	GRpBits = (BYTE *)ppvBits;
	GRpAppPalette = Game_SimcityApp_GetActivePalette(pSCApp);
	ippvBits = (BYTE *)ppvBits;
	for (DWORD biSizeImage = 0; pBmi->bmiHeader.biSizeImage > biSizeImage; ++biSizeImage)
		*ippvBits++ = 0;
	GRpBitmapInfo = pBmi;
	if (GRpBitmapInfo->bmiHeader.biHeight < 0)
		GRpBitmapInfo->bmiHeader.biHeight = -GRpBitmapInfo->bmiHeader.biHeight;
	return 1;
}
