//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritEngine.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"

#include "DebugHelp.h"
#include "FileSys.h"
#include "FuBiPersist.h"
#include "FuBiSchemaImpl.h"
#include "FuBiTraits.h"
#include "Fuel.h"
#include "GpStatsDefs.h"
#include "NamingKey.h"
#include "RatioStack.h"
#include "Skrit.h"
#include "SkritEngine.h"
#include "SkritMachine.h"
#include "SkritStructure.h"
#include "SkritSupport.h"
#include "StringTool.h"

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class Engine implementation

#if GP_RETAIL
#define CHECK_DB_LOCKED
#define CHECK_DB_LOCKED_HANDLE( x )
#define CHECK_DB_LOCKED_REQUEST( x )
#else // GP_RETAIL
#define CHECK_DB_LOCKED												\
	if ( gSkritEngine.IsLockedForSaveLoad() )						\
	{																\
		gperror( "Very bad! Someone is attempting to "				\
				 "modify the Skrit Engine during a load/save!!" );	\
	}
#define CHECK_DB_LOCKED_HANDLE( x )									\
	if ( (x).IsPersistent() )										\
	{																\
		CHECK_DB_LOCKED;											\
	}
#define CHECK_DB_LOCKED_REQUEST( x )								\
	if (   ((x).m_PersistType != PERSIST_NO)						\
		|| !(x).m_DeferConstruction )								\
	{																\
		CHECK_DB_LOCKED;											\
	}
#endif // GP_RETAIL

Engine :: Engine( void )
	: m_PersistentObjects   ( HObject::MAX_MAGIC )
	, m_NonPersistentObjects( HObject::MAX_MAGIC )
{
	// init members
	m_Options           = OPTION_NONE;
	m_LockedForSaveLoad = false;
	m_Compiler          = new Compiler;

	// install handler for game stack
#	if !GP_ERROR_FREE
	DbgSymbolEngine::RegisterStackQueryCb( makeFunctor( Machine::StackQueryProc ) );
#	endif // !GP_ERROR_FREE
}

Engine :: ~Engine( void )
{
	// nuke leftovers
	ResetCache();

	// detect leaks
#	if !GP_RETAIL
	if ( !m_ObjectByNameIndex.empty() )
	{
		ReportSys::AutoReport autoReport( &gErrorContext );
		gperror( "Skrit::Engine shut down with open Skrit objects! Leaks are bad!\n\nLeaks:\n\n" );

		StringToObjectMap::iterator i, ibegin = m_ObjectByNameIndex.begin(), iend = m_ObjectByNameIndex.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			ReportSys::AutoIndent autoIndent( &gErrorContext );
			gperrorf(( "%s\n", i->first.c_str() ));

			ObjectList::iterator j, jbegin = i->second.m_Instances.begin(), jend = i->second.m_Instances.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j != jbegin )
				{
					gperror( ", " );
				}
				gperrorf(( "0x%08X", *j ));
			}
			gperror( "\n" );
		}
	}
#	endif // !GP_RETAIL

	// kill owned objects
	delete ( m_Compiler );

	// kill machines
	stdx::for_each( m_MachineCache, stdx::delete_by_ptr() );

	// don't need our translator any more
#	if !GP_ERROR_FREE
	DbgSymbolEngine::RegisterStackQueryCb( makeFunctor( Machine::StackQueryProc ) );
#	endif // !GP_ERROR_FREE
}

void Engine :: SetOptions( eOptions options, bool set )
{
#	if !GP_RETAIL
	bool oldRetailMode = TestOptions( OPTION_RETAIL_MODE );
#	endif // !GP_RETAIL

	m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));

#	if !GP_RETAIL
	bool newRetailMode = TestOptions( OPTION_RETAIL_MODE );
	if ( oldRetailMode != newRetailMode )
	{
		GetCompiler().RegisterDefaultOnlyConditions();
	}
#	endif // !GP_RETAIL
}

void Engine :: ResetCache( void )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED;

	stdx::for_each( m_ObjectCache, stdx::delete_second_by_ptr() );
	m_ObjectCache.clear();
	m_ObjectCacheList.clear();
}

