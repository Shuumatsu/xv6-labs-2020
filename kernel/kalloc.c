// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct {
    struct spinlock lock;
    int cnt[PHYSTOP / PGSIZE];
} reference_counter;

void freerange(void* pa_start, void* pa_end);

extern char end[];  // first address after kernel.
                    // defined by kernel.ld.

struct run {
    struct run* next;
};

struct {
    struct spinlock lock;
    struct run* freelist;
} kmem;

void inc_cnt(void* pa) {
    acquire(&reference_counter.lock);
    reference_counter.cnt[(uint64)pa / PGSIZE] += 1;
    release(&reference_counter.lock);
}

void dec_cnt(void* pa) {
    acquire(&reference_counter.lock);
    reference_counter.cnt[(uint64)pa / PGSIZE] -= 1;
    release(&reference_counter.lock);
}

void kinit() {
    initlock(&reference_counter.lock, "reference_counter");
    for (int i = 0; i < PHYSTOP / PGSIZE; i += 1) {
        reference_counter.cnt[i] = 0;
    }

    initlock(&kmem.lock, "kmem");
    freerange(end, (void*)PHYSTOP);
}

void freerange(void* pa_start, void* pa_end) {
    for (char* pa = (char*)PGROUNDUP((uint64)pa_start);
         pa + PGSIZE <= (char*)pa_end; pa += PGSIZE) {
        memset(pa, 1, PGSIZE);

        acquire(&kmem.lock);

        struct run* r = (struct run*)pa;
        r->next = kmem.freelist;
        kmem.freelist = r;

        release(&kmem.lock);
    };
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void* pa) {
    if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    acquire(&kmem.lock);
    acquire(&reference_counter.lock);

    reference_counter.cnt[(uint64)pa / PGSIZE] -= 1;

    if (reference_counter.cnt[(uint64)pa / PGSIZE] == 0) {
        memset(pa, 1, PGSIZE);
        struct run* r = (struct run*)pa;
        r->next = kmem.freelist;
        kmem.freelist = r;
    }

    release(&reference_counter.lock);
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void* kalloc(void) {
    struct run* r;

    acquire(&kmem.lock);
    acquire(&reference_counter.lock);

    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
        memset((void*)r, 5, PGSIZE);  // fill with junk
        reference_counter.cnt[(uint64)r / PGSIZE] += 1;
    }

    release(&reference_counter.lock);
    release(&kmem.lock);

    return (void*)r;
}

int cow_alloc(pagetable_t pagetable, uint64 va) {
    pte_t* pte = walk(pagetable, PGROUNDDOWN(va), 0);
    if (pte == 0) { return -1; }

    uint64 pa = PTE2PA(*pte);
    if (pa == 0) { return -1; }

    void* allocated = kalloc();
    if (allocated == 0) { return -1; }

    memmove(allocated, (void*)pa, PGSIZE);

    uint flags = (PTE_FLAGS(*pte) & ~PTE_COW) | PTE_W;
    *pte = PA2PTE((uint64)allocated) | flags;

    kfree((void*)pa);
    return 0;
}