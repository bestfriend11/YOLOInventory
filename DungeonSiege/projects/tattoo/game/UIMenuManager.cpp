//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIMenuManager.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "Precomp_Game.h"
#include "AppModule.h"
#include "filesysutils.h"
#include "NetPipe.h"
#include "Player.h"
#include "RapiImage.h"
#include "Server.h"
#include "siege_compass.h"
#include "Siege_Engine.h"
#include "siege_engine.h"
#include "siege_hotpoint_database.h"
#include "TattooGame.h"
#include "ui.h"
#include "ui_animation.h"
#include "ui_chatbox.h"
#include "ui_combobox.h"
#include "ui_dockbar.h"
#include "ui_editbox.h"
#include "ui_listbox.h"
#include "ui_shell.h"
#include "ui_statusbar.h"
#include "ui_text.h"
#include "ui_textbox.h"
#include "UICamera.h"
#include "UIDialogueHandler.h"
#include "UIGame.h"
#include "UIGameConsole.h"
#include "UIMenuManager.h"
#include "UIMultiplayer.h"
#include "UIOptions.h"
#include "UIPartyManager.h"
#include "UIFrontend.h"
#include "WorldMap.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldTime.h"
#include "Victory.h"
#include "gps_manager.h"


UIMenuManager::UIMenuManager()
{
	Init();
	m_scrollText				= 5.0f;
	m_maxText					= 6;
	m_bExitAfterSave			= false;
	m_bStatusDockTop			= false;
	m_bEnableTips				= true;
	m_TempFadeDuration			= 0.0f;
	m_bModalActive				= false;
	m_bClientRequestedExit		= false;
}


void UIMenuManager::Init()
{
	m_bGameWasPaused			= false;
	m_bOptionsMenuActive		= false;	
	m_bLoadSaveActive			= false;
	m_cameraYTrack				= -1.0;
	m_bExitDialogActive			= false;
	m_bGoldTradeDialogActive	= false;
	m_bGoldInvalidDialogActive	= false;
	m_bMultiRespawn				= false;
	m_bQuickSaving				= false;
	m_bAutoSaving				= false;
	m_bSaving					= false;
	m_bPastUpdate				= false;
	m_bLastUpdate				= false;
	m_bExitAfterSave			= false;
	m_bStatusDockTop			= false;
	m_dwQuickColor				= 0xffffffff;
	m_dwAutoColor				= 0xffffffff;
	m_currentTip				= 1;
	m_TempFadeDuration			= 0;
	m_bIsLoading				= false;
	m_bModalActive				= false;	
	m_bDefeated					= false;
	m_bIgnoreAzimuth			= false;
	m_bClientRequestedExit		= false;
	m_TradeShift				= 0;
	
	m_sFadedInterface.clear();

	FastFuelHandle hSaveColors( "config:global_settings:save_game_settings" );
	if ( hSaveColors.IsValid() )
	{
		hSaveColors.Get( "quick_save_color", m_dwQuickColor );
		hSaveColors.Get( "auto_save_color", m_dwAutoColor );
	}

	gVictory.RegisterGuiDefeatCallback( makeFunctor( *this, &UIMenuManager::DefeatCallback ) );
	gVictory.RegisterGuiVictoryCallback( makeFunctor( *this, &UIMenuManager::MpVictoryCallback ) );

	ShowServerTrafficBacklogIcon( false );
	ShowServerTimeoutIcon( false );
}


void UIMenuManager::DeInit()
{
	bool bContinue = true;
	while ( bContinue )
	{
		UIWindow * pFadeWindow = gUIShell.FindUIWindow( "fade_window" );
		if ( pFadeWindow )
		{
			gUIShell.DeactivateInterface( pFadeWindow->GetInterfaceParent() );
		}
		else
		{
			bContinue = false;
		}
	}		
}


bool UIMenuManager::OptionsMenu()
{
	if ( GetIsDefeated() )
	{
		return true;
	}

	gUIGame.HideGUIEditBox();

	if ( gUI.GetGameGUI().IsInterfaceVisible( "in_game_menu" ) )
	{
		if ( ::IsSinglePlayer() && !m_bGameWasPaused )
		{
			gTattooGame.RSUserPause( false );			
		}
		gUI.GetGameGUI().HideInterface( "in_game_menu" );
		m_bOptionsMenuActive = false;
	}
	else
	{
		UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "load_button" );
		if ( pButton )
		{
			if ( gTattooGame.HasCampaignSaveGameInfos( true ) )
			{
				pButton->EnableButton();				
			}
			else
			{
				pButton->DisableButton();
			}
		}

		gUIPartyManager.CloseAllCharacterMenus();

		if ( ::IsSinglePlayer() )
		{
			m_bGameWasPaused = gAppModule.IsUserPaused();
			gTattooGame.RSUserPause( true );
			GuiPause( true );
		}
		else if ( ::IsMultiPlayer() && gNetPipe.HasOpenSession() )
		{
			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "exit_game_button" );
			if ( pButton && ::IsServerLocal() )
			{
				if ( gWorldState.GetCurrentState() != WS_MP_INGAME_JIP )
				{
					pButton->SetVisible( true );
				}
				else
				{
					pButton->SetVisible( false );
				}
			}

			gUIMultiplayer.ShowGameSettingsList();

			NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
			UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_game_name", "in_game_menu" );
			if ( pText )
			{
				pText->SetText( session->GetSessionName() );
			}
		}

		gUI.GetGameGUI().ShowInterface( "in_game_menu" );
		m_bOptionsMenuActive = true;
	}

	return true;
}


void UIMenuManager::OptionsDialog()
{
}


void UIMenuManager::ExitGameDialog()
{
	gUIPartyManager.CloseAllCharacterMenus();	

	if ( ::IsSinglePlayer() )
	{
		gTattooGame.RSUserPause( true );
		GuiPause( true );
	}

	gUIGame.ShowDialog( DIALOG_EXIT_GAME );
}

void UIMenuManager::ExitWindowsDialog()
{
	gUIPartyManager.CloseAllCharacterMenus();

	gUIOptions.Deactivate();

	m_bGameWasPaused = gAppModule.IsUserPaused(); 

	if ( ::IsSinglePlayer() )
	{
		gTattooGame.RSUserPause( true );
		GuiPause( true );
	}

	SetExitDialogActive( true );
	gUIGame.ShowDialog( DIALOG_EXIT_WINDOWS );
}


