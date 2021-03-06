//
// Created by yananh on 2019/4/27.
//
#include "elf_linker.h"
#include <sys/mman.h>
#include <link.h>
#include <elf.h>
#include <linux/elf.h>
#include <vector>
#include <functional>

using namespace std;

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

static bool is_symbol_global_and_defined(soinfo *si, const ElfW(Sym) *s) {
    if (ELF_ST_BIND(s->st_info) == STB_GLOBAL || // 全局变量, 对所有将组合的目标文件都是可见的
        ELF_ST_BIND(s->st_info) == STB_WEAK) { // 弱符号与全局符号类似，不过他们的定义优先级比较低。
        return s->st_shndx != SHN_UNDEF; // SHN_UNDEF: 表示该符号未定义，这个符号表示该符号在本目标文件被引用，但是定义在其他目标文件中
    } else if (ELF_ST_BIND(s->st_info) != STB_LOCAL) { // 局部符号
        // 在包含该符号定义的目标文件以外不可见，相同名称的局部符号可以存在于多个文件中，互不影响。
        // 在每个符号表中，所有具有 STB_LOCAL 绑定的符号都优先于弱符号和全局符号
        LOGW("unexpected ST_BIND value: %d for '%s' in '%s'",
             (ELF_ST_BIND(s->st_info)), si->get_string(s->st_name), si->bias_addr + si->so_name);
    }
    return false;
}

/*
 * 通过gnu hash查询的是一部分，有一些的是无法查询到的。只能查找可访问的
 * 比如在GNU_HASH_TABLE中有symndx成员变量，此变量为动态符号表中可被访问的第一个符号的索引，假如说动态符号表有dynsymcount数量的符号，
 * 表示有(dynsymcount-symndx)个符号可以通过哈希表访问到，而且查找到的索引区间为[symndx, dynsymcount), 区间[0, symndx)无法通过此模式查找到，只能通过线性查找。
 * 此方法通过gnu hash进行查找，能够更快的找到其中的一部分。
 *
 * 但是具体可访问的需要还需要确认！！！
 * 详见https://r00tk1ts.github.io/2017/08/24/GNU%20Hash%20ELF%20Sections/#Dynamic-Section-%E9%9C%80%E6%B1%82
 *
 * @see _gnu_lookup_hided
 */
static ElfW(Sym) *_gnu_lookup_public(soinfo *si, const char *symbol_name, uint32_t *idx) {
    gnu_hash_table *hash_table = si->gnu_hash_ptr;
    if (!hash_table) return nullptr;
    uint32_t hash = gnu_hash(symbol_name);
    uint32_t h2 = hash >> hash_table->bloom_shift;
    uint32_t bloom_mask_bits = sizeof(ElfW(Addr)) * 8;
    uint32_t word_num = (hash / bloom_mask_bits) & hash_table->maskwords;
    ElfW(Addr) bloom_word = hash_table->bloom_filter[word_num];

    // test against bloom filter
    if ((1 & (bloom_word >> (hash % bloom_mask_bits)) & (bloom_word >> (h2 % bloom_mask_bits))) == 0) {
        return nullptr;
    }
    // bloom test says "probably yes"...
    uint32_t n = hash_table->buckets[hash % hash_table->nbuckets];
    if (n == 0) {
        return nullptr;
    }
    do {
        ElfW(Sym) *s = si->symtab_ + n;
        if (((hash_table->chain[n] ^ hash) >> 1) == 0 && strcmp(si->get_string(s->st_name), symbol_name) == 0 /*&&
            is_symbol_global_and_defined(this, s)*/) {
            if (idx) *idx = n;
            LOGI("found %s at symidx: %u (GNU_HASH DEF)\n", symbol_name, n);
            return s;
        }
    } while ((hash_table->chain[n++] & 1) == 0);
    return nullptr;
}

/*
 * 通过gnu hash无法查询的模块可以通过此方法进行查询，只能查询无法访问的点
 * @see _gnu_lookup_public
 */
