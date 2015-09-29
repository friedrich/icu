/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.util;

import java.text.FieldPosition;
import java.util.Locale;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.impl.DigitFormatter;
import com.ibm.icu.impl.DigitFormatter.Options;
import com.ibm.icu.impl.DigitGrouping;
import com.ibm.icu.impl.FieldPositionHandler;
import com.ibm.icu.impl.FieldPositionHandlers;
import com.ibm.icu.impl.FixedPrecision;
import com.ibm.icu.impl.ScientificPrecision;
import com.ibm.icu.impl.VisibleDigits;
import com.ibm.icu.text.DecimalFormatSymbols;
import com.ibm.icu.text.NumberFormat;
import com.ibm.icu.text.NumberFormat.Field;

/**
 * @author rocketman
 *
 */
public class DigitFormatterTest extends TestFmwk {
       
    public static void main(String[] args) throws Exception {
        new DigitFormatterTest().run(args);
    }
    
    public void TestDigitFormatterMonetary() {
        DecimalFormatSymbols sym = DecimalFormatSymbols.getInstance(Locale.ENGLISH);
        sym.setMonetaryGroupingSeparator('G');
        sym.setMonetaryDecimalSeparator('D');
        DigitGrouping grouping = new DigitGrouping();
        grouping.setGrouping(3);
        DigitFormatter formatter = DigitFormatter.getMonetaryInstance(sym);
        assertEquals("", "43G560D02", 
                formatter.format(
                        FixedPrecision.DEFAULT.toVisibleDigits(43560.02),
                        grouping,
                        DigitFormatter.Options.DEFAULT,
                        FieldPositionHandlers.DONT_CARE,
                        new StringBuilder()).toString()); 
    }
    
    public void TestDigitFormatterNonNumeric() {
        DecimalFormatSymbols sym = DecimalFormatSymbols.getInstance(Locale.ENGLISH);
        DigitFormatter formatter = DigitFormatter.getMonetaryInstance(sym);
        FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);
        assertEquals("", "NaN",
                formatter.format(
                        VisibleDigits.NOT_A_NUMBER,
                        DigitGrouping.NO_GROUPING,
                        Options.DEFAULT,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 3);
        handler.verifyNoMoreFields();
        
        assertEquals("", "∞",
                formatter.format(
                        VisibleDigits.POSITIVE_INFINITY,
                        DigitGrouping.NO_GROUPING,
                        Options.DEFAULT,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 1);
        handler.verifyNoMoreFields();
    }
    
    public void TestDigitFormatter() {
        DecimalFormatSymbols sym = DecimalFormatSymbols.getInstance(Locale.ENGLISH);
        DigitFormatter formatter = DigitFormatter.getMonetaryInstance(sym);
        {
            DigitGrouping grouping = new DigitGrouping();
            DigitFormatter.Options options = new DigitFormatter.Options();
            options.setAlwaysShowDecimal(true);
            FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);
            VisibleDigits digits = FixedPrecision.DEFAULT.toVisibleDigits(8192);
            assertEquals("", "8192.", 
                    formatter.format(
                            digits,
                            grouping,
                            options,
                            handler,
                            new StringBuilder()).toString());
            handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 4);
            handler.verify(-1, Field.DECIMAL_SEPARATOR, 4, 5);
            handler.verifyNoMoreFields();
            
            grouping.setGrouping(3);
            options.setAlwaysShowDecimal(false);
            
            assertEquals("", "8,192", 
                    formatter.format(
                            digits,
                            grouping,
                            options,
                            FieldPositionHandlers.DONT_CARE,
                            new StringBuilder()).toString());
            
            grouping.setMinGrouping(2);
            
            assertEquals("", "8192", 
                    formatter.format(
                            digits,
                            grouping,
                            options,
                            FieldPositionHandlers.DONT_CARE,
                            new StringBuilder()).toString());
            
            assertEquals("", "43,560", 
                    formatter.format(
                            FixedPrecision.DEFAULT.toVisibleDigits(43560),
                            grouping,
                            options,
                            FieldPositionHandlers.DONT_CARE,
                            new StringBuilder()).toString());
        }
        {
            DigitGrouping grouping = new DigitGrouping();
            FixedPrecision precision = new FixedPrecision();
            assertEquals("", "31415926.0078125", 
                    formatter.format(
                            precision.toVisibleDigits(31415926.0078125),
                            grouping,
                            Options.DEFAULT,
                            FieldPositionHandlers.DONT_CARE,
                            new StringBuilder()).toString());
            
            grouping.setGrouping(2);
            grouping.setGrouping2(3);
            
            assertEquals("", "314,159,26.0078125", 
                    formatter.format(
                            precision.toVisibleDigits(31415926.0078125),
                            grouping,
                            Options.DEFAULT,
                            FieldPositionHandlers.DONT_CARE,
                            new StringBuilder()).toString());
            
            precision.getMutableMin().setIntDigitCount(9);
            precision.getMutableMin().setFracDigitCount(10);
            FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);
            
