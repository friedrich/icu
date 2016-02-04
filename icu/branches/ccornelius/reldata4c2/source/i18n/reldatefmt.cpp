/*
******************************************************************************
* Copyright (C) 2014-2016, International Business Machines Corporation and
* others. All Rights Reserved.
******************************************************************************
*
* File reldatefmt.cpp
******************************************************************************
*/

#include "unicode/reldatefmt.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_BREAK_ITERATION

#include "cstr.h"
#include "unicode/ureldatefmt.h"
#include "unicode/udisplaycontext.h"
#include "unicode/unum.h"
#include "unicode/localpointer.h"
#include "resource.h"
#include "unicode/dtfmtsym.h"
#include "unicode/simpleformatter.h"
#include "quantityformatter.h"
#include "unicode/plurrule.h"
#include "unicode/msgfmt.h"
#include "unicode/decimfmt.h"
#include "unicode/numfmt.h"
#include "unicode/brkiter.h"
#include "uresimp.h"
#include "unicode/ures.h"
#include "cstring.h"
#include "ucln_in.h"
#include "mutex.h"
#include "charstr.h"
#include "uassert.h"

#include "sharedbreakiterator.h"
#include "sharedpluralrules.h"
#include "sharednumberformat.h"
#include "standardplural.h"
#include "unifiedcache.h"

// Copied from uscript_props.cpp

// TODO: Remove when done debugging.
#include <iostream>

using namespace std;

static UMutex gBrkIterMutex = U_MUTEX_INITIALIZER;

U_NAMESPACE_BEGIN

// RelativeDateTimeFormatter specific data for a single locale
class RelativeDateTimeCacheData: public SharedObject {
public:
    RelativeDateTimeCacheData() : combinedDateAndTime(NULL) {
        for (int i = 0; i < UDAT_STYLE_COUNT; ++i) {
          fallBackCache[i] = -1;
        }
    }
    virtual ~RelativeDateTimeCacheData();

    // no numbers: e.g Next Tuesday; Yesterday; etc.
    UnicodeString absoluteUnits[UDAT_STYLE_COUNT][UDAT_ABSOLUTE_UNIT_COUNT][UDAT_DIRECTION_COUNT];
    const UnicodeString& getAbsoluteUnitString(int fStyle,
                                                UDateAbsoluteUnit unit,
                                                UDateDirection direction) const;
    const SimpleFormatter* getRelativeUnitFormatter(int fStyle,
                                                     UDateRelativeUnit unit,
                                                     int pastFutureIndex,
                                                     int pluralUnit) const;

  // has numbers: e.g Next Tuesday; Yesterday; etc. For second index, 0
    // means past e.g 5 days ago; 1 means future e.g in 5 days.
    // TODO: remove this eventually.
    QuantityFormatter relativeUnits[UDAT_STYLE_COUNT][UDAT_RELATIVE_UNIT_COUNT][2];

    // The new data structure to be used in formatting.
    SimpleFormatter *relativeUnitsFormatter[UDAT_STYLE_COUNT]
      [UDAT_RELATIVE_UNIT_COUNT][2][StandardPlural::COUNT];

    // Mappping from source to target styles for alias fallback.
    int fallBackCache[UDAT_STYLE_COUNT];

    void adoptCombinedDateAndTime(MessageFormat *mfToAdopt) {
        delete combinedDateAndTime;
        combinedDateAndTime = mfToAdopt;
    }
    const MessageFormat *getCombinedDateAndTime() const {
        return combinedDateAndTime;
    }

private:
    MessageFormat *combinedDateAndTime;
    RelativeDateTimeCacheData(const RelativeDateTimeCacheData &other);
    RelativeDateTimeCacheData& operator=(
            const RelativeDateTimeCacheData &other);
};

RelativeDateTimeCacheData::~RelativeDateTimeCacheData() {
    delete combinedDateAndTime;
}


