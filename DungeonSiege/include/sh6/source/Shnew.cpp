// shnew.cpp -- SmartHeap(tm) C++ operators new/delete
// Professional Memory Management Library
//
// Copyright (C) 1991-2000 Compuware Corporation.
// All Rights Reserved.
//
// No part of this source code may be copied, modified or reproduced
// in any form without retaining the above copyright notice.
// This source code, or source code derived from it, may not be redistributed
// without express written permission of the copyright owner.
//
// COMMENTS:
// - This module provides SmartHeap definitions for C++ global operators new
//   and delete.
//
// - Compile this file with your C++ compiler and link with your object
//   modules to replace new and delete.
//
// - Can be used in both EXEs and DLLs.
//
// - For 16-bit x86 environments, use only in large or compact memory model.
//
// - Use MemDefaultPool global variable to retrieve a memory pool to pass
//   to SmartHeap functions that accept a pool as a parameter,
//   e.g. MemPoolCount, MemPoolSize, MemPoolWalk, etc.
//
// - If you are using SmartHeap operator new with Borland C++ ObjectWindows
//   Libraries (OWL) version 1.x, include the define -DOWL=1 when compiling
//   shnew.cpp.  shnew.cpp then implements the SafetyPool functionality in
//   a way that does not conflict with SmartHeap.
//
//   Note that for OWL version 2.0 or higher, you can use the standard
//   SmartHeap operator new, since the new version of OWL does not redefine
//   global operators new and delete.
//
//   If you want to use the DLL version of OWL with SmartHeap, you must
//   recompile the OWL DLL from its source code.  Modify the OWL make file to
//   include shnew.cpp as a replacement for safepool.cpp (i.e. do not link
//   safepool.cpp into the OWL DLL).  If you are using the DLL version of
//   OWL, define the OWL=1 macro when compiling shnew.cpp for linking into
//   the OWL DLL, but do _not_ define the OWL macro when compiling shnew.cpp
//   for linking into your application, or you will have multiple definitions
//   for the SafetyPool class.
//
// - If you are using SmartHeap operator new with the _DEBUG version of
//   the Microsoft Foundation Class Library (MFC), then include the define
//   -DMFC=1 when compiling shnew.cpp.  shnew.cpp then implements the _DEBUG
//   memory-checking APIs of MFC.
//

#ifdef MFC
#if defined(_MSC_VER)
#if (!defined(MEM_DEBUG) || !defined(_DEBUG)) && _MSC_VER < 1000
#error Both MEM_DEBUG and _DEBUG must be defined for MSC version of SHNEW.CPP -- use standard SmartHeap operator new for non-debug MFC.
#endif
#include <afx.h>
#ifdef MEM_DEBUG
static void _shi_updateAfxMemDF();
#endif
#else
#undef MFC
#endif
#endif /* MFC */

#define _SHI_Pool _SHI_Pool
#include "smrtheap.hpp"

#ifdef OWL
#ifdef __BORLANDC__
#include <safepool.h>
int SafetyPool::Size = DEFAULT_SAFETY_POOL_SIZE;
void far *SafetyPool::safetyPool = DEBUG_NEW char[DEFAULT_SAFETY_POOL_SIZE];
#else
#undef OWL
#endif
#endif /* OWL */


#if defined(MEM_DEBUG) && defined(_MSC_VER)
#pragma optimize("y", off)
#endif

#ifdef _MSC_VER
#pragma warning(disable:4505)

#if UINT_MAX == 0xFFFFu
_PNHH mem_new_handler_msc_huge = 0;
#endif
#endif /* _MSC_VER */

#if defined(__IBMCPP__) && __IBMCPP__ >= 300
#define _new_handler __new_handler
#endif

pnh _new_handler = 0;
extern "C" {
int SmartHeap_new = 1;
}

