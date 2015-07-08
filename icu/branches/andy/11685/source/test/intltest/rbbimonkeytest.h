/*************************************************************************
 * Copyright (c) 2015, International Business Machines
 * Corporation and others. All Rights Reserved.
 *************************************************************************
*/
#ifndef RBBIMONKEYTEST_H
#define RBBIMONKEYTEST_H

#include "unicode/utypes.h"

#include "intltest.h"

#include "unicode/rbbi.h"
#include "unicode/regex.h"
#include "unicode/unistr.h"

#include "ucbuf.h"
#include "uvector.h"

class BreakRules;       // Forward declaration

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

// The following classes are internal to the RBBI Monkey Test implementation.



//  class CharClass    Represents a single character class from the source break rules.

class CharClass {
  public:
    UnicodeString     fName;
    UnicodeString     fOriginalDef;    // set definition as it appeared in user supplied rules.
    UnicodeString     fExpandedDef;    // set definition with any embedded named sets replaced by their defs, recursively.
    CharClass(const UnicodeString &name, const UnicodeString &originalDef, const UnicodeString &expandedDef) :
            fName(name), fOriginalDef(originalDef), fExpandedDef(expandedDef) {}
};


// class BreakRule    represents a single rule from a set of break rules.
//                    Each rule has the set definitions expanded, and
//                    is compiled to a regular expression.

class BreakRule {
  public:
    BreakRule();
    ~BreakRule();
    UnicodeString    fName;                            // Original name from source rules.
    UnicodeString    fPaddedName;                      // Name, adjusted so it will compare in desired sort order.
                                                       //   (First numeric field expanded with leading '0's)
    UnicodeString    fRule;                            // Rule expression, excluding the name.
    UnicodeString    fExpandedRule;                    // Rule expression after expanding the set definitions.
    LocalPointer<RegexMatcher>  fRuleMatcher;          // Regular expression that matches the rule.
    static int8_t comparator(UElement a, UElement b);  // See uelement.h
};


// class BreakRules    represents a complete set of break rules, possibly tailored,
//                     compiled from testdata break rules.

class BreakRules {
  public:
    BreakRules(UErrorCode &status);

    void compileRules(UCHARBUF *rules, UErrorCode &status);

    
    icu::UVector       fBreakRules;        // Contents are of type (BreakRule *).
                                           // Contents are ordered by the order of application.
    UHashtable        *fCharClasses;       // Key is set name (UnicodeString).
                                           // Value is (CharClass *)
    LocalPointer<UVector>  fCharClassList; // Char Classes, same contents as fCharClasses values,
                                           //   but in a vector so they can be accessed by index.
    Locale             fLocale;
    UBreakIteratorType fType;

    void addCharClass(const UnicodeString &name, const UnicodeString &def, UErrorCode &status);
    void addRule(const UnicodeString &name, const UnicodeString &def, UErrorCode &status);
    bool setKeywordParameter(const UnicodeString &keyword, const UnicodeString &value, UErrorCode &status);
    RuleBasedBreakIterator *createICUBreakIterator(UErrorCode &status);

    LocalPointer<RegexMatcher> fSetRefsMatcher;
    LocalPointer<RegexMatcher> fCommentsMatcher;
    LocalPointer<RegexMatcher> fClassDefMatcher;
    LocalPointer<RegexMatcher> fRuleDefMatcher;
    LocalPointer<RegexMatcher> fNumericFieldMatcher;
};


// class MonkeyTestData    represents a randomly synthesized test data string together
//                         with the expected break positions obtained by applying
//                         the test break rules.

class MonkeyTestData {
  public:
    MonkeyTestData(UErrorCode &status);
    ~MonkeyTestData() {};
    void set(BreakRules *rules, IntlTest::icu_rand &rand, UErrorCode &status);

    UnicodeString          fString;            // The text.
    UnicodeString          fBoundaries;        // Parallel to fString. Non-zero if break preceding.
    LocalPointer<UVector>  fRuleForPosition;   // Pointer to BreakRule that applied at each position.
                                               // Also parallel to fString.

};


// class RBBIMonkeyImpl     holds (some indirectly) everything associated with running a monkey
//                          test for one set of break rules.
//
//                          (Future) in a multithreaded version, multiple instances of this class
//                          could exist concurrently, each testing a different set of rules or
//                          rule tailorings in a separate thread.
class RBBIMonkeyImpl {
  public:
    RBBIMonkeyImpl(const char *ruleFile, UErrorCode &status);
    ~RBBIMonkeyImpl();
    
    void runTest(int32_t numIterations, UErrorCode &status);

    LocalUCHARBUFPointer                 fRuleCharBuffer;
    LocalPointer<BreakRules>             fRuleSet;
    LocalPointer<RuleBasedBreakIterator> fBI;
    LocalPointer<MonkeyTestData>         fTestData;

  private:
    void openBreakRules(const char *fileName, UErrorCode &status);
};

#endif  //  RBBIMONKEYTEST_H
