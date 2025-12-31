//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTypes.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"

#include "FuBi.h"
#include "FuBiSchema.h"
#include "ReportSys.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// enum eVarType implementation

bool IsRegularEnum( eVarType type )
{
	if ( IsEnum( type ) )
	{
		return ( gFuBiSysExports.FindEnum( type )->m_Exporter.IsContinuous() );
	}
	return ( false );
}

bool IsIrregularEnum( eVarType type )
{
	if ( IsEnum( type ) )
	{
		return ( !gFuBiSysExports.FindEnum( type )->m_Exporter.IsContinuous() );
	}
	return ( false );
}

bool IsUnsignedInt( eVarType type )
{
	switch ( type )
	{
		case ( VAR_UCHAR  ):
		case ( VAR_USHORT ):
		case ( VAR_UINT   ):
		case ( VAR_UINT64 ):
		{
			return ( true );
		}
		break;
	}

	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// struct VarTypeSpec implementation

VarTypeSpec :: VarTypeSpec( const ClassSpec* spec )
{
	::ZeroObject( *this );
	m_InternalName = spec->m_Name;
	m_ExternalName = spec->m_Name;

	if ( spec->m_Flags & ClassSpec::FLAG_POINTERCLASS )
	{
		m_SizeBytes = sizeof( void* );
		m_Flags = Trait::FLAG_POD;
	}
}

VarTypeSpec :: VarTypeSpec( const EnumSpec* spec )
{
	m_InternalName   = spec->m_Name;
	m_ExternalName   = spec->m_Name;
	m_SizeBytes      = sizeof( m_Flags );		// $ same size as any other enum
	m_Flags          = Trait::FLAG_POD | Trait::FLAG_FORCETEXT;
	m_ToStringProc   = spec->m_Exporter.m_ToStringProc;
	m_FromStringProc = spec->m_Exporter.m_FromStringProc;
	m_CopyVarProc    = NULL;
}

VarTypeSpec :: VarTypeSpec( const char* iname, const char* ename, int size,
							Trait::eFlags flags, ToStringProc to, FromStringProc from,
							CopyVarProc copy )
{
	m_InternalName   = iname;
	m_ExternalName   = ename;
	m_SizeBytes      = size;
	m_Flags          = flags;
	m_ToStringProc   = to;
	m_FromStringProc = from;
	m_CopyVarProc    = copy;
}

//////////////////////////////////////////////////////////////////////////////
// struct TypeSpec implementation

bool TypeSpec :: IsSimple( void ) const
{
	// pointer classes are simple
	if ( IsPointerClass() )
	{
		return ( true );
	}

	// must pass by value
	if ( !IsPassByValue() )
	{
		return ( false );
	}

	// check for user POD type
	if ( IsUser( m_Type ) )
	{
		if ( gFuBiSysExports.FindClass( m_Type )->m_Flags & ClassSpec::FLAG_POD )
		{
			return ( true );
		}
	}
	else
	{
		return ( IsBasic( m_Type ) || IsEnum( m_Type ) );		// must be built-in type or enum
	}

	// default is "no"
	return ( false );
}

bool TypeSpec :: IsSimpleInteger( void ) const
{
	return ( gFuBiSysExports.IsSimpleInteger( m_Type ) && (m_Flags == FLAG_NONE) );
}

bool TypeSpec :: IsSimpleFloat( void ) const
{
	return ( gFuBiSysExports.IsSimpleFloat( m_Type ) && (m_Flags == FLAG_NONE) );
}

bool TypeSpec :: IsPointerClass( void ) const
{
	if ( IsUser( m_Type ) )
	{
		const ClassSpec* spec = gFuBiSysExports.FindClass( m_Type );
		if ( spec->m_Flags & ClassSpec::FLAG_POINTERCLASS )
		{
			gpassert( !IsPassByValue() );
			return ( true );
		}
	}
	return ( false );
}

bool TypeSpec :: IsCString( void ) const
{
	// support it iff char* or const char*
	if (   ((m_Type == VAR_CHAR) || (m_Type == VAR_SCHAR))
		&& ((m_Flags & ~FLAG_CONST) == FLAG_POINTER) )
	{
		return ( true );
	}

	return ( false );
}

bool TypeSpec :: IsCStringW( void ) const
{
	// support it iff char* or const char*
	if (   (m_Type == VAR_USHORT)
		&& ((m_Flags & ~FLAG_CONST) == FLAG_POINTER) )
	{
		return ( true );
	}

	return ( false );
}

bool TypeSpec :: IsString( void ) const
{
	// support it iff gpstring&, *, const
	if ( m_Type == VAR_STRING )
	{
		eFlags flags = m_Flags & NOT( FLAG_CONST );
		if ( (flags == FLAG_POINTER) || (flags == FLAG_REFERENCE) )
		{
			return ( true );
		}
	}

	return ( false );
}

bool TypeSpec :: IsStringW( void ) const
{
	// support it iff gpstring&, *, const
	if ( m_Type == VAR_WSTRING )
	{
		eFlags flags = m_Flags & NOT( FLAG_CONST );
		if ( (flags == FLAG_POINTER) || (flags == FLAG_REFERENCE) )
		{
			return ( true );
		}
	}

	return ( false );
}

void TypeSpec :: SetCString( void )
{
	m_Type = VAR_CHAR;
	m_Flags = FLAG_CONST | FLAG_POINTER;
}

void TypeSpec :: SetString( void )
{
	m_Type = VAR_STRING;
	m_Flags = FLAG_POINTER;
}

bool TypeSpec :: IsFuBiCookie( void ) const
{
	return (    (m_Type == gFuBiSysExports.FindFuBiCookie()->m_Type)
			 && (m_Flags == FLAG_POINTER) );
}

bool TypeSpec :: IsSingleton( void ) const
{
	if ( IsUser( m_Type ) )
	{
		return ( !!(gFuBiSysExports.FindClass( m_Type )->m_Flags & ClassSpec::FLAG_SINGLETON) );
	}
	return ( false );
}

bool TypeSpec :: ContentsAreSimple( void ) const
{
	if ( !IsUser( m_Type ) )
	{
		// special types are out of the question
		return ( !IsSpecial( m_Type ) );
	}

	const ClassSpec* cls = gFuBiSysExports.FindClass( m_Type );
	const ClassHeaderSpec* hdr = cls->m_HeaderSpec;
	if ( hdr != NULL )
	{
		HeaderSpec::ConstIter i, begin = hdr->GetColumnBegin(), end = hdr->GetColumnEnd();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !i->m_Type.IsPassByValue() || !i->m_Type.ContentsAreSimple() )
			{
				return ( false );
			}
		}
	}

	return ( true );
}

bool TypeSpec :: CanRPC( GPDEBUG_ONLY( gpstring* cantRpcReason ) ) const
{
	// strings ok
	if ( IsAString() || IsAStringW() )
	{
		return ( true );
	}

	// if user type, must allow rpc
	if ( IsUser( m_Type ) )
	{
		// pointer class? we're cool
		const ClassSpec* classSpec = gFuBiSysExports.FindClass( m_Type );
		if ( classSpec->m_Flags & ClassSpec::FLAG_POINTERCLASS )
		{
			gpassert( !IsPassByValue() );
			return ( true );
		}

		// if we know that we contain pointers (via schema) then NO WAY
		if ( !ContentsAreSimple() )
		{
#			if GP_DEBUG
			if ( cantRpcReason != NULL )
			{
				cantRpcReason->appendf( "contents of user-defined type '%s' are not 'simple'",
										classSpec->m_Name.c_str() );
			}
#			endif // GP_DEBUG
			return ( false );
		}

		// pod types are always cool
		const VarTypeSpec* spec = gFuBiSysExports.GetVarTypeSpec( m_Type );
		if ( (spec != NULL) && (spec->m_Flags & Trait::FLAG_POD) )
		{
			return ( true );
		}

		// pass-by-value is only allowed for pod types
		if ( IsPassByValue() )
		{
#			if GP_DEBUG
			if ( cantRpcReason != NULL )
			{
				cantRpcReason->appendf( "cannot pass non-POD type '%s' by value",
										classSpec->m_Name.c_str() );
			}
#			endif // GP_DEBUG
			return ( false );
		}

		// pass-by-reference only ok when the class itself can rpc
		if ( !(classSpec->m_Flags & ClassSpec::FLAG_CANRPC) )
		{
#			if GP_DEBUG
			if ( cantRpcReason != NULL )
			{
				cantRpcReason->appendf( "user-defined type is not a FuBi RPC class, cannot pass by reference",
										classSpec->m_Name.c_str() );
			}
#			endif // GP_DEBUG
			return ( false );
		}

		// must be ok
		return ( true );
	}

	// no pass-by-reference
	if ( !IsPassByValue() )
	{
#		if GP_DEBUG
		if ( cantRpcReason != NULL )
		{
			cantRpcReason->appendf( "cannot pass type '%s' by reference",
									gFuBiSysExports.FindTypeName( m_Type ) );
		}
#		endif // GP_DEBUG
		return ( false );
	}

	// must be ok
	return ( true );
}

