//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIGameConsole.cpp
// Author(s):  Chad Queen
//			   Anti-hackitude by James Loe
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


/*****************************************************************************

  Notes:

  This is for the retail end-user console that is activated by the enter key.
  It is intended for use in the game to chat with various members in
  multiplayer games as well as input cheat codes and some select dev commands
  ( we'll need to have proper security in place in order to allow the dev
  commands to function )

*****************************************************************************/


#include "Precomp_Game.h"
#include "AppModule.h"
#include "Config.h"
#include "GpConsole.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "UIGameConsole.h"
#include "UIMultiplayer.h"
#include "UIPartyManager.h"
#include "ui_shell.h"
#include "ui_chatbox.h"
#include "ui_combobox.h"
#include "Server.h"
#include "StringTool.h"
#include "Player.h"
#include "WorldState.h"
#include "GoDb.h"
#include "AiQuery.h"
#include "GoActor.h"
#include "siege_options.h"
#include "siege_compass.h"
#include "AiAction.h"
#include "GoMind.h"
#include "ContentDb.h"
#include "sim.h"
#include "netlog.h"
#include "RapiAppModule.h"
#include "RapiMouse.h"
#include "UIZoneMatch.h"
#include "ui_dialogbox.h"

//////////////////////////////////////////////////////////////////////////////
// Cheat Codes

#if !GP_RETAIL

// Disband selected party members
class gpc_disband : public GpConsole_command
{
public:
	gpc_disband();
	~gpc_disband(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_disband::gpc_disband()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "Disband selected party members";
	usage.push_back( "disband" );

	gGpConsole.RegisterCommand( *this, "disband", help, usage );
};


bool gpc_disband::Execute( GpConsole_command_arguments & /*args*/, gpstring & /*output*/ )
{
	gUIPartyManager.DisbandButton();
	return true;
}

#endif


//////////////////////////////////////////////////////////////////////////////


UIGameConsole :: UIGameConsole()
{
	Init();

#	if !GP_RETAIL
	stdx::make_auto_ptr( m_gpc_disband, new gpc_disband );
#	endif // !GP_RETAIL

}

void UIGameConsole :: Init()
{
	m_chatWindowSize = CHAT_SIZE_MIN;
}

void UIGameConsole :: ReceiveConsoleText( gpwstring const & sText )
{
	// $ early bailout if nothing there
	if ( sText.empty() )
	{
		return;
	}

	bool submitChat = true;

	// check for specials
	if ( (sText[ 0 ] == L'+') || (sText[ 0 ] == L'-') )
	{
		gpwstring::const_iterator i;
		for ( i = sText.begin(); i != sText.end(); i++ )
		{
			if ( *i < 0x20 || *i > 0x7f )
				break;
		}
		if ( i == sText.end() )
		{
			// cheat/internal command
			submitChat = SubmitCommand( ::ToAnsi( sText.c_str() + 1 ), sText[ 0 ] == L'+' );
		}
	}
#	if GP_DIRECT_SKRIT_ENTRY
	else if ( sText[ 0 ] == L'/' && sText[ 1 ] == L'/' )
	{
		gpwstring::const_iterator i;
		for ( i = sText.begin(); i != sText.end(); i++ )
		{
			if ( *i < 0x20 || *i > 0x7f)
				break;
		}
		if ( i == sText.end() )
		{
			// skrit command
			Skrit::ExecuteCommand( "console", ::ToAnsi( sText.c_str() + 1 ) );
		}
	}
#	endif // GP_DIRECT_SKRIT_ENTRY

	if ( submitChat )
	{
		SubmitChat( sText );
	}
}


void UIGameConsole :: SubmitChat( gpwstring const & sChat, const char * listboxParentInterface /*= "game_console_mp"*/, bool sendToAll /*=false*/ )
{
	if ( IsInGame( gWorldState.GetCurrentState() ) )
	{
		gpwstring sName;
		if ( gServer.GetLocalHumanPlayer()->GetParty() &&
			 gServer.GetLocalHumanPlayer()->GetParty()->GetChildren().size() != 0 )
		{
			sName = (*(gServer.GetLocalHumanPlayer()->GetParty()->GetChildren().begin()))->GetCommon()->GetScreenName();
		}

		if ( ::IsMultiPlayer() )
		{
			gUIMultiplayer.ProcessChatMessage( sChat, listboxParentInterface, sendToAll );
		}
		else
		{
			RCChat( sChat );
		}
	}
}


void UIGameConsole :: RSChat( gpwstring const & text )
{
	FUBI_RPC_THIS_CALL( RSChat, RPC_TO_SERVER );
	RCChat( text );
}


void UIGameConsole :: RCChat( gpwstring const & text )
{
	FUBI_RPC_THIS_CALL( RCChat, RPC_TO_ALL );

	Chat( text );
}


void UIGameConsole :: RSChatPlayer( gpwstring const & sChat, Player * player )
{
	FUBI_RPC_THIS_CALL( RSChatPlayer, RPC_TO_SERVER );

	RCChatPlayer( sChat, player->GetMachineId() );
}


void UIGameConsole :: RCChatPlayer( gpwstring const & sChat, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCChatPlayer, machineId );

