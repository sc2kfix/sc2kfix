// sc2kfix hooks/hook_spritesandtilesets.cpp: sprite and tileset handling
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SPRITE_DEBUG_OTHER 1
#define SPRITE_DEBUG_SPRITES 2
#define SPRITE_DEBUG_TILESETS 4

#define SPRITE_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SPRITE_DEBUG
#define SPRITE_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT sprite_debug = SPRITE_DEBUG;

static DWORD dwDummy; 

std::vector<sprite_ids_t> spriteIDs;

static BOOL CheckForExistingID(WORD nID) {
	WORD nSkipHit;
	sprite_ids_t *pSprEnt;

	nSkipHit = 0;
	for (int i = 0; i < (int)spriteIDs.size(); i++) {
		pSprEnt = &spriteIDs[i];
		if (pSprEnt && pSprEnt->nID == nID) {
			if (sprite_debug & SPRITE_DEBUG_SPRITES)
				ConsoleLog(LOG_DEBUG, "CheckForExistingID(%u): (%u, %u, 0x%06X, %u) ID already exists.\n", nID, pSprEnt->nArcID, pSprEnt->nID, pSprEnt->dwOffset, pSprEnt->dwSize);
			pSprEnt->bMultiple = TRUE;
			nSkipHit = ++pSprEnt->nSkipHit;
		}
	}
	return (nSkipHit) ? TRUE : FALSE;
}

