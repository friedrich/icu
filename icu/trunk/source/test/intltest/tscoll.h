/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * MajorTestLevel is the top level test class for everything in the directory "IntlWork".
 */

#ifndef _INTLTESTCOLLATOR
#define _INTLTESTCOLLATOR

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "intltest.h"
#include "unicode/coll.h"
#include "unicode/sortkey.h"
#include "unicode/schriter.h"
#include "unicode/ures.h"
#include "unicode/coleitr.h"
#include "cmemory.h"


class IntlTestCollator: public IntlTest {
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );
protected:
    // These two should probably go down in IntlTest
    void doTest(Collator* col, const UChar *source, const UChar *target, UCollationResult result);

    void doTest(Collator* col, const UnicodeString &source, const UnicodeString &target, UCollationResult result);
    void doTestVariant(Collator* col, const UnicodeString &source, const UnicodeString &target, UCollationResult result);
    virtual void reportCResult( const UnicodeString &source, const UnicodeString &target,
                                CollationKey &sourceKey, CollationKey &targetKey,
                                UCollationResult compareResult,
                                UCollationResult keyResult,
                                UCollationResult incResult,
                                UCollationResult expectedResult );

    static UnicodeString &prettify(const CollationKey &source, UnicodeString &target);
    static UnicodeString &appendCompareResult(UCollationResult result, UnicodeString &target);
    void backAndForth(CollationElementIterator &iter);
    /**
     * Return an integer array containing all of the collation orders
     * returned by calls to next on the specified iterator
     */
    int32_t *getOrders(CollationElementIterator &iter, int32_t &orderLength);
    UCollationResult compareUsingPartials(UCollator *coll, const UChar source[], int32_t sLen, const UChar target[], int32_t tLen, int32_t pieceSize, UErrorCode &status);

};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
