/*
* Copyright � {1996-1999}, International Business Machines Corporation and others. All Rights Reserved.
*****************************************************************************************
*/
//===============================================================================
//
// File sortkey.h
//
// 
//
// Created by: Helena Shih
//
// Modification History:
//
//  Date         Name          Description
//
//  6/20/97     helena      Java class name change.
//  8/18/97     helena      Added internal API documentation.
//  6/26/98     erm         Changed to use byte arrays and memcmp. 
//===============================================================================

#ifndef SORTKEY_H
#define SORTKEY_H


#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/coll.h"

/* forward declaration */
class RuleBasedCollator;

/**
 * Collation keys are generated by the Collator class.  Use the CollationKey objects
 * instead of Collator to compare strings multiple times.  A CollationKey
 * preprocesses the comparison information from the Collator object to
 * make the comparison faster.  If you are not going to comparing strings
 * multiple times, then using the Collator object is generally faster,
 * since it only processes as much of the string as needed to make a
 * comparison.
 * <p> For example (with strength == tertiary)
 * <p>When comparing "Abernathy" to "Baggins-Smythworthy", Collator
 * only needs to process a couple of characters, while a comparison
 * with CollationKeys will process all of the characters.  On the other hand,
 * if you are doing a sort of a number of fields, it is much faster to use
 * CollationKeys, since you will be comparing strings multiple times.
 * <p>Typical use of CollationKeys are in databases, where you store a CollationKey
 * in a hidden field, and use it for sorting or indexing.
 *
 * <p>Example of use:
 * <pre>
 * .    UErrorCode success = U_ZERO_ERROR;
 * .    Collator* myCollator = Collator::createInstance(success);
 * .    CollationKey* keys = new CollationKey [3];
 * .    myCollator->getCollationKey("Tom", keys[0], success );
 * .    myCollator->getCollationKey("Dick", keys[1], success );
 * .    myCollator->getCollationKey("Harry", keys[2], success );
 * . 
 * .    // Inside body of sort routine, compare keys this way:
 * .    CollationKey tmp;
 * .    if(keys[0].compareTo( keys[1] ) > 0 ) {
 * .        tmp = keys[0]; keys[0] = keys[1]; keys[1] = tmp;
 * .    }
 * .    //...
 * </pre>
 * <p>Because Collator::compare()'s algorithm is complex, it is faster to sort
 * long lists of words by retrieving collation keys with Collator::getCollationKey().
 * You can then cache the collation keys and compare them using CollationKey::compareTo().
 * <p>
 * <strong>Note:</strong> <code>Collator</code>s with different Locale,
 * CollationStrength and DecompositionMode settings will return different
 * CollationKeys for the same set of strings. Locales have specific 
 * collation rules, and the way in which secondary and tertiary differences 
 * are taken into account, for example, will result in different CollationKeys
 * for same strings.
 * <p>

 * @see          Collator
 * @see          RuleBasedCollator
 * @version      1.3 12/18/96
 * @author       Helena Shih
 * @deprecated
 */
class U_I18N_API CollationKey {
public:
    /**
     * This creates an empty collation key based on the null string.  An empty 
     * collation key contains no sorting information.  When comparing two empty
     * collation keys, the result is Collator::EQUAL.  Comparing empty collation key
     * with non-empty collation key is always Collator::LESS.
	 * @deprecated
     */
     CollationKey();
    /**
     * Creates a collation key based on the collation key values.  
     * @param values the collation key values
     * @param count number of collation key values, including trailing nulls.
     * @see #createBits
	 * @deprecated
     */
     CollationKey(const  uint8_t*    values,
					int32_t     count);

    /**
     * Copy constructor.
	 * @deprecated
     */
     CollationKey(const CollationKey& other);
    /** 
     * Sort key destructor.
	 * @deprecated
     */
     ~CollationKey();

    /**
     * Assignment operator
	 * @deprecated
     */
    const   CollationKey&           operator=(const CollationKey& other);

    /**
     * Compare if two collation keys are the same.
     * @param source the collation key to compare to.
     * @return Returns true if two collation keys are equal, false otherwise.
	 * @deprecated
     */
     UBool                  operator==(const CollationKey& source) const;

