// sc2kfix include/hooklists.h: all the lists used for native code hooks
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#include <list>
#include <sc2kfix.h>

extern std::vector<hook_function_t> stHooks_Hook_OnNewCity_Before;
extern std::vector<hook_function_t> stHooks_Hook_SimcityApp_OpenCityData_Before;
extern std::vector<hook_function_t> stHooks_Hook_SimcityApp_OpenCityData_After;
extern std::vector<hook_function_t> stHooks_L_SimcityApp_DoSave_Before;
extern std::vector<hook_function_t> stHooks_L_SimcityApp_DoSave_After;
extern std::vector<hook_function_t> stHooks_Hook_PrepareGame_Before;
extern std::vector<hook_function_t> stHooks_Hook_PrepareGame_After;
extern std::vector<hook_function_t> stHooks_Hook_SimcityApp_BuildSubFrames_Before;
extern std::vector<hook_function_t> stHooks_Hook_SimcityApp_BuildSubFrames_After;
extern std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_Before;
extern std::vector<hook_function_t> stHooks_Hook_SimCalendarDay23_Before;
extern std::vector<hook_function_t> stHooks_Hook_ScenarioSuccessCheck;
extern std::vector<hook_function_t> stHooks_Hook_SimCalendarDay23_After;
extern std::vector<hook_function_t> stHooks_Hook_SimCalendarAdvance_After;
extern std::vector<hook_function_t> stHooks_Hook_SimulationGrowSpecificZone_Success;
