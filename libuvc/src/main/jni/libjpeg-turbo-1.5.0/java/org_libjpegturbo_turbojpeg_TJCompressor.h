
#include <jni.h>


#ifndef _Included_org_libjpegturbo_turbojpeg_TJCompressor
#define _Included_org_libjpegturbo_turbojpeg_TJCompressor
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_init
  (JNIEnv *, jobject);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_destroy
  (JNIEnv *, jobject);


JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3BIIII_3BIII
  (JNIEnv *, jobject, jbyteArray, jint, jint, jint, jint, jbyteArray, jint, jint, jint);


JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3BIIIIII_3BIII
  (JNIEnv *, jobject, jbyteArray, jint, jint, jint, jint, jint, jint, jbyteArray, jint, jint, jint);


JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3IIIII_3BIII
  (JNIEnv *, jobject, jintArray, jint, jint, jint, jint, jbyteArray, jint, jint, jint);


JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3IIIIIII_3BIII
  (JNIEnv *, jobject, jintArray, jint, jint, jint, jint, jint, jint, jbyteArray, jint, jint, jint);


JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compressFromYUV___3_3B_3II_3III_3BII
  (JNIEnv *, jobject, jobjectArray, jintArray, jint, jintArray, jint, jint, jbyteArray, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3BIIII_3BII
  (JNIEnv *, jobject, jbyteArray, jint, jint, jint, jint, jbyteArray, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3BIIIIII_3_3B_3I_3III
  (JNIEnv *, jobject, jbyteArray, jint, jint, jint, jint, jint, jint, jobjectArray, jintArray, jintArray, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3IIIII_3BII
  (JNIEnv *, jobject, jintArray, jint, jint, jint, jint, jbyteArray, jint, jint);


JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3IIIIIII_3_3B_3I_3III
  (JNIEnv *, jobject, jintArray, jint, jint, jint, jint, jint, jint, jobjectArray, jintArray, jintArray, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
