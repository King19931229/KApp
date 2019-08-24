/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006,2008 Oracle.  All rights reserved.
 *
 * $Id: repmgr_queue.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#define	__INCLUDE_NETWORKING	1
#include "db_int.h"

typedef STAILQ_HEAD(__repmgr_q_header, __repmgr_message) QUEUE_HEADER;
struct __repmgr_queue {
	int size;
	QUEUE_HEADER header;
};

/*
 * PUBLIC: int __repmgr_queue_create __P((ENV *, DB_REP *));
 */
int
__repmgr_queue_create(env, db_rep)
	ENV *env;
	DB_REP *db_rep;
{
	REPMGR_QUEUE *q;
	int ret;

	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_QUEUE), &q)) != 0)
		return (ret);
	q->size = 0;
	STAILQ_INIT(&q->header);
	db_rep->input_queue = q;
	return (0);
}

/*
 * Frees not only the queue header, but also any messages that may be on it,
 * along with their data buffers.
 *
 * PUBLIC: void __repmgr_queue_destroy __P((ENV *));
 */
void
__repmgr_queue_destroy(env)
	ENV *env;
{
	REPMGR_MESSAGE *m;
	REPMGR_QUEUE *q;

	if ((q = env->rep_handle->input_queue) == NULL)
		return;

	while (!STAILQ_EMPTY(&q->header)) {
		m = STAILQ_FIRST(&q->header);
		STAILQ_REMOVE_HEAD(&q->header, entries);
		__os_free(env, m);
	}
	__os_free(env, q);
}

/*
 * PUBLIC: int __repmgr_queue_get __P((ENV *, REPMGR_MESSAGE **));
 *
 * Get the first input message from the queue and return it to the caller.  The
 * caller hereby takes responsibility for the entire message buffer, and should
 * free it when done.
 *
 * Note that caller is NOT expected to hold the mutex.  This is asymmetric with
 * put(), because put() is expected to be called in a loop after select, where
 * it's already necessary to be holding the mutex.
 */
int
__repmgr_queue_get(env, msgp)
	ENV *env;
	REPMGR_MESSAGE **msgp;
{
	DB_REP *db_rep;
	REPMGR_MESSAGE *m;
	REPMGR_QUEUE *q;
	int ret;

	ret = 0;
	db_rep = env->rep_handle;
	q = db_rep->input_queue;

	LOCK_MUTEX(db_rep->mutex);
	while (STAILQ_EMPTY(&q->header) && !db_rep->finished) {
#ifdef DB_WIN32
		if (!ResetEvent(db_rep->queue_nonempty)) {
			ret = GetLastError();
			goto err;
		}
		if (SignalObjectAndWait(db_rep->mutex, db_rep->queue_nonempty,
			INFINITE, FALSE) != WAIT_OBJECT_0) {
			ret = GetLastError();
			goto err;
		}
		LOCK_MUTEX(db_rep->mutex);
#else
		if ((ret = pthread_cond_wait(&db_rep->queue_nonempty,
		    &db_rep->mutex)) != 0)
			goto err;
#endif
	}
	if (db_rep->finished)
		ret = DB_REP_UNAVAIL;
	else {
		m = STAILQ_FIRST(&q->header);
		STAILQ_REMOVE_HEAD(&q->header, entries);
		q->size--;
		*msgp = m;
	}

err:
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

/*
 * PUBLIC: int __repmgr_queue_put __P((ENV *, REPMGR_MESSAGE *));
 *
 * !!!
 * Caller must hold repmgr->mutex.
 */
int
__repmgr_queue_put(env, msg)
	ENV *env;
	REPMGR_MESSAGE *msg;
{
	DB_REP *db_rep;
	REPMGR_QUEUE *q;

	db_rep = env->rep_handle;
	q = db_rep->input_queue;

	STAILQ_INSERT_TAIL(&q->header, msg, entries);
	q->size++;

	return (__repmgr_signal(&db_rep->queue_nonempty));
}

/*
 * PUBLIC: int __repmgr_queue_size __P((ENV *));
 *
 * !!!
 * Caller must hold repmgr->mutex.
 */
int
__repmgr_queue_size(env)
	ENV *env;
{
	return (env->rep_handle->input_queue->size);
}
