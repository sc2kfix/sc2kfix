// sc2kfix hooks/hook_animation.cpp: hooks to do with animations across the various
// supported versions of SimCity 2000.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

extern "C" int __cdecl Hook_AnimationFunctionSimCity1996(HPALETTE *hP1, int iToggle) {
	int(__cdecl *H_AnimationFunction1996)(HPALETTE *, int) = (int(__cdecl *)(HPALETTE *, int))0x457110;

	int ret = H_AnimationFunction1996(hP1, iToggle);
	RedrawWindow(GameGetRootWindowHandle(), NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

	return ret;
}

extern "C" int __cdecl Hook_AnimationFunctionSimCity1995(HPALETTE *hP1, int iToggle) {
	void *&pRootWnd1995 = *(void **)0x4C602C;
	HWND hMainWnd = (HWND)((DWORD*)pRootWnd1995)[7];

	int(__cdecl *H_AnimationFunction1995)(HPALETTE *, int) = (int(__cdecl *)(HPALETTE *, int))0x456A60;

	int ret = H_AnimationFunction1995(hP1, iToggle);
	RedrawWindow(hMainWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

	return ret;
}

void InstallAnimationSimCity1996Hooks(void) {
	VirtualProtect((LPVOID)0x4023D3, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4023D3, Hook_AnimationFunctionSimCity1996);
}

void InstallAnimationSimCity1995Hooks(void) {
	VirtualProtect((LPVOID)0x402405, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402405, Hook_AnimationFunctionSimCity1995);
}
