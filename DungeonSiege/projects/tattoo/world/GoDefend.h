//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDefend.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the defend component for Go's. Any Go that is capable
//             of modifying damage received will probably have a defend
//             component.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GODEFEND_H
#define __GODEFEND_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoDefend declaration

class GoDefend : public GoComponent
{
public:
	SET_INHERITED( GoDefend, GoComponent );

// Types.

	enum eOptions
	{
		OPTION_NONE						=      0,

		OPTION_IS_CRITICAL_HIT_IMMUNE	= 1 << 0,		// whether or not this object is immune to critical hits
	};

// Setup.

	// ctor/dtor
	GoDefend( Go* parent );
	GoDefend( const GoDefend& source, Go* parent );
	virtual ~GoDefend( void );

// Options.

	// simple options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// query helpers
	bool GetIsCriticalHitImmune( void ) const					{  return ( TestOptions( OPTION_IS_CRITICAL_HIT_IMMUNE ) );  }

// Interface.

	// const query
	float  GetDamageThreshold( void ) const;

	// armor type and style
	const gpstring& GetArmorType      ( void ) const			{  return ( m_ArmorType );  }
	void            SetArmorType      ( const gpstring& at )	{  m_ArmorType = at;  }
	const gpstring& GetArmorStyle     ( void ) const			{  return ( m_ArmorStyle );  }
	void            SetArmorStyle     ( const gpstring& as )	{  m_ArmorStyle = as;  }

	// query
	eDefendClass GetDefendClass		   ( void ) const	{  return ( m_DefendClass );  }
	bool         GetIsShield		   ( void ) const	{  return ( m_DefendClass == DC_SHIELD );  }
	float		 GetNaturalDefense	   ( void ) const   {  return ( m_Defense.GetNatural() ); }
FEX	float        GetDefense			   ( void ) const	{  return ( m_Defense );  }

	// setting
	void SetDefense       ( float val )					{  m_Defense = val;  }
	void SetDefenseNatural( float val )					{  m_Defense.SetNatural( val );  GetGo()->SetModifiersDirty();  }

FEX float GetBlockDamage( void ) const					{  return ( m_BlockDamage );  }
	void  SetBlockDamage( float set )					{  m_BlockDamage = set;  }

FEX float GetBlockMeleeDamage( void ) const				{  return ( m_BlockMeleeDamage );  }
	void  SetBlockMeleeDamage( float set )				{  m_BlockMeleeDamage = set;  }

FEX float GetBlockRangedDamage( void ) const			{  return ( m_BlockRangedDamage );  }
	void  SetBlockRangedDamage( float set )				{  m_BlockRangedDamage = set;  }

FEX float GetBlockNatureMagicDamage( void ) const		{  return ( m_BlockNatureMagicDamage );  }
	void  SetBlockNatureMagicDamage( float set )		{  m_BlockNatureMagicDamage = set;  }

FEX float GetBlockCombatMagicDamage( void ) const		{  return ( m_BlockCombatMagicDamage );  }
	void  SetBlockCombatMagicDamage( float set )		{  m_BlockCombatMagicDamage = set;  }

FEX float GetBlockPiercingDamage( void ) const			{  return ( m_BlockPiercingDamage );  }
	void  SetBlockPiercingDamage( float set )			{  m_BlockPiercingDamage = set;  }

FEX float GetBlockCustomDamage( const char * damageType ) const;
	void  SetBlockCustomDamage( const char * damageType, float value, bool cumulative = true );

FEX float GetBlockPartDamage( void ) const				{  return ( m_BlockPartDamage );  }
	void  SetBlockPartDamage( float set )				{  m_BlockPartDamage = set;  }

FEX float GetBlockPartMeleeDamage( void ) const			{  return ( m_BlockPartMeleeDamage );  }
	void  SetBlockPartMeleeDamage( float set )			{  m_BlockPartMeleeDamage = set;  }

FEX float GetBlockPartRangedDamage( void ) const		{  return ( m_BlockPartRangedDamage );  }
	void  SetBlockPartRangedDamage( float set )			{  m_BlockPartRangedDamage = set;  }

FEX float GetBlockPartNatureMagicDamage( void ) const	{  return ( m_BlockPartNatureMagicDamage );  }
	void  SetBlockPartNatureMagicDamage( float set )	{  m_BlockPartNatureMagicDamage = set;  }

FEX float GetBlockPartCombatMagicDamage( void ) const	{  return ( m_BlockPartCombatMagicDamage );  }
	void  SetBlockPartCombatMagicDamage( float set )	{  m_BlockPartCombatMagicDamage = set;  }

FEX float GetBlockPartPiercingDamage( void ) const		{  return ( m_BlockPartPiercingDamage );  }
	void  SetBlockPartPiercingDamage( float set )		{  m_BlockPartPiercingDamage = set;  }

FEX float GetBlockPartCustomDamage( const char * damageType ) const;
	void  SetBlockPartCustomDamage( const char * damageType, float value, bool cumulative = true );

FEX float GetBlockMeleeDamageChance( void ) const		{  return ( m_BlockMeleeDamageChance );  }
	void  SetBlockMeleeDamageChance( float set )		{  m_BlockMeleeDamageChance = set;  }

FEX float GetBlockRangedDamageChance( void ) const		{  return ( m_BlockRangedDamageChance );  }
	void  SetBlockRangedDamageChance( float set )		{  m_BlockRangedDamageChance = set;  }

FEX float GetBlockCombatMagicChance( void ) const		{  return ( m_BlockCombatMagicChance );  }
	void  SetBlockCombatMagicChance( float set )		{  m_BlockCombatMagicChance = set;  }

FEX float GetBlockNatureMagicChance( void ) const		{  return ( m_BlockNatureMagicChance );  }
	void  SetBlockNatureMagicChance( float set )		{  m_BlockMeleeDamageChance = set;  }

FEX float GetBlockMeleeDamageChanceAmount( void ) const		{  return ( m_BlockMeleeDamageChanceAmount );  }
	void  SetBlockMeleeDamageChanceAmount( float set )		{  m_BlockMeleeDamageChanceAmount = set;  }

FEX float GetBlockRangedDamageChanceAmount( void ) const		{  return ( m_BlockRangedDamageChanceAmount );  }
	void  SetBlockRangedDamageChanceAmount( float set )		{  m_BlockRangedDamageChanceAmount = set;  }

FEX float GetBlockCombatMagicChanceAmount( void ) const		{  return ( m_BlockCombatMagicChanceAmount );  }
	void  SetBlockCombatMagicChanceAmount( float set )		{  m_BlockCombatMagicChanceAmount = set;  }

FEX float GetBlockNatureMagicChanceAmount( void ) const		{  return ( m_BlockNatureMagicChanceAmount );  }
	void  SetBlockNatureMagicChanceAmount( float set )		{  m_BlockMeleeDamageChanceAmount = set;  }

FEX float GetChanceToDodgeHit( void ) const				{  return ( m_ChanceToDodgeHit );  }
	void  SetChanceToDodgeHit( float set )				{  m_ChanceToDodgeHit = set;  }

FEX float GetChanceToDodgeHitMelee( void ) const		{  return ( m_ChanceToDodgeHitMelee );  }
	void  SetChanceToDodgeHitMelee( float set )			{  m_ChanceToDodgeHitMelee = set;  }

FEX float GetChanceToDodgeHitRanged( void ) const		{  return ( m_ChanceToDodgeHitRanged );  }
	void  SetChanceToDodgeHitRanged( float set )		{  m_ChanceToDodgeHitRanged = set;  }

FEX float GetReflectDamageAmount( void ) const			{  return ( m_ReflectDamageAmount );  }
	void  SetReflectDamageAmount( float set )			{  m_ReflectDamageAmount = set;  }

FEX float GetReflectDamageChance( void ) const			{  return ( m_ReflectDamageChance );  }
	void  SetReflectDamageChance( float set )			{  m_ReflectDamageChance = set;  }

FEX float GetReflectPiercingDamageAmount( void ) const	{  return ( m_ReflectPiercingDamageAmount );  }
	void  SetReflectPiercingDamageAmount( float set )	{  m_ReflectPiercingDamageAmount = set;  }

FEX float GetReflectPiercingDamageChance( void ) const	{  return ( m_ReflectDamageChance );  }
	void  SetReflectPiercingDamageChance( float set )	{  m_ReflectPiercingDamageChance = set;  }

FEX float GetManaShield( void ) const					{  return ( m_ManaShield );  }
	void  SetManaShield( float set )					{  m_ManaShield = set;  }

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation( void );
	virtual void ResetModifiers( void );

private:
	eOptions     m_Options;				// options configuring this component
	GoString     m_ArmorType;			// type of this armor
	GoString     m_ArmorStyle;			// style of this armor
	eDefendClass m_DefendClass;			// classification of defense
	GobFloat     m_Defense;				// general toughness of this object to remove damage

