/********************************************************************
 * Copyright (c) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#include "unicode/utypes.h"

#include "rbbimonkeytest.h"
#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"

#include "charstr.h"
#include "cmemory.h"
#include "uelement.h"
#include "uhash.h"

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

//---------------------------------------------------------------------------------------
//
//   class BreakRule implementation.
//
//---------------------------------------------------------------------------------------

BreakRule::BreakRule()      // :  all field default initialized.
{
}

BreakRule::~BreakRule() {};


// Break rules are ordered in the order they should be applied,
// which is based on the rule name but with the first numeric field treated in numeric order.

int8_t BreakRule::comparator(UElement a, UElement b) {
    BreakRule *bra = static_cast<BreakRule *>(a.pointer);
    BreakRule *brb = static_cast<BreakRule *>(b.pointer);
    return bra->fPaddedName.compare(brb->fPaddedName);
}


//---------------------------------------------------------------------------------------
//
//   class BreakRules implementation.
//
//---------------------------------------------------------------------------------------
BreakRules::BreakRules(UErrorCode &status)  : 
        fBreakRules(status), fCharClasses(NULL), fType(UBRK_COUNT) {
    fCharClasses = uhash_open(uhash_hashUnicodeString,
                              uhash_compareUnicodeString,
                              NULL,      // value comparator.
                              &status);

    fCharClassList.adoptInstead(new UVector(status));

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
                "(?<RuleName>[A-Za-z_][A-Za-z0-9_.]*)"    // The rule name
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

    UnicodeSet *s = new UnicodeSet(expandedDef, USET_IGNORE_SPACE, NULL, status);
    if (U_FAILURE(status)) {
        return;
    }
    CharClass *cclass = new CharClass(name, definition, expandedDef, s);

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
    // If the rule name contains embedded digits, pad the first numeric field to a fixed length with leading zeroes,
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
        CharString localeName;
        localeName.appendInvariantChars(value, status);
        fLocale = Locale::createFromName(localeName.data());
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

RuleBasedBreakIterator *BreakRules::createICUBreakIterator(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    RuleBasedBreakIterator *bi = NULL;
    switch(fType) {
        case UBRK_CHARACTER:
            bi = dynamic_cast<RuleBasedBreakIterator *>(BreakIterator::createCharacterInstance(fLocale, status));
            break;
        case UBRK_WORD:
            bi = dynamic_cast<RuleBasedBreakIterator *>(BreakIterator::createWordInstance(fLocale, status));
            break;
        case UBRK_LINE:
            bi = dynamic_cast<RuleBasedBreakIterator *>(BreakIterator::createLineInstance(fLocale, status));
            break;
        case UBRK_SENTENCE:
            bi = dynamic_cast<RuleBasedBreakIterator *>(BreakIterator::createSentenceInstance(fLocale, status));
            break;
        default:
            IntlTest::gTest->errln("%s:%d Bad break iterator type of %d", __FILE__, __LINE__, fType);
            status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return bi;
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
    
    // Build the vector of char classes, omitting the dictionary class if there is one.
    // This will be used when constructing the random text to be tested.

    int32_t pos = UHASH_FIRST;
    const UHashElement *el = NULL;
    std::cout << "Building char class list ..." << std::endl;
    while ((el = uhash_nextElement(fCharClasses , &pos)) != NULL) {
        const UnicodeString *ccName = static_cast<const UnicodeString *>(el->key.pointer);
        CharClass *cclass = static_cast<CharClass *>(el->value.pointer);
        // std::cout << "    Adding " << stdstr(*ccName) << std::endl;
        if (*ccName != cclass->fName) {
            std::cout << "%s:%d: internal error, set names inconsistent.\n";
        }
        const UnicodeSet *set = cclass->fSet.getAlias();
        if (*ccName == UnicodeString("dictionary")) {
            fDictionarySet = *set;
        } else {
            fCharClassList->addElement(cclass, status);
        }
    }


}



//---------------------------------------------------------------------------------------
//
//   class MonkeyTestData implementation.
//
//---------------------------------------------------------------------------------------

MonkeyTestData::MonkeyTestData(UErrorCode &status) : fRuleForPosition(status)  {
}

void MonkeyTestData::set(BreakRules *rules, IntlTest::icu_rand &rand, UErrorCode &status) {
    const int32_t dataLength = 100;

    // Fill the test string with random characters.
    // First randomly pick a char class, then randomly pick a character from that class.
    // Exclude any characters from the dictionary set.

    std::cout << "Populating Test Data" << std::endl;
    int32_t n = 0;
    for (n=0; n<dataLength;) {
        int charClassIndex = rand() % rules->fCharClassList->size();
        const CharClass *cclass = static_cast<CharClass *>(rules->fCharClassList->elementAt(charClassIndex));
        // std::cout << "   class " << stdstr(cclass->fName) << std::endl;
        if (cclass->fSet->size() == 0) {
            // Some rules or tailorings do end up with empty char classes.
            continue;
        }
        int32_t charIndex = rand() % cclass->fSet->size();
        UChar32 c = cclass->fSet->charAt(charIndex);
        if (!rules->fDictionarySet.contains(c)) {
            fString.append(c);
            ++n;
        }
    }

    // Reset each rule matcher regex with this new string.
    //    (Although we are always using the same string object, ICU regular expressions
    //    don't like the underlying string data changing without doing a reset).

    for (int32_t ruleNum=0; ruleNum<rules->fBreakRules.size(); ruleNum++) {
        BreakRule *rule = static_cast<BreakRule *>(rules->fBreakRules.elementAt(ruleNum));
            rule->fRuleMatcher->reset(fString);
    }

    // Init the expectedBreaks, actualBreaks and ruleForPosition strings (used as arrays).
    // The actual data doesn't matter at this point, but they need to be of the same length as fString.

    fExpectedBreaks = fString;   // (assigning values, not pointers)
    fActualBreaks = fString;
    fRuleForPosition = fString;

    // Apply reference rules to find the expected breaks.
    
    n = 0;
    while (n < fString.length()) {
        BreakRule *matchingRule = NULL;
        int32_t ruleNum = 0;
        for (ruleNum=0; ruleNum<rules->fBreakRules.size(); ruleNum++) {
            BreakRule *rule = static_cast<BreakRule *>(rules->fBreakRules.elementAt(ruleNum));
            if (rule->fRuleMatcher->lookingAt(n, status)) {
               matchingRule = rule;
               break;
            }
        }
        if (matchingRule == NULL) {
            // No reference rule matched. This is an error in the rules that should never happen.
            IntlTest::gTest->errln("%s:%d Trouble with monkey test reference rules.", __FILE__, __LINE__);
            status = U_INVALID_FORMAT_ERROR;
            return;
        }
        for (int32_t strIndex = n; strIndex < matchingRule->fRuleMatcher->end(status); strIndex++) {
            fRuleForPosition.setCharAt(n, (UChar)ruleNum);
        }

    }
}


//---------------------------------------------------------------------------------------
//
//   class RBBIMonkeyImpl 
//
//---------------------------------------------------------------------------------------


// RBBIMonkeyImpl constructor does all of the setup for a single rule set - compiling the
//                            reference rules and creating the icu breakiterator to test,
//                            with its type and locale coming from the reference rules.

RBBIMonkeyImpl::RBBIMonkeyImpl(const char *ruleFile, UErrorCode &status) {
    openBreakRules(ruleFile, status);
    if (U_FAILURE(status)) {
        IntlTest::gTest->errln("%s:%d Error %s opening file %s.", __FILE__, __LINE__, u_errorName(status), ruleFile);
        return;
    }
    fRuleSet.adoptInstead(new BreakRules(status));
    fRuleSet->compileRules(fRuleCharBuffer.getAlias(), status);
    if (U_FAILURE(status)) {
        IntlTest::gTest->errln("%s:%d Error %s processing file %s.", __FILE__, __LINE__, u_errorName(status), ruleFile);
        return;
    }
    fBI.adoptInstead(fRuleSet->createICUBreakIterator(status));
    fTestData.adoptInstead(new MonkeyTestData(status));
}


RBBIMonkeyImpl::~RBBIMonkeyImpl() {
}


void RBBIMonkeyImpl::openBreakRules(const char *fileName, UErrorCode &status) {
    CharString path;
    path.append(IntlTest::getSourceTestData(status), status);
    path.append("break_rules" U_FILE_SEP_STRING, status);
    path.appendPathPart(fileName, status);
    const char *codePage = "UTF-8";
    fRuleCharBuffer.adoptInstead(ucbuf_open(path.data(), &codePage, TRUE, FALSE, &status));
}


void RBBIMonkeyImpl::runTest(int32_t numIterations, UErrorCode &status) {
    if (U_FAILURE(status)) { return; };

    for (int loopCount = 0; loopCount < numIterations; loopCount++) {
        fTestData->set(fRuleSet.getAlias(), fRandomGenerator, status);
        if (U_FAILURE(status)) { break; };
        // check forwards
        // check reverse
        // check following / preceding
        // check is boundary
    }
}


//---------------------------------------------------------------------------------------
//
//   class RBBIMonkeyTest implementation.
//
//---------------------------------------------------------------------------------------
RBBIMonkeyTest::RBBIMonkeyTest() {
}

RBBIMonkeyTest::~RBBIMonkeyTest() {
}


void RBBIMonkeyTest::testMonkey() {
    // TODO: need parameters from test framework for specific test, exhaustive, iteration count, starting seed, asynch, ...
    const char *tests[] = {"grapheme.txt", "word.txt", "line.txt"};

    for (int i=0; i < UPRV_LENGTHOF(tests); ++i) {
        UErrorCode status = U_ZERO_ERROR;
        RBBIMonkeyImpl test(tests[i], status);
        test.runTest(1, status);
    }
}

