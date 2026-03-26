
#include <jni.h>


#ifndef _Included_org_libjpegturbo_turbojpeg_TJTransformer
#define _Included_org_libjpegturbo_turbojpeg_TJTransformer
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_init
  (JNIEnv *, jobject);


JNIEXPORT jintArray JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_transform
  (JNIEnv *, jobject, jbyteArray, jint, jobjectArray, jobjectArray, jint);

#ifdef __cplusplus
}
#endif
#endif
