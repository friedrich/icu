/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * @author rocketman
 *
 */
public class ScientificPrecisionDriver {

    /**
     * @param args
     */
    public static void main(String[] args) {
        ScientificPrecision sciPrecision = new ScientificPrecision();
        sciPrecision.getMutableMantissa().getMutableMax().setIntDigitCount(3);
        sciPrecision.getMutableMantissa().getMutableMax().setFracDigitCount(5);
        ScientificPrecision sciPrecision2 = new ScientificPrecision();
        
        // SciPrecision and SciPrecision2 each have their own copy of the mantissa field,
        // but those two copies share their nested fields.
        // If sciPrecision were frozen before this call, then sciPrecision and sciPrecision2
        // would share the same frozen mantissa field.
        sciPrecision2.setMantissa(sciPrecision.getMantissa());
        
        // Now the mantissa fields don't share the same sig field because of copy-on-write
        sciPrecision2.getMutableMantissa().getMutableSig().setMax(4);
        
        // Now they don't share the same max field either
        sciPrecision2.getMutableMantissa().getMutableMax().setFracDigitCount(7);
        
        // Now sciPrecision3 and sciPrecision2 share the same fields
        ScientificPrecision sciPrecision3 = sciPrecision2.clone();
        
        // sciPrecision2 and sciPrecision3 still share the same fields, but each have
        // their own copy of the mantissa field. These two copies of the mantissa field still
        // share their nested fields except each has their own copy of the sig field.
        sciPrecision3.getMutableMantissa().getMutableSig().setMax(6);
        
        System.out.println(sciPrecision);
        System.out.println(sciPrecision2);
        System.out.println(sciPrecision3);
    }

}
