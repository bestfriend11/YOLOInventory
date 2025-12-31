/* shmalloc.c -- SmartHeap(tm) ANSI C malloc/calloc/realloc/free.
 *
 * Copyright (C) 1991-2000 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the copyright owner.
 *
 * For 16-bit X86 platforms:
 * You can override malloc et al only in large data models (compact, large, or
 * huge).  You can compile shmalfar.c to define _fmalloc or farmalloc et al
 * in any memory model.
 *
 * Note to SmartHeap source customers:
 * This file in incompatible with sysansi.c, because malloc cannot be
 * defined in terms of itself.  In order to use SmartHeap malloc et al,
 * you must link in a system dependent memory allocation module, such
 * as syswin16.c.
 * Another work-around to the self-referential malloc problem is to
 * define the macro constant `MALLOC_MACRO' on the compiler command-line when
 * compiling this module.  The malloc et al exported from this file will
 * then be prefixed with `MEM_'.  If you include the smrtansi.h header
 * file in modules that use malloc, malloc et al will be defined as macros
 * that expand to MEM_malloc et al -- you will get the SmartHeap versions
 * while also allowing the compiler RTL version of malloc to coexist.
 *
 * Note: this source file should not be used for OS/2 IBM Visual Age 3.0,
 * shvisage.c and dbvisage.c provide CRT heap routines for that compiler.
 *
 */

#ifdef _AIX43
#pragma options PROCIMPORTED=MemAllocPtr:MemFreePtr:MemReAllocPtr:MemInitDefaultPool:_shi_enterCriticalSection:_shi_leaveCriticalSection
#pragma options DATAIMPORTED=MemDefaultPoolFlags:MemDefaultPool
#endif

#ifdef SHANSI
/* force macro version of malloc for ANSI version of SmartHeap to prevent
 * the possibility of a self-referential malloc function.
 */
#define MALLOC_MACRO
#endif /* SHANSI */

#if defined(MALLOC_MACRO) || defined(MEM_DEBUG)
#define NO_MALLOC_MACRO
#include "shmalloc.h"
#else
#include "smrtheap.h"
#endif /* MALLOC_MACRO || MEM_DEBUG */

#ifndef MALLOC_MACRO

#if defined(__IBMC__) && __IBMC__ >= 300 && !defined(_AIX)
#error shmalloc.c should not be used for IBM Visual Age C++ 3.00 or higher
#endif

#include <stdlib.h>
#if !(defined(WIN32) && defined(_MSC_VER) && defined(_M_IX86)) \
	 && !defined(SH_AIX_USER_MALLOC)
int SmartHeap_malloc = 1;
#endif

#endif /* MALLOC_MACRO */

#if defined(MEM_DEBUG) && defined(_MSC_VER)
#pragma optimize("y", off)
#endif

#if defined(_WIN32) && defined(_MSC_VER)
/* work-around for bug in Microsoft C onexit routine, in which memory blocks
 * are assumed to always have 4-byte granularity, since the MSC malloc
 * implementation has 16-byte per alloc overhead vs. SmartHeap's 2 bytes
 */
#ifndef MEM_DEBUG /* Debug SmartHeap handles MALLOC_INC internally */
#define MALLOC_INC(size) { if ((size) == 0) (size) = 4; else \
   { (size) += 3; (size) &= 0xFFFFFFFCul; } }
#endif /* MEM_DEBUG */
#define SIZE_DEC(size) ((size) &= 0xFFFFFFFCul)
#endif /* _MSC_VER */

#ifndef MALLOC_INC
#ifndef NO_MALLOC_ZERO
#define MALLOC_INC(size) { if ((size) == 0) (size)++; }
#else
#define MALLOC_INC(s)
#endif /* NO_MALLOC_ZERO */
#endif /* MALLOC_INC */

#ifndef SIZE_DEC
#define SIZE_DEC(s)
#endif /* SIZE_DEC */

void MEM_FAR * MEM_ENTRY_ANSI malloc(size_t size)
{
   void MEM_FAR *result;
   unsigned flags = 0;

#ifndef MEM_MT
   if (!MemDefaultPool && !MemInitDefaultPool())
		return NULL;
#endif /* MEM_MT */

   if (MemDefaultPoolFlags & MEM_POOL_ZEROINIT)
      flags |= MEM_ZEROINIT;

   MALLOC_INC(size);

again:
#ifdef MEM_DEBUG
   result = _dbgMemAllocPtr1(MemDefaultPool, size, flags, MEM_MEM_MALLOC, NULL, 0);
#else
   result = MemAllocPtr(MemDefaultPool, size, flags);
#endif /* MEM_DEBUG */

#if defined(_MSC_VER) && _MSC_VER >= 900
   if (!result && shi_call_new_handler_msc(size, 1))
      goto again;
#endif /* _MSC_VER */

   return result;
}

