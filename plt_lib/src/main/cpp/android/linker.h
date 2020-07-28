/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __LINKER_H
#define __LINKER_H

#include <link.h>
#include <dlfcn.h>
#include <string>
#include <inttypes.h>
#include <elf.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <linux/elf.h>
#include "linked_list.h"
#include "linker_namespaces.h"

// https://android.googlesource.com/platform/bionic.git/+/android-m-preview-1/linker/linker.h
// Returns the address of the page containing address 'x'.
#define PAGE_START(x)  ((x) & PAGE_MASK)
// Returns the offset of address 'x' in its page.
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)
// Returns the address of the next page after address 'x', unless 'x' is
// itself at the start of a page.
#define PAGE_END(x)    PAGE_START((x) + (PAGE_SIZE-1))

// Android uses RELA for aarch64 and x86_64. mips64 still uses REL.
#if defined(__aarch64__) || defined(__x86_64__)
#define USE_RELA 1
#endif



// copy from bionic/linker/linker_soinfo.h
#define FLAG_LINKED           0x00000001
#define FLAG_EXE              0x00000004 // The main executable
#define FLAG_LINKER           0x00000010 // The linker itself
#define FLAG_GNU_HASH         0x00000040 // uses gnu hash
#define FLAG_MAPPED_BY_CALLER 0x00000080 // the map is reserved by the caller and should not be unmapped
#define FLAG_IMAGE_LINKED     0x00000100 // Is image linked - this is a guard on link_image. The difference between this flag and
// FLAG_LINKED is that FLAG_LINKED
// means is set when load_group is
// successfully loaded whereas this
// flag is set to avoid linking image
// when link_image called for the
// second time. This situation happens
// when load group is crossing
// namespace boundary twice and second
// local group depends on the same libraries.
#define FLAG_TLS_NODELETE     0x00000200 // This flag set when there is at least one
// outstanding thread_local dtor
// registered with this soinfo. In such
// a case the actual unload is
// postponed until the last thread_local
// destructor associated with this
// soinfo is executed and this flag is
// unset.
#define FLAG_NEW_SOINFO       0x40000000 // new soinfo format

#define SUPPORTED_DT_FLAGS_1 (DF_1_NOW | DF_1_GLOBAL | DF_1_NODELETE | DF_1_PIE)

#define SOINFO_VERSION 4

// Android uses RELA for aarch64 and x86_64. mips64 still uses REL.
#if defined(__aarch64__) || defined(__x86_64__)
#define USE_RELA 1
#endif

typedef void (*linker_dtor_function_t)();

typedef void (*linker_ctor_function_t)(int, char **, char **);


class SymbolName {
public:
    explicit SymbolName(const char *name)
            : name_(name), has_elf_hash_(false), has_gnu_hash_(false),
              elf_hash_(0), gnu_hash_(0) {}

    const char *get_name() {
        return name_;
    }

    uint32_t elf_hash();

    uint32_t gnu_hash();

private:
    const char *name_;
    bool has_elf_hash_;
    bool has_gnu_hash_;
    uint32_t elf_hash_;
    uint32_t gnu_hash_;
};

struct soinfo;

struct version_info {
    constexpr version_info() : elf_hash(0), name(nullptr), target_si(nullptr) {}

    uint32_t elf_hash;
    const char *name;
    const soinfo *target_si;
};

// TODO(dimitry): remove reference from soinfo member functions to this class.
class VersionTracker;

#if !defined(__work_around_b_24465209__) && !defined(__LP64__)
#define __work_around_b_24465209__ // 在arm和x86模式下，编译命令中默认增加-D__work_around_b_24465209__，arm64、x86_64、mips，mips64则没有此参数
// 具体参见：https://android.googlesource.com/platform/bionic/+/refs/tags/android-cts-8.1_r14/linker/Android.bp
#endif

#if defined(__work_around_b_24465209__)
#define SOINFO_NAME_LEN 128
#endif

struct soinfo {
public:
#if defined(__work_around_b_24465209__)
public:
//    private:
    char old_name_[SOINFO_NAME_LEN];
#endif
public:
    const ElfW(Phdr) *phdr; // Elf32_Phdr *phdr;
//    const Elf32_Phdr *phdr;
    size_t phnum;
#if defined(__work_around_b_24465209__)
    ElfW(Addr) unused0; // DO NOT USE, maintained for compatibility.
#endif
    ElfW(Addr) base; // Elf32_Addr base;
    size_t size;

#if defined(__work_around_b_24465209__)
    uint32_t unused1;  // DO NOT USE, maintained for compatibility.
#endif