static ElfW(Sym) *_gnu_lookup_hided(soinfo *si, const char *symbol_name, uint32_t *idx) {
    gnu_hash_table *hash_table = si->gnu_hash_ptr;
    if (!hash_table) return nullptr;
    for (uint32_t i = 0; i < hash_table->symndx; i++) {
        if (0 == strcmp(si->get_string((si->symtab_ + i)->st_name), symbol_name)) {
            if (idx) *idx = i;
            LOGI("found %s at symidx: %u (GNU_HASH UNDEF)\n", symbol_name, i);
            return 0;
        }
    }
    return nullptr;
}

static ElfW(Sym) *_elf_lookup(soinfo *si, const char *symbol_name, uint32_t *idx) {
    elf_hash_table *hash_table = si->elf_hash_ptr;
    if (!hash_table) return nullptr;
    uint32_t hash = elf_hash(symbol_name);
    for (uint32_t n = hash_table->bucket[hash % hash_table->nbucket]; n != 0; n = hash_table->chain[n]) {
        ElfW(Sym) *s = si->symtab_ + n;
        if (strcmp(si->get_string(s->st_name), symbol_name) == 0) {
            if (idx) *idx = n;
            return s;
        }
    }
    return nullptr;
}


ElfW(Sym) *soinfo::gnu_lookup(const char *symbol_name, uint32_t *idx) {
    if (!is_gnu_hash()) return nullptr;
    ElfW(Sym) *res = _gnu_lookup_public(this, symbol_name, idx);
    if (res) return res;
    return _gnu_lookup_hided(this, symbol_name, idx);
}

ElfW(Sym) *soinfo::elf_lookup(const char *symbol_name, uint32_t *idx) {
    if (!is_gnu_hash()) {
        return _elf_lookup(this, symbol_name, idx);
    }
    return nullptr;
}

ElfW(Sym) *soinfo::lookup_symbol(const char *symbol_name, uint32_t *idx) {
    if (gnu_hash_ptr) return gnu_lookup(symbol_name, idx);
    return elf_lookup(symbol_name, idx);
}

bool soinfo::is_gnu_hash() const {
    return gnu_hash_ptr != nullptr;
}

const char *soinfo::get_string(ElfW(Word) offset) const {
    return strtab_ + offset;
}

/**
 * 通过读取.dynamic段获取如下关键(非全部)信息：
 * 1. symtab，表示".dynsym"的地址
 * 2. strtab，表示链接字符串".dynstr"的地址
 * 3. strsz，表示链接字符串的长度
 * 4. hash，动态链接哈希表的地址，".hash"的地址
 * 5. soName，本共享对象的"SO-NAME"
 * 6. RPath，动态链接共享对象搜索路径
 * 7. INIT, 初始化搜索代码地址
 * 8. FINIT, 结束代码地址
 * 9. NEED, 依赖的共享文件，d_ptr表示所以来的共享文件名
 * 10. REL/RELA, 动态链接重定位表地址
 * 11. RELENT/RELAENT 动态重定位表数量
 * 封装成soinfo的类型，具体可以参见Android Linker.cpp源码
 *
 * @see https://android.googlesource.com/platform/bionic/+/refs/tags/android-vts-9.0_r9/linker/linker.cpp
 * decode all ElfW(Dyn) info
 * @param name so name
 * @param load_bias so library bias offset in virtual memory
 * @param dynamic_ptr .dynamic segment address in virtual memory
 * @param dynamic_end_ptr .dynamic end segment address in virtual memory
 * @param result {@code 0} success, else failed
 * @return
 */
