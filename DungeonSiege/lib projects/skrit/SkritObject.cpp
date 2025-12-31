//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritObject.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "SkritObject.h"

#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "GpStatsDefs.h"
#include "ReportSys.h"
#include "Skrit.h"
#include "SkritByteCode.h"
#include "SkritEngine.h"
#include "SkritMachine.h"
#include "SkritStructure.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// internal events

DECLARE_EVENT
{
	SKRIT_IMPORT void OnConstruct( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnConstruct, skrit )  }
	SKRIT_IMPORT void OnLoad( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnLoad, skrit )  }
	SKRIT_IMPORT void OnDestroy( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnDestroy, skrit )  }
	SKRIT_IMPORT void OnConstructShared( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnConstructShared, skrit )  }
	SKRIT_IMPORT void OnLoadShared( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnLoadShared, skrit )  }
	SKRIT_IMPORT void OnDestroyShared( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnDestroyShared, skrit )  }
	SKRIT_IMPORT void OnEnterState( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnEnterState, skrit )  }
	SKRIT_IMPORT void OnExitState( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnExitState, skrit )  }
	SKRIT_IMPORT void OnTransitionPoll( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnTransitionPoll, skrit )  }
	SKRIT_IMPORT void OnTimer( Skrit::Object* skrit, int /*id*/, float /*delta*/ )
		{  SKRIT_EVENTV( OnTimer, skrit );  }
}

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_ENUM_TRAITS( Skrit::eResult, Skrit::RESULT_SUCCESS );
FUBI_DECLARE_SELF_TRAITS( Skrit::Object::TriggerEntry );
FUBI_DECLARE_SELF_TRAITS( Skrit::Object::TimerEntry );

FUBI_DECLARE_XFER_TRAITS( Skrit::Result )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		bool success = persist.EnterBlock( key );
		if ( success )
		{
			persist.Xfer   ( "m_Code", obj.m_Code );
			persist.XferHex( "m_Raw",  obj.m_Raw );
		}
		persist.LeaveBlock();

		return ( success );
	}
};

//////////////////////////////////////////////////////////////////////////////

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class ObjectImpl implementation

ObjectImpl :: ObjectImpl( const char* name )
{
	m_RefCount        = 0;
	m_Pack            = new Raw::Pack;
	m_GlobalStore     = NULL;
	m_StoreHeaderSpec = NULL;
	m_IsValid         = false;
#	if !GP_RETAIL
	m_DebugDb         = NULL;
#	endif // !GP_RETAIL

	if ( name != NULL )
	{
		m_Name = name;
	}
}

ObjectImpl :: ~ObjectImpl( void )
{
	// special: execute skrit destructor if it exists and we're ready
	if ( m_IsValid )
	{
		static const FuBi::FunctionSpec* eventDtor = FuBi::LookupEvent( FuBiEvent::OnDestroyShared, "OnDestroyShared" );
		int eventSerial = FindEvent( STATE_INDEX_NONE, eventDtor->m_SerialID );
		if ( eventSerial != INVALID_FUNCTION_INDEX )
		{
			Machine machine( this );
			machine.Call( GetFunctionSpec( eventSerial ) );
		}
	}

	// clear indexes that may be pointing into the pack
	m_FunctionIndex      .clear();
	m_EventIndex         .clear();
	m_FunctionByNameIndex.clear();
	m_SignatureCache     .clear();
	m_States             .clear();

	// shut down
	delete ( m_Pack );
	delete ( m_GlobalStore );
	delete ( m_StoreHeaderSpec );
#	if !GP_RETAIL
	delete ( m_DebugDb );
#	endif // !GP_RETAIL
}

bool ObjectImpl :: Xfer( FuBi::PersistContext& persist, const gpstring* name )
{
	// must persist type as a string to handle version changes
	gpstring type;
	if ( persist.IsSaving() )
	{
		if ( m_OwnerType != FuBi::VAR_UNKNOWN )
		{
			const char* typeName = gFuBiSysExports.FindTypeName( m_OwnerType );
			if_gpassert( typeName != NULL )
			{
				type = typeName;
			}
		}
	}
	persist.Xfer( "m_OwnerType", type );
	if ( persist.IsRestoring() )
	{
		if ( type.empty() )
		{
			m_OwnerType = FuBi::VAR_UNKNOWN;
		}
		else
		{
			m_OwnerType = gFuBiSysExports.FindType( type );
		}
	}

	// take name
	if ( name != NULL )
	{
		m_Name = *name;
	}

	// rebuild object if we're restoring and aren't already compiled
	if ( persist.IsRestoring() && (m_RefCount == 0) )
	{
		FileSys::AutoFileHandle file;
		FileSys::AutoMemHandle mem;
		if ( file.Open( m_Name ) && mem.Map( file ) )
		{
			const_mem_ptr memPtr( mem.GetData(), mem.GetSize() );
			if ( Raw::Header::ShouldCompile( memPtr, m_OwnerType ) )
			{
				if ( gSkritEngine.GetCompiler().Compile( m_Name, memPtr ) )
				{
					gSkritEngine.GetCompiler().MoveTo( *this );
				}
				else
				{
					mem.Close();
					file.Close();

					gSkritEngine.GetCompiler().ReportLastCompile();

					gpreportf( persist.GetReportContext(),
						( "Unable to compile skrit file '%s'\n", m_Name.c_str() ));
					return ( false );
				}
			}
			else
			{
				Copy( rcast <const Raw::Header*> ( memPtr.mem ) );
			}
		}
		else
		{
			gpreportf( persist.GetReportContext(),
				( "Unable to load skrit file '%s': '%s'\n",
				  m_Name.c_str(), stringtool::GetLastErrorText().c_str() ));
			return ( false );
		}
	}

	// xfer the store
	bool success = true;
	if ( m_GlobalStore != NULL )
	{
		if ( persist.IsRestoring() || !m_GlobalStore->IsEmpty() )
		{
			if ( persist.EnterBlock( "m_GlobalStore" ) )
			{
				success = XferStore( persist, *m_GlobalStore, Raw::STORE_SHARED );
			}
			persist.LeaveBlock();
		}
	}

	// done
	return ( success );
}

bool ObjectImpl :: CommitLoad( void )
{
	bool success = true;

	// special: execute loader if it exists
	static const FuBi::FunctionSpec* eventLoad = FuBi::LookupEvent( FuBiEvent::OnLoadShared, "OnLoadShared" );
	int eventSerial = FindEvent( STATE_INDEX_NONE, eventLoad->m_SerialID );
	if ( eventSerial != INVALID_FUNCTION_INDEX )
	{
		Machine machine( this );
		Result result = machine.Call( m_FunctionIndex[ eventSerial ] );
		success = result.IsSuccess();
	}

	// commit the store
	if ( !PostLoadStore( *m_GlobalStore, Raw::STORE_SHARED ) )
	{
		success = false;
	}

	return ( success );
}

void ObjectImpl :: Reset( void )
{
	gpassert( m_RefCount == 0 );

	m_Pack->Reset();
	if ( m_GlobalStore != NULL )
	{
		m_GlobalStore->FreeAll();
	}
	Delete( m_StoreHeaderSpec );
	m_IsValid = false;

	m_FunctionIndex.clear();
	m_EventIndex.clear();
	m_FunctionByNameIndex.clear();
	m_SignatureCache.clear();
}

bool ObjectImpl :: Copy( const Raw::Header* header )
{
	Reset();
	m_Pack->Copy( header );
	m_OwnerType = header->m_OwnerType;
	return ( Commit() );
}

bool ObjectImpl :: Swap( Raw::Pack& pack, eVarType ownerType )
{
	Reset();
	m_Pack->Swap( pack );
	m_OwnerType = ownerType;
	return ( Commit() );
}

UINT32 ObjectImpl :: AddCrc32( UINT32 seed ) const
{
	return ( m_Pack->AddCrc32( seed ) );
}

int ObjectImpl :: FindFunction( const char* funcName )
{
	gpassert( IsValid() );
	gpassert( funcName != NULL );

	// fill function name index on demand
	if ( m_FunctionByNameIndex.empty() )
	{
		FunctionIndex::const_iterator i, begin = m_FunctionIndex.begin(), end = m_FunctionIndex.end();
		for ( i = begin ; i != end ; ++i )
		{
			m_FunctionByNameIndex[ (*i)->m_Name ] = i - begin;
		}
	}

	// look up function
	int index = INVALID_FUNCTION_INDEX;
	FunctionByNameIndex::const_iterator found = m_FunctionByNameIndex.find( funcName );
	if ( found != m_FunctionByNameIndex.end() )
	{
		index = found->second;
	}

	// done
	return ( index );
}

