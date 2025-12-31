//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBi.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBi.h"

#include "DebugHelp.h"
#include "FuBiNetTransport.h"
#include "FuBiPersistImpl.h"
#include "FuBiSchema.h"
#include "FuBiSignature.h"
#include "GpZLib.h"
#include "RatioStack.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "StringTool.h"

#include <limits>
#include <list>
#include <set>

#include <DbgHelp.h>

//////////////////////////////////////////////////////////////////////////////
// out-of-namespace definitions

const BYTE FUBI_EMBEDDED_RPC_TAG[] =
{
	FUBI_EMBED_TAG_MOV,
	FUBI_EMBED_TAG_3,
	FUBI_EMBED_TAG_1,
	FUBI_EMBED_TAG_2,
	FUBI_EMBED_TAG_0,
	FUBI_EMBED_TAG_MOV,
	FUBI_RPC_TAG_3,
	FUBI_RPC_TAG_2,
	FUBI_RPC_TAG_1,
	FUBI_RPC_TAG_0,
	0
};

FuBi::AutoRpcTagBase* FUBI_AutoRpcTagBasePtr = NULL;

FUBI_DECLARE_SELF_TRAITS( FuBi::ModuleEntry );

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// FuBi API implementation - from FuBiDefs.h

const char EVENT_NAMESPACE  [] = "FuBiEvent";
const char TYPE_SKRITOBJECT [] = "SkritObject";
const char TYPE_SKRITVM     [] = "SkritMachine";
const char TYPE_SIEGEPOSDATA[] = "SiegePosData";
const char TYPE_SIEGEPOS    [] = "SiegePos";
const char TYPE_QUAT        [] = "Quat";
const char TYPE_VECTOR3     [] = "Vector";
const char TYPE_FUBICOOKIE  [] = "FuBiCookie";

const char* ToString( Cookie cookie )
{
#	define MSG( x )  if ( cookie == (x) )  return ( #x );

	MSG( RPC_SUCCESS )
	MSG( RPC_SUCCESS_IGNORE )
	MSG( RPC_FAILURE )
	MSG( RPC_FAILURE_IGNORE )

#	undef MSG

	return ( NULL );
}

DECL_NAKED DWORD GetEIP( void )
{
	__asm
	{
		mov eax, dword ptr [esp]
		ret
	}
}

DWORD GetST0( void )
{
	DWORD f;
	__asm fstp dword ptr [f]
	return ( f );
}

// rpc macro helpers

const FunctionSpec* ResolveRpc( DWORD EIP, const char* funcName, int funcNameLen )
	{  return ( gFuBiSysExports.ResolveRpc( EIP, funcName, funcNameLen ) );  }
Cookie SendRpc( const FunctionSpec* spec, void* thisParam, const void* firstParam, DWORD toAddress )
	{  return ( gFuBiSysExports.SendRpc( spec, thisParam, firstParam, toAddress ) );  }
int GetCurrentRpcPacketSize( bool take )
	{  return ( FuBi::SysExports::GetCurrentRpcPacketSize( take ) );  }
bool RpcTestMacro( DWORD addr, const FunctionSpec* spec )
	{  return ( gFuBiSysExports.RpcTestMacro( addr, spec ) );  }
const FunctionSpec* LookupEvent( const void* addr, const char* funcName, int funcNameLen )
	{  return ( gFuBiSysExports.FindFunctionByAddrExact( rcast <DWORD> ( addr ), funcName, funcNameLen ) );  }
const FunctionSpec* LookupEvent( const void* addr, const char* funcName )
	{  return ( LookupEvent( addr, funcName, ::strlen( funcName ) ) );  }
const FunctionSpec* ResolveEvent( DWORD EIP, const char* funcName, int funcNameLen )
	{  return ( gFuBiSysExports.ResolveEvent( EIP, funcName, funcNameLen ) );  }
const FunctionSpec* ResolveCall( DWORD EIP, const char* funcName, int funcNameLen )
	{  return ( gFuBiSysExports.ResolveCall( EIP, funcName, funcNameLen ) );  }

// helper classes

template <typename T>
struct AutoDelete
{
	T** m_Ptr;

	AutoDelete( T*& ptr )  {  m_Ptr = &ptr;  }
   ~AutoDelete( void )  {  Delete ( *m_Ptr );  }
};

// name replacement stuff

struct Entry
{
	const char* m_Entry;
	bool m_Used;

	Entry( void )  {  }

	Entry( const char* entry )
	{
		m_Entry = entry;
		m_Used  = false;
	}
};

typedef std::map <const char*, Entry, string_less> ReplaceMap;
static ReplaceMap* gs_ReplaceMap = NULL;
static AutoDelete <ReplaceMap> s_ReplaceMapAssassin( gs_ReplaceMap );

const char* NameReplaceSpec :: GetReplacement( const char* oldName )
{
	// build replace map on demand
	if ( (gs_ReplaceMap == NULL) && (NameReplaceSpec::ms_Root != NULL) )
	{
		gs_ReplaceMap = new ReplaceMap;

		for ( NameReplaceSpec* i = NameReplaceSpec::ms_Root ; i != NULL ; i = i->m_Next )
		{
			gpassert( gs_ReplaceMap->find( i->m_OldName ) == gs_ReplaceMap->end() );
			(*gs_ReplaceMap)[ i->m_OldName ] = Entry( i->m_NewName );
		}
	}

	// find it
	if ( gs_ReplaceMap != NULL )
	{
		ReplaceMap::iterator found = gs_ReplaceMap->find( oldName );
		if ( found != gs_ReplaceMap->end() )
		{
			found->second.m_Used = true;
			return ( found->second.m_Entry );
		}
	}

	return ( NULL );
}

void NameReplaceSpec :: GetUnusedReplacements( ReplaceColl& strings )
{
	gpassert( gs_ReplaceMap != NULL );

	ReplaceMap::iterator i, begin = gs_ReplaceMap->begin(), end = gs_ReplaceMap->end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->second.m_Used )
		{
			strings.push_back( std::make_pair( i->first, i->second.m_Entry ) );
		}
	}
}

// rpc stuff

static DWORD g_RPC_TO_SERVER = RPC_TO_SERVER_LOCAL;
static DWORD g_RPC_TO_LOCAL  = RPC_TO_LOCAL_DEFAULT;
static DWORD g_RPC_CALLER    = RPC_TO_LOCAL_DEFAULT;

const DWORD& RPC_TO_LOCAL  = g_RPC_TO_LOCAL;
const DWORD& RPC_TO_SERVER = g_RPC_TO_SERVER;
const DWORD& RPC_CALLER    = g_RPC_CALLER;

void SetCallerMachineId( DWORD machineId )
{
	g_RPC_CALLER = machineId;
}

void SetLocalRpcId( DWORD id )
{
	g_RPC_TO_LOCAL = id;
}

void SetRpcIdsToDefaults( void )
{
	g_RPC_TO_SERVER = RPC_TO_SERVER_LOCAL;
	g_RPC_TO_LOCAL  = RPC_TO_LOCAL_DEFAULT;
	g_RPC_CALLER    = RPC_TO_LOCAL_DEFAULT;
}

void SetServerIsRemote( void )
{
	g_RPC_TO_SERVER = RPC_TO_SERVER_REMOTE;
}

void SetServerIsLocal( void )
{
	g_RPC_TO_SERVER = RPC_TO_SERVER_LOCAL;
}

bool IsServerLocal( void )
{
	return ( g_RPC_TO_SERVER == RPC_TO_SERVER_LOCAL );
}

bool IsMachineRemote( DWORD machineId )
{
	if ( machineId == RPC_TO_SERVER_REMOTE )
	{
		return( true );
	}
	else
	{
		return ( machineId != RPC_TO_LOCAL );
	}
}

inline bool ShouldSetAddresses( void )
{
#	if GP_RETAIL
	return ( NetSendTransport::DoesSingletonExist() );
#	else // GP_RETAIL
	return ( true );
#	endif // GP_RETAIL
}

DECL_THREAD int AutoRpcTagBase::ms_PeekAddress = RPC_INVALID_ADDR;

AutoRpcTagBase :: AutoRpcTagBase( void )
{
	m_IsDispatching = gFuBiSysExports.ClearDispatcher();
}

AutoRpcTag :: AutoRpcTag( const AutoRpcTagBase* base )
{
	m_IsDispatching = base ? base->m_IsDispatching : gFuBiSysExports.IsDispatching();
	gFuBiSysExports.EnterRpc( base ? &base->m_IsDispatching : NULL );
}

AutoRpcTag :: ~AutoRpcTag( void )
{
	gFuBiSysExports.LeaveRpc();
	AutoRpcTagBase::ms_PeekAddress = RPC_INVALID_ADDR;
}

// verify spec

#if !GP_RETAIL

gpstring RpcVerifySpec :: BuildQualifiedNameAndParams( bool isRpcPacked, bool allowBinaryOut, int skipParams ) const
{
	gpassert( m_Function != NULL );
	return ( m_Function->BuildQualifiedNameAndParams( m_Params, m_This, isRpcPacked, allowBinaryOut, skipParams ) );
}

#endif // !GP_RETAIL

// nest-detect cancellation

AutoRpcNestDetectCancel :: AutoRpcNestDetectCancel( void )
{
	gFuBiSysExports.PushNestDetectCancel();
}

AutoRpcNestDetectCancel :: ~AutoRpcNestDetectCancel( void )
{
	gFuBiSysExports.PopNestDetectCancel();
}

// traits

#if !GP_RETAIL

static ReportSys::Context* g_XferErrorsPtr = NULL;

ReportSys::Context& GetXferErrors( void )
{
	if ( g_XferErrorsPtr == NULL )
	{
		static ReportSys::Context s_XferErrors( &gWarningContext, "XferErrors" );
		g_XferErrorsPtr = &s_XferErrors;
	}
	return ( *g_XferErrorsPtr );
}

#endif // !GP_RETAIL

eVarType GetTypeByName( const char* name )
{
	return ( gFuBiSysExports.FindType( name ) );
}

// enums
bool EnumConstantFromString( const char* str, const EnumSpec* spec, DWORD& val, bool fastOnly )
{
	return ( gFuBiSysExports.FindEnumConstant( str, val, fastOnly ) == spec );
}

// verification

