//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.cpp
// Author(s):  Scott Bilas
//
// Summary  :  This is the DERES utility - it will convert a set of resources
//             from a .res file into a .dat file that can be used with a
//             series of #includes using different #defines to select the
//             resource to use for a template.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_DeRes.h"

#include "ReportSys.h"
#include "StringTool.h"

#include <iostream>

//////////////////////////////////////////////////////////////////////////////
// class ResId declaration and implementation

class ResId
{
public:
	const void* Init( const void* ptr, bool align = false )
	{
		m_Name.erase();
		m_Number = 0;

		const WORD* wptr = rcast <const WORD*> ( ptr );
		if ( *wptr == 0xFFFF )
		{
			++wptr;
			m_Number = *wptr;
			++wptr;
		}
		else
		{
			for ( ; *wptr != 0 ; ++wptr )
			{
				m_Name += *(rcast <const char*> ( wptr ));
			}
			++wptr;
			if ( align )
			{
				wptr = (const WORD*)GetDwordAlignUp( (DWORD)wptr );
			}
		}

		return ( wptr );
	}

	bool IsNumber( void ) const						{  return (  m_Name.empty() );  }
	bool IsName  ( void ) const						{  return ( !m_Name.empty() );  }
	bool IsNull  ( void ) const						{  return (  m_Name.empty() && (m_Number == 0) );  }

	DWORD           GetNumber( void ) const			{  gpassert( IsNumber() );  return ( m_Number );  }
	const gpstring& GetName  ( void ) const			{  gpassert( IsName() );  return ( m_Name );  }

	gpstring MakeIdName( void ) const
	{
		if ( IsName() )
		{
			return ( GetName() );
		}
		else
		{
			return ( gpstringf( "%d", GetNumber() ) );
		}
	}

	gpstring MakeTypeName( void ) const
	{
		if ( IsName() )
		{
			return ( GetName() );
		}
		else
		{
			switch ( GetNumber() )
			{
				#define TYPE( x )  case ( RT_##x ):  return ( #x );
				TYPE( CURSOR );
				TYPE( BITMAP );
				TYPE( ICON );
				TYPE( MENU );
				TYPE( DIALOG );
				TYPE( STRING );
				TYPE( FONTDIR );
				TYPE( FONT );
				TYPE( ACCELERATOR );
				TYPE( RCDATA );
				TYPE( MESSAGETABLE );
				TYPE( GROUP_CURSOR );
				TYPE( GROUP_ICON );
				TYPE( VERSION );
				TYPE( DLGINCLUDE );
				TYPE( PLUGPLAY );
				TYPE( VXD );
				TYPE( ANICURSOR );
				TYPE( ANIICON );
				#undef TYPE
			}
			gpassert( 0 );
			return ( "RT_UNKNOWN" );
		}
	}

	bool operator == ( WORD val ) const				{  return ( IsNumber() ? ( m_Number == val ) : false );  }
	bool operator == ( const gpstring& val ) const	{  return ( IsName() ? m_Name.same_no_case( val ) : false );  }

private:
	gpstring m_Name;
	WORD     m_Number;
};

//////////////////////////////////////////////////////////////////////////////
// class ResHeader declaration and implementation

template <typename T>
const void* CopyAndAdvance( T& obj, const void* ptr )
{
	const T* tptr = rcast <const T*> ( ptr );
	obj = *tptr;
	return ( tptr + 1 );
}

class ResHeader
{
public:
	const void* Init( const void* ptr )
	{
		// peek at size
		if ( rcast <const Sizes*> ( ptr )->m_HeaderSize == 0 )
		{
			return ( NULL );
		}

		// init
		m_Base = rcast <const BYTE*> ( ptr );
		ptr = CopyAndAdvance( m_Sizes, ptr );
		ptr = m_Type.Init( ptr );
		ptr = m_Name.Init( ptr, true );
		ptr = CopyAndAdvance( m_Info, ptr );
		return ( GetDataBegin() );
	}