    ElfW(Dyn) *dynamic; // Elf32_Dyn dynamic;

#if defined(__work_around_b_24465209__)
    uint32_t unused2; // DO NOT USE, maintained for compatibility
    uint32_t unused3; // DO NOT USE, maintained for compatibility
#endif

    soinfo *next;
//private:
    uint32_t flags_;

    const char *strtab_; // 默认第一个字节是null，以防万一
    ElfW(Sym) *symtab_; // Elf32_Sym *symtab_;

    size_t nbucket_;
    size_t nchain_;
    uint32_t *bucket_;
    uint32_t *chain_;

#if defined(__mips__) || !defined(__LP64__)
    // __LP64__ 为linux64位的宏
    // This is only used by mips and mips64, but needs to be here for
    // all 32-bit architectures to preserve binary compatibility.
    ElfW(Addr) **plt_got_;
#endif

//    ● .rel.text –the relocation for functions (for statically linked modules)
//    ● .rel.data – the relocation for static variables (for statically linked modules)
//    ● .rel.plt – the list of elements in the PLT (Procedure Linkage Table), which are liable to the relocation during the dynamic linking (if PLT is used)
//    ● .rel.dyn – the relocation for dynamically linked functions (if PLT is not used)
//    ● .got – Global Offset Table, contains the information about the offsets of relocated objects
//    对于数据，根据.rel.dyn找到.got中的offset位置
//    对于函数则通过.plt桩函数和.rel.plt节区来获取函数真实地址，然后存在于.got.plt
//    要理解动态连接中访问外部符号是通过.got和.got.plt
//    ● .rel.text：重定位的地方在.text节区内，以offset指定具体要定位位置。在连接时候由连接器完成。注意比较.text段前后变化。指的是比较.o文件和最终的执行文件（或者动态库文件）。就是重定位前后比较，以上是说明了具体比较对象而已。
//    ● .rel.dyn：重定位的地方在.got节区内。主要是针对外部数据变量符号。例如全局数据。重定位在程序运行时定位，一般是在.init段内。定位过程：获得符号对应value后，根据rel.dyn表中对应的offset，修改.got表对应位置的value。另外，.rel.dyn 含义是指和dyn有关，一般是指在程序运行时候，动态加载。区别于rel.plt，rel.plt是指和plt相关，具体是指在某个函数被调用时候加载。我个人理解这个Section的作用是，在重定位过程中，动态链接器根据r_offset找到.got对应表项，来完成对.got表项值的修改。
//    ● .rel.plt：重定位的地方在.got.plt节区内（注意也是.got内,具体区分而已）。主要是针对外部函数符号。一般是函数首次被调用时候重定位。可看汇编，理解其首次访问是如何重定位的，实际很简单，就是初次重定位函数地址，然后把最终函数地址放到.got.plt内，以后读取该.got.plt就直接得到最终函数地址(参考过程说明)。  所有外部函数调用都是经过一个对应桩函数，这些桩函数都在.plt段内。
//    .rel.dyn和.rel.plt是动态定位辅助段。由连接器产生，存在于可执行文件或者动态库文件内。借助这两个辅助段可以动态修改对应.got和.got.plt段，从而实现运行时重定位。
//    .plt（过程链接表），存放重定位桩函数的。所有外部函数调用都是经过一个对应桩函数，这些桩函数都在.plt段内。具体调用外部函数过程是：
//    过程说明：调用对应桩函数--->桩函数取出.got表(具体是.got.plt)表内地址--->然后跳转到这个地址。如果是第一次,这个跳转地址默认是桩函数本身跳转处地址的下一个指令地址(目的是通过桩函数统一集中取地址和加载地址),后续接着把对应函数的真实地址加载进来放到.got.plt表对应处,同时跳转执行该地址指令.以后桩函数从.got.plt取得地址都是真实函数地址了。
//    typedef struct {
//      Elf32_Addr r_offset;    // 重定位入口偏移（受影响的存储单位的第一个字节的偏移或者虚拟地址）
//      Elf32_Word r_info;     // 要进行重定位的符号表索引，以及将实施的重定位类型（哪些位需要修改，以及如何计算它们的取值）
//                            // 其中 .rel.dyn 重定位类型一般为R_386_GLOB_DAT和R_386_COPY；.rel.plt为R_386_JUMP_SLOT
//    } Elf32_Rel;
#if defined(USE_RELA)
    ElfW(Rela)* plt_rela_;
    size_t plt_rela_count_;

    ElfW(Rela)* rela_;
    size_t rela_count_;
#else
    ElfW(Rel) *plt_rel_;  // Elf32_Rel *plt_rel_;
    size_t plt_rel_count_;

    ElfW(Rel) *rel_;  // Elf32_Rel *rel_;
    size_t rel_count_;
#endif

