// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.h
//
#ifndef RBBI_DICT_CACHE_H
#define RBBI_DICT_CACHE_H

#include "unicode/utypes.h"


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


U_NAMESPACE_END

#endif // RBBI_DICT_CACHE_H
