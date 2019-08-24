/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 *
 * $Id: FastOutputStreamTest.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.util.test;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.test.SharedTestUtils;

/**
 * @author Mark Hayes
 */
public class FastOutputStreamTest extends TestCase {

    public static void main(String[] args)
        throws Exception {

        junit.framework.TestResult tr =
            junit.textui.TestRunner.run(suite());
        if (tr.errorCount() > 0 ||
            tr.failureCount() > 0) {
            System.exit(1);
        } else {
            System.exit(0);
        }
    }

    public static Test suite()
        throws Exception {

        TestSuite suite = new TestSuite(FastOutputStreamTest.class);
        return suite;
    }

    public FastOutputStreamTest(String name) {

        super(name);
    }

    public void setUp() {

        SharedTestUtils.printTestName("FastOutputStreamTest." + getName());
    }

    public void testBufferSizing()
        throws Exception {

        FastOutputStream fos = new FastOutputStream();
        assertEquals
            (FastOutputStream.DEFAULT_INIT_SIZE, fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 2X+1 */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 2) + 1,
             fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 4X+3 = (2(2X+1) + 1) */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 4) + 3,
             fos.getBufferBytes().length);
    }
}
