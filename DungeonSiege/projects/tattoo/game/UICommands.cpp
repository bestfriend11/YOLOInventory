/**************************************************************************

Filename		: UICommands.cpp

Class			: UICommands

Description		: Handles the command interface for the user 

Author			: Chad Queen

					(C)opyright Gas Powered Games 1999

**************************************************************************/



// Include Files
#include "precomp_game.h"

#include "appmodule.h"
#include "uicommands.h"
#include "inputbinder.h"
#include "aiaction.h"
#include "aiquery.h"
#include "GoBody.h"
#include "enchantment.h"
#include "Go.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "GoAttack.h"
#include "GoCore.h"
#include "GoAspect.h"
#include "GoMagic.h"
#include "GoParty.h"
#include "GoConversation.h"
#include "GoStore.h"
#include "GoShmo.h"
#include "siege_engine.h"
#include "siege_mouse_shadow.h"
#include "siege_pos.h"
#include "ui_shell.h"
#include "ui_cursor.h"
#include "UIInventoryManager.h"
#include "uigame.h"
#include "uipartymanager.h"
#include "worldmessage.h"
#include "player.h"
#include "GoDefs.h"
#include "GuiHelper.h"
#include "Job.h"
#include "UIStoreManager.h"
#include "GoActor.h"
#include "GameAuditor.h"
#include "UIMenuManager.h"
#include "Server.h"
#include "WorldState.h"


// Constructor 
UICommands::UICommands()
{	
	m_bFriendlyFire = false;
	m_ActiveCursor = CURSOR_GUI_STANDARD;
	m_bAttackSoundPlayed = false;
}



// Destructor
UICommands::~UICommands()
{
}



// Take selected objects and attack the target
void UICommands::CommandAttack( Goid target, bool noqueueclear, bool breakObject )
{
	gUIPartyManager.HandleMenusFromCommand();

	// Get a collection of all selected party members
	GoidColl selectedparty;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	GoHandle hTarget( target );
	gpassert( hTarget.IsValid() );

	if ( !IsAlive( hTarget->GetAspect()->GetLifeState() ) )
	{
		return;
	}

	// Make sure we have the ability to attack this target
	if ( ( hTarget->HasActor() && hTarget->GetActor()->GetAlignment() == AA_GOOD && !GetFriendlyFire() ) ||
		 ( hTarget->HasActor() == false && hTarget->IsBreakable() == false ) )
	{
		return;
	}

	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

	bool bPlayedOutOfMana	= false;
	bool bCasting			= false;
	bool bRangedOnly		= gAppModule.GetShiftKey();

	// Let's check everyone and see if they can make the team!
	GoidColl::iterator j;
	GoidColl attackers;	
	GoHandle hMember;
	for ( j = selectedparty.begin(); j != selectedparty.end(); ++j )
	{	
		hMember = *j;

		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() || hMember->GetInventory()->IsPackOnly() ) 
		{
			continue;
		}

		if ( (hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL || hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) && hMember->GetInventory()->GetSelectedSpell() )
		{
			if ( hMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( hTarget ) )
			{
				if ( hMember->GetAspect()->GetCurrentMana() < hMember->GetInventory()->GetSelectedSpell()->GetMagic()->EvaluateManaCost( hMember, hTarget ) )
				{
					if ( !bPlayedOutOfMana )
					{
						hMember->PlayVoiceSound( "order_out_of_mana", false );
						bPlayedOutOfMana = true;
					}
					gpscreenf( ( $MSG$ "%S does not have enough mana.\n", hMember->GetCommon()->GetScreenName().c_str() ) );				
					continue;
				}	
				else
				{
					bCasting = true;
				}
			}
			else
			{
				continue;
			}
		}

		if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
				hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
		{
			if ( (!bRangedOnly ||
				  bRangedOnly && hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON) )				  
			{
				attackers.push_back( *j );
				if ( breakObject )
				{
					break;
				}
			}
		}
		else
		{
			bCasting = false;
		}
	}

	if ( bCasting && breakObject && attackers.size() != 0 )
	{
		GoHandle hAttacker( *(attackers.begin()) );
		if ( hAttacker.IsValid() )
		{
			CommandCast( hAttacker, hTarget, noqueueclear );
		}
	}
	else
	{
		gAIAction.RSDoJob( MakeJobReq( JAT_ATTACK_OBJECT, JQ_ACTION, qplace, AO_USER, hTarget->GetGoid() ), attackers );
	}

	if ( breakObject && hMember.IsValid() )
	{	
		float distance = 2.0f;
		GoidColl members;
		GoidColl::iterator i;
		gUIPartyManager.GetSelectedPartyMembers( members );
		// have the non-packmules follow first to keep the formation in line.
		for ( i = members.begin(); i != members.end(); ++i )
		{	
			if ( *i != hMember->GetGoid() )
			{
				GoHandle hFollower( *i );
				if( !hFollower->GetInventory()->IsPackOnly() )
				{
					JobReq tempReq( MakeJobReq( JAT_FOLLOW, JQ_ACTION, qplace, AO_USER, hMember->GetGoid() ) );
					tempReq.m_Float1 = distance;
					
					hFollower->GetMind()->SDoJob( tempReq );
					distance += 1.5f;
				}
			}
		}
		// have the packmules follow 
		for ( i = members.begin(); i != members.end(); ++i )
		{	
			if ( *i != hMember->GetGoid())
			{
				GoHandle hFollower( *i );
				if( hFollower->GetInventory()->IsPackOnly() )
				{
					JobReq tempReq( MakeJobReq( JAT_FOLLOW, JQ_ACTION, qplace, AO_USER, hMember->GetGoid() ) );
					tempReq.m_Float1 = distance;
					
					hFollower->GetMind()->SDoJob( tempReq );
					distance += 1.5f;
				}
			}
		}

	}

	if ( attackers.size() != 0 )
	{
		GoHandle hAttacker( *(attackers.begin()) );
		if ( hAttacker.IsValid() )
		{
			if ( breakObject && bCasting )
			{
				return;
			}
			else
			{
				hAttacker->PlayVoiceSound( "order_attack", false );
			}
		}
	}
	else if ( hMember.IsValid() )
	{
		hMember->PlayVoiceSound( "order_cant_move", false );
	}
}


void UICommands::CommandAttack( SiegePos spos, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoidColl selectedParty;
	gUIPartyManager.GetSelectedPartyMembers( selectedParty );
	
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;
		
	bool bPlayedSound		= false;
	bool bPlayedOutOfMana	= false;
	GoidColl::iterator iParty;
	for ( iParty = selectedParty.begin(); iParty != selectedParty.end(); ++iParty )
	{
		GoHandle hMember( *iParty );

		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
		{
			continue;
		}

		if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
			  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
		{

			if ( (hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL || 
				 hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) )
			{
				Go * pSpell = hMember->GetInventory()->GetSelectedSpell();
				if ( hMember->GetAspect()->GetCurrentMana() >= pSpell->GetMagic()->EvaluateManaCost( hMember, NULL ) )
				{
					if ( ( pSpell->GetMagic()->GetTargetTypeFlags() & TT_TERRAIN) == pSpell->GetMagic()->GetTargetTypeFlags() )
					{
						hMember->GetMind()->RSDoJob( MakeJobReq( JAT_ATTACK_POSITION, JQ_ACTION, qplace, AO_USER, spos ) );
					}
				}
				else
				{
					gpscreenf( ( $MSG$ "%S does not have enough mana.\n", hMember->GetCommon()->GetScreenName().c_str() ) );				

					if ( !bPlayedOutOfMana )
					{
						hMember->PlayVoiceSound( "order_out_of_mana", false );
						bPlayedOutOfMana = true;
					}
				}
			}
			else
			{
				hMember->GetMind()->RSDoJob( MakeJobReq( JAT_ATTACK_POSITION, JQ_ACTION, qplace, AO_USER, spos ) );

				if ( !bPlayedSound )
				{
					bPlayedSound = true;
					hMember->PlayVoiceSound( "order_attack", false );
				}
			}
		}
	}
}