bool TypeSpec :: CanPack( void ) const
{
	// currently all we support is these
	if ( (m_Type != VAR_MEM_PTR) && (m_Type != VAR_CONST_MEM_PTR) )
	{
		return ( false );
	}

	// pass-by-value only!
	return ( IsPassByValue() );
}

bool TypeSpec :: CanSkrit( void ) const
{
	// strings ok
	if ( IsAString() )
	{
		return ( true );
	}

	// can be user type only if not passing directly
	if ( IsUser( m_Type ) )
	{
		// if we know that we contain pointers (via schema) then NO WAY
		if ( !ContentsAreSimple() )
		{
			return ( false );
		}

		return ( !IsPassByValue() );
	}

	// cannot be anything big like a double
	if ( GetSizeBytes() > 4 )
	{
		return ( false );
	}

	// can be any other type only if passing directly
	return ( IsPassByValue() );
}

int TypeSpec :: GetSizeBytes ( void ) const
{
	// have to calc it...
	if ( IsPassByValue() && !IsEnum( m_Type ) )
	{
		int size = gFuBiSysExports.GetVarTypeSizeBytes( m_Type );
		return ( GetDwordAlignUp( size ) );
	}
	else
	{
		// default size for all pointers, references, handles
		return ( sizeof( DWORD ) );
	}
}

TypeSpec TypeSpec :: GetBaseType( void ) const
{
	TypeSpec spec( *this );

	// mask off unimportant bits
	spec.m_Flags &= NOT( FLAG_CONST );

	// convert references to pointers (treated the same internally)
	if ( spec.m_Flags & FLAG_REFERENCE )
	{
		spec.m_Flags |=      FLAG_POINTER;
		spec.m_Flags &= NOT( FLAG_REFERENCE );
	}
	return ( spec );
}

#if !GP_RETAIL

void TypeSpec :: GenerateDocs( ReportSys::ContextRef context ) const
{
	ReportSys::OutputF( context, "%s%s%s%s%s",
			(m_Flags & FLAG_CONST    ) ? "const " : "",
			(m_Flags & FLAG_HANDLE   ) ? "H"      : "",
			gFuBiSysExports.FindTypeName( m_Type ),
			(m_Flags & FLAG_POINTER  ) ? "*"      : "",
			(m_Flags & FLAG_REFERENCE) ? "&"      : "" );
}

#endif // !GP_RETAIL

TypeSpec::eCompare TestCompatibility( const TypeSpec& tfrom, const TypeSpec& tto )
{
	// $$ this function should be optimized with a LUT

	// convert to base types for comparison
	TypeSpec from = tfrom.GetBaseType();
	TypeSpec to   = tto  .GetBaseType();

	// test for exact match
	if ( from == to )
	{
		return ( TypeSpec::COMPARE_EQUAL );
	}

	// if one is a user type and the other is not, well...
	bool luser = IsUser( from.m_Type ), ruser = IsUser( to.m_Type );
	if ( luser != ruser )
	{
		return ( TypeSpec::COMPARE_INCOMPATIBLE );
	}

	// split based on type
	if ( luser )
	{
		// if not the same type and not a derivative, can't do it
		if ( (from.m_Type != to.m_Type) && !gFuBiSysExports.IsDerivative( from.m_Type, to.m_Type ) )
		{
			return ( TypeSpec::COMPARE_INCOMPATIBLE );
		}

		// handle-to-pointer conversion is the only thing we can do here assuming they're the same eVarType
		if (   (   (from.m_Flags & TypeSpec::FLAG_HANDLE)
				|| (from.m_Flags & TypeSpec::FLAG_POINTER))
			&& (to.m_Flags & TypeSpec::FLAG_POINTER) )
		{
			return ( TypeSpec::COMPARE_COMPATIBLE );
		}
	}
	else
	{
		bool li = from.IsSimpleInteger();
		bool ri = to  .IsSimpleInteger();

		// test for int-to-bool
		if ( li && (to.m_Type == VAR_BOOL) )
		{
			return ( TypeSpec::COMPARE_COMPATIBLE );
		}

		// test for implicit promotions
		if ( li && ri )
		{
			return ( TypeSpec::COMPARE_PROMOTABLE );
		}

		// test for float-bool conversions (bad)
		if (   ((to  .m_Type == VAR_FLOAT) && (from.m_Type == VAR_BOOL))
			|| ((from.m_Type == VAR_FLOAT) && (to  .m_Type == VAR_BOOL)) )
		{
			return ( TypeSpec::COMPARE_INCOMPATIBLE );
		}

		// test for float-or-bool/int conversions
		if (   (li && ((to  .m_Type == VAR_FLOAT) || (to  .m_Type == VAR_BOOL)))
			|| (ri && ((from.m_Type == VAR_FLOAT) || (from.m_Type == VAR_BOOL))) )
		{
			return ( TypeSpec::COMPARE_COMPATIBLE );
		}

		// test for string-string conversions
		if ( to.IsAString() && from.IsAString() )
		{
			return ( TypeSpec::COMPARE_COMPATIBLE );
		}
	}

	// default - can't do it
	return ( TypeSpec::COMPARE_INCOMPATIBLE );
}

//////////////////////////////////////////////////////////////////////////////
// struct ConstrainEnumStrings implementation

#if !GP_RETAIL

ConstrainEnumStrings :: ConstrainEnumStrings( eVarType enumType )
	: m_EnumType( enumType )
{
	gpassert( IsEnum( enumType ) );
	gpassert( gFuBiSysExports.FindEnum( m_EnumType )->m_Exporter.IsContinuous() );
}

