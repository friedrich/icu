/*
 *******************************************************************************
 * Copyright (C) 1996-2000, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/impl/Attic/UCharacterIterator.java,v $ 
 * $Date: 2002/06/21 01:16:06 $ 
 * $Revision: 1.7 $
 *
 *******************************************************************************
 */
package com.ibm.icu.impl;

import com.ibm.icu.text.Replaceable;
import com.ibm.icu.text.StringCharacterIterator;
import com.ibm.icu.text.UTF16;

import java.text.CharacterIterator;
import com.ibm.icu.impl.UCharArrayIterator;

/**
 * DLF- Docs mostly need 1) much more description of iteration behavior,
 * especially at endpoints and with empty or single character strings,
 * and 2) need to describe the other major difference with Java
 * CharacterIterator, which is that this also returns code points as
 * well as code units.  
 *
 * Don't understand why setIndex and moveIndex have different exception behavior.
 * I expect they shouldn't.
 */

/**
 * Abstract class that defines an API for iteration on text objects.This is an 
 * interface for forward and backward iteration and random access into a text 
 * object. Forward iteration is done with post-increment and backward iteration 
 * is done with pre-decrement semantics, while the 
 * <code>java.text.CharacterIterator</code> interface methods provided forward 
 * iteration with "pre-increment" and backward iteration with pre-decrement 
 * semantics. This API is more efficient for forward iteration over code points.
 * @author Ram
 * @version release 2.2, May 2002
 */
