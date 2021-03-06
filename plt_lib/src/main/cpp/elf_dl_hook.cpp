//
// Created by yananh on 2019/4/30.
//
#include "elf_dl_hook.h"
#include "elf_linker.h"
#include <vector>
#include <sys/mman.h>
#include <elf.h>
#include <linux/elf.h>
#include <link.h>
#include "jni_loader.h"
#include <string>
#include <unistd.h>

using namespace std;

static int do_hook(soinfo *si, const char *so_name, ElfW(Addr) addr, void *newValue, void **old_addr_ptr) {
    void **relocate_ptr = (void **) addr;
    if (*relocate_ptr == newValue) {
        LOGW("do hook same address %10p, do nothing!", newValue);
        return 0;
    }
    if (old_addr_ptr) *old_addr_ptr = *relocate_ptr;
    size_t pageSize = (size_t) sysconf(_SC_PAGESIZE);
    LOGD("_SC_PAGESIZE %u", pageSize);
    uintptr_t originPtr = (uintptr_t) (addr);
    void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
    segment_attr *attr = lookup_segment_attr(si, (ElfW(Addr)) (aligned_pointer));
    if (attr) {
        mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
        LOGI("do_hoo: %s@%10p Value[%10p -> %10p]，flag=%d", so_name, addr, *relocate_ptr, newValue, attr->flag);
        *relocate_ptr = newValue;
        __builtin___clear_cache((char *) aligned_pointer, ((char *) aligned_pointer) + pageSize);
        mprotect(aligned_pointer, pageSize, attr->flag);
    } else {
        mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
        LOGI("do_hoo: %s@%10p Value[%10p -> %10p]", so_name, addr, *relocate_ptr, newValue);
        *relocate_ptr = newValue;
        __builtin___clear_cache((char *) aligned_pointer, ((char *) aligned_pointer) + pageSize);
    }
    return 0;
}

