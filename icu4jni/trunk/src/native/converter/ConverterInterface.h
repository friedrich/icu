/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class NativeConverter */

#ifndef _Included_com_ibm_icu4jni_converters_NativeConverter
#define _Included_com_ibm_icu4jni_converters_NativeConverter
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    canConvert
 * Signature: (JI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_canConvert
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    closeConverter
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_closeConverter
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    convertByteToChar
 * Signature: (J[BI[CI[IZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_convertByteToChar
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
 * Method:    flushByteToChar
 * Signature: (J[CI[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_flushByteToChar
  (JNIEnv *, jclass, jlong, jcharArray, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    flushCharToByte
 * Signature: (J[BI[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_flushCharToByte
  (JNIEnv *, jclass, jlong, jbyteArray, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    getMaxBytesPerChar
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_getMaxBytesPerChar
  (JNIEnv *, jclass, jlong);

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
 * Method:    setSubstitutionBytes
 * Signature: (J[BI)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionBytes
  (JNIEnv *, jclass, jlong, jbyteArray, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setSubstitutionChars
 * Signature: (J[CI)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionChars
  (JNIEnv *, jclass, jlong, jcharArray, jint);

/*
 * Class:     com_ibm_icu4jni_converters_NativeConverter
 * Method:    setSubstitutionModeCharToByte
 * Signature: (JZ)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_converters_NativeConverter_setSubstitutionModeCharToByte
  (JNIEnv *, jclass, jlong, jboolean);

#ifdef __cplusplus
}
#endif
#endif
