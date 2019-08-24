/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999,2008 Oracle.  All rights reserved.
 *
 * $Id: mut_pthread.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * This is where we load in architecture/compiler specific mutex code.
 */
#define	LOAD_ACTUAL_MUTEX_CODE
#include "dbinc/mutex_int.h"

#ifdef HAVE_MUTEX_SOLARIS_LWP
#define	pthread_cond_destroy(x)		0
#define	pthread_cond_signal		_lwp_cond_signal
#define	pthread_cond_wait		_lwp_cond_wait
#define	pthread_mutex_destroy(x)	0
#define	pthread_mutex_lock		_lwp_mutex_lock
#define	pthread_mutex_trylock		_lwp_mutex_trylock
#define	pthread_mutex_unlock		_lwp_mutex_unlock
#endif
#ifdef HAVE_MUTEX_UI_THREADS
#define	pthread_cond_destroy(x)		cond_destroy
#define	pthread_cond_signal		cond_signal
#define	pthread_cond_wait		cond_wait
#define	pthread_mutex_destroy		mutex_destroy
#define	pthread_mutex_lock		mutex_lock
#define	pthread_mutex_trylock		mutex_trylock
#define	pthread_mutex_unlock		mutex_unlock
#endif

#define	PTHREAD_UNLOCK_ATTEMPTS	5

/*
 * IBM's MVS pthread mutex implementation returns -1 and sets errno rather than
 * returning errno itself.  As -1 is not a valid errno value, assume functions
 * returning -1 have set errno.  If they haven't, return a random error value.
 */
#define	RET_SET(f, ret) do {						\
	if (((ret) = (f)) == -1 && ((ret) = errno) == 0)		\
		(ret) = EAGAIN;						\
} while (0)

/*
 * __db_pthread_mutex_init --
 *	Initialize a pthread mutex.
 *
 * PUBLIC: int __db_pthread_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
 */
int
__db_pthread_mutex_init(env, mutex, flags)
	ENV *env;
	db_mutex_t mutex;
	u_int32_t flags;
{
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	int ret;

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);
	ret = 0;

#ifdef HAVE_MUTEX_PTHREADS
	{
	pthread_condattr_t condattr, *condattrp = NULL;
	pthread_mutexattr_t mutexattr, *mutexattrp = NULL;

	if (!LF_ISSET(DB_MUTEX_PROCESS_ONLY)) {
		RET_SET((pthread_mutexattr_init(&mutexattr)), ret);
#ifndef HAVE_MUTEX_THREAD_ONLY
		if (ret == 0)
			RET_SET((pthread_mutexattr_setpshared(
			    &mutexattr, PTHREAD_PROCESS_SHARED)), ret);
#endif
		mutexattrp = &mutexattr;
	}

	if (ret == 0)
		RET_SET((pthread_mutex_init(&mutexp->mutex, mutexattrp)), ret);
	if (mutexattrp != NULL)
		(void)pthread_mutexattr_destroy(mutexattrp);
	if (ret == 0 && LF_ISSET(DB_MUTEX_SELF_BLOCK)) {
		if (!LF_ISSET(DB_MUTEX_PROCESS_ONLY)) {
			RET_SET((pthread_condattr_init(&condattr)), ret);
			if (ret == 0) {
				condattrp = &condattr;
#ifndef HAVE_MUTEX_THREAD_ONLY
				RET_SET((pthread_condattr_setpshared(
				    &condattr, PTHREAD_PROCESS_SHARED)), ret);
#endif
			}
		}

		if (ret == 0)
			RET_SET(
			    (pthread_cond_init(&mutexp->cond, condattrp)), ret);

		F_SET(mutexp, DB_MUTEX_SELF_BLOCK);
		if (condattrp != NULL)
			(void)pthread_condattr_destroy(condattrp);
	}

	}
