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

#define MISCHOOK_SCURK_DEBUG_INTERNAL 1
#define MISCHOOK_SCURK_DEBUG_PICKANDPLACE 2
#define MISCHOOK_SCURK_DEBUG_PLACEANDCOPY 4

#define MISCHOOK_SCURK_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURK_DEBUG
#define MISCHOOK_SCURK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define MAX_EDNUM 184

extern UINT mischook_scurk_debug;

#pragma pack(push, 1)
class CWinGBitmap {
public:
	HBITMAP GRBitmap;
	HBITMAP GRBitmapLoColor;
	int GRlastPalUpdate;
	TBC45XPalette *GRpAppPalette;
	int GRwidth;
	int GRheight;
	int GRorient;
	int GRIsLockCnt;
	BYTE *GRpBits;
	BYTE *GRpBitsLoColor;
	WORD wGRDIBUsage;
};

class cEditableTileSet {
public:
	uint8_t *mTiles[SPRITE_COUNT + 10];
	char *mTileNames[MAX_EDNUM];
	int32_t mTileIsRenamed[MAX_EDNUM];
	sprite_archive_t *mTileSet;
	char *mFileName;
	int32_t *mDBIndexFromShapeNum;
	int32_t *mShapeNumFromEditableNum;
	int32_t *mTileSizeTable;
	int32_t mStartPos;
	int32_t mNumTiles;
};

class TEncodeDib : public TBC45XDib {
public:
	BYTE *mShapeBuf;
	DWORD mLength;
	int32_t mHeight;
};

class cScurkToolBox : public TBC45XParToolBox {
public:
	char *mOldHint;
	TBC45XDerivedWindowFoot __clFoot;
};

class cShowTileWindow : public TBC45XParWindow {
public:
	TBC45XRect *mRect[150];
	DWORD dwSelectedTiles[MAX_EDNUM];
	DWORD *dwPointerOne;
	DWORD *dwPointerTwo;
	TBC45XDib *mDibs[150];
	DWORD mNumTiles;
	cShowTileWindow *dwDestination;
	cEditableTileSet *mTileSet;
	DWORD dwSomethingOne[7];
	DWORD dwPosition;
	int nSomethingTwo[2];
	TBC45XDerivedWindowFoot __clFoot;
};

class TPlaceTileListDlg : public TBC45XParDialog {
public:
	TBC45XDib **pDibs;
	int nMaxTileButtons;
	cEditableTileSet *mWorkingTiles;
	TBC45XListBox *pListBox;
	int nLBButtonWidth;
	int nPosWidth;
	int nTileRow;
	int nMaxHitArea;
	int nCurPos;
	int nXPos;
	int mNumTiles;
	int nChldHndlorX;
	int nChldIDorY;
	int nSelected;
	TBC45XDerivedWindowFootNewWnd __wndFoot;
	DWORD dwUnknownThree;
};

class winscurkParMDIChild : public TBC45XParMDIChild {
public:
	DWORD mSelectedSet;
};

