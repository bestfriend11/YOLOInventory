// shnewhnd.cpp -- SmartHeap(tm) C++ new handler
// Professional Memory Management Library
//
// Copyright (C) 1991-1996 Compuware Corporation.
// All Rights Reserved.
//
// No part of this source code may be copied, modified or reproduced
// in any form without retaining the above copyright notice.
// This source code, or source code derived from it, may not be redistributed
// without express written permission of the author.
//

#include "smrtheap.hpp"

#if !defined(_MSC_VER)
#error shmsvc.c is for the Microsoft Visual C++ version of SmartHeap only
#endif

#if _MSC_VER < 900
// for VC++ below 2.0, emulate VC++ 2.0 _query_new_handler/_query_new_mode

extern _PNH mem_new_handler_msc = 0;

_PNH MEM_ENTRY_ANSI _query_new_handler();
_PNH MEM_ENTRY_ANSI _query_new_handler()
{
    return mem_new_handler_msc;
}

int MEM_ENTRY_ANSI _query_new_mode();
int MEM_ENTRY_ANSI _query_new_mode()
{
    return FALSE;
}
#endif // _MSC_VER < 900

// returns non-zero if new_handler exists and returns non-zero
MEM_BOOL shi_call_new_handler_msc(size_t sz, MEM_BOOL isMalloc)
{
	if (!isMalloc || _query_new_mode())
	{
		_PNH handler = _query_new_handler();

		if (handler && handler(sz))
			return 1;
	}

	return 0;
}

/* for VC++ 2.0 compatibility (where _pnhHeap is defined in malloc.c) */
extern "C" _PNH _pnhHeap = NULL;