bool UIMenuManager::LoadGameDialog()
{
	if ( ::IsMultiPlayer() || gUIShell.GetItemActive() )
	{
		return true;
	}

	gUIPartyManager.CloseAllCharacterMenus();
	gTattooGame.RSUserPause( true );
	gUIShell.HideInterface( "in_game_menu" );
	gUIShell.HideInterface( "defeat_dialog" );
	gUIShell.ShowInterface( "loadsave_game" );
	gUIShell.ShowGroup( "save_group", false, false, "loadsave_game" );

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "loadsave_game_title" );
	if( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "LOAD GAME" ) );
	}

	pText = (UIText *)gUIShell.FindUIWindow( "save_text_ok", "loadsave_game" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Load" ) );
	}

	gpwstring selectedGame;

	ConstructSaveList( true );

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "loadsave_game_name_edit_box" );
	if( pEditBox )
	{
		pEditBox->SetText( selectedGame );
	}

	SelectSaveGame();	

	m_bIsLoading = true;
	m_bLoadSaveActive = true;

	return true;
}


bool UIMenuManager::SaveGameDialog()
{
	// $$$ note: this function has a ton of shared code with uifrontend.cpp
	//     and loadgamedialog() - consider moving it all out to a helper func. -sb

	if ( ::IsMultiPlayer() || GetIsDefeated() || gUIShell.GetItemActive() )
	{
		return true;
	}

	gUIPartyManager.CloseAllCharacterMenus();
	if ( gUIGame.GetBackendDialog() == DIALOG_EXIT_GAME )
	{
		if ( !(::IsMultiPlayer() && gUIMultiplayer.GetMultiplayerMode() == MM_GUN) )		
		{		
			m_bExitAfterSave = true;
		}
	}

	gTattooGame.RSUserPause( true );
	gUI.GetGameGUI().HideInterface( "in_game_menu" );
	gUI.GetGameGUI().ShowInterface( "loadsave_game" );
	gUIShell.ShowGroup( "save_group", true, false, "loadsave_game" );

	UIText * pText = (UIText *)gUIShell.FindUIWindow( "loadsave_game_title" );
	if( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "SAVE GAME" ) );
	}

	pText = (UIText *)gUIShell.FindUIWindow( "save_text_ok", "loadsave_game" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Save" ) );
	}

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "loadsave_game_name_text" );
	if( pTextBox )
	{
		pTextBox->SetText( L"" );
	}
	
	ConstructSaveList( false );

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "loadsave_game_name_edit_box" );
	if( pEditBox )
	{
		pEditBox->SetText( L"" );
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "preview_window", "loadsave_game" );	
	if ( pWindow )
	{
		gUIShell.GetRenderer().DestroyTexture( pWindow->GetTextureIndex() );
		pWindow->SetTextureIndex( 0 );
		pWindow->SetHasTexture( false );
	}

	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_save_delete", "loadsave_game" );
	if ( pButton )
	{
		pButton->DisableButton();
	}

	if ( pEditBox )
	{
		
		float seconds = (float)gWorldTime.GetTime();
		int hours = 0, minutes = 0;
		::ConvertTime( hours, minutes, seconds );
		gpwstring sElapsedTimeText;
		sElapsedTimeText.assignf( gpwtranslate( $MSG$ "(%d-%02d-%02d)" ), hours, minutes, (int)seconds );

		gpwstring sSave;
		sSave.assignf( L"%s %s", gServer.GetScreenPlayer()->GetHeroName().c_str(), sElapsedTimeText.c_str() );
		pEditBox->SetText( sSave );
	}

	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
	if( pListbox )
	{
		pListbox->ClearSelection();
	}

	m_bIsLoading = false;
	m_bLoadSaveActive = true;	

	CheckLoadSaveEnable();	

	return true;
}


void UIMenuManager::ConstructSaveList( bool forRead )
{
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
	if( pListbox )
	{
		// get save infos
		gTattooGame.GetCampaignSaveGameInfos( m_saveInfoColl, forRead );

		// re-sort based on save time
		m_saveInfoColl.sort( SaveInfo::GreaterByFileTime() );

		// add each
		pListbox->RemoveAllElements();

		SaveInfoColl::iterator i;
		int listIndex = 0;

		// autosave first
		if ( forRead )
		{			
			for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
			{
				if ( (*i)->m_IsAutoSave )
				{
					pListbox->AddElement( (*i)->m_ScreenName, listIndex, m_dwAutoColor );
					
					if ( gTattooGame.GetLastSaveGame().same_no_case( (*i)->m_FileName ) )
					{
						pListbox->SelectElement( listIndex );
					}

					break;
				}

				++listIndex;
			}
				
			// quicksave second
			listIndex = 0;
			for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
			{
				if ( (*i)->m_IsQuickSave )
				{
					pListbox->AddElement( (*i)->m_ScreenName, listIndex, m_dwQuickColor );	
					
					if ( gTattooGame.GetLastSaveGame().same_no_case( (*i)->m_FileName ) )
					{
						pListbox->SelectElement( listIndex );
					}

					break;
				}

				++listIndex;
			}
		}

		// and the rest		
		listIndex = 0;
		for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
		{
			if ( !( (*i)->m_IsQuickSave || (*i)->m_IsAutoSave ) )
			{				
				pListbox->AddElement( (*i)->m_ScreenName, listIndex );				

				if ( gTattooGame.GetLastSaveGame().same_no_case( (*i)->m_FileName ) )
				{
					pListbox->SelectElement( listIndex );
				}
			}

			++listIndex;
		}
		
		UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_save_delete", "loadsave_game" );
		UIButton * pButtonOk = (UIButton *)gUIShell.FindUIWindow( "button_save_okay", "loadsave_game" );
		if ( pButton )
		{
			if ( m_saveInfoColl.empty() )
			{
				pButton->DisableButton();
				
				if ( m_bIsLoading )
				{
					pButtonOk->DisableButton();
				}
				else
				{
					pButtonOk->EnableButton();
				}
			}
			else
			{
				pButton->EnableButton();
				pButtonOk->EnableButton();
			}
		}
	}
}


void UIMenuManager::ExitToWindows()
{
	// this callback is from the confirmation dialog - force a quit
	gAppModule.RequestQuit( true );
}


void UIMenuManager::ExitGame()
{
	gTattooGame.RSUserPause( false );

	if ( ::IsSinglePlayer() )
	{
		gWorldStateRequest( WS_GAME_ENDED );
	}
	else // multiplayer
	{
		if ( gServer.IsLocal() )
		{
			gWorldState.SSetWorldStateOnMachine( RPC_TO_ALL, WS_ANY, WS_GAME_ENDED );
		}
		else
		{
			m_bClientRequestedExit = true;
			gWorldStateRequest( WS_GAME_ENDED );
			gWorldState.Update();	//	to make sure network processing shuts down right now, not next frame
		}
	}
	// make sure to clear the variable that thinks we are still
	// in the options menu
	m_bOptionsMenuActive = false;
}


