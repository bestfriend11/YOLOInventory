//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoConversation.cpp
// Author(s):  Scott Bilas, Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoConversation.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "Job.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoMind.h"
#include "GoStore.h"
#include "GoAspect.h"
#include "GoInventory.h"
#include "ui_types.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldState.h"
#include "Victory.h"

DECLARE_GO_SERVER_COMPONENT( GoConversation, "conversation" );

//////////////////////////////////////////////////////////////////////////////
// class GoConversation implementation

GoConversation :: GoConversation( Go* parent )
	: Inherited( parent )
	, m_bCanTalk( true )
	, m_Listener( GOID_INVALID )
{
	// this space intentionally left blank...
}

GoConversation :: GoConversation( const GoConversation& source, Go* parent )
	: Inherited( source, parent )
	, m_bCanTalk( true )
	, m_Listener( GOID_INVALID )
{
	// this space intentionally left blank...
}

GoConversation :: ~GoConversation( void )
{
	// this space intentionally left blank...
}

// Add Conversation
void GoConversation :: AddConversation( gpstring sConversation )
{
	m_Conversations.insert( std::make_pair( sConversation, 0 ) );
	m_GoodbyeConv.insert( std::make_pair( sConversation, 0 ) );
}

// Remove Conversation
void GoConversation :: RemoveConversation( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		m_Conversations.erase( iFind );
	}
}

void GoConversation :: GetDialogue( ConversationInfo & convInfo )
{
	gpstring sConversation;
	DetermineConversation( sConversation );
	RetrieveConversation( sConversation, convInfo );
}

void GoConversation :: GetGoodbyeSample( gpstring & sSample )
{
	gpstring sConversation;
	DetermineConversation( sConversation );
	RetrieveGoodbyeSample( sConversation, sSample );
}

FuBiCookie GoConversation :: RSActivateDialogue()
{
	FUBI_RPC_THIS_CALL_RETRY( RSActivateDialogue, RPC_TO_SERVER );

	Job * pJob = GetGo()->GetMind()->GetFrontJob( JQ_ACTION );
	if ( pJob && pJob->GetJobAbstractType() == JAT_TALK )
	{
		GoHandle hMember( pJob->GetGoalObject() );
		if ( hMember.IsValid() )
		{
			return RCActivateDialogue( hMember->GetGoid(), hMember->GetPlayer()->GetMachineId() );
		}
	}

	return RPC_SUCCESS;
}

FuBiCookie GoConversation :: RCActivateDialogue( Goid member, DWORD machineId )
{
	FUBI_RPC_THIS_CALL_RETRY( RCActivateDialogue, machineId );

	ActivateDialogue( member );

	return RPC_SUCCESS;
}

FuBiCookie GoConversation :: RSSelectConversation( const char * sConversation )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSelectConversation, RPC_TO_SERVER );

	Job * pJob = GetGo()->GetMind()->GetFrontJob( JQ_ACTION );
	if ( pJob && pJob->GetJobAbstractType() == JAT_TALK )
	{
		GoHandle hMember( pJob->GetGoalObject() );
		if ( hMember.IsValid() )
		{
			return RCSelectConversation( sConversation, hMember->GetPlayer()->GetMachineId() );
		}
	}

	return RPC_SUCCESS;
}

FuBiCookie GoConversation :: RCSelectConversation( const char * sConversation, DWORD machineId )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSelectConversation, machineId );

	if ( sConversation )
	{
		ConvToRefCountMap::iterator iFound = m_Conversations.find( sConversation );
		if ( iFound == m_Conversations.end() )
		{
			gperrorf( ( "Conversation does not exist in data: Conversation: %s, for Scid: 0x%x, Screen Name: %s",
						sConversation, GetGo()->GetScid(), GetGo()->GetCommon()->GetScreenName().c_str() ) );
		}
	}

	SelectConversation( sConversation );

	return RPC_SUCCESS;
}

