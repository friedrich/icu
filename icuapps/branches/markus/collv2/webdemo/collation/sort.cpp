/*
*******************************************************************************
* Copyright (C) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* icuapps/webdemo/collation/sort.cpp
*
* created on: 2014mar15
* created by: Markus W. Scherer
*
* Simple ICU collation/sorting CGI web demo.
* Requires UTF-8 as the source code and execution charset.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <memory>
#include <string>

#include "unicode/utypes.h"
#include "unicode/coll.h"
#include "unicode/locid.h"
#include "unicode/tblcoll.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

using std::string;

using icu::Collator;
using icu::Locale;
using icu::RuleBasedCollator;
using icu::UnicodeString;

namespace {

typedef std::map<string, std::pair<char *, char *>> KeysToLimits;

void readFormData(std::unique_ptr<char[]> &content, KeysToLimits &data) {
    // Read the content bytes from stdin.
    const char *lengthString = getenv("CONTENT_LENGTH");
    if(lengthString == NULL) {
        fprintf(stderr, "missing CONTENT_LENGTH\n");
        exit(1);
    }
    unsigned long contentLength = strtoul(lengthString, NULL, 10);
    content.reset(new char[contentLength + 1]);
    size_t numRead = fread(content.get(), 1, contentLength, stdin);
    if(numRead != contentLength) {
        fprintf(stderr, "fewer than CONTENT_LENGTH bytes read\n");
        exit(1);
    }
    content[contentLength] = 0;  // NUL-terminate

    // Parse the bytes into key-value pairs and put them into the data map.
    // Copy the keys but store (start, limit) pointer pairs for the values.
    for(char *start = content.get(); *start != 0;) {
        char *p;  // Find the end of the key.
        char c;
        for(p = start; (c = *p) != 0 && c != '=' && c != '&'; ++p) {}
        char *q;  // Find the end of the value.
        for(q = p; (c = *q) != 0 && c != '&'; ++q) {}
        char *v = p;  // Value start after the '=' separator if there was one.
        if(p != q) { ++v; }
        data[string(start, p)] = std::make_pair(v, q);
        if(c == 0) { break; }
        *q++ = 0;  // Overwrite the value terminator '&' with a NUL.
        start = q;
    }
}

int getHexValue(int c) {
    int h = c - '0';
    if(h <= 9) { return h; }
    h = c - 'A';
    if(0 <= h && h <= 5) { return h + 10; }
    h = c - 'a';
    if(0 <= h && h <= 5) { return h + 10; }
    return -1;
}

char *unPercentString(char *s) {
    char *t;  // Modify the string in-place.
    char c;
    for(t = s; (c = *s++) != 0;) {
        if(c == '+') {
            c = ' ';
        } else if(c == '%') {
            int h1, h2;
            if((h1 = getHexValue(s[0])) >= 0 && (h2 = getHexValue(s[1])) >= 0) {
                s += 2;
                c = (char)((h1 << 4) | h2);
            }
        }
        *t++ = c;
    }
    *t = 0;
    return t;  // Return the modified string limit.
}

const char *readChars(const KeysToLimits &data, const char *key, const char *defaultChars) {
    const auto entry = data.find(key);
    if(entry == data.end()) { return defaultChars; }
    char *s = (*entry).second.first;
    unPercentString(s);
    return s;
}

UnicodeString &readUnicodeString(const KeysToLimits &data, const char *key, UnicodeString &dest) {
    const auto entry = data.find(key);
    if(entry == data.end()) { return dest; }
    char *s = (*entry).second.first;
    char *limit = unPercentString(s);
    int32_t u8Length = (int32_t)(limit - s);
    UChar *buffer = dest.getBuffer(u8Length);
    int32_t u16Length;
    UErrorCode errorCode = U_ZERO_ERROR;
    u_strFromUTF8(buffer, dest.getCapacity(), &u16Length, s, u8Length, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "invalid UTF-8 string value - %s: \"%s\"\n", u_errorName(errorCode), s);
        exit(1);
    }
    dest.releaseBuffer(u16Length);
    return dest;
}

bool onlyPatternWhiteSpace(const UnicodeString &s) {
    for(int32_t i = 0; i < s.length(); ++i) {
        if(!u_hasBinaryProperty(s[i], UCHAR_PATTERN_WHITE_SPACE)) { return false; }
    }
    return true;
}

Collator *createCollator(const KeysToLimits &data) {
    const char *localeString = readChars(data, "co", "");
    UErrorCode errorCode = U_ZERO_ERROR;
    Locale loc(localeString);
    std::unique_ptr<Collator> coll(Collator::createInstance(loc, errorCode));
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "Collator::createInstance(%s) failed - %s\n",
                localeString, u_errorName(errorCode));
        exit(1);
    }

    UnicodeString moreRules;
    readUnicodeString(data, "rules", moreRules);
    if(!onlyPatternWhiteSpace(moreRules)) {
        const RuleBasedCollator *rbc = dynamic_cast<const RuleBasedCollator *>(coll.get());
        if(rbc == NULL) {
            puts("unable to append rules to a Collator that is not a RuleBasedCollator");
            exit(2);
        }
        const UnicodeString &collRules = rbc->getRules();
        UnicodeString rules = collRules + moreRules;
        UParseError parseError;
        UnicodeString reason;
        std::unique_ptr<Collator> coll2(
            new RuleBasedCollator(rules, parseError, reason, errorCode));
        if(U_FAILURE(errorCode)) {
            printf("RuleBasedCollator(rules) failed - %s\n", u_errorName(errorCode));
            string s8;
            printf("  reason: %s\n", reason.toUTF8String(s8).c_str());
            if(parseError.offset >= 0) {
                printf("  offset in appended rules: %d\n",
                       (int)(parseError.offset - collRules.length()));
            }
            if(parseError.preContext[0] != 0 || parseError.postContext[0] != 0) {
                string pre, post;
                printf("  snippet: ...%s(!)%s...\n",
                       UnicodeString(parseError.preContext).toUTF8String(pre).c_str(),
                       UnicodeString(parseError.postContext).toUTF8String(post).c_str());
            }
            exit(2);
        }
        coll.reset(coll2.release());
    }

    UColAttributeValue value = (UColAttributeValue)(*readChars(data, "kk", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_NORMALIZATION_MODE, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "ks", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_STRENGTH, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "kn", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_NUMERIC_COLLATION, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "kb", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_FRENCH_COLLATION, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "kc", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_CASE_LEVEL, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "kf", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_CASE_FIRST, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "ka", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setAttribute(UCOL_ALTERNATE_HANDLING, value, errorCode);
    }
    value = (UColAttributeValue)(*readChars(data, "kv", "?") - '@');
    if(value != UCOL_DEFAULT) {
        coll->setMaxVariable((UColReorderCode)(UCOL_REORDER_CODE_FIRST + value), errorCode);
    }
    if(U_FAILURE(errorCode)) {
        printf("setting attributes failed - %s", u_errorName(errorCode));
        exit(2);
    }
    return coll.release();
}

}  // namespace

extern "C" int
main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Read the form data from stdin.
    std::unique_ptr<char[]> content;
    KeysToLimits data;
    readFormData(content, data);
    puts("Content-type:text/plain; charset=UTF-8\n");
    std::unique_ptr<Collator> coll(createCollator(data));
    puts("ok");
    return 0;
}
