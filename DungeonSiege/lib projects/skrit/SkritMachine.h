//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritMachine.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the virtual machine implementation for Skrit.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITMACHINE_H
#define __SKRITMACHINE_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiSchemaImpl.h"
#include "GpColl.h"
#include "SkritDefs.h"
#include "StdHelp.h"

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class Machine declaration

// $ note: this stack operates opposite of win32 - it grows upward, though it is
//         still dword-aligned. to compensate, parameters are pushed left-to-
//         right so that it matches up with C calling conventions and is more
//         efficient.

class Machine
{
public:
	SET_NO_INHERITED( Machine );

	// ctor/dtor
	Machine( Object* object )  {  Init( object );  }
	Machine( ObjectImpl* impl )  {  Init( impl );  }
   ~Machine( void )  {  Shutdown();  }

	// setup
	void Init( Object* object );
	void Init( ObjectImpl* impl );
	bool IsInitialized( void ) const  {  return ( m_Pack != NULL );  }
	void Shutdown( void );

	// execution
	Result Call( const Raw::ExportEntry* function, const_mem_ptr params = const_mem_ptr() );
	Result CallFragment( DWORD codeOffset, const_mem_ptr params = const_mem_ptr() );

	// results
	Result GetLastResult( void ) const  {  return ( m_LastResult );  }

// Debugging.

#	if !GP_RETAIL

	// general ip-based query
	int      GetLine             ( const BYTE* ip, int* offset = NULL ) const;
	gpstring GetSourceFile       ( const BYTE* ip ) const;
	gpstring MakeLocationText    ( const BYTE* ip ) const;
	gpstring MakeLocationTextFull( const BYTE* ip ) const;

	// current code query
FEX	int         GetCurrentLine             ( void ) const;
	gpstring    GetCurrentSourceFile       ( void ) const;
FEX	const char* MakeCurrentSourceFile      ( void ) const;
FEX	const char* MakeCurrentLocationText    ( void ) const;
FEX	const char* MakeCurrentLocationTextFull( void ) const;

	// disassembling
	void Disassemble       ( const BYTE* ip, ReportSys::Context* ctx ) const;
FEX	void DisassembleCurrent( ReportSys::Context* ctx ) const;
FEX	void DisassembleCurrent( void ) const;

#	endif // !GP_RETAIL

#	if !GP_ERROR_FREE
	// special debug stack generation
	typedef stdx::fast_vector <gpstring> StringVec;
	static bool StackQueryProc( DWORD threadId, gpstring& stackName, StringVec& stack );
#	endif // !GP_ERROR_FREE

private:

// Private methods.

	// internal utility
	void   PrivateInit   ( ObjectImpl* impl );
	Result Execute       ( void );
	Result PrivateExecute( bool& done );
	void   Unwind        ( void );

#	if !GP_RETAIL
	void ReportException( const char* type, const SeException* sex = NULL, int paramCount = 0 ) const;
#	endif // !GP_RETAIL

	// bytestream access
	Op::Code GetNextOp    ( void )  {  return ( scast <Op::Code> ( *m_IP++ ) );  }
	Op::Code PeekNextOp   ( void )  {  return ( scast <Op::Code> ( *m_IP ) );  }
	BYTE     GetNextByte  ( void )  {  return ( *m_IP++ );  }
	BYTE     PeekNextByte ( void )  {  return ( *m_IP );  }
	char     GetNextChar  ( void )  {  char c = *rcast <const char*> ( m_IP );  ++m_IP;  return ( c );  }
	char     PeekNextChar ( void )  {  return ( *rcast <const char*> ( m_IP ) );  }
	WORD     GetNextWord  ( void )  {  WORD w = *rcast <const WORD*> ( m_IP );  m_IP += 2;  return ( w );  }
	WORD     PeekNextWord ( void )  {  return ( *rcast <const WORD*> ( m_IP ) );  }
	short    GetNextSword ( void )  {  short s = *rcast <const short*> ( m_IP );  m_IP += 2;  return ( s );  }
	short    PeekNextSword( void )  {  return ( *rcast <const short*> ( m_IP ) );  }
	DWORD    GetNextDword ( void )  {  DWORD d = *rcast <const DWORD*> ( m_IP );  m_IP += 4;  return ( d );  }
	DWORD    PeekNextDword( void )  {  return ( *rcast <const DWORD*> ( m_IP ) );  }

	// store access
	Store* GetNextStore( void );

// Private types.

	union StackEntry
	{
		DWORD       m_RawData;
		float       m_Float;
		int         m_Int;
		gpstring*   m_String;
		const char* m_CString;
		void*       m_Ptr;

		StackEntry( void )           {  }

		StackEntry( DWORD       d )  {  m_RawData = d;  }
		StackEntry( float       f )  {  m_Float   = f;  }
		StackEntry( int         i )  {  m_Int     = i;  }
		StackEntry( gpstring*   g )  {  m_String  = g;  }
		StackEntry( const char* s )  {  m_CString = s;  }
		StackEntry( void*       p )  {  m_Ptr     = p;  }
	};

	typedef stdx::public_stack <StackEntry, stdx::fast_vector <StackEntry> > Stack;

// Private data.

	// incoming
	const_mem_ptr    m_Params;					// params passed to function call
	const void*      m_CurrentParam;			// current param we're working on extracting

	// the stores
	Stack            m_Stack;					// expression/parameter stack
	Store            m_LocalStore;				// local variable stacks
	Store*           m_GlobalStore;				// global variable stacks local to this Object instance
	Store*           m_SharedStore;				// global variable stacks shared among all Object instances

	// execution state
	Object*          m_Object;					// owning object
	const Raw::Pack* m_Pack;					// object data for this skrit
	const BYTE*      m_IP;						// instruction pointer
	Result           m_LastResult;				// most recent result returned from last function call

	// special debug tracking info
#	if !GP_RETAIL
	const BYTE*      m_StartIP;					// instruction pointer of start of statement
#	endif // !GP_RETAIL

	SET_NO_COPYING( Machine );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

#endif  // __SKRITMACHINE_H

//////////////////////////////////////////////////////////////////////////////
