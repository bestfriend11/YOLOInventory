//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIDialogueHandler.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "Precomp_Game.h"
#include "UIDialogueHandler.h"
#include "World.h"
#include "UI_Textbox.h"
#include "UI_Shell.h"
#include "GoConversation.h"
#include "Gps_manager.h"
#include "GoAspect.h"
#include "GoMind.h"
#include "GoInventory.h"
#include "WorldState.h"
#include "worldmessage.h"
#include "siege_compass.h"
#include "victory.h"
#include "job.h"
#include "UIStoreManager.h"
#include "GoStore.h"
#include "UIPartyManager.h"
#include "UIGameConsole.h"
#include "ui_animation.h"
#include "ui_tab.h"
#include "UICommands.h"
#include "GameAuditor.h"
#include "UIMenuManager.h"


UIDialogueHandler::UIDialogueHandler()
	: m_talker( GOID_INVALID )
	, m_member( GOID_INVALID )
	, m_scrollRate( 20.0f )
	, m_bMiniMap( false )
	, m_currentSample( GPGSound::INVALID_SOUND_ID )
	, m_bUpdate( false )
	, m_bRestarted( false )
	, m_bNextUpdate( false )
	, m_bTalkAttempt( false )
	, m_bJournalSet( false )
{
	// Register the dialog callback
	gWorld.RegisterDialogueCallback( makeFunctor( *this, &UIDialogueHandler::DialogueCallback ) );

	FastFuelHandle hSettings( "config:global_settings:journal_settings" );
	if ( hSettings )
	{
		hSettings.Get( "chapter_color", m_chapterColor );
		hSettings.Get( "active_quest_color", m_activeQuestColor );
		hSettings.Get( "completed_quest_color", m_completedQuestColor );
		hSettings.Get( "speaker_color", m_speakerColor );
	}
}


void UIDialogueHandler::Deinit()
{
	m_currentSample = GPGSound::INVALID_SOUND_ID;
	m_talker		= GOID_INVALID;
	m_member		= GOID_INVALID;
	m_bUpdate		= false;	
	m_bMiniMap		= false;	
	m_bUpdate		= false;
	m_bRestarted	= false;
	m_bNextUpdate	= false;
	m_bTalkAttempt	= false;
	m_bJournalSet	= false;
}


void UIDialogueHandler::NisUpdate( float deltaTime )
{
	if ( m_bUpdate )
	{
		m_elapsedTime += deltaTime;
		if ( m_elapsedTime >= m_duration )
		{
			GoHandle hObject( GetTalkerGUID() );
			hObject->GetMind()->RSStop( AO_COMMAND );							
			
			GoHandle hMember( GetMemberGUID() );
			if( hMember )
			{
				hMember->GetMind()->SDoJob( MakeJobReq( JAT_LISTEN, JQ_ACTION, QP_FRONT, AO_USER, GetTalkerGUID() ) );
			}
		}		
	}

	RequestResetDialogue();
}

void UIDialogueHandler::Update( float /* deltaTime */ )
{
	RequestResetDialogue();

	GoHandle hMember( GetMemberGUID() );
	if ( hMember.IsValid() && !::IsAlive( hMember->GetAspect()->GetLifeState() ) )
	{
		gUIStoreManager.DeactivateStore();
	}
}


