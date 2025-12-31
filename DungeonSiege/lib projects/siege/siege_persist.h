//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_persist.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains persistence related helper functions for Siege.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SIEGE_PERSIST_H
#define __SIEGE_PERSIST_H

//////////////////////////////////////////////////////////////////////////////

#include "siege_defs.h"
#include "siege_frustum.h"
#include "siege_pos.h"
#include "FuBiTraits.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"

//////////////////////////////////////////////////////////////////////////////
// traits declarations

// SiegeGuid (siege::database_guid)

	typedef siege::database_guid SiegeGuid;

	FUBI_DECLARE_POD_TRAITS( SiegeGuid )
	{
	FEX	static void ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ = FuBi::XFER_NORMAL );
	FEX	static bool FromString( const char* str, Type& obj );

		// extensions to class
	FEX	bool   IsValid ( void ) const			{  return ( rcast <const SiegeGuid*> ( this )->IsValid() );  }
	FEX	bool   IsNodeInAnyFrustum( void ) const;
	FEX	UINT32 GetValue( void ) const			{  return ( rcast <const SiegeGuid*> ( this )->GetValue() );  }
	FEX	void   SetValue( UINT32 guid )			{  rcast <SiegeGuid*> ( this )->SetValue( guid );  }

		// static extensions to class
	FEX	static SiegeGuid& Make( const char* guidText );
	FEX	static SiegeGuid& Make( unsigned int guidInt );
	FEX	static const SiegeGuid& GetUndefinedSiegeGuid( void );
	};

// SiegePos

	FUBI_DECLARE_POD_TRAITS( SiegePos )
	{
	FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
	FEX	static bool FromString( const char* str, Type& obj );

		// extensions to class
	FEX	vector_3&  GetPos ( void )					{  return ( rcast <SiegePos*> ( this )->pos );  }
	FEX	SiegeGuid& GetNode( void )					{  return ( rcast <SiegePos*> ( this )->node );  }
	FEX	void       SetPos ( const vector_3& v )		{  GetPos() = v;  }
	FEX	void       SetNode( const SiegeGuid& g )	{  GetNode() = g;  }

		// static extensions to class
	FEX	static SiegePos& Make( float x, float y, float z, const SiegeGuid& guid );
	FEX	static SiegePos& Make( float x, float y, float z, unsigned int guidInt );
	FEX	static SiegePos& MakeDefault( void );
	FEX	static SiegePos& Copy( const SiegePos& obj );
	FEX	static const SiegePos& GetUndefinedSiegePos( void );
	};

	// to SiegePos
FEX	inline SiegePos& ToSiegePosRef( int siegePosRef ) {  return ( _ToSiegePosRef(siegePosRef) );  }
	FUBI_DOC       ( ToSiegePosRef,    "siegePosRef", "int to SiegePos reference." );

FEX	float		GetSiegeDistance( const SiegePos& pos_orig, const SiegePos& pos_dest );
FEX	void		GetSiegeDifference( vector_3& ret_diff, const SiegePos& pos_orig, const SiegePos& pos_dest );

FEX	const char* MakeSiegePosString( const SiegePos& pos );
	FUBI_DOC  ( MakeSiegePosString,  "pos",
			   "Returns the string version of the given siege position" );

// SiegeRot

	FUBI_DECLARE_POD_TRAITS( SiegeRot )
	{
	FEX	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
	FEX	static bool FromString( const char* str, Type& obj );

		// extensions to class
	FEX	Quat&      GetRot ( void )					{  return ( rcast <SiegeRot*> ( this )->rot );  }
	FEX	SiegeGuid& GetNode( void )					{  return ( rcast <SiegeRot*> ( this )->node );  }
	FEX	void       SetRot ( const Quat& q )			{  GetRot() = q;  }
	FEX	void       SetNode( const SiegeGuid& g )	{  GetNode() = g;  }

		// static extensions to class
	FEX	static SiegeRot& Make( float x, float y, float z, float w, const SiegeGuid& guid );
	FEX	static SiegeRot& Make( float x, float y, float z, float w, unsigned int guidInt );
	FEX	static SiegeRot& MakeDefault( void );
	FEX	static SiegeRot& Copy( const SiegeRot& obj );
	FEX	static const SiegeRot& GetUndefinedSiegeRot( void );

	};

// FadeType

FEX	const char* ToString  ( FADETYPE val );
FEX	bool        FromString( const char* str, FADETYPE& val );

	FUBI_DECLARE_ENUM_TRAITS( FADETYPE, FT_NONE );

	FUBI_DECLARE_XFER_TRAITS( siege::NODEFADE )
	{
		static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj );
	};

//////////////////////////////////////////////////////////////////////////////

#endif  // __SIEGE_PERSIST_H

//////////////////////////////////////////////////////////////////////////////
