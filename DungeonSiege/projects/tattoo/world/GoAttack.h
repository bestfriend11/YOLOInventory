//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoAttack.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the attack component for Go's. Any Go that is capable
//             of doing damage will probably have an attack component.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOATTACK_H
#define __GOATTACK_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoAttack declaration

class GoAttack : public GoComponent
{
public:
	SET_INHERITED( GoAttack, GoComponent );

// Types.

	enum eOptions
	{
		OPTION_NONE                   =      0,

		OPTION_REQUIRES_LINE_OF_SIGHT = 1 << 0,		// must have a line of sight on the target in order to attack it
		OPTION_IS_MELEE               = 1 << 1,		// attack available in close combat
		OPTION_IS_PROJECTILE          = 1 << 2,		// attack available to ranged combat
		OPTION_IS_ONE_SHOT            = 1 << 3,		// attacks once and then is done - subsequent attacks must be explicit
		OPTION_IS_TWO_HANDED          = 1 << 4,		// requires two hands to operate
	};

// Setup.

	// ctor/dtor
	GoAttack( Go* parent );
	GoAttack( const GoAttack& source, Go* parent );
	virtual ~GoAttack( void );

// Interface.

	// query
FEX eAttackClass	GetAttackClass			( void ) const		{  return ( m_AttackClass );  }
FEX const gpstring&	GetSkillClass			( void ) const;
FEX float			GetAttackRange			( void ) const;	// could be modifier if actor is in a party
FEX float			GetReloadDelay			( void ) const;
FEX float			GetCriticalHitChance	( void ) const;

	// ranged
FEX float			GetAreaDamageRadius		( void ) const;

FEX void			SetProjectileLauncher	( Goid id )			{  m_ProjectileLauncher = id; }
FEX Goid			GetProjectileLauncher	( void ) const		{  return ( m_ProjectileLauncher );  }
FEX	bool			GetUseAimingError		( void ) const;
FEX	float			GetWeaponErrorRangeX	( void ) const;
FEX	float			GetWeaponErrorRangeY	( void ) const;

FEX Goid			GetAmmoCloneSource		( void ) const		{  return ( m_AmmoCloneSource    );  }
FEX Goid			GetAmmoReady			( void ) const		{  return ( m_AmmoReady			 );	 }
FEX const gpstring& GetAmmoAttachBone		( void ) const;
FEX bool			GetAmmoAttachesToWeapon	( void ) const;
FEX bool			GetAmmoAppearsJIT		( void ) const;

	// melee
FEX float			GetMeleeAimPiTime		( void ) const;
FEX float			GetMeleeSwingTime		( void ) const;
FEX float			GetMeleeRecoverTime		( void ) const;
FEX float			GetMeleeFidgetTime		( void ) const;

	// simple options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// damage modifiers
FEX float GetDamageMin( void ) const;
	float GetBaseDamageMin( void ) const						{ return m_DamageMin; }
	float GetDamageMinNatural( void ) const;
	float GetBaseDamageMinNatural( void ) const					{ return m_DamageMin.GetNatural(); }
	void  SetDamageMin( float damage )							{  m_DamageMin = damage;  }
FEX	void  SetDamageMinNatural( float damage )					{  m_DamageMin.SetNatural( damage );  GetGo()->SetModifiersDirty();  }

FEX float GetDamageMax( void ) const;
	float GetBaseDamageMax( void ) const						{ return m_DamageMax; }
	float GetDamageMaxNatural( void ) const;
	float GetBaseDamageMaxNatural( void ) const					{ return m_DamageMax.GetNatural(); }
	void  SetDamageMax( float damage )							{  m_DamageMax = damage;  }
FEX	void  SetDamageMaxNatural( float damage )					{  m_DamageMax.SetNatural( damage );  GetGo()->SetModifiersDirty();  }

	void  EvaluateTotalMinMaxDamage( float & minDamage, float & maxDamage, bool bUseDifficulty = true );
	void  EvaluateCustomMinMaxDamage( float & minDamage, float & maxDamage );

