//
// Created by yananh on 2019/4/7.
//

#include "elf_log.h"
#include "linker/linker.h"
#include "jni_loader.h"
//#include "linker/link_support.h"

void printPhdr(const ElfW(Phdr) *phdr) {
    auto typeDesc = [](int type) {
        switch (type) {
            case PT_NULL:
                return "PT_NULL";
            case PT_LOAD:
                return "PT_LOAD";
            case PT_DYNAMIC:
                return "PT_DYNAMIC";
            case PT_INTERP:
                return "PT_INTERP";
            case PT_NOTE:
                return "PT_NOTE";
            case PT_SHLIB:
                return "PT_SHLIB";
            case PT_LOOS:
                return "PT_LOOS";
            case PT_HIOS:
                return "PT_HIOS";
            case PT_LOPROC:
                return "PT_LOPROC";
            case PT_HIPROC:
                return "PT_HIPROC";
            case PT_GNU_EH_FRAME:
                return "PT_GNU_EH_FRAME";
            case PT_GNU_STACK:
                return "PT_GNU_STACK";
        }
        return "PT_UNKOWN";
    };
    LOGD("Phdr Type=%s, filesz=%d, memsz=%d, flag=0x%X", typeDesc(phdr->p_type), phdr->p_filesz, phdr->p_memsz,
         phdr->p_flags);
}

static auto symbol_type_lambda = [](const char st_info) {
    switch (ELF32_ST_TYPE(st_info)) {
        case STT_NOTYPE:
            return "Unspecified type";
        case STT_OBJECT:
            return "Data object";
        case STT_FUNC:
            return "Function";
        case STT_SECTION:
            return "Section";
        case STT_FILE:
            return "Source file";
        case STT_COMMON:
            return "Uninitialized common block";
        case STT_TLS:
            return "TLS object";
        case STT_LOOS:
            return "TLS object";
        case STT_HIOS:
            return "specific semantics";
        case STT_LOPROC:
            return "reserved range for processor";
        case STT_HIPROC:
            return "specific semantics";
        default:
            return "";
    }
};

static auto symbol_bind_lambda = [](const char st_info) {
    switch (ELF32_ST_TYPE(st_info)) {
        case STB_LOCAL:
            return "Local symbol";
        case STB_GLOBAL:
            return "Global symbol";
        case STB_WEAK:
            return "like global - lower precedence";
        case STB_LOOS:
            return "Reserved range for operating system";
        case STB_HIOS:
            return "specific semantics";
        case STB_LOPROC:
            return "reserved range for processor";
        case STB_HIPROC:
            return "specific semantics";
        default:
            return "";
    }
};

void printSym(soinfo *si, ElfW(Sym) *sym) {
    if (!sym || !si) {
        return;
    }
    const char *symbol_name = si->strtab_ + sym->st_name;

    LOGD("Symbol Name=%-16s, value=0x%X, size=%d, type=%-16s，bind=%-37s",
         symbol_name,
         sym->st_value,
         sym->st_size,
         symbol_type_lambda(sym->st_info),
         symbol_bind_lambda(sym->st_info));
}

void printSyms(soinfo *si) {
//    if (IS_GNU_HASH(si)) {
//        LOGD("printSyms so：%s is gnu hash! \n", si->soname_);
//        return;
//    }
    unsigned symtab_cnt = IS_GNU_HASH(si) ? si->nchain_ : si->nchain_;
    LOGD("Symbol table '.dynsym' contains %u entries:\n", symtab_cnt);
    for (ElfW(Sym) *sym = si->symtab_; sym < si->symtab_ + symtab_cnt; ++sym) {
        printSym(si, sym);
    }
}


void printSoInfo(soinfo *si) {
    LOGD("soinfo=0x%X", si);
    LOGD("old_name_=%s", si->old_name_);
    LOGD("phnum=%d", si->phnum);
    LOGD("size=%d", si->size);
    LOGD("next=0x%X", si->next);
    LOGD("flags_=%d", si->flags_);
    LOGD("nbucket_=%d", si->nbucket_);
    LOGD("nchain_=%d", si->nchain_);
    LOGD("plt_rel_count_=%d", si->plt_rel_count_);
    LOGD("rel_count_=%d", si->rel_count_);
    LOGD("load_bias=%d", si->load_bias);
    LOGD("version_=%d", si->version_);
    LOGD("android_relocs_size_=%d", si->android_relocs_size_);
    LOGD("soname_=%s", si->soname_);
    LOGD("realpath_=%s", (si->realpath_).c_str());
    LOGD("target_sdk_version_=%d", si->target_sdk_version_);

    LOGD("%s", "--------------------------- Start Print Phdr ---------------------------");
    for (const ElfW(Phdr) *phdr = si->phdr; phdr < si->phdr + si->phnum; phdr++) {
        printPhdr(phdr);
    }
    LOGD("%s", "--------------------------- Finish Print Phdr ---------------------------");
    LOGD("%s", "--------------------------- Start Print Symbol ---------------------------");
    printSyms(si);
    LOGD("%s", "--------------------------- Finish Print Symbol ---------------------------");
}