#include <stdio.h>
#include <smrtheap.h>

#include "tshared.h"

void main()
{
   MEM_POOL namedPool;
   MEM_POOL pool2;
   MEM_POOL *ppool2;
   MEM_POOL_ENTRY entry;
   unsigned long i = 0;
   
   printf("Starting Client\n");

   namedPool = MemPoolInitNamedShared(T_SHARED_POOL_NAME, T_SHARED_POOL_SIZE, 0);
   printf("Client: namedPool: %p\n", namedPool);

#ifdef __OS2__
   /* OS/2 allows us to create an unnamed shared memory pool */
   pool2 = MemPoolInit(MEM_POOL_SHARED);
#else
   pool2 = MemPoolInitNamedShared(T_SHARED_POOL2_NAME, T_SHARED_POOL_SIZE, 0);
#endif

   printf("Client: pool2: %p\n", pool2);

   printf("Client: allocating zero-inited block from namedPool\n");
   ppool2 = MemAllocPtr(namedPool, sizeof(MEM_POOL *), MEM_ZEROINIT);

   printf("Client: writing pool2 to address to namedPool block\n");
   *ppool2 = pool2;

   printf("Client: waiting for Server to alloc three blocks from pool2\n");

   do
      i = MemPoolCount(pool2);
   while (i < 3);

   printf("Client: reading allocations from pool2\n");
   while (i--)
   {
      entry.entry = NULL;
      do
         if (MemPoolWalk(pool2, &entry) != MEM_POOL_OK)
            return;
      while (!entry.isInUse);

      printf("Client: pool2 entry %p: \"%s\"; size: %ld\n",
             entry.entry, entry.entry, entry.size);
#ifdef MEM_DEBUG
/* intentional overwrite to test interprocess file name error reporting */
      ((char *)entry.entry)[entry.size] = 'z';
#endif
      MemFreePtr(entry.entry);
   }

   /* detach shared pools from current process */
   MemPoolFree(namedPool);
   MemPoolFree(pool2);
}