	// specific damage type bonuses
	float GetDamageBonusMinMelee		( void ) const			{ return m_DamageBonusMinMelee;  }
	void  SetDamageBonusMinMelee		( float value )			{ m_DamageBonusMinMelee = value; }
	void  SetDamageBonusMinMeleeNatural	( float value )			{ m_DamageBonusMinMelee.SetNatural( value ); }

	float GetDamageBonusMaxMelee		( void ) const			{ return m_DamageBonusMaxMelee;   }
	void  SetDamageBonusMaxMelee		( float value )			{ m_DamageBonusMaxMelee = value;  }
	void  SetDamageBonusMaxMeleeNatural	( float value )			{ m_DamageBonusMaxMelee.SetNatural( value );  }

	float GetDamageBonusMinRanged		( void ) const			{ return m_DamageBonusMinRanged;  }
	void  SetDamageBonusMinRanged		( float value )			{ m_DamageBonusMinRanged = value; }
	void  SetDamageBonusMinRangedNatural( float value )			{ m_DamageBonusMinRanged.SetNatural( value ); }

	float GetDamageBonusMaxRanged		( void ) const			{ return m_DamageBonusMaxRanged;  }
	void  SetDamageBonusMaxRanged		( float value )			{ m_DamageBonusMaxRanged = value; }
	void  SetDamageBonusMaxRangedNatural( float value )			{ m_DamageBonusMaxRanged.SetNatural( value ); }

	float GetDamageBonusMinCMagic		( void ) const			{ return m_DamageBonusMinCMagic;  }
	void  SetDamageBonusMinCMagic		( float value )			{ m_DamageBonusMinCMagic = value; }
	void  SetDamageBonusMinCMagicNatural( float value )			{ m_DamageBonusMinCMagic.SetNatural( value ); }

	float GetDamageBonusMaxCMagic		( void ) const			{ return m_DamageBonusMaxCMagic;  }
	void  SetDamageBonusMaxCMagic		( float value )			{ m_DamageBonusMaxCMagic = value; }
	void  SetDamageBonusMaxCMagicNatural( float value )			{ m_DamageBonusMaxCMagic.SetNatural( value ); }

	float GetDamageBonusMinNMagic		( void ) const			{ return m_DamageBonusMinNMagic;  }
	void  SetDamageBonusMinNMagic		( float value )			{ m_DamageBonusMinNMagic = value; }
	void  SetDamageBonusMinNMagicNatural( float value )			{ m_DamageBonusMinNMagic.SetNatural( value ); }