const gpstring& GoConversation :: GetSelectedConversation()
{	
	if ( m_selectedConversation.empty() && m_Conversations.size() != 0 )
	{
		// Default
		ConvToRefCountMap::iterator iFirst = m_Conversations.begin();
		m_selectedConversation = (*iFirst).first;
	}

	return m_selectedConversation;
}

GoComponent* GoConversation :: Clone( Go* newParent )
{
	return ( new GoConversation( *this, newParent ) );
}

bool GoConversation :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "can_talk", m_bCanTalk );

	if ( persist.IsFullXfer() )
	{
		persist.XferMap( "m_Conversations", m_Conversations );
		persist.XferMap( "m_GoodbyeConv", m_GoodbyeConv );
		persist.Xfer( "m_Listener", m_Listener );
	}
	else
	{
		// only build storage if this is initial creation
		if ( persist.IsRestoring() && !GetGo()->HasValidGoid() )
		{
			FastFuelHandle fuel = GetData()->GetInternalField( "conversations" );
			if ( fuel )
			{
				FastFuelFindHandle fh( fuel );
				if ( fh.FindFirstValue( "*" ) )
				{
					gpstring sValue;
					while ( fh.GetNextValue( sValue ) )
					{
						AddConversation( sValue );
					}
				}
			}
		}
	}

	return ( true );
}


#if !GP_RETAIL

bool GoConversation :: ShouldSaveInternals( void )
{
	if ( m_Conversations.empty() )
	{
		return false;
	}
	return true;
}

#endif // !GP_RETAIL


#if !GP_RETAIL

void GoConversation :: SaveInternals( FuelHandle fuel )
{
	FuelHandle hConversations = fuel->CreateChildBlock( "conversations" );
	ConvToRefCountMap::iterator i;
	for ( i = m_Conversations.begin(); i != m_Conversations.end(); ++i )
	{
		hConversations->Add( "*", (*i).first.c_str() );
	}
}

#endif // !GP_RETAIL


#if !GP_RETAIL 

void GoConversation :: CheckForValidStore( void )
{
	if ( GetGo()->HasStore() && GetGo()->GetStore()->IsItemStore() && GetGo()->HasInventory() )
	{
		int invCount = 0;
		GopColl items;
		GopColl::iterator iItem;
		GetGo()->GetInventory()->ListItems( IL_MAIN, items );
		for ( iItem = items.begin(); iItem != items.end(); ++iItem )
		{
			if ( !(*iItem)->IsEquipped() )
			{
				invCount++;
			}
		}

		bool bHasShopConv = false;
		if ( invCount != 0 ) // Probably a store
		{			
			ConvToRefCountMap::iterator iConv;
			for ( iConv = m_Conversations.begin(); iConv != m_Conversations.end(); ++iConv )
			{
				FastFuelHandle hFound( gWorldMap.MakeConversationFuel( GetGo()->GetRegionSource(), (*iConv).first ) );
				if ( hFound.IsValid() )
				{			
					FastFuelHandleColl hlText;
					hFound.ListChildrenNamed( hlText, "text*" );
					FastFuelHandleColl::iterator i;
					
					for ( i = hlText.begin(); i != hlText.end(); ++i )
					{				
						gpstring sChoice;
						(*i).Get( "choice", sChoice );
						if ( sChoice.same_no_case( "shop" ) )
						{
							bHasShopConv = true;
						}
					}
				}
			}

			if ( !bHasShopConv )
			{
				gperrorf( ( "CONTENT PROBLEM: An item store (Template: %s, Scid: 0x%08x) was loaded, but the conversation won't let you shop there!", GetGo()->GetTemplateName(), MakeInt( GetGo()->GetScid() ) ) );
			}
		}		
	}
}

#endif


