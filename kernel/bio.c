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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  uint init_time = ticks;
  // Create linked list of buffers
  // 变为单项链表
  // bcache.head.prev = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    b->timestamp = init_time;//初始化将所有的空闲buf时间设置为init_time
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      acquiresleep(&b->lock);
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        b->timestamp = ticks;
        return b;
      }
      releasesleep(&b->lock);
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //变为单向链表，向下一个查找
  //查找时间最早的节点，将该节点抛弃
  acquire(&bcache.lock);

  uint earlier_time = ~0;//init
  struct buf* earlier_buf = 0;

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    if(b->refcnt == 0) {//找到空闲buf，记录一下其时间
      // acquiresleep(&b->lock);
      // printf("earlier_time = %d, b->timestamp = %d\n",earlier_time,b->timestamp);
      if(earlier_time > b->timestamp){
        // if(holdingsleep(&earlier_buf->lock)){
          // releasesleep(&earlier_buf->lock);
        // }
        earlier_time = b->timestamp;
        earlier_buf = b;//此时的earlier buf记录就是当前搜索的最早buf
      }
    }
  }
  if(earlier_buf!=0){
    earlier_buf->dev = dev;
    earlier_buf->blockno = blockno;
    earlier_buf->valid = 0;
    earlier_buf->refcnt = 1;
    earlier_buf->timestamp = ticks;
    release(&bcache.lock);
    acquiresleep(&earlier_buf->lock);
    return earlier_buf;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  if (b->refcnt == 1) {
    // no one is waiting for it.
    b->timestamp = ticks;//刚刚使用完毕，将时间戳设置为当前时间
  }
  b->refcnt--;
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


