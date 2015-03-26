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
#include "valueformatter.h"
#include "fphdlimp.h"

U_NAMESPACE_BEGIN

static const int32_t kFormattingPosPrefix = (1 << 0);
static const int32_t kFormattingNegPrefix = (1 << 1);
static const int32_t kFormattingPosSuffix = (1 << 2);
static const int32_t kFormattingNegSuffix = (1 << 3);
static const int32_t kFormattingSymbols = (1 << 4);
static const int32_t kFormattingCurrency = (1 << 5);
static const int32_t kFormattingWidth = (1 << 6);
static const int32_t kFormattingUsesCurrency = (1 << 7);
static const int32_t kFormattingPluralRules = (1 << 8);
static const int32_t kFormattingAffixParser = (1 << 9);
static const int32_t kFormattingAll = (1 << 10) - 1;
static const int32_t kFormattingAffixes =
        kFormattingPosPrefix | kFormattingNegPrefix |
        kFormattingNegPrefix | kFormattingNegSuffix;


DecimalFormat2::DecimalFormat2(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status)
        : fRoundingMode(DigitList::kRoundHalfEven),
          fSymbols(symbolsToAdopt), fRules(NULL) {
    fCurr[0] = 0;
    applyPattern(pattern, FALSE, parseError, status);
    updateAll(status);
}

DecimalFormat2::DecimalFormat2(const DecimalFormat2 &other) :
          UMemory(other),
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
          fFormatWidth(other.fFormatWidth),
          fRules(other.fRules),
          fAffixParser(other.fAffixParser),
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
    UMemory::operator=(other);
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
    fFormatWidth = other.fFormatWidth;
    fAffixParser = other.fAffixParser;
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
DecimalFormat2::setPositivePrefix(const UnicodeString &prefix) {
    fPositivePrefixPattern.remove();
    UErrorCode status = U_ZERO_ERROR;
    AffixPattern::parseAffixString(
            prefix,
            fPositivePrefixPattern,
            status);
    updateFormatting(kFormattingPosPrefix, status);
}

void
DecimalFormat2::adoptDecimalFormatSymbols(DecimalFormatSymbols *symbolsToAdopt) {
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
    fFormatWidth = out.fFormatWidth;
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

void
DecimalFormat2::updatePrecisionForScientific() {
    DigitInterval max;
    DigitInterval min;
    extractMinMaxDigits(min, max);

    int32_t maxIntDigitCount = max.getIntDigitCount();
    int32_t minIntDigitCount = min.getIntDigitCount();
    int32_t maxFracDigitCount = max.getFracDigitCount();
    int32_t minFracDigitCount = min.getFracDigitCount();

    FixedPrecision *result = &fEffPrecision.fMantissa;
    result->fMax.clear();
    result->fMin.setIntDigitCount(0);
    result->fMin.setFracDigitCount(0);
    result->fSignificant.clear();

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
        result->fMax.setIntDigitCount(maxIntDigitCount);

        // For exponent grouping minIntDigits is always treated as 1 even
        // if it wasn't set to 1!
        result->fMin.setIntDigitCount(1);
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
        result->fMax.setIntDigitCount(fixedIntDigitCount);
        result->fMin.setIntDigitCount(fixedIntDigitCount);
    }
    // Spec says this is how we compute significant digits. 0 means
    // unlimited significant digits.
    int32_t maxSigDigits = minIntDigitCount + maxFracDigitCount;
    if (maxSigDigits > 0) {
        int32_t minSigDigits = minIntDigitCount + minFracDigitCount;
        result->fSignificant.setMin(minSigDigits);
        result->fSignificant.setMax(maxSigDigits);
    }
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
    // Each function updates one field. Order matters. For instance,
    // updatePluralRules comes before updateAffixParser because the fRules
    // field is needed to update the fAffixParser field.
    updateFormattingUsesCurrency(changedFormattingFields);
    updateFormattingPluralRules(changedFormattingFields, status);
    updateFormattingAffixParser(changedFormattingFields, status);
    updateFormattingFormatters(changedFormattingFields);
    updateFormattingLocalizedAffixes(changedFormattingFields, status);
    updateFormattingWidth(changedFormattingFields);
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
    if (fOptions.fMantissa.fMonetary != newUsesCurrency) {
        fOptions.fMantissa.fMonetary = newUsesCurrency;
        changedFormattingFields |= kFormattingUsesCurrency;
    }
}

void
DecimalFormat2::updateFormattingPluralRules(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if ((changedFormattingFields & (kFormattingSymbols | kFormattingUsesCurrency)) == 0) {
        // No work to do if both fSymbols and fOptions.fMantissa.fMonetary
        // fields are unchanged
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    PluralRules *newRules = NULL;
    if (fOptions.fMantissa.fMonetary) {
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
DecimalFormat2::updateFormattingAffixParser(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if ((changedFormattingFields & (kFormattingSymbols | kFormattingCurrency | kFormattingUsesCurrency | kFormattingPluralRules)) == 0) {
        // If all these fields are unchanged, no work to do.
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    if (changedFormattingFields & kFormattingSymbols) {
        fAffixParser.setDecimalFormatSymbols(*fSymbols);
        changedFormattingFields |= kFormattingAffixParser;
    }
    if (!fOptions.fMantissa.fMonetary) {
        if (fAffixParser.fCurrencyAffixInfo.isDefault()) {
            // In this case don't have to do any work
            return;
        }
        fAffixParser.fCurrencyAffixInfo.set(NULL, NULL, NULL, status);
        if (U_FAILURE(status)) {
            return;
        }
        changedFormattingFields |= kFormattingAffixParser;
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
        fAffixParser.fCurrencyAffixInfo.set(
                fSymbols->getLocale().getName(), fRules, currency, status);
        if (U_FAILURE(status)) {
            return;
        }
        // If DecimalFormatSymbols has custom currency symbol, prefer
        // that over what we just read from the resource bundles
        if (fSymbols->isCustomCurrencySymbol()) {
            fAffixParser.fCurrencyAffixInfo.fSymbol =
                    fSymbols->getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
        }
        changedFormattingFields |= kFormattingAffixParser;
        if (currency) {
            FixedPrecision precision;
            CurrencyAffixInfo::adjustPrecision(
                    currency, UCURR_USAGE_STANDARD, precision, status);
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
DecimalFormat2::updateFormattingFormatters(
        int32_t &changedFormattingFields) {
    if ((changedFormattingFields & kFormattingSymbols) == 0) {
        // No work to do if fSymbols is unchanged
        return;
    }
    fSciFormatter.setDecimalFormatSymbols(*fSymbols);
    fFormatter.setDecimalFormatSymbols(*fSymbols);
}

void
DecimalFormat2::updateFormattingLocalizedAffixes(
        int32_t &changedFormattingFields, UErrorCode &status) {
    if ((changedFormattingFields & (kFormattingAffixes | kFormattingAffixParser)) == 0) {
        // No work to do if all affixes and fAffixParser fields are
        // unchanged
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    if ((changedFormattingFields & kFormattingAffixes) != 0) {
        setScale(getScale());
    }

    if (changedFormattingFields & (kFormattingPosPrefix | kFormattingAffixParser)) {
        fAap.fPositivePrefix.remove();
        fAffixParser.parse(
                fPositivePrefixPattern, fAap.fPositivePrefix, status);
    }
    if (changedFormattingFields & (kFormattingNegPrefix | kFormattingAffixParser)) {
        fAap.fNegativePrefix.remove();
        fAffixParser.parse(
                fNegativePrefixPattern, fAap.fNegativePrefix, status);
    }
    if (changedFormattingFields & (kFormattingPosSuffix | kFormattingAffixParser)) {
        fAap.fPositiveSuffix.remove();
        fAffixParser.parse(
                fPositiveSuffixPattern, fAap.fPositiveSuffix, status);
    }
    if (changedFormattingFields & (kFormattingNegSuffix | kFormattingAffixParser)) {
        fAap.fNegativeSuffix.remove();
        fAffixParser.parse(
                fNegativeSuffixPattern, fAap.fNegativeSuffix, status);
    }
}

void
DecimalFormat2::updateFormattingWidth(int32_t &changedFormattingFields) {
    if ((changedFormattingFields & (kFormattingWidth | kFormattingPosPrefix | kFormattingPosSuffix)) == 0) {
        // No work to do if these fields are unchanged
        return;
    }
    if (fFormatWidth == 0) {
        fAap.fWidth = 0;
        return;
    }
    fAap.fWidth =
            fFormatWidth +
            fPositivePrefixPattern.countChar32() +
            fPositiveSuffixPattern.countChar32();
}
        

void
DecimalFormat2::updateAll(UErrorCode &status) {
    updatePrecision();
    updateGrouping();
    updateFormatting(kFormattingAll, status);
}

U_NAMESPACE_END

