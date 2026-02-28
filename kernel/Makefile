# Makefile

# 1. 定义交叉编译工具链
CC = riscv64-unknown-elf-gcc

# 2. 裸机编译核心参数 (极其重要)
# -nostdlib: 严禁链接 C 语言标准库 (因为系统里根本没有标准库)
# -fno-builtin: 禁用编译器的内置函数优化
# -mcmodel=medany: 允许程序在物理内存的任意位置运行 (对 0x80000000 起始的地址必须加这个)
# -g: 生成调试信息，方便以后用 GDB 单步调试
# -Wall: 开启所有警告
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -g -Wall

# 3. 目标文件
OBJS = boot.o uart.o printf.o main.o

# 4. 默认目标：链接成最终的 ELF 镜像
os.elf: $(OBJS)
	$(CC) $(CFLAGS) -T os.ld -o os.elf $(OBJS)

# 5. 编译 C 源码
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 6. 编译汇编源码
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 7. 终极指令：挂载到 QEMU 启动
# -machine virt: 指定使用 virt 虚拟主板
# -bios none: 禁用默认的 OpenSBI 固件，让 QEMU 直接从我们的代码起点跑！
# -kernel os.elf: 加载我们刚编译出的内核镜像
# -nographic: 在当前终端输出，不弹窗
run: os.elf
	qemu-system-riscv64 -machine virt -bios none -kernel os.elf -nographic

# 8. 清理工程
clean:
	rm -f *.o os.elf