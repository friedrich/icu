/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* sciformatter.h
*
* created on: 2015jan06
* created by: Travis Keep
*/

#ifndef __SCIFORMATTER_H__
#define __SCIFORMATTER_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "digitformatter.h"

U_NAMESPACE_BEGIN

class DecimalFormatSymbols;
class DigitList;
class DigitInterval;
class UnicodeString;
class FieldPositionHandler;
class VisibleDigitsWithExponent;


/**
 * This class formats scientific notation. This class does no rounding.
 */
class U_I18N_API SciFormatter : public UMemory {
public:

/**
 * Use 'E' as the exponent symbol.
 */
SciFormatter();

/**
 * Use symbols to determine what to use for exponent symbol.
 */
SciFormatter(const DecimalFormatSymbols &symbols);

/**
 * Change what to use for exponent symbol.
 */
void setDecimalFormatSymbols(const DecimalFormatSymbols &symbols);

/**
 * formats in scientifc notation.
 * @param positiveMantissa the mantissa to format.
 * @param exponent the exponent to format. May be positive or negative.
 * @param formatter used to format the mantissa
 * @param mantissaInterval controls what part of mantissa gets formatted.
 * @param options formatting options
 * @param handler records field positions.
 * @param appendTo formatted value appended here.
 */
UnicodeString &format(
        const DigitList &positiveMantissa,
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

/**
 * formats in scientifc notation.
 * @param positiveDigits the scientific quantity to format
 * @param formatter used to format the mantissa
 * @param options formatting options
 * @param handler records field positions.
 * @param appendTo formatted value appended here.
 */
UnicodeString &format(
        const VisibleDigitsWithExponent &positiveDigits,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

/**
 * Counts how many code points are needed for the formatting.
 */
int32_t countChar32(
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options) const;

/**
 * Counts how many code points are needed for the formatting.
 */
int32_t countChar32(
        const VisibleDigitsWithExponent &digits,
        const DigitFormatter &formatter,
        const SciFormatterOptions &options) const;


/**
 * Returns TRUE if this object equals rhs.
 */
UBool equals(const SciFormatter &rhs) const {
    return (fExponent == rhs.fExponent);
}

private:
UnicodeString fExponent;
int32_t countChar32(
        const VisibleDigits &exponent,
        const DigitFormatter &formatter,
        const DigitInterval &mantissaInterval,
        const SciFormatterOptions &options) const;
};

U_NAMESPACE_END

#endif  // __SCIFORMATTER_H__
