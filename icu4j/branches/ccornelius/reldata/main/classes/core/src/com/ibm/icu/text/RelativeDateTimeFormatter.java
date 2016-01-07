/*
 *******************************************************************************
 * Copyright (C) 2013-2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.text;

import java.util.EnumMap;
import java.util.EnumSet;
import java.util.Locale;

import com.ibm.icu.impl.CalendarData;
import com.ibm.icu.impl.ICUCache;
import com.ibm.icu.impl.ICUResourceBundle;
import com.ibm.icu.impl.SimpleCache;
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.util.ICUException;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.impl.UResource;
import com.ibm.icu.util.UResourceBundle;


/**
 * Formats simple relative dates. There are two types of relative dates that
 * it handles:
 * <ul>
 *   <li>relative dates with a quantity e.g "in 5 days"</li>
 *   <li>relative dates without a quantity e.g "next Tuesday"</li>
 * </ul>
 * <p>
 * This API is very basic and is intended to be a building block for more
 * fancy APIs. The caller tells it exactly what to display in a locale
 * independent way. While this class automatically provides the correct plural
 * forms, the grammatical form is otherwise as neutral as possible. It is the
 * caller's responsibility to handle cut-off logic such as deciding between
 * displaying "in 7 days" or "in 1 week." This API supports relative dates
 * involving one single unit. This API does not support relative dates
 * involving compound units.
 * e.g "in 5 days and 4 hours" nor does it support parsing.
 * This class is both immutable and thread-safe.
 * <p>
 * Here are some examples of use:
 * <blockquote>
 * <pre>
 * RelativeDateTimeFormatter fmt = RelativeDateTimeFormatter.getInstance();
 * fmt.format(1, Direction.NEXT, RelativeUnit.DAYS); // "in 1 day"
 * fmt.format(3, Direction.NEXT, RelativeUnit.DAYS); // "in 3 days"
 * fmt.format(3.2, Direction.LAST, RelativeUnit.YEARS); // "3.2 years ago"
 * 
 * fmt.format(Direction.LAST, AbsoluteUnit.SUNDAY); // "last Sunday"
 * fmt.format(Direction.THIS, AbsoluteUnit.SUNDAY); // "this Sunday"
 * fmt.format(Direction.NEXT, AbsoluteUnit.SUNDAY); // "next Sunday"
 * fmt.format(Direction.PLAIN, AbsoluteUnit.SUNDAY); // "Sunday"
 * 
 * fmt.format(Direction.LAST, AbsoluteUnit.DAY); // "yesterday"
 * fmt.format(Direction.THIS, AbsoluteUnit.DAY); // "today"
 * fmt.format(Direction.NEXT, AbsoluteUnit.DAY); // "tomorrow"
 * 
 * fmt.format(Direction.PLAIN, AbsoluteUnit.NOW); // "now"
 * </pre>
 * </blockquote>
 * <p>
 * In the future, we may add more forms, such as abbreviated/short forms
 * (3 secs ago), and relative day periods ("yesterday afternoon"), etc.
 * 
 * @stable ICU 53
 */
public final class RelativeDateTimeFormatter {
    
    /**
     * The formatting style
     * @stable ICU 54
     *
     */
    public static enum Style {
        
        /**
         * Everything spelled out.
         * @stable ICU 54
         */
        LONG,
        
        /**
         * Abbreviations used when possible.
         * @stable ICU 54
         */
        SHORT,
        
        /**
         * Use single letters when possible.
         * @stable ICU 54
         */
        NARROW;
        
        private static final int INDEX_COUNT = 3;  // NARROW.ordinal() + 1
    }
    
    /**
     * Represents the unit for formatting a relative date. e.g "in 5 days"
     * or "in 3 months"
     * @stable ICU 53
     */
    public static enum RelativeUnit {
        
        /**
         * Seconds
         * @stable ICU 53
         */
        SECONDS,
        
        /**
         * Minutes
         * @stable ICU 53
         */
        MINUTES,
        
       /**
        * Hours
        * @stable ICU 53
        */
        HOURS,
        
        /**
         * Days
         * @stable ICU 53
         */
        DAYS,
        
        /**
         * Weeks
         * @stable ICU 53
         */
        WEEKS,
        
        /**
         * Months
         * @stable ICU 53
         */
        MONTHS,
        
        /**
         * Years
         * @stable ICU 53
         */
        YEARS,
        
        /**
         * Quarters
         * @internal TODO: propose for addition in ICU 57
         * @provisional This API might change or be removed in a future release.
         */
        QUARTERS,
    }
    
    /**
     * Represents an absolute unit.
     * @stable ICU 53
     */
    public static enum AbsoluteUnit {
        
       /**
        * Sunday
        * @stable ICU 53
        */
        SUNDAY,
        
