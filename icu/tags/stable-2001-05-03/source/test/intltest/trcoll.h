/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * CollationTurkishTest is a third level test class.  This tests the locale
 * specific primary and tertiary rules.  For example, the dotless-i and dotted-I 
 * sorts between h and j.
 */

#ifndef _TRCOLL
#define _TRCOLL

#ifndef _UTYPES
#include "unicode/utypes.h"
#endif

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _INTLTEST
#include "intltest.h"
#endif

class CollationTurkishTest: public IntlTest {
public:
    // If this is too small for the test data, just increase it.
    // Just don't make it too large, otherwise the executable will get too big
    enum EToken_Len { MAX_TOKEN_LEN = 16 };

    CollationTurkishTest();
    virtual ~CollationTurkishTest();
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    // main test routine, tests rules specific to turkish locale
    void doTest( UnicodeString source, UnicodeString target, Collator::EComparisonResult result);

    // perform tests for turkish locale with strength PRIMARY
    void TestPrimary(/* char* par */);

    // perform tests for turkish locale with strength TERTIARY
    void TestTertiary(/* char* par */);

private:
    // static constants
    static const UChar testSourceCases[][MAX_TOKEN_LEN];
    static const UChar testTargetCases[][MAX_TOKEN_LEN];
    static const Collator::EComparisonResult results[];

    Collator *myCollation;
};
#endif