int ObjectImpl :: FindEvent( int state, UINT eventSerial ) const
{
	int index = INVALID_FUNCTION_INDEX;
	FunctionByEventIndex::const_iterator found = m_EventIndex.find( EventEntry( state, eventSerial ) );
	if ( found != m_EventIndex.end() )
	{
		// found it in a local state or as a global handler
		index = found->second;
	}
	else if ( state != STATE_INDEX_NONE )
	{
		// try it as a global if we weren't already there
		index = FindEvent( STATE_INDEX_NONE, eventSerial );
	}
	return ( index );
}

int ObjectImpl :: GetOpCodeCount( int funcIndex ) const
{
	gpassert( IsValid() );

	// look up function
	const Raw::ExportEntry* func = GetFunctionSpec( funcIndex );
	gpassert( func != NULL );

	// iterate over op codes and count each
	int count = 0;
	const BYTE* i = &*m_Pack->m_ByteCode.begin() + func->m_FunctionOffset;
	for ( ; *i != Op::CODE_SITNSPIN ; ++count, ++i )
	{
		// skip over data
		i += Op::GetData( scast <Op::Code> ( *i ) ).m_DataSizeBytes;
	}

	// done
	return ( count );
}

bool ObjectImpl :: CheckFunctionMatch( int funcIndex, const FuBi::FunctionSpec* spec )
{
	gpassert( spec != NULL );
	gpassert( (funcIndex >= 0) && (funcIndex < scast <int> ( m_FunctionIndex.size()) ) );

	// $ remember that the first param is a skrit object and the second is the func index

	// set up signature cache on demand (fill with nulls)
	if ( m_SignatureCache.empty() )
	{
		m_SignatureCache.resize( m_FunctionIndex.size(), NULL );
	}

	// early bailout if cached
	if ( m_SignatureCache[ funcIndex ] == spec )
	{
		return ( true );
	}

	// check and set
	const Raw::ExportEntry* export = GetFunctionSpec( funcIndex );

	// check for a match
	bool success = false;
	int badParam;
	switch ( export->CheckFunctionMatch( 2, spec, &badParam ) )
	{
		case ( Raw::ExportEntry::MATCH_OK ):
		{
			success = true;
			m_SignatureCache[ funcIndex ] = spec;		// cache success
		}	break;

		case ( Raw::ExportEntry::MATCH_RETURN_MISMATCH ):
		{
			gperrorf(( "'%s': return type does not match exactly\n", export->m_Name ));
		}	break;

		case ( Raw::ExportEntry::MATCH_DIFF_PARAM_COUNT ):
		{
			int exp = export->m_ParamCount;
			int req = spec->m_ParamSpecs.size() - 2;
			gperrorf(( "'%s': Skrit function takes %d parameter%s, but %d %s passed in\n",
					   export->m_Name,
					   exp, exp == 1 ? "" : "s",
					   req, req == 1 ? "was" : "were" ));
		}	break;

		case ( Raw::ExportEntry::MATCH_PARAM_MISMATCH ):
		{
			gperrorf(( "'%s': parameter list does not match exactly starting at parameter %d\n",
					   export->m_Name, badParam ));
		}	break;
	}

	// all ok
	return ( success );
}