static void AllocateAndLoadSprites1996(CMFC3XFile *pFile, sprite_archive_t *lpBuf, WORD nSpriteSet) {
	void(__cdecl *H_OpDelete)(void *) = (void(__cdecl *)(void *))0x401C99;
	void *(__cdecl *H_AllocateDataEntry)(size_t iSz) = (void *(__cdecl *)(size_t))0x402045;
	UINT(__thiscall *H_CFileRead)(CMFC3XFile *, void *, DWORD) = (UINT(__thiscall *)(CMFC3XFile *, void *, DWORD))0x4A8313;
	LONG(__thiscall *H_CFileSeek)(CMFC3XFile *, LONG, UINT) = (LONG(__thiscall *)(CMFC3XFile *, LONG, UINT))0x4A83B8;

	WORD nPos, nID;
	sprite_ids_t *pSprEnt;
	void *pSpriteData;

	H_CFileSeek(pFile, lpBuf->pData[0].sprHeader.dwAddress, 0);
	for (nPos = 0; nPos < spriteIDs.size(); ++nPos) {
		pSprEnt = &spriteIDs[nPos];
		if (pSprEnt && pSprEnt->nArcID == nSpriteSet) {
			nID = pSprEnt->nID;
			if (pSprEnt->bMultiple)
				if (sprite_debug & SPRITE_DEBUG_SPRITES)
					ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): Multiple sprites with the same ID Detected (%u, 0x%06X, %u) (%u)\n", nSpriteSet, nID, pSprEnt->dwOffset, pSprEnt->dwSize, pSprEnt->nSkipHit);
			if (pSprEnt->dwSize > 0) {
				pSpriteData = H_AllocateDataEntry(pSprEnt->dwSize);
				if (pSpriteData) {
					if (H_CFileRead(pFile, pSpriteData, pSprEnt->dwSize) == pSprEnt->dwSize) {
						if (pSprEnt->bMultiple && pSprEnt->nSkipHit > 0) {
							if (sprite_debug & SPRITE_DEBUG_SPRITES)
								ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): discarding skipped sprite with ID (%u, 0x%06X, %u).\n", nSpriteSet, nID, pSprEnt->dwOffset, pSprEnt->dwSize);
							H_OpDelete(pSpriteData);
							continue;
						}
						if (pArrSpriteHeaders[nID].dwAddress) {
							H_OpDelete((void *)pArrSpriteHeaders[nID].dwAddress);
							pArrSpriteHeaders[nID].dwAddress = 0;
						}
						pArrSpriteHeaders[nID].dwAddress = (DWORD)pSpriteData;
						pArrSpriteHeaders[nID].wHeight = pSprEnt->wHeight;
						pArrSpriteHeaders[nID].wWidth = pSprEnt->wWidth;
					}
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_LoadSpriteDataArchive1996(WORD nSpriteSet) {
	int(__cdecl *H_SwLong)(int) = (int(__cdecl *)(int))0x401429;
	__int16(__cdecl *H_SwShort)(__int16) = (__int16(__cdecl *)(__int16))0x401861;
	void(__cdecl *H_FailRadio)(UINT) = (void(__cdecl *)(UINT))0x402A40;
	void(__thiscall *H_SimcityAppGetValueStringA)(void *, CMFC3XString *, const char *, const char *) = (void(__thiscall *)(void *, CMFC3XString *, const char *, const char *))0x402F4F;
	void(__cdecl *H_CStringFormat)(CMFC3XString *, char const *Ptr, ...) = (void(__cdecl *)(CMFC3XString *, char const *Ptr, ...))0x49EBD3;
	CMFC3XString *(__thiscall *H_CStringCons)(CMFC3XString *) = (CMFC3XString *(__thiscall *)(CMFC3XString *))0x4A2C28;
	void(__thiscall *H_CStringDest)(CMFC3XString *) = (void(__thiscall *)(CMFC3XString *))0x4A2CB0;
	CMFC3XFile *(__thiscall *H_CFileCons)(CMFC3XFile *) = (CMFC3XFile *(__thiscall *)(CMFC3XFile *))0x4A7E82;
	void(__thiscall *H_CFileDest)(CMFC3XFile *) = (void(__thiscall *)(CMFC3XFile *))0x4A8072;
	BOOL(__thiscall *H_CFileOpen)(CMFC3XFile *, LPCTSTR, UINT, void *) = (BOOL(__thiscall *)(CMFC3XFile *, LPCTSTR, UINT, void *))0x4A8190;
	UINT(__thiscall *H_CFileRead)(CMFC3XFile *, void *, DWORD) = (UINT(__thiscall *)(CMFC3XFile *, void *, DWORD))0x4A8313;
	void(__thiscall *H_CFileClose)(CMFC3XFile *) = (void(__thiscall *)(CMFC3XFile *))0x4A8448;
	DWORD(__thiscall *H_CFileGetLength)(CMFC3XFile *) = (DWORD(__thiscall *)(CMFC3XFile *))0x4A854E;

	CMFC3XString *cStrDataArchiveNames = (CMFC3XString *)0x4CA160;
	const char *aPaths = (const char *)0x4E61D0;
	const char *cBackslash = (const char *)0x4E6278;
	const char *aData = (const char *)0x4E728C;
	sprite_archive_stored_t *dwBaseSpriteLoading = (sprite_archive_stored_t *)0x4E7448;

	CMFC3XFile datArchive;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nPos, nNextPos;
	__int16 nArcFileCnt;
	WORD nID, nNextID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;
	DWORD dwNextAddress;
	sprite_ids_t spriteEnt;
	int nSize;

	H_CFileCons(&datArchive);
	H_CStringCons(&retString);
	H_CStringCons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> LoadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	H_SimcityAppGetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	H_CStringFormat(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	if (H_CFileOpen(&datArchive, retStrPath.m_pchData, 0, 0)) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		nFlen = H_CFileGetLength(&datArchive);
		H_CFileRead(&datArchive, &nArcFileCnt, 2);
		nArcFileCnt = H_SwShort(nArcFileCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nArcFileCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nFileCnt = nArcFileCnt;
		if (lpBuf) {
			if (H_CFileRead(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nArcFileCnt; ++nPos) {
					nID = H_SwShort(lpBuf->pData[nPos].nID);
					lpBuf->pData[nPos].nID = nID;

					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->dwAddress = H_SwLong(pSprtHead->dwAddress);

					pArrSpriteHeaders[nID] = *pSprtHead;
					pArrSpriteHeaders[nID].dwAddress = 0;
					pArrSpriteHeaders[nID].wHeight = H_SwShort(pArrSpriteHeaders[nID].wHeight);
					pArrSpriteHeaders[nID].wWidth = H_SwShort(pArrSpriteHeaders[nID].wWidth);

					nNextPos = (nPos >= nArcFileCnt - 1) ? -1 : nPos + 1;
					nNextID = (nNextPos >=0) ? nID : -1;

					if (nNextPos >= 0) {
						// The next position hasn't yet been processed, do so here so
						// we can get the file size.
						dwNextAddress = H_SwLong(lpBuf->pData[nNextPos].sprHeader.dwAddress);
						nSize = (dwNextAddress - pSprtHead->dwAddress);
					}
					else
						nSize = (nFlen - pSprtHead->dwAddress);

					if (nSize > 0) {
						spriteEnt.nArcID = nSpriteSet;
						spriteEnt.nID = nID;
						spriteEnt.dwOffset = pSprtHead->dwAddress;
						spriteEnt.dwSize = nSize;
						spriteEnt.wHeight = pArrSpriteHeaders[nID].wHeight;
						spriteEnt.wWidth = pArrSpriteHeaders[nID].wWidth;
						spriteEnt.nSkipHit = 0;
						spriteEnt.bMultiple = (CheckForExistingID(nID)) ? TRUE : FALSE;
						spriteIDs.push_back(spriteEnt);
					}
				}

				AllocateAndLoadSprites1996(&datArchive, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		H_CFileClose(&datArchive);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		H_FailRadio(uFailMsg);

GETOUT:
	H_CStringDest(&retStrPath);
	H_CStringDest(&retString);
	H_CFileDest(&datArchive);
}

static void ReloadSpriteDataArchive1996(WORD nSpriteSet) {
	int(__cdecl *H_SwLong)(int) = (int(__cdecl *)(int))0x401429;
	__int16(__cdecl *H_SwShort)(__int16) = (__int16(__cdecl *)(__int16))0x401861;
	void(__cdecl *H_FailRadio)(UINT) = (void(__cdecl *)(UINT))0x402A40;
	void(__thiscall *H_SimcityAppGetValueStringA)(void *, CMFC3XString *, const char *, const char *) = (void(__thiscall *)(void *, CMFC3XString *, const char *, const char *))0x402F4F;
	void(__cdecl *H_CStringFormat)(CMFC3XString *, char const *Ptr, ...) = (void(__cdecl *)(CMFC3XString *, char const *Ptr, ...))0x49EBD3;
	CMFC3XString *(__thiscall *H_CStringCons)(CMFC3XString *) = (CMFC3XString *(__thiscall *)(CMFC3XString *))0x4A2C28;
	void(__thiscall *H_CStringDest)(CMFC3XString *) = (void(__thiscall *)(CMFC3XString *))0x4A2CB0;
	CMFC3XFile *(__thiscall *H_CFileCons)(CMFC3XFile *) = (CMFC3XFile *(__thiscall *)(CMFC3XFile *))0x4A7E82;
	void(__thiscall *H_CFileDest)(CMFC3XFile *) = (void(__thiscall *)(CMFC3XFile *))0x4A8072;
	BOOL(__thiscall *H_CFileOpen)(CMFC3XFile *, LPCTSTR, UINT, void *) = (BOOL(__thiscall *)(CMFC3XFile *, LPCTSTR, UINT, void *))0x4A8190;
	UINT(__thiscall *H_CFileRead)(CMFC3XFile *, void *, DWORD) = (UINT(__thiscall *)(CMFC3XFile *, void *, DWORD))0x4A8313;
	void(__thiscall *H_CFileClose)(CMFC3XFile *) = (void(__thiscall *)(CMFC3XFile *))0x4A8448;
	DWORD(__thiscall *H_CFileGetLength)(CMFC3XFile *) = (DWORD(__thiscall *)(CMFC3XFile *))0x4A854E;

	CMFC3XString *cStrDataArchiveNames = (CMFC3XString *)0x4CA160;
	const char *aPaths = (const char *)0x4E61D0;
	const char *cBackslash = (const char *)0x4E6278;
	const char *aData = (const char *)0x4E728C;
	sprite_archive_stored_t *dwBaseSpriteLoading = (sprite_archive_stored_t *)0x4E7448;

	CMFC3XFile datArchive;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nArcFileCnt;
	WORD nPos;
	WORD nID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;

	H_CFileCons(&datArchive);
	H_CStringCons(&retString);
	H_CStringCons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> ReloadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	H_SimcityAppGetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	H_CStringFormat(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	if (H_CFileOpen(&datArchive, retStrPath.m_pchData, 0, 0)) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		nFlen = H_CFileGetLength(&datArchive);
		H_CFileRead(&datArchive, &nArcFileCnt, 2);
		nArcFileCnt = H_SwShort(nArcFileCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nArcFileCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nFileCnt = nArcFileCnt;
		if (lpBuf) {
			if (H_CFileRead(&datArchive, &lpBuf->pData, nBufSize) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nArcFileCnt; ++nPos) {
					nID = H_SwShort(lpBuf->pData[nPos].nID);
					lpBuf->pData[nPos].nID = nID;
					pSprtHead = (sprite_header_t *)&lpBuf->pData[nPos].sprHeader;
					pSprtHead->dwAddress = H_SwLong(pSprtHead->dwAddress);
					pSprtHead->wHeight = H_SwShort(pSprtHead->wHeight);
					pSprtHead->wWidth = H_SwShort(pSprtHead->wWidth);
				}

				AllocateAndLoadSprites1996(&datArchive, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		H_CFileClose(&datArchive);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		H_FailRadio(uFailMsg);

GETOUT:
	H_CStringDest(&retStrPath);
	H_CStringDest(&retString);
	H_CFileDest(&datArchive);
}

void ReloadDefaultTileSet_SC2K1996() {
	void(__thiscall *H_CCmdTargetBeginWaitCursor)(void *) = (void(__thiscall *)(void *))0x4A28BB;
	void(__thiscall *H_CCmdTargetEndWaitCursor)(void *) = (void(__thiscall *)(void *))0x4A28D2;

	DWORD *pApp;

	pApp = &pCSimcityAppThis;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to reload the base game tile set?", gamePrimaryKey, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDYES)
		return;

	H_CCmdTargetBeginWaitCursor(pApp);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	H_CCmdTargetEndWaitCursor(pApp);
}

void InstallSpriteAndTileSetHooks_SC2K1996(void) {
	// Hook LoadSpriteDataArchive
	VirtualProtect((LPVOID)0x4029B4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4029B4, Hook_LoadSpriteDataArchive1996);
}
