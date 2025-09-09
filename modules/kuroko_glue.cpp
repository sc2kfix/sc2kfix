// sc2kfix modules/kuroko_glue.cpp: Kuroko VM main thread, glue libraries, and associated functions
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <io.h>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#include <kuroko/kuroko.h>
#include <kuroko/util.h>

#ifdef KRK_BUNDLE_LIBS
#define BUNDLED(name) extern "C" KrkValue krk_module_onload_ ## name (KrkString*);
KRK_BUNDLE_LIBS
#undef BUNDLED
#endif

#define KRK_GAMEOFF(x) krk_attachNamedValue(&objState->fields, #x, INTEGER_VAL(&x))
#define KRK_GAMEOFF_PTR(x) krk_attachNamedValue(&objState->fields, #x, INTEGER_VAL(x))
#define KRK_ENUM_PAIR(x) #x, INTEGER_VAL(x)

extern "C" int EnterKurokoREPL(void);
extern "C" int EnterKurokoFile(const char* szFilename);

DWORD dwKurokoThreadID;

DWORD WINAPI KurokoThread(LPVOID lpParameter) {
	MSG msg;

	krk_initVM(0);

#ifdef KRK_BUNDLE_LIBS
#define BUNDLED(name) do { \
	KrkValue moduleOut = krk_module_onload_ ## name (NULL); \
	krk_attachNamedValue(&vm.modules, # name, moduleOut); \
	krk_attachNamedObject(&AS_INSTANCE(moduleOut)->fields, "__name__", (KrkObj*)krk_copyString(#name, sizeof(#name)-1)); \
	krk_attachNamedValue(&AS_INSTANCE(moduleOut)->fields, "__file__", NONE_VAL()); \
} while (0)
	KRK_BUNDLE_LIBS
#undef BUNDLED
#endif
	
	krk_startModule("__main__");
	krk_interpret("import kuroko\nimport sc2k\nimport sc2kfix", "<dllmain>");

	bKurokoVMInitialized = TRUE;
	ConsoleLog(LOG_INFO, "CORE: Kuroko VM initialized.\n");

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_KUROKO_REPL && bConsoleEnabled) {
			EnterKurokoREPL();
			PostThreadMessage(dwConsoleThreadID, WM_CONSOLE_REPL, NULL, NULL);
		} else if (msg.message == WM_KUROKO_FILE && bConsoleEnabled) {
			// NOTE: wParam *must* be a filename/path allocated with malloc!
			// Passing anything else will almost certainly cause a crash at some point.

			if (!msg.wParam) {
				ConsoleLog(LOG_WARNING, "CORE: Kuroko root thread received WM_KUROKO_FILE with wParam == NULL.\n");
				continue;
			}
			EnterKurokoFile((const char*)msg.wParam);
			PostThreadMessage(dwConsoleThreadID, WM_CONSOLE_REPL, NULL, NULL);
		}
		else if (msg.message == WM_QUIT)
			break;
	}

	krk_freeVM();
	ConsoleLog(LOG_INFO, "CORE: Shutting down Kuroko thread.\n");

	return EXIT_SUCCESS;
}

// Kuroko extension library

