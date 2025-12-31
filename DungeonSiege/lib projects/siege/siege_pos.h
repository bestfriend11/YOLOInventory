#pragma once
#ifndef _SIEGE_POS_
#define _SIEGE_POS_

#include "vector_3.h"
#include "quat.h"
#include "siege_database_guid.h"


struct SiegePos
{
	// Default constructor
	SiegePos() {}

	// Data constructor
	inline SiegePos( const vector_3& ipos, const siege::database_guid& inode )
		: pos( ipos )
		, node( inode ) {}

	static const SiegePos INVALID ;

	// The following helper functions are defined in siege_engine.cpp

	// Convert to World space
	vector_3 WorldPos() const;

	// Convert from World space
	void FromWorldPos( const vector_3& worldpos, const siege::database_guid& nnode );

	// Convert to screen space
	vector_3 ScreenPos() const;

	// Equality
	bool operator == ( const SiegePos& p ) const				{ return( (node == p.node) && pos.IsEqual( p.pos ) ); }
	bool operator != ( const SiegePos& p ) const				{ return( (node != p.node) || !pos.IsEqual( p.pos ) ); }

	bool IsEqual( const SiegePos& p, float epsilon ) const		{ return( (node == p.node) && pos.IsEqual( p.pos, epsilon ) ); }
	bool IsExactlyEqual( const SiegePos& p ) const				{ return( !::memcmp( this, (void *)&p, sizeof(SiegePos) ) ); }

	//----- data

	// Positional information
	vector_3				pos;
	siege::database_guid	node;
};


struct SiegeRot
{

	// Default constructor
	SiegeRot()
		: node( siege::UNDEFINED_GUID )
		, rot( Quat::ZERO )	{}

	static const SiegeRot INVALID ;

	// Data constructor
	inline SiegeRot( const Quat& irot, const siege::database_guid& inode )
		: rot( irot )
		, node( inode ) {}

	// Equality
	bool operator == ( const SiegeRot& r ) const
		{ return( (rot == r.rot) && (node == r.node) ); }

	Quat					rot;
	siege::database_guid	node;	// The node that the rotatrion is relative to
};


inline SiegePos& _ToSiegePosRef( int siegePosRef ) { return ( *(SiegePos*)(siegePosRef) );  }


#endif
