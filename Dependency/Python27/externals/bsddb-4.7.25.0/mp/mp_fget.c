/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996,2008 Oracle.  All rights reserved.
 *
 * $Id: mp_fget.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

/*
 * __memp_fget_pp --
 *	DB_MPOOLFILE->get pre/post processing.
 *
 * PUBLIC: int __memp_fget_pp
 * PUBLIC:     __P((DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, u_int32_t, void *));
 */
int
__memp_fget_pp(dbmfp, pgnoaddr, txnp, flags, addrp)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t *pgnoaddr;
	DB_TXN *txnp;
	u_int32_t flags;
	void *addrp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_blocked, ret;

	env = dbmfp->env;

	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->get");

	/*
	 * Validate arguments.
	 *
	 * !!!
	 * Don't test for DB_MPOOL_CREATE and DB_MPOOL_NEW flags for readonly
	 * files here, and create non-existent pages in readonly files if the
	 * flags are set, later.  The reason is that the hash access method
	 * wants to get empty pages that don't really exist in readonly files.
	 * The only alternative is for hash to write the last "bucket" all the
	 * time, which we don't want to do because one of our big goals in life
	 * is to keep database files small.  It's sleazy as hell, but we catch
	 * any attempt to actually write the file in memp_fput().
	 */
#define	OKFLAGS		(DB_MPOOL_CREATE | DB_MPOOL_DIRTY | \
	    DB_MPOOL_EDIT | DB_MPOOL_LAST | DB_MPOOL_NEW)
	if (flags != 0) {
		if ((ret = __db_fchk(env, "memp_fget", flags, OKFLAGS)) != 0)
			return (ret);

		switch (flags) {
		case DB_MPOOL_DIRTY:
		case DB_MPOOL_CREATE:
		case DB_MPOOL_EDIT:
		case DB_MPOOL_LAST:
		case DB_MPOOL_NEW:
			break;
		default:
			return (__db_ferr(env, "memp_fget", 1));
		}
	}

	ENV_ENTER(env, ip);

	rep_blocked = 0;
	if (txnp == NULL && IS_ENV_REPLICATED(env)) {
		if ((ret = __op_rep_enter(env)) != 0)
			goto err;
		rep_blocked = 1;
	}
	ret = __memp_fget(dbmfp, pgnoaddr, ip, txnp, flags, addrp);
	/*
	 * We only decrement the count in op_rep_exit if the operation fails.
	 * Otherwise the count will be decremented when the page is no longer
	 * pinned in memp_fput.
	 */
	if (ret != 0 && rep_blocked)
		(void)__op_rep_exit(env);

	/* Similarly if an app has a page pinned it is ACTIVE. */
err:	if (ret != 0)
		ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __memp_fget --
 *	Get a page from the file.
 *
 * PUBLIC: int __memp_fget __P((DB_MPOOLFILE *,
 * PUBLIC:     db_pgno_t *, DB_THREAD_INFO *, DB_TXN *, u_int32_t, void *));
 */
