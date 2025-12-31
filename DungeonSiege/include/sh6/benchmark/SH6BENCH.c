/* sh6bench.c -- SmartHeap (tm) Portable memory management benchmark.
 *
 * Copyright (C) 2000 MicroQuill Software Publishing Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the copyright owner.
 *
 *
 * Compile-time flags.  Define the following flags on the compiler command-line
 * to include the selected APIs in the benchmark.  When testing an ANSI C
 * compiler, include MALLOC_ONLY flag to avoid any SmartHeap API calls.
 * Define these symbols with the macro definition syntax for your compiler,
 * e.g. -DMALLOC_ONLY=1 or -d MALLOC_ONLY=1
 *
 *  Flag                   Meaning
 *  -----------------------------------------------------------------------
 *  MALLOC_ONLY=1       Test ANSI malloc/realloc/free only
 *  INCLUDE_NEW=1       Test C++ new/delete
 *  INCLUDE_MOVEABLE=1  Test SmartHeap handle-based allocation API
 *  MIXED_ONLY=1        Test interdispersed alloc/realloc/free only
 *                      (no tests for alloc, realloc, free individually)
 *  SYS_MULTI_THREAD=1  Test with multiple threads (OS/2, NT, HP, Solaris only)
 *  SMARTHEAP=1         Required when compiling if linking with SmartHeap lib
 * 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_Windows) || defined(_WINDOWS) || defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#define WIN32 1
#if defined(SYS_MULTI_THREAD)
#include <process.h>
typedef HANDLE ThreadID;
#define THREAD_NULL (ThreadID)-1
#endif /* SYS_MULTI_THREAD */

#elif defined(UNIX) || defined(__hpux) || defined(_AIX) || defined(__sun) \
	|| defined(sgi) || defined(__DGUX__)
/* Unix prototypes */
#ifndef UNIX
#define UNIX 1
#endif
#include <unistd.h>
#ifdef SYS_MULTI_THREAD
#if defined(__hpux) || defined(__osf__) || defined(_AIX) || defined(sgi) \
	|| defined(__DGUX__)
#define _INCLUDE_POSIX_SOURCE
#include <sys/signal.h>
#include <pthread.h>
typedef pthread_t ThreadID;
#if defined(_DECTHREADS_) && !defined(__osf__)
ThreadID ThreadNULL = {0, 0, 0};
#define THREAD_NULL ThreadNULL
#define THREAD_EQ(a,b) pthread_equal(a,b)
#endif /* __hpux */
			int thread_specific;
#elif defined(__sun)
#include <thread.h>
typedef thread_t ThreadID;
#endif /* __sun */
#endif /* SYS_MULTI_THREAD */

#elif defined(__OS2__) || defined(__IBMC__)
#ifdef __cplusplus
extern "C" {
#endif
#include <process.h>
#define INCL_DOS
#define INCL_DOSPROCESS    /* thread control	*/
#define INCL_DOSMEMMGR     /* Memory Manager values */
#define INCL_DOSERRORS     /* Get memory error codes */
#include <os2.h>
#include <bsememf.h>      /* Get flags for memory management */
typedef TID ThreadID; 
#define THREAD_NULL (ThreadID)-1
#ifdef __cplusplus
}
#endif

#endif /* end of environment-specific header files */

#ifndef THREAD_NULL
#define THREAD_NULL 0
#endif
#ifndef THREAD_EQ
#define THREAD_EQ(a,b) ((a)==(b))
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Note: the SmartHeap header files must be included _after_ any files that
 * declare malloc et al
 */
#include "smrtheap.h"
#ifdef MALLOC_ONLY
#undef malloc
#undef realloc
#undef free
#endif

#ifdef INCLUDE_NEW
#include "smrtheap.hpp"
#endif

#if defined(SHANSI)
#include "shmalloc.h"
int SmartHeap_malloc = 0;
#endif

#ifdef SILENT
void fprintf_silent(FILE *, ...);
void fprintf_silent(FILE *x, ...) { (void)x; }
#else
#define fprintf_silent fprintf
#endif

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifdef CLK_TCK
#undef CLK_TCK
#endif
#define CLK_TCK CLOCKS_PER_SEC

#define TRUE 1
#define FALSE 0
typedef int Bool;

FILE *fout, *fin;
unsigned uMaxBlockSize = 1000;
unsigned uMinBlockSize = 1;
unsigned long ulCallCount = 1000;

unsigned long promptAndRead(char *msg, unsigned long defaultVal, char fmtCh);

#ifdef SYS_MULTI_THREAD
unsigned uThreadCount = 8;
ThreadID RunThread(void (*fn)(void *), void *arg);
void WaitForThreads(ThreadID[], unsigned);
int GetNumProcessors(void);
#else
unsigned uThreadCount = 1;
#endif

#ifdef HEAPALLOC_WRAPPER
#define TEST_HEAPALLOC 1
#endif