        /**
         * Monday
         * @stable ICU 53
         */
        MONDAY,
        
        /**
         * Tuesday
         * @stable ICU 53
         */
        TUESDAY,
        
        /**
         * Wednesday
         * @stable ICU 53
         */
        WEDNESDAY,
        
        /**
         * Thursday
         * @stable ICU 53
         */
        THURSDAY,
        
        /**
         * Friday
         * @stable ICU 53
         */
        FRIDAY,
        
        /**
         * Saturday
         * @stable ICU 53
         */
        SATURDAY,
        
        /**
         * Day
         * @stable ICU 53
         */
        DAY,
        
        /**
         * Week
         * @stable ICU 53
         */
        WEEK,
        
        /**
         * Month
         * @stable ICU 53
         */
        MONTH,
        
        /**
         * Year
         * @stable ICU 53
         */
        YEAR,
        
        /**
         * Now
         * @stable ICU 53
         */
        NOW,
        
        /*
         * Quarter
         * @provisional This may be added in ICU57.
         */
        QUARTER,
      }

      /**
       * Represents a direction for an absolute unit e.g "Next Tuesday"
       * or "Last Tuesday"
       * @stable ICU 53
       */
      public static enum Direction {
          
          /**
           * Two before. Not fully supported in every locale
           * @stable ICU 53
           */
          LAST_2,

          /**
           * Last
           * @stable ICU 53
           */  
          LAST,

          /**
           * This
           * @stable ICU 53
           */
          THIS,

          /**
           * Next
           * @stable ICU 53
           */
          NEXT,

          /**
           * Two after. Not fully supported in every locale
           * @stable ICU 53
           */
          NEXT_2,

          /**
           * Plain, which means the absence of a qualifier
           * @stable ICU 53
           */
          PLAIN;
      }

    /**
     * Returns a RelativeDateTimeFormatter for the default locale.
     * @stable ICU 53
     */
    public static RelativeDateTimeFormatter getInstance() {
        return getInstance(ULocale.getDefault(), null, Style.LONG, DisplayContext.CAPITALIZATION_NONE);
    }

    /**
     * Returns a RelativeDateTimeFormatter for a particular locale.
     * 
     * @param locale the locale.
     * @return An instance of RelativeDateTimeFormatter.
     * @stable ICU 53
     */
    public static RelativeDateTimeFormatter getInstance(ULocale locale) {
        return getInstance(locale, null, Style.LONG, DisplayContext.CAPITALIZATION_NONE);
    }

    /**
     * Returns a RelativeDateTimeFormatter for a particular JDK locale.
     * 
     * @param locale the JDK locale.
     * @return An instance of RelativeDateTimeFormatter.
     * @stable ICU 54
     */
    public static RelativeDateTimeFormatter getInstance(Locale locale) {
        return getInstance(ULocale.forLocale(locale));
    }

    /**
     * Returns a RelativeDateTimeFormatter for a particular locale that uses a particular
     * NumberFormat object.
     * 
     * @param locale the locale
     * @param nf the number format object. It is defensively copied to ensure thread-safety
     * and immutability of this class. 
     * @return An instance of RelativeDateTimeFormatter.
     * @stable ICU 53
     */
    public static RelativeDateTimeFormatter getInstance(ULocale locale, NumberFormat nf) {
        return getInstance(locale, nf, Style.LONG, DisplayContext.CAPITALIZATION_NONE);
    }
 
    /**
     * Returns a RelativeDateTimeFormatter for a particular locale that uses a particular
     * NumberFormat object, style, and capitalization context
     * 
     * @param locale the locale
     * @param nf the number format object. It is defensively copied to ensure thread-safety
     * and immutability of this class. May be null.
     * @param style the style.
     * @param capitalizationContext the capitalization context.
     * @stable ICU 54
     */
    public static RelativeDateTimeFormatter getInstance(
            ULocale locale,
            NumberFormat nf,
            Style style,
            DisplayContext capitalizationContext) {
        RelativeDateTimeFormatterData data = cache.get(locale);
        if (nf == null) {
            nf = NumberFormat.getInstance(locale);
        } else {
            nf = (NumberFormat) nf.clone();
        }
        return new RelativeDateTimeFormatter(
                data.qualitativeUnitMap,
                data.quantitativeUnitMap,
                new MessageFormat(data.dateTimePattern),
                PluralRules.forLocale(locale),
                nf,
                style,
                capitalizationContext,
                capitalizationContext == DisplayContext.CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE ?
                    BreakIterator.getSentenceInstance(locale) : null,
                locale);
                
    }
           
