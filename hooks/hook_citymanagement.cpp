// sc2kfix hooks/hook_citymanagement.cpp: hooks to do with city management: budget
// and ordinances.
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

extern int nOwnDrwDlg;
extern CMFC3XWnd *pStoredWnd;

extern "C" int __stdcall Hook_BudgetMainDialog_OnInitDialog() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	int ret = GameMain_BudgetMainDialog_OnInitDialog(pThis);
	pStoredWnd = pThis;
	nOwnDrwDlg = OWNDRW_DLG_BUDGETMAIN;
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

static int nLastSBCode = -1;

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
		nLastSBCode = -1;
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
			nLastSBCode = nSBCode;
		dwBudgetScrollClicked = 0;
		if (nLastSBCode == SB_VERT) // Up Arrow
			dwBudgetScrollGoingUp = 0;
		else if (nLastSBCode == SB_HORZ) // Down Arrow
			dwBudgetScrollGoingUp = 1;
		// Reset here, otherwise it can get stuck trying to go in a specific direction.
		if (nSBCode == SB_ENDSCROLL)
			nLastSBCode = -1;
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
	else {
		// Always record the last one here (regardless of what it is).
		nLastSBCode = nSBCode;
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
	RECT areaRect, tdeRect;
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
	// The field width is used in this case after the
	// coordinates have been converted.
	GetWindowRect(pThis->dwBDEditTransitTDE.m_hWnd, &tdeRect);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.left);
	ScreenToClient(pThis->m_hWnd, (LPPOINT)&tdeRect.right);
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

static void OpenOrdinanceDialog(CBudgetOrdinanceDialog *pOrdinanceDialog) {
	bOrdinanceOpen = true;
	Game_GameDialog_DoModal(pOrdinanceDialog);
	bOrdinanceOpen = false;
}

extern "C" void __stdcall Hook_BudgetMainDialog_OpenOrdinanceDialog() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	BudgetMain_PreCheckHourGlassTimer(pThis);
	OpenOrdinanceDialog(&ordinanceDlg);
	BudgetMain_PostCheckHourGlassTimer(pThis);
	Game_BudgetMainDialog_UpdateInternalInformation(pThis, 0);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
}

extern "C" void __stdcall Hook_SimcityView_DoOrdinance() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	OpenOrdinanceDialog(&ordinanceDlg);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
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
}