void GoConversation :: HandleMessage( WorldMessage const & Message )
{
	gpassert( !Message.IsCC() );

	switch ( Message.GetEvent() )
	{
		case ( WE_CONSTRUCTED ):
		{
			m_macroMap.insert( MacroToValuePair( L"goldvalue", GetGo()->GetAspect()->GetGoldValue() ) );
		}
		break;
#if !GP_RETAIL
		case ( WE_ENTERED_WORLD ):
		{
			// Check to see if there is a conversation for shopping if necessary.
			CheckForValidStore();
		}
		break;
#endif		
	}
}

void  GoConversation :: ActivateDialogue( Goid member )
{
	if ( GetCanTalk() )
	{
		m_Listener = member;
		gWorld.DialogueCallback( GetGoid(), member );
	}
}

// Reference Conversation
void GoConversation :: IncReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		(*iFind).second++;
	}
}

void GoConversation :: DecReferenceCount( gpstring sConversation )
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

unsigned int GoConversation :: GetReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		return ( (*iFind).second );
	}

	return 0;
}

void GoConversation :: IncGoodbyeReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_GoodbyeConv.find( sConversation );
	if ( iFind != m_GoodbyeConv.end() )
	{
		(*iFind).second++;
	}
}

void GoConversation :: DecGoodbyeReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_GoodbyeConv.find( sConversation );
	if ( iFind != m_GoodbyeConv.end() )
	{
		if ( (*iFind).second != 0 )
		{
			(*iFind).second--;
		}
	}
}

unsigned int GoConversation :: GetGoodbyeReferenceCount( gpstring sConversation )
{
	ConvToRefCountMap::iterator iFind = m_GoodbyeConv.find( sConversation );
	if ( iFind != m_GoodbyeConv.end() )
	{
		return ( (*iFind).second );
	}

	return 0;
}

void GoConversation :: DetermineConversation( gpstring & sConversation )
{
	if ( m_Conversations.size() == 0 )
	{
		return;
	}

	if ( GetSelectedConversation().empty() )
	{
		// Default
		ConvToRefCountMap::iterator iFirst = m_Conversations.begin();
		sConversation = (*iFirst).first;
	}
	else
	{
		ConvToRefCountMap::iterator iFirst = m_Conversations.find( GetSelectedConversation() );
		if ( iFirst != m_Conversations.end() )
		{
			// Yay, we have a valid conversation!
			sConversation = (*iFirst).first;
		}
		else
		{
			// Hmmm, the conversation doesn't exist that we're requesting.  hmmm...
			iFirst = m_Conversations.begin();
			sConversation = (*iFirst).first;
			gpwarningf( ( "Conversation: %s - Does not exist for this actor", GetSelectedConversation().c_str() ) );
		}
	}

	SelectConversation( "" );
}

struct ConvQuest
{
	gpstring	sName;
	int			order;
};

typedef std::vector< ConvQuest > ConvQuests;