// Use fallback cache for absolute units.
const UnicodeString& RelativeDateTimeCacheData::getAbsoluteUnitString(
    int fStyle, UDateAbsoluteUnit unit, UDateDirection direction) const {
      int style = fStyle;
      do {
        if (!absoluteUnits[style][unit][direction].isEmpty()) {
          return absoluteUnits[style][unit][direction];
        }
         style = fallBackCache[style];
       } while (style != -1);
       return absoluteUnits[fStyle][unit][direction]; //  The original, which is empty.
     }

 // Use fallback cache for SimpleFormatter relativeUnits.
 const SimpleFormatter* RelativeDateTimeCacheData::getRelativeUnitFormatter(
     int fStyle,
     UDateRelativeUnit unit,
     int pastFutureIndex,
     int pluralUnit) const {
   int style = fStyle;
   do {
     if (relativeUnitsFormatter[style][unit][pastFutureIndex][pluralUnit]) {
       return relativeUnitsFormatter[style][unit][pastFutureIndex][pluralUnit];
     }
     style = fallBackCache[style];
   } while (style != -1);
   return NULL;  // No formatter found.
 }

 static UBool getStringWithFallback(
         const UResourceBundle *resource,
         const char *key,
         UnicodeString &result,
         UErrorCode &status) {
     int32_t len = 0;
     const UChar *resStr = ures_getStringByKeyWithFallback(
         resource, key, &len, &status);
     if (U_FAILURE(status)) {
         return FALSE;
     }
     result.setTo(TRUE, resStr, len);
     return TRUE;
 }


 static UBool getStringByIndex(
         const UResourceBundle *resource,
         int32_t idx,
         UnicodeString &result,
         UErrorCode &status) {
     int32_t len = 0;
     const UChar *resStr = ures_getStringByIndex(
             resource, idx, &len, &status);
     if (U_FAILURE(status)) {
         return FALSE;
     }
     result.setTo(TRUE, resStr, len);
     return TRUE;
 }

 namespace {

 /**
  * Sink for enumerating all of the measurement unit display names.
  * Contains inner sink classes, each one corresponding to a type of resource table.
  * The outer sink handles the top-level units, unitsNarrow, and unitsShort tables.
  *
  * More specific bundles (en_GB) are enumerated before their parents (en_001, en, root):
  * Only store a value if it is still missing, that is, it has not been overridden.
  *
  * C++: Each inner sink class has a reference to the main outer sink.
  */
 struct RelDateTimeFmtDataSink : public ResourceTableSink {

     /**
      * Sink for patterns for relative dates and times. For example,
      * fields/relative/...
      * TODO: Fix comment
      */

   // Generic unit enum for storing Unit info.
   typedef enum RelAbsUnit {
     SECOND,
     MINUTE,
     HOUR,
     DAY,
     WEEK,
     MONTH,
     QUARTER,
     YEAR,
     SUNDAY,
     MONDAY,
     TUESDAY,
     WEDNESDAY,
     THURSDAY,
     FRIDAY,
     SATURDAY,
     INVALID_UNIT
   } RelAbsUnit;

   static int genericToRelUnit(RelAbsUnit genUnit) {
     // Converts the generic units to UDAT_RELATIVE version.
     switch (genUnit) {
       case SECOND:
         return UDAT_RELATIVE_SECONDS;
       case MINUTE:
         return UDAT_RELATIVE_MINUTES;
       case HOUR:
         return UDAT_RELATIVE_HOURS;
       case DAY:
         return UDAT_RELATIVE_DAYS;
       case WEEK:
         return UDAT_RELATIVE_WEEKS;
       case MONTH:
         return UDAT_RELATIVE_MONTHS;
         /*
          * case QUARTER:
          * return UDATE_RELATIVE_QUARTERS;
          */
       case YEAR:
         return UDAT_RELATIVE_YEARS;
       default:
         return -1;
     }
   }

   static int genericToAbsUnit(RelAbsUnit genUnit) {
     // Converts the generic units to UDAT_RELATIVE version.
     switch (genUnit) {
       case DAY:
         return UDAT_ABSOLUTE_DAY;
       case WEEK:
         return UDAT_ABSOLUTE_WEEK;
       case MONTH:
         return UDAT_ABSOLUTE_MONTH;
         /* TODO: Add in QUARTER
          *  case QUARTER:
          * return UDAT_ABSOLUTE_QUARTER;
          */
       case YEAR:
         return UDAT_ABSOLUTE_YEAR;
       case SUNDAY:
         return UDAT_ABSOLUTE_SUNDAY;
       case MONDAY:
         return UDAT_ABSOLUTE_MONDAY;
       case TUESDAY:
         return UDAT_ABSOLUTE_TUESDAY;
       case WEDNESDAY:
         return UDAT_ABSOLUTE_WEDNESDAY;
       case THURSDAY:
         return UDAT_ABSOLUTE_THURSDAY;
       case FRIDAY:
         return UDAT_ABSOLUTE_FRIDAY;
       case SATURDAY:
         return UDAT_ABSOLUTE_SATURDAY;
       default:
         return -1;
     }
   }

     static int32_t keyToDirection(const char* key) {
       if (uprv_strcmp(key, "-2") == 0) {
         return int(UDAT_DIRECTION_LAST_2);
       }
       if (uprv_strcmp(key, "-1") == 0) {
         return UDAT_DIRECTION_LAST;
       }
       if (uprv_strcmp(key, "0") == 0) {
         return UDAT_DIRECTION_THIS;
       }
       if (uprv_strcmp(key, "1") == 0) {
         return UDAT_DIRECTION_NEXT;
       }
       if (uprv_strcmp(key, "2") == 0) {
         return UDAT_DIRECTION_NEXT_2;
       }
       return -1;
     }



   // Sinks for additional levels under /fields/*/relative/ and /fields/*/relativeTime/
   /* Make list of simplePatternFmtList, for past and for future.
    *  Set a SimpleFormatter for the <style, relative unit, plurality>
    *
    * Fill in values for the particular plural given, e.g., ONE, FEW, OTHER, etc.
    */
   struct RelDateTimeDetailSink : public ResourceTableSink {
     RelDateTimeDetailSink(RelDateTimeFmtDataSink &sink) : outer(sink) {}
     ~RelDateTimeDetailSink();

     virtual void put(const char *key, const ResourceValue &value,
                      UErrorCode &errorCode) {
       if (U_FAILURE(errorCode)) { return; }

       outer.relUnitIndex = genericToRelUnit(outer.genericUnit);
       if (outer.relUnitIndex == -1) {
         return;
       }

       /* Make two lists of simplePatternFmtList, one for past and one for future.
        *  Set a SimpleFormatter pattern for the <style, relative unit, plurality>
        *
        * Fill in values for the particular plural given, e.g., ONE, FEW, OTHER, etc.
        */
       int pluralIndex = StandardPlural::indexOrNegativeFromString(key);
       SimpleFormatter **pattern =
             &outer.outputData.relativeUnitsFormatter[outer.style][outer.relUnitIndex]
           [outer.pastFutureIndex][pluralIndex];

       if (pluralIndex != -1) {
           // Only set if not already established.
         if (U_SUCCESS(errorCode) && *pattern == NULL) {
           *pattern = new SimpleFormatter(
               value.getUnicodeString(errorCode), 0, 1, errorCode);

           if (U_SUCCESS(errorCode) && *pattern == NULL) {
             errorCode = U_MEMORY_ALLOCATION_ERROR;
           }
         }
       }
     }

     RelDateTimeFmtDataSink &outer;
   } relDateTimeDetailSink;

   /*
    * Handles "relativeTime" entries, e.g., under "day", "hour", "minute",
    * "minute-short", etc.
    */
   struct RelativeTimeSink : public ResourceTableSink {
         RelativeTimeSink(RelDateTimeFmtDataSink &sink) : outer(sink) {}
         ~RelativeTimeSink();

     virtual ResourceTableSink *getOrCreateTableSink(
         const char *key, int32_t /* initialSize */, UErrorCode &errorCode) {
       outer.relUnitIndex = genericToRelUnit(outer.genericUnit);
       if (outer.relUnitIndex == -1) {
         return NULL;
       }

       if (uprv_strcmp(key, "past") == 0) {
         outer.pastFutureIndex = 0;
       } else if (uprv_strcmp(key, "future") == 0) {
         outer.pastFutureIndex = 1;
       } else {
         // Unknown key.
         return NULL;
       }
       return &outer.relDateTimeDetailSink;
     }

     RelDateTimeFmtDataSink &outer;
   } relativeTimeSink;

   /*
    * Handles "relative" entries, e.g., under "day", "day-short", "fri",
    * "fri-narrow", "fri-short", etc.
   */
   struct RelativeSink : public ResourceTableSink {
         RelativeSink(RelDateTimeFmtDataSink &sink) : outer(sink) {}
         ~RelativeSink();

     virtual void put(const char *key, const ResourceValue &value, UErrorCode &errorCode) {
       int relUnitIndex = genericToRelUnit(outer.genericUnit);
       if (relUnitIndex == UDAT_RELATIVE_SECONDS && uprv_strcmp(key, "0") == 0) {
         // Handle "NOW"
         outer.outputData.absoluteUnits[outer.style][UDAT_ABSOLUTE_NOW]
             [UDAT_DIRECTION_PLAIN].fastCopyFrom(value.getUnicodeString(errorCode));
       }

       int32_t direction = keyToDirection(key);
       if (direction == -1) {
         return;
       }
       int absUnitIndex = genericToAbsUnit(outer.genericUnit);
       if (absUnitIndex == -1) {
         return;
       }
       // TODO: Make sure the check for empty is correct.
       if (outer.outputData.absoluteUnits[outer.style][absUnitIndex]
            [direction].isEmpty()) {
         outer.outputData.absoluteUnits[outer.style][absUnitIndex]
             [direction].fastCopyFrom(value.getUnicodeString(errorCode));
       }
     }

     RelDateTimeFmtDataSink &outer;
   } relativeSink;

   /*
    * Handles entries under units, recognizing "relative" and "relativeTime" entries.
    */
   struct UnitSink : public ResourceTableSink {
         UnitSink(RelDateTimeFmtDataSink &sink) : outer(sink) {}
         ~UnitSink();

     virtual void put(const char *key, const ResourceValue &value, UErrorCode &errorCode) {
       if (uprv_strcmp(key, "dn") != 0) {
         return;
       }

       // Handle Display Name for PLAIN direction for some units.
       int absUnit = genericToAbsUnit(outer.genericUnit);
       if (absUnit == -1) {
         return;  // Not interesting.
       }

       const UnicodeString unitName = value.getUnicodeString(errorCode);

       // TODO(Travis Keep): This is a hack to get around CLDR bug 6818.
       UnicodeString displayName = value.getUnicodeString(errorCode);
       if (U_SUCCESS(errorCode)) {
         if (uprv_strcmp("en", outer.sinkLocaleId) == 0) {
           displayName.toLower();
         }
       }
       // end hack

       // Store displayname.
       outer.outputData.absoluteUnits[outer.style]
           [absUnit][UDAT_DIRECTION_PLAIN].fastCopyFrom(displayName);
       return;
     }

     virtual ResourceTableSink *getOrCreateTableSink(
         const char *key, int32_t initialSize, UErrorCode &errorCode) {
       if (uprv_strcmp(key, "relative") == 0) {
         return &outer.relativeSink;
       } else if (uprv_strcmp(key, "relativeTime") == 0) {
         return &outer.relativeTimeSink;
       }
       return NULL;
     }

     RelDateTimeFmtDataSink &outer;
   } unitSink;

   // For hack for locale "en".
   const char* sinkLocaleId;

   // Values kept between levels of parsing the CLDR data.
   // LOCAL DATA
   int32_t pastFutureIndex;  // 0 == past or 1 ==  future
   UDateRelativeDateTimeFormatterStyle style;  // {LONG, SHORT, NARROW }}
   RelAbsUnit genericUnit;
   int32_t relUnitIndex;
   int32_t absUnitIndex;
   int32_t width; // Ordinal for style
   int32_t unitIndex;

   RelativeDateTimeCacheData &outputData;

   // Constructor
   RelDateTimeFmtDataSink(RelativeDateTimeCacheData& cacheData, const char* localeId)
     : relDateTimeDetailSink(*this), relativeTimeSink(*this), relativeSink(*this),
       unitSink(*this), sinkLocaleId(localeId), outputData(cacheData) {
     // Clear cacheData.fallBackCache
     cacheData.fallBackCache[UDAT_STYLE_LONG] = -1;
     cacheData.fallBackCache[UDAT_STYLE_SHORT] = -1;
     cacheData.fallBackCache[UDAT_STYLE_NARROW] = -1;
 }

   ~RelDateTimeFmtDataSink();

   // Utility functions
   static UDateRelativeDateTimeFormatterStyle styleFromString(const char *s) {
     int len = uprv_strlen(s);
     if (uprv_strcmp(s + len - 7, "-narrow") == 0) {
       return UDAT_STYLE_NARROW;
     }
     if (uprv_strcmp(s + len - 6, "-short") == 0) {
       return UDAT_STYLE_SHORT;
     }
     return UDAT_STYLE_LONG;
   }

   static int styleSuffixLength(UDateRelativeDateTimeFormatterStyle style) {
     switch (style) {
       case UDAT_STYLE_NARROW:
         return 7;
       case UDAT_STYLE_SHORT:
         return 6;
       default:
         return 0;
     }
   }

   // Utility functions
   static UDateRelativeDateTimeFormatterStyle styleFromAliasUnicodeString(UnicodeString s) {
     static const UnicodeString narrow = UnicodeString("-narrow");
     static const UnicodeString sshort = UnicodeString("-short");
     if (s.endsWith(narrow)) {
       return UDAT_STYLE_NARROW;
     }
     if (s.endsWith(sshort)) {
       return UDAT_STYLE_SHORT;
     }
     return UDAT_STYLE_LONG;
   }

   static RelAbsUnit unitOrNullFromString(const char* keyword, int32_t length) {
     // Quick check from string to enum.
     switch (length) {
       case 3:
         if (uprv_strncmp(keyword, "day", length) == 0) {
           return DAY;
         } else if (uprv_strncmp(keyword, "sun", length) == 0) {
           return SUNDAY;
         } else if (uprv_strncmp(keyword, "mon", length) == 0) {
           return MONDAY;
         } else if (uprv_strncmp(keyword, "tue", length) == 0) {
           return TUESDAY;
         } else if (uprv_strncmp(keyword, "wed", length) == 0) {
           return WEDNESDAY;
         } else if (uprv_strncmp(keyword, "thu", length) == 0) {
           return THURSDAY;
         } else if (uprv_strncmp(keyword, "fri", length) == 0) {
           return FRIDAY;
         } else if (uprv_strncmp(keyword, "sat", length) == 0) {
           return SATURDAY;
         }
         break;
       case 4:
         if (uprv_strncmp(keyword, "hour", length) == 0) {
           return HOUR;
         } else if (uprv_strncmp(keyword, "week", length) == 0) {
           return WEEK;
         } else if (uprv_strncmp(keyword, "year", length) == 0) {
           return YEAR;
         }
         break;
       case 5:
         if (uprv_strncmp(keyword, "month", length) == 0) {
           return MONTH;
         }
         break;
       case 6:
         if (uprv_strncmp(keyword, "minute", length) == 0) {
           return MINUTE;
         } else if (uprv_strncmp(keyword, "second", length) == 0) {
           return SECOND;
         }
         break;
       case 7:
         if (uprv_strncmp(keyword, "quarter", length) == 0) {
           return QUARTER;  // TODO: Check @provisional
         }
         break;
       default:
         break;
     }
     return INVALID_UNIT;
   }

   // Member functions of top level sink.
   virtual void put(const char *key, const ResourceValue &value, UErrorCode &errorCode) {
     // Only handle aliases, storing information about alias fallback.

     if (U_SUCCESS(errorCode)) {
       if (value.getType() != URES_ALIAS) {
         return;
       }
       const UnicodeString valueStr = value.getAliasUnicodeString(errorCode);
       if (U_SUCCESS(errorCode)) {
         UDateRelativeDateTimeFormatterStyle sourceStyle= styleFromString(key);
         UDateRelativeDateTimeFormatterStyle targetStyle = styleFromAliasUnicodeString(valueStr);

         if (sourceStyle == targetStyle) {
           // TODO: Report an circularity error in the alias.
           return;
         }
         if (outputData.fallBackCache[sourceStyle] != -1 &&
             outputData.fallBackCache[sourceStyle] != targetStyle) {
           // TODO: Report an reset error in the alias
           return;
         }
         outputData.fallBackCache[sourceStyle] = targetStyle;
       }
     }
     return;
   }

   // Top level sink
   virtual ResourceTableSink *getOrCreateTableSink(
       const char *key, int32_t /* initialSize */, UErrorCode &errorCode) {
     style= styleFromString(key);
     int unitSize = uprv_strlen(key) - styleSuffixLength(style);
     genericUnit = unitOrNullFromString(key, unitSize);
     if (genericUnit == INVALID_UNIT) {
       return NULL;
     }
     return &unitSink;
   }

 };

 // Virtual descructors must be defined out of line.
 RelDateTimeFmtDataSink::RelDateTimeDetailSink::~RelDateTimeDetailSink() {};
 RelDateTimeFmtDataSink::RelativeTimeSink::~RelativeTimeSink() {};
 RelDateTimeFmtDataSink::RelativeSink::~RelativeSink() {};
 RelDateTimeFmtDataSink::UnitSink::~UnitSink() {};
 RelDateTimeFmtDataSink::~RelDateTimeFmtDataSink() {};

 } // namespace

 static UBool loadUnitData(
         const UResourceBundle *resource,
         RelativeDateTimeCacheData &cacheData,
         const char* localeId,
         UErrorCode &status) {

     // Initialize the cache arrays
     for (int style = 0; style < UDAT_STYLE_COUNT; ++style) {
       for (int relUnit = 0; relUnit < UDAT_RELATIVE_UNIT_COUNT; ++relUnit) {
         for (int pl = 0; pl < StandardPlural::COUNT; ++pl) {
           cacheData.relativeUnitsFormatter[style][relUnit][0][pl] = NULL;
           cacheData.relativeUnitsFormatter[style][relUnit][1][pl] = NULL;
         }
       }
     }

     RelDateTimeFmtDataSink sink(cacheData, localeId);
     ures_getAllTableItemsWithFallback(resource, "fields", sink, status);

     return U_SUCCESS(status);
 }

 static UBool getDateTimePattern(
         const UResourceBundle *resource,
         UnicodeString &result,
         UErrorCode &status) {
     UnicodeString defaultCalendarName;
     if (!getStringWithFallback(
             resource,
             "calendar/default",
             defaultCalendarName,
             status)) {
         return FALSE;
     }
     CharString pathBuffer;
     pathBuffer.append("calendar/", status)
             .appendInvariantChars(defaultCalendarName, status)
             .append("/DateTimePatterns", status);
     LocalUResourceBundlePointer topLevel(
             ures_getByKeyWithFallback(
                     resource, pathBuffer.data(), NULL, &status));
     if (U_FAILURE(status)) {
         return FALSE;
     }
     int32_t size = ures_getSize(topLevel.getAlias());
     if (size <= 8) {
         // Oops, size is to small to access the index that we want, fallback
         // to a hard-coded value.
         result = UNICODE_STRING_SIMPLE("{1} {0}");
         return TRUE;
     }
     return getStringByIndex(topLevel.getAlias(), 8, result, status);
 }

 template<> U_I18N_API
 const RelativeDateTimeCacheData *LocaleCacheKey<RelativeDateTimeCacheData>::createObject(const void * /*unused*/, UErrorCode &status) const {
     const char *localeId = fLoc.getName();
     LocalUResourceBundlePointer topLevel(ures_open(NULL, localeId, &status));
     if (U_FAILURE(status)) {
         return NULL;
     }
     LocalPointer<RelativeDateTimeCacheData> result(
             new RelativeDateTimeCacheData());
     if (result.isNull()) {
         status = U_MEMORY_ALLOCATION_ERROR;
         return NULL;
     }
     if (!loadUnitData(
             topLevel.getAlias(),
             *result,
             localeId,
             status)) {
         return NULL;
     }
     UnicodeString dateTimePattern;
     if (!getDateTimePattern(topLevel.getAlias(), dateTimePattern, status)) {
         return NULL;
     }
     result->adoptCombinedDateAndTime(
             new MessageFormat(dateTimePattern, localeId, status));
     if (U_FAILURE(status)) {
         return NULL;
     }
     result->addRef();
     return result.orphan();
 }

 RelativeDateTimeFormatter::RelativeDateTimeFormatter(UErrorCode& status) :
         fCache(NULL),
         fNumberFormat(NULL),
         fPluralRules(NULL),
         fStyle(UDAT_STYLE_LONG),
         fContext(UDISPCTX_CAPITALIZATION_NONE),
         fOptBreakIterator(NULL) {
     init(NULL, NULL, status);
 }

 RelativeDateTimeFormatter::RelativeDateTimeFormatter(
         const Locale& locale, UErrorCode& status) :
         fCache(NULL),
         fNumberFormat(NULL),
         fPluralRules(NULL),
         fStyle(UDAT_STYLE_LONG),
         fContext(UDISPCTX_CAPITALIZATION_NONE),
         fOptBreakIterator(NULL),
         fLocale(locale) {
     init(NULL, NULL, status);
 }

 RelativeDateTimeFormatter::RelativeDateTimeFormatter(
         const Locale& locale, NumberFormat *nfToAdopt, UErrorCode& status) :
         fCache(NULL),
         fNumberFormat(NULL),
         fPluralRules(NULL),
         fStyle(UDAT_STYLE_LONG),
         fContext(UDISPCTX_CAPITALIZATION_NONE),
         fOptBreakIterator(NULL),
         fLocale(locale) {
     init(nfToAdopt, NULL, status);
 }

 RelativeDateTimeFormatter::RelativeDateTimeFormatter(
         const Locale& locale,
         NumberFormat *nfToAdopt,
         UDateRelativeDateTimeFormatterStyle styl,
         UDisplayContext capitalizationContext,
         UErrorCode& status) :
         fCache(NULL),
         fNumberFormat(NULL),
         fPluralRules(NULL),
         fStyle(styl),
         fContext(capitalizationContext),
         fOptBreakIterator(NULL),
         fLocale(locale) {
     if (U_FAILURE(status)) {
         return;
     }
     if ((capitalizationContext >> 8) != UDISPCTX_TYPE_CAPITALIZATION) {
         status = U_ILLEGAL_ARGUMENT_ERROR;
         return;
     }
     if (capitalizationContext == UDISPCTX_CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE) {
         BreakIterator *bi = BreakIterator::createSentenceInstance(locale, status);
         if (U_FAILURE(status)) {
             return;
         }
         init(nfToAdopt, bi, status);
     } else {
         init(nfToAdopt, NULL, status);
     }
 }

 RelativeDateTimeFormatter::RelativeDateTimeFormatter(
         const RelativeDateTimeFormatter& other)
         : UObject(other),
           fCache(other.fCache),
           fNumberFormat(other.fNumberFormat),
           fPluralRules(other.fPluralRules),
           fStyle(other.fStyle),
           fContext(other.fContext),
           fOptBreakIterator(other.fOptBreakIterator),
           fLocale(other.fLocale) {
     fCache->addRef();
     fNumberFormat->addRef();
     fPluralRules->addRef();
     if (fOptBreakIterator != NULL) {
       fOptBreakIterator->addRef();
     }
 }

 RelativeDateTimeFormatter& RelativeDateTimeFormatter::operator=(
         const RelativeDateTimeFormatter& other) {
     if (this != &other) {
         SharedObject::copyPtr(other.fCache, fCache);
         SharedObject::copyPtr(other.fNumberFormat, fNumberFormat);
         SharedObject::copyPtr(other.fPluralRules, fPluralRules);
         SharedObject::copyPtr(other.fOptBreakIterator, fOptBreakIterator);
         fStyle = other.fStyle;
         fContext = other.fContext;
         fLocale = other.fLocale;
     }
     return *this;
 }

 RelativeDateTimeFormatter::~RelativeDateTimeFormatter() {
     if (fCache != NULL) {
         fCache->removeRef();
     }
     if (fNumberFormat != NULL) {
         fNumberFormat->removeRef();
     }
     if (fPluralRules != NULL) {
         fPluralRules->removeRef();
     }
     if (fOptBreakIterator != NULL) {
         fOptBreakIterator->removeRef();
     }
 }

 const NumberFormat& RelativeDateTimeFormatter::getNumberFormat() const {
     return **fNumberFormat;
 }

 UDisplayContext RelativeDateTimeFormatter::getCapitalizationContext() const {
     return fContext;
 }

 UDateRelativeDateTimeFormatterStyle RelativeDateTimeFormatter::getFormatStyle() const {
     return fStyle;
 }