void ObjectImpl :: CreateAndInitStore( FuBi::StoreHeaderSpec** spec,
									   Store** store,
									   Raw::eStore storeType,
									   bool propertiesOnly,
									   PropertyFilterCb cb )
{
	// init counts
	int ivar = 0, istr = 0, ihdl = 0;

	// iter over these vars
	const Raw::GlobalEntry* i,
						  * ibegin = m_Pack->GetGlobals()->m_Entries,
						  * iend   = Advance <const Raw::GlobalEntry*> ( ibegin, m_Pack->m_Globals.size() );
	for ( i = ibegin ; i != iend ; i = i->GetNext() )
	{
		// only bother with our type
		if ( i->m_Store != storeType )
		{
			continue;
		}

		// skip if properties only and not a property (and not hidden)
		if ( propertiesOnly )
		{
			if ( !(i->m_Flags & Raw::FLAG_PROPERTY) || (i->m_Flags & Raw::FLAG_HIDDEN) )
			{
				continue;
			}
			else
			{
				// properties only allowed on per-instance globals
				gpassert( storeType == Raw::STORE_GLOBAL );

				// skip if cb says so
				if ( cb )
				{
					const Raw::PropertyEntry* prop = rcast <const Raw::PropertyEntry*> ( i );
					if ( !cb( rcast <const char*> ( &*m_Pack->m_Strings.begin() + prop->m_NameOffset ) ) )
					{
						continue;
					}
				}
			}
		}

		// optional spec processing
		if ( spec != NULL )
		{
			// get vars
			eVarType type = scast <eVarType> ( i->m_Type );
			int index = 0;
			FuBi::TypeSpec::eFlags flags = FuBi::TypeSpec::FLAG_NONE;
			Store::eColl coll = Store::COLL_VARIABLES;

			// classify it
			if ( FuBi::IsUser( type ) )
			{
				// UDT pointer or handle
				const FuBi::ClassSpec* cspec = gFuBiSysExports.FindClass( type );
				gpassert( cspec != NULL );

				// managed class?
				if ( cspec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
				{
					// handle
					flags = FuBi::TypeSpec::FLAG_HANDLE;
					coll  = Store::COLL_HANDLES;
					index = ihdl++;
				}
				else
				{
					// pointer
					flags = FuBi::TypeSpec::FLAG_POINTER;
					index = ivar++;
				}
			}
			else if ( type == FuBi::VAR_STRING )
			{
				coll  = Store::COLL_STRINGS;
				index = istr++;
			}
			else
			{
				// normal built-in
				index = ivar++;
			}

			// for properties only
			if ( i->m_Flags & Raw::FLAG_PROPERTY )
			{
				// build spec if none
				if ( *spec == NULL )
				{
					*spec = new FuBi::StoreHeaderSpec;
				}

				// add column to it
				const Raw::PropertyEntry* prop = rcast <const Raw::PropertyEntry*> ( i );
				gpstring cheapName( rcast <const char*> ( &*m_Pack->m_Strings.begin() + prop->m_NameOffset ) );
				gpassert( *cheapName.rbegin() == '$' );
				cheapName.erase( cheapName.length() - 1 );
				(*spec)->AddColumn( type, flags, cheapName, coll, index
									GPDEV_PARAM( rcast <const char*> ( &*m_Pack->m_Strings.begin() + prop->m_DocsOffset ) ) );
			}
		}

		// optional store processing
		if ( store != NULL )
		{
			// build store if none
			if ( *store == NULL )
			{
				*store = new Store;
			}

			// add it
			eVarType type = scast <eVarType> ( i->m_Type );
			if ( FuBi::IsUser( type ) )
			{
				// UDT pointer or handle
				const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
				gpassert( spec != NULL );

				// managed class?
				if ( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
				{
					// handle
					(*store)->AddHandle( type );
				}
				else
				{
					// pointer
					(*store)->AddVariable( 0 );
				}
			}
			else if ( type == FuBi::VAR_STRING )
			{
				(*store)->AddString( rcast <const char*> ( &*m_Pack->m_Strings.begin() + i->m_InitialValue ) );
			}
			else
			{
				(*store)->AddVariable( i->m_InitialValue );
			}
		}
	}

	// finish
	if ( (spec != NULL) && (*spec != NULL) )
	{
		// set the name to just the filename
		(*spec)->SetName( FileSys::GetFileNameOnly( GetName() ) );
	}
}

void ObjectImpl :: BuildPropertiesAndStore( FuBi::StoreHeaderSpec& spec,
											FuBi::Store& store,
											PropertyFilterCb cb )
{
	gpassert( spec.GetColumnCount() == 0 );
	gpassert( store.IsEmpty() );

	FuBi::StoreHeaderSpec* pspec = &spec;
	FuBi::Store* pstore = &store;
	CreateAndInitStore( &pspec, &pstore, Raw::STORE_GLOBAL, true, cb );
}

bool ObjectImpl :: XferStore( FuBi::PersistContext& persist, Store& store, Raw::eStore storeType )
{
	// init counts
	int ivar = 0, istr = 0, ihdl = 0;

	// get a temp var
	gpstring temp;

	// iter over these vars
	const Raw::GlobalEntry* i,
						  * ibegin = m_Pack->GetGlobals()->m_Entries,
						  * iend   = Advance <const Raw::GlobalEntry*> ( ibegin, m_Pack->m_Globals.size() );
	for ( i = ibegin ; i != iend ; i = i->GetNext() )
	{
		// only bother with our type
		if ( i->m_Store != storeType )
		{
			continue;
		}

		// get vars
		eVarType type = scast <eVarType> ( i->m_Type );
		const char* name = rcast <const char*> ( &*m_Pack->m_Strings.begin() + i->m_NameOffset );

		// classify it
		if ( FuBi::IsUser( type ) )
		{
			// UDT pointer or handle
			const FuBi::ClassSpec* cspec = gFuBiSysExports.FindClass( type );
			gpassert( cspec != NULL );

			// managed class?
			if ( cspec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
			{
				// xfer directly, the handle should be persisted independently
				DWORD handle = persist.IsSaving() ? store.GetHandle( ihdl ) : 0;
				persist.XferHex( name, handle );
				if ( persist.IsRestoring() )
				{
					store.SetHandle( ihdl, handle );
				}
				++ihdl;
			}
			else
			{
				DWORD* param = rcast <DWORD*> ( store.GetVariablePtr( ivar ) );

				if ( cspec->m_Flags & FuBi::ClassSpec::FLAG_POINTERCLASS )
				{
					// just xfer it! always safe.
					persist.XferHex( name, *param );
				}
				else if ( cspec->m_InstanceToNetProc && cspec->m_NetToInstanceProc )
				{
					if ( persist.IsSaving() )
					{
						// xfer this as a cookie
						DWORD cookie = *param ? (DWORD)(*cspec->m_InstanceToNetProc)( (void*)(*param) ) : 0;
						persist.XferHex( name, cookie );
					}
					else
					{
						// just xfer it, we'll do the reverse cookie conversion later
						persist.XferHex( name, *param );
					}
				}

				++ivar;
			}
		}
		else if ( type == FuBi::VAR_STRING )
		{
			// persist this
			persist.Xfer( FuBi::XFER_QUOTED, name, store.GetString( istr++ ) );
		}
		else
		{
			temp.clear();

			// normal built-in or enum
			void* var = store.GetVariablePtr( ivar++ );
			if ( persist.AsText() || FuBi::IsEnum( type ) )
			{
				if ( persist.IsSaving() )
				{
					if ( gFuBiSysExports.ToString( type, temp, var ) )
					{
						persist.XferString( name, temp, type );
					}
				}
				else
				{
					if ( persist.XferString( name, temp, type ) )
					{
						gFuBiSysExports.FromString( type, temp, var );
					}
				}
			}
			else
			{
				persist.XferBinary( name, mem_ptr( var, sizeof( DWORD ) ) );
			}
		}
	}

	return ( true );
}

bool ObjectImpl :: PostLoadStore( Store& store, Raw::eStore storeType )
{
	// init counts
	int ivar = 0;

	// iter over these vars
	const Raw::GlobalEntry* i,
						  * ibegin = m_Pack->GetGlobals()->m_Entries,
						  * iend   = Advance <const Raw::GlobalEntry*> ( ibegin, m_Pack->m_Globals.size() );
	for ( i = ibegin ; i != iend ; i = i->GetNext() )
	{
		// only bother with our type
		if ( i->m_Store != storeType )
		{
			continue;
		}

		// classify the var in here
		eVarType type = scast <eVarType> ( i->m_Type );
		if ( FuBi::IsUser( type ) )
		{
			// UDT pointer or handle
			const FuBi::ClassSpec* cspec = gFuBiSysExports.FindClass( type );
			gpassert( cspec != NULL );

			// unmanaged class?
			if ( !(cspec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED) )
			{
				DWORD* param = rcast <DWORD*> ( store.GetVariablePtr( ivar ) );

				if ( cspec->m_Flags & FuBi::ClassSpec::FLAG_SINGLETON )
				{
					// this is a singleton - i have no idea why the skrit writer
					// would want to keep a pointer to it but we have to handle it...
					gpassert( cspec->m_GetClassSingletonProc != NULL );
					*param = (DWORD)(*cspec->m_GetClassSingletonProc)();
				}
				else if ( cspec->m_InstanceToNetProc && cspec->m_NetToInstanceProc )
				{
					// convert existing entry from cookie into instance
					FuBiCookie fubiCookie = RPC_SUCCESS;
					*param = (DWORD)(*cspec->m_NetToInstanceProc)( *param, &fubiCookie );

					// fall back to 0 on failure
					if ( fubiCookie != RPC_SUCCESS )
					{
						*param = 0;
					}
				}
				else if ( !(cspec->m_Flags & FuBi::ClassSpec::FLAG_POINTERCLASS) )
				{
					// well we can't do anything about this, so clear it,
					// hopefully it's going to get reset during the post-load
					// event when it's given a chance to restore pointers.
					*param = 0;
				}

				++ivar;
			}
		}
		else if ( type != FuBi::VAR_STRING )
		{
			++ivar;
		}
	}

	return ( true );
}

bool ObjectImpl :: DoesWantPolls( void ) const
{
	// first check all our states for needing polls
	{
		StateIndex::const_iterator i, ibegin = m_States.begin(), iend = m_States.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->m_NeedsPoll )
			{
				return ( true );
			}
		}
	}

	// next check for any 'at' functions
	{
		FunctionIndex::const_iterator i, ibegin = m_FunctionIndex.begin(), iend = m_FunctionIndex.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->HasAtTime() )
			{
				return ( true );
			}
		}
	}

	// get serials for events that imply required polling
	static UINT s_UpdateSerialIds[ 2 ];
	if ( s_UpdateSerialIds[ 0 ] == 0 )
	{
		s_UpdateSerialIds[ 0 ] = gFuBiSysExports.FindEventSerialByName( "OnTransitionPoll" );
		s_UpdateSerialIds[ 1 ] = gFuBiSysExports.FindEventSerialByName( "OnTimer"          );
	}

	// finally check for these events anywhere in the skrit
	if ( DoesUseEvents( s_UpdateSerialIds, ELEMENT_COUNT( s_UpdateSerialIds ) ) )
	{
		return ( true );
	}

	// guess we don't need polls
	return ( false );
}

bool ObjectImpl :: DoesUseEvents( UINT* eventSerial, int eventCount ) const
{
	gpassert( IsPointerValid( eventSerial ) );
	gpassert( eventCount > 0 );

	// loop through all states
	for ( int i = -1, stateCount = GetStateCount() ; i != stateCount ; ++i )
	{
		for ( int j = 0 ; j != eventCount ; ++j )
		{
			// try to find an event handler for this event
			if ( FindEvent( i, eventSerial[ j ] ) != INVALID_FUNCTION_INDEX )
			{
				return ( true );
			}

			// try to find a trigger for this event
			const StateEntry* stateEntry = GetStateEntry( i );
			if ( stateEntry != NULL )
			{
				StateEntry::TriggerIndex::const_iterator k,
						kbegin = stateEntry->m_StaticTriggers.begin(),
						kend   = stateEntry->m_StaticTriggers.end();
				for ( k = kbegin ; k != kend ; ++k )
				{
					if ( k->first == eventSerial[ j ] )
					{
						return ( true );
					}
				}
			}
		}
	}

	// guess we didn't find it
	return ( false );
}

#if !GP_RETAIL

const Raw::DebugDb* ObjectImpl :: GetDebugDb( void ) const
{
	if ( (m_DebugDb == NULL) && !m_Pack->m_Debug.empty() )
	{
		ccast <ThisType*> ( this )->m_DebugDb = new Raw::DebugDb( *m_Pack );
	}
	return ( m_DebugDb );
}

#endif // !GP_RETAIL

