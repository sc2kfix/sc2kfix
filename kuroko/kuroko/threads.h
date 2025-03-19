#pragma once
/**
 * @file threads.h
 * @brief Convience header for providing atomic operations to threads.
 */

#ifndef KRK_DISABLE_THREADS
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
#define sched_yield() YieldProcessor()
#endif

static inline void _krk_internal_spin_lock(int volatile * lock) {
	while (*lock)
		sched_yield();
	*lock = 1;
}

static inline void _krk_internal_spin_unlock(int volatile * lock) {
	*lock = 0;
}

#define _obtain_lock(v)  _krk_internal_spin_lock(&v);
#define _release_lock(v) _krk_internal_spin_unlock(&v);

#else

#define _obtain_lock(v)
#define _release_lock(v)

#define pthread_rwlock_init(a,b) ((void)0)
#define pthread_rwlock_wrlock(a) ((void)0)
#define pthread_rwlock_rdlock(a) ((void)0)
#define pthread_rwlock_unlock(a) ((void)0)

#endif

