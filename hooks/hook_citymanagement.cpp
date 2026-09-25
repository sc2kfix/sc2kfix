// sc2kfix hooks/hook_citymanagement.cpp: hooks to do with city management: budget,
// ordinances, graphs, population and advisors.
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>
#include <stack>

#include <sc2kfix.h>
#include "../resource.h"

extern int nOwnDrwDlg;
extern CMFC3XWnd *pStoredWnd;

bool bBudgetOpen = false;
bool bAdvisorCustomString = false;
bool bOrdinanceOpen = false;

static int nBudgetLastSBCode = -1;
static int nBudgetZoneTaxLastSBCode = -1;
static int nBudgetEducationLastSBCode = -1;
static int nBudgetTransitLastSBCode = -1;

// Local destruct function for proper clarity. Since allocation also occurs
// local to the library, a crash would otherwise occur if the remote call
// were used (and it were set to 'delete' pBudgetMainDialog remotely).
static void L_BudgetMainDialog_Dest(CBudgetMainDialog *pBudgetMainDialog) {
	if (pBudgetMainDialog) {
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringToDateExpense);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringYearEndEstimate);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringBudgetNameYearMonth);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBudgetNameYearMonth);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticEndOfYearFunds);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticBudgetSummary);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticHourGlass);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPropertyTaxTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPropertyTaxYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditOrdinanceTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBondPaymentsTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditEducationTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditFireDepartmentTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditHealthAndWelfareTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPoliceDepartmentTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBondPaymentsYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditEducationYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditFireDepartmentYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditHealthAndWelfareYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditOrdinanceYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPoliceDepartmentYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditTransitTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditTransitYEE);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarEducation);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarFireDepartment);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarHealthAndWelfare);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarPoliceDepartment);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarPropertyTax);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarTransit);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringFour);
		for (int i = 8 - 1; i >= 0; --i)
			Game_BitmapButton_Dest(&pBudgetMainDialog->dwBDMBitmapButtonAdvisor[i]);
		for (int i = 8 - 1; i >= 0; --i)
			Game_BitmapButton_Dest(&pBudgetMainDialog->dwBDMBitmapButtonProperty[i]);
		delete pBudgetMainDialog;
		pBudgetMainDialog = 0;
	}
}

extern "C" void __cdecl Hook_SimulationPrepareBudgetDialog(BOOL bNoParent) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMainFrame *pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;

	int nCurrentYear;
	char szBudgetStr[255 + 1], szResStr[255 + 1];
	CBudgetMainDialog *pBudgetMainDialog;

	if (!bOptionsAutoBudget && !IsIconic(pMainFrm->m_hWnd)) {
		Game_SimcityApp_SetGameCursor(pSCApp, GAMECURSOR_REWARDS, FALSE);
		// There was originally a "StopSounds" call here, however it's been removed
		// in order to allow for the "click" to be played when the Budget dialogue
		// is executed from the city toolbar.
		switch (Game_RandomWordLFSRMod(4)) {
			case 0:
				Game_SimcityApp_MusicPlay(pSCApp, 10016);
				break;
			case 1:
				Game_SimcityApp_MusicPlay(pSCApp, 10005);
				break;
			case 2:
				Game_SimcityApp_MusicPlay(pSCApp, 10002);
				break;
			case 3:
				Game_SimcityApp_MusicPlay(pSCApp, 10010);
				break;
			default:
				break;
		}
		nCurrentYear = wCityStartYear + dwCityDays / 300;
		sprintf_s(szBudgetStr, "%s\r\n%4d Budget\r\n%s %4d", pszCityName.m_pchData, nCurrentYear, pSCApp->dwSCApCStringLongMonths[(dwCityDays / 25 % 12)].m_pchData, nCurrentYear);
		pBudgetMainDialog = new CBudgetMainDialog();
		if (pBudgetMainDialog)
			pBudgetMainDialog = Game_BudgetMainDialog_Cons(pBudgetMainDialog, (bNoParent) ? NULL : pMainFrm);
		if (pBudgetMainDialog) {
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringBudgetNameYearMonth, szBudgetStr);
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((bYearEndFlag) ? 890 : 891), szResStr, sizeof(szResStr) - 1);
			sprintf_s(szBudgetStr, szResStr, nCurrentYear);
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringToDateExpense, szBudgetStr);
			if (bYearEndFlag)
				++nCurrentYear;
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((bYearEndFlag) ? 892 : 893), szResStr, sizeof(szResStr) - 1);
			sprintf_s(szBudgetStr, szResStr, nCurrentYear);
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringYearEndEstimate, szBudgetStr);
			bBudgetOpen = true;
			GameMain_GameDialog_DoModal(pBudgetMainDialog);
			bBudgetOpen = false;
			L_BudgetMainDialog_Dest(pBudgetMainDialog);
		}
	}
}

void BudgetMain_PreCheckHourGlassTimer(CBudgetMainDialog *bBudgetMainDialog) {
	if (bBudgetMainDialog->dwDisplayHourGlass != -1)
		KillTimer(bBudgetMainDialog->m_hWnd, 32917);
}

void BudgetMain_PostCheckHourGlassTimer(CBudgetMainDialog *bBudgetMainDialog) {
	if (bBudgetMainDialog->dwDisplayHourGlass != -1) {
		SetTimer(bBudgetMainDialog->m_hWnd, 32917, 6000, 0);
		bBudgetMainDialog->dwDisplayHourGlass = 0;
		Game_BudgetMainDialog_ReleaseObjects(bBudgetMainDialog);
	}
}

extern "C" int __stdcall Hook_BudgetMainDialog_OnInitDialog() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetMainDialog_OnInitDialog(pThis);
	pStoredWnd = pThis;
	nOwnDrwDlg = OWNDRW_DLG_BUDGETMAIN;
	nBudgetLastSBCode = -1;
	return ret;
}

extern "C" void __stdcall Hook_BudgetMainDialog_ReleaseObjects() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	HDC hDC;
	CMFC3XDC *pDC;
	int nHourGlassState;

	// Added this here; the image for state 7 appears to be a copy of 6.. but the
	// image itself is offset and cut-off, so.. avoiding that one.
	nHourGlassState = (pThis->dwDisplayHourGlass > 6) ? 6 : pThis->dwDisplayHourGlass;
	hDC = GetDC(pThis->dwBDEditBudgetNameYearMonth.m_hWnd);
	pDC = GameMain_DC_FromHandle(hDC);
	Game_Graphics_DeleteObject(pThis->dwBDCGraphicsOne);
	Game_Graphics_LoadWithHandleData(pThis->dwBDCGraphicsOne, dwHourGlassImage[nHourGlassState], 0);
	ReleaseDC(pThis->dwBDEditBudgetNameYearMonth.m_hWnd, pDC->m_hDC);
	InvalidateRect(pThis->dwBDEditBudgetNameYearMonth.m_hWnd, NULL, FALSE);
}

extern "C" void __stdcall Hook_BudgetMainDialog_OnPaint() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMFC3XPalette *pActivePal, *pOldPal;
	CMFC3XPaintDC pPaintDC;

	GameMain_PaintDC_Cons(&pPaintDC, pThis);
	pActivePal = Game_SimcityApp_GetActivePalette(pSCApp);
	pOldPal = GameMain_DC_SelectPalette(&pPaintDC, pActivePal, FALSE);
	RealizePalette(pPaintDC.m_hDC);
	if (dwUpdateBudgetInformation) {
		Game_BudgetMainDialog_UpdateInternalInformation(pThis, 0);
		dwUpdateBudgetInformation = 0;
	}
	GameMain_DC_SelectPalette(&pPaintDC, pOldPal, FALSE);
	GameMain_PaintDC_Dest(&pPaintDC);
	Game_BudgetMainDialog_DrawCosts(pThis, &pPaintDC);
	InvalidateRect(pThis->dwBDEditBudgetNameYearMonth.m_hWnd, NULL, FALSE);
}

extern "C" void __stdcall Hook_BudgetMainDialog_DrawCosts(CMFC3XDC *pDC) {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	InvalidateRect(pThis->dwBDStaticBudgetSummary.m_hWnd, NULL, FALSE);
	InvalidateRect(pThis->dwBDStaticEndOfYearFunds.m_hWnd, NULL, FALSE);
}

