// quick and dirty implementation of the things kuroko needs from pthreads

#pragma once

#include <windows.h>
#include <process.h>

typedef struct {
	SRWLOCK lock;
	BOOL exclusive;
} pthread_rwlock_t;

typedef struct {
	HANDLE handle;
	TCHAR* name;
} pthread_mutex_t;

typedef HANDLE pthread_t;
typedef int pid_t;

#ifndef __cplusplus
int pthread_rwlock_init(pthread_rwlock_t* rwlock, const void* attr);
int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t* rwlock);

int pthread_create(pthread_t* thread, const void* attr, void* (*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void** value_ptr);

int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
#endif