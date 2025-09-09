// sc2kfix include/mfc3xhelp.h: helper classes/structs/macros/functions for manipulating MFC 3.x state
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#define _CN_COMMAND 0
#define _CN_COMMAND_UI -1

// Hierarchy for reference:
//
// NOTE: Any classes that don't contain the 'MFC3X' term
//       are from the target program, however they're present
//       in order to better account for the general structure.
//
// struct CMFC3XRuntimeClass;
//
// class CMFC3XPoint;
// class CMFC3XRect;
// class CMFC3XSize;
//
// class CMFC3XObject;
//		class CMFC3XException;
//			class CMFC3XMemoryException;
//			class CMFC3XNotSupportedException;
//			class CMFC3XArchiveException;
//			class CMFC3XFileException;
//			class CMFC3XResourceException;
//			class CMFC3XUserException;
//
//		class CMFC3XFile;
//			class CMFC3XStdioFile;
//			class CMFC3XMemFile;
//
//		class CMFC3XGdiObject;
//			class CMFC3XPen;
//			class CMFC3XBrush;
//			class CMFC3XFont;
//			class CMFC3XBitmap;
//			class CMFC3XPalette;
//			class CMFC3XRgn;
//		
//		class CMFC3XDC;
//			class CMFC3XClientDC;
//			class CMFC3XWindowDC;
//			class CMFC3XPaintDC;
//		
//		class CMFC3XMenu;
//
//		class CMFC3XCmdTarget;
//			class CMFC3XWnd; 
//				class CMFC3XDialog;
//					class CAboutDialog;
//					class CMovieDialog;
//					class CGameDialog;
//						class CBridgeSelectDIalog;
//						class CBudgetDialog;
//						class CBudgetAdvisorDialog;
//						class CBudgetEducationDialog;
//						class CBudgetFireDialog;
//						class CBudgetFundDialog;
//						class CBudgetHealthDialog;
//						class CBudgetInformationDialog;
//						class CBudgetOrdinanceDialog;
//						class CBudgetPoliceDialog;
//						class CBudgetTransportDialog;
//						class CBudgetZoneTaxSubDialog;
//						class CCityIndustryDialog;
//						class CCityMapDialog;
//						class CEventDialog;
//						class CGeneralInfoDialog;
//						class CInflightDialog;
//						class CInitialDialog;
//						class CNeighbourDialog;
//						class CNewGameDialog;
//						class CNewspaperDialog;
//						class COwnerInfoDialog;
//						class CPopulationDialog;
//						class CPowerPlantDialog;
//						class CQueryGeneralDialog;
//						class CQuerySpecificDialog;
//						class CScenarioDialog;
//						class CSelectArcologyDialog;
//						class CSimGraphDialog;
//
//				class CMFC3XStatic;
//				class CMFC3XListBox;
//				class CMFC3XComboBox;
//				class CMFC3XEdit;
//					class CSimcityEdit;
//						class CSimcityEditOne;
//						class CSimcityEditTwo;
//				class CMFC3XScrollBar;
//
//				class CMFC3XButton
//					class CMFC3XBitmapButton;
//
//				class CMFC3XControlBar;
//					class CMFC3XStatusBar;
//					class CMFC3XToolBar;
//					class CMFC3XDialogBar;
//						class CStatusControlBar;
//					class CMyToolBar;
//						class CCityToolBar;
//						class CMapToolBar;
//
//				class CMFC3XFrameWnd;
//					class CMFC3XMDIFrameWnd;
//						class CMainFrame;
//
//					class CMFC3XMDIChildWnd;
//						class CMyMDIChildWnd;
//					class CMFC3XMiniFrameWnd;
// 
//				class CMFC3XView;
//					class CMFC3XScrollView;
//
//					class CSimcityView;
//
//				class CSimcityWnd;
//
//		class CMFC3XWinThread;
//			class CMFC3XWinApp;
//				class CSimcityApp;
//
//		class CMFC3XDocTemplate;
//			class CMFC3XSingleDocTemplate;
//			class CMFC3XMultiDocTemplate;
//		
//		class CMFC3XDocument;
//			class CSimcityDoc;
//			class CEngine;
//			class CSimGraphData;
// 
// 
// 
// class CMFC3XCmdUI;
//		class CMFC3XTestCmdUi;
//
// class CMFC3XDataExchange;
//
// class CMFC3XString;