extern "C" void __stdcall Hook_BudgetMainDialog_OnVScroll(UINT nSBCode, UINT nPos, CMFC3XScrollBar *pScrollBar) {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;

	// nLastSBCode has been introduced in order to account for an
	// ancient problem whereas dwBudgetScrollClicked would be set
	// to 1 with the wrong nSBCode - specifically SB_ENDSCROLL -
	// this would then result in the up/down control of any scrollbar
	// within the BudgetMainDialog not functioning as expected.

	// While shift is pressed, do not perform any scrolling action.
	// Reset all relevant variables.
	if (GetAsyncKeyState(VK_SHIFT) < 0) {
		dwBudgetScrollClicked = 0;
		nBudgetLastSBCode = -1;
		if (pScrollBar) {
			int nID = GetDlgCtrlID(pScrollBar->m_hWnd);
			if (nID > 0) {
				BudgetMain_PreCheckHourGlassTimer(pThis);
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
				DisplayItemHelp(pThis->m_hWnd, HELPTYPE_BUDGET, nID, false);
				BudgetMain_PostCheckHourGlassTimer(pThis);
			}
		}
		return;
	}
	
	if (dwBudgetScrollClicked) {
		// Record the last nSBCode as long as it's not SB_ENDSCROLL.
		if (nSBCode != SB_ENDSCROLL)
			nBudgetLastSBCode = nSBCode;
		dwBudgetScrollClicked = 0;
		if (nBudgetLastSBCode == SB_VERT) // Up Arrow
			dwBudgetScrollDirection = -1;
		else if (nBudgetLastSBCode == SB_HORZ) // Down Arrow
			dwBudgetScrollDirection = 1;
		else
			dwBudgetScrollDirection = 0;
		// Reset here, otherwise it can get stuck trying to go in a specific direction.
		if (nSBCode == SB_ENDSCROLL)
			nBudgetLastSBCode = -1;
		if (dwBudgetScrollDirection != 0) {
			switch (GetDlgCtrlID(pScrollBar->m_hWnd)) {
				case SC2K_DIALOG_BUDGET_SCROLLBAR_PROPTAX:
					Game_BudgetMainDialog_AdjustPropertyTaxPercentage(pThis);
					break;
				case SC2K_DIALOG_BUDGET_SCROLLBAR_POLICE:
					Game_BudgetMainDialog_AdjustPoliceFundingPercentage(pThis);
					break;
				case SC2K_DIALOG_BUDGET_SCROLLBAR_FIRE:
					Game_BudgetMainDialog_AdjustFireFundingPercentage(pThis);
					break;
				case SC2K_DIALOG_BUDGET_SCROLLBAR_EDUCATION:
					Game_BudgetMainDialog_AdjustEducationFundingPercentage(pThis);
					break;
				case SC2K_DIALOG_BUDGET_SCROLLBAR_TRANSIT:
					Game_BudgetMainDialog_AdjustTransitFundingPercentage(pThis);
					break;
				case SC2K_DIALOG_BUDGET_SCROLLBAR_HEALTH:
					Game_BudgetMainDialog_AdjustHealthFundingPercentage(pThis);
					break;
				default:
					break;
			}
		}
	}
	else {
		// Always record the last one here (regardless of what it is).
		nBudgetLastSBCode = nSBCode;
		dwBudgetScrollClicked = 1;
	}
	GameMain_Wnd_OnVScroll(pThis, nSBCode, nPos, pScrollBar);
}

extern "C" void __stdcall Hook_BudgetMainDialog_SetCursorAndClearGraphics() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	pStoredWnd = NULL;
	nOwnDrwDlg = OWNDRW_DLG_NONE;
	GameMain_BudgetMainDialog_SetCursorAndClearGraphics(pThis);
}

