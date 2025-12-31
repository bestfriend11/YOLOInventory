//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTraitsImpl.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains special traits for custom types.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBITRAITSIMPL_H
#define __FUBITRAITSIMPL_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "FuBiTraits.h"
#include "Vector_3.h"
#include "Matrix_3x3.h"
#include "Quat.h"
#include "Space_2.h"

//////////////////////////////////////////////////////////////////////////////
// Windows types declarations

#define FUBI_CAST_VARIABLE( T, NAME ) \
	FEX T    Get##NAME( void ) const  {  return ( rcast <const CastType*> ( this )->NAME );  } \
	FEX void Set##NAME( T v  )        {  rcast <CastType*> ( this )->NAME = v;  }

FUBI_DECLARE_POD_TRAITS( POINT )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );

	typedef POINT CastType;

	DECLARE_CLASS_TABLE_BEGIN( Type )
		CLASS_TABLE_ADD_COLUMN( "x", x );
		CLASS_TABLE_ADD_COLUMN( "y", y );
	DECLARE_CLASS_TABLE_END()

	FUBI_CAST_VARIABLE( LONG, x );
	FUBI_CAST_VARIABLE( LONG, y );
};

FUBI_DECLARE_POD_TRAITS( SIZE )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );

	typedef SIZE CastType;

	DECLARE_CLASS_TABLE_BEGIN( Type )
		CLASS_TABLE_ADD_COLUMN( "cx", cx );
		CLASS_TABLE_ADD_COLUMN( "cy", cy );
	DECLARE_CLASS_TABLE_END()

	FUBI_CAST_VARIABLE( LONG, cx );
	FUBI_CAST_VARIABLE( LONG, cy );
};

FUBI_DECLARE_POD_TRAITS( RECT )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );

	typedef RECT CastType;

	DECLARE_CLASS_TABLE_BEGIN( Type )
		CLASS_TABLE_ADD_COLUMN( "left",   left   );
		CLASS_TABLE_ADD_COLUMN( "top",    top    );
		CLASS_TABLE_ADD_COLUMN( "right",  right  );
		CLASS_TABLE_ADD_COLUMN( "bottom", bottom );
	DECLARE_CLASS_TABLE_END()

	FUBI_CAST_VARIABLE( LONG, left   );
	FUBI_CAST_VARIABLE( LONG, top    );
	FUBI_CAST_VARIABLE( LONG, right  );
	FUBI_CAST_VARIABLE( LONG, bottom );
};

FUBI_DECLARE_POD_TRAITS( GUID )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );
};

FUBI_DECLARE_POD_TRAITS( SYSTEMTIME )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );
};

FUBI_DECLARE_CAST_TRAITS( FILETIME, INT64 );

#undef FUBI_CAST_VARIABLE

//////////////////////////////////////////////////////////////////////////////
// winx types declarations

struct GPoint;  template <> struct FuBi::Traits <GPoint> : FuBi::Traits <POINT>  {  typedef GPoint Type;  FUBI_CLASS_INHERIT( GPoint, POINT );  };
struct GSize ;  template <> struct FuBi::Traits <GSize > : FuBi::Traits <SIZE >  {  typedef GSize  Type;  FUBI_CLASS_INHERIT( GSize,  SIZE  );  };
struct GRect ;  template <> struct FuBi::Traits <GRect > : FuBi::Traits <RECT >  {  typedef GRect  Type;  FUBI_CLASS_INHERIT( GRect,  RECT  );  };

//////////////////////////////////////////////////////////////////////////////
// math types declarations

FUBI_DECLARE_POD_TRAITS( vector_3 )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );

	DECLARE_CLASS_TABLE_BEGIN( Type )
		CLASS_TABLE_ADD_COLUMN( "x", x );
		CLASS_TABLE_ADD_COLUMN( "y", y );
		CLASS_TABLE_ADD_COLUMN( "z", z );
	DECLARE_CLASS_TABLE_END()
};

FUBI_DECLARE_POD_TRAITS( Quat )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );

	DECLARE_CLASS_TABLE_BEGIN( Type )
		CLASS_TABLE_ADD_COLUMN( "x", m_x );
		CLASS_TABLE_ADD_COLUMN( "y", m_y );
		CLASS_TABLE_ADD_COLUMN( "z", m_z );
		CLASS_TABLE_ADD_COLUMN( "w", m_w );
	DECLARE_CLASS_TABLE_END()

FEX	static const Quat& GetZERO      ( void )	{  return ( Quat::ZERO       );  }
FEX	static const Quat& GetIDENTITY  ( void )	{  return ( Quat::IDENTITY   );  }
FEX	static const Quat& GetINVALID   ( void )	{  return ( Quat::INVALID    );  }
FEX	static const Quat& GetINDEFINITE( void )	{  return ( Quat::INDEFINITE );  }
};

FUBI_DECLARE_XFER_TRAITS( matrix_3x3 )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
};

FUBI_DECLARE_POD_TRAITS( Point2 )
{
FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
FEX	static bool FromString( const char* str, Type& obj );
};

FUBI_DECLARE_XFER_TRAITS( OBB2 )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
};

FUBI_DECLARE_POD_TRAITS( gpversion )
{
FEX	static void ToString( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __FUBITRAITSIMPL_H

//////////////////////////////////////////////////////////////////////////////
