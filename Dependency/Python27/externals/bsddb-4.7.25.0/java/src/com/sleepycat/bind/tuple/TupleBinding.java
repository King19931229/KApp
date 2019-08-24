/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2008 Oracle.  All rights reserved.
 *
 * $Id: TupleBinding.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.bind.tuple;

import java.util.HashMap;
import java.util.Map;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.DatabaseEntry;

/**
 * An abstract <code>EntryBinding</code> that treats a key or data entry as a
 * tuple; it includes predefined bindings for Java primitive types.
 *
 * <p>This class takes care of converting the entries to/from {@link
 * TupleInput} and {@link TupleOutput} objects.  Its two abstract methods must
 * be implemented by a concrete subclass to convert between tuples and key or
 * data objects.</p>
 * <ul>
 * <li> {@link #entryToObject(TupleInput)} </li>
 * <li> {@link #objectToEntry(Object,TupleOutput)} </li>
 * </ul>
 *
 * <p>For key or data entries which are Java primitive classes (String,
 * Integer, etc) {@link #getPrimitiveBinding} may be used to return a built in
 * tuple binding.  A custom tuple binding for these types is not needed.
 * <em>Note:</em> {@link #getPrimitiveBinding} returns bindings that do not
 * sort negative floating point numbers correctly by default.  See {@link
 * SortedFloatBinding} and {@link SortedDoubleBinding} for details.</p>
 *
 * <p>When a tuple binding is used as a key binding, it produces key values
 * with a reasonable default sort order.  For more information on the default
 * sort order, see {@link com.sleepycat.bind.tuple.TupleOutput}.</p>
 *
 * @author Mark Hayes
 */
public abstract class TupleBinding extends TupleBase implements EntryBinding {

    private static final Map primitives = new HashMap();
    static {
        addPrimitive(String.class, String.class, new StringBinding());
        addPrimitive(Character.class, Character.TYPE, new CharacterBinding());
        addPrimitive(Boolean.class, Boolean.TYPE, new BooleanBinding());
        addPrimitive(Byte.class, Byte.TYPE, new ByteBinding());
        addPrimitive(Short.class, Short.TYPE, new ShortBinding());
        addPrimitive(Integer.class, Integer.TYPE, new IntegerBinding());
        addPrimitive(Long.class, Long.TYPE, new LongBinding());
        addPrimitive(Float.class, Float.TYPE, new FloatBinding());
        addPrimitive(Double.class, Double.TYPE, new DoubleBinding());
    }

    private static void addPrimitive(Class cls1, Class cls2,
                                     TupleBinding binding) {
        primitives.put(cls1, binding);
        primitives.put(cls2, binding);
    }

    /**
     * Creates a tuple binding.
     */
    public TupleBinding() {
    }

    // javadoc is inherited
    public Object entryToObject(DatabaseEntry entry) {

        return entryToObject(entryToInput(entry));
    }

    // javadoc is inherited
    public void objectToEntry(Object object, DatabaseEntry entry) {

        TupleOutput output = getTupleOutput(object);
        objectToEntry(object, output);
        outputToEntry(output, entry);
    }

    /**
     * Constructs a key or data object from a {@link TupleInput} entry.
     *
     * @param input is the tuple key or data entry.
     *
     * @return the key or data object constructed from the entry.
     */
    public abstract Object entryToObject(TupleInput input);

    /**
     * Converts a key or data object to a tuple entry.
     *
     * @param object is the key or data object.
     *
     * @param output is the tuple entry to which the key or data should be
     * written.
     */
    public abstract void objectToEntry(Object object, TupleOutput output);

    /**
     * Creates a tuple binding for a primitive Java class.  The following
     * Java classes are supported.
     * <ul>
     * <li><code>String</code></li>
     * <li><code>Character</code></li>
     * <li><code>Boolean</code></li>
     * <li><code>Byte</code></li>
     * <li><code>Short</code></li>
     * <li><code>Integer</code></li>
     * <li><code>Long</code></li>
     * <li><code>Float</code></li>
     * <li><code>Double</code></li>
     * </ul>
     *
     * <p><em>Note:</em> {@link #getPrimitiveBinding} returns bindings that do
     * not sort negative floating point numbers correctly by default.  See
     * {@link SortedFloatBinding} and {@link SortedDoubleBinding} for
     * details.</p>
     *
     * @param cls is the primitive Java class.
     *
     * @return a new binding for the primitive class or null if the cls
     * parameter is not one of the supported classes.
     */
    public static TupleBinding getPrimitiveBinding(Class cls) {

        return (TupleBinding) primitives.get(cls);
    }
}
