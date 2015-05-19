/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* digitformatter.h
*
* created on: 2015jan06
* created by: Travis Keep
*/

#ifndef __DIGITFORMATTER_H__
#define __DIGITFORMATTER_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "digitaffix.h"

U_NAMESPACE_BEGIN

class DecimalFormatSymbols;
class DigitList;
class DigitGrouping;
class DigitInterval;
class UnicodeString;
class FieldPositionHandler;
class IntDigitCountRange;
class NumericValue;

/**
 * Various options for formatting in fixed point.
 */
class U_I18N_API DigitFormatterOptions : public UMemory {
    public:
    DigitFormatterOptions() : fAlwaysShowDecimal(FALSE) { }

    /**
     * Returns TRUE if this object equals rhs.
     */
    UBool equals(const DigitFormatterOptions &rhs) const {
        return (
            fAlwaysShowDecimal == rhs.fAlwaysShowDecimal);
    }

    /**
     * Returns TRUE if these options allow for fast formatting of
     * integers.
     */
    UBool isFastFormattable() const {
        return (fAlwaysShowDecimal == FALSE);
    }

    /**
     * If TRUE, show the decimal separator even when there are no fraction
     * digits. default is FALSE.
     */
    UBool fAlwaysShowDecimal;
};

/**
 * Various options for formatting an integer.
 */
class U_I18N_API DigitFormatterIntOptions : public UMemory {
    public:
    DigitFormatterIntOptions() : fMinDigits(1), fAlwaysShowSign(FALSE) { }

    /**
     * Returns TRUE if this object equals rhs.
     */
    UBool equals(const DigitFormatterIntOptions &rhs) const {
        return ((fMinDigits == rhs.fMinDigits) &&
                (fAlwaysShowSign == rhs.fAlwaysShowSign));
    }

    /**
     * Minimum digit count to use. Left pad small ints with 0's
     * (or equivalent) to get the minimum digits. Default is 1.
     */
    int32_t fMinDigits;

    /**
     * If TRUE, always prefix the integer with its sign even if the number is
     * positive. Default is FALSE.
     */
    UBool fAlwaysShowSign;
};

/**
 * Does fixed point formatting.
 *
 * This class only does fixed point formatting. It does no rounding before
 * formatting.
 */
class U_I18N_API DigitFormatter : public UMemory {
public:

/**
 * Decimal separator is period (.), Plus sign is plus (+),
 * minus sign is minus (-), grouping separator is comma (,), digits are 0-9.
 */
DigitFormatter();

/**
 * Let symbols determine the digits, decimal separator,
 * plus and mius sign, grouping separator, and possibly other settings.
 */
DigitFormatter(const DecimalFormatSymbols &symbols);

/**
 * Change what this instance uses for digits, decimal separator,
 * plus and mius sign, grouping separator, and possibly other settings
 * according to symbols.
 */
void setDecimalFormatSymbols(const DecimalFormatSymbols &symbols);

/**
 * Change what this instance uses for digits, decimal separator,
 * plus and mius sign, grouping separator, and possibly other settings
 * according to symbols in the context of monetary amounts.
 */
void setDecimalFormatSymbolsForMonetary(const DecimalFormatSymbols &symbols);

/**
 * Fixed point formatting.
 *
 * @param positiveDigits the value to format must be positive.
 * @param grouping controls how digit grouping is done
 * @param interval specifies what digits of the value get formatted.
 *   Provides poor man's truncation without doing any arithmetic.
 * @param options formatting options
 * @param handler records field positions
 * @param appendTo formatted value appended here.
 * @return appendTo
 */
UnicodeString &format(
        const DigitList &positiveDigits,
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        const DigitFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

UnicodeString &format(
        const NumericValue &value,
        const DigitGrouping &grouping,
        const DigitFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

/**
 * Formats NaN.
 * @param handler records field positions
 * @param appendTo formatted value appended here.
 * @return appendTo
 */
UnicodeString &formatNaN(
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    return fNan.format(handler, appendTo);
}

/**
 * Counts code points for NaN.
 */
int32_t countChar32ForNaN() const {
    return fNan.toString().countChar32();
}

/**
 * Formats positive infinity.
 * @param handler records field positions
 * @param appendTo formatted value appended here.
 * @return appendTo
 */
UnicodeString &formatInfinity(
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    return fInfinity.format(handler, appendTo);
}

/**
 * Counts code points for positive infinity.
 */
int32_t countChar32ForInfinity() const {
    return fInfinity.toString().countChar32();
}

/**
 * Fixed point formatting of integers.
 * Always performed with no grouping and no decimal point.
 *
 * @param positiveValue the value to format must be positive.
 * @param range specifies minimum and maximum number of digits.
 * @param handler records field positions
 * @param appendTo formatted value appended here.
 * @return appendTo
 */
UnicodeString &formatPositiveInt32(
        int32_t positiveValue,
        const IntDigitCountRange &range,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

/**
 * Fixed point formatting for 32 bit ints.
 * @param value the value to format. May be positive or negative.
 * @param options formatting options.
 * @param signField The field ID to use when recording the sign field.
 *   Can be anything if handler is not recording field positions.
 * @param intField The field ID to use when recording the integer field.
 *   Can be anything if handler is not recording field positions.
 * @param handler Records the field positions.
 * @param appendTo the formatted value appended here.
 */
UnicodeString &formatInt32(
        int32_t value,
        const DigitFormatterIntOptions &options,
        int32_t signField,
        int32_t intField,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

/**
 * Counts the number of code points needed for formatting.
 */
int32_t countChar32(
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        const DigitFormatterOptions &options) const;

int32_t countChar32(
        const NumericValue &value,
        const DigitGrouping &grouping,
        const DigitFormatterOptions &options) const;

/**
 * Counts the number of code points needed for formatting an int32.
 */
int32_t countChar32ForInt32(
        int32_t value,
        const DigitFormatterIntOptions &options) const;

/**
 * Returns TRUE if this object equals rhs.
 */
UBool equals(const DigitFormatter &rhs) const;

private:
UChar32 fLocalizedDigits[10];
UnicodeString fGroupingSeparator;
UnicodeString fDecimal;
UnicodeString fNegativeSign;
UnicodeString fPositiveSign;
DigitAffix fInfinity;
DigitAffix fNan;
UBool fIsStandardDigits;

UBool isStandardDigits() const;

UnicodeString &formatDigits(
        uint8_t *digits,
        int32_t count,
        const IntDigitCountRange &range,
        int32_t intField,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

void setOtherDecimalFormatSymbols(const DecimalFormatSymbols &symbols);

};


U_NAMESPACE_END

#endif  // __DIGITFORMATTER_H__