void UICommands::CommandAttack( Go * pMember, Go * pObject, bool noqueueclear )
{
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

	if ( gUIPartyManager.IsCharacterInterfaceBusy( pMember ) || pMember->IsGhost() )
	{
		return;
	}

	if ( !( pMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		  pMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
	{
		if ( (pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL || 
			 pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL))
		{
			CommandCast( pMember, pObject, noqueueclear );
		}
		else
		{
			pMember->GetMind()->RSDoJob( MakeJobReq( JAT_ATTACK_OBJECT, JQ_ACTION, qplace, AO_USER, pObject->GetGoid() ) );

			if ( !m_bAttackSoundPlayed )
			{				
				pMember->PlayVoiceSound( "order_attack", false );
				m_bAttackSoundPlayed = true;
			}
		}
	}	
}


// Cast a spell on the target
void UICommands::CommandCast( Goid target, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoHandle hMember;

	GoidColl selectedparty;
	gUIGame.CreateSelectedActorPartyList( selectedparty );

	bool bPlayedOutOfMana = false;

	GoidColl::iterator i;
	for( i = selectedparty.begin(); i < selectedparty.end(); ++i )
	{
		hMember = *i;		

		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
		{
			continue;
		}

		GoHandle hTarget( target );
		gpassert( hTarget.IsValid() );
		
		bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
		eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

		Go * pSpell = hMember->GetInventory()->GetSelectedItem();

		if( pSpell && pSpell->IsSpell() )
		{
			if ( hMember->GetAspect()->GetCurrentMana() >= hMember->GetInventory()->GetSelectedSpell()->GetMagic()->EvaluateManaCost( hMember, hTarget ) )
			{
				if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
					  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
				{
					if ( hMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( hTarget ) )
					{
						hMember->GetMind()->RSDoJob( MakeJobReq( JAT_CAST, JQ_ACTION, qplace, AO_USER, target, hMember->GetInventory()->GetSelectedSpell()->GetGoid() ));											
					}
				}
			}
			else
			{
				if ( !hMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( hTarget ) )
				{
					gpscreenf( ( $MSG$ "%S does not have enough mana.\n", hMember->GetCommon()->GetScreenName().c_str() ) );				

					if ( !bPlayedOutOfMana )
					{
						hMember->PlayVoiceSound( "order_out_of_mana", false );
						bPlayedOutOfMana = true;
					}
				}
			}			
		}
	}
}


void UICommands::CommandCast( Go * pMember, Go * pObject, bool noqueueclear )
{
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

	bool bPlayedOutOfMana = false;

	if ( gUIPartyManager.IsCharacterInterfaceBusy( pMember ) || pMember->IsGhost() )
	{
		return;
	}

	if ( !pMember->GetInventory()->GetSelectedSpell() || !pMember->GetInventory()->GetSelectedSpell()->HasMagic() )
	{	
		return;
	}

	if ( pMember->GetAspect()->GetCurrentMana() >= pMember->GetInventory()->GetSelectedSpell()->GetMagic()->EvaluateManaCost( pMember, pObject ) )
	{
		if ( !( pMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
			  pMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
		{	
			if ( pMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( pObject ) )
			{
				bool bCast = true;
				gpstring sState = pMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetCasterStateName();
				if ( pMember->GetActor()->HasGenericState( pMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetCasterStateName() ) &&
				     sState.same_no_case( "controlling" ) )
				{
					gpscreenf( ( $MSG$ "%S can only control one creature at a time.\n", pMember->GetCommon()->GetScreenName().c_str() ) );
					bCast = false;
				}					

				if ( bCast )
				{
					pMember->GetMind()->RSDoJob( MakeJobReq( JAT_CAST, JQ_ACTION, qplace, AO_USER, pObject->GetGoid(), pMember->GetInventory()->GetSelectedSpell()->GetGoid() ));				
				}
			}
		}	
	}
	else
	{	
		gpscreenf( ( $MSG$ "%S does not have enough mana.\n", pMember->GetCommon()->GetScreenName().c_str() ) );				

		if ( !bPlayedOutOfMana )
		{
			pMember->PlayVoiceSound( "order_out_of_mana", false );
			bPlayedOutOfMana = true;
		}
	}
}


void UICommands::CommandCast( SiegePos spos, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoidColl selectedParty;
	gUIPartyManager.GetSelectedPartyMembers( selectedParty );
	
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

	bool bPlayedOutOfMana = false;
	
	GoHandle hMember;
	bool bCast = false;

	GoidColl::iterator iParty;
	for ( iParty = selectedParty.begin(); iParty != selectedParty.end(); ++iParty )
	{
		hMember = *iParty;

		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() || hMember->GetInventory()->IsPackOnly() )
		{
			continue;
		}

		if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
			  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
		{
			Go * pSpell = hMember->GetInventory()->GetSelectedSpell();
			if( pSpell )
			{
				if ( hMember->GetAspect()->GetCurrentMana() >= pSpell->GetMagic()->EvaluateManaCost( hMember, NULL ) )
				{
					if ( ( pSpell->GetMagic()->GetTargetTypeFlags() & TT_TERRAIN) && 
						   !hMember->GetActor()->HasGenericState( hMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetCasterStateName() ) )
					{
						hMember->GetMind()->RSDoJob( MakeJobReq( JAT_CAST_POSITION, JQ_ACTION, qplace, AO_USER, spos, pSpell->GetGoid() ) );
					}
					else if ( hMember->GetActor()->HasGenericState( hMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetCasterStateName() ) &&
							  hMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetCasterStateName().same_no_case( "controlling" ) )
					{
						gpscreenf( ( $MSG$ "%S can only control one creature at a time.\n", hMember->GetCommon()->GetScreenName().c_str() ) );
					}					
					bCast = true;
				}
				else
				{
					gpscreenf( ( $MSG$ "%S does not have enough mana.\n", hMember->GetCommon()->GetScreenName().c_str() ) );				

					if ( !bPlayedOutOfMana )
					{
						hMember->PlayVoiceSound( "order_out_of_mana", false );
						bPlayedOutOfMana = true;
					}

					bCast = true;
				}
			}
			else
			{
				if ( !bCast )
				{
					gpscreen( $MSG$ "You do not have a spell selected.\n" );
				}
			}
		}
	}


}


// Drop an object at a specified position
void UICommands::CommandDrop( Goid object, const SiegePos & siegepos, bool noqueueclear, bool bDeactivateItems, Go * pDropper )
{
	if( object != GOID_INVALID ) 
	{	
		GoHandle hMember;
		if ( pDropper )
		{
			hMember = pDropper->GetGoid();
		}
		else
		{
			hMember = gUIGame.GetActorWhoCarriesObject( object );
		}

		SiegePos droppos = siegepos;		

		if( hMember.IsValid() ) 
		{
			if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
			{
				return;
			}

			if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
				  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
			{
				bool bClearQ = !noqueueclear;
				eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

				GoHandle hObject( object );

				// need to see if any data we're getting is invalid
				if ( !hMember->HasMind() || !hObject.IsValid() || hObject->HasAspect() )
				{
					if ( !hMember->HasMind() )
					{
						gperrorf( ( "Member: %S has no mind?  Please tell Chad.", hMember->GetCommon()->GetScreenName().c_str() ) );
						return;
					}

					if ( !hObject.IsValid() )
					{
						gperror( "Object that you are trying to drop is invalid." );
						return;
					}

					if ( !hObject->HasAspect() )
					{
						gperrorf( ( "Object: %S that you are trying to drop has no aspect.", hObject->GetCommon()->GetScreenName().c_str() ) );
						return;
					}
				}

				SiegePos	dropPos;	
				Quat		dropQuat;
				hMember->GetInventory()->RetrieveDropPositionAndOrientation( NULL, hObject, dropPos, dropQuat );	
				
				if (hObject->HasGold())
				{
					hMember->GetMind()->RSDropGold( hObject->GetAspect()->GetGoldValue(), dropPos, qplace, AO_USER );
				}
				else
				{
					hMember->GetMind()->RSDrop( hObject, dropPos, qplace, AO_USER );				
					
				}

				if ( bDeactivateItems )
				{
					gUI.GetGameGUI().GetMessenger()->SendUIMessage( UIMessage( MSG_DEACTIVATEITEMS ) );
				}
			}
		}
		else 
		{	
			// Object isn't owned by anyone - do nothing			
			return;
		}		
	}
	else 
	{	
		// Object isn't valid
		gpassertm( 0, "CommandDrop: Object isn't valid!!!" );
	}	
}


// Unequip an object
void UICommands::CommandUnequip( const eEquipSlot slot, Goid actor )
{
	CommandStop();

	if (( gGoDb.GetSelection().size() == 1 ) || ( actor != GOID_INVALID )) 
	{
		GoHandle hMember;
		if ( actor != GOID_INVALID ) {
			hMember = actor;
		}
		else {
			hMember = (*(gGoDb.GetSelection().begin()))->GetGoid();
		}

		if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
		{
			return;
		}	
		
		Go * object = hMember->GetInventory()->GetEquipped( slot );

		if( object )
		{
			if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
				  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
			{
				hMember->GetMind()->RSDoJob( MakeJobReq( JAT_EQUIP, JQ_ACTION, QP_FRONT, AO_USER, GOID_NONE, slot ) );							
			}
		}
	}	
}

void UICommands::CommandEquip( Goid object, const eEquipSlot slot, bool noqueueclear, bool bUseControl )
{
	CommandEquip( object, slot, gUIGame.ClosestSelectedCharacter( object ), noqueueclear, bUseControl );
}

// Equip an object
void UICommands::CommandEquip( Goid object, const eEquipSlot slot, Goid actor, bool noqueueclear, bool bUseControl )
{
	// Make sure we can really equip the object
	if ( object == GOID_INVALID ) {
		return;
	}

	GoHandle goalObject( object );
		
	if( goalObject->GetGui()->GetEquipSlot() == ES_NONE )
	{
		gpassertm( 0, "Trying to 'equip' something that can't be equipped!" );
		return;
	}

	Goid character = actor;
	if ( character == GOID_INVALID ) {
		return; 
	}
	GoHandle hMember( character );
	if( hMember->GetGoid() == object || !hMember->HasActor() )
	{
		gpassertm( 0, "Trying to 'equip' yourself or not a valid actor!" );
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
	{
		return;
	}


	//----- tell the ai to equip the item
	bool bClearQ = !noqueueclear && (bUseControl ? (!gAppModule.GetControlKey()) : true);
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;
	GoHandle hObject( object );

	if ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) 
	{
		return;
	}

	hMember->GetMind()->RSDoJob( MakeJobReq( JAT_EQUIP, JQ_ACTION, qplace, AO_USER, hObject->GetGoid(), ES_ANY ));

	// end tell ai

	
	// Begin GUI HANDLING 
	hMember->GetInventory()->SetInventoryDirty();
	
	eEquipSlot projected_slot = ES_NONE;
	
	if( slot == ES_NONE )
	{
		projected_slot = goalObject->GetGui()->GetEquipSlot();
	}
	else
	{
		projected_slot = slot;
	}

	{
		Go * item_in_slot = 0;
		if ( projected_slot != ES_RING )
		{
			item_in_slot = hMember->GetInventory()->GetEquipped( projected_slot );
		}

		if ( item_in_slot ) 
		{
			return;
		}
	}		
		
	GoHandle item( object );	

	if (( projected_slot == ES_SHIELD_HAND ) && ( item->HasAttack() )) {
		projected_slot = ES_SHIELD_HAND;

		Go * item_in_slot = hMember->GetInventory()->GetEquipped( projected_slot );
		if ( item_in_slot ) {
			return;
		}

		gUIInventoryManager.SetAsEquipped( hMember, item, "ranged_weapon" );
	}		
	else if ( projected_slot == ES_WEAPON_HAND )
	{
		Go * item_in_slot = hMember->GetInventory()->GetEquipped( ES_SHIELD_HAND );

		if ( item_in_slot )
		{
			if ( item_in_slot->HasAttack() )
			{
				return;
			}
			else {
				hMember->GetInventory()->RSSetLocation( item_in_slot, IL_SHIELD );
				gUIInventoryManager.SetAsEquipped( hMember, item, "melee_weapon" );
			}
		}
	}
	else if (( projected_slot == ES_SHIELD_HAND ) && ( !item->HasAttack() ) && ( hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON ))
	{
		if ( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) )
		{
			CommandEquip( hMember->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid(), ES_SHIELD_HAND );
		}
		hMember->GetInventory()->RSSetLocation( item, IL_SHIELD );
	}
	else 
	{
		switch ( projected_slot )
		{
		case ES_CHEST:
			gUIInventoryManager.SetAsEquipped( hMember, item, "armor" );
			break;
		case ES_HEAD:
			gUIInventoryManager.SetAsEquipped( hMember, item, "helmet" );
			break;
		case ES_FEET:
			gUIInventoryManager.SetAsEquipped( hMember, item, "boots" );
			break;
		case ES_SHIELD_HAND:			
			gUIInventoryManager.SetAsEquipped( hMember, item, "shield" );
			break;
		}		
	}
	// End GUI HANDLING 
}


// Get an object
void UICommands::CommandGet( Goid object, bool noqueueclear, ActionSource action_source, bool bForceGet )
{
	gUIPartyManager.HandleMenusFromCommand();

	// Make sure we can really get the object
	GoHandle goalObject( object );
	if ( !(goalObject->HasGui() ) ) {
		gpassertm( 0, "Trying to 'get' something that can't be picked up!" );
		return;
	}

	GoHandle hMember( gUIGame.ClosestSelectedCharacter( object ) );
	if ( !hMember.IsValid() || (bForceGet && !hMember->GetInventory()->TestGet( object, true )) )
	{
		Go *pMember = hMember.GetUnsafe();
		hMember = GOID_INVALID; // hMember is invalid

		GoidColl partyColl;
		GoidColl::iterator iMember;
		gUIPartyManager.GetPortraitOrderedParty( partyColl );
		for ( iMember = partyColl.begin(); iMember != partyColl.end(); ++iMember )
		{
			GoHandle hTestMember( *iMember );			

			if ( hTestMember.IsValid() && hTestMember->IsGhost() )
			{
				continue;
			}

			if ( hTestMember.IsValid() )
			{
				pMember = hTestMember.GetUnsafe();
			}

			if ( hTestMember.IsValid() && hTestMember->IsSelected() &&				
				 hTestMember->IsInActiveWorldFrustum() && 
				 (bForceGet || hTestMember->GetInventory()->TestGet( object, true )) )
			{				
				hTestMember->GetInventory()->RSSetForceGet( bForceGet );
				hMember = *iMember;
				break;
			}
		}

		// If we couldn't find a valid picker-upper, do another TestGet just 
		// to get the "inventory full" sound.
		if ( !hMember.IsValid() && NULL != pMember && !bForceGet )
		{
			pMember->GetInventory()->TestGet( object );
		}
	}
	else if ( hMember->GetInventory()->TestGet( object, true ) || bForceGet )
	{
		hMember->GetInventory()->RSSetForceGet( bForceGet );
	}
	else
	{		
		GoidColl selectedParty;
		GoidColl::iterator iParty;
		gUIPartyManager.GetSelectedPartyMembers( selectedParty );
		bool bCanPickup = false;
		for ( iParty = selectedParty.begin(); iParty != selectedParty.end(); ++iParty )
		{
			hMember = *iParty;
			if ( hMember.IsValid() && hMember->GetInventory()->TestGet( object, true ) )
			{
				bCanPickup = true;
				break;
			}
		}		

		if ( !bCanPickup && hMember.IsValid() )
		{
			hMember->GetInventory()->TestGet( object );	
			return;
		}
	}
	
	if ( hMember.IsValid() && (hMember->GetGoid() == object || !hMember->HasActor()) ) 
	{
		gpassertm( 0, "Trying to 'get' yourself or not a valid actor!" );
		return;
	}
	else if ( !hMember.IsValid() )
	{
		return;
	}

	if ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		 hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) 
	{
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
	{
		return;
	}

	// Add this item to the actor's inventory, if it can hold it
	bool bAutoPlace = false;
	if ( action_source == SOURCE_ACTOR ) 
	{
		bAutoPlace = true;
	}


	// begin tell ai
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;
	GoHandle hObject( object );
	hMember->GetMind()->RSGet( hObject, qplace, AO_USER );	
	// end tell ai
	
	UIItem *pItem = gUI.GetGameGUI().FindItemByID( MakeInt(object) );
	if ( pItem ) 
	{
		pItem->SetParentGridbox( hMember->GetInventory()->GetGridbox() );
	}
}



// Give an object to another character
bool UICommands::CommandGive( Goid object, Goid target, bool noqueueclear )
{
	if ( object != GOID_INVALID ) 
	{
		Goid actor = gUIGame.GetActorWhoCarriesObject( object );
		GoHandle hMember( actor );
		
		if ( hMember.IsValid() && hMember->GetGoid() != target ) 
		{
			if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
			{
				return false;
			}

			if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
				  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
			{	
				// begin tell ai
				bool bClearQ = !noqueueclear;
				eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;
				GoHandle hObject( object );
				GoHandle hTarget( target );

				if ( !hTarget )
				{
					gperror( "The target you are giving to appears to be invalid, give aborted." );
					return false;
				}

				if ( !hTarget->IsInActiveWorldFrustum() || !hTarget->IsScreenPartyMember() )
				{
					return false;
				}
				
				bool bCanGive = true;
				if ( gUIStoreManager.GetStoreActive() )
				{
					GoHandle hStore( gUIStoreManager.GetActiveStore() );
					if ( hStore.IsValid() )
					{
						if ( !gAIQuery.IsInRange(	hObject->GetPlacement()->GetPosition(), hTarget->GetPlacement()->GetPosition(),
													hStore->GetStore()->GetActivateRange() ) )				
						{
							bCanGive = false;
						}
					}
							
				}				

				if ( bCanGive )
				{
					hMember->GetMind()->RSGive( hTarget, hObject, qplace, AO_USER );
					return true;
				}
				else
				{
					return false;
				}
				
				// end tell ai
			}
		}		
	}
	else 
	{	
		// Object isn't valid
		gpassertm( 0, "CommandGive: Object isn't valid!!!" );
	}
	return false;
}



// Guard a specified target
void UICommands::CommandGuard( Goid target, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoidColl selectedparty;
	GoidColl::iterator i;
	GoidColl guards;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );
	
	GoHandle hTarget( target );

	for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
	{
		GoHandle hMember( *i );
		if ( hMember.IsValid() )
		{
			if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || hMember->IsGhost() )
			{
				continue;
			}

			if ( !hTarget->HasActor() || !hMember->GetMind()->IsFriend( hTarget ) )
			{
				continue;
			}

			if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION )
					&& ( hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
					&& ( hMember->GetMind()->CanOperateOn( hTarget ) ) )
			{
				guards.push_back( *i );
			}			
		}
	}

	// begin tell ai
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;	

	gAIAction.RSDoJob( MakeJobReq( JAT_GUARD, JQ_ACTION, qplace, AO_USER, hTarget->GetGoid() ), guards );

	// end tell ai
}