	float GetDamageBonusMaxNMagic		( void ) const			{ return m_DamageBonusMaxNMagic;  }
	void  SetDamageBonusMaxNMagic		( float value )			{ m_DamageBonusMaxNMagic = value; }
	void  SetDamageBonusMaxNMagicNatural( float value )			{ m_DamageBonusMaxNMagic.SetNatural( value ); }


FEX float GetPiercingDamageChance( void ) const					{  return ( m_PiercingDamageChance );  }
	void  SetPiercingDamageChance( float set )					{  m_PiercingDamageChance = set;  }

FEX float GetPiercingDamageChanceMelee( void ) const			{  return ( m_PiercingDamageChanceMelee );  }
	void  SetPiercingDamageChanceMelee( float set )				{  m_PiercingDamageChanceMelee = set;  }

FEX float GetPiercingDamageChanceRanged( void ) const			{  return ( m_PiercingDamageChanceRanged );  }
	void  SetPiercingDamageChanceRanged( float set )			{  m_PiercingDamageChanceRanged = set;  }

FEX float GetPiercingDamageChanceAmount( void ) const			{  return ( m_PiercingDamageChanceAmount );  }
	void  SetPiercingDamageChanceAmount( float set )			{  m_PiercingDamageChanceAmount = set;  }

FEX float GetPiercingDamageChanceAmountMelee( void ) const		{  return ( m_PiercingDamageChanceAmountMelee );  }
	void  SetPiercingDamageChanceAmountMelee( float set )		{  m_PiercingDamageChanceAmountMelee = set;  }

FEX float GetPiercingDamageChanceAmountRanged( void ) const		{  return ( m_PiercingDamageChanceAmountRanged );  }
	void  SetPiercingDamageChanceAmountRanged( float set )		{  m_PiercingDamageChanceAmountRanged = set;  }

FEX float GetPiercingDamageMin( void ) const					{  return ( m_PiercingDamageMin );  }
	void  SetPiercingDamageMin( float set )						{  m_PiercingDamageMin = set;  }

FEX float GetPiercingDamageMax( void ) const					{  return ( m_PiercingDamageMax );  }
	void  SetPiercingDamageMax( float set )						{  m_PiercingDamageMax = set;  }

FEX float GetPiercingDamageMeleeMin( void ) const				{  return ( m_PiercingDamageMeleeMin );  }
	void  SetPiercingDamageMeleeMin( float set )				{  m_PiercingDamageMeleeMin = set;  }

FEX float GetPiercingDamageMeleeMax( void ) const				{  return ( m_PiercingDamageMeleeMax );  }
	void  SetPiercingDamageMeleeMax( float set )				{  m_PiercingDamageMeleeMax = set;  }

FEX float GetPiercingDamageRangedMin( void ) const				{  return ( m_PiercingDamageRangedMin );  }
	void  SetPiercingDamageRangedMin( float set )				{  m_PiercingDamageRangedMin = set;  }

FEX float GetPiercingDamageRangedMax( void ) const				{  return ( m_PiercingDamageRangedMax );  }
	void  SetPiercingDamageRangedMax( float set )				{  m_PiercingDamageRangedMax = set;  }

FEX float GetChanceToHitBonus( void ) const						{  return ( m_ChanceToHitBonus );  }
	void  SetChanceToHitBonus( float set )						{  m_ChanceToHitBonus = set;  }

FEX float GetChanceToHitBonusMelee( void ) const				{  return ( m_ChanceToHitBonusMelee );  }
	void  SetChanceToHitBonusMelee( float set )					{  m_ChanceToHitBonusMelee = set;  }

FEX float GetChanceToHitBonusRanged( void ) const				{  return ( m_ChanceToHitBonusRanged );  }
	void  SetChanceToHitBonusRanged( float set )				{  m_ChanceToHitBonusRanged = set;  }

FEX float GetExperienceBonus( void ) const						{  return ( m_ExperienceBonus );  }
	void  SetExperienceBonus( float set )						{  m_ExperienceBonus = set;  }

FEX	float GetLifeStealAmount( void ) const						{  return ( m_LifeStealAmount );  }
	void  SetLifeStealAmount( float set )						{  m_LifeStealAmount = set;  }

FEX float GetManaStealAmount( void ) const						{  return ( m_ManaStealAmount );  }
	void  SetManaStealAmount( float set )						{  m_ManaStealAmount = set;  }

FEX	float GetLifeBonusAmount( void ) const						{  return ( m_LifeBonusAmount );  }
	void  SetLifeBonusAmount( float set )						{  m_LifeBonusAmount = set;  }

FEX float GetManaBonusAmount( void ) const						{  return ( m_ManaBonusAmount );  }
	void  SetManaBonusAmount( float set )						{  m_ManaBonusAmount = set;  }

FEX const gpstring& GetDamageToType( void ) const				{  return ( m_DamageToType );  }
	void  SetDamageToType( const gpstring & set )				{  m_DamageToType = set;  }

FEX float GetAmountDamageToType( void ) const					{  return ( m_AmountDamageToType );  }
	void  SetAmountDamageToType( float set )					{  m_AmountDamageToType = set;  }

FEX float GetDamageToUndead( void ) const						{  return ( m_DamageToUndead );  }
	void  SetDamageToUndead( float set )						{  m_DamageToUndead = set;  }

