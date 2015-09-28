/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import com.ibm.icu.text.NumberFormat.Field;

/**
 * Captures field positions when DigitFormatter formats numbers.
 * @see DigitFormatter
 * @see FieldPositionHandlers
 */
public interface FieldPositionHandler {
    /**
     * Called to mark the position of each field
     * @param fieldId The integer field Id or -1 if unknown.
     * @param field the field type.
     * @param begin the 0 based start position of the field in string.
     * @param end the 0 based end position of the field in string.
     */
    void addAttribute(int fieldId, Field field, int begin, int end);
}
