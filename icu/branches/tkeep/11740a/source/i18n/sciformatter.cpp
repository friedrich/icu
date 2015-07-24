/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: sciformatter.cpp
 */

#include "unicode/utypes.h"

#include "sciformatter.h"
#include "digitgrouping.h"
#include "unicode/dcfmtsym.h"
#include "unicode/unum.h"
#include "fphdlimp.h"
#include "visibledigits.h"

// circular dependency. Here only to support old format method.
#include "precision.h"

static UChar gDefaultExponent = 0x45; // 'E'

U_NAMESPACE_BEGIN

SciFormatter::SciFormatter() : fExponent(gDefaultExponent) {
}

SciFormatter::SciFormatter(const DecimalFormatSymbols &symbols) {
    setDecimalFormatSymbols(symbols);
}

void
SciFormatter::setDecimalFormatSymbols(
        const DecimalFormatSymbols &symbols) {
fExponent = symbols.getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
}

UnicodeString &
SciFormatter::format(
        const DigitList &positiveMantissa,
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    UErrorCode status = U_ZERO_ERROR;
    VisibleDigitsWithExponent digits;
    ScientificPrecision::initVisibleDigitsWithExponent(
            positiveMantissa, mantissaInterval,
            exponent, options.fExponent.fMinDigits,
            digits, status);
    return format(digits, formatter, options, handler, appendTo);
}

UnicodeString &
SciFormatter::format(
        const VisibleDigitsWithExponent &digits,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    DigitGrouping grouping;
    formatter.format(
            digits.getMantissa(),
            grouping,
            options.fMantissa,
            handler,
            appendTo);
    const VisibleDigits *exponent = digits.getExponent();
    if (exponent == NULL) {
        return appendTo;
    }
    int32_t expBegin = appendTo.length();
    appendTo.append(fExponent);
    handler.addAttribute(
            UNUM_EXPONENT_SYMBOL_FIELD, expBegin, appendTo.length());
    return formatter.formatExponent(
            *exponent,
            options.fExponent,
            UNUM_EXPONENT_SIGN_FIELD,
            UNUM_EXPONENT_FIELD,
            handler,
            appendTo);
}

int32_t
SciFormatter::countChar32(
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options) const {
    UErrorCode status = U_ZERO_ERROR;
    VisibleDigits digits;
    FixedPrecision expPrecision;
    expPrecision.fMin.setIntDigitCount(options.fExponent.fMinDigits);
    return countChar32(
            expPrecision.initVisibleDigits((int64_t) exponent, digits, status),
            formatter,
            mantissaInterval,
            options);
}

int32_t
SciFormatter::countChar32(
        const VisibleDigitsWithExponent &digits,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options) const {
    const VisibleDigits *exponent = digits.getExponent();
    if (exponent == NULL) {
        DigitGrouping grouping;
        return formatter.countChar32(
                grouping,
                digits.getMantissa().getInterval(),
                options.fMantissa);
    }
    return countChar32(
            *exponent, formatter, digits.getMantissa().getInterval(), options);
}


int32_t
SciFormatter::countChar32(
        const VisibleDigits &exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options) const {
    DigitGrouping grouping;
    int32_t count = formatter.countChar32(
            grouping, mantissaInterval, options.fMantissa);
    count += fExponent.countChar32();
    count += formatter.countChar32ForExponent(
            exponent, options.fExponent);
    return count;
}



U_NAMESPACE_END

