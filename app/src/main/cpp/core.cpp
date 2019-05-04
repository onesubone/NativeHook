//
// Created by yananh on 2019/4/8.
//
#include "jni_loader.h"
#include "core.h"
#include "elf_log.h"
#include <sys/mman.h>
#include <unordered_map>

static uint32_t elf_hash(const char *sym_name) {
    const unsigned char *name = (const unsigned char *) sym_name;
    unsigned h = 0, g;
    while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        h ^= g;
        h ^= g >> 24;
    }
    return h;
}

static uint32_t gnu_hash(const char *sym_name) {
    uint32_t h = 5381;
    const uint8_t *name = reinterpret_cast<const uint8_t *>(sym_name);
    while (*name != 0) {
        h += (h << 5) + *name++; // h*33 + c = h + h * 32 + c = h + h << 5 + c
    }
    return h;
}

static ElfW(Sym) *soinfo_elf_lookup(struct soinfo *si, uint32_t hash, const char *name, uint32_t *idx) {
    ElfW(Sym) *symtab = si->symtab_;
    const char *strtab = si->strtab_;
    unsigned n;

    for (n = si->bucket_[hash % si->nbucket_]; n != 0; n = si->chain_[n]) {
        ElfW(Sym) *s = symtab + n;
        if (strcmp(strtab + s->st_name, name) == 0) {
            *idx = n;
            return s;
        }
    }

    return NULL;
}

static ElfW(Sym) *soinfo_gnu_lookup(struct soinfo *si, uint32_t hash, const char *name, uint32_t *idx) {
    uint32_t h2 = hash >> si->gnu_shift2_;

    uint32_t bloom_mask_bits = sizeof(uintptr_t) * 8;
    uint32_t word_num = (hash / bloom_mask_bits) & si->gnu_maskwords_;
    uintptr_t bloom_word = si->gnu_bloom_filter_[word_num];

    if ((1 & (bloom_word >> (hash % bloom_mask_bits)) & (bloom_word >> (h2 % bloom_mask_bits))) == 0) {
        return NULL;
    }

    uint32_t n = si->gnu_bucket_[hash % si->gnu_nbucket_];
    if (n == 0) {
        return NULL;
    }

    do {
        ElfW(Sym) *s = si->symtab_ + n;

        if (((si->gnu_chain_[n] ^ hash) >> 1) == 0 &&
            strcmp(si->strtab_ + s->st_name, name) == 0) {
            *idx = n;
            return s;
        }
    } while ((si->gnu_chain_[n++] & 1) == 0);

    return NULL;
}

static ElfW(Sym) *soinfo_gnu_lookup_undef(struct soinfo *si, const char *name, uint32_t *idx) {
    unsigned symtab_cnt = IS_GNU_HASH(si) ? si->nchain_ : si->nchain_;
    for (uint32_t i = 0; i < symtab_cnt; ++i) {
        ElfW(Sym) *sym = si->symtab_ + i;
        if (0 == strcmp(si->strtab_ + sym->st_name, name)) {
            *idx = i;
            LOGD("found %s at symidx: %u (GNU_HASH UNDEF)\n", name, *idx);
            return sym;
        }
    }

    return NULL;
}


ElfW(Sym) *find_symbol_index_by_name(soinfo *soinfo, const char *symbol, uint32_t *idx) {
    ElfW(Sym) *sym = nullptr;
    if (IS_GNU_HASH(soinfo)) {
        LOGD("%s", "current is gnu hash!");
        sym = soinfo_gnu_lookup(soinfo, gnu_hash(symbol), symbol, idx);
        if (NULL == sym) {
            sym = soinfo_gnu_lookup_undef(soinfo, symbol, idx);
        }
    } else {
        LOGD("%s", "current is elf hash!");
        sym = soinfo_elf_lookup(soinfo, elf_hash(symbol), symbol, idx);
    }
    printSym(soinfo, sym);
    return sym;
}