// Global operators new and delete
void MEM_FAR * MEM_ENTRY_ANSI shi_New(unsigned long sz DBG_FORMAL,
                                      unsigned flags, MEM_POOL pool)
{
   void MEM_FAR *result;

   if (sz == 0)
      sz++;

#if defined(MFC) && defined(MEM_DEBUG)
   _shi_updateAfxMemDF();
#endif

#ifdef ZAPP
   flags |= MEM_ZEROINIT;
#endif
   
   if (MemDefaultPoolFlags & MEM_POOL_ZEROINIT)
      flags |= MEM_ZEROINIT;

#ifndef MEM_MT
	if (!MemDefaultPool)
		MemInitDefaultPool();
#endif /* MEM_MT */

   for (;;)
   {
      if (pool || (pool = MemDefaultPool) != 0)
      {
#ifdef MEM_DEBUG
         if ((result = _dbgMemAllocPtr1(pool, sz, flags, MEM_NEW DBG_ACTUAL))
             != 0)
#else
         if ((result = MemAllocPtr(pool, sz, flags)) != 0)
#endif
            return result;
      }

#ifdef _MSC_VER
#if UINT_MAX == 0xFFFFu
      if ((flags & MEM_HUGE) && mem_new_handler_msc_huge)
      {
         if (!mem_new_handler_msc_huge(sz, 1))
            break;
      }
      else
#endif
      if (shi_call_new_handler_msc((size_t)sz, 0))
			continue;
#endif
      if (_new_handler)
         _new_handler();
      else
         break;
   }

#ifdef MFC
   AfxThrowMemoryException();
#endif

   return 0;
}

#ifdef MEM_DEBUG
void MEM_FAR * MEM_ENTRY_ANSI shi_New(unsigned long sz, unsigned flags,
                                      MEM_POOL pool)
{
   return shi_New(sz, 0, 0, flags, pool);
}
#endif

#ifdef SHI_ARRAY_NEW
void MEM_FAR *operator new[](size_t sz) MEM_CPP_THROW1(bad_alloc)
{
   return shi_New(sz);
}
#endif /* SHI_ARRAY_NEW */

#ifdef SHI_ARRAY_DELETE
void operator delete[](void MEM_FAR *p) MEM_CPP_THROW
{
   ::operator delete(p);
}
#endif /* SHI_ARRAY_DELETE */

#if defined(OWL)
// Borland C++ OWL-specific operator new
void far *operator new(size_t sz)
{
   void far *allocated;

   while ((allocated = shi_New(sz)) == 0
         && SafetyPool::safetyPool)
   {
      delete SafetyPool::safetyPool;
      SafetyPool::safetyPool = 0;
   }

   return allocated;
}

int SafetyPool::Allocate(size_t sz)
{
   delete safetyPool;
   safetyPool = DEBUG_NEW char[sz];
   Size = sz;

   return SafetyPool::safetyPool != 0;
}

#elif !(/* defined(MEM_DEBUG) && */ defined(WIN32) && defined(_MSC_VER) \
   && defined(_M_IX86)) || defined(ZAPP)
// these operator new variants are defined in shnewjmp.asm for Win32/VC++
void MEM_FAR *operator new(size_t sz) MEM_CPP_THROW1(bad_alloc)
{
   return shi_New(sz);
}
#endif /* OWL */


#ifdef ZAPP
void freePages(void);
void freePages() { if (MemDefaultPool) MemPoolShrink(MemDefaultPool); }
#endif /* ZAPP */

// Microsoft Foundation Class versions of operator new
#ifdef MFC

#ifndef AFXAPI
#define AFXAPI PASCAL
#endif
#if _MFC_VER > 0x0100
#define AFXAPI_ALLOC_HOOK AFXAPI
#else
#define AFXAPI_ALLOC_HOOK 
#endif
#if _MFC_VER >= 0x0400
#define AFXAPI_NEW AFXAPI 
#else
#define AFXAPI_NEW AFX_CDECL
#endif


#ifdef MEM_DEBUG
#ifdef _WINDOWS
#define _AFX_EXPORT _export
#include <afxwin.h>
#else
#include <stdio.h>
#endif

/* out-of-line version required here, as _DEBUG MFC library has non-inlined
 * calls to operator new that passes file/line info
 */
#if !(/* defined(MEM_DEBUG) && */ defined(WIN32) && defined(_MSC_VER) \
   && defined(_M_IX86))