int
__memp_fget(dbmfp, pgnoaddr, ip, txn, flags, addrp)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t *pgnoaddr;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	u_int32_t flags;
	void *addrp;
{
	enum { FIRST_FOUND, FIRST_MISS, SECOND_FOUND, SECOND_MISS } state;
	BH *alloc_bhp, *bhp, *frozen_bhp, *oldest_bhp;
	ENV *env;
	DB_LSN *read_lsnp, vlsn;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *c_mp;
	MPOOLFILE *mfp;
	PIN_LIST *list, *lp;
	REGINFO *infop, *t_infop, *reginfo;
	TXN_DETAIL *td;
	roff_t list_off, mf_offset;
	u_int32_t pinmax, st_hsearch;
	int b_incr, b_locked, dirty, edit, extending, first;
	int makecopy, mvcc, need_free, ret;

	*(void **)addrp = NULL;
	COMPQUIET(c_mp, NULL);
	COMPQUIET(infop, NULL);
	COMPQUIET(oldest_bhp, NULL);

	env = dbmfp->env;
	dbmp = env->mp_handle;

	mfp = dbmfp->mfp;
	mvcc = mfp->multiversion;
	mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	alloc_bhp = bhp = frozen_bhp = NULL;
	read_lsnp = NULL;
	td = NULL;
	hp = NULL;
	b_incr = b_locked = extending = makecopy = ret = 0;

	if (LF_ISSET(DB_MPOOL_DIRTY)) {
		if (F_ISSET(dbmfp, MP_READONLY)) {
			__db_errx(env,
			    "%s: dirty flag set for readonly file page",
			    __memp_fn(dbmfp));
			return (EINVAL);
		}
		if ((ret = __db_fcchk(env, "DB_MPOOLFILE->get",
		    flags, DB_MPOOL_DIRTY, DB_MPOOL_EDIT)) != 0)
			return (ret);
	}

	dirty = LF_ISSET(DB_MPOOL_DIRTY);
	edit = LF_ISSET(DB_MPOOL_EDIT);
	LF_CLR(DB_MPOOL_DIRTY | DB_MPOOL_EDIT);

	/*
	 * If the transaction is being used to update a multiversion database
	 * for the first time, set the read LSN.  In addition, if this is an
	 * update, allocate a mutex.  If no transaction has been supplied, that
	 * will be caught later, when we know whether one is required.
	 */
	if (mvcc && txn != NULL && txn->td != NULL) {
		/* We're only interested in the ultimate parent transaction. */
		while (txn->parent != NULL)
			txn = txn->parent;
		td = (TXN_DETAIL *)txn->td;
		if (F_ISSET(txn, TXN_SNAPSHOT)) {
			read_lsnp = &td->read_lsn;
			if (IS_MAX_LSN(*read_lsnp) &&
			    (ret = __log_current_lsn(env, read_lsnp,
			    NULL, NULL)) != 0)
				return (ret);
		}
		if ((dirty || LF_ISSET(DB_MPOOL_CREATE | DB_MPOOL_NEW)) &&
		    td->mvcc_mtx == MUTEX_INVALID && (ret =
		    __mutex_alloc(env, MTX_TXN_MVCC, 0, &td->mvcc_mtx)) != 0)
			return (ret);
	}

	switch (flags) {
	case DB_MPOOL_LAST:
		/* Get the last page number in the file. */
		MUTEX_LOCK(env, mfp->mutex);
		*pgnoaddr = mfp->last_pgno;
		MUTEX_UNLOCK(env, mfp->mutex);
		break;
	case DB_MPOOL_NEW:
		/*
		 * If always creating a page, skip the first search
		 * of the hash bucket.
		 */
		state = FIRST_MISS;
		goto alloc;
	case DB_MPOOL_CREATE:
	default:
		break;
	}

	/*
	 * If mmap'ing the file and the page is not past the end of the file,
	 * just return a pointer.  We can't use R_ADDR here: this is an offset
	 * into an mmap'd file, not a shared region, and doesn't change for
	 * private environments.
	 *
	 * The page may be past the end of the file, so check the page number
	 * argument against the original length of the file.  If we previously
	 * returned pages past the original end of the file, last_pgno will
	 * have been updated to match the "new" end of the file, and checking
	 * against it would return pointers past the end of the mmap'd region.
	 *
	 * If another process has opened the file for writing since we mmap'd
	 * it, we will start playing the game by their rules, i.e. everything
	 * goes through the cache.  All pages previously returned will be safe,
	 * as long as the correct locking protocol was observed.
	 *
	 * We don't discard the map because we don't know when all of the
	 * pages will have been discarded from the process' address space.
	 * It would be possible to do so by reference counting the open
	 * pages from the mmap, but it's unclear to me that it's worth it.
	 */
	if (dbmfp->addr != NULL &&
	    F_ISSET(mfp, MP_CAN_MMAP) && *pgnoaddr <= mfp->orig_last_pgno) {
		*(void **)addrp = (u_int8_t *)dbmfp->addr +
		    (*pgnoaddr * mfp->stat.st_pagesize);
		STAT(++mfp->stat.st_map);
		return (0);
	}

retry:	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	MP_GET_BUCKET(env, mfp, *pgnoaddr, &infop, hp, ret);
	if (ret != 0)
		return (ret);
	c_mp = infop->primary;

	/* Search the hash chain for the page. */
	st_hsearch = 0;
	b_locked = 1;
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		++st_hsearch;
		if (bhp->pgno != *pgnoaddr || bhp->mf_offset != mf_offset)
			continue;

		/* Snapshot reads -- get the version visible at read_lsn. */
		if (mvcc && !edit && read_lsnp != NULL) {
			while (bhp != NULL &&
			    !BH_OWNED_BY(env, bhp, txn) &&
			    !BH_VISIBLE(env, bhp, read_lsnp, vlsn))
				bhp = SH_CHAIN_PREV(bhp, vc, __bh);

			DB_ASSERT(env, bhp != NULL);
		}

		makecopy = mvcc && dirty && !BH_OWNED_BY(env, bhp, txn);

		if (F_ISSET(bhp, BH_FROZEN) && !F_ISSET(bhp, BH_FREED)) {
			DB_ASSERT(env, frozen_bhp == NULL);
			frozen_bhp = bhp;
		}

		/*
		 * Increment the reference count.  We may discard the hash
		 * bucket lock as we evaluate and/or read the buffer, so we
		 * need to ensure it doesn't move and its contents remain
		 * unchanged.
		 */
		if (bhp->ref == UINT16_MAX) {
			__db_errx(env,
			    "%s: page %lu: reference count overflow",
			    __memp_fn(dbmfp), (u_long)bhp->pgno);
			ret = __env_panic(env, EINVAL);
			goto err;
		}
		++bhp->ref;
		b_incr = 1;

		/*
		 * BH_LOCKED --
		 * I/O is in progress or sync is waiting on the buffer to write
		 * it.  Because we've incremented the buffer reference count,
		 * we know the buffer can't move.  Unlock the bucket lock, wait
		 * for the buffer to become available, re-acquire the bucket.
		 */
		for (first = 1; F_ISSET(bhp, BH_LOCKED) &&
		    !F_ISSET(env->dbenv, DB_ENV_NOLOCKING); first = 0) {
			/*
			 * If someone is trying to sync this buffer and the
			 * buffer is hot, they may never get in.  Give up and
			 * try again.
			 */
			if (!first && bhp->ref_sync != 0) {
				--bhp->ref;
				MUTEX_UNLOCK(env, hp->mtx_hash);
				bhp = frozen_bhp = NULL;
				b_incr = b_locked = 0;
				__os_yield(env, 0, 1);
				goto retry;
			}

			/*
			 * If we're the first thread waiting on I/O, set the
			 * flag so the thread doing I/O knows to wake us up,
			 * and lock the mutex.
			 */
			if (!F_ISSET(hp, IO_WAITER)) {
				F_SET(hp, IO_WAITER);
				MUTEX_LOCK(env, hp->mtx_io);
			}
			STAT(++hp->hash_io_wait);

			/* Release the hash bucket lock. */
			MUTEX_UNLOCK(env, hp->mtx_hash);

			/* Wait for I/O to finish. */
			MUTEX_LOCK(env, hp->mtx_io);
			MUTEX_UNLOCK(env, hp->mtx_io);

			/* Re-acquire the hash bucket lock. */
			MUTEX_LOCK(env, hp->mtx_hash);
		}

		/*
		 * If the buffer was frozen before we waited for any I/O to
		 * complete and is still frozen, we will need to thaw it.
		 * Otherwise, it was thawed while we waited, and we need to
		 * search again.
		 */
		if (frozen_bhp != NULL && F_ISSET(frozen_bhp, BH_THAWED)) {
thawed:			need_free = (--frozen_bhp->ref == 0);
			b_incr = 0;
			MUTEX_UNLOCK(env, hp->mtx_hash);
			MPOOL_REGION_LOCK(env, infop);
			if (alloc_bhp != NULL) {
				__memp_free(infop, mfp, alloc_bhp);
				alloc_bhp = NULL;
			}
			if (need_free)
				SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen,
				    frozen_bhp, hq);
			MPOOL_REGION_UNLOCK(env, infop);
			bhp = frozen_bhp = NULL;
			goto retry;
		}

		/*
		 * If the buffer we wanted was frozen or thawed while we
		 * waited, we need to start again.
		 */
		if (SH_CHAIN_HASNEXT(bhp, vc) &&
		    SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off == bhp->td_off) {
			--bhp->ref;
			b_incr = 0;
			MUTEX_UNLOCK(env, hp->mtx_hash);
			bhp = frozen_bhp = NULL;
			goto retry;
		} else if (dirty && SH_CHAIN_HASNEXT(bhp, vc)) {
			ret = DB_LOCK_DEADLOCK;
			goto err;
		}

