//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankFileMgr.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a file class that works from a single TANK file.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TANKFILEMGR_H
#define __TANKFILEMGR_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSys.h"
#include "FileSysHandleMgr.h"
#include "KernelTool.h"
#include "TankStructure.h"

#include <map>

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// notes

/*
	An IndexPtr is, to a TankFileMgr, a direct pointer to a FileSet or DirSet
	(depending on whether it's a dir or a file).
*/

//////////////////////////////////////////////////////////////////////////////
// class TankFileMgr declaration

class TankFileMgr : public IFileMgr
{
public:
	SET_INHERITED( TankFileMgr, IFileMgr );

	enum eOptions								// options to configure a TankFileMgr
	{
		OPTION_NONE                     =      0,	// disable all options
		OPTION_DISABLE_DECOMPRESS_FATAL = 1 << 0,	// disable fatal on decompression error
		OPTION_NO_PAGING                = 1 << 1,	// do not use paging method - read in full at once
		OPTION_NO_VIRTUALALLOC          = 1 << 2,	// do not use virtualalloc'd memory, use new instead (implies no paging)

#		if !GP_RETAIL
		OPTION_VALIDATE_CRC             = 1 << 3,	// validate crc32 after opening any file (implies no paging)
#		endif // !GP_RETAIL
	};

	// shortcut types
	typedef Tank::RawIndex::Header    RawHeader   ;
	typedef Tank::RawIndex::DirEntry  RawDirEntry ;
	typedef Tank::RawIndex::FileEntry RawFileEntry;

// TankFileMgr-specific functions

	// ctor/dtor
						TankFileMgr( void );
	virtual			   ~TankFileMgr( void );

	// default options
	static void 		SetDefaultOptions( eOptions options )					{  ms_DefaultOptions = options;  }

	// options
	void 				SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void 				ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void				ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool 				TestOptions  ( eOptions options ) const					{  return ( (m_Options & options) != 0 );  }

	// tank file work
	bool				OpenTank        ( const char* name, bool verifyIndex = true, bool verifyData = false );
	bool				CloseTank       ( void );
	bool				IsTankOpen      ( void ) const							{  return ( m_TankFile.IsOpen() );  }
	bool				CanUseTank      ( const gpversion& exeVersion );
	const gpstring& 	GetTankFileName ( void ) const							{  return ( IsTankOpen() ? m_TankFile.GetFileName() : m_LastFileName );  }
	GUID				GetTankGuid     ( void ) const							{  return ( GetTankHeader().m_GUID );  }
	DWORD				GetTankPriority ( void ) const							{  return ( GetTankHeader().m_Priority );  }
	FILETIME			GetTankBuildTime( void ) const;

	// other/special query
	int					GetTankPriorityForMaster( void ) const					{  return ( (INT_MAX / 2) - (int)GetTankPriority() );  }
	gpversion			GetTankProductVersion   ( void ) const;
	gpversion			GetTankMinimumVersion   ( void ) const;
	const Tank::TankFile& GetTankFile           ( void ) const					{  return ( m_TankFile );  }
	const RawHeader&	GetTankHeader           ( void ) const;
	bool				HasUsedHandles          ( void ) const;
	const RawFileEntry* GetRawEntry             ( FileHandle file ) const;
	const RawFileEntry* GetRawEntry             ( MemHandle mem ) const;

	// returns true if this is higher override priority than 'other'
	bool				ShouldOverride( const TankFileMgr& other ) const;

	typedef CBFunctor0wRet <bool> ProgressCb;

	// checks file crc's
	bool				VerifyIndex( bool reportError = false );
	bool				VerifyAll  ( ProgressCb progressCb = NULL );
	bool				VerifyFile ( const char* path );

#	if 0 // $$$ !GP_RETAIL
	void				Dump( int type ) const;
#	endif // !GP_RETAIL

// File i/o function overrides.

	virtual bool		Enable      ( bool enable = true );

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

