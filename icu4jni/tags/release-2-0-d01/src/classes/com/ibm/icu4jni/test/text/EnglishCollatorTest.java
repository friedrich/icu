/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: 
*  /usr/cvs/icu4j/icu4j/src/com/ibm/icu/test/text/EnglishCollatorTest.java,v $ 
* $Date: 2001/03/23 19:43:17 $ 
* $Revision: 1.5 $
*
*******************************************************************************
*/

package com.ibm.icu4jni.test.text;

import java.util.Locale;
import com.ibm.icu4jni.text.Collator;
import com.ibm.icu4jni.text.CollationAttribute;
import com.ibm.icu4jni.test.TestFmwk;

/**
* Testing class for english collator
* Mostly following the test cases for ICU
* @author Syn Wee Quek
* @since jan 23 2001
*/
public final class EnglishCollatorTest extends TestFmwk
{ 
  
  // constructor ===================================================
  
  /**
  * Constructor
  */
  public EnglishCollatorTest() throws Exception
  {
    m_collator_ = Collator.getInstance(Locale.ENGLISH);
  }
  
  // public methods ================================================

  /**
  * Test with primary collation strength
  * @exception thrown when error occurs while setting strength
  */
  public void TestPrimary() throws Exception
  {
    m_collator_.setStrength(CollationAttribute.VALUE_PRIMARY);
    for (int i = 38; i < 43 ; i ++) {
      CollatorTest.doTest(this, m_collator_, SOURCE_TEST_CASE_[i], 
                          TARGET_TEST_CASE_[i], EXPECTED_TEST_RESULT_[i]);
    }
  }

  /**
  * Test with secondary collation strength
  * @exception thrown when error occurs while setting strength
  */
  public void TestSecondary() throws Exception
  {
    m_collator_.setStrength(CollationAttribute.VALUE_SECONDARY);
    for (int i = 43; i < 49 ; i ++) {
      CollatorTest.doTest(this, m_collator_, SOURCE_TEST_CASE_[i], 
                          TARGET_TEST_CASE_[i], EXPECTED_TEST_RESULT_[i]);
    }

    //test acute and grave ordering (compare to french collation)
    int expected;
    int size = ACUTE_TEST_CASE_.length / (ACUTE_TEST_CASE_[0].length());
    for (int i = 0; i < size; i ++)
      for (int j = 0; j < size; j ++)
      {
        if (i <  j)
          expected = Collator.RESULT_LESS;
        else 
          if (i == j)
            expected = Collator.RESULT_EQUAL;
          else // (i >  j)
            expected = Collator.RESULT_GREATER;
        CollatorTest.doTest(this, m_collator_, ACUTE_TEST_CASE_[i], ACUTE_TEST_CASE_[j], expected);
      }
  }
  
  /**
  * Test with tertiary collation strength
  * @exception thrown when error occurs while setting strength
  */
  public void TestTertiary() throws Exception
  {
    m_collator_.setStrength(CollationAttribute.VALUE_TERTIARY);
    for (int i = 0; i < 38 ; i ++) {
      CollatorTest.doTest(this, m_collator_, SOURCE_TEST_CASE_[i], 
                          TARGET_TEST_CASE_[i], EXPECTED_TEST_RESULT_[i]);
    }

    for (int i = 0; i < 10; i ++) {
      for (int j = i + 1; j < 10; j ++) {
        CollatorTest.doTest(this, m_collator_, BUGS_TEST_CASE_[i], 
                            BUGS_TEST_CASE_[j], Collator.RESULT_LESS);
      }
    }
        
    //test more interesting cases
    int expected = Collator.RESULT_EQUAL;
    int size = MISCELLANEOUS_TEST_CASE_.length;
    for (int i = 0; i < size; i ++) {
      for (int j = 0; j < size; j ++)
      {
        if (i <  j) {
          expected = Collator.RESULT_LESS;
        }
        else {
          if (i == j) {
            expected = Collator.RESULT_EQUAL;
          }
          else { // (i >  j)
            expected = Collator.RESULT_GREATER;
          }
        }
     
        CollatorTest.doTest(this, m_collator_, MISCELLANEOUS_TEST_CASE_[i], 
                          MISCELLANEOUS_TEST_CASE_[j], expected);
      }
    }
  }
  
  // private variables =============================================
  
  /**
  * RuleBasedCollator for testing
  */
  private Collator m_collator_;
 
