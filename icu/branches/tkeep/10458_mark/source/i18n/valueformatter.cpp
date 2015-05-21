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

UnicodeString
ValueFormatter::select(
        const PluralRules &rules,
        const DigitList &value) const {
    switch (fType) {
    case kFixedDecimal:
        {
            DigitInterval interval;
            return rules.select(
                    FixedDecimal(
                            value,
                            fFixedPrecision->getInterval(value, interval)));
        }
        break;
    case kScientificNotation:
        return UnicodeString(TRUE, gOther, -1);
    default:
        U_ASSERT(FALSE);
        break;
    }
    return UnicodeString();
}

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

DigitList &
ValueFormatter::round(DigitList &value, UErrorCode &status) const {
    if (value.isNaN() || value.isInfinite()) {
        return value;
    }
    switch (fType) {
    case kFixedDecimal:
        return fFixedPrecision->round(value, 0, status);
    case kScientificNotation:
        return fScientificPrecision->round(value, status);
    default:
        U_ASSERT(FALSE);
        break;
    }
    return value;
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

UnicodeString &
ValueFormatter::format(
        const DigitList &value,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const {
    if (value.isNaN()) {
        return fDigitFormatter->formatNaN(handler, appendTo);
    }
    if (value.isInfinite()) {
        return fDigitFormatter->formatInfinity(handler, appendTo);
    }
    switch (fType) {
    case kFixedDecimal:
        {
            DigitInterval interval;
            return fDigitFormatter->format(
                    value,
                    *fGrouping,
                    fFixedPrecision->getInterval(value, interval),
                    *fFixedOptions,
                    handler,
                    appendTo);
        }
        break;
    case kScientificNotation:
        {
            DigitList mantissa(value);
            int32_t exponent = fScientificPrecision->toScientific(mantissa);
            DigitInterval interval;
            return fSciFormatter->format(
                    mantissa,
                    exponent,
                    *fDigitFormatter,
                    fScientificPrecision->fMantissa.getInterval(mantissa, interval),
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
ValueFormatter::countChar32(const DigitList &value) const {
    if (value.isNaN()) {
        return fDigitFormatter->countChar32ForNaN();
    }
    if (value.isInfinite()) {
        return fDigitFormatter->countChar32ForInfinity();
    }
    switch (fType) {
    case kFixedDecimal:
        {
            DigitInterval interval;
            return fDigitFormatter->countChar32(
                    *fGrouping,
                    fFixedPrecision->getInterval(value, interval),
                    *fFixedOptions);
        }
        break;
    case kScientificNotation:
        {
            DigitList mantissa(value);
            int32_t exponent = fScientificPrecision->toScientific(mantissa);
            DigitInterval interval;
            return fSciFormatter->countChar32(
                    exponent,
                    *fDigitFormatter,
                    fScientificPrecision->fMantissa.getInterval(mantissa, interval),
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
