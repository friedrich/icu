/*
**********************************************************************
*   Copyright (C) 1999-2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/
#ifndef CPDTRANS_H
#define CPDTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

#ifdef ICU_COMPOUNDTRANSLITERATOR_USE_DEPRECATES

U_NAMESPACE_BEGIN

class U_I18N_API UVector;
class TransliteratorRegistry;

/**
 * A transliterator that is composed of two or more other
 * transliterator objects linked together.  For example, if one
 * transliterator transliterates from script A to script B, and
 * another transliterates from script B to script C, the two may be
 * combined to form a new transliterator from A to C.
 *
 * <p>Composed transliterators may not behave as expected.  For
 * example, inverses may not combine to form the identity
 * transliterator.  See the class documentation for {@link
 * Transliterator} for details.
 *
 * @author Alan Liu
 * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
 */
class U_I18N_API CompoundTransliterator : public Transliterator {

    Transliterator** trans;

    int32_t count;

    /**
     * For compound RBTs (those with an ::id block before and/or after
     * the main rule block) we record the index of the RBT here.
     * Otherwise, this should have a value of -1.  We need this
     * information to implement toRules().
     */
    int32_t compoundRBTIndex;

public:

    /**
     * Constructs a new compound transliterator given an array of
     * transliterators.  The array of transliterators may be of any
     * length, including zero or one, however, useful compound
     * transliterators have at least two components.
     * @param transliterators array of <code>Transliterator</code>
     * objects
     * @param transliteratorCount The number of
     * <code>Transliterator</code> objects in transliterators.
     * @param adoptedFilter the filter.  Any character for which
     * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(Transliterator* const transliterators[],
                           int32_t transliteratorCount,
                           UnicodeFilter* adoptedFilter = 0);

    /**
     * Constructs a new compound transliterator.
     * @param id compound ID
     * @param dir either UTRANS_FORWARD or UTRANS_REVERSE
     * @param adoptedFilter a global filter for this compound transliterator
     * or NULL
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UTransDirection dir,
                           UnicodeFilter* adoptedFilter,
                           UParseError& parseError,
                           UErrorCode& status);

    /**
     * Constructs a new compound transliterator in the FORWARD
     * direction with a NULL filter.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UParseError& parseError,
                           UErrorCode& status);
    /**
     * Destructor.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual ~CompoundTransliterator();

    /**
     * Copy constructor.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const CompoundTransliterator&);

    /**
     * Assignment operator.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator& operator=(const CompoundTransliterator&);

    /**
     * Transliterator API.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    Transliterator* clone(void) const;

    /**
     * Returns the number of transliterators in this chain.
     * @return number of transliterators in this chain.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual int32_t getCount(void) const;

    /**
     * Returns the transliterator at the given index in this chain.
     * @param index index into chain, from 0 to <code>getCount() - 1</code>
     * @return transliterator at the given index
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual const Transliterator& getTransliterator(int32_t index) const;

    /**
     * Sets the transliterators.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    void setTransliterators(Transliterator* const transliterators[],
                            int32_t count);

    /**
     * Adopts the transliterators.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    void adoptTransliterators(Transliterator* adoptedTransliterators[],
                              int32_t count);

    /**
     * Override Transliterator:
     * Create a rule string that can be passed to createFromRules()
     * to recreate this transliterator.
     * @param result the string to receive the rules.  Previous
     * contents will be deleted.
     * @param escapeUnprintable if TRUE then convert unprintable
     * character to their hex escape representations, \uxxxx or
     * \Uxxxxxxxx.  Unprintable characters are those other than
     * U+000A, U+0020..U+007E.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

 protected:
    /**
     * Implement Transliterator framework
     */
    virtual void handleGetSourceSet(UnicodeSet& result) const;

 public:
    /**
     * Override Transliterator framework
     */
    virtual UnicodeSet& getTargetSet(UnicodeSet& result) const;

// handleTransliterate should be protected, but was declared public before ICU 2.2.
// We do not have a separate deprecation date for this method since the entire class
// will become internal after 2002-sep-30.
#ifndef U_USE_DEPRECATED_TRANSLITERATOR_API
 protected:
#endif
    /**
     * Implements {@link Transliterator#handleTransliterate}.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual void handleTransliterate(Replaceable& text, UTransPosition& index,
                                     UBool incremental) const;

#ifndef U_USE_DEPRECATED_TRANSLITERATOR_API
 public:
#endif

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual inline UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static inline UClassID getStaticClassID();

private:

    friend class Transliterator;
    friend class TransliteratorAlias; // to access private ct

    /**
     * Private constructor for compound RBTs.  Construct a compound
     * transliterator using the given idBlock, with the adoptedTrans
     * inserted at the idSplitPoint.
     */
    CompoundTransliterator(const UnicodeString& ID,
                           const UnicodeString& idBlock,
                           int32_t idSplitPoint,
                           Transliterator *adoptedTrans,
                           UErrorCode& status);
                           
    /**
     * Private constructor for Transliterator.
     */
    CompoundTransliterator(UVector& list,
                           UParseError& parseError,
                           UErrorCode& status);

    void init(const UnicodeString& id,
              UTransDirection direction,
              int32_t idSplitPoint,
              Transliterator *adoptedRbt,
              UBool fixReverseID,
              UErrorCode& status);

    void init(UVector& list,
              UTransDirection direction,
              UBool fixReverseID,
              UErrorCode& status);

    /**
     * Return the IDs of the given list of transliterators, concatenated
     * with ';' delimiting them.  Equivalent to the perlish expression
     * join(';', map($_.getID(), transliterators).
     */
    UnicodeString joinIDs(Transliterator* const transliterators[],
                          int32_t transCount);

    void freeTransliterators(void);

    void computeMaximumContextLength(void);

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

#ifdef U_USE_DEPRECATED_TRANSLITERATOR_API

public:

    /**
     * Constructs a new compound transliterator.
     * Use Transliterator::createInstance factory method.
     * @param filter the filter.  Any character for which
     * <tt>filter.isIn()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UTransDirection dir,
                           UnicodeFilter* adoptedFilter,
                           UErrorCode& status);

    /**
     * Constructs a new compound transliterator in the FORWARD
     * direction with a NULL filter.
     * Use Transliterator::createInstance factory method.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UErrorCode& status);

#endif
};

inline UClassID
CompoundTransliterator::getStaticClassID()
{ return (UClassID)&fgClassID; }

inline UClassID
CompoundTransliterator::getDynamicClassID() const
{ return CompoundTransliterator::getStaticClassID(); }

/**
 * Definitions for obsolete API
 * TODO: Remove after Aug 2002
 */

#ifdef U_USE_DEPRECATED_TRANSLITERATOR_API

inline CompoundTransliterator::CompoundTransliterator( const UnicodeString& id,
                                                       UTransDirection dir,
                                                       UnicodeFilter* adoptedFilter,
                                                       UErrorCode& status):
                                            Transliterator(id, adoptedFilter),
                                            trans(0), compoundRBTIndex(-1) {
    UParseError parseError;
    init(id, dir, -1, 0, TRUE,parseError,status);
}

inline CompoundTransliterator::CompoundTransliterator(const UnicodeString& id,
                                                      UErrorCode& status) :
                                            Transliterator(id, 0), // set filter to 0 here!
                                            trans(0), compoundRBTIndex(-1) {
    UParseError parseError;
    init(id, UTRANS_FORWARD, -1, 0, TRUE,parseError,status);       
}

#endif

U_NAMESPACE_END
#endif /* ICU_COMPOUNDTRANSLITERATOR_USE_DEPRECATES */

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