  /**
  * Source strings for testing
  */
  private static final String SOURCE_TEST_CASE_[] = 
  {
    "\u0061\u0062",
    "\u0062\u006C\u0061\u0063\u006B\u002D\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B\u0020\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B\u002D\u0062\u0069\u0072\u0064",
    "\u0048\u0065\u006C\u006C\u006F",
    "\u0041\u0042\u0043", 
    "\u0061\u0062\u0063",
    "\u0062\u006C\u0061\u0063\u006B\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B\u002D\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B\u002D\u0062\u0069\u0072\u0064",
    "\u0070\u00EA\u0063\u0068\u0065",                                            
    "\u0070\u00E9\u0063\u0068\u00E9",
    "\u00C4\u0042\u0308\u0043\u0308",
    "\u0061\u0308\u0062\u0063",
    "\u0070\u00E9\u0063\u0068\u0065\u0072",
    "\u0072\u006F\u006C\u0065\u0073",
    "\u0061\u0062\u0063",
    "\u0041",
    "\u0041",
    "\u0061\u0062",                                                                
    "\u0074\u0063\u006F\u006D\u0070\u0061\u0072\u0065\u0070\u006C\u0061\u0069\u006E",
    "\u0061\u0062", 
    "\u0061\u0023\u0062",
    "\u0061\u0023\u0062",
    "\u0061\u0062\u0063",
    "\u0041\u0062\u0063\u0064\u0061",
    "\u0061\u0062\u0063\u0064\u0061",
    "\u0061\u0062\u0063\u0064\u0061",
    "\u00E6\u0062\u0063\u0064\u0061",
    "\u00E4\u0062\u0063\u0064\u0061",                                            
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0063\u0048\u0063",
    "\u0061\u0308\u0062\u0063",
    "\u0074\u0068\u0069\u0302\u0073",
    "\u0070\u00EA\u0063\u0068\u0065",
    "\u0061\u0062\u0063",                                                         
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u00E6\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u00E6\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",               
    "\u0070\u00E9\u0063\u0068\u00E9"
  };

  /**
  * Target strings for testing
  */
  private final String TARGET_TEST_CASE_[] = 
  {
    "\u0061\u0062\u0063",
    "\u0062\u006C\u0061\u0063\u006B\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B\u002D\u0062\u0069\u0072\u0064",
    "\u0062\u006C\u0061\u0063\u006B",
    "\u0068\u0065\u006C\u006C\u006F",
    "\u0041\u0042\u0043",
    "\u0041\u0042\u0043",
    "\u0062\u006C\u0061\u0063\u006B\u0062\u0069\u0072\u0064\u0073",
    "\u0062\u006C\u0061\u0063\u006B\u0062\u0069\u0072\u0064\u0073",
    "\u0062\u006C\u0061\u0063\u006B\u0062\u0069\u0072\u0064",                             
    "\u0070\u00E9\u0063\u0068\u00E9",
    "\u0070\u00E9\u0063\u0068\u0065\u0072",
    "\u00C4\u0042\u0308\u0043\u0308",
    "\u0041\u0308\u0062\u0063",
    "\u0070\u00E9\u0063\u0068\u0065",
    "\u0072\u006F\u0302\u006C\u0065",
    "\u0041\u00E1\u0063\u0064",
    "\u0041\u00E1\u0063\u0064",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u0063",                                                             
    "\u0054\u0043\u006F\u006D\u0070\u0061\u0072\u0065\u0050\u006C\u0061\u0069\u006E",
    "\u0061\u0042\u0063",
    "\u0061\u0023\u0042",
    "\u0061\u0026\u0062",
    "\u0061\u0023\u0063",
    "\u0061\u0062\u0063\u0064\u0061",
    "\u00C4\u0062\u0063\u0064\u0061",
    "\u00E4\u0062\u0063\u0064\u0061",
    "\u00C4\u0062\u0063\u0064\u0061",
    "\u00C4\u0062\u0063\u0064\u0061",                                             
    "\u0061\u0062\u0023\u0063",
    "\u0061\u0062\u0063",
    "\u0061\u0062\u003D\u0063",
    "\u0061\u0062\u0064",
    "\u00E4\u0062\u0063",
    "\u0061\u0043\u0048\u0063",
    "\u00E4\u0062\u0063",
    "\u0074\u0068\u00EE\u0073",
    "\u0070\u00E9\u0063\u0068\u00E9",
    "\u0061\u0042\u0043",                                                          
    "\u0061\u0062\u0064",
    "\u00E4\u0062\u0063",
    "\u0061\u00C6\u0063",
    "\u0061\u0042\u0064",
    "\u00E4\u0062\u0063",
    "\u0061\u00C6\u0063",
    "\u0061\u0042\u0064",
    "\u00E4\u0062\u0063",          
    "\u0070\u00EA\u0063\u0068\u0065"
  };