bool ObjectImpl :: Commit( void )
{
	bool success = true;

// Fill indexes.

	// function entry points
	{
		const Raw::ExportEntry* i,
							  * begin = m_Pack->GetExports()->m_Entries,
							  * end = Advance <const Raw::ExportEntry*> ( begin, m_Pack->m_Exports.size() );
		for ( i = begin ; i != end ; i = i->GetNext() )
		{
			// register events
			if ( i->m_EventBinding != (WORD)INVALID_FUNCTION_INDEX )
			{
				int state = GetStateIndex( i->m_StateMember );
				m_EventIndex[ EventEntry( state, i->m_EventBinding ) ] = m_FunctionIndex.size();
			}

			// add to function index
			m_FunctionIndex.push_back( i );
		}
	}

	// build states
	if ( m_Pack->GetState() != NULL )
	{
		m_States.resize( m_Pack->GetState()->m_TotalStates + 1 );

		// fill state config tables
		const Raw::StateEntry* i,
							 * begin = m_Pack->GetState()->GetFirst(),
							 * end = Advance <const Raw::StateEntry*> ( m_Pack->GetState(), m_Pack->m_State.size() );
		for ( i = begin ; i != end ; i = i->GetNext() )
		{
			switch ( i->m_Type )
			{
				case ( Raw::STATE_CONFIG ):
				{
					const Raw::StateConfig* config = rcast <const Raw::StateConfig*> ( i );
					m_States[ config->m_State ].m_Config = config;
				}	break;

				case ( Raw::STATE_TRANSITION ):
				{
					// get at our entries
					const Raw::StateTransition* rawtrans = rcast <const Raw::StateTransition*> ( i );
					StateEntry* entry = GetStateEntry( GetStateIndex( rawtrans->m_FromState ) );

					// add a new transition to the index
					int transIndex = scast <int> ( entry->m_Transitions.size() );
					TransitionEntry* trans = &*entry->m_Transitions.insert( entry->m_Transitions.end() );
					trans->m_Transition = rawtrans;

					// add each condition
					const Raw::ConditionEntry* cond = rawtrans->GetFirst();
					for ( int k = 0 ; k < rawtrans->m_ConditionCount ; ++k )
					{
						if ( cond->m_IsStatic )
						{
							// add static condition
							const Raw::ConditionStatic* scond = rcast <const Raw::ConditionStatic*> ( cond );
							cond = scond->GetNext();
							trans->m_StaticIndex.push_back( scond );

							// add static trigger index
							TriggerEntry triggerEntry;
							triggerEntry.m_TransitionIndex = transIndex;
							triggerEntry.m_StaticIndex = k;
							entry->m_StaticTriggers[ scond->m_EventSerial ].push_back( triggerEntry );
						}
						else
						{
							// add dynamic condition
							const Raw::ConditionDynamic* dcond = rcast <const Raw::ConditionDynamic*> ( cond );
							cond = dcond->GetNext();
							trans->m_DynamicIndex.push_back( dcond );

							// this state will need polling
							entry->m_NeedsPoll = true;
						}
					}
				}	break;
			}
		}
	}

// Construct globals and properties.

	// build shared globals
	CreateAndInitStore( NULL, &m_GlobalStore, Raw::STORE_SHARED );

	// build properties
	CreateAndInitStore( &m_StoreHeaderSpec, NULL, Raw::STORE_GLOBAL );

// Startup Skrit.

	// special: execute skrit constructor if it exists
	static const FuBi::FunctionSpec* eventCtor = FuBi::LookupEvent( FuBiEvent::OnConstructShared, "OnConstructShared" );
	int eventSerial = FindEvent( STATE_INDEX_NONE, eventCtor->m_SerialID );
	if ( eventSerial != INVALID_FUNCTION_INDEX )
	{
		success = Machine( this ).Call( m_FunctionIndex[ eventSerial ] ).IsSuccess();
	}

	// postprocess error state
	if ( success )
	{
		m_IsValid = true;
	}
	else
	{
		Reset();
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class Object implementation

Object :: Object( ObjectImpl* impl, void* owner, bool deferConstruct )
{
	gpassert( impl != NULL );

// Initialize.

	// set spec
	m_Owner       = owner;
	m_Constructed = false;

	// build owned objects
	m_Impl                  = impl;
	m_Impl                  ->AddRef();
	m_StateChangeDepth      = 0;
#	if !GP_RETAIL
	m_DebugStateChangeDepth = 0;
#	endif // !GP_RETAIL
	m_GlobalStore           = NULL;
	m_LastResult            = RESULT_SUCCESS;
	m_Record                = NULL;

	// init local vars
	m_CurrentState  = STATE_INDEX_NONE;
	m_PendingState  = STATE_INDEX_INVALID;
	m_IsPolling     = false;
	m_PollPeriod    = 0;
	m_CurrentPoll   = 0;
	m_CurrentTime   = 0;
	m_CurrentFrame  = 0;
	m_NextTimerId   = 1;
	m_StateEntry    = NULL;

	// debug traces initially off
#	if !GP_RETAIL
	m_TraceOpCodes      = false;
	m_TraceSysCalls     = false;
	m_TraceSkritCalls   = false;
	m_TraceStateChanges = false;
	m_TraceEvents       = false;
#	endif // !GP_RETAIL

// Construct per-instance globals.

	m_Impl->CreateAndInitStore( NULL, &m_GlobalStore, Raw::STORE_GLOBAL );

// Optionally startup VM.

	if ( !deferConstruct )
	{
		CommitCreation();
	}
}

Object :: ~Object( void )
{

// Shutdown VM if we were started up.

	if ( IsConstructed() )
	{
		FuBiEvent::OnDestroy( this );
	}

// Release resources.

	// let go of impl
	if ( m_Impl->Release() == 0 )
	{
		delete ( m_Impl );
	}

	// kill owned objects
	delete ( m_GlobalStore );
	delete ( m_Record );
}

struct TriggerEntryShouldPersistHelper
{
	template <typename T>
	bool operator () ( const T& entry ) const
	{
		return ( entry.ShouldPersist() );
	}
};

bool Object :: Xfer( FuBi::PersistContext& persist )
{
	gpassert( m_StateChangeDepth == 0 );
	gpassert( !IsStateChangePending() );

	persist.Xfer      ( "m_Constructed",   m_Constructed   );
	persist.Xfer      ( "m_LastResult",    m_LastResult    );
	persist.Xfer      ( "m_CurrentState",  m_CurrentState  );
	persist.Xfer      ( "m_IsPolling",     m_IsPolling     );
	persist.Xfer      ( "m_PollPeriod",    m_PollPeriod    );
	persist.Xfer      ( "m_CurrentPoll",   m_CurrentPoll   );
	persist.Xfer      ( "m_CurrentTime",   m_CurrentTime   );
	persist.Xfer      ( "m_CurrentFrame",  m_CurrentFrame  );
	persist.XferVector( "m_AtFunctions",   m_AtFunctions   );
	persist.XferVector( "m_Timers",        m_Timers        );
	persist.Xfer      ( "m_NextTimerId",   m_NextTimerId   );

	if ( persist.IsRestoring() )
	{
		// make sure the state is in range, if not just use the default
		if ( m_CurrentState >= m_Impl->GetStateCount() )
		{
			if ( m_Impl->GetPack()->GetState() != NULL )
			{
				m_CurrentState = GetStateIndex( m_Impl->GetPack()->GetState()->m_InitialState );
			}
		}

		// update cache
		m_StateEntry = scast <const ObjectImpl*> ( m_Impl )->GetStateEntry( m_CurrentState );

		// alloc memory for triggers to be filled from save game
		PrivateResetTriggers();
	}

	// we're now ready to xfer triggers
	persist.XferSparseArray( "m_Triggers", m_Triggers.begin(), m_Triggers.end(),
							 TriggerEntryShouldPersistHelper() );

	// xfer the store
	bool success = true;
	if ( m_GlobalStore != NULL )
	{
		if ( persist.IsRestoring() || !m_GlobalStore->IsEmpty() )
		{
			if ( persist.EnterBlock( "m_GlobalStore" ) )
			{
				success = m_Impl->XferStore( persist, *m_GlobalStore, Raw::STORE_GLOBAL );
			}
			persist.LeaveBlock();
		}
	}

	// xfer the record flags
	if ( persist.IsSaving() )
	{
		if ( m_Record != NULL )
		{
			persist.EnterBlock( "m_Record" );
			m_Record->XferFlags( persist );
			persist.LeaveBlock();
		}
	}
	else
	{
		if ( persist.EnterBlock( "m_Record" ) )
		{
			GetRecord()->XferFlags( persist );
		}
		persist.LeaveBlock();
	}

	return ( success );
}

bool Object :: CommitLoad( void )
{
	FuBiEvent::OnLoad( this );
	bool success = m_LastResult.IsSuccess();

	// commit the store
	if ( !m_Impl->PostLoadStore( *m_GlobalStore, Raw::STORE_GLOBAL ) )
	{
		success = false;
	}

	return ( success );
}

bool Object :: CommitCreation( void )
{
	// $ early bailout if already committed
	if ( m_Constructed )
	{
		return ( true );
	}

	// we're constructed now
	m_Constructed = true;

	// $ enter scope for auto class
	{
		// may change state
		AutoState autoState( this );

		// set initial pending state (may be overridden by event handler)
		if ( m_Impl->GetPack()->GetState() != NULL )
		{
			int startupState = GetStateIndex( m_Impl->GetPack()->GetState()->m_InitialState );
			SetState( startupState );
		}

		FuBiEvent::OnConstruct( this );
	}

	return ( m_LastResult.IsSuccess() );
}

FuBi::Record* Object :: GetRecord( void )
{
	if ( m_Record == NULL )
	{
		m_Record = CreateRecord();
	}
	return ( m_Record );
}

FuBi::Record* Object :: CreateRecord( void )
{
	const FuBi::StoreHeaderSpec* spec = GetImpl()->GetStoreHeaderSpec();
	if ( spec == NULL )
	{
		return ( NULL );
	}

	return ( new FuBi::StoreHeaderSpec::Record( m_GlobalStore, spec ) );
}

bool Object :: ReadRecord( const char* params )
{
	FuBi::Record* record = GetRecord();
	if ( record == NULL )
	{
		return ( false );
	}
	return ( record->LoadFromString( params ) );
}

Result Object :: Event( UINT eventSerial, const void* params )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	// trace
#	if !GP_RETAIL
	ReportSys::AutoIndent autoIndent( &gSkritContext, false );
	if ( m_TraceEvents )
	{
		const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( eventSerial );
		gpassert( spec != NULL );

		gpskritf(( "(%s) 0x%08X EVENT: %s\n",
				   FileSys::GetFileNameOnly( GetName() ).c_str(),
				   this,
				   spec->BuildQualifiedNameAndParams( const_mem_ptr( params, 0 ), NULL, false, false, 1 ).c_str() ));
		autoIndent.Indent();
	}
#	endif // !GP_RETAIL

	// by default assume we don't have an event response
	m_LastResult = RESULT_NOEVENT;

	// only do this if we are constructed
	if ( IsConstructed() )
	{
		// may change state
		AutoState autoState( this );

		// fire event if any
		int event = FindEvent( eventSerial );
		if ( event != INVALID_FUNCTION_INDEX )
		{
			size_t paramSize = m_Impl->GetFunctionSpec( event )->m_ParamCount * 4;
			Call( event, const_mem_ptr( params, paramSize ) );
		}

		// only process triggers if we didn't change states and we're in a valid state
		if ( !IsStateChangePending() )
		{
			// try to trigger it in the current state
			if ( ( m_StateEntry == NULL) || !PrivateCheckTriggers( m_StateEntry, eventSerial, params ) )
			{
				// ok now try it in the global state
				const StateEntry* global = scast <const ObjectImpl*> ( m_Impl )->GetStateEntry( STATE_INDEX_NONE );
				if ( (global == NULL) || (global == m_StateEntry) || !PrivateCheckTriggers( global, eventSerial, params ) )
				{
					// an event that we trigger on but didn't transition will
					// always cause a dynamic check, unless we're already in
					// a polling state.
					if ( !m_IsPolling )
					{
						Poll( 0, false );
					}
				}
			}
		}
	}

	// done
	return ( m_LastResult );
}

bool Object :: ForcePoll( void )
{
	gpassert( IsConstructed() );

	// force a poll by using the poll period
	return ( Poll( m_PollPeriod, true ) );
}

bool Object :: Poll( float deltaTime, bool advanceFrame )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	gpassert( IsConstructed() );
	bool polled = false;

	// advance
	m_CurrentTime += deltaTime;
	if ( advanceFrame )
	{
		++m_CurrentFrame;
	}

	// may change state
	AutoState autoState( this );

// Handle timed function polling.

	{
		for ( IntColl::iterator i = m_AtFunctions.begin() ; i != m_AtFunctions.end() ; )
		{
			// get the function
			int index = *i;
			const Raw::ExportEntry* func = GetImpl()->GetFunctionSpec( index );
			gpassert( func && func->HasAtTime() );

			// see if it's time to go
			bool triggerIt = func->IsTimeInFrames()
						   ? (m_CurrentFrame >= func->GetTimeInFrames ())
						   : (m_CurrentTime  >= func->GetTimeInSeconds());

			// do it
			if ( triggerIt )
			{
				// remove
				i = m_AtFunctions.erase( i );
				polled = true;

				// trigger
				Call( index );
				if ( IsStateChangePending() )
				{
					return ( true );
				}
			}
			else
			{
				// advance
				++i;
			}
		}
	}

// Handle timer polling.

	// only check this if not nested
	if ( m_StateChangeDepth <= 1 )
	{
		typedef stdx::fast_vector <std::pair <int, float> > TimerCommandColl;

		TimerCommandColl timerCommands;
		for ( TimerColl::iterator i = m_Timers.begin() ; i != m_Timers.end() ; )
		{
			// see if it's time to go
			bool triggerIt = i->m_TimeInFrames
						   ? (m_CurrentFrame >= i->m_FrameTimeState.m_Frame)
						   : (m_CurrentTime  >= i->m_FrameTimeState.m_Time );

			// do it
			if ( triggerIt )
			{
				// get id and delta
				int id = i->m_Id;
				float delta = i->m_TimeInFrames ? 0 : (float)(m_CurrentTime - i->m_FrameTimeState.m_Time);

				// erase if no more
				if ( (i->m_Remaining > 0) && (--i->m_Remaining == 0) )
				{
					i = m_Timers.erase( i );
				}
				else
				{
					// update for next time
					if ( i->m_TimeInFrames )
					{
						i->m_FrameTimeState.m_Frame = m_CurrentFrame + i->m_FrameTimeSpec.m_Frame;
					}
					else
					{
						i->m_FrameTimeState.m_Time = m_CurrentTime + i->m_FrameTimeSpec.m_Time;
					}
					++i;
				}

				// queue it
				timerCommands.push_back( std::make_pair( id, delta ) );
			}
			else
			{
				++i;
			}
		}

		// fire off timer events
		for ( TimerCommandColl::iterator j = timerCommands.begin() ; j != timerCommands.end() ; ++j )
		{
			FuBiEvent::OnTimer( this, j->first, j->second );
			if ( IsStateChangePending() )
			{
				return ( true );
			}
		}
	}

// Handle state transition polling.

	// only poll if we need to
	if ( (m_StateEntry != NULL) && m_IsPolling )
	{
		// advance
		m_CurrentPoll += deltaTime;

		// check for next poll
		if ( m_CurrentPoll >= m_PollPeriod )
		{
			// reset poll time
			m_CurrentPoll = 0;

			// check local event handler
			FuBiEvent::OnTransitionPoll( this );

			// only process transitions if we aren't changing states
			if ( !IsStateChangePending() )
			{
				// check all transitions
				TriggerIndex::iterator i, ibegin = m_Triggers.begin(), iend = m_Triggers.end();
				StateEntry::TransitionIndex::const_iterator ti = m_StateEntry->m_Transitions.begin();
				for ( i = ibegin ; i != iend ; ++i, ++ti )
				{
					// only check those with all static triggers hit
					if ( (i->m_Remaining == 0) && PrivateCheckDynamic( &*ti ) )
					{
						// got them all? state transition time!
						PrivateTransition( &*ti );
						polled = true;
						break;
					}
				}
			}
		}
	}

	// done
	return ( polled );
}

