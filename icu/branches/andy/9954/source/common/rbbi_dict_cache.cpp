// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.cpp

#include "unicode/utypes.h"
#include "unicode/ubrk.h"
#include "unicode/rbbi.h"

#include "rbbi_dict_cache.h"
#include "cmemory.h"
#include "uassert.h"
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


RuleBasedBreakIterator::BreakCache::BreakCache(RuleBasedBreakIterator *bi, UErrorCode &status) : fBI(bi) {
    reset();
}


RuleBasedBreakIterator::BreakCache::~BreakCache() {
}


void RuleBasedBreakIterator::BreakCache::reset(int32_t pos, int32_t ruleStatus) {
    fStartBufIdx = 0;
    fEndBufIdx = 0;
    fTextIdx = pos;
    fBufIdx = 0;
    fBoundaries[0] = pos;
    fStatuses[0] = (uint16_t)ruleStatus;
}


int32_t RuleBasedBreakIterator::BreakCache::following(int32_t startPos, int32_t *ruleStatus, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return UBRK_DONE;
    }
    if (((startPos == fTextIdx) || seek(startPos)) && (fEndBufIdx != fBufIdx)) {
        // Chache Hit. return the next entry from this cache block.
        int32_t newBufIdx = modChunkSize(fBufIdx + 1);
        int32_t newTextIdx = fBoundaries[newBufIdx];
        U_ASSERT(newTextIdx > fTextIdx);
        fBufIdx = newBufIdx;
        fTextIdx = newTextIdx;
        *ruleStatus = fStatuses[newBufIdx];
        return fTextIdx;
    }
    if (startPos == fBoundaries[fEndBufIdx]) {
        //  Cache Miss at end of cache. Call back to the engine to add more boundaries.
        if (fBI->populateCacheFollowing(startPos, status)) {
            U_ASSERT(fBufIdx != fEndBufIdx);
            static int recursionCount = 0;
            if (recursionCount++ > 0) {
                printf("Recursion!");
            }
            int32_t retVal = following(startPos, ruleStatus, status);
            --recursionCount;
            return retVal;
        } else {
            *ruleStatus = 0;
            return UBRK_DONE;
        }
    }

    // Cache miss, asking for contents disjoint ... 
    // Call coming in from RBBI::following(), which backs up with the safe rules, then calls next(), which ends up here.
    // Replace with a direct call to BreakCache::following().
    // Figure out where to put the safe rule invocation.
}


void RuleBasedBreakIterator::BreakCache::addFollowing(
        int32_t previousPos, int32_t newPos, int32_t ruleStatus, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    U_ASSERT(previousPos < newPos);
    U_ASSERT(previousPos >= 0);
    U_ASSERT(ruleStatus >= 0 && ruleStatus < UINT16_MAX);

    if (previousPos == fBoundaries[fEndBufIdx]) {
        // Appending to the existing cached range.
        fEndBufIdx = modChunkSize(fEndBufIdx + 1);
        if (fEndBufIdx == fStartBufIdx) {
            // Buffer full, make space.
            fStartBufIdx = modChunkSize(fStartBufIdx + 1);
        }
        fBoundaries[fEndBufIdx] = newPos;
        fStatuses[fEndBufIdx] = (uint16_t)ruleStatus;
    } else if (previousPos < fBoundaries[fStartBufIdx] || previousPos > fBoundaries[fEndBufIdx]) {
        // Adding a position wholly outside of the existing cached range.
        // Drop the existing cache and start over.
        reset(newPos, ruleStatus);
    } else {
        // Request to add a boundary within the existing cached range.
        // Should be impossible to get here.
        U_ASSERT(FALSE);
        status = U_INTERNAL_PROGRAM_ERROR;
    }
}


UBool RuleBasedBreakIterator::BreakCache::seek(int32_t pos) {
    if (pos < fBoundaries[fStartBufIdx] || pos > fBoundaries[fEndBufIdx]) {
        return FALSE;
    }
    if (pos == fBoundaries[fEndBufIdx]) {
        fBufIdx = fEndBufIdx;
        fTextIdx = fBoundaries[fBufIdx];
        return TRUE;
    }
    
    int32_t min = fStartBufIdx;
    int32_t max = fEndBufIdx;
    while (min != max) {
        int32_t probe = (min + max + (min>max ? CACHE_CHUNK_SIZE : 0)) / 2;
        probe = modChunkSize(probe);
        if (fBoundaries[probe] > pos) {
            max = probe;
        } else {
            min = modChunkSize(probe + 1);
        }
    }
    U_ASSERT(fBoundaries[max] > pos);
    fBufIdx = modChunkSize(max - 1);
    fTextIdx = fBoundaries[fBufIdx];
    U_ASSERT(fTextIdx <= pos);
    return TRUE;
}

#if 0
    int32_t startPos = current();
    int32_t dictResult = 0;
    if (fDictionaryCache->following(startPos, &dictResult, &fLastRuleStatusIndex)) {
        utext_setNativeIndex(fText, dictResult);
        return dictResult;
    }

    int32_t prevStatus = fLastRuleStatusIndex;
    int32_t ruleResult = handleNext(fData->fForwardTable);
    if (fDictionaryCharCount > 0) {
        checkDictionary(startPos, ruleResult, prevStatus, fLastRuleStatusIndex);
        if (fDictionaryCache->following(startPos, &dictResult, &fLastRuleStatusIndex)) {
            utext_setNativeIndex(fText, dictResult);
            return dictResult;
        }
    }
    return ruleResult;
#endif

UBool RuleBasedBreakIterator::BreakCache::populateNear(int32_t position, UErrorCode &status) {
    U_ASSERT(position < fBoundaries[fStartBufIdx] || position > fBoundaries[fEndBufIdx]);
    U_ASSERT(postion >= 0);

    reset();
    utext_setNativeIndex(position);
    if (utext_current32(fBI->fText) == U_SENTINEL) {
        return FALSE;
    }

    // Apply safe reverse rule.
    int32_t backupPos = handlePrevious(fBI->fData->fSafeRevTable);

    if (backupPos == UBRK_DONE) {
        backupPos = 0;
        utext_setNativeIndex(fText, 0);
    }

    while cache last pos < position
        populate following

    while cache first pos > x
        populate preceding


PopulateFollowing()


PopulatePreceding()



int32_t RuleBasedBreakIterator::BreakCache::modChunkSize(int32_t n) {
    if (n >= CACHE_CHUNK_SIZE) {
        return n - CACHE_CHUNK_SIZE;
    }
    if (n < 0) {
        return n + CACHE_CHUNK_SIZE;
    }
    return n;
}


U_NAMESPACE_END
