//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_persist.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Siege.h"
#include "siege_persist.h"
#include "stringtool.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_EXPORT_POD_TYPE( SiegeGuid );
FUBI_EXPORT_POD_TYPE( SiegePos );
FUBI_EXPORT_POD_TYPE( SiegeRot );

//////////////////////////////////////////////////////////////////////////////
// SiegeGuid traits implementation

void FuBi::Traits <SiegeGuid> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out = obj.ToString();
}

bool FuBi::Traits <SiegeGuid> :: FromString( const char* str, Type& obj )
{
	return ( obj.FromString( str ) );
}

bool FuBi::Traits <SiegeGuid> :: IsNodeInAnyFrustum( void ) const
{
	return ( gSiegeEngine.IsNodeInAnyFrustum( *rcast <const SiegeGuid*> ( this ) ) != NULL );
}

SiegeGuid& FuBi::Traits <SiegeGuid> :: Make( const char* guidText )
{
	static SiegeGuid s_Guid;
	s_Guid = SiegeGuid( guidText );
	return ( s_Guid );
}

SiegeGuid& FuBi::Traits <SiegeGuid> :: Make( unsigned int guidInt )
{
	static SiegeGuid s_Guid;
	s_Guid = SiegeGuid( guidInt );
	return ( s_Guid );
}

const SiegeGuid& FuBi::Traits <SiegeGuid> :: GetUndefinedSiegeGuid( void )
{
	static SiegeGuid s_Guid;
	s_Guid = siege::UNDEFINED_GUID;
	return ( s_Guid );
}

//////////////////////////////////////////////////////////////////////////////
// SiegePos traits implementation

template <> void FuBi::Traits <SiegePos> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g,%g,%s", obj.pos.x, obj.pos.y, obj.pos.z, obj.node.ToString().c_str() );
}

template <> bool FuBi::Traits <SiegePos> :: FromString( const char* str, Type& obj )
{
	char buffer[ 50 ];
	if ( ::sscanf( str, "%f , %f , %f , %50s[abcdefABCDEF0123456789 ]",
					   &obj.pos.x, &obj.pos.y, &obj.pos.z, buffer ) != 4 )
	{
		return ( false );
	}

	return ( obj.node.FromString( buffer ) );
}

SiegePos& FuBi::Traits <SiegePos> :: Make( float x, float y, float z, const SiegeGuid& guid )
{
	static SiegePos s_SiegePos;
	s_SiegePos.pos.x = x;
	s_SiegePos.pos.y = y;
	s_SiegePos.pos.z = z;
	s_SiegePos.node  = guid;
	return ( s_SiegePos );
}

SiegePos& FuBi::Traits <SiegePos> :: Make( float x, float y, float z, unsigned int guidInt )
{
	static SiegePos s_SiegePos;
	s_SiegePos.pos.x = x;
	s_SiegePos.pos.y = y;
	s_SiegePos.pos.z = z;
	s_SiegePos.node  = SiegeGuid( guidInt );
	return ( s_SiegePos );
}

SiegePos& FuBi::Traits <SiegePos> :: MakeDefault( void )
{
	static SiegePos s_SiegePos;
	s_SiegePos = SiegePos::INVALID;
	return ( s_SiegePos );
}

SiegePos& FuBi::Traits <SiegePos> :: Copy( const SiegePos& obj )
{
	static SiegePos s_SiegePos;
	s_SiegePos = obj;
	return ( s_SiegePos );
}

const SiegePos& FuBi::Traits <SiegePos> :: GetUndefinedSiegePos( void )
{
	static SiegePos s_SiegePos;
	s_SiegePos = SiegePos();
	return ( s_SiegePos );
}

float GetSiegeDistance( const SiegePos& pos_orig, const SiegePos& pos_dest )
{
	return( gSiegeEngine.GetDifferenceVector( pos_orig, pos_dest ).Length() );
}

void GetSiegeDifference( vector_3& ret_diff, const SiegePos& pos_orig, const SiegePos& pos_dest )
{
	ret_diff = gSiegeEngine.GetDifferenceVector( pos_orig, pos_dest );
}

