/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: 
*  /usr/cvs/icu4j/icu4j/src/com/ibm/icu/test/text/CollatorAPITest.java,v $ 
* $Date: 2003/04/12 01:21:59 $ 
* $Revision: 1.13 $
*
*******************************************************************************
*/

package com.ibm.icu4jni.test.text;

import java.util.Locale;
import com.ibm.icu4jni.test.TestFmwk;
import com.ibm.icu4jni.text.Collator;
import com.ibm.icu4jni.text.RuleBasedCollator;
import com.ibm.icu4jni.text.CollationKey;
import com.ibm.icu4jni.text.CollationAttribute;
import com.ibm.icu4jni.text.CollationElementIterator;

/**
* Collator API testing class
* Mostly following the test cases for ICU
* @author Syn Wee Quek
* @since jan 29 2001
*/
public final class CollatorAPITest extends TestFmwk
{ 
  
  // constructor ===================================================
  
  /**
  * Constructor
  */
  public CollatorAPITest()
  {
  }
  
  // public methods ================================================

  /**
  * Testing collator class properties.
  * Constructor, compare, strength retrieval/set, decomposition 
  * retrievale/set
  * @exception thrown when error occurs while setting strength
  */
  public void TestProperties() throws Exception
  {
    logln("TestProperties --");
    Collator collator = Collator.getInstance(Locale.ENGLISH);
    
    if (collator.compare("ab", "abc") != Collator.RESULT_LESS)
      errln("Failed : ab < abc comparison");
    if (collator.compare("ab", "AB") != Collator.RESULT_LESS)
      errln("Failed : ab < AB");
    if (collator.compare("blackbird", "black-bird") != 
                                                Collator.RESULT_GREATER)
      errln("Failed : black-bird > blackbird comparison");
    if (collator.compare("black bird", "black-bird") != 
                                                   Collator.RESULT_LESS)
      errln("Failed : black bird > black-bird comparison");
    if (collator.compare("Hello", "hello") != Collator.RESULT_GREATER)
      errln("Failed : Hello > hello comparison");

    if (collator.getStrength() != Collator.TERTIARY)
      errln("Failed : Default collation have tertiary strength");
        
    collator.setStrength(Collator.SECONDARY);
    if (collator.getStrength() != Collator.SECONDARY)
      errln("Failed : Collation strength set to secondary");
   
    collator.setDecomposition(Collator.NO_DECOMPOSITION);
    if (collator.getDecomposition() != Collator.NO_DECOMPOSITION)
      errln("Failed : Collation strength set to no normalization");

    collator =  Collator.getInstance(Locale.FRENCH);
    
    collator.setStrength(Collator.PRIMARY);
    if (collator.getStrength() != Collator.PRIMARY)
      errln("Failed : Collation strength set to primary");
      
    collator.setStrength(Collator.TERTIARY);
    if (collator.getStrength() != Collator.TERTIARY)
      errln("Failed : Collation strength set to tertiary");

    // testing rubbish collator
    // should return the default
    Locale abcd = new Locale("ab", "CD");
    collator = Collator.getInstance(abcd);
    Collator defaultcollator = Collator.getInstance();

    if (!((RuleBasedCollator)collator).getRules().equals(
         ((RuleBasedCollator)defaultcollator).getRules())) {
      errln("Failed: Undefined locale should return the default " +
                    "collator ");
      String str = ((RuleBasedCollator)defaultcollator).getRules();
      errln("Default collator: " + str + " " + str.length());
      str = ((RuleBasedCollator)collator).getRules();
      errln("Default collator: " + str + " " + str.length());
    }
                    
    Collator frenchcollator = Collator.getInstance(Locale.FRANCE);
    if (frenchcollator.equals(collator))
      errln("Failed : French collator should be different from default " +
            "collator");
                    
    Collator clonefrench = (Collator)frenchcollator.clone();
    if (!((RuleBasedCollator)frenchcollator).getRules().equals(
         ((RuleBasedCollator)clonefrench).getRules()))
      errln("Failed : Cloning of a French collator");
  }

  public void TestRuleBasedColl() throws Exception
  {
    String ruleset1 = "&9 < a, A < b, B < c, C; ch, cH, Ch, CH < d, D, e, E";
    String ruleset2 = "&9 < a, A < b, B < c, C < d, D, e, E";
      
    RuleBasedCollator col1 = new RuleBasedCollator(ruleset1);
    RuleBasedCollator col2 = new RuleBasedCollator(ruleset2);
    RuleBasedCollator col3 = (RuleBasedCollator)Collator.getInstance();
      
    String rule1 = col1.getRules();
    String rule2 = col2.getRules();
    String rule3 = col3.getRules();

    if (rule1.equals(rule2)) {
      errln("Failed : Collators with different rules should produce " +
            "different results with getRules");
    }
    if (rule2.equals(rule3)) {
      errln("Failed : Collators with different rules should produce " +
            "different results with getRules");
    }
    if (rule1.equals(rule3)) {
      errln("Failed : Collators with different rules should produce " +
            "different results with getRules");
    }
      
    RuleBasedCollator col4 = new RuleBasedCollator(rule2);
    String rule4 = col4.getRules();
    if (!rule2.equals(rule4)) {
      errln("Railed : Collators with the same rules should produce " +
            "same results with getRules");
    }
  }

