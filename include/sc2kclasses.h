#pragma once

/*
 * ****** NOTE: The classes referenced here allow for a reasonably
 *              method of accessing certain class variables.
 *
 *              Not all defined variables in the classes are accessible.
 *              Those that are reasonably generic such as 'ThirtyFour'
 *              for instance are for padding purposes in order to
 *              ensure alignment.
 */

#define HICOLORCNT 256
#define LOCOLORCNT 16

// Forward declaration
class CMainFrame;

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
	CMFC3XString dwSCACStringLang;
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
	CMFC3XString dwSCApCStringLongMonths[12];
	CMFC3XString dwSCApCStringShortMonths[12];
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
	CMFC3XString dwSCACStringLang;
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
	CMFC3XString dwSCApCStringLongMonths[12];
	CMFC3XString dwSCApCStringShortMonths[12];
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
	__int16 wSCAInitDialogFinishLastProgramStep;
};
#pragma pack(pop)

class CMyToolBar : public CMFC3XControlBar {
public:
	int iMyTBMenuButtonPos;
	int iMyTBLastButtonPos;
	CMFC3XPoint dwMyTBPointFour;
	DWORD dwMyTBButtonMenu;
	CGraphics *cGMyTBCGraphicsNormal;
	CGraphics *cGMyTBCGraphicsDisabled;
	CGraphics *cGMyTBCGraphicsPressed;
	DWORD dwMyTBButtonPressed;
	DWORD dwMyTBten;
	DWORD dwMyTBeleven;
	DWORD dwMyTBButtonFace;
	DWORD dwMyTBButtonShadow;
	DWORD dwMyTBButtonHighlighted;
	DWORD dwMyTBButtonText;
	DWORD dwMyTBWindowFrame;
	DWORD dwMyTBseventeen;
	DWORD dwMyTBeighteen;
	CMFC3XPoint dwMyTBPointThree;
	DWORD dwMyTBControlsDisabled;
	DWORD dwMyTBToolBarTitleDrag;
	tagPOINT dwMyTBPointOne;
	tagPOINT dwMyTBPointTwo;
};

class CCityToolBar : public CMyToolBar {
public:
	tagPOINT dwCTBPointThree;
	CMainFrame *dwCTBMainFrame;
	DWORD dwCTBcxRightBorder;
	DWORD dwCTBthirtyone;
	CMFC3XMenu dwCTBMenuOne;
	CMFC3XString dwCTBString[15];
	DWORD dwCTBseventysix;
	DWORD dwCTBseventyseven;
	DWORD dwCTBseventyeight;
	DWORD dwCTBseventynine;
	DWORD dwCTBeighty;
	DWORD dwCTBeightyone;
	DWORD dwCTBeightytwo;
	DWORD dwCTBeightythree;
	DWORD dwCTBeightyfour;
	DWORD dwCTToolSelection[15];
};

class CMapToolBar : public CMyToolBar {
public:
	DWORD dwMTBTwentyNine;
	CMainFrame *dwMTBMainFrame;
	DWORD dwMTBcxRightBorder;
};

class CStatusControlBar : public CMFC3XDialogBar {
public:
	CMFC3XString dwSCBCStringCtrlSelection;
	CMFC3XString dwSCBCStringNotification;
	CMFC3XString dwSCBCStringWeather;
	COLORREF dwSCBColorCtrlSelection;
	COLORREF dwSCBColorNotification;
	COLORREF dwSCBColorWeather;
};

class CGameDialog : public CMFC3XDialog {
public:
	DWORD dwGDOne;
};

class CNewspaperDialog : public CGameDialog {
public:
	CGraphics *dwNDCGraphicsOne;
	CMFC3XBitmapButton dwNDCBitmapButton;
	DWORD dwNDPaperChoice;
	DWORD dwNDPaperSectionOpen;
	CMFC3XFont *dwNDCFonts;
};

class CJokeDialog : public CMFC3XDialog {
public:
	CGraphics *dwJDGraphics;
	CMFC3XRect dwJDRECTOne;
	DWORD dwJDsix;
	DWORD dwJDseven;
	DWORD dwJDeight;
	DWORD dwJDnine;
	DWORD dwJDPrepare;
};

