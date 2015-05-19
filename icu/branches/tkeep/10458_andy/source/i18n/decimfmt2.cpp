/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: decimfmt2.cpp
 */

#include "decimfmt2.h"
#include "unicode/plurrule.h"
#include "unicode/ustring.h"
#include "decimalformatpattern.h"
#include "decimalformatpatternimpl.h"
#include "valueformatter.h"
#include "fphdlimp.h"
#include "numericvalue.h"

U_NAMESPACE_BEGIN

static const int32_t kMaxScientificIntegerDigits = 8;

static const int32_t kFormattingPosPrefix = (1 << 0);
static const int32_t kFormattingNegPrefix = (1 << 1);
static const int32_t kFormattingPosSuffix = (1 << 2);
static const int32_t kFormattingNegSuffix = (1 << 3);
static const int32_t kFormattingSymbols = (1 << 4);
static const int32_t kFormattingCurrency = (1 << 5);
static const int32_t kFormattingUsesCurrency = (1 << 6);
static const int32_t kFormattingPluralRules = (1 << 7);
static const int32_t kFormattingAffixParser = (1 << 8);
static const int32_t kFormattingCurrencyAffixInfo = (1 << 9);
static const int32_t kFormattingAll = (1 << 10) - 1;
static const int32_t kFormattingAffixes =
        kFormattingPosPrefix | kFormattingPosSuffix |
        kFormattingNegPrefix | kFormattingNegSuffix;
static const int32_t kFormattingAffixParserWithCurrency =
        kFormattingAffixParser | kFormattingCurrencyAffixInfo;

