//////////////////////////////////////////////////////////////////////////////
//
// File     :  Spec.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_TankBuilder.h"
#include "Spec.h"

#include "FileSysXfer.h"
#include "Filters.h"
#include "FuelDb.h"
#include "GpLzo.h"
#include "GpZLib.h"
#include "Main.h"
#include "StdHelp.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// helper methods

inline bool ShouldUse( FuelHandle fuel, bool isDev )
{
	return ( fuel && fuel->GetBool( isDev ? "dev_ok" : "retail_ok", true ) );
}

//////////////////////////////////////////////////////////////////////////////
// spec declarations

Pattern :: Pattern( const gpstring& pattern, bool forCompare )
{
	m_PathEndsWild = false;

	// split
	const char* end = FileSys::GetPathEnd( pattern );
	if ( end != NULL )
	{
		// take them
		m_PathPattern = pattern.left( end - pattern );
		m_FilePattern = end;

		// is there path info?
		if ( !m_PathPattern.empty() )
		{
			// always backslashes
			std::replace( m_PathPattern.begin_split(), m_PathPattern.end_split(), '/', '\\' );

			// lowercase
			m_PathPattern.to_lower();

			// no leading backslash
			if ( *m_PathPattern.begin() == '\\' )
			{
				m_PathPattern.erase( 0, 1 );
			}

			// always a trailing backslash
			if ( *m_PathPattern.rbegin() != '\\' )
			{
				m_PathPattern += '\\';
			}

			// check for path ending in wild
			if ( (m_PathPattern.size() > 3) && ::same_with_case( m_PathPattern.end() - 3, (const char*)"\\*\\" ) )
			{
				m_PathEndsWild = true;
			}
		}
		else if ( !forCompare )
		{
			// default to wildcard match
			m_PathPattern = "*";
		}
	}
	else
	{
		// just a filename
		m_FilePattern = pattern;
	}

	// never should be empty
	if ( m_FilePattern.empty() )
	{
		if ( !forCompare )
		{
			m_FilePattern = "*";
		}
	}
	else
	{
		// always lowercase
		m_FilePattern.to_lower();
	}
}

bool Pattern :: IsMatch( const Pattern& pattern ) const
{
	if ( !stringtool::IsDosWildcardMatch( m_PathPattern, pattern.m_PathPattern ) )
	{
		if ( m_PathEndsWild )
		{
			gpstring localPath( m_PathPattern.begin(), m_PathPattern.end() - 2 );
			if ( !stringtool::IsDosWildcardMatch( localPath, pattern.m_PathPattern ) )
			{
				return ( false );
			}
		}
		else
		{
			return ( false );
		}
	}

	return ( stringtool::IsDosWildcardMatch( m_FilePattern, pattern.m_FilePattern ) );
}

bool PatternColl :: Load( FuelHandle block, const char* key )
{
	if ( block->FindFirst( key ) )
	{
		gpstring patternText, each;
		while ( block->GetNext( patternText ) )
		{
			stringtool::Extractor extractor( ",", patternText );
			while ( extractor.GetNextString( each ) )
			{
				using FileSys::IFileMgr::StringColl;

				// check for aliases
				int offset;
				const StringColl* aliases = FileSys::IFileMgr::GetFileTypeAliases( each, &offset );
				if ( aliases != NULL )
				{
					StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						each.replace( offset, gpstring::npos, *i );
						push_back( each );
					}
				}
				else
				{
					push_back( each );
				}
			}
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

bool FileList :: IsMatch( const gpstring& pattern ) const
{
	gpstring localPattern( pattern );
	localPattern.to_lower();

	return ( find( localPattern ) != end() );
}

bool FileList :: Load( FuelHandle block )
{
	gpassert( block );

	if ( block->FindFirst( "*" ) )
	{
		gpstring entry, each;
		while ( block->GetNext( entry ) )
		{
			stringtool::Extractor extractor( ",", entry );
			while ( extractor.GetNextString( each ) )
			{
				// $$$$ process same as above for \\\ in string and lowercase and \ vs /

				each.to_lower();
				insert( each );
			}
		}
	}

	return ( true );
}

bool FileList :: Load( const char* fileName )
{
	FileSys::File file;
	if ( file.OpenExisting( fileName ) )
	{
		FileSys::FileReader reader( file );
		gpstring line;
		while ( reader.ReadLine( line ) )
		{
			// $$$$ process same as above for \\\ in string and lowercase and \ vs /
			// $$$$ watch for open: format stuff...
			// $$$$ handle comma delimiting
		}
	}
	else
	{
		gperrorf(( "Error opening file list in file '%s', error is '%s'\n", fileName, stringtool::GetLastErrorText().c_str() ));
	}

	return ( true );
}

bool FileList :: Load( FuelHandle block, const char* key )
{
	gpassert( empty() );

	if ( block->FindFirst( key ) )
	{
		gpstring patternText, each;
		while ( block->GetNext( patternText ) )
		{
			stringtool::Extractor extractor( ",", patternText );
			while ( extractor.GetNextString( each ) )
			{
				// try first as fuel block
				FuelHandle fuel( each );
				if ( fuel )
				{
					Load( fuel );
				}
				else
				{
					// try as file
					Load( each );
				}
			}
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

bool BaseSpec :: IsMatch( const gpstring& pattern ) const
{
	Pattern localPattern( pattern );

	// check include pattern
	if ( !m_Include.empty() || !m_IncludeList.empty() )
	{
		PatternColl::const_iterator i, ibegin = m_Include.begin(), iend = m_Include.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->IsMatch( localPattern ) )
			{
				break;
			}
		}

		if ( (i == iend) && !m_IncludeList.IsMatch( pattern ) )
		{
			return ( false );
		}
	}

	PatternColl::const_iterator i, ibegin = m_Exclude.begin(), iend = m_Exclude.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->IsMatch( localPattern ) )
		{
			return ( false );
		}
	}

	if ( m_ExcludeList.IsMatch( pattern ) )
	{
		return ( false );
	}

	return ( true );
}

bool BaseSpec :: ShouldExcludeZeroSize( const gpstring& pattern ) const
{
	Pattern localPattern( pattern );

	PatternColl::const_iterator i, ibegin = m_ExcludeZero.begin(), iend = m_ExcludeZero.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->IsMatch( localPattern ) )
		{
			return ( true );
		}
	}

	return ( false );
}

bool BaseSpec :: Load( FuelHandle block )
{
	m_Include    .Load( block, "include*" );
	m_Exclude    .Load( block, "exclude*" );
	m_ExcludeZero.Load( block, "exclude_zero*" );
	m_IncludeList.Load( block, "include_list*" );
	m_ExcludeList.Load( block, "exclude_list*" );

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

static const char s_BaseRootPrefix[] = "%base_root%";

bool RootDir :: Load( const gpstring& path, const RootDir* base, bool* substitution )
{
	gpassert( m_RootDir.empty() );

	// look for base env var
	if ( path.same_no_case( s_BaseRootPrefix, ELEMENT_COUNT( s_BaseRootPrefix ) - 1 ) )
	{
		if ( substitution != NULL )
		{
			*substitution = true;
		}

		// substitute
		if ( (base != NULL) && !base->m_RootDir.empty() )
		{
			m_RootDir = base->m_RootDir;
			stringtool::RemoveTrailingBackslash( m_RootDir );
		}
		m_RootDir += path.c_str() + ELEMENT_COUNT( s_BaseRootPrefix ) - 1;
	}
	else
	{
		if ( substitution != NULL )
		{
			*substitution = false;
		}

		m_RootDir = path;
	}

	// lowercase
	m_RootDir.to_lower();

	// always backslashes
	std::replace( m_RootDir.begin_split(), m_RootDir.end_split(), '/', '\\' );

	// only process if it's got something in it
	if ( !m_RootDir.empty() )
	{
		// check for recursion
		if ( *m_RootDir.rbegin() == '+' )
		{
			m_RootDir.erase( m_RootDir.length() - 1 );
			m_Recursive = true;
		}

		// always a trailing backslash
		if ( *m_RootDir.rbegin() != '\\' )
		{
			m_RootDir += '\\';
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

bool InputSpec :: Load( FuelHandle block, const InputSpec* global )
{
	gpstring inputRoot;

	// get base first if any
	if ( global != NULL )
	{
		m_Include     = global->m_Include;
		m_Exclude     = global->m_Exclude;
		m_ExcludeZero = global->m_ExcludeZero;
	}
	else if ( ::same_no_case( block->GetName(), "input_global" ) )
	{
		m_Global = true;

		// possible override
		inputRoot = stringtool::CommandLineExtractor().FindPostTag( "-in" );
		m_InputRoot.Load( inputRoot, global ? &global->m_InputRoot : NULL );
	}

	// load up our patterns
	BaseSpec::Load( block );

	// get the input root and process it - note that if there is no input
	// root then we just use the global, so we effectively un-global ourself.
	if ( inputRoot.empty() )
	{
		inputRoot = global ? global->m_InputRoot.m_RootDir : "";
		if ( !block->Get( "input_root", inputRoot ) )
		{
			m_Global = false;
			if ( global )
			{
				m_InputRoot = global->m_InputRoot;
			}
		}
		else
		{
			m_InputRoot.Load( inputRoot, global ? &global->m_InputRoot : NULL );
		}
	}

	// get the output root and process it
	gpstring outputRoot = block->GetString( "output_root", global ? global->m_OutputRoot.m_RootDir : "" );
	m_OutputRoot.Load( outputRoot, global ? &global->m_OutputRoot : NULL );
	if ( m_OutputRoot.m_RootDir.empty() && (global != NULL) )
	{
		// use input root extra if didn't specify an output root
		RootDir empty;
		bool substitution;
		m_OutputRoot.Load( inputRoot, &empty, &substitution );

		// no substitution? then just kill it
		if ( !substitution )
		{
			m_OutputRoot.m_RootDir.clear();
		}
	}

	return ( true );
}

bool InputSpec :: Fill( FileSpecColl& coll, FileSpecIndex& index, const FILETIME& dirTime, gpstring in, gpstring out ) const
{
	// status
	gpstatusf(( "Searching: %s\n", in.c_str() ));

	// build input
	stringtool::AppendTrailingBackslash( in );
	gpstring localIn( in );
	localIn += '*';

	// build output
	if ( out.empty() )
	{
		out = '\\';
	}
	else
	{
		stringtool::AppendTrailingBackslash( out );
	}

	// now do the search
	FileSys::FileFinder finder( localIn );
	gpstring fileName, inFile, outFile;
	while ( finder.Next( fileName ) )
	{
		// subdir?
		if ( finder.Get().dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			// if recursively searching, add its files
			if ( m_InputRoot.m_Recursive && !fileName.same_with_case( "." ) && !fileName.same_with_case( ".." ) )
			{
				if ( !Fill( coll, index, finder.Get().ftLastWriteTime, in + fileName, out + fileName ) )
				{
					return ( false );
				}
			}
		}
		else
		{
			// build filenames
			inFile = in;
			inFile += fileName;
			inFile.to_lower();
			outFile = out;
			outFile += fileName;
			outFile.to_lower();

			// only include if nonzero size or if not excluding that zero type
			if ( (finder.Get().nFileSizeLow > 0) || !ShouldExcludeZeroSize( outFile ) )
			{
				// see if we're excluding
				if ( IsMatch( outFile ) )
				{
					// add the file if new
					std::pair <FileSpecIndex::iterator, bool> rc = index.insert( std::make_pair( outFile, FileSpecColl::iterator() ) );
					if ( rc.second )
					{
						// new file
						FileSpecColl::iterator& it = rc.first->second;
						it = coll.insert( coll.end() );

						// configure it
						it->m_SrcPath  = inFile;
						it->m_Size     = finder.Get().nFileSizeLow;
						it->m_FileName = outFile;
						it->m_FileTime = finder.Get().ftLastWriteTime;
						it->m_DirTime  = dirTime;
					}
					else
					{
						// check to see that the one already there is the same as what
						// we just found, otherwise it's a new file with the same name!
						if ( !rc.first->second->m_SrcPath.same_with_case( inFile ) )
						{
							gperrorf(( "Error! The same output file was found in two source locations!\n"
									   "\tOutput file: '%s'\n"
									   "\tSource 1   : '%s'\n"
									   "\tSource 2   : '%s'\n",
									   outFile.c_str(),
									   rc.first->second->m_SrcPath.c_str(),
									   inFile.c_str() ));
						}
					}
				}
			}
		}
	}

	// done
	return ( true );
}

bool InputSpec :: Fill( FileSpecColl& coll, FileSpecIndex& index, const FILETIME& dirTime ) const
{
	return ( Fill( coll, index, dirTime, m_InputRoot.m_RootDir, m_OutputRoot.m_RootDir ) );
}

bool InputSpecColl :: Load( FuelHandle config, bool isDev, const InputSpecColl* global )
{
	if ( global == NULL )
	{
		FuelHandle block = config->GetChildBlock( "input_global" );
		if ( ::ShouldUse( block, isDev ) )
		{
			InputSpec* spec = insert( end() );
			spec->Load( block );
		}
	}

	FuelHandleList blocks = config->ListChildBlocks( 1, "", "input*" );
	FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( ::ShouldUse( *i, isDev ) )
		{
			InputSpec* spec = insert( end() );
			spec->Load( *i, ((global == NULL) || global->empty()) ? NULL : global->begin() );
		}
	}

	return ( true );
}

bool InputSpecColl :: Fill( FileSpecColl& coll, FileSpecIndex& index, bool isDev, bool noLiquid, bool nodeleteliquid ) const
{
	// get current time for root dir
	SYSTEMTIME systemTime;
	::GetSystemTime( &systemTime );
	FILETIME dirTime;
	::SystemTimeToFileTime( &systemTime, &dirTime );

	// fill with each input spec
	gpstring gasRoot;
	for ( const_iterator i = begin() ; i != end() ; ++i )
	{
		const gpstring& root = i->m_InputRoot.m_RootDir;

		if ( i->m_Global )
		{
			gasRoot = root;
		}
		else
		{
			BinaryFuelDB* bdb = gFuelSys.AddBinaryDb( "default_binary" );

			if ( !gasRoot.empty() )
			{
				BinFuel::SetDictionaryPath( gasRoot );
			}
			else
			{
				BinFuel::SetDictionaryPath( root );
			}

			bdb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE, root, root );
			bdb->SetOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS, !isDev );

			if ( noLiquid )
			{
				bdb->DeleteAllLqdFiles();
			}
			else
			{
				gpgenericf(( "Compiling binary fuel from '%s'\n", root.c_str() ));

				bdb->RecompileAll( !nodeleteliquid );
			}
			gFuelSys.Remove( bdb );

			gpgenericf(( "Filling '%s' into '%s'\n", root.c_str(), i->m_OutputRoot.m_RootDir.c_str() ));

			if ( !i->Fill( coll, index, dirTime ) )
			{
				return ( false );
			}
		}
	}

	gpgeneric( "\n" );

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

bool FormatSpec :: Load( FuelHandle block, bool isDev )
{
	bool success = BaseSpec::Load( block );
	if ( success )
	{
		// get the basic format
		gpstring format = block->GetString( "format" );
		if ( format.same_no_case( "lzo" ) )
		{
			m_DataFormat = isDev ? DATAFORMAT_ZLIB : DATAFORMAT_LZO;
		}
		else if ( format.same_no_case( "zlib" ) )
		{
			m_DataFormat = DATAFORMAT_ZLIB;
		}
		else
		{
			gperrorf(( "Unrecognized format '%s' found at '%s'\n", format.c_str(), block->GetAddress().c_str() ));
			success = false;
		}

		// get compression-specific stuff
		if ( success && IsCompressed( m_DataFormat ) )
		{
			if ( block->Get( "compression_level", m_CompressionLevel ) )
			{
				if ( clamp_min_max( m_CompressionLevel, 0, 10 ) )
				{
					gperrorf(( "Compression level given at '%s' is out of range, setting to %d\n",
							   block->GetAddress().c_str(), m_CompressionLevel ));
				}
			}
			if ( block->Get( "chunk_size", m_ChunkSize ) )
			{
				if ( clamp_min( m_ChunkSize, 0 ) )
				{
					gperrorf(( "Chunk size given at '%s' is out of range, setting to %d\n",
							   block->GetAddress().c_str(), m_ChunkSize ));
				}
				if ( !IsAligned( m_ChunkSize, SysInfo::GetSystemPageSize() ) )
				{
					AlignUp( m_ChunkSize, SysInfo::GetSystemPageSize() );
					gperrorf(( "Chunk size given at '%s' is not aligned to page size of %d, rounding up to %d\n",
							   block->GetAddress().c_str(), SysInfo::GetSystemPageSize(), m_ChunkSize ));
				}
			}
			if ( block->Get( "min_compression_ratio", m_MinCompressionRatio ) )
			{
				if ( clamp_min_max( m_MinCompressionRatio, 0.0f, 1.0f ) )
				{
					gperrorf(( "Minimum compression ratio given at '%s' is out of range, setting to %.2f\n",
							   block->GetAddress().c_str(), m_MinCompressionRatio ));
				}
			}
		}
	}

	return ( success );
}

bool FormatSpec :: Format( FileSpecColl& coll ) const
{
	FileSpecColl::iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsMatch( i->m_FileName ) )
		{
			i->m_DataFormat          = m_DataFormat;
			i->m_CompressionLevel    = m_CompressionLevel;
			i->m_ChunkSize           = m_ChunkSize;
			i->m_MinCompressionRatio = m_MinCompressionRatio;
		}
	}

	return ( true );
}

