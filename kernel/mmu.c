// mmu.c
#include "mmu.h"

extern void *alloc_page(void);
extern void printf(const char *fmt, ...);
extern void free_page(void *p);
extern char kernel_end[];

// 内核的顶级页表（Root Page Table）物理地址
uint64_t root_page_table = 0;

/*
 * 提取虚拟地址的 VPN (Virtual Page Number)
 * level: 2 代表顶级, 1 代表中级, 0 代表末级
 */
static inline uint64_t get_vpn(uint64_t va, int level) {
    // Sv39 的 VPN 分别在位 [38:30], [29:21], [20:12]
    // 每个 VPN 长 9 位 (所以能表示 512 个表项)
    return (va >> (12 + level * 9)) & 0x1FF;
}

/*
 * 从 PTE (页表项) 中提取下一级物理页的地址
 */
static inline uint64_t pte_to_pa(uint64_t pte) {
    // 物理页号 (PPN) 存在 PTE 的 [53:10] 位，提取出来后左移 12 位加上全 0 偏移，还原物理地址
    return (pte >> 10) << 12;
}

/*
 * 将物理地址和权限打包成一个 PTE
 */
static inline uint64_t pa_to_pte(uint64_t pa, uint64_t flags) {
    // 将物理地址的物理页号放入 [53:10]，并在末尾加上标志位
    return ((pa >> 12) << 10) | flags;
}

uint64_t *walk_pte(uint64_t *pgd, uint64_t va, int create) {
    uint64_t *table = pgd;

    for (int level = 2; level > 0; level--) {
        uint64_t vpn = get_vpn(va, level);
        uint64_t *pte = &table[vpn];

        if (*pte & PTE_V) {
            table = (uint64_t *)pte_to_pa(*pte);
            continue;
        }

        if (!create) {
            return 0;
        }

        uint64_t new_page = (uint64_t)alloc_page();
        *pte = pa_to_pte(new_page, PTE_V);
        table = (uint64_t *)new_page;
    }

    return &table[get_vpn(va, 0)];
}

/*
 * 【核心魔法】：将一个物理地址 pa 映射到虚拟地址 va
 */
void map_page(uint64_t *pgd, uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t *pte = walk_pte(pgd, va, 1);
    // 只有叶子节点才写入真正的权限和 A/D 位
    *pte = pa_to_pte(pa, flags | PTE_A | PTE_D);
}
/*
 * MMU 初始化与点火
 */
void mmu_init(void) {
    root_page_table = (uint64_t)alloc_page();

    // 【新增】：映射 CLINT (定时器)，否则开启 MMU 后心脏会停！
    // 基址 0x2000000，映射 64KB 足够覆盖 mtime 和 mtimecmp
    for(uint64_t p = 0x2000000; p < 0x2010000; p += PAGE_SIZE) {
        map_page((uint64_t *)root_page_table, p, p, PTE_R | PTE_W | PTE_V);
    }

    // 恒等映射 UART
    map_page((uint64_t *)root_page_table, 0x10000000, 0x10000000, PTE_R | PTE_W  | PTE_V);

    // 恒等映射 RAM (0x80000000 - 0x88000000)
    uint64_t ram_start = 0x80000000L;
    uint64_t ram_size  = 128 * 1024 * 1024;
    for (uint64_t pa = ram_start; pa < ram_start + ram_size; pa += PAGE_SIZE) {
        map_page((uint64_t *)root_page_table, pa, pa, PTE_R | PTE_W | PTE_X | PTE_V);
    }
    printf("[SYS] Page tables built. Root PGD at physical address: 0x%x\n", root_page_table);

    // 4. 【深渊一跃：激活 satp 寄存器】
    // satp 寄存器的第 63~60 位是 MODE：8 代表开启 Sv39 分页。
    // 第 43~0 位是顶级页表的物理页号 (PPN，即物理地址右移 12 位)。
    uint64_t satp = (8ULL << 60) | (root_page_table >> 12);

    // sfence.vma 是一条极其关键的指令：清空 TLB (硬件地址缓存)，防止它记住没开 MMU 前的垃圾数据
    asm volatile("sfence.vma zero, zero");
    
    // 轰！就在这条指令执行完毕的纳秒级瞬间，物理世界被彻底屏蔽，虚拟宇宙诞生！
    asm volatile("csrw satp, %0" : : "r"(satp));
    
    // 再次刷新缓存，确保万无一失
    asm volatile("sfence.vma zero, zero");

    printf("[SYS] MMU activated! System is now running in Virtual Memory space.\n");
}

