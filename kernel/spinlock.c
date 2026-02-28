// spinlock.c
#include "spinlock.h"

void spin_init(spinlock_t *lock) {
    lock->locked = 0;
}

/*
 * 获取锁 (加锁)
 */
void spin_lock(spinlock_t *lock) {
    int old_value;
    
    // 死循环尝试获取锁（这就是“自旋”名字的由来，原地打转疯狂重试）
    while (1) {
        // 内联汇编魔法：amoswap.w.aq (Atomic Swap Word with Acquire semantics)
        // 把 1 写进 lock->locked，并把里面原来的值读到 old_value 中
        asm volatile(
            "amoswap.w.aq %0, %1, (%2)"
            : "=r"(old_value)      // 输出：旧值
            : "r"(1), "r"(&(lock->locked)) // 输入：我们要写入的 1，以及锁的内存地址
            : "memory"             // 内存屏障：警告编译器不要把前后的指令乱序优化
        );

        // 如果读出来的旧值是 0，说明我们成功抢到了锁（把它变成了 1）！
        // 打破死循环，进入临界区
        if (old_value == 0) {
            break;
        }
        
        // 如果旧值是 1，说明别人早就上锁了，我们继续下一轮循环死等...
    }
}

/*
 * 释放锁 (解锁)
 */
void spin_unlock(spinlock_t *lock) {
    // amoswap.w.rl (with Release semantics)
    // 把 0 强行写回 lock->locked，同时丢弃读出来的旧值 (写入 x0/zero 寄存器)
    asm volatile(
        "amoswap.w.rl x0, x0, (%0)"
        : 
        : "r"(&(lock->locked))
        : "memory"
    );
}