  /**
  * Comparison result corresponding to above source and target cases
  */
  private final int EXPECTED_TEST_RESULT_[] = 
  {
    Collator.RESULT_LESS, 
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_GREATER,
    Collator.RESULT_GREATER,
    Collator.RESULT_EQUAL,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,                                                           /* 10 */
    Collator.RESULT_GREATER,
    Collator.RESULT_LESS,
    Collator.RESULT_EQUAL,
    Collator.RESULT_LESS,
    Collator.RESULT_GREATER,
    Collator.RESULT_GREATER,
    Collator.RESULT_GREATER,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,                                                             /* 20 */
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_GREATER,
    Collator.RESULT_GREATER,
    Collator.RESULT_GREATER,
    /* Test Tertiary  > 26 */
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_GREATER,
    Collator.RESULT_LESS,                                                             /* 30 */
    Collator.RESULT_GREATER,
    Collator.RESULT_EQUAL,
    Collator.RESULT_GREATER,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    /* test identical > 36 */
    Collator.RESULT_EQUAL,
    Collator.RESULT_EQUAL,
    /* test primary > 38 */
    Collator.RESULT_EQUAL,
    Collator.RESULT_EQUAL,                                                            /* 40 */
    Collator.RESULT_LESS,
    Collator.RESULT_EQUAL,
    Collator.RESULT_EQUAL,
    /* test secondary > 43 */
    Collator.RESULT_LESS,
    Collator.RESULT_LESS,
    Collator.RESULT_EQUAL,
    Collator.RESULT_LESS,
    Collator.RESULT_LESS, 
    Collator.RESULT_LESS                                                                 // 49
  };

  /**
  * Bug testing data set
  */
  private final String BUGS_TEST_CASE_[] = 
  {
    "\u0061",
    "\u0041",
    "\u0065",
    "\u0045",
    "\u00e9",
    "\u00e8",
    "\u00ea",
    "\u00eb",
    "\u0065\u0061",
    "\u0078"
  };

  /**
  * \u000300 is grave\u000301 is acute.
  * the order of elements in this array must be different than the order in 
  * CollationFrenchTest.
  * Data set for testing accents.
  */
  private final String ACUTE_TEST_CASE_[] = 
  {
    "\u0065\u0065",
    "\u0065\u0065\u0301",
    "\u0065\u0065\u0301\u0300",
    "\u0065\u0065\u0300",
    "\u0065\u0065\u0300\u0301",
    "\u0065\u0301\u0065",
    "\u0065\u0301\u0065\u0301",
    "\u0065\u0301\u0065\u0301\u0300",
    "\u0065\u0301\u0065\u0300",
    "\u0065\u0301\u0065\u0300\u0301",
    "\u0065\u0301\u0300\u0065",
    "\u0065\u0301\u0300\u0065\u0301",
    "\u0065\u0301\u0300\u0065\u0301\u0300",
    "\u0065\u0301\u0300\u0065\u0300",
    "\u0065\u0301\u0300\u0065\u0300\u0301",
    "\u0065\u0300\u0065",
    "\u0065\u0300\u0065\u0301",
    "\u0065\u0300\u0065\u0301\u0300",
    "\u0065\u0300\u0065\u0300",
    "\u0065\u0300\u0065\u0300\u0301",
    "\u0065\u0300\u0301\u0065",
    "\u0065\u0300\u0301\u0065\u0301",
    "\u0065\u0300\u0301\u0065\u0301\u0300",
    "\u0065\u0300\u0301\u0065\u0300",
    "\u0065\u0300\u0301\u0065\u0300\u0301"
  };

  /**
  * Miscellaneous tests
  */
  private final static String MISCELLANEOUS_TEST_CASE_[] = 
  {
    "\u0061\u0065",
    "\u0061\u0066",
    "\u00E6",
    "\u00C6",
    "\u006F\u0065",
    "\u0153",
    "\u0152",
    "\u006F\u0066"
  };
}

