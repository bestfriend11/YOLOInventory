//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiPersistImpl.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiPersistImpl.h"

#include "FileSysDefs.h"
#include "ReportSys.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helper interface implementations

bool TextPersistWriter :: AsText( void )
{
	return ( true );
}

bool TextPersistWriter :: WriteBinary( const char* key, const_mem_ptr ptr )
{
	gpstring str;
	UuEncode( ptr.mem, ptr.size, str );
	return ( WriteString( XFER_QUOTED, key, str, VAR_UNKNOWN ) );
}

bool BinaryPersistWriter :: AsText( void )
{
	return ( false );
}

bool BinaryPersistWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpstring& value, eVarType /*type*/ )
{
	DWORD length = value.length();
	return (   WriteBinary( key, make_const_mem_ptr( length ) )
			&& WriteBinary( key, const_mem_ptr( value.c_str(), value.length() ) ) );
}

bool BinaryPersistWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpwstring& value )
{
	DWORD length = value.length();
	return (   WriteBinary( key, make_const_mem_ptr( length ) )
			&& WriteBinary( key, const_mem_ptr( value.c_str(), value.length() * 2 ) ) );
}

bool TextPersistReader :: AsText( void )
{
	return ( true );
}

bool TextPersistReader :: ReadBinary( const char* key, mem_ptr ptr )
{
	// read the uuencoded text
	gpstring str;
	if ( !ReadString( XFER_QUOTED, key, str ) )
	{
		return ( false );
	}

	// uudecode it
	return ( UuDecode( str, ptr.mem, ptr.size ) == (int)ptr.size );
}

bool BinaryPersistReader :: AsText( void )
{
	return ( false );
}

bool BinaryPersistReader :: ReadString( eXfer /*xfer*/, const char* key, gpstring& value )
{
	DWORD length;
	if ( !ReadBinary( key, make_mem_ptr( length ) ) )
	{
		return ( false );
	}
	value.resize( length );
	return ( ReadBinary( key, mem_ptr( value.begin_split(), length ) ) );
}

