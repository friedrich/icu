/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.RoundingMode;

/**
 * @author rocketman
 *
 */
public class Dumb {
  public static void main(String[] args) {
      BigDecimal b = new BigDecimal(-65001);
      
      b = b.setScale(-1, RoundingMode.HALF_EVEN);
      System.out.println(b.scale());
      System.out.println(b.unscaledValue().toString());
      System.out.println(b.unscaledValue().toByteArray().length);
      
      System.out.println(Double.parseDouble("000"));
      
  }
}
