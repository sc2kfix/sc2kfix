// sc2kfix kuroko/pthread.cpp: Basic implementation of pthreads for Kuroko
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#include <pthread.h>

extern "C" int pthread_rwlock_init(pthread_rwlock_t* rwlock, const void* attr) {
	if (!rwlock)
		return 1;
	InitializeSRWLock(&rwlock->lock);
	rwlock->exclusive = false;
	return 0;
}
extern "C" int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock) {
	if (!rwlock)
		return 1;
	AcquireSRWLockShared(&rwlock->lock);
	return 0;
}
extern "C" int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock) {
	if (!rwlock)
		return 1;
	AcquireSRWLockExclusive(&rwlock->lock);
	rwlock->exclusive = true;
	return 0;
}
extern "C" int pthread_rwlock_unlock(pthread_rwlock_t* rwlock) {
	if (!rwlock)
		return 1;

	if (rwlock->exclusive) {
		rwlock->exclusive = false;
		ReleaseSRWLockExclusive(&rwlock->lock);
	} else
		ReleaseSRWLockShared(&rwlock->lock);
	return 0;
}

extern "C" int pthread_create(pthread_t* thread, const void* attr, void* (*start_routine)(void*), void* arg) {
	*thread = (pthread_t)_beginthread((void(__cdecl*)(void*))start_routine, 1024 * 1024, arg);
	if ((intptr_t)*thread == -1L)
		return -1;
	return 0;
}

extern "C" int pthread_join(pthread_t thread, void** value_ptr) {
	DWORD dwExitCode;
	WaitForSingleObject(thread, INFINITE);
	GetExitCodeThread(thread, &dwExitCode);
	CloseHandle(thread);
	if (!value_ptr)
		return 0;

	DWORD* dwCodeOut = (DWORD*)*value_ptr;
	if (dwCodeOut)
		*dwCodeOut = dwExitCode;
	
	return 0;
}

extern "C" int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr) {
	HANDLE hMutex;
	if (!mutex)
		return -1;

	TCHAR* szMutexName = (TCHAR*)malloc(sizeof(TCHAR) * 9);
#ifdef UNICODE
	wsprintf(szMutexName, L"%08X", (DWORD)mutex);
#else
	sprintf(szMutexName, L"%08X", (DWORD)mutex);
#endif
	mutex->name = szMutexName;

	hMutex = CreateMutex(NULL, FALSE, mutex->name);
	mutex->handle = hMutex;

	if (mutex->handle == NULL) {
		free(szMutexName);
		return 1;
	}
	return 0;
}

extern "C" int pthread_mutex_lock(pthread_mutex_t* mutex) {
	HANDLE hMutex;
	if (!mutex)
		return -1;

	while ((hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, mutex->name)) == NULL)
		WaitForSingleObject(mutex->handle, INFINITE);
	return 0;
}

extern "C" int pthread_mutex_unlock(pthread_mutex_t* mutex) {
	if (!mutex)
		return -1;
	ReleaseMutex(mutex->handle);
	return 0;
}

extern "C" int pthread_mutex_destroy(pthread_mutex_t* mutex) {
	if (!mutex)
		return -1;
	ReleaseMutex(mutex->handle);
	return 0;
}