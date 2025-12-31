//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoActor.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the GoActor component.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOACTOR_H
#define __GOACTOR_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "Rules.h"

//////////////////////////////////////////////////////////////////////////////
// class GoActor declaration

class GoActor : public GoComponent
{
public:
	SET_INHERITED( GoActor, GoComponent );

// Types.

	enum eOptions
	{
		OPTION_NONE				=      0,

		OPTION_CAN_LEVEL_UP		= 1 << 0,		// true if skills and base attributes can progress
		OPTION_IS_HERO			= 1 << 1,		// is this a character that you can play as?
		OPTION_IS_CHARMABLE		= 1 << 2,		// is this character capable of being charmed?
		OPTION_IS_POSSESSABLE	= 1 << 3,		// is this character capable of being possessed?
		OPTION_DROPS_SPELLBOOK	= 1 << 4,		// does this character drop their spell book when killed?
	};

// Setup.

	// ctor/dtor
	GoActor( Go* go );
	GoActor( const GoActor& source, Go* go );
	virtual ~GoActor( void );

// Options.

	// simple options
	void SetOptions   ( eOptions options, bool set = true )				{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )								{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )								{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const						{  return ( (m_Options & options) != 0 );  }

	// query helpers
FEX	bool GetCanLevelUp    ( void ) const								{  return ( TestOptions( OPTION_CAN_LEVEL_UP    ) );  }
FEX	bool GetIsHero        ( void ) const								{  return ( TestOptions( OPTION_IS_HERO         ) );  }
FEX	bool GetIsCharmable   ( void ) const								{  return ( TestOptions( OPTION_IS_CHARMABLE    ) );  }
FEX	bool GetIsPossessable ( void ) const								{  return ( TestOptions( OPTION_IS_POSSESSABLE  ) );  }
FEX	bool GetDropsSpellbook( void ) const								{  return ( TestOptions( OPTION_DROPS_SPELLBOOK ) );  }

	// Simple generic state storage for magic spells controlled from skrit
FEX	void	SAddGenericState		 ( const char* sName, const char* sDescription, float timeout, Goid caster, Goid spell, float spell_level );
FEX	void	RCAddGenericState		 ( const char* sName, const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level );
	void	AddGenericState		 	 ( const char* sName, const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level );
FEX	void	SRemoveGenericState		 ( const char* sName );
FEX	void	RCRemoveGenericState	 ( const char* sName );
	void	RemoveGenericState		 ( const char* sName );
FEX	bool	HasGenericState			 ( const char* sName ) const;
FEX	float	GetGenericStateSpellLevel( const char* sName ) const;
FEX	Goid	GetGenericStateSpellGoid ( const char* sName ) const;

	// Generic state accessors for the UI - format is "description - seconds remaining"
	int		GetGenericStateInfo( gpwstring &sUIDescriptions, int numStates = -1 ) const;

// Interface.

	// portrait override
	void  SetPortraitTexture( DWORD portraitTex, bool addRef );
	DWORD GetPortraitTexture( void ) const								{  return ( m_PortraitTex );  }

	// server calls this
FEX	FuBiCookie RCUpdatePortraitTexture( void );

	// const query
	const gpstring& GetPortraitIcon( void ) const;
	const gpstring& GetRace        ( void ) const;	
FEX	bool			GetIsMale      ( void ) const;	

	// trait query
	const SkillColl& GetSkills				( void ) const								{  return ( m_Skills ); }
	void			 ResetSkillModifiers	( void )									{  m_Skills.ResetModifiers(); }
FEX	float            GetSkillLevel			( const char* skill ) const					{  return ( m_Skills.GetSkillLevel( skill ) );  }
	bool             GetSkill				( const char* skill, Skill** pSkill ) const	{  return ( m_Skills.GetSkill( skill, pSkill ) );  }
FEX	float            GetHighestSkillLevel	( void )  const;

	// skill modifications
	void	CreateSkill( const char* skill );
FEX	void	RCSetSkillLevels( float strength, float intelligence, float dexterity );

	// alignment		$$$ alignment seems to better belong in the GoMind
FEX	eActorAlignment GetAlignment  ( void ) const						{  return ( m_Alignment ); }
FEX	void            SSetAlignment ( eActorAlignment alignment );
FEX	void            RCSetAlignment( eActorAlignment alignment );
	void			SetAlignment  ( eActorAlignment alignment );

	// attributes for chance to hit bonus
	int   GetChanceToHitBonusStart( void ) const						{  return ( m_ChanceToHitBonusStart );  }
	int   GetChanceToHitBonusKillStep( void ) const						{  return ( m_ChanceToHitBonusKillStep );  }
	float GetChanceToHitBonusIncrement( void ) const					{  return ( m_ChanceToHitBonusIncrement );  }
	float GetChanceToHitBonusMax( void ) const							{  return ( m_ChanceToHitBonusMax );  }

	// class for UI (generated)
	const gpwstring& GetClass( void ) const								{  return ( m_Class ); }
	void             SetClass( const gpwstring& cls )					{  m_Class = cls;  }
FEX	void             GetClass( gpstring& cls )							{  cls = ::ToAnsi( m_Class );  }
FEX	void             SetClass( const gpstring& cls )					{  m_Class = ::ToUnicode( cls );  }

	// actor pickup
FEX	void			SetCanBePickedUp( bool bSet )						{  m_bCanBePickedUp = bSet; }
FEX	bool			GetCanBePickedUp()									{  return ( m_bCanBePickedUp ); }

	// show status
FEX void			SSetCanShowHealth( bool bSet );						
FEX void			RCSetCanShowHealth( bool bSet );
	void			SetCanShowHealth( bool bSet );
FEX	bool			GetCanShowHealth( void ) const						{  return ( m_bCanShowHealth ); }		

	// enchantment storage sets this value; allows use to know when to call the gui back	
	// to make sure everything is still equippable
	void SetEnchantmentRemoved( bool bSet )								{ m_bEnchantmentRemoved = bSet; }
	bool GetEnchantmentRemoved()										{ return m_bEnchantmentRemoved; }

	float GetUnconsciousDuration( void ) const;
FEX	void  RSResetUnconsciousDuration( float duration );
FEX	void  RCSetUnconsiousEndTime( double endTime );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );
	        void         XferForSync  ( FuBi::BitPacker& packer );