bool soinfo::initialize(const char *name, ElfW(Addr) load_bias, ElfW(Dyn) *dynamic_ptr, ElfW(Dyn) *dynamic_end_ptr) {
    this->bias_addr = load_bias;
    dynamic_ = dynamic_ptr;
    dynamic_end_ = dynamic_end_ptr;
    for (ElfW(Dyn) *d = dynamic_ptr; d < dynamic_end_ptr && d->d_tag != DT_NULL; ++d) { // 遍历dynamic中所有的项
        LOGD("Dynamic Segment find tag: = %p \td[0](tag) = %p \td[1](val) = %p",
             d, reinterpret_cast<void *>(d->d_tag), reinterpret_cast<void *>(d->d_un.d_val));
        switch (d->d_tag) {
            case DT_SONAME:
                // TODO: glibc dynamic linker uses this name for
                // initial library lookup; consider doing the same here.
                so_name = d->d_un.d_ptr;
                break;
            case DT_HASH:
                // The address of the symbol hash table. This table refers to the symbol table indicated by the DT_SYMTAB element. See Hash Table Section.
                // This element holds the address of the symbol hash table, described in 'http://www.sco.com/developers/gabi/latest/ch5.dynamic.html#hash'.
                // This hash table refers to the symbol table referenced by the DT_SYMTAB element.
                elf_hash_ptr = new elf_hash_table;
                elf_hash_ptr->nbucket = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr)[0];
                elf_hash_ptr->nchain = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr)[1];
                elf_hash_ptr->bucket = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr + 8);
                elf_hash_ptr->chain = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr + 8 +
                                                                   elf_hash_ptr->nbucket * 4);
                break;
            case DT_GNU_HASH:
                gnu_hash_ptr = new gnu_hash_table;
                gnu_hash_ptr->nbuckets = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr)[0];
                gnu_hash_ptr->symndx = ((uint32_t *) (load_bias + d->d_un.d_ptr))[1];
                gnu_hash_ptr->maskwords = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr)[2];
                gnu_hash_ptr->bloom_shift = reinterpret_cast<uint32_t *>(load_bias + d->d_un.d_ptr)[3];
                gnu_hash_ptr->bloom_filter = reinterpret_cast<ElfW(Addr) *>(load_bias + d->d_un.d_ptr + 16);
                gnu_hash_ptr->buckets = reinterpret_cast<uint32_t *>(gnu_hash_ptr->bloom_filter +
                                                                     gnu_hash_ptr->maskwords);
                // amend chain for symndx = header[1]
                gnu_hash_ptr->chain = gnu_hash_ptr->buckets + gnu_hash_ptr->nbuckets - gnu_hash_ptr->symndx;
                if (!powerof2(gnu_hash_ptr->maskwords)) {
                    LOGE("invalid maskwords for gnu_hash_ptr = 0x%x, in \"%s\" expecting power to two",
                         gnu_hash_ptr->maskwords, name);
                    return false;
                }
                --gnu_hash_ptr->maskwords;
                break;
            case DT_STRTAB:
                // The address of the string table. Symbol names, dependency names, and other strings required by the runtime linker reside in this table.
                // See <href='https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-73709.html#scrolltoc'>String Table Section</href>.
                strtab_ = reinterpret_cast<const char *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_STRSZ:
                // The total size, in bytes, of the DT_STRTAB string table.
                strtab_size_ = d->d_un.d_val;
                break;
            case DT_SYMTAB:
                // This element holds the address of the symbol table, described in the first part of this chapter,
                // with Elf32_Sym entries for the 32-bit class of files and Elf64_Sym entries for the 64-bit class of files.
                // The address of the symbol table. See <href='https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-79797.html#scrolltoc'>Symbol Table Section</href>.
                symtab_ = reinterpret_cast<ElfW(Sym) *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_SYMENT:
                // The size, in bytes, of the DT_SYMTAB symbol entry.每一条数据大小
                if (d->d_un.d_val != sizeof(ElfW(Sym))) {
                    LOGE("invalid DT_SYMENT: %zd in \"%s\"", static_cast<size_t>(d->d_un.d_val), name);
                    return false;
                }
                break;
            case DT_PLTREL:
                // Android not support now
#if defined(USE_RELA)
                if (d->d_un.d_val != DT_RELA) {
                    LOGE("unsupported DT_PLTREL in \"%s\"; expected DT_RELA", name);
                    return false;
                }
#else
            if (d->d_un.d_val != DT_REL) {
                LOGE("unsupported DT_PLTREL in \"%s\"; expected DT_REL", name);
                return false;
            }
