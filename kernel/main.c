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
extern void sys_exec(void); // 引入执行系统调用

void task_a(void) {
    sys_print("\n => Process A: I am the father. Calling fork()...\n");
    int pid = sys_fork();

    if (pid == 0) {
        // --- 子进程区域 ---
        sys_print("\n => [CHILD] I am the clone! I will now sacrifice my memory to the ELF...\n");
        sys_print(" => [CHILD] Calling execve(). GOODBYE ORIGINAL SOUL!\n");
        
        // 呼叫夺舍机制！
        sys_exec();
        
        // 如果 exec 成功，CPU 的 PC 指针会直接跳入 app.c 的 _start 函数
        // 下面这句话永远不可能被打印！
        sys_print(" => ERROR: If you see this, exec failed miserably!\n");
        while(1);
    } else if (pid > 0) {
        // --- 父进程区域 ---
        sys_print("\n => [PARENT] My son went to run an App. I will continue my work.\n");
        while(1) {
            sys_print("P");
            for (volatile int i = 0; i < 15000000; i++);
        }
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