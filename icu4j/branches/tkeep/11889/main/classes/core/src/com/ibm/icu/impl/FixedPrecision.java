/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.io.ByteArrayOutputStream;
import java.math.RoundingMode;
import java.math.BigDecimal;

/**
 * @author rocketman
 *
 */
public final class FixedPrecision extends FreezableBase<FixedPrecision> {
    public static final FixedPrecision DEFAULT = new FixedPrecision().freeze();
    private static final double MAX_LONG_IN_DOUBLE = 9007199254740992.0;
    private static final int[] POWER_10 = {1, 10, 100, 1000};
    
    public FixedPrecision() {       
    }

    // FreezableBase fields go here
    
    private DigitInterval fMin = DigitInterval.SINGLE_INT_DIGIT;
    private DigitInterval fMax = DigitInterval.DEFAULT;
    private SignificantDigitInterval fSignificant = SignificantDigitInterval.DEFAULT;
    
    @Override
    protected void freezeFreezableBaseFields() {
        fMin.freeze();
        fMax.freeze();
        fSignificant.freeze();
    }
    
    // End FreezableBase fields.
    
    private BigDecimal fRoundingIncrement = null;
    private boolean fExactOnly = false;
    private boolean fFailIfOverMax = false;
    private RoundingMode fRoundingMode = RoundingMode.HALF_EVEN;
    
    /**
     * The smallest format interval allowed. Default is 1 integer digit and no
     * fraction digits.
     */
    public DigitInterval getMin() {
        return fMin;
    }
    
    public DigitInterval getMutableMin() {
        checkThawed();
        fMin = thaw(fMin);
        return fMin;
    }
    
    public void setMin(DigitInterval interval) {
        checkThawed();
        fMin = interval.clone();
    }

    
    /**
     * The largest format interval allowed. Must contain fMin.
     *  Default is all digits.
     */
    public DigitInterval getMax() {
        return fMax;
    }
    
    public DigitInterval getMutableMax() {
        checkThawed();
        fMax = thaw(fMax);
        return fMax;
    }
    
    public void setMax(DigitInterval interval) {
        checkThawed();
        fMax = interval.clone();
    }
    
    
    /**
     * Min and max significant digits allowed. The default is no constraints.
     */
    public SignificantDigitInterval getSignificant() {
        return fSignificant;
    }
    
    public SignificantDigitInterval getMutableSignificant() {
        checkThawed();
        fSignificant = thaw(fSignificant);
        return fSignificant;
    }
    
    public void setSignificant(SignificantDigitInterval interval) {
        checkThawed();
        fSignificant = interval.clone();
    }

    
    /**
     * The rounding increment or zero if there is no rounding increment.
     * Default is zero.
     */
    public BigDecimal getRoundingIncrement() {
        return fRoundingIncrement;
    }
    
    public void setRoundingIncrement(BigDecimal x) {
        checkThawed();
        fRoundingIncrement = x;
    }

    
    /**
     * If set, causes round() to set status to U_FORMAT_INEXACT_ERROR if
     * any rounding is done. Default is FALSE.
     */
    public boolean getExactOnly() {
        return fExactOnly;
    }
    
    public void setExactOnly(boolean b) {
        checkThawed();
        fExactOnly = b;
    }
    
    
    /**
     * If set, causes round() to set status to U_ILLEGAL_ARGUMENT_ERROR if
     * rounded number has more than maximum integer digits. Default is FALSE.
     */
    public boolean getFailIfOverMax() {
        return fFailIfOverMax;
    }
    
    public void setFailIfOverMax(boolean b) {
        checkThawed();
        fFailIfOverMax = b;
    }
       
    
    
    public RoundingMode getRoundingMode() {
        return fRoundingMode;
    }
    
    public void setRoundingMode(RoundingMode x) {
        checkThawed();
        fRoundingMode = x;
    }

    /**
     * Returns TRUE if this object equals rhs.
     */
    public boolean equals(Object other) {
        FixedPrecision rhs = (FixedPrecision) other;
        return (fMin.equals(rhs.fMin) &&
                fMax.equals(rhs.fMax) &&
                fSignificant.equals(rhs.fSignificant) &&
                (fRoundingIncrement.equals(rhs.fRoundingIncrement)) &&
                fExactOnly == rhs.fExactOnly &&
                fFailIfOverMax == rhs.fFailIfOverMax &&
                fRoundingMode == rhs.fRoundingMode);
    }


