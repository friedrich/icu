/*
 *******************************************************************************
 * Copyright (C) 2007-2014, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.text.FieldPosition;
import java.text.ParsePosition;
import java.util.Date;
import java.util.MissingResourceException;
import com.ibm.icu.impl.UResource;
import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.BreakIterator;
import com.ibm.icu.text.DateFormat;
import com.ibm.icu.text.DisplayContext;
import com.ibm.icu.text.MessageFormat;
import com.ibm.icu.text.RelativeDateTimeFormatter;
import com.ibm.icu.text.SimpleDateFormat;
import com.ibm.icu.util.Calendar;
import com.ibm.icu.util.TimeZone;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.util.UResourceBundle;


/**
 * @author srl
 */
public class RelativeDateFormat extends DateFormat {

    /**
     * @author srl
     *
     */
    
    /**
     * @param timeStyle The time style for the date and time.
     * @param dateStyle The date style for the date and time.
     * @param locale The locale for the date.
     * @param cal The calendar to be used
     */
    public RelativeDateFormat(int timeStyle, int dateStyle, ULocale locale, Calendar cal) {
        calendar = cal;

        fLocale = locale;
        fTimeStyle = timeStyle;
        fDateStyle = dateStyle;
        int newStyle = fDateStyle & ~DateFormat.RELATIVE;

        if (fDateStyle != DateFormat.NONE) {
            DateFormat df = DateFormat.getDateInstance(newStyle, locale);
            if (df instanceof SimpleDateFormat) {
                fDateTimeFormat = (SimpleDateFormat)df;
            } else {
                throw new IllegalArgumentException("Can't create SimpleDateFormat for date style");
            }
            fDatePattern = fDateTimeFormat.toPattern();
            if (fTimeStyle != DateFormat.NONE) {
                newStyle = fTimeStyle & ~DateFormat.RELATIVE;
                df = DateFormat.getTimeInstance(newStyle, locale);
                if (df instanceof SimpleDateFormat) {
                    fTimePattern = ((SimpleDateFormat)df).toPattern();
                }
            }
        } else {
            // does not matter whether timeStyle is UDAT_NONE, we need something for fDateTimeFormat
            DateFormat df = DateFormat.getTimeInstance(newStyle, locale);
            if (df instanceof SimpleDateFormat) {
                fDateTimeFormat = (SimpleDateFormat)df;
            } else {
                throw new IllegalArgumentException("Can't create SimpleDateFormat for time style");
            }
            fTimePattern = fDateTimeFormat.toPattern();
        }

        initializeCalendar(fLocale);
        loadDates();
        initializeCombinedFormat(calendar, fLocale);
    }

    /**
     * serial version (generated)
     */
    private static final long serialVersionUID = 1131984966440549435L;

    /* (non-Javadoc)
     * @see com.ibm.icu.text.DateFormat#format(com.ibm.icu.util.Calendar, java.lang.StringBuffer, java.text.FieldPosition)
     */
    public StringBuffer format(Calendar cal, StringBuffer toAppendTo,
            FieldPosition fieldPosition) {

        String relativeDayString = null;
        DisplayContext capitalizationContext = getContext(DisplayContext.Type.CAPITALIZATION);

        if (fDateStyle != DateFormat.NONE) {
            // calculate the difference, in days, between 'cal' and now.
            int dayDiff = dayDifference(cal);

            // look up string
            relativeDayString = getStringForDay(dayDiff);
        }

        if (fDateTimeFormat != null) {
            if (relativeDayString != null && fDatePattern != null &&
                    (fTimePattern == null || fCombinedFormat == null || combinedFormatHasDateAtStart) ) {
                // capitalize relativeDayString according to context for relative, set formatter no context
                if ( relativeDayString.length() > 0 && UCharacter.isLowerCase(relativeDayString.codePointAt(0)) &&
                     (capitalizationContext == DisplayContext.CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE ||
                        (capitalizationContext == DisplayContext.CAPITALIZATION_FOR_UI_LIST_OR_MENU && capitalizationOfRelativeUnitsForListOrMenu) ||
                        (capitalizationContext == DisplayContext.CAPITALIZATION_FOR_STANDALONE && capitalizationOfRelativeUnitsForStandAlone) )) {
                    if (capitalizationBrkIter == null) {
                        // should only happen when deserializing, etc.
                        capitalizationBrkIter = BreakIterator.getSentenceInstance(fLocale);
                    }
                    relativeDayString = UCharacter.toTitleCase(fLocale, relativeDayString, capitalizationBrkIter,
                                    UCharacter.TITLECASE_NO_LOWERCASE | UCharacter.TITLECASE_NO_BREAK_ADJUSTMENT);
                }
                fDateTimeFormat.setContext(DisplayContext.CAPITALIZATION_NONE);
            } else {
                // set our context for the formatter
                fDateTimeFormat.setContext(capitalizationContext);
            }
        }

        if (fDateTimeFormat != null && (fDatePattern != null || fTimePattern != null)) {
            // The new way
            if (fDatePattern == null) {
                // must have fTimePattern
                fDateTimeFormat.applyPattern(fTimePattern);
                fDateTimeFormat.format(cal, toAppendTo, fieldPosition);
            } else if (fTimePattern == null) {
                // must have fDatePattern
                if (relativeDayString != null) {
                    toAppendTo.append(relativeDayString);
                } else {
                    fDateTimeFormat.applyPattern(fDatePattern);
                    fDateTimeFormat.format(cal, toAppendTo, fieldPosition);
                }
            } else {
                String datePattern = fDatePattern; // default;
                if (relativeDayString != null) {
                    // Need to quote the relativeDayString to make it a legal date pattern
                    datePattern = "'" + relativeDayString.replace("'", "''") + "'";
                }
                StringBuffer combinedPattern = new StringBuffer("");
                fCombinedFormat.format(new Object[] {fTimePattern, datePattern}, combinedPattern, new FieldPosition(0));
                fDateTimeFormat.applyPattern(combinedPattern.toString());
                fDateTimeFormat.format(cal, toAppendTo, fieldPosition);
            }
        } else if (fDateFormat != null) {
            // A subset of the old way, for serialization compatibility
            // (just do the date part)
            if (relativeDayString != null) {
                toAppendTo.append(relativeDayString);
            } else {
                fDateFormat.format(cal, toAppendTo, fieldPosition);
            }
        }

        return toAppendTo;
    }

