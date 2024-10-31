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
#define NBUCKETS 13
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  struct buf hashbucket[NBUCKETS];//每个hash桶作为buf类型，头指针不放内容，相当于head
  struct spinlock global_lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

int getHash(int input){
  input %= NBUCKETS;
  return input;
}

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcache.lock[i], "bcache_hash");
    bcache.hashbucket[i].next = &bcache.hashbucket[i];//初始化每个头节点
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    // printf("bcache.hashbucker[%d]的地址为%p\n",i,&bcache.hashbucket[i]);
    
  }
  initlock(&bcache.global_lock, "bcache_global");

  // Create linked list of buffers
  int num = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[getHash(num)].next;
    b->prev = &bcache.hashbucket[getHash(num)];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[getHash(num)].next->prev = b;
    bcache.hashbucket[getHash(num)].next = b;
    num++;
    // printf("%d,%d\n",num,getHash(num));
  }//以单向链表的方式相连

  // for(int i=0;i<NBUCKETS;i++){
  //   for(b = bcache.hashbucket[i].next; b != &bcache.hashbucket[i]; b = b->next){
  //     printf("发生了死循环？%d\n",i);
  //   }
  // }
  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  // printf("准备获取当前哈希桶%d的锁\n",getHash(blockno));
  acquire(&bcache.lock[getHash(blockno)]);//获取当前哈希桶的锁
  // printf("获取当前哈希桶%d的锁\n",getHash(blockno));
  // Is the block already cached?
  for(b = bcache.hashbucket[getHash(blockno)].next; b != &bcache.hashbucket[getHash(blockno)]; b = b->next){
    // printf("发生了死循环？");
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[getHash(blockno)]);
      // printf("命中！释放当前哈希桶%d的锁\n",getHash(blockno));
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // to steal from other 
  // get the global lock, then search from 
  // 查看自己链表中是否有空闲锁，若有，则自己替换
  // printf("未命中！不释放当前哈希桶%d的锁\n",getHash(blockno));
  for(b = bcache.hashbucket[getHash(blockno)].prev; b != &bcache.hashbucket[getHash(blockno)]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[getHash(blockno)]);
      // printf("获得空闲块，释放当前哈希桶%d的锁\n",getHash(blockno));
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[getHash(blockno)]);
  // printf("没有获得空闲块，释放当前哈希桶%d的锁\n",getHash(blockno));
  //若自己的锁不存在空闲，在其他桶中查找空闲块
  //先将全局锁拿到
  // printf("准备获取当前全局锁\n");
  acquire(&bcache.global_lock);
  // printf("获取当前全局锁\n");

  for(int i = getHash(blockno+1);i != getHash(blockno);i = getHash(i+1)){
    // printf("没有获得空闲块，查看其他桶并准备获取其锁\n",i);
    acquire(&bcache.lock[i]);//查找这个桶是否有空闲
    // printf("没有获得空闲块，查看其他桶并获取其锁\n",i);
      for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){  
        if(b->refcnt == 0) {
          b->prev->next = b->next;
          b->next->prev = b->prev;

          acquire(&bcache.lock[(getHash(blockno))]);
          b->next = bcache.hashbucket[getHash(blockno)].next;
          b->prev = &bcache.hashbucket[getHash(blockno)];
          bcache.hashbucket[getHash(blockno)].next->prev = b;
          bcache.hashbucket[getHash(blockno)].next = b;
          release(&bcache.lock[(getHash(blockno))]);

          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          
          release(&bcache.global_lock);
          release(&bcache.lock[i]);
          acquiresleep(&b->lock);
          return b;
        }
      }
      release(&bcache.lock[i]);
  }
  release(&bcache.global_lock);
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

  releasesleep(&b->lock);

  acquire(&bcache.lock[getHash(b->blockno)]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[getHash(b->blockno)].next;
    b->prev = &bcache.hashbucket[getHash(b->blockno)];
    bcache.hashbucket[getHash(b->blockno)].next->prev = b;
    bcache.hashbucket[getHash(b->blockno)].next = b;
  }
  
  release(&bcache.lock[getHash(b->blockno)]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[getHash(b->blockno)]);
  b->refcnt++;
  release(&bcache.lock[getHash(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[getHash(b->blockno)]);
  b->refcnt--;
  release(&bcache.lock[getHash(b->blockno)]);
}


