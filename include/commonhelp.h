#pragma once

// Common structures between supported programs.
// Common wrapped functions.

// Key:
// R_ - Remote (typically used for wrapper remote vars or calls)
// L_ - Local (Used for locally implemented partial or whole calls)
// 'WRP' - Wrapper (Only present if it is indeed a wrapper)
// 'ADDR' - Address reference (either a void *pointer or DWORD - depends on context)
//
// Format:
// Origin_Base_Wrapper_Name
// L_SCURK_Name
// R_SCURK_WRP_Name
// R_SCURK_ADDR_Name
// R_BOR_WRP_Name

#define HIGHNIBBLE(x) ((x >> 4) & 0xF)
#define LOWNIBBLE(x) ((x) & 0xF)

#define PAL_IDX(x) (16 * (15 - x / 16) + x % 16)

// BC45X forward declarations.
class BC45Xstring;
class TBC45XPoint;
class TBC45XRect;
class TBC45XSize;
class TBC45XPalette;
class TBC45XDC;
class TBC45XDib;
class TBC45XModule;
class TBC45XParWindow;
class TBC45XWindow;
class TBC45XParFrameWindow;
class TBC45XFrameWindow;
class TBC45XWindowDC;
class TBC45XClientDC;
class TBC45XParDialog;
class TBC45XDialog;
class TBC45XParListBox;
class TBC45XParMDIClient;
class TBC45XMDIChild;
class TBC45XMDIFrame;
class TBC45XResId;
class TBC45XCommandEnabler;
class TBC45XApplication;

// SCURK forward declarations.
class TPlaceTileListDlg;
class CWinGBitmap;
class TEncodeDib;
class cEditableTileSet;
class winscurkEditWindow;
class winscurkMDIFrame;
class winscurkMoverWindow;
class winscurkPlaceWindow;
class cPaintWindow;
class winscurkMDIClient;
class winscurkApp;

#pragma pack(push, 1)
typedef struct {
	union uSpriteOffset {
		BYTE *sprPtr;
		int32_t sprLong;
	};
	uSpriteOffset sprOffset;
	WORD wHeight;
	WORD wWidth;
} sprite_header_t;

typedef struct {
	__int16 nSprNum;
	sprite_header_t sprHeader;
} sprite_file_header_t;

typedef struct {
	__int16 nSprites;
	sprite_file_header_t pData[1];
} sprite_archive_t;

typedef struct {
	char szTypeHead[4];
	DWORD dwSize;
	char szSC2KHead[4];
} tilesetMainHeader_t;

typedef struct {
	char szHead[4];
	DWORD dwSize;
} tilesetHeadInfo_t;

typedef struct {
	char szHead[4];
	DWORD dwSize;
	WORD nMaxChunks;
} tilesetTileInfo_t;

typedef struct {
	DWORD dwSize;
	char szHead[4];
} tilesetChunkHeader_t;

typedef struct {
	WORD nSpriteID;
	WORD nWidth;
	WORD nHeight;
	DWORD dwSize;
} tilesetShapHeader_t;

typedef struct {
	WORD nShapNum;
	WORD nNameLength;
} tilesetNameHeader_t;

typedef struct {
	char szHead[4];
	DWORD dwSize;
	char pBuf;
} tileMem_t;

typedef struct {
	WORD nMaxChunks;
	tileMem_t tileMem;
} tilesetMem_t;

typedef struct {
	WORD nSpriteID;
	WORD nWidth;
	WORD nHeight;
	DWORD dwSize;
	char pBuf;
} tileShap_t;

typedef struct {
	WORD nTileNameID;
	WORD nNameLength;
	char pBuf;
} tileName_t;
#pragma pack(pop)

typedef struct {
	DWORD dwOffset;
	BYTE height;
	BYTE width;
	WORD wPad;
} tilHeader_t;

typedef struct {
	BYTE largeArc[12];
	DWORD dwLargeSize;
	BYTE largeHed[12];
	DWORD dwLargeOffset;
	BYTE otherArc[12];
	DWORD dwOtherSize;
	BYTE otherHed[12];
	DWORD dwOtherOffset;
	BYTE smallArc[12];
	DWORD dwSmallSize;
	BYTE smallHed[12];
	DWORD dwSmallOffset;
	BYTE urkTextFile[12];
	DWORD dwUrkTextOffset;
	BYTE readOnlyFile[12];
	DWORD dwReadOnlyOffset;
} tilMainStruct_t;

