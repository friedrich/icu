/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _TBLCOLL
#include "unicode/tblcoll.h"
#endif

#ifndef _UNISTR
#include "unicode/unistr.h"
#endif

#ifndef _SORTKEY
#include "unicode/sortkey.h"
#endif

#ifndef _DACOLL
#include "dacoll.h"
#endif

CollationDanishTest::CollationDanishTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale("da", "DK", ""),status);
}

CollationDanishTest::~CollationDanishTest()
{
    delete myCollation;
}

const UChar CollationDanishTest::testSourceCases[][CollationDanishTest::MAX_TOKEN_LEN] = {
    {0x4c, 0x75, 0x63, 0},
    {0x6c, 0x75, 0x63, 0x6b, 0},
    {0x4c, 0x00FC, 0x62, 0x65, 0x63, 0x6b, 0},
    {0x4c, 0x00E4, 0x76, 0x69, 0},
    {0x4c, 0x00F6, 0x77, 0x77, 0},
    {0x4c, 0x76, 0x69, 0},
    {0x4c, 0x00E4, 0x76, 0x69, 0},
    {0x4c, 0x00FC, 0x62, 0x65, 0x63, 0x6b, 0}
};

const UChar CollationDanishTest::testTargetCases[][CollationDanishTest::MAX_TOKEN_LEN] = {
    {0x6c, 0x75, 0x63, 0x6b, 0},
    {0x4c, 0x00FC, 0x62, 0x65, 0x63, 0x6b, 0},
    {0x6c, 0x79, 0x62, 0x65, 0x63, 0x6b, 0},
    {0x4c, 0x00F6, 0x77, 0x65, 0},
    {0x6d, 0x61, 0x73, 0x74, 0},
    {0x4c, 0x77, 0x69, 0},
    {0x4c, 0x00F6, 0x77, 0x69, 0},
    {0x4c, 0x79, 0x62, 0x65, 0x63, 0x6b, 0}
};

const Collator::EComparisonResult CollationDanishTest::results[] = {
    Collator::LESS,
    Collator::LESS,
    Collator::GREATER,
    Collator::LESS,
    Collator::LESS,
    // test primary > 5
    Collator::EQUAL,
    Collator::LESS,
    Collator::EQUAL
};

