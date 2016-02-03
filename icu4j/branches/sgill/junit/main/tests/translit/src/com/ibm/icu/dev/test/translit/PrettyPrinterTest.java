/*
 **********************************************************************
 * Copyright (c) 2009-2012, Google, International Business Machines
 * Corporation and others.  All Rights Reserved.
 **********************************************************************
 */
package com.ibm.icu.dev.test.translit;

import java.text.Collator;
import java.util.Locale;

import org.junit.Test;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.dev.util.PrettyPrinter;
import com.ibm.icu.text.UnicodeSet;

public class PrettyPrinterTest extends TestFmwk {
    public static final UnicodeSet TO_QUOTE = new UnicodeSet("[[:z:][:me:][:mn:][:di:][:c:]-[\u0020]]");

    @Test
    public void TestBasicUnicodeSet() {

        Collator spaceComp = Collator.getInstance(Locale.ENGLISH);
        spaceComp.setStrength(Collator.PRIMARY);
        
        final PrettyPrinter PRETTY_PRINTER = new PrettyPrinter()
        .setOrdering(Collator.getInstance(Locale.ENGLISH))
        .setSpaceComparator(spaceComp)
        .setToQuote(TO_QUOTE);

        UnicodeSet expected = new UnicodeSet("[:L:]");
        String formatted = PRETTY_PRINTER.format(expected);
        logln(formatted);
        UnicodeSet actual = new UnicodeSet(formatted);
        assertEquals("PrettyPrinter preserves meaning", expected, actual);
    }
}