void UIMenuManager::ResumeGame()
{
	if ( !m_bGameWasPaused )
	{
		gTattooGame.RSUserPause( false );
		gUI.GetGameGUI().ShowGroup( "paused", false );
	}
	gUI.GetGameGUI().HideInterface( "in_game_menu" );	
	m_bOptionsMenuActive = false;
	SetExitAfterSave( false );
}


void UIMenuManager::ReturnToMenu()
{
	gUIGame.ShowDialog( DIALOG_NONE );
	gUIMenuManager.SetExitDialogActive( false );
	if ( gUI.GetGameGUI().IsInterfaceVisible( "in_game_menu" ) == false )
	{
		if ( ::IsSinglePlayer() )
		{
			if ( !m_bGameWasPaused )
			{
				gTattooGame.RSUserPause( false );
				GuiPause( false );
			}
		}
		m_bOptionsMenuActive = false;
	}
}


void UIMenuManager::AlignCompass()
{	
	UIWindow * pDrag = gUIShell.FindUIWindow( "compass_drag_window" );
	if ( pDrag )
	{
		int radius = gSiegeEngine.GetCompass().GetCompassRadius();
		gSiegeEngine.GetCompass().SetCompassPosition( pDrag->GetRect().left+radius, pDrag->GetRect().top+radius );					
	}	
}


void UIMenuManager::CheckLoadSaveEnable()
{
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
	if( pListbox )
	{
		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "loadsave_game_name_edit_box" );
		if( pEditBox )
		{
			UIButton * pButtonOk = (UIButton *)gUIShell.FindUIWindow( "button_save_okay", "loadsave_game" );
			
			if ( !m_bIsLoading )
			{
				if ( !pEditBox->GetText().empty() )
				{										
					pButtonOk->EnableButton();
				}
				else
				{
					pButtonOk->DisableButton();
				}
			}
			else
			{				
				SaveInfoColl::iterator i;
				int listIndex = 0;
				bool bFound = false;
				for( i = m_saveInfoColl.begin(); i != m_saveInfoColl.end(); ++i )
				{					
					if ( (*i)->m_ScreenName.same_no_case( pEditBox->GetText() ) )
					{						
						pButtonOk->EnableButton();
						pListbox->SelectElement( listIndex );
						bFound = true;
						break;
					}

					listIndex++;
				}

				if ( pEditBox->GetText().empty() || !bFound )
				{					
					pButtonOk->DisableButton();
				}
			}
		}
	}
}

void UIMenuManager::SelectSaveGame()
{
	gpwstring sFilename;
	
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
	if( pListbox )
	{
		sFilename = pListbox->GetSelectedText();
	}

	UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "loadsave_game_name_edit_box" );
	if( pEditBox )
	{
		pEditBox->SetText( sFilename );
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "preview_window", "loadsave_game" );	
	if ( pWindow )
	{
		gUIShell.GetRenderer().DestroyTexture( pWindow->GetTextureIndex() );
		pWindow->SetTextureIndex( 0 );
		pWindow->SetHasTexture( false );
	}

	CheckLoadSaveEnable();

	UITextBox * pTextBox = (UITextBox *)gUIShell.FindUIWindow( "loadsave_game_name_text" );
	if( pTextBox )
	{
		if ( m_saveInfoColl.size() != 0 )
		{
			int tag = pListbox->GetSelectedTag();
			if ( tag == -1 )
			{
				tag = pListbox->GetTag();
			}
			if ( tag != -1 )
			{			
				pTextBox->SetText( L"" );
				gpwstring sTemp;
				sTemp.assignf( gpwtranslate( $MSG$ "Hero: %s\nMap: %s\nElapsed Time: %s\n" ), m_saveInfoColl[ tag ]->m_HeroName.c_str(),
																			m_saveInfoColl[ tag ]->m_MapScreenName.c_str(),
																			m_saveInfoColl[ tag ]->m_ElapsedTimeText.c_str() );
				pTextBox->SetText( sTemp );			
			}

			if ( pWindow )
			{
				unsigned int tex = m_saveInfoColl[tag]->CreateThumbnailTexture();
				pWindow->SetTextureIndex( tex );
				pWindow->SetVisible( true );
				pWindow->SetHasTexture( true );
			}

			UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_save_delete", "loadsave_game" );
			if ( (tag != -1) && (m_saveInfoColl[ tag ]->m_IsQuickSave || m_saveInfoColl[ tag ]->m_IsAutoSave) )
			{
				pButton->DisableButton();
				
			}
			else
			{
				pButton->EnableButton();				
			}
		}
	}
}


void UIMenuManager::DeleteSavedGameDialog()
{
	gUIGame.ShowDialog( DIALOG_CONFIRM_SAVE_DELETE );
}


void UIMenuManager::DeleteSavedGame()
{
	UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
	if ( pListbox )
	{
		int tag = pListbox->GetSelectedTag();
		if ( tag != -1 )
		{
			gpwstring fileName = m_saveInfoColl[ tag ]->m_FileName;

			if ( !gTattooGame.DeleteSaveGame( fileName ) )
			{
				gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "The selected save game file could not be deleted." ) );
			}
			else
			{
				UIWindow * pWindow = gUIShell.FindUIWindow( "preview_window", "loadsave_game" );	
				if ( pWindow )
				{
					gUIShell.GetRenderer().DestroyTexture( pWindow->GetTextureIndex() );
					pWindow->SetTextureIndex( 0 );
					pWindow->SetHasTexture( false );
				}

				if ( m_bIsLoading )
				{
					LoadGameDialog();
				}
				else
				{
					SaveGameDialog();
				}
			}
		}
	}
}


