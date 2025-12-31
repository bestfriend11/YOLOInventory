//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritMachine.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "SkritMachine.h"

#include "DebugHelp.h"
#include "FileSysXfer.h"
#include "GpMath.h"
#include "ReportSys.h"
#include "SkritByteCode.h"
#include "SkritEngine.h"
#include "SkritStructure.h"
#include "SkritObject.h"
#include "StringTool.h"

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class Machine implementation

#if !GP_ERROR_FREE
DECL_THREAD Machine* g_MachineStack[ 100 ];		// all currently executing VM's
DECL_THREAD int      g_MachineStackTop;			// top of stack
struct ThreadIdHack
{
	DWORD     m_ThreadId;
	Machine** m_MachineStack;
	int*      m_MachineStackTop;
};
static ThreadIdHack g_ThreadIdHacks[ 50 ];		// use this to match thread id's to machine stacks (DAMMIT NO HANDLE-TO-TID FUNCTION!!) -sb
#endif // !GP_ERROR_FREE

void Machine :: Init( Object* object )
{
	m_Object = object;
	m_GlobalStore = object->GetGlobalStore();
	PrivateInit( object->GetImpl() );
}

void Machine :: Init( ObjectImpl* impl )
{
	m_Object = NULL;
	m_GlobalStore = NULL;
	PrivateInit( impl );
}

void Machine :: PrivateInit( ObjectImpl* impl )
{
	// parameter validation
	gpassert( impl != NULL );
	gpassert( impl->GetPack() != NULL );

	// initialize
	m_CurrentParam = NULL;
	m_SharedStore  = impl->GetGlobalStore();
	m_Pack         = impl->GetPack();
	m_IP           = NULL;
	m_LastResult   = RESULT_SUCCESS;

#	if !GP_RETAIL
	m_StartIP = NULL;
#	endif // !GP_RETAIL
}

void Machine :: Shutdown( void )
{
	gpassert( m_IP == NULL );		// do not delete "this" while executing!
	gpassert( m_StartIP == NULL );
	Unwind();

	m_Pack = NULL;
}

Result Machine :: Call( const Raw::ExportEntry* function, const_mem_ptr params )
{
	// verify
	gpassert( function != NULL );
	gpassert( (function->m_ParamCount * 4) == scast <int> ( params.size ) );
	gpassert( (function->m_ParamCount == 0) || (params.mem != NULL) );

	// run
	return ( CallFragment( function->m_FunctionOffset, params ) );
}

Result Machine :: CallFragment( DWORD codeOffset, const_mem_ptr params )
{
	// verify range
	gpassert( codeOffset < m_Pack->m_ByteCode.size() );

	// setup
	m_Params       = params;
	m_CurrentParam = params.mem;
	m_IP           = &*m_Pack->m_ByteCode.begin() + codeOffset;
	GPDEV_ONLY( m_StartIP = m_IP );

	// execute
	m_LastResult = Execute();

	// cleanup
	m_Params       = const_mem_ptr();
	m_CurrentParam = NULL;
	m_IP           = NULL;
	GPDEV_ONLY( m_StartIP = NULL );
	Unwind();

	// done
	return ( m_LastResult );
}

#if !GP_RETAIL

int Machine :: GetLine( const BYTE* ip, int* offset ) const
{
	int line = 0;
	int localOffset = 0;
	if ( offset == NULL )
	{
		offset = &localOffset;
	}
	*offset = 0;

	// valid ip and debug db?
	const Raw::DebugDb* debugDb = m_Object->GetImpl()->GetDebugDb();
	if ( (ip != NULL) && (debugDb != NULL) )
	{
		int absOffset = ip - &*m_Pack->m_ByteCode.begin();

		const Raw::DebugLocation* debugLoc = debugDb->GetLocationFromIp( absOffset );
		if ( debugLoc != NULL )
		{
			line = debugLoc->m_Line;
			*offset = absOffset - debugLoc->m_CodeOffset;
		}
	}

	// done
	return ( line );
}

gpstring Machine :: GetSourceFile( const BYTE* ip ) const
{
	gpstring name;

	// valid ip and debug db?
	const Raw::DebugDb* debugDb = m_Object->GetImpl()->GetDebugDb();
	if ( (ip != NULL) && (debugDb != NULL) )
	{
		const Raw::DebugLocation* debugLoc = debugDb->GetLocationFromIp( ip - &*m_Pack->m_ByteCode.begin() );
		if ( debugLoc != NULL )
		{
			name = debugDb->GetValidSourceFileName( debugLoc->m_SourceIndex );
		}
	}

	// done
	return ( name );
}

gpstring Machine :: MakeLocationText( const BYTE* ip ) const
{
	int line = GetLine( ip );
	gpstring name = GetSourceFile( ip );
	int offset = ip - &*m_Pack->m_ByteCode.begin();

	if ( line != 0 )
	{
		return ( gpstringf( "'%s' (line = %d, ip = 0x%08X)", name.c_str(), line, offset ) );
	}
	else if ( ip != NULL )
	{
		return ( gpstringf( "'%s' (ip = 0x%08X)", m_Object->GetName().c_str(), offset ) );
	}
	else
	{
		return ( gpstringf( "'%s' (NULL/INACTIVE)", m_Object->GetName().c_str() ) );
	}
}