    /**
     * Returns a RelativeDateTimeFormatter for a particular JDK locale that uses a particular
     * NumberFormat object.
     * 
     * @param locale the JDK locale
     * @param nf the number format object. It is defensively copied to ensure thread-safety
     * and immutability of this class. 
     * @return An instance of RelativeDateTimeFormatter.
     * @stable ICU 54
     */
    public static RelativeDateTimeFormatter getInstance(Locale locale, NumberFormat nf) {
        return getInstance(ULocale.forLocale(locale), nf);
    }

    /**
     * Formats a relative date with a quantity such as "in 5 days" or
     * "3 months ago"
     * @param quantity The numerical amount e.g 5. This value is formatted
     * according to this object's {@link NumberFormat} object.
     * @param direction NEXT means a future relative date; LAST means a past
     * relative date.
     * @param unit the unit e.g day? month? year?
     * @return the formatted string
     * @throws IllegalArgumentException if direction is something other than
     * NEXT or LAST.
     * @stable ICU 53
     */
    public String format(double quantity, Direction direction, RelativeUnit unit) {
        if (direction != Direction.LAST && direction != Direction.NEXT) {
            throw new IllegalArgumentException("direction must be NEXT or LAST");
        }
        String result;
        int quantIndex = (direction == Direction.NEXT ? 1 : 0);

        // This class is thread-safe, yet numberFormat is not. To ensure thread-safety of this
        // class we must guarantee that only one thread at a time uses our numberFormat.
        synchronized (numberFormat) {
            QuantityFormatter fmt = getQuantitativeResultWithFallback(style, unit, quantIndex);
            result = fmt.format(quantity, numberFormat, pluralRules);
        }
        return adjustForContext(result);
    }
    

    /**
     * Formats a relative date without a quantity.
     * @param direction NEXT, LAST, THIS, etc.
     * @param unit e.g SATURDAY, DAY, MONTH
     * @return the formatted string. If direction has a value that is documented as not being
     *  fully supported in every locale (for example NEXT_2 or LAST_2) then this function may
     *  return null to signal that no formatted string is available.
     * @throws IllegalArgumentException if the direction is incompatible with
     * unit this can occur with NOW which can only take PLAIN.
     * @stable ICU 53
     */
    public String format(Direction direction, AbsoluteUnit unit) {
        if (unit == AbsoluteUnit.NOW && direction != Direction.PLAIN) {
            throw new IllegalArgumentException("NOW can only accept direction PLAIN.");
        }
        // Get formatting string with fallback.
        String result = getQualitativeResultWithFallback(style, unit, direction);
        return result != null ? adjustForContext(result) : null;
    }

    /**
     * Gets the string value from qualitativeUnitMap with fall back based on style.
     * @param style
     * @param unit
     * @param direction
     * @return
     */
    private String getQualitativeResultWithFallback(Style style, AbsoluteUnit unit, Direction direction) {
        if (style == null || unit == null || direction == null) {
            return null;
        }
        String result;
        EnumMap<AbsoluteUnit, EnumMap<Direction, String>> unitMap;
        EnumMap<Direction, String> dirMap;

        unitMap = qualitativeUnitMap.get(style);
        if (unitMap != null) {
            dirMap = unitMap.get(unit);
            if (dirMap != null) {
                result = dirMap.get(direction);
                if (result != null) {
                    return result;
                }
            }
        }

        // Consider new styles from alias fall back.
        Style newStyle = fallbackCache[style.ordinal()];
        if (newStyle == null) {
            // No fall back possible.
            return null;
        }
        unitMap = qualitativeUnitMap.get(newStyle);
        if (unitMap != null) {
            dirMap = unitMap.get(unit);
            if (dirMap != null) {
                result = dirMap.get(direction);
                if (result != null) {
                    return result;
                }
            }
        }
        // Second fallback.
        newStyle = fallbackCache[newStyle.ordinal()];
        if (newStyle == null) {
            // No fall back possible.
            return null;
        }
        unitMap = qualitativeUnitMap.get(newStyle);
        if (unitMap != null) {
            dirMap = unitMap.get(unit);
            if (dirMap != null) {
                result = dirMap.get(direction);
                if (result != null) {
                    return result;
                }
            }
        }
        // Only two fall back steps are supported. 
        return null;
    }
            
    /**
     * Combines a relative date string and a time string in this object's
     * locale. This is done with the same date-time separator used for the
     * default calendar in this locale.
     * @param relativeDateString the relative date e.g 'yesterday'
     * @param timeString the time e.g '3:45'
     * @return the date and time concatenated according to the default
     * calendar in this locale e.g 'yesterday, 3:45'
     * @stable ICU 53
     */
    public String combineDateAndTime(String relativeDateString, String timeString) {
        return this.combinedDateAndTime.format(
            new Object[]{timeString, relativeDateString}, new StringBuffer(), null).toString();
    }
    
