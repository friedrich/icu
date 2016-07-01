// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.cpp

#include "unicode/utypes.h"

#include "rbbi_dict_cache.h"
#include "cmemory.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

RBBIDictCache::RBBIDictCache(UErrorCode &status) :
        fBreaks(NULL), fNumCachedBreakPositions(0), fPositionInCache(-1),
        fStart(0), fLimit(0), fFirstRuleStatusIndex(0), fOtherRuleStatusIndex(0) {
    fBreaks = new UVector32(status);
}

RBBIDictCache::~RBBIDictCache() {
    delete fBreaks;
    fBreaks = NULL;
}

void
RBBIDictCache::reset() {
    fNumCachedBreakPositions = 0;
    fPositionInCache = -1;
    fStart = 0;
    fLimit = 0;
    fFirstRuleStatusIndex = 0;
    fOtherRuleStatusIndex = 0;
    fBreaks->removeAllElements();
}

UBool RBBIDictCache::following(int32_t fromPos, int32_t *result, int32_t *statusIndex) {
    if (fromPos >= fLimit || fromPos < fStart) {
        fPositionInCache = -1;
        return FALSE;
    }

    // Sequential iteration, move from previous boundary to the following

    int32_t r = 0;
    if (fPositionInCache >= 0 && fPositionInCache < fBreaks->size() && fBreaks->elementAti(fPositionInCache) == fromPos) {
        ++fPositionInCache;
        if (fPositionInCache >= fBreaks->size()) {
            fPositionInCache = -1;
            return FALSE;
        }
        r = fBreaks->elementAti(fPositionInCache);
        U_ASSERT(r > fromPos);
        *result = r;
        *statusIndex = fOtherRuleStatusIndex;
        return TRUE;
    }

    // Random indexing. Linear search for the boundary following the given position.

    for (fPositionInCache = 0; fPositionInCache < fBreaks->size(); ++fPositionInCache) {
        r= fBreaks->elementAti(fPositionInCache);
        if (r > fromPos) {
            *result = r;
            *statusIndex = fOtherRuleStatusIndex;
            return TRUE;
        }
    }
    U_ASSERT(FALSE);
    fPositionInCache = -1;
    return FALSE;
}


UBool RBBIDictCache::preceding(int32_t fromPos, int32_t *result, int32_t *statusIndex) {
    if (fromPos <= fStart || fromPos > fLimit) {
        fPositionInCache = -1;
        return FALSE;
    }

    if (fromPos == fLimit) {
        fPositionInCache = fBreaks->size() - 1;
        if (fPositionInCache >= 0) {
            U_ASSERT(fBreaks->elementAti(fPositionInCache) == fromPos);
        }
    }

    int32_t r;
    if (fPositionInCache > 0 && fPositionInCache < fBreaks->size() && fBreaks->elementAti(fPositionInCache) == fromPos) {
        --fPositionInCache;
        r = fBreaks->elementAti(fPositionInCache);
        U_ASSERT(r < fromPos);
        *result = r;
        *statusIndex = ( r== fStart) ? fFirstRuleStatusIndex : fOtherRuleStatusIndex;
        return TRUE;
    }

    if (fPositionInCache == 0) {
        fPositionInCache = -1;
        return FALSE;
    }

    for (fPositionInCache = fBreaks->size()-1; fPositionInCache >= 0; --fPositionInCache) {
        r = fBreaks->elementAti(fPositionInCache);
        if (r < fromPos) {
            *result = r;
            *statusIndex = ( r == fStart) ? fFirstRuleStatusIndex : fOtherRuleStatusIndex;
            return TRUE;
        }
    }
    U_ASSERT(FALSE);
    fPositionInCache = -1;
    return FALSE;
}



    

U_NAMESPACE_END
