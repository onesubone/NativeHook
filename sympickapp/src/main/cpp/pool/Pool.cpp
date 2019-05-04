#include "../jni_loader.h"
#include "Container.h"

Container *out = new Container();

#ifdef __cplusplus
extern "C" {
#endif

void empty() {

}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);
    registerMethod(env, "eap/com/sympickapp/NativeBridge", "bridge", "()V", reinterpret_cast<void *>(empty));
    // 返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif





