// main.c
extern void uart_init(void);
extern void printf(const char *fmt, ...);
extern void trap_vector(void);
extern void timer_init(void);
extern void task_init(int id, void (*func)(void));
extern void page_init(void);
extern void mmu_init(void);
extern unsigned long get_task_satp(int id);

// 引入属于用户的 API 库
#include "syscall.h"
// main.c
// main.c
void task_a(void) {
    sys_print("\n => Process A: Reading from Read-Only page (0x50000000)...\n");
    
    // 我们自己指定地址，不依赖 C 语言的指针强转
    unsigned long target_addr = 0x50000000;
    
    // 读操作：这步应该成功
    int val = *(volatile int *)target_addr; 
    sys_print(" => Process A: Read successful!\n");
    
    sys_print(" => Process A: Forcing WRITE via raw Assembly...\n");
    
    // 【终极杀招】：强行向 CPU 注入一条 sw (Store Word) 汇编指令！
    // 无论 GCC 怎么优化，这句话绝对会被硬生生刻进二进制文件里。
    asm volatile(
        "sw %0, 0(%1)\n"
        : 
        : "r"(999), "r"(target_addr)
        : "memory"
    );
    
    sys_print(" => ERROR: If you see this, MMU Write-Protection completely failed!\n");
    while(1);
}

void task_b(void) {
    while(1) {
        sys_print("B");
        for (volatile int i = 0; i < 15000000; i++);
    }
}
// void task_a(void) {
//     sys_print("\n => Process A: I will try to hack the Kernel in 3 seconds...\n");
//     for (volatile int i = 0; i < 30000000; i++); // 等待一会
    
//     sys_print("\n => Process A: Hacking NULL pointer!!!\n");
    
//     // 恶意操作：试图往虚拟地址 0x00000000 写入数据！
//     // 我们的 MMU 页表里根本没有映射这个地址，且它没有 U 权限。
//     volatile int *illegal_ptr = (int *)0x00000000;
//     *illegal_ptr = 999; 
    
//     sys_print("If you see this, the sandbox FAILED.\n");
// }

// void task_b(void) {
//     while(1) {
//         sys_print("B");
//         for (volatile int i = 0; i < 5000000; i++);
//     }
// }

// void task_a(void) {
//     sys_print("\n => Process A entering infinite silent loop...\n");
//     // 纯纯的死循环，没有任何系统调用，没有任何延时，全速空转！
//     while(1) { } 
// }

// void task_b(void) {
//     sys_print("\n => Process B started.\n");
//     while(1) {
//         sys_print("B");
//         for (volatile int i = 0; i < 15000000; i++);
//     }
// }

// void task_a(void) {
//     sys_print("\n => Process A started.\n");
//     while(1) {
//         sys_print("A");
//         // A 非常谦虚，打印一个字符后，立刻主动呼叫内核交出 CPU！
//         sys_yield(); 
//     }
// }

// void task_b(void) {
//     sys_print("\n => Process B started.\n");
//     while(1) {
//         sys_print("B");
//         // B 非常贪婪，死死霸占 CPU，直到 1 秒后被定时器强行踢下去
//         for (volatile int i = 0; i < 15000000; i++); 
//     }
// }
// 用户态进程 A
// void task_a(void) {
//     sys_print("\n => Process A started in strictly isolated U-mode.\n");
    
//     while(1) {
//         sys_print("A");
//         for (volatile int i = 0; i < 150000000; i++);
//     }
// }

// // 用户态进程 B
// void task_b(void) {
//     sys_print("\n => Process B started in strictly isolated U-mode.\n");
    
//     while(1) {
//         sys_print("B");
//         for (volatile int i = 0; i < 150000000; i++);
//     }
// }

void os_main(void) {
    uart_init();
    printf("\n[SYS] riscv OS: Syscall & Privilege Drop Booting...\n");
    
    page_init();
    mmu_init();

    asm volatile("csrw pmpaddr0, %0" : : "r"(0xffffffffffffffffULL));
    asm volatile("csrw pmpcfg0, %0" : : "r"(0x1f));
    
    asm volatile("csrw mtvec, %0" : : "r"(trap_vector));
    
    task_init(0, task_a);
    task_init(1, task_b);
    timer_init();

    printf("[SYS] System Initialization Complete. Giving up privileges...\n\n");

    unsigned long first_satp = get_task_satp(0);
    asm volatile("csrw satp, %0" : : "r"(first_satp));
    asm volatile("sfence.vma zero, zero");

    extern void task_wrapper(void);
    task_wrapper(); 
}