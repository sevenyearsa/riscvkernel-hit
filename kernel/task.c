// task.c
extern void printf(const char *fmt, ...);
extern void switch_to(void *prev, void *next);

// 定义任务状态
#define TASK_READY   0   // 准备就绪，随时可以跑
#define TASK_RUNNING 1   // 正在运行
#define TASK_SLEEPING 2  // 正在休眠（等锁）

struct context {
    unsigned long ra;
    unsigned long sp;
    unsigned long s0, s1, s2, s3;
};

#define MAX_TASKS 2

// 将状态加入任务结构体（为了简单，我们用平行数组管理状态）
struct context tasks[MAX_TASKS];
int task_states[MAX_TASKS]; 
unsigned char task_stacks[MAX_TASKS][4096]; 
int current_task = 0;

static void (*task_entry_points[MAX_TASKS])(void);

void task_wrapper(void) {
    asm volatile("csrsi mstatus, 8");
    task_entry_points[current_task]();
}

void task_init(int id, void (*func)(void)) {
    task_entry_points[id] = func;
    tasks[id].ra = (unsigned long)task_wrapper; 
    tasks[id].sp = (unsigned long)&task_stacks[id][4096];
    task_states[id] = TASK_READY; // 初始状态为就绪
    printf("[SYS] Task %d initialized (READY).\n", id);
}

/*
 * 智能调度器：只调度 READY 或 RUNNING 的任务，跳过 SLEEPING 的任务
 */
void task_yield(void) {
    int prev = current_task;
    
    // 寻找下一个不是 SLEEPING 状态的任务
    do {
        current_task = (current_task + 1) % MAX_TASKS;
    } while (task_states[current_task] == TASK_SLEEPING);

    if (prev != current_task) {
        if (task_states[prev] == TASK_RUNNING) {
            task_states[prev] = TASK_READY;
        }
        task_states[current_task] = TASK_RUNNING;
        switch_to(&tasks[prev], &tasks[current_task]);
    }
}

// 暴露给 Mutex 用的休眠和唤醒接口
void task_sleep(void) {
    task_states[current_task] = TASK_SLEEPING;
    task_yield(); // 状态改为休眠后，立刻主动交出 CPU！
}

void task_wakeup(int id) {
    task_states[id] = TASK_READY;
}