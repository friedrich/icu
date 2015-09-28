/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.DecimalFormatSymbols;
import com.ibm.icu.text.NumberFormat;
import com.ibm.icu.text.NumberFormat.Field;
import com.ibm.icu.text.UTF16;

/**
 * DigitFormatter formats positive values, NaN, and infinity.
 * Caller responsible for adding correct prefix and suffix for sign.
 *
 */
public final class DigitFormatter {
    private final int[] fLocalizedDigits;
    private final String fGroupingSeparator;
    private final String fDecimal;
    private final String fNegativeSign;
    private final String fPositiveSign;
    private final String fInfinity;
    private final String fNan;
    private final String fExponent;
    
    private DigitFormatter(
            int[] localizedDigits, String groupingSeparator, String decimal,
            String negativeSign, String positiveSign, String infinity,
            String nan, String exponent) {
        fLocalizedDigits = localizedDigits;
        fGroupingSeparator = groupingSeparator;
        fDecimal = decimal;
        fNegativeSign = negativeSign;
        fPositiveSign = positiveSign;
        fInfinity = infinity;
        fNan = nan;
        fExponent = exponent;
    }
    
    /**
     * Create a new DigitFormatter.
     */
    public static DigitFormatter getInstance(DecimalFormatSymbols symbols) {
        int[] digits = extractIntDigits(symbols);
        return new DigitFormatter(
                digits, String.valueOf(symbols.getGroupingSeparator()),
                String.valueOf(symbols.getDecimalSeparator()), symbols.getMinusString(),
                symbols.getPlusString(), symbols.getInfinity(),
                symbols.getNaN(), symbols.getExponentSeparator());
    }

    
    private static int[] extractIntDigits(DecimalFormatSymbols symbols) {
        char[] cdigits = symbols.getDigits();
        int[] digits = new int[cdigits.length];
        for (int i = 0; i < digits.length; ++i) {
            digits[i] = cdigits[i];
        }
        return digits;
    }
    
    /**
     * Create a new DigitFormatter for monetary use.
     */
    public static DigitFormatter getMonetaryInstance(DecimalFormatSymbols symbols) {
        int[] digits = extractIntDigits(symbols);
        return new DigitFormatter(
                digits, String.valueOf(symbols.getMonetaryGroupingSeparator()),
                String.valueOf(symbols.getMonetaryDecimalSeparator()), symbols.getMinusString(),
                symbols.getPlusString(), symbols.getInfinity(),
                symbols.getNaN(), symbols.getExponentSeparator());
    }
    