bool UIMenuManager::ProcessLoadSave( bool bOverwriteSaveGame )
{
	if( m_bIsLoading )
	{
		// Load selected game
		UIButton *pButton = (UIButton *)gUIShell.FindUIWindow( "button_save_okay" );
		if ( pButton && !pButton->GetDisabled() )
		{	
			UIListbox * pListbox = (UIListbox *)gUIShell.FindUIWindow( "loadsave_game_listbox" );
			if( pListbox )
			{
				int tag = pListbox->GetSelectedTag();
				if( (tag != -1) && !gTattooGame.LoadGame( m_saveInfoColl[ tag ]->m_FileName ) )
				{
					gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Load game failed." ) );
					// $$$ need a real error dialog here... -sb
					gpassertm( 0, "Load game failed." );
					return false;
				}
			}

			gSoundManager.PlaySample( "s_e_interface_close" );
		}
		else
		{
			gSoundManager.PlaySample( "s_e_gui_order_cant_move_SED" );			
			return false;
		}
	}
	else
	{
		// Save specified game

		gpwstring sFilename;
		gpwstring sTemp;

		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "loadsave_game_name_edit_box" );
		if( pEditBox )
		{
			sTemp = pEditBox->GetText();
			if ( !gUIFrontend.IsNameValid( sTemp ) )
			{
				gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "Please enter a valid save game name." ) );
				return false;
			}
		}

		for( unsigned int i = 0; i < sTemp.size(); ++i )
		{
			if( wcsrchr( WBAD_FILENAME_CHARS_FULL, sTemp[i] ) == 0 )
			{
				sFilename.append( 1, sTemp[i] );
			}
		}

		gpwstring errorMsg;
		if ( !TattooGame::IsValidSaveScreenName( sFilename, &errorMsg ) )
		{
			gUIGame.ShowDialog( DIALOG_OK, errorMsg );
			return false;
		}

		// check for save game existence
		if ( !bOverwriteSaveGame && gTattooGame.HasCampaignSaveGame( sFilename ) )
		{
			gUIGame.ShowDialog( DIALOG_OVERWRITE_SAVED_GAME );
			return false;
		}

		gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:quick_save" ) );
		UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_quick", "quick_save" );
		if ( pText )
		{
			pText->SetText( gpwtranslate( $MSG$ "Saving..." ) );
		}

		UIStatusBar *pStatus = (UIStatusBar *) gUIShell.FindUIWindow( "saveload_bar", "quick_save" );
		if ( pStatus )
		{
			pStatus->SetStatus( 0 );
		}

		m_sSaveFile = sFilename;
		m_bSaving = true;
		m_bAutoSaving = false;
		m_bQuickSaving = false;
		m_bPastUpdate = false;
		m_bLastUpdate = false;
		
	}
	return true;
}


bool UIMenuManager::KeyCloseAllDialogs()
{
	if ( IsInGame( gWorldState.GetCurrentState() ) )
	{
		if ( gUIShell.GetItemActive() )
		{
			return true;
		}

		if ( gUIMenuManager.GetMultiRespawnOption() )
		{
			gUIMenuManager.ShowMultiRespawnMenu();
		}
		else if ( gUIPartyManager.CloseAllCharacterMenus() )
		{
			gUIPartyManager.SetInventoriesVisible( false );	
			gUIMenuManager.CloseAllDialogs();
			return true;
		}
		else if ( gUIMenuManager.CloseAllDialogs() )
		{
			return true;
		}
		else if ( gWorldState.GetCurrentState() == WS_MEGA_MAP )
		{
			gWorldStateRequest( gWorldState.GetPreviousState() );
		}
		else if ( gUIGame.GetBackendDialog() == DIALOG_EXIT_GAME )
		{
			ReturnToMenu();
		}
		else if ( gUI.GetGameGUI().IsInterfaceVisible( "loadsave_game" ) )
		{
			CloseLoadSave();
			ResumeGame();
		}
		else if ( gUIOptions.IsActive() )
		{
			gUIOptions.Deactivate();
			ResumeGame();
		}			
	}

	return true;	
}


bool UIMenuManager::CloseAllDialogs()
{
	bool bClosed = false;

	if ( gUIShell.IsInterfaceVisible( "backend_dialog" ) )
	{	
		gUIShell.HideInterface( "backend_dialog" );
		bClosed = true;
	}
	if ( gUIShell.IsInterfaceVisible( "dialogue_box" ) )
	{
		gUIDialogueHandler.ExitDialogue();
		bClosed = true;
	}		
	
	return bClosed;
}


void UIMenuManager::CloseLoadSave()
{
	m_bLoadSaveActive = false;
	gUI.GetGameGUI().HideInterface( "loadsave_game" );

	// clean out cache
	m_saveInfoColl.clear();
}


void UIMenuManager::ShowCompass( bool bShow )
{
	UIWindow * pHide =	gUIShell.FindUIWindow( "button_compass_hide",	"compass_hotpoints" );
	UIWindow * pShow =	gUIShell.FindUIWindow( "button_compass_show",	"compass_hotpoints" );
	UIWindow * pDrag =	gUIShell.FindUIWindow( "compass_drag_window",	"compass_hotpoints" );
	UIWindow * pSmall = gUIShell.FindUIWindow( "compass_small",			"compass_hotpoints" );

	if ( pShow && pHide )
	{
		if ( bShow )
		{
			pHide->SetVisible( true );
			pShow->SetVisible( false );
			pDrag->SetVisible( true );
			gSiegeEngine.GetCompass().SetMinimized( false );			
			AlignCompass();
		}
		else
		{
			pHide->SetVisible( false );
			pShow->SetVisible( true );
			pDrag->SetVisible( false );			
			gSiegeEngine.GetCompass().SetMinimized( true );

			int newPosX	= pSmall->GetRect().left + (pSmall->GetRect().right - pSmall->GetRect().left)/2;
			int newPosY	= pSmall->GetRect().top + (pSmall->GetRect().bottom - pSmall->GetRect().top)/2;
			gSiegeEngine.GetCompass().SetCompassPosition( newPosX, newPosY );
		}		
	}
}


void UIMenuManager::SetGoldTradeDialogActive( bool bSet )
{
	m_bGoldTradeDialogActive = bSet;
	if ( !bSet )
	{
		gUIShell.HideInterface( "gold_transfer" );
	}
	else
	{
		gUIShell.ShowInterface( "gold_transfer" );
	}
}


void UIMenuManager::SetGoldInvalidDialogActive( bool bSet )
{
	m_bGoldInvalidDialogActive = bSet;

	if ( !bSet )
	{
		gUIGame.ShowDialog( DIALOG_NONE );
	}
	else
	{
		gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "You do not have any gold to pick up." ) );
	}
}


void UIMenuManager::SetMultiRespawnOption( bool bSet )
{
	m_bMultiRespawn = bSet;
	if ( !bSet )
	{
		HideMultiRespawnMenu();
		UIWindow * pWindow = gUIShell.FindUIWindow( "defeated_escape", "data_bar" );
		if ( pWindow )
		{
			pWindow->SetVisible( false );
		}
	}
	else
	{
		UIWindow * pWindow = gUIShell.FindUIWindow( "defeated_escape", "data_bar" );
		if ( pWindow )
		{
			pWindow->SetVisible( true );
		}
	}
}


