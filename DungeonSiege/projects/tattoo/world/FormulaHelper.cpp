//////////////////////////////////////////////////////////////////////////////
//
// File     :  FormulaHelper.cpp
// Author(s):  Rick Saenz
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "FormulaHelper.h"

#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoMagic.h"
#include "GoDefend.h"
#include "GoStore.h"
#include "SkritDefs.h"

//////////////////////////////////////////////////////////////////////////////

namespace FormulaHelper
{

enum Token
{
	SET_BEGIN_ENUM( TOKEN_, 0 ),

	TOKEN_INFINITE,


	// Target
	TOKEN_LIFE,
	TOKEN_MAXLIFE,
	TOKEN_LIFE_RECOVERY_UNIT,
	TOKEN_LIFE_RECOVERY_PERIOD,
	TOKEN_MANA,
	TOKEN_MAXMANA,
	TOKEN_MANA_RECOVERY_UNIT,
	TOKEN_MANA_RECOVERY_PERIOD,
	TOKEN_STRENGTH,
	TOKEN_INTELLIGENCE,
	TOKEN_DEXTERITY,
	TOKEN_RANGED,
	TOKEN_MELEE,
	TOKEN_COMBAT_MAGIC_LEVEL,
	TOKEN_NATURE_MAGIC_LEVEL,

	TOKEN_BASE_DAMAGE,
	TOKEN_BASE_DEFENSE,
	TOKEN_HIGHEST_SKILL,

	TOKEN_MELEE_DAMAGE_MIN,
	TOKEN_MELEE_DAMAGE_MAX,

	TOKEN_RANGED_DAMAGE_MIN,
	TOKEN_RANGED_DAMAGE_MAX,

	TOKEN_CMAGIC_DAMAGE_MIN,
	TOKEN_CMAGIC_DAMAGE_MAX,

	TOKEN_NMAGIC_DAMAGE_MIN,
	TOKEN_NMAGIC_DAMAGE_MAX,


	// Source
	TOKEN_SRC_LIFE,
	TOKEN_SRC_MAXLIFE,
	TOKEN_SRC_LIFE_RECOVERY_UNIT,
	TOKEN_SRC_LIFE_RECOVERY_PERIOD,
	TOKEN_SRC_MANA,
	TOKEN_SRC_MAXMANA,
	TOKEN_SRC_MANA_RECOVERY_UNIT,
	TOKEN_SRC_MANA_RECOVERY_PERIOD,
	TOKEN_SRC_STRENGTH,
	TOKEN_SRC_INTELLIGENCE,
	TOKEN_SRC_DEXTERITY,
	TOKEN_SRC_RANGED,
	TOKEN_SRC_MELEE,
	TOKEN_SRC_COMBAT_MAGIC_LEVEL,
	TOKEN_SRC_NATURE_MAGIC_LEVEL,
	
	TOKEN_SRC_BASE_DAMAGE,
	TOKEN_SRC_BASE_DEFENSE,
	TOKEN_SRC_HIGHEST_SKILL,

	TOKEN_SRC_MELEE_DAMAGE_MIN,
	TOKEN_SRC_MELEE_DAMAGE_MAX,

	TOKEN_SRC_RANGED_DAMAGE_MIN,
	TOKEN_SRC_RANGED_DAMAGE_MAX,

	TOKEN_SRC_CMAGIC_DAMAGE_MIN,
	TOKEN_SRC_CMAGIC_DAMAGE_MAX,

	TOKEN_SRC_NMAGIC_DAMAGE_MIN,
	TOKEN_SRC_NMAGIC_DAMAGE_MAX,

	// Spell
	TOKEN_SPELL_REQ_LEVEL,
	TOKEN_SPELL_MAX_LEVEL,

	// Caster & Spell
	TOKEN_COMBAT_MAGIC,
	TOKEN_NATURE_MAGIC,
	TOKEN_MAGIC,