// Placeholders and forward-declarations

class CMFC3XControlBar;
class CMFC3XDialogBar;
class CMFC3XDockBar;
class CMFC3XDockContext;
class CMFC3XObject;
class CMFC3XOleDropTarget;
class CMFC3XOleFrameHook;
class CMFC3XOleMessageFilter;
class CMFC3XRecentFileList;
class CMFC3XWnd;

// Reimplementation of the MFC 3.x message map entry structure.
typedef struct {
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	UINT nSig;
	void *pfn;
} MFC3X_AFX_MSGMAP_ENTRY;

#pragma pack(push, 1)
struct CMFC3XPlex {
	CMFC3XPlex* pNext;
	UINT nMax;
	UINT nCur;
};
#pragma pack(pop)

class CMFC3XPoint : public tagPOINT {

};

class CMFC3XRect : public tagRECT {

};

class CMFC3XSize : public tagSIZE {

};

class CMFC3XObject {
	DWORD *__vftable;
};

struct CMFC3XRuntimeClass {
	const char *m_lpszClassName;
	int m_nObjectSize;
	unsigned int m_wSchema;
	CMFC3XObject *(__stdcall *m_pfnCreateObject)();
	CMFC3XRuntimeClass *m_pBaseClass;
	CMFC3XRuntimeClass *m_pNextClass;
};

// Reimplementation of the CString class from MFC 3.x.
class CMFC3XString {
public:
	LPTSTR m_pchData;
	int m_nDataLength;
	int m_nAllocLength;
};

class CMFC3XFile : public CMFC3XObject {
public:
	enum OpenFlags {
		modeRead =          0x0000,
		modeWrite =         0x0001,
		modeReadWrite =     0x0002,
		shareCompat =       0x0000,
		shareExclusive =    0x0010,
		shareDenyWrite =    0x0020,
		shareDenyRead =     0x0030,
		shareDenyNone =     0x0040,
		modeNoInherit =     0x0080,
		modeCreate =        0x1000,
		typeText =          0x4000, // typeText and typeBinary are used in
		typeBinary =   (int)0x8000 // derived classes only
	};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
	};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

	unsigned int m_hFile;

	enum BufferCommand { bufferRead, bufferWrite, bufferCommit, bufferCheck };

	int m_bCloseOnDelete;
};

class CMFC3XGdiObject : public CMFC3XObject {
public:
	HGDIOBJ m_hObject;
};

class CMFC3XBitmap : public CMFC3XGdiObject {

};

class CMFC3XBrush : public CMFC3XGdiObject {

};

class CMFC3XFont : public CMFC3XGdiObject {

};

class CMFC3XPalette : public CMFC3XGdiObject {
	
};

class CMFC3XPen : public CMFC3XGdiObject {

};

class CMFC3XDC : public CMFC3XObject {
public:
	HDC m_hDC;
	HDC m_hAttribDC;
	BOOL m_bPrinting;
};

class CMFC3XMenu : public CMFC3XObject {
public:
	HMENU m_hMenu;
};

class CMFC3XPtrArray : public CMFC3XObject {
public:
	void **m_pData;
	int m_nSize;
	int m_nMaxSize;
	int m_nGrowBy;
};


class CMFC3XPtrList : public CMFC3XObject {
public:
	struct CMFC3XNode
	{
		CMFC3XNode* pNext;
		CMFC3XNode* pPrev;
		void* data;
	};
	CMFC3XNode* m_pNodeHead;
	CMFC3XNode* m_pNodeTail;
	int m_nCount;
	CMFC3XNode* m_pNodeFree;
	struct CMFC3XPlex* m_pBlocks;
	int m_nBlockSize;
};

class CMFC3XCmdTarget : public CMFC3XObject {
public:
	DWORD m_dwRef;
	IUnknown *m_pOuterUnknown;
	DWORD m_xInnerUnknown;
	struct XDispatch
	{
		DWORD m_vtbl;
		size_t m_nOffset;
	} m_xDispatch;
	BOOL m_bResultExpected;
};

