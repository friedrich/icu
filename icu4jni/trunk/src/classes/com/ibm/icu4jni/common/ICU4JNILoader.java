/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4jni/src/classes/com/ibm/icu4jni/common/ICU4JNILoader.java,v $ 
* $Date: 2003/05/30 00:06:03 $ 
* $Revision: 1.7 $
*
*******************************************************************************
*/ 

package com.ibm.icu4jni.common;
/**
 * Class for loading the JNI library
 * @internal ICU 2.4
 */
public final class ICU4JNILoader {
    /**
     * @internal ICU 2.4
     */    
    public static boolean LIBRARY_LOADED = false;
    /**
     * Loads the JNI library
     * @internal ICU 2.4
     */
    public static final void loadLibrary() 
            throws UnsatisfiedLinkError{
        try{
            System.loadLibrary("ICUInterface26");
            ErrorCode.LIBRARY_LOADED = true;  
        }
        catch(UnsatisfiedLinkError e){
            System.loadLibrary("ICUInterface26d");
        } 
    }
   
}  

