//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoStash.h
// Author(s):  Chad Queen
//
// Summary  :  Contains the GoStash go component.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOSTASH_H
#define __GOSTASH_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoStash declaration

class GoStash : public GoComponent
{

public:

	SET_INHERITED( GoStash, GoComponent );

// Setup.

	// ctor/dtor
	GoStash( Go* go );
	GoStash( const GoStash& source, Go* newGo );
	virtual ~GoStash( void );
	void Init( void );
	
// Interface.

	// Query
	Goid GetStashActivator()			{ return m_StashActivator;	}	
	Goid GetStashMember()				{ return m_StashMember;		}
	void SetStashTransfer( bool bSet )	{ m_bStashTransfer = bSet;  }
	bool GetStashTransfer()				{ return m_bStashTransfer;	}

	// Activate/Deactivate
	void ActivateStash( Goid member, Goid stashActivator );
	void RefreshStash();
	void CloseStash();

	// Add/Remove
	FEX	void RSAddToStash( Goid member, Goid item );
	FEX	void RCAddToStash( Goid item, DWORD machineId );		
	FEX	void RSRemoveFromStash( Goid member, Goid item );
	FEX void RCRemoveFromStash( Goid item, DWORD machineId );	
	
		void SDisableOmniStatus( Goid item );
	FEX	void RCDisableOmniStatus( Goid item );

	// Misc
	FEX void RSSetStashMemberAndActivator( Goid member, Goid stashActivator );

	// const query
	FEX float GetActivateRange( void ) const;

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

private:

	static DWORD    GoStashToNet( GoStash* x );
	static GoStash* NetToGoStash( DWORD d, FuBiCookie* cookie );
	
	Goid					m_StashActivator;		// Chest or object that triggered the activation of the stash
	Goid					m_StashMember;			// Party member that is currently accessing the stash
	bool					m_bStashTransfer;		// Is the stash currently working on a transaction? (add/remove)

	FUBI_RPC_CLASS( GoStash, GoStashToNet, NetToGoStash, "" );
	SET_NO_COPYING( GoStash );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOSTASH_H

//////////////////////////////////////////////////////////////////////////////