DateFormatSymbols::DtWidthType styleToDateFormatSymbolWidth[UDAT_STYLE_COUNT] = {
  DateFormatSymbols::WIDE, DateFormatSymbols::SHORT, DateFormatSymbols::NARROW
};

UnicodeString& RelativeDateTimeFormatter::format(
         double quantity, UDateDirection direction, UDateRelativeUnit unit,
         UnicodeString& appendTo, UErrorCode& status) const {
     if (U_FAILURE(status)) {
         return appendTo;
     }
     if (direction != UDAT_DIRECTION_LAST && direction != UDAT_DIRECTION_NEXT) {
         status = U_ILLEGAL_ARGUMENT_ERROR;
         return appendTo;
     }
     int32_t bFuture = direction == UDAT_DIRECTION_NEXT ? 1 : 0;
     FieldPosition pos(FieldPosition::DONT_CARE);

     UnicodeString result;
     UnicodeString formattedNumber;

     StandardPlural::Form pluralIndex = QuantityFormatter::selectPlural(
         quantity, **fNumberFormat, **fPluralRules, formattedNumber, pos,
         status);

     const SimpleFormatter* formatter =
         fCache->getRelativeUnitFormatter(fStyle, unit, bFuture, pluralIndex);
     if (formatter == NULL) {
       // TODO: WARN
     }
     UnicodeString emptyString("");
     result = formatter->format(formattedNumber, emptyString, status);
    adjustForContext(result);
    return appendTo.append(result);
}

