/*
******************************************************************************
*
*   Copyright (C) 2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  ubiditransform.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2016jul24
*   created by: Lina Kemmel
*
*/

#include "cmemory.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"
#include "unicode/ushape.h"
#include "unicode/utf16.h"
#include "ustr_imp.h"
#include "ubiditransform.h"

/* Some convenience defines */
#define LTR                     UBIDI_LTR
#define RTL                     UBIDI_RTL
#define LOGICAL                 UBIDI_LOGICAL
#define VISUAL                  UBIDI_VISUAL
#define NOMINAL                 U_SHAPE_DIGITS_NOOP
#define SHAPE_LOGICAL           U_SHAPE_TEXT_DIRECTION_LOGICAL
#define SHAPE_VISUAL            U_SHAPE_TEXT_DIRECTION_VISUAL_LTR

#define CHECK_LEN(STR, LEN, ERROR) { \
        if (LEN == 0) return 0; \
        if (LEN < -1) { *(ERROR) = U_ILLEGAL_ARGUMENT_ERROR; return 0; } \
        if (LEN == -1) LEN = u_strlen(STR); \
    } 

#define MAX_ACTIONS     7

/**
 * Typedef for a pointer to a function, which performs some operation (such as
 * reordering, setting "inverse" mode, character mirroring, etc.). Return value
 * indicates whether the text was changed in the course of this operation or
 * not.
 */
typedef UBool (*bidiAction)(UBiDiTransform *);

/**
 * Structure that holds a predefined reordering scheme, including the following
 * information:
 * <ul>
 * <li>input base direction,</li>
 * <li>input order,</li>
 * <li>output base direction,</li>
 * <li>output order,</li>
 * <li>a digit option that should be applied when the shaping engine is
 *     invoked (which can not always be derived from the caller-defined
 *     options),</li>
 * <li>a letter option that should be applied when the shaping engine is
 *     invoked (which can not always be derived from the caller-defined
 *     options),</li>
 * <li>base direction that should be applied when the reordering engine is
 *     invoked (which can not always be derived from the caller-defined
 *     options),</li>
 * <li>array of pointers to functions that perform actions aimed at
 *     accomplishing the required bidi layout transformation.</li>
 * </ul>
 */
typedef struct {
    UBiDiLevel       inLevel;               /* input level */
    UBiDiOrder       inOrder;               /* input order */
    UBiDiLevel       outLevel;              /* output level */
    UBiDiOrder       outOrder;              /* output order */
    uint32_t         digitsDir;             /* digit shaping direction */
    uint32_t         lettersDir;            /* letter shaping direction */
    UBiDiLevel       baseLevel;             /* paragraph level to be used with setPara */
    const bidiAction actions[MAX_ACTIONS];  /* array of pointers to functions carrying out the transformation */
} ReorderingScheme;

struct UBiDiTransform {
    UBiDi                   *pBidi;             /* pointer to a UBiDi object */
    const ReorderingScheme  *pActiveScheme;     /* active reordering scheme */
    uint32_t                reorderingOptions;  /* reordering options - currently only suppot DO_MIRRORING */
    uint32_t                digits;             /* digit option for ArabicShaping */
    uint32_t                letters;            /* letter option for ArabicShaping */
    UChar                   *src;               /* input text */
    UChar                   *dest;              /* output text */
    uint32_t                srcLength;          /* input text length - not really needed as we are zero-terminated and can u_strlen */
    uint32_t                srcSize;            /* input text capacity excluding the trailing zero */
    uint32_t                destSize;           /* output text capacity in UChars */
    uint32_t                *pDestLength;       /* number of UChars written to dest */
    UErrorCode              *pErrorCode;        /* pointer to an error code value */
};