int Object :: CreateTimer( float timeFromNow )
{
	int id = m_NextTimerId;
	CreateTimer( id, timeFromNow );
	return ( id );
}

void Object :: CreateTimer( int id, float timeFromNow )
{
	CreateTimer( id, &timeFromNow, NULL );
}

int Object :: CreateFrameTimer( int framesFromNow )
{
	int id = m_NextTimerId;
	CreateFrameTimer( id, framesFromNow );
	return ( id );
}

void Object :: CreateFrameTimer( int id, int framesFromNow )
{
	CreateTimer( id, NULL, &framesFromNow );
}

void Object :: DestroyTimer( int id )
{
	TimerColl::iterator timer = FindTimer( id );
	if ( timer != m_Timers.end() )
	{
		m_Timers.erase( timer );
	}
}

void Object :: SetTimerGlobal( int id, bool global )
{
	TimerColl::iterator timer = FindTimer( id );
	if ( timer != m_Timers.end() )
	{
		timer->m_Global = global;
	}
}

void Object :: SetTimerRepeatCount( int id, int count )
{
	TimerColl::iterator timer = FindTimer( id );
	if ( timer != m_Timers.end() )
	{
		timer->m_Remaining = count;
	}
}

float Object :: AddTimerSeconds( int id, float extraTime )
{
	TimerColl::iterator timer = FindSecondsTimer( id );
	if ( timer != m_Timers.end() )
	{
		timer->m_FrameTimeSpec .m_Time += extraTime;
		timer->m_FrameTimeState.m_Time += extraTime;
		return ( (float)(timer->m_FrameTimeState.m_Time - m_CurrentTime) );
	}
	else
	{
		return ( 0 );
	}
}

