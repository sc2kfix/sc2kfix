// sc2kfix include/hooklists.h: all the lists used for native code hooks
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#include <list>
#include <sc2kfix.h>

extern std::vector<hook_function_t> stHooks_Hook_OnNewCity_Before;
extern std::vector<hook_function_t> stHooks_Hook_LoadGame_Before;
extern std::vector<hook_function_t> stHooks_Hook_LoadGame_After;
extern std::vector<hook_function_t> stHooks_Hook_SaveGame_Before;
extern std::vector<hook_function_t> stHooks_Hook_SaveGame_After;
extern std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_Before;
extern std::vector<hook_function_t> stHooks_Hook_GameDoIdleUpkeep_After;
extern std::vector<hook_function_t> stHooks_Hook_ScenarioSuccessCheck;