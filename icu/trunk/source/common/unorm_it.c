/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  unorm_it.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003jan21
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uiter.h"
#include "unicode/unorm.h"
#include "unorm_it.h"
#include "cmemory.h"

/* UNormIterator ------------------------------------------------------------ */

enum {
    initialCapacity=100
};

struct UNormIterator {
    UCharIterator api;
    UCharIterator *iter;

    /*
     * chars and states either use the static buffers
     * or are allocated in the same memory block
     *
     * They are parallel arrays with states[] holding the getState() values
     * from normalization boundaries, and UITER_NO_STATE in between.
     */
    UChar *chars;
    uint32_t *states;

    /*
     * api.start: first valid character & state in the arrays
     * api.index: current position
     * api.limit: one past the last valid character in chars[], but states[limit] is valid
     * capacity: length of allocated arrays
     */
    int32_t capacity;

    /* the current iter->getState(), saved to avoid unnecessary setState() calls; may not correspond to api->index! */
    uint32_t state;

    /* there are UChars available before start or after limit? */
    UBool hasPrevious, hasNext;

    UNormalizationMode mode;

    UChar charsBuffer[initialCapacity];
    uint32_t statesBuffer[initialCapacity+1]; /* one more than charsBuffer[]! */
};

static void
initIndexes(UNormIterator *uni, UCharIterator *iter) {
    /* do not pass api so that the compiler knows it's an alias pointer to uni itself */
    UCharIterator *api=&uni->api;

    if(!iter->hasPrevious(iter)) {
        /* set indexes to the beginning of the arrays */
        api->start=api->index=api->limit=0;
        uni->hasPrevious=FALSE;
        uni->hasNext=iter->hasNext(iter);
    } else if(!iter->hasNext(iter)) {
        /* set indexes to the end of the arrays */
        api->start=api->index=api->limit=uni->capacity;
        uni->hasNext=FALSE;
        uni->hasPrevious=iter->hasPrevious(iter);
    } else {
        /* set indexes into the middle of the arrays */
        api->start=api->index=api->limit=uni->capacity/2;
        uni->hasPrevious=uni->hasNext=TRUE;
    }
}

static UBool
reallocArrays(UNormIterator *uni, int32_t capacity, UBool addAtStart) {
    /* do not pass api so that the compiler knows it's an alias pointer to uni itself */
    UCharIterator *api=&uni->api;

    uint32_t *states;
    UChar *chars;
    int32_t start, limit;

    states=(uint32_t *)uprv_malloc((capacity+1)*4+capacity*2);
    if(states==NULL) {
        return FALSE;
    }

    chars=(UChar *)(states+(capacity+1));
    uni->capacity=capacity;

    start=api->start;
    limit=api->limit;

    if(addAtStart) {
        /* copy old contents to the end of the new arrays */
        int32_t delta;

        delta=capacity-uni->capacity;
        uprv_memcpy(states+delta+start, uni->states+start, (limit-start+1)*4);
        uprv_memcpy(chars+delta+start, uni->chars+start, (limit-start)*4);

        api->start=start+delta;
        api->index+=delta;
        api->limit=limit+delta;
    } else {
        /* copy old contents to the beginning of the new arrays */
        uprv_memcpy(states+start, uni->states+start, (limit-start+1)*4);
        uprv_memcpy(chars+start, uni->chars+start, (limit-start)*4);
    }

    uni->chars=chars;
    uni->states=states;

    return TRUE;
}

static void
moveContentsTowardStart(UCharIterator *api, UChar chars[], uint32_t states[], int32_t delta) {
    /* move array contents up to make room */
    int32_t srcIndex, destIndex, limit;

    limit=api->limit;
    srcIndex=delta;
    if(srcIndex>api->start) {
        /* look for a position in the arrays with a known state */
        while(srcIndex<limit && chars[srcIndex]==UITER_NO_STATE) {
            ++srcIndex;
        }
    }

    /* now actually move the array contents */
    api->start=destIndex=0;
    while(srcIndex<limit) {
        chars[destIndex]=chars[srcIndex];
        states[destIndex++]=states[srcIndex++];
    }

    /* copy states[limit] as well! */
    states[destIndex]=states[srcIndex];

    api->limit=destIndex;
}