void Object :: ResetTimerSeconds( int id, float timeFromBase )
{
	TimerColl::iterator timer = FindSecondsTimer( id );
	if ( timer != m_Timers.end() )
	{
		timer->m_FrameTimeState.m_Time -= timer->m_FrameTimeSpec.m_Time;
		timer->m_FrameTimeState.m_Time += timeFromBase;
		timer->m_FrameTimeSpec .m_Time = timeFromBase;
	}
}

void Object :: SetNewTimerSeconds( int id, float timeFromNow )
{
	TimerColl::iterator timer = FindSecondsTimer( id );
	if ( timer != m_Timers.end() )
	{
		timer->m_FrameTimeSpec .m_Time = timeFromNow;
		timer->m_FrameTimeState.m_Time = m_CurrentTime + timeFromNow;
	}
}

void Object :: SetState( int newState )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	gpassert( IsConstructed() );
	m_PendingState = newState;

	// $ early bailout - buffer state change for later
	if ( m_StateChangeDepth > 0 )
	{
		return;
	}

#	if !GP_RETAIL
	ReportSys::AutoIndent autoIndent( &gSkritContext, false );
	if ( m_TraceStateChanges )
	{
		gpskritf(( "(%s) 0x%08X STATECHANGE: old = %s, new = %s\n",
				   FileSys::GetFileNameOnly( GetName() ).c_str(),
				   this,
				   GetCurrentStateName(),
				   GetPendingStateName() ));
		autoIndent.Indent();
	}
#	endif // !GP_RETAIL

	// add depth to catch state changes into pending
	++m_StateChangeDepth;

	// play exit event (this may redirect the state and be caught in pending)
	FuBiEvent::OnExitState( this );

	// rebuild trigger index
	m_StateEntry = scast <const ObjectImpl*> ( m_Impl )->GetStateEntry( m_PendingState );
	if ( m_StateEntry != NULL )
	{
		// alloc space in our triggers
		PrivateResetTriggers();

		// set new polling info
		if ( m_StateEntry->m_Config != NULL )
		{
			m_PollPeriod = m_StateEntry->m_Config->m_PollPeriod;
			m_IsPolling = (m_PollPeriod >= 0) && m_StateEntry->m_NeedsPoll;
			m_CurrentPoll = 0;
		}
		else
		{
			m_IsPolling = false;
		}
	}
	else
	{
		// no state info
		m_Triggers.clear();

		// clear out polling info
		m_IsPolling = false;
	}

	// rebuild 'at' function index
	m_AtFunctions.clear();
	{
		for ( int i = 0, count = GetImpl()->GetFunctionCount() ; i != count ; ++i )
		{
			// get function
			const Raw::ExportEntry* func = GetImpl()->GetFunctionSpec( i );

			// see if it has an at-time
			if ( func->HasAtTime() )
			{
				// see if it is in the right state
				if ( (WORD)newState == func->m_StateMember )
				{
					m_AtFunctions.push_back( i );
				}
			}
		}
	}

	// delete local timers (don't do it for initial construction though!)
	if ( (m_CurrentState != STATE_INDEX_NONE) && (m_PendingState != STATE_INDEX_NONE) )
	{
		for ( TimerColl::iterator i = m_Timers.begin() ; i != m_Timers.end() ; )
		{
			if ( !i->m_Global )
			{
				i = m_Timers.erase( i );
			}
			else
			{
				// adjust backwards because we'll be resetting the time/frame
				if ( i->m_TimeInFrames )
				{
					i->m_FrameTimeState.m_Frame -= m_CurrentFrame;
				}
				else
				{
					i->m_FrameTimeState.m_Time -= m_CurrentTime;
				}
				++i;
			}
		}
	}

	// reset the time info
	m_CurrentTime = 0;
	m_CurrentFrame = 0;

	// take state
	m_CurrentState = m_PendingState;
	m_PendingState = STATE_INDEX_INVALID;

	// done with changes
	--m_StateChangeDepth;
	gpassert( m_StateChangeDepth == 0 );

	// play enter event (this may change state too, that's ok we're in the clear now)
	FuBiEvent::OnEnterState( this );
}

Result Object :: Call( int funcIndex, const FuBi::FunctionSpec* spec, const_mem_ptr params )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	gpassert( IsConstructed() );

	if ( m_Impl->CheckFunctionMatch( funcIndex, spec ) )
	{
		Call( funcIndex, params );
	}
	else
	{
		m_LastResult = RESULT_BADPARAMS;
	}

	return ( m_LastResult );
}

Result Object :: Call( const char* funcName, const FuBi::FunctionSpec* spec, const_mem_ptr params )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	int funcIndex = FindFunction( funcName );
	if ( funcIndex != INVALID_FUNCTION_INDEX )
	{
		Call( funcIndex, spec, params );
	}
	else
	{
		m_LastResult = RESULT_NOFUNCTION;
	}

	return ( m_LastResult );
}

Result Object :: Call( int funcIndex, const_mem_ptr params )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	gpassert( IsConstructed() );

	m_LastResult = AutoMachine( this )->Call( m_Impl->GetFunctionSpec( funcIndex ), params );
	return ( m_LastResult );
}

Result Object :: Call( const char* funcName, const_mem_ptr params )
{
	GPSTATS_SYSTEM( SP_SKRIT );

	int funcIndex = FindFunction( funcName );
	if ( funcIndex != INVALID_FUNCTION_INDEX )
	{
		Call( funcIndex, params );
	}
	else
	{
		gperrorf(( "Skrit::Object: cannot find function named '%s'\n", funcName ));
		m_LastResult = RESULT_NOFUNCTION;
	}
	return ( m_LastResult );
}

#if !GP_RETAIL

void Object :: GenerateDocs( ReportSys::Context* ctx ) const
{
	m_Impl->GetPack()->GenerateDocs( ctx );
}

void Object :: GenerateDocs( void ) const
{
	ReportSys::AutoReport autoReport( &gGenericContext );
	gpgenericf(( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				  "Dumping '%s':\n\n", GetName().c_str() ));
	GenerateDocs( &gGenericContext );
}

const char* Object :: GetStateName( int state ) const
{
	static gpstring s_StateName;

	const Raw::DebugDb* debugDb = m_Impl->GetDebugDb();
	if ( debugDb == NULL )
	{
		s_StateName = "<debug info not available>";
	}
	else
	{
		s_StateName = debugDb->GetValidStateName( state );
	}

	return ( s_StateName );
}

const char* Object :: GetCurrentStateName( void ) const
{
	static gpstring s_StateName;
	s_StateName = GetStateName( GetCurrentState() );
	return ( s_StateName );
}

const char* Object :: GetPendingStateName( void ) const
{
	static gpstring s_StateName;
	s_StateName = GetStateName( GetPendingState() );
	return ( s_StateName );
}

void Object :: Disassemble( ReportSys::Context* ctx, FileSys::Reader* base ) const
{
	m_Impl->GetPack()->DisassembleMixed( ctx, base );
}

