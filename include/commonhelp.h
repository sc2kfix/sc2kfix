#pragma once

// Common structures between supported programs.
// Common wrapped functions.

class cEditableTileSet;

#pragma pack(push, 1)
typedef struct {
	union uSpriteOffset {
		BYTE *sprPtr;
		int32_t sprLong;
	};
	uSpriteOffset sprOffset;
	WORD wHeight;
	WORD wWidth;
} sprite_header_t;

typedef struct {
	__int16 nSprNum;
	sprite_header_t sprHeader;
} sprite_file_header_t;

typedef struct {
	__int16 nSprites;
	sprite_file_header_t pData[1];
} sprite_archive_t;

typedef struct {
	char szTypeHead[4];
	DWORD dwSize;
	char szSC2KHead[4];
} tilesetMainHeader_t;

typedef struct {
	char szHead[4];
	DWORD dwSize;
} tilesetHeadInfo_t;

typedef struct {
	char szHead[4];
	DWORD dwSize;
	char pBuf;
} tileMem_t;

typedef struct {
	WORD nMaxChunks;
	tileMem_t tileMem;
} tilesetMem_t;

typedef struct {
	WORD nSpriteID;
	WORD nWidth;
	WORD nHeight;
	DWORD dwSize;
	char pBuf;
} tileShap_t;

typedef struct {
	WORD nTileNameID;
	WORD nNameLength;
	char pBuf;
} tileName_t;
#pragma pack(pop)

// General
__int16 __cdecl L_FlipShortBytes(__int16 nVal);
int __cdecl L_FlipLongBytePortions(int nVal);

// MFC


// Borland
void *__cdecl L_BOR_gAllocBlock(size_t nSz);
void __cdecl L_BOR_gFreeBlock(void *pBlock);
int __stdcall L_BOR_gUpdateWaitWindow();

// SCURK-only
LONG __cdecl L_SCURK_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, const char *lpPathName);

// SC2K-only
