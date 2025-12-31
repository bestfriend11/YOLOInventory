//////////////////////////////////////////////////////////////////////////////
//
// File     :  TankBuilder.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes to construct and modify tank files.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TANKBUILDER_H
#define __TANKBUILDER_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"
#include "FileSysUtils.h"
#include "GpColl.h"
#include "TankStructure.h"

#include <map>

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace zlib
{
	class DeflateStreamer;
}

//////////////////////////////////////////////////////////////////////////////

namespace Tank  {  // begin of namespace Tank

using namespace TankConstants;

// $ i shouldn't need to do these using directives, but for some reason the
//   compiler doesn't "get" the above 'using namespace' directive sometimes.

using TankConstants::eDataFormat;
using TankConstants::eFileFlags;
using TankConstants::ePriority;
using TankConstants::eTankFlags;

//////////////////////////////////////////////////////////////////////////////
// class Builder declaration

// $$$$ be sure to have versioninfo resource store stuff from the tank, like
//      author, title, description, etc...

// $$$ support adding a default icon to the tank.

class Builder
{
public:
	SET_NO_INHERITED( Builder );

// Types.

	typedef stdx::fast_vector <BYTE> Buffer;
	typedef stdx::fast_vector <FOURCC> FilterColl;

	enum eOptions
	{
		OPTION_NONE          =      0,

		OPTION_PE_FORMAT     = 1 << 0,		// output PE format wrapper
		OPTION_USE_TEMP_FILE = 1 << 1,		// write to a .tmp file and wait for full success before delete-and-rename
		OPTION_BACKUP_OLD    = 1 << 2,		// backup old tank to .bak on an overwrite
	};

	struct FileSpec
	{
		gpstring    m_FileName;				// filename to save as
		eDataFormat m_DataFormat;			// data format for save
		eFileFlags  m_FileFlags;			// flags for this file
		FILETIME    m_FileTime;				// last modified timestamp of file when it was added
		FILETIME    m_DirTime;				// last modified timestamp of dir when it was added
		int         m_CompressionLevel;		// compression level 0-10 (0 is max speed, 10 is max compression, -1 for default)
		int         m_ChunkSize;			// size of chunks to compress data in (0 for none, where streaming is ok, -1 for default)
		float       m_MinCompressionRatio;	// minimum compression ratio required to accept the file using a compressed format (0 for don't use, -1 for default)
		FilterColl  m_Filters;				// filters to run on this

		FileSpec( const gpstring& fileName = gpstring::EMPTY,
				  eDataFormat dataFormat   = TankConstants::DATAFORMAT_RAW,
				  eFileFlags fileFlags     = TankConstants::FILEFLAG_NONE );
	};

	struct Filter
	{
		virtual ~Filter( void )				{  }

		virtual bool OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer ) = 0;
	};

// Setup.

	// ctor/dtor
	Builder( eOptions options = OPTION_NONE );
	Builder( const Builder& other );
   ~Builder( void );

	Builder& operator = ( const Builder& other );

	// tank builder options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// new tank settings
	void SetTankProductVersion( const gpversion& version )		{  m_TankHeader.m_ProductVersion = ::ToMSQAVersion( version );  }
	void SetTankMinimumVersion( const gpversion& version )		{  m_TankHeader.m_MinimumVersion = ::ToMSQAVersion( version );  }
	void SetTankPriority      ( ePriority priority )			{  m_TankHeader.m_Priority = priority;  }
	void SetTankFlags         ( eTankFlags tankFlags )			{  m_TankHeader.m_Flags = tankFlags;  }
	void AddTankFlags         ( eTankFlags tankFlags )			{  m_TankHeader.m_Flags |= tankFlags;  }
	void SetCreatorId         ( FOURCC creatorId )				{  m_TankHeader.m_CreatorId = creatorId;  }
	void SetTitleText         ( const wchar_t* title );
	void SetAuthorText        ( const wchar_t* author );
	void SetCopyrightText     ( const wchar_t* copyright );
	void SetDescriptionText   ( const gpwstring& description )	{  m_DescriptionText = description;  }

// Tank-level functions.

	// $$ don't attempt to be able to modify existing...too hard. just copy from
	//    one to another in already-compressed form. provide func for that.

	// creating new tank
	bool  CreateTank      ( const wchar_t* tankName, bool always );
	bool  CreateTankNew   ( const wchar_t* tankName )			{  return ( CreateTank( tankName, false ) );  }
	bool  CreateTankAlways( const wchar_t* tankName )			{  return ( CreateTank( tankName, true ) );  }
	bool  IsTankOpen      ( void ) const						{  return ( m_File.IsOpen() );  }
	DWORD GetTankSize     ( void ) const						{  return ( m_File.GetSize() );  }

	// finishing
	bool CommitTank( void );
	bool AbortTank ( void );

// File-level functions.

