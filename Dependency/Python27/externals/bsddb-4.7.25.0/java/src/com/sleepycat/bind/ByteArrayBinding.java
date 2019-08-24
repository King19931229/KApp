/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2008 Oracle.  All rights reserved.
 *
 * $Id: ByteArrayBinding.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.bind;

import com.sleepycat.db.DatabaseEntry;

/**
 * A pass-through <code>EntryBinding</code> that uses the entry's byte array as
 * the key or data object.
 *
 * @author Mark Hayes
 */
public class ByteArrayBinding implements EntryBinding {

    /*
     * We can return the same byte[] for 0 length arrays.
     */
    private static byte[] ZERO_LENGTH_BYTE_ARRAY = new byte[0];

    /**
     * Creates a byte array binding.
     */
    public ByteArrayBinding() {
    }

    // javadoc is inherited
    public Object entryToObject(DatabaseEntry entry) {

	int len = entry.getSize();
	if (len == 0) {
	    return ZERO_LENGTH_BYTE_ARRAY;
	} else {
	    byte[] bytes = new byte[len];
	    System.arraycopy(entry.getData(), entry.getOffset(),
			     bytes, 0, bytes.length);
	    return bytes;
	}
    }

    // javadoc is inherited
    public void objectToEntry(Object object, DatabaseEntry entry) {

        byte[] bytes = (byte[]) object;
        entry.setData(bytes, 0, bytes.length);
    }
}
