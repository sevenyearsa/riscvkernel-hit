/* app.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    /* 程序的灵魂，从虚拟地址 0x100000 开始生长 */
    . = 0x100000;
    
    .text : { *(.text) }       /* 代码段 */
    .rodata : { *(.rodata*) }  /* 只读数据段 (比如字符串) */
    .data : { *(.data) }       /* 全局变量段 */
    .bss : {                   /* 未初始化变量段 */
        *(.bss)
        *(COMMON)
    }
}