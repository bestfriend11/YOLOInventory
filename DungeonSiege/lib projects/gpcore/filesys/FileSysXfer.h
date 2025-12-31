//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysXfer.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes for transferring data to and from an
//             abstracted concept of storage.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSXFER_H
#define __FILESYSXFER_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSys.h"
#include "FileSysUtils.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class FileHandleReader declaration

// read from a FileHandle

class FileHandleReader : public Reader
{
public:
	SET_INHERITED( FileHandleReader, Reader );

	FileHandleReader( FileHandle handle, bool own = false )					: m_Handle( handle )  {  m_Owned = own;  }

	virtual ~FileHandleReader( void )
	{
		if ( m_Owned )
		{
			m_Handle.Close();
		}
	}

// Overrides.

	virtual bool Read    ( void* out, int size, int* bytesRead = NULL )		{  return ( m_Handle.Read( out, size, bytesRead ) );  }
	virtual bool ReadLine( gpstring& out )									{  return ( m_Handle.ReadLine( out ) );  }
	virtual bool Seek    ( int offset, eSeekFrom origin = SEEKFROM_BEGIN )	{  return ( m_Handle.Seek( offset, origin ) );  }
	virtual int  GetPos  ( void )											{  return ( m_Handle.GetPos() );  }
	virtual int  GetSize ( void )											{  return ( m_Handle.GetSize() );  }

private:
	bool       m_Owned;
	FileHandle m_Handle;

	SET_NO_COPYING( FileHandleReader );
};

//////////////////////////////////////////////////////////////////////////////
// class FileReader declaration

// read from a Win32 file

class FileReader : public Reader
{
public:
	SET_INHERITED( FileReader, Reader );

	FileReader( HANDLE handle, int offset = 0, int size = 0, DWORD fileSize = 0 );
	virtual ~FileReader( void )												{  m_File.Release();  }

// Overrides.

	virtual bool Read    ( void* out, int size, int* bytesRead = NULL );
	virtual bool ReadLine( gpstring& out );
	virtual bool Seek    ( int offset, eSeekFrom origin = SEEKFROM_BEGIN )	{  return ( m_File.Seek( m_Offset + offset, origin ) );  }
	virtual int  GetPos  ( void )											{  DWORD pos;  return ( m_File.GetPos( pos ) ? (scast <int> ( pos ) - m_Offset) : INVALID_POS_VALUE );  }
	virtual int  GetSize ( void )											{  return ( m_Size );  }

private:
	File m_File;
	int  m_Offset;
	int  m_Size;

	SET_NO_COPYING( FileReader );
};

//////////////////////////////////////////////////////////////////////////////
// class MemReader declaration

// read from a memory buffer

class MemReader : public Reader
{
public:
	SET_INHERITED( MemReader, Reader );

	MemReader( const_mem_ptr mem )											: m_Mem( mem )  {  m_FilePointer = 0;  }
	virtual ~MemReader( void )												{  }

	const void* GetData       ( void ) const								{  return ( m_Mem.mem );  }
	const void* GetCurrentData( void ) const								{  return ( rcast <const BYTE*> ( m_Mem.mem ) + m_FilePointer );  }

// Overrides.

	virtual bool Read    ( void* out, int size, int* bytesRead = NULL );
	virtual bool ReadLine( gpstring& out );
	virtual bool Seek    ( int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int  GetPos  ( void )											{  return ( m_FilePointer );  }
	virtual int  GetSize ( void )											{  return ( m_Mem.size );  }

private:
	const_mem_ptr m_Mem;
	int           m_FilePointer;

	SET_NO_COPYING( MemReader );
};

//////////////////////////////////////////////////////////////////////////////
// class ReportWriter declaration

class ReportWriter : public StreamWriter
{
public:
	SET_INHERITED( ReportWriter, StreamWriter );

	ReportWriter( ReportSys::ContextRef ctx = NULL )						: m_Context( ctx )  {  }
	virtual ~ReportWriter( void )											{  }

// Overrides.

