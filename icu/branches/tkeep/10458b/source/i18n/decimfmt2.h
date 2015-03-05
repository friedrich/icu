/*
********************************************************************************
*   Copyright (C) 2015, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File DECIMFMT2.H
********************************************************************************
*/

#ifndef DECIMFMT2_H
#define DECIMFMT2_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "affixpatternparser.h"
#include "sciformatter.h"
#include "digitformatter.h"
#include "digitgrouping.h"
#include "precision.h"
#include "digitaffixesandpadding.h"

U_NAMESPACE_BEGIN

class UnicodeString;
class FieldPosition;

class DecimalFormat2 : public UMemory {
public:

DecimalFormat2(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status);
DecimalFormat2(const DecimalFormat2 &other);
DecimalFormat2 &operator=(const DecimalFormat2 &other);
~DecimalFormat2();
UnicodeString &format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status);
void setMinimumIntegerDigits(int32_t newValue);
void setMaximumIntegerDigits(int32_t newValue);
void setMinimumFractionDigits(int32_t newValue);
void setMaximumFractionDigits(int32_t newValue);
void setScientificNotation(UBool newValue);
int32_t getMinimumIntegerDigits() const { 
        return fMinIntDigits; }
int32_t getMaximumIntegerDigits() const { 
        return fMaxIntDigits; }
int32_t getMinimumFractionDigits() const { 
        return fMinFracDigits; }
int32_t getMaximumFractionDigits() const { 
        return fMaxFracDigits; }
UBool isScientificNotation() const { return fUseScientific; }
void setGroupingSize(int32_t newValue);
void setSecondaryGroupingSize(int32_t newValue);
void setMinimumGroupingDigits(int32_t newValue);
void setGroupingUsed(UBool newValue);
int32_t getGroupingSize() const { return fGrouping.fGrouping; }
int32_t getSecondaryGroupingSize() const { return fGrouping.fGrouping2; }
int32_t getMinimumGroupingDigits() const { return fGrouping.fMinGrouping; }
UBool isGroupingUsed() const { return fUseGrouping; }
void setCurrency(const UChar *currency, UErrorCode &status);

private:
// These fields include what the user can see and set.
// When the user updates these fields, it triggers automatic updates of
// other fields that may be invisible to user

// Updating the following fields triggers an update to the fEffPrecision field
int32_t fMinIntDigits;
int32_t fMaxIntDigits;
int32_t fMinFracDigits;
int32_t fMaxFracDigits;
int32_t fMinSigDigits;
int32_t fMaxSigDigits;
UBool fUseScientific;

// Updating the following fields triggers an update to fEffGrouping field
DigitGrouping fGrouping;
UBool fUseGrouping;

// Updating the following fields triggers updates on the following:
// fFormatter, fSciFormatter, fUsesCurrency, fRules, fAffixParser,
// fAap.fPositivePrefiix, fAap.fPositiveSuffix,
// fAap.fNegativePrefiix, fAap.fNegativeSuffix,
AffixPattern fPositivePrefixPattern;
AffixPattern fNegativePrefixPattern;
AffixPattern fPositiveSuffixPattern;
AffixPattern fNegativeSuffixPattern;
DecimalFormatSymbols *fSymbols;
UChar fCurr[4];

// TRUE if one or more affixes use currency.
UBool fUsesCurrency;

// Optional may be NULL
PluralRules *fRules;

// This field is totally hidden from user and is used to derive the affixes
// in fAap below from the four affix patterns above.
AffixPatternParser fAffixParser;

// The actual precision used when formatting
ScientificPrecision fEffPrecision;

// The actual grouping used when formatting
DigitGrouping fEffGrouping;
SciFormatterOptions fOptions;   // Encapsulates fixed precision options
SciFormatter fSciFormatter;
DigitFormatter fFormatter;
DigitAffixesAndPadding fAap;

void parsePattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status);

// Updates everything
void updateAll(UErrorCode &status);

// Updates from changes to first group of attributes
void updatePrecision(int32_t flags);

// Updates from changes to second group of attributes
void updateGrouping(int32_t flags);

// Updates from changes to third group of attributes
void updateFormatting(int32_t flags, UErrorCode &status);

// Helper functions for updateFormattersAndAffixes
UBool updateUsesCurrency();
void updatePluralRules();
void updateAffixParser(int32_t flags);
void updateFormatters();
void updateLocalizedAffixes(int32_t flags);

};


U_NAMESPACE_END

#endif // _DECIMFMT2
//eof
