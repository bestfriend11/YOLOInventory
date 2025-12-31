//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritStructure.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "SkritStructure.h"

#include "FileSysXfer.h"
#include "ReportSys.h"
#include "SkritByteCode.h"
#include "SkritEngine.h"
#include "StringTool.h"
#include "StdHelp.h"

#include <limits>

namespace Skrit  {  // begin of namespace Skrit
namespace Raw    {  // begin of namespace Raw

//////////////////////////////////////////////////////////////////////////////
// struct Header implementation

void Header :: Init( void )
{
	m_ProductID        = PRODUCT_ID;
	m_SkritID          = SKRIT_ID;
	m_TotalSize        = 0;
	m_SourceOffset     = 0;
	m_HeaderVersion    = HEADER_VERSION;
	m_SysExportsDigest = gFuBiSysExports.GetDigest();
	m_OwnerType        = FuBi::VAR_UNKNOWN;

	ZeroObject( m_SectionOffsets );
	ZeroObject( m_SectionSizes   );
}

bool Header :: ShouldCompile( const_mem_ptr mem, eVarType ownerType )
{
	// gotta be big enough to test
	if ( mem.size < sizeof( Header ) )
	{
		return ( true );
	}

	// ok have it test itself
	return ( rcast <const Header*> ( mem.mem )->ShouldCompile( ownerType ) );
}

bool Header :: ShouldCompile( eVarType ownerType ) const
{
	//$$$ test to make sure that we aren't ALWAYS recompiling!

	if (   (m_ProductID     != PRODUCT_ID)
		|| (m_SkritID       != SKRIT_ID)
		|| (m_HeaderVersion != HEADER_VERSION) )
	{
		return ( true );
	}

	return ( (m_SysExportsDigest != gFuBiSysExports.GetDigest()) && (m_OwnerType == ownerType) );
}

//////////////////////////////////////////////////////////////////////////////
// struct ExportEntry implementation

FuBi::TypeSpec ExportEntry :: BuildTypeSpec( eVarType type )
{
	FuBi::TypeSpec spec( type );
	if ( FuBi::IsUser( type ) )
	{
		const FuBi::ClassSpec* cls = gFuBiSysExports.FindClass( type );
		gpassert( cls != NULL );
		if ( cls->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
		{
			spec.m_Flags = FuBi::TypeSpec::FLAG_HANDLE;
		}
		else
		{
			spec.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
		}
	}
	else if ( type == FuBi::VAR_STRING )
	{
		spec.SetString();
	}
	return ( spec );
}

ExportEntry::eMatch ExportEntry :: CheckFunctionMatch( int paramOffset, const FuBi::FunctionSpec* spec, int* badParam ) const
{
	gpassert( spec != NULL );
	gpassert( paramOffset >= 0 );

	// check for return type compatibility
	{
		// build a full typespec for comparison
		FuBi::TypeSpec to( BuildTypeSpec( m_ReturnType ) );

		// if not exactly compatible or promotable it will fail
		FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( spec->m_ReturnType, to );
		if ( (compare != FuBi::TypeSpec::COMPARE_EQUAL) && (compare != FuBi::TypeSpec::COMPARE_PROMOTABLE) )
		{
			return ( MATCH_RETURN_MISMATCH );
		}
	}

	// check for param count match
	if ( (m_ParamCount + paramOffset) != scast <int> ( spec->m_ParamSpecs.size() ) )
	{
		return ( MATCH_DIFF_PARAM_COUNT );
	}

	// check for param type compatibility
	FuBi::FunctionSpec::ParamSpecs::const_iterator i,
			ibegin = spec->m_ParamSpecs.begin() + paramOffset,
			iend   = spec->m_ParamSpecs.end();
	const eVarType* j = GetParamsBegin();
	for ( i = ibegin ; i != iend ; ++i, ++j )
	{
		// build a full typespec for comparison
		FuBi::TypeSpec to( BuildTypeSpec( *j ) );

		// if not exactly compatible or promotable it will fail
		FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( i->m_Type, to );
		if ( (compare != FuBi::TypeSpec::COMPARE_EQUAL) && (compare != FuBi::TypeSpec::COMPARE_PROMOTABLE) )
		{
			if ( badParam != NULL )
			{
				*badParam = i - ibegin;
			}
			return ( MATCH_PARAM_MISMATCH );
		}
	}

	// all ok
	return ( MATCH_OK );
}

#if !GP_RETAIL

gpstring ExportEntry :: BuildNameAndParams( const void* param, bool allowBinaryOut ) const
{
	using namespace FuBi;

	gpstring out;

	// first, the function name
	out += m_Name;

	// next, the params
	out += "(";
	if ( m_ParamCount != 0 )
	{
		out += " ";
		const eVarType* i, * ibegin = GetParamsBegin(), * iend = GetParamsEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i != ibegin )
			{
				out += ", ";
			}

			FuBi::TypeSpec type( *i );
			if ( FuBi::IsUser( type.m_Type ) )
			{
				type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
			}

			const_mem_ptr mem;
			eXfer xfer = IsUnsignedInt( type.m_Type ) ? XFER_HEX : XFER_NORMAL;

			// reprocess if actually a string object pointer
			if ( type.IsString() )
			{
				out.appendf( "\"%s\"", (*(const gpstring**)param)->c_str() );
			}
			else if ( type.IsPassByValue() || type.IsPointerClass() )
			{
				gpstring paramText;
				if ( gFuBiSysExports.ToString( type, paramText, param, xfer, XMODE_QUIET ) )
				{
					out += paramText;
				}
				else if ( allowBinaryOut )
				{
					mem.mem = param;
					mem.size = type.GetSizeBytes();
				}
				else
				{
					out.appendf( "pod:%d", type.GetSizeBytes() );
				}
			}
			else
			{
				if ( gFuBiSysExports.IsPod( type.m_Type ) )
				{
					const void* obj = *(const BYTE**)param;

					TypeSpec localType( type.m_Type );
					int size = gFuBiSysExports.GetVarTypeSizeBytes( localType.m_Type );
					gpstring paramText;
					if ( gFuBiSysExports.ToString( localType, paramText, obj, xfer, XMODE_QUIET ) )
					{
						out.appendf( "[%s]", paramText.c_str() );
					}
					else if ( allowBinaryOut )
					{
						mem.mem = obj;
						mem.size = size;
					}
					else
					{
						out.appendf( "[pod:%d]", size );
					}
				}
				else
				{
					// get the cookie
					const ClassSpec* classSpec = gFuBiSysExports.FindClass( type.m_Type );
					gpassert( classSpec != NULL );
					gpassert( classSpec->m_Flags & ClassSpec::FLAG_CANRPC );
					gpassert( classSpec->m_InstanceToNetProc );
					if ( *(DWORD*)param != NULL )
					{
						DWORD cookie = (*classSpec->m_InstanceToNetProc)( (void*)*(DWORD*)param );
						out.appendf( cookie > 0xFFFF ? "0x%08X" : "0x%X", cookie );
					}
					else
					{
						out.append( "<null>" );
					}
				}
			}

			if ( mem )
			{
				if ( mem.mem != param )
				{
					out += '[';
				}

				const BYTE* b, * bbegin = (const BYTE*)mem.mem, * bend = bbegin + mem.size;
				if ( bbegin != bend )
				{
					for ( b = bbegin ; b != bend ; ++b )
					{
						if ( b != bbegin )
						{
							out += " ";
						}
						out.appendf( "%02x", *b );
					}
				}
				else
				{
					out += "null";
				}

				if ( mem.mem != param )
				{
					out += ']';
				}

			}

			param = (const BYTE*)param + type.GetSizeBytes();
		}
		out += " ";
	}
	out += ")";