	// Custom Damage
	struct CustomDamage
	{
		CustomDamage()
		{
			m_bActive       = false;
			m_bMeleeDamage  = false;
			m_bRangedDamage = false;
			m_DamageChance  = 0.0f;
			m_DamageMin     = 0.0f;
			m_DamageMax     = 0.0f;
		}

		~CustomDamage();

		bool Xfer( FuBi::PersistContext& persist );

		gpstring	   m_DamageType;

		bool		   m_bActive;

		bool		   m_bMeleeDamage;
		bool		   m_bRangedDamage;
		float		   m_DamageChance;		// chance to apply damage
		float		   m_DamageMin;			// damage level range
		float		   m_DamageMax;
		Skrit::HObject m_Skrit;				// special skrit called when damage is applied
	};

	typedef std::map< gpstring, my CustomDamage*, istring_less > CustomDamageMap;

	const CustomDamageMap & GetCustomDamageMap() const			{  return ( m_CustomDamageMap );  }

	CustomDamage*       GetCustomDamage( const char* damageType, bool createIfNeeded );
	const CustomDamage* GetCustomDamage( const char* damageType ) const;
	void                SetCustomDamage( const char* damageType, const char* attackType, float damageMin, float damageMax, const char* skritName, bool cumulative = true );

FEX	float GetCustomDamageChance( const char * damageType ) const;
	void  SetCustomDamageChance( const char * damageType, float damageChance, bool cumulative = true );

FEX	float GetCustomDamageMin( const char * damageType ) const;
FEX	float GetCustomDamageMax( const char * damageType ) const;

	// query helpers
FEX bool GetRequiresLineOfSight( void ) const  					{  return ( TestOptions( OPTION_REQUIRES_LINE_OF_SIGHT ) );  }
FEX bool GetIsMelee            ( void ) const  					{  return ( TestOptions( OPTION_IS_MELEE ) );  }
FEX bool GetIsProjectile       ( void ) const  					{  return ( TestOptions( OPTION_IS_PROJECTILE ) );  }
FEX bool GetIsWeapon           ( void ) const  					{  return ( TestOptions( (eOptions)(OPTION_IS_MELEE | OPTION_IS_PROJECTILE) ) );  }
FEX bool GetIsOneShot          ( void ) const  					{  return ( TestOptions( OPTION_IS_ONE_SHOT ) );  }
FEX bool GetIsTwoHanded        ( void ) const  					{  return ( TestOptions( OPTION_IS_TWO_HANDED ) );  }
FEX Go*  GetShooter            ( void );

// Ranged projectile attack

	// Determine what the firing position is based on the ammo that is being shot
FEX SiegePos & ComputeFiringPos( void );

	// Determine what the target position is derived from a game object
FEX SiegePos & ComputeTargetPos( Goid target );

	// Determine if the projectile has enough velocity to hit the target
FEX bool  IsInProjectileRange( float firing_velocity, const SiegePos &target_position );

	// Determine what the aiming error is for the shooter
FEX void  ComputeAimingError( void );
FEX float GetAimingErrorX   ( void ) const						{  return ( m_AimingErrorX );  }
FEX float GetAimingErrorY   ( void ) const						{  return ( m_AimingErrorY );  }

	// Determine what the actual firing angle will be based on specified params
FEX float ComputeAimingAngle( const SiegePos &firingPos, const SiegePos &targetPos, float velocity );
FEX float ComputeAimingAngle( Goid ammo, const SiegePos &firingPos, const SiegePos &targetPos, float velocity );

	// Create ammo and attach it
FEX Goid SPrepareAmmo ( void );
	void PrepareAmmo( Go* ammo );

	// Launch the ammo
FEX void SLaunchAmmo( float velocity, const SiegePos &firingPos, const SiegePos &targetPos, float x_error, float y_error, Goid intended_target = GOID_INVALID );
FEX void RCLaunchAmmo( Go* ammo, Goid intended_target, SiegePos firing_position, vector_3 velocity_vector, DWORD seed );

