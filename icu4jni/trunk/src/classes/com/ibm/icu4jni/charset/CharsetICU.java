/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4jni/src/classes/com/ibm/icu4jni/charset/CharsetICU.java,v $ 
* $Date: 2001/10/18 01:16:44 $ 
* $Revision: 1.3 $
*
*******************************************************************************
*/ 

package com.ibm.icu4jni.charset;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.*;
import java.nio.charset.spi.CharsetProvider;
import com.ibm.icu4jni.charset.*;
import com.ibm.icu4jni.converters.*;
import com.ibm.icu4jni.common.*;

public class CharsetICU extends Charset{
    /**
     * Constructor to create a the CharsetICU object
     * @param the canonical name as a string
     * @param the alias set as an array of strings
     */
    protected CharsetICU(String canonicalName, String[] aliases) {
	     super(canonicalName,aliases);
    }
    /**
     * Returns a new decoder instance of this charset object
     * @return a new decoder object
     */
    public CharsetDecoder newDecoder(){
        return new CharsetDecoderICU((Charset)this,this.toString());
    };
    
    /**
     * Returns a new encoder object of the charset
     * @return a new encoder
     */
    public CharsetEncoder newEncoder(){
        byte[] replacement = { 0x001a };
        return new CharsetEncoderICU((Charset)this,this.toString(),replacement);
    }; 
    
    /**
     * Ascertains if a charset is a sub set of this charset
     * @param charset to test
     * @return true if the given charset is a subset of this charset
     */
    public boolean contains(Charset cs){
        return false;
    }
}