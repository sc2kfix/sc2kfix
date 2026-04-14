// sc2kfix modules/lua_glue.cpp: the sticky stuff between sc2kfix and Lua
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include <lua_glue.h>
#include "../resource.h"

#define LUA_DEBUG_LOAD 1

#define LUA_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef LUA_DEBUG
#define LUA_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define BAILOUT(s, ...) do { \
	ConsoleLog(LOG_ERROR, "SAVE: " s, __VA_ARGS__); \
	return 0; \
} while (0)

UINT lua_debug = LUA_DEBUG;

static BOOL bLuaLibLoadedFromDiskNotice = FALSE;

const char* LuaGetModName(lua_State* L) {
	const char* szModName = "unknown";
	if (lua_getglobal(L, "mod_info")) {
		lua_getfield(L, -1, "shortname");
		szModName = lua_tostring(L, -1);
	}
	return szModName;
}

void LuaReport(lua_State* L, int iStatus) {
	if (iStatus) {
		const char* msg = lua_tostring(L, -1);
		if (msg == NULL)
			msg = "unknown error";

		fprintf(stderr, "%s", msg);
		lua_pop(L, 1);
	}
}

int LuaGlueCall_ConsoleLog(lua_State* L) {
	int iLogLevel = luaL_checkinteger(L, 1);
	const char* szFormattedMessage = luaL_checkstring(L, 2);
	ConsoleLog(iLogLevel, "MODS: (%s) %s", LuaGetModName(L), szFormattedMessage);
}

