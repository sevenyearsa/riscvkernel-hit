// mmu.c
#include "mmu.h"

extern void *alloc_page(void);
extern void printf(const char *fmt, ...);

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

/*
 * 【核心魔法】：将一个物理地址 pa 映射到虚拟地址 va
 */
void map_page(uint64_t *pgd, uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t *table = pgd;

    for (int level = 2; level > 0; level--) {
        uint64_t vpn = get_vpn(va, level);
        uint64_t *pte = &table[vpn];

        if (*pte & PTE_V) {
            table = (uint64_t *)pte_to_pa(*pte);
        } else {
            uint64_t new_page = (uint64_t)alloc_page();
            // 【关键修复】：中间节点只能有 PTE_V！
            // 绝对不能在这里放 flags，否则硬件会把它误认为是一个“巨页”映射
            *pte = pa_to_pte(new_page, PTE_V); 
            table = (uint64_t *)new_page;
        }
    }

    uint64_t vpn0 = get_vpn(va, 0);
    // 只有叶子节点才写入真正的权限和 A/D 位
    table[vpn0] = pa_to_pte(pa, flags | PTE_A | PTE_D);
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
        map_page((uint64_t *)root_page_table, pa, pa, PTE_R | PTE_W | PTE_X | PTE_U | PTE_V);
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