	virtual bool Write    ( const void* out, int size )						{  return ( ReportSys::OutputRaw( m_Context, rcast <const char*> ( out ), size ) );  }
	virtual bool WriteText( const char* text, int len = -1 )				{  return ( ReportSys::Output( m_Context, text, len ) );  }
	virtual bool WriteArgs( const char* format, va_list args )				{  return ( ReportSys::OutputArgs( m_Context, format, args ) );  }

private:
	ReportSys::ContextRef m_Context;

	SET_NO_COPYING( ReportWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class FileWriter declaration

class FileWriter : public Writer
{
public:
	SET_INHERITED( FileWriter, Writer );

	FileWriter( HANDLE handle, bool owns = false )							{  m_File = handle;  m_Owns = owns;  }
	virtual ~FileWriter( void )												{  if ( !m_Owns )  {  m_File.Release();  }  }

// Overrides.

	virtual bool Write    ( const void* out, int size )						{  return ( m_File.Write( out, size ) );  }
	virtual bool WriteText( const char* text, int len = -1 )				{  return ( m_File.WriteText( text, len ) );  }
	virtual bool WriteArgs( const char* format, va_list args )				{  return ( m_File.WriteArgs( format, args ) );  }
	virtual bool Seek    ( int offset, eSeekFrom origin = SEEKFROM_BEGIN )	{  return ( m_File.Seek( offset, origin ) );  }
	virtual int  GetPos  ( void )											{  DWORD pos;  return ( m_File.GetPos( pos ) ? scast <int> ( pos ) : INVALID_POS_VALUE );  }

private:
	File m_File;
	bool m_Owns;

	SET_NO_COPYING( FileWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class MemWriter declaration

class MemWriter : public Writer
{
public:
	SET_INHERITED( MemWriter, Writer );

	MemWriter( mem_ptr mem )												: m_Mem( mem )  {  m_FilePointer = 0;  }
	virtual ~MemWriter( void )												{  }

// Overrides.

	virtual bool Write    ( const void* out, int size );
	virtual bool Seek     ( int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int  GetPos   ( void )											{  return ( m_FilePointer );  }

private:
	mem_ptr m_Mem;
	int     m_FilePointer;

	SET_NO_COPYING( MemWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class BufferWriter declaration

class BufferWriter : public Writer
{
public:
	SET_INHERITED( BufferWriter, Writer );
	typedef stdx::fast_vector <BYTE> Buffer;

	BufferWriter( Buffer& buffer, bool clearBuffer = false )				
		: m_Buffer( buffer )
	{
		if ( clearBuffer )
		{
			m_Buffer.clear();
		}

		m_FilePointer = 0;
		m_BaseOffset = m_Buffer.size();
	}

	virtual ~BufferWriter( void )											{  }

// Overrides.

	virtual bool Write    ( const void* out, int size );
	virtual bool Seek     ( int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int  GetPos   ( void )											{  return ( m_FilePointer );  }

private:
	Buffer& m_Buffer;
	int     m_FilePointer;
	int     m_BaseOffset;

	SET_NO_COPYING( BufferWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class StringWriter declaration

class StringWriter : public Writer
{
public:
	SET_INHERITED( StringWriter, Writer );

	StringWriter( gpstring& out )											: m_String( out )  {  m_FilePointer = 0;  m_BaseOffset = m_String.length();  }
	virtual ~StringWriter( void )											{  }

// Overrides.

	virtual bool Write    ( const void* out, int size );
	virtual bool Seek     ( int offset, eSeekFrom origin = SEEKFROM_BEGIN );
	virtual int  GetPos   ( void )											{  return ( m_FilePointer );  }

private:
	gpstring& m_String;
	int       m_FilePointer;
	int       m_BaseOffset;

	SET_NO_COPYING( StringWriter );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSXFER_H

//////////////////////////////////////////////////////////////////////////////
