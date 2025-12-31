//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTraitsImpl.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiTraitsImpl.h"

#include "FuBiPersist.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_EXPORT_POD_TYPE( POINT );
FUBI_EXPORT_POD_TYPE( SIZE );
FUBI_EXPORT_POD_TYPE( RECT );
FUBI_EXPORT_POD_TYPE( GUID );
FUBI_EXPORT_POD_TYPE( SYSTEMTIME );
FUBI_EXPORT_POD_TYPE( vector_3 );
FUBI_EXPORT_POD_TYPE( Quat );
FUBI_EXPORT_POD_TYPE( Point2 );

//////////////////////////////////////////////////////////////////////////////
// Windows types implementations

template <> void FuBi::Traits <POINT> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%x,0x%x" : "%d,%d",
				 obj.x, obj.y );
}

template <> bool FuBi::Traits <POINT> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str, "%i , %i", &obj.x, &obj.y ) == 2 );
}

template <> void FuBi::Traits <SIZE> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%x,0x%x" : "%d,%d",
				 obj.cx, obj.cy );
}

template <> bool FuBi::Traits <SIZE> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str, "%i , %i", &obj.cx, &obj.cy ) == 2 );
}

template <> void FuBi::Traits <RECT> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%x,0x%x,0x%x,0x%x" : "%d,%d,%d,%d",
				 obj.left, obj.top, obj.right, obj.bottom );
}

template <> bool FuBi::Traits <RECT> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str, "%i , %i , %i , %i", &obj.left, &obj.top, &obj.right, &obj.bottom ) == 4 );
}

template <> void FuBi::Traits <GUID> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	stringtool::ToString( out, obj );
}

template <> bool FuBi::Traits <GUID> :: FromString( const char* str, Type& obj )
{
	return ( stringtool::FromString( str, obj ) );
}

template <> void FuBi::Traits <SYSTEMTIME> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf(
			"%02d/%02d(%d)/%04d,%02d:%02d:%02d.%d",
			obj.wMonth, obj.wDay, obj.wDayOfWeek, obj.wYear,
			obj.wHour, obj.wMinute, obj.wSecond, obj.wMilliseconds );
}

template <> bool FuBi::Traits <SYSTEMTIME> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str,
			"%hd / %hd ( %hd ) / %hd , %hd : %hd : %hd.%hd",
			&obj.wMonth, &obj.wDay, &obj.wDayOfWeek, &obj.wYear,
			&obj.wHour, &obj.wMinute, &obj.wSecond, &obj.wMilliseconds ) == 8 );
}

//////////////////////////////////////////////////////////////////////////////
// math types implementations

template <> void FuBi::Traits <vector_3> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g,%g", obj.x, obj.y, obj.z );
}

template <> bool FuBi::Traits <vector_3> :: FromString( const char* str, Type& obj )
{
	if ( ::sscanf( str, "%f , %f , %f", &obj.x, &obj.y, &obj.z ) == 3 )
	{
		return ( true );
	}
	else if ( ::sscanf( str, "%f", &obj.x ) == 1 )
	{
		obj.z = obj.y = obj.x;
		return ( true );
	}
	return ( false );
}

template <> void FuBi::Traits <Quat> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g,%g,%g", obj.m_x, obj.m_y, obj.m_z, obj.m_w );
}

template <> bool FuBi::Traits <Quat> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str, "%f , %f , %f , %f", &obj.m_x, &obj.m_y, &obj.m_z, &obj.m_w ) == 4 );
}

template <> bool FuBi::Traits <matrix_3x3> :: XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
{
	bool success = persist.EnterBlock( key );
	if ( success )
	{
		persist.Xfer( "0", rcast <vector_3&> ( obj.v00 ) );
		persist.Xfer( "1", rcast <vector_3&> ( obj.v10 ) );
		persist.Xfer( "2", rcast <vector_3&> ( obj.v20 ) );
	}
	persist.LeaveBlock();

	return ( success );
}

template <> void FuBi::Traits <Point2> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g", obj.x, obj.y );
}

template <> bool FuBi::Traits <Point2> :: FromString( const char* str, Type& obj )
{
	return ( ::sscanf( str, "%f , %f", &obj.x, &obj.y ) == 2 );
}

template <> bool FuBi::Traits <OBB2> :: XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
{
	bool success = persist.EnterBlock( key );
	if ( success )
	{
		persist.Xfer( "m_center",    obj.m_center    );
		persist.Xfer( "m_half_diag", obj.m_half_diag );
		persist.Xfer( "m_orient",    obj.m_orient    );
	}
	persist.LeaveBlock();

	return ( success );
}

template <> void FuBi::Traits <gpversion> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out = ::ToString( obj, gpversion::MODE_COMPLETE );
}

//////////////////////////////////////////////////////////////////////////////
