/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 *
 * $Id: TestKeyAssigner.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.collections.test;

import com.sleepycat.bind.RecordNumberBinding;
import com.sleepycat.collections.PrimaryKeyAssigner;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;

/**
 * @author Mark Hayes
 */
class TestKeyAssigner implements PrimaryKeyAssigner {

    private byte next = 1;
    private boolean isRecNum;

    TestKeyAssigner(boolean isRecNum) {

        this.isRecNum = isRecNum;
    }

    public void assignKey(DatabaseEntry keyData)
        throws DatabaseException {

        if (isRecNum) {
            RecordNumberBinding.recordNumberToEntry(next, keyData);
        } else {
            keyData.setData(new byte[] { next }, 0, 1);
        }
        next += 1;
    }

    void reset() {

        next = 1;
    }
}
