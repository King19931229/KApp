/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999,2008 Oracle.  All rights reserved.
 *
 * $Id: globals.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * brew_bdb_begin --
 *	Initialize the BREW port of Berkeley DB.
 */
int
brew_bdb_begin()
{
	void *p;

	/*
	 * The BREW ARM compiler can't handle statics or globals, so we have
	 * store them off the AEEApplet and initialize them in in-line code.
	 */
	p = ((BDBApp *)GETAPPINSTANCE())->db_global_values;
	if (p == NULL) {
		if ((p = malloc(sizeof(DB_GLOBALS))) == NULL)
			return (ENOMEM);
		memset(p, 0, sizeof(DB_GLOBALS));

		((BDBApp *)GETAPPINSTANCE())->db_global_values = p;

		TAILQ_INIT(&DB_GLOBAL(envq));
		DB_GLOBAL(db_line) =
		    "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";
	}
	return (0);
}

/*
 * brew_bdb_end --
 *	Close down the BREW port of Berkeley DB.
 */
void
brew_bdb_end()
{
	void *p;

	p = ((BDBApp *)GETAPPINSTANCE())->db_global_values;

	free(p);
}
