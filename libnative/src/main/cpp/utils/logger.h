

#ifndef SUPERKVM_LOGGER_H
#define SUPERKVM_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <android/log.h>

#define TAG "SuperKVM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, "%s", __VA_ARGS__)
#define LOG_I(format, ...) __android_log_print(ANDROID_LOG_INFO, TAG, format, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, "%s", __VA_ARGS__)
#define LOG_E(format, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, format, __VA_ARGS__)

#ifdef __cplusplus
};
#endif
#endif 
