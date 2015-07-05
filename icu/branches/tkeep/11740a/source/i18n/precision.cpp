/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: precisison.cpp
 */

#include "unicode/utypes.h"

#include "precision.h"
#include "digitlst.h"
#include "visibledigits.h"
#include "fmtableimp.h"
#include <math.h>

U_NAMESPACE_BEGIN

FixedPrecision::FixedPrecision() 
        : fExactOnly(FALSE), fFailIfOverMax(FALSE), fRoundingMode(DecimalFormat::kRoundHalfEven) {
    fMin.setIntDigitCount(1);
    fMin.setFracDigitCount(0);
}

UBool
FixedPrecision::isRoundingRequired(
        int32_t upperExponent, const DigitInterval &interval) const {
    int32_t leastSigAllowed = fMax.getLeastSignificantInclusive();
    int32_t maxSignificantDigits = fSignificant.getMax();
    int32_t roundDigit;
    if (maxSignificantDigits == INT32_MAX) {
        roundDigit = leastSigAllowed;
    } else {
        int32_t limitDigit = upperExponent - maxSignificantDigits;
        roundDigit =
                limitDigit > leastSigAllowed ? limitDigit : leastSigAllowed;
    }
    return (roundDigit > interval.getLeastSignificantInclusive());
}

DigitList &
FixedPrecision::round(
        DigitList &value, int32_t exponent, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return value;
    }
    value .fContext.status &= ~DEC_Inexact;
    if (!fRoundingIncrement.isZero()) {
        if (exponent == 0) {
            value.quantize(fRoundingIncrement, status);
        } else {
            DigitList adjustedIncrement(fRoundingIncrement);
            adjustedIncrement.shiftDecimalRight(exponent);
            value.quantize(adjustedIncrement, status);
        }
        if (U_FAILURE(status)) {
            return value;
        }
    }
    int32_t leastSig = fMax.getLeastSignificantInclusive();
    if (leastSig == INT32_MIN) {
        value.round(fSignificant.getMax());
    } else {
        value.roundAtExponent(
                exponent + leastSig,
                fSignificant.getMax());
    }
    if (fExactOnly && (value.fContext.status & DEC_Inexact)) {
        status = U_FORMAT_INEXACT_ERROR;
    } else if (fFailIfOverMax) {
        // Smallest interval for value stored in interval
        DigitInterval interval;
        value.getSmallestInterval(interval);
        if (fMax.getIntDigitCount() < interval.getIntDigitCount()) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    return value;
}

DigitInterval &
FixedPrecision::getIntervalForZero(DigitInterval &interval) const {
    interval = fMin;
    if (fSignificant.getMin() > 0) {
        interval.expandToContainDigit(interval.getIntDigitCount() - fSignificant.getMin());
    }
    interval.shrinkToFitWithin(fMax);
    return interval;
}

DigitInterval &
FixedPrecision::getInterval(
        int32_t upperExponent, DigitInterval &interval) const {
    if (fSignificant.getMin() > 0) {
        interval.expandToContainDigit(
                upperExponent - fSignificant.getMin());
    }
    interval.expandToContain(fMin);
    interval.shrinkToFitWithin(fMax);
    return interval;
}

DigitInterval &
FixedPrecision::getInterval(
        const DigitList &value, DigitInterval &interval) const {
    if (value.isZero()) {
        interval = fMin;
        if (fSignificant.getMin() > 0) {
            interval.expandToContainDigit(interval.getIntDigitCount() - fSignificant.getMin());
        }
    } else {
        value.getSmallestInterval(interval);
        if (fSignificant.getMin() > 0) {
            interval.expandToContainDigit(
                    value.getUpperExponent() - fSignificant.getMin());
        }
        interval.expandToContain(fMin);
    }
    interval.shrinkToFitWithin(fMax);
    return interval;
}

UBool
FixedPrecision::isFastFormattable() const {
    return (fMin.getFracDigitCount() == 0 && fSignificant.isNoConstraints() && fRoundingIncrement.isZero() && !fFailIfOverMax);
}

VisibleDigits &
FixedPrecision::initVisibleDigits(
        DigitList &value,
        VisibleDigits &digits,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return digits;
    }
    digits.clear();
    if (value.isNaN()) {
        digits.setNaN();
        return digits;
    }
    if (value.isInfinite()) {
        digits.setInfinite();
        if (!value.isPositive()) {
            digits.setNegative();
        }
        return digits;
    }
    if (!value.isPositive()) {
        digits.setNegative();
    }
    value.setRoundingMode(fRoundingMode);
    round(value, 0, status);
    getInterval(value, digits.fInterval);
    digits.fExponent = value.getLowerExponent();
    value.appendDigitsTo(digits.fDigits, status);
    return digits;
}

