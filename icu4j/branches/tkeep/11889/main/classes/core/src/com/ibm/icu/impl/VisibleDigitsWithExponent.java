/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * Immutable class representing the visible digits of a mantissa and optionally the visible digits of an exponent.
 *
 */
public final class VisibleDigitsWithExponent {
    private final VisibleDigits fMantissa;
    private final VisibleDigits fExponent;
    
    private static final VisibleDigitsWithExponent NOT_A_NUMBER =
            new VisibleDigitsWithExponent(VisibleDigits.NOT_A_NUMBER);

    private static final VisibleDigitsWithExponent NEGATIVE_INFINITY =
            new VisibleDigitsWithExponent(VisibleDigits.NEGATIVE_INFINITY);
    
    private static final VisibleDigitsWithExponent POSITIVE_INFINITY =
            new VisibleDigitsWithExponent(VisibleDigits.POSITIVE_INFINITY);
    
    
    /**
     * Converts a VisibleDigits to a VisibleDigitsWithExponent that has no exponent.
     */
    public static VisibleDigitsWithExponent valueOf(VisibleDigits digits) {
        if (digits == VisibleDigits.NOT_A_NUMBER) {
            return VisibleDigitsWithExponent.NOT_A_NUMBER;
        }
        if (digits == VisibleDigits.NEGATIVE_INFINITY) {
            return VisibleDigitsWithExponent.NEGATIVE_INFINITY;
        }
        if (digits == VisibleDigits.POSITIVE_INFINITY) {
            return VisibleDigitsWithExponent.POSITIVE_INFINITY;
        }
        return new VisibleDigitsWithExponent(digits);
    }
    
    private VisibleDigitsWithExponent(VisibleDigits mantissa) {
        this(mantissa, null);
    }
    
    VisibleDigitsWithExponent(VisibleDigits mantissa, VisibleDigits exponent) {
        fMantissa = mantissa;
        fExponent = exponent;
    }
    
    /**
     * Returns the mantissa of this object.
     */
    public VisibleDigits getMantissa() { return fMantissa; }
    
    /**
     * Returns the optional exponent of this object. Returns null if this object represents only a fixed
     * point number.
     */
    public VisibleDigits getExponent() { return fExponent; }
    
    public String toString() {
        if (fExponent == null) {
            return fMantissa.toString();
        }
        return fMantissa.toString() + "E" + fExponent.toString();
    }
}
