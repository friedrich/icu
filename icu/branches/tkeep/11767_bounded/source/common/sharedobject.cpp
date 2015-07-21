/*
******************************************************************************
* Copyright (C) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
******************************************************************************
* sharedobject.cpp
*/
#include "sharedobject.h"

U_NAMESPACE_BEGIN

SharedObject::~SharedObject() {}

UnifiedCacheBase::~UnifiedCacheBase() {}

void
SharedObject::addRef(UBool fromWithinCache) const {
    umtx_atomic_inc(&totalRefCount);

    // Although items in use may not be correct immediately, it
    // will be correct eventually.
    if (umtx_atomic_inc(&hardRefCount) == 1 && cachePtr != NULL) {
        if (fromWithinCache) {
            cachePtr->incrementItemsInUse();
        } else {
            cachePtr->incrementItemsInUseWithLocking();
        }
    }
}

void
SharedObject::removeRef(UBool fromWithinCache) const {
    UBool decrementItemsInUse = (umtx_atomic_dec(&hardRefCount) == 0);
    UBool allReferencesGone = (umtx_atomic_dec(&totalRefCount) == 0);

    // Although items in use may not be correct immediately, it
    // will be correct eventually.
    if (decrementItemsInUse && cachePtr != NULL) {
        if (fromWithinCache) {
            cachePtr->decrementItemsInUse();
        } else {
            cachePtr->decrementItemsInUseWithLockingAndEviction();
        }
    }
    if (allReferencesGone) {
        delete this;
    }
}

void
SharedObject::addSoftRef() const {
    umtx_atomic_inc(&totalRefCount);
    umtx_atomic_inc(&softRefCount);
}

void
SharedObject::removeSoftRef() const {
    umtx_atomic_dec(&softRefCount);
    if (umtx_atomic_dec(&totalRefCount) == 0) {
        delete this;
    }
}

UBool
SharedObject::allSoftReferences() const {
    return umtx_loadAcquire(hardRefCount) == 0;
}

UBool
SharedObject::allHardReferences() const {
    return umtx_loadAcquire(softRefCount) == 0;
}
int32_t
SharedObject::getRefCount() const {
    return umtx_loadAcquire(totalRefCount);
}

int32_t
SharedObject::getSoftRefCount() const {
    return umtx_loadAcquire(softRefCount);
}

int32_t
SharedObject::getHardRefCount() const {
    return umtx_loadAcquire(hardRefCount);
}

void
SharedObject::deleteIfZeroRefCount() const {
    if(getRefCount() == 0) {
        delete this;
    }
}

U_NAMESPACE_END