	const ResId& GetType( void ) const			{  return ( m_Type );  }
	const ResId& GetName( void ) const			{  return ( m_Name );  }

	const void* GetDataBegin( void ) const		{  return ( m_Base + m_Sizes.m_HeaderSize );  }
	const void* GetDataEnd  ( void ) const		{  return ( (const void*)GetDwordAlignUp( (DWORD)(m_Base + m_Sizes.m_HeaderSize + m_Sizes.m_DataSize) ) );  }

	DWORD GetDataSize  ( void ) const			{  return ( m_Sizes.m_DataSize   );  }
	DWORD GetHeaderSize( void ) const			{  return ( m_Sizes.m_HeaderSize );  }

private:
	#pragma pack ( push, 1 )

	struct Sizes
	{
		DWORD m_DataSize;
		DWORD m_HeaderSize;
	};

	struct Info
	{
		DWORD m_DataVersion;
		WORD  m_MemoryFlags;
		WORD  m_Language;
		DWORD m_Version;
		DWORD m_Characteristics;
	};

	#pragma pack ( pop, 1 )

	const BYTE* m_Base;
	Sizes       m_Sizes;
	ResId       m_Type;
	ResId       m_Name;
	Info        m_Info;
};

//////////////////////////////////////////////////////////////////////////////
// Main

