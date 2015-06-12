/*
*******************************************************************************
* Copyright (C) 1997-2015, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File DECIMFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/20/97    clhuang     Implemented with new APIs.
*   03/31/97    aliu        Moved isLONG_MIN to DigitList, and fixed it.
*   04/3/97     aliu        Rewrote parsing and formatting completely, and
*                           cleaned up and debugged.  Actually works now.
*                           Implemented NAN and INF handling, for both parsing
*                           and formatting.  Extensive testing & debugging.
*   04/10/97    aliu        Modified to compile on AIX.
*   04/16/97    aliu        Rewrote to use DigitList, which has been resurrected.
*                           Changed DigitCount to int per code review.
*   07/09/97    helena      Made ParsePosition into a class.
*   08/26/97    aliu        Extensive changes to applyPattern; completely
*                           rewritten from the Java.
*   09/09/97    aliu        Ported over support for exponential formats.
*   07/20/98    stephen     JDK 1.2 sync up.
*                             Various instances of '0' replaced with 'NULL'
*                             Check for grouping size in subFormat()
*                             Brought subParse() in line with Java 1.2
*                             Added method appendAffix()
*   08/24/1998  srl         Removed Mutex calls. This is not a thread safe class!
*   02/22/99    stephen     Removed character literals for EBCDIC safety
*   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
*   06/28/99    stephen     Fixed bugs in toPattern().
*   06/29/99    stephen     Fixed operator= to copy fFormatWidth, fPad,
*                             fPadPosition
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "fphdlimp.h"
#include "unicode/decimfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/dcfmtsym.h"
#include "unicode/ures.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/curramt.h"
#include "unicode/currpinf.h"
#include "unicode/plurrule.h"
#include "unicode/utf16.h"
#include "unicode/numsys.h"
#include "unicode/localpointer.h"
#include "uresimp.h"
#include "ucurrimp.h"
#include "charstr.h"
#include "cmemory.h"
#include "patternprops.h"
#include "digitlst.h"
#include "cstring.h"
#include "umutex.h"
#include "uassert.h"
#include "putilimp.h"
#include <math.h>
#include "hash.h"
#include "decfmtst.h"
#include "dcfmtimp.h"
#include "plurrule_impl.h"
#include "decimalformatpattern.h"
#include "fmtableimp.h"
#include "decimfmtimpl.h"

/*
 * On certain platforms, round is a macro defined in math.h
 * This undefine is to avoid conflict between the macro and
 * the function defined below.
 */
#ifdef round
#undef round
#endif


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


/* For currency parsing purose,
 * Need to remember all prefix patterns and suffix patterns of
 * every currency format pattern,
 * including the pattern of default currecny style
 * and plural currency style. And the patterns are set through applyPattern.
 */
struct AffixPatternsForCurrency : public UMemory {
	// negative prefix pattern
	UnicodeString negPrefixPatternForCurrency;
	// negative suffix pattern
	UnicodeString negSuffixPatternForCurrency;
	// positive prefix pattern
	UnicodeString posPrefixPatternForCurrency;
	// positive suffix pattern
	UnicodeString posSuffixPatternForCurrency;
	int8_t patternType;

	AffixPatternsForCurrency(const UnicodeString& negPrefix,
							 const UnicodeString& negSuffix,
							 const UnicodeString& posPrefix,
							 const UnicodeString& posSuffix,
							 int8_t type) {
		negPrefixPatternForCurrency = negPrefix;
		negSuffixPatternForCurrency = negSuffix;
		posPrefixPatternForCurrency = posPrefix;
		posSuffixPatternForCurrency = posSuffix;
		patternType = type;
	}
#ifdef FMT_DEBUG
  void dump() const  {
    debugout( UnicodeString("AffixPatternsForCurrency( -=\"") +
              negPrefixPatternForCurrency + (UnicodeString)"\"/\"" +
              negSuffixPatternForCurrency + (UnicodeString)"\" +=\"" + 
              posPrefixPatternForCurrency + (UnicodeString)"\"/\"" + 
              posSuffixPatternForCurrency + (UnicodeString)"\" )");
  }
#endif
};

/* affix for currency formatting when the currency sign in the pattern
 * equals to 3, such as the pattern contains 3 currency sign or
 * the formatter style is currency plural format style.
 */
struct AffixesForCurrency : public UMemory {
	// negative prefix
	UnicodeString negPrefixForCurrency;
	// negative suffix
	UnicodeString negSuffixForCurrency;
	// positive prefix
	UnicodeString posPrefixForCurrency;
	// positive suffix
	UnicodeString posSuffixForCurrency;

	int32_t formatWidth;

	AffixesForCurrency(const UnicodeString& negPrefix,
					   const UnicodeString& negSuffix,
					   const UnicodeString& posPrefix,
					   const UnicodeString& posSuffix) {
		negPrefixForCurrency = negPrefix;
		negSuffixForCurrency = negSuffix;
		posPrefixForCurrency = posPrefix;
		posSuffixForCurrency = posSuffix;
	}
#ifdef FMT_DEBUG
  void dump() const {
    debugout( UnicodeString("AffixesForCurrency( -=\"") +
              negPrefixForCurrency + (UnicodeString)"\"/\"" +
              negSuffixForCurrency + (UnicodeString)"\" +=\"" + 
              posPrefixForCurrency + (UnicodeString)"\"/\"" + 
              posSuffixForCurrency + (UnicodeString)"\" )");
  }
#endif
};

U_CDECL_BEGIN

/**
 * @internal ICU 4.2
 */
static UBool U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2);

/**
 * @internal ICU 4.2
 */
static UBool U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2);


static UBool
U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2) {
    const AffixesForCurrency* affix_1 =
        (AffixesForCurrency*)val1.pointer;
    const AffixesForCurrency* affix_2 =
        (AffixesForCurrency*)val2.pointer;
    return affix_1->negPrefixForCurrency == affix_2->negPrefixForCurrency &&
           affix_1->negSuffixForCurrency == affix_2->negSuffixForCurrency &&
           affix_1->posPrefixForCurrency == affix_2->posPrefixForCurrency &&
           affix_1->posSuffixForCurrency == affix_2->posSuffixForCurrency;
}


static UBool
U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2) {
    const AffixPatternsForCurrency* affix_1 =
        (AffixPatternsForCurrency*)val1.pointer;
    const AffixPatternsForCurrency* affix_2 =
        (AffixPatternsForCurrency*)val2.pointer;
    return affix_1->negPrefixPatternForCurrency ==
           affix_2->negPrefixPatternForCurrency &&
           affix_1->negSuffixPatternForCurrency ==
           affix_2->negSuffixPatternForCurrency &&
           affix_1->posPrefixPatternForCurrency ==
           affix_2->posPrefixPatternForCurrency &&
           affix_1->posSuffixPatternForCurrency ==
           affix_2->posSuffixPatternForCurrency &&
           affix_1->patternType == affix_2->patternType;
}

U_CDECL_END




// *****************************************************************************
// class DecimalFormat
// *****************************************************************************

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DecimalFormat)

// Constants for characters used in programmatic (unlocalized) patterns.
#define kPatternZeroDigit            ((UChar)0x0030) /*'0'*/
#define kPatternSignificantDigit     ((UChar)0x0040) /*'@'*/
#define kPatternGroupingSeparator    ((UChar)0x002C) /*','*/
#define kPatternDecimalSeparator     ((UChar)0x002E) /*'.'*/
#define kPatternPerMill              ((UChar)0x2030)
#define kPatternPercent              ((UChar)0x0025) /*'%'*/
#define kPatternDigit                ((UChar)0x0023) /*'#'*/
#define kPatternSeparator            ((UChar)0x003B) /*';'*/
#define kPatternExponent             ((UChar)0x0045) /*'E'*/
#define kPatternPlus                 ((UChar)0x002B) /*'+'*/
#define kPatternMinus                ((UChar)0x002D) /*'-'*/
#define kPatternPadEscape            ((UChar)0x002A) /*'*'*/
#define kQuote                       ((UChar)0x0027) /*'\''*/
/**
 * The CURRENCY_SIGN is the standard Unicode symbol for currency.  It
 * is used in patterns and substitued with either the currency symbol,
 * or if it is doubled, with the international currency symbol.  If the
 * CURRENCY_SIGN is seen in a pattern, then the decimal separator is
 * replaced with the monetary decimal separator.
 */
#define kCurrencySign                ((UChar)0x00A4)
#define kDefaultPad                  ((UChar)0x0020) /* */

const int32_t DecimalFormat::kDoubleIntegerDigits  = 309;
const int32_t DecimalFormat::kDoubleFractionDigits = 340;

const int32_t DecimalFormat::kMaxScientificIntegerDigits = 8;

/**
 * These are the tags we expect to see in normal resource bundle files associated
 * with a locale.
 */
const char DecimalFormat::fgNumberPatterns[]="NumberPatterns"; // Deprecated - not used
static const char fgNumberElements[]="NumberElements";
static const char fgLatn[]="latn";
static const char fgPatterns[]="patterns";
static const char fgDecimalFormat[]="decimalFormat";
static const char fgCurrencyFormat[]="currencyFormat";

static const UChar fgTripleCurrencySign[] = {0xA4, 0xA4, 0xA4, 0};

inline int32_t _min(int32_t a, int32_t b) { return (a<b) ? a : b; }
inline int32_t _max(int32_t a, int32_t b) { return (a<b) ? b : a; }

static void copyString(const UnicodeString& src, UBool isBogus, UnicodeString *& dest, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (isBogus) {
        delete dest;
        dest = NULL;
    } else {
        if (dest != NULL) {
            *dest = src;
        } else {
            dest = new UnicodeString(src);
            if (dest == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
    }
}


//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance in the default locale.

DecimalFormat::DecimalFormat(UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern in the default locale.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError, &pattern);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the default locale.  The
// created instance owns the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status, parseError, &pattern, symbolsToAdopt);
}

DecimalFormat::DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UParseError& parseErr,
                    UErrorCode& status) {
    init();
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status,parseErr, &pattern, symbolsToAdopt);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the default locale.  The
// created instance owns the clone of the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             const DecimalFormatSymbols& symbols,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError, &pattern, new DecimalFormatSymbols(symbols));
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern, the number format symbols, and the number format style.
// The created instance owns the clone of the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             UNumberFormatStyle style,
                             UErrorCode& status) {
    init();
    fStyle = style;
    UParseError parseError;
    construct(status, parseError, &pattern, symbolsToAdopt);
}

//-----------------------------------------------------------------------------
// Common DecimalFormat initialization.
//    Put all fields of an uninitialized object into a known state.
//    Common code, shared by all constructors.
//    Can not fail. Leave the object in good enough shape that the destructor
//    or assignment operator can run successfully.
void
DecimalFormat::init() {
    fPosPrefixPattern = 0;
    fPosSuffixPattern = 0;
    fNegPrefixPattern = 0;
    fNegSuffixPattern = 0;
    fCurrencyChoice = 0;
    fMultiplier = NULL;
    fScale = 0;
    fGroupingSize = 0;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fSymbols = NULL;
    fUseSignificantDigits = FALSE;
    fMinSignificantDigits = 1;
    fMaxSignificantDigits = 6;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;
    fExponentSignAlwaysShown = FALSE;
    fBoolFlags.clear();
    fRoundingIncrement = 0;
    fRoundingMode = kRoundHalfEven;
    fPad = 0;
    fFormatWidth = 0;
    fPadPosition = kPadBeforePrefix;
    fStyle = UNUM_DECIMAL;
    fCurrencySignCount = fgCurrencySignCountZero;
    fAffixPatternsForCurrency = NULL;
    fAffixesForCurrency = NULL;
    fPluralAffixesForCurrency = NULL;
    fCurrencyPluralInfo = NULL;
    fCurrencyUsage = UCURR_USAGE_STANDARD;
#if UCONFIG_HAVE_PARSEALLINPUT
    fParseAllInput = UNUM_MAYBE;
#endif

    fStaticSets = NULL;
    fImpl = NULL;
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the desired locale.  The
// created instance owns the symbols.

void
DecimalFormat::construct(UErrorCode&            status,
                         UParseError&           parseErr,
                         const UnicodeString*   pattern,
                         DecimalFormatSymbols*  symbolsToAdopt)
{
    fSymbols = symbolsToAdopt; // Do this BEFORE aborting on status failure!!!
    fRoundingIncrement = NULL;
    fRoundingMode = kRoundHalfEven;
    fPad = kDefaultPad;
    fPadPosition = kPadBeforePrefix;
    if (U_FAILURE(status))
        return;

    fPosPrefixPattern = fPosSuffixPattern = NULL;
    fNegPrefixPattern = fNegSuffixPattern = NULL;
    fGroupingSize = 3;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;
        
    if (fSymbols == NULL)
    {
        fSymbols = new DecimalFormatSymbols(Locale::getDefault(), status);
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    fStaticSets = DecimalFormatStaticSets::getStaticSets(status);
    if (U_FAILURE(status)) {
        return;
    }
    UErrorCode nsStatus = U_ZERO_ERROR;
    NumberingSystem *ns = NumberingSystem::createInstance(nsStatus);
    if (U_FAILURE(nsStatus)) {
        status = nsStatus;
        return;
    }


    UnicodeString str;
    // Uses the default locale's number format pattern if there isn't
    // one specified.
    if (pattern == NULL)
    {
        int32_t len = 0;
        UResourceBundle *top = ures_open(NULL, Locale::getDefault().getName(), &status);

        UResourceBundle *resource = ures_getByKeyWithFallback(top, fgNumberElements, NULL, &status);
        resource = ures_getByKeyWithFallback(resource, ns->getName(), resource, &status);
        resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &status);
        const UChar *resStr = ures_getStringByKeyWithFallback(resource, fgDecimalFormat, &len, &status);
        if ( status == U_MISSING_RESOURCE_ERROR && uprv_strcmp(fgLatn,ns->getName())) {
            status = U_ZERO_ERROR;
            resource = ures_getByKeyWithFallback(top, fgNumberElements, resource, &status);
            resource = ures_getByKeyWithFallback(resource, fgLatn, resource, &status);
            resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &status);
            resStr = ures_getStringByKeyWithFallback(resource, fgDecimalFormat, &len, &status);
        }
        str.setTo(TRUE, resStr, len);
        pattern = &str;
        ures_close(resource);
        ures_close(top);
    }

    // TODO(refactor): Just have this adopt symbolsToAdopt instead of copy it once
    // fSymbols goes away.
    fImpl = new DecimalFormatImpl(*pattern, new DecimalFormatSymbols(*fSymbols), parseErr, status);
    if (fImpl == NULL && U_SUCCESS(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        return;
    }
    setMultiplier(1);

    delete ns;

    if (U_FAILURE(status))
    {
        return;
    }

    if (pattern->indexOf((UChar)kCurrencySign) >= 0) {
        // If it looks like we are going to use a currency pattern
        // then do the time consuming lookup.
        setCurrencyForSymbols();
    } else {
        setCurrencyInternally(NULL, status);
    }

    const UnicodeString* patternUsed;
    UnicodeString currencyPluralPatternForOther;
    // apply pattern
    if (fStyle == UNUM_CURRENCY_PLURAL) {
        fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        if (U_FAILURE(status)) {
            return;
        }

        // the pattern used in format is not fixed until formatting,
        // in which, the number is known and
        // will be used to pick the right pattern based on plural count.
        // Here, set the pattern as the pattern of plural count == "other".
        // For most locale, the patterns are probably the same for all
        // plural count. If not, the right pattern need to be re-applied
        // during format.
        fCurrencyPluralInfo->getCurrencyPluralPattern(UNICODE_STRING("other", 5), currencyPluralPatternForOther);
        // TODO(refactor): Revisit, we are setting the pattern twice.
        fImpl->applyPattern(currencyPluralPatternForOther, status);
        patternUsed = &currencyPluralPatternForOther;
        // TODO: not needed?
        setCurrencyForSymbols();

    } else {
        patternUsed = pattern;
    }

    if (patternUsed->indexOf(kCurrencySign) != -1) {
        // initialize for currency, not only for plural format,
        // but also for mix parsing
        if (fCurrencyPluralInfo == NULL) {
           fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
           if (U_FAILURE(status)) {
               return;
           }
        }
        // need it for mix parsing
        setupCurrencyAffixPatterns(status);
        // expanded affixes for plural names
        if (patternUsed->indexOf(fgTripleCurrencySign, 3, 0) != -1) {
            setupCurrencyAffixes(*patternUsed, TRUE, TRUE, status);
        }
    }

    applyPatternWithoutExpandAffix(*patternUsed,FALSE, parseErr, status);

    // expand affixes
    if (fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        expandAffixAdjustWidth(NULL);
    }

    // If it was a currency format, apply the appropriate rounding by
    // resetting the currency. NOTE: this copies fCurrency on top of itself.
    if (fCurrencySignCount != fgCurrencySignCountZero) {
        setCurrencyInternally(getCurrency(), status);
    }
}

static void 
applyPatternWithNoSideEffects(
        const UnicodeString& pattern,
        UParseError& parseError,
        UnicodeString &negPrefix,
        UnicodeString &negSuffix,
        UnicodeString &posPrefix,
        UnicodeString &posSuffix,
        UErrorCode& status) {
        if (U_FAILURE(status))
    {    
        return;
    }    
    DecimalFormatPatternParser patternParser;
    DecimalFormatPattern out; 
    patternParser.applyPatternWithoutExpandAffix(
        pattern,
        out, 
        parseError,
        status);
    if (U_FAILURE(status)) {
      return;
    }    
    negPrefix = out.fNegPrefixPattern;
    negSuffix = out.fNegSuffixPattern;
    posPrefix = out.fPosPrefixPattern;
    posSuffix = out.fPosSuffixPattern;
}

void
DecimalFormat::setupCurrencyAffixPatterns(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    fAffixPatternsForCurrency = initHashForAffixPattern(status);
    if (U_FAILURE(status)) {
        return;
    }

    NumberingSystem *ns = NumberingSystem::createInstance(fSymbols->getLocale(),status);
    if (U_FAILURE(status)) {
        return;
    }

    // Save the default currency patterns of this locale.
    // Here, chose onlyApplyPatternWithoutExpandAffix without
    // expanding the affix patterns into affixes.
    UnicodeString currencyPattern;
    UErrorCode error = U_ZERO_ERROR;   
    
    UResourceBundle *resource = ures_open(NULL, fSymbols->getLocale().getName(), &error);
    UResourceBundle *numElements = ures_getByKeyWithFallback(resource, fgNumberElements, NULL, &error);
    resource = ures_getByKeyWithFallback(numElements, ns->getName(), resource, &error);
    resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &error);
    int32_t patLen = 0;
    const UChar *patResStr = ures_getStringByKeyWithFallback(resource, fgCurrencyFormat,  &patLen, &error);
    if ( error == U_MISSING_RESOURCE_ERROR && uprv_strcmp(ns->getName(),fgLatn)) {
        error = U_ZERO_ERROR;
        resource = ures_getByKeyWithFallback(numElements, fgLatn, resource, &error);
        resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &error);
        patResStr = ures_getStringByKeyWithFallback(resource, fgCurrencyFormat,  &patLen, &error);
    }
    ures_close(numElements);
    ures_close(resource);
    delete ns;

    if (U_SUCCESS(error)) {
        UnicodeString negPrefix;
        UnicodeString negSuffix;
        UnicodeString posPrefix;
        UnicodeString posSuffix;
        applyPatternWithNoSideEffects(UnicodeString(patResStr, patLen),
                                       parseErr,
                negPrefix, negSuffix, posPrefix, posSuffix,  status);
        AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    negPrefix,
                                                    negSuffix,
                                                    posPrefix,
                                                    posSuffix,
                                                    UCURR_SYMBOL_NAME);
        fAffixPatternsForCurrency->put(UNICODE_STRING("default", 7), affixPtn, status);
    }

    // save the unique currency plural patterns of this locale.
    Hashtable* pluralPtn = fCurrencyPluralInfo->fPluralCountToCurrencyUnitPattern;
    const UHashElement* element = NULL;
    int32_t pos = UHASH_FIRST;
    Hashtable pluralPatternSet;
    while ((element = pluralPtn->nextElement(pos)) != NULL) {
        const UHashTok valueTok = element->value;
        const UnicodeString* value = (UnicodeString*)valueTok.pointer;
        const UHashTok keyTok = element->key;
        const UnicodeString* key = (UnicodeString*)keyTok.pointer;
        if (pluralPatternSet.geti(*value) != 1) {
            UnicodeString negPrefix;
            UnicodeString negSuffix;
            UnicodeString posPrefix;
            UnicodeString posSuffix;
            pluralPatternSet.puti(*value, 1, status);
            applyPatternWithNoSideEffects(
                    *value, parseErr,
                    negPrefix, negSuffix, posPrefix, posSuffix, status);
            AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    negPrefix,
                                                    negSuffix,
                                                    posPrefix,
                                                    posSuffix,
                                                    UCURR_LONG_NAME);
            fAffixPatternsForCurrency->put(*key, affixPtn, status);
        }
    }
}


void
DecimalFormat::setupCurrencyAffixes(const UnicodeString& pattern,
                                    UBool setupForCurrentPattern,
                                    UBool setupForPluralPattern,
                                    UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    if (setupForCurrentPattern) {
        if (fAffixesForCurrency) {
            deleteHashForAffix(fAffixesForCurrency);
        }
        fAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            applyPatternWithoutExpandAffix(pattern, false, parseErr, status);
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const UnicodeString* pluralCount;
                while ((pluralCount = keywords->snext(status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        expandAffixAdjustWidth(pluralCount);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fAffixesForCurrency->put(*pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }

    if (U_FAILURE(status)) {
        return;
    }

    if (setupForPluralPattern) {
        if (fPluralAffixesForCurrency) {
            deleteHashForAffix(fPluralAffixesForCurrency);
        }
        fPluralAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const UnicodeString* pluralCount;
                while ((pluralCount = keywords->snext(status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        UnicodeString ptn;
                        fCurrencyPluralInfo->getCurrencyPluralPattern(*pluralCount, ptn);
                        applyPatternInternally(*pluralCount, ptn, false, parseErr, status);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fPluralAffixesForCurrency->put(*pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }
}


//------------------------------------------------------------------------------

DecimalFormat::~DecimalFormat()
{
    delete fPosPrefixPattern;
    delete fPosSuffixPattern;
    delete fNegPrefixPattern;
    delete fNegSuffixPattern;
    delete fCurrencyChoice;
    delete fMultiplier;
    delete fSymbols;
    delete fRoundingIncrement;
    deleteHashForAffixPattern();
    deleteHashForAffix(fAffixesForCurrency);
    deleteHashForAffix(fPluralAffixesForCurrency);
    delete fCurrencyPluralInfo;
    delete fImpl;
}

//------------------------------------------------------------------------------
// copy constructor

DecimalFormat::DecimalFormat(const DecimalFormat &source) :
    NumberFormat(source) {
    init();
    *this = source;
}

//------------------------------------------------------------------------------
// assignment operator

template <class T>
static void _copy_ptr(T** pdest, const T* source) {
    if (source == NULL) {
        delete *pdest;
        *pdest = NULL;
    } else if (*pdest == NULL) {
        *pdest = new T(*source);
    } else {
        **pdest = *source;
    }
}

template <class T>
static void _clone_ptr(T** pdest, const T* source) {
    delete *pdest;
    if (source == NULL) {
        *pdest = NULL;
    } else {
        *pdest = static_cast<T*>(source->clone());
    }
}

DecimalFormat&
DecimalFormat::operator=(const DecimalFormat& rhs)
{
    if(this != &rhs) {
        UErrorCode status = U_ZERO_ERROR;
        NumberFormat::operator=(rhs);
        if (fImpl == NULL) {
            fImpl = new DecimalFormatImpl(*rhs.fImpl);
        } else {
            *fImpl = *rhs.fImpl;
        }
        fStaticSets     = DecimalFormatStaticSets::getStaticSets(status);
        fPositivePrefix = rhs.fPositivePrefix;
        fPositiveSuffix = rhs.fPositiveSuffix;
        fNegativePrefix = rhs.fNegativePrefix;
        fNegativeSuffix = rhs.fNegativeSuffix;
        _copy_ptr(&fPosPrefixPattern, rhs.fPosPrefixPattern);
        _copy_ptr(&fPosSuffixPattern, rhs.fPosSuffixPattern);
        _copy_ptr(&fNegPrefixPattern, rhs.fNegPrefixPattern);
        _copy_ptr(&fNegSuffixPattern, rhs.fNegSuffixPattern);
        _clone_ptr(&fCurrencyChoice, rhs.fCurrencyChoice);
        setRoundingIncrement(rhs.getRoundingIncrement());
        fRoundingMode = rhs.fRoundingMode;
        setMultiplier(rhs.getMultiplier());
        fGroupingSize = rhs.fGroupingSize;
        fGroupingSize2 = rhs.fGroupingSize2;
        fDecimalSeparatorAlwaysShown = rhs.fDecimalSeparatorAlwaysShown;
        _copy_ptr(&fSymbols, rhs.fSymbols);
        fUseExponentialNotation = rhs.fUseExponentialNotation;
        fExponentSignAlwaysShown = rhs.fExponentSignAlwaysShown;
        fBoolFlags = rhs.fBoolFlags;
        /*Bertrand A. D. Update 98.03.17*/
        fCurrencySignCount = rhs.fCurrencySignCount;
        /*end of Update*/
        fMinExponentDigits = rhs.fMinExponentDigits;

        /* sfb 990629 */
        fFormatWidth = rhs.fFormatWidth;
        fPad = rhs.fPad;
        fPadPosition = rhs.fPadPosition;
        /* end sfb */
        fMinSignificantDigits = rhs.fMinSignificantDigits;
        fMaxSignificantDigits = rhs.fMaxSignificantDigits;
        fUseSignificantDigits = rhs.fUseSignificantDigits;
        fFormatPattern = rhs.fFormatPattern;
        fCurrencyUsage = rhs.fCurrencyUsage;
        fStyle = rhs.fStyle;
        _clone_ptr(&fCurrencyPluralInfo, rhs.fCurrencyPluralInfo);
        deleteHashForAffixPattern();
        if (rhs.fAffixPatternsForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fAffixPatternsForCurrency = initHashForAffixPattern(status);
            copyHashForAffixPattern(rhs.fAffixPatternsForCurrency,
                                    fAffixPatternsForCurrency, status);
        }
        deleteHashForAffix(fAffixesForCurrency);
        if (rhs.fAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fAffixesForCurrency, fAffixesForCurrency, status);
        }
        deleteHashForAffix(fPluralAffixesForCurrency);
        if (rhs.fPluralAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fPluralAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fPluralAffixesForCurrency, fPluralAffixesForCurrency, status);
        }
    }

    return *this;
}

//------------------------------------------------------------------------------

UBool
DecimalFormat::operator==(const Format& that) const
{
    if (this == &that)
        return TRUE;

    // NumberFormat::operator== guarantees this cast is safe
    const DecimalFormat* other = (DecimalFormat*)&that;

#ifdef FMT_DEBUG
    // This code makes it easy to determine why two format objects that should
    // be equal aren't.
    UBool first = TRUE;
    if (!NumberFormat::operator==(that)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("NumberFormat::!=");
    } else {
    if (!((fPosPrefixPattern == other->fPosPrefixPattern && // both null
              fPositivePrefix == other->fPositivePrefix)
           || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
               *fPosPrefixPattern  == *other->fPosPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Prefix !=");
    }
    if (!((fPosSuffixPattern == other->fPosSuffixPattern && // both null
           fPositiveSuffix == other->fPositiveSuffix)
          || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
              *fPosSuffixPattern  == *other->fPosSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Suffix !=");
    }
    if (!((fNegPrefixPattern == other->fNegPrefixPattern && // both null
           fNegativePrefix == other->fNegativePrefix)
          || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
              *fNegPrefixPattern  == *other->fNegPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Prefix ");
        if (fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativePrefix);
            debug(")");
        } else {
            debugout(*fNegPrefixPattern);
        }
        debug(" != ");
        if (other->fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativePrefix);
            debug(")");
        } else {
            debugout(*other->fNegPrefixPattern);
        }
    }
    if (!((fNegSuffixPattern == other->fNegSuffixPattern && // both null
           fNegativeSuffix == other->fNegativeSuffix)
          || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
              *fNegSuffixPattern  == *other->fNegSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Suffix ");
        if (fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativeSuffix);
            debug(")");
        } else {
            debugout(*fNegSuffixPattern);
        }
        debug(" != ");
        if (other->fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativeSuffix);
            debug(")");
        } else {
            debugout(*other->fNegSuffixPattern);
        }
    }
    if (!((fRoundingIncrement == other->fRoundingIncrement) // both null
          || (fRoundingIncrement != NULL &&
              other->fRoundingIncrement != NULL &&
              *fRoundingIncrement == *other->fRoundingIncrement))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Rounding Increment !=");
              }
    if (fRoundingMode != other->fRoundingMode) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Rounding Mode %d != %d", (int)fRoundingMode, (int)other->fRoundingMode);
    }
    if (getMultiplier() != other->getMultiplier()) {
        if (first) { printf("[ "); first = FALSE; }
        printf("Multiplier %ld != %ld", getMultiplier(), other->getMultiplier());
    }
    if (fGroupingSize != other->fGroupingSize) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Grouping Size %ld != %ld", fGroupingSize, other->fGroupingSize);
    }
    if (fGroupingSize2 != other->fGroupingSize2) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Secondary Grouping Size %ld != %ld", fGroupingSize2, other->fGroupingSize2);
    }
    if (fDecimalSeparatorAlwaysShown != other->fDecimalSeparatorAlwaysShown) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("fDecimalSeparatorAlwaysShown %d != %d", fDecimalSeparatorAlwaysShown, other->fDecimalSeparatorAlwaysShown);
    }
    if (fUseExponentialNotation != other->fUseExponentialNotation) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fUseExponentialNotation !=");
    }
    if (fUseExponentialNotation &&
        fMinExponentDigits != other->fMinExponentDigits) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fMinExponentDigits !=");
    }
    if (fUseExponentialNotation &&
        fExponentSignAlwaysShown != other->fExponentSignAlwaysShown) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fExponentSignAlwaysShown !=");
    }
    if (fBoolFlags.getAll() != other->fBoolFlags.getAll()) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fBoolFlags !=");
    }
    if (*fSymbols != *(other->fSymbols)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Symbols !=");
    }
    // TODO Add debug stuff for significant digits here
    if (fUseSignificantDigits != other->fUseSignificantDigits) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fUseSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMinSignificantDigits != other->fMinSignificantDigits) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fMinSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMaxSignificantDigits != other->fMaxSignificantDigits) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fMaxSignificantDigits !=");
    }
    if (fFormatWidth != other->fFormatWidth) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fFormatWidth !=");
    }
    if (fPad != other->fPad) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fPad !=");
    }
    if (fPadPosition != other->fPadPosition) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fPadPosition !=");
    }
    if (fStyle == UNUM_CURRENCY_PLURAL &&
        fStyle != other->fStyle)
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fStyle !=");
    }
    if (fStyle == UNUM_CURRENCY_PLURAL &&
        fFormatPattern != other->fFormatPattern) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fFormatPattern !=");
    }

    if (!first) { printf(" ]"); }
    if (fCurrencySignCount != other->fCurrencySignCount) {
        debug("fCurrencySignCount !=");
    }
    if (fCurrencyPluralInfo == other->fCurrencyPluralInfo) {
        debug("fCurrencyPluralInfo == ");
        if (fCurrencyPluralInfo == NULL) {
            debug("fCurrencyPluralInfo == NULL");
        }
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo != *(other->fCurrencyPluralInfo)) {
        debug("fCurrencyPluralInfo !=");
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo == NULL ||
        fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo != NULL) {
        debug("fCurrencyPluralInfo one NULL, the other not");
    }
    if (fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo == NULL) {
        debug("fCurrencyPluralInfo == ");
    }
    }