bool Engine :: Xfer( FuBi::PersistContext& persist )
{
	RatioSample ratioSample( "xfer_skrit_engine", 0, 0 );

	// $ don't persist options

	// setup
	if ( persist.IsRestoring() )
	{
		// can't have anything allocated
#		if !GP_RETAIL
		if ( !m_PersistentObjects.empty() )
		{
			ReportSys::AutoReport autoReport( &gErrorContext );
			gperror( "Skrit::Engine shut down with open persistent Skrit objects! Leaks are bad!\n\nLeaks:\n\n" );

			ObjectDb::iterator i, ibegin = m_PersistentObjects.begin(), iend = m_PersistentObjects.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				ReportSys::AutoIndent autoIndent( &gErrorContext );
				gperrorf(( "name = %s, refs = %d, hobject = 0x%08X\n",
						   i->m_NameEntry->first.c_str(), i->m_RefCount, *i->m_ListEntry ));
			}
		}
#		endif // !GP_RETAIL

		// remove any persistent handle entries from the index
		if ( !m_PersistentObjects.empty() )
		{
			StringToObjectMap::iterator i, ibegin = m_ObjectByNameIndex.begin(), iend = m_ObjectByNameIndex.end();
			for ( i = ibegin ; i != iend ; )
			{
				ObjectList::iterator j, jbegin = i->second.m_Instances.begin(), jend = i->second.m_Instances.end();
				for ( j = jbegin ; j != jend ; )
				{
					if ( j->IsPersistent() )
					{
						j = i->second.m_Instances.erase( j );
					}
					else
					{
						++j;
					}
				}

				if ( i->second.m_Instances.empty() )
				{
					i = m_ObjectByNameIndex.erase( i );
				}
				else
				{
					++i;
				}
			}
		}

		// free up memory we were using
		m_PersistentObjects.Shutdown();
	}

	// xfer object handle magics
	UINT magic = m_PersistentObjects.GetCurrentMagic();
	persist.Xfer( "m_PersistentObjectsMagic", magic );
	m_PersistentObjects.SetCurrentMagic( magic );

	// xfer index and objects
	bool success = true;
	if ( persist.EnterColl( "m_ObjectByNameIndex" ) )
	{
		if ( persist.IsSaving() )
		{
			RatioSample ratioSample( "xfer_objects", m_ObjectByNameIndex.size() );

			StringToObjectMap::iterator i, ibegin = m_ObjectByNameIndex.begin(), iend = m_ObjectByNameIndex.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				ObjectShared& shared = i->second;

				// check for dups
#				if !GP_RETAIL
				{
					ObjectList instances = shared.m_Instances;
					instances.sort();
					size_t size = instances.size();
					instances.unique();
					if ( instances.size() != size )
					{
						gperrorf((
								"TELL SCOTT: something horrible happened - "
								"while I was saving the game I discovered that "
								"there are duplicate skrit instance handles! "
								"This save game is invalid, something very bad "
								"happened recently in the code. Go TELL SCOTT!" ));
					}
				}
#				endif // !GP_RETAIL

				// see if at least one requires it
				ObjectList::iterator j, jbegin = shared.m_Instances.begin(), jend = shared.m_Instances.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( j->m_Persist )
					{
						break;
					}
				}

				// only save those that may be persisted
				if ( j != jend )
				{
					persist.AdvanceCollIter();

					// save name (ccast ok, we're always saving here)
					persist.Xfer( "_key", ccast <gpstring&> ( i->first ) );

					// save shared
					persist.EnterBlock( "m_Impl" );
					shared.m_Impl->Xfer( persist );
					persist.LeaveBlock();

					// save instances
					persist.EnterColl( "m_Instances" );
					for ( ; j != jend ; ++j )
					{
						if ( j->m_Persist )
						{
							// advance
							persist.AdvanceCollIter();

							// xfer the handle id and refcount
							persist.Xfer( FuBi::PersistContext::VALUE_STR, *j );
							int refCount = m_PersistentObjects.Dereference( j->m_Index, j->m_Magic )->m_RefCount;
							persist.Xfer( "_refs", refCount );

							// xfer the object
							(*j)->Xfer( persist );
						}
					}
					persist.LeaveColl();
				}

				ratioSample.Advance( i->first );
			}
		}
		else
		{
			gpstring key, namingKeyName;
			while ( persist.AdvanceCollIter() )
			{
				// load name
				key.clear();
				persist.Xfer( "_key", key );

				// create impl on demand
				std::pair <StringToObjectMap::iterator, bool> rc
						= m_ObjectByNameIndex.insert( std::make_pair( key, ObjectShared() ) );
				ObjectShared& shared = rc.first->second;
				if ( rc.second )
				{
					gpassert( shared.m_Impl == NULL );
					shared.m_Impl = new Skrit::ObjectImpl;
				}
				gpassert( shared.m_Impl != NULL );

				// resolve with naming key
				if ( NamingKey::DoesSingletonExist() && gNamingKey.BuildSkritLocation( key, namingKeyName ) )
				{
					key = namingKeyName;
				}
				else
				{
					// auto-add ".skrit" if no extension
					key = AddSkritExtension( key );
				}

				// load shared
				persist.EnterBlock( "m_Impl" );
				if ( !shared.m_Impl->Xfer( persist, &key ) )
				{
					success = false;
				}
				persist.LeaveBlock();

				// load instances
				if ( persist.EnterColl( "m_Instances" ) )
				{
					// for each
					while ( persist.AdvanceCollIter() )
					{
						// build a new object and load it
						Object* object = new Object( shared.m_Impl, NULL, true );
						if ( !object->Xfer( persist ) )
						{
							success = false;
						}

						// get the handle
						HObject handle;
						persist.Xfer( FuBi::PersistContext::VALUE_STR, handle.m_Handle );

						// add to db
						ObjectInstance* instance = NULL;
						if ( m_PersistentObjects.AllocSpecific( instance, handle.m_Index, handle.m_Magic ) )
						{
							gpassert( instance != NULL );

							// fill in the rest
							instance->m_Instance = object;
							instance->m_NameEntry = rc.first;
							instance->m_ListEntry = shared.m_Instances.insert( shared.m_Instances.end(), handle );

							// xfer the ref count
							persist.Xfer( "_refs", instance->m_RefCount );
						}
						else
						{
							gperrorf(( /*"TELL SCOTT: "$$$ */ "something horrible happened - during game load I ended up with a duplicate instance of '%s'!!!", key.c_str() ));
						}
					}
				}
				else
				{
					success = false;
				}
				persist.LeaveColl();
			}
		}
	}
	persist.LeaveColl();

	// errors restoring skrits are fatal, sorry...
	if ( !success )
	{
		persist.SetWatsonError();
	}

	return ( success );
}

