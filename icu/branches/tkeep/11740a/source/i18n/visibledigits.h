/* ******************************************************************************* * Copyright (C) 2015, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* digitinterval.h
*
* created on: 2015jan6
* created by: Travis Keep
*/

#ifndef __VISIBLEDIGITS_H__
#define __VISIBLEDIGITS_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "charstr.h"
#include "digitinterval.h"

U_NAMESPACE_BEGIN

class DigitList;

/**
 * An interval of digits.
 */
class U_I18N_API VisibleDigits : public UMemory {
public:
    VisibleDigits() : fExponent(0), fFlags(0), fAbsIntValue(0), fAbsIntValueSet(FALSE), fAbsDoubleValue(0.0), fAbsDoubleValueSet(FALSE) { }

    /**
     * For testing only. value must be real, not NaN or infinite.
     */
    static VisibleDigits &initVisibleDigits(
            const DigitList &value,
            const DigitInterval &interval,
            VisibleDigits &digits,
            UErrorCode &status);

    UBool isNegative() const;
    UBool isNaN() const;
    UBool isInfinite() const;
    UBool isNaNOrInfinity() const;

    /**
     * Gets the digit at particular exponent, if number is 987.6, then
     * getDigit(2) == 9 and gitDigit(0) == 7 and gitDigit(-1) == 6.
     * If isNaN() or isInfinity() return TRUE, then the result of this
     * function is undefined.
     */
    int32_t getDigitByExponent(int32_t digitPos) const;

    /**
     * Returns the digit interval which indicates the leftmost and rightmost
     * position of this instance. 
     * If isNaN() or isInfinity() return TRUE, then the result of this
     * function is undefined.
     */
    const DigitInterval &getInterval() const { return fInterval; }

    /**
     * Gets the parameters needed to create a FixedDecimal.
     */
    void getFixedDecimal(double &source, int64_t &intValue, int64_t &f, int64_t &t, int32_t &v, UBool &hasIntValue) const;


private:
    CharString fDigits;
    DigitInterval fInterval;
    int32_t fExponent;
    int32_t fFlags;
    int64_t fAbsIntValue;
    UBool fAbsIntValueSet;
    double fAbsDoubleValue;
    UBool fAbsDoubleValueSet;

    void setNegative();
    void setNaN();
    void setInfinite();
    void clear();
    double computeAbsDoubleValue() const;
    UBool isOverMaxDigits() const;

    VisibleDigits(const VisibleDigits &);
    VisibleDigits &operator=(const VisibleDigits &);

    friend class FixedPrecision;
};

U_NAMESPACE_END

#endif  // __VISIBLEDIGITS_H__