void free_user_page_table(uint64_t *pgd) {
    uint64_t *root_pgd = (uint64_t *)root_page_table;

    // 遍历顶级页表 (PGD) 的 512 个槽位
    for (int i = 0; i < 512; i++) {
        // 1. 【安全锁】：如果这个表项和内核一模一样，说明是共享的内核空间（如 UART, RAM）
        // 我们只是借用，绝对不能销毁！
        if (pgd[i] == root_pgd[i]) {
            continue;
        }

        // 2. 如果不一样，且 V 位为 1，说明这是该进程私有开辟的空间！
        if (pgd[i] & PTE_V) {
            uint64_t *pmd = (uint64_t *)pte_to_pa(pgd[i]);
            
            // 遍历第二级页表 (PMD)
            for (int j = 0; j < 512; j++) {
                if (pmd[j] & PTE_V) {
                    uint64_t *pte_table = (uint64_t *)pte_to_pa(pmd[j]);
                    
                    // 遍历第三级页表 (叶子节点 PTE)
                    for (int k = 0; k < 512; k++) {
                        if (pte_table[k] & PTE_V) {
                            // 【斩草除根】：拔掉存有用户真实数据的物理页！
                            uint64_t leaf_pa = pte_to_pa(pte_table[k]);
                            if (leaf_pa >= (uint64_t)kernel_end) {
                                free_page((void *)leaf_pa);
                            }
                        }
                    }
                    // 拔掉第三级页表本身！
                    free_page(pte_table); 
                }
            }
            // 拔掉第二级页表本身！
            free_page(pmd);
        }
    }
    // 最后，把这张顶级页表（PGD）本身也给扬了！
    free_page(pgd); 
}

uint64_t *copy_user_page_table(uint64_t *src_pgd) {
    // 为子进程申请一张全新的顶级页表
    uint64_t *dst_pgd = (uint64_t *)alloc_page();
    uint64_t *root_pgd = (uint64_t *)root_page_table;

    for (int i = 0; i < 512; i++) {
        if (src_pgd[i] == root_pgd[i]) {
            // 内核的共享设施（如 UART, RAM 代码区），直接抄指针，不用深拷贝
            dst_pgd[i] = src_pgd[i];
            continue;
        }

        if (src_pgd[i] & PTE_V) {
            uint64_t *src_pmd = (uint64_t *)pte_to_pa(src_pgd[i]);
            uint64_t *dst_pmd = (uint64_t *)alloc_page();
            // 复制中间页表路标，保留原有的权限位 (低10位)
            dst_pgd[i] = pa_to_pte((uint64_t)dst_pmd, src_pgd[i] & 0x3FF);

            for (int j = 0; j < 512; j++) {
                if (src_pmd[j] & PTE_V) {
                    uint64_t *src_pte_table = (uint64_t *)pte_to_pa(src_pmd[j]);
                    uint64_t *dst_pte_table = (uint64_t *)alloc_page();
                    dst_pmd[j] = pa_to_pte((uint64_t)dst_pte_table, src_pmd[j] & 0x3FF);

                    for (int k = 0; k < 512; k++) {
                        if (src_pte_table[k] & PTE_V) {
                            // 触底！找到了父进程真实的物理数据页！
                            uint64_t src_data_pa = pte_to_pa(src_pte_table[k]);
                            // 为子进程申请一块独立的新物理地皮
                            uint64_t dst_data_pa = (uint64_t)alloc_page();

                            // 极其暴力而优雅的深拷贝：逐字节复刻 4096 字节的数据！
                            char *src_data = (char *)src_data_pa;
                            char *dst_data = (char *)dst_data_pa;
                            for (int b = 0; b < 4096; b++) {
                                dst_data[b] = src_data[b];
                            }

                            // 映射给子进程，权限与父进程完全一致
                            dst_pte_table[k] = pa_to_pte(dst_data_pa, src_pte_table[k] & 0x3FF);
                        }
                    }
                }
            }
        }
    }
    return dst_pgd;
}
