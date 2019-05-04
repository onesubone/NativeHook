#include "../jni_loader.h"

#ifdef __cplusplus
extern "C" {
#endif


jint max(JNIEnv *env, jobject jObj, jint v1, jint v2) {
    jint res = v1 > v2 ? v1 : v2;
    LOGI("Call max [%d, %d]->%d", v1, v2, res);
    return res;
}

typedef jint(*work_prt)(JNIEnv *, jobject, jint, jint);
work_prt compare_ptr = max;

static jint _doCompare(JNIEnv *env, jobject jObj, jint v1, jint v2) {
    return compare_ptr(env, jObj, v1, v2);
}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);
    registerMethod(env, "eap/com/nch/MainActivity", "compare", "(II)I", reinterpret_cast<void *>(_doCompare));
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif





