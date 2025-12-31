//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoConversation.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the conversation component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOCONVERSATION_H
#define __GOCONVERSATION_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoConversation declaration

class GoConversation : public GoComponent
{
public:
	SET_INHERITED( GoConversation, GoComponent );

// Setup.

	// ctor/dtor
	GoConversation( Go* parent );
	GoConversation( const GoConversation& source, Go* parent );
	virtual ~GoConversation( void );

// Interface.

	struct ConversationInfo
	{
		gpwstring	sText;
		gpstring	sSample;
		float		scrollRate;
		gpstring	sChoice;
		float		duration;
		bool		bNis;

		// Generic Buttons
		gpwstring	sButton1Name;
		gpstring	sButton1Value;
		gpwstring	sButton2Name;
		gpstring	sButton2Value;		
	};

	typedef std::vector< ConversationInfo > ConversationInfoVec;
	typedef std::map <gpstring, unsigned int, istring_less> ConvToRefCountMap;
	typedef std::map <gpwstring, DWORD, istring_less > MacroToValueMap;
	typedef std::pair<gpwstring, DWORD > MacroToValuePair;

	// Add Conversation
	void AddConversation( gpstring sConversation );

	// Remove Conversation
	void RemoveConversation( gpstring sConveration );
		
	// Get the actual dialogue to use from this Go
	void GetDialogue( ConversationInfo & convInfo );
	void GetGoodbyeSample( gpstring & sSample );

	int GetConversationsSize() { return m_Conversations.size(); }

FEX FuBiCookie RSActivateDialogue();
FEX FuBiCookie RCActivateDialogue( Goid member, DWORD machineId );
	void ActivateDialogue( Goid member );

FEX FuBiCookie RSSelectConversation( const char * sConversation );
FEX	FuBiCookie RCSelectConversation( const char * sConversation, DWORD machineId );
	FUBI_MEMBER_SET( RCSelectConversation, +SERVER );
	void SelectConversation( const char * sConversation ) { m_selectedConversation = sConversation; }

FEX	const gpstring& GetSelectedConversation();
	
FEX void SSetCanTalk( bool bCanTalk );
FEX	void RCSetCanTalk( bool bCanTalk );
	void SetCanTalk( bool bCanTalk )	{ m_bCanTalk = bCanTalk; }

	bool GetCanTalk();

	// For use with Siege Editor
	void GetAvailableConversations( ConvToRefCountMap & conversations ) { conversations = m_Conversations; }

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	
	// Messaging
	virtual void HandleMessage( WorldMessage const & Message );

#	if !GP_RETAIL
	virtual bool ShouldSaveInternals( void );
	virtual void SaveInternals      ( FuelHandle fuel );

	void CheckForValidStore( void );
#	endif // !GP_RETAIL


private:
	// Reference Conversation
	void				IncReferenceCount( gpstring sConversation );
	void				DecReferenceCount( gpstring sConversation );
	unsigned int		GetReferenceCount( gpstring sConversation );

	void				IncGoodbyeReferenceCount( gpstring sConversation );
	void				DecGoodbyeReferenceCount( gpstring sConversation );
	unsigned int		GetGoodbyeReferenceCount( gpstring sConversation );

	// Determine the conversation to use
	void DetermineConversation( gpstring & sConversation );

	// Loading/Retrieving Conversation information
	void RetrieveConversation( gpstring sConversation, ConversationInfo & convInfo );

	void RetrieveGoodbyeSample( gpstring & sConversation, gpstring & sSample );	

	ConvToRefCountMap m_Conversations;
	ConvToRefCountMap m_GoodbyeConv;

	MacroToValueMap m_macroMap;

	gpstring m_selectedConversation;

	bool m_bCanTalk;

	Goid m_Listener;

	static DWORD           GoConversationToNet( GoConversation* x );
	static GoConversation* NetToGoConversation( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoConversation );
	FUBI_RPC_CLASS( GoConversation, GoConversationToNet, NetToGoConversation, "" );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOCONVERSATION_H

//////////////////////////////////////////////////////////////////////////////
