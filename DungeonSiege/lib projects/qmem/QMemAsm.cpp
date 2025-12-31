//////////////////////////////////////////////////////////////////////////////
//
// File     :  QMemAsm.cpp
// Author(s):  Scott Bilas
//
// Summary  :  This is a special inline file meant to be #included multiple
//             times by QMem.cpp ONLY. Use to define multiple versions of the
//             same function easily to keep things in sync.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if !defined( QMEMASM_HOOK ) && !defined( QMEMASM_INTERNAL )
#error ! Do NOT include this file! It's meant for KernelTool.cpp only!
#endif

//////////////////////////////////////////////////////////////////////////////

#if QMEMASM_HOOK
extern "C" _declspec ( naked ) void __cdecl _penter( void )
#endif // QMEMASM_HOOK
#if QMEMASM_INTERNAL
_declspec ( naked ) DWORD SampleAndAdvance( void )
#endif // QMEMASM_INTERNAL
{
	__asm
	{
		// $$ this code can easily be optimized. i wrote it to get it working,
		//    and if perf is an issue, it can be fixed up for pipelining, better
		//    org of register usage, etc...

		// $$ if using this for cpu profiling, to get more accurate profiles it
		//    would be worth adding code to profile the overhead of _penter and
		//    subtract those cycles from the sampled data.

#		define PUSH_SIZE 0x20

		pushad													// docs for _penter say preserve anything we change

		// see if we have any storage
	retry:
#		if QMEMASM_INTERNAL
		mov			dword ptr [esp+PUSH_SIZE-4], 0xFFFFFFFF		// pre-fill with invalid index value in case we're not sampling
#		endif // QMEMASM_INTERNAL
		mov			edx, dword ptr [s_LogStorageMaxIndex]
		test		edx, edx
		jz			done										// if max index is zero then it's disabled

		// get index to use
		mov			eax, 1
		xadd		dword ptr [s_LogStorageNextIndex], eax		// inc next index
		cmp			eax, edx									// are we out of room?
		jge			storage_full								// yeah abort

		// find log entry
#		if QMEMASM_INTERNAL
		mov			dword ptr [esp+PUSH_SIZE-4], eax			// update what will get popped into eax so that will be the return value
#		endif // QMEMASM_INTERNAL
		imul		eax, SAMPLE_SIZE							// index -> memory offset
		mov			edi, dword ptr [s_LogStorageBase]			// get base
		add			edi, eax									// edi points to our log entry

		// add log entry
		mov			edx, dword ptr [esp+PUSH_SIZE]				// get return address offset from regs we pushed
		mov			eax, dword ptr [esp+PUSH_SIZE+4]			// get caller's return address
		mov			dword ptr [edi], edx						// set m_Eip
		mov			dword ptr [edi+4], eax						// set m_CallerEip
		mov			dword ptr [edi+8], esp						// set m_Esp

		// optionally sample cpu cycles
		mov			eax, dword ptr [s_LogCpuCycles]				// get flag
		test		eax, eax									// logging cpu?
		jnz			sample_cpu									// yeah...

		// done
	done:
		popad
		ret

		// sample cpu clocks
	sample_cpu:
		rdtsc													// get cycles
		mov			dword ptr [edi+12], eax						// set low order bits
		mov			dword ptr [edi+16], edx						// set high order bits
		popad
		ret

		// flush and try again
	storage_full:
		jnz			storage_full_wait
		push		0											// -> fullFlush
		push		edx											// -> maxIndex
		call		LogFlush									// the thread that got exactly to the end gets to do the flush
		jmp			retry										// do it again

		// this is where other threads pile up
	storage_full_wait:
		mov			edx, dword ptr [s_LogStorageMaxIndex]
		mov			eax, dword ptr [s_LogStorageNextIndex]
		cmp			eax, edx
		jle			retry										// if current is <= than max then flush has completed
		push		0
		call		dword ptr Sleep								// yield thread's cpu
		jmp			storage_full_wait
	}
}

#undef PUSH_SIZE

//////////////////////////////////////////////////////////////////////////////
