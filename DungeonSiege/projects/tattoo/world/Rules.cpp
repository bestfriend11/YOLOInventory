#include "precomp_world.h"

#include "Rules.h"
#include "aiquery.h"
#include "contentdb.h"
#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAttack.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "GoDefend.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoPhysics.h"
#include "GoSupport.h"
#include "GoData.h"
#include "server.h"
#include "SkritEngine.h"
#include "worldmessage.h"
#include "gameauditor.h"
#include "player.h"
#include "sim.h"
#include "stdhelp.h"
#include "stringtool.h"
#include "Victory.h"
#include "WorldFx.h"
#include "WorldTime.h"

// this number indicates the number of levels to display the "full" version of the level up messages.
// after this level is passed, a shorter one is used.
const int MAX_LEVELS_FOR_FULL_LEVELUP_DISPLAY = 5;

#if !GP_RETAIL
	AuditorDb * pCombatProbe = NULL;
#endif

///////////////////////////////////////////////////////////////

SKRIT_IMPORT void OnAwardExperience( Skrit::Object* skrit, const char* funcName, Goid /*Client*/, Goid /*Weapon*/, float /*experience_points*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( OnAwardExperience, skrit, funcName );  }

SKRIT_IMPORT void OnCustomDamage( Skrit::HObject skrit, const char* funcName, Go* /*victim*/, Go* /*attacker*/, Go* /*weapon*/, float /*amount*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( OnCustomDamage, skrit, funcName );  }

///////////////////////////////////////////////////////////////

Rules::Rules()
{
	m_AttackSkillScalar		= 1.0f;
	m_AttackDexScalar		= 1.0f;
	m_AttackIntScalar		= 1.0f;
	m_DefendSkillScalar		= 1.0f;
	m_DefendDexScalar		= 1.0f;
	m_DefendIntScalar		= 1.0f;
	m_Chance				= 50.0f;
	m_AttackerDiffScalar	= 3.0f;
	m_DefenderDiffScalar	= 1.0f;
	m_AttackerHitCap		= 95.0f;
	m_DefenderHitCap		= 5.0f;
	m_DeathThreshold		= 0.33f;
	m_EnemyNearSphere		= 5.0f;
	m_MinUnconsciousDuration= 5.0f;
	m_ArmorScalar			= 0.45f;
	m_MaxLifeBase			= 0.0f;
	m_MaxLifeConstant		= 21.0f;
	m_MaxLifeStrPercent		= 0.6f;
	m_MaxLifeDexPercent		= 0.2f;
	m_MaxLifeIntPercent		= 0.2f;
	m_MaxManaBase			= 0.0f;
	m_MaxManaConstant		= 15.0f;
	m_MaxManaStrPercent		= 0.1f;
	m_MaxManaDexPercent		= 0.1f;
	m_MaxManaIntPercent		= 0.8f;
	m_ErrorScalar			= 5.0f;
	m_Dex_					= 0.35f;
	m_Int_					= 0.10f;
	m_Skill_				= 0.55f;
	m_SyncPeriod			= 3.0f;
	m_SyncPeriodRand		= 2.0f;
	m_StrPackInfluence		= 1;
	m_IntPackInfluence		= 1;
	m_DexPackInfluence		= 1;
	m_ELF_first				= 1.0f;
	m_ELF_later				= 1.0f;
	m_MaxLevelUber			= 0;
	m_ExpShareDist			= 20.0f;
	m_ExpFormulaRatio		= 0.5f;
	m_ExpShareRatio			= 0.75f;



	FastFuelHandle	hCombatBlock( "world:global:formula:combat_constants" );
	if( !hCombatBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:formula:combat_constants in gas tree!");
	}

	FastFuelHandle	hRecalcBlock( "world:global:formula:recalculation_constants" );
	if( !hRecalcBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:formula:recalculation_constants in gas tree!");
	}

	FastFuelHandle	hPackBlock( "world:global:formula:pack_constants" );
	if( !hPackBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:formula:pack_constants in gas tree!");
	}

	FastFuelHandle	hRatingBlock;

	// Get the rating block settings
	hRatingBlock = hCombatBlock.GetChildNamed		( "attack_rating" );

	hRatingBlock.Get( "skill_scalar",				m_AttackSkillScalar			);
	hRatingBlock.Get( "dex_scalar",					m_AttackDexScalar			);
	hRatingBlock.Get( "int_scalar",					m_AttackIntScalar			);

	hRatingBlock = hCombatBlock.GetChildNamed		( "defend_rating" );

	hRatingBlock.Get( "skill_scalar",				m_DefendSkillScalar			);
	hRatingBlock.Get( "dex_scalar",					m_DefendDexScalar			);
	hRatingBlock.Get( "int_scalar",					m_DefendIntScalar			);


	// Get the hit chance settings
	hCombatBlock.Get( "hit_chance",					m_Chance					);
	hCombatBlock.Get( "attacker_diff_scalar",		m_AttackerDiffScalar		);
	hCombatBlock.Get( "defender_diff_scalar",		m_DefenderDiffScalar		);
	hCombatBlock.Get( "attacker_hit_cap",			m_AttackerHitCap			);
	hCombatBlock.Get( "defender_hit_cap",			m_DefenderHitCap			);

	hCombatBlock.Get( "death_threshold",			m_DeathThreshold			);

	hCombatBlock.Get( "enemy_near_sphere",			m_EnemyNearSphere			);
	hCombatBlock.Get( "min_unconscious_duration",	m_MinUnconsciousDuration	);

	hCombatBlock.Get( "armor_scalar",				m_ArmorScalar				);

	hCombatBlock.Get( "error_scalar",				m_ErrorScalar				);
	hCombatBlock.Get( "dex_scalar",					m_Dex_						);
	hCombatBlock.Get( "int_scalar",					m_Int_						);
	hCombatBlock.Get( "skill_scalar",				m_Skill_					);


	hCombatBlock.Get( "difficulty_easy_player",		m_DifficultyPlayer[ DIFFICULTY_EASY ]	);
	hCombatBlock.Get( "difficulty_medium_player",	m_DifficultyPlayer[ DIFFICULTY_MEDIUM ]	);
	hCombatBlock.Get( "difficulty_hard_player",		m_DifficultyPlayer[ DIFFICULTY_HARD ]	);
	hCombatBlock.Get( "difficulty_easy_computer",	m_DifficultyComputer[ DIFFICULTY_EASY ]	);
	hCombatBlock.Get( "difficulty_medium_computer",	m_DifficultyComputer[ DIFFICULTY_MEDIUM ]);
	hCombatBlock.Get( "difficulty_hard_computer",	m_DifficultyComputer[ DIFFICULTY_HARD ]	);

	// Get settings for actors that can level up so that the max life and
	// max mana are calculated based off of their stats
	hRecalcBlock.Get( "max_life_base",				m_MaxLifeBase				);
	hRecalcBlock.Get( "max_life_constant",			m_MaxLifeConstant			);
	hRecalcBlock.Get( "max_life_str_percent",		m_MaxLifeStrPercent			);
	hRecalcBlock.Get( "max_life_dex_percent",		m_MaxLifeDexPercent			);
	hRecalcBlock.Get( "max_life_int_percent",		m_MaxLifeIntPercent			);

	hRecalcBlock.Get( "max_mana_base",				m_MaxManaBase				);
	hRecalcBlock.Get( "max_mana_constant",			m_MaxManaConstant			);
	hRecalcBlock.Get( "max_mana_str_percent",		m_MaxManaStrPercent			);
	hRecalcBlock.Get( "max_mana_dex_percent",		m_MaxManaDexPercent			);
	hRecalcBlock.Get( "max_mana_int_percent",		m_MaxManaIntPercent			);

	hRecalcBlock.Get( "sync_period",				m_SyncPeriod				);
	hRecalcBlock.Get( "sync_period_rand",			m_SyncPeriodRand			);

	// Get the settings to determine the pack animal attributes based on party strength
	hPackBlock.Get( "strength_influence",			m_StrPackInfluence			);
	hPackBlock.Get( "intelligence_influence",		m_IntPackInfluence			);
	hPackBlock.Get( "dexterity_influence",			m_DexPackInfluence			);

	if( m_SyncPeriodRand > m_SyncPeriod )
	{
		m_SyncPeriodRand = m_SyncPeriod;
		gpwarningf(("SyncPeriodRand cannot be greater than m_SyncPeriod from world:global:formula:recalculation_constants!\n"));
	}

	// Build the global skill list to be used by all actors
	FastFuelHandle		hSkillsBlock( "world:global:formula:actor_skills" );
	if( !hSkillsBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:actor_skills in gas tree!");
	}

	FastFuelHandleColl	skill_list;
	hSkillsBlock.ListChildrenNamed( skill_list, "skill*" );

	FastFuelHandleColl::iterator iSkill		= skill_list.begin();
	FastFuelHandleColl::iterator iSkill_end = skill_list.end();


	for(; iSkill != iSkill_end; ++iSkill )
	{
		gpstring	sName;
		gpstring	sClass;
		float		max_level		=	0;
		float		str_influence	=	0;
		float		dex_influence	=	0;
		float		int_influence	=	0;

		(*iSkill).Get( "name",			sName			);
		(*iSkill).Get( "screen_name",	sClass			);
		(*iSkill).Get( "max_level",		max_level		);
		(*iSkill).Get( "str_influence",	str_influence	);
		(*iSkill).Get( "dex_influence",	dex_influence	);
		(*iSkill).Get( "int_influence",	int_influence	);

		m_MaxLevelUber += max_level;

		Skill	new_skill			( sName			);
		new_skill.SetClassType		( sClass		);
		new_skill.SetMaxLevel		( max_level		);
		new_skill.SetStrInfluence	( str_influence );
		new_skill.SetDexInfluence	( dex_influence );
		new_skill.SetIntInfluence	( int_influence );

		m_ActorSkills.push_back( new_skill );

		// Now process any level up indicators that go along with this skill
		FastFuelHandleColl	indicator_list;
		(*iSkill).ListChildrenNamed( indicator_list, "level_up_indicator*" );

		FastFuelHandleColl::iterator iIndicator		= indicator_list.begin();
		FastFuelHandleColl::iterator iIndicator_end = indicator_list.end();

		for(; iIndicator != iIndicator_end; ++iIndicator )
		{
			gpstring sLevelRange;
			gpstring sScript;
			(*iIndicator).Get( "level_range",	sLevelRange );
			(*iIndicator).Get( "script",		sScript		);

			if( ( stringtool::GetNumDelimitedStrings( sLevelRange, '-' ) == 2 ) )
			{
				gpstring sMin, sMax, sScript_name, sScript_params;

				stringtool::GetDelimitedValue( sLevelRange, '-', 0, sMin );
				stringtool::GetDelimitedValue( sLevelRange, '-', 1, sMax );

				stringtool::GetDelimitedValue( sScript, ',', 0, sScript_name );
				if( stringtool::GetNumDelimitedStrings( sScript, ',' ) == 2 ) {
					stringtool::GetDelimitedValue( sScript, ',', 1, sScript_params );
				}

				UINT32 min = atoi( sMin.c_str() );
				UINT32 max = atoi( sMax.c_str() );

				LevelUpIndicator new_indicator( sName, min, max, sScript_name, sScript_params );
				m_LevelUpIndicators.push_back( new_indicator );
			}
			else
			{
				gpassertf( 0, ( "Rules::Rules - Skill %s does not define a valid level up range\n", sName ));
			}
		}
	}

	// Set the uber skill level max based on the max_level total
	Skill *pUberSkill = NULL;

	if( m_ActorSkills.GetSkill( "Uber", &pUberSkill ) )
	{
		pUberSkill->SetMaxLevel( m_MaxLevelUber - pUberSkill->GetMaxLevel() );
	}
	else
	{
		gpfatal( "Uber skill is not defined in formulas.gas!\n" );
	}


	// Load the experience table
	FastFuelHandle		hXPBlock( "world:global:formula:experience_table" );
	if( !hXPBlock.IsValid() )
	{
		gpfatal("Cannot find world:global:formula:experience_table in gas tree!");
	}
	FastFuelFindHandle	xp_table_entry( hXPBlock );

	xp_table_entry.FindFirstKeyAndValue();
	gpstring key, value;

	xp_table_entry.GetNextKeyAndValue( key, value );

	const size_t count = stringtool::GetNumDelimitedStrings( value.c_str(), ',' );

	for( size_t i = 0; i < count; ++i )
	{
		double dValue = 0;
		if( stringtool::GetDelimitedValue( value.c_str(), ',', i, dValue ) )
		{
			m_ExperienceTable.push_back( dValue );
		}
	}

	// load the experience limiting values
	FastFuelHandle	hELF( "world:global:formula:experience_limiting_factors" );
	if( hELF )
	{
		hELF.Get( "first_level",	m_ELF_first );
		hELF.Get( "later_levels",	m_ELF_later );
	}

	// load the experience sharing constants
	FastFuelHandle	hExpShare( "world:global:formula:experience_sharing" );
	if( hExpShare )
	{
		hExpShare.Get( "exp_sharing_formula_ratio",	m_ExpFormulaRatio );
		hExpShare.Get( "exp_sharing_distance",		m_ExpShareDist );
		hExpShare.Get( "exp_sharing_ratio",		m_ExpShareRatio );
	}

	FastFuelHandle hClassLookup( "world:global:class_lookup" );
	if ( hClassLookup )
	{
		FastFuelHandleColl hlClasses;
		FastFuelHandleColl::iterator iClass;
		hClassLookup.ListChildren( hlClasses, 1 );
		for ( iClass = hlClasses.begin(); iClass != hlClasses.end(); ++iClass )
		{
			ClassLookup cl;
			cl.meleeSkillMin		= 0;
			cl.meleeSkillMax		= 0;
			cl.rangedSkillMin		= 0;
			cl.rangedSkillMax		= 0;
			cl.natureMagicSkillMin	= 0;
			cl.natureMagicSkillMax	= 0;
			cl.combatMagicSkillMin	= 0;
			cl.combatMagicSkillMax	= 0;
			cl.bFemale				= true;
			cl.bMale				= true;

			(*iClass).Get( "screen_name", cl.sClass );
			(*iClass).Get( "is_female", cl.bFemale );
			(*iClass).Get( "is_male", cl.bMale );

			FastFuelHandleColl hlSkillReq;
			FastFuelHandleColl::iterator iSkillReq;
			(*iClass).ListChildren( hlSkillReq, 1 );
			for ( iSkillReq = hlSkillReq.begin(); iSkillReq != hlSkillReq.end(); ++iSkillReq )
			{
				gpstring sName;
				(*iSkillReq).Get( "name", sName );
				if ( sName.same_no_case( "melee" ) )
				{
					(*iSkillReq).Get( "min_range", cl.meleeSkillMin );
					(*iSkillReq).Get( "max_range", cl.meleeSkillMax );
				}
				else if ( sName.same_no_case( "ranged" ) )
				{
					(*iSkillReq).Get( "min_range", cl.rangedSkillMin );
					(*iSkillReq).Get( "max_range", cl.rangedSkillMax );
				}
				else if ( sName.same_no_case( "combat magic" ) )
				{
					(*iSkillReq).Get( "min_range", cl.combatMagicSkillMin );
					(*iSkillReq).Get( "max_range", cl.combatMagicSkillMax );
				}
				else if ( sName.same_no_case( "nature magic" ) )
				{
					(*iSkillReq).Get( "min_range", cl.natureMagicSkillMin );
					(*iSkillReq).Get( "max_range", cl.natureMagicSkillMax );
				}
			}

			m_ClassLookup.push_back( cl );
		}
	}

	// Set the scaling tables
	FastFuelHandle	hMPScaling( "world:global:formula:mp_scaling" );
	if( hMPScaling )
	{
		struct reader
		{
			static void GetTableValues( const char *sKey, FloatColl &coll, FastFuelHandle &handle )
			{
				gpstring sValue;

				if( handle.Get( sKey, sValue ) )
				{
					const size_t count = stringtool::GetNumDelimitedStrings( sValue, ',' );

					for( size_t i = 0; i < count; ++i )
					{
						float value = 0;
						if( stringtool::GetDelimitedValue( sValue.c_str(), ',', i, value ) )
						{
							coll.push_back( value );
						}
					}
				}
				else
				{
					coll.push_back( 1 );
				}
			}
		};

		reader::GetTableValues( "life", m_MPScaleLife,	hMPScaling );
		reader::GetTableValues( "xp",	m_MPScaleXP,	hMPScaling );
	}

	// get the skrit object
	Skrit::CreateReq createReq( "world/global/skrits/rules" );
	m_RuleFunctions = gSkritEngine.CreateNewObject( createReq );

#if !GP_RETAIL
	pCombatProbe = gGameAuditor.AddDb( "combatprobe", false );

	m_bPDCReset		= true;
	m_bPDCEnabled	= false;
	m_PDCResetPeriod= 1.0f;
	m_PDCTotalT		= 0.0f;
	m_PDCTotalD		= 0.0f;

#endif
}

Rules::~Rules()
{
	Delete ( m_RuleFunctions );
}

bool
Rules::CanHit( Goid Attacker, Goid Victim, Goid AttackerWeapon, Goid VictimWeapon )
{
	CHECK_SERVER_ONLY;

	// Validation
	GoHandle hAttacker( Attacker );
	GoHandle hVictim( Victim );

	if( !hAttacker || !hVictim )
	{
		gpassertf( hAttacker, ("In Rules::CanHit(), Attacker GUID is not valid"));
		gpassertf( hVictim, ("In Rules::CanHit(), Victim GUID is not valid"));
		return false;
	}

	// Check for instant hit
	if( !hAttacker->HasActor() || !hVictim->HasActor() )
	{
		return true;
	}

	// Setup type of attack
	GoHandle hAttackerWeapon( AttackerWeapon );
	GoHandle hVictimWeapon( VictimWeapon );

	const bool bArmedAttacker( hAttackerWeapon && hAttackerWeapon->HasAttack() && (AttackerWeapon != Attacker) );
	const bool bArmedVictim( hVictimWeapon && hVictimWeapon->HasAttack() && (VictimWeapon != Victim) );

	const bool isMeleeWeapon = hAttackerWeapon && hAttackerWeapon->IsMeleeWeapon();
	const bool isRangedWeapon = hAttackerWeapon && hAttackerWeapon->IsRangedWeapon();

	// Check if the victim dodged the hit
	if ( hVictim->HasDefend() )
	{
		if ( hVictim->GetDefend()->GetChanceToDodgeHit() > Random( 100.0f ) )
		{
			return false;
		}

		if ( isMeleeWeapon && (hVictim->GetDefend()->GetChanceToDodgeHitMelee() > Random( 100.0f )) )
		{
			return false;
		}
		else if ( isRangedWeapon && (hVictim->GetDefend()->GetChanceToDodgeHitRanged() > Random( 100.0f )) )
		{
			return false;
		}
	}

	// Get the actor components
	GoActor *pActorAttacker	= hAttacker->GetActor();
	GoActor *pActorVictim	= hVictim->GetActor();

	// Based on the attack scenario determine what the attacker and victim skill levels should be

	// Attack Scenario		Attacker		Victim
	//--------------------------------------------------------
	// Karate Vs Karate		no skill		no skill
	// Karate Vs Melee		no skill		melee skill
	// Karate Vs Ranged		no skill		no skill
	// Karate Vs Magic		no skill		no skill

	// Melee Vs Karate		melee skill		no skill
	// Melee Vs Melee		melee skill		melee skill
	// Melee Vs Ranged		melee skill		no skill
	// Melee Vs Magic		melee skill		no skill

	// Ranged Vs Karate		ranged skill	no defensive calc
	// Ranged Vs Melee		ranged skill	no defensive calc
	// Ranged Vs Ranged		ranged skill	no defensive calc
	// Ranged Vs Magic		ranged skill	no defensive calc

	// Magic Vs Karate		magic skill		no defensive calc
	// Magic Vs Melee		magic skill		no defensive calc
	// Magic Vs Ranged		magic skill		no defensive calc
	// Magic Vs Magic		magic skill		no defensive calc

	float		attacker_skill_level	=	0;
	float		victim_skill_level		=	0;
	float		attacker_rating			=	0;
	float		victim_rating			=	0;
	bool		bPerformDefensiveCalc	=	false;
	gpstring	attacker_skill;
	gpstring	victim_skill;

	// Determine the attacker skill level to use
	if( bArmedAttacker )
	{
		// Attacker skill will be melee, ranged, magic
		attacker_skill		= hAttackerWeapon->GetAttack()->GetSkillClass();
		attacker_skill_level= pActorAttacker->GetSkillLevel( attacker_skill );
	}

	// Determine the victim skill level to use
	if( (attacker_skill.empty() ) || attacker_skill.same_no_case( "melee" ) )
	{
		if( bArmedVictim && hVictimWeapon->GetAttack()->GetSkillClass().same_no_case( "melee" ) )
		{
			victim_skill = "melee";
		}
		bPerformDefensiveCalc = true;
	}

	if( pActorVictim && !victim_skill.empty()  )
	{
		victim_skill_level = pActorVictim->GetSkillLevel( victim_skill );
	}

	// Get the other skill traits that will be used in the final calculation
	const float attacker_dex = floorf(pActorAttacker->GetSkillLevel( "dexterity"	));
	const float attacker_int = floorf(pActorAttacker->GetSkillLevel( "intelligence"));

	const float victim_dex	 = floorf( pActorVictim ? pActorVictim->GetSkillLevel ( "dexterity"	) : 0.0f );
	const float victim_int	 = floorf( pActorVictim ? pActorVictim->GetSkillLevel ( "intelligence") : 0.0f );

	// Calculate the combatants ratings based on skill level and other attributes
	attacker_rating		= (m_AttackSkillScalar * attacker_skill_level) + (m_AttackDexScalar * attacker_dex) +
							(m_AttackIntScalar * attacker_int);

	if( bPerformDefensiveCalc )
	{
		victim_rating	= (m_DefendSkillScalar * victim_skill_level) + (m_DefendDexScalar * victim_dex) +
							(m_DefendIntScalar * victim_int);
	}

	const float diff = attacker_rating - victim_rating;
	float chance = m_Chance;

	// Attacker has the advantage
	if( diff > 0.0f )
	{
		chance += diff * m_AttackerDiffScalar;
		if( chance > m_AttackerHitCap )
		{
			chance = m_AttackerHitCap;
		}
	}
	else
	{
	// Victim has the advantage
		chance += diff * m_DefenderDiffScalar;
		if( chance < m_DefenderHitCap )
		{
			chance = m_DefenderHitCap;
		}
	}

#if !GP_RETAIL
	pCombatProbe->Set( MakeInt( Attacker ), "chance_hit_before_bonus", chance );
#endif

	const float baseChance = chance;

	float chanceToHitBonus = 0.0f;
	gGameAuditor.Get( MakeInt( Attacker ), gpstring( "chance_to_hit_bonus_verses_" ).append( hVictim->GetTemplateName() ), chanceToHitBonus );

	chance += (chanceToHitBonus * baseChance);
	chance += (hAttacker->GetAttack()->GetChanceToHitBonus() * baseChance);

	if ( hAttackerWeapon && hAttackerWeapon->IsMeleeWeapon() )
	{
		chance += (float) (0.01 * hAttacker->GetAttack()->GetChanceToHitBonusMelee() * baseChance);
	}
	if ( hAttackerWeapon && hAttackerWeapon->IsRangedWeapon() )
	{
		chance += (float) (0.01 * hAttacker->GetAttack()->GetChanceToHitBonusRanged() * baseChance);
	}

	if( chance > m_AttackerHitCap )
	{
		chance = m_AttackerHitCap;
	}

#if !GP_RETAIL
	pCombatProbe->Set( MakeInt( Attacker ), "chance_hit", chance );
	pCombatProbe->Set( MakeInt( Attacker ), "attacker_rating", attacker_rating );
	pCombatProbe->Set( MakeInt( Attacker ), "victim_rating", victim_rating );
#endif

	if ( chance > Random( 100.0f ) )
	{
		return true;
	}
	return false;
}


bool
Rules::CheckForCriticalHit( Goid Attacker, Goid Victim )
{
	GoHandle hAttacker( Attacker );
	GoHandle hVictim( Victim );

	if( !(hAttacker && hVictim ) )
	{
		return false;
	}

	bool critical_hit_immune = false;

	if( hVictim->HasDefend() )
	{
		critical_hit_immune = hVictim->GetDefend()->GetIsCriticalHitImmune();
	}

	if( hAttacker->HasActor() && hAttacker->GetActor()->GetCanLevelUp() && !critical_hit_immune )
	{
		const float random_unit = Random( 0.0f, 1.0f );
		return ( hAttacker->GetAttack()->GetCriticalHitChance() >= random_unit );
	}

	return false;
}


float
Rules::GetTotalDefense( Goid Client )
{
	GoHandle hClient( Client );
	float total_defense = 0;

	if( hClient )
	{
#if !GP_RETAIL
		pCombatProbe->Set( MakeInt( Client ), "defense_base", 0.0f );
		pCombatProbe->Set( MakeInt( Client ), "defense_dexterity", 0.0f );
		pCombatProbe->Set( MakeInt( Client ), "defense_equip", 0.0f );
#endif

		// Get base defense
		if( hClient->HasDefend() )
		{
#if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Client ), "defense_base", hClient->GetDefend()->GetDefense() );
#endif
			total_defense += hClient->GetDefend()->GetDefense();
		}
		// Get defense bonus from dexterity
		if( hClient->HasActor() )
		{
#if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Client ), "defense_dexterity", hClient->GetActor()->GetSkillLevel( "dexterity" ) / 3.0f );
#endif
			const float dexterity = hClient->GetActor()->GetSkillLevel( "dexterity" );
			float dexterity_bonus = 0;

			if( dexterity >= 10.0f )
			{
				dexterity_bonus = ((( dexterity - 10.0f ) * 3.5f) + 9.0f );
			}
			else
			{
				dexterity_bonus = dexterity / 1.5f;
			}

			total_defense += dexterity_bonus;
		}
		// Get equipped items defense
		if( hClient->HasActor() )
		{
			for( UINT32 i=ES_VISIBLE_BEGIN; i<ES_VISIBLE_END; i++ )
			{
				Go * pEquipped = hClient->GetInventory()->GetEquipped( (enum eEquipSlot)i );

				if( pEquipped  && pEquipped->HasDefend() )
				{
#if !GP_RETAIL
					pCombatProbe->IncrementFloat( MakeInt( Client ), "defense_equip", pEquipped->GetDefend()->GetDefense() );
#endif
					total_defense += pEquipped->GetDefend()->GetDefense();
				}
			}
		}
	}
	return total_defense;
}


float
Rules::GetTotalFireResistance( Goid Client )
{
	GoHandle hClient( Client );
	float total_fire_resistance = 0;

	if( hClient && hClient->HasPhysics() )
	{
		// Get base resistance
		total_fire_resistance += hClient->GetPhysics()->GetFireResistance();

		// Go through any equipped items and total their resistances
		if( hClient->HasActor() )
		{
			for( UINT32 i=ES_VISIBLE_BEGIN; i<ES_VISIBLE_END; i++ )
			{
				Go * pEquipped = hClient->GetInventory()->GetEquipped( (enum eEquipSlot)i );

				if( pEquipped  && pEquipped->HasPhysics() )
				{
					total_fire_resistance += pEquipped->GetPhysics()->GetFireResistance();
				}
			}
		}
	}
	return min_t( total_fire_resistance, 1.0f );
}


float
Rules::CalculateDamage( Goid Attacker, Goid Weapon, Goid Victim, float min_damage, float max_damage, float total_defense, bool bPiercing, bool duration_damage )
{
 	CHECK_SERVER_ONLY;

	GoHandle hAttacker	( Attacker );
	GoHandle hWeapon	( Weapon );
	GoHandle hVictim	( Victim );

	float damage = 0;

	// we need to have the attacker and victim, but not 
	// nessisarily the weapon.
	if( !(hAttacker && hVictim) )
	{
		return damage;
	}

	const float adj_max_damage = max_t( min_damage, (0.95f * max_damage) );

	damage = Random( min_damage, adj_max_damage );

#	if !GP_RETAIL
		pCombatProbe->Set( "damage_roll", damage );
		pCombatProbe->Set( "pct_blocked", 0.0f );
#	endif

	if( total_defense > 0 )
	{
		float pct_blocked = 0.0f;

		if ( !bPiercing )
		{
			pct_blocked = max_t((1.0f - m_ArmorScalar * damage / total_defense), 0.05f );
#			if !GP_RETAIL
				pCombatProbe->Set( "pct_blocked", pct_blocked );
#			endif
		}

		damage = max_t( (damage - damage * pct_blocked), 0.05f * min_damage );
	}

	// compute any special information for a passed in weapon
	if( hWeapon )
	{
		const bool bMeleeWeapon = hWeapon->IsMeleeWeapon();
		const bool bRangedWeapon = hWeapon->IsRangedWeapon();

		// check for blocked damage
		if ( hVictim->HasDefend() )
		{
			float baseAmount = damage;

			damage -= (hVictim->GetDefend()->GetBlockDamage() / 100.0f) * baseAmount;
			damage -= hVictim->GetDefend()->GetBlockPartDamage();

			if ( bMeleeWeapon )
			{
				damage -= (hVictim->GetDefend()->GetBlockMeleeDamage() / 100.0f) * baseAmount;
				damage -= hVictim->GetDefend()->GetBlockPartMeleeDamage();
			}
			else if ( bRangedWeapon )
			{
				damage -= (hVictim->GetDefend()->GetBlockRangedDamage() / 100.0f) * baseAmount;
				damage -= hVictim->GetDefend()->GetBlockPartRangedDamage();
			}

			if ( bPiercing )
			{
				damage -= (hVictim->GetDefend()->GetBlockPiercingDamage() / 100.0f) * baseAmount;
				damage -= hVictim->GetDefend()->GetBlockPartPiercingDamage();
			}

			if ( damage < 0.0f )
			{
				damage = 0.0f;
			}
		}

		float piercingDamage = 0.0f;

		// calculate damage bonuses
		if ( hAttacker->HasAttack() )
		{
			// damage bonus versus specific type
			if ( !hAttacker->GetAttack()->GetDamageToType().empty() &&
				  hAttacker->GetAttack()->GetDamageToType().same_no_case( hVictim->GetTemplateName() ) )
			{
				damage += (hAttacker->GetAttack()->GetAmountDamageToType() / 100.0f) * damage;
			}

			// piercing damage bonus
			if ( hAttacker->GetAttack()->GetPiercingDamageMax() > 0 )
			{
				piercingDamage = Random( hAttacker->GetAttack()->GetPiercingDamageMin(), hAttacker->GetAttack()->GetPiercingDamageMax() );
			}

			if ( bMeleeWeapon && (hAttacker->GetAttack()->GetPiercingDamageMeleeMax() > 0) )
			{
				piercingDamage += Random( hAttacker->GetAttack()->GetPiercingDamageMeleeMin(), hAttacker->GetAttack()->GetPiercingDamageMeleeMax() );
			}
			else if ( bRangedWeapon && (hAttacker->GetAttack()->GetPiercingDamageRangedMax() > 0) )
			{
				piercingDamage += Random( hAttacker->GetAttack()->GetPiercingDamageRangedMin(), hAttacker->GetAttack()->GetPiercingDamageRangedMax() );
			}

			if ( hVictim->HasDefend() )
			{
				piercingDamage -= (hVictim->GetDefend()->GetBlockPiercingDamage() / 100.0f) * piercingDamage;
			}

			damage += piercingDamage;

			// custom damage
			if ( !hAttacker->GetAttack()->GetCustomDamageMap().empty() )
			{
				GoAttack::CustomDamageMap::const_iterator i = hAttacker->GetAttack()->GetCustomDamageMap().begin();
				for ( ; i != hAttacker->GetAttack()->GetCustomDamageMap().end(); ++i )
				{
					const GoAttack::CustomDamage * pDamage = i->second;

					if ( (!pDamage->m_bMeleeDamage && !pDamage->m_bRangedDamage) ||
						 (pDamage->m_bMeleeDamage && hWeapon && hWeapon->IsMeleeWeapon()) ||
						 (pDamage->m_bRangedDamage && hWeapon && hWeapon->IsRangedWeapon()) )
					{
						float chance = 100.0f;
						if ( pDamage->m_DamageChance > 0.0f )
						{
							chance = pDamage->m_DamageChance;
						}

						if ( chance >= Random( 100.0f ) )
						{
							float customDamage = Random( pDamage->m_DamageMin, pDamage->m_DamageMax );

							if ( hVictim->HasDefend() )
							{
								customDamage -= (hVictim->GetDefend()->GetBlockCustomDamage( pDamage->m_DamageType ) / 100.0f) * customDamage;
								customDamage -= hVictim->GetDefend()->GetBlockPartCustomDamage( pDamage->m_DamageType );
							}

							damage += customDamage;

							if ( (customDamage > 0.0f) && pDamage->m_Skrit.IsValid() && !duration_damage )
							{
								::OnCustomDamage( pDamage->m_Skrit, "on_custom_damage$", hVictim, hAttacker, hWeapon, customDamage );
							}
						}
					}
				}
			}

			// life and mana stealing bonus
			if ( bMeleeWeapon )
			{
				damage += hAttacker->GetAttack()->GetLifeStealAmount();
				damage += hAttacker->GetAttack()->GetManaStealAmount();
			}
		}

	}

	return damage;
}


bool
Rules::GetDamageRange( Goid Client, Goid Weapon, float &min_damage, float &max_damage, bool bUseDifficulty )
{
	GoHandle hClient( Client );
	GoHandle hWeapon( Weapon );

	if( (hClient && hWeapon) && hClient->HasAttack() && hWeapon->HasAttack() )
	{
		GoActor *pActor = hClient->QueryActor();
		GoAttack *pAttack = hClient->QueryAttack();

		if( (Client == Weapon) || hWeapon->GetAttack()->GetIsMelee() )
		{
			//  ###########################
			//  ### Melee/Karate damage ###
			//  ###########################

			// Get the weapon damage range
			if ( bUseDifficulty )
			{
				min_damage = hWeapon->GetAttack()->GetDamageMin();
				max_damage = hWeapon->GetAttack()->GetDamageMax();
			}
			else
			{
				min_damage = hWeapon->GetAttack()->GetBaseDamageMin();
				max_damage = hWeapon->GetAttack()->GetBaseDamageMax();
			}

			// Should we add a strength bonus?
			if( pActor && pActor->GetCanLevelUp() )
			{
#if !GP_RETAIL
				pCombatProbe->Set( MakeInt( Client ), "damage_weapon_min", min_damage );
				pCombatProbe->Set( MakeInt( Client ), "damage_weapon_max", max_damage );
#endif
				// Calculate strength bonus
				const float strength = floorf(pActor->GetSkillLevel( "strength" ));

				INT32 min_str_bonus = 1;
				INT32 max_str_bonus = 1;

				if( strength >= 10.0f )
				{
					min_str_bonus = INT32( 2.0f + (( strength - 10.0f ) * 2.07f ) );
					max_str_bonus = INT32( 4.0f + (( strength - 10.0f ) * 2.13f ) );
				}
				else
				{
					min_str_bonus = 2;
					max_str_bonus = 4;
				}

				min_damage += ( min_str_bonus + pAttack->GetDamageBonusMinMelee() );
				max_damage += ( max_str_bonus + pAttack->GetDamageBonusMaxMelee() );

#if !GP_RETAIL
				pCombatProbe->Set( MakeInt( Client ), "damage_strength_min", float(min_str_bonus) );
				pCombatProbe->Set( MakeInt( Client ), "damage_strength_max", float(max_str_bonus) );
#endif
			}
			else
			{
				// Non-actors use base damage from the weapon if they aren't the weapon
				if( Client != Weapon )
				{
					min_damage += ( hClient->GetAttack()->GetDamageMin() + pAttack->GetDamageBonusMinMelee() );
					max_damage += ( hClient->GetAttack()->GetDamageMax() + pAttack->GetDamageBonusMaxMelee() );
				}
				else
				{
					// this non-level increasing actor has a melee bonus but doesn't use a weapon - so apply it
					min_damage += pAttack->GetDamageBonusMinMelee();
					max_damage += pAttack->GetDamageBonusMaxMelee();
				}
#if !GP_RETAIL
				pCombatProbe->Set( MakeInt( Client ), "damage_base_min", min_damage );
				pCombatProbe->Set( MakeInt( Client ), "damage_base_max", max_damage );
#endif
			}
		}
		else if( hWeapon->GetAttack()->GetIsProjectile() )
		{
			//  #####################
			//  ### Ranged damage ###
			//  #####################

			if( hWeapon->GetAttack()->GetProjectileLauncher() != GOID_INVALID )
			{
				// ## Projectile ##
				GoHandle hLauncher( hWeapon->GetAttack()->GetProjectileLauncher() );
				if( hLauncher )
				{
					min_damage = hLauncher->GetAttack()->GetDamageMin() + pAttack->GetDamageBonusMinRanged();
					max_damage = hLauncher->GetAttack()->GetDamageMax() + pAttack->GetDamageBonusMaxRanged();

#if !GP_RETAIL
					pCombatProbe->Set( MakeInt( Client ), "damage_weapon_min", min_damage );
					pCombatProbe->Set( MakeInt( Client ), "damage_weapon_max", max_damage );
#endif
					goto Wussy;
				}
				return false;
			}
			else
			{
				// ## Launcher ( UI querry ) ##
				min_damage = hWeapon->GetAttack()->GetDamageMin() + pAttack->GetDamageBonusMinRanged();
				max_damage = hWeapon->GetAttack()->GetDamageMax() + pAttack->GetDamageBonusMaxRanged();
#if !GP_RETAIL
				pCombatProbe->Set( MakeInt( Client ), "damage_weapon_min", min_damage );
				pCombatProbe->Set( MakeInt( Client ), "damage_weapon_max", max_damage );
#endif
			}
		}
		else if( hWeapon->IsSpell() )
		{
			//  ####################
			//  ### Spell damage ###
			//  ####################

			// attack_damage_modifers were calculated at the time of the spell cloning and written back
			// into the min and max damage.
			
			float min_damage_bonus = 0;
			float max_damage_bonus = 0;

			if( hWeapon->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
			{
				min_damage_bonus = pAttack->GetDamageBonusMinCMagic();
				max_damage_bonus = pAttack->GetDamageBonusMaxCMagic();
			}
			else
			{
				min_damage_bonus = pAttack->GetDamageBonusMinNMagic();
				max_damage_bonus = pAttack->GetDamageBonusMaxNMagic();
			}

			min_damage = hWeapon->GetAttack()->GetDamageMin() + min_damage_bonus;
			max_damage = hWeapon->GetAttack()->GetDamageMax() + max_damage_bonus;

#if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Client ), "damage_spell_min", min_damage );
			pCombatProbe->Set( MakeInt( Client ), "damage_spell_max", max_damage );
#endif
		}
Wussy:	// Make sure we aren't a total wuss and use difficulty level
		min_damage = max_t( 1.0f, min_damage );
		max_damage = max_t( min_damage, max_damage );

		return true;
	}
	return false;
}


bool
Rules::GetDamageRange( Goid Client, Goid Weapon, float &min_damage, float &max_damage )
{
	return GetDamageRange( Client, Weapon, min_damage, max_damage, true );
}


float
Rules::GetDifficultyFactor( Go *pClient )
{
	if( pClient )
	{
		// Determine how difficulty level should be applied
		if( pClient->GetPlayer()->IsComputerPlayer() )
		{
			return m_DifficultyComputer[ gWorldOptions.GetDifficulty() ] ;
		}
		else
		{
			return m_DifficultyPlayer[ gWorldOptions.GetDifficulty() ];
		}
	}
	return 1.0f;
}


void
Rules::DamageGoMelee( Goid Attacker, Goid Weapon, Goid Victim )
{
	CHECK_SERVER_ONLY;

	GoHandle hAttacker( Attacker );
	GoHandle hWeapon( Weapon );
	GoHandle hVictim( Victim );

	// make sure that we are all around before anyone gets any damage ;0
	if( !hAttacker || !hWeapon || !hVictim || hVictim->IsMarkedForDeletion() )
	{
		return;
	}

#	if !GP_RETAIL
		pCombatProbe->Set( MakeInt( Attacker ), "DamageGoMelee", (int)MakeInt( Victim ) );
		pCombatProbe->Increment( MakeInt( Attacker ), gpstringf( "attacking_go_%d", MakeInt( Victim ) ) );
#	endif

	const float total_defense	= GetTotalDefense( Victim );

#	if !GP_RETAIL
		pCombatProbe->Set( MakeInt( Victim ), "defense_total", total_defense );
#	endif

	bool bPiercing = false;
	float min_damage = 0, max_damage = 0;
	if( GetDamageRange( Attacker, Weapon, min_damage, max_damage ) )
	{

#		if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Attacker ), "damage_total_min", min_damage );
			pCombatProbe->Set( MakeInt( Attacker ), "damage_total_max", max_damage );
#		endif

		float damage = 0;
		bool bFatalBlow = false;
		eHitType hit_type = HIT_SOLID;

		if( CheckForCriticalHit( Attacker, Victim ) )
		{
			damage = hVictim->GetAspect()->GetCurrentLife();
			bFatalBlow = true;
			hit_type = HIT_CRITICAL;
		}
		else
		{
			if ( hAttacker->HasAttack() )
			{
				 if ( (hAttacker->GetAttack()->GetPiercingDamageChance() >= Random( 100.0f )) ||
					  (hWeapon->IsMeleeWeapon() && (hAttacker->GetAttack()->GetPiercingDamageChanceMelee() >= Random( 100.0f ))) ||
					  (hWeapon->IsRangedWeapon() && (hAttacker->GetAttack()->GetPiercingDamageChanceRanged() >= Random( 100.0f ))) )
				 {
					 bPiercing = true;
				 }
			}

			damage = max_t( CalculateDamage( Attacker, Weapon, Victim, min_damage, max_damage, total_defense, bPiercing, false ), 1.0f );
			hit_type = DetermineHitType( damage, min_damage, max_damage );

			if ( bPiercing )
			{
				damage += hAttacker->GetAttack()->GetPiercingDamageChanceAmount();

				if ( hWeapon->IsMeleeWeapon() )
				{
					damage += hAttacker->GetAttack()->GetPiercingDamageChanceAmountMelee();
				}
				else if ( hWeapon->IsRangedWeapon() )
				{
					damage += hAttacker->GetAttack()->GetPiercingDamageChanceAmountRanged();
				}
			}
		}

#		if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Victim ), "life_before", hVictim->GetAspect()->GetCurrentLife() );
			float pct_blocked = pCombatProbe->GetFloat( "pct_blocked" );
			float damage_roll = pCombatProbe->GetFloat( "damage_roll" );
			pCombatProbe->Set( MakeInt( Attacker ), "percent_blocked", pct_blocked );
			pCombatProbe->Set( MakeInt( Attacker ), "damage_roll", damage_roll );
			pCombatProbe->Set( MakeInt( Attacker ), "damage_given", damage );
			pCombatProbe->Set( MakeInt( Victim ), "damage_taken", damage );
#		endif

		bool bBlockedHit = false;

		if ( hWeapon->IsMeleeWeapon() )
		{
			if ( hVictim->HasDefend() )
			{
				if ( hVictim->GetDefend()->GetBlockMeleeDamageChance() >= Random( 100.0f ) )
				{
					damage -= hVictim->GetDefend()->GetBlockMeleeDamageChanceAmount();
					if ( hVictim->GetDefend()->GetBlockMeleeDamageChanceAmount() == 0 )
					{
						// if there's no amount set, default to a full block
						bBlockedHit = true;
					}
				}
			}
		}
		else if ( hWeapon->IsRangedWeapon() )
		{
			if ( hVictim->HasDefend() )
			{
				if ( hVictim->GetDefend()->GetBlockRangedDamageChance() >= Random( 100.0f ) )
				{
					damage -= hVictim->GetDefend()->GetBlockRangedDamageChanceAmount();
					if ( hVictim->GetDefend()->GetBlockRangedDamageChanceAmount() == 0 )
					{
						bBlockedHit = true;
					}
				}
			}
		}

		// Apply the damage
		if ( !bBlockedHit )
		{
			if( hAttacker->HasActor() && (hAttacker->GetActor()->GetCanLevelUp() || hAttacker->GetAspect()->GetExperienceBenefactor() != GOID_INVALID ) )
			{
				AwardExperience( Attacker, Weapon, CalculateExperience( Attacker, Weapon, Victim, damage ) );
			}

			GoMagic *pWeaponMagic = hWeapon->QueryMagic();

			// do the hit effect if one exists for this weapon for melee weapons only - ranged happens earlier at the launch
			if( hWeapon->IsMeleeWeapon() && pWeaponMagic && pWeaponMagic->HasEnchantments() )
			{
				gpstring sScriptName, sScriptParams;

				if( pWeaponMagic->GetEnchantmentStorage().GetFirstHitEffectInfo( sScriptName, sScriptParams ) )
				{
					gWorldFx.SRunScript( sScriptName, Victim, Weapon, sScriptParams, Attacker, WE_DAMAGED );
				}
			}

			gpassert( !hVictim->IsMarkedForDeletion() && hVictim != NULL );
			DamageGo( Victim, Attacker, Weapon, damage, bPiercing, false,
					CombatSoundCallback( hit_type, Weapon, Victim, bFatalBlow ) );
		}

#		if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Victim ), "life_after", hVictim->GetAspect()->GetCurrentLife() );
#		endif
	}
}


void
Rules::DamageGoRanged( Goid Attacker, Goid Weapon, Goid Victim )
{
	CHECK_SERVER_ONLY;

	// For the time being this is the same as melee
	DamageGoMelee( Attacker, Weapon, Victim );
}


void
Rules::DamageGoMagic( Goid Attacker, Goid Spell, Goid Victim, SiegePos &damage_origin )
{
	CHECK_SERVER_ONLY;

	GoHandle hSpell( Spell );
	GoHandle hAttacker( Attacker );

	if( !hSpell || !hSpell->HasAttack() || !hAttacker || !hAttacker->HasAttack() )
	{
		return;
	}

#	if !GP_RETAIL
		pCombatProbe->Set( MakeInt( Attacker ), "DamageGoMagic", (int)MakeInt( Victim ) );
		pCombatProbe->Increment( MakeInt( Attacker ), gpstringf( "attacking_go_%d", MakeInt( Victim ) ) );
#	endif

	GoAttack *pSpellAttack = hSpell->GetAttack();
	GoHandle hVictim( Victim );

	if( pSpellAttack->GetAreaDamageRadius() > 0 )
	{
		if( hSpell->HasPhysics() )
		{
			const float radius = pSpellAttack->GetAreaDamageRadius();
			float damage = Random( pSpellAttack->GetDamageMin(), pSpellAttack->GetDamageMax() );

			const float magnitude = CalculateMagnitude( hSpell->GetPhysics()->GetExplosionMagnitude(), damage );

#if !GP_RETAIL
				pCombatProbe->Set( MakeInt( Attacker ), "explosive_damage", damage );
				pCombatProbe->Set( MakeInt( Attacker ), "explosive_damage_radius", radius );
#endif

			SimID explosion = gSim.CreateExplosion( damage_origin, radius, magnitude, damage, Attacker, Spell );
			gSim.SetIgnorePartyMembers( explosion );
		}
	}

	if( hVictim && !hVictim->IsMarkedForDeletion())
	{
		float min_damage = 0;
		float max_damage = 0;
		GetDamageRange( Attacker, Spell, min_damage, max_damage );

		bool bPiercing = false;

		if( hAttacker->GetAttack()->GetPiercingDamageChance() >= Random( 100.0f ) )
		{
			bPiercing = true;
		}

		const float defense = GetTotalDefense( Victim );

		float damage = max_t( CalculateDamage( Attacker, Spell, Victim, min_damage, max_damage, defense, bPiercing, false ), 1.0f );

#if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Attacker ), "victim_defense", defense );
			pCombatProbe->Set( MakeInt( Attacker ), "damage_before_blocking", damage );
#endif

		bool bBlockedHit = false;

		if ( hVictim->HasDefend() )
		{
			if ( hSpell->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
			{
				if ( (hVictim->GetDefend()->GetBlockCombatMagicChance() >= Random( 100.0f )) )
				{
					damage -= hVictim->GetDefend()->GetBlockCombatMagicChanceAmount();
					if ( hVictim->GetDefend()->GetBlockCombatMagicChanceAmount() == 0.0f )
					{
						// if there's no amount set, default to a full block
						bBlockedHit = true;
					}
				}
			}
			else if ( hSpell->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
			{
				if ( (hVictim->GetDefend()->GetBlockNatureMagicChance() >= Random( 100.0f )) )
				{
					damage -= hVictim->GetDefend()->GetBlockNatureMagicChanceAmount();
					if ( hVictim->GetDefend()->GetBlockNatureMagicChanceAmount() == 0.0f )
					{
						bBlockedHit = true;
					}
				}
			}
		}

		if ( !bBlockedHit )
		{
			if ( hVictim->HasDefend() )
			{
				if ( hSpell->GetMagic()->GetMagicClass() == MC_COMBAT_MAGIC )
				{
					damage -= (hVictim->GetDefend()->GetBlockCombatMagicDamage() / 100.0f) * damage;
					damage -= hVictim->GetDefend()->GetBlockPartCombatMagicDamage();
				}
				else if ( hSpell->GetMagic()->GetMagicClass() == MC_NATURE_MAGIC )
				{
					damage -= (hVictim->GetDefend()->GetBlockNatureMagicDamage() / 100.0f) * damage;
					damage -= hVictim->GetDefend()->GetBlockPartNatureMagicDamage();
				}
			}

			const float experience = CalculateExperience( Attacker, Spell, Victim, damage );

			if( hAttacker->HasActor() && (hAttacker->GetActor()->GetCanLevelUp() || hAttacker->GetAspect()->GetExperienceBenefactor() != GOID_INVALID ) )
			{
				AwardExperience( Attacker, Spell, experience );
			}

#if !GP_RETAIL
			pCombatProbe->Set( MakeInt( Attacker ), "awarded_experience", experience );
	
			if( bPiercing )
			{
				pCombatProbe->Set( MakeInt( Attacker ), "piercing_damage", damage );
			}
			else
			{
				pCombatProbe->Set( MakeInt( Attacker ), "magic_damage", damage );
			}
#endif

			// Apply the damage
			eHitType hit_type = DetermineHitType( damage, min_damage, max_damage );
			DamageGo( Victim, Attacker, Spell, damage, bPiercing, false,
					CombatSoundCallback( hit_type, Spell, Victim ) );
		}
	}
}


bool
Rules::DamageGoTrap( Goid Victim, Goid Attacker, Goid AttackerWeapon, float min_damage, float max_damage )
{
	CHECK_SERVER_ONLY;

	// Ignore invalid goes
	if( Victim == GOID_INVALID )
	{
		return false;
	}

	GoHandle hVictim( Victim );

	// Ignore if without visual component or if go is invincible
	if( hVictim && hVictim->HasAspect() && !hVictim->IsMarkedForDeletion())
	{
		if( !hVictim->GetAspect()->GetIsInvincible() )
		{
			const float total_defense = GetTotalDefense( Victim );

			// Calculate simple hit damage amount
			float damage = CalculateDamage( Attacker, AttackerWeapon, Victim, min_damage, max_damage, total_defense, false, true );

			WorldMessage ( WE_DAMAGED, Attacker, Victim, MakeInt(Attacker) ).Send();

			eHitType hit_type = DetermineHitType( damage, min_damage, max_damage );
			const bool bFatality = DamageGo( Victim, Attacker, AttackerWeapon, damage, false, false,
					CombatSoundCallback( hit_type, AttackerWeapon, Victim ) );

			return bFatality;
		}
	}
	return false;
}


bool
Rules::DamageGoParticle( Goid Victim, Goid Attacker, Goid AttackerWeapon, float min_damage, float max_damage, float exposure_time, bool award_xp, bool bIgnite, bool bCalcFireResist )
{
	CHECK_SERVER_ONLY;

	// Ignore invalid goes
	if( (Victim == GOID_INVALID) || (exposure_time <= 0) || (Attacker == Victim) )
	{
		return false;
	}

	GoHandle hVictim( Victim );

	// Ignore if without visual component or if go is invincible
	if( hVictim && hVictim->HasAspect() || !hVictim->GetAspect()->GetIsInvincible() || hVictim->IsMarkedForDeletion() )
	{
		float damage = CalculateDamage( Attacker, AttackerWeapon, Victim, min_damage, max_damage, GetTotalDefense( Victim ), false, true );

		if( (exposure_time > 0) && bCalcFireResist )
		{
			// Get a random time correct damage amount using fire resistance
			const float fire_inf = exposure_time * (1.0f - GetTotalFireResistance( Victim ));

			// adjust min/max ranges based on fire resistances
			damage *= fire_inf;
		}
		else
		{
			damage *= exposure_time;
		}

		// Check the burn threshold and catch fire if beyond it
		if( bIgnite && ( damage > hVictim->GetPhysics()->GetFireBurnThreshold() ) )
		{
			hVictim->GetPhysics()->SetIsOnFire( true, Attacker );
		}

		float experience = 0;

		// Award experience if specified to do so
		if( award_xp )
		{
			experience = CalculateExperience( Attacker, AttackerWeapon, Victim, damage );
			AwardExperience( Attacker, AttackerWeapon, experience );
		}

#if !GP_RETAIL
		pCombatProbe->Set( MakeInt( Attacker ), "particle_damage", damage );
		pCombatProbe->Set( MakeInt( Attacker ), "particle_experience_awarded", experience );

		if( m_bPDCEnabled )
		{
			if( m_bPDCReset || (m_PDCTotalT >= m_PDCResetPeriod) )
			{
				m_bPDCReset = false;
				m_PDCTotalT	= exposure_time;
				m_PDCTotalD = damage;
				gpgeneric("-------------------------------------------------------------------------\n");
			}
			else
			{
				m_PDCTotalT += exposure_time;
				m_PDCTotalD += damage;
			}

			gpgenericf(("t:%g dmg:%g   total_t:%g total_dmg:%g\n", exposure_time, damage, m_PDCTotalT, m_PDCTotalD));
		}
#endif
		eHitType hit_type = DetermineHitType( damage, min_damage, max_damage );
		// damage go will take care of playing the combat sound for us!!! yey!

		CombatSoundCallback cscb( hit_type, AttackerWeapon, Victim );
		cscb.m_bPlayOnFatalOnly = true;

		const bool bFatality = DamageGo( Victim, Attacker, AttackerWeapon, damage, false, false, cscb );
		
		return bFatality;
	}
	return false;
}


bool
Rules::DamageGoVolume( Goid Victim, Goid Attacker, Goid AttackerWeapon, float damage, float delta_t, bool explosive_damage )
{
	const float victim_defense	= GetTotalDefense( Victim );
	const float actual_damage	= delta_t * CalculateDamage( Attacker, AttackerWeapon, Victim, damage, damage, victim_defense, false, true );

	GoHandle hOwner	( Attacker );
	GoHandle hVictim( Victim );

	bool bAwardExperience = true;

	// If the owner is valid and is an actor then he should get some experience from the damage dealt
	if( hOwner && hOwner->HasActor() )
	{
		// Filter out party members - if damage is done to them then don't award experience
		if( hOwner->HasParent() && hVictim->HasParent() )
		{
			Go *pOwnerParty;
			if( hOwner->GetParent( pOwnerParty ) )
			{
				// If the pointers are equal then they are members of the same party - so give no xp
				if( pOwnerParty == hVictim->GetParent() )
				{
					bAwardExperience = false;
				}
			}
		}
	}

	if( bAwardExperience )
	{
		// Accumulate experience points to award
		const float experience_points = CalculateExperience( Attacker, AttackerWeapon, Victim, actual_damage );

		// Award XP
		AwardExperience( Attacker, AttackerWeapon, experience_points );
	}

	// Damage the go 
	return DamageGo( Victim, Attacker, AttackerWeapon, actual_damage, false, explosive_damage, NULL );
}


bool
Rules::DamageGo( Goid Victim, Goid Attacker, Goid AttackerWeapon, float amount, bool piercing_damage, bool explosive_damage, CombatSoundCallback* cb )
{
	CHECK_SERVER_ONLY;

	GoHandle hVictim( Victim );
	GoHandle hAttacker( Attacker );
	GoHandle hWeapon( AttackerWeapon );

	// make sure that the victim is still around
	if( !hVictim || !hVictim->HasAspect() || hVictim->IsMarkedForDeletion() || ( hVictim->HasPhysics() && hVictim->GetPhysics()->GetHasExploded() ) )
	{
		return false;
	}

	GoAspect *pAspectVictim = hVictim->GetAspect();

	eLifeState life_state = hVictim->GetLifeState();
	if( (life_state == LS_GONE) || (life_state == LS_IGNORE) )
	{
		return false;
	}

	bool bWasKilled = false;

	eLifeState victimLifeStateBefore = hVictim->GetAspect()->GetLifeState();

	if( !(pAspectVictim->GetIsInvincible() || ( hVictim->HasDefend() && ( amount < hVictim->GetDefend()->GetDamageThreshold() ) )) )
	{
		ChangeLife( Victim, -amount, RPC_TO_ALL );
		life_state = hVictim->GetLifeState();
		bWasKilled = IsAlive( victimLifeStateBefore ) && IsDead( life_state );
	}

	DWORD ShouldGib = 0;

	if( IsDead( life_state ) && hVictim->HasPhysics() )
	{
		GoPhysics *pVictimPhysics = hVictim->GetPhysics();

		const bool caused_gib = ( (amount > pVictimPhysics->GetGibMinDamage()) && (amount > (pVictimPhysics->GetGibThreshold()*pAspectVictim->GetMaxLife())) );

		if ( caused_gib && hAttacker && hAttacker->IsAnyHumanPartyMember() )
		{
			gVictory.SIncrementStat( GS_GIBS, hAttacker->GetPlayer() );
		}

		if( gWorldOptions.GetAlwaysGib() || caused_gib || pVictimPhysics->GetExplodeWhenKilled() || ( !hVictim->IsActor() && pVictimPhysics->GetIsBreakable() ) )
		{
			ShouldGib = 1;

			const float magnitude = pVictimPhysics->GetExplosionMagnitude();

			// have to play the sound NOW before we explode the go, which causes
			// it to get deleted. (#10417, #9035).
			if ( cb != NULL )
			{
				(*cb)( true );
			}
			gSim.SExplodeGo( Victim, CalculateMagnitude( magnitude, amount ) );

			if( hVictim->HasAttack() )
			{
				GoAttack *pVictimAttack = hVictim->GetAttack();
				const float damage_radius = pVictimAttack->GetAreaDamageRadius();

				if( damage_radius > 0 )
				{
					float damage = Random( pVictimAttack->GetDamageMin(), pVictimAttack->GetDamageMax() );

					SimID explosion = gSim.CreateExplosion( hVictim->GetPlacement()->GetPosition(), damage_radius, magnitude, damage, Attacker, AttackerWeapon );

					const bool bDamageAll = pVictimPhysics->GetDamageAll();

					gSim.SetDamageAll( explosion, bDamageAll );

					if( !bDamageAll )
					{
						gSim.SetIgnorePartyMembers( explosion );
					}
				}
			}
		}
	}

	// Now let the attacker know what happened

	if( hAttacker )
	{
		const bool isMeleeWeapon = hWeapon && hWeapon->IsMeleeWeapon();

		// do life/mana steal for attacker
		if ( isMeleeWeapon )
		{
			if ( (hAttacker->GetAttack()->GetLifeStealAmount() > 0) ||
				 (hAttacker->GetAttack()->GetLifeBonusAmount() > 0) )
			{
				float NewLife = hAttacker->GetAspect()->GetCurrentLife() + hAttacker->GetAttack()->GetLifeStealAmount() + min_t( hAttacker->GetAttack()->GetLifeBonusAmount(), amount );

				if ( NewLife > hAttacker->GetAspect()->GetMaxLife() )
				{
					NewLife = hAttacker->GetAspect()->GetMaxLife();
				}

				hAttacker->GetAspect()->SSetCurrentLife( NewLife );
			}

			if ( (hAttacker->GetAttack()->GetManaStealAmount() > 0) ||
				 (hAttacker->GetAttack()->GetManaBonusAmount() > 0) )
			{
				float NewMana = hAttacker->GetAspect()->GetCurrentMana() + hAttacker->GetAttack()->GetManaStealAmount() + min_t( hAttacker->GetAttack()->GetManaBonusAmount(), amount );

				if ( NewMana > hAttacker->GetAspect()->GetMaxMana() )
				{
					NewMana = hAttacker->GetAspect()->GetMaxMana();
				}

				hAttacker->GetAspect()->SSetCurrentMana( NewMana );
			}
		}

		// calculate reflected damage to attacker
		if ( hVictim->HasDefend() && (AttackerWeapon != GOID_INVALID) && ( isMeleeWeapon || ( AttackerWeapon == Attacker ) ) )
		{
			float reflectedDamage = 0.0f;
			float reflectedDamageChance = 100.0f;
			float reflectedPiercingDamgeChance = 100.0f;
			
			if( hVictim->GetDefend()->GetReflectDamageChance() > 0 )
			{
				reflectedDamageChance = hVictim->GetDefend()->GetReflectDamageChance();
			}

			if( hVictim->GetDefend()->GetReflectPiercingDamageChance() > 0 )
			{
				reflectedPiercingDamgeChance = hVictim->GetDefend()->GetReflectPiercingDamageChance();
			}

			if ( (hVictim->GetDefend()->GetReflectDamageAmount() > 0) &&
				 (reflectedDamageChance >= Random( 100.0f )) )
			{
				reflectedDamage = (hVictim->GetDefend()->GetReflectDamageAmount() / 100.0f) * amount;
			}
			else if ( (hVictim->GetDefend()->GetReflectPiercingDamageAmount() > 0) &&
				      (reflectedPiercingDamgeChance >= Random( 100.0f )) )
			{
				reflectedDamage = (hVictim->GetDefend()->GetReflectPiercingDamageAmount() / 100.0f) * (piercing_damage ? amount : piercing_damage );
			}

			if ( reflectedDamage > 0.0f )
			{
				Go * pWeaponVictim = ( hVictim->HasInventory() ) ? hVictim->GetInventory()->GetSelectedItem() : NULL;
				Goid Weapon = pWeaponVictim? pWeaponVictim->GetGoid() : Victim;

				if( hVictim->HasActor() && (hVictim->GetActor()->GetCanLevelUp() || hVictim->GetAspect()->GetExperienceBenefactor() != GOID_INVALID ) )
				{
					AwardExperience( Victim, Weapon, CalculateExperience( Victim, Weapon, Attacker, reflectedDamage ) );
				}
				
				DamageGo( Attacker, Victim, GOID_INVALID, reflectedDamage, piercing_damage, explosive_damage );
			}
		}

		if( bWasKilled )
		{
			// Audit damage, death, and destruction
			if ( hVictim->HasActor() && !hAttacker->GetPlayer()->IsComputerPlayer() )
			{
				gGameAuditor.Increment( GE_PLAYER_KILL, MakeInt( hAttacker->GetPlayerId() ), hVictim->GetTemplateName() );

				if ( hVictim->IsAnyHumanPartyMember() )
				{
					gVictory.SIncrementStat( GS_HUMAN_KILLS, hAttacker->GetPlayer() );

					if ( ::IsMultiPlayer() )
					{
						gVictory.SLeaderMessage( GS_HUMAN_KILLS, "leader_human_kills" );
					}
				}
				else
				{
					gVictory.SIncrementStat( GS_KILLS, hAttacker->GetPlayer() );

					if ( ::IsMultiPlayer() )
					{
						gVictory.SLeaderMessage( GS_KILLS, "leader_kills" );
					}
				}
			}

			// update chance to hit bonus for attacker
			int killsVersesVictim = 0;
			gGameAuditor.Get( GE_PLAYER_KILL, MakeInt( hAttacker->GetPlayerId() ), hVictim->GetTemplateName(), killsVersesVictim );

			if ( hAttacker->HasActor() && hVictim->HasActor()
				&& hAttacker->GetActor()->GetCanLevelUp()
				&& !hVictim->GetActor()->GetCanLevelUp()
				&& (killsVersesVictim >= hVictim->GetActor()->GetChanceToHitBonusStart() )
				&& ((killsVersesVictim - hVictim->GetActor()->GetChanceToHitBonusStart()) % hVictim->GetActor()->GetChanceToHitBonusKillStep() == 0) )
			{
				gpstring chanceToHitKey( "chance_to_hit_bonus_versus_" );
				chanceToHitKey.append( hVictim->GetTemplateName() );

				float chanceToHitBonus = 0.0f;
				gGameAuditor.Get( MakeInt( Attacker ), chanceToHitKey, chanceToHitBonus );

				chanceToHitBonus += hVictim->GetActor()->GetChanceToHitBonusIncrement();
				if ( chanceToHitBonus > hVictim->GetActor()->GetChanceToHitBonusMax() )
				{
					chanceToHitBonus = hVictim->GetActor()->GetChanceToHitBonusMax();
				}
				gGameAuditor.Set( MakeInt( Attacker ), chanceToHitKey, chanceToHitBonus );
			}

			// tell the attacker he knocked out his engaged object
			if( hAttacker->HasMind() && hAttacker->GetMind()->GetEngagedObject() == Victim )
			{
				WorldMessage( WE_ENGAGED_HIT_KILLED, GOID_INVALID, Attacker, MakeInt(Victim) ).Send();
			}
			// now tell everyone else who may be interacting with victim
			if( hVictim->HasMind() )
			{
				WorldMessage msgEngaged( WE_ENGAGED_KILLED );
				hVictim->GetMind()->MessageEngagedMe( msgEngaged, Attacker );
			}
		}
		else if( ( victimLifeStateBefore == LS_ALIVE_CONSCIOUS ) && LS_ALIVE_UNCONSCIOUS == hVictim->GetAspect()->GetLifeState() )
		{
			// tell everyone else who may be interacting with victim
			if( hVictim->HasMind() )
			{
				WorldMessage msgEngaged( WE_ENGAGED_LOST_CONSCIOUSNESS );
				hVictim->GetMind()->MessageEngagedMe( msgEngaged );
			}
		}
	}

	if( amount > 0 )
	{
		WorldMessage ( WE_DAMAGED, Attacker, Victim, MakeInt(Attacker) ).Send();
	}

	if ( cb != NULL )
	{
		cb->m_bFatalBlow = bWasKilled;
	}

	// now get rid of the go.  do this at the end so we aren't passin around 
	// a victim that is deleted !
	if( bWasKilled )
	{
		if ( hVictim->IsAnyHumanPartyMember() )
		{
			gVictory.SIncrementStat( GS_DEATHS, hVictim->GetPlayer() );
		}

		// Send a message telling the victim he has been killed
		// but only send the killed message if we aren't already killed ! ;)
		if( !hVictim->IsMarkedForDeletion() && !hVictim->IsDelayedMpDeletion())
		{
			// have to play the sound NOW before we explode the go, which causes
			// it to get deleted. (#10417, #9035).
			if ( cb != NULL )
			{
				(*cb)( true );
			}
			WorldMessage( WE_KILLED, Attacker, Victim, ShouldGib ).Send();
		}
	}

	return bWasKilled;
}


double
Rules::GetNextLevelXP( float current_level ) const
{
	// do not accept any negative levels
	if( current_level < 0 )
	{
		gperror( "Rules::GetNextLevelXP - (current_level < 0)\n" );
		return 0;
	}

	// Calculate amount needed for next level
	if( current_level < GetMaxLevel() )
	{
		return m_ExperienceTable[ UINT32( current_level+1 ) ];
	}
	return m_ExperienceTable[ UINT32( GetMaxLevel() ) ];
}


double
Rules::LevelToXP( float level ) const
{
	// do not accept any negative levels
	if( level < 0 )
	{
		gperror( "Rules::LevelToXP - (level < 0)\n" );
		return 0;
	}

	const double starting_level = (level > GetMaxLevel()) ? GetMaxLevel()-1 : floor(level);

	const UINT32 next_level = ( starting_level < GetMaxLevel() ) ? UINT32(starting_level+1):UINT32(starting_level);

	const double start = m_ExperienceTable[ UINT32( starting_level ) ];
	const double end   = m_ExperienceTable[ UINT32( next_level ) ];

	double partial = level - starting_level;

	return ( start + partial * (end - start) );
}


float
Rules::XPToLevel( double xp ) const
{
	// no such thing as negative xp
	if( xp < 0 )
	{
		gperror( "Rules::XPToLevel - (xp < 0)\n" );
		return 0;
	}

	UINT32 level = 0;
	const UINT32 max_level = UINT32( GetMaxLevel() );

	for( ; ( (level < max_level ) && ( xp > m_ExperienceTable[level+1]) ); ++level );

	if( level < max_level )
	{
		const double start = m_ExperienceTable[ UINT32( level ) ];
		const double end   = m_ExperienceTable[ UINT32( level+1 ) ];

		return float( (1.0f + 10000.0f * (level + ((xp-start)/(end-start))))/10000.0f );
	}

	return ( float((1.0f + 10000.0f * level)/10000.0f) );
}


float
Rules::CalculateExperience( Goid Attacker, Goid Weapon, Goid Victim, float damage )
{
	if( damage < 0 )
	{
		gperror( "Rules::CalculateExperience - (damage < 0)\n" );
		return 0;
	}

	GoHandle hPhysicalAttacker( Attacker );
	GoHandle hWeapon( Weapon );
	GoHandle hVictim( Victim );

	if( !(hPhysicalAttacker && hWeapon && hVictim && hPhysicalAttacker->HasActor() ) || ( hVictim && !hVictim->HasAspect() ) )
	{
		return 0;
	}

	GoAspect *pVictimAspect = hVictim->GetAspect();
	const float curLife = pVictimAspect->GetCurrentLife();

	// Don't award experience from something that is already dead
	if( curLife <= 0 )
	{
		return 0;
	}

	float maxLife = pVictimAspect->GetMaxLife();

	maxLife = ( maxLife < 1.0f ) ? 1.0f : maxLife;
	damage	= ( damage > curLife ) ? curLife : damage;

	// Experience points are a finite resource, a monster is only capable of giving it out once.
	float xp_left = pVictimAspect->GetExperienceRemaining();
	float xp_calc = pVictimAspect->GetExperienceValue() * damage/maxLife;


	// Determine who should get the experience points from what the benefactor is set to
	Goid Benefactor = Attacker;
	gpstring sBenefactorSkill;

	// Check for the benefactor stuff if there is one
	if( hPhysicalAttacker && hPhysicalAttacker->HasAspect() )
	{
		GoAspect *pAttackerAspect = hPhysicalAttacker->GetAspect();

		Benefactor		= pAttackerAspect->GetExperienceBenefactor();
		Benefactor		= (Benefactor == GOID_INVALID) ? Attacker : Benefactor;
		sBenefactorSkill= pAttackerAspect->GetExperienceBenefactorSkill();
	}

	GoHandle hAttacker( Benefactor );

	if( !hAttacker )
	{
		return 0;
	}

	// get the skill so we can figure out the max percent
	float current_level = 0;

	Skill *pSkill = NULL;

	if( hWeapon->IsSpell() )
	{
		hAttacker->GetActor()->GetSkill( hWeapon->GetMagic()->GetSkillClass(), &pSkill );
	}
	else if( hWeapon->HasAttack() )
	{
		hAttacker->GetActor()->GetSkill( hWeapon->GetAttack()->GetSkillClass(), &pSkill );
	}

	if( pSkill )
	{
		current_level = floorf( pSkill->GetNaturalLevel() );
	}
	else
	{
		gperrorf(("Weapon: [%s] is specifying unknown skill\n", hWeapon->GetTemplateName()));
		return 0;
	}

	const float ELF = (current_level < 1.0f)? m_ELF_first : m_ELF_later;
	const double xp_diff = LevelToXP( current_level + 1 ) - LevelToXP( current_level );

	// try and give all the remaining xp
	if( (xp_left - xp_calc) < 0 )
	{
		pVictimAspect->SetExperienceRemaining( 0 );

		const float percent_award = (float)(xp_left / xp_diff);

		if( percent_award > ELF )
		{
			xp_left = (float)(ELF * xp_diff);
		}

		return ( xp_left );
	}

	// award calculated amount of xp
	pVictimAspect->SetExperienceRemaining( xp_left - xp_calc );

	const float percent_award = (float)(xp_calc / xp_diff);

	if( percent_award > ELF )
	{
		xp_calc = (float)(ELF * xp_diff);
	}
	return ( xp_calc );
}


void 
Rules :: RCPlayLevelUpSoundAndText( Goid Client, const char *sSkillName )
{
	FUBI_RPC_CALL( RCPlayLevelUpSoundAndText, RPC_TO_ALL );

	GoHandle hClient( Client );
	GoActor *pActorClient = hClient->QueryActor();

	if( !pActorClient )
	{
		return;
	}

	// make sure that all of our enchanements are applied before accessing the 
	// skill level on client machines.  since we recieve this call over the network,
	// it has the posiblility of comming in before the modifiers for the enchantments
	// have been applied.
	if( IsServerRemote() )
	{
		hClient->CheckModifierRecalc();
	}
	
	Skill *pPrimarySkill = NULL;

	if( !pActorClient->GetSkill( sSkillName, &pPrimarySkill ) )
	{
		gperrorf(("Rules:RCLevelUpSoundAndText cannot find Primary Skill %s for %s\n",sSkillName, hClient->GetTemplateName()));
		return;
	}

	// Print a message to the screen that says what happened with this level increase
	gpwstring sName = hClient->GetCommon()->GetScreenName();

	UINT32  level = (UINT32)floorf( pPrimarySkill->GetNaturalLevel() );

	if ( pPrimarySkill->GetName().same_no_case( "melee" ) ||
				 pPrimarySkill->GetName().same_no_case( "ranged" ) )
	{
		if ( level <= MAX_LEVELS_FOR_FULL_LEVELUP_DISPLAY )
		{
			gpscreenf(( $MSG$ "%s has advanced to level %d in %S skill by fighting with %S weapons!", 
						::ToAnsi( sName ).c_str(),
						level,
						pPrimarySkill->GetScreenName().c_str(),
						pPrimarySkill->GetScreenName().c_str() ));
		}
		else
		{
			gpscreenf(( $MSG$ "%s has advanced to level %d in %S skill!", 
						::ToAnsi( sName ).c_str(),
						level,
						pPrimarySkill->GetScreenName().c_str(),
						pPrimarySkill->GetScreenName().c_str() ));
		}
	}
	else if ( pPrimarySkill->GetName().same_no_case( "nature magic" ) ||
			  pPrimarySkill->GetName().same_no_case( "combat magic" ) )
	{
		if ( level <= MAX_LEVELS_FOR_FULL_LEVELUP_DISPLAY )
		{
			gpscreenf(( $MSG$ "%s has advanced to level %d in %S skill by casting %S spells!", 
						::ToAnsi( sName ).c_str(),
						level,
						pPrimarySkill->GetScreenName().c_str(),
						pPrimarySkill->GetScreenName().c_str() ));
		}
		else
		{
			gpscreenf(( $MSG$ "%s has advanced to level %d in %S skill!", 
						::ToAnsi( sName ).c_str(),
						level,
						pPrimarySkill->GetScreenName().c_str(),
						pPrimarySkill->GetScreenName().c_str() ));

		}
	}

	gVictory.TrackPlayerExperience( MakeInt( hClient->GetPlayer()->GetId() ), pPrimarySkill->GetName(), 1 );


	gpstring sSound = pPrimarySkill->GetName();
	sSound.append( "_up" );

	hClient->PlayVoiceSound( sSound );	

	// Set inventory as dirty to update highlights
	hClient->GetInventory()->SetInventoryDirty();					
}


float
Rules :: GetDifficultyScaleLife( int player_count )
{
	player_count -= 1;
	player_count = max_t( player_count, 0 );
	player_count = min_t( player_count, int(m_MPScaleLife.size()-1) );

	return ( m_MPScaleLife[ player_count ] );
}


float
Rules:: GetDifficultyScaleEP( int player_count )
{
	player_count -= 1;
	player_count = max_t( player_count, 0 );
	player_count = min_t( player_count, int(m_MPScaleXP.size()-1) );

	return ( m_MPScaleXP[ player_count ] );
}

void
Rules::AwardExperience( Goid Client, Goid Weapon, double experience_points )
{
	CHECK_SERVER_ONLY;

	if( m_RuleFunctions != NULL )
	{
		::OnAwardExperience( m_RuleFunctions, "on_award_experience$", Client, Weapon, (float) experience_points );
	}
}

void
Rules::AwardExperienceSingle( Goid Client, Goid Weapon, double experience_points )
{
	CHECK_SERVER_ONLY;

	gpassert( experience_points >= 0 );

	if( experience_points <= 0 )
	{
		return;
	}

	GoHandle hPreClient( Client );
	GoHandle hWeapon( Weapon );

	// Determine who should get the experience points from what the benefactor is set to
	Goid Benefactor = Client;
	gpstring sBenefactorSkill;

	// Check for the benefactor stuff if there is one
	if( hPreClient && hPreClient->HasAspect() )
	{
		GoAspect *pClientAspect = hPreClient->GetAspect();

		Benefactor		= pClientAspect->GetExperienceBenefactor();
		Benefactor		= (Benefactor == GOID_INVALID)?Client:Benefactor;
		sBenefactorSkill= pClientAspect->GetExperienceBenefactorSkill();
	}

	GoHandle hClient( Benefactor );

	if( !hClient )
	{
		return;
	}

	GoActor *pActorClient = hClient->QueryActor();

	if( !pActorClient || !pActorClient->GetCanLevelUp() || hClient->GetInventory()->IsPackOnly() || !hClient->IsAnyHumanPartyMember() )
	{
		return;
	}

#if !GP_RETAIL
	pCombatProbe->Set( MakeInt( Client ), "experience_gained", experience_points );
#endif

	if( pActorClient->GetCanLevelUp() && hWeapon && (experience_points > 0)  )
	{
		Skill *pPrimarySkill = NULL;

		gpstring sSkillName;

		if( Benefactor == Client )
		{
			if( hWeapon->HasAttack() )
			{
				sSkillName = hWeapon->GetAttack()->GetSkillClass();
			}

			if( sSkillName.empty() && hWeapon->HasMagic() )
			{
				sSkillName = hWeapon->GetMagic()->GetSkillClass();
			}
		}
		else
		{
			sSkillName = sBenefactorSkill;
		}

		if( sSkillName.empty() )
		{
			gperrorf(("No skill class defined for weapon [%s]\n", hWeapon->GetTemplateName() ));
		}

		// apply experience bonus, if any
		if ( hClient->HasAttack() )
		{
			experience_points += (hClient->GetAttack()->GetExperienceBonus() / 100.0f) * experience_points;
		}

		// Award primary skill experience
		if( !pActorClient->GetSkill( sSkillName, &pPrimarySkill ) )
		{
			gperrorf(("Rules:AwardExperience cannot award experience to %s for skill %s\n",hClient->GetTemplateName(),sSkillName.c_str()));
			return;
		}

		// Award experience to primary skill
		const float old_level = floorf(pPrimarySkill->GetNaturalLevel());

		RCAwardPrimaryExperience( Benefactor, sSkillName, experience_points, ( (Client == Weapon) && (Client == Benefactor) ) );

		const float levels_increased = floorf(pPrimarySkill->GetNaturalLevel()) - old_level;

		if( levels_increased >= 1.0f )
		{
			GoAspect *pAspect = hClient->GetAspect();

			// Run the appropriate level up script
			LevelUpColl::iterator iIndicator	 = m_LevelUpIndicators.begin();
			LevelUpColl::iterator iIndicator_end = m_LevelUpIndicators.end();

			const float new_level = floorf(pPrimarySkill->GetLevel());

			for(; iIndicator != iIndicator_end; ++iIndicator )
			{
				if( (*iIndicator).m_sSkillName.same_no_case( pPrimarySkill->GetName() ) )
				{
					if( (new_level >= (*iIndicator).m_Range_min) && (new_level <= (*iIndicator).m_Range_max) )
					{
						gWorldFx.SRunScript( (*iIndicator).m_sScript_name.c_str(), Benefactor, Benefactor, (*iIndicator).m_sScript_params.c_str(), Benefactor, WE_LEVELED_UP );
						break;
					}
				}
			}

			// Print a message to the screen that says what happened with this level increase
			RCPlayLevelUpSoundAndText( Benefactor, sSkillName );
 
			// Update all the clients with our new max life/mana now that the level has changed
			pAspect->SSetNaturalMaxLife( pAspect->GetNaturalMaxLife() );
			pAspect->SSetNaturalMaxMana( pAspect->GetNaturalMaxMana() );

			// Reward the recipient with maxed out mana and life for increasing a skill level
			ChangeLife( Benefactor, pAspect->GetMaxLife()-pAspect->GetCurrentLife() );			
			ChangeMana( Benefactor, pAspect->GetMaxMana()-pAspect->GetCurrentMana() );			

			// Set the stats of any pack entities according to overall power of the party
			SetPackMemberStats( Benefactor );
		}
	}
}


void
Rules::RCAwardPrimaryExperience( Goid client, const char *sSkillName, double experience_points, bool bInnateOnly )
{
	FUBI_RPC_CALL( RCAwardPrimaryExperience, RPC_TO_ALL );

	gpassert( experience_points >= 0 );

	if( experience_points < 0 )
	{
		return;
	}

	GoHandle hClient( client );

	if( hClient && hClient->HasActor() )
	{
		GoActor* pActorClient = hClient->GetActor();
		Skill *pPrimarySkill = NULL, *pUberSkill = NULL;

		if( pActorClient->GetSkill( sSkillName, &pPrimarySkill ) && pActorClient->GetSkill( "Uber", &pUberSkill ) )
		{
			const double old_experience = pUberSkill->GetExperience();

			// Award UBER Experience
			pUberSkill->AwardExperience( experience_points );

			if ( client == hClient->GetPlayer()->GetHero() )
			{
				hClient->GetPlayer()->SetHeroUberLevel( pUberSkill->GetLevel() );
			}

			if( !bInnateOnly )
			{
				// Award experience to -> Melee, Ragned, Nature Magic, Combat Magic
				pPrimarySkill->AwardExperience( experience_points );
			}

			// Now determine what level strength, dexterity, intelligence should be at

			double experience_difference = 0;

			const float	 uber_level	= pUberSkill->GetLevelNoBias();

			// handle special case when level totals exceed table size
			if( (old_experience + experience_points) > LevelToXP( GetMaxLevel() ) )
			{
				Skill *pSkill = NULL;
				double total_level_xp = 0;

				if( pActorClient->GetSkill( "melee",		&pSkill ) )
				{
					total_level_xp += pSkill->GetExperience();
				}
				if( pActorClient->GetSkill( "ranged",		&pSkill ) )
				{
					total_level_xp += pSkill->GetExperience();
				}
				if( pActorClient->GetSkill( "nature magic", &pSkill ) )
				{
					total_level_xp += pSkill->GetExperience();
				}
				if( pActorClient->GetSkill( "combat magic", &pSkill ) )
				{
					total_level_xp += pSkill->GetExperience();
				}

				experience_difference = total_level_xp * 1.225f;

			}
			else
			{
				const double uber_current_xp= LevelToXP( floorf( uber_level ) );
				const double uber_next_xp	= GetNextLevelXP( uber_level );

				experience_difference = uber_next_xp - uber_current_xp;
			}

			const float uber_factor		= float( experience_points / experience_difference );

			const float dStr = uber_factor * pPrimarySkill->GetStrInfluence();
			const float dInt = uber_factor * pPrimarySkill->GetIntInfluence();
			const float dDex = uber_factor * pPrimarySkill->GetDexInfluence();

			float initialStr = 0.0f;
			float initialDex = 0.0f;
			float initialInt = 0.0f;
			bool  bUpdateInv = false;

			Skill *pBasicSkill = NULL;
			if( pActorClient->GetSkill( "strength", &pBasicSkill ) )
			{
				initialStr = pBasicSkill->GetNaturalLevel();
				pBasicSkill->SetNaturalLevel( dStr + pBasicSkill->GetNaturalLevel() );
				if ( floorf(initialStr) != floorf(pBasicSkill->GetNaturalLevel()) )
				{
					bUpdateInv = true;
					gpscreenf(( $MSG$	"%s has just increased in Strength!", 
										::ToAnsi( hClient->GetCommon()->GetScreenName().c_str() ).c_str() ));
				}
			}
			if( pActorClient->GetSkill( "dexterity", &pBasicSkill ) )
			{
				initialDex = pBasicSkill->GetNaturalLevel();
				pBasicSkill->SetNaturalLevel( dDex + pBasicSkill->GetNaturalLevel() );
				if ( floorf(initialDex) != floorf(pBasicSkill->GetNaturalLevel()) )
				{
					bUpdateInv = true;
					gpscreenf(( $MSG$	"%s has just increased in Dexterity!", 
										::ToAnsi( hClient->GetCommon()->GetScreenName().c_str() ).c_str() ));
				}
			}
			if( pActorClient->GetSkill( "intelligence", &pBasicSkill ) )
			{
				initialInt = pBasicSkill->GetNaturalLevel();
				pBasicSkill->SetNaturalLevel( dInt + pBasicSkill->GetNaturalLevel() );
				if ( floorf(initialInt) != floorf(pBasicSkill->GetNaturalLevel()) )
				{
					bUpdateInv = true;
					gpscreenf(( $MSG$	"%s has just increased in Intelligence!", 
										::ToAnsi( hClient->GetCommon()->GetScreenName().c_str() ).c_str() ));
				}
			}

			// Make sure all the level indication stuff is correct
			pActorClient->GetGo()->SetModifiersDirty();
			pActorClient->GetGo()->CheckModifierRecalc();

			// Determine what class the actor is now with the new experience applied
			UpdateClassDesignation( client );

			// Set inventory to dirty to update grid highlights
			if ( bUpdateInv )
			{
				hClient->GetInventory()->SetInventoryDirty();				
			}
		}
	}
}


void
Rules::UpdateClassDesignation( Goid Client )
{
	GoHandle hClient( Client );

	if( hClient && hClient->HasActor() )
	{
		GoActor *pClientActor = hClient->GetActor();

		SkillColl::const_iterator iSkill	 = pClientActor->GetSkills().begin();
		SkillColl::const_iterator iSkill_end = pClientActor->GetSkills().end();

		int melee = 0;
		int ranged = 0;
		int natureMagic = 0;
		int combatMagic = 0;

		for(; iSkill != iSkill_end; ++iSkill )
		{
			if ( (*iSkill).GetName().same_no_case( "melee" ) )
			{
				melee = (int)floorf((*iSkill).GetLevel());
			}
			else if ( (*iSkill).GetName().same_no_case( "ranged" ) )
			{
				ranged = (int)floorf((*iSkill).GetLevel());
			}
			else if ( (*iSkill).GetName().same_no_case( "nature magic" ) )
			{
				natureMagic = (int)floorf((*iSkill).GetLevel());
			}
			else if ( (*iSkill).GetName().same_no_case( "combat magic" ) )
			{
				combatMagic = (int)floorf((*iSkill).GetLevel());
			}
		}

		if ( !(melee == 0 && ranged == 0 && natureMagic == 0 && combatMagic == 0) )
		{
			gpwstring actorClass = ReportSys::TranslateW( GetCombinedClass( melee, ranged, natureMagic, combatMagic, pClientActor->GetIsMale() ) );
			pClientActor->SetClass( actorClass );
		}
		else
		{
			static AutoConstQuery <gpstring> s_Query( "screen_class" );
			gpwstring actorClass = ReportSys::TranslateW( s_Query.Get( pClientActor->GetData()->GetRecord() ) );			
			pClientActor->SetClass( actorClass );
		}
	}
}


void Rules::SetPackMemberStats( Goid party_member )
{
	GoHandle hClient( party_member );

	if( hClient && hClient->HasParent() )
	{
		// Syncronize pack animals/people stats to the avarage of the parties based on the multipliers
		gpassert( hClient->GetParent()->HasParty() );
		const GopColl &party = hClient->GetParent()->GetChildren();

		float str_avg = 0;
		float int_avg = 0;
		float dex_avg = 0;
		float n = 0;

		// Sum the skills
		GopColl::const_iterator it = party.begin(), it_end = party.end();
		for( ; it != it_end; ++it )
		{
			GoInventory *pInventory = (*it)->QueryInventory();

			// Skip pack animals
			if( pInventory && !pInventory->IsPackOnly() )
			{
				GoActor *pPartyActor = (*it)->GetActor();

				str_avg += pPartyActor->GetSkillLevel( "strength" );
				int_avg += pPartyActor->GetSkillLevel( "intelligence" );
				dex_avg += pPartyActor->GetSkillLevel( "dexterity" );

				n += 1.0f;
			}
		}

		// Average them and scale them based on the influences
		if( n > 0 )
		{
			str_avg	= max_t( 1.0f, m_StrPackInfluence * (str_avg / n) );
			int_avg	= max_t( 1.0f, m_IntPackInfluence * (int_avg / n) );
			dex_avg	= max_t( 1.0f, m_DexPackInfluence * (dex_avg / n) );

			// Apply them to the pack animals only
			it = party.begin();

			for( ; it != it_end; ++it )
			{
				GoInventory *pInventory = (*it)->QueryInventory();

				if( pInventory && pInventory->IsPackOnly() )
				{
					(*it)->GetActor()->RCSetSkillLevels( str_avg, int_avg, dex_avg );
					(*it)->SetModifiersDirty();
				}
			}
		}
	}
}


void
Rules::CalculateAimingError( Goid Client, const char *szSkill, float &x_axis, float &y_axis )
{
	GoHandle hClient( Client );

	if( hClient )
	{
		if( hClient->HasActor() )
		{
			GoActor *pActor = hClient->GetActor();

			const float Dexterity	 = pActor->GetSkillLevel( "Dexterity"	);
			const float Intelligence = pActor->GetSkillLevel( "Intelligence"	);
			const float Skill_level	 = pActor->GetSkillLevel( szSkill		);

			const float error = m_ErrorScalar*((100.0f-ATan( ((m_Dex_*Dexterity)+(m_Int_*Intelligence)+(m_Skill_*Skill_level))/14.7f )*63.0f)/100.0f);
	
			x_axis = DegreesToRadians( Random( -error, error ) );
			y_axis = DegreesToRadians( Random( -error, error ) );
		}
		else
		{
			x_axis = 0.0f;
			y_axis = 0.0f;
		}
	}
}


void
Rules::CalculateWeaponAimingError( Goid Weapon, float &x_axis, float &y_axis )
{
	GoHandle hWeapon( Weapon );

	if( hWeapon && hWeapon->HasAttack() )
	{
		GoAttack *pAttack = hWeapon->GetAttack();

		const float x_error = pAttack->GetWeaponErrorRangeX();
		const float y_error = pAttack->GetWeaponErrorRangeY();

		x_axis = DegreesToRadians( Random( -x_error, x_error ) );
		y_axis = DegreesToRadians( Random( -y_error, y_error ) );
	}
}


void
Rules::OnProjectileCollision( Goid Projectile, Goid Collided )
{
	CHECK_SERVER_ONLY;

	GoHandle	hProjectile( Projectile );

	if( hProjectile )
	{
		Go * pWeapon, * pAttacker;

		// Collided = GOID_INVALID if collision with terrain occured

		// Shot from a weapon by a go
		if( (Collided != GOID_INVALID) && hProjectile->GetParent( pWeapon ) && ( pWeapon->GetParent( pAttacker ) ) )
		{
			// ammo -> weapon -> attacker
			Goid Attacker;

			if( pAttacker->IsActor() )
			{
				Attacker = pAttacker->GetGoid();
			}
			else
			{
				Attacker = pWeapon->GetGoid();
			}

			if( pWeapon->IsSpell() )
			{	
				SiegePos DamagePos( hProjectile->GetPlacement()->GetPosition() );
				DamageGoMagic( Attacker, pWeapon->GetGoid(), Collided, DamagePos );	
			}
			else
			{
				DamageGoRanged( Attacker, Projectile, Collided );
			}
		}
		// Shot from a trap or anything else
		else
		{
			if( hProjectile->HasAttack() )
			{
				GoAttack *pAttack = hProjectile->GetAttack();
				const float min_damage = pAttack->GetDamageMin();
				const float max_damage = pAttack->GetDamageMax();

				Goid ProjectileLauncher( GOID_INVALID );
				if( hProjectile->HasParent() )
				{
					ProjectileLauncher = hProjectile->GetParent()->GetGoid();
				}

				// If the collision was with a go do hit damage
				if( Collided != GOID_INVALID )
				{
					DamageGoTrap( Collided, Projectile, ProjectileLauncher, min_damage, max_damage );
				}

				if( (pAttack->GetAreaDamageRadius() > 0) && hProjectile->HasPhysics() && hProjectile->IsAmmo() )
				{
					const float radius = pAttack->GetAreaDamageRadius();
					const float damage = Random( min_damage, max_damage );
					const float magnitude = CalculateMagnitude( hProjectile->GetPhysics()->GetExplosionMagnitude(), damage );

					SimID explosion = gSim.CreateExplosion( hProjectile->GetPlacement()->GetPosition(), radius, magnitude, damage, Projectile, ProjectileLauncher );
					gSim.SetIgnorePartyMembers( explosion );
				}
			}
		}
	}
}


float
Rules::CalculateMagnitude( float mag_a, float damage )
{
	mag_a = (mag_a==0)?1.0f:mag_a;
	const float mag_b = damage / 8.0f;
	return ( min_t( 10.0f, mag_a + max_t( 1.25f, mag_b ) ) );
}


void
Rules::ChangeLife( Goid Client, float LifeDelta, DWORD rpcWhere )
{
	gpassert( (rpcWhere != RPC_TO_ALL) || ::IsServerLocal() );

	if( LifeDelta == 0 )
	{
		return;
	}

	/*
		-----------------------------------------------------------------------
		-=*** DO NOT CHANGE THIS FUNCTION WITHOUT READING THESE COMMENTS! ***=-
		-----------------------------------------------------------------------
	-	Characters that level up or are able to increase their skills have their MaxLife calculated.

	-	You cannot set MaxLife for any character that levels up. If you try the setting will only last
		until the next time SetLife is called which is one game sim.

	-	Only characters that can level up can go unconscious.

	-	A character is considered to be conscious if their life is above 1

	-	A monster is considered to be dead when thier life is below 1

	-	A character is considered to be dead if their life is below -MaxLife * death_threshold
		Thus unconsciousness is the state in between -MaxLife*death_threshold and 1.

	-	While a character is unconscious he is healing and will eventually get back up again once his
		life has gone above 0 and once this happens he will instantly go from 0 life to 1 life.

	-	If a character is knocked unconscious and there are threats near he will stay unconscious
		while gaining hit points through natural recovery but only up to 0 until the threat is
		gone. Once the threat is gone he gets up with 1 life.

	-	Going unconscious carries a time penalty with it which is currently 5 seconds. If you go
		unconscious you stay down for a minimum of 5 seconds before you can get back up. RestoreLife
		keeps track of the penalty duration.
		-----------------------------------------------------------------------
		-=*** DO NOT CHANGE THIS FUNCTION WITHOUT READING THESE COMMENTS! ***=-
		-----------------------------------------------------------------------
	*/

	GoHandle hClient( Client );

	if( !hClient )
	{
		return;
	}

	GoAspect	*pAspectClient	= hClient->GetAspect();

	// Get the old lifestate before modification
	const eLifeState ols = pAspectClient->GetLifeState();

	const bool bWasDead =	(( ols == LS_DEAD_NORMAL  ) ||
							(  ols == LS_DEAD_CHARRED ) ||
							(  ols == LS_DECAY_FRESH  ) ||
							(  ols == LS_DECAY_BONES  ) ||
							(  ols == LS_DECAY_DUST   ) );

	// Calculate mana shield, if any
	if ( LifeDelta < 0 )
	{
		if ( hClient->HasDefend() && (hClient->GetDefend()->GetManaShield() > 0) )
		{
			float manaShield = (hClient->GetDefend()->GetManaShield() / 100.0f) * fabsf( LifeDelta );

			manaShield = min_t( manaShield, pAspectClient->GetCurrentMana() );

			LifeDelta += manaShield;

			// this code will execute on both client and server - client will recieve periodic sync from server to sync life & mana up
			pAspectClient->SetCurrentMana( pAspectClient->GetCurrentMana() - manaShield );
		}
	}

	float NewLife = pAspectClient->GetCurrentLife() + LifeDelta;

	if( NewLife > pAspectClient->GetMaxLife() )
	{
		NewLife = pAspectClient->GetMaxLife();
	}

	bool bCanLevelUp = false;
	if( hClient->HasActor() && hClient->GetActor()->GetCanLevelUp() )
	{
		bCanLevelUp = true;		
	}
	
	// if life is >= 1 then we are alive
	if( NewLife >= 1.0f && pAspectClient->GetLifeState() != LS_GHOST )
	{
		const eLifeState life_state = pAspectClient->GetLifeState();

		// if we are unconscious, get up since we're about to increase life such that we are conscious again
		if( gServer.IsLocal() &&
			(( life_state == LS_DEAD_NORMAL  ) ||
			(  life_state == LS_DEAD_CHARRED ) ||
			(  life_state == LS_DECAY_FRESH  ) ||
			(  life_state == LS_DECAY_BONES  ) ||
			(  life_state == LS_DECAY_DUST   ) ||
			(  life_state == LS_ALIVE_UNCONSCIOUS  ) ))
		{
			// Send appropriate messages
			WorldMessage( WE_GAINED_CONSCIOUSNESS, GOID_INVALID, Client, MakeInt(Client) ).Send();
		}

		if( ( life_state == LS_ALIVE_UNCONSCIOUS  ) && bCanLevelUp )
		{
			// clamp life
			NewLife = 1.0f;
		}

		if( gServer.IsLocal() )
		{
			pAspectClient->SSetLifeState( LS_ALIVE_CONSCIOUS, false );

			if( bWasDead )
			{
				WorldMessage( WE_RESURRECTED, GOID_INVALID, Client, MakeInt(Client) ).Send();
			}
		}
	}
	else
	{
	// Life is below 0 so we are either dead or unconscious
		const eLifeState life_state = pAspectClient->GetLifeState();

		// Determine the death threshold
		float DeathThreshold = bCanLevelUp ? ( -pAspectClient->GetMaxLife() * m_DeathThreshold ) : 1.0f;

		if( NewLife <= DeathThreshold )
		{
			// Death
			const bool is_alive = ( (life_state == LS_ALIVE_CONSCIOUS ) || (life_state == LS_ALIVE_UNCONSCIOUS) );

			// Without this test fKILLED gets sent every frame, we just want a state delta
			if( is_alive ) // currently alive, soon to be marked dead...
			{
				// I'm commenting this out this because we cannot resurrect people that aren't selectable
				// because the Siege mouse shadow only raycasts on selectable objects.
				// pAspectClient->SSetIsSelectable( false );

				if( gServer.IsLocal() )
				{
					pAspectClient->SSetLifeState( LS_DEAD_NORMAL );
				}
			}
		}
		else
		{
			// Unconsciousness
			// Without this test fUNCONSCIOUS gets sent every frame, we just want a state deltas

			if( (life_state == LS_ALIVE_CONSCIOUS ) && bCanLevelUp )
			{
				// only the server can reset and clear the unconsious duration.
				if ( gServer.IsLocal() && hClient->HasActor() )
				{
					hClient->GetActor()->RSResetUnconsciousDuration( m_MinUnconsciousDuration );
				}

				// I'm commenting this out this because we cannot resurrect people that aren't selectable
				// because the Siege mouse shadow only raycasts on selectable objects.
				// pAspectClient->SSetIsSelectable( true );

				pAspectClient->GetGo()->Deselect();

				if( gServer.IsLocal() )
				{
					pAspectClient->SSetLifeState( LS_ALIVE_UNCONSCIOUS );

					// Send a message telling the victim he is unconscious
					WorldMessage( WE_LOST_CONSCIOUSNESS, GOID_INVALID, Client, MakeInt(Client) ).Send();
				}
			}
		}
	}

	// finally, set the life
	if ( rpcWhere == RPC_TO_ALL )
	{
		pAspectClient->SSetCurrentLife( NewLife );
	}
	else
	{
		gpassert( rpcWhere == RPC_TO_LOCAL );
		pAspectClient->SetCurrentLife( NewLife );
	}
}