class winscurkMDIChild : public winscurkParMDIChild {
public:
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class winscurkPlaceWindow;
class winscurkMoverWindow;
class winscurkEditWindow;

class winscurkMDIClient : public TBC45XParMDIClient {
public:
	int ChildCount;
	TBC45XOpenSaveDialog::TData FileData;
	TBC45XDecoratedMDIFrame *mParent;
	int mNewCity;
	winscurkPlaceWindow *mPlaceWindow;
	winscurkMoverWindow *mMoverWindow;
	winscurkEditWindow *mEditWindow;
	PALETTEENTRY mSlowColors[16];
	PALETTEENTRY mFastColors[49];
	int mCyclingColors;
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class winscurkMDIFrame : public TBC45XDecoratedMDIFrame {

};

class TButtonPalette : public TBC45XParFloatingFrame {
public:
	cScurkToolBox *pScurkToolBox;
	TBC45XResId MenuResID;
	DWORD dwColumns;
	DWORD dwRows;
	DWORD dwTempOne;
	TBC45XWindow *pParentWnd;
	HMENU hToolMenu;
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class winscurkEditWindow;

class cPaintWindow : public TBC45XParWindow {
public:
	DWORD dwUndoBufferPreserved;
	TBC45XDib *pDibMask;
	BYTE *pEncodeDibBits;
	BYTE *pGraphicBits;
	BYTE *pDibBitsEmpty;
	BYTE *pBits;
	int nPenSizeHalf;
	BYTE *pDibBitsSmallCircle;
	BYTE *pDibBitsBigCircle;
	TBC45XRect shapeRect;
	TBC45XClientDC *pClientDC;
	TBC45XBitmap *pBitmap;
	int nVertScrollPos;
	int nHorzScrollPos;
	DWORD dwLButtonDown;
	DWORD dwPaintTool;
	int nPenSize;
	int iZoomLevel;
	int nMaxVertScrollPos;
	int nMaxHorzScrollPos;
	int nHeight;
	int bSnapToGrid;
	int bInValidDrawingArea;
	DWORD dwTempSeven;
	BYTE *pSomeBuffer;
	BYTE *pUndoBitsBuffer;
	winscurkEditWindow *pScurkEditParent;
	TBC45XPoint areaPointGridOne;
	TBC45XPoint areaPointGridTwo;
	TBC45XPoint gridCoords;
	TBC45XPoint areaPointMouse;
	TBC45XDib *pDibBuffer;
	DWORD dwTempNine;
	TBC45XDib *pDibSmallCircle;
	TBC45XDib *pDibBigCircle;
	TBC45XDib *pDibEmptyTile;
	CWinGBitmap *pGraphic;
	DWORD dwTempTen;
	int bShowClipGrid;
	DWORD dwTouching;
	DWORD dwTempEleven;
	int bShowDrawGrid;
	TEncodeDib *pEncodeDib;
	TBC45XDerivedWindowFoot __clFoot;
};

class cPaletteWindow : public TBC45XParWindow {
public:
	TBC45XDib *pDibOne;
	TBC45XDib *pDibThree;
	TBC45XDib *pDibFour;
	TBC45XDib *pDibTwo;
	float floatOne;
	float floatTwo;
	winscurkEditWindow *pScurkEditParent;
	int *pPaletteBuffer;
	BYTE colFrGrnd;
	BYTE colBkGrnd;
	TBC45XDerivedWindowFoot __clFoot;
};

class cPatternWindow : public TBC45XParWindow {
public:
	TBC45XDib *pDibOne[39];
	TBC45XDib *pDibTwo;
	TBC45XDib *pDibThree;
	TBC45XDib *pDibFour;
	TBC45XDib *pDibFive;
	TBC45XDib *pDibSix;
	DWORD dwOne;
	DWORD dwPatternButPosition;
	winscurkEditWindow *pScurkEditParent;
	TBC45XDerivedWindowFoot __clFoot;
};

class winscurkEditWindow : public winscurkParMDIChild {
public:
	int nEdNum;
	cPaintWindow *pPaintWindow;
	cPaletteWindow *pPaletteWindow;
	cPatternWindow *pPatternWindow;
	winscurkMDIClient *pScurkMDIParent;
	DWORD dwTempTwo[2];
	char *pCharBuf;
	int nSelectedBrush;
	BYTE *pSelectedDibBits;
	TBC45XDib *pDibOne;
	cScurkToolBox *pScurkToolBoxOne;
	cScurkToolBox *pScurkToolBoxTwo;
	TBC45XRect *pDibDimensions[8];
	DWORD dwTempThree[2];
	DWORD *dwPtrOne[8];
	DWORD dwTempFour[2];
	TBC45XDib *pDibTwo;
	DWORD dwTempFive[2];
	TBC45XPopupMenu *pMenuOne;
	TBC45XPopupMenu *pMenuTwo;
	TBC45XPopupMenu *pMenuThree;
	TBC45XPopupMenu *pMenuFour;
	TBC45XPopupMenu *pMenuFive;
	int nItemOne;
	int nItemTwo;
	int nItemThree;
	int nItemFour;
	int nItemFive;
	int nSelectedRow;
	int nSelectedTool;
	TBC45XToolBox *pToolBoxOne;
	TBC45XDib *pDibThree;
	DWORD dwTempSix;
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class winscurkMoverWindow : public winscurkParMDIChild {
public:
	DWORD dwTempOne[187];
	TBC45XToolBox *pTileSourceToolBox;
	TBC45XToolBox *pTileWorkingToolBox;
	DWORD dwTempTwo[6];
	cShowTileWindow *pTileSourceWindow;
	cShowTileWindow *pTileWorkingWindow;
	DWORD dwTempThree;
	TBC45XDib *mDibsOne;
	TBC45XDib *mDibsTwo;
	TBC45XDib *mDibsThree;
	TBC45XDib *mDibsFour;
	TBC45XDib *mDibsFive;
	TBC45XDib *mDibsSix;
	CWinGBitmap *pGraphicsOne;
	TBC45XPoint labelPt;
	CWinGBitmap *pGraphicsFour;
	DWORD dwTempFive;
	winscurkMDIClient *pScurkMDIParent;
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class winscurkPlaceWindow : public winscurkParMDIChild {
public:
	DWORD dwTempOne[16];
	TPlaceTileListDlg *pPlaceTileListDlg;
	winscurkMDIClient *pScurkMDIParent;
	char *pToolText;
	TButtonPalette *pToolBar;
	DWORD dwMapModeVarCheck;
	DWORD dwTempTwoTwo;
	DWORD dwGameStarted;
	DWORD dwIsZoomed;
	DWORD dwLeftMouseDownInGameArea;
	DWORD dwTempThreeOne;
	DWORD dwLeftMouseButtonDown;
	DWORD dwTempThreeThree;
	DWORD dwTempThreeFour;
	TBC45XRect scrollRect;
	char *pCityName;
	char *pCityFileName;
	char *pBufThree;
	DWORD dwTempFourOne;
	DWORD mWorkingSet;
	CWinGBitmap *pGraphics;
	TBC45XSize GraphicSize;
	int nInWidth;
	int nInHeight;
	BYTE *pGraphicBits;
	TBC45XRect AreaView;
	DWORD dwZoomLevel;
	TBC45XPoint tileCoordPt;
	TBC45XPoint mousePt;
	TBC45XPoint LastTileCoordPt;
	DWORD dwDragSuspendSim;
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class winscurkApp : public TBC45XApplication {
public:
	cEditableTileSet *mSourceTiles;
	cEditableTileSet *mWorkingTiles;
	TBC45XDib *mPaletteDIB;
	DWORD mClosing;
	DWORD mSoundOn;
	char *mExePath;
	TBC45XPalette *mScurkPalette;
	TBC45XPalette *mDeadPalette;
	DWORD mScurkRegistrationKey;
	HKEY mRegKey[5];
	BOOL mUseIniFile;
	char *m_currDir;
	char *mDefaultCityFile;
	char *mWavePath;
	char *mMayorsName;
	char *mCommandLineFileName;
	DWORD mGotCommandLineFile;
	char *mMiffPath;
	char *mCityPath;
	char *mHelpFile;
	DWORD mLocalDLL;
	DWORD mLocalModule;
	winscurkMDIClient *mdiClient;
	DWORD *Printer;
	DWORD Printing;
};
#pragma pack(pop)