DecimalFormat2::DecimalFormat2(
        const Locale &locale,
        const UnicodeString &pattern,
        UErrorCode &status)
        : fRoundingMode(DigitList::kRoundHalfEven),
          fSymbols(NULL),
          fCurrencyUsage(UCURR_USAGE_STANDARD),
          fRules(NULL),
          fMonetary(FALSE) {
    if (U_FAILURE(status)) {
        return;
    }
    fSymbols = new DecimalFormatSymbols(
            locale, status);
    if (fSymbols == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    UParseError parseError;
    fCurr[0] = 0;
    applyPattern(pattern, FALSE, parseError, status);
    updateAll(status);
}

DecimalFormat2::DecimalFormat2(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status)
        : fRoundingMode(DigitList::kRoundHalfEven),
          fSymbols(symbolsToAdopt),
          fCurrencyUsage(UCURR_USAGE_STANDARD),
          fRules(NULL),
          fMonetary(FALSE) {
    fCurr[0] = 0;
    applyPattern(pattern, FALSE, parseError, status);
    updateAll(status);
}

DecimalFormat2::DecimalFormat2(const DecimalFormat2 &other) :
          UObject(other),
          fMultiplier(other.fMultiplier),
          fRoundingMode(other.fRoundingMode),
          fMinIntDigits(other.fMinIntDigits),
          fMaxIntDigits(other.fMaxIntDigits),
          fMinFracDigits(other.fMinFracDigits),
          fMaxFracDigits(other.fMaxFracDigits),
          fMinSigDigits(other.fMinSigDigits),
          fMaxSigDigits(other.fMaxSigDigits),
          fUseScientific(other.fUseScientific),
          fUseSigDigits(other.fUseSigDigits),
          fGrouping(other.fGrouping),
          fUseGrouping(other.fUseGrouping),
          fPositivePrefixPattern(other.fPositivePrefixPattern),
          fNegativePrefixPattern(other.fNegativePrefixPattern),
          fPositiveSuffixPattern(other.fPositiveSuffixPattern),
          fNegativeSuffixPattern(other.fNegativeSuffixPattern),
          fSymbols(other.fSymbols),
          fCurrencyUsage(other.fCurrencyUsage),
          fRules(other.fRules),
          fMonetary(other.fMonetary),
          fAffixParser(other.fAffixParser),
          fCurrencyAffixInfo(other.fCurrencyAffixInfo),
          fEffPrecision(other.fEffPrecision),
          fEffGrouping(other.fEffGrouping),
          fOptions(other.fOptions),
          fSciFormatter(other.fSciFormatter),
          fFormatter(other.fFormatter),
          fAap(other.fAap) {
    fSymbols = new DecimalFormatSymbols(*fSymbols);
    if (fRules != NULL) {
        fRules = new PluralRules(*fRules);
    }
    u_strcpy(fCurr, other.fCurr);
}


DecimalFormat2 &
DecimalFormat2::operator=(const DecimalFormat2 &other) {
    if (this == &other) {
        return (*this);
    }
    UObject::operator=(other);
    fMultiplier = other.fMultiplier;
    fRoundingMode = other.fRoundingMode;
    fMinIntDigits = other.fMinIntDigits;
    fMaxIntDigits = other.fMaxIntDigits;
    fMinFracDigits = other.fMinFracDigits;
    fMaxFracDigits = other.fMaxFracDigits;
    fMinSigDigits = other.fMinSigDigits;
    fMaxSigDigits = other.fMaxSigDigits;
    fUseScientific = other.fUseScientific;
    fUseSigDigits = other.fUseSigDigits;
    fGrouping = other.fGrouping;
    fUseGrouping = other.fUseGrouping;
    fPositivePrefixPattern = other.fPositivePrefixPattern;
    fNegativePrefixPattern = other.fNegativePrefixPattern;
    fPositiveSuffixPattern = other.fPositiveSuffixPattern;
    fNegativeSuffixPattern = other.fNegativeSuffixPattern;
    fCurrencyUsage = other.fCurrencyUsage;
    fMonetary = other.fMonetary;
    fAffixParser = other.fAffixParser;
    fCurrencyAffixInfo = other.fCurrencyAffixInfo;
    fEffPrecision = other.fEffPrecision;
    fEffGrouping = other.fEffGrouping;
    fOptions = other.fOptions;
    fSciFormatter = other.fSciFormatter;
    fFormatter = other.fFormatter;
    fAap = other.fAap;
    *fSymbols = *other.fSymbols;
    if (fRules != NULL && other.fRules != NULL) {
        *fRules = *other.fRules;
    } else {
        delete fRules;
        fRules = other.fRules;
        if (fRules != NULL) {
            fRules = new PluralRules(*fRules);
        }
    }
    u_strcpy(fCurr, other.fCurr);
    return *this;
}

UBool
DecimalFormat2::operator==(const DecimalFormat2 &other) const {
    if (this == &other) {
        return TRUE;
    }
    return (fMultiplier == other.fMultiplier)
            && (fRoundingMode == other.fRoundingMode)
            && (fMinIntDigits == other.fMinIntDigits)
            && (fMaxIntDigits == other.fMaxIntDigits)
            && (fMinFracDigits == other.fMinFracDigits)
            && (fMaxFracDigits == other.fMaxFracDigits)
            && (fMinSigDigits == other.fMinSigDigits)
            && (fMaxSigDigits == other.fMaxSigDigits)
            && (fUseScientific == other.fUseScientific)
            && (fUseSigDigits == other.fUseSigDigits)
            && fGrouping.equals(other.fGrouping)
            && fUseGrouping == other.fUseGrouping
            && fPositivePrefixPattern.equals(other.fPositivePrefixPattern)
            && fNegativePrefixPattern.equals(other.fNegativePrefixPattern)
            && fPositiveSuffixPattern.equals(other.fPositiveSuffixPattern)
            && fNegativeSuffixPattern.equals(other.fNegativeSuffixPattern)
            && fCurrencyUsage == other.fCurrencyUsage
            && fAffixParser.equals(other.fAffixParser)
            && fCurrencyAffixInfo.equals(other.fCurrencyAffixInfo)
            && fEffPrecision.equals(other.fEffPrecision)
            && fEffGrouping.equals(other.fEffGrouping)
            && fOptions.equals(other.fOptions)
            && fSciFormatter.equals(other.fSciFormatter)
            && fFormatter.equals(other.fFormatter)
            && fAap.equals(other.fAap)
            && (*fSymbols == *other.fSymbols)
            && ((fRules == other.fRules) || (
                    (fRules != NULL) && (other.fRules != NULL)
                    && (*fRules == *other.fRules)))
            && (fMonetary == other.fMonetary)
            && u_strcmp(fCurr, other.fCurr) == 0;
}

DecimalFormat2::~DecimalFormat2() {
    delete fSymbols;
    delete fRules;
}

ValueFormatter &
DecimalFormat2::prepareValueFormatter(ValueFormatter &vf) const {
    if (fUseScientific) {
        vf.prepareScientificFormatting(
                fSciFormatter, fFormatter, fEffPrecision, fOptions);
        return vf;
    }
    vf.prepareFixedDecimalFormatting(
            fFormatter, fEffGrouping, fEffPrecision.fMantissa, fOptions.fMantissa);
    return vf;
}

int32_t
DecimalFormat2::getScale() const {
    UBool usesPercent = fPositivePrefixPattern.usesPercent() || 
            fPositiveSuffixPattern.usesPercent() || 
            fNegativePrefixPattern.usesPercent() || 
            fNegativeSuffixPattern.usesPercent();
    if (usesPercent) {
        return 2;
    }
    UBool usesPermill = fPositivePrefixPattern.usesPermill() || 
            fPositiveSuffixPattern.usesPermill() || 
            fNegativePrefixPattern.usesPermill() || 
            fNegativeSuffixPattern.usesPermill();
    if (usesPermill) {
        return 3;
    }
    return 0;
}
    
void
DecimalFormat2::setScale(int32_t scale) {
    fMultiplier.set(1);
    fMultiplier.shiftDecimalRight(scale);
}

UnicodeString &
DecimalFormat2::format(
        int32_t number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const {
    FieldPositionOnlyHandler handler(pos);
    if (!fMultiplier.isZero()) {
        DigitList digits;
        digits.set(number);
        digits.mult(fMultiplier, status);
        return formatAdjustedDigitList(digits, appendTo, handler, status);
    }
    ValueFormatter vf;
    return fAap.formatInt32(
            number,
            prepareValueFormatter(vf),
            handler,
            fRules,
            appendTo,
            status);
}

UnicodeString &
DecimalFormat2::format(
        int32_t number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const {
    FieldPositionIteratorHandler handler(posIter, status);
    if (!fMultiplier.isZero()) {
        DigitList digits;
        digits.set(number);
        digits.mult(fMultiplier, status);
        return formatAdjustedDigitList(digits, appendTo, handler, status);
    }
    ValueFormatter vf;
    return fAap.formatInt32(
            number,
            prepareValueFormatter(vf),
            handler,
            fRules,
            appendTo,
            status);
}

UnicodeString &
DecimalFormat2::format(
        int64_t number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const {
    if (number >= -2147483648LL && number <= 2147483647LL) {
        return format((int32_t) number, appendTo, pos, status);
    }
    DigitList dl;
    dl.set(number);
    FieldPositionOnlyHandler handler(pos);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        double number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const {
    DigitList dl;
    dl.set(number);
    FieldPositionOnlyHandler handler(pos);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const {
    DigitList dl(number);
    FieldPositionOnlyHandler handler(pos);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        int64_t number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const {
    DigitList dl;
    dl.set(number);
    FieldPositionIteratorHandler handler(posIter, status);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        double number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const {
    DigitList dl;
    dl.set(number);
    FieldPositionIteratorHandler handler(posIter, status);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const {
    DigitList dl(number);
    FieldPositionIteratorHandler handler(posIter, status);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::format(
        const StringPiece &number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const {
    DigitList dl;
    dl.set(number, status);
    FieldPositionIteratorHandler handler(posIter, status);
    return formatDigitList(dl, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::formatDigitList(
        DigitList &number,
        UnicodeString &appendTo,
        FieldPositionHandler &handler,
        UErrorCode &status) const {
    if (!fMultiplier.isZero()) {
        number.mult(fMultiplier, status);
    }
    number.reduce();
    return formatAdjustedDigitList(number, appendTo, handler, status);
}

UnicodeString &
DecimalFormat2::formatAdjustedDigitList(
        DigitList &number,
        UnicodeString &appendTo,
        FieldPositionHandler &handler,
        UErrorCode &status) const {
    number.setRoundingMode(fRoundingMode);
    ValueFormatter vf;
    return fAap.format(
            number,
            prepareValueFormatter(vf),
            handler,
            fRules,
            appendTo,
            status);
}

UnicodeString
DecimalFormat2::select(double number, const PluralRules &rules) const {
    DigitList dl;
    dl.set(number);
    return select(dl, rules);
}

UnicodeString
DecimalFormat2::select(
        DigitList &number, const PluralRules &rules) const {
    UErrorCode status = U_ZERO_ERROR;
    if (!fMultiplier.isZero()) {
        number.mult(fMultiplier, status);
    }
    number.reduce();
    ValueFormatter vf;
    prepareValueFormatter(vf);
    NumericValue value;
    vf.initNumericValue(number, value, status);
    return value.select(rules);
}

void
DecimalFormat2::setMinimumSignificantDigits(int32_t newValue) {
    fMinSigDigits = newValue;
    fUseSigDigits = TRUE; // ticket 9936
    updatePrecision();
}
        
void
DecimalFormat2::setMaximumSignificantDigits(int32_t newValue) {
    fMaxSigDigits = newValue;
    fUseSigDigits = TRUE; // ticket 9936
    updatePrecision();
}
        
void
DecimalFormat2::setMinimumIntegerDigits(int32_t newValue) {
    fMinIntDigits = newValue;
    updatePrecision();
}
        
void
DecimalFormat2::setMaximumIntegerDigits(int32_t newValue) {
    fMaxIntDigits = newValue;
    updatePrecision();
}
        
void
DecimalFormat2::setMinimumFractionDigits(int32_t newValue) {
    fMinFracDigits = newValue;
    updatePrecision();
}
        
void
DecimalFormat2::setMaximumFractionDigits(int32_t newValue) {
    fMaxFracDigits = newValue;
    updatePrecision();
}

void
DecimalFormat2::setScientificNotation(UBool newValue) {
    fUseScientific = newValue;
    updatePrecision();
}
        
void
DecimalFormat2::setSignificantDigitsUsed(UBool newValue) {
    fUseSigDigits = newValue;
    updatePrecision();
}
        
void
DecimalFormat2::setGroupingSize(int32_t newValue) {
    fGrouping.fGrouping = newValue;
    updateGrouping();
}

void
DecimalFormat2::setSecondaryGroupingSize(int32_t newValue) {
    fGrouping.fGrouping2 = newValue;
    updateGrouping();
}

void
DecimalFormat2::setMinimumGroupingDigits(int32_t newValue) {
    fGrouping.fMinGrouping = newValue;
    updateGrouping();
}

void
DecimalFormat2::setGroupingUsed(UBool newValue) {
    fUseGrouping = newValue;
    updateGrouping();
}

void
DecimalFormat2::setCurrency(const UChar *currency, UErrorCode &status) {
    if (currency == NULL) {
        fCurr[0] = 0;
    } else {
        u_strncpy(fCurr, currency, UPRV_LENGTHOF(fCurr) - 1);
        fCurr[UPRV_LENGTHOF(fCurr) - 1] = 0;
    }
    updateFormatting(kFormattingCurrency, status);
}

void
DecimalFormat2::setCurrencyUsage(
        UCurrencyUsage currencyUsage, UErrorCode &status) {
    fCurrencyUsage = currencyUsage;
    updateFormatting(kFormattingCurrency, status);
}

void
DecimalFormat2::setRoundingIncrement(double d) {
    if (d > 0.0) {
        fEffPrecision.fMantissa.fRoundingIncrement.set(d);
    } else {
        fEffPrecision.fMantissa.fRoundingIncrement.set(0.0);
    }
}

double
DecimalFormat2::getRoundingIncrement() const {
    return fEffPrecision.fMantissa.fRoundingIncrement.getDouble();
}

int32_t
DecimalFormat2::getMultiplier() const {
    if (fMultiplier.isZero()) {
        return 1;
    }
    return (int32_t) fMultiplier.getDouble();
}

void
DecimalFormat2::setMultiplier(int32_t m) {
    if (m == 0 || m == 1) {
        fMultiplier.set(0);
    } else {
        fMultiplier.set(m);
    }
}

void
DecimalFormat2::setPositivePrefix(const UnicodeString &str) {
    fPositivePrefixPattern.remove();
    fPositivePrefixPattern.addLiteral(str.getBuffer(), 0, str.length());
    UErrorCode status = U_ZERO_ERROR;
    updateFormatting(kFormattingPosPrefix, status);
}

void
DecimalFormat2::setPositiveSuffix(const UnicodeString &str) {
    fPositiveSuffixPattern.remove();
    fPositiveSuffixPattern.addLiteral(str.getBuffer(), 0, str.length());
    UErrorCode status = U_ZERO_ERROR;
    updateFormatting(kFormattingPosSuffix, status);
}

void
DecimalFormat2::setNegativePrefix(const UnicodeString &str) {
    fNegativePrefixPattern.remove();
    fNegativePrefixPattern.addLiteral(str.getBuffer(), 0, str.length());
    UErrorCode status = U_ZERO_ERROR;
    updateFormatting(kFormattingNegPrefix, status);
}

void
DecimalFormat2::setNegativeSuffix(const UnicodeString &str) {
    fNegativeSuffixPattern.remove();
    fNegativeSuffixPattern.addLiteral(str.getBuffer(), 0, str.length());
    UErrorCode status = U_ZERO_ERROR;
    updateFormatting(kFormattingNegSuffix, status);
}

UnicodeString &
DecimalFormat2::getPositivePrefix(UnicodeString &result) const {
    result = fAap.fPositivePrefix.getOtherVariant().toString();
    return result;
}

UnicodeString &
DecimalFormat2::getPositiveSuffix(UnicodeString &result) const {
    result = fAap.fPositiveSuffix.getOtherVariant().toString();
    return result;
}

UnicodeString &
DecimalFormat2::getNegativePrefix(UnicodeString &result) const {
    result = fAap.fNegativePrefix.getOtherVariant().toString();
    return result;
}

UnicodeString &
DecimalFormat2::getNegativeSuffix(UnicodeString &result) const {
    result = fAap.fNegativeSuffix.getOtherVariant().toString();
    return result;
}

void
DecimalFormat2::adoptDecimalFormatSymbols(DecimalFormatSymbols *symbolsToAdopt) {
    if (symbolsToAdopt == NULL) {
        return;
    }
    delete fSymbols;
    fSymbols = symbolsToAdopt;
    UErrorCode status = U_ZERO_ERROR;
    updateFormatting(kFormattingSymbols, status);
}

void
DecimalFormat2::applyPattern(
        const UnicodeString &pattern, UErrorCode &status) {
    UParseError perror;
    applyPattern(pattern, FALSE, perror, status);
    // TODO: Consider updating everything except symbols
    updateAll(status);
}

void
DecimalFormat2::applyPattern(
        const UnicodeString &pattern,
        UParseError &perror, UErrorCode &status) {
    applyPattern(pattern, FALSE, perror, status);
    // TODO: Consider updating everything except symbols
    updateAll(status);
}

void
DecimalFormat2::applyLocalizedPattern(
        const UnicodeString &pattern, UErrorCode &status) {
    UParseError perror;
    applyPattern(pattern, TRUE, perror, status);
    updateAll(status);
}

void
DecimalFormat2::applyPattern(
        const UnicodeString &pattern,
        UBool localized, UParseError &perror, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    DecimalFormatPatternParser patternParser;
    if (localized) {
        patternParser.useSymbols(*fSymbols);
    }
    DecimalFormatPattern out;
    patternParser.applyPatternWithoutExpandAffix(
            pattern, out, perror, status);
    if (U_FAILURE(status)) {
        return;
    }
    fUseScientific = out.fUseExponentialNotation;
    fUseSigDigits = out.fUseSignificantDigits;
    fMinIntDigits = out.fMinimumIntegerDigits;
    fMaxIntDigits = out.fMaximumIntegerDigits;
    fMinFracDigits = out.fMinimumFractionDigits;
    fMaxFracDigits = out.fMaximumFractionDigits;
    fMinSigDigits = out.fMinimumSignificantDigits;
    fMaxSigDigits = out.fMaximumSignificantDigits;
    fOptions.fExponent.fMinDigits = out.fMinExponentDigits;
    fOptions.fExponent.fAlwaysShowSign = out.fExponentSignAlwaysShown;
    fUseGrouping = out.fGroupingUsed;
    fGrouping.fGrouping = out.fGroupingSize;
    fGrouping.fGrouping2 = out.fGroupingSize2;
    fOptions.fMantissa.fAlwaysShowDecimal = out.fDecimalSeparatorAlwaysShown;
    if (out.fRoundingIncrementUsed) {
        fEffPrecision.fMantissa.fRoundingIncrement = out.fRoundingIncrement;
    }
    fAap.fPadChar = out.fPad;
    fNegativePrefixPattern = out.fNegPrefixAffix;
    fNegativeSuffixPattern = out.fNegSuffixAffix;
    fPositivePrefixPattern = out.fPosPrefixAffix;
    fPositiveSuffixPattern = out.fPosSuffixAffix;

    // Work around. Pattern parsing code and DecimalFormat code don't agree
    // on the definition of field width, so we have to translate from
    // pattern field width to decimal format field width here.
    fAap.fWidth = out.fFormatWidth == 0 ? 0 :
            out.fFormatWidth + fPositivePrefixPattern.countChar32()
            + fPositiveSuffixPattern.countChar32();
    switch (out.fPadPosition) {
    case DecimalFormatPattern::kPadBeforePrefix:
        fAap.fPadPosition = DigitAffixesAndPadding::kPadBeforePrefix;
        break;    
    case DecimalFormatPattern::kPadAfterPrefix:
        fAap.fPadPosition = DigitAffixesAndPadding::kPadAfterPrefix;
        break;    
    case DecimalFormatPattern::kPadBeforeSuffix:
        fAap.fPadPosition = DigitAffixesAndPadding::kPadBeforeSuffix;
        break;    
    case DecimalFormatPattern::kPadAfterSuffix:
        fAap.fPadPosition = DigitAffixesAndPadding::kPadAfterSuffix;
        break;    
    default:
        break;
    }
}

void
DecimalFormat2::updatePrecision() {
    if (fUseScientific) {
        updatePrecisionForScientific();
    } else {
        updatePrecisionForFixed();
    }
}

static void updatePrecisionForScientificMinMax(
        const DigitInterval &min,
        const DigitInterval &max,
        DigitInterval &resultMin,
        DigitInterval &resultMax,
        SignificantDigitInterval &resultSignificant) {
    resultMin.setIntDigitCount(0);
    resultMin.setFracDigitCount(0);
    resultSignificant.clear();
    resultMax.clear();
    
    int32_t maxIntDigitCount = max.getIntDigitCount();
    int32_t minIntDigitCount = min.getIntDigitCount();
    int32_t maxFracDigitCount = max.getFracDigitCount();
    int32_t minFracDigitCount = min.getFracDigitCount();


    // Not in spec: maxIntDigitCount > 8 assume
    // maxIntDigitCount = minIntDigitCount. Current DecimalFormat API has
    // no provision for unsetting maxIntDigitCount which would be useful for
    // scientific notation. The best we can do is assume that if
    // maxIntDigitCount is the default of 2000000000 or is "big enough" then
    // user did not intend to explicitly set it. The 8 was derived emperically
    // by extensive testing of legacy code.
    if (maxIntDigitCount > 8) {
        maxIntDigitCount = minIntDigitCount;
    }

    // Per the spec, exponent grouping happens if maxIntDigitCount is more
    // than 1 and more than minIntDigitCount.
    UBool bExponentGrouping = maxIntDigitCount > 1 && minIntDigitCount < maxIntDigitCount;
    if (bExponentGrouping) {
        resultMax.setIntDigitCount(maxIntDigitCount);

        // For exponent grouping minIntDigits is always treated as 1 even
        // if it wasn't set to 1!
        resultMin.setIntDigitCount(1);
    } else {
        // Fixed digit count left of decimal. minIntDigitCount doesn't have
        // to equal maxIntDigitCount i.e minIntDigitCount == 0 while
        // maxIntDigitCount == 1.
        int32_t fixedIntDigitCount = maxIntDigitCount;

        // If fixedIntDigitCount is 0 but
        // min or max fraction count is 0 too then use 1. This way we can get
        // unlimited precision for X.XXXEX
        if (fixedIntDigitCount == 0 && (minFracDigitCount == 0 || maxFracDigitCount == 0)) {
            fixedIntDigitCount = 1;
        }
        resultMax.setIntDigitCount(fixedIntDigitCount);
        resultMin.setIntDigitCount(fixedIntDigitCount);
    }
    // Spec says this is how we compute significant digits. 0 means
    // unlimited significant digits.
    int32_t maxSigDigits = minIntDigitCount + maxFracDigitCount;
    if (maxSigDigits > 0) {
        int32_t minSigDigits = minIntDigitCount + minFracDigitCount;
        resultSignificant.setMin(minSigDigits);
        resultSignificant.setMax(maxSigDigits);
    }
}

void
DecimalFormat2::updatePrecisionForScientific() {
    FixedPrecision *result = &fEffPrecision.fMantissa;
    if (fUseSigDigits) {
        result->fMax.setFracDigitCount(-1);
        result->fMax.setIntDigitCount(1);
        result->fMin.setFracDigitCount(0);
        result->fMin.setIntDigitCount(1);
        result->fSignificant.clear();
        extractSigDigits(result->fSignificant);
        return;
    }
    DigitInterval max;
    DigitInterval min;
    extractMinMaxDigits(min, max);
    updatePrecisionForScientificMinMax(
            min, max,
            result->fMin, result->fMax, result->fSignificant);
}

void
DecimalFormat2::updatePrecisionForFixed() {
    FixedPrecision *result = &fEffPrecision.fMantissa;
    if (!fUseSigDigits) {
        extractMinMaxDigits(result->fMin, result->fMax);
        result->fSignificant.clear();
    } else {
        extractSigDigits(result->fSignificant);
        result->fMin.setIntDigitCount(1);
        result->fMin.setFracDigitCount(0);
        result->fMax.clear();
    }
}

void
 DecimalFormat2::extractMinMaxDigits(
        DigitInterval &min, DigitInterval &max) const {
    min.setIntDigitCount(fMinIntDigits < 0 ? 0 : fMinIntDigits);
    max.setIntDigitCount(fMaxIntDigits < 0 ? 0 : fMaxIntDigits);
    min.setFracDigitCount(fMinFracDigits < 0 ? 0 : fMinFracDigits);
    max.setFracDigitCount(fMaxFracDigits < 0 ? 0 : fMaxFracDigits);
}

void
 DecimalFormat2::extractSigDigits(
        SignificantDigitInterval &sig) const {
    sig.setMin(fMinSigDigits < 0 ? 0 : fMinSigDigits);
    sig.setMax(fMaxSigDigits < 0 ? 0 : fMaxSigDigits);
}

void
DecimalFormat2::updateGrouping() {
    if (fUseGrouping) {
        fEffGrouping = fGrouping;
    } else {
        fEffGrouping.clear();
    }
}

void
DecimalFormat2::updateFormatting(
        int32_t changedFormattingFields, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    // Each function updates one field. Order matters. For instance,
    // updatePluralRules comes before updateCurrencyAffixInfo because the
    // fRules field is needed to update the fCurrencyAffixInfo field.
    updateFormattingUsesCurrency(changedFormattingFields);
    updateFormattingFixedPointFormatter(changedFormattingFields);
    updateFormattingScientificFormatter(changedFormattingFields);
    updateFormattingAffixParser(changedFormattingFields);
    updateFormattingPluralRules(changedFormattingFields, status);
    updateFormattingCurrencyAffixInfo(changedFormattingFields, status);
    updateFormattingLocalizedPositivePrefix(
            changedFormattingFields, status);
    updateFormattingLocalizedPositiveSuffix(
            changedFormattingFields, status);
    updateFormattingLocalizedNegativePrefix(
            changedFormattingFields, status);
    updateFormattingLocalizedNegativeSuffix(
            changedFormattingFields, status);
}

void
DecimalFormat2::updateFormattingUsesCurrency(
        int32_t &changedFormattingFields) {
    if ((changedFormattingFields & kFormattingAffixes) == 0) {
        // If no affixes changed, don't need to do any work
        return;
    }
    UBool newUsesCurrency =
            fPositivePrefixPattern.usesCurrency() ||
            fPositiveSuffixPattern.usesCurrency() ||
            fNegativePrefixPattern.usesCurrency() ||
            fNegativeSuffixPattern.usesCurrency();
    if (fMonetary != newUsesCurrency) {
        fMonetary = newUsesCurrency;
        changedFormattingFields |= kFormattingUsesCurrency;
    }
}

void
DecimalFormat2::updateFormattingPluralRules(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if ((changedFormattingFields & (kFormattingSymbols | kFormattingUsesCurrency)) == 0) {
        // No work to do if both fSymbols and fMonetary
        // fields are unchanged
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    PluralRules *newRules = NULL;
    if (fMonetary) {
        newRules = PluralRules::forLocale(fSymbols->getLocale(), status);
        if (U_FAILURE(status)) {
            return;
        }
    }
    // Its ok to say a field has changed when it really hasn't but not
    // the other way around. Here we assume the field changed unless it
    // was NULL before and is still NULL now
    if (fRules != newRules) {
        delete fRules;
        fRules = newRules;
        changedFormattingFields |= kFormattingPluralRules;
    }
}

void
DecimalFormat2::updateFormattingCurrencyAffixInfo(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if ((changedFormattingFields & (
            kFormattingSymbols | kFormattingCurrency |
            kFormattingUsesCurrency | kFormattingPluralRules)) == 0) {
        // If all these fields are unchanged, no work to do.
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    if (!fMonetary) {
        if (fCurrencyAffixInfo.isDefault()) {
            // In this case don't have to do any work
            return;
        }
        fCurrencyAffixInfo.set(NULL, NULL, NULL, status);
        if (U_FAILURE(status)) {
            return;
        }
        changedFormattingFields |= kFormattingCurrencyAffixInfo;
    } else {
        UChar currencyBuf[4];
        const UChar *currency = fCurr;
        if (currency[0] == 0) {
            ucurr_forLocale(fSymbols->getLocale().getName(), currencyBuf, UPRV_LENGTHOF(currencyBuf), &status);
            if (U_SUCCESS(status)) {
                currency = currencyBuf;
            } else {
                currency = NULL;
                status = U_ZERO_ERROR;
            }
        }
        fCurrencyAffixInfo.set(
                fSymbols->getLocale().getName(), fRules, currency, status);
        if (U_FAILURE(status)) {
            return;
        }
        // If DecimalFormatSymbols has custom currency symbol, prefer
        // that over what we just read from the resource bundles
        if (fSymbols->isCustomCurrencySymbol()) {
            fCurrencyAffixInfo.fSymbol =
                    fSymbols->getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
        }
        changedFormattingFields |= kFormattingCurrencyAffixInfo;
        if (currency) {
            FixedPrecision precision;
            CurrencyAffixInfo::adjustPrecision(
                    currency, fCurrencyUsage, precision, status);
            if (U_FAILURE(status)) {
                return;
            }
            fMinFracDigits = precision.fMin.getFracDigitCount();
            fMaxFracDigits = precision.fMax.getFracDigitCount();
            updatePrecision();
            fEffPrecision.fMantissa.fRoundingIncrement =
                    precision.fRoundingIncrement;
        }
 
    }
}

void
DecimalFormat2::updateFormattingScientificFormatter(
        int32_t &changedFormattingFields) {
    if ((changedFormattingFields & kFormattingSymbols) == 0) {
        // No work to do if fSymbols is unchanged
        return;
    }
    fSciFormatter.setDecimalFormatSymbols(*fSymbols);
}


void
DecimalFormat2::updateFormattingFixedPointFormatter(
        int32_t &changedFormattingFields) {
    if ((changedFormattingFields & (kFormattingSymbols | kFormattingUsesCurrency)) == 0) {
        // No work to do if fSymbols is unchanged
        return;
    }
    if (fMonetary) {
        fFormatter.setDecimalFormatSymbolsForMonetary(*fSymbols);
    } else {
        fFormatter.setDecimalFormatSymbols(*fSymbols);
    }
}

void
DecimalFormat2::updateFormattingAffixParser(
        int32_t &changedFormattingFields) {
    if ((changedFormattingFields & kFormattingSymbols) == 0) {
        // No work to do if fSymbols is unchanged
        return;
    }
    fAffixParser.setDecimalFormatSymbols(*fSymbols);
    changedFormattingFields |= kFormattingAffixParser;
}

void
DecimalFormat2::updateFormattingLocalizedPositivePrefix(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ((changedFormattingFields & (
            kFormattingPosPrefix | kFormattingAffixParserWithCurrency)) == 0) {
        // No work to do
        return;
    }
    fAap.fPositivePrefix.remove();
    fAffixParser.parse(
            fPositivePrefixPattern,
            fCurrencyAffixInfo,
            fAap.fPositivePrefix,
            status);
}

void
DecimalFormat2::updateFormattingLocalizedPositiveSuffix(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ((changedFormattingFields & (
            kFormattingPosSuffix | kFormattingAffixParserWithCurrency)) == 0) {
        // No work to do
        return;
    }
    fAap.fPositiveSuffix.remove();
    fAffixParser.parse(
            fPositiveSuffixPattern,
            fCurrencyAffixInfo,
            fAap.fPositiveSuffix,
            status);
}

void
DecimalFormat2::updateFormattingLocalizedNegativePrefix(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ((changedFormattingFields & (
            kFormattingNegPrefix | kFormattingAffixParserWithCurrency)) == 0) {
        // No work to do
        return;
    }
    fAap.fNegativePrefix.remove();
    fAffixParser.parse(
            fNegativePrefixPattern,
            fCurrencyAffixInfo,
            fAap.fNegativePrefix,
            status);
}

void
DecimalFormat2::updateFormattingLocalizedNegativeSuffix(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ((changedFormattingFields & (
            kFormattingNegSuffix | kFormattingAffixParserWithCurrency)) == 0) {
        // No work to do
        return;
    }
    fAap.fNegativeSuffix.remove();
    fAffixParser.parse(
            fNegativeSuffixPattern,
            fCurrencyAffixInfo,
            fAap.fNegativeSuffix,
            status);
}

void
DecimalFormat2::updateAll(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    updatePrecision();
    updateGrouping();
    updateFormatting(kFormattingAll, status);
    setScale(getScale());
}

static int32_t
getMinimumLengthToDescribeGrouping(const DigitGrouping &grouping) {
    if (grouping.fGrouping <= 0) {
        return 0;
    }
    if (grouping.fGrouping2 <= 0) {
        return grouping.fGrouping + 1;
    }
    return grouping.fGrouping + grouping.fGrouping2 + 1;
}

/**
 * Given a grouping policy, calculates how many digits are needed left of
 * the decimal point to achieve a desired length left of the
 * decimal point.
 * @param grouping the grouping policy
 * @param desiredLength number of characters needed left of decimal point
 * @param minLeftDigits at least this many digits is returned
 * @param leftDigits the number of digits needed stored here
 *  which is >= minLeftDigits.
 * @return true if a perfect fit or false if having leftDigits would exceed
 *   desiredLength
 */
static UBool
getLeftDigitsForLeftLength(
        const DigitGrouping &grouping,
        int32_t desiredLength,
        int32_t minLeftDigits,
        int32_t &leftDigits) {
    leftDigits = minLeftDigits;
    int32_t lengthSoFar = leftDigits + grouping.getSeparatorCount(leftDigits);
    while (lengthSoFar < desiredLength) {
        lengthSoFar += grouping.isSeparatorAt(leftDigits + 1, leftDigits) ? 2 : 1;
        ++leftDigits;
    }
    return (lengthSoFar == desiredLength);
}

int32_t
DecimalFormat2::computeExponentPatternLength() const {
    if (fUseScientific) {
        return 1 + (fOptions.fExponent.fAlwaysShowSign ? 1 : 0) + fOptions.fExponent.fMinDigits;
    }
    return 0;
}

int32_t
DecimalFormat2::countFractionDigitAndDecimalPatternLength(
        int32_t fracDigitCount) const {
    if (!fOptions.fMantissa.fAlwaysShowDecimal && fracDigitCount == 0) {
        return 0;
    }
    return fracDigitCount + 1;
}

UnicodeString&
DecimalFormat2::toNumberPattern(
        UBool hasPadding, int32_t minimumLength, UnicodeString& result) const {
    // Get a grouping policy like the one in this object that does not
    // have minimum grouping since toPattern doesn't support it.
    DigitGrouping grouping(fEffGrouping);
    grouping.fMinGrouping = 0;

    // Only for fixed digits, these are the digits that get 0's.
    DigitInterval minInterval;

    // Only for fixed digits, these are the digits that get #'s.
    DigitInterval maxInterval;

    // Only for significant digits
    int32_t sigMin;
    int32_t sigMax;

    // These are all the digits to be displayed. For significant digits,
    // this interval always starts at the 1's place an extends left.
    DigitInterval fullInterval;

    // Digit range of rounding increment. If rounding increment is .025.
    // then roundingIncrementLowerExp = -3 and roundingIncrementUpperExp = -1
    int32_t roundingIncrementLowerExp = 0;
    int32_t roundingIncrementUpperExp = 0;

    if (fUseSigDigits) {
        SignificantDigitInterval sigInterval;
        extractSigDigits(sigInterval);
        sigMax = sigInterval.getMax();
        sigMin = sigInterval.getMin();
        fullInterval.setFracDigitCount(0);
        fullInterval.setIntDigitCount(sigMax);
    } else {
        extractMinMaxDigits(minInterval, maxInterval);
        if (fUseScientific) {
           if (maxInterval.getIntDigitCount() > kMaxScientificIntegerDigits) {
               maxInterval.setIntDigitCount(1);
               minInterval.shrinkToFitWithin(maxInterval);
           }
        } else if (hasPadding) {
            // Make max int digits match min int digits for now, we
            // compute necessary padding later.
            maxInterval.setIntDigitCount(minInterval.getIntDigitCount());
        } else {
            // For some reason toPattern adds at least one leading '#'
            maxInterval.setIntDigitCount(minInterval.getIntDigitCount() + 1);
        }
        if (!fEffPrecision.fMantissa.fRoundingIncrement.isZero()) {
            roundingIncrementLowerExp = 
                    fEffPrecision.fMantissa.fRoundingIncrement.getLowerExponent();
            roundingIncrementUpperExp = 
                    fEffPrecision.fMantissa.fRoundingIncrement.getUpperExponent();
            // We have to include the rounding increment in what we display
            maxInterval.expandToContainDigit(roundingIncrementLowerExp);
            maxInterval.expandToContainDigit(roundingIncrementUpperExp - 1);
        }
        fullInterval = maxInterval;
    }
    // We have to include enough digits to show grouping strategy
    int32_t minLengthToDescribeGrouping =
           getMinimumLengthToDescribeGrouping(grouping);
    if (minLengthToDescribeGrouping > 0) {
        fullInterval.expandToContainDigit(
                getMinimumLengthToDescribeGrouping(grouping) - 1);
    }

    // If we have a minimum length, we have to add digits to the left to
    // depict padding.
    if (hasPadding) {
        // For non scientific notation,
        //  minimumLengthForMantissa = minimumLength
        int32_t minimumLengthForMantissa = 
                minimumLength - computeExponentPatternLength();
        int32_t mininumLengthForMantissaIntPart =
                minimumLengthForMantissa
                - countFractionDigitAndDecimalPatternLength(
                        fullInterval.getFracDigitCount());
        // Because of grouping, we may need fewer than expected digits to
        // achieve the length we need.
        int32_t digitsNeeded;
        if (getLeftDigitsForLeftLength(
                grouping,
                mininumLengthForMantissaIntPart,
                fullInterval.getIntDigitCount(),
                digitsNeeded)) {

            // In this case, we achieved the exact length that we want.
            fullInterval.setIntDigitCount(digitsNeeded);
        } else if (digitsNeeded > fullInterval.getIntDigitCount()) {

            // Having digitsNeeded digits goes over desired length which
            // means that to have desired length would mean starting on a
            // grouping sepearator e.g ,###,### so add a '#' and use one
            // less digit. This trick gives ####,### but that is the best
            // we can do.
            result.append(kPatternDigit);
            fullInterval.setIntDigitCount(digitsNeeded - 1);
        }
    }
    int32_t maxDigitPos = fullInterval.getMostSignificantExclusive();
    int32_t minDigitPos = fullInterval.getLeastSignificantInclusive();
    for (int32_t i = maxDigitPos - 1; i >= minDigitPos; --i) {
        if (!fOptions.fMantissa.fAlwaysShowDecimal && i == -1) {
            result.append(kPatternDecimalSeparator);
        }
        if (fUseSigDigits) {
            // Use digit symbol
            if (i >= sigMax || i < sigMax - sigMin) {
                result.append(kPatternDigit);
            } else {
                result.append(kPatternSignificantDigit);
            }
        } else {
            if (i < roundingIncrementUpperExp && i >= roundingIncrementLowerExp) {
                result.append(fEffPrecision.fMantissa.fRoundingIncrement.getDigitByExponent(i) + kPatternZeroDigit);
            } else if (minInterval.contains(i)) {
                result.append(kPatternZeroDigit);
            } else {
                result.append(kPatternDigit);
            }
        }
        if (grouping.isSeparatorAt(i + 1, i)) {
            result.append(kPatternGroupingSeparator);
        }
        if (fOptions.fMantissa.fAlwaysShowDecimal && i == 0) {
            result.append(kPatternDecimalSeparator);
        }
    }
    if (fUseScientific) {
        result.append(kPatternExponent);
        if (fOptions.fExponent.fAlwaysShowSign) {
            result.append(kPatternPlus);
        }
        for (int32_t i = 0; i < 1 || i < fOptions.fExponent.fMinDigits; ++i) {
            result.append(kPatternZeroDigit);
        }
    }
    return result;
}

UnicodeString&
DecimalFormat2::toPattern(UnicodeString& result) const {
    result.remove();
    UnicodeString padSpec;
    if (fAap.fWidth > 0) {
        padSpec.append(kPatternPadEscape);
        padSpec.append(fAap.fPadChar);
    }
    if (fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix) {
        result.append(padSpec);
    }
    fPositivePrefixPattern.toUserString(result);
    if (fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix) {
        result.append(padSpec);
    }
    toNumberPattern(
            fAap.fWidth > 0,
            fAap.fWidth - fPositivePrefixPattern.countChar32() - fPositiveSuffixPattern.countChar32(),
            result);
    if (fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix) {
        result.append(padSpec);
    }
    fPositiveSuffixPattern.toUserString(result);
    if (fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix) {
        result.append(padSpec);
    }
    AffixPattern withNegative;
    withNegative.add(AffixPattern::kNegative);
    withNegative.append(fPositivePrefixPattern);
    if (!fPositiveSuffixPattern.equals(fNegativeSuffixPattern) ||
            !withNegative.equals(fNegativePrefixPattern)) {
        result.append(kPatternSeparator);
        if (fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix) {
            result.append(padSpec);
        }
        fNegativePrefixPattern.toUserString(result);
        if (fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix) {
            result.append(padSpec);
        }
        toNumberPattern(
                fAap.fWidth > 0,
                fAap.fWidth - fNegativePrefixPattern.countChar32() - fNegativeSuffixPattern.countChar32(),
                result);
        if (fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix) {
            result.append(padSpec);
        }
        fNegativeSuffixPattern.toUserString(result);
        if (fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix) {
            result.append(padSpec);
        }
    }
    return result;
}


U_NAMESPACE_END

