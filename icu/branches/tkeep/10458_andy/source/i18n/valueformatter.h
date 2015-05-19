/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#ifndef VALUEFORMATTER_H
#define VALUEFORMATTER_H

#if !UCONFIG_NO_FORMATTING

#include "unicode/utypes.h"

#include "unicode/uobject.h"


U_NAMESPACE_BEGIN

class UnicodeString;
class FieldPositionHandler;
class DigitGrouping;
class PluralRules;
class FixedPrecision;
class DigitFormatter;
class DigitFormatterOptions;
class ScientificPrecision;
class SciFormatter;
class SciFormatterOptions;
class NumericValue;
class DigitList;


/**
 * A closure around rounding and formatting a value. As these instances are
 * designed to be short lived (they only exist while formatting a value), they
 * do not own their own attributes. Rather the caller maintains ownership of
 * all attributes. A caller first calls a prepareXXX method on an instance
 * to share its data before using that instance. Using an
 * instance without first calling a prepareXXX method results in an
 * assertion error and a program crash.
 */
class ValueFormatter : public UObject {
public:
    ValueFormatter() : fType(kFormatTypeCount) {
    }

    NumericValue &initNumericValue(
            const DigitList &digitList,
            NumericValue &value,
            UErrorCode &status) const;

    /**
     * Returns TRUE if the absolute value of value can be fast formatted
     * using ValueFormatter::formatInt32.
     */
    UBool isFastFormattable(int32_t value) const;

    /**
     * formats positiveValue and appends to appendTo. Returns appendTo.
     * @param positiveValue must be positive. May be positive infinity or NaN.
     * @param handler stores the field positions
     * @param appendTo formatted value appended here.
     */
    UnicodeString &format(
        const NumericValue &value,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

    /**
     * formats positiveValue and appends to appendTo. Returns appendTo.
     * value must be positive. Calling formatInt32 to format a value when
     * isFastFormattable indicates that the value cannot be fast formatted
     * results in undefined behavior.
     */
    UnicodeString &formatInt32(
        int32_t positiveValue,
        FieldPositionHandler &handler,
        UnicodeString &appendTo) const;

    /**
     * Returns the number of code points needed to format.
     * @param positiveValue must be positive. May be positive infinity or NaN.
     */
    int32_t countChar32(const NumericValue &value) const;
  
    /**
     * Prepares this instance for fixed decimal formatting.
     */
    void prepareFixedDecimalFormatting(
        const DigitFormatter &formatter,
        const DigitGrouping &grouping,
        const FixedPrecision &precision,
        const DigitFormatterOptions &options);
    /**
     * Prepares this instance for scientific formatting.
     */
    void prepareScientificFormatting(
        const SciFormatter &sciformatter,
        const DigitFormatter &formatter,
        const ScientificPrecision &precision,
        const SciFormatterOptions &options);
private:
    ValueFormatter(const ValueFormatter &);
    ValueFormatter &operator=(const ValueFormatter &);
    enum FormatType {
        kFixedDecimal,
        kScientificNotation,
        kFormatTypeCount
    };

    FormatType fType;

    // for fixed decimal and scientific formatting
    const DigitFormatter *fDigitFormatter;

    // for fixed decimal formatting
    const FixedPrecision *fFixedPrecision;
    const DigitFormatterOptions *fFixedOptions;
    const DigitGrouping *fGrouping;

    // for scientific formatting
    const SciFormatter *fSciFormatter;
    const ScientificPrecision *fScientificPrecision;
    const SciFormatterOptions *fScientificOptions;
};

U_NAMESPACE_END

#endif /* !UCONFIG_NO_FORMATTING */

#endif /* VALUEFORMATTER_H */