void
Rules::ChangeMana( Goid Client, float Delta, DWORD rpcWhere )
{
	GoHandle hClient( Client );

	if( !hClient )
	{
		return;
	}

	GoAspect *pAspectClient = hClient->GetAspect();

	float Mana = 0;

	if( IsNegative( Delta ) )
	{
		gpassertm( (pAspectClient->GetCurrentMana() + Delta) >= 0, "Trying to take too much mana away!" );

		Mana = max( 0.0f, pAspectClient->GetCurrentMana() + Delta );
	}
	else
	{
		Mana = min( pAspectClient->GetMaxMana(), pAspectClient->GetCurrentMana() + Delta );
	}

	if ( rpcWhere == RPC_TO_ALL )
	{
		pAspectClient->SSetCurrentMana( Mana );
	}
	else
	{
		gpassert( rpcWhere == RPC_TO_LOCAL );
		pAspectClient->SetCurrentMana( Mana );
	}
}


float
Rules::CalculateMaxLife( Go * pClient )
{
	gpassert( pClient->HasActor() && pClient->GetActor()->GetCanLevelUp() );

	GoActor *pActorClient = pClient->GetActor();

	float strength_bonus		= 0;
	float dexterity_bonus		= 0;
	float intelligence_bonus	= 0;

	const float strength = pActorClient->GetSkillLevel( "strength" );
	if( strength < 10 )
	{
		strength_bonus = floorf(strength) * m_MaxLifeStrPercent;
	}
	else
	{
		strength_bonus = (floorf(strength)-9.0f) * m_MaxLifeStrPercent * m_MaxLifeConstant;
	}

	const float dexterity = pActorClient->GetSkillLevel( "dexterity" );
	if( dexterity < 10 )
	{
		dexterity_bonus = floorf(dexterity) * m_MaxLifeDexPercent;
	}
	else
	{
		dexterity_bonus = (floorf(dexterity)-9.0f) * m_MaxLifeDexPercent * m_MaxLifeConstant;
	}

	const float intelligence = pActorClient->GetSkillLevel( "intelligence" );
	if( intelligence < 10 )
	{
		intelligence_bonus = floorf(intelligence) * m_MaxLifeIntPercent;
	}
	else
	{
		intelligence_bonus = (floorf(intelligence)-9.0f) * m_MaxLifeIntPercent * m_MaxLifeConstant;
	}

	return ( m_MaxLifeBase + strength_bonus + dexterity_bonus + intelligence_bonus );
}


