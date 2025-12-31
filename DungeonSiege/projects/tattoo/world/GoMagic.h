//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoMagic.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the magic component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOMAGIC_H
#define __GOMAGIC_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoMagic declaration

class GoMagic : public GoComponent
{
public:
	SET_INHERITED( GoMagic, GoComponent );

// Types.

	enum eOptions
	{
		OPTION_NONE                   =      0,

		OPTION_REQUIRES_LINE_OF_SIGHT = 1 << 0,		// must have a line of sight on the target in order to cast spell on it
		OPTION_IS_ONE_SHOT            = 1 << 1,		// casts once and then is done - subsequent casts must be explicit
		OPTION_COMMAND_CAST			  = 1 << 2,		// can only be explicitly cast by user.  no auto-casting
	};

// Setup.

	// ctor/dtor
	GoMagic( Go* parent );
	GoMagic( const GoMagic& source, Go* parent );
	virtual ~GoMagic( void );

// Interface.

	// const query
FEX const gpstring&	 GetSkillClass				( void ) const;
FEX float			 GetRequiredCastLevel		( void ) const;
FEX float			 GetPContentLevel			( void ) const;
FEX float			 GetMaxCastLevel			( void ) const;
FEX eMagicClass		 GetMagicClass				( void ) const;
FEX float			 GetManaCost				( void ) const;
FEX const gpstring&  GetManaCostModifier		( void ) const;
FEX const gpstring&  GetManaCostUI				( void ) const;
FEX const gpstring&  GetManaCostUIModifier		( void ) const;
FEX float			 GetSpeedBias				( void ) const;
FEX float			 GetCastRange				( void ) const;
FEX float			 GetCastReloadDelay			( void ) const;
FEX float			 GetSkillCoefficientA		( void ) const;
FEX float			 GetSkillCoefficientB		( void ) const;
FEX	bool			 GetApplyEnchantmentsOnCast	( void ) const;
FEX const gpstring&  GetAttackDamageModifierMin ( void ) const;
FEX const gpstring&  GetAttackDamageModifierMax ( void ) const;
FEX UINT32			 GetCastSubAnimation		( void ) const;
FEX const gpstring&  GetEffectDuration			( void ) const;
FEX const gpstring&  GetCastExperience			( void ) const;
FEX const gpstring&  GetStateName				( void ) const;
FEX	const gpstring&	 GetCasterStateName			( void ) const;
FEX bool			 GetRequireStateCheck		( void ) const;
FEX bool			 GetIsOneUse				( void ) const;
FEX bool			 GetDoesDamagePerSecond		( void ) const;

	// formula evaluation
	void			 GetManaCostRangeUI				( float &min_cost, float &max_cost, Go * caster, const char *pOverrides = "" );

FEX	float			 GetMagicLevel					( Go const * caster );
FEX	bool			 CanReachNextLevel				( Go const * caster );

	float			 EvaluateManaCost				( Go const * caster, Go const * target, const char *pOverrides ) const;
FEX float			 EvaluateManaCost				( Go const * caster, Go const * target ) const	{ return (EvaluateManaCost( caster, target, NULL ) ); }
	
	float			 EvaluateEffectDuration			( Go const * caster, Go const * target, const char *pOverrides ) const;
FEX float			 EvaluateEffectDuration			( Go const * caster, Go const * target ) const	{ return (EvaluateEffectDuration( caster, target, NULL ) ); }

	float			 EvaluateCastExperience			( Go const * caster, Go const * target, const char *pOverrides ) const;
FEX float			 EvaluateCastExperience			( Go const * caster, Go const * target ) const	{ return (EvaluateCastExperience( caster, target, NULL ) ); }

	float			 EvaluateAttackDamageModifierMin( Go const * caster, Go const * target, const char *pOverrides ) const;
FEX float			 EvaluateAttackDamageModifierMin( Go const * caster, Go const * target ) const	{ return (EvaluateAttackDamageModifierMin( caster, target, NULL ) ); }

	float			 EvaluateAttackDamageModifierMax( Go const * caster, Go const * target, const char *pOverrides ) const;
FEX float			 EvaluateAttackDamageModifierMax( Go const * caster, Go const * target ) const	{ return (EvaluateAttackDamageModifierMax( caster, target, NULL ) ); }

	// flags
FEX	eUsageContextFlags GetUsageContextFlags( void ) const;

	// flag helpers 
FEX bool IsDefensive()													{ return ( GetUsageContextFlags() & UC_DEFENSIVE ) != 0; }
FEX bool IsOffensive()													{ return ( GetUsageContextFlags() & UC_OFFENSIVE ) != 0; }

FEX eTargetTypeFlags   GetTargetTypeFlags	( void ) const				{ return ( m_TargetTypeFlags ); }
FEX eTargetTypeFlags   GetTargetTypeFlagsNot( void ) const				{ return ( m_TargetTypeFlagsNot ); }
FEX void			   SetTargetTypeFlags	( eTargetTypeFlags flags )	{ m_TargetTypeFlags = flags; }

