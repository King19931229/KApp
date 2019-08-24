/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbUtil;

/**
Statistics returned by a {@link Database#compact} operation.
*/
public class CompactStats {
    // no public constructor
    /* package */ CompactStats() {}

    /* package */
    CompactStats(int fillpercent, int timeout, int pages) {
        this.compact_fillpercent = fillpercent;
        this.compact_timeout = timeout;
        this.compact_pages = pages;
    }

    private int compact_fillpercent;
    /* package */ int getFillPercent() {
        return compact_fillpercent;
    }

    private int compact_timeout;
    /* package */ int getTimeout() {
        return compact_timeout;
    }

    private int compact_pages;
    /* package */ int getPages() {
        return compact_pages;
    }

    private int compact_pages_free;
    /**
Return the the number of database pages free during
    the compaction phase.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the number of database pages free during
    the compaction phase.
    **/
    public int getPagesFree() {
        return compact_pages_free;
    }

    private int compact_pages_examine;
    /**
Return the the number of database pages reviewed
    during the compaction phase.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the number of database pages reviewed
    during the compaction phase.
    **/
    public int getPagesExamine() {
        return compact_pages_examine;
    }

    private int compact_levels;
    /**
Return the the number of levels removed from the
    Btree or Recno database during the compaction phase.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the number of levels removed from the
    Btree or Recno database during the compaction phase.
    **/
    public int getLevels() {
        return compact_levels;
    }

    private int compact_deadlock;
    /**
Return the if no transaction parameter was specified to
    {@link Database#compact}, the number of deadlocks
    which occurred..
<p>
This method may be called at any time during the life of the application.
<p>
@return
The if no transaction parameter was specified to
    {@link Database#compact}, the number of deadlocks
    which occurred..
    **/
    public int getDeadlock() {
        return compact_deadlock;
    }

    private int compact_pages_truncated;
    /**
Return the the number of database pages returned
    to the filesystem.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the number of database pages returned
    to the filesystem.
    **/
    public int getPagesTruncated() {
        return compact_pages_truncated;
    }

    private int compact_truncate;
    /* package */ int getTruncate() {
        return compact_truncate;
    }

    /**
    For convenience, the CompactStats class has a toString method that lists
    all the data fields.
    */
    public String toString() {
        return "CompactStats:"
            + "\n  compact_fillpercent=" + compact_fillpercent
            + "\n  compact_timeout=" + compact_timeout
            + "\n  compact_pages=" + compact_pages
            + "\n  compact_pages_free=" + compact_pages_free
            + "\n  compact_pages_examine=" + compact_pages_examine
            + "\n  compact_levels=" + compact_levels
            + "\n  compact_deadlock=" + compact_deadlock
            + "\n  compact_pages_truncated=" + compact_pages_truncated
            + "\n  compact_truncate=" + compact_truncate
            ;
    }
}