bool Engine :: CommitLoad( void )
{
	CHECK_DB_LOCKED;
	bool success = true;

	// commit all shared objects if they have any persistent instances
	StringToObjectMap::iterator i, ibegin = m_ObjectByNameIndex.begin(), iend = m_ObjectByNameIndex.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		ObjectList& objects = i->second.m_Instances;

		// see if any are persistent
		ObjectList::iterator j, jbegin = objects.begin(), jend = objects.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( j->m_Persist )
			{
				break;
			}
		}

		// persistent? commit shared and all persistent instances
		if ( (j != jend) && i->second.m_Impl->CommitLoad() )
		{
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j->m_Persist && !(*j)->CommitLoad() )
				{
					success = false;
				}
			}
		}
		else
		{
			success = false;
		}
	}

	return ( success );
}

void Engine :: RegisterOnlyCondition( const gpstring& name, bool value )
{
	m_Compiler->RegisterOnlyCondition( name, value );
}

const char* Engine :: AddSkritExtension( gpstring& local, const char* source )
{
	// special: fuel addresses don't need extensions
	if ( ::IsFuelAddress( source ) )
	{
		return ( source );
	}

	return ( stringtool::AppendExtension( local, source, GetSkritExtension() ) );
}

gpstring Engine :: AddSkritExtension( gpstring name )
{
	// special: fuel addresses don't need extensions
	if ( name.find( ':' ) != name.npos )
	{
		return ( name );
	}

	stringtool::AppendExtension( name, GetSkritExtension() );
	return ( name );
}

gpstring Engine :: MakeFullSkritFileName( const char* name )
{
	gpstring localName;

	if ( !NamingKey::DoesSingletonExist() || !gNamingKey.BuildSkritLocation( name, localName ) )
	{
		localName = AddSkritExtension( name );
	}

	return ( localName );
}

const char* Engine :: GetSkritExtension( void )
{
	return ( ".skrit" );
}

