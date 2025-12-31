/* ========================================================================
   Source File: siege_database_guid.cpp
   Description: Implements siege::database_guid
   ======================================================================== */

#include "precomp_siege.h"
#include "siege_database_guid.h"

#include "siege_engine.h"
#include "siege_node.h"
#include "stringtool.h"

using namespace siege;

//////////////////////////////////////////////////////////////////////////////
// class database_guid implementation

const database_guid siege::UNDEFINED_GUID;

database_guid :: database_guid( const char* str )
{
	FromString( str );
}

bool database_guid :: IsValidSlow( void ) const
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if ( !IsValid() || !gSiegeEngine.NodeCache().ObjectExistsInCache( *this ) )
	{
		return ( false );
	}

	return gSiegeEngine.NodeDatabase().IsNodeValid( *this );
}

gpstring database_guid :: ToString( void ) const
{
	return ( gpstringf( "0x%08x", GetValue() ) );
}

bool database_guid :: FromString( const char* str )
{
	bool success = false;

	if ( str[ 0 ] == '0' && str[ 1 ] == 'x' )
	{
		int err = 0;
		DWORD value = stringtool::strtoul( str, NULL, 0, &err );
		if ( !err )
		{
			success = true;
			SetValue( value );
		}
	}
	else
	{
		int value[ 4 ];
		if ( (success = (sscanf( str, "%x %x %x %x", &value[ 0 ], &value[ 1 ], &value[ 2 ], &value[ 3 ] ) == 4)) == true )
		{
			m_Guid[ 0 ] = (UINT8)value[ 0 ];
			m_Guid[ 1 ] = (UINT8)value[ 1 ];
			m_Guid[ 2 ] = (UINT8)value[ 2 ];
			m_Guid[ 3 ] = (UINT8)value[ 3 ];
		}
		else
		{
			gpassertm(0, "sccanf() failed");
			success = false;
		}
	}

	if ( !success )
	{
		GetRawValue() = 0;
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
