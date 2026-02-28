// user_lib.c
#include "syscall.h"

/*
 * 向内核发送 打印字符 的系统调用
 * (未来可以通过实现真正的 sys_write(1, &c, 1) 来替代)
 */
void sys_putchar(char c) {
    asm volatile(
        "mv a7, %0\n"    // 【修改】：使用标准的 SYS_WRITE (64)
        "mv a0, %1\n"    
        "ecall\n"        
        : 
        : "r"(SYS_WRITE), "r"(c)
        : "a0", "a7"     
    );
}

void sys_print(const char *s) {
    while (*s) {
        sys_putchar(*s++);
    }
}

/*
 * 向内核发送 任务主动休眠 的系统调用
 */
void sys_yield(void) {
    asm volatile(
        "mv a7, %0\n"    // 【修改】：使用标准的 SYS_SCHED_YIELD (124)
        "ecall\n"
        : 
        : "r"(SYS_SCHED_YIELD)
        : "a7"
    );
}