/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: digitformatter.cpp
 */

#include "unicode/utypes.h"

#include "digitformatter.h"
#include "digitinterval.h"
#include "digitlst.h"
#include "digitgrouping.h"
#include "unicode/dcfmtsym.h"
#include "unicode/unum.h"
#include "fphdlimp.h"
#include "smallintformatter.h"
#include "unistrappender.h"
#include "numericvalue.h"

U_NAMESPACE_BEGIN

DigitFormatter::DigitFormatter()
        : fGroupingSeparator(",", -1, US_INV), fDecimal(".", -1, US_INV),
          fNegativeSign("-", -1, US_INV), fPositiveSign("+", -1, US_INV),
          fIsStandardDigits(TRUE) {
    for (int32_t i = 0; i < 10; ++i) {
        fLocalizedDigits[i] = (UChar32) (0x30 + i);
    }
}

DigitFormatter::DigitFormatter(const DecimalFormatSymbols &symbols) {
    setDecimalFormatSymbols(symbols);
}

void
DigitFormatter::setOtherDecimalFormatSymbols(
        const DecimalFormatSymbols &symbols) {
    fLocalizedDigits[0] = symbols.getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    fLocalizedDigits[1] = symbols.getConstSymbol(DecimalFormatSymbols::kOneDigitSymbol).char32At(0);
    fLocalizedDigits[2] = symbols.getConstSymbol(DecimalFormatSymbols::kTwoDigitSymbol).char32At(0);
    fLocalizedDigits[3] = symbols.getConstSymbol(DecimalFormatSymbols::kThreeDigitSymbol).char32At(0);
    fLocalizedDigits[4] = symbols.getConstSymbol(DecimalFormatSymbols::kFourDigitSymbol).char32At(0);
    fLocalizedDigits[5] = symbols.getConstSymbol(DecimalFormatSymbols::kFiveDigitSymbol).char32At(0);
    fLocalizedDigits[6] = symbols.getConstSymbol(DecimalFormatSymbols::kSixDigitSymbol).char32At(0);
    fLocalizedDigits[7] = symbols.getConstSymbol(DecimalFormatSymbols::kSevenDigitSymbol).char32At(0);
    fLocalizedDigits[8] = symbols.getConstSymbol(DecimalFormatSymbols::kEightDigitSymbol).char32At(0);
    fLocalizedDigits[9] = symbols.getConstSymbol(DecimalFormatSymbols::kNineDigitSymbol).char32At(0);
    fIsStandardDigits = isStandardDigits();
    fNegativeSign = symbols.getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
    fPositiveSign = symbols.getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
    fInfinity.remove();
    fInfinity.append(symbols.getConstSymbol(DecimalFormatSymbols::kInfinitySymbol), UNUM_INTEGER_FIELD);
    fNan.remove();
    fNan.append(symbols.getConstSymbol(DecimalFormatSymbols::kNaNSymbol), UNUM_INTEGER_FIELD);
}