#pragma pack(push, 1)
typedef struct {
	BYTE nChunkMode;
	BYTE nCount;
	WORD pBuf;
} spriteDosData_t;

typedef struct {
	BYTE nCount;
	BYTE nChunkMode;
	WORD pBuf;
} spriteData_t;
#pragma pack(pop)

#define SPRITEDOSDATA(x) ((spriteDosData_t *)x)
#define SPRITEDATA(x) ((spriteData_t *)x)

#define MIF_CM_EMPTY 0
#define MIF_CM_NEWROWSTART 1
#define MIF_CM_ENDOFSPRITE 2
#define MIF_CM_SKIPPIXELS  3
#define MIF_CM_PROCPIXELS  4

#define TIL_CM_ENDOFSPRITE 0
#define TIL_CM_SKIPPIXELS  0x4
#define TIL_CM_PROCPIXELS  0xC
#define TIL_CM_NEWROWSTART 0x10

// General
static inline BOOL IsEven(int nVal) {
	return (nVal % 2) == 0 ? TRUE : FALSE;
}

static inline BOOL IsEvenUnsigned(DWORD nVal) {
	return (nVal % 2) == 0 ? TRUE : FALSE;
}

// MFC


// Borland
void *__cdecl R_BOR_WRP_gAllocBlock(size_t nSz);
void *__cdecl R_BOR_WRP_gResizeBlock(BYTE *pBlock, size_t nSz);
void __cdecl R_BOR_WRP_gFreeBlock(void *pBlock);
void __stdcall R_BOR_WRP_gUpdateWaitWindow();
void R_BOR_WRP_DC_SelectObjectPalette(TBC45XDC *pThis, TBC45XPalette *pPal, int nVal);
LRESULT R_BOR_WRP_Window_EvCommand(TBC45XWindow *pThis, DWORD dwID, HWND hWndCtl, DWORD dwNotifyCode);
unsigned int R_BOR_WRP_Window_DefaultProcessing(TBC45XParWindow *pThis);
LRESULT R_BOR_WRP_Window_HandleMessage(TBC45XParWindow *pThis, unsigned int msg, WPARAM wParam, LPARAM lParam);
TBC45XWindow *R_BOR_WRP_GetWindowPtr(HWND hWndTarget, TBC45XApplication *pApp);
void R_BOR_WRP_Window_SetCursor(TBC45XParWindow *pThis, TBC45XModule *pModule, const char *pResID);
void R_BOR_WRP_WindowDC_Destruct(TBC45XWindowDC *pThis, char c);
TBC45XClientDC *R_BOR_WRP_ClientDC_Construct(TBC45XClientDC *pThis, HWND hWnd);
void R_BOR_WRP_Dialog_SetupWindow(TBC45XParDialog *pThis);
void R_BOR_WRP_Dialog_SetCaption(TBC45XParDialog *pThis, char *pCaption);
void R_BOR_WRP_Dialog_EvClose(TBC45XParDialog *pThis);
HWND R_BOR_WRP_FrameWindow_GetCommandTarget(TBC45XFrameWindow *pThis);
int R_BOR_WRP_ListBox_GetSelIndex(TBC45XParListBox *pThis);
LRESULT R_BOR_WRP_ListBox_GetString(TBC45XParListBox *pThis, char *pString, int nIndex);
int R_BOR_WRP_ListBox_SetItemData(TBC45XParListBox *pThis, int nIndex, unsigned int uItemData);
int R_BOR_WRP_ListBox_AddString(TBC45XParListBox *pThis, const char *pString);
TBC45XMDIChild *R_BOR_WRP_MDIClient_GetActiveMDIChild(TBC45XParMDIClient *pThis);
BOOL R_BOR_MDIFrame_SetMenu(TBC45XMDIFrame *pThis, HMENU hMenu);
char *R_BOR_strnewdup(char *pStr, size_t nSz);
void *R_BOR_Op_New(size_t nSz);