#ifdef HAVE_STATISTICS
		++mfp->stat.st_cache_hit;
#endif
		break;
	}

#ifdef HAVE_STATISTICS
	/*
	 * Update the hash bucket search statistics -- do now because our next
	 * search may be for a different bucket.
	 */
	++c_mp->stat.st_hash_searches;
	if (st_hsearch > c_mp->stat.st_hash_longest)
		c_mp->stat.st_hash_longest = st_hsearch;
	c_mp->stat.st_hash_examined += st_hsearch;
#endif

	/*
	 * There are 4 possible paths to this location:
	 *
	 * FIRST_MISS:
	 *	Didn't find the page in the hash bucket on our first pass:
	 *	bhp == NULL, alloc_bhp == NULL
	 *
	 * FIRST_FOUND:
	 *	Found the page in the hash bucket on our first pass:
	 *	bhp != NULL, alloc_bhp == NULL
	 *
	 * SECOND_FOUND:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and found the page in the hash bucket on
	 *	our second pass:
	 *	bhp != NULL, alloc_bhp != NULL
	 *
	 * SECOND_MISS:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and didn't find the page in the hash bucket
	 *	on our second pass:
	 *	bhp == NULL, alloc_bhp != NULL
	 */
	state = bhp == NULL ?
	    (alloc_bhp == NULL ? FIRST_MISS : SECOND_MISS) :
	    (alloc_bhp == NULL ? FIRST_FOUND : SECOND_FOUND);

	switch (state) {
	case FIRST_FOUND:
		/*
		 * If we are to free the buffer, then this had better be the
		 * only reference. If so, just free the buffer.  If not,
		 * complain and get out.
		 */
		if (flags == DB_MPOOL_FREE) {
			if (--bhp->ref == 0) {
				if (F_ISSET(bhp, BH_DIRTY)) {
					--hp->hash_page_dirty;
					F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
				}
				/*
				 * In a multiversion database, this page could
				 * be requested again so we have to leave it in
				 * cache for now.  It should *not* ever be
				 * requested again for modification without an
				 * intervening DB_MPOOL_CREATE or DB_MPOOL_NEW.
				 *
				 * Mark it with BH_FREED so we don't reuse the
				 * data when the page is resurrected.
				 */
				if (mvcc && (F_ISSET(bhp, BH_FROZEN) ||
				    !SH_CHAIN_SINGLETON(bhp, vc) ||
				    bhp->td_off == INVALID_ROFF ||
				    !IS_MAX_LSN(*VISIBLE_LSN(env, bhp)))) {
					F_SET(bhp, BH_FREED);
					MUTEX_UNLOCK(env, hp->mtx_hash);
					return (0);
				}
				return (__memp_bhfree(
				    dbmp, infop, hp, bhp, BH_FREE_FREEMEM));
			}
			__db_errx(env,
			    "File %s: freeing pinned buffer for page %lu",
				__memp_fns(dbmp, mfp), (u_long)*pgnoaddr);
			ret = __env_panic(env, EINVAL);
			goto err;
		}

		if (mvcc) {
			if (flags == DB_MPOOL_CREATE &&
			    F_ISSET(bhp, BH_FREED)) {
				extending = makecopy = 1;
				MUTEX_LOCK(env, mfp->mutex);
				if (*pgnoaddr > mfp->last_pgno)
					mfp->last_pgno = *pgnoaddr;
				MUTEX_UNLOCK(env, mfp->mutex);
			}

			/*
			 * With multiversion databases, we might need to
			 * allocate a new buffer into which we can copy the one
			 * that we found.  In that case, check the last buffer
			 * in the chain to see whether we can reuse an obsolete
			 * buffer.
			 *
			 * To provide snapshot isolation, we need to make sure
			 * that we've seen a buffer older than the oldest
			 * snapshot read LSN.
			 */
reuse:			if ((makecopy || frozen_bhp != NULL) && (oldest_bhp =
			    SH_CHAIN_PREV(bhp, vc, __bh)) != NULL) {
				while (SH_CHAIN_HASPREV(oldest_bhp, vc))
					oldest_bhp = SH_CHAIN_PREVP(oldest_bhp,
					    vc, __bh);

				if (oldest_bhp->ref == 0 && !BH_OBSOLETE(
				    oldest_bhp, hp->old_reader, vlsn) &&
				    (ret = __txn_oldest_reader(env,
				    &hp->old_reader)) != 0)
					goto err;

				if (BH_OBSOLETE(
				    oldest_bhp, hp->old_reader, vlsn) &&
				    oldest_bhp->ref == 0) {
					if (F_ISSET(oldest_bhp, BH_FROZEN)) {
						++oldest_bhp->ref;
						if ((ret = __memp_bh_thaw(dbmp,
						    infop, hp, oldest_bhp,
						    NULL)) != 0)
							goto err;
						goto reuse;
					} else if ((ret = __memp_bhfree(dbmp,
					    infop, hp, oldest_bhp,
					    BH_FREE_REUSE)) != 0)
						goto err;
					alloc_bhp = oldest_bhp;
				}

				DB_ASSERT(env, alloc_bhp == NULL ||
				    !F_ISSET(alloc_bhp, BH_FROZEN));
			}
		}

		/* We found the buffer or we're ready to copy -- we're done. */
		if ((!makecopy && frozen_bhp == NULL) || alloc_bhp != NULL)
			break;

		/* FALLTHROUGH */
	case FIRST_MISS:
		/*
		 * We didn't find the buffer in our first check.  Figure out
		 * if the page exists, and allocate structures so we can add
		 * the page to the buffer pool.
		 */
		MUTEX_UNLOCK(env, hp->mtx_hash);
		b_locked = 0;

		/*
		 * The buffer is not in the pool, so we don't need to free it.
		 */
		if (flags == DB_MPOOL_FREE)
			return (0);

alloc:		/*
		 * If DB_MPOOL_NEW is set, we have to allocate a page number.
		 * If neither DB_MPOOL_CREATE or DB_MPOOL_NEW is set, then
		 * it's an error to try and get a page past the end of file.
		 */
		DB_ASSERT(env, !b_locked);
		MUTEX_LOCK(env, mfp->mutex);
		switch (flags) {
		case DB_MPOOL_NEW:
			extending = 1;
			if (mfp->maxpgno != 0 &&
			    mfp->last_pgno >= mfp->maxpgno) {
				__db_errx(
				    env, "%s: file limited to %lu pages",
				    __memp_fn(dbmfp), (u_long)mfp->maxpgno);
				ret = ENOSPC;
			} else
				*pgnoaddr = mfp->last_pgno + 1;
			break;
		case DB_MPOOL_CREATE:
			if (mfp->maxpgno != 0 && *pgnoaddr > mfp->maxpgno) {
				__db_errx(
				    env, "%s: file limited to %lu pages",
				    __memp_fn(dbmfp), (u_long)mfp->maxpgno);
				ret = ENOSPC;
			} else if (!extending)
				extending = *pgnoaddr > mfp->last_pgno;
			break;
		default:
			ret = *pgnoaddr > mfp->last_pgno ? DB_PAGE_NOTFOUND : 0;
			break;
		}
		MUTEX_UNLOCK(env, mfp->mutex);
		if (ret != 0)
			goto err;

		/*
		 * !!!
		 * In the DB_MPOOL_NEW code path, infop and c_mp have
		 * not yet been initialized.
		 */
		MP_GET_REGION(dbmfp, *pgnoaddr, &infop, ret);
		if (ret != 0)
			goto err;
		c_mp = infop->primary;

		/* Allocate a new buffer header and data space. */
		if ((ret =
		    __memp_alloc(dbmp, infop, mfp, 0, NULL, &alloc_bhp)) != 0)
			goto err;
#ifdef DIAGNOSTIC
		if ((uintptr_t)alloc_bhp->buf & (sizeof(size_t) - 1)) {
			__db_errx(env,
		    "DB_MPOOLFILE->get: buffer data is NOT size_t aligned");
			ret = __env_panic(env, EINVAL);
			goto err;
		}
#endif
		/*
		 * If we are extending the file, we'll need the mfp lock
		 * again.
		 */
		if (extending)
			MUTEX_LOCK(env, mfp->mutex);

		/*
		 * DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control.  (That guarantee is interesting
		 * for DB_MPOOL_NEW, unlike DB_MPOOL_CREATE, because the caller
		 * did not specify the page number, and so, may reasonably not
		 * have any way to lock the page outside of mpool.) Regardless,
		 * if we allocate the page, and some other thread of control
		 * requests the page by number, we will not detect that and the
		 * thread of control that allocated using DB_MPOOL_NEW may not
		 * have a chance to initialize the page.  (Note: we *could*
		 * detect this case if we set a flag in the buffer header which
		 * guaranteed that no gets of the page would succeed until the
		 * reference count went to 0, that is, until the creating page
		 * put the page.)  What we do guarantee is that if two threads
		 * of control are both doing DB_MPOOL_NEW calls, they won't
		 * collide, that is, they won't both get the same page.
		 *
		 * There's a possibility that another thread allocated the page
		 * we were planning to allocate while we were off doing buffer
		 * allocation.  We can do that by making sure the page number
		 * we were going to use is still available.  If it's not, then
		 * we check to see if the next available page number hashes to
		 * the same mpool region as the old one -- if it does, we can
		 * continue, otherwise, we have to start over.
		 */
		if (flags == DB_MPOOL_NEW && *pgnoaddr != mfp->last_pgno + 1) {
			*pgnoaddr = mfp->last_pgno + 1;
			MP_GET_REGION(dbmfp, *pgnoaddr, &t_infop,ret);
			if (ret != 0)
				goto err;
			if (t_infop != infop) {
				/*
				 * flags == DB_MPOOL_NEW, so extending is set
				 * and we're holding the mfp locked.
				 */
				MUTEX_UNLOCK(env, mfp->mutex);

				MPOOL_REGION_LOCK(env, infop);
				__memp_free(infop, mfp, alloc_bhp);
				c_mp->stat.st_pages--;
				MPOOL_REGION_UNLOCK(env, infop);

				alloc_bhp = NULL;
				goto alloc;
			}
		}

		/*
		 * We released the mfp lock, so another thread might have
		 * extended the file.  Update the last_pgno and initialize
		 * the file, as necessary, if we extended the file.
		 */
		if (extending) {
			if (*pgnoaddr > mfp->last_pgno)
				mfp->last_pgno = *pgnoaddr;

			MUTEX_UNLOCK(env, mfp->mutex);
			if (ret != 0)
				goto err;
		}

		/*
		 * If we're doing copy-on-write, we will already have the
		 * buffer header.  In that case, we don't need to search again.
		 */
		if (bhp != NULL) {
			MUTEX_LOCK(env, hp->mtx_hash);
			b_locked = 1;
			break;
		}
		DB_ASSERT(env, frozen_bhp == NULL);
		goto retry;
	case SECOND_FOUND:
		/*
		 * We allocated buffer space for the requested page, but then
		 * found the page in the buffer cache on our second check.
		 * That's OK -- we can use the page we found in the pool,
		 * unless DB_MPOOL_NEW is set.  If we're about to copy-on-write,
		 * this is exactly the situation we want.
		 *
		 * For multiversion files, we may have left some pages in cache
		 * beyond the end of a file after truncating.  In that case, we
		 * would get to here with extending set.  If so, we need to
		 * insert the new page in the version chain similar to when
		 * we copy on write.
		 */
		if (extending && F_ISSET(bhp, BH_FREED))
			makecopy = 1;
		if (makecopy || frozen_bhp != NULL)
			break;

		/* Free the allocated memory, we no longer need it.  Since we
		 * can't acquire the region lock while holding the hash bucket
		 * lock, we have to release the hash bucket and re-acquire it.
		 * That's OK, because we have the buffer pinned down.
		 */
		MUTEX_UNLOCK(env, hp->mtx_hash);
		MPOOL_REGION_LOCK(env, infop);
		__memp_free(infop, mfp, alloc_bhp);
		c_mp->stat.st_pages--;
		MPOOL_REGION_UNLOCK(env, infop);
		alloc_bhp = NULL;

		/*
		 * We can't use the page we found in the pool if DB_MPOOL_NEW
		 * was set.  (For details, see the above comment beginning
		 * "DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control".)  If DB_MPOOL_NEW is set, we
		 * release our pin on this particular buffer, and try to get
		 * another one.
		 */
		if (flags == DB_MPOOL_NEW) {
			--bhp->ref;
			b_incr = b_locked = 0;
			bhp = NULL;
			goto alloc;
		}

		/* We can use the page -- get the bucket lock. */
		MUTEX_LOCK(env, hp->mtx_hash);
		break;
	case SECOND_MISS:
		/*
		 * We allocated buffer space for the requested page, and found
		 * the page still missing on our second pass through the buffer
		 * cache.  Instantiate the page.
		 */
		bhp = alloc_bhp;
		alloc_bhp = NULL;

		/*
		 * Initialize all the BH and hash bucket fields so we can call
		 * __memp_bhfree if an error occurs.
		 *
		 * Append the buffer to the tail of the bucket list and update
		 * the hash bucket's priority.
		 */
		/*lint --e{668} (flexelint: bhp cannot be NULL). */
#ifdef DIAG_MVCC
		memset(bhp, 0, SSZ(BH, align_off));
#else
		memset(bhp, 0, sizeof(BH));
#endif
		bhp->ref = 1;
		b_incr = 1;
		bhp->priority = UINT32_MAX;
		bhp->pgno = *pgnoaddr;
		bhp->mf_offset = mf_offset;
		SH_TAILQ_INSERT_TAIL(&hp->hash_bucket, bhp, hq);
		SH_CHAIN_INIT(bhp, vc);

		/* We created a new page, it starts dirty. */
		if (extending) {
			++hp->hash_page_dirty;
			F_SET(bhp, BH_DIRTY | BH_DIRTY_CREATE);
		}

		/*
		 * If we created the page, zero it out.  If we didn't create
		 * the page, read from the backing file.
		 *
		 * !!!
		 * DB_MPOOL_NEW doesn't call the pgin function.
		 *
		 * If DB_MPOOL_CREATE is used, then the application's pgin
		 * function has to be able to handle pages of 0's -- if it
		 * uses DB_MPOOL_NEW, it can detect all of its page creates,
		 * and not bother.
		 *
		 * If we're running in diagnostic mode, smash any bytes on the
		 * page that are unknown quantities for the caller.
		 *
		 * Otherwise, read the page into memory, optionally creating it
		 * if DB_MPOOL_CREATE is set.
		 */
		if (extending) {
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
			    PROT_READ | PROT_WRITE);
			if (mfp->clear_len == DB_CLEARLEN_NOTSET)
				memset(bhp->buf, 0, mfp->stat.st_pagesize);
			else {
				memset(bhp->buf, 0, mfp->clear_len);
#if defined(DIAGNOSTIC) || defined(UMRW)
				memset(bhp->buf + mfp->clear_len, CLEAR_BYTE,
				    mfp->stat.st_pagesize - mfp->clear_len);
#endif
			}

			if (flags == DB_MPOOL_CREATE && mfp->ftype != 0)
				F_SET(bhp, BH_CALLPGIN);

			STAT(++mfp->stat.st_page_create);
		} else {
			F_SET(bhp, BH_TRASH);
			STAT(++mfp->stat.st_cache_miss);
		}

		/* Increment buffer count referenced by MPOOLFILE. */
		MUTEX_LOCK(env, mfp->mutex);
		++mfp->block_cnt;
		MUTEX_UNLOCK(env, mfp->mutex);
	}

	DB_ASSERT(env, bhp != NULL);
	DB_ASSERT(env, bhp->ref != 0);

	/* We've got a buffer header we're re-instantiating. */
	if (frozen_bhp != NULL) {
		DB_ASSERT(env, alloc_bhp != NULL);

		/*
		 * If the empty buffer has been filled in the meantime, don't
		 * overwrite it.
		 */
		if (F_ISSET(frozen_bhp, BH_THAWED))
			goto thawed;
		else {
			if ((ret = __memp_bh_thaw(dbmp, infop, hp,
			    frozen_bhp, alloc_bhp)) != 0)
				goto err;
			bhp = alloc_bhp;
		}

		frozen_bhp = alloc_bhp = NULL;

		/*
		 * If we're updating a buffer that was frozen, we have to go
		 * through all of that again to allocate another buffer to hold
		 * the new copy.
		 */
		if (makecopy) {
			MUTEX_UNLOCK(env, hp->mtx_hash);
			b_locked = 0;
			goto alloc;
		}
	}

	/*
	 * BH_TRASH --
	 * The buffer we found may need to be filled from the disk.
	 *
	 * It's possible for the read function to fail, which means we fail as
	 * well.  Note, the __memp_pgread() function discards and reacquires
	 * the hash lock, so the buffer must be pinned down so that it cannot
	 * move and its contents are unchanged.  Discard the buffer on failure
	 * unless another thread is waiting on our I/O to complete.  It's OK to
	 * leave the buffer around, as the waiting thread will see the BH_TRASH
	 * flag set, and will also attempt to discard it.  If there's a waiter,
	 * we need to decrement our reference count.
	 */
	if (F_ISSET(bhp, BH_TRASH) &&
	    (ret = __memp_pgread(dbmfp,
	    hp, bhp, LF_ISSET(DB_MPOOL_CREATE) ? 1 : 0)) != 0)
		goto err;

	/*
	 * BH_CALLPGIN --
	 * The buffer was processed for being written to disk, and now has
	 * to be re-converted for use.
	 */
	if (F_ISSET(bhp, BH_CALLPGIN)) {
		MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
		    PROT_READ | PROT_WRITE);
		if ((ret = __memp_pg(dbmfp, bhp, 1)) != 0)
			goto err;
		F_CLR(bhp, BH_CALLPGIN);
	}

	/* Copy-on-write. */
	if (makecopy && state != SECOND_MISS) {
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		DB_ASSERT(env, bhp != NULL);
		DB_ASSERT(env, alloc_bhp != NULL);
		DB_ASSERT(env, alloc_bhp != bhp);

		if (bhp->ref == 1)
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
			    PROT_READ);

		alloc_bhp->ref = 1;
		alloc_bhp->ref_sync = 0;
		alloc_bhp->flags = F_ISSET(bhp, BH_DIRTY | BH_DIRTY_CREATE);
		F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
		alloc_bhp->priority = bhp->priority;
		alloc_bhp->pgno = bhp->pgno;
		alloc_bhp->mf_offset = bhp->mf_offset;
		alloc_bhp->td_off = INVALID_ROFF;
		if (txn != NULL &&
		    (ret = __memp_bh_settxn(dbmp, mfp, alloc_bhp, td)) != 0)
			goto err;
		if (extending) {
			memset(alloc_bhp->buf, 0, mfp->stat.st_pagesize);
			F_SET(alloc_bhp, BH_DIRTY_CREATE);
		} else
			memcpy(alloc_bhp->buf, bhp->buf, mfp->stat.st_pagesize);

		SH_CHAIN_INSERT_AFTER(bhp, alloc_bhp, vc, __bh);
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket,
		    bhp, alloc_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
		if (--bhp->ref == 0) {
			bhp->priority = c_mp->lru_count;
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize, 0);
		}
		bhp = alloc_bhp;

		if (alloc_bhp != oldest_bhp) {
			MUTEX_LOCK(env, mfp->mutex);
			++mfp->block_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
		}

		alloc_bhp = NULL;
	} else if (mvcc && extending && txn != NULL &&
	    (ret = __memp_bh_settxn(dbmp, mfp, bhp, td)) != 0)
		goto err;

	if ((dirty || edit || extending) && !F_ISSET(bhp, BH_DIRTY)) {
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		++hp->hash_page_dirty;
		F_SET(bhp, BH_DIRTY);
	}

	/*
	 * If we're the only reference, update buffer priority.  We may be
	 * about to release the hash bucket lock, and everything should be
	 * correct, first.  (We've already done this work if we created the
	 * buffer, so there is no need to do it again.)
	 */
	if (state != SECOND_MISS && bhp->ref == 1) {
		bhp->priority = UINT32_MAX;
		if (SH_CHAIN_SINGLETON(bhp, vc)) {
			if (bhp != SH_TAILQ_LAST(&hp->hash_bucket, hq, __bh)) {
				SH_TAILQ_REMOVE(&hp->hash_bucket,
				    bhp, hq, __bh);
				SH_TAILQ_INSERT_TAIL(&hp->hash_bucket, bhp, hq);
			}
		}
	}

	MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize, PROT_READ |
	    (dirty || edit || extending || F_ISSET(bhp, BH_DIRTY) ?
	    PROT_WRITE : 0));