    linker_ctor_function_t *preinit_array_;
    size_t preinit_array_count_;

    linker_ctor_function_t *init_array_;
    size_t init_array_count_;
    linker_dtor_function_t *fini_array_;
    size_t fini_array_count_;

    linker_ctor_function_t init_func_;
    linker_dtor_function_t fini_func_;

#if defined(__arm__)
public:
    // ARM EABI section used for stack unwinding.
    uint32_t *ARM_exidx;
    size_t ARM_exidx_count;
private:
#endif
    size_t ref_count_;
public:
    link_map link_map_head;

    bool constructors_called;

    // When you read a virtual address from the ELF file, add this
    // value to get the corresponding address in the process' address space.
    ElfW(Addr) load_bias; // Elf32_Addr *load_bias;

#if !defined(__LP64__)
    bool has_text_relocations;
#endif
    bool has_DT_SYMBOLIC;

public:
    // 其实可以删除所有的方法，因为成员方法都是通过符号表进行访问的，存在与否不会影响成员变量的偏移。保留一些只是为了顺便看下
    void call_constructors();

    void call_destructors();

    void call_pre_init_constructors();

    bool prelink_image();

    bool protect_relro();

    void add_child(soinfo *child);

    void remove_all_links();

    bool find_symbol_by_name(SymbolName &symbol_name,
                             const version_info *vi,
                             const ElfW(Sym) **symbol) const;

    ElfW(Sym) *find_symbol_by_address(const void *addr);

    ElfW(Addr) resolve_symbol_address(const ElfW(Sym) *s) const;

    const char *get_string(ElfW(Word) index) const;

    bool is_gnu_hash() const;

    bool is_linked() const;

    bool is_linker() const;

    void increment_ref_count();

    size_t decrement_ref_count();

    soinfo *get_local_group_root() const;

    void set_soname(const char *soname);

    const char *get_soname() const;

    const char *get_realpath() const;

    const ElfW(Versym) *get_versym(size_t n) const;

    ElfW(Addr) get_verneed_ptr() const;

private:
    bool elf_lookup(SymbolName &symbol_name, const version_info *vi, uint32_t *symbol_index) const;

    ElfW(Sym) *elf_addr_lookup(const void *addr);

    bool gnu_lookup(SymbolName &symbol_name, const version_info *vi, uint32_t *symbol_index) const;

    ElfW(Sym) *gnu_addr_lookup(const void *addr);

//private:
public:
    // This part of the structure is only available
    // when FLAG_NEW_SOINFO is set in this->flags.
    uint32_t version_;

    // version >= 0
    dev_t st_dev_;
    ino_t st_ino_;

    // dependency graph
    soinfo_list_t children_;
    soinfo_list_t parents_;

    // version >= 1
    off64_t file_offset_;
    uint32_t rtld_flags_;
    uint32_t dt_flags_1_;
    size_t strtab_size_;

    // version >= 2

    size_t gnu_nbucket_;
    uint32_t *gnu_bucket_;
    uint32_t *gnu_chain_;
    uint32_t gnu_maskwords_;
    uint32_t gnu_shift2_;
    ElfW(Addr) *gnu_bloom_filter_;

    soinfo *local_group_root_;

    uint8_t *android_relocs_;
    size_t android_relocs_size_;

    const char *soname_;
    std::string realpath_;

    const ElfW(Versym) *versym_;

    ElfW(Addr) verdef_ptr_;
    size_t verdef_cnt_;

    ElfW(Addr) verneed_ptr_;
    size_t verneed_cnt_;

    uint32_t target_sdk_version_;

    // version >= 3
    std::vector<std::string> dt_runpath_;
    android_namespace_t *primary_namespace_; // 指针类型其实可以使用void*进行代替，这里为了便于阅读，如果仍然适应原有模式，并不遗余力的引入其他头文件
    android_namespace_list_t secondary_namespaces_;
    uintptr_t handle_;

    // version >= 4
    ElfW(Relr) *relr_;
    size_t relr_count_;
};

// This function is used by dlvsym() to calculate hash of sym_ver
uint32_t calculate_elf_hash(const char *name);

const char *fix_dt_needed(const char *dt_needed, const char *sopath);

template<typename F>
void for_each_dt_needed(const soinfo *si, F action) {
    for (const ElfW(Dyn) *d = si->dynamic; d->d_tag != DT_NULL; ++d) {
        if (d->d_tag == DT_NEEDED) {
            action(fix_dt_needed(si->get_string(d->d_un.d_val), si->get_realpath()));
        }
    }
}

#define IS_GNU_HASH(SOINFO) (SOINFO->flags_ & FLAG_GNU_HASH)
#endif  /* __LINKER_SOINFO_H */
