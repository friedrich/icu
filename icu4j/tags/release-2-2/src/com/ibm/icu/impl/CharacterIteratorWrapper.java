/*
 *******************************************************************************
 * Copyright (C) 1996-2000, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /usr/cvs/icu4j/icu4j/src/com/ibm/icu/impl/ICUCharacterIterator.java,v $ 
 * $Date: 2002/06/20 01:18:07 $ 
 * $Revision: 1.1 $
 *
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.text.CharacterIterator;

/**
 * This class is a wrapper around CharacterIterator and implements the 
 * UCharacterIterator protocol
 * @author ram
 */

public class CharacterIteratorWrapper extends UCharacterIterator {
    
    private CharacterIterator iterator;
    
    /**
     * length
     */
    private int length;
    
    public CharacterIteratorWrapper(CharacterIterator iter){
        if(iter==null){
            throw new IllegalArgumentException();
        }
        iterator     = iter;
        length       = iter.getEndIndex() - iter.getBeginIndex();    
    }

    /**
     * @see UCharacterIterator#current()
     */
    public int current() {
	/*	if (currentIndex < length) {
		    return iterator.setIndex(beginIndex + currentIndex);
		}
    */
        int c = iterator.current();
        if(c==iterator.DONE){
		  return DONE;
        }
        return c;
    }

    /**
     * @see UCharacterIterator#getLength()
     */
    public int getLength() {
	    return length;
    }

    /**
     * @see UCharacterIterator#getIndex()
     */
    public int getIndex() {
	    //return currentIndex;
        return iterator.getIndex();
    }

    /**
     * @see UCharacterIterator#next()
     */
    public int next() {
		/*if(currentIndex < length){
		    return iterator.setIndex(beginIndex + currentIndex++);
		}
        return DONE;
        */
        int i = iterator.current();
        iterator.next();
        if(i==iterator.DONE){  
		  return DONE;
        }
        return i;
    }

    /**
     * @see UCharacterIterator#previous()
     */
    public int previous() {
	    /*if(currentIndex>0){
	        return iterator.setIndex(beginIndex + --currentIndex);
	    }
        return DONE;
        */
        int i = iterator.previous();
        if(i==iterator.DONE){
            return DONE;
        }
        return i;
    }

    /**
     * @see UCharacterIterator#setIndex(int)
     */
    public void setIndex(int index) {
		/*if (index < 0 || index > length) {
		    throw new IndexOutOfBoundsException();
		}
		currentIndex = index;
        */
        try{
            iterator.setIndex(index);
        }catch(IllegalArgumentException e){
            throw new IndexOutOfBoundsException();
        }
    }

    /**
     * @see UCharacterIterator#setToLimit()
     */
    public void setToLimit() {
		iterator.setIndex(length);
    }

    /**
     * @see UCharacterIterator#getText(char[])
     */
    public int getText(char[] fillIn, int offset){
        int currentIndex = iterator.getIndex();
        if(offset < 0 || offset + length > fillIn.length){
            throw new IndexOutOfBoundsException(Integer.toString(length));
        }
	
        for (char ch = iterator.first(); ch != iterator.DONE; ch = iterator.next()) {
	        fillIn[offset++] = ch;
	    }
	    iterator.setIndex(currentIndex);

        return length;
    }

    /**
     * Creates a clone of this iterator.  Clones the underlying character iterator.
     * @see UCharacterIterator#clone()
     */
    public Object clone(){
		try {
		    CharacterIteratorWrapper result = (CharacterIteratorWrapper) super.clone();
		    result.iterator = (CharacterIterator)this.iterator.clone();
		    return result;
		} catch (CloneNotSupportedException e) {      
            return null; // only invoked if bad underlying character iterator
		}
    }
    
    /**
     * @see UCharacterIterator#moveIndex()
     */
    public int moveIndex(int index){
        int idx = iterator.getIndex()+index;
        
        if(idx < 0) {
	        idx = 0;
		} else if(idx > length) {
		    idx = length;
		}
        return iterator.setIndex(idx);
    }
    
    /**
     * @see UCharacterIterator#getCharacterIterator()
     */
    public CharacterIterator getCharacterIterator(){
        return (CharacterIterator)iterator.clone();
    } 
}