static void L_BudgetMainDialog_OnDraw_NameYearMonth_SC2K1996(CBudgetMainDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	HWND hDlgItem;
	HDC hDlgDC;
	COLORREF cr;
	HBRUSH hBrush;
	RECT areaRect;
	HFONT hOldFont;
	int nWidth, nHeight;
	POINT pt;
	CMFC3XPaintDC *pDC;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	cr = RGB(255, 255, 255);
	SetBkColor(pDC->m_hDC, cr);
	SetTextColor(pDC->m_hDC, RGB(0,0,0));
	hBrush = CreateSolidBrush(cr);
	FillRect(pDC->m_hDC, &lpDIS->rcItem, hBrush);
	CopyRect(&areaRect, &lpDIS->rcItem);
	areaRect.left += 2;
	areaRect.top  += 2;
	hOldFont = SelectFont(pDC->m_hDC, hFontMSSansSerifRegular8);
	DrawTextA(pDC->m_hDC, pThis->dwBDStringBudgetNameYearMonth.m_pchData, pThis->dwBDStringBudgetNameYearMonth.m_nDataLength, &areaRect, DT_LEFT);
	if (pThis->dwDisplayHourGlass != -1) {
		// The Hour Glass when displayed is positioned more towards
		// the bottom right-hand corner in-order to avoid any overlap
		// cases with over-long city names.
		nWidth = Game_Graphics_Width(pThis->dwBDCGraphicsOne);
		nHeight = Game_Graphics_Height(pThis->dwBDCGraphicsOne);
		pt.x = (lpDIS->rcItem.right - nWidth) - 5;
		pt.y = (lpDIS->rcItem.bottom - nHeight) - 3;
		Game_Graphics_SetColorTableFromApplicationPalette(pThis->dwBDCGraphicsOne);
		Game_Graphics_Paint(pThis->dwBDCGraphicsOne, lpDIS->hDC, pt.x, pt.y);
	}
	SelectFont(pDC->m_hDC, hOldFont);
	DeleteBrush(hBrush);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

static void L_BudgetMainDialog_OnDraw_Summary_SC2K1996(CBudgetMainDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	HWND hDlgItem;
	HDC hDlgDC;
	COLORREF cr;
	HBRUSH hBrush;
	RECT areaRect, tdeRect;
	HFONT hOldFont;
	int nOffSetX;
	const char *pStr;
	SIZE textSZ;
	POINT pt;
	CMFC3XPaintDC *pDC;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	cr = RGB(255, 255, 255);
	SetBkColor(pDC->m_hDC, cr);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	hBrush = CreateSolidBrush(cr);
	FillRect(pDC->m_hDC, &lpDIS->rcItem, hBrush);
	CopyRect(&areaRect, &lpDIS->rcItem);
	areaRect.left += 2;
	areaRect.top  += 2;
	// The field width is used in this case after the
	// coordinates have been converted.
	GetWindowRect(pThis->dwBDEditTransitTDE.m_hWnd, &tdeRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.right);
	hOldFont = SelectFont(pDC->m_hDC, hFontMSSansSerifRegular8);
	SetTextColor(pDC->m_hDC, RGB(0,0,255));
	pStr = L_GetCurrencyString_SC2K1996(pThis->dwBDYearToDateCashFlow);
	GetTextExtentPointA(pDC->m_hAttribDC, pStr, strlen(pStr), &textSZ);
	MoveToEx(pDC->m_hDC, (areaRect.right - areaRect.left) - (tdeRect.right - tdeRect.left) - textSZ.cx, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, pStr, strlen(pStr));
	areaRect.top += textSZ.cy;
	SetTextColor(pDC->m_hDC, RGB(255,0,0));
	pStr = L_GetCurrencyString_SC2K1996(pThis->dwBDEstimatedCashFlow);
	GetTextExtentPointA(pDC->m_hAttribDC, pStr, strlen(pStr), &textSZ);
	MoveToEx(pDC->m_hDC, (areaRect.right - areaRect.left) - textSZ.cx, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, pStr, strlen(pStr));
	areaRect.top += textSZ.cy;
	SetTextColor(pDC->m_hDC, RGB(0,0,0));
	if (bYearEndFlag)
		nOffSetX = (areaRect.right - areaRect.left) - (tdeRect.right - tdeRect.left);
	else
		nOffSetX = (areaRect.right - areaRect.left);
	pStr = L_GetCurrencyString_SC2K1996(dwCityFunds);
	GetTextExtentPointA(pDC->m_hAttribDC, pStr, strlen(pStr), &textSZ);
	MoveToEx(pDC->m_hDC, nOffSetX - textSZ.cx, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, pStr, strlen(pStr));
	SelectFont(pDC->m_hDC, hOldFont);
	DeleteBrush(hBrush);
	SetTextAlign(pDC->m_hDC, TA_LEFT);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

static void L_BudgetMainDialog_OnDraw_EOYFunds_SC2K1996(CBudgetMainDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	HWND hDlgItem;
	HDC hDlgDC;
	COLORREF cr;
	HBRUSH hBrush;
	RECT areaRect, tdeRect;
	HFONT hOldFont;
	int nAddedCashFlow, nOffSetX;
	const char *pStr;
	SIZE textSZ;
	POINT pt;
	CMFC3XPaintDC *pDC;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	cr = RGB(255, 255, 255);
	SetBkColor(pDC->m_hDC, cr);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	hBrush = CreateSolidBrush(cr);
	FillRect(pDC->m_hDC, &lpDIS->rcItem, hBrush);
	CopyRect(&areaRect, &lpDIS->rcItem);
	areaRect.left += 2;
	areaRect.top  += 2;
	// The field width is used in this case after the
	// coordinates have been converted.
	GetWindowRect(pThis->dwBDEditTransitTDE.m_hWnd, &tdeRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.right);
	hOldFont = SelectFont(pDC->m_hDC, hFontMSSansSerifRegular8);
	SetTextColor(pDC->m_hDC, RGB(0,0,0));
	if (bYearEndFlag) {
		nAddedCashFlow = pThis->dwBDYearToDateCashFlow;
		nOffSetX = (areaRect.right - areaRect.left) - (tdeRect.right - tdeRect.left);
	}
	else {
		nAddedCashFlow = pThis->dwBDEstimatedCashFlow;
		nOffSetX = (areaRect.right - areaRect.left);
	}
	pStr = L_GetCurrencyString_SC2K1996(nAddedCashFlow);
	GetTextExtentPointA(pDC->m_hAttribDC, pStr, strlen(pStr), &textSZ);
	MoveToEx(pDC->m_hDC, nOffSetX - textSZ.cx, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, pStr, strlen(pStr));
	SelectFont(pDC->m_hDC, hOldFont);
	DeleteBrush(hBrush);
	SetTextAlign(pDC->m_hDC, TA_LEFT);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

static void L_BudgetMainDialog_OnDraw_AlignedFields_SC2K1996(CBudgetMainDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS, int nVal, bool bRightAlign) {
	HWND hDlgItem;
	HDC hDlgDC;
	RECT areaRect;
	COLORREF cr;
	HBRUSH hBrush;
	HFONT hOldFont;
	int nOffSetX, nPosX;
	std::string str;
	SIZE textSZ;
	POINT pt;
	CMFC3XPaintDC *pDC;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	cr = RGB(255, 255, 255);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	SetBkColor(pDC->m_hDC, cr);
	hBrush = CreateSolidBrush(cr);
	FillRect(pDC->m_hDC, &lpDIS->rcItem, hBrush);
	CopyRect(&areaRect, &lpDIS->rcItem);
	areaRect.left += 2;
	areaRect.top  += 2;
	hOldFont = SelectFont(pDC->m_hDC, hFontMSSansSerifRegular8);
	SetTextColor(pDC->m_hDC, RGB(0,0,0));
	nOffSetX = (areaRect.right - areaRect.left);
	str = string_format("%d", nVal);
	GetTextExtentPointA(pDC->m_hAttribDC, str.c_str(), str.length(), &textSZ);
	nPosX = (bRightAlign) ? nOffSetX - textSZ.cx : areaRect.left;
	MoveToEx(pDC->m_hDC, nPosX, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, str.c_str(), str.length());
	SelectFont(pDC->m_hDC, hOldFont);
	DeleteBrush(hBrush);
	SetTextAlign(pDC->m_hDC, TA_LEFT);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

bool L_BudgetMainDialog_OnDrawItem_SC2K1996(CBudgetMainDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	int nVal = 0;

	if (nCtlID == SC2K_DIALOG_BUDGET_EDIT_NAMEYEARMONTH) {
		L_BudgetMainDialog_OnDraw_NameYearMonth_SC2K1996(pThis, nCtlID, lpDIS);
		return true;
	}
	else if (nCtlID == SC2K_DIALOG_BUDGET_STATIC_SUMMARY) {
		L_BudgetMainDialog_OnDraw_Summary_SC2K1996(pThis, nCtlID, lpDIS);
		return true;
	}
	else if (nCtlID == SC2K_DIALOG_BUDGET_STATIC_EOYFUNDS) {
		L_BudgetMainDialog_OnDraw_EOYFunds_SC2K1996(pThis, nCtlID, lpDIS);
		return true;
	}
	else if ((nCtlID >= SC2K_DIALOG_BUDGET_EDIT_TDE_PROPTAX && nCtlID <= SC2K_DIALOG_BUDGET_EDIT_TDE_TRANSIT) ||
		(nCtlID >= SC2K_DIALOG_BUDGET_EDIT_YEE_PROPTAX && nCtlID <= SC2K_DIALOG_BUDGET_EDIT_YEE_TRANSIT)) {
		switch (nCtlID) {
			case SC2K_DIALOG_BUDGET_EDIT_TDE_PROPTAX:
				nVal = pThis->dwBDPropertyTaxTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_PROPTAX:
				nVal = pThis->dwBDPropertyTaxYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_ORDINANCES:
				nVal = pThis->dwBDOrdinanceTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_ORDINANCES:
				nVal = pThis->dwBDOrdinanceYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_BONDPAYMNT:
				nVal = pThis->dwBDBondPaymentsTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_BONDPAYMNT:
				nVal = pThis->dwBDBondPaymentsYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_POLICE:
				nVal = pThis->dwBDPoliceDepartmentTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_POLICE:
				nVal = pThis->dwBDPoliceDepartmentYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_FIRE:
				nVal = pThis->dwBDFireDepartmentTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_FIRE:
				nVal = pThis->dwBDFireDepartmentYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_HEALTH:
				nVal = pThis->dwBDHealthAndWelfareTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_HEALTH:
				nVal = pThis->dwBDHealthAndWelfareYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_EDUCATION:
				nVal = pThis->dwBDEducationTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_EDUCATION:
				nVal = pThis->dwBDEducationYEELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TDE_TRANSIT:
				nVal = pThis->dwBDTransitTDELim;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_YEE_TRANSIT:
				nVal = pThis->dwBDTransitYEELim;
				break;
			default:
				return false;
		}
		L_BudgetMainDialog_OnDraw_AlignedFields_SC2K1996(pThis, nCtlID, lpDIS, nVal, true);
		return true;
	}
	else if (nCtlID == SC2K_DIALOG_BUDGET_EDIT_PROPTAX ||
		(nCtlID >= SC2K_DIALOG_BUDGET_EDIT_POLICE && nCtlID <= SC2K_DIALOG_BUDGET_EDIT_TRANSIT)) {
		switch (nCtlID) {
			case SC2K_DIALOG_BUDGET_EDIT_PROPTAX:
				nVal = pThis->dwPropertyTaxPercent;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_POLICE:
				nVal = pThis->dwPoliceDepartmentPercent;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_FIRE:
				nVal = pThis->dwFireDepartmentPercent;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_HEALTH:
				nVal = pThis->dwHealthDepartmentPercent;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_EDUCATION:
				nVal = pThis->dwEducationDepartmentPercent;
				break;
			case SC2K_DIALOG_BUDGET_EDIT_TRANSIT:
				nVal = pThis->dwTransitDepartmentPercent;
				break;
			default:
				return false;
		}
		L_BudgetMainDialog_OnDraw_AlignedFields_SC2K1996(pThis, nCtlID, lpDIS, nVal, false);
		return true;
	}
	return false;
}

static void OpenOrdinanceDialog(CBudgetOrdinanceDialog *pOrdinanceDialog, bool bFromBudget) {
	CMFC3XWnd *pOldWnd;

	bOrdinanceOpen = true;
	pOldWnd = pStoredWnd;
	Game_GameDialog_DoModal(pOrdinanceDialog);
	pStoredWnd = pOldWnd;
	nOwnDrwDlg = (bFromBudget) ? OWNDRW_DLG_BUDGETMAIN : OWNDRW_DLG_NONE;
	bOrdinanceOpen = false;
}

extern "C" void __stdcall Hook_BudgetMainDialog_OpenOrdinanceDialog() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	BudgetMain_PreCheckHourGlassTimer(pThis);
	OpenOrdinanceDialog(&ordinanceDlg, true);
	BudgetMain_PostCheckHourGlassTimer(pThis);
	Game_BudgetMainDialog_UpdateInternalInformation(pThis, 0);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
}

static void L_BudgetOrdinanceDialog_OnDraw_AlignedFields_SC2K1996(CBudgetOrdinanceDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS, bool bRightAlign) {
	HWND hDlgItem;
	HDC hDlgDC;
	RECT areaRect;
	COLORREF cr;
	HBRUSH hBrush;
	HFONT hOldFont;
	int nOffSetX, nPosX;
	char szStr[255 + 1];
	int nLen;
	SIZE textSZ;
	POINT pt;
	CMFC3XPaintDC *pDC;

	hDlgItem = GetDlgItem(pThis->m_hWnd, nCtlID);
	hDlgDC = GetDC(hDlgItem);
	pDC = (CMFC3XPaintDC *)GameMain_DC_FromHandle(hDlgDC);
	cr = RGB(255, 255, 255);
	SetTextAlign(pDC->m_hDC, TA_UPDATECP);
	SetBkColor(pDC->m_hDC, cr);
	hBrush = CreateSolidBrush(cr);
	FillRect(pDC->m_hDC, &lpDIS->rcItem, hBrush);
	CopyRect(&areaRect, &lpDIS->rcItem);
	areaRect.left += 2;
	areaRect.top  += 2;
	hOldFont = SelectFont(pDC->m_hDC, hFontMSSansSerifRegular8);
	SetTextColor(pDC->m_hDC, RGB(0,0,0));
	nOffSetX = (areaRect.right - areaRect.left);
	memset(szStr, 0, sizeof(szStr));
	SendMessageA(hDlgItem, WM_GETTEXT, ARRAYSIZE(szStr), (LPARAM)szStr);
	nLen = strlen(szStr);
	GetTextExtentPointA(pDC->m_hAttribDC, szStr, nLen, &textSZ);
	nPosX = (bRightAlign) ? nOffSetX - textSZ.cx : areaRect.left;
	MoveToEx(pDC->m_hDC, nPosX, areaRect.top, &pt);
	TextOutA(pDC->m_hDC, 0, 0, szStr, nLen);
	SelectFont(pDC->m_hDC, hOldFont);
	DeleteBrush(hBrush);
	SetTextAlign(pDC->m_hDC, TA_LEFT);
	ReleaseDC(hDlgItem, pDC->m_hDC);
}

bool L_BudgetOrdinanceDialog_OnDrawItem_SC2K1996(CBudgetOrdinanceDialog *pThis, int nCtlID, LPDRAWITEMSTRUCT lpDIS) {
	L_BudgetOrdinanceDialog_OnDraw_AlignedFields_SC2K1996(pThis, nCtlID, lpDIS, false);
	return true;
}

extern "C" void __stdcall Hook_SimcityView_DoOrdinance() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	OpenOrdinanceDialog(&ordinanceDlg, false);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
}

static void UpdatePercentageValue(int *nPercent, int nDirection, int nMin, int nMax) {
	int nNewValue;

	nNewValue = *nPercent + nDirection;
	if (nNewValue > nMax)
		nNewValue = nMax;
	else if (nNewValue < nMin)
		nNewValue = nMin;
	*nPercent = nNewValue;
}

extern "C" int __stdcall Hook_BudgetOrdinanceDialog_OnInitDialog() {
	CBudgetOrdinanceDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetOrdinanceDialog_OnInitDialog(pThis);
	pStoredWnd = pThis;
	nOwnDrwDlg = OWNDRW_DLG_ORDINANCES;
	return ret;
}

void BudgetOrdinanceDialog_ToggleOrdinance(CBudgetOrdinanceDialog *pThis, int nDlgID) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	int nOrdOpt = -1;

	switch (nDlgID) {
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_FINAN_ONEPCTSALESTAX:
		nOrdOpt = ORDINANCE_OPT_SALES_TAX;
		break;
	case SC2K_DIALOG_ORDINANCES_CHECKBOX_FINAN_ONEPCTINCOMETAX:
		nOrdOpt = ORDINANCE_OPT_INCOME_TAX;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_FINAN_LEGALIZEDGAMBLING:
		nOrdOpt = ORDINANCE_OPT_LEGALIZED_GAMBLING;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_FINAN_PARKINGFINES:
		nOrdOpt = ORDINANCE_OPT_PARKING_FINES;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_HLSFT_VOLUNTEERFIREDEPT:
		nOrdOpt = ORDINANCE_OPT_VOLUNTEER_FIRE_DEPARTMENT;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_HLSFT_PUBLICSMOKINGBAN:
		nOrdOpt = ORDINANCE_OPT_PUBLIC_SMOKING_BAN;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_HLSFT_FREECLINICS:
		nOrdOpt = ORDINANCE_OPT_FREE_CLINICS;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_HLSFT_JUNIORSPORTS:
		nOrdOpt = ORDINANCE_OPT_JUNIOR_SPORTS;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_EDUCA_PROREADCAMPAIGN:
		nOrdOpt = ORDINANCE_OPT_PRO_READING_CAMPAIGN;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_EDUCA_ANTIDRUGCAMPAIGN:
		nOrdOpt = ORDINANCE_OPT_ANTI_DRUG_CAMPAIGN;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_EDUCA_CPRTRAINING:
		nOrdOpt = ORDINANCE_OPT_CPR_TRAINING;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_EDUCA_NGHBRHOODWATCH:
		nOrdOpt = ORDINANCE_OPT_NEIGHBORHOOD_WATCH;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_PROMO_TOURISTADVERT:
		nOrdOpt = ORDINANCE_OPT_TOURIST_ADVERTISING;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_PROMO_BUSINESSADVERT:
		nOrdOpt = ORDINANCE_OPT_BUSINESS_ADVERTISING;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_PROMO_CITYBEAUTIFIC:
		nOrdOpt = ORDINANCE_OPT_CITY_BEAUTIFICATION;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_PROMO_ANNUALCARNIVAL:
		nOrdOpt = ORDINANCE_OPT_ANNUAL_CARNIVAL;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_OTHER_ENERGYCONSERV:
		nOrdOpt = ORDINANCE_OPT_ENERGY_CONSERVATION;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_OTHER_NUCLEARFREEZONE:
		nOrdOpt = ORDINANCE_OPT_NUCLEAR_FREE_ZONE;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_OTHER_HOMELESSSHELTER:
		nOrdOpt = ORDINANCE_OPT_HOMELESS_SHELTER;
		break;
    case SC2K_DIALOG_ORDINANCES_CHECKBOX_OTHER_POLLUTIONCTRLS:
		nOrdOpt = ORDINANCE_OPT_POLLUTION_CONTROLS;
		break;
	default:
		break;
	}

	if (nOrdOpt < ORDINANCE_OPT_SALES_TAX || nOrdOpt >= ORDINANCE_OPT_COUNT)
		return;

	if (GetAsyncKeyState(VK_SHIFT) < 0) {
		Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
		DisplayItemHelp(pThis->m_hWnd, HELPTYPE_ORDINANCES, nOrdOpt, false);
		return;
	}
	Game_BudgetOrdinanceDialog_ToggleOrdinanceOption(pThis, nOrdOpt);
}

extern "C" int __stdcall Hook_BudgetZoneTaxDialog_OnInitDialog() {
	CBudgetZoneTaxDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetZoneTaxDialog_OnInitDialog(pThis);
	nBudgetZoneTaxLastSBCode = -1;
	return ret;
}

extern "C" void __stdcall Hook_BudgetZoneTaxDialog_OnVScroll(UINT nSBCode, UINT nPos, CMFC3XScrollBar *pScrollBar) {
	CBudgetZoneTaxDialog *pThis;

	__asm mov [pThis], ecx

	int dwBudgetZoneTaxScrollDirection;

	if (dwBudgetZoneTaxScrollClicked) {
		// Record the last nSBCode as long as it's not SB_ENDSCROLL.
		if (nSBCode != SB_ENDSCROLL)
			nBudgetZoneTaxLastSBCode = nSBCode;
		dwBudgetZoneTaxScrollClicked = 0;
		if (nBudgetZoneTaxLastSBCode == SB_VERT) // Up Arrow
			dwBudgetZoneTaxScrollDirection = -1;
		else if (nBudgetZoneTaxLastSBCode == SB_HORZ) // Down Arrow
			dwBudgetZoneTaxScrollDirection = 1;
		else
			dwBudgetZoneTaxScrollDirection = 0;
		// Reset here, otherwise it can get stuck trying to go in a specific direction.
		if (nSBCode == SB_ENDSCROLL)
			nBudgetZoneTaxLastSBCode = -1;
		if (dwBudgetZoneTaxScrollDirection != 0) {
			switch (GetDlgCtrlID(pScrollBar->m_hWnd)) {
				case SC2K_DIALOG_BUDGET_PROPTAX_SCROLLBAR_RES:
					UpdatePercentageValue(&pThis->dwBZTDResPercent, dwBudgetZoneTaxScrollDirection, 0, 20);
					Game_BudgetZoneTaxDialog_UpdateResFunding(pThis);
					break;
				case SC2K_DIALOG_BUDGET_PROPTAX_SCROLLBAR_COM:
					UpdatePercentageValue(&pThis->dwBZTDComPercent, dwBudgetZoneTaxScrollDirection, 0, 20);
					Game_BudgetZoneTaxDialog_UpdateComFunding(pThis);
					break;
				case SC2K_DIALOG_BUDGET_PROPTAX_SCROLLBAR_IND:
					UpdatePercentageValue(&pThis->dwBZTDIndPercent, dwBudgetZoneTaxScrollDirection, 0, 20);
					Game_BudgetZoneTaxDialog_UpdateIndFunding(pThis);
					break;
				default:
					break;
			}
		}
	}
	else {
		// Always record the last one here (regardless of what it is).
		nBudgetZoneTaxLastSBCode = nSBCode;
		dwBudgetZoneTaxScrollClicked = 1;
	}
	GameMain_Wnd_OnVScroll(pThis, nSBCode, nPos, pScrollBar);
}

extern "C" int __stdcall Hook_BudgetEducationDialog_OnInitDialog() {
	CBudgetEducationDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetEducationDialog_OnInitDialog(pThis);
	nBudgetEducationLastSBCode = -1;
	return ret;
}

extern "C" void __stdcall Hook_BudgetEducationDialog_OnVScroll(UINT nSBCode, UINT nPos, CMFC3XScrollBar *pScrollBar) {
	CBudgetEducationDialog *pThis;

	__asm mov [pThis], ecx

	int dwBudgetEducationScrollDirection;

	if (dwBudgetEducationScrollClicked) {
		// Record the last nSBCode as long as it's not SB_ENDSCROLL.
		if (nSBCode != SB_ENDSCROLL)
			nBudgetEducationLastSBCode = nSBCode;
		dwBudgetEducationScrollClicked = 0;
		if (nBudgetEducationLastSBCode == SB_VERT) // Up Arrow
			dwBudgetEducationScrollDirection = -1;
		else if (nBudgetEducationLastSBCode == SB_HORZ) // Down Arrow
			dwBudgetEducationScrollDirection = 1;
		else
			dwBudgetEducationScrollDirection = 0;
		// Reset here, otherwise it can get stuck trying to go in a specific direction.
		if (nSBCode == SB_ENDSCROLL)
			nBudgetEducationLastSBCode = -1;
		if (dwBudgetEducationScrollDirection != 0) {
			switch (GetDlgCtrlID(pScrollBar->m_hWnd)) {
			case SC2K_DIALOG_BUDGET_EDUCATION_SCROLLBAR_COLLEGE:
				UpdatePercentageValue(&pThis->dwBEDCollegePercent, dwBudgetEducationScrollDirection, 0, 100);
				Game_BudgetEducationDialog_UpdateCollegeFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_EDUCATION_SCROLLBAR_SCHOOL:
				UpdatePercentageValue(&pThis->dwBEDSchoolPercent, dwBudgetEducationScrollDirection, 0, 100);
				Game_BudgetEducationDialog_UpdateSchoolFunding(pThis);
				break;
			default:
				break;
			}
		}
	}
	else {
		// Always record the last one here (regardless of what it is).
		nBudgetEducationLastSBCode = nSBCode;
		dwBudgetEducationScrollClicked = 1;
	}
	GameMain_Wnd_OnVScroll(pThis, nSBCode, nPos, pScrollBar);
}

extern "C" int __stdcall Hook_BudgetTransitDialog_OnInitDialog() {
	CBudgetTransitDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetTransitDialog_OnInitDialog(pThis);
	nBudgetTransitLastSBCode = -1;
	return ret;
}

extern "C" void __stdcall Hook_BudgetTransitDialog_OnVScroll(UINT nSBCode, UINT nPos, CMFC3XScrollBar *pScrollBar) {
	CBudgetTransitDialog *pThis;

	__asm mov [pThis], ecx

	int dwBudgetTransitScrollDirection;

	if (dwBudgetTransitScrollClicked) {
		// Record the last nSBCode as long as it's not SB_ENDSCROLL.
		if (nSBCode != SB_ENDSCROLL)
			nBudgetTransitLastSBCode = nSBCode;
		dwBudgetTransitScrollClicked = 0;
		if (nBudgetTransitLastSBCode == SB_VERT) // Up Arrow
			dwBudgetTransitScrollDirection = -1;
		else if (nBudgetTransitLastSBCode == SB_HORZ) // Down Arrow
			dwBudgetTransitScrollDirection = 1;
		else
			dwBudgetTransitScrollDirection = 0;
		// Reset here, otherwise it can get stuck trying to go in a specific direction.
		if (nSBCode == SB_ENDSCROLL)
			nBudgetTransitLastSBCode = -1;
		if (dwBudgetTransitScrollDirection != 0) {
			switch (GetDlgCtrlID(pScrollBar->m_hWnd)) {
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_ROAD:
				UpdatePercentageValue(&pThis->dwBTDRoadPercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateRoadFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_RAIL:
				UpdatePercentageValue(&pThis->dwBTDRailPercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateRailFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_HIGHWAY:
				UpdatePercentageValue(&pThis->dwBTDHighwayPercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateHighwayFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_SUBWAY:
				UpdatePercentageValue(&pThis->dwBTDSubwayPercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateSubwayFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_BRIDGE:
				UpdatePercentageValue(&pThis->dwBTDBridgePercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateBridgeFunding(pThis);
				break;
			case SC2K_DIALOG_BUDGET_TRANSIT_SCROLLBAR_TUNNEL:
				UpdatePercentageValue(&pThis->dwBTDTunnelPercent, dwBudgetTransitScrollDirection, 0, 100);
				Game_BudgetTransitDialog_UpdateTunnelFunding(pThis);
				break;
			default:
				break;
			}
		}
	}
	else {
		// Always record the last one here (regardless of what it is).
		nBudgetTransitLastSBCode = nSBCode;
		dwBudgetTransitScrollClicked = 1;
	}
	GameMain_Wnd_OnVScroll(pThis, nSBCode, nPos, pScrollBar);
}

bool DoPopDialogButton(CPopulationDialog *pPopDlg, int nDlgID) {
	int nSelected;
	CMFC3XString *pStr;

	switch (nDlgID) {
	case SC2K_DIALOG_POPULATION_RADIO_POPULATION:
		nSelected = POPDLG_POPULATION;
		break;
	case SC2K_DIALOG_POPULATION_RADIO_HEALTH:
		nSelected = POPDLG_HEALTH;
		break;
	case SC2K_DIALOG_POPULATION_RADIO_EDUCATION:
		nSelected = POPDLG_EDUCATION;
		break;
	default:
		return false;
	}

	pStr = pPopDlg->dwPDStringOne[nSelected];
	pPopDlg->dwPDSelection = nSelected;
	SetWindowTextA(pPopDlg->m_hWnd, pStr->m_pchData);
	return true;
}

// This function is to ensure that the radio controls retain their
// correct state while using the shift-click 'Help' functionality.
void FixPopDialogButtons(CPopulationDialog *pPopDlg) {
	int nState[POPDLG_COUNT];

	memset(nState, BST_UNCHECKED, sizeof(nState));
	if (pPopDlg->dwPDSelection >= POPDLG_POPULATION && pPopDlg->dwPDSelection <= POPDLG_EDUCATION) {
		nState[pPopDlg->dwPDSelection] = BST_CHECKED;
		Button_SetCheck(GetDlgItem(pPopDlg->m_hWnd, SC2K_DIALOG_POPULATION_RADIO_POPULATION), nState[POPDLG_POPULATION]);
		Button_SetCheck(GetDlgItem(pPopDlg->m_hWnd, SC2K_DIALOG_POPULATION_RADIO_HEALTH), nState[POPDLG_HEALTH]);
		Button_SetCheck(GetDlgItem(pPopDlg->m_hWnd, SC2K_DIALOG_POPULATION_RADIO_EDUCATION), nState[POPDLG_EDUCATION]);
	}
}

extern "C" void __stdcall Hook_PopulationDialog_DoDataExchange(CMFC3XDataExchange *pDatEx) {
	CPopulationDialog *pThis;

	__asm mov [pThis], ecx
	
	// Nothing happens here at this point.
	// Originally there was a DDX_Radio call that iterated through the 'then' radio controls
	// starting from SC2K_DIALOG_POPULATION_RADIO_POPULATION and setting dwPDSelection based
	// on which control was highlighted; this no longer occurs due to those radio controls
	// now being checkboxes.
}

extern "C" BOOL __stdcall Hook_PopulationDialog_ToggleDialog() {
	CPopulationDialog *pThis;

	__asm mov [pThis], ecx

	if (pThis->dwPDDialogActive) {
		ShowWindow(pThis->m_hWnd, SW_HIDE);
		if (dwRefreshControls)
			Game_PopulationDialog_DeleteFont(pThis);
		pThis->dwPDDialogActive = 0;
	}
	else {
		if (dwRefreshControls)
			Game_PopulationDialog_UpdateControls(pThis);
		FixPopDialogButtons(pThis);
		ShowWindow(pThis->m_hWnd, SW_SHOWNORMAL);
		pThis->dwPDDialogActive = 1;
	}
	return pThis->dwPDDialogActive;
}

extern "C" void __stdcall Hook_CityMapDialog_OnLButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CCityMapDialog *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMainFrame *pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;
	CSimcityView *pSCView;

	if (PtInRect(&pThis->dwCMDRECTOne, pt)) {
		if (GetAsyncKeyState(VK_SHIFT) < 0) {
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
			DisplayItemHelp(pMainFrm->m_hWnd, HELPTYPE_CITYMAP, SC2K_DIALOG_CITYMAP_STATIC_MAPAREA, true);
			return;
		}
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		Game_CityMapDialog_CenterOnPoint(pThis, pt.x, pt.y);
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
	Game_GameDialog_OnLButtonDown(pThis, nFlags, pt);
}

extern "C" void __stdcall Hook_SimGraphDialog_DoDataExchange(CMFC3XDataExchange *pDatEx) {
	CSimGraphDialog *pThis;

	__asm mov [pThis], ecx

	// Nothing happens here at this point.
}

static void SimGraphDialog_GetRangeSelect(CSimGraphDialog *pSimGraphDlg) {
	int nRangeState[GRAPHDLG_RANGE_COUNT];

	memset(nRangeState, BST_UNCHECKED, sizeof(nRangeState));
	if (pSimGraphDlg->dwSGDRange >= GRAPHDLG_RANGE_ONEYEAR && pSimGraphDlg->dwSGDRange <= GRAPHDLG_RANGE_HUNDREDYEARS) {
		nRangeState[pSimGraphDlg->dwSGDRange] = BST_CHECKED;
		Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_RADIO_RANGEONEYEAR), nRangeState[GRAPHDLG_RANGE_ONEYEAR]);
		Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_RADIO_RANGETENYEARS), nRangeState[GRAPHDLG_RANGE_TENYEARS]);
		Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_RADIO_RANGEHUNDREDYEARS), nRangeState[GRAPHDLG_RANGE_HUNDREDYEARS]);
	}
}

