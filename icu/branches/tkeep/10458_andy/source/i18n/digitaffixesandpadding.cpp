/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: digitformatter.cpp
 */

#include "digitaffixesandpadding.h"

#include "digitlst.h"
#include "digitaffix.h"
#include "valueformatter.h"
#include "uassert.h"
#include "charstr.h"
#include "numericvalue.h"

U_NAMESPACE_BEGIN

UBool
DigitAffixesAndPadding::needsPluralRules() const {
    return (
            fPositivePrefix.hasMultipleVariants() ||
            fPositiveSuffix.hasMultipleVariants() ||
            fNegativePrefix.hasMultipleVariants() ||
            fNegativeSuffix.hasMultipleVariants());
}

UnicodeString &
DigitAffixesAndPadding::formatInt32(
        int32_t value,
        const ValueFormatter &formatter,
        FieldPositionHandler &handler,
        const PluralRules *optPluralRules,
        UnicodeString &appendTo,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (optPluralRules != NULL || fWidth > 0 || !formatter.isFastFormattable(value)) {
        DigitList digitList;
        digitList.set(value);
        return format(
                digitList,
                formatter,
                handler,
                optPluralRules,
                appendTo,
                status);
    }
    UBool bPositive = value >= 0;
    const DigitAffix *prefix = bPositive ? &fPositivePrefix.getOtherVariant() : &fNegativePrefix.getOtherVariant();
    const DigitAffix *suffix = bPositive ? &fPositiveSuffix.getOtherVariant() : &fNegativeSuffix.getOtherVariant();

    // This is safe because if value == INT32_MIN then it won't be fast
    // formattable and won't get here because it will return up above.
    if (value < 0) {
        value = -value;
    }
    prefix->format(handler, appendTo);
    formatter.formatInt32(value, handler, appendTo);
    return suffix->format(handler, appendTo);
}

static UnicodeString &
formatAffix(
        const DigitAffix *affix,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) {
    if (affix) {
        affix->format(handler, appendTo);
    }
    return appendTo;
}

static int32_t
countAffixChar32(const DigitAffix *affix) {
    if (affix) {
        return affix->countChar32();
    }
    return 0;
}

UnicodeString &
DigitAffixesAndPadding::format(
        DigitList &digitList,
        const ValueFormatter &formatter,
        FieldPositionHandler &handler,
        const PluralRules *optPluralRules,
        UnicodeString &appendTo,
        UErrorCode &status) const {
    NumericValue value;
    formatter.initNumericValue(digitList, value, status);
    if (U_FAILURE(status)) {
        return appendTo;
    }
    const DigitAffix *prefix = NULL;
    const DigitAffix *suffix = NULL;
    if (!value.isNaN()) {
        UBool bPositive = value.isPositive();
        const PluralAffix *pluralPrefix = bPositive ? &fPositivePrefix : &fNegativePrefix;
        const PluralAffix *pluralSuffix = bPositive ? &fPositiveSuffix : &fNegativeSuffix;
        if (optPluralRules == NULL || value.isInfinite()) {
            prefix = &pluralPrefix->getOtherVariant();
            suffix = &pluralSuffix->getOtherVariant();
        } else {
            UnicodeString count(value.select(*optPluralRules));
            prefix = &pluralPrefix->getByVariant(count);
            suffix = &pluralSuffix->getByVariant(count);
        }
    }
    if (fWidth <= 0) {
        formatAffix(prefix, handler, appendTo);
        formatter.format(value, handler, appendTo);
        return formatAffix(suffix, handler, appendTo);
    }
    int32_t codePointCount = countAffixChar32(prefix) + formatter.countChar32(value) + countAffixChar32(suffix);
    int32_t paddingCount = fWidth - codePointCount;
    switch (fPadPosition) {
    case kPadBeforePrefix:
        appendPadding(paddingCount, appendTo);
        formatAffix(prefix, handler, appendTo);
        formatter.format(value, handler, appendTo);
        return formatAffix(suffix, handler, appendTo);
    case kPadAfterPrefix:
        formatAffix(prefix, handler, appendTo);
        appendPadding(paddingCount, appendTo);
        formatter.format(value, handler, appendTo);
        return formatAffix(suffix, handler, appendTo);
    case kPadBeforeSuffix:
        formatAffix(prefix, handler, appendTo);
        formatter.format(value, handler, appendTo);
        appendPadding(paddingCount, appendTo);
        return formatAffix(suffix, handler, appendTo);
    case kPadAfterSuffix:
        formatAffix(prefix, handler, appendTo);
        formatter.format(value, handler, appendTo);
        formatAffix(suffix, handler, appendTo);
        return appendPadding(paddingCount, appendTo);
    default:
        U_ASSERT(FALSE);
        return appendTo;
    }
}

UnicodeString &
DigitAffixesAndPadding::appendPadding(int32_t paddingCount, UnicodeString &appendTo) const {
    for (int32_t i = 0; i < paddingCount; ++i) {
        appendTo.append(fPadChar);
    }
    return appendTo;
}


U_NAMESPACE_END