    /* (non-Javadoc)
     * @see com.ibm.icu.text.DateFormat#parse(java.lang.String, com.ibm.icu.util.Calendar, java.text.ParsePosition)
     */
    public void parse(String text, Calendar cal, ParsePosition pos) {
        throw new UnsupportedOperationException("Relative Date parse is not implemented yet");
    }

    /* (non-Javadoc)
     * @see com.ibm.icu.text.DateFormat#setContext(com.ibm.icu.text.DisplayContext)
     * Here we override the DateFormat implementation in order to
     * lazily initialize relevant items 
     */
    public void setContext(DisplayContext context) {
        super.setContext(context);
        if (!capitalizationInfoIsSet &&
              (context==DisplayContext.CAPITALIZATION_FOR_UI_LIST_OR_MENU || context==DisplayContext.CAPITALIZATION_FOR_STANDALONE)) {
            initCapitalizationContextInfo(fLocale);
            capitalizationInfoIsSet = true;
        }
        if (capitalizationBrkIter == null && (context==DisplayContext.CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE ||
              (context==DisplayContext.CAPITALIZATION_FOR_UI_LIST_OR_MENU && capitalizationOfRelativeUnitsForListOrMenu) ||
              (context==DisplayContext.CAPITALIZATION_FOR_STANDALONE && capitalizationOfRelativeUnitsForStandAlone) )) {
            capitalizationBrkIter = BreakIterator.getSentenceInstance(fLocale);
        }
    }

    private DateFormat fDateFormat; // keep for serialization compatibility
    @SuppressWarnings("unused")
    private DateFormat fTimeFormat; // now unused, keep for serialization compatibility
    private MessageFormat fCombinedFormat; //  the {0} {1} format. 
    private SimpleDateFormat fDateTimeFormat = null; // the held date/time formatter
    private String fDatePattern = null;
    private String fTimePattern = null;

    int fDateStyle;
    int fTimeStyle;
    ULocale  fLocale;
    
    private transient String fDateStrings[] = null;  // Size depends on range in RelativeDateTimeFormatter
    
    private boolean combinedFormatHasDateAtStart = false;
    private boolean capitalizationInfoIsSet = false;
    private boolean capitalizationOfRelativeUnitsForListOrMenu = false;
    private boolean capitalizationOfRelativeUnitsForStandAlone = false;
    private transient BreakIterator capitalizationBrkIter = null;
   
    // TODO: TRY using RelativeDateTimeFormatter instead of this implementation.
    // It will need a mapping from the date and time styles. This will effectively 
    // make RDF a wrapper.
    RelativeDateTimeFormatter relDateTimeFmt;
    
    /**
     * Get the string at a specific offset.
     * @param day day offset ( -1, 0, 1, etc.. )
     * @return the string, or NULL if none at that location.
     */
    private String getStringForDay(int day) {
        if(fDateStrings == null) {
            loadDates();
        }
        int dayOffset = day + 2;  // TODO: Compute from LAST_2.
        if (dayOffset >= 0 && dayOffset < fDateStrings.length) {
            return fDateStrings[dayOffset];  // Offset RelativeDateTimeFormatter.LAST_2
        }
        return null;
    }
    
    // Implements basic sink structure to get "fields/day/relative". Use
    //   alias mechanism to handle the other cases.
    //
    private static final class RelDateFmtCapContextSink extends UResource.TableSink {
   
        @Override
        public void put(UResource.Key key, UResource.Value value) {
            if (value.getType() == ICUResourceBundle.ALIAS) { 
                return;
            }

            if (key.contentEquals("relative")) {
              if (contextValues == null) {
                  contextValues = value.getIntVector();
              }
            }
        }
        
        private int[] contextValues;
               
        public int[] getCapitalizationContextValues() {
          return contextValues;
        }
        
        public RelDateFmtCapContextSink() {
          contextValues = null;
        }
    }
    