    /**
     * Rounds value to prepare it for formatting.
     * @param value The value to be rounded.
     * @param exponent Always pass 0 for fixed decimal formatting. scientific
     *  precision passes the exponent value.  Essentially, it divides value by
     *  10^exponent, rounds and then multiplies by 10^exponent.
     * @return rounded value.
     */
    BigDecimal roundAndTrim(BigDecimal value, int exponent) {
        RoundingMode roundingMode = fExactOnly ? RoundingMode.UNNECESSARY : fRoundingMode;
        
        if (fRoundingIncrement != null) {
            if (exponent == 0) {
                value = quantizeAndTrim(value, fRoundingIncrement, roundingMode);
            } else {
                value = quantizeAndTrim(value, fRoundingIncrement.scaleByPowerOfTen(exponent), roundingMode);
            }
        }
        int leastSig = fMax.getLeastSignificantInclusive();
        BigDecimal trimmedValue = roundAtExponentAndTrim(
                value,
                leastSig == Integer.MIN_VALUE ? leastSig : exponent + leastSig,
                fSignificant.getMax(),
                roundingMode);
       if (fFailIfOverMax) {
            // Smallest interval for value stored in interval
            if (fMax.getIntDigitCount() < getUpperExponent(trimmedValue)) {
                throwTooBigException();
            }
        }
        return trimmedValue;        
    }
    
    
    private static BigDecimal quantizeAndTrim(BigDecimal value, BigDecimal increment,
            RoundingMode roundingMode) {
        BigDecimal quotient = value.divide(increment, 0, roundingMode);
        return quotient.multiply(increment).stripTrailingZeros();
    }
    
    private static BigDecimal roundAtExponentAndTrim(
            BigDecimal value, int exponent, RoundingMode roundingMode) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        
        // Don't attempt to round if exponent is minimum integer. 
        if (exponent == Integer.MIN_VALUE) {
            return trimmedValue;
        }
        