class CSimcityWnd : public CMFC3XWnd {
public:
	CGraphics *m_pSCWGraphics;
	CMFC3XSize SCWSize;
};

class CMainFrame : public CMFC3XMDIFrameWnd {
public:
	DWORD *dwMFSimGraphDialog; // CSimGraphDialog
	DWORD *dwMFPopulationDialog; // CPopulationDialog
	DWORD *dwMFCityMapDialog; // CCityMapDialog
	DWORD *dwMFNeighbourDialog; // CNeighbourDialog
	DWORD *dwMFCityIndustryDialog; // CCityIndustryDialog
	CMFC3XPalette *dwMFEight;
	CGraphics *dwMFCGraphicsOne;
	CMFC3XPalette *dwMFnine;
	DWORD dwMFTimerActive;
	UINT_PTR dwMFuIDEvent;
	UINT dwMFuDelay;
	UINT dwMFuPeriod;
	CStatusControlBar dwMFStatusControlBar;
	BOOL bMFShowStatusBar;
	CCityToolBar dwMFCityToolBar;
	DWORD dwMFOneHundredEightyFour;
	DWORD dwMFOneHundredEightyFive;
	DWORD dwMFOneHundredEightySix;
	CMapToolBar dwMFMapToolBar;
	DWORD dwMFTwoHundredFortyTwo;
	DWORD dwMFToolBarsCreated;
	DWORD dwMFTwoHundredFortyFour;
	CSimcityWnd dwMFCSimcityWnd;
	LONG iMFbiWidth;
	LONG iMFbiHeight;
	CMFC3XSize dwMFCSize;
	tagPOINT dwMFPointOne;
};

#pragma pack(push, 1)
class CSimcityView : public CMFC3XView {
public:
	CGraphics *dwSCVCGraphics;
	DWORD bSCVViewActive;
	DWORD dwSCVThree;
	void *dwSCVLockDIBRes;
	LONG dwSCVWidth;
	LONG dwSCVHeight;
	CMFC3XScrollBar *dwSCVScrollBarVert;
	CMFC3XScrollBar *dwSCVScrollBarHorz;
	CMFC3XStatic *dwSCVStaticOne;
	CMFC3XRect dwSCVScrollBarVertRectOne;
	CMFC3XRect dwSCVScrollBarVertRectTwo;
	CMFC3XRect dwSCVScrollBarVertRectThree;
	CMFC3XRect dwSCVScrollPosVertRect;
	CMFC3XRect dwSCVScrollBarHorzRectOne;
	CMFC3XRect dwSCVScrollBarHorzRectTwo;
	CMFC3XRect dwSCVScrollBarHorzRectThree;
	CMFC3XRect dwSCVScrollPosHorzRect;
	CMFC3XRect dwSCVScrollPosRect;
	CMFC3XRect dwSCVStaticRect;
	DWORD dwSCVLeftMouseButtonDown;
	DWORD dwSCVLeftMouseDownInGameArea;
	DWORD dwSCVCursorInGameArea;
	CMFC3XPoint dwSCVMousePoint;
	DWORD dwSCVRightClickMenuOpen;
	CMFC3XPoint dwSCVRealPoint;
	DWORD dwSCVSixtyOne[3];
	CMFC3XRect dwSCVRECTOne;
	int iSCVYCoord;
	CMFC3XPoint dwSCVAdjustedPoint;
	WORD wSCVZoomLevel;
	DWORD dwSCVIsZoomed;
};
#pragma pack(pop)

// NOTE: Variable use-case here is not know.
//       Perhaps it was for internal debugging at the time.
//       However it allows for alignment at present.

class CEngine : public CMFC3XDocument {
public:
	DWORD dwEngineOne;
	DWORD dwEngineTwo;
	DWORD dwEngineThree;
	DWORD dwEngineFour;
	WORD wEngineOne;
	WORD wEngineTwo;
};

class CSimcityDoc : public CMFC3XDocument {
public:
	CEngine *pSimEngine;
};
