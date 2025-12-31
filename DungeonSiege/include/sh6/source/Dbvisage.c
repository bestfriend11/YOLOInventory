/* dbvisage.c -- SmartHeap(tm) IBM Visual Age C++ integration (debug APIs)
 * For OS/2 IBM Visual Age C++
 *
 * Copyright (C) 1995 by Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the author.
 *
 * This source file overrides all definitions from the following object
 * modules of the Visual Age C runtime:
 *
 * rdbg
 * 
 * Note that SmartHeap does *not* override
 * debug_malloc/debug_calloc/debug_free/debug_realloc directly,
 * since these are defined by VisualAge to call
 * debug_umalloc/debug_ufree/debug_urealloc/debug_ucalloc,
 * which SmartHeap *does* override.
 *
 * Hence, shmalloc.c is *not* included in Visual Age builds of SmartHeap.
 *
 */

#if !defined(__IBMC__) || __IBMC__ < 300 || !defined(MEM_DEBUG)
#error shvisage.c is for IBM Visual Age C++ 3.00 or higher Debug SmartHeap builds only
#endif

#include <malloc.h>
#include "shmalloc.h"

extern int SmartHeap_debug = 0;

/* module rdbg: _debug_ufree, _debug_urealloc, _debug_uheapmin, _debug_ucalloc,
 * _debug_umalloc, _udump_allocated_delta, __udump_allocated_delta,
 * _udump_allocated, __udump_allocated, _int_ipmd_heap_check, __uheap_check,
 * _uheap_check
 */
#define __DEBUG_ALLOC__ 1
#include <umalloc.h>

void MEM_ENTRY_ANSI _debug_ufree(void *p, const char *f, size_t l);
void MEM_ENTRY_ANSI _debug_ufree(void *p, const char *f, size_t l)
{
   _dbgMEM_free(p, f, l);
}

void * MEM_ENTRY_ANSI _debug_urealloc(void *p,size_t s,const char *f,size_t l);
void * MEM_ENTRY_ANSI _debug_urealloc(void *p, size_t s,const char *f,size_t l)
{
   return _dbgMEM_realloc(p, s, f, l);
}

int MEM_ENTRY_ANSI _debug_uheapmin(Heap_t h, const char *f, size_t l)
{
   _dbgMemPoolShrink((MEM_POOL)h, f, l);
   return 0;
}

void * MEM_ENTRY_ANSI _debug_ucalloc(Heap_t h, size_t s, size_t c,
                                     const char *f, size_t l)
{
   s *= c;
   if (s == 0)
      s++;
   return _dbgMemAllocPtr1((MEM_POOL)h, s, MEM_ZEROINIT, MEM_MEM_CALLOC, f, l);
}

void * MEM_ENTRY_ANSI _debug_umalloc(Heap_t h,size_t s,const char *f, size_t l)
{
   if (s == 0)
      s++;
   return _dbgMemAllocPtr1((MEM_POOL)h, s, 0, MEM_MEM_MALLOC, f, l);
}

/* SmartHeap does not implement "dump" functions -- stub these */
#undef _udump_allocated_delta
void MEM_ENTRY_ANSI _udump_allocated_delta(Heap_t h,int x) { (void)h;(void)x;}
void MEM_ENTRY_ANSI __udump_allocated_delta(Heap_t h, int x, const char * f,
  size_t s) { (void)h; (void)x; (void)f; (void)s; }
#undef _udump_allocated
void MEM_ENTRY_ANSI _udump_allocated(Heap_t h,int x) { (void)h; (void)x; }
void MEM_ENTRY_ANSI __udump_allocated(Heap_t h, int x, const char *f, size_t s)
{ (void)h; (void)x; (void)f; (void)s; }

void MEM_ENTRY_ANSI _int_ipmd_heap_check(void);
#undef _heap_check
void MEM_ENTRY_ANSI _int_ipmd_heap_check(void) { _heap_check(); }

#undef _uheap_check
void MEM_ENTRY_ANSI _uheap_check(Heap_t h) { __uheap_check(h, NULL, 0); }

void MEM_ENTRY_ANSI __uheap_check(Heap_t h, const char *f, size_t l)
{
   _dbgMemPoolCheck((MEM_POOL)h, f, l);
}