int ConstrainEnumStrings :: OnGetStrings( ConstrainStrings::StringColl& strings )
{
	const EnumExporter& spec = gFuBiSysExports.FindEnum( m_EnumType )->m_Exporter;
	for ( DWORD i = spec.m_Begin, end = i + spec.m_Count ; i != end ; ++i )
	{
		if ( spec.m_EnumToFullStringProc != NULL )
		{
			strings.push_back( gpstring( (*spec.m_EnumToFullStringProc)( i ) ) );
		}
		else
		{
			gpassert( spec.m_EnumToStringProc != NULL );
			strings.push_back( gpstring( (*spec.m_EnumToStringProc)( i ) ) );
		}
	}

	return ( spec.m_Count );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ParamSpec implementation

ParamSpec :: ParamSpec( const ParamSpec& other )
	: m_Type( other.m_Type ), m_Extra( NULL )
{
	if ( other.m_Extra != NULL )
	{
		*GetExtra() = *other.m_Extra;
	}
}

ParamSpec& ParamSpec :: operator = ( const ParamSpec& other )
{
	m_Type = other.m_Type;
	if ( other.m_Extra != NULL )
	{
		*GetExtra() = *other.m_Extra;
	}
	else
	{
		Delete( m_Extra );
	}
	return ( *this );
}

gpstring ParamSpec :: GetDefaultValue( void ) const
{
	gpstring defValue;

	if ( m_Extra != NULL )
	{
		defValue = m_Extra->m_DefaultValue;
	}

	return ( defValue );
}

void ParamSpec :: SetDefaultValue( const gpstring& defValue, bool isCode )
{
	BuildExtra();

	m_Extra->m_DefaultValue = defValue;
	m_Extra->m_DefaultIsCode = isCode;
}

void ParamSpec :: SetNoDefaultValue( void )
{
	BuildExtra();
	m_Extra->m_DefaultValue.clear();
	m_Extra->m_DefaultIsCode = false;
}

DWORD ParamSpec :: CalcDigest( void ) const
{
	UINT32 digest = 0;
	if ( m_Extra != NULL )
	{
		digest = GetCRC32( digest, GetName().c_str(), GetName().length() + 1 );
	}
	AddCRC32( digest, m_Type );

	// $$$ include default value

	return ( digest );
}

#if !GP_RETAIL

void ParamSpec :: SetConstraint( ConstraintSpec* spec )
{
	BuildExtra();
	Delete ( m_Extra->m_ConstraintSpec );
	m_Extra->m_ConstraintSpec = spec;
}

const ConstraintSpec* ParamSpec :: GetConstraint( void ) const
{
	return ( m_Extra ? m_Extra->m_ConstraintSpec : NULL );
}

void ParamSpec :: GenerateDocs( ReportSys::ContextRef context, eDocsLevel /*level*/ ) const
{
	m_Type.GenerateDocs( context );

	if ( (m_Extra != NULL) && !m_Extra->m_Name.empty() )
	{
		ReportSys::Output( context, " " );
		ReportSys::Output( context, m_Extra->m_Name );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ParamSpec::Extra implementation

ParamSpec::Extra :: Extra( void )
{
	m_DefaultIsCode  = false;
	m_NoXfer         = false;
#	if !GP_RETAIL
	m_ConstraintSpec = NULL;
#	endif // !GP_RETAIL
}

ParamSpec::Extra :: ~Extra( void )
{
#	if !GP_RETAIL
	delete ( m_ConstraintSpec );
#	endif // !GP_RETAIL
}

ParamSpec::Extra& ParamSpec::Extra :: operator = ( const Extra& other )
{
	m_Name          = other.m_Name;
	m_DefaultValue  = other.m_DefaultValue;
	m_DefaultIsCode = other.m_DefaultIsCode;

#	if !GP_RETAIL
	Delete ( m_ConstraintSpec );
	if ( other.m_ConstraintSpec != NULL )
	{
		m_ConstraintSpec = other.m_ConstraintSpec->Clone();
	}
#	endif // !GP_RETAIL

	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// struct FunctionSpec implementation

const FunctionSpec::eFlags FunctionSpec::DEFAULT_MEMBERSHIP = FLAG_MEMBER_OF_GAME | FLAG_MEMBER_OF_CONSOLE;

FunctionSpec::eFlags FunctionSpec :: ToFlags( FunctionSpecFlags::ePermissions bit )
{
	using namespace FunctionSpecFlags;

	switch ( bit )
	{
		case ( SERVER  ):  return ( FLAG_CHECK_SERVER_ONLY );
		case ( DEV     ):  return ( FLAG_DEV_ONLY          );
		case ( RETRY   ):  return ( FLAG_RETRY             );
	}

	gpassert( 0 );				// hmm??
	return ( FLAG_NONE );
}

FunctionSpec::eFlags FunctionSpec :: ToFlags( FunctionSpecFlags::eMembership bit )
{
	using namespace FunctionSpecFlags;

	switch ( bit )
	{
		case ( ALL     ):  return ( FLAG_MEMBER_OF_ALL     );
		case ( GAME    ):  return ( FLAG_MEMBER_OF_GAME    );
		case ( CONSOLE ):  return ( FLAG_MEMBER_OF_CONSOLE );
		case ( EDITOR  ):  return ( FLAG_MEMBER_OF_EDITOR  );
		case ( TRIAL   ):  return ( FLAG_MEMBER_OF_TRIAL   );
	}

	gpassert( 0 );				// hmm??
	return ( FLAG_NONE );
}

bool FunctionSpec :: TestMembership( eFlags flags, FunctionSpecFlags::eMembership bit )
{
	return ( (bit == FunctionSpecFlags::ALL) || !!(flags & ToFlags( bit )) );
}

FunctionSpec :: FunctionSpec( void )
{
	m_Parent         = NULL;
	m_Flags          = DEFAULT_MEMBERSHIP;
	m_FunctionPtr    = 0;
	m_SerialID       = INVALID_FUNCTION_SERIAL;
	m_ParamSizeBytes = 0;
	m_ReturnType     = VAR_UNKNOWN;

#	if !GP_RETAIL
	m_Docs           = NULL;
	m_MangledName    = NULL;
#	endif // !GP_RETAIL
}

bool FunctionSpec :: IsValidGet( void ) const
{
	GPDEBUG_ONLY( AssertValid() );

	// must be of form: Type Class::GetVariable( void )

	// member of a class
	if ( m_Parent == NULL )
	{
		return ( false );
	}

	// no params
	if ( !m_ParamSpecs.empty() )
	{
		return ( false );
	}

	// non-void return
	if ( m_ReturnType == VAR_VOID )
	{
		return ( false );
	}

	// proper naming, at least GetX
	if ( m_Name.length() < 4 )
	{
		return ( false );
	}

	// prefix of "get"
	if (   (tolower( m_Name[ 0 ] ) != 'g')
		|| (tolower( m_Name[ 1 ] ) != 'e')
		|| (tolower( m_Name[ 2 ] ) != 't') )
	{
		return ( false );
	}

	// good enough
	return ( true );
}

bool FunctionSpec :: IsValidSet( void ) const
{
	GPDEBUG_ONLY( AssertValid() );

	// must be of form: void Class::SetVariable( Type )

	// member of a class
	if ( m_Parent == NULL )
	{
		return ( false );
	}

	// one param
	if ( m_ParamSpecs.size() != 1 )
	{
		return ( false );
	}

	// void return
	if ( m_ReturnType != VAR_VOID )
	{
		return ( false );
	}

	// proper naming, at least SetX
	if ( m_Name.length() < 4 )
	{
		return ( false );
	}

	// prefix of "set"
	if (   (tolower( m_Name[ 0 ] ) != 's')
		|| (tolower( m_Name[ 1 ] ) != 'e')
		|| (tolower( m_Name[ 2 ] ) != 't') )
	{
		return ( false );
	}

	// good enough
	return ( true );
}

bool FunctionSpec :: GetRpcRequiresObjectParam( void ) const
{
	if ( !(m_Flags & FLAG_CALL_THISCALL) )
	{
		return ( false );
	}

	gpassert( (m_Parent != NULL) && (m_Parent->m_Flags & ClassSpec::FLAG_CANRPC) );
	return ( !(m_Parent->m_Flags & ClassSpec::FLAG_SINGLETON) );
}

#if GP_DEBUG
void RpcError( const FunctionSpec* spec, bool& badRpc, const gpstring& reason )
{
	ReportSys::AutoReport autoReport( &gErrorContext );
	if ( !badRpc )
	{
		gperrorf(( "FuBi RPC function '%s' cannot RPC, reasons:\n",
				   spec->BuildQualifiedName().c_str() ));
	}
	ReportSys::AutoIndent autoIndent( &gErrorContext );
	gperrorf(( "%s\n", reason.c_str() ));
}
#endif // GP_DEBUG

void FunctionSpec :: PostProcess( void )
{
	// special: check to see if it's *supposed* to be an rpc function. if true,
	// then verify that it can RPC, otherwise verify that it can Skrit.
#	if GP_DEBUG

	bool targetRpc = false;
	bool badRpc = false;

	{
		// check up to FUBI_RPC_MAX_SEARCH bytes or the end of the function,
		// or the next page boundary if function end not known (whichever comes
		// first), looking for the special tag FUBI_EMBEDDED_RPC_TAG.
		int remaining = gFuBiSysExports.GetFunctionSize( this );
		if ( remaining < 0 )
		{
			remaining = GetAlignUp( m_FunctionPtr, SysInfo::GetSystemPageSize() ) - m_FunctionPtr;
		}
		int maxSearch = min( FUBI_RPC_MAX_SEARCH, remaining );
		int tagSize   = ::strlen( (const char*)FUBI_EMBEDDED_RPC_TAG );

		// get iterators
		const BYTE* i,
				  * begin = (const BYTE*)m_FunctionPtr,
				  * end   = begin + maxSearch;
		for ( i = begin ; i != end ; ++i )
		{
			int searchLen = (int)(end - i) - (tagSize - 1);
			if ( searchLen < 1 )
			{
				break;
			}
			i = (const BYTE*)::memchr( i, FUBI_EMBEDDED_RPC_TAG[ 0 ], searchLen );
			if ( i == NULL )
			{
				break;
			}
			if ( ::memcmp( i, FUBI_EMBEDDED_RPC_TAG, tagSize ) == 0 )
			{
				targetRpc = true;
				break;
			}
		}
	}

#	endif // GP_DEBUG

	// get some iterators we're going to use over and over
	ParamSpecs::iterator i, begin = m_ParamSpecs.begin(), end = m_ParamSpecs.end();

	// check to see if function is a simple type (meaning that no parameters
	// will require any special handling).
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->m_Type.IsSimple() )
		{
			break;
		}
	}
	if ( i == end )
	{
		m_Flags |= FLAG_SIMPLE_ARGS;
	}

	// strip off const on pass-by-value
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Type.m_Flags == TypeSpec::FLAG_CONST )
		{
			i->m_Type.m_Flags = TypeSpec::FLAG_NONE;
		}
	}
	if ( m_ReturnType.m_Flags == TypeSpec::FLAG_CONST )
	{
		m_ReturnType.m_Flags = TypeSpec::FLAG_NONE;
	}

	// check for illegal types
#	if GP_DEBUG
	for ( i = begin ; i != end ; ++i )
	{
		gpassertm( i->m_Type.m_Flags != TypeSpec::FLAG_POINTER_POINTER, "Pointer-pointer (int** etc) parameters not supported" );
	}
#	endif // GP_DEBUG

// Check for RPC capability.

	// cannot wait for return value
	if ( (m_ReturnType != VAR_VOID) && !m_ReturnType.IsFuBiCookie() )
	{
#		if GP_DEBUG
		if ( targetRpc )
		{
			RpcError( this, badRpc, gpstringf(
					"return types are not allowed except for RETRY functions, "
					"which must return a FuBiCookie" ) );
		}
		badRpc = true;
#		else // GP_DEBUG
		goto DoneRPC;
#		endif // GP_DEBUG
	}

	// not supporting variable arguments
	if ( m_Flags & FLAG_VARARG )
	{
#		if GP_DEBUG
		if ( targetRpc )
		{
			RpcError( this, badRpc, gpstringf(
					"variable argument functions not supported" ) );
		}
		badRpc = true;
#		else // GP_DEBUG
		goto DoneRPC;
#		endif // GP_DEBUG
	}

	// member methods are ok, but only if class is rpc'able
	if ( (m_Flags & FLAG_CALL_THISCALL) && !(m_Parent->m_Flags & ClassSpec::FLAG_CANRPC) )
	{
#		if GP_DEBUG
		if ( targetRpc )
		{
			RpcError( this, badRpc, gpstringf(
					"function is a method of class '%s' that is not a FuBi RPC class",
					m_Parent->m_Name.c_str() ) );
		}
		badRpc = true;
#		else // GP_DEBUG
		goto DoneRPC;
#		endif // GP_DEBUG
	}

	// check all types for RPC capability
	for ( i = begin ; i != end ; ++i )
	{
#		if GP_DEBUG
		gpstring reason;
#		endif // GP_DEBUG

		if ( !i->m_Type.CanRPC( GPDEBUG_ONLY( targetRpc ? &reason : NULL ) ) )
		{
#			if GP_DEBUG
			if ( targetRpc )
			{
				RpcError( this, badRpc, gpstringf(
						"parameter %d cannot be passed over the network, "
						"reason: '%s'", i - begin, reason.c_str() ) );
			}
			badRpc = true;
#			else // GP_DEBUG
			goto DoneRPC;
#			endif // GP_DEBUG
		}
	}

#	if GP_DEBUG
	if ( badRpc )
	{
		goto DoneRPC;
	}
#	endif // GP_DEBUG

	// cool, we got it
	m_Flags |= FLAG_CAN_RPC;
	if ( m_ReturnType.IsFuBiCookie() )
	{
		m_Flags |= FLAG_RETRY;
	}

	// now check for packability
	if ( begin != end )
	{
		ParamSpecs::iterator localBegin = begin, localEnd = end;

		bool shouldCheck = false;
		if ( localBegin->m_Type.CanPack() )
		{
			m_Flags |= FLAG_CAN_PACK_FRONT;
			++localBegin;
			shouldCheck = true;
		}
		else if ( localEnd[ -1 ].m_Type.CanPack() )
		{
			m_Flags |= FLAG_CAN_PACK_BACK;
			--localEnd;
			shouldCheck = true;
		}

		// pack is an option - check to make sure the rest are simple for it to work
		if ( shouldCheck )
		{
			for ( i = localBegin ; i != localEnd ; ++i )
			{
				if ( !i->m_Type.IsSimple() )
				{
					// can't do it!!!
					m_Flags &= NOT( FLAG_CAN_PACK_FRONT | FLAG_CAN_PACK_BACK );
					break;
				}
			}
		}

	}

DoneRPC:

	// floats and doubles are the same on a return type (ST0)
	if ( m_ReturnType == VAR_DOUBLE )
	{
		m_ReturnType = VAR_FLOAT;
	}

	// check for skrit capability
	if ( m_ReturnType.CanSkrit() )
	{
		for ( i = begin ; i != end ; ++i )
		{
			if ( !i->m_Type.CanSkrit() )
			{
				break;
			}
		}

		if ( i == end )
		{
			m_Flags |= FLAG_CAN_SKRIT;
		}
	}

	// add up parameter sizes - only needed for rpc and skrit btw
	m_ParamSizeBytes = 0;
	if ( m_Flags & (FLAG_CAN_SKRIT | FLAG_CAN_RPC) )
	{
		for ( i = begin ; i != end ; ++i )
		{
			m_ParamSizeBytes += i->m_Type.GetSizeBytes();
		}
	}

	// check for namespace globals and make them statics (for convenience)
	if ( (m_Parent != NULL) && !(m_Flags & FLAG_CALL_THISCALL) && !(m_Flags & FLAG_STATIC_MEMBER) )
	{
		// $ this will appear as nonstatic, non-thiscall function. it will in
		//   reality be a global, not a member function (requiring "this").
		//   coincidentally, this signature is the same as a nonstatic, vararg
		//   member function. so to disambiguate, always assume that vararg
		//   functions at this point are members. there's no real way to tell
		//   otherwise from the mangled name.

		if ( !(m_Flags & FLAG_VARARG) )
		{
			m_Flags |= FLAG_STATIC_MEMBER;
		}
	}

#	if !GP_RETAIL

	// check for names that are the same as Skrit keywords - not possible
	if ( gFuBiSysExports.IsKeyword( m_Name ) )
	{
		gperrorf(( "FUBI function name '%s' uses a keyword\n", BuildQualifiedName().c_str() ));
	}

	// check for returning or taking a singleton class pointer - should not be
	//  passing around pointers to singletons.
	if ( m_ReturnType.IsSingleton() )
	{
		gperrorf(( "FUBI function '%s' returns a singleton class!\n", BuildQualifiedName().c_str() ));
	}
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Type.IsSingleton() )
		{
			gperrorf(( "FUBI function '%s' takes a singleton class as parameter #%d!\n", BuildQualifiedName().c_str(), i - begin ));
		}
	}