int main( int /*argc*/, const char* /*argv*/[] )
{
	#define SYSTRYF( op, desc )													\
		if ( !(op) )															\
		{																		\
			DWORD err = ::GetLastError();										\
			gpstring descText;													\
			descText.assignf desc;												\
			gperrorf(( "Error while %s: 0x%08X ('%s')\n",						\
					   descText.c_str(), err,									\
					   stringtool::GetLastErrorText().c_str() ));				\
			return ( (int)err );												\
		}

	#define SYSTRY( op )														\
		SYSTRYF( op, ( "File: %s, Line: %d", __FILE__, __LINE__ ) )

	// setup debug output
	gGlobalSink.AddSink( new ReportSys::OstreamSink( std::cout ), true );

// Get commands.

	// tokenize
	std::vector <gpstring> tokens;

	stringtool::TokenizeCommandLine( tokens );
	if ( tokens.size() < 3 )
	{
		gpgeneric( "Usage: DeRes.exe <DAT-file> [<prefix_opt>] BUILD=<RES-file> ...\n"
				   "\n"
				   "Example: DeRes.exe file.dat GP_ DEBUG=debug\file.res\n" );
		return ( -1 );
	}

	// get some vars
	gpstring datFileName = tokens[ 1 ];
	gpstring prefix;
	bool autoCreate = false;

	// get options and builds
	typedef std::pair <gpstring, gpstring> BuildPair;
	typedef std::vector <BuildPair> BuildVec;
	BuildVec builds;
	for ( size_t itok = 2 ; itok < tokens.size() ; ++itok )
	{
		gpstring token = tokens[ itok ];
		size_t pos = token.find( '=' );
		if ( pos == gpstring::npos )
		{
			if ( token[ 0 ] == '-' )
			{
				if ( token.same_no_case( "-autocreate" ) )
				{
					autoCreate = true;
				}
				else
				{
					gpwarningf(( "Warning: unrecognized option: '%s'\n", token.c_str() ));
				}
			}
			else
			{
				if ( !prefix.empty() )
				{
					gpwarningf(( "Warning: overriding old prefix of '%s' with '%s'\n",
								 prefix.c_str(), token.c_str() ));
				}
				prefix = token;
			}
		}
		else
		{
			BuildPair build;
			build.first = token.left( pos );
			token.erase( 0, pos + 1 );
			if ( *token.begin() == '\"' )
			{
				token.erase( 0, 1 );
			}
			if ( *token.rbegin() == '\"' )
			{
				token.erase( token.length() - 1, 1 );
			}
			build.second = token;
			builds.push_back( build );
		}
	}

// Process.

	// open output file
	FileSys::File datFile;
	SYSTRYF( datFile.CreateAlways( datFileName ),
			 ( "creating DAT file '%s'", datFileName.c_str() ) );
	ReportSys::FileSink sink( datFile.Release() );
	ReportSys::LocalContext ctx( &sink, false );
	ReportSys::AutoReport autoReport( ctx );

	// for each input file
	BuildVec::const_iterator i, begin = builds.begin(), end = builds.end();
	int written = 0, files = 0;
	for ( i = begin ; i != end ; ++i )
	{
		gpstring resFileName = i->second;
		gpgenericf(( "Processing RES file '%s'\n", resFileName.c_str() ));
		ReportSys::AutoIndent autoIndent( gGenericContext );

		// open input file
		FileSys::MemoryFile resFile;
		bool rc = resFile.Create( resFileName );
		if ( !rc )
		{
			if ( autoCreate )
			{
				gpgeneric( "<Auto-creating...>\n" );

				gpstring resPath( resFileName.c_str(), FileSys::GetPathEnd( resFileName, false ) );
				SYSTRYF( FileSys::CreateFullDirectory( resPath ),
						 ( "auto-creating RES path path '%s'", resPath.c_str() ) );

				FileSys::File forceFile;
				SYSTRYF( forceFile.CreateAlways( resFileName ),
						 ( "auto-creating RES file '%s'", resFileName.c_str() ) );
				FileSys::SetFileTimeAncient( forceFile );
			}
			else
			{
				SYSTRYF( rc, ( "opening RES file '%s'", resFileName.c_str() ) );
			}
			continue;
		}
		++files;

		const void* resBegin = resFile.GetData();
		const void* resEnd   = rcast <const BYTE*> ( resFile.GetData() ) + resFile.GetSize();

		// write build
		if ( !i->first.empty() )
		{
			SYSTRYF( ctx.OutputF( "#if %s\n\n", i->first.c_str() ),
					 ( "writing to DAT file '%s'", datFileName.c_str() ) );
			ctx.Indent();
		}

		// loop and write
		for ( const void* resIter = resBegin ; ((size_t)resEnd - (size_t)resIter) > 2 ; )
		{
			// get header
			ResHeader header;
			resIter = header.Init( resIter );
			if ( resIter == NULL )
			{
				break;
			}

			if ( !header.GetType().IsNull() )
			{
				gpstring typeName = header.GetType().MakeTypeName();
				gpstring idName = header.GetName().MakeIdName();
				gpgenericf(( "Adding '%s' (type is '%s')\n", idName.c_str(), typeName.c_str() ));

				// write out leader
				gpstring macroName = prefix;
				macroName += typeName;
				macroName += '_';
				macroName += idName;
				SYSTRY( ctx.OutputF( "#if %s\n\n", macroName.c_str() ) );
				ctx.Indent();

				// write out dats
				const WORD* wptr = rcast <const WORD*> ( resIter );
				int col = 0;
				for ( int count = header.GetDataSize() ; count != 0 ; count -= 2, ++wptr, ++col )
				{
					if ( col == 8 )
					{
						SYSTRY( ctx.Output( "\n" ) );
						col = 0;
					}

					SYSTRY( ctx.OutputF( "0x%04X, ", *wptr ) );
				}

				// write out trailer
				ctx.Outdent();
				SYSTRY( ctx.OutputF( "%s\n#endif // %s\n\n", (col == 0) ? "" : "\n", macroName.c_str() ) );
				++written;
			}

			// advance
			resIter = header.GetDataEnd();
		}

		// build trailer
		if ( !i->first.empty() )
		{
			ctx.Outdent();
			SYSTRY( ctx.OutputF( "#endif // %s\n\n", i->first.c_str() ) );
		}
	}

	gpgenericf(( "Total of %d resource%s in %d file%s written to DAT file '%s'\n",
				 written, (written == 1) ? "" : "s",
				 files,   (files   == 1) ? "" : "s",
				 datFileName.c_str() ));

// Finish.

	#undef SYSTRYF
	#undef SYSTRY

	// done
	return ( 0 );
}

//////////////////////////////////////////////////////////////////////////////
