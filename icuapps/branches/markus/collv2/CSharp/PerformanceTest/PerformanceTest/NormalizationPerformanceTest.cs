/*
*******************************************************************************
*
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/
﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using PerformanceTest;
using icu;

namespace NormPerfTest
{
    class NormalizationPerformanceTest : PerformanceTestFramework
    {
        public enum UErrorCode
        {
            U_USING_FALLBACK_WARNING = -128,
            U_ERROR_WARNING_START = -128,
            U_USING_DEFAULT_WARNING = -127,
            U_SAFECLONE_ALLOCATED_WARNING = -126,
            U_STATE_OLD_WARNING = -125,
            U_STRING_NOT_TERMINATED_WARNING = -124,
            U_SORT_KEY_TOO_SHORT_WARNING = -123,
            U_AMBIGUOUS_ALIAS_WARNING = -122,
            U_DIFFERENT_UCA_VERSION = -121,
            U_ERROR_WARNING_LIMIT,
            U_ZERO_ERROR = 0,
            U_ILLEGAL_ARGUMENT_ERROR = 1,
            U_MISSING_RESOURCE_ERROR = 2,
            U_INVALID_FORMAT_ERROR = 3,
            U_FILE_ACCESS_ERROR = 4,
            U_INTERNAL_PROGRAM_ERROR = 5,
            U_MESSAGE_PARSE_ERROR = 6,
            U_MEMORY_ALLOCATION_ERROR = 7,
            U_INDEX_OUTOFBOUNDS_ERROR = 8,
            U_PARSE_ERROR = 9,
            U_INVALID_CHAR_FOUND = 10,
            U_TRUNCATED_CHAR_FOUND = 11,
            U_ILLEGAL_CHAR_FOUND = 12,
            U_INVALID_TABLE_FORMAT = 13,
            U_INVALID_TABLE_FILE = 14,
            U_BUFFER_OVERFLOW_ERROR = 15,
            U_UNSUPPORTED_ERROR = 16,
            U_RESOURCE_TYPE_MISMATCH = 17,
            U_ILLEGAL_ESCAPE_SEQUENCE = 18,
            U_UNSUPPORTED_ESCAPE_SEQUENCE = 19,
            U_NO_SPACE_AVAILABLE = 20,
            U_CE_NOT_FOUND_ERROR = 21,
            U_PRIMARY_TOO_LONG_ERROR = 22,
            U_STATE_TOO_OLD_ERROR = 23,
            U_TOO_MANY_ALIASES_ERROR = 24,
            U_ENUM_OUT_OF_SYNC_ERROR = 25,
            U_INVARIANT_CONVERSION_ERROR = 26,
            U_INVALID_STATE_ERROR = 27,
            U_COLLATOR_VERSION_MISMATCH = 28,
            U_USELESS_COLLATOR_ERROR = 29,
            U_NO_WRITE_PERMISSION = 30,
            U_STANDARD_ERROR_LIMIT,
            U_BAD_VARIABLE_DEFINITION = 0x10000,
            U_PARSE_ERROR_START = 0x10000,
            U_MALFORMED_RULE,
            U_MALFORMED_SET,
            U_MALFORMED_SYMBOL_REFERENCE,
            U_MALFORMED_UNICODE_ESCAPE,
            U_MALFORMED_VARIABLE_DEFINITION,
            U_MALFORMED_VARIABLE_REFERENCE,
            U_MISMATCHED_SEGMENT_DELIMITERS,
            U_MISPLACED_ANCHOR_START,
            U_MISPLACED_CURSOR_OFFSET,
            U_MISPLACED_QUANTIFIER,
            U_MISSING_OPERATOR,
            U_MISSING_SEGMENT_CLOSE,
            U_MULTIPLE_ANTE_CONTEXTS,
            U_MULTIPLE_CURSORS,
            U_MULTIPLE_POST_CONTEXTS,
            U_TRAILING_BACKSLASH,
            U_UNDEFINED_SEGMENT_REFERENCE,
            U_UNDEFINED_VARIABLE,
            U_UNQUOTED_SPECIAL,
            U_UNTERMINATED_QUOTE,
            U_RULE_MASK_ERROR,
            U_MISPLACED_COMPOUND_FILTER,
            U_MULTIPLE_COMPOUND_FILTERS,
            U_INVALID_RBT_SYNTAX,
            U_INVALID_PROPERTY_PATTERN,
            U_MALFORMED_PRAGMA,
            U_UNCLOSED_SEGMENT,
            U_ILLEGAL_CHAR_IN_SEGMENT,
            U_VARIABLE_RANGE_EXHAUSTED,
            U_VARIABLE_RANGE_OVERLAP,
            U_ILLEGAL_CHARACTER,
            U_INTERNAL_TRANSLITERATOR_ERROR,
            U_INVALID_ID,
            U_INVALID_FUNCTION,
            U_PARSE_ERROR_LIMIT,
            U_UNEXPECTED_TOKEN = 0x10100,
            U_FMT_PARSE_ERROR_START = 0x10100,
            U_MULTIPLE_DECIMAL_SEPARATORS,
            U_MULTIPLE_DECIMAL_SEPERATORS = U_MULTIPLE_DECIMAL_SEPARATORS,
            U_MULTIPLE_EXPONENTIAL_SYMBOLS,
            U_MALFORMED_EXPONENTIAL_PATTERN,
            U_MULTIPLE_PERCENT_SYMBOLS,
            U_MULTIPLE_PERMILL_SYMBOLS,
            U_MULTIPLE_PAD_SPECIFIERS,
            U_PATTERN_SYNTAX_ERROR,
            U_ILLEGAL_PAD_POSITION,
            U_UNMATCHED_BRACES,
            U_UNSUPPORTED_PROPERTY,
            U_UNSUPPORTED_ATTRIBUTE,
            U_FMT_PARSE_ERROR_LIMIT,
            U_BRK_ERROR_START = 0x10200,
            U_BRK_INTERNAL_ERROR,
            U_BRK_HEX_DIGITS_EXPECTED,
            U_BRK_SEMICOLON_EXPECTED,
            U_BRK_RULE_SYNTAX,
            U_BRK_UNCLOSED_SET,
            U_BRK_ASSIGN_ERROR,
            U_BRK_VARIABLE_REDFINITION,
            U_BRK_MISMATCHED_PAREN,
            U_BRK_NEW_LINE_IN_QUOTED_STRING,
            U_BRK_UNDEFINED_VARIABLE,
            U_BRK_INIT_ERROR,
            U_BRK_RULE_EMPTY_SET,
            U_BRK_UNRECOGNIZED_OPTION,
            U_BRK_MALFORMED_RULE_TAG,
            U_BRK_ERROR_LIMIT,
            U_REGEX_ERROR_START = 0x10300,
            U_REGEX_INTERNAL_ERROR,
            U_REGEX_RULE_SYNTAX,
            U_REGEX_INVALID_STATE,
            U_REGEX_BAD_ESCAPE_SEQUENCE,
            U_REGEX_PROPERTY_SYNTAX,
            U_REGEX_UNIMPLEMENTED,
            U_REGEX_MISMATCHED_PAREN,
            U_REGEX_NUMBER_TOO_BIG,
            U_REGEX_BAD_INTERVAL,
            U_REGEX_MAX_LT_MIN,
            U_REGEX_INVALID_BACK_REF,
            U_REGEX_INVALID_FLAG,
            U_REGEX_LOOK_BEHIND_LIMIT,
            U_REGEX_SET_CONTAINS_STRING,
            U_REGEX_ERROR_LIMIT,
            U_IDNA_ERROR_START = 0x10400,
            U_IDNA_PROHIBITED_ERROR,
            U_IDNA_UNASSIGNED_ERROR,
            U_IDNA_CHECK_BIDI_ERROR,
            U_IDNA_STD3_ASCII_RULES_ERROR,
            U_IDNA_ACE_PREFIX_ERROR,
            U_IDNA_VERIFICATION_ERROR,
            U_IDNA_LABEL_TOO_LONG_ERROR,
            U_IDNA_ZERO_LENGTH_LABEL_ERROR,
            U_IDNA_ERROR_LIMIT,
            U_STRINGPREP_PROHIBITED_ERROR = U_IDNA_PROHIBITED_ERROR,
            U_STRINGPREP_UNASSIGNED_ERROR = U_IDNA_UNASSIGNED_ERROR,
            U_STRINGPREP_CHECK_BIDI_ERROR = U_IDNA_CHECK_BIDI_ERROR,
            U_ERROR_LIMIT = U_IDNA_ERROR_LIMIT
        };

        public enum UNormalizationMode
        {
          UNORM_NONE = 1, 
          UNORM_NFD = 2,
          UNORM_NFKD = 3,
          UNORM_NFC = 4,
          UNORM_DEFAULT = UNORM_NFC, 
          UNORM_NFKC =5,
          UNORM_FCD = 6,
          UNORM_MODE_COUNT
        };

        public static UNormalizationMode mode;
        public static NormalizationForm wmode;

        public static UErrorCode status = (UErrorCode)0;

        protected override void loadTestFunction()
        {
            if (testName == "TestWin_NFC")
            {
                myTestFunction = new TestFunction(TestWin);
                wmode = NormalizationForm.FormC;
            }
            else if (testName == "TestWin_NFD")
            {
                myTestFunction = new TestFunction(TestWin);
                wmode = NormalizationForm.FormD;
            }
            else if (testName == "TestWin_NFKC")
            {
                myTestFunction = new TestFunction(TestWin);
                wmode = NormalizationForm.FormKC;
            }
            else if (testName == "TestWin_NFKD")
            {
                myTestFunction = new TestFunction(TestWin);
                wmode = NormalizationForm.FormKD;
            }
            else if (testName == "TestICU_NFC")
            {
                myTestFunction = new TestFunction(TestICU);
                mode = (UNormalizationMode)4;
            }
            else if (testName == "TestICU_NFD")
            {
                myTestFunction = new TestFunction(TestICU);
                mode = (UNormalizationMode)2;
            }
            else if (testName == "TestICU_NFKC")
            {
                myTestFunction = new TestFunction(TestICU);
                mode = (UNormalizationMode)5;
            }
            else if (testName == "TestICU_NFKD")
            {
                myTestFunction = new TestFunction(TestICU);
                mode = (UNormalizationMode)3;
            }
            else
            {
                Console.Out.WriteLine("Unknown test name: {0}", testName);
                Environment.Exit(2);
            }
        }

        // Windows Test Function
        private void TestWin(int lineNum)
        {
            wLines[lineNum].Normalize(wmode);
        }

        // ICU Test Function
        [DllImport("C:\\PerformanceTest\\icu\\bin\\icuuc45.dll", EntryPoint="unorm_normalize_45")]
        public unsafe static extern int unorm_normalize(
            char[] source,
            int sourceLength,
            UNormalizationMode mode,
            int options,
            char* result,
            int resultLength,
            ref UErrorCode status
        );
        private void TestICU(int lineNum)
        {
            unsafe
            {
                fixed (char* p = results)
                {
                    unorm_normalize(Lines[lineNum], Lines[lineNum].Length, mode, 0, p, resultsLength, ref status);
                }
            }
        }
    }
}
