#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <smrtheap.h>

#include "tshared.h"

void main()
{
	MEM_POOL namedPool, pool2, pool3;
	MEM_POOL *ppool2;
	MEM_POOL_ENTRY entry;
	unsigned long i = 0;
	static const char str1[] = "alloc 1";
	static const char str2[] = "and alloc 2!";
	void *str, *p;

	printf("Starting Server -- waiting for namedPool\n");
	do
		namedPool = MemPoolAttachShared(NULL, T_SHARED_POOL_NAME);
	while (namedPool == NULL);

	printf("Server: namedPool: %p\n", namedPool);

	printf("Server: waiting for Client to allocate from named pool\n");
	do
		i = MemPoolCount(namedPool);
	while (i == 0);

	entry.entry = NULL;
	do
		if (MemPoolWalk(namedPool, &entry) != MEM_POOL_OK)
		{
			printf("Server: ERROR -- no valid entry in namedPool\n");
			return;
		}
	while (!entry.isInUse);
	ppool2 = (MEM_POOL *)entry.entry;

	do
		printf("Server: waiting for Client to set pool2 in named pool alloc\n");
	while (*ppool2 == NULL);

	pool2 = *ppool2;
	printf("Server: pool2 as read from namedPool alloc: %p\n", pool2);
	MemFreePtr(entry.entry);

	if ((pool3 = MemPoolAttachShared(pool2, T_SHARED_POOL2_NAME)) != pool2)
	{
		printf("Server: pool2 returned from MemPoolAttachShared not equal Client pool2: %p instead of %p", pool3, pool2);
		return;
	}
		
	printf("Server: allocating two blocks from pool2\n");

	/* allocate 2 blocks from pool2, copying strings into them... */
	str = MemAllocPtr(pool2, sizeof(str1)+1, 0);
	strcpy(str, str1);
	printf("Server: pool2 entry %p: \"%s\"; size: %ld\n", str, str,
			 sizeof(str1)+1);
	str = MemAllocPtr(pool2, sizeof(str2)+1, 0);
	strcpy(str, str2);
	printf("Server: pool2 entry %p: \"%s\"; size: %ld\n", str, str,
			 sizeof(str2)+1);

	printf("Server: has allocated two blocks from pool2; pool count: %d",
		MemPoolCount(pool2));
	
	/* ... then allocate a third zero-initialized block: this is a signal
	 * to the client that two strings are available in the pool
	 */
	MemAllocPtr(pool2, 1, MEM_ZEROINIT);

	/* detach shared pools from current process */
	MemPoolFree(namedPool);
	MemPoolFree(pool2);
}
