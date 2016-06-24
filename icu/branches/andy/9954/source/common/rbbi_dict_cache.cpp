// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.cpp

#include "unicode/utypes.h"

#include "rbbi_dict_cache.h"
#include "cmemory.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

RBBIDictCache::RBBIDictCache(UErrorCode &status) :
        fBreaks(NULL), fNumCachedBreakPositions(0), fPositionInCache(-1), fStart(0), fLimit(0) {
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
    fBreaks->removeAllElements();
}

int32_t RBBIDictCache::following(int32_t pos) {
    if (pos >= fLimit || pos < fStart) {
        fPositionInCache = -1;
        return -1;
    }

    // Sequential iteration, move from previous boundary to the following

    if (fPositionInCache >= 0 && fPositionInCache < fBreaks->size() && fBreaks->elementAti(fPositionInCache) == pos) {
        ++fPositionInCache;
        if (fPositionInCache >= fBreaks->size()) {
            fPositionInCache = -1;
            return -1;
        }
        int32_t result = fBreaks->elementAti(fPositionInCache);
        U_ASSERT(result > pos);
        return result;
    }

    // Random indexing. Linear search for the boundary following the given position.

    int32_t result = 0;
    for (fPositionInCache = 0; fPositionInCache < fBreaks->size(); ++fPositionInCache) {
        result = fBreaks->elementAti(fPositionInCache);
        if (result > pos) {
            return result;
        }
    }
    U_ASSERT(FALSE);
    fPositionInCache = -1;
    return -1;
}


int32_t RBBIDictCache::preceding(int32_t pos) {
    if (pos <= fStart || pos > fLimit) {
        fPositionInCache = -1;
        return -1;
    }

    int32_t result = -1;
    if (fPositionInCache > 0 && fPositionInCache < fBreaks->size() && fBreaks->elementAti(fPositionInCache) == pos) {
        --fPositionInCache;
        result = fBreaks->elementAti(fPositionInCache);
        U_ASSERT(result < pos);
        return result;
    }

    if (fPositionInCache == 0) {
        fPositionInCache = -1;
        return -1;
    }

    for (fPositionInCache = fBreaks->size()-1; fPositionInCache >= 0; --fPositionInCache) {
        result = fBreaks->elementAti(fPositionInCache);
        if (result < pos) {
            return result;
        }
    }
    U_ASSERT(FALSE);
    fPositionInCache = -1;
    return -1;
}



    

U_NAMESPACE_END
