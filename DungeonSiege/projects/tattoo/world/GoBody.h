//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoBody.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the GoBody component.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOBODY_H
#define __GOBODY_H

//////////////////////////////////////////////////////////////////////////////

#include "BoneTranslator.h"
#include "Choreographer.h"
#include "Go.h"
#include "nema_aspect.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_EXPORT AnimReq& MakeAnimReq( eAnimChore chore );

//////////////////////////////////////////////////////////////////////////////
// struct AnimReq declaration

struct AnimReq
{
	FUBI_POD_CLASS( AnimReq );

	FUBI_VARIABLE( eAnimChore , m_, ReqChore,       "Chore to run" );
	FUBI_VARIABLE( DWORD      , m_, ReqBlock,       "Request Block" );
	FUBI_VARIABLE( eAnimStance, m_, ReqStance,      "Requested stance" );
	FUBI_VARIABLE( int        , m_, ReqSubAnim,     "Requested subanim" );
	FUBI_VARIABLE( DWORD      , m_, ReqFlags,	    "Requested flags" );

	AnimReq( void );
	AnimReq( eAnimChore chore, DWORD reqblock = 0, eAnimStance stance = AS_DEFAULT, int subanim = 0 ,DWORD flags = 0 );

	void Init( void );
};


//////////////////////////////////////////////////////////////////////////////
// class GoBody declaration

class GoBody : public GoComponent
{
public:
	SET_INHERITED( GoBody, GoComponent );

// Setup.

	// ctor/dtor
	GoBody( Go* parent );
	GoBody( const GoBody& source, Go* newParent );
	virtual ~GoBody( void );

// Attributes.

	// const query
	const gpstring& GetArmorVersion( void ) const;

// Animation.

	// animation commands $$$ should these be retry exports??
	FUBI_EXPORT void RCAnimate( const AnimReq& animReq );
	FUBI_EXPORT void SAnimate ( const AnimReq& animReq );
				void Animate  ( const AnimReq& animReq );

// animation query

				float                 GetAngularTurningVelocity( void ) const				{  return ( m_AngularTurningVelocity );  }
				bool                  GetCanTurnAndMove        ( void ) const				{  return ( m_CanTurnAndMove );  }
				const BoneTranslator& GetBoneTranslator        ( void ) const				{  return ( *m_BoneTranslator );  }

	// movement - walk/run speed params
FEX	float GetMinMoveVelocity( void )						{  return ( m_MinMoveVelocity ); }
FEX	float GetNaturalAvgMoveVelocity( void )					{  return ( m_AvgMoveVelocity.GetNatural() ); }
FEX	float GetAvgMoveVelocity( void )						{  return ( m_AvgMoveVelocity ); }
FEX	float SetNaturalAvgMoveVelocity( float v )				{  float oldv = m_AvgMoveVelocity.GetNatural();  m_AvgMoveVelocity.SetNatural( v ); GetGo()->SetModifiersDirty(); return oldv;  }
FEX	void  SetAvgMoveVelocity( float v )						{  m_AvgMoveVelocity = v;  }
FEX	float GetMaxMoveVelocity( void )						{  return ( m_MaxMoveVelocity ); }

	siege::eLogicalNodeFlags GetTerrainMovementPermissions() const {  return ( m_TerrainMovementPermissions ); }

FEX	bool  IsPositionWalkable( SiegePos& position );


// Limited access functions.

	// $ you're on your honor here - don't call these unless you know what
	//   you are doing!

	// special: GoInventory/GoDefend may call these, and nobody else!
	bool InstantiateDeformableArmor( Go* armor , const gpstring& ChestSlotArmorType = gpstring::EMPTY);
	static bool InstantiateDeformableUnwornArmor( Go* armor );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation ( void );
	virtual bool CommitLoad     ( void );
	virtual void Shutdown       ( void );
	virtual void ResetModifiers ( void );
	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void Update         ( float deltaTime );

#	if !GP_RETAIL
FEX	void Dump( bool verbose, ReportSys::Context * ctx );
#	endif // !GP_RETAIL


private:

	bool CommitLoadOrCreate( bool load = true );

// Spec.

	// animation
	RpChoreDictionary m_AnimChoreDictionary;
	float			  m_AngularTurningVelocity;
	bool			  m_CanTurnAndMove;

	// sim/sfx
	RpBoneTranslator  m_BoneTranslator;

	// walk/run speeds
	float		m_MinMoveVelocity;
	GobFloat	m_AvgMoveVelocity;
	float		m_MaxMoveVelocity;

	// permissions
	siege::eLogicalNodeFlags m_TerrainMovementPermissions;

// State.

	// animation

	static DWORD   GoBodyToNet( GoBody* x );
	static GoBody* NetToGoBody( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoBody );
	FUBI_RPC_CLASS( GoBody, GoBodyToNet, NetToGoBody, "" );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOBODY_H

//////////////////////////////////////////////////////////////////////////////
