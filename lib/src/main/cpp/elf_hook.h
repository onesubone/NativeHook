//
// Created by yananh on 2019/5/10.
//
#ifndef RUBICK_ELF_HOOK_H
#define RUBICK_ELF_HOOK_H

#if defined(__LP64__)
#define ELF_R_SYM(info)  ELF64_R_SYM(info)
#define ELF_R_TYPE(info) ELF64_R_TYPE(info)
#else
#define ELF_R_SYM(info)  ELF32_R_SYM(info)
#define ELF_R_TYPE(info) ELF32_R_TYPE(info)
#endif


struct hook_symbol {
    const char *so_name;
    const char *symbol_name;
    void *new_value;
};

#endif
