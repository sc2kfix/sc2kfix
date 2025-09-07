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

#pragma pack(push, 1)
class CSimcityAppPrimary : public CMFC3XWinApp {
public:
	HMODULE dwSCAhModule;
	DWORD dwSCAGameAutoSave;
	DWORD dwSCACursorGameHit;
	DWORD dwSCALoadCityMode;
	int iSCAGDCHorzRes;
	int iSCAGDCVertRes;
	DWORD dwSCAbForceBkgd;
	DWORD dwSCAGameDialogDoModalVar;
	DWORD bSCAPriscillaActivated;
	DWORD dwSCADragSuspendSim;
	DWORD dwSCAOnQuitSuspendSim;
	DWORD dwSCAMainFrameDestroyVar;
	DWORD dwSCAGameStarted;
	DWORD dwSCANoNewspapers;
	CMFC3XPalette *dwSCAMainPaletteFore;
	CMFC3XPalette *dwSCAMainPaletteBkgd;
	CMFC3XString dwSCACStringOne;
	CMFC3XString dwSCACStringTwo;
	CMFC3XString dwSCACStringThree;
	CMFC3XString dwSCACStringFour;
	DWORD dwSCASCURK;
	CSound *SCASNDLayer;
	DWORD dwSCASetNextStep;
	CMFC3XMultiDocTemplate *dwSCAMultiDocOne;
	CMFC3XMultiDocTemplate *dwSCAMultiDocTwo;
	DWORD dwSCAThirtyFour;
	DWORD dwSCAThirtyFive;
	HCURSOR dwSCACursors[30];
	DWORD dwSCASixtySix;
	int iSCAActiveCursor;
	DWORD dwSCAGameMusic;
	DWORD dwSCAGameSound;
	CMFC3XString *dwSCApCStringArrOne;
	DWORD dwSCASeventyOne;
	DWORD dwSCASeventyTwo;
	DWORD dwSCASeventyThree;
	DWORD dwSCASeventyFour;
	DWORD dwSCASeventyFive;
	DWORD dwSCASeventySix;
	DWORD dwSCASeventySeven;
	DWORD dwSCASeventyEight;
	DWORD dwSCASeventyNine;
	DWORD dwSCAEighty;
	DWORD dwSCAEightyOne;
	DWORD dwSCAEightyTwo;
	DWORD dwSCAEightyThree;
	DWORD dwSCAEightyFour;
	DWORD dwSCAEightyFive;
	DWORD dwSCAEightySix;
	DWORD dwSCAEightySeven;
	DWORD dwSCAEightyEight;
	DWORD dwSCAEightyNine;
	DWORD dwSCANinety;
	DWORD dwSCANinetyOne;
	DWORD dwSCANinetyTwo;
	DWORD dwSCANinetyThree;
	DWORD dwSCANinetyFour;
	DWORD dwSCANinetyFive;
	DWORD dwSCANinetySix;
	DWORD dwSCANinetySeven;
	DWORD dwSCANinetyEight;
	DWORD dwSCANinetyNine;
	DWORD dwSCAOneHundred;
	DWORD dwSCAOneHundredOne;
	DWORD dwSCAOneHundredTwo;
	DWORD dwSCAOneHundredThree;
	DWORD dwSCAOneHundredFour;
	DWORD dwSCAOneHundredFive;
	CMFC3XString *dwSCApCStringArrTwo;
	DWORD dwSCAStoredStringLengths[35];
	WORD wSCAGameSpeedLOW;
	WORD wSCAGameSpeedHIGH;
	DWORD dwSCASimulationTicking;
	DWORD dwSCAOneHundredFortyFour;
	DWORD dwSCAAnimationOnCycle;
	DWORD dwSCAAnimationOffCycle;
	DWORD dwSCAToggleTitleScreenAnimation;
	DWORD dwSCALastTick;
	int iSCAProgramStep;
	DWORD dwSCADoStepSkip;
	DWORD dwSCAMenuDialogStep;
	DWORD dwSCAMapModeVarCheck;
	DWORD dwSCAOnInitToggleToolBar;
	DWORD dwSCASysCmdOnQuitVar;
	__int16 wSCAInitDialogFinishLastProgramStep;
};
#pragma pack(pop)

