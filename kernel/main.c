// main.c
extern void uart_init(void);
extern void printf(const char *fmt, ...);
extern void trap_vector(void);
extern void timer_init(void);
extern void task_init(int id, void (*func)(void));

void task_a(void) {
    printf(" => Task A is running...\n");
    
    // 【系统调用演示】：Task A 发射 ecall 指令
    printf(" => Task A: Executing ecall to request Kernel service...\n");
    asm volatile("ecall"); // 这条指令会瞬间陷进 trap_handler！
    
    printf(" => Task A: Returned from Kernel safely!\n");
    
    while(1) {
        // 让出 CPU，演示平滑运行
        printf(" => Task A is running...\n");
        for (volatile int i = 0; i < 5000000; i++);
    }
}

void task_b(void) {
    while(1) {
        printf(" => Task B is running...\n");
        for (volatile int i = 0; i < 5000000; i++);
    }
}

void os_main(void) {
    uart_init();
    printf("\n[SYS] uniproton OS: Mutex & Syscall Ready\n");
    
    asm volatile("csrw mtvec, %0" : : "r"(trap_vector));
    task_init(0, task_a);
    task_init(1, task_b);
    timer_init();

    extern void task_wrapper(void);
    asm volatile("csrsi mstatus, 8");
    task_a(); 
}