// Move to a specified area specified target
void UICommands::CommandMove( const SiegePos & siegepos, bool patrol, bool noqueueclear )
{	
	gUIPartyManager.HandleMenusFromCommand();

	if ( !patrol )
	{	
		Formation * formation	= gServer.GetScreenFormation();
		Go * party				= gServer.GetScreenParty();
		GoidColl selectedparty;
		gUIPartyManager.GetSelectedPartyMembers( selectedparty );

		vector_3 test;
		if ( gSiegeEngine.IsNodeValid( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node ) &&
		     gSiegeEngine.IsNodeValid( party->GetPlacement()->GetPosition().node ) )
		{
			vector_3 Direction = -gSiegeEngine.GetDifferenceVector( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), party->GetPlacement()->GetPosition() );
			if ( Direction.IsZero() )
			{
				return;
			}

			gUIGame.RankPartyMembers();
			formation->SetPosition( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
			formation->SetDirection( Direction );
			formation->SetShape( formation->GetShape() );
			bool bMoved = formation->Move( gAppModule.GetControlKey() ? QP_BACK : QP_CLEAR, AO_USER, gUIPartyManager.GetFollowMode(), true );
			
			formation->SetDrawFreeSpots( false );
			formation->SetDrawAssignedSpots( false );

			formation->HideSpots();
			formation->SetDirty( false );

			GoHandle hMember;
			if( !selectedparty.empty() ) 
			{
				hMember = selectedparty.front();
				if ( bMoved )
				{
					InsertWaypoint( siegepos );
					hMember->PlayVoiceSound( "order_move", false );
				}
				else
				{
					hMember->PlayVoiceSound( "order_cant_move", false );
				}
			}	
		}
	}
	else
	{
		// begin tell ai
		bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
		eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

		if ( gGoDb.GetSelection().size() == 0 ) 
		{
			return;
		}

		GoidColl selectedparty;
		GoidColl::iterator i;
		GoidColl movers;
		gUIPartyManager.GetSelectedPartyMembers( selectedparty );

		for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
		{
			GoHandle hMember( *i );
			if ( hMember.IsValid() )
			{
				if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
				{
					continue;
				}

				if ( !( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
					  hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK ) )
				{
					movers.push_back( *i );		
				}
			}
		}

		if( movers.empty() )
		{
			return;
		}

		gAIAction.RSDoJob( MakeJobReq( JAT_PATROL, JQ_ACTION, qplace, AO_USER, siegepos ), movers );

		GoHandle hMover( *(movers.begin()) );
		if ( hMover.IsValid() )
		{
			InsertWaypoint( siegepos );
			hMover->PlayVoiceSound( "order_move", false );
		}
	}
}


