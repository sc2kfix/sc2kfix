// sc2kfix hooks/hook_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is where I test a bunch of stuff live to cross reference what I think is going on in the
// game engine based on decompiling things in IDA and following the code paths. As a result,
// there's a lot of experimental stuff in here. Comments will probably be unhelpful. Godspeed.

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define MISCHOOK_DEBUG_OTHER 1
#define MISCHOOK_DEBUG_MILITARY 2
#define MISCHOOK_DEBUG_MENU 4
#define MISCHOOK_DEBUG_SAVES 8
#define MISCHOOK_DEBUG_WINDOW 16
#define MISCHOOK_DEBUG_DISASTERS 32
#define MISCHOOK_DEBUG_MOVIES 64
#define MISCHOOK_DEBUG_SMACK 128

#define MISCHOOK_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

static char def_data_path[] = "A:\\DATA\\";

UINT iMilitaryBaseTries = 0;
WORD wMilitaryBaseX = 0, wMilitaryBaseY = 0;

AFX_MSGMAP_ENTRY afxMessageMapMainMenu[9];
DLGPROC lpNewCityAfxProc = NULL;
char szTempMayorName[24] = { 0 };
char szCurrentMonthDay[24] = { 0 };
const char* szMonthNames[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

static void AdjustDefDataPathDrive() {
	// Let's get the drive letter from
	// the movies path.
	const char *temp = GetSetMoviesPath();
	if (!temp)
		return;
	def_data_path[0] = temp[0];
}

// Reference and inspiration for this comes from the separate
// 'simcity-noinstall' project.
static const char *AdjustSource(char *buf, const char *path) {
	int plen = strlen(path);
	int flen = strlen(def_data_path);
	if (plen <= flen || _strnicmp(def_data_path, path, flen) != 0) {
		return path;
	}

	char temp[MAX_PATH+1];
	const char *ptemp = GetSetMoviesPath();
	if (!ptemp) {
		return path;
	}

	memset(temp, 0, sizeof(temp));

	strcpy_s(temp, MAX_PATH, path + (flen - 1));

	strcpy_s(buf, MAX_PATH, ptemp);
	strcat_s(buf, MAX_PATH, temp);

	if (mischook_debug & MISCHOOK_DEBUG_OTHER)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> Adjustment - %s -> %s\n", _ReturnAddress(), path, buf);

	return buf;
}

extern "C" HANDLE __stdcall Hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
	if (mischook_debug & MISCHOOK_DEBUG_OTHER)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), lpFileName,
			dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if (bSettingsUseLocalMovies) {
		if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
			(DWORD)_ReturnAddress() == 0x48A810) {
			char buf[MAX_PATH + 1];

			memset(buf, 0, sizeof(buf));

			HANDLE hFileHandle = CreateFileA(AdjustSource(buf, lpFileName), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			if (mischook_debug & MISCHOOK_DEBUG_OTHER)
				ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> CreateFileA(%s, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X) (0x%08x)\n", _ReturnAddress(), lpFileName,
					dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, hFileHandle);
			return hFileHandle;
		}
	}
	return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" HANDLE __stdcall Hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
	if (mischook_debug & MISCHOOK_DEBUG_OTHER)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> FindFirstFileA(%s, 0x%08X)\n", _ReturnAddress(), lpFileName, lpFindFileData);
	if (bSettingsUseLocalMovies) {
		if ((DWORD)_ReturnAddress() == 0x4A8A90 ||
			(DWORD)_ReturnAddress() == 0x48A810) {
			char buf[MAX_PATH + 1];

			memset(buf, 0, sizeof(buf));

			HANDLE hFileHandle = FindFirstFileA(AdjustSource(buf, lpFileName), lpFindFileData);
			if (mischook_debug & MISCHOOK_DEBUG_OTHER)
				ConsoleLog(LOG_DEBUG, "MISC: (Modification): 0x%08X -> FindFirstFileA(%s, 0x%08X) (0x%08x)\n", _ReturnAddress(), buf, lpFindFileData, hFileHandle);
			return hFileHandle;
		}
	}
	return FindFirstFileA(lpFileName, lpFindFileData);
}

