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
        return fPrecision.fMantissa.fMin.getIntDigitCount(); }
int32_t getMaximumIntegerDigits() const { 
        return fPrecision.fMantissa.fMax.getIntDigitCount(); }
int32_t getMinimumFractionDigits() const { 
        return fPrecision.fMantissa.fMin.getFracDigitCount(); }
int32_t getMaximumFractionDigits() const { 
        return fPrecision.fMantissa.fMax.getFracDigitCount(); }
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
// These fields are the state that the user can see and set.
// They alone determine the behavior of this object
ScientificPrecision fPrecision;  // encapsulates fixed precision settings
SciFormatterOptions fOptions;   // Encapsulates fixed precision options
UBool fUseScientific;
DigitGrouping fGrouping;
UBool fUseGrouping;
AffixPattern fPositivePrefixPattern;
AffixPattern fNegativePrefixPattern;
AffixPattern fPositiveSuffixPattern;
AffixPattern fNegativeSuffixPattern;
DecimalFormatSymbols *fSymbols;
UChar fCurr[4];

// This field is totally hidden from user and is used to derive the affixes
// in fAap below from the four affix patterns above. This field is set
// from fSymbols above.
AffixPatternParser fAffixParser;

// These fields are used to format numbers. These fields are derived from
// the first two groups of fields. Their settings are entirely hidden from
// the user.
ScientificPrecision fEffPrecision;
DigitGrouping fEffGrouping;
SciFormatter fSciFormatter;
DigitFormatter fFormatter;
DigitAffixesAndPadding fAap;
PluralRules *fRules;

// How many places to move decimal to the left. Used for percent and permille.
int32_t fScale;

void parsePattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status);
void updateSymbols();
void updateLocalizedAffixes(UErrorCode &status);
void updateForFixedDecimal();
void updateIntDigitCounts();
void updateFracDigitCounts();
void updateSigDigitCounts();
void updateForScientific();
void updateLocalAffixes(UErrorCode &status);
UBool affixesUseCurrency();

};


U_NAMESPACE_END

#endif // _DECIMFMT2
//eof
