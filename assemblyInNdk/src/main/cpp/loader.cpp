//
// Created by yananh on 2019/9/6.
//
#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <stdlib.h>
#include "loader.h"
#include <string>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

std::string deviceABI() {
#if defined(__arm__)
#if defined(__ARM_ARCH_7A__)
#if defined(__ARM_NEON__)
#if defined(__ARM_PCS_VFP)
#define ABI "armeabi-v7a/NEON (hard-float)"
#else
#define ABI "armeabi-v7a/NEON"
#endif
#else
#if defined(__ARM_PCS_VFP)
#define ABI "armeabi-v7a (hard-float)"
#else
#define ABI "armeabi-v7a"
#endif
#endif
#else
#define ABI "armeabi"
#endif
#elif defined(__i386__)
#define ABI "x86"
#elif defined(__x86_64__)
#define ABI "x86_64"
#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
#define ABI "mips64"
#elif defined(__mips__)
#define ABI "mips"
#elif defined(__aarch64__)
#define ABI "arm64-v8a"
#else
#define ABI "unknown"
#endif
    return ABI;
}


//#ifdef __arm__
//int asm_main(void);
//#endif


#ifndef __x86_64__

/**
 * 测试内联汇编，分别根据AArch32架构以及AArch64架构来实现一个简单的减法计算
 * @param a 被减数
 * @param b 减数
 * @return 减法得到的差值
 */
static int __attribute__((naked, pure)) ASM_Sub(int a, int b) {
    /*__arm__ 			armeabi
     * __arm__ 			armeabi-v7
     * __aarch64__		arm64-v8a
     * __i386__			x86
     * __x86_64__ 		x86_64
     */
#ifdef __arm__ // 适用于armeabi和armeabi-v7，32位

    asm(".thumb"); // thumb模式下是16bit和32bit混合模式
    asm(".syntax unified");

    asm("sub r0, r0, r1"); // r0是第一个参数，r1为第二个参数, r0=r0-r1
    asm("add r0, r0, #1"); // 为了区分当前用的是AArch32还是AArch64，这里对于AArch32情况下再加1。r0

    /* [Start] 计算pc的与当前指令的偏移值  */
    asm("sub r1, pc, #4"); // R1中存放STR指令地址
    asm("str pc, r2"); // 用STR指令将PC保存到R0指向的地址单元中，PC=STR指令地址+偏移量（偏移量为8或者12）。
    asm("ldr r0, r2"); // 读取STR指令地址+偏移量的值
    asm("sub r0, r0, r1"); // STR指令地址+偏移量的值减去STR指令的地址，得到偏移量值（8或者12）。
    /* [End] 计算pc的与当前指令的偏移值  */

    asm("bx lr");

#else // AArch64，arm 64模式，arm64-v8a

    asm("sub w0, w0, w1");
    asm("ret");

#endif
}
#else // __x86_64__
extern int ASM_Sub(int a, int b);
#endif

void __attribute__((constructor)) __on_so_init__() {
    // 在JNI_OnLoad方法前调用，可以在这里生成hook文件
    LOGI("%s __attribute__((constructor)) __on_so_init__", LOG_TAG);
    LOGI("Device Runtime ABI %s", deviceABI().c_str());
    LOGI("MyASMTest %d", ASM_Sub(100, 200)); // 输出->MyASMTest -99
}

void __attribute__((destructor)) __on_so_finit__() {
    // so被卸载时触发
    LOGI("%s __attribute__((destructor)) __on_so_finit__", LOG_TAG);
}

// 回调函数 在这里面注册函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    // 判断虚拟机状态是否有问题
    if (vm->GetEnv(reinterpret_cast<void **> (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    //返回jni 的版本
    return JNI_VERSION_1_6;
}


#ifdef __cplusplus
}
#endif