/*
**********************************************************************
* Copyright (c) 2003, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: March 19 2003
* Since: ICU 2.6
**********************************************************************
*/
#ifndef UCAT_H
#define UCAT_H

#include "unicode/utypes.h"
#include "unicode/ures.h"

U_CDECL_BEGIN

/**
 * An ICU message catalog descriptor, analogous to nl_catd.
 * 
 * @draft ICU 2.6
 */
typedef UResourceBundle* u_nl_catd;

/**
 * Open and return an ICU message catalog descriptor. The descriptor
 * may be passed to u_catgets() to retrieve localized strings.
 *
 * @param name string containing the full path pointing to the
 * directory where the resources reside followed by the package name
 * e.g. "/usr/resource/my_app/resources/guimessages" on a Unix system.
 * If NULL, ICU default data files will be used.
 *
 * @param locale the locale for which we want to open the resource. If
 * NULL, the default locale will be used. If strlen(locale) == 0, the
 * root locale will be used.
 *
 * @param ec input/output error code. Upon output,
 * U_USING_FALLBACK_WARNING indicates that a fallback locale was
 * used. For example, 'de_CH' was requested, but nothing was found
 * there, so 'de' was used. U_USING_DEFAULT_WARNING indicates that the
 * default locale data or root locale data was used; neither the
 * requested locale nor any of its fallback locales were found.
 *
 * @return a message catalog descriptor that may be passed to
 * u_catgets(). If the ec parameter indicates success, then the caller
 * is responsible for calling u_catclose() to close the message
 * catalog. If the ec parameter indicates failure, then NULL will be
 * returned.
 * 
 * @draft ICU 2.6
 */
U_CAPI u_nl_catd U_EXPORT2
u_catopen(const char* name, const char* locale, UErrorCode* ec);

/**
 * Close an ICU message catalog, given its descriptor.
 *
 * @param catd a message catalog descriptor to be closed. May be NULL,
 * in which case no action is taken.
 * 
 * @draft ICU 2.6
 */
U_CAPI void U_EXPORT2
u_catclose(u_nl_catd catd);

/**
 * Retrieve a localized string from an ICU message catalog.
 *
 * @param catd a message catalog descriptor returned by u_catopen.
 *
 * @param set_num the message catalog set number. Sets need not be
 * numbered consecutively.
 *
 * @param msg_num the message catalog message number within the
 * set. Messages need not be numbered consecutively.
 *
 * @param s the default string. This is returned if the string
 * specified by the set_num and msg_num is not found. It must be
 * zero-terminated.
 *
 * @param len fill-in parameter to receive the length of the result.
 * May be NULL, in which case it is ignored.
 *
 * @param ec input/output error code. May be U_USING_FALLBACK_WARNING
 * or U_USING_DEFAULT_WARNING. U_MISSING_RESOURCE_ERROR indicates that
 * the set_num/msg_num tuple does not specify a valid message string
 * in this catalog.
 *
 * @return a pointer to a zero-terminated UChar array which lives in
 * an internal buffer area, typically a memory mapped/DLL file. The
 * caller must NOT delete this pointer. If the call is unsuccessful
 * for any reason, then s is returned.
 * 
 * @draft ICU 2.6
 */
U_CAPI const UChar* U_EXPORT2
u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num,
          const UChar* s,
          int32_t* len, UErrorCode* ec);

U_CDECL_END

#endif /*UCAT_H*/
/*eof*/
