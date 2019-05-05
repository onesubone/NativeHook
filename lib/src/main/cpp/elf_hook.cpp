//
// Created by yananh on 2019/4/30.
//
#include "elf_hook.h"
#include "elf_linker.h"
#include <vector>
#include <sys/mman.h>
#include <elf.h>
#include <linux/elf.h>
#include <link.h>
#include "jni_loader.h"
#include <string>
#include <unistd.h>

static int do_hook(ElfW(Addr) addr, void *newValue, void **old_addr_ptr) {
    void **relocate_ptr = (void **) addr;
    if (*relocate_ptr == newValue) {
        LOGW("do hook same address %10p, do nothing!", newValue);
        return 0;
    }
    if (old_addr_ptr) *old_addr_ptr = *relocate_ptr;
//    LOGI("do hook old address %10p, *addr=%10p,**addr=%10p", addr, *relocate_ptr, **(void ***)relocate_ptr);

    size_t pageSize = (size_t) sysconf(_SC_PAGESIZE);
    LOGD("_SC_PAGESIZE %u", pageSize);
    uintptr_t originPtr = (uintptr_t) (addr);
    void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
    mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
//    *((uintptr_t *) addr) = reinterpret_cast<uintptr_t>(newValue);
    LOGI("hook from %10p to %10p", *relocate_ptr, newValue);
    *relocate_ptr = newValue;
    __builtin___clear_cache((char *) aligned_pointer, ((char *) aligned_pointer) + pageSize);
    return 0;
}