class CMFC3XDocTemplate : public CMFC3XCmdTarget {
public:
	CMFC3XObject *m_pAttachedFactory;
	HMENU m_hMenuInPlace;
	HACCEL m_hAccelInPlace;
	HMENU m_hMenuEmbedding;
	HACCEL m_hAccelEmbedding;
	HMENU m_hMenuInPlaceServer;
	HACCEL m_hAccelInPlaceServer;
	unsigned int m_nIDResource;
	unsigned int m_nIDServerResource;
	CMFC3XRuntimeClass *m_pDocClass;
	CMFC3XRuntimeClass *m_pFrameClass;
	CMFC3XRuntimeClass *m_pViewClass;
	CMFC3XRuntimeClass *m_pOleFrameClass;
	CMFC3XRuntimeClass *m_pOleViewClass;
	CMFC3XString m_strDocStrings;
};

class CMFC3XMultiDocTemplate : CMFC3XDocTemplate {
	HMENU m_hMenuShared;
	HACCEL m_hAccelTable;
	CMFC3XPtrList m_docList;
	unsigned int m_nUntitledCount;
};

class CMFC3XDocument : public CMFC3XCmdTarget {
public:
	CMFC3XString m_strTitle;
	CMFC3XString m_strPathName;
	CMFC3XDocTemplate *m_pDocTemplate;
	CMFC3XPtrList m_viewList;
	int m_bModified;
	int m_bAutoDelete;
	int m_bEmbedded;
};

class CMFC3XWnd : public CMFC3XCmdTarget {
public:
	HWND m_hWnd;
	enum AdjustType { adjustBorder = 0, adjustOutside = 1 };
	enum RepositionFlags
	{ reposDefault = 0, reposQuery = 1, reposExtra = 2 };
	HWND m_hWndOwner;
	WNDPROC m_pfnSuper;
	BOOL m_bTempHidden;
	CMFC3XOleDropTarget *m_pDropTarget;
};

class CMFC3XView : public CMFC3XWnd {
public:
	CMFC3XDocument *m_pDocument;
};

#pragma pack(push, 1)
class CMFC3XFrameWnd : public CMFC3XWnd {
public:
	int m_bAutoMenuEnable;
	int m_nWindow;
	HMENU m_hMenuDefault;
	HACCEL m_hAccelTable;
	unsigned int m_dwPromptContext;
	BOOL m_bHelpMode;
	BOOL m_bStayActive;
	CMFC3XFrameWnd *m_pNextFrameWnd;
	CMFC3XRect m_rectBorder;
	CMFC3XOleFrameHook *m_pNotifyHook;
	CMFC3XPtrList m_listControlBars;
	BOOL m_bModalDisable;
	int m_nShowDelay;
	unsigned int m_nIDHelp;
	unsigned int m_nIDTracking;
	unsigned int m_nIDLastMessage;
	CMFC3XView *m_pViewActive;
	int (__stdcall *m_lpfnCloseProc)(CMFC3XFrameWnd *);
	unsigned int m_cModalStack;
	HWND__ **m_phWndDisable;
	HMENU__ *m_hMenuAlt;
	CMFC3XString m_strTitle;
	int m_bInRecalcLayout;
	CMFC3XRuntimeClass *m_pFloatingFrameClass;
	unsigned int m_nIdleFlags;
};
#pragma pack(pop)

class CMFC3XDockContext {
public:
	DWORD *__vftable /*VFT*/;
	CMFC3XPoint m_ptLast;
	CMFC3XRect m_rectLast;
	CMFC3XSize m_sizeLast;
	int m_bDitherLast;
	CMFC3XRect m_rectDragHorz;
	CMFC3XRect m_rectDragVert;
	CMFC3XRect m_rectFrameDragHorz;
	CMFC3XRect m_rectFrameDragVert;
	CMFC3XControlBar *m_pBar;
	CMFC3XFrameWnd *m_pDockSite;
	unsigned int m_dwDockStyle;
	unsigned int m_dwOverDockStyle;
	unsigned int m_dwStyle;
	int m_bFlip;
	int m_bForceFrame;
	CMFC3XDC *m_pDC;
	int m_bDragging;
};