  public void TestDecomposition() throws Exception {
    Collator en_US = Collator.getInstance(Locale.US),
      el_GR = Collator.getInstance(new Locale("el", "GR")),
      vi_VN = Collator.getInstance(new Locale("vi", "VN"));

    // there is no reason to have canonical decomposition in en_US OR default 
    // locale
    if (vi_VN.getDecomposition() != Collator.CANONICAL_DECOMPOSITION) {
      errln("Failed : vi_VN collation did not have cannonical " +
            "decomposition for normalization!");
    }

    if (el_GR.getDecomposition() != Collator.CANONICAL_DECOMPOSITION) {
      errln("Failed : el_GR collation did not have cannonical " +
            "decomposition for normalization!");
    }

    if (en_US.getDecomposition() != Collator.NO_DECOMPOSITION) {
      errln("Failed : en_US collation had cannonical decomposition for " +
            "normalization!");
    }
  }

  public void TestSafeClone() throws Exception {
    Collator col;
	  Collator clone;
    
    String test1 = "abCda";
    String test2 = "abcda";
    
    Locale loc[] = {Locale.ENGLISH, Locale.KOREA, Locale.JAPAN};
      
    // one default collator & two complex ones
    for (int i = 0; i < 3; i ++) {
      col = Collator.getInstance(loc[i]);
      clone = (Collator)col.clone();
      clone.setStrength(Collator.TERTIARY);
      col.setStrength(Collator.PRIMARY);
      clone.setAttribute(CollationAttribute.CASE_LEVEL, 
                        CollationAttribute.VALUE_OFF);
      col.setAttribute(CollationAttribute.CASE_LEVEL, 
                      CollationAttribute.VALUE_OFF);
      if (clone.compare(test1, test2) != Collator.RESULT_GREATER) {
        errln("Failed : Result should be " + test1 + " >>> " + test2);
      }
      if (col.compare(test1, test2) != Collator.RESULT_EQUAL) {
        errln("Failed : Result should be " + test1 + " >>> " + test2);
      }
    }
  }

  /**
  * Testing hash code method
  * @exception thrown when error occurs while setting strength
  */
  public void TestHashCode() throws Exception
  {
    logln("TestHashCode --");

    /* hash code not implemented yet
    Locale dk = new Locale("da", "DK");
    Collator collator = Collator.getInstance(dk);
    
    Collator defaultcollator = Collator.getInstance(Locale.English);
    
    if (defaultcollator.hashCode() == collator.hashCode())
      errln("Failed : Default collator's hash code not equal to " +
                    "Danish collator's hash code");                 
    if (defaultcollator.hashCode() != defaultcollator.hashCode())
      errln("Failed : Hash code of two default collators are equal");               
      */
  }
  
  /**
  * Test collation key
  * @exception thrown when error occurs while setting strength
  */
  public void TestCollationKey() throws Exception
  {
    logln("TestCollationKey --");

    String test1 = "Abcda", 
           test2 = "abcda";
    
    Collator defaultcollator = Collator.getInstance(Locale.ENGLISH);
    defaultcollator.setStrength(Collator.TERTIARY);
    CollationKey sortk1 = defaultcollator.getCollationKey(test1), 
                 sortk2 = defaultcollator.getCollationKey(test2);
    if (sortk1.compareTo(sortk2) != Collator.RESULT_GREATER)
      errln("Failed : Abcda >>> abcda");

    if (sortk1.equals(sortk2))
      errln("Failed : The sort keys of different strings should be " +
                    "different");
    if (sortk1.hashCode() == sortk2.hashCode())
      errln("Failed : sort key hashCode() for different strings " +
                    "should be different");
                    
    defaultcollator.setStrength(Collator.SECONDARY);
    if (defaultcollator.getCollationKey(test1).compareTo(
                                     defaultcollator.getCollationKey(test2)) 
                                     != Collator.RESULT_EQUAL) {
      errln("Failed : Result should be " + test1 + " == " + test2);
    }
  }
  
