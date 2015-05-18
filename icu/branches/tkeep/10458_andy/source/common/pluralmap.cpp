/*
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 */

#include "pluralmap.h"
#include "cstring.h"
#include "unicode/unistr.h"
#include "charstr.h"

U_NAMESPACE_BEGIN

static const char * const gPluralForms[] = {
        "other", "zero", "one", "two", "few", "many"};

PluralMapBase::Variant
PluralMapBase::toVariant(const char *pluralForm) {
    for (int32_t i = 0; i < UPRV_LENGTHOF(gPluralForms); ++i) {
        if (uprv_strcmp(pluralForm, gPluralForms[i]) == 0) {
            return static_cast<Variant>(i);
        }
    }
    return NONE;
}

PluralMapBase::Variant
PluralMapBase::toVariant(const UnicodeString &pluralForm) {
    CharString cvariant;
    UErrorCode status = U_ZERO_ERROR;
    cvariant.appendInvariantChars(pluralForm, status);    
    return U_FAILURE(status) ? NONE : toVariant(cvariant.data());
}

const char *PluralMapBase::getVariantName(Variant v) {
    int32_t index = v;
    return (index < 0 || index >= UPRV_LENGTHOF(gPluralForms)) ?
            NULL : gPluralForms[index];
}


U_NAMESPACE_END

