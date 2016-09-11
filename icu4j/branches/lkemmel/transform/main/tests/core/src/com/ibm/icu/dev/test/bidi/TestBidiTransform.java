// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
*******************************************************************************
*   Copyright (C) 2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*/

package com.ibm.icu.dev.test.bidi;

import org.junit.Test;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.ArabicShaping;
import com.ibm.icu.text.Bidi;
import com.ibm.icu.text.BidiTransform;
import com.ibm.icu.text.BidiTransform.Mirroring;
import com.ibm.icu.text.BidiTransform.Order;

/**
 * Verify Bidi Layout Transformations
 *
 * @author Lina Kemmel
 *
 */
public class TestBidiTransform extends TestFmwk {

    static final char LATN_ZERO         = '\u0030';
    static final char ARAB_ZERO         = '\u0660';
    static final char MIN_HEB_LETTER    = '\u05d0';
    static final char MIN_ARAB_LETTER   = '\u0630'; // relevant to this test only
    static final char MIN_SHAPED_LETTER = '\ufeab'; // relevant to this test only


    private BidiTransform bidiTransform;
    private Bidi bidi;

    public TestBidiTransform() {}

    @Test
    public void testBidiTransform() {
        logln("\nEntering TestBidiTransform\n");

        bidi = new Bidi();
        bidiTransform = new BidiTransform();

        autoDirectionTest();
        allTransformOptionsTest();

        logln("\nExiting TestBidiTransform\n");
    }

    /**
     * Tests various combinations of base directions, with the input either
     * <code>Direction.DEFAULT_LTR</code> or <code>Direction.DEFAULT_RTL</code>,
     * and the output either <code>Direction.LTR</code> or
     * <code>Direction.RTL</code>. Order is always <code>Order.LOGICAL</code>
     * for the input and <code>Order.VISUAL</code> for the output.
     */
    private void autoDirectionTest() {
        final String[] inTexts = {
            "abc \u05d0\u05d1",
            "... abc \u05d0\u05d1",
            "\u05d0\u05d1 abc",
            "... \u05d0\u05d1 abc",
            ".*^"
        };
        final byte[] inLevels = {
                Bidi.LEVEL_DEFAULT_LTR, Bidi.LEVEL_DEFAULT_RTL
        };
        final byte[] outLevels = {
            Bidi.LTR, Bidi.RTL
        };
        logln("\nEntering autoDirectionTest\n");

        for (String inText : inTexts) {
            for (byte inLevel : inLevels) {
                for (byte outLevel : outLevels) {
                    String outText = bidiTransform.transform(inText, inLevel, Order.LOGICAL,
                            outLevel, Order.VISUAL, Mirroring.OFF, 0);
                    bidi.setPara(inText, inLevel, null);
                    String expectedText = bidi.writeReordered(Bidi.REORDER_DEFAULT);
                    if ((outLevel & 1) != 0) {
                        expectedText = Bidi.writeReverse(expectedText, Bidi.OUTPUT_REVERSE);
                    }
                    logResultsForDir(inText, outText, expectedText, inLevel, outLevel);
                }
            }
        }
        logln("\nExiting autoDirectionTest\n");
    }

