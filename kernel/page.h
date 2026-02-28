// page.h
#ifndef PAGE_H
#define PAGE_H

#define PAGE_SIZE 4096
#define RAM_START 0x80000000L
#define RAM_SIZE  (128 * 1024 * 1024 ) // QEMU 默认的 128MB RAM
#define RAM_END   (RAM_START + RAM_SIZE)

void page_init(void);
void *alloc_page(void);
void free_page(void *p);

#endif