void Object :: Disassemble( ReportSys::Context* ctx ) const
{
	Disassemble( ctx, NULL );
}

void Object :: Disassemble( void ) const
{
	ReportSys::AutoReport autoReport( &gGenericContext );
	gpgenericf(( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				  "Disassembling '%s':\n\n", GetName().c_str() ));
	Disassemble( &gGenericContext );
}

void Object :: Dump( ReportSys::Context* ctx ) const
{
	GenerateDocs( ctx );
	ctx->OutputF( "Last result = %d:%d\n", m_LastResult.m_Code, m_LastResult.m_Raw );
}

void Object :: Dump( void ) const
{
	GenerateDocs();
	gpgenericf(( "Last result = %d:%d\n", m_LastResult.m_Code, m_LastResult.m_Raw ));
}

void Object :: DumpStateMachine( ReportSys::Context* ctx ) const
{
	ReportSys::AutoReport autoReport( ctx );

	// get debug db, bail out if missing
	const Raw::DebugDb* debugDb = m_Impl->GetDebugDb();
	if ( debugDb == NULL )
	{
		ctx->OutputF( "No debug info available, unable to dump state machine for '%s'\n", GetName().c_str() );
		return;
	}

	// basic info
	ctx->OutputF( "state machine = '%s'\ncurrent = %s\npending = %s\n",
				  GetName().c_str(),
				  debugDb->GetValidStateName( m_CurrentState ).c_str(),
				  debugDb->GetValidStateName( m_PendingState ).c_str() );

	// polling
	if ( m_IsPolling )
	{
		if ( m_PollPeriod == 0 )
		{
			ctx->Output( "polling period = <every frame>\n" );
		}
		else
		{
			ctx->OutputF( "polling period = %f%s\n",
						  (m_PollPeriod < 1.0f) ? (m_PollPeriod * 1000.0f) : m_PollPeriod,
						  (m_PollPeriod < 1.0f) ? "ms" : "s" );
		}
	}

	// at functions
	if ( !m_AtFunctions.empty() )
	{
		ctx->OutputF( "'at' functions:\n" );

		IntColl::const_iterator i, ibegin = m_AtFunctions.begin(), iend = m_AtFunctions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			const Raw::ExportEntry* func = GetImpl()->GetFunctionSpec( *i );
			if ( func->IsTimeInFrames() )
			{
				ctx->OutputF( "\t%s (%d frames remaining, %d total)\n",
							  func->m_Name, func->GetTimeInFrames() - m_CurrentFrame, func->GetTimeInFrames() );
			}
			else
			{
				ctx->OutputF( "\t%s (%d msec remaining, %d total)\n",
							  func->m_Name, func->GetTimeInMsec() - (int)( m_CurrentTime / 1000.0f), func->GetTimeInMsec() );
			}
		}
	}

	// if not in any current state bail out
	if ( m_StateEntry == NULL )
	{
		return;
	}

	// static transition map
	StateEntry::TransitionIndex::const_iterator
			ispec,
			ibegin = m_StateEntry->m_Transitions.begin(),
			iend   = m_StateEntry->m_Transitions.end();
	TriggerIndex::const_iterator itrigger = m_Triggers.begin();
	for ( ispec = ibegin ; ispec != iend ; ++ispec, ++itrigger )
	{
		DumpTransitionEntry( ctx, *debugDb, *ispec, *itrigger );
	}

	// global triggers
	if ( itrigger != m_Triggers.end() )
	{
		const StateEntry* global = scast <const ObjectImpl*> ( m_Impl )->GetStateEntry( STATE_INDEX_NONE );
		gpassert( global != m_StateEntry );
		gpassert( m_Triggers.size() == (m_StateEntry->m_Transitions.size() + global->m_Transitions.size()) );

		ibegin = global->m_Transitions.begin(),
		iend   = global->m_Transitions.end();
		for ( ispec = ibegin ; ispec != iend ; ++ispec, ++itrigger )
		{
			DumpTransitionEntry( ctx, *debugDb, *ispec, *itrigger );
		}
	}
}

void Object :: DumpStateMachine( void ) const
{
	ReportSys::AutoReport autoReport( &gGenericContext );
	gpgenericf(( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				  "Dumping state machine for '%s':\n\n", GetName().c_str() ));
	DumpStateMachine( &gGenericContext );
}

void Object :: DumpStateMachine( gpstring& dump, int indent ) const
{
	ReportSys::LocalContext ctx( new ReportSys::StringSink( dump ), true );
	ctx.SetIndentSpaces( indent );
	ctx.Indent();
	DumpStateMachine( &ctx );
}

#endif // !GP_RETAIL

void Object :: EnterPotentialStateChange( void )
{
	gpassert( IsConstructed() );
	gpassert( m_StateChangeDepth >= 0 );

	++m_StateChangeDepth;

	GPDEV_ONLY( ++m_DebugStateChangeDepth );
}

void Object :: LeavePotentialStateChange( void )
{
	gpassert( IsConstructed() );
	gpassert( m_StateChangeDepth >= 1 );
	--m_StateChangeDepth;

#	if !GP_RETAIL
	if ( m_DebugStateChangeDepth > 15 )
	{
		gperrorf(( "SKRIT: potential infinite recursion detected!\n"
				   "\n"
				   "Skrit         = %s\n"
				   "'this'        = 0x%08X\n"
				   "Current State = %s\n"
				   "Pending State = %s\n",
				   GetName().c_str(),
				   this,
				   GetCurrentStateName(),
				   GetPendingStateName() ));
	}
#	endif // !GP_RETAIL

	if ( m_StateChangeDepth == 0 )
	{
		Result savedResult = m_LastResult;
		if ( IsStateChangePending() )
		{
			SetState( m_PendingState );
		}
		m_LastResult = savedResult;
	}

	GPDEV_ONLY( --m_DebugStateChangeDepth );
}

void Object :: PrivateResetTriggers( void )
{
	if ( m_StateEntry == NULL )
	{
		return;
	}

	m_Triggers.resize( m_StateEntry->m_Transitions.size() );

	// rebuild each trigger entry
	TriggerIndex::iterator i, begin = m_Triggers.begin(), end = m_Triggers.end();
	StateEntry::TransitionIndex::const_iterator ti = m_StateEntry->m_Transitions.begin();
	for ( i = begin ; i != end ; ++i, ++ti )
	{
		i->Reset( ti->m_StaticIndex.size() );
	}

	// add global triggers if any
	const StateEntry* global = scast <const ObjectImpl*> ( m_Impl )->GetStateEntry( STATE_INDEX_NONE );
	if ( (global != m_StateEntry) && (global != NULL) )
	{
		size_t oldSize = m_Triggers.size();
		m_Triggers.resize( oldSize + global->m_Transitions.size() );

		// rebuild each trigger entry
		TriggerIndex::iterator i, begin = m_Triggers.begin() + oldSize, end = m_Triggers.end();
		StateEntry::TransitionIndex::const_iterator ti = global->m_Transitions.begin();
		for ( i = begin ; i != end ; ++i, ++ti )
		{
			i->Reset( ti->m_StaticIndex.size() );
		}
	}
}

bool Object :: PrivateCheckTriggers( const ObjectImpl::StateEntry* stateEntry, UINT eventSerial, const void* params )
{
	gpassert( stateEntry != NULL );

	bool transitioned = false;

	// find this event in our current state's trigger set
	StateEntry::TriggerIndex::const_iterator found = stateEntry->m_StaticTriggers.find( eventSerial );
	if ( found != stateEntry->m_StaticTriggers.end() )
	{
		StateEntry::TriggerColl::const_iterator i, begin = found->second.begin(), end = found->second.end();
		for ( i = begin ; i != end ; ++i )
		{
			// get at our test entries
			int transIndex = i->m_TransitionIndex;
			const ObjectImpl::TransitionEntry& transEntry = stateEntry->m_Transitions[ transIndex ];

			// optionally adjust index for global triggers stored at the end
			if ( (m_StateEntry != stateEntry) && (m_StateEntry != NULL) )
			{
				transIndex += m_StateEntry->m_Transitions.size();
			}

			// get at our current triggers
			gpassert( transIndex < (int)m_Triggers.size() );
			TriggerEntry& triggerEntry = m_Triggers[ transIndex ];

			// if all the static triggers are set, try the dynamic ones too
			if (   triggerEntry.CheckAndSet( i->m_StaticIndex, transEntry.m_StaticIndex[ i->m_StaticIndex ], params )
				&& PrivateCheckDynamic( &transEntry ) )
			{
				// we have a hit! do the transition and we're outta here
				PrivateTransition( &transEntry );
				transitioned = true;
				break;
			}
		}
	}

	return ( transitioned );
}

