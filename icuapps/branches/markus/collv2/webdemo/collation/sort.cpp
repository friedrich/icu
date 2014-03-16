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
#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

using std::string;

using icu::Collator;
using icu::Locale;
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

UnicodeString &readUnicodeString(char *s, UnicodeString &dest) {
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
    for(const auto &entry : data) {
        const string &key = entry.first.c_str();
        const auto &range = entry.second;
        char *value = range.first;
        // char *limit = range.second;
        UnicodeString s16;
        string s8;
        if(key == "rules" || key == "input") {
            readUnicodeString(value, s16).unescape().toUTF8String(s8);
            value = (char *)s8.c_str();
        } else {
            unPercentString(value);
        }
        printf("- %s: %s\n", key.c_str(), value);
    }
    return 0;
}