static void SimGraphDialog_GetOptionsSelect(CSimGraphDialog *pSimGraphDlg) {
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTCITYSIZE), pSimGraphDlg->dwSGDOptCitySize ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTRESIDENTS), pSimGraphDlg->dwSGDOptResidents ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTCOMMERCE), pSimGraphDlg->dwSGDOptCommerce ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTINDUSTRY), pSimGraphDlg->dwSGDOptIndustry ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTTRAFFIC), pSimGraphDlg->dwSGDOptTraffic ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOLLUTION), pSimGraphDlg->dwSGDOptPollution ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTLANDVALUE), pSimGraphDlg->dwSGDOptLandValue ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTCRIME), pSimGraphDlg->dwSGDOptCrime ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOWERPERCENT), pSimGraphDlg->dwSGDOptPowerPercentage ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTWATERPERCENT), pSimGraphDlg->dwSGDOptWaterPercentage ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTHEALTH), pSimGraphDlg->dwSGDOptHealth ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTEDUCATION), pSimGraphDlg->dwSGDOptEducation ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTUNEMPLOYMENT), pSimGraphDlg->dwSGDOptUnemployment ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTGNP), pSimGraphDlg->dwSGDOptGNP ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTNATIONALPOP), pSimGraphDlg->dwSGDOptNationalPop ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(pSimGraphDlg->m_hWnd, SC2K_DIALOG_GRAPH_CHECKBOX_OPTFEDRATE), pSimGraphDlg->dwSGDOptFedRate ? BST_CHECKED : BST_UNCHECKED);
}

