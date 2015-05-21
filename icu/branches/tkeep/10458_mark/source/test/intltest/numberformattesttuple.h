/*
*******************************************************************************
* Copyright (C) 1997-2014, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef _NUMBER_FORMAT_TEST_TUPLE
#define _NUMBER_FORMAT_TEST_TUPLE

#include "unicode/utypes.h"
#include "decimalformatpattern.h"
#include "digitaffixesandpadding.h"
#include "unicode/ucurr.h"

#define NFTT_GET_FIELD(tuple, fieldName, defaultValue) ((tuple).fieldName##Flag ? (tuple).fieldName : (defaultValue))

U_NAMESPACE_BEGIN

enum ENumberFormatTestTupleField {
    kLocale,
    kCurrency,
    kPattern,
    kFormat,
    kOutput,
    kComment,
    kMinIntegerDigits,
    kMaxIntegerDigits,
    kMinFractionDigits,
    kMaxFractionDigits,
    kMinGroupingDigits,
    kBreaks,
    kUseSigDigits,
    kMinSigDigits,
    kMaxSigDigits,
    kUseGrouping,
    kMultiplier,
    kRoundingIncrement,
    kFormatWidth,
    kPadCharacter,
    kUseScientific,
    kGrouping,
    kGrouping2,
    kRoundingMode,
    kCurrencyUsage,
    kMinimumExponentDigits,
    kExponentSignAlwaysShown,
    kDecimalSeparatorAlwaysShown,
    kPadPosition,
    kPositivePrefix,
    kPositiveSuffix,
    kNegativePrefix,
    kNegativeSuffix,
    kLocalizedPattern,
    kToPattern,
    kToLocalizedPattern,
    kStyle,
    kParse,
    kLenient,
    kPlural,
    kNumberFormatTestTupleFieldCount,
};

class NumberFormatTestTuple : UMemory {
public:
    Locale locale;
    UnicodeString currency;
    UnicodeString pattern;
    UnicodeString format;
    UnicodeString output;
    UnicodeString comment;
    int32_t minIntegerDigits;
    int32_t maxIntegerDigits;
    int32_t minFractionDigits;
    int32_t maxFractionDigits;
    int32_t minGroupingDigits;
    UnicodeString breaks;
    int32_t useSigDigits;
    int32_t minSigDigits;
    int32_t maxSigDigits;
    int32_t useGrouping;
    int32_t multiplier;
    double roundingIncrement;
    int32_t formatWidth;
    UnicodeString padCharacter;
    int32_t useScientific;
    int32_t grouping;
    int32_t grouping2;
    DigitList::ERoundingMode roundingMode;
    UCurrencyUsage currencyUsage;
    int32_t minimumExponentDigits;
    int32_t exponentSignAlwaysShown;
    int32_t decimalSeparatorAlwaysShown;
    DigitAffixesAndPadding::EPadPosition padPosition;
    UnicodeString positivePrefix;
    UnicodeString positiveSuffix;
    UnicodeString negativePrefix;
    UnicodeString negativeSuffix;
    UnicodeString localizedPattern;
    UnicodeString toPattern;
    UnicodeString toLocalizedPattern;
    UNumberFormatStyle style;
    UnicodeString parse;
    int32_t lenient;
    UnicodeString plural;

    UBool localeFlag;
    UBool currencyFlag;
    UBool patternFlag;
    UBool formatFlag;
    UBool outputFlag;
    UBool commentFlag;
    UBool minIntegerDigitsFlag;
    UBool maxIntegerDigitsFlag;
    UBool minFractionDigitsFlag;
    UBool maxFractionDigitsFlag;
    UBool minGroupingDigitsFlag;
    UBool breaksFlag;
    UBool useSigDigitsFlag;
    UBool minSigDigitsFlag;
    UBool maxSigDigitsFlag;
    UBool useGroupingFlag;
    UBool multiplierFlag;
    UBool roundingIncrementFlag;
    UBool formatWidthFlag;
    UBool padCharacterFlag;
    UBool useScientificFlag;
    UBool groupingFlag;
    UBool grouping2Flag;
    UBool roundingModeFlag;
    UBool currencyUsageFlag;
    UBool minimumExponentDigitsFlag;
    UBool exponentSignAlwaysShownFlag;
    UBool decimalSeparatorAlwaysShownFlag;
    UBool padPositionFlag;
    UBool positivePrefixFlag;
    UBool positiveSuffixFlag;
    UBool negativePrefixFlag;
    UBool negativeSuffixFlag;
    UBool localizedPatternFlag;
    UBool toPatternFlag;
    UBool toLocalizedPatternFlag;
    UBool styleFlag;
    UBool parseFlag;
    UBool lenientFlag;
    UBool pluralFlag;

    NumberFormatTestTuple() {
        clear();
    }
    UBool setField(
            ENumberFormatTestTupleField field,
            const UnicodeString &fieldValue,
            UErrorCode &status);
    UBool clearField(
            ENumberFormatTestTupleField field,
            UErrorCode &status);
    void clear();
    UnicodeString &toString(UnicodeString &appendTo) const;
    static ENumberFormatTestTupleField getFieldByName(const UnicodeString &name);
private:
    const void *getFieldAddress(int32_t fieldId) const;
    void *getMutableFieldAddress(int32_t fieldId);
    void setFlag(int32_t fieldId, UBool value);
    UBool isFlag(int32_t fieldId) const;
};

U_NAMESPACE_END

#endif