	// chunked output - for streamed output
	bool BeginFile ( FileSpec fileSpec );
	bool WriteFile ( const_mem_ptr data );
	bool EndFile   ( void );
	bool IsFileOpen( void ) const								{  return ( m_CurrentFile != NULL );  }

	// total output - for non-streamed output
	bool AddFile( const FileSpec& fileSpec, const_mem_ptr data );

// Filters.

	static void    RegisterFilter     ( FOURCC fourCc, Filter* takeFilter );
	static void    RegisterFilterChain( FOURCC fourCc, const FOURCC* links, int linkCount );
	static Filter* FindFilter         ( FOURCC fourCc );
	static void    UnregisterFilter   ( FOURCC fourCc );

private:
	bool WriteFile( const_mem_ptr data, bool flush );
	bool RemoveExistingTank( void );

	struct DirData;
	friend DirData;

	typedef Tank::RawIndex::FileEntry FileEntry;
	typedef Tank::RawIndex::CompressedHeader CompressedHeader;
	typedef Tank::RawIndex::ChunkHeader ChunkHeader;

	typedef stdx::fast_vector <ChunkHeader> ChunkColl;

	struct FileData
	{
		FileEntry m_FileEntry;					// raw data entry for the tank
		DirData*  m_ParentDir;					// dir containing this file
		DWORD     m_OffsetOffset;				// absolute offset to offset
		DWORD     m_CompressedSize;				// how big compressed (m_FileEntry has uncompressed size)
		bool      m_MustChunk;					// true if we must use the chunks due to overhead calc'd
		DWORD     m_ChunkSize;					// size of uncompressed chunks in this file
		ChunkColl m_ChunkColl;					// sizes of compressed chunks

		FileData( DirData* parent )  {  ::ZeroMembers( m_FileEntry, m_ChunkSize );  m_ParentDir = parent;  }
	};

	typedef std::map <gpstring, FileData, string_less> FileColl;
	typedef std::map <gpstring, my DirData*, string_less> DirColl;
	typedef stdx::fast_vector <DWORD> OffsetColl;

	struct DirData
	{
		FileColl m_Files;
		DirColl  m_SubDirs;
		FILETIME m_FileTime;
		DWORD    m_Offset;

		DirData( void )  {  m_Offset = 0;  ::ZeroObject( m_FileTime );  }
	   ~DirData( void )  {  Clear();  }

		void      Clear         ( void );
		FileData* GetFile       ( const gpstring& fileName, const FILETIME& dirTime );
		FileData* ChangeFileName( FileData* file, const gpstring& newName );
		DWORD     GetDirCount   ( void ) const;
		DWORD     GetFileCount  ( void ) const;
		bool      WriteDirs     ( DWORD base, DWORD parentOffset, OffsetColl::iterator& io, FileSys::File& out, const char* name = "" );
		bool      WriteFiles    ( DWORD base, DWORD dirBase, DWORD parentOffset, OffsetColl::iterator& io, FileSys::File& out );
	};

	typedef zlib::DeflateStreamer Deflater;

	struct FilterDb : std::map <FOURCC, my Filter*>
	{
		typedef std::map <FOURCC, my Filter*> Inherited;

		~FilterDb( void )
		{
			iterator i, ibegin = begin(), iend = end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				delete ( i->second );
			}
		}

		iterator erase( iterator i )
		{
			delete ( i->second );
			return ( Inherited::erase( i ) );
		}
	};

	// tank spec
	eOptions         m_Options;
	RawIndex::Header m_TankHeader;
	gpwstring        m_DescriptionText;

	// state
	FileSys::File m_File;				// current tank file we're working on building
	Buffer        m_FilterBuffer;		// data goes through here when we're filtering
	bool          m_IsFiltering;		// true if we're filtering
	gpwstring     m_TankFileName;		// name of this tank file
	DirData       m_RootDir;			// root directory
	FileSpec      m_CurrentFileSpec;	// spec for current file we're working with
	FileData*     m_CurrentFile;		// current file we're working with
	my Deflater*  m_Deflater;			// streamer for compressing files
	Buffer        m_DeflateBuffer;		// simple buffer for compressing files
	Buffer        m_TestBuffer;			// just a random test buffer

	// statics
	static FilterDb ms_FilterDb;		// available filters to run on data
};

MAKE_ENUM_BIT_OPERATORS( Builder::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class Writer declaration

class Writer : public FileSys::StreamWriter
{
public:
	SET_INHERITED( Writer, FileSys::StreamWriter );

	Writer( Builder& builder )							: m_Builder( builder )  {  }
	virtual ~Writer( void )								{  }

// Overrides.

	virtual bool Write( const void* out, int size )		{  return ( m_Builder.WriteFile( const_mem_ptr( out, size ) ) );  }

private:
	Builder& m_Builder;

	SET_NO_COPYING( Writer );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Tank

#endif  // __TANKBUILDER_H

//////////////////////////////////////////////////////////////////////////////