bool IsValidSysId( const char* name, eNamingConvention naming )
{
	if ( (name == NULL) || (*name == '\0') )
	{
		return ( false );
	}

	if ( !__iscsymf( *name ) || ((*name == '_') && (naming != NAMING_DATA_OR_CODE)) )
	{
		return ( false );
	}

	bool hasLower = !!::islower( *name );
	bool hasUpper = !hasLower;
	bool hasUnder = false;

	// characterize the name
	for ( ++name ; *name != '\0' ; ++name )
	{
		if ( ::islower( *name ) )
		{
			hasLower = true;
		}
		else if ( ::isupper( *name ) )
		{
			hasUpper = true;
		}
		else if ( *name == '_' )
		{
			hasUnder = true;
		}
		else if ( !__iscsym( *name ) )
		{
			return ( false );
		}
	}

	// $ default code/data convention allows either of these cases

	// switch on convention
	if ( naming == NAMING_DATA )
	{
		// no upper case is allowed at all
		if ( hasUpper )
		{
			return ( false );
		}
	}
	else if ( naming == NAMING_CODE )
	{
		// if we have underscores, it must be UPPER_CASE convention
		if ( hasLower && hasUnder )
		{
			return ( false );
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// utility function implementations

	/* general stack notes:

		stack grows downward
		a push will decrement first, then store data
		a pop will load data first, then increment
		esp must be dword-aligned
		each member must be dword-aligned
		passed buffer must be a multiple of 4 in size
	*/

DWORD BlindCallFunction_CdeclCall( const void* args, size_t sz, DWORD func )
{
	// __cdecl notes:
	//
	//  pass right-to-left (so go left-to-right appending to buffer)
	//  restore stack on exit

	gpassert( IsDwordAligned( sz ) );				// make sure size dword-aligned
	gpassert( func );								// make sure calling a valid function
	gpassert( args || (!args && !sz) );				// make sure that args exists (or that it's a void function)
	DWORD rc;										// here's our return value...

	_asm
	{
		mov   ecx, sz         // get size of buffer
		mov   esi, args       // get buffer
		sub   esp, ecx        // allocate stack space
		mov   edi, esp        // start of destination stack frame
		shr   ecx, 2          // make it dwords
		rep   movsd           // copy it
		call  [func]          // call the function

		mov   rc,  eax        // save the return value
		add   esp, sz         // restore the stack pointer
	}

	return ( rc );
}

DWORD BlindCallFunction_StdCall( const void* args, size_t sz, DWORD func )
{
	// __stdcall notes:
	//
	//  pass right-to-left (so go left-to-right appending to buffer)
	//  do not restore stack on exit

	gpassert( IsDwordAligned( sz ) );				// make sure size dword-aligned
	gpassert( func );								// make sure calling a valid function
	gpassert( args || (!args && !sz) );				// make sure that args exists (or that it's a void function)
	DWORD rc;										// here's our return value...

	_asm
	{
		mov   ecx, sz         // get size of buffer
		mov   esi, args       // get buffer
		sub   esp, ecx        // allocate stack space
		mov   edi, esp        // start of destination stack frame
		shr   ecx, 2          // make it dwords
		rep   movsd           // copy it
		call  [func]          // call the function

		mov   rc,  eax        // save the return value
	}

	return ( rc );
}

DWORD BlindCallFunction_ThisCall( const void* args, size_t sz, void* object, DWORD func )
{
	// __thiscall (nonstatic class method) notes
	//
	//  same as __stdcall except additionally store "this" in ecx
	//  $$ question: ctor seems to set eax to "this" on its way out...important?

	gpassert( IsDwordAligned( sz ) );				// make sure size dword-aligned
	gpassert( func );								// make sure calling a valid function
	gpassert( args || (!args && !sz) );				// make sure that args exists (or that it's a void function)
	DWORD rc;										// here's our return value...

	_asm
	{
		mov   ecx, sz         // get size of buffer
		mov   esi, args       // get buffer
		sub   esp, ecx        // allocate stack space
		mov   edi, esp        // start of destination stack frame
		shr   ecx, 2          // make it dwords
		rep   movsd           // copy it
		mov   ecx, object     // set "this"
		call  [func]          // call the function

		mov   rc,  eax        // save the return value
	}

	return ( rc );
}

DWORD BlindCallFunction_ThisCallVarArg( const void* args, size_t sz, void* object, DWORD func )
{
	// __thiscall (nonstatic class method) for variable arguments notes
	//
	//  same as __cdecl except additionally store "this" in ecx and push on the stack last

	gpassert( IsDwordAligned( sz ) );				// make sure size dword-aligned
	gpassert( func );								// make sure calling a valid function
	gpassert( args || (!args && !sz) );				// make sure that args exists (or that it's a void function)
	DWORD rc;										// here's our return value...

	_asm
	{
		mov   ecx, sz         // get size of buffer
		mov   esi, args       // get buffer
		sub   esp, ecx        // allocate stack space
		mov   edi, esp        // start of destination stack frame
		shr   ecx, 2          // make it dwords
		rep   movsd           // copy it
		mov   ecx, object     // set "this"
		push  ecx             // push it on the stack
		call  [func]          // call the function

		mov   rc,  eax        // save the return value
		mov   eax, sz         // ready to restore stack pointer
		add   eax, 4          // pushed ecx too
		add   esp, eax        // restore the stack pointer
	}

	return ( rc );
}

void PostProcess( FunctionByNameIndex& functions )
{
	static const char DOCO_POSTFIX[]      = "$QueryDocs";
	static const char FLAG_POSTFIX[]      = "$QueryFlags";
	static const char CLASS_DOCO_PREFIX[] = "CLASSDOCO";

	enum eQueryType
	{
		QUERY_NONE,
		QUERY_DOCS,
		QUERY_FLAGS,
	};

	FunctionByNameIndex::iterator i, ibegin = functions.begin(), iend = functions.end();
	for ( i = ibegin ; i != iend ; ++i )
	{

	// Check base postprocessing.

		// postprocess each
		bool newFunctions = false;
		FunctionIndex::iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( !((*j)->m_Flags & FunctionSpec::FLAG_POSTPROCESSED) )
			{
				(*j)->m_Flags |= FunctionSpec::FLAG_POSTPROCESSED;
				(*j)->PostProcess();
				newFunctions = true;
			}
		}

		// only continue if this is a NEW function for further postproc
		if ( !newFunctions )
		{
			continue;
		}

		// get doco type and function name
		eQueryType type = QUERY_NONE;
		gpstring name;
		size_t postfixPos;
		if ( (postfixPos = i->first.find( DOCO_POSTFIX )) != gpstring::npos )
		{
			type = QUERY_DOCS;
		}
		else if ( (postfixPos = i->first.find( FLAG_POSTFIX )) != gpstring::npos )
		{
			type = QUERY_FLAGS;
		}
		else
		{
			// not doco
			continue;
		}
		name = i->first.left( postfixPos );

		// tag as doco and hidden;
		FunctionSpec* spec = *jbegin;
		spec->m_Flags |= FunctionSpec::FLAG_HIDDEN | FunctionSpec::FLAG_DOCO;

	// Find the function's match.

		// remove unique leader if there is one
		size_t leader = name.find_last_of( '$' );
		if ( leader != gpstring::npos )
		{
			name.erase( leader );
		}

		// replace on demand
		const char* replace = NameReplaceSpec::GetReplacement( name );
		if ( replace != NULL )
		{
			name = replace;
		}

		// default function
		FunctionSpec* foundFunction = *i->second.begin();

#		if !GP_RETAIL

		// some other vars
		bool classDoco = false;
		FunctionByNameIndex::iterator foundIndex = i;

		// same as the class name?
		if (   (spec->m_Parent != NULL)
			&& name.same_with_case( CLASS_DOCO_PREFIX ) )
		{
			// this is class doco
			gpassert( leader == gpstring::npos );		// should have had a leader!
			classDoco = true;
		}
		else
		{
			// it's probably the previous entry - try that first
			if (   (foundIndex == functions.begin())					// either we're already at the start...
				|| !(--foundIndex)->first.same_with_case( name ) )		// ...or prev entry is not a match
			{
				// ok do a full search <sigh>
				foundIndex = functions.find( name );
				if ( foundIndex == functions.end() )
				{
					// how did that happen?
					if ( !gFuBiSysExports.TestOptions( SysExports::OPTION_NO_EXTRA_DOC_WARNING ) )
					{
						if ( spec->m_Parent != NULL )
						{
							gpwarningf(( "Found documentation for nonexistent class export '%s::%s'\n",
										 spec->m_Parent->m_Name.c_str(), name.c_str() ));
						}
						else
						{
							gpwarningf(( "Found documentation for nonexistent global export '%s'\n",
										 name.c_str() ));
						}
					}
					continue;
				}
			}

			FunctionIndex::iterator k, kbegin = foundIndex->second.begin(), kend = foundIndex->second.end();
			for ( k = kbegin ; k != kend ; ++k )
			{
				// skip until find one with no doco
				if ( (*k)->m_Docs == NULL )
				{
					break;
				}
			}

			if ( k == kend )
			{
				gperrorf(( "Documentation sync problem for function '%s'\n", spec->BuildQualifiedName().c_str() ));
				continue;
			}

			foundFunction = *k;
		}

#		endif // !GP_RETAIL

	// Extract the doco.

		switch ( type )
		{
#			if !GP_RETAIL
			case ( QUERY_DOCS ):
			{
				// verify function params
				gpassert(   (spec->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
						 && IsUser( spec->m_ReturnType.m_Type )
						 && (spec->m_ReturnType.m_Flags == (TypeSpec::FLAG_REFERENCE | TypeSpec::FLAG_CONST) )
						 && (spec->m_ParamSpecs.size()  == 0) );

				// extract the doco
				typedef const FuBi::FunctionDocs& (__cdecl *FuBiQueryDocsProc)( void );
				const FuBi::FunctionDocs& doco = (*rcast <FuBiQueryDocsProc> ( spec->m_FunctionPtr ))();

			// Add doco.

				if ( classDoco )
				{
					spec->m_Parent->m_Docs = doco.m_FunctionDocs;
				}
				else
				{
					// normal docs
					foundFunction->m_Docs = doco.m_FunctionDocs;

					// parameter docs
					if ( !foundFunction->m_ParamSpecs.empty() && (doco.m_ParamDocs != NULL) )
					{
						gpstring paramDoc, constrainName;
						stringtool::Extractor paramEx( ",", doco.m_ParamDocs, true );
						FunctionSpec::ParamSpecs::iterator l,
														   lbegin = foundFunction->m_ParamSpecs.begin(),
														   lend   = foundFunction->m_ParamSpecs.end();
						for ( l = lbegin ; l != lend ; ++l )
						{
							// get param doco and skip out on rest if nothing there
							if ( !paramEx.GetNextString( paramDoc ) )
							{
								break;
							}
							ParamSpec::Extra* extra = l->GetExtra();

							// look for constraints, simple grammar:
							//
							// name? '[' INT_CONSTANT ';' INT_CONSTANT ']'
							// name? '[' FLOAT_CONSTANT ';' FLOAT_CONSTANT ']'
							// name? '[' STRING_CONSTANT ']'

							// find constraint tag
							size_t found = paramDoc.find( '[' );
							if ( found != gpstring::npos )
							{
#								if 0	//$$$$$
								// must be trailed by ']'
								gpassert( *paramDoc.rbegin() == ']' );

								// check syntax and set constraint
								if ( l->m_Type == VAR_FLOAT )
								{
									stringtool::Extractor finder( ";", paramDoc.begin() + found + 1 );
									gpverify( finder.GetNextFloat( extra->m_FloatMin ) );
									gpverify( finder.GetNextFloat( extra->m_FloatMax ) );
									extra->m_Constrain = ParamSpec::Extra::CONSTRAIN_INTERNAL;
								}
								else if ( l->m_Type.IsSimpleInteger() )
								{
									stringtool::Extractor finder( ";", paramDoc.begin() + found + 1 );
									gpverify( finder.GetNextInt( extra->m_IntMin ) );
									gpverify( finder.GetNextInt( extra->m_IntMax ) );
									extra->m_Constrain = ParamSpec::Extra::CONSTRAIN_INTERNAL;
								}
/*$$$								else if ( l->m_Type.IsAString() )
								{
									constrainName.assign( paramDoc, found + 1, paramDoc.size() - 2 - found );
									stringtool::RemoveBorderingWhiteSpace( constrainName );
									extra->m_ExternalType = gFuBiSysExports.FindConstraint( constrainName );
									extra->m_Constrain = ParamSpec::Extra::CONSTRAIN_EXTERNAL;
									gpassert( extra->m_ExternalType != INVALID_CONSTRAINT...$$$ );
								}*/
								else
								{
									gpassert( 0 );		// constraint not supported on this type!
								}
#								endif	// $$$$$
							}

							// take the name, make it nice
							extra->m_Name.assign( paramDoc, 0, found );
							stringtool::RemoveBorderingWhiteSpace( extra->m_Name );
						}
					}
				}
			}	break;
#			endif // !GP_RETAIL

			case ( QUERY_FLAGS ):
			{
				// verify function params
				gpassert(   (spec->m_Flags & FunctionSpec::FLAG_CALL_CDECL)
						 && (spec->m_ReturnType.m_Type              == VAR_VOID)
						 && (spec->m_ParamSpecs.size()              == 1)
						 && IsUser( spec->m_ParamSpecs[ 0 ].m_Type.m_Type )
						 && (spec->m_ParamSpecs[ 0 ].m_Type.m_Flags == TypeSpec::FLAG_REFERENCE) );

				// extract the flags
				typedef void (__cdecl *FuBiQueryFlagsProc)( FuBi::FunctionFlagBuilder& );
				FuBi::FunctionFlagBuilder builder;
				(*rcast <FuBiQueryFlagsProc> ( spec->m_FunctionPtr ))( builder );

				// should not be class doco - don't have class attributes
				gpassert( !classDoco );

			// Add doco.

				// extract permissions
				FuBi::FunctionFlagBuilder::PermissionColl::const_iterator
						i, ibegin = builder.m_Permissions.begin(), iend = builder.m_Permissions.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					foundFunction->m_Flags |= FuBi::FunctionSpec::ToFlags( *i );
				}

				// extract memberships
				FuBi::FunctionFlagBuilder::MembershipColl::const_iterator
						j, jbegin = builder.m_Memberships.begin(), jend = builder.m_Memberships.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( j == jbegin )
					{
						// if we're explicitly adding membership, then clear
						//  the default "all" membership
						if ( j->second == FuBi::FunctionFlagBuilder::COMMAND_ADD )
						{
							foundFunction->m_Flags &= NOT( FunctionSpec::DEFAULT_MEMBERSHIP );
						}
					}

					FuBi::FunctionSpec::eFlags flags = FuBi::FunctionSpec::ToFlags( j->first );
					if ( j->second == FuBi::FunctionFlagBuilder::COMMAND_ADD )
					{
						foundFunction->m_Flags |= flags;
					}
					else
					{
						foundFunction->m_Flags &= NOT( flags );
					}
				}

			}	break;
		}
	}

#	if ( GP_DEBUG )

	// additional verification - make sure all overloads have doco or none have it.

	for ( i = ibegin ; i != iend ; ++i )
	{
		size_t doco = 0;

		FunctionIndex::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( (*j)->m_Docs != NULL )
			{
				++doco;
			}
		}

		gpassertf( (doco == 0) || (doco == i->second.size()), ( "Documentation sync problem for function '%s'\n", (*i->second.begin())->BuildQualifiedName().c_str() ) );
	}

#	endif // GP_DEBUG
}

bool AddFunctionToIndex( FunctionByNameIndex& index, FunctionSpec* spec )
{
	FunctionByNameIndexInsertRet rc = index.insert( std::make_pair( spec->m_Name, FunctionIndex() ) );
	const gpstring& name = rc.first->first;
	FunctionIndex& overloads = rc.first->second;

	// already there
	if ( !rc.second )
	{
		// check name collisions
		if ( !name.same_with_case( spec->m_Name ) )
		{
			if ( spec->m_Parent == NULL )
			{
				gperrorf(( "global function name collision detected between '%s' and '%s' (exports are case insensitive)\n",
							   name.c_str(), spec->m_Name.c_str() ));
			}
			else
			{
				gperrorf(( "member function of '%s' name collision detected between '%s' and '%s' (exports are case insensitive)\n",
							   spec->m_Parent->m_Name.c_str(), name.c_str(), spec->m_Name.c_str() ));
			}
			return ( false );
		}

		// check for signature mismatch - this can happen if two separate modules
		//  declare slightly different functions. they can differ in just the
		//  calling convention or if one is in a namespace, another in a class,
		//  etc. the linker sees this as ok (if only compares mangled names i
		//  guess) but it screws up fubi so check the exact signature for match.
		//  overloads are ok so only check the things that can vary and make
		//  sure that they do.
		FunctionIndex::const_iterator f, fbegin = overloads.begin(), fend = overloads.end();
		for ( f = fbegin ; f != fend ; ++f )
		{
			const FunctionSpec* func = *f;
			if ( func->m_ParamSpecs.size() == spec->m_ParamSpecs.size() )
			{
				// test the params, see if any differ
				FunctionSpec::ParamSpecs::const_iterator i, j,
					ibegin = func->m_ParamSpecs.begin(), iend = func->m_ParamSpecs.end(),
					jbegin = spec->m_ParamSpecs.begin();
				for ( i = ibegin, j = jbegin ; i != iend ; ++i, ++j )
				{
					if ( i->m_Type != j->m_Type )
					{
						break;
					}
				}

				// none differed
				if ( i == iend )
				{
					// identical function? could be module-level replacement
#					if !GP_RETAIL
					if ( ::same_with_case( spec->m_MangledName, func->m_MangledName ) )
					{
						gpgenericf(( "FuBi REPLACE '%s'\n", spec->BuildQualifiedName().c_str() ));
					}
					else
					{
						// nope, this is definitely an error then
						gperrorf(( "critical error! separate functions are nearly identical to FuBi: '%s'\n",
								spec->BuildQualifiedName().c_str() ));
					}
#					endif // !GP_RETAIL
					return ( false );
				}
			}
		}
	}

	// add function (may be new or may be an overload)
	overloads.push_back( spec );
	return ( true );
}

// randomly generated numbers from http://www.fourmilab.ch/hotbits/. we're
// storing the decryption key with the EXE so don't expect this to be safe (stop
// laughing, it's really not meant to be safe, it's just for casual users).

static const DWORD ENCRYPT_DUMMY1[] =
{
	0x82CA9C90, 0xC4B347FB, 0xA0CA23FF, 0xAD8F6100,
	0x6358B648, 0x0E1E5551, 0x9099F733,
};

static const DWORD ENCRYPT_REAL[] =
{
	/*k0*/ 0xD26A184D, 0x708576E2,
	/*k1*/ 0x54A53767, 0xC48CA6A8, 0xE7AB95B0,
	/*k2*/ 0x42E3A574, 0x7D10C897,
	/*k3*/ 0x25657DD2,
};

static const DWORD ENCRYPT_DUMMY2[] =
{
	0x04AF2CBC, 0x23EA40B3, 0x075C0AA7, 0xCE59D490,
	0x2D66DE2F, 0x490DD032, 0x1CF1CE95,
};

static const DWORD ENCRYPT_DELTA = 0x502D0591;