void
Rules::RecalculateMaxLife( Go * pClient )
{
	gpassert( pClient->HasActor() && pClient->GetActor()->GetCanLevelUp() );

	GoAspect *pAspectClient = pClient->GetAspect();

	const float new_life = CalculateMaxLife( pClient );
	pAspectClient->SetNaturalMaxLife( new_life );
}


void
Rules::RegenerateLife( Go * pClient, float deltaTime )
{
	GoActor *pActorClient = pClient->HasActor() ? pClient->GetActor() : NULL;
	GoAspect *pAspectClient = pClient->GetAspect();

	eLifeState life_state = pAspectClient->GetLifeState();
	const bool is_alive = ( (LS_ALIVE_CONSCIOUS == life_state) || (LS_ALIVE_UNCONSCIOUS == life_state) );

	if( is_alive && (pAspectClient->GetLifeRecoveryPeriod() > 0) )
	{
		if( !( pActorClient && pActorClient->GetCanLevelUp() && pActorClient->GetUnconsciousDuration() > 0 ) )
		{
#			define PREFIX "life_recovery_rate$?"
			static char formula[ 200 ] = PREFIX;
			char* runner = formula + ELEMENT_COUNT( PREFIX ) - 1;
			int count = ARRAY_END( formula ) - runner;
#			undef PREFIX

			if( pActorClient )
			{
				const float _s = pActorClient->GetSkillLevel( "strength" );
				const float _i = pActorClient->GetSkillLevel( "intelligence" );
				const float _d = pActorClient->GetSkillLevel( "dexterity" );
				const float _m = pActorClient->GetSkillLevel( "melee" );
				const float _r = pActorClient->GetSkillLevel( "ranged" );
				const float _n = pActorClient->GetSkillLevel( "nature magic" );
				const float _c = pActorClient->GetSkillLevel( "combat magic" );

				gpverify( advance_snprintf( runner, count, "str=%g&int=%g&dex=%g&melee=%g&ranged=%g&nmagic=%g&cmagic=%g&", _s,_i,_d,_m,_r,_n,_c ) );
			}
			gpverify( advance_snprintf( runner, count, "lr_unit=%g&lr_period=%g", pAspectClient->GetLifeRecoveryUnit(), pAspectClient->GetLifeRecoveryPeriod() ) );

			const float FormulaResult = gContentDb.EvalFloatFormula( formula, 0.10f );

			const float old_life = pAspectClient->GetCurrentLife();

			float new_life_value = old_life + (deltaTime * FormulaResult );

			new_life_value = min( new_life_value, pAspectClient->GetMaxLife() );

			ChangeLife( pClient->GetGoid(), new_life_value - old_life, RPC_TO_LOCAL );
		}
	}
}


