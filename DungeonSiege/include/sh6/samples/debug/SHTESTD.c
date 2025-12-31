/* Simple test cases for debug version of SmartHeap */

#include <stdio.h>

/* Note: the SmartHeap header file must be included _after_ any files that
 * declare malloc et al
 */
#ifdef __cplusplus
#define DEFINE_NEW_MACRO
#include "smrtheap.hpp"
#else
#include "smrtheap.h"
#endif
#include "shmalloc.h"

#ifndef MEM_DEBUG
#error shtestd.c must be compiled with MEM_DEBUG defined
#endif

#if defined(_MSC_VER) && defined(_WINDOWS) && UINT_MAX == 0xFFFFu
/* MSC Quick Win library has a bug that prevents fmalloc from being
 * overriden (it references data that has been freed!)
 */
int SmartHeap_malloc = 0;
int SmartHeap_far_malloc = 0;
#endif

#if defined(SHANSI)
int SmartHeap_malloc = 0;
#endif

#if defined(applec) || defined(THINK_C)
#include <dialogs.h>
#include <fonts.h>
#include <menus.h>
#endif

#define TRUE 1
#define FALSE 0

int maxErrOut = 0;

int main()
{
   MEM_POOL pool;
   unsigned char *buf;
   int i;
   unsigned char c;

#if defined(applec) || defined(THINK_C)
	InitGraf((char *) &qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
#else
   setbuf(stdout, NULL);  /* turn off buffering for screen output */
   setbuf(stderr, NULL);  /* turn off buffering for screen output */
#endif

   dbgMemSetSafetyLevel(MEM_SAFETY_DEBUG);
   dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_PROMPT | DBGMEM_OUTPUT_CONSOLE
      | DBGMEM_OUTPUT_BEEP | DBGMEM_OUTPUT_FILE, "shtestd.out");

   pool = MemPoolInit(0);
   dbgMemPoolSetCheckFrequency(pool, 1);
   dbgMemPoolDeferFreeing(pool, TRUE);
   dbgMemSetCheckpoint(2);

  /* the following allocation is never freed (leakage) */
#ifdef __cplusplus
   buf = new unsigned char[3];
#else
   buf = (unsigned char *)MemAllocPtr(pool, 3, 0);
#endif

   /* invalid buffer */
   MemPoolInfo(pool, NULL, NULL);

   /* invalid pointer parameter */
   MemFreePtr((void *)ULONG_MAX);

   /* underwrite */
   c = buf[-1];
   buf[-1] = 'x';
   MemCheckPtr(pool, buf);
   buf[-1] = c;

   /* overwrite */
   buf = (unsigned char *)MemAllocPtr(pool, 3, 0);  /* more leakage */
   c = buf[3];
   buf[3] = 'z';
   MemReAllocPtr(buf, 1, 0);
   buf[3] = c;

   dbgMemSetCheckpoint(3);
   
   /* write into read-only block */
   buf = (unsigned char *)MemAllocPtr(pool, 10, MEM_ZEROINIT);  /* more leakage */
   *buf = 'a';
   MemCheckPtr(pool, buf);
   dbgMemProtectPtr(buf, DBGMEM_PTR_READONLY);
   *buf = 'b';
   MemCheckPtr(pool, buf);
   *buf = 'a';
   dbgMemProtectPtr(buf, DBGMEM_PTR_NOFREE | DBGMEM_PTR_NOREALLOC);
   free(buf);
   realloc(buf, 44);

   /* double free */
   buf = (unsigned char *)malloc(1);
   dbgMemPoolDeferFreeing(MemDefaultPool, TRUE);
   dbgMemPoolSetCheckFrequency(MemDefaultPool, 1);
   for (i = 0;  i < 3;  i++)
      free(buf);

   /* write into free block */
   c = *buf;
   *buf = 'a';
   calloc(1, 3);
   *buf = c;

   dbgMemReportLeakage(NULL, 2, 3);

#if !defined(applec) && !defined(THINK_C)
   printf("\nMaximum error chars formatted: %d.", maxErrOut);
#endif

   return 1;
}
