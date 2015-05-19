/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "valueformatter.h"

#include "digitinterval.h"
#include "digitlst.h"
#include "digitformatter.h"
#include "sciformatter.h"
#include "precision.h"
#include "unicode/unistr.h"
#include "unicode/plurrule.h"
#include "plurrule_impl.h"
#include "uassert.h"
#include "smallintformatter.h"
#include "digitgrouping.h"

U_NAMESPACE_BEGIN

const UChar gOther[] = {0x6f, 0x74, 0x68, 0x65, 0x72, 0x0};

static UBool isNoGrouping(
        const DigitGrouping &grouping,
        int32_t value,
        const FixedPrecision &precision) {
    IntDigitCountRange range(
            precision.fMin.getIntDigitCount(),
            precision.fMax.getIntDigitCount());
    return grouping.isNoGrouping(value, range);
}

UBool
ValueFormatter::isFastFormattable(int32_t value) const {
    switch (fType) {
    case kFixedDecimal:
        {
            if (value == INT32_MIN) {
                return FALSE;
            }
            if (value < 0) {
                value = -value;
            }
            return fFixedPrecision->isFastFormattable() && fFixedOptions->isFastFormattable() && isNoGrouping(*fGrouping, value, *fFixedPrecision);
        }
    case kScientificNotation:
        return FALSE;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return FALSE;
}

UnicodeString &
ValueFormatter::formatInt32(
        int32_t value,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    switch (fType) {
    case kFixedDecimal:
        {
            IntDigitCountRange range(
                    fFixedPrecision->fMin.getIntDigitCount(),
                    fFixedPrecision->fMax.getIntDigitCount());
            return fDigitFormatter->formatPositiveInt32(
                    value,
                    range,
                    handler,
                    appendTo);
        }
        break;
    case kScientificNotation:
    default:
        U_ASSERT(FALSE);
        break;
    }
    return appendTo;
}

NumericValue &
ValueFormatter::initNumericValue(
        const DigitList &digitList,
        NumericValue &value,
        UErrorCode &status) const {
    switch (fType) {
    case kFixedDecimal:
        {
            return fFixedPrecision->initNumericValue(digitList, value, status);
        }
        break;
    case kScientificNotation:
        {
            return fScientificPrecision->initNumericValue(
                    digitList, value, status);
        }
        break;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return value;
}

UnicodeString &
ValueFormatter::format(
        const NumericValue &value,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    switch (fType) {
    case kFixedDecimal:
        {
            return fDigitFormatter->format(
                    value,
                    *fGrouping,
                    *fFixedOptions,
                    handler,
                    appendTo);
        }
        break;
    case kScientificNotation:
        {
            return fSciFormatter->format(
                    value,
                    *fDigitFormatter,
                    *fScientificOptions,
                    handler,
                    appendTo);
        }
        break;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return appendTo;
}

int32_t
ValueFormatter::countChar32(const NumericValue &value) const {
    switch (fType) {
    case kFixedDecimal:
        {
            return fDigitFormatter->countChar32(
                    value,
                    *fGrouping,
                    *fFixedOptions);
        }
        break;
    case kScientificNotation:
        {
            return fSciFormatter->countChar32(
                    value,
                    *fDigitFormatter,
                    *fScientificOptions);
        }
        break;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return 0;
}

void
ValueFormatter::prepareFixedDecimalFormatting(
        const DigitFormatter &formatter,
        const DigitGrouping &grouping,
        const FixedPrecision &precision,
        const DigitFormatterOptions &options) {
    fType = kFixedDecimal;
    fDigitFormatter = &formatter;
    fGrouping = &grouping;
    fFixedPrecision = &precision;
    fFixedOptions = &options;
}

void
ValueFormatter::prepareScientificFormatting(
        const SciFormatter &sciformatter,
        const DigitFormatter &formatter,
        const ScientificPrecision &precision,
        const SciFormatterOptions &options) {
    fType = kScientificNotation;
    fSciFormatter = &sciformatter;
    fDigitFormatter = &formatter;
    fScientificPrecision = &precision;
    fScientificOptions = &options;
}

U_NAMESPACE_END

#endif /* !UCONFIG_NO_FORMATTING */
