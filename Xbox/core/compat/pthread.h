/* A tiny pthreads for the core on UWP: the mutex the save states use, and the thread of the threaded renderer
 * (which the Xbox app leaves off). */
#pragma once
#include <windows.h>
#include <process.h>
#include <stdlib.h>

typedef SRWLOCK pthread_mutex_t; /* zero = unlocked, like a static pthread mutex */
typedef void pthread_mutexattr_t;
#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
static __inline int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a) { (void)a; InitializeSRWLock(m); return 0; }
static __inline int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }
static __inline int pthread_mutex_lock(pthread_mutex_t *m) { AcquireSRWLockExclusive(m); return 0; }
static __inline int pthread_mutex_unlock(pthread_mutex_t *m) { ReleaseSRWLockExclusive(m); return 0; }

typedef HANDLE pthread_t;
typedef void pthread_attr_t;
typedef struct { void *(*fn)(void *); void *arg; } pthread_start_info_;
static unsigned __stdcall pthread_start_(void *p)
{
   pthread_start_info_ info = *(pthread_start_info_ *)p;
   free(p);
   info.fn(info.arg);
   return 0;
}
static __inline int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg)
{
   pthread_start_info_ *info = (pthread_start_info_ *)malloc(sizeof(*info));
   (void)a;
   info->fn = fn;
   info->arg = arg;
   *t = (HANDLE)_beginthreadex(NULL, 0, pthread_start_, info, 0, NULL);
   return *t ? 0 : -1;
}
static __inline int pthread_join(pthread_t t, void **ret)
{
   (void)ret;
   WaitForSingleObject(t, INFINITE);
   CloseHandle(t);
   return 0;
}