#	endif // !GP_RETAIL
}

gpstring FunctionSpec :: BuildQualifiedName( void ) const
{
	if ( m_Parent != NULL )
	{
		gpstring name( m_Parent->m_Name );
		name += "::";
		name += m_Name;
		return ( name );
	}
	else
	{
		return ( m_Name );
	}
}

#if !GP_RETAIL

typedef std::map <eVarType, FunctionSpec::PodTranslateCb> PodTranslateDb;
static PodTranslateDb s_PodTranslateDb;

typedef std::map <UINT, FunctionSpec::MemTranslateCb> MemTranslateDb;
static MemTranslateDb s_MemTranslateDb;

gpstring FunctionSpec :: BuildQualifiedNameAndParams( const_mem_ptr params, const void* object, bool isRpcPacked, bool allowBinaryOut, int skipParams ) const
{
	gpstring out;
	const void* param = params.mem;
	const BYTE* paramIter = (const BYTE*)param + m_ParamSizeBytes;

	// first, the function name
	out += BuildQualifiedName();

	// do any pack work
	eFlags packType = FLAG_NONE;
	const_mem_ptr packMem;
	if ( isRpcPacked )
	{
		// validate incoming data, must at least be size of serial ID even
		// though the params don't actually include the serial ID...
		gpassert( params.size >= sizeof( WORD ) );

		// figure out pack type and memory size
		packType = m_Flags & (FLAG_CAN_PACK_FRONT | FLAG_CAN_PACK_BACK);
		if ( packType != FLAG_NONE )
		{
			int memSize = params.size - sizeof( WORD ) - (m_ParamSizeBytes - sizeof( const_mem_ptr ));
			if ( GetRpcRequiresObjectParam() )
			{
				memSize -= sizeof( DWORD );
			}

			// take param
			packMem = const_mem_ptr( param, memSize );

			// skip over the packed memory
			param = (const BYTE*)param + memSize;
			paramIter = (const BYTE*)param + m_ParamSizeBytes - sizeof( const_mem_ptr );
		}
	}

	// get translator if needed
	MemTranslateDb::const_iterator memTrans = s_MemTranslateDb.find( m_SerialID );

	// get param iters
	ParamSpecs::const_iterator i, ibegin = m_ParamSpecs.begin(), iend = m_ParamSpecs.end();
	ibegin += skipParams;

	// next, the params
	int memParamIndex = 0;
	out += "(";
	if ( ibegin != iend )
	{
		out += " ";
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i != ibegin )
			{
				out += ", ";
			}

			const_mem_ptr mem;
			eXfer xfer = IsUnsignedInt( i->m_Type.m_Type ) ? XFER_HEX : XFER_NORMAL;
			bool advance = true;
			bool isMemParam = false;

			if ( (i->m_Type.m_Type == VAR_MEM_PTR) || (i->m_Type.m_Type == VAR_CONST_MEM_PTR) )
			{
				const const_mem_ptr* pmem = (const const_mem_ptr*)param;

				// redirect?
				if (   ((packType == FLAG_CAN_PACK_FRONT) && (i == ibegin))
					|| ((packType == FLAG_CAN_PACK_BACK ) && (i == (iend - 1))) )
				{
					pmem = &packMem;
					advance = false;
				}

				if ( allowBinaryOut || (memTrans != s_MemTranslateDb.end()) )
				{
					mem = *pmem;
					if ( isRpcPacked && advance )
					{
						mem.mem = paramIter;
					}
				}
				else if ( pmem->size > 0 )
				{
					out.appendf( "mem:%d", pmem->size );
				}
				else
				{
					out.append( "mem:null" );
				}

				isMemParam = true;
				if ( advance )
				{
					paramIter += pmem->size;
				}
			}
			else if ( i->m_Type.IsAString() || i->m_Type.IsAStringW() )
			{
				bool wideChar = i->m_Type.IsAStringW();
				const void* str = NULL;
				DWORD size = 0;

				// figure out where our string is and how big it is
				if ( isRpcPacked )
				{
					str = paramIter;
					size = *rcast <const DWORD*> ( param );
				}
				else
				{
					// reprocess if actually a string object pointer
					if ( i->m_Type.IsString() && (param != NULL) )
					{
						str = (*(const gpstring**)param)->c_str();
					}
					else if ( i->m_Type.IsStringW() && (param != NULL) )
					{
						str = (*(const gpwstring**)param)->c_str();
					}
					else
					{
						str = *(const BYTE**)param;
					}

					// how big?
					if ( str != NULL )
					{
						size = wideChar ? ::wcslen( (const wchar_t*)str ) : ::strlen( (const char*)str );
						++size;
					}
				}

				// output
				if ( size == 0 )
				{
					out += "[null]";
				}
				else
				{
					out.appendf( wideChar ? "\"%.*S\"" : "\"%.*s\"", size - 1, str );
				}

				// advance
				paramIter += size * (wideChar ? 2 : 1);
			}
			else if ( i->m_Type.IsPassByValue() || i->m_Type.IsPointerClass() )
			{
				gpstring paramText;
				if ( gFuBiSysExports.ToString( i->m_Type, paramText, param, xfer, XMODE_QUIET ) )
				{
					out += paramText;
				}
				else
				{
					mem.mem = param;
					mem.size = i->m_Type.GetSizeBytes();

					PodTranslateDb::const_iterator found = s_PodTranslateDb.find( i->m_Type.m_Type );
					if ( found != s_PodTranslateDb.end() )
					{
						out.append( "{ " );
						found->second( out, i->m_Type.m_Type, mem );
						out.append( " }" );
						mem = const_mem_ptr();
					}
					else if ( !allowBinaryOut )
					{
						out.appendf( "pod:%d", i->m_Type.GetSizeBytes() );
						mem = const_mem_ptr();
					}
				}
			}
			else
			{
				if ( gFuBiSysExports.IsPod( i->m_Type.m_Type ) )
				{
					const void* obj = isRpcPacked ? paramIter : *(const BYTE**)param;

					TypeSpec localType( i->m_Type.m_Type );
					int size = gFuBiSysExports.GetVarTypeSizeBytes( localType.m_Type );
					gpstring paramText;
					if ( gFuBiSysExports.ToString( localType, paramText, obj, xfer, XMODE_QUIET ) )
					{
						out.appendf( "[%s]", paramText.c_str() );
					}
					else
					{
						mem.mem = obj;
						mem.size = size;

						PodTranslateDb::const_iterator found = s_PodTranslateDb.find( i->m_Type.m_Type );
						if ( found != s_PodTranslateDb.end() )
						{
							out.append( "[{ " );
							found->second( out, i->m_Type.m_Type, mem );
							out.append( " }]" );
							mem = const_mem_ptr();
						}
						else if ( !allowBinaryOut )
						{
							out.appendf( "[pod:%d]", i->m_Type.GetSizeBytes() );
							mem = const_mem_ptr();
						}
					}

					paramIter += size;
				}
				else if ( isRpcPacked )
				{
					// this is a cookie
					out.appendf( "0x%08X", *(DWORD*)param );
				}
				else if ( *(DWORD*)param == NULL )
				{
					out.append( "<null>" );
				}
				else
				{
					// does it have a cookie form?
					const ClassSpec* classSpec = gFuBiSysExports.FindClass( i->m_Type.m_Type );
					gpassert( classSpec != NULL );
					if ( classSpec->m_Flags & ClassSpec::FLAG_CANRPC )
					{
						gpassert( classSpec->m_InstanceToNetProc );
						DWORD cookie = (*classSpec->m_InstanceToNetProc)( (void*)*(DWORD*)param );
						out.appendf( cookie > 0xFFFF ? "0x%08X" : "0x%X", cookie );
					}
					else
					{
						out.appendf( "0x%08X", *(DWORD*)param );
					}
				}
			}

			if ( mem )
			{
				// see if we have any mem translators
				if ( isMemParam && (memTrans != s_MemTranslateDb.end()) )
				{
					out.append( "[{ " );
					memTrans->second( out, memParamIndex, mem );
					out.append( " }]" );
				}
				else
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

				// now we can advance this index
				if ( isMemParam )
				{
					++memParamIndex;
				}
			}

			if ( advance )
			{
				param = (const BYTE*)param + i->m_Type.GetSizeBytes();
			}
		}
		out += " ";
	}
	out += ")";

	// finally, the "this" value
	if ( (m_Flags & FLAG_CALL_THISCALL) && (m_Parent != NULL) && !(m_Parent->m_Flags & ClassSpec::FLAG_SINGLETON) )
	{
		// use ptr if passed in, otherwise it's at the end of the packet
		DWORD thisPtr = (DWORD)object;
		if ( thisPtr == NULL )
		{
			thisPtr = *(const DWORD*)paramIter;
		}

		// not a cookie? convert to one for pretty print
		if ( !isRpcPacked )
		{
			// does it have a cookie form?
			const ClassSpec* classSpec = gFuBiSysExports.FindClass( m_Parent->m_Type );
			gpassert( classSpec != NULL );
			if ( classSpec->m_Flags & ClassSpec::FLAG_CANRPC )
			{
				gpassert( classSpec->m_InstanceToNetProc );
				thisPtr = (*classSpec->m_InstanceToNetProc)( (void*)thisPtr );
			}
		}

		// output it
		out.insertf( 0, (thisPtr > 0xFFFF) ? "0x%08X." : "0x%X.", thisPtr );
	}

    // finally add the serial id (so I can correlate w/ netmon captures)
    out.appendf( " [0x%04x]", m_SerialID );

	return ( out );
}