UnicodeString& RelativeDateTimeFormatter::formatNumeric(
        double offset, URelativeDateTimeUnit unit,
        UnicodeString& appendTo, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    // For a quick bringup just call the above method; this leaves some
    // holes (e.g. for handling quarter). Need to redo the data structure
    // to support all necessary data items, and to be indexed by the
    // newer enum.
    UDateRelativeUnit relunit = UDAT_RELATIVE_UNIT_COUNT;
    switch (unit) {
        case UDAT_REL_UNIT_YEAR:    relunit = UDAT_RELATIVE_YEARS; break;
        case UDAT_REL_UNIT_MONTH:   relunit = UDAT_RELATIVE_MONTHS; break;
        case UDAT_REL_UNIT_WEEK:    relunit = UDAT_RELATIVE_WEEKS; break;
        case UDAT_REL_UNIT_DAY:     relunit = UDAT_RELATIVE_DAYS; break;
        case UDAT_REL_UNIT_HOUR:    relunit = UDAT_RELATIVE_HOURS; break;
        case UDAT_REL_UNIT_MINUTE:  relunit = UDAT_RELATIVE_MINUTES; break;
        case UDAT_REL_UNIT_SECOND:  relunit = UDAT_RELATIVE_SECONDS; break;
        default: // a unit that the above method does not handle
            status = U_MISSING_RESOURCE_ERROR;
            return appendTo;
    }
    UDateDirection direction = UDAT_DIRECTION_NEXT;
    if (offset < 0) {
        direction = UDAT_DIRECTION_LAST;
        offset = -offset;
    }
    return format(offset, direction, relunit, appendTo, status);
}