// SCURK-only - wrappers above, common reconstructed calls below from scurkfix_common.cpp
__int16 *R_SCURK_WRP_GetwTileObjects();
WORD *R_SCURK_WRP_GetwColFastCnt();
WORD *R_SCURK_WRP_GetwColSlowCnt();
__int16 *R_SCURK_WRP_GetwToolNum();
__int16 *R_SCURK_WRP_GetwToolValue();
TBC45XDib *R_SCURK_WRP_mTileBack();
winscurkApp *R_SCURK_WRP_winscurkApp_GetPointerToClass();
BC45Xstring *R_SCURK_WRP_GetTAppInitCmdLine();
DWORD R_SCURK_ADDR_FrameWindow_EvCommand_To_TDecoratedFrame_EvCommand();
int R_SCURK_WRP_WinGBitmap_Width(CWinGBitmap *pThis);
int R_SCURK_WRP_WinGBitmap_Height(CWinGBitmap *pThis);
TEncodeDib *R_SCURK_WRP_EncodeDib_Construct_Dimens(TEncodeDib *pThis, LONG nWidth, LONG nHeight, DWORD dwColors, WORD wMode);
void R_SCURK_WRP_EncodeDib_Destruct(TEncodeDib *pThis, char c);
void R_SCURK_WRP_EncodeDib_mFillAt(TEncodeDib *pThis, TBC45XPoint *pt);
void R_SCURK_WRP_EncodeDib_mFillLine(TEncodeDib *pThis, TBC45XPoint *pt, BYTE Bit);
int R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(TEncodeDib *pThis);
void R_SCURK_WRP_EncodeDib_mShrink(TEncodeDib *pThis, TBC45XDib *pInDib, int nScaleBy);
void R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(TEncodeDib *pThis, TEncodeDib *pEncDib);
void R_SCURK_WRP_EncodeDib_mEncodeShape(TEncodeDib *pThis, WORD shapeHeight, WORD shapeWidth, WORD nOffSet);
int R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(cEditableTileSet *pThis, int nShapNum);
char *R_SCURK_WRP_EditableTileSet_GetLongName(cEditableTileSet *pThis, int nEdNum);
int R_SCURK_WRP_EditableTileSet_mGetShapeWidth(cEditableTileSet *pThis, int nEdNum);
void R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Dib(cEditableTileSet *pThis, TBC45XDib *pDib, int nEdNum);
void R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Graphic(cEditableTileSet *pThis, CWinGBitmap *pGraphic, int nEdNum);
void R_SCURK_WRP_EditableTileSet_mBuildSmallMedTiles(cEditableTileSet *pThis);
TBC45XDib *R_SCURK_WRP_EditWindow_mGetForegroundPattern(winscurkEditWindow *pThis);
int R_SCURK_WRP_EditWindow_mGetShapeWidth(winscurkEditWindow *pThis);
BYTE R_SCURK_WRP_EditWindow_mGetForegroundColor(winscurkEditWindow *pThis);
BYTE R_SCURK_WRP_EditWindow_mGetBackgroundColor(winscurkEditWindow *pThis);
void R_SCURK_WRP_PlaceWindow_ClearCurrentTool(winscurkPlaceWindow *pThis);
void R_SCURK_WRP_PaintWindow_mPreserveToUndoBuffer(cPaintWindow *pThis);
void R_SCURK_WRP_PaintWindow_mApplyTileToScreen(cPaintWindow *pThis);
void R_SCURK_WRP_PaintWindow_mClearTile(cPaintWindow *pThis, int nTileBase);
void R_SCURK_WRP_PaintWindow_mClipDrawing(cPaintWindow *pThis);
void R_SCURK_WRP_PaintWindow_mScreenToDib(TBC45XPoint *pt, cPaintWindow *pThis, TBC45XPoint *pPoint);
void R_SCURK_WRP_PaintWindow_mPutPixel(cPaintWindow *pThis, TBC45XRect *pRect, BYTE foreColor, BYTE backColor);
void R_SCURK_WRP_PaintWindow_mDraw(cPaintWindow *pThis);
void R_SCURK_WRP_PaintWindow_mEncodeShape(cPaintWindow *pThis, int nZoomLevel);
void R_SCURK_WRP_winscurkMDIClient_RotateColors(winscurkMDIClient *pThis, BOOL bFast);
TBC45XPalette *R_SCURK_WRP_winscurkApp_GetPalette(winscurkApp *pThis);
void R_SCURK_WRP_winscurkApp_ScurkSound(winscurkApp *pThis, int nSoundID);
winscurkPlaceWindow *R_SCURK_WRP_winscurkApp_GetPlaceWindow(winscurkApp *pThis);
winscurkEditWindow *R_SCURK_WRP_winscurkApp_GetEditWindow(winscurkApp *pThis);

