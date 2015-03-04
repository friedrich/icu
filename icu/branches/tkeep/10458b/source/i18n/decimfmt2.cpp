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

DecimalFormat2::DecimalFormat2(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status)
        : fSymbols(symbolsToAdopt), fRules(NULL) {
    fCurr[0] = 0;
    parsePattern(pattern, parseError, status);
    if (U_FAILURE(status)) {
        return;
    }
    updateSymbols();
    updateLocalizedAffixes(status);
}

DecimalFormat2::DecimalFormat2(const DecimalFormat2 &other) :
          UMemory(other),
          fPrecision(other.fPrecision),
          fOptions(other.fOptions),
          fUseScientific(other.fUseScientific),
          fGrouping(other.fGrouping),
          fUseGrouping(other.fUseGrouping),
          fPositivePrefixPattern(other.fPositivePrefixPattern),
          fNegativePrefixPattern(other.fNegativePrefixPattern),
          fPositiveSuffixPattern(other.fPositiveSuffixPattern),
          fNegativeSuffixPattern(other.fNegativeSuffixPattern),
          fSymbols(other.fSymbols),
          fAffixParser(other.fAffixParser),
          fEffPrecision(other.fEffPrecision),
          fEffGrouping(other.fEffGrouping),
          fSciFormatter(other.fSciFormatter),
          fFormatter(other.fFormatter),
          fAap(other.fAap),
          fRules(other.fRules),
          fScale(other.fScale) {
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
    fPrecision = other.fPrecision;
    fOptions = other.fOptions;
    fUseScientific = other.fUseScientific;
    fGrouping = other.fGrouping;
    fUseGrouping = other.fUseGrouping;
    fPositivePrefixPattern = other.fPositivePrefixPattern;
    fNegativePrefixPattern = other.fNegativePrefixPattern;
    fPositiveSuffixPattern = other.fPositiveSuffixPattern;
    fNegativeSuffixPattern = other.fNegativeSuffixPattern;
    *fSymbols = *other.fSymbols;
    u_strcpy(fCurr, other.fCurr);
    fAffixParser = other.fAffixParser;
    fEffPrecision = other.fEffPrecision;
    fEffGrouping = other.fEffGrouping;
    fSciFormatter = other.fSciFormatter;
    fFormatter = other.fFormatter;
    fAap = other.fAap;
    if (fRules != NULL && other.fRules != NULL) {
        *fRules = *other.fRules;
    } else {
        delete fRules;
        fRules = other.fRules;
        if (fRules != NULL) {
            fRules = new PluralRules(*fRules);
        }
    }
    fScale = other.fScale;
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
    fPrecision.fMantissa.fMin.setIntDigitCount(newValue);
    if (fUseScientific) {
        updateForScientific();
    } else {
        updateIntDigitCounts();
    }
}
        
void
DecimalFormat2::setMaximumIntegerDigits(int32_t newValue) {
    fPrecision.fMantissa.fMax.setIntDigitCount(newValue);
    if (fUseScientific) {
        updateForScientific();
    } else {
        updateIntDigitCounts();
    }
}
        
void
DecimalFormat2::setMinimumFractionDigits(int32_t newValue) {
    fPrecision.fMantissa.fMin.setFracDigitCount(newValue);
    if (fUseScientific) {
        updateForScientific();
    } else {
        updateFracDigitCounts();
    }
}
        
void
DecimalFormat2::setMaximumFractionDigits(int32_t newValue) {
    fPrecision.fMantissa.fMax.setFracDigitCount(newValue);
    if (fUseScientific) {
        updateForScientific();
    } else {
        updateFracDigitCounts();
    }
}

void
DecimalFormat2::setScientificNotation(UBool newValue) {
    fUseScientific = newValue;
    if (fUseScientific) {
        updateForScientific();
    } else {
        updateForFixedDecimal();
    }
}
        
void
DecimalFormat2::setGroupingSize(int32_t newValue) {
    fGrouping.fGrouping = newValue;
    fEffGrouping.fGrouping = newValue;
}

void
DecimalFormat2::setSecondaryGroupingSize(int32_t newValue) {
    fGrouping.fGrouping2 = newValue;
    fEffGrouping.fGrouping2 = newValue;
}

void
DecimalFormat2::setMinimumGroupingDigits(int32_t newValue) {
    fGrouping.fMinGrouping = newValue;
    fEffGrouping.fMinGrouping = newValue;
}

void
DecimalFormat2::setGroupingUsed(UBool newValue) {
    fUseGrouping = newValue;
    if (fUseGrouping) {
        fEffGrouping = fGrouping;
    } else {
        fEffGrouping.clear();
    }
}

void
DecimalFormat2::setCurrency(const UChar *currency, UErrorCode &status) {
    if (currency == NULL) {
        fCurr[0] = 0;
    } else {
        u_strncpy(fCurr, currency, UPRV_LENGTHOF(fCurr) - 1);
        fCurr[UPRV_LENGTHOF(fCurr) - 1] = 0;
    }
    if (affixesUseCurrency()) {
        updateLocalizedAffixes(status);
    }
}

void
DecimalFormat2::parsePattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status) {
}

void
DecimalFormat2::updateSymbols() {
}

void
DecimalFormat2::updateLocalizedAffixes(UErrorCode &status) {
}

void
DecimalFormat2::updateForFixedDecimal() {
    updateIntDigitCounts();
    updateFracDigitCounts();
    updateSigDigitCounts();
}

void
DecimalFormat2::updateIntDigitCounts() {
}

void
DecimalFormat2::updateFracDigitCounts() {
}

void
DecimalFormat2::updateSigDigitCounts() {
}

void
DecimalFormat2::updateForScientific() {
}

void
DecimalFormat2::updateLocalAffixes(UErrorCode &status) {
}

UBool
DecimalFormat2::affixesUseCurrency() {
    return fPositivePrefixPattern.usesCurrency()
    || fNegativePrefixPattern.usesCurrency()
    || fPositiveSuffixPattern.usesCurrency()
    || fNegativeSuffixPattern.usesCurrency();
}


U_NAMESPACE_END

