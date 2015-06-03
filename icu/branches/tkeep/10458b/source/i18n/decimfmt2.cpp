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
#include "decfmtst.h"
#include "unicode/utf16.h"
#include "charstr.h"
#include "uassert.h"
#include "unicode/uniset.h"
#include "patternprops.h"
#include "ucurrimp.h"

U_NAMESPACE_BEGIN

#ifdef FMT_DEBUG
#include <stdio.h>
static void _debugout(const char *f, int l, const UnicodeString& s) {
    char buf[2000];
    s.extract((int32_t) 0, s.length(), buf, "utf-8");
    printf("%s:%d: %s\n", f,l, buf);
}
#define debugout(x) _debugout(__FILE__,__LINE__,x)
#define debug(x) printf("%s:%d: %s\n", __FILE__,__LINE__, x);
static const UnicodeString dbg_null("<NULL>","");
#define DEREFSTR(x)   ((x!=NULL)?(*x):(dbg_null))
#else
#define debugout(x)
#define debug(x)
#endif

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
        : fRoundingMode(DigitList::kRoundHalfEven), fLenient(FALSE),
          fParseDecimalMarkRequired(FALSE),
          fParseNoExponent(FALSE),
          fParseIntegerOnly(FALSE),
          fSymbols(NULL),
          fCurrencyUsage(UCURR_USAGE_STANDARD),
          fRules(NULL),
          fMonetary(FALSE) {
    fStaticSets = DecimalFormatStaticSets::getStaticSets(status);
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
          fLenient(FALSE),
          fParseDecimalMarkRequired(FALSE),
          fParseNoExponent(FALSE),
          fParseIntegerOnly(FALSE),
          fSymbols(symbolsToAdopt),
          fCurrencyUsage(UCURR_USAGE_STANDARD),
          fRules(NULL),
          fMonetary(FALSE) {
    fCurr[0] = 0;
    fStaticSets = DecimalFormatStaticSets::getStaticSets(status);
    applyPattern(pattern, FALSE, parseError, status);
    updateAll(status);
}

DecimalFormat2::DecimalFormat2(const DecimalFormat2 &other) :
          UObject(other),
          fMultiplier(other.fMultiplier),
          fRoundingMode(other.fRoundingMode),
          fLenient(other.fLenient),
          fStaticSets(other.fStaticSets),
          fParseDecimalMarkRequired(other.fParseDecimalMarkRequired),
          fParseNoExponent(other.fParseNoExponent),
          fParseIntegerOnly(other.fParseIntegerOnly),
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
    fLenient = other.fLenient;
    fStaticSets = other.fStaticSets;
    fParseDecimalMarkRequired = other.fParseDecimalMarkRequired;
    fParseNoExponent = other.fParseNoExponent;
    fParseIntegerOnly = other.fParseIntegerOnly;
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
            && (fLenient == other.fLenient)
            && (fParseDecimalMarkRequired == other.fParseDecimalMarkRequired)
            && (fParseNoExponent == other.fParseNoExponent)
            && (fParseIntegerOnly == other.fParseIntegerOnly)
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
    vf.round(number, status);
    return vf.select(rules, number);
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
DecimalFormat2::getEffectiveCurrency(
        UChar *result, UErrorCode &status) const {
    if (*fCurr != 0) {
        u_strncpy(result, fCurr, 3);
        result[3] = 0;
    } else {
        ucurr_forLocale(fSymbols->getLocale().getName(), result, 4, &status);
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

int32_t
DecimalFormat2::getOldFormatWidth() const {
    if (fAap.fWidth == 0) {
        return 0;
    }
    return fAap.fWidth - fPositiveSuffixPattern.countChar32() - fPositivePrefixPattern.countChar32();
}

const UnicodeString &
DecimalFormat2::getConstSymbol(
        DecimalFormatSymbols::ENumberFormatSymbol symbol) const {
   return fSymbols->getConstSymbol(symbol); 
}

UBool
DecimalFormat2::isParseFastpath() const {
    AffixPattern negative;
    negative.add(AffixPattern::kNegative);

    return fAap.fWidth == 0 &&
    fPositivePrefixPattern.countChar32() == 0 &&
    fNegativePrefixPattern.equals(negative) &&
    fPositiveSuffixPattern.countChar32() == 0 &&
    fNegativeSuffixPattern.countChar32() == 0;
}

void
DecimalFormat2::parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const {
    parse(text, result, parsePosition, NULL);
}

/**
 * Parses the given text as a number, optionally providing a currency amount.
 * @param text the string to parse
 * @param result output parameter for the numeric result.
 * @param parsePosition input-output position; on input, the
 * position within text to match; must have 0 <= pos.getIndex() <
 * text.length(); on output, the position after the last matched
 * character. If the parse fails, the position in unchanged upon
 * output.
 * @param currency if non-NULL, it should point to a 4-UChar buffer.
 * In this case the text is parsed as a currency format, and the
 * ISO 4217 code for the parsed currency is put into the buffer.
 * Otherwise the text is parsed as a non-currency format.
 */
void DecimalFormat2::parse(const UnicodeString& text,
                          Formattable& result,
                          ParsePosition& parsePosition,
                          UChar* currency) const {
    int32_t startIdx, backup;
    int32_t i = startIdx = backup = parsePosition.getIndex();

    // clear any old contents in the result.  In particular, clears any DigitList
    //   that it may be holding.
    result.setLong(0);
    if (currency != NULL) {
        for (int32_t ci=0; ci<4; ci++) {
            currency[ci] = 0;
        }
    }

    // Handle NaN as a special case:
    int32_t formatWidth = getOldFormatWidth();

    // Skip padding characters, if around prefix
    if (formatWidth > 0 && (
            fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix ||
            fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix)) {
        i = skipPadding(text, i);
    }

    if (isLenient()) {
        // skip any leading whitespace
        i = backup = skipUWhiteSpace(text, i);
    }

    // If the text is composed of the representation of NaN, returns NaN.length
    const UnicodeString *nan = &getConstSymbol(DecimalFormatSymbols::kNaNSymbol);
    int32_t nanLen = (text.compare(i, nan->length(), *nan)
                      ? 0 : nan->length());
    if (nanLen) {
        i += nanLen;
        if (formatWidth > 0 && (fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix || fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix)) {
            i = skipPadding(text, i);
        }
        parsePosition.setIndex(i);
        result.setDouble(uprv_getNaN());
        return;
    }

    // NaN parse failed; start over
    i = backup;
    parsePosition.setIndex(i);

    // status is used to record whether a number is infinite.
    UBool status[fgStatusLength];

    DigitList *digits = result.getInternalDigitList(); // get one from the stack buffer
    if (digits == NULL) {
        return;    // no way to report error from here.
    }

    if (fMonetary) {
        if (!parseForCurrency(text, parsePosition, *digits,
                              status, currency)) {
          return;
        }
    } else {
        if (!subparse(text,
                      &fAap.fNegativePrefix.getOtherVariant().toString(),
                      &fAap.fNegativeSuffix.getOtherVariant().toString(),
                      &fAap.fPositivePrefix.getOtherVariant().toString(),
                      &fAap.fPositiveSuffix.getOtherVariant().toString(),
                      FALSE, UCURR_SYMBOL_NAME,
                      parsePosition, *digits, status, currency)) {
            debug("!subparse(...) - rewind");
            parsePosition.setIndex(startIdx);
            return;
        }
    }

    // Handle infinity
    if (status[fgStatusInfinite]) {
        double inf = uprv_getInfinity();
        result.setDouble(digits->isPositive() ? inf : -inf);
        // TODO:  set the dl to infinity, and let it fall into the code below.
    }

    else {

        if (!fMultiplier.isZero()) {
            UErrorCode ec = U_ZERO_ERROR;
            digits->div(fMultiplier, ec);
        }

        // Negative zero special case:
        //    if parsing integerOnly, change to +0, which goes into an int32 in a Formattable.
        //    if not parsing integerOnly, leave as -0, which a double can represent.
        if (digits->isZero() && !digits->isPositive() && isParseIntegerOnly()) {
            digits->setPositive(TRUE);
        }
        result.adoptDigitList(digits);
    }
}



UBool
DecimalFormat2::parseForCurrency(const UnicodeString& text,
                                ParsePosition& parsePosition,
                                DigitList& digits,
                                UBool* status,
                                UChar* currency) const {
    int origPos = parsePosition.getIndex();
    int maxPosIndex = origPos;
    int maxErrorPos = -1;
    // First, parse against current pattern.
    // Since current pattern could be set by applyPattern(),
    // it could be an arbitrary pattern, and it may not be the one
    // defined in current locale.
    UBool tmpStatus[fgStatusLength];
    ParsePosition tmpPos(origPos);
    DigitList tmpDigitList;
    UBool found;
    // TODO: Figure out how to tell if we are using long form currency anywhere.
    found = subparse(text,
                     &fAap.fNegativePrefix.getOtherVariant().toString(),
                     &fAap.fNegativeSuffix.getOtherVariant().toString(),
                     &fAap.fPositivePrefix.getOtherVariant().toString(),
                     &fAap.fPositiveSuffix.getOtherVariant().toString(),
                     TRUE, UCURR_SYMBOL_NAME,
                     tmpPos, tmpDigitList, tmpStatus, currency);
    if (found) {
        if (tmpPos.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus[i];
            }
            digits = tmpDigitList;
        }
    } else {
        maxErrorPos = tmpPos.getErrorIndex();
    }

    // TODO: Consider old fCurrencyAffixPatterns hash table

    // Finally, parse against simple affix to find the match.
    // For example, in TestMonster suite,
    // if the to-be-parsed text is "-\u00A40,00".
    // complexAffixCompare will not find match,
    // since there is no ISO code matches "\u00A4",
    // and the parse stops at "\u00A4".
    // We will just use simple affix comparison (look for exact match)
    // to pass it.
    //
    // TODO: We should parse against simple affix first when
    // output currency is not requested. After the complex currency
    // parsing implementation was introduced, the default currency
    // instance parsing slowed down because of the new code flow.
    // I filed #10312 - Yoshito
    UBool tmpStatus_2[fgStatusLength];
    ParsePosition tmpPos_2(origPos);
    DigitList tmpDigitList_2;

    // Disable complex currency parsing and try it again.
    UBool result = subparse(text,
                            &fAap.fNegativePrefix.getOtherVariant().toString(),
                            &fAap.fNegativeSuffix.getOtherVariant().toString(),
                            &fAap.fPositivePrefix.getOtherVariant().toString(),
                            &fAap.fPositiveSuffix.getOtherVariant().toString(),
                            FALSE /* disable complex currency parsing */, UCURR_SYMBOL_NAME,
                            tmpPos_2, tmpDigitList_2, tmpStatus_2,
                            currency);
    if (result) {
        if (tmpPos_2.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos_2.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus_2[i];
            }
            digits = tmpDigitList_2;
        }
        found = true;
    } else {
            maxErrorPos = (tmpPos_2.getErrorIndex() > maxErrorPos) ?
                          tmpPos_2.getErrorIndex() : maxErrorPos;
    }

    if (!found) {
        //parsePosition.setIndex(origPos);
        parsePosition.setErrorIndex(maxErrorPos);
    } else {
        parsePosition.setIndex(maxPosIndex);
        parsePosition.setErrorIndex(-1);
    }
    return found;
}


/**
 * Parse the given text into a number.  The text is parsed beginning at
 * parsePosition, until an unparseable character is seen.
 * @param text the string to parse.
 * @param negPrefix negative prefix.
 * @param negSuffix negative suffix.
 * @param posPrefix positive prefix.
 * @param posSuffix positive suffix.
 * @param complexCurrencyParsing whether it is complex currency parsing or not.
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param parsePosition The position at which to being parsing.  Upon
 * return, the first unparsed character.
 * @param digits the DigitList to set to the parsed value.
 * @param status output param containing boolean status flags indicating
 * whether the value was infinite and whether it was positive.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or NULL for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 */
UBool DecimalFormat2::subparse(const UnicodeString& text,
                              const UnicodeString* negPrefix,
                              const UnicodeString* negSuffix,
                              const UnicodeString* posPrefix,
                              const UnicodeString* posSuffix,
                              UBool complexCurrencyParsing,
                              int8_t type,
                              ParsePosition& parsePosition,
                              DigitList& digits, UBool* status,
                              UChar* currency) const
{
    //  The parsing process builds up the number as char string, in the neutral format that
    //  will be acceptable to the decNumber library, then at the end passes that string
    //  off for conversion to a decNumber.
    UErrorCode err = U_ZERO_ERROR;
    CharString parsedNum;
    digits.setToZero();

    int32_t position = parsePosition.getIndex();
    int32_t oldStart = position;
    int32_t textLength = text.length(); // One less pointer to follow
    UBool strictParse = !isLenient();
    UChar32 zero = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    const UnicodeString *groupingString = &getConstSymbol(!fMonetary ? 
        DecimalFormatSymbols::kGroupingSeparatorSymbol : DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
    UChar32 groupingChar = groupingString->char32At(0);
    int32_t groupingStringLength = groupingString->length();
    int32_t groupingCharLength   = U16_LENGTH(groupingChar);
    UBool   groupingUsed = isGroupingUsed();
#ifdef FMT_DEBUG
    UChar dbgbuf[300];
    UnicodeString s(dbgbuf,0,300);;
    s.append((UnicodeString)"PARSE \"").append(text.tempSubString(position)).append((UnicodeString)"\" " );
#define DBGAPPD(x) if(x) { s.append(UnicodeString(#x "="));  if(x->isEmpty()) { s.append(UnicodeString("<empty>")); } else { s.append(*x); } s.append(UnicodeString(" ")); } else { s.append(UnicodeString(#x "=NULL ")); }
    DBGAPPD(negPrefix);
    DBGAPPD(negSuffix);
    DBGAPPD(posPrefix);
    DBGAPPD(posSuffix);
    debugout(s);
    printf("currencyParsing=%d, fFormatWidth=%d, isParseIntegerOnly=%c text.length=%d negPrefLen=%d\n", currencyParsing, formatWidth, (isParseIntegerOnly())?'Y':'N', text.length(),  negPrefix!=NULL?negPrefix->length():-1);
#endif

    UBool fastParseOk = false; /* TRUE iff fast parse is OK */
    if((isParseFastpath()) &&
       !fMonetary &&
       text.length()>0 &&
       text.length()<32 &&
       (posPrefix==NULL||posPrefix->isEmpty()) &&
       (posSuffix==NULL||posSuffix->isEmpty())) {  // optimized path
      int j=position;
      int l=text.length();
      int digitCount=0;
      UChar32 ch = text.char32At(j);
      const UnicodeString *decimalString = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
      UChar32 decimalChar = 0;
      UBool intOnly = FALSE;
      UChar32 lookForGroup = (groupingUsed&&intOnly&&strictParse)?groupingChar:0;

      int32_t decimalCount = decimalString->countChar32(0,3);
      if(isParseIntegerOnly()) {
        decimalChar = 0; // not allowed
        intOnly = TRUE; // Don't look for decimals.
      } else if(decimalCount==1) {
        decimalChar = decimalString->char32At(0); // Look for this decimal
      } else if(decimalCount==0) {
        decimalChar=0; // NO decimal set
      } else {
        j=l+1;//Set counter to end of line, so that we break. Unknown decimal situation.
      }

#ifdef FMT_DEBUG
      printf("Preparing to do fastpath parse: decimalChar=U+%04X, groupingChar=U+%04X, first ch=U+%04X intOnly=%c strictParse=%c\n",
        decimalChar, groupingChar, ch,
        (intOnly)?'y':'n',
        (strictParse)?'y':'n');
#endif
      if(ch==0x002D) { // '-'
        j=l+1;//=break - negative number.
      } else {
        parsedNum.append('+',err);
      }
      while(j<l) {
        int32_t digit = ch - zero;
        if(digit >=0 && digit <= 9) {
          parsedNum.append((char)(digit + '0'), err);
          if((digitCount>0) || digit!=0 || j==(l-1)) {
            digitCount++;
          }
        } else if(ch == 0) { // break out
          digitCount=-1;
          break;
        } else if(ch == decimalChar) {
          parsedNum.append((char)('.'), err);
          decimalChar=0; // no more decimals.
          // fastParseHadDecimal=TRUE;
        } else if(ch == lookForGroup) {
          // ignore grouping char. No decimals, so it has to be an ignorable grouping sep
        } else if(intOnly && (lookForGroup!=0) && !u_isdigit(ch)) {
          // parsing integer only and can fall through
        } else {
          digitCount=-1; // fail - fall through to slow parse
          break;
        }
        j+=U16_LENGTH(ch);
        ch = text.char32At(j); // for next  
      }
      if(
         ((j==l)||intOnly) // end OR only parsing integer
         && (digitCount>0)) { // and have at least one digit
#ifdef FMT_DEBUG
        printf("PP -> %d, good = [%s]  digitcount=%d, fGroupingSize=%d fGroupingSize2=%d!\n", j, parsedNum.data(), digitCount, fGroupingSize, fGroupingSize2);
#endif
        fastParseOk=true; // Fast parse OK!

#ifdef SKIP_OPT
        debug("SKIP_OPT");
        /* for testing, try it the slow way. also */
        fastParseOk=false;
        parsedNum.clear();
#else
        parsePosition.setIndex(position=j);
        status[fgStatusInfinite]=false;
#endif
      } else {
        // was not OK. reset, retry
#ifdef FMT_DEBUG
        printf("Fall through: j=%d, l=%d, digitCount=%d\n", j, l, digitCount);
#endif
        parsedNum.clear();
      }
    } else {
#ifdef FMT_DEBUG
      printf("Could not fastpath parse. ");
      printf("fFormatWidth=%d ", formatWidth);
      printf("text.length()=%d ", text.length());
      printf("posPrefix=%p posSuffix=%p ", posPrefix, posSuffix);

      printf("\n");
#endif
    }

  UnicodeString formatPattern;
  toPattern(formatPattern);

  if(!fastParseOk)
  {
    // Match padding before prefix
    if (getOldFormatWidth() > 0 && fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix) {
        position = skipPadding(text, position);
    }

    // Match positive and negative prefixes; prefer longest match.
    int32_t posMatch = compareAffix(text, position, FALSE, TRUE, posPrefix, complexCurrencyParsing, type, currency);
    int32_t negMatch = compareAffix(text, position, TRUE,  TRUE, negPrefix, complexCurrencyParsing, type, currency);
    if (posMatch >= 0 && negMatch >= 0) {
        if (posMatch > negMatch) {
            negMatch = -1;
        } else if (negMatch > posMatch) {
            posMatch = -1;
        }
    }
    if (posMatch >= 0) {
        position += posMatch;
        parsedNum.append('+', err);
    } else if (negMatch >= 0) {
        position += negMatch;
        parsedNum.append('-', err);
    } else if (strictParse){
        parsePosition.setErrorIndex(position);
        return FALSE;
    } else {
        // Temporary set positive. This might be changed after checking suffix
        parsedNum.append('+', err);
    }

    // Match padding before prefix
    int32_t formatWidth = getOldFormatWidth();
    if (formatWidth > 0 && fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix) {
        position = skipPadding(text, position);
    }

    if (! strictParse) {
        position = skipUWhiteSpace(text, position);
    }

    // process digits or Inf, find decimal position
    const UnicodeString *inf = &getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);
    int32_t infLen = (text.compare(position, inf->length(), *inf)
        ? 0 : inf->length());
    position += infLen; // infLen is non-zero when it does equal to infinity
    status[fgStatusInfinite] = infLen != 0;

    if (infLen != 0) {
        parsedNum.append("Infinity", err);
    } else {
        // We now have a string of digits, possibly with grouping symbols,
        // and decimal points.  We want to process these into a DigitList.
        // We don't want to put a bunch of leading zeros into the DigitList
        // though, so we keep track of the location of the decimal point,
        // put only significant digits into the DigitList, and adjust the
        // exponent as needed.


        UBool strictFail = FALSE; // did we exit with a strict parse failure?
        int32_t lastGroup = -1; // where did we last see a grouping separator?
        int32_t digitStart = position;
        int32_t gs2 = fEffGrouping.fGrouping2 == 0 ? fEffGrouping.fGrouping : fEffGrouping.fGrouping2;

        const UnicodeString *decimalString;
        if (fMonetary) {
            decimalString = &getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
        } else {
            decimalString = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
        }
        UChar32 decimalChar = decimalString->char32At(0);
        int32_t decimalStringLength = decimalString->length();
        int32_t decimalCharLength   = U16_LENGTH(decimalChar);

        UBool sawDecimal = FALSE;
        UChar32 sawDecimalChar = 0xFFFF;
        UBool sawGrouping = FALSE;
        UChar32 sawGroupingChar = 0xFFFF;
        UBool sawDigit = FALSE;
        int32_t backup = -1;
        int32_t digit;

        // equivalent grouping and decimal support
        const UnicodeSet *decimalSet = NULL;
        const UnicodeSet *groupingSet = NULL;

        if (decimalCharLength == decimalStringLength) {
            decimalSet = DecimalFormatStaticSets::getSimilarDecimals(decimalChar, strictParse);
        }

        if (groupingCharLength == groupingStringLength) {
            if (strictParse) {
                groupingSet = fStaticSets->fStrictDefaultGroupingSeparators;
            } else {
                groupingSet = fStaticSets->fDefaultGroupingSeparators;
            }
        }

        // We need to test groupingChar and decimalChar separately from groupingSet and decimalSet, if the sets are even initialized.
        // If sawDecimal is TRUE, only consider sawDecimalChar and NOT decimalSet
        // If a character matches decimalSet, don't consider it to be a member of the groupingSet.

        // We have to track digitCount ourselves, because digits.fCount will
        // pin when the maximum allowable digits is reached.
        int32_t digitCount = 0;
        int32_t integerDigitCount = 0;

        for (; position < textLength; )
        {
            UChar32 ch = text.char32At(position);

            /* We recognize all digit ranges, not only the Latin digit range
             * '0'..'9'.  We do so by using the Character.digit() method,
             * which converts a valid Unicode digit to the range 0..9.
             *
             * The character 'ch' may be a digit.  If so, place its value
             * from 0 to 9 in 'digit'.  First try using the locale digit,
             * which may or MAY NOT be a standard Unicode digit range.  If
             * this fails, try using the standard Unicode digit ranges by
             * calling Character.digit().  If this also fails, digit will 
             * have a value outside the range 0..9.
             */
            digit = ch - zero;
            if (digit < 0 || digit > 9)
            {
                digit = u_charDigitValue(ch);
            }
            
            // As a last resort, look through the localized digits if the zero digit
            // is not a "standard" Unicode digit.
            if ( (digit < 0 || digit > 9) && u_charDigitValue(zero) != 0) {
                digit = 0;
                if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kZeroDigitSymbol)).char32At(0) == ch ) {
                    break;
                }
                for (digit = 1 ; digit < 10 ; digit++ ) {
                    if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kOneDigitSymbol+digit-1)).char32At(0) == ch ) {
                        break;
                    }
                }
            }

            if (digit >= 0 && digit <= 9)
            {
                if (strictParse && backup != -1) {
                    // comma followed by digit, so group before comma is a
                    // secondary group.  If there was a group separator
                    // before that, the group must == the secondary group
                    // length, else it can be <= the the secondary group
                    // length.
                    if ((lastGroup != -1 && backup - lastGroup - 1 != gs2) ||
                        (lastGroup == -1 && position - digitStart - 1 > gs2)) {
                        strictFail = TRUE;
                        break;
                    }
                    
                    lastGroup = backup;
                }
                
                // Cancel out backup setting (see grouping handler below)
                backup = -1;
                sawDigit = TRUE;
                
                // Note: this will append leading zeros
                parsedNum.append((char)(digit + '0'), err);

                // count any digit that's not a leading zero
                if (digit > 0 || digitCount > 0 || sawDecimal) {
                    digitCount += 1;
                    
                    // count any integer digit that's not a leading zero
                    if (! sawDecimal) {
                        integerDigitCount += 1;
                    }
                }
                    
                position += U16_LENGTH(ch);
            }
            else if (groupingStringLength > 0 && 
                matchGrouping(groupingChar, sawGrouping, sawGroupingChar, groupingSet, 
                            decimalChar, decimalSet,
                            ch) && groupingUsed)
            {
                if (sawDecimal) {
                    break;
                }

                if (strictParse) {
                    if ((!sawDigit || backup != -1)) {
                        // leading group, or two group separators in a row
                        strictFail = TRUE;
                        break;
                    }
                }

                // Ignore grouping characters, if we are using them, but require
                // that they be followed by a digit.  Otherwise we backup and
                // reprocess them.
                backup = position;
                position += groupingStringLength;
                sawGrouping=TRUE;
                // Once we see a grouping character, we only accept that grouping character from then on.
                sawGroupingChar=ch;
            }
            else if (matchDecimal(decimalChar,sawDecimal,sawDecimalChar, decimalSet, ch))
            {
                if (strictParse) {
                    if (backup != -1 ||
                        (lastGroup != -1 && position - lastGroup != fEffGrouping.fGrouping + 1)) {
                        strictFail = TRUE;
                        break;
                    }
                }

                // If we're only parsing integers, or if we ALREADY saw the
                // decimal, then don't parse this one.
                if (isParseIntegerOnly() || sawDecimal) {
                    break;
                }

                parsedNum.append('.', err);
                position += decimalStringLength;
                sawDecimal = TRUE;
                // Once we see a decimal character, we only accept that decimal character from then on.
                sawDecimalChar=ch;
                // decimalSet is considered to consist of (ch,ch)
            }
            else {

                if(!fParseNoExponent || // don't parse if this is set unless..
                   isScientificNotation()) { // .. it's an exponent format - ignore setting and parse anyways
                    const UnicodeString *tmp;
                    tmp = &getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
                    // TODO: CASE
                    if (!text.caseCompare(position, tmp->length(), *tmp, U_FOLD_CASE_DEFAULT))    // error code is set below if !sawDigit 
                    {
                        // Parse sign, if present
                        int32_t pos = position + tmp->length();
                        char exponentSign = '+';

                        if (pos < textLength)
                        {
                            tmp = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                            if (!text.compare(pos, tmp->length(), *tmp))
                            {
                                pos += tmp->length();
                            }
                            else {
                                tmp = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                                if (!text.compare(pos, tmp->length(), *tmp))
                                {
                                    exponentSign = '-';
                                    pos += tmp->length();
                                }
                            }
                        }

                        UBool sawExponentDigit = FALSE;
                        while (pos < textLength) {
                            ch = text[(int32_t)pos];
                            digit = ch - zero;

                            if (digit < 0 || digit > 9) {
                                digit = u_charDigitValue(ch);
                            }
                            if (0 <= digit && digit <= 9) {
                                if (!sawExponentDigit) {
                                    parsedNum.append('E', err);
                                    parsedNum.append(exponentSign, err);
                                    sawExponentDigit = TRUE;
                                }
                                ++pos;
                                parsedNum.append((char)(digit + '0'), err);
                            } else {
                                break;
                            }
                        }

                        if (sawExponentDigit) {
                            position = pos; // Advance past the exponent
                        }

                        break; // Whether we fail or succeed, we exit this loop
                    } else {
                        break;
                    }
                } else { // not parsing exponent
                    break;
              }
            }
        }

        // if we didn't see a decimal and it is required, check to see if the pattern had one
        if(!sawDecimal && fParseDecimalMarkRequired) 
        {
            if(formatPattern.indexOf(DecimalFormatSymbols::kDecimalSeparatorSymbol) != 0) 
            {
                parsePosition.setIndex(oldStart);
                parsePosition.setErrorIndex(position);
                debug("decimal point match required fail!");
                return FALSE;
            }
        }

        if (backup != -1)
        {
            position = backup;
        }

        if (strictParse && !sawDecimal) {
            if (lastGroup != -1 && position - lastGroup != fEffGrouping.fGrouping + 1) {
                strictFail = TRUE;
            }
        }

        if (strictFail) {
            // only set with strictParse and a grouping separator error

            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(position);
            debug("strictFail!");
            return FALSE;
        }

        // If there was no decimal point we have an integer

        // If none of the text string was recognized.  For example, parse
        // "x" with pattern "#0.00" (return index and error index both 0)
        // parse "$" with pattern "$#0.00". (return index 0 and error index
        // 1).
        if (!sawDigit && digitCount == 0) {
#ifdef FMT_DEBUG
            debug("none of text rec");
            printf("position=%d\n",position);
#endif
            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(oldStart);
            return FALSE;
        }
    }

    // Match padding before suffix
    if (formatWidth > 0 && fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix) {
        position = skipPadding(text, position);
    }

    int32_t posSuffixMatch = -1, negSuffixMatch = -1;

    // Match positive and negative suffixes; prefer longest match.
    if (posMatch >= 0 || (!strictParse && negMatch < 0)) {
        posSuffixMatch = compareAffix(text, position, FALSE, FALSE, posSuffix, complexCurrencyParsing, type, currency);
    }
    if (negMatch >= 0) {
        negSuffixMatch = compareAffix(text, position, TRUE, FALSE, negSuffix, complexCurrencyParsing, type, currency);
    }
    if (posSuffixMatch >= 0 && negSuffixMatch >= 0) {
        if (posSuffixMatch > negSuffixMatch) {
            negSuffixMatch = -1;
        } else if (negSuffixMatch > posSuffixMatch) {
            posSuffixMatch = -1;
        }
    }

    // Fail if neither or both
    if (strictParse && ((posSuffixMatch >= 0) == (negSuffixMatch >= 0))) {
        parsePosition.setErrorIndex(position);
        debug("neither or both");
        return FALSE;
    }

    position += (posSuffixMatch >= 0 ? posSuffixMatch : (negSuffixMatch >= 0 ? negSuffixMatch : 0));

    // Match padding before suffix
    if (formatWidth > 0 && fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix) {
        position = skipPadding(text, position);
    }

    parsePosition.setIndex(position);

    parsedNum.data()[0] = (posSuffixMatch >= 0 || (!strictParse && negMatch < 0 && negSuffixMatch < 0)) ? '+' : '-';
