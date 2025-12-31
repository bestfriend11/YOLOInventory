//////////////////////////////////////////////////////////////////////////////
//
// File     :  Filters.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_TankBuilder.h"
#include "Filters.h"

#include "FileSysXfer.h"
#include "RapiImage.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////

void RegisterTankFilters( void )
{
	Tank::Builder::RegisterFilter( ImgFilter    ::FILTER_RAW_MIPMAPS_STRIPTOP, new ImgFilter( ImgFilter::FILTER_RAW_MIPMAPS_STRIPTOP ) );
	Tank::Builder::RegisterFilter( ImgFilter    ::FILTER_RAW_MIPMAPS,          new ImgFilter( ImgFilter::FILTER_RAW_MIPMAPS          ) );
	Tank::Builder::RegisterFilter( ImgFilter    ::FILTER_RAW_PLAIN,            new ImgFilter( ImgFilter::FILTER_RAW_PLAIN            ) );
	Tank::Builder::RegisterFilter( DollarScanner::FILTER_DEFAULT,              new DollarScanner );
}

//////////////////////////////////////////////////////////////////////////////
// class ImgFilter implementation

bool ImgFilter::ms_TrackDupes = false;
ImgFilter::ImgDupDb ImgFilter::ms_ImgDupDb;  

bool ImgFilter :: OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer )
{
	bool success = false;
	bool useMipMaps = (m_Type == FILTER_RAW_MIPMAPS_STRIPTOP) || (m_Type == FILTER_RAW_MIPMAPS);
	bool stripTop   = m_Type == FILTER_RAW_MIPMAPS_STRIPTOP;

	FileSys::BufferWriter writer( m_TempBuffer );
	m_TempBuffer.clear();

	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( fileName, const_mem_ptr( &*buffer.begin(), buffer.size() ), useMipMaps, false ) );
	if ( reader.get() )
	{
#if 0 // $$$$$$
		if ( ms_TrackDupes )
		{
			// get the surface
			std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( fileName, const_mem_ptr( buffer.begin(), buffer.size() ), useMipMaps, false ) );
			RapiMemImage image;
			reader->GetNextSurface( image, false );

			// get a new entry
			ImgColl::iterator entryi = *ms_ImgColl.insert( ms_ImgColl.end() );
			ImgEntry& entry = *entryi;
			DWORD* crci = entry.m_Crcs;

			// square?
			entry.m_Square = image.IsSquare();

			// crc and rotate
			if ( entry.m_Square )
			{
				for ( int i = 0 ; i < 4 ; ++i )
				{
					*crci = image.CalcCrc32();
					ms_ImgDupDb.insert( std::make_pair( *crci, entryi ) );
					++crci;
					image.RotateClockwise();
				}

				image.FlipVertical();

				for ( int i = 0 ; ; )
				{
					*crci = image.CalcCrc32();
					ms_ImgDupDb.insert( std::make_pair( *crci, entryi ) );
					++crci;

					if ( ++i == 4 )
					{
						break;
					}

					image.RotateClockwise();
				}
			}
			else
			{
				for ( int i = 0 ; ; )
				{
					*crci = image.CalcCrc32();
					ms_ImgDupDb.insert( std::make_pair( *crci, entryi ) );
					++crci;
					image.FlipVertical();

					*crci = image.CalcCrc32();
					ms_ImgDupDb.insert( std::make_pair( *crci, entryi ) );
					++crci;

					if ( ++i == 2 )
					{
						break;
					}

					image.FlipHorizontal();
				}
			}
		}
#endif // 0 $$$$$$

		// strip off mip 0 if (a) it's been requested and (b) it's large enough
		// to be worth worrying about
		if ( useMipMaps && stripTop && (reader->GetWidth() >= 16) && (reader->GetHeight() >= 16) )
		{
			reader->SkipNextSurface();
		}

		if ( WriteImage( writer, IMAGE_RAW, reader.get(), false, useMipMaps ) )
		{
			m_TempBuffer.swap( buffer );
			stringtool::ReplaceExtension( newName, gpstring( "." ) + ::GetExtension( IMAGE_RAW ) );
			success = true;
		}
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class DollarScanner implementation

bool DollarScanner :: OnFilter( const gpstring& fileName, gpstring& /*newName*/, Buffer& buffer )
{
	FileSys::MemReader reader( const_mem_ptr( &*buffer.begin(), buffer.size() ) );

	gpstring text;
	for ( int line = 1 ; reader.ReadLine( text ) ; ++line )
	{
		if ( text.find( "$$$" ) != gpstring::npos )
		{
			gpwarningf(( "$$$ found, this must not ship! File = '%s', line = %d\n", fileName.c_str(), line ));
		}
		text.clear();
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
