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

/**
 * Converts a variant name to an index.
 */
extern int32_t pluralMap_getIndex(const char *variantName);

/**
 * Converts a variant name to an index.
 */
extern int32_t pluralMap_getIndexByUniStr(const UnicodeString &variantName);

/**
 * Converts a variant index to a name.
 */
extern const char *pluralMap_getName(int32_t index);

/**
 * A Map of plural variants to values. It maintains ownership of the
 * values.
 *
 * Type T is the value type. T must provide the followng:
 * 1) Default constructor
 * 2) Copy constructor
 * 3) Assignment operator
 * 4) Must extend UMemory
 */
template<typename T>
class PluralMap : public UMemory {
public:

    /**
     * Other variant is mapped to a copy of the default value.
     */
    PluralMap() : fOtherVariant() {
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

    /**
     * Removes all mappings and makes 'other' point to the default value.
     */
    void reset() {
        *fVariants[0] = T();
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            delete fVariants[i];
            fVariants[i] = NULL;
        }
    }

    /**
     * Iterates through the mappings in this instance, set index to -1
     * prior to using. Call next repeatedly to get the values until it
     * returns NULL. Each time next returns, caller may pass index
     * to pluralMap_getName() to get the name of the plural variant.
     */
    const T *next(int32_t &index) const {
        ++index;
        for (; index < UPRV_LENGTHOF(fVariants); ++index) {
            if (fVariants[index] != NULL) {
                return fVariants[index];
            }
        }
        return NULL;
    }

    /**
     * non const version of next.
     */
    T *nextMutable(int32_t &index) {
        const T *result = next(index);
        return const_cast<T *>(result);
    }

    /**
     * Returns the 'other' variant value.
     */
    const T &getOther() const {
        return get(0);
    }

    /**
     * Returns the variant value by index.
     * Use pluralMap_getIndex to find the index by variant name.
     * If no variant value found, or index is out of range, falls
     * back to returning the 'other' variant value.
     */
    const T &get(int32_t index) const {
        if (index < 0 || index > UPRV_LENGTHOF(fVariants) || fVariants[index] == NULL) {
            return *fVariants[0];
        }
        return *fVariants[index];
    }

    /**
     * Convenience routine to get the variant value by name. Otherwise
     * works just like get(int32_t).
     */
    const T &get(const char *variant) const {
        return get(pluralMap_getIndex(variant));
    }

    /**
     * Convenience routine to get the variant value by name as a
     * UnicodeString. Otherwise works just like get(int32_t).
     */
    const T &getByUniStr(const UnicodeString &variant) const {
        return get(pluralMap_getIndexByUniStr(variant));
    }

    /**
     * Returns a pointer to the variant value that caller can
     * freely modify. If it was defaulting to the ‘other’
     * variant value because
     * no explicit value was stored, it stores a copy of the
     * default value at the returned pointer.

     *
     * @param index the variant index caller wishes to change.
     *   Use pluralMap_getIndex to find the index by variant name.
     * @param status error returned here if index is out of range or
     *  memory could not be allocated, or any other error happens.
     */
    T *getMutable(
            int32_t index,
            UErrorCode &status) {
        return getMutable(index, NULL, status);
    }

    /**
     * Convenience routine to get a pointer to a variant value by name.
     * Otherwise works just like getMutable(int32_t, UErrorCode &).
     * reports an error if the variant name is invalid.
     */
    T *getMutable(
            const char *variant,
            UErrorCode &status) {
        return getMutable(pluralMap_getIndex(variant), NULL, status);
    }

    /**
     * Just like get(int32_t, UErrorCode &) but copies defaultValue to
     * returned pointer if it was defaulting to the 'other' variant
     * before because no explicit value was stored.
     */
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
