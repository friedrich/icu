/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*/

#include "numericvalue.h"
#include "unicode/utypes.h"
#include "unicode/plurrule.h"
#include "unicode/unistr.h"
#include "plurrule_impl.h"

U_NAMESPACE_BEGIN

const UChar gOther[] = {0x6f, 0x74, 0x68, 0x65, 0x72, 0x0};

UnicodeString
NumericValue::select(
        const PluralRules &rules) const {
    if (fIsScientific || isNaN() || isInfinite()) {
        return UnicodeString(TRUE, gOther, -1);
    }
    return rules.select(FixedDecimal(fValue, fInterval));
}

U_NAMESPACE_END

