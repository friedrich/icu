/*
**********************************************************************
*   Copyright (c) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   01/11/2000  aliu        Creation.
**********************************************************************
*/
#include "unicode/nultrans.h"

const char* NullTransliterator::_ID = "Null";

Transliterator* NullTransliterator::clone(void) const {
    return new NullTransliterator();
}

int32_t NullTransliterator::transliterate(Replaceable&, int32_t,
                                          int32_t limit) const {
    return limit;
}

void NullTransliterator::handleTransliterate(Replaceable&,
                                             int32_t offsets[3]) const {
    offsets[CURSOR] = offsets[LIMIT];
}
