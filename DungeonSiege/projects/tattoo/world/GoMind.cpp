////////////////////////////////////////////////////////////////////////////
//
// File     :  GoMind.cpp
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//
//
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#include "Precomp_World.h"
#include "GoMind.h"

#include "AiQuery.h"
#include "Enchantment.h"

#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "Job.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoParty.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoShmo.h"
#include "GoSupport.h"
#include "GuiHelper.h"
#include "ReportSys.h"
#include "stdhelp.h"
#include "Siege_Engine.h"
#include "siege_logical_node.h"
#include "ui_gridbox.h"
#include "ui_item.h"
#include "World.h"
#include "WorldState.h"
#include "WorldTime.h"
#include "MCP.h"
#include "Sim.h"
#include "oriented_bounding_box_3.h"
#include "contentdb.h"

#include <algorithm>



using namespace std;
using namespace siege;
using namespace MCP;


DECLARE_GO_COMPONENT( GoMind, "mind" );


FUBI_EXPORT_ENUM( eJobAbstractType, JAT_BEGIN, JAT_COUNT );
FUBI_EXPORT_ENUM( eJobQ, JQ_BEGIN, JQ_COUNT );
FUBI_EXPORT_ENUM( eQPlace, QP_BEGIN, QP_COUNT );
FUBI_EXPORT_ENUM( eActorDisposition, AD_BEGIN, AD_COUNT );
FUBI_EXPORT_ENUM( eWeaponPreference, WP_BEGIN, WP_COUNT );
FUBI_EXPORT_ENUM( eMovementOrders, MO_BEGIN, MO_COUNT );
FUBI_EXPORT_ENUM( eCombatOrders, CO_BEGIN, CO_COUNT );
FUBI_EXPORT_ENUM( eFocusOrders, FO_BEGIN, FO_COUNT );
FUBI_EXPORT_ENUM( eRangeLimit, RL_BEGIN, RL_COUNT );
FUBI_EXPORT_ENUM( eDoJobMode, DJM_BEGIN, DJM_COUNT );


/////////////////////////////////////////////////////////////////////
// eCombatOrders

static const char * s_COStrings[] =
{
	"co_invalid",
	"co_free",
	"co_limited",
	"co_hold"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_COStrings ) == CO_COUNT );

static stringtool::EnumStringConverter s_COConverter( s_COStrings, CO_BEGIN, CO_END );

bool FromString( const char* str, eCombatOrders & co )
{
	return ( s_COConverter.FromString( str, rcast <int&> ( co ) ) );
}

const char* ToString( eCombatOrders co )
{
	return ( s_COConverter.ToString( co ) );
}

FUBI_DECLARE_ENUM_TRAITS( eCombatOrders, CO_INVALID );

int MakeInt( eCombatOrders co )
{
	return scast<int>( co );
}

eCombatOrders MakeCombatOrders( DWORD x )
{
	return scast<eCombatOrders>(x);
}


//////////////////////////////////////////////////////////////////////
// eMovementOrders

static const char * s_MOStrings[] =
{
	"mo_invalid",
	"mo_free",
	"mo_limited",
	"mo_hold"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_MOStrings ) == MO_COUNT );

static stringtool::EnumStringConverter s_MOConverter( s_MOStrings, MO_BEGIN, MO_END );

bool FromString( const char* str, eMovementOrders & mo )
{
	return ( s_MOConverter.FromString( str, rcast <int&> ( mo ) ) );
}

const char* ToString( eMovementOrders mo )
{
	return ( s_MOConverter.ToString( mo ) );
}

FUBI_DECLARE_ENUM_TRAITS( eMovementOrders, MO_INVALID );

int MakeInt( eMovementOrders mo )
{
	return scast<int>( mo );
}

eMovementOrders MakeMovementOrders( DWORD x )
{
	return scast<eMovementOrders>(x);
}


//////////////////////////////////////////////////////////////////////
// eFocusOrders

static const char * s_FOStrings[] =
{
	"fo_invalid",
	"fo_closest",
	"fo_farthest",
	"fo_weakest",
	"fo_strongest"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_FOStrings ) == FO_COUNT );

static stringtool::EnumStringConverter s_FOConverter( s_FOStrings, FO_BEGIN, FO_END );

bool FromString( const char* str, eFocusOrders & fo )
{
	return ( s_FOConverter.FromString( str, rcast <int&> ( fo ) ) );
}

const char* ToString( eFocusOrders fo )
{
	return ( s_FOConverter.ToString( fo ) );
}

FUBI_DECLARE_ENUM_TRAITS( eFocusOrders, FO_INVALID );


//////////////////////////////////////////////////////////////////////
// eRangeLimit

static const char * s_RLStrings[] =
{
	"rl_invalid",
	"rl_none",
	"rl_effective_attack_range",
	"rl_effective_attack_response_range",
	"rl_engage_range",
	"rl_sight_range",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_RLStrings ) == RL_COUNT );

static stringtool::EnumStringConverter s_RLConverter( s_RLStrings, RL_BEGIN, RL_END );

bool FromString( const char* str, eRangeLimit & val )
{
	return ( s_RLConverter.FromString( str, rcast <int&> ( val ) ) );
}

const char* ToString( eRangeLimit val )
{
	return ( s_RLConverter.ToString( val ) );
}

FUBI_DECLARE_ENUM_TRAITS( eRangeLimit, RL_INVALID );


//////////////////////////////////////////////////////////////////////
// eQPlace

static const char * s_QPlaceStrings[] =
{
	"qp_invalid",
	"qp_none",
	"qp_front",
	"qp_unused1",
	"qp_back",
	"qp_clear"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_QPlaceStrings ) == QP_COUNT );

//FUBI_EXPORT_ENUM( eQPlace, QP_BEGIN, QP_COUNT );

static stringtool::EnumStringConverter s_QPlaceConverter( s_QPlaceStrings, QP_BEGIN, QP_END );

bool FromString( const char* str, eQPlace & qp )
{
	return ( s_QPlaceConverter.FromString( str, rcast <int&> ( qp ) ) );
}

const char* ToString( eQPlace qp )
{
	return ( s_QPlaceConverter.ToString( qp ) );
}


///////////////////////////////////////////////////////////////////////
// eJobAbstractType

struct JatEntry
{
	const char *		m_Name;
	bool        		m_Traits[ 23 ];
};

