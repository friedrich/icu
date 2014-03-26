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

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "unicode/utypes.h"
#include "unicode/coll.h"
#include "unicode/locid.h"
#include "unicode/sortkey.h"
#include "unicode/tblcoll.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "unicode/utf8.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

using std::string;

using icu::CollationKey;
using icu::Collator;
using icu::Locale;
using icu::RuleBasedCollator;
using icu::UnicodeString;

namespace {

// Map of CGI key-value pairs where the map keys are CGI key strings
// and the map values are pairs of start/limit pointers to the CGI values.
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
    *t = 0;  // NUL-terminate
    return t;  // Return the modified string limit.
}

const char *readChars(const KeysToLimits &data, const char *key, const char *defaultChars) {
    const auto entry = data.find(key);
    if(entry == data.end()) { return defaultChars; }
    char *s = (*entry).second.first;
    unPercentString(s);
    return s;
}

UColAttributeValue readUColAttributeValue(const KeysToLimits &data, const char *key) {
    // Simple attribute values are encoded as one ASCII character '@'+value.
    // '?' encodes -1=UCOL_DEFAULT. ('?'=0x3f, '@'=0x40, 'A'=0x41, 'B'=0x42, ...)
    return (UColAttributeValue)(*readChars(data, key, "?") - '@');
}

void readAndSetSimpleUColAttribute(Collator &coll,
                                   const KeysToLimits &data, const char *key,
                                   UColAttribute attr, UErrorCode &errorCode) {
    UColAttributeValue value = readUColAttributeValue(data, key);
    if(value != UCOL_DEFAULT) {
        coll.setAttribute(attr, value, errorCode);
    }
}

