// sc2kfix include/lua_glue.h: the sticky stuff between sc2kfix and Lua
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#include <windows.h>

#include "../thirdparty/lua/lua.hpp"
#include "../thirdparty/lua/llimits.h"

#define luaS_setIntEntry(L, name, i) do { lua_pushinteger(L, i); lua_setfield(L, -2, name); } while (0)
#define luaS_setStringEntry(L, name, s) do { lua_pushstring(L, s); lua_setfield(L, -2, name); } while (0)
#define luaS_setCFuncEntry(L, name, f) do { lua_pushcfunction(L, f); lua_setfield(L, -2, name); } while (0)

extern UINT lua_debug;

void LuaReport(lua_State* L, int iStatus);
const char* LuaGetModName(lua_State * L);
int LuaRunREPL(void);
void LuaGlueSetupState(lua_State* L);