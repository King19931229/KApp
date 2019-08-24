/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

/**
Log statistics for a database environment.
*/
public class LogStats {
    // no public constructor
    /* package */ LogStats() {}

    private int st_magic;
    /**
    The magic number that identifies a file as a log file.
    */
    public int getMagic() {
        return st_magic;
    }

    private int st_version;
    /**
    The version of the log file type.
    */
    public int getVersion() {
        return st_version;
    }

    private int st_mode;
    /**
    The mode of any created log files.
    */
    public int getMode() {
        return st_mode;
    }

    private int st_lg_bsize;
    /**
    The in-memory log record cache size.
    */
    public int getLgBSize() {
        return st_lg_bsize;
    }

    private int st_lg_size;
    /**
    The current log file size.
    */
    public int getLgSize() {
        return st_lg_size;
    }

    private int st_wc_bytes;
    /**
    The number of bytes over and above {@link com.sleepycat.db.LogStats#getWcMbytes LogStats.getWcMbytes}
    written to this log since the last checkpoint.
    */
    public int getWcBytes() {
        return st_wc_bytes;
    }

    private int st_wc_mbytes;
    /**
    The number of megabytes written to this log since the last checkpoint.
    */
    public int getWcMbytes() {
        return st_wc_mbytes;
    }

    private int st_record;
    /** The number of records written to this log. **/
    public int getRecord() {
        return st_record;
    }

    private int st_w_bytes;
    /**
    The number of bytes over and above st_w_mbytes written to this log.
    */
    public int getWBytes() {
        return st_w_bytes;
    }

    private int st_w_mbytes;
    /**
    The number of megabytes written to this log.
    */
    public int getWMbytes() {
        return st_w_mbytes;
    }

    private int st_wcount;
    /**
    The number of times the log has been written to disk.
    */
    public int getWCount() {
        return st_wcount;
    }

    private int st_wcount_fill;
    /**
    The number of times the log has been written to disk because the
    in-memory log record cache filled up.
    */
    public int getWCountFill() {
        return st_wcount_fill;
    }

    private int st_rcount;
    /** The number of times the log has been read from disk. **/
    public int getRCount() {
        return st_rcount;
    }

    private int st_scount;
    /**
    The number of times the log has been flushed to disk.
    */
    public int getSCount() {
        return st_scount;
    }

    private int st_region_wait;
    /**
    The number of times that a thread of control was forced to wait
    before obtaining the region lock.
    */
    public int getRegionWait() {
        return st_region_wait;
    }

    private int st_region_nowait;
    /**
    The number of times that a thread of control was able to obtain the
    region lock without waiting.
    */
    public int getRegionNowait() {
        return st_region_nowait;
    }

    private int st_cur_file;
    /**
    The current log file number.
    */
    public int getCurFile() {
        return st_cur_file;
    }

    private int st_cur_offset;
    /**
    The byte offset in the current log file.
    */
    public int getCurOffset() {
        return st_cur_offset;
    }

    private int st_disk_file;
    /**
    The log file number of the last record known to be on disk.
    */
    public int getDiskFile() {
        return st_disk_file;
    }

    private int st_disk_offset;
    /**
    The byte offset of the last record known to be on disk.
    */
    public int getDiskOffset() {
        return st_disk_offset;
    }

    private int st_maxcommitperflush;
    /**
    The maximum number of commits contained in a single log flush.
    */
    public int getMaxCommitperflush() {
        return st_maxcommitperflush;
    }

    private int st_mincommitperflush;
    /**
    The minimum number of commits contained in a single log flush that
    contained a commit.
    */
    public int getMinCommitperflush() {
        return st_mincommitperflush;
    }

    private int st_regsize;
    /**
    The size of the region.
    */
    public int getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the LogStats class has a toString method that lists
    all the data fields.
    */
    public String toString() {
        return "LogStats:"
            + "\n  st_magic=" + st_magic
            + "\n  st_version=" + st_version
            + "\n  st_mode=" + st_mode
            + "\n  st_lg_bsize=" + st_lg_bsize
            + "\n  st_lg_size=" + st_lg_size
            + "\n  st_wc_bytes=" + st_wc_bytes
            + "\n  st_wc_mbytes=" + st_wc_mbytes
            + "\n  st_record=" + st_record
            + "\n  st_w_bytes=" + st_w_bytes
            + "\n  st_w_mbytes=" + st_w_mbytes
            + "\n  st_wcount=" + st_wcount
            + "\n  st_wcount_fill=" + st_wcount_fill
            + "\n  st_rcount=" + st_rcount
            + "\n  st_scount=" + st_scount
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_cur_file=" + st_cur_file
            + "\n  st_cur_offset=" + st_cur_offset
            + "\n  st_disk_file=" + st_disk_file
            + "\n  st_disk_offset=" + st_disk_offset
            + "\n  st_maxcommitperflush=" + st_maxcommitperflush
            + "\n  st_mincommitperflush=" + st_mincommitperflush
            + "\n  st_regsize=" + st_regsize
            ;
    }
}