	// Detach and destroy ammo
FEX	void SUnprepareAmmo ( void );

	// Tell the target that he was attacked by a ranged attack
FEX void AlertRangedAttack( Goid target );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual void Shutdown       ( void );
	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void ResetModifiers ( void );
	virtual void RecalcModifiers( void );
	virtual void UnlinkParent   ( Go* parent );
	virtual void UnlinkChild    ( Go* child );

private:

	// spec
	eOptions     m_Options;				// attack options
	eAttackClass m_AttackClass;			// attack class for this go
	Goid         m_AmmoCloneSource;		// go to clone from for ammo
	Goid         m_ProjectileLauncher;	// if this weapon is a projectile then the launcher would be the bow NOT the actor

// Combat modifiers

	GobFloat m_DamageMin;			// minimum damage amount this object can do
	GobFloat m_DamageMax;			// maximum damage amount this object can do

	// specific damage type bonuses
	GobFloat m_DamageBonusMinMelee;
	GobFloat m_DamageBonusMaxMelee;
	GobFloat m_DamageBonusMinRanged;
	GobFloat m_DamageBonusMaxRanged;
	GobFloat m_DamageBonusMinCMagic;
	GobFloat m_DamageBonusMaxCMagic;
	GobFloat m_DamageBonusMinNMagic;
	GobFloat m_DamageBonusMaxNMagic;

	GobFloat m_PiercingDamageChance;		// percent chance to do piercing damage
	GobFloat m_PiercingDamageChanceMelee;
	GobFloat m_PiercingDamageChanceRanged;
	GobFloat m_PiercingDamageChanceAmount;	// additional amount of piercing damage done if a piercing hit is made
	GobFloat m_PiercingDamageChanceAmountMelee;
	GobFloat m_PiercingDamageChanceAmountRanged;
	GobFloat m_PiercingDamageMin;			// min amount of additional piercing damage
	GobFloat m_PiercingDamageMax;			// max amount of additional piercing damage
	GobFloat m_PiercingDamageMeleeMin;
	GobFloat m_PiercingDamageMeleeMax;
	GobFloat m_PiercingDamageRangedMin;
	GobFloat m_PiercingDamageRangedMax;

	GobFloat m_ChanceToHitBonus;	// percent to increase/decrease the chance to hit value
	GobFloat m_ChanceToHitBonusMelee;
	GobFloat m_ChanceToHitBonusRanged;
	GobFloat m_ExperienceBonus;		// percent to increase/decrease experience gained per kill

	GobFloat m_LifeStealAmount;		// amount of life to steal per hit
	GobFloat m_ManaStealAmount;		// amount of mana to steal per hit
	GobFloat m_LifeBonusAmount;		// max life given per hit
	GobFloat m_ManaBonusAmount;		// max mana given per hit

	GobFloat m_DamageToUndead;		// damage bonus versus undead monsters

	GoString m_DamageToType;		// this go will do modified damage against this monster type
	GobFloat m_AmountDamageToType;

	// Custom damage types, created by modifiers.
	CustomDamageMap	m_CustomDamageMap;

	// state
	Goid	 m_AmmoReady;			// If GOID_INVALID then ammo needs to be created
	float    m_AimingErrorX;		// ComputeAimingError fills this in
	float    m_AimingErrorY;		// (ditto)
	SiegePos m_FiringPos;			// storage for the firing position
	SiegePos m_TargetPos;			// storage for the target position

	static DWORD     GoAttackToNet( GoAttack* x );
	static GoAttack* NetToGoAttack( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoAttack );
	FUBI_RPC_CLASS( GoAttack, GoAttackToNet, NetToGoAttack, "" );
};

MAKE_ENUM_BIT_OPERATORS( GoAttack::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOATTACK_H

//////////////////////////////////////////////////////////////////////////////
