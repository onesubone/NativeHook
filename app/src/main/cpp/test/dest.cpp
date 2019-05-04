
#include "../jni_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

jint min(JNIEnv *env, jobject jObj, jint v1, jint v2) {
    jint res = v1 > v2 ? v2 : v1;
    LOGI("Call hook min [%d, %d]->%d", v1, v2, res);
    return res;
}


// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);
    registerMethod(env, "eap/com/nch/MainActivity", "compare2", "(II)I", reinterpret_cast<void *>(min));
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
