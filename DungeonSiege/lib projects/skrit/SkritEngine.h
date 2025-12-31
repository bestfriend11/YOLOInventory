//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritEngine.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the main engine that runs Skrits, manages their object
//             files, and routes queries and calls.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITENGINE_H
#define __SKRITENGINE_H

//////////////////////////////////////////////////////////////////////////////

#include "BucketVector.h"
#include "FuBiDefs.h"
#include "SkritObject.h"

#include <list>
#include <map>

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class Engine declaration

class Engine : public Singleton <Engine>
{
public:
	SET_NO_INHERITED( Engine );

// Types.

	enum eOptions
	{
		#if !GP_RETAIL
		OPTION_RETRY_LOAD    = 1 << 0,		// retry (prompt the user) on failed find/load
		OPTION_RETRY_COMPILE = 1 << 1,		// retry (prompt the user) on failed compile
		OPTION_RETAIL_MODE   = 1 << 2,		// pretend we're in retail mode
		#endif // !GP_RETAIL

		#if !GP_ERROR_FREE
		OPTION_DEFER_ERRORS  = 1 << 3,		// defer errors so a dialog can pull them all together
		#endif // !GP_ERROR_FREE

		// ...

		OPTION_NONE = 0,
	};

	struct CacheOptions
	{
		bool m_Enabled;					// true if enabled
		int  m_MaxLength;				// max length of string to cache
		int  m_MaxEntries;				// max entries to include in cache table

		CacheOptions( void )
		{
			m_Enabled    = true;
			m_MaxLength  = 100;
			m_MaxEntries = 100;
		}
	};

// Setup.

	// ctor/dtor
	Engine( void );
   ~Engine( void );

	// local options
	void SetOptions   ( eOptions options, bool set = true );
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	// cache options
	void SetCacheOptions( const CacheOptions& options )		{  m_CacheOptions = options;  ResetCache();  }
	const CacheOptions& GetCacheOptions( void ) const		{  return ( m_CacheOptions );  }
	void ResetCache( void );

	// persistence
	void SetLockedForSaveLoad( bool set = true )			{  gpassert( m_LockedForSaveLoad != set );  m_LockedForSaveLoad = set;  }
	bool IsLockedForSaveLoad ( void ) const					{  return ( m_LockedForSaveLoad );  }
	bool Xfer( FuBi::PersistContext& persist );				// call to xfer the skrit engine
	bool CommitLoad( void );								// call this to commit xfers - call this after owners are set up

	// registration of 'only' directive conditions
	void RegisterOnlyCondition( const gpstring& name, bool value );

	// extension mucking
	static const char* AddSkritExtension    ( gpstring& local, const char* source );
	static gpstring    AddSkritExtension    ( gpstring name );
	static gpstring    MakeFullSkritFileName( const char* name );
	static const char* GetSkritExtension    ( void );

// Object database API.

	// $ skrit objects represent code and data for an individual skrit file.
	//   global variables and the compiled code are shared across instances.

	HObject         CreateObject ( CreateReq createReq );		// create a skrit given the spec
	HObject         CloneObject  ( const CloneReq& cloneReq );	// clone an existing skrit
	Object*         GetObject    ( HObject hobject );			// resolve a handle into a pointer
	bool            IsObjectValid( HObject hobject );			// pointing to a valid object?
	int             AddRefObject ( HObject hobject );			// add a new reference to a skrit
	int             ReleaseObject( HObject hobject );			// release a reference on a skrit, deleting if no more
	const gpstring& GetObjectName( HObject hobject ) const;		// resolve name of the object

// Command/evaluation API.

	Object* CreateCommand     ( const char* name, const char* command, int len = -1, const CreateReq* defOptions = NULL );
	Result  ExecuteCommand    ( const char* name, const char* command, int len = -1, const CreateReq* defOptions = NULL )	{  return ( ExecuteCommand( NULL, name, command, len, defOptions ) );  }
	Result  ExecuteCommand    ( Object** object, const char* name, const char* command, int len = -1, const CreateReq* defOptions = NULL );
	bool    CheckCommandSyntax( const char* command, int len = -1 );

	bool EvalFloatExpression( const char* expr, float& result, const CreateReq* defOptions = NULL )						{  return ( EvalFloatExpression( NULL, expr, result, defOptions ) );  }
	bool EvalFloatExpression( Object** object, const char* expr, float& result, const CreateReq* defOptions = NULL );
	bool EvalIntExpression  ( const char* expr, int& result, const CreateReq* defOptions = NULL )						{  return ( EvalIntExpression( NULL, expr, result, defOptions ) );  }
	bool EvalIntExpression  ( Object** object, const char* expr, int  & result, const CreateReq* defOptions = NULL );
	bool EvalBoolExpression ( const char* expr, bool& result, const CreateReq* defOptions = NULL )						{  return ( EvalBoolExpression( NULL, expr, result, defOptions ) );  }
	bool EvalBoolExpression ( Object** object, const char* expr, bool & result, const CreateReq* defOptions = NULL );

// Misc.

	// force a recompile underneath your feet
	bool ForceRecompileAll     ( HObject hobject );			// all instances (recomp the impl)
	bool ForceRecompileInstance( HObject hobject );			// just this instance (build new impl, detach from name db - careful)

