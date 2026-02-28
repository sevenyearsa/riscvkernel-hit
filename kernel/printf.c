// printf.c
#include <stdarg.h>

// 引入我们在 uart.c 中写好的底层输出函数
extern void uart_putc(char c);
extern void uart_puts(const char *str);

/*
 * 核心算法：将数字转化为字符串并打印
 * num: 要打印的数字 (使用 unsigned long 兼容 64 位地址)
 * base: 进制 (10 代表十进制, 16 代表十六进制)
 * sign: 是否是有符号数 (1 代表是，0 代表无符号)
 */
static void print_num(unsigned long num, int base, int sign) {
    char buf[64]; // 足够装下 64 位二进制/十六进制的字符串
    const char *digits = "0123456789abcdef";
    int i = 0;

    // 处理带符号的十进制负数
    if (sign && (long)num < 0) {
        uart_putc('-');
        num = (unsigned long)(-((long)num));
    }

    // 处理数字 0 的特例
    if (num == 0) {
        uart_putc('0');
        return;
    }

    // 核心进制转换算法：除基取余，逆序排列
    while (num > 0) {
        buf[i++] = digits[num % base];
        num /= base;
    }

    // 因为算出来的余数是反的，所以必须倒序打印
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

/*
 * 微内核版 printf
 * 目前支持: %d (十进制), %x (十六进制), %s (字符串), %c (字符)
 */
void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        // 如果遇到格式化转义符 '%'
        if (*fmt == '%') {
            fmt++; // 跳过 '%'
            switch (*fmt) {
                case 'd': // 打印有符号十进制
                    print_num(va_arg(args, int), 10, 1);
                    break;
                case 'x': // 打印无符号十六进制 (常用于指针和寄存器)
                    print_num(va_arg(args, unsigned long), 16, 0);
                    break;
                case 's': // 打印字符串
                    uart_puts(va_arg(args, const char *));
                    break;
                case 'c': // 打印单个字符
                    uart_putc((char)va_arg(args, int));
                    break;
                case '%': // 打印 '%' 本身
                    uart_putc('%');
                    break;
                default:  // 未知格式，原样输出
                    uart_putc('%');
                    uart_putc(*fmt);
                    break;
            }
        } else {
            // 普通字符，直接输出
            uart_putc(*fmt);
        }
        fmt++;
    }
    va_end(args);
}