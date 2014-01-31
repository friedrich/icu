/**********************************************************************
*   Copyright (C) 2003-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
***********************************************************************/
#include "locexp.h"
#include <unicode/ustdio.h>
#include "unicode/unum.h"
#include <stdarg.h>
#include <stdio.h>

/*
Stubbed out - use cgiutil instead.
*/


/* helpers for CGI-like environments */
void initCGIVariables(LXContext* lx)
{
  lx->cgi = cgi_open();
  cgi_initCGIVariables(lx->cgi);
}

void initPOSTFromFILE(LXContext* lx, FILE *f)
{
  cgi_initPOSTFromFILE(lx->cgi, f);
}

void closeCGIVariables(LXContext* lx)
{
  cgi_close(lx->cgi);
}


void closePOSTFromFILE(LXContext* lx)
{
  cgi_closePOSTFromFILE(lx->cgi);
  /* free(lx->POSTdata); */
}

const char *fieldInQuery(LXContext* lx, const char *query, const char *field)
{
  return cgi_fieldInQuery(lx->cgi, query, field);
}

const char *fieldInCookie(LXContext* lx, const char *query, const char *field)
{
  return cgi_fieldInCookie(lx->cgi, query, field);
}

const char *copyField(LXContext* lx, const char *val)
{
  return cgi_copyField(lx->cgi, val);
}

const char *copyCookieField(LXContext* lx, const char *val)
{
  return cgi_copyCookieField(lx->cgi, val);
}


const char *queryField(LXContext* lx, const char *field)
{
  return cgi_queryField(lx->cgi, field);
}

UBool hasQueryField(LXContext* lx, const char *field)
{
  return cgi_hasQueryField(lx->cgi, field);
}

/* Cookie versions */
const char *cookieField(LXContext* lx, const char *field)
{
  return cgi_cookieField(lx->cgi, field);
}

UBool hasCookieField(LXContext* lx, const char *field)
{
  return cgi_hasCookieField(lx->cgi, field);
}

void appendHeader(LXContext* lx, const char *header, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  return cgi_vappendHeader(lx->cgi, header, fmt, ap);
}

double parseDoubleFromField(LXContext* lx, UNumberFormat* nf, const char *key, double defVal) {
  return cgi_parseDoubleFromField(lx->cgi, nf, key, defVal);
}

double parseDoubleFromString(LXContext* lx, UNumberFormat* nf, const char *str, double defVal) {
  return cgi_parseDoubleFromString(lx->cgi, nf, str, defVal);
}