extern "C" {
	KRK_Function(version) {
		FUNCTION_TAKES_NONE();
		return OBJECT_VAL((KrkObj*)S(SC2KFIX_VERSION));
	}

	KRK_Function(read_dword) {
		FUNCTION_TAKES_EXACTLY(1);
		CHECK_ARG(0, int, krk_integer_type, address);

		__try {
			DWORD* dwVal = (DWORD*)address;
			return INTEGER_VAL(*dwVal);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.read_dword().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	KRK_Function(read_word) {
		FUNCTION_TAKES_EXACTLY(1);
		CHECK_ARG(0, int, krk_integer_type, address);

		__try {
			WORD* wVal = (WORD*)address;
			return INTEGER_VAL(*wVal);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.read_word().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	KRK_Function(read_byte) {
		FUNCTION_TAKES_EXACTLY(1);
		CHECK_ARG(0, int, krk_integer_type, address);

		__try {
			BYTE* bVal = (BYTE*)address;
			return INTEGER_VAL(*bVal);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.read_byte().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	KRK_Function(write_dword) {
		FUNCTION_TAKES_EXACTLY(2);
		CHECK_ARG(0, int, krk_integer_type, address);
		CHECK_ARG(1, int, krk_integer_type, data);

		__try {
			DWORD olddata = *(DWORD*)address;
			*(DWORD*)address = (DWORD)(data & 0xFFFFFFFF);
			return INTEGER_VAL(olddata);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.write_dword().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	KRK_Function(write_word) {
		FUNCTION_TAKES_EXACTLY(2);
		CHECK_ARG(0, int, krk_integer_type, address);
		CHECK_ARG(1, int, krk_integer_type, data);

		__try {
			WORD olddata = *(WORD*)address;
			*(WORD*)address = (WORD)(data & 0xFFFFFFFF);
			return INTEGER_VAL(olddata);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.write_word().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	KRK_Function(write_byte) {
		FUNCTION_TAKES_EXACTLY(2);
		CHECK_ARG(0, int, krk_integer_type, address);
		CHECK_ARG(1, int, krk_integer_type, data);

		__try {
			BYTE olddata = *(BYTE*)address;
			*(BYTE*)address = (BYTE)(data & 0xFFFFFFFF);
			return INTEGER_VAL(olddata);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ConsoleLog(LOG_ERROR, "MODS: Segmentation fault caught in sc2k.write_byte().\n");
			return krk_runtimeError(vm.exceptions->indexError, "Caught segmentation fault.");
		}
	}

	// Based on the internal print function.
	KRK_Function(console_log) {
		FUNCTION_TAKES_AT_LEAST(1);
		const char* sep = " ";
		const char* end = "\n";
		size_t sepLen = 1;
		size_t endLen = 1;

		CHECK_ARG(0, int, krk_integer_type, iLogLevel);

		char* szBuf = (char*)malloc(1024);
		if (!szBuf)
			return NONE_VAL();

		memset(szBuf, 0, 1024);

		for (int i = 1; i < argc; ++i) {
			/* Convert the argument to a printable form, first by trying __str__, then __repr__ */
			KrkValue printable = argv[i];
			krk_push(printable);
			if (!IS_STRING(printable)) {
				KrkClass* type = krk_getType(printable);
				if (type->_tostr) {
					krk_push((printable = krk_callDirect(type->_tostr, 1)));
				}
				else if (type->_reprer) {
					krk_push((printable = krk_callDirect(type->_reprer, 1)));
				}
				if (!IS_STRING(printable)) return krk_runtimeError(vm.exceptions->typeError, "__str__ returned non-string (type %T)", printable);
			}

			strcat_s(szBuf, 1024, AS_CSTRING(printable));
			krk_pop();
			if (i + 1 < argc) strcat_s(szBuf, 1024, sep);
		}

		strcat_s(szBuf, 1024, end);
		ConsoleLog(iLogLevel, "MODS: %s", szBuf);
		return NONE_VAL();
	}

	KRK_Module(sc2kfix) {
		krk_attachNamedObject(&module->fields, "__doc__", (KrkObj*)S("Functions for interacting with sc2kfix and the modding framework."));

		krk_attachNamedValue(&module->fields, "LOG_NONE", INTEGER_VAL(LOG_NONE));
		krk_attachNamedValue(&module->fields, "LOG_EMERGENCY", INTEGER_VAL(LOG_EMERGENCY));
		krk_attachNamedValue(&module->fields, "LOG_ALERT", INTEGER_VAL(LOG_ALERT));
		krk_attachNamedValue(&module->fields, "LOG_CRITICAL", INTEGER_VAL(LOG_CRITICAL));
		krk_attachNamedValue(&module->fields, "LOG_ERROR", INTEGER_VAL(LOG_ERROR));
		krk_attachNamedValue(&module->fields, "LOG_WARNING", INTEGER_VAL(LOG_WARNING));
		krk_attachNamedValue(&module->fields, "LOG_NOTICE", INTEGER_VAL(LOG_NOTICE));
		krk_attachNamedValue(&module->fields, "LOG_INFO", INTEGER_VAL(LOG_INFO));
		krk_attachNamedValue(&module->fields, "LOG_DEBUG", INTEGER_VAL(LOG_DEBUG));

		BIND_FUNC(module, version)->doc = "Returns the sc2kfix version as a string.";
		BIND_FUNC(module, console_log)->doc = "Provides access to the sc2kfix ConsoleLog function.";
	}

	KRK_Function(GameGetRootWindowHandle) {
		FUNCTION_TAKES_EXACTLY(0);
		return INTEGER_VAL(GameGetRootWindowHandle());
	}

	KRK_Function(SoundPlaySound) {
		FUNCTION_TAKES_EXACTLY(2);
		CHECK_ARG(0, int, krk_integer_type, pThis);
		CHECK_ARG(1, int, krk_integer_type, iSoundID);

		return INTEGER_VAL(Game_SimcityApp_SoundPlaySound((CSimcityAppPrimary*)pThis, iSoundID));
	}

	KRK_Module(sc2k) {
		krk_attachNamedObject(&module->fields, "__doc__", (KrkObj*)S("Functions and classes for interacting with the SimCity 2000 game state."));

		BIND_FUNC(module, read_dword)->doc = "Reads a DWORD from a specified address and returns it as an integer.";
		BIND_FUNC(module, read_word)->doc = "Reads a WORD from a specified address and returns it as an integer.";
		BIND_FUNC(module, read_byte)->doc = "Reads a BYTE from a specified address and returns it as an integer.";
		BIND_FUNC(module, write_dword)->doc = "Writes a DWORD to a specified address and returns the previous value as an integer.";
		BIND_FUNC(module, write_word)->doc = "Writes a WORD to a specified address and returns the previous value as an integer.";
		BIND_FUNC(module, write_byte)->doc = "Writes a BYTE to a specified address and returns the previous value as an integer.";

		KrkInstance* objCalls = krk_newInstance(KRK_BASE_CLASS(object));
		krk_attachNamedObject(&objCalls->fields, "__doc__", (KrkObj*)S("Functions in the SimCity 2000 engine callable from Kuroko."));
		krk_attachNamedObject(&module->fields, "calls", (KrkObj*)objCalls);

		BIND_FUNC(objCalls, GameGetRootWindowHandle)->doc = "HWND GameGetRootWindowHandle(void)";
		BIND_FUNC(objCalls, SoundPlaySound)->doc = "int SoundPlaySound(void* this, int iSoundID)";

		KrkInstance* objState = krk_newInstance(KRK_BASE_CLASS(object));
		krk_attachNamedObject(&objState->fields, "__doc__", (KrkObj*)S("Pointers to SimCity 2000 game state objects."));
		krk_attachNamedObject(&module->fields, "state", (KrkObj*)objState);

		KRK_GAMEOFF(pCSimcityAppThis);
		KRK_GAMEOFF(wCurrentTileCoordinates);
		KRK_GAMEOFF(wTileCoordinateX);
		KRK_GAMEOFF(wTileCoordinateY);
		KRK_GAMEOFF(wGameScreenAreaX);
		KRK_GAMEOFF(wGameScreenAreaY);
		KRK_GAMEOFF(wMaybeActiveToolGroup);
		KRK_GAMEOFF(wViewRotation);
		KRK_GAMEOFF(bCityHasOcean);
		KRK_GAMEOFF(dwArcologyPopulation);
		KRK_GAMEOFF(dwCityResidentialPopulation);
		KRK_GAMEOFF(pszCityName);
		KRK_GAMEOFF(wNationalEconomyTrend);
		KRK_GAMEOFF(wCurrentMapToolGroup);
		KRK_GAMEOFF(wCityNeighborConnections1500);
		KRK_GAMEOFF(wSubwayXUNDCount);
		KRK_GAMEOFF(wSetTriggerDisasterType);
		KRK_GAMEOFF(wCityMode);
		KRK_GAMEOFF(dwCityLandValue);
		KRK_GAMEOFF(dwCityFunds);
		KRK_GAMEOFF_PTR(dwTileCount);
		KRK_GAMEOFF(dwCityValue);
		KRK_GAMEOFF(dwCityGarbage);
		KRK_GAMEOFF(wCityStartYear);
		KRK_GAMEOFF(dwCityUnemployment);
		KRK_GAMEOFF_PTR(dwNeighborValue);
		KRK_GAMEOFF(wWaterLevel);
		KRK_GAMEOFF(wMonsterXTHGIndex);
		KRK_GAMEOFF(dwNationalPopulation);
		KRK_GAMEOFF_PTR(dwNeighborFame);
		KRK_GAMEOFF_PTR(dwMilitaryTiles);
		KRK_GAMEOFF(wNationalTax);
		KRK_GAMEOFF(wCurrentDisasterID);
		KRK_GAMEOFF(dwCityOrdinances);
		KRK_GAMEOFF(dwPowerUsedPercentage);
		KRK_GAMEOFF(dwCityPopulation);
		KRK_GAMEOFF_PTR(dwNeighborPopulation);
		KRK_GAMEOFF(dwCityFame);
		KRK_GAMEOFF(bYearEndFlag);
		KRK_GAMEOFF(wScreenPointX);
		KRK_GAMEOFF(wScreenPointY);
		KRK_GAMEOFF(bInScenario);
		KRK_GAMEOFF_PTR(szNeighborNameSouth);
		KRK_GAMEOFF_PTR(szNeighborNameWest);
		KRK_GAMEOFF_PTR(szNeighborNameNorth);
		KRK_GAMEOFF_PTR(szNeighborNameEast);
		KRK_GAMEOFF(bWeatherHeat);
		KRK_GAMEOFF(dwCityDays);
		KRK_GAMEOFF(bWeatherWind);
		KRK_GAMEOFF(wCityProgression);
		KRK_GAMEOFF(dwNationalValue);
		KRK_GAMEOFF(dwCityAdvertising);
		KRK_GAMEOFF(wCityCurrentMonth);
		KRK_GAMEOFF(wCityElapsedYears);
		KRK_GAMEOFF_PTR(pArrSpriteHeaders);
		KRK_GAMEOFF(bNewspaperSubscription);
		KRK_GAMEOFF(bWeatherHumidity);
		KRK_GAMEOFF(wCityCurrentSeason);
		KRK_GAMEOFF(pMicrosimArr);
		KRK_GAMEOFF(bCityHasRiver);
		KRK_GAMEOFF(wCityDifficulty);
		KRK_GAMEOFF(bWeatherTrend);
		KRK_GAMEOFF(dwCityWorkforceLE);
		KRK_GAMEOFF_PTR(wCityInventionYears);
		KRK_GAMEOFF(dwCityCrime);
		KRK_GAMEOFF(wCityCenterX);
		KRK_GAMEOFF(wCityCenterY);
		KRK_GAMEOFF(dwCityWorkforcePercent);
		KRK_GAMEOFF(wCurrentCityToolGroup);
		KRK_GAMEOFF(dwCityWorkforceEQ);
		KRK_GAMEOFF(dwWaterUsedPercentage);
		KRK_GAMEOFF(bNewspaperExtra);
		KRK_GAMEOFF_PTR(pBudgetArr);
		KRK_GAMEOFF(bNoDisasters);
		KRK_GAMEOFF_PTR(wNeighborNameIdx);
		KRK_GAMEOFF(wCityNeighborConnections1000);
		KRK_GAMEOFF(bMilitaryBaseType);
		KRK_GAMEOFF(dwCityBonds);
		KRK_GAMEOFF(dwCityTrafficUnknown);
		KRK_GAMEOFF_PTR(wCityDemand);
		KRK_GAMEOFF(dwCityPollution);
		KRK_GAMEOFF(dwLFSRState);
		KRK_GAMEOFF(dwLCGState);
		KRK_GAMEOFF(game_AfxCoreState);
		KRK_GAMEOFF(dwSimulationSubtickCounter);
		KRK_GAMEOFF(pCDocumentMainWindow);
		KRK_GAMEOFF(wPreviousTileCoordinateX);
		KRK_GAMEOFF(wPreviousTileCoordinateY);
		KRK_GAMEOFF(pCSimcityView);
		KRK_GAMEOFF_PTR(dwCityProgressionRequirements);
		KRK_GAMEOFF(dwNextRefocusSongID);
		KRK_GAMEOFF_PTR(dwZoneNameStringIDs);
		KRK_GAMEOFF_PTR(dwCityNoticeStringIDs);
		KRK_GAMEOFF(dwCityRewardsUnlocked);

		KRK_GAMEOFF_PTR(dwMapXTER);
		KRK_GAMEOFF_PTR(dwMapXZON);
		KRK_GAMEOFF_PTR(dwMapXTXT);
		KRK_GAMEOFF_PTR(dwMapXBIT);
		KRK_GAMEOFF_PTR(dwMapALTM);
		KRK_GAMEOFF_PTR(dwMapXUND);
		KRK_GAMEOFF_PTR(dwMapXBLD);
		KRK_GAMEOFF_PTR(dwMapXCRM);
		KRK_GAMEOFF_PTR(dwMapXPLT);
		KRK_GAMEOFF_PTR(dwMapXTRF);
		KRK_GAMEOFF_PTR(dwMapXVAL);
		KRK_GAMEOFF_PTR(dwMapXPLC);
		KRK_GAMEOFF_PTR(dwMapXPOP);
		KRK_GAMEOFF_PTR(dwMapXFIR);
		KRK_GAMEOFF_PTR(dwMapXROG);
		KRK_GAMEOFF_PTR(dwMapXLAB);
		KRK_GAMEOFF_PTR(dwMapXTHG);
		KRK_GAMEOFF_PTR(dwMapXGRP);

		// Sounds enum
		KrkInstance* objSounds = krk_newInstance(KRK_BASE_CLASS(object));
		krk_attachNamedObject(&objSounds->fields, "__doc__", (KrkObj*)S("Constants for sound IDs."));
		krk_attachNamedObject(&module->fields, "sounds", (KrkObj*)objSounds);
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_BUILD));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_ERROR));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_WIND));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_PLOP));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_EXPLODE));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_CLICK));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_POLICE));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_FIRE));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_BULLDOZER));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_FIRETRUCK));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_SIMCOPTER));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_FLOOD));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_BOOS));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_CHEERS));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_ZAP));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_MAYDAY));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_IMHIT));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_SHIP));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_TAKEOFF));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_LAND));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_SIREN));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_HORNS));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_PRISON));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_SCHOOL));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_TRAIN));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_MILITARY));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_ARCO));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_MONSTER));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_BULLDOZER2));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_RETICULATINGSPLINES));
		krk_attachNamedValue(&objSounds->fields, KRK_ENUM_PAIR(SOUND_SILENT));

		// Things enum
		KrkInstance* objThings = krk_newInstance(KRK_BASE_CLASS(object));
		krk_attachNamedObject(&objThings->fields, "__doc__", (KrkObj*)S("Constants for Thing IDs."));
		krk_attachNamedObject(&module->fields, "things", (KrkObj*)objThings);
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_NONE));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_AIRPLANE));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_HELICOPTER));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_CARGO_SHIP));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_BULLDOZER));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_MONSTER));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_EXPLOSION));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_DEPLOY_POLICE));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_DEPLOY_FIRE));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_SAILBOAT));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_TRAIN_ENGINE));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_TRAIN_CAR));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_UNKNOWN_3));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_UNKNOWN_4));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_DEPLOY_MILITARY));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_TORNADO));
		krk_attachNamedValue(&objThings->fields, KRK_ENUM_PAIR(XTHG_MAXIS_MAN));
	}
}