void UIDialogueHandler::DialogueCallback( Goid talker, Goid member )
{
	if ( gUIStoreManager.GetAutoActivator() != GOID_INVALID )
	{
		if ( member == gUIStoreManager.GetAutoActivator() )
		{
			gUIStoreManager.ActivateStore( GoHandle( talker ), GoHandle( member ) );
			gUIStoreManager.SetAutoActivator( GOID_INVALID );
			return;
		}
	}

	if ( m_currentSample != GPGSound::INVALID_SOUND_ID )
	{
		gSoundManager.ClearStreamStopCallback( m_currentSample );
	}

	if ( gWorldState.GetCurrentState() == WS_SP_NIS || gWorldState.GetPendingState() == WS_SP_NIS )
	{		
		GoHandle hOldTalker( GetTalkerGUID() );	
		if ( hOldTalker.IsValid() && m_currentSample != GPGSound::INVALID_SOUND_ID && gSoundManager.IsStreamPlaying( m_currentSample ) )
		{
			gpwarning( "Trying to talk again while in middle of a conversation.  This is valid in special cases.  Request ignored." );
			return;
		}
	}

	gUIPartyManager.CloseAllCharacterMenus();
	gUIShell.ShowGroup( "shop", false );
	gUIShell.ShowGroup( "potential_member", false );
	gUIShell.ShowGroup( "buy_packmule", false );
	gUIShell.ShowGroup( "party_gold", false );
	gUIShell.ShowGroup( "more", false );
	SetMemberGUID( member );
	m_bRestarted = false;
	SetTalkAttempt( false );
	
	GoHandle hTalker( talker );
	GoHandle hMember( member );
	if ( hTalker.IsValid() && hMember.IsValid() )
	{
		GoConversation::ConversationInfo convInfo;
		convInfo.bNis		= false;
		convInfo.scrollRate = 0.0f;
		convInfo.duration	= 0.0f;
		
		hTalker->GetConversation()->GetDialogue( convInfo );
		SetTalkerGUID( talker );
	
		SetDialogueText( convInfo.sText.c_str() );
		SetDialogueSample( convInfo.sSample );
		SetScrollRate( convInfo.scrollRate );
		
		if ( ::IsMultiPlayer() && ::IsServerRemote() && hTalker && hTalker->HasComponent( "check_level" ) )
		{
			WorldMessage msg;
			msg.SetEvent( WE_REQ_TALK_BEGIN );
			msg.SetSendFrom( member );
			msg.SetSendTo( talker );			
			hTalker->GetComponent( "check_level" )->HandleMessage( msg );
		}
		
		if ( gWorldState.GetCurrentState() == WS_SP_NIS || gWorldState.GetPendingState() == WS_SP_NIS && convInfo.bNis )
		{
			m_duration = convInfo.duration;
			m_elapsedTime = 0.0f;
			
			if ( m_duration > 0.0f )
			{
				m_bUpdate = true;
			}

			UITextBox * pText = (UITextBox *)gUIShell.FindUIWindow( "text_subtitle", "nis_subtitle" );
			if ( pText && !convInfo.sText.empty() )
			{		
				pText->SetScrollRate( convInfo.scrollRate );
				pText->SetScrollThrough( true );
				pText->SetText( GetDialogueText() );
				gUIShell.ShowInterface( "nis_subtitle" );
				if ( !convInfo.sSample.empty() )
				{	
					PlayStream( convInfo.sSample );					
				}
			}
		}
		else if ( !convInfo.bNis )
		{
			UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "dialogue_box" );
			if (( pTextBox ) && ( !convInfo.sText.empty() ))
			{
				pTextBox->SetScrollThrough( false );
				pTextBox->SetLeadElement( 0 );
				pTextBox->RecalculateSlider();

				gUIShell.ShowGroup( "dialog_close", true );
				if ( !convInfo.sChoice.empty() )
				{
					gUIShell.ShowGroup( convInfo.sChoice, true, true );					

					if ( convInfo.sChoice.same_no_case( "buy_packmule" ) )
					{
						gUIStoreManager.SetActiveStore( hTalker->GetGoid() );
						gUIStoreManager.SetActiveStoreBuyer( hMember->GetGoid() );

						GoHandle hStore( gUIStoreManager.GetActiveStore() );
						
						if ( hStore.IsValid() )
						{
							UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_pack_cost", "dialogue_box" );
							if ( pText )
							{
								gpwstring sCost;
								sCost.assignf( gpwtranslate( $MSG$ "Cost: %d" ), hStore->GetStore()->GetHireCost() );
								pText->SetText( sCost );
							}
						}
					}
					else if ( convInfo.sChoice.same_no_case( "potential_member" ) )
					{
						UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_pack_cost", "dialogue_box" );
						if ( pText )
						{
							if ( hTalker->GetAspect()->GetGoldValue() > 0 )
							{
								gpwstring sCost;
								sCost.assignf( gpwtranslate( $MSG$ "Cost: %d" ), hTalker->GetAspect()->GetGoldValue() );
								pText->SetText( sCost );
								pText->SetVisible( true );
							}
							else
							{
								pText->SetVisible( false );
							}
						}

						gUIShell.ShowGroup( "dialog_close", false );
					}

					if ( convInfo.sChoice.same_no_case( "buy_packmule" ) || convInfo.sChoice.same_no_case( "potential_member" ) )
					{
						gUIStoreManager.SetStoreActive( true );
						UIText * pGold = (UIText *)gUIShell.FindUIWindow( "text_available_gold" );
						if ( pGold )
						{
							gpwstring sGold;
							sGold.assignf( gpwtranslate( $MSG$ "Available Gold: %d" ), gUIStoreManager.GetBuyerGold() );
							pGold->SetText( sGold );

							int gold = 0;
							if ( convInfo.sChoice.same_no_case( "buy_packmule" ) )
							{
								UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_buy_packmule", "dialogue_box" );
								if ( pButton )
								{
									pButton->EnableButton();
								}

								GoHandle hStore( gUIStoreManager.GetActiveStore() );
								if ( hStore.IsValid() )
								{
									gold = hStore->GetStore()->GetHireCost();
									if ( gold > hMember->GetInventory()->GetGold() )
									{
										pButton = (UIButton *)gUIShell.FindUIWindow( "button_buy_packmule", "dialogue_box" );
										if ( pButton )
										{
											pButton->DisableButton();
										}
									}
								}
							}
							else if ( convInfo.sChoice.same_no_case( "potential_member" ) )
							{
								gold = hTalker->GetAspect()->GetGoldValue();
							}

							if ( gold > 0 )
							{							
								pGold->SetVisible( true );
							}
							else
							{
								pGold->SetVisible( false );
							}
						}						
					}					

					// Set the store to active if there's a shop option
					if ( convInfo.sChoice.same_no_case( "shop" ) )
					{
						gUIStoreManager.SetStoreActive( true );
					}
				}

				// Generic Button 1
				if ( !convInfo.sButton1Name.empty() && !convInfo.sButton1Value.empty() )
				{
					gUIShell.ShowGroup( "generic_button_1", true, false, "dialogue_box" );
					UIText * pButtonText = (UIText *)gUIShell.FindUIWindow( "text_1", "dialogue_box" );
					if ( pButtonText )
					{
						pButtonText->SetText( ReportSys::TranslateW( convInfo.sButton1Name ) );
					}
					SetGenericButton1Value( convInfo.sButton1Value );
				}
				else
				{
					gUIShell.ShowGroup( "generic_button_1", false, false, "dialogue_box" );
				}

				// Generic Button 2
				if ( !convInfo.sButton2Name.empty() && !convInfo.sButton2Value.empty() )
				{
					gUIShell.ShowGroup( "generic_button_2", true, false, "dialogue_box" );
					UIText * pButtonText = (UIText *)gUIShell.FindUIWindow( "text_2", "dialogue_box" );
					if ( pButtonText )
					{
						pButtonText->SetText( ReportSys::TranslateW( convInfo.sButton2Name ) );
					}
					SetGenericButton2Value( convInfo.sButton2Value );
				}
				else
				{
					gUIShell.ShowGroup( "generic_button_2", false, false, "dialogue_box" );
				}
				
				pTextBox->SetText( GetDialogueText() );
				pTextBox->SetScrollRate( convInfo.scrollRate );
				
				if ( gUIShell.DoWindowsOverlap( "compass_drag_window", "compass_hotpoints",
												"dialogue_main_bg", "dialogue_box" ) )
				{
					gSiegeEngine.GetCompass().SetVisible( false );
					gUIShell.HideInterface( "compass_hotpoints" );
				}
				
				gUIShell.ShowInterface( "dialogue_box" );
				gUIGameConsole.EnableConsoleUpdate( false );											

				if ( !convInfo.sSample.empty() )
				{
					gUIShell.ShowGroup( "dialog_sound", true );

					UIRadioButton * pWindow = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_soundoff" );
					if ( pWindow )
					{
						if ( gSoundManager.IsEnabled() )
						{							
							PlayStream( convInfo.sSample );
							UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_replay", "dialogue_box" );
							if ( pButton )
							{									
								pButton->DisableButton();
							}

							if ( pWindow->GetCheck() )
							{
								MuteDialogue();
							}
							else
							{
								UnMuteDialogue();
							}
						}						
					}			
				}
				else
				{
					gUIShell.ShowGroup( "dialog_sound", false );
					StopDialogue( false );
				}
			}
			else
			{
				ExitDialogue( false );
			}
		}		
	}
}