	// print out results of the last compile
	void ReportLastCompile( bool forceSummary = false );

	// these do not use the object database
	Object* CreateNewObject( CreateReq createReq );
	Object* CreateNewObject( const CreateReq& createReq, const_mem_ptr mem );

	// $ special - only call these if you know what you're doing!
	Compiler& GetCompiler   ( void )						{  return ( *m_Compiler );  }
	Machine*  AcquireMachine( Object* object, int& index );	// get a machine from the cache or a new one
	void      ReleaseMachine( int index );					// release a machine to be cached or dumped

private:
	void ReportOwnerTypeMismatch( const char* skritName, eVarType skritType, eVarType reqType );

	typedef std::list <HObject> ObjectList;

	struct ObjectShared
	{
		// purpose of m_Instances below is to remember the HObjects that
		// reference an ObjectImpl. this is needed because GetObject() won't
		// necessarily clone a new object, but may instead want to do an
		// AddRef() on an existing Object, and for that we just use the Object
		// still remaining that was created first.

		ObjectImpl* m_Impl;							// shared impl
		ObjectList  m_Instances;					// referencing instances

		ObjectShared( void )
		{
			m_Impl = NULL;
		}
	};

	typedef std::map <gpstring, ObjectShared, istring_less> StringToObjectMap;

	struct ObjectInstance
	{
		Object*                     m_Instance;		// instance of the object
		int                         m_RefCount;		// ref count on this object
		StringToObjectMap::iterator m_NameEntry;	// where in the index our name is stored
		ObjectList::iterator        m_ListEntry;	// where in the index our instance is stored

		ObjectInstance( void )
		{
			m_Instance = NULL;
			m_RefCount = 0;
		}
	};

	typedef BucketVector <ObjectInstance> ObjectDb;
	typedef stdx::fast_vector <my Machine*> MachineColl;
	typedef stdx::fast_vector <int> IntColl;
	typedef stdx::fast_vector <int> IntColl;

	struct CacheEntry;
	typedef std::map <gpstring, my CacheEntry*, string_less> ObjectCache;
	typedef std::list <ObjectCache::iterator> ObjectCacheList;

	struct CacheEntry
	{
		my Object* m_Object;
		ObjectCacheList::iterator m_Iter;

		CacheEntry( void )							{  m_Object = NULL;  }
	   ~CacheEntry( void )							{  delete ( m_Object );  }
	};

	enum  {  MAX_MACHINE_CACHE = 10  };				// only keep this many extra (unused) machines in the cache

	ObjectDb&             GetObjectDb           ( HObject hobject )			{  return ( hobject.m_Persist ? m_PersistentObjects : m_NonPersistentObjects );  }
	const ObjectDb&       GetObjectDb           ( HObject hobject ) const	{  return ( ccast <ThisType*> ( this )->GetObjectDb( hobject ) );  }
	ObjectInstance*       GetObjectInstance     ( HObject hobject )			{  return ( GetObjectDb( hobject ).Dereference( hobject.m_Index, hobject.m_Magic ) );  }
	const ObjectInstance* GetObjectInstance     ( HObject hobject ) const	{  return ( ccast <ThisType*> ( this )->GetObjectInstance( hobject ) );  }
	ObjectInstance*       GetValidObjectInstance( HObject hobject )			{  ObjectInstance* inst = GetObjectInstance( hobject );  gpassert( inst != NULL );  return ( inst );  }
	const ObjectInstance* GetValidObjectInstance( HObject hobject ) const	{  const ObjectInstance* inst = GetObjectInstance( hobject );  gpassert( inst != NULL );  return ( inst );  }

	eOptions             m_Options;					// global engine options
	CacheOptions         m_CacheOptions;			// global cache options
	bool                 m_LockedForSaveLoad;		// special: skrit engine is locked for save/load - no creation requests allowed
	ObjectDb             m_PersistentObjects;		// these objects will be xfer'd
	ObjectDb             m_NonPersistentObjects;	// these objects are independent from persistent ones and will not be xfer'd
	StringToObjectMap    m_ObjectByNameIndex;		// index by name into the object collections
	ObjectCache          m_ObjectCache;				// cached expressions and commands
	ObjectCacheList      m_ObjectCacheList;			// most recently cached items at front, used to nuke old stuff
	kerneltool::Critical m_CacheCritical;			// lock cache data here
	Compiler*            m_Compiler;				// cached compiler for skrits
	kerneltool::Critical m_CompileCritical;			// lock compiler here
	MachineColl          m_MachineCache;			// cache of virtual machines to be used by skrit objects
	kerneltool::Critical m_MachineCritical;			// lock critical alloc/free here
	IntColl              m_FreeMachineIndexes;		// list of indexes to available machines
	IntColl              m_FreeSlotIndexes;			// list of indexes to empty slots for machines

	SET_NO_COPYING( Engine );
};

MAKE_ENUM_BIT_OPERATORS( Engine::eOptions );

#define gSkritEngine Skrit::Engine::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

#endif  // __SKRITENGINE_H

//////////////////////////////////////////////////////////////////////////////