public abstract class UCharacterIterator 
                      implements Cloneable,UForwardCharacterIterator {

    
    // static final methods ----------------------------------------------------
    
    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * <code>Replaceable</code> object.
     * @param source a valid source as a <code>Replaceable</code> object
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */
    public static final UCharacterIterator getInstance(Replaceable source){
        return new ReplaceableCharacterIterator(source);
    }
    
    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * source string.
     * @param source a string
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */
    public static final UCharacterIterator getInstance(String source){
        return new ReplaceableCharacterIterator(source);
    }
    
    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * source character array.
     * @param source an array of UTF-16 code units
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */
    public static final UCharacterIterator getInstance(char[] source){
        return getInstance(source,0,source.length);
    }
    
    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * source character array.
     * @param source an array of UTF-16 code units
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */
    public static final UCharacterIterator getInstance(char[] source, int start, int limit){
        return new UCharArrayIterator(source,start,limit);
    }
    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * source StringBuffer.
     * @param source an string buffer of UTF-16 code units
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */
    public static final UCharacterIterator getInstance(StringBuffer source){
        return new ReplaceableCharacterIterator(source);
    }

    /**
     * Returns a <code>UCharacterIterator</code> object given a 
     * CharacterIterator.
     * @param source a valid CharacterIterator object.
     * @return UCharacterIterator object
     * @exception IllegalArgumentException if the argument is null
     */    
    public static final UCharacterIterator getInstance(CharacterIterator source){
        return new ICUCharacterIterator(source);
    }
       
    // public methods ----------------------------------------------------------
    /**
     * Returns a <code>java.text.CharacterIterator</code> object for
     * the underlying text of this iterator.  The returned iterator is
     * independent of this iterator.
     * @return java.text.CharacterIterator object 
     */
    public CharacterIterator getCharacterIterator(){
        return new StringCharacterIterator(this.getText());
    }    
   
    /**
     * Returns the code unit at the current index.  If index is out
     * of range, returns DONE.  Index is not changed.
     * @return current code unit
     */
    public abstract int current();
    
    /**
     * Returns the codepoint at the current index.
     * If the current index is invalid, DONE is returned.
     * If the current index points to a lead surrogate, and there is a following
     * trail surrogate, then the code point is returned.  Otherwise, the code
     * unit at index is returned.  Index is not changed. 
     * @return current codepoint
     */
    public int currentCodePoint(){
        int ch = current();
        if(UTF16.isLeadSurrogate((char)ch)){
            // advance the index to get the
            // next code point
            next();
            // due to post increment semantics
            // current() after next() actually
            // returns the char we want
            int ch2 = current();
            // current should never change
            // the current index so back off
            previous();
            
            if(UTF16.isTrailSurrogate((char)ch2)){
                // we found a surrogate pair 
                // return the codepoint
                return UCharacterProperty.getRawSupplementary(
                                                          (char)ch,(char)ch2
                                                             );
            }
        }
        return ch;
    }
    
    /**
     * Returns the length of the text
     * @return length of the text
     */
    public abstract int getLength();

    
    /**
     * Gets the current index in text.
     * @return current index in text.
     */
    public abstract int getIndex();


    /**
     * Returns the UTF16 code unit at index, and increments to the next
     * code unit (post-increment semantics).  If index is out of
     * range, DONE is returned, and the iterator is reset to the limit
     * of the text.
     * @return the next UTF16 code unit, or DONE if the index is at the limit
     *         of the text.  
     */
    public abstract int next();

    /**
     * Returns the code point at index, and increments to the next code
     * point (post-increment semantics).  If index does not point to a
     * valid surrogate pair, the behavior is the same as
     * <code>next()<code>.  Otherwise the iterator is incremented past
     * the surrogate pair, and the code point represented by the pair
     * is returned.
     * @return the next codepoint in text, or DONE if the index is at
     *         the limit of the text.  
     */
    public int nextCodePoint(){
        int ch1 = next();
        if(UTF16.isLeadSurrogate((char)ch1)){
            int ch2 = next();
            if(UTF16.isTrailSurrogate((char)ch2)){
                return UCharacterProperty.getRawSupplementary((char)ch1,
                                                              (char)ch2);
            }else if (ch2 != DONE) {
                // unmatched surrogate so back out
                previous();
            }
        }
        return ch1;
    }

    /**
     * Decrement to the position of the previous code unit in the
     * text, and return it (pre-decrement semantics).  If the
     * resulting index is less than 0, the index is reset to 0 and
     * DONE is returned.
     * @return the previous code unit in the text, or DONE if the new
     *         index is before the start of the text.  
     */
    public abstract int previous();

    
    /**
     * Retreat to the start of the previous code point in the text,
     * and return it (pre-decrement semantics).  If the index is not
     * preceeded by a valid surrogate pair, the behavior is the same
     * as <code>previous()</code>.  Otherwise the iterator is
     * decremented to the start of the surrogate pair, and the code
     * point represented by the pair is returned.
     * @return the previous code point in the text, or DONE if the new
     *         index is before the start of the text.  
     */
    public int previousCodePoint(){
        int ch1 = previous();
        if(UTF16.isTrailSurrogate((char)ch1)){
            int ch2 = previous();
            if(UTF16.isLeadSurrogate((char)ch2)){
                return UCharacterProperty.getRawSupplementary((char)ch2,
                                                              (char)ch1);
            }else if (ch2 != DONE) {
                //unmatched trail surrogate so back out
                next();
            }   
        }
        return ch1;
    }

    /**
     * Sets the index to the specified index in the text.
     * @param index the index within the text. 
     * @exception IndexOutOfBoundsException is thrown if an invalid index is 
     *            supplied
     */
    public abstract void setIndex(int index);

    /**
     * Sets the current index to the limit.
     */
    public void setToLimit() {
	    setIndex(getLength());
    }
    
    /**
     * Sets the current index to the start.
     */
    public void setToStart() {
	    setIndex(0);
    }

    /**
     * Fills the buffer with the underlying text storage of the iterator
     * If the buffer capacity is not enough a exception is thrown. The capacity
     * of the fill in buffer should at least be equal to length of text in the 
     * iterator obtained by calling <code>getLength()</code).
     * <b>Usage:</b>
     * 
     * <code>
     * <pre>
     *         UChacterIterator iter = new UCharacterIterator.getInstance(text);
     *         char[] buf = new char[iter.getLength()];
     *         iter.getText(buf);
     *         
     *         OR
     *         char[] buf= new char[1];
     *         int len = 0;
     *         for(;;){
     *             try{
     *                 len = iter.getText(buf);
     *                 break;
     *             }catch(IndexOutOfBoundsException e){
     *                 buf = new char[iter.getLength()];
     *             }
     *         }
     * </pre>
     * </code>
     *             
     * @param fillIn an array of chars to fill with the underlying UTF-16 code 
     *         units.
     * @param offset the position within the array to start putting the data.
     * @return the number of code units added to fillIn, as a convenience
     * @exception IndexOutOfBounds exception if there is not enough
     *            room after offset in the array, or if offset < 0.  
     */
    public int getText(char[] fillIn, int offset) {
		int len = getLength();
		if (offset < 0 || offset + len > fillIn.length) {
		    throw new IndexOutOfBoundsException(Integer.toString(offset));
		}
		int index = getIndex();
		setToStart();
		int ch;
		while ((ch = next())!= DONE) {
		    fillIn[offset++] = (char)ch;
		}
		setIndex(index);
		return len;
    }

    /**
     * Convenience override for <code>getText(char[], int)>/code> that provides
     * an offset of 0.
     * @param fillIn an array of chars to fill with the underlying UTF-16 code 
     *         units.
     * @return the number of code units added to fillIn, as a convenience
     * @exception IndexOutOfBounds exception if there is not enough
     *            room in the array.  
     */
    public final int getText(char[] fillIn) {
		return getText(fillIn, 0);
    }
         
    /**
     * Convenience method for returning the underlying text storage as as string
     * @return the underlying text storage in the iterator as a string
     */
    public String getText() {
		char[] text = new char[getLength()];
		getText(text);
		return new String(text);
    }
       
    /**
     * Moves the current position by the number of code units
     * specified, either forward or backward depending on the sign
     * of delta (positive or negative respectively).  If the resulting
     * index would be less than zero, the index is set to zero, and if
     * the resulting index would be greater than limit, the index is
     * set to limit.
     *
     * @param delta the number of code units to move the current
     *              index.
     * @return the new index.
     * @exception IndexOutOfBoundsException is thrown if an invalid index is 
     *            supplied  
     * 
     */
    public int moveIndex(int delta) {
		int x = Math.max(0, Math.min(getIndex() + delta, getLength()));
		setIndex(x);
		return x;
    }

    /**
     * Moves the current position by the number of code points
     * specified, either forward or backward depending on the sign of
     * delta (positive or negative respectively). If the current index
     * is at a trail surrogate then the first adjustment is by code
     * unit, and the remaining adjustments are by code points.  If the
     * resulting index would be less than zero, the index is set to
     * zero, and if the resulting index would be greater than limit,
     * the index is set to limit.
     * @param delta the number of code units to move the current index.
     * @return the new index  
     * @exception IndexOutOfBoundsException is thrown if an invalid delta is 
     *            supplied
     */
    public int moveCodePointIndex(int delta){
        if(delta>0){
            while(delta-->0 && nextCodePoint() != DONE);
        }else{
	        while(delta++<0 && previousCodePoint() != DONE);
        }
        if(delta!=0){
            throw new IndexOutOfBoundsException();
        }
          
        return getIndex();
    }

    /**
     * Creates a copy of this iterator, independent from other iterators.
     * If it is not possible to clone the iterator, returns null.
     * @return copy of this iterator
     */
    public Object clone() throws CloneNotSupportedException{
	    return super.clone();
    }   
    
}

