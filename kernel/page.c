// page.c
#include "page.h"

extern void printf(const char *fmt, ...);

// 提取链接器脚本 os.ld 中定义的绝对物理地址符号
// 注意：在 C 语言中，获取链接器符号的地址必须当作数组或取址符来用
extern char kernel_end[]; 

// 空闲链表的节点结构（它不占额外内存，直接写在空闲页的头部！）
struct page {
    struct page *next;
};

// 链表的头指针
static struct page *free_list = 0;

// 一个极其经典的系统级宏：将地址 a 向上对齐到 size 的整数倍
// 例如，如果 kernel_end 是 0x80012004，向上对齐 4096 后变成 0x80013000
#define ALIGN_UP(a, size) (((a) + (size) - 1) & ~((size) - 1))

/*
 * 初始化物理页分配器
 */
void page_init(void) {
    // 1. 划定安全区：从内核结束的地方开始，必须 4KB 对齐
    unsigned long start = ALIGN_UP((unsigned long)kernel_end, PAGE_SIZE);

    // 2. 切蛋糕：以 4KB 为步长，遍历直到物理内存尽头
    int page_count = 0;
    for (unsigned long p = start; p < RAM_END; p += PAGE_SIZE) {
        // 把这块物理内存强转成一个链表节点
        struct page *page = (struct page *)p;
        
        // 经典的头插法：把新切下来的页挂到链表头上
        page->next = free_list;
        free_list = page;
        
        page_count++;
    }

    printf("[SYS] Physical Memory Allocator initialized.\n");
    printf("[SYS] Managed RAM: 0x%x -> 0x%x\n", start, RAM_END);
    printf("[SYS] Total free pages: %d (approx %d MB)\n", page_count, (page_count * 4096) / 1024 / 1024);
}

/*
 * 批发内存：向系统申请一页 (4KB) 物理内存
 */
void *alloc_page(void) {
    struct page *page = free_list;
    
    if (page) {
        // 从链表头上摘下一个节点
        free_list = page->next;
        
        // 【极其重要的系统安全规则】：分配出去的内存必须清零！
        // 如果不清零，这块内存可能残留着其他进程的密码或敏感数据。
        char *p = (char *)page;
        for (int i = 0; i < PAGE_SIZE; i++) {
            p[i] = 0;
        }
    } else {
        printf("[FATAL] Out of Memory (OOM)!\n");
        while(1); // 物理内存耗尽，系统直接宕机
    }
    
    return (void *)page;
}

/*
 * 归还内存：释放一页物理内存
 */
void free_page(void *p) {
    if (!p) return;
    
    // 把归还的内存当成节点，重新挂回链表头
    struct page *page = (struct page *)p;
    page->next = free_list;
    free_list = page;
}
int get_free_page_count(void) {
    int count = 0;
    struct page *p = free_list;
    while (p) {
        count++;
        p = p->next;
    }
    return count;
}