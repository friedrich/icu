// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.h
//
#ifndef RBBI_DICT_CACHE_H
#define RBBI_DICT_CACHE_H

#include "unicode/utypes.h"

#include "unicode/rbbi.h"
#include "unicode/uobject.h"

#include "uvectr32.h"

U_NAMESPACE_BEGIN

class RBBIDictCache: public UMemory {
  public:
     RBBIDictCache(UErrorCode &status);
     ~RBBIDictCache();

     void reset();

     UBool following(int32_t fromPos, int32_t *pos, int32_t *statusIndex);
     UBool preceding(int32_t fromPos, int32_t *pos, int32_t *statusIndex);


    /**
     * When a range of characters is divided up using the dictionary, the break
     * positions that are discovered are stored here, preventing us from having
     * to use either the dictionary or the state table again until the iterator
     * leaves this range of text. Has the most impact for line breaking.
     * @internal
     */
    // int32_t*            fCachedBreakPositions;

    UVector32          *fBreaks;

    /**
     * The number of elements in fCachedBreakPositions
     * @internal
     */
    int32_t             fNumCachedBreakPositions;

    /**
     * if fCachedBreakPositions is not null, this indicates which item in the
     * cache the current iteration position refers to
     * @internal
     */
    int32_t             fPositionInCache;

    int32_t             fStart;
    int32_t             fLimit;

    int32_t             fFirstRuleStatusIndex;
    int32_t             fOtherRuleStatusIndex;
};

static const int32_t CACHE_CHUNK_SIZE = 128;

class RuleBasedBreakIterator::BreakCache: public UObject {
  public:
                BreakCache(RuleBasedBreakIterator *bi, UErrorCode &status);
    virtual     ~BreakCache();
    void        reset(int32_t pos = 0, int32_t ruleStatus = 0);
    void        next(UErrorCode &status);
    void        previous(UErrorCode &status);

    // Move the iteration state to the position following the startPosition.
    // Input position must be pinned to the input length.
    void        following(int32_t startPosition, UErrorCode &status);

    void        preceding(int32_t startPosition, UErrorCode &status);

    int32_t     current();

    // Add boundaries to the cache near the specified position.
    // The given position need not be a boundary itself.
    // The input position must be within the range of the text, and
    // on a code point boundary.
    // Include a boundary at or preceding the position, and one
    // following the position.
    // Leave cache position on the boundary at or preceding the requested position.
    //
    // Return FALSE if the operation failed.
    UBool populateNear(int32_t position, UErrorCode &status);

    /**
     *  Add boundary(s) to the cache following the current last boundary.
     *  Return FALSE if at the end of the text, and no more boundaries can be added.
     *  Leave iteration position at the first newly added boundary.
     */
    UBool populateFollowing(UErrorCode &status);

    UBool populatePreceding(UErrorCode &status);

    enum UpdatePositionValues {
        RetainCachePosition = 0,
        UpdateCachePosition = 1
    };
    void addFollowing(int32_t position, int32_t ruleStatusIdx, UpdatePositionValues update);
    void addPreceding(int32_t position, int32_t ruleStatusIdx, UpdatePositionValues update);

    /**
     *  Set the cache position to the specified position, or, if the position
     *  falls between to cached boundaries, to the preceding boundary.
     *  The startPosition should be preset to be within the bounds of the
     *  input text, and on a code point boundary.
     *  Return TRUE if successful, FALSE if the specified position is after
     *  the last cached boundary or before the first.
     */
    UBool                   seek(int32_t startPosition);

    static int32_t          modChunkSize(int index);

    void dumpCache();

    RuleBasedBreakIterator *fBI;
    int32_t                 fStartBufIdx;
    int32_t                 fEndBufIdx;    // inclusive

    int32_t                 fTextIdx;
    int32_t                 fBufIdx;

    int32_t                 fBoundaries[CACHE_CHUNK_SIZE];
    uint16_t                fStatuses[CACHE_CHUNK_SIZE];

    UVector32               fSideBuffer;
};

U_NAMESPACE_END

#endif // RBBI_DICT_CACHE_H
