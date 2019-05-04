//
// Created by yananh on 2019/3/28.
//
#include "../jni_loader.h"
#include "Container.h"

Container::Container() {
    inner1 = 1111;
    innerVar2 = 2222.222;
    outVar1 = 3333;
    outVar = 4444;
    outVar2 = 555;
    outVar3 = 6;
    LOGI("Container::Container %s", "");
}

int Container::innerFunc() {
    LOGI("Container::innerFunc %s", "");
    return 1033;
}

int Container::innerFunc1() {
    LOGI("Container::innerFunc1 %s", "");
    return 101;
}

int Container::outFunc() {
    LOGI("Container::outFunc %s", "");
    return 1067;
}


int Container::outFunc1() {
    LOGI("Container::outFunc1 %s", "");
    return 1067;
}