/* os.ld */

/* 1. 指定目标架构为 RISC-V */
OUTPUT_ARCH(riscv)

/* 2. 指定内核的绝对入口点（点火函数），这个标号我们会在后面的汇编里定义 */
ENTRY(_start)

/* 3. 定义一个变量表示内存基址，QEMU virt 机器的 RAM 起点 */
BASE_ADDRESS = 0x80000000;

SECTIONS
{
    /* 告诉链接器：从 BASE_ADDRESS 开始往下排布数据 */
    . = BASE_ADDRESS;

    /* 记录内核起始的物理地址，方便后续内存管理 */
    PROVIDE(text_start = .);

    /* --- 1. 代码段 (.text) --- */
    .text : {
        /* 强制把 boot.S 编译出的 .text.init 放在最前面！确保 _start 是内存中的第一条指令 */
        *(.text.init)
        /* 接着放其他所有的 C/汇编 代码 */
        *(.text .text.*)
    }
    /* 按照 4KB (一页) 对齐，为以后开启 MMU 页表做准备 */
    . = ALIGN(4096);
    PROVIDE(text_end = .);

    /* --- 2. 只读数据段 (.rodata) --- */
    PROVIDE(rodata_start = .);
    .rodata : {
        *(.rodata .rodata.*)
    }
    . = ALIGN(4096);
    PROVIDE(rodata_end = .);

    /* --- 3. 已初始化的数据段 (.data) --- */
    /* 比如：int a = 1; */
    PROVIDE(data_start = .);
    .data : {
        *(.sdata .sdata.*) /* 小数据段 */
        *(.data .data.*)
    }
    . = ALIGN(4096);
    PROVIDE(data_end = .);

    /* --- 4. 未初始化的数据段 (.bss) --- */
    /* 比如：int b; 或者 int array[100]; */
    PROVIDE(bss_start = .);
    .bss : {
        *(.sbss .sbss.*)
        *(.bss .bss.*)
    }
    . = ALIGN(4096);
    PROVIDE(bss_end = .);

    /* 记录整个内核镜像的结束物理地址 */
    PROVIDE(kernel_end = .);
}