class CMFC3XControlBar : public CMFC3XWnd {
public:
	int m_bAutoDelete;
	int m_cxLeftBorder;
	int m_cxRightBorder;
	int m_cyTopBorder;
	int m_cyBottomBorder;
	int m_cxDefaultGap;
	int m_nCount;
	void *m_pData;
	unsigned int m_nStateFlags;
	unsigned int m_dwStyle;
	unsigned int m_dwDockStyle;
	CMFC3XFrameWnd *m_pDockSite;
	CMFC3XDockBar *m_pDockBar;
	CMFC3XDockContext *m_pDockContext;
};

class CMFC3XDialogBar : public CMFC3XControlBar {
public:
	CMFC3XSize m_sizeDefault;
};

class CMFC3XDockBar : public CMFC3XControlBar {
public:
	BOOL m_bFloating;
	CMFC3XPtrArray m_arrBars;
	BOOL m_bLayoutQuery;
};

class CMFC3XMDIFrameWnd : public CMFC3XFrameWnd {
public:
	HWND m_hWndMDIClient;
};

class CMFC3XDialog : public CMFC3XWnd {
public:
	unsigned int m_nIDHelp;
	void *m_hDialogTemplate;
	const DLGTEMPLATE *m_lpDialogTemplate;
	CMFC3XWnd *m_pParentWnd;
	HWND m_hWndTop;
};

class CMFC3XCommonDialog : public CMFC3XDialog {

};

class CMFC3XFileDialog : public CMFC3XCommonDialog {
public:
	OPENFILENAME m_ofn;
	int m_bOpenFileDialog;
	CMFC3XString m_strFilter;
	char m_szFileTitle[64];
	char m_szFileName[260];
};

class CMFC3XStatic : public CMFC3XWnd {

};

class CMFC3XScrollBar : public CMFC3XWnd {

};

class CMFC3XWinThread : public CMFC3XCmdTarget {
public:
	CMFC3XWnd *m_pMainWnd;
	CMFC3XWnd *m_pActiveWnd;
	BOOL m_bAutoDelete;
	HANDLE m_hThread;
	DWORD m_nThreadID;
	MSG m_msgCur;
	LPVOID m_pThreadParams;
	UINT (__cdecl *m_pfnThreadProc)(void *);
	CMFC3XPoint m_ptCursorLast;
	UINT m_nMsgLast;
};

class CMFC3XWinApp : public CMFC3XWinThread {
public:
	HINSTANCE m_hInstance;
	HINSTANCE m_hPrevInstance;
	LPTSTR m_lpCmdLine;
	int m_nCmdShow;
	LPCTSTR m_pszAppName;
	LPCTSTR m_pszRegistryKey;
	BOOL m_bHelpMode;
	LPCTSTR m_pszExeName;
	LPCTSTR m_pszHelpFilePath;
	LPCTSTR m_pszProfileName;
	HGLOBAL m_hDevMode;
	HGLOBAL m_hDevNames;
	DWORD m_dwPromptContext;
	int m_nWaitCursorCount;
	HICON m_hcurWaitCursorRestore;
	CMFC3XRecentFileList *m_pRecentFileList;
	CMFC3XPtrList m_templateList;
	unsigned __int16 m_atomApp;
	unsigned __int16 m_atomSystemTopic;
	unsigned int m_nNumPreviewPages;
	unsigned int m_nSafetyPoolSize;
	void (CALLBACK* m_lpfnOleFreeLibraries)();
	void (CALLBACK* m_lpfnOleTerm)(BOOL);
	CMFC3XOleMessageFilter *m_pMessageFilter;
};

class CMFC3XCmdUI {
public:
	DWORD *__vftable;

	unsigned int m_nID;

	unsigned int m_nIndex;

	CMFC3XMenu *m_pMenu;
	CMFC3XMenu *m_pSubMenu;

	CMFC3XWnd *m_pOther;

	int m_bEnableChanged;
	int m_bContinueRouting;
	unsigned int m_nIndexMax;

	CMFC3XMenu *m_pParentMenu;
};

class CMFC3XTestCmdUI : public CMFC3XCmdUI {
public:
	BOOL m_bEnabled;
};

class CMFC3XDataExchange {
public:
	BOOL m_bSaveAndValidate;
	CMFC3XWnd *m_pDlgWnd;
	HWND m_hWndLastControl;
	BOOL m_bEditLastControl;
};