        // scale = 4 means unscaled * 10^-4
        int newScale = -exponent;
        
        
        // If new scale is at least as much as old scale no rounding necessary
        if (newScale >= trimmedValue.scale()) {
            return trimmedValue;
        }
        return trimmedValue.setScale(newScale, roundingMode).stripTrailingZeros();
        
    }

    private static BigDecimal roundAtExponentAndTrim(
            BigDecimal value, int exponent, int maxSigDigits, RoundingMode roundingMode) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        if (maxSigDigits < trimmedValue.precision()) {
            int minExponent = getUpperExponent(trimmedValue) - maxSigDigits;
            if (exponent < minExponent) {
                exponent = minExponent;
            }
        }
        return roundAtExponentAndTrim(trimmedValue, exponent, roundingMode);
    }

    
    private static DigitInterval getSmallestInterval(BigDecimal value) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        return DigitInterval.forDigitRange(getLowerExponent(trimmedValue), getUpperExponent(trimmedValue));
    }

    static int getUpperExponent(BigDecimal value) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        return trimmedValue.precision() - trimmedValue.scale();
    }
    
    private static int getLowerExponent(BigDecimal value) {
        BigDecimal trimmedValue = value.stripTrailingZeros();
        return -trimmedValue.scale();
    }

    private DigitInterval getInterval(BigDecimal value) {
        DigitInterval result;
        BigDecimal trimmedValue = value.stripTrailingZeros();
        if (trimmedValue.equals(BigDecimal.ZERO)) {
            result = fMin.cloneAsThawed();
            if (fSignificant.getMin() > 0) {
                result.expandToContainDigit(result.getIntDigitCount() - fSignificant.getMin());
            }
        } else {
            result = getSmallestInterval(trimmedValue);
            if (fSignificant.getMin() > 0) {
                result.expandToContainDigit(
                        getUpperExponent(trimmedValue) - fSignificant.getMin());
            }
            result.expandToContain(fMin);
        }
        result.shrinkToFitWithin(fMax);
        return result;
    }

 

    /**
     * Returns TRUE if this instance allows for fast formatting of integers.
     */
    public boolean isFastFormattable() {
        return (fMin.getFracDigitCount() == 0 && fSignificant.isNoConstraints()
                && fRoundingIncrement == null && !fFailIfOverMax);  
    }


    
    public VisibleDigits initVisibleDigits(BigDecimal value) {
        BigDecimal trimmedValue = roundAndTrim(value, 0);
        DigitInterval interval = getInterval(trimmedValue).freeze();
        int exponent = getLowerExponent(trimmedValue);
        String unscaledStr = trimmedValue.unscaledValue().toString();
        boolean isNegative = unscaledStr.startsWith("-");
        byte[] digits;
        if (isNegative) {
            digits = createDigits(unscaledStr, 1);
        } else {
            digits = createDigits(unscaledStr, 0);
        }
        return VisibleDigits.create(digits, exponent, interval, isNegative ? VisibleDigits.NEGATIVE : 0, 0L, 0.0, false);
    }
    
    private byte[] createDigits(String str, int mostSigIndex) {
        byte[] result = new byte[str.length() - mostSigIndex];
        int index = 0;
        for (int i = str.length() - 1; i >= mostSigIndex; --i) {
            result[index++] = (byte) (str.charAt(i) - '0');
        }
        return result;
    }
    
    static VisibleDigits handleNonNumeric(double value) {
        if (Double.isNaN(value)) {
            return VisibleDigits.NOT_A_NUMBER;
        }
        if (Double.isInfinite(value)) {
            if (Double.doubleToLongBits(value) < 0) {
                return VisibleDigits.NEGATIVE_INFINITY;
            } else {
                return VisibleDigits.POSITIVE_INFINITY;
            }
        }
        return null;
    }

   
    public VisibleDigits initVisibleDigits(double value) {
        VisibleDigits nonNumeric = handleNonNumeric(value);
        if (nonNumeric != null) {
            return nonNumeric;
        }
        if (fRoundingIncrement != null) {
            return initVisibleDigits(new BigDecimal(String.valueOf(value)));
        }
        int n = -1;
        double scaled = value;
        for (int i = 0; i < POWER_10.length; ++i) {
            scaled = value * POWER_10[i];
            if (scaled > MAX_LONG_IN_DOUBLE || scaled < -MAX_LONG_IN_DOUBLE) {
                break;
            }
            if (scaled == Math.floor(scaled)) {
                n = i;
                break;
            }
        }
        if (n >= 0) {
            VisibleDigits result = initVisibleDigits((long) scaled, -n, value);
            if (result != null) {
                if (scaled == 0.0 && Double.doubleToLongBits(scaled) < 0) {
                    result = result.withNegative();
                }
                return result;
            }
        }
        return initVisibleDigits(new BigDecimal(String.valueOf(value)));
    }

    public VisibleDigits initVisibleDigits(long value) {
        if (fRoundingIncrement != null) {
            return initVisibleDigits(new BigDecimal(value));
        }
        VisibleDigits result = initVisibleDigits(value, 0, value);
        if (result != null) {
            return result;
        }
        return initVisibleDigits(new BigDecimal(value));
    }

    private VisibleDigits initVisibleDigits(long mantissa, int exponent, double value) {
        // Precompute fAbsIntValue if it is small enough, but we don't know yet
        // if it will be valid.
        boolean absIntValueComputed = false;
        long absIntValue = 0L;
        if (mantissa > -1000000000000000000L /* -1e18 */
                && mantissa < 1000000000000000000L /* 1e18 */) {
            absIntValue = mantissa;
            if (absIntValue < 0) {
                absIntValue = -absIntValue;
            }
            int i = 0;
            int maxPower10Exp = POWER_10.length - 1;
            for (; i > exponent + maxPower10Exp; i -= maxPower10Exp) {
                absIntValue /= POWER_10[maxPower10Exp];
            }
            absIntValue /= POWER_10[i - exponent];
            absIntValueComputed = true;
        }
        if (mantissa == 0) {
            return VisibleDigits.create(VisibleDigits.NO_DIGITS, 0, getIntervalForZero().freeze(), 0, 0L, 0.0, true);
        }
        // be sure least significant digit is non zero
        while (mantissa % 10 == 0) {
            mantissa /= 10;
            ++exponent;
        }
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        boolean isNegative = false;
        if (mantissa < 0) {
            bos.write((int) -(mantissa % -10));
            mantissa /= -10;
            isNegative = true;
        }
        while (mantissa > 0) {
            bos.write((int) (mantissa % 10));
            mantissa /= 10;
        }
        byte[] digits = bos.toByteArray();
        int upperExponent = exponent + digits.length;
        if (fFailIfOverMax && upperExponent > fMax.getIntDigitCount()) {
            // TODO: Check with PMC
            throwTooBigException();
        }
        boolean roundingRequired = isRoundingRequired(upperExponent, exponent);
        if (roundingRequired) {
            if (fExactOnly) {
               throwRoundingNecessaryException();
            }
            return null;
        }

        // The intValue we computed above is only valid if our visible digits
        // doesn't exceed the maximum integer digits allowed.
        return VisibleDigits.create(
                digits,
                exponent,
                getInterval(exponent, upperExponent).freeze(),
                isNegative ? VisibleDigits.NEGATIVE : 0,
                absIntValue,
                Math.abs(value),
                absIntValueComputed);
    }

    private static void throwRoundingNecessaryException() {
        throw new ArithmeticException("Rounding necessary");
    }

    private static void throwTooBigException() {
        throw new IllegalArgumentException("Value too big");
    }
    
    private boolean isRoundingRequired(int upperExponent, int lowerExponent) {
        int leastSigAllowed = fMax.getLeastSignificantInclusive();
        int maxSignificantDigits = fSignificant.getMax();
        int roundDigit;
        if (maxSignificantDigits == Integer.MAX_VALUE) {
            roundDigit = leastSigAllowed;
        } else {
            int limitDigit = upperExponent - maxSignificantDigits;
            roundDigit =
                    limitDigit > leastSigAllowed ? limitDigit : leastSigAllowed;
        }
        return (roundDigit > lowerExponent);
     
    }
    
    
    private DigitInterval getIntervalForZero() {
        DigitInterval result = fMin.cloneAsThawed();
        if (fSignificant.getMin() > 0) {
            result.expandToContainDigit(result.getIntDigitCount() - fSignificant.getMin());
        }
        result.shrinkToFitWithin(fMax);
        return result;
     
    }
    
    private DigitInterval getInterval(int lowerExponent, int upperExponent) {
        DigitInterval interval = DigitInterval.forDigitRange(lowerExponent, upperExponent);
        if (fSignificant.getMin() > 0) {
            interval.expandToContainDigit(
                    upperExponent - fSignificant.getMin());
        }
        interval.expandToContain(fMin);
        interval.shrinkToFitWithin(fMax);
        return interval;        
    }
}

    