float
Rules::CalculateMaxMana( Go * pClient )
{
	gpassert( pClient->HasActor() && pClient->GetActor()->GetCanLevelUp() );

	GoActor *pActorClient = pClient->GetActor();

	float strength_bonus		= 0;
	float dexterity_bonus		= 0;
	float intelligence_bonus	= 0;

	const float strength = pActorClient->GetSkillLevel( "strength" );
	if( strength < 10 )
	{
		strength_bonus = floorf(strength) * m_MaxManaStrPercent;
	}
	else
	{
		strength_bonus = (floorf(strength)-9.0f) * m_MaxManaStrPercent * m_MaxManaConstant;
	}

	const float dexterity = pActorClient->GetSkillLevel( "dexterity" );
	if( dexterity < 10 )
	{
		dexterity_bonus = floorf(dexterity) * m_MaxManaDexPercent;
	}
	else
	{
		dexterity_bonus = (floorf(dexterity)-9.0f) * m_MaxManaDexPercent * m_MaxManaConstant;
	}

	const float intelligence = pActorClient->GetSkillLevel( "intelligence" );
	if( intelligence < 10 )
	{
		intelligence_bonus = floorf(intelligence) * m_MaxManaIntPercent;
	}
	else
	{
		intelligence_bonus = (floorf(intelligence)-9.0f) * m_MaxManaIntPercent * m_MaxManaConstant;
	}

	return ( m_MaxManaBase + strength_bonus + dexterity_bonus + intelligence_bonus );
}


