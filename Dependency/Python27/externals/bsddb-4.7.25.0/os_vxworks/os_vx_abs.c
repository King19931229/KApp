/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: os_vx_abs.c 63573 2008-05-23 21:43:21Z trent.nelson $
 */

#include "db_config.h"

#include "db_int.h"
#include "iosLib.h"

/*
 * __os_abspath --
 *	Return if a path is an absolute path.
 */
int
__os_abspath(path)
	const char *path;
{
	DEV_HDR *dummy;
	char *ptail;

	/*
	 * VxWorks devices can be rooted at any name at all.
	 * Use iosDevFind() to see if name matches any of our devices.
	 */
	if ((dummy = iosDevFind((char *)path, &ptail)) == NULL)
		return (0);
	/*
	 * If the routine used a device, then ptail points to the
	 * rest and we are an abs path.
	 */
	if (ptail != path)
		return (1);
	/*
	 * If the path starts with a '/', then we are an absolute path,
	 * using the host machine, otherwise we are not.
	 */
	return (path[0] == '/');
}