#ifdef TEST_HEAPALLOC
#ifdef malloc
#undef malloc
#undef realloc
#undef free
#endif

#define malloc(s) HeapAlloc(GetProcessHeap(), 0, s)
#define realloc(p, s) HeapReAlloc(GetProcessHeap(), 0, p, s)
#define free(p) HeapFree(GetProcessHeap(), 0, p)

#endif

void doBench(void *);

void main(int argc, char *argv[])
{
	clock_t startCPU;
	time_t startTime;
	double elapsedTime, cpuTime;

#ifdef SMARTHEAP
	MemRegisterTask();
#endif
	
	setbuf(stdout, NULL);  /* turn off buffering for output */

	if (argc > 1)
		fin = fopen(argv[1], "r");
	else
		fin = stdin;
	if (argc > 2)
		fout = fopen(argv[2], "w");
	else
		fout = stdout;		

	ulCallCount = promptAndRead("call count", ulCallCount, 'u');
	uMinBlockSize = (unsigned)promptAndRead("min block size",uMinBlockSize,'u');
	uMaxBlockSize = (unsigned)promptAndRead("max block size",uMaxBlockSize,'u');

#ifdef HEAPALLOC_WRAPPER
	LoadLibrary("shsmpsys.dll");
#endif
	
#ifdef SYS_MULTI_THREAD
	{
		unsigned i;
		void *threadArg = NULL;
		ThreadID *tids;

#ifdef WIN32
		unsigned uCPUs = promptAndRead("CPUs (0 for all)", 0, 'u');

		if (uCPUs)
		{
			DWORD m1, m2;

			if (GetProcessAffinityMask(GetCurrentProcess(), &m1, &m2))
			{
				i = 0;
				m1 = 1;

				/*
				 * iterate through process affinity mask m2, counting CPUs up to
				 * the limit specified in uCPUs
				 */
				do
					if (m2 & m1)
						i++;
				while ((m1 <<= 1) && i < uCPUs);

				/* clear any extra CPUs in affinity mask */
				do
					if (m2 & m1)
						m2 &= ~m1;
				while (m1 <<= 1);

				if (SetProcessAffinityMask(GetCurrentProcess(), m2))
					fprintf(fout,
							  "\nThreads in benchmark will run on max of %u CPUs", i);
			}
		}
#endif /* WIN32 */
	
		uThreadCount = (int)promptAndRead("threads", GetNumProcessors(), 'u');

		if (uThreadCount < 1)
			uThreadCount = 1;
		ulCallCount /= uThreadCount;
		if ((tids = malloc(sizeof(ThreadID) * uThreadCount)) != NULL)
		{
			startCPU = clock();
			startTime = time(NULL);
			for (i = 0;  i < uThreadCount;  i++)
				if (THREAD_EQ(tids[i] = RunThread(doBench, threadArg),THREAD_NULL))
				{
					fprintf(fout, "\nfailed to start thread #%d", i);
					break;
				}

			WaitForThreads(tids, uThreadCount);
			free(tids);
		}
		if (threadArg)
			free(threadArg);
	}
#else
	startCPU = clock();
	startTime = time(NULL);
	doBench(NULL);
#endif

	elapsedTime = difftime(time(NULL), startTime);
	cpuTime = (double)(clock()-startCPU) / (double)CLK_TCK;

	fprintf_silent(fout, "\n");
	fprintf(fout, "\nTotal elapsed time"
#ifdef SYS_MULTI_THREAD
			  " for %d threads"
#endif
			  ": %.2f (%.4f CPU)\n",
#ifdef SYS_MULTI_THREAD
			  uThreadCount,
#endif
			  elapsedTime, cpuTime);

	if (fin != stdin)
		fclose(fin);
	if (fout != stdout)
		fclose(fout);
}

void doBench(void *arg)
{ 
  char **memory = malloc(ulCallCount * sizeof(void *));
  int	size_base, size, iterations;
  int	repeat = ulCallCount;
  char **mp = memory;
  char **mpe = memory + ulCallCount;
  char **save_start = mpe;
  char **save_end = mpe;

  while (repeat--)  
  { 
    for (size_base = 1;
		 size_base < uMaxBlockSize;
		 size_base = size_base * 3 / 2 + 1)
    { 
      for (size = size_base; size; size /= 2)
      {
			/* allocate smaller blocks more often than large */
			iterations = 1;

			if (size < 10000)
				iterations = 10;

			if (size < 1000)
				iterations *= 5;

			if (size < 100)
				iterations *= 5;

			while (iterations--)
			{ 

				if (!memory || !(*mp ++ = (char *)malloc(size)))
				{
					printf("Out of memory\n");
					_exit (1);
				}

	  /* while allocating skip over that portion of the buffer that still
	     holds pointers from the previous cycle
           */
	  if (mp == save_start)
	    mp = save_end;

	  if (mp >= mpe)   /* if we've reached the end of the malloc buffer */
	  { mp = memory;
            
	    /* mark the next portion of the buffer */
	    save_start = save_end;  
	    if (save_start >= mpe)	save_start = mp;
	    save_end = save_start + (ulCallCount / 5);
	    if (save_end > mpe)		save_end = mpe;
            
	    /* free the bottom and top parts of the buffer.
	     * The bottom part is freed in the order of allocation.
	     * The top part is free in reverse order of allocation.
	     */
	    while (mp < save_start)
	      free (*mp ++);

		 mp = mpe;

	    while (mp > save_end)
	      free (*--mp);

	    mp = memory;
	  }
	}
      }
    }
  }
  /* free the residual allocations */
  mpe = mp;
  mp = memory;

  while (mp < mpe)
    free (*mp ++);

  free(memory);
}