gpstring Machine :: MakeLocationTextFull( const BYTE* ip ) const
{
	gpstring str;
	gpstring objName( FileSys::GetFileNameOnly( m_Object->GetName() ) );
	gpstring sourceFile( FileSys::GetFileNameOnly( GetSourceFile( ip ) ) );

	str.appendf( "0x%08X %s", ip - &*m_Pack->m_ByteCode.begin(), objName.c_str() );

	if ( !sourceFile.same_no_case( objName ) )
	{
		str += ":";
		str += sourceFile;
	}

	int offset = 0;
	int line = GetLine( ip, &offset );
	str.appendf( ", Line %d + %d bytes (vm = 0x%08X, obj = 0x%08X)", line, offset, this, m_Object );

	return ( str );
}

int Machine :: GetCurrentLine( void ) const
{
	return ( GetLine( m_IP ) );
}

gpstring Machine :: GetCurrentSourceFile( void ) const
{
	return ( GetSourceFile( m_IP ) );
}

const char* Machine :: MakeCurrentSourceFile( void ) const
{
	static gpstring s_String;
	s_String = GetSourceFile( m_IP );
	return ( s_String );
}

const char* Machine :: MakeCurrentLocationText( void ) const
{
	static gpstring s_String;
	s_String = MakeLocationText( m_IP );
	return ( s_String );
}

const char* Machine :: MakeCurrentLocationTextFull( void ) const
{
	static gpstring s_String;
	s_String = MakeLocationTextFull( m_IP );
	return ( s_String );
}

void Machine :: Disassemble( const BYTE* ip, ReportSys::Context* ctx ) const
{
	static const int BACK_UP_BYTES = 20;
	static const int DISASM_BYTES  = 35;

	int offset = ip - &*m_Pack->m_ByteCode.begin();

	// disasm from here
	m_Pack->DisassembleMixed( ctx, NULL, max( offset - BACK_UP_BYTES, 0 ), DISASM_BYTES, offset );
}

void Machine :: DisassembleCurrent( ReportSys::Context* ctx ) const
{
	Disassemble( m_IP, ctx );
}

void Machine :: DisassembleCurrent( void ) const
{
	ReportSys::AutoReport autoReport( &gGenericContext );
	gpgenericf(( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				  "Disassembling %s:\n\n", MakeCurrentLocationText() ));
	DisassembleCurrent( &gGenericContext );
	gpgeneric( "\n" );
}

#endif // !GP_RETAIL

#if !GP_ERROR_FREE

bool Machine :: StackQueryProc( DWORD threadId, gpstring& stackName, StringVec& stack )
{
	stackName = "Skrit VM's";

	ThreadIdHack* i, * ibegin = g_ThreadIdHacks, * iend = ARRAY_END( g_ThreadIdHacks );
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_ThreadId == threadId )
		{
			Machine** j, ** jbegin = i->m_MachineStack, ** jend = jbegin + *i->m_MachineStackTop;
			for ( j = jbegin ; j != jend ; ++j )
			{
#				if GP_RETAIL
				stack.push_back( (*j)->m_Object->GetName() );
#				else // GP_RETAIL
				stack.push_back( (*j)->MakeLocationTextFull( (*j)->m_StartIP ) );
#				endif // GP_RETAIL
			}

			return ( true );
		}
	}

	// didn't find a match for this thread
	return ( false );
}

#endif // !GP_ERROR_FREE

// these are helper macros for the bytecode interpreter. they're a little ugly
// and impossible to debug but i don't care. they're easier for me, so there.

// Math.

#	define MATH_X( OPCODE, OP, TYPE, NAME ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			TYPE r = m_Stack.top().NAME;  m_Stack.pop(); \
			m_Stack.top().NAME OP##= r; \
		}	break;
#	define MATH_I( OPCODE, OP )  MATH_X( OPCODE, OP, int, m_Int )
#	define MATH_F( OPCODE, OP )  MATH_X( OPCODE, OP, float, m_Float )
#	define MATH( OPCODE, OP )    MATH_I( OPCODE##I, OP );  MATH_F( OPCODE##F, OP )

// Unary.

#	define UNARY_X( OPCODE, OP, TYPE, NAME ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			TYPE& l = m_Stack.top().NAME; \
			l = OP l; \
		}	break;
#	define UNARY_I( OPCODE, OP )  UNARY_X( OPCODE, OP, int, m_Int )
#	define UNARY_F( OPCODE, OP )  UNARY_X( OPCODE, OP, float, m_Float )
#	define UNARY( OPCODE, OP )    UNARY_I( OPCODE##I, OP );  UNARY_F( OPCODE##F, OP )

// Compare.

#	define COMPARE_I( OPCODE, OP ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			int r = m_Stack.top().m_Int;  m_Stack.pop(); \
			int& l = m_Stack.top().m_Int; l = l OP r; \
		}	break;