#endif
                break;
            case DT_JMPREL:
                // The address of relocation entries that are associated solely with the procedure linkage table.
                // See Procedure Linkage Table (Processor-Specific). The separation of these relocation entries enables
                // the runtime linker to ignore these entries when the object is loaded with lazy binding enabled.
                // This element requires the DT_PLTRELSZ and DT_PLTREL elements also be present.
#if defined(USE_RELA)
                plt_rela_ = reinterpret_cast<ElfW(Rela) *>(load_bias + d->d_un.d_ptr);
#else
                plt_rel_ = reinterpret_cast<ElfW(Rel) *>(load_bias + d->d_un.d_ptr);
#endif
                break;
            case DT_PLTRELSZ:
                // The total size, in bytes, of the relocation entries associated with the procedure linkage table.
                // See <href='https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-1235.html#scrolltoc'>Procedure Linkage Table (Processor-Specific)</href>.
#if defined(USE_RELA)
                plt_rela_count_ = d->d_un.d_val / sizeof(ElfW(Rela));
#else
                plt_rel_count_ = d->d_un.d_val / sizeof(ElfW(Rel));
#endif
                break;
            case DT_PLTGOT:
#if defined(__mips__)
                // Used by mips and mips64.
                plt_got_ = reinterpret_cast<ElfW(Addr)**>(load_bias + d->d_un.d_ptr);
#endif
                // Ignore for other platforms... (because RTLD_LAZY is not supported)
                break;
            case DT_DEBUG:
                // Set the DT_DEBUG entry to the address of _r_debug for GDB
                // if the dynamic table is writable
                // FIXME: not working currently for N64
                // The flags for the LOAD and DYNAMIC program headers do not agree.
                // The LOAD section containing the dynamic table has been mapped as
                // read-only, but the DYNAMIC header claims it is writable.
                break;
#if defined(USE_RELA)
            case DT_RELA:
                rela_ = reinterpret_cast<ElfW(Rela) *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_RELASZ:
                rela_count_ = d->d_un.d_val / sizeof(ElfW(Rela));
                break;
            case DT_ANDROID_RELA:
                android_relocs_ = reinterpret_cast<uint8_t *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_ANDROID_RELASZ:
                android_relocs_size_ = d->d_un.d_val;
                break;
            case DT_ANDROID_REL:
                LOGE("unsupported DT_ANDROID_REL in \"%s\"", name);
                return false;
            case DT_ANDROID_RELSZ:
                LOGE("unsupported DT_ANDROID_RELSZ in \"%s\"", name);
                return false;
            case DT_RELAENT:
                if (d->d_un.d_val != sizeof(ElfW(Rela))) {
                    LOGE("invalid DT_RELAENT: %zd", static_cast<size_t>(d->d_un.d_val));
                    return false;
                }
                break;
                // ignored (see DT_RELCOUNT comments for details)
            case DT_RELACOUNT:
                break;
            case DT_REL:
                LOGE("unsupported DT_REL in \"%s\"", name);
                return false;
            case DT_RELSZ:
                LOGE("unsupported DT_RELSZ in \"%s\"", name);
                return false;
