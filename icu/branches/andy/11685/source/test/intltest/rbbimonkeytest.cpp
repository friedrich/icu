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
#include "uelement.h"
#include "uhash.h"
#include "uvector.h"

#include "iostream"
#include "string"

using namespace icu;

// Debugging function
static std::string stdstr(const UnicodeString &s) { std::string ss; return s.toUTF8String(ss); };


void RBBIMonkeyTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* params) {

    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testMonkey);
    TESTCASE_AUTO_END;
}



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
    UnicodeString    fName;                         // Original name from source rules.
    UnicodeString    fPaddedName;                   // Name, adjusted so it will compare in desired sort order.
                                                    //   (First numeric field expanded with leading '0's)
    UnicodeString    fRule;                         // Rule expression, excluding the name.
    UnicodeString    fExpandedRule;                 // Rule expression after expanding the set definitions.
    LocalPointer<RegexMatcher>  fRuleMatcher;       // Regular expression that matches the rule.
    static int8_t comparator(UElement a, UElement b);  // See uelement.h
};

int8_t BreakRule::comparator(UElement a, UElement b) {
    BreakRule *bra = static_cast<BreakRule *>(a.pointer);
    BreakRule *brb = static_cast<BreakRule *>(b.pointer);
    return bra->fPaddedName.compare(brb->fPaddedName);
}


// class BreakRules    represents a complete set of break rules, possibly tailored,
//                     compiled from testdata break rules.

class BreakRules {
  public:
    BreakRules(UErrorCode &status);

    void compileRules(UCHARBUF *rules, UErrorCode &status);

    
    icu::UVector       fBreakRules;     // Contents are of type (BreakRule *).
                                        // Contents are ordered by the order of application.
    UHashtable        *fCharClasses;    // Key is set name (UnicodeString).
                                        // Value is (CharClass *)

    CharString         fLocale;
    UBreakIteratorType fType;

    void addCharClass(const UnicodeString &name, const UnicodeString &def, UErrorCode &status);
    void addRule(const UnicodeString &name, const UnicodeString &def, UErrorCode &status);
    bool setKeywordParameter(const UnicodeString &keyword, const UnicodeString &value, UErrorCode &status);

    LocalPointer<RegexMatcher> fSetRefsMatcher;
    LocalPointer<RegexMatcher> fCommentsMatcher;
    LocalPointer<RegexMatcher> fClassDefMatcher;
    LocalPointer<RegexMatcher> fRuleDefMatcher;
    LocalPointer<RegexMatcher> fNumericFieldMatcher;
};


BreakRules::BreakRules(UErrorCode &status)  : fBreakRules(status), fCharClasses(NULL) {
    fCharClasses = uhash_open(uhash_hashUnicodeString,
                              uhash_compareUnicodeString,
                              NULL,      // value comparator.
                              &status);

    fSetRefsMatcher.adoptInstead(new RegexMatcher(UnicodeString(
             "(?!(?:\\{|=|\\[:)[ \\t]{0,4})"              // Negative lookbehind for '{' or '=' or '[:'
                                                          //   (the identifier is a unicode property name or value)
             "(?<ClassName>[A-Za-z_][A-Za-z0-9_]*)"),     // The char class name
        0, status));

    // Match comments and blank lines. Matches will be replaced with "", stripping the comments from the rules.
    fCommentsMatcher.adoptInstead(new RegexMatcher(UnicodeString(
                "(^|(?<=;))"                    // Start either at start of line, or just after a ';' (look-behind for ';')
                "[ \\t]*+"                      //   Match white space.
                "(#.*)?+"                       //   Optional # plus whatever follows
                "\\n$"                          //   new-line at end of line.
            ), 0, status));

    // Match (initial parse) of a character class defintion line.
    fClassDefMatcher.adoptInstead(new RegexMatcher(UnicodeString(
                "[ \\t]*"                                // leading white space
                "(?<ClassName>[A-Za-z_][A-Za-z0-9_]*)"   // The char class name
                "[ \\t]*=[ \\t]*"                        //   =
                "(?<ClassDef>.*?)"                       // The char class UnicodeSet expression
                "[ \\t]*;$"),                     // ; <end of line>
            0, status));

    // Match (initial parse) of a break rule line.
    fRuleDefMatcher.adoptInstead(new RegexMatcher(UnicodeString(
                "[ \\t]*"                                // leading white space
                "(?<RuleName>[A-Za-z_][A-Za-z0-9_]*)"    // The rule name
                "[ \\t]*:[ \\t]*"                        //   :
                "(?<RuleDef>.*?)"                        // The rule definition
                "[ \\t]*;$"),                            // ; <end of line>
            0, status));

    // Find a numeric field within a rule name.
    fNumericFieldMatcher.adoptInstead(new RegexMatcher(UnicodeString(
                 "[0-9]+"), 0, status));
}


