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
#include "unicode/uniset.h"
#include "unicode/unistr.h"

#include "ucbuf.h"
#include "uvector.h"

class BreakRules;       // Forward declaration
class RBBIMonkeyImpl;

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
    const char *fParams;                  // Copy of user parameters passed in from IntlTest.
    

    void testRules(const char *ruleFile);
    static UBool getIntParam(UnicodeString name, UnicodeString &params, int64_t &val, UErrorCode &status);
    static UBool getStringParam(UnicodeString name, UnicodeString &params, CharString &dest, UErrorCode &status);
    static UBool getBoolParam(UnicodeString name, UnicodeString &params, UBool &dest, UErrorCode &status);

};

// The following classes are internal to the RBBI Monkey Test implementation.



//  class CharClass    Represents a single character class from the source break rules.

class CharClass {
  public:
    UnicodeString                fName;
    UnicodeString                fOriginalDef;    // set definition as it appeared in user supplied rules.
    UnicodeString                fExpandedDef;    // set definition with any embedded named sets replaced by their defs, recursively.
    LocalPointer<const UnicodeSet>     fSet;
    CharClass(const UnicodeString &name, const UnicodeString &originalDef, const UnicodeString &expandedDef, const UnicodeSet *set) :
            fName(name), fOriginalDef(originalDef), fExpandedDef(expandedDef), fSet(set) {}
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
    BreakRules(RBBIMonkeyImpl *monkeyImpl, UErrorCode &status);

    void compileRules(UCHARBUF *rules, UErrorCode &status);

    const CharClass *getClassForChar(UChar32 c, int32_t *iter=NULL) const;

    
    RBBIMonkeyImpl    *fMonkeyImpl;        // Pointer back to the owning MonkeyImpl instance.
    icu::UVector       fBreakRules;        // Contents are of type (BreakRule *).
                                           // Contents are ordered by the order of application.
    UHashtable        *fCharClasses;       // Key is set name (UnicodeString).
                                           // Value is (CharClass *)
    LocalPointer<UVector>  fCharClassList; // Char Classes, same contents as fCharClasses values,
                                           //   but in a vector so they can be accessed by index.
    UnicodeSet         fDictionarySet;     // Dictionary set, empty if none is defined.
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
    void clearActualBreaks();
    void dump(int32_t around = -1) const;

    uint32_t               fRandomSeed;        // The initial seed value from the random number genererator.
    const BreakRules      *fBkRules;           // The break rules used to generate this data.
    UnicodeString          fString;            // The text.
    UnicodeString          fExpectedBreaks;    // Breaks as found by the reference rules.
                                               //     Parallel to fString. Non-zero if break preceding.
    UnicodeString          fActualBreaks;      // Breaks as found by ICU break iterator.
    UnicodeString          fRuleForPosition;   // Index into BreakRules.fBreakRules of rule that applied at each position.
                                               // Also parallel to fString.
    UnicodeString          f2ndRuleForPos;     // As above. A 2nd rule applies when the preceding rule
                                               //   didn't cause a break, and a subsequent rule match starts
                                               //   on the last code point of the preceding match.

};


// class RBBIMonkeyImpl     holds (some indirectly) everything associated with running a monkey
//                          test for one set of break rules.
//
//                          (Future) in a multithreaded version, multiple instances of this class
//                          could exist concurrently, each testing a different set of rules or
//                          rule tailorings in a separate thread.
class RBBIMonkeyImpl {
  public:
    RBBIMonkeyImpl(UErrorCode &status);
    ~RBBIMonkeyImpl();
    
    void setup(const char *ruleFileName, UErrorCode &status);

    void runTest(int32_t numIterations, UErrorCode &status);

    LocalUCHARBUFPointer                 fRuleCharBuffer;
    LocalPointer<BreakRules>             fRuleSet;
    LocalPointer<RuleBasedBreakIterator> fBI;
    LocalPointer<MonkeyTestData>         fTestData;
    IntlTest::icu_rand                   fRandomGenerator;
    const char                          *fRuleFileName;
    UBool                                fVerbose;                 // True to do long dump of failing data.

    UBool fDumpExpansions;               // Debug flag to output epananded form of rules and sets.

    enum CheckDirection {
        FORWARD = 1,
        REVERSE = 2
    };
    void clearActualBreaks();
    void testForwards(UErrorCode &status);
    void checkResults(CheckDirection dir, UErrorCode &status);

  private:
    void openBreakRules(const char *fileName, UErrorCode &status);

};

#endif  //  RBBIMONKEYTEST_H
