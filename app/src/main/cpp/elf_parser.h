//
// Created by yananh on 2019/4/27.
//

#ifndef RUBICK_ELF_PARSER_H
#define RUBICK_ELF_PARSER_H
#include <sys/mman.h>
#include <elf.h>
#include <linux/elf.h>
#include <link.h>

//typedef struct dynamic {
//    Elf32_Sword d_tag;
//    union {
//        Elf32_Sword d_val;
//        Elf32_Addr d_ptr;
//    } d_un;
//} Elf32_Dyn;
//typedef struct {
//    Elf64_Sxword d_tag;
//    union {
//        Elf64_Xword d_val;
//        Elf64_Addr d_ptr;
//    } d_un;
//} Elf64_Dyn;
typedef ElfW(Dyn) _DYN;


// Same as in DT_HASH, symbols are put in one of nbuckets buckets depending on their hashes.
// To be specific, each symbol should be placed into hash % nbuckets bucket.
struct {
    uint32_t nbuckets;
    uint32_t symoffset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
    uint32_t *bloom; /* bloom[bloom_size], uint64_t for 64-bit binaries */
    uint32_t *buckets; /* buckets[nbuckets] */
    uint32_t chain[];
} _gnu_hash_table_32;

struct {
    uint32_t nbuckets;
    uint32_t symoffset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
    uint64_t *bloom; /* bloom[bloom_size], uint32_t for 32-bit binaries */
    uint32_t *buckets; /* buckets[nbuckets] */
    uint32_t chain[];
} _gnu_hash_table_64;






size_t  get_library_address(const char*  libname)
{
    char path[256];
    char buff[256];
    int len_libname = strlen(libname);
    FILE* file;
    size_t  addr = 0;

    snprintf(path, sizeof path, "/proc/%d/smaps", getpid());
    file = fopen(path, "rt");
    if (file == NULL)
        return 0;

    while (fgets(buff, sizeof buff, file) != NULL) {
        int  len = strlen(buff);
        if (len > 0 && buff[len-1] == '\n') {
            buff[--len] = '\0';
        }
        if (len <= len_libname || memcmp(buff + len - len_libname, libname, len_libname)) {
            continue;
        }
        size_t start, end, offset;
        char flags[4];
        if (sscanf(buff, "%zx-%zx %c%c%c%c %zx", &start, &end,
                   &flags[0], &flags[1], &flags[2], &flags[3], &offset) != 7) {
            continue;
        }
        if (flags[0] != 'r' || flags[2] != 'x') {
            continue;
        }
        addr = start - offset;
        break;
    }
    fclose(file);
    return addr;
}

struct

soinfo *parse(void *ptr);

#endif //RUBICK_ELF_PARSER_H