    /**
     * This method covers:
     * <ul>
     * <li>all possible combinations of ordering schemes and <strong>explicit</strong>
     * base directions, applied to both input and output,</li>
     * <li>selected tests for auto direction (systematically, auto direction is
     * covered in a dedicated test) applied on both input and output,</li>
     * <li>all possible combinations of mirroring, numerals and literals applied
     * to output only.</li>
     * </ul>
     */
    private void allTransformOptionsTest() {
        final String inText = "a[b]c \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662";

        final Object[][] testCases = {
            { Bidi.LTR, Order.LOGICAL, Bidi.LTR, Order.LOGICAL,
                    inText, // reordering no mirroring
                    "a[b]c \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662", // mirroring
                    "a[b]c \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u0662\u0663\u0660 e\u0631\u0664\u0665\u0666 f \u0632 \u0661\u0662", // context numeric shaping
                    "1: Logical LTR ==> Logical LTR" },
            { Bidi.LTR, Order.LOGICAL, Bidi.LTR, Order.VISUAL,
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d \u0662\u0663\u0660 \u0630 e\u0664\u0665\u0666\u0631 f \u0661\u0662 \u0632",
                    "2: Logical LTR ==> Visual LTR" },
            { Bidi.LTR, Order.LOGICAL, Bidi.RTL, Order.LOGICAL,
                    "\u0632 \u0661\u0662 f \u0631e456 \u0630 23\u0660 d \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 a[b]c",
                    "\u0632 \u0661\u0662 f \u0631e456 \u0630 23\u0660 d \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 a[b]c",
                    "\u0632 \u0661\u0662 f \u0631e\u0664\u0665\u0666 \u0630 \u0662\u0663\u0660 d \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 a[b]c",
                    "3: Logical LTR ==> Logical RTL" },
            { Bidi.LTR, Order.LOGICAL, Bidi.RTL, Order.VISUAL,
                    "\u0632 \u0662\u0661 f \u0631654e \u0630 \u066032 d \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 c]b[a",
                    "\u0632 \u0662\u0661 f \u0631654e \u0630 \u066032 d \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 c]b[a",
                    "\u0632 \u0662\u0661 f \u0631\u0666\u0665\u0664e \u0630 \u0660\u0663\u0662 d \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 c]b[a",
                    "4: Logical LTR ==> Visual RTL" },

            { Bidi.RTL, Order.LOGICAL, Bidi.RTL, Order.LOGICAL, inText,
                    "a[b]c \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662", // mirroring
                    "a[b]c \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662",
                    "5: Logical RTL ==> Logical RTL" },
            { Bidi.RTL, Order.LOGICAL, Bidi.RTL, Order.VISUAL,
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "6: Logical RTL ==> Visual RTL" },
            { Bidi.RTL, Order.LOGICAL, Bidi.LTR, Order.LOGICAL,
                    "\u0632 \u0661\u0662 f 456\u0631e 23\u0630 \u0660 d 1 \u05d0(\u05d1\u05d2 \u05d3)\u05d4 a[b]c",
                    "\u0632 \u0661\u0662 f 456\u0631e 23\u0630 \u0660 d 1 \u05d0)\u05d1\u05d2 \u05d3(\u05d4 a[b]c",
                    "\u0632 \u0661\u0662 f 456\u0631e 23\u0630 \u0660 d 1 \u05d0(\u05d1\u05d2 \u05d3)\u05d4 a[b]c",
                    "7: Logical RTL ==> Logical LTR" },
            { Bidi.RTL, Order.LOGICAL, Bidi.LTR, Order.VISUAL,
                    "\u0661\u0662 \u0632 f 456\u0631e 23\u0660 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 a[b]c",
                    "\u0661\u0662 \u0632 f 456\u0631e 23\u0660 \u0630 d 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 a[b]c",
                    "\u0661\u0662 \u0632 f 456\u0631e 23\u0660 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 a[b]c",
                    "8: Logical RTL ==> Visual LTR" },

            { Bidi.LTR, Order.VISUAL, Bidi.LTR, Order.VISUAL, inText,
                    "a[b]c \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662", // mirroring
                    "a[b]c \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u0662\u0663\u0660 e\u0631\u0664\u0665\u0666 f \u0632 \u0661\u0662",
                    "9: Visual LTR ==> Visual LTR" },
            { Bidi.LTR, Order.VISUAL, Bidi.LTR, Order.LOGICAL,
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "10: Visual LTR ==> Logical LTR" },
            { Bidi.LTR, Order.VISUAL, Bidi.RTL, Order.VISUAL,
                    "\u0662\u0661 \u0632 f 654\u0631e \u066032 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 c]b[a",
                    "\u0662\u0661 \u0632 f 654\u0631e \u066032 \u0630 d 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 c]b[a",
                    "\u0662\u0661 \u0632 f \u0666\u0665\u0664\u0631e \u0660\u0663\u0662 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 c]b[a",
                    "11: Visual LTR ==> Visual RTL" },
            { Bidi.LTR, Order.VISUAL, Bidi.RTL, Order.LOGICAL,
                    "\u0661\u0662 \u0632 f 456\u0631e 23\u0660 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 a[b]c",
                    "\u0661\u0662 \u0632 f 456\u0631e 23\u0660 \u0630 d 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 a[b]c",
                    "\u0661\u0662 \u0632 f \u0664\u0665\u0666\u0631e \u0662\u0663\u0660 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 a[b]c",
                    "12: Visual LTR ==> Logical RTL" },

            { Bidi.RTL, Order.VISUAL, Bidi.RTL, Order.VISUAL, inText,
                    "a[b]c \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662",
                    "a[b]c \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 23\u0660 e\u0631456 f \u0632 \u0661\u0662",
                    "13: Visual RTL ==> Visual RTL" },
            { Bidi.RTL, Order.VISUAL, Bidi.RTL, Order.LOGICAL,
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "14: Visual RTL ==> Logical RTL" },
            { Bidi.RTL, Order.VISUAL, Bidi.LTR, Order.VISUAL,
                    "\u0662\u0661 \u0632 f 654\u0631e \u066032 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 c]b[a",
                    "\u0662\u0661 \u0632 f 654\u0631e \u066032 \u0630 d 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 c]b[a",
                    "\u0662\u0661 \u0632 f 654\u0631e \u066032 \u0630 d 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 c]b[a",
                    "15: Visual RTL ==> Visual LTR" },
            { Bidi.RTL, Order.VISUAL, Bidi.LTR, Order.LOGICAL,
                    "\u0632 \u0662\u0661 f 654\u0631e \u066032 \u0630 d 1 \u05d0(\u05d1\u05d2 \u05d3)\u05d4 c]b[a",
                    "\u0632 \u0662\u0661 f 654\u0631e \u066032 \u0630 d 1 \u05d0)\u05d1\u05d2 \u05d3(\u05d4 c]b[a",
                    "\u0632 \u0662\u0661 f 654\u0631e \u066032 \u0630 d 1 \u05d0(\u05d1\u05d2 \u05d3)\u05d4 c]b[a",
                    "16: Visual RTL ==> Logical LTR" },

            { Bidi.LEVEL_DEFAULT_RTL, Order.LOGICAL, Bidi.LTR, Order.VISUAL,
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d \u0662\u0663\u0660 \u0630 e\u0664\u0665\u0666\u0631 f \u0661\u0662 \u0632",
                    "17: Logical DEFAULT_RTL ==> Visual LTR" },
            /*
            { Bidi.RTL, Order.LOGICAL, Bidi.LEVEL_DEFAULT_LTR, Order.VISUAL,
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "18: Logical RTL ==> Visual DEFAULT_LTR" },
            */
            { Bidi.LEVEL_DEFAULT_LTR, Order.LOGICAL, Bidi.LTR, Order.VISUAL,
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4(\u05d3 \u05d2\u05d1)\u05d0 d 23\u0660 \u0630 e456\u0631 f \u0661\u0662 \u0632",
                    "a[b]c 1 \u05d4)\u05d3 \u05d2\u05d1(\u05d0 d \u0662\u0663\u0660 \u0630 e\u0664\u0665\u0666\u0631 f \u0661\u0662 \u0632",
                    "19: Logical DEFAULT_LTR ==> Visual LTR" },
            { Bidi.RTL, Order.LOGICAL, Bidi.LEVEL_DEFAULT_RTL, Order.VISUAL,
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0)\u05d1\u05d2 \u05d3(\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "c]b[a \u05d0(\u05d1\u05d2 \u05d3)\u05d4 1 d \u0630 \u066032 e\u0631654 f \u0632 \u0662\u0661",
                    "20: Logical RTL ==> Visual DEFAULT_RTL" },
        };

        final int[] digits = {
                ArabicShaping.DIGITS_NOOP, ArabicShaping.DIGITS_EN2AN, ArabicShaping.DIGITS_AN2EN, ArabicShaping.DIGITS_EN2AN_INIT_AL
        };
        final int[] letters = {
                ArabicShaping.LETTERS_NOOP, ArabicShaping.LETTERS_SHAPE, ArabicShaping.LETTERS_UNSHAPE
        };

        logln("\nEntering allTransformOptionsTest\n");

        // Test various combinations of Direction, Order, Mirroring, Numerals and Literals
        for (Object[] test : testCases) {
            verifyResultsForAllOpts(test, inText,
                    bidiTransform.transform(inText, (Byte)test[0], (Order)test[1], (Byte)test[2],
                            (Order)test[3], Mirroring.ON, 0),
                    5, 0, 0);

            for (int digit : digits) {
                for (int letter : letters) {
                    verifyResultsForAllOpts(test, inText,
                            bidiTransform.transform(inText, (Byte)test[0], (Order)test[1], (Byte)test[2],
                                    (Order)test[3], Mirroring.OFF, digit | letter),
                            4, digit, letter);
                }
            }
        }
        logln("\nExiting allTransformOptionsTest\n");
    }