VisibleDigits &
FixedPrecision::initVisibleDigits(
        int64_t value,
        VisibleDigits &digits,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return digits;
    }
    if (!fRoundingIncrement.isZero()) {
        // If we have round increment, use digit list.
        DigitList digitList;
        digitList.set(value);
        return initVisibleDigits(digitList, digits, status);
    }
    // Try fast path
    if (initVisibleDigits(value, 0, digits, status)) {
        digits.fAbsDoubleValue = fabs((double) value);
        digits.fAbsDoubleValueSet = TRUE;
        return digits;
    }
    // Oops have to use digit list
    DigitList digitList;
    digitList.set(value);
    return initVisibleDigits(digitList, digits, status);
}

VisibleDigits &
FixedPrecision::initVisibleDigits(
        double value,
        VisibleDigits &digits,
        UErrorCode &status) const {
    static int32_t p10[] = {1, 10, 100, 1000};
    if (U_FAILURE(status)) {
        return digits;
    }
    digits.clear();
    if (uprv_isNaN(value)) {
        digits.setNaN();
        return digits;
    }
    if (uprv_isPositiveInfinity(value)) {
        digits.setInfinite();
        return digits;
    }
    if (uprv_isNegativeInfinity(value)) {
        digits.setInfinite();
        digits.setNegative();
        return digits;
    }
    if (!fRoundingIncrement.isZero()) {
        // If we have round increment, use digit list.
        DigitList digitList;
        digitList.set(value);
        return initVisibleDigits(digitList, digits, status);
    }
    // Try to find n such that value * 10^n is an integer
    int32_t n = -1;
    double scaled;
    for (int32_t i = 0; i < UPRV_LENGTHOF(p10); ++i) {
        scaled = value * p10[i];
        if (scaled > MAX_INT64_IN_DOUBLE) {
            break;
        }
        if (scaled == floor(scaled)) {
            n = i;
            break;
        }
    }
    // Try fast path
    if (n >= 0 && initVisibleDigits(scaled, -n, digits, status)) {
        digits.fAbsDoubleValue = fabs(value);
        digits.fAbsDoubleValueSet = TRUE;
        return digits;
    }

    // Oops have to use digit list
    DigitList digitList;
    digitList.set(value);
    return initVisibleDigits(digitList, digits, status);
}

UBool
FixedPrecision::initVisibleDigits(
        int64_t mantissa,
        int32_t exponent,
        VisibleDigits &digits,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return TRUE;
    }
    digits.clear();

    if (mantissa >= 0) {
        digits.fAbsIntValue = mantissa;
        for (int32_t i = 0; i > exponent; --i) {
            digits.fAbsIntValue /= 10;
        }
        digits.fAbsIntValueSet = TRUE;
    }
    if (mantissa == 0) {
        getIntervalForZero(digits.fInterval);
        return TRUE;
    }
    // be sure least significant digit is non zero
    while (mantissa % 10 == 0) {
        mantissa /= 10;
        ++exponent;
    }
    if (mantissa < 0) {
        digits.fDigits.append((char) -(mantissa % -10), status);
        mantissa /= -10;
        digits.setNegative();
    }
    while (mantissa) {
        digits.fDigits.append((char) (mantissa % 10), status);
        mantissa /= 10;
    }
    if (U_FAILURE(status)) {
        return TRUE;
    }
    digits.fExponent = exponent;
    int32_t upperExponent = exponent + digits.fDigits.length();
    if (fFailIfOverMax && upperExponent > fMax.getIntDigitCount()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return TRUE;
    }
    digits.fInterval.setLeastSignificantInclusive(exponent);
    digits.fInterval.setMostSignificantExclusive(upperExponent);
    UBool roundingRequired =
            isRoundingRequired(upperExponent, digits.fInterval);
    if (roundingRequired) {
        if (fExactOnly) {
            status = U_FORMAT_INEXACT_ERROR;
            return TRUE;
        }
        return FALSE;
    }
    getInterval(upperExponent, digits.fInterval);
    return TRUE;
}

DigitList &
ScientificPrecision::round(DigitList &value, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return value;
    }
    int32_t exponent = value.getScientificExponent(
            fMantissa.fMin.getIntDigitCount(), getMultiplier());
    return fMantissa.round(value, exponent, status);
}

int32_t
ScientificPrecision::toScientific(DigitList &value) const {
    return value.toScientific(
            fMantissa.fMin.getIntDigitCount(), getMultiplier());
}

int32_t
ScientificPrecision::getMultiplier() const {
    int32_t maxIntDigitCount = fMantissa.fMax.getIntDigitCount();
    if (maxIntDigitCount == INT32_MAX) {
        return 1;
    }
    int32_t multiplier =
        maxIntDigitCount - fMantissa.fMin.getIntDigitCount() + 1;
    return (multiplier < 1 ? 1 : multiplier);
}


U_NAMESPACE_END