static void SimGraphDialog_SetControlStates(CSimGraphDialog *pSimGraphDlg) {
	SimGraphDialog_GetRangeSelect(pSimGraphDlg);
	SimGraphDialog_GetOptionsSelect(pSimGraphDlg);
}

extern "C" void __stdcall Hook_SimGraphDialog_UpdateDialog() {
	CSimGraphDialog *pThis;

	__asm mov [pThis], ecx

	RECT r;

	if (pThis->dwSGDDialogActive) {
		SimGraphDialog_SetControlStates(pThis);
		UnionRect(&r, &pThis->dwSGDRectSelectionAxis, &pThis->dwSGDRectRangeAxis);
		InvalidateRect(pThis->m_hWnd, &r, 0);
		UpdateWindow(pThis->m_hWnd);
	}
}

void SimGraphDialog_UpdateRange(CSimGraphDialog *pSimGraphDlg, int nDlgID) {
	int nSelected;

	switch (nDlgID) {
	case SC2K_DIALOG_GRAPH_RADIO_RANGEONEYEAR:
		nSelected = GRAPHDLG_RANGE_ONEYEAR;
		break;
	case SC2K_DIALOG_GRAPH_RADIO_RANGETENYEARS:
		nSelected = GRAPHDLG_RANGE_TENYEARS;
		break;
	case SC2K_DIALOG_GRAPH_RADIO_RANGEHUNDREDYEARS:
		nSelected = GRAPHDLG_RANGE_HUNDREDYEARS;
		break;
	default:
		return;
	}

	pSimGraphDlg->dwSGDRange = nSelected;
	Game_SimGraphDialog_UpdateDialog(pSimGraphDlg);
}