void UIDialogueHandler::StopSampleCB( UINT32 param )
{	
	if ( gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		GoHandle hObject( GetTalkerGUID() );		

		if ( hObject.IsValid() )
		{
			RSSendReqTalkEnd( GetMemberGUID(), GetTalkerGUID() );
			hObject->GetMind()->RSStop( AO_COMMAND );				

			UITextBox * pText = (UITextBox *)gUIShell.FindUIWindow( "text_subtitle", "nis_subtitle" );
			if ( pText )
			{
				pText->SetScrollThrough( false );
				pText->SetText( L"" );
			}	
		}		
	}
	else
	{	
		if ( param == m_currentSample )
		{
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_replay", "dialogue_box" );
			if ( pButton )
			{				
				pButton->EnableButton();
			}				
		}
	}		

	if ( param == m_currentSample )
	{
		m_currentSample = GPGSound::INVALID_SOUND_ID;
	}
}


void UIDialogueHandler::RequestResetDialogue()
{
	if ( m_bRestarted && m_bNextUpdate )
	{		
		PlayStream( GetDialogueSample() );		
		m_bRestarted = false;
		m_bNextUpdate = false;
	}
	else
	{
		m_bNextUpdate = true;
	}
}


void UIDialogueHandler::ResetDialogue()
{
	GoHandle hTalker( GetTalkerGUID() );
	if ( hTalker.IsValid() )
	{
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "dialogue_box" );
		if (( pTextBox ) && ( !GetDialogueText().empty() ))
		{
			pTextBox->SetText( GetDialogueText() );
			pTextBox->SetScrollRate( GetScrollRate() );
			gUIShell.ShowInterface( "dialogue_box" );
			if ( !GetDialogueSample().empty() )
			{
				UIRadioButton * pWindow = (UIRadioButton *)gUIShell.FindUIWindow( "radio_button_soundoff" );
				if ( pWindow )
				{
					if ( !pWindow->GetCheck() && gSoundManager.IsEnabled() )
					{						
						m_bRestarted = true;						
						
						UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_replay", "dialogue_box" );
						if ( pButton )
						{	
							pButton->DisableButton();													
						}					
					}
				}
			}
		}
	}
}


