/*
******************************************************************************
* Copyright (C) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
******************************************************************************
* quantityformatter.cpp
*/
#include "quantityformatter.h"
#include "simplepatternformatter.h"
#include "uassert.h"
#include "unicode/unistr.h"
#include "unicode/decimfmt.h"
#include "cstring.h"
#include "plurrule_impl.h"
#include "unicode/plurrule.h"
#include "charstr.h"
#include "unicode/fmtable.h"
#include "unicode/fieldpos.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

void QuantityFormatter::reset() {
    formatters.reset();
    bValid = FALSE;
}

UBool QuantityFormatter::add(
        const char *variant,
        const UnicodeString &rawPattern,
        UErrorCode &status) {
    int32_t pluralIndex = pluralMap_getIndex(variant);
    SimplePatternFormatter *current = formatters.getMutable(
            pluralIndex, status);
    SimplePatternFormatter fmt;
    fmt.compile(rawPattern, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (fmt.getPlaceholderCount() > 1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    if (pluralIndex == 0) {
        bValid = TRUE;
    }
    *current = fmt;
    return TRUE;
}

UBool QuantityFormatter::isValid() const {
    return bValid;
}

const SimplePatternFormatter *QuantityFormatter::getByVariant(
        const char *variant) const {
    return &formatters.get(variant);
}

UnicodeString &QuantityFormatter::format(
            const Formattable& quantity,
            const NumberFormat &fmt,
            const PluralRules &rules,
            UnicodeString &appendTo,
            FieldPosition &pos,
            UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    UnicodeString count;
    const DecimalFormat *decFmt = dynamic_cast<const DecimalFormat *>(&fmt);
    if (decFmt != NULL) {
        FixedDecimal fd = decFmt->getFixedDecimal(quantity, status);
        if (U_FAILURE(status)) {
            return appendTo;
        }
        count = rules.select(fd);
    } else {
        if (quantity.getType() == Formattable::kDouble) {
            count = rules.select(quantity.getDouble());
        } else if (quantity.getType() == Formattable::kLong) {
            count = rules.select(quantity.getLong());
        } else if (quantity.getType() == Formattable::kInt64) {
            count = rules.select((double) quantity.getInt64());
        } else {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return appendTo;
        }
    }
    CharString buffer;
    buffer.appendInvariantChars(count, status);
    if (U_FAILURE(status)) {
        return appendTo;
    }
    const SimplePatternFormatter *pattern = getByVariant(buffer.data());
    UnicodeString formattedNumber;
    FieldPosition fpos(pos.getField());
    fmt.format(quantity, formattedNumber, fpos, status);
    const UnicodeString *params[1] = {&formattedNumber};
    int32_t offsets[1];
    pattern->formatAndAppend(
            params,
            UPRV_LENGTHOF(params),
            appendTo,
            offsets,
            UPRV_LENGTHOF(offsets),
            status);
    if (offsets[0] != -1) {
        if (fpos.getBeginIndex() != 0 || fpos.getEndIndex() != 0) {
            pos.setBeginIndex(fpos.getBeginIndex() + offsets[0]);
            pos.setEndIndex(fpos.getEndIndex() + offsets[0]);
        }
    }
    return appendTo;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
