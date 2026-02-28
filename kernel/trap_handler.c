// trap_handler.c
extern void printf(const char *fmt, ...);
extern void timer_load_next(void);
extern void task_yield(void);

// 【修改点 1】：返回值改为 unsigned long
unsigned long trap_handler(unsigned long cause, unsigned long epc, unsigned long tval) {
    int is_interrupt = (cause & 0x8000000000000000L) != 0;
    unsigned long exception_code = cause & 0x7FFFFFFFFFFFFFFFL;

    if (is_interrupt) {
        if (exception_code == 7) { 
            timer_load_next();
            task_yield(); 
            // 【修改点 2】：正常的定时器中断，原路返回 (epc)
            return epc; 
        }
    } else {
        if (exception_code == 11 || exception_code == 8) {
            printf("[KERNEL] Syscall intercepted! (ecall from User Space)\n");
            
            // 【修改点 3】：系统调用，返回下一条指令 (epc + 4)
            // 注意这里不再使用 asm volatile 去写 mepc 了！
            return epc + 4; 
        }

        printf("\n!!! KERNEL PANIC !!!\n");
        printf("mcause : 0x%x \n", cause);
        while(1) {}
    }
    return epc; // 默认兜底
}