void
Rules::RecalculateMaxMana( Go * pClient )
{
	gpassert( pClient->HasActor() && pClient->GetActor()->GetCanLevelUp() );

	const float new_mana = CalculateMaxMana( pClient );

	GoAspect *pAspectClient = pClient->GetAspect();
	pAspectClient->SetNaturalMaxMana( new_mana );
}


void
Rules::RegenerateMana( Go * pClient, float deltaTime )
{
	GoAspect *pAspectClient = pClient->GetAspect();
	GoActor *pActorClient = pClient->HasActor() ? pClient->GetActor() : NULL;

	if( pAspectClient->GetManaRecoveryPeriod() > 0 )
	{
		//Calculate how much mana to restore this update
		const float old_mana = pAspectClient->GetCurrentMana();

#		define PREFIX "mana_recovery_rate$?"
		static char formula[ 200 ] = PREFIX;
		char* runner = formula + ELEMENT_COUNT( PREFIX ) - 1;
		int count = ARRAY_END( formula ) - runner;
#		undef PREFIX

		float _s = 0, _i = 0, _d = 0, _m = 0, _r = 0, _n = 0, _c = 0;

		if( pActorClient != NULL )
		{
			_s = pActorClient->GetSkillLevel( "strength" );
			_i = pActorClient->GetSkillLevel( "intelligence" );
			_d = pActorClient->GetSkillLevel( "dexterity" );
			_m = pActorClient->GetSkillLevel( "melee" );
			_r = pActorClient->GetSkillLevel( "ranged" );
			_n = pActorClient->GetSkillLevel( "nature magic" );
			_c = pActorClient->GetSkillLevel( "combat magic" );
		}
		gpverify( advance_snprintf( runner, count, "str=%g&int=%g&dex=%g&melee=%g&ranged=%g&nmagic=%g&cmagic=%g&", _s,_i,_d,_m,_r,_n,_c ) );
		gpverify( advance_snprintf( runner, count, "mr_unit=%g&mr_period=%g", pAspectClient->GetManaRecoveryUnit(), pAspectClient->GetManaRecoveryPeriod() ) );

		const float FormulaResult = gContentDb.EvalFloatFormula( formula, 0.10f );

		float new_mana_value = old_mana + ( deltaTime * FormulaResult );

		new_mana_value = min( new_mana_value, pAspectClient->GetMaxMana() );

		ChangeMana( pClient->GetGoid(), new_mana_value - old_mana, RPC_TO_LOCAL );
	}
}


