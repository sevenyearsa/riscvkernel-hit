// main.c

// 声明外部驱动函数
extern void uart_init(void);
extern void uart_puts(const char *str);

void os_main(void) {
    // 1. 初始化串口硬件
    uart_init();
    
    // 2. 发出第一声啼哭！
    uart_puts("\n");
    uart_puts("======================================\n");
    uart_puts("       Hello, RISC-V Bare-metal OS!   \n");
    uart_puts("======================================\n");
    uart_puts("\n");

    // 3. 安全着陆，进入休眠死循环
    while (1) {
        // 未来，这里将会变成操作系统的心跳（Idle 进程）
    }
}