const UChar CollationDanishTest::testBugs[][CollationDanishTest::MAX_TOKEN_LEN] = {
    {0x41, 0x2f, 0x53, 0},
    {0x41, 0x4e, 0x44, 0x52, 0x45, 0},
    {0x41, 0x4e, 0x44, 0x52, 0x00C9, 0},
    {0x41, 0x4e, 0x44, 0x52, 0x45, 0x41, 0x53, 0},
    {0x41, 0x53, 0},
    {0x43, 0x41, 0},
    {0x00C7, 0x41, 0},
    {0x43, 0x42, 0},
    {0x00C7, 0x43, 0},
    {0x44, 0x2e, 0x53, 0x2e, 0x42, 0x2e, 0},
    {0x44, 0x41, 0},                                                                           // 10
    {0x44, 0x42, 0},
    {0x44, 0x53, 0x42, 0},
    {0x44, 0x53, 0x43, 0},
    {0x45, 0x4b, 0x53, 0x54, 0x52, 0x41, 0x5f, 0x41, 0x52, 0x42, 0x45, 0x4a, 0x44, 0x45, 0},
    {0x45, 0x4b, 0x53, 0x54, 0x52, 0x41, 0x42, 0x55, 0x44, 0},
    {0x48, 0x00D8, 0x53, 0x54, 0},  // could the 0x00D8 be 0x2205?
    {0x48, 0x41, 0x41, 0x47, 0},                                                                 // 20
    {0x48, 0x00C5, 0x4e, 0x44, 0x42, 0x4f, 0x47, 0},
    {0x48, 0x41, 0x41, 0x4e, 0x44, 0x56, 0x00C6, 0x52, 0x4b, 0x53, 0x42, 0x41, 0x4e, 0x4b, 0x45, 0x4e, 0},
    {0x6b, 0x61, 0x72, 0x6c, 0},
    {0x4b, 0x61, 0x72, 0x6c, 0},
    {0x4e, 0x49, 0x45, 0x4c, 0x53, 0x45, 0x4e, 0},
    {0x4e, 0x49, 0x45, 0x4c, 0x53, 0x20, 0x4a, 0x00D8, 0x52, 0x47, 0x45, 0x4e, 0},
    {0x4e, 0x49, 0x45, 0x4c, 0x53, 0x2d, 0x4a, 0x00D8, 0x52, 0x47, 0x45, 0x4e, 0},
    {0x52, 0x00C9, 0x45, 0x2c, 0x20, 0x41, 0},
    {0x52, 0x45, 0x45, 0x2c, 0x20, 0x42, 0},
    {0x52, 0x00C9, 0x45, 0x2c, 0x20, 0x4c, 0},                                                    // 30
    {0x52, 0x45, 0x45, 0x2c, 0x20, 0x56, 0},
    {0x53, 0x43, 0x48, 0x59, 0x54, 0x54, 0x2c, 0x20, 0x42, 0},
    {0x53, 0x43, 0x48, 0x59, 0x54, 0x54, 0x2c, 0x20, 0x48, 0},
    {0x53, 0x43, 0x48, 0x00DC, 0x54, 0x54, 0x2c, 0x20, 0x48, 0},
    {0x53, 0x43, 0x48, 0x59, 0x54, 0x54, 0x2c, 0x20, 0x4c, 0},
    {0x53, 0x43, 0x48, 0x00DC, 0x54, 0x54, 0x2c, 0x20, 0x4d, 0},
    {0x53, 0x53, 0},
    {0x00DF, 0},
    {0x53, 0x53, 0x41, 0},
    {0x53, 0x54, 0x4f, 0x52, 0x45, 0x4b, 0x00C6, 0x52, 0},
    {0x53, 0x54, 0x4f, 0x52, 0x45, 0x20, 0x56, 0x49, 0x4c, 0x44, 0x4d, 0x4f, 0x53, 0x45, 0},               // 40
    {0x53, 0x54, 0x4f, 0x52, 0x4d, 0x4c, 0x59, 0},
    {0x53, 0x54, 0x4f, 0x52, 0x4d, 0x20, 0x50, 0x45, 0x54, 0x45, 0x52, 0x53, 0x45, 0x4e, 0},
    {0x54, 0x48, 0x4f, 0x52, 0x56, 0x41, 0x4c, 0x44, 0},
    {0x54, 0x48, 0x4f, 0x52, 0x56, 0x41, 0x52, 0x44, 0x55, 0x52, 0},
    {0x00FE, 0x4f, 0x52, 0x56, 0x41, 0x52, 0x0110, 0x55, 0x52, 0},
    {0x54, 0x48, 0x59, 0x47, 0x45, 0x53, 0x45, 0x4e, 0},
    {0x56, 0x45, 0x53, 0x54, 0x45, 0x52, 0x47, 0x00C5, 0x52, 0x44, 0x2c, 0x20, 0x41, 0},
    {0x56, 0x45, 0x53, 0x54, 0x45, 0x52, 0x47, 0x41, 0x41, 0x52, 0x44, 0x2c, 0x20, 0x41, 0},
    {0x56, 0x45, 0x53, 0x54, 0x45, 0x52, 0x47, 0x00C5, 0x52, 0x44, 0x2c, 0x20, 0x42, 0},                 // 50
    {0x00C6, 0x42, 0x4c, 0x45, 0},
    {0x00C4, 0x42, 0x4c, 0x45, 0},
    {0x00D8, 0x42, 0x45, 0x52, 0x47, 0},
    {0x00D6, 0x42, 0x45, 0x52, 0x47, 0},
    {0x0110, 0x41, 0},
    {0x0110, 0x43, 0}                                                                         // 54
};

