//
// Created by yananh on 2019/3/31.
//
#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct P {
    char c[128];
};

void invoke(JNIEnv *env, jobject obj) {

}

void no_param_no_res() {

}

int no_param_int_res() {
    return 2;
}

int int_param_int_res(int p) {
    return p - 2;
}

int multi_param_int_res(int p, double p1) {
    return p - 3;
}

P no_param_complex_ret() {
    P p;
    return p;
}

P int_param_complex_ret(int i) {
    P p;
    return p;
}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass clazz;
    //找到声明native方法的类
    clazz = env->FindClass("open/qukan/com/simpleso/SoInvoker");
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    static JNINativeMethod methods[] = {
            {"invoke", "(V)V", (void *) invoke}
    };
    //注册函数 参数：java类 所要注册的函数数组 注册函数的个数
    if (env->RegisterNatives(clazz, methods, 1) < 0) {
        return JNI_FALSE;
    }
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
