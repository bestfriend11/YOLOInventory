/*=======================================================================================

  BoneTranslator


  purpose:	Provides concrete unchanging bone names for any model as long as the
			translation information exists within the gas file for the model
			under the block name of [bone_translator].

			This allows for the modeler to use any name that they want as long
			as they fill in the blanks for the minimum bone names.

  author:	Rick Saenz

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/

#pragma once
#ifndef _BONE_TRANSLATOR_H_
#define _BONE_TRANSLATOR_H_

#include "GoDefs.h"
#include "NeMa_Types.h"

class BoneTranslator
{
public:

// Types.

	enum eBone
	{
		SET_BEGIN_ENUM( BONE_, 0 ),

		// Actor implied
		KILL_BONE,			// Bone to shoot at or cast magic spell at

		WEAPON_BONE,		// Bone that an actor uses to hold a weapon
		SHIELD_BONE,		// Bone that an actor uses to hold a shield

		BODY_ANTERIOR,		// Nearest bone to the head
		BODY_MID,			// Middlemost bone of the body
		BODY_POSTERIOR,		// The ass or tail

		// Item implied
		TIP,				// The tip of the item
		MIDDLE,				// The middle of the item
		HANDLE,				// The handle or base of the item

		SET_END_ENUM( BONE_ ),
	};

	static const char* ToString  ( eBone b );
	static bool        FromString( const char* str, eBone& b );

// Setup.

	// ctor/dtor
	BoneTranslator( void );

	// initialization
	bool AddTranslation( nema::HAspect aspect, const char* nativeName, const char* nemaName );
	bool Load( nema::HAspect aspect, FastFuelHandle fuel );

// Query.

	// simple lookup
	bool Translate( eBone bone, int& nemaBoneIndex ) const;
	bool Translate( int nemaBoneIndex, eBone& bone ) const;

	// positional resolution - calculates the position of a bone in a model in
	//  nodespace using orientation and an offset in model space for a 'fudge'
	//  factor or small adjustment
	bool GetPosition( Go const * go, eBone bone, SiegePos& position, const vector_3& offset = vector_3::ZERO, bool adjustPointToTerrain = true ) const;

private:
	int m_DemBones[ BONE_COUNT ];		// storage for Nema-specific bone indexes
};

#endif	// _BONE_TRANSLATOR_H_
