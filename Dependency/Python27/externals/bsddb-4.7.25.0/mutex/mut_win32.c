/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 *
 * $Id: mut_win32.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * This is where we load in the actual test-and-set mutex code.
 */
#define	LOAD_ACTUAL_MUTEX_CODE
#include "dbinc/mutex_int.h"

/* We don't want to run this code even in "ordinary" diagnostic mode. */
#undef MUTEX_DIAG

/*
 * Common code to get an event handle.  This is executed whenever a mutex
 * blocks, or when unlocking a mutex that a thread is waiting on.  We can't
 * keep these handles around, since the mutex structure is in shared memory,
 * and each process gets its own handle value.
 *
 * We pass security attributes so that the created event is accessible by all
 * users, in case a Windows service is sharing an environment with a local
 * process run as a different user.
 */
static _TCHAR hex_digits[] = _T("0123456789abcdef");
static SECURITY_DESCRIPTOR null_sd;
static SECURITY_ATTRIBUTES all_sa;
static int security_initialized = 0;

static __inline int get_handle(env, mutexp, eventp)
	ENV *env;
	DB_MUTEX *mutexp;
	HANDLE *eventp;
{
	_TCHAR idbuf[] = _T("db.m00000000");
	_TCHAR *p = idbuf + 12;
	int ret = 0;
	u_int32_t id;

	for (id = (mutexp)->id; id != 0; id >>= 4)
		*--p = hex_digits[id & 0xf];

#ifndef DB_WINCE
	if (!security_initialized) {
		InitializeSecurityDescriptor(&null_sd,
		    SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&null_sd, TRUE, 0, FALSE);
		all_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		all_sa.bInheritHandle = FALSE;
		all_sa.lpSecurityDescriptor = &null_sd;
		security_initialized = 1;
	}
#endif

	if ((*eventp = CreateEvent(&all_sa, FALSE, FALSE, idbuf)) == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "Win32 create event failed");
	}

	return (ret);
}

/*
 * __db_win32_mutex_init --
 *	Initialize a Win32 mutex.
 *
 * PUBLIC: int __db_win32_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
 */
int
__db_win32_mutex_init(env, mutex, flags)
	ENV *env;
	db_mutex_t mutex;
	u_int32_t flags;
{
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

	mutexp->id = ((getpid() & 0xffff) << 16) ^ P_TO_UINT32(mutexp);

	return (0);
}

/*
 * __db_win32_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_win32_mutex_lock __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_lock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	HANDLE event;
	u_int32_t nspins;
	int ms, ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER now;
#endif
#ifdef DB_WINCE
	volatile db_threadid_t tmp_tid;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

	CHECK_MTX_THREAD(env, mutexp);

	event = NULL;
	ms = 50;
	ret = 0;

loop:	/* Attempt to acquire the resource for N spins. */
	for (nspins =
	    mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		/*
		 * We can avoid the (expensive) interlocked instructions if
		 * the mutex is already "set".
		 */
#ifdef DB_WINCE
		/*
		 * Memory mapped regions on Windows CE cause problems with
		 * InterlockedExchange calls. Each page in a mapped region
		 * needs to have been written to prior to an
		 * InterlockedExchange call, or the InterlockedExchange call
		 * hangs. This does not seem to be documented anywhere. For
		 * now, read/write a non-critical piece of memory from the
		 * shared region prior to attempting an InterlockedExchange
		 * operation.
		 */
		tmp_tid = mutexp->tid;
		mutexp->tid = tmp_tid;
#endif
		if (mutexp->tas || !MUTEX_SET(&mutexp->tas)) {
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
#ifdef MUTEX_PAUSE
			MUTEX_PAUSE
#endif
			continue;
		}

#ifdef DIAGNOSTIC
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			__db_errx(env,
			    "Win32 lock failed: mutex already locked by %s",
			     dbenv->thread_id_string(dbenv,
			     mutexp->pid, mutexp->tid, buf));
			return (__env_panic(env, EACCES));
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);

#ifdef HAVE_STATISTICS
		if (event == NULL)
			++mutexp->mutex_set_nowait;
		else
			++mutexp->mutex_set_wait;
#endif
		if (event != NULL) {
			CloseHandle(event);
			InterlockedDecrement(&mutexp->nwaiters);
#ifdef MUTEX_DIAG
			if (ret != WAIT_OBJECT_0) {
				QueryPerformanceCounter(&now);
				printf("[%I64d]: Lost signal on mutex %p, "
				    "id %d, ms %d\n",
				    now.QuadPart, mutexp, mutexp->id, ms);
			}
#endif
		}

#ifdef DIAGNOSTIC
		/*
		 * We want to switch threads as often as possible.  Yield
		 * every time we get a mutex to ensure contention.
		 */
		if (F_ISSET(dbenv, DB_ENV_YIELDCPU))
			__os_yield(env, 0, 0);
#endif

		return (0);
	}

	/*
	 * Yield the processor; wait 50 ms initially, up to 1 second.  This
	 * loop is needed to work around a race where the signal from the
	 * unlocking thread gets lost.  We start at 50 ms because it's unlikely
	 * to happen often and we want to avoid wasting CPU.
	 */
	if (event == NULL) {
#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&now);
		printf("[%I64d]: Waiting on mutex %p, id %d\n",
		    now.QuadPart, mutexp, mutexp->id);
#endif
		InterlockedIncrement(&mutexp->nwaiters);
		if ((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
	}
	if ((ret = WaitForSingleObject(event, ms)) == WAIT_FAILED) {
		ret = __os_get_syserr();
		goto err;
	}
	if ((ms <<= 1) > MS_PER_SEC)
		ms = MS_PER_SEC;

	PANIC_CHECK(env);
	goto loop;

err:	__db_syserr(env, ret, "Win32 lock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_win32_mutex_unlock --
 *	Release a mutex.
 *
 * PUBLIC: int __db_win32_mutex_unlock __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_unlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	HANDLE event;
	int ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER now;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

#ifdef DIAGNOSTIC
	if (!mutexp->tas || !F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
		__db_errx(env, "Win32 unlock failed: lock already unlocked");
		return (__env_panic(env, EACCES));
	}
#endif
	F_CLR(mutexp, DB_MUTEX_LOCKED);
	MUTEX_UNSET(&mutexp->tas);

	if (mutexp->nwaiters > 0) {
		if ((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;

#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&now);
		printf("[%I64d]: Signalling mutex %p, id %d\n",
		    now.QuadPart, mutexp, mutexp->id);
#endif
		if (!PulseEvent(event)) {
			ret = __os_get_syserr();
			CloseHandle(event);
			goto err;
		}

		CloseHandle(event);
	}

	return (0);

err:	__db_syserr(env, ret, "Win32 unlock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_win32_mutex_destroy --
 *	Destroy a mutex.
 *
 * PUBLIC: int __db_win32_mutex_destroy __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_destroy(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (0);
}
