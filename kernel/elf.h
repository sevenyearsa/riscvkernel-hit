// kernel/elf.h
#ifndef ELF_H
#define ELF_H

#include "stdint.h"

// ELF 文件头 (描述程序的入口点和段表位置)
typedef struct {
    unsigned char e_ident[16]; // 包含 0x7f, 'E', 'L', 'F'
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;     // 【极其重要】程序的起始执行地址！
    uint64_t      e_phoff;     // Program Header 的偏移量
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;     // 到底有几个段？
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf64_Ehdr;

// Program Header (描述代码段、数据段应该被加载到内存的哪里)
typedef struct {
    uint32_t p_type;           // 段类型 (1 代表 PT_LOAD，即需要加载到内存的段)
    uint32_t p_flags;
    uint64_t p_offset;         // 这个段在文件(数组)中的位置
    uint64_t p_vaddr;          // 这个段要放到虚拟内存的哪个地址？
    uint64_t p_paddr;
    uint64_t p_filesz;         // 在文件中的大小
    uint64_t p_memsz;          // 在内存中的大小 (如果 memsz > filesz，多出来的就是 BSS 段，需清零)
    uint64_t p_align;
} Elf64_Phdr;

#endif