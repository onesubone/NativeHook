//
// Created by yananh on 2019/3/31.
//
#include "linker/linker.h"
#include "jni_loader.h"
#include <sys/mman.h>
#include "elf_log.h"
#include "core.h"
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

int mySocket(int protofamily, int type, int protocol) {
    LOGI("Call socket method protofamily:%d, type:%d, protocol:%d", type, type, protocol);
    return socket(protofamily, type, protocol);
}

int myAccept(int sockfd, struct sockaddr *restric, socklen_t *restrict) {
    LOGI("Call accept method sockfd:%d", sockfd);
    return accept(sockfd, restric, restrict);
}

int myConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    LOGI("Call connect method sockfd:%d", sockfd);
    return connect(sockfd, addr, addrlen);
}

ssize_t myRecv(int sockfd, void *buf, size_t nbytes, int flags) {
    LOGI("Call myRecv method sockfd:%d， size：%u", sockfd, nbytes);
    return recv(sockfd, buf, nbytes, flags);
}
static jint printSo(JNIEnv *env, jobject jObj, jstring soName) {
    void *ptr = (void *) (myRecv);
    LOGI("connect address 0x%X",&recv);
    add_hook("libcronet.72.0.3626.0.so", "recv", ptr, nullptr);
    ptr = (void *) (mySocket);
    add_hook("libcronet.72.0.3626.0.so", "socket", ptr, nullptr);
    return 0;
}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);
    registerMethod(env, "eap/com/nch/NativeHook", "soInfoPrint", "(Ljava/lang/String;)V", (void *) (printSo));
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
