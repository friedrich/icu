/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: decimfmt2.cpp
 */

#include "decimfmt2.h"
#include "unicode/plurrule.h"
#include "unicode/ustring.h"

U_NAMESPACE_BEGIN

static const int32_t kPrecisionInt = (1 << 0);
static const int32_t kPrecisionFrac = (1 << 1);
static const int32_t kPrecisionSig = (1 << 2);
static const int32_t kPrecisionUseScientific = (1 << 3);
static const int32_t kPrecisionAll = (1 << 4) - 1;

static const int32_t kGroupingPrimary = (1 << 0);
static const int32_t kGroupingSecondary = (1 << 1);
static const int32_t kGroupingMin = (1 << 2);
static const int32_t kGroupingOn = (1 << 3);
static const int32_t kGroupingAll = (1 << 4) - 1;

static const int32_t kFormattingPosPrefix = (1 << 0);
static const int32_t kFormattingNegPrefix = (1 << 1);
static const int32_t kFormattingPosSuffix = (1 << 2);
static const int32_t kFormattingNegSuffix = (1 << 3);
static const int32_t kFormattingSymbols = (1 << 4);
static const int32_t kFormattingCurrency = (1 << 5);
static const int32_t kFormattingUsesCurrency = (1 << 6);
static const int32_t kFormattingPluralRules = (1 << 7);
static const int32_t kFormattingAffixParser = (1 << 8);
static const int32_t kFormattingAll = (1 << 9) - 1;


DecimalFormat2::DecimalFormat2(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status)
        : fSymbols(symbolsToAdopt), fRules(NULL) {
    fCurr[0] = 0;
    parsePattern(pattern, parseError, status);
    updateAll(status);
}

DecimalFormat2::DecimalFormat2(const DecimalFormat2 &other) :
          UMemory(other),
          fMinIntDigits(other.fMinIntDigits),
          fMaxIntDigits(other.fMaxIntDigits),
          fMinFracDigits(other.fMinFracDigits),
          fMaxFracDigits(other.fMaxFracDigits),
          fMinSigDigits(other.fMinSigDigits),
          fMaxSigDigits(other.fMaxSigDigits),
          fUseScientific(other.fUseScientific),
          fGrouping(other.fGrouping),
          fUseGrouping(other.fUseGrouping),
          fPositivePrefixPattern(other.fPositivePrefixPattern),
          fNegativePrefixPattern(other.fNegativePrefixPattern),
          fPositiveSuffixPattern(other.fPositiveSuffixPattern),
          fNegativeSuffixPattern(other.fNegativeSuffixPattern),
          fSymbols(other.fSymbols),
          fUsesCurrency(other.fUsesCurrency),
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
    fMinIntDigits = other.fMinIntDigits;
    fMaxIntDigits = other.fMaxIntDigits;
    fMinFracDigits = other.fMinFracDigits;
    fMaxFracDigits = other.fMaxFracDigits;
    fMinSigDigits = other.fMinSigDigits;
    fMaxSigDigits = other.fMaxSigDigits;
    fUseScientific = other.fUseScientific;
    fGrouping = other.fGrouping;
    fUseGrouping = other.fUseGrouping;
    fPositivePrefixPattern = other.fPositivePrefixPattern;
    fNegativePrefixPattern = other.fNegativePrefixPattern;
    fPositiveSuffixPattern = other.fPositiveSuffixPattern;
    fNegativeSuffixPattern = other.fNegativeSuffixPattern;
    fUsesCurrency = other.fUsesCurrency;
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

UnicodeString &
DecimalFormat2::format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) {
    return appendTo;
}

void
DecimalFormat2::setMinimumIntegerDigits(int32_t newValue) {
    fMinIntDigits = newValue;
    updatePrecision(kPrecisionInt);
}
        
void
DecimalFormat2::setMaximumIntegerDigits(int32_t newValue) {
    fMaxIntDigits = newValue;
    updatePrecision(kPrecisionInt);
}
        
void
DecimalFormat2::setMinimumFractionDigits(int32_t newValue) {
    fMinFracDigits = newValue;
    updatePrecision(kPrecisionFrac);
}
        
void
DecimalFormat2::setMaximumFractionDigits(int32_t newValue) {
    fMaxFracDigits = newValue;
    updatePrecision(kPrecisionFrac);
}

void
DecimalFormat2::setScientificNotation(UBool newValue) {
    fUseScientific = newValue;
    updatePrecision(kPrecisionUseScientific);
}
        
void
DecimalFormat2::setGroupingSize(int32_t newValue) {
    fGrouping.fGrouping = newValue;
    updateGrouping(kGroupingPrimary);
}

void
DecimalFormat2::setSecondaryGroupingSize(int32_t newValue) {
    fGrouping.fGrouping2 = newValue;
    updateGrouping(kGroupingSecondary);
}

void
DecimalFormat2::setMinimumGroupingDigits(int32_t newValue) {
    fGrouping.fMinGrouping = newValue;
    updateGrouping(kGroupingMin);
}

void
DecimalFormat2::setGroupingUsed(UBool newValue) {
    fUseGrouping = newValue;
    updateGrouping(kGroupingOn);
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
DecimalFormat2::parsePattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status) {
}

void
DecimalFormat2::updatePrecision(int32_t flags) {
}

void
DecimalFormat2::updateGrouping(int32_t flags) {
}

void
DecimalFormat2::updateFormatting(int32_t flags, UErrorCode &status) {
}

UBool
DecimalFormat2::updateUsesCurrency() {
    return TRUE;
}

void
DecimalFormat2::updatePluralRules() {
}

void
DecimalFormat2::updateAffixParser(int32_t flags) {
}

void
DecimalFormat2::updateFormatters() {
}

void
DecimalFormat2::updateLocalizedAffixes(int32_t flags) {
}

void
DecimalFormat2::updateAll(UErrorCode &status) {
    updatePrecision(kPrecisionAll);
    updateGrouping(kGroupingAll);
    updateFormatting(kFormattingAll, status);
}

U_NAMESPACE_END