    /**
     * Formats positive digits as fixed decimal. Also handles NaN and Infinity.
     * @param positiveDigits the digits
     * @param grouping the grouping policy
     * @param options formatting options
     * @param handler captures field positions
     * @param appendTo result appended here
     * @return appendTo
     */
    public StringBuilder format(
            VisibleDigits positiveDigits,
            DigitGrouping grouping,
            Options options,
            FieldPositionHandler handler,
            StringBuilder appendTo) {
        if (positiveDigits.isNaN()) {
            return appendField(NumberFormat.INTEGER_FIELD, Field.INTEGER, fNan, handler, appendTo);
        }
        if (positiveDigits.isInfinite()) {
            return appendField(NumberFormat.INTEGER_FIELD, Field.INTEGER, fInfinity, handler, appendTo);
        }
        DigitInterval interval = positiveDigits.getInterval();
        int digitsLeftOfDecimal = interval.getMostSignificantExclusive();
        int lastDigitPos = interval.getLeastSignificantInclusive();
        int intBegin = appendTo.length();
        int fracBegin = 0;

        // Emit "0" instead of empty string.
        if (digitsLeftOfDecimal == 0 && lastDigitPos == 0) {
            append(fLocalizedDigits[0], appendTo);
            handler.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, intBegin, appendTo.length());
            if (options.isAlwaysShowDecimal()) {
                appendField(
                        -1,
                        Field.DECIMAL_SEPARATOR,
                        fDecimal,
                        handler,
                        appendTo);
            }
            return appendTo;
        }
        for (int i = interval.getMostSignificantExclusive() - 1;
                i >= interval.getLeastSignificantInclusive(); --i) {
            if (i == -1) {
                appendField(
                        -1,
                        Field.DECIMAL_SEPARATOR,
                        fDecimal,
                        handler,
                        appendTo);
                fracBegin = appendTo.length();
            }
            append(fLocalizedDigits[positiveDigits.getDigitByExponent(i)], appendTo);
            if (grouping.isSeparatorAt(digitsLeftOfDecimal, i)) {
                appendField(
                        -1,
                        Field.GROUPING_SEPARATOR,
                        fGroupingSeparator,
                        handler,
                        appendTo);
            }
            if (i == 0) {
                if (digitsLeftOfDecimal > 0) {
                    handler.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, intBegin, appendTo.length());
                }
            }
        }
        if (options.isAlwaysShowDecimal() && lastDigitPos == 0) {
            appendField(
                    -1,
                    Field.DECIMAL_SEPARATOR,
                    fDecimal,
                    handler,
                    appendTo);
        }
        // lastDigitPos is never > 0 so we are guaranteed that kIntegerField
        // is already added.
        if (lastDigitPos < 0) {
            handler.addAttribute(NumberFormat.FRACTION_FIELD, Field.FRACTION, fracBegin, appendTo.length());
        }
        return appendTo;

    }
    
    /**
     * Formats positive digits in scientific notation. Also handles NaN and Infinity.
     * @param digits the digits
     * @param options formatting options
     * @param handler captures field positions
     * @param appendTo result appended here
     * @return appendTo
     */
    public StringBuilder format(
            VisibleDigitsWithExponent digits,
            Options options,
            FieldPositionHandler handler,
            StringBuilder appendTo) {
        format(
                digits.getMantissa(),
                DigitGrouping.NO_GROUPING,
                options,
                handler,
                appendTo);
        VisibleDigits exponent = digits.getExponent();
        if (exponent == null) {
            return appendTo;
        }
        appendField(-1, Field.EXPONENT_SYMBOL, fExponent, handler, appendTo);
        return formatExponent(
                exponent,
                options,
                -1,
                Field.EXPONENT_SIGN,
                -1,
                Field.EXPONENT,
                handler,
                appendTo);
    }
    
    private StringBuilder formatExponent(
            VisibleDigits digits,
            Options options,
            int signFieldId,
            Field signField,
            int intFieldId,
            Field intField,
            FieldPositionHandler handler,
            StringBuilder appendTo) {
        boolean neg = digits.isNegative();
        if (neg || options.isAlwaysShowExponentSign()) {
            appendField(
                    signFieldId,
                    signField,
                    neg ? fNegativeSign : fPositiveSign,
                    handler,
                    appendTo);
        }
        int begin = appendTo.length();
        format(
                digits,
                DigitGrouping.NO_GROUPING,
                Options.DEFAULT,
                FieldPositionHandlers.DONT_CARE,
                appendTo);
        handler.addAttribute(intFieldId, intField, begin, appendTo.length());
        return appendTo;
    }


    
    private static StringBuilder appendField(
            int fieldId,
            Field field,
            String value,
            FieldPositionHandler handler,
            StringBuilder appendTo) {
        int currentLength = appendTo.length();
        appendTo.append(value);
        handler.addAttribute(
                fieldId,
                field,
                currentLength,
                appendTo.length());
        return appendTo;
    }

    private static StringBuilder append(int ch, StringBuilder appendTo)
    {
        if (ch < UCharacter.MIN_VALUE || ch > UCharacter.MAX_VALUE) {
            // do nothing
            return appendTo;
        }

        if (ch < UCharacter.SUPPLEMENTARY_MIN_VALUE) {
            return appendTo.append((char)ch);
        }

        appendTo.append(UTF16.getLeadSurrogate(ch));
        return appendTo.append(UTF16.getTrailSurrogate(ch));
    }
    
    /**
     * Options for DigitFormatter
     */
    public static class Options extends FreezableBase<Options> {
        public static final Options DEFAULT = new Options().freeze();
        
        private boolean fAlwaysShowDecimal = false;
        private boolean fAlwaysShowExponentSign = false;
        
        public Options() {
        }
        
        public boolean isAlwaysShowDecimal() { return fAlwaysShowDecimal; }
        public void setAlwaysShowDecimal(boolean b) {
            checkThawed();
            fAlwaysShowDecimal = b;
        }
        
        public boolean isAlwaysShowExponentSign() { return fAlwaysShowExponentSign; }
        public void setAlwaysShowExponentSign(boolean b) {
            checkThawed();
            fAlwaysShowExponentSign = b;
        }
        
        @Override
        public boolean equals(Object other) {
            if (this == other) {
                return true;
            }
            if (!(other instanceof Options)) {
                return false;
            }
            Options rhs = (Options) other;
            return (fAlwaysShowDecimal == rhs.fAlwaysShowDecimal && fAlwaysShowExponentSign == rhs.fAlwaysShowExponentSign);
        }
        
        @Override
        public int hashCode() {
            return (fAlwaysShowDecimal ? 2 : 0) | (fAlwaysShowExponentSign ? 1 : 0);
        }
    }    
}