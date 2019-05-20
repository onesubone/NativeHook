//
// Created by yananh on 2019/3/31.
//
#include "jni_loader.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include "elf_linker.h"
#include "elf_dl_hook.h"
#include <unistd.h>
#include <unordered_map>
#include <map>
#include <stdlib.h>
#include <malloc.h>
# include<pthread.h>

static pthread_rwlock_t rwlock;             //声明读写锁

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;
std::map<void *, size_t> mem;
struct _hook_func {
    const char *so_name;

    _hook_func(const char *sn) {
        so_name = sn;
    }

    void *my_calloc(size_t n, size_t size) {
        void *res = calloc(n, size);
        LOGD("calloc %d @%s", size, so_name);
//    pthread_rwlock_rdlock(&rwlock);
//    mem.insert(map<void *, size_t>::value_type(res, size));
//    pthread_rwlock_unlock(&rwlock);
        return res;
    }

    void *my_realloc(void *mem_address, size_t newsize) {
        void *res = realloc(mem_address, newsize);
        LOGD("realloc %d @%s", newsize, so_name);
//    if (mem_address) {
//        pthread_rwlock_wrlock(&rwlock);
//        map<void *, size_t>::iterator it = mem.find(mem_address);
//        if (it != mem.end()) {
//            mem.erase(it);
//        }
//        pthread_rwlock_unlock(&rwlock);
//    }
//
//    if (res != nullptr) {
//        pthread_rwlock_rdlock(&rwlock);
//        mem.insert(map<void *, size_t>::value_type(res, newsize));
//        pthread_rwlock_unlock(&rwlock);
//    }

        return res;
    }

    void *my_malloc(size_t num_bytes) {
        void *res = malloc(num_bytes);
        LOGD("malloc %d @%s", num_bytes, so_name);
//    pthread_rwlock_rdlock(&rwlock);
//    mem.insert(map<void *, size_t>::value_type(res, num_bytes));
//    pthread_rwlock_unlock(&rwlock);
        return res;
    }

    void my_free(void *ptr) {
//    if (ptr != nullptr) {
//        pthread_rwlock_wrlock(&rwlock);
//        map<void *, size_t>::iterator it = mem.find(ptr);
//        if (it != mem.end()) {
//            mem.erase(it);
//        }
//        pthread_rwlock_unlock(&rwlock);
//    }
        LOGD("free %10p @%s", ptr, so_name);
        free(ptr);
    }

    int my_posix_memalign(void **memptr, size_t alignment, size_t size) {
        int res = posix_memalign(memptr, alignment, size);
//    if (0 == res) {
//        pthread_rwlock_rdlock(&rwlock);
//        mem.insert(map<void *, size_t>::value_type(*memptr, size));
//        pthread_rwlock_unlock(&rwlock);
//    }
        LOGD("posix_memalign %d @%s", size, so_name);
        return res;
    }

    void *my_memalign(size_t alignment, size_t size) {
        void *res = memalign(alignment, size);
        LOGD("memalign %d @%s", size, so_name);
//    if (res) {
//        pthread_rwlock_rdlock(&rwlock);
//        mem.insert(map<void *, size_t>::value_type(res, size));
//        pthread_rwlock_unlock(&rwlock);
//    }
        return res;
    }
};

//void *my_aligned_alloc(size_t alignment, size_t size) {
//    void *res = aligned_alloc(alignment, size);
//    if (res) {
//        mem.insert(map<void *, size_t>::value_type(res, size));
//    }
//    return res;
//}
// since c++17
//void *my_aligned_alloc(size_t alignment, size_t size) {
//    void *res = std::aligned_alloc(alignment, size);
//    if(res) {
//        mem.insert(map<void *, size_t>::value_type(res, size));
//    }
//    return res;
//}


void *my_calloc(size_t n, size_t size) {
    void *res = calloc(n, size);
    LOGD("calloc %d", size);
    return res;
}

void *my_realloc(void *mem_address, size_t newsize) {
    void *res = realloc(mem_address, newsize);
    LOGD("realloc %d", newsize);
    return res;
}

void *my_malloc(size_t num_bytes) {
    void *res = malloc(num_bytes);
    LOGD("malloc %d", num_bytes);
    return res;
}

void my_free(void *ptr) {
    LOGD("free %10p", ptr);
    free(ptr);
}

int my_posix_memalign(void **memptr, size_t alignment, size_t size) {
    int res = posix_memalign(memptr, alignment, size);
    LOGD("posix_memalign %d", size);
    return res;
}

void *my_memalign(size_t alignment, size_t size) {
    void *res = memalign(alignment, size);
    LOGD("memalign %d", size);
    return res;
}

static void print_leaked_mem() {
//    LOGI("----------- %s ------------", "print_leaked_mem");
//    uint32_t t = 0;
//    for (auto iter = mem.begin(); iter != mem.end(); iter++) {
//        LOGI("Leak %u @%10p", iter->second, (int32_t) iter->first);
//        t += iter->second;
//    }
//    LOGI("Total: %u", t);
}

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

void printEnviron(char *tag) {
    char **p = environ;
    LOGE("char**ptr(%s)@%10p", tag, p);
    if (nullptr == p) {
        return;
    }
    while (*p) {
        LOGE("%s: %s", tag, *p);
        p++;
    }
}
char *new_environ[] = {"Hello ", "World!", nullptr};

//std::map<void *, size_t> mem;
#define NEW_HOOK_SYM(N, S, V) new hook_symbol{N, S, (void *) V}
//#define DEFINEHOOK( SONAME, NAME,) \
//    typedef RET_TYPE (* NAME ## _t)ARGS; \
//    RET_TYPE hook_ ## NAME ARGS

static void hook(JNIEnv *env, jobject jObj) {
    pthread_rwlock_init(&rwlock, nullptr);

    std::vector<hook_symbol *> sym_list;
    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "malloc", (void *) (my_malloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "malloc", (void *) (my_malloc)));

    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "realloc", (void *) (my_realloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "realloc", (void *) (my_realloc)));

    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "calloc", (void *) (my_calloc)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "calloc", (void *) (my_calloc)));

    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "posix_memalign", (void *) (my_posix_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "posix_memalign", (void *) (my_posix_memalign)));

    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "memalign", (void *) (my_memalign)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "memalign", (void *) (my_memalign)));

    sym_list.push_back(NEW_HOOK_SYM("libijkffmpeg.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libijkplayer.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libijksdl.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libGLES_mali.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libandroid_runtime.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libutils.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libwilhelm.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libnetdutils.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libcrypto.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libnativehelper.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libdebuggerd_client.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libui.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libgui.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libsensor.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libsqlite.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libziparchive.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libhardware.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libselinux.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libmedia.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libicuuc.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libjpeg.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libimg_utils.so", "free", (void *) (my_free)));
    sym_list.push_back(NEW_HOOK_SYM("libnativebridge.so", "free", (void *) (my_free)));

    add_hook(&sym_list);
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
    registerMethod(env, "com/eap/nh/lib/NativeHook", "printLeakedMem", "(Ljava/lang/String;)V",
                   (void *) (print_leaked_mem));
//    registerMethod(env, "com/eap/nh/lib/NativeHook", "printDynamicLib", "(Ljava/lang/String;)V", (void *) (printDynamicLib));
    //返回jni 的版本
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
