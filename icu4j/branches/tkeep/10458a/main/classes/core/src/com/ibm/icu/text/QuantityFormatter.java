/*
 *******************************************************************************
 * Copyright (C) 2013-2014, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.text;

import java.util.EnumMap;

import com.ibm.icu.impl.PluralMap;
import com.ibm.icu.impl.SimplePatternFormatter;

/**
 * QuantityFormatter represents an unknown quantity of something and formats a known quantity
 * in terms of that something. For example, a QuantityFormatter that represents X apples may
 * format 1 as "1 apple" and 3 as "3 apples" 
 * <p>
 * QuanitityFormatter appears here instead of in com.ibm.icu.impl because it depends on
 * PluralRules and DecimalFormat. It is package-protected as it is not meant for public use.
 */
class QuantityFormatter {
    
    

    /**
     * Builder builds a QuantityFormatter.
     * 
     * @author rocketman
     */
    static class Builder {
        
        private EnumMap<PluralMap.Variant, SimplePatternFormatter> templates =
                        PluralMap.newEnumMap();

        /**
         * Adds a template.
         * @param variant the plural variant, e.g "zero", "one", "two", "few", "many", "other"
         * @param template the text for that plural variant with "{0}" as the quantity. For
         * example, in English, the template for the "one" variant may be "{0} apple" while the
         * template for the "other" variant may be "{0} apples"
         * @return a reference to this Builder for chaining.
         * @throws IllegalArgumentException if variant is not recognized or
         *  if template has more than just the {0} placeholder.
         */
        public Builder add(String variant, String template) {
            SimplePatternFormatter newT = SimplePatternFormatter.compile(template);
            if (newT.getPlaceholderCount() > 1) {
                throw new IllegalArgumentException(
                        "Extra placeholders: " + template);
            }
            templates.put(PluralMap.Variant.valueOfName(variant), newT);
            return this;
        }

        /**
         * Builds the new QuantityFormatter and resets this Builder to its initial state.
         * @return the new QuantityFormatter object.
         * @throws IllegalStateException if no template is specified for the "other" variant.
         *  When throwing this exception, build leaves this builder in its current state.
         */
        public QuantityFormatter build() {
            QuantityFormatter result = new QuantityFormatter(PluralMap.valueOf(templates));
            reset();
            return result;          
        }

        /**
         * Resets this builder to its initial state.
         */
        public Builder reset() {
            templates.clear();
            return this;
        }
    }

    private final PluralMap<SimplePatternFormatter> templates;

    private QuantityFormatter(PluralMap<SimplePatternFormatter> templates) {
        this.templates = templates;
    }

    /**
     * Format formats a quantity with this object.
     * @param quantity the quantity to be formatted
     * @param numberFormat used to actually format the quantity.
     * @param pluralRules uses the quantity and the numberFormat to determine what plural
     *  variant to use for fetching the formatting template.
     * @return the formatted string e.g '3 apples'
     */
    public String format(double quantity, NumberFormat numberFormat, PluralRules pluralRules) {
        String formatStr = numberFormat.format(quantity);
        String variant = computeVariant(quantity, numberFormat, pluralRules);
        return getByVariant(variant).format(formatStr);
    }
    
    /**
     * Gets the SimplePatternFormatter for a particular variant.
     * @param variant "zero", "one", "two", "few", "many", "other"
     * @return the SimplePatternFormatter
     */
    public SimplePatternFormatter getByVariant(String variant) {
        return templates.get(variant);
    }
 
    private String computeVariant(double quantity, NumberFormat numberFormat, PluralRules pluralRules) {
        if (numberFormat instanceof DecimalFormat) {
            return pluralRules.select(((DecimalFormat) numberFormat).getFixedDecimal(quantity));            
        }
        return pluralRules.select(quantity);
    }

 
}
