/*************************************************************************
 * Copyright (c) 2015, International Business Machines
 * Corporation and others. All Rights Reserved.
 *************************************************************************
*/
#ifndef RBBIMONKEYTEST_H
#define RBBIMONKEYTEST_H

#include "unicode/utypes.h"


#include "intltest.h"
#include "unicode/brkiter.h"


/**
 * Test the RuleBasedBreakIterator class giving different rules
 */
class RBBIMonkeyTest: public IntlTest {
  public:
    RBBIMonkeyTest();
    virtual ~RBBIMonkeyTest();

    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    void testMonkey();


  private:
    void testRules(const char *ruleFile);


};


#endif  //  RBBIMONKEYTEST_H