	Chat( sChat );
}


void UIGameConsole :: Chat( gpwstring const & text )
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box" );
	if ( pChat )
	{
		pChat->SubmitText( text, pChat->GetFontColor() );

		GoHandle hMember( gUIPartyManager.GetSelectedCharacter() );
		if ( hMember.IsValid() )
		{
			hMember->PlayVoiceSound( "chat_talk" );
		}
	}

	if ( ::IsMultiPlayer() && IsStagingArea( gWorldState.GetCurrentState() ) )
	{
		gUIMultiplayer.Chat( text );
	}
}


bool UIGameConsole :: SubmitCommand( gpstring sCommand, bool enable )
{
	stringtool::RemoveBorderingWhiteSpace( sCommand );
	sCommand.to_lower();
	bool isCheat	= true;

	// Calculate CRC using lowercase command string
	UINT32 cheatCRC	= GetCRC32( 7333, (const void*)sCommand.c_str(), sCommand.size() );

	if ( cheatCRC == 0xafa5fac1 )
	{
		// ZOOL

		if ( ::IsSinglePlayer() )
		{
			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );

			for ( i = party.begin(); i != party.end(); ++i )
			{
				GoHandle hMember( *i );
				if ( hMember.IsValid() && hMember->HasAspect() )
				{
					hMember->GetAspect()->SetIsInvincible( enable );
					gpscreenf(( "%S is %s!\n", hMember->GetCommon()->GetScreenName().c_str(), enable ? "invincible" : "puny" ));
				}
			}
		}
	}
