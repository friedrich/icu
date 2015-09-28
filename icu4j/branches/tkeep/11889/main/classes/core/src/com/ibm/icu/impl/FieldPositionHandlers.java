/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.text.FieldPosition;
import java.text.Format;
import java.util.ArrayList;

import com.ibm.icu.text.NumberFormat.Field;

/**
 * Factory methods for FieldPositionHandler implementations.
 */
public class FieldPositionHandlers {
   
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