static int hook_spec(soinfo *si, const char *symbol, void *newValue, void **old_addr_ptr) throw(std::string) {
    uint32_t idx = 0;
    ElfW(Sym) *sym = si->lookup_symbol(symbol, &idx);
    LOGI("find_symbol_by_name %s@%s idx: %d", symbol, si->strtab_ + si->so_name, idx);

    if (!sym && !si->is_gnu_hash()) { //
        // GNU hash table doesn't contain relocation symbols.
        // We still need to search the .rel.plt section for the symbol
        return 0;
    }

    //replace for .rel(a).plt
#if defined(USE_RELA)
    const char *plt_name = ".rela.plt";
    // loop reloc table to find the symbol by index, replace for .rela.plt
    for (ElfW(Rela) *rel_plt = si->plt_rela_;
         rel_plt != nullptr && rel_plt < si->plt_rela_ + si->plt_rela_count_; ++rel_plt) {
#else
        const char *plt_name = ".rel.plt";
        // loop reloc table to find the symbol by index, replace for .rel.plt
        for (ElfW(Rel) *rel_plt = si->plt_rel_; rel_plt != nullptr && rel_plt < si->plt_rel_ + si->plt_rel_count_; ++rel_plt) {
#endif
        // gnu hash同elf hash不同，gnu模式下，idx可能没有找到该字符表的索引
        uint32_t rel_idx = ELF_R_SYM(rel_plt->r_info); // rel.plt等符号在符号表(.dynsym)中的索引下标
        // 检查符号表的索引和重定位表中表示的索引是否相等
        if ((idx == rel_idx || (idx == 0 && si->is_gnu_hash())) /* 字符表中索引下标匹配，如果gnu hash没有搜索到，则直接匹配方法名*/
            && strcmp(si->strtab_ + (si->symtab_ + rel_idx)->st_name, symbol) == 0) {
            if (_ELF_R_GENERIC_JUMP_SLOT == ELF_R_TYPE(rel_plt->r_info)) {
                LOGD("found [%s] at %s offset: %p\n", symbol, plt_name, (void *) rel_plt->r_offset);
                if (rel_plt->r_offset < 0) {
                    LOGE("relocate offset:%u less than 0!", rel_plt->r_offset);
                    throw std::string("hook_and_replace: relocate offset less than 0!");
                }
                ElfW(Addr) reloc = rel_plt->r_offset + si->bias_addr; // r_offset->重定位入口偏移;
                return do_hook(si, si->strtab_ + si->so_name, reloc, newValue, old_addr_ptr);
            } else {
                LOGE("%s idx %u Expected R_ARM_JUMP_SLOT, found %u", plt_name, rel_idx, ELF_R_TYPE(rel_plt->r_info));
                throw std::string("hook_and_replace: only support R_ARM_JUMP_SLOT type!Plz check mode");
            }
        }
    }

    //replace for .rel(a).dyn，适用于全局变量等
#if defined(USE_RELA)
    const char *dyn_name = ".rela.dyn";
    // loop reloc table to find the symbol by index, replace for .rela.dyn
    for (ElfW(Rela) *rel = si->rela_; rel != nullptr && rel < si->rela_ + si->rela_count_; ++rel) {
#else
        const char *dyn_name = ".rel.dyn";
        // loop reloc table to find the symbol by index, replace for .rel.dyn
        for (ElfW(Rel) *rel = si->rel_; rel != nullptr && rel < si->rel_ + si->rel_count_; ++rel) {
#endif
        uint32_t rel_idx = ELF_R_SYM(rel->r_info);
        // 检查符号表的索引和重定位表中表示的索引是否相等
        if (idx == rel_idx && strcmp(si->strtab_ + (si->symtab_ + rel_idx)->st_name, symbol) == 0) {
            if (_ELF_R_GENERIC_GLOB_DAT == ELF_R_TYPE(rel->r_info) || _ELF_R_GENERIC_ABS == ELF_R_TYPE(rel->r_info)) {
                LOGD("found [%s]at %s offset: %p\n", symbol, dyn_name, (void *) rel->r_offset);
                if (rel->r_offset < 0) {
                    LOGE("relocate offset:%u less than 0!", rel->r_offset);
                    throw std::string("hook_and_replace: relocate offset less than 0!");
                }
                ElfW(Addr) reloc = rel->r_offset + si->bias_addr; // r_offset->重定位入口偏移;
                return do_hook(si, si->strtab_ + si->so_name, reloc, newValue, old_addr_ptr);
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

extern "C" int
dl_iterate_phdr(int (*__callback)(struct dl_phdr_info *, size_t, void *), void *__data)__attribute__ ((weak));

static std::vector<hook_symbol *> *filter_hook_symbols(const char *so_name_or_path, std::vector<hook_symbol *> *all) {
    if (!so_name_or_path || 0 == endWith("linker", so_name_or_path) || 0 == endWith("linker.so", so_name_or_path))
        return nullptr;

    std::vector<hook_symbol *> *res = new std::vector<hook_symbol *>;
    for (std::vector<hook_symbol *>::iterator itr = all->begin(); itr != all->end(); ++itr) {
        hook_symbol *entry = *itr;
        if (!entry) continue;
        if (!entry->so_name) {
            if (0 == endWith("libnh_plt.so", so_name_or_path)) {
                continue;
            }
            res->push_back(entry);
            continue;
        }
        if (strcmp(entry->so_name, so_name_or_path) == 0 || endWith(entry->so_name, so_name_or_path) == 0) {
            res->push_back(entry);
        }
    }
    return res;
}

int add_hook(std::vector<hook_symbol *> &all) {
    if (all.empty()) {
        LOGI("%s", "Not Found Hook_Symbol, do nothing!");
        return 0;
    }

    auto travel_si_callback = [&](std::unique_ptr<soinfo> si_p) {
        if (!si_p) {
            return;
        }
        // 所有命中本so的需要Hook的符号
        std::vector<hook_symbol *> *symbols = filter_hook_symbols((char *) (si_p->strtab_ + si_p->so_name), &all);
        if (!symbols || symbols->empty()) return;
        for (std::vector<hook_symbol *>::iterator itr = symbols->begin(); itr != symbols->end(); ++itr) {
            hook_spec(&(*si_p), (*itr)->symbol_name, (*itr)->new_value, nullptr);
        }
        delete symbols;
    };
    travel_all_loaded_so(travel_si_callback);
    return 0;
}