void MEM_FAR * MEM_ENTRY_ANSI calloc(size_t nobj, size_t siz)
{
   void MEM_FAR *result;
   unsigned long size = (unsigned long)nobj * (unsigned long)siz;
   
   if ((unsigned long)(size_t)size != size)
      /* ### should pass size as ULONG_MAX, report as MEM_BLOCK_TOO_BIG?? */
      return NULL;

#ifndef MEM_MT
   if (!MemDefaultPool && !MemInitDefaultPool())
		return NULL;
#endif /* MEM_MT */

   MALLOC_INC(size);
   
again:
#ifdef MEM_DEBUG
   result = _dbgMemAllocPtr1(MemDefaultPool, size, MEM_ZEROINIT,
      MEM_MEM_CALLOC, NULL, 0);
#else
   result = MemAllocPtr(MemDefaultPool, size, MEM_ZEROINIT);
#endif

#if defined(_MSC_VER) && _MSC_VER >= 900
   if (!result && shi_call_new_handler_msc(size, 1))
      goto again;
#endif /* _MSC_VER */

   return result;
}

void MEM_FAR * MEM_ENTRY_ANSI realloc(void MEM_FAR *p, size_t size)
{
#ifdef MEM_DEBUG
   return _dbgMEM_realloc(p, size, NULL, 0);
#else
   if (!p)
      return malloc(size);
   else if (size == 0)
   {
      free(p);
      return NULL;
   }
   else
   {
      unsigned flags = 0;

      if (MemDefaultPoolFlags & MEM_POOL_ZEROINIT)
         flags |= MEM_ZEROINIT;
      
      MALLOC_INC(size);
   
      return MemReAllocPtr(p, size, flags);
   }
#endif
}

void MEM_ENTRY_ANSI free(void MEM_FAR *p)
{
   if (p)
	{
#ifdef MEM_DEBUG
      _dbgMemFreePtr1(p, MEM_MEM_FREE, NULL, 0);
#else
      MemFreePtr(p);
#endif
	}
}

#ifdef MALLOC_MACRO

#ifdef MEM_DEBUG
void MEM_FAR * MEM_ENTRY _dbgMEM_alloc(size_t size, unsigned flags,MEM_API api,
                                       const char MEM_FAR *file, int line)
{
   void MEM_FAR *result;

#ifndef MEM_MT
   if (!MemDefaultPool && !MemInitDefaultPool())
		return NULL;
#endif /* MEM_MT */

   MALLOC_INC(size);
   
   if (MemDefaultPoolFlags & MEM_POOL_ZEROINIT)
      flags |= MEM_ZEROINIT;

again:
   result = _dbgMemAllocPtr1(MemDefaultPool, size, flags, api, file, line);
   
#if defined(_MSC_VER) && _MSC_VER >= 900
   if (!result && shi_call_new_handler_msc(size, 1))
      goto again;
#endif

   return result;
}

void MEM_FAR * MEM_ENTRY _dbgMEM_realloc(void MEM_FAR *p, size_t size,
                                         const char MEM_FAR *file, int line)
{
   unsigned flags = 0;

   if (MemDefaultPoolFlags & MEM_POOL_ZEROINIT)
      flags |= MEM_ZEROINIT;

   if (!p)
      return _dbgMEM_alloc(size, flags, MEM_MEM_REALLOC, file, line);
   else if (size == 0)
   {
      _dbgMemFreePtr1(p, MEM_MEM_REALLOC, file, line);
      return NULL;
   }
   else
   {
      MALLOC_INC(size);
   
      return _dbgMemReAllocPtr1(p, size, flags, MEM_MEM_REALLOC, file, line);
   }
}

void MEM_ENTRY _dbgMEM_free(void MEM_FAR *p,const char MEM_FAR *file, int line)
{
   if (p)
      _dbgMemFreePtr1(p, MEM_MEM_FREE, file, line);
}

MEM_POOL_STATUS MEM_ENTRY _dbgMemWalkHeap(
   MEM_POOL_ENTRY MEM_FAR *entry, const char MEM_FAR *file, int line)
{
#ifndef MEM_MT
   if (MemDefaultPool)
#endif /* MEM_MT */
      return _dbgMemPoolWalk(MemDefaultPool, entry, file, line);
#ifndef MEM_MT
   else
      return MEM_POOL_END;
#endif /* MEM_MT */
}

#if defined(_MSC_VER) && _MSC_VER >= 900
int MEM_ENTRY_ANSI _CrtIsValidPointer(
        const void * pv,
        unsigned int nBytes,
        int bReadWrite
        );
