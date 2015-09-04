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
public class DigitInterval extends FreezableBase<DigitInterval> {
    public static final DigitInterval DEFAULT = new DigitInterval().freeze();
    public static final DigitInterval SINGLE_INT_DIGIT = new DigitInterval(1, 0).freeze();
    
    /**
     * Returns a new unfrozen digit interval for the given digit range.
     * @param lowerInclusive
     * @param upperExclusive
     * @return
     */
    public static DigitInterval forDigitRange(int lowerInclusive, int upperExclusive) {
        DigitInterval result = new DigitInterval();
        result.setLeastSignificantInclusive(lowerInclusive);
        result.setMostSignificantExclusive(upperExclusive);
        return result;
    }
    
    /**
     * Spans all integer and fraction digits
     */ 
    public DigitInterval() {
        clear();
    }
    
    private DigitInterval(int largestExclusive, int smallestInclusive) {
        fLargestExclusive = largestExclusive;
        fSmallestInclusive = smallestInclusive;
    }
    
    /**
     * Makes this instance span all digits.
     */    
    public void clear() {
        checkThawed();
        fLargestExclusive = Integer.MAX_VALUE;
        fSmallestInclusive = Integer.MIN_VALUE;
    }
    
    /**
     * Returns true if this object is the same as rhs.
     */
    public boolean equals(DigitInterval other) {
        DigitInterval rhs = (DigitInterval) other;
        return ((fLargestExclusive == rhs.fLargestExclusive) &&
                (fSmallestInclusive == rhs.fSmallestInclusive));
    }
    
    public int hashCode() {
        return 37*fLargestExclusive + fSmallestInclusive;
    }

    /**
     * Expand this interval so that it contains all of rhs.
     */
    public void expandToContain(DigitInterval rhs) {
        checkThawed();
        if (fSmallestInclusive > rhs.fSmallestInclusive) {
            fSmallestInclusive = rhs.fSmallestInclusive;
        }
        if (fLargestExclusive < rhs.fLargestExclusive) {
            fLargestExclusive = rhs.fLargestExclusive;
        }   
    }
    
   /**
     * Shrink this interval so that it contains no more than rhs.
     */
    public void shrinkToFitWithin(DigitInterval rhs) {
        checkThawed();
        if (fSmallestInclusive < rhs.fSmallestInclusive) {
            fSmallestInclusive = rhs.fSmallestInclusive;
        }
        if (fLargestExclusive > rhs.fLargestExclusive) {
            fLargestExclusive = rhs.fLargestExclusive;
        }
    }

    /**
     * Expand this interval as necessary to contain digit with given exponent
     * After this method returns, this interval is guaranteed to contain
     * digitExponent.
     */
    public void expandToContainDigit(int digitExponent) {
        checkThawed();
        if (fLargestExclusive <= digitExponent) {
            fLargestExclusive = digitExponent + 1;
        } else if (fSmallestInclusive > digitExponent) {
            fSmallestInclusive = digitExponent;
        }
    }


    /**
     * Changes the number of digits to the left of the decimal point that
     * this interval spans. If count is negative, it means span all digits
     * to the left of the decimal point.
     */
    public void setIntDigitCount(int count) {
        checkThawed();
        fLargestExclusive = count < 0 ? Integer.MAX_VALUE : count;    
    }

    /**
     * Changes the number of digits to the right of the decimal point that
     * this interval spans. If count is negative, it means span all digits
     * to the right of the decimal point.
     */
    public void setFracDigitCount(int count) {
        checkThawed();
        fSmallestInclusive = count < 0 ? Integer.MIN_VALUE : -count;   
    }

    /**
     * Sets the least significant inclusive value to smallest. If smallest >= 0
     * then least significant inclusive value becomes 0.
     */
    public void setLeastSignificantInclusive(int smallest) {
        checkThawed();
        fSmallestInclusive = smallest < 0 ? smallest : 0;
    }
    /**
     * Sets the most significant exclusive value to largest.
     * If largest <= 0 then most significant exclusive value becomes 0.
     */
    public void setMostSignificantExclusive(int largest) {
        checkThawed();
        fLargestExclusive = largest > 0 ? largest : 0;
    }

    /**
     * If returns 8, the most significant digit in interval is the 10^7 digit.
     * Returns INT32_MAX if this interval spans all digits to left of
     * decimal point.
     */
    public int getMostSignificantExclusive() {
        return fLargestExclusive;
    }

    /**
     * Returns number of digits to the left of the decimal that this
     * interval includes. This is a synonym for getMostSignificantExclusive().
     */
    public int getIntDigitCount() {
        return fLargestExclusive;
    }

    /**
     * Returns number of digits to the right of the decimal that this
     * interval includes.
     */
    public int getFracDigitCount() {
        return fSmallestInclusive == Integer.MIN_VALUE ? Integer.MAX_VALUE : -fSmallestInclusive;
    }

    /**
     * Returns the total number of digits that this interval spans.
     * Caution: If this interval spans all digits to the left or right of
     * decimal point instead of some fixed number, then what length()
     * returns is undefined.
     */
    public int length() {
        return fLargestExclusive - fSmallestInclusive;
     }

    /**
     * If returns -3, the least significant digit in interval is the 10^-3
     * digit. Returns INT32_MIN if this interval spans all digits to right of
     * decimal point.
     */
    public int getLeastSignificantInclusive() {
        return fSmallestInclusive;
    }
      
    /**
     * Returns TRUE if this interval contains this digit position.
     */
    public boolean contains(int digitPosition) {
        return (digitPosition < fLargestExclusive
                && digitPosition >= fSmallestInclusive);   
    }
     
    private int fLargestExclusive;
    private int fSmallestInclusive;
}