void GoConversation :: RetrieveConversation( gpstring sConversation, ConversationInfo & convInfo )
{
	ConvToRefCountMap::iterator iFind = m_Conversations.find( sConversation );
	if ( iFind != m_Conversations.end() )
	{
		FastFuelHandle hFound( gWorldMap.MakeConversationFuel( GetGo()->GetRegionSource(), sConversation ) );
		if ( hFound.IsValid() )
		{			
			FastFuelHandleColl hlText;
			hFound.ListChildrenNamed( hlText, "text*" );
			FastFuelHandleColl::iterator i;
			
			ConversationInfoVec conversations;									
			std::vector< ConvQuests	>	quests;
			std::vector< StringVec >	finish_quests;
			std::vector< StringVec >	deactivate_quests;

			for ( i = hlText.begin(); i != hlText.end(); ++i )
			{
				ConversationInfo currConv;
				currConv.duration	= 0.0f;
				currConv.bNis		= false;
				currConv.scrollRate	= 20.0f;
				int	order			= 0;
				
				ConvQuests	activateQuests;	
				StringVec	completedQuests;
				StringVec	deactivateQuests;
				
				(*i).Get( "screen_text", currConv.sText );
				gpwstring sTranslated = ReportSys::TranslateW( currConv.sText.c_str() );

				unsigned int macroBegin = sTranslated.find( L'<' );
				unsigned int macroEnd	= sTranslated.find( L'>' );
				while ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
				{
					gpwstring sMacro = sTranslated.substr( macroBegin+1, (macroEnd-macroBegin)-1 );
					MacroToValueMap::iterator iMacro = m_macroMap.find( sMacro );
					if ( iMacro == m_macroMap.end() )
					{
						gperrorf(( "Conversation macro not found: %s in Conversation: %s", ToAnsi( sMacro.c_str() ).c_str(), sConversation.c_str() ));
					}

					gpwstring sBegin	= sTranslated.substr( 0, macroBegin );
					gpwstring sEnd		= sTranslated.substr( macroEnd+1, sTranslated.size() );

					sBegin.appendf( L"%d", (*iMacro).second );

					sTranslated = sBegin;
					sTranslated += sEnd;

					macroBegin = sTranslated.find( L'<' );
					macroEnd	= sTranslated.find( L'>' );
				}

				if (	( macroBegin != gpwstring::npos && macroEnd == gpwstring::npos ) ||
						( macroBegin != gpwstring::npos && macroEnd == gpwstring::npos ) )
				{
					gperrorf(( "Mismatched bracket pair in Conversation: %s", sConversation.c_str() ));
				}

				(*i).Get( "sample", currConv.sSample );
				(*i).Get( "scroll_rate", currConv.scrollRate );
				(*i).Get( "choice", currConv.sChoice );

				FastFuelFindHandle hFind( (*i) );
				if ( hFind.FindFirstValue( "activate_quest*" ) )
				{
					gpstring	sQuest;
					while ( hFind.GetNextValue( sQuest ) )
					{
						ConvQuest activateQuest;
						activateQuest.order = 0;
						if ( stringtool::GetNumDelimitedStrings( sQuest, ',' ) == 2 )
						{
							stringtool::GetDelimitedValue( sQuest, ',', 0, activateQuest.sName );
							stringtool::GetDelimitedValue( sQuest, ',', 1, activateQuest.order );							
						}	
						else
						{
							activateQuest.sName = sQuest;
						}

						activateQuests.push_back( activateQuest );						
					}
				}

				if ( hFind.FindFirstValue( "complete_quest*" ) )
				{
					gpstring sQuest;
					while ( hFind.GetNextValue( sQuest ) )
					{
						completedQuests.push_back( sQuest );
					}
				}

				if ( hFind.FindFirstValue( "deactivate_quest*" ) )
				{
					gpstring sQuest;
					while ( hFind.GetNextValue( sQuest ) )
					{
						deactivateQuests.push_back( sQuest );
					}
				}

				(*i).Get( "duration", currConv.duration );
				(*i).Get( "nis", currConv.bNis );
				(*i).Get( "button_1_text", currConv.sButton1Name );
				(*i).Get( "button_1_value", currConv.sButton1Value );
				(*i).Get( "button_2_text", currConv.sButton2Name );
				(*i).Get( "button_2_value", currConv.sButton2Value );

				if ( (*i).Get( "order", order ) )
				{
					if ( order == (int)(*iFind).second )
					{
						convInfo = currConv;
						convInfo.sText		= sTranslated;
						
						if ( convInfo.bNis && gWorldState.GetCurrentState() != WS_SP_NIS )
						{
							return;
						}

						ConvQuests::iterator iQuest;
						for ( iQuest = activateQuests.begin(); iQuest != activateQuests.end(); ++iQuest )
						{
							gVictory.RSActivateQuest( (*iQuest).sName, (*iQuest).order );
						}

						StringVec::iterator iCompleteQuest;
						for ( iCompleteQuest = completedQuests.begin(); iCompleteQuest != completedQuests.end(); ++iCompleteQuest )
						{
							gVictory.RSCompletedQuest( (*iCompleteQuest), m_Listener );
						}

						StringVec::iterator iDeactivateQuest;
						for ( iDeactivateQuest = deactivateQuests.begin(); iDeactivateQuest != deactivateQuests.end(); ++iDeactivateQuest )
						{
							gVictory.RSDeactivateQuest( (*iDeactivateQuest) );
						}

						IncReferenceCount( sConversation );
						return;
					}
					continue;
				}				

				currConv.sText = sTranslated;
				conversations.push_back( currConv );
				quests.push_back( activateQuests );	
				finish_quests.push_back( completedQuests );
				deactivate_quests.push_back( deactivateQuests );
			}

			IncReferenceCount( sConversation );

			if ( conversations.size() != 0 )
			{
				int index	= Random( (int)(conversations.size()-1) );
				convInfo	= conversations[index];
				
				ConvQuests::iterator iQuest;
				if ( (int)quests.size() > index )
				{
					for ( iQuest = quests[index].begin(); iQuest != quests[index].end(); ++iQuest )
					{
						gVictory.RSActivateQuest( (*iQuest).sName, (*iQuest).order );
					}
				}

				StringVec::iterator iCompleteQuest;
				if ( (int)finish_quests.size() > index )
				{
					for ( iCompleteQuest = finish_quests[index].begin(); iCompleteQuest != finish_quests[index].end(); ++iCompleteQuest )
					{
						gVictory.RSCompletedQuest( (*iCompleteQuest), m_Listener );
					}
				}
				
				StringVec::iterator iDeactivateQuest;
				if ( (int)deactivate_quests.size() > index )
				{
					for ( iDeactivateQuest = deactivate_quests[index].begin(); iDeactivateQuest != deactivate_quests[index].end(); ++iDeactivateQuest )
					{
						gVictory.RSDeactivateQuest( (*iDeactivateQuest) );
					}
				}
			}
		}
	}
}

