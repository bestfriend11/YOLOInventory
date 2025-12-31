//////////////////////////////////////////////////////////////////////////////
//
// File     :  UITeamManager.cpp
// Author(s):  Chad Queen
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "Precomp_Game.h"
#include "UITeamManager.h"
#include "Server.h"
#include "Player.h"
#include "GoAspect.h"
#include "GoActor.h"
#include "GuiHelper.h"
#include "ui_itemslot.h"
#include "ui_item.h"
#include "ui_statusbar.h"
#include "ui_shell.h"
#include "UIGame.h"
#include "UICommands.h"
#include "UIInventoryManager.h"
#include "UIPartyManager.h"
#include "nema_aspect.h"
#include "nema_blender.h"
#include "nema_kevents.h"
#include "nema_chore.h"
#include "siege_options.h"
#include "siege_loader.h"
#include "RapiImage.h"
#include "AppModule.h"
#include "GoInventory.h"
#include "WorldState.h"
#include "GoDb.h"
#include "GoMind.h"
#include "Job.h"


UITeamManager::UITeamManager()
{
	memset( (void *)m_UITeamMembers, 0, sizeof (UITeamMember *) * MAX_TEAM_MEMBERS );	
}

UITeamManager::~UITeamManager()
{	
	DeInit();
}

void UITeamManager::Init()
{
	m_TeamSize = 0;

	if ( ::IsMultiPlayer() )
	{
		gUIShell.ActivateInterface( "ui:interfaces:backend:team_portraits", true );
	}
}

void UITeamManager::DeInit()
{
	m_TeamSize = 0;
	DeconstructTeam();
	gUIShell.MarkInterfaceForDeactivation( "team_portraits" );
}

void UITeamManager::Update( float seconds )
{
	if ( ::IsMultiPlayer() && gServer.GetLocalHumanPlayer() )
	{	
		int teamSize = 0;
		Team * pHomeTeam = gServer.GetTeam( gServer.GetLocalHumanPlayer()->GetId() );		

		Server::PlayerColl players = gServer.GetPlayers();
		Server::PlayerColl::iterator iPlayer;
		for ( iPlayer = players.begin(); iPlayer != players.end(); ++iPlayer )
		{
			if ( IsValid( *iPlayer ) )
			{
				if ( (*iPlayer)->GetId() != gServer.GetLocalHumanPlayer()->GetId() )
				{
					Team * pTeam = gServer.GetTeam( (*iPlayer)->GetId() );
					if ( pTeam == pHomeTeam )
					{
						teamSize++;
					}
				}
			}
		}

		if ( teamSize != m_TeamSize )
		{			
			ConstructTeam();
			gUIShell.ShowInterface( "team_portraits" );
		}

		if ( teamSize == 0 )
		{
			gUIShell.HideInterface( "team_portraits" );
		}

		UpdateTeamInterface( seconds );
	}
}