int LuaGlueCall_ReadDword(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	__try {
		lua_pushinteger(L, *(DWORD*)dwAddress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.read_dword(0x%08X) call from Lua mod \"%s\".\n", dwAddress, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

int LuaGlueCall_ReadWord(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	__try {
		lua_pushinteger(L, *(WORD*)dwAddress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.read_word(0x%08X) call from Lua mod \"%s\".\n", dwAddress, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

int LuaGlueCall_ReadByte(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	__try {
		lua_pushinteger(L, *(BYTE*)dwAddress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.read_byte(0x%08X) call from Lua mod \"%s\".\n", dwAddress, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

int LuaGlueCall_WriteDword(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	DWORD dwData = luaL_checkinteger(L, 2) & 0xFFFFFFFF;
	__try {
		lua_pushinteger(L, *(DWORD*)dwAddress);
		*(DWORD*)dwAddress = dwData;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.write_word(0x%08X, 0x%08X) call from Lua mod \"%s\".\n", dwAddress, dwData, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

int LuaGlueCall_WriteWord(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	WORD wData = luaL_checkinteger(L, 2) & 0xFFFF;
	__try {
		lua_pushinteger(L, *(WORD*)dwAddress);
		*(WORD*)dwAddress = wData;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.write_word(0x%08X, 0x%04X) call from Lua mod \"%s\".\n", dwAddress, wData, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

int LuaGlueCall_WriteByte(lua_State* L) {
	DWORD dwAddress = luaL_checkinteger(L, 1);
	BYTE bData = luaL_checkinteger(L, 2) & 0xFF;
	__try {
		lua_pushinteger(L, *(BYTE*)dwAddress);
		*(BYTE*)dwAddress = bData;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ConsoleLog(LOG_ERROR, "CORE: Segmentation fault caught in sc2k.write_byte(0x%08X, 0x%02X) call from Lua mod \"%s\".\n", dwAddress, bData, LuaGetModName(L));
		luaL_error(L, "sc2kfix caught segmentation fault");
	}
	return 1;
}

void LuaGlueSetupState(lua_State* L) {
	HRSRC hResFind;
	HGLOBAL hLuaResource;

	// Load libsc2kfix from a file if it exists, or the built-in copy if not
	if (FileExists("libsc2kfix.lua")) {
		if (!bLuaLibLoadedFromDiskNotice) {
			ConsoleLog(LOG_INFO, "LUA:  Loading libsc2kfix.lua from disk instead of using built-in copy.\n");
			ConsoleLog(LOG_INFO, "LUA:  This message will only be shown once.\n");
			bLuaLibLoadedFromDiskNotice = TRUE;
		}

		if (lua_debug & LUA_DEBUG_LOAD)
			ConsoleLog(LOG_INFO, "LUA:  (%s) Loading libsc2kfix.lua from disk.\n", LuaGetModName(L));

		int status = luaL_dofile(L, "libsc2kfix.lua");
		if (status)
			LuaReport(L, status);
	} else {
		if (lua_debug & LUA_DEBUG_LOAD)
			ConsoleLog(LOG_INFO, "LUA:  (%s) Loading libsc2kfix.lua from memory.\n", LuaGetModName(L));

		hResFind = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_BLOB_LIBSC2KFIX_LUA), "BLOB");
		if (hResFind) {
			hLuaResource = LoadResource(hSC2KFixModule, hResFind);
			if (hLuaResource) {
				size_t nBlobSize = SizeofResource(hSC2KFixModule, hResFind);
				char* szLuaLibrary = (char*)malloc(nBlobSize + 1);

				if (szLuaLibrary) {
					void* ptr = LockResource(hLuaResource);
					if (ptr) {
						memset(szLuaLibrary, 0, nBlobSize + 1);
						memcpy_s(szLuaLibrary, nBlobSize + 1, ptr, nBlobSize);
						FreeResource(hLuaResource);

						int status = luaL_dostring(L, szLuaLibrary);
						if (status)
							LuaReport(L, status);
						free(szLuaLibrary);
						return;
					}
				}
				else
					ConsoleLog(LOG_ERROR, "LUA:  (%s) Couldn't allocate buffer for libsc2kfix.lua. Lua will almost certainly fail to initialize.\n", LuaGetModName(L));
			}
		}
		else
			ConsoleLog(LOG_ERROR, "LUA:  (%s) Couldn't load libsc2kfix.lua from memory. Lua will almost certainly fail to initialize.\n", LuaGetModName(L));
	}

	// Add the log levels to the global scope
	lua_pushinteger(L, LOG_NONE); lua_setglobal(L, "LOG_NONE");
	lua_pushinteger(L, LOG_EMERGENCY); lua_setglobal(L, "LOG_EMERGENCY");
	lua_pushinteger(L, LOG_ALERT); lua_setglobal(L, "LOG_ALERT");
	lua_pushinteger(L, LOG_CRITICAL); lua_setglobal(L, "LOG_CRITICAL");
	lua_pushinteger(L, LOG_ERROR); lua_setglobal(L, "LOG_ERROR");
	lua_pushinteger(L, LOG_WARNING); lua_setglobal(L, "LOG_WARNING");
	lua_pushinteger(L, LOG_NOTICE); lua_setglobal(L, "LOG_NOTICE");
	lua_pushinteger(L, LOG_INFO); lua_setglobal(L, "LOG_INFO");
	lua_pushinteger(L, LOG_DEBUG); lua_setglobal(L, "LOG_DEBUG");

	// Set up the baked-in parts of the sc2kfix and sc2k tables
	// Note: libsc2kfix's table.dump() function won't print anything prefixed with an underscore
	// by default (see libsc2kfix.lua for details).
	lua_getglobal(L, "sc2kfix");
	luaS_setCFuncEntry(L, "__ConsoleLog", LuaGlueCall_ConsoleLog);

	lua_getglobal(L, "sc2k");
	luaS_setCFuncEntry(L, "read_dword", LuaGlueCall_ReadDword);
	luaS_setCFuncEntry(L, "read_word", LuaGlueCall_ReadWord);
	luaS_setCFuncEntry(L, "read_byte", LuaGlueCall_ReadByte);
	luaS_setCFuncEntry(L, "write_dword", LuaGlueCall_WriteDword);
	luaS_setCFuncEntry(L, "write_word", LuaGlueCall_WriteWord);
	luaS_setCFuncEntry(L, "write_byte", LuaGlueCall_WriteByte);

	// Run the user's autoexec.lua file if it exists
	if (FileExists("autoexec.lua")) {
		if (lua_debug & LUA_DEBUG_LOAD)
			ConsoleLog(LOG_INFO, "LUA:  (%s) Found autoexec.lua on disk, running it.\n", LuaGetModName(L));
		int status = luaL_dofile(L, "autoexec.lua");
		if (status)
			LuaReport(L, status);
	}
}