void UICommands::CommandMove()
{
	Formation * formation	= gServer.GetScreenFormation();
	Go * party				= gServer.GetScreenParty();
	
	vector_3 Direction = -gSiegeEngine.GetDifferenceVector( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), party->GetPlacement()->GetPosition() );

	gUIGame.RankPartyMembers();
	formation->SetPosition( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
	formation->SetDirection( Direction );
	formation->SetShape( formation->GetShape() );
	formation->Move( QP_BACK, AO_USER, gUIPartyManager.GetFollowMode() );	
	InsertWaypoint( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
}


void UICommands::InsertWaypoint( const SiegePos & spos )
{
	GoCloneReq cloneReq( "click_waypoint" );
	cloneReq.SetStartingPos( spos );	
	GoHandle hWaypoint( gGoDb.CloneLocalGo( cloneReq ) );
	if ( hWaypoint.IsValid() )
	{
		gGoDb.SMarkForDeletion( hWaypoint->GetGoid(), true, true );
	}
}


// Patrol between two positions
void UICommands::CommandPatrol( const SiegePos & siegepos, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	CommandMove( siegepos, true, noqueueclear );
}



// Stop whatever the actor is doing
void UICommands::CommandStop( void )
{
	GoidColl		selectedparty;
	GoidColl		stoppers;
	GoidColl::iterator i;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );
	for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
	{
		GoHandle hMember( *i );
		if ( hMember.IsValid() )
		{
			if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
			{
				continue;
			}

			bool bStop = true;
			JobQueue jobs = hMember->GetMind()->GetQ( JQ_ACTION );
			JobQueue::iterator j;
			for ( j = jobs.begin(); j != jobs.end(); ++j )
			{
				if ( ((*j)->GetJobAbstractType() == JAT_DRINK) ||
					 ((*j)->GetJobAbstractType() == JAT_USE) ||
					 ((*j)->GetJobAbstractType() == JAT_EQUIP) ) 
				{			
					bStop = false;
				}
			}

			if ( bStop )
			{
				stoppers.push_back( *i );			
			}
		}
	}
	
	gAIAction.RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_USER ), stoppers );

	if ( stoppers.size() != 0 )
	{
		GoHandle hStopper( *(stoppers.begin()) );
		if ( hStopper.IsValid() )
		{
			hStopper->PlayVoiceSound( "order_stop", false );
		}
	}
}


// Use an object
void UICommands::CommandUse( Goid object, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoHandle hMember( gUIGame.ClosestSelectedCharacter( object ) );	

	if ( !hMember.IsValid() || !hMember->HasActor() )
	{
		return;
	}

	if ( hMember->GetGoid() == object )
	{
		return;
	}

	if ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		 hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK )
	{
		return;
	}

	GoHandle hObject( object );

	if ( gUIPartyManager.IsCharacterInterfaceBusy( hMember ) || (hMember->IsGhost() && !hObject->IsGhostUsable() ) )
	{
		return;
	}

	if ( !hObject->IsUsable() && hObject->IsGhostUsable() && !hMember->IsGhost() )
	{
		return;
	}

	// begin tell ai
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;	

	if ( hObject->IsPotion() )
	{		
		hMember->GetMind()->RSDoJob( MakeJobReq( JAT_DRINK, JQ_ACTION, qplace, AO_USER, hObject->GetGoid() ) );
	}
	else
	{		
		hMember->GetMind()->RSUse( hObject, qplace, AO_USER );

	//$$
	//$$ I've removed the extra CommandMove here
	//$$
	//$$ CommandMove() doesn't seem to pay any attention to whether or not we are heading for a
	//$$ USE_POINT or if there is a party of 1 (remember the JOB_USE ai skrit will already do all
	//$$ the moving for the single character case)
	//$$ 
	//$$ Perhaps more hazardous is the way that CommandMove automatically assumes that you want everyone
	//$$ to move to the terrain hit location. In the case of a wall switch the hitpos may be on the other
	//$$ side of the wall from the use_point, or in an some other unreachable/far off area
	//$$
	//$$ This was causing the character to move around awkwardly immediately after using an item
	//$$
	//$$ --biddle
	
	//	CommandMove();

		float distance = 2.0f;
		GoidColl members;
		GoidColl::iterator i;
		gUIPartyManager.GetSelectedPartyMembers( members );
		for ( i = members.begin(); i != members.end(); ++i )
		{	
			if ( *i != hMember->GetGoid() )
			{
				JobReq tempReq( MakeJobReq( JAT_FOLLOW, JQ_ACTION, qplace, AO_USER, hMember->GetGoid() ) );
				tempReq.m_Float1 = distance;

				GoHandle hFollower( *i );
				hFollower->GetMind()->SDoJob( tempReq );
				distance += 1.5f;
			}
		}
	}
}


