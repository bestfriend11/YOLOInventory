#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <smrtheap.h>

#include "tshared.h"

#define MAX_PIDS 32

void main(int argc, char *argv[])
{
	MEM_POOL pool;
	void *ptr;
	unsigned long i;

	printf("\npress any key to start...");
	getch();

	if ((pool = MemPoolAttachShared(NULL, T_SHARED_POOL_NAME)) != NULL)
	{
		printf("\nclient: attached to pool %p", pool);
	}
	else
	{
		unsigned long addr = 0;
		unsigned long pidCount = 0;
		unsigned long pids[MAX_PIDS];

		if (argc > 1)
			addr = strtoul(argv[1], NULL, 0);
		if (argc > 2)
			pidCount = strtoul(argv[2], NULL, 0);

		i = pidCount;

		if (pidCount && argc < pidCount+3)
		{
parmError:
			printf("\nusage: tshared [<addr>] [<pidCount> <pid1> <pid2>...]\n");
			return;
		}

		while (i--)
		{
			if ((pids[i] = strtoul(argv[i+3], NULL, 0)) == 0)
				goto parmError;				
		}

		printf("server: allocating 3MB named shared pool...\n");
		pool = MemPoolInitNamedSharedEx((void *)addr, pidCount, pids,
												  T_SHARED_POOL_NAME, 0x300000, 0);
	}

	printf("\npress any key to allocate %lu byte block from pool %p...",
			 i = rand() * 10, pool);
	getch();
	ptr = MemAllocPtr(pool, i, 0);

	printf("\npress any key to realloc process' block %p to size %lu ...",
			 ptr, i = rand() * 10);
	getch();
	ptr = MemReAllocPtr(ptr, i, 0);

	printf("\npress any key to free process' block %p ...", ptr);
	getch();
	MemFreePtr(ptr);

	printf("\npress any key to free pool and end process...");
	getch();
	MemPoolFree(pool);
}