#else
            case DT_REL:
                // .rel.dyn, 适用于全局变量
                // Similar to DT_RELA, except its table has implicit addends. This element requires that the DT_RELSZ and DT_RELENT elements also be present.
                rel_ = reinterpret_cast<ElfW(Rel) *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_RELSZ:
                // The total size, in bytes, of the DT_REL relocation table.
                rel_count_ = d->d_un.d_val / sizeof(ElfW(Rel));
                break;
            case DT_RELENT:
                // The size, in bytes, of the DT_REL relocation entry.
                // it same that illegal on android platform
                if (d->d_un.d_val != sizeof(ElfW(Rel))) {
                    LOGE("invalid DT_RELENT: %zd", static_cast<size_t>(d->d_un.d_val));
                    return false;
                }
                break;
            case DT_ANDROID_REL:
                android_relocs_ = reinterpret_cast<uint8_t *>(load_bias + d->d_un.d_ptr);
                break;
            case DT_ANDROID_RELSZ:
                android_relocs_size_ = d->d_un.d_val;
                break;
            case DT_ANDROID_RELA:
                LOGE("unsupported DT_ANDROID_RELA in \"%s\"", name);
                return false;
            case DT_ANDROID_RELASZ:
                LOGE("unsupported DT_ANDROID_RELASZ in \"%s\"", name);
                return false;
                // "Indicates that all RELATIVE relocations have been concatenated together,
                // and specifies the RELATIVE relocation count."
                //
                // TODO: Spec also mentions that this can be used to optimize relocation process;
                // Not currently used by bionic linker - ignored.
            case DT_RELCOUNT:
                // Indicates the RELATIVE relocation count, which is produced from the concatenation of all Elf32_Rel relocations.
                // See <href='https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter4-10454.html#chapter4-14'>Combined Relocation Sections</href>.
                break;
            case DT_RELA:
                LOGE("unsupported DT_RELA in \"%s\"", name);
                return false;
            case DT_RELASZ:
                // The total size, in bytes, of the DT_RELA relocation table.
                LOGE("unsupported DT_RELASZ in \"%s\"", name);
                return false;
#endif
            case DT_INIT:
                // he address of an initialization function. https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter2-55859.html#chapter2-48195
                init_func_ = reinterpret_cast<linker_function_t>(load_bias + d->d_un.d_ptr);
                LOGD("%s constructors (DT_INIT) found at %p", name, init_func_);
                break;
            case DT_FINI:
                // The address of a termination function. https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter2-55859.html#chapter2-48195
                fini_func_ = reinterpret_cast<linker_function_t>(load_bias + d->d_un.d_ptr);
                LOGD("%s destructors (DT_FINI) found at %p", name, fini_func_);
                break;
            case DT_INIT_ARRAY:
                // The address of an array of pointers to initialization functions.
                // This element requires that a DT_INIT_ARRAYSZ element also be present.
                // https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter2-55859.html#chapter2-48195
                init_array_ = reinterpret_cast<linker_function_t *>(load_bias + d->d_un.d_ptr);
                LOGD("%s constructors (DT_INIT_ARRAY) found at %p", name, init_array_);
                break;
            case DT_INIT_ARRAYSZ:
                // The total size, in bytes, of the DT_INIT_ARRAY array.
                init_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
                break;
            case DT_FINI_ARRAY:
                // The address of an array of pointers to termination functions.
                // This element requires that a DT_FINI_ARRAYSZ element also be present.
                // https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter2-55859.html#chapter2-48195
                fini_array_ = reinterpret_cast<linker_function_t *>(load_bias + d->d_un.d_ptr);
                LOGD("%s destructors (DT_FINI_ARRAY) found at %p", name, fini_array_);
                break;
            case DT_FINI_ARRAYSZ:
                // The total size, in bytes, of the DT_FINI_ARRAY array.
                fini_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
                break;
            case DT_PREINIT_ARRAY:
                // The address of an array of pointers to pre-initialization functions.
                // This element requires that a DT_PREINIT_ARRAYSZ element also be present.
                // This array is processed only in an executable file. This array is ignored if contained in a shared object.
                // https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter2-55859.html#chapter2-48195
                preinit_array_ = reinterpret_cast<linker_function_t *>(load_bias + d->d_un.d_ptr);
                LOGD("%s constructors (DT_PREINIT_ARRAY) found at %p", name, preinit_array_);
                break;
            case DT_PREINIT_ARRAYSZ:
                // The total size, in bytes, of the DT_PREINIT_ARRAY array.
                preinit_array_count_ = static_cast<uint32_t>(d->d_un.d_val) / sizeof(ElfW(Addr));
                break;
            case DT_TEXTREL:
#if defined(__LP64__)
                LOGE("text relocations (DT_TEXTREL) found in 64-bit ELF file \"%s\"", name);
                return false;
#else
            has_text_relocations = true;
            break;