bool readBoolean(const KeysToLimits &data, const char *key) {
    return *readChars(data, key, "0") == '1';
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

struct Line {
    Line(int n, const UnicodeString &s) : nr(n), str(s) {}
    int nr;
    UnicodeString str;
};

void splitAndUnescapeLines(const UnicodeString &s, std::vector<Line> &lines) {
    // Unescape after splitting so that we do not get confused by escaped CR or LF.
    int nr = 1;
    int32_t start = 0;
    for(int32_t i = 0;; ++i) {
        if(i == s.length()) {
            if(start < i) {
                UnicodeString str = s.tempSubStringBetween(start, i).unescape();
                lines.emplace_back(nr++, str);
            }
            break;
        }
        UChar c = s[i];
        if(c == 0x0A || c == 0x0D) {  // LF or CR
            UnicodeString str = s.tempSubStringBetween(start, i).unescape();
            lines.emplace_back(nr++, str);
            if(c == 0x0D && s[i + 1] == 0x0A) { ++i; }  // CR LF is one single line break
            start = i + 1;
        }
    }
    // Delete empty trailing lines.
    while(!lines.empty() && lines.back().str.isEmpty()) {
        lines.pop_back();
    }
}

bool containsOnlyPatternWhiteSpace(const UnicodeString &s) {
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
    if(!containsOnlyPatternWhiteSpace(moreRules)) {
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

    Collator &c = *coll;
    readAndSetSimpleUColAttribute(c, data, "kk", UCOL_NORMALIZATION_MODE, errorCode);
    readAndSetSimpleUColAttribute(c, data, "ks", UCOL_STRENGTH, errorCode);
    readAndSetSimpleUColAttribute(c, data, "kn", UCOL_NUMERIC_COLLATION, errorCode);
    readAndSetSimpleUColAttribute(c, data, "kb", UCOL_FRENCH_COLLATION, errorCode);
    readAndSetSimpleUColAttribute(c, data, "kc", UCOL_CASE_LEVEL, errorCode);
    readAndSetSimpleUColAttribute(c, data, "kf", UCOL_CASE_FIRST, errorCode);
    readAndSetSimpleUColAttribute(c, data, "ka", UCOL_ALTERNATE_HANDLING, errorCode);

    UColAttributeValue value = readUColAttributeValue(data, "kv");
    if(value != UCOL_DEFAULT) {
        c.setMaxVariable((UColReorderCode)(UCOL_REORDER_CODE_FIRST + value), errorCode);
    }
    if(U_FAILURE(errorCode)) {
        printf("setting attributes failed - %s", u_errorName(errorCode));
        exit(2);
    }
    return coll.release();
}

void appendLastHexDigit(string &s8, int32_t i) {
    i &= 0xf;
    s8.push_back(i <= 9 ? (char)('0' + i) : (char)('A' + i - 10));
}

void appendEscaped(string &s8, UChar32 c) {
    if(c <= 0xffff) {
        s8.append("\\u");
    } else {
        s8.append("\\U00");
        appendLastHexDigit(s8, c >> 20);
        appendLastHexDigit(s8, c >> 16);
    }
    appendLastHexDigit(s8, c >> 12);
    appendLastHexDigit(s8, c >> 8);
    appendLastHexDigit(s8, c >> 4);
    appendLastHexDigit(s8, c);
}

void escapeForHTML(const UnicodeString &s, string &s8) {
    for(int32_t i = 0; i < s.length();) {
        UChar32 c = s.char32At(i);
        if(c == '&') {
            s8.append("&amp;");
        } else if(c == '<') {
            s8.append("&lt;");
        } else if(c == '>') {
            s8.append("&gt;");
        } else if(u_hasBinaryProperty(c, UCHAR_DEFAULT_IGNORABLE_CODE_POINT) ||
                ((U_GET_GC_MASK(c) & (U_GC_MN_MASK|U_GC_MC_MASK|U_GC_CO_MASK|U_GC_CN_MASK)) != 0) ||
                ((c != 0x20 || i == 0 || i == (s.length() - 1)) && u_isUWhiteSpace(c))) {
            appendEscaped(s8, c);
        } else {
            char c8[U8_MAX_LENGTH];
            size_t len = 0;
            U8_APPEND_UNSAFE(c8, len, c);
            s8.append(c8, len);
        }
        i += U16_LENGTH(c);
    }
}

const char *const diffStrings[] = {
    "=", "&lt;1", "&lt;2", "&lt;c", "&lt;3", "&lt;4", "&lt;i"
};

int getDiffStrength(const CollationKey &prevKey, const CollationKey &key, bool hasCaseLevel) {
    int32_t prevKeyLength;
    const uint8_t *prevBytes = prevKey.getByteArray(prevKeyLength);
    int32_t keyLength;
    const uint8_t *bytes = key.getByteArray(keyLength);
    int32_t level = 1;  // primary level
    for(int32_t i = 0;; ++i) {
        uint8_t b = prevBytes[i];
        if(b != bytes[i]) {
            return level;
        }
        if(b == 1) {  // level separator
            ++level;
            if(level == 3 && !hasCaseLevel) {
                ++level;  // skip case level which is off
            }
        } else if(b == 0) {  // sort key terminator
            return 0;  // equal
        }
    }
}

struct LessThan {
    LessThan(const Collator &c) : coll(c) {}
    bool operator()(const Line &left, const Line &right) const {
        UErrorCode errorCode = U_ZERO_ERROR;
        return coll.compare(left.str, right.str, errorCode) < 0;
    }

    const Collator &coll;
};

}  // namespace

extern "C" int
main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Read the form data from stdin.
    std::unique_ptr<char[]> content;
    KeysToLimits data;
    readFormData(content, data);
    puts("Content-type:text/html; charset=UTF-8\n");
    std::unique_ptr<Collator> coll(createCollator(data));

    UnicodeString input;
    std::vector<Line> lines;
    splitAndUnescapeLines(readUnicodeString(data, "input", input), lines);
    LessThan lessThan(*coll);
    std::sort(lines.begin(), lines.end(), lessThan);

    puts("<!DOCTYPE html><html><meta charset='utf-8'>");
    puts("<head><style type='text/css'>");
    puts(".a{}");
    puts(".b{background-color:Bisque}");
    puts("</style></head><body>");

    bool inputIsSorted = true;
    int nr = 1;
    for(const Line &line : lines) {
        if(line.nr != nr) {
            inputIsSorted = false;
            break;
        }
        ++nr;
    }
    if(inputIsSorted) {
        puts("<p><b>The input is already in sorted order.</b></p>");
    }

    bool showDiffStrengths = readBoolean(data, "ds");
    bool showLineNumbers = readBoolean(data, "nr");
    bool showSortKeys = readBoolean(data, "sk");
    bool showCEs = readBoolean(data, "ce");

    UErrorCode errorCode = U_ZERO_ERROR;
    bool hasCaseLevel = coll->getAttribute(UCOL_CASE_LEVEL, errorCode) == UCOL_ON;

    CollationKey sk0, sk1;
    CollationKey *prevSK = &sk0, *sk = &sk1;
    UnicodeString empty;
    const UnicodeString *prevStr = &empty;
    coll->getCollationKey(empty, *prevSK, errorCode);

    int i = 0;
    for(const Line &line : lines) {
        printf("<span class='%c'>", (char)('a' + (i & 1)));
        coll->getCollationKey(line.str, *sk, errorCode);
        // TODO: show when coll->compare() != prevSK->compareTo(*sk)
        if(showDiffStrengths) {
            int diffStrength = getDiffStrength(*prevSK, *sk, hasCaseLevel);
            printf("%s ", diffStrings[diffStrength]);
        }
        if(showLineNumbers) {
            printf("[%d] ", line.nr);
        }
        string s8;
        escapeForHTML(line.str, s8);
        printf("%s", s8.c_str());
        // TODO: get & show sortkeys; optionally show diff strength
        // TODO: get & show CEs
        if(showSortKeys) {
            // TODO
        }
        if(showCEs) {
            // TODO
        }
        puts("</span><br>");
        prevStr = &line.str;
        if(sk == &sk0) {
            prevSK = &sk0;
            sk = &sk1;
        } else {
            prevSK = &sk1;
            sk = &sk0;
        }
        ++i;
    }

    if(U_FAILURE(errorCode)) {
        printf("<p><b>%s</b></p>\n", u_errorName(errorCode));
    }
    puts("</body></html>");
    return 0;
}