    /**
     * Returns a copy of the NumberFormat this object is using.
     * @return A copy of the NumberFormat.
     * @stable ICU 53
     */
    public NumberFormat getNumberFormat() {
        // This class is thread-safe, yet numberFormat is not. To ensure thread-safety of this
        // class we must guarantee that only one thread at a time uses our numberFormat.
        synchronized (numberFormat) {
            return (NumberFormat) numberFormat.clone();
        }
    }
    
    /**
     * Return capitalization context.
     *
     * @stable ICU 54
     */
    public DisplayContext getCapitalizationContext() {
        return capitalizationContext;
    }

    /**
     * Return style
     *
     * @stable ICU 54
     */
    public Style getFormatStyle() {
        return style;
    }
    
    private String adjustForContext(String originalFormattedString) {
        if (breakIterator == null || originalFormattedString.length() == 0 
                || !UCharacter.isLowerCase(UCharacter.codePointAt(originalFormattedString, 0))) {
            return originalFormattedString;
        }
        synchronized (breakIterator) {
            return UCharacter.toTitleCase(
                    locale,
                    originalFormattedString,
                    breakIterator,
                    UCharacter.TITLECASE_NO_LOWERCASE | UCharacter.TITLECASE_NO_BREAK_ADJUSTMENT);
        }
    }
    
    // Sets PLAIN direction string for AbsoluteUnit NOW.
    private static void addQualitativeUnitForNow(
            EnumMap<AbsoluteUnit, EnumMap<Direction, String>> qualitativeUnits,
            String current) {
        EnumMap<Direction, String> unitStrings = new EnumMap<Direction, String>(Direction.class);
        unitStrings.put(Direction.PLAIN, current);
        qualitativeUnits.put(AbsoluteUnit.NOW,  unitStrings);       
    }

    private RelativeDateTimeFormatter(
            EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap,
            EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap,
            MessageFormat combinedDateAndTime,
            PluralRules pluralRules,
            NumberFormat numberFormat,
            Style style,
            DisplayContext capitalizationContext,
            BreakIterator breakIterator,
            ULocale locale) {
        this.qualitativeUnitMap = qualitativeUnitMap;
        this.quantitativeUnitMap = quantitativeUnitMap;
        this.combinedDateAndTime = combinedDateAndTime;
        this.pluralRules = pluralRules;
        this.numberFormat = numberFormat;
        this.style = style;
        if (capitalizationContext.type() != DisplayContext.Type.CAPITALIZATION) {
            throw new IllegalArgumentException(capitalizationContext.toString());
        }
        this.capitalizationContext = capitalizationContext;
        this.breakIterator = breakIterator;
        this.locale = locale;
    }
     
    private QuantityFormatter getQuantitativeResultWithFallback(Style style, RelativeUnit unit, int quantIndex) {
        if (style == null || unit == null) {
            return null;
        }
        EnumMap<RelativeUnit, QuantityFormatter[]> unitMap = quantitativeUnitMap.get(style);
        QuantityFormatter [] quantFormatterArray;
        
        /* TODO: find a way to remove the isValid() call, since that only checks for plurality OTHER. */
        if (unitMap != null) {
            quantFormatterArray = unitMap.get(unit);
            if (quantFormatterArray != null) {
                if (quantFormatterArray != null && quantFormatterArray[quantIndex].isValid()) {
                    return quantFormatterArray[quantIndex];
                }
            }
        }

        // Consider other styles from alias fall back.
        Style newStyle = fallbackCache[style.ordinal()];
        if (newStyle == null) {
            return null;
        }
        unitMap = quantitativeUnitMap.get(newStyle);
        if (unitMap != null) {
            quantFormatterArray = unitMap.get(unit);
            if (quantFormatterArray != null) {
                if (quantFormatterArray != null && quantFormatterArray[quantIndex].isValid()) {
                    return quantFormatterArray[quantIndex];
                }
            }
        }
        // Second fall back.
        newStyle = fallbackCache[style.ordinal()];
        if (newStyle == null) {
            return null;
        }
        unitMap = quantitativeUnitMap.get(newStyle);
        if (unitMap != null) {
            quantFormatterArray = unitMap.get(unit);
            if (quantFormatterArray != null) {
                if (quantFormatterArray != null && quantFormatterArray[quantIndex].isValid()) {
                    return quantFormatterArray[quantIndex];
                }
            }
        }
        return null;
    }
    
    private final EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap;
    private final EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap;
    private final MessageFormat combinedDateAndTime;
    private final PluralRules pluralRules;
    private final NumberFormat numberFormat;
    private final Style style;
    private final DisplayContext capitalizationContext;
    private final BreakIterator breakIterator;
    private final ULocale locale;
    
    // 
    private static final Style fallbackCache[] = new Style[Style.INDEX_COUNT];

    
    private static class RelativeDateTimeFormatterData {
        public RelativeDateTimeFormatterData(
                EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap,
                EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap,
                String dateTimePattern) {
            this.qualitativeUnitMap = qualitativeUnitMap;
            this.quantitativeUnitMap = quantitativeUnitMap;
            this.dateTimePattern = dateTimePattern;
        }
        
        public final EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap;
        public final EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap;
        public final String dateTimePattern;  // Example: "{1}, {0}"
    }
    
    private static class Cache {
        private final ICUCache<String, RelativeDateTimeFormatterData> cache =
            new SimpleCache<String, RelativeDateTimeFormatterData>();

        public RelativeDateTimeFormatterData get(ULocale locale) {
            String key = locale.toString();
            RelativeDateTimeFormatterData result = cache.get(key);
            if (result == null) {
                result = new Loader(locale).load();
                cache.put(key, result);
            }
            return result;
        }
    }
    
    private static RelativeUnit stringToRelativeUnit(String keyString) {
      if (keyString.equals("second")) {
          return RelativeUnit.SECONDS;
      }
      if (keyString.equals("minute")) {
          return RelativeUnit.MINUTES;
      }
      if (keyString.equals("hour")) {
          return RelativeUnit.HOURS;
      }
      if (keyString.equals("day")) {
          return RelativeUnit.DAYS;
      }
      if (keyString.equals("week")) {
          return RelativeUnit.WEEKS;
      }
      if (keyString.equals("month")) {
          return RelativeUnit.MONTHS;
      }
      if (keyString.equals("year")) {
          return RelativeUnit.YEARS;
      }
      if (keyString.equals("quarter")) {
          return RelativeUnit.QUARTERS;  // TODO: Check @provisional
      }
      return null;
  }


  private static AbsoluteUnit stringToAbsoluteUnit(String keyString) {
      if (keyString.equals("sun")) {
          return AbsoluteUnit.SUNDAY;
      }
      if (keyString.equals("mon")) {
          return AbsoluteUnit.MONDAY;
      }
      if (keyString.equals("tue")) {
          return AbsoluteUnit.TUESDAY;
      }
      if (keyString.equals("wed")) {
          return AbsoluteUnit.WEDNESDAY;
      }
      if (keyString.equals("thu")) {
          return AbsoluteUnit.THURSDAY;
      }
      if (keyString.equals("fri")) {
          return AbsoluteUnit.FRIDAY;
      }
      if (keyString.equals("sat")) {
          return AbsoluteUnit.SATURDAY;
      }
      if (keyString.equals("day")) {
          return AbsoluteUnit.DAY;
      }
      if (keyString.equals("week")) {
          return AbsoluteUnit.WEEK;
      }
      if (keyString.equals("month")) {
          return AbsoluteUnit.MONTH;
      }
      if (keyString.equals("year")) {
          return AbsoluteUnit.YEAR;
      }
      if (keyString.equals("second")) {
          return AbsoluteUnit.NOW;
      }
      if (keyString.equals("quarter")) {       // TODO: Check @provisional
          return AbsoluteUnit.QUARTER;
      }
      return null;
  }


private static Direction keyToDirection(UResource.Key key) {
    if (key.contentEquals("-2")) {
        return Direction.LAST_2;
    }
    if (key.contentEquals("-1")) {
        return Direction.LAST;
    }
    if (key.contentEquals("0")) {
        return Direction.THIS;
    }
    if (key.contentEquals("1")) {
        return Direction.NEXT;
    }
    if (key.contentEquals("2")) {
        return Direction.NEXT_2;
    }
    return null;
}


/**
     * Sink for enumerating all of the relative data time formatter names.
     * Contains inner sink classes, each one corresponding to a type of resource table.
     * The outer sink handles the top-level fields, calendar tables.
     *
     * More specific bundles (en_GB) are enumerated before their parents (en_001, en, root):
     * Only store a value if it is still missing, that is, it has not been overridden.
     *
     * C++: Each inner sink class has a reference to the main outer sink.
     * Java: Use non-static inner classes instead.
     */
    private static final class RelDateTimeFmtDataSink extends UResource.TableSink {
        
        // For white list of units to handle in RelativeDateTimeFormatter.
        private static enum DateTimeUnit {
            SECOND("second"),
            MINUTE("minute"),
            HOUR("hour"),
            DAY("day"),
            WEEK("week"),
            MONTH("month"),
            QUARTER("quarter"),  // TODO: Check @provisional
            YEAR("year"),
            SUNDAY("sun"),
            MONDAY("mon"),
            TUESDAY("tue"),
            WEDNESDAY("wed"),
            THURSDAY("thu"),
            FRIDAY("fri"),
            SATURDAY("sat");
            
            private final String keyword;

            private DateTimeUnit(String kw) {
                keyword = kw;
            }
            
