/********************************************************************
 * Copyright (c) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#include "unicode/utypes.h"

#include "rbbimonkeytest.h"
#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/rbbi.h"
#include "unicode/regex.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"

#include "charstr.h"
#include "ucbuf.h"
#include "uvector.h"

#include "iostream"
#include "string"

void RBBIMonkeyTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* params) {

    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testMonkey);
    TESTCASE_AUTO_END;
}



//
//  CharClass    Represents a character class from the source break rules.
//
class CharClass {
    icu::CharString   name;
    icu::UnicodeSet   set;
};


class BreakRule {
  public:
    icu::UnicodeString    fName;            // Original name from source rules.
    icu::UnicodeString    fAdjustedName;    // Name, adjusted so it will compare in desired sort order.
                                         //   (First numeric field expanded with leading '0's)
    icu::UnicodeString    fRule;            // Rule expression, excluding the name.
    icu::RegexMatcher    *fRuleRegex;      // Regular expression that matches the rule.
};


// BreakRules represents a set of break rules, possibly tailored,
//  compiled from testdata break rules, for use by the monkey test reference implementation.

class BreakRules {
  public:
    BreakRules(UErrorCode &status) :fCharClasses(status), fBreakRules(status) {
    };

    void compileRules(UCHARBUF *rules, UErrorCode &status);

    icu::UVector     fCharClasses;   // Contents are of type (CharClass *)
    icu::UVector     fBreakRules;    // Contents are of type (BreakRule *).
                                     // Contents are ordered by the order of application.
};


void BreakRules::compileRules(UCHARBUF *rules, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    // Match comments and blank lines. Matches will be replaced with "", stripping the comments from the rules.
    RegexMatcher commentsMatcher(UnicodeString(
                "(^|(?<=;))"                    // Start either at start of line, or just after a ';' (look-behind for ';')
                "[ \\t]*+"                      //   Match white space.
                "(#.*)?+"                       //   Optional # plus whatever follows
                "\\n$"                          //   new-line at end of line.
            ), 0, status);
    const UnicodeString emptyString;

    // Match (initial parse) of a character class defintion line.
    RegexMatcher classDefMatcher(UnicodeString(
                "[ \\t]*"                                // leading white space
                "(?<ClassName>[A-Za-z_][A-Za-z0-9_]*)"   // The char class name
                "[ \\t]*=[ \\t]*"                        //   =
                "(?<ClassDef>.*?)"                       // The char class UnicodeSet expression
                "[ \\t]*;[ \\t]*$"),                     // ; <end of line>
            0, status);

    // Match (initial parse) of a break rule line.
    RegexMatcher ruleDefMatcher(UnicodeString(
                "[ \\t]*"                                // leading white space
                "(?<RuleName>[A-Za-z_][A-Za-z0-9_]*)"    // The rule name
                "[ \\t]*:[ \\t]*"                        //   :
                "(?<RuleDef>.*?)"                        // The rule definition
                "[ \\t]*;[ \\t]$"),                      // ; <end of line>
            0, status);




    for (int32_t lineNumber=0; ;lineNumber++) {    // Loop once per input line.
        if (U_FAILURE(status)) {
            return;
        }
        int32_t lineLength = 0;
        const UChar *lineBuf = ucbuf_readline(rules, &lineLength, &status);
        if (lineBuf == NULL) {
            break;
        }
        UnicodeString line(lineBuf, lineLength);

        // Strip comment lines.
        commentsMatcher.reset(line);
        line = commentsMatcher.replaceFirst(emptyString, status);
        if (line.isEmpty()) {
            continue;
        }

        // Recognize character class definition lines
        classDefMatcher.reset(line);
        if (classDefMatcher.matches(status)) {
            std::string cl;
            std::cout << "scanned class: "
                      << classDefMatcher.group(classDefMatcher.pattern().groupNumberFromName("ClassName", status), status).toUTF8String(cl)
                      << std::endl;
            continue;
        }

        if (U_FAILURE(status)) {
            break;
        }

        std::string s;
        std::cout << "unrecognized line: " << line.toUTF8String(s) << std::endl;

        
    }

}

RBBIMonkeyTest::RBBIMonkeyTest() {
}

RBBIMonkeyTest::~RBBIMonkeyTest() {
}


void RBBIMonkeyTest::testMonkey() {
    testRules("grapheme.txt", "char", "en");
    // testRules("word.txt", "word", "en");
    // testRules("line.txt", "line", "en");
}


static UCHARBUF *openBreakRules(const char *fileName, UErrorCode &status) {
    CharString path;
    path.append(IntlTest::getSourceTestData(status), status);
    path.append("break_rules" U_FILE_SEP_STRING, status);
    path.appendPathPart(fileName, status);
    const char *codePage = "UTF-8";
    UCHARBUF *charBuf = ucbuf_open(path.data(), &codePage, TRUE, FALSE, &status);
    return charBuf;
}
     

/* Run a monkey test on a set of break rules.
 * Each break type and tailoring of a break type results in a separate call to this function.
 *  @param ruleFile   The name of the break rules file for the reference implementation.
 *  @param breakType  The type of ICU break iterator to create for testing.
 *  @param locale     The locale for the ICU break iterator
 */
void RBBIMonkeyTest::testRules(const char *ruleFile,
                               const char *breakType,
                               const char *locale) {
    UErrorCode status = U_ZERO_ERROR;

   
    LocalUCHARBUFPointer ruleCharBuf(openBreakRules(ruleFile, status));
    if (U_FAILURE(status)) {
        errln("%s:%d Error %s opening file %s.", __FILE__, __LINE__, u_errorName(status), ruleFile);
        return;
    }
    BreakRules ruleSet(status);
    ruleSet.compileRules(ruleCharBuf.getAlias(), status);
    if (U_FAILURE(status)) {
        errln("%s:%d Error %s processing file %s.", __FILE__, __LINE__, u_errorName(status), ruleFile);
        return;
    }


}
