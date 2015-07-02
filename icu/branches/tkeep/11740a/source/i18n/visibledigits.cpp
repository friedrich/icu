/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: visibledigits.cpp
 */

#include "unicode/utypes.h"

#include "visibledigits.h"
#include "digitlst.h"

static const int32_t kNegative = 1;
static const int32_t kInfinite = 2;
static const int32_t kNaN = 4;

U_NAMESPACE_BEGIN

VisibleDigits &VisibleDigits::initVisibleDigits(
            const DigitList &value,
            const DigitInterval &interval,
            VisibleDigits &digits,
            UErrorCode &status) {
    digits.fInterval = interval;
    digits.fExponent = value.getLowerExponent();
    value.appendDigitsTo(digits.fDigits, status);
    if (!value.isPositive()) {
        digits.setNegative();
    }
    return digits;
}

void VisibleDigits::setNegative() {
    fFlags |= kNegative;
}

void VisibleDigits::setNaN() {
    fFlags |= kNaN;
}

void VisibleDigits::setInfinite() {
    fFlags |= kInfinite;
}

void VisibleDigits::clear() {
    fInterval.clear();
    fDigits.clear();
    fExponent = 0;
    fFlags = 0;
    fIntValue = 0LL;
    fIsInitializedFromInt = FALSE;
}

UBool VisibleDigits::isNegative() const {
    return (fFlags & kNegative);
}

UBool VisibleDigits::isNaN() const {
    return (fFlags & kNaN);
}

UBool VisibleDigits::isInfinite() const {
    return (fFlags & kInfinite);
}

int32_t VisibleDigits::getDigitByExponent(int32_t digitPos) const {
    if (digitPos < fExponent || digitPos >= fExponent + fDigits.length()) {
        return 0;
    }
    const char *ptr = fDigits.data();
    return ptr[digitPos - fExponent];
}

UBool VisibleDigits::isNaNOrInfinity() const {
    return (fFlags & (kInfinite | kNaN)) != 0;
}

void VisibleDigits::getFixedDecimal(
    int64_t &intValue, int64_t &f, int64_t &t, int32_t &v, UBool &hasIntValue) const {
    intValue = 0;
    f = 0;
    t = 0;
    v = 0;
    hasIntValue = FALSE;
    if (isNaNOrInfinity()) {
        return;
    }

    // visible decimal digits
    v = fInterval.getFracDigitCount();

    // intValue

    // If we initialized from an int64 just use that instead of
    // calculating
    if (fIsInitializedFromInt) {
        intValue = fIntValue;
    } else {
        int32_t startPos = fInterval.getMostSignificantExclusive();
        int32_t endPos = 0;
        for (; startPos - endPos > 18 && getDigitByExponent(endPos) == 0; ++endPos);
        if (startPos - endPos > 18) {
            startPos = 18;
            endPos = 0;
        }
        // process the integer digits
        for (int32_t i = startPos - 1; i >= endPos; --i) {
            intValue = intValue * 10LL + getDigitByExponent(i);
        }
    }

    // f (decimal digits)
    // skip over any leading 0's in fraction digits.
    int32_t idx = -1;
    for (; idx >= -v && getDigitByExponent(idx) == 0; --idx);

    // Only process up to first 18 non zero fraction digits for decimalDigits
    // since that is all we can fit into an int64.
    for (int32_t i = idx; i >= -v && i > idx - 18; --i) {
        f = f * 10LL + getDigitByExponent(i);
    }

    // If we have no decimal digits, we don't have an integer value
    hasIntValue = (f == 0LL);

    // t (decimal digits without trailing zeros)
   t = f;
    while (t > 0 && t % 10LL == 0) {
        t /= 10;
    }
}

U_NAMESPACE_END

