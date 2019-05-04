//
// Created by yananh on 2019/2/8.
//

#include "hook.h"

#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

#ifdef __cplusplus
extern "C" {  // only need to export C interface if used by C++ source code
#endif
#define VPTR void*

static void *_symPtr(shared_ptr<string> soName, shared_ptr<string> symName) throw(runtime_error) {
    void *handle = dlopen(soName->c_str(), RTLD_NOW);
    if (!handle) {
        char *error = dlerror();
        LOGE("native library [%s] open error %s\n", soName->c_str(), error);
        throw std::runtime_error("native library [" + (*soName) + "] open error " + string(error));
    }

    void *ptr = dlsym(handle, symName->c_str());
    if (!ptr) {
        char *error = dlerror();
        LOGE("native library [%s] dlsym error %s\n", soName->c_str(), error);
        throw std::runtime_error("native library [" + (*soName) + "] dlsym error " + string(error));
    }
    dlclose(handle);
    return ptr;
}

void hook(shared_ptr<string> soName, shared_ptr<string> fieldName,
          shared_ptr<string> destSo, shared_ptr<string> varName) throw(runtime_error) {
    void *originSymPtr = _symPtr(soName, fieldName);
    void *destSymPtr = _symPtr(destSo, varName);

#if 0
    if (sizeof(void *) == 4) {
        uint32_t pageSize = (uint32_t)sysconf(_SC_PAGESIZE);
        LOGD("In Memory, PageSize %ll", pageSize);
        uint32_t originPtr = (uint32_t) (originSymPtr);
        void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
        mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
        *((uint32_t *) originPtr) = reinterpret_cast<uint32_t>(destSymPtr);
    } else if (sizeof(void *) == 8) {
        uint64_t pageSize = (uint64_t)sysconf(_SC_PAGESIZE);
        LOGD("In Memory, PageSize %l", pageSize);
        uint64_t originPtr = (uint64_t) (originSymPtr);
        void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
        mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
        *((uint64_t *) originPtr) = reinterpret_cast<uint64_t>(destSymPtr);
    }
#endif

    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
    LOGD("In Memory, PageSize %ul", pageSize);
    uintptr_t originPtr = (uintptr_t) (originSymPtr);
    void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
    mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
    *((uintptr_t *) originPtr) = reinterpret_cast<uintptr_t>(destSymPtr);
}

static jint _hook(JNIEnv *env, jobject, jstring jSoName, jstring jSymName, jstring jDestSoName, jstring jDestSymName) {
    // 获取字符串指针，必须使用指针，不能使用char strContent[],因为GetStringUTFChars()返回值为const char *;
    const char *soName = env->GetStringUTFChars(jSoName, JNI_FALSE);
    const char *symName = env->GetStringUTFChars(jSymName, JNI_FALSE);
    const char *destSoName = env->GetStringUTFChars(jDestSoName, JNI_FALSE);
    const char *destSymName = env->GetStringUTFChars(jDestSymName, JNI_FALSE);

#define string_share(ARGV) make_shared<string>(ARGV)
    try {
        hook(string_share(soName), string_share(symName), string_share(destSoName), string_share(destSymName));
    } catch (runtime_error error) {
        return -1;
    }

    env->ReleaseStringUTFChars(jSoName, soName);
    env->ReleaseStringUTFChars(jSymName, symName);
    env->ReleaseStringUTFChars(jDestSoName, destSoName);
    env->ReleaseStringUTFChars(jDestSymName, destSymName);
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
    registerMethod(env, "eap/com/nch/NativeHook", "hook",
                   "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I", (VPTR) _hook);
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif







