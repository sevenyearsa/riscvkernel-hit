// uart.c

// 1. 手工打磨基本数据类型
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// 2. 硬件手册查阅结果：UART 控制器的物理内存基址
#define UART0_BASE 0x10000000L

// 3. 定义关键寄存器的物理地址
// THR (Transmit Holding Register): 发送数据寄存器，偏移量为 0
#define UART0_THR  ((volatile uint8_t *)(UART0_BASE + 0x00))

// LSR (Line Status Register): 线路状态寄存器，偏移量为 5
#define UART0_LSR  ((volatile uint8_t *)(UART0_BASE + 0x05))

// LSR 的第 5 位 (0x20) 如果为 1，表示发送缓冲区为空，可以塞入下一个字符了
#define UART0_LSR_EMPTY_MASK 0x20

/*
 * 串口初始化
 */
void uart_init(void) {
    // 在真实的物理主板上，这里需要写寄存器来配置波特率（如 115200）、
    // 数据位（8位）、停止位（1位）等。
    // 但 QEMU 模拟器在启动时已经把虚拟硬件配置好了，所以我们可以直接白嫖，留空即可。
}

/*
 * 发送单个字符
 */
void uart_putc(char c) {
    // 轮询（Polling）模式：死循环读取 LSR 寄存器
    // 只要第 5 位不是 1，就说明硬件还没把上一个字符发完，我们就一直等
    while ((*UART0_LSR & UART0_LSR_EMPTY_MASK) == 0) {
        // 原地自旋等待
    }
    
    // 一旦缓冲区空了，直接把字符砸进 THR 寄存器，硬件会自动把它转成电信号发出去！
    *UART0_THR = c;
}

/*
 * 发送字符串
 */
void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}