static void
moveContentsTowardEnd(UCharIterator *api, UChar chars[], uint32_t states[], int32_t delta) {
    /* move array contents up to make room */
    int32_t srcIndex, destIndex, start;

    start=api->start;
    destIndex=((UNormIterator *)api)->capacity;
    srcIndex=destIndex-delta;
    if(srcIndex<api->limit) {
        /* look for a position in the arrays with a known state */
        while(srcIndex>start && chars[srcIndex]==UITER_NO_STATE) {
            --srcIndex;
        }
    }

    /* now actually move the array contents */
    api->limit=destIndex;

    /* copy states[limit] as well! */
    states[destIndex]=states[srcIndex];

    while(srcIndex>start) {
        chars[--destIndex]=chars[--srcIndex];
        states[destIndex]=states[srcIndex];
    }

    api->start=destIndex;
}

/* normalize forward from the limit, assume hasNext is true */
static UBool
readNext(UNormIterator *uni, UCharIterator *iter) {
    /* do not pass api so that the compiler knows it's an alias pointer to uni itself */
    UCharIterator *api=&uni->api;

    /* make capacity/4 room at the end of the arrays */
    int32_t limit, capacity, room, delta;
    UErrorCode errorCode;

    limit=api->limit;
    capacity=uni->capacity;
    room=capacity/4;
    delta=room-(capacity-limit);
    if(delta>0) {
        /* move array contents to make room */
        moveContentsTowardStart(api, uni->chars, uni->states, delta);
        api->index=limit=api->limit;
    }

    /* normalize starting from the limit position */
    errorCode=U_ZERO_ERROR;
    if(uni->state!=uni->states[limit]) {
        uiter_setState(iter, uni->states[limit], &errorCode);
        if(U_FAILURE(errorCode)) {
            uni->state=UITER_NO_STATE;
            uni->hasNext=FALSE;
            return FALSE;
        }
    }

    room=unorm_next(iter, uni->chars+limit, capacity-limit, uni->mode, 0, TRUE, NULL, &errorCode);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        if(room<=capacity) {
            /* empty and re-use the arrays */
            uni->states[0]=uni->states[limit];
            api->start=api->index=api->limit=limit=0;
        } else {
            capacity+=room+100;
            if(!reallocArrays(uni, capacity, FALSE)) {
                uni->state=UITER_NO_STATE;
                uni->hasNext=FALSE;
                return FALSE;
            }
            limit=api->limit;
        }

        errorCode=U_ZERO_ERROR;
        uiter_setState(iter, uni->states[limit], &errorCode);
        room=unorm_next(iter, uni->chars+limit, capacity-limit, uni->mode, 0, TRUE, NULL, &errorCode);
    }
    if(U_FAILURE(errorCode) || room==0) {
        uni->state=UITER_NO_STATE;
        uni->hasNext=FALSE;
        return FALSE;
    }

    /* room>0 */
    ++limit; /* leave the known states[limit] alone */
    for(--room; room>0; --room) {
        /* set unknown states for all but the normalization boundaries */
        uni->states[limit++]=UITER_NO_STATE;
    }
    uni->states[limit]=uni->state=uiter_getState(iter);
    uni->hasNext=iter->hasNext(iter);
    api->limit=limit;
    return TRUE;
}