void UICommands::CommandUse( Goid object, Goid actor, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();

	GoHandle hMember( actor );
	GoHandle hObject( object );
	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;

	if ( hMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		 hMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK )
	{
		return;
	}
	
	if ( hObject.IsValid() && hMember.IsValid() )
	{		
		if ( hObject->IsPotion() )
		{
			hMember->GetMind()->RSDoJob( MakeJobReq( JAT_DRINK, JQ_ACTION, qplace, AO_USER, hObject->GetGoid() ) );
		}
		else
		{
			hMember->GetMind()->RSUse( hObject, qplace, AO_USER );
		}	
	}
}


void UICommands::CommandTalk( Go * pMember, Go * pObject )
{
	gUIPartyManager.HandleMenusFromCommand();

	if ( pMember->GetMind()->GetFrontJob( JQ_ACTION ) &&
		 pMember->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK )
	{
		return;
	}

	if ( gUIPartyManager.IsCharacterInterfaceBusy( pMember ) )
	{
		return;
	}
	
	// pMember->GetMind()->Clear( JQ_ACTION );
	pMember->GetMind()->RSStop( AO_REFLEX );
	pMember->GetMind()->RSDoJob( MakeJobReq( JAT_LISTEN, JQ_ACTION, QP_CLEAR, AO_USER, pObject->GetGoid() ) );
}


void UICommands::CommandTalk( Go * pObject, bool noqueueclear )
{
	gUIPartyManager.HandleMenusFromCommand();
	
	GoidColl selectedparty;	
	GoidColl::iterator i;
	gUIPartyManager.GetPortraitOrderedParty( selectedparty );

	bool bFound = false;
	GoHandle hMember( gUIGame.ClosestSelectedCharacter( pObject->GetGoid() ) );	
	if ( !hMember.IsValid() || !hMember->HasInventory() || 
		 hMember->GetInventory()->IsPackOnly() || !gUIGame.Contains( hMember->GetGoid(), selectedparty ) ||
		 gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
	{
		for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
		{
			hMember = *i;
			if ( hMember.IsValid() && hMember->IsSelected() && hMember->HasInventory() && !hMember->GetInventory()->IsPackOnly() &&
				 !gUIPartyManager.IsCharacterInterfaceBusy( hMember ) )
			{
				bFound = true;
				break;
			}
		}	
	}		
	else
	{
		bFound = true;
	}

	if ( bFound )
	{
		RSTalk( pObject, hMember, noqueueclear );			
	}
}


void UICommands::RSTalk( Go * pObject, Go * pMember, bool noqueueclear )
{
	FUBI_RPC_THIS_CALL( RSTalk, RPC_TO_SERVER )

	CHECK_SERVER_ONLY;

	if ( ::IsMultiPlayer() )
	{
		Job * pJob = pObject->GetMind()->GetFrontJob( JQ_ACTION );
		if ( pJob && pJob->GetJobAbstractType() == JAT_TALK )
		{
			Job * pMemberJob = pMember->GetMind()->GetFrontJob( JQ_ACTION );
			if ( pMemberJob && pMemberJob->GetJobAbstractType() != JAT_LISTEN )
			{
				RCTalkBusy( pObject, pMember->GetPlayer()->GetMachineId() );
				return;
			}
		}	
	}

	bool bClearQ = !noqueueclear && !gAppModule.GetControlKey();
	eQPlace qplace = bClearQ ? QP_CLEAR : QP_BACK;		
	pMember->GetMind()->RSDoJob( MakeJobReq( JAT_LISTEN, JQ_ACTION, qplace, AO_USER, pObject->GetGoid() ) );								
}


void UICommands::RCTalkBusy( Go * pObject, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCTalkBusy, machineId )

	gpwstring sOutput;
	sOutput.assignf( gpwtranslate( $MSG$ "%s is currently busy." ), pObject->GetCommon()->GetScreenName().c_str() );
	gUIGame.ShowDialog( DIALOG_OK, sOutput.c_str() );	

}

// This handles all the commands that the user EXPLICITLY
// specifies when clicking on the command buttons in the
// user interface.
bool UICommands::Selection_SelectionCommandOnCursor( const gpstring command_name, Goid object, bool noqueueclear )
{
	if ( command_name.same_no_case( "action_move" )	) 
	{
		return( SelectionCommandMove( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_patrol" ) ) 
	{
		return ( SelectionCommandPatrol( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_attack" ) ) 
	{
		return ( SelectionCommandAttack( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_cast" ) ) 
	{
		return ( SelectionCommandCast( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_guard" ) ) 
	{
		return ( SelectionCommandGuard( noqueueclear ) );
	}	
	else if ( command_name.same_no_case( "action_get" ) ) 
	{
		return ( SelectionCommandGet( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_give" ) ) 
	{
		return ( SelectionCommandGive( object, noqueueclear  ) );
	}
	else if ( command_name.same_no_case( "action_use" ))
	{
		return ( SelectionCommandUse( noqueueclear ) );
	}
	else if ( command_name.same_no_case( "action_stop" )) 
	{
		return ( SelectionCommandStop() );
	}
	return false;
}



// This explicitly handles the move command
bool UICommands::SelectionCommandMove( bool noqueueclear )
{
	if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandMove( siegepos, false, noqueueclear );
		return true;
	}
	return false;	
}
// For input binder callback ( no parameters )
bool UICommands::InputCommandMove()
{	
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandMove( siegepos, false, false );
		return true;
	}
	return false;	
}



// This explicitly handles the patrol command
bool UICommands::SelectionCommandPatrol( bool noqueueclear )
{
	if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandPatrol( siegepos, noqueueclear );
		return true;
	}
	return false;
}
bool UICommands::InputCommandPatrol()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandPatrol( siegepos, false );
		return true;
	}
	return false;
}



// This explicitly handles the attack command
bool UICommands::SelectionCommandAttack( bool noqueueclear )
{
	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandAttack( gUIGame.GetGoUnderCursor(), noqueueclear );
		return true;
	}
	else if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandAttack( siegepos, noqueueclear );
	}
	
	return false;	
}
bool UICommands::InputCommandAttack()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandAttack( gUIGame.GetGoUnderCursor(), false );
		return true;
	}
	else if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandAttack( siegepos, false );
	}
	return false;	
}


// This explicitly handles the cast command
bool UICommands::SelectionCommandCast( bool noqueueclear )
{
	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandCast( gUIGame.GetGoUnderCursor(), noqueueclear );
		return true;
	}
	else if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
	{
		CommandCast( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), noqueueclear );
		return true;
	}
	return false ;	
}
bool UICommands::InputCommandCast()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	SelectionCommandCast( false );
	return false ;	
}



// This explicitly handles the guard command
bool UICommands::SelectionCommandGuard( bool noqueueclear )
{
	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandGuard( gUIGame.GetGoUnderCursor(), noqueueclear );
		return true;
	}
	return false;	
}
bool UICommands::InputCommandGuard()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandGuard( gUIGame.GetGoUnderCursor(), false );
		return true;
	}
	return false;	
}


// This explicitly handles the get command
bool UICommands::SelectionCommandGet( bool noqueueclear )
{
	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandGet( gUIGame.GetGoUnderCursor(), noqueueclear );
		return true;
	}
	return false;	
}
bool UICommands::InputCommandGet()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandGet( gUIGame.GetGoUnderCursor(), false );
		return true;
	}
	return false;	
}



