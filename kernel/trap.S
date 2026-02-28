# trap.S
.section .text
.global trap_vector
.align 4

trap_vector:
    addi sp, sp, -256

    sd ra, 0(sp)
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd a2, 24(sp)

    # 【新增】：提取 mepc 并压入栈中保存！
    csrr t0, mepc
    sd t0, 32(sp)

    # trap.S (中间部分)
    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call trap_handler

    # 【修复点】：此时 a0 寄存器里装着 C 语言返回的新 mepc (可能是原址，也可能是 +4)
    # 我们把这个新地址，强行写回到栈里存放 mepc 的位置！
    sd a0, 32(sp)

    # 接下来的代码完全不变，它会乖乖地把栈里的新地址装载进 mepc
    ld t0, 32(sp)
    csrw mepc, t0

    ld ra, 0(sp)
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld a2, 24(sp)
    addi sp, sp, 256

    mret