const char* MakeSiegePosString( const SiegePos& pos )
{
	static gpstring s_String;
	s_String = ::ToString( pos );
	return ( s_String );
}

//////////////////////////////////////////////////////////////////////////////
// SiegeRot traits implementation

template <> void FuBi::Traits <SiegeRot> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g,%g,%g,%s", obj.rot.m_x, obj.rot.m_y, obj.rot.m_z, obj.rot.m_w, obj.node.ToString().c_str() );
}

template <> bool FuBi::Traits <SiegeRot> :: FromString( const char* str, Type& obj )
{
	char buffer[ 50 ];
	if ( ::sscanf( str, "%f , %f , %f , %f , %50s[abcdefABCDEF0123456789 ]",
					   &obj.rot.m_x, &obj.rot.m_y, &obj.rot.m_z, &obj.rot.m_w, buffer ) != 5 )
	{
		return ( false );
	}

	return ( obj.node.FromString( buffer ) );
}

SiegeRot& FuBi::Traits <SiegeRot> :: Make( float x, float y, float z, float w, const SiegeGuid& guid )
{
	static SiegeRot s_SiegeRot;
	s_SiegeRot.rot.m_x = x;
	s_SiegeRot.rot.m_y = y;
	s_SiegeRot.rot.m_z = z;
	s_SiegeRot.rot.m_w = w;
	s_SiegeRot.node  = guid;
	return ( s_SiegeRot );
}

SiegeRot& FuBi::Traits <SiegeRot> :: Make( float x, float y, float z, float w, unsigned int guidInt )
{
	static SiegeRot s_SiegeRot;
	s_SiegeRot.rot.m_x = x;
	s_SiegeRot.rot.m_y = y;
	s_SiegeRot.rot.m_z = z;
	s_SiegeRot.rot.m_w = w;
	s_SiegeRot.node  = SiegeGuid( guidInt );
	return ( s_SiegeRot );
}

SiegeRot& FuBi::Traits <SiegeRot> :: MakeDefault( void )
{
	static SiegeRot s_SiegeRot;
	s_SiegeRot = SiegeRot::INVALID;
	return ( s_SiegeRot );
}

SiegeRot& FuBi::Traits <SiegeRot> :: Copy( const SiegeRot& obj )
{
	static SiegeRot s_SiegeRot;
	s_SiegeRot = obj;
	return ( s_SiegeRot );
}

const SiegeRot& FuBi::Traits <SiegeRot> :: GetUndefinedSiegeRot( void )
{
	static SiegeRot s_SiegeRot;
	s_SiegeRot = SiegeRot();
	return ( s_SiegeRot );
}


//////////////////////////////////////////////////////////////////////////////

// Fade type conversions for Skrit support
static const char* s_FadeTypeStrings[] =
{
	"ft_none"		,	
	"ft_alpha"		,
	"ft_black"		,
	"ft_in"			,
	"ft_instant"	,
	"ft_in_instant"	,
};

COMPILER_ASSERT( ELEMENT_COUNT( s_FadeTypeStrings ) == FT_COUNT );

static stringtool::EnumStringConverter s_FadeTypeConverter( s_FadeTypeStrings, FT_BEGIN, FT_END, FT_NONE );

const char* ToString( FADETYPE val )
	{  return ( s_FadeTypeConverter.ToString( val ) );  }
bool FromString( const char* str, FADETYPE& val )
	{  return ( s_FadeTypeConverter.FromString( str, rcast <int&> ( val ) ) );  }

bool FuBi::Traits <siege::NODEFADE> :: XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
{
	persist.XferHex( "m_regionID",	obj.m_regionID );
	persist.Xfer   ( "m_section",	obj.m_section );
	persist.Xfer   ( "m_level",		obj.m_level );
	persist.Xfer   ( "m_object",	obj.m_object );
	persist.Xfer   ( "m_fadeType",	obj.m_fadeType );
	persist.Xfer   ( "m_alpha",		obj.m_alpha );
	persist.Xfer   ( "m_startTime",	obj.m_startTime );

	return true;
}

//////////////////////////////////////////////////////////////////////////////

