// task.c
extern void printf(const char *fmt, ...);
extern void switch_to(void *prev, void *next);

// 引入内存管理相关的函数和宏
#include "mmu.h"
#include "page.h" // 引入计数器
extern void free_user_page_table(uint64_t *pgd); // 引入回收机
extern void *alloc_page(void);

#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEPING 2
#define TASK_DEAD    3

struct context {
    unsigned long ra;
    unsigned long sp;
    unsigned long s0, s1, s2, s3;
    unsigned long satp; // 【新增】：每个进程专属的 MMU 根页表通行证！
};

#define MAX_TASKS 2
struct context tasks[MAX_TASKS];
int task_states[MAX_TASKS]; 
unsigned char task_stacks[MAX_TASKS][4096]; 
int current_task = 0;

static void (*task_entry_points[MAX_TASKS])(void);

/*
 * 真正的降维包装器 (替换掉原来的 task_wrapper)
 */
void task_wrapper(void) {
    // 1. 获取当前任务真正的入口函数地址
    unsigned long entry = (unsigned long)task_entry_points[current_task];
    unsigned long mstatus;

    // 2. 读取当前的总闸状态
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));

    // 3. 【核心魔法：修改 MPP 位，彻底剥夺特权！】
    // MPP 是 mstatus 的第 11 和 12 位。我们把这两位清零 (00 代表 U-mode)
    mstatus &= ~(3 << 11);

    // 4. 开启未来 U-mode 的时钟中断 (MPIE = 1)
    mstatus |= (1 << 7);

    // 5. 伪造现场并执行 mret！硬件会被骗过，瞬间降级到 U-mode 并跳入 entry！
    asm volatile(
        "csrw mstatus, %0\n"      
        "csrw mepc, %1\n"         
        "mret"                    
        : 
        : "r"(mstatus), "r"(entry)
    );
}
/*
 * 进程初始化：分配独立页表与私有内存
 */
void task_init(int id, void (*func)(void)) {
    task_entry_points[id] = func;
    tasks[id].ra = (unsigned long)task_wrapper; 
    tasks[id].sp = (unsigned long)&task_stacks[id][4096];
    task_states[id] = TASK_READY;

    // ==========================================================
    // 【核心黑魔法：创建独立的地址空间 (Process Address Space)】
    // ==========================================================
    
    // 1. 申请一页物理内存，作为这个进程的专属顶级页表 (PGD)
    uint64_t *my_pgd = (uint64_t *)alloc_page();
    
    // 2. 抄写内核宇宙（共享内核空间）：
    // 把内核 root_page_table 里的 512 个顶级页表项全部复制过来。
    // 这样，进程依然能找到 UART 和 Trap 处理函数。
    uint64_t *root_pgd = (uint64_t *)root_page_table;
    for (int i = 0; i < 512; i++) {
        my_pgd[i] = root_pgd[i];
    }
    
    // 3. 开辟私有领地（进程隔离证明）：
    // 我们申请一页全新的物理内存
    uint64_t private_phys_page = (uint64_t)alloc_page();
    
    // 将这个全新的物理页，强行映射到所有进程都一样的虚拟地址：0x40000000
    // 赋予 U-mode 可读可写权限 (PTE_R | PTE_W | PTE_U | PTE_V)
    map_page(my_pgd, 0x40000000, private_phys_page, PTE_R | PTE_W | PTE_U | PTE_V | PTE_A | PTE_D);
    
    uint64_t readonly_phys_page = (uint64_t)alloc_page();
    map_page(my_pgd, 0x50000000, readonly_phys_page, PTE_R | PTE_W | PTE_U | PTE_V | PTE_A | PTE_D);
    // 4. 生成专属的 satp 通行证 (Mode = Sv39)
    tasks[id].satp = (8ULL << 60) | ((uint64_t)my_pgd >> 12);

    printf("[SYS] Process %d created! Private PGD at phys: 0x%x\n", id, my_pgd);
}

void task_exit(int status) {
    printf("\n[KERNEL] Process %d exited with status %d.\n", current_task, status);
    
    // 1. 宣判死亡
    task_states[current_task] = TASK_DEAD;
    
    // 2. 打印死亡前的内存余量
    int pages_before = get_free_page_count();
    printf("[KERNEL] Before reclamation, free pages: %d\n", pages_before);
    
    // 3. 【核心回收】：拿着死者的 satp 通行证，还原出它的 PGD 物理地址，送进焚化炉！
    // satp 寄存器的前 44 位是物理页号 (PPN)，左移 12 位就是 PGD 的绝对物理地址
    uint64_t pgd_pa = (tasks[current_task].satp & 0xFFFFFFFFFFF) << 12;
    free_user_page_table((uint64_t *)pgd_pa);
    
    // 4. 打印回收后的内存余量
    int pages_after = get_free_page_count();
    printf("[KERNEL] After reclamation, free pages: %d (Recovered %d pages!)\n", 
           pages_after, pages_after - pages_before);
    
    // 5. 换人，死者安息
    task_yield(); 
}
/*
 * 【完全重写】智能调度器：现在要学会跳过死尸，并判断是不是全军覆没
 */
void task_yield(void) {
    int prev = current_task;
    int next = current_task;
    int found = 0;
    
    // 轮询寻找下一个活着 (READY 或 RUNNING) 的任务
    for (int i = 0; i < MAX_TASKS; i++) {
        next = (next + 1) % MAX_TASKS;
        if (task_states[next] == TASK_READY || task_states[next] == TASK_RUNNING) {
            found = 1;
            break;
        }
    }
    
    // 如果找了一圈发现没活人了
    if (!found) {
        printf("\n[SYS] All tasks have exited. System halting.\n");
        asm volatile("wfi" ::: "memory");
    }
    
    current_task = next;

    if (prev != current_task) {
        // 只有当之前的任务还在 RUNNING 时，才降级为 READY。
        // 如果它是 SLEEPING 或者 DEAD，绝对不能修改它的状态！
        if (task_states[prev] == TASK_RUNNING) {
            task_states[prev] = TASK_READY;
        }
        task_states[current_task] = TASK_RUNNING;
        
        asm volatile("csrw satp, %0" : : "r"(tasks[current_task].satp));
        asm volatile("sfence.vma zero, zero");
        
        switch_to(&tasks[prev], &tasks[current_task]);
    }
}

// ... 睡眠和唤醒函数保持不变 ...
void task_sleep(void) {
    task_states[current_task] = TASK_SLEEPING;
    task_yield(); 
}
void task_wakeup(int id) {
    task_states[id] = TASK_READY;
}

unsigned long get_task_satp(int id) {
    return tasks[id].satp;
}