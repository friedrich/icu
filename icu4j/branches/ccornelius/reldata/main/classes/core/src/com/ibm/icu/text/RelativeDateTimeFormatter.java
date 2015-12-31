/*
 *******************************************************************************
 * Copyright (C) 2013-2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.text;

import java.util.EnumMap;
import java.util.Locale;

import com.ibm.icu.impl.CalendarData;
import com.ibm.icu.impl.ICUCache;
import com.ibm.icu.impl.ICUResourceBundle;
import com.ibm.icu.impl.SimpleCache;
import com.ibm.icu.impl.StandardPlural;
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.MeasureFormat.FormatWidth;
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
         * TODO: add in ICU57
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
         * ADDED ICU57.
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
        // This class is thread-safe, yet numberFormat is not. To ensure thread-safety of this
        // class we must guarantee that only one thread at a time uses our numberFormat.
        synchronized (numberFormat) {
            result = getQuantity(
                    unit, direction == Direction.NEXT).format(
                            quantity, numberFormat, pluralRules);
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
        String result = this.qualitativeUnitMap.get(style).get(unit).get(direction);
        return result != null ? adjustForContext(result) : null;
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
    
    private static void addQualitativeUnit(
            EnumMap<AbsoluteUnit, EnumMap<Direction, String>> qualitativeUnits,
            AbsoluteUnit unit,
            String current) {
        EnumMap<Direction, String> unitStrings =
                new EnumMap<Direction, String>(Direction.class);
        unitStrings.put(Direction.PLAIN, current);
        qualitativeUnits.put(unit,  unitStrings);       
    }

    private static void addQualitativeUnit(
            EnumMap<AbsoluteUnit, EnumMap<Direction, String>> qualitativeUnits,
            AbsoluteUnit unit, ICUResourceBundle bundle, String plain) {
        EnumMap<Direction, String> unitStrings =
                new EnumMap<Direction, String>(Direction.class);
        unitStrings.put(Direction.LAST, bundle.getStringWithFallback("-1"));
        unitStrings.put(Direction.THIS, bundle.getStringWithFallback("0"));
        unitStrings.put(Direction.NEXT, bundle.getStringWithFallback("1"));
        addOptionalDirection(unitStrings, Direction.LAST_2, bundle, "-2");
        addOptionalDirection(unitStrings, Direction.NEXT_2, bundle, "2");
        unitStrings.put(Direction.PLAIN, plain);
        qualitativeUnits.put(unit,  unitStrings);
    }
 
    private static void addOptionalDirection(
            EnumMap<Direction, String> unitStrings,
            Direction direction,
            ICUResourceBundle bundle,
            String key) {
        String s = bundle.findStringWithFallback(key);
        if (s != null) {
            unitStrings.put(direction, s);
        }
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
    
    private QuantityFormatter getQuantity(RelativeUnit unit, boolean isFuture) {
        QuantityFormatter[] quantities = quantitativeUnitMap.get(style).get(unit);
        return isFuture ? quantities[1] : quantities[0];
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
          return RelativeUnit.QUARTERS;
      }
      // TODO: check on "quarter"
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
      if (keyString.equals("now")) {
          return AbsoluteUnit.NOW;
      }
      if (keyString.equals("quarter")) {       // TODO: check on "quarter"

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
    // TODO: check on PLAIN
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
        private enum DateTimeUnit {
            SECOND("second"),
            MINUTE("minute"),
            HOUR("hour"),
            DAY("day"),
            WEEK("week"),
            MONTH("month"),
            QUARTER("quarter"),  // NEEDED?
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
                        return QUARTER;
                    }
                    break;                    
                default:
                    break;
                }
                return null;
            }
        }
        
        // Constructor
        RelDateTimeFmtDataSink() {}

        EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> sinkQualitativeUnitMap;
        EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> sinkQuantitativeUnitMap;
        
        private String[] toppings = {"Cheese", "Pepperoni", "Black Olives"};

        
        Style style;                        // {LONG, SHORT, NARROW} Derived from name of unit.
        
        // TODO: get rid of this, replacing with local Enum.
        String unitString;                  // From the unit name, with qualifier stripped, e.g., "-short"
        DateTimeUnit dateTimeUnit;
        AbsoluteUnit absUnit;
        String displayName;                 // From "dn".
        int formatterArrayLength;           // Counts items in relativeTime structure.
        String parentKeyString;
        RelativeUnit relativeUnit;          // For relativeTime and relative keys, e.g., "one", "other", "-1", etc.
                
        // Sinks for additional levels under /fields/*/relative/ and /fields/*/relativeTime/
 
        // Sets values under relativeTime paths, e.g., "hour/relativeTime/future/one"
        class RelativeTimeDetailSink extends UResource.TableSink {
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                // TODO: get the reference to the sink data map.
                EnumMap<RelativeUnit, QuantityFormatter[]> relUnitQuantFmtMap = sinkQuantitativeUnitMap.get(style);
                
                // Get the right index from futureOrPast for adding.
                int futurePastIndex = -1;
                if (parentKeyString.contentEquals("past")) {
                    futurePastIndex = 0;
                } else if (parentKeyString.contentEquals("future")) {
                    futurePastIndex = 1;    
                } else {
                    // Not a value relativeTime.
                    return;
                }

                if (relUnitQuantFmtMap.get(relativeUnit)[futurePastIndex] == null) {
                    relUnitQuantFmtMap.get(relativeUnit)[futurePastIndex] = new QuantityFormatter();
                }
                relUnitQuantFmtMap.get(relativeUnit)[futurePastIndex].addIfAbsent(key, value.getString()); 
            }
        }
        RelativeTimeDetailSink relativeTimeDetailSink = new RelativeTimeDetailSink();
        
        // Handles "relativeTime" entries, e.g., under "day", "hour", "minute", "minute-short", etc.
        class RelativeTimeSink extends UResource.TableSink {
            @Override
            public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
                if (key.contentEquals("future") || key.contentEquals("past")) {
                    // Attach QuantityFormatter to the UnitMap.
                    relativeUnit = stringToRelativeUnit(unitString);
                    if (relativeUnit == null) {
                        int x = -1;
                        // stop
                    }
                    // Remember which key it is.
                    parentKeyString = key.toString();
                    
                    // See if the mapping is already there.
                    EnumMap<RelativeUnit, QuantityFormatter[]> relUnitQuantFmtMap = sinkQuantitativeUnitMap.get(style);
                    if (relUnitQuantFmtMap == null) {
                        relUnitQuantFmtMap = new EnumMap<RelativeUnit, QuantityFormatter[]>(RelativeUnit.class);
                        sinkQuantitativeUnitMap.put(style, relUnitQuantFmtMap);
                    }
                    // Add new mapping for relativeUnit, if needed.
                    QuantityFormatter futureOrPast[] = relUnitQuantFmtMap.get(relativeUnit);
                    if (futureOrPast == null) {
                        futureOrPast = new QuantityFormatter[formatterArrayLength];
                        relUnitQuantFmtMap.put(relativeUnit, new QuantityFormatter[2]);
                    }
                    return relativeTimeDetailSink;
                }
                return null;
            }
        }
        RelativeTimeSink relativeTimeSink = new RelativeTimeSink();
 
        // Handles "relative" entries, e.g., under "day", "day-short", "fri", "fri-narrow", "fri-short", etc.
        class RelativeSink extends UResource.TableSink {

            @Override
            public void put(UResource.Key key, UResource.Value value) {
                // TODO: fill in for "-2", "-1", ...
                // Get the ENUM and the value.
                Direction keyDirection = keyToDirection(key);
                String valueString = value.getString();
                absUnit = stringToAbsoluteUnit(unitString);
                if (absUnit == null) {
                    return;  // not interesting.
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
                dirMap.put(keyDirection, valueString);
                                
                // TODO: If needed, create the EnumMap and add PLAIN value.
                // Add value to map.
            }
        }
        RelativeSink relativeSink = new RelativeSink();

        // Handles entries under units, recognizing "relative" and "relativeTime" entries.
        class UnitSink extends UResource.TableSink {
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                if (key.contentEquals("dn")) {  // Handle Display Name
                    displayName = value.toString();
                    // TODO: follow aliases if this is missing.
                }
            }
            
            @Override
            public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
                // If this is 'fields', create and return the fieldsSink.
                if (key.contentEquals("relative")) {
                    return relativeSink;
                } else if (key.contentEquals("relativeTime")) {
                    formatterArrayLength = initialSize;
                    return relativeTimeSink;
                }
                return null;
            }
        }
        UnitSink unitSink = new UnitSink();
        
        // Deals with items under /field/
        class FieldsSink extends UResource.TableSink {
            private Style getKeyStyle(UResource.Key key) {
                int limit;
                Style style = null; 
                if (key.endsWith("-short")) {
                    style = Style.SHORT;
                    limit = key.length()-6;
                    unitString = key.substring(0, limit);
                }
                else if (key.endsWith("-narrow")) {
                    style  = Style.NARROW;
                    limit = key.length()-7;
                    unitString = key.substring(0, limit);
                } else {
                    style = Style.LONG;
                    limit = key.length();
                    unitString = key.toString();
                }
                return style;
            }
            
            private Style styleFromAlias(UResource.Value value) {
                    String s = value.getAliasString();
                    Style style = null; 
                    if (s.endsWith("-short")) {
                        style = Style.SHORT;
                    } else if (s.endsWith("-narrow")) {
                        style  = Style.NARROW;
                    } else {
                        style = Style.LONG;
                    }
                    return style;
            }
            
            private int limitFromKeyStyle(UResource.Key key, Style style) {
                // Finds the last character of the base string, before appended style.
                int limit = -1;
                if (style == Style.SHORT) {
                    limit = key.length()-6;
                }
                else if (style == Style.NARROW) {
                    limit = key.length()-7;
                } else {
                    limit = key.length();
                }
                return limit;
            }
            
            @Override
            public void put(UResource.Key key, UResource.Value value) {
                // Mostly here to detect aliases.
                if (value.getType() != ICUResourceBundle.ALIAS) { return; }

                Style sourceStyle = getKeyStyle(key);
                int limit = limitFromKeyStyle(key, sourceStyle);
                dateTimeUnit = DateTimeUnit.orNullFromString(key.subSequence(0, limit));
                
                if (dateTimeUnit != null) {
                    // Record the fallback chain for the values.
                    // At formatting time, limit to 2 levels of backup.
                    Style targetStyle = styleFromAlias(value);
                    
                    // Should I keep this list for each relative/absolute unit and direction?
                    fallbackCache[sourceStyle.ordinal()] = targetStyle;
                } 
            }

            @Override
            public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
                // Determine if the style is LONG, SHORT, or NARROW.
                // Also get the base unit from the key, i.e., remove any "-short" or "-narrow"
                // TODO: do we filter here on key values, e.g., reject dayperiod", accept "hour*"?
                int limit;
                style = getKeyStyle(key);
                limit = limitFromKeyStyle(key, style);

                displayName = ""; // Needed?
               
                // Check if the unitString is in the white list.
                dateTimeUnit = DateTimeUnit.orNullFromString(key.subSequence(0, limit));
                if (dateTimeUnit == null) {
                    return null;
                }
                unitString = key.substring(0, limit);
                return unitSink;  // Continue parsing this path.
            }
        }
        FieldsSink fieldsSink = new FieldsSink();

        @Override
        public UResource.TableSink getOrCreateTableSink(UResource.Key key, int initialSize) {
            // If this is 'fields', create and return the fieldsSink.
            String stringVal = key.toString();
            if (key.contentEquals("fields")) {
                return fieldsSink;
            }
            return null;
        }
        
    }
    
    private static class Loader {
        private final ULocale ulocale;
        
        public Loader(ULocale ulocale) {
            this.ulocale = ulocale;
        }

        public RelativeDateTimeFormatterData load() {
          
            // TODO: finish.
            // Sink 
            RelDateTimeFmtDataSink sink = new RelDateTimeFmtDataSink();
            // New maps for loading with sink.
            EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> sinkQualitativeUnitMap = 
                new EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>>(Style.class);
            
            // TODO: use the other data structure.
            // sink.sinkQualitativeUnitMap = qualitativeUnitMap;
            sink.sinkQualitativeUnitMap = sinkQualitativeUnitMap;

            EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> sinkQuantitativeUnitMap =
                new EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>>(Style.class);
            // sink.sinkQuantitativeUnitMap = quantitativeUnitMap;
            sink.sinkQuantitativeUnitMap = sinkQuantitativeUnitMap;

            // Previous maps
            EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap = 
                    new EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>>(Style.class);
            
            EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap =
                    new EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>>(Style.class);
            
            for (Style style : Style.values()) {
                qualitativeUnitMap.put(style, new EnumMap<AbsoluteUnit, EnumMap<Direction, String>>(AbsoluteUnit.class));
                quantitativeUnitMap.put(style, new EnumMap<RelativeUnit, QuantityFormatter[]>(RelativeUnit.class));                
                
                // New maps.
                // TODO: consider if these should be initialized using the sink structure.
                sinkQualitativeUnitMap.put(style, new EnumMap<AbsoluteUnit, EnumMap<Direction, String>>(AbsoluteUnit.class));
                sinkQuantitativeUnitMap.put(style, new EnumMap<RelativeUnit, QuantityFormatter[]>(RelativeUnit.class));                
            }
                    
            ICUResourceBundle r = (ICUResourceBundle)UResourceBundle.
                    getBundleInstance(ICUResourceBundle.ICU_BASE_NAME, ulocale);
            
            r.getAllTableItemsWithFallback("", sink);

            addTimeUnits(
                    r,
                    "fields/day", "fields/day-short", "fields/day-narrow",
                    RelativeUnit.DAYS,
                    AbsoluteUnit.DAY,
                    quantitativeUnitMap,
                    qualitativeUnitMap);
            addTimeUnits(
                    r,
                    "fields/week", "fields/week-short", "fields/week-narrow",
                    RelativeUnit.WEEKS,
                    AbsoluteUnit.WEEK,
                    quantitativeUnitMap,
                    qualitativeUnitMap);
            addTimeUnits(
                    r,
                    "fields/month", "fields/month-short", "fields/month-narrow",
                    RelativeUnit.MONTHS,
                    AbsoluteUnit.MONTH,
                    quantitativeUnitMap,
                    qualitativeUnitMap);
            addTimeUnits(
                    r,
                    "fields/year", "fields/year-short", "fields/year-narrow",
                    RelativeUnit.YEARS,
                    AbsoluteUnit.YEAR,
                    quantitativeUnitMap,
                    qualitativeUnitMap);
            initRelativeUnits(
                    r,
                    "fields/second", "fields/second-short", "fields/second-narrow",
                    RelativeUnit.SECONDS,
                    quantitativeUnitMap);
            initRelativeUnits(
                    r,
                    "fields/minute", "fields/minute-short", "fields/minute-narrow",
                    RelativeUnit.MINUTES,
                    quantitativeUnitMap);
            initRelativeUnits(
                    r,
                    "fields/hour", "fields/hour-short", "fields/hour-narrow",
                    RelativeUnit.HOURS,
                    quantitativeUnitMap);
            
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.LONG),
                    AbsoluteUnit.NOW,
                    r.getStringWithFallback("fields/second/relative/0"));
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.SHORT),
                    AbsoluteUnit.NOW,
                    r.getStringWithFallback("fields/second-short/relative/0"));
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.NARROW),
                    AbsoluteUnit.NOW,
                    r.getStringWithFallback("fields/second-narrow/relative/0"));
            
            EnumMap<Style, EnumMap<AbsoluteUnit, String>> dayOfWeekMap = 
                    new EnumMap<Style, EnumMap<AbsoluteUnit, String>>(Style.class);
            dayOfWeekMap.put(Style.LONG, readDaysOfWeek(
                    r.getWithFallback("calendar/gregorian/dayNames/stand-alone/wide")));
            dayOfWeekMap.put(Style.SHORT, readDaysOfWeek(
                    r.getWithFallback("calendar/gregorian/dayNames/stand-alone/short")));
            dayOfWeekMap.put(Style.NARROW, readDaysOfWeek(
                    r.getWithFallback("calendar/gregorian/dayNames/stand-alone/narrow")));
            
            addWeekDays(
                    r,
                    "fields/mon/relative",
                    "fields/mon-short/relative",
                    "fields/mon-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.MONDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/tue/relative",
                    "fields/tue-short/relative",
                    "fields/tue-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.TUESDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/wed/relative",
                    "fields/wed-short/relative",
                    "fields/wed-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.WEDNESDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/thu/relative",
                    "fields/thu-short/relative",
                    "fields/thu-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.THURSDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/fri/relative",
                    "fields/fri-short/relative",
                    "fields/fri-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.FRIDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/sat/relative",
                    "fields/sat-short/relative",
                    "fields/sat-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.SATURDAY,
                    qualitativeUnitMap);
            addWeekDays(
                    r,
                    "fields/sun/relative",
                    "fields/sun-short/relative",
                    "fields/sun-narrow/relative",
                    dayOfWeekMap,
                    AbsoluteUnit.SUNDAY,
                    qualitativeUnitMap);   
            CalendarData calData = new CalendarData(
                    ulocale, r.getStringWithFallback("calendar/default"));
            
            // TODO: Return with new sinkQualitativeUnitMap and sinkQuantitativeUnitMap
            // return new RelativeDateTimeFormatterData(
            //    sinkQualitativeUnitMap, sinkQuantitativeUnitMap, calData.getDateTimePattern());

            return new RelativeDateTimeFormatterData(
                    qualitativeUnitMap, quantitativeUnitMap, calData.getDateTimePattern());
        }

        private void addTimeUnits(
                ICUResourceBundle r,
                String path, String pathShort, String pathNarrow,
                RelativeUnit relativeUnit, 
                AbsoluteUnit absoluteUnit,
                EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap,
                EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap) {
           addTimeUnit(
                   r.getWithFallback(path),
                   relativeUnit,
                   absoluteUnit,
                   quantitativeUnitMap.get(Style.LONG),
                   qualitativeUnitMap.get(Style.LONG));
           addTimeUnit(
                   r.getWithFallback(pathShort),
                   relativeUnit,
                   absoluteUnit,
                   quantitativeUnitMap.get(Style.SHORT),
                   qualitativeUnitMap.get(Style.SHORT));
           addTimeUnit(
                   r.getWithFallback(pathNarrow),
                   relativeUnit,
                   absoluteUnit,
                   quantitativeUnitMap.get(Style.NARROW),
                   qualitativeUnitMap.get(Style.NARROW));
            
        }

        private void addTimeUnit(
                ICUResourceBundle timeUnitBundle,
                RelativeUnit relativeUnit,
                AbsoluteUnit absoluteUnit,
                EnumMap<RelativeUnit, QuantityFormatter[]> quantitativeUnitMap,
                EnumMap<AbsoluteUnit, EnumMap<Direction, String>> qualitativeUnitMap) {
            addTimeUnit(timeUnitBundle, relativeUnit, quantitativeUnitMap);
            String unitName = timeUnitBundle.getStringWithFallback("dn");
            // TODO(Travis Keep): This is a hack to get around CLDR bug 6818.
            if (ulocale.getLanguage().equals("en")) {
                unitName = unitName.toLowerCase();
            }
            timeUnitBundle = timeUnitBundle.getWithFallback("relative");
            addQualitativeUnit(
                    qualitativeUnitMap,
                    absoluteUnit,
                    timeUnitBundle,
                    unitName);
        }
        
        private void initRelativeUnits(
                ICUResourceBundle r, 
                String path,
                String pathShort,
                String pathNarrow,
                RelativeUnit relativeUnit,
                EnumMap<Style, EnumMap<RelativeUnit, QuantityFormatter[]>> quantitativeUnitMap) {
            addTimeUnit(
                    r.getWithFallback(path),
                    relativeUnit,
                    quantitativeUnitMap.get(Style.LONG));
            addTimeUnit(
                    r.getWithFallback(pathShort),
                    relativeUnit,
                    quantitativeUnitMap.get(Style.SHORT));
            addTimeUnit(
                    r.getWithFallback(pathNarrow),
                    relativeUnit,
                    quantitativeUnitMap.get(Style.NARROW));
        }

        private static void addTimeUnit(
                ICUResourceBundle timeUnitBundle,
                RelativeUnit relativeUnit,
                EnumMap<RelativeUnit, QuantityFormatter[]> quantitativeUnitMap) {
            QuantityFormatter future = new QuantityFormatter();
            QuantityFormatter past = new QuantityFormatter();
            timeUnitBundle = timeUnitBundle.getWithFallback("relativeTime");
            addTimeUnit(
                    timeUnitBundle.getWithFallback("future"),
                    future);
            addTimeUnit(
                    timeUnitBundle.getWithFallback("past"),
                    past);
            quantitativeUnitMap.put(
                    relativeUnit, new QuantityFormatter[] { past, future });
        }

        private static void addTimeUnit(
                ICUResourceBundle pastOrFuture, QuantityFormatter qf) {
            int size = pastOrFuture.getSize();
            for (int i = 0; i < size; i++) {
                UResourceBundle r = pastOrFuture.get(i);
                qf.addIfAbsent(r.getKey(), r.getString());
            }
        }
        
        private void addWeekDays(
                ICUResourceBundle r,
                String path,
                String pathShort,
                String pathNarrow,
                EnumMap<Style, EnumMap<AbsoluteUnit, String>> dayOfWeekMap,
                AbsoluteUnit weekDay,
                EnumMap<Style, EnumMap<AbsoluteUnit, EnumMap<Direction, String>>> qualitativeUnitMap) {
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.LONG),
                    weekDay,
                    r.findWithFallback(path),
                    dayOfWeekMap.get(Style.LONG).get(weekDay)); 
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.SHORT),
                    weekDay,
                    r.findWithFallback(pathShort),
                    dayOfWeekMap.get(Style.SHORT).get(weekDay)); 
            addQualitativeUnit(
                    qualitativeUnitMap.get(Style.NARROW),
                    weekDay,
                    r.findWithFallback(pathNarrow),
                    dayOfWeekMap.get(Style.NARROW).get(weekDay)); 
            
        }

        private static EnumMap<AbsoluteUnit, String> readDaysOfWeek(ICUResourceBundle daysOfWeekBundle) {
            EnumMap<AbsoluteUnit, String> dayOfWeekMap = new EnumMap<AbsoluteUnit, String>(AbsoluteUnit.class);
            if (daysOfWeekBundle.getSize() != 7) {
                throw new IllegalStateException(String.format("Expect 7 days in a week, got %d", daysOfWeekBundle.getSize()));
            }
            // Sunday always comes first in CLDR data.
            int idx = 0;
            dayOfWeekMap.put(AbsoluteUnit.SUNDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.MONDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.TUESDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.WEDNESDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.THURSDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.FRIDAY, daysOfWeekBundle.getString(idx++));
            dayOfWeekMap.put(AbsoluteUnit.SATURDAY, daysOfWeekBundle.getString(idx++));
            return dayOfWeekMap;
        }
    }

    private static final Cache cache = new Cache();
}