const UChar CollationDanishTest::testNTList[][CollationDanishTest::MAX_TOKEN_LEN] = {
    {0x61, 0x6e, 0x64, 0x65, 0x72, 0x65, 0},
    {0x63, 0x68, 0x61, 0x71, 0x75, 0x65, 0},
    {0x63, 0x68, 0x65, 0x6d, 0x69, 0x6e, 0},
    {0x63, 0x6f, 0x74, 0x65, 0},
    {0x63, 0x6f, 0x74, 0x00e9, 0},
    {0x63, 0x00f4, 0x74, 0x65, 0},
    {0x63, 0x00f4, 0x74, 0x00e9, 0},
    {0x010d, 0x75, 0x010d, 0x0113, 0x74, 0},
    {0x43, 0x7a, 0x65, 0x63, 0x68, 0},
    {0x68, 0x69, 0x0161, 0x61, 0},
    {0x69, 0x72, 0x64, 0x69, 0x73, 0x63, 0x68, 0},
    {0x6c, 0x69, 0x65, 0},
    {0x6c, 0x69, 0x72, 0x65, 0},
    {0x6c, 0x6c, 0x61, 0x6d, 0x61, 0},
    {0x6c, 0x00f5, 0x75, 0x67, 0},
    {0x6c, 0x00f2, 0x7a, 0x61, 0},
    {0x6c, 0x75, 0x010d, 0},                                
    {0x6c, 0x75, 0x63, 0x6b, 0},
    {0x4c, 0x00fc, 0x62, 0x65, 0x63, 0x6b, 0},
    {0x6c, 0x79, 0x65, 0},                               /* 20 */
    {0x6c, 0x00e4, 0x76, 0x69, 0},
    {0x4c, 0x00f6, 0x77, 0x65, 0x6e, 0},
    {0x6d, 0x00e0, 0x0161, 0x74, 0x61, 0},
    {0x6d, 0x00ee, 0x72, 0},
    {0x6d, 0x79, 0x6e, 0x64, 0x69, 0x67, 0},
    {0x4d, 0x00e4, 0x6e, 0x6e, 0x65, 0x72, 0},
    {0x6d, 0x00f6, 0x63, 0x68, 0x74, 0x65, 0x6e, 0},
    {0x70, 0x69, 0x00f1, 0x61, 0},
    {0x70, 0x69, 0x6e, 0x74, 0},
    {0x70, 0x79, 0x6c, 0x6f, 0x6e, 0},
    {0x0161, 0x00e0, 0x72, 0x61, 0x6e, 0},
    {0x73, 0x61, 0x76, 0x6f, 0x69, 0x72, 0},
    {0x0160, 0x65, 0x72, 0x62, 0x016b, 0x72, 0x61, 0},
    {0x53, 0x69, 0x65, 0x74, 0x6c, 0x61, 0},
    {0x015b, 0x6c, 0x75, 0x62, 0},
    {0x73, 0x75, 0x62, 0x74, 0x6c, 0x65, 0},
    {0x73, 0x79, 0x6d, 0x62, 0x6f, 0x6c, 0},
    {0x73, 0x00e4, 0x6d, 0x74, 0x6c, 0x69, 0x63, 0x68, 0},
    {0x77, 0x61, 0x66, 0x66, 0x6c, 0x65, 0},
    {0x76, 0x65, 0x72, 0x6b, 0x65, 0x68, 0x72, 0x74, 0},
    {0x77, 0x6f, 0x6f, 0x64, 0},
    {0x76, 0x6f, 0x78, 0},                                 /* 40 */
    {0x76, 0x00e4, 0x67, 0x61, 0},
    {0x79, 0x65, 0x6e, 0},
    {0x79, 0x75, 0x61, 0x6e, 0},
    {0x79, 0x75, 0x63, 0x63, 0x61, 0},
    {0x017e, 0x61, 0x6c, 0},
    {0x017e, 0x65, 0x6e, 0x61, 0},
    {0x017d, 0x65, 0x6e, 0x0113, 0x76, 0x61, 0},
    {0x7a, 0x6f, 0x6f, 0},
    {0x5a, 0x76, 0x69, 0x65, 0x64, 0x72, 0x69, 0x6a, 0x61, 0},
    {0x5a, 0x00fc, 0x72, 0x69, 0x63, 0x68, 0},
    {0x7a, 0x79, 0x73, 0x6b, 0},             
    {0x00e4, 0x6e, 0x64, 0x65, 0x72, 0x65, 0}                  /* 53 */
};
void CollationDanishTest::doTest( UnicodeString source, UnicodeString target, Collator::EComparisonResult result)
{
    Collator::EComparisonResult compareResult = myCollation->compare(source, target);
    CollationKey sortKey1, sortKey2;
    UErrorCode key1status = U_ZERO_ERROR, key2status = U_ZERO_ERROR; //nos
    myCollation->getCollationKey(source, /*nos*/ sortKey1, key1status );
    myCollation->getCollationKey(target, /*nos*/ sortKey2, key2status );
    if (U_FAILURE(key1status) || U_FAILURE(key2status)) {
        errln("SortKey generation Failed.\n");
        return;
    }
    Collator::EComparisonResult keyResult = sortKey1.compareTo(sortKey2);
    reportCResult( source, target, sortKey1, sortKey2, compareResult, keyResult, result );
}

void CollationDanishTest::TestTertiary( char* par )
{
    int32_t i = 0;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 5 ; i++) {
        doTest(testSourceCases[i], testTargetCases[i], results[i]);
    }
    int32_t j = 0;
    logln("Test internet data list : ");
    for (i = 0; i < 53; i++) {
        for (j = i+1; j < 54; j++) {
            doTest(testBugs[i], testBugs[j], Collator::LESS);
        }
    }
    logln("Test NT data list : ");
    for (i = 0; i < 52; i++) {
        for (j = i+1; j < 53; j++) {
            doTest(testNTList[i], testNTList[j], Collator::LESS);
        }
    }
}
void CollationDanishTest::TestPrimary( char* par )
{
    int32_t i;
    myCollation->setStrength(Collator::PRIMARY);
    for (i = 5; i < 8; i++) {
        doTest(testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationDanishTest::runIndexedTest( int32_t index, UBool exec, char* &name, char* par )
{
    if (exec) logln("TestSuite CollationDanishTest: ");
    switch (index) {
        case 0: name = "TestPrimary";   if (exec)   TestPrimary( par ); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary( par ); break;
        default: name = ""; break;
    }
}


