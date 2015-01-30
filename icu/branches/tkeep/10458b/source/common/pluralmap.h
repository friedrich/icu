/*
******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and
* others. All Rights Reserved.
******************************************************************************
*
* File PLURALMAP.H - The ICU Unified cache.
******************************************************************************
*/

#ifndef __PLURAL_MAP_H__
#define __PLURAL_MAP_H__

#include "cmemory.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

class UnicodeString;

extern int32_t pluralMap_getIndex(const char *variantName);
extern int32_t pluralMap_getIndexByUniStr(const UnicodeString &variantName);
extern const char *pluralMap_getName(int32_t index);

template<typename T>
class PluralMap : public UMemory {
public:
    PluralMap() : fOtherVariant() {
        initializeNew();
    }

    PluralMap(const T &otherVariant) : fOtherVariant(otherVariant) {
        initializeNew();

    }

    PluralMap(const PluralMap<T> &other) : fOtherVariant(other.fOtherVariant) {
        fVariants[0] = &fOtherVariant;
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            fVariants[i] = other.fVariants[i] ?
                    new T(*other.fVariants[i]) : NULL;
        }
    }

    PluralMap<T> &operator=(const PluralMap<T> &other) {
        if (this == &other) {
            return *this;
        }
        for (int32_t i = 0; i < UPRV_LENGTHOF(fVariants); ++i) {
            if (fVariants[i] != NULL && other.fVariants[i] != NULL) {
                *fVariants[i] = *other.fVariants[i];
            } else if (fVariants[i] != NULL) {
                delete fVariants[i];
                fVariants[i] = NULL;
            } else if (other.fVariants[i] != NULL) {
                fVariants[i] = new T(*other.fVariants[i]);
            } else {
                // do nothing
            }
        }
        return *this;
    }

    ~PluralMap() {
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            delete fVariants[i];
        }
    }

    void reset() {
        *fVariants[0] = T();
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            delete fVariants[i];
            fVariants[i] = NULL;
        }
    }

    const T *next(int32_t &index) const {
        ++index;
        for (; index < UPRV_LENGTHOF(fVariants); ++index) {
            if (fVariants[index] != NULL) {
                return fVariants[index];
            }
        }
        return NULL;
    }

    T *nextMutable(int32_t &index) {
        const T *result = next(index);
        return const_cast<T *>(result);
    }

    const T &getOther() const {
        return get(0);
    }

    const T &get(int32_t index) const {
        if (index < 0 || index > UPRV_LENGTHOF(fVariants) || fVariants[index] == NULL) {
            return *fVariants[0];
        }
        return *fVariants[index];
    }

    const T &get(const char *variant) const {
        return get(pluralMap_getIndex(variant));
    }

    const T &getByUniStr(const UnicodeString &variant) const {
        return get(pluralMap_getIndexByUniStr(variant));
    }

    T *getMutable(
            int32_t index,
            UErrorCode &status) {
        return getMutable(index, NULL, status);
    }

    T *getMutable(
            const char *variant,
            UErrorCode &status) {
        return getMutable(pluralMap_getIndex(variant), NULL, status);
    }

    T *getMutableWithDefault(
            int32_t index,
            const T &defaultValue,
            UErrorCode &status) {
        return getMutable(index, &defaultValue, status);
    }

private:
    T fOtherVariant;
    T* fVariants[6];

    T *getMutable(
            int32_t index,
            const T *defaultValue,
            UErrorCode &status) {
        if (U_FAILURE(status)) {
            return NULL;
        }
        if (index < 0 || index > UPRV_LENGTHOF(fVariants)) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }
        if (fVariants[index] == NULL) {
            fVariants[index] = defaultValue == NULL ?
                    new T() : new T(*defaultValue);
        }
        if (!fVariants[index]) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return fVariants[index];
    }

    void initializeNew() {
        fVariants[0] = &fOtherVariant;
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            fVariants[i] = NULL;
        }
    }
};

U_NAMESPACE_END

#endif