static JatEntry s_JatTraits[] =
{

/*
                                        R  R  R  R  U  G  D  E  E  E  A  A  A  A  H  O  D  C  W  M  U  R  A
                                        E  E  E  E  N  H  E  N  N  N  L  L  L  U  U  F  E  I  A  O  S  E  L
                                        Q  Q  Q  Q  C  O  A  G  G  G  L  L  L  T  M  F  F  R  T  V  E  P  W
                                        .  .  .  .  O  S  D  D  D  D  O  O  O  O  A  E  E  C  C  E  .  E  A
                                        G  M  P  S  N  T  .  .  .  .  W  W  W  .  N  N  N  U  H  S  S  A  Y
                                        O  O  O  L  S  .  O  U  G  D  .  .  .  I  .  S  S  L  .  .  P  T  S
                                        A  D  S  O  C  O  K  N  H  E  R  R  R  N  I  I  I  A  .  A  A  I  .
                                        L  .  .  T  .  K  .  C  O  A  E  E  E  T  N  V  V  R  G  C  T  N  C
                                        .  .  .  .  O  .  .  O  S  D  F  F  F  E  T  E  E     O  T  I  G  R
                                        .  .  .  .  K  .  .  N  T  .  L  L  L  R  E  .  .  A  A  O  A  .  T
                                        .  .  .  .  .  .  .  S  .  O  X  X  X  R  R  .  .  S  L  R  L  .  .
                                        .  .  .  .  .  .  .  C  O  K  .  .  .  U  R  .  .  G  .  .  .  .  I
                                        .  .  .  .  .  .  .  .  K  .  G  M  P  P  U  .  .  N  M  .  S  .  C
                                        .  .  .  .  .  .  .  O  .  .  O  O  O  T  P  .  .     S  .  E  .  O
                                        .  .  .  .  .  .  .  K  .  .  A  D  S  .  T  .  .  O  G  .  N  .  N
                                        .  .  .  .  .  .  .  .  .  .  L  .  .  .  .  .  .  K  .  .  S  .  .
                                        .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R  .  .
*/

    { "jat_invalid"					, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_none"					, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_attack_object"			, { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_attack_object_melee"		, { 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0 }, },
    { "jat_attack_object_ranged"	, { 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0 }, },
    { "jat_attack_position"			, { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1 }, },
    { "jat_attack_position_melee"	, { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1 }, },
    { "jat_attack_position_ranged"	, { 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1 }, },

    { "jat_brain"					, { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },

	{ "jat_cast"					, { 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0 }, },
    { "jat_cast_position"			, { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_die"						, { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_do_se_command"			, { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0 }, },
    { "jat_drink"					, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_drop"					, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_equip"					, { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_face"					, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_fidget"					, { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0 }, },
    { "jat_flee_from_object"		, { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0 }, },
    { "jat_follow"					, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0 }, },
    { "jat_gain_consciousness"		, { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_get"						, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_give"					, { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_guard"					, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0 }, },
    { "jat_listen"					, { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0 }, },
    { "jat_move"					, { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_patrol"					, { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0 }, },
    { "jat_play_anim"				, { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_startup"					, { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, },
    { "jat_stop"					, { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_talk"					, { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
    { "jat_unconscious"				, { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, },
    { "jat_use"						, { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, },
};


FUBI_DECLARE_ENUM_TRAITS( eJobAbstractType, JAT_INVALID );

COMPILER_ASSERT( ELEMENT_COUNT( s_JatTraits ) == JAT_COUNT );

stringtool::EnumStringConverter & GetJatConverter( void )
{
	static const char* s_Strings[ WE_COUNT ];

	static bool s_StringsBuilt = false;
	if ( !s_StringsBuilt )
	{
		s_StringsBuilt = true;

		const JatEntry* i, * ibegin = s_JatTraits, * iend = ARRAY_END( s_JatTraits );
		const char** out = s_Strings;
		for ( i = ibegin ; i != iend ; ++i, ++out )
		{
			*out = i->m_Name;
		}
	}

	static stringtool::EnumStringConverter s_Converter( s_Strings, JAT_BEGIN, JAT_END, JAT_INVALID );
	return ( s_Converter );
}

const char* ToString( eJobAbstractType val )
{
	return ( GetJatConverter().ToString( val ) );
}

bool FromString( const char* str, eJobAbstractType & val )
{
	return ( GetJatConverter().FromString( str, rcast <int&> ( val ) ) );
}


bool IsSpeedLimitedJat( eJobAbstractType jat )
{
	switch( jat )
	{
		case JAT_ATTACK_OBJECT:
		case JAT_ATTACK_OBJECT_MELEE:
		case JAT_ATTACK_OBJECT_RANGED:
		case JAT_ATTACK_POSITION:
		case JAT_ATTACK_POSITION_MELEE:
		case JAT_ATTACK_POSITION_RANGED:
		case JAT_CAST:
		case JAT_CAST_POSITION:
		{
			return(true);
		}
		break;

		default:
		{
			return(false);
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////////
// enum eWorldEventTraits implementation

eJobTraits GetTraits( eJobAbstractType e )
{
	static eJobTraits s_Traits[ JAT_COUNT ];

	static bool s_TraitsBuilt = false;
	if ( !s_TraitsBuilt )
	{
		s_TraitsBuilt = true;

		const JatEntry* i, * ibegin = s_JatTraits, * iend = ARRAY_END( s_JatTraits );
		eJobTraits* out = s_Traits;
		for ( i = ibegin ; i != iend ; ++i, ++out )
		{
			*out = JT_NONE;

			const bool* j, * jbegin = i->m_Traits, * jend = ARRAY_END( i->m_Traits );
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( *j )
				{
					*out |= (eJobTraits)(1 << (j - jbegin));
				}
			}
		}
	}

	return ( s_Traits[ e ] );
}

bool TestJobTraits( eWorldEvent e, eWorldEventTraits t )
{
	return ( (GetWorldEventTraits( e ) & t) != 0 );
}

bool TestJobTraitsEq( eWorldEvent e, eWorldEventTraits t )
{
	return ( (GetWorldEventTraits( e ) & t) == t );
}


///////////////////////////////////////////////////////////////////////
// eJobQ

static const char* s_JQStrings[] =
{
	"jq_brain",
	"jq_action"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_JQStrings ) == JQ_COUNT );

static stringtool::EnumStringConverter s_JQConverter( s_JQStrings, JQ_BEGIN, JQ_END );

bool FromString( const char* str, eJobQ & jq )
{
	return ( s_JQConverter.FromString( str, rcast <int&> ( jq ) ) );
}

const char* ToString( eJobQ jq )
{
	return ( s_JQConverter.ToString( jq ) );
}


///////////////////////////////////////////////////////////////////////
// eActorDisposition

static const char* s_ADStrings[] =
{
	"ad_invalid",
	"ad_offensive",
	"ad_defensive",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ADStrings ) == AD_COUNT );

static stringtool::EnumStringConverter s_ADConverter( s_ADStrings, AD_BEGIN, AD_END );

bool FromString( const char* str, eActorDisposition & ad )
{
	return ( s_ADConverter.FromString( str, rcast <int&> ( ad ) ) );
}

const char* ToString( eActorDisposition ad )
{
	return ( s_ADConverter.ToString( ad ) );
}

FUBI_DECLARE_ENUM_TRAITS( eActorDisposition, AD_INVALID );


////////////////////////////////////////////////////////////////////////////////
//	eWeaponPreference

static const char* s_WPStrings[] =
{
	"wp_invalid",
	"wp_none",
	"wp_karate",
	"wp_melee",
	"wp_ranged",
	"wp_magic",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_WPStrings ) == WP_COUNT );

static stringtool::EnumStringConverter s_WPConverter( s_WPStrings, WP_BEGIN, WP_END );

bool FromString( const char* str, eWeaponPreference & value )
{
	return ( s_WPConverter.FromString( str, rcast <int&> ( value ) ) );
}

const char* ToString( eWeaponPreference value )
{
	return ( s_WPConverter.ToString( value ) );
}

FUBI_DECLARE_ENUM_TRAITS( eWeaponPreference, WP_INVALID );


/////////////////////////////////////////////////////////////
// eComponentCommandMode

static const char* s_DJMStrings[] =
{
	"djm_cv1",
	"djm_1v2",
	"djm_1vc",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_DJMStrings ) == DJM_COUNT );

static stringtool::EnumStringConverter s_DJMConverter( s_DJMStrings, DJM_BEGIN, DJM_END );

bool FromString( const char* str, eDoJobMode & x )
{
	return ( s_DJMConverter.FromString( str, rcast <int&> ( x ) ) );
}

const char* ToString( eDoJobMode x )
{
	return ( s_DJMConverter.ToString( x ) );
}

FUBI_DECLARE_ENUM_TRAITS( eDoJobMode, DJM_BEGIN );


////////////////////////////////////////////////////////////////////////////////
//	job result
JobResult :: JobResult( Job * job )
{
	ZeroObject( *this );

	m_TimeFinished	= gWorldTime.GetTime();
	m_Result		= job->GetJobResult();
	m_Count			= 1;
	m_Jat			= job->GetJobAbstractType();
	m_Traits		= job->GetTraits();
	m_Origin		= job->GetOrigin();
	m_GoalObject	= job->GetGoalObject();
	m_GoalModifier	= job->GetGoalModifier();
	m_GoalPosition	= job->GetGoalPosition();
}




////////////////////////////////////////////////////////////////////////////////
//	done for CC speed - revisit and integrate into master Job traits table or something like that
// $$$ localize
/*
	$$$ english names in waypoint table
	$$$ AND localizable names
*/
const char * GetJobWaypointName( eJobAbstractType jat )
{
	switch( jat )
	{
		case JAT_ATTACK_OBJECT_MELEE:
		case JAT_ATTACK_POSITION_MELEE:
		{
			return( "melee attack" );
			break;
		}
		case JAT_ATTACK_OBJECT_RANGED:
		case JAT_ATTACK_POSITION_RANGED:
		{
			return( "ranged attack" );
			break;
		}
		case JAT_ATTACK_POSITION:
		case JAT_ATTACK_OBJECT:
		{
			return( "attack" );
			break;
		}
		case JAT_CAST:
		case JAT_CAST_POSITION:
		{
			return( "cast" );
			break;
		}
		case JAT_DIE:
		{
			return( "die" );
			break;
		}
		case JAT_DO_SE_COMMAND:
		{
			return( "do editor command" );
			break;
		}
		case JAT_DRINK:
		{
			return( "drink" );
			break;
		}
		case JAT_DROP:
		{
			return( "drop" );
			break;
		}
		case JAT_EQUIP:
		{
			return( "equip" );
			break;
		}
		case JAT_FACE:
		{
			return( "face" );
			break;
		}
		case JAT_FIDGET:
		{
			return( "fidget" );
			break;
		}
		case JAT_FLEE_FROM_OBJECT:
		{
			return( "flee" );
			break;
		}
		case JAT_FOLLOW:
		{
			return( "follow" );
			break;
		}
		case JAT_GAIN_CONSCIOUSNESS:
		{
			return( "gain consciousness" );
			break;
		}
		case JAT_GET:
		{
			return( "get" );
			break;
		}
		case JAT_GIVE:
		{
			return( "give" );
			break;
		}
		case JAT_GUARD:
		{
			return( "guard" );
			break;
		}
		case JAT_LISTEN:
		{
			return( "listen" );
			break;
		}
		case JAT_MOVE:
		{
			return( "move" );
			break;
		}
		case JAT_PATROL:
		{
			return( "patrol" );
			break;
		}
		case JAT_PLAY_ANIM:
		{
			return( "play anim" );
			break;
		}
		case JAT_STOP:
		{
			return( "stop" );
			break;
		}
		case JAT_TALK:
		{
			return( "talk" );
			break;
		}
		case JAT_USE:
		{
			return( "use" );
			break;
		}
		default:
		{
			gpassertm( 0, "Unrecognized enum" );
			return( "" );
			break;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// class GoMind implementation

GoMind::eOptions GoMind::m_Options = GoMind::OPTION_UPDATE_ALL;

GoMind :: GoMind( Go * parent )
	: Inherited( parent )
{

	////////////////////////////////////////////////////////////////////////////////
	//	state
	//

	////////////////////////////////////////
	//	critical flags
	m_ShuttingDown							= false;
	m_BrainActive							= false;
	m_FrustumMembershipChanged				= false;
	m_GiveInitialSECommand			  		= false;
	m_InQueueDeletionScope			  		= false;
	m_InUpdateScope					  		= false;
	m_ReqDoLastSECommand					= false;
	m_ReqGiveStartupJob						= false;
	m_ReqInitBrain							= true;

	////////////////////////////////////////
	//	timers
	m_IdleTimer						  		= 0;
	m_IdleTimerTrigger				  		= 5.0f;
	m_SensorScanPeriod				  		= 0.5f;
	m_NextSensorScanTime			  		= 0;
	m_NextTimeAttackAllowed					= 0;
	m_NextTimeCastAllowed					= 0;

	////////////////////////////////////////
	//	misc
	m_InitialSECommand				  		= SCID_INVALID;
	m_LastSECommand							= SCID_INVALID;
	m_LastSECommandPathFailureTime			= 0;
	m_pHandlingMessage						= NULL;
	m_HandleMessageEntryCount				= 0;
	m_Rank									= 0;
	m_bEnemySpottedPlayed					= false;
	m_AllignedTeam							= -1;

	////////////////////////////////////////
	//	jobs
	m_LastActionProcessedId			  		= 0;
	m_LastJobReachedTravelDistanceId		= 0;
	m_LastCurrentBaseJat					= JAT_NONE;
	m_ActionResultMemoryLength				= 1.5;
	m_LastElevatorRideTime					= 0;
	m_TimeEnemySpottedLast					= 0;
	m_JobTravelDistanceLimit				= 0;
	m_LimitedMovementRange					= 0;

	// orders
	m_DispositionOrders 			  		= AD_OFFENSIVE;
	m_MovementOrders				  		= MO_FREE;
	m_CombatOrders					  		= CO_FREE;
	m_FocusOrders					  		= FO_CLOSEST;

	// requests
	m_RequestProcessHitEngaged		  		= false;
	m_RequestProcessCombatSensors	  		= false;

	////////////////////////////////////////
	//	sensors
	m_RequestProcessCombatSensors	  		= 0;
	m_RequestProcessHitEngaged		  		= 0;
	m_ActorLifeRatioLast			  		= 100.0f;
	m_ActorManaRatioLast			  		= 100.0f;
	m_MyPositionLastVisibilityCheck			= SiegePos::INVALID;

	m_AliveFriendsVisibleDirty				= true;
	m_AliveFriendsVisible					= false;
	m_AliveEnemiesVisibleDirty				= true;
	m_AliveEnemiesVisible					= false;

	m_ClientCachedEngagedObject				= GOID_INVALID;

	m_TempPos1								= SiegePos::INVALID;
	m_TempVector1							= vector_3::INVALID;
	m_FrustumOnEntry						= MakeFrustumId( 0 );

	//------ spec


	////////////////////////////////////////////////////////////////////////////////
	//	job permissions

	m_ActorAutoFidgets								= false;
	m_MayProcessAI									= true;
	m_MayAttack										= true;
	m_MayBeAttacked									= true;

	// general permissions
	m_ActorAutoDefendsOthers                        = false;
	m_ActorAutoHealsOthersLife                      = false;
	m_ActorAutoHealsOthersMana                      = false;
	m_ActorAutoHealsSelfLife                        = false;
	m_ActorAutoHealsSelfMana                        = false;
	m_ActorAutoPicksUpItems                         = false;
	m_ActorAutoXfersMana                            = false;
	m_ActorAutoReanimatesFriends					= false;

	m_WeaponPreference								= WP_INVALID;

	m_ActorAutoSwitchesToKarate				   		= false;
	m_ActorAutoSwitchesToMelee 				   		= false;
	m_ActorAutoSwitchesToRanged				   		= false;
	m_ActorAutoSwitchesToMagic 				   		= false;

	m_ActorAutoUsesStoredItems				   		= false;

	m_ActorBalancedAttackPreference					= 1.0;

	// specific event permissions
	m_OnEnemySpottedAlertFriends					= false;
	m_OnEnemySpottedAttack							= false;
	m_OnEnemySpottedFlee							= false;

	m_OnAlertProjectileNearMissedAttack				= false;
	m_OnAlertProjectileNearMissedFlee				= false;
	m_OnEnemyEnteredICZAttack						= false;
	m_OnEnemyEnteredICZFlee							= false;
	m_OnEnemyEnteredICZSwitchToMelee				= false;
	m_OnEnemyEnteredOCZAttack						= false;
	m_OnEnemyEnteredOCZFlee							= false;

	m_OnFriendEnteredICZAttack						= false;
	m_OnFriendEnteredICZFlee						= false;
	m_OnFriendEnteredOCZAttack						= false;
	m_OnFriendEnteredOCZFlee						= false;

	m_OnLifeRatioLowFlee							= false;
	m_OnManaRatioLowFlee							= false;

#	if !GP_RETAIL
	m_DebugWeKilledReceived							= false;
	m_DebugWeLostConsciousnessReceived				= false;
	m_DebugWeEngagedLostConsciousnessRecevied		= false;
#	endif

	m_OnJobReachedTravelDistanceAbortAttack			= false;

	m_OnEngagedLostLoiter							= false;
	m_OnEngagedLostReturnToJobOrigin				= false;
	m_OnEngagedFledAbortAttack						= false;
	m_OnEngagedLostConsciousnessAbortAttack			= false;


	///////////////////////////////////////
	//	sensor parameters

	// sensor response
	m_ActorLifeRatioLowThreshold						= 0.3f;
	m_ActorManaRatioLowThreshold						= 0.3f;
	m_ActorLifeRatioHighThreshold						= 0.8f;
	m_ActorManaRatioHighThreshold						= 0.8f;

	// actor
	m_PersonalSpaceRange								= 1.0f;
	m_MeleeEngageRange									= 6.0f;
	m_RangedEngageRange									= 10.0f;
	m_SightFov											= 180.0f;
	m_SightRange										= 15.0f;
	m_SightOriginHeight									= 0;
	m_VisibilityMemoryDuration							= 3.0f;
	m_InnerComfortZoneRange								= 0;
	m_OuterComfortZoneRange								= 0;

	m_FleeDistance										= 0;
	m_FleeCount											= 0;

	m_MinComfortDelayThreshold							= 3.0;
	m_MaxComfortDelayThreshold							= 6.0;

	m_ComRange											= 0;

	m_FrustumOnEntry									= 0;
}


GoMind :: GoMind( const GoMind& source, Go* parent )
	: Inherited( source, parent )
{
	//----- state

	m_pHandlingMessage									= NULL;

	m_IdleTimer											= 0;
	m_IdleTimerTrigger									= source.m_IdleTimerTrigger;
	m_LastActionProcessedId			  					= 0;
	m_LastJobReachedTravelDistanceId					= 0;
	m_LastCurrentBaseJat								= JAT_NONE;
	m_ActionResultMemoryLength							= source.m_ActionResultMemoryLength;

	////////////////////////////////////////
	//	timers
	m_IdleTimer						  					= 0;
	m_IdleTimerTrigger				  					= 5.0f;
	m_SensorScanPeriod				  					= 0.5f;
	m_NextSensorScanTime			  					= 0;
	m_NextTimeAttackAllowed								= 0;
	m_NextTimeCastAllowed								= 0;
	m_TimeEnemySpottedLast								= 0;
	m_LastElevatorRideTime								= 0;

	// orders
	m_CombatOrders                          			= source.m_CombatOrders;
	m_MovementOrders                        			= source.m_MovementOrders;
	m_FocusOrders                           			= source.m_FocusOrders;
	m_DispositionOrders									= source.m_DispositionOrders;

	// requests
	m_RequestProcessHitEngaged							= false;
	m_RequestProcessCombatSensors						= false;
	m_RequestFlushSensors								= false;
	m_InUpdateScope										= false;
	m_InQueueDeletionScope								= false;
	m_ReqInitBrain										= true;
	m_ReqDoLastSECommand								= false;
	m_ReqGiveStartupJob									= source.m_ReqGiveStartupJob;

	m_HandleMessageEntryCount							= 0;
	m_Rank												= 0;
	m_bEnemySpottedPlayed								= false;
	m_AllignedTeam										= source.m_AllignedTeam;

	// sensors
	m_NextSensorScanTime								= source.m_NextSensorScanTime;
	m_RequestProcessCombatSensors						= source.m_RequestProcessCombatSensors;
	m_RequestProcessHitEngaged							= source.m_RequestProcessHitEngaged;
	m_ActorLifeRatioLast								= source.m_ActorLifeRatioLast;
	m_ActorManaRatioLast								= source.m_ActorManaRatioLast;
	m_MyPositionLastVisibilityCheck						= SiegePos::INVALID;

	m_ClientCachedEngagedObject							= GOID_INVALID;

	// actor
	m_EngagedMeMap.clear();

	//------ spec

	// job-related stuff

	gpassert( m_JobLookupEntryColl.empty() );
	for( JobLookupEntryColl::const_iterator ij = source.m_JobLookupEntryColl.begin(); ij != source.m_JobLookupEntryColl.end(); ++ij )
	{
		gAIAction.RegisterReference( (*ij).m_pSlot );
		m_JobLookupEntryColl.push_back( (*ij) );
	}

	m_SensorScanPeriod									= source.m_SensorScanPeriod;
	m_InitialSECommand									= source.m_InitialSECommand;
	m_GiveInitialSECommand								= source.m_GiveInitialSECommand;
	m_LastSECommand										= SCID_INVALID;
	m_LastSECommandPathFailureTime						= 0;
	m_BrainActive										= source.m_BrainActive;

	////////////////////////////////////////////////////////////////////////////////
	//	job permissions

	// auto permissions

	m_ShuttingDown										= 0;

	m_ActorAutoFidgets									= source.m_ActorAutoFidgets;
	m_MayProcessAI										= source.m_MayProcessAI;
	m_MayAttack											= source.m_MayAttack;
	m_MayBeAttacked										= source.m_MayBeAttacked;

	m_ActorAutoDefendsOthers            		  		= source.m_ActorAutoDefendsOthers;
	m_ActorAutoHealsOthersLife          		  		= source.m_ActorAutoHealsOthersLife;
	m_ActorAutoHealsOthersMana          		  		= source.m_ActorAutoHealsOthersMana;
	m_ActorAutoHealsSelfLife            		  		= source.m_ActorAutoHealsSelfLife;
	m_ActorAutoHealsSelfMana            		  		= source.m_ActorAutoHealsSelfMana;
	m_ActorAutoPicksUpItems             		  		= source.m_ActorAutoPicksUpItems;
	m_ActorAutoXfersMana                		  		= source.m_ActorAutoXfersMana;
	m_ActorAutoReanimatesFriends						= source.m_ActorAutoReanimatesFriends;

	m_WeaponPreference	         						= source.m_WeaponPreference;

	m_ActorAutoSwitchesToKarate 						= source.m_ActorAutoSwitchesToKarate;
	m_ActorAutoSwitchesToMelee  						= source.m_ActorAutoSwitchesToMelee;
	m_ActorAutoSwitchesToRanged 						= source.m_ActorAutoSwitchesToRanged;
	m_ActorAutoSwitchesToMagic  						= source.m_ActorAutoSwitchesToMagic;

	m_ActorAutoUsesStoredItems  						= source.m_ActorAutoUsesStoredItems;

	m_ActorBalancedAttackPreference						= source.m_ActorBalancedAttackPreference;

	// specific event reflex permissions
	m_OnEnemySpottedAlertFriends		 		  		= source.m_OnEnemySpottedAlertFriends;
	m_OnEnemySpottedAttack				 		  		= source.m_OnEnemySpottedAttack;
	m_OnEnemySpottedFlee				 		  		= source.m_OnEnemySpottedFlee;

	m_OnAlertProjectileNearMissedAttack					= source.m_OnAlertProjectileNearMissedAttack;
	m_OnAlertProjectileNearMissedFlee					= source.m_OnAlertProjectileNearMissedFlee;
	m_OnEnemyEnteredICZAttack			 		  		= source.m_OnEnemyEnteredICZAttack;
	m_OnEnemyEnteredICZFlee			 			  		= source.m_OnEnemyEnteredICZFlee;
	m_OnEnemyEnteredICZSwitchToMelee					= source.m_OnEnemyEnteredICZSwitchToMelee;
	m_OnEnemyEnteredOCZAttack			 		  		= source.m_OnEnemyEnteredOCZAttack;
	m_OnEnemyEnteredOCZFlee			 			  		= source.m_OnEnemyEnteredOCZFlee;

	m_OnFriendEnteredICZAttack			 		  		= source.m_OnFriendEnteredICZAttack;
	m_OnFriendEnteredICZFlee			 		  		= source.m_OnFriendEnteredICZFlee;
	m_OnFriendEnteredOCZAttack			 		  		= source.m_OnFriendEnteredOCZAttack;
	m_OnFriendEnteredOCZFlee			 		  		= source.m_OnFriendEnteredOCZFlee;

	m_OnLifeRatioLowFlee				 		  		= source.m_OnLifeRatioLowFlee;
	m_OnManaRatioLowFlee				 		  		= source.m_OnManaRatioLowFlee;

	m_OnJobReachedTravelDistanceAbortAttack		  		= source.m_OnJobReachedTravelDistanceAbortAttack;

	m_OnEngagedLostLoiter				 		  		= source.m_OnEngagedLostLoiter;
	m_OnEngagedLostReturnToJobOrigin	 		  		= source.m_OnEngagedLostReturnToJobOrigin;
	m_OnEngagedFledAbortAttack			 		  		= source.m_OnEngagedFledAbortAttack;
	m_OnEngagedLostConsciousnessAbortAttack				= source.m_OnEngagedLostConsciousnessAbortAttack;


	// sensor ranges
	m_PersonalSpaceRange								= source.m_PersonalSpaceRange;
	m_MeleeEngageRange									= source.m_MeleeEngageRange;
	m_RangedEngageRange									= source.m_RangedEngageRange;
	m_SightFov											= source.m_SightFov;
	m_SightRange										= source.m_SightRange;
	m_SightOriginHeight									= source.m_SightOriginHeight;
	m_VisibilityMemoryDuration							= source.m_VisibilityMemoryDuration;
	m_InnerComfortZoneRange								= source.m_InnerComfortZoneRange;
	m_OuterComfortZoneRange								= source.m_OuterComfortZoneRange;
	m_JobTravelDistanceLimit							= source.m_JobTravelDistanceLimit;
	m_LimitedMovementRange								= source.m_LimitedMovementRange;

	m_ActorLifeRatioLowThreshold						= source.m_ActorLifeRatioLowThreshold;
	m_ActorManaRatioLowThreshold						= source.m_ActorManaRatioLowThreshold;
	m_ActorLifeRatioHighThreshold						= source.m_ActorLifeRatioHighThreshold;
	m_ActorManaRatioHighThreshold						= source.m_ActorManaRatioHighThreshold;

	m_FleeDistance										= source.m_FleeDistance;
	m_FleeCount											= source.m_FleeCount;
	m_MinComfortDelayThreshold							= source.m_MinComfortDelayThreshold;
	m_MaxComfortDelayThreshold							= source.m_MaxComfortDelayThreshold;

	m_ComChannels										= source.m_ComChannels;
	m_ComRange											= source.m_ComRange;

	m_FrustumOnEntry									= 0;
}


GoMind :: ~GoMind( void )
{
	for( JobQueue::iterator ia = m_Actions.begin(); ia != m_Actions.end(); ++ia )
	{
		(*ia)->Deinit( true );
		delete (*ia);
	}
	m_Actions.clear();

	for( JobQueue::iterator ib = m_Brains.begin(); ib != m_Brains.end(); ++ib )
	{
		(*ib)->Deinit( true );
		delete (*ib);
	}
	m_Brains.clear();

	for( JobQueue::iterator id = m_Deletion.begin(); id != m_Deletion.end(); ++id )
	{
		delete (*id);
	}
	m_Deletion.clear();

	gpassert( m_Actions.empty()		);
	gpassert( m_Brains.empty()		);
	gpassert( m_Deletion.empty()	);

	for( DistanceVisibleMap::iterator idv = m_DistanceVisibleMap.begin(); idv != m_DistanceVisibleMap.end(); ++idv )
	{
		delete (*idv).second;
	}

	m_DistanceVisibleMap.clear();
	m_GoidVisibleMap.clear();

	for( JobLookupEntryColl::const_iterator ij = m_JobLookupEntryColl.begin(); ij != m_JobLookupEntryColl.end(); ++ij )
	{
		gAIAction.UnregisterReference( (*ij).m_pSlot );
	}
	m_JobLookupEntryColl.clear();


	for( JobResultColl::iterator ir = m_ActionResultColl.begin(); ir != m_ActionResultColl.end(); ++ir )
	{
		delete *ir;
	}
	m_ActionResultColl.clear();
}


void GoMind :: HandleMessage( WorldMessage const & msg )
{
	GPPROFILERSAMPLE( "GoMind :: HandleMessage", SP_AI | SP_HANDLE_MESSAGE );

	gpassertm( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC, "This Go shouldn't get messages." );
	gpassertm( GoHandle( GetGoid() ), "Parent go is not valid?" );

	if( !TestWorldEventTraits( msg.GetEvent(), WMT_AI_WANTS ) )
	{
		return;
	}

	// special handling for this one - use it to resolve pointers
	if ( msg.HasEvent( WE_POST_RESTORE_GAME ) && !msg.IsCC() )
	{
		// $ we have to do this post-restore stuff here because only now do we
		//   have all of the go's restored and are able to reinit pointers for
		//   go's and components other than this one for the jobs.

		JobQueue::iterator i;
		for ( i = m_Brains.begin() ; i != m_Brains.end() ; ++i )
		{
			(*i)->InitPointers();
		}
		for ( i = m_Actions.begin() ; i != m_Actions.end() ; ++i )
		{
			(*i)->InitPointers();
		}

		return;
	}

	if( m_HandleMessageEntryCount > 0 )
	{
		m_DelayedMessages.push_back( msg );
	}
	else
	{
		++m_HandleMessageEntryCount;

		PrivateHandleMessage( msg );

		if( !m_DelayedMessages.empty() )
		{
			// $ prevent change-while-iterating problems by working from a local coll

			WorldMessageColl messages;
			messages.swap( m_DelayedMessages );

			WorldMessageColl::iterator i, ibegin = messages.begin(), iend = messages.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				PrivateHandleMessage( *i );
			}
		}

		--m_HandleMessageEntryCount;
	}
}


void GoMind :: HandleCcMessage( WorldMessage const & Message )
{
	// if/else's for cc status are inside main handler already
	HandleMessage( Message );
}


// for now only update and draw the MOVE Queue
void GoMind :: Update( float /*secondsElapsed*/ )
{
	GPPROFILERSAMPLE( "GoMind :: Update", SP_AI | SP_AI_MIND );

	if( GetGo()->IsOmni() )
	{
		return;
	}

	gpassert( GetGo()->IsInActiveWorldFrustum() );
	gpassertm( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC, "This Go shouldn't get an update." );

	m_InUpdateScope = true;

#	if !GP_RETAIL
	Validate();
#	endif

	////////////////////////////////////////////////////////////////////////////////
	//	client-side only sensor updates for visibility - for human controlled actors only

	if( gServer.IsRemote() )
	{
		if( GetGo()->IsActor() && ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) )
		{
			if( m_NextSensorScanTime <= gWorldTime.GetTime() )
			{
				m_NextSensorScanTime = PreciseAdd( gWorldTime.GetTime(), (m_SensorScanPeriod + Random( 0.02f )) );
				ProcessSpatialSensors();
			}
		}
		m_InUpdateScope = false;
		return;
	}

	CHECK_SERVER_ONLY;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	special - draw action Q icons here because it may involve creation of Gos...  can't do that in Draw() scope

	if( GetGo()->IsScreenPartyMember() )
	{
		bool drawWaypoint = TestOptions( OPTION_DRAW_HUMAN_WAYPOINTS ) && GetGo()->IsSelected();
#if		!GP_RETAIL
		drawWaypoint = drawWaypoint || GetGo()->IsHuded();
#endif
		for( JobQueue::iterator i = m_Actions.begin(); i != m_Actions.end(); ++i )
		{
			if( drawWaypoint )
			{
				(*i)->DrawWaypointIcon();
			}
			else
			{
				(*i)->HideWaypointIcon();
			}
		}
	}

	/////////////////////////////////////////////////////////
	//	init the brain if requested

	if( m_ReqInitBrain )
	{
		if( ( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC ) && UnderstandsJob( JAT_BRAIN ) )
		{
			SDoJob( MakeJobReq( JAT_BRAIN, JQ_BRAIN, QP_CLEAR, AO_REFLEX ) );
		}
		m_ReqInitBrain = false;
	}

	if( m_ReqDoLastSECommand )
	{
		// retry every 2 seconds to do last SE command it failed last time
		if( PreciseSubtract( gWorldTime.GetTime(), m_LastSECommandPathFailureTime ) > 2.0 )
		{
			gpassert( m_LastSECommand != SCID_INVALID );

			Goid commandGoid = gGoDb.FindGoidByScid( m_LastSECommand );
			GoHandle command( commandGoid );
			if( command && CanOperateOn( command ) )
			{
				JobReq & req = MakeJobReq( JAT_DO_SE_COMMAND, JQ_ACTION, QP_CLEAR, AO_COMMAND );
				req.SetInt1( MakeInt( m_LastSECommand ) );
				SDoJob( req );
				m_ReqDoLastSECommand = false;
			}
		}
	}

	/////////////////////////////////////////////////////////
	//	early-out based on options

	if( !m_MayProcessAI )
	{
		m_InUpdateScope = false;
		return;
	}
	
	if( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && !TestOptions( OPTION_UPDATE_COMPUTER ) )
	{
		m_InUpdateScope = false;
		return;
	}

	if( ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) && !TestOptions( OPTION_UPDATE_HUMAN ) )
	{
		m_InUpdateScope = false;
		return;
	}

	bool allowBrainUpdate = ( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && TestOptions( OPTION_UPDATE_COMPUTER_BRAINS ) ) ||
							( ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) && TestOptions( OPTION_UPDATE_HUMAN_BRAINS ) );

	bool allowActionUpdate =	( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && TestOptions( OPTION_UPDATE_COMPUTER_ACTIONS ) ) ||
								( ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) && TestOptions( OPTION_UPDATE_HUMAN_ACTIONS ) );

	if( !allowBrainUpdate && !allowActionUpdate )
	{
		m_InUpdateScope = false;
		return;
	}

#if !GP_RETAIL
	bool oldShow = gWorldOptions.GetShowSpatialQueries();
	gWorldOptions.SetShowSpatialQueries( GetGo()->IsHuded() );
#endif

	/////////////////////////////////////////////////////////
	//	begin update

	MoveMarkedJobsToDeletionQ( false );
	ProcessDeletionQueue();
	ClearTemps();

	Job * brain = GetFrontJob( JQ_BRAIN );

	bool bActionUpdated = false;


	/////////////////////////////////////////////////////////
	//	update brain

	if( brain && allowBrainUpdate )
	{
		if( !brain->IsInitialized() )
		{
			brain->Init();
		}
		brain->Update();
		if( brain->IsMarkedForDeletion() )
		{
			brain->Deinit();
		}

		////////////////////////////////////////
		//	generate WE_MIND_PROCESSING_NEW_JOB

		Job * action = GetFrontJob( JQ_ACTION );
		if( action )
		{
			if( m_LastActionProcessedId != action->GetId() )
			{
				m_LastActionProcessedId = action->GetId();
				Job * brain = GetFrontJob( JQ_BRAIN );
				if( brain )
				{
					Send( WorldMessage( WE_MIND_PROCESSING_NEW_JOB, GOID_INVALID, GetGo()->GetGoid() ) );
				}
			}
		}
	}


	/////////////////////////////////////////////////////////
	//	update current action

	if( allowActionUpdate )
	{
		if( GetFrontJob( JQ_ACTION ) )
		{
			ExamineEngaged();

			Job * action = GetFrontJob( JQ_ACTION );
			if( action )
			{
				if( action->InAtomicState() || MaySDoJob( action->GetJobAbstractType() ) )
				{
					if( !action->IsInitialized() )
					{
						action->Init();
					}

					action->Update();
					if( action->IsMarkedForDeletion() )
					{
						action->Deinit();
					}
					PostJobAnyEvent( action );

					bActionUpdated	= true;

					if( m_RequestProcessHitEngaged )
					{
						ProcessRequestHitEngaged();
					}
				}
			}
		}
		else if( GetGo()->HasAspect() )
		{
			eLifeState ls =  GetGo()->GetAspect()->GetLifeState();
			if( IsConscious( ls ) || IsGhost( ls ) || (ls == LS_IGNORE) )
			{
				// start a fidget
				if( UnderstandsJob( JAT_FIDGET ) && m_ActorAutoFidgets )
				{
					SDoJob( MakeJobReq( JAT_FIDGET, JQ_ACTION, QP_FRONT, AO_REFLEX ) );
				}

				// no actions in Q, so update timeout vars
				if( m_IdleTimer <= m_IdleTimerTrigger  )
				{
					m_IdleTimer += float( gWorldTime.GetSecondsElapsed() );
					if( m_IdleTimer > m_IdleTimerTrigger )
					{
						// tell the brain that we're idling... give it a chance to do something about it
						Send( WorldMessage( WE_MIND_ACTION_Q_IDLED, GOID_INVALID, GetGoid() ) );
					}
				}
			}
		}
	}

	/////////////////////////////////////////////////////
	//	process sensors

	ProcessSensors();

#if !GP_RETAIL
	gWorldOptions.SetShowSpatialQueries( oldShow );
#endif

	m_InUpdateScope = false;
}


// for now only update and draw the MOVE Queue

void GoMind :: Draw()
{
	gpassertm( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC, "This Go shouldn't get a draw." );

#if DEBUG
	Job * action = GetFrontJob( JQ_ACTION );

	if( action )
	{
		if( action->GetJobAbstractType() == JAT_DO_SE_COMMAND )
		{
			eColor color = ( Modulus( float( gWorldTime.GetTime() ), 0.5f ) < 0.25f ) ? COLOR_RED : COLOR_YELLOW;
			gWorld.DrawDebugSphere( GetGo()->GetPlacement()->GetPosition(), 2.0, color, "next command not available" );
		}
	}
#endif

	DrawActionQueue();
}


#if !GP_RETAIL

void GoMind :: DrawDebugHud()
{
	gpassertm( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC, "This Go shouldn't get a drawdebughud." );

	if( GetGo()->HasAspect() && !IsAlive( GetGo()->GetAspect()->GetLifeState() ) )
	{
		return;
	}

	////////////////////////////////////////
	//	SE command

	if( m_InitialSECommand != SCID_INVALID )
	{
		GoHandle iCommand( gGoDb.FindGoidByScid( m_InitialSECommand ) );
		if( iCommand && iCommand->IsInAnyWorldFrustum() )
		{
			gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), iCommand->GetPlacement()->GetPosition(), COLOR_ORANGE, "initial command" );
		}
	}

	if( m_MovementOrders == MO_LIMITED )
	{
		if( m_LastExecutedUserAssignedActionPosition != SiegePos::INVALID )
		{
			gWorld.DrawDebugSphere( m_LastExecutedUserAssignedActionPosition, m_LimitedMovementRange, COLOR_ORANGE, "limited_movement_range" );
			gWorld.DrawDebugPoint( m_LastExecutedUserAssignedActionPosition, 0.25, COLOR_ORANGE, "LastExecutedUserAssignedActionPosition" );
		}
	}

	////////////////////////////////////////
	//	ranges

	// ranges for Go separation distance checks
	DrawDebugRangeOffsetAngle( m_PersonalSpaceRange,							PI*0/8.0f, COLOR_ORANGE,       "personal space"	);
	DrawDebugRangeOffsetAngle( m_PersonalSpaceRange+m_MeleeEngageRange,			PI*1/8.0f, COLOR_RED,          "melee engage"	);
	DrawDebugRangeOffsetAngle( m_PersonalSpaceRange+GetRangedEngageRange(),		PI*2/8.0f, COLOR_YELLOW,       "ranged engage"	);
	DrawDebugRangeOffsetAngle( m_PersonalSpaceRange+m_InnerComfortZoneRange,	PI*3/8.0f, COLOR_PURPLE,       "inner comfort"	);
	DrawDebugRangeOffsetAngle( m_PersonalSpaceRange+m_OuterComfortZoneRange,	PI*4/8.0f, COLOR_LIGHT_ORANGE, "outer comfort"	);

	// ranges for true distance checks
	DrawDebugRangeOffsetAngle( m_SightRange,									PI*5/8.0f, COLOR_WHITE,        "sight range"	);
	DrawDebugRangeOffsetAngle( m_ComRange,										PI*6/8.0f, COLOR_LIGHT_BLUE,   "com range"		);

	if( GetGo()->HasInventory() && GetGo()->GetInventory()->GetSelectedWeapon() )
	{
		DrawDebugRangeOffsetAngle( m_PersonalSpaceRange + GetGo()->GetInventory()->GetSelectedWeapon()->GetAttack()->GetAttackRange(),
			PI*7/8.0f,COLOR_LIGHT_RED, "weapon range" );
	}

	////////////////////////////////////////
	//	focus

	if( !GetGo()->HasParty() )
	{
		Go * focused = GetBestFocusEnemy();
		if( focused )
		{
			gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), focused->GetPlacement()->GetPosition(), COLOR_DARK_RED, "focus enemy" );
		}
	}

	////////////////////////////////////////
	//	jobs

	{
		JobQueue & queue = GetQ( JQ_ACTION );

		if( !queue.empty() )
		{
			queue.front()->DrawDebugHud();
		}
	}

	{
		JobQueue & queue = GetQ( JQ_BRAIN );

		if( !queue.empty() )
		{
			queue.front()->DrawDebugHud();
		}
	}

	////////////////////////////////////////
	//	enemies

	if( gWorldOptions.GetDebugHUDObject() == GetGoid() )
	{
		for( GoidColl::iterator i = m_EnemiesVisible.begin(); i != m_EnemiesVisible.end(); ++i )
		{
			GoHandle enemy( *i );
			if( enemy )
			{
				gWorld.DrawDebugTriangle( enemy->GetPlacement()->GetPosition(), enemy->GetMind()->GetPersonalSpaceRange(), COLOR_RED, "enemy" );
			}
		}

		for( i = m_FriendsVisible.begin(); i != m_FriendsVisible.end(); ++i )
		{
			GoHandle frnd( *i );
			if( frnd )
			{
				gWorld.DrawDebugTriangle( frnd->GetPlacement()->GetPosition(), frnd->GetMind()->GetPersonalSpaceRange(), COLOR_GREEN, "friend" );
			}
		}
	}

	////////////////////////////////////////
	//	visible and invisible objects

	if( gWorldOptions.GetDebugHUDObject() == GetGoid() )
	{
		SiegePos myPos = GetGo()->GetPlacement()->GetPosition();
		myPos.pos.y += 1.0;

		SiegePos fromPos;
		gAIQuery.GetLOSPoint( GetGo(), fromPos );

		for( GoidVisibleMap::iterator ig = m_GoidVisibleMap.begin(); ig != m_GoidVisibleMap.end(); ++ig )
		{
			bool isVisible = (*ig).second->m_Visible;

			GoHandle visible( (*ig).second->m_Object );
			if( !visible )
			{
				continue;
			}

			SiegePos toPos;
			gAIQuery.GetLOSPoint( visible, toPos );

			gpstring label;

			if( isVisible )
			{
				label.appendf( "los - %s", visible->GetTemplateName() );
				gWorld.DrawDebugDirectedLine( fromPos, toPos, COLOR_WHITE, label );
			}
			else
			{
				label.appendf( "NO los - %s", visible->GetTemplateName() );
				gWorld.DrawDebugDirectedLine( fromPos, toPos, COLOR_WHITE & 0x40ffffff, label );
			}
		}
	}
}


void GoMind :: DrawDebugRange( float range, DWORD color, const char * label )
{
	DrawDebugRangeOffsetAngle( range, 0, color, label );
}

void GoMind :: DrawDebugRangeOffsetAngle( float range, float offsetangle, DWORD color, const char * label )
{
	if( !IsZero( range ) )
	{
		gWorld.DrawDebugSphereOffsetAngle( GetGo()->GetPlacement()->GetPosition(), range, offsetangle, color, label );
	}
}
#endif // !GP_RETAIL


GoComponent* GoMind :: Clone( Go* newParent )
{
	return ( new GoMind( *this, newParent ) );
}

FUBI_DECLARE_XFER_TRAITS( Job * )
{
	static GoMind * ms_Owner;

	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if( persist.IsRestoring() )
		{
			obj = new Job;
			obj->SetMind( ms_Owner );
		}
		return ( obj->Xfer( persist ) );
	}
};


GoMind * FuBi::Traits <Job*>::ms_Owner = NULL;


#define RANGE_CHECK_A_LEQ_B( a, b )	\
	if( a > b )	{ \
		gperrorf(( "Actor '%s' violates convention that '%s' <= '%s'.  Please fix content.", GetGo()->GetTemplateName(), #a, #b )); } \


bool GoMind :: Xfer( FuBi::PersistContext& persist )
{
	gpassert( !m_InUpdateScope );
	gpassert( !m_InQueueDeletionScope );

	// only load stuff if we're not in the FE

	if( IsInFrontend( gWorldState.GetCurrentState() ) && !IsInStagingArea( gWorldState.GetCurrentState() ) )
	{
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
// 	Xfer state.

	if ( persist.IsFullXfer() )
	{
		gpassert( m_Deletion.empty() );

		FuBi::Traits <Job*>::ms_Owner = this;

		////////////////////////////////////////////////
		//	job related

		FUBI_XFER_BITFIELD_BOOL( persist, "m_ReqInitBrain",         m_ReqInitBrain         );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_ReqDoLastSECommand",   m_ReqDoLastSECommand   );

		FUBI_XFER_BITFIELD_BOOL( persist, "m_GiveInitialSECommand", m_GiveInitialSECommand );
		persist.Xfer( "m_LastSECommand", m_LastSECommand );
		persist.Xfer( "m_LastSECommandPathFailureTime", m_LastSECommandPathFailureTime );

		FUBI_XFER_BITFIELD_BOOL( persist, "m_ReqGiveStartupJob",	m_ReqGiveStartupJob    );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_BrainActive",  		m_BrainActive		   );

		persist.Xfer( "m_Rank",					  m_Rank			);
		persist.Xfer( "m_bEnemySpottedPlayed",	  m_bEnemySpottedPlayed );

		persist.Xfer( "m_AllignedTeam",			  m_AllignedTeam,	-1 );

		persist.XferVector( "m_Brains",           m_Brains          );
		persist.XferVector( "m_Actions",          m_Actions         );

		persist.Xfer ( "m_IdleTimer",             m_IdleTimer				);
		persist.Xfer ( "m_IdleTimerTrigger",      m_IdleTimerTrigger		);
		persist.Xfer ( "m_NextTimeAttackAllowed", m_NextTimeAttackAllowed	);
		persist.Xfer ( "m_NextTimeCastAllowed",   m_NextTimeCastAllowed		);

		persist.Xfer ( "m_LastActionProcessedId", m_LastActionProcessedId	);
		persist.Xfer ( "m_LastCurrentBaseJat",	  m_LastCurrentBaseJat		);	// $$$ is this still useful?

		persist.Xfer ( "m_LastExecutedUserAssignedActionPosition", m_LastExecutedUserAssignedActionPosition );

		persist.Xfer ( "m_ActionResultMemoryLength", m_ActionResultMemoryLength );
		persist.Xfer ( "m_LastElevatorRideTime", m_LastElevatorRideTime );

		////////////////////////////////////////////////
		//	sensors

		persist.Xfer( "m_NextSensorScanTime", m_NextSensorScanTime );
		persist.Xfer( "m_ActorLifeRatioLast", m_ActorLifeRatioLast );
		persist.Xfer( "m_ActorManaRatioLast", m_ActorManaRatioLast );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_RequestProcessCombatSensors", m_RequestProcessCombatSensors );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_RequestProcessHitEngaged",    m_RequestProcessHitEngaged    );

		persist.XferMap    ( "m_EngagedMe"                , m_EngagedMeMap             );
		persist.XferVector ( "m_EnemiesVisible"           , m_EnemiesVisible           );
		persist.XferVector ( "m_FriendsVisible"           , m_FriendsVisible           );
		persist.XferVector ( "m_PathBlockers"			  , m_PathBlockers			   );
		persist.XferVector ( "m_ActorsInInnerComfortZone" , m_ActorsInInnerComfortZone );
		persist.XferVector ( "m_ActorsInOuterComfortZone" , m_ActorsInOuterComfortZone );

		FUBI_XFER_BITFIELD_BOOL( persist, "m_AliveFriendsVisibleDirty" , m_AliveFriendsVisibleDirty );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_AliveFriendsVisible"      , m_AliveFriendsVisible      );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_AliveEnemiesVisibleDirty" , m_AliveEnemiesVisibleDirty );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_AliveEnemiesVisible"      , m_AliveEnemiesVisible      );
	}


////////////////////////////////////////////////////////////////////////////////
// 	Xfer state that is also spec.

	persist.Xfer( "disposition_orders",      m_DispositionOrders );
	persist.Xfer( "combat_orders",           m_CombatOrders      );
	persist.Xfer( "movement_orders",         m_MovementOrders    );
	persist.Xfer( "focus_orders",            m_FocusOrders       );
	persist.Xfer( "actor_weapon_preference", m_WeaponPreference  );
#if !GP_RETAIL
	if( m_WeaponPreference == WP_INVALID )
	{
		gperrorf(( "Actor %s, scid 0x%08x, didn't specify a weapon preference.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
	}
#endif

	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_fidgets", m_ActorAutoFidgets		);


////////////////////////////////////////////////////////////////////////////////
//	XFER spec

	////////////////////////////////////////
	//	basic permissions

	FUBI_XFER_BITFIELD_BOOL( persist, "actor_may_process_ai",             	 				m_MayProcessAI								);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_may_attack",             	 					m_MayAttack									);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_may_be_attacked",             	 				m_MayBeAttacked								);

	////////////////////////////////////////
	//	job permissions

	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_defends_others",             	 			m_ActorAutoDefendsOthers      			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_heals_others_life",          	 			m_ActorAutoHealsOthersLife    			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_heals_others_mana",          	 			m_ActorAutoHealsOthersMana    			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_heals_self_life",            	 			m_ActorAutoHealsSelfLife      			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_heals_self_mana",            	 			m_ActorAutoHealsSelfMana      			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_picks_up_items",             	 			m_ActorAutoPicksUpItems       			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_xfers_mana",                      		m_ActorAutoXfersMana                     	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_reanimates_friends",						m_ActorAutoReanimatesFriends	         	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_switches_to_karate",         	 			m_ActorAutoSwitchesToKarate   			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_switches_to_melee",          	 			m_ActorAutoSwitchesToMelee    			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_switches_to_ranged",         	 			m_ActorAutoSwitchesToRanged   			 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_switches_to_magic",          	 			m_ActorAutoSwitchesToMagic    			 	);

#if !GP_RETAIL

	if( !persist.IsFullXfer() )
	{
		if( GetGo()->GetPlayer()->GetController() != PC_HUMAN )
		{
			if( (m_WeaponPreference == WP_MELEE) && !m_ActorAutoSwitchesToMelee )
			{
				gperrorf(( "CONTENT ERROR: Actor %s, scid 0x%08x has WP_MELEE, but doesn't have permissions to switch to weapons he prefers.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
			}
			if( (m_WeaponPreference == WP_RANGED) && !m_ActorAutoSwitchesToRanged )
			{
				gperrorf(( "CONTENT ERROR: Actor %s, scid 0x%08x has WP_RANGED, but doesn't have permissions to switch to weapons he prefers.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
			}
			if( (m_WeaponPreference == WP_MAGIC) && !m_ActorAutoSwitchesToMagic )
			{
				gperrorf(( "CONTENT ERROR: Actor %s, scid 0x%08x has WP_MAGIC, but doesn't have permissions to switch to weapons he prefers.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
			}
		}
	}

#endif

	FUBI_XFER_BITFIELD_BOOL( persist, "actor_auto_uses_stored_items",          	 			m_ActorAutoUsesStoredItems    			 	);

	persist.Xfer( "actor_balanced_attack_preference",										m_ActorBalancedAttackPreference			 	);

	FUBI_XFER_BITFIELD_BOOL( persist, "on_alert_projectile_near_missed_flee",       		m_OnAlertProjectileNearMissedFlee		 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_alert_projectile_near_missed_attack",     		m_OnAlertProjectileNearMissedAttack		 	);

	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_icz_attack",                		m_OnEnemyEnteredICZAttack                	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_icz_flee",                  		m_OnEnemyEnteredICZFlee                  	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_icz_switch_to_melee",				m_OnEnemyEnteredICZSwitchToMelee         	);

#if !GP_RETAIL
	if( m_OnEnemyEnteredICZSwitchToMelee && !m_ActorAutoSwitchesToMelee )
	{
		gperrorf(( "CONTENT ERROR: Actor %s, scid 0x%08x, is told to 'm_OnEnemyEnteredICZSwitchToMelee' but doesn't have required 'm_ActorAutoSwitchesToMelee' premission.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
	}
#endif

	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_ocz_attack",      	        		m_OnEnemyEnteredOCZAttack                	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_ocz_flee",      	        		m_OnEnemyEnteredOCZFlee                  	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_entered_weapon_engage_range_attack",		m_OnEnemyEnteredWeaponEngageRangeAttack  	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_spotted_alert_friends",             		m_OnEnemySpottedAlertFriends             	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_enemy_spotted_attack",                    		m_OnEnemySpottedAttack                   	);

	FUBI_XFER_BITFIELD_BOOL( persist, "on_engaged_fled_abort_attack",               		m_OnEngagedFledAbortAttack             	 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_engaged_lost_consciousness_abort_attack",			m_OnEngagedLostConsciousnessAbortAttack  	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_engaged_lost_loiter",                     		m_OnEngagedLostLoiter                    	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_engaged_lost_return_to_job_origin",       		m_OnEngagedLostReturnToJobOrigin         	);

	FUBI_XFER_BITFIELD_BOOL( persist, "on_friend_entered_icz_attack",               		m_OnFriendEnteredICZAttack               	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_friend_entered_icz_flee",                 		m_OnFriendEnteredICZFlee                 	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_friend_entered_ocz_attack",               		m_OnFriendEnteredOCZAttack               	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_friend_entered_ocz_flee",                 		m_OnFriendEnteredOCZFlee                 	);

	FUBI_XFER_BITFIELD_BOOL( persist, "on_job_reached_travel_distance_abort_attack",		m_OnJobReachedTravelDistanceAbortAttack  	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_life_ratio_low_flee",                     		m_OnLifeRatioLowFlee                     	);
	FUBI_XFER_BITFIELD_BOOL( persist, "on_mana_ratio_low_flee",                     		m_OnManaRatioLowFlee                     	);

	////////////////////////////////////////
	//	sensor parameters

	persist.Xfer( "sensor_scan_period",              m_SensorScanPeriod            );

	persist.Xfer( "actor_life_ratio_high_threshold", m_ActorLifeRatioHighThreshold );
	persist.Xfer( "actor_life_ratio_low_threshold",  m_ActorLifeRatioLowThreshold  );
	persist.Xfer( "actor_mana_ratio_high_threshold", m_ActorManaRatioHighThreshold );
	persist.Xfer( "actor_mana_ratio_low_threshold",  m_ActorManaRatioLowThreshold  );
	persist.Xfer( "com_range",                       m_ComRange                    );
	persist.Xfer( "flee_count",                      m_FleeCount                   );

	persist.Xfer( "min_comfort_delay_threshold",     m_MinComfortDelayThreshold    );
	persist.Xfer( "max_comfort_delay_threshold",     m_MaxComfortDelayThreshold    );

	persist.Xfer( "inner_comfort_zone_range",        m_InnerComfortZoneRange       );
	persist.Xfer( "job_travel_distance_limit",       m_JobTravelDistanceLimit      );
	persist.Xfer( "limited_movement_range",          m_LimitedMovementRange        );
	persist.Xfer( "melee_engage_range",              m_MeleeEngageRange            );
	persist.Xfer( "outer_comfort_zone_range",        m_OuterComfortZoneRange       );
	persist.Xfer( "personal_space_range",            m_PersonalSpaceRange          );
	persist.Xfer( "ranged_engage_range",             m_RangedEngageRange           );
	persist.Xfer( "sight_fov",                       m_SightFov                    );
	persist.Xfer( "sight_range",                     m_SightRange                  );
	persist.Xfer( "sight_origin_height",             m_SightOriginHeight           );
	persist.Xfer( "visibility_memory_duration",      m_VisibilityMemoryDuration    );

	RANGE_CHECK_A_LEQ_B( m_PersonalSpaceRange,    m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_MeleeEngageRange,      m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_RangedEngageRange,     m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_InnerComfortZoneRange, m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_OuterComfortZoneRange, m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_ComRange,              m_SightRange            );
	RANGE_CHECK_A_LEQ_B( m_InnerComfortZoneRange, m_OuterComfortZoneRange );
	RANGE_CHECK_A_LEQ_B( m_MeleeEngageRange,      m_RangedEngageRange     );

	////////////////////////////////////////////////////////////////////////////////
	//	job parameters

	persist.Xfer( "flee_distance",					m_FleeDistance                );
	persist.Xfer( "initial_command",				m_InitialSECommand            );

	///////////////////////////////////////////
	// xfer job definitions

	if( persist.IsRestoring() )
	{
		for( eJobAbstractType jat = JAT_BEGIN; jat != JAT_END; ++jat )
		{
			const char* key = ::ToString( jat );

			gpstring value;
			persist.Xfer( key, value );

			if( value.empty() || value.same_no_case( "jat_none" ) )
			{
				continue;
			}

			if( jat == JAT_STARTUP &&  !persist.IsFullXfer() )
			{
				m_ReqGiveStartupJob = true;
			}

			JobCloneSourceMap::iterator islot = gAIAction.RegisterJobCloneSource( value, persist.IsFullXfer() );
			gpassert( islot != NULL );

			JobLookupEntry jatEntry;
			jatEntry.m_Jat		= jat;
			jatEntry.m_pSlot	= islot;

			m_JobLookupEntryColl.push_back( jatEntry );
		}
		// com channels
		gpstring sMembership;
		persist.Xfer( "com_channels", sMembership );
		if( !sMembership.empty() )
		{
			m_ComChannels.Add( sMembership );
		}
		// check initial command
		if( m_InitialSECommand != SCID_INVALID )
		{
			m_GiveInitialSECommand = true;
		}
	}
	else
	{
		for( JobLookupEntryColl::iterator i = m_JobLookupEntryColl.begin(); i != m_JobLookupEntryColl.end(); ++i )
		{
			const char* key = ::ToString( (*i).m_Jat );
			persist.Xfer( key, (*i).m_pSlot->second->m_SkritPath );
		}
	}
	return true;
}


bool GoMind :: XferCharacter( FuBi::PersistContext& persist )
{
	persist.Xfer( "movement_orders",             m_MovementOrders            );
	persist.Xfer( "combat_orders",               m_CombatOrders              );
	persist.Xfer( "focus_orders",                m_FocusOrders               );
	persist.Xfer( "disposition_orders",          m_DispositionOrders         );
	persist.Xfer( "m_ClientCachedEngagedObject", m_ClientCachedEngagedObject );

	return ( true );
}


bool GoMind :: CommitCreation()
{
	/////////////////////////////////////////////////////
	//	init the brain

	if( !IsServerLocal() )
	{
		return true;
	}

	return true;
}


bool GoMind :: CommitLoad()
{
	return ( CommitCreation() );
}


DWORD GoMind :: GoMindToNet( GoMind* x )
{
	return ( MakeInt( x->GetGoid() ) );
}


GoMind* GoMind :: NetToGoMind( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetMind() : NULL );
}


void GoMind :: ProcessDeletionQueue()
{
	GPPROFILERSAMPLE( "GoMind :: ProcessDeletionQueue", SP_AI | SP_AI_MIND );

	gpassert( !m_InQueueDeletionScope );

	m_InQueueDeletionScope = true;

	if( m_Deletion.empty() )
	{
		m_InQueueDeletionScope = false;
		return;
	}

	DWORD debugSizeBefore = m_Deletion.size();
	JobQueue::iterator i;
	for( i = m_Deletion.begin(); i != m_Deletion.end(); ++i )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i) );
		delete (*i);
		gpassert( debugSizeBefore == m_Deletion.size() );
	}
	m_Deletion.clear();

	m_InQueueDeletionScope = false;
}


bool GoMind :: AmBusy( eJobQ Q ) const
{
	Job * pJob = GetFrontJob( Q );

	if( pJob == NULL )
	{
		return false;
	}

	return ( pJob->GetTraits() & JT_AUTO_INTERRUPTABLE ) == 0;
}


bool GoMind :: AmPatrolling() const
{
	for( JobQueue::const_iterator i = m_Actions.begin(); i != m_Actions.end(); ++i )
	{
		if( !(*i)->IsMarkedForDeletion() && ( (*i)->GetJobAbstractType() == JAT_PATROL ) )
		{
			return true;
		}
	}
	return false;
}


void GoMind :: Clear( JobQueue & q )
{
	JobQueue::iterator i;

	for( i=q.begin(); i != q.end(); ++i )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i ) );
		(*i)->SetTraits( (*i)->GetTraits() & NOT( JT_REPEATING ) );
		(*i)->MarkForDeletion( JR_OK );
	}
}


void GoMind :: Clear( eJobQ q )
{
	Clear( GetQ( q ) );
}


void GoMind :: Clear()
{
	Clear( m_Actions );
	Clear( m_Brains );
}


void GoMind :: ClearAndCommit( eJobQ q )
{
	Clear( q );
	Update( 0 );
}


bool GoMind :: UnderstandsJob( eJobAbstractType Abstract ) const
{
	GPPROFILERSAMPLE( "GoMind :: UnderstandsJob", SP_AI | SP_AI_MIND );

	for( JobLookupEntryColl::const_iterator i = m_JobLookupEntryColl.begin(); i != m_JobLookupEntryColl.end(); ++i )
	{
		if( (*i).m_Jat == Abstract )
		{
			return true;
		}
	}
	return false;
}


bool GoMind :: Validate( JobReq const & req )
{
	bool valid = true;
	eJobTraits traits = GetTraits( req.m_Jat );
	gpstring error;

	if( (GetGo()->GetPlayer()->GetController() != PC_HUMAN ) )
	{
		if( m_Actions.size() > 100 )
		{
			gperrorf(( "Insane Actor '%s', Scid 0x%08x - sure has a lot of jobs!  Possible error in content; Triggers or Commands.", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
			if( GetGo()->GetPlayer()->GetController() != PC_HUMAN )
			{
				//gGoDb.MarkForDeletion( GetGo()->GetGoid() );
				valid = false;
			}
		}
	}

	if( (GetGo()->GetPlayer()->GetController() == PC_HUMAN ) )
	{
		if( ( req.m_Origin == AO_USER ) && (( req.m_QPlace == QP_FRONT ) || ( req.m_QPlace == QP_CLEAR )) && !IsCurrentActionHumanInterruptable() )
		{
			error.appendf( "Actor '%s', Scid 0x%08x - human has asked for an action while a non-human-interruptable action is in progress.", GetGo()->GetTemplateName(), GetGo()->GetScid() );
			valid = false;
		}
	}

	if( IsInGame( gWorldState.GetCurrentState() ) )
	{
		if( !GetGo()->IsInAnyWorldFrustum() )
		{
			error.appendf( "Go '%s' scid:0x%08x can't process job request for '%s' because it is outside of all world frustums.\n",
							GetGo()->GetTemplateName(),
							GetGo()->GetScid(),
							::ToString( req.m_Jat ) );
			valid = false;
		}
	}

	if( req.m_Jat == JAT_NONE )
	{
		error.appendf( "Requesting to do job '%s' is not a valid request.  Check your content.\n", ::ToString( req.m_Jat ) );
		valid = false;
	}

	if( IsDead( GetGo()->GetLifeState() ) && ((traits&JT_DEAD_OK) == 0) )
	{
		error.appendf( "Job %s can't run on a 'dead' actor.\n", ::ToString( req.m_Jat ) );
		valid = false;
	}
	else if( GetGo()->GetLifeState() == LS_GHOST && (( traits & JT_GHOST_OK ) == 0 ) )
	{
		error.appendf( "Job %s can't run on a 'ghost' actor.\n", ::ToString( req.m_Jat ) );
		valid = false;
	}
	else if( ( GetGo()->GetLifeState() == LS_ALIVE_UNCONSCIOUS ) && (( traits & JT_UNCONSCIOUS_OK ) == 0 ) )
	{
		error.appendf( "Job %s can't run on an 'unconscious' actor.\n", ::ToString( req.m_Jat ) );
		valid = false;
	}

	if( traits & JT_REQUIRE_GOAL )
	{
		if( req.m_GoalObject != GOID_NONE )
		{
			Go * go = ::GetGo( req.m_GoalObject );
			if( (go == NULL) || !CanOperateOn( go ) )
			{
				error.appendf( "Job %s requires valid GoalObject.\n", ::ToString( req.m_Jat ) );
				valid = false;
			}

			if( go != NULL )
			{
				if( go == GetGo() )
				{
					if( !(traits & JT_ALLOW_REFLEXIVE_GOAL ) )
					{
						error.appendf( "Job %s doesn't allow reflexive GoalObject.\n", ::ToString( req.m_Jat ) );
						valid = false;
					}
				}

				if( (traits & JT_OFFENSIVE ) && go->HasMind() && !go->GetMind()->GetMayBeAttacked() )
				{
					error.appendf( "Job %s not allowed to run on %s because that actor may not be attacked.\n", ::ToString( req.m_Jat ), go->GetTemplateName() );
					valid = false;
				}
			}
		}
	}

	if( traits & JT_REQUIRE_MODIFIER )
	{
		Go * go = ::GetGo( req.m_GoalModifier );
		if( go == NULL )
		{
			error.appendf( "Job %s requires GoalModifier.\n", ::ToString( req.m_Jat ) );
			valid = false;
		}

		if( go == GetGo() )
		{
			if( !(traits & JT_ALLOW_REFLEXIVE_MODIFIER ) )
			{
				error.appendf( "Job %s doesn't allow reflexive GoalModifier.\n", ::ToString( req.m_Jat ) );
				valid = false;
			}
		}

		if( req.m_QPlace ==	QP_CLEAR || req.m_QPlace == QP_FRONT )
		{
			bool ownModifier = false;
			Go * parent = go;
			while( parent != NULL )
			{
				if( parent == GetGo() )
				{
					ownModifier = true;
				}
				parent = parent->GetParent();
			}
			if( !ownModifier )
			{
				error.appendf( "Job %s uses a GoalModifier that is not owned by the Actor who owns the job.\n", ::ToString( req.m_Jat ) );
				valid = false;
			}
		}
	}

	if( traits & JT_REQUIRE_POSITION )
	{
		if( !gSiegeEngine.IsNodeInAnyFrustum( req.m_GoalPosition.node ) )
		{
			error.appendf( "Job %s requires valid GoalPosition.\n", ::ToString( req.m_Jat ) );
			valid = false;
		}

		if( GetGo()->GetPlacement()->GetPosition() == req.m_GoalPosition )
		{
			if( !( traits & JT_ALLOW_REFLEXIVE_POSITION ) )
			{
				error.appendf( "Job %s doesn't allow reflexive GoalPosition.\n", ::ToString( req.m_Jat ) );
				valid = false;
			}
		}

	}

	if( traits & JT_REQUIRE_SLOT )
	{
		if( req.m_Slot == ES_NONE )
		{
			error.appendf( "Job %s requires valid EquipSlot.\n", ::ToString( req.m_Jat ) );
			valid = false;
		}
	}

	if( !valid )
	{
		gpstring fullError;
		fullError.appendf( "Job request for '%s' Scid 0x%08x invalid:\n", GetGo()->GetTemplateName(), GetGo()->GetScid() );
		fullError.append( error );
		fullError.append( "Job request will be ignored.\n" );
		gpwarning( fullError.c_str() );
	}

	return valid;
}


void GoMind :: RSDoJob( JobReq const & req )
{
	if( !Validate( req ) )
	{
		gpwarningf(( "[%2.4f] GoMind :: RSDoJob - Request to run Job '%s' for actor '%s' ignored.", gWorldTime.GetTime(), ::ToString( req.m_Jat ), GetGo()->GetTemplateName() ));
		return;
	}

	if ( IsSendLocalOnly( RPC_TO_SERVER ) )
	{
		SDoJob( req );
	}
	else
	{
		FuBi::BitPacker packer;
		ccast <JobReq&> ( req ).Xfer( packer );
		RSDoJobPacker( packer.GetSavedBits() );
	}
}


void GoMind :: RSDoJobPacker( const_mem_ptr packola )
{
	FUBI_RPC_THIS_CALL( RSDoJobPacker, RPC_TO_SERVER );

	JobReq req;
	FuBi::BitPacker packer( packola );
	req.Xfer( packer );

	SDoJob( req );
}


Job * GoMind :: SDoJob( JobReq const & _req )
{
	GPPROFILERSAMPLE( "GoMind :: SDoJob", SP_AI | SP_AI_MIND );

	gpassertm( GetGoidClass( GetGoid() ) != GO_CLASS_CLONE_SRC, "Clone sources can't do jobs." );

	CHECK_SERVER_ONLY;

	/////////////////////////////////////////////////////////////
	//	re-entry guard - new jobs may not be started while we're cleaning out the deletion q

	if( m_InQueueDeletionScope )
	{
		return NULL;
	}

	JobReq req( _req );

#	if !GP_RETAIL
	if( GetGo()->IsHuded() )
	{
		gpgenericf((	"[%2.4f, 0x%08x] GoMind :: SDoJob - actor '%s': jat = %s, Q = %s, QP = %s, JO = %s\n",
						gWorldTime.GetTime(),
						gWorldTime.GetSimCount(),
						GetGo()->GetTemplateName(),
						::ToString( req.m_Jat ),
						::ToString( req.m_Q ),
						::ToString( req.m_QPlace ),
						::ToString( req.m_Origin )  ));
	}
#	endif // !GP_RETAIL

	ClearTemps();

	/////////////////////////////////////////////////////////////
	//	limit the number of jobs that can be queued up

	if( ( req.m_Q == JQ_ACTION ) && ( m_Actions.size() > 128 ) )
	{
		return NULL;
	}

	/////////////////////////////////////////////////////////////
	//	validate parameters based on job traits

	if( !Validate( req ) )
	{
		gpwarningf(( "Request to run Job '%s' for actor '%s', Scid 0x%08x not granted.", ::ToString( req.m_Jat ), GetGo()->GetTemplateName(), GetGo()->GetScid() ));
		return NULL;
	}

	{
		eJobTraits traits = GetTraits( req.m_Jat );
		if( traits & JT_DISALLOW_CIRCULAR_JOB_ASSIGNMENT )
		{
			GoHandle target( req.m_GoalObject );
			if( target && target->IsInAnyWorldFrustum() && target->HasMind() )
			{
				Job * job = target->GetMind()->GetFrontJob( JQ_ACTION );
				if( job && ( job->GetJobAbstractType() == req.m_Jat ) && ( job->GetGoalObject() == GetGo()->GetGoid() ) )
				{
					gpgenericf(( "Job request '%s' for '%s' Scid 0x%08x ignored due to circular job assignment.\n", ::ToString( req.m_Jat), GetGo()->GetTemplateName(), GetGo()->GetScid() ));
					return NULL;
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////
	// check if we can ignore this request - if it's a duplicate

	bool clearAfterInsertionPoint = false;

	if( CheckActionCollision( req ) )
	{
		return NULL;
	}
	else
	{
		if( req.m_Jat != JAT_DIE )
		{
			bool delay = false;

			Job * frontAction = GetFrontJob( JQ_ACTION );

			if( req.m_QPlace == QP_CLEAR || req.m_QPlace == QP_FRONT || ( frontAction && ( req.m_QPlace == QP_NONE && req.m_PlaceBefore == frontAction->GetId() ) ) )
			{
				delay = !MaySDoJob( req.m_Jat );

				Job * frontAction = GetFrontJob( JQ_ACTION );

				if( delay )
				{
					if( ( frontAction && ( req.m_Jat != JAT_DIE ) && frontAction->InAtomicState() ) )
					{
						req.m_QPlace				= QP_NONE;
						req.m_PlaceAfter			= frontAction->GetId();
						clearAfterInsertionPoint	= true;

						frontAction->RequestEnd();
					}
					else
					{
						return NULL;
					}
				}
			}
		}
	}


	/////////////////////////////////////////////////////////////
	// resolve behavior from abstract to specific

	if( !UnderstandsJob( req.m_Jat ) )
	{
		gperrorf(( "GoMind :: AddJob - Could not translate job alias '%s' for %s - check template [mind] component", ::ToString( req.m_Jat ), GetGo()->GetTemplateName() ));
		return NULL;
	}

	Job * pJob = NULL;

	if( !Create( pJob, req.m_Jat ) )
	{
		gperrorf(( "GoMind :: AddJob - failed to create '%s' job for %s" , ::ToString( req.m_Jat ), GetGo()->GetTemplateName() ));
		return NULL;
	}

	/////////////////////////////////////////////////////////////
	// parameterize new job

	eJobTraits traits = JT_NONE;

	if( req.m_Origin == AO_USER )
	{
		traits = JT_HUMAN_ASSIGNED;
	}
	else if( req.m_Origin == AO_REFLEX )
	{
		traits = JT_REFLEX_ASSIGNED;
	}
	else if( req.m_Origin == AO_PARTY )
	{
		traits = JT_PARTY_ASSIGNED;
	}
	else if( req.m_Origin == AO_COMMAND )
	{
		traits = JT_COMMAND_ASSIGNED;
	}

	if( ( req.m_QPlace == QP_CLEAR ) || ( req.m_QPlace == QP_FRONT ) )
	{
		traits |= JT_ASSIGNED_FRONT;
	}
	else if( req.m_QPlace == QP_BACK )
	{
		traits |= JT_ASSIGNED_BACK;
	}

	if( req.m_Jat == JAT_CAST )
	{
		GoHandle spell( req.m_GoalModifier );
		if( ( spell->GetMagic()->GetUsageContextFlags() == UC_AGGRESSIVE ) || ( spell->GetMagic()->GetUsageContextFlags() == UC_OFFENSIVE ) )
		{
			traits |= JT_OFFENSIVE;
		}
	}

	pJob->SetTraits( pJob->GetTraits() | traits );

	if( pJob->GetTraits() & JT_REQUIRE_GOAL )
	{
		pJob->SetGoalObject( req.m_GoalObject );
	}

	if( pJob->GetTraits() & JT_REQUIRE_MODIFIER )
	{
		pJob->SetGoalModifier( req.m_GoalModifier );
	}

	if( pJob->GetTraits() & JT_REQUIRE_POSITION )
	{
		pJob->SetGoalPosition( req.m_GoalPosition );
		if( req.m_GoalOrientation != Quat::INVALID )
		{
			pJob->SetGoalOrientation( req.m_GoalOrientation );
		}
	}

	if( pJob->GetTraits() & JT_REQUIRE_SLOT )
	{
		pJob->SetGoalSlot( req.m_Slot );
	}

	if( req.m_Notify != GOID_INVALID )
	{
		pJob->SetNotify( req.m_Notify );
	}

	pJob->SetInt1( req.m_Int1 );
	pJob->SetInt2( req.m_Int2 );
	pJob->SetFloat1( req.m_Float1 );
	pJob->SetStartedBySECommand( req.m_SECommandScid );

	if( req.m_Jat != JAT_FIDGET && req.m_Jat != JAT_BRAIN )
	{
		m_LastSECommand = req.m_SECommandScid;
	}

	/////////////////////////////////////////////////////////////
	// place new job in the right spot in Q
	InsertJob( pJob, req, clearAfterInsertionPoint );


	/////////////////////////////////////////////////////////////
	//	$$$ move up one scope - handle exceptional case - user assigned patrol

	// if this is the first patrol job in the Q and it's human-assigned, add a patrol-back job

	if( req.m_Jat == JAT_PATROL && req.m_Origin == AO_COMMAND )
	{
		pJob->SetTraits( pJob->GetTraits() & NOT( JT_REPEATING ) );
	}

	if( req.m_Jat == JAT_PATROL && req.m_Origin == AO_USER )
	{
		bool patrolFound = false;

		JobQueue & queue = GetQ( req.m_Q );

		JobQueue::iterator i;

		for( i = queue.begin(); i != queue.end(); ++i )
		{
			gpassert( !IsMemoryBadFood( (DWORD)*i ) );

			if( (*i)->IsMarkedForDeletion() )
			{
				continue;
			}

			if( ( pJob != (*i) ) && ( (*i)->GetJobAbstractType() == JAT_PATROL ) )
			{
				patrolFound = true;
				break;
			}
		}

		if( !patrolFound )
		{
			Job * patrolBack = NULL;

			if( !Create( patrolBack, req.m_Jat ) )
			{
				gperrorf(( "GoMind :: AddJob - failed to create '%s' job for %s" , ::ToString( req.m_Jat ), GetGo()->GetTemplateName() ));
				return NULL;
			}
			patrolBack->SetTraits( patrolBack->GetTraits() | JT_REFLEX_ASSIGNED );

			patrolBack->SetGoalPosition( GetGo()->GetPlacement()->GetPosition() );

			JobReq backReq = req;
			backReq.m_QPlace = QP_BACK;

			InsertJob( patrolBack, backReq, false );
		}
	}

	OnJobAdded( pJob );

	return pJob;
}


bool GoMind :: MaySDoJob( eJobAbstractType jat )
{
	if( IsAttackJat( jat ) && ( m_NextTimeAttackAllowed > gWorldTime.GetTime() ) )
	{
		return false;
	}
	else if( IsCastJat( jat ) && ( m_NextTimeCastAllowed > gWorldTime.GetTime() ) )
	{
		return false;
	}
	return true;
}


bool GoMind :: CheckActionCollision( JobReq const & req )
{
	GPPROFILERSAMPLE( "GoMind :: CheckActionCollision", SP_AI | SP_AI_MIND );

	if( req.m_Origin != AO_USER )
	{
		return false;
	}

	Job * action = GetFrontJob( JQ_ACTION );
	if( action && !action->IsMarkedForDeletion() )
	{
		eJobAbstractType newJat, oldJat;
		newJat = req.m_Jat;
		oldJat = GetBaseJat( action->GetJobAbstractType() );

		////////////////////////////////////////
		//	when two jat's collide, they must have unique parameters
		if( newJat == oldJat )
		{
			if( ( action->GetTraits() & JT_REQUIRE_GOAL ) && ( req.m_GoalObject == action->GetGoalObject() ) )
			{
				return true;
			}
			else if( ( action->GetTraits() & JT_REQUIRE_POSITION ) )
			{
				if( gAIQuery.GetDistance( req.m_GoalPosition, action->GetGoalPosition() ) < GetPersonalSpaceRange() )
				{
					return true;
				}
			}
		}

		////////////////////////////////////////
		//	all moves must be at least personal_space distance away from actor
		if( newJat == JAT_MOVE )
		{
			if( gAIQuery.GetDistance( GetGo()->GetPlacement()->GetPosition(), req.m_GoalPosition ) < GetPersonalSpaceRange() )
			{
				return true;
			}
		}
	}
	return false;
}


void GoMind :: OnJobAdded( Job * job )
{
	if( !job )
	{
		return;
	}

	if( job->GetJobAbstractType() == JAT_FLEE_FROM_OBJECT )
	{
		if( m_FleeCount )
		{
			--m_FleeCount;
		}
		MessageEngagedMe( WorldMessage( WE_ENGAGED_FLED ) );
	}
}


void GoMind :: OnJobEnterAtomicState( Job * job, float duration )
{
	if( IsLeafAttackJat( job->GetJobAbstractType() ) )
	{
		m_NextTimeAttackAllowed = PreciseAdd( gWorldTime.GetTime(), duration );
	}
	else if( IsCastJat( job->GetJobAbstractType() ) )
	{
		m_NextTimeCastAllowed = PreciseAdd( gWorldTime.GetTime(), duration );
	}
}


void GoMind :: PostJobAnyEvent( Job * job, WorldMessage const * msg )
{
	if( msg == NULL )
	{
		if( !job->IsMarkedForDeletion() )
		{
			if( job->GetTraits() & ( JT_HUMAN_ASSIGNED | JT_COMMAND_ASSIGNED ) )
			{
				if( job->GetJobAbstractType() == JAT_STOP || job->GetJobAbstractType() == JAT_FOLLOW )
				{
					m_LastExecutedUserAssignedActionPosition = GetGo()->GetPlacement()->GetPosition();
				}
				else if( job->GetTraits() & JT_REQUIRE_POSITION )
				{
					m_LastExecutedUserAssignedActionPosition = job->GetGoalPosition();
				}
				else if( job->GetTraits() & JT_REQUIRE_GOAL )
				{
					GoHandle goal( job->GetGoalObject() );
					if( goal && (( MakeInt( GetGo()->GetWorldFrustumMembership() ) & MakeInt( goal->GetWorldFrustumMembership() )) != 0 ) )
					{
						m_LastExecutedUserAssignedActionPosition = goal->GetPlacement()->GetPosition();
					}
				}
			}
		}
	}
	else
	{
		if( msg->GetEvent() == WE_MCP_SECTION_COMPLETE_WARNING )
		{
			Goid notifyGoid = job->GetNotify();
			GoHandle notify( notifyGoid );
			if( notify && CanOperateOn( notify ) )
			{
				// notify anyone that might have wanted to know about this jobs almost ending
				Send( WorldMessage( msg->GetEvent(), GetGo()->GetGoid(), notifyGoid ) );
			}
		}
	}

	if( job->IsMarkedForDeletion() )
	{
		GoHandle notifyGo( job->GetNotify() );

		// notify anyone that might have wanted to know about this jobs ending
		if( notifyGo && (job->GetJobResult() != JR_FAILED_NO_PATH) && CanOperateOn( notifyGo ) )
		{
			Send( WorldMessage( WE_JOB_FINISHED, GetGo()->GetGoid(), job->GetNotify(), MakeInt( GetGo()->GetGoid() ) ) );
		}

		if( job->GetJobResult() == JR_FAILED_NO_PATH )
		{
			if( job->GetStartedBySECommand() != SCID_INVALID )
			{
				m_LastSECommandPathFailureTime = gWorldTime.GetTime();
			}

			SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_FRONT, AO_REFLEX ) );
		}
	}
}


void GoMind :: MessageEngagedMe( WorldMessage const & _msg, Goid ignore )
{
	WorldMessage msg( _msg );

	// to prevent list re-entry problems:

	GoidColl engaged;
	EngagedMeMap::iterator i;
	for( i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		engaged.push_back( (*i).first );
	}

	// now message all the happy enagege-ees

	for( GoidColl::iterator j = engaged.begin(); j != engaged.end(); ++j )
	{
		if( (*j) == ignore )
		{
			continue;
		}

		GoHandle engaged( *j );

		if( !engaged )
		{
			continue; // $$ this should be avoided
		}

		if( !CanOperateOn( engaged, false ) )
		{
			//gperror( "Found bad Go in engaged me list." );
			continue;
		}

		if( engaged && engaged->HasMind() && ( engaged->GetMind()->GetEngagedObject() != GetGoid() ) )
		{
			continue;
		}

		gpassertm( GetGoid() != *j, "I can't be in my own engagedme list." );
		msg.SetSendFrom( GetGoid() );
		msg.SetSendTo( *j );
		Send( msg );
	}
}


void GoMind :: MessageVisible( WorldMessage const & _msg )
{
	WorldMessage msg( _msg );
	msg.SetSendFrom( GetGoid() );

	for( DistanceVisibleMap::iterator ivclear = m_DistanceVisibleMap.begin(); ivclear != m_DistanceVisibleMap.end(); ++ivclear )
	{
		GoHandle visible( (*ivclear).second->m_Object );
		if( visible )
		{
			msg.SetSendTo( visible->GetGoid() );
			Send( msg );
		}
	}
}


void GoMind :: ReqResetSensorsSelfAndVisible()
{
	WorldMessage message( WE_REQ_SENSOR_FLUSH );
	FlushCaches();
	MessageVisible( message );
}


bool GoMind :: Send( WorldMessage const & msg )
{
	GoHandle sendTo( msg.GetSendTo() );
	if( !sendTo )
	{
		gperror( "Trying to send to non-existant Go" );
		return false;
	}

	msg.Send();
	return true;
}


bool GoMind :: SendDelayed( WorldMessage const & msg, float delay )
{
	GoHandle sendTo( msg.GetSendTo() );
	if( !sendTo )
	{
		gperror( "Trying to send to non-existant Go" );
		return false;
	}

	if( !CanOperateOn( sendTo, false ) )
	{
		gperrorf(( "GoMind :: Send - asked to send to bad Go.  Sender = '%s', bad go = '%s'", GetGo()->GetTemplateName(), sendTo->GetTemplateName() ));
		return false;
	}

	if( TestWorldEventTraits( msg.GetEvent(), WMT_GO_IN_FRUSTUM ) && !sendTo->IsInAnyWorldFrustum() )
	{
		return false;
	}
	msg.SendDelayed( delay );
	return true;
}


bool GoMind :: HasJob( JobQueue const & q, Job * job )
{
	return stdx::has_any( q, job );
}


void GoMind :: InsertJob( Job * job, JobReq const & req, bool clearAfterInsertionPoint )
{
	GPPROFILERSAMPLE( "GoMind :: InsertJob 1", SP_AI | SP_AI_MIND );

	////////////////////////////////////////
	//	BEGIN debug
	////////////////////////////////////////
#	if !GP_RETAIL

	if( GetGo()->IsHuded() )
	{
		gpstring msg;
		msg.assignf( "[%2.4f, 0x%08x] GoMind :: InsertJob - inserting '%s' to %s queue\n",
						gWorldTime.GetTime(),
						gWorldTime.GetSimCount(),
						::ToString( job->GetJobAbstractType() ),
						::ToString( req.m_Q ) );
		gpgeneric( msg );
	}
#	endif // !GP_RETAIL

	////////////////////////////////////////
	//	END debug
	////////////////////////////////////////

	gpassert( !job->IsMarkedForDeletion() );

	JobQueue & q = GetQ( req.m_Q );
	gpassert( !HasJob( q, job ) );
	eQPlace place = req.m_QPlace;

	JobQueue::iterator insertionPoint = q.begin();

	while( (insertionPoint != q.end()) && (*insertionPoint)->IsMarkedForDeletion() )
	{
		++insertionPoint;
	}

	if( place == QP_FRONT || place == QP_CLEAR )
	{
		if( place == QP_CLEAR )
		{
			Clear( q );
		}
		insertionPoint = q.begin();
	}
	else if( place == QP_BACK )
	{
		insertionPoint = q.end();
	}
	else if( (place == QP_NONE) && ( req.m_PlaceBefore || req.m_PlaceAfter ) )
	{
		DWORD findId = req.m_PlaceBefore | req.m_PlaceAfter;

		JobQueue::iterator irelative = q.begin();
		while( ( irelative != q.end() ) && ((*irelative)->GetId() != findId ))
		{
			++irelative;
		}

		if( ( irelative != q.end() ) && ( (*irelative)->GetId() == findId ) )
		{
			if( req.m_PlaceBefore )
			{
				insertionPoint = irelative;
			}
			else if( req.m_PlaceAfter )
			{
				++irelative;
				insertionPoint = irelative;
			}
			else
			{
				gperrorf(( "GoMind :: InsertJob - Go '%s' invalid job request for job '%s' - QP_NONE, but Place Before/After not specified.\n", GetGo()->GetTemplateName(), ::ToString( job->GetJobAbstractType() ) ));
				insertionPoint = q.begin();
			}
		}
		else
		{
			gperrorf(( "GoMind :: InsertJob - Go '%s' failed to find valid insertion point for job '%s'\n", GetGo()->GetTemplateName(), ::ToString( job->GetJobAbstractType() ) ));
			insertionPoint = q.begin();
		}
	}
	else
	{
		gpassertm( 0, "invalid request" );
		return;
	}

	bool initJob = true;

	if( insertionPoint == q.begin() )
	{
		initJob = true;
	}
	else
	{
		initJob = false;
	}

	if( req.m_Jat == JAT_BRAIN )
	{
		m_BrainActive = false;
	}

	InsertJob( job, q, insertionPoint, req, clearAfterInsertionPoint );

	if( initJob )
	{
		job->Init();
		PostJobAnyEvent( job );
	}

	////////////////////////////////////////////////////////////////////////
	//	handle action q timeout

	if( ( req.m_Q == JQ_ACTION ) && !q.empty() )
	{
		m_IdleTimer = 0.0;
	}

	////////////////////////////////////////
	//	BEGIN debug
	////////////////////////////////////////
#	if !GP_RETAIL

	if( GetGo()->IsHuded() )
	{
		gpstring msg;
		msg.assignf( "[%2.4f, 0x%08x] GoMind :: InsertJob - inserted '%s' to %s of %s queue\n",
						gWorldTime.GetTime(),
						gWorldTime.GetSimCount(),
						::ToString( job->GetJobAbstractType() ),
						::ToString( place ),
						::ToString( req.m_Q ) );
		gpgeneric( msg );
	}
#	endif // !GP_RETAIL

}


void GoMind :: InsertJob( Job * job, JobQueue & q, JobQueue::iterator const insertionPoint, JobReq const & /*req*/, bool clearAfterInsertionPoint )
{
	GPPROFILERSAMPLE( "GoMind :: InsertJob 2", SP_AI | SP_AI_MIND );

	JobQueue::iterator i = q.begin();

	////////////////////////////////////////
	//	check preceeding jobs
	while( i != insertionPoint )
	{
		if( !(*i)->IsMarkedForDeletion() )
		{
			if( (*i)->GetJobAbstractType() == JAT_FIDGET )
			{
				//(*i)->RequestFinish();
				(*i)->MarkForDeletion( JR_OK );
			}
		}
		++i;
	}

	////////////////////////////////////////
	//	insert job
	bool flushMCP = false;

	if( insertionPoint == q.begin() && ( job->GetTraits() & JT_MOVES_ACTOR ) )
	{
		flushMCP = true;
	}

	{
		GPPROFILERSAMPLE( "GoMind :: InsertJob 2 - collection insert", SP_AI | SP_AI_MIND );
		i = q.insert( insertionPoint, job );
		++i;
	}

	////////////////////////////////////////
	//	deinit trailing/displaced jobs
	{
		GPPROFILERSAMPLE( "GoMind :: InsertJob 2 - deinit jobs", SP_AI | SP_AI_MIND );
		while( i != q.end() )
		{
			if( !(*i)->IsMarkedForDeletion() )
			{
				if( clearAfterInsertionPoint )
				{
					(*i)->MarkForDeletion();
					(*i)->Deinit();
				}
				else
				{
					(*i)->Deinit();
				}
			}
			++i;
		}
	}

	if( flushMCP )
	{
#	if !GP_RETAIL
		if( GetGo()->IsHuded() )
		{
			gpwarningf((	"[%2.4f, 0x%08x] GoMind :: InsertJob - flushing MCP Request to run Job '%s' for actor '%s'\n",
							gWorldTime.GetTime(),
							gWorldTime.GetSimCount(),
							::ToString( job->GetJobAbstractType() ),
							GetGo()->GetTemplateName() ));
		}
#	endif
		gMCP.Flush( GetGoid() );
	}
}


bool GoMind :: CanOperateOn( Go * go, bool silentError ) const
{
	gpassert( go );
	//gpassert( GetGo()->IsInActiveWorldFrustum() );

	if( go->IsActor() )
	{
		if( !go->HasMind() || !go->HasInventory() )
		{
			if( !silentError )
			{
				gperrorf(( "Convention-busting Go found!  '%s', scid 0x%08x has GoActor, but is missing GoMind or GoInventory!", go->GetTemplateName(), go->GetScid() ));
			}
			return false;
		}
	}
	if( go->HasMind() )
	{
		if( !go->HasActor() || !go->HasInventory() )
		{
			if( !silentError )
			{
				gperrorf(( "Convention-busting Go found!  '%s', scid 0x%08x has GoMind, but is missing GoActor or GoInventory!", go->GetTemplateName(), go->GetScid() ));
			}
			return false;
		}
	}

	if( go->IsItem() )
	{
		if( go->HasActor() || go->HasMind() )
		{
			if( !silentError )
			{
				gperrorf(( "Convention-busting Go found!  '%s', scid 0x%08x is item, but has GoActor or GoMind!", go->GetTemplateName(), go->GetScid() ));
			}
			return false;
		}
	}

	if( !go->IsGold() )	// $$ gold is an exception - don't perform this check with gold
	{
		FrustumId myFrustum		= GetGo()->GetWorldFrustumMembership();
		FrustumId theirFrustum	= go->GetWorldFrustumMembership();

		if( m_FrustumOnEntry )
		{
			myFrustum = m_FrustumOnEntry;
		}
		if( go->HasMind() && go->GetMind()->GetFrustumOnEntry() )
		{
			theirFrustum = go->GetMind()->GetFrustumOnEntry();
		}

		if( ( MakeInt( myFrustum ) & MakeInt( go->GetWorldFrustumMembership() ) ) == 0 )
		{
			if( !silentError )
			{
				gperrorf((	"Convention-busting Go found!  '%s', scid 0x%08x is being referenced by %s, and they don't share the same frustum!\n%s frustum = 0x%08x, %s frustum = 0x%08x\n",
							go->GetTemplateName(), go->GetScid(), GetGo()->GetTemplateName(),
							GetGo()->GetTemplateName(),
							myFrustum,
							go->GetTemplateName(),
							theirFrustum ));
			}
			return false;
		}

		if( !GetGo()->IsInActiveWorldFrustum() || !go->IsInActiveWorldFrustum() )
		{
			if( !silentError )
			{
				gperrorf((	"Convention-busting Go found!  '%s', scid 0x%08x is being referenced by %s, and they aren't both in an active frustum!\n%s frustum = 0x%08x, %s frustum = 0x%08x\n",
							go->GetTemplateName(), go->GetScid(), GetGo()->GetTemplateName(),
							GetGo()->GetTemplateName(),
							myFrustum,
							go->GetTemplateName(),
							theirFrustum ));
			}
			return false;
		}
	}

	return ( true );
}


Skrit::HAutoObject const * GoMind :: GetJobSkritCloneSource( eJobAbstractType jat )
{
	for( JobLookupEntryColl::iterator i = m_JobLookupEntryColl.begin(); i != m_JobLookupEntryColl.end(); ++i )
	{
		if( (*i).m_Jat == jat )
		{
			return( &(*i).m_pSlot->second->m_hSkrit );
		}
	}
	return NULL;
}


bool GoMind :: DoingJobOriginatingFrom( eJobQ q, Go * origin )
{
	Job * job = GetFrontJob( q );
	if( job )
	{
		return job->GetNotify() == origin->GetGoid();
	}
	else
	{
		return false;
	}
}


void GoMind :: Remove( Job * behavior, eJobQ q )
{
	JobQueue & queue = GetQ( q );

	JobQueue::iterator i;

	i = find( queue.begin(), queue.end(), behavior );

	if( i != queue.end() )
	{
		m_Deletion.push_back( *i );
		queue.erase( i );
	}
	else
	{
		gpassertm( 0, "Job not found." );
	}
}


Job * GoMind :: GetNextJob( Job * pJob )
{
	JobQueue::iterator i;

	for( i = m_Actions.begin(); i != m_Actions.end(); ++i )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i ) );

		if( *i == pJob )
		{
			++i;
			if( i != m_Actions.end() )
			{
				return *i;
			}
			else{
				return 0;
			}
		}
	}
	return 0;
}


Job * GoMind :: GetFrontJob( eJobQ q ) const
{
	JobQueue const & queue = GetQ( q );
	Job * front = NULL;
	JobQueue::const_iterator i = queue.begin();
	while( i != queue.end() )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i ) );

		if( !(*i)->IsMarkedForDeletion() )
		{
			front = (*i);
			break;
		}
		++i;
	}
	return front;
}


eJobAbstractType GoMind :: GetFrontActionJat() const
{
	Job * job = GetFrontJob( JQ_ACTION );
	if( job )
	{
		return( job->GetJobAbstractType() );
	}
	else
	{
		return( JAT_INVALID );
	}
}


bool GoMind :: IsCurrentActionHumanInterruptable() const
{
	Job * job = GetFrontJob( JQ_ACTION );
	if( job )
	{
		return( ( job->GetTraits() & JT_HUMAN_INTERRUPTABLE ) != 0 );
	}
	else
	{
		return ( true );
	}
}


Goid GoMind :: GetEngagedObject()
{
	Goid engaged = GOID_INVALID;

	if( gServer.IsLocal() )
	{
		Job * current = GetFrontJob( JQ_ACTION );

		if( current )
		{
			if( current->GetTraits() & JT_REQUIRE_GOAL )
			{
				if( GoHandle( current->GetGoalObject() ) )
				{
					engaged = current->GetGoalObject();
				}
			}
		}

		if( m_ClientCachedEngagedObject != engaged )
		{
			RCSetClientCachedEngagedObject( engaged );
		}

		return engaged;
	}
	else
	{
		return m_ClientCachedEngagedObject;
	}
}


void GoMind :: RCSetClientCachedEngagedObject( Goid obj )
{
	FUBI_RPC_THIS_CALL( RCSetClientCachedEngagedObject, RPC_TO_ALL );

	m_ClientCachedEngagedObject = obj;
}


void GoMind :: ExamineEngaged()
{
	CHECK_SERVER_ONLY;

	Goid engagedGoid = GetEngagedObject();

	bool engagedInvalid = false;

	if( engagedGoid != GOID_INVALID )
	{
		GoHandle engaged( engagedGoid );

		if( !engaged || !CanOperateOn( engaged ) )
		{
			engagedInvalid = true;
		}

		if( !engagedInvalid )
		{
			Job * action = GetFrontJob( JQ_ACTION );
			if( action  )
			{
				if( engaged->HasActor() )
				{
					if( ((action->GetTraits() & JT_ENGAGED_UNCONSCIOUS_OK) == 0) && !IsConscious( engaged->GetLifeState() ) )
					{
						engagedInvalid = true;
					}
					else if( ((action->GetTraits() & JT_ENGAGED_GHOST_OK) == 0) && IsGhost( engaged->GetLifeState() ) )
					{
						engagedInvalid = true;
					}
					else if( ((action->GetTraits() & JT_ENGAGED_DEAD_OK) == 0) && IsDead( engaged->GetLifeState() ) )
					{
						engagedInvalid = true;
					}
				}
			}
		}

		if( engagedInvalid )
		{
			// $$ maybe this should really be "engaged_lost"
			WorldMessage msg( WE_ENGAGED_INVALID, GOID_INVALID, GetGoid() );

			Job * brain   = GetFrontJob( JQ_BRAIN );
			if( brain )
			{
				brain->HandleMessage( msg );
			}

			Job * current = GetFrontJob( JQ_ACTION );
			if( current )
			{
				current->HandleMessage( msg );
				PostJobAnyEvent( current, &msg );
			}
		}
	}
}


void GoMind :: RegisterEngagedMe( Goid engagedBy )
{
	gpassertm( m_EngagedMeMap.size() < 32, "Sanity check: you're engaged by a horde of actors!" );

	// don't acknowladge self-engagement
	if( engagedBy == GetGoid() )
	{
		return;
	}

	GoHandle engaged( engagedBy );
	if( !CanOperateOn( engaged, false ) )
	{
		return;
	}

	EngagedMeMap::iterator i = m_EngagedMeMap.find( engagedBy );

	if( i != m_EngagedMeMap.end() )
	{
		(*i).second += 1;
	}
	else
	{
		m_EngagedMeMap.insert( EngagedMeMap::value_type( engagedBy, 1 ) );
	}
}


void GoMind :: UnregisterEngagedMe( Goid engagedBy )
{
	// don't acknowladge self-engagement
	if( engagedBy == GetGoid() )
	{
		return;
	}

	EngagedMeMap::iterator i = m_EngagedMeMap.find( engagedBy );

	if( i != m_EngagedMeMap.end() )
	{
		(*i).second -= 1;
		if( (*i).second == 0 )
		{
			m_EngagedMeMap.erase( i );
		}
	}
	else
	{
		gperror( "RegisterEngagedMe/UnegisterEngagedMe mismatch." );
	}
}


void GoMind :: SetIsRidingElevator()
{
	m_LastElevatorRideTime = gWorldTime.GetTime();
}


bool GoMind :: IsRidingElevator() const
{
	return( PreciseSubtract( gWorldTime.GetTime(), m_LastElevatorRideTime ) <= gWorldOptions.GetDefaultSensorScanPeriod() );
}


bool GoMind :: Validate()
{
	bool success = true;

//#	if !GP_RETAIL

	Job * brain		= GetFrontJob( JQ_BRAIN );
	Job * current	= GetFrontJob( JQ_ACTION );

	////////////////////////////////////////
	//	misc

#	if !GP_RETAIL
	if( ( brain || current ) && GetGo()->IsInAnyWorldFrustum() )
	{
		if( !GetGo()->GetPlacement()->VerifyPosition() )
		{
			gpwarningf(( "GoMind - Actor '%s', Scid 0x%08x has found his position to be in an unloaded node.  Restoring honor through self-destruction.\n", GetGo()->GetTemplateName(), GetGo()->GetScid() ));
			Clear();
			success = false;
		}
	}
#	endif

	////////////////////////////////////////
	//	validate engaged list

	for( EngagedMeMap::iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); )
	{
		if( (*i).first == GetGoid() )
		{
			gperror( "Found self in engagedme list" );
			i = m_EngagedMeMap.erase( i );
			continue;
		}

		GoHandle engaged( (*i).first );
		if( !engaged )
		{
			gpwarningf(( "'%s' found dud in engaged list.  Goid = 0x%08x\n", GetGo()->GetTemplateName(), (*i).first ));
			i = m_EngagedMeMap.erase( i );
			continue;
		}

		if( !CanOperateOn( engaged ) )
		{
			gpwarningf(( "Found convention violator in engaged list.  Object in list = %s, scid 0x%08x\n", engaged->GetTemplateName(), engaged->GetScid() ));
			//i = m_EngagedMeMap.erase( i );
		}
		++i;
	}

	if( m_GoidVisibleMap.size() != m_DistanceVisibleMap.size() )
	{
		gperror( "m_GoidVisibleMap.size() != m_DistanceVisibleMap.size()" );
	}

//#	endif // !GP_RETAIL

	return success;
}


void GoMind :: ClearTemps()
{
	GPPROFILERSAMPLE( "GoMind :: ClearTemps", SP_AI | SP_AI_MIND );

	////////////////////////////////////////
	//	clear mind temp vars

	if( !m_TempGopColl1.empty() )
	{
		m_TempGopColl1.clear();
	}
	if( !m_TempGopColl2.empty() )
	{
		m_TempGopColl2.clear();
	}
	if( !m_TempGopColl3.empty() )
	{
		m_TempGopColl3.clear();
	}

	////////////////////////////////////////
	//	clear party temp vars

	if( GetGo()->HasParty() )
	{
		if( !GetGo()->GetParty()->GetTempGopCollA().empty() )
		{
			GetGo()->GetParty()->GetTempGopCollA().clear();
		}
		if( !GetGo()->GetParty()->GetTempGopCollB().empty() )
		{
			GetGo()->GetParty()->GetTempGopCollB().clear();
		}
		if( !GetGo()->GetParty()->GetTempGopCollC().empty() )
		{
			GetGo()->GetParty()->GetTempGopCollC().clear();
		}
	}

	////////////////////////////////////////
	//	clear misc

	if( !m_TempQtColl1.empty() )
	{
		m_TempQtColl1.clear();
	}

	m_TempPos1		= SiegePos::INVALID;
	m_TempVector1	= vector_3::ZERO;
}


void GoMind :: DeleteJobsNotActingInFrustum( FrustumId const frustumId )
{
	JobQueue & q = GetQ( JQ_ACTION );

	Scid secommand = SCID_INVALID;

	for( JobQueue::iterator i = q.begin(); i != q.end(); ++i )
	{
		if( (*i)->IsMarkedForDeletion() )
		{
			continue;
		}

		if( (*i)->GetTraits() & JT_REQUIRE_GOAL )
		{
			if( (*i)->GetStartedBySECommand() && ( secommand != SCID_INVALID ) )
			{
				secommand = (*i)->GetStartedBySECommand();
			}

			GoHandle goal( (*i)->GetGoalObject() );

			if( goal )
			{
				if( (MakeInt( frustumId ) & MakeInt( goal->GetWorldFrustumMembership() )) == 0 )
				{
					if( !(*i)->IsCleaningUp() )
					{
						(*i)->Deinit();
						(*i)->MarkForDeletion();
					}
					continue;
				}
			}
		}

		if( (*i)->GetTraits() & JT_REQUIRE_POSITION )
		{
			if( ( MakeInt( frustumId ) & gSiegeEngine.GetNodeFrustumOwnedBitfield( (*i)->GetGoalPosition().node )  ) == 0 )
			{
				if( !(*i)->IsCleaningUp() )
				{
					(*i)->Deinit();
					(*i)->MarkForDeletion();
				}
				continue;
			}
		}
	}
}


void GoMind :: PrivateHandleMessage( WorldMessage const & Message )
{
	gpassert( m_pHandlingMessage == NULL );
	m_pHandlingMessage = &const_cast<WorldMessage&>(Message);

	ClearTemps();

	bool shortCircuit = false;

	if( IsMCPMessage( Message.GetEvent() ) && !GetGo()->IsInActiveWorldFrustum() )
	{
		shortCircuit = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	handle messages intended for this Go

	if( Message.IsCC() )
	{
		if( Message.HasEvent( WE_TERRAIN_TRANSITION_DONE ) )
		{
			FlushCaches();
		}
	}
	else
	{
#if		!GP_RETAIL

		if( Message.HasEvent( WE_ENGAGED_LOST_CONSCIOUSNESS ) )
		{
			m_DebugWeEngagedLostConsciousnessRecevied = true;
		}
		else if( Message.HasEvent( WE_KILLED ) )
		{
			m_DebugWeKilledReceived = true;
		}
		else if( Message.HasEvent( WE_LOST_CONSCIOUSNESS ) )
		{
			m_DebugWeLostConsciousnessReceived = true;
		}

#		endif

		switch( Message.GetEvent() )
		{
			case WE_CONSTRUCTED:
			{
				break;
			}

			case WE_DESTRUCTED:
			{
				m_ShuttingDown = true;
				Clear();
				break;
			}

			case WE_LEFT_WORLD:
			{
				//gpgenericf(( "'%s' WE_LEFT_WORLD\n", GetGo()->GetTemplateName() ));

				WorldMessage msg( WE_ENGAGED_INVALID );
				MessageEngagedMe( msg );

				if( GetFrontActionJat() == JAT_STARTUP )
				{
					m_ReqGiveStartupJob = true;
				}

				DeleteJobsNotActingInFrustum( MakeFrustumId( 0 ) );
				Clear();
				MoveMarkedJobsToDeletionQ( true );
				ProcessDeletionQueue();

				gpassert( m_Brains.empty() );
				gpassert( m_Actions.empty() );

				m_ReqInitBrain = true;	// for if we ever come back into the world

				//m_Brains;
				m_Actions.destroy();
				m_Deletion.destroy();
				//m_EngagedMeMap.clear();

				m_ActorsInInnerComfortZone.destroy();
				m_ActorsInOuterComfortZone.destroy();
				m_EnemiesVisible.destroy();
				m_FriendsVisible.destroy();
				m_ItemsVisible.destroy();
				m_PathBlockers.destroy();

				FlushCaches();
				m_ActionResultColl.destroy();

				//m_DistanceVisibleMap.clear();
				//m_GoidVisibleMap.clear();

				m_TempGopColl1.destroy();
				m_TempGopColl2.destroy();
				m_TempGopColl3.destroy();

				//m_DelayedMessages.destroy();
				//m_JobLookupEntryColl.destroy();
				break;
			}

			case WE_ENTERED_WORLD:
			{
				//gpgenericf(( "'%s' WE_ENTERED_WORLD\n", GetGo()->GetTemplateName() ));

				// make sure all jobs have been deinitialized

				if( !m_EngagedMeMap.empty() )
				{
					bool allDuds = true;
					for( EngagedMeMap::iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
					{
						GoHandle dud( (*i).first );
						if( dud )
						{
							allDuds = false;
							break;
						}
					}

					gpwarningf(( "Tell Bart: on WE_ENTERED_WORLD, !m_EngagedMeMap.empty(), all duds = %s\n", allDuds ? "TRUE" : "FALSE" ));
				}

#				if !GP_RETAIL
				JobQueue & q = GetQ( JQ_ACTION );
				for( JobQueue::iterator ij = q.begin(); ij != q.end(); ++ij )
				{
					if( (*ij)->IsInitialized() )
					{
						gperrorf(( "GoMind has initialized jobs on world entry.  Go = %s, scid = 0x%08x, job = %s", GetGo()->GetTemplateName(), GetGo()->GetScid(), ::ToString( (*ij)->GetJobAbstractType() ) ));
					}
				}
#				endif

				m_FrustumOnEntry = MakeFrustumId( Message.GetData1() );

				if( gServer.IsLocal() && GetGo()->HasBody() )
				{
					if( gSiegeEngine.GetLogicalNodeAtPosition( GetGo()->GetPlacement()->GetPosition(), siege::LF_ALL ) )
					{
						if( !gSiegeEngine.GetLogicalNodeAtPosition( GetGo()->GetPlacement()->GetPosition(), GetGo()->GetBody()->GetTerrainMovementPermissions() ) )
						{
							gperrorf(( "BAD CONTENT: Go '%s' scid:0x%08x is entering world with a position it does not have permission to be in.\n",
											GetGo()->GetTemplateName(),
											GetGo()->GetScid() ));
						}
					}
				}

				m_MyPositionLastVisibilityCheck = GetGo()->GetPlacement()->GetPosition();

				if( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && ( m_LastSECommand != SCID_INVALID ) && ( GetGo()->GetLifeState() == LS_ALIVE_CONSCIOUS ) )
				{
					m_ReqDoLastSECommand = true;
				}

				m_FrustumOnEntry = 0;
				break;
			}

			case WE_FRUSTUM_MEMBERSHIP_CHANGED:
			{
				OnFrustumMembershipChanged( MakeFrustumId( Message.GetData1() ));

				FrustumId oldMembership		= MakeFrustumId( Message.GetData1() );
				FrustumId currentActive		= gGoDb.GetActiveFrustumMask();
				FrustumId currentMembership	= GetGo()->GetWorldFrustumMembership();

				if( !BitFlagsContainAny( oldMembership, currentActive ) && BitFlagsContainAny( currentMembership, currentActive ) )
				{
					OnFrustumGainedActiveState();
				}
				else if( BitFlagsContainAny( oldMembership, currentActive ) && !BitFlagsContainAny( currentMembership, currentActive ) )
				{
					OnFrustumLostActiveState();
				}

				shortCircuit = true;
				break;
			}

			case WE_FRUSTUM_ACTIVE_STATE_CHANGED:
			{
				FrustumId oldActive			= MakeFrustumId( Message.GetData1() );
				FrustumId currentActive		= gGoDb.GetActiveFrustumMask();
				FrustumId actorMembership	= GetGo()->GetWorldFrustumMembership();

				if( !BitFlagsContainAny( oldActive, actorMembership ) && BitFlagsContainAny( currentActive, actorMembership ) )
				{
					// going from inactive frustum to active
					OnFrustumGainedActiveState();
				}
				else if( BitFlagsContainAny( oldActive, actorMembership ) && !BitFlagsContainAny( currentActive, actorMembership ) )
				{
					// going from active frustum to inactive
					OnFrustumLostActiveState();
				}

				shortCircuit = true;
				break;
			}

			case WE_PLAYER_CHANGED:
			{
				if( GetGo()->GetPlayer()->GetController() == PC_COMPUTER )
				{
					if( ((Player*)Message.GetData1())->GetController() == PC_HUMAN )
					{
						m_ActorAutoSwitchesToMelee	= ( m_WeaponPreference == WP_MELEE	);
						m_ActorAutoSwitchesToRanged	= ( m_WeaponPreference == WP_RANGED );
						m_ActorAutoSwitchesToMagic	= ( m_WeaponPreference == WP_MAGIC	);
					}
				}
				break;
			}

			case WE_REQ_DIE:
			{
				Validate();

				gRules.ChangeLife( GetGo()->GetGoid(), -2.0f * GetGo()->GetAspect()->GetCurrentLife(), RPC_TO_ALL );
				break;
			}

			case WE_KILLED:
			{
				Validate();

				if( !IsAlive( GetGo()->GetLifeState() ) )
				{
					// clean up behaviors if this actor just got snuffed
					// play the die chore if we are in single player
					if( IsSinglePlayer() || GetGo()->GetPlayer()->GetController() == PC_COMPUTER )
					{
						if( GetGo()->GetMind()->UnderstandsJob( JAT_DIE ) )
						{
							GetGo()->GetMind()->SDoJob( MakeJobReq( JAT_DIE, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
						}
						else
						{	
							gperrorf(( "Actor %s, scid 0x%08x - doesn't understand JAT_DIE but needs to.\n", GetGo()->GetTemplateName(), GetScid() ));
							gMCPManager.Flush( GetGoid() );
							Clear();
						}
					}					
					else 
					{
						Job * current = GetFrontJob( JQ_ACTION );

						if( !GetGo()->GetMind()->UnderstandsJob( JAT_UNCONSCIOUS ) )
						{
							gperrorf(( "Actor %s, scid 0x%08x - doesn't understand JAT_UNCONSCIOUS but needs to.\n", GetGo()->GetTemplateName(), GetScid() ));
							gMCPManager.Flush( GetGoid() );
							Clear();
						}
						else if( !current || ( current && ( current->GetJobAbstractType() != JAT_UNCONSCIOUS ) ) )
						{
							GetGo()->GetMind()->SDoJob( MakeJobReq( JAT_UNCONSCIOUS, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
						}	
					}
				}
				else
				{
					gpwarningf(( "Somebody is sending a WE_KILLED message to %s, Scid 0x%08x, who is still alive.  Ignored", GetGo()->GetTemplateName(), GetScid() ));
				}
				break;
			}

			case WE_LOST_CONSCIOUSNESS:
			{
				Validate();

				GetGo()->GetMind()->SDoJob( MakeJobReq( JAT_UNCONSCIOUS, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
				break;
			}

			case WE_GAINED_CONSCIOUSNESS:
			{
				Validate();

				Job * current = GetFrontJob( JQ_ACTION );
				if( !current || current->GetJobAbstractType() != JAT_UNCONSCIOUS )
				{
					GetGo()->GetMind()->SDoJob( MakeJobReq( JAT_GAIN_CONSCIOUSNESS, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
				}
				break;
			}

			case WE_ENEMY_SPOTTED:
			{
				Validate();

				if ( !m_bEnemySpottedPlayed )
				{
					GetGo()->RCPlayVoiceSoundId( RPC_TO_ALL, VS_ENEMY_SPOTTED );
					m_bEnemySpottedPlayed = true;
				}
				break;
			}

			case WE_REQ_SENSOR_FLUSH:
			{
				FlushCaches();
				break;
			}
			
			case WE_AI_ACTIVATE:
			{
				SetMayProcessAI( true );
				break;
			}
			
			case WE_AI_DEACTIVATE:
			{
				SetMayProcessAI( false );
				break;
			}

			default:
			{
				if( IsEngagedEvent( Message.GetEvent() ) )
				{
					if( Message.HasEvent( WE_ENGAGED_HIT_KILLED ) ||
							Message.HasEvent( WE_ENGAGED_INVALID ) ||
							( Message.HasEvent( WE_ENGAGED_FLED ) && !OnEngagedFleedAbortAttack() ) ||
							( Message.HasEvent( WE_ENGAGED_LOST_CONSCIOUSNESS ) && OnEngagedLostConsciousnessAbortAttack() ) )
					{
						// $$$ this is infrequent, but might still want to optimize -bk
						Goid engaged = GetEngagedObject();

						GoidColl::iterator i;
						i = std::find( m_ActorsInInnerComfortZone.begin(), m_ActorsInInnerComfortZone.end(), engaged );
						if( i != m_ActorsInInnerComfortZone.end() )
						{
							m_ActorsInInnerComfortZone.erase( i );
						}

						i = std::find( m_ActorsInOuterComfortZone.begin(), m_ActorsInOuterComfortZone.end(), engaged );
						if( i != m_ActorsInOuterComfortZone.end() )
						{
							m_ActorsInOuterComfortZone.erase( i );
						}

						i = std::find( m_EnemiesVisible.begin(), m_EnemiesVisible.end(), engaged );
						if( i != m_EnemiesVisible.end() )
						{
							m_EnemiesVisible.erase( i );
						}

						i = std::find( m_FriendsVisible.begin(), m_FriendsVisible.end(), engaged );
						if( i != m_FriendsVisible.end() )
						{
							m_FriendsVisible.erase( i );
						}

						i = std::find( m_ItemsVisible.begin(), m_ItemsVisible.end(), engaged );
						if( i != m_ItemsVisible.end() )
						{
							m_ItemsVisible.erase( i );
						}
					}
				}
				else if( Message.HasEvent( WE_TERRAIN_TRANSITION_DONE ) )
				{
					FlushCaches();
					MessageEngagedMe( WorldMessage( WE_REQ_SENSOR_FLUSH, GOID_INVALID, GetGoid() ) );
				}
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	handle all messages

	if( !shortCircuit )
	{
		Job * brain		= GetFrontJob( JQ_BRAIN );
		Job * current	= GetFrontJob( JQ_ACTION );

		if( ( brain || current ) && !IsEngagedEvent( Message.GetEvent() ) && (Message.GetEvent() != WE_LEFT_WORLD) )
		{
			ExamineEngaged();
		}

		brain = GetFrontJob( JQ_BRAIN );
		if( brain )
		{
			if( brain->IsInitialized() )
			{
				brain->HandleMessage( Message );
			}
		}

		current = GetFrontJob( JQ_ACTION );

		if( current )
		{
			if( current->IsInitialized() )
			{
				current->HandleMessage( Message );
				PostJobAnyEvent( current, &Message );
			}
		}
	}

	m_pHandlingMessage = NULL;
}


void GoMind :: OnFrustumMembershipChanged( FrustumId const /*oldMembership*/ )
{
	// to prevent list re-entry problems:

	GoidColl engaged;
	EngagedMeMap::iterator i;
	for( i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		engaged.push_back( (*i).first );
	}

	// now message all the happy enagege-ees

	for( GoidColl::iterator j = engaged.begin(); j != engaged.end(); ++j )
	{
		GoHandle engaged( *j );

		if( !engaged )
		{
			continue; // $$ this should be avoided
		}

		if( engaged && engaged->HasMind() && ( engaged->GetMind()->GetEngagedObject() != GetGoid() ) )
		{
			continue;
		}

		gpassertm( GetGoid() != *j, "I can't be in my own engagedme list." );
		engaged->GetMind()->DeleteJobsNotActingInFrustum( engaged->GetWorldFrustumMembership() );
	}

	DeleteJobsNotActingInFrustum( GetGo()->GetWorldFrustumMembership() );
}


void GoMind :: OnFrustumGainedActiveState()
{
	Job * action = GetFrontJob( JQ_ACTION );
	if( action && !action->IsInitialized() )
	{
		gMCPManager.Flush( GetGoid() );
		action->Init();
	}

	Job * brain = GetFrontJob( JQ_BRAIN );
	if( brain && !brain->IsInitialized() )
	{
		brain->Init();
	}
}


void GoMind :: OnFrustumLostActiveState()
{
	bool shutdownActions = false;

	for( JobQueue::iterator ia = m_Actions.begin(); ia != m_Actions.end(); ++ia )
	{
		if( !(*ia)->IsMarkedForDeletion() && (*ia)->IsInitialized() && ( (*ia)->IsRegisteredWithEngaged() || ((*ia)->GetTraits() & JT_MOVES_ACTOR) ) )
		{
			(*ia)->Deinit();
			gMCPManager.Flush( GetGoid() );
			shutdownActions = true;
			break;
		}
	}

	if( shutdownActions )
	{
		for( JobQueue::iterator ib = m_Brains.begin(); ib != m_Brains.end(); ++ib )
		{
			if( !(*ib)->IsMarkedForDeletion() && (*ib)->IsInitialized() )
			{
				(*ib)->Deinit();
				break;
			}
		}

		// to prevent list re-entry problems:

		GoidColl engaged;
		EngagedMeMap::iterator i;
		for( i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
		{
			engaged.push_back( (*i).first );
		}

		// now message all the happy enagege-ees

		FrustumId currentActive		= gGoDb.GetActiveFrustumMask();

		for( GoidColl::iterator j = engaged.begin(); j != engaged.end(); ++j )
		{
			GoHandle engaged( *j );

			if( !engaged )
			{
				continue; // $$ this should be avoided
			}

			if( engaged && engaged->HasMind() && ( engaged->GetMind()->GetEngagedObject() != GetGoid() ) )
			{
				continue;
			}

			gpassertm( GetGoid() != *j, "I can't be in my own engagedme list." );

			FrustumId engagedId = engaged->GetWorldFrustumMembership();
			if( BitFlagsContainAny( engagedId, gGoDb.GetActiveFrustumMask() ) )
			{
				engaged->GetMind()->DeleteJobsNotActingInFrustum( currentActive	);
			}
		}
	}
}


void GoMind :: MoveMarkedJobsToDeletionQ( bool leavingWorld )
{
	gpassert( !m_InQueueDeletionScope );

	m_InQueueDeletionScope = true;

	////////////////////////////////////////////////////////////////////////////////
	//	actions

	DWORD actionSizeBefore = m_Actions.size();

	JobQueue::iterator i;
	for( i = m_Actions.begin(); i != m_Actions.end(); )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i ) );

		#if !GP_RETAIL
		DWORD debugSize = m_Actions.size();
		#endif

		if( (*i)->IsMarkedForDeletion() && !(*i)->InUpdateScope() )
		{
			gpassert( !(*i)->InUpdateScope() );

			Job * job = *i;

			// if this is a repeating job, just shove it to the back of the queue
			if( job->GetTraits() & JT_REPEATING )
			{
				job->UnmarkForDeletion();
				job->Deinit();

				gpassert( debugSize == m_Actions.size() );

				gpassert( HasJob( m_Actions, job ) );
				m_Actions.erase( i );
				gpassert( !HasJob( m_Actions, job ) );

				InsertJob( job, MakeJobReq( job->GetJobAbstractType(), JQ_ACTION, QP_BACK, job->GetOrigin() ), false );
				gpassert( debugSize == m_Actions.size() );

				if( job->IsMarkedForDeletion() )
				{
					gpwarningf(( "[%2.4f, 0x%08x] - Repeating job '%s' for '%s' scid:0x%08x failed re-initalization.  Will be deleted.\n",
									gWorldTime.GetTime(),
									gWorldTime.GetSimCount(),
									::ToString( job->GetJobAbstractType() ),
									GetGo()->GetTemplateName(),
									GetGo()->GetScid() ));

					JobQueue::iterator id = stdx::find( m_Actions, job );
					m_Actions.erase( id );
					m_Deletion.push_back( job );
				}

				i = m_Actions.begin();
			}
			else
			{
				gpassert( debugSize == m_Actions.size() );

				if( (job->GetTraits() & ( JT_HUMAN_ASSIGNED | JT_COMMAND_ASSIGNED )) && ( i == m_Actions.begin() ) )
				{
					Plan * plan = gMCPManager.FindPlan( GetGoid(), false );
					if( plan )
					{
						m_LastExecutedUserAssignedActionPosition =  plan->GetFinalDestination();
					}
					else
					{
						m_LastExecutedUserAssignedActionPosition =  GetGo()->GetPlacement()->GetPosition();
					}
				}

				RegisterActionResult( job );

				if( m_ShuttingDown || leavingWorld )
				{
					job->HandleMessage( WorldMessage( WE_DESTRUCTED, GOID_INVALID, GetGoid(), MakeInt(GetGoid()) ) );
				}
				else
				{
					job->HandleMessage( WorldMessage( WE_JOB_DESTRUCTED, GOID_INVALID, GetGoid(), MakeInt(GetGoid()) ) );
				}

				gpassert( debugSize == m_Actions.size() );

				#if !GP_RETAIL
				if( debugSize != m_Actions.size() )
				{
					gperrorf(( "Go '%s', job '%s' misbehaved on deletion.", GetGo()->GetTemplateName(), ::ToString( job->GetJobAbstractType() ) ));
				}
				#endif

				job->Deinit();
				gpassert( debugSize == m_Actions.size() );

				gpassert( !HasJob( m_Deletion, job ) );
				m_Deletion.push_back( job );
				gpassert( HasJob( m_Deletion, job ) );

				gpassert( HasJob( m_Actions, job ) );
				i = m_Actions.erase( i );
				gpassert( !HasJob( m_Actions, job ) );

			}

			// notify the brain that a job was finished
			#if !GP_RETAIL
			debugSize = m_Actions.size();
			#endif

			//WorldMessage msg( WE_JOB_FINISHED, GetGo()->GetGoid(), GetGo()->GetGoid(), MakeInt( GetGo()->GetGoid() ) );
			//SendDelayed( msg, 0 );
			gpassert( debugSize == m_Actions.size() );
		}
		else
		{
			++i;
		}
	}

	// brains
	for( i = m_Brains.begin(); i != m_Brains.end(); )
	{
		gpassert( !IsMemoryBadFood( (DWORD)*i ) );

		if( (*i)->IsMarkedForDeletion() && !(*i)->InUpdateScope() )
		{
			Job * job = *i;

			job->Deinit();

			gpassert( !HasJob( m_Deletion, job ) );
			m_Deletion.push_back( job );
			gpassert( HasJob( m_Deletion, job ) );

			gpassert( HasJob( m_Brains, job ) );
			i = m_Brains.erase( i );
			gpassert( !HasJob( m_Brains, job ) );
		}
		else
		{
			++i;
		}
	}


	////////////////////////////////////////////////////////////////////////////////
	//	check for action q emptied

	bool emptied = false;

	Job * action = GetFrontJob( JQ_ACTION );

	if( !action )
	{
		emptied = actionSizeBefore > 0;
	}
	else if ( action && action->GetJobAbstractType() == JAT_FIDGET )
	{
		emptied = actionSizeBefore > 1;
	}

	if( emptied )
	{
		Job * brain = GetFrontJob( JQ_BRAIN );
		if( brain )
		{
			Send( WorldMessage( WE_MIND_ACTION_Q_EMPTIED, GOID_INVALID, GetGoid() ) );
		}
	}

	m_InQueueDeletionScope = false;
}


void GoMind :: ProcessSensors()
{
	GPPROFILERSAMPLE( "GoMind :: ProcessSensors", SP_AI | SP_AI_MIND );

	// dead men don't process sensors
	if( !IsAlive( GetGo()->GetLifeState() ) )
	{
		return;
	}

	if( !m_MayProcessAI )
	{
		return;
	}

	Job * brain = GetFrontJob( JQ_BRAIN );

	bool allowBrainUpdate = ( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && TestOptions( OPTION_UPDATE_COMPUTER_BRAINS ) ) ||
							( ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) && TestOptions( OPTION_UPDATE_HUMAN_BRAINS ) );

	bool allowActionUpdate =	( ( GetGo()->GetPlayer()->GetController() == PC_COMPUTER ) && TestOptions( OPTION_UPDATE_COMPUTER_ACTIONS ) ) ||
								( ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) && TestOptions( OPTION_UPDATE_HUMAN_ACTIONS ) );

	Job * action = GetFrontJob( JQ_ACTION );

	if( (!allowBrainUpdate && !allowActionUpdate) || (!brain && !action) )
	{
		return;
	}

	////////////////////////////////////////
	//	process base JAT change

	eJobAbstractType currentBaseJat = JAT_NONE;
	if( action )
	{
		currentBaseJat = GetBaseJat( action->GetJobAbstractType() );
	}
	if( m_LastCurrentBaseJat != currentBaseJat )
	{
		m_LastCurrentBaseJat = currentBaseJat;
		Send( WorldMessage( WE_JOB_CURRENT_ACTION_BASE_JAT_CHANGED, GOID_INVALID, GetGoid(), scast<DWORD>( currentBaseJat ) ) );
	}

	//////////////////////////////////////////////
	//	early-out if it's not time to process

	if( m_NextSensorScanTime > gWorldTime.GetTime() )
	{
		return;
	}
	else
	{
		m_NextSensorScanTime = PreciseAdd( gWorldTime.GetTime(), (m_SensorScanPeriod + Random( 0.02f )) );
	}

#if !GP_RETAIL
	if( !gWorldOptions.GetAICaching() )
	{
		FlushCaches();
	}
#endif

	//////////////////////////////////////////////
	//	run sensors appropriate to situation

	if( brain || action )
	{
		UpdateJobResults();
	}

	if( brain )
	{
		ProcessStatsSensors();
	}

	action = GetFrontJob( JQ_ACTION );

	if( action )
	{
		if( ( action->GetTraits() & JT_USE_SPATIAL_SENSORS ) || ( GetGo()->GetPlayer()->GetController() == PC_HUMAN ) )
		{
			ProcessSpatialSensors();
		}
	}
	else if( brain )
	{
		ProcessSpatialSensors();
	}

	if( m_RequestFlushSensors )
	{
		FlushCaches();
		m_RequestFlushSensors = false;
	}
}


void GoMind :: ProcessStatsSensors()
{
	GPPROFILERSAMPLE( "GoMind :: ProcessStatsSensors", SP_AI | SP_AI_MIND );

	if( GetGoidClass( GetGoid() ) == GO_CLASS_CLONE_SRC )
	{
		return;
	}

	//////////////////////////////////////////////
	//	check for initial command assignment

	if( m_ReqGiveStartupJob )
	{
		JobReq & req = MakeJobReq( JAT_STARTUP, JQ_ACTION, QP_CLEAR, AO_REFLEX );
		if( SDoJob( req ) )
		{
			m_ReqGiveStartupJob = false;
		}
	}

	if( m_GiveInitialSECommand && !AmBusy() )
	{
		JobReq & req = MakeJobReq( JAT_DO_SE_COMMAND, JQ_ACTION, QP_CLEAR, AO_COMMAND );
		req.SetInt1( MakeInt( m_InitialSECommand ) );
		if( SDoJob( req ) )
		{
			m_GiveInitialSECommand = false;
		}
	}

	if( !m_ReqGiveStartupJob && !m_GiveInitialSECommand && !m_BrainActive )
	{
		m_BrainActive = true;
	}

	////////////////////////////////////////
	//	process low life
	if( ( GetGo()->GetAspect()->GetLifeRatio() <= m_ActorLifeRatioLowThreshold ) &&
		( m_ActorLifeRatioLast > m_ActorLifeRatioLowThreshold ) )
	{
		Send( WorldMessage( WE_LIFE_RATIO_REACHED_LOW, GOID_INVALID, GetGoid() ) );
	}
	////////////////////////////////////////
	//	process high life
	if( ( GetGo()->GetAspect()->GetLifeRatio() >= m_ActorLifeRatioHighThreshold ) &&
		( m_ActorLifeRatioLast < m_ActorLifeRatioHighThreshold ) )
	{
		Send( WorldMessage( WE_LIFE_RATIO_REACHED_HIGH, GOID_INVALID, GetGoid() ) );
	}
	m_ActorLifeRatioLast = GetGo()->GetAspect()->GetLifeRatio();
	////////////////////////////////////////
	//	process low mana
	if( ( GetGo()->GetAspect()->GetManaRatio() <= m_ActorManaRatioLowThreshold ) &&
		( m_ActorManaRatioLast > m_ActorManaRatioLowThreshold ) )
	{
		Send( WorldMessage( WE_MANA_RATIO_REACHED_LOW, GOID_INVALID, GetGoid() ) );
	}
	////////////////////////////////////////
	//	process high mana
	if( ( GetGo()->GetAspect()->GetManaRatio() >= m_ActorManaRatioHighThreshold ) &&
		( m_ActorManaRatioLast < m_ActorManaRatioHighThreshold ) )
	{
		Send( WorldMessage( WE_MANA_RATIO_REACHED_HIGH, GOID_INVALID, GetGoid() ) );
	}
	m_ActorManaRatioLast = GetGo()->GetAspect()->GetManaRatio();

	////////////////////////////////////////
	//	process ENGAGED object visibility timeout
	Job * current = GetFrontJob( JQ_ACTION );
	if( current )
	{
		Goid engaged = GetEngagedObject();

		if( engaged != GOID_INVALID )
		{
			GoidColl::iterator ien = find( m_EnemiesVisible.begin(), m_EnemiesVisible.end(), engaged );	// $$$ optimize - inline in master visible list build loop above -bk
			GoidColl::iterator ifr = find( m_FriendsVisible.begin(), m_FriendsVisible.end(), engaged );	// $$$ optimize - inline in master visible list build loop above -bk

			if( (ien != m_EnemiesVisible.end()) || (ifr != m_FriendsVisible.end()) )
			{
				current->SetTimeEngagedLastVisible( gWorldTime.GetTime() );
			}
		}

		if( ( PreciseSubtract(gWorldTime.GetTime(), current->GetTimeEngagedLastVisible()) ) >= m_VisibilityMemoryDuration )
		{
			Send( WorldMessage( WE_ENGAGED_LOST, GOID_INVALID, GetGoid() ) );
			current->SetTimeEngagedLastVisible( DBL_MAX );
		}
	}

	////////////////////////////////////////
	//	process job travel distance					$$ this is a spatial sensor... but it can run all the time...
	if( m_JobTravelDistanceLimit > 0 )
	{
		Job * action = GetFrontJob( JQ_ACTION );
		if( action && ( m_LastJobReachedTravelDistanceId != action->GetId() ) )
		{
			if( action->GetJobTravelDistance() >= m_JobTravelDistanceLimit )
			{
				Send( WorldMessage( WE_JOB_REACHED_TRAVEL_DISTANCE, GOID_INVALID, GetGoid() ) );
				m_LastJobReachedTravelDistanceId = action->GetId();
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
void GoMind :: ProcessCombatSensors()
{
	// if I don't have brains to process the signals, don't even bother
	if( GetFrontJob( JQ_BRAIN ) == NULL )
	{
		return;
	}

	if( !m_RequestProcessCombatSensors )
	{
		return;
	}
	m_RequestProcessCombatSensors = false;
}


////////////////////////////////////////////////////////////////////////////////
//
void GoMind :: ProcessSpatialSensors()
{
	GPPROFILERSAMPLE( "GoMind :: ProcessSpatialSensors", SP_AI | SP_AI_MIND );

	m_TempGopColl1.clear();
	m_TempGopColl2.clear();

	m_AliveFriendsVisibleDirty = true;
	m_AliveEnemiesVisibleDirty = true;

	m_PathBlockers.clear();

	GetOccupantsInSphere( GetSightRange(), m_TempGopColl1 );

	////////////////////////////////////////
	//	filter out shit we don't care about
	for( GopColl::iterator ix = m_TempGopColl1.begin(); ix != m_TempGopColl1.end(); ++ix )
	{
		// ignore self
		if( ( (*ix) == GetGo() ) ||
			( !(*ix)->HasAspect() ) ||
			( !(*ix)->GetAspect()->GetIsVisible() ) ||
			( (*ix)->IsInsideInventory()) ||
			( ( (*ix)->IsItem() && !(*ix)->HasGui() && (!(*ix)->GetAspect()->GetDoesBlockPath() && !(*ix)->GetAspect()->GetHasBlockedPath()) )) ||
			( (*ix)->IsSpell() && (*ix)->HasParent() ) )
		{
			continue;
		}

		if( !CanOperateOn( *ix ) )
		{
			continue;
		}

		if( (*ix)->HasAspect() && ((*ix)->GetAspect()->GetDoesBlockPath() || (*ix)->GetAspect()->GetHasBlockedPath()) )
		{
			m_PathBlockers.push_back( (*ix)->GetGoid() );
		}

		// ignore unconscious actors, if we must
		if( (*ix)->HasActor() )
		{
			if( !IsConscious((*ix)->GetLifeState()) && OnEngagedLostConsciousnessAbortAttack() )
			{
				continue;
			}

			// ignore fleeing actors, if we must
			if( OnEngagedFleedAbortAttack() &&
				(*ix)->GetMind()->GetFrontJob( JQ_ACTION ) &&
				( (*ix)->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_FLEE_FROM_OBJECT ) )
			{
				continue;
			}
		}

		m_TempGopColl2.push_back( *ix );
	}

	////////////////////////////////////////
	//	check for observer moved
	bool observerMoved;
	if( m_MyPositionLastVisibilityCheck == GetGo()->GetPlacement()->GetPosition() )
	{
		observerMoved = false;
	}
	else
	{
		if( !gSiegeEngine.IsNodeInAnyFrustum( m_MyPositionLastVisibilityCheck.node ) )
		{
			m_MyPositionLastVisibilityCheck = GetGo()->GetPlacement()->GetPosition();
			observerMoved = true;
		}
		else
		{
			observerMoved = gAIQuery.GetDistance( m_MyPositionLastVisibilityCheck, GetGo()->GetPlacement()->GetPosition() ) > gWorldOptions.GetVisibilityCacheDirtyDistance();
		}
	}
	if( observerMoved )
	{
		m_MyPositionLastVisibilityCheck = GetGo()->GetPlacement()->GetPosition();
	}

	////////////////////////////////////////
	//	clear visit bit
	for( DistanceVisibleMap::iterator ivclear = m_DistanceVisibleMap.begin(); ivclear != m_DistanceVisibleMap.end(); ++ivclear )
	{
		(*ivclear).second->m_Visited = false;

		GoHandle visible( (*ivclear).second->m_Object );
		if( visible )
		{
			if( !visible->IsInAnyWorldFrustum() )
			{
				(*ivclear).second->m_MarkedForDeletion = true;
			}
		}
		else
		{
			(*ivclear).second->m_MarkedForDeletion = true;
		}
	}

	////////////////////////////////////////
	//	calc visibility
	for( GopColl::iterator it = m_TempGopColl2.begin(); it != m_TempGopColl2.end(); ++it )
	{
		bool addEntry = false;

		GoidVisibleMap::iterator ig = m_GoidVisibleMap.find( (*it)->GetGoid() );

		if( ig != m_GoidVisibleMap.end() )
		{
			////////////////////////////////////////
			//	process existing record
			if( (*ig).second->m_MarkedForDeletion )
			{
				continue;
			}

			if( !CanOperateOn( *it, false ) )
			{
				(*ig).second->m_MarkedForDeletion = true;
				continue;
			}

			(*ig).second->m_Visited = true;

			if( observerMoved || ( (*it)->HasMind() && (*it)->GetMind()->IsRidingElevator() ) || !(*it)->GetAspect()->GetIsVisible() )
			{
				addEntry = true;
			}
			else if( gAIQuery.GetDistance( (*ig).second->m_LastPositionVisible, (*it)->GetPlacement()->GetPosition() ) > gWorldOptions.GetVisibilityCacheDirtyDistance() )
			{
				addEntry = true;
			}
			else if( (*ig).second->m_BlocksPath != (*it)->GetAspect()->GetDoesBlockPath() )
			{
				m_RequestFlushSensors = true;
				return;
			}

			if( addEntry )
			{
				gpassert( (*ig).second->m_Object == (*it)->GetGoid() );
				// kill existing entry...
				(*ig).second->m_MarkedForDeletion = true;
			}
		}
		else
		{
			addEntry = true;
		}

		////////////////////////////////////////
		//	add new record

		if( addEntry )
		{
			gpassert( m_GoidVisibleMap.size() == m_DistanceVisibleMap.size() );

			bool visible = false;

			if( (*it)->HasMind() && ((*it)->GetMind()->GetSightRange() >= GetSightRange() ) )
			{
				if( !(*it)->GetMind()->IsInVisibleCache( GetGo(), visible ) )
				{
					visible = (*it)->GetAspect()->GetIsVisible() && IsLosClear( *it );
				}
			}
			else
			{
				visible = (*it)->GetAspect()->GetIsVisible() && IsLosClear( *it );
			}

			// add new entry to Goid map
			VisibilityInfo * vis = new VisibilityInfo( (*it)->GetGoid(), (*it)->GetPlacement()->GetPosition(), visible );
			vis->m_Visited = true;
			vis->m_BlocksPath = (*it)->GetAspect()->GetDoesBlockPath();
			m_GoidVisibleMap.insert( make_pair( (*it)->GetGoid(), vis ) );

			// add entry to distance map
			float dist = gAIQuery.GetDistance( GetGo(), *it );
			m_DistanceVisibleMap.insert( make_pair( dist, vis ) );

			gpassert( m_GoidVisibleMap.size() == m_DistanceVisibleMap.size() );
		}
	}

	////////////////////////////////////////
	//	all visible objects which have not been visited are no longer in our sight range
	// 	delete all non-visited and marked for deletion entries
	for( ivclear = m_DistanceVisibleMap.begin(); ivclear != m_DistanceVisibleMap.end(); )
	{
		gpassert( m_GoidVisibleMap.size() == m_DistanceVisibleMap.size() );

		if( !(*ivclear).second->m_Visited || (*ivclear).second->m_MarkedForDeletion )
		{
			VisibilityInfo * vis = (*ivclear).second;

			GoidVisibleMap::iterator igclear = m_GoidVisibleMap.find( vis->m_Object );
			gpassert( igclear != m_GoidVisibleMap.end() );
			m_GoidVisibleMap.erase( igclear );

			ivclear = m_DistanceVisibleMap.erase( ivclear );

			Delete( vis );
		}
		else
		{
			++ivclear;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	bin-process visible objects into enemies, friends, and all that BS.

	GoidColl enemies, friends, innerComfort, outerComfort;

	m_ItemsVisible.clear();

	for( GoidVisibleMap::iterator ig = m_GoidVisibleMap.begin(); ig != m_GoidVisibleMap.end(); ++ig )
	{
		if( (*ig).second->m_MarkedForDeletion || !(*ig).second->m_Visible )
		{
			continue;
		}

		GoHandle go( (*ig).second->m_Object );
		if( !go )
		{
			continue;
		}

		if( go->IsActor() )
		{
			////////////////////////////////////////
			//	friends/enemies

			if( IsEnemy( go ) )
			{
				enemies.push_back( go->GetGoid() );
				if( IsAlive( go->GetLifeState() ) )
				{
					m_TimeEnemySpottedLast = gWorldTime.GetTime();
				}
			}
			else if ( IsFriend( go ) )
			{
				friends.push_back( go->GetGoid() );
			}

			////////////////////////////////////////
			//	comfort

			if( IsInRange( go, m_InnerComfortZoneRange ) )
			{
				innerComfort.push_back( go->GetGoid() );
			}
			else if( IsInRange( go, m_OuterComfortZoneRange ) )
			{
				outerComfort.push_back( go->GetGoid() );
			}

		}
		else if( go->IsItem() )
		{
			m_ItemsVisible.push_back( go->GetGoid() );
		}
	}

	GoidColl newEnemies;
	gAIQuery.FindBNotInA( m_EnemiesVisible, enemies, newEnemies );
	m_EnemiesVisible = enemies;

	GoidColl newFriends;
	gAIQuery.FindBNotInA( m_FriendsVisible, friends, newFriends );
	m_FriendsVisible = friends;

	GoidColl newInnerComfort;
	gAIQuery.FindBNotInA( m_ActorsInInnerComfortZone, innerComfort, newInnerComfort );
	m_ActorsInInnerComfortZone = innerComfort;

	GoidColl newOuterComfort;
	gAIQuery.FindBNotInA( m_ActorsInOuterComfortZone, outerComfort, newOuterComfort );
	m_ActorsInOuterComfortZone = outerComfort;

	////////////////////////////////////////////////////////////////////////////////
	//	EARLY OUT IF WE'RE A CLIENT - so we don't send any worldmessages

	if( gServer.IsRemote() )
	{
		return;
	}

	//////////////////////////////////////////
	// process new enemies seen

	GoidColl::iterator im;

	bool friendsAlerted = false;

	for( im = newEnemies.begin(); im != newEnemies.end(); ++im )
	{
		DWORD ac = Job::GetInstanceCount();

		GoHandle enemy( *im );
		if( enemy && IsAlive( enemy->GetLifeState() ) )
		{
			WorldMessage message( WE_ENEMY_SPOTTED, GOID_INVALID, GetGoid() );
			message.SetData1( MakeInt( *im ) );
			Send( message );

			if( OnEnemySpottedAlertFriends() && !friendsAlerted )
			{
				GoidColl::iterator ifriend;
				for( ifriend = m_FriendsVisible.begin(); ifriend != m_FriendsVisible.end(); ++ifriend )
				{
					GoHandle frnd( *ifriend );
					if( frnd && frnd->IsInAnyWorldFrustum() && frnd->GetCommon()->GetMembership().ContainsAny( m_ComChannels ) )
					{
						if( IsInRange( frnd, m_ComRange ) )
						{
							Send( WorldMessage( WE_ALERT_ENEMY_SPOTTED, GetGoid(), (*ifriend), MakeInt( *im ) ) );
						}
					}
				}
				friendsAlerted = true;
			}
		}

		// $$$ optimization: check for logic taking action on message, if so, don't send more messages

#		if !GP_RETAIL
		if( GetGo()->IsHuded() )
		{
			GoHandle enemy( (*im) );
			if( enemy && enemy->IsInAnyWorldFrustum() )
			{
				gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), enemy->GetPlacement()->GetPosition(), 0xffff0000, "enemy spotted" );
			}
		}
#		endif // !GP_RETAIL

		if( ac != Job::GetInstanceCount() )
		{
			// took action on message
			break;
		}
	}

	//////////////////////////////////////////
	// process new friends seen

	for( im = newFriends.begin(); im != newFriends.end(); ++im )
	{
		DWORD ac = Job::GetInstanceCount();

		Send( WorldMessage( WE_FRIEND_SPOTTED, GOID_INVALID, GetGoid(), MakeInt( *im ) ) );

#		if !GP_RETAIL
		if( GetGo()->IsHuded() )
		{
			GoHandle frnd( (*im) );
			if( frnd && frnd->IsInAnyWorldFrustum() )
			{
				gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), frnd->GetPlacement()->GetPosition(), 0xff00ff00, "friend spotted" );
			}
		}
#		endif // !GP_RETAIL

		if( ac != Job::GetInstanceCount() )
		{
			// took action on message
			break;
		}
	}

	//////////////////////////////////////////
	// process comfort zones

	GoidColl::iterator ic;

	for( ic = newOuterComfort.begin(); ic != newOuterComfort.end(); ++ic )
	{
		Go * go = ::GetGo( *ic );

#		if !GP_RETAIL
		if( GetGo()->IsHuded() )
		{
			gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), go->GetPlacement()->GetPosition(), COLOR_LIGHT_ORANGE, "entered outer comfort zone" );
		}
#		endif // !GP_RETAIL

		if( IsEnemy( go ) && IsAlive( go->GetLifeState() ) )
		{
			Send( WorldMessage( WE_ENEMY_ENTERED_OUTER_COMFORT_ZONE, GOID_INVALID, GetGoid(), MakeInt( *ic ) ) );
		}
		else if( IsFriend( go ) && IsAlive( go->GetLifeState() ) )
		{
			Send( WorldMessage( WE_FRIEND_ENTERED_OUTER_COMFORT_ZONE, GOID_INVALID, GetGoid(), MakeInt( *ic ) ) );
		}
	}

	for( ic = newInnerComfort.begin(); ic != newInnerComfort.end(); ++ic )
	{
		Go * go = ::GetGo( *ic );

#		if !GP_RETAIL
		if( GetGo()->IsHuded() )
		{
			gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), go->GetPlacement()->GetPosition(), COLOR_ORANGE, "entered inner comfort zone" );
		}
#		endif // !GP_RETAIL

		if( IsEnemy( go ) && IsAlive( go->GetLifeState() ) )
		{
			Send( WorldMessage( WE_ENEMY_ENTERED_INNER_COMFORT_ZONE, GOID_INVALID, GetGoid(), MakeInt( *ic ) ) );
		}
		else if( IsFriend( go ) && IsAlive( go->GetLifeState() ) )
		{
			Send( WorldMessage( WE_FRIEND_ENTERED_INNER_COMFORT_ZONE, GOID_INVALID, GetGoid(), MakeInt( *ic ) ) );
		}
	}
}


void GoMind :: FlushCaches()
{
	if( !m_DistanceVisibleMap.empty() )
	{
		for( DistanceVisibleMap::iterator ivclear = m_DistanceVisibleMap.begin(); ivclear != m_DistanceVisibleMap.end(); ++ivclear )
		{
			gpassert( m_GoidVisibleMap.size() == m_DistanceVisibleMap.size() );
			Delete( (*ivclear).second );
		}
		m_DistanceVisibleMap.clear();
		m_GoidVisibleMap.clear();
	}

	if( !m_ActionResultColl.empty() )
	{
		for( JobResultColl::iterator ir = m_ActionResultColl.begin(); ir != m_ActionResultColl.end(); ++ir )
		{
			delete *ir;
		}
		m_ActionResultColl.clear();
	}
}


void GoMind :: UpdateJobResults()
{
	double cutoffTime = PreciseSubtract(gWorldTime.GetTime(), double( m_ActionResultMemoryLength ));

	for( JobResultColl::iterator i = m_ActionResultColl.begin(); i != m_ActionResultColl.end(); )
	{
		bool iadvance = true;

		if( (*i)->m_TimeFinished <= cutoffTime )
		{
			delete (*i);
			i = m_ActionResultColl.erase( i );
			iadvance = false;
		}
		else if( (*i)->GetTraits() & JT_REQUIRE_GOAL )
		{
			GoHandle goal( (*i)->m_GoalObject );
			if( !goal )
			{
				delete (*i);
				i = m_ActionResultColl.erase( i );
				iadvance = false;
			}
		}

		if( iadvance )
		{
			++i;
		}
	}
}


void GoMind :: RegisterActionResult( Job * job )
{
	JobResult * result = FindActionResult( job );
	if( result )
	{
		DWORD oldCount = result->m_Count;
		JobResult newResult( job );
		*result = newResult;
		result->m_Count = oldCount + 1;
	}
	else
	{
		JobResult * newResult = new JobResult( job );
		m_ActionResultColl.push_back( newResult );
	}
}


JobResult * GoMind :: FindActionResult( JobReq const & req )
{
	for( JobResultColl::iterator i = m_ActionResultColl.begin(); i != m_ActionResultColl.end(); ++i )
	{
		bool match	= (*i)->m_Jat == req.m_Jat;

		if( match && ( req.m_GoalObject != GOID_INVALID ) )
		{
			match = req.m_GoalObject == (*i)->m_GoalObject;
		}

		if( match && ( req.m_GoalModifier != GOID_INVALID ) )
		{
			match = req.m_GoalModifier == (*i)->m_GoalModifier;
		}

		if( match )
		{
			return( (*i) );
		}
	}
	return NULL;
}


JobResult * GoMind :: FindActionResult( Job * job )
{
	for( JobResultColl::iterator i = m_ActionResultColl.begin(); i != m_ActionResultColl.end(); ++i )
	{
		bool match	= (*i)->m_Jat == job->GetJobAbstractType();

		if( match && ( job->GetGoalObject() != GOID_INVALID ) )
		{
			match = job->GetGoalObject() == (*i)->m_GoalObject;
		}

		if( match && ( job->GetGoalModifier() != GOID_INVALID ) )
		{
			match = job->GetGoalModifier() == (*i)->m_GoalModifier;
		}

		if( match )
		{
			return( (*i) );
		}
	}
	return NULL;
}


bool GoMind :: JobRecentlyFailedToPathTo( Go * target )
{
	for( JobResultColl::iterator i = m_ActionResultColl.begin(); i != m_ActionResultColl.end(); ++i )
	{
		GoHandle go( (*i)->m_GoalObject );
		if( go && ( go == target ) && ( (*i)->m_Result == JR_FAILED_NO_PATH ) )
		{
			return( true );
		}
	}
	return( false );
}


bool GoMind :: IsVisible( Goid object ) const
{
	GoidVisibleMap::const_iterator ig = m_GoidVisibleMap.find( object );
	if( ig == m_GoidVisibleMap.end() )
	{
		return( false );
	}
	else
	{
		GoHandle visible( object );
		if( !visible || !CanOperateOn( visible, false ) )
		{
			return false;
		}
		return( (*ig).second->m_Visible );
	}
}


bool GoMind :: IsInVisibleCache( Go * object, bool & visible )
{
	GoidVisibleMap::iterator ig = m_GoidVisibleMap.find( object->GetGoid() );

	if( ig == m_GoidVisibleMap.end() )
	{
		visible = false;
		return( false );
	}
	else
	{
		if( !gSiegeEngine.IsNodeInAnyFrustum( (*ig).second->m_LastPositionVisible.node ) || !CanOperateOn( object, false ) )
		{
			(*ig).second->m_MarkedForDeletion = true;
			return false;
		}

		if( gAIQuery.GetDistance( (*ig).second->m_LastPositionVisible, object->GetPlacement()->GetPosition() ) > gWorldOptions.GetVisibilityCacheDirtyDistance() )
		{
			visible	= IsLosClear( object );

			(*ig).second->m_LastPositionVisible	= object->GetPlacement()->GetPosition();
			(*ig).second->m_Visible				= visible;
		}
		else
		{
			visible = (*ig).second->m_Visible;
		}
		return( true );
	}
}


JobQueue const & GoMind :: GetQ( eJobQ Q ) const
{
//	return GetQ( Q );
	if( Q == JQ_BRAIN )
	{
		return( m_Brains );
	}
	else if( Q == JQ_ACTION )
	{
		return( m_Actions );
	}
	else
	{
		gpassertm( 0, "Invalid Q specified." );
		return( m_Actions );
	}
}


JobQueue & GoMind :: GetQ( eJobQ Q )
{
	if( Q == JQ_BRAIN )
	{
		return( m_Brains );
	}
	else if( Q == JQ_ACTION )
	{
		return( m_Actions );
	}
	else
	{
		gpassertm( 0, "Invalid Q specified." );
		return( m_Actions );
	}
}


void GoMind :: DrawActionQueue()
{
	if( gWorld.IsMultiPlayer() || !GetFrontJob( JQ_ACTION ) )
	{
		return;
	}

	bool drawWaypoint = TestOptions( OPTION_DRAW_HUMAN_WAYPOINTS ) && GetGo()->IsSelected();
#if	!GP_RETAIL
	drawWaypoint = drawWaypoint || GetGo()->IsHuded();
#endif

	if( drawWaypoint )
	{
		SiegePos lastPos = GetGo()->GetPlacement()->GetPosition();
		JobQueue::iterator i = m_Actions.begin();

		while( ( i != m_Actions.end() ) && !(*i)->IsMarkedForDeletion() )
		{
			gpassert( !IsMemoryBadFood( (DWORD)*i ) );
			SiegePos currentActionFocusPos;

			if( (*i)->GetFocusPosition( currentActionFocusPos ) )
			{
				DrawActionWaypoint( lastPos, currentActionFocusPos, (*i) );
				lastPos = currentActionFocusPos;
			}
			++i;
		}
	}
}


void GoMind :: DrawActionWaypoint( SiegePos const & from, SiegePos const & to, Job * action )
{
#if !GP_RETAIL
	gWorld.DrawDebugArc( from, to, COLOR_GREEN, GetJobWaypointName( action->GetJobAbstractType() ) );
#else
	UNREFERENCED_PARAMETER(action);
	gWorld.DrawDebugArc( from, to, COLOR_GREEN);
#endif
}


bool GoMind :: Create( Job * & pJob, eJobAbstractType AType )
{
	GPPROFILERSAMPLE( "GoMind :: Create", SP_AI | SP_AI_MIND );

	pJob = 0;

	pJob = new Job( this, AType );

	if( pJob )
	{
		pJob->SetJobAbstractType( AType );
		// set initial traits
		pJob->SetTraits( GetTraits( AType ) );
		gpassertm( pJob->GetTraits() != 0, "Every job must have properties set." );
	}

	return( pJob != NULL );
}


#if !GP_RETAIL
void UpdateDebugWatchHelper( JobQueue & q, gpstring const & name, bool verbose, ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Indent();

	ctx->Output( name );
	ctx->OutputEol();

	if( q.empty() )
	{
		return;
	}

	int j=0;

	for( JobQueue::iterator i = q.begin(); i<q.end(); ++i, ++j )
	{
		(*i)->Dump( verbose, ctx );
	}

	ctx->Outdent();
}


void GoMind :: Dump( bool verbose, ReportSys::Context * ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->SetIndentSpaces( 2 );

	ctx->Indent();
	ctx->Output( "[] Mind:\n" );

	ctx->Indent();

	ctx->OutputF( "tname=%s, pos=%s\n", GetGo()->GetTemplateName(), ::ToString( GetGo()->GetPlacement()->GetPosition()).c_str() );
	ctx->OutputF( "time to sensor scan = %1.3f, am busy = %d\n", float( m_NextSensorScanTime - gWorldTime.GetTime() ), AmBusy() );
	ctx->OutputF( "alignment= %s, MO_[ %s ] CO_[ %s ] FO_[ %s ]\n",
					GetGo()->HasActor() ? ::ToString( GetGo()->GetActor()->GetAlignment() ) : "N/A",
					::ToString( GetMovementOrders() ),
					::ToString( GetCombatOrders() ),
					::ToString( GetFocusOrders() ) );

	if( GetGo()->HasActor() )
	{
		ctx->OutputF(	"life=%1.0f/%1.0f mana=%1.0f/%1.0f lifestate=%s\n",
						GetGo()->GetAspect()->GetCurrentLife(),
						GetGo()->GetAspect()->GetMaxLife(),
						GetGo()->GetAspect()->GetCurrentMana(),
						GetGo()->GetAspect()->GetMaxMana(),
						::ToString( GetGo()->GetAspect()->GetLifeState() ) );

		ctx->OutputF( "m_GoidVisibleMap= %d, m_DistanceVisibleMap= %d, onelevator= %d\n", m_GoidVisibleMap.size(), m_DistanceVisibleMap.size(), IsRidingElevator() );

		ctx->OutputF( "offenseFactor = %2.2f, survivalFactor = %2.2f\n",
						gAIQuery.CalcOffenseFactor( GetGo() ),
						gAIQuery.CalcSurvivalFactor( GetGo() ) );

		ctx->OutputF( "sensors: enm %d, frd %d, itm %d, icz %d, ocz %d\n",
						m_EnemiesVisible.size(),
						m_FriendsVisible.size(),
						m_ItemsVisible.size(),
						m_ActorsInInnerComfortZone.size(),
						m_ActorsInOuterComfortZone.size() );

		////////////////////////////////////////
		//	display focus enemy
		Go * focus = GetBestFocusEnemy();
		bool focusInWeaponRange = false;
		if( focus )
		{
			Go * weapon = GetGo()->GetInventory()->GetSelectedWeapon();
			if( !weapon )
			{
				weapon = GetGo();
			}
			focusInWeaponRange = IsInRange( focus, weapon, RL_EFFECTIVE_ATTACK_RANGE );
		}
		ctx->OutputF( "best focus enemy= %s, in weapon range= %s\n", focus ? focus->GetTemplateName() : "none", focusInWeaponRange ? "YES" : "NO" );

		////////////////////////////////////////
		//	autoitems

		ctx->Output( "autoitems:\n" );
		ctx->Indent();

		GopColl items;
		GetGo()->GetMind()->GetAutoItems( items );
		for( GopColl::iterator i = items.begin(); i != items.end(); ++i )
		{
			ctx->OutputF( "%s - %s%s\n",	(*i)->GetTemplateName(),
										(*i)->IsSpell() && gAIQuery.Is( GetGo(), (*i), QT_CASTABLE ) ? "castable" : "not castable",
										(*i)->IsPContentInventory() ? ", pcontent" : "" );
		}

		ctx->Outdent();

		if( !m_ActionResultColl.empty() )
		{
			ctx->Output( "job result memory:\n" );

			ctx->Indent();
			for( JobResultColl::iterator ir = m_ActionResultColl.begin(); ir != m_ActionResultColl.end(); ++ir )
			{
				GoHandle go( (*ir)->m_GoalObject );
				ctx->OutputF( "%s, goal= %s, goid=0x%08x, result= %s\n", ::ToString( (*ir)->m_Jat ), go ? go->GetTemplateName() : "unknown", go ? go->GetGoid() : GOID_INVALID, ::ToString( (*ir)->m_Result ) );
			}
			ctx->Outdent();
		}
	}

	if( !m_EngagedMeMap.empty() )
	{
		ctx->Indent();
		ctx->Output( "engaged me:\n" );
		ctx->Indent();

		for( EngagedMeMap::iterator ie = m_EngagedMeMap.begin(); ie != m_EngagedMeMap.end(); ++ie )
		{
			GoHandle engaged( (*ie).first );
			if( engaged )
			{
				ctx->OutputF( "actor 0x%08x - ref = %d, %s, offenseFactor=%2.2f\n",
								(*ie).first,
								(*ie).second,
								IsEnemy( engaged.Get() ) ? "enemy" : "friend",
								gAIQuery.CalcOffenseFactor( engaged.Get() ) );
			}
		}
		ctx->Outdent();
		ctx->Outdent();
	}

	ctx->Outdent();
	ctx->Outdent();

	UpdateDebugWatchHelper( m_Brains, "\n=== BRAIN", verbose, ctx );
	UpdateDebugWatchHelper( m_Actions, "\n=== ACTIONS", verbose, ctx );
}


void GoMind :: Dump2( bool /*verbose*/, ReportSys::Context * ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->SetIndentSpaces( 2 );
	ctx->Indent();

	ctx->Output( "[] Mind2:\n" );

	ctx->OutputF( "m_Rank                                   = %d\n",  	m_Rank   								);
	ctx->OutputF( "m_MayProcessAI                           = %d\n",  	m_MayProcessAI							);
	ctx->OutputF( "m_MayAttack                              = %d\n",  	m_MayAttack								);
	ctx->OutputF( "m_MayBeAttacked                          = %d\n",  	m_MayBeAttacked							);

	ctx->Output( "\nauto behavior permissions:\n" );
	ctx->Output( "--------------------------\n" );

	ctx->OutputF( "m_ActorAutoDefendsOthers                 = %d\n",  	m_ActorAutoDefendsOthers               	);
	ctx->OutputF( "m_ActorAutoHealsOthersLife               = %d\n",  	m_ActorAutoHealsOthersLife             	);
	ctx->OutputF( "m_ActorAutoHealsOthersMana               = %d\n",  	m_ActorAutoHealsOthersMana             	);
	ctx->OutputF( "m_ActorAutoHealsSelfLife                 = %d\n",  	m_ActorAutoHealsSelfLife               	);
	ctx->OutputF( "m_ActorAutoHealsSelfMana                 = %d\n",  	m_ActorAutoHealsSelfMana               	);
	ctx->OutputF( "m_ActorAutoPicksUpItems                  = %d\n",  	m_ActorAutoPicksUpItems                	);
	ctx->OutputF( "m_ActorAutoXfersMana                     = %d\n",  	m_ActorAutoXfersMana                   	);
	ctx->OutputF( "m_ActorAutoReanimatesFriends             = %d\n",  	m_ActorAutoReanimatesFriends		   	);

	ctx->OutputF( "m_WeaponPreference                       = %s\n",    ::ToString( m_WeaponPreference)		   	);
	ctx->OutputF( "m_ActorAutoSwitchesToKarate              = %d\n",    m_ActorAutoSwitchesToKarate			   	);
	ctx->OutputF( "m_ActorAutoSwitchesToMelee               = %d\n",    m_ActorAutoSwitchesToMelee 			   	);
	ctx->OutputF( "m_ActorAutoSwitchesToRanged              = %d\n",    m_ActorAutoSwitchesToRanged			   	);
	ctx->OutputF( "m_ActorAutoSwitchesToMagic               = %d\n",    m_ActorAutoSwitchesToMagic 			   	);
	ctx->OutputF( "m_ActorAutoUsesStoredItems               = %d\n",    m_ActorAutoUsesStoredItems 			   	);

	ctx->OutputF( "m_ActorBalancedAttackPreference          = %1.2f\n", m_ActorBalancedAttackPreference		   	);

	ctx->Output( "\nreflex permissions, sensor parameters:\n" );
	ctx->Output( "--------------------------------------\n" );

	ctx->OutputF( "m_OnAlertProjectileNearMissedAttack      = %d    ",   m_OnAlertProjectileNearMissedAttack 	   	);	 ctx->OutputF( "m_ActorLifeRatioHighThreshold           = %2.2f\n", m_ActorLifeRatioHighThreshold			);
	ctx->OutputF( "m_OnAlertProjectileNeaMrissedFlee        = %d    ",   m_OnAlertProjectileNearMissedFlee 	   		);	 ctx->OutputF( "m_ActorLifeRatioLowThreshold            = %2.2f\n", m_ActorLifeRatioLowThreshold			);
	ctx->OutputF( "m_OnEnemyEnteredICZAttack                = %d    ",   m_OnEnemyEnteredICZAttack               	);	 ctx->OutputF( "m_ActorManaRatioHighThreshold           = %2.2f\n", m_ActorManaRatioHighThreshold			);
	ctx->OutputF( "m_OnEnemyEnteredICZFlee                  = %d    ",   m_OnEnemyEnteredICZFlee                 	);	 ctx->OutputF( "m_ActorManaRatioLowThreshold            = %2.2f\n", m_ActorManaRatioLowThreshold			);
	ctx->OutputF( "m_OnEnemyEnteredICZSwitchToMelee         = %d    ",   m_OnEnemyEnteredICZSwitchToMelee 	   		);	 ctx->OutputF( "m_ComRange                              = %2.2f\n", m_ComRange				 				);
	ctx->OutputF( "m_OnEnemyEnteredOCZAttack                = %d    ",   m_OnEnemyEnteredOCZAttack               	);	 ctx->OutputF( "m_InnerComfortZoneRange                 = %2.2f\n", m_InnerComfortZoneRange	 			);
	ctx->OutputF( "m_OnEnemyEnteredOCZFlee                  = %d    ",   m_OnEnemyEnteredOCZFlee                 	);	 ctx->OutputF( "m_JobTravelDistanceLimit                = %2.2f\n", m_JobTravelDistanceLimit 				);
	ctx->OutputF( "m_OnEnemyEnteredWeaponEngageRangeAttack  = %d    ",   m_OnEnemyEnteredWeaponEngageRangeAttack 	);	 ctx->OutputF( "m_LimitedMovementRange                  = %2.2f\n", m_LimitedMovementRange					);
	ctx->OutputF( "m_OnEnemySpottedAlertFriends             = %d    ",   m_OnEnemySpottedAlertFriends            	);	 ctx->OutputF( "m_MeleeEngageRange                      = %2.2f\n", m_MeleeEngageRange		 				);
	ctx->OutputF( "m_OnEnemySpottedAttack                   = %d    ",   m_OnEnemySpottedAttack                  	);	 ctx->OutputF( "m_OuterComfortZoneRange                 = %2.2f\n", m_OuterComfortZoneRange	 			);
	ctx->OutputF( "m_OnEnemySpottedFlee                     = %d    ",   m_OnEnemySpottedFlee                    	);	 ctx->OutputF( "m_PersonalSpaceRange                    = %2.2f\n", m_PersonalSpaceRange	 				);
	ctx->OutputF( "m_OnEngagedFledAbortAttack               = %d    ",   m_OnEngagedFledAbortAttack              	);	 ctx->OutputF( "m_RangedEngageRange                     = %2.2f\n", m_RangedEngageRange		 			);
	ctx->OutputF( "m_OnEngagedLostConsciousnessAbortAttack  = %d    ",   m_OnEngagedLostConsciousnessAbortAttack 	);	 ctx->OutputF( "m_SensorScanPeriod                      = %2.2f\n", m_SensorScanPeriod		 				);
	ctx->OutputF( "m_OnEngagedLostLoiter                    = %d    ",   m_OnEngagedLostLoiter                   	);	 ctx->OutputF( "m_SightFov                              = %2.2f\n", m_SightFov				 				);
	ctx->OutputF( "m_OnEngagedLostReturnToJobOrigin         = %d    ",   m_OnEngagedLostReturnToJobOrigin        	);	 ctx->OutputF( "m_SightRange                            = %2.2f\n", m_SightRange							);
	ctx->OutputF( "m_OnFriendEnteredICZAttack               = %d    ",   m_OnFriendEnteredICZAttack              	);	 ctx->OutputF( "m_SightOriginHeight                     = %2.2f\n", m_SightOriginHeight					);
	ctx->OutputF( "m_OnFriendEnteredICZFlee                 = %d    ",   m_OnFriendEnteredICZFlee                	);	 ctx->OutputF( "m_VisibilityMemoryDuration              = %2.2f\n", m_VisibilityMemoryDuration				);
	ctx->OutputF( "m_OnFriendEnteredOCZAttack               = %d    ",   m_OnFriendEnteredOCZAttack              	);	 ctx->OutputF( "m_FleeCount                             = %d\n", 	m_FleeCount 							);
	ctx->OutputF( "m_OnFriendEnteredOCZFlee                 = %d  \n",   m_OnFriendEnteredOCZFlee                	);
	ctx->OutputF( "m_OnJobReachedTravelDistanceAbortAttack  = %d  \n",   m_OnJobReachedTravelDistanceAbortAttack 	);
	ctx->OutputF( "m_OnLifeRatioLowFlee                     = %d  \n",   m_OnLifeRatioLowFlee                    	);
	ctx->OutputF( "m_OnManaRatioLowFlee                     = %d  \n",   m_OnManaRatioLowFlee                    	);

	ctx->OutputF( "m_MinComfortDelayThreshold               = %1.1f  ",  m_MinComfortDelayThreshold					);	 ctx->OutputF( "time elapsed since enemy spotted         = %1.1f\n", GetTimeElapsedSinceLastEnemySpotted()	);
	ctx->OutputF( "m_MaxComfortDelayThreshold               = %1.1f  \n",  m_MaxComfortDelayThreshold				);
	ctx->OutputF( "comfort                                  = %1.1f  \n",  GetComfort()								);

	ctx->OutputF( "m_DebugWeEngagedLostConsciousnessRecevied= %d  \n",  m_DebugWeEngagedLostConsciousnessRecevied		);
	ctx->OutputF( "m_DebugWeKilledReceived                  = %d  \n",  m_DebugWeKilledReceived						);
	ctx->OutputF( "m_DebugWeELostConsciousnessReceived      = %d  \n",  m_DebugWeLostConsciousnessReceived			);


	ctx->Outdent();
}


#endif


void GoMind :: RSMove( SiegePos const & Pos, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_MOVE, JQ_ACTION, place, origin, Pos ) );
}


void GoMind :: RSGet( Go * item, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_GET, JQ_ACTION, place, origin, item->GetGoid() ) );
}


void GoMind :: RSGive( Go * target, Go * item, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_GIVE, JQ_ACTION, place, origin, target->GetGoid(), item->GetGoid() ) );
}


void GoMind :: RSGuard( Go * target, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_GUARD, JQ_ACTION, place, origin, target->GetGoid() ) );
}


void GoMind :: RSStop( eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, origin ) );
}


void GoMind :: RSUse( Go * item, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_USE, JQ_ACTION, place, origin, item->GetGoid(), GetGo()->GetGoid() ) );
}


void GoMind :: RSDrop( Go * item, SiegePos Pos, eQPlace place, eActionOrigin origin )
{
	RSDoJob( MakeJobReq( JAT_DROP, JQ_ACTION, place, origin, item->GetGoid(), Pos ) );
}

void GoMind :: RSDropGold( int gold, SiegePos Pos, eQPlace place, eActionOrigin origin )
{
	// $$$ move into job_drop.skrit

	FUBI_RPC_THIS_CALL( RSDropGold, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	GoHandle hMember( GetGo()->GetGoid());
	if ( hMember.IsValid() )
	{
		if ( gold <= hMember->GetInventory()->GetGold() && hMember->GetInventory()->GetGold() != 0 )
		{
			GoCloneReq cloneReq( gContentDb.GetDefaultGoldTemplate(), hMember->GetPlayerId() );
			cloneReq.SetStartingPos( hMember->GetPlacement()->GetPosition() );
			cloneReq.SetPrepareToDrawNow();
			Goid goldGoid = gGoDb.SCloneGo( cloneReq );
			GoHandle hItem( goldGoid );

			unsigned int goldTotal =  hMember->GetInventory()->GetGold() - gold;
			hMember->GetInventory()->SSetGold(goldTotal);
			hItem->GetAspect()->SSetGoldValue( gold, false );
			hItem->GetGold()->SetDroppedBy( GetGoid() );

			SDoJob(
				MakeJobReq( JAT_DROP, JQ_ACTION, place, origin, hItem->GetGoid(),
				Pos ));

			hItem->RCPlayVoiceSound( hMember->GetPlayer()->GetMachineId(), "put_down" );
		}
		else
		{
			//$$$ Security. Cant drop more gold than you have

		}
	}
}


bool GoMind :: MayMove()
{
	return ( GetMovementOrders() != MO_HOLD );
}


void GoMind :: RSDrinkLifeHealingPotion( eActionOrigin jo )
{
	FUBI_RPC_THIS_CALL( RSDrinkLifeHealingPotion, RPC_TO_SERVER );

	// $$$ security

	SDrinkLifeOrManaPotion( jo, true );
}


void GoMind :: RSDrinkManaHealingPotion( eActionOrigin jo )
{
	FUBI_RPC_THIS_CALL( RSDrinkManaHealingPotion, RPC_TO_SERVER );

	// $$$ security

	SDrinkLifeOrManaPotion( jo, false );
}


void GoMind :: SDrinkLifeOrManaPotion( eActionOrigin jo, bool bLifePotion )
{
	CHECK_SERVER_ONLY;

	if ( !UnderstandsJob( JAT_DRINK ) )
	{
		return;
	}

	Job * pJob = GetFrontJob( JQ_ACTION );
	if( pJob != NULL )
	{
		if ( ( pJob->GetTraits() & JT_HUMAN_INTERRUPTABLE ) == 0 )
		{
			return;
		}
	}

	if( (bLifePotion && GetGo()->GetAspect()->GetLifeRatio() != 1.0) ||
		(!bLifePotion && GetGo()->GetAspect()->GetManaRatio() != 1.0) )
	{
		// get inventory
		GopColl InventoryColl;

		if( GetGo()->GetInventory()->ListItems( QT_POTION, IL_ALL, InventoryColl ) )
		{
			GopColl ResultColl;
			if( gAIQuery.GetMaxN( GetGo(), bLifePotion ? QT_LIFE_HEALING : QT_MANA_HEALING, 1000, InventoryColl, ResultColl ) )
			{
				Go * pPotion = NULL;
				RetrieveNextPotion( &pPotion, ResultColl );
				gpassert( pPotion );

				// apply potion to self
				SDoJob( MakeJobReq( JAT_DRINK, JQ_ACTION, QP_FRONT, jo, pPotion->GetGoid() ) );

				Go * pOwner = 0;
				if ( pPotion->GetParent( pOwner ) && pOwner->HasActor() )
				{
					gWorld.UpdateInventoryGUICallback( pOwner->GetGoid() );
				}

				if ( GetGo()->HasAspect() &&
					  (bLifePotion && ::IsEqual( GetGo()->GetAspect()->GetCurrentLife(), GetGo()->GetAspect()->GetMaxLife() )) ||
					  (!bLifePotion && ::IsEqual( GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() ) ) )
				{
					return;
				}
				else
				{
					bool bMaxed = false;
					if ( bLifePotion )
					{
						bMaxed = ::IsEqual( GetGo()->GetAspect()->GetCurrentLife(), GetGo()->GetAspect()->GetMaxLife() );
					}
					else
					{
						bMaxed = ::IsEqual( GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() );
					}

					while ( RetrieveNextPotion( &pPotion, ResultColl ) && !bMaxed )
					{
						pPotion->GetMagic()->SApplyEnchantments( GetGo()->GetGoid(), GetGo()->GetGoid() );
						if ( pPotion->GetMagic()->IsPotion() && pPotion->GetMagic()->HasEnchantments() &&
							 ((pPotion->GetMagic()->GetPotionFullRatio() == 0.0f || pPotion->GetMagic()->GetPotionAmount() < 1.0f) ||
							  pPotion->GetMagic()->IsRejuvenationPotion()) )
						{
							if ( pPotion->GetParent( pOwner ) && pOwner->HasActor() )
							{
								pOwner->GetInventory()->RCRemoveExpendedItem( pPotion->GetGoid() );
							}

							gGoDb.SMarkForDeletion( pPotion->GetGoid() );
						}

						if ( bLifePotion )
						{
							bMaxed = ::IsEqual( GetGo()->GetAspect()->GetCurrentLife(), GetGo()->GetAspect()->GetMaxLife() );
						}
						else
						{
							bMaxed = ::IsEqual( GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() );
						}
					}
				}
			}
		}
	}
}

bool GoMind :: RetrieveNextPotion( Go ** pPotion, GopColl & potions )
{
	if ( potions.size() == 0 )
	{
		return false;
	}

	float smallestAmount = FLT_MAX;
	GopColl smallestPotions;

	float smallestRejuvAmount = FLT_MAX;
	GopColl smallestRejuvies;

	GopColl::iterator i;
	for ( i = potions.begin(); i != potions.end(); ++i )
	{
		if ( (*i)->GetMagic()->IsRejuvenationPotion() )
		{
			if ( (*i)->GetMagic()->GetPotionAmount( true ) <= smallestRejuvAmount )
			{
				if ( (*i)->GetMagic()->GetPotionAmount( true ) < smallestRejuvAmount )
				{
					smallestRejuvAmount = (*i)->GetMagic()->GetPotionAmount( true );
					smallestRejuvies.clear();
				}

				smallestRejuvies.push_back( *i );
			}
		}
		else
		{
			if ( (*i)->GetMagic()->GetPotionAmount( true ) <= smallestAmount )
			{
				if ( (*i)->GetMagic()->GetPotionAmount( true ) < smallestAmount )
				{
					smallestAmount = (*i)->GetMagic()->GetPotionAmount( true );
					smallestPotions.clear();
				}

				smallestPotions.push_back( *i );
			}
		}
	}

	smallestAmount = FLT_MAX;
	for ( i = smallestPotions.begin(); i != smallestPotions.end(); ++i )
	{
		if ( (*i)->GetMagic()->GetPotionAmount() < smallestAmount )
		{
			*pPotion = *i;
			smallestAmount = (*i)->GetMagic()->GetPotionAmount();
		}
	}

	if ( !(*pPotion) )
	{
		// All out, gotta use the rejuvies
		smallestRejuvAmount = FLT_MAX;
		for ( i = smallestRejuvies.begin(); i != smallestRejuvies.end(); ++i )
		{
			if ( (*i)->GetMagic()->GetPotionAmount() < smallestRejuvAmount )
			{
				*pPotion = *i;
				smallestRejuvAmount = (*i)->GetMagic()->GetPotionAmount();
			}
		}
	}

	for ( i = potions.begin(); i != potions.end(); ++i )
	{
		if ( (*i) == *pPotion )
		{
			potions.erase( i );
			return true;
		}
	}

	return false;
}

void GoMind :: RSCastLifeHealingSpell( eActionOrigin jo )
{
	FUBI_RPC_THIS_CALL( RSCastLifeHealingSpell, RPC_TO_SERVER );

	// $$$ security

	if( GetGo()->GetAspect()->GetLifeRatio() != 1.0 )
	{
		// get inventory
		GopColl InventoryColl;

		if( GetGo()->GetInventory()->ListItems( QT_SPELL, IL_ALL, InventoryColl ) )
		{
			GopColl SpellColl;

			if( gAIQuery.Get( GetGo(), QT_SPELL, InventoryColl, SpellColl ) )
			{
				GopColl ResultColl;

				if( gAIQuery.GetMax( GetGo(), QT_LIFE_HEALING, SpellColl, ResultColl ) )
				{
					Go * pSpell = ResultColl.front();
					gpassert( pSpell );
					gpassertm( 0, "Select approproate spell here" );
					//RSCastMagic( GetGo(), QP_CLEAR, JO_REFLEX );
					RSDoJob( MakeJobReq( JAT_CAST, JQ_ACTION, QP_FRONT, jo, GetGoid(), pSpell->GetGoid() ));
				}
			}
		}
	}
}


bool GoMind :: IsEnemy( const Go * pB ) const
{
	gpassert( pB );

	if( !pB->HasActor() )
	{
		return false;
	}

	if ( GetAllignedTeam() != -1 )
	{
		return ( GetAllignedTeam() == GetGo()->GetPlayer()->GetTeam() );
	}

	// in multiplayer - first check player alliances
	if( GetGo()->GetPlayer()->GetIsEnemy( pB->GetPlayer() ) )
	{
		return true;
	}

	if( !pB->HasActor() || (pB->GetActor()->GetAlignment() == AA_NEUTRAL) )
	{
		return false;
	}
	else
	{
		if( GetGo()->GetActor()->GetAlignment() == AA_NEUTRAL )
		{
			return false;
		}
		else
		{
			return( GetGo()->GetActor()->GetAlignment() != pB->GetActor()->GetAlignment() );
		}
	}
}


bool GoMind :: IsFriend( const Go * pB ) const
{
	if( !pB->HasActor() )
	{
		return false;
	}

	return( !IsEnemy( pB ) );
}


bool GoMind :: IsLosClear( Go * pB ) const
{
	if( pB == NULL )
	{
		gperror( "GoMind :: IsLosClear - called with NULL parameter.\n" );
		return false;
	}

	if( !pB->HasAspect() )
	{
		gperrorf(( "GoMind :: IsLosClear - called with an Aspect-less Go.  Go %s, scid = 0x%08x", pB->GetTemplateName(), pB->GetScid() ));
		return false;
	}

	if( !CanOperateOn( pB, false ) )
	{
		return false;
	}
/*
	gWorld.DrawDebugSphere( GetGo()->GetPlacement()->GetPosition(), 0.1f, COLOR_WHITE, "" );
	gWorld.DrawDebugSphere( pB->GetPlacement()->GetPosition(),		0.1f, COLOR_WHITE, "" );
	gWorld.DrawDebugLine( GetGo()->GetPlacement()->GetPosition(), pB->GetPlacement()->GetPosition(), COLOR_WHITE, "" );
*/
	if( GetGo() == pB )
	{
		gperror( "Asking for LOS with self?" );
		return true;
	}

	if( GetDistance( pB ) < 0.01 )
	{
		return true;
	}

	if( gAIQuery.IsLosClear( GetGo(), pB ) )
	{
		matrix_3x3	orient;
		vector_3	center;
		vector_3	halfDiag;

		vector_3 fromPos;
		vector_3 toPos;

		gAIQuery.GetLOSPoint( GetGo(), fromPos );
		gAIQuery.GetLOSPoint( pB, toPos );

		for( GoidColl::const_iterator i = m_PathBlockers.begin(); i != m_PathBlockers.end(); ++i )
		{
			if( (*i) == pB->GetGoid() )
			{
				continue;
			}

			GoHandle obstacle( (*i) );

			if( !obstacle )
			{
				continue;
			}

			if( !obstacle->HasAspect() )
			{
				gpwarningf(( "GoMind :: IsLosClear - ( Handled, but need to track ) - found a blocker without an Aspect!.  Go %s, scid = 0x%08x", obstacle->GetTemplateName(), obstacle->GetScid() ));
				continue;
			}

			if( !CanOperateOn( obstacle, true ) )
			{
				continue;
			}

			if( obstacle && obstacle->GetAspect()->GetDoesBlockPath() )
			{
				obstacle->GetAspect()->GetWorldSpaceOrientedBoundingVolume( orient, center, halfDiag );
				oriented_bounding_box_3 obb( center, halfDiag, orient );

				if( obb.SegmentIntersectsOrientedBox( fromPos, toPos ) )
				{
#					if !GP_RETAIL
					if( GetGo()->IsHuded() )
					{
						vector_3 halfDiagonal;
						obstacle->GetAspect()->GetHalfDiagonal( halfDiagonal );

						matrix_3x3 orientation;
						obstacle->GetPlacement()->GetOrientation().BuildMatrix( orientation );

 						gWorld.DrawDebugBox(	obstacle->GetPlacement()->GetPosition(),
												orientation,
												halfDiagonal,
												0xFFFF0000,
												0.1f );
					}
#					endif
					return false;
				}
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}


bool GoMind :: IsLosClear( SiegePos const & target ) const
{
/*
	gWorld.DrawDebugSphere( GetGo()->GetPlacement()->GetPosition(), 0.1f, COLOR_WHITE, "" );
	gWorld.DrawDebugSphere( pB->GetPlacement()->GetPosition(),		0.1f, COLOR_WHITE, "" );
	gWorld.DrawDebugLine( GetGo()->GetPlacement()->GetPosition(), pB->GetPlacement()->GetPosition(), COLOR_WHITE, "" );
*/
	return gAIQuery.IsLosClear( GetGo(), target );
}


bool GoMind :: IsInVisibilityCone( Go * /*pTarget*/ ) const
{
	gpassertm( 0, "not implemeted" );
	return false;
/*
	GO * pClient = GetGo();

	if( gAIQuery.IsInRange( pClient->GetPosition(), pTarget->GetPosition(), pClient->GetStats()->GetImmediateSightRange() ) )
	{
		return true;
	}

	SiegeNodeHandle hClientNode	= gSiegeEngine.NodeCache().UseObject( pClient->GetPosition().node );
	SiegeNode & ClientNode		= hClientNode.RequestObject( gSiegeEngine.NodeCache() );
	vector_3 ClientPos			= ClientNode.NodeToWorldSpace( pClient->GetPosition().pos );

	SiegeNodeHandle hTargetNode	= gSiegeEngine.NodeCache().UseObject( pTarget->GetPosition().node );
	SiegeNode & TargetNode		= hTargetNode.RequestObject( gSiegeEngine.NodeCache() );
	vector_3 TargetPos			= TargetNode.NodeToWorldSpace( pTarget->GetPosition().pos );

	return PointInSphereConeSection(	ClientPos,
										pClient->GetBody()->GetDirection(),	// $$$ eo
										pClient->GetStats()->GetSightAngle(),
										pClient->GetStats()->GetSightRange(),
										TargetPos );
*/

}


bool GoMind :: IsInRange( Go * pB, float range ) const
{
	gpassert( pB );
	return GetDistance( pB ) <= range;
}


bool GoMind :: IsIn2DRange( Go * pB, float range ) const
{
	gpassert( pB );
	return Get2DDistance( pB ) <= range;
}


float GoMind :: GetWeaponRange() const
{
	// $$ revisit - this makes assumption that a spell is a weapon, and looks only at equipped spell.  perhaps this should be
	//				explicit

	Go * weapon = NULL;
	Go * spell  = NULL;

	if( GetGo()->HasInventory() )
	{
		weapon	= GetGo()->GetInventory()->GetSelectedWeapon();
		spell	= GetGo()->GetInventory()->GetSelectedSpell();
	}

	if( weapon )
	{
		return weapon->GetAttack()->GetAttackRange();
	}
	else if( spell )
	{
		return spell->GetMagic()->GetCastRange();
	}
	else
	{
		return GetGo()->GetAttack()->GetAttackRange();
	}
}


bool GoMind :: IsInMeleeContactRange( Go * target, Go * weapon ) const
{
	Go * attacker = GetGo();
	gpassert( weapon->IsMeleeWeapon() || ( weapon == attacker ) );
	gpassert( target != GetGo() );

	float weaponRange = weapon->GetAttack()->GetAttackRange();

	if( target->HasMind() )
	{
		if( target->GetInventory()->GetSelectedMeleeWeapon() )
		{
			weaponRange = max( weaponRange, target->GetInventory()->GetSelectedMeleeWeapon()->GetAttack()->GetAttackRange() );
		}
		else
		{
			weaponRange = max( weaponRange, target->GetAttack()->GetAttackRange() );
		}
	}

	float	targ3Ddist = GetDistance( target );
	bool	in_range = !IsPositive( targ3Ddist-weaponRange, MCP::MINIMUM_DISTANCE_TOLERANCE );

	// Problem is that the range given to the MCP is assume to be 2D not 3D!

	if (!in_range)
	{
		float targ2Ddist = Get2DDistance( target );
		if (!IsPositive( targ2Ddist-weaponRange , 0.01f ))
		{
			// If we are within range in the 2D projection then
			// examine the vertical separation to see if it is
			// less than the weapon range

			vector_3 diff = gAIQuery.GetDifferenceVector( GetGo(), target );
			float vertical_separation = FABSF(diff.y)-m_PersonalSpaceRange;
			vertical_separation -= ( target->HasMind() ? target->GetMind()->GetPersonalSpaceRange() : target->GetAspect()->GetBoundingSphereRadius() );
			in_range =  !IsPositive( vertical_separation-weaponRange, MCP::MINIMUM_DISTANCE_TOLERANCE );
		}
	}

	return( in_range );
}


bool GoMind :: IsInSpellRange( Go * pTarget, Go * spell ) const
{
	gpassert( spell->IsSpell() );
	return( ( spell->GetMagic()->GetCastRange() ) > GetDistance( pTarget ) );
}


bool GoMind :: IsInRange( Go * target, Go * weapon, eRangeLimit limit ) const
{
	if( limit == RL_EFFECTIVE_ATTACK_RANGE )
	{
		return( IsInEffectiveAttackRange( target, weapon, false ) );
	}
	if( limit == RL_EFFECTIVE_ATTACK_RESPONSE_RANGE )
	{
		return( IsInEffectiveAttackRange( target, weapon, true ) );
	}
	else if( limit == RL_ENGAGE_RANGE )
	{
		return( IsInEngageRange( target, weapon ) );
	}
	else if( limit == RL_SIGHT_RANGE )
	{
		return( IsVisible( target->GetGoid() ) );
	}
	else
	{
		gpassert( 0 );
		return false;
	}
}


bool GoMind :: IsInEffectiveAttackRange( Go * target, Go * weapon, bool respondingToAttack ) const
{
	if( GetCombatOrders() == CO_HOLD )
	{
		return( false );
	}

	bool result = false;

	Go * attacker = GetGo();

	float weaponRange = 0.0f;

	if ( weapon->IsSpell() )
	{
		weaponRange = weapon->GetMagic()->GetCastRange();
	}
	else
	{
		weaponRange = weapon->GetAttack()->GetAttackRange();
	}

	////////////////////////////////////////////////////////////////////////////////
	//	special range checks if we're to hold position

	SiegePos attackerPos = attacker->GetPlacement()->GetPosition();

	if( attacker->IsAnyHumanPartyMember() )
	{
		if( attacker->GetParent()->GetChildren().size() > 1 )
		{
			attacker->GetMind()->GetLastExecutedUserAssignedActionPosition();
		}
	}

	if( attacker->GetMind()->GetMovementOrders() == MO_HOLD )
	{
		float rangeToTarget = weaponRange + attacker->GetMind()->GetPersonalSpaceRange();

		if( target->HasMind() )
		{
			rangeToTarget += target->GetMind()->GetPersonalSpaceRange();
		}
		else
		{
			rangeToTarget += target->GetAspect()->GetBoundingSphereRadius();
		}

		if( !gAIQuery.IsInRange( attacker->GetPlacement()->GetPosition(), target->GetPlacement()->GetPosition(), rangeToTarget ) )
		{
#			if !GP_RETAIL
			if( attacker->IsHuded() )
			{
				gpreportf( gGetAISkritContext, ( "'%s' - GoMind :: IsInEffectiveAttackRange() - false; MO_HOLD and target not within weapon range.\n", attacker->GetTemplateName() ));
			}
#			endif
			result = false;
		}
		else
		{
			result = true;
		}
	}
	else if ( attacker->GetMind()->GetMovementOrders() == MO_LIMITED || attacker->GetMind()->GetMovementOrders() == MO_FREE )
	{
		float limitedRange = attacker->GetMind()->GetLimitedMovementRange();

		if( respondingToAttack )
		{
			if( attacker->GetMind()->GetMovementOrders() == MO_FREE )
			{
				limitedRange = attacker->GetMind()->GetSightRange();
			}
		}
		else
		{
			if( attacker->IsAnyHumanPartyMember() && ( attacker->GetParent()->GetChildren().size() > 1 ) )
			{
				limitedRange = attacker->GetMind()->GetSightRange();
			}
		}

		if( weapon->IsRangedWeapon() || weapon->IsSpell() )
		{
			float rangeToTarget = weaponRange + attacker->GetMind()->GetPersonalSpaceRange() + limitedRange;

			if( target->HasMind() )
			{
				rangeToTarget += target->GetMind()->GetPersonalSpaceRange();
			}
			else
			{
				rangeToTarget += target->GetAspect()->GetBoundingSphereRadius();
			}

			if( !gAIQuery.IsInRange( attackerPos, target->GetPlacement()->GetPosition(), rangeToTarget ) )
			{
				result = false;
			}
			else
			{
				result = true;
			}
		}
		else
		{
			gpassert( weapon->IsMeleeWeapon() || weapon == GetGo() );

			float allowedRange = limitedRange + ( target->HasMind() ? target->GetMind()->GetPersonalSpaceRange() : target->GetAspect()->GetBoundingSphereRadius() );

			if( !gAIQuery.IsInRange( attackerPos, target->GetPlacement()->GetPosition(), allowedRange ) )
			{
#				if !GP_RETAIL
				if( attacker->IsHuded() )
				{
					gpreportf(  gGetAISkritContext, ( "'%s' - GoMind :: IsInEffectiveAttackRange() - false; MO_LIMITED and target not within limited range\n", attacker->GetTemplateName() ));
				}
#				endif
				result = false;
			}
			else
			{
				result = true;
			}
		}
	}

#	if !GP_RETAIL
	gpreportf(  gGetAISkritContext, ( "'%s' - GoMind :: IsInEffectiveAttackRange() - %s\n", attacker->GetTemplateName(), result ? "TRUE" : "FALSE" ));
#	endif

	return( result );
}


bool GoMind :: IsInMeleeEngageRange(  Go * target ) const
{
	return IsInRange( target, m_MeleeEngageRange );
}


bool GoMind :: IsInRangedEngageRange( Go * target ) const
{
	return IsInRange( target, GetRangedEngageRange() );
}


bool GoMind :: IsInEngageRange( Go * target, Go * weapon ) const
{
	bool result = false;
	Go * attacker = GetGo();

	////////////////////////////////////////////////////////////////////////////////
	//	is target alive && within engage range

	if( IsAlive( target->GetLifeState() ) )
	{
		if( weapon->IsRangedWeapon() || weapon->IsSpell() )
		{
			result = attacker->GetMind()->IsInRangedEngageRange( target );
		}
		else if( weapon->IsMeleeWeapon() || weapon->IsActor() )
		{
			result = attacker->GetMind()->IsInMeleeEngageRange( target );
		}
	}
	return( result );
}


bool GoMind :: HasPartyMembersInSphere( float const r ) const
{
	return GetPartyMembersInSphere( r );
}


float GoMind :: GetDistance( Go * pTo ) const
{
	gpassert( pTo );
	return( gAIQuery.GetDistance( GetGo(), pTo ) );
}


float GoMind :: GetDistanceAtPlanEnd( Go * pGo ) const
{
	SiegePos myPos		= SiegePos::INVALID;
	SiegePos targetPos  = SiegePos::INVALID;
	double endTime = 0;

	Plan * plan = gMCPManager.FindPlan( GetGoid(), false );
	if( plan )
	{
		myPos = plan->GetFinalDestination();
		endTime = plan->GetFinalDestinationTime();
	}
	else
	{
		myPos	  = GetGo()->GetPlacement()->GetPosition();
	}

	Plan * targetPlan = gMCPManager.FindPlan( pGo->GetGoid(), false );
	if( targetPlan )
	{
		if( plan )
		{
			targetPos = targetPlan->GetPosition( endTime, true, false );
		}
		else
		{
			targetPos = targetPlan->GetFinalDestination();
		}
	}
	else
	{
		targetPos = pGo->GetPlacement()->GetPosition();
	}

	if( !gSiegeEngine.IsNodeInAnyFrustum( myPos.node ) )
	{
		gpwarningf(("GoMind::GetDistanceAtPlanEnd() [OUT_OF_WORLD] source [%s] is no longer in the world\n",GetGo()->GetTemplateName()));
		return( FLOAT_MAX );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( targetPos.node ) )
	{
		gpwarningf(("GoMind::GetDistanceAtPlanEnd() [OUT_OF_WORLD] target [%s] is no longer in the world\n",pGo->GetTemplateName()));
		return( FLOAT_MAX );
	}

	if( !gSiegeEngine.IsNodeInAnyFrustum( myPos.node ) )
	{
		gpwarningf(("GoMind::GetDistanceAtPlanEnd() [OUT_OF_WORLD] source [%s] is no longer in the world\n",GetGo()->GetTemplateName()));
		return( FLOAT_MAX );
	}
	else if (!gSiegeEngine.IsNodeInAnyFrustum( targetPos.node ) )
	{
		gpwarningf(("GoMind::GetDistanceAtPlanEnd() [OUT_OF_WORLD] target [%s] is no longer in the world\n",pGo->GetTemplateName()));
		return( FLOAT_MAX );
	}

	float totalPersonalDistance	= GetPersonalSpaceRange() + ( pGo->HasMind() ? pGo->GetMind()->GetPersonalSpaceRange() : pGo->GetAspect()->GetBoundingSphereRadius() );
	float pointDistance			= gAIQuery.GetDistance( myPos, targetPos );

	return( max( 0.0f, pointDistance - totalPersonalDistance ) );
}


float GoMind :: Get2DDistance( Go * pTo ) const
{
	return ( gAIQuery.Get2DDistance( GetGo(), pTo ) );
}


bool GoMind :: GetPartyMembersInSphere( float const r, GopColl * output ) const
{
	return gAIQuery.GetOccupantsInSphere(	GetGo()->GetPlacement()->GetPosition(),
											r,
											GetGo(),
											NULL,
											output ? 0 : 1,
											OF_PARTY_MEMBERS | OF_ALIVE,
											output,
											!IsRidingElevator() ) > 0;
}


bool GoMind :: GetOccupantsInSphere( float const r, GopColl & output ) const
{
	return gAIQuery.GetOccupantsInSphere(	GetGo()->GetPlacement()->GetPosition(),
											r,
											GetGo(),
											NULL,
											0,
											OF_ACTORS | OF_ITEMS | OF_MATCH_ANY,
											&output,
											!IsRidingElevator() ) > 0;
}


bool GoMind :: GetEnemiesInSphere( float const r, GopColl * output ) const
{
	return gAIQuery.GetOccupantsInSphere(	GetGo()->GetPlacement()->GetPosition(),
											r,
											GetGo(),
											NULL,
											output ? 0 : 1,
											OF_ACTORS | OF_ENEMIES | OF_ALIVE,
											output,
											!IsRidingElevator() ) > 0;
}


bool GoMind :: HasEnemiesInSphere( float const r ) const
{
	return GetEnemiesInSphere( r );
}


bool GoMind :: GetFriendsInSphere( float const r, GopColl & output ) const
{
	return gAIQuery.GetOccupantsInSphere(	GetGo()->GetPlacement()->GetPosition(),
											r,
											GetGo(),
											NULL,
											0,
											OF_ACTORS | OF_FRIENDS | OF_ALIVE,
											&output,
											!IsRidingElevator() ) > 0;
}


bool GoMind :: HaveOccupantsInSphere( float const r ) const
{
	return gAIQuery.GetOccupantsInSphere(	GetGo()->GetPlacement()->GetPosition(),
											r,
											GetGo(),
											NULL,
											1,
											OF_ACTORS | OF_ITEMS | OF_MATCH_ANY,
											NULL,
											!IsRidingElevator() ) > 0;
}


bool GoMind :: AliveEnemiesVisible()
{
	if( m_AliveEnemiesVisibleDirty )
	{
		m_AliveEnemiesVisible = false;

		for( GoidColl::iterator i = m_EnemiesVisible.begin(); i != m_EnemiesVisible.end(); ++i )
		{
			GoHandle enemy( *i );
			if( enemy && IsAlive( enemy->GetLifeState() ) )
			{
				m_AliveEnemiesVisible = true;
				break;
			}
		}
		m_AliveEnemiesVisibleDirty = false;
	}
	return( m_AliveEnemiesVisible );
}


bool GoMind :: AliveFriendsVisible()
{
	if( m_AliveFriendsVisibleDirty )
	{
		m_AliveFriendsVisible = false;

		for( GoidColl::iterator i = m_FriendsVisible.begin(); i != m_FriendsVisible.end(); ++i )
		{
			GoHandle bud( *i );
			if( bud && IsAlive( bud->GetLifeState() ) )
			{
				m_AliveFriendsVisible = true;
				break;
			}
		}
		m_AliveFriendsVisibleDirty = false;
	}
	return( m_AliveFriendsVisible );
}


SiegePos & GoMind :: GetLastExecutedUserAssignedActionPosition()
{
	if( m_LastExecutedUserAssignedActionPosition == SiegePos::INVALID )
	{
		m_LastExecutedUserAssignedActionPosition = GetGo()->GetPlacement()->GetPosition();
	}
	return( m_LastExecutedUserAssignedActionPosition );
}


bool GoMind :: GetVisible( eQueryTrait qt, GopColl & out, float range ) const
{
	QtColl qtc;
	qtc.push_back( qt );
	return GetVisible( qtc, out, range );
}


bool GoMind :: GetVisible( QtColl & qtc, GopColl & out, float range ) const
{
	GPPROFILERSAMPLE( "GoMind :: GetVisible", SP_AI | SP_AI_MIND );

	DWORD sizeBefore = out.size();

	DistanceVisibleMap::const_iterator i;
	for( i = m_DistanceVisibleMap.begin(); i != m_DistanceVisibleMap.end(); ++i )
	{
		GoHandle visible( (*i).second->m_Object );
		if( visible )
		{
			if( (*i).first <= range )
			{
				if( visible->GetAspect()->GetIsVisible() && gAIQuery.Is( GetGo(), visible.Get(), qtc ) )
				{
					out.push_back( visible.Get() );
				}
			}
			else
			{
				break;
			}
		}
	}
	return( sizeBefore < out.size() );
}


Go * GoMind :: GetVisible( QtColl & qtc, float range ) const
{
	GopColl out;
	if( GetVisible( qtc, out, range ) )
	{
		return out.front();
	}
	else
	{
		return NULL;
	}
}


Go * GoMind :: GetVisible( eQueryTrait trait, float range ) const
{
	QtColl qtc;
	qtc.push_back( trait );
	return( GetVisible( qtc, range ) );
}


bool GoMind :: GetClosestVisible( eQueryTrait trait, GopColl & out ) const
{
	QtColl qtc;
	qtc.push_back( trait );
	return GetClosestVisible( qtc, out );
}


bool GoMind :: GetClosestVisible( QtColl & qtc, GopColl & out ) const
{
	GPPROFILERSAMPLE( "GoMind :: GetClosestVisible", SP_AI | SP_AI_MIND );

	DWORD sizeBefore = out.size();

	DistanceVisibleMap::const_iterator i;

	for( i = m_DistanceVisibleMap.begin(); i != m_DistanceVisibleMap.end(); ++i )
	{
		GoHandle visible( (*i).second->m_Object );
		if( visible )
		{
			if( gAIQuery.Is( GetGo(), visible.Get(), qtc ) )
			{
				out.push_back( visible.Get() );
			}
		}
	}

	return( sizeBefore < out.size() );
}


Go * GoMind :: GetClosestVisible( QtColl & qtc ) const
{
	GopColl out;
	if( GetClosestVisible( qtc, out ) )
	{
		return out.front();
	}
	else
	{
		return NULL;
	}
}


Go * GoMind :: GetClosestVisible( eQueryTrait trait ) const
{
	QtColl qtc;
	qtc.push_back( trait );
	return GetClosestVisible( qtc );
}


bool GoMind :: GetFarthestVisible( eQueryTrait trait, GopColl & out ) const
{
	QtColl qtc;
	qtc.push_back( trait );
	return GetFarthestVisible( qtc, out );
}


bool GoMind :: GetFarthestVisible( QtColl & qtc, GopColl & out ) const
{
	GPPROFILERSAMPLE( "GoMind :: GetFarthestVisible", SP_AI | SP_AI_MIND );

	DWORD sizeBefore = out.size();

	DistanceVisibleMap::const_reverse_iterator i;

	for( i = m_DistanceVisibleMap.rbegin(); i != m_DistanceVisibleMap.rend(); ++i )
	{
		GoHandle visible( (*i).second->m_Object );
		if( visible )
		{
			if( gAIQuery.Is( GetGo(), visible.Get(), qtc ) )
			{
				out.push_back( visible.Get() );
			}
		}
	}

	return( sizeBefore < out.size() );
}


Go * GoMind :: GetFarthestVisible( QtColl & qtc ) const
{
	GopColl out;
	if( GetFarthestVisible( qtc, out ) )
	{
		return out.front();
	}
	else
	{
		return NULL;
	}
}


Go * GoMind :: GetFarthestVisible( eQueryTrait trait ) const
{
	QtColl qtc;
	qtc.push_back( trait );
	return GetFarthestVisible( qtc );
}


Go * GoMind :: GetBestFocusEnemy() const
{
 	GPPROFILERSAMPLE( "GoMind :: GetBestFocusEnemy", SP_AI | SP_AI_MIND );

	Go * weapon = GetGo()->GetInventory()->GetSelectedWeapon();

	////////////////////////////////////////
	//	mark Gos which are not good targets for attack

	for( DistanceVisibleMap::const_iterator id = m_DistanceVisibleMap.begin(); id != m_DistanceVisibleMap.end(); ++id )
	{
		GoHandle go( (*id).second->m_Object );
		if( go )
		{
			go->SetVisit( (*id).second->m_Visible && !(*id).second->m_MarkedForDeletion );
		}
	}

	for( JobResultColl::const_iterator ir = m_ActionResultColl.begin(); ir != m_ActionResultColl.end(); ++ir )
	{
		if( (*ir)->m_Result == JR_FAILED_NO_PATH )
		{
			GoHandle go( (*ir)->m_GoalObject );
			if( go )
			{
				go->SetVisit( false );
			}
		}
	}

	////////////////////////////////////////
	//	get best focused enemy

	switch( m_FocusOrders )
	{
		case FO_CLOSEST:
		{
			QtColl qtc;
			qtc.push_back( QT_ENEMY );
			qtc.push_back( QT_ALIVE );
			qtc.push_back( QT_ATTACKABLE );

			for( DistanceVisibleMap::const_iterator id = m_DistanceVisibleMap.begin(); id != m_DistanceVisibleMap.end(); ++id )
			{
				GoHandle go( (*id).second->m_Object );
 				if( go && go->IsVisited() && gAIQuery.Is( GetGo(), go, qtc ) )
				{
					if( weapon && IsInEngageRange( go, weapon ) )
					{
						return( go );
					}
					else if( !weapon )
					{
						return( go );
					}
				}
			}
		}
		break;

		case FO_FARTHEST:
		{
			QtColl qtc;
			qtc.push_back( QT_ENEMY );
			qtc.push_back( QT_ALIVE );
			qtc.push_back( QT_ATTACKABLE );

			for( DistanceVisibleMap::const_reverse_iterator id = m_DistanceVisibleMap.rbegin(); id != m_DistanceVisibleMap.rend(); ++id )
			{
				GoHandle go( (*id).second->m_Object );
				if( go && go->IsVisited() && gAIQuery.Is( GetGo(), go, qtc ) )
				{
					if( weapon && !IsInEngageRange( go, weapon ) )
					{
						continue;
					}
					return( go );
				}
			}
		}
		break;

		case FO_WEAKEST:
		{
//			GoidColl temp;
			QtColl qtc;
			qtc.push_back( QT_ENEMY );
			qtc.push_back( QT_ALIVE );
			qtc.push_back( QT_ATTACKABLE );

			Go * weakest = NULL;
			float weakestValue = FLT_MAX;

			for( DistanceVisibleMap::const_iterator id = m_DistanceVisibleMap.begin(); id != m_DistanceVisibleMap.end(); ++id )
			{
				GoHandle go( (*id).second->m_Object );
				if( go && go->IsVisited() && gAIQuery.Is( GetGo(), go, qtc ) )
				{
					float value;
					if( gAIQuery.Is( GetGo(), go, QT_OFFENSE_FACTOR, value ) )
					{
						if( weapon && !IsInEngageRange( go, weapon ) )
						{
							continue;
						}
						if( value < weakestValue )
						{
							weakestValue = value;
							weakest = go;
						}
					}
				}
			}
			return weakest;
		}
		break;

		case FO_STRONGEST:
		{
			GoidColl temp;
			QtColl qtc;
			qtc.push_back( QT_ENEMY );
			qtc.push_back( QT_ALIVE );
			qtc.push_back( QT_ATTACKABLE );

			Go * strongest = NULL;
			float strongestValue = 0;

			for( DistanceVisibleMap::const_iterator id = m_DistanceVisibleMap.begin(); id != m_DistanceVisibleMap.end(); ++id )
			{
				GoHandle go( (*id).second->m_Object );
				if( go && go->IsVisited() && gAIQuery.Is( GetGo(), go, qtc ) )
				{
					float value;
					if( gAIQuery.Is( GetGo(), go, QT_OFFENSE_FACTOR, value ) )
					{
						if( weapon && !IsInEngageRange( go, weapon ) )
						{
							continue;
						}
						temp.push_back( go->GetGoid() );
						if( value > strongestValue )
						{
							strongestValue = value;
							strongest = go;
						}
					}
				}
			}
			return strongest;
		}
		break;

		default:
			gpassertm( 0, "unsupported focus order" );
			return NULL;
			break;
	};

	return NULL;
}


bool GoMind :: ItemsVisible() const
{
	return !m_ItemsVisible.empty();
}


// $$$ kill this

bool GoMind :: PickSafeSpotAwayFrom( Go * target, float distance, SiegePos & pos ) const
{
	return( gAIQuery.FindSpotRelativeToSource(	GetGo(),
												target,
												true,
												distance,
												distance,
												0,
												15,
												2,
												pos,
												false ) );
}


bool GoMind :: AmFacing( Go * target ) const
{
	return AmFacing( target->GetPlacement()->GetPosition() );
}


bool GoMind :: AmFacing( SiegePos const & target ) const
{
	vector_3 facingdir = gSiegeEngine.GetDifferenceVector( GetGo()->GetPlacement()->GetPosition(), target );

	if( facingdir.IsZero() )
	{
		return false;
	}

	facingdir.y = 0;

	vector_3 orientdir;
	GetGo()->GetPlacement()->GetOrientation().BuildMatrix().GetColumn2( orientdir );
	orientdir.y = 0;

	float angle = AngleBetween( facingdir, orientdir );
	/*
		testangle = 0.5f * 	acosf(GetGo()->GetBody()->GetOrient().w);
	*/

	return angle < ( 15.0f * DEGREES_TO_RADIANS );
}


bool GoMind :: IsLifeLow() const
{
	return GetGo()->GetAspect()->GetLifeRatio() < m_ActorLifeRatioLowThreshold;
}


bool GoMind :: IsLifeHigh() const
{
	return GetGo()->GetAspect()->GetLifeRatio() >= m_ActorLifeRatioHighThreshold;
}


bool GoMind :: IsManaLow() const
{
	return GetGo()->GetAspect()->GetManaRatio() < m_ActorManaRatioLowThreshold;
}


bool GoMind :: IsManaHigh() const
{
	return GetGo()->GetAspect()->GetManaRatio() >= m_ActorManaRatioHighThreshold;
}


void GoMind :: ProcessRequestHitEngaged()
{
	GPPROFILERSAMPLE( "GoMind :: ProcessRequestHitEngaged", SP_AI | SP_AI_MIND );

	GoHandle hEngaged( GetEngagedObject() );

	if( hEngaged && hEngaged->IsInAnyWorldFrustum() && !hEngaged->IsMarkedForDeletion() )
	{
		Goid weaponAttacker, weaponEngaged;

		Go * pWeaponAttacker = GetGo()->GetInventory()->GetSelectedWeapon();
		Go * pWeaponEngaged  = ( hEngaged->HasInventory() ) ? hEngaged->GetInventory()->GetSelectedWeapon() : NULL;

		pWeaponAttacker	= ( pWeaponAttacker ) ? pWeaponAttacker : GetGo();
		pWeaponEngaged	= ( pWeaponEngaged  ) ? pWeaponEngaged  : hEngaged;

		weaponAttacker	= pWeaponAttacker->GetGoid();
		weaponEngaged	= pWeaponEngaged->GetGoid();

		bool hit = false;

		// we hit

		if( ( pWeaponAttacker->IsMeleeWeapon() || pWeaponAttacker == GetGo() ) && IsInMeleeContactRange( hEngaged, pWeaponAttacker ) )
		{
			/*
			bool GoMind :: IsInWeaponRange( Go * pTarget, Go * pWeapon ) const
			{
				gpassert( pWeapon == GetGo() || pWeapon->IsWeapon() );
				float attrange = pWeapon->GetAttack()->GetAttackRange();
				float targdist = GetDistance( pTarget );
				return( !IsPositive( targdist-attrange , 0.01f ) );
			}

			*/

			if( gRules.CanHit( GetGoid(), GetEngagedObject(), weaponAttacker, weaponEngaged ) )
			{
				gRules.DamageGoMelee( GetGoid(), weaponAttacker, GetEngagedObject() );
				hit = true;

#				if !GP_RETAIL
				if( GetGo()->IsHuded() )
				{
					float attrange = pWeaponAttacker->GetAttack()->GetAttackRange();
					float defrange = pWeaponEngaged->HasAttack() ? pWeaponEngaged->GetAttack()->GetAttackRange() : 0;
					float targdist = GetDistance( hEngaged );

					gpwarningf((	"Received a message that [%s] hit [%s] with [%s]\n"
									"  attacker's weapon range is %6.4f\n"
									"  dist between is %6.4f\n"
									"  [defender's weapon range is %6.4f]\n",
									GetGo()->GetTemplateName(),
									hEngaged->GetTemplateName(),
									pWeaponAttacker->GetTemplateName(),
									attrange,
									targdist,
									defrange ));
				}
				if (gWorldOptions.GetHudMCP())
				{
					// Need to be able to toggle this off
					SiegePos p = GetGo()->GetPlacement()->GetPosition();
					p.pos.y += 2.0f*GetGo()->GetAspect()->GetBoundingSphereRadius();
					gWorld.DrawDebugWorldLabelScroll(p,"HIT",0xffff0000,3,true);
				}
#				endif // !GP_RETAIL
			}
			else
			{
				// message target
				Send( WorldMessage( WE_ATTACKED_MELEE, GOID_INVALID, GetEngagedObject(), MakeInt( GetGo()->GetGoid() ) ) );
				// message self
				Send( WorldMessage( WE_ENGAGED_MISSED, GetGoid(), GetGoid() ) );
				pWeaponAttacker->PlayMaterialSound( "attack_miss", hEngaged->GetBestArmor()->GetGoid() );
#				if !GP_RETAIL
				// Need to be able to toggle this off
				if (gWorldOptions.GetHudMCP())
				{
					SiegePos p = GetGo()->GetPlacement()->GetPosition();
					p.pos.y += 2.0f*GetGo()->GetAspect()->GetBoundingSphereRadius();
					gWorld.DrawDebugWorldLabelScroll(p,"MISS",0xff00ff00,3,true);
				}
#				endif // !GP_RETAIL
			}
		}
	}
	else
	{
		// $$$ why is this still getting hit?
//		gpassertm( 0, "Engaged actor invalid" );
	}

	ProcessCombatSensors();

	m_RequestProcessHitEngaged = false;
}


float GoMind :: GetSensorScanPeriod() const
{
	gpassertm( m_SensorScanPeriod > 0, "sensor_scan_period must be nonzero." );

	return m_SensorScanPeriod - ( m_SensorScanPeriod * Random( 0.05f ) ) + ( m_SensorScanPeriod * Random( 0.1f ) );
}


int GoMind :: GetEngagedMeAttackerCount() const
{
	int count = 0;

	for( EngagedMeMap::const_iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		GoHandle engaged( (*i).first );

		if( engaged
			&& CanOperateOn( engaged, false )
			&& ( engaged->HasMind() && engaged->GetMind()->GetFrontJob( JQ_ACTION ) && ( engaged->GetMind()->GetFrontJob( JQ_ACTION )->GetTraits() & JT_OFFENSIVE ) ) )
		{
			++count;
		}
	}
	return count;
}


int GoMind :: GetEngagedMeMeleeAttackerCount() const
{
	int count = 0;

	for( EngagedMeMap::const_iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		GoHandle engaged( (*i).first );

		if( engaged && CanOperateOn( engaged, false )
			&& ( engaged->GetInventory()->GetSelectedMeleeWeapon() || (!engaged->GetInventory()->GetSelectedWeapon() && !engaged->GetInventory()->GetSelectedSpell() ) ) )
		{
			++count;
		}
	}
	return count;
}


int GoMind :: GetEngagedMeRangedAttackerCount() const
{
	int count = 0;

	for( EngagedMeMap::const_iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		GoHandle engaged( (*i).first );
		if( engaged
			&& CanOperateOn( engaged, false )
			&& (engaged->GetInventory()->GetSelectedRangedWeapon() || engaged->GetInventory()->GetSelectedSpell() ) )
		{
			++count;
		}
	}
	return count;
}


float GoMind :: GetTimeElapsedSinceLastEnemySpotted() const
{
	return( float( PreciseSubtract(gWorldTime.GetTime(), m_TimeEnemySpottedLast) ) );
}


float GoMind :: GetRangedEngageRange() const
{
	if( GetGo()->IsAnyHumanPartyMember() )
	{
		Go * party = GetGo()->GetOwningParty();
		if( party && ( party->GetParty()->GetNumRangedFighters() > 1 ) )
		{
			return party->GetParty()->GetMaxEquppedRangedWeaponRange() + gSim.GetSniperRange();
		}
	}
	return m_RangedEngageRange;
}


float GoMind :: GetMeleeSeparationRange() const
{
	Go * weapon = NULL;

	if( GetGo()->HasInventory() )
	{
		weapon	= GetGo()->GetInventory()->GetSelectedWeapon();
	}

	if( weapon )
	{
		return weapon->GetAttack()->GetAttackRange();
	}
	else
	{
		return GetGo()->GetAttack()->GetAttackRange();
	}
}


void GoMind :: RSSetMovementOrders( eMovementOrders Order )
{
	FUBI_RPC_THIS_CALL( RSSetMovementOrders, RPC_TO_SERVER )

	CHECK_SERVER_ONLY;

	if( m_MovementOrders != Order )
	{
		GetGo()->GetPlayer()->SetPartyDirty();
		RCSetMovementOrders( Order );
		m_MovementOrders = Order;

		Job * job = GetFrontJob( JQ_ACTION );
		if( job && !( job->GetTraits() & JT_AUTO_INTERRUPTABLE ) )
		{
			SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
		}
	}
}


void GoMind :: RSSetCombatOrders( eCombatOrders Order )
{
	FUBI_RPC_THIS_CALL( RSSetCombatOrders, RPC_TO_SERVER )

	CHECK_SERVER_ONLY;

	if ( m_CombatOrders != Order )
	{
		GetGo()->GetPlayer()->SetPartyDirty();
		RCSetCombatOrders( Order );
		m_CombatOrders = Order;

		Job * job = GetFrontJob( JQ_ACTION );
		if( job && !( job->GetTraits() & JT_AUTO_INTERRUPTABLE ) )
		{
			SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
		}
	}
}


void GoMind :: RSSetFocusOrders( eFocusOrders Order )
{
	FUBI_RPC_THIS_CALL( RSSetFocusOrders, RPC_TO_SERVER )

	CHECK_SERVER_ONLY;

	if ( m_FocusOrders != Order )
	{
		GetGo()->GetPlayer()->SetPartyDirty();
		RCSetFocusOrders( Order );
		m_FocusOrders = Order;

		Job * job = GetFrontJob( JQ_ACTION );
		if( job && !( job->GetTraits() & JT_AUTO_INTERRUPTABLE ) )
		{
			SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
		}
	}
}


void GoMind :: RSSetDispositionOrders( eActorDisposition Order )
{
	FUBI_RPC_THIS_CALL( RSSetDispositionOrders, RPC_TO_SERVER )

	CHECK_SERVER_ONLY;

	if ( m_DispositionOrders != Order )
	{
		GetGo()->GetPlayer()->SetPartyDirty();
		RCSetDispositionOrders( Order );
		m_DispositionOrders = Order;

		Job * job = GetFrontJob( JQ_ACTION );
		if( job && !( job->GetTraits() & JT_AUTO_INTERRUPTABLE ) )
		{
			SDoJob( MakeJobReq( JAT_STOP, JQ_ACTION, QP_CLEAR, AO_REFLEX ) );
		}
	}
}


void GoMind :: RCSetMovementOrders( eMovementOrders Order )
{
	FUBI_RPC_THIS_CALL( RCSetMovementOrders, GetGo()->GetPlayer()->GetMachineId() )
	m_MovementOrders = Order;
	if ( GetGo()->IsScreenPartyMember() )
	{
		gWorld.PartyCallback();
	}
}


void GoMind :: RCSetCombatOrders( eCombatOrders Order )
{
	FUBI_RPC_THIS_CALL( RCSetCombatOrders, GetGo()->GetPlayer()->GetMachineId() )
	m_CombatOrders = Order;
	if ( GetGo()->IsScreenPartyMember() )
	{
		gWorld.PartyCallback();
	}
}


void GoMind :: RCSetFocusOrders( eFocusOrders Order )
{
	FUBI_RPC_THIS_CALL( RCSetFocusOrders, GetGo()->GetPlayer()->GetMachineId() )
	m_FocusOrders = Order;
	if ( GetGo()->IsScreenPartyMember() )
	{
		gWorld.PartyCallback();
	}
}


void GoMind :: RCSetDispositionOrders( eActorDisposition Order )
{
	FUBI_RPC_THIS_CALL( RCSetDispositionOrders, GetGo()->GetPlayer()->GetMachineId() )
	m_DispositionOrders = Order;
	if ( GetGo()->IsScreenPartyMember() )
	{
		gWorld.PartyCallback();
	}
}


float GoMind :: GetComfort()
{
	gpassert( m_MinComfortDelayThreshold >= 0 );
	gpassert( m_MaxComfortDelayThreshold >= 0 );

	float delay = GetTimeElapsedSinceLastEnemySpotted();
	clamp_min_max( delay, m_MinComfortDelayThreshold, m_MaxComfortDelayThreshold );

	return (delay - m_MinComfortDelayThreshold) / (m_MaxComfortDelayThreshold - m_MinComfortDelayThreshold);
}


bool GoMind :: GetEngagedMe( GopColl & out ) const
{
	unsigned int sizeBefore = out.size();

	for( EngagedMeMap::const_iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		GoHandle engaged( (*i).first );
		if( engaged && engaged->IsInAnyWorldFrustum() )
		{
			out.push_back( engaged.Get() );
		}
	}
	return out.size() > sizeBefore;
}


bool GoMind :: GetEngagedMeEnemies( GopColl & out ) const
{
	unsigned int sizeBefore = out.size();

	for( EngagedMeMap::const_iterator i = m_EngagedMeMap.begin(); i != m_EngagedMeMap.end(); ++i )
	{
		GoHandle engaged( (*i).first );
		if( engaged && CanOperateOn( engaged, false ) && IsEnemy( engaged ) )
		{
			out.push_back( engaged.Get() );
		}
	}
	return out.size() > sizeBefore;
}


bool GoMind :: GetAutoItems( GopColl & out ) const
{
	return( GetAutoItems( QT_ANY, out ) );
}


bool GoMind :: GetAutoItems( eQueryTrait qt, GopColl & out ) const
{
	QtColl qtc;
	qtc.push_back( qt );
	return( GetAutoItems( qtc, out ) );
}


bool GoMind :: GetAutoItems( QtColl & qtc, GopColl & out ) const
{
	unsigned int sizeBefore = out.size();

	eInventoryLocation il = GetGo()->GetInventory()->GetSelectedLocation();

	if( ActorAutoSwitchesToMelee() || ( il == IL_ACTIVE_MELEE_WEAPON ) )
	{
		GetGo()->GetInventory()->ListItems( qtc, IL_ACTIVE_MELEE_WEAPON, out );
	}

	if( ActorAutoSwitchesToRanged() || ( il == IL_ACTIVE_RANGED_WEAPON ) )
	{
		GetGo()->GetInventory()->ListItems( qtc, IL_ACTIVE_RANGED_WEAPON, out );
	}

	if( ActorAutoSwitchesToMagic() || ( il == IL_ACTIVE_PRIMARY_SPELL || il == IL_ACTIVE_SECONDARY_SPELL ) )
	{
		Go * spellBook = GetGo()->GetInventory()->GetEquipped( ES_SPELLBOOK );
		if( spellBook )
		{
			if( ActorAutoSwitchesToMagic() || ( il == IL_ACTIVE_PRIMARY_SPELL  ) )
			{
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_1, out );
			}
			if( ActorAutoSwitchesToMagic() || ( il == IL_ACTIVE_SECONDARY_SPELL ) )
			{
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_2, out );
			}
			if( ActorAutoUsesStoredItems() )
			{
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_3, out );
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_4, out );
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_5, out );
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_6, out );
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_7, out );
				spellBook->GetInventory()->ListItems( qtc, IL_SPELL_8, out );
			}
		}
	}

	if( ActorAutoUsesStoredItems() )
	{
		GetGo()->GetInventory()->ListItems( qtc, IL_MAIN, out );
	}

#if !GP_RETAIL
	out.AssertValid();
#endif
	return sizeBefore < out.size();
}


bool GoMind :: MayAutoUseMeleeWeapon()
{
	GopColl weapon;
	GetAutoItems( QT_MELEE_WEAPON, weapon );
	return !weapon.empty();
}


bool GoMind :: MayAutoUseRangedWeapon()
{
	GopColl weapon;
	GetAutoItems( QT_RANGED_WEAPON, weapon );
	return !weapon.empty();
}


bool GoMind :: MayAutoCastLifeDamagingSpell()
{
	QtColl qtc;
	qtc.push_back( QT_SPELL );
	qtc.push_back( QT_CASTABLE );
	qtc.push_back( QT_LIFE_DAMAGING );
	GopColl spell;
	GetAutoItems( qtc, spell );
	return !spell.empty();
}


bool GoMind :: MayAutoCastLifeHealingSpell()
{
	QtColl qtc;
	qtc.push_back( QT_SPELL );
	qtc.push_back( QT_CASTABLE );
	qtc.push_back( QT_LIFE_HEALING );
	GopColl spell;
	GetAutoItems( qtc, spell );
	return !spell.empty();
}


void GoMind :: ResetSensors()
{
	m_ActorsInInnerComfortZone.clear();
	m_ActorsInOuterComfortZone.clear();
	m_EnemiesVisible.clear();
	m_FriendsVisible.clear();
	m_ItemsVisible.clear();

	m_NextSensorScanTime = gWorldTime.GetTime();
}


eJobAbstractType GetBaseJat( eJobAbstractType in )
{
	switch( in )
	{
		case JAT_ATTACK_OBJECT_MELEE:
		case JAT_ATTACK_OBJECT_RANGED:
		{
			return( JAT_ATTACK_OBJECT );
		}
		case JAT_ATTACK_POSITION_MELEE:
		case JAT_ATTACK_POSITION_RANGED:
		{
			return( JAT_ATTACK_POSITION );
		}
		case JAT_ATTACK_OBJECT:
		case JAT_ATTACK_POSITION:
		case JAT_BRAIN:
		case JAT_CAST:
		case JAT_CAST_POSITION:
		case JAT_DIE:
		case JAT_DO_SE_COMMAND:
		case JAT_DRINK:
		case JAT_DROP:
		case JAT_EQUIP:
		case JAT_FACE:
		case JAT_FIDGET:
		case JAT_FLEE_FROM_OBJECT:
		case JAT_FOLLOW:
		case JAT_GAIN_CONSCIOUSNESS:
		case JAT_GET:
		case JAT_GIVE:
		case JAT_GUARD:
		case JAT_LISTEN:
		case JAT_MOVE:
		case JAT_PATROL:
		case JAT_PLAY_ANIM:
		case JAT_STARTUP:
		case JAT_STOP:
		case JAT_TALK:
		case JAT_UNCONSCIOUS:
		case JAT_USE:
		{
			return( in );
		}
		default:
		{
			gpassertm( 0, "Unsupported JAT" );
			return( in );
		}
	}
}


bool IsAttackJat( eJobAbstractType jat )
{
	switch( jat )
	{
		case JAT_ATTACK_OBJECT:
		case JAT_ATTACK_OBJECT_MELEE:
		case JAT_ATTACK_OBJECT_RANGED:
		case JAT_ATTACK_POSITION:
		case JAT_ATTACK_POSITION_MELEE:
		case JAT_ATTACK_POSITION_RANGED:
		{
			return( true );
		}
		default:
		{
			return( false );
		}
	}
}


bool IsLeafAttackJat( eJobAbstractType jat )
{
	switch( jat )
	{
		case JAT_ATTACK_OBJECT_MELEE:
		case JAT_ATTACK_OBJECT_RANGED:
		case JAT_ATTACK_POSITION_MELEE:
		case JAT_ATTACK_POSITION_RANGED:
		{
			return( true );
		}
		default:
		{
			return( false );
		}
	}
}


bool IsCastJat( eJobAbstractType jat )
{
	switch( jat )
	{
		case JAT_CAST:
		case JAT_CAST_POSITION:
		{
			return( true );
		}
		default:
		{
			return( false );
		}
	}
}