void UIDialogueHandler::MuteDialogue()
{
	gSoundManager.SetStreamVolume( m_currentSample, GPGSound::SOUND_MIN_VOLUME );
}


void UIDialogueHandler::UnMuteDialogue()
{
	gSoundManager.SetStreamVolume( m_currentSample, gSoundManager.GetStreamVolume( GPGSound::STRM_VOICE ) );
}


void UIDialogueHandler::StopDialogue( bool bStopScroll )
{	
	gSoundManager.StopStream( m_currentSample );
	m_currentSample = GPGSound::INVALID_SOUND_ID;

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_replay", "dialogue_box" );
	if ( pButton )
	{				
		pButton->EnableButton();
	}		
	if ( bStopScroll )
	{
		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "dialogue_box" );
		if ( pTextBox )
		{
			pTextBox->SetScrolling( true );
		}
	}
}


bool UIDialogueHandler::ExitDialogue( bool bStopTalking, bool bExplicitExit )
{	
	if ( !gUIShell.IsInterfaceVisible( "dialogue_box" ) &&
		 !gUIShell.IsInterfaceVisible( "nis_subtitle" ) &&
		 !gUIShell.IsInterfaceVisible( "store" ) )
	{
		return false;		
	}

	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		gSiegeEngine.GetCompass().SetVisible( true );
		gUIShell.ShowInterface( "compass_hotpoints" );
	}

	gUIShell.HideInterface( "hire_stats" );
	
	bool bPlayingGoodbye = false;
	if ( bExplicitExit )
	{		
		GoHandle hObject( GetTalkerGUID() );
		if ( hObject.IsValid() )
		{
			gpstring sSample;
			hObject->GetConversation()->GetGoodbyeSample( sSample );
			if ( !sSample.empty() )
			{				
				PlayStream( sSample );
				bPlayingGoodbye = true;
			}
		}
	}

	gUIShell.HideInterface( "dialogue_box" );
	gUIShell.HideInterface( "nis_subtitle" );		

	m_bUpdate = false;

	if ( bStopTalking && GetTalkerGUID() != GOID_INVALID )
	{		
		GoHandle hObject( GetTalkerGUID() );
		if ( hObject.IsValid() )
		{	
			if ( hObject->IsInActiveScreenWorldFrustum() )
			{
				RSSendReqTalkEnd( GetMemberGUID(), GetTalkerGUID() );
			}
			hObject->GetMind()->RSStop( AO_COMMAND );	

			if ( !bPlayingGoodbye && !m_sDialogueSample.empty() )
			{
				gSoundManager.StopStream( m_currentSample );			
			}
			else if ( m_sDialogueSample.empty() )
			{
				m_currentSample = GPGSound::INVALID_SOUND_ID;
			}
		}
	}

	UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_replay", "dialogue_box" );
	if ( pButton )
	{			
		pButton->EnableButton();	
	}

	SetTalkerGUID( GOID_INVALID );

	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		gUIGameConsole.EnableConsoleUpdate( true );
		gUIAnimation.SetDisableUpdate( false );
	}		

	return true;
}