Rules::eHitType
Rules::DetermineHitType( float damage_done, float min_damage, float max_damage )
{
	eHitType hitType = HIT_NONE;

	// $ tune the range for glancing vs. solid blow here - avg / 2
	if ( damage_done < ((min_damage + max_damage) / 4.0f) )
	{
		hitType = HIT_GLANCING;
	}
	else
	{
		hitType = HIT_SOLID;
	}

	return hitType;
}


void
Rules::PlayCombatSound( eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow )
{
	GoHandle hWeapon( Weapon );
	GoHandle hVictim( Victim );

	if( !(hWeapon && hVictim) )
	{
		return;
	}

	// Play combat sounds
	switch ( hitType )
	{
		case ( HIT_GLANCING ):
		{
			hVictim->PlayVoiceSound( bFatalBlow ? "die" : "hit_glance" );
			hWeapon->PlayMaterialSound( "attack_hit_glance", hVictim->GetBestArmor()->GetGoid() );
		}
		break;

		case ( HIT_SOLID ):
		{
			if ( bFatalBlow )
			{
				hVictim->PlayVoiceSound( "die" );
			}
			else
			{
				DWORD sound = hVictim->PlayVoiceSound( "hit_solid" );
				if ( sound == GPGSound::INVALID_SOUND_ID )
				{
					hVictim->PlayVoiceSound( "hit_glance" );
				}
			}

			DWORD sound = hWeapon->PlayMaterialSound( "attack_hit_solid", hVictim->GetBestArmor()->GetGoid() );
			if ( sound == GPGSound::INVALID_SOUND_ID )
			{
				hWeapon->PlayMaterialSound( "attack_hit_glance", hVictim->GetBestArmor()->GetGoid() );
			}
		}
		break;

		case ( HIT_CRITICAL ):
		{
			DWORD sound = hVictim->PlayVoiceSound( "hit_critical" );
			if ( sound == GPGSound::INVALID_SOUND_ID )
			{
				hVictim->PlayVoiceSound( "die" );
			}

			sound = hWeapon->PlayMaterialSound( "attack_hit_critical", Victim );
			if ( sound == GPGSound::INVALID_SOUND_ID )
			{
				sound = hWeapon->PlayMaterialSound( "attack_hit_solid", Victim );
				if ( sound == GPGSound::INVALID_SOUND_ID )
				{
					hWeapon->PlayMaterialSound( "attack_hit_glance", Victim );
				}
			}
		}
		break;

		case ( HIT_EXPLODED ):
		{
			DWORD sound = hVictim->PlayVoiceSound( "hit_exploded" );
			if( sound == GPGSound::INVALID_SOUND_ID ) {
				// $ Todo: play a generic explosion sound
			}
		} break;
	}
}