	// finally add the function offset
	out.appendf( " [0x%08X]", m_FunctionOffset );

	return ( out );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Exports implementation

#if !GP_RETAIL

void Exports :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Output( "Exports\n"
				 "------------------------------------------------------------------------------\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );

	const ExportEntry* i, * ibegin = m_Entries, * iend = Advance <const ExportEntry*> ( this, sectionSize );
	for ( i = ibegin ; i != iend ; i = i->GetNext() )
	{
		ctx->OutputF( "Name           : '%s'\n"
					  "Function Offset: %04X\n",
					  i->m_Name, i->m_FunctionOffset );

		if ( i->m_ReturnType != FuBi::VAR_VOID )
		{
			ctx->OutputF( "Return Type    : %s\n", gFuBiSysExports.FindTypeName( i->m_ReturnType ) );
		}

		if ( i->m_EventBinding != (WORD)INVALID_FUNCTION_INDEX )
		{
			ctx->OutputF( "Event Binding  : %s\n", gFuBiSysExports.FindFunctionBySerialID( i->m_EventBinding )->BuildQualifiedName().c_str() );
		}

		if ( GetStateIndex( i->m_StateMember ) != STATE_INDEX_NONE )
		{
			ctx->OutputF( "State Member   : %d\n", i->m_StateMember );
		}

		if ( i->HasAtTime() )
		{
			if ( i->IsTimeInFrames() )
			{
				ctx->OutputF( "Trigger at     : %d frames\n", i->GetTimeInFrames() );
			}
			else
			{
				ctx->OutputF( "Trigger at     : %d msec\n", i->GetTimeInMsec() );
			}
		}

		if ( i->m_ParamCount != 0 )
		{
			ctx->Output( "Parameters     : " );

			const eVarType* j, * jbegin = i->GetParamsBegin(), * jend = i->GetParamsEnd();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j != jbegin )
				{
					ctx->Output( ", " );
				}
				FuBi::TypeSpec spec( ExportEntry::BuildTypeSpec( *j ) );
				spec.GenerateDocs( ctx );
			}
			ctx->OutputEol();
		}

		if ( i->m_DocsLength != 0 )
		{
			ctx->OutputF( "Documentation  : %s\n", i->GetDocs() );
		}

		ctx->OutputEol();
	}