HObject Engine :: CreateObject( CreateReq createReq )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED_REQUEST( createReq );

	gpassert( createReq.AssertValid() );

	// $ to add caching, add another ref to impl and then it will hang around
	//   for a while. store a frame number or something for LRU destruction.

	HObject hobject;

	// parameters?
	gpstring localFileName, localParams;
	bool oldDeferConstruction = createReq.m_DeferConstruction;
	const char* paramStart = ::strchr( createReq.m_FileName, '?' );
	if ( paramStart != NULL )
	{
		// find end - there may be w/s in the way
		const char* fileNameEnd = paramStart;
		stringtool::RemoveTrailingWhiteSpace( createReq.m_FileName, fileNameEnd );

		// take vars
		localFileName.assign( createReq.m_FileName, fileNameEnd );
		localParams.assign( paramStart + 1 );
		createReq.m_FileName = localFileName;

		// defer construction to set props
		if ( !localParams.empty() )
		{
			createReq.m_DeferConstruction = true;
		}
	}

	// resolve with naming key
	gpstring objectName;
	if ( !::IsFuelAddress( createReq.m_FileName ) )
	{
		if ( NamingKey::DoesSingletonExist() )
		{
			gpstring namingKeyName;
			if ( gNamingKey.BuildSkritLocation( createReq.m_FileName, namingKeyName ) )
			{
				objectName = createReq.m_FileName;
				localFileName = namingKeyName;
				createReq.m_FileName = localFileName;
			}
		}
		else
		{
			// auto-add ".skrit" if no extension
			createReq.m_FileName = AddSkritExtension( localFileName, createReq.m_FileName );
		}
	}

	// get the object name if not already set
	if ( objectName.empty() )
	{
		const char* ext = FileSys::GetExtension( createReq.m_FileName );
		if ( ext != NULL )
		{
			objectName.assign( createReq.m_FileName, ext - 1 );
		}
		else
		{
			objectName = createReq.m_FileName;
		}
	}

	// this part requires sync
	{
		kerneltool::RwCritical::WriteLock writeLock( m_PersistentObjects.GetCritical() );

		// look for existing one
		StringToObjectMap::iterator found = m_ObjectByNameIndex.find( objectName );
		if ( found == m_ObjectByNameIndex.end() )
		{
			// create new one
			Object* object = CreateNewObject( createReq );
			if ( object != NULL )
			{
				// check database
				bool persist = true;
				if ( createReq.m_PersistType == PERSIST_NO )
				{
					persist = false;
				}
				else if ( createReq.m_PersistType == PERSIST_DETECT )
				{
					// detect it
					const FuBi::StoreHeaderSpec* spec = object->GetImpl()->GetStoreHeaderSpec();
					if ( spec != NULL )
					{
						FuBi::StoreHeaderSpec::Record record( object->GetGlobalStore(), spec, FuBi::XMODE_QUIET );
						record.Get( "_persist", persist );
					}
				}

				// create handle
				ObjectInstance* instance = NULL;
				UINT index, magic;
				ObjectDb& db = persist ? m_PersistentObjects : m_NonPersistentObjects;
				db.Alloc( instance, index, magic );
				hobject = HObject( index, magic, persist );

				// add to index
				found = m_ObjectByNameIndex.insert( std::make_pair( objectName, ObjectShared() ) ).first;
				ObjectShared& shared = found->second;
				shared.m_Impl = object->GetImpl();

				// check
	#			if !GP_RETAIL
				if ( stdx::has_any( shared.m_Instances, hobject ) )
				{
					gperror( "*** DO NOT IGNORE THIS ERROR ***\n"
							 "\n"
							 "TELL SCOTT: something horrible happened, I now have "
							 "a duplicate instance!!! Please report this "
							 "immediately and be sure to get a call stack. This is "
							 "a very important bug!\n" );
				}
	#			endif // !GP_RETAIL

				// configure instance
				gpassert( instance != NULL );
				instance->m_Instance = object;
				instance->m_RefCount = 1;
				instance->m_NameEntry = found;
				instance->m_ListEntry = shared.m_Instances.insert( shared.m_Instances.begin(), hobject );
			}
		}
		else
		{
			const ObjectList& objects = found->second.m_Instances;
			gpassert( !objects.empty() );

			bool shouldClone = true;

			// see if we can addref on something already in there based on a match
			// on whether or not it's persistent vs. what was requested
			if ( !createReq.m_CreateAlways )
			{
				ObjectList::const_reverse_iterator i, ibegin = objects.rbegin(), iend = objects.rend();
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( IsMatch( createReq.m_PersistType, i->IsPersistent() ) )
					{
						// cool, just addref
						hobject = *i;
						AddRefObject( hobject );
						shouldClone = false;
						break;
					}
				}
			}

			// clone it
			if ( shouldClone )
			{
				hobject = CloneObject( CloneReq( objects.back(), createReq ) );
			}
		}
	}

	// update properties
	if ( hobject && !localParams.empty() )
	{
		// copy params straight in
		hobject->ReadRecord( localParams );

		// finish the construction that we temporarily delayed
		if ( !oldDeferConstruction )
		{
			hobject->CommitCreation();
		}
	}

	// safety check
	gpassert( !hobject || IsMatch( createReq.m_PersistType, hobject.IsPersistent() ) );

	// return it (may be null on failure)
	return ( hobject );
}

HObject Engine :: CloneObject( const CloneReq& cloneReq )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED_REQUEST( cloneReq );

	kerneltool::RwCritical::WriteLock writeLock( m_PersistentObjects.GetCritical() );
	gpassert( cloneReq.AssertValid() );

	// find index
	const ObjectInstance* source = GetValidObjectInstance( cloneReq.m_CloneSource );
	if ( source == NULL )
	{
		return ( HObject() );
	}

	// check database
	bool persist = true;
	if ( cloneReq.m_PersistType == PERSIST_NO )
	{
		persist = false;
	}
	else if ( cloneReq.m_PersistType == PERSIST_DETECT )
	{
		// detect it
		const FuBi::StoreHeaderSpec* spec = source->m_Instance->GetImpl()->GetStoreHeaderSpec();
		if ( spec != NULL )
		{
			FuBi::StoreHeaderSpec::Record record( source->m_Instance->GetGlobalStore(), spec, FuBi::XMODE_QUIET );
			record.Get( "_persist", persist );
		}
	}

	// get at entry
	ObjectShared& shared = source->m_NameEntry->second;
	if ( (shared.m_Impl->GetOwnerType() != cloneReq.m_OwnerType) && (cloneReq.m_OwnerType != FuBi::VAR_UNKNOWN) )
	{
		ReportOwnerTypeMismatch( cloneReq.m_CloneSource->GetName(), shared.m_Impl->GetOwnerType(), cloneReq.m_OwnerType );
		return ( HObject() );
	}

	// create object and handle, add to index
	bool oldDeferConstruction = cloneReq.m_DeferConstruction;
	Object* clone = new Object( shared.m_Impl, cloneReq.m_Owner, true );
	ObjectInstance* instance = NULL;
	UINT index, magic;
	ObjectDb& db = persist ? m_PersistentObjects : m_NonPersistentObjects;
	db.Alloc( instance, index, magic );
	HObject hclone = HObject( index, magic, persist );

	// reset source, the alloc above may have caused a realloc of the vector
	source = GetValidObjectInstance( cloneReq.m_CloneSource );

	// check