void FunctionSpec :: AssertValid( void ) const
{
	// must have a name
	gpassert( !m_Name.empty() );

	// must have exactly one of these bits set
	gpassert( IsPower2( m_Flags & (  FLAG_CALL_CDECL
								   | FLAG_CALL_FASTCALL
								   | FLAG_CALL_STDCALL
								   | FLAG_CALL_THISCALL) ) );

	// must have a non-NULL function pointer
	gpassert( m_FunctionPtr != NULL );

	// must have a valid return type
	gpassert( m_ReturnType != VAR_UNKNOWN );

	// all params must be of non-void type
	gpassert( stdx::find_const( m_ParamSpecs, VAR_VOID ) == m_ParamSpecs.end() );

	// can't have both front and back pack-able
	gpassert( !(m_Flags & FLAG_CAN_PACK_FRONT) || !(m_Flags & FLAG_CAN_PACK_BACK) );

	// can't be packable and simple at the same time
	gpassert( !(m_Flags & (FLAG_CAN_PACK_FRONT | FLAG_CAN_PACK_BACK)) || !(m_Flags & FLAG_SIMPLE_ARGS) );
}

void FunctionSpec :: GenerateDocs( ReportSys::ContextRef context, eDocsLevel level ) const
{
	GPDEBUG_ONLY( AssertValid() );

	// doco type
	if ( level > DOCS_MINIMAL )
	{
		ReportSys::Output( context, "Function: " );
	}

	// possible static function
	if ( m_Flags & FLAG_STATIC_MEMBER )
	{
		ReportSys::Output( context, "static " );
	}

	// return type
	m_ReturnType.GenerateDocs( context );
	ReportSys::Output( context, " " );

	// function name
	if ( (level > DOCS_MINIMAL) && (m_Parent != NULL) )
	{
		ReportSys::Output( context, m_Parent->m_Name );
		ReportSys::Output( context, "::" );
	}
	ReportSys::Output( context, m_Name );
	ReportSys::Output( context, "( " );

	// parameter list
	if ( m_ParamSpecs.empty() && !(m_Flags & FLAG_VARARG) )
	{
		ReportSys::Output( context, "void" );
	}
	else
	{
		// params
		ParamSpecs::const_iterator i, begin = m_ParamSpecs.begin(), end = m_ParamSpecs.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( i != begin )
			{
				ReportSys::Output( context, ", " );
			}

			i->m_Type.GenerateDocs( context );
			if ( (i->m_Extra != NULL) && !i->m_Extra->m_Name.empty() )
			{
				ReportSys::Output( context, " " );
				ReportSys::Output( context, i->m_Extra->m_Name );
			}

		}

		// handle vararg
		if ( m_Flags & FLAG_VARARG )
		{
			if ( !m_ParamSpecs.empty() )
			{
				ReportSys::Output( context, ", " );
			}
			ReportSys::Output( context, "..." );
		}
	}
	ReportSys::Output( context, " )" );

	if (   m_Flags & FLAG_CONST              )  ReportSys::Output( context, " const"          );
	if ( !(m_Flags & FLAG_SIMPLE_ARGS)       )  ReportSys::Output( context, " [COMPLEX]"      );
	if (   m_Flags & FLAG_CAN_RPC            )  ReportSys::Output( context, " [RPC]"          );
	if ( !(m_Flags & FLAG_CAN_SKRIT  )       )  ReportSys::Output( context, " [!SKRIT]"       );
	if (   m_Flags & FLAG_CAN_PACK_FRONT     )  ReportSys::Output( context, " [PACK_FRONT]"   );
	if (   m_Flags & FLAG_CAN_PACK_BACK      )  ReportSys::Output( context, " [PACK_BACK]"    );
	if (   m_Flags & FLAG_HIDDEN             )  ReportSys::Output( context, " [HIDDEN]"       );
	if (   m_Flags & FLAG_CHECK_SERVER_ONLY  )  ReportSys::Output( context, " [CHECK_SERVER]" );
	if (   m_Flags & FLAG_DEV_ONLY           )  ReportSys::Output( context, " [DEV]"          );
	if (   m_Flags & FLAG_RETRY              )  ReportSys::Output( context, " [RETRY]"        );

	// docs
	if ( level == DOCS_MINIMAL )
	{
		if ( (m_Docs != NULL) && (*m_Docs != '\0') )
		{
			ReportSys::Output( context, " /* " );
			ReportSys::Output( context, m_Docs );
			ReportSys::Output( context, " */"  );
		}
	}
	else if ( level > DOCS_MINIMAL )
	{
		ReportSys::Output( context, "\n\n" );
		if ( (m_Docs != NULL) && (*m_Docs != '\0') )
		{
			ReportSys::Output( context, m_Docs );
			ReportSys::Output( context, "\n\n" );
		}
	}
}