int MEM_ENTRY_ANSI _CrtIsValidPointer(
        const void * pv,
        unsigned int nBytes,
        int bReadWrite
        )
{
	return _dbg_MemCheckPtr((void *)pv, bReadWrite ? MEM_POINTER_READWRITE :
									MEM_POINTER_READONLY, nBytes, NULL, 0);
}

#elif defined(__IBMC__) && __IBMC__ < 300
/* Debug IBM C APIs -- defined in dbvisage.c for Visual Age 3.00+ */

#define __DEBUG_ALLOC__ 1
#undef malloc
#undef calloc
#undef realloc
#undef free
#include <malloc.h>

void * MEM_ENTRY_ANSI _debug_calloc(size_t s, size_t c, const char *f,size_t l)
{
	return _dbgMEM_alloc(s * c, MEM_ZEROINIT, MEM_MEM_CALLOC, f, l);
}
void MEM_ENTRY_ANSI _debug_free(void *p, const char *f, size_t l)
{
	_dbgMEM_free(p, f, l);
}
void * MEM_ENTRY_ANSI _debug_malloc(size_t s, const char *f, size_t l)
{
	return _dbgMEM_alloc(s, 0, MEM_MEM_MALLOC, f, l);
}
void * MEM_ENTRY_ANSI _debug_realloc(void *p, size_t s, const char *f,size_t l)
{
	return _dbgMEM_realloc(p, s, f, l);
}
int MEM_ENTRY_ANSI _debug_heapmin(const char *f, size_t l)
{
#ifndef MEM_MT
	if (MemDefaultPool)
#endif /* MEM_MT */
		_dbgMemPoolShrink(MemDefaultPool, f, l);
	return 0;
}
void MEM_ENTRY_ANSI _dump_allocated(int x) { (void) x; }

#endif /* __IBMC__ */
#endif  /* MEM_DEBUG */

#else /* MALLOC_MACRO */

/*** Compiler-specific funtions that must be overridden along with malloc ***/
/*** (see shmalfar.c, if present, for additional functions that SmartHeap ***/
/***  can override.)                                                      ***/

#ifdef _MSC_VER
#include <malloc.h>
/* Microsoft-only routines
 * (routines defined by both MSC and IBM C are defined later in this file)
 */
void MEM_FAR * __cdecl _expand(void MEM_FAR *p, size_t s)
 { MALLOC_INC(s); return MemReAllocPtr(p, s, MEM_RESIZE_IN_PLACE) ? p : NULL; }

int __cdecl _heapadd(void MEM_FAR *p, size_t s)
{
   /* adding user memory to heap is not supported by SmartHeap */
   (void)p; (void)s;
   return -1;
}

int __cdecl _heapwalk(_HEAPINFO MEM_FAR *mentry)
{
   /* must be static so that reserved_xxx fields are preserved across calls */
   static MEM_POOL_ENTRY entry;

#ifndef MEM_MT
   if (!MemDefaultPool)
      return _HEAPEMPTY;
#endif /* MEM_MT */

   if (!mentry
       || (mentry->_pentry && !MemCheckPtr(MemDefaultPool, mentry->_pentry)))
      return _HEAPBADPTR;

   if (!mentry->_pentry)
      entry.entry = NULL;
   
   switch (MemPoolWalk(MemDefaultPool, &entry))
   {
      case MEM_POOL_OK:
         mentry->_pentry = entry.entry;
         mentry->_size = (size_t)entry.size;
         mentry->_useflag = (entry.isInUse ? _USEDENTRY : _FREEENTRY);
         return _HEAPOK;

      case MEM_POOL_END:
         return _HEAPEND;

      default:
         return _HEAPBADNODE;
   }
}

#if UINT_MAX == 0xFFFFFFFFul
/* undocumented MSC routine that is exported from v. 2.0 CRT DLL */
size_t __cdecl _heapused(MEM_FAR size_t *pUsed, MEM_FAR size_t *pCommit)
{
   size_t result;

	/* force a reference to shmsvc.c to ensure that module is linked in */
#ifndef _M_IX86
	extern void *__imp__heapused;
	__imp__heapused = (void *)_heapused;
#endif

#ifndef MEM_MT
   if (!MemDefaultPool)
      return 0;
#endif /* MEM_MT */

   /* use total system memory consumed by heap as approximation for
    * all three _heapused() values
    */
   result = MemPoolSize(MemDefaultPool);

   if (pUsed)
      *pUsed = result;
   if (pCommit)
      *pCommit = result;

   return result;
}
#endif /* 32-bit */

#elif (defined(__BORLANDC__) && defined(_Windows) && UINT_MAX == 0xFFFFu) \
	|| defined(_AIX)
/* we must override strdup for BC++ in case user links with DLL C RTL */
#include <string.h>

