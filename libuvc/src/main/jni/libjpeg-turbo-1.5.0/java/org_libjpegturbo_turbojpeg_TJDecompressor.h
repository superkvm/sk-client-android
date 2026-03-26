
#include <jni.h>


#ifndef _Included_org_libjpegturbo_turbojpeg_TJDecompressor
#define _Included_org_libjpegturbo_turbojpeg_TJDecompressor
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_init
  (JNIEnv *, jobject);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_destroy
  (JNIEnv *, jobject);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressHeader
  (JNIEnv *, jobject, jbyteArray, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3BIIIII
  (JNIEnv *, jobject, jbyteArray, jint, jbyteArray, jint, jint, jint, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3BIIIIIII
  (JNIEnv *, jobject, jbyteArray, jint, jbyteArray, jint, jint, jint, jint, jint, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3IIIIII
  (JNIEnv *, jobject, jbyteArray, jint, jintArray, jint, jint, jint, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3IIIIIIII
  (JNIEnv *, jobject, jbyteArray, jint, jintArray, jint, jint, jint, jint, jint, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV___3BI_3BI
  (JNIEnv *, jobject, jbyteArray, jint, jbyteArray, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV___3BI_3_3B_3II_3III
  (JNIEnv *, jobject, jbyteArray, jint, jobjectArray, jintArray, jint, jintArray, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decodeYUV___3_3B_3I_3II_3BIIIIIII
  (JNIEnv *, jobject, jobjectArray, jintArray, jintArray, jint, jbyteArray, jint, jint, jint, jint, jint, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decodeYUV___3_3B_3I_3II_3IIIIIIII
  (JNIEnv *, jobject, jobjectArray, jintArray, jintArray, jint, jintArray, jint, jint, jint, jint, jint, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
