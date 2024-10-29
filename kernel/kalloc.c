// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];
//为每个cpu分配一个空间
void
kinit()
{
  for(int i=0;i<NCPU;i++){
    initlock(&kmems[i].lock, "kmem");
    printf("%d\n",i);
  }
  freerange(end, (void*)PHYSTOP);
}

//这里的end是空闲空间起始地址
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start) + PGSIZE;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  int cpu_id = cpuid();
  // printf("%d\n",cpu_id);
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
//同时放入了freelist中
  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist;
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  int cpu_id = cpuid();

  struct run *r;

  acquire(&kmems[cpu_id].lock);
  r = kmems[cpu_id].freelist;
  if(r)
    kmems[cpu_id].freelist = r->next;
  release(&kmems[cpu_id].lock);
  int temp = cpu_id;
  while(!r){
    cpu_id++;
    cpu_id = cpu_id % NCPU;
    if(cpu_id==temp){
      pop_off();
      return 0;
    }
    acquire(&kmems[cpu_id].lock);
    r = kmems[cpu_id].freelist;
    if(r)
    kmems[cpu_id].freelist = r->next;
    release(&kmems[cpu_id].lock);
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  pop_off();
  return (void*)r;
}