void UIMenuManager::ShowMultiRespawnMenu()
{
	gUIShell.ShowInterface( "multiplayer_respawn" );
	UIButton * pButton = (UIButton *)gUIShell.FindUIWindow( "button_respawn", "multiplayer_respawn" );
	if ( pButton )
	{
		pButton->SetEnabled( gServer.GetAllowRespawn() );		
	}
}


void UIMenuManager::HideMultiRespawnMenu()
{
	gUIShell.HideInterface( "multiplayer_respawn" );
}


bool UIMenuManager::QuickSaveDialog()
{
	if ( ::IsMultiPlayer() || GetIsDefeated() || gUIShell.GetItemActive() )
	{
		return true;
	}

	gUIPartyManager.CloseAllCharacterMenus();
	if ( !m_bOptionsMenuActive )
	{
		m_bGameWasPaused = gAppModule.IsUserPaused();
		gTattooGame.RSUserPause( true );
	}	

	QuickSave();
	return true;
}


bool UIMenuManager::QuickLoadDialog()
{
	if ( ::IsMultiPlayer() || gUIShell.GetItemActive() )
	{
		return true;
	}

	gUIPartyManager.CloseAllCharacterMenus();
	if ( !m_bOptionsMenuActive )
	{
		m_bGameWasPaused = gAppModule.IsUserPaused();
		gTattooGame.RSUserPause( true );
	}

	if ( gTattooGame.HasQuickSaveGame() )
	{
		gUIGame.ShowDialog( DIALOG_QUICK_LOAD );
	}
	else
	{
		gUIGame.ShowDialog( DIALOG_NO_QUICK_SAVE, gpwtranslate( $MSG$ "No quick save game file is available." ) );
	}

	return true;
}


bool UIMenuManager::QuickSave()
{	
	gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:quick_save" ) );
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_quick", "quick_save" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Quick-Saving..." ) );
	}

	UIStatusBar *pStatus = (UIStatusBar *) gUIShell.FindUIWindow( "saveload_bar", "quick_save" );
	if ( pStatus )
	{
		pStatus->SetStatus( 0 );
	}
	
	m_bQuickSaving = true;
	m_bPastUpdate = false;
	m_bLastUpdate = false;
	m_bAutoSaving = false;
	m_bSaving	  = false;

	if ( !m_bOptionsMenuActive && !GetGameWasPaused() )
	{
		gTattooGame.RSUserPause( false );
	}
	return true;
}

void UIMenuManager::AutoSave()
{	
	gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:quick_save" ) );
	UIText * pText = (UIText *)gUIShell.FindUIWindow( "text_quick", "quick_save" );
	if ( pText )
	{
		pText->SetText( gpwtranslate( $MSG$ "Auto-Saving..." ) );
	}

	UIStatusBar *pStatus = (UIStatusBar *) gUIShell.FindUIWindow( "saveload_bar", "quick_save" );
	if ( pStatus )
	{
		pStatus->SetStatus( 0 );
	}
	
	m_bQuickSaving = false;	
	m_bSaving	  = false;
	m_bPastUpdate = false;
	m_bLastUpdate = false;
	m_bAutoSaving = true;	

	if ( !m_bOptionsMenuActive && !GetGameWasPaused() )
	{
		gTattooGame.RSUserPause( false );
	}
}


bool UIMenuManager::QuickLoad()
{
	gTattooGame.QuickLoadGame();
	return true;
}


void UIMenuManager::UpdateSave()
{
	if ( m_bLastUpdate )
	{			
		if ( m_bSaving )
		{
			gTattooGame.SaveGame( m_sSaveFile );
		}
		else if ( m_bQuickSaving )
		{
			gTattooGame.QuickSaveGame();						
		}
		else if ( m_bAutoSaving )
		{
			gTattooGame.AutoSaveGameNow();
		}		

		m_bSaving = false;
		m_bQuickSaving = false;
		m_bAutoSaving = false;
		m_bPastUpdate = false;
		m_bLastUpdate = false;
		gUIShell.DeactivateInterface( "quick_save" );

		if ( m_bExitAfterSave )
		{
			ExitGame();		
			m_bExitAfterSave = false;
		}
	}	
}


void UIMenuManager::DrawSave()
{
	if ( m_bPastUpdate && (m_bSaving || m_bQuickSaving || m_bAutoSaving) )
	{
		m_bLastUpdate = true;
	}
	else if ( m_bSaving || m_bQuickSaving || m_bAutoSaving )
	{
		m_bPastUpdate = true;		
	}
}


void UIMenuManager::ApplyConsoleOptions()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->SetLineTimeout( m_scrollText );
		pChat->SetLineSize( m_maxText );
		gUIGameConsole.ResizeChatWindow();
	}
}


void UIMenuManager::DefeatCallback()
{
	gUI.GetGameGUI().GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
	m_bDefeated = true;
	CloseAllDialogs();
}


void UIMenuManager::MpVictoryCallback()
{
	// Looks like this is the traditional map, we will want to show the game ended dialog.	
	gUIShell.ActivateInterface( FastFuelHandle( "ui:interfaces:backend:end_game" ) );
	UITextBox * pMpBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_eg_mp", "end_game" );
	if ( pMpBox )
	{
		pMpBox->SetVisible( true );
	}

	UITextBox * pSpBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_eg", "end_game" );
	if ( pSpBox )
	{
		pSpBox->SetVisible( false );
	}			
}


