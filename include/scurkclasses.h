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

#pragma pack(push, 1)
class CWinGBitmap {
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

struct shapedetail_t {
	union uShapeOffset {
		BYTE *shapePtr;
		int32_t shapeLong;
	};

	uShapeOffset shapeOffset;
	uint16_t shapeHeight;
	uint16_t shapeWidth;
};

struct shapeinfo_t {
	int16_t shapeNum;
	shapedetail_t shapeDetail;
};

struct tilesetheader_t {
	int16_t numShapes;
	shapeinfo_t infoShapes;
};

class cEditableTileSet {
public:
	uint8_t *mTiles[1510];
	char *mTileNames[184];
	int32_t *mTileIsRenamed[184];
	tilesetheader_t *mTileSet;
	char *mFileName;
	int32_t *mDBIndexFromShapeNum;
	int32_t *mShapeNumFromEditableNum;
	int32_t *mTileSizeTable;
	int32_t mStartPos;
	int32_t mNumTiles;
};

class TEncodeDib : public TBC45XDib {
public:
	uint8_t *mShapeBuf;
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
	DWORD dwSelectedTiles[184];
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

class winscurkMDIClient : public TBC45XParMDIClient {
public:
	int ChildCount;
	TBC45XOpenSaveDialog::TData FileData;
	TBC45XDecoratedMDIFrame *mParent;
	int mNewCity;
	winscurkPlaceWindow *mPlaceWindow;
	winscurkMoverWindow *mMoverWindow;
	DWORD *mEditWindow;
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
