//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDefend.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoDefend.h"

#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoData.h"
#include "GoSupport.h"

DECLARE_GO_COMPONENT( GoDefend, "defend" );

//////////////////////////////////////////////////////////////////////////////
// class GoDefend implementation

GoDefend :: GoDefend( Go* parent )
	: Inherited( parent )
{
	m_Options     = OPTION_NONE;
	m_DefendClass = DC_SKIN;
}

GoDefend :: GoDefend( const GoDefend& source, Go* parent )
	: Inherited   ( source, parent      )
	, m_ArmorType ( source.m_ArmorType  )
	, m_ArmorStyle( source.m_ArmorStyle )
	, m_Defense   ( source.m_Defense    )
	, m_BlockDamage			   ( source.m_BlockDamage )
	, m_BlockMeleeDamage	   ( source.m_BlockMeleeDamage )
	, m_BlockRangedDamage	   ( source.m_BlockRangedDamage )
	, m_BlockCombatMagicDamage ( source.m_BlockCombatMagicDamage )
	, m_BlockNatureMagicDamage ( source.m_BlockNatureMagicDamage )
	, m_BlockPiercingDamage    ( source.m_BlockPiercingDamage )
	, m_BlockPartDamage			   ( source.m_BlockPartDamage )
	, m_BlockPartMeleeDamage	   ( source.m_BlockPartMeleeDamage )
	, m_BlockPartRangedDamage	   ( source.m_BlockPartRangedDamage )
	, m_BlockPartCombatMagicDamage ( source.m_BlockPartCombatMagicDamage )
	, m_BlockPartNatureMagicDamage ( source.m_BlockPartNatureMagicDamage )
	, m_BlockPartPiercingDamage    ( source.m_BlockPartPiercingDamage )
	, m_BlockMeleeDamageChance		 ( source.m_BlockMeleeDamageChance )
	, m_BlockRangedDamageChance	 	 ( source.m_BlockRangedDamageChance )
	, m_BlockCombatMagicChance		 ( source.m_BlockCombatMagicChance )
	, m_BlockNatureMagicChance		 ( source.m_BlockNatureMagicChance )
	, m_BlockMeleeDamageChanceAmount ( source.m_BlockMeleeDamageChanceAmount )
	, m_BlockRangedDamageChanceAmount( source.m_BlockRangedDamageChanceAmount )
	, m_BlockCombatMagicChanceAmount ( source.m_BlockCombatMagicChanceAmount )
	, m_BlockNatureMagicChanceAmount ( source.m_BlockNatureMagicChanceAmount )
	, m_ChanceToDodgeHit	   ( source.m_ChanceToDodgeHit )
	, m_ChanceToDodgeHitMelee  ( source.m_ChanceToDodgeHitMelee )
	, m_ChanceToDodgeHitRanged ( source.m_ChanceToDodgeHitRanged )
	, m_ReflectDamageAmount	   ( source.m_ReflectDamageAmount )
	, m_ReflectDamageChance	   ( source.m_ReflectDamageChance )
	, m_ReflectPiercingDamageAmount	   ( source.m_ReflectPiercingDamageAmount )
	, m_ReflectPiercingDamageChance	   ( source.m_ReflectPiercingDamageChance )
	, m_ManaShield			   ( source.m_ManaShield )
{
	m_Options     = source.m_Options;
	m_DefendClass = source.m_DefendClass;
}

GoDefend :: ~GoDefend( void )
{
	// this space intentionally left blank...
}