static void SimGraphDialog_ToggleOption(CSimGraphDialog *pSimGraphDlg, int nDlgID) {
	switch (nDlgID) {
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCITYSIZE:
		pSimGraphDlg->dwSGDOptCitySize = !pSimGraphDlg->dwSGDOptCitySize;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTRESIDENTS:
		pSimGraphDlg->dwSGDOptResidents = !pSimGraphDlg->dwSGDOptResidents;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCOMMERCE:
		pSimGraphDlg->dwSGDOptCommerce = !pSimGraphDlg->dwSGDOptCommerce;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTINDUSTRY:
		pSimGraphDlg->dwSGDOptIndustry = !pSimGraphDlg->dwSGDOptIndustry;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTTRAFFIC:
		pSimGraphDlg->dwSGDOptTraffic = !pSimGraphDlg->dwSGDOptTraffic;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOLLUTION:
		pSimGraphDlg->dwSGDOptPollution = !pSimGraphDlg->dwSGDOptPollution;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTLANDVALUE:
		pSimGraphDlg->dwSGDOptLandValue = !pSimGraphDlg->dwSGDOptLandValue;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCRIME:
		pSimGraphDlg->dwSGDOptCrime = !pSimGraphDlg->dwSGDOptCrime;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOWERPERCENT:
		pSimGraphDlg->dwSGDOptPowerPercentage = !pSimGraphDlg->dwSGDOptPowerPercentage;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTWATERPERCENT:
		pSimGraphDlg->dwSGDOptWaterPercentage = !pSimGraphDlg->dwSGDOptWaterPercentage;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTHEALTH:
		pSimGraphDlg->dwSGDOptHealth = !pSimGraphDlg->dwSGDOptHealth;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTEDUCATION:
		pSimGraphDlg->dwSGDOptEducation = !pSimGraphDlg->dwSGDOptEducation;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTUNEMPLOYMENT:
		pSimGraphDlg->dwSGDOptUnemployment = !pSimGraphDlg->dwSGDOptUnemployment;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTGNP:
		pSimGraphDlg->dwSGDOptGNP = !pSimGraphDlg->dwSGDOptGNP;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTNATIONALPOP:
		pSimGraphDlg->dwSGDOptNationalPop = !pSimGraphDlg->dwSGDOptNationalPop;
		break;
	case SC2K_DIALOG_GRAPH_CHECKBOX_OPTFEDRATE:
		pSimGraphDlg->dwSGDOptFedRate = !pSimGraphDlg->dwSGDOptFedRate;
		break;
	default:
		return;
	}
}

