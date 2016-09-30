// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
* Copyright (C) 2003 - 2013, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include "ethpccal.h"
#include "cecal.h"
#include <float.h>
#include "gregoimp.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(EthiopicCalendar)

//static const int32_t JD_EPOCH_OFFSET_AMETE_ALEM = -285019;
static const int32_t JD_EPOCH_OFFSET_AMETE_MIHRET = 1723856;
static const int32_t AMETE_MIHRET_DELTA = 5500; // 5501 - 1 (Amete Alem 5501 = Amete Mihret 1)

//-------------------------------------------------------------------------
// Constructors...
//-------------------------------------------------------------------------

EthiopicCalendar::EthiopicCalendar(const Locale& aLocale,
                                   UErrorCode& success,
                                   EEraType type /*= AMETE_MIHRET_ERA*/,
                                   ETimeType ttype /*= E_WESTERN_TIME */)
:   CECalendar(aLocale, success),
    eraType(type),
    timeType(ttype)
{
  //  puts(isEthiopianTime()?"ETHTIME":"WTIME");
}

EthiopicCalendar::EthiopicCalendar(const EthiopicCalendar& other)
:   CECalendar(other),
    eraType(other.eraType),
    timeType(other.timeType)
{
  //  puts(isEthiopianTime()?"ETHTIME":"WTIME");
}

EthiopicCalendar::~EthiopicCalendar()
{
}

Calendar*
EthiopicCalendar::clone() const
{
    return new EthiopicCalendar(*this);
}

const char *
EthiopicCalendar::getType() const
{
    if (isAmeteAlemEra()) {
        return "ethiopic-amete-alem";
    }
    return "ethiopic";
}

void
EthiopicCalendar::setAmeteAlemEra(UBool onOff)
{
    eraType = onOff ? AMETE_ALEM_ERA : AMETE_MIHRET_ERA;
}
    
UBool
EthiopicCalendar::isAmeteAlemEra() const
{
    return (eraType == AMETE_ALEM_ERA);
}

//-------------------------------------------------------------------------
// Calendar framework
//-------------------------------------------------------------------------

int32_t
EthiopicCalendar::handleGetExtendedYear()
{
    // Ethiopic calendar uses EXTENDED_YEAR aligned to
    // Amelete Hihret year always.
    int32_t eyear;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        eyear = internalGet(UCAL_EXTENDED_YEAR, 1); // Default to year 1
    } else if (isAmeteAlemEra()) {
        eyear = internalGet(UCAL_YEAR, 1 + AMETE_MIHRET_DELTA)
            - AMETE_MIHRET_DELTA; // Default to year 1 of Amelete Mihret
    } else {
        // The year defaults to the epoch start, the era to AMETE_MIHRET
        int32_t era = internalGet(UCAL_ERA, AMETE_MIHRET);
        if (era == AMETE_MIHRET) {
            eyear = internalGet(UCAL_YEAR, 1); // Default to year 1
        } else {
            eyear = internalGet(UCAL_YEAR, 1) - AMETE_MIHRET_DELTA;
        }
    }
    return eyear;
}

void
EthiopicCalendar::handleComputeFields(int32_t julianDay, UErrorCode &/*status*/)
{
    int32_t eyear, month, day, era, year;
    jdToCE(julianDay, getJDEpochOffset(), eyear, month, day);

    if (isAmeteAlemEra()) {
        era = AMETE_ALEM;
        year = eyear + AMETE_MIHRET_DELTA;
    } else {
        if (eyear > 0) {
            era = AMETE_MIHRET;
            year = eyear;
        } else {
            era = AMETE_ALEM;
            year = eyear + AMETE_MIHRET_DELTA;
        }
    }

    internalSet(UCAL_EXTENDED_YEAR, eyear);
    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DATE, day);
    internalSet(UCAL_DAY_OF_YEAR, (30 * month) + day);
}

int32_t
EthiopicCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    if (isAmeteAlemEra() && field == UCAL_ERA) {
        return 0; // Only one era in this mode, era is always 0
    }
    return CECalendar::handleGetLimit(field, limitType);
}

/**
 * The system maintains a static default century start date and Year.  They are
 * initialized the first time they are used.  Once the system default century date 
 * and year are set, they do not change.
 */
static UDate           gSystemDefaultCenturyStart       = DBL_MIN;
static int32_t         gSystemDefaultCenturyStartYear   = -1;
static icu::UInitOnce  gSystemDefaultCenturyInit        = U_INITONCE_INITIALIZER;

