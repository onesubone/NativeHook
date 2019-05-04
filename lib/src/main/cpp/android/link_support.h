//
// Created by yananh on 2019/3/27.
//
#pragma once

#include <elf.h>
#include <linux/elf.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <unistd.h>

typedef struct {
    /**
     *  在.strtab section 中的索引，表示符号名
     */
    Elf32_Word st_name;      // 符号名。如果该值非0，则表示符号名的字符串表索引(offset)，否则符号表项没有名称。注:外部 C 符号在 C 语言和目标文件的符号表中具有相同的名称。
    /**
     * ● 如果符号不是”COMMON”类型的(即st_shndx不为SHN_COMMON)，则st_value表示该符号在节区中的偏移，
     * 即符号所对应的函数或变量位于由st_shndx指定的节区，偏移st_value的位置。
     * 就是说，st_value 是从 st_shndx 所标识的节区头部开始计算，到符号位置的偏移为st_value。
     * 比如so中存在say_hello方法，其st_shndx指向12，表示第12个节区(一般是.text节区)，st_size为指令码的长度，st_value为so中的偏移地址
     * ● 在目标文件中，如果符号是”COMMON”类型(即st_shndx为SHN_COMMON)，则st_value表示该符号的对齐属性。
     * ● 在可执行和共享目标文件中，st_value表示符号的虚拟地址。为了使得这些文件的符号对动态链接器更有用，
     * 节区偏移（针对文件的解释）让位于虚拟地址（针对内存的解释），因为这时与节区号无关。
     */
    Elf32_Addr st_value;       // 符号相对应的值。这个值跟符号相关，可能是一个绝对值、一个地址等，不同的符号，它所对应的值含义不同。
    Elf32_Word st_size;         // 符号大小。对于包含数据的符号，这个值是该数据类型的大小，比如一个double型的符号它占用8各字节。如果该值为0，则表示符号大小为0或者大小未知。
    unsigned char st_info;    // 符号的类型和绑定属性。下面给出若干取值和含义的绑定关系。
    unsigned char st_other;  // 没用，目前都为0。
    /**
     * 如果符号定义在本目标文件中，那么这个成员表示符号所在节区在节区表中的下标，但是如果符号不是定义在本目标文件中，或者对于有些特殊符号，sh_shndx的值有些特殊。如下：
     * SHN_ABS（0xFFF1）：该符号包含一个绝对值，不会因为重定位而发生变化，比如文件名的符号就属于此类。
     * SHN_COMMON（0xFFF2）： 表示该符号是一个"COMMON块"类型的符号，一般来说，未初始化的全局定义就是这种类型。符号标注了一个尚未分配的公共块。符号的取值给出了对齐约束，与节区的 sh_addralign成员类似。就是说，链接编辑器将为符号分配存储空间，地址位于 st_value 的倍数处。符号的大小给出了所需要的字节数。
     * SHN_UNDEF（0）： 表示该符号未定义，这个符号表示该符号在本目标文件被引用，但是定义在其他目标文件中。
     *      当链接编辑器将此目标文件与其他定义了该符号的目标文件进行组合时，此文件中对该符号的引用将被链接到实际定义的位置。
     */
    Elf32_Half st_shndx;        // 符号所在节区。此成员给出相关的节区头部表索引。
} Elf32_sym;

/**
 * 符号绑定信息
 * 名称           取值          说明
 * STB_LOCAL      0            局部符号在包含该符号定义的目标文件以外不可见，相同名称的局部符号可以存在于多个文件中，互不影响。
 * STB_GLOBAL     1            全局符号对所有将组合的目标文件都是可见的。一个文件中对某个全局符号的定义将满足另一个文件对相同全局符号的未定义引用。
 * STB_WEAK       2            弱符号与全局符号类似，不过他们的定义优先级比较低。
 * STB_LOPROC     13           这个范围的取值是保留给处理器专用语义的。
 * STB_HIPROC     15           处于这个范围的取值是保留给处理器专用语义的。
 */
#define ELF32_ST_BIND(i) ((i)>>4)
/**
 * 名称           取值      说明
 * STT_NOTYPE     0         未知类型符号
 * STT_OBJECT     1         该符号是一个数据对象，比如一个变量、数组等等
 * STT_FUNC       2         该符号是一个函数或者其他可执行代码
 * STT_SECTION    3         该符号必须是一个节区这种类型的符号表项主要用于重定位，通常具有 STB_LOCAL 绑定。
 * STT_FILE       4         该符号表示文件名一般都是该目标文件所对应的源文件名称。其一定为STB_LOCAL 类型，并且他的st_shndx一定是SHN_ABS
 * STT_LOPROC     13        此范围的符号类型值保留给处理器专用语义用途。
 * STT_HIPROC     15        此范围的符号类型值保留给处理器专用语义用途。
 * 在共享目标文件中的函数符号（类型为 STT_FUNC）具有特别的重要性。当其他目标文件引用了来自某个共享目标中的函数时，
 * 链接编辑器自动为所引用的符号创建过程链接表项(PLT)。类型不是 STT_FUNC 的共享目标符号不会自动通过过程链接表(PLT)进行引用。
 */
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b, t) (((b)<<4) + ((t)&0xf))





//typedef struct {
//    Elf32_Addr r_offset;    // 重定位入口偏移（受影响的存储单位的第一个字节的偏移或者虚拟地址）
//    Elf32_Word r_info;     // 重定位入口的类型和符号，要进行重定位的符号表索引，以及将实施的重定位类型（哪些位需要修改，以及如何计算它们的取值）
//    // 其中 .rel.dyn 重定位类型一般为R_386_GLOB_DAT和R_386_COPY；.rel.plt为R_386_JUMP_SLOT
//} Elf32_Rel;
//typedef struct {
//    Elf32_Addr r_offset;
//    Elf32_Word r_info;
//    Elf32_Word r_addend;  // 给出一个常量补齐，用来计算将被填充到可重定位字段的数值。
//} Elf32_Rela;
//重定位节区会引用两个其它节区: 符号表、要修改的节区


