void SimGraphDialog_UpdateOptions(CSimGraphDialog *pSimGraphDlg, int nDlgID) {
	if (pSimGraphDlg->dwSGDDialogActive) {
		SimGraphDialog_ToggleOption(pSimGraphDlg, nDlgID);
		SimGraphDialog_SetControlStates(pSimGraphDlg);
		InvalidateRect(pSimGraphDlg->m_hWnd, &pSimGraphDlg->dwSGDRectSelectionAxis, 0);
		UpdateWindow(pSimGraphDlg->m_hWnd);
	}
}

extern "C" BOOL __stdcall Hook_SimGraphDialog_ToggleDialog() {
	CSimGraphDialog *pThis;

	__asm mov [pThis], ecx

	int ret;

	if (pThis->dwSGDDialogActive) {
		ShowWindow(pThis->m_hWnd, SW_HIDE);
		if (dwRefreshControls)
			Game_SimGraphDialog_DeleteObjects(pThis);
		pThis->dwSGDDialogActive = 0;
		SimGraphDialog_SetControlStates(pThis);
		ret = pThis->dwSGDDialogActive;
	}
	else {
		SimGraphDialog_SetControlStates(pThis);
		if (dwRefreshControls)
			Game_SimGraphDialog_AttachObjects(pThis);
		if (dwGraphBitmapResOne && dwGraphBitmapResTwo) {
			ShowWindow(pThis->m_hWnd, SW_SHOWNORMAL);
			pThis->dwSGDDialogActive = 1;
			ret = pThis->dwSGDDialogActive;
		}
		else {
			GameMain_AfxMessageBoxStr(aNotEnoughMem, 0, 0);
			dwRefreshControls = 1;
			Game_SimGraphDialog_DeleteObjects(pThis);
			ret = 0;
		}
	}
	return ret;
}

extern "C" BOOL __stdcall Hook_SimGraphDialog_HideDialog() {
	CSimGraphDialog *pThis;

	__asm mov [pThis], ecx

	if (pThis->dwSGDDialogActive) {
		ShowWindow(pThis->m_hWnd, SW_HIDE);
		if (dwRefreshControls)
			Game_SimGraphDialog_DeleteObjects(pThis);
		pThis->dwSGDDialogActive = 0;
		SimGraphDialog_SetControlStates(pThis);
	}
	return pThis->dwSGDDialogActive;
}

static void BudgetAdvisorDialog_SelectTypeAndSetMessage(CBudgetAdvisorDialog *pBudgetAdvisorDlg) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	int nItem, nTotalDemand, nCityCrime, nTotalFunding, nAdvice = -1, nSound = -1;
	unsigned int nFireCoverage, nHealthCoverage, nHealthOrd, nTotalCosts;

	nCityCrime = dwMapXGRP[GRP_CITYCRIME][0];
	switch (pBudgetAdvisorDlg->m_dwBDAType) {
	case ADVISOR_TAXES:
		nTotalDemand = wCityDemand[DEMAND_IND] + wCityDemand[DEMAND_COM] + wCityDemand[DEMAND_RES];
		if (nTotalDemand < -666) 
			nAdvice = ADVICE_PROPTAX_LOWER;
		else if (dwCityFunds >= (int)dwCityPopulation)
			nAdvice = ADVICE_NONE;
		else if (nTotalDemand <= 666)
			nAdvice = ADVICE_PROPTAX_CUTBACK;
		else
			nAdvice = ADVICE_PROPTAX_RAISE;
		nSound = SOUND_BOOS;
		break;
	case ADVISOR_ORDINANCES:
		nAdvice = ADVICE_NONE;
		if ((dwCityOrdinances & ORDINANCE_POLLUTION_CONTROLS) == 0 && dwMapXGRP[GRP_POLLUTION][0] > 30)
			nAdvice = ADVICE_ORDIN_DO_POLLUTIONCTRLS;
		else if ((dwCityOrdinances & ORDINANCE_ENERGY_CONSERVATION) == 0 && dwPowerUsedPercentage > 98) {
			// Make sure of the following in order to hit this condition:
			// - total number of generated tiles is above 0
			// - energy conservation isn't enabled
			// - utilisation is above 98%
			//
			// Previously on empty maps you'd also get the same warning - but there isn't anything to conserve.
			//
			// An alternative possibility could well be to get the player to enable it as early as possible.
			// I'll leave that thought there while accounting for this change either way.
			if (dwTotalGeneratedPowerTiles > 0)
				nAdvice = ADVICE_ORDIN_DO_ENERGYCONSERVE;
		}
		else if (nCityCrime > 30) {
			if ((dwCityOrdinances & ORDINANCE_NEIGHBORHOOD_WATCH) == 0)
				nAdvice = ADVICE_POLICE_DO_NEIGHBORWATCH;
			else if ((dwCityOrdinances & ORDINANCE_ANTI_DRUG_CAMPAIGN) == 0)
				nAdvice = ADVICE_ORDIN_DO_ANTIDRUGCAMPGN;
			else if ((dwCityOrdinances & ORDINANCE_LEGALIZED_GAMBLING) != 0)
				nAdvice = ADVICE_ORDIN_DROP_LEGALGAMBLING;
		}
		else if ((dwCityOrdinances & ORDINANCE_INCOME_TAX) != 0 && wCityDemand[DEMAND_RES] < -666)
			nAdvice = ADVICE_ORDIN_DROP_INCOMETAX;
		else if ((dwCityOrdinances & ORDINANCE_SALES_TAX) != 0 && wCityDemand[DEMAND_COM] < -666)
			nAdvice = ADVICE_ORDIN_DROP_SALESTAX;
		break;
	case ADVISOR_BONDS:
		nAdvice = ADVICE_NONE;
		if (dwCityFunds < -1000) 
			nAdvice = ADVICE_BOND_FLOATCITYEXPAND;
		else if (dwCityFunds < 0) 
			nAdvice = ADVICE_BOND_CUTBACK;
		else if (wNationalFedRate + (__int16)(25000 * dwCityBonds / (dwCityValue + 1)) + 1 < 4) 
			nAdvice = ADVICE_BOND_FLOATGOODRATES;
		else if (pBudgetArr[BUDGET_COMFUND].iEstimatedCost + pBudgetArr[BUDGET_INDFUND].iEstimatedCost + pBudgetArr[BUDGET_RESFUND].iEstimatedCost < pBudgetArr[BUDGET_BOND].iEstimatedCost)
			nAdvice = ADVICE_BOND_OUTSTANDINGKILLING;
		break;
	case ADVISOR_POLICE:
		if (nCityCrime > 40) 
			nAdvice = ADVICE_POLICE_OUTOFCONTROL;
		else if (nCityCrime > 30 && (dwCityOrdinances & ORDINANCE_NEIGHBORHOOD_WATCH) == 0) 
			nAdvice = ADVICE_POLICE_DO_NEIGHBORWATCH;
		else if (nCityCrime >= 20)
			nAdvice = ADVICE_POLICE_NATAVERAGE;
		else
			nAdvice = ADVICE_POLICE_CRIMELOW;
		break;
	case ADVISOR_FIRE:
		nFireCoverage = pBudgetArr[BUDGET_FIRE].iFundingPercent * (150 * wTileCount[TILE_SERVICES_FIRE] / 9);
		if (nFireCoverage < dwCityPopulation) 
			nAdvice = ADVICE_FIRE_COVERAGENEEDMORE;
		else if ((int)(2 * nFireCoverage) / 3 <= (int)dwCityPopulation)
			nAdvice = ADVICE_FIRE_COVERAGEADEQUATE;
		else
			nAdvice = ADVICE_FIRE_COVERAGEEXCELLENT;
		break;
	case ADVISOR_HEALTH:
		nHealthCoverage = pBudgetArr[BUDGET_HEALTH].iFundingPercent * (250 * wTileCount[TILE_SERVICES_HOSPITAL] / 9);
		if (nHealthCoverage < dwCityPopulation) 
			nAdvice = ADVICE_HEALTH_NEEDMORE;
		else if ((int)(2 * nHealthCoverage) / 3 <= (int)dwCityPopulation) {
			nAdvice = ADVICE_HEALTH_ADEQUATE;
			nHealthOrd = rand() % 3;
			if (nHealthOrd > 0) {
				if (nHealthOrd == 1) {
					if ((dwCityOrdinances & ORDINANCE_CPR_TRAINING) == 0)
						nAdvice = ADVICE_HEALTH_DO_CPRTRAINING;
				}
				else if (nHealthOrd == 2) {
					if ((dwCityOrdinances & ORDINANCE_FREE_CLINICS) == 0)
						nAdvice = ADVICE_HEALTH_DO_FREECLINICS;
				}
			}
			else if ((dwCityOrdinances & ORDINANCE_PUBLIC_SMOKING_BAN) == 0)
				nAdvice = ADVICE_HEALTH_DO_SMOKINGBAN;
		}
		else
			nAdvice = ADVICE_HEALTH_EXCELLENT;
		break;
	case ADVISOR_EDUCATION:
		if (pBudgetArr[BUDGET_SCHOOL].iFundingPercent * (15 * wTileCount[TILE_SERVICES_SCHOOL] / 9) < (int)(pRawPopRatioTable[2] + pRawPopRatioTable[1])) 
			nAdvice = ADVICE_EDUCATION_NEEDMORESCHOOLS;
		else if (pBudgetArr[BUDGET_COLLEGE].iFundingPercent * (50 * wTileCount[TILE_SERVICES_COLLEGE] / 16) < (int)pRawPopRatioTable[3])
			nAdvice = ADVICE_EDUCATION_NEEDMORECOLLEGES;
		else
			nAdvice = ADVICE_EDUCATION_ADEQUATE;
		break;
	case ADVISOR_TRANSIT:
		nTotalCosts = 0;
		nTotalFunding = 0;
		for (nItem = BUDGET_ROAD; nItem <= BUDGET_TUNNEL; ++nItem) {
			nTotalCosts += pBudgetArr[nItem].iCurrentCosts;
			nTotalFunding += pBudgetArr[nItem].iFundingPercent;
		}
		nAdvice = ADVICE_NONE;
		if (nTotalFunding < 600) 
			nAdvice = ADVICE_TRANSIT_YESWECAN; // and we do regret it...
		else if (dwCityPopulation / 100 > nTotalCosts) 
			nAdvice = ADVICE_TRANSIT_INADEQUATEFLOATBOND;
		else if (dwCityPopulation / 10 < nTotalCosts)
			nAdvice = ADVICE_TRANSIT_TOOMANYROADS;
		break;
	default:
		break;
	}

	if (nAdvice < ADVICE_NONE || nAdvice >= ADVICE_COUNT)
		return;

	Game_BudgetAdvisorDialog_SetAdvisorMessage(pBudgetAdvisorDlg, nAdvice);
	if (nSound >= SOUND_START && nSound <= SOUND_SILENT)
		Game_SimcityApp_SoundPlaySound(pSCApp, nSound);
}

