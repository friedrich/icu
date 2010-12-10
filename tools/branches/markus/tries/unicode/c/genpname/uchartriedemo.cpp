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
    UnicodeString str=builder.add(UnicodeString(), 0, errorCode).build(errorCode);
    printUChars("empty string", str);
    UCharTrie empty(str.getBuffer());
    UBool hasValue=empty.hasValue();
    printf("empty.next() %d %d\n", hasValue, (int)empty.getValue());
    printTrie(str);

    str=builder.clear().add("a", 1, errorCode).build(errorCode);
    printUChars("a", str);
    UCharTrie a(str.getBuffer());
    hasValue=a.next('a') && a.hasValue();
    printf("a.next(a) %d %d\n", hasValue, (int)a.getValue());
    printTrie(str);

    str=builder.clear().add("ab", -1, errorCode).build(errorCode);
    printUChars("ab", str);
    UCharTrie ab(str.getBuffer());
    hasValue=ab.next('a') && ab.next('b') && ab.hasValue();
    printf("ab.next(ab) %d %d\n", hasValue, (int)ab.getValue());
    printTrie(str);

    str=builder.clear().add("a", 1, errorCode).add("ab", 100, errorCode).build(errorCode);
    printUChars("a+ab", str);
    UCharTrie a_ab(str.getBuffer());
    hasValue=a_ab.next('a') && a_ab.hasValue();
    printf("a_ab.next(a) %d %d\n", hasValue, (int)a_ab.getValue());
    hasValue=a_ab.next('b') && a_ab.hasValue();
    printf("a_ab.next(b) %d %d\n", hasValue, (int)a_ab.getValue());
    hasValue=a_ab.hasValue();
    printf("a_ab.next() %d %d\n", hasValue, (int)a_ab.getValue());
    printTrie(str);

    str=builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode).build(errorCode);
    printUChars("a+b+c", str);
    UCharTrie a_b_c(str.getBuffer());
    hasValue=a_b_c.next('a') && a_b_c.hasValue();
    printf("a_b_c.next(a) %d %d\n", hasValue, (int)a_b_c.getValue());
    hasValue=a_b_c.next('b') && a_b_c.hasValue();
    printf("a_b_c.next(b) %d %d\n", hasValue, (int)a_b_c.getValue());
    hasValue=a_b_c.reset().next('b') && a_b_c.hasValue();
    printf("a_b_c.r.next(b) %d %d\n", hasValue, (int)a_b_c.getValue());
    hasValue=a_b_c.reset().next('c') && a_b_c.hasValue();
    printf("a_b_c.r.next(c) %d %d\n", hasValue, (int)a_b_c.getValue());
    hasValue=a_b_c.reset().next('d') && a_b_c.hasValue();
    printf("a_b_c.r.next(d) %d %d\n", hasValue, (int)a_b_c.getValue());
    printTrie(str);

    builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode);
    builder.add("d", 10, errorCode).add("e", 20, errorCode).add("f", 30, errorCode);
    builder.add("g", 100, errorCode).add("h", 200, errorCode).add("i", 300, errorCode);
    builder.add("j", 1000, errorCode).add("k", 2000, errorCode).add("l", 3000, errorCode);
    builder.add("m", 10000, errorCode).add("n", 100000, errorCode).add("o", 1000000, errorCode);
    str=builder.build(errorCode);
    printUChars("a-o", str);
    UCharTrie a_o(str.getBuffer());
    for(char c='`'; c<='p'; ++c) {
        hasValue=a_o.reset().next(c) && a_o.hasValue();
        printf("a_o.r.next(%c) %d %d\n", c, hasValue, (int)a_o.getValue());
    }
    printTrie(str);

    return 0;
}
