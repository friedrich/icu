/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4jni/src/classes/com/ibm/icu4jni/text/CollationElementIterator.java,v $ 
* $Date: 2002/11/07 22:38:22 $ 
* $Revision: 1.6 $
*
*******************************************************************************
*/

package com.ibm.icu4jni.text;

import com.ibm.icu4jni.common.ErrorCode;

/**
* Collation element iterator JNI wrapper.
* Iterates over the collation elements of a data string.
* The iterator supports both forward and backwards full iteration, ie if 
* backwards iteration is performed in the midst of a forward iteration, the 
* result is undefined. 
* To perform a backwards iteration in the midst of a forward iteration, 
* reset() has to be called. 
* This will shift the position to either the start or the last character in the 
* data string depending on whether next() is called or previous().
* <pre>
*   RuleBasedCollator coll = Collator.getInstance();
*   CollationElementIterator iterator = coll.getCollationElementIterator("abc");
*   int ce = 0;
*   while (ce != CollationElementIterator.NULLORDER) {
*     ce = iterator.next();
*   }
*   iterator.reset();
*   while (ce != CollationElementIterator.NULLORDER) {
*     ce = iterator.previous();
*   }
* </pre>
* @author syn wee quek
* @since Jan 22 01
*/
    
public final class CollationElementIterator
{
  // public data member -------------------------------------------
  
  /**
   * @stable
   */
  public static final int NULLORDER = 0xFFFFFFFF;
  
  // public methods -----------------------------------------------
  
  /**
  * Reset the collation elements to their initial state.
  * This will move the 'cursor' to the beginning of the text.
  * @stable
  */
  public void reset()
  {
    NativeCollation.reset(m_collelemiterator_);
  }

  /**
  * Get the ordering priority of the next collation element in the text.
  * A single character may contain more than one collation element.
  * @return next collation elements ordering, or NULLORDER if the end of the 
  *         text is reached.
  * @stable
  */
  public int next()
  {
    return NativeCollation.next(m_collelemiterator_);
  }

  /**
  * Get the ordering priority of the previous collation element in the text.
  * A single character may contain more than one collation element.
  * @return previous collation element ordering, or NULLORDER if the end of 
  *         the text is reached.
  * @stable
  */
  public int previous()
  {
    return NativeCollation.previous(m_collelemiterator_);
  }

  /**
  * Get the maximum length of any expansion sequences that end with the 
  * specified comparison order.
  * @param order collation order returned by previous or next.
  * @return maximum size of the expansion sequences ending with the collation 
  *              element or 1 if collation element does not occur at the end of 
  *              any expansion sequence
  * @stable
  */
  public int getMaxExpansion(int order)
  {
    return NativeCollation.getMaxExpansion(m_collelemiterator_, order);
  }

  /**
  * Set the text containing the collation elements.
  * @param source text containing the collation elements.
  * @stable
  */
  public void setText(String source)
  {
    NativeCollation.setText(m_collelemiterator_, source);
  }

  /**
  * Get the offset of the current source character.
  * This is an offset into the text of the character containing the current
  * collation elements.
  * @return offset of the current source character.
  * @stable
  */
  public int getOffset()
  {
    return NativeCollation.getOffset(m_collelemiterator_);
  }

  /**
  * Set the offset of the current source character.
  * This is an offset into the text of the character to be processed.
  * @param offset The desired character offset.
  * @stable
  */
  public void setOffset(int offset)
  {
    NativeCollation.setOffset(m_collelemiterator_, offset);
  }
  
  /**
  * Gets the primary order of a collation order.
  * @param order the collation order
  * @return the primary order of a collation order.
  * @stable
  */
  public static int primaryOrder(int order) 
  {
    return ((order & PRIMARY_ORDER_MASK_) >> PRIMARY_ORDER_SHIFT_) &
                                                       UNSIGNED_16_BIT_MASK_;
  }

  /**
  * Gets the secondary order of a collation order.
  * @param order the collation order
  * @return the secondary order of a collation order.
  * @stable
  */
  public static int secondaryOrder(int order)
  {
    return (order & SECONDARY_ORDER_MASK_) >> SECONDARY_ORDER_SHIFT_;
  }

  /**
  * Gets the tertiary order of a collation order.
  * @param order the collation order
  * @return the tertiary order of a collation order.
  * @stable
  */
  public static int tertiaryOrder(int order)
  {
    return order & TERTIARY_ORDER_MASK_;
  }
  
  // protected constructor ----------------------------------------
  
  /**
  * CollationElementIteratorJNI constructor. 
  * The only caller of this class should be 
  * RuleBasedCollator.getCollationElementIterator(). 
  * @param collelemiteratoraddress address of C collationelementiterator
  */
  CollationElementIterator(long collelemiteratoraddress)
  {
    m_collelemiterator_ = collelemiteratoraddress;
  }

  // protected methods --------------------------------------------
  
  /**
  * Garbage collection.
  * Close C collator and reclaim memory.
  */
  protected void finalize()
  {
    NativeCollation.closeElements(m_collelemiterator_);
  }
  
  // private data members -----------------------------------------
 
  /**
  * C collator
  */
  private long m_collelemiterator_;
  
  /** 
  * ICU constant primary order mask for collation elements 
  */
  private static final int PRIMARY_ORDER_MASK_ = 0xffff0000;
  /** 
  * ICU constant secondary order mask for collation elements 
  */
  private static final int SECONDARY_ORDER_MASK_ = 0x0000ff00;
  /** 
  * ICU constant tertiary order mask for collation elements 
  */
  private static final int TERTIARY_ORDER_MASK_ = 0x000000ff;
  /** 
  * ICU constant primary order shift for collation elements 
  */
  private static final int PRIMARY_ORDER_SHIFT_ = 16;
  /** 
  * ICU constant secondary order shift for collation elements 
  */
  private static final int SECONDARY_ORDER_SHIFT_ = 8;
  /**
  * Unsigned 16 bit mask
  */
  private static final int UNSIGNED_16_BIT_MASK_ = 0x0000FFFF;
}