/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.util;

import java.util.LinkedList;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.impl.FieldPositionHandler;
import com.ibm.icu.text.NumberFormat.Field;

/**
 * Fake implementation of FieldPositionHandler for testing.<br>
 * To use <ol>
 * <li>Create an instance by passing the params field of enclosing test.</li>
 * <li>Pass instance to format method.</li>
 * <li>After format method returns, call <code>verify()</code> on instance for each expected field.
 *    The order of <code>verify()</code> calls must match the order that the format method encountered the fields.</li>
 * <li>Finally call verifyNoMoreFields() on instance.</li>
 * </ol>
 *
 */
public final class FieldPositionHandlerForTesting extends TestFmwk implements FieldPositionHandler {

    private static class Entry {
        public final int fieldId;
        public final Field field;
        public final int begin;
        public final int end;
        
        public Entry(int fieldId, Field field, int begin, int end) {
            this.fieldId = fieldId;
            this.field = field;
            this.begin = begin;
            this.end = end;
        }
    }
    
    private final LinkedList<Entry> entries = new LinkedList<Entry>();
    
    public FieldPositionHandlerForTesting(TestParams params) {
        this.params = params;
    }
    
    public void addAttribute(int fieldId, Field field, int begin, int end) {
        entries.add(new Entry(fieldId, field, begin, end));
    }
    
    /**
     * Verifies that the format method logged a particular field.
     * @param fieldId field ID or -1 if no matching field ID.
     * @param field The field type.
     * @param begin begin position.
     * @param end end position.
     */
    public void verify(int fieldId, Field field, int begin, int end) {
        Entry actual = entries.poll();
        if (actual == null) {
            fail("Expected fieldId: " + fieldId + "; field: " + field + "; begin: " +begin + "; end: " +end
                    + ". Got nothing.");
            return;
        }
        if (fieldId != actual.fieldId || !field.equals(actual.field)
                || begin != actual.begin || end != actual.end) {
           this.fail("Expected fieldId: " + fieldId + "; field: " + field + "; begin: " +begin + "; end: " +end
                   + ". got fieldId: " + actual.fieldId + "; field: " + actual.field + "; begin: " +actual.begin + "; end: " +actual.end + "."); 
        }
    }
    
    /**
     * Verifies that the format method encountered no more fields.
     */
    public void verifyNoMoreFields() {
        assertTrue("No more fields", entries.isEmpty());
    }
}

