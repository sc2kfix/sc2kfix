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

struct shapedetail_t {
	union uShapeOffset {
		BYTE *shapePtr;
		int32_t shapeLong;
	};

	uShapeOffset shapeOffset;
	uint16_t shapeHeight;
	uint16_t shapeWidth;
};

#pragma pack(push, 1)
struct shapeinfo_t {
	int16_t shapeNum;
	shapedetail_t shapeDetail;
};
#pragma pack(pop)

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

class TEncodeDib : TBC45XDib {
public:
	uint8_t *mShapeBuf;
	DWORD mLength;
	int32_t mHeight;
};

#pragma pack(push, 1)
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
	TBC45XDerivedWindowFoot wndFoot;
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
	TBC45XDerivedWindowFoot wndFoot;
	DWORD dwUnknownTwo;
	TBC45XParWindow newWnd;
	DWORD dwEndBoundary;
};
#pragma pack(pop)
