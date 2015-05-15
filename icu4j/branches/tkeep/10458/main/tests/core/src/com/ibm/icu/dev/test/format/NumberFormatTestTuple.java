/*
 *******************************************************************************
 * Copyright (C) 2014, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.format;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

import com.ibm.icu.math.BigDecimal;
import com.ibm.icu.text.DecimalFormat;
import com.ibm.icu.text.NumberFormat;
import com.ibm.icu.util.Currency;
import com.ibm.icu.util.ULocale;

/**
 * A representation of a single NumberFormat specification test from a data driven test file.
 * <p>
 * The purpose of this class is to hide the details of the data driven test file from the
 * main testing code.
 * <p>
 * This class contains fields describing an attribute of the test. Each attribute may
 * contain either nothing (no specific value specified in data driven test file)
 * or just some value. The name of each attribute corresponds to the name used in the
 * data driven test file.
 * <p>
 * <b>Adding new attributes</b>
 * <p>
 * Each attribute name is lower case. Moreover, for each attribute there is also a
 * setXXX method for that attribute that is used to initialize the attribute from a
 * String value read from the data file. For example, there is a setLocale(String) method
 * for the locale attribute and a setCurrency(String) method for the currency attribute.
 * In general, for an attribute named abcd, the setter will be setAbcd(String).
 * This naming rule must be strictly followed or else the test runner will not know how to
 * initialize instances of this class.
 * <p>
 * In addition each attribute is listed in the fieldOrdering static array which specifies
 * The order that attributes are printed whenever there is a test failure.
 * <p> 
 * To add a new attribute, first create a public field for it of type Maybe<T> where T
 * is the type of the attribute. Next, add the attribute name to the fieldOrdering array.
 * Finally, create a setter method for it.
 * 
 * @author rocketman
 */
public class NumberFormatTestTuple {
    
    /**
     * The locale.
     */
    public Maybe<ULocale> locale = Maybe.nothing();
    
    /**
     * The currency.
     */
    public Maybe<Currency> currency = Maybe.nothing();
    
    /**
     * The pattern to initialize the formatter, for example 0.00"
     */
    public Maybe<String> pattern = Maybe.nothing();
    
    /**
     * The value to format as a string. For example 1234.5 would be "1234.5"
     */
    public Maybe<String> format = Maybe.nothing();
    
    /**
     * The formatted value.
     */
    public Maybe<String> output = Maybe.nothing();
    
    /**
     * Field for arbitrary comments.
     */
    public Maybe<String> comment = Maybe.nothing();
    
    public Maybe<Integer> minIntegerDigits = Maybe.nothing();
    public Maybe<Integer> maxIntegerDigits = Maybe.nothing();
    public Maybe<Integer> minFractionDigits = Maybe.nothing();
    public Maybe<Integer> maxFractionDigits = Maybe.nothing();
    public Maybe<Integer> minGroupingDigits = Maybe.nothing();
    public Maybe<Integer> useSigDigits = Maybe.nothing();
    public Maybe<Integer> minSigDigits = Maybe.nothing();
    public Maybe<Integer> maxSigDigits = Maybe.nothing();
    public Maybe<Integer> useGrouping = Maybe.nothing();
    public Maybe<Integer> multiplier = Maybe.nothing();
    public Maybe<Double> roundingIncrement = Maybe.nothing();
    public Maybe<Integer> formatWidth = Maybe.nothing();
    public Maybe<String> padCharacter = Maybe.nothing();
    public Maybe<Integer> useScientific = Maybe.nothing();
    public Maybe<Integer> grouping = Maybe.nothing();
    public Maybe<Integer> grouping2 = Maybe.nothing();
    public Maybe<Integer> roundingMode = Maybe.nothing();
    public Maybe<Currency.CurrencyUsage> currencyUsage = Maybe.nothing();
    public Maybe<Integer> minimumExponentDigits = Maybe.nothing();
    public Maybe<Integer> exponentSignAlwaysShown = Maybe.nothing();
    public Maybe<Integer> decimalSeparatorAlwaysShown = Maybe.nothing();
    public Maybe<Integer> padPosition = Maybe.nothing();
    public Maybe<String> positivePrefix = Maybe.nothing();
    public Maybe<String> positiveSuffix = Maybe.nothing();
    public Maybe<String> negativePrefix = Maybe.nothing();
    public Maybe<String> negativeSuffix = Maybe.nothing();
    public Maybe<String> localizedPattern = Maybe.nothing();
    public Maybe<String> toPattern = Maybe.nothing();
    public Maybe<String> toLocalizedPattern = Maybe.nothing();
    public Maybe<Integer> style = Maybe.nothing();
    public Maybe<String> parse = Maybe.nothing();
    public Maybe<Integer> lenient = Maybe.nothing();
    public Maybe<String> plural = Maybe.nothing();
    
    
    
