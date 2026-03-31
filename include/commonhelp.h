#pragma once

// Common structures between supported programs.
// Common wrapped functions.

#define PAL_IDX(x) (16 * (15 - x / 16) + x % 16)

// BC45X forward declarations.
class BC45Xstring;
class TBC45XPalette;
class TBC45XDC;
class TBC45XModule;
class TBC45XParWindow;
class TBC45XWindow;
class TBC45XWindowDC;
class TBC45XClientDC;
class TBC45XParDialog;
class TBC45XDialog;
class TBC45XParListBox;
class TBC45XParMDIClient;
class TBC45XMDIChild;

// SCURK forward declarations.
class TPlaceTileListDlg;
class cEditableTileSet;
class winscurkMoverWindow;
class winscurkPlaceWindow;
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
void *__cdecl L_BOR_WRP_gAllocBlock(size_t nSz);
void *__cdecl L_BOR_WRP_gResizeBlock(BYTE *pBlock, size_t nSz);
void __cdecl L_BOR_WRP_gFreeBlock(void *pBlock);
void __stdcall L_BOR_WRP_gUpdateWaitWindow();
void L_BOR_WRP_DC_SelectObjectPalette(TBC45XDC *pThis, TBC45XPalette *pPal, int nVal);
unsigned int L_BOR_WRP_Window_DefaultProcessing(TBC45XParWindow *pThis);
LRESULT L_BOR_WRP_Window_HandleMessage(TBC45XParWindow *pThis, unsigned int msg, WPARAM wParam, LPARAM lParam);
void L_BOR_WRP_Window_SetCursor(TBC45XParWindow *pThis, TBC45XModule *pModule, const char *pResID);
void L_BOR_WRP_WindowDC_Destruct(TBC45XWindowDC *pThis, char c);
TBC45XClientDC *L_BOR_WRP_ClientDC_Construct(TBC45XClientDC *pThis, HWND hWnd);
void L_BOR_WRP_Dialog_SetupWindow(TBC45XParDialog *pThis);
void L_BOR_WRP_Dialog_SetCaption(TBC45XParDialog *pThis, char *pCaption);
void L_BOR_WRP_Dialog_EvClose(TBC45XParDialog *pThis);
int L_BOR_WRP_ListBox_GetSelIndex(TBC45XParListBox *pThis);
LRESULT L_BOR_WRP_ListBox_GetString(TBC45XParListBox *pThis, char *pString, int nIndex);
int L_BOR_WRP_ListBox_SetItemData(TBC45XParListBox *pThis, int nIndex, unsigned int uItemData);
int L_BOR_WRP_ListBox_AddString(TBC45XParListBox *pThis, const char *pString);
TBC45XMDIChild *L_BOR_WRP_MDIClient_GetActiveMDIChild(TBC45XParMDIClient *pThis);

// SCURK-only - wrappers above, common reconstructed calls below from scurkfix_common.cpp
__int16 *L_SCURK_WRP_GetwTileObjects();
WORD *L_SCURK_WRP_GetwColFastCnt();
WORD *L_SCURK_WRP_GetwColSlowCnt();
__int16 *L_SCURK_WRP_GetwToolNum();
__int16 *L_SCURK_WRP_GetwToolValue();
winscurkApp *L_SCURK_WRP_winscurkApp_GetPointerToClass();
BC45Xstring *L_SCURK_WRP_GetTAppInitCmdLine();
char *L_SCURK_WRP_EditableTileSet_GetLongName(cEditableTileSet *pThis, int nEdNum);
void L_SCURK_WRP_winscurkPlaceWindow_ClearCurrentTool(winscurkPlaceWindow *pThis);
void L_SCURK_WRP_winscurkMDIClient_RotateColors(winscurkMDIClient *pThis, BOOL bFast);
TBC45XPalette *L_SCURK_WRP_winscurkApp_GetPalette(winscurkApp *pThis);
void L_SCURK_WRP_winscurkApp_ScurkSound(winscurkApp *pThis, int nSoundID);
winscurkPlaceWindow *L_SCURK_WRP_winscurkApp_GetPlaceWindow(winscurkApp *pThis);

void L_SCURK_gDebugOut(const char *fmt, va_list args);
char *L_SCURK_OwlMainCommandLineFix(char **pArgs, int nArgs);
void L_SCURK_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis);
void L_SCURK_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis);
void L_SCURK_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis);
void L_SCURK_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis);
LONG L_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName);
TBC45XWindow *L_SCURK_MoverWindow_DisableMaximizeBox(TBC45XWindow *pThis);
void L_SCURK_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi);
void L_SCURK_BCDialog_CmCancel(TBC45XDialog *pThis);

// SC2K-only