/*  Disable for ESRB
#if !DISABLE_GORE
	else if ( cheatCRC == 0xcb59d775 )
	{
		// CHUNKY

		// $ this changes gameplay (chunked actors cannot be resurrected) so
		//   don't permit in MP game.
		if ( ::IsSinglePlayer() )
		{
			gWorldOptions.SetAlwaysGib( enable );
			gpscreen( enable ? "Always chunky!\n" : "Sometimes chunky!\n" );
		}
	}
	else if ( cheatCRC == 0x249a9cae )
	{
		// SUPERCHUNKY

		int mult = gWorldOptions.GetGibMultiplier() + (enable ? 1 : -1);
		if ( mult >= 0 )
		{
			gWorldOptions.SetGibMultiplier( mult );
			if ( mult == 0 )
			{
				gpscreen( "Chunks disabled!\n" );
			}
			else
			{
				gpscreenf(( "Chunk factor %d\n", mult ));
			}
		}

		// not really a cheat, just visual...
		isCheat = false;
	}
#endif
*/
	else if ( sCommand.same_no_case( "version" ) )
	{
		// VERSION


		gpscreenf(( $MSG$ "Version is %s\n", SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ).c_str() ));

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( cheatCRC == 0x7fb396dd )
	{
		// SHOOTALL

		// this changes gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gWorldOptions.SetCommandCastEnabled( enable );
			gpscreenf(( "Clicks %s\n", enable ? "not required!" : "are required..." ));
		}
	}
	else if ( sCommand.same_no_case( "resizelabels" ) )
	{
		// RESIZELABELS

		gUIGame.SetResizeLabels( enable );
		gpscreenf(( "Character label resizing %s\n", enable ? "enabled!" : "disabled." ));

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( cheatCRC == 0xe49d0758 )
	{
		// DRDEATH

		// this changes gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gpscreen( "MAX DAMAGE!\n" );

			Go* party = gServer.GetScreenParty();
			if ( party != NULL )
			{
				GopColl partyColl = party->GetChildren();
				GopColl::iterator iMember;
				for ( iMember = partyColl.begin(); iMember != partyColl.end(); ++iMember )
				{
					Skill *pSkill = NULL;
					const float max_level = gRules.GetMaxLevel();

					if( (*iMember)->GetActor()->GetSkill( "uber", &pSkill ) )
					{
						pSkill->SetNaturalLevel( gRules.GetMaxLevelUber() );
					}
					if( (*iMember)->GetActor()->GetSkill( "strength", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
					}
					if( (*iMember)->GetActor()->GetSkill( "intelligence", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
					}
					if( (*iMember)->GetActor()->GetSkill( "dexterity", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
					}
					if( (*iMember)->GetActor()->GetSkill( "melee", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level );
					}
					if( (*iMember)->GetActor()->GetSkill( "ranged", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level );
					}
					if( (*iMember)->GetActor()->GetSkill( "nature magic", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level );
					}
					if( (*iMember)->GetActor()->GetSkill( "combat magic", &pSkill ) )
					{
						pSkill->SetNaturalLevel( max_level );
					}

					(*iMember)->GetAspect()->SetNaturalManaRecoveryUnit( max_level );
					(*iMember)->GetAspect()->SetNaturalManaRecoveryPeriod( 1 );
					(*iMember)->GetAspect()->SetNaturalLifeRecoveryUnit( max_level );
					(*iMember)->GetAspect()->SetNaturalLifeRecoveryPeriod( 1 );
					(*iMember)->SetModifiersDirty();
				}
			}
		}
	}
	else if ( cheatCRC == 0xcacbbc7d )
	{
		// SIXDEMONBAG

		// this changes gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gpscreen( "Just like your salad bar...\n" );

			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );
			i = party.begin();
			GoHandle hMember( *i );
			if ( hMember.IsValid() && hMember->HasAspect() )
			{
				SiegePos spos;
				gAIQuery.FindSpotRelativeToSource(	hMember,
													gWorldOptions.GetDropItemsDistanceMin(),
													gWorldOptions.GetDropItemsDistanceMax(),
													gWorldOptions.GetInventorySpewVerticalConstraint(),
													spos );
				GoCloneReq cloneReq;
				cloneReq.SetStartingPos( spos );
				gGoDb.SCloneGo( cloneReq, "spell_summon_zombie_headless" );
				gGoDb.SCloneGo( cloneReq, "spell_summon_drake_black" );
				gGoDb.SCloneGo( cloneReq, "spell_summon_fire_elemental" );
				gGoDb.SCloneGo( cloneReq, "spell_summon_larch" );
				gGoDb.SCloneGo( cloneReq, "spell_summon_furok" );
				gGoDb.SCloneGo( cloneReq, "spell_summon_rock_golem" );
			}
		}
	}
	else if ( cheatCRC == 0x15e977d1 )
	{
		// LOEFERVISION

		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gSiegeEngine.GetOptions().SetDrawDiscoveryFog( !enable );
			gpscreenf(( "Discovery fog %s\n", !enable ? "enabled!" : "disabled." ));
		}
	}
	else if ( cheatCRC == 0x87cf3c6e )
	{
		// XRAYVISION

		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gSiegeEngine.GetOptions().SetWireframeEnabled( enable );
			gpscreenf(( "X-Ray vision %s\n", enable ? "enabled!" : "disabled." ));
		}
	}
	else if ( cheatCRC == 0x1c536d1d || cheatCRC == 0xfa33c2fc )
	{
		// MINJOOKY, MAXJOOKY

		bool isMin = (cheatCRC == 0x1c536d1d);

		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gpscreenf(( "%s\n", isMin ? "MINJOOKY" : "MAXJOOKY" ));

			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );
			for ( i = party.begin(); i != party.end(); ++i )
			{
				GoHandle hMember( *i );
				if ( hMember.IsValid() && hMember->HasAspect() )
				{
					if ( isMin )
					{
						if ( hMember->GetAspect()->GetRenderScaleMultiplier() >= 1.0f )
						{
							hMember->GetAspect()->SetRenderScaleMultiplier( 0.5f );
						}
						else
						{
							hMember->GetAspect()->SetRenderScaleMultiplier( 1.0f );
						}
					}
					else
					{
						if ( hMember->GetAspect()->GetRenderScaleMultiplier() > 1.0f )
						{
							hMember->GetAspect()->SetRenderScaleMultiplier( 1.0f );
						}
						else
						{
							hMember->GetAspect()->SetRenderScaleMultiplier( 2.0f );
						}
					}
				}
			}
		}
	}
	else if ( cheatCRC == 0x489cefcf )
	{
		// CHECKSINTHEMAIL

		if ( ::IsSinglePlayer() )
		{
			gpscreen( "Have you paid your dues?\n" );

			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );
			i = party.begin();
			GoHandle hMember( *i );
			if ( hMember.IsValid() && hMember->HasAspect() )
			{
				hMember->GetInventory()->SSetGold( gContentDb.GetMaxPartyGold() );
			}
		}
	}
	else if ( cheatCRC == 0x42e9b215 )
	{
		// SNIPER

		if ( ::IsSinglePlayer() )
		{
			gpscreenf(( "Sniping %s\n", enable ? "enabled!" : "disabled." ));
			gSim.SetSniperRange( enable ? 1000.0f : 0 );
		}
	}
	else if ( sCommand.same_no_case( "dslog" ) )
	{
		// DSLOG

		gpscreenf(( "DSLOG %s\n", enable ? "enabled!" : "disabled." ));
		if ( enable )
		{
			gNetLog.StartLogging( false );
		}
		else
		{
			gNetLog.StopLogging();
		}
	}
	else if ( sCommand.same_no_case( "movie" ) )
	{
		// MOVIE

		if ( enable )
		{
			gpscreen( "30-second 30 FPS 'clip' movie starting in 10 seconds...\n" );
			gRapiAppModule.StartCaptureMovie( "clip", 30, 10, 30 );
		}
		else if ( gRapiAppModule.IsCapturingMovie() )
		{
			gpscreen( "Movie capture has been stopped.\n" );
			gRapiAppModule.StopCaptureMovie();
		}

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( sCommand.same_no_case( "gui" ) )
	{
		// GUI

		gpscreenf(( "GUI %s\n", enable ? "enabled!" : "disabled." ));

		gUI.GetGameGUI().SetVisible( enable );
		gSiegeEngine.GetCompass().SetActive( enable );

		if ( enable )
		{
			gUIShell.ShowInterface( "compass_hotpoints" );
		}
		else
		{
			gUIGame.GetUIPartyManager()->CloseAllCharacterMenus();
			gUIShell.HideInterface( "compass_hotpoints" );
		}

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( sCommand.same_no_case( "mouse" ) )
	{
		// MOUSE

		gpscreenf(( "Mouse %s\n", enable ? "enabled!" : "disabled." ));
		gUI.GetGameGUI().SetDrawCursor( enable );

		if ( enable )
		{
			gRapiMouse.Enable();
		}
		else
		{
			gRapiMouse.Disable();
		}

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( sCommand.same_no_case( "rings" ) )
	{
		// RINGS

		gpscreenf(( "Selection rings %s\n", enable ? "enabled!" : "disabled." ));
		gWorldOptions.SetSelectionRings( enable );

		// this is not a cheat, no need to broadcast it
		isCheat = false;
	}
	else if ( sCommand.same_no_case( "z" ) )
	{		
		// Z - TELEPORT

		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gUIGame.TeleportGameObjectToMouse();		
		}
	}
	else if ( cheatCRC == 0x2dccf2bd )
	{
		// POTIONAHOLIC
		
		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gpscreen( "Just give me one more potion! I can quit anytime I want. Honest." );

			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );
			i = party.begin();
			GoHandle hMember( *i );
			if ( hMember.IsValid() && hMember->HasAspect() )
			{
				SiegePos spos;
				gAIQuery.FindSpotRelativeToSource(	hMember,
													gWorldOptions.GetDropItemsDistanceMin(),
													gWorldOptions.GetDropItemsDistanceMax(),
													gWorldOptions.GetInventorySpewVerticalConstraint(),
													spos );
				GoCloneReq cloneReq;
				cloneReq.SetStartingPos( spos );
				gGoDb.SCloneGo( cloneReq, "potion_health_super" );
				gGoDb.SCloneGo( cloneReq, "potion_health_super" );
				gGoDb.SCloneGo( cloneReq, "potion_health_super" );
				gGoDb.SCloneGo( cloneReq, "potion_mana_super" );
				gGoDb.SCloneGo( cloneReq, "potion_mana_super" );
				gGoDb.SCloneGo( cloneReq, "potion_mana_super" );
			}				
		}
	}
	else if ( cheatCRC == 0x91c1481a )
	{
		// FAERTEHBADGAR

		// this affects gameplay, so no permit in MP
		if ( ::IsSinglePlayer() )
		{
			gpscreen( "TEH BADSGAR SI TEH MASTAR!@#$" );

			GoidColl party;
			GoidColl::iterator i;
			gUIPartyManager.GetSelectedPartyMembers( party );
			i = party.begin();
			GoHandle hMember( *i );
			if ( hMember.IsValid() && hMember->HasAspect() )
			{
				SiegePos spos;
				gAIQuery.FindSpotRelativeToSource(	hMember,
													gWorldOptions.GetDropItemsDistanceMin(),
													gWorldOptions.GetDropItemsDistanceMax(),
													gWorldOptions.GetInventorySpewVerticalConstraint(),
													spos );
				GoCloneReq cloneReq;
				cloneReq.SetStartingPos( spos );
				gGoDb.SCloneGo( cloneReq, "#helm/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#gloves/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#body/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#sword/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#amulet/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#ring/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#ring/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#ring/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#ring/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#boots/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#bow/1-10/+ofthebadger" );
				gGoDb.SCloneGo( cloneReq, "#shield/1-10/+ofthebadger" );
			}
		}
	}
	else
	{
		// get some tokens to process
		stdx::fast_vector <gpstring> tokens;
		stringtool::TokenizeCommandLine( tokens, sCommand );
		CommandLine command( sCommand );

		// switch on type
		if ( !tokens.empty() )
		{
			// MOVIE (params )

			if ( tokens[ 0 ].same_no_case( "movie" ) )
			{
				gpstring prefix = command.GetString( "prefix" );
				float fps       = command.GetFloat ( "fps",      30 );
				float delay     = command.GetFloat ( "delay",    10 );
				float duration  = command.GetFloat ( "duration", 30 );

				if ( prefix.empty() )
				{
					prefix = "clip";
				}

				gpscreenf(( "%g-second %g FPS '%s' movie starting in %g seconds...\n", duration, fps, prefix.c_str(), delay ));
				gRapiAppModule.StartCaptureMovie( prefix, fps, delay, duration );

				// this is not a cheat, no need to broadcast it
				isCheat = false;
			}
		}
	}	
	return ( isCheat );
}