void
Rules::RCPlayCombatSound( eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow )
{
	// $ This call is brodcast to the appropriate clients through the go

	GoHandle go( Victim );
	gpassert( go );
	go->RCPlayCombatSound( hitType, Weapon, Victim, bFatalBlow );
}




//
// Privately declared
//

float
Rules::GetAreaDamageRadius( Goid Weapon )
{
	float area_damage_radius = 0;

	GoHandle hWeapon( Weapon );

	if( hWeapon )
	{
		// Get the radius for the damage volume
		if( GOID_INVALID != hWeapon->GetAttack()->GetProjectileLauncher() )
		{
			GoHandle hLauncher( hWeapon->GetAttack()->GetProjectileLauncher() );
			area_damage_radius = hLauncher->GetAttack()->GetAreaDamageRadius();
		}
		else
		{
			area_damage_radius = hWeapon->GetAttack()->GetAreaDamageRadius();
		}
	}
	return area_damage_radius;
}


const gpstring&
Rules::GetCombinedClass( int melee, int ranged, int natureMagic, int combatMagic, bool bMale )
{
	ClassLookupColl::reverse_iterator iClass;
	for ( iClass = m_ClassLookup.rbegin(); iClass != m_ClassLookup.rend(); ++iClass )
	{
		if ( ((*iClass).bMale && (*iClass).bFemale) ||
			 ( bMale == (*iClass).bMale ) )
		{
			if ( (melee >= (*iClass).meleeSkillMin || (*iClass).meleeSkillMin == 0 ) &&
				 (melee <= (*iClass).meleeSkillMax || (*iClass).meleeSkillMax == 0 ) &&
				 (ranged >= (*iClass).rangedSkillMin || (*iClass).rangedSkillMin == 0 ) &&
				 (ranged <= (*iClass).rangedSkillMax || (*iClass).rangedSkillMax == 0 ) &&
				 (natureMagic >= (*iClass).natureMagicSkillMin || (*iClass).natureMagicSkillMin == 0 ) &&
				 (natureMagic <= (*iClass).natureMagicSkillMax || (*iClass).natureMagicSkillMax == 0 ) &&
				 (combatMagic >= (*iClass).combatMagicSkillMin || (*iClass).combatMagicSkillMin == 0 ) &&
				 (combatMagic <= (*iClass).combatMagicSkillMax || (*iClass).combatMagicSkillMax == 0 ) )
			{
				return (*iClass).sClass;
			}
		}
	}

	return gpstring::EMPTY;
}