char MEM_FAR * MEM_ENTRY_ANSI strdup(const char MEM_FAR *s)
{
   char MEM_FAR *result;
   size_t len = strlen(s) + 1;
   
   if ((result = (char MEM_FAR *)malloc(len)) != NULL)
      /* use memcpy, as BC++ will inline this fn */
      memcpy(result, s, len);
   
   return result;
}

#elif defined(__WATCOMC__) && UINT_MAX == ULONG_MAX \
   && !defined(__COMPACT__) && !defined(__LARGE__)
/* Watcom C with a small 32-bit data model */
#include <malloc.h>
void __near *_nmalloc(size_t s) { return malloc(s); }
void _nfree(void __near *p) { free(p); }

#endif /* _MSC_VER */

#if ((defined(__IBMC__) || defined(__IBMCPP__)) && !defined(_AIX)) \
	|| defined(_MSC_VER)
/* The IBM C Set/2 compiler calls _heapmin from the CRT termination code,
 * and _heapmin references a global variable defined in the same module as
 * malloc.  Therefore, to override malloc in IBM C, we must override _heapmin
 */

#include <malloc.h>

#if (defined(__IBMC__) || defined(__IBMCPP__)) && !defined(_AIX)
#if __IBMC__ < 300
/* IBM Set C/C++ compiler uses 12-byte global variable for multi-thread lib */
char _memory_sem[12];
#endif
#endif

int MEM_ENTRY_ANSI _heapmin(void)
{
#ifndef MEM_MT
   if (MemDefaultPool)
#endif /* MEM_MT */
      MemPoolShrink(MemDefaultPool);
   return 0;
}

size_t MEM_ENTRY_ANSI _msize(void MEM_FAR *p)
{
   size_t s = (size_t)MemSizePtr(p);

   SIZE_DEC(s);

   return s;
}

int MEM_ENTRY_ANSI _heapchk(void)
{
#ifndef MEM_MT
   if (!MemDefaultPool)
      return _HEAPEMPTY;
#endif /* MEM_MT */
   
   if (MemPoolCheck(MemDefaultPool))
      return _HEAPOK;
   else
      return _HEAPBADNODE;
}

int MEM_ENTRY_ANSI _heapset(unsigned int x)
{
   /* free-memory filling is automatic in Debug SmartHeap and HeapAgent */
   (void)x;
   return _heapchk();
}

#if defined(__IBMC__) && !defined(_AIX)
/* IBM C APIs */

int MEM_ENTRY_ANSI _heap_walk(int (* MEM_ENTRY_ANSI callback)(const void *,
	size_t, int, int ,const char *, size_t))
{
   MEM_POOL_ENTRY entry;
	MEM_POOL_STATUS status;
	MEM_POOL pool = MemDefaultPool;

	if (!pool)
		return _HEAPEMTPY;

   if (!MemPoolLock(pool))
      return _HEAPBADBEGIN;

	entry.entry = NULL;
   
   while ((status = MemPoolWalk(pool, &entry)) == MEM_POOL_OK)
   {
		const char *file = NULL;
		size_t line = 0;

#ifdef MEM_DEBUG
		DBGMEM_PTR_INFO info;
		if (_dbgMemPtrInfo(entry.entry, &info, NULL, 0))
		{
			file = info.createFile;
			line = (size_t)info.createLine;
		}
#endif
		
		if (callback(entry.entry, entry.size,
				(entry.isInUse ? _USEDENTRY : _FREEENTRY), _HEAPOK, file, line)
			 != 0)
		{
			status = MEM_POOL_END;
			break;
		}
   }

	if (status != MEM_POOL_END)
		callback(entry.entry, entry.size,
			(entry.isInUse ? _USEDENTRY : _FREEENTRY), _HEAPBADNODE, file, line);
	
   MemPoolUnlock(pool);

	return status == MEM_POOL_END ? _HEAPOK : _HEAPBADNODE;
}
#endif /* __IBMC__ */

#endif /* __IBMC__ || __IBMCPP__ */

#if defined(__osf__) || defined(sgi) || defined(_AIX) || defined(__sun) \
	|| defined(__DGUX__)

#include <unistd.h>

void *memalign(size_t alignment, size_t size)
{
	return MemAllocAligned(MemInitDefaultPool(), size, alignment, 0);
}

void *valloc(size_t size)
{
	static int page_size = 0;

	if (!page_size)
		page_size = getpagesize();
	
  return memalign(page_size, size);
}

#include <malloc.h>

int mallopt(int command, int value)
{
	return 0;
}

struct mallinfo mallinfo()
{
	static struct mallinfo info;

	return info;
}

#endif /* __osf__ || sgi || _AIX || __sun || __DGUX__ */

#endif /* !MALLOC_MACRO */