extern "C" BOOL _declspec(naked) Hook_BudgetAdvisorDialog_OnInitDialog_MsgHandling() {
	CBudgetAdvisorDialog *pThis;

	__asm {
		mov ecx, esi
		mov [pThis], ecx
	}

	if (bAdvisorCustomString)
		GameMain_Wnd_UpdateData(pThis, 0);
	else
		BudgetAdvisorDialog_SelectTypeAndSetMessage(pThis);

	__asm mov ecx, [pThis]

	GAMEJMP(0x41A50B);
}

// Spawns a budget advisor dialog for the selected advisor with a custom message.
void DisplayBudgetAdvisorMessage(int iAdvisor, const char* szMessage) {
	CSimcityAppPrimary* pSCApp;
	CMainFrame* pMainFrm;

	pSCApp = &pCSimcityAppThis;
	pMainFrm = (CMainFrame*)pSCApp->m_pMainWnd;

	// Construct the dialog and display it
	CBudgetAdvisorDialog dlg;
	Game_BudgetAdvisorDialog_Cons(&dlg, pMainFrm);
	dlg.m_dwBDAType = iAdvisor;
	GameMain_String_OperatorSet(&dlg.m_dwBDACStringOne, (char*)szMessage);
	bAdvisorCustomString = true;
	Game_GameDialog_DoModal(&dlg);
	bAdvisorCustomString = false;
	Game_BudgetAdvisorDialog_Dest(&dlg);
}

void InstallCityManagementHooks_SC2K1996(void) {
	// Hook for SimulationPrepareBudgetDialog
	SafeVirtualProtect((LPVOID)0x4015E6, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015E6, Hook_SimulationPrepareBudgetDialog);

	// Hook for CBudgetMainDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x402649, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402649, Hook_BudgetMainDialog_OnInitDialog);

	// Hook for CBudgetMainDialog::ReleaseObjects
	SafeVirtualProtect((LPVOID)0x402A04, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402A04, Hook_BudgetMainDialog_ReleaseObjects);

	// Hook for CBudgetMainDialog::OnPaint
	SafeVirtualProtect((LPVOID)0x4020F9, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4020F9, Hook_BudgetMainDialog_OnPaint);

	// Hook for CBudgetMainDialog::DrawCosts
	SafeVirtualProtect((LPVOID)0x402347, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402347, Hook_BudgetMainDialog_DrawCosts);

	// Hook for CBudgetMainDialog::OnVScroll
	SafeVirtualProtect((LPVOID)0x402ACC, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402ACC, Hook_BudgetMainDialog_OnVScroll);

	// Hook for CBudgetMainDialog::SetCursorAndClearGraphics
	SafeVirtualProtect((LPVOID)0x40252C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40252C, Hook_BudgetMainDialog_SetCursorAndClearGraphics);

	// Hook for CBudgetMainDialog::OpenOrdinanceDialog
	SafeVirtualProtect((LPVOID)0x401AFF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401AFF, Hook_BudgetMainDialog_OpenOrdinanceDialog);

	// Hook for CSimcityView::DoOrdinance
	SafeVirtualProtect((LPVOID)0x4013C5, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4013C5, Hook_SimcityView_DoOrdinance);

	// Hook for CBudgetOrdinanceDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x401447, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401447, Hook_BudgetOrdinanceDialog_OnInitDialog);

	// Hook for CBudgetZoneTaxDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x40166D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40166D, Hook_BudgetZoneTaxDialog_OnInitDialog);

	// Hook for CBudgetZoneTaxDialog::OnVScroll
	SafeVirtualProtect((LPVOID)0x402A0E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402A0E, Hook_BudgetZoneTaxDialog_OnVScroll);

	// Hook for CBudgetEducationDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x40231A, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40231A, Hook_BudgetEducationDialog_OnInitDialog);

	// Hook for CBudgetEducationDialog::OnVScroll
	SafeVirtualProtect((LPVOID)0x40243C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40243C, Hook_BudgetEducationDialog_OnVScroll);

	// Hook for CBudgetTransitDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x40220C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40220C, Hook_BudgetTransitDialog_OnInitDialog);

	// Hook for CBudgetTransitDialog::OnVScroll
	SafeVirtualProtect((LPVOID)0x4011C2, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4011C2, Hook_BudgetTransitDialog_OnVScroll);

	// Hook for CPopulationDialog::DoDataExchange
	SafeVirtualProtect((LPVOID)0x402DE2, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402DE2, Hook_PopulationDialog_DoDataExchange);

	// Hook for CPopulationDialog::ToggleDialog
	SafeVirtualProtect((LPVOID)0x401EB5, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401EB5, Hook_PopulationDialog_ToggleDialog);

	// Hook for CCityMapDialog::OnLButtonDown
	SafeVirtualProtect((LPVOID)0x402FD6, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402FD6, Hook_CityMapDialog_OnLButtonDown);

	// Hook for CSimGraphDialog::DoDataExchange
	SafeVirtualProtect((LPVOID)0x4015A5, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015A5, Hook_SimGraphDialog_DoDataExchange);

	// Hook for CSimGraphDialog::UpdateDialog
	SafeVirtualProtect((LPVOID)0x402CFC, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402CFC, Hook_SimGraphDialog_UpdateDialog);

	// Hook for CSimGraphDialog::ToggleDialog
	SafeVirtualProtect((LPVOID)0x403003, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403003, Hook_SimGraphDialog_ToggleDialog);

	// Hook for CSimGraphDialog::HideDialog
	SafeVirtualProtect((LPVOID)0x4023C4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4023C4, Hook_SimGraphDialog_HideDialog);

	// Detour hook for CBudgetAdvisorDialog::OnInitDialog
	// This is to account for the following:
	// 1) Being able to make use of any advisor while sending a custom string.
	// 2) To import the standard advisor/advice handling and account for bug fixes.
	SafeVirtualProtect((LPVOID)0x41A4F6, 21, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x41A4F6, 0x90, 21);
	NEWJMP((LPVOID)0x41A4F6, Hook_BudgetAdvisorDialog_OnInitDialog_MsgHandling);
}
