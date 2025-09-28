// sc2kfix include/sc2k_demo.h: defines specific to the Interactive Demo version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the Interactive Demo... AAAA!!

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define DEMO_GAMEOFF(type, name, address) \
	type* __ptr__Demo##name = (type*)address; \
	type& name = *__ptr__Demo##name;
#define DEMO_GAMEOFF_ARR(type, name, address) type* Demo##name = (type*)address;
#else
#define DEMO_GAMEOFF(type, name, address) extern type& Demo##name;
#define DEMO_GAMEOFF_ARR(type, name, address) extern type* Demo##name;
#endif

#define DEMO_GAMEOFF_PTR DEMO_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define DEMO_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *DemoGameFuncPtr_##name)(__VA_ARGS__); \
	DemoGameFuncPtr_##name DemoGame_##name = (DemoGameFuncPtr_##name)address;
#else
#define DEMO_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *DemoGameFuncPtr_##name)(__VA_ARGS__);\
	extern DemoGameFuncPtr_##name DemoGame_##name;
#endif
