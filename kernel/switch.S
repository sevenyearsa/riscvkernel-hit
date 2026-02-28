# switch.S
.section .text
.global switch_to

# void switch_to(struct context *prev, struct context *next);
# a0 存放 prev 的地址，a1 存放 next 的地址
switch_to:
    # 1. 封存旧任务 (prev)
    sd ra, 0(a0)      # 保存返回地址
    sd sp, 8(a0)      # 保存栈指针 (极其关键！)
    sd s0, 16(a0)     # 保存 Callee-saved 寄存器 s0-s3
    sd s1, 24(a0)     
    sd s2, 32(a0)
    sd s3, 40(a0)

    # --------------------------------------------------
    # 楚河汉界：下方开始加载新任务，世界线变动！
    # --------------------------------------------------

    # 2. 唤醒新任务 (next)
    ld ra, 0(a1)      # 恢复返回地址
    ld sp, 8(a1)      # 恢复栈指针 (此时 CPU 已经在用新任务的栈了！)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)

    # 3. 神鬼莫测的 ret
    # ret 指令会把刚刚恢复的 ra 寄存器的值塞进 PC 指针。
    # 下一纳秒，CPU 瞬间跳入新任务的代码中！
    ret