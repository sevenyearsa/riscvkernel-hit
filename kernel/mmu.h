// mmu.h
#ifndef MMU_H
#define MMU_H

// 页大小 4KB
#define PAGE_SIZE 4096

// --- RISC-V Sv39 页表项 (PTE) 标志位 ---
#define PTE_V (1L << 0) // Valid: 存在位（如果不为 1，访问必死机）
#define PTE_R (1L << 1) // Read: 允许读
#define PTE_W (1L << 2) // Write: 允许写
#define PTE_X (1L << 3) // Execute: 允许执行 (代码段必须有这个)
#define PTE_U (1L << 4) // User: 允许 U-mode 用户态访问！(核心隔离标记)
#define PTE_A (1L << 6) // Accessed
#define PTE_D (1L << 7) // Dirty


// 组合权限
// 内核代码权限：可读、可执行
#define PAGE_KERNEL_RX  (PTE_R | PTE_X | PTE_V)
// 内核数据权限：可读、可写
#define PAGE_KERNEL_RW  (PTE_R | PTE_W | PTE_V)
// 用户态权限：可读、可写、可执行、并且允许用户态访问
#define PAGE_USER_RWX   (PTE_R | PTE_W | PTE_X | PTE_U | PTE_V)

// 核心类型定义，使用 64 位无符号整数表示地址
typedef unsigned long uint64_t;

void mmu_init(void);

extern uint64_t root_page_table;
void map_page(uint64_t *pgd, uint64_t va, uint64_t pa, uint64_t flags);
#endif