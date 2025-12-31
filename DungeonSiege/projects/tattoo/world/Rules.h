#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Rules.h

	Author(s)	: Bartosz Kijanka, Rick Saenz

	Purpose		: A collection of methods used throughout gameplay in relation
				  to changing stats.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __RULES_H
#define __RULES_H


#include "GpColl.h"
#include "WorldOptions.h"
#include "SkritDefs.h"

class Skill;


struct SkillColl : stdx::fast_vector< Skill >
{
	typedef const Skill* CPSkill;

	// Initialization
	void	LoadSkills( FastFuelHandle hSkills );

	// Skill query
	float	GetSkillLevel( const char *szSkill ) const;
	bool	GetSkill     ( const char* szSkill, Skill **pSkill ) const;
	bool	GetSkill     ( const char* szSkill, CPSkill *pSkill ) const;
	bool	HasSkill     ( const char* szSkill ) const;

	// Modifier setup
	void	ResetModifiers( void );

	// Persistence
	bool	Xfer         ( FuBi::PersistContext& persist, const char* key );
	bool	XferCharacter( FuBi::PersistContext& persist, const char* key );
};



struct CombatSoundCallback;


class Rules : public Singleton< Rules >
{
	struct	LevelUpIndicator;
public:

	typedef stdx::fast_vector< LevelUpIndicator >					LevelUpColl;
	typedef stdx::fast_vector< double >							XPColl;
	typedef stdx::fast_vector< float >							FloatColl;


	enum eHitType
	{
		SET_BEGIN_ENUM( HIT_, 0 ),

		HIT_NONE,
		HIT_GLANCING,
		HIT_SOLID,
		HIT_CRITICAL,
		HIT_EXPLODED,

		SET_END_ENUM( HIT_ ),
	};

	Rules();

	~Rules();

	// --- Defenses

	// Determines whether or not the attacker can actually hit the victim
FEX bool	CanHit					( Goid Attacker, Goid Victim, Goid AttackerWeapon, Goid VictimWeapon );

	// Checks to see if the attacker was able to critically hit the victim
FEX bool	CheckForCriticalHit		( Goid Attacker, Goid Victim );

	// Sums all defenses for the specified go
FEX float	GetTotalDefense			( Goid Client );

	// Sums the fire resistance for the speicifed go
FEX	float	GetTotalFireResistance	( Goid Client );



	// --- Damage Calculation

	// Calculates the appropriate damage range based on bonuses from attacker and weapon
FEX bool	GetDamageRange		( Goid Client, Goid Weapon, float &min_damage, float &max_damage, bool bUseDifficulty );
FEX bool	GetDamageRange		( Goid Client, Goid Weapon, float &min_damage, float &max_damage );

	// Returns the amount of damage done
FEX float	CalculateDamage		( Goid Attacker, Goid Weapon, Goid Victim, float min_damage, float max_damage, float total_defense, bool bPiercing, bool duration_damage );

	// Gets the multiplier for changing the damage by according to the difficulty level
	float	GetDifficultyFactor	( Go* pClient );



	// --- Damage Application

	// Does special preparations for melee based damage and then calls DamageGo()
FEX void	DamageGoMelee	( Goid Attacker, Goid Weapon, Goid Victim );

	// Does special preparations for ranged based damage and then calls DamageGo()
FEX void	DamageGoRanged	( Goid Attacker, Goid Weapon, Goid Victim );

	// Does special preparations for magic based damage and then calls DamageGo()
FEX void	DamageGoMagic	( Goid Attacker, Goid Spell, Goid Victim, SiegePos &damage_origin );

	// Does simple damage using victim defenses - Returns true if Go was killed
FEX bool	DamageGoTrap	( Goid Victim, Goid Attacker, Goid AttackerWeapon, float min_damage, float max_damage );