            assertEquals("", "0,314,159,26.0078125000", 
                    formatter.format(
                            precision.toVisibleDigits(31415926.0078125),
                            grouping,
                            Options.DEFAULT,
                            handler,
                            new StringBuilder()).toString());
            handler.verify(-1, Field.GROUPING_SEPARATOR, 1, 2);
            handler.verify(-1, Field.GROUPING_SEPARATOR, 5, 6);
            handler.verify(-1, Field.GROUPING_SEPARATOR, 9, 10);
            handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 12);
            handler.verify(-1, Field.DECIMAL_SEPARATOR, 12, 13);
            handler.verify(NumberFormat.FRACTION_FIELD, Field.FRACTION, 13, 23);
            handler.verifyNoMoreFields();
        }
        {
            FixedPrecision precision = new FixedPrecision();
            FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);
            DigitFormatter.Options options = new DigitFormatter.Options();
            precision.getMutableMax().setIntDigitCount(0);
            precision.getMutableMax().setFracDigitCount(0);
            
            assertEquals("", "0", 
                    formatter.format(
                            precision.toVisibleDigits(3125),
                            DigitGrouping.NO_GROUPING,
                            options,
                            handler,
                            new StringBuilder()).toString());
            
            handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 1);
            handler.verifyNoMoreFields();
            
            options.setAlwaysShowDecimal(true);
            
            assertEquals("", "0.", 
                    formatter.format(
                            precision.toVisibleDigits(3125),
                            DigitGrouping.NO_GROUPING,
                            options,
                            handler,
                            new StringBuilder()).toString());
            
            handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 1);
            handler.verify(-1, Field.DECIMAL_SEPARATOR, 1, 2);
            handler.verifyNoMoreFields();
            
            precision = new FixedPrecision();
            precision.getMutableMax().setIntDigitCount(1);
            precision.getMutableMin().setFracDigitCount(1);
            assertEquals("", "5.0", 
                    formatter.format(
                            precision.toVisibleDigits(3125),
                            DigitGrouping.NO_GROUPING,
                            Options.DEFAULT,
                            handler,
                            new StringBuilder()).toString());
            
            handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 1);
            handler.verify(-1, Field.DECIMAL_SEPARATOR, 1, 2);
            handler.verify(NumberFormat.FRACTION_FIELD, Field.FRACTION, 2, 3);
            handler.verifyNoMoreFields();
        }
    }
    
    public void TestSciFormatterNonNumeric() {
        DecimalFormatSymbols sym = DecimalFormatSymbols.getInstance(Locale.ENGLISH);
        DigitFormatter formatter = DigitFormatter.getMonetaryInstance(sym);
        FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);
        assertEquals("", "NaN",
                formatter.format(
                        ScientificPrecision.DEFAULT.toVisibleDigitsWithExponent(Double.NaN),
                        Options.DEFAULT,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 3);
        handler.verifyNoMoreFields();
        
        assertEquals("", "∞",
                formatter.format(
                        ScientificPrecision.DEFAULT.toVisibleDigitsWithExponent(Double.POSITIVE_INFINITY),
                        Options.DEFAULT,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 1);
        handler.verifyNoMoreFields();
    }
        
    public void TestSciFormatter() {
        DecimalFormatSymbols sym = DecimalFormatSymbols.getInstance(Locale.ENGLISH);
        DigitFormatter formatter = DigitFormatter.getMonetaryInstance(sym);
        ScientificPrecision precision = new ScientificPrecision();
        DigitFormatter.Options options = new DigitFormatter.Options();
        FieldPositionHandlerForTesting handler = new FieldPositionHandlerForTesting(this.params);    
        assertEquals("", "6.02E23", 
                formatter.format(
                        precision.toVisibleDigitsWithExponent(6.02E23),
                        options,
                        FieldPositionHandlers.DONT_CARE,
                        new StringBuilder()).toString());
        assertEquals("", "6.62E-34", 
                formatter.format(
                        precision.toVisibleDigitsWithExponent(6.62E-34),
                        options,
                        FieldPositionHandlers.DONT_CARE,
                        new StringBuilder()).toString());
        
        precision.getMutableMantissa().getMutableMin().setIntDigitCount(4);
        precision.getMutableMantissa().getMutableMax().setIntDigitCount(4);
        precision.getMutableMantissa().getMutableMin().setFracDigitCount(0);
        precision.getMutableMantissa().getMutableMax().setFracDigitCount(0);
        precision.setMinExponentDigits(3);
        
        options.setAlwaysShowExponentSign(true);
        
        assertEquals("", "1248E+023", 
                formatter.format(
                        precision.toVisibleDigitsWithExponent(1.248E26),
                        options,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 4);
        handler.verify(-1, Field.EXPONENT_SYMBOL, 4, 5);
        handler.verify(-1,  Field.EXPONENT_SIGN, 5, 6);
        handler.verify(-1,  Field.EXPONENT, 6, 9);
        handler.verifyNoMoreFields();
        
        options.setAlwaysShowDecimal(true);
        options.setAlwaysShowExponentSign(false);
        
        assertEquals("", "1248.E023", 
                formatter.format(
                        precision.toVisibleDigitsWithExponent(1.248E26),
                        options,
                        handler,
                        new StringBuilder()).toString());
        handler.verify(NumberFormat.INTEGER_FIELD, Field.INTEGER, 0, 4);
        handler.verify(-1, Field.DECIMAL_SEPARATOR, 4, 5);
        handler.verify(-1, Field.EXPONENT_SYMBOL, 5, 6);
        handler.verify(-1,  Field.EXPONENT, 6, 9);
        handler.verifyNoMoreFields();
        
    }
    
    public void TestFieldPositionHandlersFieldPositionInteger() {
        FieldPosition fp = new FieldPosition(NumberFormat.INTEGER_FIELD);
        FieldPositionHandler fph = FieldPositionHandlers.forFieldPosition(fp);
        fph.addAttribute(-1, Field.CURRENCY, 3, 4);
        fph.addAttribute(NumberFormat.FRACTION_FIELD, Field.FRACTION, 5, 7);
        assertEquals("begin not found", 0, fp.getBeginIndex());
        assertEquals("end not found", 0, fp.getEndIndex());
        fph.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, 7, 9);
        fph.addAttribute(-1, Field.EXPONENT, 13, 14);
        fph.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, 17, 20);
        assertEquals("begin", 7, fp.getBeginIndex());
        assertEquals("end", 9, fp.getEndIndex());
    }
    
    public void TestFieldPositionHandlersFieldPositionField() {
        FieldPosition fp = new FieldPosition(Field.EXPONENT);
        FieldPositionHandler fph = FieldPositionHandlers.forFieldPosition(fp);
        fph.addAttribute(-1, Field.CURRENCY, 3, 4);
        fph.addAttribute(NumberFormat.FRACTION_FIELD, Field.FRACTION, 5, 7);
        fph.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, 7, 9);
        assertEquals("begin not found", 0, fp.getBeginIndex());
        assertEquals("end not found", 0, fp.getEndIndex());
        fph.addAttribute(-1, Field.EXPONENT, 13, 14);
        fph.addAttribute(NumberFormat.INTEGER_FIELD, Field.INTEGER, 17, 20);
        fph.addAttribute(-1, Field.EXPONENT, 25, 28);
        assertEquals("begin", 13, fp.getBeginIndex());
        assertEquals("end", 14, fp.getEndIndex());
    }

}
