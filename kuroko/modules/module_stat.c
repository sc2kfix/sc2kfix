#include <sys/stat.h>
#include <kuroko/vm.h>
#include <kuroko/util.h>

KRK_Function(S_ISBLK) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL((mode & S_IFMT) == 0);
}
KRK_Function(S_ISCHR) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL((mode & S_IFMT) == S_IFCHR);
}
KRK_Function(S_ISDIR) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL((mode & S_IFMT) == S_IFDIR);
}
KRK_Function(S_ISFIFO) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL((mode & S_IFMT) == _S_IFIFO);
}
KRK_Function(S_ISREG) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL((mode & S_IFMT) == S_IFCHR);
}
#ifndef _WIN32
KRK_Function(S_ISLNK) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL(S_ISLNK(mode));
}
KRK_Function(S_ISSOCK) {
	int mode;
	if (!krk_parseArgs("i",(const char*[]){"mode"},&mode)) return NONE_VAL();
	return INTEGER_VAL(S_ISSOCK(mode));
}
#endif

KRK_Module(stat) {
	KRK_DOC(module, "@brief Functions to check results from @ref stat calls.");

	BIND_FUNC(module,S_ISBLK);
	BIND_FUNC(module,S_ISCHR);
	BIND_FUNC(module,S_ISDIR);
	BIND_FUNC(module,S_ISFIFO);
	BIND_FUNC(module,S_ISREG);
#ifndef _WIN32
	BIND_FUNC(module,S_ISLNK);
	BIND_FUNC(module,S_ISSOCK);
#endif
}


