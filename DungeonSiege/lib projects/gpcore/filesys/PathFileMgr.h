//////////////////////////////////////////////////////////////////////////////
//
// File     :  PathFileMgr.h (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Summary  :  Contains a file class based on paths and files in the Win32
//             file system. Give it a tree root and off it goes...
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __PATHFILEMGR_H
#define __PATHFILEMGR_H_

//////////////////////////////////////////////////////////////////////////////

#include "FileSys.h"
#include "FileSysHandleMgr.h"
#include "GpColl.h"
#include "KernelTool.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class PathFileMgr declaration

class PathFileMgr : public IFileMgr
{
public:
	SET_INHERITED( PathFileMgr, IFileMgr );

// Types.

	enum eOptions								// options to configure a PathFileMgr
	{
		// one of
		OPTION_EXTERNAL_ADDRESSING = 1 << 0,		// (default in !GP_RETAIL) allow Open() to use absolute paths and to ..\.. up out of the root
		OPTION_QUERY_ABS_LOCATIONS = 1 << 1,		// (default) always query absolute files for their eLocation (otherwise they inherit root path's eLocation)
		OPTION_COPY_NET_LOCAL      = 1 << 2,		// always copy net files to local memory on an Open() (for VSS shadow work)

		// OR one of
		OPTION_NONE = 0,							// disable all options
		OPTION_ALL  = 0xFFFFFFFF,					// enable all options
	};

// PathFileMgr-specific functions.

	// ctor/dtor
	         PathFileMgr( void );
	         PathFileMgr( const char* root, bool autoResolve = true );						// calls SetRoot()
	virtual ~PathFileMgr( void );

	// root work
	bool				SetRoot       ( const char* root, bool autoResolve = true );		// set root path - set autoResolve to true to fully-qualify ".\xxx" type relative paths
	static gpstring		BuildRoot     ( const char* root, bool autoResolve = true );
	const gpstring&		GetRootPath   ( void ) const  {  return ( m_RootPath );  }
	const gpstring&		GetRootDrive  ( void ) const  {  return ( m_RootDrive );  }

	// pathfilemgr options
	void 				SetOptions    ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void 				ClearOptions  ( eOptions options )						{  SetOptions( options, false );  }
	void				ToggleOptions ( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool 				TestOptions   ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// other query
	bool 				HasUsedHandles( void ) const;
	bool 				IsReady       ( void ) const;
	bool				IsCopyingLocal( const gpstring& name ) const;

#	if !GP_RETAIL
	void				Dump( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL

// File I/O function overrides.

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

// Memory mapped I/O function overrides.

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

private:

// Private utility.

	// returns false if file is not allowed (it's absolute or whatever) - the
	//  "resolved" string gets the root path prepended if necessary.
	bool ResolveFileName( gpstring& resolved, eLocation* location, const char* name );

	// inits self - call with ctor only
	void PrivateInit( void );

// Handle data structures and types.

	struct FileEntry
	{
		HANDLE      m_File;			// Win32::CreateFile()
		HANDLE      m_Map;			// Win32::CreateFileMapping()
		const void* m_View;			// Win32::MapViewOfFile()
		DWORD       m_Size;			// Win32::GetFileSize()
		eLocation   m_Location;		// FileSys::GetLocation()
		DWORD       m_FilePointer;	// Win32::SetFilePointer()

#		if GP_DEBUG
		gpstring    m_Name;			// Open()
#		endif // GP_DEBUG
	};

	struct MemEntry
	{
		const void* m_Data;			// FileEntry::m_View + Map( ..., offset, ... );
		DWORD       m_Size;			// Map( ..., size )
		FileHandle  m_File;			// Map( file, ... )

#		if GP_DEBUG
		gpstring    m_Name;			// Open()
#		endif // GP_DEBUG
	};

	struct FindEntry
	{
		enum eMode
		{
			MODE_EMPTY,
			MODE_FIRST,
			MODE_NORMAL,
		};

		HANDLE          m_Find;			// Win32::FindFirstFile()
		WIN32_FIND_DATA m_FindData;		// Win32::FindNextFile()
		eMode           m_Mode;			// search mode
		gpstring        m_Pattern;		// Find( pattern, ... )
		eFindFilter     m_Filter;   	// Find( pattern, ... )

		bool First  ( const char* pattern );
		bool Next   ( bool prependPath );
		bool Close  ( void );
		bool IsMatch( void ) const;
	};

	typedef HandleMgr <FileEntry> FileHandleMgr;
	typedef HandleMgr <MemEntry > MemHandleMgr;
	typedef HandleMgr <FindEntry> FindHandleMgr;
	typedef kerneltool::Critical  Critical;

// Private data.

	// spec
	gpstring  m_RootPath;				// root of tree to manage
	gpstring  m_RootDrive;				// just the root of m_RootName
	eLocation m_RootLocation;			// where's the root?
	eOptions  m_Options;				// options for this object

	// state
	FileHandleMgr    m_FileHandleMgr;	// currently open file handles
	MemHandleMgr     m_MemHandleMgr;	// currently open memory handles
	FindHandleMgr    m_FindHandleMgr;	// currently open find handles
	mutable Critical m_Critical;		// serializer

	SET_NO_COPYING( PathFileMgr );
};

MAKE_ENUM_BIT_OPERATORS( PathFileMgr::eOptions );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __PATHFILEMGR_H__

//////////////////////////////////////////////////////////////////////////////
