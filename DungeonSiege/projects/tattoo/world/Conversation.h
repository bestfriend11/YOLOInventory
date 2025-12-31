//////////////////////////////////////////////////////////////////////////////
//
// File     :  conversation.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __CONVERSATION_H
#define __CONVERSATION_H

#include "gpstring.h"
#include <map>


typedef std::map< gpstring, unsigned int, istring_less >	ConvToRefCountMap;
typedef std::pair< gpstring, unsigned int >					ConvToRefCountPair;

class Go;

class Conversation
{
public:

	// Existence
	Conversation( Go * owner );
	~Conversation();

	// Messaging
	void HandleMessage( WorldMessage const & Message );

	// Go Ownership
	Go * GetGo();
	void SetGo( Go * owner );

	// Add Conversation
	void AddConversation( gpstring sConversation );

	// Remove Conversation
	void RemoveConversation( gpstring sConveration );
		
	// Get the actual dialogue to use from this Go
	void GetDialogue( gpstring & sText, gpstring & sSample, float & scrollRate );

private:

	// Reference Conversation
	void				IncReferenceCount( gpstring sConversation );
	void				DecReferenceCount( gpstring sConversation );
	unsigned int		GetReferenceCount( gpstring sConversation );

	// Determine the conversation to use
	void DetermineConversation( gpstring & sConversation );

	// Loading/Retrieving Conversation information
	void RetrieveConversation( gpstring sConversation, gpstring & sText, gpstring & sSample, float & scrollRate );


	Go * m_pGo;
	ConvToRefCountMap m_Conversations;

};


#endif