void FunctionSpec :: RegisterPodTranslateCb( eVarType type, PodTranslateCb cb )
{
	gpassert( type != VAR_UNKNOWN );
	gpassert( SysExports::DoesSingletonExist() && (gFuBiSysExports.FindTypeName( type ) != NULL) );
	s_PodTranslateDb[ type ] = cb;
}

void FunctionSpec :: UnregisterPodTranslateCb( eVarType type )
{
	PodTranslateDb::iterator found = s_PodTranslateDb.find( type );
	if ( found != s_PodTranslateDb.end() )
	{
		s_PodTranslateDb.erase( found );
	}
}

void FunctionSpec :: RegisterMemTranslateCb( UINT serialId, MemTranslateCb cb )
{
	gpassert( SysExports::DoesSingletonExist() && (gFuBiSysExports.FindFunctionBySerialID( serialId ) != NULL) );
	s_MemTranslateDb[ serialId ] = cb;
}

void FunctionSpec :: UnregisterMemTranslateCb( UINT serialId )
{
	MemTranslateDb::iterator found = s_MemTranslateDb.find( serialId );
	if ( found != s_MemTranslateDb.end() )
	{
		s_MemTranslateDb.erase( found );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct VariableSpec implementation

#if !GP_RETAIL

void VariableSpec :: AssertValid( void ) const
{
	// must have a name
	gpassert( !m_Name.empty() );

	// must have a parent class (no global var exports supported)
	gpassert( m_Parent != NULL );

	// must have a valid type
	gpassert( m_Type != VAR_UNKNOWN );

	// must have at least one non-NULL function
	gpassert( (m_GetMethod != NULL) || (m_SetMethod != NULL) );
}

void VariableSpec :: GenerateDocs( ReportSys::ContextRef context, eDocsLevel level ) const
{
	GPDEBUG_ONLY( AssertValid() );

	// doco type
	if ( level > DOCS_MINIMAL )
	{
		ReportSys::Output( context, "Variable: " );
	}

	// variable type
	m_Type.GenerateDocs( context );
	ReportSys::Output( context, " " );

	// variable name
	if ( (level > DOCS_MINIMAL) && (m_Parent != NULL) )
	{
		ReportSys::Output( context, m_Parent->m_Name );
		ReportSys::Output( context, "::" );
	}
	ReportSys::Output( context, m_Name );

	// docs
	if ( level == DOCS_MINIMAL )
	{
		if ( (m_Docs != NULL) && (*m_Docs != '\0') )
		{
			ReportSys::Output( context, " /* " );
			ReportSys::Output( context, m_Docs );
			ReportSys::Output( context, " */"  );
		}
	}
	else if ( level > DOCS_MINIMAL )
	{
		ReportSys::Output( context, "\n\n" );
		if ( (m_Docs != NULL) && (*m_Docs != '\0') )
		{
			ReportSys::Output( context, m_Docs );
			ReportSys::Output( context, "\n\n" );
		}
	}
}

#endif // !GP_RETAIL

bool VariableSpec :: IsMatched( const FunctionSpec* get, const FunctionSpec* set )
{
	gpassert( (get != NULL) && get->IsValidGet() );
	gpassert( (set != NULL) && set->IsValidSet() );

	// param of one must match the return of the other
	if ( get->m_ReturnType.GetBaseType() != set->m_ParamSpecs[0].m_Type.GetBaseType() )
	{
		return ( false );
	}

	// classes must match
	if ( get->m_Parent != set->m_Parent )
	{
		return ( false );
	}

	// names must match $ note that this IS case sensitive - we're dealing with
	//  actual C++ function names here and don't want setFoo() to match GetFoo()
	if ( ::strcmp( get->m_Name.begin() + 3, set->m_Name.begin() + 3 ) != 0 )
	{
		return ( false );
	}

	// good enough
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// struct ClassSpec implementation

ClassSpec :: ClassSpec( void )
{
	m_Type        = VAR_UNKNOWN;
	m_Flags       = FLAG_NONE;
	m_Membership  = FunctionSpec::FLAG_NONE;
	m_HeaderSpec  = NULL;
	m_VarTypeSpec = NULL;

#	if !GP_RETAIL
	m_Docs = NULL;
#	endif // !GP_RETAIL

	m_DoesHandleMgrExistProc = NULL;
	m_HandleAddRefProc       = NULL;
	m_HandleReleaseProc      = NULL;
	m_HandleIsValidProc      = NULL;
	m_HandleGetProc          = NULL;
	m_GetClassSingletonProc  = NULL;
	m_InstanceToNetProc      = NULL;
	m_NetToInstanceProc      = NULL;
}

ClassSpec :: ClassSpec( const ClassSpec& other )
{
	m_HeaderSpec  = NULL;
	m_VarTypeSpec = NULL;
	*this = other;
}

ClassSpec :: ~ClassSpec( void )
{
	delete ( m_HeaderSpec  );
	delete ( m_VarTypeSpec );
}

ClassSpec& ClassSpec :: operator = ( const ClassSpec& other )
{
	gpassert( this != &other );

	m_Type       = other.m_Type;
	m_Flags      = other.m_Flags;
	m_Membership = other.m_Membership;

#	if !GP_RETAIL
	m_Docs = other.m_Docs;
#	endif // !GP_RETAIL

	m_DoesHandleMgrExistProc = other.m_DoesHandleMgrExistProc;
	m_HandleAddRefProc       = other.m_HandleAddRefProc;
	m_HandleReleaseProc      = other.m_HandleReleaseProc;
	m_HandleIsValidProc      = other.m_HandleIsValidProc;
	m_HandleGetProc          = other.m_HandleGetProc;
	m_GetClassSingletonProc  = other.m_GetClassSingletonProc;
	m_InstanceToNetProc      = other.m_InstanceToNetProc;
	m_NetToInstanceProc      = other.m_NetToInstanceProc;

	Delete( m_HeaderSpec );
	if ( other.m_HeaderSpec != NULL )
	{
		m_HeaderSpec = new ClassHeaderSpec( *other.m_HeaderSpec );
	}

	Delete( m_VarTypeSpec );
	if ( other.m_VarTypeSpec != NULL )
	{
		m_VarTypeSpec = new VarTypeSpec( *other.m_VarTypeSpec );
	}

	return ( *this );
}

bool ClassSpec :: IsDerivedFrom( eVarType type ) const
{
	ParentColl::const_iterator i, begin = m_ParentClasses.begin(), end = m_ParentClasses.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->first->m_Type == type )
		{
			return ( true );
		}
		else if ( i->first->IsDerivedFrom( type ) )
		{
			return ( true );
		}
	}
	return ( false );
}

int ClassSpec :: GetDerivedBaseOffset( eVarType base ) const
{
	ParentColl::const_iterator i, begin = m_ParentClasses.begin(), end = m_ParentClasses.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->first->m_Type == base )
		{
			return ( i->second );
		}
		else if ( i->first->IsDerivedFrom( base ) )
		{
			return ( i->second + i->first->GetDerivedBaseOffset( base ) );
		}
	}

	gpassert( 0 );		// should never get here
	return ( 0 );
}

void ClassSpec :: SetPod( size_t size )
{
	GetVarTypeSpec()->m_SizeBytes = size;
	GetVarTypeSpec()->m_Flags |= Trait::FLAG_POD;
	m_Flags |= FLAG_POD;

	gpassert( GetVarTypeSpec()->m_SizeBytes > 0 );
}

