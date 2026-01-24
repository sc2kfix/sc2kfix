#pragma once

/*
 * ****** NOTE: The classes referenced here allow for a reasonable
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

// enums

enum {
	DEMO_ONIDLE_STATE_INGAME = -1,
	DEMO_ONIDLE_STATE_MAPMODE,
	DEMO_ONIDLE_STATE_DISPLAYMAXIS,
	DEMO_ONIDLE_STATE_WAITMAXIS,
	DEMO_ONIDLE_STATE_DISPLAYTITLE,
	DEMO_ONIDLE_STATE_DIALOGFINISH,
	DEMO_ONIDLE_STATE_DISPLAYREGISTRATION,
	DEMO_ONIDLE_STATE_CLOSEREGISTRATION,
	DEMO_ONIDLE_STATE_PENDINGACTION,
	DEMO_ONIDLE_STATE_NONE_8,
	DEMO_ONIDLE_STATE_NONE_9,
	DEMO_ONIDLE_STATE_LOADCITY_RETURN,
	DEMO_ONIDLE_STATE_NEWCITY_RETURN,
	DEMO_ONIDLE_STATE_EDITNEWMAP_RETURN,
	DEMO_ONIDLE_STATE_LOADSCENARIO_RETURN,
	DEMO_ONIDLE_STATE_MENUDIALOG,
	DEMO_ONIDLE_STATE_FROMCMDLINE,
	DEMO_ONIDLE_STATE_RETURNTOSTART
};

enum {
	ONIDLE_STATE_INGAME = -1,
	ONIDLE_STATE_MAPMODE,
	ONIDLE_STATE_DISPLAYMAXIS,
	ONIDLE_STATE_WAITMAXIS,
	ONIDLE_STATE_DISPLAYTITLE,
	ONIDLE_STATE_DIALOGFINISH,
	ONIDLE_STATE_DISPLAYREGISTRATION,
	ONIDLE_STATE_CLOSEREGISTRATION,
	ONIDLE_STATE_PENDINGACTION,
	ONIDLE_STATE_NONE_8,
	ONIDLE_STATE_NONE_9,
	ONIDLE_STATE_LOADCITY_RETURN,
	ONIDLE_STATE_NEWCITY_RETURN,
	ONIDLE_STATE_EDITNEWMAP_RETURN,
	ONIDLE_STATE_LOADSCENARIO_RETURN,
	ONIDLE_STATE_MENUDIALOG,
	ONIDLE_STATE_FROMCMDLINE,
	ONIDLE_STATE_INTROVIDEO,
	ONIDLE_STATE_DISPLAYINFLIGHT,
	ONIDLE_STATE_CLOSEINFLIGHT,

	ONIDLE_STATE_COUNT
};

#define ONIDLE_STATE_MAX (ONIDLE_STATE_COUNT + 1)

enum {
	ONIDLE_INITIALDIALOG_NONE,
	ONIDLE_INITIALDIALOG_LOADCITY,
	ONIDLE_INITIALDIALOG_NEWCITY,
	ONIDLE_INITIALDIALOG_EDITNEWMAP,
	ONIDLE_INITIALDIALOG_LOADSCENARIO,
	ONIDLE_INITIALDIALOG_ONQUIT,
	ONIDLE_INITIALDIALOG_LOADTILESET,
	ONIDLE_INITIALDIALOG_MOVIES,
	ONIDLE_INITIALDIALOG_SC2KFIXSETTINGS,

	ONIDLE_INITIALDIALOG_COUNT
};

#define ONIDLE_INITIALDIALOG_MAX (ONIDLE_INITIALDIALOG_COUNT)

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
	HWND hMainWnd;
	BOOL bSNDPlaySound;
	int iSNDCurrSoundID;
	CMFC3XString dwSNDSoundString;
	void *dwSNDBufferTool;
	int iSNDToolSoundID;
	void *dwSNDBufferActionThing;
	BOOL bSNDWasPlaying;
	int iSNDActionThingSoundID;
	void *dwSNDBufferClick;
	void *dwSNDBufferExplosion;
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

	void DeleteStored_SC2K1996();
	int CreateWithPalette_SC2K1996(LONG ibiWidth, LONG ibiHeight);
	CMFC3XDC *GetDC_SC2K1996();
	void ReleaseDC_SC2K1996(CMFC3XDC *pDC);
};
#pragma pack(pop)

// Currency string handling class.
class CCurrencyString {
public:
	char *pStr;
};

#pragma pack(push, 1)
class CSimcityAppPrimary : public CMFC3XWinApp {
public:
	HMODULE dwSCAhModule;
	DWORD dwSCAGameAutoSave;
	DWORD dwSCACursorGameHit;
	DWORD dwSCACMDLineLoadMode;
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
	CMFC3XString dwSCACStringDriveCurrentWorkingDirectory;
	CMFC3XString dwSCACStringHomeDirectory;
	CMFC3XString dwSCACStringTargetTypePath;
	CMFC3XString dwSCACStringLang;
	DWORD dwSCASCURK;
	CSound *SCASNDLayer;
	DWORD dwSCASetNextStep;
	CMFC3XMultiDocTemplate *dwSCAMultiDocSC2;
	CMFC3XMultiDocTemplate *dwSCAMultiDocSCN;
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
	DWORD dwSCABackgroundColourCyclingActive;
	DWORD dwSCALastTick;
	int iSCAProgramStep;
	DWORD dwSCADoStepSkip;
	int iSCAMenuDialogStep;
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
	DWORD dwSCACMDLineLoadMode;
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
	CMFC3XString dwSCACStringDriveCurrentWorkingDirectory;
	CMFC3XString dwSCACStringHomeDirectory;
	CMFC3XString dwSCACStringTargetTypePath;
	CMFC3XString dwSCACStringLang;
	DWORD dwSCASCURK;
	CSound *SCASNDLayer;
	DWORD dwSCASetNextStep;
	CMFC3XMultiDocTemplate *dwSCAMultiDocSC2;
	CMFC3XMultiDocTemplate *dwSCAMultiDocSCN;
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
	DWORD dwSCABackgroundColourCyclingActive;
	DWORD dwSCALastTick;
	int iSCAProgramStep;
	DWORD dwSCADoStepSkip;
	int iSCAMenuDialogStep;
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
	CMFC3XPoint MyTBMaxButtonHitPoint;
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
	CMFC3XPoint MyTBToolBarShapePt;
};

class CCityToolBar : public CMyToolBar {
public:
	DWORD dwCTBControlsDisabled;
	DWORD dwCTBToolBarTitleDrag;
	DWORD dwCTBOne;
	CMFC3XPoint CTBGrabPoint;
	CMFC3XPoint CTBBorderPoint;
	DWORD dwCTBTwo;
	CMainFrame *pCTBMainFrame;
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
	DWORD dwMTBToolBarTitleDrag;
	DWORD dwMTBControlsDisabled;
	CMFC3XPoint MTBGrabPoint;
	CMFC3XPoint MTBBorderPoint;
	DWORD dwMTBTwentyNine;
	CMainFrame *pMTBMainFrame;
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
	DWORD dwNDPaperSectionLoadFailure;
	CMFC3XFont dwNDCFonts[33];
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
	CGraphics *SCVGraphics;
	DWORD bSCVViewActive;
	void *dwSCVThree;
	void *pSCVGraphicLockDIBRes;
	LONG dwSCVGraphicWidth;
	LONG dwSCVGraphicHeight;
	CMFC3XScrollBar *SCVScrollBarVert;
	CMFC3XScrollBar *SCVScrollBarHorz;
	CMFC3XStatic *SCVStaticOne;
	CMFC3XRect SCVScrollBarVertRectBar;
	CMFC3XRect SCVScrollBarVertRectUpButton;
	CMFC3XRect SCVScrollBarVertRectDownButton;
	CMFC3XRect SCVScrollPosVertRectThumb;
	CMFC3XRect SCVScrollBarHorzRectBar;
	CMFC3XRect SCVScrollBarHorzRectLeftButton;
	CMFC3XRect SCVScrollBarHorzRectRightButton;
	CMFC3XRect SCVScrollPosHorzRectThumb;
	CMFC3XRect SCVScrollPosRect;
	CMFC3XRect SCVStaticRect;
	DWORD dwSCVLeftMouseButtonDown;
	DWORD dwSCVLeftMouseDownInGameArea;
	DWORD dwSCVCursorInGameArea;
	CMFC3XPoint SCVMousePoint;
	DWORD dwSCVRightClickMenuOpen;
	CMFC3XPoint SCVRealPoint;
	DWORD dwSCVSixtyOne[6];
	CMFC3XRect SCVAreaView;
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

class CQuerySpecificDialog : public CGameDialog {
public:
	DWORD dwQSDTileID;
	DWORD dwQSDMapX;
	DWORD dwQSDMapY;
	CGraphics *dwQSDCGraphicsOne;
	CMFC3XPoint dwQSDPointOne;
	CMFC3XEdit dwQSDCEdit;
	CMFC3XButton dwQSDCButton;
	CMFC3XString dwQSDCStringOne;
};

class CQueryGeneralDialog : public CGameDialog {
public:
	CMFC3XPoint dwQGPointOne;
	DWORD QGDthree;
	DWORD QGDfour;
	CGraphics *pQGDGraphic;
	CMFC3XDC QGDDC;
};

// !!! At the moment the CMovieDialog class is structured
// to achieve alignment, it is heavily subject to change.

class CMovieDialog : public CMFC3XDialog {
public:
	HICON hMovIcon;
	BITMAPINFO *pOWMainBitmapInfo;
	CMFC3XRect MovRECT[5];
	BITMAPINFO *pOWButtonBitmapInfo[10];
	CMFC3XPalette MovPalette;
	int iButtonOne;
	int iButtonTwo;
};
