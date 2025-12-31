//////////////////////////////////////////////////////////////////////////////
//
// File     :  MasterFileMgr.h (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Summary  :  This contains the master file class. All file access goes
//             through this thing.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __MASTERFILEMGR_H
#define __MASTERFILEMGR_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FileSys.h"
#include "FileSysHandleMgr.h"
#include "KernelTool.h"

#include <vector>
#include <set>

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr declaration

// note regarding the filemgr index - 0 is always the MasterFileMgr. it is
//  not included in the OrderColl but is always the first entry in the
//  FileMgrColl. the MasterFileMgr is only used as the owner when iterating
//  using FindHandles.

class MasterFileMgr : public IFileMgr
{
public:
	SET_INHERITED( MasterFileMgr, IFileMgr );

// Types

	enum eOptions						// options to configure a MasterFileMgr
	{
		// one of
#		if !GP_RETAIL
		OPTION_LOG_OPENS = 1 << 0,			// for each Open() call, log it to a file
#		endif // !GP_RETAIL
		OPTION_FORCE_MFT = 1 << 1,			// force usage of MFT even if a single file manager

		// OR one of
		OPTION_NONE = 0,					// disable all options
		OPTION_ALL  = 0xFFFFFFFF,			// enable all options
	};

// MasterFileMgr-specific functions.

	// ctor/dtor
	         MasterFileMgr( eOptions options = OPTION_NONE );
	virtual ~MasterFileMgr( void );

