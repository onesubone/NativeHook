//
// Created by yananh on 2019/4/7.
//

#ifndef NATIVEHOOK_ELF_LOG_H
#define NATIVEHOOK_ELF_LOG_H

#include "linker/linker.h"
#include <sys/mman.h>

void printSoInfo(soinfo *so_info);

void printPhdr(ElfW(Phdr) *phdr);

void printSym(soinfo *so_info, ElfW(Sym) *sym);

void printSyms(soinfo *si);


#endif //NATIVEHOOK_ELF_LOG_H
