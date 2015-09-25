/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.math.BigDecimal;


/**
 * JAVA equivalent to the C++ ScientificPrecision class
 *
 */
public final class ScientificPrecision extends FreezableBase<ScientificPrecision> {
    public static final ScientificPrecision DEFAULT = new ScientificPrecision().freeze();
    
    public ScientificPrecision() {       
    }
    
    // Begin nested FreezableBase fields
    
    private FixedPrecision fMantissa = FixedPrecision.DEFAULT;
    private FixedPrecision fExponent = FixedPrecision.DEFAULT;

    @Override
    protected void freezeFreezableBaseFields() {
        fMantissa.freeze();
        fExponent.freeze();
    }
    
    // End nested FreezableBase fields
    
    /**
     * Return the FixedPrecision that controls the formatting of mantissas.
     */
    public FixedPrecision getMantissa() {
        return fMantissa;
    }
    
    public FixedPrecision getMutableMantissa() {
        checkThawed();
        fMantissa = thaw(fMantissa);
        return fMantissa;
    }
    
    public void setMantissa(FixedPrecision mantissa) {
        checkThawed();
        fMantissa = mantissa.clone();
    }
    
    /**
     * Return the minimum number of exponent digits.
     */
    public int getMinExponentDigits() { return fExponent.getMin().getIntDigitCount(); }
    
    public void setMinExponentDigits(int i) {
        checkThawed();
        fExponent = thaw(fExponent);
        fExponent.getMutableMin().setIntDigitCount(i);
    }
    
    /**
     * Returns true if this object equals other.
     */
    @Override
    public boolean equals(Object other) {
        ScientificPrecision rhs = (ScientificPrecision) other;
        return fMantissa.equals(rhs.fMantissa) && fExponent.equals(rhs.fExponent);
    }
    
    @Override
    public int hashCode() {
        return 37 * fMantissa.hashCode() + fExponent.hashCode();
    }
    
    /**
     * Create a VisibleDigitsWithExponent from given value.
     */
    public VisibleDigitsWithExponent toVisibleDigitsWithExponent(BigDecimal value) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        int exponent = getScientificExponent(trimmedValue);
        BigDecimal trimmedRounded = fMantissa.roundAndTrim(trimmedValue,  exponent);
        
        // Have to recompute exponent in case rounding moved us into a different order of magnitude.
        exponent = getScientificExponent(trimmedRounded);
        
        VisibleDigits mantissaDigits = fMantissa.toVisibleDigits(trimmedRounded.scaleByPowerOfTen(-exponent));
        VisibleDigits exponentDigits = fExponent.toVisibleDigits(exponent);
        return new VisibleDigitsWithExponent(mantissaDigits, exponentDigits);      
    }

    /**
     * Create a VisibleDigitsWithExponent from given value.
     */
    public VisibleDigitsWithExponent toVisibleDigitsWithExponent(double value) {
        VisibleDigits nonNumeric = FixedPrecision.handleNonNumeric(value);
        if (nonNumeric != null) {
            return VisibleDigitsWithExponent.valueOf(nonNumeric);
        }
        return toVisibleDigitsWithExponent(new BigDecimal(String.valueOf(value)));
    }
    
    /**
     * Create a VisibleDigitsWithExponent from given value.
     */
    public VisibleDigitsWithExponent toVisibleDigitsWithExponent(long value) {
        return toVisibleDigitsWithExponent(new BigDecimal(value));
    }
    
    private int getScientificExponent(BigDecimal trimmedValue) {
        return getScientificExponent(trimmedValue, fMantissa.getMin().getIntDigitCount(), getMultiplier());
    }
    
    private static int getScientificExponent(BigDecimal value, int minIntDigitCount, int exponentMultiplier) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        if (trimmedValue.equals(BigDecimal.ZERO)) {
            return  0;
        }
        int intDigitCount = FixedPrecision.getUpperExponent(trimmedValue);
        int exponent;
        if (intDigitCount >= minIntDigitCount) {
            int maxAdjustment = intDigitCount - minIntDigitCount;
            exponent = (maxAdjustment / exponentMultiplier) * exponentMultiplier;
        } else {
            int minAdjustment = minIntDigitCount - intDigitCount;
            exponent = ((minAdjustment + exponentMultiplier - 1) / exponentMultiplier) * -exponentMultiplier;
        }
        return exponent;
    }
    
    private int getMultiplier() {
        int maxIntDigitCount = fMantissa.getMax().getIntDigitCount();
        if (maxIntDigitCount == Integer.MAX_VALUE) {
            return 1;
        }
        int multiplier = maxIntDigitCount - fMantissa.getMin().getIntDigitCount() + 1;
        return (multiplier < 1 ? 1 : multiplier);     
    }
}
