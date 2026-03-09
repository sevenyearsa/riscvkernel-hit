// task.c
extern void printf(const char *fmt, ...);
extern void switch_to(void *prev, void *next);
extern void trap_return(void);

#include "mmu.h"
#include "page.h"
#include "context.h"
#include "exec.h"
#include "sched.h"
#include "trapframe.h"
#include "user_layout.h"

extern void *alloc_page(void);
extern uint64_t *copy_user_page_table(uint64_t *src_pgd);

extern char text_start[];
extern char text_end[];
extern char rodata_start[];
extern char rodata_end[];

void task_yield(void);

struct context tasks[MAX_TASKS];
int task_states[MAX_TASKS] = {TASK_DEAD, TASK_DEAD, TASK_DEAD, TASK_DEAD};
int current_task = 0;

static uint64_t task_user_stacks[MAX_TASKS];
static unsigned char task_kernel_stacks[MAX_TASKS][PAGE_SIZE];

static uint64_t pte_to_pa_local(uint64_t pte) {
    return (pte >> 10) << 12;
}

static struct trapframe *task_trap_frame(int id) {
    return (struct trapframe *)&task_kernel_stacks[id][PAGE_SIZE - sizeof(struct trapframe)];
}

static uint64_t *task_page_table(int id) {
    return mmu_satp_to_pgd(tasks[id].satp);
}

static void map_range(uint64_t *pgd, uint64_t start, uint64_t end, uint64_t flags) {
    uint64_t begin = start & ~(PAGE_SIZE - 1);
    uint64_t limit = (end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (uint64_t va = begin; va < limit; va += PAGE_SIZE) {
        map_page(pgd, va, va, flags);
    }
}

static void map_boot_user_text(uint64_t *pgd) {
    map_range(pgd, (uint64_t)text_start, (uint64_t)text_end, PTE_R | PTE_X | PTE_U | PTE_V);
    map_range(pgd, (uint64_t)rodata_start, (uint64_t)rodata_end, PTE_R | PTE_U | PTE_V);
}

static void map_user_stack(uint64_t *pgd, uint64_t stack_pa) {
    // Leave the page below the stack unmapped as a guard page.
    map_page(pgd, USER_STACK_PAGE_VA, stack_pa, PTE_R | PTE_W | PTE_U | PTE_V);
}

static void map_boot_user_data(uint64_t *pgd) {
    map_page(pgd, USER_RW_PAGE_VA, (uint64_t)alloc_page(), PTE_R | PTE_W | PTE_U | PTE_V);
    map_page(pgd, USER_RO_PAGE_VA, (uint64_t)alloc_page(), PTE_R | PTE_U | PTE_V);
}

static void init_user_address_space(uint64_t *pgd, uint64_t stack_pa) {
    map_boot_user_text(pgd);
    map_user_stack(pgd, stack_pa);
    map_boot_user_data(pgd);
}

static void bind_task_frame(int id, struct trapframe *tf) {
    tasks[id].ra = (unsigned long)trap_return;
    tasks[id].sp = (unsigned long)tf;
}

static void bind_task_page_table(int id, uint64_t *pgd) {
    tasks[id].satp = mmu_make_satp(pgd);
}

static void init_user_frame(int id, unsigned long entry) {
    struct trapframe *tf = task_trap_frame(id);

    trapframe_clear(tf);
    tf->epc = entry;
    tf->user_sp = USER_STACK_TOP;

    bind_task_frame(id, tf);
}

static int alloc_task_slot(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_states[i] == TASK_DEAD) {
            return i;
        }
    }
    return -1;
}

static void switch_task(int next) {
    int prev = current_task;

    sched_prepare_switch(next);
    switch_to(&tasks[prev], &tasks[current_task]);
}

void task_wrapper(void) {
    task_states[current_task] = TASK_RUNNING;
    asm volatile(
        "mv sp, %0\n"
        "j trap_return\n"
        :
        : "r"(tasks[current_task].sp)
    );
}

void task_init(int id, void (*func)(void)) {
    uint64_t *pgd = (uint64_t *)alloc_page();

    task_user_stacks[id] = (uint64_t)alloc_page();
    init_user_frame(id, (unsigned long)func);
    init_user_address_space(pgd, task_user_stacks[id]);
    bind_task_page_table(id, pgd);
    task_states[id] = TASK_READY;

    printf("[SYS] Process %d created! Private PGD at phys: 0x%x\n", id, pgd);
}

void task_exit(int status) {
    int pages_before = get_free_page_count();

    printf("\n[KERNEL] Process %d exited with status %d.\n", current_task, status);
    printf("[KERNEL] Before reclamation, free pages: %d\n", pages_before);

    free_user_page_table(task_page_table(current_task));
    task_user_stacks[current_task] = 0;
    task_states[current_task] = TASK_DEAD;

    printf("[KERNEL] After reclamation, free pages: %d (Recovered %d pages!)\n",
           get_free_page_count(), get_free_page_count() - pages_before);

    task_yield();
    while (1) {
    }
}

void task_yield(void) {
    int next = sched_pick_next(current_task);

    if (next < 0) {
        printf("\n[SYS] All tasks have exited. System halting.\n");
        while (1) {
            asm volatile("wfi" ::: "memory");
        }
    }

    if (next != current_task) {
        switch_task(next);
    }
}

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

int task_fork(struct trapframe *parent_tf, unsigned long epc) {
    int child_id = alloc_task_slot();
    if (child_id == -1) {
        return -1;
    }

    uint64_t *child_pgd = copy_user_page_table(task_page_table(current_task));
    uint64_t *stack_pte = walk_pte(child_pgd, USER_STACK_PAGE_VA, 0);
    struct trapframe *child_tf = task_trap_frame(child_id);

    if (!stack_pte || !(*stack_pte & PTE_V)) {
        return -1;
    }

    trapframe_copy(child_tf, parent_tf);

    context_copy(&tasks[child_id], &tasks[current_task]);
    bind_task_frame(child_id, child_tf);
    bind_task_page_table(child_id, child_pgd);

    child_tf->epc = epc + 4;
    child_tf->a0 = 0;

    task_user_stacks[child_id] = pte_to_pa_local(*stack_pte);
    task_states[child_id] = TASK_READY;
    return child_id;
}
