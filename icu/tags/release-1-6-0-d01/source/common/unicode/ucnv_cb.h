/*
**********************************************************************
*   Copyright (C) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
 *  ucnv_cb.h:
 *  External APIs for the ICU's codeset conversion library
 *  Helena Shih
 * 
 * Modification History:
 *
 *   Date        Name        Description
 */

/**
 * @name Character Conversion C API
 *
 */

#ifndef UCNV_CB_H
#define UCNV_CB_H

#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"

/* From Unicode */
/**
 * Used by the callback functions.  This function will write out the error
 * character(s) to the target byte buffer.
 *
 * @param args : callback fromUnicode arguments
 * @param target : output target buffer of the callback result.
 * @param length : the length of target buffer
 * @param err error status <TT>U_BUFFER_OVERFLOW</TT>
 * @see ucnv_cbFromUWriteSub
 * @draft
 */

U_CAPI void U_EXPORT2 ucnv_cbFromUWriteBytes (UConverterFromUnicodeArgs *args,
                       const char* target,
                       int32_t length,
                       UErrorCode * err);

/**
 * Used by the callback functions.  This function will write out the error
 * character(s) to the substitution character.
 *
 * @param args : callback fromUnicode arguments
 * @param target : output target buffer of the callback result.
 * @param length : the length of target buffer
 * @param err error status <TT>U_BUFFER_OVERFLOW</TT>
 * @see ucnv_cbFromUWriteBytes
 * @draft
 */

U_CAPI void U_EXPORT2 ucnv_cbFromUWriteSub (UConverterFromUnicodeArgs *args,
                       UErrorCode * err);
/**
 * Used by the callback functions.  This function will write out the error
 * character(s) to the target UChar buffer.
 *
 * @param args : callback toUnicode arguments
 * @param target : output target buffer of the callback result.
 * @param length : the length of target buffer
 * @param err error status <TT>U_BUFFER_OVERFLOW</TT>
 * @see ucnv_cbToUWriteSub
 * @draft
 */

U_CAPI void U_EXPORT2 ucnv_cbToUWriteUChars (UConverterToUnicodeArgs *args,
                       UChar* target,
                       int32_t length,
                       UErrorCode * err);
/**
 * Used by the callback functions.  This function will write out the error
 * character(s) to the substitution character.
 *
 * @param args : callback fromUnicode arguments
 * @param err error status <TT>U_BUFFER_OVERFLOW</TT>
 * @see ucnv_cbToUWriteUChars
 * @draft
 */

U_CAPI void U_EXPORT2 ucnv_cbToUWriteSub (UConverterToUnicodeArgs *args,
                       UErrorCode * err);
#endif

