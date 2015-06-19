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

class PluralMapBase : public UMemory {
public:
    enum Variant {
        NONE = -1,
        OTHER,
        ZERO,
        ONE,
        TWO,
        FEW,
        MANY,
        VARIANT_COUNT
    };

    /**
     * Converts a variant name to a variant.
     * Returns NONE for bad variant name.
     */
    static Variant toVariant(const char *variantName);

    /**
     * Converts a variant name to a variant.
     * Returns NONE for bad variant name.
     */
    static Variant toVariant(const UnicodeString &variantName);

    /**
     * Converts a variant to a name.
     * Passing NONE or VARIANT_COUNT for variant returns NULL.
     */
    static const char *getVariantName(Variant variant);
};

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
class PluralMap : public PluralMapBase {
public:
    /**
     * Other variant is mapped to a copy of the default value.
     */
    PluralMap() : fOtherVariant() {
        initializeNew();
    }

    /**
     * Other variant is mapped to otherVariant.
     */
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

    /**
     * Removes all mappings and makes 'other' point to the default value.
     */
    void clear() {
        *fVariants[0] = T();
        for (int32_t i = 1; i < UPRV_LENGTHOF(fVariants); ++i) {
            delete fVariants[i];
            fVariants[i] = NULL;
        }
    }

    /**
     * Iterates through the mappings in this instance, set index to NONE
     * prior to using. Call next repeatedly to get the values until it
     * returns NULL. Each time next returns, caller may pass index
     * to pluralMap_getName() to get the name of the plural variant.
     * When this function returns NULL, index is VARIANT_COUNT
     */
    const T *next(Variant &index) const {
        int32_t idx = index;
        ++idx;
        for (; idx < UPRV_LENGTHOF(fVariants); ++idx) {
            if (fVariants[idx] != NULL) {
                index = static_cast<Variant>(idx);
                return fVariants[idx];
            }
        }
        index = static_cast<Variant>(idx);
        return NULL;
    }

    /**
     * non const version of next.
     */
    T *nextMutable(Variant &index) {
        const T *result = next(index);
        return const_cast<T *>(result);
    }

    /**
     * Returns the 'other' variant value.
     */
    const T &getOther() const {
        return get(OTHER);
    }

    /**
     * Returns the value associated with a variant.
     * If no value found, or v is NONE or VARIANT_COUNT, falls
     * back to returning the 'other' variant value.
     */
    const T &get(Variant v) const {
        int32_t index = v;
        if (index < 0 || index >= UPRV_LENGTHOF(fVariants) || fVariants[index] == NULL) {
            return *fVariants[0];
        }
        return *fVariants[index];
    }

    /**
     * Convenience routine to get the variant value by name. Otherwise
     * works just like get(Variant).
     */
    const T &get(const char *variant) const {
        return get(toVariant(variant));
    }

    /**
     * Convenience routine to get the variant value by name as a
     * UnicodeString. Otherwise works just like get(Variant).
     */
    const T &get(const UnicodeString &variant) const {
        return get(toVariant(variant));
    }

    /**
     * Returns a pointer to the variant value that caller can
     * freely modify. If it was defaulting to the 'other'
     * variant value because no explicit value was stored,
     * it stores a copy of the default value at the returned pointer.
     *
     * @param index the variant caller wishes to change.
     * @param status error returned here if index is NONE or VARIANT_COUNT
     *  or memory could not be allocated, or any other error happens.
     */
    T *getMutable(
            Variant v,
            UErrorCode &status) {
        return getMutable(v, NULL, status);
    }

    /**
     * Convenience routine to get a pointer to a variant value by name.
     * Otherwise works just like getMutable(Variant, UErrorCode &).
     * reports an error if the variant name is invalid.
     */
    T *getMutable(
            const char *variant,
            UErrorCode &status) {
        return getMutable(toVariant(variant), NULL, status);
    }

    /**
     * Just like getMutable(Variant, UErrorCode &) but copies defaultValue to
     * returned pointer if it was defaulting to the 'other' variant
     * before because no explicit value was stored.
     */
    T *getMutableWithDefault(
            Variant v,
            const T &defaultValue,
            UErrorCode &status) {
        return getMutable(v, &defaultValue, status);
    }

    /**
     * Returns TRUE if this object equals rhs.
     */
    UBool equals(
            const PluralMap<T> &rhs,
            UBool (*eqFunc)(const T &, const T &)) const {
        for (int32_t i = 0; i < UPRV_LENGTHOF(fVariants); ++i) {
            if (fVariants[i] == rhs.fVariants[i]) {
                continue;
            }
            if (fVariants[i] == NULL || rhs.fVariants[i] == NULL) {
                return FALSE;
            }
            if (!eqFunc(*fVariants[i], *rhs.fVariants[i])) {
                return FALSE;
            }
        }
        return TRUE;
    }

private:
    T fOtherVariant;
    T* fVariants[6];

    T *getMutable(
            Variant v,
            const T *defaultValue,
            UErrorCode &status) {
        if (U_FAILURE(status)) {
            return NULL;
        }
        int32_t index = v;
        if (index < 0 || index >= UPRV_LENGTHOF(fVariants)) {
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