#ifdef DIAGNOSTIC
	{
	BH *next_bhp = SH_CHAIN_NEXT(bhp, vc, __bh);

	DB_ASSERT(env, !mfp->multiversion ||
	    !F_ISSET(bhp, BH_DIRTY) || next_bhp == NULL);

	DB_ASSERT(env, !mvcc || edit || read_lsnp == NULL ||
	    bhp->td_off == INVALID_ROFF || BH_OWNED_BY(env, bhp, txn) ||
	    (BH_VISIBLE(env, bhp, read_lsnp, vlsn) &&
	    (next_bhp == NULL || F_ISSET(next_bhp, BH_FROZEN) ||
	    (next_bhp->td_off != INVALID_ROFF &&
	    (BH_OWNER(env, next_bhp)->status != TXN_COMMITTED ||
	    !BH_VISIBLE(env, next_bhp, read_lsnp, vlsn))))));
	}
#endif

	MUTEX_UNLOCK(env, hp->mtx_hash);
	/*
	 * Record this pin for this thread.  Holding the page pinned
	 * without recording the pin is ok since we do not recover from
	 * a death from within the library itself.
	 */
	if (ip != NULL) {
		reginfo = env->reginfo;
		if (ip->dbth_pincount == ip->dbth_pinmax) {
			pinmax = ip->dbth_pinmax;
			if ((ret = __env_alloc(reginfo,
			    2 * pinmax * sizeof(PIN_LIST), &list)) != 0)
				goto err;

			memcpy(list, R_ADDR(reginfo, ip->dbth_pinlist),
			    pinmax * sizeof(PIN_LIST));
			memset(&list[pinmax], 0, pinmax * sizeof(PIN_LIST));
			list_off = R_OFFSET(reginfo, list);
			list = R_ADDR(reginfo, ip->dbth_pinlist);
			ip->dbth_pinmax = 2 * pinmax;
			ip->dbth_pinlist = list_off;
			if (list != ip->dbth_pinarray)
				__env_alloc_free(reginfo, list);
		}
		list = R_ADDR(reginfo, ip->dbth_pinlist);
		for (lp = list; lp < &list[ip->dbth_pinmax]; lp++)
			if (lp->b_ref == INVALID_ROFF)
				break;

		ip->dbth_pincount++;
		lp->b_ref = R_OFFSET(infop, bhp);
		lp->region = (int)(infop - dbmp->reginfo);
	}

