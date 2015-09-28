/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
  package com.ibm.icu.dev.test.util;

import java.math.BigDecimal;
import java.math.RoundingMode;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.impl.DigitGrouping;
import com.ibm.icu.impl.FixedPrecision;
import com.ibm.icu.impl.ScientificPrecision;
import com.ibm.icu.impl.VisibleDigits;
import com.ibm.icu.impl.VisibleDigits.VFixedDecimal;
import com.ibm.icu.impl.VisibleDigitsWithExponent;


/**
 * @author rocketman
 *
 */
public class DigitGroupingTest extends TestFmwk {
  
    public DigitGroupingTest()
    {
    }
    
    public void TestSimpleGrouping() {
        DigitGrouping dg = new DigitGrouping();
        dg.setGrouping(3);
        assertFalse("", dg.isSeparatorAt(7, -3));
        assertFalse("", dg.isSeparatorAt(7, -2));
        assertFalse("", dg.isSeparatorAt(7, -1));
        assertFalse("", dg.isSeparatorAt(7, 0));
        assertFalse("", dg.isSeparatorAt(7, 1));
        assertFalse("", dg.isSeparatorAt(7, 2));
        assertTrue("", dg.isSeparatorAt(7, 3));
        assertFalse("", dg.isSeparatorAt(7, 4));
        assertFalse("", dg.isSeparatorAt(7, 5));
        assertTrue("", dg.isSeparatorAt(7, 6));
        
        assertEquals("", 0, dg.getSeparatorCount(0));
        assertEquals("", 0, dg.getSeparatorCount(1));
        assertEquals("", 0, dg.getSeparatorCount(2));
        assertEquals("", 0, dg.getSeparatorCount(3));
        assertEquals("", 1, dg.getSeparatorCount(4));
        assertEquals("", 1, dg.getSeparatorCount(5));
        assertEquals("", 1, dg.getSeparatorCount(6));
        assertEquals("", 2, dg.getSeparatorCount(7));
    }
    
    public void TestGrouping2() {
        DigitGrouping dg = new DigitGrouping();
        dg.setGrouping(3);
        dg.setGrouping2(2);
        assertFalse("", dg.isSeparatorAt(7, -3));
        assertFalse("", dg.isSeparatorAt(7, -2));
        assertFalse("", dg.isSeparatorAt(7, -1));
        assertFalse("", dg.isSeparatorAt(7, 0));
        assertFalse("", dg.isSeparatorAt(7, 1));
        assertFalse("", dg.isSeparatorAt(7, 2));
        assertTrue("", dg.isSeparatorAt(7, 3));
        assertFalse("", dg.isSeparatorAt(7, 4));
        assertTrue("", dg.isSeparatorAt(7, 5));
        assertFalse("", dg.isSeparatorAt(7, 6));
        
        assertEquals("", 0, dg.getSeparatorCount(0));
        assertEquals("", 0, dg.getSeparatorCount(1));
        assertEquals("", 0, dg.getSeparatorCount(2));
        assertEquals("", 0, dg.getSeparatorCount(3));
        assertEquals("", 1, dg.getSeparatorCount(4));
        assertEquals("", 1, dg.getSeparatorCount(5));
        assertEquals("", 2, dg.getSeparatorCount(6));
        assertEquals("", 2, dg.getSeparatorCount(7));
    }
    
    public void TestMinGrouping() {
        DigitGrouping dg = new DigitGrouping();
        dg.setGrouping(3);
        dg.setMinGrouping(2);
        assertFalse("", dg.isSeparatorAt(7, -3));
        assertFalse("", dg.isSeparatorAt(7, -2));
        assertFalse("", dg.isSeparatorAt(7, -1));
        assertFalse("", dg.isSeparatorAt(7, 0));
        assertFalse("", dg.isSeparatorAt(7, 1));
        assertFalse("", dg.isSeparatorAt(7, 2));
        assertTrue("", dg.isSeparatorAt(7, 3));
        assertFalse("", dg.isSeparatorAt(7, 4));
        assertFalse("", dg.isSeparatorAt(7, 5));
        assertTrue("", dg.isSeparatorAt(7, 6));
        
        assertFalse("", dg.isSeparatorAt(4, 3));
        assertTrue("", dg.isSeparatorAt(5, 3));
        
        assertEquals("", 0, dg.getSeparatorCount(0));
        assertEquals("", 0, dg.getSeparatorCount(1));
        assertEquals("", 0, dg.getSeparatorCount(2));
        assertEquals("", 0, dg.getSeparatorCount(3));
        assertEquals("", 0, dg.getSeparatorCount(4));
        assertEquals("", 1, dg.getSeparatorCount(5));
        assertEquals("", 1, dg.getSeparatorCount(6));
        assertEquals("", 2, dg.getSeparatorCount(7));
    }
    
    public void TestNoGrouping() {
        DigitGrouping dg = DigitGrouping.NO_GROUPING;

        assertFalse("", dg.isSeparatorAt(7, -3));
        assertFalse("", dg.isSeparatorAt(7, -2));
        assertFalse("", dg.isSeparatorAt(7, -1));
        assertFalse("", dg.isSeparatorAt(7, 0));
        assertFalse("", dg.isSeparatorAt(7, 1));
        assertFalse("", dg.isSeparatorAt(7, 2));
        assertFalse("", dg.isSeparatorAt(7, 3));
        assertFalse("", dg.isSeparatorAt(7, 4));
        assertFalse("", dg.isSeparatorAt(7, 5));
        assertFalse("", dg.isSeparatorAt(7, 6));
        
        assertEquals("", 0, dg.getSeparatorCount(0));
        assertEquals("", 0, dg.getSeparatorCount(1));
        assertEquals("", 0, dg.getSeparatorCount(2));
        assertEquals("", 0, dg.getSeparatorCount(3));
        assertEquals("", 0, dg.getSeparatorCount(4));
        assertEquals("", 0, dg.getSeparatorCount(5));
        assertEquals("", 0, dg.getSeparatorCount(6));
        assertEquals("", 0, dg.getSeparatorCount(7));
    }
      
    // public methods -----------------------------------------------
    
    public static void main(String arg[]) 
    {
        DigitGroupingTest test = new DigitGroupingTest();
        try {
            test.run(arg);
        } catch (Exception e) {
            test.errln("Error testing icubinarytest");
        }
    }

}