    /**
     * nothing or empty means that test ought to work for both C and JAVA;
     * "C" means test is known to fail in C. "J" means test is known to fail in JAVA.
     * "CJ" means test is known to fail for both languages.
     */
    public Maybe<String> breaks = Maybe.nothing();
    
    private static Map<String, Integer> roundingModeMap =
            new HashMap<String, Integer>();
    
    static {
        roundingModeMap.put("ceiling", BigDecimal.ROUND_CEILING);
        roundingModeMap.put("floor", BigDecimal.ROUND_FLOOR);
        roundingModeMap.put("down", BigDecimal.ROUND_DOWN);
        roundingModeMap.put("up", BigDecimal.ROUND_UP);
        roundingModeMap.put("halfEven", BigDecimal.ROUND_HALF_EVEN);
        roundingModeMap.put("halfDown", BigDecimal.ROUND_HALF_DOWN);
        roundingModeMap.put("halfUp", BigDecimal.ROUND_HALF_UP);
        roundingModeMap.put("unnecessary", BigDecimal.ROUND_UNNECESSARY);
    }
    
    private static Map<String, Currency.CurrencyUsage> currencyUsageMap =
            new HashMap<String, Currency.CurrencyUsage>();
    
    static {
        currencyUsageMap.put("standard", Currency.CurrencyUsage.STANDARD);
        currencyUsageMap.put("cash", Currency.CurrencyUsage.CASH);
    }
    
    private static Map<String, Integer> padPositionMap =
            new HashMap<String, Integer>();
    
    static {
        // TODO: Fix so that it doesn't depend on DecimalFormat.
        padPositionMap.put("beforePrefix", DecimalFormat.PAD_BEFORE_PREFIX);
        padPositionMap.put("afterPrefix", DecimalFormat.PAD_AFTER_PREFIX);
        padPositionMap.put("beforeSuffix", DecimalFormat.PAD_BEFORE_SUFFIX);
        padPositionMap.put("afterSuffix", DecimalFormat.PAD_AFTER_SUFFIX);
    }
    
    private static Map<String, Integer> formatStyleMap =
            new HashMap<String, Integer>();
    
    static {
        formatStyleMap.put("decimal", NumberFormat.NUMBERSTYLE);
        formatStyleMap.put("currency", NumberFormat.CURRENCYSTYLE);
        formatStyleMap.put("percent", NumberFormat.PERCENTSTYLE);
        formatStyleMap.put("scientific", NumberFormat.SCIENTIFICSTYLE);
        formatStyleMap.put("currencyIso", NumberFormat.ISOCURRENCYSTYLE);
        formatStyleMap.put("currencyPlural", NumberFormat.PLURALCURRENCYSTYLE);
        formatStyleMap.put("currencyAccounting", NumberFormat.ACCOUNTINGCURRENCYSTYLE);
        formatStyleMap.put("cashCurrency", NumberFormat.CASHCURRENCYSTYLE);
    }
    
