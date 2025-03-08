#ifndef __SMK_H__
#define __SMK_H__

typedef DWORD(__cdecl *SMKOpenPtr) (LPCSTR lpFileName, DWORD a1, DWORD a2);

extern SMKOpenPtr SMKOpenProc;

#endif