// Override some strings that have egregiously bad grammar/capitalization.
// Maxis fail English? That's unpossible!
extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	if (hInstance == hSC2KAppModule && bSettingsUseNewStrings) {
		switch (uID) {
		case 97:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Hydroelectric Dam"))
				return strlen(lpBuffer);
			break;
		case 108:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Hydroelectric dams can only be placed on waterfall tiles."))
				return strlen(lpBuffer);
			break;
		case 111:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would intersect an existing tunnel."))
				return strlen(lpBuffer);
			break;
		case 112:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would leave the city limits."))
				return strlen(lpBuffer);
			break;
		case 113:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as it would be too deep in the terrain."))
				return strlen(lpBuffer);
			break;
		case 114:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel cannot be built as the exit terrain is unstable."))
				return strlen(lpBuffer);
			break;
		case 115:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"An existing subway or sewer line is blocking construction."))
				return strlen(lpBuffer);
			break;
		case 116:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Tunnel entrances must be placed on a hillside."))
				return strlen(lpBuffer);
			break;
		case 129:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Nuclear Power"))
				return strlen(lpBuffer);
			break;
		case 132:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Microwave Power"))
				return strlen(lpBuffer);
			break;
		case 133:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Fusion Power"))
				return strlen(lpBuffer);
			break;
		case 240:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Your nation's military is interested in building a base on your city's soil. "
				"This could mean extra revenue. It could also raise new problems. "
				"Do you wish to grant land to the military?"))
				return strlen(lpBuffer);
			break;
		case 289:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Current rates are %d%%.\r\n"
				"Do you wish to issue the bond?"))
				return strlen(lpBuffer);
			break;
		case 290:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"You need $10,000 in cash to repay an outstanding bond."))
				return strlen(lpBuffer);
			break;
		case 291:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"The oldest outstanding bond rate is %d%%.\r\n"
				"Do you wish to repay this bond?"))
				return strlen(lpBuffer);
			break;
		case 346:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Engineers report that tunnel construction costs will be %s.\r\n"
				"Do you wish to construct the tunnel?"))
				return strlen(lpBuffer);
			break;
		case 640:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Grocery store"))
				return strlen(lpBuffer);
			break;
		case 745:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Launch Arcology"))
				return strlen(lpBuffer);
			break;
		case 32921:
			if (!strcpy_s(lpBuffer, cchBufferMax,
				"Saves city every 5 years"))
				return strlen(lpBuffer);
			break;
		default:
			return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
		}
	}
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

// Hook LoadMenuA so we can insert our own menu items.
extern "C" HMENU __stdcall Hook_LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) {
	if ((DWORD)lpMenuName == 3 && hGameMenu)
		return hGameMenu;
	return LoadMenuA(hInstance, lpMenuName);
}

// Make sure our own menu items get enabled instead of disabled
extern "C" BOOL __stdcall Hook_EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable) {
	// XXX - There's gotta be a better way to do this.
	if (uIDEnableItem == 5 && uEnable == 0x403)
		return EnableMenuItem(hMenu, uIDEnableItem, MF_BYPOSITION | MF_ENABLED);
	return EnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

extern "C" BOOL __stdcall Hook_ShowWindow(HWND hWnd, int nCmdShow) {
	if (mischook_debug & MISCHOOK_DEBUG_WINDOW)
		ConsoleLog(LOG_DEBUG, "WND:  0x%08X -> ShowWindow(0x%08X, %i)\n", _ReturnAddress(), hWnd, nCmdShow);
	DWORD* CWndMainWindow = (DWORD*)*(DWORD*)0x4C702C;
	HWND hWndStatusBar = (HWND)CWndMainWindow[68];
	if (hWnd == hWndStatusBar && bSettingsUseStatusDialog) {
		if (hStatusDialog)
			ShowWindow(hStatusDialog, SW_SHOW);
		return ShowWindow(hWnd, SW_HIDE);
	}

	// Workaround for the game window not showing if started by a launcher process
	if (nCmdShow == 11 && (DWORD)_ReturnAddress() == 0x40586C)
		return ShowWindow(hWnd, SW_MAXIMIZE);

	return ShowWindow(hWnd, nCmdShow);
}

extern "C" DWORD __cdecl Hook_SmackOpen(LPCSTR lpFileName, uint32_t uFlags, int32_t iExBuf) {
	if (mischook_debug & MISCHOOK_DEBUG_SMACK)
		ConsoleLog(LOG_DEBUG, "SMK:  0x%08X -> _SmackOpen(%s, %u, %i)\n", _ReturnAddress(), lpFileName, uFlags, iExBuf);
	if (bSettingsUseLocalMovies) {
		char buf[MAX_PATH + 1];

		memset(buf, 0, sizeof(buf));

		return SMKOpenProc(AdjustSource(buf, lpFileName), uFlags, iExBuf);
	}
	return SMKOpenProc(lpFileName, uFlags, iExBuf);
}

static BOOL CALLBACK Hook_NewCityDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, 150, szSettingsMayorName);
		break;
	case WM_DESTROY:
		// XXX - there's probably a better window message to use here.
		memset(szTempMayorName, 0, 24);
		if (!GetDlgItemText(hwndDlg, 150, szTempMayorName, 24))
			strcpy_s(szTempMayorName, 24, szSettingsMayorName);

		strcpy_s(dwMapXLAB[0]->szLabel, 24, szTempMayorName);
		break;
	}

	return lpNewCityAfxProc(hwndDlg, message, wParam, lParam);
}