#ifdef DIAGNOSTIC
	/* Update the file's pinned reference count. */
	MPOOL_SYSTEM_LOCK(env);
	++dbmfp->pinref;
	MPOOL_SYSTEM_UNLOCK(env);

	/*
	 * We want to switch threads as often as possible, and at awkward
	 * times.  Yield every time we get a new page to ensure contention.
	 */
	if (F_ISSET(env->dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif

	DB_ASSERT(env, alloc_bhp == NULL);

	*(void **)addrp = bhp->buf;
	return (0);

err:	/*
	 * Discard our reference.  If we're the only reference, discard the
	 * the buffer entirely.  If we held a reference to a buffer, we are
	 * also still holding the hash bucket mutex.
	 */
	if (b_incr || frozen_bhp != NULL) {
		if (!b_locked) {
			MUTEX_LOCK(env, hp->mtx_hash);
			b_locked = 1;
		}
		if (frozen_bhp != NULL)
			--frozen_bhp->ref;
		if (b_incr && bhp != frozen_bhp)
			--bhp->ref;
	}
	if (b_locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);

	/* If alloc_bhp is set, free the memory. */
	if (alloc_bhp != NULL) {
		MPOOL_REGION_LOCK(env, infop);
		__memp_free(infop, mfp, alloc_bhp);
		c_mp->stat.st_pages--;
		MPOOL_REGION_UNLOCK(env, infop);
	}

	return (ret);
}