#if defined(__LP64__)
#define ELF_R_SYM(info)  ELF64_R_SYM(info)
#define ELF_R_TYPE(info) ELF64_R_TYPE(info)
#else
#define ELF_R_SYM(info)  ELF32_R_SYM(info)
#define ELF_R_TYPE(info) ELF32_R_TYPE(info)
#endif

typedef size_t (*RESOLVE_INFO)(void *);

typedef ElfW(Addr) (*RESOLVE_OFFSET)(void *);

int replace(ElfW(Addr) addr, void *newValue, void **old_addr_ptr) {
//    if (*(void **) addr == newValue) {
//        LOGW("Replace same address 0x%X, do nothing!", newValue);
//        return 0;
//    }
//    void *old_addr = *(void **) addr;
//    LOGD("old_addr is 0x%X", old_addr);
//    if (nullptr != old_addr_ptr) *old_addr_ptr = old_addr;
//
//    size_t pageSize = (size_t) sysconf(_SC_PAGESIZE);
//    LOGD("In Memory, PageSize %ul", pageSize);
//    uintptr_t originPtr = (uintptr_t) (addr);
//    void *aligned_pointer = (void *) (originPtr & ~(pageSize - 1));
//    mprotect(aligned_pointer, pageSize, PROT_WRITE | PROT_READ);
//    *((uintptr_t *) addr) = reinterpret_cast<uintptr_t>(newValue);
//    LOGI("replace Addr 0x%X to value 0x%X", addr, newValue);
//    __builtin___clear_cache((char *)aligned_pointer, ((char *)aligned_pointer) + pageSize);
    return 0;
}

static void *_symPtr(shared_ptr<string> symName) throw(runtime_error) {
    void *handle = dlopen(NULL, RTLD_NOW);
    if (!handle) {
        char *error = dlerror();
        throw std::runtime_error("native library [NULL] open error " + string(error));
    }

    void *ptr = dlsym(handle, symName->c_str());
    if (!ptr) {
        char *error = dlerror();
        LOGE("native library [NULL] dlsym error %s\n", error);
        throw std::runtime_error("native library [NULL] dlsym error " + string(error));
    }
    dlclose(handle);
    return ptr;
}