    // Add any new fields here. On test failures, fields are printed in the same order they
    // appear here.
    private static String[] fieldOrdering = {
        "locale",
        "currency",
        "pattern",
        "format",
        "output",
        "comment",
        "minIntegerDigits",
        "maxIntegerDigits",
        "minFractionDigits",
        "maxFractionDigits",
        "minGroupingDigits",
        "breaks",
        "useSigDigits",
        "minSigDigits",
        "maxSigDigits",
        "useGrouping",
        "multiplier",
        "roundingIncrement",
        "formatWidth",
        "padCharacter",
        "useScientific",
        "grouping",
        "grouping2",
        "roundingMode",
        "currencyUsage",
        "minimumExponentDigits",
        "exponentSignAlwaysShown",
        "decimalSeparatorAlwaysShown",
        "padPosition",
        "positivePrefix",
        "positiveSuffix",
        "negativePrefix",
        "negativeSuffix",
        "localizedPattern",
        "toPattern",
        "toLocalizedPattern",
        "style",
        "parse",
        "lenient",
        "plural",
    };
    
    static {
        HashSet<String> set = new HashSet<String>();
        for (String s : fieldOrdering) {
            if (!set.add(s)) {
                throw new ExceptionInInitializerError(s + "is a duplicate field.");    
            }
        }
    }
    
    private static <T> T fromString(Map<String, T> map, String key) {
        T value = map.get(key);
        if (value == null) {
            throw new IllegalArgumentException("Bad value: "+ key);
        }
        return value;
    }
    
    // start field setters.
    // add setter for each new field in this block.
    
    public void setLocale(String value) {
        locale = Maybe.just(new ULocale(value));
    }
    
    public void setCurrency(String value) {
        currency = Maybe.just(Currency.getInstance(value));
    }
    
    public void setPattern(String value) {
        pattern = Maybe.just(value);
    }
    
    public void setFormat(String value) {
        format = Maybe.just(value);
    }
    
    public void setOutput(String value) {
        output = Maybe.just(value);
    }
    
    public void setComment(String value) {
        comment = Maybe.just(value);
    }
    