	// Handles doing damage from fire - Returns true if Go was killed
FEX bool	DamageGoParticle( Goid Victim, Goid Attacker, Goid AttackerWeapon, float min_damage, float max_damage, float exposure_time, bool award_xp, bool bIgnite, bool bCalcFireResist );

	// Handles doing damage from a damage volume or explosion
FEX bool	DamageGoVolume( Goid Victim, Goid Attacker, Goid AttackerWeapon, float damage, float delta_t, bool explosive_damage = false );

	// Returns true if killed
	bool	DamageGo		( Goid Victim, Goid Attacker, Goid AttackerWeapon, float amount, bool piercing_damage, bool explosive_damage, CombatSoundCallback* cb );
FEX bool	DamageGo		( Goid Victim, Goid Attacker, Goid AttackerWeapon, float amount, bool piercing_damage, bool explosive_damage = false )
							{  return ( DamageGo( Victim, Attacker, AttackerWeapon, amount, piercing_damage, explosive_damage, NULL ) );  }



	// --- Experience

	// Returns the highest level an actor can be
FEX	float	GetMaxLevel					( void ) const			{ return float(m_ExperienceTable.size()-1); }

FEX float	GetMaxLevelUber				( void ) const			{ return m_MaxLevelUber; }

	// Returns the XP total for current_level + 1
FEX	double	GetNextLevelXP				( float current_level ) const;
	// $$$ just a work around until skrit supports doubles.
FEX	float	GetNextLevelXPTEMP			( float current_level ) const { return float( GetNextLevelXP( current_level ) ); };

	// Conversion
FEX	double	LevelToXP					( float level ) const;
	float	XPToLevel					( double xp ) const;
FEX	float	FUBI_RENAME( XPToLevel )	( float xp ) const		{ return ( XPToLevel( xp ) ); }

	// Returns the amount of expereience earned based on how much damage was done to the victim
FEX float	CalculateExperience			( Goid Attacker, Goid Weapon, Goid Victim, float damage );

	// Awards experience to all associated skills and plays appropriate effects This will split EXP among the appropriate party members.
	void	AwardExperience				( Goid Client, Goid Weapon, double experience_points );
FEX void	AwardExperience				( Goid Client, Goid Weapon, float experience_points )		{ AwardExperience( Client, Weapon, double(experience_points) ); }

	// Awards eperience to a single character. This will not split experience among party members.
	 void	AwardExperienceSingle		( Goid Client, Goid Weapon, double experience_points );
FEX void	AwardExperienceSingle		( Goid Client, Goid Weapon, float experience_points )		{ AwardExperienceSingle( Client, Weapon, double(experience_points) ); }

	// Awards expereince to a primary skill such as melee, ranged, combat magic, or nature magic
FEX void	RCAwardPrimaryExperience	( Goid client, const char *sSkillName, double experience_points, bool bInnateOnly );

	// Determines the designation of the class based on best skill
FEX void	UpdateClassDesignation		( Goid Client );

	// Sets the stats of all the pack members of the party members party according to and adjusted overall party power
FEX void	SetPackMemberStats			( Goid party_member );

	// Plays level up sound on clients
FEX	void	RCPlayLevelUpSoundAndText	( Goid Client, const char *sSkillName );

	// Multiplayer scaling related
	float	GetDifficultyScaleLife		( int player_count );
	float	GetDifficultyScaleEP		( int player_count );

	// Experience Sharing
FEX float	GetExpShareDist				( void ) const			{ return ( m_ExpShareDist );	}
FEX	float	GetExpFormulaRatio			( void ) const			{ return ( m_ExpFormulaRatio );	}
FEX	float	GetExpShareRatio			( void ) const			{ return ( m_ExpShareRatio );	}

	// --- Physics related

	// Calculates 2-axis deviation for aiming a ranged weapon
FEX void	CalculateAimingError		( Goid Client, const char *szSkill, float &x_axis, float &y_axis );

