/************************************************************************
 * COPYRIGHT:
 * Copyright (c) 2015, International Business Machines Corporation
 * and others. All Rights Reserved.
 ************************************************************************/

#ifndef _DATADRIVENNUMBERFORMATTESTSUITE_H__
#define _DATADRIVENNUMBERFORMATTESTSUITE_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "numberformattesttuple.h"

struct UCHARBUF;
class IntlTest;

U_NAMESPACE_BEGIN

/**
 * Performs various in-depth test on NumberFormat
 **/
class DataDrivenNumberFormatTestSuite : public UObject {

 public:
     DataDrivenNumberFormatTestSuite() {
         for (int32_t i = 0; i < UPRV_LENGTHOF(fPreviousFormatters); ++i) {
             fPreviousFormatters[i] = NULL;
         }
     }
     void run(const char *fileName, UBool runAllTests);
     virtual ~DataDrivenNumberFormatTestSuite();
 protected:
    virtual UBool isFormatPass(
            const NumberFormatTestTuple &tuple,
            UnicodeString &appendErrorMessage,
            UErrorCode &status);
    virtual UBool isFormatPass(
            const NumberFormatTestTuple &tuple,
            UObject *somePreviousFormatter,
            UnicodeString &appendErrorMessage,
            UErrorCode &status);
    virtual UObject *newFormatter(UErrorCode &status);
 private:
    UnicodeString fFileLine;
    int32_t fFileLineNumber;
    UnicodeString fFileTestName;
    NumberFormatTestTuple fTuple;
    int32_t fFormatTestNumber;
    UObject *fPreviousFormatters[13];

    void setTupleField(UErrorCode &);
    int32_t splitBy(
            UnicodeString *columnValues,
            int32_t columnValueCount,
            UChar delimiter);
    void showError(const char *message);
    void showFailure(const UnicodeString &message);
    void showLineInfo();
    UBool breaksC();
    UBool readLine(UCHARBUF *f, UErrorCode &);
    UBool isPass(
            const NumberFormatTestTuple &tuple,
            UnicodeString &appendErrorMessage,
            UErrorCode &status);
};

U_NAMESPACE_END

#endif // _DATADRIVENNUMBERFORMATTESTSUITE_
