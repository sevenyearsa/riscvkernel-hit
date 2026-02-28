# trap.S
.section .text
.global trap_vector
.align 4

trap_vector:
    # 1. 申请 256 字节的栈空间 (足够保存 32 个 8 字节的寄存器)
    addi sp, sp, -256

    # 2. 封存现场：把通用寄存器全部压入栈中保存
    sd ra, 1*8(sp)
    sd gp, 3*8(sp)
    sd tp, 4*8(sp)
    sd t0, 5*8(sp)
    sd t1, 6*8(sp)
    sd t2, 7*8(sp)
    sd s0, 8*8(sp)
    sd s1, 9*8(sp)
    sd a0, 10*8(sp)  # <--- 用户的第一个参数 (arg0) 存在这里！
    sd a1, 11*8(sp)
    sd a2, 12*8(sp)
    sd a3, 13*8(sp)
    sd a4, 14*8(sp)
    sd a5, 15*8(sp)
    sd a6, 16*8(sp)
    sd a7, 17*8(sp)  # <--- 用户的业务号 (syscall_num) 存在这里！
    sd s2, 18*8(sp)
    sd s3, 19*8(sp)
    sd s4, 20*8(sp)
    sd s5, 21*8(sp)
    sd s6, 22*8(sp)
    sd s7, 23*8(sp)
    sd s8, 24*8(sp)
    sd s9, 25*8(sp)
    sd s10, 26*8(sp)
    sd s11, 27*8(sp)
    sd t3, 28*8(sp)
    sd t4, 29*8(sp)
    sd t5, 30*8(sp)
    sd t6, 31*8(sp)

    # 保存发生异常时的 PC 地址 (mepc) 到栈的第 0 个位置
    csrr t0, mepc
    sd t0, 0(sp)

    # 3. 准备调用 trap_handler(cause, epc, tval, arg0, arg1, sys_num)
    # 根据 C 语言调用约定，前 6 个参数分别放在 a0~a5 寄存器中
    csrr a0, mcause      # 第 1 个参数：cause
    csrr a1, mepc        # 第 2 个参数：epc
    csrr a2, mtval       # 第 3 个参数：tval
    ld   a3, 10*8(sp)    # 第 4 个参数：从栈里挖出用户刚才存的 a0
    ld   a4, 11*8(sp)    # 第 5 个参数：从栈里挖出用户刚才存的 a1
    ld   a5, 17*8(sp)    # 第 6 个参数：从栈里挖出用户刚才存的 a7 (业务号)

    # 4. 呼叫 C 语言的内核总管！
    call trap_handler

    # 5. trap_handler 结束工作后，会通过 a0 返回新的 PC 地址
    # 我们立刻把这个新地址写入 mepc
    csrw mepc, a0

    # 6. 恢复现场：把寄存器原封不动地从栈里挖出来还给 CPU
    ld ra, 1*8(sp)
    ld gp, 3*8(sp)
    ld tp, 4*8(sp)
    ld t0, 5*8(sp)
    ld t1, 6*8(sp)
    ld t2, 7*8(sp)
    ld s0, 8*8(sp)
    ld s1, 9*8(sp)
    ld a0, 10*8(sp)
    ld a1, 11*8(sp)
    ld a2, 12*8(sp)
    ld a3, 13*8(sp)
    ld a4, 14*8(sp)
    ld a5, 15*8(sp)
    ld a6, 16*8(sp)
    ld a7, 17*8(sp)
    ld s2, 18*8(sp)
    ld s3, 19*8(sp)
    ld s4, 20*8(sp)
    ld s5, 21*8(sp)
    ld s6, 22*8(sp)
    ld s7, 23*8(sp)
    ld s8, 24*8(sp)
    ld s9, 25*8(sp)
    ld s10, 26*8(sp)
    ld s11, 27*8(sp)
    ld t3, 28*8(sp)
    ld t4, 29*8(sp)
    ld t5, 30*8(sp)
    ld t6, 31*8(sp)

    addi sp, sp, 256
    
    # 7. 神圣跳跃：降级回到 User 态的下一条指令
    mret