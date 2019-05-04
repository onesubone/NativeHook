//
// Created by yananh on 2019/2/16.
//
#include "jni_loader.h"
#include <jni.h>

// 此函数通过调用JNI中 RegisterNatives 方法来注册我们的函数
int registerMethod(JNIEnv *env, const char *className, const char *methodName, const char *signature, void *fnPtr) {
    jclass clazz = env->FindClass(className);
    // 找到声明native方法的类
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    //注册函数 参数：java类 所要注册的函数数组 注册函数的个数
    if (env->RegisterNatives(clazz, new JNINativeMethod{methodName, signature, fnPtr}, 1) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}














