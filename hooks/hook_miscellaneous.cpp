// sc2kfix hooks/hook_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 github.com/araxestroy - released under the MIT license

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

#pragma intrinsic(_ReturnAddress)

#define MISCHOOK_DEBUG_OTHER 1
#define MISCHOOK_DEBUG_MILITARY 2
#define MISCHOOK_DEBUG_MENU 4

#define MISCHOOK_DEBUG 0

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

UINT iMilitaryBaseTries = 0;
WORD wMilitaryBaseX = 0, wMilitaryBaseY = 0;

AFX_MSGMAP_ENTRY afxMessageMapMainMenu[9];

// Override some strings that have egregiously bad grammar/capitalization.
// Maxis fail English? That's unpossible!
extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	if (hInstance == hSC2KAppModule && bSettingsUseNewStrings) {
		switch (uID) {
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

// Load our own version of the main menu
extern "C" INT_PTR __stdcall Hook_DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	if (hInstance == hSC2KAppModule && (DWORD)lpTemplateName == 103)
		return DialogBoxParamA(hSC2KFixModule, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
	return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
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

// Hooks save filename determiner
extern "C" void __cdecl Hook_432870(LPSTR* szFilename, char* szExtension) {
	// XXX - do we need to do anything here or can this entire call be turned into a NOP chain?
	return;
}

void InstallMiscHooks(void) {
	// Install LoadStringA hook
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;

	// Install LoadMenuA hook
	*(DWORD*)(0x4EFDCC) = (DWORD)Hook_LoadMenuA;
	*(DWORD*)(0x4EFE58) = (DWORD)Hook_EnableMenuItem;
	*(DWORD*)(0x4EFC64) = (DWORD)Hook_DialogBoxParamA;

	// Savegame hook test
	VirtualProtect((LPVOID)0x4321B9, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x4321B9, Hook_432870);

	// Fix power and water grid updates slowing down after the population hits 50,000
	VirtualProtect((LPVOID)0x440943, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440943 = 50000000;
	VirtualProtect((LPVOID)0x440987, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x440987 = 50000000;
	VirtualProtect((LPVOID)0x43F428, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F428 = 50000000;
	VirtualProtect((LPVOID)0x43F3A3, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x43F3A3 = 50000000;
	
	// Fix city name being overwritten by filename on save
	VirtualProtect((LPVOID)0x42FE6C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x42FE6C, 0x90, 5);
	VirtualProtect((LPVOID)0x42FEA3, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x42FEA3, 0x90, 5);

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

	// Allow military tiles to be rotated. I don't know if this will work or not.
	// 
	// (no. it does not work. leaving this here for now as a potential to-to list item.)
	/*VirtualProtect((LPVOID)0x43931E, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x43931E = 0x7F;
	VirtualProtect((LPVOID)0x4393B5, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4393B5 = 0x7F;*/

	// Move the alt+query bottom text to not be blocked by the OK button
	VirtualProtect((LPVOID)0x428FB1, 3, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x428FB1 = 0x83;
	*(BYTE*)0x428FB2 = 0xE8;
	*(BYTE*)0x428FB3 = 0x32;

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

