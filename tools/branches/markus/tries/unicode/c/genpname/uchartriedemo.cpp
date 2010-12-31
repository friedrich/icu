/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  uchartriedemo.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010nov15
*   created by: Markus W. Scherer
*/

#include <stdio.h>
#include <string>

#include "unicode/utypes.h"
#include "unicode/unistr.h"

#include "denseranges.h"
#include "toolutil.h"
#include "uchartrie.h"
#include "uchartriebuilder.h"
#include "uchartrieiterator.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void
printUChars(const char *name, const UnicodeString &uchars) {
    printf("%18s  [%3d]", name, (int)uchars.length());
    for(int32_t i=0; i<uchars.length(); ++i) {
        printf(" %04x", uchars[i]);
    }
    puts("");
}

static void
printTrie(const UnicodeString &uchars) {
    IcuToolErrorCode errorCode("printTrie");
    UCharTrieIterator iter(uchars.getBuffer(), 0, errorCode);
    std::string utf8;
    while(iter.next(errorCode)) {
        utf8.clear();
        iter.getString().toUTF8String(utf8);
        printf("  '%s': %d\n", utf8.c_str(), (int)iter.getValue());
    }
}

extern int main(int argc, char* argv[]) {
    IcuToolErrorCode errorCode("bytetriedemo");
    UCharTrieBuilder builder;
    UnicodeString str;
    builder.add(UnicodeString(), 0, errorCode).build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("empty string", str);
    UCharTrie empty(str.getBuffer());
    UDictTrieResult result=empty.current();
    printf("empty.current() %d %d\n", result, (int)empty.getValue());
    printTrie(str);

    builder.clear().add("a", 1, errorCode).build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("a", str);
    UCharTrie a(str.getBuffer());
    result=a.next('a');
    printf("a.next(a) %d %d\n", result, (int)a.getValue());
    printTrie(str);

    builder.clear().add("ab", -1, errorCode).build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("ab", str);
    UCharTrie ab(str.getBuffer());
    ab.next('a');
    result=ab.next('b');
    printf("ab.next(ab) %d %d\n", result, (int)ab.getValue());
    printTrie(str);

    builder.clear().add("a", 1, errorCode).add("ab", 100, errorCode).build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("a+ab", str);
    UCharTrie a_ab(str.getBuffer());
    result=a_ab.next('a');
    printf("a_ab.next(a) %d %d\n", result, (int)a_ab.getValue());
    result=a_ab.next('b');
    printf("a_ab.next(b) %d %d\n", result, (int)a_ab.getValue());
    result=a_ab.current();
    printf("a_ab.current() %d %d\n", result, (int)a_ab.getValue());
    printTrie(str);

    builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode).build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("a+b+c", str);
    UCharTrie a_b_c(str.getBuffer());
    result=a_b_c.next('a');
    printf("a_b_c.next(a) %d %d\n", result, (int)a_b_c.getValue());
    result=a_b_c.next('b');
    printf("a_b_c.next(b) %d\n", result);
    result=a_b_c.reset().next('b');
    printf("a_b_c.r.next(b) %d %d\n", result, (int)a_b_c.getValue());
    result=a_b_c.reset().next('c');
    printf("a_b_c.r.next(c) %d %d\n", result, (int)a_b_c.getValue());
    result=a_b_c.reset().next('d');
    printf("a_b_c.r.next(d) %d\n", result);
    printTrie(str);

    builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode);
    builder.add("d", 10, errorCode).add("e", 20, errorCode).add("f", 30, errorCode);
    builder.add("g", 100, errorCode).add("h", 200, errorCode).add("i", 300, errorCode);
    builder.add("j", 1000, errorCode).add("k", 2000, errorCode).add("l", 3000, errorCode);
    builder.add("m", 10000, errorCode).add("n", 100000, errorCode).add("o", 1000000, errorCode);
    builder.build(UDICTTRIE_BUILD_FAST, str, errorCode);
    printUChars("a-o", str);
    UCharTrie a_o(str.getBuffer());
    for(char c='`'; c<='p'; ++c) {
        result=a_o.reset().next(c);
        if(UDICTTRIE_RESULT_HAS_VALUE(result)) {
            printf("a_o.r.next(%c) %d %d\n", c, result, (int)a_o.getValue());
        } else {
            printf("a_o.r.next(%c) %d\n", c, result);
        }
    }
    printTrie(str);

    return 0;
}