bool ClassSpec :: AddMemberFunction( FunctionSpec* function )
{
	static const char RENAME_PREFIX[]   = "FUBI_RENAME_";
	static const UINT RENAME_PREFIX_LEN = ELEMENT_COUNT( RENAME_PREFIX ) - 1;

	bool rename = false;
	if ( function->m_Name.same_with_case( RENAME_PREFIX, RENAME_PREFIX_LEN ) )
	{
		function->m_Name.erase( 0, RENAME_PREFIX_LEN );
		rename = true;
	}

	static const char FUBI_PREFIX[]   = "FUBI_";
	static const UINT FUBI_PREFIX_LEN = ELEMENT_COUNT( FUBI_PREFIX ) - 1;

	bool success = true;

	if ( !rename && function->m_Name.same_with_case( FUBI_PREFIX, FUBI_PREFIX_LEN ) )
	{
		// this is a reserved class management function
		function->m_Flags |= FunctionSpec::FLAG_HIDDEN;

		// get some vars
		const char* name = function->m_Name.begin() + FUBI_PREFIX_LEN;
		size_t nameLen = function->m_Name.length() - FUBI_PREFIX_LEN;

		// helper macro for fast compares
#		define MATCH( str ) \
				(   (nameLen == (ELEMENT_COUNT( str ) - 1)) \
				 && (::memcmp( name, str, (ELEMENT_COUNT( str ) - 1) ) == 0))
#		define MATCH_FRONT( str ) \
				(   (nameLen >= (ELEMENT_COUNT( str ) - 1)) \
				 && (::memcmp( name, str, (ELEMENT_COUNT( str ) - 1) ) == 0))

		// figure out which one and verify its signature. if this code asserts
		// then SOMEBODY was STUPID and didn't update this after changing the
		// managed function specs...

		if ( MATCH( "DoesHandleMgrExist" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType         == VAR_BOOL)
					 && (function->m_ParamSpecs.size()  == 0) );
			m_DoesHandleMgrExistProc = rcast <DoesHandleMgrExistProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_MANAGED;
		}
		else if ( MATCH( "HandleAddRef" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_UINT)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_HANDLE) );
			m_HandleAddRefProc = rcast <HandleAddRefProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_MANAGED;
		}
		else if ( MATCH( "HandleRelease" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_UINT)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_HANDLE) );
			m_HandleReleaseProc = rcast <HandleReleaseProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_MANAGED;
		}
		else if ( MATCH( "HandleIsValid" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_BOOL)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_HANDLE) );
			m_HandleIsValidProc = rcast <HandleIsValidProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_MANAGED;
		}
		else if ( MATCH( "HandleGet" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && IsUser( function->m_ReturnType.m_Type )
					 && (function->m_ReturnType.m_Flags             == TypeSpec::FLAG_POINTER)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_HANDLE) );
			m_HandleGetProc = rcast <HandleGetProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_MANAGED;
		}
		else if ( MATCH( "GetClassSingleton" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && IsUser( function->m_ReturnType.m_Type )
					 && (function->m_ReturnType.m_Flags             == TypeSpec::FLAG_POINTER)
					 && (function->m_ParamSpecs.size()              == 0) );
			m_GetClassSingletonProc = rcast <GetClassSingletonProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_SINGLETON | FLAG_CANRPC;
		}
		else if ( MATCH( "InstanceToNet" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_DWORD)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_POINTER) );
			m_InstanceToNetProc = rcast <InstanceToNetProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_CANRPC;
		}
		else if ( MATCH( "NetToInstance" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && IsUser( function->m_ReturnType.m_Type )
					 && (function->m_ReturnType.m_Flags             == TypeSpec::FLAG_POINTER)
					 && (function->m_ParamSpecs.size()              == 2)
					 && (function->m_ParamSpecs[ 0 ].m_Type         == VAR_DWORD)
					 && (function->m_ParamSpecs[ 1 ].m_Type.m_Type  == gFuBiSysExports.FindType( "FuBiCookie" ))
					 && (function->m_ParamSpecs[ 1 ].m_Type.m_Flags == TypeSpec::FLAG_POINTER_POINTER) );
			m_NetToInstanceProc = rcast <NetToInstanceProc> ( function->m_FunctionPtr );
			m_Flags |= FLAG_CANRPC;
		}
		else if ( MATCH( "GetHeaderSpec" ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_VOID)
					 && (function->m_ParamSpecs.size()              == 1)
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Type  == gFuBiSysExports.FindType( "FuBiClassHeaderSpec" ))
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_REFERENCE) );
			gpassert( m_HeaderSpec == NULL );
			m_HeaderSpec = new ClassHeaderSpec;
			rcast <GetHeaderSpecProc> ( function->m_FunctionPtr )( *m_HeaderSpec );
			if ( m_HeaderSpec->CanSkrit() )
			{
				m_Flags |= FLAG_PROPCANSKRIT;
			}
		}
		else if ( MATCH_FRONT( "Inheritance" /* $0, $1, etc. */ ) )
		{
			gpassert(   (function->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					 && (function->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
					 && (function->m_ReturnType                     == VAR_INT)
					 && (function->m_ParamSpecs.size()              == 1)
					 && IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type )
					 && (function->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_POINTER) );
			typedef int (__cdecl *InheritanceProc)( void* );
			const ClassSpec* cls = gFuBiSysExports.FindClass( function->m_ParamSpecs[ 0 ].m_Type.m_Type );
			InheritanceProc proc = rcast <InheritanceProc> ( function->m_FunctionPtr );
			m_ParentClasses.push_back( std::make_pair( cls, (*proc)( NULL ) ) );
		}
		else if ( MATCH( "PodGetSize" ) )
		{
			gpassert( function->m_Flags                          & FunctionSpec::FLAG_CALL_CDECL );
			gpassert( function->m_ReturnType                     == VAR_UINT );
			gpassert( function->m_ParamSpecs.size()              == 0 );

			typedef size_t (__cdecl *PodGetSizeProc)( void );
			PodGetSizeProc proc = rcast <PodGetSizeProc> ( function->m_FunctionPtr );
			SetPod( (*proc)() );
		}
		else if ( MATCH( "CopyVar" ) )
		{
			gpassert( function->m_Flags                          & FunctionSpec::FLAG_CALL_CDECL );
			gpassert( function->m_ReturnType                     == VAR_VOID );
			gpassert( function->m_ParamSpecs.size()              == 2 );
			gpassert( IsUser( function->m_ParamSpecs[ 0 ].m_Type.m_Type ) );
			gpassert( !function->m_ParamSpecs[ 0 ].m_Type.IsPassByValue() );
			gpassert( IsUser( function->m_ParamSpecs[ 1 ].m_Type.m_Type ) );
			gpassert( !function->m_ParamSpecs[ 1 ].m_Type.IsPassByValue() );
			gpassert( function->m_ParamSpecs[ 1 ].m_Type.m_Flags & TypeSpec::FLAG_CONST );

			GetVarTypeSpec()->m_CopyVarProc = rcast <CopyVarProc> ( function->m_FunctionPtr );
		}
		else
		{
			gperrorf(( "FUBI-prefixed function '%s' not recognized\n", function->m_Name.c_str() ));
			success = false;
		}

		// don't need this any more
#		undef MATCH
#		undef MATCH_FRONT
	}
	else
	{
#		define MATCH( str ) \
				(   (function->m_Name.length() == (ELEMENT_COUNT( str ) - 1)) \
				 && (::memcmp( function->m_Name.c_str(), str, (ELEMENT_COUNT( str ) - 1) ) == 0))

		if ( MATCH( "ToString" ) )
		{
			gpassert( function->m_Flags                          &  FunctionSpec::FLAG_CALL_CDECL );
			gpassert( function->m_ReturnType                     == VAR_VOID );
			gpassert( function->m_ParamSpecs.size()              == 3 );
			gpassert( function->m_ParamSpecs[ 0 ].m_Type         .  IsString() );
			gpassert( function->m_ParamSpecs[ 1 ].m_Type.m_Flags == (TypeSpec::FLAG_REFERENCE | TypeSpec::FLAG_CONST) );
			gpassert( IsEnum( function->m_ParamSpecs[ 2 ].m_Type.m_Type ) );
			gpassert( function->m_ParamSpecs[ 2 ].m_Type.m_Flags == TypeSpec::FLAG_NONE );

			GetVarTypeSpec()->m_ToStringProc = rcast <ToStringProc> ( function->m_FunctionPtr );
		}
		else if ( MATCH( "FromString" ) )
		{
			gpassert( function->m_Flags                          & FunctionSpec::FLAG_CALL_CDECL );
			gpassert( function->m_ReturnType                     == VAR_BOOL );
			gpassert( function->m_ParamSpecs.size()              == 2 );
			gpassert( function->m_ParamSpecs[ 0 ].m_Type         .  IsCString() );
			gpassert( function->m_ParamSpecs[ 1 ].m_Type.m_Flags == TypeSpec::FLAG_REFERENCE );

			GetVarTypeSpec()->m_FromStringProc = rcast <FromStringProc> ( function->m_FunctionPtr );
		}
		else
		{
			success = AddFunctionToIndex( m_Functions, function );
			if ( success )
			{
				// take membership flags
				m_Membership |= function->m_Flags;
			}
		}

		// don't need this any more
#		undef MATCH
	}

	return ( success );
}