	ctx->OutputEol();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Globals implementation

#if !GP_RETAIL

void Globals :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Output( "Globals\n"
				 "------------------------------------------------------------------------------\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );

	const GlobalEntry* i, * begin = m_Entries, * end = Advance <const GlobalEntry*> ( this, sectionSize );
	for ( i = begin ; i != end ; i = i->GetNext() )
	{
		// out variable offset
		ctx->OutputF( "%04X   ", (int)i - (int)begin );

		// out shared flag
		ctx->OutputF( "%s", (i->m_Store == STORE_SHARED) ? "shared " : "       " );

		// out shared flag
		ctx->OutputF( "%s", (i->m_Flags & FLAG_PROPERTY) ? "prop " : "     " );

		// out type and initializer
		eVarType type = scast <eVarType> ( i->m_Type );
		if ( FuBi::IsUser( type ) )
		{
			// UDT pointer or handle
			const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
			gpassert( spec != NULL );

			// handle?
			if ( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
			{
				ctx->OutputF( "H%s", spec->m_Name.c_str() );
			}
			else
			{
				ctx->OutputF( "%s*", spec->m_Name.c_str() );
			}
			ctx->Output( " = NULL" );
		}
		else
		{
			// simple variable or string
			ctx->OutputF( "%s = ", gFuBiSysExports.FindTypeName( type ) );
			if ( type == FuBi::VAR_FLOAT )
			{
				ctx->OutputF( "%f", *rcast <const float*> ( &i->m_InitialValue ) );
			}
			else if ( type == FuBi::VAR_STRING )
			{
				ctx->OutputF( "%04X (str-off)", i->m_InitialValue );
			}
			else if ( type == FuBi::VAR_BOOL )
			{
				ctx->OutputF( "%s", i->m_InitialValue ? "true" : "false" );
			}
			else
			{
				gpassert( type == FuBi::VAR_INT );
				ctx->OutputF( "%d", *rcast <const int*> ( &i->m_InitialValue ) );
			}
		}

		// name
		ctx->OutputF( " name = %04X (str-off)", i->m_NameOffset );

		// doc?
		if ( i->m_Flags & FLAG_PROPERTY )
		{
			const PropertyEntry* prop = rcast <const PropertyEntry*> ( i );
			if ( prop->m_DocsOffset != 0 )
			{
				ctx->OutputF( " /* %04X (str-off) */", prop->m_DocsOffset );
			}
		}

		// terminate
		ctx->OutputEol();
	}

	ctx->OutputEol();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Strings implementation

#if !GP_RETAIL

void Strings :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Output( "Strings\n"
				 "------------------------------------------------------------------------------\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );

	const char* i, * begin = m_Strings, * end = Advance <const char*> ( this, sectionSize );
	for ( i = begin ; i != end ; ++i )
	{
		ctx->OutputF( "%04X   '", i - begin );
		gpstring str;
		for ( ; *i != '\0' ; ++i )
		{
			switch ( *i )
			{
				case ( '\a' ):  str += "\\a";  break;
				case ( '\b' ):  str += "\\b";  break;
				case ( '\v' ):  str += "\\v";  break;
				case ( '\t' ):  str += "\\t";  break;
				case ( '\n' ):  str += "\\n";  break;
				case ( '\f' ):  str += "\\f";  break;
				case ( '\r' ):  str += "\\r";  break;

				default:
				{
					str += *i;
				}
			}
		}
		ctx->OutputF( "%s'\n", str.c_str() );
	}

	ctx->OutputEol();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct State implementation

#if !GP_RETAIL

void State :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize, DebugDb* debugDb ) const
{
	ReportSys::AutoReport autoReport( ctx );

	DebugDb localDb;
	if ( debugDb == NULL )
	{
		debugDb = &localDb;
	}

	ctx->Output( "State\n"
				 "------------------------------------------------------------------------------\n\n" );

	ctx->OutputF( "Initial state: %s\n", debugDb->GetValidStateName( m_InitialState ).c_str() );
	ctx->OutputF( "Total states : %d\n\n", m_TotalStates );

	ReportSys::AutoIndent autoIndent( ctx );

	const StateEntry* i, * begin = GetFirst(), * end = Advance <const StateEntry*> ( this, sectionSize );
	for ( i = begin ; i != end ; i = i->GetNext() )
	{
		switch ( i->m_Type )
		{
			case ( STATE_CONFIG ):
			{
				const StateConfig* entry = rcast <const StateConfig*> ( i );
				ctx->OutputF( "Type          : CONFIG\n"
							  "State         : %s\n", debugDb->GetValidStateName( entry->m_State ).c_str() );

				if ( entry->m_PollPeriod >= 0.0f )
				{
					ctx->Output( "Poll Period: " );
					if ( entry->m_PollPeriod == 0.0f )
					{
						ctx->Output( "<always>" );
					}
					else if ( entry->m_PollPeriod > 0.0f )
					{
						ctx->OutputF( "every %d seconds", entry->m_PollPeriod );
					}
					ctx->OutputEol();
				}

				ctx->OutputEol();

			}	break;

			case ( STATE_TRANSITION ):
			{
				const StateTransition* entry = rcast <const StateTransition*> ( i );
				if ( GetStateIndex( entry->m_ToState ) != STATE_INDEX_NONE )
				{
					ctx->OutputF( "Type          : TRANSITION %s -> %s",
								  debugDb->GetValidStateName( entry->m_FromState ).c_str(),
								  debugDb->GetValidStateName( entry->m_ToState ).c_str() );
				}
				else
				{
					ctx->OutputF( "Type          : TRIGGER [in %s]",
								  debugDb->GetValidStateName( entry->m_FromState ).c_str() );
				}
				if ( entry->m_ResponseCodeOffset != INVALID_FUNCTION_INDEX )
				{
					ctx->OutputF( " = %04X", entry->m_ResponseCodeOffset );
				}
				ctx->OutputEol();
				gpassert( entry->m_ConditionCount != 0 );

				const ConditionEntry* cond = entry->GetFirst();
				for ( int j = 0 ; j < entry->m_ConditionCount ; ++j )
				{
					if ( cond->m_IsStatic )
					{
						const ConditionStatic* scond = rcast <const ConditionStatic*> ( cond );
						cond = scond->GetNext();
						gpassert( scond->m_EventSerial != (WORD)INVALID_FUNCTION_INDEX );

						ctx->OutputF( "- Static      : %s", gFuBiSysExports.FindFunctionBySerialID( scond->m_EventSerial )->m_Name.c_str() );
						if ( scond->m_TestDataSize != 0 )
						{
							ctx->Output( "( " );
							scond->OutputParams( ctx );
							ctx->Output( " )" );
						}
					}
					else
					{
						const ConditionDynamic* dcond = rcast <const ConditionDynamic*> ( cond );
						cond = dcond->GetNext();

						ctx->OutputF( "- Dynamic     : Function Offset = %04X", dcond->m_CodeOffset );
					}
					ctx->OutputEol();
				}
				
				ctx->OutputEol();

			}	break;
		}
	}

	ctx->OutputEol();
}

void ConditionStatic :: OutputParams( ReportSys::ContextRef ctx ) const
{
	const FuBi::FunctionSpec* func = gFuBiSysExports.FindFunctionBySerialID( m_EventSerial );

	const DWORD* di,
			   * dbegin = rcast <const DWORD*> ( GetDataBegin() ),
			   * dend   = rcast <const DWORD*> ( GetDataEnd  () );
	FuBi::FunctionSpec::ParamSpecs::const_iterator pi = func->m_ParamSpecs.begin();
	for ( di = dbegin, ++pi ; di != dend ; ++di, ++pi )
	{
		if ( di != dbegin )
		{
			ctx->Output( ", " );
		}

		gpstring str;
		if ( gFuBiSysExports.ToString( pi->m_Type, str, di, FuBi::XFER_NORMAL, FuBi::XMODE_QUIET ) )
		{
			ctx->Output( str );
		}
		else
		{
			ctx->OutputF( "0x%08X", *di );
		}
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ByteCode implementation

#if !GP_RETAIL

void ByteCode :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize, DebugDb* debugDb ) const
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Output( "Byte code\n"
				 "------------------------------------------------------------------------------\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );

	const BYTE* iter = NULL;
	do
	{
		iter = GenerateNextDocs( ctx, sectionSize, iter, debugDb );
	}
	while( iter != NULL );

	ctx->OutputEol();
}

const BYTE* ByteCode :: GenerateNextDocs( ReportSys::ContextRef ctx, UINT sectionSize, const BYTE* last, const DebugDb* debugDb ) const
{
	const BYTE* i     = (last != NULL ) ? last : m_Code;
	const BYTE* begin = m_Code;
	const BYTE* end   = Advance <const BYTE*> ( this, sectionSize );

	if ( i == end )
	{
		return ( NULL );
	}

	int maxData = Op::GetMaxDataSizeBytes();

	// get data for opcode
	Op::Code code = scast <Op::Code> ( *i );
	const Op::CodeData* data = &Op::GetData( code );

	// out instruction offset
	ctx->OutputF( "%04X", i - begin );

	// out bytecode
	ctx->OutputF( "   %02X ", *i );
	for ( int j = 0 ; j < maxData ; ++j )
	{
		if ( j < data->m_DataSizeBytes )
		{
			ctx->OutputF( "%02X ", i[ j + 1 ] );
		}
		else
		{
			ctx->OutputF( "   " );
		}
	}

	// out text
	ctx->OutputF( "  %s  ", data->m_Name );

	// out data bytes
	if ( data->m_DataSizeBytes > 0 )
	{
		// space out bit
		for ( int k = 0, count = 20 /*used to be maxlen$$$*/ - data->m_NameLen ; k < count ; ++k )
		{
			// not exactly efficient but who cares
			ctx->Output( " " );
		}

		// out the data
		if ( data->m_DataSizeBytes == 1 )
		{
			ctx->OutputF( "%d", i[ 1 ] );
		}
		else if ( data->m_DataSizeBytes == 2 )
		{
			ctx->OutputF( "%04X", *rcast <const WORD*> ( i + 1 ) );
		}
		else if ( (data->m_DataSizeBytes == 4) && (code != Op::CODE_CSTRTOSTR) )
		{
			ctx->OutputF( "%08X", *rcast <const DWORD*> ( i + 1 ) );
		}
	}

	// special
	switch ( code )
	{
		case ( Op::CODE_CALL ):
		case ( Op::CODE_CALLSINGLETON ):
		case ( Op::CODE_CALLMEMBER ):
		case ( Op::CODE_CALLSINGLETONBASE ):
		case ( Op::CODE_CALLMEMBERBASE ):
		{
			const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( *rcast <const WORD*> ( i + 1 ) );
			if ( spec != NULL )
			{
				ctx->OutputF( " (%s)", spec->BuildQualifiedName().c_str() );
			}
			else
			{
				ctx->OutputF( " <unknown function>" );
			}

			if ( Op::GetTraits( code ) & Op::TRAIT_BASEOFFSET )
			{
				WORD baseOffset = *rcast <const WORD*> ( i + 3 );
				ctx->OutputF( " +%d", baseOffset );
			}
		}	break;

		case ( Op::CODE_BRANCH ):
		case ( Op::CODE_BRANCHZERO ):
		{
			short offset = *rcast <const short*> ( i + 1 );
			ctx->OutputF( " (%04X)", i - begin + offset + 1 + data->m_DataSizeBytes );
		}	break;

		case ( Op::CODE_CSTRTOSTR ):
		{
			short outindex = *rcast <const short*> ( i + 1 );
			BYTE store = i[ 3 ];
			BYTE inindex = i[ 4 ];
			ctx->OutputF( "%d, %d, (%d)", outindex, inindex, store );
		}	break;

		case ( Op::CODE_HDLTOPTR ):
		{
			eVarType type = scast <eVarType> ( *rcast <const WORD*> ( i + 1 ) );
			ctx->OutputF( "%d (%s)", i[ 3 ], gFuBiSysExports.FindTypeName( type ) );
		}	break;

		case ( Op::CODE_BASEADJUST ):
		{
			ctx->OutputF( "%d +%d", i[ 3 ], *rcast <const short*> ( i + 1 ) );
		}	break;

		case ( Op::CODE_LOADVAR ):
		case ( Op::CODE_LOADSTR ):
		case ( Op::CODE_LOADHDL ):
		case ( Op::CODE_STOREVAR ):
		case ( Op::CODE_STORESTR ):
		case ( Op::CODE_STOREHDL ):
		{
			ctx->OutputF( "%d", *rcast <const WORD*> ( i + 1 ) );
			if ( i[ 3 ] == Raw::STORE_GLOBAL )
			{
				ctx->Output( " (global)" );
			}
			else if ( i[ 3 ] == Raw::STORE_SHARED )
			{
				ctx->Output( " (shared)" );
			}
		}	break;

		case ( Op::CODE_ALLOCHDL ):
		{
			eVarType type = scast <eVarType> ( *rcast <const WORD*> ( i + 1 ) );
			ctx->OutputF( " (H%s)", gFuBiSysExports.FindTypeName( type ) );
		}	break;

		case ( Op::CODE_SETSTATE ):
		{
			int state = *rcast <const WORD*> ( i + 1 );
			if ( debugDb )
			{
				gpstring str = debugDb->GetStateName( state );
				if ( !str.empty() )
				{
					ctx->OutputF( " (%s)", str.c_str() );
				}
			}
		}	break;
	}

	// done
	ctx->OutputEol();
	i += 1 + data->m_DataSizeBytes;
	return ( (i == end) ? NULL : i );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Debug implementation

#if !GP_RETAIL

void Debug :: GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Output( "Debug Info\n"
				 "------------------------------------------------------------------------------\n\n" );
	ReportSys::AutoIndent autoIndent( ctx );

	int sourceIndex = 0;

	const DebugEntry* i, * begin = m_Entries, * end = Advance <const DebugEntry*> ( this, sectionSize );
	for ( i = begin ; i != end ; i = i->GetNext() )
	{
		switch ( i->m_Type )
		{
			case ( DEBUG_LOCATION ):
			{
				const DebugLocation* entry = rcast <const DebugLocation*> ( i );
				ctx->OutputF( "Type: LOCATION   - Code Offset: %04X, Source Line: %d\n",
							  entry->m_CodeOffset, entry->m_Line );
			}
			break;

			case ( DEBUG_STATE ):
			{
				const DebugState* entry = rcast <const DebugState*> ( i );
				ctx->OutputF( "Type: STATE      - State %d = %s\n", entry->m_State, entry->GetName() );
			}
			break;

			case ( DEBUG_SOURCEFILE ):
			{
				const DebugSourceFile* entry = rcast <const DebugSourceFile*> ( i );
				ctx->OutputF( "Type: SOURCEFILE - File %d = '%s', includer = %d line %d\n",
							  sourceIndex, entry->GetName(), entry->m_IncluderIndex, entry->m_IncludeLine );
				++sourceIndex;
			}
			break;
		}
	}

	ctx->OutputEol();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Pack implementation

Pack :: Pack( void )
{
	m_Buffers[ SECTION_EXPORTS  ] = &m_Exports;
	m_Buffers[ SECTION_GLOBALS  ] = &m_Globals;
	m_Buffers[ SECTION_STRINGS  ] = &m_Strings;
	m_Buffers[ SECTION_STATE    ] = &m_State;
	m_Buffers[ SECTION_BYTECODE ] = &m_ByteCode;
#	if !GP_RETAIL
	m_Buffers[ SECTION_DEBUG    ] = &m_Debug;
#	endif // !GP_RETAIL
}

void Pack :: Copy( const Header* header )
{
	gpassert( header != NULL );
	const BYTE* base = rcast <const BYTE*> ( header );

	Buffer** i, ** begin = m_Buffers, ** end = begin + ELEMENT_COUNT( m_Buffers );
	for ( i = begin ; i != end ; ++i )
	{
		Buffer* buffer = *i;
		int section = i - begin;

		buffer->resize( header->m_SectionSizes[ section ] );
		if ( !buffer->empty() )
		{
			::memcpy( &*buffer->begin(), base + header->m_SectionOffsets[ section ], buffer->size() );
		}
	}
}

void Pack :: Reset( void )
{
	Buffer** i, ** begin = m_Buffers, ** end = begin + ELEMENT_COUNT( m_Buffers );
	for ( i = begin ; i != end ; ++i )
	{
		(*i)->clear();
	}
}

void Pack :: Swap( Pack& pack )
{
	Buffer** i, ** begin = m_Buffers, ** end = begin + ELEMENT_COUNT( m_Buffers ), ** out = pack.m_Buffers;
	for ( i = begin ; i != end ; ++i, ++out )
	{
		(*i)->swap( **out );
	}
}

UINT32 Pack :: AddCrc32( UINT32 seed ) const
{
	typedef const Buffer* CBuffer;
	const CBuffer* i, * ibegin = m_Buffers, * iend = ARRAY_END( m_Buffers );
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( *i != NULL )
		{
			::AddCRC32( seed, &*(*i)->begin(), (*i)->size_bytes() );
		}
	}

	return ( seed );
}

#if !GP_RETAIL

void Pack :: GenerateDocs( ReportSys::ContextRef ctx ) const
{
	DebugDb debugDb( *this );

	if ( !m_Exports.empty() )
	{
		GetExports()->GenerateDocs( ctx, m_Exports.size() );
		ctx->OutputEol();
	}
	if ( !m_Globals.empty() )
	{
		GetGlobals()->GenerateDocs( ctx, m_Globals.size() );
		ctx->OutputEol();
	}
	if ( !m_Strings.empty() )
	{
		GetStrings()->GenerateDocs( ctx, m_Strings.size() );
		ctx->OutputEol();
	}
	if ( !m_State.empty() )
	{
		GetState()->GenerateDocs( ctx, m_State.size(), &debugDb );
		ctx->OutputEol();
	}
	if ( !m_ByteCode.empty() )
	{
		GetByteCode()->GenerateDocs( ctx, m_ByteCode.size(), &debugDb );
		ctx->OutputEol();
	}
	if ( !m_Debug.empty() )
	{
		GetDebug()->GenerateDocs( ctx, m_Debug.size() );
		ctx->OutputEol();
	}

	ctx->OutputF( "\nTotal size: %d\n\n", GetSize() );
}

struct LineEntry
{
	gpstring m_Text;
	int      m_Line;

	LineEntry( void )
		: m_Line( 0 )  {  }
	LineEntry( const gpstring& text, int line )
		: m_Text( text ), m_Line( line )  {  }
};

struct LineCollBuilder
{
	typedef stdx::fast_vector <LineEntry> LineColl;
	typedef DebugDb::LocationColl LocationColl;
	typedef DebugDb::SourceFileColl SourceFileColl;

	DebugDb      m_DebugDb;
	LineColl     m_LineColl;
	LocationColl m_LocColl;
	int          m_CurrentOffset;

	LineCollBuilder( const Pack& pack )
		: m_DebugDb( pack )
		, m_LocColl( m_DebugDb.GetLocationBegin(), m_DebugDb.GetLocationEnd() )
	{
		m_CurrentOffset = 0;
	}

	bool Init( ReportSys::ContextRef ctx, FileSys::Reader* reader, SourceFileColl::const_iterator& ifile, LocationColl::iterator& icode )
	{
		int sourceIndex = ifile - m_DebugDb.GetSourceFileBegin();
		gpstring fileName, fullName;

		// build reader if none
		std::auto_ptr <FileSys::FileHandleReader> localReader;
		if ( reader == NULL )
		{
			fileName = m_DebugDb.GetSourceFileName( sourceIndex );
			fullName = Engine::MakeFullSkritFileName( fileName );

			// output divider
			OutputDivider( fileName, fullName, false );

			// try to open the file
			FileSys::FileHandle file;
			if ( !file.Open( fullName ) )
			{
				ctx->OutputF( "ERROR: unable to open file '%s', downshifting to raw disassembly...\n\n", fileName.c_str() );
				return ( false );
			}

			// create local reader for it
			localReader = stdx::make_auto_ptr( new FileSys::FileHandleReader( file, true ) );
			reader = localReader.get();
		}

		// if we're at the end, just read in the rest
		int currentLine = 0;
		for ( ; ; )
		{
			if ( (ifile == m_DebugDb.GetSourceFileEnd()) || ((ifile + 1) == m_DebugDb.GetSourceFileEnd()) )
			{
				// no more includes at all, so read in the rest
				ReadLines( reader, -1, sourceIndex, currentLine, icode );
				break;
			}
			else
			{
				const DebugSourceFile& include = *(ifile + 1);

				// if not in current file...
				if ( include.m_IncluderIndex != sourceIndex )
				{
					// there are no includes in this file, so read in the rest
					ReadLines( reader, -1, sourceIndex, currentLine, icode );
					break;
				}
				else
				{
					// read in lines up to and including that include line
					int deltaLines = include.m_IncludeLine - currentLine;
					ReadLines( reader, deltaLines, sourceIndex, currentLine, icode );
					currentLine += deltaLines;

					// get rid of include
					m_LineColl.pop_back();
					--m_CurrentOffset;

					// include that file
					++ifile;
					if ( !Init( ctx, NULL, ifile, icode ) )
					{
						return ( false );
					}

					// output another divider saying we've continued
					OutputDivider( fileName, fullName, true );
				}
			}
		}

		// done
		return ( true );
	}

	bool Init( ReportSys::ContextRef ctx, FileSys::Reader* base )
	{
		SourceFileColl::const_iterator ifile = m_DebugDb.GetSourceFileBegin();
		LocationColl::iterator icode = m_LocColl.begin();
		return ( Init( ctx, base, ifile, icode ) );
	}

	void OutputDivider( const gpstring& fileName, const gpstring& fullName, bool cont )
	{
		gpstring& divider = m_LineColl.push_back()->m_Text;
		divider = "--- ";
		++m_CurrentOffset;

		if ( !fileName.empty() )
		{
			divider.appendf( "%s ", fileName.c_str() );
			if ( !fileName.same_no_case( fullName ) )
			{
				divider.appendf( "(%s) ", fullName.c_str() );
			}
		}
		if ( cont )
		{
			divider += "(cont.) ";
		}

		int extra = 70 - divider.length();
		if ( extra > 0 )
		{
			divider.append( extra, '-' );
		}
	}

	void ReadLines( FileSys::Reader* reader, int deltaLines, int sourceIndex, int baseLine, LocationColl::iterator& icode )
	{
		gpassert( reader != NULL );

		// adjust for remaining
		if ( deltaLines < 0 )
		{
			deltaLines = std::numeric_limits <int>::max();
		}

		// read in lines
		gpstring text;
		int linesRead = 0;
		for ( ; deltaLines > 0 ; --deltaLines, ++linesRead )
		{
			if ( !reader->ReadLine( text ) )
			{
				break;
			}

			if ( !text.empty() && (*text.rbegin() == '\r') )
			{
				text.erase( text.size() - 1 );
			}

			stringtool::ConvertTabsToSpaces( text );
			m_LineColl.push_back( LineEntry( text, baseLine + linesRead + 1 ) );
			text.clear();
		}

		// adjust location data
		while (   (icode != m_LocColl.end())
			   && (icode->m_SourceIndex == sourceIndex)
			   && (icode->m_Line < (baseLine + linesRead)) )
		{
			icode->m_Line = (WORD)(icode->m_Line + m_CurrentOffset - baseLine);
			++icode;
		}

		// finish up
		m_CurrentOffset += linesRead;
	}
};


void Pack :: DisassembleMixed( ReportSys::ContextRef ctx, FileSys::Reader* base, DWORD startIp, DWORD maxBytes, DWORD tagIp ) const
{
	// $ this isn't exactly efficient, but it was easy to write

	ReportSys::AutoReport autoReport( ctx );

// Setup.

	// if no debug symbols, do cheaper one
	if ( m_Debug.empty() )
	{
		DisassembleRaw( ctx, startIp, maxBytes, tagIp );
		return;
	}

	// build the database
	LineCollBuilder builder( *this );
	if ( !builder.Init( ctx, base ) )
	{
		DisassembleRaw( ctx, startIp, maxBytes, tagIp );
		return;
	}

// Now do the mixed-mode disassembly.

	LineCollBuilder::LocationColl::const_iterator icode = builder.m_LocColl.begin(), codeEnd = builder.m_LocColl.end();
	LineCollBuilder::LineColl::const_iterator iline = builder.m_LineColl.begin(), lineEnd = builder.m_LineColl.end();

	const BYTE* iobj = NULL, * objBegin = NULL, * objEnd = NULL;
	if ( !m_ByteCode.empty() )
	{
		iobj     = GetByteCode()->m_Code;
		objBegin = iobj;
		objEnd   = Advance <const BYTE*> ( objBegin, m_ByteCode.size() );
	}

	int sourceLine = 1;
	while ( (icode != codeEnd) || (iline != lineEnd) || (iobj != objEnd) )
	{
		// figure out end of source block
		int nextSourceLine = std::numeric_limits <int>::max();
		const BYTE* nextObj = objEnd;
		if ( icode != codeEnd )
		{
			nextSourceLine = icode->m_Line;
			++icode;
			if ( icode != codeEnd )
			{
				nextObj = objBegin + icode->m_CodeOffset;
			}
		}

		// output source till we get to the end
		for ( ; sourceLine <= nextSourceLine ; ++sourceLine )
		{
			if ( iline == lineEnd )
			{
				sourceLine = nextSourceLine;
				break;
			}

			if ( (iobj - &*m_ByteCode.begin()) >= (int)startIp )
			{
				if ( tagIp != 0xFFFFFFFF )
				{
					ctx->Output( "  " );
				}

				if ( iline->m_Line )
				{
					ctx->OutputF( "%4d: %s\n", iline->m_Line, iline->m_Text.c_str() );
				}
				else
				{
					ctx->OutputF( "%s\n", iline->m_Text.c_str() );
				}
			}
			++iline;
		}

		// output disassembled code until we get to the end
		while ( iobj != nextObj )
		{
			DWORD offset = iobj - &*m_ByteCode.begin();
			if ( (maxBytes != 0) && (offset > (startIp + maxBytes)) )
			{
				return;
			}

			if ( offset >= startIp )
			{
				if ( tagIp != 0xFFFFFFFF )
				{
					ctx->Output( (offset == tagIp) ? "->" : "  " );
				}
				iobj = GetByteCode()->GenerateNextDocs( ctx, m_ByteCode.size(), iobj, &builder.m_DebugDb );
			}
			else
			{
				iobj += 1 + Op::GetData( scast <Op::Code> ( *iobj ) ).m_DataSizeBytes;
				if ( iobj == &*m_ByteCode.end() )
				{
					iobj = NULL;
				}
			}

			if ( iobj == NULL )
			{
				iobj = objEnd;
				break;
			}
		}
	}
}

void Pack :: DisassembleRaw( ReportSys::ContextRef ctx, DWORD startIp, DWORD maxBytes, DWORD tagIp ) const
{
	ReportSys::AutoReport autoReport( ctx );

	const BYTE* iter = &*m_ByteCode.begin() + startIp;
	do
	{
		DWORD offset = iter - &*m_ByteCode.begin();
		if ( tagIp != 0xFFFFFFFF )
		{
			ctx->Output( (offset == tagIp) ? "->" : "  " );
		}
		iter = GetByteCode()->GenerateNextDocs( ctx, m_ByteCode.size(), iter );
	}
	while( (iter != NULL) && ((iter - &*m_ByteCode.begin()) < (int)(startIp + maxBytes)) );
}


#endif // !GP_RETAIL

size_t Pack :: GetSize( void ) const
{
	size_t size = sizeof( Raw::Header );

	const Buffer* const* i, * const* begin = m_Buffers, * const* end = begin + ELEMENT_COUNT( m_Buffers );
	for ( i = begin ; i != end ; ++i )
	{
		size += GetDwordAlignUp( (*i)->size() );
	}

	return ( size );
}

void Pack :: Save( mem_ptr out ) const
{
	gpassert( out.size >= GetSize() );
	BYTE* runner = rcast <BYTE*> ( out.mem ), * base = runner;

	// store header
	Header* header = rcast <Header*> ( runner );
	header->Init();
	runner += sizeof( Header );

	// store tables
	const Buffer* const* i, * const* begin = m_Buffers, * const* end = begin + ELEMENT_COUNT( m_Buffers );
	for ( i = begin ; i != end ; ++i )
	{
		const Buffer* buffer = *i;
		int section = i - begin;

		if ( !buffer->empty() )
		{
			header->m_SectionOffsets[ section ] = runner - base;
			header->m_SectionSizes  [ section ] = buffer->size();
			::memcpy( runner, &*buffer->begin(), buffer->size() );
			runner += GetDwordAlignUp( buffer->size() );
		}
	}

	// final size
	header->m_TotalSize = runner - base;
}

//////////////////////////////////////////////////////////////////////////////
// class DebugDb implementation

#if !GP_RETAIL

DebugDb :: DebugDb( const Pack& pack )
{
	// bail early if no debug info
	if ( pack.m_Debug.empty() )
	{
		return;
	}

	// index the debug info
	const DebugEntry* i,
					* begin = pack.GetDebug()->m_Entries,
					* end   = Advance <const DebugEntry*> ( pack.GetDebug(), pack.m_Debug.size() );
	for ( i = begin ; i != end ; i = i->GetNext() )
	{
		switch ( i->m_Type )
		{
			case ( DEBUG_LOCATION ):
			{
				m_Locations.push_back( *rcast <const DebugLocation*> ( i ) );
			}
			break;

			case ( DEBUG_STATE ):
			{
				const DebugState* dbgState = rcast <const DebugState*> ( i );
				int state = dbgState->m_State;
				m_States.resize( max( scast <int> ( m_States.size() ), state + 1 ) );
				m_States[ state ] = dbgState->GetName();
			}
			break;

			case ( DEBUG_SOURCEFILE ):
			{
				const DebugSourceFile* dbgSourceFile = rcast <const DebugSourceFile*> ( i );
				m_SourceFileNames.push_back( dbgSourceFile->GetName() );
				m_SourceFiles.push_back( *dbgSourceFile );
			}
			break;

			default:
			{
				gpassert( 0 );		// should never get here!
			}
		}
	}
}

const DebugLocation* DebugDb :: GetLocationFromIp( DWORD ip ) const
{
	// $ this can be optimized with a bsearch or an index coll

	LocationColl::const_iterator i, begin = m_Locations.begin(), end = m_Locations.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( ip <= i->m_CodeOffset )
		{
			// use prev entry
			if ( i != begin )
			{
				return ( &*(i - 1) );
			}
			else
			{
				return ( &*i );
			}
		}
	}

	// must be the last one
	if ( begin != end )
	{
		return ( &m_Locations.back() );
	}
	else
	{
		// no code? is this a coding bug?
		gpassert( 0 );
		return ( NULL );
	}
}

gpstring DebugDb :: GetStateName( int state ) const
{
	gpstring name;

	gpassert( state >= 0 );
	if ( state < scast <int> ( m_States.size() ) )
	{
		name = m_States[ state ];
	}
	return ( name );
}

gpstring DebugDb :: GetValidStateName( int state ) const
{
	if ( state == STATE_INDEX_NONE )
	{
		return ( "<global>" );
	}
	else if ( state == Object::STATE_INDEX_INVALID )
	{
		return ( "<none>" );
	}
	else
	{
		gpstring name = GetStateName( state );
		if ( name.empty() )
		{
			stringtool::Set( state, name );
		}
		return ( name );
	}
}

gpstring DebugDb :: GetStateName( WORD state ) const
{
	return ( GetStateName( GetStateIndex( state ) ) );
}

gpstring DebugDb :: GetValidStateName( WORD state ) const
{
	return ( GetValidStateName( GetStateIndex( state ) ) );
}

gpstring DebugDb :: GetSourceFileName( int index ) const
{
	gpstring name;

	gpassert( index >= 0 );
	if ( index < scast <int> ( m_SourceFileNames.size() ) )
	{
		name = m_SourceFileNames[ index ];
	}
	return ( name );
}

gpstring DebugDb :: GetValidSourceFileName( int index ) const
{
	gpstring name = GetSourceFileName( index );
	if ( name.empty() )
	{
		stringtool::Set( index, name );
	}
	return ( name );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Raw
}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