void BreakRules::addCharClass(const UnicodeString &name, const UnicodeString &definition, UErrorCode &status) {
    
    // Create the expanded definition for this char class,
    // replacing any set references with the corresponding definition.

    UnicodeString expandedDef;
    UnicodeString emptyString;
    fSetRefsMatcher->reset(definition);
    while (fSetRefsMatcher->find() && U_SUCCESS(status)) {
        const UnicodeString name = 
                fSetRefsMatcher->group(fSetRefsMatcher->pattern().groupNumberFromName("ClassName", status), status); 
        CharClass *nameClass = static_cast<CharClass *>(uhash_get(fCharClasses, &name));
        const UnicodeString &expansionForName = nameClass ? nameClass->fExpandedDef : name;

        fSetRefsMatcher->appendReplacement(expandedDef, emptyString, status);
        expandedDef.append(expansionForName);
    }
    fSetRefsMatcher->appendTail(expandedDef);

    // Verify that the expanded set defintion is valid.

    std::cout << "expandedDef: " << stdstr(expandedDef) << std::endl;

    UnicodeSet s(expandedDef, USET_IGNORE_SPACE, NULL, status);
    if (U_FAILURE(status)) {
        return;
    }

    CharClass *cclass = new CharClass(name, definition, expandedDef);
    CharClass *previousClass = static_cast<CharClass *>(uhash_put(fCharClasses, &cclass->fName, cclass, &status));
    if (previousClass != NULL) {
        // Duplicate class def.
        // These are legitimate, they are adustments of an existing class.
        // TODO: will need to keep the old around when we handle tailorings.
        std::cout << "Duplicate definition of " << stdstr(cclass->fName) << std::endl;
        delete previousClass;
    }

}


void BreakRules::addRule(const UnicodeString &name, const UnicodeString &definition, UErrorCode &status) {
    // If the rule name contains embedded digits, pad the first such field with leading zeroes,
    // This gives a numeric sort order that matches Unicode UAX rule numbering conventions.
    UnicodeString paddedName;
    UnicodeString emptyString;
    fNumericFieldMatcher->reset(name);
    if (fNumericFieldMatcher->find()) {
        fNumericFieldMatcher->appendReplacement(paddedName, emptyString, status);
        int32_t paddingWidth = 6 - fNumericFieldMatcher->group(status).length();
        if (paddingWidth > 0) {
            paddedName.padTrailing(paddedName.length() + paddingWidth, (UChar)0x30);
        }
        paddedName.append(fNumericFieldMatcher->group(status));
    }
    fNumericFieldMatcher->appendTail(paddedName);
    // std::cout << stdstr(paddedName) << std::endl;

    // Expand the char class definitions within the rule.
    UnicodeString expandedDef;
    fSetRefsMatcher->reset(definition);
    while (fSetRefsMatcher->find() && U_SUCCESS(status)) {
        const UnicodeString name = 
                fSetRefsMatcher->group(fSetRefsMatcher->pattern().groupNumberFromName("ClassName", status), status); 
        CharClass *nameClass = static_cast<CharClass *>(uhash_get(fCharClasses, &name));
        const UnicodeString &expansionForName = nameClass ? nameClass->fExpandedDef : name;

        fSetRefsMatcher->appendReplacement(expandedDef, emptyString, status);
        expandedDef.append(expansionForName);
    }
    fSetRefsMatcher->appendTail(expandedDef);
    // std::cout << "expandedDef: " << stdstr(expandedDef) << std::endl;

    // Replace the divide sign (\u00f7) with a regular expression named capture.
    // When running the rules, a match that includes this group means we found a break position.

    int32_t dividePos = expandedDef.indexOf((UChar)0x00f7);
    if (dividePos >= 0) {
        expandedDef.replace(dividePos, 1, UnicodeString("(<BreakPosition>)"));
    }
    if (expandedDef.indexOf((UChar)0x00f7) != -1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;   // TODO: produce a good error message.
    }

    // UAX break rule set definitions can be empty, just [].
    // Regular expression set expressions don't accept this. Substiture with [^\u0000-\U0010ffff], which
    // also matches nothing.

    static const UChar emptySet[] = {(UChar)0x5b, (UChar)0x5d, 0};
    int32_t where = 0;
    while ((where = expandedDef.indexOf(emptySet, 2, 0)) >= 0) {
        expandedDef.replace(where, 2, UnicodeString("[^\\u0000-\\U0010ffff]"));
    }
        
    // Compile a regular expression for this rule.
    LocalPointer<RegexMatcher> reMatcher(new RegexMatcher(expandedDef, UREGEX_COMMENTS, status));
    if (U_FAILURE(status)) {
        std::cout << stdstr(expandedDef) << std::endl;
        return;
    }
}


