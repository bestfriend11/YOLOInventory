//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoShmo.h
// Author(s):  Scott Bilas
//
// Summary  :  Go component cum-dumpster.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOSHMO_H
#define __GOSHMO_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoGold declaration

class GoGold : public GoComponent
{
public:
	SET_INHERITED( GoGold, GoComponent );

// Setup.

	// ctor/dtor
	GoGold( Go* go );
	GoGold( const GoGold& source, Go* newGo );
	virtual ~GoGold( void );

// Interface.

	void SetGoldValueDirty( void );

	// special routing callback!
FEX	FuBiCookie RCDepositSelfIn( Go* where );

	// Need to know who dropped the gold MP Gold sharing to avoid splitting player dropped piles of gold.
FEX Goid	GetDroppedBy() const				{ return m_DroppedBy; };
	void	SetDroppedBy( Goid droppedBy )		{ m_DroppedBy = droppedBy; };

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation( void );
	virtual void Shutdown      ( void );

private:
	Goid				m_DroppedBy;			

	RpGoGoldRangeColl	m_RangeColl;

	static DWORD   GoGoldToNet( GoGold* x );
	static GoGold* NetToGoGold( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoGold );
	FUBI_RPC_CLASS( GoGold, GoGoldToNet, NetToGoGold, "" );
};

//////////////////////////////////////////////////////////////////////////////
// class GoPotion declaration

class GoPotion : public GoComponent
{
public:
	SET_INHERITED( GoPotion, GoComponent );

// Setup.

	// ctor/dtor
	GoPotion( Go* go );
	GoPotion( const GoPotion& source, Go* newGo );
	virtual ~GoPotion( void );

// Interface.

	// const query
	const gpstring& GetInventoryIcon2( void ) const;

	// call this when the potion changes its value
	void SetFullRatioDirty( void );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation( void );

private:
	RpGoPotionRangeColl m_RangeColl;

	static DWORD     GoPotionToNet( GoPotion* x );
	static GoPotion* NetToGoPotion( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoPotion );
	FUBI_RPC_CLASS( GoPotion, GoPotionToNet, NetToGoPotion, "" );
};

//////////////////////////////////////////////////////////////////////////////
// class GoFader declaration

class GoFader : public GoComponent
{
public:
	SET_INHERITED( GoFader, GoComponent );

// Setup.

	// ctor/dtor
	GoFader( Go* go );
	GoFader( const GoFader& source, Go* newGo );
	virtual ~GoFader( void );

// Interface.

	// rendering
	nema::HAspect        GetAspectHandle  ( void ) const;
	siege::eCollectFlags BuildCollectFlags( void ) const;
	bool                 BuildRenderInfo  ( siege::ASPECTINFO& info, siege::eRequestFlags rFlags );

	// scavenging
	void TakeAspect( GoAspect* aspect );

	// helper
	static bool FadeAspect( Goid goid );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

private:
	float         m_ObjectScale;		// scale for aspect
	nema::HAspect m_Aspect;				// nema aspect (representation)
	Goid          m_OldAspect;			// the old aspect
	vector_3	  m_PrimaryBonePos;		// position of locked primary bone
	Quat		  m_PrimaryBoneOrient;	// orientation of locked primary bone

	SET_NO_COPYING( GoFader );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOSHMO_H

//////////////////////////////////////////////////////////////////////////////