#	define COMPARE_F( OPCODE, OP ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			float r = m_Stack.top().m_Float;  m_Stack.pop(); \
			m_Stack.top().m_Int = m_Stack.top().m_Float OP r; \
		}	break
#	define COMPARE_S( OPCODE, OP ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			gpstring* r = m_Stack.top().m_String;  m_Stack.pop(); \
			m_Stack.top().m_Int = m_Stack.top().m_String->compare_no_case( *r ) OP 0; \
		}	break;
#	define COMPARE_C( OPCODE, OP ) \
		case ( Op::CODE_##OPCODE ): \
		{ \
			const char* r = m_Stack.top().m_CString;  m_Stack.pop(); \
			m_Stack.top().m_Int = ::stricmp( m_Stack.top().m_CString, r ) OP 0; \
		}	break;
#	define COMPARE( OPCODE, OP ) COMPARE_I( OPCODE##I, OP ); COMPARE_F( OPCODE##F, OP ); COMPARE_I( OPCODE##B, OP )
#	define COMPARES( OPCODE, OP ) COMPARE( OPCODE, OP ); COMPARE_S( OPCODE##S, OP ); COMPARE_C( OPCODE##C, OP )

Result Machine :: Execute( void )
{
	bool done = false, reported = false;
	Result rc;

	GPDEBUG_ONLY( gFuBiSysExports.PushNestDetectCancel() );

#	if !GP_ERROR_FREE
	g_MachineStack[ g_MachineStackTop++ ] = this;
	gpassert( g_MachineStackTop != ELEMENT_COUNT( g_MachineStack ) );
	{
		static kerneltool::Critical s_Critical;
		kerneltool::Critical::Lock locker( s_Critical );

		DWORD threadId = ::GetCurrentThreadId();
		for ( ThreadIdHack* i = g_ThreadIdHacks ; i != ARRAY_END( g_ThreadIdHacks ) ; ++i )
		{
			if ( i->m_ThreadId == 0 )
			{
				i->m_ThreadId        = threadId;
				i->m_MachineStack    = g_MachineStack;
				i->m_MachineStackTop = &g_MachineStackTop;
			}
			else if ( i->m_ThreadId == threadId )
			{
				break;
			}
		}
	}
#	endif // !GP_ERROR_FREE

#	if !GP_RETAIL
	SeException* sex = new SeException;
#	endif // !GP_RETAIL

	__try
	{
		rc = PrivateExecute( done );
	}
	__except( ::GlobalExceptionFilter(
				  GetExceptionInformation()
				, EXCEPTION_CONTINUE_SEARCH
				, EXCEPTION_EXECUTE_HANDLER
				, false
#			if !GP_RETAIL
				, sex
#			endif // !GP_RETAIL
			) )
	{
#		if !GP_RETAIL
		ReportException( "a fatal SEH exception occurred!", sex );
		reported = true;
#		endif // !GP_RETAIL
	}

#	if !GP_RETAIL
	Delete( sex );
	if ( !done && !reported )
	{
		gperrorf(( "Unexpected function return from '%s'!\n", m_Object->GetName().c_str() ));
	}
#	endif // !GP_RETAIL

#	if !GP_ERROR_FREE
	gpassert( g_MachineStackTop > 0 );
	gpassert( g_MachineStack[ g_MachineStackTop - 1 ] == this );
	--g_MachineStackTop;
#	endif // !GP_ERROR_FREE

	GPDEBUG_ONLY( gFuBiSysExports.PopNestDetectCancel() );

	return ( rc );
}

Result Machine :: PrivateExecute( bool& done )
{
	Result rc;

	while ( !done )
	{
		// remember for debug help
		GPDEV_ONLY( m_StartIP = m_IP );

#		if !GP_RETAIL
		ReportSys::AutoIndent autoIndent( &gSkritContext, false );
		if ( m_Object->GetTraceOpCodes() )
		{
			// $$ future: enhance this with real-time (non-const) data. for
			//    example, when a value is pushed, added, etc., print out the
			//    parameters and results.

			gpskritf(( "%s OP: ", MakeLocationTextFull( m_StartIP ).c_str() ));
			m_Pack->GetByteCode()->GenerateNextDocs( &gSkritContext, m_Pack->m_ByteCode.size(), m_IP, m_Object->GetImpl()->GetDebugDb() );
			autoIndent.Indent();
		}
#		endif // !GP_RETAIL

		Op::Code code = GetNextOp();

		switch ( code )
		{

		// General.

			case ( Op::CODE_SITNSPIN ):
			{
				// do nothing
			}	break;

			case ( Op::CODE_ABORT ):
			{
				rc = RESULT_FATAL;
				done = true;
			}	break;

		// Flow control.

			case ( Op::CODE_RETURN0 ):
			{
				gpassert( m_Stack.empty() );
				rc = RESULT_SUCCESS;
				done = true;
			}	break;

			case ( Op::CODE_RETURN ):
			{
				gpassert( m_Stack.size() == 1 );
				DWORD top = m_Stack.top().m_RawData;
				m_Stack.pop();
				rc = Result( RESULT_SUCCESS, top );
				done = true;
			}	break;

			case ( Op::CODE_CALL ):
			case ( Op::CODE_CALLSINGLETON ):
			case ( Op::CODE_CALLMEMBER ):
			case ( Op::CODE_CALLSINGLETONBASE ):
			case ( Op::CODE_CALLMEMBERBASE ):
			{
				// $$$ this opcode needs to be optimized:
				//
				//     move work into compiler so less done here
				//
				//     the insert() on a return of handle type is inefficient
				//     considering that a memcpy's going to happen anyway.
				//     have the blindcall functions be able to insert the
				//     RC pointer before their own memcpy call.
				//
				//     possible solution: have a pair of DWORDs owned by
				//     this instance that remembers the return code and the
				//     ST0.

				// $$$ set/reset error codes, error out...

				// $$$ have the caller ALWAYS push its total args on the stack,
				//     easier than having this guy calc it and only when it's
				//     not a vararg call.

				// lookup function, verify it will work
				const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( GetNextWord() );
				gpassert( spec != NULL );
				gpassert( spec->m_Flags & FuBi::FunctionSpec::FLAG_CAN_SKRIT );

				// test for vararg, get sizes
				bool vararg = false;
				size_t sz, count;
				if ( spec->m_Flags & FuBi::FunctionSpec::FLAG_VARARG )
				{
					count = m_Stack.top().m_Int;
					m_Stack.pop();
					sz = count * 4;
					vararg = true;
				}
				else
				{
					sz = spec->m_ParamSizeBytes;
					count = sz / 4;
				}

				// set up return - UDT's have the address of return passed first
				DWORD rc = 0, handle = 0;
				bool handleReturn = false;
				if ( spec->m_ReturnType.m_Flags & FuBi::TypeSpec::FLAG_HANDLE )
				{
					handleReturn = true;
					Stack::iterator pos = m_Stack.get_container().end() - count;
					m_Stack.get_container().insert( pos )->m_Ptr = &handle;
					++count;
					sz += 4;
				}

				// set up params for blind call, make sure it's valid
				void* args = &*(m_Stack.get_container().end() - count);
				gpassert( m_Stack.size() >= count );

				// do it
				void* object = NULL;
				if ( Op::GetTraits( code ) & Op::TRAIT_MEMBERCALL )
				{
					// get object if member call
					if ( (code == Op::CODE_CALLMEMBER) || (code == Op::CODE_CALLMEMBERBASE) )
					{
						// object is beneath all the params
						object = m_Stack.top( count ).m_Ptr;
						++count;
					}
					else
					{
						// verify this will work
						gpassert( (code == Op::CODE_CALLSINGLETON) || (code == Op::CODE_CALLSINGLETONBASE) );
						gpassert( spec->m_Parent != NULL );
						gpassert( spec->m_Parent->m_GetClassSingletonProc != NULL );

						// find the singleton
						object = (*(spec->m_Parent->m_GetClassSingletonProc))();
					}

					// optionally get the base offset and adjust
					DWORD baseOffset = 0;
					if ( Op::GetTraits( code ) & Op::TRAIT_BASEOFFSET )
					{
						baseOffset = GetNextWord();
					}

					// check for validity
					if ( (object != NULL) || (spec->m_Parent->m_Flags & FuBi::ClassSpec::FLAG_POINTERCLASS) )
					{
						// add offset
						object = rcast <BYTE*> ( object ) + baseOffset;

						// out
#						if !GP_RETAIL
						ReportSys::AutoIndent autoIndent( &gSkritContext, false );
						if ( m_Object->GetTraceSysCalls() )
						{
							gpskritf(( "%s SYSCALL: %s\n",
									   MakeLocationTextFull( m_StartIP ).c_str(),
									   spec->BuildQualifiedNameAndParams( const_mem_ptr( args, 0 ), object, false ).c_str() ));
							autoIndent.Indent();
						}
#						endif // !GP_RETAIL

						// call the function
						if ( vararg )
						{
							rc = FuBi::BlindCallFunction_ThisCallVarArg( args, sz, object, spec->m_FunctionPtr );
						}
						else
						{
							gpassert( spec->m_Flags & FuBi::FunctionSpec::FLAG_CALL_THISCALL );
							rc = FuBi::BlindCallFunction_ThisCall( args, sz, object, spec->m_FunctionPtr );
						}
					}
					else
					{
						int paramCount = count;
						if ( (code == Op::CODE_CALLMEMBER) || (code == Op::CODE_CALLMEMBERBASE) )
						{
							// dec to adjust for the bottom param being the member (don't care about that)
							--paramCount;
						}
#						if !GP_RETAIL
						ReportException( gpstringf( "dereferencing NULL on a %s call!",
										 (code == Op::CODE_CALLMEMBER) ? "member function" : "function" ), NULL, paramCount );
#						endif // !GP_RETAIL
					}
				}
				else
				{
					// unsupported calling convention
					gpassert( !(spec->m_Flags & FuBi::FunctionSpec::FLAG_CALL_FASTCALL) );
					// not possible to get this type if it's global
					gpassert( !(spec->m_Flags & FuBi::FunctionSpec::FLAG_CALL_THISCALL) );

					// out
#					if !GP_RETAIL
					ReportSys::AutoIndent autoIndent( &gSkritContext, false );
					if ( m_Object->GetTraceSysCalls() )
					{
						gpskritf(( "%s SYSCALL: %s\n",
								   MakeLocationTextFull( m_StartIP ).c_str(),
								   spec->BuildQualifiedNameAndParams( const_mem_ptr( args, 0 ), object, false ).c_str() ));
						autoIndent.Indent();
					}
#					endif // !GP_RETAIL

					// do call
					if ( spec->m_Flags & FuBi::FunctionSpec::FLAG_CALL_CDECL )
					{
						rc = FuBi::BlindCallFunction_CdeclCall( args, sz, spec->m_FunctionPtr );
					}
					else
					{
						rc = FuBi::BlindCallFunction_StdCall( args, sz, spec->m_FunctionPtr );
					}
				}

				// pop parameters off the stack
				m_Stack.pop( count );

				// push return value back on the stack
				if ( spec->m_ReturnType.m_Type != FuBi::VAR_VOID )
				{
					// adjust rc
					if ( spec->m_ReturnType.m_Type == FuBi::VAR_FLOAT )
					{
						// floats returned in FPU's ST0
						rc = FuBi::GetST0();
					}
					else if ( handleReturn )
					{
						rc = handle;
					}
					m_Stack.push( rc );
				}

			}	break;

			case ( Op::CODE_CALLSKRIT ):
			{
				// get call params
				int index = GetNextWord();
				const Raw::ExportEntry* spec = m_Object->GetImpl()->GetFunctionSpec( index );
				size_t count = spec->m_ParamCount;
				void* args = &*(m_Stack.get_container().end() - count);
				gpassert( m_Stack.size() >= count );

				// out
#				if !GP_RETAIL
				ReportSys::AutoIndent autoIndent( &gSkritContext, false );
				if ( m_Object->GetTraceSkritCalls() )
				{
					gpskritf(( "%s SKRITCALL: %s\n",
							   MakeLocationTextFull( m_StartIP ).c_str(),
							   spec->BuildNameAndParams( args ).c_str() ));
					autoIndent.Indent();
				}
#				endif // !GP_RETAIL

				// call it
				const_mem_ptr params( args, count * 4 );
				Result rc = m_Object->Call( index, params );
				m_Stack.pop( count );

				// push return value on the stack if there is one
				if ( spec->m_ReturnType != FuBi::VAR_VOID )
				{
					m_Stack.push( rc.m_Raw );
				}
			}	break;

			case ( Op::CODE_BRANCH ):
			{
				m_IP += GetNextSword();
			}	break;

			case ( Op::CODE_BRANCHZERO ):
			{
				short offset = GetNextSword();
				if ( !m_Stack.top().m_Int )
				{
					m_IP += offset;
				}
				m_Stack.pop();
			}	break;

		// Arithmetic.

			MATH  ( ADD,      + );
			MATH  ( SUBTRACT, - );
			MATH  ( MULTIPLY, * );
			MATH  ( DIVIDE,   / );
			MATH_I( MODULOI,  % );
			UNARY ( NEGATE,   - );

			case ( Op::CODE_MODULOF ):
			{
				float r = m_Stack.top().m_Float;  m_Stack.pop();
				float l = m_Stack.top().m_Float;
				m_Stack.top().m_Float = ::fmodf( l, r );
			}	break;

			case ( Op::CODE_POWERI ):
			{
				int r = m_Stack.top().m_Int;  m_Stack.pop();
				int l = m_Stack.top().m_Int;
				m_Stack.top().m_Int = FTOL( ::pow( l, r ) );
			}	break;

			case ( Op::CODE_POWERF ):
			{
				float r = m_Stack.top().m_Float;  m_Stack.pop();
				float l = m_Stack.top().m_Float;
				m_Stack.top().m_Float = (float)::pow( l, r );
			}	break;

			case ( Op::CODE_ADDSS ):
			{
				gpstring* dst = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* r   = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* l   = m_Stack.top().m_String;
				m_Stack.top().m_String = dst;
				*dst = *l;
				*dst += *r;
			}	break;

			case ( Op::CODE_ADDSC ):
			{
				gpstring* dst = m_Stack.top().m_String;   m_Stack.pop();
				const char* r = m_Stack.top().m_CString;  m_Stack.pop();
				gpstring* l   = m_Stack.top().m_String;
				m_Stack.top().m_String = dst;
				*dst = *l;
				*dst += r;
			}	break;

			case ( Op::CODE_ADDCS ):
			{
				gpstring* dst = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* r   = m_Stack.top().m_String;  m_Stack.pop();
				const char* l = m_Stack.top().m_CString;
				m_Stack.top().m_String = dst;
				*dst = l;
				*dst += *r;
			}	break;

			case ( Op::CODE_ADDCC ):
			{
				gpstring* dst = m_Stack.top().m_String;   m_Stack.pop();
				const char* r = m_Stack.top().m_CString;  m_Stack.pop();
				const char* l = m_Stack.top().m_CString;
				m_Stack.top().m_String = dst;
				*dst = l;
				*dst += r;
			}	break;

		// Logic.

			COMPARE_I( ANDIB, && );
			COMPARE_I( ORIB,  || );
			UNARY_I  ( NOTIB, !  );

			MATH_I ( BANDI, & );
			MATH_I ( BXORI, ^ );
			MATH_I ( BORI,  | );
			UNARY_I( BNOTI, ~ );

		// Comparisons.

			// bool/float/int ops
			COMPARE ( ISEQUAL,        == );
			COMPARE ( ISNOTEQUAL,     != );
			COMPARES( ISGREATER,      >  );
			COMPARES( ISLESS,         <  );
			COMPARES( ISGREATEREQUAL, >= );
			COMPARES( ISLESSEQUAL,    <= );

			// other

			case ( Op::CODE_ISEQUALS ):
			{
				gpstring* r = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* l = m_Stack.top().m_String;
				m_Stack.top().m_Int = l->same_with_case( *r );
			}	break;

			case ( Op::CODE_ISEQUALC ):
			{
				const char* r = m_Stack.top().m_CString;  m_Stack.pop();
				const char* l = m_Stack.top().m_CString;
				m_Stack.top().m_Int = ::strcmp( l, r ) == 0;
			}	break;

			case ( Op::CODE_ISNOTEQUALS ):
			{
				gpstring* r = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* l = m_Stack.top().m_String;
				m_Stack.top().m_Int = !l->same_with_case( *r );
			}	break;

			case ( Op::CODE_ISNOTEQUALC ):
			{
				const char* r = m_Stack.top().m_CString;  m_Stack.pop();
				const char* l = m_Stack.top().m_CString;
				m_Stack.top().m_Int = ::strcmp( l, r ) != 0;
			}	break;

			case ( Op::CODE_ISAPPROXEQUALF ):
			{
				float r = m_Stack.top().m_Float;  m_Stack.pop();
				float l = m_Stack.top().m_Float;
				m_Stack.top().m_Int = ::IsEqual(l, r );
			}	break;

			case ( Op::CODE_ISAPPROXEQUALS ):
			{
				gpstring* r = m_Stack.top().m_String;  m_Stack.pop();
				gpstring* l = m_Stack.top().m_String;
				m_Stack.top().m_Int = l->same_no_case( *r );
			}	break;

			case ( Op::CODE_ISAPPROXEQUALC ):
			{
				const char* r = m_Stack.top().m_CString;  m_Stack.pop();
				const char* l = m_Stack.top().m_CString;
				m_Stack.top().m_Int = ::stricmp( l, r ) == 0;
			}	break;

		// Type conversions.

			case ( Op::CODE_INTTOBOO ):
			{
				int& item = m_Stack.top( GetNextByte() ).m_Int;
				item = !!item;
			}	break;

			case ( Op::CODE_INTTOFLT ):
			{
				StackEntry& item = m_Stack.top( GetNextByte() );
				item.m_Float = scast <float> ( item.m_Int );
			}	break;

			case ( Op::CODE_FLTTOINT ):
			{
				StackEntry& item = m_Stack.top( GetNextByte() );
				item.m_Int = scast <int> ( item.m_Float );
			}	break;

			case ( Op::CODE_FLTTODBL ):
			{
				// note: this is not fast, but should be very rare. it's
				//  only needed in vararg functions to convert floats to
				//  doubles as per c++ standard.
				Stack::iterator pos = m_Stack.top_iter( GetNextByte() );
				float old = pos->m_Float;
				pos = m_Stack.get_container().insert( pos );
				rcast <double&> ( *pos ) = old;
			}	break;

			case ( Op::CODE_OFFTOPCC ):
			{
				StackEntry& item = m_Stack.top( GetNextByte() );
				item.m_CString = rcast <const char*> ( &*m_Pack->m_Strings.begin() + item.m_Int );
			}	break;

			case ( Op::CODE_STRTOPCC ):
			{
				StackEntry& item = m_Stack.top( GetNextByte() );
				item.m_CString = item.m_String->c_str();
			}	break;

			case ( Op::CODE_CSTRTOSTR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				StackEntry& item = m_Stack.top( GetNextByte() );
				if ( store != NULL )
				{
					gpstring* str = &store->GetString( index );
					*str = item.m_CString;
					item.m_String = str;
				}
			}	break;

			case ( Op::CODE_HDLTOPTR ):
			{
				// get class spec
				eVarType type = scast <eVarType> ( GetNextWord() );
				const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
				gpassert( spec != NULL );
				gpassert( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED );
				gpassert( spec->m_HandleGetProc != NULL );

				// do the conversion
				StackEntry& item = m_Stack.top( GetNextByte() );
				if ( item.m_RawData != 0 )
				{
					item.m_Ptr = (*spec->m_HandleGetProc)( item.m_RawData );
				}
			}	break;

			case ( Op::CODE_BASEADJUST ):
			{
				WORD offset = GetNextWord();
				StackEntry& item = m_Stack.top( GetNextByte() );
				item.m_RawData += offset;
			}	break;

			case ( Op::CODE_MASK3 ):
			{
				m_Stack.top().m_RawData &= 0x000000FF;
			}	break;

			case ( Op::CODE_MASK2 ):
			{
				m_Stack.top().m_RawData &= 0x0000FFFF;
			}	break;

		// Stack operations.

			case ( Op::CODE_PUSH1 ):
			{
				m_Stack.push( GetNextChar() );
			}	break;

			case ( Op::CODE_PUSH2 ):
			{
				m_Stack.push( GetNextSword() );
			}	break;

			case ( Op::CODE_PUSH4 ):
			{
				m_Stack.push( GetNextDword() );
			}	break;

			case ( Op::CODE_PUSHTHIS ):
			{
				m_Stack.push( m_Object );
			}	break;

			case ( Op::CODE_PUSHVM ):
			{
				m_Stack.push( this );
			}	break;

			case ( Op::CODE_PUSHOWNER ):
			{
				m_Stack.push( m_Object->GetOwner() );
			}	break;

			case ( Op::CODE_PUSHPRM ):
			{
				DWORD data = *rcast <const DWORD*> ( m_CurrentParam );
				m_Stack.push( data );
				m_CurrentParam = rcast <const BYTE*> ( m_CurrentParam ) + 4;
				gpassert( (size_t)m_CurrentParam <= ((size_t)m_Params.mem + m_Params.size) );
			}	break;

			case ( Op::CODE_SKIPPRM ):
			{
				m_CurrentParam = rcast <const BYTE*> ( m_CurrentParam ) + 4;
				gpassert( (size_t)m_CurrentParam <= ((size_t)m_Params.mem + m_Params.size) );
			}	break;

			case ( Op::CODE_SWAP ):
			{
				DWORD* l = &m_Stack.top( 0 ).m_RawData;
				DWORD* r = &m_Stack.top( 1 ).m_RawData;
				std::swap( *l, *r );
			}	break;

			case ( Op::CODE_POP ):
			{
				m_Stack.pop( GetNextByte() );
			}	break;

			case ( Op::CODE_POP1 ):
			{
				m_Stack.pop();
			}	break;

			case ( Op::CODE_LOADVAR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					m_Stack.push( store->GetVariable( index ) );
				}
			}	break;

			case ( Op::CODE_LOADSTR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					m_Stack.push( &store->GetString( index ) );
				}
			}	break;

			case ( Op::CODE_LOADCSTR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					m_Stack.push( store->GetString( index ).c_str() );
				}
			}	break;

			case ( Op::CODE_LOADHDL ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					m_Stack.push( store->GetHandle( index ) );
				}
			}	break;

			case ( Op::CODE_STOREVAR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					store->SetVariable( index, m_Stack.top().m_RawData );
				}
				m_Stack.pop();
			}	break;

			case ( Op::CODE_STORESTR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					store->SetString( index, *m_Stack.top().m_String );
				}
				m_Stack.pop();
			}	break;

			case ( Op::CODE_STORECSTR ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					store->SetString( index, m_Stack.top().m_CString );
				}
				m_Stack.pop();
			}	break;

			case ( Op::CODE_STOREHDL ):
			{
				UINT index = GetNextWord();
				Store* store = GetNextStore();
				if ( store != NULL )
				{
					store->SetHandle( index, m_Stack.top().m_RawData );
				}
				m_Stack.pop();
			}	break;

		// Storage operations.

			case ( Op::CODE_ALLOCVAR ):
			{
				m_LocalStore.AddVariable( 0 );
			}	break;

			case ( Op::CODE_ALLOCSTR ):
			{
				m_LocalStore.AddString();
			}	break;

			case ( Op::CODE_ALLOCHDL ):
			{
				m_LocalStore.AddHandle( scast <eVarType> ( GetNextWord() ) );
			}	break;

			case ( Op::CODE_FREEVAR ):
			{
				m_LocalStore.FreeVariables( GetNextByte() );
			}	break;

			case ( Op::CODE_FREESTR ):
			{
				m_LocalStore.FreeStrings( GetNextByte() );
			}	break;

			case ( Op::CODE_FREEHDL ):
			{
				m_LocalStore.FreeHandles( GetNextByte() );
			}	break;

		// State operations.

			case ( Op::CODE_SETSTATE ):
			{
				int state = GetNextWord();
				if ( m_Object != NULL )
				{
					m_Object->SetState( state );
				}
			}	break;

		// Fallthrough.

			default:
			{
				gpassertm( 0, "Unrecognized Skrit opcode" );
				rc = RESULT_FATAL;
				done = true;
			}
		}
	}

	return ( rc );
}

