// sc2kfix hooks/hook_currencystring.cpp: hooks for the CurrencyString class.
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

char *L_GetCurrencyString_SC2K1996(unsigned __int32 nAmount) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CCurrencyString *pCurrString = NULL;
	const char *pCurrStr = NULL;
	char *pOutStr = NULL;

	pCurrString = new CCurrencyString();
	if (pCurrString)
		pCurrString->pStr = NULL;
	else
		return NULL;
	if (_stricmp(pSCApp->dwSCACStringLang.m_pchData, gameLangGerman) == 0)
		pCurrStr = gameCurrDM;
	else if (_stricmp(pSCApp->dwSCACStringLang.m_pchData, gameLangFrench) == 0)
		pCurrStr = gameCurrFF;
	else
		pCurrStr = gameCurrDollar;
	pCurrString = Game_CurrencyString_SetString(pCurrString, pCurrStr, 20, (double)nAmount);
	if (pCurrString) {
		if (pCurrString->pStr) {
			Game_CurrencyString_TruncateAtSpace(pCurrString);
			pOutStr = _strdup(pCurrString->pStr);
			Game_CurrencyString_Dest(pCurrString);
		}
		delete pCurrString;
		pCurrString = NULL;
	}
	return (pOutStr) ? pOutStr : NULL;
}

void InstallCurrencyStringHooks_SC2K1996(void) {
	// Empty for now - but retained for if we
	// ever need it.
}
