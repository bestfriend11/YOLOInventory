//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoMagic.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoMagic.h"

#include "Enchantment.h"
#include "FormulaHelper.h"
#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "ContentDb.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "GoPhysics.h"
#include "GoShmo.h"
#include "GoSupport.h"
#include "PContentDb.h"
#include "World.h"

DECLARE_GO_COMPONENT( GoMagic, "magic" );

//////////////////////////////////////////////////////////////////////////////
// class GoMagic implementation

GoMagic :: GoMagic( Go* parent )
	: Inherited( parent )
{
	m_Options            = OPTION_NONE;
	m_TargetTypeFlags    = TT_NONE;
	m_TargetTypeFlagsNot = TT_NONE;
	m_Charges            = 0;
	m_EnchantmentStorage = NULL;
}

GoMagic :: GoMagic( const GoMagic& source, Go* parent )
	: Inherited( source, parent )
	, m_PrefixModifierName( source.m_PrefixModifierName )
	, m_SuffixModifierName( source.m_SuffixModifierName )
{
	m_Options            = source.m_Options;
	m_TargetTypeFlags    = source.m_TargetTypeFlags;
	m_TargetTypeFlagsNot = source.m_TargetTypeFlagsNot;
	m_Charges            = source.m_Charges;
	m_EnchantmentStorage = NULL;

	if ( source.m_EnchantmentStorage != NULL )
	{
		m_EnchantmentStorage = new EnchantmentStorage( *source.m_EnchantmentStorage );
	}
}

GoMagic :: ~GoMagic( void )
{
	delete ( m_EnchantmentStorage );
}

float GoMagic :: GetPotionFullRatio( void ) const
{
	gpassert( IsPotion() );
	gpassert( m_EnchantmentStorage != NULL );

	float ratio = 1.0f;

	const EnchantmentVector& enchVec = m_EnchantmentStorage->GetEnchantments();
	EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
	for ( i = begin ; i != end ; ++i )
	{
		const Enchantment* ench = *i;
		if( ench->IsValueLimited() )
		{
			ratio = ench->GetValue() / ench->GetMaxValue();
			break;
		}
	}

	return ( ratio );
}

float GoMagic :: GetPotionAmount( bool bMax ) const
{
	gpassert( IsPotion() );
	gpassert( m_EnchantmentStorage != NULL );

	float amount = 0.0f;

	const EnchantmentVector& enchVec = m_EnchantmentStorage->GetEnchantments();
	EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
	for ( i = begin ; i != end ; ++i )
	{
		const Enchantment* ench = *i;
		if ( (ench->GetAlterationType() == ALTER_LIFE) || (ench->GetAlterationType() == ALTER_MANA) )
		{
			if ( bMax )
			{
				amount = ench->GetMaxValue();
			}
			else
			{
				amount = ench->GetValue();
			}
			break;
		}
	}

	return ( amount );
}

void GoMagic :: RCSetPotionAmount( float value )
{
	FUBI_RPC_THIS_CALL( RCSetPotionAmount, RPC_TO_ALL );

	if ( GetPotionAmount() != value )
	{
		GetGo()->SetBucketDirty();
		SetPotionAmount( value );
	}
}

void GoMagic :: SetPotionAmount( float value )
{
	gpassert( IsPotion() );
	gpassert( m_EnchantmentStorage != NULL );

	const EnchantmentVector& enchVec = m_EnchantmentStorage->GetEnchantments();
	EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
	for ( i = begin ; i != end ; ++i )
	{
		Enchantment* ench = *i;
		if ( (ench->GetAlterationType() == ALTER_LIFE) || (ench->GetAlterationType() == ALTER_MANA) )
		{
			ench->SetValue( value );
			break;
		}
	}

	GoPotion* potion = GetGo()->QueryPotion();
	if ( potion != NULL )
	{
		potion->SetFullRatioDirty();
	}
}

bool GoMagic :: CanOwnerCast( void ) const
{
	if ( GetGo()->GetParent() && GetGo()->GetParent()->GetParent() && GetGo()->GetParent()->GetParent()->HasActor() )
	{
		return ( GetRequiredCastLevel() <= GetGo()->GetParent()->GetParent()->GetActor()->GetSkillLevel( GetSkillClass() ) );
	}

	return false;
}