static void U_CALLCONV initializeSystemDefaultCentury()
{
    UErrorCode status = U_ZERO_ERROR;
    EthiopicCalendar calendar(Locale("@calendar=ethiopic"), status);
    if (U_SUCCESS(status)) {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);

        gSystemDefaultCenturyStart = calendar.getTime(status);
        gSystemDefaultCenturyStartYear = calendar.get(UCAL_YEAR, status);
    }
    // We have no recourse upon failure unless we want to propagate the failure
    // out.
}

UDate
EthiopicCalendar::defaultCenturyStart() const
{
    // lazy-evaluate systemDefaultCenturyStart
    umtx_initOnce(gSystemDefaultCenturyInit, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStart;
}

int32_t
EthiopicCalendar::defaultCenturyStartYear() const
{
    // lazy-evaluate systemDefaultCenturyStartYear
    umtx_initOnce(gSystemDefaultCenturyInit, &initializeSystemDefaultCentury);
    if (isAmeteAlemEra()) {
        return gSystemDefaultCenturyStartYear + AMETE_MIHRET_DELTA;
    }
    return gSystemDefaultCenturyStartYear;
}


int32_t
EthiopicCalendar::getJDEpochOffset() const
{
    return JD_EPOCH_OFFSET_AMETE_MIHRET;
}

#include <stdio.h>

// TIME STUFF
void
EthiopicCalendar::computeFields(UErrorCode &ec)
{
  if (U_FAILURE(ec)) {
    //    puts("s0");
    return;
  } else if (timeType == E_WESTERN_TIME) {
    //    puts("s1");
    return CECalendar::computeFields(ec);
  } else {
    //    puts("s2");
  }
  // Else: E-Time mode.
  
  // YES. Copy most of Calendar::computeFields.. refactor maybe??

  // Compute local wall millis
  double localMillis = internalGetTime();
  int32_t rawOffset, dstOffset;
  getTimeZone().getOffset(localMillis, FALSE, rawOffset, dstOffset, ec);
  localMillis += (rawOffset + dstOffset); 
  
  // Mark fields as set.  Do this before calling handleComputeFields().
  uint32_t mask =   //fInternalSetMask;
    (1 << UCAL_ERA) |
    (1 << UCAL_YEAR) |
    (1 << UCAL_MONTH) |
    (1 << UCAL_DAY_OF_MONTH) | // = UCAL_DATE
    (1 << UCAL_DAY_OF_YEAR) |
    (1 << UCAL_EXTENDED_YEAR);  
  
  for (int32_t i=0; i<UCAL_FIELD_COUNT; ++i) {
    if ((mask & 1) == 0) {
      fStamp[i] = kInternallySet;
      fIsSet[i] = TRUE; // Remove later
    } else {
      fStamp[i] = kUnset;
      fIsSet[i] = FALSE; // Remove later
    }
    mask >>= 1;
  }
  
  // We used to check for and correct extreme millis values (near
  // Long.MIN_VALUE or Long.MAX_VALUE) here.  Such values would cause
  // overflows from positive to negative (or vice versa) and had to
  // be manually tweaked.  We no longer need to do this because we
  // have limited the range of supported dates to those that have a
  // Julian day that fits into an int.  This allows us to implement a
  // JULIAN_DAY field and also removes some inelegant code. - Liu
  // 11/6/00
  
  int32_t days =  (int32_t)ClockMath::floorDivide(
                                                  localMillis
                                                  - (kOneDay/4), // offset of 6 hours for Ethiopian time
                                                  (double)kOneDay);
  
  internalSet(UCAL_JULIAN_DAY,days + kEpochStartAsJulianDay);
  
#if defined (U_DEBUG_CAL)
  //fprintf(stderr, "%s:%d- Hmm! Jules @ %d, as per %.0lf millis\n",
  //__FILE__, __LINE__, fFields[UCAL_JULIAN_DAY], localMillis);
#endif  
  
  computeGregorianAndDOWFields(fFields[UCAL_JULIAN_DAY], ec);
  
  // Call framework method to have subclass compute its fields.
  // These must include, at a minimum, MONTH, DAY_OF_MONTH,
  // EXTENDED_YEAR, YEAR, DAY_OF_YEAR.  This method will call internalSet(),
  // which will update stamp[].
  handleComputeFields(fFields[UCAL_JULIAN_DAY], ec);
  
  // Compute week-related fields, based on the subclass-computed
  // fields computed by handleComputeFields().
  computeWeekFields(ec);
  
  // Compute time-related fields.  These are indepent of the date and
  // of the subclass algorithm.  They depend only on the local zone
  // wall milliseconds in day.
  int32_t millisInDay =  (int32_t) (localMillis
                                    - (days * kOneDay)
                                    /*); */  - (kOneDay/4)); // offset of 6 hours for Ethiopian time
  //printf("MID=%d\n", millisInDay);
  fFields[UCAL_MILLISECONDS_IN_DAY] = millisInDay;
  fFields[UCAL_MILLISECOND] = millisInDay % 1000;
  millisInDay /= 1000;
  fFields[UCAL_SECOND] = millisInDay % 60;
  millisInDay /= 60;
  fFields[UCAL_MINUTE] = millisInDay % 60;
  millisInDay /= 60;
  fFields[UCAL_HOUR_OF_DAY] = millisInDay;
  fFields[UCAL_AM_PM] = millisInDay / 12; // Assume AM == 0
  fFields[UCAL_HOUR] = millisInDay % 12;
  fFields[UCAL_ZONE_OFFSET] = rawOffset;
  fFields[UCAL_DST_OFFSET] = dstOffset;

  //printf("HOUR = %d\n", fFields[UCAL_HOUR]);
}

void
EthiopicCalendar::computeTime(UErrorCode& status) {
  if (U_FAILURE(status)) {
    //    puts("z0");
    return;
  } else if (timeType == E_WESTERN_TIME) {
    //puts("z1");
    return CECalendar::computeTime(status);
  } else {
    //puts("z2");
  }

  if (!isLenient()) {
    //puts("zp1");
    return CECalendar::computeTime(status);
  }

    // Compute the Julian day
    int32_t julianDay = computeJulianDay();

    double millis = Grego::julianDayToMillis(julianDay)
      - (kOneDay/4); // Ethiopian 6 hr

    //printf("z: %g\n", millis);

#if defined (U_DEBUG_CAL)
    //  int32_t julianInsanityCheck =  (int32_t)ClockMath::floorDivide(millis, kOneDay);
    //  julianInsanityCheck += kEpochStartAsJulianDay;
    //  if(1 || julianInsanityCheck != julianDay) {
    //    fprintf(stderr, "%s:%d- D'oh- computed jules %d, to mills (%s)%.lf, recomputed %d\n",
    //            __FILE__, __LINE__, julianDay, millis<0.0?"NEG":"", millis, julianInsanityCheck);
    //  }
#endif

    int32_t millisInDay;

    // We only use MILLISECONDS_IN_DAY if it has been set by the user.
    // This makes it possible for the caller to set the calendar to a
    // time and call clear(MONTH) to reset the MONTH to January.  This
    // is legacy behavior.  Without this, clear(MONTH) has no effect,
    // since the internally set JULIAN_DAY is used.
    if (fStamp[UCAL_MILLISECONDS_IN_DAY] >= ((int32_t)kMinimumUserStamp) &&
            newestStamp(UCAL_AM_PM, UCAL_MILLISECOND, kUnset) <= fStamp[UCAL_MILLISECONDS_IN_DAY]) {
        millisInDay = internalGet(UCAL_MILLISECONDS_IN_DAY);
    } else {
        millisInDay = computeMillisInDay();
    }

    UDate t = 0;
    if (fStamp[UCAL_ZONE_OFFSET] >= ((int32_t)kMinimumUserStamp) || fStamp[UCAL_DST_OFFSET] >= ((int32_t)kMinimumUserStamp)) {
        t = millis + millisInDay - (internalGet(UCAL_ZONE_OFFSET) + internalGet(UCAL_DST_OFFSET));
    } else {
      // PUNT
      //puts("zp0");
      return CECalendar::computeTime(status);
      #if 0
        // Compute the time zone offset and DST offset.  There are two potential
        // ambiguities here.  We'll assume a 2:00 am (wall time) switchover time
        // for discussion purposes here.
        //
        // 1. The positive offset change such as transition into DST.
        //    Here, a designated time of 2:00 am - 2:59 am does not actually exist.
        //    For this case, skippedWallTime option specifies the behavior.
        //    For example, 2:30 am is interpreted as;
        //      - WALLTIME_LAST(default): 3:30 am (DST) (interpreting 2:30 am as 31 minutes after 1:59 am (STD))
        //      - WALLTIME_FIRST: 1:30 am (STD) (interpreting 2:30 am as 30 minutes before 3:00 am (DST))
        //      - WALLTIME_NEXT_VALID: 3:00 am (DST) (next valid time after 2:30 am on a wall clock)
        // 2. The negative offset change such as transition out of DST.
        //    Here, a designated time of 1:00 am - 1:59 am can be in standard or DST.  Both are valid
        //    representations (the rep jumps from 1:59:59 DST to 1:00:00 Std).
        //    For this case, repeatedWallTime option specifies the behavior.
        //    For example, 1:30 am is interpreted as;
        //      - WALLTIME_LAST(default): 1:30 am (STD) - latter occurrence
        //      - WALLTIME_FIRST: 1:30 am (DST) - former occurrence
        //
        // In addition to above, when calendar is strict (not default), wall time falls into
        // the skipped time range will be processed as an error case.
        //
        // These special cases are mostly handled in #computeZoneOffset(long), except WALLTIME_NEXT_VALID
        // at positive offset change. The protected method computeZoneOffset(long) is exposed to Calendar
        // subclass implementations and marked as @stable. Strictly speaking, WALLTIME_NEXT_VALID
        // should be also handled in the same place, but we cannot change the code flow without deprecating
        // the protected method.
        //
        // We use the TimeZone object, unless the user has explicitly set the ZONE_OFFSET
        // or DST_OFFSET fields; then we use those fields.

        if (!isLenient() || fSkippedWallTime == UCAL_WALLTIME_NEXT_VALID) {
            // When strict, invalidate a wall time falls into a skipped wall time range.
            // When lenient and skipped wall time option is WALLTIME_NEXT_VALID,
            // the result time will be adjusted to the next valid time (on wall clock).
            int32_t zoneOffset = computeZoneOffset(millis, millisInDay, status);
            UDate tmpTime = millis + millisInDay - zoneOffset;

            int32_t raw, dst;
            fZone->getOffset(tmpTime, FALSE, raw, dst, status);

            if (U_SUCCESS(status)) {
                // zoneOffset != (raw + dst) only when the given wall time fall into
                // a skipped wall time range caused by positive zone offset transition.
                if (zoneOffset != (raw + dst)) {
                    if (!isLenient()) {
                        status = U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        U_ASSERT(fSkippedWallTime == UCAL_WALLTIME_NEXT_VALID);
                        // Adjust time to the next valid wall clock time.
                        // At this point, tmpTime is on or after the zone offset transition causing
                        // the skipped time range.
                        UDate immediatePrevTransition;
                        UBool hasTransition = getImmediatePreviousZoneTransition(tmpTime, &immediatePrevTransition, status);
                        if (U_SUCCESS(status) && hasTransition) {
                            t = immediatePrevTransition;
                        }
                    }
                } else {
                    t = tmpTime;
                }
            }
        } else {
            t = millis + millisInDay - computeZoneOffset(millis, millisInDay, status);
        }
        #endif
    }
    if (U_SUCCESS(status)) {
        internalSetTime(t);
    }
}
  

#if 0
// We do not want to introduce this API in ICU4C.
// It was accidentally introduced in ICU4J as a public API.

//-------------------------------------------------------------------------
// Calendar system Conversion methods...
//-------------------------------------------------------------------------

int32_t
EthiopicCalendar::ethiopicToJD(int32_t year, int32_t month, int32_t date)
{
    return ceToJD(year, month, date, JD_EPOCH_OFFSET_AMETE_MIHRET);
}
#endif

#if 0
/* - ETimeCalendar - */

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ETimeCalendar)


