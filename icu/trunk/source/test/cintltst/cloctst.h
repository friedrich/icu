/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-1999, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CLOCTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Converted to C
*********************************************************************************
*/
#ifndef _CLOCTEST
#define _CLOCTEST

#include "cintltst.h"
/*C API TEST FOR LOCALE */

static void addLocaleTests(TestNode**);

/**
Test functions to set and get data fields
 **/
static void TestBasicGetters(void);
/**
 * Use Locale to access Resource file data and compare against expected values
 **/
static void TestSimpleResourceInfo(void);
/**
 * Use Locale to access Resource file display names and compare against expected values
 **/
static  void TestDisplayNames(void);
/**
 * Test functions for basic object behaviour
 **/
static  void TestSimpleObjectStuff(void);
/**
 * Test getAvailableLocales
 **/
 static  void TestGetAvailableLocales(void);
/**
 * Test functions to set and access a custom data directory
 **/
 static void TestDataDirectory(void);
/**
 * Test functions to test get ISO countries and Languages
 **/
 static void TestISOFunctions(void);
/**
 * Test functions to test get ISO3 countries and Languages Fallback
 **/
 static void TestISO3Fallback(void);
/**
 * Test functions to test get ISO3 countries and Languages for Uninstalled locales
 **/
 static void TestUninstalledISO3Names(void);
/**
 * Test functions uloc_getDisplaynames()
 **/
 static void TestSimpleDisplayNames(void);
/**
 * Test functions uloc_getDisplaynames()
 **/
 static void TestVariantParsing(void);

 /*
 * routine to perform subtests, used by TestDisplayNames
 **/
 static void doTestDisplayNames(const char* inLocale, int32_t compareIndex, int32_t defaultIsFrench);
/**
 * additional intialization for datatables storing expected values
 **/
static void setUpDataTable(void);
void displayDataTable(void);

#endif