	GobFloat	 m_BlockDamage;
	GobFloat	 m_BlockMeleeDamage;
	GobFloat	 m_BlockRangedDamage;
	GobFloat	 m_BlockNatureMagicDamage;
	GobFloat	 m_BlockCombatMagicDamage;
	GobFloat	 m_BlockPiercingDamage;

	GobFloat	 m_BlockPartDamage;
	GobFloat	 m_BlockPartMeleeDamage;
	GobFloat	 m_BlockPartRangedDamage;
	GobFloat	 m_BlockPartNatureMagicDamage;
	GobFloat	 m_BlockPartCombatMagicDamage;
	GobFloat	 m_BlockPartPiercingDamage;

	GobFloat	 m_BlockMeleeDamageChance;
	GobFloat	 m_BlockRangedDamageChance;
	GobFloat	 m_BlockCombatMagicChance;
	GobFloat	 m_BlockNatureMagicChance;
	GobFloat	 m_BlockMeleeDamageChanceAmount;
	GobFloat	 m_BlockRangedDamageChanceAmount;
	GobFloat	 m_BlockCombatMagicChanceAmount;
	GobFloat	 m_BlockNatureMagicChanceAmount;
	GobFloat	 m_ChanceToDodgeHit;
	GobFloat	 m_ChanceToDodgeHitMelee;
	GobFloat	 m_ChanceToDodgeHitRanged;

	typedef std::map< gpstring, float, istring_less > BlockCustomDamageMap;
	BlockCustomDamageMap	m_BlockCustomDamage;
	BlockCustomDamageMap	m_BlockPartCustomDamage;

	GobFloat	 m_ReflectDamageAmount;
	GobFloat	 m_ReflectDamageChance;
	GobFloat	 m_ReflectPiercingDamageAmount;
	GobFloat	 m_ReflectPiercingDamageChance;

	GobFloat	 m_ManaShield;

	static DWORD     GoDefendToNet( GoDefend* x );
	static GoDefend* NetToGoDefend( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoDefend );
	FUBI_RPC_CLASS( GoDefend, GoDefendToNet, NetToGoDefend, "" );
};

MAKE_ENUM_BIT_OPERATORS( GoDefend::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GODEFEND_H

//////////////////////////////////////////////////////////////////////////////
