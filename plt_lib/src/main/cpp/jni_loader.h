//
// Created by yananh on 2019/2/2.
//

#pragma once

#include <jni.h>
#include <string.h>
#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <stdlib.h>
#include <functional>

#define LOG_TAG "NativeHook"

using namespace std;

#define LOGD(F, ...) __android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, F, __VA_ARGS__ )
#define LOGI(F, ...) __android_log_print( ANDROID_LOG_INFO, LOG_TAG, F, __VA_ARGS__ )
#define LOGW(F, ...) __android_log_print( ANDROID_LOG_WARN, LOG_TAG, F, __VA_ARGS__ )
#define LOGE(F, ...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, F, __VA_ARGS__ )
#define LOGF(F, ...) __android_log_print( ANDROID_LOG_FATAL, LOG_TAG, F, __VA_ARGS__ )

inline int endWith(const char *src, const char *dest) {
    size_t src_size = strlen(src);
    size_t dest_size = strlen(dest);

    if (src_size > dest_size) {
        return -1;
    }

    return strcmp(src, dest + (dest_size - src_size));
}

#ifdef __cplusplus
extern "C" {
#endif

// 此函数通过调用JNI中 RegisterNatives 方法来注册我们的函数
int registerMethod(JNIEnv *env, const char *className, const char *methodName, const char *signature, void *fnPtr);

#ifdef __cplusplus
}
#endif