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
	krk_interpret("import kuroko\n", "<dllmain>");

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
		KRK_DOC(module, "Functions for interacting with sc2kfix.");

		krk_attachNamedValue(&module->fields, "LOG_NONE", INTEGER_VAL(LOG_NONE));
		krk_attachNamedValue(&module->fields, "LOG_EMERGENCY", INTEGER_VAL(LOG_EMERGENCY));
		krk_attachNamedValue(&module->fields, "LOG_ALERT", INTEGER_VAL(LOG_ALERT));
		krk_attachNamedValue(&module->fields, "LOG_CRITICAL", INTEGER_VAL(LOG_CRITICAL));
		krk_attachNamedValue(&module->fields, "LOG_ERROR", INTEGER_VAL(LOG_ERROR));
		krk_attachNamedValue(&module->fields, "LOG_WARNING", INTEGER_VAL(LOG_WARNING));
		krk_attachNamedValue(&module->fields, "LOG_NOTICE", INTEGER_VAL(LOG_NOTICE));
		krk_attachNamedValue(&module->fields, "LOG_INFO", INTEGER_VAL(LOG_INFO));
		krk_attachNamedValue(&module->fields, "LOG_DEBUG", INTEGER_VAL(LOG_DEBUG));

		BIND_FUNC(module, version);
		BIND_FUNC(module, console_log);
	}

	
}