    private void logResultsForDir(String inText, String outText, String expected,
            byte inLevel, byte outLevel) {

        //assertEquals("Unexpected output for BTD", expected, outText);
        if (!expected.equals(outText)) {
            errln("======");
            errln("inLevel: " + inLevel);
            errln("outLevel: " + outLevel);
            /* TODO: BidiFwk#u16ToPseudo isn't sufficient for us at present, maybe update it and use here? */
            errln("inText: " + pseudoScript(inText));
            errln("outText: " + pseudoScript(outText));
            errln("expected: " + pseudoScript(expected));
        }
    }

    private void verifyResultsForAllOpts(Object[] test, String inText, String outText, int expectedIdx, int digits, int letters) {
        String expected = (String)test[expectedIdx];
        switch (digits) {
            case ArabicShaping.DIGITS_AN2EN:
            case ArabicShaping.DIGITS_EN2AN:
                expected = shapeNumerals(expected, digits);
                break;
            case ArabicShaping.DIGITS_EN2AN_INIT_AL:
                expected = (String)test[6];
                break;
            case ArabicShaping.DIGITS_NOOP:
                break;
        }
        if ((letters & ArabicShaping.LETTERS_SHAPE) != 0) {
            /*
             * TODO: the goal is not to thoroughly test ArabicShaping, so the test can be quite trivial,
             * but maybe still more sophisticated?
             */
            expected = expected.replace('\u0630', '\ufeab').replace('\u0631', '\ufead').replace('\u0632', '\ufeaf');
        }
        //assertEquals("Unexpected output for All Options", expected, outText);
        if (!expected.equals(outText)) {
            errln("======");
            errln("Test " + test[7]);
            errln("Digits: " + digits);
            errln("Letters: " + letters);
            errln("In: " + pseudoScript(inText));
            errln("Out: " + pseudoScript(outText));
            errln("Expected: " + pseudoScript(expected));
        }
    }