bool BinaryPersistReader :: ReadString( eXfer /*xfer*/, const char* key, gpwstring& value )
{
	DWORD length;
	if ( !ReadBinary( key, make_mem_ptr( length ) ) )
	{
		return ( false );
	}
	value.resize( length );
	return ( ReadBinary( key, mem_ptr( value.begin_split(), length * 2 ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class ReportWriter implementation

ReportWriter :: ReportWriter( ReportSys::ContextRef ctx )
	: m_Context( ctx )
{
	m_Level = 0;
}

ReportWriter :: ~ReportWriter( void )
{
	// this space intentionally left blank...
}

bool ReportWriter :: EnterBlock( const char* name )
{
	m_Context->OutputF( "[%s]\n", name );
	if ( m_Level++ > 0 )
	{
		m_Context->Indent();
	}
	return ( true );
}

bool ReportWriter :: LeaveBlock( void )
{
	if ( --m_Level > 0 )
	{
		m_Context->Outdent();
	}
	return ( true );
}

bool ReportWriter :: WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType /*type*/ )
{
	gpassert( key != NULL );
	return ( m_Context->OutputF( (xfer == XFER_QUOTED) ? "%s = \"%s\"\n" : "%s = %s\n", key, value.c_str() ) );
}

bool ReportWriter :: WriteString( eXfer xfer, const char* key, const gpwstring& value )
{
	gpassert( key != NULL );
	return ( m_Context->OutputF( (xfer == XFER_QUOTED) ? "%s = \"%s\"\n" : "%s = %s\n", key, ::ToAnsi( value ).c_str() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class GasWriter implementation

GasWriter :: GasWriter( FileSys::StreamWriter& writer )
{
	m_Context = new ReportSys::LocalContext( new ReportSys::FileSysStreamerSink( writer ), true );
}

GasWriter :: ~GasWriter( void )
{
	delete ( m_Context );
}

bool GasWriter :: EnterBlock( const char* name )
{
	m_Context->Output( "[" );
	m_Context->Output( name );
	m_Context->Output( "]\n{\n" );
	m_Context->Indent();
	return ( true );
}

bool GasWriter :: LeaveBlock( void )
{
	m_Context->Outdent();
	m_Context->Output( "}\n" );
	return ( true );
}

bool GasWriter :: WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType /*type*/ )
{
	bool rc = true;

	rc &= m_Context->Output( key );
	rc &= m_Context->Output( " = " );

	if ( xfer == XFER_QUOTED )
	{
		rc &= m_Context->Output( "\"" );
	}

	m_Context->Output( value );

	if ( xfer == XFER_QUOTED )
	{
		rc &= m_Context->Output( "\"" );
	}

	rc &= m_Context->Output( ";\n" );

	return ( rc );
}

bool GasWriter :: WriteString( eXfer xfer, const char* key, const gpwstring& value )
{
	return ( WriteString( xfer, key, ::UnicodeToUtf8( value ), VAR_STRING ) );
}

//////////////////////////////////////////////////////////////////////////////
// class FuelWriter implementation

bool FuelWriter :: EnterBlock( const char* name )
{
	m_Blocks.push( m_Blocks.top()->GetChildBlock( name, true ) );
	return ( true );
}

bool FuelWriter :: LeaveBlock( void )
{
	gpassert( m_Blocks.size() > 1 );
	m_Blocks.pop();
	return ( true );
}

bool FuelWriter :: WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	return ( m_Blocks.top()->Add( key, value, GetFuelType( xfer, type ) ) );
}

bool FuelWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpwstring& value )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	return ( m_Blocks.top()->Add( key, value ) );
}

eFuelValueProperty FuelWriter :: GetFuelType( eXfer xfer, eVarType fubiType )
{
	eFuelValueProperty fuelType = FVP_STRING;

	if ( xfer == XFER_QUOTED )
	{
		fuelType = FVP_QUOTED_STRING;
	}
	else if ( xfer != XFER_FOURCC )
	{
		switch ( fubiType )
		{
			case ( VAR_INT ):
				fuelType = (xfer == XFER_HEX) ? FVP_HEX_INT : FVP_NORMAL_INT;
				break;

			case ( VAR_UINT ):
				// $$$
				break;

			case ( VAR_FLOAT ):
				fuelType = FVP_FLOAT;
				break;

			case ( VAR_DOUBLE ):
				fuelType = FVP_DOUBLE;
				break;

			case ( VAR_BOOL ):
				fuelType = FVP_BOOL;
				break;

			default:
			{
				if ( IsUser( fubiType ) )
				{
					static const ClassSpec* const SPEC_SIEGEPOSDATA = gFuBiSysExports.FindSiegePosData();
					static const ClassSpec* const SPEC_SIEGEPOS     = gFuBiSysExports.FindSiegePos();
					static const ClassSpec* const SPEC_QUAT         = gFuBiSysExports.FindQuat();
					static const ClassSpec* const SPEC_VECTOR3      = gFuBiSysExports.FindVector3();

					gpassert( SPEC_SIEGEPOSDATA != NULL );
					gpassert( SPEC_SIEGEPOS     != NULL );
					gpassert( SPEC_QUAT         != NULL );
					gpassert( SPEC_VECTOR3      != NULL );

					const ClassSpec* spec = gFuBiSysExports.FindClass( fubiType );
					if ( (spec == SPEC_SIEGEPOSDATA) || (spec == SPEC_SIEGEPOS) )
					{
						fuelType = FVP_SIEGE_POS;
					}
					else if ( spec == SPEC_QUAT )
					{
						fuelType = FVP_QUAT;
					}
					else if ( spec == SPEC_VECTOR3 )
					{
						fuelType = FVP_VECTOR_3;
					}
					else if ( spec->m_Flags & ClassSpec::FLAG_POINTERCLASS )
					{
						fuelType = FVP_HEX_INT;
					}
				}
			}
		}
	}

	return ( fuelType );
}
//////////////////////////////////////////////////////////////////////////////
// class FuelReader implementation

bool FuelReader :: EnterBlock( const char* name )
{
	FuelHandle fh = m_Blocks.top()->GetChildBlock( name );
	if ( fh.IsValid() )
	{
		m_Blocks.push( fh );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

bool FuelReader :: LeaveBlock( void )
{
	gpassert( m_Blocks.size() > 1 )
	m_Blocks.pop();
	return ( true );
}

bool FuelReader :: ReadString( eXfer /*xfer*/, const char* key, gpstring& value )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	// get the value
	return ( m_Blocks.top()->Get( key, value ) != FVP_NONE );
}

bool FuelReader :: ReadString( eXfer /*xfer*/, const char* key, gpwstring& value )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	// get the value
	return ( m_Blocks.top()->Get( key, value ) != FVP_NONE );
}

//////////////////////////////////////////////////////////////////////////////
// class FastFuelReader implementation

bool FastFuelReader :: EnterBlock( const char* name )
{
	FastFuelHandle fh = m_Blocks.top().GetChildNamed( name );
	if ( fh.IsValid() )
	{
		m_Blocks.push( fh );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

bool FastFuelReader :: LeaveBlock( void )
{
	gpassert( m_Blocks.size() > 1 )
	m_Blocks.pop();
	return ( true );
}

bool FastFuelReader :: ReadString( eXfer /*xfer*/, const char* key, gpstring& value )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	// get the value
	return ( m_Blocks.top().Get( key, value ) != FVP_NONE );
}

bool FastFuelReader :: ReadString( eXfer /*xfer*/, const char* key, gpwstring& value )
{
	gpassert( !m_Blocks.empty() );
	gpassert( key != NULL );

	// get the value
	return ( m_Blocks.top().Get( key, value ) != FVP_NONE );
}

//////////////////////////////////////////////////////////////////////////////
// class RecordReader implementation

bool RecordReader :: ReadString( eXfer /*xfer*/, const char* key, gpstring& value )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	return ( m_Record.GetAsString( column, value ) );
}

bool RecordReader :: ReadString( eXfer /*xfer*/, const char* key, gpwstring& value )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	return ( m_Record.Get( column, value ) );
}

bool RecordReader :: ReadBinary( const char* key, mem_ptr ptr )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	::memcpy( ptr.mem, m_Record.OnGetFieldPtr( column ), ptr.size );
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class RecordWriter implementation

bool RecordWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpstring& value, eVarType /*type*/ )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	return ( m_Record.SetAsString( column, value ) );
}

bool RecordWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpwstring& value )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	return ( m_Record.Set( column, value ) );
}

bool RecordWriter :: WriteBinary( const char* key, const_mem_ptr ptr )
{
	int column = m_Record.GetHeaderSpec()->FindColumn( key );
	if ( column == -1 )
	{
		return ( false );
	}

	::memcpy( m_Record.OnGetFieldPtr( column ), ptr.mem, ptr.size );
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class SerialBinaryReader implementation

bool SerialBinaryReader :: ReadBinary( const char* /*key*/, mem_ptr ptr )
{
	// validate
	if_not_gpassert( ptr.size <= m_Buffer.size )
	{
		return ( false );
	}

	// copy
	::memcpy( ptr.mem, m_Buffer.mem, ptr.size );

	// advance
	m_Buffer.mem = rcast <const BYTE*> ( m_Buffer.mem ) + ptr.size;
	m_Buffer.size -= ptr.size;

	// ok
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class SerialBinaryWriter implementation

bool SerialBinaryWriter :: WriteBinary( const char* /*key*/, const_mem_ptr ptr )
{
	// serialize
	const BYTE* data = rcast <const BYTE*> ( ptr.mem );
	m_Buffer.insert( m_Buffer.end(), data, data + ptr.size );

	// ok
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
