#ifndef __SMK_H__
#define __SMK_H__

typedef DWORD(__cdecl *SMKOpenPtr) (LPCSTR lpFileName, uint32_t uFlags, int32_t iExBuf);

extern SMKOpenPtr SMKOpenProc;

#endif