void Encrypt( void* data, size_t length )
{
	::TeaEncrypt( data, length, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

void Decrypt( void* data, size_t length )
{
	::TeaDecrypt( data, length, ENCRYPT_REAL[ 0 ], ENCRYPT_REAL[ 2 ], ENCRYPT_REAL[ 5 ], ENCRYPT_REAL[ 7 ] );
}

//////////////////////////////////////////////////////////////////////////////
// struct ModuleEntry implementation

bool ModuleEntry :: Xfer( PersistContext& persist )
{
	persist.XferHex( "name", m_Name );
	persist.XferHex( "crc32", m_ActualCrc );
	persist.XferHex( "extra", m_HeaderCrc );
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// struct Manifest implementation

void Manifest :: SaveToFuel( const char* key, FuelHandle fuel )
{
	FuelWriter writer( fuel );
	PersistContext persist( &writer );
	Xfer( key, persist );
}

void Manifest :: LoadFromSystem( void )
{
	m_Digest = gFuBiSysExports.GetDigest();
	m_Modules = gFuBiSysExports.GetModuleColl();

	for ( ModuleColl::iterator i = m_Modules.begin() ; i != m_Modules.end() ; ++i )
	{
		gpstring fileName = FileSys::GetFileName( i->m_Name );
		i->m_Name = fileName;
	}
}

bool Manifest :: LoadFromFuel( const char* key, FuelHandle fuel )
{
	FuelReader reader( fuel );
	PersistContext persist( &reader );
	return ( Xfer( key, persist ) );
}

bool Manifest :: Xfer( const char* key, PersistContext& persist )
{
	bool rc = persist.EnterBlock( key );
	if ( rc )
	{
		persist.XferHex( "digest", m_Digest );
		persist.XferList( "modules", m_Modules );
	}
	persist.LeaveBlock();
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class SysExports::SyncSpec implementation

bool SysExports::SyncSpec :: Write( FileSys::StreamWriter& writer ) const
{
	return ( writer.WriteStruct( *this ) );
}

bool SysExports::SyncSpec :: Read( FileSys::Reader& reader )
{
	if ( !reader.ReadStruct( m_Size ) )
	{
		return ( false );
	}
	if ( m_Size != sizeof( *this ) )
	{
		// ffwd past data and reset self
		reader.Seek( m_Size - sizeof( m_Size ), FileSys::SEEKFROM_CURRENT );
		*this = SyncSpec();
		return ( false );
	}
	return ( reader.Read( &m_Size + 1, sizeof( *this ) - sizeof( m_Size ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class SysExports implementation

DECL_THREAD bool SysExports::ms_IsSending = false;
DECL_THREAD bool SysExports::ms_IsDispatching = false;
DECL_THREAD int  SysExports::ms_CurrentRpcPacketSize = 0;

// $ the first T* param is just to get the type down. i tried using explicit
//   template instantiation to call the one i wanted but it didn't work. have
//   to use annoying workaround.

template <typename T>
VarTypeSpec MakeVarTypeSpec( T*, ToStringProc to, FromStringProc from )
{
	return ( VarTypeSpec( Traits <T>::GetInternalVarName(),
						  Traits <T>::GetExternalVarName(),
						  Traits <T>::GetSize(),
						  Traits <T>::GetFlags(),
						  to,
						  from,
						  NULL ) );
}

template <typename T>
VarTypeSpec MakeVarTypeSpecStr( T* )
{
	return ( MakeVarTypeSpec( (T*)0, (ToStringProc  )Traits <T>::ToString,
								     (FromStringProc)Traits <T>::FromString ) );
}

template <typename T>
VarTypeSpec MakeVarTypeSpecNoStr( T* )
{
	return ( MakeVarTypeSpec( (T*)0, NULL, NULL ) );
}

SysExports :: SysExports( void )
{

// Init basic infos.

	m_Rebroadcaster   = NULL;
	m_Options         = OPTION_NONE;
	m_Digest          = VERSION;
	m_RenameComplete  = false;
	m_ImportComplete  = false;
	m_RpcSyncComplete = false;

	SetOptions( OPTION_NO_REQUIRE_SYNC );

// Enumerations.

	// add enums
	for ( EnumExporter* ie = EnumExporter::ms_Root ; ie != NULL ; ie = ie->m_Next )
	{
		// rename enum if necessary
		gpstring name = ie->m_Name;
		const char* newName = NameReplaceSpec::GetReplacement( name );
		if ( newName != NULL )
		{
			name = newName;
		}

		// add to spec map (this should never fail)
		EnumSpec* spec = GetEnum( name );
		spec->m_Exporter = *ie;
		spec->m_Exporter.m_Parent = spec;

		// now add its symbols if it's continuous
		if ( spec->m_Exporter.IsContinuous() )
		{
			gpassert( spec->m_Exporter.m_EnumToStringProc != NULL );
			DWORD i, begin = spec->m_Exporter.m_Begin, end = begin + spec->m_Exporter.m_Count;
			for ( i = begin ; i != end ; ++i )
			{
				const char* sym = (*spec->m_Exporter.m_EnumToStringProc)( i );
				gpassert( m_EnumConstants.find( sym ) == m_EnumConstants.end() );
				m_EnumConstants[ sym ] = EnumConstant( spec->m_Type, i );
			}
		}
		else
		{
			// otherwise add it to the irregular set
			m_IrregularEnums.push_back( spec );
		}
	}

// POD types.

	// standard types
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (char            *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (unsigned char   *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (short           *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (unsigned short  *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (int             *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (unsigned int    *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (__int64         *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (unsigned __int64*)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (float           *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (double          *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecNoStr( (void            *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (bool            *)0 ) );

	// special types
	m_VarTypeSpecs.push_back( MakeVarTypeSpecNoStr( (mem_ptr         *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecNoStr( (const_mem_ptr   *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (gpstring        *)0 ) );
	m_VarTypeSpecs.push_back( MakeVarTypeSpecStr  ( (gpwstring       *)0 ) );

	// check count
	gpassert( m_VarTypeSpecs.size() == (size_t)(VAR_COUNT + VAR_SPECIAL_COUNT) );

	// add user-exported types
	for ( PodExporter* ip = PodExporter::ms_Root ; ip != NULL ; ip = ip->m_Next )
	{
		// rename class if necessary
		gpstring name = ip->m_Name;
		const char* newName = NameReplaceSpec::GetReplacement( name );
		if ( newName != NULL )
		{
			name = newName;
		}

		// add each to set
		GetClass( name )->SetPod( ip->m_Size );
	}

// Pointer classes.

	// add user-exported pointer class types
	for ( PointerClassExporter* pp = PointerClassExporter::ms_Root ; pp != NULL ; pp = pp->m_Next )
	{
		// rename class if necessary
		gpstring name = pp->m_Name;
		const char* newName = NameReplaceSpec::GetReplacement( name );
		if ( newName != NULL )
		{
			name = newName;
		}

		// add each to set
		GetClass( name )->m_Flags |= ClassSpec::FLAG_POINTERCLASS;
	}

// Keywords.

	// add keywords
	for ( KeywordExporter* ik = KeywordExporter::ms_Root ; ik != NULL ; ik = ik->m_Next )
	{
		// add each to set
		for ( int i = 0 ; i < ik->m_Count ; ++i )
		{
			const char* keyword = (*ik->m_QueryProc)( i );
			gpassert( m_Keywords.find( keyword ) == m_Keywords.end() );
			m_Keywords.insert( keyword );
		}
	}

	// add the custom types
	GetClass( EVENT_NAMESPACE   );
	GetClass( TYPE_SKRITOBJECT  );
	GetClass( TYPE_SKRITVM      );
	GetClass( TYPE_SIEGEPOSDATA );
	GetClass( TYPE_SIEGEPOS     );
	GetClass( TYPE_QUAT         );
	GetClass( TYPE_VECTOR3      );
	GetClass( TYPE_FUBICOOKIE   );
}

SysExports :: ~SysExports( void )
{
	stdx::for_each( m_Classes, stdx::delete_second_by_ptr() );
	stdx::for_each( m_Enums,   stdx::delete_second_by_ptr() );
}

bool SysExports :: Synchronize( const SyncSpec& spec )
{
	// digest and version must match up - this is very simple code that will be
	// expanded when we get new versions...
	m_RpcSyncComplete = MakeSynchronizeSpec() == spec;

	// return success code
	return ( m_RpcSyncComplete );
}

SysExports::SyncSpec SysExports :: MakeSynchronizeSpec( void ) const
{
	return ( SyncSpec( GetDigest(), m_Modules.size() ) );
}

void SysExports :: EnableRpcBuffering( void )
{
	SetOptions( OPTION_BUFFER_RPCS );
	gpgeneric( "FuBi: Enabling RPC buffering...\n" );
}

void SysExports :: DisableRpcBuffering( bool flushPending )
{
	SetOptions( OPTION_BUFFER_RPCS, false );
	m_BufferedExceptIds.clear();

	gpgenericf(( "FuBi: Disabling RPC buffering (%s)...\n", flushPending ? "FLUSH" : "NOFLUSH" ));

	if ( flushPending )
	{
		FlushRpcBuffers();
	}

	gpgeneric( "FuBi: flush complete\n" );
}

void SysExports :: FlushRpcBuffers( bool deleteOnly )
{
	// swap locally to prevent change-while-iter during dispatch
	PacketColl packets;
	packets.swap( m_BufferedRpcs );
	if ( deleteOnly )
	{
		return;
	}

	// track progress if needed
	RatioSample ratioSample( "flush_rpc_buffers", packets.size() );

	// local dispatcher
	NetReceiveTransport transport;

	// dispatch
	PacketColl::iterator i, ibegin = packets.begin(), iend = packets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// get some debug info
#		if !GP_RETAIL
		const FunctionSpec* spec = FindFunctionBySerialID( *(WORD*)&*i->m_Data.begin() );
		gpassert( spec != NULL );
#		endif // !GP_RETAIL

		// set packet
		int size = i->m_Data.size();
		transport.TakeBuffer( i->m_Data );

		// dispatch it
		Cookie rc = DispatchNextRpc( i->m_CallerAddr, size );
#		if !GP_RETAIL
		if ( (rc == RPC_FAILURE) || (rc == RPC_FAILURE_IGNORE) )
		{
			gpstring info = spec->BuildQualifiedNameAndParams( const_mem_ptr( &*i->m_Data.begin() + 2, i->m_Data.size() ), NULL, true );
			gperrorf(( "Hey, you're not supposed to return an error code from a buffered RPC flush!\n\n"
					   "Spec: %s\n", info.c_str() ));
		}
#		endif // !GP_RETAIL

		// spin the counter
		ratioSample.Advance( GPDEV_ONLY( spec->BuildQualifiedName().c_str() ) );
	}
}

void SysExports :: AddRpcBufferExceptions( const char* funcName )
{
	FunctionIndexColl coll;
	FindFunctionsByQualifiedName( coll, funcName );

	FunctionIndexColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FunctionIndex::const_iterator j, jbegin = (*i)->begin(), jend = (*i)->end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			m_BufferedExceptIds.insert( (*j)->m_SerialID );
		}
	}
}

#if GP_DEBUG

void SysExports :: RegisterNestedRpcException( const char* caller, const char* called )
{
	FunctionIndexColl calledColl, callerColl;
	FindFunctionsByQualifiedName( callerColl, caller );
	FindFunctionsByQualifiedName( calledColl, called );

	if ( same_no_case( caller, "*" ) )
	{
		gpassert( !calledColl.empty() );

		FunctionIndexColl::const_iterator i, ibegin = calledColl.begin(), iend = calledColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FunctionIndex::const_iterator j, jbegin = (*i)->begin(), jend = (*i)->end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				m_NestedRpcXCalleds.push_back( (*j)->m_SerialID );
			}
		}

		std::sort( m_NestedRpcXCalleds.begin(), m_NestedRpcXCalleds.end() );
		std::unique( m_NestedRpcXCalleds.begin(), m_NestedRpcXCalleds.end() );
	}
	else if ( same_no_case( called, "*" ) )
	{
		gpassert( !callerColl.empty() );

		FunctionIndexColl::const_iterator i, ibegin = callerColl.begin(), iend = callerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FunctionIndex::const_iterator j, jbegin = (*i)->begin(), jend = (*i)->end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				m_NestedRpcXCallers.push_back( (*j)->m_SerialID );
			}
		}

		std::sort( m_NestedRpcXCallers.begin(), m_NestedRpcXCallers.end() );
		std::unique( m_NestedRpcXCallers.begin(), m_NestedRpcXCallers.end() );
	}
	else
	{
		gpassert( !callerColl.empty() );
		gpassert( !calledColl.empty() );

		FunctionIndexColl::const_iterator i, ibegin = callerColl.begin(), iend = callerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FunctionIndex::const_iterator j, jbegin = (*i)->begin(), jend = (*i)->end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				FunctionIndexColl::const_iterator k, kbegin = calledColl.begin(), kend = calledColl.end();
				for ( k = kbegin ; k != kend ; ++k )
				{
					FunctionIndex::const_iterator l, lbegin = (*k)->begin(), lend = (*k)->end();
					for ( l = lbegin ; l != lend ; ++l )
					{
						m_NestedRpcExceptionColl.push_back( std::make_pair( (*j)->m_SerialID, (*l)->m_SerialID ) );
					}
				}
			}
		}

		std::sort( m_NestedRpcExceptionColl.begin(), m_NestedRpcExceptionColl.end() );
		std::unique( m_NestedRpcExceptionColl.begin(), m_NestedRpcExceptionColl.end() );
	}
}

#endif // GP_DEBUG

bool SysExports :: ImportFunction( const char* mangledName, const char* unmangledName, DWORD address, SignatureParser* parser )
{
	gpassert( !m_ImportComplete );
	int mangledLen = 0;

#	if GP_ENABLE_PROFILING

	// check for special exports that we should ignore
	static const char* s_IgnoreFuncs[] =
	{
		"_q_prof_data_",
		"CoverageAddAnnotation",
		"CoverageClearData",
		"CoverageDisableRecordingData",
		"CoverageIsRecordingData",
		"CoverageIsRunning",
		"CoverageSaveData",
		"CoverageStartRecordingData",
		"CoverageStopRecordingData",
		"PurelockIsRunning",
		"PurelockPrintf",
		"PurePrintf",
		"PurifyAllHandlesInuse",
		"PurifyAllInuse",
		"PurifyAllLeaks",
		"PurifyAssertIsReadable",
		"PurifyAssertIsWritable",
		"PurifyBlue",
		"PurifyClearInuse",
		"PurifyClearLeaks",
		"PurifyDescribe",
		"PurifyGetPoolId",
		"PurifyGetUserData",
		"PurifyGreen",
		"PurifyHeapValidate",
		"PurifyIsInitialized",
		"PurifyIsReadable",
		"PurifyIsRunning",
		"PurifyIsWritable",
		"PurifyMapPool",
		"PurifyMarkAsInitialized",
		"PurifyMarkAsUninitialized",
		"PurifyMarkForNoTrap",
		"PurifyMarkForTrap",
		"PurifyNewHandlesInuse",
		"PurifyNewInuse",
		"PurifyNewLeaks",
		"PurifyPrintf",
		"PurifyRed",
		"PurifySetLateDetectScanCounter",
		"PurifySetLateDetectScanInterval",
		"PurifySetPoolId",
		"PurifySetUserData",
		"PurifyWhatColors",
		"PurifyYellow",
		"QuantifyAddAnnotation",
		"QuantifyClearData",
		"QuantifyDisableRecordingData",
		"QuantifyIsRecordingData",
		"QuantifyIsRunning",
		"QuantifySaveData",
		"QuantifyStartRecordingData",
		"QuantifyStopRecordingData",
	};

    // c++ exports always start with ?
	if ( *mangledName != '?' )
	{
		const char** i, ** ibegin = s_IgnoreFuncs, ** iend = ARRAY_END( s_IgnoreFuncs );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( same_with_case( unmangledName, *i ) )
			{
				return ( true );
			}
		}
	}

#	endif // GP_ENABLE_PROFILING

// Build function spec.

	// add generic spec to map
	FunctionByAddrMap::iterator newFunc = m_Functions.insert( std::make_pair( address, FunctionSpec() ) );
	FunctionSpec& spec = newFunc->second;

	// initialize
	spec.m_FunctionPtr = address;
	spec.m_SerialID    = m_FunctionsBySerialID.size();

#	if !GP_RETAIL
	spec.m_MangledName   = mangledName;
	spec.m_UnmangledName = unmangledName;
#	endif // !GP_RETAIL

	// add it
	m_FunctionsBySerialID.push_back( &spec );

	// this should be SMALL - plus it must be in a word anyway for RPC's
	gpassert( m_FunctionsBySerialID.size() <= std::numeric_limits <WORD>::max() );

	// parse it
	SignatureParser::eParse parse = parser->Parse( unmangledName, spec );
	if ( parse != SignatureParser::PARSE_OK )
	{
		if ( parse == SignatureParser::PARSE_ERROR )
		{
			gperrorf(( "Error parsing function '%s'\n", unmangledName ));
		}
		goto no_func;
	}

	// check for dup name at same address - this is illegal because it will
	// cause name resolution problems for rpc and skrit callbacks
#	if GP_DEBUG
	{
		FunctionByAddrMap::const_iterator i,
										  begin = m_Functions.lower_bound( address ),
										  end   = m_Functions.upper_bound( address );
		for ( i = begin ; i != end ; ++i )
		{
			if ( i != newFunc )
			{
				gpassert( !newFunc->second.m_Name.same_no_case( i->second.m_Name ) );
			}
		}
	}
#	endif // GP_DEBUG

// Add it to database.

	if ( (spec.m_Parent == NULL) && !(spec.m_Flags & FunctionSpec::FLAG_TRAIT) )
	{
		// global function
		if ( !AddFunctionToIndex( m_GlobalFunctions, &spec ) )
		{
			goto no_func;
		}
	}
	else
	{
		// special: reassign traits to a particular class
		if ( spec.m_Parent == NULL )
		{
			gpassert( spec.m_Flags & FunctionSpec::FLAG_TRAIT );
			spec.m_Parent = FindClass( parser->GetTraitType().m_Type );
		}

		// trait or member function
		if ( !spec.m_Parent->AddMemberFunction( &spec ) )
		{
			goto no_func;
		}
	}

// Update digest.

	// $$$ skrits will be affected by default params! also include param defaults
	//     in the checksum (requires doco access)

	// set individual function digest
	mangledLen = ::strlen( mangledName );
	spec.m_Digest = GetCRC32( 0, mangledName, mangledLen + 1 );

	// only do this for non-doco functions
	if ( !(spec.m_Flags & FunctionSpec::FLAG_DOCO) )
	{
		// use the mangled name - it contains the full signature!
		m_Digest = GetCRC32( m_Digest, mangledName, mangledLen + 1 );
	}

	return ( true );

// Non-function case.

no_func:

	// undo addition of function into database
	m_Functions.erase( newFunc );
	m_FunctionsBySerialID.pop_back();

	// not a real function
	return ( false );
}

// name-replacement helper
template <typename COLL>
void ReplaceNameInIndex( COLL& coll )
{
	COLL::iterator i, begin = coll.begin(), end = coll.end();
	for ( i = begin ; i != end ; )
	{
		const char* replace = NameReplaceSpec::GetReplacement( i->first );
		if ( replace != NULL )
		{
			// replaced name cannot also appear in map
			gpassert( coll.find( replace ) == coll.end() );

			// replace it
			i->second->m_Name = replace;
			coll[ i->second->m_Name ] = i->second;
			i = coll.erase( i );
		}
		else
		{
			++i;
		}
	}
}

bool SysExports :: ImportBindings( HMODULE module, ExportColl* dest, bool finish )
{
	// cannot add to existing import set - i'm not set up for that.
	gpassert( !m_ImportComplete );

	// build parser
	SignatureParser parser;

	// set to local image by default
	bool isLocalImage = module == NULL;
	if ( isLocalImage )
	{
		module = ::GetModuleHandle( NULL );
	}

// Find the export table.

	// get headers
	const BYTE* imageBase = rcast <const BYTE*> ( module );
	const IMAGE_DOS_HEADER* dosHeader = rcast <const IMAGE_DOS_HEADER*> ( imageBase );
	const IMAGE_NT_HEADERS* winHeader = rcast <const IMAGE_NT_HEADERS*> ( imageBase + dosHeader->e_lfanew );

	// find the export data directory
	const IMAGE_DATA_DIRECTORY& exportDataDir = winHeader->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
	DWORD exportRva  = exportDataDir.VirtualAddress;
	DWORD exportSize = exportDataDir.Size;

	// see if it exists
	if ( exportRva == 0 )
	{
		return ( false );	// nope
	}

	// find the export header or export directory
	DWORD exportBegin = exportRva, exportEnd = exportBegin + exportSize;
	const ExportTableHeader* fubiHeader = rcast <const ExportTableHeader*> ( imageBase + exportBegin );
	const IMAGE_EXPORT_DIRECTORY* exportDir = rcast <const IMAGE_EXPORT_DIRECTORY*> ( imageBase + exportBegin );
	bool success = true;

	// check to see if it's been repacked
	if ( fubiHeader->m_Signature == FUBI_SIGNATURE )
	{
		// get deflation buffer
		stdx::fast_vector <BYTE> buffer, exports;
		buffer.resize( fubiHeader->m_CompressedSize );
		::memcpy( &*buffer.begin(), fubiHeader + 1, buffer.size() );

		// decrypt
		Decrypt( &*buffer.begin(), buffer.size() );

		// uncompress
		exports.resize( fubiHeader->m_UncompressedSize );
		if ( zlib::Inflate(
				mem_ptr( &*exports.begin(), exports.size() ),
				const_mem_ptr( &*buffer.begin(), buffer.size() ) ) != (int)exports.size() )
		{
			gpfatal( "FuBi: unable to read export table, possible corruption? App must fatal\n" );
		}

		// loop and extract
		const_mem_iter iter( &*exports.begin(), exports.size() );
		for ( WORD count = iter.advance_word() ; count ; --count )
		{
			// extract
			const char* mangledName = (const char*)iter.mem;
			iter.advance( ::strlen( mangledName ) + 1 );
			const char* unmangledName = (const char*)iter.mem;
			iter.advance( ::strlen( unmangledName ) + 1 );
			DWORD functionAddress = iter.advance_dword() + (DWORD)imageBase;

			// store or forward
			if ( dest != NULL )
			{
				// add entry
				dest->push_back( ExportEntry( mangledName, unmangledName, functionAddress ) );
			}
			else
			{
				// attempt to import it
				if ( !ImportFunction( mangledName, unmangledName, functionAddress, &parser ) )
				{
					success = false;
				}
			}
		}
	}
	else
	{
		// find the subtables
		const DWORD* funcTable    = rcast <const DWORD*> ( imageBase + exportDir->AddressOfFunctions    );
		const DWORD* nameTable    = rcast <const DWORD*> ( imageBase + exportDir->AddressOfNames        );
		const WORD * ordinalTable = rcast <const WORD *> ( imageBase + exportDir->AddressOfNameOrdinals );

	// Iterate over all the exported functions.

		const DWORD* nameTableBegin = nameTable,
				   * nameTableEnd   = nameTableBegin + exportDir->NumberOfNames,
				   * nameTableIter;
		const WORD * ordinalTableIter = ordinalTable;

		for ( nameTableIter = nameTableBegin ; nameTableIter != nameTableEnd ; ++nameTableIter, ++ordinalTableIter )
		{
			const char* functionName = rcast <const char*> ( imageBase + *nameTableIter );
			DWORD functionAddress = funcTable[ *ordinalTableIter ];

			// $ special: skip if it's just a string
			if ( same_with_case( "??_C@_", functionName, 6 ) )
			{
				continue;
			}

			// this will be false if it's a forwarding export (unlikely but whatever...)
			if ( (functionAddress < exportBegin) || (functionAddress >= exportEnd) )
			{
				// get address to function call
				functionAddress += rcast <DWORD> ( imageBase );

				// $ NOTE $ this code is VERY VC++ version specific. it knows about
				//          how the compiler will export a function via a relative
				//          jump (probably for easy relocations). if this pattern
				//          changes in the future, this code will need to be
				//          updated.

				// it probably points to a jump indirection - redirect to actual function
				if ( *rcast <const BYTE*> ( functionAddress ) == 0xE9 /*jmp*/ )
				{
					// cool, figure out relative jump address (function + sizeof op)
					++functionAddress;
					DWORD offset = *rcast <const DWORD*> ( functionAddress );
					functionAddress += 4 + offset;
				}

				// get length
				const char* mangledName = functionName;

				// make sure we've got our dll
				static DbgHelpDll s_DbgHelpDll;
				if ( !s_DbgHelpDll.Load() )
				{
					gpfatal( "FuBi: unable to parse export table because no DBGHELP.DLL support, app must fatal\n" );
				}

			// De-mangle the name.

				// $ note that we could decode the mangled name directly by writing a
				//   parser specifically for that. however the format is specific to the
				//   compiler and version, plus there are no publicly available docs on
				//   it. rather than reverse engineer the format by exporting every possible
				//   function variant, just let DbgHelp to do the work for us. to keep from
				//   being dependent on a debug support DLL and exposing our functions to
				//   the world through the export table, a postprocessor will strip the
				//   export table and store it in the resources instead.

				// de-mangle into readable text for our cheesy little parser
				char unmangledName[ 0x400 ];
				if ( s_DbgHelpDll.UnDecorateSymbolName(
						mangledName,
						unmangledName,
						ELEMENT_COUNT( unmangledName ),
						UNDNAME_COMPLETE | UNDNAME_32_BIT_DECODE ) == 0 )
				{
					return ( false );		// $ error is in ::GetLastError();
				}

				// store or forward
				if ( dest != NULL )
				{
					// add entry
					dest->push_back( ExportEntry( mangledName, unmangledName, functionAddress - rcast <DWORD> ( imageBase ) ) );
				}
				else
				{
					// attempt to import it
					if ( !ImportFunction( mangledName, unmangledName, functionAddress, &parser ) )
					{
						success = false;
					}
				}
			}
		}
	}

	// if we want to finish and weren't just interested in exports, do it
	if ( dest == NULL )
	{
		// register it
		gpstring moduleName = FileSys::GetModuleFileName( module );
		DWORD headerCrc = 0, actualCrc = 0;
		FileSys::GetModuleChecksums( headerCrc, actualCrc, module );
		m_Modules.push_back( ModuleEntry( moduleName, headerCrc, actualCrc ) );
		if ( !isLocalImage )
		{
			gpgenericf(( "FuBi IMPORT '%s'\n", moduleName.c_str() ));
		}

		// finalize
		if ( finish )
		{
			success &= ImportFinish();
		}
	}

	return ( success );
}

bool SysExports :: ImportModules( const char* modulePathSpec, ExportColl* dest, bool finish )
{
	// cannot add to existing import set - i'm not set up for that.
	gpassert( !m_ImportComplete );

	// figure out path prefix
	const char* pathEnd = FileSys::GetPathEnd( modulePathSpec );

	// convention is to always process in alphabetical order so it's consistent,
	// and not up to the file system to decide.
	StringSet modules;

	// find all files matching the pattern
	FileSys::FileFinder finder( modulePathSpec );
	gpstring module;
	while ( finder.Next( module ) )
	{
		// add with full path prefix
		module.insert( 0, modulePathSpec, pathEnd - modulePathSpec );
		modules.insert( module );
	}

	// now import each
	StringSet::const_iterator i, ibegin = modules.begin(), iend = modules.end();
	bool success = true;
	for ( i = ibegin ; i != iend ; ++i )
	{
		HMODULE module = ::LoadLibrary( *i );
		if ( module != NULL )
		{
			success &= ImportBindings( module, dest, false );
		}
		else
		{
			DWORD err = ::GetLastError();
			gperrorf(( "LoadLibrary() failure on '%s', error is '%s' (0x%08X)\n",
					   i->c_str(), stringtool::GetLastErrorText( err ).c_str(), err ));
			success = false;
		}
	}

	// if we want to finish and weren't just interested in exports, do it
	if ( (dest == NULL) && finish )
	{
		success &= ImportFinish();
	}

	// done
	return ( success );
}

bool SysExports :: ImportFinish( void )
{
	// cannot finish twice!
	if ( m_ImportComplete )
	{
		return ( true );
	}

	bool success = true;
	const ClassSpec* eventSpec = FindEventNamespace();
	eVarType skritObjectType = FindSkritObject()->m_Type;

	// postprocess global functions
	PostProcess( m_GlobalFunctions );

	// fix up classes
	{
		ClassByNameMap::iterator i, begin = m_Classes.begin(), end = m_Classes.end();
		for ( i = begin ; i != end ; ++i )
		{
			ClassSpec& spec = *i->second;

			gpassert( !(spec.m_Flags & ClassSpec::FLAG_POSTPROCESSED) );
			spec.PostProcess();
			spec.m_Flags |= ClassSpec::FLAG_POSTPROCESSED;

			m_Digest = spec.AddExtraDigest( m_Digest );
		}
	}

	// postprocess our enums
	{
		EnumIndex::iterator i, begin = m_EnumVarTypeIndex.begin(), end = m_EnumVarTypeIndex.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->PostProcess();
		}

		begin = m_IrregularEnums.begin(), end = m_IrregularEnums.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->PostProcess();
		}
	}

	// postprocess SKRIT_IMPORT and DECLARE_EVENT functions
	{
		FunctionByAddrMap::iterator i, begin = m_Functions.begin(), end = m_Functions.end();
		for ( i = begin ; i != end ; ++i )
		{
			FunctionSpec& spec = i->second;
			if (   (spec.m_ParamSpecs.size() >= 2)		// object + function name
				&& (spec.m_ParamSpecs[ 0 ].m_Type.m_Type == skritObjectType)
				&& (spec.m_ParamSpecs[ 1 ].m_Type.IsCString()) )
			{
				spec.m_Flags |= FunctionSpec::FLAG_HIDDEN | FunctionSpec::FLAG_SKRIT_IMPORT;
			}

			if ( spec.m_Parent == eventSpec )
			{
				spec.m_Flags |= FunctionSpec::FLAG_EVENT;
#				if ( !GP_RETAIL )
				{
					if ( !TestOptions( OPTION_NO_STRICT ) && !spec.m_Name.same_no_case( "On", 2 ) )
					{
						gpwarningf(( "FuBi consistency warning for event function '%s' - events should be of the form 'OnEvent'\n",
									 spec.m_Name.c_str() ));
					}
				}
#				endif // !GP_RETAIL
			}
		}
	}

	// final renaming
	ReplaceNameInIndex( m_Classes );
	ReplaceNameInIndex( m_Enums );
	m_RenameComplete = true;

	// check for overloads in event namespace - not supported
#	if ( !GP_RETAIL )
	{
		FunctionByNameIndex::const_iterator
				i,
				begin = eventSpec->m_Functions.begin(),
				end   = eventSpec->m_Functions.end();
		for ( i = begin ; i != end ; ++i )
		{
			// check for overloads
			if ( i->second.size() != 1 )
			{
				gperrorf(( "%s namespace contains overloaded function '%s' - not supported by FuBi/Skrit\n",
							   eventSpec->m_Name.c_str(), i->first.c_str() ));
				success = false;
			}
			const FunctionSpec* funcSpec = *i->second.begin();

			// check for vararg
			if ( funcSpec->m_Flags & FunctionSpec::FLAG_VARARG )
			{
				gperrorf(( "%s namespace cannot contain functions using a variable argument list ('%s')\n",
							   eventSpec->m_Name.c_str(), i->first.c_str() ));
				success = false;
			}

			// first param must be skrit object
			if ( funcSpec->m_ParamSpecs.empty() )
			{
				gperrorf(( "Event function '%s' must at least take a Skrit object as its first param\n",
							   i->first.c_str() ));
				success = false;
			}
			else
			{
				const TypeSpec& firstParam = funcSpec->m_ParamSpecs.begin()->m_Type;
				if (   (firstParam.m_Type != skritObjectType)
					|| !(firstParam.m_Flags & (TypeSpec::FLAG_POINTER | TypeSpec::FLAG_HANDLE | TypeSpec::FLAG_REFERENCE)) )
				{
					gperrorf(( "Event function '%s' must at least take a Skrit object as its first param\n",
								   i->first.c_str() ));
					success = false;
				}
			}
		}
	}
#	endif // !GP_RETAIL

	// check to make sure we've used all our replacements
#	if ( GP_DEBUG )
	{
		if ( !TestOptions( OPTION_NO_STRICT ) )
		{
			NameReplaceSpec::ReplaceColl coll;
			NameReplaceSpec::GetUnusedReplacements( coll );

			if ( !coll.empty() )
			{
				ReportSys::AutoReport autoReport( &gErrorContext );
				gperrorf(( "Found unused global FuBi name replacement specs:\n" ));
				ReportSys::AutoIndent autoIndent( &gErrorContext );

				NameReplaceSpec::ReplaceColl::const_iterator i, begin = coll.begin(), end = coll.end();
				for ( i = begin ; i != end ; ++i )
				{
					gperrorf(( "%s -> %s\n", i->first, i->second ));
				}
			}
		}
	}
#	endif // GP_DEBUG

	// final step is to include module filenames and CRC's in overall digest
	{
		ModuleColl::const_iterator i, ibegin = m_Modules.begin(), iend = m_Modules.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpstring fileName = FileSys::GetFileName( i->m_Name );
			fileName.to_lower();
			m_Digest = i->m_ActualCrc + GetCRC32( m_Digest, fileName.c_str(), fileName.length() + 1 );
		}
	}

	// import all done - no more data
	m_ImportComplete = true;

	// verify that everything ok
	GPDEBUG_ONLY( CheckSanity() );

	// done
	return ( success );
}

const FunctionSpec* SysExports :: FindFunctionByAddrNear( DWORD ptr, const char* funcName, int funcNameLen ) const
{
	gpassert( (funcName != NULL) && (::strlen( funcName ) == (size_t)funcNameLen) );

	// same as FindFunctionExact() but meant for finding what function we're
	//  currently executing in.

	FunctionByAddrMap::const_iterator found = m_Functions.lower_bound( ptr );

	// three conditions from lower_bound:
	//
	// 1. found == end() - found points to the iterator AFTER the one we want
	// 2. found points to the iterator AFTER the one we want
	// 3. found == begin() - ptr is not pointing to a valid function

	if ( found == m_Functions.begin() )
	{
		// invalid function
		return ( NULL );
	}
	else
	{
		// we're at least one past what we want, but it's a multimap - go to the front
		--found;
		DWORD addr = found->first;
		while ( (found != m_Functions.begin()) && (found->first == addr) )
		{
			// seek backwards
			--found;
		}

		// now we're one before what we want
		++found;

		// all done
		return ( FindFunctionByAddrHelper( funcName, funcNameLen, found ) );
	}
}

const FunctionIndex* SysExports :: FindFunctionsByQualifiedName( const char* funcName ) const
{
	const FunctionIndex* funcSpec = NULL;

	const char* start = ::strstr( funcName, "::" );
	if ( start != NULL )
	{
		const ClassSpec* classSpec = FindClass( gpstring( funcName, start ) );
		if ( classSpec != NULL )
		{
			FunctionByNameIndex::const_iterator found = classSpec->m_Functions.find( start + 2 );
			if ( found != classSpec->m_Functions.end() )
			{
				funcSpec = &found->second;
			}
		}
	}
	else
	{
		funcSpec = FindGlobalFunction( funcName );
	}

	return ( funcSpec );
}

int SysExports :: FindFunctionsByQualifiedName( FunctionIndexColl& coll, const char* funcName ) const
{
	int oldSize = coll.size();

	const char* start = ::strstr( funcName, "::" );
	if ( start != NULL )
	{
		const ClassSpec* classSpec = FindClass( gpstring( funcName, start ) );
		if ( classSpec != NULL )
		{
			const char* memberName = start + 2;
			if ( stringtool::HasDosWildcards( memberName ) )
			{
				FunctionByNameIndex::const_iterator i, ibegin = classSpec->m_Functions.begin(), iend = classSpec->m_Functions.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( stringtool::IsDosWildcardMatch( memberName, i->first ) )
					{
						coll.push_back( &i->second );
					}
				}
			}
			else
			{
				FunctionByNameIndex::const_iterator found = classSpec->m_Functions.find( memberName );
				if ( found != classSpec->m_Functions.end() )
				{
					coll.push_back( &found->second );
				}
			}
		}
	}
	else
	{
		const FunctionIndex* found = FindGlobalFunction( funcName );
		if ( found != NULL )
		{
			coll.push_back( found );
		}
	}

	return ( coll.size() - oldSize );
}

UINT SysExports :: FindEventSerialByName( const char* eventName ) const
{
	UINT serialId = INVALID_FUNCTION_SERIAL;

	const ClassSpec* classSpec = FindEventNamespace();
	if ( classSpec != NULL )
	{
		FunctionByNameIndex::const_iterator found = classSpec->m_Functions.find( eventName );
		if ( found != classSpec->m_Functions.end() )
		{
			gpassert( !found->second.empty() );
			serialId = found->second[ 0 ]->m_SerialID;
		}
	}

	return ( serialId );
}

int SysExports :: GetFunctionSize( const FunctionSpec* spec )
{
	gpassert( spec != NULL );
	FunctionByAddrMap::const_iterator found = m_Functions.find( spec->m_FunctionPtr );
	gpassert( found != m_Functions.end() );
	FunctionByAddrMap::const_iterator next = found;
	++next;
	if ( next != m_Functions.end() )
	{
		return ( next->second.m_FunctionPtr - spec->m_FunctionPtr );
	}
	else
	{
		return ( -1 );
	}
}

const char* SysExports :: FindTypeName( eVarType type ) const
{
	const char* name = NULL;
	if ( IsUser( type ) )
	{
		type -= VAR_USER;
		gpassert( (type >= 0) && (type < scast <int> ( m_ClassVarTypeIndex.size() )) );
		name = m_ClassVarTypeIndex[ type ]->m_Name;
	}
	else if ( IsEnum( type ) )
	{
		type -= VAR_ENUM;
		gpassert( (type >= 0) && (type < scast <int> ( m_EnumVarTypeIndex.size() )) );
		name = m_EnumVarTypeIndex[ type ]->m_Name;
	}
	else
	{
		const VarTypeSpec* spec = GetVarTypeSpec( type );
		if ( spec != NULL )
		{
			name = spec->m_ExternalName;
		}
	}
	return ( name );
}

const char* SysExports :: FindNiceTypeName( const FuBi::TypeSpec& spec ) const
{
	if ( spec.m_Type == FuBi::VAR_UNKNOWN )
	{
		return ( "<unknown>" );
	}
	else if ( spec.IsCString() )
	{
		return ( "string" );
	}
	else
	{
		return ( FindTypeName( spec.m_Type ) );
	}
}

gpstring SysExports :: MakeFullTypeName( const FuBi::TypeSpec& spec ) const
{
	gpstring base( FindNiceTypeName( spec ) );
	if ( spec.m_Flags & FuBi::TypeSpec::FLAG_HANDLE )
	{
		base.insert( 0, "H" );
	}
	else if ( spec.m_Flags & (FuBi::TypeSpec::FLAG_REFERENCE | FuBi::TypeSpec::FLAG_POINTER) )
	{
		base.append( "*" );
	}
	return ( base );
}

eVarType SysExports :: FindType( const char* name ) const
{
	gpassert( (name != NULL) && (*name != '\0') );

	// class?
	ClassByNameMap::const_iterator cfound = m_Classes.find( name );
	if ( cfound != m_Classes.end() )
	{
		return ( cfound->second->m_Type );
	}

	// enum?
	EnumByNameMap::const_iterator efound = m_Enums.find( name );
	if ( efound != m_Enums.end() )
	{
		return ( efound->second->m_Type );
	}

	// failure
	return ( VAR_UNKNOWN );
}

eVarType SysExports :: FindTypeInternalOk( const char* name ) const
{
	eVarType type = FindType( name );
	if ( type == VAR_UNKNOWN )
	{
		// check internal set
		VarTypeSpecColl::const_iterator i, begin = m_VarTypeSpecs.begin(), end = m_VarTypeSpecs.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( ::stricmp( name, i->m_ExternalName ) == 0 )
			{
				int index = i - begin;
				if ( index < VAR_COUNT )
				{
					type = scast <eVarType> ( VAR_BEGIN + index );
				}
				else
				{
					type = scast <eVarType> ( VAR_SPECIAL_BEGIN + index - VAR_COUNT );
				}
				break;
			}
		}
	}

	return ( type );
}

eVarType SysExports :: FindOrCreateType( const char* name )
{
	gpassert( (name != NULL) && (*name != '\0') );

	// optional rename test
	if ( !m_RenameComplete )
	{
		const char* replace = NameReplaceSpec::GetReplacement( name );
		if ( replace != NULL )
		{
			name = replace;
		}
	}

	return ( GetClass( name )->m_Type );
}

ClassSpec* SysExports :: GetClass( const gpstring& name )
{
	gpassert( !m_ImportComplete );

	ClassByNameMapInsertRet rc = m_Classes.insert( std::make_pair( name, (ClassSpec*)NULL ) );
	ClassSpec*& spec = rc.first->second;

	// new insertion - initialize it
	if ( rc.second )
	{
		// set it up
		spec = new ClassSpec;
		spec->m_Name = name;			// uses same string as the key (ref counting!)
		spec->m_Type = scast <eVarType> ( m_ClassVarTypeIndex.size() ) + VAR_USER;

		// lots of stuff depends on eVarType fitting into a word
		gpassert( spec->m_Type < std::numeric_limits <WORD>::max() );

		// add to user coll map
		m_ClassVarTypeIndex.push_back( spec );

		// check for names that are the same as keywords - not possible
#		if !GP_RETAIL
		if ( IsKeyword( name ) )
		{
			gperrorf(( "FUBI type name '%s' uses a keyword\n", name.c_str() ));
		}
#		endif // !GP_RETAIL
	}

	return ( spec );
}

EnumSpec* SysExports :: GetEnum( const gpstring& name )
{
	gpassert( !m_ImportComplete );

	// check for name replacement
	gpstring localName( name );
	const char* newName = NameReplaceSpec::GetReplacement( localName );
	if ( newName != NULL )
	{
		localName = newName;
	}

	// insert it
	EnumByNameMapInsertRet rc = m_Enums.insert( std::make_pair( localName, (EnumSpec*)NULL ) );
	EnumSpec*& spec = rc.first->second;

	// new insertion - initialize it
	if ( rc.second )
	{
		// set it up
		spec = new EnumSpec;
		spec->m_Name = localName;		// uses same string as the key (ref counting!)
		spec->m_Type = scast <eVarType> ( m_EnumVarTypeIndex.size() ) + VAR_ENUM;

		// lots of stuff depends on eVarType fitting into a word
		gpassert( spec->m_Type < std::numeric_limits <WORD>::max() );

		// add to user coll map
		m_EnumVarTypeIndex.push_back( spec );
	}

	return ( spec );
}

const EnumSpec* SysExports :: FindEnumConstant( const char* name, DWORD& value, bool fastOnly ) const
{
	// first try our normal set (fast)
	StringToEnumMap::const_iterator found = m_EnumConstants.find( name );
	if ( found != m_EnumConstants.end() )
	{
		value = found->second.second;
		return ( m_EnumVarTypeIndex[ found->second.first - VAR_ENUM ] );
	}

	// only search irregular set if we didn't specify the normal set only
	if ( !fastOnly )
	{
		// now try our irregular set (slower)
		EnumIndex::const_iterator i, begin = m_IrregularEnums.begin(), end = m_IrregularEnums.end();
		for ( i = begin ; i != end ; ++i )
		{
			const EnumSpec* spec = *i;
			if ( spec->m_Exporter.m_EnumFromFullStringProc != NULL )
			{
				if ( (*spec->m_Exporter.m_EnumFromFullStringProc)( name, value ) )
				{
					return ( spec );
				}
			}
			else
			{
				gpassert( spec->m_Exporter.m_EnumFromStringProc != NULL );
				if ( (*spec->m_Exporter.m_EnumFromStringProc)( name, value ) )
				{
					return ( spec );
				}
			}
		}
	}

	// didn't find anything
	return ( NULL );
}

const VarTypeSpec* SysExports :: GetVarTypeSpec( eVarType type ) const
{
	// check unknown
	if ( type <= VAR_UNKNOWN )
	{
		return ( NULL );
	}

	// check standard
	if ( type < VAR_END )
	{
		return ( &m_VarTypeSpecs[ type - VAR_BEGIN ] );
	}

	// check special types
	if ( type < VAR_SPECIAL_END )
	{
		return ( &m_VarTypeSpecs[ type - VAR_SPECIAL_BEGIN + VAR_COUNT ] );
	}

	// check enums
	if ( type < VAR_ENUM_END )
	{
		const EnumSpec* enm = FindEnum( type );
		gpassert( enm != NULL );
		return ( &enm->m_VarTypeSpec );
	}

	// check user defined types
	const ClassSpec* cls = FindClass( type );
	gpassert( cls != NULL );
	return ( cls->m_VarTypeSpec );
}

int SysExports :: GetVarTypeSizeBytes( eVarType type ) const
{
	if ( IsUser( type ) )
	{
		const ClassSpec* cls = FindClass( type );
		gpassert( cls != NULL );

		if ( cls->m_VarTypeSpec != NULL )
		{
			const VarTypeSpec* spec = GetVarTypeSpec( type );
			gpassert( spec != NULL );
			gpassert( spec->m_SizeBytes >= 0 );
			return ( spec->m_SizeBytes );
		}
		else
		{
			gpassert( IsPod( type ) );
			gpassert( cls->m_Flags & ClassSpec::FLAG_POINTERCLASS );
			return ( sizeof( void* ) );
		}
	}
	else if ( IsEnum( type ) )
	{
		return ( sizeof( type ) );		// $ same size as any other enum
	}
	else
	{
		const VarTypeSpec* spec = GetVarTypeSpec( type );
		gpassert( spec != NULL );
		gpassert( spec->m_SizeBytes >= 0 );
		return ( spec->m_SizeBytes );
	}
}

bool SysExports :: IsSimpleInteger( eVarType type ) const
{
	bool is = false;

	const VarTypeSpec* spec = GetVarTypeSpec( type );
	if ( spec != NULL )
	{
		is = (spec->m_SizeBytes <= 4) && (spec->m_Flags & Trait::FLAG_INTEGER);
	}

	return ( is );
}

bool SysExports :: IsSimpleFloat( eVarType type ) const
{
	bool is = false;

	const VarTypeSpec* spec = GetVarTypeSpec( type );
	if ( spec != NULL )
	{
		is = (spec->m_SizeBytes <= 4) && (spec->m_Flags & Trait::FLAG_FLOAT);
	}

	return ( is );
}

bool SysExports :: IsPod( eVarType type ) const
{
	if ( IsUser( type ) )
	{
		const ClassSpec* cls = FindClass( type );
		gpassert( cls != NULL );

		if ( cls->m_VarTypeSpec != NULL )
		{
			return ( !!(cls->m_VarTypeSpec->m_Flags & Trait::FLAG_POD) );
		}
		else
		{
			return ( !!(cls->m_Flags & ClassSpec::FLAG_POINTERCLASS) );
		}
	}
	else if ( IsEnum( type ) )
	{
		return ( true );
	}
	else
	{
		const VarTypeSpec* spec = GetVarTypeSpec( type );
		gpassert( spec != NULL );
		return ( !!(spec->m_Flags & Trait::FLAG_POD) );
	}
}

bool SysExports :: IsKeyword( const gpstring& name ) const
{
	return ( m_Keywords.find( name ) != m_Keywords.end() );
}

bool SysExports :: IsDerivative( eVarType derived, eVarType base ) const
{
	const ClassSpec* cls = FindClass( derived );
	return ( (cls != NULL) && cls->IsDerivedFrom( base ) );
}

int SysExports :: GetDerivedBaseOffset( eVarType derived, eVarType base ) const
{
	if ( derived == base )
	{
		return ( 0 );
	}

	const ClassSpec* cls = FindClass( derived );
	gpassert( cls != NULL );
	return ( cls->GetDerivedBaseOffset( base ) );
}

bool SysExports :: ToString( eVarType type, gpstring& out, const void* obj, eXfer xfer, eXMode xmode ) const
{
	bool success = true;

	if ( type == VAR_STRING )
	{
		out = *rcast <const gpstring*> ( obj );
	}
	else if ( type == VAR_WSTRING )
	{
		out = ::ToAnsi( *rcast <const gpwstring*> ( obj ) );
	}
	else if ( IsEnum( type ) )
	{
		const EnumSpec* spec = FindEnum( type );
		gpassert( spec != NULL );
		if ( spec->m_Exporter.m_EnumToFullStringProc != NULL )
		{
			out = (*spec->m_Exporter.m_EnumToFullStringProc)( *rcast <const DWORD*> ( obj ) );
		}
		else if ( spec->m_Exporter.m_EnumToStringProc != NULL )
		{
			out = (*spec->m_Exporter.m_EnumToStringProc)( *rcast <const DWORD*> ( obj ) );
		}
		else
		{
			out.assignf( "0x%08X", *rcast <const DWORD*> ( obj ) );
		}
	}
	else
	{
		const VarTypeSpec* vspec = GetVarTypeSpec( type );
		if ( (vspec != NULL) && (vspec->m_ToStringProc != NULL) )
		{
			(*vspec->m_ToStringProc)( out, obj, xfer );
		}
		else if ( IsUser( type ) && (FindClass( type )->m_Flags & ClassSpec::FLAG_POINTERCLASS) )
		{
			::ToString( out, *rcast <const DWORD*> ( obj ), XFER_HEX );
		}
		else
		{
#			if !GP_RETAIL
			if ( ShouldReportConversionErrors( xmode ) )
			{
				gpreportf( &gXferErrors, ( "ToString() conversion not implemented for type '%s'\n",
						   FindTypeName( type ) ));
			}
#			endif // !GP_RETAIL
			success = false;
		}
	}

	return ( success );
}

bool SysExports :: ToString( const TypeSpec& type, gpstring& out, const void* obj, eXfer xfer, eXMode xmode ) const
{
	bool success = true;

	// special handling for const char* etc.
	if ( type.IsCString() )
	{
		out = *rcast <const char* const*> ( obj );
	}
	else if ( type.IsCStringW() )
	{
		out = ::ToAnsi( *rcast <const wchar_t* const*> ( obj ) );
	}
	else if ( !type.IsPassByValue() && !type.IsPointerClass() )
	{
		// $ treat pointers as dwords for now
		success = ToString( VAR_DWORD, out, &obj, XFER_HEX, xmode );
	}
	else
	{
		success = ToString( type.m_Type, out, obj, xfer, xmode );
	}

	return ( success );
}

bool SysExports :: FromString( eVarType type, const char* str, void* obj, eXMode xmode ) const
{
	bool success = true;

	if ( type == VAR_STRING )
	{
		*rcast <gpstring*> ( obj ) = str;
	}
	else if ( type == VAR_WSTRING )
	{
		*rcast <gpwstring*> ( obj ) = ::ToUnicode( str );
	}
	else if ( IsEnum( type ) )
	{
		success = FindEnumConstant( str, *rcast <DWORD*> ( obj ) ) != NULL;
		if ( !success )
		{
			// try it as an integer
			success = ::FromString( str, *rcast <DWORD*> ( obj ) );
		}
	}
	else
	{
		const VarTypeSpec* vspec = GetVarTypeSpec( type );
		if ( (vspec != NULL) && (vspec->m_FromStringProc != NULL) )
		{
			success = (*vspec->m_FromStringProc)( str, obj );
		}
		else if ( IsUser( type ) && (FindClass( type )->m_Flags & ClassSpec::FLAG_POINTERCLASS) )
		{
			success = ::FromString( str, *rcast <DWORD*> ( obj ) );
		}
		else
		{
#			if !GP_RETAIL
			gpreportf( &gXferErrors, ( "FromString() conversion not implemented/exported for type '%s'\n",
					   FindTypeName( type ) ));
#			endif // !GP_RETAIL
			success = false;
		}

#		if !GP_RETAIL
		if ( !success && ShouldReportConversionErrors( xmode ) )
		{
			gpreportf( &gXferErrors, ( "FromString() conversion failed for type '%s' (data is '%s')\n",
					   FindTypeName( type ), stringtool::EscapeCharactersToTokens( str ).c_str() ));
		}
#		endif // !GP_RETAIL
	}

	return ( success );
}

bool SysExports :: FromString( const TypeSpec& type, const char* str, void* obj, eXMode xmode ) const
{
	// $ treat pointers as dwords for now
	TypeSpec spec = type;
	if ( !spec.IsPassByValue() )
	{
		spec = VAR_DWORD;
	}

	return ( FromString( spec.m_Type, str, obj, xmode ) );
}

bool SysExports :: CopyVar( eVarType type, void* dst, const void* src, eXMode xmode ) const
{
	bool success = true;

	if ( type == VAR_STRING )
	{
		*rcast <gpstring*> ( dst ) = *rcast <const gpstring*> ( src );
	}
	else if ( type == VAR_WSTRING )
	{
		*rcast <gpwstring*> ( dst ) = *rcast <const gpwstring*> ( src );
	}
	else
	{
		const VarTypeSpec* vspec = GetVarTypeSpec( type );
		if ( (vspec != NULL) && (vspec->m_CopyVarProc != NULL) )
		{
			(*vspec->m_CopyVarProc)( dst, src );
		}
		else if ( (vspec != NULL) && (vspec->m_Flags & Trait::FLAG_POD) )
		{
			gpassert( vspec->m_SizeBytes > 0 );
			::memcpy( dst, src, vspec->m_SizeBytes );
		}
		else
		{
			// the only remaining way we can do this is using string conversions
			// as the common language
			gpstring temp;
			success = ToString( type, temp, src, XFER_NORMAL, xmode ) && FromString( type, temp, dst, xmode );
		}
	}
	return ( success );
}

bool SysExports :: CopyVar( const TypeSpec& type, void* dst, const void* src, eXMode xmode ) const
{
	if ( type.IsPassByValue() )
	{
		return ( CopyVar( type.m_Type, dst, src, xmode ) );
	}
	else
	{
		// simple pointer copy
		*rcast <DWORD*> ( dst ) = *rcast <const DWORD*> ( src );
		return ( true );
	}
}

int SysExports :: CompareVar( eVarType type, const void* l, const void* r ) const
{
	if ( type == VAR_STRING )
	{
		return ( rcast <const gpstring*> ( l )->compare_with_case( *rcast <const gpstring*> ( r ) ) );
	}
	else if ( type == VAR_WSTRING )
	{
		return ( rcast <const gpwstring*> ( l )->compare_with_case( *rcast <const gpwstring*> ( r ) ) );
	}
	else
	{
		const VarTypeSpec* vspec = GetVarTypeSpec( type );
		if ( (vspec != NULL) && (vspec->m_Flags & Trait::FLAG_POD) )
		{
			gpassert( vspec->m_SizeBytes > 0 );
			return ( ::memcmp( l, r, vspec->m_SizeBytes ) );
		}
	}

	// can't compare with this type yet!!
	gpassert( 0 );
	return ( 0 );
}

int SysExports :: CompareVar( const TypeSpec& type, const void* l, const void* r ) const
{
	if ( type.IsPassByValue() )
	{
		return ( CompareVar( type.m_Type, l, r ) );
	}
	else
	{
		// simple pointer test
		const DWORD& ld = *rcast <const DWORD*> ( l );
		const DWORD& rd = *rcast <const DWORD*> ( r );
		if ( ld < rd )
		{
			return ( -1 );
		}
		else if ( ld > rd )
		{
			return ( 1 );
		}
		else
		{
			return ( 0 );
		}
	}
}

const FunctionSpec* SysExports :: ResolveRpc( DWORD EIP, const char* funcName, int funcNameLen ) const
{
	// in here for dev modes we attempt to limp along by returning null, which
	// will abort the RPC, rather than just crashing.

	// find the function
	const FunctionSpec* spec = FindFunctionByAddrNear( EIP, funcName, funcNameLen );

#	if !GP_RETAIL

	// verify it even exists
	if ( spec == NULL )
	{
		gperrorf(( "RPC: unable to find FuBi function from EIP = 0x%08X (should be '%s')\n",
				    EIP, funcName ));
		return ( NULL );
	}

	// verify it can be rpc'd
	if ( !(spec->m_Flags & FunctionSpec::FLAG_CAN_RPC) || (spec->m_Flags & FunctionSpec::FLAG_VARARG) )
	{
		gperrorf(( "RPC: function '%s' cannot be called remotely (check its signature)\n",
				    spec->BuildQualifiedName().c_str() ));
		return ( NULL );
	}

#	endif // !GP_RETAIL

	return ( spec );
}

Cookie SysExports :: SendRpc( const FunctionSpec* spec, void* thisParam, const void* firstParam, DWORD toAddr )
{
	// early bailout if we don't have a transport (common in SP games)
	if ( !ShouldSetAddresses() )
	{
		return ( RPC_SUCCESS );
	}

	gpassert( spec != NULL );
	gpassert( !::IsMemoryBadFood( (DWORD)thisParam ) );
	gpassert( !::IsMemoryBadFood( (DWORD)firstParam ) );

	// check server permissions
	if ( spec->m_Flags & FunctionSpec::FLAG_CHECK_SERVER_ONLY )
	{
		if ( !IsServerLocal() )
		{
			// permission failure!
			gperrorf(( "SysExports::SendRpc(): illegal send of "
					   "SERVER-only RPC from non-server machine!\n"
					   "  RPC = %s\n", spec->BuildQualifiedName().c_str() ));
			return ( RPC_FAILURE_IGNORE );
		}
	}

	// serialize RPC requests
	Critical::Lock locker( m_SendCritical );

	struct AutoClear
	{
		bool& m_B;

		AutoClear( bool& b ) : m_B( b )  {  gpassert( !m_B );  m_B = true;  }
	   ~AutoClear( void )  {  gpassert( m_B );  m_B = false;  }
	};

	AutoClear autoClear( ms_IsSending );

// Preprocess.

	// get local thread's rpc call spec - one must have been added by auto tag ctor
	RpcVerifyColl& coll = GetRpcVerifyColl();
	gpassert( !coll.empty() );

	// set spec for call on stack
	RpcVerifySpec& verifySpec = *coll.rbegin();
	verifySpec.m_Function = spec;
	verifySpec.m_Params = const_mem_ptr( firstParam, 0 );
	verifySpec.m_This = thisParam;
	gpassert( verifySpec.m_Address == toAddr );

	// verify that we are ready for rpc's
	gpassert( m_RpcSyncComplete || TestOptions( OPTION_NO_REQUIRE_SYNC ) );

	// verify that we're using the proper macro - it needs a "THIS_" if we're
	// calling a nonstatic method. note that if we're calling it on a singleton
	// we don't *need* the "this" pointer.
#	if ( GP_DEBUG )
	{
		if ( spec->m_Flags & FunctionSpec::FLAG_CALL_THISCALL )
		{
			const ClassSpec* classSpec = spec->m_Parent;
			gpassertf( (classSpec != NULL) && (classSpec->m_Flags & ClassSpec::FLAG_CANRPC),
					 ( "Attempted to RPC-send from a non-RPCable function '%s'!",
					   spec->BuildQualifiedName().c_str() ));
			if ( !(classSpec->m_Flags & ClassSpec::FLAG_SINGLETON) )
			{
				gpassertf( thisParam != NULL,
						 ( "Incorrect macro usage for FuBi RPC's for function "
						   "'%s'! Did you forget the '_THIS'?",
						   spec->BuildQualifiedName().c_str() ));
			}
		}
	}
#	endif // GP_DEBUG

	// verify we have a transport again - this time we do it after we verify
	// some stuff, so non-retail can catch some errors.
#	if !GP_RETAIL
	if ( !NetSendTransport::DoesSingletonExist() )
	{
		return ( RPC_SUCCESS );
	}
#	endif // !GP_RETAIL

	// ask callback to reprocess the RPC stack for addressing on a broadcast
	AddressColl addresses;
	eBroadcastType broadcastType = BROADCAST_NORMAL;
	if ( ((toAddr == RPC_TO_ALL) || (toAddr == RPC_TO_OTHERS)) && (m_Rebroadcaster != NULL) )
	{
		// calc rebroadcast type
		broadcastType = m_Rebroadcaster->OnProcessBroadcastRpc( verifySpec, addresses );

		// see how to handle the addressing
		if ( broadcastType == BROADCAST_ABORT )
		{
			// broadcaster wanted to abort, no need to continue! early bailout!
			return ( RPC_SUCCESS );
		}
		else if ( IsNormal( broadcastType ) )
		{
			// leave it alone
			addresses.clear();
		}
		else if ( addresses.size() == 1 )
		{
			// only a single shot
			toAddr = addresses.front();

			// is this just for myself?
			if ( toAddr == RPC_TO_LOCAL )
			{
				// $ success! abort! early bailout
				return ( RPC_SUCCESS );
			}

			// take it
			verifySpec.m_Address = toAddr;
			addresses.clear();
		}
		else if ( addresses.empty() )
		{
			// broadcaster removed all addresses, no need to continue! early bailout!
			return ( RPC_SUCCESS );
		}
	}

	// ask callback to verify the RPC stack
	if ( (m_Rebroadcaster != NULL) && !IsDeferred( broadcastType ) && !AssertData::IsAssertOccurring() )
	{
		if ( !m_Rebroadcaster->OnVerifySendRpc( coll, addresses, false ) )
		{
			// client doesn't want to allow this
			return ( RPC_FAILURE_IGNORE );
		}
	}

	// if we were supposed to defer the rpc, temporarily redirect
	if ( IsDeferred( broadcastType ) )
	{
		gpassert( m_Rebroadcaster != NULL );
		m_Rebroadcaster->OnBeginRedirect( coll );
	}

	// begin the rpc
	bool isRetrying = (spec->m_Flags & FunctionSpec::FLAG_RETRY) != 0;
	if ( addresses.empty() )
	{
		gFuBiNetSendTransport.BeginRPC( toAddr, isRetrying );
	}
	else
	{
		gFuBiNetSendTransport.BeginMultiRPC( &*addresses.begin(), addresses.size(), isRetrying );
	}

// Pack parameters.

	// pack function serial ID
	gFuBiNetSendTransport.Store( scast <WORD> ( spec->m_SerialID ) );

	// manage packed memory
	int packedParamSizeBytes = spec->m_ParamSizeBytes;
	FunctionSpec::eFlags packType = spec->m_Flags & (FunctionSpec::FLAG_CAN_PACK_FRONT | FunctionSpec::FLAG_CAN_PACK_BACK);
	if ( packType != FunctionSpec::FLAG_NONE )
	{
		// total param size reduced now that it's specially packed
		packedParamSizeBytes -= sizeof( const_mem_ptr );

		// get first param
		const const_mem_ptr* mem = (const const_mem_ptr*)firstParam;
		if ( packType == FunctionSpec::FLAG_CAN_PACK_BACK )
		{
			// oops, we want last param
			mem = (const const_mem_ptr*)((const BYTE*)mem + packedParamSizeBytes);
		}

		// ok now pack the front/back memory
		gFuBiNetSendTransport.Store( mem->mem, mem->size );

		// skip first param for main storage if necessary
		if ( packType == FunctionSpec::FLAG_CAN_PACK_FRONT )
		{
			firstParam = (const BYTE*)firstParam + sizeof( const_mem_ptr );
		}
	}

	// pack parameters
	UINT paramIter = gFuBiNetSendTransport.Store( firstParam, packedParamSizeBytes );

	// pack riders and special data
	if ( !(spec->m_Flags & FunctionSpec::FLAG_SIMPLE_ARGS) )
	{
		// skip if necessary
		FunctionSpec::ParamSpecs::const_iterator i, begin = spec->m_ParamSpecs.begin(), end = spec->m_ParamSpecs.end();
		if ( packType == FunctionSpec::FLAG_CAN_PACK_FRONT )
		{
			++begin;
		}
		else if ( packType == FunctionSpec::FLAG_CAN_PACK_BACK )
		{
			--end;
		}

		// process other params as needed
		for ( i = begin ; i != end ; ++i )
		{
			// note: in all of the below code - any call to Send() will
			//       invalidate any pointers to the contents of the transport's
			//       buffer - due to vector's demand-reallocation.

			// process special data
			switch ( i->m_Type.m_Type )
			{
				case ( VAR_MEM_PTR ):
				case ( VAR_CONST_MEM_PTR ):
				{
					// get at the mem_ptr param
					mem_ptr* param = rcast <mem_ptr*> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
					gpassert( param->size < 0x10000 );
					void* oldParam = param->mem;
					// clear pointer to help compression
					param->mem = NULL;
					// store rider
					gFuBiNetSendTransport.Store( oldParam, param->size );
				}	break;

				case ( VAR_CHAR ):
				{
					if ( i->m_Type.m_Flags == (TypeSpec::FLAG_POINTER | TypeSpec::FLAG_CONST) )
					{
						// get at the const char* param
						const char** param = rcast <const char**> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
						// copy what it was pointing to, get its size (check for NULL)
						const char* oldParam = *param;
						int oldParamLen = (oldParam == NULL) ? 0 : (::strlen( oldParam ) + 1);
						// set first DWORD to size of the rider
						*rcast <DWORD*> ( param ) = oldParamLen;
						// store rider
						gFuBiNetSendTransport.Store( oldParam, oldParamLen );
					}
				}	break;

				case ( VAR_USHORT ):
				{
					if ( i->m_Type.m_Flags == (TypeSpec::FLAG_POINTER | TypeSpec::FLAG_CONST) )
					{
						// get at the const wchar_t* param
						const wchar_t** param = rcast <const wchar_t**> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
						// copy what it was pointing to, get its size (check for NULL)
						const wchar_t* oldParam = *param;
						int oldParamLen = (oldParam == NULL) ? 0 : (::wcslen( oldParam ) + 1);
						// set first DWORD to size of the rider
						*rcast <DWORD*> ( param ) = oldParamLen;
						// store rider
						gFuBiNetSendTransport.Store( oldParam, oldParamLen * 2 );
					}
				}	break;

				case ( VAR_STRING ):
				{
#					if !GP_RETAIL
					if ( i->m_Type.IsPassByValue() )
					{
						gperror( "Serious error!! Can't pass a gpstring by value in an RPC!!!\n" );
					}
#					endif // !GP_RETAIL

					// get at the gpstring param
					const gpstring** param = rcast <const gpstring**> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
					// copy what it was pointing to, get its size (check for NULL)
					const gpstring* oldParam = *param;
					gpassert( oldParam != NULL );
					// set first DWORD to size of the rider
					*rcast <DWORD*> ( param ) = oldParam->length() + 1;
					// store rider
					gFuBiNetSendTransport.Store( oldParam->c_str(), oldParam->length() + 1 );
				}	break;

				case ( VAR_WSTRING ):
				{
#					if !GP_RETAIL
					if ( i->m_Type.IsPassByValue() )
					{
						gperror( "Serious error!! Can't pass a gpwstring by value in an RPC!!!\n" );
					}
#					endif // !GP_RETAIL

					// get at the gpwstring param
					const gpwstring** param = rcast <const gpwstring**> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
					// copy what it was pointing to, get its size (check for NULL)
					const gpwstring* oldParam = *param;
					gpassert( oldParam != NULL );
					// set first DWORD to size of the rider
					*rcast <DWORD*> ( param ) = oldParam->length() + 1;
					// store rider
					gFuBiNetSendTransport.Store( oldParam->c_str(), (oldParam->length() + 1) * 2 );
				}	break;

				default:
				{
					// check for POD types
					const VarTypeSpec* spec = gFuBiSysExports.GetVarTypeSpec( i->m_Type.m_Type );
					if ( (spec != NULL) && (spec->m_Flags & Trait::FLAG_POD) )
					{
						// pod types passed by ref can be tacked on to the end
						if ( !i->m_Type.IsPassByValue() )
						{
							// get at the param
							const void** param = rcast <const void**> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
							// copy what it was pointing to
							const void* oldParam = *param;
							// set first DWORD to true/false for NULL
							*rcast <DWORD*> ( param ) = (oldParam != NULL);
							// store rider
							gpassert( spec->m_SizeBytes > 0 );
							if ( oldParam != NULL )
							{
								gFuBiNetSendTransport.Store( oldParam, spec->m_SizeBytes );
							}
						}
					}
					// special user type requires network cookie conversion
					else if ( IsUser( i->m_Type.m_Type ) )
					{
						const ClassSpec* classSpec = gFuBiSysExports.FindClass( i->m_Type.m_Type );

						// don't bother with pointer classes
						if ( !(classSpec->m_Flags & ClassSpec::FLAG_POINTERCLASS) )
						{
							// verify system integrity
							gpassert( (classSpec != NULL) && (classSpec->m_Flags & ClassSpec::FLAG_CANRPC) );
							gpassert( !(classSpec->m_Flags & ClassSpec::FLAG_SINGLETON) );
							gpassert( classSpec->m_InstanceToNetProc != NULL );
							gpassert( !i->m_Type.IsPassByValue() );

							// patch pointer with cookie
							DWORD* param = rcast <DWORD*> ( gFuBiNetSendTransport.GetPtrFromIndex( paramIter ) );
							if ( *param != NULL )
							{
								*param = (*classSpec->m_InstanceToNetProc)( (void*)(*param) );
							}
						}
						else
						{
#							if !GP_RETAIL
							if ( i->m_Type.IsPassByValue() )
							{
								gperror( "Serious error!! Can't pass a 'solid' by value in an RPC!!!\n" );
							}
#							endif // !GP_RETAIL
						}
					}
				}
			}

			// move on to next param
			paramIter += i->m_Type.GetSizeBytes();
		}
	}

	// pack "this" at the end if not a singleton
	DWORD netCookie = 0;
	if ( spec->m_Flags & FunctionSpec::FLAG_CALL_THISCALL )
	{
		const ClassSpec* classSpec = spec->m_Parent;
		gpassert( (classSpec != NULL) && (classSpec->m_Flags & ClassSpec::FLAG_CANRPC) );
		if ( !(classSpec->m_Flags & ClassSpec::FLAG_SINGLETON) )
		{
			gpassert( classSpec->m_InstanceToNetProc != NULL );

			// $ this assert will fire if you have used FUBI_RPC_CALL rather than
			//   FUBI_RPC_THIS_CALL in your exported function.
			gpassert( thisParam != NULL );

			netCookie = (*classSpec->m_InstanceToNetProc)( thisParam );
			gFuBiNetSendTransport.Store( netCookie );
		}
	}

// Seal off the RPC packet.

	// done
	int packetSize = gFuBiNetSendTransport.EndRPC();
	verifySpec.m_Params.size = packetSize;

	// restore old singleton
	if ( IsDeferred( broadcastType ) )
	{
		gpassert( m_Rebroadcaster != NULL );
		m_Rebroadcaster->OnEndRedirect();
	}

	// stats
	if ( m_Rebroadcaster != NULL )
	{
		m_Rebroadcaster->OnSendRpc( spec, netCookie, packetSize );
	}

	// done
	return ( RPC_SUCCESS );
}

bool SysExports :: RpcTestMacro( DWORD addr, const FunctionSpec* spec )
{
#	if GP_DEBUG
	if (   !TestOptions( OPTION_NO_CHECK_ALL_RPC_NESTING )
		&& ((addr == RPC_TO_ALL) || (addr == RPC_TO_OTHERS))
		&& (spec != NULL) )
	{
		// get local thread's rpc call spec - one must have been added by auto tag ctor
		RpcVerifyColl& coll = GetRpcVerifyColl();
		gpassert( !coll.empty() );

		// check to see if we've got a nested "all" dispatch
		int skip = 0;
		RpcVerifyColl::const_iterator i, begin = coll.begin(), end = coll.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( ((i->m_Address == RPC_TO_ALL) || (i->m_Address == RPC_TO_OTHERS)) && (i->m_Function != NULL) )
			{
				if ( skip == 0 )
				{
					// check for a nest detect cancel
					if ( i->IsNestDetectCancel() )
					{
						++skip;
					}
					else
					{
						const FunctionSpec* caller = i->m_Function;
						const FunctionSpec* called = spec;

						// check against exception list
						if (   !std::binary_search( m_NestedRpcExceptionColl.begin(), m_NestedRpcExceptionColl.end(), SerialIdPair( caller->m_SerialID, called->m_SerialID ) )
							&& !std::binary_search( m_NestedRpcXCallers.begin(), m_NestedRpcXCallers.end(), caller->m_SerialID )
							&& !std::binary_search( m_NestedRpcXCalleds.begin(), m_NestedRpcXCalleds.end(), called->m_SerialID ) )
						{
							// the following output may blow out our coll, so preserve it
							RpcVerifyColl localColl;
							localColl.swap( coll );

							// report
							ReportSys::ContextRef ctx = AssertData::IsAssertOccurring() ? &gWarningContext : &gPerfContext;
							ReportSys::OutputF( ctx, "PERFORMANCE WARNING: Nested 'all' RPC send detected!\n"
													 "\tCaller: '%s'\n"
													 "\tCalled: '%s'\n",
												caller->BuildQualifiedName().c_str(),
												called->BuildQualifiedName().c_str() );

							// restore
							localColl.swap( coll );
						}
					}
				}
				else
				{
					--skip;
				}
			}
		}
	}
#	endif // GP_DEBUG

	// set address for call on stack (this requires a TLS lookup so don't bother
	// if we are retail and SP
	if ( ShouldSetAddresses() )
	{
		GetRpcVerifyColl().rbegin()->m_Address = addr;
	}

	// if we're in an rpc, run it local - this will reset dispatch flag
	bool old = ms_IsDispatching;
	ms_IsDispatching = false;
	if ( old )
	{
		return ( false );
	}

	// if it's supposed to go local, run it local
	if ( addr == RPC_TO_LOCAL )
	{
		return ( false );
	}

	// if it's for the server which is local, run it local
	if ( (addr == RPC_TO_SERVER) && IsServerLocal() )
	{
		return ( false );
	}

	// ok, do the rpc
	return ( true );
}

bool SysExports :: ClearDispatcher( void )
{
	// temporarily clear dispatcher in case we early-out before hitting RpcTestMacro
	bool old = ms_IsDispatching;
	ms_IsDispatching = false;
	return ( old );
}

void SysExports :: EnterRpc( const bool* oldDispatching )
{
	// restore old dispatch value if any
	if ( oldDispatching != NULL )
	{
		ms_IsDispatching = *oldDispatching;
	}

	if ( ShouldSetAddresses() )
	{
		RpcVerifyColl& coll = GetRpcVerifyColl();
		coll.push_back( RpcVerifySpec() );
	}
}

void SysExports :: LeaveRpc( void )
{
	if ( ShouldSetAddresses() )
	{
		RpcVerifyColl& coll = GetRpcVerifyColl();
		gpassert( !coll.empty() );
		coll.pop_back();
	}
}

void SysExports :: PushNestDetectCancel( void )
{
	GetRpcVerifyColl().push_back()->SetAsNestDetectCancel();
}

void SysExports :: PopNestDetectCancel( void )
{
	gpassert( GetRpcVerifyColl().back().IsNestDetectCancel() );
	GetRpcVerifyColl().pop_back();
}

Cookie SysExports :: DispatchNextRpc( DWORD callerAddr, DWORD packetSize )
{

// Preprocess.

	// figure out which function was called, bail if invalid
	UINT serialID = ReceiveRef <WORD> ();
	const FunctionSpec* spec = FindFunctionBySerialID( serialID );
	if ( spec == NULL )
	{
		gperrorf(( "SysExports::DispatchNextRpc(): invalid serial ID %d received\n" ));
		return ( RPC_FAILURE_IGNORE );
	}

	// if we're buffering and this isn't an exceptional case, grab it and bail early
	if ( TestOptions( OPTION_BUFFER_RPCS ) && (m_BufferedExceptIds.find( serialID ) == m_BufferedExceptIds.end()) )
	{
		// grab a new packet
		Packet& packet = *m_BufferedRpcs.push_back();
		packet.m_CallerAddr = callerAddr;

		// re-insert the serial id in the header
		packet.m_Data.resize( sizeof( WORD ) );
		*(WORD*)&*packet.m_Data.begin() = (WORD)serialID;

		// grab its data
		UINT size = gFuBiNetReceiveTransport.Remaining();
		BYTE* begin = (BYTE*)gFuBiNetReceiveTransport.ReceivePtr( size );
		packet.m_Data.insert( packet.m_Data.end(), begin, begin + size );

		// $ early bailout
		return ( RPC_SUCCESS );
	}

	// safety check - make sure that this function can be rpc'd. this will fail
	// if the versions of the game are mismatched (i.e. the digest was not
	// checked as a precondition to connecting to the host).
	gpassertf( spec->m_Flags & FunctionSpec::FLAG_CAN_RPC,
			 ( "FuBi RPC receiver: unexpected dispatch of a non-RPC function!\n\n"
			   "Received serialID = 0x%04X, found function '%s'.\n",
			   serialID, spec->BuildQualifiedName().c_str() ));

	// default cookie
	Cookie fubiCookie = RPC_SUCCESS;
	DWORD rc = 0;

// Unpack parameters.

	// param pointer is here
	void* params = NULL;
	BYTE* paramIter = NULL;

	// clear out temp data
	m_ParamBuffer.clear();
	m_RpcStrings.clear();
	m_WRpcStrings.clear();

	// manage packed memory
	FunctionSpec::eFlags packType = spec->m_Flags & (FunctionSpec::FLAG_CAN_PACK_FRONT | FunctionSpec::FLAG_CAN_PACK_BACK);
	if ( packType != FunctionSpec::FLAG_NONE )
	{
		gpassert( packetSize != 0 );

		// total param size reduced because it's specially packed
		int packedParamSizeBytes = spec->m_ParamSizeBytes - sizeof( const_mem_ptr );

		// get total size we need
		int bufferSize = packetSize - sizeof( WORD ) + sizeof( const_mem_ptr );

		// get size of memory block
		int memSize = bufferSize - spec->m_ParamSizeBytes;
		if ( spec->GetRpcRequiresObjectParam() )
		{
			// subtract off 'this' param that is in the buffer
			memSize -= sizeof( DWORD );
		}

		// alloc space (note dword alignment needed for stack params)
		m_ParamBuffer.resize( GetDwordAlignUp( bufferSize ) );
		BYTE* memBegin = &*m_ParamBuffer.begin();

		// receive memory as leader
		gFuBiNetReceiveTransport.ReceiveCopy( memBegin, memSize );
		paramIter = memBegin + GetDwordAlignUp( memSize );
		params = paramIter;
		BYTE* ptrBegin = paramIter;

		// adjust positions
		if ( packType == FunctionSpec::FLAG_CAN_PACK_BACK )
		{
			ptrBegin += packedParamSizeBytes;
		}
		else
		{
			paramIter += sizeof( const_mem_ptr );
		}

		// patch pointer
		const_mem_ptr* mem = (const_mem_ptr*)ptrBegin;
		*mem = const_mem_ptr( memBegin, memSize );

		// receive rest of params
		gFuBiNetReceiveTransport.ReceiveCopy( paramIter, packedParamSizeBytes );
	}
	else
	{
		// unpack parameters normally
		params = gFuBiNetReceiveTransport.ReceivePtr( spec->m_ParamSizeBytes );
		paramIter = rcast <BYTE*> ( params );
	}

	// unpack riders and special data
	if ( !(spec->m_Flags & FunctionSpec::FLAG_SIMPLE_ARGS) )
	{
		// skip if necessary
		FunctionSpec::ParamSpecs::const_iterator i, begin = spec->m_ParamSpecs.begin(), end = spec->m_ParamSpecs.end();
		if ( packType == FunctionSpec::FLAG_CAN_PACK_FRONT )
		{
			++begin;
		}
		else if ( packType == FunctionSpec::FLAG_CAN_PACK_BACK )
		{
			--end;
		}

		// process other params as needed
		for ( i = begin ; i != end ; ++i )
		{
			// process special data
			switch ( i->m_Type.m_Type )
			{
				case ( VAR_MEM_PTR ):
				case ( VAR_CONST_MEM_PTR ):
				{
					// get at the mem_ptr param
					mem_ptr* param = rcast <mem_ptr*> ( paramIter );
					// point to rider and advance
					if ( param->size != 0 )
					{
						param->mem = gFuBiNetReceiveTransport.ReceivePtr( param->size );
					}
				}	break;

				case ( VAR_CHAR ):
				{
					if ( i->m_Type.m_Flags == (TypeSpec::FLAG_POINTER | TypeSpec::FLAG_CONST) )
					{
						// get at the const char* param
						const char** param = rcast <const char**> ( paramIter );
						// fetch the size
						DWORD size = *rcast <const DWORD*> ( param );
						// point to rider and advance
						if ( size != 0 )
						{
							*param = rcast <const char*> ( gFuBiNetReceiveTransport.ReceivePtr( size ) );
						}
					}
				}	break;

				case ( VAR_USHORT ):
				{
					if ( i->m_Type.m_Flags == (TypeSpec::FLAG_POINTER | TypeSpec::FLAG_CONST) )
					{
						// get at the const wchar_t* param
						const wchar_t** param = rcast <const wchar_t**> ( paramIter );
						// fetch the size
						DWORD size = *rcast <const DWORD*> ( param );
						// point to rider and advance
						if ( size != 0 )
						{
							*param = rcast <const wchar_t*> ( gFuBiNetReceiveTransport.ReceivePtr( size * 2 ) );
						}
					}
				}	break;

				case ( VAR_STRING ):
				{
					gpassert( !i->m_Type.IsPassByValue() )

					// get at the gpstring param
					const gpstring** param = rcast <const gpstring**> ( paramIter );
					// fetch the size
					DWORD size = *rcast <const DWORD*> ( param );
					// assign it to rider and advance
					gpstring str;
					if ( size != 0 )
					{
						str.assign( rcast <const char*> ( gFuBiNetReceiveTransport.ReceivePtr( size ) ), size - 1 );
					}
					m_RpcStrings.push_back( str );
					// reassign to point to the new string
					*param = &m_RpcStrings.back();
				}	break;

				case ( VAR_WSTRING ):
				{
					gpassert( !i->m_Type.IsPassByValue() )

					// get at the gpwstring param
					const gpwstring** param = rcast <const gpwstring**> ( paramIter );
					// fetch the size
					DWORD size = *rcast <const DWORD*> ( param );
					// assign it to rider and advance
					gpwstring str;
					if ( size != 0 )
					{
						str.assign( rcast <const wchar_t*> ( gFuBiNetReceiveTransport.ReceivePtr( size * 2 ) ), size - 1 );
					}
					m_WRpcStrings.push_back( str );
					// reassign to point to the new string
					*param = &m_WRpcStrings.back();
				}	break;

				default:
				{
					// check for POD types
					const VarTypeSpec* spec = gFuBiSysExports.GetVarTypeSpec( i->m_Type.m_Type );
					if ( (spec != NULL) && (spec->m_Flags & Trait::FLAG_POD) )
					{
						// pod types passed by ref have their data tacked on to the end
						if ( !i->m_Type.IsPassByValue() )
						{
							// get at the param
							const void** param = rcast <const void**> ( paramIter );

							// if non-null, point to rider
							gpassert( spec->m_SizeBytes > 0 );
							if ( *rcast <const DWORD*> ( param ) != 0 )
							{
								*param = gFuBiNetReceiveTransport.ReceivePtr( spec->m_SizeBytes );
							}
						}
					}
					// special user type requires reverse network cookie conversion
					else if ( IsUser( i->m_Type.m_Type ) )
					{
						const ClassSpec* classSpec = gFuBiSysExports.FindClass( i->m_Type.m_Type );

						// don't bother with pointer classes
						if ( !(classSpec->m_Flags & ClassSpec::FLAG_POINTERCLASS) )
						{
							// verify system integrity
							gpassert( (classSpec != NULL) && (classSpec->m_Flags & ClassSpec::FLAG_CANRPC) );
							gpassert( !(classSpec->m_Flags & ClassSpec::FLAG_SINGLETON) );
							gpassert( classSpec->m_NetToInstanceProc != NULL );
							gpassert( !i->m_Type.IsPassByValue() );

							// patch net cookie with pointer
							DWORD* param = rcast <DWORD*> ( paramIter );
							*param = (DWORD)(*classSpec->m_NetToInstanceProc)( *param, &fubiCookie );
							gpassert( FuBi::ToString( fubiCookie ) != NULL );

							// abort if told to
							if ( fubiCookie != RPC_SUCCESS )
							{
								// clients should not do retries, so if we're
								// the server don't allow RPC_FAILURE returns
								// because they'll end up in retries
								if ( IsServerLocal() && (fubiCookie == RPC_FAILURE) )
								{
									fubiCookie = RPC_FAILURE_IGNORE;
								}

								return ( fubiCookie );
							}
						}
						else
						{
							// must be a pointer
							gpassert( !i->m_Type.IsPassByValue() );
						}
					}
				}
			}

			// move on to next param
			paramIter += i->m_Type.GetSizeBytes();
		}
	}

// Perform security and verification checks.

	// check server permissions
	if ( spec->m_Flags & FunctionSpec::FLAG_CHECK_SERVER_ONLY )
	{
		if ( callerAddr != RPC_TO_SERVER )
		{
			// permission failure!
			gperrorf(( "SysExports::DispatchNextRpc(): illegal receipt of "
					   "SERVER-only RPC from non-server machine!\n"
					   "  RPC = %s\n", spec->BuildQualifiedName().c_str() ));
			return ( RPC_FAILURE_IGNORE );
		}
	}

	// get address of object
	void* object = NULL;
	DWORD netCookie = 0;
	if ( spec->m_Flags & FunctionSpec::FLAG_CALL_THISCALL )
	{
		const ClassSpec* classSpec = spec->m_Parent;
		gpassert( (classSpec != NULL) && (classSpec->m_Flags & ClassSpec::FLAG_CANRPC) );

		if ( classSpec->m_Flags & ClassSpec::FLAG_SINGLETON )
		{
			// get the singleton
			object = (*classSpec->m_GetClassSingletonProc)();
		}
		else
		{
			// unpack cookie
			gFuBiNetReceiveTransport.ReceiveCopy( netCookie );

			// convert it to "this"
			gpassert( classSpec->m_NetToInstanceProc != NULL );
			object = (*classSpec->m_NetToInstanceProc)( netCookie, &fubiCookie );
			gpassert( FuBi::ToString( fubiCookie ) != NULL );

			// abort if told to
			if ( fubiCookie != RPC_SUCCESS )
			{
				// clients should not do retries, so if we're the server don't
				// allow RPC_FAILURE returns because they'll end up in retries
				if ( IsServerLocal() && (fubiCookie == RPC_FAILURE) )
				{
					fubiCookie = RPC_FAILURE_IGNORE;
				}

				return ( fubiCookie );
			}
		}
	}

	// ask callback to verify the RPC stack
	if ( (m_Rebroadcaster != NULL) && !AssertData::IsAssertOccurring() )
	{
		RpcVerifySpec verifySpec;
		verifySpec.m_Address  = callerAddr;
		verifySpec.m_Function = spec;
		verifySpec.m_Params   = const_mem_ptr( params, packetSize );
		verifySpec.m_This     = object;
		if ( !m_Rebroadcaster->OnVerifyDispatchRpc( true, verifySpec ) )
		{
			// client doesn't want to allow this
			return ( RPC_FAILURE_IGNORE );
		}
	}

// Call the function.

	// make the function call
	ms_IsDispatching = true;
	ms_CurrentRpcPacketSize = packetSize;
	if ( spec->m_Flags & FunctionSpec::FLAG_CALL_CDECL )
	{
		rc = BlindCallFunction_CdeclCall( params, spec->m_ParamSizeBytes, spec->m_FunctionPtr );
	}
	else if ( spec->m_Flags & FunctionSpec::FLAG_CALL_STDCALL )
	{
		rc = BlindCallFunction_StdCall( params, spec->m_ParamSizeBytes, spec->m_FunctionPtr );
	}
	else if ( spec->m_Flags & FunctionSpec::FLAG_CALL_THISCALL )
	{
		// call the function on the resulting object (if any)
		if ( object != NULL )
		{
			rc = BlindCallFunction_ThisCall( params, spec->m_ParamSizeBytes, object, spec->m_FunctionPtr );
		}
		else
		{
			gperrorf(( "Illegal member function call %s, with 'this' == NULL! (Net cookie: 0x%08X)\n",
					   spec->BuildQualifiedName().c_str(), netCookie ));

			fubiCookie = RPC_FAILURE;
			ms_IsDispatching = false;
		}
	}
	else
	{
		gpassert( 0 );	// should never get here
	}

	// reset
	ms_CurrentRpcPacketSize = 0;

	// should have been reset by the test inside the function
	gpassertm( ms_IsDispatching == false,
			   "Serious error! Dispatched an RPC to a non-RPC function! Do "
			   "the EXE's match exactly? Perhaps you forgot a FUBI_RPC_TAG() "
			   "in your function and do a return statement before the "
			   "FUBI_RPC_CALL() macro?" );

// Finish.

	// set cookie properly
	if ( (fubiCookie == RPC_SUCCESS) && (spec->m_Flags & FunctionSpec::FLAG_RETRY) )
	{
		fubiCookie = (Cookie)rc;
	}

	// clients should not do retries, so if we're the server don't allow
	// RPC_FAILURE returns because they'll end up in retries
	if ( IsServerLocal() && (fubiCookie == RPC_FAILURE) )
	{
		fubiCookie = RPC_FAILURE_IGNORE;
	}

	// tell callback about it so it can do another verify
	if ( (m_Rebroadcaster != NULL) && !AssertData::IsAssertOccurring() )
	{
		RpcVerifySpec verifySpec;
		verifySpec.m_Address  = callerAddr;
		verifySpec.m_Function = spec;
		verifySpec.m_Params   = const_mem_ptr( params, packetSize );
		verifySpec.m_This     = object;
		m_Rebroadcaster->OnVerifyDispatchRpc( false, verifySpec );
	}

	// stats
	if ( m_Rebroadcaster != NULL )
	{
		m_Rebroadcaster->OnDispatchRpc( spec, netCookie, packetSize );
	}

	// return cookie
	gpassert( FuBi::ToString( fubiCookie ) != NULL );
	return ( fubiCookie );
}

const FunctionSpec* SysExports :: ResolveEvent( DWORD EIP, const char* funcName, int funcNameLen ) const
{
	// in here for dev modes we attempt to limp along by returning null, which
	// will abort the event, rather than just crashing.

	// find the function
	const FunctionSpec* spec = FindFunctionByAddrNear( EIP, funcName, funcNameLen );

#	if !GP_RETAIL

	// verify it even exists
	if ( spec == NULL )
	{
		gperrorf(( "Event: unable to find FuBi event function from EIP = 0x%08X (should be '%s')\n",
				    EIP, funcName ));
		return ( NULL );
	}

	// verify it's in the right namespace
	if ( (spec->m_Parent == NULL) || (spec->m_Parent != FindEventNamespace()) )
	{
		gperrorf(( "Event: function '%s' is not part of the %s namespace\n",
				    spec->BuildQualifiedName().c_str(), EVENT_NAMESPACE ));
		return ( NULL );
	}

#	endif // !GP_RETAIL

	return ( spec );
}

const FunctionSpec* SysExports :: ResolveCall( DWORD EIP, const char* funcName, int funcNameLen ) const
{
	// in here for dev modes we attempt to limp along by returning null, which
	// will abort the event, rather than just crashing.

	// find the function
	const FunctionSpec* spec = FindFunctionByAddrNear( EIP, funcName, funcNameLen );

#	if !GP_RETAIL

	// verify it even exists
	if ( spec == NULL )
	{
		gperrorf(( "Event: unable to find FuBi call function from EIP = 0x%08X (should be '%s')\n",
				    EIP, funcName ));
		return ( NULL );
	}

#	endif // !GP_RETAIL

	return ( spec );
}

#if !GP_RETAIL

void SysExports :: GenerateDocs( ReportSys::ContextRef context ) const
{
	bool printedHeader = false;
	ClassByNameMap::const_iterator i, ibegin = m_Classes.begin(), iend = m_Classes.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !printedHeader )
		{
			ReportSys::Output( context, "** Classes **\n\n" );
			printedHeader = true;
		}
		i->second->GenerateDocs( context );
	}

	printedHeader = false;
	FunctionByNameIndex::const_iterator j, jbegin = m_GlobalFunctions.begin(), jend = m_GlobalFunctions.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		FunctionIndex::const_iterator k, kbegin = j->second.begin(), kend = j->second.end();
		for ( k = kbegin ; k != kend ; ++k )
		{
			if ( !((*k)->m_Flags & FunctionSpec::FLAG_HIDDEN) )
			{
				if ( !printedHeader )
				{
					ReportSys::Output( context, "** Global functions **\n\n" );
					printedHeader = true;
				}

				(*k)->GenerateDocs( context, DOCS_MINIMAL );
				ReportSys::OutputEol( context );
			}
		}
	}
	if ( printedHeader )
	{
		ReportSys::OutputEol( context );
	}

	printedHeader = false;
	EnumIndex::const_iterator k, kbegin = m_EnumVarTypeIndex.begin(), kend = m_EnumVarTypeIndex.end();
	for ( k = kbegin ; k != kend ; ++k )
	{
		if ( !printedHeader )
		{
			ReportSys::Output( context, "** Enumerations **\n\n" );
			printedHeader = true;
		}

		(*k)->GenerateDocs( context );
	}
	if ( printedHeader )
	{
		ReportSys::OutputEol( context );
	}
}

void SysExports :: DumpStatistics( ReportSys::ContextRef ctx ) const
{
	// dump stats
	ReportSys::AutoReport autoReport( ctx );
	ctx->Output( "FuBi initialization completed:\n" );
	{
		int singletonCount    = 0;
		int podCount          = 0;
		int rpcClassCount     = 0;
		int managedCount      = 0;
		int pointerClassCount = 0;
		int irregularCount    = 0;
		int noConstantsCount  = 0;
		int memberCount       = 0;
		int globalCount       = 0;
		int rpcCount          = 0;
		int skritCount        = 0;
		int eventCount        = 0;
		int importCount       = 0;

		FunctionByAddrMap::const_iterator i, ibegin = m_Functions.begin(), iend = m_Functions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->second.m_Flags & FunctionSpec::FLAG_SKRIT_IMPORT )
			{
				++importCount;
			}

			if ( i->second.m_Flags & FunctionSpec::FLAG_HIDDEN )
			{
				continue;
			}

			if ( i->second.m_Flags & (FunctionSpec::FLAG_MEMBER | FunctionSpec::FLAG_STATIC_MEMBER) )
			{
				++memberCount;
			}

			if ( i->second.m_Flags & FunctionSpec::FLAG_CAN_RPC )
			{
				++rpcCount;
			}

			if ( i->second.m_Flags & FunctionSpec::FLAG_CAN_SKRIT )
			{
				++skritCount;
			}

			if ( i->second.m_Flags & FunctionSpec::FLAG_EVENT )
			{
				++eventCount;
			}
		}

		FunctionByNameIndex::const_iterator j, jbegin = m_GlobalFunctions.begin(), jend = m_GlobalFunctions.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			globalCount += j->second.size();
		}

		ClassByNameMap::const_iterator k, kbegin = m_Classes.begin(), kend = m_Classes.end();
		for ( k = kbegin ; k != kend ; ++k )
		{
			if ( k->second->m_Flags & ClassSpec::FLAG_SINGLETON )
			{
				++singletonCount;
			}

			if ( k->second->m_Flags & ClassSpec::FLAG_POD )
			{
				++podCount;
			}

			if ( k->second->m_Flags & ClassSpec::FLAG_CANRPC )
			{
				++rpcClassCount;
			}

			if ( k->second->m_Flags & ClassSpec::FLAG_MANAGED )
			{
				++managedCount;
			}

			if ( k->second->m_Flags & ClassSpec::FLAG_POINTERCLASS )
			{
				++pointerClassCount;
			}
		}

		EnumByNameMap::const_iterator l, lbegin = m_Enums.begin(), lend = m_Enums.end();
		for ( l = lbegin ; l != lend ; ++l )
		{
			if ( !l->second->m_Exporter.IsContinuous() )
			{
				if ( l->second->m_Exporter.HasConstants() )
				{
					++irregularCount;
				}
				else
				{
					++noConstantsCount;
				}
			}
		}

		ReportSys::AutoIndent autoIndent( ctx );
		ctx->OutputF( "Class count       : %d\n", m_Classes.size()   );
		ctx->OutputF( " - Singletons     : %d\n", singletonCount     );
		ctx->OutputF( " - POD's          : %d\n", podCount           );
		ctx->OutputF( " - RPC'able       : %d\n", rpcClassCount      );
		ctx->OutputF( " - Managed        : %d\n", managedCount       );
		ctx->OutputF( " - Pointerclass   : %d\n", pointerClassCount  );
		ctx->OutputF( "Enum count        : %d\n", m_Enums.size()     );
		ctx->OutputF( " - Irregular      : %d\n", irregularCount     );
		ctx->OutputF( " - NoConstants    : %d\n", noConstantsCount   );
		ctx->OutputF( "Function count    : %d\n", m_Functions.size() );
		ctx->OutputF( " - Class methods  : %d\n", memberCount        );
		ctx->OutputF( " - Global         : %d\n", globalCount        );
		ctx->OutputF( " - RPC'able       : %d\n", rpcCount           );
		ctx->OutputF( " - Skrit-callable : %d\n", skritCount         );
		ctx->OutputF( " - Events         : %d\n", eventCount         );
		ctx->OutputF( " - Skrit imports  : %d\n", importCount        );
	}
}

