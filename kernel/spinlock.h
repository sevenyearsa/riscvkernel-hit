// spinlock.h
#ifndef SPINLOCK_H
#define SPINLOCK_H

// 自旋锁结构体，0 代表未上锁，1 代表已上锁
typedef struct {
    volatile int locked; 
} spinlock_t;

void spin_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif