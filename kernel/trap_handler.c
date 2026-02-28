// trap_handler.c
extern void printf(const char *fmt, ...);
extern void timer_load_next(void);
extern void task_yield(void);
extern void uart_putc(char c);

// 引入系统调用号
#include "syscall.h"

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
        if (exception_code == 8 || exception_code == 11) {
            
            // 系统调用分发中心 (Syscall Dispatcher)
            switch (sys_num) {
                case SYS_WRITE_CHAR:
                    // 只有在 M-mode 下，调用 uart_putc 才是绝对安全的
                    uart_putc((char)arg0); 
                    break;
                case SYS_YIELD:
                    task_yield();
                    break;
                default:
                    printf("\n[KERNEL] Invalid syscall number: %d\n", sys_num);
                    break;
            }
            
            // 极其关键：系统调用完成后，PC 必须 +4 跳过原来的 ecall 指令
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