void UITeamManager::UpdateTeamInterface( float /* seconds */ )
{
	for ( int i = 0; i != MAX_TEAM_MEMBERS; ++i )
	{
		if ( m_UITeamMembers[i] && m_UITeamMembers[i]->GetGoid() != GOID_INVALID )
		{
			GoHandle hTeamMember( m_UITeamMembers[i]->GetGoid() );
			if ( hTeamMember.IsValid() )
			{
				if ( !hTeamMember->IsInAnyScreenWorldFrustum() && gAppModule.IsAppActive() )
				{
					gUIShell.InsertDragBox( ((UIItemSlot *)m_UITeamMembers[i]->GetGuiWindow( TW_PORTRAIT ))->GetRect(), gUIPartyManager.GetPortraitInactiveColor() );
					
				}

				// Set up death portrait if necessary
				if ( !IsAlive( hTeamMember->GetAspect()->GetLifeState() ) )
				{
					m_UITeamMembers[i]->GetGuiWindow( TW_DEATH )->SetVisible( true );
					m_UITeamMembers[i]->GetGuiWindow( TW_GHOST_TIMER )->SetVisible( false );	

					// Handle the ghost timer if the player is a ghost
					if ( hTeamMember->GetAspect()->GetLifeState() == LS_GHOST )
					{							
						m_UITeamMembers[i]->GetGuiWindow( TW_GHOST_TIMER )->SetVisible( true );
						int seconds = (int)floorf( hTeamMember->GetAspect()->GetGhostTimeout() );
						int minutes = seconds / 60;
						seconds = seconds - (minutes * 60);
						gpstring sGhostTimer;
						sGhostTimer.assignf( "%d:%s%d", minutes, seconds < 10 ? "0" : "", seconds );
						((UIText *)m_UITeamMembers[i]->GetGuiWindow( TW_GHOST_TIMER ))->SetText( ToUnicode( sGhostTimer ) );
					}
				}
				else
				{
					m_UITeamMembers[i]->GetGuiWindow( TW_DEATH )->SetVisible( false );					
					m_UITeamMembers[i]->GetGuiWindow( TW_GHOST_TIMER )->SetVisible( false );	
				}				
				
				// Update health/mana bars
				((UIStatusBar *)m_UITeamMembers[i]->GetGuiWindow( TW_HEALTH ))->SetStatus( hTeamMember->GetAspect()->GetLifeRatio() );
				((UIStatusBar *)m_UITeamMembers[i]->GetGuiWindow( TW_MANA ))->SetStatus( hTeamMember->GetAspect()->GetManaRatio() );

				// Update unconscious portrait state
				UIStatusBar * pUnconscious = (UIStatusBar *)m_UITeamMembers[i]->GetGuiWindow( TW_UNCONSCIOUS );
				if ( pUnconscious )
				{
					if ( hTeamMember->GetAspect()->GetLifeState() == LS_ALIVE_UNCONSCIOUS )
					{
						if ( !pUnconscious->GetVisible() )
						{
							pUnconscious->SetVisible( true );
							pUnconscious->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
							pUnconscious->SetBaseMax( hTeamMember->GetAspect()->GetCurrentLife()-1.0f );
						}

						if ( hTeamMember->GetAspect()->GetCurrentLife() < pUnconscious->GetBaseMax() )
						{
							pUnconscious->SetBaseMax( hTeamMember->GetAspect()->GetCurrentLife()-1.0f );
						}
						
						if ( pUnconscious->GetBaseMax() != 0.0f )
						{
							pUnconscious->SetStatus( (hTeamMember->GetAspect()->GetCurrentLife()-1.0f) / pUnconscious->GetBaseMax() );
						}
					}
					else
					{
						pUnconscious->SetVisible( false );
					}
				}

				// Update the health warning state
				UIWindow * pWarning = m_UITeamMembers[i]->GetGuiWindow( TW_HEALTH_WARNING );
				if ( pWarning )
				{
					if ( hTeamMember->GetAspect()->GetLifeRatio() <= 0.33333f && !(hTeamMember->GetAspect()->GetLifeState() == LS_ALIVE_UNCONSCIOUS) )
					{
						if ( !pWarning->GetVisible() )
						{
							pWarning->SetVisible( true );
							pWarning->ReceiveMessage( UIMessage( MSG_STARTANIMATION ) );
						}
					}
					else
					{
						pWarning->SetVisible( false );
					}
				}

				m_UITeamMembers[i]->GetGuiWindow( TW_GUARDED )->SetVisible( false );
				GoidColl::const_iterator iGuarded;
				for ( iGuarded = gUIPartyManager.GetGuardedCharacters().begin(); iGuarded != gUIPartyManager.GetGuardedCharacters().end(); ++iGuarded )
				{
					if ( (*iGuarded) == hTeamMember->GetGoid() )
					{
						if ( hTeamMember->IsInAnyScreenWorldFrustum() )
						{
							m_UITeamMembers[i]->GetGuiWindow( TW_GUARDED )->SetVisible( true );
						}
						else
						{
							gUIPartyManager.RemoveCharacterAsGuarded( *iGuarded );
						}						
					}				
				
				}

				if ( !IsAlive( hTeamMember->GetAspect()->GetLifeState() ) )
				{
					pWarning->SetVisible( false );
					pUnconscious->SetVisible( false );					
					continue;
				}
			}
		}
	}
}