    // Implements basic sink structure to get "fields/day/relative". Use
    //   alias mechanism to handle the other cases.
    //
    private static final class RelDateFmtDataSink extends UResource.TableSink {

        @Override
        public void put(UResource.Key key, UResource.Value value) {
            if (value.getType() == ICUResourceBundle.ALIAS) { 
                // TODO: Handle alias.
                String alias = value.getAliasString();
                return;
            }

            int keyOffset;
            try {
                keyOffset = Integer.parseInt(key.toString()) + 2;  // TODO: Use LAST_2;
            }
            catch (NumberFormatException nfe) {
                // Flag the error?
                return;
            }
            // Check if already set.
            if (localFDateStrings[keyOffset] == null) {
                localFDateStrings[keyOffset] = value.getString(); 
            }
        }
        
        private String[] localFDateStrings;
        
        public RelDateFmtDataSink(String[] dateStrings) {
            localFDateStrings = dateStrings;
            for (int index = 0; index < localFDateStrings.length; index ++) {
                localFDateStrings[index] = null;
            }
        }
    }
    
    /** 
     * Load the Date string array
     */
    private synchronized void loadDates() {
        ICUResourceBundle rb = (ICUResourceBundle) UResourceBundle.getBundleInstance(ICUResourceBundle.ICU_BASE_NAME, fLocale);

        // Use sink mechanism to traverse data structure.
        if (fDateStrings == null) {
         fDateStrings = new String[6];
        }
        RelDateFmtDataSink sink = new RelDateFmtDataSink(fDateStrings);
        rb.getAllTableItemsWithFallback("fields/day/relative", sink);
        
        RelDateFmtCapContextSink contextSink = new RelDateFmtCapContextSink();
        rb.getAllTableItemsWithFallback("contextTransforms", contextSink);
        int[] contextValues = contextSink.getCapitalizationContextValues();
        if (contextValues != null) {
          
        }
    }
    
    /**
     * Set capitalizationOfRelativeUnitsForListOrMenu, capitalizationOfRelativeUnitsForStandAlone 
     */
    private void initCapitalizationContextInfo(ULocale locale) {
        ICUResourceBundle rb = (ICUResourceBundle) UResourceBundle.getBundleInstance(ICUResourceBundle.ICU_BASE_NAME, locale);
        try { 
            RelDateFmtCapContextSink contextSink = new RelDateFmtCapContextSink();
            rb.getAllTableItemsWithFallback("contextTransforms", contextSink);
            int[] intVector = contextSink.getCapitalizationContextValues();
            if (intVector.length >= 2) {
                capitalizationOfRelativeUnitsForListOrMenu = (intVector[0] != 0);
                capitalizationOfRelativeUnitsForStandAlone = (intVector[1] != 0);
            }
        } catch (MissingResourceException e) {
            // use default
        }
    }

    /**
     * @return the number of days in "until-now"
     */
    private static int dayDifference(Calendar until) {
        Calendar nowCal = (Calendar)until.clone();
        Date nowDate = new Date(System.currentTimeMillis());
        nowCal.clear();
        nowCal.setTime(nowDate);
        int dayDiff = until.get(Calendar.JULIAN_DAY) - nowCal.get(Calendar.JULIAN_DAY);
        return dayDiff;
    }
    
    /**
     * initializes fCalendar from parameters.  Returns fCalendar as a convenience.
     * @param zone  Zone to be adopted, or NULL for TimeZone::createDefault().
     * @param locale Locale of the calendar
     * @param status Error code
     * @return the newly constructed fCalendar
     */
    private Calendar initializeCalendar(ULocale locale) {
        if (calendar == null) {
            calendar = Calendar.getInstance(locale);
        }
        return calendar;
    }

    private MessageFormat initializeCombinedFormat(Calendar cal, ULocale locale) {
        String pattern = "{1} {0}";
        try {
            CalendarData calData = new CalendarData(locale, cal.getType());
            String[] patterns = calData.getDateTimePatterns();
            if (patterns != null && patterns.length >= 9) {
                int glueIndex = 8;
                if (patterns.length >= 13)
                {
                    switch (fDateStyle)
                    {
                        case DateFormat.RELATIVE_FULL:
                        case DateFormat.FULL:
                            glueIndex += (DateFormat.FULL + 1);
                            break;
                        case DateFormat.RELATIVE_LONG:
                        case DateFormat.LONG:
                            glueIndex += (DateFormat.LONG +1);
                            break;
                        case DateFormat.RELATIVE_MEDIUM:
                        case DateFormat.MEDIUM:
                            glueIndex += (DateFormat.MEDIUM +1);
                            break;
                        case DateFormat.RELATIVE_SHORT:
                        case DateFormat.SHORT:
                            glueIndex += (DateFormat.SHORT + 1);
                            break;
                        default:
                            break;
                    }
                }
                pattern = patterns[glueIndex];
            }
        } catch (MissingResourceException e) {
            // use default
        }
        combinedFormatHasDateAtStart = pattern.startsWith("{1}");
        fCombinedFormat = new MessageFormat(pattern, locale);
        return fCombinedFormat;
    }
}