#	if !GP_RETAIL
	if ( stdx::has_any( shared.m_Instances, hclone ) )
	{
		gperror( "*** DO NOT IGNORE THIS ERROR ***\n"
				 "\n"
				 "TELL SCOTT: something horrible happened, I now have "
				 "a duplicate instance!!! Please report this "
				 "immediately and be sure to get a call stack. This is "
				 "a very important bug!\n" );
	}
#	endif // !GP_RETAIL

	// configure instance
	gpassert( instance != NULL );
	instance->m_Instance = clone;
	instance->m_RefCount = 1;
	instance->m_NameEntry = source->m_NameEntry;
	instance->m_ListEntry = shared.m_Instances.insert( shared.m_Instances.begin(), hclone );

	// optionally clone its data too
	if ( cloneReq.m_CloneGlobals && clone->HasGlobalStore() )
	{
		*clone->GetGlobalStore() = *cloneReq.m_CloneSource->GetGlobalStore();
	}

	// now commit its creation
	if ( !oldDeferConstruction )
	{
		hclone->CommitCreation();
	}

	// safety check
	gpassert( !hclone || IsMatch( cloneReq.m_PersistType, hclone.IsPersistent() ) );

	// done
	return ( hclone );
}

Object* Engine :: GetObject( HObject hobject )
{
	kerneltool::RwCritical::ReadLock readLock( m_PersistentObjects.GetCritical() );

	ObjectInstance* inst = GetObjectInstance( hobject );
	return ( inst ? inst->m_Instance : NULL );
}

bool Engine :: IsObjectValid( HObject hobject )
{
	return ( GetObjectInstance( hobject ) != NULL );
}

int Engine :: AddRefObject( HObject hobject )
{
	kerneltool::RwCritical::ReadLock readLock( m_PersistentObjects.GetCritical() );
	CHECK_DB_LOCKED_HANDLE( hobject );

	ObjectInstance* inst = GetValidObjectInstance( hobject );
	if ( inst == NULL )
	{
		return ( 0 );
	}

	return ( ++inst->m_RefCount );
}

int Engine :: ReleaseObject( HObject hobject )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED_HANDLE( hobject );

	Object* object = NULL;
	{
		kerneltool::RwCritical::WriteLock writeLock( m_PersistentObjects.GetCritical() );

		// get at instance
		ObjectInstance* inst = GetValidObjectInstance( hobject );
		if ( inst == NULL )
		{
			return ( 0 );
		}

		// ref count?
		if ( --inst->m_RefCount != 0 )
		{
			return ( inst->m_RefCount );
		}

		// last ref gone, delete it
		ObjectShared& shared = inst->m_NameEntry->second;
		shared.m_Instances.erase( inst->m_ListEntry );

		// if it's the last one and nobody is waiting on accessing this,
		// remove name entry
		if ( shared.m_Instances.empty() )
		{
			m_ObjectByNameIndex.erase( inst->m_NameEntry );
		}

		// free up the bucket
		object = inst->m_Instance;
		GetObjectDb( hobject ).Free( hobject.m_Index );
	}

	// delete the object (do this outside of the lock because it may be slow)
	delete ( object );

	// no more refs
	return ( 0 );
}

const gpstring& Engine :: GetObjectName( HObject hobject ) const
{
	kerneltool::RwCritical::ReadLock readLock( m_PersistentObjects.GetCritical() );

	const ObjectInstance* inst = GetValidObjectInstance( hobject );
	return ( inst ? inst->m_NameEntry->first : gpstring::EMPTY );
}

