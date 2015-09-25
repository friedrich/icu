/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * JAVA equivalent to the C++ SignificantDigitInterval class
 *
 */
public final class SignificantDigitInterval extends FreezableBase<SignificantDigitInterval> {
    public static final SignificantDigitInterval DEFAULT = new SignificantDigitInterval().freeze();
    
    /**
     * No limits on significant digits.
     */
    public SignificantDigitInterval() {
        clear();
    }

    /**
     * Make this instance have no limit on significant digits.
     */
    public void clear() {
        checkThawed();
        fMin = 0;
        fMax = Integer.MAX_VALUE;
    }

    /**
     * Returns true if this object is equal to other.
     */
    boolean equals(SignificantDigitInterval other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof SignificantDigitInterval)) {
            return false;
        }
        SignificantDigitInterval rhs = (SignificantDigitInterval) other;
        return ((fMax == rhs.fMax) && (fMin == rhs.fMin));
    }
    
    public int hashCode() {
        return 37 * fMax + fMin;
    }

    /**
     * Sets maximum significant digits. 0 or negative means no maximum.
     */
    public void setMax(int count) {
        checkThawed();
        fMax = count <= 0 ? Integer.MAX_VALUE : count;
    }

    /**
     * Get maximum significant digits. INT32_MAX means no maximum.
     */
    public int getMax() {
        return fMax;
    }

    /**
     * Sets minimum significant digits. 0 or negative means no minimum.
     */
    public void setMin(int count) {
        checkThawed();
        fMin = count <= 0 ? 0 : count;
    }

    /**
     * Get maximum significant digits. 0 means no minimum.
     */
    public int getMin() {
        return fMin;
    }

    /**
     * Returns TRUE if this instance represents no constraints on significant
     * digits.
     */
    public boolean isNoConstraints() {
        return fMin == 0 && fMax == Integer.MAX_VALUE;
    }

    private int fMax;
    private int fMin;
}