            private static final DateTimeUnit orNullFromString(CharSequence keyword) {
                // Quick check from string to enum.
                switch (keyword.length()) {
                case 3:
                    if ("day".contentEquals(keyword)) {
                        return DAY;
                    } else if ("sun".contentEquals(keyword)) {
                        return SUNDAY;
                    } else if ("mon".contentEquals(keyword)) {
                        return MONDAY;
                    } else if ("tue".contentEquals(keyword)) {
                        return TUESDAY;
                    } else if ("wed".contentEquals(keyword)) {
                        return WEDNESDAY;
                    } else if ("thu".contentEquals(keyword)) {
                        return THURSDAY;
                    }    else if ("fri".contentEquals(keyword)) {
                        return FRIDAY;
                    } else if ("sat".contentEquals(keyword)) {
                        return SATURDAY;
                    }
                    break;
                case 4:
                    if ("hour".contentEquals(keyword)) {
                        return HOUR;
                    } else if ("week".contentEquals(keyword)) {
                        return WEEK;
                    } else if ("year".contentEquals(keyword)) {
                        return YEAR;
                    }
                    break;
                case 5:
                    if ("month".contentEquals(keyword)) {
                        return MONTH;
                    }
                    break;
                case 6:
                    if ("minute".contentEquals(keyword)) {
                        return MINUTE;
                    }else if ("second".contentEquals(keyword)) {
                        return SECOND;
                    }
                    break;
                case 7:
                    if ("quarter".contentEquals(keyword)) {
                        return QUARTER;  // TODO: Check @provisional
                    }
                    break;                    
                default:
                    break;
                }
                return null;
            }
        }

        EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> sinkQualitativeUnitMap;
        EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> sinkQuantitativeUnitMap;

        // Values keep between levels of parsing the CLDR data.
        String displayName;                 // From the "dn" key, used for some Direction.PLAIN values. 
        int pastFutureIndex;
        RelativeUnit relativeUnit;          // For relativeTime and relative keys, e.g., "one", "other", "-1", etc.
        Style style;                        // {LONG, SHORT, NARROW} Derived from name of unit.
        String unitString;                  // From the unit name, with qualifier stripped, e.g., "-short"
    
        private Style styleFromKey(UResource.Key key) {
            if (key.endsWith("-short")) {
                return Style.SHORT;
            } else if (key.endsWith("-narrow")) {
                return Style.NARROW;
            } else {
                return Style.LONG;
            }
        }
        
        private Style styleFromAlias(UResource.Value value) {
                String s = value.getAliasString();
                if (s.endsWith("-short")) {
                    return Style.SHORT;
                } else if (s.endsWith("-narrow")) {
                    return Style.NARROW;
                } else {
                    return Style.LONG;
                }
        }

        private static int styleSuffixLength(Style style) {
            switch (style) {
            case SHORT: return 6;
            case NARROW: return 7;
            default: return 0;
            }
        }

        @Override
        public void put(UResource.Key key, UResource.Value value) {
            // Mostly here to detect aliases.
            if (value.getType() != ICUResourceBundle.ALIAS) { return; }

            Style sourceStyle = styleFromKey(key);
            int limit = key.length() - styleSuffixLength(sourceStyle);
            DateTimeUnit unit = DateTimeUnit.orNullFromString(key.substring(0, limit));
            if (unit != null) {
                // Record the fall back chain for the values.
                // At formatting time, limit to 2 levels of backup.
                Style targetStyle = styleFromAlias(value);
                
                // Check for inconsistent fall back or loop in fallback.
                if (fallbackCache[sourceStyle.ordinal()] == null) {
                    fallbackCache[sourceStyle.ordinal()] = targetStyle;
                } else if (fallbackCache[sourceStyle.ordinal()] != targetStyle ||
                        fallbackCache[sourceStyle.ordinal()] == sourceStyle) {
                    throw new ICUException("Invalid style fallback for style in: " + value.getAliasString());
                }
            } 
        }

        @Override
        public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
            // Figure out the style:is LONG, SHORT, or NARROW.
            // Get base unit from the key, i.e., remove any "-short" or "-narrow"
            style = styleFromKey(key);
            int limit = key.length() - styleSuffixLength(style);
            unitString = key.substring(0, limit);

            // Check if the unitString is in the white list.
            if (DateTimeUnit.orNullFromString(unitString) == null) {
                unitString = "";
                return null;
            }
            return unitSink;  // Continue parsing this path.
        }

        // Sinks for additional levels under /fields/*/relative/ and /fields/*/relativeTime/
 