#endif
#ifdef HAVE_MUTEX_SOLARIS_LWP
	/*
	 * XXX
	 * Gcc complains about missing braces in the static initializations of
	 * lwp_cond_t and lwp_mutex_t structures because the structures contain
	 * sub-structures/unions and the Solaris include file that defines the
	 * initialization values doesn't have surrounding braces.  There's not
	 * much we can do.
	 */
	if (LF_ISSET(DB_MUTEX_PROCESS_ONLY)) {
		static lwp_mutex_t mi = DEFAULTMUTEX;

		mutexp->mutex = mi;
	} else {
		static lwp_mutex_t mi = SHAREDMUTEX;

		mutexp->mutex = mi;
	}
	if (LF_ISSET(DB_MUTEX_SELF_BLOCK)) {
		if (LF_ISSET(DB_MUTEX_PROCESS_ONLY)) {
			static lwp_cond_t ci = DEFAULTCV;

			mutexp->cond = ci;
		} else {
			static lwp_cond_t ci = SHAREDCV;

			mutexp->cond = ci;
		}
		F_SET(mutexp, DB_MUTEX_SELF_BLOCK);
	}
#endif
#ifdef HAVE_MUTEX_UI_THREADS
	{
	int type;

	type = LF_ISSET(DB_MUTEX_PROCESS_ONLY) ? USYNC_THREAD : USYNC_PROCESS;

	ret = mutex_init(&mutexp->mutex, type, NULL);
	if (ret == 0 && LF_ISSET(DB_MUTEX_SELF_BLOCK)) {
		ret = cond_init(&mutexp->cond, type, NULL);

		F_SET(mutexp, DB_MUTEX_SELF_BLOCK);
	}}
#endif

	if (ret != 0) {
		__db_err(env, ret, "unable to initialize mutex");
	}
	return (ret);
}

/*
 * __db_pthread_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_pthread_mutex_lock __P((ENV *, db_mutex_t));
 */
int
__db_pthread_mutex_lock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	int i, ret;

	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

	CHECK_MTX_THREAD(env, mutexp);

#if defined(HAVE_STATISTICS) && !defined(HAVE_MUTEX_HYBRID)
	/*
	 * We want to know which mutexes are contentious, but don't want to
	 * do an interlocked test here -- that's slower when the underlying
	 * system has adaptive mutexes and can perform optimizations like
	 * spinning only if the thread holding the mutex is actually running
	 * on a CPU.  Make a guess, using a normal load instruction.
	 */
	if (F_ISSET(mutexp, DB_MUTEX_LOCKED))
		++mutexp->mutex_set_wait;
	else
		++mutexp->mutex_set_nowait;
#endif

	RET_SET((pthread_mutex_lock(&mutexp->mutex)), ret);
	if (ret != 0)
		goto err;

	if (F_ISSET(mutexp, DB_MUTEX_SELF_BLOCK)) {
		/*
		 * If we are using hybrid mutexes then the pthread mutexes
		 * are only used to wait after spinning on the TAS mutex.
		 * Set the wait flag before checking to see if the mutex
		 * is still locked.  The holder will clear the bit before
		 * checking the wait flag.
		 */
#ifdef HAVE_MUTEX_HYBRID
		mutexp->wait++;
		MUTEX_MEMBAR(mutexp->wait);
#endif
		while (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			RET_SET((pthread_cond_wait(
			    &mutexp->cond, &mutexp->mutex)), ret);
			/*
			 * !!!
			 * Solaris bug workaround:
			 * pthread_cond_wait() sometimes returns ETIME -- out
			 * of sheer paranoia, check both ETIME and ETIMEDOUT.
			 * We believe this happens when the application uses
			 * SIGALRM for some purpose, e.g., the C library sleep
			 * call, and Solaris delivers the signal to the wrong
			 * LWP.
			 */
			if (ret != 0 && ret != EINTR &&
#ifdef ETIME
			    ret != ETIME &&
#endif
			    ret != ETIMEDOUT) {
				(void)pthread_mutex_unlock(&mutexp->mutex);
				goto err;
			}
		}

#ifdef HAVE_MUTEX_HYBRID
		mutexp->wait--;
#else
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
#endif

		/*
		 * According to HP-UX engineers contacted by Netscape,
		 * pthread_mutex_unlock() will occasionally return EFAULT
		 * for no good reason on mutexes in shared memory regions,
		 * and the correct caller behavior is to try again.  Do
		 * so, up to PTHREAD_UNLOCK_ATTEMPTS consecutive times.
		 * Note that we don't bother to restrict this to HP-UX;
		 * it should be harmless elsewhere. [#2471]
		 */
		i = PTHREAD_UNLOCK_ATTEMPTS;
		do {
			RET_SET((pthread_mutex_unlock(&mutexp->mutex)), ret);
		} while (ret == EFAULT && --i > 0);
		if (ret != 0)
			goto err;
	} else {
#ifdef DIAGNOSTIC
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			(void)dbenv->thread_id_string(dbenv,
			    mutexp->pid, mutexp->tid, buf);
			__db_errx(env,
		    "pthread lock failed: lock currently in use: pid/tid: %s",
			    buf);
			ret = EINVAL;
			goto err;
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
	}

