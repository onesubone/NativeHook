//
// Created by yananh on 2019/4/27.
//

#ifndef RUBICK_ELF_PARSER_H
#define RUBICK_ELF_PARSER_H

#include <vector>
#include <sys/mman.h>
#include <elf.h>
#include <linux/elf.h>
#include <link.h>
#include "jni_loader.h"

#define FLAG_LINKED           0x00000001
#define FLAG_EXE              0x00000004 // The main executable
#define FLAG_LINKER           0x00000010 // The linker itself
#define FLAG_GNU_HASH         0x00000040 // uses gnu hash
#define FLAG_MAPPED_BY_CALLER 0x00000080 // the map is reserved by the caller and should not be unmapped
#define powerof2(x)    ((((x) - 1) & (x)) == 0)
#define FLAG_NEW_SOINFO       0x40000000 // new soinfo format
#define HOOK_ERR -1
#define LOOK_UP_DYNAMIC_SEGMENT_ERR -2

using namespace std;

typedef void (*linker_function_t)();

typedef struct {
    Elf32_Sword d_tag;
    union {
        Elf32_Sword d_val;
        Elf32_Addr d_ptr;
    } d_un;
} _Elf32_Dyn;
typedef struct {
    Elf64_Sxword d_tag;
    union {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    } d_un;
} _Elf64_Dyn;
typedef ElfW(Dyn) _DYN;

typedef struct {
    ElfW(Addr) dlpi_addr; // so加载的起始偏移地址
    const char *dlpi_name; // so name
    const ElfW(Phdr) *dlpi_phdr; // so program header table (PHT)
    ElfW(Half) dlpi_phnum; // count of program header table (PHT)
} _dl_phdr_info;

struct segment_attr {
    ElfW(Addr) start;
    ElfW(Addr) end;
    ElfW(Word) flag;
    bool in_this_segment(ElfW(Addr) addr) {
        return addr >= start && addr <= end;
    }
};

// Same as in DT_HASH, symbols are put in one of nbuckets buckets depending on their hashes.
// To be specific, each symbol should be placed into hash % nbuckets bucket.
typedef struct {
    // 哈希桶个数
    uint32_t nbuckets;
    // 动态符号表有dynsymcount数量的符号。symndx是动态符号表中可被访问的第一个符号的索引。这意味着有(dynsymcount-symndx)个符号可以通过哈希表访问到。
    uint32_t symndx;
    // 在哈希表节的Bloom filter部分中，ELFCLASS单位大小的掩码个数（每个掩码可以为32位或64位）。这个值必须非零，同时必须是一个2的幂次数，见下面的描述。
    // 注意到值0可以被解释成哈希节中不存在Bloom filter。然而，GNU linkers不会做此处理——GNU哈希节永远至少包含1个掩码。
    uint32_t maskwords;
    // Bloom filter使用的位移计数
    uint32_t bloom_shift;
    // GNU_HASH sections包含一个Bloom filter。该过滤器用于快速拒绝在对象中不存在的符号查找。对ELFCLASS32对象来说，Bloom filter的掩码是32位的，ELFCLASS64则是64位。
    ElfW(Addr) *bloom_filter; /* bloom[bloom_size], uint64_t for 64-bit binaries */
    // nbuckets长度的32位哈希桶数组。
    uint32_t *buckets; /* buckets[nbuckets] */
    // 32位哈希链值的数组，长度为(dynsymcount-symndx)，每个符号都源自动态符号表的第二部分。
    uint32_t *chain;
} gnu_hash_table;

typedef struct {
    size_t nbucket;
    size_t nchain;
    uint32_t *bucket;
    uint32_t *chain;
} elf_hash_table;

class soinfo {
public:
    soinfo(const char *name);

    ElfW(Addr) bias_addr;
    ElfW(Addr) so_name; // 实际so name起始地址：elf_ptr->strtab + elf_ptr->so_name
    /* 动态库的dynamic segment的起始地址，真实地址需要增加动态库的偏移地址 */
    ElfW(Dyn) *dynamic_;
    ElfW(Dyn) *dynamic_end_;
    const char *strtab_; // 默认第一个字节是null，以防万一
    size_t strtab_size_;
    ElfW(Sym) *symtab_; // Elf32_Sym *symtab_;


    ElfW(Rel) *plt_rel_; // for .rel.plt，用于PLT方法的重定向
    size_t plt_rel_count_;
    ElfW(Rel) *rel_; // for .rel.dyn，用于全局变量等
    size_t rel_count_;

    ElfW(Rela) *rela_;
    size_t rela_count_;
    ElfW(Rela) *plt_rela_;
    size_t plt_rela_count_;

    uint8_t *android_relocs_;
    size_t android_relocs_size_;


    linker_function_t init_func_;
    linker_function_t fini_func_;
    linker_function_t *init_array_;
    uint32_t init_array_count_;
    linker_function_t *fini_array_;
    uint32_t fini_array_count_;
    linker_function_t *preinit_array_;
    uint32_t preinit_array_count_;

    // elf hash
    elf_hash_table *elf_hash_ptr;
    // gnu hash
    gnu_hash_table *gnu_hash_ptr;

    /**
     * 依赖的第三方库在strtab中的索引
     */
    std::vector<ElfW(Sword)> needed_lib;
    bool has_text_relocations;
    bool has_DT_SYMBOLIC;

    bool initialize(const char *name, ElfW(Addr) load_bias, ElfW(Dyn) *dynamic_ptr, ElfW(Dyn) *dynamic_end_ptr);

    const char *get_string(ElfW(Word) index) const;

    bool is_gnu_hash() const;

    ElfW(Sym) *elf_lookup(const char *symbol_name, uint32_t *idx);

    ElfW(Sym) *gnu_lookup(const char *symbol_name, uint32_t *idx);

    ElfW(Sym) *lookup_symbol(const char *symbol_name, uint32_t *idx);

    segment_attr **segment_attr_list;
    uint32_t segment_attr_count;
};

extern "C" soinfo *read_from_dl_phdr_info(struct dl_phdr_info *info);
extern "C" void print_android_elf(soinfo *si);
extern "C" segment_attr *lookup_segment_attr(soinfo *elf_ptr, ElfW(Addr) addr);
//extern "C" void lookup_soinfo(const char *so_name, lookup_result *result);
#endif //RUBICK_ELF_PARSER_H
