/* defpool.c -- SmartHeap (tm) Default memory pool.
 *
 * Copyright (C) 1991-2000 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the copyright owner.
 *
 */

#include "smrtheap.h"
#ifndef MEM_MT

MEM_POOL MemDefaultPool = 0;

MEM_POOL MEM_ENTRY MemInitDefaultPool()
{
	if (!_shi_enterCriticalSection())
		return NULL;

   if (!MemDefaultPool)
   {
#ifndef SYS_SMP
		void MEM_ENTRY shi_notSMP(void);
		shi_notSMP();
#endif

#if UINT_MAX == 0xFFFFu && (defined(_MSC_VER) || defined(__BORLANDC__))
#ifdef MEM_DEBUG
		extern int _shi_terminateRef;
		_shi_terminateRef = 1;  /* make sure agenterm.cpp is linked in */
#endif
      SmartHeap_far_malloc = 1;  /* make sure shmalfar.c is linked in */
#endif
      SmartHeap_malloc = 1;

#ifdef MEM_DEBUG
      if (dbgMemGuardSize != UINT_MAX)
         dbgMemSetGuardSize(dbgMemGuardSize);
      if (dbgMemGuardFill != 0)
         dbgMemSetGuardFill(dbgMemGuardFill);
      if (dbgMemFreeFill != 0)
         dbgMemSetFreeFill(dbgMemFreeFill);
      if (dbgMemInUseFill != 0)
         dbgMemSetInUseFill(dbgMemInUseFill);
#endif

      /* note: MEM_POOL_DEFAULT flag causes MEM_POOL_SHARED flag to be set
       * iff caller is a DLL in 16-bit Windows version
       */
      if ((MemDefaultPool = MemPoolInit(MemDefaultPoolFlags)) != NULL)
      {
         if (MemDefaultPoolPageSize != 0)
            MemPoolSetPageSize(MemDefaultPool, MemDefaultPoolPageSize);
         if (MemDefaultPoolBlockSizeFS != USHRT_MAX)
            MemPoolSetBlockSizeFS(MemDefaultPool, MemDefaultPoolBlockSizeFS);
			dbgMemPoolSetName(MemDefaultPool, "Default");
      }
   }
   
	_shi_leaveCriticalSection();	

	return MemDefaultPool;
}

MEM_BOOL MEM_ENTRY MemFreeDefaultPool(void)
{
	MEM_POOL pool;

	if (!_shi_enterCriticalSection())
		return 0;
	if ((pool = MemDefaultPool) != NULL)
      MemDefaultPool = 0;
	_shi_leaveCriticalSection();

	if (pool)
		return MemPoolFree(pool);

   return 0;
}
#endif /* MEM_MT */