void UIDialogueHandler::AdvanceDialogue()
{
	RSSendReqTalkEnd( GetMemberGUID(), GetTalkerGUID() );
	gUIGame.GetUICommands()->CommandTalk( GoHandle( GetMemberGUID() ), GoHandle( GetTalkerGUID() ) );
	gUIShell.SetRolloverName( "" );
}


void UIDialogueHandler::GenericButton1()
{
	RSSetButtonValue( m_sGenericButton1Value );
	AdvanceDialogue();
}


void UIDialogueHandler::GenericButton2()
{	
	RSSetButtonValue( m_sGenericButton2Value );
	AdvanceDialogue();
}


void UIDialogueHandler::RSSetButtonValue( const gpstring & sValue )
{
	FUBI_RPC_THIS_CALL( RSSetButtonValue, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;	

	gGameAuditor.SetBool( sValue, true );
}


void UIDialogueHandler::RSSendReqTalkEnd( Goid member, Goid talker )
{
	FUBI_RPC_THIS_CALL( RSSendReqTalkEnd, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	SendWorldMessage( WE_REQ_TALK_END, member, talker );
}


void UIDialogueHandler::ShowHireStats()
{	
	GoHandle hTalker( GetTalkerGUID() );
	if ( hTalker.IsValid() )
	{		
		gUIPartyManager.ShowHireStats( hTalker );
	}
}


void UIDialogueHandler::PlayStream( gpstring sSample )
{
	bool bStop = true;
	DWORD oldSample = m_currentSample;
	if ( !gSoundManager.IsStreamPlaying( oldSample ) )
	{
		bStop = false;
	}

	m_currentSample = gSoundManager.PlayStream( sSample, GPGSound::STRM_VOICE, false );
	gSoundManager.SetStreamStopCallback( m_currentSample, makeFunctor( *this, &UIDialogueHandler::StopSampleCB ) );

	if ( bStop )
	{
		gSoundManager.StopStream( oldSample );
	}
}

bool UIDialogueHandler::ToggleJournal()
{	
	if ( m_bJournalSet )
	{
		return true;
	}

	gSoundManager.PlaySample( "s_e_gui_spell_book" );

	m_bJournalSet = true;
	if ( !gUIShell.IsInterfaceVisible( "journal" ) && !gUIMenuManager.IsOptionsMenuActive() )
	{		
		gUIPartyManager.CloseAllCharacterMenus();
		gUIShell.ActivateInterface( "ui:interfaces:backend:journal", true );		
		gUIDialogueHandler.ShowQuests( gVictory.GetLastUpdatedQuest() );		

		UICheckbox * pCheckBox = (UICheckbox *)gUIShell.FindUIWindow( "button_quest_log", "data_bar" );
		if ( pCheckBox )
		{
			pCheckBox->SetState( true );
		}
	}
	else
	{
		ExitJournal();				
	}

	m_bJournalSet = false;

	return true;
}


void UIDialogueHandler::ShowQuests( int selected )
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_quest_indicator", "data_bar" );
	if ( pWindow )
	{
		gUIAnimation.RemoveAnimationsForWindow( pWindow );
		pWindow->SetVisible( false );
	}

	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_quests", "journal" );
	if ( pList )
	{
		gUIShell.ShowGroup( "quest_listbox", true, false, "journal" );
		gUIShell.ShowGroup( "quest_dialogues", false, false, "journal" );

		if ( selected == -1 )
		{
			return;
		}

		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_desc", "journal" );
		if ( pTextBox )
		{
			pTextBox->SetText( gpwstring::EMPTY );
		}
		
		pList->RemoveAllElements();

		if ( gVictory.HasChapters() )
		{
			const Victory::ChapterColl chapters = gVictory.GetChapters();
			Victory::ChapterColl::const_iterator iChapter;
			for ( iChapter = chapters.begin(); iChapter != chapters.end(); ++iChapter )
			{
				bool bShowChapter = false;			

				Victory::Quest* quest = gVictory.FindQuestByName( (*iChapter).sName );
				if ( quest && ( quest->bActive || gVictory.IsQuestCharActive( quest->sName ) ) )
				{					
					if ( !bShowChapter )
					{
						pList->AddElement( (*iChapter).sScreenName, -1, m_chapterColor );
						bShowChapter = true;
					}

					DWORD color = m_activeQuestColor;
					if ( ( quest->bActive || gVictory.IsQuestCharActive( quest->sName ) ) && ( quest->bCompleted || gVictory.IsQuestCharCompleted( quest->sName ) ) )
					{
						color = m_completedQuestColor;
					}
					
					pList->AddElement( quest->sScreenName, quest->id, color );				
					
					if ( selected == quest->id )
					{
						pList->SelectElement( quest->id );							
						SelectQuest();
					}					
				}
				
				if ( !bShowChapter && (*iChapter).bActive )
				{
					pList->AddElement( (*iChapter).sScreenName, -1, m_chapterColor );
					bShowChapter = true;
				}
			}
		}
		else
		{
			Victory::QuestColl quests = gVictory.GetQuests();
			Victory::QuestColl::iterator i;
			for ( i = quests.begin(); i != quests.end(); ++i )
			{	
				if ( (*i).bActive || gVictory.IsQuestCharActive( (*i).sName ) )
				{					
					DWORD color = m_activeQuestColor;
					if ( ( (*i).bActive || gVictory.IsQuestCharActive( (*i).sName ) ) && ( (*i).bCompleted || gVictory.IsQuestCharCompleted( (*i).sName ) ) )
					{
						color = m_completedQuestColor;
					}
					
					pList->AddElement( (*i).sScreenName, (*i).id, color );				
					
					if ( selected == (*i).id )
					{
						pList->SelectElement( (*i).id );							
						SelectQuest();
					}										
				}
			}
		}
	}
}


void UIDialogueHandler::JournalButton1()
{
	gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
	UIButton * pButton1 = (UIButton *)gUIShell.FindUIWindow( "button_journal_1", "journal" );	
	UIText * pText2		= (UIText *)gUIShell.FindUIWindow( "text_journal_2", "journal" );
	if ( pButton1 && pText2 )
	{
		gUIShell.ShowGroup( "journal_button_1", false, false, "journal" );

		UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_quests", "journal" );
		if ( pList )
		{
			pText2->SetText( gpwtranslate( $MSG$ "Back" ) );

			UIButton * pButton2 = (UIButton *)gUIShell.FindUIWindow( "button_journal_2", "journal" );
			pButton2->SetRolloverKey( "journal_back" );

			if ( pList->GetSelectedTag() != -1 )
			{
				ShowDialogues( pList->GetSelectedTag() );
			}
			else
			{
				ShowChapter( pList->GetSelectedText() );
			}
		}		
	}	
}


void UIDialogueHandler::JournalButton2()
{	
	UIButton * pButton1 = (UIButton *)gUIShell.FindUIWindow( "button_journal_1", "journal" );
	UIText * pText1		= (UIText *)gUIShell.FindUIWindow( "text_journal_1", "journal" );

	UIButton * pButton2 = (UIButton *)gUIShell.FindUIWindow( "button_journal_2", "journal" );
	UIText * pText2		= (UIText *)gUIShell.FindUIWindow( "text_journal_2", "journal" );
	if ( pButton1 && pText1 && pButton2 && pText2 )
	{
		if ( pButton1->GetVisible() )
		{
			pButton2->SetRolloverKey( "journal_close" );
			pText2->SetText( gpwtranslate( $MSG$ "Close" ) );
			ExitJournal();
		}
		else 
		{
			gUIShell.ShowGroup( "journal_button_1", true, false, "journal" );
			pButton2->SetRolloverKey( "journal_close" );
			pText2->SetText( gpwtranslate( $MSG$ "Close" ) );
			ShowQuests( -1 );
			StopQuestDialogue();
			gSoundManager.PlaySample( "s_e_frontend_tiny_button", false );
		}
	}
}


void UIDialogueHandler::SelectQuest()
{
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_quests", "journal" );
	if ( pList )
	{
		int quest = pList->GetSelectedTag();

		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_desc", "journal" );
		if ( pTextBox )
		{
			pTextBox->SetText( L"" );
		}

		UIButton * pPlayButton = (UIButton *)gUIShell.FindUIWindow( "button_journal_1", "journal" );
		if ( pPlayButton )
		{
			pPlayButton->EnableButton();
		}		

		UIWindow * pPicture = gUIShell.FindUIWindow( "window_quest_picture", "journal" );				
		if ( pPicture )
		{
			pPicture->SetHasTexture( false );
		}

		UIWindow * pCompleted = gUIShell.FindUIWindow( "window_quest_completed", "journal" );
		if ( pCompleted )
		{
			pCompleted->SetVisible( false );
		}

		if ( quest != -1 )
		{
			Victory::Quest* findQuest = gVictory.FindQuestById( quest );
			if ( findQuest && pTextBox )
			{
				gpwstring sText;
				if ( findQuest->bCompleted || gVictory.IsQuestCharCompleted( findQuest->sName ) )
				{
					sText.assignf( gpwtranslate( $MSG$ "<c:0x%x>Quest Completed</c>\n" ), m_completedQuestColor );
					if ( pCompleted )
					{
						pCompleted->SetVisible( true );
						if ( !findQuest->bViewed )
						{							
							pCompleted->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
							gVictory.SetQuestViewed( findQuest->id, true );
						}								
					}
				}
				sText.append( findQuest->sActiveDesc.c_str() );
				pTextBox->SetText( sText );
			}					
			
			if ( pPicture && !findQuest->sQuestImage.empty() )
			{
				pPicture->SetHasTexture( true );
				pPicture->LoadTexture( findQuest->sQuestImage );
			}
		}
		else
		{
			Victory::Chapter* findChapter = gVictory.FindChapterByName( pList->GetSelectedText() );
			if ( findChapter )
			{
				if ( pTextBox )
				{
					pTextBox->SetText( findChapter->sScreenName );
				}	
				if ( pPicture && !findChapter->sImage.empty() )
				{
					pPicture->SetHasTexture( true );
					pPicture->LoadTexture( findChapter->sImage );
				}
			}
		}
	}	
}


void UIDialogueHandler::ShowDialogues( int /* questId */ )
{
	UIListbox * pList = (UIListbox *)gUIShell.FindUIWindow( "listbox_quests", "journal" );
	if ( pList )
	{
		gUIShell.ShowGroup( "quest_listbox", false, false, "journal" );
		gUIShell.ShowGroup( "quest_dialogues", true, false, "journal" );

		int quest = pList->GetSelectedTag();

		UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "textbox_dialogues", "journal" );
		if ( pTextBox )
		{
			gpstring sSample;
			bool bFirst = true;
			gpwstring sText;
			Victory::QuestConversations conversations;
			Victory::QuestConversations::iterator i;
			gVictory.GetQuestConversations( quest, conversations );
			for ( i = conversations.begin(); i != conversations.end(); ++i )
			{
				sText.appendf( L"<c:0x%x>%s</c>\n", m_speakerColor, (*i).sSpeaker.c_str() );
				sText.appendf( L"%s\n\n", ReportSys::TranslateW( (*i).sConversation ) );

				if ( bFirst )
				{
					sSample = (*i).sSample;
					bFirst = false;
				}
			}

			pTextBox->SetText( sText );

			if ( m_currentSample != GPGSound::INVALID_SOUND_ID )
			{
				gSoundManager.ClearStreamStopCallback( m_currentSample );
			}

			if ( !sSample.empty() )
			{
				PlayStream( sSample );
			}
		}
	}	
}


