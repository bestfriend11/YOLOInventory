//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiPersistImpl.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains specific implementations of readers and writers for
//             use by persistence class.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBIPERSISTIMPL_H
#define __FUBIPERSISTIMPL_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiPersist.h"
#include "FuBiSchemaImpl.h"
#include "Fuel.h"
#include "GpColl.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace FileSys
{
	class Writer;
	class Reader;
}

namespace ReportSys
{
	class LocalContext;
}

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helper interface declarations

struct TextPersistWriter : PersistWriter
{
	virtual bool AsText( void );
	virtual bool WriteBinary( const char* key, const_mem_ptr ptr );

	// derived must implement these
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type ) = 0;
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value ) = 0;
};

struct BinaryPersistWriter : PersistWriter
{
	virtual bool AsText( void );
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type );
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value );

	// derived must implement these
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool WriteBinary( const char* key, const_mem_ptr ptr ) = 0;
};

struct TextPersistReader : PersistReader
{
	virtual bool AsText( void );
	virtual bool ReadBinary( const char* key, mem_ptr ptr );

	// derived must implement these
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value ) = 0;
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value ) = 0;
};

struct BinaryPersistReader : PersistReader
{
	virtual bool AsText( void );
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value );
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value );

	// derived must implement these
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool ReadBinary( const char* key, mem_ptr ptr ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
// class ReportWriter declaration

class ReportWriter : public TextPersistWriter
{
public:
	SET_INHERITED( ReportWriter, TextPersistWriter );

	ReportWriter( ReportSys::ContextRef ctx = NULL );
	virtual ~ReportWriter( void );

	virtual bool EnterBlock( const char* name );
	virtual bool LeaveBlock( void );
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type );
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value );

private:
	ReportSys::ContextRef m_Context;
	int m_Level;

	SET_NO_COPYING( ReportWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class GasWriter declaration

// this outputs a gas file directly

class GasWriter : public TextPersistWriter
{
public:
	SET_INHERITED( GasWriter, TextPersistWriter );

	GasWriter( FileSys::StreamWriter& writer );
	virtual ~GasWriter( void );

	virtual bool EnterBlock( const char* name );
	virtual bool LeaveBlock( void );
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type );
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value );

private:
	my ReportSys::LocalContext* m_Context;

	SET_NO_COPYING( GasWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class FuelWriter declaration

class FuelWriter : public TextPersistWriter
{
public:
	SET_INHERITED( FuelWriter, TextPersistWriter );

	FuelWriter( FuelHandle fh )							{  m_Blocks.push( fh );  }
	virtual ~FuelWriter( void )							{  }

	virtual bool EnterBlock( const char* name );
	virtual bool LeaveBlock( void );
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type );
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value );

	static eFuelValueProperty GetFuelType( eXfer xfer, eVarType fubiType );

private:
	typedef std::stack <FuelHandle, stdx::fast_vector <FuelHandle> > BlockStack;

	BlockStack m_Blocks;

	SET_NO_COPYING( FuelWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class FuelReader declaration

class FuelReader : public TextPersistReader
{
public:
	SET_INHERITED( FuelReader, TextPersistReader );

	FuelReader( FuelHandle fh )							{  m_Blocks.push( fh );  }
	virtual ~FuelReader( void )							{  }

	virtual bool EnterBlock( const char* name );
	virtual bool LeaveBlock( void );
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value );
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value );

private:
	typedef std::stack <FuelHandle, stdx::fast_vector <FuelHandle> > BlockStack;

	BlockStack m_Blocks;

	SET_NO_COPYING( FuelReader );
};

//////////////////////////////////////////////////////////////////////////////
// class FastFuelReader declaration

class FastFuelReader : public TextPersistReader
{
public:
	SET_INHERITED( FastFuelReader, TextPersistReader );

	FastFuelReader( FastFuelHandle fh )					{  m_Blocks.push( fh );  }
	virtual ~FastFuelReader( void )						{  }

	virtual bool EnterBlock( const char* name );
	virtual bool LeaveBlock( void );
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value );
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value );

private:
	typedef std::stack <FastFuelHandle, stdx::fast_vector <FastFuelHandle> > BlockStack;

	BlockStack m_Blocks;

	SET_NO_COPYING( FastFuelReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RecordReader declaration

class RecordReader : public BinaryPersistReader
{
public:
	SET_INHERITED( RecordReader, BinaryPersistReader );

	RecordReader( const Record& record )				: m_Record( record )  {  }
	virtual ~RecordReader( void )						{  }

	virtual bool EnterBlock( const char* /*name*/ )		{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool LeaveBlock( void )						{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value );
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value );
	virtual bool ReadBinary( const char* key, mem_ptr ptr );

private:
	const Record& m_Record;

	SET_NO_COPYING( RecordReader );
};

//////////////////////////////////////////////////////////////////////////////
// class RecordWriter declaration

class RecordWriter : public BinaryPersistWriter
{
public:
	SET_INHERITED( RecordWriter, BinaryPersistWriter );

	RecordWriter( Record& record )						: m_Record( record )  {  }
	virtual ~RecordWriter( void )						{  }

	virtual bool EnterBlock( const char* /*name*/ )		{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool LeaveBlock( void )						{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type );
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value );
	virtual bool WriteBinary( const char* key, const_mem_ptr ptr );

private:
	Record& m_Record;

	SET_NO_COPYING( RecordWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class SerialBinaryReader declaration

// $ special usage: for serial packing and unpacking of data. ignores all
//   keynames - writer must match reader exactly. no blocks allowed.

class SerialBinaryReader : public BinaryPersistReader
{
public:
	SET_INHERITED( SerialBinaryReader, BinaryPersistReader );

	SerialBinaryReader( const_mem_ptr buffer )			: m_Buffer( buffer )  {  }
	virtual ~SerialBinaryReader( void )					{  }

	virtual bool EnterBlock( const char* /*name*/ )		{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool LeaveBlock( void )						{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool ReadBinary( const char* key, mem_ptr ptr );

private:
	const_mem_ptr m_Buffer;

	SET_NO_COPYING( SerialBinaryReader );
};

//////////////////////////////////////////////////////////////////////////////
// class SerialBinaryWriter declaration

// $ special usage: for serial packing and unpacking of data. ignores all
//   keynames - writer must match reader exactly. no blocks allowed.

class SerialBinaryWriter : public BinaryPersistWriter
{
public:
	SET_INHERITED( SerialBinaryWriter, BinaryPersistWriter );

	SerialBinaryWriter( void )							{  }
	virtual ~SerialBinaryWriter( void )					{  }

	virtual bool EnterBlock ( const char* /*name*/ )	{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool LeaveBlock ( void )					{  gpassertm( 0, "Not allowed!" );  return ( false );  }
	virtual bool WriteBinary( const char* key, const_mem_ptr ptr );

	const_mem_ptr GetBuffer    ( void ) const			{  return ( const_mem_ptr( &*m_Buffer.begin(), m_Buffer.size() ) );  }
	const_mem_ptr ReleaseBuffer( void*& freeThisPtr )	{  return ( m_Buffer.release_as_mem( freeThisPtr ) );  }

private:
	stdx::fast_vector <BYTE> m_Buffer;

	SET_NO_COPYING( SerialBinaryWriter );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBIPERSISTIMPL_H

//////////////////////////////////////////////////////////////////////////////