bool BreakRules::setKeywordParameter(const UnicodeString &keyword, const UnicodeString &value, UErrorCode &status) {
    if (keyword == UnicodeString("locale")) {
        fLocale.appendInvariantChars(value, status);
        return true;
    } 
    if (keyword == UnicodeString("type")) {
        if (value == UnicodeString("grapheme")) {
            fType = UBRK_CHARACTER;
        } else if (value == UnicodeString("word")) {
            fType = UBRK_WORD;
        } else if (value == UnicodeString("line")) {
            fType = UBRK_LINE;
        } else if (value == UnicodeString("sentence")) {
            fType = UBRK_SENTENCE;
        } else {
            std::cout << "Unrecognized break type " << stdstr(value) << std::endl;
        }
        return true;
    }
    // TODO: add tailoring base setting here.
    return false;
}
    

void BreakRules::compileRules(UCHARBUF *rules, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }



    UnicodeString emptyString;
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
        fCommentsMatcher->reset(line);
        line = fCommentsMatcher->replaceFirst(emptyString, status);
        if (line.isEmpty()) {
            continue;
        }

        // Recognize character class definition and keyword lines
        fClassDefMatcher->reset(line);
        if (fClassDefMatcher->matches(status)) {
            UnicodeString className = fClassDefMatcher->group(fClassDefMatcher->pattern().groupNumberFromName("ClassName", status), status); 
            UnicodeString classDef  = fClassDefMatcher->group(fClassDefMatcher->pattern().groupNumberFromName("ClassDef", status), status); 
            std::cout << "scanned class: " << stdstr(className) << " = " << stdstr(classDef) << std::endl;
            if (setKeywordParameter(className, classDef, status)) {
                // was "type = ..." or "locale = ...", etc.
                continue;
            }
            addCharClass(className, classDef, status);
            continue;
        }

        // Recognize rule lines.
        fRuleDefMatcher->reset(line);
        if (fRuleDefMatcher->matches(status)) {
            UnicodeString ruleName = fRuleDefMatcher->group(fRuleDefMatcher->pattern().groupNumberFromName("RuleName", status), status); 
            UnicodeString ruleDef  = fRuleDefMatcher->group(fRuleDefMatcher->pattern().groupNumberFromName("RuleDef", status), status); 
            std::cout << "scanned rule: " << stdstr(ruleName) << " : " << stdstr(ruleDef) << std::endl;
            addRule(ruleName, ruleDef, status);
            continue;
        }

        std::cout << "unrecognized line: " << stdstr(line) << std::endl;

    }

}

RBBIMonkeyTest::RBBIMonkeyTest() {
}

RBBIMonkeyTest::~RBBIMonkeyTest() {
}


void RBBIMonkeyTest::testMonkey() {
    // testRules("grapheme.txt");
    // testRules("word.txt");
    testRules("line.txt");
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
 */
void RBBIMonkeyTest::testRules(const char *ruleFile) {
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



    // TODO: make running the rules be multi-threaded.

}