	// Calculates 2-axis deviation for aiming derived from the weapon itself only
FEX void	CalculateWeaponAimingError	( Goid Weapon, float &x_axis, float &y_axis );

	// Handles special projectile collision items
FEX void	OnProjectileCollision		( Goid Projectile, Goid Collided );

	// Returns explosive magnitude contrived from damage and addtional magnitude
FEX float	CalculateMagnitude			( float mag_a, float damage );



	// --- Life/Mana modification

	// Changes the life of a go by the delta, if possible call damage go if this is an attack
FEX void	ChangeLife		( Goid Client, float Delta, DWORD rpcWhere );
FEX void	ChangeLife		( Goid Client, float Delta )		{  ChangeLife( Client, Delta, RPC_TO_ALL );  }
FEX void	ChangeLifeLocal	( Goid Client, float Delta )		{  ChangeLife( Client, Delta, RPC_TO_LOCAL );}

	// Changes the mana of a game object by the delta
FEX void	ChangeMana		( Goid Client, float Delta, DWORD rpcWhere );
FEX void	ChangeMana		( Goid Client, float Delta )		{  ChangeMana( Client, Delta, RPC_TO_ALL );  }
FEX void	ChangeManaLocal	( Goid Client, float Delta )		{  ChangeMana( Client, Delta, RPC_TO_LOCAL );}



	// --- Update

	// For actors that level up maximum life is calculated from strength, dexterity, intelligence
	float	CalculateMaxLife	( Go * pClient );
	void	RecalculateMaxLife	( Go * pClient );
	void	RegenerateLife		( Go * pClient, float deltaTime );

	// For actors that level up maximum mana is calculated from strength, dexterity, intelligence
	float	CalculateMaxMana	( Go * pClient );
	void	RecalculateMaxMana	( Go * pClient );
	void	RegenerateMana		( Go * pClient, float deltaTime );



	// --- Sound

	// Returns what type of sound should be played based on the damage ratio
FEX eHitType	DetermineHitType( float damage_done, float min_damage, float max_damage );

	// Plays the specifed combat sound
	void	PlayCombatSound		   ( eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow );
	void	RCPlayCombatSound	   ( eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow );



	// --- Misc

	// Base skills that every actor should have
	const SkillColl& GetBaseSkills	( void ) const		{ return m_ActorSkills; }

	// Calc a new sync period for an aspect
	float CalcSyncPeriod( void ) const					{ return m_SyncPeriod + Random( -m_SyncPeriodRand, m_SyncPeriodRand ); }

#if !GP_RETAIL

	// Particle Damage Counter (PDC)

	void	SetPDCEnable		( bool enable )			{ m_bPDCEnabled = enable; }
	bool	GetPDCEnable		( void ) const			{ return m_bPDCEnabled; }

	void	SetPDCResetPeriod	( float time )			{ m_PDCResetPeriod = time; }
	float	GetPDCResetPeriod	( void ) const			{ return m_PDCResetPeriod; }

	void	PDCReset			( void )				{ m_bPDCReset = true; }

#endif


private:

	float GetAreaDamageRadius( Goid Weapon );

	const gpstring& GetCombinedClass( int melee, int ranged, int natureMagic, int combatMagic, bool bMale );

	struct LevelUpIndicator
	{
		LevelUpIndicator( const gpstring &sSkillName, UINT32 range_min, UINT32 range_max,
						const gpstring &sSFX_script_name, const gpstring &sScript_params )
							: m_sSkillName( sSkillName )
							, m_Range_min( range_min )
							, m_Range_max( range_max )
							, m_sScript_name( sSFX_script_name )
							, m_sScript_params( sScript_params )
		{}

		gpstring			m_sSkillName;
		UINT32				m_Range_min;
		UINT32				m_Range_max;
		gpstring			m_sScript_name;
		gpstring			m_sScript_params;
	};


	SkillColl				m_ActorSkills;
	LevelUpColl				m_LevelUpIndicators;