bool FormatSpecColl :: Load( FuelHandle config, bool isDev, const FormatSpecColl* global )
{
	if ( global != NULL )
	{
		*this = *global;
	}
	else
	{
		FuelHandleList blocks = config->ListChildBlocks( 1, "", "format_global*" );
		FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( ::ShouldUse( *i, isDev ) )
			{
				FormatSpec* spec = insert( end() );
				if ( !spec->Load( *i, isDev ) )
				{
					pop_back();
				}
			}
		}
	}

	FuelHandleList blocks = config->ListChildBlocks( 1, "", "format*" );
	FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( ::ShouldUse( *i, isDev ) )
		{
			FormatSpec* spec = insert( end() );
			if ( !spec->Load( *i, isDev ) )
			{
				pop_back();
			}
		}
	}

	return ( true );
}

bool FormatSpecColl :: Format( FileSpecColl& coll ) const
{
	for ( const_iterator i = begin() ; i != end() ; ++i )
	{
		if ( !i->Format( coll ) )
		{
			return ( false );
		}
	}
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

bool FilterSpec :: Load( FuelHandle block )
{
	bool success = BaseSpec::Load( block );
	if ( success )
	{
		gpstring filter = block->GetString( "filter" );
		if ( filter.same_no_case( "create_raw_texture" ) )
		{
			if ( block->GetBool( "mipmaps", false ) )
			{
				if ( block->GetBool( "strip_mip0", false ) )
				{
					m_Filter = ImgFilter::FILTER_RAW_MIPMAPS_STRIPTOP;
				}
				else
				{
					m_Filter = ImgFilter::FILTER_RAW_MIPMAPS;
				}
			}
			else
			{
				m_Filter = ImgFilter::FILTER_RAW_PLAIN;
			}
		}
		else if ( filter.same_no_case( "scan_dollars" ) )
		{
			m_Filter = DollarScanner::FILTER_DEFAULT;
		}
		else
		{
			gperrorf(( "Unrecognized filter '%s' found at '%s'\n", filter.c_str(), block->GetAddress().c_str() ));
			success = false;
		}
	}

	return ( success );
}

bool FilterSpec :: Filter( FileSpecColl& coll ) const
{
	FileSpecColl::iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( IsMatch( i->m_FileName ) )
		{
			if ( !stdx::has_any( i->m_Filters, m_Filter ) )
			{
				i->m_Filters.push_back( m_Filter );
			}
		}
	}

	return ( true );
}

