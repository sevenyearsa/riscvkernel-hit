// user_lib.c
#include "syscall.h"

/*
 * 向内核发送 打印字符 的系统调用
 */
void sys_putchar(char c) {
    // RISC-V 系统调用规矩：a7 放业务号，a0 放参数 1
    asm volatile(
        "mv a7, %0\n"    // 填入系统调用号 1
        "mv a0, %1\n"    // 填入要打印的字符
        "ecall\n"        // 拨打 110 求助内核！
        : 
        : "r"(SYS_WRITE_CHAR), "r"(c)
        : "a0", "a7"     // 告诉编译器，这两个寄存器被我修改了
    );
}

/*
 * 包装函数：打印整个字符串
 */
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
        "mv a7, %0\n"
        "ecall\n"
        : 
        : "r"(SYS_YIELD)
        : "a7"
    );
}