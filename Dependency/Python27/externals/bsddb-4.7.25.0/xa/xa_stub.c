/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996,2008 Oracle.  All rights reserved.
 *
 * $Id: xa_stub.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/txn.h"

/*
 * If the library wasn't compiled with XA support, various routines
 * aren't available.  Stub them here, returning an appropriate error.
 */
static int __db_noxa __P((DB_ENV *));

/*
 * __db_noxa --
 *	Error when a Berkeley DB build doesn't include XA support.
 */
static int
__db_noxa(dbenv)
	DB_ENV *dbenv;
{
	__db_errx(dbenv->env,
	    "library build did not include support for XA");
	return (DB_OPNOTSUP);
}

int
__db_xa_create(dbp)
	DB *dbp;
{
	return (__db_noxa(dbp->dbenv));
}
