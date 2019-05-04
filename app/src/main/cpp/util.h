// Copyright (c) 2018-present, iQIYI, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

// Created by caikelun on 2018-04-11.

#ifndef XH_UTILS_H
#define XH_UTILS_H 
#include <sys/mman.h>


#if defined(__LP64__)
#define UTIL_FMT_LEN     "16"
#define UTIL_FMT_X       "llx"
#else
#define UTIL_FMT_LEN     "8"
#define UTIL_FMT_X       "x"
#endif

#define UTIL_FMT_FIXED_X UTIL_FMT_LEN UTIL_FMT_X
#define UTIL_FMT_FIXED_S UTIL_FMT_LEN "s"

int util_get_mem_protect(uintptr_t addr, size_t len, const char *pathname, unsigned int *prot);
int util_get_addr_protect(uintptr_t addr, const char *pathname, unsigned int *prot);
int util_set_addr_protect(uintptr_t addr, unsigned int prot);
void util_flush_instruction_cache(uintptr_t addr);

#endif
