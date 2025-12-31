//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoInventory.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoInventory.h"

#include "AiQuery.h"
#include "ContentDb.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "GuiHelper.h"
#include "NamingKey.h"
#include "NeMa_Aspect.h"
#include "NeMa_AspectMgr.h"
#include "NeMa_Blender.h"
#include "GameAuditor.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoDefend.h"
#include "GoMagic.h"
#include "GoShmo.h"
#include "GoStore.h"
#include "GoMind.h"
#include "GoSupport.h"
#include "Job.h"
#include "PContentDb.h"
#include "ReportSys.h"
#include "Ui_Infoslot.h"
#include "Ui_Shell.h"
#include "Sim.h"
#include "World.h"
#include "WorldState.h"
#include "Victory.h"

DECLARE_GO_COMPONENT( GoInventory, "inventory" );

//////////////////////////////////////////////////////////////////////////////
// class GoInventory implementation

GoInventory :: GoInventory( Go* parent )
	: Inherited( parent )
{
	m_GridWidth                = 0;
	m_GridHeight               = 0;
	m_IsPackOnly               = false;
	m_SelectedActiveLocation   = IL_ACTIVE_MELEE_WEAPON;
	m_Gold                     = 0;
	m_SelectedItem             = NULL;
	m_SelectedItemActionOrigin = AO_INVALID;
	m_CurrentStance            = AS_PLAIN;
	m_GridBox                  = NULL;
	m_InventoryDirty           = true;
	m_GoldDirty                = false;
	m_ActiveSpellBook          = NULL;
	m_bForceGet				   = false;
	m_IgnoreEquipMinReqs       = false;

	m_CustomHeadName           = "";
	m_CustomHeadAspect         = nema::HAspect();

	::ZeroObject( m_Equipped );
}


GoInventory :: GoInventory( const GoInventory& source, Go* parent )
	: Inherited( source, parent )
	, m_RangeColl( source.m_RangeColl )
	, m_GoldPiles( source.m_GoldPiles )
{
	m_GridWidth                = source.m_GridWidth;
	m_GridHeight               = source.m_GridHeight;
	m_IsPackOnly               = source.m_IsPackOnly;
	m_SelectedActiveLocation   = source.m_SelectedActiveLocation;
	m_Gold                     = source.m_Gold;
	m_SelectedItem             = source.m_SelectedItem;
	m_SelectedItemActionOrigin = source.m_SelectedItemActionOrigin;
	m_CurrentStance            = source.m_CurrentStance;
	m_GridBox                  = NULL;
	m_InventoryDirty           = true;
	m_GoldDirty                = false;
	m_ActiveSpellBook          = NULL;
	m_bForceGet				   = source.m_bForceGet;
	m_IgnoreEquipMinReqs       = false;

	m_CustomHeadName           = source.m_CustomHeadName;

	::ZeroObject( m_Equipped );
}


GoInventory :: ~GoInventory( void )
{
	// inventory should already be empty
	gpassert( m_InventoryDb.empty() );

	// Delete it's personal gridbox as long as it is not a stash (because the stash gridbox is not dynamically created)
	if ( m_GridBox != NULL && !GetGo()->HasStash() )
	{
		gUIShell.DeleteWindow( GetGridbox() );
	}
}


Go* GoInventory :: GetEquipped( eEquipSlot slot ) const
{
	gpassert( (slot >= 0) && (slot < ELEMENT_COUNT( m_Equipped )) );
	return ( m_Equipped[ slot ] );
}


eAnimStance GoInventory :: GetAnimStance( void )
{
	if ( IsInventoryDirty() )
	{
		UpdateAnimStance();
	}

	return ( m_CurrentStance );
}



eEquipSlot GoInventory :: GetEquippedSlot( Goid item ) const
{
	eEquipSlot slot = ES_NONE;

	GoHandle go( item );
	if ( go )
	{
		slot = GetEquippedSlot( go );
	}

	return ( slot );
}


eEquipSlot GoInventory :: GetEquippedSlot( const Go* item ) const
{
	gpassert( item != NULL );

	eEquipSlot slot = ES_NONE;

	const CGo* found = std::find( m_Equipped, ARRAY_END( m_Equipped ), item );
	if ( found != ARRAY_END( m_Equipped ) )
	{
		slot = (eEquipSlot)( found - m_Equipped );
	}

	return ( slot );
}


bool GoInventory :: IsActiveSpell( const Go* item ) const
{
	return (   (GetItem( IL_ACTIVE_PRIMARY_SPELL   ) == item)
		    || (GetItem( IL_ACTIVE_SECONDARY_SPELL ) == item) );
}


bool GoInventory :: IsMeleeWeaponEquipped() const
{
	Go * weapon = GetEquipped( ES_WEAPON_HAND );
	Go * shield = GetEquipped( ES_SHIELD_HAND );

	if( weapon && weapon->IsWeapon() && weapon->IsMeleeWeapon() )
	{
		return true;
	}

	if( shield && shield->IsWeapon() && shield->IsMeleeWeapon() )
	{
		return true;
	}

	return false;
}


bool GoInventory :: IsRangedWeaponEquipped() const
{
	Go * weapon = GetEquipped( ES_WEAPON_HAND );
	Go * shield = GetEquipped( ES_SHIELD_HAND );

	if( weapon && weapon->IsRangedWeapon() )
	{
		return true;
	}

	if( shield && shield->IsRangedWeapon() )
	{
		return true;
	}

	return false;
}


bool GoInventory :: Contains( const Go* item ) const
{
	return ( m_InventoryDb.find( ccast <Go*> ( item ) ) != m_InventoryDb.end() );
}


eInventoryLocation GoInventory :: GetLocation( const Go* item ) const
{
	eInventoryLocation il = IL_INVALID;

	InventoryDb::const_iterator found = m_InventoryDb.find( ccast <Go*> ( item ) );
	if ( found != m_InventoryDb.end() )
	{
		il = found->second;
	}

	return ( il );
}


bool GoInventory :: LocationContainsTemplate( eInventoryLocation il, const gpstring & sTemplate )
{
	GopColl items;
	GopColl::iterator i;
	ListItems( il, items );
	for ( i = items.begin(); i != items.end(); ++i )
	{
		if ( sTemplate.same_no_case( (*i)->GetTemplateName() ) )
		{
			return true;
		}
	}

	return false;
}


Go* GoInventory :: GetItem( eInventoryLocation il ) const
{
	Go* item = NULL;

	if ( IsSingularSlot( il ) )
	{
		if ( il == IL_ACTIVE_PRIMARY_SPELL || il == IL_ACTIVE_SECONDARY_SPELL )
		{
			Go * pSpellbook = GetEquipped( ES_SPELLBOOK );
			if ( pSpellbook )
			{
				if ( pSpellbook->HasInventory() )
				{
					if ( il == IL_ACTIVE_PRIMARY_SPELL )
					{
						item = pSpellbook->GetInventory()->GetItem( IL_SPELL_1 );
					}
					else
					{
						item = pSpellbook->GetInventory()->GetItem( IL_SPELL_2 );
					}
				}
				else
				{
					gperrorf(( "GoInventory::GetItem - item '%s' Goid 0x%08X equipped in ES_SPELLBOOK does not have inventory!!!!\n",
							   pSpellbook->GetTemplateName(), pSpellbook->GetGoid() ));
				}
			}
		}
		else
		{
			InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( i->second == il )
				{
					item = i->first;
					break;
				}
			}
		}
	}
	else
	{
		gperrorf(( "GoInventory::GetItem - nonspecific inventory location '%s' not supported in query for Go 0x%08X.\n",
				   ::ToString( il ), GetGoid() ));
	}

	return ( item );
}

Go* GoInventory :: GetItemFromTemplate( const gpstring & sTemplate ) const
{
	Go* item = NULL;

	InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( sTemplate.same_no_case( i->first->GetTemplateName() ) )
		{
			item = i->first;
			break;
		}
	}

	return ( item );
}

const gpstring& GoInventory :: GetSpewEquippedKillCount ( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "spew_equipped_kill_count" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}


GoInventory::eSpewType GoInventory :: GetEquippedItemsSpewType( PlayerId forPlayerId ) const
{
	bool should = false;

	// quick check - ignore spew?
	const gpstring& spewCount = GetSpewEquippedKillCount();
	if ( spewCount.same_no_case( "<ignore>" ) )
	{
		return ( SPEW_IGNORE );
	}

	// first get my kill count
	int killCount = 0;
	gGameAuditor.Get( GE_PLAYER_KILL, MakeInt( forPlayerId ), GetGo()->GetTemplateName(), killCount );

	// see if it's there
	if ( killCount != 0 )
	{
		int killEntry;
		bool success = true;
		stringtool::Extractor extractor( ",", spewCount );
		while ( extractor.GetNextInt( killEntry, &success ) )
		{
			if ( killEntry == killCount )
			{
				should = true;
				break;
			}
		}

		if ( !success )
		{
			gperrorf(( "Illegal spew equipped items count for template '%s' scid 0x%08X: '%s'\n",
					   GetGo()->GetTemplateName(), GetScid(), spewCount.c_str() ));
		}
	}

	// done
	return ( should ? SPEW_GROUND : SPEW_DELETE );
}

bool GoInventory :: ListItems( eInventoryLocation il, GopColl& items ) const
{
	return ( QueryItems( il, &items ) > 0 );
}


bool GoInventory :: ListItems( eQueryTrait qt, eInventoryLocation il, GopColl& items ) const
{
	QtColl qtc;
	qtc.push_back( qt );
	return( ListItems( qtc, il, items ) );
}


bool GoInventory :: ListItems( QtColl & qtc, eInventoryLocation il, GopColl& items ) const
{
	bool hasAny = false;

	GopColl temp;
	if ( ListItems( il, temp ) )
	{
		hasAny = gAIQuery.Get( GetGo(), qtc, temp, items );
	}

	return ( hasAny );
}

bool GoInventory :: GetDropAtUsePoint( void ) const
{
	static AutoConstQuery <bool> s_Query( "drop_at_use_point" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

FuBiCookie GoInventory :: RSEquip( eEquipSlot slot, Goid item, eActionOrigin origin )
{
	FUBI_RPC_THIS_CALL_RETRY( RSEquip, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	return ( RCEquip( slot, item, origin ) );
}


FuBiCookie GoInventory :: RCEquip( eEquipSlot slot, Goid item, eActionOrigin origin )
{
	FUBI_RPC_THIS_CALL_RETRY( RCEquip, RPC_TO_ALL );

	return ( Equip( slot, item, origin ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}


bool GoInventory :: Equip( eEquipSlot slot, Goid item, eActionOrigin origin )
{
	LOADER_OR_RPC_ONLY;
	gpassert( AssertValid() );
	gpassert( !m_IsPackOnly );
	gpassert( item != GOID_INVALID );

	// special support for rings
	if ( (item != GOID_NONE) && (slot == ES_RING) )
	{
		if ( !IsSlotEquipped( ES_RING_0 ) )
		{
			slot = ES_RING_0;
		}
		else if ( !IsSlotEquipped( ES_RING_1 ) )
		{
			slot = ES_RING_1;
		}
		else if ( !IsSlotEquipped( ES_RING_2 ) )
		{
			slot = ES_RING_2;
		}
		else if ( !IsSlotEquipped( ES_RING_3 ) )
		{
			slot = ES_RING_3;
		}
	}

	// check sanity
	if ( !((slot >= 0) && (slot < ELEMENT_COUNT( m_Equipped ))) )
	{
		gperror( "Error: attempted to equip a nonspecific equip slot\n" );
		return ( false );
	}

// Check to see if nothing will change.

	// this is an early bailout
	GoHandle itemGo( item );
	if ( m_Equipped[ slot ] == itemGo )
	{
		// cool
		return ( true );
	}

	// not allowed to equip? only test on server
	if ( IsServerLocal() && itemGo && !TestEquipMinReqs( itemGo ) )
	{
#		if !GP_RETAIL
		gperrorf((
				"Error: can't equip this item due to minimum skill requirements\n"
				"\n"
				"Inventory owner:\n"
					"\tTemplate = '%s'\n"
					"\tScid     = 0x%08X\n"
					"\tGoid     = 0x%08X\n"
				"Inventory item:\n"
					"\tTemplate = '%s'\n"
					"\tScid     = 0x%08X\n"
					"\tGoid     = 0x%08X\n"
					"\tRequired = %s\n",
				GetGo()->GetTemplateName(),
				GetGo()->GetScid(),
				GetGoid(),
				itemGo->GetTemplateName(),
				itemGo->GetScid(),
				itemGo->GetGoid(),
				itemGo->GetGui()->GetEquipRequirements().c_str() ));
#		endif // !GP_RETAIL

		return ( false );
	}

// Unequip old slot member.

	if ( m_Equipped[ slot ] != NULL )
	{
		// do it
		PrivateUnequip( slot );

		// get go
		Go* go = m_Equipped[ slot ];

		// clear cache
		ClearSelected( go );

		// always clear this out
		m_Equipped[ slot ] = NULL;


		// clear any running enchantments
		if( (go->IsArmor() && !go->IsShield()) || IsRingSlot( slot ) || (slot == ES_AMULET) || (slot == ES_SPELLBOOK) )
		{
			GoMagic* magic = go->QueryMagic();
			if ( magic )
			{
				gGoDb.RemoveEnchantments( GetGoid(), go->GetGoid(), true );
			}
		}

		// tell everyone
		WorldMessage( WE_UNEQUIPPED, GetGoid(), go->GetGoid() ).Send();
	}

// Equip new slot member.

	bool success = true;

	if ( item != GOID_NONE )
	{
		if ( itemGo && Contains( itemGo ) )
		{
			if ( PrivateEquip( slot, itemGo ) )
			{
				// take the slot
				m_Equipped[ slot ] = itemGo;

				// update inv location
				if ( (slot == ES_WEAPON_HAND) || (slot == ES_SHIELD_HAND) )
				{
					if ( itemGo->IsWeapon() || itemGo->IsPotion() )
					{
						if ( itemGo->IsMeleeWeapon() )
						{
							SetLocation( itemGo, IL_ACTIVE_MELEE_WEAPON );
							Select( IL_ACTIVE_MELEE_WEAPON, origin );
						}
						else if( itemGo->IsRangedWeapon() )
						{
							SetLocation( itemGo, IL_ACTIVE_RANGED_WEAPON );
							Select( IL_ACTIVE_RANGED_WEAPON, origin );
						}
					}
					else
					{
						gpassert( itemGo->IsShield() );
						SetLocation( itemGo, IL_SHIELD );
					}
				}
				else if( (itemGo->IsArmor() && !itemGo->IsShield()) || IsRingSlot( slot ) || (slot == ES_AMULET) || (slot == ES_SPELLBOOK) )
				{
					GoMagic* magic = itemGo->QueryMagic();
					if ( magic )
					{
						magic->ApplyEnchantments( GetGoid(), itemGo->GetGoid() );
					}

					if ( slot == ES_SPELLBOOK )
					{
						SetActiveSpellBook( itemGo );
					}
				}

				// tell everyone
				WorldMessage( WE_EQUIPPED, GetGoid(), itemGo->GetGoid() ).Send();
			}
			else
			{
				success = false;
				gpwarningf(( "Failed to equip item '%s' - missing bones or deformation info? Check the art!\n",
							 itemGo->GetAspect()->GetAspectHandle()->GetDebugName() ));
			}
		}
		else
		{
			success = false;
			gperror( "Attempted to equip an item not in inventory\n" );
		}
	}

// Finish.

	// always dirty the inventory
	SetInventoryDirty();

	// check for stance changed
	UpdateAnimStance();

	// update the scale of weapon/shield only AFTER we have updated the stance!!!
	if ( ( item != GOID_NONE ) && ((slot == ES_WEAPON_HAND) || (slot == ES_SHIELD_HAND)) )
	{
		eAnimStance stance = GetAnimStance();

		if ( stance == AS_SINGLE_MELEE_AND_SHIELD)
		{
			if (slot == ES_SHIELD_HAND)
			{
				stance = AS_SHIELD_ONLY;
			}
			else
			{
				stance = AS_SINGLE_MELEE;
			}
		}

		// query for scale
		FastFuelHandle scales = GetGo()->GetBody()->GetData()->GetInternalField( "weapon_scales" );
		gpstring scaleText;
		vector_3 scale;
		if ( scales && scales.Get( ToString( stance ), scaleText ) && ::FromString( scaleText, scale ) )
		{
			itemGo->GetAspect()->GetAspectHandle()->SetCurrentScale( scale );

			// Need to re-scale the tracepoints too!
			nema::Blender* blender = GetGo()->GetAspect()->GetAspectHandle()->GetBlender();
			if (blender)
			{
				vector_3 stp(DoNotInitialize);

				HadamardProduct(stp,blender->GetTracePoint0(),scale);
				blender->SetTracePoint0(stp);

				HadamardProduct(stp,blender->GetTracePoint1(),scale);
				blender->SetTracePoint1(stp);
			}
		}
	}

	// modifiers may have changed, recalc
	GetGo()->SetModifiersDirty();

	// all done
	return ( success );
}

bool GoInventory :: TestEquipMinReqs( Go* item ) const
{
	// early out for special case
	if ( m_IgnoreEquipMinReqs )
	{
		return ( true );
	}

	// early out for npc's
	if ( !GetGo()->IsAnyHumanPartyMember() )
	{
		return ( true );
	}

	if ( GetGo()->HasActor() && item->HasMagic() && item->GetMagic()->HasEnchantments() )
	{
		std::map< gpstring, float, istring_less > skillNameToValueMap;
		const SkillColl& skills = GetGo()->GetActor()->GetSkills();
		SkillColl::const_iterator iSkill;
		for ( iSkill = skills.begin(); iSkill != skills.end(); ++iSkill )
		{
			skillNameToValueMap.insert( std::make_pair( (*iSkill).GetName(), (*iSkill).GetLevel() ) );
		}

		StringVec negativeModifiers;			
		const EnchantmentVector& enchVec = item->GetMagic()->GetEnchantmentStorage().GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			gpstring sAlterSkill;
			const Enchantment* ench = *i;			
				
			switch ( ench->GetAlterationType() )
			{
				case ALTER_STRENGTH:
					sAlterSkill = "strength";						
					break;
				case ALTER_INTELLIGENCE:
					sAlterSkill = "intelligence";
					break;
				case ALTER_DEXTERITY:
					sAlterSkill = "dexterity";
					break;
				case ALTER_MELEE:
					sAlterSkill = "melee";
					break;
				case ALTER_RANGED:
					sAlterSkill = "ranged";
					break;
				case ALTER_NATURE_MAGIC:
					sAlterSkill = "nature magic";
					break;
				case ALTER_COMBAT_MAGIC:
					sAlterSkill = "combat magic";
					break;
			}

			if ( ench->GetValue() < 0 && !sAlterSkill.empty() )
			{
				negativeModifiers.push_back( sAlterSkill );
			}

			if ( !(!item->IsEquipped() && (GetGo()->GetInventory()->GetLocation( item ) == IL_MAIN || !Contains( item ))) )
			{
				continue;
			}		

			std::map< gpstring, float, istring_less >::iterator iFind = skillNameToValueMap.find( sAlterSkill );
			if ( iFind != skillNameToValueMap.end() )
			{
				(*iFind).second += ench->GetValue();
			}
		}			

		std::map< gpstring, float, istring_less >::iterator iValue;
		for ( iValue = skillNameToValueMap.begin(); iValue != skillNameToValueMap.end(); ++iValue )
		{
			if ( (*iValue).second < 0.0f )
			{
				return ( false );
			}
			else if ( (*iValue).second == 0.0f )
			{
				StringVec::iterator iNegMod;
				for ( iNegMod = negativeModifiers.begin(); iNegMod != negativeModifiers.end(); ++iNegMod )
				{
					if ( (*iNegMod).same_no_case( (*iValue).first ) )
					{
						float total = gGoDb.GetEnchantmentTotal( GetGo()->GetGoid(), (*iValue).first );
						if ( total < 0 )
						{
							return ( false );
						}
					}
				}
			}
		}
	}

	// do the test
	if ( (item != NULL) && item->HasGui() && GetGo()->HasActor() )
	{
		EquipRequirementColl requirements;
		item->GetGui()->GetEquipRequirements( requirements );
		if ( !requirements.CanEquip( GetGo() ) )
		{
			return ( false );
		}
	}

	return ( true );
}

bool GoInventory :: TestEquip( Goid item, eEquipSlot slot ) const
{
	gpassert( AssertValid() );

	GoHandle object( item );
	if ( slot == ES_ANY )
	{
		slot = object->GetGui()->GetEquipSlot();
	}

	if ( IsSlotEquipped( slot ) )
	{
		return ( TestGet( item ) );
	}

	return ( true );
}

bool GoInventory :: TestEquipPassive( Goid item ) const
{
	GoHandle object( item );
	if ( !object.IsValid() || IsPackOnly() )
	{
		return false;
	}

	// Not told to equip it in any particular place
	eEquipSlot slot = object->GetGui()->GetEquipSlot();

	bool bPlaceInActiveSlot = false;
	bool bCanEquip = TestEquipMinReqs( object );

	switch( slot )
	{
		case ES_WEAPON_HAND:

			if( GetEquipped( ES_WEAPON_HAND ) == NULL )
			{
					// If there is no active melee weapon ( or it is the same as the item we're about to equip ) and a spell is not our active weapon, it is okay to equip
				if(	((GetItem( IL_ACTIVE_MELEE_WEAPON ) == NULL || GetItem( IL_ACTIVE_MELEE_WEAPON )->GetGoid() == item) ) ||

					// If there is a spell selected and the weapon we want to equip is a staff, it is okay.  Other weapons are not allowed to be equipped in this case
					( IsSpellSelected() && object->GetAttack()->GetAttackClass() == AC_STAFF && GetItem( IL_ACTIVE_MELEE_WEAPON ) == NULL ) ||

					// If there is no active ranged weapon ( or it is the same as the item we're about to equip ) and a spell is not our active weapon, and it is a ranged weapon, it is also okay to equip
					((GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL || GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid() == item) && object->IsRangedWeapon() ) )
				{
					// Is there a shield item in the other hand?
					bPlaceInActiveSlot = true;
				}
			}
			else if ( object->IsRangedWeapon() && GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;

		case ES_SHIELD_HAND:

			if( GetEquipped( ES_SHIELD_HAND ) == NULL )
			{
				if ( (GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL || GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid() == item) && object->IsRangedWeapon() )
				{
					bPlaceInActiveSlot = true;
				}
				else if ( (GetItem( IL_SHIELD ) == NULL || GetItem( IL_SHIELD )->GetGoid() == item) && object->IsShield() )
				{
					bPlaceInActiveSlot = true;
				}
			}
			else if ( IsSpellSelected() && object->IsShield() )
			{
				if ( GetItem( IL_SHIELD ) == NULL )
				{
					bPlaceInActiveSlot = true;
				}
			}
			else if (( GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL && object->IsRangedWeapon() ) ||
					( GetItem( IL_SHIELD ) == NULL && object->IsShield() ))
			{
				bPlaceInActiveSlot = true;
			}
			break;

		case ES_CHEST:

			if( GetEquipped( ES_CHEST ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;

		case ES_HEAD:
			if( GetEquipped( ES_HEAD ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;

		case ES_FOREARMS:
			if( GetEquipped( ES_FOREARMS ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;

		case ES_FEET:
			if( GetEquipped( ES_FEET ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;
		case ES_AMULET:
			if( GetEquipped( ES_AMULET ) == NULL )
			{
				bPlaceInActiveSlot = true;
			}
			break;
		case ES_RING:
			if(( GetEquipped( ES_RING_0 ) == NULL ) ||
			   ( GetEquipped( ES_RING_1 ) == NULL ) ||
			   ( GetEquipped( ES_RING_2 ) == NULL ) ||
			   ( GetEquipped( ES_RING_3 ) == NULL ))
			{
				bPlaceInActiveSlot = true;
			}
			break;
	}

	if ( bPlaceInActiveSlot && bCanEquip )
	{
		return true;
	}

	return false;
}

bool GoInventory :: UnequipAll( eActionOrigin origin )
{
	gpassert( AssertValid() );

	bool success = true;

	for ( eEquipSlot slot = ES_VISIBLE_BEGIN ; slot != ES_INVISIBLE_END ; ++slot )
	{
		if ( !Unequip( slot, origin ) )
		{
			success = false;
		}
	}

	return ( success );
}

FuBiCookie GoInventory :: RSAutoEquip( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace )
{
	FUBI_RPC_THIS_CALL_RETRY( RSAutoEquip, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$$ should do some param validation here...

	return ( RCAutoEquip( slot, item, origin, bAutoplace ) );
}

FuBiCookie GoInventory :: RCAutoEquip( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace )
{
	FUBI_RPC_THIS_CALL_RETRY( RCAutoEquip, RPC_TO_ALL );

	GoHandle hItem( item );
	if ( !hItem && IsServerRemote() )
	{
		return ( RPC_FAILURE );		// retry later
	}

	AutoEquip( slot, item, origin, bAutoplace );

	return ( RPC_SUCCESS );
}

void GoInventory :: AutoEquip ( eEquipSlot slot, Goid item, eActionOrigin origin, bool bAutoplace )
{
	gpassert( AssertValid() );
	gpassert( item != GOID_INVALID );

	////////////////////////////////////////
	//	unequip

	if( item == GOID_NONE )
	{
		Equip( slot, GOID_NONE, origin );
		gWorld.UpdateInventoryGUICallback( GetGoid() );
		return;
	}

	////////////////////////////////////////
	//	equip

	GoHandle hItem( item );
	if( !hItem )	// extra hax0r protection for #13218
	{
		gpassert( hItem );
		return;
	}

	// Not told to equip it in any particular place
	if( slot == ES_ANY )
	{
		slot = hItem->GetGui()->GetEquipSlot();
	}

	bool bEquip = true;

	//	begin with shoving this into our "main" inventory storage, if it isn't already there... it's final resting place will
	//	likely change when the rest of this method executes...

	if ( !Contains( hItem.Get() ) )
	{
		Add( hItem.Get(), IL_MAIN, origin, false );
	}

	switch( slot )
	{
		case ES_WEAPON_HAND:
			{
				// First off, let's check what's in the shield hand
				Go * pShieldHand = GetEquipped( ES_SHIELD_HAND );
				if ( pShieldHand )
				{
					if ( (pShieldHand->HasAttack() && pShieldHand->GetAttack()->GetIsTwoHanded()) ||
						 (hItem->HasAttack() && hItem->GetAttack()->GetIsTwoHanded()) )
					{
						// Okay, one of these bad boys is two handed, so we'll need to de-equip our
						// shield hand
						Unequip( ES_SHIELD_HAND, origin );

						// Now place it into the appropriate slot
						if ( pShieldHand->IsRangedWeapon() && !hItem->IsRangedWeapon() )
						{
							SetLocation( pShieldHand, IL_ACTIVE_RANGED_WEAPON );
						}
						else if ( pShieldHand->IsShield() )
						{
							SetLocation( pShieldHand, IL_SHIELD );
						}
						else
						{
							SetLocation( pShieldHand, IL_MAIN, bAutoplace );
						}
					}
				}

				// Okay, our shield slot is taken care of, now what's going on in the weapon hand?
				Go * pWeaponHand = GetEquipped( ES_WEAPON_HAND );
				if ( pWeaponHand )
				{
					// Let's unequip it first
					Unequip( ES_WEAPON_HAND, origin );

					// Now let's place it into the appropriate slot
					if ( pWeaponHand->IsRangedWeapon() && !hItem->IsRangedWeapon() )
					{
						// Looks like it's a minigun or crossbow, let's keep it in the active slot
						SetLocation( pWeaponHand, IL_ACTIVE_RANGED_WEAPON );
					}
					else if ( pWeaponHand->IsMeleeWeapon() && !hItem->IsMeleeWeapon() )
					{
						SetLocation( pWeaponHand, IL_ACTIVE_MELEE_WEAPON );
					}
					else
					{
						// Sorry, no room for you in the active slots!
						SetLocation( pWeaponHand, IL_MAIN, bAutoplace );
					}
				}

				// Do we have a shield?  If so, can we equip it?
				Go * pShield = GetItem( IL_SHIELD );
				if ( (pShield && !hItem->GetAttack()->GetIsTwoHanded()) && (!pShieldHand || pShieldHand->IsRangedWeapon()) )
				{
					Equip( ES_SHIELD_HAND, pShield->GetGoid(), origin );
				}
			}
			break;
		case ES_SHIELD_HAND:
			{
				// First off, let's check what's in the weapon hand
				Go * pWeaponHand = GetEquipped( ES_WEAPON_HAND );
				if ( pWeaponHand )
				{
					if ( ((pWeaponHand->HasAttack() && pWeaponHand->GetAttack()->GetIsTwoHanded()) ||
						 (hItem->HasAttack() && hItem->GetAttack()->GetIsTwoHanded())) )
					{
						// Okay, one of these bad boys is two handed, so we'll need to de-equip our
						// weapon hand

						bool b2hRanged	= pWeaponHand->IsRangedWeapon() && (GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON);
						bool b2hMelee	= pWeaponHand->IsMeleeWeapon()  && (GetSelectedLocation() == IL_ACTIVE_MELEE_WEAPON ||
																			GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||
																			GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL);

						if ( !( (b2hRanged || b2hMelee) &&	hItem->IsShield() ) )
						{
							// As long as we're not trying to equip a shield over our active weapon, it's
							// okay to unequip.

							Unequip( ES_WEAPON_HAND, origin );

							// Now let's place it into the appropriate slot
							if ( pWeaponHand->IsRangedWeapon() && !hItem->IsRangedWeapon() )
							{
								SetLocation( pWeaponHand, IL_ACTIVE_RANGED_WEAPON );
							}
							else if ( pWeaponHand->IsMeleeWeapon() )
							{
								SetLocation( pWeaponHand, IL_ACTIVE_MELEE_WEAPON );
							}
							else
							{
								SetLocation( pWeaponHand, IL_MAIN, bAutoplace );
							}
						}
						else
						{
							// We don't want the shield to equip if it will displace an active weapon
							bEquip = false;
						}
					}
				}

				// Okay, our weapon slot is taken care of, now what's going on in the shield hand?
				Go * pShieldHand = GetEquipped( ES_SHIELD_HAND );
				if ( pShieldHand )
				{
					// We will not allow a shield to be equipped if the user is using their ranged weapon in
					// their shield hand
					if ( !(pShieldHand->IsRangedWeapon() && GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON &&
						 hItem->IsShield()) )
					{
						Unequip( ES_SHIELD_HAND, origin );
						if ( pShieldHand->IsRangedWeapon() && !hItem->IsRangedWeapon() )
						{
							SetLocation( pShieldHand, IL_ACTIVE_RANGED_WEAPON );
						}
						else if ( pShieldHand->IsShield() )
						{
							SetLocation( pShieldHand, IL_SHIELD );
						}
						else
						{
							SetLocation( pShieldHand, IL_MAIN, bAutoplace );
						}
					}
					else
					{
						bEquip = false;
					}
				}

				// Let's place it in the appropriate location
				if ( hItem->IsShield() )
				{
					// Let's move this to the shield location for now, we may need to add it
					// and handle the other shield cases.
					if ( bAutoplace )
					{
						if ( GetItem( IL_SHIELD ) )
						{
							SetLocation( hItem, IL_MAIN, bAutoplace );
						}
					}
					SetLocation( hItem, IL_SHIELD );

					if ( IsSpellSelected() )
					{
						Go * pWeapon = GetItem( IL_ACTIVE_MELEE_WEAPON );
						if ( pWeapon && pWeapon->HasAttack() && !pWeapon->GetAttack()->GetIsTwoHanded() )
						{
							Equip( ES_WEAPON_HAND, GetItem( IL_ACTIVE_MELEE_WEAPON )->GetGoid(), origin );
						}
						else if ( !pWeapon )
						{
							Select( IL_ACTIVE_MELEE_WEAPON, AO_USER );
							SetInventoryDirty();
						}
						else
						{
							bEquip = false;
						}
					}
				}
				else if ( hItem->IsRangedWeapon() )
				{
					SetLocation( hItem, IL_ACTIVE_RANGED_WEAPON );
				}
			}
			break;
		case ES_CHEST:
		case ES_HEAD:
		case ES_FOREARMS:
		case ES_FEET:
		case ES_AMULET:
		case ES_RING_0:
		case ES_RING_1:
		case ES_RING_2:
		case ES_RING_3:
			{
				if ( GetEquipped( slot ) )
				{
					SetLocation( GetEquipped( slot ), IL_MAIN, bAutoplace );
					Unequip( slot, origin );
				}
			}
			break;
	}

	if ( bEquip )
	{
		if ( !Equip( slot, item, origin ) )
		{
			// We were unable to equip the object, move it back into the main inventory -- biddle
			bool bAutoplace = true;
			if ( hItem->HasGold() )
			{
				bAutoplace = false;
			}

			SetLocation ( hItem.Get(), IL_MAIN , bAutoplace );
		}
		else
		{
			if ( HasGridbox() )
			{
				GetGridbox()->RemoveID( MakeInt( hItem->GetGoid() ) );
			}
		}

		if ( hItem->IsRangedWeapon() )
		{
			Select( IL_ACTIVE_RANGED_WEAPON, AO_USER );
		}
		else if ( hItem->IsMeleeWeapon() )
		{
			Select( IL_ACTIVE_MELEE_WEAPON, AO_USER );
		}
	}

	SetInventoryDirty();
}

FuBiCookie GoInventory :: RSSelect( eInventoryLocation il, eActionOrigin origin )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSelect, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security_info_1

	return ( RCSelect( il, origin ) );
}

FuBiCookie GoInventory :: RCSelect( eInventoryLocation il, eActionOrigin origin )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSelect, RPC_TO_ALL );

	Select( il, origin );

	return ( RPC_SUCCESS );
}

void GoInventory :: Select( eInventoryLocation il, eActionOrigin origin )
{
	LOADER_OR_RPC_ONLY;

	gpassert( AssertValid() );

	m_SelectedActiveLocation = il;
	m_SelectedItem = GetItem( il );

	if ( il == IL_ACTIVE_PRIMARY_SPELL || il == IL_ACTIVE_SECONDARY_SPELL )
	{
		Go * pSpellbook = GetEquipped( ES_SPELLBOOK );
		if ( pSpellbook )
		{
			if ( il == IL_ACTIVE_PRIMARY_SPELL )
			{
				pSpellbook->GetInventory()->Select( IL_SPELL_1, origin );
			}
			else if ( il == IL_ACTIVE_SECONDARY_SPELL )
			{
				pSpellbook->GetInventory()->Select( IL_SPELL_2, origin );
			}

			m_SelectedItem = pSpellbook->GetInventory()->GetSelectedItem();

			// design convention:	-	if we select a spell, but have a staff active, we equip the staff if we have one active
			//						-	if we don't have a staff, we unequip current melee weapons

			if( m_SelectedItem )
			{
				Go * es_weapon_hand = GetEquipped( ES_WEAPON_HAND );
				Go * es_shield_hand = GetEquipped( ES_SHIELD_HAND );

				if ( es_weapon_hand )
				{
					if ( !( es_weapon_hand->HasAttack() && es_weapon_hand->GetAttack()->GetAttackClass() == AC_STAFF ))
					{
						Unequip( ES_WEAPON_HAND, origin );
					}
				}

				if ( es_shield_hand )
				{
					if ( !( es_shield_hand->HasAttack() && es_shield_hand->GetAttack()->GetAttackClass() == AC_STAFF ))
					{
						Unequip( ES_SHIELD_HAND, origin );
					}

				}

				Go * meleeWeapon = GetItem( IL_ACTIVE_MELEE_WEAPON );

				if (	meleeWeapon &&
						meleeWeapon->HasAttack() &&
						( meleeWeapon->GetAttack()->GetAttackClass() == AC_STAFF ) &&
						( meleeWeapon != es_weapon_hand ) &&
						meleeWeapon != es_shield_hand )
				{
					Equip( meleeWeapon->GetGui()->GetEquipSlot(), meleeWeapon->GetGoid(), origin );
				}
			}
		}
	}

	if( origin == AO_USER )
	{
		if( il == IL_ACTIVE_MELEE_WEAPON )
		{
			GetGo()->GetMind()->SetActorWeaponPreference( WP_MELEE );
		}
		else if( il == IL_ACTIVE_RANGED_WEAPON )
		{
			GetGo()->GetMind()->SetActorWeaponPreference( WP_RANGED );
		}
		else if( il == IL_ACTIVE_PRIMARY_SPELL || il == IL_ACTIVE_SECONDARY_SPELL )
		{
			GetGo()->GetMind()->SetActorWeaponPreference( WP_MAGIC );
		}
	}

	m_SelectedActiveLocation = il;
	m_SelectedItem = GetItem( il );

	SetInventoryDirty();
}


FuBiCookie GoInventory :: RSAdd( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace )
{
	FUBI_RPC_THIS_CALL_RETRY( RSAdd, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security

	if ( item->HasGold() )
	{
		// special case for gold, just deposit it right now and bypass goofy all-clients matching code
		SDepositGold( item );
		return ( RPC_SUCCESS );
	}
	else
	{
		// make sure all clients flag is the same first
		gGoDb.SetAsAllClientsGo( item, GetGo()->IsAllClients() );

		// add it
		return ( RCAdd( item, il, origin, autoPlace ) );
	}
}

FuBiCookie GoInventory :: RCAdd( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace  )
{
	FUBI_RPC_THIS_CALL_RETRY( RCAdd, RPC_TO_ALL );

	return ( Add( item, il, origin, autoPlace ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoInventory :: Add( Go* item, eInventoryLocation il, eActionOrigin origin, bool autoPlace )
{
	LOADER_OR_RPC_ONLY;

	gpassert( AssertValid() );
	gpassert( item != NULL );
	gpassert( !::same_no_case( item->GetTemplateName(), gContentDb.GetDefaultGoldTemplate() ) );
	gpassert( !GetGo()->IsSpellBook() || item->IsSpell() )

	bool success = false;

	item->Deselect();

	// Apply the enchantments that this item might have
	if( IsSingularSlot( il ) && (item != GetItem( il ))  )
	{
		if( !IsSpellSlot( il ) )
		{
			GoMagic* magic = item->QueryMagic();
			if ( (magic != NULL) && (il != IL_MAIN) )
			{
				magic->ApplyEnchantments( GetGoid(), item->GetGoid() );
			}
		}
	}

	// already there? note that we must check for existence before adding it
	// so we can do addchild() on it. very important that it is a go child
	// before it goes into inventory - much crusty code assumes this.
	if ( m_InventoryDb.find( item ) == m_InventoryDb.end() )
	{
		// get original node as it's sitting there
		SiegeGuid oldNode = item->GetPlacement()->GetPosition().node;

		// rules about to change - note this
		GPDEV_ONLY( GetGo()->SetPlacementChanging() );

		// take ownership
		GetGo()->AddChild( item );

		if( item->HasAspect() )
		{
			item->GetAspect()->SetIsVisible( false );
		}

		// add it
		m_InventoryDb[ item ] = il;

		// that go is now in my inventory
		gpassert( !item->IsInsideInventory() );
		item->SetIsInsideInventory();

		// put it where i am as long as it is not an omni go stash
		if ( !(GetGo()->IsOmni() && GetGo()->HasStash()) )
		{
			if ( GetGo()->GetPlacement()->GetPosition().node.IsValid() )
			{
				item->GetPlacement()->CopyPlacement( *GetGo()->GetPlacement() );
			}

			// done changing rules - reset checks
			GPDEV_ONLY( GetGo()->ClearPlacementChanging() );

			// tell aiq to force update its cache now that the item is now in
			// inventory and is filtered out
			if ( !item->IsPendingEntry() && item->IsInAnyWorldFrustum() )
			{
				gAIQuery.OnOccupantChanged( item, oldNode, GetGo()->GetPlacement()->GetPosition().node, true );
			}
		}
		else
		{
			// done changing rules - reset checks
			GPDEV_ONLY( GetGo()->ClearPlacementChanging() );
		}

		// update its location
		success = SetLocation( item, il, autoPlace );
	

		// if the owner of the inventory is a party member, they can "identify" the object
		if ( GetGo()->IsAnyHumanPartyMember() && item->HasGui() )
		{
			item->GetGui()->SetIsIdentified( true );
		}
	}
	else
	{
		gpassertm( 0, "the inventory already contains this item" );
	}

	if ( GetGo()->IsScreenPartyMember() && origin == AO_USER )
	{
		item->PlayVoiceSound( "put_down" );
	}

	// Send out a world message to let the effects know about stopping and whatever else
	if ( !item->IsTransferring() )
	{
		WorldMessage( WE_PICKED_UP, GetGoid(), item->GetGoid() ).Send();
	}

	// always dirty on an addition
	SetInventoryDirty();
	
	if ( GetGo()->IsScreenPartyMember() && !success )
	{
		GetGo()->GetMind()->RSDrop( item, GetGo()->GetPlacement()->GetPosition(), QP_BACK, AO_REFLEX );
	}

	// done
	return ( success );
}

bool GoInventory :: CanAdd( const Go* item, eInventoryLocation il, bool autoPlace ) const
{
	gpassert( AssertValid() );
	gpassert( item != NULL );

	if ( Contains( item ) )
	{
		return ( false );
	}

	if ( !item->HasGui() )
	{
		return ( false );
	}

	if ( item->HasGold() )
	{
		return ( true );
	}

	if ( il == IL_ALL_SPELLS )
	{
		if ( QueryItems( IL_ALL_SPELLS ) >= MAX_SPELLS )
		{
			return ( false );
		}
	}

	if ( autoPlace )
	{
		gpassert( il == IL_MAIN );
		return ( CanAutoPlace( item ) );
	}

	return ( true );
}

FuBiCookie GoInventory :: RSSetLocation( Go* item, eInventoryLocation il, bool autoPlace )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetLocation, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security

	return ( RCSetLocation( item, il, autoPlace ) );
}

FuBiCookie GoInventory :: RCSetLocation( Go* item, eInventoryLocation il, bool autoPlace )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetLocation, RPC_TO_ALL );

	return ( SetLocation( item, il, autoPlace ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoInventory :: SetLocation( Go* item, eInventoryLocation il, bool autoPlace )
{
	LOADER_OR_RPC_ONLY;

	gpassert( AssertValid() );

	gpassert( il != IL_ALL );

	InventoryDb::iterator found = m_InventoryDb.find( item );
	gpassertm( found != m_InventoryDb.end(), "Item not in inventory" );

	bool success = true;

	// auto-place?
	if ( autoPlace )
	{
		gpassert( il == IL_MAIN );
		success = AutoPlace( item );
	}

	// Apply the enchantments that this item might have
	if( IsSingularSlot( il ) && (item != GetItem( il ))  )
	{
		if( !IsSpellSlot( il ) )
		{
			GoMagic* magic = item->QueryMagic();
			if ( (magic != NULL) && (il != IL_MAIN) )
			{
				magic->ApplyEnchantments( GetGoid(), item->GetGoid() );
			}
		}
	}

	if ( il != found->second )
	{
		// release old item at that location
		if ( (il >= IL_ACTIVE_MELEE_WEAPON) && (il <= IL_ACTIVE_RANGED_WEAPON) )
		{
			Go* existing = GetItem( il );

			if ( existing != NULL )
			{
				SetLocation( existing, IL_MAIN, true );
			}
		}
		else if (( il >= IL_ACTIVE_PRIMARY_SPELL ) && ( il <= IL_ACTIVE_SECONDARY_SPELL ))
		{
			Go* existing = GetItem( il );

			if ( existing != NULL )
			{
				SetLocation( existing, IL_ALL_SPELLS );
			}
		}

		// any item moved to IL_MAIN must have it's enchantments removed
		if ( il == IL_MAIN )
		{
			// remove any enchantments applied by the item
			GoMagic* magic = item->QueryMagic();
			if ( (magic != NULL) && !magic->GetGo()->IsPotion() )
			{
				gGoDb.RemoveEnchantments( GetGoid(), item->GetGoid(), true );
			}
		}

		// ok stick it there
		found->second = il;

		SetInventoryDirty();
	}

	// If this location is the selected one, then setting the location may change the
	// current selection, so let's reselect it.  Let's also exclude any gos that are
	// being created to prevent any conflicts.
	if ( !GetGo()->IsCommittingCreation() && !GetGo()->IsPendingEntry() )
	{
		if ( GetGo()->IsSpellBook() && GetGo()->HasParent() &&
			 GetGo()->GetParent()->HasInventory() && GetGo()->GetParent()->GetInventory()->IsEquipped( GetGo() ) )
		{
			if ( (il == IL_SPELL_1 && GetGo()->GetParent()->GetInventory()->GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL) ||
				 (il == IL_SPELL_2 && GetGo()->GetParent()->GetInventory()->GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL)	)
			{
				GetGo()->GetParent()->GetInventory()->Select( il == IL_SPELL_1 ? IL_ACTIVE_PRIMARY_SPELL : IL_ACTIVE_SECONDARY_SPELL, AO_USER );
				GetGo()->GetParent()->GetMind()->RSStop( AO_USER );
			}
		}
	}

	// done
	return ( success );
}

bool GoInventory :: CanSetLocation( const Go* item, eInventoryLocation il, bool autoPlace ) const
{
	gpassert( AssertValid() );

	if ( !Contains( item ) )
	{
		return ( false );
	}

	if ( il == IL_ALL_SPELLS )
	{
		if ( QueryItems( IL_ALL_SPELLS ) >= MAX_SPELLS )
		{
			return ( false );
		}
	}

	if ( autoPlace )
	{
		gpassert( il == IL_MAIN );
		return ( CanAutoPlace( item ) );
	}

	return ( true );
}


FuBiCookie GoInventory :: RSRemove( Go* item, bool drop, Go* dropFor )
{
	FUBI_RPC_THIS_CALL_RETRY( RSRemove, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security

	SiegePos dropPos;
	Quat dropQuat;

	if ( drop )
	{
		RetrieveDropPositionAndOrientation( dropFor, item, dropPos, dropQuat );
	}
	else
	{
		dropPos = GetGo()->GetPlacement()->GetPosition();
		dropQuat = GetGo()->GetPlacement()->GetOrientation();
	}

	return ( RCRemove( item, dropPos, dropQuat, drop ) );
}

FuBiCookie GoInventory :: RCRemove( Go* item, const SiegePos& dropPos, const Quat& dropQuat, bool bDrop )
{
	FUBI_RPC_TAG();

	// handled case, it's ok
	if ( !Contains( item ) )
	{
		return ( RPC_SUCCESS );
	}

	FUBI_RPC_THIS_CALL_RETRY( RCRemove, RPC_TO_ALL );

	return ( Remove( item, &dropPos, &dropQuat, bDrop ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

FuBiCookie GoInventory :: RCRemove( Go* item, const SiegePos& dropPos )
{
	FUBI_RPC_TAG();

	// handled case, it's ok
	if ( !Contains( item ) )
	{
		return ( RPC_SUCCESS );
	}

	FUBI_RPC_THIS_CALL_RETRY( RCRemove, RPC_TO_ALL );

	return ( Remove( item, &dropPos, &Quat::IDENTITY ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

FuBiCookie GoInventory :: RCRemove( Go* item )
{
	FUBI_RPC_TAG();

	// handled case, it's ok
	if ( !Contains( item ) )
	{
		return ( RPC_SUCCESS );
	}

	FUBI_RPC_THIS_CALL_RETRY( RCRemove, RPC_TO_ALL );

	return ( Remove( item, NULL, NULL ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoInventory :: Remove( Go* item, const SiegePos* dropPos, const Quat* dropQuat, bool bDrop )
{
	gpassert( AssertValid() );

	// find it
	InventoryDb::iterator found = m_InventoryDb.find( item );
	if ( found == m_InventoryDb.end() )
	{
		return ( false );
	}

	// unequip if equipped
	eEquipSlot slot = GetEquippedSlot( item );
	if ( slot != ES_NONE )
	{
		Unequip( slot, AO_REFLEX );
	}

	// remove any enchantments applied by the item
	GoMagic* magic = item->QueryMagic();
	if ( (magic != NULL) && !magic->GetGo()->IsPotion() )
	{
		gGoDb.RemoveEnchantments( GetGoid(), item->GetGoid(), true );
	}

	// release ownership - this will unlink the child and remove it from inv db
	// and it will assign ownership to the server player
	GetGo()->RemoveChild( item );
	item->SetPlayer( gServer.GetComputerPlayer() );

	// drop it where server told us
	if ( dropPos != NULL )
	{
		SiegeGuid oldNode = item->GetPlacement()->GetPosition().node;

		if ( bDrop && !item->HasActor() )
		{
			gSim.StartDropAnimation( item->GetGoid(), *dropPos, *dropQuat );
		}

		// tell aiq to force update its cache now that the item is no longer in
		// inventory and no longer being filtered
		if ( !item->IsPendingEntry() && item->IsInAnyWorldFrustum() )
		{
			// get the new node it may be in due to the drop animation. may be
			// different from the requested drop node.
			SiegeGuid newNode = item->GetPlacement()->GetPosition().node;
			gAIQuery.OnOccupantChanged( item, oldNode, newNode, true );
		}
	}

	// Send out a world message to let the effects know about starting and whatever else
	if ( !item->IsTransferring() )
	{
		WorldMessage( WE_DROPPED, GetGoid(), item->GetGoid() ).Send();
	}

	if ( HasGridbox() )
	{
		GetGridbox()->RemoveID( MakeInt( item->GetGoid() ) );
	}

	// always dirty inventory on a removal
	SetInventoryDirty();

	// set the item to be a virgin drop if a monster droped it!
	if( !GetGo()->GetPlayer() || ( GetGo()->GetPlayer()->IsComputerPlayer() && !GetGo()->HasStore() ) )
	{
		item->GetAspect()->SetVirginPickup( true );
	}

	// done
	return ( true );
}

void GoInventory :: SDeactivateGuiItems()
{
	CHECK_SERVER_ONLY;

	// We only ever need to do this for party members.
	if ( GetGo()->IsAnyHumanPartyMember() )
	{
		RCDeactivateGuiItems();
	}
}

void GoInventory :: RCDeactivateGuiItems()
{
	FUBI_RPC_THIS_CALL( RCDeactivateGuiItems, GetGo()->GetPlayer()->GetMachineId() );
	// In order to properly synchronize between the world and the GUI,
	// we need to see if any of the active items ( items stuck to the cursor )
	// belong to that character.  If they do, we will deactivate the item automatically,
	// because if they're killed, it will drop that item to the ground ( or will be autoplaced
	// back in inventory if they're knocked unconscious ).

	if ( gUIShell.GetItemActive() )
	{
		UIItemVec items;
		UIItemVec::iterator iItem;
		gUIShell.FindActiveItems( items );
		for ( iItem = items.begin(); iItem != items.end(); ++iItem )
		{
			GoHandle hItem( MakeGoid( (*iItem)->GetItemID() ) );
			if ( hItem.IsValid() && Contains( hItem ) )
			{
				(*iItem)->ActivateItem( false );

				// The person is still alive, so they didn't drop the weapon
				if ( ::IsAlive( GetGo()->GetAspect()->GetLifeState() ) )
				{
					// Let's try to autoplace it back in
					if ( !GetGridbox()->AutoItemPlacement( BuildUiName( hItem ), true, MakeInt( hItem->GetGoid() ) ) )
					{
						// Shoot, we can't auto-fit it, gotta drop it.
						RSRemove( hItem, true );
					}
				}
			}
		}
	}
}

FuBiCookie GoInventory :: RSTransfer( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace )
{
	FUBI_RPC_THIS_CALL_RETRY( RSTransfer, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	gpassert( item != NULL );
	gpassert( dest != NULL );
	gpassert( dest->HasInventory() );	

	// $$ security

	// simple case is if the all-clients state matches
	if ( (item->IsAllClients() == dest->IsAllClients()) && !item->HasGold() )
	{
		return ( RCTransfer( item, dest, il, origin, autoPlace ) );
	}
	else
	{
		// damn, do it the hard way - let's force all gold to go this way so it is handled through the proper server calls 
		FuBiCookie rem = RSRemove( item, false );
		FuBiCookie add = dest->GetInventory()->RSAdd( item, il, origin, autoPlace );
		return ( ((rem == RPC_SUCCESS) && (add == RPC_SUCCESS)) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
	}
}

FuBiCookie GoInventory :: RCTransfer( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace )
{
	FUBI_RPC_THIS_CALL_RETRY( RCTransfer, RPC_TO_ALL );

	return ( Transfer( item, dest, il, origin, autoPlace ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoInventory :: Transfer( Go* item, Go* dest, eInventoryLocation il, eActionOrigin origin, bool autoPlace )
{
	gpassert( item != NULL );
	gpassert( dest != NULL );
	gpassert( dest->HasInventory() );

	// $ this is a network optimization to prevent unnecessary paired calls to
	//   RCSetAsAllClientsGo, once for false, and once for true.
	item->SetTransferring();
	bool success = Remove( item, false ) && dest->GetInventory()->Add( item, il, origin, autoPlace );
	item->ClearTransferring();

	if ( GetGo()->IsScreenPartyMember() && dest->HasGui() && dest->GetGui()->IsSpellBook() && origin == AO_USER )
	{
		item->PlayVoiceSound( "put_down" );
	}

	return ( success );
}

void GoInventory :: SBeginUse( Goid item )
{
	CHECK_SERVER_ONLY;

	// Only want to do this on party members who are drinking a potion, don't care about lever pulling, etc.
	GoHandle hItem( item );
	if ( GetGo()->IsAnyHumanPartyMember() && hItem && hItem->IsPotion() )
	{
		RCBeginUse();
	}
}

void GoInventory :: RCBeginUse()
{
	FUBI_RPC_THIS_CALL( RCBeginUse, GetGo()->GetPlayer()->GetMachineId() );	

	if ( HasGridbox() )
	{
		GetGridbox()->SetLocalPlace( false );
	}
}

bool GoInventory :: SAutoUse( Goid item, eActionOrigin /* origin */, bool bDeleteExpendedItem )
{
	CHECK_SERVER_ONLY;

	gpassert( AssertValid() );

	GoHandle hItem( item );
	if ( !hItem )
	{
		return ( false );
	}

	WorldMessage message_item( WE_REQ_USE, GetGoid(), item );
	message_item.Send();

	if ( hItem->IsItem() )
	{
		if( hItem->HasMagic() )
		{
			bool lifeHealing = gAIQuery.GetAlterationSum( hItem, ALTER_LIFE ) > 0.0f ? true : false;
			bool manaHealing = gAIQuery.GetAlterationSum( hItem, ALTER_MANA ) > 0.0f ? true : false;
			if ( lifeHealing && manaHealing )
			{
				if ( ( lifeHealing && ::IsEqual( GetGo()->GetAspect()->GetCurrentLife(), GetGo()->GetAspect()->GetMaxLife() ) ) &&
					 ( manaHealing && ::IsEqual( GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() ) ) )
				{
					return false;
				}
			}
			else if ( ( lifeHealing && ::IsEqual( GetGo()->GetAspect()->GetCurrentLife(), GetGo()->GetAspect()->GetMaxLife() ) ) ||
					  ( manaHealing && ::IsEqual( GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() ) ) )
			{
				return false;
			}

			// disallow use of any item that would apply an enchantment that is single instance
			if( hItem->GetMagic()->HasEnchantments() )
			{
				if( gGoDb.HasSingleInstanceEnchantment( GetGoid(), hItem->GetMagic()->GetEnchantmentStorage() ) )
				{
					return false;
				}
			}

			hItem->GetMagic()->SApplyEnchantments( GetGoid(), GetGoid() );

			if ( hItem->GetMagic()->IsPotion() && hItem->GetMagic()->HasEnchantments() &&
				 ((hItem->GetMagic()->GetPotionFullRatio() == 0.0f || hItem->GetMagic()->GetPotionAmount() < 1.0f) ||
				 (lifeHealing && manaHealing) ))
			{
				RCRemoveExpendedItem( hItem->GetGoid() );

				if ( bDeleteExpendedItem )
				{
					gGoDb.SMarkForDeletion( hItem->GetGoid() );
					RCEndUse();
				}
			}			
		}
	}

	if ( hItem->HasParent() && hItem->GetParent() == GetGo() )
	{
		SetInventoryDirty();
	}

	return ( true );
}

void GoInventory :: RCRemoveExpendedItem( Goid item )
{
	FUBI_RPC_THIS_CALL( RCRemoveExpendedItem, GetGo()->GetPlayer()->GetMachineId() );

	GoHandle hTarget( item );
	if ( hTarget.IsValid() )
	{
		UIItem * pItem = GetGridbox()->RemoveID( MakeInt(hTarget->GetGoid()) );
		if ( !pItem )
		{
			pItem = GetItemFromGO( hTarget, false );
			pItem->ActivateItem( false );
		}

		SetInventoryDirty();
	}
}

void GoInventory :: SEndUse( Goid item )
{
	GoHandle hTarget( item );
	if ( hTarget )
	{
		if ( hTarget->IsItem() && hTarget->HasMagic() &&
			 hTarget->GetMagic()->IsPotion() && hTarget->GetMagic()->HasEnchantments() &&
			 ( (hTarget->GetMagic()->GetPotionFullRatio() == 0.0f || hTarget->GetMagic()->GetPotionAmount() < 1.0f) ||
			   hTarget->GetMagic()->IsRejuvenationPotion() ) )
		{
			gGoDb.SMarkForDeletion( hTarget->GetGoid() );
		}

		// Only want to reset grid state if it is a potion drink of another party member.
		if ( hTarget->IsPotion() && GetGo()->IsAnyHumanPartyMember() )
		{
			RCEndUse();
		}
	}
}

void GoInventory :: RCEndUse()
{
	FUBI_RPC_THIS_CALL( RCEndUse, GetGo()->GetPlayer()->GetMachineId() );

	if ( HasGridbox() )
	{
		GetGridbox()->SetLocalPlace( true );
	}
}

bool GoInventory :: SAutoGet( Goid item, eActionOrigin origin )
{
	gpassert( AssertValid() );

	GoHandle object( item );

	if ( !object.IsValid() || object->IsInsideInventory() )
	{
		return false;
	}

	// Not told to equip it in any particular place
	eEquipSlot slot = object->GetGui()->GetEquipSlot();

	bool bEquipObject = false;
	bool bAddToInventory = true;

	if ( TestEquipMinReqs( object ) && !GetForceGet() && !IsPackOnly() )
	{
		switch( slot )
		{
			case ES_WEAPON_HAND:

				if( GetEquipped( ES_WEAPON_HAND ) == NULL )
				{
						// If there is no active melee weapon ( or it is the same as the item we're about to equip ) and a spell is not our active weapon, it is okay to equip
					if(	((GetItem( IL_ACTIVE_MELEE_WEAPON ) == NULL || GetItem( IL_ACTIVE_MELEE_WEAPON )->GetGoid() == item) && !IsSpellSelected() && !object->IsRangedWeapon() ) ||

						// If there is a spell selected and the weapon we want to equip is a staff, it is okay.  Other weapons are not allowed to be equipped in this case
						( IsSpellSelected() && object->GetAttack()->GetAttackClass() == AC_STAFF && GetItem( IL_ACTIVE_MELEE_WEAPON ) == NULL ) ||

						// If there is no active ranged weapon ( or it is the same as the item we're about to equip ) and a spell is not our active weapon, and it is a ranged weapon, it is also okay to equip
						((GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL || GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid() == item) && !IsSpellSelected() && object->IsRangedWeapon() ) )
					{
						// Is there a shield item in the other hand?

						Go * shield_item = GetEquipped( ES_SHIELD_HAND );

						if ( shield_item )
						{
							if( shield_item->HasAttack() && shield_item->GetAttack()->GetIsTwoHanded() )
							{
								if ( !Contains( object ) )
								{
									RSAdd( object.Get(), IL_ACTIVE_MELEE_WEAPON, origin, false );
									bAddToInventory = false;
								}
								else
								{
									RSSetLocation( object, IL_ACTIVE_MELEE_WEAPON );
								}
								break;
							}

							// Is either item two-handed?
							if ( object->HasAttack() && object->GetAttack()->GetIsTwoHanded() )
							{
								// Have to unequip the shield, this baby takes both hands
								RSUnequip( ES_SHIELD_HAND, origin );
							}
						}
						bEquipObject = true;
					}
					else if ( IsSpellSelected() && (object->IsRangedWeapon() || object->IsMeleeWeapon()) )
					{
						eInventoryLocation il = IL_ACTIVE_MELEE_WEAPON;
						if ( object->IsRangedWeapon() )
						{
							il = IL_ACTIVE_RANGED_WEAPON;
						}

						if (( GetItem( il ) == NULL ) && !Contains( object ))
						{
							RSAdd( object.Get(), il, origin, false );
							bAddToInventory = false;
						}
						else if (( GetItem( il ) == NULL ) && Contains( object ))
						{
							RSSetLocation( object, il );
						}
					}
				}
				else
				{
					if ( object->IsRangedWeapon() && GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL )
					{
						RSAdd( object.Get(), IL_ACTIVE_RANGED_WEAPON, origin, false );
					}
				}
				break;

			case ES_SHIELD_HAND:

				if( GetEquipped( ES_SHIELD_HAND ) == NULL &&
					((GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL || GetItem( IL_ACTIVE_RANGED_WEAPON )->GetGoid() == item ) ||
					(GetItem( IL_SHIELD ) == NULL || GetItem( IL_SHIELD )->GetGoid() == item )) && !IsSpellSelected() )
				{

					// Is there an weapon item in the other hand?

					Go * weapon_item = GetEquipped( ES_WEAPON_HAND );

					if ( weapon_item )
					{
						// Is either item two-handed?
						if( (object->HasAttack() && object->GetAttack()->GetIsTwoHanded()) ||
							 weapon_item->HasAttack() && weapon_item->GetAttack()->GetIsTwoHanded() )
						{
							if ( object->HasAttack() )
							{
								if ( GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL )
								{
									RSAdd( object.Get(), IL_ACTIVE_RANGED_WEAPON, origin, false );
									bAddToInventory = false;
								}
							}
							else
							{
								if (( GetItem( IL_SHIELD ) == NULL ) && !Contains( object ))
								{
									RSAdd( object.Get(), IL_SHIELD, origin, false );
									bAddToInventory = false;
								}
								else if (( GetItem( IL_SHIELD ) == NULL ) && Contains( object ))
								{
									RSSetLocation( object, IL_SHIELD );
								}
							}
							break;
						}
					}
					else if ( object->HasAttack() && ( GetSelectedLocation() != IL_ACTIVE_RANGED_WEAPON ) )
					{
						// If we don't have any weapon at all, let's equip the bow...
						if ( !(GetSelectedLocation() == IL_ACTIVE_MELEE_WEAPON &&
							 GetItem( IL_ACTIVE_MELEE_WEAPON ) == NULL) )
						{
							// Otherwise, let's add this to our free ranged slot
							// This is a ranged weapon like the bow
							if( ( GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL ))
							{
								RSAdd( object.Get(), IL_ACTIVE_RANGED_WEAPON, origin, false );
								bAddToInventory = false;
							}
							break;
						}
					}

					bEquipObject = true;
				}
				else if ( IsSpellSelected() )
				{
					eInventoryLocation il = IL_MAIN;
					if ( object->IsShield() )
					{
						il = IL_SHIELD;
					}
					else if ( object->IsWeapon() )
					{
						il = IL_ACTIVE_RANGED_WEAPON;
					}

					if (( GetItem( il ) == NULL ) && !Contains( object ))
					{
						RSAdd( object.Get(), il, origin, false );
						bAddToInventory = false;
					}
					else if (( GetItem( il ) == NULL ) && Contains( object ))
					{
						RSSetLocation( object, il );
					}
				}
				else if ( GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON &&
						  object->IsShield() && !GetItem( IL_SHIELD ) )
				{
					if ( !Contains( object ) )
					{
						RSAdd( object.Get(), IL_MAIN, origin, false );
					}
					RSSetLocation( object, IL_SHIELD );
					bAddToInventory = false;
				}
				else if ( GetItem( IL_ACTIVE_RANGED_WEAPON ) == NULL && object->IsRangedWeapon() )
				{
					if ( !Contains( object ) )
					{
						RSAdd( object.Get(), IL_MAIN, origin, false );
					}
					RSSetLocation( object, IL_ACTIVE_RANGED_WEAPON );
					bAddToInventory = false;
				}
				break;

			case ES_CHEST:

				if( GetEquipped( ES_CHEST ) == NULL )
				{
					bEquipObject = true;
				}
				break;

			case ES_HEAD:
				if( GetEquipped( ES_HEAD ) == NULL )
				{
					bEquipObject = true;
				}
				break;

			case ES_FOREARMS:
				if( GetEquipped( ES_FOREARMS ) == NULL )
				{
					bEquipObject = true;
				}
				break;

			case ES_FEET:
				if( GetEquipped( ES_FEET ) == NULL )
				{
					bEquipObject = true;
				}
				break;
			case ES_AMULET:
				if( GetEquipped( ES_AMULET ) == NULL )
				{
					bEquipObject = true;
				}
				break;
			case ES_SPELLBOOK:
				if( GetEquipped( ES_SPELLBOOK ) == NULL )
				{
					bEquipObject = true;
				}
				break;
			case ES_RING:
				if(( GetEquipped( ES_RING_0 ) == NULL ) ||
				   ( GetEquipped( ES_RING_1 ) == NULL ) ||
				   ( GetEquipped( ES_RING_2 ) == NULL ) ||
				   ( GetEquipped( ES_RING_3 ) == NULL ))
				{
					bEquipObject = true;
				}
				break;
		}
	}

	bool bEquipped	= true;
	bool bAdded		= false;

	if( bEquipObject )
	{
		eInventoryLocation selLoc = GetSelectedLocation();

		// Add this item to our "main" inventory storage, if it isn't already there
		if ( !Contains( object.Get() ) )
		{
			RSAdd( object.Get(), IL_MAIN, origin, false );
		}

		if (RPC_FAILURE == RCEquip( slot, item, origin ) )
		{
			// We were unable to equip the object, move it back into the main inventory -- biddle
			RSSetLocation ( object.Get(), IL_MAIN , true );
			bEquipped = false;
			bAdded = true;
		}
		else if (( selLoc == IL_ACTIVE_PRIMARY_SPELL || selLoc == IL_ACTIVE_SECONDARY_SPELL ) && object->HasAttack() && object->GetAttack()->GetAttackClass() == AC_STAFF )
		{
			// When you equip a weapon, it automatically sets itself as the active weapon.  Since this a context-sensitive function,
			// we have to detect if something is already equipped.  Since a staff CAN be used while a spell is active, we have to equip it,
			// but also maintain that the spell slot stays selected.
			RSSelect( selLoc, origin );
		}

	}
	else
	{
		bEquipped = false;

		// No free slots - just add to inventory
		if ( bAddToInventory && ( CanAdd( object.Get(), IL_MAIN, GetForceGet() ? false : true ) ) )
		{
			bAdded = true;

			RSAdd( object.Get(), IL_MAIN, origin, GetForceGet() ? false : true );
		}
	}

	WorldMessage( WE_GOT, GetGoid(), item, MakeInt(item) ).SendDelayed();

	
	// set the victory, end game statistics for loot pick up!
	if ( object->IsGold() )
	{
		// picking up gold!
		gpassert( GetGo()->GetPlayer() );
		gpassert( object->HasAspect() );

		if( object->GetAspect()->GetIsVirginPickup() ) 
		{
			gVictory.SIncrementStat( GS_GOLD_GAINED, GetGo()->GetPlayer(), object->GetAspect()->GetGoldValue() ) ; 
			object->GetAspect()->SetVirginPickup( false ); // gold goes away, but might as well :)
		}
	} 
	else if( object->HasAspect() )
	{
		if( object->GetAspect()->GetIsVirginPickup() ) 
		{
			// increment the loot number
			gVictory.SIncrementStat( GS_LOOT_GAINED, GetGo()->GetPlayer() ) ; 
			object->GetAspect()->SetVirginPickup( false );

			// increment the loot value!
			gVictory.SIncrementStat( GS_LOOT_VALUE, GetGo()->GetPlayer(), object->GetAspect()->GetGoldValue() ) ; 
		}
	}

	return true;
}

bool GoInventory :: TestGet( Goid item, bool silent ) const
{
	GoHandle object( item );

	if ( !object.IsValid() || object->IsInsideInventory() )
	{
		// The object may have been picked up and deleted by now ( if it is gold ), safely exit
		return false;
	}

	bool bCanEquip = TestEquipPassive( item );
	if ( bCanEquip )
	{
		return true;
	}

	if( object.IsValid() )
	{
		bool bStored = Contains( object.Get() );
		bool bCanAdd = CanAdd( object.Get(), IL_MAIN, true );

		if( !bStored && bCanAdd )
		{
			return true;
		}

		if ( bStored )
		{
			return false;
		}
	}

	if( !silent )
	{
		GetGo()->PlayVoiceSound( "gui_inventory_full" );

		if ( GetGridbox() && GetGridbox()->GetFullRatio() == 1.0f )
		{
			gpscreen( $MSG$ "Inventory is full.\n" );
		}
		else if ( (object->GetGui()->GetEquipSlot() != ES_NONE) && !bCanEquip )
		{
			gpscreen( $MSG$ "Item cannot be equipped and does not fit into Inventory" );
		}
		else
		{
			gpscreen( $MSG$ "Item does not fit into Inventory.\n" );
		}
	}
	return false;
}

#if !GP_RETAIL
FuBiCookie GoInventory :: RSSetGold( int gold )
{
	FUBI_RPC_THIS_CALL_RETRY( RSSetGold, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security

	SSetGold( gold );
	return ( RPC_SUCCESS );
}
#endif

void GoInventory :: SSetGold( int gold )
{
	CHECK_SERVER_ONLY;
	RCSetGold( gold );
}

FuBiCookie GoInventory :: RCSetGold( int gold )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetGold, RPC_TO_ALL );

	// $$ security

	SetGold( gold );

	return ( RPC_SUCCESS );
}

void GoInventory :: SetGold( int gold )
{
	m_Gold = gold;

	SetInventoryDirty();
	SetGoldDirty();
}

bool GoInventory :: SDepositGold( Go* item )
{
	CHECK_SERVER_ONLY;
	gpassertm( item->HasGold(), "Trying to deposit an item without gold value." );

	// $$ security

	// update gold that we've taken

	if( item->HasGold() )
	{
		if ( HasGridbox() )
		{
			GetGridbox()->RemoveID( MakeInt( item->GetGoid() ) );
		}

		int gold	 = GetGold();

		item->GetGold()->RCDepositSelfIn( GetGo() );

		if ( (gold + item->GetAspect()->GetGoldValue()) <= gContentDb.GetMaxPartyGold() )
		{
			gGoDb.SMarkForDeletion( item->GetGoid() );
		}
		else
		{
			item->GetAspect()->SSetGoldValue( item->GetAspect()->GetGoldValue() - (gContentDb.GetMaxPartyGold()-gold), true );
		}
		return true;
	}
	else
	{
		return false;
	}
}


void GoInventory :: RCDepositGold( Go * item )
{
	// $ important: this is NOT an rpc function - that is handled by a router
	//   function in GoGold called RCDepositSelfIn().

	gpassert( item->HasGold() );

	if ( HasGridbox() )
	{
		GetGridbox()->RemoveID( MakeInt( item->GetGoid() ) );
	}

	DepositGold( item->GetAspect()->GetGoldValue() );

	// triggers may be waiting for this message, have to send this out because
	// gold is a special case.
	WorldMessage( WE_PICKED_UP, GetGoid(), item->GetGoid() ).Send();

	if ( GetGo()->IsScreenPartyMember() )
	{
		item->PlayVoiceSound( "pick_up" );
		item->PlayVoiceSound( "put_down" );
	}
}


void GoInventory :: DepositGold( DWORD amount )
{
	LOADER_OR_RPC_ONLY;

	gpassert( AssertValid() );

	if ( GetGold() == gContentDb.GetMaxPartyGold() )
	{
		if ( GetGo()->GetPlayer()->GetMachineId() == gServer.GetLocalHumanPlayer()->GetMachineId() )
		{
			gpscreen( $MSG$ "Party cannot carry any more gold." );
		}
	}
	else if ( (GetGold() + (int)amount) > gContentDb.GetMaxPartyGold() )
	{
		SetGold( gContentDb.GetMaxPartyGold() );
	}
	else
	{
		SetGold( GetGold() + amount );
	}

	SetInventoryDirty();
}


void GoInventory :: DropItemsFor( Goid killerGoid )
{
	CHECK_SERVER_ONLY;

	gpassert( AssertValid() );

	GoHandle killer( killerGoid );

	// drop inventory items
	if ( !m_InventoryDb.empty() )
	{
		// get the player who killed us
		PlayerId killerPlayer = IsServerLocal() ? PLAYERID_COMPUTER : gServer.GetScreenPlayer()->GetId();

		if ( killer )
		{
			killerPlayer = killer->GetPlayer()->GetId();
		}

		// see if we should spew equipped items or inventory items
		eSpewType spewEquipped = GetEquippedItemsSpewType( killerPlayer );

		// now spew
		InventoryDb db( m_InventoryDb );
		InventoryDb::iterator i, ibegin = db.begin(), iend = db.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Go* item = i->first;
			eSpewType spewItem = SPEW_DELETE;

			// non-actors always spew
			if ( !GetGo()->IsActor() )
			{
				spewItem = SPEW_GROUND;
			}
			else if ( item->GetGui()->IsDroppable() )
			{
				bool isEquipped = ( (GetLocation(item) != IL_MAIN) || item->IsEquipped() || item->IsActiveSpell() );

				// Drop option ( removed ::IsMultiPlayer() && for DSX feature )
				if ( GetGo()->IsAnyHumanPartyMember() )
				{
					if ( gServer.GetDropInvOption() == DIO_NOTHING )
					{
						continue;
					}
					else if ( gServer.GetDropInvOption() == DIO_EQUIPPED )
					{
						if ( !isEquipped )
						{
							continue;
						}
					}
					else if ( gServer.GetDropInvOption() == DIO_INVENTORY )
					{
						if ( isEquipped )
						{
							continue;
						}
					}
				}

				// only droppable items are considered here, default to ground
				spewItem = SPEW_GROUND;

				bool canExpire = true;

				// find expiration class info, this will make sure we don't delete anyone's armor
				// that is really the mesh of an instance that can't be deleted.  if they can be deleted
				// this will clean itself up correctly because the go will be removed too !
				const gpstring& expireClass	= GetGo()->GetCommon()->GetAutoExpirationClass();
				const ContentDb::ExpireEntry* expire = gContentDb.FindExpirationClass( expireClass );
				gpassert( expire != NULL );
				if ( expire == NULL )
				{
					canExpire = true;
				}
				// never expires?
				if ( expire != NULL && expire->m_DelaySeconds < 0 )
				{
					canExpire = false;
				}

				// non-pcontent items are usually ok
				if ( !item->IsPContentInventory() || GetGo()->IsAnyHumanPartyMember() || canExpire == false )
				{
					spewItem = SPEW_GROUND;

					if ( isEquipped && (spewEquipped == SPEW_IGNORE) )
					{
						// special case - leave it alone
						spewItem = SPEW_IGNORE;
					}
				}
				else if ( isEquipped )
				{
					// now we just spew based on whether or not we're supposed
					// to spew the non-equipped inventory vs. the equipped
					// inventory (detected previously).
					spewItem = spewEquipped;
				}

				// special: may not want to drop spell book
				if (   (spewItem == SPEW_GROUND)
					&& item->GetGui()->IsSpellBook()
					&& GetGo()->HasActor()
					&& !GetGo()->GetActor()->GetDropsSpellbook() )
				{
					spewItem = SPEW_DELETE;
				}				
			}

			// do it
			if ( spewItem == SPEW_GROUND )
			{
				// drop it
				RSRemove( item, true, killer );
			}
			else if ( spewItem == SPEW_DELETE )
			{
				// delete it
				gGoDb.SMarkGoAndChildrenForDeletion( item->GetGoid() );
			}
		}
	}

	if ( HasGold() )
	{
		// see if we should spew gold. we do this by default unless we're a human
		// party member and there are other party members still standing.
		bool shouldSpewGold = true;
		if ( GetGo()->IsAnyHumanPartyMember() )
		{
			if ( gServer.GetDropInvOption() == DIO_NOTHING ||
				 gServer.GetDropInvOption() == DIO_EQUIPPED )
			{
				return;
			}

			// are there other party members alive? we can just see if they have
			// gold, and if they do, then we can assume they didn't spew yet.
			GopColl::const_iterator
					i,
					ibegin = GetGo()->GetParent()->GetChildBegin(),
					iend   = GetGo()->GetParent()->GetChildEnd  ();
			for ( i = ibegin ; i != iend ; ++i )
			{
				Go* child = *i;
				if ( (child != GetGo()) && child->HasInventory() && child->GetInventory()->HasGold() )
				{
					shouldSpewGold = false;
					break;
				}
			}
		}

		// spewing then?
		if ( shouldSpewGold )
		{
			// spew piles of gold
			IntColl::iterator i = m_GoldPiles.begin();
			int totalGold = GetGold();
			while ( totalGold > 0 )
			{
				int goldSpew = totalGold;
				if ( i != m_GoldPiles.end() )
				{
					goldSpew = *i;
					++i;
				}

				::minimize( goldSpew, totalGold );
				gpassert( goldSpew > 0 );

				// figure out what and where to drop
				GoCloneReq cloneReq( GOID_INVALID, gContentDb.GetDefaultGoldTemplate() );
				SiegePos dropPos;
				Quat dropQuat;
				RetrieveDropPositionAndOrientation( killer, NULL, dropPos, dropQuat );
				cloneReq.SetStartingPos( dropPos );
				cloneReq.SetStartingOrient( dropQuat );
				cloneReq.SetFadeIn();
				cloneReq.SetForceClientAllowed();

				// create the gold and transfer our stuff to it
				GoHandle gold( gGoDb.SCloneGo( cloneReq ) );
				if ( gold )
				{
					gold->GetAspect()->SSetGoldValue( goldSpew, false );
					gold->GetGold()->SetDroppedBy( GetGoid() );

					// all went well, make sure this is not a player (no player class, or a computer player)
					// and then set the item to be a virgin drop !
					if( !GetGo()->GetPlayer() || ( GetGo()->GetPlayer()->IsComputerPlayer() && !GetGo()->HasStore() ) )
					{
						gold->GetAspect()->SetVirginPickup( true );
					}
				}

				totalGold -= goldSpew;
			}
		}

		// always clear out the gold
		RCSetGold( 0 );
	}
}

void GoInventory :: RetrieveDropPositionAndOrientation( Go * dropFor, Go * item, SiegePos & spos, Quat & dropQuat )
{
	int NumDropPoints = 0;
	GoPlacement::UsePointColl pts;

	if (GetGo()->GetPlacement()->HasDropPoints())
	{
		NumDropPoints = GetGo()->GetPlacement()->GetDropPoints( pts );
	}
	else if (GetDropAtUsePoint())
	{
		NumDropPoints = GetGo()->GetPlacement()->GetUsePoints( pts );
	}

	if (NumDropPoints>0)
	{
		int p = Random(0,NumDropPoints-1);
		spos = pts[p];
		gAIQuery.GetRandomOrientation( dropQuat );
	}
	else
	{
		vector_3 center, diag;
		GetGo()->GetAspect()->GetAspectHandle()->GetBoundingBox( center, diag );
		diag.y = 0;
		float minDistance = Length( diag );

		if( dropFor && gAIQuery.FindSpotForDrop(	GetGo(),
													dropFor,
													item,
													minDistance,
													90,
													gWorldOptions.GetInventorySpewVerticalConstraint(),
													spos,
													GetGo()->IsContainer() // Containers drop in same place
												) )
		{
			gAIQuery.GetRandomOrientation( dropQuat );
		}
		else if( gAIQuery.FindSpotRelativeToSource(	GetGo(),
													minDistance + gWorldOptions.GetDropItemsDistanceMin(),
													minDistance + gWorldOptions.GetDropItemsDistanceMax(),
													gWorldOptions.GetInventorySpewVerticalConstraint(),
													spos ) )
		{
			gAIQuery.GetRandomOrientation( dropQuat );
		}
		else
		{
			spos = GetGo()->GetPlacement()->GetPosition();
			gpwarningf(( "Go '%s' couldn't find a good position to drop gold.  Standing in a funny spot?", GetGo()->GetTemplateName() ));
		}
	}
}

void GoInventory :: SSynchronizePartyGold()
{
	CHECK_SERVER_ONLY;

	if ( GetGo()->HasAspect() && !IsAlive( GetGo()->GetAspect()->GetLifeState() ) )
	{
		return;
	}

	Go * pParty = GetGo()->GetParent();
	if ( pParty && pParty->HasParty() )
	{
		GopColl partyColl = pParty->GetChildren();
		GopColl::iterator i;
		for ( i = partyColl.begin(); i != partyColl.end(); ++i )
		{
			(*i)->GetInventory()->SSetGold( GetGold() );
			(*i)->GetInventory()->SetGoldDirty( false );
		}
	}
}

void GoInventory :: SSyncRevivedPartyGold()
{
	CHECK_SERVER_ONLY;

	Go * pParty = GetGo()->GetParent();
	if ( pParty && pParty->HasParty() )
	{
		GopColl partyColl = pParty->GetChildren();
		if ( partyColl.size() != 0 )
		{
			GopColl::iterator i;
			for ( i = partyColl.begin(); i != partyColl.end(); ++i )
			{
				if ( IsAlive( (*i)->GetAspect()->GetLifeState() ) )
				{
					SSetGold( (*i)->GetInventory()->GetGold() );
					SetGoldDirty( false );
				}
			}
		}
	}
}

void GoInventory :: AddDelayedPcontent( void )
{
	gpassert( AssertValid() );

	// do our pcontent
	FastFuelHandle dpcontent = GetData()->GetInternalField( "delayed_pcontent" );
	if ( dpcontent && !dpcontent.IsEmpty() )
	{
		gpassert( !gGoDb.IsCurrentThreadConstructingGo() );
		AddPcontent( dpcontent, GROUP_ALL, true );
	}
}

FuBiCookie GoInventory :: RSCreateAddItem( eInventoryLocation il, const gpstring & templateName )
{
	FUBI_RPC_THIS_CALL_RETRY( RSCreateAddItem, RPC_TO_SERVER );

	GoCloneReq cloneReq;
	cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
	cloneReq.SetForceClientAllowed();
	cloneReq.m_AllClients = GetGo()->IsAllClients();
	Goid goid = gGoDb.SCloneGo( cloneReq, templateName );

	// failed?
	GoHandle go( goid );
	if ( !go )
	{
		return ( RPC_FAILURE_IGNORE );
	}

	return ( Add( go, il, AO_REFLEX, false ) ? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoInventory :: IsSpellInProgress( const gpstring & templateName ) const
{
	GopColl::iterator i		= GetGo()->GetChildBegin();
	GopColl::iterator iend	= GetGo()->GetChildEnd();

	for ( ; i != iend; ++i )
	{
		if ( templateName.same_no_case( (*i)->GetTemplateName() ) )
		{
			return ( true );
		}
	}
	return ( false );
}

Go* GoInventory :: GetSelectedWeapon( void ) const
{
	Go* go = GetSelectedItem();
	if ( (go != NULL) && !go->IsWeapon() )
	{
		go = NULL;
	}
	return ( go );
}

Go* GoInventory :: GetSelectedMeleeWeapon( void ) const
{
	Go* go = GetSelectedWeapon();
	if ( (go != NULL) && !go->GetAttack()->GetIsMelee() )
	{
		go = NULL;
	}
	return ( go );
}

Go* GoInventory :: GetSelectedRangedWeapon( void ) const
{
	Go* go = GetSelectedWeapon();
	if ( (go != NULL) && !go->GetAttack()->GetIsProjectile() )
	{
		go = NULL;
	}
	return ( go );
}

bool GoInventory :: IsMeleeWeaponSelected( void ) const
{
	bool is = false;

	Go* go = GetSelectedWeapon();
	if ( go )
	{
		is = go->GetAttack()->GetIsMelee();
	}

	return ( is );
}

bool GoInventory :: IsRangedWeaponSelected( void ) const
{
	bool is = false;

	Go* go = GetSelectedWeapon();
	if ( go )
	{
		is = go->GetAttack()->GetIsProjectile();
	}

	return ( is );
}

Go* GoInventory :: GetSelectedSpell( void ) const
{
	Go * go = NULL;

	if ( GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||
		 GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
	{
		Go * pSpellbook = GetEquipped( ES_SPELLBOOK );
		if ( pSpellbook )
		{
			go = pSpellbook->GetInventory()->GetSelectedItem();
		}

		if ( (go != NULL) && !go->IsSpell() )
		{
			go = NULL;
		}
	}
	return ( go );
}

bool GoInventory :: IsSpellSelected( void ) const
{
	return ( ( GetSelectedSpell() != NULL ) &&
			 ( GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL || GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL ) );
}

Go* GoInventory :: GetShield( void ) const
{
	// make sure it's actually a shield

	Go* shield = GetEquipped( ES_SHIELD_HAND );
	if ( (shield != NULL) && !shield->IsShield() )
	{
		shield = NULL;
	}
	return ( shield );
}

Go* GoInventory :: GetBestArmor( void ) const
{
	// first try shield hand
	Go* best = GetShield();
	if ( best == NULL )
	{
		// ok, try torso armor
		best = GetEquipped( ES_CHEST );
		if ( best == NULL )
		{
			// what a wuss, tits != defense stupid
			best = GetGo();
		}
	}

	return ( best );
}

void GoInventory :: SSetCustomHead( const gpstring& ch )
{
	CHECK_SERVER_ONLY;
	RCSetCustomHead( ch );
}

void GoInventory :: RCSetCustomHead( const gpstring& ch )
{
	FUBI_RPC_THIS_CALL( RCSetCustomHead, RPC_TO_ALL );
	SetCustomHead( ch );
}


bool GoInventory :: SetCustomHead( const gpstring& ch )
{
	bool ret = true;
	if ( !ch.same_no_case( m_CustomHeadName.GetString() ) )
	{
		m_CustomHeadName = ch;
 		ret = PrivateSetCustomHead( GetGo()->HasInventory() &&
							       !GetGo()->GetInventory()->IsSlotEquipped(ES_HEAD) );
	}
	return ret;
}

void GoInventory :: UpdateCustomHeadTexture( void )
{
	if ( m_CustomHeadAspect.IsValid() )
	{

		DWORD t = m_CustomHeadAspect->GetNumSubTextures();
		DWORD i;

		nema::HAspect aspect = GetGo()->GetAspect()->GetAspectHandle();

		m_CustomHeadAspect->CopyTexture(0,*aspect,0);

		for (i = 1;  i < t; i++)
		{
			gpstring textureFile;
			if ( gNamingKey.BuildIMGLocation( m_CustomHeadAspect->GetDefaultTextureString(i), textureFile ) )
			{
				if (FileSys::DoesResourceFileExist( textureFile ) )
				{
					m_CustomHeadAspect->SetTextureFromFile( i, textureFile );
				}
				else if( gNamingKey.BuildIMGLocation( "b_a_err-badstyle", textureFile ) )
				{
					m_CustomHeadAspect->SetTextureFromFile( i, textureFile );
				}
			}
		}
	}
}

bool GoInventory :: PrivateSetCustomHead( bool force_wear )
{

	// Unload any custom head we are using and then load in the new one

	nema::HAspect aspect = GetGo()->GetAspect()->GetAspectHandle();

	bool wearing_custom = m_CustomHeadAspect.IsValid() &&
						  aspect.IsValid() &&
						  (m_CustomHeadAspect->GetParent() == aspect);

	if ( wearing_custom )
	{
		aspect->DetachChild( m_CustomHeadAspect );
		aspect->ClearInstanceAttrFlags( nema::HIDE_SUBMESH1 );
	}

	// Release the old custom head
	if ( m_CustomHeadAspect.IsValid() )
	{
		gAspectStorage.ReleaseAspect(m_CustomHeadAspect);
		m_CustomHeadAspect = nema::HAspect();
	}

	// Load in a new custom head
	bool ret = false;

	gpstring headFile;

	if (!m_CustomHeadName.empty() && gNamingKey.BuildASPLocation( m_CustomHeadName.GetString(), headFile ) )
	{
		m_CustomHeadAspect = gAspectStorage.LoadAspect( headFile, m_CustomHeadName.GetString() );

		if	(m_CustomHeadAspect.IsValid())
		{
			UpdateCustomHeadTexture();
			m_CustomHeadAspect->SetGoid( GetGoid() );
			ret = true;
		}
		else
		{
			// The new head wasn't valid
			gperrorf(("ERROR: GoInventory() - Attempt was made to load invalid head [%s]",m_CustomHeadName.c_str()));
			ret = false;
		}

	}

	// If we were wearing the old custom head or are required to wear the new one
	// then attach the new version

	if (ret && (wearing_custom || force_wear))
	{
		ret = aspect->AttachDeformableChild( m_CustomHeadAspect );
		if (!aspect->IsPurged(2))
		{
			m_CustomHeadAspect->Reconstitute();
		}
		if (ret)
		{
			aspect->SetInstanceAttrFlags( nema::HIDE_SUBMESH1 );
		}
		else
		{
			m_CustomHeadName = "";
			gAspectStorage.ReleaseAspect(m_CustomHeadAspect);
			m_CustomHeadAspect = nema::HAspect();
		}
	}

	return ret;
}

int GoInventory :: GetInventoryDb( GopColl& storedItems ) const
{
	size_t oldSize = storedItems.size();

	InventoryDb::const_iterator i, ibegin = m_InventoryDb.begin(), iend = m_InventoryDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		storedItems.push_back( i->first );
	}

	return ( storedItems.size() - oldSize );
}

bool GoInventory :: SBeginGive( Goid actor, eActionOrigin /*origin*/ )
{
	CHECK_SERVER_ONLY;

	GoHandle hTrader( actor );
	if ( hTrader->IsInActiveWorldFrustum() )
	{
		gUIShell.SetGridPlace( false );	// Prevent the user from manipulating their gridbox until job is completed.
		return true;
	}

	return false;
}

bool GoInventory :: SAutoGive( Goid actor, Goid item, eActionOrigin origin )
{
	CHECK_SERVER_ONLY;

	gpassert( AssertValid() );

	GoHandle hReceiver( actor );
	GoHandle hItem( item );

	if ( !hItem )
	{
		gpwarningf((	"GoInventory :: SAutoGive - go %s, trying to give an item ( goid 0x%08x ) that no longer exists.",
						GetGo()->GetTemplateName(),
						item ));
		return false;
	}

	// if we're giving something that's equipped
	//
	if( GetEquippedSlot( item ) != ES_NONE )
	{
		RCUnequip( GetEquippedSlot( item ), origin );
	}

	// if it's something just in our inventory
	//
	if( Contains( hItem.Get() ) && !hItem->HasGold() )
	{
		SiegePos dropPos;
		Quat dropQuat;

		if( gAIQuery.FindSpotRelativeToSource(	GetGo(),
												gWorldOptions.GetDropItemsDistanceMin(),
												gWorldOptions.GetDropItemsDistanceMax(),
												gWorldOptions.GetInventorySpewVerticalConstraint(),
												dropPos ) )
		{
			gAIQuery.GetRandomOrientation( dropQuat );
		}
		else
		{
			dropPos = GetGo()->GetPlacement()->GetPosition();
		}

		if( hReceiver->GetInventory()->HasGridbox() && hReceiver->GetInventory()->GetGridbox()->ContainsID( MakeInt(item)) == false )
		{
			if( hReceiver->GetInventory()->CanAdd( hItem.Get(), IL_MAIN, true ) )
			{
				RCRemove( hItem.Get(), dropPos, dropQuat, true );
				hReceiver->GetInventory()->RSAdd( hItem.Get(), IL_MAIN, origin, true );
			}
			else if ( origin == AO_USER )
			{
				gpscreen( $MSG$ "Inventory is full.\n" );
			}
		}
		else
		{
			if( hReceiver->GetInventory()->CanAdd( hItem.Get(), IL_MAIN, false ) )
			{
				RCRemove( hItem.Get(), dropPos, dropQuat, true );
				hReceiver->GetInventory()->RSAdd( hItem.Get(), IL_MAIN, origin, false );
			}
			else if ( origin == AO_USER )
			{
				gpscreen( $MSG$ "Inventory is full.\n" );
			}
		}
	}
	else if ( !hItem->HasGold() )
	{
		gpwarningf((	"GoInventory :: SAutoGive - go %s, scid 0x%08x doesn't have %s to give.",
						GetGo()->GetTemplateName(),
						GetGo()->GetScid(),
						hItem->GetTemplateName() ));
		return false;
	}

	return true;
}

bool GoInventory :: SEndGive( Goid actor, Goid item, eActionOrigin origin )
{
	CHECK_SERVER_ONLY;

	gpassert( AssertValid() );

	GoHandle hReceiver( actor );
	GoHandle hItem( item );

	if( Contains( hItem ) )
	{
		if( !hReceiver->GetInventory()->Contains( hItem.Get() ) && !hItem->HasGold() )
		{
			if( hReceiver->GetInventory()->HasGridbox() )
			{
				if ( HasGridbox() && !GetGridbox()->ContainsID( MakeInt(item) ) )
				{
					hReceiver->GetInventory()->GetGridbox()->RemoveID( MakeInt( item ) );
				}

				if ( CanAutoPlace( hItem ) )
				{
					if ( !Contains( hItem ) )
					{
						Add( hItem, IL_MAIN, origin, HasGridbox() );
					}
					else
					{
						SetLocation( hItem, IL_MAIN, HasGridbox() );
					}
				}
				else
				{
					RSRemove( hItem, true );
				}
			}
		}
	}

	gUIShell.SetGridPlace( true );	// Job's done, user now gets control back.
	UIWindowVec gridboxes = gUIShell.ListWindowsOfType( UI_TYPE_GRIDBOX );
	UIWindowVec::iterator k;
	for ( k = gridboxes.begin(); k != gridboxes.end(); ++k )
	{
		((UIGridbox *)(*k))->SetItemDetect( true );
	}

	return true;
}

void GoInventory :: RSSetForceGet( bool bSet )
{
	FUBI_RPC_TAG();

	if ( GetForceGet() == bSet )
	{
		return;
	}

	FUBI_RPC_THIS_CALL( RSSetForceGet, RPC_TO_SERVER );

	SetForceGet( bSet );

	RCSetForceGet( bSet );
}

void GoInventory :: RCSetForceGet( bool bSet )
{
	FUBI_RPC_THIS_CALL( RCSetForceGet, GetGo()->GetPlayer()->GetMachineId() );

	SetForceGet( bSet );
}

int GoInventory :: GetGridAreaUsed( void ) const
{
	// count up how full we are
	int usage = 0;
	InventoryDb::const_iterator i, ibegin = m_InventoryDb.begin(), iend = m_InventoryDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->second == IL_MAIN && !IsEquipped( i->first ) )
		{
			Go* go = i->first;
			if ( go->HasGui() )
			{
				// subtract out how many squares we're using
				usage += go->GetGui()->GetInventoryWidth() * go->GetGui()->GetInventoryHeight();
			}
		}
	}
	return usage;
}

float GoInventory :: GetFullRatio( void ) const
{
	int size = m_GridWidth * m_GridHeight;
	if ( size == 0 )
	{
		// empty inventory? must be full...
		return ( 1.0f );
	}

	// clamp
	int used = GetGridAreaUsed();
	clamp_max( used, size );

	// return ratio
	return ( (float)used / (float)size );
}

void GoInventory :: ProcessGridboxRequests( void )
{
	if ( GetGo()->IsScreenPartyMember() || GetGo()->HasStash() )
	{
		// early out if it is a stash and it is attempting to assemble the grid on a machine
		// it doesn't need to.
		if ( GetGo()->HasStash() && GetGo()->GetPlayer() != gServer.GetScreenPlayer() )
		{
			m_GridItems.clear();
			return;
		}

		if ( !HasGridbox() )
		{
			SetGridbox( GetGridboxFromGO( GetGo(), 0 ) );
		}

		if ( HasGridbox() && m_GridItems.size() != 0 )
		{
			GridItemColl::iterator i;
			for ( i = m_GridItems.begin(); i != m_GridItems.end(); ++i )
			{
				(*i).rect.left		+= GetGridbox()->GetRect().left;
				(*i).rect.right		+= GetGridbox()->GetRect().left;
				(*i).rect.top		+= GetGridbox()->GetRect().top;
				(*i).rect.bottom	+= GetGridbox()->GetRect().top;

				GetGridbox()->AddGridItem( *i );
			}
			m_GridItems.clear();
		}
	}
}

void GoInventory :: SSetInventoryDirty( bool set )
{
	CHECK_SERVER_ONLY;

	SetInventoryDirty( set ); // set the inventory dirty on the server.
	RCSetInventoryDirty( set ); // set the inventory dirty on the clients machine.
}

void GoInventory :: RCSetInventoryDirty( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetInventoryDirty, GetGo()->GetPlayer()->GetMachineId() );

	SetInventoryDirty( set );
}

void GoInventory :: SetInventoryDirty( bool set )
{
	m_InventoryDirty = set;
	GetGo()->GetPlayer()->SetPartyDirty();
}

void GoInventory :: SetGoldDirty( bool set )
{
	m_GoldPiles.clear();
	m_GoldDirty = set;
}

gpstring GoInventory :: BuildUiName( const Go* item )
{
	GetItemFromGO( (Go *)item, false );
	gpstring icon( item->GetGui()->GetLastInventoryIcon() );
	return ( icon.appendf( "_%d", MakeInt( item->GetGoid() ) ) );
}

void GoInventory :: ReportItemSwitching( Go * pToEquip )
{
	if ( pToEquip )
	{
		gpscreenf( ( $MSG$ "%S switching to %S", GetGo()->GetCommon()->GetScreenName().c_str(),
										 pToEquip->GetCommon()->GetScreenName().c_str()) );
	}
	else
	{
		gpscreenf( ( $MSG$ "%S switching to fists", GetGo()->GetCommon()->GetScreenName().c_str()) );
	}
}

void GoInventory :: UpdateChildrenPositions( void )
{
	const GoPlacement& placement = *GetGo()->GetPlacement();

	InventoryDb::iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		GPDEV_ONLY( i->first->SetPlacementChanging() );
		i->first->GetPlacement()->CopyPosition( placement );
		GPDEV_ONLY( i->first->ClearPlacementChanging() );
		i->first->SetVisit();
	}
}

GoComponent* GoInventory :: Clone( Go* newParent )
{
	return ( new GoInventory( *this, newParent ) );
}

FUBI_DECLARE_POINTERCLASS_TRAITS_EX( nema::HAspect, NemaAspect, nema::HAspect::Null );

bool GoInventory :: Xfer( FuBi::PersistContext& persist )
{
	gpassert( !m_IgnoreEquipMinReqs );

	// spec
	persist.Xfer( "grid_width",   m_GridWidth  );
	persist.Xfer( "grid_height",  m_GridHeight );
	persist.Xfer( "is_pack_only", m_IsPackOnly );

	// other
	persist.Xfer( "selected_active_location", m_SelectedActiveLocation );
	persist.Xfer( "gold",                     m_Gold                   );

	if ( persist.IsRestoring() || !m_CustomHeadName.empty() )
	{
		persist.Xfer( "custom_head",  m_CustomHeadName );
	}

	if (persist.IsFullXfer())
	{
		if (!m_CustomHeadName.empty())
		{
			if ( persist.EnterBlock( "custom_head_data" ) )
			{
				persist.Xfer( "custom_head_aspect", m_CustomHeadAspect );
				if ( persist.IsRestoring() )
				{
					UpdateCustomHeadTexture();
				}
				persist.LeaveBlock();
			}
		}
	}

	// full
	if ( persist.IsFullXfer() )
	{
		// xfer equipment
		gpstring temp;
		if ( persist.IsSaving() )
		{
			Go** i, ** begin = m_Equipped, ** end = ARRAY_END( m_Equipped );
			for ( i = begin ; i != end ; ++i )
			{
				if ( *i != NULL )
				{
					persist.EnterColl( "m_Equipped" );
					for ( ; i != end ; ++i )
					{
						if ( *i != NULL )
						{
							persist.AdvanceCollIter();
							temp = ::ToString( (eEquipSlot)(i - begin) );
							persist.Xfer( "_key", temp );
							persist.Xfer( FuBi::PersistContext::VALUE_STR, *i );
						}
					}
					persist.LeaveColl();

					// always break out of here
					break;
				}
			}
		}
		else
		{
			if ( persist.EnterColl( "m_Equipped" ) )
			{
				while ( persist.AdvanceCollIter() )
				{
					persist.Xfer( "_key", temp );
					eEquipSlot slot;
					if ( ::FromString( temp, slot ) )
					{
						GPDEBUG_ONLY( GetEquipped( slot ) );
						persist.Xfer( FuBi::PersistContext::VALUE_STR, m_Equipped[ slot ] );
					}
				}
			}
			persist.LeaveColl();
		}


		// xfer rest
		persist.XferMap   ( "m_InventoryDb",              m_InventoryDb              );
		persist.Xfer      ( "m_SelectedItem",             m_SelectedItem             );
		persist.Xfer      ( "m_SelectedItemActionOrigin", m_SelectedItemActionOrigin );
		persist.Xfer      ( "m_CurrentStance",            m_CurrentStance            );
		persist.Xfer      ( "m_ActiveSpellBook",          m_ActiveSpellBook          );
		persist.XferVector( "m_GoldPiles",				  m_GoldPiles				 );
		persist.Xfer      ( "m_bForceGet",				  m_bForceGet				 );
	}

	// finish
	if ( persist.IsRestoring() )
	{
		SetInventoryDirty();

		// get our range setup
		m_RangeColl = gContentDb.AddOrFindInvRangeColl( GetData()->GetInternalField( "ranges" ) );
	}

	return ( true );
}

bool GoInventory :: XferCharacter( FuBi::PersistContext& persist )
{
	gpstring           templateName;
	eInventoryLocation invLocation;
	eEquipSlot         equipSlot;
	int                goldValue;
	gpstring           variationName;
	gpstring           modPrefixName;
	gpstring           modSuffixName;
	Goid               ammoGoid;
	Goid               ammoParent;
	gpstring           ammoType;

	// temporarily disable min equip checking (we'll hit this after done loading)
	m_IgnoreEquipMinReqs = true;

	bool bAutoPlace = false;
	int xOffset = 0, yOffset = 0;
	if ( persist.IsSaving() && HasGridbox() )
	{
		persist.Xfer( "grid_x_offset", GetGridbox()->GetRect().left );
		persist.Xfer( "grid_y_offset", GetGridbox()->GetRect().top );
	}
	else
	{
		persist.Xfer( "grid_x_offset", xOffset );
		persist.Xfer( "grid_y_offset", yOffset );

		if ( xOffset == 0 && yOffset == 0 )
		{
			bAutoPlace = true;
		}
	}

	// Xfer this information unless we are a stash
	if ( !GetGo()->HasStash() )
	{
		// traits
		persist.Xfer( "pack_only", m_IsPackOnly, false );		

		// Custom head comes first! 	
		if ( persist.IsRestoring() || !m_CustomHeadName.empty() )
		{
			persist.Xfer( "custom_head", m_CustomHeadName );
		}

		if ( persist.IsRestoring() )
		{
			PrivateSetCustomHead( true );
		}
	
		// what did we have ready?
		persist.Xfer( "selected_active_location", m_SelectedActiveLocation );
	}

	// inventory
	persist.EnterColl( "inventory" );
	if ( persist.IsSaving() )
	{
		InventoryDb::iterator i, ibegin = m_InventoryDb.begin(), iend = m_InventoryDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			persist.AdvanceCollIter();

			Go* child = i->first;

			// save goid on full xfer
			if ( persist.IsFullXfer() )
			{
				Goid goid = child->GetGoid();
				persist.Xfer( "goid", goid );
			}

			// which template
			templateName = child->GetTemplateName();
			persist.Xfer( "template_name", templateName );

			// inventory traits
			invLocation = i->second;
			if ( invLocation != IL_INVALID )
			{
				persist.Xfer( "inv_location", invLocation );
			}
			equipSlot = GetEquippedSlot( child );
			if ( equipSlot != ES_NONE )
			{
				persist.Xfer( "equip_slot", equipSlot );
			}
			goldValue = child->GetAspect()->GetGoldValue();
			if ( goldValue != 0 )
			{
				persist.Xfer( "gold_value", goldValue );
			}

			// variation of item
			variationName = child->GetGui()->GetVariation();
			if ( !variationName.empty() )
			{
				persist.Xfer( "variation_name", variationName );
			}

			// modifier names
			if ( child->HasMagic() )
			{
				modPrefixName = child->GetMagic()->GetPrefixModifierName();
				if ( !modPrefixName.empty() )
				{
					persist.Xfer( "modifier_prefix_name", modPrefixName );
				}
				modSuffixName = child->GetMagic()->GetSuffixModifierName();
				if ( !modSuffixName.empty() )
				{
					persist.Xfer( "modifier_suffix_name", modSuffixName );
				}
			}

			// ammo?
			if ( persist.IsFullXfer() && child->HasAttack() && (child->GetAttack()->GetAmmoReady() != GOID_INVALID) )
			{
				ammoGoid = child->GetAttack()->GetAmmoReady();
				GoHandle ammo( ammoGoid );
				if ( ammo )
				{
					persist.Xfer( "ammo_goid", ammoGoid );
					ammoParent = ammo->HasParent() ? ammo->GetParent()->GetGoid() : NULL;
					persist.Xfer( "ammo_parent", ammoParent );
					ammoType = ammo->GetTemplateName();
					persist.Xfer( "ammo_type", ammoType );
				}
			}

			// spellbook children
			if ( child->GetGui()->IsSpellBook() && !child->GetChildren().empty() )
			{
				persist.EnterBlock( "spellbook" );
				child->GetInventory()->XferCharacter( persist );
				persist.LeaveBlock();
			}

			if ( HasGridbox() )
			{
				GridItem item;
				if ( GetGridbox()->GetItem( MakeInt( child->GetGoid() ), item ) )
				{
					persist.Xfer( "grid_x", item.rect.left );
					persist.Xfer( "grid_y", item.rect.top );
				}
			}
		}
	}
	else
	{
		while ( persist.AdvanceCollIter() )
		{
			// gather info
			persist.Xfer( "template_name",        templateName            );
			persist.Xfer( "inv_location",         invLocation, IL_INVALID );
			persist.Xfer( "equip_slot",           equipSlot,   ES_NONE    );
			persist.Xfer( "gold_value",           goldValue,   0          );
			persist.Xfer( "variation_name",       variationName           );
			persist.Xfer( "modifier_prefix_name", modPrefixName           );
			persist.Xfer( "modifier_suffix_name", modSuffixName           );

			// build pcontent request
			PContentReq req;
			req.SetFinish( variationName );
			if ( !modPrefixName.empty() )
			{
				req.SetModifier( 0, gPContentDb.FindModifier( modPrefixName ) );
			}
			if ( !modSuffixName.empty() )
			{
				req.SetModifier( 1, gPContentDb.FindModifier( modSuffixName ) );
			}

			// on full xfer we create with an explicit goid, not auto-generated
			Goid explicitGoid = GOID_INVALID;
			if ( persist.IsFullXfer() )
			{
				persist.Xfer( "goid", explicitGoid );
			}

			// load it
			Go* child = LoadItem( ToString( invLocation ), templateName, equipSlot, false,
								  !gGoDb.IsConstructingGo(), &req, bAutoPlace, explicitGoid );
			if ( child != NULL )
			{
				// make sure it's available immediately if visible so we can
				// attach stuff to it
				if ( ::IsVisibleEquipSlot( equipSlot ) )
				{
					child->PrepareToDrawNow( false );
				}

				// apply gold value
				child->GetAspect()->SetGoldValue( goldValue );

				// xfer it in if a spell book
				if ( child->GetGui()->IsSpellBook() )
				{
					if ( persist.EnterBlock( "spellbook" ) )
					{
						child->GetInventory()->XferCharacter( persist );
					}
					persist.LeaveBlock();
				}

				// ammo?
				if ( persist.IsFullXfer() && child->HasAttack() )
				{
					persist.Xfer( "ammo_goid",   ammoGoid, GOID_INVALID   );
					persist.Xfer( "ammo_parent", ammoParent, GOID_INVALID );
					persist.Xfer( "ammo_type",   ammoType                 );

					if ( ammoGoid != GOID_INVALID )
					{
						GoCloneReq cloneReq( child->GetGoid() );
						cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
						cloneReq.SetForceClientAllowed();
						cloneReq.m_Parent = ammoParent;
						cloneReq.m_AllClients = GetGo()->IsAllClients();
						cloneReq.m_PrepareAmmo = true;
						gGoDb.CloneGo( ammoGoid, cloneReq, ammoType );
					}
				}

				// Collect the item data to insert into the gridbox when it is available.
				int x = 0, y = 0;
				persist.Xfer( "grid_x", x );
				persist.Xfer( "grid_y", y );
				if ( x != 0 && y != 0 )
				{
					GridItem item;

					// We'll have the game auto-calculate these
					// on an as needed basis
					item.alpha				= 1.0f;
					item.bDrawHighlight		= false;
					item.dwHighlightColor	= 0;
					item.dwSecondaryColor	= 0;
					item.numItems			= 0;
					item.secondary_percent	= 0;

					// these are the important ones
					UIItem * pItem = GetItemFromGO( child, false );

					item.rect.left		=  x - xOffset;
					item.rect.right		= item.rect.left + ( pItem->GetColumns() * pItem->GetBoxWidth() );
					item.rect.top		= y - yOffset;
					item.rect.bottom	= item.rect.top + ( pItem->GetRows() * pItem->GetBoxHeight() );

					item.itemID = MakeInt( child->GetGoid() );

					// All done, add to the grid box queue
					m_GridItems.push_back( item );
				}
			}
		}

		// Fix-up logic: If the party save was done while someone was drinking a potion or something
		// similar, this code is meant to use the given data to reconstruct the character's equipment
		{
			// this will equip a staff if necessary
			if ( GetSelectedLocation() == IL_ACTIVE_PRIMARY_SPELL ||
				 GetSelectedLocation() == IL_ACTIVE_SECONDARY_SPELL )
			{
				Select( GetSelectedLocation(), AO_REFLEX );
			}
			else if ( GetSelectedLocation() == IL_ACTIVE_MELEE_WEAPON ||
					  GetSelectedLocation() == IL_ACTIVE_RANGED_WEAPON )
			{
				Go * pSelected = GetItem( GetSelectedLocation() );
				if ( pSelected )
				{
					AutoEquip( pSelected->GetGui()->GetEquipSlot(),
				    		   pSelected->GetGoid(),
							   AO_REFLEX, false );
				}
			}
		}
	}
	persist.LeaveColl();

	// gold
	if ( (GetGo()->HasGui() && !GetGo()->GetGui()->IsSpellBook()) || GetGo()->HasActor() )
	{
		persist.Xfer( "gold", m_Gold );
	}

	// ready to go again!
	m_IgnoreEquipMinReqs = false;

	// done
	return ( true );
}

FUBI_DECLARE_XFER_TRAITS( PersistGridItem )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		persist.EnterBlock( key );
		persist.Xfer   ( "rect",              obj.rect              );
		persist.Xfer   ( "secondary_percent", obj.secondary_percent );
		persist.Xfer   ( "alpha",             obj.alpha             );
		persist.XferHex( "itemGoid",          obj.itemGoid          );
		persist.Xfer   ( "xoffset",			  obj.xoffset			);
		persist.Xfer   ( "yoffset",			  obj.yoffset			);
		persist.LeaveBlock();

		return ( true );
	}
};

bool GoInventory :: XferGridbox( FuBi::PersistContext& persist )
{
	if ( !HasGridbox() )
	{
		return ( false );
	}

	PersistGridItemColl items;
	bool bLocalPlace = true;
	if ( persist.IsSaving() )
	{
		items = GetGridbox()->PackagePersistGridItems();
		bLocalPlace = GetGridbox()->GetLocalPlace();
	}

	persist.XferVector( "grid_items", items );
	persist.Xfer( "local_place", bLocalPlace );

	if ( persist.IsRestoring() )
	{
		GetGridbox()->AssembleGrid( items );
		GetGridbox()->SetLocalPlace( bLocalPlace );
	}

	return ( true );
}

bool GoInventory :: CommitCreation( void )
{
	// get our range setup
	m_RangeColl = gContentDb.AddOrFindInvRangeColl( GetData()->GetInternalField( "ranges" ) );

	// get our intended active slot for the new go
	eInventoryLocation activeLoc = m_SelectedActiveLocation;

	// clone sources and locals are not allowed to have kids
	if ( GetGoidClass( GetGoid() ) != GO_CLASS_GLOBAL )
	{
		// $ early bailout
		return ( true );
	}

	// standalone, so no inventory
	if ( gGoDb.IsConstructingStandaloneGo() )
	{
		// $ early bailout
		return ( true );
	}

	// This is being created as a child of another go, so we don't want to create it's inventory.
	GlobalGoidBits bits( GetGoid() );
	if ( bits.m_MinorIndex != 0 )
	{
		// $ early bailout
		return ( true );
	}

	// record it
	GPDEV_ONLY( GoRngNR::AutoLog autoLog );

	// Create any custom head
	if ( !m_CustomHeadName.empty() )
	{
		bool force = !GetGo()->HasInventory() || !GetGo()->GetInventory()->IsSlotEquipped( ES_HEAD );
		PrivateSetCustomHead( force );
	}

	// populate and equip our inventory
	FastFuelHandle equipment = GetData()->GetInternalField( "equipment" );

	if ( equipment )
	{
		FastFuelFindHandle fh( equipment );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring equipSlotText, templateText;
			while ( fh.GetNextKeyAndValue( equipSlotText, templateText ) )
			{
				eEquipSlot equipSlot = ES_NONE;
				bool success = false;

				if ( ::FromString( equipSlotText, equipSlot ) && IsEquippableSlot( equipSlot ) )
				{
					success = LoadItem( "", templateText, equipSlot ) != NULL;
				}
				else
				{
					gperrorf(( "Unable to convert to eEquipSlot from value '%s' at '%s'\n",
							   equipSlotText.c_str(), equipment.GetAddress().c_str() ));
				}

				if ( !success )
				{
					gpwarningf((
							"Failed to add item '%s' to equip slot '%s'\n",
							templateText.c_str(), equipSlotText.c_str() ));
				}
			}
		}
	}

	// populate the rest of the inventory
	FastFuelHandle other = GetData()->GetInternalField( "other" );
	if ( other )
	{
		FastFuelFindHandle fh( other );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring ilText, templateText;
			while ( fh.GetNextKeyAndValue( ilText, templateText ) )
			{
				if ( LoadItem( ilText, templateText ) == NULL )
				{
					gpwarningf(( "Failed to add item '%s' to the general inventory\n", templateText.c_str() ));
				}
			}
		}
	}

	// do our pcontent
#	if !GP_RETAIL
	if ( !gGoDb.IsEditMode() )
	{
#	endif // !GP_RETAIL
		if ( ::GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC )
		{
			FastFuelHandle pcontent = GetData()->GetInternalField( "pcontent" );
			if ( pcontent && !pcontent.IsEmpty() )
			{
				AddPcontent( pcontent, GROUP_ALL );
			}
			if ( GetGo()->HasStore() )
			{
				FastFuelHandle storePContent = GetData()->GetInternalField( "store_pcontent" );
				if ( storePContent && !storePContent.IsEmpty() )
				{
					AddStorePcontent( storePContent );
				}
			}
		}
#	if !GP_RETAIL
	}
#	endif // !GP_RETAIL

	// now that we've got the equipment set, figure out our stance
	UpdateAnimStance();

	// now that we've got our inventory, figure out our visual representation
	UpdateAspect();

	// update selection cache
	Select( activeLoc, AO_REFLEX );

	gpassert( AssertValid() );

	// done
	return ( true );
}

void GoInventory :: Shutdown( void )
{
	if ( GetGo()->HasAspect() && GetGo()->GetAspect()->HasAspectHandle() )
	{
		// Detach any custom head we are using
		if ( m_CustomHeadAspect.IsValid() && (m_CustomHeadAspect->GetParent() == GetGo()->GetAspect()->GetAspectHandle() ) )
		{
			GetGo()->GetAspect()->GetAspectHandle()->DetachChild( m_CustomHeadAspect );
		}
	}
	// Release the custom head
	if ( m_CustomHeadAspect.IsValid() )
	{
		gAspectStorage.ReleaseAspect(m_CustomHeadAspect);
		m_CustomHeadAspect = nema::HAspect();
	}

}

inline void CopyFleshTexture(Go* kid, const nema::Aspect& pAsp)
{
	if (kid)
	{
		nema::Aspect* kAsp = kid->GetAspect()->GetAspectPtr();
		if (kAsp && (kAsp->GetNumSubTextures() > 1))
		{
			// You need at least 2 textures in order to have a flesh texture
			kAsp->CopyTexture(0,pAsp,0);
		}
	}
}

void GoInventory :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.GetEvent() == WE_KILLED )
	{
		SDeactivateGuiItems();
		DropItemsFor( msg.GetSendFrom() );
	}
	else if ( msg.GetEvent() == WE_LOST_CONSCIOUSNESS )
	{
		SDeactivateGuiItems();
	}
	else if ( msg.GetEvent() == WE_GAINED_CONSCIOUSNESS )
	{
		if ( GetGo()->IsScreenPartyMember() )
		{
			SSyncRevivedPartyGold();
		}
	}
	else if ( msg.GetEvent() == WE_POST_RESTORE_GAME )
	{
		if (GetGo()->HasAspect())
		{
			// How can I filter ONLY characters WITH custom flesh textures? --biddle
			const nema::Aspect& pAsp = *GetGo()->GetAspect()->GetAspectPtr();
			GoInventory* inv = GetGo()->GetInventory();
			CopyFleshTexture(inv->GetEquipped(ES_CHEST),pAsp);
			CopyFleshTexture(inv->GetEquipped(ES_HEAD),pAsp);
			CopyFleshTexture(inv->GetEquipped(ES_FOREARMS),pAsp);
			CopyFleshTexture(inv->GetEquipped(ES_FEET),pAsp);
		}
	}
	else if ( msg.GetEvent() == WE_ENTERED_WORLD )
	{
		if (GetGo()->HasAspect())
		{
			// Make sure we are in correct stance
			nema::Aspect* pAsp = GetGo()->GetAspect()->GetAspectPtr();
			if (pAsp->GetNextChore() != CHORE_NONE)
			{
				pAsp->SetNextStance(m_CurrentStance);
			}
		}
	}
}

void GoInventory :: Update( float /*deltaTime*/ )
{
	if ( m_InventoryDirty )
	{
		// no longer dirty
		m_InventoryDirty = false;

		// notify party member gui
		if ( GetGo()->IsScreenPartyMember() ||
			 ( GetGo()->HasParent() && GetGo()->GetParent()->IsScreenPartyMember() ))
		{
			gWorld.UpdateInventoryGUICallback( GetGoid() );
		}

		// No need to call these for spellbooks
		if ( !GetGo()->IsSpellBook() )
		{
			// update anim stance
			UpdateAnimStance();

			// reset our aspect depending on load
			UpdateAspect();
		}
	}

	if ( gServer.IsLocal() && m_GoldDirty )
	{
		SSynchronizePartyGold();
		m_GoldDirty = false;
	}
}

void GoInventory :: UnlinkChild( Go* child )
{
	gpassert( child != NULL );

	InventoryDb::iterator found = m_InventoryDb.find( child );
	if ( found != m_InventoryDb.end() )
	{
		// erase item from db
		m_InventoryDb.erase( found );

		// that go no longer is in my inventory
		gpassert( child->IsInsideInventory() );
		child->SetIsInsideInventory( false );

		// reset cache
		ClearSelected( child );

		// special handling for spell books
		if ( m_ActiveSpellBook == child )
		{
			m_ActiveSpellBook = NULL;
		}
		else
		{
			// maybe i'm the spell book and my child spell needs to be deselected from my parent
			Go* parent = GetGo()->GetParent();
			if ( parent != NULL )
			{
				GoInventory* inventory = parent->QueryInventory();
				if ( inventory != NULL )
				{
					inventory->ClearSelected( child );
				}
			}
		}

		// unequip (don't mess with any links)
		eEquipSlot slot = GetEquippedSlot( child );
		if ( slot != ES_NONE )
		{
			PrivateUnequip( slot, false );
			m_Equipped[ slot ] = NULL;
		}

		// unapply any enchantments that may be active if this code was called as the result of dying
		if ( ::IsServerLocal() && (slot != IL_MAIN) )
		{
			GoMagic* magic = child->QueryMagic();
			if ( (magic != NULL) && !magic->GetGo()->IsPotion() )
			{
				gGoDb.RemoveEnchantments( GetGoid(), child->GetGoid(), true );
			}
		}

		// update gold that we've taken
		if ( child->HasGold() )
		{
			SubtractGold( child->GetAspect()->GetGoldValue() );
		}

		// always make visible
		if ( child->HasAspect() )
		{
			child->GetAspect()->SetIsVisible( true );
		}

		// it's changed
		SetInventoryDirty();
		if ( HasGridbox() && !gGoDb.IsShuttingDown() )
		{
			GetGridbox()->RemoveID( MakeInt(child->GetGoid()) );
		}
	}
}

#if !GP_RETAIL

bool GoInventory :: ShouldSaveInternals( void )
{
	// $ this function ain't pretty

	// get template's inventory data
	GoHandle srcGo( gGoDb.FindCloneSource( GetGo()->GetTemplateName() ) );
	const GoDataComponent* srcData = srcGo->GetInventory()->GetData();

	// compare equipment
	{
		Go* srcEquipped[ ES_EQUIP_COUNT ];
		::ZeroObject( srcEquipped );

		FastFuelHandle equipment = srcData->GetInternalField( "equipment" );
		if ( equipment )
		{
			FastFuelFindHandle fh( equipment );
			if( fh.FindFirstKeyAndValue() )
			{
				gpstring equipSlotText, templateText;
				while ( fh.GetNextKeyAndValue( equipSlotText, templateText ) )
				{
					if ( templateText[ 0 ] != '#' )
					{
						eEquipSlot equipSlot = ES_NONE;
						if ( ::FromString( equipSlotText, equipSlot ) && (equipSlot != ES_NONE) )
						{
							srcEquipped[ equipSlot ] = GoHandle( gGoDb.FindCloneSource( templateText ) );
						}
					}
				}
			}
		}

		Go** i, ** begin = m_Equipped, ** end = ARRAY_END( m_Equipped );
		Go** isrc = srcEquipped;
		for ( i = begin ; i != end ; ++i, ++isrc )
		{
			if ( *i != NULL )
			{
				if ( *isrc != NULL )
				{
					if ( (*i)->GetDataTemplate() != (*isrc)->GetDataTemplate() )
					{
						return ( true );
					}
				}
				// $$$ Cheap hack, if the book is the default spellbook, we're going to assume
				// that the book was auto-generated and not save it out.
				else if ( !gContentDb.GetDefaultSpellBookTemplate().same_no_case( (*i)->GetTemplateName() ) )
				{
					return ( true );
				}
			}
			else if ( *isrc != NULL )
			{
				return ( true );
			}
		}
	}

	// compare inventory
	{
		typedef std::multiset <const GoDataTemplate*> Set;
		Set src, me;

		FastFuelHandle other = srcData->GetInternalField( "other" );

		if ( other )
		{
			FastFuelFindHandle fh( other );
			if( fh.FindFirstKeyAndValue() )
			{
				gpstring templateText;
				gpstring key;
				while ( fh.GetNextKeyAndValue( key, templateText ) )
				{
					if ( templateText[0] == '#' )
					{
						continue;
					}

					eInventoryLocation loc;
					::FromString( key, loc );
					if ( key.same_no_case( "il_main" ) )
					{
						src.insert( GoHandle( gGoDb.FindCloneSource( templateText ) )->GetDataTemplate() );
					}
				}
			}
		}

		InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !IsEquipped( i->first ) && !(i->first->IsSpell() && i->first->GetParent() != GetGo()) )
			{
				me.insert( i->first->GetDataTemplate() );
			}
		}

		if ( src != me )
		{
			return ( true );
		}
	}

	return ( false );
}

void GoInventory :: SaveInternals( FuelHandle fuel )
{
	gpassert( AssertValid() );

	// store equipment
	{
		bool first = true;

		FuelHandle fuelEquip;
		Go** i, ** begin = m_Equipped, ** end = ARRAY_END( m_Equipped );
		for ( i = begin ; i != end ; ++i )
		{
			if ( *i != NULL )
			{
				if ( first )
				{
					first = false;
					fuelEquip = fuel->CreateChildBlock( "equipment" );
				}

				fuelEquip->Add( ::ToString( (eEquipSlot)(i - begin) ), (*i)->GetTemplateName() );
			}
		}
	}

	// store inventory
	{
		bool first = true;

		FuelHandle fuelInv;
		InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !IsEquipped( i->first ) )
			{
				if ( first )
				{
					first = false;
					fuelInv = fuel->CreateChildBlock( "other" );
				}

				if ( i->second == IL_INVALID )
				{
					gperrorf( ( "Item: %s in Object: %s's inventory is stored in an invalid inventory slot (IL_INVALID).  This item will be not be saved.", i->first->GetTemplateName(), GetGo()->GetTemplateName() ) );
				}
				else
				{
					fuelInv->Add( ::ToString( i->second ), i->first->GetTemplateName() );
				}
			}
		}
	}
}


void GoInventory :: Dump( bool verbose, ReportSys::Context * ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->OutputF( "info  : width = %d, height = %d", m_GridWidth, m_GridHeight );
	if ( m_IsPackOnly )
	{
		ctx->Output( ", [pack only]" );
	}
	ctx->OutputEol();
	ctx->OutputF( "gold  : %d\n", m_Gold );
	ctx->OutputF( "stance: %s\n", ::ToString( m_CurrentStance ) );

	ctx->OutputF( "selected item   : 0x%08x - %s\n",	GetSelectedItem() ? GetSelectedItem()->GetScid() : 0,
														GetSelectedItem() ? GetSelectedItem()->GetTemplateName() : "na" );

	ctx->OutputF( "selected weapon : 0x%08x - %s\n",	GetSelectedWeapon() ? GetSelectedWeapon()->GetScid() : 0,
														GetSelectedWeapon() ? GetSelectedWeapon()->GetTemplateName() : "na" );

	ctx->OutputF( "selected spell  : 0x%08x - %s\n",	GetSelectedSpell()  ? GetSelectedSpell()->GetScid() : 0 ,
														GetSelectedSpell()  ? GetSelectedSpell()->GetTemplateName() : "na"   );

	DumpDb( verbose, ctx );

	Go * spellBook = GetEquipped( ES_SPELLBOOK );
	if( spellBook )
	{
		ctx->Indent();
		spellBook->GetInventory()->DumpDb( verbose, ctx );
		ctx->Outdent();
	}
}


void GoInventory :: DumpDb( bool /*verbose*/, ReportSys::ContextRef ctx )
{
	ctx->OutputF( "\ninventory for '%s' db: %s\n", GetGo()->GetTemplateName(), m_InventoryDb.empty() ? "EMPTY" : "" );

	InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		ctx->OutputF( "%s : ", ::ToString( i->second ) );

		Go* go = i->first;

		eEquipSlot slot = GetEquippedSlot( go );
		if ( slot != ES_NONE )
		{
			ctx->OutputF( "eES= %s, ", ::ToString( slot ) );
		}

		ctx->OutputF( "'%S', scid= 0x%08x, '%s', goid= 0x%08x, devinstance= '%s'",
				go->GetCommon()->GetScreenName().c_str(),
				go->GetScid(),
				go->GetTemplateName(),
				MakeInt( go->GetGoid() ),
				go->DevGetFuelAddress().c_str() );
		if( go->IsWeapon() || go->IsSpell() )
		{
			float dmin, dmax;
			if( go->IsSpell() )
			{
				dmin = go->GetMagic()->EvaluateAttackDamageModifierMin( GetGo(), NULL );
				dmax = go->GetMagic()->EvaluateAttackDamageModifierMax( GetGo(), NULL );
			}
			else
			{
				gRules.GetDamageRange( GetGo()->GetGoid(), go->GetGoid(), dmin, dmax );
			}
			ctx->OutputF( ", min_damage= %2.2f, max_damage= %2.2f", dmin, dmax );
		}

		float lifeHealing;
		gAIQuery.Is( GetGo(), go, QT_LIFE_HEALING, lifeHealing );

		float lifeDamaging;
		gAIQuery.Is( GetGo(), go, QT_LIFE_DAMAGING, lifeDamaging );

		float manaHealing;
		gAIQuery.Is( GetGo(), go, QT_MANA_HEALING, manaHealing );

		float manaDamaging;
		gAIQuery.Is( GetGo(), go, QT_MANA_DAMAGING, manaDamaging );

		ctx->OutputF( ", aiq= lh=%2.2f, ld=%2.2f, mh=%2.2f, md=%2.2f", lifeHealing, lifeDamaging, manaHealing, manaDamaging );

		if ( go->HasAspect() )
		{
			ctx->OutputF( ", goldValue= %d", go->GetAspect()->GetGoldValue() );
		}

		if ( go->HasGui() )
		{
			ctx->OutputF( ", item_power= %f", go->GetGui()->GetItemPower() );
		}

		if ( go->HasMagic() )
		{
			if ( !go->GetMagic()->GetPrefixModifierName().empty() )
			{
				ctx->OutputF( ", prefix= %s", go->GetMagic()->GetPrefixModifierName().c_str() );
			}
			if ( !go->GetMagic()->GetSuffixModifierName().empty() )
			{
				ctx->OutputF( ", suffix= %s", go->GetMagic()->GetSuffixModifierName().c_str() );
			}
		}

/*
		if( verbose )
		{
			gpwstring tip = go->GetGui()->MakeToolTipString();
			ctx->OutputF( "tooltip= {%S}\n", tip.c_str() );
			ctx->Output(  "-------------------------------\n" );
		}
*/

		ctx->OutputEol();
	}
}


#endif // !GP_RETAIL


#if GP_DEBUG

bool GoInventory :: AssertValid( void ) const
{
	Inherited::AssertValid();

	// check random
	gpassert( m_GridWidth > 0 );
	gpassert( m_GridHeight > 0 );
	gpassert( m_Gold >= 0 );
	gpassert( (m_SelectedItem == NULL) || (m_SelectedItem != GetEquipped( ES_SPELLBOOK )) );
	gpassert( (m_ActiveSpellBook == NULL) || Contains( m_ActiveSpellBook ) );
	gpassert( ( !IsMeleeWeaponEquipped() && !IsRangedWeaponEquipped() ) || ( IsMeleeWeaponEquipped() && !IsRangedWeaponEquipped() ) || ( IsRangedWeaponEquipped() && !IsMeleeWeaponEquipped() ) );

	// check inventory relationships
	{
		InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			gpassert( i->first->GetParent() == GetGo() );
			gpassert( GetGo()->HasChild( i->first ) );
		}
	}

	// check equipment
	{
		const CGo* i, * begin = m_Equipped, * end = ARRAY_END( m_Equipped );
		for ( i = begin ; i != end ; ++i )
		{
			gpassert( (*i == NULL) || Contains( *i ) );
			if( *i )
			{
				gpassert( GetGo()->HasChild( *i ) );
			}
		}
	}

	// check selected item
	{
		if ( m_SelectedItem != NULL )
		{
			if ( Contains( m_SelectedItem ) )
			{
				gpassert( GetGo()->IsSpellBook() || !m_SelectedItem->IsSpell() );
			}
			else
			{
				Go* spellBook = GetEquipped( ES_SPELLBOOK );
				if_gpassert( spellBook != NULL )
				{
					gpassert( m_SelectedItem->GetParent() == spellBook );
					gpassert( m_SelectedItem->IsSpell() );
					gpassert( m_SelectedItem == GetSelectedSpell() );
				}
			}
		}
	}

	// done
	return ( true );
}

#endif // GP_DEBUG

void GoInventory :: UpdateAnimStance( void )
{
	// $$$: [PRE-NWO LEGACY COMMENT]
	//
	//		this is really really really not the way we should look up the
	//		stance. at the very least, the weapon definition should store the
	//		stance that it needs to look 'right'.
	//
	//		if the 'common_name' is really the 'anim_required' then lets call it
	//		that. does the ai really need to know what stance is being used?
	//		just to tell the  choreographer? should we let the choreographer
	//		keep track of the stance? ai could query the choreographer for valid
	//		attack modes. ai would not have to be rebuilt every time there is a
	//		modification of animations.
	//
	//		-- biddle

	// default
	m_CurrentStance = AS_PLAIN;

	// get components
	Go* weapon = GetEquipped( ES_WEAPON_HAND );
	Go* shield = GetEquipped( ES_SHIELD_HAND );

	if ( shield != NULL )
	{
		if ( shield->HasDefend() && (shield->GetDefend()->GetDefendClass() == DC_SHIELD) )
		{
			if (weapon == NULL)
			{
				m_CurrentStance = AS_SHIELD_ONLY;
			}
			else
			{
				m_CurrentStance = AS_SINGLE_MELEE_AND_SHIELD;
			}
		}
		else if ( shield->HasAttack() && (shield->GetAttack()->GetAttackClass() == AC_BOW) )
		{
			m_CurrentStance = AS_BOW_AND_ARROW;
		}
	}
	else if ( (weapon != NULL) && weapon->HasAttack() )
	{
		switch ( weapon->GetAttack()->GetAttackClass() )
		{
			case ( AC_STAFF ):
			{
				m_CurrentStance = AS_STAFF;
			}
			break;

			case ( AC_MINIGUN ):
			{
				m_CurrentStance = AS_MINIGUN;
			}
			break;

			default:
			{
				// default attack
				if ( weapon->GetAttack()->GetIsTwoHanded() )
				{
					if (weapon->GetAttack()->GetAttackClass() == AC_SWORD)
					{
						m_CurrentStance = AS_TWO_HANDED_SWORD;
					}
					else
					{
						m_CurrentStance = AS_TWO_HANDED_MELEE;
					}
				}
				else
				{
					m_CurrentStance = AS_SINGLE_MELEE;
				}
			}
		}
	}
}

void GoInventory :: UpdateAspect( void )
{
	// $ early bailout if no specific representation
	if ( !m_RangeColl || m_RangeColl->empty() )
	{
		return;
	}

	// now figure out which aspect to use
	float ratio = GetFullRatio();

	// find a good aspect/texture combo to use
	GoInvRangeColl::const_iterator i, begin = m_RangeColl->begin(), end = m_RangeColl->end() - 1;
	for ( i = begin ; i != end ; ++i )
	{
		if ( ratio <= i->m_MaxRatio )
		{
			break;
		}
	}

	// swap out the aspect
	GetGo()->GetAspect()->SetCurrentAspectSimple( i->m_Model.c_str(), i->m_Texture.c_str() );
}

int GoInventory :: QueryItems( eInventoryLocation il, GopColl* items ) const
{
	int count = 0;

	if ( il == IL_ALL )
	{
		if ( items != NULL )
		{
			InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
			for ( i = begin ; i != end ; ++i )
			{
				items->push_back( i->first );
			}
		}
		count = m_InventoryDb.size();
	}
	else
	{
		InventoryDb::const_iterator i, begin = m_InventoryDb.begin(), end = m_InventoryDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( il == i->second )
			{
				if ( items != NULL )
				{
					items->push_back( i	->first );
				}
				++count;
			}
			else if ( il == IL_ALL_ACTIVE )
			{
				if ( IsActiveSlot( i->second ) )
				{
					if ( items != NULL )
					{
						items->push_back( i	->first );
					}
					++count;
				}
			}
			else if ( il == IL_ALL_SPELLS )
			{
				if ( IsSpellSlot( i->second ) )
				{
					if ( items != NULL )
					{
						items->push_back( i	->first );
					}
					++count;
				}
			}
		}

	}

	return ( count );
}

bool GoInventory :: AutoPlace( Go* item )
{
	// Handled case - gridbox already contains item
	if ( m_GridBox != NULL && m_GridBox->ContainsID( MakeInt(item->GetGoid()) ) )
	{
		return ( true );
	}

	if ( (m_GridBox != NULL) && !m_GridBox->AutoItemPlacement( BuildUiName( item ), true, MakeInt( item->GetGoid() ) ) )
	{
		return ( false );
	}

	SetInventoryDirty();

	// $$ should this sound be played here? -bk
	// $$ no it shouldn't, the gui should handle this itself -sb
	// $$$ HACKA HACKA HACKA - I put the world state check in here until our sounddb lovin' is hooked up - cq
	if ( GetGo()->GetParent() == gServer.GetScreenParty() &&
		gWorldState.GetCurrentState() != WS_WAIT_FOR_BEGIN &&
		!IsLoading( gWorldState.GetCurrentState() ) )
	{
		item->PlayVoiceSound( "pick_up" );
	}

	return ( true );
}

bool GoInventory :: CanAutoPlace( const Go* item ) const
{
	// We want to handle the case where the id is already in the grid for autolooting as well.
	if ( m_GridBox != NULL && !m_GridBox->ContainsID( MakeInt( item->GetGoid() ) ) )
	{
		return ( m_GridBox->AutoItemPlacement( BuildUiName( item ), false ) );
	}
	else
	{
		return ( true );
	}
}

bool GoInventory :: PrivateEquip( eEquipSlot slot, Go* go )
{
	gpassert( go != NULL );

	// $ early bailout
	if ( !IsVisibleEquipSlot( slot ) )
	{
		// not visible, no need to change anything wrt the mesh
		return ( true );
	}

	bool success = false;

	// check for full suit
	// $$$ TODO -- will want more control over the channel/bones that
	//     get mapped to each other -biddle

	// get nema aspects
	nema::HAspect parentAspect = GetGo()->GetAspect()->GetAspectHandle();
	nema::HAspect childAspect = go->GetAspect()->GetAspectHandle();

	if ( IsArmorEquipSlot( slot ) )
	{
		if ( ( slot == ES_FEET ) || ( slot == ES_FOREARMS ) )
		{
			// Need to know what is on the chest/body in order to instantiate
			// the correct mesh for the feet & hands -- biddle

			if ( IsSlotEquipped( ES_CHEST ) )
			{
				success = GetGo()->GetBody()->InstantiateDeformableArmor(
					go , GetEquipped(ES_CHEST)->GetDefend()->GetArmorType() ) ;
			}
			else
			{
				success = GetGo()->GetBody()->InstantiateDeformableArmor( go , "a1" ) ;
			}
		}
		else
		{
			success = GetGo()->GetBody()->InstantiateDeformableArmor( go ) ;
		}

		if ( success )
		{
			childAspect = go->GetAspect()->GetAspectHandle();

			if ( ( slot == ES_CHEST ) )
			{
				if ( IsSlotEquipped( ES_FEET ) )
				{
					Go* boots = GetEquipped(ES_FEET);
					nema::HAspect trueparent = boots->GetAspect()->GetAspectHandle()->GetParent();
					trueparent->DetachChild( boots->GetAspect()->GetAspectHandle() );
					boots->GetAspect()->RestoreNativeAspect();
					GetGo()->GetBody()->InstantiateDeformableArmor(boots,go->GetDefend()->GetArmorType());
					trueparent->AttachDeformableChild( boots->GetAspect()->GetAspectHandle() );
				}

				if ( IsSlotEquipped( ES_FOREARMS ) )
				{
					Go* gaunts = GetEquipped(ES_FOREARMS);
					nema::HAspect trueparent = gaunts->GetAspect()->GetAspectHandle()->GetParent();
					trueparent->DetachChild( gaunts->GetAspect()->GetAspectHandle() );
					gaunts->GetAspect()->RestoreNativeAspect();
					GetGo()->GetBody()->InstantiateDeformableArmor(gaunts,go->GetDefend()->GetArmorType());
					trueparent->AttachDeformableChild( gaunts->GetAspect()->GetAspectHandle() );
				}

				GetGo()->GetAspect()->SetCurrentAspect(childAspect,true);
				gAspectStorage.AddRefAspect(childAspect);
				success = true;

			}
			else
			{
				success = parentAspect->AttachDeformableChild( childAspect );

				// hide the sub mesh that this equipment covers
				if ( success )
				{
					switch ( slot )
					{
						case ( ES_FEET ):
						{
							if ( parentAspect->GetNumMeshes() < 4 )
							{
								gpwarningf(( "Error: attempting to hide submesh 3 but aspect '%s' only has %d submeshes!\n",
											 parentAspect->GetDebugName(), parentAspect->GetNumMeshes() ));
							}
							parentAspect->SetInstanceAttrFlags( nema::HIDE_SUBMESH3 );
						}
						break;

						case ( ES_SHIELD_HAND ):
						case ( ES_WEAPON_HAND ):
						case ( ES_FOREARMS ):
						{
							if ( parentAspect->GetNumMeshes() < 3 )
							{
								gpwarningf(( "Error: attempting to hide submesh 3 but aspect '%s' only has %d submeshes!\n",
											 parentAspect->GetDebugName(), parentAspect->GetNumMeshes() ));
							}
							parentAspect->SetInstanceAttrFlags( nema::HIDE_SUBMESH2 );
						}
						break;

						case ( ES_HEAD ):
						{
							bool has_custom_head = GetCustomHeadAspect() &&
												   (GetCustomHeadAspect() != childAspect) &&
												   parentAspect->DetachChild( GetCustomHeadAspect() );
							if ( !has_custom_head )
							{
								if ( parentAspect->GetNumMeshes() < 2 )
								{
									gpwarningf(( "Error: attempting to hide submesh 3 but aspect '%s' only has %d submeshes!\n",
												 parentAspect->GetDebugName(), parentAspect->GetNumMeshes() ));
								}
								parentAspect->SetInstanceAttrFlags( nema::HIDE_SUBMESH1 );
							}
						}
						break;
					}
				}
			}
		}
	}
	else if (( slot == ES_WEAPON_HAND ) || ( slot == ES_SHIELD_HAND ))
	{

		if ( slot == ES_WEAPON_HAND )
		{
			// May not be a weapon at all, might be a potion

			bool child_has_weapon_tracers = false;

			if ( go->IsWeapon() && ToPContentType(go->GetAttack()->GetAttackClass()) == PT_MELEE)
			{
				child_has_weapon_tracers = true;
			}

			if ( go->HasAttack() &&
				 go->GetAttack()->GetIsTwoHanded() &&
				 go->GetAttack()->GetAttackClass() != AC_SWORD &&
				 go->GetAttack()->GetAttackClass() != AC_STAFF &&
				 go->GetAttack()->GetAttackClass() != AC_MINIGUN
				)
			{
				// Two handed melee weapons are actually attached to shield hand --biddle
				success = parentAspect->AttachRigidLinkedChild( childAspect, "SHIELD_GRIP", "GRIP", child_has_weapon_tracers);
			}
			else
			{
				success = parentAspect->AttachRigidLinkedChild( childAspect, "WEAPON_GRIP", "GRIP", child_has_weapon_tracers);
			}
		}
		else if ( slot == ES_SHIELD_HAND )
		{
			success = parentAspect->AttachRigidLinkedChild( childAspect, "SHIELD_GRIP", "GRIP" );
		}


	}
	else
	{
		success = parentAspect->AttachDeformableChild( childAspect );
	}

	// postprocess
	if ( success )
	{
		// (body armour gets handled in the code that transfers the internals )
		// inherit visibility from parent -biddle #11564
		go->GetAspect()->SetIsVisible( (slot != ES_CHEST) && GetGo()->GetAspect()->GetIsVisible() );

	}

	// done
	return ( success );
}

void GoInventory :: PrivateUnequip( eEquipSlot slot, bool unlink )
{
	gpassert( m_Equipped[ slot ] != NULL );

	// $ early bailout
	if ( !IsVisibleEquipSlot( slot ) )
	{
		// not visible, no need to change anything wrt the mesh
		return;
	}

	// get the go
	Go* go = m_Equipped[ slot ];
	gpassert( go != NULL );
	
	// If we can leave the armor meshes alone when we know it will fade, 
	// We can avoid the pop to the god pose in the fader -- biddle
	if ( slot == ES_CHEST && GetEquipped( ES_CHEST ) && GetEquipped( ES_CHEST )->IsMarkedForDeletion() )
	{
		return;
	}	

	// unhide the sub mesh that this equipment was covering
	if ( (slot == ES_CHEST) )
	{
		if ( IsSlotEquipped( ES_FEET ) )
		{
			Go* boots = GetEquipped(ES_FEET);

			nema::eAttachMethods attach_type = boots->GetAspect()->GetAspectHandle()->GetAttachType();
			if ( attach_type != nema::UNATTACHED )
			{
				nema::HAspect trueparent = boots->GetAspect()->GetAspectHandle()->GetParent();
				trueparent->DetachChild( boots->GetAspect()->GetAspectHandle() );
				boots->GetAspect()->RestoreNativeAspect();
				GetGo()->GetBody()->InstantiateDeformableArmor(boots,"a1");
				trueparent->AttachDeformableChild( boots->GetAspect()->GetAspectHandle() );
			}
			else
			{
				if ( !GetGo()->IsMarkedForDeletion()  )
				{
					gperrorf(("The BOOTS equipped by [%s:0x%08x] aren't attached to any nema parent anymore",GetGo()->GetTemplateName(),GetGo()->GetScid()));
				}
				boots->GetAspect()->RestoreNativeAspect();
				GetGo()->GetBody()->InstantiateDeformableArmor(boots,"a1");
			}
		}

		if ( IsSlotEquipped( ES_FOREARMS ) )
		{
			Go* gaunts = GetEquipped(ES_FOREARMS);

			nema::eAttachMethods attach_type = gaunts->GetAspect()->GetAspectHandle()->GetAttachType();

			if ( attach_type != nema::UNATTACHED )
			{
				nema::HAspect trueparent = gaunts->GetAspect()->GetAspectHandle()->GetParent();
				trueparent->DetachChild( gaunts->GetAspect()->GetAspectHandle() );
				gaunts->GetAspect()->RestoreNativeAspect();
				GetGo()->GetBody()->InstantiateDeformableArmor(gaunts,"a1");
				trueparent->AttachDeformableChild( gaunts->GetAspect()->GetAspectHandle() );
			}
			else
			{
				if ( !GetGo()->IsMarkedForDeletion()  )
				{
					gperrorf(("The GAUNTLETS equipped by [%s:0x%08x] aren't attached to any nema parent anymore",GetGo()->GetTemplateName(),GetGo()->GetScid()));
				}
				gaunts->GetAspect()->RestoreNativeAspect();
				GetGo()->GetBody()->InstantiateDeformableArmor(gaunts,"a1");
			}
		}

		GetGo()->GetAspect()->RestoreNativeAspect( true );
		go->GetAspect()->RestoreNativeAspect( false );

		if ( unlink )
		{
			// always set it invisible
			go->GetAspect()->SetIsVisible( false );
		}
	}
	else
	{
		// only do these if we're detaching (i.e. not during destruction)
		if ( unlink )
		{
			// always set it invisible
			go->GetAspect()->SetIsVisible( false );

			// tell nema to detach the child
			go->GetAspect()->ClearNemaParent();
		}

		// get nema aspects
		nema::HAspect parentAspect = GetGo()->GetAspect()->GetAspectHandle();
		nema::HAspect childAspect = go->GetAspect()->GetAspectHandle();

		// restore the scale
		childAspect->SetCurrentScale( vector_3::ONE );

		// unequip it
		if ( IsArmorEquipSlot( slot ) )
		{
			go->GetAspect()->RestoreNativeAspect();
			switch ( slot )
			{
				case ( ES_FEET ):
				{
					parentAspect->ClearInstanceAttrFlags( nema::HIDE_SUBMESH3 );
					parentAspect->ForceDeformation();
				}
				break;

				case ( ES_SHIELD_HAND ):
				case ( ES_WEAPON_HAND ):
				case ( ES_FOREARMS ):
				{
					parentAspect->ClearInstanceAttrFlags( nema::HIDE_SUBMESH2 );
					parentAspect->ForceDeformation();
				}
				break;

				case ( ES_HEAD ):
				{
					// If we're being deleted, let's not do all this work, may confuse nema.
					if ( GetGo()->IsMarkedForDeletion() )
					{
						break;
					}

					// Attaching a deformable child will force the deformation
					bool has_custom_head = false;
					if (GetCustomHeadAspect())
					{
						if ( IsValid( parentAspect->GetGoid() ) )
						{
							has_custom_head = parentAspect->AttachDeformableChild( GetCustomHeadAspect() );
						}
						else
						{
							has_custom_head = false;
						}
					}

					if ( has_custom_head )
					{
						GetCustomHeadAspect()->MatchPurgeStatus(parentAspect.Get(),GetGo()->IsInAnyScreenWorldFrustum());
					}
					else
					{
						parentAspect->ClearInstanceAttrFlags( nema::HIDE_SUBMESH1 );
						parentAspect->ForceDeformation();
					}
				}
				break;
			}
		}

	}
}

struct Entry
{
	gpstring           m_Name;		// name of entry (key - slot, group name, etc.)
	gpstring           m_Value;		// value of entry (template name)
	FastFuelHandle     m_Fuel;		// fuel address if a subgroup
	float              m_Chance;	// chance (defaults to 0 if unassigned, to be recalced later)
	eEquipSlot         m_Slot;		// slot we're in if any
	eInventoryLocation m_Iloc;		// inventory location to use if any

	Entry( void ) : m_Chance( 0 ), m_Slot( ES_NONE ), m_Iloc( IL_INVALID )  {  }
};

void GoInventory :: ClearSelected( Go* child )
{
	gpassert( child != NULL );

	if ( m_SelectedItem == child )
	{
		m_SelectedItem = NULL;
	}
	else if ( m_SelectedItem != NULL )
	{
		// maybe it's in the spell book...
		Go* spellBook = GetEquipped( ES_SPELLBOOK );
		if ( (child == spellBook) && (m_SelectedItem->GetParent() == spellBook) )
		{
			m_SelectedItem = NULL;
		}
	}
}

static int calcMinMax( Go* forGo, const GoDataTemplate* godt, float minCount, float maxCount )
{
	gppcontentf(( "Calcing min/max for Go '%s' using incoming minCount = %g, maxCount = %g\n",
				  forGo ? forGo->GetTemplateName() : "<NONE>", minCount, maxCount ));
	GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

	// now that we have an item, use it to determine the multiplier to use for
	// min/max calc if we haven't done so already
	if ( godt != NULL )
	{
		// get the type from the item
		PContentDb::ItemTraits traits;
		if (   gPContentDb.QueryItemTraits( traits, godt, PT_INVALID, true, false, true )
			&& ((traits.m_SpecificType != PT_INVALID) || (traits.m_AttackClass != AC_BEASTFU)) )
		{
			gppcontentf(( "Item traits: specificType = %s, attackClass = %s\n",
						  ::ToString( traits.m_SpecificType ), ::ToString( traits.m_AttackClass ) ));

			// look up the entry
			float minMult, maxMult;
			if ( gPContentDb.GetInvMpMinMaxMultipliers(
					forGo, traits.m_SpecificType, traits.m_AttackClass, minMult, maxMult ) )
			{
				// multiply the min/max
				float localMinCount = minCount * minMult;
				float localMaxCount = maxCount * maxMult;
				swap_if( localMinCount, localMaxCount );
				int count = GetGoCreateRng().Random( (int)localMinCount, Round( localMaxCount ) );
				gppcontentf(( "Calc'd new count %d from minCount = %g, maxCount = %g using MP loot-scaled traits\n",
							  count, localMinCount, localMaxCount ));
				return ( count );
			}
		}
	}

	// just use default min/max if not set already
	int count = GetGoCreateRng().Random( (int)minCount, Round( maxCount ) );
	gppcontentf(( "Calc'd new count %d from minCount = %g (%d), maxCount = %g (%d) using defaults\n",
				  count, minCount, (int)minCount, maxCount, Round( maxCount ) ));
	return ( count );
}

void GoInventory :: AddPcontent( FastFuelHandle group, eGroup type, bool isStandaloneLoad, bool isRootQuery )
{
	gpassert( group );

	if ( isRootQuery )
	{
		// ok god DAMMIT i'm just going to @$&&%@# set the @&*$&*!# FPU mode 
		// every GOD DAMN QUERY and maybe THEN my !*$&*&$ hell @$%*&&% bugs
		// will never come back. see 11777, 11933, 12111, 11726. -sb
		SetFpuChopMode();

		gppcontentf(( "\n\nAdding PContent inventory for goid = 0x%08X, scid = 0x%08X, template = '%s', spec is at '%s'\n\n",
					  GetGoid(), GetGo()->GetScid(), GetGo()->GetTemplateName(), group.GetAddress().c_str() ));
	}

	// this function is called when a group has been selected and we are to
	// add items from it into the inventory.

	if ( type == GROUP_DETECT )
	{
		gppcontentf(( "Detecting group type from '%s'\n", group.GetName() ));

		if ( same_no_case( group.GetName(), "all*" ) )
		{
			type = GROUP_ALL;
		}
		else if ( same_no_case( group.GetName(), "oneof*" ) )
		{
			type = GROUP_ONEOF;
		}
		else if ( same_no_case( group.GetName(), "gold*" ) )
		{
			type = GROUP_GOLD;
		}
		else
		{
			// $ early bailout

			gperrorf(( "Illegal subgroup type '%s' at '%s'\n",
					   group.GetName(), group.GetAddress().c_str() ));
			return;
		}
	}

	float minCount = group.GetFloat( "min", 1 );
	float maxCount = group.GetFloat( "max", 1 );
	if ( maxCount < minCount )
	{
		gperrorf(( "Max %g is less than min %g found at '%s'\n",
				   maxCount, minCount, group.GetAddress().c_str() ));
		std::swap( minCount, maxCount );
	}

	float allMultiplier   = gPContentDb.GetInvMpAllChanceMultiplier  ( GetGo() );
	float oneOfMultiplier = gPContentDb.GetInvMpOneOfChanceMultiplier( GetGo() );

	gppcontentf(( "min = %g, max = %g, allMultiplier = %g, oneOfMultiplier = %g\n",
				  minCount, maxCount, allMultiplier, oneOfMultiplier ));

	if ( type == GROUP_GOLD )
	{
		gppcontent( "GROUP_GOLD: Generating 'gold*'\n" );
		GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

		// get our multipliers
		float minMult, maxMult;
		gPContentDb.GetInvMpGoldMinMaxMultipliers( GetGo(), minMult, maxMult );
		minCount *= minMult;
		maxCount *= maxMult;
		swap_if( minCount, maxCount );

		// add gold!
		int newGold = GetGoCreateRng().Random( (int)minCount, Round( maxCount ) );
		m_Gold += newGold;
		gppcontentf(( "Gold amount %d (total is now %d) chosen from (post-mult) min = %g, max = %g, minMult = %g, maxMult = %g\n",
					  newGold, m_Gold, minCount, maxCount, minMult, maxMult ));

		// how many piles? use # or #-# style grammar only.
		gpstring piles = group.GetString( "piles" );
		if ( !piles.empty() )
		{
			gppcontentf(( "Dividing into piles based on spec '%s'\n", piles.c_str() ));

			int pileCount = 1, minPiles = 1, maxPiles = 1;
			bool success = true;

			stringtool::Extractor extractor( "-", piles );
			if ( extractor.GetNextInt( minPiles, &success ) )
			{
				::maximize( minPiles, 1 );

				if ( extractor.GetNextInt( maxPiles, &success ) )
				{
					::maximize( maxPiles, minPiles );
					pileCount = GetGoCreateRng().Random( minPiles, maxPiles );
				}
				else if ( success )
				{
					pileCount = maxPiles = minPiles;
				}
				else
				{
					gperrorf(( "Badly formatted max-piles parameter found at '%s': '%s'\n",
							   group.GetAddress().c_str(), piles.c_str() ));
				}

				// only bother if we have more than one pile
				if ( pileCount > 1 )
				{
					// pile size ranges
					int minPileSize = ::max_t( 1, group.GetInt( "min_pile" ) );
					float variance = 2.0f - ((float)minPiles / maxPiles);
					float maxBase = (float)newGold / pileCount;
					int maxPileSize = ::max_t( minPileSize, (int)(maxBase * variance) );
					gppcontentf(( "Adding piles, count = %d, minPileSize = %d, variance = %g, maxBase = %f, maxPileSize = %d\n",
								  pileCount, minPileSize, variance, maxBase, maxPileSize ));
					GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

					// just add piles until we run out of gold
					while ( newGold > 0 )
					{
						int pileSize = newGold;
						if ( (int)m_GoldPiles.size() < (maxPiles - 1) )
						{
							pileSize = ::min( newGold, GetGoCreateRng().Random( minPileSize, maxPileSize ) );
						}

						if ( (pileSize >= minPileSize) || m_GoldPiles.empty() )
						{
							m_GoldPiles.push_back( pileSize );
							gppcontentf(( "Adding pile of size %d\n", pileSize ));
						}
						else
						{
							// special case if below the min size
							m_GoldPiles.back() += pileSize;
							gppcontentf(( "Extending back pile by size %d to %d\n", pileSize, m_GoldPiles.back() ));
						}
						newGold -= pileSize;
					}
				}
			}

			if ( !success )
			{
				gperrorf(( "Illegal piles parameter found at '%s': '%s'\n",
						   group.GetAddress().c_str(), piles.c_str() ));
			}
		}
	}
	else
	{
#		if !GP_RETAIL
		if ( minCount > 250 )
		{
			gperrorf(( "Min %g is HUUUUUUGE for items, please fix (found at '%s')\n",
					   minCount, group.GetAddress().c_str() ));
		}
		if ( maxCount > 250 )
		{
			gperrorf(( "Max %g is HUUUUUUGE for items, please fix (found at '%s')\n",
					   maxCount, group.GetAddress().c_str() ));
		}
#		endif // !GP_RETAIL

		switch ( type )
		{

		// "all*"

			case ( GROUP_ALL ):
			{
				gppcontent( "GROUP_ALL: Generating 'all*' items\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				FastFuelFindHandle fh( group );

				bool found = fh.FindFirstKeyAndValue();
				if ( found )
				{
					// $ for each entry in the block, figure out its min/max and
					//   create that many items of that type.

					gpstring key, value;
					while ( fh.GetNextKeyAndValue( key, value ) )
					{
						bool first = true;
						int count = 1;

						do
						{
							eEquipSlot slot = ES_NONE;
							eInventoryLocation iloc = IL_INVALID;
							Go* item = NULL;
							bool success = true;

							if ( ::FromString( key, slot ) && IsEquippableSlot( slot ) )
							{
								if ( (slot != ES_RING) && IsSlotEquipped( slot ) )
								{
									slot = ES_NONE;
								}

								gppcontentf(( "Loading new item '%s' into slot %s\n", value.c_str(), ::ToString( slot ) ));
								success = (item = LoadItem( "", value, slot, true, isStandaloneLoad )) != NULL;
							}
							else if ( ::FromString( key, iloc ) && IsAddableSlot( iloc ) )
							{
								gppcontentf(( "Loading new item '%s' into location %s\n", value.c_str(), ::ToString( iloc ) ));
								success = (item = LoadItem( key, value, ES_NONE, true, isStandaloneLoad )) != NULL;
							}
							else if (   !key.same_no_case( "min"    )
									 && !key.same_no_case( "max"    )
									 && !key.same_no_case( "chance" ) )
							{
								gperrorf(( "Unable to convert to eEquipSlot or eInventoryLocation from value '%s' at '%s'\n",
										   key.c_str(), group.GetAddress().c_str() ));
								success = false;
							}

							if ( !success )
							{
								gpwarningf((
										"Failed to add item '%s' to slot '%s'\n",
										value.c_str(), key.c_str() ));
							}

							if ( first )
							{
								count = calcMinMax( GetGo(), item ? item->GetDataTemplate() : NULL, minCount, maxCount );
								first = false;
							}
						}
						while ( --count > 0 );
					}
				}

				// possibly add subgroups
				FastFuelHandleColl subGroups;
				group.ListChildren( subGroups );

				FastFuelHandleColl::iterator i, ibegin = subGroups.begin(), iend = subGroups.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					// check chance
					float chance = (*i).GetFloat( "chance", 1.0f ) * allMultiplier;
					gppcontentf(( "Considering adding subgroup '%s', chance is %g\n", (*i).GetName(), chance ));
					if ( GetGoCreateRng().Random( 1.0f ) <= chance )
					{
						// add it!
						GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );
						AddPcontent( *i, GROUP_DETECT, isStandaloneLoad, false );
					}
				}

				// special safety
				if ( !found && subGroups.empty() )
				{
					gperrorf(( "No entries to choose from at '%s'!\n", group.GetAddress().c_str() ));
				}
			}
			break;

		// "oneof*"

			case ( GROUP_ONEOF ):
			{
				// $ this one's a little more complicated...

				// $ in order to calc the min/max to use for the block, just
				//   pick the first one normally, then use its traits to
				//   determine the min/max multipliers to use to get the rest
				//   (if there are any).

				gppcontent( "GROUP_ONEOF: Generating 'oneof*' items\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				bool first = true;
				int count = 1;

				do
				{
					typedef std::list <Entry> EntryColl;
					typedef std::list <Entry*> EntryPtrColl;
					EntryColl entries;
					EntryPtrColl unknowns;

					// get items
					FastFuelFindHandle fh( group );
					if ( fh.FindFirstKeyAndValue() )
					{
						gppcontent( "Finding items for chooser\n" );
						GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

						gpstring key, value;
						while ( fh.GetNextKeyAndValue( key, value ) )
						{
							eEquipSlot slot = ES_NONE;
							eInventoryLocation iloc = IL_INVALID;

							if ( ::FromString( key, slot ) && IsEquippableSlot( slot ) )
							{
								if ( (slot != ES_RING) && IsSlotEquipped( slot ) )
								{
									slot = ES_NONE;
								}
								Entry entry;
								entry.m_Name  = key;
								entry.m_Value = value;
								entry.m_Slot  = slot;
								entries.push_back( entry );
								unknowns.push_back( &entries.back() );

								gppcontentf(( "Found potential item '%s' for slot %s\n", value.c_str(), ::ToString( slot ) ));
							}
							else if ( ::FromString( key, iloc ) && IsAddableSlot( iloc ) )
							{
								Entry entry;
								entry.m_Name  = key;
								entry.m_Value = value;
								entry.m_Iloc  = iloc;
								entries.push_back( entry );
								unknowns.push_back( &entries.back() );

								gppcontentf(( "Found potential item '%s' for location %s\n", value.c_str(), ::ToString( iloc ) ));
							}
							else if (   !key.same_no_case( "min"    )
									 && !key.same_no_case( "max"    )
									 && !key.same_no_case( "chance" ) )
							{
								gperrorf(( "Unable to convert to eEquipSlot or eInventoryLocation from value '%s' at '%s'\n",
										   key.c_str(), group.GetAddress().c_str() ));
							}
						}
					}

					// get subgroups
					float remaining = 1.0f;
					FastFuelHandleColl subGroups;
					group.ListChildren( subGroups );
					if ( !subGroups.empty() )
					{
						gppcontent( "Finding blocks for chooser\n" );
						GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

						FastFuelHandleColl::iterator i, ibegin = subGroups.begin(), iend = subGroups.end();
						for ( i = ibegin ; i != iend ; ++i )
						{
							if (   same_no_case( (*i).GetName(), "all*"   )
								|| same_no_case( (*i).GetName(), "oneof*" )
								|| same_no_case( (*i).GetName(), "gold*"  ) )
							{
								Entry entry;
								entry.m_Fuel = (*i);
								entry.m_Chance = (*i).GetFloat( "chance", 0.0f ) * oneOfMultiplier;
								entries.push_back( entry );
								if ( entry.m_Chance > 0 )
								{
									remaining -= entry.m_Chance;

									// check to see if we're out of space and aren't
									// scaling up for multiplayer
									if ( IsNegative( remaining ) && (oneOfMultiplier <= 1.0f) )
									{
										gperrorf(( "Total 'chance' greater than 1.0f! Check your math at '%s'.\n", group.GetAddress().c_str() ));
									}

									gppcontentf(( "Found potential block '%s' with chance %g\n", (*i).GetName(), entry.m_Chance ));
								}
								else
								{
									unknowns.push_back( &entries.back() );
									gppcontentf(( "Found potential block '%s' with derived chance\n", (*i).GetName() ));
								}
							}
							else
							{
								gperrorf(( "Illegal subgroup type '%s' at '%s'\n",
										   (*i).GetName(), (*i).GetAddress().c_str() ));
							}
						}
					}

					// update randomizations
					if ( !unknowns.empty() )
					{
						gppcontent( "Assigning random chances for derived\n" );
						GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

						if ( remaining <= 0 )
						{
							gperrorf(( "Total 'remaining' is <= 0, nothing left for auto chance items! Check your math at '%s'.\n", group.GetAddress().c_str() ));
						}

						float each = remaining / unknowns.size();
						EntryPtrColl::iterator i, ibegin = unknowns.begin(), iend = unknowns.end();
						for ( i = ibegin ; i != iend ; ++i )
						{
							(*i)->m_Chance = each;
							gppcontentf(( "'%s' gets %g\n", (*i)->m_Name.empty() ? (*i)->m_Fuel.GetName() : (*i)->m_Name.c_str(), each ));
						}
					}

					// re-scale if necessary
					float totalScale = 1.0f;
					if ( remaining < 0 )
					{
						// this will increase the scale to include everything.
						// necessary for mp loot scaling where the multipliers
						// can result in total chances going over 100%.
						totalScale += remaining;
						gppcontentf(( "Scaling total by %g to account for MP loot multipliers", totalScale ));
					}

					// choose random entry
					if ( !entries.empty() )
					{
						gppcontent( "Choosing a single random entry\n" );
						GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

						// find one
						float runner = 0, target = GetGoCreateRng().Random( totalScale );
						gppcontentf(( "Chose target chance of %g\n", target ));
						EntryColl::iterator i, ibegin = entries.begin(), iend = entries.end();
						for ( i = ibegin ; i != iend ; ++i )
						{
							runner += i->m_Chance;
							if ( target <= runner )
							{
								break;
							}
						}

						// use it
						if ( i != iend )
						{
							gppcontentf(( "Chose new item '%s'!\n", i->m_Name.empty() ? i->m_Fuel.GetAddress().c_str() : i->m_Name.c_str() ));

							if ( i->m_Fuel )
							{
								AddPcontent( i->m_Fuel, GROUP_DETECT, isStandaloneLoad, false );
							}
							else
							{
								Go* item = NULL;

								if ( i->m_Slot != ES_NONE )
								{
									item = LoadItem( "", i->m_Value, i->m_Slot, true, isStandaloneLoad );
								}
								else
								{
									item = LoadItem( i->m_Name, i->m_Value, ES_NONE, true, isStandaloneLoad );
								}

								if ( item == NULL )
								{
									gpwarningf((
											"Failed to add item '%s' to slot '%s'\n",
											i->m_Value.c_str(), i->m_Name.c_str() ));
								}

								if ( first )
								{
									count = calcMinMax( GetGo(), item ? item->GetDataTemplate() : NULL, minCount, maxCount );
									first = false;
								}
							}
						}
					}
					else
					{
						gperrorf(( "No entries to choose from at '%s'!\n", group.GetAddress().c_str() ));
					}
				}
				while ( --count > 0 );
			}
			break;

			default:
			{
				gpassert( 0 );	// $ should never get here
			}
		}
	}
}

void GoInventory :: AddStorePcontent( FastFuelHandle base )
{
	gpassert( base );
	gpassert( GetGo()->HasStore() );

	FastFuelHandleColl tabs;
	base.ListChildren( tabs );
	FastFuelHandleColl::const_iterator i, ibegin = tabs.begin(), iend = tabs.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		AddPcontent( *i, GROUP_ALL );
	}
}

Go* GoInventory :: LoadItem(
		const gpstring& ilText,
		const gpstring& templateText,
		eEquipSlot      slot,
		bool            isPContent,
		bool            isStandaloneLoad,
		PContentReq*    pcontentReq,
		bool            bAutoPlace,
		Goid            explicitGoid )
{
	// bail out if we're generating content in siege editor
#	if ( !GP_RETAIL )
	{
		if ( gGoDb.IsEditMode() && (templateText[ 0 ] == '#') )
		{
			return ( (Go*)1 );
		}
	}
#	endif // !GP_RETAIL

	// add it to the general collection
	eInventoryLocation il = IL_MAIN;
	if ( !ilText.same_no_case( "*" ) && !ilText.empty() )
	{
		::FromString( ilText, il );
	}

	// check location dup
	if ( ::IsSingularSlot( il ) )
	{
		Go* go = GetItem( il );
		if ( go != NULL )
		{
			gperrorf((
					"TELL ERNSIE: Serious error! Someone is trying to load a new item '%s' "
					"into location '%s', but an object (goid = 0x%08X, "
					"scid = 0x%08X, template = '%s') already exists there!\n\n"
					"Owner is goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
					templateText.c_str(), ::ToString( il ), go->GetGoid(),
					go->GetScid(), go->GetTemplateName(),
					GetGoid(), GetScid(), GetGo()->GetTemplateName() ));
			return ( NULL );
		}
	}

	// check slot dup
	if ( (slot != ES_NONE) && (slot != ES_ANY) )
	{
		Go* go = GetEquipped( slot );
		if ( go != NULL )
		{
			gperrorf((
					"TELL ERNSIE: Serious error! Someone is trying to load a new item '%s' "
					"into equip slot '%s', but an object (goid = 0x%08X, "
					"scid = 0x%08X, template = '%s') already exists there!\n\n"
					"Owner is goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
					templateText.c_str(), ::ToString( slot ), go->GetGoid(),
					go->GetScid(), go->GetTemplateName(),
					GetGoid(), GetScid(), GetGo()->GetTemplateName() ));
			return ( NULL );
		}
	}

	// first create and add it using me as the creation location
	GoCloneReq cloneReq;

	// stashes load the items as omni gos instead of using the parent position
	if ( GetGo()->HasStash() )
	{
		cloneReq.m_OmniGo = true;	
	}
	else
	{
		cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
	}

	cloneReq.SetForceClientAllowed();
	cloneReq.m_AllClients = GetGo()->IsAllClients();
	Goid goid = explicitGoid;
	if ( goid == GOID_INVALID )
	{
		if ( isStandaloneLoad )
		{
			goid = gGoDb.SCloneGo( cloneReq, templateText );
		}
		else
		{
			goid = gGoDb.CloneGo( cloneReq, templateText );
		}
	}
	else
	{
		gGoDb.CloneGo( explicitGoid, cloneReq, templateText );
	}

	// failed?
	GoHandle go( goid );
	if ( !go )
	{
		return ( NULL );
	}

	// finish it?
	if ( pcontentReq != NULL )
	{
		gPContentDb.ApplyPContentRequest( go, *pcontentReq );
	}

	// pcontent?
	if ( isPContent )
	{
		go->SetIsPContentInventory();
	}

	// special handling for a spell
	if ( ( il >= IL_ACTIVE_PRIMARY_SPELL ) && ( il <= IL_SPELL_12 ) && !GetGo()->IsSpellBook() )
	{
		GoHandle hSpellBook( GetEquipped( ES_SPELLBOOK ) );
		if ( !hSpellBook )
		{
			GoCloneReq cloneBookReq;
			cloneBookReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
			cloneBookReq.SetForceClientAllowed();
			cloneBookReq.m_AllClients = GetGo()->IsAllClients();
			Goid bookgoid = isStandaloneLoad ? gGoDb.SCloneGo( cloneBookReq, gContentDb.GetDefaultSpellBookTemplate() )
											 : gGoDb. CloneGo( cloneBookReq, gContentDb.GetDefaultSpellBookTemplate() );
			hSpellBook = bookgoid;
			if ( !hSpellBook || !Add( hSpellBook, IL_MAIN, AO_REFLEX, true ) || !Equip( ES_SPELLBOOK, hSpellBook->GetGoid(), AO_REFLEX ) )
			{
				return ( NULL );
			}
		}

		if ( il == IL_ACTIVE_PRIMARY_SPELL )
		{
			il = IL_SPELL_1;
		}
		else if ( il == IL_ACTIVE_SECONDARY_SPELL )
		{
			il = IL_SPELL_2;
		}

		if ( !hSpellBook->GetInventory()->Add( go, il, AO_REFLEX ) )
		{
			return ( NULL );
		}

		SetActiveSpellBook( hSpellBook );
	}
	else
	{
		// add to inventory
		if ( !Add( go, il, AO_REFLEX, bAutoPlace ? (il == IL_MAIN && slot == ES_NONE ) : false ) )
		{
			return ( NULL );
		}

		// optionally equip it
		if ( (slot != ES_NONE) && !Equip( slot, goid, AO_REFLEX ) )
		{
			return ( NULL );
		}
	}

	return ( go );
}

DWORD GoInventory :: GoInventoryToNet( GoInventory* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoInventory* GoInventory :: NetToGoInventory( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetInventory() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