void UIMenuManager::StatusBarDock( bool bTop, UIDockbar * pDockbar )
{	
	m_bStatusDockTop = bTop;

	int height = pDockbar->GetRect().bottom-pDockbar->GetRect().top;
	height -= gUIPartyManager.GetDataBarVariation();
		
	if ( bTop )
	{
		gUIShell.ShiftInterface( "character_awp",		0, height + 20 );
		gUIShell.ShiftInterface( "team_portraits",		0, height + 20 );		
		gUIShell.ShiftInterface( "compass_hotpoints",	0, height + 20 );
		gUIShell.ShiftInterface( "multiplayer_ranks",	0, height + 20 );
		gUIShell.ShiftInterface( "multiplayer_timer",	0, height + 20 );
		gUIShell.ShiftInterface( "store",				0, height + 10 );		
		gUIShell.ShiftInterface( "character",			0, height - 3 );
		gUIShell.ShiftInterface( "spell",				0, height - 3 );
		gUIShell.ShiftInterface( "inventory",			0, height - 3 );
		gUIShell.ShiftInterface( "multi_inventory",		0, height - 3 );
		gUIShell.ShiftInterface( "multiplayer_trade",	0, height - 3 );
		gUIShell.ShiftInterface( "dialogue_box",		0, height - 3 );
		gUIShell.ShiftInterface( "console_output",		0, height + 10 );
		gUIShell.ShiftInterface( "field_commands",		0, height + 10 );			
		gUIGameConsole.ResizeChatWindow();
		SetTradeShift( height - 3 );
	}
	else
	{
		gUIShell.ShiftInterface( "character_awp",		0, -height - 20 );
		gUIShell.ShiftInterface( "team_portraits",		0, -height - 20 );		
		gUIShell.ShiftInterface( "compass_hotpoints",	0, -height - 20 );
		gUIShell.ShiftInterface( "multiplayer_ranks",	0, -height - 20 );	
		gUIShell.ShiftInterface( "multiplayer_timer",	0, -height - 20 );	
		gUIShell.ShiftInterface( "store",				0, -height - 10 );
		gUIShell.ShiftInterface( "character",			0, -height + 3 );
		gUIShell.ShiftInterface( "spell",				0, -height + 3 );
		gUIShell.ShiftInterface( "inventory",			0, -height + 3 );
		gUIShell.ShiftInterface( "multi_inventory",		0, -height + 3 );
		gUIShell.ShiftInterface( "multiplayer_trade",	0, -height + 3 );		
		gUIShell.ShiftInterface( "dialogue_box",		0, -height + 3 );		
		gUIShell.ShiftInterface( "console_output",		0, -height - 10 );
		gUIShell.ShiftInterface( "field_commands",		0, -height - 10 );		
		gUIGameConsole.ResizeChatWindow();
		SetTradeShift( 0 );
	}

	gUIMenuManager.AlignCompass();
	pDockbar->SetUVRect( pDockbar->GetUVRect().left, pDockbar->GetUVRect().right, pDockbar->GetUVRect().bottom, pDockbar->GetUVRect().top );
}


void UIMenuManager::HandleMenuResize()
{
	gUIMenuManager.AlignCompass();

	UIDockbar * pBar = (UIDockbar *)gUIShell.FindUIWindow( "data_bar", "data_bar" );
	if ( pBar )
	{
		if ( pBar->GetRect().top <= (gAppModule.GetGameHeight() / 2) )
		{
			// Readjust any "anchored" windows to the new resolution according to the status bar dock position.
			gUIShell.ShiftInterface( "field_commands", 0, (pBar->GetRect().bottom-pBar->GetRect().top) + 15 );
		}
	}
}


void UIMenuManager::SActivateTip( const gpstring & sTip, Goid target )
{
	CHECK_SERVER_ONLY;

	GoHandle hTarget( target );
	if ( hTarget.IsValid() )
	{
		RCActivateTip( sTip, hTarget->GetPlayer()->GetMachineId() );
	}
}


void UIMenuManager::RCActivateTip( const gpstring & sTip, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCActivateTip, machineId );

	ActivateTip( sTip, true, false );
}


void UIMenuManager::ActivateTip( const gpstring & sTip, bool bPause, bool bDatabase, bool bForce )
{
	if ( !m_bEnableTips && !bDatabase && !bForce )
	{
		return;
	}

	CloseAllDialogs();
	gUIPartyManager.CloseAllCharacterMenus();

	gpstring sTipAddress;
	sTipAddress.assignf( "%s:info:world_tips:%s", gWorldMap.MakeMapDirAddress().c_str(), sTip.c_str() );
	FastFuelHandle hTips( sTipAddress );
	if ( hTips )
	{		
		gUIShell.ShowInterface( "world_tip" );

		gUIShell.ShowGroup( "tip_database", bDatabase, false, "world_tip" );
		gUIShell.ShowGroup( "tip_popup", !bDatabase, false, "world_tip" );		

		if ( !gAppModule.IsUserPaused() && bPause && !::IsMultiPlayer() )
		{
			gTattooGame.RSUserPause( true );	
		}

		UIText * pNumber = (UIText *)gUIShell.FindUIWindow( "text_tip_number", "world_tip" );

		if ( !hTips.Get( "order", m_currentTip ) )
		{
			pNumber->SetVisible( false );
		}
		else
		{
			pNumber->SetVisible( true );
		}

		// Display the tip order
		{
			gpstring sAllTipsAddress;
			sAllTipsAddress.assignf( "%s:info:world_tips", gWorldMap.MakeMapDirAddress().c_str() );
			FastFuelHandle hAllTips( sAllTipsAddress );
			if ( hAllTips )
			{
				FastFuelHandleColl hlTips;
				hAllTips.ListChildrenTyped( hlTips, "tip", 1 );
				
				if ( pNumber )
				{
					gpwstring sNumber;
					sNumber.assignf( gpwtranslate( $MSG$ "Tip %d of %d" ), m_currentTip, hlTips.size() );
					pNumber->SetText( sNumber );
				}
			}
		}

		// Display the proper tip enabled/disabled state
		UICheckbox * pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_tip_toggle", "world_tip" );
		if ( pCheckbox )
		{
			 pCheckbox->SetState( !m_bEnableTips );
		}

		gpstring sStyle;
		hTips.Get( "style", sStyle );

		bool bShowText		= false;
		bool bShowBullet1	= false;
		bool bShowBullet2	= false;
		bool bShowBullet3	= false;
		bool bShowBullet4	= false;
		int  numText		= 0;
		if ( sStyle.same_no_case( "text_only" ) )
		{			
			bShowText = true;	
			numText = 1;		
		}
		else if ( sStyle.same_no_case( "bullets_1" ) )
		{
			bShowBullet1 = true;
			numText = 1;
		}
		else if ( sStyle.same_no_case( "bullets_2" ) )
		{
			bShowBullet1 = true;
			bShowBullet2 = true;
			numText = 2;
		}
		else if ( sStyle.same_no_case( "bullets_3" ) )
		{
			bShowBullet1 = true;
			bShowBullet2 = true;
			bShowBullet3 = true;
			numText = 3;
		}
		else if ( sStyle.same_no_case( "bullets_4" ) )
		{			
			bShowBullet1 = true;
			bShowBullet2 = true;
			bShowBullet3 = true;
			bShowBullet4 = true;
			numText = 4;
		}

		for ( int iText = 0; iText != numText; ++iText )
		{
			gpstring sBlock;
			sBlock.assignf( "text_%d", iText+1 );
			FastFuelHandle hText = hTips.GetChildNamed( sBlock );
			gpstring sScreenTip;
			hText.Get( "screen_name", sScreenTip );			

			UITextBox * pBox = 0;			
			if ( sStyle.same_no_case( "text_only" ) )
			{
				pBox = (UITextBox *)gUIShell.FindUIWindow( "text_box_tip", "world_tip" );
			}
			else
			{
				gpstring sBox;
				sBox.assignf( "text_box_bullet_%d", iText+1 );
				pBox = (UITextBox *)gUIShell.FindUIWindow( sBox, "world_tip" );

				// Load image
				gpstring sTexture;
				hText.Get( "texture", sTexture );
				gpstring sImage;
				sImage.assignf( "window_image_%d", iText+1 );
				UIWindow * pWindow = gUIShell.FindUIWindow( sImage, "world_tip" );
				if ( sTexture.empty() )
				{
					pWindow->LoadTexture( 0 );
				}
				else
				{
					pWindow->LoadTexture( sTexture );	
				}
			}

			if ( pBox )
			{
				pBox->SetText( ReportSys::TranslateW( sScreenTip ) );
			}
		}

		gUIShell.ShowGroup( "tip_text_only", bShowText, false, "world_tip" );			
		gUIShell.ShowGroup( "tip_bullet_1", bShowBullet1, false, "world_tip" );
		gUIShell.ShowGroup( "tip_bullet_2", bShowBullet2, false, "world_tip" );
		gUIShell.ShowGroup( "tip_bullet_3", bShowBullet3, false, "world_tip" );
		gUIShell.ShowGroup( "tip_bullet_4", bShowBullet4, false, "world_tip" );	
		
		StopTipAnimations();		

		FastFuelHandle hActions = hTips.GetChildNamed( "actions" );
		if ( hActions )
		{		
			FastFuelHandleColl hlFlashes;
			FastFuelHandleColl::iterator iFlash;
			hActions.ListChildrenNamed( hlFlashes, "flash*", 1 );
			for ( iFlash = hlFlashes.begin(); iFlash != hlFlashes.end(); ++iFlash )
			{
				gpstring sWindow;
				int numLoops	= 0;
				float duration	= 0.0f;
				(*iFlash).Get( "window", sWindow );
				(*iFlash).Get( "num_loops", numLoops );
				(*iFlash).Get( "duration", duration );

				UIWindow * pWindow = gUIShell.FindUIWindow( sWindow );
				if ( pWindow )
				{
					gUIAnimation.AddFlashAnimation( (double)duration, pWindow, true, 1.0f, numLoops );
					m_AnimatingTipWindows.push_back( pWindow );
				}
			}
		}
	}
}