	// event handlers
	virtual bool CommitCreation ( void )								{  return ( CommitCreateOrImport( false ) );  }
	virtual bool CommitImport   ( bool fullXfer )						{  return ( CommitCreateOrImport( fullXfer ) );  }
	virtual void Update         ( float deltaTime );
	virtual void UpdateSpecial  ( float deltaTime );
	virtual void ResetModifiers ( void );
	virtual void RecalcModifiers( void );

	// initalization of mana and life,  this was in CommitCreateOrImport
	// but was expanded.
	void ScaleForMultiplayer( bool useAllPlayers = false );

private:
	bool CommitCreateOrImport( bool importFullXfer );	
	void UpdateGenericStates( void );

	// traits
	eActorAlignment m_Alignment;
	SkillColl		m_Skills;
	DWORD			m_PortraitTex;
	int				m_ChanceToHitBonusStart;
	int				m_ChanceToHitBonusKillStep;
	float			m_ChanceToHitBonusIncrement;
	float			m_ChanceToHitBonusMax;
	bool			m_bCanBePickedUp;
	bool			m_bCanShowHealth;
	bool			m_bEnchantmentRemoved;
	double			m_UnconsciousEndTime;			// track what time to wake up from unconsciousness
	bool			m_bMPScaled;

	// state
	eOptions		m_Options;
	GowString		m_Class;

	// Generic state storage
	struct	GenericState
	{
		GenericState( void ){}
		GenericState( const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level );

		bool		Xfer( FuBi::PersistContext &persist );
		void		Xfer( FuBi::BitPacker &packer );

		gpwstring	m_sDescription;
		Goid		m_caster;
		Goid		m_spell;
		float		m_spellLevel;
		double		m_absTimeout;
	};

	typedef std::map< gpstring, GenericState, istring_less > GSColl;

	GSColl			m_GenericStateStorage;

	static DWORD    GoActorToNet( GoActor* x );
	static GoActor* NetToGoActor( DWORD d, FuBiCookie* cookie );

	FUBI_RPC_CLASS( GoActor, GoActorToNet, NetToGoActor, "" );
	SET_NO_COPYING( GoActor );
};

MAKE_ENUM_BIT_OPERATORS( GoActor::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOACTOR_H

//////////////////////////////////////////////////////////////////////////////