#ifdef DIAGNOSTIC
	/*
	 * We want to switch threads as often as possible.  Yield every time
	 * we get a mutex to ensure contention.
	 */
	if (F_ISSET(dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif
	return (0);

err:	__db_err(env, ret, "pthread lock failed");
	return (__env_panic(env, ret));
}

/*
 * __db_pthread_mutex_unlock --
 *	Release a mutex.
 *
 * PUBLIC: int __db_pthread_mutex_unlock __P((ENV *, db_mutex_t));
 */
int
__db_pthread_mutex_unlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	int i, ret;

	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

#if !defined(HAVE_MUTEX_HYBRID) && defined(DIAGNOSTIC)
	if (!F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
		__db_errx(
		    env, "pthread unlock failed: lock already unlocked");
		return (__env_panic(env, EACCES));
	}
#endif
	if (F_ISSET(mutexp, DB_MUTEX_SELF_BLOCK)) {
		RET_SET((pthread_mutex_lock(&mutexp->mutex)), ret);
		if (ret != 0)
			goto err;

		F_CLR(mutexp, DB_MUTEX_LOCKED);

		RET_SET((pthread_cond_signal(&mutexp->cond)), ret);
		if (ret != 0)
			goto err;
	} else
		F_CLR(mutexp, DB_MUTEX_LOCKED);

	/* See comment above;  workaround for [#2471]. */
	i = PTHREAD_UNLOCK_ATTEMPTS;
	do {
		RET_SET((pthread_mutex_unlock(&mutexp->mutex)), ret);
	} while (ret == EFAULT && --i > 0);

err:	if (ret != 0) {
		__db_err(env, ret, "pthread unlock failed");
		return (__env_panic(env, ret));
	}
	return (ret);
}

/*
 * __db_pthread_mutex_destroy --
 *	Destroy a mutex.
 *
 * PUBLIC: int __db_pthread_mutex_destroy __P((ENV *, db_mutex_t));
 */
int
__db_pthread_mutex_destroy(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	int ret, t_ret;

	if (!MUTEX_ON(env))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mutex);

	ret = 0;
	if (F_ISSET(mutexp, DB_MUTEX_SELF_BLOCK)) {
		RET_SET((pthread_cond_destroy(&mutexp->cond)), ret);
		if (ret != 0)
			__db_err(env, ret, "unable to destroy cond");
	}
	RET_SET((pthread_mutex_destroy(&mutexp->mutex)), t_ret);
	if (t_ret != 0) {
		__db_err(env, t_ret, "unable to destroy mutex");
		if (ret == 0)
			ret = t_ret;
	}
	return (ret);
}
