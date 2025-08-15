// sc2kfix smk.h: run-time linking globals for smk.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

typedef DWORD(__cdecl* SMKOpenPtr) (LPCSTR lpFileName, uint32_t uFlags, int32_t iExBuf);

extern BOOL smk_enabled;
extern SMKOpenPtr SMKOpenProc;

void GetSMKFuncs();
void ReleaseSMKFuncs();
extern "C" DWORD __cdecl Hook_MovieCheck(char* sMovStr);
