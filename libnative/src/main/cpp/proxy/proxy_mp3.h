
#ifndef SUPERKVM_PROXY_MP3_H
#define SUPERKVM_PROXY_MP3_H

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

void lameInit(JNIEnv *env, jobject instance, jint inSampleRate, jint outChannel, jint outSampleRate, jint outBitRate, jint quality);
jint lameEncode(JNIEnv *env, jobject instance, jshortArray leftBuf_, jshortArray rightBuf, jint sampleRate, jbyteArray mp3Buf);
jint lameFlush(JNIEnv *env, jobject instance, jbyteArray mp3Buf);
void lameClose(JNIEnv *env, jobject instance);

#ifdef __cplusplus
};
#endif

#endif 
