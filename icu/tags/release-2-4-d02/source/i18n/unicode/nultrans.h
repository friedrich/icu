/*
**********************************************************************
*   Copyright (c) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   01/11/2000  aliu        Creation.
**********************************************************************
*/
#ifndef NULTRANS_H
#define NULTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

#ifdef ICU_NULLTRANSLITERATOR_USE_DEPRECATES
U_NAMESPACE_BEGIN

/**
 * A transliterator that leaves text unchanged.
 * @author Alan Liu
 * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
 */
class U_I18N_API NullTransliterator : public Transliterator {

public:

    /**
     * ID for this transliterator.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    static const UChar ID[]; // public for Transliterator

    /**
     * ID for this transliterator.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    static const UChar SHORT_ID[]; // public for Transliterator

    /**
     * Constructs a transliterator.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    NullTransliterator();

    /**
     * Destructor.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual ~NullTransliterator();

    /**
     * Transliterator API.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    Transliterator* clone(void) const;

    /**
     * Implements {@link Transliterator#handleTransliterate}.
     * @obsolete ICU 2.4. Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

private:

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;
};

inline NullTransliterator::NullTransliterator() : Transliterator(ID, 0) {}

inline NullTransliterator::~NullTransliterator() {}

U_NAMESPACE_END
#endif /* ICU_NULLTRANSLITERATOR_USE_DEPRECATES */

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
