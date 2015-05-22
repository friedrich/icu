/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#ifndef NUMERICVALUE_H
#define NUMERICVALUE_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"

#include "digitlst.h"
#include "digitinterval.h"


U_NAMESPACE_BEGIN

class PluralRules;

class NumericValue : public UMemory {
public:
    NumericValue() : fExponent(0), fIsScientific(FALSE) {
        fInterval.setIntDigitCount(1);
        fInterval.setFracDigitCount(0);
    }

    UnicodeString select(const PluralRules &rules) const;
    inline UBool isNaN() const { return fValue.isNaN(); }
    inline UBool isInfinite() const { return fValue.isInfinite(); }
    inline UBool isPositive() const { return fValue.isPositive(); }

    DigitList fValue;
    DigitInterval fInterval;
    int32_t fExponent;
    UBool fIsScientific;
};

U_NAMESPACE_END

#endif /* NUMERICVALUE_H */
