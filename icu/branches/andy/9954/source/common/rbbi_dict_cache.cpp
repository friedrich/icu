// Copyright (C) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// file: rbbi_dict_cache.cpp

#include "unicode/utypes.h"

#include "rbbi_dict_cache.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

RBBIDictCache::RBBIDictCache() : fCachedBreakPositions(0), fNumCachedBreakPositions(0), fPositionInCache(0) {
}

RBBIDictCache::~RBBIDictCache() {
    delete fCachedBreakPositions;
    fCachedBreakPositions = NULL;
}

void
RBBIDictCache::reset() {
    if (fCachedBreakPositions) {
        uprv_free(fCachedBreakPositions);
    }
    fCachedBreakPositions = NULL;
    fNumCachedBreakPositions = 0;
    fPositionInCache = 0;
}


U_NAMESPACE_END
