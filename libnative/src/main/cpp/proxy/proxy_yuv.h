
#ifndef SUPERKVM_PROXY_YUV_H
#define SUPERKVM_PROXY_YUV_H
#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <cstdlib>
#include "../module/yuv/yuv.h"
#include "../utils/logger.h"

void yuv420spToNv21(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height);
void nv21ToYuv420sp(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height);
void nv21ToYuv420spWithMirror(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height);
void nv21ToYuv420p(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height);
void nv21ToYuv420pWithMirror(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height);
void nativeRotateNV21(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height, jint degree);

#ifdef __cplusplus
};
#endif
#endif 
