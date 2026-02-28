// main.c

extern void uart_init(void);
extern void printf(const char *fmt, ...);

void os_main(void) {
    uart_init();
    
    printf("\n");
    printf("======================================\n");
    printf("       Welcome to uniproton OS!       \n");
    printf("======================================\n");
    
    // 测试 1：普通字符串和字符
    printf("[INFO] Printf module %s! Grade: %c\n", "initialized", 'A');
    
    // 测试 2：十进制与负数运算
    int a = 100;
    int b = 256;
    printf("[INFO] Math test: %d - %d = %d\n", a, b, a - b);
    
    // 测试 3：极其重要的 64 位十六进制指针打印！
    unsigned long fake_pgd_addr = 0x8000AABBCCDD1122; 
    printf("[INFO] Mock PGD base address: 0x%x\n", fake_pgd_addr);
    
    printf("\n[SYS] Entering idle loop...\n");

    while (1) {
    }
}