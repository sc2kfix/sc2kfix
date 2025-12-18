// sc2kfix include/scurk_1996.h: defines specific to the 1996 (Network Edition) version of WinSCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the 1996 (Network Edition) version of WinSCURK - highly experimental.
// Any corrections for WinSCURK are very much "best effort" - good luck.
//
// This version has been confirmed to have been used with the following games:
// 1) SimCity 2000 Network Edition

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMEOFF(type, name, address) \
	type* __ptr__##name##_SCURK1996 = (type*)address; \
	type& ##name##_SCURK1996 = *__ptr__##name##_SCURK1996;
#define SCURK1996_GAMEOFF_ARR(type, name, address) type* ##name##_SCURK1996 = (type*)address;
#else
#define SCURK1996_GAMEOFF(type, name, address) extern type& ##name##_SCURK1996;
#define SCURK1996_GAMEOFF_ARR(type, name, address) extern type* ##name##_SCURK1996;
#endif

#define SCURK1996_GAMEOFF_PTR SCURK1996_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996)(__VA_ARGS__); \
	GameFuncPtr_##name##_SCURK1996 Game_##name##_SCURK1996 = (GameFuncPtr_##name##_SCURK1996)address;
#else
#define SCURK1996_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_SCURK1996 Game_##name##_SCURK1996;
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_SCURK1996 GameMain_##name##_SCURK1996 = (GameMainFuncPtr_##name##_SCURK1996)address;
#else
#define SCURK1996_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_SCURK1996 GameMain_##name##_SCURK1996;
#endif
