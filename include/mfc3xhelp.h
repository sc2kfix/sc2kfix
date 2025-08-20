#pragma once

#define _CN_COMMAND 0
#define _CN_COMMAND_UI -1

// Reimplementation of the MFC 3.x message map entry structure.
typedef struct {
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	UINT nSig;
	void *pfn;
} MFC3X_AFX_MSGMAP_ENTRY;

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

	DWORD *m_pMenu;
	DWORD *m_pSubMenu;

	DWORD *m_pOther;

	int m_bEnableChanged;
	int m_bContinueRouting;
	unsigned int m_nIndexMax;

	DWORD *m_pParentMenu;
};

class CMFC3XTestCmdUI : public CMFC3XCmdUI {
public:
	BOOL m_bEnabled;
};