void
CombatSoundXfer::Xfer( FuBi::BitPacker& packer )
{
	packer.XferRaw( m_HitType, FUBI_MAX_ENUM_BITS( Rules::HIT_ ) );
	packer.XferRaw( m_Weapon );
	packer.XferBit( m_bFatalBlow );
}

#if !GP_RETAIL

void
CombatSoundXfer::RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf( "hitType = %d, weapon = 0x%08X", m_HitType, m_Weapon );
	if ( m_bFatalBlow )
	{
		appendHere += " +fatalBlow";
	}
}

#endif // !GP_RETAIL


FUBI_DECLARE_SELF_TRAITS( Skill );


Skill::Skill( const gpstring &sName )
 :	m_sName( sName )
 ,	m_StrInfluence( 0 )
 ,	m_IntInfluence( 0 )
 ,	m_DexInfluence( 0 )
 ,	m_XP( 0 )
 ,  m_XP_NextLevel( 0 )
 ,	m_Level( 0 )
 ,	m_LevelBias( 0 )
 ,	m_LevelBiasModifier( 0 )
 ,  m_MaxLevel( 0 )
 ,	m_pGo( NULL )
{}


bool
Skill::Init( const char *sData )
{
	// Scan it
	const UINT32 param_count = stringtool::GetNumDelimitedStrings( sData, ',' );

	if ( param_count < 2 )
	{
		return ( false );
	}

	gpstring sLevel;
	gpstring sXP;
	gpstring sLevelBias;

	float level		= 0;
	float XP		= 0;

	stringtool::GetDelimitedValue( sData, ',', 0, sLevel );
	stringtool::GetDelimitedValue( sData, ',', 1, sXP );
	stringtool::Get( sLevel,	level );
	stringtool::Get( sXP,		XP );

	if( param_count > 2 )
	{
		stringtool::GetDelimitedValue( sData, ',', 2, sLevelBias );
		stringtool::Get( sLevelBias,	m_LevelBias );
	}

	// Set values
	if ( XP == 0 )
	{
		SetNaturalLevel( level );
	}
	else
	{
		SetExperience( XP );
	}

	m_XP_NextLevel = gRules.GetNextLevelXP( level );

	return true;
}


bool
Skill::Xfer( FuBi::PersistContext& persist )
{
	persist.XferQuoted( "m_sName",             m_sName				);
	persist.XferQuoted( "m_sClass",            m_sClass				);
	persist.Xfer      ( "m_StrInfluence",      m_StrInfluence		);
	persist.Xfer      ( "m_DexInfluence",      m_DexInfluence		);
	persist.Xfer      ( "m_IntInfluence",      m_IntInfluence		);
	persist.Xfer      ( "m_XP",                m_XP					);
	persist.Xfer      ( "m_XP_NextLevel",      m_XP_NextLevel		);
	persist.Xfer      ( "m_Level",             m_Level				);
	persist.Xfer      ( "m_LevelBias",         m_LevelBias			);
	persist.Xfer      ( "m_LevelBiasModifier", m_LevelBiasModifier	);
	persist.Xfer	  ( "m_MaxLevel",		   m_MaxLevel			);
	persist.Xfer      ( "m_pGo",               m_pGo				);

	return ( true );
}


void
Skill::ResetModifiers( void )
{
	m_Level.Reset();
}


void
Skill::SetExperience( double val )
{
	if( val < 0 )
	{
		gperror( "Skill::SetExperience - (val < 0)\n" );
		return;
	}

	m_XP = val;
	if ( m_pGo != NULL )
	{
		m_pGo->SetModifiersDirty();
	}
}


void
Skill::SetNaturalLevel( float val )
{
	if( val < 0 )
	{
		gperror( "Skill::SetNaturalLevel - (val < 0)\n" );
		return;
	}

	val = min( val, gRules.GetMaxLevel() );

	m_Level.SetNatural( val );

	if ( m_pGo != NULL )
	{
		m_pGo->SetModifiersDirty();
	}

	SetExperience( gRules.LevelToXP( val ) );

	m_XP_NextLevel = gRules.GetNextLevelXP( m_Level.GetNatural() );
}


bool
Skill::AwardExperience( double experience_points, bool bSetDirty )
{
	if( experience_points < 0 )
	{
		gperror( "Skill::AwardExperience - (experience_points < 0)\n" );
		return false;
	}

	const float old_level = floorf(m_Level.GetNatural());

	const double new_xp = m_XP + experience_points;

	// Accumulate experience points
	SetExperience( new_xp );

	const float new_level = gRules.XPToLevel( m_XP );

	m_Level.SetNatural( new_level );

	const int levels_increased	= (int)(floorf(new_level) - old_level );

	if( levels_increased > 0 )
	{
		m_XP_NextLevel = gRules.GetNextLevelXP( m_Level.GetNatural() );

		if( bSetDirty )
		{
			m_pGo->SetModifiersDirty();
			m_pGo->GetPlayer()->SetPartyDirty();
		}

		return true;
	}
	return false;
}


void
SkillColl::LoadSkills( FastFuelHandle hSkills )
{
	FastFuelFindHandle fh( hSkills );

	if( hSkills && fh.FindFirstKeyAndValue() )
	{
		gpstring sKey, sData;
		Skill* pSkill = NULL;

		// Read a line in
		while ( fh.GetNextKeyAndValue( sKey, sData ) )
		{
			// Convert underscores in key to spaces
			for( size_t i = sKey.size()-1; i > 0; --i )
			{
				if( sKey[ i ] == '_' )
				{
					sKey.at_split( i ) = ' ';
				}
			}

			// Find the skill
			if ( GetSkill( sKey, &pSkill ) )
			{
				if ( !pSkill->Init( sData ) )
				{
					gperrorf(( "Bad skill data format '%s' at '%s'\n", sData.c_str(), hSkills.GetAddress().c_str() ));
				}
			}
			else
			{
				gperrorf(( "Unknown skill name '%s' at '%s'\n", sKey.c_str(), hSkills.GetAddress().c_str() ));
			}
		}
	}
}


float
SkillColl::GetSkillLevel( const char *szSkill ) const
{
	Skill* pSkill;
	if ( GetSkill( szSkill, &pSkill ) )
	{
		return ( pSkill->GetLevel() );
	}
	else
	{
		return ( 0 );
	}
}


bool
SkillColl::GetSkill( const char *szSkill, Skill **pSkill ) const
{
	const_iterator iSkill		= begin();
	const_iterator iSkill_end	= end();

	for(; iSkill != iSkill_end; ++iSkill )
	{
		if( (*iSkill).GetName().same_no_case( szSkill ) )
		{
			*pSkill = ccast <Skill*> ( &*iSkill );
			return true;
		}
	}
	return false;
}


bool
SkillColl::GetSkill( const char *szSkill, CPSkill *pSkill ) const
{
	const_iterator iSkill		= begin();
	const_iterator iSkill_end	= end();

	for(; iSkill != iSkill_end; ++iSkill )
	{
		if( (*iSkill).GetName().same_no_case( szSkill ) )
		{
			*pSkill = &(*iSkill);
			return true;
		}
	}
	return false;
}


bool
SkillColl::HasSkill( const char* szSkill ) const
{
	const Skill* dummy;
	return ( GetSkill( szSkill, &dummy ) );
}


void
SkillColl::ResetModifiers( void )
{
	stdx::for_each( *this, stdx::vmem_fun_ref( &Skill::ResetModifiers ) );
}


bool
SkillColl::Xfer( FuBi::PersistContext& persist, const char* key )
{
	return ( persist.XferVector( key, *this ) );
}


bool
SkillColl::XferCharacter( FuBi::PersistContext& persist, const char* key )
{
	persist.EnterBlock( key );

	gpstring name;
	for ( iterator i = begin() ; i != end() ; ++i )
	{
		double xp = 0;
		if ( persist.IsSaving() )
		{
			xp = i->GetExperience();
		}

		name = i->GetName();
		std::replace( name.begin_split(), name.end_split(), ' ', '_' );
		persist.Xfer( name + "_xp", xp );

		if ( persist.IsSaving() )
		{
			// this is informational only (for character chooser)
			float level = i->GetNaturalLevel() + i->GetLevelBias();
			persist.Xfer( name + "_level", level );
		}
		else
		{
			i->SetExperience( xp );		// set experience
			i->AwardExperience( 0 );	// commit
			i->ResetModifiers();		// force modifier reset
		}
	}

	persist.LeaveBlock();

	return ( true );
}
