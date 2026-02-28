// syscall.h
#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE_CHAR 1   // 打印一个字符业务
#define SYS_YIELD      2   // 主动让出 CPU 业务

// 暴露给用户程序的标准库 API
void sys_putchar(char c);
void sys_print(const char *s);
void sys_yield(void);

#endif