	struct ClassLookup
	{
		gpstring sClass;
		int meleeSkillMin;
		int meleeSkillMax;
		int rangedSkillMin;
		int rangedSkillMax;
		int natureMagicSkillMin;
		int natureMagicSkillMax;
		int combatMagicSkillMin;
		int combatMagicSkillMax;
		bool bFemale;
		bool bMale;
	};

	typedef stdx::fast_vector< ClassLookup > ClassLookupColl;

	ClassLookupColl		m_ClassLookup;


	// Skrit Objects
	Skrit::Object*			m_RuleFunctions;			// skrit functions to handle different gameplay actions events.

	// Global constants
	float					m_AttackSkillScalar;
	float					m_AttackDexScalar;
	float					m_AttackIntScalar;
	float					m_DefendSkillScalar;
	float					m_DefendDexScalar;
	float					m_DefendIntScalar;

	float					m_Chance;
	float					m_AttackerDiffScalar;
	float					m_DefenderDiffScalar;
	float					m_AttackerHitCap;
	float					m_DefenderHitCap;

	float					m_DeathThreshold;
	float					m_EnemyNearSphere;
	float					m_MinUnconsciousDuration;
	float					m_ArmorScalar;

	float					m_ErrorScalar;
	float					m_Dex_;
	float					m_Int_;
	float					m_Skill_;

	float					m_DifficultyPlayer[ DIFFICULTY_SIZE ];
	float					m_DifficultyComputer[ DIFFICULTY_SIZE ];

	float					m_MaxLifeBase;
	float					m_MaxLifeConstant;
	float					m_MaxLifeStrPercent;
	float					m_MaxLifeDexPercent;
	float					m_MaxLifeIntPercent;

	float					m_MaxManaBase;
	float					m_MaxManaConstant;
	float					m_MaxManaStrPercent;
	float					m_MaxManaDexPercent;
	float					m_MaxManaIntPercent;

	float					m_StrPackInfluence;
	float					m_IntPackInfluence;
	float					m_DexPackInfluence;

	float					m_SyncPeriod;
	float					m_SyncPeriodRand;

#if !GP_RETAIL
	bool					m_bPDCReset;
	bool					m_bPDCEnabled;
	float					m_PDCResetPeriod;
	float					m_PDCTotalT;
	float					m_PDCTotalD;
#endif

	XPColl					m_ExperienceTable;
	float					m_MaxLevelUber;

	float					m_ELF_first;
	float					m_ELF_later;

	float					m_ExpShareDist;
	float					m_ExpFormulaRatio;
	float					m_ExpShareRatio;

	FloatColl				m_MPScaleLife;
	FloatColl				m_MPScaleXP;


	FUBI_SINGLETON_CLASS( Rules, "Rules for gameplay." );
	SET_NO_COPYING( Rules );
};


#define gRules Rules::GetSingleton()



struct CombatSoundXfer
{
	Rules::eHitType m_HitType;
	Goid            m_Weapon;
	bool            m_bFatalBlow;

	CombatSoundXfer( void )
	{
		::ZeroObject( *this );
	}

	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};



struct CombatSoundCallback
{
	Rules::eHitType m_HitType;
	Goid            m_Weapon;
	Goid            m_Victim;
	bool            m_bFatalBlow;
	bool            m_bPlayed;
	bool			m_bPlayOnFatalOnly;

	CombatSoundCallback( Rules::eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow = false )
	{
		m_HitType			= hitType;
		m_Weapon			= Weapon;
		m_Victim			= Victim;
		m_bFatalBlow		= bFatalBlow;
		m_bPlayed			= false;
		m_bPlayOnFatalOnly	= false;
	}

   ~CombatSoundCallback( void )
	{
		if (!( m_bPlayOnFatalOnly && !m_bFatalBlow ) && !m_bPlayed )
		{
			(*this)( m_bFatalBlow );
		}
	}

