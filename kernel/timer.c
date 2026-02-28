// timer.c
extern void printf(const char *fmt, ...);

// 1. CLINT 硬件寄存器的物理内存地址 (QEMU virt)
#define CLINT_BASE 0x2000000L
// mtimecmp 寄存器地址 (对应 Hart 0)
#define CLINT_MTIMECMP (CLINT_BASE + 0x4000)
// mtime 寄存器地址 (全局时间)
#define CLINT_MTIME    (CLINT_BASE + 0xBFF8)

// QEMU 的时钟频率大约是 10MHz，即每秒 1000 万次 tick
#define TIMER_FREQ 10000000

// 2. 读写 64 位寄存器的宏
#define Read_mtime()     (*(volatile unsigned long *)(CLINT_MTIME))
#define Write_mtimecmp(val) (*(volatile unsigned long *)(CLINT_MTIMECMP) = (val))

/*
 * 调度下一次定时器中断
 */
void timer_load_next(void) {
    // 读取当前时间，加上一秒钟的 tick 数，写入比较寄存器
    unsigned long current_time = Read_mtime();
    Write_mtimecmp(current_time + TIMER_FREQ);
}

/*
 * 初始化定时器并开启中断
 */
void timer_init(void) {
    // 1. 设置第一次中断触发的时间
    timer_load_next();

    // 2. 开启 CPU 的机器定时器中断允许位 (MTIE, 第 7 位)
    // 使用内联汇编设置 mie (Machine Interrupt Enable) 寄存器
    unsigned long mie_mtie = (1 << 7);
    asm volatile("csrs mie, %0" : : "r"(mie_mtie));

    // 3. 开启全局机器中断允许位 (MIE, 第 3 位)
    // 使用内联汇编设置 mstatus (Machine Status) 寄存器
    unsigned long mstatus_mie = (1 << 3);
    asm volatile("csrs mstatus, %0" : : "r"(mstatus_mie));

    printf("[SYS] CLINT Timer initialized. Heartbeat started.\n");
}