void UIGameConsole :: EnableConsoleUpdate( bool bEnable )
{
	if ( bEnable && gWorldState.GetCurrentState() == WS_SP_NIS )
	{
		return;
	}

	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		if ( gUIShell.IsInterfaceVisible( "dialogue_box" ) || gWorldState.GetCurrentState() == WS_SP_NIS  )
		{
			pChat->SetDisableUpdate( !bEnable );
		}
		else
		{
			pChat->SetMinimized( !bEnable );
			if ( bEnable )
			{
				pChat->SetLineSpeedUp( 0.0f );
			}
			else
			{
				pChat->SetLineSpeedUp( 4.0f );
			}
		}

		if ( bEnable )
		{
			pChat->SetDisableUpdate( false );
		}
	}
}


bool UIGameConsole :: ChatHistoryUp()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->ViewHistoryUp();
	}

	return true;
}


bool UIGameConsole :: ChatHistoryDown()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->ViewHistoryDown();
	}

	return true;
}


bool UIGameConsole :: ChatHistoryClear()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->Clear();
	}

	return true;
}


bool UIGameConsole :: ChatHistoryLock()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->SetLockView( !pChat->GetLockView() );
	}

	return true;
}


void UIGameConsole::ConsoleRollover()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->SetRolloverView( true );
	}
}