  /**
  * Testing the functionality of the collation element iterator
  * @exception thrown when error occurs while setting strength
  */
  public void TestElementIterator() throws Exception
  {       
    logln("TestElementIterator --");

    String test1 = "XFILE What subset of all possible test cases has the " +
                   "highest probability of detecting the most errors?";
    String test2 = "Xf_ile What subset of all possible test cases has the " +
                   "lowest probability of detecting the least errors?";
    Collator defaultcollator = Collator.getInstance(Locale.ENGLISH);
    
    CollationElementIterator iterator1 = 
      ((RuleBasedCollator)defaultcollator).getCollationElementIterator(test1);
    iterator1.setOffset(6);
    iterator1.setOffset(0);
    
    // copy ctor
    CollationElementIterator iterator2 = ((RuleBasedCollator)defaultcollator).getCollationElementIterator(test1);
    CollationElementIterator iterator3 = ((RuleBasedCollator)defaultcollator).getCollationElementIterator(test2);
    
    int offset = 0;
    offset = iterator1.getOffset();
    if (offset != 0) {
        errln("Error in getOffset for collation element iterator\n");
        return;
    }
    iterator1.setOffset(6);
    iterator1.setOffset(0);

    int order1, order2, order3;
    
	order1 = iterator1.next();
    /* equals not implemented
	if (iterator1.equals(iterator2)) {
		errln("The first iterator advance failed");
		return;
	}
	*/
    order2 = iterator2.next();
      
	/* equals not implemented  
    if (!iterator1.equals(iterator2)) {
		errln("The second iterator advance failed");
		return;
	} 
	*/
    if (order1 != order2) {
	    errln("The order result should be the same");
		return;
	}
    order3 = iterator3.next();
       
    if (CollationElementIterator.primaryOrder(order1) != 
        CollationElementIterator.primaryOrder(order3)) { 
		errln("The primary orders should be the same");
		return;
	}
    if (CollationElementIterator.secondaryOrder(order1) != 
        CollationElementIterator.secondaryOrder(order3)) {
		errln("The secondary orders should be the same");
		return;
	}
    if (CollationElementIterator.tertiaryOrder(order1) != 
        CollationElementIterator.tertiaryOrder(order3)) {
		errln("The tertiary orders should be the same");
		return;
	}
    
    order1 = iterator1.next(); 
    order3 = iterator3.next();
        
    if (CollationElementIterator.primaryOrder(order1) != 
        CollationElementIterator.primaryOrder(order3)) {
		errln("The primary orders should be identical");
	}
    if (CollationElementIterator.tertiaryOrder(order1) == 
        CollationElementIterator.tertiaryOrder(order3)) {
		errln("The tertiary orders should be different");
		return;
	}
    
    order1 = iterator1.next(); 
    order3 = iterator3.next();
    // invalid test wrong in UCA
    // doAssert((CollationElementIterator.secondaryOrder(order1) != 
    //    CollationElementIterator.secondaryOrder(order3)), "The secondary orders should not be the same");
            
    if (order1 == CollationElementIterator.NULLORDER) {
		errln("Unexpected end of iterator reached");
		return;
	}
    
    iterator1.reset(); 
    iterator2.reset(); 
    iterator3.reset();
    order1 = iterator1.next();
        
    /* equals not implemented
	if (iterator1.equals(iterator2)) {
		errln("The first iterator advance failed");
		return;
	}
	*/
        
    order2 = iterator2.next();
       
    /* equals not implemented
	if (!iterator1.equals(iterator2)) {
		errln("The second iterator advance failed");
		return;
	}
	*/
    if (order1 != order2) {
		errln("The order result should be the same");
		return;
	}
    
    order3 = iterator3.next();
        
    if (CollationElementIterator.primaryOrder(order1) != 
        CollationElementIterator.primaryOrder(order3)) {
		errln("The primary orders should be the same");
		return;
	}
    if (CollationElementIterator.secondaryOrder(order1) != 
        CollationElementIterator.secondaryOrder(order3)) {
		errln("The secondary orders should be the same");
		return;
	}
    if (CollationElementIterator.tertiaryOrder(order1) != 
        CollationElementIterator.tertiaryOrder(order3)) {
		errln("The tertiary orders should be the same");
		return;
	}
    
    order1 = iterator1.next(); 
    order2 = iterator2.next(); 
    order3 = iterator3.next();
        
    if (CollationElementIterator.primaryOrder(order1) != 
        CollationElementIterator.primaryOrder(order3)) {
		errln("The primary orders should be identical");
		return;
	}
    if (CollationElementIterator.tertiaryOrder(order1) == 
        CollationElementIterator.tertiaryOrder(order3)) {
		errln("The tertiary orders should be different");
		return;
	}
    
    order1 = iterator1.next(); 
    order3 = iterator3.next();
        
    // obsolete invalid test, removed
    // doAssert((CollationElementIterator.secondaryOrder(order1) != 
    //    CollationElementIterator.secondaryOrder(order3)), "The secondary orders should not be the same");
    if (order1 == CollationElementIterator.NULLORDER) {
		errln("Unexpected end of iterator reached");
		return;
	}
    /* equals not implemented
	if (iterator2.equals(iterator3)) {
		errln("The iterators should be different");
		return;
	}
	*/
    logln("testing CollationElementIterator ends...");
  }