U_DRAFT UBiDiTransform* U_EXPORT2
ubiditransform_open(UErrorCode *pErrorCode)
{
    UBiDiTransform *pBiDiTransform = NULL;
    if (U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    pBiDiTransform = (UBiDiTransform*) uprv_malloc(sizeof(UBiDiTransform));
    if (pBiDiTransform == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(pBiDiTransform, 0, sizeof(UBiDiTransform));
    return pBiDiTransform;
}

U_DRAFT void U_EXPORT2
ubiditransform_close(UBiDiTransform *pBiDiTransform)
{
    if (pBiDiTransform != NULL) {
        if (pBiDiTransform->pBidi != NULL) {
            ubidi_close(pBiDiTransform->pBidi);
        }
        if (pBiDiTransform->src != NULL) {
            uprv_free(pBiDiTransform->src);
        }
        /* Fill up with zeros to be on the safe side */
        uprv_memset(pBiDiTransform, 0, sizeof(UBiDiTransform));
        uprv_free(pBiDiTransform);
    }
}

/**
 * Performs Bidi resolution of text.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
resolve(UBiDiTransform *pTransform)
{
    ubidi_setPara(pTransform->pBidi, pTransform->src, pTransform->srcLength,
            pTransform->pActiveScheme->baseLevel, NULL, pTransform->pErrorCode);
    return FALSE;
}

/**
 * Performs basic reordering of text (Logical -> Visual LTR).
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
reorder(UBiDiTransform *pTransform)
{
    ubidi_writeReordered(pTransform->pBidi, pTransform->dest, pTransform->destSize,
            pTransform->reorderingOptions, pTransform->pErrorCode);

    *pTransform->pDestLength = pTransform->srcLength;
    pTransform->reorderingOptions = UBIDI_REORDER_DEFAULT;
    return TRUE;
}

/**
 * Sets "inverse" mode on the <code>UBiDi</code> object.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
setInverse(UBiDiTransform *pTransform)
{
    ubidi_setInverse(pTransform->pBidi, TRUE);
    ubidi_setReorderingMode(pTransform->pBidi, UBIDI_REORDER_INVERSE_LIKE_DIRECT);
    return FALSE;
}

/**
 * Sets "runs only" reordering mode indicating a Logical LTR <-> Logical RTL
 * transformation.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
setRunsOnly(UBiDiTransform *pTransform)
{
    ubidi_setReorderingMode(pTransform->pBidi, UBIDI_REORDER_RUNS_ONLY);
    return FALSE;
}

/**
 * Performs string reverse.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
reverse(UBiDiTransform *pTransform)
{
    ubidi_writeReverse(pTransform->src, pTransform->srcLength,
            pTransform->dest, pTransform->destSize,
            UBIDI_REORDER_DEFAULT,
            pTransform->pErrorCode);
    *pTransform->pDestLength = pTransform->srcLength;
    return TRUE;
}

/**
 * Calls a lower level shaping function.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 * @param options Shaping options.
 */
static void
doShape(UBiDiTransform *pTransform, uint32_t options)
{
    *pTransform->pDestLength = u_shapeArabic(pTransform->src,
            pTransform->srcLength, pTransform->dest, pTransform->destSize,
            options, pTransform->pErrorCode);
}

/**
 * Applies a new value to the text that serves as input at the current
 * processing step. This value is identical to the original one when we begin
 * the processing, but usually changes as the transformation progresses.
 * 
 * @param pTransform A pointer to the <code>UBiDiTransform</code> structure.
 * @param newSrc A pointer to the new text to be used as input.
 * @param newLength A length of the new text in <code>UChar</code>s.
 * @param newSize A new source capacity in <code>UChar</code>s.
 */
static void
applyNewSrc(UBiDiTransform *pTransform, const UChar *newSrc, uint32_t newLength,
        uint32_t newSize)
{
    if (newSize > pTransform->srcSize) {
        if (pTransform->src != NULL) {
            uprv_free(pTransform->src);
            pTransform->src = NULL;
        }
        pTransform->src = (UChar *)uprv_malloc((newSize + 1) * sizeof(UChar));
        if (pTransform->src == NULL) {
            *pTransform->pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            pTransform->srcLength = pTransform->srcSize = 0;
            return;
        }
        uprv_memset(pTransform->src, 0, (newSize + 1) * sizeof(UChar));
        pTransform->srcSize = newSize;
    }
    u_strncpy(pTransform->src, newSrc, newLength);
    //*(pTransform->src + newLength) = 0;
    pTransform->srcLength = u_terminateUChars(pTransform->src,
    		pTransform->srcSize, newLength, pTransform->pErrorCode);
}

/**
 * Performs numeric and literal shaping.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
shapeArabic(UBiDiTransform *pTransform)
{
    uint32_t letters = pTransform->letters;
    uint32_t digits = pTransform->digits;
    if ((letters | digits) == 0) {
        return FALSE;
    }
    letters |= pTransform->pActiveScheme->lettersDir;
    digits |= pTransform->pActiveScheme->digitsDir;
    if (((digits ^ letters) & U_SHAPE_TEXT_DIRECTION_MASK) == 0) {
        doShape(pTransform, letters | digits);
    } else {
        doShape(pTransform, digits);
        if (U_SUCCESS(*pTransform->pErrorCode)) {
            applyNewSrc(pTransform, pTransform->dest, *pTransform->pDestLength,
                    *pTransform->pDestLength);
            doShape(pTransform, letters);
        }
    }
    return TRUE;
}

/**
 * Performs character mirroring.
 * 
 * @param pTransform Pointer to the <code>UBiDiTransform</code> structure.
 *
 * @return Whether or not this function modifies the text. Besides the return
 * value, the caller should also check
 * <code>U_SUCCESS(*pTransform->pErrorCode)</code>.
 */
static UBool
mirror(UBiDiTransform *pTransform)
{
    UChar32 c;
    uint32_t i = 0, j = 0;
    if (0 == (pTransform->reorderingOptions & UBIDI_DO_MIRRORING)) {
        return FALSE;
    }
    if (pTransform->destSize < pTransform->srcLength) {
        *pTransform->pErrorCode = U_BUFFER_OVERFLOW_ERROR;
        return FALSE;
    }
    do {
        UBool isOdd = ubidi_getLevelAt(pTransform->pBidi, i) & 1;
        U16_NEXT(pTransform->src, i, pTransform->srcLength, c); 
        U16_APPEND_UNSAFE(pTransform->dest, j, isOdd ? u_charMirror(c) : c);
    } while (i < pTransform->srcLength);
    
    *pTransform->pDestLength = pTransform->srcLength;
    pTransform->reorderingOptions = UBIDI_REORDER_DEFAULT;
    return TRUE;
}

/**
 * All possible reordering schemes.
 *
 */
static const ReorderingScheme Schemes[] =
{
    /* 0: Logical LTR => Visual LTR */
    {LTR, LOGICAL, LTR, VISUAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {shapeArabic, resolve, reorder, NULL}},
    /* 1: Logical RTL => Visual LTR */
    {RTL, LOGICAL, LTR, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, RTL,
            {resolve, reorder, shapeArabic, NULL}},
    /* 2: Logical LTR => Visual RTL */
    {LTR, LOGICAL, RTL, VISUAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {shapeArabic, resolve, reorder, reverse, NULL}},
    /* 3: Logical RTL => Visual RTL */
    {RTL, LOGICAL, RTL, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, RTL,
            {resolve, reorder, shapeArabic, reverse, NULL}},
    /* 4: Visual LTR => Logical RTL */
    {LTR, VISUAL, RTL, LOGICAL, SHAPE_LOGICAL, SHAPE_VISUAL, RTL,
            {shapeArabic, setInverse, resolve, reorder, NULL}},
    /* 5: Visual RTL => Logical RTL */
    {RTL, VISUAL, RTL, LOGICAL, SHAPE_LOGICAL, SHAPE_VISUAL, RTL,
            {reverse, shapeArabic, setInverse, resolve, reorder, NULL}},
    /* 6: Visual LTR => Logical LTR */
    {LTR, VISUAL, LTR, LOGICAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {setInverse, resolve, reorder, shapeArabic, NULL}},
    /* 7: Visual RTL => Logical LTR */
    {RTL, VISUAL, LTR, LOGICAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {reverse, setInverse, resolve, reorder, shapeArabic, NULL}},
    /* 8: Logical LTR => Logical RTL */
    {LTR, LOGICAL, RTL, LOGICAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {shapeArabic, resolve, mirror, setRunsOnly, resolve, reorder, NULL}},
    /* 9: Logical RTL => Logical LTR */
    {RTL, LOGICAL, LTR, LOGICAL, SHAPE_LOGICAL, SHAPE_LOGICAL, RTL,
            {resolve, mirror, setRunsOnly, resolve, reorder, shapeArabic, NULL}},
    /* 10: Visual LTR => Visual RTL */
    {LTR, VISUAL, RTL, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, LTR,
            {shapeArabic, setInverse, resolve, mirror, reverse, NULL}},
    /* 11: Visual RTL => Visual LTR */
    {RTL, VISUAL, LTR, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, LTR,
            {reverse, shapeArabic, setInverse, resolve, mirror, NULL}},
    /* 12: Logical LTR => Logical LTR */
    {LTR, LOGICAL, LTR, LOGICAL, SHAPE_LOGICAL, SHAPE_LOGICAL, LTR,
            {resolve, mirror, shapeArabic, NULL}},
    /* 13: Logical RTL => Logical RTL */
    {RTL, LOGICAL, RTL, LOGICAL, SHAPE_VISUAL, SHAPE_LOGICAL, RTL,
            {resolve, mirror, shapeArabic, NULL}},
    /* 14: Visual LTR => Visual LTR */
    {LTR, VISUAL, LTR, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, LTR,
            {resolve, mirror, shapeArabic, NULL}},
    /* 15: Visual RTL => Visual RTL */
    {RTL, VISUAL, RTL, VISUAL, SHAPE_LOGICAL, SHAPE_VISUAL, LTR,
            {reverse, resolve, mirror, shapeArabic, reverse, NULL}}
};

static const int32_t nSchemes = sizeof(Schemes) / sizeof(*Schemes);

/**
 * When the direction option is <code>UBIDI_DEFAULT_LTR</code> or
 * <code>UBIDI_DEFAULT_RTL</code>, resolve the base direction according to that
 * of the first strong bidi character.
 */
static void
resolveBaseDirection(const UChar *text, uint32_t length,
        UBiDiLevel *pInLevel, UBiDiLevel *pOutLevel)
{
    UBiDiLevel level = LTR;
    switch (*pInLevel) {
        case UBIDI_DEFAULT_RTL:
            level = RTL;
            // fall through
        case UBIDI_DEFAULT_LTR:
            *pInLevel = ubidi_getBaseDirection(text, length);
            if (*pInLevel == UBIDI_NEUTRAL) {
                *pInLevel = level;
            }
            break;
        default:
            *pInLevel &= 1;
            break;
    }
    switch (*pOutLevel) {
        case UBIDI_DEFAULT_LTR:
        case UBIDI_DEFAULT_RTL:
            *pOutLevel = *pInLevel;
            break;
        default:
            *pOutLevel &= 1;
            break;
    }
}

/**
 * Finds a valid <code>ReorderingScheme</code> matching the
 * caller-defined scheme.
 * 
 * @return A valid <code>ReorderingScheme</code> object or NULL
 */
static const ReorderingScheme*
findMatchingScheme(UBiDiLevel inLevel, UBiDiLevel outLevel,
        UBiDiOrder inOrder, UBiDiOrder outOrder)
{
    int i;
    for (i = 0; i < nSchemes; i++) {
        const ReorderingScheme *pScheme = Schemes + i;
        if (inLevel == pScheme->inLevel && outLevel == pScheme->outLevel
            && inOrder == pScheme->inOrder && outOrder == pScheme->outOrder) {
            return pScheme;
        }
    }
    return NULL;
}

U_DRAFT uint32_t U_EXPORT2
ubiditransform_transform(UBiDiTransform *pBiDiTransform,
            const UChar *src, int32_t srcLength,
            UChar *dest, int32_t destSize,
            UBiDiLevel inParaLevel, UBiDiOrder inOrder,
            UBiDiLevel outParaLevel, UBiDiOrder outOrder,
            UBiDiMirroring doMirroring, uint32_t shapingOptions,
            UErrorCode *pErrorCode)
{
    uint32_t destLength = 0;
    UBool textChanged = FALSE;
    const UBiDiTransform *pOrigTransform = pBiDiTransform;
    const bidiAction *action = NULL;
#if 0
    /* Guidelines say "we do not want to test for pErrorCode==NULL" */
    if (pErrorCode == NULL) {
        return 0;
    }
#endif
    if (U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if (src == NULL || dest == NULL) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    CHECK_LEN(src, srcLength, pErrorCode);
    CHECK_LEN(dest, destSize, pErrorCode);

    if (pBiDiTransform == NULL) {
        pBiDiTransform = ubiditransform_open(pErrorCode);
        if (U_FAILURE(*pErrorCode)) {
            return 0;
        }
    }
    /* Current limitation: in multiple paragraphs will be resolved according
       to the 1st paragraph */
    resolveBaseDirection(src, srcLength, &inParaLevel, &outParaLevel);

    pBiDiTransform->pActiveScheme = findMatchingScheme(inParaLevel, outParaLevel,
            inOrder, outOrder);
    if (pBiDiTransform->pActiveScheme == NULL) {
        goto cleanup;
    }
    pBiDiTransform->reorderingOptions = doMirroring ? UBIDI_DO_MIRRORING
            : UBIDI_REORDER_DEFAULT;
    pBiDiTransform->pErrorCode = pErrorCode;

    shapingOptions &= ~U_SHAPE_TEXT_DIRECTION_MASK;
    pBiDiTransform->digits = shapingOptions & ~U_SHAPE_LETTERS_MASK;
    pBiDiTransform->letters = shapingOptions & ~U_SHAPE_DIGITS_MASK;

    applyNewSrc(pBiDiTransform, src, srcLength, destSize > srcLength ? destSize : srcLength);
    if (U_FAILURE(*pBiDiTransform->pErrorCode)) {
        goto cleanup;
    }
    if (pBiDiTransform->pBidi == NULL) {
        pBiDiTransform->pBidi = ubidi_open();
    }
    pBiDiTransform->dest = dest;
    pBiDiTransform->destSize = destSize;
    pBiDiTransform->pDestLength = &destLength;

    /* Guidelines say "It is not necessary to check for U_FAILURE() immediately before calling
       a function that takes a UErrorCode parameter, because that function is supposed to check
       for failure" - but here we don't want to continue the whole loop on failure. */
    for (action = pBiDiTransform->pActiveScheme->actions; action && *action && U_SUCCESS(*pErrorCode); action++) {
        if ((*action)(pBiDiTransform)) {
            applyNewSrc(pBiDiTransform, pBiDiTransform->dest, *pBiDiTransform->pDestLength,
                    *pBiDiTransform->pDestLength);
            textChanged = TRUE;
        }
    }
    if (!textChanged && U_SUCCESS(*pErrorCode)) {
        /* Text was not changed - just copy src to dest */
        if (destSize < srcLength) {
            *pErrorCode = U_BUFFER_OVERFLOW_ERROR;
        } else {
            u_strncpy(dest, src, srcLength);
            destLength = srcLength;
        }
    }
cleanup:
    ubidi_setInverse(pBiDiTransform->pBidi, FALSE);
    if (pOrigTransform != pBiDiTransform) {
        ubiditransform_close(pBiDiTransform);
    }
    return U_FAILURE(*pErrorCode) ? 0 : destLength;
}
