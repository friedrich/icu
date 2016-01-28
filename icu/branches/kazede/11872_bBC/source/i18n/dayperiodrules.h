/*
*******************************************************************************
* Copyright (C) 2016, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* dayperiodrules.h
*
* created on: 2016-01-20
* created by: kazede
*/

#ifndef DAYPERIODRULES_H
#define DAYPERIODRULES_H

#include "unicode/locid.h"
#include "unicode/unistr.h"
#include "unicode/uobject.h"
#include "unicode/utypes.h"
#include "resource.h"
#include "uhash.h"



U_NAMESPACE_BEGIN

struct DayPeriodRulesDataSink;

class DayPeriodRules : public UMemory {
    friend struct DayPeriodRulesDataSink;
public:
    enum DayPeriod {
        DAYPERIOD_UNKNOWN = -1,
        DAYPERIOD_MIDNIGHT,
        DAYPERIOD_NOON,
        DAYPERIOD_MORNING1,
        DAYPERIOD_AFTERNOON1,
        DAYPERIOD_EVENING1,
        DAYPERIOD_NIGHT1,
        DAYPERIDO_MORNING2,
        DAYPERIOD_AFTERNOON2,
        DAYPERIOD_EVENING2,
        DAYPERIOD_NIGHT2
    };

    static const DayPeriodRules *getInstance(const Locale &locale, UErrorCode &errorCode);

    UBool hasMidnight() const { return fHasMidnight; }
    UBool hasNoon() const { return fHasNoon; }
    DayPeriod getDayPeriodForHour(int32_t hour) const { return fDayPeriodForHour[hour]; }

private:
    DayPeriodRules();

    // Translates "morning1" to DAYPERIOD_MORNING1, for example.
    static DayPeriod getDayPeriodFromString(const char *type_str);

    static void load(UErrorCode &errorCode);

    // Sets period type for all hours in [startHour, limitHour).
    void add(int32_t startHour, int32_t limitHour, DayPeriod period);

    // Returns TRUE if for all i, DayPeriodForHour[i] has a type other than UNKNOWN.
    // Values of HasNoon and HasMidnight do not affect the return value.
    UBool allHoursAreSet();

    UBool fHasMidnight;
    UBool fHasNoon;
    DayPeriod fDayPeriodForHour[24];
};

U_NAMESPACE_END

#endif /* DAYPERIODRULES_H */