void ClassSpec :: PostProcess( void )
{
	GPDEBUG_ONLY( AssertValid() );

	// postprocess methods
	FuBi::PostProcess( m_Functions );

	// look for statics, and convert set/get into variables
	FunctionByNameIndex::iterator i, ibegin = m_Functions.begin(), iend = m_Functions.end();
	for ( i = ibegin ; i != iend ; ++i )
	{

	// Postprocess methods.

		// static?
		if ( !(m_Flags & (FLAG_HAS_STATICS | FLAG_HIDDEN)) )
		{
			FunctionIndex::iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if (    ((*j)->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
					&& !((*j)->m_Flags & FunctionSpec::FLAG_HIDDEN) )
				{
					m_Flags |= FLAG_HAS_STATICS;
					break;
				}
			}
		}

		// get the func
		FunctionSpec* func = *i->second.begin();

	// Add variables.

		// this only works on non-overloaded functions
		if ( i->second.size() != 1 )
		{
			continue;
		}

		// figure out function type
		if ( func->m_Flags & FunctionSpec::FLAG_HIDDEN )
		{
			continue;
		}
		bool isGet = false;
		if ( func->IsValidGet() )
		{
			isGet = true;
		}
		else if ( !func->IsValidSet() )
		{
			continue;
		}

		// fail if our variable is also a function name
		gpstring name = func->m_Name.mid( 3 );		// skip over "?et"
		if ( m_Functions.find( name ) != m_Functions.end() )
		{
			gperrorf(( "Variable '%s::%s' collides with method of the same name\n",
					   m_Name.c_str(), name.c_str() ));
			continue;
		}

		// tag as hidden
		func->m_Flags |= FunctionSpec::FLAG_HIDDEN;

		// get new variablespec
		VariableMapInsertRet rc = m_Variables.insert( std::make_pair( name, VariableSpec() ) );
		VariableSpec& var = rc.first->second;

		// initialize it or add the get function to the list
		if ( rc.second )
		{
			var.m_Name      = name;
			var.m_Parent    = this;
			var.m_Type      = isGet ? func->m_ReturnType : func->m_ParamSpecs[ 0 ].m_Type;
			var.m_GetMethod = isGet ? func : NULL;
			var.m_SetMethod = isGet ? NULL : func;

#			if !GP_RETAIL
			var.m_Docs = func->m_Docs;
#			endif // !GP_RETAIL
		}
		else
		{
			const FunctionSpec* get = isGet ? func : var.m_GetMethod;
			const FunctionSpec* set = isGet ? var.m_SetMethod : func;
			if ( VariableSpec::IsMatched( get, set ) )
			{
				if ( isGet )
				{
					var.m_GetMethod = func;
				}
				else
				{
					var.m_SetMethod = func;
				}

				// other method should have the docs if this one does not
#				if !GP_RETAIL
				if ( (var.m_Docs == NULL) || (*var.m_Docs == '\0') )
				{
					var.m_Docs = func->m_Docs;
				}
#				endif // !GP_RETAIL
			}
			else
			{
				gperrorf(( "Get/set mismatch between '%s::%s' and '%s::%s'\n",
							   m_Name.c_str(), func->m_Name.c_str(),
							   m_Name.c_str(), (isGet ? var.m_SetMethod : var.m_GetMethod)->m_Name.c_str() ));
			}
		}

		// verify it's not messed up
		gpassert( var.m_Type != VAR_VOID );
	}

	// update doco if has schema
#	if !GP_RETAIL
	if ( m_HeaderSpec != NULL )
	{
		m_HeaderSpec->SetDocs( m_Docs );
	}
#	endif // !GP_RETAIL

	// finish
	if ( m_Functions.empty() && m_Variables.empty() && (m_HeaderSpec == NULL) )
	{
		m_Flags |= FLAG_HIDDEN;
	}
}

DWORD ClassSpec :: AddExtraDigest( DWORD digest ) const
{
	// schema
	if ( m_HeaderSpec != NULL )
	{
		digest += m_HeaderSpec->GetDigest();
	}

	// inheritance
	ParentColl::const_iterator i, ibegin = m_ParentClasses.begin(), iend = m_ParentClasses.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		AddCRC32( rcast <UINT&> ( digest ), &i->first->m_Type, sizeof( i->first->m_Type ) );
	}

	// done
	return ( digest );
}

#if !GP_RETAIL

void ClassSpec :: AssertValid( void ) const
{
	// must have a name
	gpassert( !m_Name.empty() );

	// if a managed class, then make sure we have all the entry points
	if ( m_Flags & FLAG_MANAGED )
	{
		gpassert( m_DoesHandleMgrExistProc != NULL );
		gpassert( m_HandleAddRefProc       != NULL );
		gpassert( m_HandleReleaseProc      != NULL );
		gpassert( m_HandleIsValidProc      != NULL );
		gpassert( m_HandleGetProc          != NULL );
	}

	// if a singleton class, then make sure we have all the entry points
	if ( m_Flags & FLAG_SINGLETON )
	{
		gpassert( m_GetClassSingletonProc != NULL );

		// but if it's a singleton there's no point in conversion functions!
		gpassert( m_InstanceToNetProc == NULL );
		gpassert( m_NetToInstanceProc == NULL );
	}

	// if a non-singleton rpc'able class, then make sure we have all the entry points
	if ( (m_Flags & FLAG_CANRPC) && !(m_Flags & FLAG_SINGLETON) )
	{
		gpassert( m_InstanceToNetProc != NULL );
		gpassert( m_NetToInstanceProc != NULL );

		// also make sure we aren't a pointer class. no point in bothering with
		// this instance-to-net stuff if it's just a handle.
		gpassert( !(m_Flags & FLAG_POINTERCLASS) );
	}

	// can't be both singleton and managed
	gpassert( !((m_Flags & FLAG_MANAGED) && (m_Flags & FLAG_SINGLETON)) );

	// no point in being both POD and RPC convertible
	gpassert( !((m_Flags & FLAG_CANRPC) && (m_Flags & FLAG_POD)) );

	// if can't be instantiated ever, POD is wrong here
	gpassert( !((m_Flags & FLAG_POINTERCLASS) && (m_Flags & FLAG_POD)) );
}

void ClassSpec :: GenerateDocs( ReportSys::ContextRef context ) const
{
	GPDEBUG_ONLY( AssertValid() );

	// class name
	ReportSys::Output( context, "Class: " );
	ReportSys::Output( context, m_Name );

	// derivatives
	if ( !m_ParentClasses.empty() )
	{
		ReportSys::Output( context, " : " );

		ParentColl::const_iterator i, ibegin = m_ParentClasses.begin(), iend = m_ParentClasses.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i != ibegin )
			{
				ReportSys::Output( context, ", " );
			}
			ReportSys::Output( context, i->first->m_Name );
		}
	}

	// managed?
	if ( m_Flags & FLAG_MANAGED )
	{
		ReportSys::Output( context, " [MANAGED]" );
	}
	// singleton?
	if ( m_Flags & FLAG_SINGLETON )
	{
		ReportSys::Output( context, " [SINGLETON]" );
	}
	else if ( m_Flags & FLAG_CANRPC )
	{
		ReportSys::Output( context, " [RPC]" );
	}

	// endl
	ReportSys::Output( context, "\n\n" );

	// functions
	bool printedFunctions = false;
	FunctionByNameIndex::const_iterator i, ibegin = m_Functions.begin(), iend = m_Functions.end();

	// nonstatic functions
	for ( i = ibegin ; i != iend ; ++i )
	{
		FunctionIndex::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if (   !((*j)->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
				&& !((*j)->m_Flags & FunctionSpec::FLAG_HIDDEN) )
			{
				ReportSys::Output( context, "    " );
				(*j)->GenerateDocs( context, DOCS_MINIMAL );
				ReportSys::Output( context, "\n" );
				printedFunctions = true;
			}
		}
	}
	if ( printedFunctions )
	{
		printedFunctions = false;
		ReportSys::Output( context, "\n" );
	}

	// nonstatic functions
	for ( i = ibegin ; i != iend ; ++i )
	{
		FunctionIndex::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if (    ((*j)->m_Flags & FunctionSpec::FLAG_STATIC_MEMBER)
				&& !((*j)->m_Flags & FunctionSpec::FLAG_HIDDEN) )
			{
				ReportSys::Output( context, "    " );
				(*j)->GenerateDocs( context, DOCS_MINIMAL );
				ReportSys::Output( context, "\n" );
				printedFunctions = true;
			}
		}
	}
	if ( printedFunctions )
	{
		printedFunctions = false;
		ReportSys::Output( context, "\n" );
	}

	// variables
	if ( !m_Variables.empty() )
	{
		VariableMap::const_iterator i, begin = m_Variables.begin(), end = m_Variables.end();
		for ( i = begin ; i != end ; ++i )
		{
			ReportSys::Output( context, "    " );
			i->second.GenerateDocs( context, DOCS_MINIMAL );
			ReportSys::Output( context, "\n" );
		}
		ReportSys::Output( context, "\n" );
	}

	// schema
	if ( m_HeaderSpec != NULL )
	{
		m_HeaderSpec->GenerateDocs( context );
	}

	// docs
	if ( (m_Docs != NULL) && (*m_Docs != '\0') )
	{
		ReportSys::Output( context, m_Docs );
		ReportSys::Output( context, "\n\n" );
	}
}

#endif // !GP_RETAIL

VarTypeSpec* ClassSpec :: GetVarTypeSpec( void ) const
{
	if ( m_VarTypeSpec == NULL )
	{
		ccast <ClassSpec*> ( this )->m_VarTypeSpec = new VarTypeSpec( this );
	}
	return ( m_VarTypeSpec );
}

//////////////////////////////////////////////////////////////////////////////
// struct EnumSpec implementation

EnumSpec :: EnumSpec( void )
{
	m_Type  = VAR_UNKNOWN;
}

void EnumSpec :: PostProcess( void )
{
	m_VarTypeSpec = VarTypeSpec( this );
}

#if !GP_RETAIL

void EnumSpec :: GenerateDocs( ReportSys::ContextRef context ) const
{
	ReportSys::Output( context, m_Name );
	if ( m_Exporter.IsContinuous() )
	{
		ReportSys::Output( context, " [CONTINUOUS]" );
	}
	else if ( m_Exporter.HasConstants() )
	{
		ReportSys::Output( context, " [IRREGULAR]" );
	}
	else
	{
		ReportSys::Output( context, " [NOCONSTANTS]" );
	}
	ReportSys::OutputEol( context );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
