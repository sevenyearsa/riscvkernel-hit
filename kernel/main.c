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

extern int sys_fork(void);

void task_a(void) {
    sys_print("\n => Process A: I am the father. Calling fork()...\n");
    
    // 【见证奇迹的时刻】
    int pid = sys_fork();

    if (pid == 0) {
        // --- 这段代码只有子进程才会执行！---
        sys_print("\n => [CHILD] Hello from the cloned universe! I am the son.\n");
        while(1) {
            sys_print("S"); // Son
            for (volatile int i = 0; i < 15000000; i++);
        }
    } else if (pid > 0) {
        // --- 这段代码只有父进程才会执行！---
        sys_print("\n => [PARENT] I created a son! Returning to my work.\n");
        while(1) {
            sys_print("P"); // Parent
            for (volatile int i = 0; i < 15000000; i++);
        }
    } else {
        sys_print("\n => Fork failed!\n");
        while(1);
    }
}

void task_b(void) {
    sys_print("\n => Process B: I am an observer.\n");
    while(1) {
        sys_print("B");
        for (volatile int i = 0; i < 20000000; i++);
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