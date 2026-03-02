// app.c
// 这是一个完全独立的第三方应用程序，它不知道内核长什么样，只知道系统调用号

#define SYS_WRITE 64  // Linux 标准：写入文件 (我们用来打印字符)
#define SYS_EXIT  93  // Linux 标准：退出进程

// 自己手搓一个发出 SYS_WRITE ecall 的打印函数
void sys_print(const char *s) {
    while (*s) {
        asm volatile(
            "mv a7, %0\n"
            "mv a0, %1\n"
            "ecall\n"
            : 
            : "r"(SYS_WRITE), "r"(*s) 
            : "a0", "a7"
        );
        s++;
    }
}

// 自己手搓一个发出 SYS_EXIT ecall 的退出函数
void sys_exit(int status) {
    asm volatile(
        "mv a7, %0\n"
        "mv a0, %1\n"
        "ecall\n"
        : 
        : "r"(SYS_EXIT), "r"(status) 
        : "a0", "a7"
    );
    while(1);
}

// 【关键】：真正的可执行文件需要一个入口点
// 我们不叫 main，我们叫 _start，这是链接器默认寻找的入口
void _start(void) {
    sys_print("\n============================================\n");
    sys_print("  [APP] SOUL INJECTION SUCCESSFUL!\n");
    sys_print("  [APP] Hello from a totally independent ELF!\n");
    sys_print("============================================\n\n");
    
    // 干完活，自我了断
    sys_exit(0);
}