float GoDefend :: GetDamageThreshold( void ) const
{
	static AutoConstQuery <float> s_Query( "damage_threshold" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoDefend :: GetBlockCustomDamage( const char * damageType ) const
{
	BlockCustomDamageMap::const_iterator findBlock = m_BlockCustomDamage.find( damageType );

	if ( findBlock != m_BlockCustomDamage.end() )
	{
		return ( findBlock->second );
	}
	else
	{
		return ( 0.0f );
	}
}

void GoDefend :: SetBlockCustomDamage( const char * damageType, float value, bool cumulative )
{
	BlockCustomDamageMap::iterator findBlock = m_BlockCustomDamage.find( damageType );

	if ( findBlock == m_BlockCustomDamage.end() )
	{
		m_BlockCustomDamage.insert( BlockCustomDamageMap::value_type( damageType, value ) );
	}
	else
	{
		if ( !cumulative )
		{
			findBlock->second = value;
		}
		else
		{
			findBlock->second += value;
		}
	}
}

float GoDefend :: GetBlockPartCustomDamage( const char * damageType ) const
{
	BlockCustomDamageMap::const_iterator findBlock = m_BlockPartCustomDamage.find( damageType );

	if ( findBlock != m_BlockPartCustomDamage.end() )
	{
		return ( findBlock->second );
	}
	else
	{
		return ( 0.0f );
	}
}

void GoDefend :: SetBlockPartCustomDamage( const char * damageType, float value, bool cumulative )
{
	BlockCustomDamageMap::iterator findBlock = m_BlockPartCustomDamage.find( damageType );

	if ( findBlock == m_BlockPartCustomDamage.end() )
	{
		m_BlockPartCustomDamage.insert( BlockCustomDamageMap::value_type( damageType, value ) );
	}
	else
	{
		if ( !cumulative )
		{
			findBlock->second = value;
		}
		else
		{
			findBlock->second += value;
		}
	}
}

GoComponent* GoDefend :: Clone( Go* newParent )
{
	return ( new GoDefend( *this, newParent ) );
}

bool GoDefend :: Xfer( FuBi::PersistContext& persist )
{
	// xfer options
	persist.XferOption( "is_critical_hit_immune", *this, OPTION_IS_CRITICAL_HIT_IMMUNE );

	// xfer members
	persist.Xfer( "armor_type",   m_ArmorType   );
	persist.Xfer( "armor_style",  m_ArmorStyle  );
	persist.Xfer( "defend_class", m_DefendClass );
	persist.Xfer( "defense",      m_Defense     );

	if ( persist.IsFullXfer() )
	{
		// modifiers
		persist.Xfer( "m_BlockDamage",						m_BlockDamage );
		persist.Xfer( "m_BlockMeleeDamage",					m_BlockMeleeDamage );
		persist.Xfer( "m_BlockRangedDamage",				m_BlockRangedDamage );
		persist.Xfer( "m_BlockCombatMagicDamage",			m_BlockCombatMagicDamage );
		persist.Xfer( "m_BlockNatureMagicDamage",			m_BlockNatureMagicDamage );
		persist.Xfer( "m_BlockPiercingDamage",				m_BlockPiercingDamage );
		persist.Xfer( "m_BlockPartDamage",					m_BlockPartDamage );
		persist.Xfer( "m_BlockPartMeleeDamage",				m_BlockPartMeleeDamage );
		persist.Xfer( "m_BlockPartRangedDamage",			m_BlockPartRangedDamage );
		persist.Xfer( "m_BlockPartCombatMagicDamage",		m_BlockPartCombatMagicDamage );
		persist.Xfer( "m_BlockPartNatureMagicDamage",		m_BlockPartNatureMagicDamage );
		persist.Xfer( "m_BlockPartPiercingDamage",			m_BlockPartPiercingDamage );
		persist.Xfer( "m_BlockMeleeDamageChance",			m_BlockMeleeDamageChance );
		persist.Xfer( "m_BlockRangedDamageChance",			m_BlockRangedDamageChance );
		persist.Xfer( "m_BlockCombatMagicChance",			m_BlockCombatMagicChance );
		persist.Xfer( "m_BlockNatureMagicChance",			m_BlockNatureMagicChance );
		persist.Xfer( "m_BlockMeleeDamageChanceAmount",		m_BlockMeleeDamageChanceAmount );
		persist.Xfer( "m_BlockRangedDamageChanceAmount",	m_BlockRangedDamageChanceAmount );
		persist.Xfer( "m_BlockCombatMagicChanceAmount",		m_BlockCombatMagicChanceAmount );
		persist.Xfer( "m_BlockNatureMagicChanceAmount",		m_BlockNatureMagicChanceAmount );
		persist.Xfer( "m_ChanceToDodgeHit",					m_ChanceToDodgeHit );
		persist.Xfer( "m_ChanceToDodgeHitMelee",			m_ChanceToDodgeHitMelee );
		persist.Xfer( "m_ChanceToDodgeHitRanged",			m_ChanceToDodgeHitRanged );
		persist.Xfer( "m_ReflectDamageAmount",				m_ReflectDamageAmount );
		persist.Xfer( "m_ReflectDamageChance",				m_ReflectDamageChance );
		persist.Xfer( "m_ReflectPiercingDamageAmount",		m_ReflectPiercingDamageAmount );
		persist.Xfer( "m_ReflectPiercingDamageChance",		m_ReflectPiercingDamageChance );
		persist.Xfer( "m_ManaShield",						m_ManaShield );

		persist.XferMap( "m_BlockCustomDamage",				m_BlockCustomDamage );
		persist.XferMap( "m_BlockPartCustomDamage",			m_BlockPartCustomDamage );
	}

	return ( true );
}

bool GoDefend :: CommitCreation( void )
{
	bool success = true;

	// set armor for "on the ground" aspect
	const gpstring& armorType = GetArmorType();
	const gpstring& armorStyle = GetArmorStyle();
	if ( !armorType.empty() || !armorStyle.empty() )
	{
		// check that we have aspect
		if ( !GetGo()->HasAspect() )
		{
			gperror( "Error: defend component has armor specified but there is no aspect component!\n" );
			return ( false );
		}

		// check that we have gui
		if ( !GetGo()->HasGui() )
		{
			gperror( "Error: defend component has armor specified but there is no gui component!\n" );
			return ( false );
		}

		// check for preset
		if ( !GetGo()->GetAspect()->HasAspectHandle() )
		{
			// set aspect
			success = GoBody::InstantiateDeformableUnwornArmor( GetGo() );
		}
	}

	return ( success );
}

void GoDefend :: ResetModifiers( void )
{
	m_Defense.Reset();
	m_BlockDamage.Reset();
	m_BlockMeleeDamage.Reset();
	m_BlockRangedDamage.Reset();
	m_BlockNatureMagicDamage.Reset();
	m_BlockCombatMagicDamage.Reset();
	m_BlockPiercingDamage.Reset();
	m_BlockPartDamage.Reset();
	m_BlockPartMeleeDamage.Reset();
	m_BlockPartRangedDamage.Reset();
	m_BlockPartNatureMagicDamage.Reset();
	m_BlockPartCombatMagicDamage.Reset();
	m_BlockPartPiercingDamage.Reset();
	m_BlockMeleeDamageChance.Reset();
	m_BlockRangedDamageChance.Reset();
	m_BlockCombatMagicChance.Reset();
	m_BlockNatureMagicChance.Reset();
	m_BlockMeleeDamageChanceAmount.Reset();
	m_BlockRangedDamageChanceAmount.Reset();
	m_BlockCombatMagicChanceAmount.Reset();
	m_BlockNatureMagicChanceAmount.Reset();
	m_ChanceToDodgeHit.Reset();
	m_ChanceToDodgeHitMelee.Reset();
	m_ChanceToDodgeHitRanged.Reset();
	m_ReflectDamageAmount.Reset();
	m_ReflectDamageChance.Reset();
	m_ReflectPiercingDamageAmount.Reset();
	m_ReflectPiercingDamageChance.Reset();
	m_ManaShield.Reset();

	m_BlockCustomDamage.clear();
	m_BlockPartCustomDamage.clear();
}

DWORD GoDefend :: GoDefendToNet( GoDefend* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoDefend* GoDefend :: NetToGoDefend( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetDefend() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
