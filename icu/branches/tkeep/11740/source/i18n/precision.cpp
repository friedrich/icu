/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: precisison.cpp
 */

#include "unicode/utypes.h"

#include "precision.h"
#include "digitlst.h"

U_NAMESPACE_BEGIN

FixedPrecision::FixedPrecision() : fExactOnly(FALSE), fFailIfOverMax(FALSE) {
    fMin.setIntDigitCount(1);
    fMin.setFracDigitCount(0);
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
        // TODO(refactor): Not most efficient way, but readable.
        DigitInterval maxWithUnboundedFracDigits(fMax);
        maxWithUnboundedFracDigits.setFracDigitCount(-1);

        // Smallest interval for value stored in interval
        DigitInterval interval;
        value.getSmallestInterval(interval);

        // newInterval will be interval shrunk as necessary
        // to accomodate max int digits.
        DigitInterval newInterval(interval);
        newInterval.shrinkToFitWithin(maxWithUnboundedFracDigits);

        // If newInterval != interval we exceeded max digits initially.
        if (!newInterval.equals(interval)) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    return value;
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

