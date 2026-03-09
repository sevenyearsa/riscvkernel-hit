#include "exec.h"

#include "context.h"
#include "elf.h"
#include "mmu.h"
#include "page.h"
#include "sched.h"
#include "user_layout.h"
#include "../user/app_bin.h"

extern void printf(const char *fmt, ...);
extern void *alloc_page(void);
extern void free_user_page_table(uint64_t *pgd);
extern struct context tasks[MAX_TASKS];

#define ELF_PF_X 0x1
#define ELF_PF_W 0x2
#define ELF_PF_R 0x4

static void map_user_stack(uint64_t *pgd, uint64_t stack_pa) {
    map_page(pgd, USER_STACK_PAGE_VA, stack_pa, PTE_R | PTE_W | PTE_U | PTE_V);
}

static uint64_t load_segment_flags(const Elf64_Phdr *phdr) {
    uint64_t flags = PTE_U | PTE_V;

    if (phdr->p_flags & ELF_PF_R) flags |= PTE_R;
    if (phdr->p_flags & ELF_PF_W) flags |= PTE_W;
    if (phdr->p_flags & ELF_PF_X) flags |= PTE_X;
    return flags;
}

static int ranges_overlap(uint64_t start_a, uint64_t end_a, uint64_t start_b, uint64_t end_b) {
    return start_a < end_b && start_b < end_a;
}

static int validate_exec_image(Elf64_Ehdr *ehdr) {
    if (app_elf_len < sizeof(Elf64_Ehdr)) {
        printf("\n[KERNEL] Exec failed: ELF image too small.\n");
        return 0;
    }

    if (ehdr->e_phentsize != sizeof(Elf64_Phdr)) {
        printf("\n[KERNEL] Exec failed: Unexpected program header size.\n");
        return 0;
    }

    uint64_t ph_end = ehdr->e_phoff + (uint64_t)ehdr->e_phnum * sizeof(Elf64_Phdr);
    if (ph_end > app_elf_len || ph_end < ehdr->e_phoff) {
        printf("\n[KERNEL] Exec failed: Program header table out of range.\n");
        return 0;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)(app_elf + ehdr->e_phoff);
    int entry_ok = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != 1) {
            continue;
        }

        if (phdr[i].p_filesz > phdr[i].p_memsz) {
            printf("\n[KERNEL] Exec failed: Segment file size exceeds memory size.\n");
            return 0;
        }

        uint64_t seg_file_end = phdr[i].p_offset + phdr[i].p_filesz;
        if (seg_file_end > app_elf_len || seg_file_end < phdr[i].p_offset) {
            printf("\n[KERNEL] Exec failed: Segment file range out of image.\n");
            return 0;
        }

        if (phdr[i].p_align && ((phdr[i].p_vaddr - phdr[i].p_offset) & (phdr[i].p_align - 1))) {
            printf("\n[KERNEL] Exec failed: Segment alignment mismatch.\n");
            return 0;
        }

        uint64_t seg_start = phdr[i].p_vaddr;
        uint64_t seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
        if (seg_end < seg_start) {
            printf("\n[KERNEL] Exec failed: Segment address overflow.\n");
            return 0;
        }

        if (ranges_overlap(seg_start, seg_end, USER_STACK_GUARD_VA, USER_STACK_TOP) ||
            ranges_overlap(seg_start, seg_end, USER_RW_PAGE_VA, USER_RW_PAGE_VA + PAGE_SIZE) ||
            ranges_overlap(seg_start, seg_end, USER_RO_PAGE_VA, USER_RO_PAGE_VA + PAGE_SIZE)) {
            printf("\n[KERNEL] Exec failed: Segment overlaps reserved user space.\n");
            return 0;
        }

        for (int j = i + 1; j < ehdr->e_phnum; j++) {
            if (phdr[j].p_type != 1) {
                continue;
            }

            uint64_t this_start = phdr[i].p_vaddr & ~(PAGE_SIZE - 1);
            uint64_t this_end = (phdr[i].p_vaddr + phdr[i].p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            uint64_t other_start = phdr[j].p_vaddr & ~(PAGE_SIZE - 1);
            uint64_t other_end = (phdr[j].p_vaddr + phdr[j].p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            if (ranges_overlap(this_start, this_end, other_start, other_end)) {
                printf("\n[KERNEL] Exec failed: Loadable segments overlap.\n");
                return 0;
            }
        }

        if (ehdr->e_entry >= seg_start && ehdr->e_entry < seg_end) {
            entry_ok = 1;
        }
    }

    if (!entry_ok) {
        printf("\n[KERNEL] Exec failed: Entry point is not inside a loadable segment.\n");
        return 0;
    }

    return 1;
}

unsigned long task_exec(unsigned long epc, struct trapframe *tf) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)app_elf;
    uint64_t old_pgd_pa = (tasks[current_task].satp & 0xFFFFFFFFFFFULL) << 12;
    uint64_t *new_pgd;
    uint64_t new_stack_pa;

    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        printf("\n[KERNEL] Exec failed: Not a valid ELF file!\n");
        return epc + 4;
    }

    if (!validate_exec_image(ehdr)) {
        return epc + 4;
    }

    printf("\n[KERNEL] Loading ELF... Entry point at 0x%x\n", ehdr->e_entry);

    new_pgd = (uint64_t *)alloc_page();
    new_stack_pa = (uint64_t)alloc_page();
    map_user_stack(new_pgd, new_stack_pa);

    Elf64_Phdr *phdr = (Elf64_Phdr *)(app_elf + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != 1) {
            continue;
        }

        uint64_t flags = load_segment_flags(&phdr[i]);
        uint64_t start_page = phdr[i].p_vaddr & ~0xFFFULL;
        uint64_t end_page = (phdr[i].p_vaddr + phdr[i].p_memsz + 0xFFFULL) & ~0xFFFULL;

        for (uint64_t page = start_page; page < end_page; page += PAGE_SIZE) {
            char *phys = (char *)alloc_page();
            uint64_t copy_start = (page > phdr[i].p_vaddr) ? page : phdr[i].p_vaddr;
            uint64_t copy_end = (page + PAGE_SIZE < phdr[i].p_vaddr + phdr[i].p_filesz)
                ? page + PAGE_SIZE
                : phdr[i].p_vaddr + phdr[i].p_filesz;

            for (int b = 0; b < PAGE_SIZE; b++) {
                phys[b] = 0;
            }

            if (copy_start < copy_end) {
                uint64_t src_offset = phdr[i].p_offset + (copy_start - phdr[i].p_vaddr);
                for (uint64_t b = 0; b < (copy_end - copy_start); b++) {
                    phys[(copy_start & 0xFFFULL) + b] = app_elf[src_offset + b];
                }
            }

            map_page(new_pgd, page, (uint64_t)phys, flags);
        }
    }

    trapframe_clear(tf);
    tf->epc = ehdr->e_entry;
    tf->user_sp = USER_STACK_TOP;

    tasks[current_task].satp = mmu_make_satp(new_pgd);
    asm volatile("csrw satp, %0" : : "r"(tasks[current_task].satp));
    asm volatile("sfence.vma zero, zero");

    free_user_page_table((uint64_t *)old_pgd_pa);
    return ehdr->e_entry;
}