// Load our own version of the main menu and the New City dialog when called
extern "C" INT_PTR __stdcall Hook_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	switch ((DWORD)lpTemplateName) {
	case 101:
		lpNewCityAfxProc = lpDialogFunc;
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, Hook_NewCityDialogProc, dwInitParam);
	case 102:
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	case 103:
		if (bUpdateAvailable)
			return DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(103), hWndParent, lpDialogFunc, dwInitParam);
		return DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(20104), hWndParent, lpDialogFunc, dwInitParam);
	default:
		return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	}
}

// Fix military bases not growing.
// XXX - This could use a few extra lines as it's currently possible for a few placeable buildings
// to overwrite and effectively erase military zoned tiles, and I don't know what that will do to
// the simulation engine since it keeps meticulous track of things like that.
//
// We also might want to optionally add in a few more buildings to the growth algorithm for Army
// bases, as currently Army bases only ever build 0xE8 Small Hangar and 0xEF Military Parking Lot.
// Maybe add in 0xE3 Warehouse or 0xF1 Top Secret, since those seem to only grow on naval bases?
extern "C" void _declspec(naked) Hook_FixMilitaryBaseGrowth(void) {
	__asm {
		cmp bp, 0xDD
		jb bail
		cmp bp, 0xF9
		ja bail
		push 0x440D55					// Maxim 43:
		retn							// "If it's stupid and it works...
	bail:
		push 0x440E00					// ...it's still stupid and you're *lucky*."
		retn							//    - The Seventy Maxims of Maximally Effective Mercenaries
	}
}

// Hook to reset iMilitaryBaseTries if needed (new/loaded game, gilmartin)
extern "C" void _declspec(naked) Hook_SimulationProposeMilitaryBase(void) {
	if (mischook_debug & MISCHOOK_DEBUG_MILITARY)
		ConsoleLog(LOG_DEBUG, "MISC: SimulationProposeMilitaryBase called, resetting iMilitaryBaseTries.\n");
	iMilitaryBaseTries = 0;
	__asm {
		push 0x4142C0
		retn
	}
}

// Fix the game giving up after one attempt at placing a military base.
// 10 tries was enough to get an army base to spawn in the smallest crags of a map with a maxed-
// out mountain slider, so that's what we're going with here.
extern "C" void _declspec(naked) Hook_AttemptMultipleMilitaryBases(void) {
	if (iMilitaryBaseTries++ < 10) {
		if (mischook_debug & MISCHOOK_DEBUG_MILITARY)
			ConsoleLog(LOG_DEBUG, "MISC: Failed military base placement, attempting again.\n");
		__asm {
			push 0x4142E9
			retn
		}
	} else {
		__asm {
			push 0x4147AF
			retn
		}
	}
}

// Quick detour to pull the top-left corner coordinates of a spawned military base.
extern "C" void _declspec(naked) Hook_41442E(void) {
	__asm {
		mov edx, 0x4B234F				// AfxMessageBox
		call edx

		mov edx, [esp + 0x5C - 0x38]
		mov word ptr [wMilitaryBaseX], dx
		mov edx, [esp + 0x5C - 0x34]
		mov word ptr [wMilitaryBaseY], dx

		push 0x414433
		retn
	}
}

// Fix rail and highway border connections not loading properly
extern "C" void __stdcall Hook_LoadNeighborConnections1500(void) {
	short* wCityNeighborConnections1500 = (short*)0x4CA3F0;
	*wCityNeighborConnections1500 = 0;
	*(DWORD*)0x4C85A0 = 0;

	for (int x = 0; x < 128; x++) {
		for (int y = 0; y < 128; y++) {
			if (dwMapXTXT[x]->bTextOverlay[y] == 0xFA) {
				BYTE iTileID = dwMapXBLD[x]->iTileID[y];
				if (iTileID >= TILE_RAIL_LR && iTileID < TILE_TUNNEL_T
					|| iTileID >= TILE_CROSSOVER_ROADLR_RAILTB && iTileID < TILE_SUSPENSION_BRIDGE_START_B
					|| iTileID >= TILE_HIGHWAY_HTB && iTileID < TILE_REINFORCED_BRIDGE_PYLON)
					++*wCityNeighborConnections1500;
			}
		}
	}

	if (mischook_debug & MISCHOOK_DEBUG_SAVES)
		ConsoleLog(LOG_DEBUG, "SAVE: Loaded %d $1500 neighbor connections.\n", *wCityNeighborConnections1500);
}

// Window title hook, part 1
extern "C" char* __stdcall Hook_40D67D(void) {
	if (bSettingsTitleCalendar)
		sprintf_s(szCurrentMonthDay, 24, "%s %d,", szMonthNames[dwCityDays / 25 % 12], dwCityDays % 25 + 1);
	else
		sprintf_s(szCurrentMonthDay, 24, "%s", szMonthNames[dwCityDays / 25 % 12]);
	return szCurrentMonthDay;
}


// Window title hook, part 2 and refresh hook
extern "C" void _declspec(naked) Hook_4315D2(void) {
	__asm {
		// Update title bar
		push edx
		mov edx, 0x4E66F8
		mov ecx, [edx]
		mov edx, 0x4017B2
		call edx

		cmp [bSettingsFrequentCityRefresh], 0
		je skip_refresh

		// Refresh view for growth
		mov edx, 0x4E66F8
		mov ecx, [edx]
		push 0
		push 2
		push 0
		mov edx, 0x4AE0BC
		call edx

	skip_refresh:
		pop edx
		cmp edx, 24
		ja def

		push 0x4135DB
		retn

	def:
		push 0x413ABF
		retn
	}
}

extern "C" void _declspec(naked) Hook_SimulationStartDisaster(void) {
	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationStartDisaster(), wDisasterType = %u.\n", _ReturnAddress(), wDisasterType);

	__asm {
		push 0x45CF10
		retn
	}
}

extern "C" int __cdecl Hook_SimulationPrepareDisaster(DWORD* a1, __int16 a2, __int16 a3) {
	if (mischook_debug & MISCHOOK_DEBUG_DISASTERS)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> SimulationPrepareDisaster(0x%08X, %i, %i).\n", _ReturnAddress(), a1, a2, a3);

	a1[0] = a2;
	a1[1] = a3;

	return a2;
}