// these operator new variants are defined in shnewjmp.asm for Win32/VC++
void MEM_FAR *operator new(size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#endif // Win32/VC++


void MEM_FAR * AFXAPI_NEW CObject::operator new(size_t sz)
   { return shi_New(sz); }
void MEM_FAR * AFXAPI_NEW CObject::operator new(size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
void AFXAPI_NEW CObject::operator delete(void MEM_FAR * p)
	{ ::delete p; }

#if defined(_AFXDLL) && defined(_WINDOWS) && UINT_MAX == 0xFFFFu
extern "C" void MEM_FAR * _AFX_EXPORT CALLBACK AfxAppAlloc(size_t size)
{
   if (AfxGetApp() && _AfxGetAppDebug()->lpszAllocFileName)
      return shi_New(size, _AfxGetAppDebug()->lpszAllocFileName,
                             _AfxGetAppDebug()->nAllocLine);
   else
      return shi_New(size);
}
extern "C" void _AFX_EXPORT CALLBACK AfxAppFree(void* p) { ::delete p; }
extern "C" FARPROC _AFX_EXPORT CALLBACK AfxAppSetNewHandler(FARPROC fn)
   { return (FARPROC)_set_new_handler((_PNH)fn); }
extern "C" void* _AFX_EXPORT CALLBACK AfxAppReAlloc(void* p, size_t size)
   { return realloc(p, size); }
#endif /* 16-bit _AFXDLL */

#if UINT_MAX == 0xFFFFu
extern "C" int NEAR afxMemDF = allocMemDF;
#elif _MFC_VER < 0x0300
extern int afxMemDF = allocMemDF;
#endif

static int NEAR _shi_afxMemDF = afxMemDF;

static void _shi_updateAfxMemDF()
{
   if (afxMemDF != _shi_afxMemDF)
   {
      dbgMemSetSafetyLevel(!(afxMemDF & allocMemDF) ? MEM_SAFETY_SOME :
                           (afxMemDF & checkAlwaysMemDF) ? MEM_SAFETY_DEBUG :
                           MEM_SAFETY_FULL);
      if (MemDefaultPool)
         dbgMemPoolDeferFreeing(MemDefaultPool, afxMemDF & delayFreeMemDF);
      _shi_afxMemDF = afxMemDF;
   }
}

BOOL AFXAPI AfxCheckMemory()
   { return !MemDefaultPool || MemPoolCheck(MemDefaultPool); }

BOOL AFXAPI AfxIsMemoryBlock(const void *ptr, UINT size, long *pAllocNum)
{
   if (pAllocNum)
   {
      DBGMEM_PTR_INFO info;
      if (dbgMemPtrInfo((void *)ptr, &info))
         *pAllocNum = info.allocCount;
      else
         *pAllocNum = 0;
   }

   return dbgMemCheckPtr((void *)ptr, MEM_POINTER_HEAP, size);
}

#if !defined(_AFXDLL) || _MFC_VER >= 0x0300
BOOL AFXAPI AfxEnableMemoryTracking(BOOL) { return TRUE; }
#endif
static AFX_ALLOC_HOOK NEAR _shi_mfcAllocHookFn = NULL;
static void MEM_ENTRY _shi_mfcAllocHook(MEM_ERROR_INFO FAR *, unsigned long);
static void MEM_ENTRY _shi_mfcAllocHook(MEM_ERROR_INFO FAR *info,unsigned long)
{
   if (info->errorAPI == MEM_NEW && _shi_mfcAllocHookFn)
      _shi_mfcAllocHookFn((size_t)info->argSize, FALSE, info->allocCount);
}

AFX_ALLOC_HOOK AFXAPI_ALLOC_HOOK AfxSetAllocHook(AFX_ALLOC_HOOK fn)
{
   AFX_ALLOC_HOOK oldFn = _shi_mfcAllocHookFn;
   _shi_mfcAllocHookFn = fn;
   dbgMemSetEntryHandler(_shi_mfcAllocHook);
   return oldFn;
}
#ifndef _PORTABLE
static long NEAR _shi_mfcStopAllocNum = 0;
static AFX_ALLOC_HOOK NEAR _shi_mfcOldStopHook = NULL;
BOOL AFXAPI _shi_mfcAllocStop(size_t size, BOOL, long num)
{
   if (num == _shi_mfcStopAllocNum)
      dbgMemBreakpoint();

   return (*_shi_mfcOldStopHook)(size, FALSE, num);
}
void AFXAPI AfxSetAllocStop(long allocNum)
{
   if (_shi_mfcOldStopHook == NULL)
      _shi_mfcOldStopHook = AfxSetAllocHook(_shi_mfcAllocStop);
   _shi_mfcStopAllocNum = allocNum;
}
#endif

static void _shi_mfcNotImplemented(const char *feature, const char *alt)
{
#ifdef _WINDOWS
   char buf[200];
   sprintf(buf,
#else
   printf(
#endif
          "MFC API %.35s is not supported by SmartHeap."
          "\nSee SmartHeap API(s) %.65s.", feature, alt);
#ifdef _WINDOWS
   MessageBox(0, buf, "SmartHeap Library", MB_OK);
#endif
}

#if _MFC_VER >= 0x0200
CMemoryState::CMemoryState() { }
#endif
BOOL CMemoryState::Difference(const CMemoryState&, const CMemoryState&)
{
   _shi_mfcNotImplemented("CMemoryState::Difference",
       "dbgMemSetCheckpoint, dbgMemReportLeakage, and MemPoolWalk");
   return FALSE;
}
void CMemoryState::DumpStatistics() const
   { _shi_mfcNotImplemented("CMemoryState::DumpStatistics",
                            "dbgMemSetCheckpoint and MemPoolWalk"); }
void CMemoryState::Checkpoint()
   { _shi_mfcNotImplemented("CMemoryState::Checkpoint",
                            "dbgMemSetCheckpoint"); }
void CMemoryState::DumpAllObjectsSince() const
   { _shi_mfcNotImplemented("CMemoryState::DumpAllObjectsSince",
                            "dbgMemSetCheckpoint and MemPoolWalk"); }
void AFXAPI AfxDoForAllObjects(void (*)(CObject *, void *), void *)
   { _shi_mfcNotImplemented("AfxDoForAllObjects", "MemPoolWalk"); }

#if _MFC_VER >= 0x0400
void CMemoryState::UpdateData() { }
#endif /* MFC4 */

#if _MFC_VER >= 0x0300
void* AFXAPI_NEW AfxAllocMemoryDebug(size_t sz, BOOL /*isObject*/, LPCSTR file, int line)
{
   return shi_New(sz, file, line);
}
void AFXAPI_NEW AfxFreeMemoryDebug(void *p, BOOL /* isObject */)
{
   ::delete p;
}
BOOL AFXAPI AfxDumpMemoryLeaks()
{
   return dbgMemReportLeakage(NULL, 1, UINT_MAX);
}
void AFXAPI AfxStop()
{
   dbgMemBreakpoint();
}
#ifdef _AFXDLL
extern "C" int SmartHeap_malloc = 0;
#endif /* _AFXDLL */
#endif /* _MFC_VER >= 0x300 */
#endif /* MEM_DEBUG */

#if _MFC_VER >= 0x0400
int AFX_CDECL AfxNewHandler(size_t /* nSize */)
{
   AfxThrowMemoryException();
   return 0;
}

#pragma warning(disable: 4273)

_PNH AFXAPI AfxGetNewHandler(void)
{
   AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
   return pState->m_pfnNewHandler;
}

_PNH AFXAPI AfxSetNewHandler(_PNH pfnNewHandler)
{
   AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
   _PNH pfnOldHandler = pState->m_pfnNewHandler;
   pState->m_pfnNewHandler = pfnNewHandler;
   return pfnOldHandler;
}
#endif /* MFC4 */

#endif /* MFC */


// MSC and Borland C++ specific versions of operator new

#if defined(__BORLANDC__)
#if __BORLANDC__ >= 0x450 && UINT_MAX == 0xFFFFu
void far *operator new[](unsigned long sz)
{
   return shi_New(sz);
}
#endif  /* __BORLANDC__ >= 0x450 */

#if UINT_MAX == 0xFFFFu
#if defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__)
void far *operator new(unsigned long sz)
{
   return shi_New(sz);
}
#else
#error "SmartHeap operator new can be used only in large data models."
#endif /* __LARGE__ */
#endif /* UINT_MAX == 0xFFFFu */

#elif defined(_MSC_VER) && UINT_MAX == 0xFFFFu
#if defined(M_I86LM) || defined(M_I86CM) || defined(M_I86HM)
void __huge *operator new(unsigned long count, size_t sz)
{
   return (void __huge *)shi_New(count * sz, MEM_HUGE);
}
#else
#error "SmartHeap operator new can be used only in large data models."
#endif /* M_I86LM */
#endif /* compiler-specific */

#if !(/* defined(MEM_DEBUG) && */ defined(WIN32) && defined(_MSC_VER) \
   && defined(_M_IX86))
// these operator new variants are defined in shnewjmp.asm for Win32/VC++
void operator delete(void MEM_FAR *p) MEM_CPP_THROW
{
   if (p)
   {
#if defined(MFC) && defined(MEM_DEBUG)
      _shi_updateAfxMemDF();
#endif
      
#ifdef MEM_DEBUG
      // unfortunately, there is no way to pass file/line info to
      // operator delete with placement syntax;
      // use 2nd-best alternative: global state in HA library
      _dbgMemFreePtr1(p, MEM_DELETE, NULL, 0);
#else
      MemFreePtr(p);
#endif

#ifdef OWL
      if (SafetyPool::IsExhausted())
         SafetyPool::Allocate();
#endif
   }
   
#ifdef MEM_DEBUG
   _shi_deleteLoc(NULL, 0);
#endif
}
#endif


// new_handler: note that MSC/C++ prototype for new handler differs from the
// protosed ANSI standard prototype for the new handler
#ifdef _MSC_VER
#if UINT_MAX == 0xFFFFu
_PNH _set_fnew_handler(_PNH handler)
{
   return _set_new_handler(handler);
}

_PNHH _set_hnew_handler(_PNHH handler)
{
   _PNHH old_handler = mem_new_handler_msc_huge;
   mem_new_handler_msc_huge = handler;
   return old_handler;
}
#endif /* 16-bit */

#if !(defined(_MFC_VER) && _MFC_VER >= 0x0300 && defined(_AFXDLL))

#if _MSC_VER < 900
// no need to override _set_new_handler for VC++ 2.0 or higher, since
// we can obtain a pointer to the CRT new-handler with _query_new_handler
extern "C" _PNH mem_new_handler_msc;

_PNH _set_new_handler(_PNH handler)
{
   _PNH old_handler = mem_new_handler_msc;
   mem_new_handler_msc = handler;
   return old_handler;
}
#endif

#if _MSC_VER >= 900
#ifdef MEM_DEBUG
/* VC++ 4.0 debug new */
// these operator new variants are defined in shnewjmp.asm for Win32/VC++
//void * __cdecl operator new(size_t size, int, const char *file, int line);
//void * __cdecl operator new(size_t size, int, const char *file, int line)
//   { return shi_New(size, file, line); }
#endif /* MEM_DEBUG */
#endif /* MSC 9.0 */
#endif /* !_AFXDLL */
#endif /* _MSC_VER */


#if !defined(_MSC_VER) || _MSC_VER < 900
// no need to override _set_new_handler for VC++ 2.0 or higher, since
// we can obtain a pointer to the CRT new-handler with _query_new_handler
pnh set_new_handler(pnh handler) MEM_CPP_THROW
{
   pnh old_handler = _new_handler;
   _new_handler = handler;
   return old_handler;
}
#endif

#if defined(__IBMCPP__) && __IBMCPP__ >= 300
pnh _set_mt_new_handler(pnh handler)
{
	      return set_new_handler(handler);
}
#endif /* __IBMCPP__ */