#ifdef FMT_DEBUG
printf("PP -> %d, SLOW = [%s]!    pp=%d, os=%d, err=%s\n", position, parsedNum.data(), parsePosition.getIndex(),oldStart,u_errorName(err));
#endif
  } /* end SLOW parse */
  if(parsePosition.getIndex() == oldStart)
    {
#ifdef FMT_DEBUG
      printf(" PP didnt move, err\n");
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
    // uint32_t bits = (fastParseOk?kFastpathOk:0) |
    //   (fastParseHadDecimal?0:kNoDecimal);
    //printf("FPOK=%d, FPHD=%d, bits=%08X\n", fastParseOk, fastParseHadDecimal, bits);
    digits.set(parsedNum.toStringPiece(),
               err,
               0//bits
               );

    if (U_FAILURE(err)) {
#ifdef FMT_DEBUG
      printf(" err setting %s\n", u_errorName(err));
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }

    // check if we missed a required decimal point
    if(fastParseOk && fParseDecimalMarkRequired) 
    {
        if(formatPattern.indexOf(DecimalFormatSymbols::kDecimalSeparatorSymbol) != 0) 
        {
            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(position);
            debug("decimal point match required fail!");
            return FALSE;
        }
    }


    return TRUE;
}

/**
 * Starting at position, advance past a run of pad characters, if any.
 * Return the index of the first character after position that is not a pad
 * character.  Result is >= position.
 */