void Object :: PrivateTransition( const ObjectImpl::TransitionEntry* transition )
{
	gpassert( IsConstructed() );
	gpassert( !IsStateChangePending() );

	// may change state
	AutoState autoState( this );

	// rebuild each trigger entry
	TriggerIndex::iterator i, begin = m_Triggers.begin(), end = m_Triggers.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->Reset( i->m_Triggers.size() );
	}

	// do transition response
	DWORD response = transition->m_Transition->m_ResponseCodeOffset;
	if ( response != INVALID_FUNCTION_INDEX )
	{
		m_LastResult = AutoMachine( this )->CallFragment( response );
	}

	// only change state if response didn't do it itself
	if ( !IsStateChangePending() )
	{
		// triggers have a to-state of none, don't transition for those
		int state = GetStateIndex( transition->m_Transition->m_ToState );
		if ( state != STATE_INDEX_NONE )
		{
			SetState( state );
		}
	}
}

bool Object :: PrivateCheckDynamic( const ObjectImpl::TransitionEntry* transition )
{
	gpassert( IsConstructed() );

	// do all dynamic triggers
	ObjectImpl::TransitionEntry::DynamicIndex::const_iterator i,
			begin = transition->m_DynamicIndex.begin(),
			end   = transition->m_DynamicIndex.end();
	for ( i = begin ; i != end ; ++i )
	{
		// call fragment as an expression
		DWORD offset = (*i)->m_CodeOffset;
		m_LastResult = AutoMachine( this )->CallFragment( offset );

		// only is a hit if not an error and nonzero result
		if ( m_LastResult.IsError() || (m_LastResult.m_Raw == 0) )
		{
			return ( false );
		}
	}

	// all ok
	return ( true );
}

Object::TimerColl::iterator Object :: FindTimer( int id, bool warnOnFail )
{
	TimerColl::iterator found = stdx::find( m_Timers, id );
	if ( found == m_Timers.end() )
	{
		if ( warnOnFail )
		{
			gperrorf(( "Unable to find a timer of id %d in this Skrit\n", id ));
		}
	}

	return ( found );
}

Object::TimerColl::iterator Object :: FindSecondsTimer( int id, bool warnOnFail )
{
	TimerColl::iterator found = FindTimer( id, warnOnFail );
	if ( (found != m_Timers.end()) && found->m_TimeInFrames )
	{
		if ( warnOnFail )
		{
			gperrorf(( "Timer of id %d is a 'frame' timer, expected 'seconds' timer instead\n", id ));
		}
		found = m_Timers.end();
	}

	return ( found );
}

Object::TimerColl::iterator Object :: FindFramesTimer( int id, bool warnOnFail )
{
	TimerColl::iterator found = FindTimer( id, warnOnFail );
	if ( (found != m_Timers.end()) && !found->m_TimeInFrames )
	{
		if ( warnOnFail )
		{
			gperrorf(( "Timer of id %d is a 'seconds' timer, expected 'frames' timer instead\n", id ));
		}
		found = m_Timers.end();
	}

	return ( found );
}

void Object :: CreateTimer( int id, float* timeFromNow, int* framesFromNow )
{
	// keep the next id ahead of anything we request to avoid collisions
	if ( id >= m_NextTimerId )
	{
		m_NextTimerId = id + 1;
	}

	// check for existing timer to modify
	TimerColl::iterator timer = stdx::find( m_Timers, id );
	if ( timer == m_Timers.end() )
	{
		timer = m_Timers.insert( timer );
	}

	// set it
	if ( timeFromNow != NULL )
	{
		timer->m_FrameTimeSpec.m_Time = *timeFromNow;
		timer->m_FrameTimeState.m_Time = m_CurrentTime + *timeFromNow;
		timer->m_TimeInFrames = false;
	}
	else
	{
		gpassert( framesFromNow != NULL );
		timer->m_FrameTimeSpec.m_Frame = *framesFromNow;
		timer->m_FrameTimeState.m_Frame = m_CurrentFrame + *framesFromNow;
		timer->m_TimeInFrames = true;
	}

	// update other info
	timer->m_Global = false;
	timer->m_Remaining = 1;
	timer->m_Id = id;
}

#if !GP_RETAIL

void Object :: DumpTransitionEntry(
		ReportSys::ContextRef ctx,
		const Raw::DebugDb& debugDb,
		const ObjectImpl::TransitionEntry& spec,
		const TriggerEntry& trigger ) const
{
	ctx->OutputF( "-> %s: ", debugDb.GetValidStateName( spec.m_Transition->m_ToState ).c_str() );
	gpstring funcName;

	ObjectImpl::TransitionEntry::StaticIndex::const_iterator
		jspec,
		jbegin = spec.m_StaticIndex.begin(),
		jend   = spec.m_StaticIndex.end();
	TriggerEntry::TriggerColl::const_iterator jtrigger = trigger.m_Triggers.begin();
	gpassert( trigger.m_Triggers.size() == spec.m_StaticIndex.size() );
	for ( jspec = jbegin ; jspec != jend ; ++jspec, ++jtrigger )
	{
		// output function
		funcName = gFuBiSysExports.FindFunctionBySerialID( (*jspec)->m_EventSerial )->m_Name;
		if ( *jtrigger )
		{
			funcName.to_upper();
			ctx->Output( "+" );
		}
		else
		{
			funcName.to_lower();
			ctx->Output( "-" );
		}
		ctx->Output( funcName );

		// output static trigger data if any
		if ( (*jspec)->m_TestDataSize > 0 )
		{
			ctx->Output( "( " );
			(*jspec)->OutputParams( ctx );
			ctx->Output( " )" );
		}

		// pad
		ctx->Output( " " );
	}

	ctx->OutputEol();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct Object::TriggerEntry implementation

bool Object::TriggerEntry :: Xfer( FuBi::PersistContext& persist )
{
	persist.XferExistingColl( "m_Triggers", m_Triggers.begin(), m_Triggers.end() );
	persist.Xfer( "m_Remaining", m_Remaining );
	return ( true );
}

bool Object::TriggerEntry :: CheckAndSet( int trigger, const Raw::ConditionStatic* check, const void* params )
{
	// verify trigger index
	gpassert( (trigger >= 0) && (trigger < scast <int> ( m_Triggers.size() )) );

	// only bother if not already a hit
	if ( !m_Triggers[ trigger ] )
	{
		// should never have this condition
		gpassert( m_Remaining > 0 );

		// test to make sure our params match up
		if ( ::memcmp( check->GetDataBegin(), params, check->m_TestDataSize ) == 0 )
		{
			// it's a hit
			m_Triggers[ trigger ] = true;
			--m_Remaining;
		}
	}

	// all done if all statics are hit
	return ( m_Remaining == 0 );
}

//////////////////////////////////////////////////////////////////////////////
// struct Object::AutoMachine implementation

Object::AutoMachine :: AutoMachine( Object* object )
	: m_AutoState( object )
{
	m_Machine = gSkritEngine.AcquireMachine( object, m_MachineIndex );
}

Object::AutoMachine :: ~AutoMachine( void )
{
	gSkritEngine.ReleaseMachine( m_MachineIndex );
}

//////////////////////////////////////////////////////////////////////////////
// struct Object::TimerEntry implementation

bool Object::TimerEntry :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_TimeInFrames", m_TimeInFrames );
	persist.Xfer( "m_Global",       m_Global       );
	persist.Xfer( "m_Remaining",    m_Remaining    );
	persist.Xfer( "m_Id",           m_Id           );

	if ( m_TimeInFrames )
	{
		persist.Xfer( "m_FrameTimeSpec.m_Frame", m_FrameTimeSpec.m_Frame );
		persist.Xfer( "m_FrameTimeState.m_Frame", m_FrameTimeState.m_Frame );
	}
	else
	{
		persist.Xfer( "m_FrameTimeSpec.m_Time", m_FrameTimeSpec.m_Time );
		persist.Xfer( "m_FrameTimeState.m_Time", m_FrameTimeState.m_Time );
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