bool UIMenuManager::ActivateTip( int currentOrder, bool bUseMax, bool bPause )
{
	gpstring sTipAddress;
	sTipAddress.assignf( "%s:info:world_tips", gWorldMap.MakeMapDirAddress().c_str() );
	FastFuelHandle hTips( sTipAddress );
	if ( hTips )
	{
		FastFuelHandleColl hlTips;
		FastFuelHandleColl::iterator i;
		hTips.ListChildrenTyped( hlTips, "tip", 1 );

		if ( bUseMax )
		{
			currentOrder = hlTips.size();
		}

		for ( i = hlTips.begin(); i != hlTips.end(); ++i )
		{
			int order = 0;
			(*i).Get( "order", order );
			if ( order == currentOrder )
			{
				ActivateTip( (*i).GetName(), bPause, true );
				return true;
			}
		}					
	}

	return false;
}

void UIMenuManager::ShowNextTip()
{
	m_currentTip++;	

	if ( !ActivateTip( m_currentTip ) )
	{
		m_currentTip = 1;
		ActivateTip( m_currentTip );
	}	
}


void UIMenuManager::ShowPreviousTip()
{
	m_currentTip--;

	if ( !ActivateTip( m_currentTip ) )
	{
		m_currentTip = 1;
		ActivateTip( m_currentTip, true );
	}	
}


void UIMenuManager::CloseTips( bool bUnpause )
{
	gUIShell.HideInterface( "world_tip" );

	if ( bUnpause )
	{
		if ( !m_bGameWasPaused && gWorldState.GetCurrentState() != WS_SP_DEFEAT )
		{
			gTattooGame.RSUserPause( false );
			GuiPause( false );
		}
	}

	StopTipAnimations();
}


void UIMenuManager::SetEnableTips( bool bSet, bool bExplicit )
{
	m_bEnableTips = bSet;
	if ( bSet == false && bExplicit )
	{
		gUIGame.ShowDialog( DIALOG_OK, gpwtranslate( $MSG$ "To reactivate the tips at any time, go to the Game tab in the Options Menu and click on the \"Tutorial Tips\" button." ) );
	}
}


void UIMenuManager::ToggleTips()
{
	UICheckbox * pCheckbox = (UICheckbox *)gUIShell.FindUIWindow( "checkbox_tip_toggle", "world_tip" );
	if ( pCheckbox )
	{
		 pCheckbox->SetState( !pCheckbox->GetState() );
	}
}


bool UIMenuManager::ViewTutorialTips()
{
	if ( !gAppModule.IsUserPaused() && !::IsMultiPlayer() )
	{
		gTattooGame.RSUserPause( true );	
	}

	if ( !gUIShell.IsInterfaceVisible( "world_tip" ) )
	{
		ActivateTip( m_currentTip, false, false );
	}
	else
	{
		CloseTips( true );
	}
	return true;
}


void UIMenuManager::DefeatHelp()
{
	gpstring sTipAddress;
	sTipAddress.assignf( "%s:info:world_tips", gWorldMap.MakeMapDirAddress().c_str() );
	FastFuelHandle hTips( sTipAddress );
	if ( hTips )
	{
		gpstring sTip;
		hTips.Get( "defeat_tip", sTip );
		ActivateTip( sTip, false, false, true );
	}
}

bool UIMenuManager::HasDefeatHelp()
{
	gpstring sTipAddress;
	sTipAddress.assignf( "%s:info:world_tips", gWorldMap.MakeMapDirAddress().c_str() );
	FastFuelHandle hTips( sTipAddress );
	if ( hTips )
	{
		gpstring sTip;
		if ( hTips.Get( "defeat_tip", sTip ) )
		{
			return true;
		}		
	}

	return false;
}


void UIMenuManager::StopTipAnimations()
{
	UIWindowVec::iterator i;
	for ( i = m_AnimatingTipWindows.begin(); i != m_AnimatingTipWindows.end(); ++i )
	{
		gUIAnimation.RemoveAnimationsForWindow( *i );
	}

	m_AnimatingTipWindows.clear();
}