// This explicitly handles the give command
bool UICommands::SelectionCommandGive( Goid object, bool noqueueclear )
{	
	if ( gUIGame.GetGoUnderCursor() != GOID_INVALID ) 
	{
		CommandGive( object, gUIGame.GetGoUnderCursor(), noqueueclear );
		return false;
	}
	else if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() ) 
	{
		SiegePos siegepos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
		CommandDrop( object, siegepos, noqueueclear );
		return true;
	}
	return false;	
}



// This explicitly handles the use command
bool UICommands::SelectionCommandUse( bool noqueueclear )
{
	if ( gSiegeEngine.GetMouseShadow().IsHit() )	{
		CommandUse( MakeGoid(gSiegeEngine.GetMouseShadow().GetHit()), noqueueclear );
		return true;
	}
	return false;	
}


// This explicitly handles the stop command
bool UICommands::SelectionCommandStop()
{
	if( gWorldState.GetCurrentState() == WS_MP_INGAME_JIP )
	{
		return true;
	}

	CommandStop();
	return true;
}



// If the user clicks on terrain, figure out appropriate action.
void UICommands::ContextActionOnTerrain( const SiegePos & siegepos, bool noqueueclear )
{	
	GoidColl selectedparty;	
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	// If there are characters selected, move them
	if ( !selectedparty.empty() ) 
	{		
		CommandMove( siegepos, false, noqueueclear );
	}
}


bool UICommands::ContextCastOnTerrain( const SiegePos & siegepos, bool noqueueclear )
{
	GoidColl selectedparty;
	GoidColl::iterator i;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	if ( selectedparty.size() != 1 )
	{
		return false;
	}

	bool bHandled = false;
	for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
	{
		GoHandle hMember( *i );
		if ( hMember.IsValid() )
		{
			if ( hMember->GetInventory()->GetSelectedSpell() && 
				 (hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||  
				 hMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) &&		
				 hMember->GetInventory()->GetSelectedSpell()->GetMagic()->GetTargetTypeFlags() & TT_TERRAIN )
			{		
				CommandCast( siegepos, noqueueclear );			
				bHandled = true;
			}
		}
	}

	return bHandled;
}


// If the user clicks on a game object, figure out appropriate action.
bool UICommands::ContextActionOnGameObject( Goid object, bool noqueueclear, ActionSource action_source )
{
	// Make selected Attack, Follow, Join Formation, Guard, Use, Equip
	GoHandle hObject( object );
	if( !hObject.IsValid() )
	{
		return false;
	}

	// $$$ How are we going to filter out things you can't reach, etc?  Or should we? - cq
	/*
	if ( !hObject->GetAspect()->GetIsScreenPlayerVisible() )
	{
		return;
	}
	*/

	GoidColl selectedparty;	
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	GoHandle hMember;	
	if( !selectedparty.empty() ) 
	{
		hMember = selectedparty.front();
	}	

	if (( action_source != SOURCE_INVENTORY ) && hObject->HasGui() ) 
	{
		GetItemFromGO( hObject, false );
	}

	Goid actor = gUIGame.GetActorWhoCarriesObject( object );

	// Use Action	
	if ( hObject->IsUsable() || (hObject->IsGhostUsable() && gUIPartyManager.DoesPartyHaveGhost( selectedparty )) ) 
	{	
		// If it is an item that is not movable, use it.		
		CommandUse( hObject->GetGoid(), noqueueclear );		
	}	
	
	// Give-Get Action
	else if ( actor != GOID_INVALID && hObject->HasGui() )
	{
		gGoDb.Deselect( hObject->GetGoid() );		
	}
	
	// Get Action
	else if( (hObject->HasGui() && !hObject->HasActor()) || 
			 (hObject->HasGui() && hObject->HasActor() && hObject->GetActor()->GetCanBePickedUp() ) )
	{		
		CommandGet( hObject->GetGoid(), noqueueclear, action_source );
	}

	// Break/Damage action
	else if( !hObject->HasActor() && hObject->IsBreakable() && IsAlive( hObject->GetAspect()->GetLifeState() ) )
	{
		CommandAttack( hObject->GetGoid(), noqueueclear, true );
	}

	// Use Action
	else if( hObject->IsContainer() && !hObject->IsBreakable() ) 
	{
		CommandUse( hObject->GetGoid(), noqueueclear );
	}
	
	else if ( hObject->HasActor() && hMember->GetMind()->IsFriend( hObject ) && hObject->GetPlayer()->IsComputerPlayer() && ( hObject->GetParent() != hMember->GetParent() ) &&
			  hObject->HasInventory() && IsAlive( hObject->GetAspect()->GetLifeState() ) )
	{		
		gpstring sDisband;
		gpstring sDisbanded;		
		gpstring sRejoin;
		sDisband.assignf( "party_disband_0x%x", hObject->GetScid() );
		sDisbanded.assignf( "party_disbanded_0x%x", hObject->GetScid() );
		sRejoin.assignf( "party_allow_rejoin_0x%x", hObject->GetScid() );

		if ( gGameAuditor.GetBool( sDisband.c_str() ) || 
			 gGameAuditor.GetBool( sDisbanded.c_str() ) ||
			 gGameAuditor.GetBool( sRejoin.c_str() ) )
		{
			if ( !hObject->HasConversation() || !hObject->GetConversation()->GetCanTalk() )
			{
				gUIGame.ShowDialog( DIALOG_PACK_REHIRE );
				gUIPartyManager.SetRehireObject( hObject->GetGoid() );
			}
			else
			{
				CommandTalk( hObject, noqueueclear );
			}
		}	
		else
		{
			gpstring sMember;
			sMember.assignf( "was_party_member_0x%x", hObject->GetGoid() );
			if ( hObject->HasConversation() && hObject->GetConversation()->GetCanTalk() && !gGameAuditor.GetBool( sMember ) )
			{
				CommandTalk( hObject, noqueueclear );
			}
			else if ( gGameAuditor.GetBool( sMember ) )
			{
				gUIGame.ShowDialog( DIALOG_PACK_REHIRE );
				gUIPartyManager.SetRehireObject( hObject->GetGoid() );
			}						
		}		
	}

	else if ( hObject->HasActor() && hObject->HasConversation() && hObject->GetPlayer()->IsComputerPlayer() && hObject->GetConversation()->GetCanTalk() && hMember->GetMind()->IsFriend( hObject ) && ( hObject->GetParent() != hMember->GetParent() ) &&
		 hMember->HasInventory() && IsAlive( hObject->GetAspect()->GetLifeState() ) )
	{		
		CommandTalk( hObject, noqueueclear );								
	}		

	else
	{		
		GoidColl::iterator i;
		bool bHandled = false;

		m_bAttackSoundPlayed = false;
		for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
		{
			GoHandle hMember( *i );
			if ( hMember.IsValid() )
			{
				bool bResult = ContextActionOnGameObject( hMember, hObject, noqueueclear );
				if ( !bHandled )
				{
					bHandled = bResult;
				}
			}
		}
		m_bAttackSoundPlayed = false;

		if ( !bHandled )
		{
			// guard	
			if( ::IsMultiPlayer() && hObject->HasActor() && hMember->GetMind()->IsFriend( hObject ) && !hObject->GetPlayer()->IsComputerPlayer() && ( hObject->GetParent() != hMember->GetParent() ) &&
				hObject->HasInventory() && IsAlive( hObject->GetAspect()->GetLifeState() ) )	
			{	
				CommandGuard( hObject->GetGoid(), noqueueclear );
				bHandled = true;
			}

			if ( !gUIPartyManager.IsPlayerPartyMember( hObject->GetGoid() ) && gUIGame.IsSelectionDown() )
			{
				ContextActionOnTerrain( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), noqueueclear );
				bHandled = true;
			}

			if ( !bHandled )
			{
				return false;
			}
		}
	}

	return true;
}


