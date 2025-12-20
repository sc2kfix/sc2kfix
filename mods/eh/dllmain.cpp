// sc2kfix/eh dllmain.cpp: Canadianifies your military bases
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// This is not meant to be a serious mod. It serves primarily as an example of how to manipulate
// game state, make decisions based on events in a city, and make calls into SimCity 2000's code.
// The gentle ribbing from north of the 49th is just an added bonus.

#undef UNICODE
#define GAMEOFF_IMPL

#include <windows.h>
#include "../sc2kfix.h"
#include "resource.h"

#ifndef Game_ReadTilesetFile
GAMECALL(0x4021F8, void, __cdecl, ReadTilesetFile, const char*)
#endif

#ifndef Game_DisplayEventMessage
GAMECALL(0x40286A, void, __cdecl, DisplayEventMessage, int, CMFC3XString*)
#endif

sc2kfix_mod_hook_t stModHooks[] = {
	{ "Hook_PrepareGame_After", 0 },
	{ "Hook_SimulationGrowSpecificZone_Success", 0 }
};

sc2kfix_mod_info_t stModInfo = {
	/* .iModInfoVersion = */ 1,
	/* .iModVersionMajor = */ 0,
	/* .iModVersionMinor = */ 1,
	/* .iModVersionPatch = */ 0,
	/* .iMinimumVersionMajor = */ 0,
	/* .iMinimumVersionMinor = */ 10,
	/* .iMinimumVersionPatch = */ 0,
	/* .szModName = */ "Eh",
	/* .szModShortName = */ "eh",
	/* .szModAuthor = */ "araxestroy",
	/* .szModDescription = */ "OUR HOME AND NATIVE LAAAAAAAND",
	/* .iHookCount = */ HOOKS_COUNT(stModHooks),
	/* .pstHooks = */ stModHooks
};

BOOL bAlreadyCheered = FALSE;
BOOL bTilesetAvailable = FALSE;
char szTilesetFilename[L_tmpnam_s] = { 0 };

HOOKCB sc2kfix_mod_info_t* HookCb_GetModInfo(void) {
	return &stModInfo;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
	{
		HRSRC hResFind = FindResourceA(hModule, MAKEINTRESOURCE(IDR_EH_MIFF), "BLOB");
		if (hResFind) {
			HANDLE hMiffResource = LoadResource(hModule, hResFind);
			if (hMiffResource) {
				size_t nBufSize = SizeofResource(hModule, hResFind);
				BYTE* ptr = (BYTE*)LockResource(hMiffResource);

				if (tmpnam_s(szTilesetFilename, L_tmpnam_s)) {
					LOG(LOG_ERROR, "she's right hosed bud (tmpnam_s)\n");
					bTilesetAvailable = FALSE;
					break;
				}

				FILE* f;
				if (fopen_s(&f, szTilesetFilename, "wb")) {
					LOG(LOG_ERROR, "she's right hosed bud (fopen_s)\n");
					bTilesetAvailable = FALSE;
					break;
				}

				size_t nBytesWritten = fwrite(ptr, 1, nBufSize, f);
				if (ferror(f)) {
					LOG(LOG_ERROR, "she's right hosed bud (fwrite)\n");
					fclose(f);
					bTilesetAvailable = FALSE;
					break;
				}

#ifdef EH_DEBUG
				LOG(LOG_DEBUG, "the boys are waiting, y'know (%s, %u)\n", szTilesetFilename, nBytesWritten);
#endif

				fclose(f);
				bTilesetAvailable = TRUE;
			}
		}
	}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

HOOKCB void Hook_PrepareGame_After() {
	if (bTilesetAvailable) {
		// Canadians have taken over your military base!
		Game_ReadTilesetFile(szTilesetFilename);
		bAlreadyCheered = FALSE;
		LOG(LOG_INFO, "oh hey there bud\n");
	}
}

HOOKCB void Hook_SimulationGrowSpecificZone_Success(__int16 iX, __int16 iY, BYTE iTileID, __int16 iZoneType) {
	if (bTilesetAvailable && !bAlreadyCheered && iTileID == TILE_MILITARY_TOPSECRET) {
		Game_SimcityApp_SoundPlaySound(&pCSimcityAppThis, SOUND_CHEERS);
		CMFC3XString cStr;
		GameMain_String_Cons(&cStr);
		GameMain_String_OperatorSet(&cStr, (char*)"Your brand new military base has been taken over by Canadians!");
		Game_DisplayEventMessage(407, &cStr);
		GameMain_String_Dest(&cStr);
		bAlreadyCheered = TRUE;
	}
}