bool FilterSpecColl :: Load( FuelHandle config, bool isDev, const FilterSpecColl* global )
{
	if ( global != NULL )
	{
		*this = *global;
	}
	else
	{
		FuelHandleList blocks = config->ListChildBlocks( 1, "", "filter_global*" );
		FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( ::ShouldUse( *i, isDev ) )
			{
				FilterSpec* spec = insert( end() );
				if ( !spec->Load( *i ) )
				{
					pop_back();
				}
			}
		}
	}

	FuelHandleList blocks = config->ListChildBlocks( 1, "", "filter*" );
	FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( ::ShouldUse( *i, isDev ) )
		{
			FilterSpec* spec = insert( end() );
			if ( !spec->Load( *i ) )
			{
				pop_back();
			}
		}
	}

	return ( true );
}

bool FilterSpecColl :: Filter( FileSpecColl& coll ) const
{
	for ( const_iterator i = begin() ; i != end() ; ++i )
	{
		if ( !i->Filter( coll ) )
		{
			return ( false );
		}
	}
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

BuildSpec* BuildSpec :: CreateBuildSpec( const stringtool::CommandLineExtractor& commandLine )
{
	// get our config name
	gpstring configName = commandLine.FindPostTag( "-config" );

	// if config is empty, then attempt to choose the default
	if ( configName.empty() )
	{
		FuelHandle root( "root" );
		FuelHandleList blocks;
		if ( root && root->ListChildBlocks( 1, NULL, NULL, blocks ) && (blocks.size() == 1) )
		{
			configName = root->GetName();
		}
		else
		{
			configName = "default";
		}
	}

	FuelHandle config( configName );
	if ( !config )
	{
		gperrorf(( "Configuration '%s' not found\n\n", configName.c_str() ));

		{
			gpgeneric( "Valid configuration blocks:\n" );
			ReportSys::AutoIndent autoIndent( &gGenericContext );

			FuelHandle root( "root" );
			FuelHandleList blocks;
			if ( root && root->ListChildBlocks( 1, NULL, NULL, blocks ) && !blocks.empty() )
			{
				FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					gpgenericf(( "%s\n", (*i)->GetName() ));
				}
			}
			else
			{
				gpgeneric( "None found!\n" );
			}
		}

		return ( NULL );
	}

	std::auto_ptr <BuildSpec> base;
	std::auto_ptr <BuildSpec> spec( new BuildSpec );

	gpstring baseName = config->GetString( "specializes" );
	if ( !baseName.empty() )
	{
		FuelHandle baseConfig( baseName );
		if ( baseConfig )
		{
			base = std::auto_ptr <BuildSpec> ( new BuildSpec );
			base->Load( baseConfig, commandLine );
		}
		else
		{
			gperrorf(( "Unable to find specialization base '%s' requested from '%s'\n",
					   baseName.c_str(), config->GetAddress() ));
		}
	}

	spec->Load( config, commandLine, base.get() );

	return ( spec.release() );
}

gpwstring BuildSpec :: GetTankFileName( const stringtool::CommandLineExtractor& commandLine )
{
	gpwstring tankFilename = ::ToUnicode( commandLine.FindPostTag( "-file" ) );
	if ( tankFilename.empty() )
	{
		// check aliases for all types
		const FileSys::IFileMgr::StringColl* aliases = FileSys::IFileMgr::GetFileTypeAliases( "%tank%" );
		FileSys::IFileMgr::StringColl localAliases;
		if ( aliases == NULL )
		{
			localAliases.push_back( "tank" );
			aliases = &localAliases;
		}

		// try looking for .tank files on cl
		stringtool::CommandLineExtractor::ConstIter i, begin = commandLine.Begin(), end = commandLine.End();
		for ( i = begin ; (i != end) && tankFilename.empty() ; ++i )
		{
			const char* ext = FileSys::GetExtension( *i );
			if ( ext != NULL )
			{
				FileSys::IFileMgr::StringColl::const_iterator j, jbegin = aliases->begin(), jend = aliases->end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( j->same_no_case( ext ) )
					{
						tankFilename = ::ToUnicode( *i );
						break;
					}
				}
			}
		}
	}

	return ( tankFilename );
}

static bool ReadProductVersion( gpversion& productVersion, const char* codeVersionPath )
{
	// try to open file
	FileSys::File in;
	if ( !in.OpenExisting( codeVersionPath ) )
	{
		gperrorf(( "Unable to open file '%s' for version extraction, error is 0x%08X ('%s')\n",
				   codeVersionPath, ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	// read until we find what we want
	FileSys::FileReader reader( in );
	gpstring line;
	while ( reader.ReadLine( line ) )
	{
		size_t found = line.find( "#define DS1_VERSION_TEXT" );
		if ( found != line.npos )
		{
			// extract info
			size_t b = line.find( '\"' ), e = line.find( '\\' );
			if ( (b != line.npos) && (e != line.npos) )
			{
				gpstring version = line.substr( b + 1, e - b - 1 );
				if ( ::FromString( version, productVersion ) )
				{
					return ( true );
				}
			}

			// failure i guess
			gperrorf(( "Unable to extract version info from DS1_VERSION_TEXT line '%s'\n", line.c_str() ));
			return ( false );
		}
		line.clear();
	}

	// no work!
	gperrorf(( "Unable to find DS1_VERSION_TEXT in '%s' for version extraction.\n", codeVersionPath ));
	return ( false );
}

static bool GetProductVersion( gpversion& version, FuelHandle config, const char* name, const char* versionPath, const stringtool::CommandLineExtractor& commandLine )
{
	gpstring text;
	if ( config->Get( name, text ) )
	{
		if ( FromString( text, version ) || version.InitFromResources( versionPath + text ) )
		{
			return ( true );
		}

		// try the shadow instead looking for version.h
		gpstring fallbackPath = commandLine.FindPostTag( "-fallback_version_path" );
		if ( !fallbackPath.empty() )
		{
			if ( ::ReadProductVersion( version, fallbackPath ) )
			{
				return ( true );
			}
		}
		else
		{
			gperrorf(( "Unable to use '%s' for %s info\n", text.c_str(), name ));
		}
	}

	return ( false );
}

bool BuildSpec :: Load( FuelHandle config, const stringtool::CommandLineExtractor& commandLine, const BuildSpec* base )
{
	if ( base != NULL )
	{
		*this = *base;
	}

	// $$$$ make the following setup tune its defaults based on building
	//      core vs. map vs. mod etc

	// config prefs
	m_FileJournalName = commandLine.FindPostTag( "-journal" );
	m_IsDevBuild      = commandLine.HasTag( "-dev" );
	m_NoLiquid        = commandLine.HasTag( "-noliquid" );
	m_NoDeleteLiquid  = commandLine.HasTag( "-nodeleteliquid" ) || commandLine.HasTag( "-nodeletelqd" );
	m_Verify          = commandLine.HasTag( "-verify" );
	m_Dump            = commandLine.HasTag( "-dump" );

	// get some vars
	bool isGpg = config->GetBool( "is_gpg", false );
	gpwstring wtext;
	gpstring text, each;

	// non-retail tank?
	if ( m_IsDevBuild )
	{
		m_Builder.AddTankFlags( Tank::TANKFLAG_NON_RETAIL );
	}

	// special options
	if ( commandLine.HasTag( "-nobak" ) )
	{
		m_Builder.ClearOptions( Tank::Builder::OPTION_BACKUP_OLD );
	}

	// first set up our aliases
	FuelHandle aliases = config->GetChildBlock( "aliases" );
	if ( aliases && aliases->FindFirstKeyAndValue() )
	{
		gpstring key, value;
		while ( aliases->GetNextKeyAndValue( key, value ) )
		{
			stringtool::Extractor extractor( ",", value );
			while ( extractor.GetNextString( each ) )
			{
				FileSys::IFileMgr::AddFileTypeAlias( key, each );
			}
		}
	}

	// build filename from cl
	m_TankFileName = GetTankFileName( commandLine );
	if ( m_TankFileName.empty() )
	{
		// try getting it from block
		config->Get( "file", m_TankFileName );
		if ( m_TankFileName.empty() )
		{
			// now do it by constructing from config name
			m_TankFileName = ::ToUnicode( config->GetName() );
			if ( m_TankFileName.find_first_of( L"." ) == gpstring::npos )
			{
				m_TankFileName += L".dsres";
			}
		}
	}
	gpassert( !m_TankFileName.empty() );

	// alter output path if so desired
	text = commandLine.FindPostTag( "-out" );
	if ( !text.empty() )
	{
		stringtool::AppendTrailingBackslash( text );

		const wchar_t* filename = FileSys::GetFileName( m_TankFileName );
		m_TankFileName.erase( 0, filename - m_TankFileName.c_str() );
		m_TankFileName.insert( 0, ::ToUnicode( text ) );
	}

	// version info may come from a path
	gpstring versionPath = commandLine.FindPostTag( "-version_path" );
	stringtool::AppendTrailingBackslash( versionPath );

	// get product version
	gpversion version;
	if ( ::GetProductVersion( version, config, "product_version", versionPath, commandLine ) )
	{
		m_Builder.SetTankProductVersion( version );
	}
	if ( ::GetProductVersion( version, config, "minimum_version", versionPath, commandLine ) )
	{
		m_Builder.SetTankMinimumVersion( version );
	}

	// get priority
	if ( config->Get( "priority", text ) )
	{
		ePriority priority;
		if ( FromString( text, priority ) )
		{
			m_Builder.SetTankPriority( priority );
		}
		else
		{
			gperrorf(( "Bad priority format: '%s'\n", text.c_str() ));
		}
	}
	else if ( isGpg )
	{
		m_Builder.SetTankPriority( PRIORITY_FACTORY  );
	}

	// get tank flags
	if ( config->Get( "tank_flags", text ) )
	{
		eTankFlags tankFlags;
		if ( FromString( text, tankFlags ) )
		{
			if ( m_IsDevBuild )
			{
				tankFlags |= Tank::TANKFLAG_NON_RETAIL;
			}
			m_Builder.SetTankFlags( tankFlags );
		}
		else
		{
			gperrorf(( "Bad tank flags format: '%s'\n", text.c_str() ));
		}
	}

	// get creator
	if ( isGpg )
	{
		m_Builder.SetCreatorId( CREATOR_GPG );
	}

	// get info
	if ( config->Get( "title", wtext ) )
	{
		m_Builder.SetTitleText( wtext );
	}
	if ( config->Get( "author", wtext ) )
	{
		m_Builder.SetAuthorText( wtext );
	}
	if ( config->Get( "description", wtext ) )
	{
		m_Builder.SetDescriptionText( wtext );
	}

	// get specs
	m_InputSpecColl .Load( config, m_IsDevBuild, base ? &base->m_InputSpecColl  : NULL );
	m_FormatSpecColl.Load( config, m_IsDevBuild, base ? &base->m_FormatSpecColl : NULL );
	m_FilterSpecColl.Load( config, m_IsDevBuild, base ? &base->m_FilterSpecColl : NULL );

	return ( true );
}

bool BuildSpec :: Fill( void )
{
	gpgeneric( "Filling...\n" );

	bool rc = m_InputSpecColl.Fill( m_FileSpecColl, m_FileSpecIndex, m_IsDevBuild, m_NoLiquid, m_NoDeleteLiquid );

	if ( m_FileSpecColl.empty() )
	{
		gpwarning( "Warning: no files selected for inclusion in tank!\n" );
	}

	return ( rc );
}

bool BuildSpec :: Format( void )
{
	if ( m_FormatSpecColl.empty() )
	{
		return ( true );
	}

	gpgeneric( "Formatting...\n" );

	return ( m_FormatSpecColl.Format( m_FileSpecColl ) );
}

bool BuildSpec :: Filter( void )
{
	if ( m_FilterSpecColl.empty() )
	{
		return ( true );
	}

	gpgeneric( "Filtering...\n" );

	return ( m_FilterSpecColl.Filter( m_FileSpecColl ) );
}

bool BuildSpec :: OrderByJournal( void )
{
	if ( m_FileJournalName.empty() || m_IsDevBuild )
	{
		return ( true );
	}

	gpgeneric( "Ordering..." );

	FileSys::File journalFile;
	if ( !journalFile.OpenExisting( m_FileJournalName ) )
	{
		gperrorf(( "Unable to open journal file '%s', error is '%s'\n",
				   m_FileJournalName.c_str(), stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	FileSpecColl::iterator nextInsert = m_FileSpecColl.begin();
	FileSys::FileReader reader( journalFile );
	gpstring line;

	// get a default string coll with a single entry for reuse down below
	using FileSys::IFileMgr::StringColl;
	StringColl localStringColl;
	localStringColl.push_back();

	// read until no more lines or until we've found every entry
	for ( int remaining = m_FileSpecColl.size() ; (remaining > 0) && reader.ReadLine( line ) ; line.clear() )
	{
		// check for aliases
		int offset = 0;
		const StringColl* aliases = FileSys::IFileMgr::GetFileTypeAliases( line, &offset );
		if ( aliases == NULL )
		{
			aliases = &localStringColl;
			localStringColl.back() = line;
		}

		// loop over aliases
		StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// swap alias
			if ( offset != 0 )
			{
				line.replace( offset, gpstring::npos, *i );
			}

			// look it up in the index
			FileSpecIndex::iterator found = m_FileSpecIndex.find( line );
			if ( found != m_FileSpecIndex.end() )
			{
				if ( !found->second->m_Ordered )
				{
					m_FileSpecColl.splice( nextInsert, m_FileSpecColl, found->second );
					nextInsert->m_Ordered = true;
					--remaining;
				}

				break;
			}
		}
	}

	gpgenericf((
			"%d entries placed, %d%% coverage\n",
			m_FileSpecColl.size() - remaining,
			Round( 100.0f - (((float)remaining * 100.0f) / m_FileSpecColl.size()) ) ));

	return ( true );
}

bool BuildSpec :: OrderBySize( void )
{
	gpgeneric( "Optimizing small files..." );

	// move small files to the front, but keep their relative order the same in
	// case they were put there by the journaling facility. otherwise they'll
	// just be alphabetical.

	FileSpecColl::iterator nextInsert = m_FileSpecColl.begin();
	int filesMoved = 0, bytesMoved = 0;

	FileSpecColl::iterator i, ibegin = m_FileSpecColl.begin(), iend = m_FileSpecColl.end();
	for ( i = ibegin ; i != iend ; )
	{
		if ( i->m_Size < Tank::LARGE_FILE )
		{
			++filesMoved;
			bytesMoved += i->m_Size;

			FileSpecColl::iterator next = i++;
			m_FileSpecColl.splice( nextInsert, m_FileSpecColl, next );
		}
		else
		{
			++i;
		}
	}

	gpgenericf((
			"%d entries moved (%d bytes), %d%% of total\n",
			filesMoved, bytesMoved,
			Round( ((float)filesMoved * 100.0f) / m_FileSpecColl.size() ) ));

	return ( true );
}

bool BuildSpec :: Build( void )
{
	gpgeneric( "Building...\n\n" );

	// try to start the build
	if ( !m_Builder.CreateTankAlways( m_TankFileName ) )
	{
		gperrorf(( "Unable to create tank '%S', error is '%s' (0x%08X)\n",
				   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		return ( false );
	}

	// gather total
	int totalSize = 0, writtenSize = 0;
	FileSpecColl::const_iterator i, ibegin = m_FileSpecColl.begin(), iend = m_FileSpecColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		totalSize += i->m_Size;
	}

	// get start time
	double startTime = ::GetSystemSeconds();

	// iterate through all our files and add each
	int index = 0;
	for ( i = ibegin ; i != iend ; ++i, ++index )
	{
		try
		{
			FileSys::MemoryFile file;
			bool wasFileEmpty;
			if ( file.Create( i->m_SrcPath, &wasFileEmpty ) )
			{
				if ( gBuilderApp.BeginReportStatus() )
				{
					// $$$ also do est. time to completion
					
					gpstatus( "\n" );

					if ( totalSize > 0 )
					{
						gpstatusf(( "Bytes: %2d%% (%d/%d)\n", (int)ceil( (writtenSize * 100.0f) / totalSize ), writtenSize, totalSize ));
					}

					gpstatusf(( "Files: %2d%% (%d/%d)\n", (int)ceil( (index * 100.0f) / m_FileSpecColl.size() ), index, m_FileSpecColl.size() ));

					const zlib::PerfCounter& zlibCounter = zlib::GetPerfCounters();
					if ( (zlibCounter.m_BytesDeflatedIn > 0) && (zlibCounter.m_SecondsDeflating > 0) )
					{
						int ratio = 100 - int( zlibCounter.m_BytesDeflatedOut * 100.0f / zlibCounter.m_BytesDeflatedIn );
						int rate = int( (zlibCounter.m_BytesDeflatedIn / 1024) / zlibCounter.m_SecondsDeflating );
						gpstatusf(( "ZLIB : Avg compression = %2d%% at %dK bytes/sec\n", ratio, rate ));
					}

					const lzo::PerfCounter& lzoCounter = lzo::GetPerfCounters();
					if ( (lzoCounter.m_BytesDeflatedIn > 0) && (lzoCounter.m_SecondsDeflating > 0) )
					{
						int ratio = 100 - int( lzoCounter.m_BytesDeflatedOut * 100.0f / lzoCounter.m_BytesDeflatedIn );
						int rate = int( (lzoCounter.m_BytesDeflatedIn / 1024) / lzoCounter.m_SecondsDeflating );
						gpstatusf(( "LZO  : Avg compression = %2d%% at %dK bytes/sec\n", ratio, rate ));
					}

					double deltaTime = ::GetSystemSeconds() - startTime;
					if ( deltaTime > 0 )
					{
						int rate = int( (writtenSize / 1024) / deltaTime );
						gpstatusf(( "Write: %dK bytes/sec\n", rate ));
					}

					gBuilderApp.EndReportStatus();
				}

				m_Builder.AddFile( *i, const_mem_ptr( file.GetData(), file.GetSize() ) );
				writtenSize += i->m_Size;
			}
			else if ( wasFileEmpty )
			{
				// zero-length files are ok
				m_Builder.AddFile( *i, const_mem_ptr() );
			}
			else
			{
				gperrorf(( "\nUnable to open/read file '%s', error is '%s' (0x%08X)\n",
						   i->m_SrcPath.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			}
		}
		catch ( ... )
		{
			gperrorf(( "\nAn exception occurred while attempting to read file '%s', possibly corrupt file?\n",
					   i->m_SrcPath.c_str() ));
		}
	}
	gpgeneric( "\n\n" );

	// commit it
	if ( !m_Builder.CommitTank() )
	{
		gperrorf(( "Unable to commit tank '%S', error is '%s' (0x%08X)\n",
				   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
		return ( false );
	}

	// done
	return ( true );
}

bool BuildSpec :: AutoBuild( void )
{
	bool rc =  Fill()
			&& Format()
			&& Filter()
			&& OrderByJournal()
			&& OrderBySize()
			&& Build();
	gpgeneric( "\n" );

	if ( rc && m_Verify )
	{
		gpgeneric( "Verifying tank...\n" );

		BuilderApp::TokenColl tokens;
		tokens.push_back( "+" );
		if ( BuilderApp::VerifyFiles( m_TankFileName, tokens ) )
		{
			gpgeneric( "Tank verified OK!!\n" );
		}
		else
		{
			rc = false;
		}
	}

	if ( rc && m_Dump )
	{
		Tank::TankFile tankFile;
		if ( tankFile.Open( ::ToAnsi( m_TankFileName ) ) )
		{
			gpstring dumpFile = ::ToAnsi( m_TankFileName ) + ".txt";
			ReportSys::LocalContext context( new ReportSys::FileSink( dumpFile, true ), true );
			context.OutputF( "Dump of tank '%S'\n\n", FileSys::GetFileName( m_TankFileName ) );
			tankFile.Dump( context );
			gpgenericf(( "\nTank dumped OK to '%s'!!\n", dumpFile.c_str() ));
		}
		else
		{
			gperrorf(( "Unable to open tank file '%S', error is '%s' (0x%08X)\n",
					   m_TankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
			rc = false;
		}
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