void L_SCURK_gDebugOut(const char *fmt, va_list args);
char *L_SCURK_OwlMainCommandLineFix(char **pArgs, int nArgs);
extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis);
extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis);
extern "C" void __cdecl Hook_SCURK_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis);
extern "C" void __cdecl Hook_SCURK_EncodeDib_mFillAt(TEncodeDib *pThis, TBC45XPoint *pt);
extern "C" void __cdecl Hook_SCURK_EncodeDib_mFillLine(TEncodeDib *pThis, TBC45XPoint *pt, BYTE Bit);
extern "C" int __cdecl Hook_SCURK_EncodeDib_mDetermineShapeHeight(TEncodeDib *pThis);
extern "C" void __cdecl Hook_SCURK_EncodeDib_mShrink(TEncodeDib *pThis, TBC45XDib *pInDib, int nScaleBy);
extern "C" void __cdecl Hook_SCURK_EncodeDib_mEncodeShape(TEncodeDib *pThis, WORD shapeHeight, WORD shapeWidth, WORD nOffSet);
extern "C" void __cdecl Hook_SCURK_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis);
extern "C" LONG __cdecl Hook_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName);
extern "C" int __cdecl Hook_SCURK_EditableTileSet_mWriteToMIFFFile(cEditableTileSet *pThis, LPCSTR lpPathName);
extern "C" int __cdecl Hook_SCURK_EditableTileSet_mReadFromMIFFFile(cEditableTileSet *pThis, LPCSTR lpPathName);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadShapeFromDib_PostBuild(cEditableTileSet *pThis, int nDBID, TEncodeDib *pEncDib);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadShapeFromDib_Paint(cEditableTileSet *pThis, int nEdNum, cPaintWindow *pPaintWnd);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Dib(cEditableTileSet *pThis, TBC45XDib *pDib, int nDBID);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Graphic(cEditableTileSet *pThis, CWinGBitmap *pGraphic, int nDBID);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mRenderShapeToTile(cEditableTileSet *pThis, TBC45XDib *pDib, int nEdNum);
extern "C" void __cdecl Hook_SCURK_EditableTileSet_mReadFromDOSFile(cEditableTileSet *pThis, LPCSTR lpPathName);
void L_SCURK_InitDOSMacPaletteIdxTable();
extern "C" void __cdecl Hook_SCURK_PaintWindow_mFill(cPaintWindow *pThis, TBC45XPoint *pPoint);
extern "C" void __cdecl Hook_SCURK_PaintWindow_mClipDrawing(cPaintWindow *pThis);
extern "C" void __cdecl Hook_SCURK_PaintWindow_mEncodeShape(cPaintWindow *pThis, int nZoomLevel);
extern "C" int __cdecl Hook_SCURK_winscurkMDIFrame_AssignMenu(winscurkMDIFrame *pThis, TBC45XResId menuResID);
TBC45XWindow *L_SCURK_MoverWindow_DisableMaximizeBox(TBC45XWindow *pThis);
extern "C" void __cdecl Hook_SCURK_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi);
extern "C" void __cdecl Hook_SCURK_CommandEnabler_Enable(TBC45XCommandEnabler *pThis);
extern "C" void __cdecl Hook_SCURK_BCDialog_CmCancel(TBC45XDialog *pThis);
extern "C" LRESULT __cdecl Hook_SCURK_FrameWindow_EvCommand(TBC45XFrameWindow *pThis, DWORD id, HWND hWndCtl, DWORD notifyCode);

// SC2K-only