  /** 
  * Test RuleBasedCollator constructor, clone, copy, and getRules
  * @exception thrown when error occurs while setting strength
  */
  public void TestOperators() throws Exception
  {
    logln("TestOperators --");

    String ruleset1 = "< a, A < b, B < c, C; ch, cH, Ch, CH < d, D, e, E";
    String ruleset2 = "< a, A < b, B < c, C < d, D, e, E";
    RuleBasedCollator col1 = new RuleBasedCollator(ruleset1);
    
    RuleBasedCollator col2 = new RuleBasedCollator(ruleset2);
    
    if (col1.equals(col2))
      errln("Failed : Two different rule collations should return " +
                    "different comparisons");
    
    Collator col3 = Collator.getInstance(Locale.ENGLISH);
    
    Collator col4 = (Collator)col1.clone();
    Collator col5 = (Collator)col3.clone();
    
    if (!col1.equals(col4)) {
      errln("Failed : Cloned collation objects are equal");
    }
    if (col3.equals(col4))
      errln("Failed : Two different rule collations should compare " +
                    "different");
    if (!col3.equals(col5)) 
      errln("Failed : Cloned collation objects should be equal");
    if (col4.equals(col5))
      errln("Failed : Clones of 2 different collations should " +
                    "compare different");

    String defrules = ((RuleBasedCollator)col3).getRules();
    if (defrules.length() > 0)
    {
      RuleBasedCollator col6 = new RuleBasedCollator(defrules);
      if (!((RuleBasedCollator)col3).getRules().equals(col6.getRules())) 
        errln("Failed : Rules from one collator should create a same " +
                      "collator");
    }

    RuleBasedCollator col7 = new RuleBasedCollator(ruleset2, 
                                           Collator.TERTIARY);
    RuleBasedCollator col8 = new RuleBasedCollator(ruleset2, 
                                  Collator.PRIMARY);
    RuleBasedCollator col9 = new RuleBasedCollator(ruleset2, 
        Collator.CANONICAL_DECOMPOSITION, Collator.PRIMARY);
    
    if (col7.equals(col9))
      errln("Failed : Two different rule collations should compare " +
                    "different");
    if (col8.equals(col9))
      errln("Failed : Two different rule collations should compare " +
                    "different");
  }

  /**
  * Test clone and copy
  * @exception thrown when error occurs while setting strength
  */
  public void TestDuplicate() throws Exception
  {
    logln("TestDuplicate --");

    Collator defaultcollator = Collator.getInstance(Locale.ENGLISH);
    Collator col2 = (Collator)defaultcollator.clone();
    
    if (!((RuleBasedCollator)defaultcollator).equals(
         (RuleBasedCollator)col2))
      errln("Failed : Cloned object should be equal to the orginal");
    String ruleset = "< a, A < b, B < c, C < d, D, e, E";
    RuleBasedCollator col3 = new RuleBasedCollator(ruleset);
    if (((RuleBasedCollator)defaultcollator).equals(
         (RuleBasedCollator)col3))
      errln("Failed : Cloned object not equal to collator created " + 
                    "by rules");
  }   

  /**
  * Testing compare methods
  * @exception thrown when error occurs while setting strength
  */
  public void TestCompare() throws Exception
  {
    logln("TestCompare --");
    
    String test1 = "Abcda", 
           test2 = "abcda";
           
    Collator defaultcollator = Collator.getInstance(Locale.ENGLISH);
    
    if (defaultcollator.compare(test1, test2) != Collator.RESULT_GREATER)
      errln("Failed : Result should be Abcda >>> abcda");
    
    defaultcollator.setStrength(Collator.SECONDARY);
    
    if (defaultcollator.compare(test1, test2) != Collator.RESULT_EQUAL)
      errln("Failed : Result should be Abcda == abcda");
    
    defaultcollator.setStrength(Collator.PRIMARY);
    
    if (!defaultcollator.equals(test1, test2))
      errln("Failed : Result should be Abcda == abcda");
  }

  // private variables =============================================
  
  /**
  * Source string for testing
  */
  private static final String SOURCE_TEST_CASE_ = 
                                           "-abcdefghijklmnopqrstuvwxyz#&^$@";
}

