/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996,2008 Oracle.  All rights reserved.
 *
 * $Id: mp_fput.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"

static int __memp_reset_lru __P((ENV *, REGINFO *));

/*
 * __memp_fput_pp --
 *	DB_MPOOLFILE->put pre/post processing.
 *
 * PUBLIC: int __memp_fput_pp
 * PUBLIC:     __P((DB_MPOOLFILE *, void *, DB_CACHE_PRIORITY, u_int32_t));
 */
int
__memp_fput_pp(dbmfp, pgaddr, priority, flags)
	DB_MPOOLFILE *dbmfp;
	void *pgaddr;
	DB_CACHE_PRIORITY priority;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret, t_ret;

	env = dbmfp->env;

	if (flags != 0)
		return (__db_ferr(env, "DB_MPOOLFILE->put", 0));

	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->put");

	ENV_ENTER(env, ip);

	ret = __memp_fput(dbmfp, ip, pgaddr, priority);
	if (IS_ENV_REPLICATED(env) &&
	    (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_fput --
 *	DB_MPOOLFILE->put.
 *
 * PUBLIC: int __memp_fput __P((DB_MPOOLFILE *,
 * PUBLIC:      DB_THREAD_INFO *, void *, DB_CACHE_PRIORITY));
 */
int
__memp_fput(dbmfp, ip, pgaddr, priority)
	DB_MPOOLFILE *dbmfp;
	DB_THREAD_INFO *ip;
	void *pgaddr;
	DB_CACHE_PRIORITY priority;
{
	BH *bhp;
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	ENV *env;
	MPOOL *c_mp;
	MPOOLFILE *mfp;
	PIN_LIST *list, *lp;
	REGINFO *infop, *reginfo;
	roff_t b_ref;
	int region;
	int adjust, pfactor, ret, t_ret;
	char buf[DB_THREADID_STRLEN];

	env = dbmfp->env;
	dbenv = env->dbenv;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;
	bhp = (BH *)((u_int8_t *)pgaddr - SSZA(BH, buf));
	ret = 0;

	/*
	 * If this is marked dummy, we are using it to unpin a buffer for
	 * another thread.
	 */
	if (F_ISSET(dbmfp, MP_DUMMY))
		goto unpin;

	/*
	 * If we're mapping the file, there's nothing to do.  Because we can
	 * stop mapping the file at any time, we have to check on each buffer
	 * to see if the address we gave the application was part of the map
	 * region.
	 */
	if (dbmfp->addr != NULL && pgaddr >= dbmfp->addr &&
	    (u_int8_t *)pgaddr <= (u_int8_t *)dbmfp->addr + dbmfp->len)
		return (0);

#ifdef DIAGNOSTIC
	/*
	 * Decrement the per-file pinned buffer count (mapped pages aren't
	 * counted).
	 */
	MPOOL_SYSTEM_LOCK(env);
	if (dbmfp->pinref == 0) {
		MPOOL_SYSTEM_UNLOCK(env);
		__db_errx(env,
		    "%s: more pages returned than retrieved", __memp_fn(dbmfp));
		return (__env_panic(env, EACCES));
	}
	--dbmfp->pinref;
	MPOOL_SYSTEM_UNLOCK(env);
#endif

unpin:
	/* Convert a page address to a buffer header and hash bucket. */
	MP_GET_BUCKET(env, mfp, bhp->pgno, &infop, hp, ret);
	if (ret != 0)
		return (ret);
	c_mp = infop->primary;

	/*
	 * Check for a reference count going to zero.  This can happen if the
	 * application returns a page twice.
	 */
	if (bhp->ref == 0) {
		__db_errx(env, "%s: page %lu: unpinned page returned",
		    __memp_fn(dbmfp), (u_long)bhp->pgno);
		DB_ASSERT(env, bhp->ref != 0);
		MUTEX_UNLOCK(env, hp->mtx_hash);
		return (__env_panic(env, EACCES));
	}

	/* Note the activity so allocation won't decide to quit. */
	++c_mp->put_counter;

	if (ip != NULL) {
		reginfo = env->reginfo;
		list = R_ADDR(reginfo, ip->dbth_pinlist);
		region = (int)(infop - dbmp->reginfo);
		b_ref = R_OFFSET(infop, bhp);
		for (lp = list; lp < &list[ip->dbth_pinmax]; lp++)
			if (lp->b_ref == b_ref && lp->region == region)
				break;

		if (lp == &list[ip->dbth_pinmax]) {
			__db_errx(env,
		    "__memp_fput: pinned buffer not found for thread %s",
			    dbenv->thread_id_string(dbenv,
			    ip->dbth_pid, ip->dbth_tid, buf));
			return (__env_panic(env, EINVAL));
		}

		lp->b_ref = INVALID_ROFF;
		ip->dbth_pincount--;
	}

	/*
	 * Mark the file dirty.  Check for a dirty bit on the buffer as well
	 * as the dirty flag because the buffer might have been marked dirty
	 * in the DB_MPOOLFILE->set method.
	 */
	if (F_ISSET(bhp, BH_DIRTY)) {
		mfp->file_written = 1;

		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
	}

	/*
	 * If more than one reference to the page or a reference other than a
	 * thread waiting to flush the buffer to disk, we're done.  Ignore the
	 * discard flags (for now) and leave the buffer's priority alone.
	 */
	if (--bhp->ref > 1 || (bhp->ref == 1 && !F_ISSET(bhp, BH_LOCKED))) {
		MUTEX_UNLOCK(env, hp->mtx_hash);
		return (0);
	}

	/* The buffer should not be accessed again. */
	MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize, 0);

	/* Update priority values. */
	if (priority == DB_PRIORITY_VERY_LOW ||
	    mfp->priority == MPOOL_PRI_VERY_LOW)
		bhp->priority = 0;
	else {
		/*
		 * We don't lock the LRU counter or the stat.st_pages field, if
		 * we get garbage (which won't happen on a 32-bit machine), it
		 * only means a buffer has the wrong priority.
		 */
		bhp->priority = c_mp->lru_count;

		switch (priority) {
		default:
		case DB_PRIORITY_UNCHANGED:
			pfactor = mfp->priority;
			break;
		case DB_PRIORITY_VERY_LOW:
			pfactor = MPOOL_PRI_VERY_LOW;
			break;
		case DB_PRIORITY_LOW:
			pfactor = MPOOL_PRI_LOW;
			break;
		case DB_PRIORITY_DEFAULT:
			pfactor = MPOOL_PRI_DEFAULT;
			break;
		case DB_PRIORITY_HIGH:
			pfactor = MPOOL_PRI_HIGH;
			break;
		case DB_PRIORITY_VERY_HIGH:
			pfactor = MPOOL_PRI_VERY_HIGH;
			break;
		}

		adjust = 0;
		if (pfactor != 0)
			adjust = (int)c_mp->stat.st_pages / pfactor;

		if (F_ISSET(bhp, BH_DIRTY))
			adjust += (int)c_mp->stat.st_pages / MPOOL_PRI_DIRTY;

		if (adjust > 0) {
			if (UINT32_MAX - bhp->priority >= (u_int32_t)adjust)
				bhp->priority += adjust;
		} else if (adjust < 0)
			if (bhp->priority > (u_int32_t)-adjust)
				bhp->priority += adjust;
	}

	/*
	 * The sync code has a separate counter for buffers on which it waits.
	 * It reads that value without holding a lock so we update it as the
	 * last thing we do.  Once that value goes to 0, we won't see another
	 * reference to that buffer being returned to the cache until the sync
	 * code has finished, so we're safe as long as we don't let the value
	 * go to 0 before we finish with the buffer.
	 */
	if (F_ISSET(bhp, BH_LOCKED) && bhp->ref_sync != 0)
		--bhp->ref_sync;

	MUTEX_UNLOCK(env, hp->mtx_hash);

	/*
	 * On every buffer put we update the buffer generation number and check
	 * for wraparound.
	 */
	if (++c_mp->lru_count == UINT32_MAX)
		if ((t_ret =
		    __memp_reset_lru(env, dbmp->reginfo)) != 0 && ret == 0)
			ret = t_ret;

	return (ret);
}

/*
 * __memp_reset_lru --
 *	Reset the cache LRU counter.
 */
static int
__memp_reset_lru(env, infop)
	ENV *env;
	REGINFO *infop;
{
	BH *bhp, *tbhp;
	DB_MPOOL_HASH *hp;
	MPOOL *c_mp;
	u_int32_t bucket, priority;

	c_mp = infop->primary;
	/*
	 * Update the counter so all future allocations will start at the
	 * bottom.
	 */
	c_mp->lru_count -= MPOOL_BASE_DECREMENT;

	/* Adjust the priority of every buffer in the system. */
	for (hp = R_ADDR(infop, c_mp->htab),
	    bucket = 0; bucket < c_mp->htab_buckets; ++hp, ++bucket) {
		/*
		 * Skip empty buckets.
		 *
		 * We can check for empty buckets before locking as we
		 * only care if the pointer is zero or non-zero.
		 */
		if (SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL) {
			c_mp->lru_reset++;
			continue;
		}

		MUTEX_LOCK(env, hp->mtx_hash);
		c_mp->lru_reset++;
		/*
		 * We need to take a little care that the bucket does
		 * not become unsorted.  This is highly unlikely but
		 * possible.
		 */
		priority = 0;
		SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
			for (tbhp = bhp; tbhp != NULL;
			    tbhp = SH_CHAIN_PREV(tbhp, vc, __bh)) {
				if (tbhp->priority != UINT32_MAX &&
				    tbhp->priority > MPOOL_BASE_DECREMENT) {
					tbhp->priority -= MPOOL_BASE_DECREMENT;
					if (tbhp->priority < priority)
						tbhp->priority = priority;
				}
			}
			priority = bhp->priority;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	c_mp->lru_reset = 0;

	COMPQUIET(env, NULL);
	return (0);
}

/*
 * __memp_unpin_buffers --
 *	Unpin buffers pinned by a thread.
 *
 * PUBLIC: int __memp_unpin_buffers __P((ENV *, DB_THREAD_INFO *));
 */
int
__memp_unpin_buffers(env, ip)
	ENV *env;
	DB_THREAD_INFO *ip;
{
	BH *bhp;
	DB_MPOOL *dbmp;
	DB_MPOOLFILE dbmf;
	PIN_LIST *list, *lp;
	REGINFO *rinfop, *reginfo;
	int ret;

	memset(&dbmf, 0, sizeof(dbmf));
	dbmf.env = env;
	dbmf.flags = MP_DUMMY;
	dbmp = env->mp_handle;
	reginfo = env->reginfo;

	list = R_ADDR(reginfo, ip->dbth_pinlist);
	for (lp = list; lp < &list[ip->dbth_pinmax]; lp++) {
		if (lp->b_ref == INVALID_ROFF)
			continue;
		rinfop = &dbmp->reginfo[lp->region];
		bhp = R_ADDR(rinfop, lp->b_ref);
		dbmf.mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);
		if ((ret = __memp_fput(&dbmf, ip,
		    (u_int8_t *)bhp + SSZA(BH, buf),
		    DB_PRIORITY_UNCHANGED)) != 0)
			return (ret);
	}
	return (0);
}