bool UICommands::ContextActionOnGameObject( Go * pMember, Go * pObject, bool noqueueclear )
{
	// Cast Action
	if ( pMember->GetInventory()->GetSelectedSpell() && 
		 (pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||  
		 pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) &&		
		 (pMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( pObject ) ||
		  (pObject->IsScreenPartyMember())) )
	{		
		if ( !pMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( pObject ) )
		{
			pMember->PlayVoiceSound( "order_cant_move", false );
			
			if ( pMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( pObject, false ) )
			{
				gpscreenf( ( $MSG$ "%S does not have enough mana.\n", pMember->GetCommon()->GetScreenName().c_str() ) );				
			}

			return false;	
		}
		
		CommandCast( pMember, pObject, noqueueclear );			
		return true;
	}

	// Attack Action
	else if( (pMember->GetMind()->IsEnemy( pObject ) || (!pObject->HasActor() && pObject->IsBreakable())) && IsAlive( pObject->GetAspect()->GetLifeState() ) && pMember->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
	{
		CommandAttack( pMember, pObject, noqueueclear );
		return true;
	}	
	
	return false;
}


// Depending on what kind of game object the player has
// underneath their cursor, the appropriate cursor will 
// be displayed.
void UICommands::ContextCursorAction( bool bUseShadow )
{
	gUIPartyManager.SetRolloverCast( false );
	gUIPartyManager.SetRolloverGet( false );
	gUIPartyManager.SetRolloverAttack( false );
	
	eGuiContextCursor active = CURSOR_GUI_STANDARD;
	m_pCursors[CURSOR_GUI_STANDARD]->Activate();
	Goid object = gUIGame.GetGoUnderCursor();

	gUIGame.SetSelectionMode( SELECTION_MOVE );	

	GoidColl selectedparty;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	GoHandle hMember;	
	if( selectedparty.empty() ) 
	{
		return;
	}	
	hMember = selectedparty.front();

	GoHandle hObject( object );

	if ( hObject.IsValid() == false || !bUseShadow )
	{
		GoHandle hTest( gUIInventoryManager.GetGUIRolloverGOID() );
		if ( hTest.IsValid() && (hTest->IsScreenPartyMember() || hTest->IsTeamMember( hMember->GetGoid() )) )
		{
			hObject = hTest->GetGoid();
		}		
	}

	if ( hObject.IsValid() == false || hMember.IsValid() == false || gUIMenuManager.IsOptionsMenuActive() || gUIMenuManager.GetModalActive() ) 
	{			
		bool bWalkable = true;
		if ( gUIShell.IsMouseSettled( 2 ) && gSiegeEngine.GetMouseShadow().IsTerrainHit() )
		{	
			if ( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node.IsValid() )
			{
				bWalkable = gAIQuery.IsAreaWalkable( siege::LF_HUMAN_PLAYER, gSiegeEngine.GetMouseShadow().GetFloorHitPos(), 5, 0.15f );
			}
		}

		if ( (!gSiegeEngine.GetMouseShadow().IsTerrainHit() || !bWalkable )
			 && !gUIMenuManager.IsOptionsMenuActive() )
		{
			m_pCursors[CURSOR_GUI_CANT]->Activate();
			active = CURSOR_GUI_CANT;
		}
		else
		{
			m_pCursors[CURSOR_GUI_STANDARD]->Activate();		
		}
		
		for ( int i = 0; i != (int)CURSOR_GUI_SIZE; ++i )
		{
			if (( m_pCursors[(eGuiContextCursor)i] ) && ( ((eGuiContextCursor)i) != active ))
			{
				m_pCursors[(eGuiContextCursor)i]->SetVisible( false );
			}			
		}

		m_ActiveCursor = CURSOR_GUI_STANDARD;

		return;
	}	
	else if ( hMember->GetInventory()->IsPackOnly() || ( !IsAlive( hMember->GetAspect()->GetLifeState() ) || hMember->GetLifeState() == LS_ALIVE_UNCONSCIOUS ) )
	{
		bool bHuman = false;
		bool bGhost = false;
		GoidColl::iterator iMember;
		for ( iMember = selectedparty.begin(); iMember != selectedparty.end(); ++iMember )
		{
			GoHandle hPartyMember( (*iMember) );
			if ( !hPartyMember->GetInventory()->IsPackOnly() && IsAlive( hPartyMember->GetAspect()->GetLifeState() ) &&
				 hPartyMember->GetLifeState() != LS_ALIVE_UNCONSCIOUS )
			{
				bHuman = true;
			}

			if ( hPartyMember->GetLifeState() == LS_GHOST )
			{
				bGhost = true;
			}
		}

		if ( bGhost )
		{
			if ( hObject->IsGhostUsable() )
			{
				active = CURSOR_GUI_GRAB;
				for ( int i = 0; i != (int)CURSOR_GUI_SIZE; ++i )
				{
					if (( m_pCursors[(eGuiContextCursor)i] ) && ( ((eGuiContextCursor)i) != active ))
					{
						m_pCursors[(eGuiContextCursor)i]->SetVisible( false );
					}			
				}
				m_pCursors[CURSOR_GUI_GRAB]->Activate();
				m_ActiveCursor = CURSOR_GUI_GRAB;
				return;
			}
		}
		
		if ( !bHuman )
		{	
			if ( gUIShell.GetItemActive() && hObject->IsScreenPartyMember() &&
				 hObject->HasInventory() && hObject->GetInventory()->IsPackOnly() )
			{
				UIItemVec items;
				gUIShell.FindActiveItems( items );
				if ( items.size() != 0 )
				{
					if ( gUIGame.GetActorWhoCarriesObject( MakeGoid( (*(items.begin()))->GetItemID() ) ) == GOID_INVALID )
					{
						m_pCursors[CURSOR_GUI_CANT]->Activate();
						active = CURSOR_GUI_CANT;						
					}
				}
			}
			else if ( hObject->HasGui() ) 
			{		
				m_pCursors[CURSOR_GUI_GRAB]->Activate();
				active = CURSOR_GUI_GRAB;
			}
			else 
			{			
				m_pCursors[CURSOR_GUI_STANDARD]->Activate();
				active = CURSOR_GUI_STANDARD;
			}

			for ( int i = 0; i != (int)CURSOR_GUI_SIZE; ++i )
			{
				if (( m_pCursors[(eGuiContextCursor)i] ) && ( ((eGuiContextCursor)i) != active ))
				{
					m_pCursors[(eGuiContextCursor)i]->SetVisible( false );
				}			
			}

			m_ActiveCursor = active;
			return;
		}
	}
	
	Goid actor = gUIGame.GetActorWhoCarriesObject( object );

	if ( hObject->IsUsable() ) 
	{			
		m_pCursors[CURSOR_GUI_GRAB]->Activate();
		active = CURSOR_GUI_GRAB;
	}
	else if ( hObject->IsBreakable() && !hObject->HasActor() ) 
	{			
		m_pCursors[CURSOR_GUI_SMASH]->Activate();
		active = CURSOR_GUI_SMASH;
		
		bool bCanBreak = false;
		GoidColl::iterator iBreaker;
		for ( iBreaker = selectedparty.begin(); iBreaker != selectedparty.end(); ++iBreaker )
		{
			GoHandle hBreakMember( *iBreaker );
			if( hBreakMember.IsValid() && ( hBreakMember->GetGoid() != hObject->GetGoid() ) ) 
			{			
				Go *pWeapon = hBreakMember->GetInventory()->GetItem( hBreakMember->GetInventory()->GetSelectedLocation() );
				if( (pWeapon && pWeapon->HasAttack()) ) 
				{
					const float damage_radius = pWeapon->GetAttack()->GetAreaDamageRadius();
					if( damage_radius > 0 ) 
					{
						gUIGame.DrawAreaOfEffectVolume( pWeapon->GetGoid(), hObject->GetGoid() );
					}

					bCanBreak = true;
				}	
				if ( !pWeapon )
				{
					bCanBreak = true;
				}
			}
		}

		if ( !bCanBreak )
		{
			m_pCursors[CURSOR_GUI_CANT]->Activate();
			active = CURSOR_GUI_CANT;
		}
	}	
	// Give-Get Action
	else if ( actor != GOID_INVALID && hObject->HasGui() ) 
	{		
		m_pCursors[CURSOR_GUI_GRAB]->Activate();
		active = CURSOR_GUI_GRAB;
	}

	else if ( hObject->HasGui() )
	{			
		m_pCursors[CURSOR_GUI_GRAB]->Activate();
		active = CURSOR_GUI_GRAB;
		gUIPartyManager.SetRolloverGet( true );
	}

	// Talk Action
	else if ( hMember.IsValid() && hObject->HasActor() && hObject->HasConversation() && hObject->GetConversation()->GetCanTalk() && hMember->GetMind()->IsFriend( hObject ) && 
		( hObject->GetParent() != hMember->GetParent() ) && hMember->HasInventory() && hObject->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
	{
		GopColl sel = gGoDb.GetSelection();
		GopColl::const_iterator i;
		for ( i = sel.begin(); i != sel.end(); ++i )
		{
			int index = 0;
			if ( gUIPartyManager.IsPlayerPartyMember( (*i)->GetGoid(), index ) && 
				(*i)->HasInventory() && !(*i)->GetInventory()->IsPackOnly() && IsAlive( hObject->GetAspect()->GetLifeState() ) )
			{
				m_pCursors[CURSOR_GUI_TALK]->Activate();
				active = CURSOR_GUI_TALK;
				break;
			}
		}
	}
	else if ( hMember.IsValid() && hObject->HasActor() && hObject->GetPlayer()->IsComputerPlayer() && hMember->GetMind()->IsFriend( hObject ) && ( hObject->GetParent() != hMember->GetParent() ) &&
			  hObject->HasInventory() && !hObject->HasConversation() && hObject->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
	{
		gpstring sMember;
		sMember.assignf( "was_party_member_0x%x", hObject->GetGoid() );
		if ( gGameAuditor.GetBool( sMember ) )
		{		
			if ( hObject->GetInventory()->IsPackOnly() )
			{
				m_pCursors[CURSOR_GUI_INITIATE]->Activate();
				active = CURSOR_GUI_INITIATE;			
			}
			else
			{
				m_pCursors[CURSOR_GUI_TALK]->Activate();
				active = CURSOR_GUI_TALK;							
			}
		}	
	}
	else
	{
		GoidColl::iterator i;		
		bool bResolved = false;

		for ( i = selectedparty.begin(); i != selectedparty.end(); ++i )
		{
			GoHandle hMember( *i );
			if ( hMember.IsValid() )
			{
				bool bSuccess = ContextCursorAction( hMember, hObject, active );
				if ( !bResolved )
				{
					bResolved = bSuccess;
				}
			}
		}

		if ( !bResolved )
		{
			m_pCursors[CURSOR_GUI_STANDARD]->Activate();
					
			if ( gUIShell.GetItemActive() && hObject->IsScreenPartyMember() &&
			     hObject->HasInventory() && hObject->GetInventory()->IsPackOnly() )
			{
				UIItemVec items;
				gUIShell.FindActiveItems( items );
				if ( items.size() != 0 )
				{
					if ( gUIGame.GetActorWhoCarriesObject( MakeGoid( (*(items.begin()))->GetItemID() ) ) == GOID_INVALID )
					{
						m_pCursors[CURSOR_GUI_CANT]->Activate();
						active = CURSOR_GUI_CANT;
					}
				}
			}
			else if ( !gSiegeEngine.GetMouseShadow().IsTerrainHit() && (gUIInventoryManager.GetGUIRolloverGOID() == GOID_INVALID) )
			{
				m_pCursors[CURSOR_GUI_CANT]->Activate();
				active = CURSOR_GUI_CANT;
			}
		}
	}			

	for ( int i = 0; i != (int)CURSOR_GUI_SIZE; ++i )
	{
		if (( m_pCursors[(eGuiContextCursor)i] ) && ( ((eGuiContextCursor)i) != active ))
		{
			m_pCursors[(eGuiContextCursor)i]->SetVisible( false );
		}			
	}	

	m_ActiveCursor = active;
}


bool UICommands::ContextCursorAction( Go * pMember, Go * pObject, eGuiContextCursor & active )
{
	GoidColl selectedparty;
	gUIPartyManager.GetSelectedPartyMembers( selectedparty );

	// Cast Action
	if( pMember && pMember->GetInventory()->GetSelectedSpell() && 
		pMember->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS && 
		(pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||  
		pMember->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL) )
	{			
		if ( pMember->GetInventory()->GetSelectedSpell()->GetMagic()->IsCastableOn( pObject, false ) )
		{
			if ( active == CURSOR_GUI_ATTACK )
			{
				m_pCursors[CURSOR_GUI_ATTACK_CAST]->Activate();
				active = CURSOR_GUI_ATTACK_CAST;				
			}
			else if ( active != CURSOR_GUI_ATTACK_CAST )
			{
				m_pCursors[CURSOR_GUI_CAST]->Activate();
				active = CURSOR_GUI_CAST;				
			}

			gUIPartyManager.SetRolloverCast( true );
			gUIPartyManager.SetRolloverCastObject( pObject->GetGoid() );
			gUIPartyManager.SetRolloverCastMember( pMember->GetGoid() );
			gUIPartyManager.SetRolloverCastSpell( pMember->GetInventory()->GetSelectedSpell()->GetGoid() );

			if( pMember && ( pMember->GetGoid() != pObject->GetGoid() ) ) 
			{			
				Go *pWeapon = pMember->GetInventory()->GetItem( pMember->GetInventory()->GetSelectedLocation() );
				if( pWeapon && pWeapon->HasAttack() ) 
				{
					const float damage_radius = pWeapon->GetAttack()->GetAreaDamageRadius();
					if( damage_radius > 0 ) 
					{
						gUIGame.DrawAreaOfEffectVolume( pWeapon->GetGoid(), pObject->GetGoid() );
					}
				}
			}
		}
	
		return true;
	}

	// Attack Action
	else if ( pMember->GetMind()->IsEnemy( pObject ) && IsAlive( pObject->GetLifeState() ) &&
			  pMember->GetAspect()->GetLifeState() == LS_ALIVE_CONSCIOUS )
	{	
		if ( active == CURSOR_GUI_CAST )
		{
			m_pCursors[CURSOR_GUI_ATTACK_CAST]->Activate();
			active = CURSOR_GUI_ATTACK_CAST;
		}
		else if ( active != CURSOR_GUI_ATTACK_CAST )
		{
			m_pCursors[CURSOR_GUI_ATTACK]->Activate();
			active = CURSOR_GUI_ATTACK;		
		}	

		if( pMember && ( pMember->GetGoid() != pObject->GetGoid() ) ) 
		{			
			Go * pWeapon = pMember->GetInventory()->GetItem( pMember->GetInventory()->GetSelectedLocation() );

			if( pWeapon && pWeapon->HasAttack() ) 
			{
				const float damage_radius = pWeapon->GetAttack()->GetAreaDamageRadius();
				if( damage_radius > 0 ) 
				{
					gUIGame.DrawAreaOfEffectVolume( pWeapon->GetGoid(), pObject->GetGoid() );
				}
			}
		}

		gUIPartyManager.SetRolloverAttack( true );

		gUIGame.SetSelectionMode( SELECTION_ATTACK );
		return true;
	}

	return false;
}


void UICommands::InitCursors()
{
	m_pCursors[CURSOR_GUI_STANDARD] = gUIShell.LoadUICursor( "cursor_pointer", "cursors" );
	m_pCursors[CURSOR_GUI_GRAB]		= gUIShell.LoadUICursor( "cursor_grab", "cursors" );
	m_pCursors[CURSOR_GUI_SMASH]	= gUIShell.LoadUICursor( "cursor_smash", "cursors" );
	m_pCursors[CURSOR_GUI_CAST]		= gUIShell.LoadUICursor( "cursor_cast", "cursors" );
	m_pCursors[CURSOR_GUI_ATTACK]	= gUIShell.LoadUICursor( "cursor_attack", "cursors" );
	m_pCursors[CURSOR_GUI_TALK]		= gUIShell.LoadUICursor( "cursor_talk", "cursors" );
	m_pCursors[CURSOR_GUI_INITIATE] = gUIShell.LoadUICursor( "cursor_initiate", "cursors" );
	m_pCursors[CURSOR_GUI_GUARD]	= gUIShell.LoadUICursor( "cursor_guard", "cursors" );
	m_pCursors[CURSOR_GUI_CANT]		= gUIShell.LoadUICursor( "cursor_cant", "cursors" );	
	m_pCursors[CURSOR_GUI_ATTACK_CAST] = gUIShell.LoadUICursor( "cursor_cast_attack", "cursors" );
	m_pCursors[CURSOR_GUI_WAIT]		= gUIShell.LoadUICursor( "cursor_wait", "cursors" );	
}


void UICommands::SetCursor( eGuiContextCursor cursor )
{
	for ( int i = 0; i != (int)CURSOR_GUI_SIZE; ++i )
	{
		if ( m_pCursors[(eGuiContextCursor)i] )
		{
			m_pCursors[(eGuiContextCursor)i]->SetVisible( false );
		}		
		else
		{
			return;
		}
	}

	if ( m_pCursors[cursor] )
	{
		m_pCursors[cursor]->Activate();
	}
}