	virtual FindHandle	Find   ( const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual FindHandle	Find   ( FindHandle base, const char* pattern, eFindFilter filter = FINDFILTER_ALL );
	virtual bool		GetNext( FindHandle find, gpstring& out, bool prependPath = false );
	virtual bool		GetNext( FindHandle find, FindData& out, bool prependPath = false );
	virtual bool		HasNext( FindHandle find );
	virtual bool		Close  ( FindHandle find );

// Internal FileSys overrides.

	virtual bool		QueryFileInfo( IndexPtr where, FindData& out );

// Internal utility.

	struct ResDecoder							// ABC for a resource decoder (zlib/lzo derive from this)
	{
		virtual     ~ResDecoder ( void ) = 0  {  }
		virtual int  DecodeUntil( int offset ) = 0;
	};

private:

// Internal utility.

	// dereferencing
	const RawFileEntry* GetRawFileEntry( IndexPtr indexPtr );
	const RawDirEntry*  GetRawDirEntry ( IndexPtr indexPtr );
	bool                GetRawEntry    ( IndexPtr indexPtr, const RawFileEntry*& rawFileEntry, const RawDirEntry*& rawDirEntry );

	// random helpers
	FileHandle          OpenDirect  ( const RawFileEntry* rawFileEntry, eUsage usage );
	const RawDirEntry*  FindIndex   ( const RawDirEntry* base, const char* name, const char*& lastComponent );		// search for the given file/path in the index starting at 'base'
	FindHandle          FindHelp    ( const RawDirEntry* rawDirEntry, const char* pattern, eFindFilter filter );		// helper that does pattern matching
	void                FillFindData( FindData& out, const RawDirEntry* dirEntry, const RawFileEntry* fileEntry );

// Memory-mapped access structures and types.

	typedef TankConstants::eDataFormat eDataFormat;

	struct ResView								// represents a view of an individual resource in the tank file
	{
		ResView( void )							{  m_RefCount = 1;  m_RawFileEntry = NULL;  }
	   ~ResView( void )							{  gpassert( m_RefCount == 0 );  }

		void AddRef ( void )					{  gpassert( m_RefCount >= 0 );  ++m_RefCount;  }
		void Release( void );

		bool Init( eOptions options, eUsage usage, const TankFileMgr* tankFileMgr, const RawFileEntry* rawFileEntry, HANDLE file );

		int                 m_RefCount;			// when zero delete self
		const RawFileEntry* m_RawFileEntry;		// the file we're viewing
		DiyFileMap          m_FileMap;			// auto-decoding map of the resource
	};

// Handle data structures and types.

	struct FileEntry							// data for a FileHandle
	{
		void Release( void )					{  m_ResView->Release();  m_ResView = NULL;  }

		int         m_FilePointer;				// Seek( ... )
		ResView*    m_ResView;					// Open( ... ) [ptr to ref-counted resource]
	};

	struct MemEntry								// data for a MemHandle
	{
		void Release( void )					{  m_ResView->Release();  m_ResView = NULL;  }

		const void* m_Data;						// m_ResView->m_Data + Map( ..., offset, ... )
		int         m_Size;						// Map( ..., size )
		ResView*    m_ResView;					// Map( file, ... ) [ptr to ref-counted resource]

#		if GP_DEBUG
		FileEntry* m_FileEntry;					// who am i attached to?
#		endif // GP_DEBUG
	};

	struct HandleEntry							// entry for a pool of cached open file handles
	{
		HANDLE   m_File;						// handle
		ResView* m_Owner;						// owning view if any (null if none)
	};

	struct FindEntry							// data for a FindHandle
	{
		enum eMode
		{
			MODE_EMPTY,
			MODE_FIRST,
			MODE_NORMAL,
		};

		const Tank::TankFile* m_TankFile;		// who has our data?
		const RawDirEntry*    m_CurrentDir;		// what dir working on?
		const void*           m_CurrentEntry;	// which child?
		bool                  m_CurrentIsDir;	// current entry is dir?
		const DWORD*          m_EntryIter;		// child offset iterator
		const DWORD*          m_EntryEnd;		// end for child offsets
		eMode                 m_Mode;			// search mode
		gpstring              m_Pattern;		// Find( pattern, ... ) - will be empty if "find all"
		eFindFilter           m_Filter;			// Find( pattern, ... )

		void First  ( void );
		bool Next   ( gpstring* filename, bool prependPath );
		bool IsMatch( void ) const;

		inline void                 Resolve  ( void );
		inline const Tank::NSTRING& GetName  ( const void* entry ) const;
		inline const RawDirEntry*   GetParent( const void* entry ) const;
		inline const RawDirEntry*   GetParent( DWORD offset ) const;
		inline const Tank::NSTRING& GetName  ( void ) const;
		inline const RawDirEntry*   GetParent( void ) const;
	};

	typedef HandleMgr <FileEntry> FileHandleMgr;
	typedef HandleMgr <MemEntry > MemHandleMgr;
	typedef HandleMgr <FindEntry> FindHandleMgr;
	typedef stdx::fast_vector <HandleEntry> HandleColl;
	typedef stdx::linear_map <const RawFileEntry*, ResView*> ResViewMap;

// Private data.

	typedef kerneltool::RwCritical RwCritical;

	// spec
	eOptions  m_Options;				// options for this object
	gpstring  m_LastFileName;			// last time we had valid filename for tank
	RawHeader m_LastHeader;				// last time we had valid tank, copied header out

	static eOptions ms_DefaultOptions;	// default options for all new objects

	// state
	Tank::TankFile m_TankFile;			// the tank file
	FileHandleMgr  m_FileHandleMgr;		// currently open file handles
	MemHandleMgr   m_MemHandleMgr;		// currently open memory handles
	FindHandleMgr  m_FindHandleMgr;		// currently open find handles
	ResViewMap     m_ResViewMap;		// map of file entries to res views (for collecting dupe opens)
	HandleColl     m_HandleColl;		// pool of cached open file handles - one required per open view so each can have its own file ptr
	RwCritical     m_RwCritical;		// serializer for whole collections

	SET_NO_COPYING( TankFileMgr );
};

MAKE_ENUM_BIT_OPERATORS( TankFileMgr::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class TankFileMgr::FindEntry inline implementation

inline void TankFileMgr::FindEntry :: Resolve( void )
{
	m_CurrentEntry = m_TankFile->GetIndexOffset( *m_EntryIter );
	m_CurrentIsDir = m_TankFile->IsDirEntry( m_CurrentEntry );
}

inline const Tank::NSTRING& TankFileMgr::FindEntry :: GetName( const void* entry ) const
{
	return ( m_TankFile->GetName( entry ) );
}

inline const Tank::RawIndex::DirEntry* TankFileMgr::FindEntry :: GetParent( const void* entry ) const
{
	return ( m_TankFile->GetParent( entry ) );
}

inline const Tank::NSTRING& TankFileMgr::FindEntry :: GetName( void ) const
{
	return ( GetName( m_CurrentEntry ) );
}

inline const Tank::RawIndex::DirEntry* TankFileMgr::FindEntry :: GetParent( void ) const
{
	return ( GetParent( m_CurrentEntry ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __TANKFILEMGR_H

//////////////////////////////////////////////////////////////////////////////
