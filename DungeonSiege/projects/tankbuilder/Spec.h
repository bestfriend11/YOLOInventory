//////////////////////////////////////////////////////////////////////////////
//
// File     :  Spec.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains containers for specifications to build tanks.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SPEC_H
#define __SPEC_H

//////////////////////////////////////////////////////////////////////////////

#include "Fuel.h"
#include "TankBuilder.h"

#include <vector>
#include <set>
#include <map>

using namespace Tank;

//////////////////////////////////////////////////////////////////////////////
// spec declarations

// FileSpec.

	struct SrcFileSpec : Builder::FileSpec
	{
		gpstring m_SrcPath;			// where on hard drive we pull file from
		DWORD    m_Size;			// raw size of file
		bool     m_Ordered;			// true if we've ordered the file via journal (i.e. don't touch again)

		SrcFileSpec( void )
		{
			m_Size = 0;
			m_Ordered = false;
		}
	};

	typedef std::list <SrcFileSpec> FileSpecColl;
	typedef std::map <gpstring, FileSpecColl::iterator, string_less> FileSpecIndex;

// Pattern.

	struct Pattern
	{
		gpstring m_PathPattern;		// wildcard for the path portion of the pattern
		bool     m_PathEndsWild;	// true if \*\ style path
		gpstring m_FilePattern;		// wildcard for the file portion of the pattern

		Pattern( const gpstring& pattern = gpstring::EMPTY, bool forCompare = false );

		bool IsMatch( const Pattern& pattern ) const;
		bool IsMatch( const gpstring& pattern ) const		{  return ( IsMatch( Pattern( pattern ) ) );  }
	};

	struct PatternColl : std::vector <Pattern>
	{
		bool Load( FuelHandle block, const char* key );
	};

// FileList.

	struct FileList : std::set <gpstring, string_less>
	{
		bool IsMatch( const gpstring& pattern ) const;

		bool Load( FuelHandle block );
		bool Load( const char* fileName );
		bool Load( FuelHandle block, const char* key );
	};

// BaseSpec.

	struct BaseSpec
	{
		PatternColl m_Include;
		PatternColl m_Exclude;
		PatternColl m_ExcludeZero;
		FileList    m_IncludeList;
		FileList    m_ExcludeList;

		bool IsMatch( const gpstring& pattern ) const;
		bool ShouldExcludeZeroSize( const gpstring& pattern ) const;

		bool Load( FuelHandle block );
	};

// RootDir.

	struct RootDir
	{
		gpstring m_RootDir;
		bool     m_Recursive;

		RootDir( void )
		{
			m_Recursive = false;
		}

		bool Load( const gpstring& path, const RootDir* base = NULL, bool* substitution = NULL );
	};

// InputSpec.

	struct InputSpec : BaseSpec
	{
		bool    m_Global;
		RootDir m_InputRoot;
		RootDir m_OutputRoot;

		InputSpec( void )  {  m_Global = false;  }

		bool Load( FuelHandle block, const InputSpec* global = NULL );
		bool Fill( FileSpecColl& coll, FileSpecIndex& index, const FILETIME& dirTime, gpstring in, gpstring out ) const;
		bool Fill( FileSpecColl& coll, FileSpecIndex& index, const FILETIME& dirTime ) const;
	};

	struct InputSpecColl : std::vector <InputSpec>
	{
		bool Load( FuelHandle config, bool isDev, const InputSpecColl* global = NULL );
		bool Fill( FileSpecColl& coll, FileSpecIndex& index, bool isDev, bool noLiquid, bool nodeleteliquid ) const;
	};

// FormatSpec.

	struct FormatSpec : BaseSpec
	{
		eDataFormat m_DataFormat;
		int         m_CompressionLevel;
		int         m_ChunkSize;
		float       m_MinCompressionRatio;

		FormatSpec( void )
		{
			m_DataFormat          = DATAFORMAT_RAW;
			m_CompressionLevel    = -1;
			m_ChunkSize           = -1;
			m_MinCompressionRatio = -1;
		}

		bool Load  ( FuelHandle block, bool isDev );
		bool Format( FileSpecColl& coll ) const;
	};

	struct FormatSpecColl : std::vector <FormatSpec>
	{
		bool Load  ( FuelHandle config, bool isDev, const FormatSpecColl* global = NULL );
		bool Format( FileSpecColl& coll ) const;
	};

// FilterSpec.

	struct FilterSpec : BaseSpec
	{
		FOURCC m_Filter;

		FilterSpec( void )
		{
			m_Filter = 0;
		}

		bool Load  ( FuelHandle fuel );
		bool Filter( FileSpecColl& coll ) const;
	};

	struct FilterSpecColl : std::vector <FilterSpec>
	{
		bool Load  ( FuelHandle config, bool isDev, const FilterSpecColl* global = NULL );
		bool Filter( FileSpecColl& coll ) const;
	};

// BuildSpec.

	struct BuildSpec
	{
		// spec for building
		gpwstring      m_TankFileName;				// output filename
		InputSpecColl  m_InputSpecColl;				// input* specs used to select files
		FormatSpecColl m_FormatSpecColl;			// format* specs used to tune file formats
		FilterSpecColl m_FilterSpecColl;			// filter* specs used to tune file processing
		gpstring       m_FileJournalName;			// loose spec of the order to use for files
		bool           m_IsDevBuild;				// dev-only build?
		bool           m_NoLiquid;					// don't build liquid files?
		bool           m_NoDeleteLiquid;			// don't delete existing liquid files?
		bool           m_Verify;					// verify tank is correct after done building?
		bool           m_Dump;						// dump to text file after done building?

		// build state
		Tank::Builder  m_Builder;					// our tank builder
		FileSpecColl   m_FileSpecColl;				// files we've collected, in the order they will be stored
		FileSpecIndex  m_FileSpecIndex;				// index into the coll by name of fully-qualified output file, so we can avoid dups and do sorting and such

		static BuildSpec* CreateBuildSpec( const stringtool::CommandLineExtractor& commandLine );
		static gpwstring  GetTankFileName( const stringtool::CommandLineExtractor& commandLine );

		BuildSpec( void )
			: m_Builder(  Tank::Builder::OPTION_USE_TEMP_FILE
						| Tank::Builder::OPTION_BACKUP_OLD )
		{
			m_IsDevBuild     = false;
			m_NoLiquid       = false;
			m_NoDeleteLiquid = false;
			m_Verify         = false;
			m_Dump           = false;
		}

		bool Load( FuelHandle config, const stringtool::CommandLineExtractor& commandLine, const BuildSpec* base = NULL );

		bool Fill          ( void );		// step 1
		bool Format        ( void );		// (optional) step 1a
		bool Filter        ( void );		// (optional) step 1b
		bool OrderByJournal( void );		// (optional) step 1c
		bool OrderBySize   ( void );		// (optional) step 1d
		bool Build         ( void );		// step 2

		bool AutoBuild( void );		// all at once
	};

//////////////////////////////////////////////////////////////////////////////

#endif  // __SPEC_H

//////////////////////////////////////////////////////////////////////////////
