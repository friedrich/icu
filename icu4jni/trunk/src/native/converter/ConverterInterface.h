/**
*******************************************************************************
* Copyright (C) 1996-2001, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4jni/src/native/converter/ConverterInterface.h,v $ 
* $Date: 2002/10/29 01:58:23 $ 
* $Revision: 1.10 $
*
*******************************************************************************
*/
/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_ibm_icu4jni_converters_NativeConverter */

#ifndef _Included_com_ibm_icu4jni_converters_NativeConverter
#define _Included_com_ibm_icu4jni_converters_NativeConverter
#ifdef __cplusplus
extern "C" {
#endif
#undef com_ibm_icu4jni_converters_NativeConverter_STOP_CALLBACK
#define com_ibm_icu4jni_converters_NativeConverter_STOP_CALLBACK 0L
#undef com_ibm_icu4jni_converters_NativeConverter_SKIP_CALLBACK
#define com_ibm_icu4jni_converters_NativeConverter_SKIP_CALLBACK 1L
#undef com_ibm_icu4jni_converters_NativeConverter_SUBSTITUTE_CALLBACK
#define com_ibm_icu4jni_converters_NativeConverter_SUBSTITUTE_CALLBACK 3L
/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    convertByteToChar
 * Signature: (J[BI[CI[IZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_convertByteToChar
  (JNIEnv *, jclass, jlong, jbyteArray, jint, jcharArray, jint, jintArray, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    decode
 * Signature: (J[BI[CI[IZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_decode
  (JNIEnv *, jclass, jlong, jbyteArray, jint, jcharArray, jint, jintArray, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    convertCharToByte
 * Signature: (J[CI[BI[IZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_convertCharToByte
  (JNIEnv *, jclass, jlong, jcharArray, jint, jbyteArray, jint, jintArray, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    encode
 * Signature: (J[CI[BI[IZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_encode
  (JNIEnv *, jclass, jlong, jcharArray, jint, jbyteArray, jint, jintArray, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    flushCharToByte
 * Signature: (J[BI[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_flushCharToByte
  (JNIEnv *, jclass, jlong, jbyteArray, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    flushByteToChar
 * Signature: (J[CI[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_flushByteToChar
  (JNIEnv *, jclass, jlong, jcharArray, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    openConverter
 * Signature: ([JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_openConverter
  (JNIEnv *, jclass, jlongArray, jstring);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    resetByteToChar
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_resetByteToChar
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    resetCharToByte
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_resetCharToByte
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    closeConverter
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_closeConverter
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setSubstitutionChars
 * Signature: (J[CI)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionChars
  (JNIEnv *, jclass, jlong, jcharArray, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setSubstitutionBytes
 * Signature: (J[BI)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionBytes
  (JNIEnv *, jclass, jlong, jbyteArray, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setSubstitutionModeCharToByte
 * Signature: (JZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionModeCharToByte
  (JNIEnv *, jclass, jlong, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    countInvalidBytes
 * Signature: (J[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_countInvalidBytes
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    countInvalidChars
 * Signature: (J[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_countInvalidChars
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getMaxBytesPerChar
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getMaxBytesPerChar
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getAveBytesPerChar
 * Signature: (J)F
 */
JNIEXPORT jfloat JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getAveBytesPerChar
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getMaxCharsPerByte
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getMaxCharsPerByte
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getAveCharsPerByte
 * Signature: (J)F
 */
JNIEXPORT jfloat JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getAveCharsPerByte
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getSubstitutionBytes
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getSubstitutionBytes
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    canEncode
 * Signature: (JI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_canEncode
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    canDecode
 * Signature: (J[B)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_canDecode
  (JNIEnv *, jclass, jlong, jbyteArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    countAvailable
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_countAvailable
  (JNIEnv *, jclass);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getAvailable
 * Signature: ()[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getAvailable
  (JNIEnv *, jclass);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    countAliases
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_countAliases
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getAliases
 * Signature: (Ljava/lang/String;)[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getAliases
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getCanonicalName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getCanonicalName
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setCallbackDecode
 * Signature: (JIZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setCallbackDecode
  (JNIEnv *, jclass, jlong, jint, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setCallbackEncode
 * Signature: (JIZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setCallbackEncode
  (JNIEnv *, jclass, jlong, jint, jboolean);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    safeClone
 * Signature: (JIZ)I
 */
JNIEXPORT jlong JNICALL Java_com_ibm_icu4jni_text_NativeConverter_safeClone
  (JNIEnv *, jclass, jlong, jlongArray);

#ifdef __cplusplus
}
#endif
#endif
