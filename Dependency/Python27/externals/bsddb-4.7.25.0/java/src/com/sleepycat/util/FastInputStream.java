/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2008 Oracle.  All rights reserved.
 *
 * $Id: FastInputStream.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.util;

import java.io.IOException;
import java.io.InputStream;

/**
 * A replacement for ByteArrayInputStream that does not synchronize every
 * byte read.
 *
 * <p>This class extends {@link InputStream} and its <code>read()</code>
 * methods allow it to be used as a standard input stream.  In addition, it
 * provides <code>readFast()</code> methods that are not declared to throw
 * <code>IOException</code>.  <code>IOException</code> is never thrown by this
 * class.</p>
 *
 * @author Mark Hayes
 */
public class FastInputStream extends InputStream {

    protected int len;
    protected int off;
    protected int mark;
    protected byte[] buf;

    /**
     * Creates an input stream.
     *
     * @param buffer the data to read.
     */
    public FastInputStream(byte[] buffer) {

        buf = buffer;
        len = buffer.length;
    }

    /**
     * Creates an input stream.
     *
     * @param buffer the data to read.
     *
     * @param offset the byte offset at which to begin reading.
     *
     * @param length the number of bytes to read.
     */
    public FastInputStream(byte[] buffer, int offset, int length) {

        buf = buffer;
        off = offset;
        len = offset + length;
    }

    // --- begin ByteArrayInputStream compatible methods ---

    public int available() {

        return len - off;
    }

    public boolean markSupported() {

        return true;
    }

    public void mark(int readLimit) {

        mark = off;
    }

    public void reset() {

        off = mark;
    }

    public long skip(long count) {

        int myCount = (int) count;
        if (myCount + off > len) {
            myCount = len - off;
        }
        skipFast(myCount);
        return myCount;
    }

    public int read() throws IOException {

        return readFast();
    }

    public int read(byte[] toBuf) throws IOException {

        return readFast(toBuf, 0, toBuf.length);
    }

    public int read(byte[] toBuf, int offset, int length) throws IOException {

        return readFast(toBuf, offset, length);
    }

    // --- end ByteArrayInputStream compatible methods ---

    /**
     * Equivalent to <code>skip()<code> but takes an int parameter instead of a
     * long, and does not check whether the count given is larger than the
     * number of remaining bytes.
     * @see #skip(long)
     */
    public final void skipFast(int count) {
        off += count;
    }

    /**
     * Equivalent to <code>read()<code> but does not throw
     * <code>IOException</code>.
     * @see #read()
     */
    public final int readFast() {

        return (off < len) ? (buf[off++] & 0xff) : (-1);
    }

    /**
     * Equivalent to <code>read(byte[])<code> but does not throw
     * <code>IOException</code>.
     * @see #read(byte[])
     */
    public final int readFast(byte[] toBuf) {

        return readFast(toBuf, 0, toBuf.length);
    }

    /**
     * Equivalent to <code>read(byte[],int,int)<code> but does not throw
     * <code>IOException</code>.
     * @see #read(byte[],int,int)
     */
    public final int readFast(byte[] toBuf, int offset, int length) {

        int avail = len - off;
        if (avail <= 0) {
            return -1;
        }
        if (length > avail) {
            length = avail;
        }
        System.arraycopy(buf, off, toBuf, offset, length);
        off += length;
        return length;
    }

    /**
     * Returns the underlying data being read.
     *
     * @return the underlying data.
     */
    public final byte[] getBufferBytes() {

        return buf;
    }

    /**
     * Returns the offset at which data is being read from the buffer.
     *
     * @return the offset at which data is being read.
     */
    public final int getBufferOffset() {

        return off;
    }

    /**
     * Returns the end of the buffer being read.
     *
     * @return the end of the buffer.
     */
    public final int getBufferLength() {

        return len;
    }
}