#endif // !GP_RETAIL

#if GP_DEBUG

void SysExports :: CheckSanity( void ) const
{
	ClassByNameMap::const_iterator h, hbegin = m_Classes.begin(), hend = m_Classes.end();
	for ( h = hbegin ; h != hend ; ++h )
	{
		const ClassSpec* classSpec = h->second;
		FunctionByNameIndex::const_iterator i, ibegin = classSpec->m_Functions.begin(), iend = classSpec->m_Functions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FunctionIndex::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				FunctionSpec* functionSpec = *j;
				if ( functionSpec->m_Parent != classSpec )
				{
					BREAKPOINT();
				}
			}
		}
	}
}

#endif // GP_DEBUG

const FunctionSpec* SysExports :: FindFunctionByAddrHelper(
		const char* funcName, int funcNameLen,
		FunctionByAddrMap::const_iterator found ) const
{
	// is it the only one at that address? we get to cheat if it is. most
	//  functions will be like this so we'll keep the efficiency most of the
	//  time.
	DWORD ptr = found->first;
	FunctionByAddrMap::const_iterator next = found;
	++next;
	if ( (next == m_Functions.end()) || (next->first != ptr) )
	{
		const FunctionSpec* spec = &found->second;

#		if !GP_RETAIL
		// verify it's the right one
		if ( !same_with_case_fast( spec->m_Name.c_str(), spec->m_Name.length(), funcName, funcNameLen ) )
		{
			gperrorf(( "FuBi call function lookup mismatch from address = 0x%08X - found '%s' (should be '%s')\n",
					    ptr, spec->m_Name.c_str(), funcName ));
			return ( NULL );
		}
#		endif // !GP_RETAIL

		return ( spec );
	}

	// do linear search to find our named function - necessary to get the
	//  proper typing on the function.
	for ( ; (found != m_Functions.end()) && (found->first == ptr) ; ++found )
	{
		const FunctionSpec* spec = &found->second;
		if ( same_with_case_fast( spec->m_Name.c_str(), spec->m_Name.length(), funcName, funcNameLen ) )
		{
			return ( spec );
		}
	}

	gperrorf(( "FuBi call function lookup failed from address = 0x%08X (function is '%s')\n", ptr, funcName ));
	return ( NULL );
}

RpcVerifyColl& SysExports :: GetRpcVerifyColl( void )
{
	static kerneltool::RwCritical s_Critical;

	DWORD id = ::GetCurrentThreadId();

	{
		kerneltool::RwCritical::ReadLock readLock( s_Critical );

		RpcColl::iterator i, ibegin = m_RpcColl.begin(), iend = m_RpcColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->first == id )
			{
				return ( i->second );
			}
		}
	}

	kerneltool::RwCritical::ReadLock writeLock( s_Critical );

	m_RpcColl.reserve( 5 );
	RpcColl::iterator newEntry = m_RpcColl.insert( m_RpcColl.end(), std::make_pair( id, RpcVerifyColl() ) );
	return ( newEntry->second );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
