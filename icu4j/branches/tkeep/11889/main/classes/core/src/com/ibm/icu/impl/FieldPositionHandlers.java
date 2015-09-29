/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.text.AttributedCharacterIterator;
import java.text.AttributedString;
import java.text.FieldPosition;
import java.text.Format;
import java.util.ArrayList;

import com.ibm.icu.text.NumberFormat.Field;

/**
 * Factory methods for FieldPositionHandler implementations.
 */
public class FieldPositionHandlers {
    
    private FieldPositionHandlers() {
    }
   
    /**
     * Caller passes this as the FieldPositionHandler when it doesn't care to collect field positions.
     */
    public static final FieldPositionHandler DONT_CARE = new DontCareFieldPositionHandler();
    
    /**
     * Creates a one time use FieldPositionHandler for the given FieldPosition object. 
     * @param pos the FieldPosition object. Returned object will log location of desired field by modifying
     *   the begin and end fields of pos in place.
     * @return a one time use FieldPositionHandler.
     */
    public static FieldPositionHandler forFieldPosition(FieldPosition pos) {
        return new FieldPositionFieldPositionHandler(pos);
    }
    
    /**
     * FieldPositionHandler implementation for creating an AttributedCharacterIterator.
     * Use as follows:<br>
     * <ol>
     *   <li>Create a new instance.</li>
     *   <li>Pass it to a format method.</li>
     *   <li>After format returns, pass the formatted string to <code>toAttributedCharacterIterator</code>
     *   to get the AttributedCharacterIterator</li>
     *   <li>Don't reuse instance.</li>
     * </ol>
     *
     */
    public static final class AttributedCharacterIteratorHandler {
        private static class Entry {
            public final Field field;
            public final int begin;
            public final int end;
            
            public Entry(Field field, int begin, int end) {
                this.field = field;
                this.begin = begin;
                this.end = end;
            }
        }
        
        private final ArrayList<Entry> entries = new ArrayList<Entry>();
        
        public void addAttribute(int fieldId, Field field, int begin, int end) {
            entries.add(new Entry(field, begin, end));
        }
        
        public AttributedCharacterIterator toAttributedCharacterIterator(String text) {
            AttributedString as = new AttributedString(text);

            // add field attributes to the AttributedString
            for (Entry entry : entries) {
                as.addAttribute(entry.field, entry.field, entry.begin, entry.end);
            }

            // return the CharacterIterator from AttributedString
            return as.getIterator();
        }
    }

    private static final class DontCareFieldPositionHandler implements FieldPositionHandler {
        public void addAttribute(int fieldId, Field field, int intBegin, int length) {
        }
    }
    
    private static final class FieldPositionFieldPositionHandler implements FieldPositionHandler {
        
        private final FieldPosition fpos;
        private boolean encountered = false;
        
        public FieldPositionFieldPositionHandler(FieldPosition pos) {
            fpos = pos;
        }

        public void addAttribute(int fieldId, Field field, int begin, int end) {
            if (!encountered && matchesField(fpos, fieldId, field)) {
                fpos.setBeginIndex(begin);
                fpos.setEndIndex(end);
                encountered = (begin != end);
            }
        }
        
        private static boolean matchesField(FieldPosition pos, int fieldId, Format.Field field) {
            Format.Field fieldAttribute = pos.getFieldAttribute();
            if (fieldAttribute != null) {
                return fieldAttribute.equals(field);
            }
            return (pos.getField() == fieldId);
        }
    } 
    
   
}