    /**
     * Compare if two collation keys are not the same.
     * @param source the collation key to compare to.
     * @return Returns TRUE if two collation keys are different, FALSE otherwise.
	 * @deprecated
     */
     UBool                  operator!=(const CollationKey& source) const;


    /**
     * Test to see if the key is in an invalid state. The key will be in an
     * invalid state if it couldn't allocate memory for some operation.
     * @return Returns TRUE if the key is in an invalid, FALSE otherwise.
	 * @deprecated
     */
     UBool                  isBogus(void) const;

    /** 
     * Returns a pointer to the collation key values. The storage is owned
     * by the collation key and the pointer will become invalid if the key
     * is deleted.
     * @param count the output parameter of number of collation key values,
     * including any trailing nulls.
	 * @deprecated
     */
	 const    uint8_t*                getByteArray(int32_t& count) const;

    /** 
     * Extracts the collation key values into a new array. The caller owns
     * this storage and should free it.
     * @param count the output parameter of number of collation key values,
     * including any trailing nulls.
	 * @deprecated
     */
     uint8_t*                toByteArray(int32_t& count) const;

    /**
     * Convenience method which does a string(bit-wise) comparison of the
     * two collation keys.
     * @param sourceKey source collation key
     * @param targetKey target collation key
     * @return Returns Collator::LESS if sourceKey &lt; targetKey, 
     * Collator::GREATER if sourceKey > targetKey and Collator::EQUAL
     * otherwise.
	 * @deprecated
     */
     Collator::EComparisonResult    compareTo(const CollationKey& target) const;

    /**
     * Creates an integer that is unique to the collation key.  NOTE: this
     * is not the same as String.hashCode.
     * <p>Example of use:
     * <pre>
     * .    UErrorCode status = U_ZERO_ERROR;
     * .    Collator *myCollation = Collator::createInstance(Locale::US, status);
     * .    if (U_FAILURE(status)) return;
     * .    CollationKey key1, key2;
     * .    UErrorCode status1 = U_ZERO_ERROR, status2 = U_ZERO_ERROR;
     * .    myCollation->getCollationKey("abc", key1, status1);
     * .    if (U_FAILURE(status1)) { delete myCollation; return; }
     * .    myCollation->getCollationKey("ABC", key2, status2);
     * .    if (U_FAILURE(status2)) { delete myCollation; return; }
     * .    // key1.hashCode() != key2.hashCode()
     * </pre>
     * @return the hash value based on the string's collation order.
     * @see UnicodeString#hashCode
	 * @deprecated
     */
     int32_t                 hashCode(void) const;

private:
    /**
    * Returns an array of the collation key values as 16-bit integers.
    * The caller owns the storage and must delete it.
    * @param size output parameter of the number of collation key values
    * @return a pointer to an array of 16-bit collation key values.
    */
            uint16_t*               copyValues(int32_t &size) const;

    /*
     * Creates a collation key with a string.
     */
                                    CollationKey(const UnicodeString& value);

            int32_t                 storeBytes(int32_t cursor, uint32_t byteValue);
            int32_t                    storeUnicodeString(int32_t cursor, const UnicodeString &value);
            void                    reverseBytes(int32_t from, int32_t to);
            CollationKey&           ensureCapacity(int32_t newSize);
            CollationKey&           copyUnicodeString(const UnicodeString &value);
            CollationKey&           setToBogus(void);
            CollationKey&           reset(void);

    friend  class                   RuleBasedCollator;

    static const int32_t            kInvalidHashCode;
    static const int32_t            kEmptyHashCode;

            UBool                  fBogus;
            int32_t                 fCount;
            int32_t                 fCapacity;
            int32_t                 fHashCode;
            uint8_t*                fBytes;
};

inline UBool
CollationKey::operator!=(const CollationKey& other) const
{
    return !(*this == other);
}

inline UBool
CollationKey::isBogus() const
{
    return fBogus;
}

inline const uint8_t*
CollationKey::getByteArray(int32_t &count) const
{
    count = fCount;
    return fBytes;
}

inline UTextOffset
CollationKey::storeBytes(UTextOffset cursor, uint32_t byteValue)
{
    fBytes[cursor++] = (uint8_t) (byteValue >> 8);
    fBytes[cursor++] = (uint8_t) byteValue;

    return cursor;
}

#endif
