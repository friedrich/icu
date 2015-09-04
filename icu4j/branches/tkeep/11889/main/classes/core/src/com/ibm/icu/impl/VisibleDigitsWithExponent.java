/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * @author rocketman
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
    
    public VisibleDigits getMantissa() { return fMantissa; }
    
    public VisibleDigits getExponent() { return fExponent; }
    
    public String toString() {
        if (fExponent == null) {
            return fMantissa.toString();
        }
        return fMantissa.toString() + "E" + fExponent.toString();
    }
}
