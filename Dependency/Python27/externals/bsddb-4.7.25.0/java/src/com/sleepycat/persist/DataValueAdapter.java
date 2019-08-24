/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 *
 * $Id: DataValueAdapter.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.persist;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.DatabaseEntry;

/**
 * A ValueAdapter where the "value" is the data, although the data in this case
 * is the primary key in a KeysIndex.
 *
 * @author Mark Hayes
 */
class DataValueAdapter<V> implements ValueAdapter<V> {

    private EntryBinding dataBinding;

    DataValueAdapter(Class<V> keyClass, EntryBinding dataBinding) {
        this.dataBinding = dataBinding;
    }

    public DatabaseEntry initKey() {
        return new DatabaseEntry();
    }

    public DatabaseEntry initPKey() {
        return null;
    }

    public DatabaseEntry initData() {
        return new DatabaseEntry();
    }

    public void clearEntries(DatabaseEntry key,
                             DatabaseEntry pkey,
                             DatabaseEntry data) {
        key.setData(null);
        data.setData(null);
    }

    public V entryToValue(DatabaseEntry key,
                          DatabaseEntry pkey,
                          DatabaseEntry data) {
        return (V) dataBinding.entryToObject(data);
    }

    public void valueToData(V value, DatabaseEntry data) {
        throw new UnsupportedOperationException
            ("Cannot change the data in a key-only index");
    }
}