void
DigitFormatter::setDecimalFormatSymbolsForMonetary(
        const DecimalFormatSymbols &symbols) {
    setOtherDecimalFormatSymbols(symbols);
    fGroupingSeparator = symbols.getConstSymbol(DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
    fDecimal = symbols.getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
}

void
DigitFormatter::setDecimalFormatSymbols(
        const DecimalFormatSymbols &symbols) {
    setOtherDecimalFormatSymbols(symbols);
    fGroupingSeparator = symbols.getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
    fDecimal = symbols.getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
}

static void appendField(
        int32_t fieldId,
        const UnicodeString &value,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) {
    int32_t currentLength = appendTo.length();
    appendTo.append(value);
    handler.addAttribute(
            fieldId,
            currentLength,
            appendTo.length());
}

int32_t DigitFormatter::countChar32(
        const NumericValue &value,
        const DigitGrouping &grouping,
        const DigitFormatterOptions &options) const {
    if (value.isNaN()) {
        return countChar32ForNaN();
    }
    if (value.isInfinite()) {
        return countChar32ForInfinity();
    }
    return countChar32(grouping, value.fInterval, options);
}

int32_t DigitFormatter::countChar32(
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        const DigitFormatterOptions &options) const {
    int32_t result = interval.length();

    // We always emit '0' in lieu of no digits.
    if (result == 0) {
        result = 1;
    }
    if (options.fAlwaysShowDecimal || interval.getLeastSignificantInclusive() < 0) {
        result += fDecimal.countChar32();
    }
    result += grouping.getSeparatorCount(interval.getIntDigitCount()) * fGroupingSeparator.countChar32();
    return result;
}

UnicodeString &DigitFormatter::format(
        const NumericValue &value,
        const DigitGrouping &grouping,
        const DigitFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    if (value.isNaN()) {
        return formatNaN(handler, appendTo);
    }
    if (value.isInfinite()) {
        return formatInfinity(handler, appendTo);
    }
    return format(
            value.fValue,
            grouping,
            value.fInterval,
            options,
            handler,
            appendTo);
}

UnicodeString &DigitFormatter::format(
        const DigitList &digits,
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        const DigitFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    int32_t digitsLeftOfDecimal = interval.getMostSignificantExclusive();
    int32_t lastDigitPos = interval.getLeastSignificantInclusive();
    int32_t intBegin = appendTo.length();
    int32_t fracBegin;

    // Emit "0" instead of empty string.
    if (digitsLeftOfDecimal == 0 && lastDigitPos == 0) {
        appendTo.append(fLocalizedDigits[0]);
        handler.addAttribute(UNUM_INTEGER_FIELD, intBegin, appendTo.length());
        if (options.fAlwaysShowDecimal) {
            appendField(
                    UNUM_DECIMAL_SEPARATOR_FIELD,
                    fDecimal,
                    handler,
                    appendTo);
        }
        return appendTo;
    }
    {
        UnicodeStringAppender appender(appendTo);
        for (int32_t i = digitsLeftOfDecimal - 1; i >= lastDigitPos; --i) { 
            if (i == -1) {
                appender.flush();
                appendField(
                        UNUM_DECIMAL_SEPARATOR_FIELD,
                        fDecimal,
                        handler,
                        appendTo);
                fracBegin = appendTo.length();
            }
            appender.append(fLocalizedDigits[digits.getDigitByExponent(i)]);
            if (grouping.isSeparatorAt(digitsLeftOfDecimal, i)) {
                appender.flush();
                appendField(
                        UNUM_GROUPING_SEPARATOR_FIELD,
                        fGroupingSeparator,
                        handler,
                        appendTo);
            }
            if (i == 0) {
                appender.flush();
                if (digitsLeftOfDecimal > 0) {
                    handler.addAttribute(UNUM_INTEGER_FIELD, intBegin, appendTo.length());
                }
            }
        }
        if (options.fAlwaysShowDecimal && lastDigitPos == 0) {
            appender.flush();
            appendField(
                    UNUM_DECIMAL_SEPARATOR_FIELD,
                    fDecimal,
                    handler,
                    appendTo);
        }
    }
    // lastDigitPos is never > 0 so we are guaranteed that kIntegerField
    // is already added.
    if (lastDigitPos < 0) {
        handler.addAttribute(UNUM_FRACTION_FIELD, fracBegin, appendTo.length());
    }
    return appendTo;
}

static UBool isNeg(int32_t &value, uint8_t *digits) {
    if (value < 0) {
        digits[0] = -(value % 10);
        value = -(value / 10);
        return TRUE;
    }
    return FALSE;
}

static int32_t formatInt(
        int32_t value, uint8_t *digits) {
    int32_t idx = 0;
    while (value > 0) {
        digits[idx++] = (uint8_t) (value % 10);
        value /= 10;
    }
    return idx;
}

UnicodeString &
DigitFormatter::formatDigits(
        uint8_t *digits,
        int32_t count,
        const IntDigitCountRange &range,
        int32_t intField,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    int32_t i = range.pin(count) - 1;
    int32_t begin = appendTo.length();

    // Always emit '0' as placeholder for empty string.
    if (i == -1) {
        appendTo.append(fLocalizedDigits[0]);
        handler.addAttribute(intField, begin, appendTo.length());
        return appendTo;
    }
    {
        UnicodeStringAppender appender(appendTo);
        for (; i >= count; --i) {
            appender.append(fLocalizedDigits[0]);
        }
        for (; i >= 0; --i) {
            appender.append(fLocalizedDigits[digits[i]]);
        }
    }
    handler.addAttribute(intField, begin, appendTo.length());
    return appendTo;
}

UnicodeString &
DigitFormatter::formatInt32(
        int32_t value,
        const DigitFormatterIntOptions &options,
        int32_t signField,
        int32_t intField,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    IntDigitCountRange range(options.fMinDigits, INT32_MAX);
    uint8_t digits[10];
    UBool neg = isNeg(value, digits);
    if (neg || options.fAlwaysShowSign) {
        appendField(
                signField,
                neg ? fNegativeSign : fPositiveSign,
                handler,
                appendTo);
    }
    int32_t count = neg ? formatInt(value, digits + 1) + 1 : formatInt(value, digits);
    return formatDigits(
            digits,
            count,
            range,
            intField,
            handler,
            appendTo);
}

int32_t
DigitFormatter::countChar32ForInt32(
        int32_t value,
        const DigitFormatterIntOptions &options) const {
    IntDigitCountRange range(options.fMinDigits, INT32_MAX);
    uint8_t digits[10];
    UBool neg = isNeg(value, digits);
    int32_t count = neg ? formatInt(value, digits + 1) + 1 : formatInt(value, digits);
    int32_t result = range.pin(count);

    // We always emit '0' in lieu of no digits.
    if (result == 0) {
        result = 1;
    }
    if (neg || options.fAlwaysShowSign) {
        result += neg ? fNegativeSign.countChar32() : fPositiveSign.countChar32();
    }
    return result;
}

UnicodeString &
DigitFormatter::formatPositiveInt32(
        int32_t positiveValue,
        const IntDigitCountRange &range,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    // super fast path
    if (fIsStandardDigits && SmallIntFormatter::canFormat(positiveValue, range)) {
        int32_t begin = appendTo.length();
        SmallIntFormatter::format(positiveValue, range, appendTo);
        handler.addAttribute(UNUM_INTEGER_FIELD, begin, appendTo.length());
        return appendTo;
    }
    uint8_t digits[10];
    int32_t count = formatInt(positiveValue, digits);
    return formatDigits(
            digits,
            count,
            range,
            UNUM_INTEGER_FIELD,
            handler,
            appendTo);
}

UBool DigitFormatter::isStandardDigits() const {
    UChar32 cdigit = 0x30;
    for (int32_t i = 0; i < UPRV_LENGTHOF(fLocalizedDigits); ++i) {
        if (fLocalizedDigits[i] != cdigit) {
            return FALSE;
        }
        ++cdigit;
    }
    return TRUE;
}

UBool
DigitFormatter::equals(const DigitFormatter &rhs) const {
    UBool result = (fGroupingSeparator == rhs.fGroupingSeparator) &&
                   (fDecimal == rhs.fDecimal) &&
                   (fNegativeSign == rhs.fNegativeSign) &&
                   (fPositiveSign == rhs.fPositiveSign) &&
                   (fIsStandardDigits == rhs.fIsStandardDigits);
    if (!result) {
        return FALSE;
    }
    for (int32_t i = 0; i < UPRV_LENGTHOF(fLocalizedDigits); ++i) {
        if (fLocalizedDigits[i] != rhs.fLocalizedDigits[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


U_NAMESPACE_END