Object* Engine :: CreateCommand( const char* name, const char* command, int len, const CreateReq* defOptions )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED;

	gpassert( (name != NULL) && (command != NULL) );

	if ( len == -1 )
	{
		len = ::strlen( command );
	}

	Object* object = NULL;

	// see if it's cached
	if ( m_CacheOptions.m_Enabled && (len <= m_CacheOptions.m_MaxLength) )
	{
		kerneltool::Critical::Lock locker( m_CacheCritical );

		ObjectCache::const_iterator found = m_ObjectCache.find( command );
		if ( found != m_ObjectCache.end() )
		{
			// take it
			object = new Object( found->second->m_Object->GetImpl(), NULL );

			// freshen list, move to front
			m_ObjectCacheList.splice( m_ObjectCacheList.begin(), m_ObjectCacheList, found->second->m_Iter );

			// $ early bailout
			return ( object );
		}
	}

	// build and configure create request
	CreateReq createReq( name );
	createReq.m_IgnoreCompileErrors = true;
	if ( defOptions != NULL )
	{
		createReq.m_ForceFloatConstants	= defOptions->m_ForceFloatConstants;
		createReq.m_OwnerType			= defOptions->m_OwnerType;
		createReq.m_Owner				= defOptions->m_Owner;
	}

	// special: prefixing the command with '#' will bypass the wrapper
	if ( *command == '#' )
	{
		object = CreateNewObject( createReq, const_mem_ptr( command + 1, len - 1 ) );
	}
	else
	{
		gpstring source( "X${" );
		source.append( command, len );
		source.append( ";}" );

		object = CreateNewObject( createReq, const_mem_ptr( source.c_str(), source.length() ) );
	}

	// success?
	if ( object != NULL )
	{
		// optionally cache this
		if ( m_CacheOptions.m_Enabled && (len <= m_CacheOptions.m_MaxLength) )
		{
			kerneltool::Critical::Lock locker( m_CacheCritical );

			// first make sure there's room
			while ( (int)m_ObjectCache.size() >= m_CacheOptions.m_MaxEntries )
			{
				// drop the back off
				ObjectCache::iterator back = m_ObjectCacheList.back();
				m_ObjectCacheList.pop_back();

				// erase entry
				delete ( back->second );
				m_ObjectCache.erase( back );
			}
			gpassert( (int)m_ObjectCache.size() < m_CacheOptions.m_MaxEntries );

			// add new entry, and only build new one if not exist (the other
			// thread may have added new entry while we were compiling)
			std::pair <ObjectCache::iterator, bool> rc
					= m_ObjectCache.insert( std::make_pair( gpstring( command ), new CacheEntry ) );
			if ( rc.second )
			{
				CacheEntry& entry = *rc.first->second;
				entry.m_Object = new Object( object->GetImpl(), NULL, true );
				entry.m_Iter = m_ObjectCacheList.insert( m_ObjectCacheList.end(), rc.first );
			}
		}
	}
	else
	{
		// report on failure
		ReportLastCompile( createReq.m_ForceSummary );
	}

	return ( object );
}

Result Engine :: ExecuteCommand( Object** object, const char* name, const char* command, int len, const CreateReq* defOptions )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED;

	// redirect to local object if user not caching
	Object* localObject = NULL;
	if ( object == NULL )
	{
		object = &localObject;
	}

	// create command if not passed in
	if ( *object == NULL )
	{
		*object = CreateCommand( name, command, len, defOptions );
	}

	// call the command if it exists
	Result result;
	if ( *object != NULL )
	{
		result = (*object)->Call( 0 );
	}

	// delete local object if any
	delete ( localObject );

	// done
	return ( result );
}

bool Engine :: CheckCommandSyntax( const char* command, int len )
{
	std::auto_ptr <Object> object( CreateCommand( "<syntaxtest>", command, len ) );
	return ( object.get() != NULL );
}