void UIDialogueHandler::ShowChapter( const gpwstring & sChapter )
{
	Victory::Chapter chapter;
	if ( !gVictory.GetChapter( sChapter, chapter ) )
	{
		return;
	}

	gUIShell.ShowGroup( "quest_listbox", false, false, "journal" );
	gUIShell.ShowGroup( "quest_dialogues", true, false, "journal" );

	Victory::ChapterEventColl::iterator iEvent;
	for ( iEvent = chapter.events.begin(); iEvent != chapter.events.end(); ++iEvent )
	{
		if ( (*iEvent).order == chapter.currentOrder )
		{
			UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_desc", "journal" );
			if ( pTextBox )
			{
				pTextBox->SetText( chapter.sScreenName );
			}

			pTextBox = (UITextBox *)gUIShell.FindUIWindow( "textbox_dialogues", "journal" );
			if ( pTextBox )
			{
				pTextBox->SetText( ReportSys::TranslateW( (*iEvent).sDescription ) );
			}
			
			if ( !(*iEvent).sSample.empty() )
			{
				PlayStream( (*iEvent).sSample );
			}

			break;
		}
	}
}


void UIDialogueHandler::StopQuestDialogue( bool bClearCallback )
{
	if ( bClearCallback )
	{
		gSoundManager.ClearStreamStopCallback( m_currentSample );
	}

	gSoundManager.StopStream( m_currentSample );
	m_currentSample = GPGSound::INVALID_SOUND_ID;
}


void UIDialogueHandler::ExitJournal()
{
	if ( gUIShell.IsInterfaceVisible( "journal" ) )
	{
		if ( gSoundManager.IsStreamPlaying( m_currentSample ) )
		{
			StopQuestDialogue();
		}
		gUIShell.MarkInterfaceForDeactivation( "journal" );

		UICheckbox * pCheckBox = (UICheckbox *)gUIShell.FindUIWindow( "button_quest_log", "data_bar" );
		if ( pCheckBox )
		{
			pCheckBox->SetState( false );
		}
	}
}