	// target type construction
FEX	bool			 IsCastableOn( Go * target, bool CheckMana );
FEX	bool			 IsCastableOn( Go * target )						{ return IsCastableOn( target, true ); };
FEX	eTargetTypeFlags BuildTargetTypeFlags( Go * caster, Go * target );
	Go*				 GetCorrectTarget( Go * target );

	// simple options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// query helpers
FEX bool  GetRequiresLineOfSight( void ) const					{  return ( TestOptions( OPTION_REQUIRES_LINE_OF_SIGHT ) ); }
FEX bool  GetIsOneShot          ( void ) const					{  return ( TestOptions( OPTION_IS_ONE_SHOT ) ); }
FEX bool  GetIsCommandCast		( void ) const;
FEX bool  IsSpell               ( void ) const					{  return ( ::IsSpell ( GetMagicClass() ) ); }
FEX bool  IsPotion              ( void ) const					{  return ( ::IsPotion( GetMagicClass() ) ); }
FEX bool  HasAlterationType		( eAlteration alteration ) const;
FEX bool  IsRejuvenationPotion	( void ) const;
FEX bool  IsHealthPotion		( void ) const;
FEX	bool  IsManaPotion			( void ) const;
FEX bool  HasNonInnateEnchantments( void );
FEX float GetPotionFullRatio    ( void ) const;
	float GetPotionAmount		( bool bMax = false ) const;	
FEX void  RCSetPotionAmount		( float value );
	void  SetPotionAmount		( float value );
	bool  CanOwnerCast			( void ) const;
	Go*   GetCaster				( void );

	// if this is a spell, cast it on the target
FEX bool SCast( Go* target );

	// notify the caster that a spell was cast so we can do things like update the gui.
FEX	void RCMemberCast( DWORD machineId );

	// if it is a one use spell, we have to notify the client to refresh the gui
FEX void RCOneShotUsed( Go * pSpellBook );

	// enchantments
	void					MakeEnchantments		( void );
	bool                    HasEnchantments			( void ) const		{  return ( m_EnchantmentStorage != NULL );  }
	EnchantmentStorage&		GetEnchantmentStorage	( void ) const		{  gpassert( HasEnchantments() );  return ( *m_EnchantmentStorage );  }
FEX void					SApplyEnchantmentsByName( Goid target, Goid source, const char *sName );
FEX void					SApplyEnchantments		( Goid target, Goid source )			{  SApplyEnchantmentsByName( target, source, NULL );  }
FEX void					RCApplyEnchantments     ( Goid target, Goid source, const char *sName );
	void					ApplyEnchantmentsByName	( Goid target, Goid source, const char *sName );
FEX	void					ApplyEnchantments		( Goid target, Goid source )			{  ApplyEnchantmentsByName( target, source, NULL );  }
	float					GetLongestAlteration	( Goid target, Goid source, bool bEvaluate );
FEX	float					GetLongestAlteration	( Goid target, Goid source )			{  return ( GetLongestAlteration( target, source, true ) );  }

	struct Modifier
	{
		DWORD m_ToolTipColor;		// color for modifier, or 0 to use default
		float m_ToolTipWeight;		// weight of this modifier, use to determine which color to choose if more than one modifier
	};

	// pcontent tracking
	void            SetPrefixModifierName( const GoString& name )		{  m_PrefixModifierName = name;  }
	void            SetSuffixModifierName( const GoString& name )		{  m_SuffixModifierName = name;  }
FEX	const gpstring& GetPrefixModifierName( void ) const					{  return ( m_PrefixModifierName );  }
FEX	const gpstring& GetSuffixModifierName( void ) const					{  return ( m_SuffixModifierName );  }
	bool            GetPrefixModifier    ( Modifier& modifier );
	bool            GetSuffixModifier    ( Modifier& modifier );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// events
	virtual void HandleMessage( const WorldMessage& msg );

private:

	// spec
	eOptions			m_Options;
	eTargetTypeFlags	m_TargetTypeFlags;
	eTargetTypeFlags	m_TargetTypeFlagsNot;
	int					m_Charges;
	GoString			m_PrefixModifierName;
	GoString			m_SuffixModifierName;

	// state
	my EnchantmentStorage* m_EnchantmentStorage;

	static DWORD    GoMagicToNet( GoMagic* x );
	static GoMagic* NetToGoMagic( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoMagic );
	FUBI_RPC_CLASS( GoMagic, GoMagicToNet, NetToGoMagic, "" );
};

MAKE_ENUM_BIT_OPERATORS( GoMagic::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOMAGIC_H

//////////////////////////////////////////////////////////////////////////////