int32_t DecimalFormat2::skipPadding(const UnicodeString& text, int32_t position) const {
    int32_t padLen = U16_LENGTH(fAap.fPadChar);
    while (position < text.length() &&
           text.char32At(position) == fAap.fPadChar) {
        position += padLen;
    }
    return position;
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * Runs of white space in the affix, match runs of white space in
 * the input.  Pattern white space and input white space are
 * determined differently; see code.
 * @param text input text
 * @param pos offset into input at which to begin matching
 * @param isNegative
 * @param isPrefix
 * @param affixPat affix pattern used for currency affix comparison.
 * @param complexCurrencyParsing whether it is currency parsing or not
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or null for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat2::compareAffix(const UnicodeString& text,
                                    int32_t pos,
                                    UBool isNegative,
                                    UBool isPrefix,
                                    const UnicodeString* affixPat,
                                    UBool complexCurrencyParsing,
                                    int8_t type,
                                    UChar* currency) const
{
    const UnicodeString *patternToCompare;
    if (currency != NULL ||
        (fMonetary && complexCurrencyParsing)) {

        if (affixPat != NULL) {
            return compareComplexAffix(*affixPat, text, pos, type, currency);
        }
    }

    if (isNegative) {
        if (isPrefix) {
            patternToCompare = &fAap.fNegativePrefix.getOtherVariant().toString();
        }
        else {
            patternToCompare = &fAap.fNegativeSuffix.getOtherVariant().toString();
        }
    }
    else {
        if (isPrefix) {
            patternToCompare = &fAap.fPositivePrefix.getOtherVariant().toString();
        }
        else {
            patternToCompare = &fAap.fPositiveSuffix.getOtherVariant().toString();
        }
    }
    return compareSimpleAffix(*patternToCompare, text, pos, isLenient());
}

UBool DecimalFormat2::equalWithSignCompatibility(UChar32 lhs, UChar32 rhs) const {
    if (lhs == rhs) {
        return TRUE;
    }
    U_ASSERT(fStaticSets != NULL); // should already be loaded
    const UnicodeSet *minusSigns = fStaticSets->fMinusSigns;
    const UnicodeSet *plusSigns = fStaticSets->fPlusSigns;
    return (minusSigns->contains(lhs) && minusSigns->contains(rhs)) ||
        (plusSigns->contains(lhs) && plusSigns->contains(rhs));
}
// check for LRM 0x200E, RLM 0x200F, ALM 0x061C
#define IS_BIDI_MARK(c) (c==0x200E || c==0x200F || c==0x061C)

#define TRIM_BUFLEN 32
UnicodeString& DecimalFormat2::trimMarksFromAffix(const UnicodeString& affix, UnicodeString& trimmedAffix) {
    UChar trimBuf[TRIM_BUFLEN];
    int32_t affixLen = affix.length();
    int32_t affixPos, trimLen = 0;

    for (affixPos = 0; affixPos < affixLen; affixPos++) {
        UChar c = affix.charAt(affixPos);
        if (!IS_BIDI_MARK(c)) {
            if (trimLen < TRIM_BUFLEN) {
                trimBuf[trimLen++] = c;
            } else {
                trimLen = 0;
                break;
            }
        }
    }
    return (trimLen > 0)? trimmedAffix.setTo(trimBuf, trimLen): trimmedAffix.setTo(affix);
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * Runs of white space in the affix, match runs of white space in
 * the input.  Pattern white space and input white space are
 * determined differently; see code.
 * @param affix pattern string, taken as a literal
 * @param input input text
 * @param pos offset into input at which to begin matching
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat2::compareSimpleAffix(const UnicodeString& affix,
                                          const UnicodeString& input,
                                          int32_t pos,
                                          UBool lenient) const {
    int32_t start = pos;
    UnicodeString trimmedAffix;
    // For more efficiency we should keep lazily-created trimmed affixes around in
    // instance variables instead of trimming each time they are used (the next step)
    trimMarksFromAffix(affix, trimmedAffix);
    UChar32 affixChar = trimmedAffix.char32At(0);
    int32_t affixLength = trimmedAffix.length();
    int32_t inputLength = input.length();
    int32_t affixCharLength = U16_LENGTH(affixChar);
    UnicodeSet *affixSet;
    UErrorCode status = U_ZERO_ERROR;

    U_ASSERT(fStaticSets != NULL); // should already be loaded

    if (U_FAILURE(status)) {
        return -1;
    }
    if (!lenient) {
        affixSet = fStaticSets->fStrictDashEquivalents;

        // If the trimmedAffix is exactly one character long and that character
        // is in the dash set and the very next input character is also
        // in the dash set, return a match.
        if (affixCharLength == affixLength && affixSet->contains(affixChar))  {
            UChar32 ic = input.char32At(pos);
            if (affixSet->contains(ic)) {
                pos += U16_LENGTH(ic);
                pos = skipBidiMarks(input, pos); // skip any trailing bidi marks
                return pos - start;
            }
        }

        for (int32_t i = 0; i < affixLength; ) {
            UChar32 c = trimmedAffix.char32At(i);
            int32_t len = U16_LENGTH(c);
            if (PatternProps::isWhiteSpace(c)) {
                // We may have a pattern like: \u200F \u0020
                //        and input text like: \u200F \u0020
                // Note that U+200F and U+0020 are Pattern_White_Space but only
                // U+0020 is UWhiteSpace.  So we have to first do a direct
                // match of the run of Pattern_White_Space in the pattern,
                // then match any extra characters.
                UBool literalMatch = FALSE;
                while (pos < inputLength) {
                    UChar32 ic = input.char32At(pos);
                    if (ic == c) {
                        literalMatch = TRUE;
                        i += len;
                        pos += len;
                        if (i == affixLength) {
                            break;
                        }
                        c = trimmedAffix.char32At(i);
                        len = U16_LENGTH(c);
                        if (!PatternProps::isWhiteSpace(c)) {
                            break;
                        }
                    } else if (IS_BIDI_MARK(ic)) {
                        pos ++; // just skip over this input text
                    } else {
                        break;
                    }
                }

                // Advance over run in pattern
                i = skipPatternWhiteSpace(trimmedAffix, i);

                // Advance over run in input text
                // Must see at least one white space char in input,
                // unless we've already matched some characters literally.
                int32_t s = pos;
                pos = skipUWhiteSpace(input, pos);
                if (pos == s && !literalMatch) {
                    return -1;
                }

                // If we skip UWhiteSpace in the input text, we need to skip it in the pattern.
                // Otherwise, the previous lines may have skipped over text (such as U+00A0) that
                // is also in the trimmedAffix.
                i = skipUWhiteSpace(trimmedAffix, i);
            } else {
                UBool match = FALSE;
                while (pos < inputLength) {
                    UChar32 ic = input.char32At(pos);
                    if (!match && ic == c) {
                        i += len;
                        pos += len;
                        match = TRUE;
                    } else if (IS_BIDI_MARK(ic)) {
                        pos++; // just skip over this input text
                    } else {
                        break;
                    }
                }
                if (!match) {
                    return -1;
                }
            }
        }
    } else {
        UBool match = FALSE;

        affixSet = fStaticSets->fDashEquivalents;

        if (affixCharLength == affixLength && affixSet->contains(affixChar))  {
            pos = skipUWhiteSpaceAndMarks(input, pos);
            UChar32 ic = input.char32At(pos);

            if (affixSet->contains(ic)) {
                pos += U16_LENGTH(ic);
                pos = skipBidiMarks(input, pos);
                return pos - start;
            }
        }

        for (int32_t i = 0; i < affixLength; )
        {
            //i = skipRuleWhiteSpace(trimmedAffix, i);
            i = skipUWhiteSpace(trimmedAffix, i);
            pos = skipUWhiteSpaceAndMarks(input, pos);

            if (i >= affixLength || pos >= inputLength) {
                break;
            }

            UChar32 c = trimmedAffix.char32At(i);
            UChar32 ic = input.char32At(pos);

            if (!equalWithSignCompatibility(ic, c)) {
                return -1;
            }

            match = TRUE;
            i += U16_LENGTH(c);
            pos += U16_LENGTH(ic);
            pos = skipBidiMarks(input, pos);
        }

        if (affixLength > 0 && ! match) {
            return -1;
        }
    }
    return pos - start;
}

/**
 * Skip over a run of zero or more Pattern_White_Space characters at
 * pos in text.
 */
int32_t DecimalFormat2::skipPatternWhiteSpace(const UnicodeString& text, int32_t pos) {
    const UChar* s = text.getBuffer();
    return (int32_t)(PatternProps::skipWhiteSpace(s + pos, text.length() - pos) - s);
}

/**
 * Skip over a run of zero or more isUWhiteSpace() characters at pos
 * in text.
 */
int32_t DecimalFormat2::skipUWhiteSpace(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

/**
 * Skip over a run of zero or more isUWhiteSpace() characters or bidi marks at pos
 * in text.
 */
int32_t DecimalFormat2::skipUWhiteSpaceAndMarks(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c) && !IS_BIDI_MARK(c)) { // u_isUWhiteSpace doesn't include LRM,RLM,ALM
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

/**
 * Skip over a run of zero or more bidi marks at pos in text.
 */
int32_t DecimalFormat2::skipBidiMarks(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar c = text.charAt(pos);
        if (!IS_BIDI_MARK(c)) {
            break;
        }
        pos++;
    }
    return pos;
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * @param affixPat pattern string
 * @param input input text
 * @param pos offset into input at which to begin matching
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or null for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat2::compareComplexAffix(const UnicodeString& affixPat,
                                           const UnicodeString& text,
                                           int32_t pos,
                                           int8_t type,
                                           UChar* currency) const
{
    int32_t start = pos;
    U_ASSERT(currency != NULL || fMonetary);

    for (int32_t i=0;
         i<affixPat.length() && pos >= 0; ) {
        UChar32 c = affixPat.char32At(i);
        i += U16_LENGTH(c);

        if (c == kQuote) {
            U_ASSERT(i <= affixPat.length());
            c = affixPat.char32At(i);
            i += U16_LENGTH(c);

            const UnicodeString* affix = NULL;

            switch (c) {
            case kCurrencySign: {
                // since the currency names in choice format is saved
                // the same way as other currency names,
                // do not need to do currency choice parsing here.
                // the general currency parsing parse against all names,
                // including names in choice format.
                UBool intl = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (intl) {
                    ++i;
                }
                UBool plural = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (plural) {
                    ++i;
                    intl = FALSE;
                }
                // Parse generic currency -- anything for which we
                // have a display name, or any 3-letter ISO code.
                // Try to parse display name for our locale; first
                // determine our locale.
                const char* loc = fSymbols->getLocale().getName();
                ParsePosition ppos(pos);
                UChar curr[4];
                UErrorCode ec = U_ZERO_ERROR;
                // Delegate parse of display name => ISO code to Currency
                uprv_parseCurrency(loc, text, ppos, type, curr, ec);

                // If parse succeeds, populate currency[0]
                if (U_SUCCESS(ec) && ppos.getIndex() != pos) {
                    if (currency) {
                        u_strcpy(currency, curr);
                    } else {
                        // The formatter is currency-style but the client has not requested
                        // the value of the parsed currency. In this case, if that value does
                        // not match the formatter's current value, then the parse fails.
                        UChar effectiveCurr[4];
                        getEffectiveCurrency(effectiveCurr, ec);
                        if ( U_FAILURE(ec) || u_strncmp(curr,effectiveCurr,4) != 0 ) {
                            pos = -1;
                            continue;
                        }
                    }
                    pos = ppos.getIndex();
                } else if (!isLenient()){
                    pos = -1;
                }
                continue;
            }
            case kPatternPercent:
                affix = &getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                break;
            case kPatternPerMill:
                affix = &getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                break;
            case kPatternPlus:
                affix = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                break;
            case kPatternMinus:
                affix = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                break;
            default:
                // fall through to affix!=0 test, which will fail
                break;
            }

            if (affix != NULL) {
                pos = match(text, pos, *affix);
                continue;
            }
        }

        pos = match(text, pos, c);
        if (PatternProps::isWhiteSpace(c)) {
            i = skipPatternWhiteSpace(affixPat, i);
        }
    }
    return pos - start;
}

/**
 * Match a single character at text[pos] and return the index of the
 * next character upon success.  Return -1 on failure.  If
 * ch is a Pattern_White_Space then match a run of white space in text.
 */
int32_t DecimalFormat2::match(const UnicodeString& text, int32_t pos, UChar32 ch) {
    if (PatternProps::isWhiteSpace(ch)) {
        // Advance over run of white space in input text
        // Must see at least one white space char in input
        int32_t s = pos;
        pos = skipPatternWhiteSpace(text, pos);
        if (pos == s) {
            return -1;
        }
        return pos;
    }
    return (pos >= 0 && text.char32At(pos) == ch) ?
        (pos + U16_LENGTH(ch)) : -1;
}

/**
 * Match a string at text[pos] and return the index of the next
 * character upon success.  Return -1 on failure.  Match a run of
 * white space in str with a run of white space in text.
 */
int32_t DecimalFormat2::match(const UnicodeString& text, int32_t pos, const UnicodeString& str) {
    for (int32_t i=0; i<str.length() && pos >= 0; ) {
        UChar32 ch = str.char32At(i);
        i += U16_LENGTH(ch);
        if (PatternProps::isWhiteSpace(ch)) {
            i = skipPatternWhiteSpace(str, i);
        }
        pos = match(text, pos, ch);
    }
    return pos;
}

UBool DecimalFormat2::matchSymbol(const UnicodeString &text, int32_t position, int32_t length, const UnicodeString &symbol,
                         UnicodeSet *sset, UChar32 schar)
{
    if (sset != NULL) {
        return sset->contains(schar);
    }

    return text.compare(position, length, symbol) == 0;
}

UBool DecimalFormat2::matchDecimal(UChar32 symbolChar,
                            UBool sawDecimal,  UChar32 sawDecimalChar,
                             const UnicodeSet *sset, UChar32 schar) {
   if(sawDecimal) {
       return schar==sawDecimalChar;
   } else if(schar==symbolChar) {
       return TRUE;
   } else if(sset!=NULL) {
        return sset->contains(schar);
   } else {
       return FALSE;
   }
}

UBool DecimalFormat2::matchGrouping(UChar32 groupingChar,
                            UBool sawGrouping, UChar32 sawGroupingChar,
                             const UnicodeSet *sset,
                             UChar32 /*decimalChar*/, const UnicodeSet *decimalSet,
                             UChar32 schar) {
    if(sawGrouping) {
        return schar==sawGroupingChar;  // previously found
    } else if(schar==groupingChar) {
        return TRUE; // char from symbols
    } else if(sset!=NULL) {
        return sset->contains(schar) &&  // in groupingSet but...
           ((decimalSet==NULL) || !decimalSet->contains(schar)); // Exclude decimalSet from groupingSet
    } else {
        return FALSE;
    }
}



U_NAMESPACE_END



