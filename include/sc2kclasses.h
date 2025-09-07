#pragma once

#define HICOLORCNT 256
#define LOCOLORCNT 16

// This specifical structure is the equivalent
// of the LOGPALETTE struct from the WinAPI
// except the pPalEnts array is preset to
// 256 entries.
typedef struct tagLOGPAL {
	WORD wVersion;
	WORD wNumPalEnts;
	PALETTEENTRY pPalEnts[HICOLORCNT];
} LOGPAL, *PLOGPAL;

// Internal structure in the game
// that "may" also originate within
// older WinAPI handling cases, however
// wPos - RGB index
// pe - PaletteEntry (red, green, blue, flags)
#pragma pack(push, 1)
typedef struct testColStruct {
	WORD wPos;
	PALETTEENTRY pe;
} colStruct;
#pragma pack(pop)

// Internal structure from the very old
// programming manual for Win3.x and prior.
// Mostly for handling the setting of the
// colour table via the very old Escape()
// call.
#pragma pack(push, 1)
typedef struct COLORTABLE_STRUCT {
	WORD Index;
	DWORD rgb;
} colTable;
#pragma pack(pop)

// The sound/midi class; placed here for now.
#pragma pack(push, 1)
class CSound {
public:
	HWND *dwSNDhWnd;
	BOOL bSNDPlaySound;
	int iSNDCurrSoundID;
	CMFC3XString dwSNDSoundString;
	void *dwSNDBufferTool;
	int iSNDToolSoundID;
	void *dwSNDBufferActionThing;
	BOOL bSNDWasPlaying;
	int iSNDActionThingSoundID;
	void *dwSNDBufferClick;
	void *dwSNDBufferDestruction;
	void *dwSNDBufferGeneral;
	int iSNDGeneralSoundID;
	DWORD dwSNDUnknownOne;
	WORD wSNDMCIDevID;
	DWORD dwSNDMCIError;
	DWORD dwSNDUnknownTwo;
	CMFC3XString dwSNDMusicString;
};
#pragma pack(pop)

// The graphics class; placed here for now.
#pragma pack(push, 1)
class CGraphics {
public:
	HBITMAP GRBitmap;
	HBITMAP GRBitmapLoColor;
	int GRlastPalUpdate;
	CMFC3XPalette *GRpAppPalette;
	int GRwidth;
	int GRheight;
	int GRorient;
	int GRIsLockCnt;
	BYTE *GRpBits;
	BYTE *GRpBitsLoColor;
	BITMAPINFO *GRpBitmapInfo;
};
#pragma pack(pop)

// Reimplementation of an abstracted C string (not to be confused with the MFC CString) used in
// the original SimCity 2000 code.
class CSimString {
public:
	char *pStr;
};