template <typename T>
bool EvalGenericExpression( Object** object, const char* expr, T& result, bool forceFloatConstants, const CreateReq* defOptions )
{
	CHECK_DB_LOCKED;

	// special optimization: if it's all a number then no point in compiling
	// and running it, eh?
	{
		const char* check = expr;

		// skip leading w/s, but remember where it started
		while ( *check && ::isspace( *check ) )
		{
			++check;
		}
		const char* exprBegin = check;

		// skip leading +/-
		if ( (*check == '+') || (*check == '-') )
		{
			++check;
		}

		// skip numbers and periods
		while ( *check && (::isdigit( *check ) || (*check == '.')) )
		{
			++check;
		}

		// skip trailing w/s
		while ( *check && ::isspace( *check ) )
		{
			++check;
		}

		// reached the end? if so it's an easy conversion
		if ( *check == '\0' )
		{
			if ( ::FromString( exprBegin, result ) )
			{
				return ( true );
			}
		}
	}

	// default options
	CreateReq createReq;
	if ( defOptions != NULL )
	{
		createReq = *defOptions;
	}
	createReq.m_ForceFloatConstants = forceFloatConstants;

	// compile and eval
	Result rc;
	if ( (object == NULL) || (*object == NULL) )
	{
		gpassert( expr != NULL );

		const char* typeName = FuBi::Traits <T>::GetExternalVarName();
		gpassert( typeName != NULL );

		gpstring name;
		name.assignf( "%ceval", typeName[ 0 ] );
		gpstring code;
		code.assignf( "#%s x${return(\n%s\n);}", typeName, expr );

		rc = gSkritEngine.ExecuteCommand( object, name, code, code.length(), &createReq );
	}
	else
	{
		rc = gSkritEngine.ExecuteCommand( object, NULL, NULL, -1, &createReq );
	}

	if ( !rc.IsError() )
	{
		result = *rcast <T*> ( &rc.m_Raw );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

bool Engine :: EvalFloatExpression( Object** object, const char* expr, float& result, const CreateReq* defOptions )
{
	return ( EvalGenericExpression( object, expr, result, true, defOptions ) );
}

bool Engine :: EvalIntExpression( Object** object, const char* expr, int& result, const CreateReq* defOptions )
{
	return ( EvalGenericExpression( object, expr, result, false, defOptions ) );
}

bool Engine :: EvalBoolExpression ( Object** object, const char* expr, bool& result, const CreateReq* defOptions )
{
	return ( EvalGenericExpression( object, expr, result, false, defOptions ) );
}

// $$$ on recompile stuff, try to xfer globals in and out to attempt to preserve it

bool Engine :: ForceRecompileAll( HObject hobject )
{
	CHECK_DB_LOCKED_HANDLE( hobject );

	UNREFERENCED_PARAMETER( hobject );
	GP_Unimplemented$$$();
	return ( false );
}

bool Engine :: ForceRecompileInstance( HObject hobject )
{
	CHECK_DB_LOCKED_HANDLE( hobject );

	UNREFERENCED_PARAMETER( hobject );
	GP_Unimplemented$$$();
	return ( false );
}

void Engine :: ReportLastCompile( bool forceSummary )
{
	m_Compiler->ReportLastCompile( forceSummary );
}

Object* Engine :: CreateNewObject( CreateReq createReq )
{
	CHECK_DB_LOCKED_REQUEST( createReq );
	GPSTATS_SYSTEM( SP_SKRIT );

	gpassert( createReq.AssertValid() );

	Object* object = NULL;

	// auto-add ".skrit" if no extension
	gpstring localFilename;
	createReq.m_FileName = AddSkritExtension( localFilename, createReq.m_FileName );
	bool ignore = createReq.m_IgnoreCompileErrors;
	bool isFuelAddress = ::IsFuelAddress( createReq.m_FileName );
	createReq.m_IgnoreCompileErrors = true;

	// try to open the file
	FileSys::AutoFileHandle file;
	FileSys::AutoMemHandle mem;
	gpstring chunk;

#	if !GP_RETAIL
	ReportSys::HeyBatterBox tryAgainBox( gpstringf( "Skrit Engine: failed to find, load, or compile Skrit '%s'.", createReq.m_FileName ) );
#	endif // !GP_RETAIL

	// read in directly
	if ( isFuelAddress )
	{
		// extract the last part, that's the keyname
		const char* last = ::strrchr( createReq.m_FileName, ':' );
		if ( last != NULL )
		{
			gpstring addr( createReq.m_FileName, last );
			gpstring key ( last + 1 );
			FastFuelHandle fuel( addr );
			if ( fuel )
			{
				fuel.Get( key, chunk );
			}
			else
			{
				::SetLastError( ERROR_PATH_NOT_FOUND );
			}
		}
		else
		{
			// should be impossible to get here
			gpassert( 0 );
		}
	}
	else
	{
#		if !GP_RETAIL

	tryAgain:

		if ( TestOptions( OPTION_RETRY_LOAD ) )
		{
			while ( !file.Open( createReq.m_FileName ) || !mem.Map( file ) )
			{
				gperrorf(( "Skrit engine: unable to open file '%s': '%s'\n", createReq.m_FileName, stringtool::GetLastErrorText().c_str() ));
				if ( !tryAgainBox.TryAgain() )
				{
					createReq.m_IgnoreFileErrors = true;
					break;
				}
			}
		}
		else
#		endif // !GP_RETAIL
		{
			if ( file.Open( createReq.m_FileName ) )
			{
				mem.Map( file );
			}
		}
	}

	// opened ok?
	if ( mem.IsMapped() || !chunk.empty() )
	{
		// get ptrs
		const_mem_ptr memPtr
				= isFuelAddress
				? const_mem_ptr( chunk.c_str(), chunk.length() )
				: const_mem_ptr( mem.GetData(), mem.GetSize() );

		// try to compile it
#		if !GP_RETAIL
		if ( TestOptions( OPTION_RETRY_COMPILE ) && !isFuelAddress && Raw::Header::ShouldCompile( memPtr, createReq.m_OwnerType ) )
		{
			if ( (object = CreateNewObject( createReq, memPtr )) == NULL )
			{
				mem.Close();
				file.Close();

				if ( !ignore )
				{
					ReportLastCompile( createReq.m_ForceSummary );
				}

				if ( tryAgainBox.TryAgain() )
				{
					goto tryAgain;
				}
			}
			else if ( !ignore )
			{
				mem.Close();
				file.Close();

				ReportLastCompile( createReq.m_ForceSummary );
			}
		}
		else
#		endif // !GP_RETAIL
		{
			object = CreateNewObject( createReq, memPtr );
			if ( !ignore )
			{
				mem.Close();
				file.Close();
				chunk.clear();

				ReportLastCompile( createReq.m_ForceSummary );
			}
		}
	}
	else if ( !createReq.m_IgnoreFileErrors )
	{
		gperrorf(( "Skrit engine: unable to open file '%s': '%s'\n", createReq.m_FileName, stringtool::GetLastErrorText().c_str() ));
	}

	return ( object );
}

Object* Engine :: CreateNewObject( const CreateReq& createReq, const_mem_ptr mem )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED_REQUEST( createReq );

	gpassert( createReq.AssertValid() );

	Object* object = NULL;
	std::auto_ptr <ObjectImpl> impl( new ObjectImpl( createReq.m_FileName ) );

	if ( Raw::Header::ShouldCompile( mem, createReq.m_OwnerType ) )
	{
		kerneltool::Critical::Lock locker( m_CompileCritical );

		if ( m_Compiler->Compile( createReq.m_FileName, mem, &createReq ) )
		{
			eVarType skritOwnerType = m_Compiler->GetOwnerType();
			if ( (skritOwnerType == createReq.m_OwnerType) || (createReq.m_OwnerType == FuBi::VAR_UNKNOWN) )
			{
				m_Compiler->MoveTo( *impl );
				object = new Object( impl.release(), createReq.m_Owner, createReq.m_DeferConstruction );
			}
			else
			{
				ReportOwnerTypeMismatch( createReq.m_FileName, skritOwnerType, createReq.m_OwnerType );
			}
		}

#		if !GP_ERROR_FREE
		if ( !createReq.m_IgnoreCompileErrors )
		{
			ReportLastCompile( createReq.m_ForceSummary );
		}
#		endif // !GP_ERROR_FREE
	}
	else
	{
		impl->Copy( rcast <const Raw::Header*> ( mem.mem ) );
		gpassert( impl->GetOwnerType() == createReq.m_OwnerType );
		object = new Object( impl.release(), createReq.m_Owner, createReq.m_DeferConstruction );
	}

	return ( object );
}

Machine* Engine :: AcquireMachine( Object* object, int& index )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED;

	kerneltool::Critical::Lock locker( m_MachineCritical );

	gpassert( object != NULL );
	gpassert( object->IsConstructed() );

	Machine* machine = NULL;

	if ( m_FreeMachineIndexes.empty() )
	{
		machine = new Machine( object );

		if ( m_FreeSlotIndexes.empty() )
		{
			index = m_MachineCache.size();
			m_MachineCache.push_back( machine );
		}
		else
		{
			index = m_FreeSlotIndexes.pop_back_t();
			m_MachineCache[ index ] = machine;
		}
	}
	else
	{
		index = m_FreeMachineIndexes.pop_back_t();
		machine = m_MachineCache[ index ];
		machine->Init( object );
	}

#	if !GP_RETAIL
	if ( m_MachineCache.size() >= 50 )
	{
		gpstring names;
		MachineColl::const_iterator i, ibegin = m_MachineCache.begin(), iend = m_MachineCache.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i != NULL) && (*i)->IsInitialized() )
			{
				names += (*i)->MakeCurrentLocationText();
				names += "\n";
			}
		}

		gperrorf(( "TELL ERIC: I'm sure using a lot of Skrit Virtual Machines! "
				   "Possible infinite Skrit recursion!\n"
				   "\n"
				   "Current VM owners:\n"
				   "\n"
				   "%s\n", names.c_str() ));
	}
