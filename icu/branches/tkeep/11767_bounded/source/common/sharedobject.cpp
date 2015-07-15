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


void
SharedObject::addRef() const {
    addRef(incrementItemsInUseWithLocking, cacheContext);
}

void
SharedObject::addRefWhileHoldingCacheLock() const {
    addRef(incrementItemsInUse, cacheContext);
}


void
SharedObject::addRef(
        void (*addFunc)(const void *), const void *context) const {
    umtx_atomic_inc(&totalRefCount);

    // Although items in use may not be correct immediately, it
    // will be correct eventually.
    if (umtx_atomic_inc(&hardRefCount) == 1 && addFunc != NULL) {
        addFunc(context);
    }
}

void
SharedObject::removeRef() const {
    removeRef(decrementItemsInUseWithLockingAndEviction, cacheContext);
}

void
SharedObject::removeRefWhileHoldingCacheLock() const {
    removeRef(decrementItemsInUse, cacheContext);
}

void
SharedObject::removeRef(
        void (*removeFunc)(const void *), const void *context) const {
    UBool decrimentItemsInUse = (umtx_atomic_dec(&hardRefCount) == 0);
    UBool allReferencesGone = (umtx_atomic_dec(&totalRefCount) == 0);

    // Although items in use may not be correct immediately, it
    // will be correct eventually.
    if (decrimentItemsInUse && removeFunc != NULL) {
        removeFunc(context);
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
