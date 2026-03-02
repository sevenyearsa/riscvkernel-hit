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

// main.c 修改任务代码

void task_a(void) {
    sys_print("\n => Process A started. I will work for 5 cycles and then die.\n");
    
    // A 只打印 5 次就自杀
    for (int i = 0; i < 5; i++) {
        sys_print("A");
        for (volatile int j = 0; j < 15000000; j++);
    }
    
    sys_print("\n => Process A finished its job. Calling sys_exit(0)...\n");
    sys_exit(0); 
}

void task_b(void) {
    sys_print("\n => Process B started. I am an infinite daemon.\n");
    
    // B 永远运行
    while(1) {
        sys_print("B");
        for (volatile int i = 0; i < 15000000; i++);
        // sys_exit(1); 
    }
}
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