/* normalize backward from the start, assume hasPrevious is true */
static UBool
readPrevious(UNormIterator *uni, UCharIterator *iter) {
    /* do not pass api so that the compiler knows it's an alias pointer to uni itself */
    UCharIterator *api=&uni->api;

    /* make capacity/4 room at the start of the arrays */
    int32_t start, capacity, room, delta;
    UErrorCode errorCode;

    start=api->start;
    capacity=uni->capacity;
    room=capacity/4;
    delta=room-start;
    if(delta>0) {
        /* move array contents to make room */
        moveContentsTowardEnd(api, uni->chars, uni->states, delta);
        api->index=start=api->start;
    }

    /* normalize ending at the start position */
    errorCode=U_ZERO_ERROR;
    if(uni->state!=uni->states[start]) {
        uiter_setState(iter, uni->states[start], &errorCode);
        if(U_FAILURE(errorCode)) {
            uni->state=UITER_NO_STATE;
            uni->hasPrevious=FALSE;
            return FALSE;
        }
    }

    room=unorm_previous(iter, uni->chars, start, uni->mode, 0, TRUE, NULL, &errorCode);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        if(room<=capacity) {
            /* empty and re-use the arrays */
            uni->states[capacity]=uni->states[start];
            api->start=api->index=api->limit=start=capacity;
        } else {
            capacity+=room+100;
            if(!reallocArrays(uni, capacity, TRUE)) {
                uni->state=UITER_NO_STATE;
                uni->hasPrevious=FALSE;
                return FALSE;
            }
            start=api->start;
        }

        errorCode=U_ZERO_ERROR;
        uiter_setState(iter, uni->states[start], &errorCode);
        room=unorm_previous(iter, uni->chars, start, uni->mode, 0, TRUE, NULL, &errorCode);
    }
    if(U_FAILURE(errorCode) || room==0) {
        uni->state=UITER_NO_STATE;
        uni->hasPrevious=FALSE;
        return FALSE;
    }

    /* room>0 */
    do {
        /* copy the UChars from chars[0..room[ to chars[(start-room)..start[ */
        uni->chars[--start]=uni->chars[--room];
        /* set unknown states for all but the normalization boundaries */
        uni->states[start]=UITER_NO_STATE;
    } while(room>0);
    uni->states[start]=uni->state=uiter_getState(iter);
    uni->hasPrevious=iter->hasPrevious(iter);
    api->start=start;
    return TRUE;
}

/* Iterator runtime API functions ------------------------------------------- */

static int32_t U_CALLCONV
unormIteratorGetIndex(UCharIterator *api, UCharIteratorOrigin origin) {
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        return 0;
    case UITER_CURRENT:
    case UITER_LIMIT:
    case UITER_LENGTH:
        return UITER_UNKNOWN_INDEX;
    default:
        /* not a valid origin */
        /* Should never get here! */
        return -1;
    }
}

static int32_t U_CALLCONV
unormIteratorMove(UCharIterator *api, int32_t delta, UCharIteratorOrigin origin) {
    UNormIterator *uni=(UNormIterator *)api;
    UCharIterator *iter=uni->iter;
    int32_t pos;

    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        /* restart from the beginning */
        if(uni->hasPrevious) {
            iter->move(iter, 0, UITER_START);
            api->start=api->index=api->limit=0;
            uni->states[api->limit]=uni->state=uiter_getState(iter);
            uni->hasPrevious=FALSE;
            uni->hasNext=iter->hasNext(iter);
        } else {
            /* we already have the beginning of the normalized text */
            api->index=api->start;
        }
        break;
    case UITER_CURRENT:
        break;
    case UITER_LIMIT:
    case UITER_LENGTH:
        /* restart from the end */
        if(uni->hasNext) {
            iter->move(iter, 0, UITER_LIMIT);
            api->start=api->index=api->limit=uni->capacity;
            uni->states[api->limit]=uni->state=uiter_getState(iter);
            uni->hasPrevious=iter->hasPrevious(iter);
            uni->hasNext=FALSE;
        } else {
            /* we already have the end of the normalized text */
            api->index=api->limit;
        }
        break;
    default:
        return -1;  /* Error */
    }

    /* move relative to the current position by delta normalized UChars */
    if(delta==0) {
        /* nothing to do */
    } else if(delta>0) {
        /* go forward until the requested position is in the buffer */
        for(;;) {
            pos=api->index+delta;   /* requested position */
            delta=pos-api->limit;   /* remainder beyond buffered text */
            if(delta<=0) {
                api->index=pos;     /* position reached */
                break;
            }

            /* go to end of buffer and normalize further */
            api->index=api->limit;
            if(!uni->hasNext || !readNext(uni, iter)) {
                break;              /* reached end of text */
            }
        }
    } else /* delta<0 */ {
        /* go backward until the requested position is in the buffer */
        for(;;) {
            pos=api->index+delta;   /* requested position */
            delta=pos-api->start;   /* remainder beyond buffered text */
            if(delta>=0) {
                api->index=pos;     /* position reached */
                break;
            }

            /* go to start of buffer and normalize further */
            api->index=api->start;
            if(!uni->hasPrevious || !readPrevious(uni, iter)) {
                break;              /* reached start of text */
            }
        }
    }

    if(api->index==api->start && !uni->hasPrevious) {
        return 0;
    } else {
        return UITER_UNKNOWN_INDEX;
    }
}