	// filemgr options
	void 			 SetOptions   ( eOptions options, bool set = true )			{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void 			 ClearOptions ( eOptions options )							{  SetOptions( options, false );  }
	void			 ToggleOptions( eOptions options )							{  m_Options = (eOptions)(m_Options ^ options);  }
	bool 			 TestOptions  ( eOptions options ) const					{  return ( (m_Options & options) != 0 );  }

	// owned filemgr set
	void             AddFileMgr    ( IFileMgr* fileMgr,							// adds the given filemgr (this may invalidate the index if not a new driver)
								     bool owned = false, int priority = 0,
								     const char* driverName = NULL );
	void             AddDriver     ( IFileMgr* fileMgr,							// adds the given filemgr as a driver (this won't invalidate the index)
								     bool owned, const char* driverName )		{  AddFileMgr( fileMgr, owned, 0, driverName );  }
	bool             RemoveFileMgr ( IFileMgr* fileMgr );						// removes it from the list (this may invalidate the index)
	bool             EnableFileMgr ( IFileMgr* fileMgr, bool enable = true );	// includes/excludes a filemgr from mft and drivers, returns false on failure to enable
	bool             DisableFileMgr( IFileMgr* fileMgr )						{  return ( EnableFileMgr( fileMgr, false ) );  }

	// filemgr query
	inline IFileMgr* GetFileMgrMasterOkay( Handle handle );						// find out what filemgr a handle is pointing to (ok to point to MasterFileMgr)
	inline IFileMgr* GetFileMgr( Handle handle );								// find out what filemgr a handle is pointing to (not ok to point to MasterFileMgr)

	// master index
	inline void SetIndexDirty     ( void );										// rebuild master index next time it's accessed
	inline void CheckIndexDirty   ( void );										// if index is dirty rebuild it
	void        RebuildMasterIndex( void );										// force a rebuild right now

	// other query
	bool HasUsedHandles( void ) const;											// anything open?
	bool HasDriver     ( const char* driver ) const;							// do we have a particular driver installed?

#	if 0 // $$$ !GP_RETAIL
	void				Dump( int type ) const;
#	endif // !GP_RETAIL

// File i/o function overrides. See docs in FileSys.h on these.

	virtual FileHandle	Open        ( const char* name, eUsage usage = USAGE_DEFAULT );
	virtual FileHandle	Open        ( IndexPtr where, eUsage usage = USAGE_DEFAULT );
	virtual bool		Close       ( FileHandle file );

	virtual bool		Read        ( FileHandle file, void* out, int size, int* bytesRead = NULL );
	virtual bool		ReadLine    ( FileHandle file, gpstring& out );

	virtual bool		SetReadWrite( FileHandle file, bool set = true );

	virtual bool		Seek        ( FileHandle file, int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int			GetPos      ( FileHandle file );

	virtual int			GetSize     ( FileHandle file );
	virtual eLocation	GetLocation ( FileHandle file );
	virtual bool		GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite );

// Memory mapped i/o function overrides.

	virtual MemHandle	Map    ( FileHandle file, int offset = 0, int size = MAP_REST_OF_DATA );
	virtual bool		Close  ( MemHandle mem );

	virtual int			GetSize( MemHandle mem );
	virtual bool		Touch  ( MemHandle mem, int offset = 0, int size = TOUCH_REST_OF_DATA );
	virtual const void* GetData( MemHandle mem, int offset = 0 );

// File/directory iteration function overrides.

	virtual FindHandle	Find    ( const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual FindHandle	Find    ( FindHandle base, const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual bool		GetNext ( FindHandle find, gpstring& out, bool prependPath = false );
	virtual bool		GetNext ( FindHandle find, FindData& out, bool prependPath = false );
	virtual bool		HasNext ( FindHandle find );
	virtual bool		Close   ( FindHandle find );

// Internal FileSys overrides.

	virtual bool		QueryFileInfo( IndexPtr where, FindData& out );

// Other.

	static inline ThisType* GetMasterFileMgr( void )  {  return ( (ThisType*)Inherited::GetMasterFileMgr() );  }

private:

// Private functions.

	// types
	struct IndexEntry;
	struct IndexEntryPtr;
	struct IndexDirEntry;
	struct IndexFileEntry;
	struct FileMgrEntry;
	struct FindEntry;
	typedef kerneltool::RwCritical RwCritical;

	// collections
	typedef std::set          <IndexEntryPtr> IndexColl;
	typedef stdx::fast_vector <FileMgrEntry>  FileMgrColl;
	typedef stdx::fast_vector <UINT>          OrderColl;
	typedef HandleMgr         <FindEntry>     FindHandleMgr;

	// collection lookup functions
	inline FileMgrEntry*       GetFileMgrEntry          ( Handle handle );
	inline const FileMgrEntry* GetFileMgrEntry          ( Handle handle ) const  {  return ( ccast <ThisType*> ( this )->GetFileMgrEntry( handle ) );  }
	inline FileMgrEntry*       GetFileMgrEntryMasterOkay( Handle handle );
	inline FindEntry*          GetFindEntry             ( FindHandle find );
	inline IndexEntryPtr       GetIndexEntryPtr         ( IndexPtr where );

	// reference counting
	inline void AddRef   ( FileMgrEntry& entry );
	inline void RemoveRef( FileMgrEntry& entry );

	// query
	inline UINT GetNormalFileMgrCount  ( void ) const;
	inline UINT GetDriverFileMgrCount  ( void ) const;
	inline bool IsIndexRequired        ( void ) const;
	inline bool IsSingleFileMgr        ( void ) const;
	inline UINT GetSingleFileMgrIndex  ( void ) const;

	// random helpers
	FileHandle    OpenHelp    ( IndexEntryPtr ptr, eUsage usage );
	FileHandle    OpenDirect  ( UINT fileMgrIndex, const char* name, IndexPtr ptr, eUsage usage );
	IndexEntryPtr FindIndex   ( IndexDirEntry* base, const char* name, const char*& lastComponent );
	FindHandle    FindHelp    ( IndexEntryPtr dirPtr, const char* pattern, eFindFilter filter );			// helper that does pattern matching
	FindHandle    FindDirect  ( UINT fileMgrIndex, const char* pattern, eFindFilter filter );			// directly tell a filemgr peon to do it
	bool          QueryDirect ( IndexEntryPtr queryPtr, FindData& out, bool prependPath );
	void          AddToIndex  ( IFileMgr* mgr, UINT mgrIndex, FindHandle parentFind, IndexDirEntry* currentDir );

// Private index types.

	struct IndexEntryPtr
	{
		// to save memory, this uses the least significant bit to tell
		//  whether or not this is pointing to a directory structure. the
		//  structure itself must be word aligned for this to work. this is
		//  checked through assertions.

		void* m_Data;

		inline                     IndexEntryPtr ( void );

		inline void                Delete        ( void );

		inline void                SetNull       ( void );
		inline void                SetDir        ( IndexDirEntry*  entry );
		inline void                SetFile       ( IndexFileEntry* entry );

		inline bool                IsNull        ( void ) const;
		inline bool                IsDir         ( void ) const;
		inline bool                IsFile        ( void ) const;

		inline IndexEntry*         GetEntry      ( void ) const  {  return ( (IndexEntry*)((DWORD)m_Data & ~1) );  }
		inline IndexEntry*         GetValidEntry ( void ) const  {  gpassert( !IsNull() );  return ( GetEntry() );  }
		inline IndexDirEntry*      GetDir        ( void ) const  {  gpassert( IsDir () );   return ( rcast <IndexDirEntry* > ( GetEntry() ) );  }
		inline IndexDirEntry*      GetValidDir   ( void ) const  {  gpassert( IsDir () );   return ( rcast <IndexDirEntry* > ( GetValidEntry() ) );  }
		inline IndexFileEntry*     GetFile       ( void ) const  {  gpassert( IsFile() );   return ( rcast <IndexFileEntry*> ( GetEntry() ) );  }
		inline IndexFileEntry*     GetValidFile  ( void ) const  {  gpassert( IsFile() );   return ( rcast <IndexFileEntry*> ( GetValidEntry() ) );  }

		inline const gpdumbstring& GetName       ( void ) const;

		inline bool                operator <    ( const IndexEntryPtr& other ) const;
	};

	COMPILER_ASSERT( sizeof( IndexEntryPtr ) == sizeof( IndexPtr ) );	// required by FileSys

	typedef IndexColl::iterator         IndexIter;
	typedef IndexColl::const_iterator   IndexCiter;
	typedef std::pair <IndexIter, bool> IndexInsertRC;

	struct FakeIndexEntryPtr : IndexEntryPtr	// use only for searches and string compares
	{
		gpdumbstring m_Name;

		inline FakeIndexEntryPtr( void );
		inline FakeIndexEntryPtr( const gpdumbstring& name );
	};

	struct AutoIndexEntryPtr : IndexEntryPtr	// use for temporary ownership
	{
	   ~AutoIndexEntryPtr( void )  {  Delete();  }

		void Release( void )  {  m_Data = NULL;  }
	};

	struct IndexEntry
	{
		gpdumbstring   m_Name;			// my name
		IndexDirEntry* m_Parent;		// NULL if root
		IndexPtr       m_IndexPtr;		// direct access to filemgr entry (may be NULL)
		UINT           m_OwnerIndex;	// my filemgr index in m_FileMgrColl

		inline void Clear ( void );

		void BuildName( gpstring& name, bool prependPath ) const;
	};

	struct IndexDirEntry : IndexEntry
	{
		IndexColl m_Entries;			// dir and file children

		void Delete( void );

		IndexEntryPtr Find( const IndexEntryPtr& name )		// $ i can't figure out how to put this out-of-line without compiler complaining
		{
			const gpdumbstring& nameStr = name.GetName();
			if ( nameStr.at( 0 ) == '.' )
			{
				if ( nameStr.at( 1 ) == '\0' )
				{
					IndexEntryPtr ptr;
					ptr.SetDir( this );
					return ( ptr );
				}
				else if ( (nameStr.at( 1 ) == '.') && (nameStr.at( 2 ) == '\0') )
				{
					IndexEntryPtr ptr;
					if ( m_Parent != NULL )
					{
						ptr.SetDir( m_Parent );
					}
					else
					{
						ptr.SetDir( this );
					}
					return ( ptr );
				}
			}

			IndexCiter found = m_Entries.find( name );
			return ( (found == m_Entries.end()) ? IndexEntryPtr() : *found );
		}
	};

	struct IndexFileEntry : IndexEntry
	{
	};

// Private state types.

	struct FileMgrEntry
	{
		// note: m_FileMgr will be NULL if this entry is unused

		IFileMgr*    m_FileMgr;				// pointer to (possibly owned) filemgr
		bool         m_Owned;				// do we own it?
		int          m_Priority;			// what priority is it?
		gpstring     m_DriverName;			// name of this driver if any
		UINT         m_RefCount;			// how many outstanding handles on it?

		inline      FileMgrEntry( IFileMgr* mgr, bool owned, int priority, const char* driverName );
		inline bool operator == ( const IFileMgr* mgr ) const;

		bool IsDriver( void ) const			{  return ( !m_DriverName.empty() );  }
		void Reset   ( void )				{  *this = FileMgrEntry( NULL, false, 0, NULL );  }
	};

	struct FindEntry
	{
		enum eMode
		{
			MODE_EMPTY,
			MODE_FIRST,
			MODE_NORMAL,
		};

		IndexDirEntry* m_CurrentDir;		// what dir working on?
		IndexCiter     m_CurrentEntry;		// which child?
		eMode          m_Mode;				// search mode
		gpstring       m_Pattern;			// Find( pattern, ... ) - will be empty if "find all"
		eFindFilter    m_Filter;			// Find( pattern, ... )

		void First  ( void );
		bool Next   ( gpstring* filename, bool prependPath );
		bool IsMatch( void ) const;
	};

// Private data.

	eOptions       m_Options;				// options for this object
	FileMgrColl    m_FileMgrColl;			// set of available file managers (in no particular order, note that "this" is always entry 0)
	OrderColl      m_NormalOrderColl;		// how the filemgrs are ordered (in priority order)
	OrderColl      m_DriverOrderColl;		// how the driver filemgrs are ordered (in priority order)
	int            m_NormalDisabledCount;	// how many filemgrs are disabled
	int            m_DriverDisabledCount;	// how many driver filemgrs are disabled
	FindHandleMgr  m_FindHandleMgr;			// collection of FindHandle entry data
	IndexDirEntry  m_IndexRoot;				// root directory of index
	bool           m_IndexDirty;			// index needs rebuilding
	RwCritical     m_RwCritical;			// serializer

	SET_NO_COPYING( MasterFileMgr );
};

MAKE_ENUM_BIT_OPERATORS( MasterFileMgr::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr inline implementation

inline IFileMgr* MasterFileMgr :: GetFileMgrMasterOkay( Handle handle )
{
	FileMgrEntry* entry = GetFileMgrEntryMasterOkay( handle );
	return ( entry ? entry->m_FileMgr : NULL );
}

inline IFileMgr* MasterFileMgr :: GetFileMgr( Handle handle )
{
	FileMgrEntry* entry = GetFileMgrEntry( handle );
	return ( entry ? entry->m_FileMgr : NULL );
}

inline void MasterFileMgr :: SetIndexDirty( void )
{
	m_IndexDirty = true;
}

inline void MasterFileMgr :: CheckIndexDirty( void )
{
	if ( m_IndexDirty )
	{
		RebuildMasterIndex();
	}
}

inline bool MasterFileMgr :: HasUsedHandles( void ) const
{
	return ( m_FileMgrColl.begin()->m_RefCount > 0 );
}

inline MasterFileMgr::FileMgrEntry* MasterFileMgr :: GetFileMgrEntryMasterOkay( Handle handle )
{
	gpassertm( handle.AssertValid(), "Attempt to dereference an invalid MasterFileMgr handle" );
	if ( handle.IsNull() )
	{
		return ( NULL );
	}

	UINT index = handle.GetOwnerIndex();
	FileMgrEntry* entry = NULL;

	if (   ( index >= m_FileMgrColl.size() )
		|| ( ( entry = &m_FileMgrColl[ index ] )->m_FileMgr == NULL ) )
	{
		entry = NULL;
		::SetLastError( ERROR_INVALID_HANDLE );
		gpassertm( 0, "Attempt to dereference an invalid FileSys handle" );
	}

	return ( entry );
}

inline MasterFileMgr::FileMgrEntry* MasterFileMgr :: GetFileMgrEntry( Handle handle )
{
	FileMgrEntry* entry = GetFileMgrEntryMasterOkay( handle );
	if ( handle.IsNull() )
	{
		return ( NULL );
	}

	if ( entry->m_FileMgr == this )
	{
		entry = NULL;
		::SetLastError( ERROR_INVALID_HANDLE );
		gpassertm( 0, "Potentially recursive code detected!" );
	}

	return ( entry );
}

inline MasterFileMgr::FindEntry* MasterFileMgr :: GetFindEntry( FindHandle find )
{
	gpassert( find.GetOwnerIndex() == 0 );		// 0 is "this"
	return ( m_FindHandleMgr.Dereference( find ) );
}

inline MasterFileMgr::IndexEntryPtr MasterFileMgr :: GetIndexEntryPtr( IndexPtr where )
{
	IndexEntryPtr entryPtr;
	where.GetPointer( entryPtr );

	if ( entryPtr.IsNull() )
	{
		::SetLastError( ERROR_INVALID_HANDLE );
		gpassertm( 0, "Attempt to dereference an invalid MasterFileMgr index pointer" );
	}

	return ( entryPtr );
}

inline void MasterFileMgr :: AddRef( FileMgrEntry& entry )
{
	RwCritical::WriteLock writeLock( m_RwCritical );
	++entry.m_RefCount;						// inc local ref count
	++m_FileMgrColl.begin()->m_RefCount;	// inc global ref count
}

inline void MasterFileMgr :: RemoveRef( FileMgrEntry& entry )
{
	RwCritical::WriteLock writeLock( m_RwCritical );
	--entry.m_RefCount;						// dec local ref count
	--m_FileMgrColl.begin()->m_RefCount;	// dec global ref count
}

inline UINT MasterFileMgr :: GetNormalFileMgrCount( void ) const
{
	return ( m_NormalOrderColl.size() - m_NormalDisabledCount );
}

inline UINT MasterFileMgr :: GetDriverFileMgrCount( void ) const
{
	return ( m_DriverOrderColl.size() - m_DriverDisabledCount );
}

inline bool MasterFileMgr :: IsIndexRequired( void ) const
{
	return ( (GetNormalFileMgrCount() > 1) || TestOptions( OPTION_FORCE_MFT ) );
}

inline bool MasterFileMgr :: IsSingleFileMgr( void ) const
{
	return ( GetNormalFileMgrCount() == 1 );
}

inline UINT MasterFileMgr :: GetSingleFileMgrIndex( void ) const
{
	if ( !IsIndexRequired() )
	{
		for ( OrderColl::const_iterator i = m_NormalOrderColl.begin() ; i != m_NormalOrderColl.end() ; ++i )
		{
			const FileMgrEntry* entry = GetFileMgrEntry( Handle( *i, 0 ) );
			if ( (entry != NULL) && entry->m_FileMgr->IsEnabled() )
			{
				return ( *i );
			}
		}
	}

	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::IndexEntryPtr inline implementation

inline MasterFileMgr::IndexEntryPtr :: IndexEntryPtr( void )
{
	m_Data = NULL;
}

inline void MasterFileMgr::IndexEntryPtr :: Delete( void )
{
	if ( IsDir() )
	{
		GetDir()->Delete();
		delete( GetDir() );
	}
	else
	{
		delete( GetFile() );
	}
	m_Data = NULL;
}

inline void MasterFileMgr::IndexEntryPtr :: SetDir( IndexDirEntry* entry )
{
	gpassert( IsWordAligned( (DWORD)entry ) );
	m_Data = (BYTE*)entry + 1;
}

inline void MasterFileMgr::IndexEntryPtr :: SetFile( IndexFileEntry* entry )
{
	gpassert( IsWordAligned( (DWORD)entry ) );
	m_Data = entry;
}

inline bool MasterFileMgr::IndexEntryPtr :: IsNull( void ) const
{
	return ( !((DWORD)m_Data & ~1) );
}

inline bool MasterFileMgr::IndexEntryPtr :: IsDir( void ) const
{
	return ( !!((DWORD)m_Data & 1) );
}

inline bool MasterFileMgr::IndexEntryPtr :: IsFile( void ) const
{
	return ( !((DWORD)m_Data & 1) );
}

inline const gpdumbstring& MasterFileMgr::IndexEntryPtr :: GetName( void ) const
{
	return ( GetValidEntry()->m_Name );
}

inline bool MasterFileMgr::IndexEntryPtr :: operator < ( const IndexEntryPtr& other ) const
{
	return ( GetName().compare_with_case( other.GetName() ) < 0 );
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::FakeIndexEntryPtr inline implementation

inline MasterFileMgr::FakeIndexEntryPtr :: FakeIndexEntryPtr( void )
{
	m_Data = &m_Name;
}

inline MasterFileMgr::FakeIndexEntryPtr :: FakeIndexEntryPtr( const gpdumbstring& name )
	: m_Name( name )
{
	m_Data = &m_Name;
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::IndexEntry inline implementation

inline void MasterFileMgr::IndexEntry :: Clear( void )
{
	m_Name       . clear();
	m_Parent     = NULL;
	m_IndexPtr   = IndexPtr::Null;
	m_OwnerIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::FileMgrEntry inline implementation

inline MasterFileMgr::FileMgrEntry :: FileMgrEntry( IFileMgr* mgr, bool owned, int priority, const char* driverName )
	: m_DriverName( driverName ? driverName : "" )
{
	m_FileMgr  = mgr;
	m_Owned    = owned;
	m_Priority = priority;
	m_RefCount = 0;
}

inline bool MasterFileMgr::FileMgrEntry :: operator == ( const IFileMgr* mgr ) const
{
	return ( m_FileMgr == mgr );
}

//////////////////////////////////////////////////////////////////////////////
// class MultiFileMgr declaration

#if !GP_RETAIL

class MultiFileMgr : public IFileMgr
{
public:
	SET_INHERITED( MultiFileMgr, IFileMgr );

// MultiFileMgr-specific functions.

			 MultiFileMgr( void );
	virtual ~MultiFileMgr( void );

	// owned filemgr set
	void AddFileMgr    ( IFileMgr* fileMgr, bool owned = false, int priority = 0 );	// adds the given filemgr
	bool RemoveFileMgr ( IFileMgr* fileMgr );										// removes it from the list
	bool EnableFileMgr ( IFileMgr* fileMgr, bool enable = true );					// enables/disables a filemgr (doesn't do anything special tho)
	bool DisableFileMgr( IFileMgr* fileMgr )										{  return ( EnableFileMgr( fileMgr, false ) );  }

	struct NotAdded
	{
		gpwstring m_Name;
		DWORD     m_LastError;

		NotAdded( const gpwstring& name = gpwstring::EMPTY, DWORD lastError = 0 )
			: m_Name( name )
		{
			m_LastError = lastError;
		}
	};

	struct AddResults
	{
		std::list <gpwstring>  m_Added;
		std::list <IFileMgr*>  m_FileMgrs;
		std::list <NotAdded>   m_NotAdded;
		std::list <gpwstring>  m_BadPaths;
	};

	// helpers
	IFileMgr* AddTankFileMgr( const wchar_t* fileName );
	bool      AddTankPaths  ( const wchar_t* paths, bool parseSemicolons = true, AddResults* results = NULL );
	IFileMgr* AddBitsFileMgr( const wchar_t* path );
	bool      AddBitsPaths  ( const wchar_t* paths, bool parseSemicolons = true, AddResults* results = NULL );

	// other query
	bool HasUsedHandles( void ) const;												// anything open?

// File i/o function overrides. See docs in FileSys.h on these.

	virtual FileHandle	Open        ( const char* name, eUsage usage = USAGE_DEFAULT );
	virtual FileHandle	Open        ( IndexPtr where, eUsage usage = USAGE_DEFAULT );
	virtual bool		Close       ( FileHandle file );

	virtual bool		Read        ( FileHandle file, void* out, int size, int* bytesRead = NULL );
	virtual bool		ReadLine    ( FileHandle file, gpstring& out );

	virtual bool		SetReadWrite( FileHandle file, bool set = true );

	virtual bool		Seek        ( FileHandle file, int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int			GetPos      ( FileHandle file );

	virtual int			GetSize     ( FileHandle file );
	virtual eLocation	GetLocation ( FileHandle file );
	virtual bool		GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite );

// Memory mapped i/o function overrides.

	virtual MemHandle	Map    ( FileHandle file, int offset = 0, int size = MAP_REST_OF_DATA );
	virtual bool		Close  ( MemHandle mem );

	virtual int			GetSize( MemHandle mem );
	virtual bool		Touch  ( MemHandle mem, int offset = 0, int size = TOUCH_REST_OF_DATA );
	virtual const void* GetData( MemHandle mem, int offset = 0 );

// File/directory iteration function overrides.

	virtual FindHandle	Find    ( const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual FindHandle	Find    ( FindHandle base, const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual bool		GetNext ( FindHandle find, gpstring& out, bool prependPath = false );
	virtual bool		GetNext ( FindHandle find, FindData& out, bool prependPath = false );
	virtual bool		HasNext ( FindHandle find );
	virtual bool		Close   ( FindHandle find );

// Internal FileSys overrides.

	virtual bool		QueryFileInfo( IndexPtr where, FindData& out );

private:

// Private types.

	struct FileEntry
	{
		IFileMgr*  m_FileMgr;
		FileHandle m_Handle;

		FileEntry( void )
		{
			m_FileMgr = NULL;
		}
	};

	struct MemEntry
	{
		IFileMgr*  m_FileMgr;
		FileHandle m_FileHandle;			// for easier debugging
		MemHandle  m_Handle;

		MemEntry( void )
		{
			m_FileMgr = NULL;
		}
	};

	typedef std::map <gpstring, FindData, istring_less> DirDb;
	typedef DirDb::const_iterator DirDbIter;

	struct FindEntry
	{
		gpstring   m_Path;					// always postfixed with backslash if not empty

		// direct access - used for single filemgr or no wildcards in pattern
		IFileMgr*  m_FileMgr;
		FindHandle m_Handle;

		// gathered access - used for spanning filemgrs
		DirDb      m_DirDb;
		DirDbIter  m_DirIter;

		FindEntry( void )
		{
			m_FileMgr = NULL;
			m_DirIter = m_DirDb.end();
		}
	};

	struct FileMgrEntry
	{
		IFileMgr* m_FileMgr;				// pointer to (possibly owned) filemgr
		bool      m_Owned;					// do we own it?

		FileMgrEntry( IFileMgr* mgr, bool owned )
		{
			m_FileMgr = mgr;
			m_Owned   = owned;
		}
	};

	typedef HandleMgr <FileEntry> FileHandleMgr;
	typedef HandleMgr <MemEntry > MemHandleMgr;
	typedef HandleMgr <FindEntry> FindHandleMgr;
	typedef std::multimap <int, FileMgrEntry> FileMgrColl;
	typedef kerneltool::Critical  Critical;

// Private data.

	// sub-filemgrs
	FileMgrColl m_FileMgrColl;				// set of available file managers (in priority order)

	// handles
	FileHandleMgr m_FileHandleMgr;			// currently open file handles
	MemHandleMgr  m_MemHandleMgr;			// currently open memory handles
	FindHandleMgr m_FindHandleMgr;			// currently open find handles

	// other
	mutable Critical m_Critical;			// serializer

	SET_NO_COPYING( MultiFileMgr );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __MASTERFILEMGR_H

//////////////////////////////////////////////////////////////////////////////
