// trap_handler.c
extern void printf(const char *fmt, ...);
extern void timer_load_next(void);
extern void task_yield(void);
extern void uart_putc(char c);
extern int uart_getc(void);
#include "syscall.h"
#include "trapframe.h"
#include "exec.h"

extern int task_fork(struct trapframe *parent_tf, unsigned long epc);
extern void task_exit(int status);

typedef unsigned long (*syscall_fn_t)(unsigned long arg0, unsigned long arg1, 
                                      unsigned long epc, struct trapframe *tf);

unsigned long do_sys_write(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    uart_putc((char)arg0); 
    return epc + 4;
}

unsigned long do_sys_read(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    unsigned long count = tf->a2;
    char *buf = (char *)arg1;

    if ((unsigned int)arg0 != 0 || buf == 0 || count == 0) {
        tf->a0 = -1UL;
        return epc + 4;
    }

    for (unsigned long i = 0; i < count; i++) {
        int ch = uart_getc();

        if (ch == '\r') {
            ch = '\n';
        }
        buf[i] = (char)ch;
        tf->a0 = i + 1;
        return epc + 4;
    }

    tf->a0 = 0;
    return epc + 4;
}

unsigned long do_sys_yield(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    task_yield();
    return epc + 4;
}

unsigned long do_sys_exit(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    task_exit((int)arg0);
    return epc + 4; // 虽然永远不会返回，但需遵守接口规范
}

unsigned long do_sys_clone(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    tf->a0 = task_fork(tf, epc); // 写入子进程 PID 到父进程的 a0
    return epc + 4;
}

unsigned long do_sys_execve(unsigned long arg0, unsigned long arg1, unsigned long epc, struct trapframe *tf) {
    // 极致优雅：exec 成功直接返回新程序的入口点
    return task_exec(epc, tf); 
}

#define MAX_SYSCALL_NUM 300 

syscall_fn_t sys_call_table[MAX_SYSCALL_NUM] = {
    [SYS_READ]        = do_sys_read,
    [SYS_WRITE]       = do_sys_write,
    [SYS_SCHED_YIELD] = do_sys_yield,
    [SYS_EXIT]        = do_sys_exit,
    [SYS_CLONE]       = do_sys_clone,
    [SYS_EXECVE]      = do_sys_execve,
};

unsigned long trap_handler(unsigned long cause, unsigned long epc, unsigned long tval, 
                           unsigned long arg0, unsigned long arg1, unsigned long sys_num,
                        struct trapframe *tf) {
                           
    int is_interrupt = (cause & 0x8000000000000000L) != 0;
    unsigned long exception_code = cause & 0x7FFFFFFFFFFFFFFFL;

    if (is_interrupt) {
        if (exception_code == 7) { 
            timer_load_next();
            task_yield(); 
            return epc; 
        }
    } 
    else {
        if (exception_code == 8 || exception_code == 11) {
            if (sys_num < MAX_SYSCALL_NUM && sys_call_table[sys_num] != 0) {
                return sys_call_table[sys_num](arg0, arg1, epc, tf);
            } else {
                printf("\n[KERNEL] Unimplemented syscall number: %d\n", sys_num);
                return epc + 4;
            }
        }
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