	void operator () ( bool bFatalBlow )
	{
		const bool bFatal = (bFatalBlow | m_bFatalBlow);

		if( !(m_bPlayOnFatalOnly && !bFatal ) && !m_bPlayed )
		{
			gRules.RCPlayCombatSound( m_HitType, m_Weapon, m_Victim, bFatal );
			m_bPlayed = true;
		}
	}

	operator CombatSoundCallback* ( void )
	{
		return ( this );
	}
};



//
// Simple class for keeping track of a skill
//

class Skill
{

public:

	Skill			( const gpstring &sName = "" );

	// Set owner go (for notification of xp receipts)
	void			SetOwnerGo			( Go* pGo )						{ m_pGo = pGo;				}

	// Init it (string is usually from fuel)
	bool			Init				( const char *sData );

	// Persistence
	bool			Xfer				( FuBi::PersistContext& persist );

	// Reset any gob's in this skill
	void			ResetModifiers		( void );

	// The name of the skill
	void			SetName				( const char *sName )			{ m_sName = sName;			}
	const gpstring&	GetName				( void ) const					{ return (m_sName);			}
	gpwstring		GetScreenName		( void ) const					{ return (ReportSys::TranslateW(m_sName)); }

	// The name of the class that uses the skill
	void			SetClassType		( const char *sType )			{ m_sClass = sType;			}
	const gpstring&	GetClassType		( void ) const					{ return (m_sClass);		}
	gpwstring		GetScreenClassType	( void ) const					{ return (ReportSys::TranslateW(m_sClass)); }

	// What percentage of incoming experience will be applied to strength improvement
	void			SetStrInfluence		( float val )					{ m_StrInfluence = val;		}
	float			GetStrInfluence		( void ) const					{ return (m_StrInfluence);	}

	// What percentage of incoming experience will be applied to dexterity improvement
	void			SetDexInfluence		( float val )					{ m_DexInfluence = val;		}
	float			GetDexInfluence		( void ) const					{ return (m_DexInfluence);	}

	// What percentage of incoming experience will be applied to intelligence improvement
	void			SetIntInfluence		( float val )					{ m_IntInfluence = val;		}
	float			GetIntInfluence		( void ) const					{ return (m_IntInfluence);	}

	// Get the total amount of experience this skill has
	void			SetExperience		( double val );
	double			GetExperience		( void ) const					{ return (m_XP);			}

	// The level of the skill
	void			SetMaxLevel			( float val )					{ m_MaxLevel = val; }
	float			GetMaxLevel			( void ) const					{ return m_MaxLevel; }

	void			SetNaturalLevel		( float val );
	float			GetNaturalLevel		( void ) const					{ return m_Level.GetNatural(); }
	void			SetLevel			( float val )					{ m_Level = val; }
	float			GetLevel			( void ) const					{ return (m_Level + m_LevelBias);	}
	float			GetLevelNoBias		( void ) const					{ return (m_Level );	}

	// Base level access
	float			GetLevelBias		( void ) const					{ return m_LevelBias; }
	float			GetLevelBiasModifier( void ) const					{ return m_LevelBiasModifier; }
	void			SetLevelBiasModifier( float val )					{ m_LevelBiasModifier = val; }

	// Returns percent mastery from 0 to 1.0;
	float			GetLevelMasteryRatio( void ) const					{ return ( GetLevel() - floorf(GetLevel()) ); }

	// Award experience to the skill
	bool			AwardExperience		( double experience_points, bool bSetDirty = true );

private:
	gpstring		m_sName;
	gpstring		m_sClass;

	float			m_StrInfluence;
	float			m_DexInfluence;
	float			m_IntInfluence;

	double			m_XP;
	double			m_XP_NextLevel;
	GobFloat		m_Level;
	float			m_LevelBias;
	float			m_LevelBiasModifier;
	float			m_MaxLevel;

	Go*				m_pGo;
};



#endif