#endif
            case DT_SYMBOLIC:
                // Indicates the object contains symbolic bindings that were applied during its link-edit.
                // This elements use has been superseded by the DF_SYMBOLIC flag. See Using the -B symbolic Option.
                has_DT_SYMBOLIC = true;
                break;
            case DT_NEEDED:
                /**
                 * The DT_STRTAB string table offset of a null-terminated string, giving the name of a needed dependency.
                 * The dynamic array can contain multiple entries of this type. The relative order of these entries is significant,
                 * though their relation to entries of other types is not.
                 * See <href='https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-63352.html#scrolltoc'>Shared Object Dependencies</href>.
                 */
                needed_lib.push_back(d->d_un.d_val);
                break;
            case DT_FLAGS:
                if (d->d_un.d_val & DF_TEXTREL) {
#if defined(__LP64__)
                    LOGE("text relocations (DF_TEXTREL) found in 64-bit ELF file \"%s\"", name);
                    return false;
#else
                    has_text_relocations = true;
#endif
                }
                if (d->d_un.d_val & DF_SYMBOLIC) {
                    has_DT_SYMBOLIC = true;
                }
                break;
            case DT_FLAGS_1:
//                set_dt_flags_1(d->d_un.d_val);
//                if ((d->d_un.d_val & ~SUPPORTED_DT_FLAGS_1) != 0) {
//                    DL_WARN("Unsupported flags DT_FLAGS_1=%p", reinterpret_cast<void *>(d->d_un.d_val));
//                }
                break;
                // Ignored: "Its use has been superseded by the DF_BIND_NOW flag"
            case DT_BIND_NOW:
                break;
                // Ignore: bionic does not support symbol versioning...
            case DT_VERSYM:
            case DT_VERDEF:
            case DT_VERDEFNUM:
            case DT_VERNEED:
            case DT_VERNEEDNUM:
                // useless
                break;
            default:
//                if (!relocating_linker) {
//                    LOGW("%s: unused DT entry: type %p arg %p", name,
//                            reinterpret_cast<void *>(d->d_tag), reinterpret_cast<void *>(d->d_un.d_val));
//                }
                break;
        }
    }
    return true;
}

soinfo::soinfo(const char *name) {
    memset(this, 0, sizeof(*this));
}

soinfo::~soinfo() {
    if (elf_hash_ptr) {
        delete elf_hash_ptr;
        elf_hash_ptr = nullptr;
    }
    if (gnu_hash_ptr) {
        delete gnu_hash_ptr;
        gnu_hash_ptr = nullptr;
    }

    if (segment_attr_list) {
        for (uint32_t i = 0; i < segment_attr_count; ++i) {
            delete segment_attr_list[i];
        }

        delete segment_attr_list;
        segment_attr_list = nullptr;
    }
}

/**
 * 从所有的program header中查找.dynamic segment。一个so中只能有一个.dynamic segment, 找到指定的ElfW(Phdr)地址
 * @param info so中的所有program header集合
 * @return
 */
static inline const ElfW(Phdr) *lookup_dynamic_segment(struct dl_phdr_info *info) {
    for (int j = 0; j < info->dlpi_phnum; j++) {
        const ElfW(Phdr) *segment = info->dlpi_phdr + j;
        if (!segment) {
            continue;
        }
        if (PT_DYNAMIC == segment->p_type) {
            return segment;
        }
    }
    return nullptr;
}

extern "C" segment_attr *lookup_segment_attr(soinfo *elf_ptr, ElfW(Addr) looked_addr) {
    for (int i = 0; i < elf_ptr->segment_attr_count; ++i) {
        segment_attr *attr = elf_ptr->segment_attr_list[i];
        if (!attr) continue;
        if (attr->in_this_segment(looked_addr)) {
            return attr;
        }
    }
    return nullptr;
}