unsigned long promptAndRead(char *msg, unsigned long defaultVal, char fmtCh)
{
	char *arg = NULL, *err;
	unsigned long result;
	{
		char buf[12];
		static char fmt[] = "\n%s [%lu]: ";
		fmt[7] = fmtCh;
		fprintf_silent(fout, fmt, msg, defaultVal);
		if (fgets(buf, 11, fin))
			arg = &buf[0];
	}
	if (arg && ((result = strtoul(arg, &err, 10)) != 0
					|| (*err == '\n' && arg != err)))
	{
		return result;
	}
	else
		return defaultVal;
}


/*** System-Specific Interfaces ***/

#ifdef SYS_MULTI_THREAD
ThreadID RunThread(void (*fn)(void *), void *arg)
{
	ThreadID result = THREAD_NULL;
	
#if defined(__OS2__) && (defined(__IBMC__) || defined(__IBMCPP__) || defined(__WATCOMC__))
	if ((result = _beginthread(fn, NULL, 8192, arg)) == THREAD_NULL)
		return THREAD_NULL;

#elif (defined(__OS2__) && defined(__BORLANDC__)) || defined(WIN32)
	if ((result = (ThreadID)_beginthread(fn, 8192, arg)) == THREAD_NULL)
		return THREAD_NULL;

#elif defined(__sun)
/*	thr_setconcurrency(2); */
	return thr_create(NULL, 0, (void *(*)(void *))fn, arg, THR_BOUND, NULL)==0;

#elif defined(_DECTHREADS_) && !defined(__osf__)
	if (pthread_create(&result, pthread_attr_default,
							 (pthread_startroutine_t)fn, arg) == -1)
		 return THREAD_NULL;

#elif defined(_POSIX_THREADS) || defined(_POSIX_REENTRANT_FUNCTIONS)
	pthread_attr_t attr;
	pthread_attr_init(&attr);
#ifdef _AIX
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif /* _AIX */
	if (pthread_create(&result, &attr, (void *(*)(void *))fn, arg) == -1)
		return THREAD_NULL;

#else
#error Unsupported threads package
#endif

	return result;
}

/* wait for all benchmark threads to terminate */
void WaitForThreads(ThreadID tids[], unsigned tidCnt)
{
#ifdef __OS2__
 	while (tidCnt--)
 		DosWaitThread(&tids[tidCnt], DCWW_WAIT);

#elif defined(WIN32)
	WaitForMultipleObjects(tidCnt, tids, TRUE, INFINITE);

#elif defined(__sun)
	int prio;
	thr_getprio(thr_self(), &prio);
	thr_setprio(thr_self(), max(0, prio-1));
	while (tidCnt--)
		thr_join(0, NULL, NULL);
	
#elif defined(_POSIX_THREADS) || defined(_POSIX_REENTRANT_FUNCTIONS)
	while (tidCnt--)
		pthread_join(tids[tidCnt], NULL);

#endif
}

#ifdef __hpux
#include <sys/pstat.h>
/*#include <sys/mpctl.h>*/
#elif defined(__DGUX__)
#include <sys/dg_sys_info.h>
#endif

/* return the number of processors present */
int GetNumProcessors()
{
#ifdef WIN32
	SYSTEM_INFO info;

	GetSystemInfo(&info);

	return info.dwNumberOfProcessors;

#elif defined(UNIX)

#ifdef __hpux
	struct pst_dynamic psd;
	if (pstat_getdynamic(&psd, sizeof(psd), 1, 0) != -1)
		return psd.psd_proc_cnt;
/*	shi_bSMP mpctl(MPC_GETNUMSPUS, 0, 0) > 1; */

#elif defined(sgi)
	return sysconf(_SC_NPROC_CONF);
#elif defined(__DGUX__)
	struct dg_sys_info_pm_info info;

	if (dg_sys_info((long *)&info, DG_SYS_INFO_PM_INFO_TYPE,
						 DG_SYS_INFO_PM_CURRENT_VERSION)
		 == 0)
		 return info.idle_vp_count;
	else
		return 0;

#elif defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(_SC_NPROCESSORS_CONF)
	return sysconf(_SC_NPROCESSORS_CONF);
#endif

#endif

	return 1;
}
#endif /* SYS_MULTI_THREAD */
