// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.cpp

#include "unicode/utypes.h"
#include "unicode/ubrk.h"
#include "unicode/rbbi.h"

#include "rbbi_dict_cache.h"

#include "cmemory.h"
#include "rbbidata.h"
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

void RBBIDictCache::reset() {
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


RuleBasedBreakIterator::BreakCache::BreakCache(RuleBasedBreakIterator *bi, UErrorCode &status) : 
        fBI(bi), fSideBuffer(status) {
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


int32_t  RuleBasedBreakIterator::BreakCache::current() {
    fBI->fState.fPosition = fTextIdx;
    fBI->fState.fRuleStatusIndex = fStatuses[fBufIdx];
    fBI->fState.fDone = FALSE;
    return fTextIdx;
}


void RuleBasedBreakIterator::BreakCache::following(int32_t startPos, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (startPos == fTextIdx || seek(startPos) || populateNear(startPos, status)) {
        // startPos is in the cache. Do a next() from that position.
        next(status);
    }
    return;
}


void RuleBasedBreakIterator::BreakCache::preceding(int32_t startPos, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (startPos == fTextIdx || seek(startPos) || populateNear(startPos, status)) {
        if (startPos == fTextIdx) {
            previous(status);
        } else {
            // seek() leaves the BreakCache positioned at the preceding boundary
            //        if the requested position is between two bounaries.
            // current() pushes the BreakCache position out to the BreakIterator itself.
            U_ASSERT(startPos > fTextIdx);
            current();
        }
    }
    return;
}


void RuleBasedBreakIterator::BreakCache::next(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    int32_t initialBufIdx = fBufIdx;
    if (fBufIdx == fEndBufIdx) {
        // At end of cache. Add to it.
        populateFollowing(status);
    } else {
        // Cache already holds the next boundary
        fBufIdx = modChunkSize(fBufIdx + 1);
        fTextIdx = fBoundaries[fBufIdx];
    }
    fBI->fState.fDone = (fBufIdx == initialBufIdx);
    fBI->fState.fPosition = fTextIdx;
    fBI->fState.fRuleStatusIndex = fStatuses[fBufIdx];
    return;
}


void RuleBasedBreakIterator::BreakCache::previous(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    int32_t initialBufIdx = fBufIdx;
    if (fBufIdx == fStartBufIdx) {
        // At start of cache. Prepend to it.
        populatePreceding(status);
    } else {
        // Cache already holds the next boundary
        fBufIdx = modChunkSize(fBufIdx - 1);
        fTextIdx = fBoundaries[fBufIdx];
    }
    fBI->fState.fDone = (fBufIdx == initialBufIdx);
    fBI->fState.fPosition = fTextIdx;
    fBI->fState.fRuleStatusIndex = fStatuses[fBufIdx];
    return;
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


UBool RuleBasedBreakIterator::BreakCache::populateNear(int32_t position, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    U_ASSERT(position < fBoundaries[fStartBufIdx] || position > fBoundaries[fEndBufIdx]);

    // Find a boundary somewhere in the vicinity of the requested position.
    // Depending on the safe rules and the text data, it could be either before, at, or after
    // the requested position.

    int32_t aBoundary;
    int32_t ruleStatusIndex;

    if (position == 0) {
        aBoundary = 0;
        ruleStatusIndex = 0;
    } else {
        utext_setNativeIndex(fBI->fText, position);
        int32_t backupPos = fBI->handlePrevious(fBI->fData->fSafeRevTable);  // TODO: check for backuPos already in cache.
        fBI->fState.fPosition = backupPos;
        aBoundary = fBI->handleNext(fBI->fData->fForwardTable);  // Ignore dictionary, just finding any boundary.
        ruleStatusIndex = fBI->fState.fRuleStatusIndex;
    }

    // TODO: check if aBoundary is already in cache, don't clear if it is.
    //       Could happen with a poor reverse rule.
    reset(aBoundary, ruleStatusIndex);   // Reset cache to hold aBoundary as a single starting point.
        

    while (fBoundaries[fEndBufIdx] < position) {
        // The last position in the cache precedes the requested position.
        // Add following position(s) to the cache.
        // A sub-optimal safe reverse rule, like ".*", will require looping here.
        if (!populateFollowing(status)) {
            break;
        }
    }

    while (fBoundaries[fStartBufIdx] > position) {
        // The first position in the cache is beyond the requested position.
        // The safe reverse rule did not back up enough;
        // back up more until we get a boundary <= the requested position.
        populatePreceding(status);
    }

    // TODO: replace seek() here. Desired position should be either the last
    //       or next to last in the cache.
    UBool success = seek(position);
    U_ASSERT(success);
    if (!success) {
        status = U_INTERNAL_PROGRAM_ERROR;
    }
    return success;
}



UBool RuleBasedBreakIterator::BreakCache::populateFollowing(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    int32_t fromPosition = fBoundaries[fEndBufIdx];
    int32_t fromRuleStatusIdx = fStatuses[fEndBufIdx];
    int32_t pos = 0;
    int32_t ruleStatusIdx = 0;

    if (fBI->fDictionaryCache->following(fromPosition, &pos, &ruleStatusIdx)) {
        addFollowing(pos, ruleStatusIdx, UpdateCachePosition);
        return TRUE;
    }

    fBI->fState.fPosition = fromPosition;
    pos = fBI->handleNext(fBI->fData->fForwardTable);
    if (pos == UBRK_DONE) {
        return FALSE;
    }

    ruleStatusIdx = fBI->fState.fRuleStatusIndex;
    if (fBI->fDictionaryCharCount > 0) {
        // Segment from the rules includes dictionary characters.
        // Subdivide it, with subdivided results going into the dictionary cache.
        fBI->checkDictionary(fromPosition, pos, fromRuleStatusIdx, ruleStatusIdx);
        if (fBI->fDictionaryCache->following(fromPosition, &pos, &ruleStatusIdx)) {
            addFollowing(pos, ruleStatusIdx, UpdateCachePosition);
            return TRUE;
            // TODO: may want to move a sizeable chunk of dictionary cache to break cache at this point.
        }
    }

    // Rule based segment did not include dictionary characters.
    // Or, it did contain dictionary chars, but the dictionary segmenter didn't handle them,
    //    meaning that we didn't take the return, above.  TODO: clean this up.
    // Add its end point to the cache.
    addFollowing(pos, ruleStatusIdx, UpdateCachePosition);
    return TRUE;
}


UBool RuleBasedBreakIterator::BreakCache::populatePreceding(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }

    int32_t fromPosition = fBoundaries[fStartBufIdx];
    if (fromPosition == 0) {
        return FALSE;
    }

    int32_t position = 0;
    int32_t positionStatusIdx = 0;

    if (fBI->fDictionaryCache->preceding(fromPosition, &position, &positionStatusIdx)) {
        addPreceding(position, positionStatusIdx, UpdateCachePosition);
        return TRUE;
    }

    int32_t backupPosition = fromPosition;

    // Find a boundary somewhere preceding the first already-cached boundary
    do {
        backupPosition = backupPosition - 10;
        utext_setNativeIndex(fBI->fText, backupPosition);   // TODO: check for zero
        backupPosition = fBI->handlePrevious(fBI->fData->fSafeRevTable);
        if (backupPosition == UBRK_DONE || backupPosition == 0) {
            position = 0;
            positionStatusIdx = 0;
        } else {
            fBI->fState.fPosition = backupPosition;  // TODO: pass starting position in a clearer way.
            position = fBI->handleNext(fBI->fData->fForwardTable);
            positionStatusIdx = fBI->fState.fRuleStatusIndex;

        }
    } while (position >= fromPosition);

    // Find boundaries between the one we just located and the first already-cached boundary
    // Put them in a side buffer.

    fSideBuffer.removeAllElements();
    fSideBuffer.addElement(position, status);
    fSideBuffer.addElement(positionStatusIdx, status);

    do {
        int32_t prevPosition = fBI->fState.fPosition = position;
        int32_t prevStatusIdx = positionStatusIdx;
        position = fBI->handleNext(fBI->fData->fForwardTable);
        positionStatusIdx = fBI->fState.fRuleStatusIndex;
        if (position == UBRK_DONE) {
            break;
        }

        UBool segmentHandledByDictionary = FALSE;
        if (fBI->fDictionaryCharCount != 0) {
            // Segment from the rules includes dictionary characters.
            // Subdivide it, with subdivided results going into the dictionary cache.
            int32_t dictSegEndPosition = position;
            int32_t dictSegStatusIdx = positionStatusIdx;
            fBI->checkDictionary(prevPosition, dictSegEndPosition, prevStatusIdx, positionStatusIdx);
            while (fBI->fDictionaryCache->following(prevPosition, &position, &positionStatusIdx)) {
                segmentHandledByDictionary = TRUE;
                U_ASSERT(position > prevPosition);
                if (position >= fromPosition) {
                    break;
                }
                U_ASSERT(position <= dictSegEndPosition);
                fSideBuffer.addElement(position, status);
                fSideBuffer.addElement(positionStatusIdx, status);
                prevPosition = position;
            }
            U_ASSERT(position==dictSegEndPosition || position>=fromPosition);
        }
            
        if (!segmentHandledByDictionary && position < fromPosition) {
            fSideBuffer.addElement(position, status);
            fSideBuffer.addElement(positionStatusIdx, status);
        }
    } while (position < fromPosition);

    // Move boundaries from the side buffer to the main circular buffer, reversing their order.
    UBool success = FALSE;
    if (!fSideBuffer.isEmpty()) {
        positionStatusIdx = fSideBuffer.popi();
        position = fSideBuffer.popi();
        addPreceding(position, positionStatusIdx, UpdateCachePosition);
        success = TRUE;
    }

    // TODO: limit, in case dictionary segment gives too many boundaries for circular buffer.
    while (!fSideBuffer.isEmpty()) {
        positionStatusIdx = fSideBuffer.popi();
        position = fSideBuffer.popi();
        addPreceding(position, positionStatusIdx, RetainCachePosition);
    }
      
    return success;
}


void RuleBasedBreakIterator::BreakCache::addFollowing(int32_t position, int32_t ruleStatusIdx, UpdatePositionValues update) {
    U_ASSERT(position > fBoundaries[fEndBufIdx]);
    U_ASSERT(ruleStatusIdx <= UINT16_MAX);
    int32_t nextIdx = modChunkSize(fEndBufIdx + 1);
    if (nextIdx == fStartBufIdx) {
        fStartBufIdx = modChunkSize(fStartBufIdx + 1);
    }
    fBoundaries[nextIdx] = position;
    fStatuses[nextIdx] = ruleStatusIdx;
    fEndBufIdx = nextIdx;
    if (update == UpdateCachePosition) {
        fBufIdx = nextIdx;
        fTextIdx = position;
    }
}

void RuleBasedBreakIterator::BreakCache::addPreceding(int32_t position, int32_t ruleStatusIdx, UpdatePositionValues update) {
    U_ASSERT(position < fBoundaries[fStartBufIdx]);
    U_ASSERT(ruleStatusIdx <= UINT16_MAX);
    int32_t nextIdx = modChunkSize(fStartBufIdx - 1);
    if (nextIdx == fEndBufIdx) {
        fEndBufIdx = modChunkSize(fEndBufIdx - 1);
    }
    fBoundaries[nextIdx] = position;
    fStatuses[nextIdx] = ruleStatusIdx;
    fStartBufIdx = nextIdx;
    if (update == UpdateCachePosition) {
        fBufIdx = nextIdx;
        fTextIdx = position;
    } else {
        // The original iteration position is being retained.
        // The buffer shouldn't wrap around such that the newly added point overwrites the iteration position.
        U_ASSERT(fBufIdx != nextIdx);
    }
}




int32_t RuleBasedBreakIterator::BreakCache::modChunkSize(int32_t n) {
    if (n >= CACHE_CHUNK_SIZE) {
        return n - CACHE_CHUNK_SIZE;
    }
    if (n < 0) {
        return n + CACHE_CHUNK_SIZE;
    }
    return n;
}


void RuleBasedBreakIterator::BreakCache::dumpCache() {
    printf("fTextIdx:%d   fBufIdx:%d\n", fTextIdx, fBufIdx);
    for (int32_t i=fStartBufIdx; ; i=modChunkSize(i+1)) {
        printf("%d  %d\n", i, fBoundaries[i]);
        if (i == fEndBufIdx) {
            break;
        }
    }
}

U_NAMESPACE_END
