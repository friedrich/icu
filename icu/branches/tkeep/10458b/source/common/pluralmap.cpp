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

int32_t pluralMap_getIndex(const char *pluralForm) {
    int32_t len = UPRV_LENGTHOF(gPluralForms);
    for (int32_t i = 0; i < len; ++i) {
        if (uprv_strcmp(pluralForm, gPluralForms[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int32_t pluralMap_getIndexByUniStr(const UnicodeString &pluralForm) {
    CharString cvariant;
    UErrorCode status = U_ZERO_ERROR;
    cvariant.appendInvariantChars(pluralForm, status);    
    return U_FAILURE(status) ? -1 : pluralMap_getIndex(cvariant.data());
}

const char *pluralMap_getName(int32_t index) {
    return (index < 0 || index > UPRV_LENGTHOF(gPluralForms)) ?
            NULL : gPluralForms[index];
}


U_NAMESPACE_END