	SET_END_ENUM( TOKEN_ ),
};


static const char* s_TokenStrings[] =
{
	// General
	"infinite",

	// Target
	"life",
	"maxlife",
	"life_recovery_unit",
	"life_recovery_period",
	"mana",
	"maxmana",
	"mana_recovery_unit",
	"mana_recovery_period",
	"str",
	"int",
	"dex",
	"ranged",
	"melee",
	"combat_magic_level",
	"nature_magic_level",

	"base_damage",
	"base_defense",
	"highest_skill",

	"melee_damage_min",
	"melee_damage_max",

	"ranged_damage_min",
	"ranged_damage_max",

	"cmagic_damage_min",
	"cmagic_damage_max",

	"nmagic_damage_min",
	"nmagic_damage_max",


	// Source
	"src_life",
	"src_maxlife",
	"src_life_recovery_unit",
	"src_life_recovery_period",
	"src_mana",
	"src_maxmana",
	"src_mana_recovery_unit",
	"src_mana_recovery_period",
	"src_str",
	"src_int",
	"src_dex",
	"src_ranged",
	"src_melee",
	"src_combat_magic_level",
	"src_nature_magic_level",

	"src_base_damage",
	"src_base_defense",
	"src_highest_skill",


	"src_melee_damage_min",
	"src_melee_damage_max",

	"src_ranged_damage_min",
	"src_ranged_damage_max",

	"src_cmagic_damage_min",
	"src_cmagic_damage_max",

	"src_nmagic_damage_min",
	"src_nmagic_damage_max",


	// Spell
	"spell_req_level",
	"spell_max_level",

	// Source & Spell
	"combat_magic",
	"nature_magic",
	"magic",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_TokenStrings ) == TOKEN_COUNT );

static stringtool::EnumStringConverter s_TokenConverter( s_TokenStrings, TOKEN_BEGIN, TOKEN_END );

bool GetToken( const gpstring &sToken, UINT32 &token )
	{  return ( s_TokenConverter.FromString( sToken, rcast <int&> ( token ) ) );  }


