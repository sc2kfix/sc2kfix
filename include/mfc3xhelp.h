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
//				class CMFC3XButton;
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

// Reimplementation of the MFC 3.x message map entry structure.
typedef struct {
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	UINT nSig;
	void *pfn;
} MFC3X_AFX_MSGMAP_ENTRY;

class CMFC3XObject {
	DWORD *__vftable;
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

class CMFC3XMenu : public CMFC3XObject
{
public:
	HMENU m_hMenu;
};

// Reimplementation of the CString class from MFC 3.x.
class CMFC3XString {
public:
	LPTSTR m_pchData;
	int m_nDataLength;
	int m_nAllocLength;
};

class CMFC3XCmdUI {
public:
	DWORD *__vftable;

	unsigned int m_nID;

	unsigned int m_nIndex;

	CMFC3XMenu *m_pMenu;
	CMFC3XMenu *m_pSubMenu;

	DWORD *m_pOther;

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
	DWORD *m_pDlgWnd;
	HWND m_hWndLastControl;
	BOOL m_bEditLastControl;
};