UnicodeString& RelativeDateTimeFormatter::format(
        UDateDirection direction, UDateAbsoluteUnit unit,
        UnicodeString& appendTo, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (unit == UDAT_ABSOLUTE_NOW && direction != UDAT_DIRECTION_PLAIN) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    // Get string using fallback.
    UnicodeString result;
     if ((direction == UDAT_DIRECTION_PLAIN) &&
         (unit >= UDAT_ABSOLUTE_SUNDAY && unit <= UDAT_ABSOLUTE_SATURDAY)) {
       // Get play weekday names from DateFormatSymbols.
       int dateSymbolIndex = (unit - UDAT_ABSOLUTE_SUNDAY) + UCAL_SUNDAY;
       DateFormatSymbols::DtWidthType dtfmtWidth = styleToDateFormatSymbolWidth[fStyle];
       int32_t count;
       DateFormatSymbols* dfSym = new DateFormatSymbols(status);
       const UnicodeString* weekdayNames =
           dfSym->getWeekdays(count,
                              DateFormatSymbols::DtContextType::STANDALONE,
                              dtfmtWidth);
       if (dateSymbolIndex < count) {
         result.fastCopyFrom(weekdayNames[dateSymbolIndex]);
       }
     }
    else {
      result.fastCopyFrom(fCache->getAbsoluteUnitString(fStyle, unit, direction));
    }
    if (fOptBreakIterator) {
      adjustForContext(result);
    }
    return appendTo.append(result);
}

