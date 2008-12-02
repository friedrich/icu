/*
 * Copyright (C) 2008, International Business Machines Corporation and Others.
 * All rights reserved.
 */

#ifndef _BMS_H
#define _BMS_H

#include "unicode/utypes.h"
#include "unicode/ucol.h"

typedef void UCD;

U_CAPI UCD * U_EXPORT2
ucd_open(UCollator *coll);

U_CAPI void U_EXPORT2
ucd_close(UCD *ucd);

struct BMS;
typedef struct BMS BMS;

U_CAPI BMS * U_EXPORT2
bms_open(UCD *ucd,
         const UChar *pattern, int32_t patternLength,
         const UChar *target,  int32_t targetLength);

U_CAPI void U_EXPORT2
bms_close(BMS *bms);

U_CAPI UBool U_EXPORT2
bms_empty(BMS *bms);

U_CAPI UBool U_EXPORT2
bms_search(BMS *bms, int32_t offset, int32_t *start, int32_t *end);

U_CAPI void U_EXPORT2
bms_setTargetString(BMS *bms, UChar *target, int32_t targetLength);

#endif /* _BMS_H */