    /*
     * Using the following conventions:
     * AL unshaped: A-E
     * AL shaped: F-J
     * R:  K-Z
     * EN: 0-4
     * AN: 5-9
    */
    private static char substituteChar(char uch, char baseReal,
               char basePseudo, char max) {
        char dest = (char)(basePseudo + (uch - baseReal));
        return dest > max ? max : dest;
    }

    private static String pseudoScript(String text) {
        char[] uchars = text.toCharArray();
        for (int i = uchars.length; i-- > 0;) {
            char uch = uchars[i];
            switch (UCharacter.getDirectionality(uch)) {
                case UCharacter.RIGHT_TO_LEFT:
                    uchars[i] = substituteChar(uch, MIN_HEB_LETTER, 'K', 'Z');
                    break;
                case UCharacter.RIGHT_TO_LEFT_ARABIC:
                    if (uch > 0xFE00) {
                        uchars[i] = substituteChar(uch, MIN_SHAPED_LETTER, 'F', 'J');
                    } else {
                        uchars[i] = substituteChar(uch, MIN_ARAB_LETTER, 'A', 'E');
                    }
                    break;
                case UCharacter.ARABIC_NUMBER:
                    uchars[i] = substituteChar(uch, ARAB_ZERO, '5', '9');
                    break;
                default:
                    break;
            }
        }
        return new String(uchars);
    }

    private static String shapeNumerals(String text, int digits) {
        char[] chars = text.toCharArray();
        char srcZero = LATN_ZERO, destZero = ARAB_ZERO;
        if (digits == ArabicShaping.DIGITS_AN2EN) {
            srcZero = ARAB_ZERO;
            destZero = LATN_ZERO;
        }
        for (int i = chars.length; i-- > 0;) {
            if (chars[i] >= srcZero && chars[i] <= srcZero + 9) {
                chars[i] = substituteChar(chars[i], srcZero, destZero, (char)(destZero + 9));
            }
        }
        return new String(chars);
    }

}