void UIGameConsole::ConsoleRolloff()
{
	UIChatBox * pChat = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( pChat )
	{
		pChat->SetRolloverView( false );
	}
}

void UIGameConsole::ResizeChatWindow( eChatSize chatSize )
{
	if( chatSize == CHAT_SIZE_CURRENT )
	{
		chatSize = m_chatWindowSize;
	}

	m_chatWindowSize = chatSize;

	UIChatBox *chatBox = (UIChatBox *)gUIShell.FindUIWindow( "console_box", "console_output" );
	if ( chatBox )
	{
		UIDialogBox *dialogBox = NULL; 
		
		if( chatSize == CHAT_SIZE_MIN )
		{
			dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "min_console_box_size", "console_output" );
		}
		else if( chatSize == CHAT_SIZE_MID )
		{
			dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "mid_console_box_size", "console_output" );
		}
		else if( chatSize == CHAT_SIZE_MAX )
		{
			dialogBox = (UIDialogBox *)gUIShell.FindUIWindow( "max_console_box_size", "console_output" );
		}
		else
		{
			gperrorf(("Unknown window size: %d called in ResizeChatWindow", chatSize));
		}

		if( dialogBox )
		{
			chatBox->GetRect().bottom = dialogBox->GetRect().bottom;

			if( chatSize == CHAT_SIZE_MIN )
			{
				chatBox->SetSliderVisible( false );
			}
			else
			{
				chatBox->SetSliderVisible( true );
			}
			chatBox->RefreshSlider();
		
			UIDialogBox *outlineBox = (UIDialogBox *)gUIShell.FindUIWindow( "chat_text_border", "console_output" );
			if( outlineBox )
			{
				outlineBox->GetRect().bottom = dialogBox->GetRect().bottom;
			}

			int offsetx = dialogBox->GetRect().right;
			int offsety = dialogBox->GetRect().bottom;
			int offsetyTop = dialogBox->GetRect().top;

			UIWindow *defaultButton = (UIWindow *)gUIShell.FindUIWindow( "button_chat_max_size", "console_output" );
			if( defaultButton )
			{
				// set this group to the default size
				ResizeWindowTemplate("button_chat_min",				"button_chat_min_size",		"console_output",	offsetx - defaultButton->GetRect().right, offsetyTop - defaultButton->GetRect().top);
				ResizeWindowTemplate("button_chat_mid",				"button_chat_mid_size",		"console_output",	offsetx - defaultButton->GetRect().right, offsetyTop - defaultButton->GetRect().top);
				ResizeWindowTemplate("button_chat_max",				"button_chat_max_size",		"console_output",	offsetx - defaultButton->GetRect().right, offsetyTop - defaultButton->GetRect().top);
			}
			if( ::IsMultiPlayer() )
			{
				ResizeWindowTemplate("combo_box_players",			"combo_box_players_size",	"game_console_mp",	0,offsety);
				ResizeWindowTemplate("button_combo_show_players",	"button_combo_show_players_size","game_console_mp",0,offsety);
				ResizeWindowTemplate("listbox_players",				"listbox_players_size",		"game_console_mp",	0,offsety);
				ResizeWindowTemplate("console_edit",				"console_edit_size",		"game_console_mp",	0,offsety);
				ResizeWindowTemplate("dialog_box_console_bg",		"dialog_box_console_bg_size","game_console_mp",	0,offsety);
				ResizeWindowTemplate("dialog_players_combo_bg",		"dialog_players_combo_bg_size","game_console_mp",0,offsety);
				ResizeWindowTemplate("text_send",					"text_send_size",			"game_console_mp",	0,offsety);
				ResizeWindowTemplate("window_chat_bg",				"window_chat_bg_size",		"game_console_mp",	0,offsety);
				ResizeWindowTemplate("window_divider",				"window_divider_size",		"game_console_mp",	0,offsety);
			}
		}
	}
}

void UIGameConsole :: ResizeWindowTemplate( const char* windowName, const char* sizeTemplate, const char* interfaceName, int offsetx, int offsety )
{
	UIWindow *pWindow = (UIWindow *)gUIShell.FindUIWindow( windowName, interfaceName );
	if( pWindow )
	{
		UIWindow *pSizeTemplate = (UIWindow *)gUIShell.FindUIWindow( sizeTemplate, interfaceName );
		if( pSizeTemplate )
		{
			pWindow->GetRect().left = pSizeTemplate->GetRect().left + offsetx;
			pWindow->GetRect().right = pSizeTemplate->GetRect().right + offsetx;

			pWindow->GetRect().bottom = pSizeTemplate->GetRect().bottom + offsety;
			pWindow->GetRect().top = pSizeTemplate->GetRect().top + offsety;
		}
	}
	else
	{
		gperrorf( ("An interface relating to: %s, %s, or %s does not exist \n some windows might be in the wrong place!", windowName, interfaceName, sizeTemplate ) );
	}
}

