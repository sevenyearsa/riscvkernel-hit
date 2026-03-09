// mutex.c
#include "spinlock.h"
#include "sched.h"

// 互斥锁结构体
typedef struct {
    int locked;
    spinlock_t guard; // 保护互斥锁内部状态的微型自旋锁
    int owner_id;     // 记录当前是哪个任务拿到了锁
} mutex_t;

extern int current_task;
extern void task_sleep(void);
extern void task_wakeup(int id);

void mutex_init(mutex_t *m) {
    m->locked = 0;
    spin_init(&(m->guard));
    m->owner_id = -1;
}

void mutex_lock(mutex_t *m) {
    while (1) {
        // 先用自旋锁锁住 Mutex 的操作台
        spin_lock(&(m->guard));
        
        if (m->locked == 0) {
            // 没人用！我拿到了！
            m->locked = 1;
            m->owner_id = current_task;
            spin_unlock(&(m->guard));
            return; // 成功加锁，返回执行业务代码
        }
        
        // 如果被别人拿了，解锁操作台，然后立刻把自己挂起（休眠）
        spin_unlock(&(m->guard));
        
        // 交出 CPU，直到有人解锁时唤醒我，我再回来重新 `while(1)` 抢锁
        task_sleep(); 
    }
}

void mutex_unlock(mutex_t *m) {
    spin_lock(&(m->guard));
    
    m->locked = 0;
    m->owner_id = -1;
    
    // 粗暴简单的唤醒逻辑：把所有休眠的任务都叫醒（真实 OS 里会有一个精准的等待队列）
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_states[i] == TASK_SLEEPING) {
            task_wakeup(i);
        }
    }
    
    spin_unlock(&(m->guard));
}
