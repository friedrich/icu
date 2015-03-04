/*
 * Copyright (C) 2015, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *
 * file name: affixpatternparser.cpp
 */

#include "affixpatternparser.h"

#include "charstr.h"
#include "unicode/ucurr.h"
#include "unicode/plurrule.h"
#include "precision.h"
#include "unicode/dcfmtsym.h"
#include "uassert.h"

static const int32_t kMaxTokenLength = 0x0F;

#define PACK_TOKEN_AND_LENGTH(t, l) ((UChar) (((t) << 8) | (l & 0x0F)))

#define UNPACK_TOKEN(c) ((AffixPattern::ETokenType) ((c) >> 8))

#define UNPACK_LENGTH(c) ((c) & 0x0F)

U_NAMESPACE_BEGIN

static int32_t
nextToken(const UChar *buffer, int32_t idx, int32_t len, UChar *token) {
    if (buffer[idx] != 0x27 || idx + 1 == len) {
        *token = buffer[idx];
        return 1;
    }
    *token = buffer[idx + 1];
    if (buffer[idx + 1] == 0xA4) {
        int32_t i = 2;
        for (; idx + i < len && i < 4 && buffer[idx + i] == buffer[idx + 1]; ++i);
        return i;
    }
    return 2;
}

CurrencyAffixInfo::CurrencyAffixInfo() {
    UErrorCode status = U_ZERO_ERROR;
    set(NULL, NULL, NULL, status);
}

void
CurrencyAffixInfo::set(
        const char *locale,
        const PluralRules *rules,
        const UChar *currency,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (currency == NULL) {
        static UChar defaultSymbols[] = {0xa4, 0xa4, 0xa4};
        fSymbol.setTo(defaultSymbols, 1);
        fISO.setTo(defaultSymbols, 2);
        fLong.remove();
        fLong.append(UnicodeString(defaultSymbols, 3));
        return;
    }
    int32_t len;
    UBool unusedIsChoice;
    const UChar *symbol = ucurr_getName(
            currency, locale, UCURR_SYMBOL_NAME, &unusedIsChoice,
            &len, &status);
    if (U_FAILURE(status)) {
        return;
    }
    fSymbol.setTo(symbol, len);
    fISO.setTo(currency, u_strlen(currency));
    fLong.remove();
    StringEnumeration* keywords = rules->getKeywords(status);
    if (U_FAILURE(status)) {
        return;
    }
    const UnicodeString* pluralCount;
    while ((pluralCount = keywords->snext(status)) != NULL) {
        CharString pCount;
        pCount.appendInvariantChars(*pluralCount, status);
        const UChar *pluralName = ucurr_getPluralName(
            currency, locale, &unusedIsChoice, pCount.data(),
            &len, &status);
        fLong.setVariant(pCount.data(), UnicodeString(pluralName, len), status);
    }
    delete keywords;
}

void
CurrencyAffixInfo::adjustPrecision(
        const UChar *currency, const UCurrencyUsage usage,
        FixedPrecision &precision, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    int32_t digitCount = ucurr_getDefaultFractionDigitsForUsage(
            currency, usage, &status);
    precision.fMin.setFracDigitCount(digitCount);
    precision.fMax.setFracDigitCount(digitCount);
    double increment = ucurr_getRoundingIncrementForUsage(
            currency, usage, &status);
    if (increment == 0.0) {
        precision.fRoundingIncrement.clear();
    } else {
        precision.fRoundingIncrement.set(increment);
        // guard against round-off error
        precision.fRoundingIncrement.round(6);
    }
}

void
AffixPattern::addLiteral(
        const UChar *literal, int32_t start, int32_t len) {
    char32Count += u_countChar32(literal + start, len);
    int32_t leftToCopy = len;
    int32_t copyStart = start;
    while (leftToCopy > 0) {
        int32_t copyLen =
                leftToCopy > kMaxTokenLength ? kMaxTokenLength : leftToCopy;
        literals.append(literal, copyStart, copyLen);
        tokens.append(PACK_TOKEN_AND_LENGTH(kLiteral, copyLen));
        copyStart += copyLen;
        leftToCopy -= copyLen;
    }
}

void
AffixPattern::add(ETokenType t) {
    add(t, 1);
}

void
AffixPattern::addCurrency(uint8_t count) {
    add(kCurrency, count);
}

void
AffixPattern::add(ETokenType t, uint8_t count) {
    U_ASSERT(t != kLiteral);
    char32Count += count;
    switch (t) {
    case kCurrency: 
        hasCurrencyToken = TRUE;
        break;
    case kPercent:
        hasPercentToken = TRUE;
        break;
    case kPerMill:
        hasPermillToken = TRUE;
        break;
    }
    tokens.append(PACK_TOKEN_AND_LENGTH(t, count));
}

void
AffixPattern::remove() {
    tokens.remove();
    literals.remove();
    hasCurrencyToken = FALSE;
    hasPercentToken = FALSE;
    hasPermillToken = FALSE;
    char32Count = 0;
}

AffixPattern &
AffixPattern::parseAffixString(
        const UnicodeString &affixStr,
        AffixPattern &appendTo, 
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    int32_t len = affixStr.length();
    const UChar *buffer = affixStr.getBuffer();
    for (int32_t i = 0; i < len; ) {
        UChar token;
        int32_t tokenSize = nextToken(buffer, i, len, &token);
        if (tokenSize == 1) {
            int32_t literalStart = i;
            ++i;
            while (i < len && (tokenSize = nextToken(buffer, i, len, &token)) == 1) {
                ++i;
            }
            appendTo.addLiteral(buffer, literalStart, i - literalStart);

            // If we reached end of string, we are done
            if (i == len) {
                return appendTo;
            }
        }
        i += tokenSize;
        switch (token) {
        case 0x25:
            appendTo.add(kPercent, 1);
            break;
        case 0x2030:
            appendTo.add(kPerMill, 1);
            break;
        case 0x2D:
            appendTo.add(kNegative, 1);
            break;
        case 0xA4:
            {
                if (tokenSize - 1 > 3) {
                    status = U_PARSE_ERROR;
                    return appendTo;
                }
                appendTo.add(kCurrency, tokenSize - 1);
            }
            break;
        default:
            appendTo.addLiteral(&token, 0, 1);
            break;
        }
    }
    return appendTo;
}

AffixPatternIterator &
AffixPattern::iterator(AffixPatternIterator &result) const {
    result.nextLiteralIndex = 0;
    result.nextTokenIndex = 0;
    result.tokens = &tokens;
    result.literals = &literals;
    return result;
}

UBool
AffixPatternIterator::nextToken() {
    if (nextTokenIndex == tokens->length()) {
        return FALSE;
    }
    UChar packed = tokens->charAt(nextTokenIndex);
    AffixPattern::ETokenType token = UNPACK_TOKEN(packed);
    if (token == AffixPattern::kLiteral) {
        nextLiteralIndex += UNPACK_LENGTH(packed);
    }
    ++nextTokenIndex;
    return TRUE;
}

AffixPattern::ETokenType
AffixPatternIterator::getTokenType() const {
    return UNPACK_TOKEN(tokens->charAt(nextTokenIndex - 1));
}

UnicodeString &
AffixPatternIterator::getLiteral(UnicodeString &result) const {
    const UChar *buffer = literals->getBuffer();
    int32_t len = UNPACK_LENGTH(tokens->charAt(nextTokenIndex - 1));
    result.setTo(FALSE, buffer + (nextLiteralIndex - len), len);
    return result;
}

int32_t
AffixPatternIterator::getTokenLength() const {
    return UNPACK_LENGTH(tokens->charAt(nextTokenIndex - 1));
}

AffixPatternParser::AffixPatternParser()
        : fPercent("%"), fPermill("\u2030"), fNegative("-") {
    fPermill = fPermill.unescape();
}

AffixPatternParser::AffixPatternParser(
        const DecimalFormatSymbols &symbols) {
    setDecimalFormatSymbols(symbols);
}

void
AffixPatternParser::setDecimalFormatSymbols(
        const DecimalFormatSymbols &symbols) {
    fPercent = symbols.getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
    fPermill = symbols.getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
    fNegative = symbols.getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
}

int32_t
AffixPatternParser::parse(
        const AffixPattern &affixPattern,
        PluralAffix &appendTo, 
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    AffixPatternIterator iter;
    affixPattern.iterator(iter);
    int32_t result = 0;
    UnicodeString literal;
    while (iter.nextToken()) {
        switch (iter.getTokenType()) {
        case AffixPattern::kPercent:
            appendTo.append(fPercent, UNUM_PERCENT_FIELD);
            result = 2;
            break;
        case AffixPattern::kPerMill:
            appendTo.append(fPermill, UNUM_PERMILL_FIELD);
            result = 3;
            break;
        case AffixPattern::kNegative:
            appendTo.append(fNegative, UNUM_SIGN_FIELD);
            break;
        case AffixPattern::kCurrency:
            switch (iter.getTokenLength()) {
                case 1:
                    appendTo.append(
                            fCurrencyAffixInfo.fSymbol, UNUM_CURRENCY_FIELD);
                    break;
                case 2:
                    appendTo.append(
                            fCurrencyAffixInfo.fISO, UNUM_CURRENCY_FIELD);
                    break;
                case 3:
                    appendTo.append(
                            fCurrencyAffixInfo.fLong, UNUM_CURRENCY_FIELD, status);
                    break;
                default:
                    U_ASSERT(FALSE);
                    break;
            }
            break;
        case AffixPattern::kLiteral:
            appendTo.append(iter.getLiteral(literal));
            break;
        default:
            U_ASSERT(FALSE);
            break;
        }
    }
    return result;
}


U_NAMESPACE_END