void GoConversation :: RetrieveGoodbyeSample( gpstring & sConversation, gpstring & sSample )
{
	ConvToRefCountMap::iterator iFind = m_GoodbyeConv.find( sConversation );
	if ( iFind != m_GoodbyeConv.end() )
	{
		FastFuelHandle hFound( gWorldMap.MakeConversationFuel( GetGo()->GetRegionSource(), sConversation ) );
		if ( hFound.IsValid() )
		{
			FastFuelHandleColl hlText;
			hFound.ListChildrenNamed( hlText, "goodbye*" );
			FastFuelHandleColl::iterator i;
			std::vector< gpstring > samples;

			for ( i = hlText.begin(); i != hlText.end(); ++i )
			{
				gpstring	sCurrSample;
				int			order		= 0;

				(*i).Get( "sample", sCurrSample );
				if ( (*i).Get( "order", order ) )
				{
					if ( order == (int)(*iFind).second )
					{
						sSample		= sCurrSample;

						IncGoodbyeReferenceCount( sConversation );
						return;
					}
					continue;
				}

				samples.push_back( sCurrSample );
			}

			IncGoodbyeReferenceCount( sConversation );

			if ( samples.size() != 0 )
			{
				int index = Random( (int)(samples.size()-1) );
				sSample		= samples[index];
			}
		}
	}
}

void GoConversation :: SSetCanTalk( bool bCanTalk )
{
	FUBI_RPC_THIS_CALL( SSetCanTalk, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	RCSetCanTalk( bCanTalk );
}

void GoConversation :: RCSetCanTalk( bool bCanTalk )
{
	FUBI_RPC_THIS_CALL( RCSetCanTalk, RPC_TO_ALL );

	SetCanTalk( bCanTalk );
}

bool GoConversation :: GetCanTalk()
{ 
	if ( GetSelectedConversation().empty() )
	{
		return false;
	}

	return m_bCanTalk; 
}

DWORD GoConversation :: GoConversationToNet( GoConversation* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoConversation* GoConversation :: NetToGoConversation( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetConversation() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
