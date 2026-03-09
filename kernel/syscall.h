// syscall.h
#ifndef SYSCALL_H
#define SYSCALL_H

// =========================================================
// 标准 Linux RISC-V 64 系统调用号 (基于 asm-generic/unistd.h)
// =========================================================

// --- 1. 文件与 I/O ---
#define SYS_OPENAT      56   // 打开文件 (在相对路径下)
#define SYS_CLOSE       57   // 关闭文件
#define SYS_READ        63   // 从文件描述符读取数据
#define SYS_WRITE       64   // 向文件描述符写入数据 (原 SYS_WRITE_CHAR 升级版)

// --- 2. 进程管理 ---
#define SYS_EXIT        93   // 进程自我终止
#define SYS_KILL        129  // 发送信号杀进程
#define SYS_GETPID      172  // 获取进程 ID
#define SYS_CLONE       220  // 克隆进程 (Linux 中 fork 的底层实现)
#define SYS_EXECVE      221  // 替换当前进程执行新程序 (执行 elf)
#define SYS_WAIT4       260  // 等待子进程退出并回收资源

// --- 3. 内存管理 ---
#define SYS_BRK         214  // 改变数据段的结尾 (传统的 malloc 底层)
#define SYS_MMAP        222  // 映射虚拟内存 (现代的 malloc 底层)
#define SYS_MUNMAP      215  // 解除内存映射

// --- 4. 调度与时间 ---
#define SYS_SCHED_YIELD 124  // 主动让出 CPU (原 SYS_YIELD)
#define SYS_GETTIMEOFDAY 169 // 获取高精度系统时间

// =========================================================
// 用户态基础 C 库 (libc) 接口声明
// =========================================================

// 现有功能的标准化平替
void sys_putchar(char c); 
void sys_print(const char *s);
void sys_yield(void);

// 预留的未来标准接口（暂时不用实现）
long sys_read(unsigned int fd, char *buf, unsigned long count);
long sys_write(unsigned int fd, const char *buf, unsigned long count);
void sys_exit(int status);
int  sys_getpid(void);
int  sys_fork(void);
void sys_exec(void);

#endif
