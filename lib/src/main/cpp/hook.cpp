//
// Created by yananh on 2019/3/31.
//
#include "jni_loader.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include "elf_linker.h"
#include "elf_hook.h"
#include <unistd.h>

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

int myConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGI("Call connect method sockfd:%d", sockfd);
    return connect(sockfd, addr, addrlen);
}

ssize_t myRecv(int sockfd, void *buf, size_t nbytes, int flags) {
    LOGI("Call myRecv method sockfd:%d， size：%u", sockfd, nbytes);
    return recv(sockfd, buf, nbytes, flags);
}

ssize_t myWrite(int filedes, void *buf, size_t nbytes) {
    LOGI("Call myWrite method filedes:%d， size：%u", filedes, nbytes);
    return write(filedes, buf, nbytes);
}

ssize_t myRead(int filedes, void *buf, size_t nbytes) {
    LOGI("Call myRead method filedes:%d， size：%u", filedes, nbytes);
    return read(filedes, buf, nbytes);
}

static void hook(JNIEnv *env, jobject jObj) {
//    add_hook("libcronet.72.0.3626.0.so", "recv", ptr, nullptr);
//    add_hook("libcronet.72.0.3626.0.so", "recv", (void *) (myRecv), nullptr);
//    add_hook("libcronet.72.0.3626.0.so", "socket", (void *) (mySocket), nullptr);
    add_hook(nullptr, "accept", (void *) (myAccept), nullptr);
    add_hook(nullptr, "write", (void *) (myWrite), nullptr);
    add_hook(nullptr, "read", (void *) (myRead), nullptr);
}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != NULL);
    hook(env, nullptr);
//    registerMethod(env, "com/eap/nh/lib/NativeHook", "printDynamicLib", "(Ljava/lang/String;)V", (void *) (printDynamicLib));
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
