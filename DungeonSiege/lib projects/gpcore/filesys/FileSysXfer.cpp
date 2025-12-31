//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysXfer.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "FileSysXfer.h"

#include "StringTool.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// helper methods

bool CalcFilePointer( int& pos, int size, int offset, eSeekFrom origin )
{
	switch ( origin )
	{
		case ( SEEKFROM_BEGIN   ):  /* offset ok as is */     break;
		case ( SEEKFROM_CURRENT ):  offset += pos;            break;
		case ( SEEKFROM_END     ):  offset  = size - offset;  break;

		default:
		{
			gpassert( 0 );  // $ should never get here
			return ( false );
		}
	}

	bool success = true;
	if ( clamp_min_max( offset, 0, size ) )
	{
		success = false;
	}

	pos = offset;
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class StreamWriter implementation

bool StreamWriter :: WriteText( const char* text, int len )
{
	gpassert( text != NULL );
	if ( len == -1 )
	{
		len = ::strlen( text );
	}
	return ( Write( text, len  ) );
}

bool StreamWriter :: WriteArgs( const char* format, va_list args )
{
	auto_dynamic_vsnprintf printer( format, args );
	return ( WriteText( printer, printer.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class FileReader implementation

FileReader :: FileReader( HANDLE handle, int offset, int size, DWORD fileSize )
{
	gpassert( handle != INVALID_HANDLE_VALUE );
	gpassert( offset >= 0 );
	gpassert( size >= 0 );

	m_File     = handle;
	m_Offset   = offset;
	m_Size     = INVALID_SIZE_VALUE;

	if ( fileSize == 0 )
	{
		m_File.GetSize( fileSize );
	}

	if ( fileSize > 0 )
	{
		if ( size == 0 )
		{
			if ( offset <= (int)fileSize )
			{
				m_Size = (int)(fileSize - offset);
			}
		}
		else
		{
			if ( (offset + size) <= (int)fileSize )
			{
				m_Size = size;
			}
		}


		// go to front
		if ( m_Size != INVALID_SIZE_VALUE )
		{
			m_File.Seek( m_Offset );
		}
	}
}

bool FileReader :: Read( void* out, int size, int* bytesRead )
{
	DWORD dummy;
	bool rc = m_File.Read( out, size, dummy );
	if ( bytesRead != NULL )
	{
		*bytesRead = dummy;
	}
	return ( rc );
}

bool FileReader :: ReadLine( gpstring& out )
{
	if ( IsEof() )
	{
		return ( false );
	}

	// this ugly code is just to do some buffering - rewrite at will!

	char chunk[ 50 ];
	do
	{
		// how much can we copy?
		int maxcopy = min_t( (int)ELEMENT_COUNT( chunk ), GetRemaining() );

		// break on error
		int read;
		if ( !Read( chunk, maxcopy, &read ) )
		{
			return ( false );
		}

		// read the line out of the memory buffer
		bool foundEol = false;
		int offset = stringtool::ReadLine( chunk, read, out, &foundEol );

		// found the '\n'?
		if ( foundEol )
		{
			// reset the file pointer (seek backwards) and finish!
			return ( Seek( -(read - offset), SEEKFROM_CURRENT ) );
		}
	}
	while ( !IsEof() );

	// if read anything, it's success
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class MemReader implementation

bool MemReader :: Read( void* out, int size, int* bytesRead )
{
	gpassert( (out != NULL) && (size >= 0) );

	bool success = true;
	if ( clamp_max( size, GetRemaining() ) )
	{
		success = false;
	}

	::memcpy( out, GetCurrentData(), size );
	m_FilePointer += size;

	if ( bytesRead != NULL )
	{
		*bytesRead = size;
	}

	return ( success );
}

bool MemReader :: ReadLine( gpstring& out )
{
	if ( IsEof() )
	{
		return ( false );
	}

	m_FilePointer += stringtool::ReadLine( GetCurrentData(), GetRemaining(), out );
	return ( true );
}

bool MemReader :: Seek( int offset, eSeekFrom origin )
{
	return ( CalcFilePointer( m_FilePointer, GetSize(), offset, origin ) );
}

//////////////////////////////////////////////////////////////////////////////
// class MemWriter implementation

bool MemWriter :: Write( const void* out, int size )
{
	gpassert( (out != NULL) && (size >= 0) );

	bool success = true;
	if ( clamp_max( size, (int)(m_Mem.size - m_FilePointer) ) )
	{
		success = false;
	}

	::memcpy( rcast <BYTE*> ( m_Mem.mem ) + m_FilePointer, out, size );
	m_FilePointer += size;

	return ( success );
}

bool MemWriter :: Seek( int offset, eSeekFrom origin )
{
	return ( CalcFilePointer( m_FilePointer, m_Mem.size, offset, origin ) );
}

//////////////////////////////////////////////////////////////////////////////
// class BufferWriter implementation

bool BufferWriter :: Write( const void* out, int size )
{
	gpassert( size >= 0 );

	// overwrite portion
	int over = (m_Buffer.size() - m_BaseOffset) - m_FilePointer;
	if ( over > 0 )
	{
		int written = min( size, over );
		::memcpy( &*(m_Buffer.begin() + m_BaseOffset + m_FilePointer), out, written );
		size -= written;
		out = rcast <const BYTE*> ( out ) + written;
		m_FilePointer += written;
	}

	// remaining portion
	if ( size > 0 )
	{
		gpassert( out != NULL );
		m_Buffer.insert( m_Buffer.end(), rcast <const BYTE*> ( out ), rcast <const BYTE*> ( out ) + size );
		m_FilePointer = m_Buffer.size();
	}
	return ( true );
}

bool BufferWriter :: Seek( int offset, eSeekFrom origin )
{
	return ( CalcFilePointer( m_FilePointer, m_Buffer.size() - m_BaseOffset, offset, origin ) );
}

//////////////////////////////////////////////////////////////////////////////
// class StringWriter implementation

bool StringWriter :: Write( const void* out, int size )
{
	gpassert( (out != NULL) && (size >= 0) );

	// overwrite portion
	int over = (m_String.length() - m_BaseOffset) - m_FilePointer;
	if ( over > 0 )
	{
		int written = min( size, over );
		::memcpy( m_String.begin_split() + m_BaseOffset + m_FilePointer, out, written );
		size -= written;
		out = rcast <const BYTE*> ( out ) + written;
		m_FilePointer += written;
	}

	// remaining portion
	if ( size > 0 )
	{
		m_String.append( rcast <const char*> ( out ), size );
		m_FilePointer = m_String.length();
	}
	return ( true );
}

bool StringWriter :: Seek( int offset, eSeekFrom origin )
{
	return ( CalcFilePointer( m_FilePointer, m_String.length() - m_BaseOffset, offset, origin ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