extern "C" std::unique_ptr<soinfo> read_from_dl_phdr_info(struct dl_phdr_info *info) {
    const ElfW(Phdr) *dynamic_segment = lookup_dynamic_segment(info); // search .dynamic 段
    LOGD("library PHT(program header table) address=%10p", dynamic_segment);
    if (!dynamic_segment) return nullptr;
    ElfW(Dyn) *dynamic_ptr = (ElfW(Dyn) *) (info->dlpi_addr + dynamic_segment->p_paddr);
    unsigned count = static_cast<unsigned int>(dynamic_segment->p_memsz / sizeof(ElfW(Dyn)));
    ElfW(Dyn) *dynamic_end_ptr = dynamic_ptr + count;
    LOGD(".dynamic segment address range:[%10p-%10p], count %u", dynamic_ptr, dynamic_end_ptr, count);
    if (!dynamic_ptr || !dynamic_end_ptr) return nullptr;
    soinfo *si_p = new soinfo(info->dlpi_name);
    memset(si_p, 0, sizeof(soinfo));
    std::unique_ptr<soinfo> soinfo_ptr(si_p);
    if (!(*soinfo_ptr).initialize(info->dlpi_name, info->dlpi_addr, dynamic_ptr, dynamic_end_ptr)) {
        return nullptr;
    }
    typedef segment_attr *SAP;
    SAP *segment_attr_list = new SAP[info->dlpi_phnum];
    memset(segment_attr_list, 0, sizeof(SAP) * info->dlpi_phnum);
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *segment = info->dlpi_phdr + i;
        if (!segment) continue;
        SAP attr = new segment_attr;
        attr->start = info->dlpi_addr + segment->p_vaddr;
        attr->end = attr->start + segment->p_memsz - 1;
        attr->flag = segment->p_flags;
        segment_attr_list[i] = attr;
        LOGD("so %s, type %d, addr[%10p-%10p], p_flags=%d", info->dlpi_name, segment->p_type, attr->start, attr->end,
             segment->p_flags);
    }
    (*soinfo_ptr).segment_attr_list = segment_attr_list;
    (*soinfo_ptr).segment_attr_count = info->dlpi_phnum;
    print_android_elf(&(*soinfo_ptr));
    return std::move(soinfo_ptr);
}


extern "C" std::unique_ptr<soinfo> read_from_proc(const char *so_name) {
    // read soinfo struct from /proc/self/maps









    return nullptr;
}

extern "C" int
dl_iterate_phdr(int (*__callback)(struct dl_phdr_info *, size_t, void *), void *__data)__attribute__ ((weak));

static int dl_callback(struct dl_phdr_info *info, size_t size, void *data) {
    LOGD ("so name=%s@%10p (%d segments) size=%d\n", info->dlpi_name, info->dlpi_addr, info->dlpi_phnum, size);
    std::unique_ptr<soinfo> uniquePtr = read_from_dl_phdr_info(info);
    if (!uniquePtr) {
        LOGE("read soinfo from dl_phdr_info failed %s", info->dlpi_name);
        return 0;
    }
    std::function<void(std::unique_ptr<soinfo>)> *callback = static_cast<function<void(unique_ptr<soinfo>)> *>(data);
    (*callback)(std::move(uniquePtr));
    return 0;
}

extern "C" void travel_all_loaded_so(std::function<void(std::unique_ptr<soinfo>)> callback) {
    if (dl_iterate_phdr) {
        dl_iterate_phdr(dl_callback, &callback);
        return;
    }
}

void print_android_elf(soinfo *si) {
    LOGD("----------------%s", "--------------------------------------------------------------------------------");
    LOGD("----so name %s", (char *) si->strtab_ + si->so_name);
    LOGD("----bias addr %10p", si->bias_addr);
    LOGD("----dynamic addr %10p", si->dynamic_);
    LOGD("----rel.plt count %u", si->plt_rel_count_);
    LOGD("----rel count %u", si->rel_count_);
    LOGD("----rela count %u", si->rela_count_);
    LOGD("----rela.plt count %u", si->plt_rela_count_);
    LOGD("----android relocs size %u", si->android_relocs_size_);
    for (auto itor = si->needed_lib.begin(); itor != si->needed_lib.end(); ++itor) {
        LOGD("----Needby %s", si->strtab_ + *itor);
    }
    LOGD("----------------%s", "--------------------------------------------------------------------------------");
    return;
}

static bool verify_android_elf(struct soinfo *elf) {
    return true;
}
