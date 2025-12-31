//////////////////////////////////////////////////////////////////////////////
//
// File     :  conversation.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "precomp_world.h"
#include "conversation.h"
#include "worldmap.h"
#include "fuel.h"
#include "gocore.h"
#include "world.h"
#include "worldmessage.h"


Conversation::Conversation( Go * owner )
	: m_pGo( owner )
{
}


Conversation::~Conversation()
{
}


Go * Conversation::GetGo()
{
	gpassert( m_pGo );
	return( m_pGo );
}


void Conversation::SetGo( Go * owner )
{
	gpassert( owner );
	m_pGo = owner;
}


void Conversation::HandleMessage( WorldMessage const & Message )
{
	GPPROFILERSAMPLE( "Conversation::HandleMessage", SP_UI | SP_HANDLE_MESSAGE );

	if ( Message.HasEvent( WE_REQ_TALK_BEGIN ) )
	{
		gWorld.DialogueCallback( GetGo()->GetGoid() );
	}
}


// Add Conversation
void Conversation::AddConversation( gpstring sConversation )
{
	m_Conversations.insert( ConvToRefCountPair( sConversation, 0 ) );	
}


// Remove Conversation
void Conversation::RemoveConversation( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		m_Conversations.erase( iFind );
	}
}


// Reference Conversation
void Conversation::IncReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		(*iFind).second++;
	}
}


void Conversation::DecReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		if ( (*iFind).second != 0 )
		{
			(*iFind).second--;
		}
	}
}


unsigned int Conversation::GetReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		return ( (*iFind).second );
	}

	return 0;
}


void Conversation::RetrieveConversation( gpstring sConversation, gpstring & sText, gpstring & sSample, float & scrollRate )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		FuelHandle hRegion( gWorldMap.GetRegionGasAddress( 
							gWorldMap.GetRegionByNode( GetGo()->GetPlacement()->GetPosition().node ) ) );

		if( hRegion.IsValid() )
		{
			FuelHandle hConversation = hRegion->GetChildBlock( "conversations" );
			if ( hConversation.IsValid() )
			{
				FuelHandle hFound = hConversation->GetChildBlock( sConversation );
				if ( hFound.IsValid() )
				{						
					FuelHandleList hlText = hFound->ListChildBlocks( 1, "", "text*" );
					FuelHandleList::iterator i;
					std::vector< gpstring > lines;
					std::vector< gpstring > samples;
					std::vector< float > scroll_rates;
					for ( i = hlText.begin(); i != hlText.end(); ++i ) 
					{
						gpstring	sCurrText;
						gpstring	sCurrSample;
						int			order		= 0;
						float		currScroll	= 20.0f;
						(*i)->Get( "text", sCurrText );
						(*i)->Get( "sample", sCurrSample );
						(*i)->Get( "scroll_rate", currScroll );
						if ( (*i)->Get( "order", order ) )
						{
							if ( order == (int)(*iFind).second )
							{
								sText		= sCurrText;
								sSample		= sCurrSample;
								scrollRate	= currScroll;
								IncReferenceCount( sConversation );
								return;
							}
							continue;
						}
						
						lines.push_back( sCurrText );														
						samples.push_back( sCurrSample );
						scroll_rates.push_back( currScroll );
					}

					IncReferenceCount( sConversation );

					if ( lines.size() != 0 )
					{
						int index = Random( (int)(lines.size()-1) );						
						sText		= lines[index];
						sSample		= samples[index];
						scrollRate	= scroll_rates[index];
					}
				}
			}
		}
	}
}


void Conversation::DetermineConversation( gpstring & sConversation )
{
	// $$$ Check the brain and see which conversation should be loaded here
	// For now, just pick the first conversation

	if ( m_Conversations.size() == 0 )
	{
		return;
	}

	ConvToRefCountMap::iterator iFirst = m_Conversations.begin();
	sConversation = (*iFirst).first;
}


void Conversation::GetDialogue( gpstring & sText, gpstring & sSample, float & scrollRate )
{
	gpstring sConversation;
	DetermineConversation( sConversation );
	RetrieveConversation( sConversation, sText, sSample, scrollRate );
}
