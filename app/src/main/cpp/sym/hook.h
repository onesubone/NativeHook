//
// Created by yananh on 2019/2/8.
//

#pragma once

#include "../jni_loader.h"
#include <functional>
#include <stdlib.h>
#include <dlfcn.h>

#define LOG_TAG "NativeHook"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

void hook(shared_ptr<string> soName, shared_ptr<string> fieldName, shared_ptr<string> destSo,
          shared_ptr<string> varName) throw(runtime_error);

#ifdef __cplusplus
}
#endif