// trap_handler.c
extern void printf(const char *fmt, ...);
extern void timer_load_next(void);
extern void task_yield(void);
extern void uart_putc(char c);

// 引入系统调用号
#include "syscall.h"
extern int current_task;
// 注意参数变多了！接收从汇编 a0~a5 传来的数据
unsigned long trap_handler(unsigned long cause, unsigned long epc, unsigned long tval, 
                           unsigned long arg0, unsigned long arg1, unsigned long sys_num) {
                           
    int is_interrupt = (cause & 0x8000000000000000L) != 0;
    unsigned long exception_code = cause & 0x7FFFFFFFFFFFFFFFL;

    // --- 处理硬件中断 (时钟) ---
    if (is_interrupt) {
        if (exception_code == 7) { 
            timer_load_next();
            task_yield(); 
            return epc; 
        }
    } 
    // --- 处理异常和系统调用 ---
    else {
        // 来自 U-mode (8) 或 M-mode (11) 的 ecall
       // 来自 U-mode (8) 或 M-mode (11) 的 ecall
        if (exception_code == 8 || exception_code == 11) {
            
            // 系统调用分发中心 (Syscall Dispatcher)
            switch (sys_num) {
                // 【修改】：拦截标准写入调用
                case SYS_WRITE:
                    // 目前我们直接暴风吸入 a0 作为字符打印。
                    // 未来如果支持 fd 和 buf，就要根据规范处理 a0(fd), a1(buf), a2(count) 了。
                    uart_putc((char)arg0); 
                    break;
                    
                // 【修改】：拦截标准让出调用
                case SYS_SCHED_YIELD:
                    task_yield();
                    break;
                    
                // 【新增】：可以顺手预留一个 EXIT 接口，防止非法调用导致崩溃
                case SYS_EXIT:
                    task_exit(arg0);
                    break;
                    
                default:
                    printf("\n[KERNEL] Invalid syscall number: %d\n", sys_num);
                    break;
            }
            
            return epc + 4;
        }

        // 依然保留我们的缺页诊断雷达
        if (exception_code == 12 || exception_code == 13 || exception_code == 15) {
            printf("\n!!!! MMU PAGE FAULT DETECTED !!!!\n");
            printf("Cause: %d, Bad VA: 0x%x, At EPC: 0x%x\n", exception_code, tval, epc);
            while(1); 
        }

        printf("\n!!! KERNEL PANIC: Cause %d, EPC 0x%x !!!\n", exception_code, epc);
        while(1);
    }
    
    return epc;
}