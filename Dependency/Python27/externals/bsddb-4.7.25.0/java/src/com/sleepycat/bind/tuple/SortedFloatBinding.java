/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2008 Oracle.  All rights reserved.
 *
 * $Id: SortedFloatBinding.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.db.DatabaseEntry;

/**
 * A concrete <code>TupleBinding</code> for a <code>Float</code> primitive
 * wrapper or a <code>float</code> primitive.
 *
 * <p>This class produces byte array values that by default (without a custom
 * comparator) sort correctly, including sorting of negative values.
 * Therefore, this class should normally be used instead of {@link
 * FloatBinding} which does not by default support sorting of negative values.
 * Please note that:</p>
 * <ul>
 * <li>The byte array (stored) formats used by {@link FloatBinding} and
 * {@link SortedFloatBinding} are different and incompatible.  They are not
 * interchangeable once data has been stored.</li>
 * <li>An instance of {@link FloatBinding}, not {@link SortedFloatBinding}, is
 * returned by {@link TupleBinding#getPrimitiveBinding} method.  Therefore, to
 * use {@link SortedFloatBinding}, {@link TupleBinding#getPrimitiveBinding}
 * should not be called.</li>
 * </ul>
 *
 * <p>There are two ways to use this class:</p>
 * <ol>
 * <li>When using the {@link com.sleepycat.db} package directly, the static
 * methods in this class can be used to convert between primitive values and
 * {@link DatabaseEntry} objects.</li>
 * <li>When using the {@link com.sleepycat.collections} package, an instance of
 * this class can be used with any stored collection.</li>
 * </ol>
 */
public class SortedFloatBinding extends TupleBinding {

    /* javadoc is inherited */
    public Object entryToObject(TupleInput input) {

        return new Float(input.readSortedFloat());
    }

    /* javadoc is inherited */
    public void objectToEntry(Object object, TupleOutput output) {

        output.writeSortedFloat(((Number) object).floatValue());
    }

    /* javadoc is inherited */
    protected TupleOutput getTupleOutput(Object object) {

        return FloatBinding.sizedOutput();
    }

    /**
     * Converts an entry buffer into a simple <code>float</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static float entryToFloat(DatabaseEntry entry) {

        return entryToInput(entry).readSortedFloat();
    }

    /**
     * Converts a simple <code>float</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void floatToEntry(float val, DatabaseEntry entry) {

        outputToEntry(FloatBinding.sizedOutput().writeSortedFloat(val), entry);
    }
}