Go* GoMagic :: GetCaster( void )
{
	Go * spellOwner = GetGo()->GetParent();

	if( !spellOwner->HasActor() )
	{
		spellOwner = spellOwner->GetParent();
	}

	gpassert( spellOwner != NULL );

	return spellOwner;
}

const gpstring& GoMagic :: GetSkillClass( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "skill_class" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetRequiredCastLevel( void ) const
{
	static AutoConstQuery <float> s_Query( "required_level" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetPContentLevel( void ) const
{
	static AutoConstQuery <float> s_Query( "pcontent_level" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetMaxCastLevel( void ) const
{
	static AutoConstQuery <float> s_Query( "max_level" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

eMagicClass GoMagic :: GetMagicClass( void ) const
{
	static AutoConstQuery <eMagicClass> s_Query( "magic_class" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetManaCost( void ) const
{
	static AutoConstQuery <float> s_Query( "mana_cost" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetManaCostModifier( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "mana_cost_modifier" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetManaCostUI( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "mana_cost_ui" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetManaCostUIModifier( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "mana_cost_ui_modifier" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetSpeedBias( void ) const
{
	static AutoConstQuery <float> s_Query( "speed_bias" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetCastRange( void ) const
{
	static AutoConstQuery <float> s_Query( "cast_range" );
	return ( s_Query.Get( GetData()->GetRecord() ) + gSim.GetSniperRange() );
}

float GoMagic :: GetCastReloadDelay( void ) const
{
	static AutoConstQuery <float> s_Query( "cast_reload_delay" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetSkillCoefficientA( void ) const
{
	static AutoConstQuery <float> s_Query( "coefficient_a" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoMagic :: GetSkillCoefficientB( void ) const
{
	static AutoConstQuery <float> s_Query( "coefficient_b" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoMagic :: GetApplyEnchantmentsOnCast( void ) const
{
	static AutoConstQuery <bool> s_Query( "apply_enchantments" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetAttackDamageModifierMin( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "attack_damage_modifier_min" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetAttackDamageModifierMax( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "attack_damage_modifier_max" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

UINT32 GoMagic :: GetCastSubAnimation( void ) const
{
	static AutoConstQuery <UINT32> s_Query( "cast_sub_animation" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetEffectDuration( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "effect_duration" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetCastExperience( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "cast_experience" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoMagic :: GetStateName( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "state_name" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring&	GoMagic :: GetCasterStateName( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "caster_state_name" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoMagic :: GetRequireStateCheck( void ) const
{
	static AutoConstQuery <bool> s_Query( "require_state_check" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoMagic :: GetIsOneUse( void ) const
{
	static AutoConstQuery <bool> s_Query( "one_use" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoMagic :: GetDoesDamagePerSecond( void ) const
{
	static AutoConstQuery <bool> s_Query( "does_damage_per_second" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoMagic :: GetManaCostRangeUI( float &min_cost, float &max_cost, Go * caster, const char *pOverrides )
{
	gpassert( caster );

	float result = 0;

	if( GetManaCostUI().empty() && GetManaCostUIModifier().empty() )
	{
		min_cost = GetManaCost();

		if( GetManaCostModifier().empty() )
		{
			max_cost = min_cost;
		}
		else
		{	
			FormulaHelper::EvaluateFormula( GetManaCostModifier(), pOverrides, result, caster->GetGoid(), caster->GetGoid(), GetGo()->GetGoid() );
			min_cost += result;
			max_cost = min_cost;
		}
	}
	else
	{
		if( GetManaCostUIModifier().empty() )
		{
			FormulaHelper::EvaluateFormula( GetManaCostUI(), pOverrides, min_cost, caster->GetGoid(), caster->GetGoid(), GetGo()->GetGoid() );
			max_cost = min_cost;
		}
		else
		{
			if( GetManaCostUI().empty() )
			{
				min_cost = GetManaCost();
			}
			else
			{
				FormulaHelper::EvaluateFormula( GetManaCostUI(), pOverrides, min_cost, caster->GetGoid(), caster->GetGoid(), GetGo()->GetGoid() );
			}
			FormulaHelper::EvaluateFormula( GetManaCostUIModifier(), pOverrides, max_cost, caster->GetGoid(), caster->GetGoid(), GetGo()->GetGoid() );

			max_cost += min_cost;
		}
	}

	min_cost = floorf( IsEqual( min_cost, ceilf( min_cost ), 0.001f ) ? ceilf( min_cost ) : IsEqual( min_cost, floorf( min_cost ), 0.001f ) ? floorf( min_cost ) : min_cost );
	max_cost = floorf( IsEqual( max_cost, ceilf( max_cost ), 0.001f ) ? ceilf( max_cost ) : IsEqual( max_cost, floorf( max_cost ), 0.001f ) ? floorf( max_cost ) : max_cost );
}

float GoMagic :: GetMagicLevel( Go const * caster )
{
	gpassert( caster );

	if( caster->HasActor() )
	{
		return min_t( caster->GetActor()->GetSkillLevel( GetSkillClass() ), GetMaxCastLevel() );
	}
	return 0;
}

bool GoMagic :: CanReachNextLevel( Go const * caster )
{
	return ( GetMaxCastLevel() > GetMagicLevel( caster ) );
}

float GoMagic :: EvaluateManaCost( Go const * caster, Go const * target, const char *pOverrides ) const
{
	gpassert( caster );

	float result = 0;

	if( !GetManaCostModifier().empty() )
	{
		FormulaHelper::EvaluateFormula( GetManaCostModifier(), pOverrides, result, target ? target->GetGoid() : GOID_INVALID,
										caster->GetGoid(), GetGo()->GetGoid() );
		result = floorf( result );
	}

	return ( GetManaCost() + result );
}

float GoMagic :: EvaluateEffectDuration( Go const * caster, Go const * target, const char *pOverrides ) const
{
	gpassert( caster );
	float result = 0;

	if( !GetEffectDuration().empty() )
	{
		FormulaHelper::EvaluateFormula( GetEffectDuration(), pOverrides, result, target ? target->GetGoid() : GOID_INVALID,
										caster->GetGoid(), GetGo()->GetGoid() );
		result = floorf( result );
	}

	return ( result );
}

float GoMagic :: EvaluateCastExperience( Go const * caster, Go const * target, const char *pOverrides ) const
{
	gpassert( caster );
	float result = 0;

	if( !GetCastExperience().empty() )
	{
		FormulaHelper::EvaluateFormula( GetCastExperience(), pOverrides, result, target ? target->GetGoid() : GOID_INVALID,
										caster->GetGoid(), GetGo()->GetGoid() );
		result = floorf( result );
	}

	return ( result );
}

float GoMagic :: EvaluateAttackDamageModifierMin( Go const * caster, Go const * target, const char *pOverrides ) const
{
	float result = 0;

	if( !GetAttackDamageModifierMin().empty() )
	{
		FormulaHelper::EvaluateFormula( GetAttackDamageModifierMin(), pOverrides, result, target ? target->GetGoid() : GOID_INVALID,
										caster ? caster->GetGoid() : GOID_INVALID, GetGo()->GetGoid() );
		result = floorf( result );
	}

	return ( result );
}

float GoMagic :: EvaluateAttackDamageModifierMax( Go const * caster, Go const * target, const char *pOverrides ) const
{
	float result = 0;

	if( !GetAttackDamageModifierMax().empty() )
	{
		FormulaHelper::EvaluateFormula( GetAttackDamageModifierMax(), pOverrides, result, target ? target->GetGoid() : GOID_INVALID,
										caster ? caster->GetGoid() : GOID_INVALID, GetGo()->GetGoid() );
		result = floorf( result );
	}

	return ( result );
}

eUsageContextFlags GoMagic :: GetUsageContextFlags( void ) const
{
	static AutoConstQuery <eUsageContextFlags> s_Query( "usage_context_flags" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoMagic :: IsCastableOn( Go * target, bool CheckMana )
{
	Go *pCaster = NULL;

	if( GetGo()->GetParent( pCaster ) )
	{
		if( !pCaster->HasActor() )
		{
			pCaster = pCaster->GetParent();
		}

		gpassert( pCaster->HasActor() );

		eTargetTypeFlags targetFlags	= BuildTargetTypeFlags( pCaster, target );
		eTargetTypeFlags spellFlags		= GetTargetTypeFlags();
		eTargetTypeFlags spellFlagsNot	= GetTargetTypeFlagsNot();

		// Check for the AND bit
		if( !!(spellFlags & TT_AND) )
		{
			// Remove the AND bit
			spellFlags	 = eTargetTypeFlags ( ~TT_AND & spellFlags		);
			spellFlagsNot= eTargetTypeFlags ( ~TT_AND & spellFlagsNot	);

			// compare
			eTargetTypeFlags testFlagsNot = ( targetFlags & spellFlagsNot );
			eTargetTypeFlags testFlags	  = ( targetFlags & spellFlags	  );

			if( (testFlagsNot == TT_NONE) && (testFlags == spellFlags) )
			{
				bool manaMet	= CheckMana ? EvaluateManaCost( pCaster, target ) <= pCaster->GetAspect()->GetCurrentMana() : true;
				bool levelMet	= GetRequiredCastLevel() <= pCaster->GetActor()->GetSkillLevel( GetSkillClass() );
				bool clearState = true;
				bool EnchantOk	= (HasEnchantments())?!gGoDb.HasSingleInstanceEnchantment( target->GetGoid(), GetEnchantmentStorage() ):true;
				bool clearCasterState = ( pCaster->HasActor() && GetRequireStateCheck() ) ? !pCaster->GetActor()->HasGenericState( GetCasterStateName() ) : true;

				if( GetRequireStateCheck() && target->HasActor() )
				{
					clearState = !target->GetActor()->HasGenericState( GetStateName() );
				}

				return( manaMet && levelMet && clearState && EnchantOk && clearCasterState );
			}
			else
			{
				return( false );
			}
		}
		else
		{
			// Check if one or more bits are set
			if( ((spellFlagsNot & targetFlags) == TT_NONE) && !!(spellFlags & targetFlags) )
			{
				bool manaMet	= CheckMana ? EvaluateManaCost( pCaster, target ) <= pCaster->GetAspect()->GetCurrentMana() : true;
				bool levelMet	= GetRequiredCastLevel() <= pCaster->GetActor()->GetSkillLevel( GetSkillClass() );
				bool clearState = true;
				bool EnchantOk	= (HasEnchantments())?!gGoDb.HasSingleInstanceEnchantment( target->GetGoid(), GetEnchantmentStorage() ):true;
				bool clearCasterState = ( pCaster->HasActor() && GetRequireStateCheck() )  ? !pCaster->GetActor()->HasGenericState( GetCasterStateName() ) : true;

				if( GetRequireStateCheck() && target->HasActor() )
				{
					clearState = !target->GetActor()->HasGenericState( GetStateName() );
				}
				
				return( manaMet && levelMet && clearState && EnchantOk && clearCasterState );
			}
			else
			{
				return( false );
			}
		}
	}
	return false;
}

eTargetTypeFlags GoMagic :: BuildTargetTypeFlags( Go * caster, Go * target )
{
	eTargetTypeFlags flags = TT_NONE;

	if( caster && target )
	{
		GoActor *pTargetActor	= target->QueryActor();
		GoAspect *pTargetAspect = target->QueryAspect();

		// Generic
		if( caster == target )
		{
			flags |= TT_SELF;
		}

		if( pTargetAspect && ( pTargetAspect->GetMaxMana() > 0 ) )
		{
			flags |= TT_CAN_STORE_MANA;
		}

		// Actor
		if( pTargetActor )
		{
			switch( pTargetActor->GetAlignment() )
			{
				case AA_GOOD:
				{
					flags |= TT_GOOD;
				}	break;
				case AA_NEUTRAL:
				{
					flags |= TT_NEUTRAL;
				}	break;
				case AA_EVIL:
				{
					flags |= TT_EVIL;
				}	break;
			}

			if( target->IsAnyHumanPartyMember() )
			{
				flags |= TT_HUMAN_PARTY_MEMBER;
			}

			if ( target->IsScreenPartyMember() )
			{
				flags |= TT_SCREEN_PARTY_MEMBER;
			}

			if( pTargetActor->HasGenericState( "summoned" ) )
			{
				flags |= TT_SUMMONED;
			}

			if( pTargetActor->HasGenericState( "possessed" ) )
			{
				flags |= TT_POSSESSED;
			}

			if( pTargetActor->HasGenericState( "animated" ) )
			{
				flags |= TT_ANIMATED;
			}

			GoMind *pCasterMind = caster->QueryMind();

			if( pCasterMind )
			{
				const eLifeState life_state = target->GetLifeState();

				// Friendly Actor
				if( pCasterMind->IsFriend( target ) )
				{
					if( IsAlive( life_state ) )
					{
						if( life_state == LS_ALIVE_CONSCIOUS )
						{
							flags |= TT_CONSCIOUS_FRIEND;
						}
						else if( life_state == LS_ALIVE_UNCONSCIOUS )
						{
							flags |= TT_UNCONSCIOUS_FRIEND;
						}
					}
					else
					{
						flags |= TT_DEAD_FRIEND;
					}

					if( (pTargetAspect->GetCurrentLife() + FLOAT_TOLERANCE) < pTargetAspect->GetMaxLife() )
					{
						flags |= TT_INJURED_FRIEND;
					}
				}
				// Enemy Actor
				else if( pCasterMind->IsEnemy( target ) )
				{
					if( IsAlive( life_state ) )
					{
						if( life_state == LS_ALIVE_CONSCIOUS )
						{
							flags |= TT_CONSCIOUS_ENEMY;
						}
						else if( life_state == LS_ALIVE_UNCONSCIOUS )
						{
							flags |= TT_UNCONSCIOUS_ENEMY;
						}
					}
					else
					{
						flags |= TT_DEAD_ENEMY;
					}
				}
			}

			// Now Check the inventory for any equipped items the target might have
			GoInventory *pInventory = target->GetInventory();

			if( !pInventory )
			{
				flags |= TT_ACTOR;
			}
			else
			{
				if( pInventory->IsPackOnly() )
				{
					flags |= TT_ACTOR_PACK_ONLY;
				}
				else
				{
					flags |= TT_ACTOR;
				}

				Go *pItem = pInventory->GetEquipped( ES_WEAPON_HAND );

				if( pItem != NULL )
				{
					GoAttack *pAttack = pItem->QueryAttack();

					if( pAttack )
					{
						if( pAttack->GetIsMelee() )
						{
							flags |= TT_WEAPON_MELEE;

						}
						else if( pAttack->GetIsProjectile() )
						{
							flags |= TT_WEAPON_RANGED;
						}
					}
				}

				pItem = pInventory->GetEquipped( ES_SHIELD_HAND );

				if( pItem )
				{
					GoDefend *pDefend = pItem->QueryDefend();

					if( pDefend )
					{
						if( pDefend->GetIsShield() )
						{
							flags |= TT_SHIELD;
						}
					}

					if( pItem )
					{
						GoAttack *pAttack = pItem->QueryAttack();

						if( pAttack )
						{
							if( pAttack->GetIsProjectile() )
							{
								flags |= TT_WEAPON_RANGED;
							}
						}
					}
				}

				pItem = pInventory->GetEquipped( ES_CHEST );

				if( pItem )
				{
					flags |= TT_ARMOR;
				}
			}
		}
		// Non-Actor
		else
		{
			flags |= TT_NOT_ACTOR;

			GoDefend *pDefend = target->QueryDefend();

			if( pDefend )
			{
				if( pDefend->GetIsShield() )
				{
					flags |= TT_SHIELD;
				}
				else
				{
					flags |= TT_ARMOR;
				}
			}

			GoGui *pGui = target->QueryGui();

			if( pGui && IsEquippableSlot( pGui->GetEquipSlot() ) )
			{
				flags |= TT_EQUIPPABLE;
			}

			GoPhysics *pPhysics = target->QueryPhysics();

			if( pPhysics && pPhysics->GetIsBreakable()  )
			{
				flags |= TT_BREAKABLE;
			}

			GoAttack *pTargetAttack = target->QueryAttack();

			if( pTargetAttack )
			{
				if( pTargetAttack->GetIsMelee() )
				{
					flags |= TT_WEAPON_MELEE;
				}

				if( pTargetAttack->GetIsProjectile() )
				{
					flags |= TT_WEAPON_RANGED;
				}
			}

			if( ( flags & TT_SHIELD ) || ( flags & TT_ARMOR ) || ( flags & TT_WEAPON_MELEE ) || ( flags & TT_WEAPON_RANGED ) )
			{
				flags |= TT_TRANSMUTABLE;
			}

			if( same_no_case( target->GetTemplateName(), "point" ) )
			{
				flags |= TT_TERRAIN;
			}
		}
	}
	return flags;
}

Go* GoMagic :: GetCorrectTarget( Go * target )
{
	Go *pCorrectTarget = NULL;

	eTargetTypeFlags tf = BuildTargetTypeFlags( GetCaster(), target );
	eTargetTypeFlags sf	= GetTargetTypeFlags();

	if( tf & TT_ACTOR )
	{
		if( ((sf & TT_WEAPON_MELEE) && (tf & TT_WEAPON_MELEE)) )
		{
			if( target->HasInventory() )
			{
				pCorrectTarget = target->GetInventory()->GetEquipped( ES_WEAPON_HAND );

				return ( pCorrectTarget )? pCorrectTarget : target;
			}
		}
		else if( ((sf & TT_WEAPON_RANGED) && (tf & TT_WEAPON_RANGED)) )
		{
			if( target->HasInventory() )
			{
				pCorrectTarget = target->GetInventory()->GetEquipped( ES_SHIELD_HAND );

				if( pCorrectTarget == NULL )
				{
					// It might be the minigun so try the weapon hand too
					pCorrectTarget = target->GetInventory()->GetEquipped( ES_WEAPON_HAND );

					return ( pCorrectTarget )? pCorrectTarget : target;
				}
			}
		}
		else if( ((sf & TT_ARMOR) && (tf & TT_ARMOR)) )
		{
			if( target->HasInventory() )
			{
				pCorrectTarget = target->GetInventory()->GetEquipped( ES_CHEST );

				return ( pCorrectTarget )? pCorrectTarget : target;
			}
		}
		else if( ((sf & TT_SHIELD) && (tf & TT_SHIELD)) )
		{
			if( target->HasInventory() )
			{
				pCorrectTarget = target->GetInventory()->GetEquipped( ES_SHIELD_HAND );

				return ( pCorrectTarget )? pCorrectTarget : target;
			}
		}
	}

	return target;
}

bool GoMagic :: GetIsCommandCast( void ) const
{  
	if ( !gWorldOptions.GetCommandCastEnabled() )
	{
		return false;
	}

	return ( TestOptions( OPTION_COMMAND_CAST ) ); 
}

bool GoMagic :: HasAlterationType( eAlteration alteration ) const
{
	const EnchantmentVector& enchVec = m_EnchantmentStorage->GetEnchantments();
	EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
	for ( i = begin ; i != end ; ++i )
	{
		const Enchantment* ench = *i;
		if ( ench->GetAlterationType() == alteration )
		{
			return true;
		}
	}

	return false;
}

bool GoMagic :: IsRejuvenationPotion( void ) const
{
	if ( ::IsPotion( GetMagicClass() ) )
	{		
		return ( HasAlterationType( ALTER_LIFE ) && HasAlterationType( ALTER_MANA ) );
	}

	return false;
}

bool GoMagic :: IsHealthPotion( void ) const
{
	if ( ::IsPotion( GetMagicClass() ) )
	{
		return ( HasAlterationType( ALTER_LIFE ) );
	}

	return false;
}

bool GoMagic :: IsManaPotion( void ) const
{
	if ( ::IsPotion( GetMagicClass() ) )
	{
		return ( HasAlterationType( ALTER_MANA ) );
	}

	return false;
}

bool GoMagic :: HasNonInnateEnchantments( void )
{
	if ( m_EnchantmentStorage )
	{
		const EnchantmentVector& enchVec = m_EnchantmentStorage->GetEnchantments();
		EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
		for ( i = begin ; i != end ; ++i )
		{
			const Enchantment* ench = *i;
			if( !ench->IsInnateEnchantment() )
			{
				return true;
			}
		}
	}

	return false;
}

bool GoMagic :: SCast( Go * target )
{
	CHECK_SERVER_ONLY;
	if( target == NULL )
	{
		gperror( "GoMagic :: SCast - You called with NULL target." );
		return false;
	}

	Go * spellOwner = GetCaster();

	if( spellOwner == NULL )
	{
		return false;
	}

#	if !GP_RETAIL
	if (gWorldOptions.GetHudMCP())
	{
		SiegePos p = spellOwner->GetPlacement()->GetPosition();
		p.pos.y += 2.0f*spellOwner->GetAspect()->GetBoundingSphereRadius();
		gWorld.DrawDebugWorldLabelScroll(p,"CAST",0xff00ffff,3,true);
	}
#	endif // !GP_RETAIL

	// Make sure we this is the correct target
	target = GetCorrectTarget( target );

	// Award usage experience if there is no attack component for this spell
	if( spellOwner->HasActor() && spellOwner->GetActor()->GetCanLevelUp() )
	{
		gRules.AwardExperience( spellOwner->GetGoid(), GetGo()->GetGoid(), EvaluateCastExperience( spellOwner, target ) );
	}

	// Duplicate the game object that holds the scroll before the skrit that does the spell operations
	// occurs
	Go *pGo = GetGo();
	GoCloneReq clone_req( spellOwner->GetGoid(), pGo->GetGoid(), pGo->GetPlayerId() );
	clone_req.SetForceClientAllowed();
	Goid newGo = gGoDb.SCloneGo( clone_req );

	GoHandle hSpell( newGo );

	// Evaluate the attack_damage_modifiers if they exist
	if( hSpell && hSpell->HasMagic() && hSpell->HasAttack() )
	{
		GoAttack *pAttack = hSpell->GetAttack();
		if( !hSpell->GetMagic()->GetAttackDamageModifierMin().empty() )
		{
			float damage_modifier_min = 0;
			FormulaHelper::EvaluateFormula( hSpell->GetMagic()->GetAttackDamageModifierMin(), NULL,
											damage_modifier_min, spellOwner->GetGoid(), spellOwner->GetGoid(), GetGo()->GetGoid() );
			pAttack->SetDamageMinNatural( pAttack->GetDamageMin() + damage_modifier_min );
		}
		if( !hSpell->GetMagic()->GetAttackDamageModifierMax().empty() )
		{
			float damage_modifier_max = 0;
			FormulaHelper::EvaluateFormula( hSpell->GetMagic()->GetAttackDamageModifierMax(), NULL,
											damage_modifier_max, spellOwner->GetGoid(), spellOwner->GetGoid(), GetGo()->GetGoid() );
			pAttack->SetDamageMaxNatural( pAttack->GetDamageMax() + damage_modifier_max );
		}
	}

	// Handle charge information
	if ( GetIsOneUse() )
	{
		gGoDb.SMarkForDeletion( GetGo()->GetGoid() );
		if ( GetGo()->HasParent() && GetGo()->GetParent()->HasParent() &&
			 GetGo()->GetParent()->GetParent()->IsScreenPartyMember() &&
			 GetGo()->GetParent()->GetParent()->HasInventory() )
		{
			RCOneShotUsed( GetGo()->GetParent() );
		}
	}

	if ( spellOwner->GetPlayer() && spellOwner->GetPlayer()->GetController() == PC_HUMAN )
	{
		RCMemberCast( spellOwner->GetPlayer()->GetMachineId() );
	}

	return true;
}

void GoMagic :: RCMemberCast( DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCMemberCast, machineId );

	gWorld.CastCallback( GetGo()->GetGoid() );
}

void GoMagic :: RCOneShotUsed( Go * pSpellBook )
{
	FUBI_RPC_THIS_CALL( RCOneShotUsed, pSpellBook->GetMachineId() );

	pSpellBook->GetInventory()->SetInventoryDirty( true );
}

void GoMagic :: MakeEnchantments( void )
{
	if ( !HasEnchantments() )
	{
		m_EnchantmentStorage = new EnchantmentStorage;
	}
}

void GoMagic :: SApplyEnchantmentsByName( Goid target, Goid source, const char *sName )
{
	CHECK_SERVER_ONLY;

	RCApplyEnchantments( target, source, sName );
}

void GoMagic :: RCApplyEnchantments( Goid target, Goid source, const char *sName )
{
	FUBI_RPC_TAG();

	// early-out here to avoid net overhead if nothing to do
	if ( m_EnchantmentStorage == NULL )
	{
		return;
	}
	gpassert( m_EnchantmentStorage->HasEnchantments() );

	FUBI_RPC_THIS_CALL( RCApplyEnchantments, RPC_TO_ALL );

	ApplyEnchantmentsByName( target, source, sName );
}

void GoMagic :: ApplyEnchantmentsByName( Goid target, Goid source, const char *sName )
{
	if ( m_EnchantmentStorage != NULL )
	{
		if ( sName != NULL )
		{
			m_EnchantmentStorage->DoAlterationsByName( target, source, GetGo()->GetGoid(), sName );
		}
		else
		{
			bool bApplyInnate = true;
			if ( GetGo()->HasParent() && GetGo()->GetParent()->HasInventory() )
			{
				bApplyInnate = GetGo()->GetParent()->GetInventory()->IsEquipped( GetGo() );
			}

			m_EnchantmentStorage->DoAlterations( target, source, GetGo()->GetGoid(), bApplyInnate );
		}
	}

	GoPotion* potion = GetGo()->QueryPotion();
	if ( potion != NULL )
	{
		potion->SetFullRatioDirty();
	}
}

float GoMagic :: GetLongestAlteration( Goid target, Goid source, bool bEvaluate )
{
	return m_EnchantmentStorage->GetLongestDuration( target, source, GetGo()->GetGoid(), bEvaluate );
}

static bool FillModifier( const char* name, GoMagic::Modifier& modifier )
{
	bool success = false;

	const PContentDb::ModifierEntry* entry = gPContentDb.FindModifier( name );
	if ( entry != NULL )
	{
		success = true;
		modifier.m_ToolTipColor  = entry->GetToolTipColor ();
		modifier.m_ToolTipWeight = entry->GetToolTipWeight();
	}

	return ( success );
}

bool GoMagic :: GetPrefixModifier( Modifier& modifier )
{
	return ( FillModifier( m_PrefixModifierName.c_str(), modifier ) );
}

bool GoMagic :: GetSuffixModifier( Modifier& modifier )
{
	return ( FillModifier( m_SuffixModifierName.c_str(), modifier ) );
}

GoComponent* GoMagic :: Clone( Go* newParent )
{
	return ( new GoMagic( *this, newParent ) );
}

bool GoMagic :: Xfer( FuBi::PersistContext& persist )
{
	// xfer options
	persist.XferOption( "requires_line_of_sight", *this, OPTION_REQUIRES_LINE_OF_SIGHT	);
	persist.XferOption( "is_one_shot",            *this, OPTION_IS_ONE_SHOT				);
	persist.XferOption( "command_cast",           *this, OPTION_COMMAND_CAST			);

	// xfer members
	persist.Xfer	  ( "target_type_flags",	  m_TargetTypeFlags		);
	persist.Xfer	  ( "target_type_flags_not",  m_TargetTypeFlagsNot	);
	persist.Xfer	  ( "charges",				  m_Charges );

	// xfer enchantments
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_PrefixModifierName", m_PrefixModifierName );
		persist.Xfer( "m_SuffixModifierName", m_SuffixModifierName );

		if ( persist.IsSaving() )
		{
			if ( m_EnchantmentStorage != NULL )
			{
				persist.EnterBlock( "m_EnchantmentStorage" );
				m_EnchantmentStorage->Xfer( persist );
				persist.LeaveBlock();
			}
		}
		else
		{
			gpassert( m_EnchantmentStorage == NULL );
			if ( persist.EnterBlock( "m_EnchantmentStorage" ) )
			{
				m_EnchantmentStorage = new EnchantmentStorage;
				m_EnchantmentStorage->Xfer( persist );
			}
			persist.LeaveBlock();
		}
	}
	else
	{
		// only build storage if this is initial creation
		if ( persist.IsRestoring() && !GetGo()->HasValidGoid() )
		{
			gpassert( m_EnchantmentStorage == NULL );
			EnchantmentStorage* enchantments = NULL;

			// get source enchantments
			FastFuelHandle fuel = GetData()->GetInternalField( "enchantments" );
			if ( fuel )
			{
				enchantments = gContentDb.AddOrFindEnchantmentStorage( fuel ).Get();
			}

			// copy local if any
			if ( enchantments != NULL )
			{
				m_EnchantmentStorage = new EnchantmentStorage( *enchantments );
			}
		}
	}

	return ( true );
}

void GoMagic :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	switch ( msg.GetEvent() )
	{
		case ( WE_SPELL_COLLISION ):
		{
			// "If -I- was successfully cast on something..."

			if( gServer.IsLocal() )
			{
				gpassertm( GetGo()->IsSpell(), "If I'm not a spell, why am I receiving a cast_success message?" );
				// the client to this spell is the parent: client[has a]->spell
				Go * pClient;
				GetGo()->GetParent( pClient );
				gRules.DamageGoMagic( pClient ? pClient->GetGoid() : GetGo()->GetGoid(), GetGo()->GetGoid(), msg.GetSendFrom(), _ToSiegePosRef(msg.GetData1()) );
			}
		}
		break;
	}
}

DWORD GoMagic :: GoMagicToNet( GoMagic* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoMagic* GoMagic :: NetToGoMagic( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetMagic() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