void UITeamManager::ConstructTeam()
{
	DeconstructTeam();
	m_TeamSize = 0;	

	if ( !gServer.GetLocalHumanPlayer() )
	{
		return;
	}

	for ( int i = 0; i != MAX_TEAM_MEMBERS; ++i )
	{
		// Create new members
		m_UITeamMembers[i] = new UITeamMember( GOID_INVALID );
		m_UITeamMembers[i]->SetTexture( 0 );

		// Reset window visibility
		gpstring sGroup;
		sGroup.assignf( "character_%d_mp", i+2 );
		gUIShell.ShowGroup( sGroup, false, false, "team_portraits" );		
	}

	if ( !GenerateTeamPortraits() )
	{
		return;
	}

	int index = 0;
	Team * pHomeTeam = gServer.GetTeam( gServer.GetLocalHumanPlayer()->GetId() );		

	Server::PlayerColl players = gServer.GetPlayers();
	Server::PlayerColl::iterator iPlayer;
	for ( iPlayer = players.begin(); iPlayer != players.end(); ++iPlayer )
	{
		if ( IsValid( *iPlayer ) && ((*iPlayer)->GetId() != gServer.GetLocalHumanPlayer()->GetId()) )
		{
			Team * pTeam = gServer.GetTeam( (*iPlayer)->GetId() );
			if ( pTeam == pHomeTeam )
			{
				Go * party = (*iPlayer)->GetParty();
				if( !party || party->GetChildren().empty() )
				{
					continue;
				}

				GoHandle hMember( *(party->GetChildren().begin()) );

				if ( hMember.IsValid() )
				{
					UIWindow * pGuarded = NULL;

					m_UITeamMembers[index]->SetGoid( hMember->GetGoid() );

					gpstring sWindow;
					{
						sWindow.assignf( "awp_status_health_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_HEALTH );
					}
					{
						sWindow.assignf( "awp_status_mana_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_MANA );
					}				
					{
						sWindow.assignf( "awp_death_portrait_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_DEATH );
					}
					{
						sWindow.assignf( "awp_portrait_health_warning_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_HEALTH_WARNING );
					}
					{
						sWindow.assignf( "awp_portrait_unconscious_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_UNCONSCIOUS );
					}
					{
						sWindow.assignf( "awp_portrait_rollover_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_PORTRAIT_ROLLOVER );
					}
					{
						sWindow.assignf( "ghost_timer_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_GHOST_TIMER );
					}
					{
						sWindow.assignf( "awp_guard_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_GUARDED );
						pGuarded = pWindow;
					}
					{
						sWindow.assignf( "awp_itemslot_portrait_%d", index+2 );
						UIWindow * pWindow = gUIShell.FindUIWindow( sWindow, "team_portraits" );
						m_UITeamMembers[index]->SetGuiWindow( pWindow, TW_PORTRAIT );

						UIItemSlot *	pSlot = (UIItemSlot *)pWindow;
						UIItem *		pItem = GetItemFromGO( hMember, true, true );
						if ( pSlot && pItem ) 
						{
							m_UITeamMembers[index]->SetGuiWindow( pItem, TW_PORTRAIT_ITEM );
							pSlot->ClearItem();
							pItem->SetItemID( MakeInt(hMember->GetGoid()) );
							pSlot->PlaceItem( pItem, false );
							pSlot->SetAcceptInput( false );				
							pSlot->SetUVRect( 0.0f, 0.609375f, 0.28125f, 1.0f );
						}
					}

					if ( pGuarded )
					{
						pGuarded->SetVisible( false );
					}
					
					gpstring sGroup;
					sGroup.assignf( "character_%d_mp", index+2 );
					gUIShell.ShowGroup( sGroup, true, false, "team_portraits" );

					m_UITeamMembers[index]->SetTexture( hMember->GetActor()->GetPortraitTexture() );

					++index;
					++m_TeamSize;
				}
			}
		}
	}	
}

void UITeamManager::DeconstructTeam()
{
	for ( int i = 0; i != MAX_TEAM_MEMBERS; ++i )
	{
		if ( m_UITeamMembers[i] && m_UITeamMembers[i]->GetGoid() != GOID_INVALID )
		{
			gpstring sWindow;
			sWindow.assignf( "awp_itemslot_portrait_%d", i+2 );
			UIItemSlot * pWindow = (UIItemSlot *)gUIShell.FindUIWindow( sWindow, "team_portraits" );
			if ( pWindow )
			{
				UIItem * pItem = (UIItem *)m_UITeamMembers[i]->GetGuiWindow( TW_PORTRAIT_ITEM );
				if ( pItem )
				{
					pWindow->ClearItem();
					pItem->RemoveItemParent( pWindow );
				}
			}
		}

		Delete( m_UITeamMembers[i] );
	}
}

void UITeamManager::CastOnCharacterPortrait( int teamIndex )
{
	if ( m_UITeamMembers[teamIndex] && 
		 m_UITeamMembers[teamIndex]->GetGoid() != GOID_INVALID )
	{
		GoHandle hMember( m_UITeamMembers[teamIndex]->GetGoid() );
		if ( hMember.IsValid() && gUIGame.GetUICommands()->GetActiveContextCursor() == CURSOR_GUI_CAST )
		{
			gUIGame.GetUICommands()->CommandCast( hMember->GetGoid() );
		}	
	}	
}

void UITeamManager::GuardOnCharacterPortrait( int teamIndex )
{
	if ( !gUIShell.GetItemActive() )
	{
		if ( m_UITeamMembers[teamIndex] && 
			 m_UITeamMembers[teamIndex]->GetGoid() != GOID_INVALID )
		{
			GoHandle hMember( m_UITeamMembers[teamIndex]->GetGoid() );
			if ( hMember.IsValid() )
			{
				gUIGame.GetUICommands()->CommandGuard( hMember->GetGoid() );		
			}
		}
	}
}

void UITeamManager::ActivateTrade( int teamIndex )
{
	if ( gUIShell.GetItemActive() )
	{
		UIItemVec item_vec;
		gUIShell.FindActiveItems( item_vec );
		UIItemVec::iterator i = item_vec.begin();
		GoHandle hObject( MakeGoid( (*i)->GetItemID() ) );
		if ( hObject.IsValid() )
		{
			Goid member = gUIGame.GetActorWhoCarriesObject( hObject->GetGoid() );
			GoHandle hMember( member );
			if ( hMember.IsValid() )
			{
				if( m_UITeamMembers[teamIndex] && m_UITeamMembers[teamIndex]->GetGoid() )
				{
					GoHandle hTeamMember( m_UITeamMembers[teamIndex]->GetGoid() );
					if ( hTeamMember.IsValid() && hTeamMember->IsInActiveWorldFrustum() )
					{						
						gUIInventoryManager.RSReqInitiateTrade( hMember, hTeamMember, hObject );						
					}
				}
			}
		}
	}

}


bool UITeamManager::GenerateTeamPortraits()
{
	if ( gWorldState.GetCurrentState() != WS_MP_INGAME || !gAppModule.IsAppActive() )
	{
		return false;
	}

	if ( gDefaultRapi.GetFade() )
	{
		return false;
	}

	Team * pHomeTeam = gServer.GetTeam( gServer.GetLocalHumanPlayer()->GetId() );		

	Server::PlayerColl players = gServer.GetPlayers();
	Server::PlayerColl::iterator iPlayer;
	for ( iPlayer = players.begin(); iPlayer != players.end(); ++iPlayer )
	{
		if ( IsValid( *iPlayer ) )
		{
			if ( (*iPlayer)->GetId() != gServer.GetLocalHumanPlayer()->GetId() )
			{
				Team * pTeam = gServer.GetTeam( (*iPlayer)->GetId() );
				if ( pTeam == pHomeTeam )
				{
					Go * party = (*iPlayer)->GetParty();
					if( !party || party->GetChildren().empty() )
					{
						continue;
					}
					GoHandle hMember( *(party->GetChildren().begin()) );
					if ( hMember.IsValid() && !hMember->GetActor()->GetPortraitTexture() )
					{
						bool bSuccess = GeneratePortrait( hMember );
						if ( !bSuccess )
						{
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}


// $$$ This code in this function is subject to change.  It has a lot of similar code to functions 
// in UICharacterSelect.  I want to break out the duplicated code later once all the bugs in this current
// implementation are squashed and I'm sure it works with JIP.
bool UITeamManager::GeneratePortrait( Go * pTeamMember )
{
	DWORD dwClearColor = gDefaultRapi.GetClearColor();
	gDefaultRapi.SetClearColor( 0x00000000 );

	// Okay, we're going to create a local go look-alike for this team member to take the shot.  
	// That way we don't have to worry about any equipment ( helmets, swords, shields ) getting in the way.
	GoCloneReq req( pTeamMember->GetTemplateName() );
	req.m_OmniGo = true;	
	Goid clone = gGoDb.CloneLocalGo( req );
	GoHandle hClone( clone );
	if ( !hClone.IsValid() )
	{
		return false;
	}	
	nema::HAspect hasp = hClone->GetAspect()->GetAspectHandle();
	if ( !hasp.IsValid() )
	{		
		return false;
	}		

	// Now that we created the go from the base templates, let's set the custom head and textures.
	if ( !pTeamMember->GetInventory()->GetCustomHead().empty() )
	{
		hClone->GetInventory()->SetCustomHead( pTeamMember->GetInventory()->GetCustomHead() );
	}	

	hClone->GetAspect()->SetDynamicTexture( PS_FLESH, pTeamMember->GetAspect()->GetDynamicTexture( PS_FLESH ) );
	hClone->GetAspect()->SetDynamicTexture( PS_CLOTH, pTeamMember->GetAspect()->GetDynamicTexture( PS_CLOTH ) );
	hClone->GetInventory()->UpdateCustomHeadTexture();

	// Start a new scene
	if( !gDefaultRapi.Begin3D( true ) )
	{
		return false;
	}
	
	bool bWasPurged1 = hasp->IsPurged( 1 );
	bool bWasPurged2 = hasp->IsPurged( 2 );
	hasp->Reconstitute( 0, true );     // Load everthing, immediately
	gSiegeLoadMgr.Commit( siege::LOADER_TEXTURE_TYPE );

	// Set up viewport
	hasp->Deform();

	vector_3 head_pos;
	Quat head_rotation;

	// Get the head bone
	hasp->GetBoneOrientation( "bip01_head", head_pos, head_rotation );

	// Setup all matrices to reflect proper head facing orientation
	matrix_3x3 mat = Transpose( head_rotation.BuildMatrix() );

	vector_3 viewer_location = vector_3( 0.5f, 3.0f, -0.5f );
	vector_3 target_pos		 = vector_3( 0.1f, 0.0f, 0.0f );
	vector_3 up_vector		 = vector_3( 1.0f, 0.0f, 0.0f );

	float orthoMatrix = 0.006f;

	gpstring sCamera;
	sCamera.assignf( "ui:config:character_camera:portrait_camera:%s", pTeamMember->GetTemplateName() );
	FastFuelHandle hPortraitCam( sCamera.c_str() );
	if ( hPortraitCam.IsValid() )
	{
		gpstring sTemp;
		hPortraitCam.Get( "viewer_location_offset", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, viewer_location.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, viewer_location.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, viewer_location.z );

		hPortraitCam.Get( "target_position_offset", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, target_pos.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, target_pos.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, target_pos.z );

		hPortraitCam.Get( "up_vector", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, up_vector.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, up_vector.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, up_vector.z );

		hPortraitCam.Get( "ortho_matrix", orthoMatrix );
	}

	gSiegeEngine.Renderer().SetWorldMatrix( mat, mat * -head_pos );
	gSiegeEngine.Renderer().SetOrthoMatrix( orthoMatrix );

	gSiegeEngine.Renderer().SetViewMatrix( viewer_location,	// Offset to the viewer location
										   target_pos,		// Target position offset
										   up_vector );		// Up vector

	hasp->InitializeLighting( 0xFF808080, 255 );

	nema::LightSource nls;
	nls.m_Type = nema::NLS_INFINITE_LIGHT;
	nls.m_Color = 0xFFFFFFFF;
	nls.m_Intensity = 1.0f;
	nls.m_Direction = Normalize(vector_3(0,0.7071f,0.7071f));

	hasp->CalculateShading(  nls, gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
	hasp->Render( &gSiegeEngine.Renderer() );

	// End the scene without copying to primary
	if ( !gDefaultRapi.End3D( false, true ) )
	{
		return false;
	}

	gDefaultRapi.SetClearColor( dwClearColor );

	// Generate portrait
	RECT rect;
	rect.left	= (int)( (float)380.0f * (float)(gAppModule.GetGameWidth()/800.0f) );
	rect.top	= (int)( (float)277.0f * (float)(gAppModule.GetGameHeight()/600.0f) );
	rect.right	= rect.left + 64;
	rect.bottom	= rect.top + 64;

	if ( gAppModule.GetGameWidth() == 1024 && gAppModule.GetGameHeight() == 768 )
	{
		rect.left	+= 8;
		rect.right	+= 8;
		rect.top	+= 6;
		rect.bottom += 6;
	}
	else if ( gAppModule.GetGameWidth() == 640 && gAppModule.GetGameHeight() == 480 )
	{
		rect.left	-= 2;
		rect.right	-= 2;
		rect.top	-= 3;
		rect.bottom -= 3;
	}

	// abort in any nonstandard resolutions.
	if ( !IsPower2( rect.right-rect.left ) || !IsPower2( rect.bottom-rect.top ) )
	{		
		return false;
	}

	// Create image
	RapiMemImage* pImage = gSiegeEngine.Renderer().CreateSubsectionImage( &rect );
	if ( pImage->GetWidth() != (rect.right-rect.left) || pImage->GetHeight() != (rect.bottom-rect.top) )
	{
		gpwarningf( ( "Requested to CreateSubsectionImage Width: %d, Height %d. Received Width %d, Height %d. Will retry next update.", 
						rect.right-rect.left, rect.bottom-rect.top,
						pImage->GetWidth(), pImage->GetHeight() ) );
		return false;
	}

	// Set all of the alpha bits to full on
	DWORD* pImageBits = pImage->GetBits();
	for( int i = 0; i < (pImage->GetWidth() * pImage->GetHeight()); ++i )
	{
		pImageBits[ i ]	|= 0xFF000000;
	}

	if ( bWasPurged2 )
	{
		hasp->Purge( 2 );
	}
	else if ( bWasPurged1 )
	{
		hasp->Purge( 1 );
	}

	// Set the player's portrait
	DWORD portrait_tex = gDefaultRapi.CreateAlgorithmicTexture( "portrait", pImage, false, 0, TEX_LOCKED, false, true );
	pTeamMember->GetActor()->SetPortraitTexture( portrait_tex, false );
	
	gGoDb.SMarkForDeletion( hClone->GetGoid(), true );

	return true;
}