#pragma pack(push, 1)
class CSimcityAppDemo : public CMFC3XWinApp {
public:
	DWORD dwSCAGameAutoSave;
	DWORD dwSCACursorGameHit;
	DWORD dwSCACityMode;
	int iSCAGDCHorzRes;
	int iSCAGDCVertRes;
	DWORD dwSCAbForceBkgd;
	DWORD dwSCAGameDialogDoModalVar;
	DWORD bSCAPriscillaActivated;
	DWORD dwSCADragSuspendSim;
	DWORD dwSCAOnQuitSuspendSim;
	DWORD dwSCAMainFrameDestroyVar;
	DWORD dwSCAGameStarted;
	DWORD dwSCANoNewspapers;
	CMFC3XPalette *dwSCAMainPaletteFore;
	CMFC3XPalette *dwSCAMainPaletteBkgd;
	CMFC3XString dwSCACStringOne;
	CMFC3XString dwSCACStringTwo;
	CMFC3XString dwSCACStringThree;
	CMFC3XString dwSCACStringFour;
	HMODULE dwSCASCURK;
	CSound *SCASNDLayer;
	DWORD dwSCASetNextStep;
	CMFC3XMultiDocTemplate *dwSCAMultiDocOne;
	CMFC3XMultiDocTemplate *dwSCAMultiDocTwo;
	DWORD dwSCAThirtyFour;
	DWORD dwSCAThirtyFive;
	HCURSOR dwSCACursors[30];
	DWORD dwSCASixtySix;
	int iSCAActiveCursor;
	DWORD dwSCAGameMusic;
	DWORD dwSCAGameSound;
	CMFC3XString *dwSCApCStringArrOne;
	DWORD dwSCASeventyOne;
	DWORD dwSCASeventyTwo;
	DWORD dwSCASeventyThree;
	DWORD dwSCASeventyFour;
	DWORD dwSCASeventyFive;
	DWORD dwSCASeventySix;
	DWORD dwSCASeventySeven;
	DWORD dwSCASeventyEight;
	DWORD dwSCASeventyNine;
	DWORD dwSCAEighty;
	DWORD dwSCAEightyOne;
	DWORD dwSCAEightyTwo;
	DWORD dwSCAEightyThree;
	DWORD dwSCAEightyFour;
	DWORD dwSCAEightyFive;
	DWORD dwSCAEightySix;
	DWORD dwSCAEightySeven;
	DWORD dwSCAEightyEight;
	DWORD dwSCAEightyNine;
	DWORD dwSCANinety;
	DWORD dwSCANinetyOne;
	DWORD dwSCANinetyTwo;
	DWORD dwSCANinetyThree;
	DWORD dwSCANinetyFour;
	DWORD dwSCANinetyFive;
	DWORD dwSCANinetySix;
	DWORD dwSCANinetySeven;
	DWORD dwSCANinetyEight;
	DWORD dwSCANinetyNine;
	DWORD dwSCAOneHundred;
	DWORD dwSCAOneHundredOne;
	DWORD dwSCAOneHundredTwo;
	DWORD dwSCAOneHundredThree;
	DWORD dwSCAOneHundredFour;
	DWORD dwSCAOneHundredFive;
	CMFC3XString *dwSCApCStringArrTwo;
	DWORD dwSCAStoredStringLengths;
	DWORD dwSCAOneHundredEight;
	DWORD dwSCAOneHundredNine;
	DWORD dwSCAOneHundredTen;
	DWORD dwSCAOneHundredEleven;
	DWORD dwSCAOneHundredTwelve;
	DWORD dwSCAOneHundredThirteen;
	DWORD dwSCAOneHundredFourteen;
	DWORD dwSCAOneHundredFifteen;
	DWORD dwSCAOneHundredSixteen;
	DWORD dwSCAOneHundredSeventeen;
	DWORD dwSCAOneHundredEighteen;
	DWORD dwSCAOneHundredNineteen;
	DWORD dwSCAOneHundredTwenty;
	DWORD dwSCAOneHundredTwentyOne;
	DWORD dwSCAOneHundredTwentyTwo;
	DWORD dwSCAOneHundredTwentyThree;
	DWORD dwSCAOneHundredTwentyFour;
	DWORD dwSCAOneHundredTwentyFive;
	DWORD dwSCAOneHundredTwentySix;
	DWORD dwSCAOneHundredTwentySeven;
	DWORD dwSCAOneHundredTwentyEight;
	DWORD dwSCAOneHundredTwentyNine;
	DWORD dwSCAOneHundredThirty;
	DWORD dwSCAOneHundredThirtyOne;
	DWORD dwSCAOneHundredThirtyTwo;
	DWORD dwSCAOneHundredThirtyThree;
	DWORD dwSCAOneHundredThirtyFour;
	DWORD dwSCAOneHundredThirtyFive;
	DWORD dwSCAOneHundredThirtySix;
	DWORD dwSCAOneHundredThirtySeven;
	DWORD dwSCAOneHundredThirtyEight;
	DWORD dwSCAOneHundredThirtyNine;
	DWORD dwSCAOneHundredForty;
	DWORD dwSCAOneHundredFortyOne;
	WORD wSCAGameSpeedLOW;
	WORD wSCAGameSpeedHIGH;
	DWORD dwSCASimulationTicking;
	DWORD dwSCAOneHundredFortyFour;
	DWORD dwSCAAnimationOnCycle;
	DWORD dwSCAAnimationOffCycle;
	DWORD dwSCAToggleTitleScreenAnimation;
	DWORD dwSCALastTick;
	int iSCAProgramStep;
	DWORD dwSCADoStepSkip;
	DWORD dwSCAMenuDialogStep;
	DWORD dwSCAMapModeVarCheck;
	DWORD dwSCAReturnToMenu;
	DWORD dwSCAOnInitToggleToolBar;
	DWORD dwSCASysCmdOnQuitVar;
	WORD wSCAInitDialogFinishLastProgramStep;
};
#pragma pack(pop)
