// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
    struct spinlock lock[NBUCKET];
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    struct buf head[NBUCKET];
} bcache;

void binit(void) {
    for (uint bucket = 0; bucket < NBUCKET; bucket += 1) {
        initlock(&bcache.lock[bucket], "bcache");
        bcache.head[bucket].prev = &bcache.head[bucket];
        bcache.head[bucket].next = &bcache.head[bucket];
    }

    for (int i = 0; i < NBUF; i += 1) {
        uint bucket = i % NBUCKET;

        struct buf* b = &bcache.buf[i];
        b->next = bcache.head[bucket].next;
        b->prev = &bcache.head[bucket];
        initsleeplock(&b->lock, "buffer");

        bcache.head[bucket].next->prev = b;
        bcache.head[bucket].next = b;
    }
}

struct buf* evict(uint bucket) {
    uint target_bucket = bucket;
    do {
        acquire(&bcache.lock[target_bucket]);

        // Not cached.
        // Recycle the least recently used (LRU) unused buffer.
        for (struct buf* b = bcache.head[target_bucket].prev;
             b != &bcache.head[target_bucket]; b = b->prev) {
            if (b->refcnt == 0) {
                b->prev->next = b->next;
                b->next->prev = b->prev;

                release(&bcache.lock[target_bucket]);
                return b;
            }
        }

        release(&bcache.lock[target_bucket]);

        target_bucket = (target_bucket + 1) % NBUCKET;
    } while (target_bucket != bucket);

    return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf* bget(uint dev, uint blockno) {
    uint bucket = blockno % NBUCKET;

    acquire(&bcache.lock[bucket]);
    for (struct buf* b = bcache.head[bucket].next; b != &bcache.head[bucket];
         b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.lock[bucket]);
            acquiresleep(&b->lock);
            return b;
        }
    }
    release(&bcache.lock[bucket]);

    struct buf* ret = evict(bucket);
    if (ret == 0) { panic("bget: no buffers"); }

    acquire(&bcache.lock[bucket]);
    ret->prev = &bcache.head[bucket];
    ret->next = bcache.head[bucket].next;
    ret->next->prev = ret;
    ret->prev->next = ret;

    for (struct buf* b = bcache.head[bucket].next; b != &bcache.head[bucket];
         b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.lock[bucket]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    ret->dev = dev;
    ret->blockno = blockno;
    ret->valid = 0;
    ret->refcnt = 1;

    release(&bcache.lock[bucket]);

    acquiresleep(&ret->lock);
    return ret;
}

// Return a locked buf with the contents of the indicated block.
struct buf* bread(uint dev, uint blockno) {
    struct buf* b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf* b) {
    if (!holdingsleep(&b->lock)) panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf* b) {
    if (!holdingsleep(&b->lock)) panic("brelse");
    releasesleep(&b->lock);

    uint bucket = b->blockno % NBUCKET;

    acquire(&bcache.lock[bucket]);

    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head[bucket].next;
        b->prev = &bcache.head[bucket];
        bcache.head[bucket].next->prev = b;
        bcache.head[bucket].next = b;
    }

    release(&bcache.lock[bucket]);
}

void bpin(struct buf* b) {
    uint bucket = b->blockno % NBUCKET;

    acquire(&bcache.lock[bucket]);
    b->refcnt++;
    release(&bcache.lock[bucket]);
}

void bunpin(struct buf* b) {
    uint bucket = b->blockno % NBUCKET;

    acquire(&bcache.lock[bucket]);
    b->refcnt--;
    release(&bcache.lock[bucket]);
}
