/*
**********************************************************************
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*  FILE NAME : ustream.h
*
*   Modification History:
*
*   Date        Name        Description
*   06/25/2001  grhoten     Move iostream from unistr.h
******************************************************************************
*/
   
#ifndef USTREAM_H
#define USTREAM_H

#include "unicode/unistr.h"

// for unistrm.h
/**
 * Write the contents of a UnicodeString to an ostream. This functions writes
 * the characters in a UnicodeString to an ostream. The UChars in the
 * UnicodeString are truncated to char, leading to undefined results with
 * anything not in the Latin1 character set.
 *
 * @deprecated This will move to the unsupported ustdio library after 2002-feb-01
 */
#if U_IOSTREAM_SOURCE >= 199711
#include <iostream>
U_USTDIO_API std::ostream &operator<<(std::ostream& stream, const UnicodeString& s);
#elif U_IOSTREAM_SOURCE >= 198506
#include <iostream.h>
U_USTDIO_API ostream &operator<<(ostream& stream, const UnicodeString& s);
#endif

/* TODO: We should add the operator<< for UChar* and UDate. Also add the operator>>. */

#endif
