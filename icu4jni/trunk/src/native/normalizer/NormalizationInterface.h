/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_ibm_icu4jni_text_NativeNormalizer */

#ifndef _Included_com_ibm_icu4jni_text_NativeNormalizer
#define _Included_com_ibm_icu4jni_text_NativeNormalizer
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_ibm_icu4jni_text_NativeNormalizer
 * Method:    normalize
 * Signature: ([CI[CII[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_text_NativeNormalizer_normalize___3CI_3CII_3I
  (JNIEnv *, jclass, jcharArray, jint, jcharArray, jint, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_text_NativeNormalizer
 * Method:    normalize
 * Signature: (Ljava/lang/String;I[I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_icu4jni_text_NativeNormalizer_normalize__Ljava_lang_String_2I_3I
  (JNIEnv *, jclass, jstring, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_text_NativeNormalizer
 * Method:    quickCheck
 * Signature: ([CII[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_text_NativeNormalizer_quickCheck___3CII_3I
  (JNIEnv *, jclass, jcharArray, jint, jint, jintArray);

/*
 * Class:     com_ibm_icu4jni_text_NativeNormalizer
 * Method:    quickCheck
 * Signature: (Ljava/lang/String;I[I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_icu4jni_text_NativeNormalizer_quickCheck__Ljava_lang_String_2I_3I
  (JNIEnv *, jclass, jstring, jint, jintArray);



#ifdef __cplusplus
}
#endif
#endif