void UIMenuManager::SFadeInterface( const char * szInterface, float duration, Goid target )
{
	CHECK_SERVER_ONLY;

	GoHandle hTarget( target );
	if ( hTarget.IsValid() )
	{
		RCFadeInterface( szInterface, duration, hTarget->GetPlayer()->GetMachineId() );
	}
}


void UIMenuManager::RCFadeInterface( const char * szInterface, float duration, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCFadeInterface, machineId );

	FadeInterface( szInterface, duration );
}


void UIMenuManager::FadeInterface( const char * szInterface, float duration )
{
	if ( !m_sFadedInterface.empty() )
	{
		return;
	}

	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		CloseAllDialogs();
		gUIPartyManager.CloseAllCharacterMenus();
	}

	gpstring sInterface;
	sInterface.assignf( "ui:interfaces:chapters:%s", szInterface );
	gUIShell.ActivateInterface( sInterface, true );
	gUIShell.UnMarkInterfaceForDeactivation( sInterface );

	UIWindow * pFadeWindow = gUIShell.FindUIWindow( "fade_window", szInterface );
	if ( pFadeWindow && duration != 0.0f )
	{
		m_TempFadeDuration = duration/2.0f;
		gUIAnimation.AddAlphaAnimation( m_TempFadeDuration, 0.0f, 1.0f, pFadeWindow, false );
		m_sFadedInterface = szInterface;
	}
}


void UIMenuManager::FadeOutInterface( UIWindow * pFadeWindow )
{
	if ( pFadeWindow )
	{
		if ( pFadeWindow->GetAlpha() == 1.0f )
		{
			gUIAnimation.AddAlphaAnimation( m_TempFadeDuration, 1.0f, 0.0f, pFadeWindow, false );
		}
		else
		{
			RemoveFadedInterface( pFadeWindow );
		}
	}
}


void UIMenuManager::RemoveFadedInterface( UIWindow * pFadeWindow )
{
	gUIShell.MarkInterfaceForDeactivation( pFadeWindow->GetInterfaceParent() );
	m_sFadedInterface.clear();
}


bool UIMenuManager::CloseActiveFadeInterface()
{
	if ( m_sFadedInterface.empty() )
	{
		return false;
	}

	gUIShell.MarkInterfaceForDeactivation( m_sFadedInterface );
	m_sFadedInterface.clear();

	return true;
}


void UIMenuManager::SPlayChapterSound( const char * szSample, Goid target )
{
	CHECK_SERVER_ONLY;

	GoHandle hTarget( target );
	if ( hTarget.IsValid() )
	{
		RCPlayChapterSound( szSample, hTarget->GetPlayer()->GetMachineId() );
	}
}


void UIMenuManager::RCPlayChapterSound( const char * szSample, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCPlayChapterSound, machineId );

	PlayChapterSound( szSample );
}


void UIMenuManager::PlayChapterSound( const char * szSample )
{
	gSoundManager.PlaySample( szSample, false, GPGSound::SND_EFFECT_NORM );
}


void UIMenuManager::ConsumeCharacterInput( UIWindow * pWindow )
{
	gpstring sCharacter;
	sCharacter.assignf( "awp_itemslot_portrait_%d", pWindow->GetIndex() + 1 );
	UIItemSlot * pSlot = (UIItemSlot *)gUIShell.FindUIWindow( sCharacter, "character_awp" );
	if ( pSlot ) 
	{
		pSlot->ResetInput();
	}	
}


void UIMenuManager::SetModalActive( bool bSet )
{ 
	m_bModalActive = bSet;  
	gWorldOptions.SetDisableHighlight( bSet );

	if ( bSet )
	{
		gUIGame.CameraInputRecalibrate( true );
		gUICamera.SetFreeLook( false );
	}
} 


void UIMenuManager::GuiPause( bool bPaused )
{
	gUI.GetGameGUI().ShowGroup( "paused", bPaused );
	
	UIWindow * pWindow = gUIShell.FindUIWindow( "button_pause", "data_bar" );
	if ( pWindow )
	{
		pWindow->SetVisible( !bPaused );
	}
	pWindow = gUIShell.FindUIWindow( "button_play", "data_bar" );
	if ( pWindow )
	{
		pWindow->SetVisible( bPaused );
		if ( bPaused )
		{
			pWindow->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
		}
	}	
}


void UIMenuManager::ActivateMegaMap()
{
	m_bIgnoreAzimuth = gUICamera.GetIgnoreAzimuth();
	gUIShell.ActivateInterface( "ui:interfaces:backend:mini_map", true );
	gUICamera.SetIgnoreAzimuth( true );
	gUICamera.SetIgnoreZoom( true );

	if ( gSiegeEngine.NodeWalker().GetMiniMapMeters() <= gUIGame.GetMapShowBorderMeters() )
	{
		gUIShell.ShowGroup( "map_border", false, false, "mini_map" );
	}	
	else
	{
		gUIShell.ShowGroup( "map_border", true, false, "mini_map" );
	}
}

void UIMenuManager::DeactivateMegaMap()
{
	// Deactivate the minimap interface
	gUIShell.DeactivateInterfaceWindows( "mini_map" );
	gUICamera.SetIgnoreAzimuth( m_bIgnoreAzimuth );
	gUICamera.SetIgnoreZoom( false );
}

void UIMenuManager::ShowServerTrafficBacklogIcon( bool bShow )
{
	if ( IsServerTimeoutIconVisible() )
	{
		return;
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "window_slownet", "data_bar" );
	if ( pWindow )
	{
		pWindow->SetVisible( bShow );
	}
}

bool UIMenuManager::IsServerTrafficBacklogIconVisible()
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_slownet", "data_bar" );
	if ( pWindow )
	{
		return pWindow->GetVisible();
	}
	return false;
}

void UIMenuManager::ShowServerTimeoutIcon( bool bShow )
{
	if ( bShow )
	{
		ShowServerTrafficBacklogIcon( false );
	}

	UIWindow * pWindow = gUIShell.FindUIWindow( "window_lag", "data_bar" );
	if ( pWindow )
	{
		pWindow->SetVisible( bShow );
	}
}

bool UIMenuManager::IsServerTimeoutIconVisible()
{
	UIWindow * pWindow = gUIShell.FindUIWindow( "window_lag", "data_bar" );
	if ( pWindow )
	{
		return pWindow->GetVisible();
	}
	return false;
}