// Install hooks and run code that we only want to do for the 1996 Special Edition SIMCITY.EXE.
// This should probably have a better name. And maybe be broken out into smaller functions.
void InstallMiscHooks(void) {

	AdjustDefDataPathDrive();

	// Install CreateFileA hook
	*(DWORD*)(0x4EFADC) = (DWORD)Hook_CreateFileA;

	// Install FindFirstFileA hook
	*(DWORD*)(0x4EFB8C) = (DWORD)Hook_FindFirstFileA;

	// Install LoadStringA hook
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;

	// Install LoadMenuA hook
	*(DWORD*)(0x4EFDCC) = (DWORD)Hook_LoadMenuA;
	*(DWORD*)(0x4EFE58) = (DWORD)Hook_EnableMenuItem;
	*(DWORD*)(0x4EFC64) = (DWORD)Hook_DialogBoxParamA;

	// Install ShowWindow hook
	*(DWORD*)(0x4EFE70) = (DWORD)Hook_ShowWindow;

	// Only install this hook if SMK is enabled.
	if (smk_enabled) {
		// Install Smacker function hooks
		*(DWORD*)(0x4EFF00) = (DWORD)Hook_SmackOpen;
	}

	// Fix the sign fonts
	VirtualProtect((LPVOID)0x4E7267, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4E7267 = 'a';
	VirtualProtect((LPVOID)0x44DC42, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC42 = 5;
	VirtualProtect((LPVOID)0x44DC4F, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44DC4F = 10;

	// Fix power and water grid updates slowing down after the population hits 50,000
	VirtualProtect((LPVOID)0x440943, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440943 = 50000000;
	VirtualProtect((LPVOID)0x440987, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440987 = 50000000;
	VirtualProtect((LPVOID)0x43F429, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F429 = 50000000;
	VirtualProtect((LPVOID)0x43F3A4, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F3A4 = 50000000;
	
	// Fix city name being overwritten by filename on save
	BYTE bFilenamePatch[6] = { 0xB9, 0xA0, 0xA1, 0x4C, 0x00, 0x51 };
	VirtualProtect((LPVOID)0x42FE62, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE62, bFilenamePatch, 6);
	VirtualProtect((LPVOID)0x42FE99, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE99, bFilenamePatch, 6);

	// Fix save filenames going wonky 
	VirtualProtect((LPVOID)0x4321B9, 8, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4321B9, 0x90, 8);

	// Fix $1500 neighbor connections on game load
	VirtualProtect((LPVOID)0x434BEA, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x434BEA, Hook_LoadNeighborConnections1500);
	*(BYTE*)0x434BEF = 0x90;

	// Fix military bases not growing
	VirtualProtect((LPVOID)0x440D4F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJZ((LPVOID)0x440D4F, Hook_FixMilitaryBaseGrowth);

	// Make multiple attempts at building a military base before giving up
	VirtualProtect((LPVOID)0x4146B5, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJNZ((LPVOID)0x4146B5, Hook_AttemptMultipleMilitaryBases);
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);

	// Store the coordinates of the military base
	VirtualProtect((LPVOID)0x41442E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x41442E, Hook_41442E);

	// Move the alt+query bottom text to not be blocked by the OK button
	VirtualProtect((LPVOID)0x428FB1, 3, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x428FB1 = 0x83;
	*(BYTE*)0x428FB2 = 0xE8;
	*(BYTE*)0x428FB3 = 0x32;
	
	// Install the advanced query hook
	InstallQueryHooks();

	// Fix the broken cheat
	UINT uCheatPatch[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	memcpy_s((LPVOID)0x4E65C8, 10, "mrsoleary", 10);
	memcpy_s((LPVOID)0x4E6490, sizeof(uCheatPatch), uCheatPatch, sizeof(uCheatPatch));

	// Increase sound buffer sizes to 256K each
	VirtualProtect((LPVOID)0x480C2B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C2B = 262144;
	VirtualProtect((LPVOID)0x480C4B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C4B = 262144;
	VirtualProtect((LPVOID)0x480C5B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C5B = 262144;
	VirtualProtect((LPVOID)0x480C6B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C6B = 262144;
	VirtualProtect((LPVOID)0x480C7B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x480C7B = 262144;

	// Load higher quality sounds from DLL resources
	LoadReplacementSounds();

	// Hook sound buffer loading
	VirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401F9B, Hook_LoadSoundBuffer);

	// Restore additional music
	VirtualProtect((LPVOID)0x401A9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401A9B, Hook_MusicPlayNextRefocusSong);

	// Shuffle music if the shuffle setting is enabled
	MusicShufflePlaylist(0);

	// Replace music functions with ones to post messages to the music thread
	if (bSettingsUseMultithreadedMusic) {
		VirtualProtect((LPVOID)0x402414, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402414, Hook_MusicPlay);
		VirtualProtect((LPVOID)0x402BE4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402BE4, Hook_MusicStop);
		VirtualProtect((LPVOID)0x4D2BFC, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(DWORD*)0x4D2BFC = (DWORD)MusicMCINotifyCallback;
	}

	// Load weather icons
	for (int i = 0; i < 13; i++) {
		HANDLE hBitmap = LoadImage(hSC2KFixModule, MAKEINTRESOURCE(IDB_WEATHER0 + i), IMAGE_BITMAP, 40, 40, NULL);
		if (hBitmap)
			hWeatherBitmaps[i] = hBitmap;
		else
			ConsoleLog(LOG_ERROR, "MISC: Couldn't load weather bitmap IDB_WEATHER%i: 0x%08X\n", i, GetLastError());
	}

	// Hook status bar updates for the status dialog implementation
	VirtualProtect((LPVOID)0x402793, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402793, Hook_402793);
	VirtualProtect((LPVOID)0x4021A8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4021A8, Hook_4021A8);
	VirtualProtect((LPVOID)0x40103C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40103C, Hook_40103C);

	// Window title calendar
	VirtualProtect((LPVOID)0x40D67D, 10, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x40D67D, Hook_40D67D);
	memset((LPVOID)0x40D682, 0x90, 5);
	VirtualProtect((LPVOID)0x4135D2, 9, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4135D2, Hook_4315D2);
	memset((LPVOID)0x4135D7, 0x90, 4);

	// Hook SimulationStartDisaster
	VirtualProtect((LPVOID)0x402527, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402527, Hook_SimulationStartDisaster);

	// Hook SimulationPrepareDisaster
	VirtualProtect((LPVOID)0x40174E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40174E, Hook_SimulationPrepareDisaster);

	// Add settings buttons to SC2K's menus
	hGameMenu = LoadMenu(hSC2KAppModule, MAKEINTRESOURCE(3));
	if (hGameMenu) {
		HMENU hOptionsPopup;
		MENUITEMINFO miiOptionsPopup;
		miiOptionsPopup.cbSize = sizeof(MENUITEMINFO);
		miiOptionsPopup.fMask = MIIM_SUBMENU;
		if (!GetMenuItemInfo(hGameMenu, 2, TRUE, &miiOptionsPopup) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: GetMenuItemInfo failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		hOptionsPopup = miiOptionsPopup.hSubMenu;
		if (!AppendMenu(hOptionsPopup, MF_SEPARATOR, NULL, NULL) && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #1 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		if (!AppendMenu(hOptionsPopup, MF_STRING, 40000, "sc2kfix &Settings...") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #2 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}
		if (!AppendMenu(hOptionsPopup, MF_STRING, 40001, "Show Status Dialog") && mischook_debug & MISCHOOK_DEBUG_MENU) {
			ConsoleLog(LOG_DEBUG, "MISC: AppendMenuA #3 failed, error = 0x%08X.\n", GetLastError());
			goto skipmenu;
		}

		AFX_MSGMAP_ENTRY afxMessageMapEntry = {
			WM_COMMAND,
			0,
			40000,
			40000,
			0x0A,
			ShowSettingsDialog
		};
		VirtualProtect((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy_s((LPVOID)0x4D45C0, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));

		afxMessageMapEntry = {
			WM_COMMAND,
			0,
			40001,
			40001,
			0x0A,
			ShowStatusDialog
		};
		VirtualProtect((LPVOID)0x4D45D8, sizeof(afxMessageMapEntry), PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy_s((LPVOID)0x4D45D8, sizeof(afxMessageMapEntry), &afxMessageMapEntry, sizeof(afxMessageMapEntry));
		if (mischook_debug & MISCHOOK_DEBUG_MENU)
			ConsoleLog(LOG_DEBUG, "MISC: Updated game menu.\n");
	}

	// Copy the main menu's message map and update the runtime class to use it
	VirtualProtect((LPVOID)0x4D513C, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy_s(afxMessageMapMainMenu, sizeof(afxMessageMapMainMenu), (LPVOID)0x4D5140, sizeof(AFX_MSGMAP_ENTRY) * 8);
	afxMessageMapMainMenu[7] = { WM_COMMAND, 0, 118, 118, 0x0A, ShowSettingsDialog };
	afxMessageMapMainMenu[8] = { 0 };
	*(DWORD*)0x4D513C = (DWORD)afxMessageMapMainMenu;

	// Skip over the strange bit of code that re-arranges the original main menu.
	// 
	// For some reason the main menu dialog resource in simcity.exe has the Load Tile Set button
	// at the exact same coordinates as the Quit button. The code we're skipping (because we're
	// using our own dialog resource for the main menu) programatically resizes the dialog and
	// rearranges the buttons to fit on it. Why they didn't just fix the button coordinates in
	// the dialog resource instead is beyond me.
	VirtualProtect((LPVOID)0x41503F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP(0x41503F, 0x415161);
	*(BYTE*)0x415044 = 0x90;

skipmenu:
	// Part two!
	UpdateMiscHooks();
}

// The difference between InstallMiscHooks and UpdateMiscHooks is that UpdateMiscHooks can be run
// again at runtime because it can patch back in original game code. It's used for small stuff.
void UpdateMiscHooks(void) {
	// Music in background
	VirtualProtect((LPVOID)0x40BFDA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	if (bSettingsMusicInBackground)
		memset((LPVOID)0x40BFDA, 0x90, 5);
	else {
		BYTE bOriginalCode[5] = { 0xE8, 0xFD, 0x50, 0xFF, 0xFF };
		memcpy_s((LPVOID)0x40BFDA, 5, bOriginalCode, 5);
	}
}