#endif

    return (
        NumberFormat::operator==(that) &&

        ((fCurrencySignCount == fgCurrencySignCountInPluralFormat) ?
        (fAffixPatternsForCurrency->equals(*other->fAffixPatternsForCurrency)) :
        (((fPosPrefixPattern == other->fPosPrefixPattern && // both null
          fPositivePrefix == other->fPositivePrefix)
         || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
             *fPosPrefixPattern  == *other->fPosPrefixPattern)) &&
        ((fPosSuffixPattern == other->fPosSuffixPattern && // both null
          fPositiveSuffix == other->fPositiveSuffix)
         || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
             *fPosSuffixPattern  == *other->fPosSuffixPattern)) &&
        ((fNegPrefixPattern == other->fNegPrefixPattern && // both null
          fNegativePrefix == other->fNegativePrefix)
         || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
             *fNegPrefixPattern  == *other->fNegPrefixPattern)) &&
        ((fNegSuffixPattern == other->fNegSuffixPattern && // both null
          fNegativeSuffix == other->fNegativeSuffix)
         || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
             *fNegSuffixPattern  == *other->fNegSuffixPattern)))) &&

        ((fRoundingIncrement == other->fRoundingIncrement) // both null
         || (fRoundingIncrement != NULL &&
             other->fRoundingIncrement != NULL &&
             *fRoundingIncrement == *other->fRoundingIncrement)) &&

        fRoundingMode == other->fRoundingMode &&
        getMultiplier() == other->getMultiplier() &&
        fGroupingSize == other->fGroupingSize &&
        fGroupingSize2 == other->fGroupingSize2 &&
        fDecimalSeparatorAlwaysShown == other->fDecimalSeparatorAlwaysShown &&
        fUseExponentialNotation == other->fUseExponentialNotation &&

        (!fUseExponentialNotation ||
            (fMinExponentDigits == other->fMinExponentDigits && fExponentSignAlwaysShown == other->fExponentSignAlwaysShown)) &&

        fBoolFlags.getAll() == other->fBoolFlags.getAll() &&
        *fSymbols == *(other->fSymbols) &&
        fUseSignificantDigits == other->fUseSignificantDigits &&

        (!fUseSignificantDigits ||
            (fMinSignificantDigits == other->fMinSignificantDigits && fMaxSignificantDigits == other->fMaxSignificantDigits)) &&

        fFormatWidth == other->fFormatWidth &&
        fPad == other->fPad &&
        fPadPosition == other->fPadPosition &&

        (fStyle != UNUM_CURRENCY_PLURAL ||
            (fStyle == other->fStyle && fFormatPattern == other->fFormatPattern)) &&

        fCurrencySignCount == other->fCurrencySignCount &&

        ((fCurrencyPluralInfo == other->fCurrencyPluralInfo &&
          fCurrencyPluralInfo == NULL) ||
         (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo == *(other->fCurrencyPluralInfo))) &&

        fCurrencyUsage == other->fCurrencyUsage &&
        *fImpl == *other->fImpl

        // depending on other settings we may also need to compare
        // fCurrencyChoice (mostly deprecated?),
        // fAffixesForCurrency & fPluralAffixesForCurrency (only relevant in some cases)
        );
}

//------------------------------------------------------------------------------

Format*
DecimalFormat::clone() const
{
    return new DecimalFormat(*this);
}


FixedDecimal
DecimalFormat::getFixedDecimal(double number, UErrorCode &status) const {
    FixedDecimal result;
    return fImpl->getFixedDecimal(number, result);
}

UnicodeString
DecimalFormat::select(
        double number,
        const PluralRules &rules) const {
    return fImpl->select(number, rules);
}


UnicodeString &
DecimalFormat::select(
        const Formattable &number,
        const PluralRules &rules,
        UnicodeString &result,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return result;
    }
    result = rules.select(getFixedDecimal(number, status));
    return result;
}

FixedDecimal
DecimalFormat::getFixedDecimal(const Formattable &number, UErrorCode &status) const {
    FixedDecimal result;
    if (U_FAILURE(status)) {
        return result;
    }
    if (!number.isNumeric()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return result;
    }

    DigitList *dl = number.getDigitList();
    if (dl != NULL) {
        return fImpl->getFixedDecimal(*dl, result);
    }

    Formattable::Type type = number.getType();
    if (type == Formattable::kDouble || type == Formattable::kLong) { 
        return fImpl->getFixedDecimal(number.getDouble(status), result);
    }

    if (type == Formattable::kInt64 && number.getInt64() <= MAX_INT64_IN_DOUBLE &&
                                       number.getInt64() >= -MAX_INT64_IN_DOUBLE) {
        return fImpl->getFixedDecimal(number.getDouble(status), result);
    }

    // The only case left is type==int64_t, with a value with more digits than a double can represent.
    // Any formattable originating as a big decimal will have had a pre-existing digit list.
    // Any originating as a double or int32 will have been handled as a double.

    U_ASSERT(type == Formattable::kInt64);
    DigitList digits;
    digits.set(number.getInt64());
    return fImpl->getFixedDecimal(digits, result);
}


// Create a fixed decimal from a DigitList.
//    The digit list may be modified.
//    Internal function only.
FixedDecimal
DecimalFormat::getFixedDecimal(DigitList &number, UErrorCode &status) const {
    // Round the number according to the requirements of this Format.
    FixedDecimal result;
    _round(number, number, result.isNegative, status);

    // The int64_t fields in FixedDecimal can easily overflow.
    // In deciding what to discard in this event, consider that fixedDecimal
    //   is being used only with PluralRules, and those rules mostly look at least significant
    //   few digits of the integer part, and whether the fraction part is zero or not.
    // 
    // So, in case of overflow when filling in the fields of the FixedDecimal object,
    //    for the integer part, discard the most significant digits.
    //    for the fraction part, discard the least significant digits,
    //                           don't truncate the fraction value to zero.
    // For simplicity, the int64_t fields are limited to 18 decimal digits, even
    // though they could hold most (but not all) 19 digit values.

    // Integer Digits.
    int32_t di = number.getDecimalAt()-18;  // Take at most 18 digits.
    if (di < 0) {
        di = 0;
    }
    result.intValue = 0;
    for (; di<number.getDecimalAt(); di++) {
        result.intValue = result.intValue * 10 + (number.getDigit(di) & 0x0f);
    }
    if (result.intValue == 0 && number.getDecimalAt()-18 > 0) {
        // The number is something like 100000000000000000000000.
        // More than 18 digits integer digits, but the least significant 18 are all zero.
        // We don't want to return zero as the int part, but want to keep zeros
        //   for several of the least significant digits.
        result.intValue = 100000000000000000LL;
    }
    
    // Fraction digits.
    result.decimalDigits = result.decimalDigitsWithoutTrailingZeros = result.visibleDecimalDigitCount = 0;
    for (di = number.getDecimalAt(); di < number.getCount(); di++) {
        result.visibleDecimalDigitCount++;
        if (result.decimalDigits <  100000000000000000LL) {
                   //              9223372036854775807    Largest 64 bit signed integer
            int32_t digitVal = number.getDigit(di) & 0x0f;  // getDigit() returns a char, '0'-'9'.
            result.decimalDigits = result.decimalDigits * 10 + digitVal;
            if (digitVal > 0) {
                result.decimalDigitsWithoutTrailingZeros = result.decimalDigits;
            }
        }
    }

    result.hasIntegerValue = (result.decimalDigits == 0);

    // Trailing fraction zeros. The format specification may require more trailing
    //    zeros than the numeric value. Add any such on now.

    int32_t minFractionDigits;
    if (areSignificantDigitsUsed()) {
        minFractionDigits = getMinimumSignificantDigits() - number.getDecimalAt();
        if (minFractionDigits < 0) {
            minFractionDigits = 0;
        }
    } else {
        minFractionDigits = getMinimumFractionDigits();
    }
    result.adjustForMinFractionDigits(minFractionDigits);

    return result;
}


//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    UErrorCode status = U_ZERO_ERROR;
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition,
                      UErrorCode& status) const
{
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    return fImpl->format(number, appendTo, posIter, status);
}


//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    UErrorCode status = U_ZERO_ERROR; /* ignored */
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition,
                      UErrorCode& status) const
{
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    return fImpl->format(number, appendTo, posIter, status);
}