#	endif // !GP_RETAIL

	return ( machine );
}

void Engine :: ReleaseMachine( int index )
{
	GPSTATS_SYSTEM( SP_SKRIT );
	CHECK_DB_LOCKED;

	kerneltool::Critical::Lock locker( m_MachineCritical );

	// if cache is full, then delete it and add the free slot
	if ( m_FreeMachineIndexes.size() == MAX_MACHINE_CACHE )
	{
		Delete( m_MachineCache[ index ] );
		if ( (size_t)index == (m_MachineCache.size() - 1) )
		{
			// it was the last element in the cache, just pop it off
			m_MachineCache.pop_back();
		}
		else
		{
			m_FreeSlotIndexes.push_back( index );
		}
	}
	else
	{
		// ok then free the machine itself but don't delete it
		m_MachineCache[ index ]->Shutdown();
		m_FreeMachineIndexes.push_back( index );
	}
}

void Engine :: ReportOwnerTypeMismatch( const char* skritName, eVarType skritType, eVarType reqType )
{
	const char* skritOwnerName = gFuBiSysExports.FindSkritObject()->m_Name;
	const char* ownerName      = skritOwnerName;

	if ( skritType != FuBi::VAR_UNKNOWN )
	{
		skritOwnerName = gFuBiSysExports.FindTypeName( skritType );
	}

	ownerName = gFuBiSysExports.FindTypeName( reqType );

	gperrorf(( "Skrit engine: compiled Skrit '%s' has owner type of '%s', does not match C++ requested owner type of '%s'\n",
			   skritName, skritOwnerName, ownerName ));
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
