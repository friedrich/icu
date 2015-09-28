/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

/**
 * Handles DigitGrouping for DecimalFormat.
 */
public final class DigitGrouping extends FreezableBase<DigitGrouping> {
    public static final DigitGrouping NO_GROUPING = new DigitGrouping().freeze();
    
    // Client controls these
    private boolean fGroupingUsed;    
    private int fGrouping;
    private int fGrouping2;
    private int fMinGrouping;
    
    // Class updates these with updateEffFields() when client changes attributes above.
    private boolean fEffGroupingUsed;
    private int fEffGrouping;
    private int fEffGrouping2;
    private int fEffMinGrouping;
    
    /**
     * Default is no digit grouping
     */
    public DigitGrouping() {
        fGroupingUsed = true;
        fMinGrouping = 0;
        fGrouping = 0;
        fGrouping2 = 0;
        updateEffFields();    
    }
    
    /**
     * Returns true if digit grouping is turned on.
     */
    public boolean isGroupingUsed() { return fGroupingUsed; }
    public void setGroupingUsed(boolean b) {
        checkThawed();
        fGroupingUsed = b;
        updateEffFields();
    }
    
    /**
     * Returns the primary grouping size. 3 ==> 12,345,678.9012345
     */
    public int getGrouping() { return fGrouping; }
    public void setGrouping(int i) {
        checkThawed();
        fGrouping = i;
        updateEffFields();
    }
    
    /**
     * Returns the secondary grouping size to use after the first grouping.
     * grouping=4 grouping2=3 ==> 123,456,7890.123456
     */
    public int getGrouping2() { return fGrouping2; }
    public void setGrouping2(int i) {
        checkThawed();
        fGrouping2 = i;
        updateEffFields();
    }
    
    /**
     * Returns the minimum grouping. If minGrouping=2 then 9999 ==> 9999 but 10000 ==> 10,000.
     * minGrouping has no effect when there are multiple grouping separators
     * e.g 1000000 ==> 1,000,000.
     */
    public int getMinGrouping() { return fMinGrouping; }
    public void setMinGrouping(int i) {
        checkThawed();
        fMinGrouping = i;
        updateEffFields();
    }
    
    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof DigitGrouping)) {
            return false;
        }
        DigitGrouping rhs = (DigitGrouping) other;
        // the fEff* fields are dependent on these fields so we only check these fields.
        return (fGroupingUsed == rhs.fGroupingUsed && fMinGrouping == rhs.fMinGrouping
                && fGrouping == rhs.fGrouping && fGrouping2 == rhs.fGrouping2);
    }
    
    @Override
    public int hashCode() {
        int result = fMinGrouping;
        result = 37 * result + fGrouping;
        result = 37 * result + fGrouping2;
        return 37 * result + (fGroupingUsed ? 1 : 0);
    }
    
    /**
     * Determine whether or not a grouping separator belongs after the specified digit.
     * @param digitsLeftOfDecimal the number of digits appearing to the left of the decimal.
     * @param digitPos The position of the digit. in 1,234.56, 1 is at position 3; 2 is at position 2;
     * 3 is at position 1; 4 is at position 0; 5 is at position -1; 6 is at position -2.
     * @return true if a separator belongs after specified digit or false otherwise.
     */
    public boolean isSeparatorAt(int digitsLeftOfDecimal, int digitPos) {
        if (!isGroupingEnabled(digitsLeftOfDecimal) || digitPos < fEffGrouping) {
            return false;
        }
        return ((digitPos - fEffGrouping) % fEffGrouping2 == 0);
    }
    
    /**
     * Returns the total number of grouping separators needed to format a number.
     * @param digitsLeftOfDecimal The number of digits appearing left of the decimal.
     * @return The total number of grouping separators needed.
     */
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
    
}
