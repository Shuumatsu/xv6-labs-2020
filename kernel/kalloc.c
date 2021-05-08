// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void* pa_start, void* pa_end);

extern char end[];  // first address after kernel.
                    // defined by kernel.ld.

struct run {
    struct run* next;
};

struct {
    struct spinlock lock;
    struct run* freelist;
} kmem[NCPU];

void _kfree(int id, void* pa) {
    struct run* r;

    if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem[id].lock);
    r->next = kmem[id].freelist;
    kmem[id].freelist = r;
    release(&kmem[id].lock);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void* pa) {
    int id;
    push_off();
    id = cpuid();
    pop_off();

    return _kfree(id, pa);
}

void* _kalloc(int id) {
    struct run* r;

    acquire(&kmem[id].lock);
    r = kmem[id].freelist;
    if (r) kmem[id].freelist = r->next;
    release(&kmem[id].lock);

    if (r) memset((char*)r, 5, PGSIZE);  // fill with junk
    return (void*)r;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void* kalloc(void) {
    int id;
    push_off();
    id = cpuid();
    pop_off();

    int i = id;
    do {
        void* ret = _kalloc(i);
        if (ret != 0) { return ret; }

        i = (i + 1) % NCPU;
    } while (i != id);

    return 0;
}

void _freerange(int id, void* pa_start, void* pa_end) {
    char* p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE) _kfree(id, p);
}

void freerange(void* pa_start, void* pa_end) {
    int id;
    push_off();
    id = cpuid();
    pop_off();

    _freerange(id, pa_start, pa_end);
}

void kinit() {
    for (int id = 0; id < NCPU; id += 1) {
        // initlock(&kmem[i].lock, sprintf("kmem_%d", i));
        initlock(&kmem[id].lock, "kmem");
    }

    uint64 avg = ((PHYSTOP - (uint64)end) / NCPU);
    char* pa_start = end;
    for (int id = 0; id < NCPU; id += 1) {
        // printf("%p %p %d\n", pa_start + avg, PHYSTOP,
        //        (uint64)(pa_start + avg) <= PHYSTOP);
        _freerange(id, pa_start, pa_start + avg);
        pa_start += avg;
    }
}
