//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoAttack.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoAttack.h"
#include "GoParty.h"

#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoPhysics.h"
#include "GoInventory.h"
#include "GoDb.h"
#include "GoMagic.h"
#include "GoShmo.h"
#include "GoSupport.h"
#include "nema_aspect.h"
#include "Physcalc.h"
#include "Rules.h"
#include "SkritEngine.h"
#include "SkritSupport.h"
#include "World.h"
#include "WorldFx.h"

DECLARE_GO_COMPONENT( GoAttack, "attack" );

//////////////////////////////////////////////////////////////////////////////

SKRIT_IMPORT void OnAddCustomDamage( Skrit::HObject skrit, const char* funcName, Go* /*owner*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( OnAddCustomDamage, skrit, funcName );  }

SKRIT_IMPORT void OnRemoveCustomDamage( Skrit::HObject skrit, const char* funcName )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( OnRemoveCustomDamage, skrit, funcName );  }

GoAttack :: CustomDamage :: ~CustomDamage()
{
	if ( m_Skrit.IsValid() )
	{
		CHECK_SERVER_ONLY;

		::OnRemoveCustomDamage( m_Skrit, "on_remove_custom_damage$" );
		m_Skrit.ReleaseSkrit();
	}
}

FUBI_DECLARE_SELF_TRAITS( GoAttack::CustomDamage );

FUBI_DECLARE_XFER_TRAITS( GoAttack::CustomDamage* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new GoAttack::CustomDamage;
		}

		return ( obj->Xfer( persist ) );
	}
};

bool GoAttack :: CustomDamage :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_DamageType",    m_DamageType    );
	persist.Xfer( "m_bActive",       m_bActive       );
	persist.Xfer( "m_bMeleeDamage",  m_bMeleeDamage  );
	persist.Xfer( "m_bRangedDamage", m_bRangedDamage );
	persist.Xfer( "m_DamageChance",  m_DamageChance  );
	persist.Xfer( "m_DamageMin",     m_DamageMin     );
	persist.Xfer( "m_DamageMax",     m_DamageMax     );
	persist.Xfer( "m_Skrit",         m_Skrit         );

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// class GoAttack implementation

GoAttack :: GoAttack( Go* parent )
	: Inherited( parent )
{
	m_Options            = OPTION_NONE;
	m_AttackClass        = AC_BEASTFU;
	m_AmmoCloneSource    = GOID_INVALID;
	m_ProjectileLauncher = GOID_INVALID;
	m_AmmoReady			 = GOID_INVALID;
	m_AimingErrorX		 = 0;
	m_AimingErrorY		 = 0;

}

GoAttack :: GoAttack( const GoAttack& source, Go* parent )
	: Inherited( source, parent )
	, m_DamageMin( source.m_DamageMin )
	, m_DamageMax( source.m_DamageMax )
	, m_FiringPos( source.m_FiringPos )
	, m_TargetPos( source.m_TargetPos )
	, m_PiercingDamageChance( source.m_PiercingDamageChance )
	, m_PiercingDamageChanceMelee( source.m_PiercingDamageChanceMelee )
	, m_PiercingDamageChanceRanged( source.m_PiercingDamageChanceRanged )
	, m_PiercingDamageChanceAmount( source.m_PiercingDamageChanceAmount )
	, m_PiercingDamageChanceAmountMelee( source.m_PiercingDamageChanceAmountMelee )
	, m_PiercingDamageChanceAmountRanged( source.m_PiercingDamageChanceAmountRanged )
	, m_PiercingDamageMin	( source.m_PiercingDamageMin )
	, m_PiercingDamageMax	( source.m_PiercingDamageMax )
	, m_PiercingDamageMeleeMin	( source.m_PiercingDamageMeleeMin )
	, m_PiercingDamageMeleeMax	( source.m_PiercingDamageMeleeMax )
	, m_PiercingDamageRangedMin	( source.m_PiercingDamageRangedMin )
	, m_PiercingDamageRangedMax	( source.m_PiercingDamageRangedMax )
	, m_ChanceToHitBonus	( source.m_ChanceToHitBonus )
	, m_ChanceToHitBonusMelee	( source.m_ChanceToHitBonusMelee )
	, m_ChanceToHitBonusRanged	( source.m_ChanceToHitBonusRanged )
	, m_ExperienceBonus		( source.m_ExperienceBonus )
	, m_LifeStealAmount		( source.m_LifeStealAmount )
	, m_ManaStealAmount		( source.m_ManaStealAmount )
	, m_LifeBonusAmount		( source.m_LifeBonusAmount )
	, m_ManaBonusAmount		( source.m_ManaBonusAmount )
	, m_DamageToUndead		( source.m_DamageToUndead )
	, m_DamageToType		( source.m_DamageToType )
	, m_AmountDamageToType	( source.m_AmountDamageToType )
{
	m_Options            = source.m_Options;
	m_AttackClass        = source.m_AttackClass;
	m_AmmoCloneSource    = source.m_AmmoCloneSource;
	m_ProjectileLauncher = source.m_ProjectileLauncher;
	m_AmmoReady			 = source.m_AmmoReady;
	m_AimingErrorX       = source.m_AimingErrorX;
	m_AimingErrorY       = source.m_AimingErrorY;

	m_DamageBonusMinMelee		= source.m_DamageBonusMinMelee;
	m_DamageBonusMaxMelee		= source.m_DamageBonusMaxMelee;
	m_DamageBonusMinRanged		= source.m_DamageBonusMinRanged;
	m_DamageBonusMaxRanged		= source.m_DamageBonusMaxRanged;
	m_DamageBonusMinCMagic		= source.m_DamageBonusMinCMagic;
	m_DamageBonusMaxCMagic		= source.m_DamageBonusMaxCMagic;
	m_DamageBonusMinNMagic		= source.m_DamageBonusMinNMagic;
	m_DamageBonusMaxNMagic		= source.m_DamageBonusMaxNMagic;

	// lock our clone sources into memory
	gGoDb.LockCloneSource( m_AmmoCloneSource, GetGo() );
}

GoAttack :: ~GoAttack( void )
{
	// this space intentionally left blank...
}

const gpstring& GoAttack :: GetSkillClass( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "skill_class" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetAttackRange( void ) const
{
	static AutoConstQuery <float> s_Query( "attack_range" );
	return ( s_Query.Get( GetData()->GetRecord() ) + gSim.GetSniperRange() );
}

float GoAttack :: GetReloadDelay( void ) const
{
	static AutoConstQuery <float> s_Query( "reload_delay" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetCriticalHitChance( void ) const
{
	static AutoConstQuery <float> s_Query( "critical_hit_chance" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

// ranged
float GoAttack :: GetAreaDamageRadius( void ) const
{
	static AutoConstQuery <float> s_Query( "area_damage_radius" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}


bool GoAttack :: GetUseAimingError( void ) const
{
	static AutoConstQuery <bool> s_Query( "use_aiming_error" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetWeaponErrorRangeX( void ) const
{
	static AutoConstQuery <float> s_Query( "aiming_error_range_x" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetWeaponErrorRangeY( void ) const
{
	static AutoConstQuery <float> s_Query( "aiming_error_range_y" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}


const gpstring& GoAttack :: GetAmmoAttachBone( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "ammo_attach_bone" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoAttack :: GetAmmoAttachesToWeapon( void ) const
{
	static AutoConstQuery <bool> s_Query( "ammo_attaches_to_weapon" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoAttack :: GetAmmoAppearsJIT( void ) const
{
	static AutoConstQuery <bool> s_Query( "ammo_appears_jit" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}


// melee
float GoAttack :: GetMeleeAimPiTime( void ) const
{
	static AutoConstQuery <float> s_Query( "melee_aim_pi_time" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetMeleeSwingTime( void ) const
{
	static AutoConstQuery <float> s_Query( "melee_swing_time" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetMeleeRecoverTime( void ) const
{
	static AutoConstQuery <float> s_Query( "melee_recover_time" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetMeleeFidgetTime( void ) const
{
	static AutoConstQuery <float> s_Query( "melee_fidget_time" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAttack :: GetDamageMin( void ) const
{
	return ( m_DamageMin * gRules.GetDifficultyFactor( GetGo() ) );
}

float GoAttack :: GetDamageMinNatural( void ) const
{
	return ( m_DamageMin.GetNatural() * gRules.GetDifficultyFactor( GetGo() ) );
}

float GoAttack :: GetDamageMax( void ) const
{
	return ( m_DamageMax * gRules.GetDifficultyFactor( GetGo() ) );
}

void GoAttack :: EvaluateTotalMinMaxDamage( float & minDamage, float & maxDamage, bool bUseDifficulty )
{
	if ( bUseDifficulty )
	{
		minDamage = GetDamageMinNatural();
		maxDamage = GetDamageMaxNatural();
	}
	else
	{
		minDamage = GetBaseDamageMinNatural();
		maxDamage = GetBaseDamageMaxNatural();
	}

	if ( GetGo()->HasMagic() )
	{
		if ( GetGo()->GetMagic()->HasEnchantments() )
		{
			const EnchantmentVector& enchVec = GetGo()->GetMagic()->GetEnchantmentStorage().GetEnchantments();
			EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
			for ( i = begin ; i != end ; ++i )
			{
				const Enchantment* ench = *i;
				if( GetGo()->IsMeleeWeapon() )
				{
					if ( ench->GetAlterationType() == ALTER_MELEE_DAMAGE_MIN )
					{
						minDamage += ench->GetValue();
					}
					else if ( ench->GetAlterationType() == ALTER_MELEE_DAMAGE_MAX )
					{
						maxDamage += ench->GetValue();
					}
				}
				else if ( GetGo()->IsRangedWeapon() )
				{
					if ( ench->GetAlterationType() == ALTER_RANGED_DAMAGE_MIN )
					{
						minDamage += ench->GetValue();
					}
					else if ( ench->GetAlterationType() == ALTER_RANGED_DAMAGE_MAX )
					{
						maxDamage += ench->GetValue();
					}
				}

				// $$$ Remove when piercing is made part of custom
				if ( ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE || 
					 ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE_MELEE ||
					 ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE_RANGED )
				{
					minDamage += ench->GetValue();
					maxDamage += ench->GetMaxValue();
				}

				if ( ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE || 
					 ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE_MELEE ||
					 ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE_RANGED )
				{
					minDamage += ench->GetValue();
					maxDamage += ench->GetMaxValue();
				}
			}
		}
	}
}

void  GoAttack :: EvaluateCustomMinMaxDamage( float & minDamage, float & maxDamage )
{
	if ( GetGo()->HasMagic() )
	{
		if ( GetGo()->GetMagic()->HasEnchantments() )
		{
			const EnchantmentVector& enchVec = GetGo()->GetMagic()->GetEnchantmentStorage().GetEnchantments();
			EnchantmentVector::const_iterator i, begin = enchVec.begin(), end = enchVec.end();
			for ( i = begin ; i != end ; ++i )
			{
				const Enchantment* ench = *i;
				if ( ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE || 
					 ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE_MELEE ||
					 ench->GetAlterationType() == ALTER_PIERCING_DAMAGE_RANGE_RANGED )
				{
					minDamage += ench->GetValue();
					maxDamage += ench->GetMaxValue();
				}

				// $$$ Remove when piercing is made part of custom
				if ( ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE || 
					 ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE_MELEE ||
					 ench->GetAlterationType() == ALTER_CUSTOM_DAMAGE_RANGED )
				{
					minDamage += ench->GetValue();
					maxDamage += ench->GetMaxValue();
				}
			}
		}
	}
}

float GoAttack :: GetDamageMaxNatural( void ) const
{
	return ( m_DamageMax.GetNatural() * gRules.GetDifficultyFactor( GetGo() ) );
}

GoAttack::CustomDamage* GoAttack :: GetCustomDamage( const char* damageType, bool createIfNeeded )
{
	if ( createIfNeeded )
	{
		std::pair <CustomDamageMap::iterator, bool> rc
				= m_CustomDamageMap.insert( CustomDamageMap::value_type( damageType, NULL ) );
		if ( rc.second )
		{
			rc.first->second = new CustomDamage;
		}
		return ( rc.first->second );
	}
	else
	{
		CustomDamageMap::iterator found = m_CustomDamageMap.find( damageType );
		return ( (found == m_CustomDamageMap.end()) ? NULL : found->second );
	}
}

const GoAttack::CustomDamage* GoAttack :: GetCustomDamage( const char* damageType ) const
{
	CustomDamageMap::const_iterator found = m_CustomDamageMap.find( damageType );
	return ( (found == m_CustomDamageMap.end()) ? NULL : found->second );
}

void GoAttack :: SetCustomDamage( const char* damageType, const char* attackType, float damageMin, float damageMax, const char* skritName, bool cumulative )
{
	gpstring damageAttackType = damageType;
	if ( *attackType != '\0' )
	{
		damageAttackType += '_';
		damageAttackType += attackType;
	}

	CustomDamage * pDamage = GetCustomDamage( damageAttackType, true );

	pDamage->m_DamageType = damageType;
	pDamage->m_bActive = true;

	if ( same_no_case( attackType, "melee" ) )
	{
		pDamage->m_bMeleeDamage = true;
	}
	else if ( same_no_case( attackType, "ranged" ) )
	{
		pDamage->m_bRangedDamage = true;
	}

	if ( !cumulative )
	{
		pDamage->m_DamageMin = 0.0f;
		pDamage->m_DamageMax = 0.0f;
	}

	pDamage->m_DamageMin += damageMin;
	pDamage->m_DamageMax += damageMax;

	// $ skrit can only run on server!
	if ( ::IsServerLocal() )
	{
		if ( (*skritName != '\0') && !(pDamage->m_Skrit.IsValid() && !same_no_case( skritName, pDamage->m_Skrit.GetName() )) )
		{
			if ( pDamage->m_Skrit.IsValid() )
			{
				pDamage->m_Skrit.ReleaseSkrit();
			}

			// attempt to compile the skrit
			pDamage->m_Skrit.CreateSkrit( Skrit::CreateReq( skritName ) );
			if ( pDamage->m_Skrit.IsValid() )
			{
				FuBi::Record* record = pDamage->m_Skrit->GetRecord();
				if ( record != NULL )
				{
					record->SetXferMode( GP_DEBUG ? FuBi::XMODE_STRICT : FuBi::XMODE_QUIET );
				}

				::OnAddCustomDamage( pDamage->m_Skrit, "on_add_custom_damage$", GetGo() );
			}
			else
			{
				gperrorf(( "The skrit for damage type '%s' failed to compile.", damageType ));
			}
		}
	}
}

float GoAttack :: GetCustomDamageChance( const char * damageType ) const
{
	const CustomDamage * pDamage = GetCustomDamage( damageType );
	return ( pDamage ? pDamage->m_DamageChance : 0 );
}

void GoAttack :: SetCustomDamageChance( const char * damageType, float damageChance, bool cumulative )
{
	CustomDamage * pDamage = GetCustomDamage( damageType, true );
	if ( !cumulative )
	{
		pDamage->m_DamageChance = 0;
	}
	pDamage->m_DamageChance += damageChance;
}

float GoAttack :: GetCustomDamageMin( const char * damageType ) const
{
	const CustomDamage * pDamage = GetCustomDamage( damageType );
	return ( pDamage ? pDamage->m_DamageMin : 0 );
}


float GoAttack :: GetCustomDamageMax( const char * damageType ) const
{
	const CustomDamage * pDamage = GetCustomDamage( damageType );
	return ( pDamage ? pDamage->m_DamageMax : 0 );
}

SiegePos & GoAttack :: ComputeFiringPos( void )
{
	Go * pShooter = GetShooter();
	GoHandle pAmmo( m_AmmoReady );

	if ( pAmmo )
	{
		vector_3	ammoOffset(DoNotInitialize);
		Quat		tempRotation(DoNotInitialize);
		pAmmo->GetAspect()->GetAspectHandle()->GetPrimaryBoneOrientation( ammoOffset, tempRotation );

		float renderscale = pShooter->HasAspect() ? pShooter->GetAspect()->GetRenderScale() : 1.0f;

		vector_3 scaledpos(DoNotInitialize);

		Go* pParent = pShooter->GetParent();
		if (pParent && pParent->HasAspect() )
		{
			scaledpos = (renderscale * pParent->GetAspect()->GetRenderScale()) * ammoOffset;
		}
		else
		{
			scaledpos = renderscale * ammoOffset;
		}

		m_FiringPos = pShooter->GetPlacement()->GetPosition();
		m_FiringPos.pos += pShooter->GetPlacement()->GetOrientation().RotateVector_T(scaledpos);
	}
	else
	{
		m_FiringPos = SiegePos();
	}

	return m_FiringPos;
}

Go* GoAttack :: GetShooter( void )
{
	Go* shooter = GetGo();

	if( shooter->HasParent() && shooter->GetParent()->HasActor() )
	{
		shooter = shooter->GetParent();
	}

	return ( shooter );
}

SiegePos & GoAttack :: ComputeTargetPos( Goid target )
{
	GoHandle hTarget( target );

	if( hTarget )
	{
		m_TargetPos = hTarget->GetPlacement()->GetPosition();

		if( hTarget->HasActor() )
		{
			hTarget->GetBody()->GetBoneTranslator().GetPosition( hTarget, BoneTranslator::KILL_BONE, m_TargetPos );
		}
		else
		{
			hTarget->GetAspect()->GetNodeSpaceCenter( m_TargetPos.pos );
		}
	}

	return m_TargetPos;
}

bool GoAttack :: IsInProjectileRange( float firing_velocity, const SiegePos &target_position )
{
	if( gSim.GetSniperRange() > 0 )
	{
		return true;
	}

	GoHandle hAmmo( m_AmmoReady );

	if( hAmmo )
	{
		SiegePos firing_pos( ComputeFiringPos() );

		const vector_3 diff( gSiegeEngine.GetDifferenceVector( firing_pos, target_position ) );
		const float gravity = hAmmo->GetPhysics()->GetGravity();

		return Physcalc::IsInRange( diff, firing_velocity, gravity );
	}
	return false;
}

void GoAttack :: ComputeAimingError( void )
{
	Go * pShooter	= GetShooter();
	Go * pWeapon 	= GetGo();

	if( (gSim.GetSniperRange() > 0) && pShooter->IsAnyHumanPartyMember() )
	{
		m_AimingErrorX = m_AimingErrorY = 0;
		return;
	}

	// Determine the aiming error of the shooter based on the weapon and/or skill
	if( pWeapon->HasAttack() && GetUseAimingError() )
	{
		gRules.CalculateWeaponAimingError( pWeapon->GetGoid(), m_AimingErrorX, m_AimingErrorY );
	}
	else
	{
		gRules.CalculateAimingError( pShooter->GetGoid(), "ranged", m_AimingErrorX, m_AimingErrorY );
	}
}

float GoAttack :: ComputeAimingAngle( const SiegePos &firingPos, const SiegePos &targetPos, float velocity )
{
	return ( ComputeAimingAngle( m_AmmoReady, firingPos, targetPos, velocity ) );
}

float GoAttack :: ComputeAimingAngle( Goid ammo, const SiegePos &firingPos, const SiegePos &targetPos, float velocity )
{
	GoHandle pAmmo( ammo );

	float angle = 0;

	if( pAmmo )
	{
		const vector_3 diff	= gSiegeEngine.GetDifferenceVector( firingPos, targetPos );
		const float gravity = pAmmo->GetPhysics()->GetGravity();

		if( (gSim.GetSniperRange() > 0 ) && pAmmo->HasParent() )
		{
			if( pAmmo->GetParent()->HasParent() && pAmmo->GetParent()->GetParent()->IsAnyHumanPartyMember() )
			{
				const float delta_velocity = velocity * 0.15f;
				while( !Physcalc::IsInRange( diff, velocity, gravity )  )
				{
					velocity += delta_velocity;
				}
			}
		}

		Physcalc::CalculateFiringAngle( diff, velocity, gravity, angle );
	}
	return angle;
}

Goid GoAttack :: SPrepareAmmo( void )
{
	CHECK_SERVER_ONLY;

	if ( (m_AmmoCloneSource != GOID_INVALID) && (m_AmmoReady == GOID_INVALID) )
	{
		GoCloneReq req( GetGoid(), m_AmmoCloneSource );
		req.m_PrepareAmmo = true;
		gGoDb.SCloneGo( req );
	}

	return ( m_AmmoReady );
}

//$$ Hey Mike use or delete this function after you fix bug 6802
void SetAmmoScale( Go * weapon, Go * ammo )
{
	// set the scale of the ammo to that of the weapon using the weapon_scales field
	if( weapon->HasParent() )
	{
		Go * shooter = weapon->GetParent();

		if( shooter->HasBody() )
		{
			FastFuelHandle scales = shooter->GetBody()->GetData()->GetInternalField( "weapon_scales" );
			gpstring scaleText;
			vector_3 scale;
			if ( scales && scales.Get( ToString( AS_BOW_AND_ARROW ), scaleText ) && ::FromString( scaleText, scale ) )
			{
				ammo->GetAspect()->GetAspectHandle()->SetCurrentScale( scale );
			}
		}
	}
}

void GoAttack :: PrepareAmmo( Go* ammo )
{
	gpassert( ammo != NULL );

	// special checking to prevent a certain type of skrit coding error from
	// coming up...
#	if !GP_RETAIL
	if ( GetGo()->HasParent() && GetGo()->GetParent()->HasActor() && GetGo()->IsInsideInventory() )
	{
		GoInventory* inv = GetGo()->GetParent()->GetInventory();

		// if we're a spell, then we must be currently active
		if ( GetGo()->IsSpell() )
		{
			if ( inv->GetSelectedSpell() != GetGo() )
			{
				gperrorf(( "RCPrepareAmmo() being called by a non-selected spell!!" ));
			}
		}
		else
		{
			// otherwise we must be an equipped item
			if ( !inv->IsEquipped( GetGo() ) )
			{
				gperrorf(( "RCPrepareAmmo() being called by a non-equipped weapon!!" ));
			}
		}
	}
#	endif // !GP_RETAIL

	m_AmmoReady = ammo->GetGoid();

	nema::HAspect hasp_ammo = ammo->GetAspect()->GetAspectHandle();

	// make sure ammo is there to be attached!! (#10689)
	// only reconstitute if we need to
	if( hasp_ammo->IsPurged( 2 ) )
	{
		hasp_ammo->Reconstitute( 1, true );
	}

	if( GetAmmoAttachesToWeapon() )
	{
		// AMMO attaches to -> WEAPON using AMMO BONE0 to WEAPONs [ammo_attach_bone]
		ammo->GetPlacement()->CopyPosition( *GetGo()->GetPlacement() );

		// make sure the ammo is scaled properly just like the weapon already had done at the time of equip
		SetAmmoScale( GetGo(), ammo );

		nema::HAspect hasp	= GetGo()->GetAspect()->GetAspectHandle();
		if( hasp->IsPurged( 2 ) )
		{
			hasp->ReconstituteTop( 1, true );
		}

		hasp->AttachRigidLinkedChild(
				hasp_ammo,
				GetAmmoAttachBone(), hasp_ammo->GetBoneName(0) );

	}
	else if( GetGo()->HasParent() && GetGo()->GetParent()->HasAspect() )
	{
		// AMMO attaches to -> SHOOTER/CASTER using AMMOs [ammo_attach_bone] to WEAPON/SPELL [ammo_attach_bone]
		ammo->GetPlacement()->CopyPosition( *GetGo()->GetParent()->GetPlacement() );

		// make sure the ammo is scaled properly just like the weapon already had done at the time of equip
		SetAmmoScale( GetGo(), ammo );

		nema::HAspect hasp_parent	= GetGo()->GetParent()->GetAspect()->GetAspectHandle();
		if( hasp_parent->IsPurged( 2 ) )
		{
			hasp_parent->ReconstituteTop( 1, true );
		}
		hasp_parent->AttachReverseLinkedChild(
				hasp_ammo,
				GetAmmoAttachBone(), ammo->GetAttack()->GetAmmoAttachBone() );
	}
}

void GoAttack :: SLaunchAmmo( float velocity, const SiegePos &firingPos, const SiegePos &targetPos, float x_error, float y_error, Goid intended_target )
{
	CHECK_SERVER_ONLY;

	GoHandle pAmmo( m_AmmoReady );

	if( pAmmo != NULL )
	{
		GoidColl ignoreList;

		// Determine if ammo is being shot from a weapon or just lobbed etc...
		Go * pShooter = GetShooter();

#		if !GP_RETAIL
		if (gWorldOptions.GetHudMCP())
		{
			SiegePos p = pShooter->GetPlacement()->GetPosition();
			p.pos.y += 2.0f*pShooter->GetAspect()->GetBoundingSphereRadius();
			gWorld.DrawDebugWorldLabelScroll(p,"FIRE",0xffffff00,3,true);
		}
#		endif // !GP_RETAIL

		if( pShooter == GetGo() )
		{
			ignoreList.push_back( pShooter->GetGoid() );
		}
		else if( ( pShooter->GetPlayer()->GetController() == PC_HUMAN ) && pShooter->HasParent() && pShooter->GetParent()->HasParty() )
		{
			// shooter in party, but target external to party
			pShooter->GetParent()->GetChildren().Translate( ignoreList );
		}
		else
		{
			// shooter is a bachelor, just ignore hitting self
			ignoreList.push_back( pShooter->GetGoid() );
		}

		// start any weapon hit effects that may be attached to this weapon

		GoMagic *pMagic = GetGo()->QueryMagic();

		if( pMagic && pMagic->HasEnchantments() )
		{
			gpstring sScriptName, sScriptParams;

			if( pMagic->GetEnchantmentStorage().GetFirstHitEffectInfo( sScriptName, sScriptParams ) )
			{
				gWorldFx.SRunScript( sScriptName, pAmmo->GetGoid(), GetGo()->GetGoid(), sScriptParams, pAmmo->GetGoid(), WE_WEAPON_LAUNCHED );
			}
		}

		// Have sim engine decide where to shoot it
		SimID sim_id = 0;
		gSim.SFireProjectile(
				sim_id, pAmmo, GetGo(), intended_target,
				ignoreList, firingPos, targetPos, velocity, x_error, y_error );
	}
}

void GoAttack :: RCLaunchAmmo( Go* ammo, Goid intended_target, SiegePos firing_position, vector_3 velocity_vector, DWORD seed )
{
	FUBI_RPC_THIS_CALL( RCLaunchAmmo, RPC_TO_ALL );

	gpassert( ammo != NULL );
	Goid ammoGoid = ammo->GetGoid();

	// Start the ammo simulating
	gSim.FireProjectile( ammoGoid, GetGoid(), firing_position, velocity_vector, seed );

	// Orient the ammo
	vector_3	ammoOffset(DoNotInitialize);
	Quat		ammoRotation(DoNotInitialize);

	if( !GetAmmoAppearsJIT() )
	{
		ammo->GetAspect()->GetAspectHandle()->GetPrimaryBoneOrientation( ammoOffset, ammoRotation );
		ammo->GetPlacement()->SetOrientation( ammo->GetPlacement()->GetOrientation() * ammoRotation );
	}

	SimID sim_id = gSim.GetSimID( ammoGoid );

	// Set params
	gSim.SetExplodeIfHitGo( sim_id, ammo->GetPhysics()->GetExplodeIfHitGo() );
	gSim.SetExplodeIfHitTerrain( sim_id, ammo->GetPhysics()->GetExplodeIfHitTerrain() );
	gSim.SetIntendedTarget( sim_id, intended_target );

	ammo->GetAttack()->SetProjectileLauncher( GetGo()->GetGoid() );
	ammo->GetPhysics()->SetSimulationId( sim_id );

	// Detach the ammo
	if( GetAmmoAttachesToWeapon() && ( ammo->GetAspect()->GetAspectHandle()->GetParent() == GetGo()->GetAspect()->GetAspectHandle() ) )
	{
		GetGo()->GetAspect()->GetAspectHandle()->DetachChild( ammo->GetAspect()->GetAspectHandle() );
	}
	else if( GetGo()->HasParent() && GetGo()->GetParent()->HasAspect() && ( ammo->GetAspect()->GetAspectHandle()->GetParent() == GetGo()->GetParent()->GetAspect()->GetAspectHandle() ) )
	{
		GetGo()->GetParent()->GetAspect()->GetAspectHandle()->DetachChild( ammo->GetAspect()->GetAspectHandle() );
	}

	// make sure the ammo's aspect is ready
	nema::HAspect hasp_ammo = ammo->GetAspect()->GetAspectHandle();
	if( hasp_ammo->IsPurged( 2 ) )
	{
		hasp_ammo->Reconstitute( 1, true );
	}
	if( !hasp_ammo->LockPrimaryBone() )
	{
		gperrorf(("GoAttack::RCLaunchAmmo - could not LockPrimaryBone for [%s] after reconstituting.\n", ammo->GetTemplateName()) );
	}

	// We no longer have this
	m_AmmoReady = GOID_INVALID;

	// Tell the ammo that it was launched, so that any triggers, sfx may activate
	WorldMessage( WE_WEAPON_LAUNCHED, GetShooter()->GetGoid(), ammoGoid, MakeInt( ammoGoid ) ).Send();
}

void GoAttack :: SUnprepareAmmo( void )
{
	CHECK_SERVER_ONLY;

	gGoDb.SMarkForDeletion( m_AmmoReady );
}

void GoAttack :: AlertRangedAttack( Goid target )
{
	Go * pShooter	= GetShooter();

	// Send the victim a message telling him he was attacked
	WorldMessage( WE_ATTACKED_RANGED, pShooter->GetGoid(), target, MakeInt( pShooter->GetGoid() ) ).Send();
}

GoComponent* GoAttack :: Clone( Go* newParent )
{
	return ( new GoAttack( *this, newParent ) );
}

bool GoAttack :: Xfer( FuBi::PersistContext& persist )
{
	// xfer options
	persist.XferOption( "requires_line_of_sight", *this, OPTION_REQUIRES_LINE_OF_SIGHT );
	persist.XferOption( "is_melee",               *this, OPTION_IS_MELEE               );
	persist.XferOption( "is_projectile",          *this, OPTION_IS_PROJECTILE          );
	persist.XferOption( "is_one_shot",            *this, OPTION_IS_ONE_SHOT            );
	persist.XferOption( "is_two_handed",          *this, OPTION_IS_TWO_HANDED          );

	// xfer members
	persist.Xfer( "attack_class",				m_AttackClass );
	persist.Xfer( "damage_min",					m_DamageMin );
	persist.Xfer( "damage_max",					m_DamageMax );

	// damage bonuses
	persist.Xfer( "damage_bonus_min_melee",		m_DamageBonusMinMelee  );
	persist.Xfer( "damage_bonus_max_melee",		m_DamageBonusMaxMelee  );
	persist.Xfer( "damage_bonus_min_ranged",	m_DamageBonusMinRanged );
	persist.Xfer( "damage_bonus_max_ranged",	m_DamageBonusMaxRanged );
	persist.Xfer( "damage_bonus_min_cmagic",	m_DamageBonusMinCMagic );
	persist.Xfer( "damage_bonus_max_cmagic",	m_DamageBonusMaxCMagic );
	persist.Xfer( "damage_bonus_min_nmagic",	m_DamageBonusMinNMagic );
	persist.Xfer( "damage_bonus_max_nmagic",	m_DamageBonusMaxNMagic );

	// special for ammo
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_AmmoReady",		  m_AmmoReady		   );
		persist.Xfer( "m_AmmoCloneSource",    m_AmmoCloneSource    );
		persist.Xfer( "m_ProjectileLauncher", m_ProjectileLauncher );
		persist.Xfer( "m_AimingErrorX",       m_AimingErrorX       );
		persist.Xfer( "m_AimingErrorY",       m_AimingErrorY       );
		persist.Xfer( "m_FiringPos",          m_FiringPos          );
		persist.Xfer( "m_TargetPos",          m_TargetPos          );

		// modifiers
		persist.Xfer( "m_PiercingDamageChance",					m_PiercingDamageChance );
		persist.Xfer( "m_PiercingDamageChanceMelee",			m_PiercingDamageChanceMelee );
		persist.Xfer( "m_PiercingDamageChanceRanged",			m_PiercingDamageChanceRanged );
		persist.Xfer( "m_PiercingDamageChanceAmount",			m_PiercingDamageChanceAmount );
		persist.Xfer( "m_PiercingDamageChanceAmountMelee",		m_PiercingDamageChanceAmountMelee );
		persist.Xfer( "m_PiercingDamageChanceAmountRanged",		m_PiercingDamageChanceAmountRanged );
		persist.Xfer( "m_PiercingDamageMin",					m_PiercingDamageMin );
		persist.Xfer( "m_PiercingDamageMax",					m_PiercingDamageMax );
		persist.Xfer( "m_PiercingDamageMeleeMin",				m_PiercingDamageMeleeMin );
		persist.Xfer( "m_PiercingDamageMeleeMax",				m_PiercingDamageMeleeMax );
		persist.Xfer( "m_PiercingDamageRangedMin",				m_PiercingDamageRangedMin );
		persist.Xfer( "m_PiercingDamageRangedMax",				m_PiercingDamageRangedMax );
		persist.Xfer( "m_ChanceToHitBonus",						m_ChanceToHitBonus );
		persist.Xfer( "m_ChanceToHitBonusMelee",				m_ChanceToHitBonusMelee );
		persist.Xfer( "m_ChanceToHitBonusRanged",				m_ChanceToHitBonusRanged );
		persist.Xfer( "m_ExperienceBonus",						m_ExperienceBonus );
		persist.Xfer( "m_LifeStealAmount",						m_LifeStealAmount );
		persist.Xfer( "m_ManaStealAmount",						m_ManaStealAmount );
		persist.Xfer( "m_LifeBonusAmount",						m_LifeBonusAmount );
		persist.Xfer( "m_ManaBonusAmount",						m_ManaBonusAmount );
		persist.Xfer( "m_DamageToType",							m_DamageToType );
		persist.Xfer( "m_AmountDamageToType",					m_AmountDamageToType );
		persist.Xfer( "m_DamageToUndead",						m_DamageToUndead );

		persist.XferMap( "m_CustomDamageMap",					m_CustomDamageMap );
	}
	else
	{
		if ( persist.IsRestoring() )
		{
			gpstring templateName;
			persist.Xfer( "ammo_template", templateName );
			if ( !templateName.empty() )
			{
				m_AmmoCloneSource = gGoDb.FindCloneSource( templateName );
			}
			else
			{
				m_AmmoCloneSource = GOID_INVALID;
			}
		}
	}

	return ( true );
}

void GoAttack :: Shutdown( void )
{
	// free memory
	stdx::for_each( m_CustomDamageMap, stdx::delete_second_by_ptr() );
}

void GoAttack :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.HasEvent( WE_EQUIPPED ) )
	{
		// prepare ammo on ranged weapons, but don't do this if we're being
		// equipped on a go that is still under construction because the server
		// will end up with ammo in its bucket and the client will not! instead,
		// try again upon entering the world.
		if (   ::IsServerLocal()
			&& GetGo()->IsRangedWeapon()
			&& !GetAmmoAppearsJIT()
			&& !gGoDb.IsCurrentThreadConstructingGo() )
		{
			SPrepareAmmo();
		}
	}
	else if ( msg.HasEvent( WE_ENTERED_WORLD ) )
	{
		// prepare ammo on ranged weapons if we were supposed to equip but
		// couldn't do it because we were part of a larger go (see previous func).
		if ( ::IsServerLocal() && GetGo()->IsRangedWeapon() && !GetAmmoAppearsJIT() && GetGo()->IsEquipped() )
		{
			if( GetGo()->GetParent() && GetGo()->GetParent()->GetAspect()->GetIsVisible() )	// fixes #10730
			{
				SPrepareAmmo();
			}
		}
	}
	else if ( msg.HasEvent( WE_UNEQUIPPED ) )
	{
		// unprepare any ammo that was created while using a ranged weapon
		if ( ::IsServerLocal() && GetGo()->IsRangedWeapon() )
		{
			SUnprepareAmmo();
		}
	}
	else if ( msg.HasEvent( WE_ANIM_WEAPON_SWUNG ) )
	{
		bool bPlayed = false;
		if ( GetGo()->HasInventory() )
		{
			Go * pMelee = GetGo()->GetInventory()->GetSelectedMeleeWeapon();
			if ( pMelee && msg.GetData1() == 0 )
			{
				pMelee->PlayVoiceSound( "attack" );
				bPlayed = true;
			}
		}

		if ( !bPlayed )
		{
			GetGo()->PlayVoiceSound( "attack" );
		}
	}
	else if ( msg.HasEvent( WE_ANIM_WEAPON_FIRE ) )
	{
		bool bPlayed = false;
		if ( GetGo()->HasInventory() )
		{
			Go * pRanged = GetGo()->GetInventory()->GetSelectedRangedWeapon();
			if ( pRanged )
			{
				pRanged->PlayVoiceSound( "attack" );
				bPlayed = true;
			}
		}
		
		if ( !bPlayed )
		{
			Go * pSelected = GetGo()->HasInventory() ? GetGo()->GetInventory()->GetSelectedItem() : NULL;
			if ( !pSelected || ( pSelected && !pSelected->IsSpell() ) )
			{
				GetGo()->PlayVoiceSound( "attack" );
			}
			else if ( pSelected && pSelected->IsSpell() )
			{
				// Some creatures have a specific cast/attack sound that needs to be called in the fire event and not in the scripts
				GetGo()->PlayVoiceSound( "cast" );
			}			
		}

	}
}

void GoAttack :: ResetModifiers( void )
{
	m_DamageMin.Reset();
	m_DamageMax.Reset();

	m_PiercingDamageChance.Reset();
	m_PiercingDamageChanceMelee.Reset();
	m_PiercingDamageChanceRanged.Reset();
	m_PiercingDamageChanceAmount.Reset();
	m_PiercingDamageChanceAmountMelee.Reset();
	m_PiercingDamageChanceAmountRanged.Reset();
	m_PiercingDamageMin.Reset();
	m_PiercingDamageMax.Reset();
	m_PiercingDamageMeleeMin.Reset();
	m_PiercingDamageMeleeMax.Reset();
	m_PiercingDamageRangedMin.Reset();
	m_PiercingDamageRangedMax.Reset();

	m_ChanceToHitBonus.Reset();
	m_ChanceToHitBonusMelee.Reset();
	m_ChanceToHitBonusRanged.Reset();

	m_ExperienceBonus.Reset();

	m_LifeStealAmount.Reset();
	m_ManaStealAmount.Reset();
	m_LifeBonusAmount.Reset();
	m_ManaBonusAmount.Reset();

	m_AmountDamageToType.Reset();
	m_DamageToUndead.Reset();

	m_DamageBonusMinMelee.Reset();
	m_DamageBonusMaxMelee.Reset();
	m_DamageBonusMinRanged.Reset();
	m_DamageBonusMaxRanged.Reset();
	m_DamageBonusMinCMagic.Reset();
	m_DamageBonusMaxCMagic.Reset();
	m_DamageBonusMinNMagic.Reset();
	m_DamageBonusMaxNMagic.Reset();

	CustomDamageMap::iterator i = m_CustomDamageMap.begin();
	while ( i != m_CustomDamageMap.end() )
	{
		CustomDamage * pDamage = i->second;

		// mark inactive now - if they are not refreshed by the end of the go's
		// modifier update, they will get nuked in the flush that happens last.
		pDamage->m_bActive      = false;
		pDamage->m_DamageMin    = 0.0f;
		pDamage->m_DamageMax    = 0.0f;
		pDamage->m_DamageChance = 0.0f;

		++i;
	}
}

void GoAttack :: RecalcModifiers( void )
{
	CustomDamageMap::iterator i = m_CustomDamageMap.begin();
	while ( i != m_CustomDamageMap.end() )
	{
		CustomDamage * pDamage = i->second;

		// didn't get refreshed? nuke it
		if ( !pDamage->m_bActive )
		{
			Delete( i->second );
			i = m_CustomDamageMap.erase( i );
		}
		else
		{
			++i;
		}
	}
}

void GoAttack :: UnlinkParent( Go* parent )
{
	if ( IsServerLocal() && GetGo()->IsAmmo() && parent->HasAspect() )
	{
		// Is the ammo attached?
		nema::HAspect parentHAspect = parent->GetAspect()->GetAspectHandle();
		nema::Aspect* localAspect = GetGo()->GetAspect()->GetAspectPtr();
		if ( localAspect->GetParent() == parentHAspect )
		{
			// Delete and fade any attached ammunition
			gGoDb.SMarkForDeletion( GetGoid(), true, true );
		}
	}
}

void GoAttack :: UnlinkChild( Go* child )
{
	// detect loss of child, nuke it
	if ( child->GetGoid() == m_AmmoReady )
	{
		m_AmmoReady = GOID_INVALID;
	}
	else if ( IsServerLocal() && child->IsAmmo() )
	{
		// Is the ammo attached?
		// Can only have attached ammo if we have an aspect.
		if( GetGo()->HasAspect() )
		{
			nema::Aspect* childAspect = child->GetAspect()->GetAspectPtr();
			nema::HAspect localHAspect = GetGo()->GetAspect()->GetAspectHandle();
			if ( childAspect->GetParent() == localHAspect )
			{
				// Delete and fade any attached ammunition
				gGoDb.SMarkForDeletion( child->GetGoid(), true, true );
			}
		}
	}
}

DWORD GoAttack :: GoAttackToNet( GoAttack* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoAttack* GoAttack :: NetToGoAttack( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetAttack() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