static int hook_spec(soinfo *si, const char *symbol, void *newValue, void **old_addr_ptr) throw(std::string) {
    uint32_t idx = 0;
    ElfW(Sym) *sym = si->lookup_symbol(symbol, &idx);
    LOGD("find_symbol_by_name %s idx: %d", symbol, idx);

    if (!sym && !si->is_gnu_hash()) { //
        // GNU hash table doesn't contain relocation symbols.
        // We still need to search the .rel.plt section for the symbol
        return 0;
    }

    //replace for .rel(a).plt
#if defined(USE_RELA)
    const char *plt_name = ".rela.plt";
    // loop reloc table to find the symbol by index, replace for .rela.plt
    for (ElfW(Rela) *rel = si->plt_rela_; rel_plt!= nullptr && rel < si->plt_rela_ + si->plt_rela_count_; ++rel) {
#else
    const char *plt_name = ".rel.plt";
    // loop reloc table to find the symbol by index, replace for .rel.plt
    for (ElfW(Rel) *rel_plt = si->plt_rel_;
         rel_plt != nullptr && rel_plt < si->plt_rel_ + si->plt_rel_count_; ++rel_plt) {
#endif
        // gnu hash同elf hash不同，gnu模式下，idx可能没有找到该字符表的索引
        uint32_t rel_idx = ELF_R_SYM(rel_plt->r_info); // rel.plt等符号在符号表(.dynsym)中的索引下标
        // 检查符号表的索引和重定位表中表示的索引是否相等
        if ((idx == rel_idx || (idx == 0 && si->is_gnu_hash())) /* 字符表中索引下标匹配，如果gnu hash没有搜索到，则直接匹配方法名*/
            && strcmp(si->strtab_ + (si->symtab_ + rel_idx)->st_name, symbol) == 0) {
            if (R_ARM_JUMP_SLOT == ELF_R_TYPE(rel_plt->r_info)) {
                LOGD("found [%s] at %s offset: %p\n", symbol, plt_name, (void *) rel_plt->r_offset);
                if (rel_plt->r_offset < 0) {
                    LOGE("relocate offset:%u less than 0!", rel_plt->r_offset);
                    throw std::string("hook_and_replace: relocate offset less than 0!");
                }
                ElfW(Addr) reloc = rel_plt->r_offset + si->bias_addr; // r_offset->重定位入口偏移;
                return do_hook(reloc, newValue, old_addr_ptr);
            } else {
                LOGW("rel(a).plt idx %u Expected R_ARM_JUMP_SLOT, found %u", rel_plt, ELF_R_TYPE(rel_plt->r_info));
                throw std::string("hook_and_replace: only support R_ARM_JUMP_SLOT type!Plz check mode");
            }
        }
    }

    //replace for .rel(a).dyn，全局变量
#if defined(USE_RELA)
    const char *dyn_name = ".rela.dyn";
    // loop reloc table to find the symbol by index, replace for .rela.dyn
    for (ElfW(Rela) rel = si->rela_; rel!= nullptr && rel < si->rela_ + si->rela_count_; ++rel) {
#else
    const char *dyn_name = ".rel.dyn";
    // loop reloc table to find the symbol by index, replace for .rel.dyn
    for (ElfW(Rel) *rel = si->rel_; rel != nullptr && rel < si->rel_ + si->rel_count_; ++rel) {
#endif
        uint32_t rel_idx = ELF_R_SYM(rel->r_info);
        // 检查符号表的索引和重定位表中表示的索引是否相等
        if (idx == rel_idx && strcmp(si->strtab_ + (si->symtab_ + rel_idx)->st_name, symbol) == 0) {
            if (R_ARM_GLOB_DAT == ELF_R_TYPE(rel->r_info) || R_ARM_ABS32 == ELF_R_TYPE(rel->r_info)) {
                LOGD("found [%s]at %s offset: %p\n", symbol, dyn_name, (void *) rel->r_offset);
                if (rel->r_offset < 0) {
                    LOGE("relocate offset:%u less than 0!", rel->r_offset);
                    throw std::string("hook_and_replace: relocate offset less than 0!");
                }
                ElfW(Addr) reloc = rel->r_offset + si->bias_addr; // r_offset->重定位入口偏移;
                return do_hook(reloc, newValue, old_addr_ptr);
            } else {
                LOGW("Expected R_ARM_GLOB_DAT|R_ARM_ABS32, found 0x%X", ELF_R_SYM(rel->r_info));
                throw std::string("hook_and_replace:only support R_ARM_GLOB_DAT&R_ARM_ABS32 type!Plz check mode");
            }
        }
    }
    return -1;

//    if (si->version_ < 2) {
//        return -1;
//    }
//    //replace for .rel(a).dyn，全局变量
//#if defined(USE_RELA)
//    const char *rel_android_name = ".rela.android";
//    // loop reloc table to find the symbol by index, replace for .rela.plt
//    for (ElfW(Rela) rel = si->rela_; rel < si->rela_ + si->rela_count_; ++rel) {
//#else
//    const char *rel_android_name = ".rel.android";
//    // loop reloc table to find the symbol by index, replace for .rel.plt
//    for (ElfW(Rel) *rel = si->rel_; rel < si->rel_ + si->plt_rel_count_; ++rel) {
//#endif
//        // 检查符号表的索引和重定位表中表示的索引是否相等
//        if (idx == ELF_R_TYPE(rel->r_info) && strcmp(si->strtab_ + (si->symtab_ + idx)->st_name, symbol) == 0) {
//            switch (ELF_R_SYM(rel->r_info)) {
//                case R_ARM_GLOB_DAT: // .rel.dyn 只支持R_ARM_GLOB_DAT和R_ARM_ABS32模式
//                case R_ARM_ABS32: // .rel.dyn 只支持R_ARM_GLOB_DAT和R_ARM_ABS32模式
//                    LOGD("found %s at %s offset: %p\n", symbol, dyn_name, (void *) rel->r_offset);
//                    if (rel->r_offset < 0) {
//                        LOGE("relocate offset:%u less than 0!", rel->r_offset);
//                        throw HookException("hook_and_replace: "dyn_name"relocate offset less than 0!");
//                    }
//                    ElfW(Addr) reloc = rel->r_offset + si->load_bias; // r_offset->重定位入口偏移;
//                    return replace(reloc, newValue, old_addr_ptr);
//                default:
//                    LOGW("Expected R_ARM_JUMP_SLOT, found 0x%X", ELF_R_SYM(rel->r_info));
//                    throw HookException(
//                            "hook_and_replace:"table_name"only support R_ARM_GLOB_DAT&R_ARM_ABS32 type!Plz check mode");
//            }
//        }
//    }

//    return 0;
}

int add_hook(const char *soname, const char *symbol, void *newValue, void **old_addr_ptr) throw(std::string) {
    struct lookup_result *result = new lookup_result();
    lookup_soinfo(soname, result);
    if (!result->success) {
        LOGE("%s", "");
        return -1;
    }
    for (auto itor = result->result.begin(); itor != result->result.end(); ++itor) {
        // 输出毫无意义
        hook_spec(*itor, symbol, newValue, old_addr_ptr);
    }
    return 0;
}