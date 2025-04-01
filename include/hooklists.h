// sc2kfix include/hooklists.h: all the lists used for native code hooks
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#include <list>
#include <sc2kfix.h>

extern std::list<hook_function_t> stHooks_Hook_SimulationProcessTickDaySwitch_Before;