UnicodeString& RelativeDateTimeFormatter::format(
        double offset, URelativeDateTimeUnit unit,
        UnicodeString& appendTo, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    // For a quick bringup just use the existing data structure; this leaves
    // some holes (e.g. for handling quarter). Need to redo the data structure
    // to support all necessary data items, and to be indexed by the
    // newer enum.
    UDateDirection direction = UDAT_DIRECTION_COUNT;
    int32_t intoffset = (offset < 0)? (int32_t)(offset-0.5) : (int32_t)(offset+0.5);
    switch (intoffset) {
        case -2: direction = UDAT_DIRECTION_LAST_2; break;
        case -1: direction = UDAT_DIRECTION_LAST; break;
        case  0: direction = UDAT_DIRECTION_THIS; break;
        case  1: direction = UDAT_DIRECTION_NEXT; break;
        case  2: direction = UDAT_DIRECTION_NEXT_2; break;
        default: break;
    }
    UDateAbsoluteUnit absunit = UDAT_ABSOLUTE_UNIT_COUNT;
    switch (unit) {
        case UDAT_REL_UNIT_YEAR:    absunit = UDAT_ABSOLUTE_YEAR; break;
        case UDAT_REL_UNIT_MONTH:   absunit = UDAT_ABSOLUTE_MONTH; break;
        case UDAT_REL_UNIT_WEEK:    absunit = UDAT_ABSOLUTE_WEEK; break;
        case UDAT_REL_UNIT_DAY:     absunit = UDAT_ABSOLUTE_DAY; break;
        case UDAT_REL_UNIT_MINUTE:
        case UDAT_REL_UNIT_SECOND:
            if (direction == UDAT_DIRECTION_THIS) {
                absunit = UDAT_ABSOLUTE_NOW;
                direction = UDAT_DIRECTION_PLAIN;
            }
            break;
        case UDAT_REL_UNIT_SUNDAY:  absunit = UDAT_ABSOLUTE_SUNDAY; break;
        case UDAT_REL_UNIT_MONDAY:  absunit = UDAT_ABSOLUTE_MONDAY; break;
        case UDAT_REL_UNIT_TUESDAY:  absunit = UDAT_ABSOLUTE_TUESDAY; break;
        case UDAT_REL_UNIT_WEDNESDAY:  absunit = UDAT_ABSOLUTE_WEDNESDAY; break;
        case UDAT_REL_UNIT_THURSDAY:  absunit = UDAT_ABSOLUTE_THURSDAY; break;
        case UDAT_REL_UNIT_FRIDAY:  absunit = UDAT_ABSOLUTE_FRIDAY; break;
        case UDAT_REL_UNIT_SATURDAY:  absunit = UDAT_ABSOLUTE_SATURDAY; break;
        default: break;
    }
    if (direction != UDAT_DIRECTION_COUNT && absunit != UDAT_ABSOLUTE_UNIT_COUNT) {
      UnicodeString unitFormatString =
          fCache->getAbsoluteUnitString(fStyle, absunit, direction);

        UnicodeString result(unitFormatString);
        if (result.length() > 0) {
            if (fOptBreakIterator != NULL) {
                adjustForContext(result);
            }
            return appendTo.append(result);
        }
    }
    // otherwise fallback to formatNumeric
    return formatNumeric(offset, unit, appendTo, status);
}

UnicodeString& RelativeDateTimeFormatter::combineDateAndTime(
    const UnicodeString& relativeDateString, const UnicodeString& timeString,
    UnicodeString& appendTo, UErrorCode& status) const {
    Formattable args[2] = {timeString, relativeDateString};
    FieldPosition fpos(0);
    return fCache->getCombinedDateAndTime()->format(
            args, 2, appendTo, fpos, status);
}

void RelativeDateTimeFormatter::adjustForContext(UnicodeString &str) const {
    if (fOptBreakIterator == NULL
        || str.length() == 0 || !u_islower(str.char32At(0))) {
        return;
    }

    // Must guarantee that one thread at a time accesses the shared break
    // iterator.
    Mutex lock(&gBrkIterMutex);
    str.toTitle(
            fOptBreakIterator->get(),
            fLocale,
            U_TITLECASE_NO_LOWERCASE | U_TITLECASE_NO_BREAK_ADJUSTMENT);
}

void RelativeDateTimeFormatter::init(
        NumberFormat *nfToAdopt,
        BreakIterator *biToAdopt,
        UErrorCode &status) {
    LocalPointer<NumberFormat> nf(nfToAdopt);
    LocalPointer<BreakIterator> bi(biToAdopt);
    UnifiedCache::getByLocale(fLocale, fCache, status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedPluralRules *pr = PluralRules::createSharedInstance(
            fLocale, UPLURAL_TYPE_CARDINAL, status);
    if (U_FAILURE(status)) {
        return;
    }
    SharedObject::copyPtr(pr, fPluralRules);
    pr->removeRef();
    if (nf.isNull()) {
       const SharedNumberFormat *shared = NumberFormat::createSharedInstance(
               fLocale, UNUM_DECIMAL, status);
        if (U_FAILURE(status)) {
            return;
        }
        SharedObject::copyPtr(shared, fNumberFormat);
        shared->removeRef();
    } else {
        SharedNumberFormat *shared = new SharedNumberFormat(nf.getAlias());
        if (shared == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        nf.orphan();
        SharedObject::copyPtr(shared, fNumberFormat);
    }
    if (bi.isNull()) {
        SharedObject::clearPtr(fOptBreakIterator);
    } else {
        SharedBreakIterator *shared = new SharedBreakIterator(bi.getAlias());
        if (shared == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        bi.orphan();
        SharedObject::copyPtr(shared, fOptBreakIterator);
    }
}

U_NAMESPACE_END

// Plain C API

U_NAMESPACE_USE

U_CAPI URelativeDateTimeFormatter* U_EXPORT2
ureldatefmt_open( const char*          locale,
                  UNumberFormat*       nfToAdopt,
                  UDateRelativeDateTimeFormatterStyle width,
                  UDisplayContext      capitalizationContext,
                  UErrorCode*          status )
{
    if (U_FAILURE(*status)) {
        return NULL;
    }
    LocalPointer<RelativeDateTimeFormatter> formatter(new RelativeDateTimeFormatter(Locale(locale),
                                                              (NumberFormat*)nfToAdopt, width,
                                                              capitalizationContext, *status));
    if (U_FAILURE(*status)) {
        return NULL;
    }
    return (URelativeDateTimeFormatter*)formatter.orphan();
}

U_CAPI void U_EXPORT2
ureldatefmt_close(URelativeDateTimeFormatter *reldatefmt)
{
    delete (RelativeDateTimeFormatter*)reldatefmt;
}

U_CAPI int32_t U_EXPORT2
ureldatefmt_formatNumeric( const URelativeDateTimeFormatter* reldatefmt,
                    double                offset,
                    URelativeDateTimeUnit unit,
                    UChar*                result,
                    int32_t               resultCapacity,
                    UErrorCode*           status)
{
    if (U_FAILURE(*status)) {
        return 0;
    }
    if (result == NULL ? resultCapacity != 0 : resultCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    UnicodeString res;
    if (result != NULL) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer (copied from udat_format)
        res.setTo(result, 0, resultCapacity);
    }
    ((RelativeDateTimeFormatter*)reldatefmt)->formatNumeric(offset, unit, res, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    return res.extract(result, resultCapacity, *status);
}

U_CAPI int32_t U_EXPORT2
ureldatefmt_format( const URelativeDateTimeFormatter* reldatefmt,
                    double                offset,
                    URelativeDateTimeUnit unit,
                    UChar*                result,
                    int32_t               resultCapacity,
                    UErrorCode*           status)
{
    if (U_FAILURE(*status)) {
        return 0;
    }
    if (result == NULL ? resultCapacity != 0 : resultCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    UnicodeString res;
    if (result != NULL) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer (copied from udat_format)
        res.setTo(result, 0, resultCapacity);
    }
    ((RelativeDateTimeFormatter*)reldatefmt)->format(offset, unit, res, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    return res.extract(result, resultCapacity, *status);
}

U_CAPI int32_t U_EXPORT2
ureldatefmt_combineDateAndTime( const URelativeDateTimeFormatter* reldatefmt,
                    const UChar *     relativeDateString,
                    int32_t           relativeDateStringLen,
                    const UChar *     timeString,
                    int32_t           timeStringLen,
                    UChar*            result,
                    int32_t           resultCapacity,
                    UErrorCode*       status )
{
    if (U_FAILURE(*status)) {
        return 0;
    }
    if (result == NULL ? resultCapacity != 0 : resultCapacity < 0 ||
            relativeDateString == NULL || relativeDateStringLen < 1 ||
            timeString == NULL || timeStringLen < -1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    UnicodeString relDateStr((UBool)(relativeDateStringLen == -1), relativeDateString, relativeDateStringLen);
    UnicodeString timeStr((UBool)(timeStringLen == -1), timeString, timeStringLen);
    UnicodeString res;
    if (result != NULL) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer (copied from udat_format)
        res.setTo(result, 0, resultCapacity);
    }
    ((RelativeDateTimeFormatter*)reldatefmt)->combineDateAndTime(relDateStr, timeStr, res, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    return res.extract(result, resultCapacity, *status);
}

#endif /* !UCONFIG_NO_FORMATTING */