bool
EvaluateFormula( const char *sFormula, const char *sOverrides, float &result, Goid target_go, Goid source_go, Goid item_go )
{
	GoHandle hTarget( target_go );
	GoHandle hSource( source_go );

	// Make copy of strings so it can be maninpulated
	gpstring sExpression = sFormula;

	// Process the overrides if specified
	if( sOverrides && (*sOverrides != 0) )
	{
		// Remove whitespace
		size_t plen = ::strlen( sOverrides );
		char* pOverrides = (char* )_alloca( plen );
		char* pBegin = pOverrides;

		{for( const char *pChar = sOverrides, *pEnd = sOverrides + plen; pChar != pEnd; ++pChar )
		{
			if( (*pChar != ' ') && (*pChar != '\t') && (*pChar != '\n') && (*pChar != '\r') )
			{
				// copy and null terminate each individual entry
				*pOverrides++ = (*pChar != ',') ? *pChar : char( 0 );
			}
		}}

		// null terminate the new string
		*pOverrides = char( 0 );
		
		// do substitution
		char *pCur = pBegin;
		char *pEnd = pOverrides;

		char *pMacro= pCur;
		char *pValue= pCur;

		while( pCur < pEnd )
		{
			// Point to the macro
			pMacro = pCur;

			// Point to the value
			for(;( (*pCur != '=') || (*pCur == 0) ); ++pCur);
			*pCur = 0;
			++pCur;
			pValue = pCur;

			// replace
			size_t start = 0;
			while( start != gpstring::npos )
			{
				start = sExpression.find( pMacro, start );

				const size_t macro_length = ::strlen( pMacro );

				if( (start != gpstring::npos) )
				{
					const size_t exp_end = sExpression.find_first_of( "?:()+-*/^\r\n\t. ", start + macro_length );

					if( (start + macro_length) == exp_end )
					{
						sExpression.replace( start, macro_length, pValue );
					}
					start += ::strlen( pValue );
				}
			}

			// advance to next param
			for(; ((*pCur != 0) && (pCur < pEnd)); ++pCur);
			if( (*pCur == 0 ) && ( pCur < pEnd ) )
			{
				pCur++;
			}
		}
	}


	// Begin starting to parse the actual formula starting by replacing the # macros
	unsigned int start_pos = 0, end_pos = 0;

	for( unsigned int cur_pos = 0; cur_pos < sExpression.size()-1; ++cur_pos )
	{
		bool bFinished = false;

		start_pos = sExpression.find_first_of( "#", 0 );
		end_pos = sExpression.find_first_of( "?:()+-*/^\r\n\t. ", start_pos );

		if( start_pos == gpstring::npos )
		{
			break;
		}

		if( end_pos == gpstring::npos )
		{
			bFinished = true;
			end_pos = sExpression.size();
		}

		gpstring sMacroName = sExpression.substr( start_pos+1, (end_pos - start_pos)-1 );

		UINT32	token = 0;
		float	result = 0;

		if( GetToken( sMacroName, token ) )
		{
			switch( token )
			{
				case TOKEN_INFINITE:				{ result = -1; } break;

				case TOKEN_LIFE:					{ result = (!hTarget)?0:hTarget->GetAspect()->GetCurrentLife();										} break;
				case TOKEN_SRC_LIFE:				{ result = (!hSource)?0:hSource->GetAspect()->GetCurrentLife();										} break;
				case TOKEN_MAXLIFE:					{ result = (!hTarget)?0:hTarget->GetAspect()->GetMaxLife();											} break;
				case TOKEN_SRC_MAXLIFE:				{ result = (!hSource)?0:hSource->GetAspect()->GetMaxLife();											} break;
				case TOKEN_LIFE_RECOVERY_UNIT:		{ result = (!hTarget)?0:hTarget->GetAspect()->GetLifeRecoveryUnit();								} break;
				case TOKEN_SRC_LIFE_RECOVERY_UNIT:	{ result = (!hSource)?0:hSource->GetAspect()->GetLifeRecoveryUnit();								} break;
				case TOKEN_LIFE_RECOVERY_PERIOD:	{ result = (!hTarget)?0:hTarget->GetAspect()->GetLifeRecoveryPeriod();								} break;
				case TOKEN_SRC_LIFE_RECOVERY_PERIOD:{ result = (!hSource)?0:hSource->GetAspect()->GetLifeRecoveryPeriod();								} break;
				case TOKEN_MANA:					{ result = (!hTarget)?0:hTarget->GetAspect()->GetCurrentMana();										} break;
				case TOKEN_SRC_MANA:				{ result = (!hSource)?0:hSource->GetAspect()->GetCurrentMana();										} break;
				case TOKEN_MAXMANA:					{ result = (!hTarget)?0:hTarget->GetAspect()->GetMaxMana();											} break;
				case TOKEN_SRC_MAXMANA:				{ result = (!hSource)?0:hSource->GetAspect()->GetMaxMana();											} break;
				case TOKEN_MANA_RECOVERY_UNIT:		{ result = (!hTarget)?0:hTarget->GetAspect()->GetManaRecoveryUnit();								} break;
				case TOKEN_SRC_MANA_RECOVERY_UNIT:	{ result = (!hSource)?0:hSource->GetAspect()->GetManaRecoveryUnit();								} break;
				case TOKEN_MANA_RECOVERY_PERIOD:	{ result = (!hTarget)?0:hTarget->GetAspect()->GetManaRecoveryPeriod();								} break;
				case TOKEN_SRC_MANA_RECOVERY_PERIOD:{ result = (!hSource)?0:hSource->GetAspect()->GetManaRecoveryPeriod();								} break;
				case TOKEN_STRENGTH:				{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "strength" ):0;		} break;
				case TOKEN_SRC_STRENGTH:			{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "strength" ):0;		} break;
				case TOKEN_INTELLIGENCE:			{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "intelligence" ):0;	} break;
				case TOKEN_SRC_INTELLIGENCE:		{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "intelligence" ):0;	} break;
				case TOKEN_DEXTERITY:				{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "dexterity" ):0;	} break;
				case TOKEN_SRC_DEXTERITY:			{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "dexterity" ):0;	} break;
				case TOKEN_RANGED:					{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "ranged" ):0;		} break;
				case TOKEN_SRC_RANGED:				{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "ranged" ):0;		} break;
				case TOKEN_MELEE:					{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "melee" ):0;		} break;
				case TOKEN_SRC_MELEE:				{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "melee" ):0;		} break;
				case TOKEN_COMBAT_MAGIC_LEVEL:		{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "combat magic" ):0;	} break;
				case TOKEN_SRC_COMBAT_MAGIC_LEVEL:	{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "combat magic" ):0;	} break;
				case TOKEN_NATURE_MAGIC_LEVEL:		{ result = (!hTarget)?0:hTarget->HasActor()?hTarget->GetActor()->GetSkillLevel( "nature magic" ):0;	} break;
				case TOKEN_SRC_NATURE_MAGIC_LEVEL:	{ result = (!hSource)?0:hSource->HasActor()?hSource->GetActor()->GetSkillLevel( "nature magic" ):0;	} break;


				case TOKEN_BASE_DAMAGE:				{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageMin():0;				} break;
				case TOKEN_SRC_BASE_DAMAGE:			{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageMin():0;				} break;
				case TOKEN_BASE_DEFENSE:			{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetDefend()->GetDefense():0;					} break;
				case TOKEN_SRC_BASE_DEFENSE:		{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetDefend()->GetDefense():0;					} break;

				case TOKEN_HIGHEST_SKILL:			{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetActor()->GetHighestSkillLevel():0;			} break;
				case TOKEN_SRC_HIGHEST_SKILL:		{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetActor()->GetHighestSkillLevel():0;			} break;


				case TOKEN_MELEE_DAMAGE_MIN:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinMelee():0;		} break;
				case TOKEN_SRC_MELEE_DAMAGE_MIN:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMinMelee():0;		} break;
																																						
				case TOKEN_MELEE_DAMAGE_MAX:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMaxMelee():0;		} break;
				case TOKEN_SRC_MELEE_DAMAGE_MAX:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxMelee():0;		} break;

				case TOKEN_RANGED_DAMAGE_MIN:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinRanged():0;		} break;
				case TOKEN_SRC_RANGED_DAMAGE_MIN:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMinRanged():0;		} break;
																																						
				case TOKEN_RANGED_DAMAGE_MAX:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMaxRanged():0;		} break;
				case TOKEN_SRC_RANGED_DAMAGE_MAX:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxRanged():0;		} break;
																																						
				case TOKEN_CMAGIC_DAMAGE_MIN:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinCMagic():0;		} break;
				case TOKEN_SRC_CMAGIC_DAMAGE_MIN:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxCMagic():0;		} break;
																																						
				case TOKEN_CMAGIC_DAMAGE_MAX:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinCMagic():0;		} break;
				case TOKEN_SRC_CMAGIC_DAMAGE_MAX:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxCMagic():0;		} break;
																																						
				case TOKEN_NMAGIC_DAMAGE_MIN:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinNMagic():0;		} break;
				case TOKEN_SRC_NMAGIC_DAMAGE_MIN:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxNMagic():0;		} break;
																																						
				case TOKEN_NMAGIC_DAMAGE_MAX:		{ result = (!hTarget)?0:hTarget->HasAttack()?hTarget->GetAttack()->GetDamageBonusMinNMagic():0;		} break;
				case TOKEN_SRC_NMAGIC_DAMAGE_MAX:	{ result = (!hSource)?0:hSource->HasAttack()?hSource->GetAttack()->GetDamageBonusMaxNMagic():0;		} break;



				case TOKEN_SPELL_MAX_LEVEL:
				{
					GoHandle hItem( item_go );

					if( hItem && hItem->HasMagic() )
					{
						result = hItem->GetMagic()->GetMaxCastLevel();
					}
				} break;

				case TOKEN_SPELL_REQ_LEVEL:
				{
					GoHandle hItem( item_go );

					if( hItem && hItem->HasMagic() )
					{
						result = hItem->GetMagic()->GetRequiredCastLevel();
					}
				} break;

				case TOKEN_COMBAT_MAGIC:
				{
					GoHandle hItem( item_go );

					if( hItem && hItem->HasParent() )
					{
						Go *pCaster = hItem->GetParent();
						if ( pCaster->HasStore() && pCaster->GetStore()->IsItemStore() )
						{
							pCaster = hSource.Get();
						}

						// If pCaster is the spellbook get the parent of the spellbook else the relationship is already set
						if( !pCaster->HasActor() )
						{
							pCaster = pCaster->GetParent();
						}

						if( pCaster && pCaster->HasActor() )
						{
							result = pCaster->GetActor()->GetSkillLevel( "combat magic" );
						}
					}
				} break;

				case TOKEN_NATURE_MAGIC:
				{
					GoHandle hItem( item_go );

					if( hItem && hItem->HasParent() )
					{
						Go *pCaster = hItem->GetParent();
						if ( pCaster->HasStore() && pCaster->GetStore()->IsItemStore() )
						{
							pCaster = hSource.Get();
						}

						// If pCaster is the spellbook get the parent of the spellbook else the relationship is already set
						if( !pCaster->HasActor() )
						{
							pCaster = pCaster->GetParent();
						}

						if( pCaster && pCaster->HasActor() )
						{
							result = pCaster->GetActor()->GetSkillLevel( "nature magic" );
						}
					}
				} break;

				case TOKEN_MAGIC:
				{
					GoHandle hItem( item_go );

					if( hItem && hItem->HasParent() )
					{
						Go *pCaster = hItem->GetParent();
						if ( pCaster->HasStore() && pCaster->GetStore()->IsItemStore() )
						{
							pCaster = hSource.Get();
						}

						// If pCaster is the spellbook get the parent of the spellbook else the relationship is already set
						if( !pCaster->HasActor() )
						{
							pCaster = pCaster->GetParent();
						}

						if( pCaster && pCaster->HasActor() )
						{
							const float magic_level = pCaster->GetActor()->GetSkillLevel( hItem->GetMagic()->GetSkillClass() );
							const float max_spell_level = hItem->GetMagic()->GetMaxCastLevel();

							result = min_t( max_spell_level, magic_level );
						}
					}
				} break;
			}
		}
		else
		{
			// Token lookup failed so complain about it and exit
			gpwarningf(("Unknown formula macro #%s\nInvalid formula: %s\nOriginal Formula: %s\n",sMacroName.c_str(),sExpression.c_str(),sFormula));
			return false;
		}

		// Now replace the macro with the actual numeric value
		gpstring sResult;

		// Now floor the result
		result = floorf( result );

		// Make the string. Note that because we're doing macros here, we need
		// to be sure to wrap the params (#11967). Normally I'd do this with
		// parens (as in C) but skrit has a special optimized path if the entire
		// string is a number, and parens would bypass that. So wrap in spaces
		// instead.
		sResult.assignf( " %g ", result );

		// Replace macro inline.
		sExpression.replace( start_pos, (end_pos-start_pos), sResult );

		// Scan ahead
		if( bFinished )
		{
			break;
		}
		start_pos = end_pos;
	}

	// At this point there should be no alpha characters left in the expression only
	// numbers and operators - check for illegal characters

	gpstring::size_type st = sExpression.find_first_not_of("?:.0123456789()+-*/^><=?:\t\n\r ", 0 );
	if( st != gpstring::npos )
	{
		// String is invalid so complain
		gpwarningf(("Illegal Formula '%s' evaluated to '%s'\nInvalid at character %d\n",sFormula, sExpression.c_str(), st ));
		return false;
	}

	// String shuld be valid so evaluate it.
	bool success = Skrit::EvalFloatExpression( sExpression, result );

	// Clamp result to nearest integer if it is close enough
	result = IsEqual( result, ceilf( result ), 0.001f ) ? ceilf( result ) : IsEqual( result, floorf( result ), 0.001f ) ? floorf( result ) : result;

	return success;
}

} // end namespace FormulaHelper

//////////////////////////////////////////////////////////////////////////////
