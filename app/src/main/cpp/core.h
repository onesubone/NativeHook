//
// Created by yananh on 2019/4/8.
//

#ifndef NATIVEHOOK_HOOK_CORE_H
#define NATIVEHOOK_HOOK_CORE_H

#include "linker/linker.h"
#include <elf.h>
#include <link.h>
#include "HookException.h"

/**
 * 通过符号名称返回目标的索引
 * @param soinfo 目标so
 * @param symbol 符号名称
 * @return 返回目标的索引
 * @throw 如果不存在，则抛出异常
 */
ElfW(Sym) *find_symbol_index_by_name(soinfo *soinfo, const char *symbol, uint32_t *idx);

int add_hook(const char *soname, const char *symbol, void *newValue, void **old_addr_ptr) throw(HookException);

#endif //NATIVEHOOK_HOOK_CORE_H
