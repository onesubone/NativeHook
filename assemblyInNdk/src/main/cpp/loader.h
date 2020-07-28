//
// Created by yananh on 2019/9/6.
//

#ifndef RUBICK_LOADER_H
#define RUBICK_LOADER_H

#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <stdlib.h>

#define LOG_TAG "AssemblyInNdk"

#define LOGD(F, ...) __android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, F, __VA_ARGS__ )
#define LOGI(F, ...) __android_log_print( ANDROID_LOG_INFO, LOG_TAG, F, __VA_ARGS__ )
#define LOGW(F, ...) __android_log_print( ANDROID_LOG_WARN, LOG_TAG, F, __VA_ARGS__ )
#define LOGE(F, ...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, F, __VA_ARGS__ )
#define LOGF(F, ...) __android_log_print( ANDROID_LOG_FATAL, LOG_TAG, F, __VA_ARGS__ )

#endif //RUBICK_LOADER_H
