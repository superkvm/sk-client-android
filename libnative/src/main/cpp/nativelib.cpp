
#include <jni.h>
#include <string>
#include "utils/logger.h"
#include "proxy/proxy_yuv.h"
#include "proxy/proxy_mp3.h"

#define NUM_METHODS(x) ((int)(sizeof(x)/ sizeof(x[0])))
JavaVM *globalJvm;
const char * yuvClsPath = "com/superkvm/natives/YUVUtils";
const char * lameClsPath = "com/superkvm/natives/LameMp3";

static JNINativeMethod g_yuv_methods[] = {
        {"yuv420spToNv21", "([BII)V", (void *)yuv420spToNv21},
        {"nv21ToYuv420sp", "([BII)V", (void *)nv21ToYuv420sp},
        {"nv21ToYuv420spWithMirror", "([BII)V", (void *)nv21ToYuv420spWithMirror},
        {"nv21ToYuv420p", "([BII)V", (void *)nv21ToYuv420p},
        {"nv21ToYuv420pWithMirror", "([BII)V", (void *)nv21ToYuv420pWithMirror},
        {"nativeRotateNV21", "([BIII)V", (void *)nativeRotateNV21},
};

static JNINativeMethod g_lame_methods[] = {
        {"lameInit", "(IIIII)V", (void *)lameInit},
        {"lameEncode", "([S[SI[B)I", (void *)lameEncode},
        {"lameFlush", "([B)I", (void *)lameFlush},
        {"lameClose", "()V", (void *)lameClose},
};

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved) {
    globalJvm = jvm;

    
    JNIEnv *env;
    if(JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4)) {
        LOGE("Get JNIEnv failed");
        return JNI_ERR;
    }
    
    jclass yuvLcs = env->FindClass(yuvClsPath);
    int ret = env->RegisterNatives(yuvLcs, g_yuv_methods, NUM_METHODS(g_yuv_methods));
    if( ret < 0) {
        LOG_E("Register yuv transform natives failed, ret = %d", ret);
    }
    jclass lameLcs = env->FindClass(lameClsPath);
    ret = env->RegisterNatives(lameLcs, g_lame_methods, NUM_METHODS(g_lame_methods));
    if( ret < 0) {
        LOG_E("Register lame mp3 natives failed, ret = %d", ret);
    }
    LOGI("JNI_OnLoad success!");
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* jvm, void* reserved) {
    if(jvm) {
        jvm->DestroyJavaVM();
    }
    globalJvm = nullptr;
    LOGI("JNI_OnUnload success!");
}