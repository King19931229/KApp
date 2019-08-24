/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000,2008 Oracle.  All rights reserved.
 *
 * $Id: SerialSerialKeyCreator.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.bind.serial;

import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.ForeignKeyNullifier;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.db.SecondaryKeyCreator;

/**
 * A abstract key creator that uses a serial key and a serial data entry.
 * This class takes care of serializing and deserializing the key and data
 * entry automatically.
 * The following abstract method must be implemented by a concrete subclass
 * to create the index key using these objects
 * <ul>
 * <li> {@link #createSecondaryKey(Object,Object)} </li>
 * </ul>
 * <p>If {@link com.sleepycat.db.ForeignKeyDeleteAction#NULLIFY} was
 * specified when opening the secondary database, the following method must be
 * overridden to nullify the foreign index key.  If NULLIFY was not specified,
 * this method need not be overridden.</p>
 * <ul>
 * <li> {@link #nullifyForeignKey(Object)} </li>
 * </ul>
 *
 * @author Mark Hayes
 */
public abstract class SerialSerialKeyCreator
    implements SecondaryKeyCreator, ForeignKeyNullifier {

    protected SerialBinding primaryKeyBinding;
    protected SerialBinding dataBinding;
    protected SerialBinding indexKeyBinding;

    /**
     * Creates a serial-serial key creator.
     *
     * @param classCatalog is the catalog to hold shared class information and
     * for a database should be a {@link StoredClassCatalog}.
     *
     * @param primaryKeyClass is the primary key base class.
     *
     * @param dataClass is the data base class.
     *
     * @param indexKeyClass is the index key base class.
     */
    public SerialSerialKeyCreator(ClassCatalog classCatalog,
                                  Class primaryKeyClass,
                                  Class dataClass,
                                  Class indexKeyClass) {

        this(new SerialBinding(classCatalog, primaryKeyClass),
             new SerialBinding(classCatalog, dataClass),
             new SerialBinding(classCatalog, indexKeyClass));
    }

    /**
     * Creates a serial-serial entity binding.
     *
     * @param primaryKeyBinding is the primary key binding.
     *
     * @param dataBinding is the data binding.
     *
     * @param indexKeyBinding is the index key binding.
     */
    public SerialSerialKeyCreator(SerialBinding primaryKeyBinding,
                                  SerialBinding dataBinding,
                                  SerialBinding indexKeyBinding) {

        this.primaryKeyBinding = primaryKeyBinding;
        this.dataBinding = dataBinding;
        this.indexKeyBinding = indexKeyBinding;
    }

    // javadoc is inherited
    public boolean createSecondaryKey(SecondaryDatabase db,
                                      DatabaseEntry primaryKeyEntry,
                                      DatabaseEntry dataEntry,
                                      DatabaseEntry indexKeyEntry)
        throws DatabaseException {

        Object primaryKeyInput =
            primaryKeyBinding.entryToObject(primaryKeyEntry);
        Object dataInput = dataBinding.entryToObject(dataEntry);
        Object indexKey = createSecondaryKey(primaryKeyInput, dataInput);
        if (indexKey != null) {
            indexKeyBinding.objectToEntry(indexKey, indexKeyEntry);
            return true;
        } else {
            return false;
        }
    }

    // javadoc is inherited
    public boolean nullifyForeignKey(SecondaryDatabase db,
                                     DatabaseEntry dataEntry)
        throws DatabaseException {

        Object data = dataBinding.entryToObject(dataEntry);
        data = nullifyForeignKey(data);
        if (data != null) {
            dataBinding.objectToEntry(data, dataEntry);
            return true;
        } else {
            return false;
        }
    }

    /**
     * Creates the index key object from primary key and entry objects.
     *
     * @param primaryKey is the deserialized source primary key entry, or
     * null if no primary key entry is used to construct the index key.
     *
     * @param data is the deserialized source data entry, or null if no
     * data entry is used to construct the index key.
     *
     * @return the destination index key object, or null to indicate that
     * the key is not present.
     */
    public abstract Object createSecondaryKey(Object primaryKey, Object data);

    /**
     * Clears the index key in a data object.
     *
     * <p>On entry the data parameter contains the index key to be cleared.  It
     * should be changed by this method such that {@link #createSecondaryKey}
     * will return false.  Other fields in the data object should remain
     * unchanged.</p>
     *
     * @param data is the source and destination data object.
     *
     * @return the destination data object, or null to indicate that the
     * key is not present and no change is necessary.  The data returned may
     * be the same object passed as the data parameter or a newly created
     * object.
     */
    public Object nullifyForeignKey(Object data) {

        return null;
    }
}