// 5567: 000c72c8    64 OBJECT  LOCAL  HIDDEN    17 __dl_g_default_namespace
extern "C" __attribute__((weak)) extern android_namespace_t __dl_g_default_namespace;
static soinfo* soinfo_from_handle(void* handle) {
    LOGD("g_default_namespace address %s", "HHHHHHHHHHHHHH");
    void *addr = _symPtr(make_shared<string>("__dl_g_default_namespace"));
//    void *addr = &__dl_g_default_namespace;
    LOGD("g_default_namespace address 0x%X", addr);
    std::unordered_map<uintptr_t, soinfo*> *g_soinfo_handles_map = (unordered_map<uintptr_t, soinfo *> *)(
            (unsigned)addr + sizeof(android_namespace_t));
    if ((reinterpret_cast<uintptr_t>(handle) & 1) != 0) {
        auto it = g_soinfo_handles_map->find(reinterpret_cast<uintptr_t>(handle));
        if (it == g_soinfo_handles_map->end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }
    return static_cast<soinfo*>(handle);
}

int add_hook(const char *soname, const char *symbol, void *newValue, void **old_addr_ptr) throw(HookException) {
    uint32_t idx = 0;
    struct soinfo *si = nullptr;

//    si= (struct soinfo *) dlopen(soname, RTLD_NOLOAD);
    si = soinfo_from_handle(dlopen(soname, RTLD_NOLOAD));
//    void *handler = dlopen(soname, RTLD_NOLOAD);
//    memcpy(si, handler, sizeof(soinfo));
    if (!si) {
        LOGE("dlopen error: %s.", dlerror());
        return 1;
    }

    printSoInfo(si);

    ElfW(Sym) *sym = find_symbol_index_by_name(si, symbol, &idx);
    LOGD("find_symbol_index_by_name %s idx: %d", symbol, idx);

    if (!sym && IS_GNU_HASH(si) == 0) {
        // GNU hash table doesn't contain relocation symbols.
        // We still need to search the .rel.plt section for the symbol
        return 0;
    }

    //replace for .rel(a).plt
#if defined(USE_RELA)
    const char *plt_name = ".rela.plt";
    // loop reloc table to find the symbol by index, replace for .rela.plt
    for (ElfW(Rela) *rel = si->plt_rela_; rel < si->plt_rela_ + si->plt_rela_count_; ++rel) {
#else
    const char *plt_name = ".rel.plt";
    // loop reloc table to find the symbol by index, replace for .rel.plt
    for (ElfW(Rel) *rel_plt = si->plt_rel_; rel_plt < si->plt_rel_ + si->plt_rel_count_; ++rel_plt) {
#endif
        // 检查符号表的索引和重定位表中表示的索引是否相等
        if (idx == ELF_R_SYM(rel_plt->r_info) && strcmp(si->strtab_ + (si->symtab_ + idx)->st_name, symbol) == 0) {
            auto type = ELF_R_TYPE(rel_plt->r_info);
            switch (type) {
                case R_ARM_JUMP_SLOT: // .rel.plt 只支持R_ARM_JUMP_SLOT模式
                    LOGD("found %s at %s offset: %p\n", symbol, plt_name, (void *) rel_plt->r_offset);
                    if (rel_plt->r_offset < 0) {
                        LOGE("relocate offset:%u less than 0!", rel_plt->r_offset);
                        throw HookException("hook_and_replace: relocate offset less than 0!");
                    }
                    ElfW(Addr) reloc = rel_plt->r_offset + si->load_bias; // r_offset->重定位入口偏移;
                    return replace(reloc, newValue, old_addr_ptr);
//                default:
//                    LOGW("Expected R_ARM_JUMP_SLOT, found 0x%X", ELF_R_SYM(rel_plt->r_info));
//                    throw HookException("hook_and_replace: only support R_ARM_JUMP_SLOT type!Plz check mode");
            }
        }
    }
//
//    //replace for .rel(a).dyn，全局变量
//#if defined(USE_RELA)
//    const char *dyn_name = ".rela.dyn";
//    // loop reloc table to find the symbol by index, replace for .rela.dyn
//    for (ElfW(Rela) rel = si->rela_; rel < si->rela_ + si->rela_count_; ++rel) {
//#else
//    const char *dyn_name = ".rel.dyn";
//    // loop reloc table to find the symbol by index, replace for .rel.dyn
//    for (ElfW(Rel) *rel = si->rel_; rel < si->rel_ + si->plt_rel_count_; ++rel) {
//#endif
//        // 检查符号表的索引和重定位表中表示的索引是否相等
//        if (idx == ELF_R_SYM(rel->r_info) && strcmp(si->strtab_ + (si->symtab_ + idx)->st_name, symbol) == 0) {
//            auto type = ELF_R_TYPE(rel->r_info);
//            switch (type) {
//                case R_ARM_GLOB_DAT: // .rel.dyn 只支持R_ARM_GLOB_DAT和R_ARM_ABS32模式
//                case R_ARM_ABS32: // .rel.dyn 只支持R_ARM_GLOB_DAT和R_ARM_ABS32模式
//                    LOGD("found %s at %s offset: %p\n", symbol, dyn_name, (void *) rel->r_offset);
//                    if (rel->r_offset < 0) {
//                        LOGE("relocate offset:%u less than 0!", rel->r_offset);
//                        throw HookException("hook_and_replace: relocate offset less than 0!");
//                    }
//                    ElfW(Addr) reloc = rel->r_offset + si->load_bias; // r_offset->重定位入口偏移;
//                    return replace(reloc, newValue, old_addr_ptr);
////                default:
////                    LOGW("Expected R_ARM_GLOB_DAT|R_ARM_ABS32, found 0x%X", ELF_R_SYM(rel->r_info));
////                    throw HookException("hook_and_replace:only support R_ARM_GLOB_DAT&R_ARM_ABS32 type!Plz check mode");
//            }
//        }
//    }
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