void Machine :: Unwind( void )
{
	// all variables should have gone out of scope unless it was a fatal!
	gpassert( (m_LastResult.m_Code == RESULT_FATAL) || m_Stack.empty() );
	gpassert( (m_LastResult.m_Code == RESULT_FATAL) || m_LocalStore.IsEmpty() );

	// free until we're there
	m_Stack.clear();
	m_LocalStore.FreeAll();
}

#if !GP_RETAIL

void Machine :: ReportException( const char* type, const SeException* sex, int paramCount ) const
{
	gpassert( m_StartIP != NULL );

	// start report (abort early if error context disabled)
	ReportSys::ContextRef ctx = gErrorContext;
	if ( !ctx->IsEnabled() )
	{
		return;
	}
	ReportSys::AutoReport autoReport( ctx );
	ctx->Output( "*** Skrit runtime error ***\n\n" );

	// location
	if ( m_StartIP != NULL )
	{
		Op::Code op = scast <Op::Code> ( *m_StartIP );
		ctx->OutputF( "Error description: %s\n", type );
		ctx->OutputF( "Error occurred at %s\n", MakeLocationText( m_StartIP ).c_str() );
		ctx->OutputF( "Current op is '%s'\n", Op::ToString( op ) );
		ctx->OutputEol();

		// function call report
		switch ( op )
		{
			case ( Op::CODE_CALL ):
			case ( Op::CODE_CALLSINGLETON ):
			case ( Op::CODE_CALLMEMBER ):
			case ( Op::CODE_CALLSINGLETONBASE ):
			case ( Op::CODE_CALLMEMBERBASE ):
			{
				// get the spec
				const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( *rcast <const WORD*> ( m_StartIP + 1 ) );
				ctx->OutputF( "Function call attempted was '%s'", spec->BuildQualifiedName().c_str() );
				if ( paramCount > 0 )
				{
					ctx->Output( ", parameters passed:\n" );

					// get the start of the param stack
					const StackEntry* args = &*(m_Stack.get_container().end() - paramCount);

					// dump the params
					{
						ReportSys::AutoIndent autoIndent( ctx );

						// for each param
						FuBi::FunctionSpec::ParamSpecs::const_iterator i,
								begin = spec->m_ParamSpecs.begin(), end = spec->m_ParamSpecs.end();
						for ( i = begin ; i != end ; ++i, --paramCount, ++args )
						{
							if ( i->m_Type.IsCString() )
							{
								// test string
								if ( ::IsBadStringPtr( args->m_CString, 1000 ) )
								{
									ctx->OutputF( "0x%08X (string pointer bad!)\n", args->m_CString );
								}
								else
								{
									ctx->OutputF( "'%s'\n", stringtool::EscapeCharactersToTokens( args->m_CString ).c_str() );
								}
							}
							else if ( i->m_Type.IsString() )
							{
								gpassert( args->m_String != NULL );		// $ there's bad c++ code somewhere if this assert fires!
								ctx->OutputF( "%s\n", *args->m_String );
							}
							else if ( i->m_Type == FuBi::VAR_FLOAT )
							{
								ctx->OutputF( "%f\n", args->m_Float );
							}
							else
							{
								ctx->OutputF( "0x%08X\n", args->m_RawData );
							}
						}

						// remaining for vararg
						if ( paramCount > 0 )
						{
							gpassert( spec->m_Flags & FuBi::FunctionSpec::FLAG_VARARG );
							for ( ; paramCount != 0 ; --paramCount, ++args )
							{
								ctx->OutputF( "0x%08X (vararg)\n", args->m_RawData );
							}
						}
					}
				}

				ctx->OutputEol();
			}
		}

		// disasm a little for context
		ctx->Output( "Partial disassembly:\n\n" );
		{
			ReportSys::AutoIndent autoIndent( ctx );
			Disassemble( m_StartIP, ctx );

			ctx->OutputEol();
		}
	}
	else
	{
		ctx->Output( "<ERROR> m_StartIP is NULL for some reason!!!" );

		if ( m_IP != NULL )
		{
			ctx->Output( "But m_IP is NOT NULL!!! ???" );
		}
		else
		{
			ctx->Output( "<ERROR> m_IP is NULL too!!!" );
		}
	}

	// sex report :)
	if ( sex != NULL )
	{
		ctx->OutputF( "Win32 Structured Exception:\n\n" );
		ReportSys::AutoIndent autoIndent( ctx );
		sex->Dump( ctx );

		ctx->OutputEol();
	}
}

#endif // !GP_RETAIL

Store* Machine :: GetNextStore( void )
{
	Raw::eStore storeType = scast <Raw::eStore> ( GetNextByte() );
	switch ( storeType )
	{
		// simple cases - stores either exist or not
		case ( Raw::STORE_LOCAL  ):  return ( &m_LocalStore );
		case ( Raw::STORE_SHARED ):  gpassert( m_SharedStore != NULL );  return ( m_SharedStore );

		// special case - global store may not exist if called from ctor/dtor
		//  via ObjectImpl (i.e. there is no Object instance)
		case ( Raw::STORE_GLOBAL ):  if ( m_GlobalStore != NULL )  return ( m_GlobalStore );
	}

	gpwarning( "Error: attempt to access non-shared global variable from within ctor or dtor\n" );
	return ( NULL );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
