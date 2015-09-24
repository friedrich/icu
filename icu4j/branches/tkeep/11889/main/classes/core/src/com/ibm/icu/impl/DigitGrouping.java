/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;


public final class DigitGrouping extends FreezableBase<DigitGrouping> {
    public static final DigitGrouping NO_GROUPING = new DigitGrouping().freeze();
    
    public DigitGrouping() {
        clear();
    }
    
    private boolean fGroupingUsed;    
    public boolean isGroupingUsed() { return fGroupingUsed; }
    public void setGroupingUsed(boolean b) {
        checkThawed();
        fGroupingUsed = b;
    }
    
    private int fGrouping;
    public int getGrouping() { return fGrouping; }
    public void setGrouping(int i) {
        checkThawed();
        fGrouping = i;
        updateEffFields();
    }
    
    private int fGrouping2;
    public int getGrouping2() { return fGrouping2; }
    public void setGrouping2(int i) {
        checkThawed();
        fGrouping2 = i;
        updateEffFields();
    }
    
    private int fMinGrouping;
    public int getMinGrouping() { return fMinGrouping; }
    public void setMinGrouping(int i) {
        checkThawed();
        fMinGrouping = i;
        updateEffFields();
    }
    
    public void clear() {
        checkThawed();
        fGroupingUsed = false;
        fMinGrouping = 0;
        fGrouping = 0;
        fGrouping2 = 0;
        updateEffFields();
    }
    
    public boolean isSeparatorAt(int digitsLeftOfDecimal, int digitPos) {
        if (!isGroupingEnabled(digitsLeftOfDecimal) || digitPos < fEffGrouping) {
            return false;
        }
        return ((digitPos - fEffGrouping) % fEffGrouping2 == 0);
    }
    
    public int getSeparatorCount(int digitsLeftOfDecimal) {
        if (!isGroupingEnabled(digitsLeftOfDecimal)) {
            return 0;
        }
        return (digitsLeftOfDecimal - 1 - fEffGrouping) / fEffGrouping2 + 1;
    }
    
    private boolean isGroupingEnabled(int digitsLeftOfDecimal) {
        return fEffGroupingUsed && digitsLeftOfDecimal >= fEffGrouping + fEffMinGrouping;
    }
  
    private void updateEffFields() {
        fEffGroupingUsed = fGroupingUsed && (fGrouping > 0);
        fEffGrouping = fGrouping;
        fEffGrouping2 = fGrouping2 <= 0  ? fGrouping : fGrouping2;
        fEffMinGrouping = fMinGrouping > 0 ? fMinGrouping : 1;
    }
    
    private boolean fEffGroupingUsed;
    private int fEffGrouping;
    private int fEffGrouping2;
    private int fEffMinGrouping;
}
