// task.c
extern void printf(const char *fmt, ...);
extern void switch_to(void *prev, void *next);

// 引入内存管理相关的函数和宏
#include "mmu.h"
#include "page.h" // 引入计数器
#include "elf.h"
#include "../user/app_bin.h"  // 引入你用 xxd 生成的第三方应用数组
// extern void free_user_page_table(uint64_t *pgd); // 引入回收机
extern void *alloc_page(void);
extern uint64_t *copy_user_page_table(uint64_t *src_pgd);
extern void ret_from_fork(void);


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

#define MAX_TASKS 4
struct context tasks[MAX_TASKS];
int task_states[MAX_TASKS] = {TASK_DEAD,TASK_DEAD,TASK_DEAD,TASK_DEAD}; 
unsigned char task_stacks[MAX_TASKS][4096]; 
int current_task = 0;

static void (*task_entry_points[MAX_TASKS])(void);

/*
 * 真正的降维包装器 (替换掉原来的 task_wrapper)
 */
void task_wrapper(void) {
    unsigned long entry = (unsigned long)task_entry_points[current_task];
    unsigned long mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    
    // 剥夺特权
    mstatus &= ~(3 << 11);
    mstatus |= (1 << 7);

    // 【核心修复】：拿到该任务真正的私有栈地址！
    unsigned long user_sp = tasks[current_task].sp;

    // 强行把 CPU 的栈指针拨到 user_sp，彻底抛弃内核启动栈！
    asm volatile(
        "csrw mstatus, %0\n"      
        "csrw mepc, %1\n"         
        "mv sp, %2\n"       // <--- 就是这致命的一句！拯救了整个宇宙
        "mret"                    
        : 
        : "r"(mstatus), "r"(entry), "r"(user_sp)
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

int task_fork(unsigned long *parent_sp, unsigned long epc) {
    int child_id = -1;
    // 1. 在产房里找一个空床位 (找一个 DEAD 的任务槽)
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_states[i] == TASK_DEAD) {
            child_id = i;
            break;
        }
    }
    if (child_id == -1) return -1; // 产房满了，fork 失败

    // 2. 复制躯体：深拷贝父进程的内核栈内存
    for (int i = 0; i < 4096; i++) {
        task_stacks[child_id][i] = task_stacks[current_task][i];
    }

    // 3. 复制灵魂：拷贝上下文结构体
    tasks[child_id] = tasks[current_task];

// 4. 时空修复
    unsigned long offset = (unsigned long)parent_sp - (unsigned long)task_stacks[current_task];
    tasks[child_id].sp = (unsigned long)task_stacks[child_id] + offset;
    tasks[child_id].ra = (unsigned long)ret_from_fork;

    // 获取子进程的 Trap Frame 指针
    unsigned long *child_sp = (unsigned long *)tasks[child_id].sp;
    
    // 给子进程的 a0 塞入 0 (作为 fork 返回值)
    child_sp[10] = 0; 
    
    // ==========================================
    // 【核心神来之笔】：强行注入苏醒地址！
    // 我们直接把计算好的 epc + 4 塞进栈的第 0 个位置。
    // 彻底摆脱对汇编栈历史遗留数据的依赖！
    // ==========================================
    child_sp[0] = epc + 4;

    // 6. 复制记忆：克隆整个虚拟内存页表
    uint64_t parent_pgd_pa = (tasks[current_task].satp & 0xFFFFFFFFFFF) << 12;
    uint64_t *child_pgd = copy_user_page_table((uint64_t *)parent_pgd_pa);
    
    // 给子进程发放新的 satp 通行证
    tasks[child_id].satp = (8ULL << 60) | ((uint64_t)child_pgd >> 12);

    // 7. 唤醒子进程，加入调度队列
    task_states[child_id] = TASK_READY;

    // 父进程的 a0 会得到子进程的 PID (child_id)
    return child_id; 
}
/*
 * 灵魂注入 (execve)：抹除当前进程的记忆，注入 ELF 程序的灵魂
 */
unsigned long task_exec(unsigned long epc) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)app_elf; // 读取数组头部
    
    // 1. 验明正身
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || 
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        printf("\n[KERNEL] Exec failed: Not a valid ELF file!\n");
        return epc + 4; // 失败了就老老实实回去
    }

    printf("\n[KERNEL] Loading ELF... Entry point at 0x%x\n", ehdr->e_entry);

    // 2. 开辟一个全新的虚拟宇宙 (申请新的 PGD)
    uint64_t *new_pgd = (uint64_t *)alloc_page();
    uint64_t *root_pgd = (uint64_t *)root_page_table;
    for (int i = 0; i < 512; i++) {
        new_pgd[i] = root_pgd[i]; // 保留内核基础设施
    }

    // 3. 解析 Program Headers，搬运灵魂碎片 (代码段、数据段)
    Elf64_Phdr *phdr = (Elf64_Phdr *)(app_elf + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == 1) { // 找到 PT_LOAD 段！
            uint64_t vaddr = phdr[i].p_vaddr;
            uint64_t memsz = phdr[i].p_memsz;
            uint64_t filesz = phdr[i].p_filesz;
            uint64_t offset = phdr[i].p_offset;

            // 按照 4KB 对齐分页加载
            uint64_t start_page = vaddr & ~0xFFFULL;
            uint64_t end_page = (vaddr + memsz + 0xFFF) & ~0xFFFULL;

            for (uint64_t page = start_page; page < end_page; page += 4096) {
                char *phys = (char *)alloc_page();
                for(int b = 0; b < 4096; b++) phys[b] = 0; // 先全部清零 (为了 BSS 段)

                // 将文件中真实的数据拷贝到物理内存
                uint64_t copy_start = (page > vaddr) ? page : vaddr;
                uint64_t copy_end = (page + 4096 < vaddr + filesz) ? page + 4096 : vaddr + filesz;
                if (copy_start < copy_end) {
                    uint64_t src_offset = offset + (copy_start - vaddr);
                    for (uint64_t b = 0; b < (copy_end - copy_start); b++) {
                        phys[(copy_start & 0xFFF) + b] = app_elf[src_offset + b];
                    }
                }
                
                // 赐予该页 U-mode 权限，并挂载到新宇宙的页表上
                map_page(new_pgd, page, (uint64_t)phys, PTE_R | PTE_W | PTE_X | PTE_U | PTE_V | PTE_A | PTE_D);
            }
        }
    }

    // 4. 毁尸灭迹：使用我们的垃圾回收器，把旧的私有页表彻底扬了！
    uint64_t old_pgd_pa = (tasks[current_task].satp & 0xFFFFFFFFFFF) << 12;
    free_user_page_table((uint64_t *)old_pgd_pa);

    // 5. 将当前进程的神经元 (satp) 连接到新宇宙！
    tasks[current_task].satp = (8ULL << 60) | ((uint64_t)new_pgd >> 12);
    asm volatile("csrw satp, %0" : : "r"(tasks[current_task].satp));
    asm volatile("sfence.vma zero, zero"); // 刷新认知

    // 6. 神来之笔：直接返回 ELF 的真实入口点！
    return ehdr->e_entry; 
}