Calendar*
ETimeCalendar::clone(void) const
{
  UErrorCode status = U_ZERO_ERROR;
  LocalPointer<Calendar> n(new ETimeCalendar(fDelegate, status), status);
  n->setTime(getTime(status), status);
  return n.orphan();
}
#endif

U_NAMESPACE_END

/* argh */
#include "unicode/datefmt.h"

U_NAMESPACE_USE

U_CAPI UDateFormat *set_emode(UDateFormat *fmt, UErrorCode *status) {
  if(U_FAILURE(*status)) return fmt; // short circuit
  
  DateFormat &df = *((DateFormat*)fmt); // ok?

  //LocalPointer<Calendar> oldCal(df.getCalendar()->clone(), *status);
  Calendar &c = *((Calendar*)df.getCalendar());

  if(strcmp(c.getType(), "ethiopic")) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return fmt;
  }

  EthiopicCalendar &ec = dynamic_cast<EthiopicCalendar &>(c);

  // 'upgrade' the calendar
  LocalPointer<Calendar> newCal(new EthiopicCalendar(c.getLocale(ULOC_VALID_LOCALE, *status),
                                                     *status,
                                                     ec.isAmeteAlemEra()?EthiopicCalendar::AMETE_ALEM_ERA : EthiopicCalendar::AMETE_MIHRET_ERA,
                                                     EthiopicCalendar::E_ETHIOPIAN_TIME),
                                *status);

  if(U_FAILURE(*status)) return fmt;
  
  df.adoptCalendar(newCal.orphan());
  
  return fmt;
}


#endif