    public void setMinIntegerDigits(String value) {
        minIntegerDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMaxIntegerDigits(String value) {
        maxIntegerDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMinFractionDigits(String value) {
        minFractionDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMaxFractionDigits(String value) {
        maxFractionDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMinGroupingDigits(String value) {
        minGroupingDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setBreaks(String value) {
        breaks = Maybe.just(value);
    }
    
    public void setUseSigDigits(String value) {
        useSigDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMinSigDigits(String value) {
        minSigDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMaxSigDigits(String value) {
        maxSigDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setUseGrouping(String value) {
        useGrouping = Maybe.just(Integer.valueOf(value));
    }
    
    public void setMultiplier(String value) {
        multiplier = Maybe.just(Integer.valueOf(value));
    }
    
    public void setRoundingIncrement(String value) {
        roundingIncrement = Maybe.just(Double.valueOf(value));
    }
    
    public void setFormatWidth(String value) {
        formatWidth = Maybe.just(Integer.valueOf(value));
    }
    
    public void setPadCharacter(String value) {
        padCharacter = Maybe.just(value);
    }
    
    public void setUseScientific(String value) {
        useScientific = Maybe.just(Integer.valueOf(value));
    }
    
    public void setGrouping(String value) {
        grouping = Maybe.just(Integer.valueOf(value));
    }
    
    public void setGrouping2(String value) {
        grouping2 = Maybe.just(Integer.valueOf(value));
    }
    
    public void setRoundingMode(String value) {
        roundingMode = Maybe.just(fromString(roundingModeMap, value));
    }
    
    public void setCurrencyUsage(String value) {
        currencyUsage = Maybe.just(fromString(currencyUsageMap, value));
    }
    
    public void setMinimumExponentDigits(String value) {
        minimumExponentDigits = Maybe.just(Integer.valueOf(value));
    }
    
    public void setExponentSignAlwaysShown(String value) {
        exponentSignAlwaysShown = Maybe.just(Integer.valueOf(value));
    }
    
    public void setDecimalSeparatorAlwaysShown(String value) {
        decimalSeparatorAlwaysShown = Maybe.just(Integer.valueOf(value));
    }
    
    public void setPadPosition(String value) {
        padPosition = Maybe.just(fromString(padPositionMap, value));
    }
    
    public void setPositivePrefix(String value) {
        positivePrefix = Maybe.just(value);
    }
    
    public void setPositiveSuffix(String value) {
        positiveSuffix = Maybe.just(value);
    }
    
    public void setNegativePrefix(String value) {
        negativePrefix = Maybe.just(value);
    }
    
    public void setNegativeSuffix(String value) {
        negativeSuffix = Maybe.just(value);
    }
    
    public void setLocalizedPattern(String value) {
        localizedPattern = Maybe.just(value);
    }
    
    public void setToPattern(String value) {
        toPattern = Maybe.just(value);
    }
    
    public void setToLocalizedPattern(String value) {
        toLocalizedPattern = Maybe.just(value);
    }
    
    public void setStyle(String value) {
        style = Maybe.just(fromString(formatStyleMap, value));
    }
    
    public void setParse(String value) {
        parse = Maybe.just(value);
    }
    
    public void setLenient(String value) {
        lenient = Maybe.just(Integer.valueOf(value));
    }
    
    public void setPlural(String value) {
        plural = Maybe.just(value);
    }
    
    // end field setters.
    
    // start of field clearers
    // Add clear methods that can be set in one test and cleared
    // in the next i.e the breaks field.
    
    public void clearBreaks() {
        breaks = Maybe.nothing();
    }
    
    public void clearUseGrouping() {
        useGrouping = Maybe.nothing();
    }
    
    public void clearGrouping2() {
        grouping2 = Maybe.nothing();
    }
    
    public void clearGrouping() {
        grouping = Maybe.nothing();
    }
    
    public void clearMinGroupingDigits() {
        minGroupingDigits = Maybe.nothing();
    }
    
    public void clearUseScientific() {
        useScientific = Maybe.nothing();
    }
    
    public void clearDecimalSeparatorAlwaysShown() {
        decimalSeparatorAlwaysShown = Maybe.nothing();
    }
    
    // end field clearers
    
    public void setField(String fieldName, String valueString)
            throws NoSuchMethodException {
        Method m = getClass().getMethod(
                fieldToSetter(fieldName), String.class);
        try {
            m.invoke(this, valueString);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }
    
    public void clearField(String fieldName)
            throws NoSuchMethodException {
        Method m = getClass().getMethod(fieldToClearer(fieldName));
        try {
            m.invoke(this);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }
    
    public String toString() {
        StringBuilder result = new StringBuilder();
        result.append("{");
        boolean first = true;
        for (String fieldName : fieldOrdering) {
            try {
                Field field = getClass().getField(fieldName);
                Maybe<?> maybeValue = (Maybe<?>) field.get(this);
                if (maybeValue.isNothing()) {
                    continue;
                }
                if (!first) {
                    result.append(", ");
                }
                first = false;
                result.append(fieldName);
                result.append(": ");
                result.append(maybeValue.getValue());
            } catch (NoSuchFieldException e) {
                throw new RuntimeException(e);
            } catch (SecurityException e) {
                throw new RuntimeException(e);
            } catch (IllegalAccessException e) {
                throw new RuntimeException(e);
            }
        }
        result.append("}");
        return result.toString();
    }

    private static String fieldToSetter(String fieldName) {
        return "set"
                + Character.toUpperCase(fieldName.charAt(0))
                + fieldName.substring(1);
    }
    
    private static String fieldToClearer(String fieldName) {
        return "clear"
                + Character.toUpperCase(fieldName.charAt(0))
                + fieldName.substring(1);
    }

}