//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition) const
{
    UErrorCode status = U_ZERO_ERROR; /* ignored */
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition,
                        UErrorCode& status) const
{
    return fImpl->format(number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPositionIterator* posIter,
                        UErrorCode& status) const
{
    return fImpl->format(number, appendTo, posIter, status);
}

//------------------------------------------------------------------------------


UnicodeString&
DecimalFormat::format(const StringPiece &number,
                      UnicodeString &toAppendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const
{
  return fImpl->format(number, toAppendTo, posIter, status);
}


UnicodeString&
DecimalFormat::format(const DigitList &number,
                      UnicodeString &appendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const {
    return fImpl->format(number, appendTo, posIter, status);
}



UnicodeString&
DecimalFormat::format(const DigitList &number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    return fImpl->format(number, appendTo, pos, status);
}

DigitList&
DecimalFormat::_round(const DigitList &number, DigitList &adjustedNum, UBool& isNegative, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return adjustedNum;
    }

    // note: number and adjustedNum may refer to the same DigitList, in cases where a copy
    //       is not needed by the caller.

    adjustedNum = number;
    isNegative = false;
    if (number.isNaN()) {
        return adjustedNum;
    }

    // Do this BEFORE checking to see if value is infinite or negative! Sets the
    // begin and end index to be length of the string composed of
    // localized name of Infinite and the positive/negative localized
    // signs.

    adjustedNum.setRoundingMode(fRoundingMode);
    if (fMultiplier != NULL) {
        adjustedNum.mult(*fMultiplier, status);
        if (U_FAILURE(status)) {
            return adjustedNum;
        }
    }

    if (fScale != 0) {
        DigitList ten;
        ten.set((int32_t)10);
        if (fScale > 0) {
            for (int32_t i = fScale ; i > 0 ; i--) {
                adjustedNum.mult(ten, status);
                if (U_FAILURE(status)) {
                    return adjustedNum;
                }
            }
        } else {
            for (int32_t i = fScale ; i < 0 ; i++) {
                adjustedNum.div(ten, status);
                if (U_FAILURE(status)) {
                    return adjustedNum;
                }
            }
        }
    }

    /*
     * Note: sign is important for zero as well as non-zero numbers.
     * Proper detection of -0.0 is needed to deal with the
     * issues raised by bugs 4106658, 4106667, and 4147706.  Liu 7/6/98.
     */
    isNegative = !adjustedNum.isPositive();

    // Apply rounding after multiplier

    adjustedNum.fContext.status &= ~DEC_Inexact;
    if (fRoundingIncrement != NULL) {
        adjustedNum.div(*fRoundingIncrement, status);
        adjustedNum.toIntegralValue();
        adjustedNum.mult(*fRoundingIncrement, status);
        adjustedNum.trim();
        if (U_FAILURE(status)) {
            return adjustedNum;
        }
    }
    if (fRoundingMode == kRoundUnnecessary && (adjustedNum.fContext.status & DEC_Inexact)) {
        status = U_FORMAT_INEXACT_ERROR;
        return adjustedNum;
    }

    if (adjustedNum.isInfinite()) {
        return adjustedNum;
    }

    if (fUseExponentialNotation || areSignificantDigitsUsed()) {
        int32_t sigDigits = precision();
        if (sigDigits > 0) {
            adjustedNum.round(sigDigits);
            // Travis Keep (21/2/2014): Calling round on a digitList does not necessarily
            // preserve the sign of that digit list. Preserving the sign is especially
            // important when formatting -0.0 for instance. Not preserving the sign seems
            // like a bug because I cannot think of any case where the sign would actually
            // have to change when rounding. For now, we preserve the sign by setting the
            // positive attribute directly.
            adjustedNum.setPositive(!isNegative);
        }
    } else {
        // Fixed point format.  Round to a set number of fraction digits.
        int32_t numFractionDigits = precision();
        adjustedNum.roundFixedPoint(numFractionDigits);
    }
    if (fRoundingMode == kRoundUnnecessary && (adjustedNum.fContext.status & DEC_Inexact)) {
        status = U_FORMAT_INEXACT_ERROR;
        return adjustedNum;
    }
    return adjustedNum;
}

/**
 * Return true if a grouping separator belongs at the given
 * position, based on whether grouping is in use and the values of
 * the primary and secondary grouping interval.
 * @param pos the number of integer digits to the right of
 * the current position.  Zero indicates the position after the
 * rightmost integer digit.
 * @return true if a grouping character belongs at the current
 * position.
 */
UBool DecimalFormat::isGroupingPosition(int32_t pos) const {
    UBool result = FALSE;
    if (isGroupingUsed() && (pos > 0) && (fGroupingSize > 0)) {
        if ((fGroupingSize2 > 0) && (pos > fGroupingSize)) {
            result = ((pos - fGroupingSize) % fGroupingSize2) == 0;
        } else {
            result = pos % fGroupingSize == 0;
        }
    }
    return result;
}

void
DecimalFormat::parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const {
    parse(text, result, parsePosition, NULL);
}

CurrencyAmount* DecimalFormat::parseCurrency(const UnicodeString& text,
                                             ParsePosition& pos) const {
    Formattable parseResult;
    int32_t start = pos.getIndex();
    UChar curbuf[4] = {};
    parse(text, parseResult, pos, curbuf);
    if (pos.getIndex() != start) {
        UErrorCode ec = U_ZERO_ERROR;
        LocalPointer<CurrencyAmount> currAmt(new CurrencyAmount(parseResult, curbuf, ec), ec);
        if (U_FAILURE(ec)) {
            pos.setIndex(start); // indicate failure
        } else {
            return currAmt.orphan();
        }
    }
    return NULL;
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
void DecimalFormat::parse(const UnicodeString& text,
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
    int32_t formatWidth = fImpl->getOldFormatWidth();

    // Skip padding characters, if around prefix
    if (formatWidth > 0 && (
            fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix ||
            fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix)) {
        i = skipPadding(text, i);
    }

    if (isLenient()) {
        // skip any leading whitespace
        i = backup = skipUWhiteSpace(text, i);
    }

    // If the text is composed of the representation of NaN, returns NaN.length
    const UnicodeString *nan = &fImpl->getConstSymbol(DecimalFormatSymbols::kNaNSymbol);
    int32_t nanLen = (text.compare(i, nan->length(), *nan)
                      ? 0 : nan->length());
    if (nanLen) {
        i += nanLen;
        if (formatWidth > 0 && (fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix || fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix)) {
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

    if (fImpl->fMonetary) {
        if (!parseForCurrency(text, parsePosition, *digits,
                              status, currency)) {
          return;
        }
    } else {
        if (!subparse(text,
                      &fImpl->fAap.fNegativePrefix.getOtherVariant().toString(),
                      &fImpl->fAap.fNegativeSuffix.getOtherVariant().toString(),
                      &fImpl->fAap.fPositivePrefix.getOtherVariant().toString(),
                      &fImpl->fAap.fPositiveSuffix.getOtherVariant().toString(),
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

        if (!fImpl->fMultiplier.isZero()) {
            UErrorCode ec = U_ZERO_ERROR;
            digits->div(fImpl->fMultiplier, ec);
        }

        if (fImpl->fScale != 0) {
            DigitList ten;
            ten.set((int32_t)10);
            if (fImpl->fScale > 0) {
                for (int32_t i = fImpl->fScale; i > 0; i--) {
                    UErrorCode ec = U_ZERO_ERROR;
                    digits->div(ten,ec);
                }
            } else {
                for (int32_t i = fImpl->fScale; i < 0; i++) {
                    UErrorCode ec = U_ZERO_ERROR;
                    digits->mult(ten,ec);
                }
            }
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
DecimalFormat::parseForCurrency(const UnicodeString& text,
                                ParsePosition& parsePosition,
                                DigitList& digits,
                                UBool* status,
                                UChar* currency) const {
    UnicodeString positivePrefix;
    UnicodeString positiveSuffix;
    UnicodeString negativePrefix;
    UnicodeString negativeSuffix;
    fImpl->fPositivePrefixPattern.toString(positivePrefix);
    fImpl->fPositiveSuffixPattern.toString(positiveSuffix);
    fImpl->fNegativePrefixPattern.toString(negativePrefix);
    fImpl->fNegativeSuffixPattern.toString(negativeSuffix);

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
    if (fStyle == UNUM_CURRENCY_PLURAL) {
        found = subparse(text,
                         &negativePrefix, &negativeSuffix,
                         &positivePrefix, &positiveSuffix,
                         TRUE, UCURR_LONG_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    } else {
        found = subparse(text,
                         &negativePrefix, &negativeSuffix,
                         &positivePrefix, &positiveSuffix,
                         TRUE, UCURR_SYMBOL_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    }
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
    // Then, parse against affix patterns.
    // Those are currency patterns and currency plural patterns.
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* affixPtn = (AffixPatternsForCurrency*)valueTok.pointer;
        UBool tmpStatus[fgStatusLength];
        ParsePosition tmpPos(origPos);
        DigitList tmpDigitList;

#ifdef FMT_DEBUG
        debug("trying affix for currency..");
        affixPtn->dump();
#endif

        UBool result = subparse(text,
                                &affixPtn->negPrefixPatternForCurrency,
                                &affixPtn->negSuffixPatternForCurrency,
                                &affixPtn->posPrefixPatternForCurrency,
                                &affixPtn->posSuffixPatternForCurrency,
                                TRUE, affixPtn->patternType,
                                tmpPos, tmpDigitList, tmpStatus, currency);
        if (result) {
            found = true;
            if (tmpPos.getIndex() > maxPosIndex) {
                maxPosIndex = tmpPos.getIndex();
                for (int32_t i = 0; i < fgStatusLength; ++i) {
                    status[i] = tmpStatus[i];
                }
                digits = tmpDigitList;
            }
        } else {
            maxErrorPos = (tmpPos.getErrorIndex() > maxErrorPos) ?
                          tmpPos.getErrorIndex() : maxErrorPos;
        }
    }
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
                            &fImpl->fAap.fNegativePrefix.getOtherVariant().toString(),
                            &fImpl->fAap.fNegativeSuffix.getOtherVariant().toString(),
                            &fImpl->fAap.fPositivePrefix.getOtherVariant().toString(),
                            &fImpl->fAap.fPositiveSuffix.getOtherVariant().toString(),
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
UBool DecimalFormat::subparse(const UnicodeString& text,
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
    UChar32 zero = fImpl->getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    const UnicodeString *groupingString = &fImpl->getConstSymbol(
            !fImpl->fMonetary ?
            DecimalFormatSymbols::kGroupingSeparatorSymbol : DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
    UChar32 groupingChar = groupingString->char32At(0);
    int32_t groupingStringLength = groupingString->length();
    int32_t groupingCharLength   = U16_LENGTH(groupingChar);
    UBool   groupingUsed = fImpl->isGroupingUsed();
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
    printf("currencyParsing=%d, fFormatWidth=%d, isParseIntegerOnly=%c text.length=%d negPrefLen=%d\n", currencyParsing, fFormatWidth, (isParseIntegerOnly())?'Y':'N', text.length(),  negPrefix!=NULL?negPrefix->length():-1);
#endif

    UBool fastParseOk = false; /* TRUE iff fast parse is OK */
    // UBool fastParseHadDecimal = FALSE; /* true if fast parse saw a decimal point. */
    if((fImpl->isParseFastpath()) && !fImpl->fMonetary &&
       text.length()>0 &&
       text.length()<32 &&
       (posPrefix==NULL||posPrefix->isEmpty()) &&
       (posSuffix==NULL||posSuffix->isEmpty()) &&
       //            (negPrefix==NULL||negPrefix->isEmpty()) &&
       //            (negSuffix==NULL||(negSuffix->isEmpty()) ) &&
       TRUE) {  // optimized path
      int j=position;
      int l=text.length();
      int digitCount=0;
      UChar32 ch = text.char32At(j);
      const UnicodeString *decimalString = &fImpl->getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
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
        
        /*
          parsedNum.append('-',err); 
          j+=U16_LENGTH(ch);
          if(j<l) ch = text.char32At(j);
        */
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
      printf("fFormatWidth=%d ", fFormatWidth);
      printf("text.length()=%d ", text.length());
      printf("posPrefix=%p posSuffix=%p ", posPrefix, posSuffix);

      printf("\n");
#endif
    }

  UnicodeString formatPattern;
  toPattern(formatPattern);

  if(!fastParseOk 
#if UCONFIG_HAVE_PARSEALLINPUT
     && fParseAllInput!=UNUM_YES
#endif
     ) 
  {
    int32_t formatWidth = fImpl->getOldFormatWidth();
    // Match padding before prefix
    if (formatWidth > 0 && fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforePrefix) {
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
    if (formatWidth > 0 && fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterPrefix) {
        position = skipPadding(text, position);
    }

    if (! strictParse) {
        position = skipUWhiteSpace(text, position);
    }

    // process digits or Inf, find decimal position
    const UnicodeString *inf = &fImpl->getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);
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
        int32_t gs2 = fImpl->fEffGrouping.fGrouping2 == 0 ? fImpl->fEffGrouping.fGrouping : fImpl->fEffGrouping.fGrouping2;

        const UnicodeString *decimalString;
        if (fImpl->fMonetary) {
            decimalString = &fImpl->getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
        } else {
            decimalString = &fImpl->getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
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
                if ( fImpl->getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kZeroDigitSymbol)).char32At(0) == ch ) {
                    break;
                }
                for (digit = 1 ; digit < 10 ; digit++ ) {
                    if ( fImpl->getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kOneDigitSymbol+digit-1)).char32At(0) == ch ) {
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
                        (lastGroup != -1 && position - lastGroup != fImpl->fEffGrouping.fGrouping + 1)) {
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

                if(!fBoolFlags.contains(UNUM_PARSE_NO_EXPONENT) || // don't parse if this is set unless..
                   isScientificNotation()) { // .. it's an exponent format - ignore setting and parse anyways
                    const UnicodeString *tmp;
                    tmp = &fImpl->getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
                    // TODO: CASE
                    if (!text.caseCompare(position, tmp->length(), *tmp, U_FOLD_CASE_DEFAULT))    // error code is set below if !sawDigit 
                    {
                        // Parse sign, if present
                        int32_t pos = position + tmp->length();
                        char exponentSign = '+';

                        if (pos < textLength)
                        {
                            tmp = &fImpl->getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                            if (!text.compare(pos, tmp->length(), *tmp))
                            {
                                pos += tmp->length();
                            }
                            else {
                                tmp = &fImpl->getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
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
        if(!sawDecimal && isDecimalPatternMatchRequired()) 
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
            if (lastGroup != -1 && position - lastGroup != fImpl->fEffGrouping.fGrouping + 1) {
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
    if (formatWidth > 0 && fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadBeforeSuffix) {
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
    if (formatWidth > 0 && fImpl->fAap.fPadPosition == DigitAffixesAndPadding::kPadAfterSuffix) {
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
#if UCONFIG_HAVE_PARSEALLINPUT
  else if (fParseAllInput==UNUM_YES&&parsePosition.getIndex()!=textLength)
    {
#ifdef FMT_DEBUG
      printf(" PP didnt consume all (UNUM_YES), err\n");
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
#endif
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
    if(fastParseOk && isDecimalPatternMatchRequired()) 
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
int32_t DecimalFormat::skipPadding(const UnicodeString& text, int32_t position) const {
    int32_t padLen = U16_LENGTH(fPad);
    while (position < text.length() &&
           text.char32At(position) == fPad) {
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
int32_t DecimalFormat::compareAffix(const UnicodeString& text,
                                    int32_t pos,
                                    UBool isNegative,
                                    UBool isPrefix,
                                    const UnicodeString* affixPat,
                                    UBool complexCurrencyParsing,
                                    int8_t type,
                                    UChar* currency) const
{
    const UnicodeString *patternToCompare;
    if (fCurrencyChoice != NULL || currency != NULL ||
        (fImpl->fMonetary && complexCurrencyParsing)) {

        if (affixPat != NULL) {
            return compareComplexAffix(*affixPat, text, pos, type, currency);
        }
    }

    if (isNegative) {
        if (isPrefix) {
            patternToCompare = &fImpl->fAap.fNegativePrefix.getOtherVariant().toString();
        }
        else {
            patternToCompare = &fImpl->fAap.fNegativeSuffix.getOtherVariant().toString();
        }
    }
    else {
        if (isPrefix) {
            patternToCompare = &fImpl->fAap.fPositivePrefix.getOtherVariant().toString();
        }
        else {
            patternToCompare = &fImpl->fAap.fPositiveSuffix.getOtherVariant().toString();
        }
    }
    return compareSimpleAffix(*patternToCompare, text, pos, isLenient());
}

UBool DecimalFormat::equalWithSignCompatibility(UChar32 lhs, UChar32 rhs) const {
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
UnicodeString& DecimalFormat::trimMarksFromAffix(const UnicodeString& affix, UnicodeString& trimmedAffix) {
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
int32_t DecimalFormat::compareSimpleAffix(const UnicodeString& affix,
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
int32_t DecimalFormat::skipPatternWhiteSpace(const UnicodeString& text, int32_t pos) {
    const UChar* s = text.getBuffer();
    return (int32_t)(PatternProps::skipWhiteSpace(s + pos, text.length() - pos) - s);
}

/**
 * Skip over a run of zero or more isUWhiteSpace() characters at pos
 * in text.
 */
int32_t DecimalFormat::skipUWhiteSpace(const UnicodeString& text, int32_t pos) {
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
int32_t DecimalFormat::skipUWhiteSpaceAndMarks(const UnicodeString& text, int32_t pos) {
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
int32_t DecimalFormat::skipBidiMarks(const UnicodeString& text, int32_t pos) {
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
int32_t DecimalFormat::compareComplexAffix(const UnicodeString& affixPat,
                                           const UnicodeString& text,
                                           int32_t pos,
                                           int8_t type,
                                           UChar* currency) const
{
    int32_t start = pos;
    U_ASSERT(currency != NULL ||
             (fCurrencyChoice != NULL && *getCurrency() != 0) ||
             fImpl->fMonetary);

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
                const char* loc = fCurrencyPluralInfo->getLocale().getName();
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
                affix = &fImpl->getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                break;
            case kPatternPerMill:
                affix = &fImpl->getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                break;
            case kPatternPlus:
                affix = &fImpl->getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                break;
            case kPatternMinus:
                affix = &fImpl->getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
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
int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, UChar32 ch) {
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
int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, const UnicodeString& str) {
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

UBool DecimalFormat::matchSymbol(const UnicodeString &text, int32_t position, int32_t length, const UnicodeString &symbol,
                         UnicodeSet *sset, UChar32 schar)
{
    if (sset != NULL) {
        return sset->contains(schar);
    }

    return text.compare(position, length, symbol) == 0;
}

UBool DecimalFormat::matchDecimal(UChar32 symbolChar,
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

UBool DecimalFormat::matchGrouping(UChar32 groupingChar,
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



//------------------------------------------------------------------------------
// Gets the pointer to the localized decimal format symbols

const DecimalFormatSymbols*
DecimalFormat::getDecimalFormatSymbols() const
{
    return &fImpl->getDecimalFormatSymbols();
}

//------------------------------------------------------------------------------
// De-owning the current localized symbols and adopt the new symbols.

void
DecimalFormat::adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt)
{
    if (symbolsToAdopt == NULL) {
        return; // do not allow caller to set fSymbols to NULL
    }
    //TODO(refactor): Just adopt, don't copy
    fImpl->adoptDecimalFormatSymbols(new DecimalFormatSymbols(*symbolsToAdopt));

    UBool sameSymbols = FALSE;
    if (fSymbols != NULL) {
        sameSymbols = (UBool)(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) &&
            getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
        delete fSymbols;
    }

    fSymbols = symbolsToAdopt;
    if (!sameSymbols) {
        // If the currency symbols are the same, there is no need to recalculate.
        setCurrencyForSymbols();
    }
    expandAffixes(NULL);
}
//------------------------------------------------------------------------------
// Setting the symbols is equlivalent to adopting a newly created localized
// symbols.

void
DecimalFormat::setDecimalFormatSymbols(const DecimalFormatSymbols& symbols)
{
    adoptDecimalFormatSymbols(new DecimalFormatSymbols(symbols));
}


const CurrencyPluralInfo*
DecimalFormat::getCurrencyPluralInfo(void) const
{
    return fCurrencyPluralInfo;
}


void
DecimalFormat::adoptCurrencyPluralInfo(CurrencyPluralInfo* toAdopt)
{
    if (toAdopt != NULL) {
        delete fCurrencyPluralInfo;
        fCurrencyPluralInfo = toAdopt;
        // re-set currency affix patterns and currency affixes.
        if (fCurrencySignCount != fgCurrencySignCountZero) {
            UErrorCode status = U_ZERO_ERROR;
            if (fAffixPatternsForCurrency) {
                deleteHashForAffixPattern();
            }
            setupCurrencyAffixPatterns(status);
            if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
                // only setup the affixes of the plural pattern.
                setupCurrencyAffixes(fFormatPattern, FALSE, TRUE, status);
            }
        }
    }
}

void
DecimalFormat::setCurrencyPluralInfo(const CurrencyPluralInfo& info)
{
    adoptCurrencyPluralInfo(info.clone());
}


/**
 * Update the currency object to match the symbols.  This method
 * is used only when the caller has passed in a symbols object
 * that may not be the default object for its locale.
 */
void
DecimalFormat::setCurrencyForSymbols() {
    /*Bug 4212072
      Update the affix strings accroding to symbols in order to keep
      the affix strings up to date.
      [Richard/GCL]
    */

    // With the introduction of the Currency object, the currency
    // symbols in the DFS object are ignored.  For backward
    // compatibility, we check any explicitly set DFS object.  If it
    // is a default symbols object for its locale, we change the
    // currency object to one for that locale.  If it is custom,
    // we set the currency to null.
    UErrorCode ec = U_ZERO_ERROR;
    const UChar* c = NULL;
    const char* loc = fSymbols->getLocale().getName();
    UChar intlCurrencySymbol[4];
    ucurr_forLocale(loc, intlCurrencySymbol, 4, &ec);
    UnicodeString currencySymbol;

    uprv_getStaticCurrencyName(intlCurrencySymbol, loc, currencySymbol, ec);
    if (U_SUCCESS(ec)
        && getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) == currencySymbol
        && getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) == UnicodeString(intlCurrencySymbol))
    {
        // Trap an error in mapping locale to currency.  If we can't
        // map, then don't fail and set the currency to "".
        c = intlCurrencySymbol;
    }
    ec = U_ZERO_ERROR; // reset local error code!
    setCurrencyInternally(c, ec);
}


//------------------------------------------------------------------------------
// Gets the positive prefix of the number pattern.

UnicodeString&
DecimalFormat::getPositivePrefix(UnicodeString& result) const
{
    return fImpl->getPositivePrefix(result);
}

//------------------------------------------------------------------------------
// Sets the positive prefix of the number pattern.

void
DecimalFormat::setPositivePrefix(const UnicodeString& newValue)
{
    fImpl->setPositivePrefix(newValue);
    fPositivePrefix = newValue;
    delete fPosPrefixPattern;
    fPosPrefixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the negative prefix  of the number pattern.

UnicodeString&
DecimalFormat::getNegativePrefix(UnicodeString& result) const
{
    return fImpl->getNegativePrefix(result);
}

//------------------------------------------------------------------------------
// Gets the negative prefix  of the number pattern.

void
DecimalFormat::setNegativePrefix(const UnicodeString& newValue)
{
    fImpl->setNegativePrefix(newValue);
    fNegativePrefix = newValue;
    delete fNegPrefixPattern;
    fNegPrefixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the positive suffix of the number pattern.

UnicodeString&
DecimalFormat::getPositiveSuffix(UnicodeString& result) const
{
    return fImpl->getPositiveSuffix(result);
}

//------------------------------------------------------------------------------
// Sets the positive suffix of the number pattern.

void
DecimalFormat::setPositiveSuffix(const UnicodeString& newValue)
{
    fImpl->setPositiveSuffix(newValue);
    fPositiveSuffix = newValue;
    delete fPosSuffixPattern;
    fPosSuffixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the negative suffix of the number pattern.

UnicodeString&
DecimalFormat::getNegativeSuffix(UnicodeString& result) const
{
    return fImpl->getNegativeSuffix(result);
}

//------------------------------------------------------------------------------
// Sets the negative suffix of the number pattern.

void
DecimalFormat::setNegativeSuffix(const UnicodeString& newValue)
{
    fImpl->setNegativeSuffix(newValue);
    fNegativeSuffix = newValue;
    delete fNegSuffixPattern;
    fNegSuffixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the multiplier of the number pattern.
//   Multipliers are stored as decimal numbers (DigitLists) because that
//      is the most convenient for muliplying or dividing the numbers to be formatted.
//   A NULL multiplier implies one, and the scaling operations are skipped.

int32_t 
DecimalFormat::getMultiplier() const
{
    return fImpl->getMultiplier();
}

//------------------------------------------------------------------------------
// Sets the multiplier of the number pattern.
void
DecimalFormat::setMultiplier(int32_t newValue)
{
    fImpl->setMultiplier(newValue);
//  if (newValue == 0) {
//      throw new IllegalArgumentException("Bad multiplier: " + newValue);
//  }
    if (newValue == 0) {
        newValue = 1;     // one being the benign default value for a multiplier.
    }
    if (newValue == 1) {
        delete fMultiplier;
        fMultiplier = NULL;
    } else {
        if (fMultiplier == NULL) {
            fMultiplier = new DigitList;
        }
        if (fMultiplier != NULL) {
            fMultiplier->set(newValue);
        }
    }
}

/**
 * Get the rounding increment.
 * @return A positive rounding increment, or 0.0 if rounding
 * is not in effect.
 * @see #setRoundingIncrement
 * @see #getRoundingMode
 * @see #setRoundingMode
 */
double DecimalFormat::getRoundingIncrement() const {
    return fImpl->getRoundingIncrement();
}

/**
 * Set the rounding increment.  This method also controls whether
 * rounding is enabled.
 * @param newValue A positive rounding increment, or 0.0 to disable rounding.
 * Negative increments are equivalent to 0.0.
 * @see #getRoundingIncrement
 * @see #getRoundingMode
 * @see #setRoundingMode
 */
void DecimalFormat::setRoundingIncrement(double newValue) {
    fImpl->setRoundingIncrement(newValue);
    if (newValue > 0.0) {
        if (fRoundingIncrement == NULL) {
            fRoundingIncrement = new DigitList();
        }
        if (fRoundingIncrement != NULL) {
            fRoundingIncrement->set(newValue);
            return;
        }
    }
    // These statements are executed if newValue is less than 0.0
    // or fRoundingIncrement could not be created.
    delete fRoundingIncrement;
    fRoundingIncrement = NULL;
}

/**
 * Get the rounding mode.
 * @return A rounding mode
 * @see #setRoundingIncrement
 * @see #getRoundingIncrement
 * @see #setRoundingMode
 */
DecimalFormat::ERoundingMode DecimalFormat::getRoundingMode() const {
    return fImpl->getRoundingMode();
}

/**
 * Set the rounding mode.  This has no effect unless the rounding
 * increment is greater than zero.
 * @param roundingMode A rounding mode
 * @see #setRoundingIncrement
 * @see #getRoundingIncrement
 * @see #getRoundingMode
 */
void DecimalFormat::setRoundingMode(ERoundingMode roundingMode) {
    fImpl->setRoundingMode(roundingMode);
    fRoundingMode = roundingMode;
}

/**
 * Get the width to which the output of <code>format()</code> is padded.
 * @return the format width, or zero if no padding is in effect
 * @see #setFormatWidth
 * @see #getPadCharacter
 * @see #setPadCharacter
 * @see #getPadPosition
 * @see #setPadPosition
 */
int32_t DecimalFormat::getFormatWidth() const {
    return fImpl->getFormatWidth();
}

/**
 * Set the width to which the output of <code>format()</code> is padded.
 * This method also controls whether padding is enabled.
 * @param width the width to which to pad the result of
 * <code>format()</code>, or zero to disable padding.  A negative
 * width is equivalent to 0.
 * @see #getFormatWidth
 * @see #getPadCharacter
 * @see #setPadCharacter
 * @see #getPadPosition
 * @see #setPadPosition
 */
void DecimalFormat::setFormatWidth(int32_t width) {
    fFormatWidth = (width > 0) ? width : 0;
    fImpl->setFormatWidth(fFormatWidth);
}

UnicodeString DecimalFormat::getPadCharacterString() const {
    return UnicodeString(fImpl->getPadCharacter());
}

void DecimalFormat::setPadCharacter(const UnicodeString &padChar) {
    if (padChar.length() > 0) {
        fPad = padChar.char32At(0);
    }
    else {
        fPad = kDefaultPad;
    }
    fImpl->setPadCharacter(fPad);
}

static DecimalFormat::EPadPosition fromPadPosition(DigitAffixesAndPadding::EPadPosition padPos) {
    switch (padPos) {
    case DigitAffixesAndPadding::kPadBeforePrefix:
        return DecimalFormat::kPadBeforePrefix;
    case DigitAffixesAndPadding::kPadAfterPrefix:
        return DecimalFormat::kPadAfterPrefix;
    case DigitAffixesAndPadding::kPadBeforeSuffix:
        return DecimalFormat::kPadBeforeSuffix;
    case DigitAffixesAndPadding::kPadAfterSuffix:
        return DecimalFormat::kPadAfterSuffix;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return DecimalFormat::kPadBeforePrefix;
}

/**
 * Get the position at which padding will take place.  This is the location
 * at which padding will be inserted if the result of <code>format()</code>
 * is shorter than the format width.
 * @return the pad position, one of <code>kPadBeforePrefix</code>,
 * <code>kPadAfterPrefix</code>, <code>kPadBeforeSuffix</code>, or
 * <code>kPadAfterSuffix</code>.
 * @see #setFormatWidth
 * @see #getFormatWidth
 * @see #setPadCharacter
 * @see #getPadCharacter
 * @see #setPadPosition
 * @see #kPadBeforePrefix
 * @see #kPadAfterPrefix
 * @see #kPadBeforeSuffix
 * @see #kPadAfterSuffix
 */
DecimalFormat::EPadPosition DecimalFormat::getPadPosition() const {
    return fromPadPosition(fImpl->getPadPosition());
}

static DigitAffixesAndPadding::EPadPosition toPadPosition(DecimalFormat::EPadPosition padPos) {
    switch (padPos) {
    case DecimalFormat::kPadBeforePrefix:
        return DigitAffixesAndPadding::kPadBeforePrefix;
    case DecimalFormat::kPadAfterPrefix:
        return DigitAffixesAndPadding::kPadAfterPrefix;
    case DecimalFormat::kPadBeforeSuffix:
        return DigitAffixesAndPadding::kPadBeforeSuffix;
    case DecimalFormat::kPadAfterSuffix:
        return DigitAffixesAndPadding::kPadAfterSuffix;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return DigitAffixesAndPadding::kPadBeforePrefix;
}

/**
 * <strong><font face=helvetica color=red>NEW</font></strong>
 * Set the position at which padding will take place.  This is the location
 * at which padding will be inserted if the result of <code>format()</code>
 * is shorter than the format width.  This has no effect unless padding is
 * enabled.
 * @param padPos the pad position, one of <code>kPadBeforePrefix</code>,
 * <code>kPadAfterPrefix</code>, <code>kPadBeforeSuffix</code>, or
 * <code>kPadAfterSuffix</code>.
 * @see #setFormatWidth
 * @see #getFormatWidth
 * @see #setPadCharacter
 * @see #getPadCharacter
 * @see #getPadPosition
 * @see #kPadBeforePrefix
 * @see #kPadAfterPrefix
 * @see #kPadBeforeSuffix
 * @see #kPadAfterSuffix
 */
void DecimalFormat::setPadPosition(EPadPosition padPos) {
    fImpl->setPadPosition(toPadPosition(padPos));
    fPadPosition = padPos;
}

/**
 * Return whether or not scientific notation is used.
 * @return TRUE if this object formats and parses scientific notation
 * @see #setScientificNotation
 * @see #getMinimumExponentDigits
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
UBool DecimalFormat::isScientificNotation() const {
    return fImpl->isScientificNotation();
}

/**
 * Set whether or not scientific notation is used.
 * @param useScientific TRUE if this object formats and parses scientific
 * notation
 * @see #isScientificNotation
 * @see #getMinimumExponentDigits
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
void DecimalFormat::setScientificNotation(UBool useScientific) {
    fImpl->setScientificNotation(useScientific);
    fUseExponentialNotation = useScientific;
}

/**
 * Return the minimum exponent digits that will be shown.
 * @return the minimum exponent digits that will be shown
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
int8_t DecimalFormat::getMinimumExponentDigits() const {
    return fImpl->getMinimumExponentDigits();
}

/**
 * Set the minimum exponent digits that will be shown.  This has no
 * effect unless scientific notation is in use.
 * @param minExpDig a value >= 1 indicating the fewest exponent digits
 * that will be shown.  Values less than 1 will be treated as 1.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #getMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
void DecimalFormat::setMinimumExponentDigits(int8_t minExpDig) {
    fMinExponentDigits = (int8_t)((minExpDig > 0) ? minExpDig : 1);
    fImpl->setMinimumExponentDigits(fMinExponentDigits);
}

/**
 * Return whether the exponent sign is always shown.
 * @return TRUE if the exponent is always prefixed with either the
 * localized minus sign or the localized plus sign, false if only negative
 * exponents are prefixed with the localized minus sign.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #getMinimumExponentDigits
 * @see #setExponentSignAlwaysShown
 */
UBool DecimalFormat::isExponentSignAlwaysShown() const {
    return fImpl->isExponentSignAlwaysShown();
}

/**
 * Set whether the exponent sign is always shown.  This has no effect
 * unless scientific notation is in use.
 * @param expSignAlways TRUE if the exponent is always prefixed with either
 * the localized minus sign or the localized plus sign, false if only
 * negative exponents are prefixed with the localized minus sign.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #getMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 */
void DecimalFormat::setExponentSignAlwaysShown(UBool expSignAlways) {
    fImpl->setExponentSignAlwaysShown(expSignAlways);
    fExponentSignAlwaysShown = expSignAlways;
}

//------------------------------------------------------------------------------
// Gets the grouping size of the number pattern.  For example, thousand or 10
// thousand groupings.

int32_t
DecimalFormat::getGroupingSize() const
{
    return fImpl->getGroupingSize();
}

//------------------------------------------------------------------------------
// Gets the grouping size of the number pattern.

void
DecimalFormat::setGroupingSize(int32_t newValue)
{
    fImpl->setGroupingSize(newValue);
    fGroupingSize = newValue;
}

//------------------------------------------------------------------------------

int32_t
DecimalFormat::getSecondaryGroupingSize() const
{
    return fImpl->getSecondaryGroupingSize();
}

//------------------------------------------------------------------------------

void
DecimalFormat::setSecondaryGroupingSize(int32_t newValue)
{
    fImpl->setSecondaryGroupingSize(newValue);
    fGroupingSize2 = newValue;
}

//------------------------------------------------------------------------------
// Checks if to show the decimal separator.

UBool
DecimalFormat::isDecimalSeparatorAlwaysShown() const
{
    return fImpl->isDecimalSeparatorAlwaysShown();
}

//------------------------------------------------------------------------------
// Sets to always show the decimal separator.

void
DecimalFormat::setDecimalSeparatorAlwaysShown(UBool newValue)
{
    fImpl->setDecimalSeparatorAlwaysShown(newValue);
    fDecimalSeparatorAlwaysShown = newValue;
}

//------------------------------------------------------------------------------
// Checks if decimal point pattern match is required
UBool 
DecimalFormat::isDecimalPatternMatchRequired(void) const
{
    return fBoolFlags.contains(UNUM_PARSE_DECIMAL_MARK_REQUIRED);
}

//------------------------------------------------------------------------------
// Checks if decimal point pattern match is required
         
void 
DecimalFormat::setDecimalPatternMatchRequired(UBool newValue)
{
    fBoolFlags.set(UNUM_PARSE_DECIMAL_MARK_REQUIRED, newValue);
}


//------------------------------------------------------------------------------
// Emits the pattern of this DecimalFormat instance.

UnicodeString&
DecimalFormat::toPattern(UnicodeString& result) const
{
    return fImpl->toPattern(result);
}

//------------------------------------------------------------------------------
// Emits the localized pattern this DecimalFormat instance.

UnicodeString&
DecimalFormat::toLocalizedPattern(UnicodeString& result) const
{
    // toLocalizedPattern is deprecated, so we just make it the same as
    // toPattern.
    return fImpl->toPattern(result);
}

//------------------------------------------------------------------------------
/**
 * Expand the affix pattern strings into the expanded affix strings.  If any
 * affix pattern string is null, do not expand it.  This method should be
 * called any time the symbols or the affix patterns change in order to keep
 * the expanded affix strings up to date.
 * This method also will be called before formatting if format currency
 * plural names, since the plural name is not a static one, it is
 * based on the currency plural count, the affix will be known only
 * after the currency plural count is know.
 * In which case, the parameter
 * 'pluralCount' will be a non-null currency plural count.
 * In all other cases, the 'pluralCount' is null, which means it is not needed.
 */
void DecimalFormat::expandAffixes(const UnicodeString* pluralCount) {
    FieldPositionHandler none;
    if (fPosPrefixPattern != 0) {
      expandAffix(*fPosPrefixPattern, fPositivePrefix, 0, none, FALSE, pluralCount);
    }
    if (fPosSuffixPattern != 0) {
      expandAffix(*fPosSuffixPattern, fPositiveSuffix, 0, none, FALSE, pluralCount);
    }
    if (fNegPrefixPattern != 0) {
      expandAffix(*fNegPrefixPattern, fNegativePrefix, 0, none, FALSE, pluralCount);
    }
    if (fNegSuffixPattern != 0) {
      expandAffix(*fNegSuffixPattern, fNegativeSuffix, 0, none, FALSE, pluralCount);
    }
#ifdef FMT_DEBUG
    UnicodeString s;
    s.append(UnicodeString("["))
      .append(DEREFSTR(fPosPrefixPattern)).append((UnicodeString)"|").append(DEREFSTR(fPosSuffixPattern))
      .append((UnicodeString)";") .append(DEREFSTR(fNegPrefixPattern)).append((UnicodeString)"|").append(DEREFSTR(fNegSuffixPattern))
        .append((UnicodeString)"]->[")
        .append(fPositivePrefix).append((UnicodeString)"|").append(fPositiveSuffix)
        .append((UnicodeString)";") .append(fNegativePrefix).append((UnicodeString)"|").append(fNegativeSuffix)
        .append((UnicodeString)"]\n");
    debugout(s);
#endif
}

/**
 * Expand an affix pattern into an affix string.  All characters in the
 * pattern are literal unless prefixed by kQuote.  The following characters
 * after kQuote are recognized: PATTERN_PERCENT, PATTERN_PER_MILLE,
 * PATTERN_MINUS, and kCurrencySign.  If kCurrencySign is doubled (kQuote +
 * kCurrencySign + kCurrencySign), it is interpreted as an international
 * currency sign. If CURRENCY_SIGN is tripled, it is interpreted as
 * currency plural long names, such as "US Dollars".
 * Any other character after a kQuote represents itself.
 * kQuote must be followed by another character; kQuote may not occur by
 * itself at the end of the pattern.
 *
 * This method is used in two distinct ways.  First, it is used to expand
 * the stored affix patterns into actual affixes.  For this usage, doFormat
 * must be false.  Second, it is used to expand the stored affix patterns
 * given a specific number (doFormat == true), for those rare cases in
 * which a currency format references a ChoiceFormat (e.g., en_IN display
 * name for INR).  The number itself is taken from digitList.
 *
 * When used in the first way, this method has a side effect: It sets
 * currencyChoice to a ChoiceFormat object, if the currency's display name
 * in this locale is a ChoiceFormat pattern (very rare).  It only does this
 * if currencyChoice is null to start with.
 *
 * @param pattern the non-null, fPossibly empty pattern
 * @param affix string to receive the expanded equivalent of pattern.
 * Previous contents are deleted.
 * @param doFormat if false, then the pattern will be expanded, and if a
 * currency symbol is encountered that expands to a ChoiceFormat, the
 * currencyChoice member variable will be initialized if it is null.  If
 * doFormat is true, then it is assumed that the currencyChoice has been
 * created, and it will be used to format the value in digitList.
 * @param pluralCount the plural count. It is only used for currency
 *                    plural format. In which case, it is the plural
 *                    count of the currency amount. For example,
 *                    in en_US, it is the singular "one", or the plural
 *                    "other". For all other cases, it is null, and
 *                    is not being used.
 */
void DecimalFormat::expandAffix(const UnicodeString& pattern,
                                UnicodeString& affix,
                                double number,
                                FieldPositionHandler& handler,
                                UBool doFormat,
                                const UnicodeString* pluralCount) const {
    affix.remove();
    for (int i=0; i<pattern.length(); ) {
        UChar32 c = pattern.char32At(i);
        i += U16_LENGTH(c);
        if (c == kQuote) {
            c = pattern.char32At(i);
            i += U16_LENGTH(c);
            int beginIdx = affix.length();
            switch (c) {
            case kCurrencySign: {
                // As of ICU 2.2 we use the currency object, and
                // ignore the currency symbols in the DFS, unless
                // we have a null currency object.  This occurs if
                // resurrecting a pre-2.2 object or if the user
                // sets a custom DFS.
                UBool intl = i<pattern.length() &&
                    pattern.char32At(i) == kCurrencySign;
                UBool plural = FALSE;
                if (intl) {
                    ++i;
                    plural = i<pattern.length() &&
                        pattern.char32At(i) == kCurrencySign;
                    if (plural) {
                        intl = FALSE;
                        ++i;
                    }
                }
                const UChar* currencyUChars = getCurrency();
                if (currencyUChars[0] != 0) {
                    UErrorCode ec = U_ZERO_ERROR;
                    if (plural && pluralCount != NULL) {
                        // plural name is only needed when pluralCount != null,
                        // which means when formatting currency plural names.
                        // For other cases, pluralCount == null,
                        // and plural names are not needed.
                        int32_t len;
                        CharString pluralCountChar;
                        pluralCountChar.appendInvariantChars(*pluralCount, ec);
                        UBool isChoiceFormat;
                        const UChar* s = ucurr_getPluralName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() :
                            Locale::getDefault().getName(), &isChoiceFormat,
                            pluralCountChar.data(), &len, &ec);
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else if(intl) {
                        affix.append(currencyUChars, -1);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else {
                        int32_t len;
                        UBool isChoiceFormat;
                        // If fSymbols is NULL, use default locale
                        const UChar* s = ucurr_getName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() : Locale::getDefault().getName(),
                            UCURR_SYMBOL_NAME, &isChoiceFormat, &len, &ec);
                        if (isChoiceFormat) {
                            // Two modes here: If doFormat is false, we set up
                            // currencyChoice.  If doFormat is true, we use the
                            // previously created currencyChoice to format the
                            // value in digitList.
                            if (!doFormat) {
                                // If the currency is handled by a ChoiceFormat,
                                // then we're not going to use the expanded
                                // patterns.  Instantiate the ChoiceFormat and
                                // return.
                                if (fCurrencyChoice == NULL) {
                                    // TODO Replace double-check with proper thread-safe code
                                    ChoiceFormat* fmt = new ChoiceFormat(UnicodeString(s), ec);
                                    if (U_SUCCESS(ec)) {
                                        umtx_lock(NULL);
                                        if (fCurrencyChoice == NULL) {
                                            // Cast away const
                                            ((DecimalFormat*)this)->fCurrencyChoice = fmt;
                                            fmt = NULL;
                                        }
                                        umtx_unlock(NULL);
                                        delete fmt;
                                    }
                                }
                                // We could almost return null or "" here, since the
                                // expanded affixes are almost not used at all
                                // in this situation.  However, one method --
                                // toPattern() -- still does use the expanded
                                // affixes, in order to set up a padding
                                // pattern.  We use the CURRENCY_SIGN as a
                                // placeholder.
                                affix.append(kCurrencySign);
                            } else {
                                if (fCurrencyChoice != NULL) {
                                    FieldPosition pos(0); // ignored
                                    if (number < 0) {
                                        number = -number;
                                    }
                                    fCurrencyChoice->format(number, affix, pos);
                                } else {
                                    // We only arrive here if the currency choice
                                    // format in the locale data is INVALID.
                                    affix.append(currencyUChars, -1);
                                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                                }
                            }
                            continue;
                        }
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    }
                } else {
                    if(intl) {
                        affix += getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
                    } else {
                        affix += getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
                    }
                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                }
                break;
            }
            case kPatternPercent:
                affix += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                handler.addAttribute(kPercentField, beginIdx, affix.length());
                break;
            case kPatternPerMill:
                affix += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                handler.addAttribute(kPermillField, beginIdx, affix.length());
                break;
            case kPatternPlus:
                affix += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            case kPatternMinus:
                affix += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            default:
                affix.append(c);
                break;
            }
        }
        else {
            affix.append(c);
        }
    }
}

/**
 * Append an affix to the given StringBuffer.
 * @param buf buffer to append to
 * @param isNegative
 * @param isPrefix
 */
int32_t DecimalFormat::appendAffix(UnicodeString& buf, double number,
                                   FieldPositionHandler& handler,
                                   UBool isNegative, UBool isPrefix) const {
    // plural format precedes choice format
    if (fCurrencyChoice != 0 &&
        fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        const UnicodeString* affixPat;
        if (isPrefix) {
            affixPat = isNegative ? fNegPrefixPattern : fPosPrefixPattern;
        } else {
            affixPat = isNegative ? fNegSuffixPattern : fPosSuffixPattern;
        }
        if (affixPat) {
            UnicodeString affixBuf;
            expandAffix(*affixPat, affixBuf, number, handler, TRUE, NULL);
            buf.append(affixBuf);
            return affixBuf.length();
        }
        // else someone called a function that reset the pattern.
    }

    const UnicodeString* affix;
    if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
        // TODO: get an accurate count of visible fraction digits.
        UnicodeString pluralCount;
        int32_t minFractionDigits = this->getMinimumFractionDigits();
        if (minFractionDigits > 0) {
            FixedDecimal ni(number, this->getMinimumFractionDigits());
            pluralCount = fCurrencyPluralInfo->getPluralRules()->select(ni);
        } else {
            pluralCount = fCurrencyPluralInfo->getPluralRules()->select(number);
        }
        AffixesForCurrency* oneSet;
        if (fStyle == UNUM_CURRENCY_PLURAL) {
            oneSet = (AffixesForCurrency*)fPluralAffixesForCurrency->get(pluralCount);
        } else {
            oneSet = (AffixesForCurrency*)fAffixesForCurrency->get(pluralCount);
        }
        if (isPrefix) {
            affix = isNegative ? &oneSet->negPrefixForCurrency :
                                 &oneSet->posPrefixForCurrency;
        } else {
            affix = isNegative ? &oneSet->negSuffixForCurrency :
                                 &oneSet->posSuffixForCurrency;
        }
    } else {
        if (isPrefix) {
            affix = isNegative ? &fNegativePrefix : &fPositivePrefix;
        } else {
            affix = isNegative ? &fNegativeSuffix : &fPositiveSuffix;
        }
    }

    int32_t begin = (int) buf.length();

    buf.append(*affix);

    if (handler.isRecording()) {
      int32_t offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
        handler.addAttribute(kSignField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
        handler.addAttribute(kPercentField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
        handler.addAttribute(kPermillField, begin + offset, begin + offset + aff.length());
      }
    }
    return affix->length();
}

/**
 * Appends an affix pattern to the given StringBuffer, quoting special
 * characters as needed.  Uses the internal affix pattern, if that exists,
 * or the literal affix, if the internal affix pattern is null.  The
 * appended string will generate the same affix pattern (or literal affix)
 * when passed to toPattern().
 *
 * @param appendTo the affix string is appended to this
 * @param affixPattern a pattern such as fPosPrefixPattern; may be null
 * @param expAffix a corresponding expanded affix, such as fPositivePrefix.
 * Ignored unless affixPattern is null.  If affixPattern is null, then
 * expAffix is appended as a literal affix.
 * @param localized true if the appended pattern should contain localized
 * pattern characters; otherwise, non-localized pattern chars are appended
 */
void DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                       const UnicodeString* affixPattern,
                                       const UnicodeString& expAffix,
                                       UBool localized) const {
    if (affixPattern == 0) {
        appendAffixPattern(appendTo, expAffix, localized);
    } else {
        int i;
        for (int pos=0; pos<affixPattern->length(); pos=i) {
            i = affixPattern->indexOf(kQuote, pos);
            if (i < 0) {
                UnicodeString s;
                affixPattern->extractBetween(pos, affixPattern->length(), s);
                appendAffixPattern(appendTo, s, localized);
                break;
            }
            if (i > pos) {
                UnicodeString s;
                affixPattern->extractBetween(pos, i, s);
                appendAffixPattern(appendTo, s, localized);
            }
            UChar32 c = affixPattern->char32At(++i);
            ++i;
            if (c == kQuote) {
                appendTo.append(c).append(c);
                // Fall through and append another kQuote below
            } else if (c == kCurrencySign &&
                       i<affixPattern->length() &&
                       affixPattern->char32At(i) == kCurrencySign) {
                ++i;
                appendTo.append(c).append(c);
            } else if (localized) {
                switch (c) {
                case kPatternPercent:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                    break;
                case kPatternPerMill:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                    break;
                case kPatternPlus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                    break;
                case kPatternMinus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                    break;
                default:
                    appendTo.append(c);
                }
            } else {
                appendTo.append(c);
            }
        }
    }
}

/**
 * Append an affix to the given StringBuffer, using quotes if
 * there are special characters.  Single quotes themselves must be
 * escaped in either case.
 */
void
DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                  const UnicodeString& affix,
                                  UBool localized) const {
    UBool needQuote;
    if(localized) {
        needQuote = affix.indexOf(getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol)) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    else {
        needQuote = affix.indexOf(kPatternZeroDigit) >= 0
            || affix.indexOf(kPatternGroupingSeparator) >= 0
            || affix.indexOf(kPatternDecimalSeparator) >= 0
            || affix.indexOf(kPatternPercent) >= 0
            || affix.indexOf(kPatternPerMill) >= 0
            || affix.indexOf(kPatternDigit) >= 0
            || affix.indexOf(kPatternSeparator) >= 0
            || affix.indexOf(kPatternExponent) >= 0
            || affix.indexOf(kPatternPlus) >= 0
            || affix.indexOf(kPatternMinus) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    if (needQuote)
        appendTo += (UChar)0x0027 /*'\''*/;
    if (affix.indexOf((UChar)0x0027 /*'\''*/) < 0)
        appendTo += affix;
    else {
        for (int32_t j = 0; j < affix.length(); ) {
            UChar32 c = affix.char32At(j);
            j += U16_LENGTH(c);
            appendTo += c;
            if (c == 0x0027 /*'\''*/)
                appendTo += c;
        }
    }
    if (needQuote)
        appendTo += (UChar)0x0027 /*'\''*/;
}

void
DecimalFormat::applyPattern(const UnicodeString& pattern, UErrorCode& status)
{
    fImpl->applyPattern(pattern, status);
    UParseError parseError;
    applyPattern(pattern, FALSE, parseError, status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UParseError& parseError,
                            UErrorCode& status)
{
    fImpl->applyPattern(pattern, status);
    applyPattern(pattern, FALSE, parseError, status);
}
//------------------------------------------------------------------------------

void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern, UErrorCode& status)
{
    fImpl->applyLocalizedPattern(pattern, status);
    UParseError parseError;
    applyPattern(pattern, TRUE,parseError,status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                     UParseError& parseError,
                                     UErrorCode& status)
{
    fImpl->applyLocalizedPattern(pattern, status);
    applyPattern(pattern, TRUE,parseError,status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyPatternWithoutExpandAffix(const UnicodeString& pattern,
                                              UBool localized,
                                              UParseError& parseError,
                                              UErrorCode& status)
{
    if (U_FAILURE(status))
    {
        return;
    }
    DecimalFormatPatternParser patternParser;
    if (localized) {
      patternParser.useSymbols(*fSymbols);
    }
    fFormatPattern = pattern;
    DecimalFormatPattern out;
    patternParser.applyPatternWithoutExpandAffix(
        pattern,
        out,
        parseError,
        status);
    if (U_FAILURE(status)) {
      return;
    }

    setMinimumIntegerDigits(out.fMinimumIntegerDigits);
    setMaximumIntegerDigits(out.fMaximumIntegerDigits);
    setMinimumFractionDigits(out.fMinimumFractionDigits);
    setMaximumFractionDigits(out.fMaximumFractionDigits);
    setSignificantDigitsUsed(out.fUseSignificantDigits);
    if (out.fUseSignificantDigits) {
        setMinimumSignificantDigits(out.fMinimumSignificantDigits);
        setMaximumSignificantDigits(out.fMaximumSignificantDigits);
    }
    fUseExponentialNotation = out.fUseExponentialNotation;
    if (out.fUseExponentialNotation) {
        fMinExponentDigits = out.fMinExponentDigits;
    }
    fExponentSignAlwaysShown = out.fExponentSignAlwaysShown;
    fCurrencySignCount = out.fCurrencySignCount;
    setGroupingUsed(out.fGroupingUsed);
    fGroupingSize = out.fGroupingSize;
    fGroupingSize2 = out.fGroupingSize2;
    setMultiplier(out.fMultiplier);
    fDecimalSeparatorAlwaysShown = out.fDecimalSeparatorAlwaysShown;
    fFormatWidth = out.fFormatWidth;
    if (out.fRoundingIncrementUsed) {
        if (fRoundingIncrement != NULL) {
            *fRoundingIncrement = out.fRoundingIncrement;
        } else {
            fRoundingIncrement = new DigitList(out.fRoundingIncrement);
            /* test for NULL */
            if (fRoundingIncrement == NULL) {
                 status = U_MEMORY_ALLOCATION_ERROR;
                 return;
            }
        }
    } else {
        setRoundingIncrement(0.0);
    }
    fPad = out.fPad;
    switch (out.fPadPosition) {
        case DecimalFormatPattern::kPadBeforePrefix:
            fPadPosition = kPadBeforePrefix;
            break;
        case DecimalFormatPattern::kPadAfterPrefix:
            fPadPosition = kPadAfterPrefix;
            break;
        case DecimalFormatPattern::kPadBeforeSuffix:
            fPadPosition = kPadBeforeSuffix;
            break;
        case DecimalFormatPattern::kPadAfterSuffix:
            fPadPosition = kPadAfterSuffix;
            break;
    }
    copyString(out.fNegPrefixPattern, out.fNegPatternsBogus, fNegPrefixPattern, status);
    copyString(out.fNegSuffixPattern, out.fNegPatternsBogus, fNegSuffixPattern, status);
    copyString(out.fPosPrefixPattern, out.fPosPatternsBogus, fPosPrefixPattern, status);
    copyString(out.fPosSuffixPattern, out.fPosPatternsBogus, fPosSuffixPattern, status);
}


void
DecimalFormat::expandAffixAdjustWidth(const UnicodeString* pluralCount) {
    expandAffixes(pluralCount);
    if (fFormatWidth > 0) {
        // Finish computing format width (see above)
            // TODO: how to handle fFormatWidth,
            // need to save in f(Plural)AffixesForCurrecy?
            fFormatWidth += fPositivePrefix.length() + fPositiveSuffix.length();
    }
}


void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UBool localized,
                            UParseError& parseError,
                            UErrorCode& status)
{
    // do the following re-set first. since they change private data by
    // apply pattern again.
    if (pattern.indexOf(kCurrencySign) != -1) {
        if (fCurrencyPluralInfo == NULL) {
            // initialize currencyPluralInfo if needed
            fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        }
        if (fAffixPatternsForCurrency == NULL) {
            setupCurrencyAffixPatterns(status);
        }
        if (pattern.indexOf(fgTripleCurrencySign, 3, 0) != -1) {
            // only setup the affixes of the current pattern.
            setupCurrencyAffixes(pattern, TRUE, FALSE, status);
        }
    }
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(NULL);
}


void
DecimalFormat::applyPatternInternally(const UnicodeString& pluralCount,
                                      const UnicodeString& pattern,
                                      UBool localized,
                                      UParseError& parseError,
                                      UErrorCode& status) {
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(&pluralCount);
}


/**
 * Sets the maximum number of digits allowed in the integer portion of a
 * number. 
 * @see NumberFormat#setMaximumIntegerDigits
 */
void DecimalFormat::setMaximumIntegerDigits(int32_t newValue) {
    newValue = _min(newValue, gDefaultMaxIntegerDigits);
    NumberFormat::setMaximumIntegerDigits(newValue);
    fImpl->setMinMaxIntegerDigits(
            NumberFormat::getMinimumIntegerDigits(),
            NumberFormat::getMaximumIntegerDigits());
}

/**
 * Sets the minimum number of digits allowed in the integer portion of a
 * number. This override limits the integer digit count to 309.
 * @see NumberFormat#setMinimumIntegerDigits
 */
void DecimalFormat::setMinimumIntegerDigits(int32_t newValue) {
    newValue = _min(newValue, kDoubleIntegerDigits);
    NumberFormat::setMinimumIntegerDigits(newValue);
    fImpl->setMinMaxIntegerDigits(
            NumberFormat::getMinimumIntegerDigits(),
            NumberFormat::getMaximumIntegerDigits());
}

/**
 * Sets the maximum number of digits allowed in the fraction portion of a
 * number. This override limits the fraction digit count to 340.
 * @see NumberFormat#setMaximumFractionDigits
 */
void DecimalFormat::setMaximumFractionDigits(int32_t newValue) {
    newValue = _min(newValue, kDoubleFractionDigits);
    NumberFormat::setMaximumFractionDigits(newValue);
    fImpl->setMinMaxFractionDigits(
            NumberFormat::getMinimumFractionDigits(),
            NumberFormat::getMaximumFractionDigits());
}

/**
 * Sets the minimum number of digits allowed in the fraction portion of a
 * number. This override limits the fraction digit count to 340.
 * @see NumberFormat#setMinimumFractionDigits
 */
void DecimalFormat::setMinimumFractionDigits(int32_t newValue) {
    newValue = _min(newValue, kDoubleFractionDigits);
    NumberFormat::setMinimumFractionDigits(newValue);
    fImpl->setMinMaxFractionDigits(
            NumberFormat::getMinimumFractionDigits(),
            NumberFormat::getMaximumFractionDigits());
}

int32_t DecimalFormat::getMinimumSignificantDigits() const {
    return fImpl->getMinimumSignificantDigits();
}

int32_t DecimalFormat::getMaximumSignificantDigits() const {
    return fImpl->getMaximumSignificantDigits();
}

void DecimalFormat::setMinimumSignificantDigits(int32_t min) {
    if (min < 1) {
        min = 1;
    }
    // pin max sig dig to >= min
    int32_t max = _max(fImpl->fMaxSigDigits, min);
    fImpl->setMinMaxSignificantDigits(min, max);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
    fUseSignificantDigits = TRUE;
}

void DecimalFormat::setMaximumSignificantDigits(int32_t max) {
    if (max < 1) {
        max = 1;
    }
    // pin min sig dig to 1..max
    U_ASSERT(fImpl->fMinSigDigits >= 1);
    int32_t min = _min(fImpl->fMinSigDigits, max);
    fImpl->setMinMaxSignificantDigits(min, max);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
    fUseSignificantDigits = TRUE;
}

UBool DecimalFormat::areSignificantDigitsUsed() const {
    return fImpl->areSignificantDigitsUsed();
}

void DecimalFormat::setSignificantDigitsUsed(UBool useSignificantDigits) {
    fImpl->setSignificantDigitsUsed(useSignificantDigits);
    fUseSignificantDigits = useSignificantDigits;
}

void DecimalFormat::setCurrencyInternally(const UChar* theCurrency,
                                          UErrorCode& ec) {
    // If we are a currency format, then modify our affixes to
    // encode the currency symbol for the given currency in our
    // locale, and adjust the decimal digits and rounding for the
    // given currency.

    // Note: The code is ordered so that this object is *not changed*
    // until we are sure we are going to succeed.

    // NULL or empty currency is *legal* and indicates no currency.
    UBool isCurr = (theCurrency && *theCurrency);

    double rounding = 0.0;
    int32_t frac = 0;
    if (fCurrencySignCount != fgCurrencySignCountZero && isCurr) {
        rounding = ucurr_getRoundingIncrementForUsage(theCurrency, fCurrencyUsage, &ec);
        frac = ucurr_getDefaultFractionDigitsForUsage(theCurrency, fCurrencyUsage, &ec);
    }

    NumberFormat::setCurrency(theCurrency, ec);
    if (U_FAILURE(ec)) return;

    if (fCurrencySignCount != fgCurrencySignCountZero) {
        // NULL or empty currency is *legal* and indicates no currency.
        if (isCurr) {
            setRoundingIncrement(rounding);
            setMinimumFractionDigits(frac);
            setMaximumFractionDigits(frac);
        }
        expandAffixes(NULL);
    }
}

void DecimalFormat::setCurrency(const UChar* theCurrency, UErrorCode& ec) {
    // set the currency before compute affixes to get the right currency names
    NumberFormat::setCurrency(theCurrency, ec);
    fImpl->setCurrency(theCurrency, ec);
    if (fFormatPattern.indexOf(fgTripleCurrencySign, 3, 0) != -1) {
        UnicodeString savedPtn = fFormatPattern;
        setupCurrencyAffixes(fFormatPattern, TRUE, TRUE, ec);
        UParseError parseErr;
        applyPattern(savedPtn, FALSE, parseErr, ec);
    }
    // set the currency after apply pattern to get the correct rounding/fraction
    setCurrencyInternally(theCurrency, ec);
}

void DecimalFormat::setCurrencyUsage(UCurrencyUsage newContext, UErrorCode* ec){
    fImpl->setCurrencyUsage(newContext, *ec);
    fCurrencyUsage = newContext;

    const UChar* theCurrency = getCurrency();

    // We set rounding/digit based on currency context
    if(theCurrency){
        double rounding = ucurr_getRoundingIncrementForUsage(theCurrency, fCurrencyUsage, ec);
        int32_t frac = ucurr_getDefaultFractionDigitsForUsage(theCurrency, fCurrencyUsage, ec);

        if (U_SUCCESS(*ec)) {
            setRoundingIncrement(rounding);
            setMinimumFractionDigits(frac);
            setMaximumFractionDigits(frac);
        }
    }
}

UCurrencyUsage DecimalFormat::getCurrencyUsage() const {
    return fImpl->getCurrencyUsage();
}

// Deprecated variant with no UErrorCode parameter
void DecimalFormat::setCurrency(const UChar* theCurrency) {
    UErrorCode ec = U_ZERO_ERROR;
    setCurrency(theCurrency, ec);
}

void DecimalFormat::getEffectiveCurrency(UChar* result, UErrorCode& ec) const {
    if (fSymbols == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    ec = U_ZERO_ERROR;
    const UChar* c = getCurrency();
    if (*c == 0) {
        const UnicodeString &intl =
            fSymbols->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        c = intl.getBuffer(); // ok for intl to go out of scope
    }
    u_strncpy(result, c, 3);
    result[3] = 0;
}

/**
 * Return the number of fraction digits to display, or the total
 * number of digits for significant digit formats and exponential
 * formats.
 */
int32_t
DecimalFormat::precision() const {
    if (areSignificantDigitsUsed()) {
        return getMaximumSignificantDigits();
    } else if (fUseExponentialNotation) {
        return getMinimumIntegerDigits() + getMaximumFractionDigits();
    } else {
        return getMaximumFractionDigits();
    }
}


// TODO: template algorithm
Hashtable*
DecimalFormat::initHashForAffix(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixValueComparator);
    return hTable;
}

Hashtable*
DecimalFormat::initHashForAffixPattern(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixPatternValueComparator);
    return hTable;
}

void
DecimalFormat::deleteHashForAffix(Hashtable*& table)
{
    if ( table == NULL ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    while ( (element = table->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
        delete value;
    }
    delete table;
    table = NULL;
}



void
DecimalFormat::deleteHashForAffixPattern()
{
    if ( fAffixPatternsForCurrency == NULL ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
        delete value;
    }
    delete fAffixPatternsForCurrency;
    fAffixPatternsForCurrency = NULL;
}


void
DecimalFormat::copyHashForAffixPattern(const Hashtable* source,
                                       Hashtable* target,
                                       UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
            AffixPatternsForCurrency* copy = new AffixPatternsForCurrency(
                value->negPrefixPatternForCurrency,
                value->negSuffixPatternForCurrency,
                value->posPrefixPatternForCurrency,
                value->posSuffixPatternForCurrency,
                value->patternType);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}

void
DecimalFormat::setGroupingUsed(UBool newValue) {
  NumberFormat::setGroupingUsed(newValue);
  fImpl->setGroupingUsed(newValue);
}

void
DecimalFormat::setParseIntegerOnly(UBool newValue) {
  NumberFormat::setParseIntegerOnly(newValue);
}

void
DecimalFormat::setContext(UDisplayContext value, UErrorCode& status) {
  NumberFormat::setContext(value, status);
}

DecimalFormat& DecimalFormat::setAttribute( UNumberFormatAttribute attr,
                                            int32_t newValue,
                                            UErrorCode &status) {
  if(U_FAILURE(status)) return *this;

  switch(attr) {
  case UNUM_LENIENT_PARSE:
    setLenient(newValue!=0);
    break;

    case UNUM_PARSE_INT_ONLY:
      setParseIntegerOnly(newValue!=0);
      break;
      
    case UNUM_GROUPING_USED:
      setGroupingUsed(newValue!=0);
      break;
        
    case UNUM_DECIMAL_ALWAYS_SHOWN:
      setDecimalSeparatorAlwaysShown(newValue!=0);
        break;
        
    case UNUM_MAX_INTEGER_DIGITS:
      setMaximumIntegerDigits(newValue);
        break;
        
    case UNUM_MIN_INTEGER_DIGITS:
      setMinimumIntegerDigits(newValue);
        break;
        
    case UNUM_INTEGER_DIGITS:
      setMinimumIntegerDigits(newValue);
      setMaximumIntegerDigits(newValue);
        break;
        
    case UNUM_MAX_FRACTION_DIGITS:
      setMaximumFractionDigits(newValue);
        break;
        
    case UNUM_MIN_FRACTION_DIGITS:
      setMinimumFractionDigits(newValue);
        break;
        
    case UNUM_FRACTION_DIGITS:
      setMinimumFractionDigits(newValue);
      setMaximumFractionDigits(newValue);
      break;
        
    case UNUM_SIGNIFICANT_DIGITS_USED:
      setSignificantDigitsUsed(newValue!=0);
        break;

    case UNUM_MAX_SIGNIFICANT_DIGITS:
      setMaximumSignificantDigits(newValue);
        break;
        
    case UNUM_MIN_SIGNIFICANT_DIGITS:
      setMinimumSignificantDigits(newValue);
        break;
        
    case UNUM_MULTIPLIER:
      setMultiplier(newValue);    
       break;
        
    case UNUM_GROUPING_SIZE:
      setGroupingSize(newValue);    
        break;
        
    case UNUM_ROUNDING_MODE:
      setRoundingMode((DecimalFormat::ERoundingMode)newValue);
        break;
        
    case UNUM_FORMAT_WIDTH:
      setFormatWidth(newValue);
        break;
        
    case UNUM_PADDING_POSITION:
        /** The position at which padding will take place. */
      setPadPosition((DecimalFormat::EPadPosition)newValue);
        break;
        
    case UNUM_SECONDARY_GROUPING_SIZE:
      setSecondaryGroupingSize(newValue);
        break;

#if UCONFIG_HAVE_PARSEALLINPUT
    case UNUM_PARSE_ALL_INPUT:
      setParseAllInput((UNumberFormatAttributeValue)newValue);
        break;
#endif

    /* These are stored in fBoolFlags */
    case UNUM_PARSE_NO_EXPONENT:
    case UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS:
    case UNUM_PARSE_DECIMAL_MARK_REQUIRED:
      if(!fBoolFlags.isValidValue(newValue)) {
          status = U_ILLEGAL_ARGUMENT_ERROR;
      } else {
          if (attr == UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS) {
              fImpl->setFailIfMoreThanMaxDigits((UBool) newValue);
          }
          fBoolFlags.set(attr, newValue);
      }
      break;

    case UNUM_SCALE:
        fScale = newValue;
        fImpl->setScale(newValue);
        break;

    case UNUM_CURRENCY_USAGE:
        setCurrencyUsage((UCurrencyUsage)newValue, &status);

    default:
      status = U_UNSUPPORTED_ERROR;
      break;
  }
  return *this;
}

int32_t DecimalFormat::getAttribute( UNumberFormatAttribute attr, 
                                     UErrorCode &status ) const {
  if(U_FAILURE(status)) return -1;
  switch(attr) {
    case UNUM_LENIENT_PARSE: 
        return isLenient();

    case UNUM_PARSE_INT_ONLY:
        return isParseIntegerOnly();
        
    case UNUM_GROUPING_USED:
        return isGroupingUsed();
        
    case UNUM_DECIMAL_ALWAYS_SHOWN:
        return isDecimalSeparatorAlwaysShown();    
        
    case UNUM_MAX_INTEGER_DIGITS:
        return getMaximumIntegerDigits();
        
    case UNUM_MIN_INTEGER_DIGITS:
        return getMinimumIntegerDigits();
        
    case UNUM_INTEGER_DIGITS:
        // TBD: what should this return?
        return getMinimumIntegerDigits();
        
    case UNUM_MAX_FRACTION_DIGITS:
        return getMaximumFractionDigits();
        
    case UNUM_MIN_FRACTION_DIGITS:
        return getMinimumFractionDigits();
        
    case UNUM_FRACTION_DIGITS:
        // TBD: what should this return?
        return getMinimumFractionDigits();
        
    case UNUM_SIGNIFICANT_DIGITS_USED:
        return areSignificantDigitsUsed();
        
    case UNUM_MAX_SIGNIFICANT_DIGITS:
        return getMaximumSignificantDigits();
        
    case UNUM_MIN_SIGNIFICANT_DIGITS:
        return getMinimumSignificantDigits();
        
    case UNUM_MULTIPLIER:
        return getMultiplier();    
        
    case UNUM_GROUPING_SIZE:
        return getGroupingSize();    
        
    case UNUM_ROUNDING_MODE:
        return getRoundingMode();
        
    case UNUM_FORMAT_WIDTH:
        return getFormatWidth();
        
    case UNUM_PADDING_POSITION:
        return getPadPosition();
        
    case UNUM_SECONDARY_GROUPING_SIZE:
        return getSecondaryGroupingSize();
        
    /* These are stored in fBoolFlags */
    case UNUM_PARSE_NO_EXPONENT:
    case UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS:
    case UNUM_PARSE_DECIMAL_MARK_REQUIRED:
      return fBoolFlags.get(attr);

    case UNUM_SCALE:
        return fScale;

    case UNUM_CURRENCY_USAGE:
        return fCurrencyUsage;

    default:
        status = U_UNSUPPORTED_ERROR;
        break;
  }

  return -1; /* undefined */
}

#if UCONFIG_HAVE_PARSEALLINPUT
void DecimalFormat::setParseAllInput(UNumberFormatAttributeValue value) {
  fParseAllInput = value;
}
#endif

void
DecimalFormat::copyHashForAffix(const Hashtable* source,
                                Hashtable* target,
                                UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = UHASH_FIRST;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;

            const UHashTok valueTok = element->value;
            const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
            AffixesForCurrency* copy = new AffixesForCurrency(
                value->negPrefixForCurrency,
                value->negSuffixForCurrency,
                value->posPrefixForCurrency,
                value->posSuffixForCurrency);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
