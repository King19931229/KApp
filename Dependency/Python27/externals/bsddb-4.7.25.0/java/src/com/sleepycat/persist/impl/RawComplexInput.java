/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2008 Oracle.  All rights reserved.
 *
 * $Id: RawComplexInput.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */

package com.sleepycat.persist.impl;

import java.util.IdentityHashMap;

import com.sleepycat.persist.raw.RawObject;

/**
 * Extends RawAbstractInput to convert complex (ComplexFormat and
 * CompositeKeyFormat) RawObject instances.
 *
 * @author Mark Hayes
 */
class RawComplexInput extends RawAbstractInput {

    private FieldInfo[] fields;
    private RawObject[] objects;
    private int index;

    RawComplexInput(Catalog catalog,
                    boolean rawAccess,
                    IdentityHashMap converted,
                    FieldInfo[] fields,
                    RawObject[] objects) {
        super(catalog, rawAccess, converted);
        this.fields = fields;
        this.objects = objects;
    }

    @Override
    Object readNext() {
        RawObject raw = objects[index];
        FieldInfo field = fields[index];
        index += 1;
        Format format = field.getType();
        Object o = raw.getValues().get(field.getName());
        return checkAndConvert(o, format);
    }
}
