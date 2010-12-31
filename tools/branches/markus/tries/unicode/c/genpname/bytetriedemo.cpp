/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  bytetriedemo.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010nov05
*   created by: Markus W. Scherer
*/

#include <stdio.h>

#include "unicode/utypes.h"
#include "unicode/stringpiece.h"

#include "bytetrie.h"
#include "bytetriebuilder.h"
#include "bytetrieiterator.h"
#include "denseranges.h"
#include "toolutil.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void
printBytes(const char *name, const StringPiece &bytes) {
    printf("%18s  [%3d]", name, (int)bytes.length());
    for(int32_t i=0; i<bytes.length(); ++i) {
        printf(" %02x", bytes.data()[i]&0xff);  // TODO: Add StringPiece::operator[] const
    }
    puts("");
}

static void
printTrie(const StringPiece &bytes) {
    IcuToolErrorCode errorCode("printTrie");
    ByteTrieIterator iter(bytes.data(), 0, errorCode);
    while(iter.next(errorCode)) {
        printf("  '%s': %d\n", iter.getString().data(), (int)iter.getValue());
    }
}

static void printRanges(const int32_t ranges[][2], int32_t length) {
    printf("ranges[%d]", (int)length);
    for(int32_t i=0; i<length; ++i) {
        printf(" [%ld..%ld]", (long)ranges[i][0], (long)ranges[i][1]);
    }
    puts("");
}

extern int main(int argc, char* argv[]) {
    IcuToolErrorCode errorCode("bytetriedemo");
    ByteTrieBuilder builder;
    StringPiece sp=builder.add("", 0, errorCode).build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("empty string", sp);
    ByteTrie empty(sp.data());
    UDictTrieResult result=empty.current();
    printf("empty.current() %d %d\n", result, (int)empty.getValue());
    printTrie(sp);

    sp=builder.clear().add("a", 1, errorCode).build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("a", sp);
    ByteTrie a(sp.data());
    result=a.next('a');
    printf("a.next(a) %d %d\n", result, (int)a.getValue());
    printTrie(sp);

    sp=builder.clear().add("ab", -1, errorCode).build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("ab", sp);
    ByteTrie ab(sp.data());
    ab.next('a');
    result=ab.next('b');
    printf("ab.next(ab) %d %d\n", result, (int)ab.getValue());
    printTrie(sp);

    sp=builder.clear().add("a", 1, errorCode).add("ab", 100, errorCode).build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("a+ab", sp);
    ByteTrie a_ab(sp.data());
    result=a_ab.next('a');
    printf("a_ab.next(a) %d %d\n", result, (int)a_ab.getValue());
    result=a_ab.next('b');
    printf("a_ab.next(b) %d %d\n", result, (int)a_ab.getValue());
    result=a_ab.current();
    printf("a_ab.current() %d %d\n", result, (int)a_ab.getValue());
    printTrie(sp);

    sp=builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode).build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("a+b+c", sp);
    ByteTrie a_b_c(sp.data());
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
    printTrie(sp);

    builder.clear().add("a", 1, errorCode).add("b", 2, errorCode).add("c", 3, errorCode);
    builder.add("d", 10, errorCode).add("e", 20, errorCode).add("f", 30, errorCode);
    builder.add("g", 100, errorCode).add("h", 200, errorCode).add("i", 300, errorCode);
    builder.add("j", 1000, errorCode).add("k", 10000, errorCode).add("l", 100000, errorCode);
    sp=builder.build(UDICTTRIE_BUILD_FAST, errorCode);
    printBytes("a-l", sp);
    ByteTrie a_l(sp.data());
    for(char c='`'; c<='m'; ++c) {
        result=a_l.reset().next(c);
        if(UDICTTRIE_RESULT_HAS_VALUE(result)) {
            printf("a_l.r.next(%c) %d %d\n", c, result, (int)a_l.getValue());
        } else {
            printf("a_l.r.next(%c) %d\n", c, result);
        }
    }
    printTrie(sp);

    static const int32_t values[]={
        -1, 0, 1, 2,
        4, 5, 6, 7,
        12, 13, 14,
        24, 25, 26
    };
    int32_t ranges[3][2];
    int32_t length;
    length=uprv_makeDenseRanges(values, LENGTHOF(values), 1, ranges, LENGTHOF(ranges));
    printRanges(ranges, length);
    length=uprv_makeDenseRanges(values, LENGTHOF(values), 0xc0, ranges, LENGTHOF(ranges));
    printRanges(ranges, length);
    length=uprv_makeDenseRanges(values, LENGTHOF(values), 0xf0, ranges, LENGTHOF(ranges));
    printRanges(ranges, length);
    length=uprv_makeDenseRanges(values, LENGTHOF(values), 0x100, ranges, LENGTHOF(ranges));
    printRanges(ranges, length);

    return 0;
}