        // Sets values under relativeTime paths, e.g., "hour/relativeTime/future/one"
        class RelativeTimeDetailSink extends UResource.TableSink {
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                // TODO: Replace QuantityFormatter values with arrays of SimplePatterFormatters. 
                EnumMap<RelativeUnit, QuantityFormatter[]> relUnitQuantFmtMap = sinkQuantitativeUnitMap.get(style);
                if (relUnitQuantFmtMap == null) {
                    relUnitQuantFmtMap = new EnumMap<RelativeUnit, QuantityFormatter[]>(RelativeUnit.class);
                    sinkQuantitativeUnitMap.put(style, relUnitQuantFmtMap);
                }
                QuantityFormatter futureOrPast[] = relUnitQuantFmtMap.get(relativeUnit);
                if (futureOrPast == null) {
                    futureOrPast = new QuantityFormatter[2];
                    relUnitQuantFmtMap.put(relativeUnit, futureOrPast);
                }
                if (relUnitQuantFmtMap.get(relativeUnit)[pastFutureIndex] == null) {
                    relUnitQuantFmtMap.get(relativeUnit)[pastFutureIndex] = new QuantityFormatter();
                }
                relUnitQuantFmtMap.get(relativeUnit)[pastFutureIndex].addIfAbsent(key, value.getString()); 
            }
        }
        RelativeTimeDetailSink relativeTimeDetailSink = new RelativeTimeDetailSink();
        
        // Handles "relativeTime" entries, e.g., under "day", "hour", "minute", "minute-short", etc.
        class RelativeTimeSink extends UResource.TableSink {
            @Override
            public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
                if (key.contentEquals("past")) {
                    pastFutureIndex = 0;
                } else if (key.contentEquals("future")) {
                    pastFutureIndex = 1;
                } else {
                    return null;
                }
                // Attach QuantityFormatter to the UnitMap.
                relativeUnit = stringToRelativeUnit(unitString);
                if (relativeUnit == null) {
                    return null;
                }
                return relativeTimeDetailSink;
            }
        }
        RelativeTimeSink relativeTimeSink = new RelativeTimeSink();
 
        // Handles "relative" entries, e.g., under "day", "day-short", "fri", "fri-narrow", "fri-short", etc.
        class RelativeSink extends UResource.TableSink {
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                Direction keyDirection = keyToDirection(key);
                if (keyDirection == null) {
                    return;
                }
                String valueString = value.getString();
                AbsoluteUnit absUnit = stringToAbsoluteUnit(unitString);
                if (absUnit == null) {
                    return;
                }
                EnumMap<AbsoluteUnit, EnumMap<Direction, String>> absMap = sinkQualitativeUnitMap.get(style);
                
                if (absMap == null) {
                    absMap = new EnumMap<AbsoluteUnit, EnumMap<Direction, String>>(AbsoluteUnit.class);
                    sinkQualitativeUnitMap.put(style, absMap);
                }
                EnumMap<Direction, String> dirMap = absMap.get(absUnit);
                if (dirMap == null) {
                    dirMap = new EnumMap<Direction, String>(Direction.class);
                    absMap.put(absUnit, dirMap);
                }
                if (dirMap.get(keyDirection) == null) {
                    // Do not override values already entered.
                    dirMap.put(keyDirection, valueString);
                }            
            }
        }
        RelativeSink relativeSink = new RelativeSink();

        // Handles entries under units, recognizing "relative" and "relativeTime" entries.
        class UnitSink extends UResource.TableSink {
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                if (key.contentEquals("dn")) {
                    // Handle Display Name for PLAIN direction for some units.
                    displayName = value.toString();
           
                    // For consistency in display names with aliases in some locales, e.g., "en".
                    displayName = displayName.toLowerCase();
                    
                    AbsoluteUnit absUnit = stringToAbsoluteUnit(unitString);
                    if (absUnit == null) {
                        return;  // Not interesting.
                    }
                    EnumMap<AbsoluteUnit, EnumMap<Direction, String>> unitMap = sinkQualitativeUnitMap.get(style);
                    if (unitMap == null) {
                        unitMap = new EnumMap<AbsoluteUnit, EnumMap<Direction, String>>(AbsoluteUnit.class);
                        sinkQualitativeUnitMap.put(style, unitMap);
                    }
                    EnumMap<Direction,String> dirMap = unitMap.get(absUnit);
                    if (dirMap == null) {
                        dirMap = new EnumMap<Direction,String>(Direction.class);
                        unitMap.put(absUnit, dirMap);
                    }
                    dirMap.put(Direction.PLAIN, displayName);
                }
            }
            
            @Override
            public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
                if (key.contentEquals("relative")) {
                    return relativeSink;
                } else if (key.contentEquals("relativeTime")) {
                    return relativeTimeSink;
                }
                return null;
            }
        }
        UnitSink unitSink = new UnitSink();
    }
    
    private static class Loader {
        private final ULocale ulocale;
        
        public Loader(ULocale ulocale) {
            this.ulocale = ulocale;
        }

        public RelativeDateTimeFormatterData load() {
            // EnumMaps
            EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap = 
                    new EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>>(Style.class);
            EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap =
                    new EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>>(Style.class);

            // Sink for traversing data.
            RelDateTimeFmtDataSink sink = new RelDateTimeFmtDataSink();
            sink.sinkQualitativeUnitMap = qualitativeUnitMap;
            sink.sinkQuantitativeUnitMap = quantitativeUnitMap;
            
            // Creat top level Style entries in maps.
            for (Style style : Style.values()) {
                qualitativeUnitMap.put(style, new EnumMap<AbsoluteUnit, EnumMap<Direction, String>>(AbsoluteUnit.class));
                quantitativeUnitMap.put(style, new EnumMap<RelativeUnit, QuantityFormatter[]>(RelativeUnit.class));                
            }
                    
            ICUResourceBundle r = (ICUResourceBundle)UResourceBundle.
                    getBundleInstance(ICUResourceBundle.ICU_BASE_NAME, ulocale);
            
            // Use sink mechanism to traverse data structure.
            r.getAllTableItemsWithFallback("fields", sink);

            // Check fall backs array for loops or too many levels.
            for (Style testStyle : Style.values()) {
                Style newStyle1 = fallbackCache[testStyle.ordinal()];
                if (newStyle1 == testStyle) {
                    throw new IllegalStateException("Loop in style fallback");
                }
                if (newStyle1 != null) {
                    Style newStyle2 = fallbackCache[newStyle1.ordinal()];
                    if (newStyle2 == newStyle1 || newStyle2 == testStyle) {
                        throw new IllegalStateException("Loop in style fallback");
                    }
                    if (newStyle2 != null) {
                        // No fallback should take more than
                        if (fallbackCache[newStyle2.ordinal()] != null) {
                            throw new IllegalStateException("Loop in style fallback");
                        }
                    }
                }
            }
      
            /* Special case: Direction PLAIN for seconds needs a specific value. */
            addQualitativeUnitForNow(
                    qualitativeUnitMap.get(Style.LONG),
                    r.getStringWithFallback("fields/second/relative/0"));
            addQualitativeUnitForNow(
                    qualitativeUnitMap.get(Style.SHORT),
                    r.getStringWithFallback("fields/second-short/relative/0"));
            addQualitativeUnitForNow(
                    qualitativeUnitMap.get(Style.NARROW),
                    r.getStringWithFallback("fields/second-narrow/relative/0"));
            
            // Special case for week days
            setDaysOfWeekForStyle(Style.LONG, "calendar/gregorian/dayNames/stand-alone/wide", r,
                    qualitativeUnitMap);      
            setDaysOfWeekForStyle(Style.SHORT, "calendar/gregorian/dayNames/stand-alone/short", r,
                    qualitativeUnitMap);
            setDaysOfWeekForStyle(Style.NARROW, "calendar/gregorian/dayNames/stand-alone/narrow", r,
                    qualitativeUnitMap);           
         
            CalendarData calData = new CalendarData(
                    ulocale, r.getStringWithFallback("calendar/default"));

            return new RelativeDateTimeFormatterData(
                    qualitativeUnitMap, quantitativeUnitMap, calData.getDateTimePattern());
        }
       
        // Set up PLAIN elements for weekdays.
        private void setDaysOfWeekForStyle(Style style, String path, ICUResourceBundle r,
                EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap) {

            // Get day of week text from resources.
            ICUResourceBundle daysOfWeekBundle = r.getWithFallback(path);
            
            if (daysOfWeekBundle.getSize() != 7) {
                throw new IllegalStateException(
                        String.format("Expect 7 days in a week, got %d", daysOfWeekBundle.getSize()));
            }
            
            EnumMap<AbsoluteUnit, EnumMap<Direction, String>> unitMap = qualitativeUnitMap.get(style);
            
            AbsoluteUnit[] weekdays = {AbsoluteUnit.SUNDAY, AbsoluteUnit.MONDAY, AbsoluteUnit.TUESDAY,
                    AbsoluteUnit.WEDNESDAY, AbsoluteUnit.THURSDAY, AbsoluteUnit.FRIDAY, AbsoluteUnit.SATURDAY};
            
            // Sunday always comes first in CLDR data.
            int idx = 0;
            for (AbsoluteUnit dayOfWeek : weekdays) {
                String dayText = daysOfWeekBundle.getString(idx++);
                EnumMap<Direction, String> dirMap = unitMap.get(dayOfWeek);
                if (dirMap == null) {
                    dirMap = new EnumMap<Direction, String>(Direction.class);
                }
                dirMap.put(Direction.PLAIN, dayText);
            }
        }
    }

    private static final Cache cache = new Cache();
}