static UBool U_CALLCONV
unormIteratorHasNext(UCharIterator *api) {
    return api->index<api->limit || ((UNormIterator *)api)->hasNext;
}

static UBool U_CALLCONV
unormIteratorHasPrevious(UCharIterator *api) {
    return api->index>api->start || ((UNormIterator *)api)->hasPrevious;
}

static UChar32 U_CALLCONV
unormIteratorCurrent(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index<api->limit ||
        (uni->hasNext && readNext(uni, uni->iter))
    ) {
        return uni->chars[api->index];
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
unormIteratorNext(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index<api->limit ||
        (uni->hasNext && readNext(uni, uni->iter))
    ) {
        return uni->chars[api->index++];
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
unormIteratorPrevious(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index>api->start ||
        (uni->hasPrevious && readPrevious(uni, uni->iter))
    ) {
        return uni->chars[--api->index];
    } else {
        return U_SENTINEL;
    }
}

static uint32_t U_CALLCONV
unormIteratorGetState(const UCharIterator *api) {
    /* not uni->state because that may not be at api->index */
    return ((UNormIterator *)api)->states[api->index];
}

static void U_CALLCONV
unormIteratorSetState(UCharIterator *api, uint32_t state, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        /* do nothing */
    } else if(api==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else if(state==UITER_NO_STATE) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    } else {
        UNormIterator *uni=(UNormIterator *)api;
        UCharIterator *iter=((UNormIterator *)api)->iter;
        if(state!=uni->state) {
            uni->state=state;
            uiter_setState(iter, state, pErrorCode);
        }

        /*
         * Try shortcuts: If the requested state is in the array contents
         * then just set the index there.
         *
         * We assume that the state is unique per position!
         */
        if(state==uni->states[api->index]) {
            return;
        } else if(state==uni->states[api->limit]) {
            api->index=api->limit;
            return;
        } else {
            /* search for the index with this state */
            int32_t i;

            for(i=api->start; i<api->limit; ++i) {
                if(state==uni->states[i]) {
                    api->index=i;
                    return;
                }
            }
        }

        /* there is no array index for this state, reset for fresh contents */
        initIndexes((UNormIterator *)api, iter);
        uni->states[api->limit]=state;
    }
}

static const UCharIterator unormIterator={
    NULL, 0, 0, 0, 0, 0,
    unormIteratorGetIndex,
    unormIteratorMove,
    unormIteratorHasNext,
    unormIteratorHasPrevious,
    unormIteratorCurrent,
    unormIteratorNext,
    unormIteratorPrevious,
    NULL,
    unormIteratorGetState,
    unormIteratorSetState
};

/* Setup functions ---------------------------------------------------------- */

U_CAPI UNormIterator * U_EXPORT2
unorm_openIter(UErrorCode *pErrorCode) {
    UNormIterator *uni;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    /* allocate */
    uni=(UNormIterator *)uprv_malloc(sizeof(UNormIterator));
    if(uni==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    /* initialize */
    uprv_memset(uni, 0, sizeof(UNormIterator));
    uni->chars=uni->charsBuffer;
    uni->states=uni->statesBuffer;
    uni->capacity=initialCapacity;

    /* set a no-op iterator into the api */
    uiter_setString(&uni->api, NULL, 0);
    return uni;
}

U_CAPI void U_EXPORT2
unorm_closeIter(UNormIterator *uni) {
    if(uni!=NULL) {
        if(uni->states!=uni->statesBuffer) {
            /* chars and states are allocated in the same memory block */
            uprv_free(uni->states);
        }
        uprv_free(uni);
    }
}

U_CAPI UCharIterator * U_EXPORT2
unorm_setIter(UNormIterator *uni, UCharIterator *iter, UNormalizationMode mode, UErrorCode *pErrorCode) {
    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(uni==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    if( iter==NULL || iter->getState==NULL || iter->setState==NULL ||
        mode<UNORM_NONE || UNORM_MODE_COUNT<=mode
    ) {
        /* set a no-op iterator into the api */
        uiter_setString(&uni->api, NULL, 0);
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    /* set the iterator and initialize */
    uprv_memcpy(&uni->api, &unormIterator, sizeof(unormIterator));

    uni->iter=iter;
    uni->mode=mode;

    initIndexes(uni, iter);
    uni->states[uni->api.limit]=uni->state=uiter_getState(iter);

    return &uni->api;
}

#endif
