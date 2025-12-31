//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDefs.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoDefs.h"

#include "ContentDb.h"
#include "FuBiPersistImpl.h"
#include "NetLog.h"
#include "Go.h"
#include "GoActor.h"
#include "GoCore.h"
#include "GoInventory.h"
#include "GoDb.h"
#include "ReportSys.h"
#include "Rules.h"
#include "World.h"
#include "WorldMap.h"
#include "WorldTime.h"

FUBI_REPLACE_NAME( "Goid_", Goid );
FUBI_EXPORT_POINTER_CLASS( Goid_ );
FUBI_REPLACE_NAME( "Scid_", Scid );
FUBI_EXPORT_POINTER_CLASS( Scid_ );
FUBI_REPLACE_NAME( "PlayerId_", PlayerId );
FUBI_EXPORT_POINTER_CLASS( PlayerId_ );
FUBI_REPLACE_NAME( "FrustumId_", FrustumId );
FUBI_EXPORT_POINTER_CLASS( FrustumId_ );
FUBI_REPLACE_NAME( "TeamId_", TeamId );
FUBI_EXPORT_POINTER_CLASS( TeamId_ );
FUBI_REPLACE_NAME( "RegionId_", RegionId );
FUBI_EXPORT_POINTER_CLASS( RegionId_ );
FUBI_REPLACE_NAME( "GenericId_", GenericId );
FUBI_EXPORT_POINTER_CLASS( GenericId_ );

FUBI_EXPORT_ENUM( ePlayerController, PC_BEGIN, PC_COUNT );
FUBI_EXPORT_ENUM( eLifeState, LS_BEGIN, LS_COUNT );
FUBI_EXPORT_ENUM( eActorAlignment, AA_BEGIN, AA_COUNT );
FUBI_EXPORT_ENUM( ePContentType, PT_BEGIN, PT_COUNT );
FUBI_EXPORT_ENUM( eEquipSlot, ES_BEGIN, ES_COUNT );
FUBI_EXPORT_ENUM( eInventoryLocation, IL_BEGIN, IL_COUNT );
FUBI_EXPORT_ENUM( eAttackClass, AC_BEGIN, AC_COUNT );
FUBI_EXPORT_ENUM( eDefendClass, DC_BEGIN, DC_COUNT );
FUBI_EXPORT_ENUM( eMagicClass, MC_BEGIN, MC_COUNT );

FUBI_EXPORT_BITFIELD_ENUM( eTargetTypeFlags );
FUBI_EXPORT_BITFIELD_ENUM( eUsageContextFlags );

FUBI_EXPORT_ENUM( eQueryTrait, QT_BEGIN, QT_COUNT );
FUBI_EXPORT_ENUM( eActionOrigin, AO_BEGIN, AO_COUNT );
FUBI_EXPORT_ENUM( eVoiceSound, VS_BEGIN, VS_COUNT );

//////////////////////////////////////////////////////////////////////////////
// macro helper implementations

bool IsServerLocal( void )
{
	return ( !Server::DoesSingletonExist() || gServer.IsLocal() );
}

bool IsMultiPlayer( void )
{
	return( Server::DoesSingletonExist() && gServer.IsMultiPlayer() );
}

bool IsPrimaryThread( void )
{
	return ( !GoDb::DoesSingletonExist() || (gGoDb.GetPrimaryThreadId() == ::GetCurrentThreadId()) );
}

bool IsWorldEditMode( void )
{
	return ( GoDb::DoesSingletonExist() && gGoDb.IsEditMode() );
}

bool IsSendLocalOnly( DWORD machineId )
{
	if ( ::IsSinglePlayer() )
	{
		return ( machineId != RPC_TO_OTHERS );
	}

	if ( machineId == RPC_TO_LOCAL )
	{
		return ( true );
	}

	if ( machineId == RPC_TO_SERVER )
	{
		return ( ::IsServerLocal() );
	}

	return ( false );
}

unsigned int GetWorldSimCount( void )
{
	return ( WorldTime::DoesSingletonExist() ? gWorldTime.GetSimCount() : 0 );
}

int LoaderScope::ms_InstanceCount = 0;

//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

static ReportSys::Context* gGoLifeContextPtr   = NULL;

ReportSys::Context& GetGoLifeContext( void )
{
	if ( gGoLifeContextPtr == NULL )
	{
		static ReportSys::Context s_GoLifeContext( &gGenericContext, "GoLife" );
		gGoLifeContextPtr = &s_GoLifeContext;

		s_GoLifeContext.Enable( false );
	}
	return ( *gGoLifeContextPtr );
}

static ReportSys::Context* gGoMsgContextPtr   = NULL;

ReportSys::Context& GetGoMsgContext( void )
{
	if ( gGoMsgContextPtr == NULL )
	{
		static ReportSys::Context s_GoMsgContext( &gGenericContext, "GoMsg" );
		gGoMsgContextPtr = &s_GoMsgContext;

		s_GoMsgContext.Enable( false );
	}
	return ( *gGoMsgContextPtr );
}

static ReportSys::Context* gGoUberContextPtr   = NULL;

ReportSys::Context& GetGoUberContext( void )
{
	if ( gGoUberContextPtr == NULL )
	{
		static ReportSys::Context s_GoUberContext( &gGenericContext, "GoUber" );
		gGoUberContextPtr = &s_GoUberContext;

		s_GoUberContext.AddSink( new ReportSys::LogFileSink <> ( "uber.log" ), true );
		s_GoUberContext.SetType( "Log" );
		s_GoUberContext.Enable( false );
	}
	return ( *gGoUberContextPtr );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// numbers

void GoRngNR :: SetSeed( DWORD seed )
{
	m_Rng.SetSeed( seed );
}

DWORD GoRngNR :: RandomDword( void )
{
	DWORD rn = m_Rng.RandomDword();

#	if !GP_RETAIL
	if ( m_LogRefs > 0 )
	{
		m_RngLog.push_back( rn );
	}
#	endif // !GP_RETAIL

	return ( rn );
}

#if !GP_RETAIL

GoRngNR::AutoLog :: AutoLog( void )
{
	GoRngNR& rng = GetGoCreateRng();
	++rng.m_LogRefs;
}

GoRngNR::AutoLog :: ~AutoLog( void )
{
	GoRngNR& rng = GetGoCreateRng();
	--rng.m_LogRefs;
	gpassert( rng.m_LogRefs >= 0 );

	if ( (rng.m_LogRefs == 0) && !rng.m_RngLog.empty() )
	{
		gplog( OutputBinary( 'GRng', const_mem_ptr( &*rng.m_RngLog.begin(), rng.m_RngLog.size_bytes() ) ) );
		rng.m_RngLog.clear();
	}
}

#endif // !GP_RETAIL

GoRngNR& GetGoCreateRng( void )
{
	static GoRngNR s_MainRng( "GoCreate:Main" ), s_LoaderRng( "GoCreate:Loader" );
	return ( ::IsPrimaryThread() ? s_MainRng : s_LoaderRng );
}

//////////////////////////////////////////////////////////////////////////////
// class Goid implementation

static int s_AutoIncrement = 0;

const Goid GOID_INVALID = MakeGoid( 0 );
const Goid GOID_NONE	= MakeConstantGoid( s_AutoIncrement++ );
const Goid GOID_ANY     = MakeConstantGoid( s_AutoIncrement++ );

bool IsValid( Goid g, bool testExists )
{
	if ( testExists )
	{
		return ( GoHandle( g ).IsValid() );
	}
	else
	{
		return ( g != GOID_INVALID );
	}
}

bool IsValidMp( Goid g )
{
	GoHandle go( g );
	return ( go && !go->IsMarkedForDeletion() );
}

Go* GetGo( Goid g )
{
	return ( GoHandle( g ) );
}

Scid GetScid( Goid g )
{
	return ( GoHandle( g )->GetScid() );
}

const char* GoidClassToString( DWORD gc )
{
	switch ( gc )
	{
		case ( GO_CLASS_GLOBAL ):		return ( "GO_CLASS_GLOBAL"    );
		case ( GO_CLASS_LOCAL ):		return ( "GO_CLASS_LOCAL"     );
		case ( GO_CLASS_CLONE_SRC ):	return ( "GO_CLASS_CLONE_SRC" );
		case ( GO_CLASS_CONSTANT ):		return ( "GO_CLASS_CONSTANT"  );
	}

	return ( "UNKNOWN" );
}

#if !GP_RETAIL

const char* GoidToDebugString( Goid g )
{
	if ( g == GOID_INVALID )
	{
		return ( "<system>" );
	}
	else if ( g == GOID_ANY )
	{
		return ( "<any>" );
	}
	else if ( g == GOID_NONE )
	{
		return ( "<none>" );
	}

	GoHandle go( g );
	if ( go )
	{
		return ( go->GetTemplateName() );
	}
	else
	{
		return ( "<unknown>" );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct GopColl implementation

void GopColl :: Translate( GoidColl& out, Goid ignore ) const
{
	for ( const_iterator i = begin(), e = end() ; i != e ; ++i )
	{
		Goid translated = (*i)->GetGoid();
		if( translated != ignore )
		{
			out.push_back( translated );
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// struct GoidColl implementation

int GoidColl :: Translate( GopColl& out ) const
{
	int old = out.size();

	for ( const_iterator i = begin(), e = end() ; i != e ; ++i )
	{
		GoHandle go( *i );
		if ( go )
		{
			out.push_back( go.Get() );
		}
	}

	return ( out.size() - old );
}

//////////////////////////////////////////////////////////////////////////////
// type Scid implementation

const Scid SCID_INVALID = MakeScid( (DWORD)0 );
const Scid SCID_SPAWNED = MakeScid( (DWORD)-2 );

bool IsValid( Scid s, bool testExists )
{
	if ( testExists )
	{
		return ( WorldMap::DoesSingletonExist() && gWorldMap.ContainsScid( s ) );
	}
	else
	{
		return ( (s != SCID_INVALID) && (s != MakeScid( (DWORD)-1) ) );
	}
}

Goid GetGoid( Scid s )
{
	return ( gGoDb.FindGoidByScid( s ) );
}

Go* GetGo( Scid s )
{
	return ( GetGo( GetGoid( s ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// type PlayerId implementation

const PlayerId PLAYERID_INVALID  = MakePlayerId( 0 );
//const PlayerId PLAYERID_NEXTFREE = MakePlayerId( 0 );
const PlayerId PLAYERID_COMPUTER = MakePlayerId( 1 );

bool IsValid( PlayerId p, bool testExists )
{
	if ( testExists )
	{
		return ( gServer.HasPlayer( p ) );
	}
	else
	{
		return ( (p != PLAYERID_INVALID) && ::IsPower2( (DWORD)p ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
// type FrustumId implementation

const FrustumId FRUSTUMID_INVALID  = MakeFrustumId( 0 );
const FrustumId FRUSTUMID_NEXTFREE = MakeFrustumId( 0 );

bool IsValid( FrustumId f, bool testExists )
{
	if ( testExists )
	{
		return ( gSiegeEngine.GetFrustum( MakeInt( f ) ) != NULL );
	}
	else
	{
		return ( (f != FRUSTUMID_INVALID) && ::IsPower2( (DWORD)f ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
// enum ePlayerController implementation

static const char* s_PlayerControllerStrings[] =
{
	"pc_invalid",
	"pc_human",
	"pc_computer",
	"pc_remote_human",
	"pc_remote_computer",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_PlayerControllerStrings ) == PC_COUNT );

static stringtool::EnumStringConverter s_PlayerControllerConverter(
		s_PlayerControllerStrings, PC_BEGIN, PC_END );

const char* ToString( ePlayerController pc )
{
	return ( s_PlayerControllerConverter.ToString( pc ) );
}

bool FromString( const char* str, ePlayerController& pc )
{
	return ( s_PlayerControllerConverter.FromString( str, rcast <int&> ( pc ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eLifeState implementation

static const char* s_LifeStateStrings[] =
{
	"ls_ignore",
	"ls_alive_conscious",
	"ls_alive_unconscious",
	"ls_dead_normal",
	"ls_dead_charred",
	"ls_decay_fresh",
	"ls_decay_bones",
	"ls_decay_dust",
	"ls_gone",
	"ls_ghost",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_LifeStateStrings ) == LS_COUNT );

static stringtool::EnumStringConverter s_LifeStateConverter(
		s_LifeStateStrings, LS_BEGIN, LS_END, LS_IGNORE );

const char* ToString( eLifeState ls )
{
	return ( s_LifeStateConverter.ToString( ls ) );
}

bool FromString( const char* str, eLifeState& ls )
{
	return ( s_LifeStateConverter.FromString( str, rcast <int&> ( ls ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eActorAlignment implementation

static const char* s_ActorAlignmentStrings[] =
{
	"aa_good",
	"aa_neutral",
	"aa_evil",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ActorAlignmentStrings ) == AA_COUNT );

static stringtool::EnumStringConverter s_ActorAlignmentConverter( s_ActorAlignmentStrings, AA_BEGIN, AA_END );

const char* ToString( eActorAlignment e )
{
	return ( s_ActorAlignmentConverter.ToString( e ) );
}

bool FromString( const char* str, eActorAlignment& e )
{
	return ( s_ActorAlignmentConverter.FromString( str, rcast <int&> ( e ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum ePContentType implementation

static const char* s_PContentTypeStrings[] =
{
	// basic
	"pt_armor",
	"pt_weapon",
	"pt_amulet",
	"pt_ring",
	"pt_spell",
	"pt_scroll",
	"pt_potion",
	"pt_spellbook",

	// armor
	"pt_body",
	"pt_helm",
	"pt_gloves",
	"pt_boots",
	"pt_shield",

	// weapon
	"pt_melee",
	"pt_ranged",

	// spells
	"pt_cmagic",
	"pt_nmagic",

	// final
	"pt_invalid",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_PContentTypeStrings ) == PT_COUNT );

static stringtool::EnumStringConverter s_PContentTypeConverter( s_PContentTypeStrings, PT_BEGIN, PT_END, PT_INVALID );

const char* ToString( ePContentType e )
{
	return ( s_PContentTypeConverter.ToString( e ) );
}

const char* ToQueryString( ePContentType e )
{
	return ( ToString( e ) + 3 );
}

ePContentType ToBasicPContentType( ePContentType e )
{
	if ( IsArmorPContentType( e ) )
	{
		return ( PT_ARMOR );
	}
	else if ( IsWeaponPContentType( e ) )
	{
		return ( PT_WEAPON );
	}
	else if ( IsSpellPContentType( e ) )
	{
		return ( PT_SPELL );
	}
	return ( e );
}

bool FromString( const char* str, ePContentType& e )
{
	return ( s_PContentTypeConverter.FromString( str, rcast <int&> ( e ) ) );
}

bool FromQueryString( const char* str, ePContentType& e )
{
	int len = ::strlen( str ) + 1;
	char* localStr = (char*)::_alloca( len + 3 );
	localStr[ 0 ] = 'p';
	localStr[ 1 ] = 't';
	localStr[ 2 ] = '_';
	::memcpy( localStr + 3, str, len );
	return ( FromString( localStr, e ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eEquipSlot implementation

static const char* s_EquipSlotStrings[] =
{
	// visible
	"es_shield_hand",
	"es_weapon_hand",
	"es_feet",
	"es_chest",
	"es_head",
	"es_forearms",

	// not visible
	"es_amulet",
	"es_spellbook",
	"es_ring_0",
	"es_ring_1",
	"es_ring_2",
	"es_ring_3",

	// special
	"es_ring",

	// final
	"es_none",
	"es_any",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_EquipSlotStrings ) == ES_COUNT );

static stringtool::EnumStringConverter s_EquipSlotConverter( s_EquipSlotStrings, ES_BEGIN, ES_END, ES_NONE );

const char* ToString( eEquipSlot e )
{
	return ( s_EquipSlotConverter.ToString( e ) );
}

bool FromString( const char* str, eEquipSlot& e )
{
	return ( s_EquipSlotConverter.FromString( str, rcast <int&> ( e ) ) );
}

const char* GetArmorString( eEquipSlot e )
{
	const char* str = "";

	switch ( e )
	{
		case ( ES_CHEST ):
		{
			str = "suit";
		}
		break;

		case ( ES_FEET ):
		{
			str = "boot";
		}
		break;

		case ( ES_SHIELD_HAND ):
		case ( ES_WEAPON_HAND ):
		case ( ES_FOREARMS    ):
		{
			str = "gntl";
		}
		break;

		case ( ES_HEAD ):
		{
			str = "hlmt";
		}
		break;
	}

	return ( str );
}

ePContentType ToPContentType( eEquipSlot e )
{
	switch ( e )
	{
		case ( ES_SHIELD_HAND ):  return ( PT_SHIELD );
		case ( ES_WEAPON_HAND ):  return ( PT_WEAPON );
		case ( ES_FEET        ):  return ( PT_BOOTS  );
		case ( ES_CHEST       ):  return ( PT_BODY   );
		case ( ES_HEAD        ):  return ( PT_HELM   );
		case ( ES_FOREARMS    ):  return ( PT_GLOVES );
		case ( ES_AMULET      ):  return ( PT_AMULET );
		case ( ES_RING_0      ):  return ( PT_RING   );
		case ( ES_RING_1      ):  return ( PT_RING   );
		case ( ES_RING_2      ):  return ( PT_RING   );
		case ( ES_RING_3      ):  return ( PT_RING   );
		case ( ES_RING        ):  return ( PT_RING   );
	}

	return ( PT_INVALID );
}

///////////////////////////////////////////////////////////////////////
// enum eInventoryLocation implementation

static const char* s_InventoryLocationStrings[] =
{
	"il_active_melee_weapon",
	"il_active_ranged_weapon",
	"il_active_primary_spell",
	"il_active_secondary_spell",
	"il_spell_1",
	"il_spell_2",
	"il_spell_3",
	"il_spell_4",
	"il_spell_5",
	"il_spell_6",
	"il_spell_7",
	"il_spell_8",
	"il_spell_9",
	"il_spell_10",
	"il_spell_11",
	"il_spell_12",
	"il_shield",
	"il_invalid",
	"il_all",
	"il_all_active",
	"il_all_spells",
	"il_main",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_InventoryLocationStrings ) == IL_COUNT );

static stringtool::EnumStringConverter s_InventoryLocationConverter( s_InventoryLocationStrings, IL_BEGIN, IL_END, IL_INVALID );

bool FromString( const char* str, eInventoryLocation& e )
{
	return ( s_InventoryLocationConverter.FromString( str, rcast <int&> ( e ) ) );
}

const char* ToString( eInventoryLocation e )
{
	return ( s_InventoryLocationConverter.ToString( e ) );
}

///////////////////////////////////////////////////////////////////////
// enum eAttackClass implementation

static const char* s_AttackClassStrings[] =
{
	"ac_beastfu",
	"ac_axe",
	"ac_club",
	"ac_dagger",
	"ac_hammer",
	"ac_mace",
	"ac_staff",
	"ac_sword",
	"ac_bow",
	"ac_minigun",
	"ac_arrow",
	"ac_bolt",
	"ac_combat_magic",
	"ac_nature_magic",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_AttackClassStrings ) == AC_COUNT );

static stringtool::EnumStringConverter s_AttackClassConverter( s_AttackClassStrings, AC_BEGIN, AC_END );

const char* ToString( eAttackClass e )
{
	return ( s_AttackClassConverter.ToString( e ) );
}

const char* ToQueryString( eAttackClass e )
{
	return ( s_AttackClassConverter.ToString( e ) + 3 );
}

ePContentType ToPContentType( eAttackClass e )
{
	switch ( e )
	{
		case ( AC_BEASTFU	   ):  return ( PT_MELEE  );
		case ( AC_AXE		   ):  return ( PT_MELEE  );
		case ( AC_CLUB		   ):  return ( PT_MELEE  );
		case ( AC_DAGGER	   ):  return ( PT_MELEE  );
		case ( AC_HAMMER	   ):  return ( PT_MELEE  );
		case ( AC_MACE		   ):  return ( PT_MELEE  );
		case ( AC_STAFF		   ):  return ( PT_MELEE  );
		case ( AC_SWORD		   ):  return ( PT_MELEE  );
		case ( AC_BOW		   ):  return ( PT_RANGED );
		case ( AC_MINIGUN	   ):  return ( PT_RANGED );
		case ( AC_ARROW		   ):  return ( PT_RANGED );
		case ( AC_BOLT		   ):  return ( PT_RANGED );
		case ( AC_COMBAT_MAGIC ):  return ( PT_RANGED );
		case ( AC_NATURE_MAGIC ):  return ( PT_RANGED );
	}

	return ( PT_INVALID );
}

bool FromString( const char* str, eAttackClass& e )
{
	return ( s_AttackClassConverter.FromString( str, rcast <int&> ( e ) ) );
}

bool FromQueryString( const char* str, eAttackClass& e )
{
	int len = ::strlen( str ) + 1;
	char* localStr = (char*)::_alloca( len + 3 );
	localStr[ 0 ] = 'a';
	localStr[ 1 ] = 'c';
	localStr[ 2 ] = '_';
	::memcpy( localStr + 3, str, len );
	return ( FromString( localStr, e ) );
}

///////////////////////////////////////////////////////////////////////
// enum eDefendClass implementation

static const char* s_DefendClassStrings[] =
{
	"dc_skin",
	"dc_shield",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_DefendClassStrings ) == DC_COUNT );

static stringtool::EnumStringConverter s_DefendClassConverter( s_DefendClassStrings, DC_BEGIN, DC_END );

bool FromString( const char* str, eDefendClass& e )
{
	return ( s_DefendClassConverter.FromString( str, rcast <int&> ( e ) ) );
}

const char* ToString( eDefendClass e )
{
	return ( s_DefendClassConverter.ToString( e ) );
}

///////////////////////////////////////////////////////////////////////
// enum eMagicClass implementation

static const char* s_MagicClassStrings[] =
{
	"mc_none",
	"mc_potion",
	"mc_combat_magic",
	"mc_nature_magic",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_MagicClassStrings ) == MC_COUNT );

static stringtool::EnumStringConverter s_MagicClassConverter( s_MagicClassStrings, MC_BEGIN, MC_END );

bool FromString( const char* str, eMagicClass& e )
{
	return ( s_MagicClassConverter.FromString( str, rcast <int&> ( e ) ) );
}

const char* ToString( eMagicClass e )
{
	return ( s_MagicClassConverter.ToString( e ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eTargetTypeFlags implementation

static const stringtool::BitEnumStringConverter::Entry s_TargetTypeStrings[] =
{
	// Bit to specify AND operation
	{	"tt_none",					TT_NONE,				},
	{	"tt_and",					TT_AND,					},

	// ACTORS

	// go type

	{	"tt_actor",					TT_ACTOR,				},
	{	"tt_actor_pack_only",		TT_ACTOR_PACK_ONLY,		},
	{	"tt_not_actor",				TT_NOT_ACTOR,			},
	{	"tt_self",					TT_SELF,				},
	{	"tt_breakable",				TT_BREAKABLE,			},

	{	"tt_can_store_mana",		TT_CAN_STORE_MANA,		},
	{	"tt_injured_friend",		TT_INJURED_FRIEND,		},

	// tactical

	{	"tt_dead_friend",			TT_DEAD_FRIEND			},
	{	"tt_unconscious_friend",	TT_UNCONSCIOUS_FRIEND	},
	{	"tt_conscious_friend",		TT_CONSCIOUS_FRIEND		},

	{	"tt_dead_enemy",			TT_DEAD_ENEMY			},
	{	"tt_unconscious_enemy",		TT_UNCONSCIOUS_ENEMY	},
	{	"tt_conscious_enemy",		TT_CONSCIOUS_ENEMY		},

	{	"tt_summoned",				TT_SUMMONED				},
	{	"tt_possessed",				TT_POSSESSED			},
	{	"tt_animated",				TT_ANIMATED				},

	{	"tt_human_party_member",	TT_HUMAN_PARTY_MEMBER	},
	{	"tt_screen_party_member",	TT_SCREEN_PARTY_MEMBER	},

	// alignment

	{	"tt_good",					TT_GOOD,				},
	{	"tt_neutral",				TT_NEUTRAL,				},
	{	"tt_evil",					TT_EVIL,				},

	// items

	{	"tt_weapon_melee",			TT_WEAPON_MELEE,		},
	{	"tt_weapon_ranged",			TT_WEAPON_RANGED,		},
	{	"tt_armor",					TT_ARMOR,				},
	{	"tt_shield",				TT_SHIELD,				},
	{	"tt_equippable",			TT_EQUIPPABLE,			},

	{	"tt_transmutable",			TT_TRANSMUTABLE,		},
	
	{	"tt_terrain",				TT_TERRAIN,				},
};

static stringtool::BitEnumStringConverter
	s_TargetTypeConverter( s_TargetTypeStrings, ELEMENT_COUNT( s_TargetTypeStrings ), TT_ACTOR );

const char* ToString( eTargetTypeFlags e )
{
	return ( s_TargetTypeConverter.ToString( e ) );
}

gpstring ToFullString( eTargetTypeFlags e )
{
	return ( s_TargetTypeConverter.ToFullString( e ) );
}

bool FromString( const char* str, eTargetTypeFlags & e )
{
	return ( s_TargetTypeConverter.FromString( str, rcast <DWORD&> ( e ) ) );
}

bool FromFullString( const char* str, eTargetTypeFlags & e )
{
	return ( s_TargetTypeConverter.FromFullString( str, rcast <DWORD&> ( e ) ) );
}


//////////////////////////////////////////////////////////////////////////////
// enum eUsageContextFlags implementation

static const stringtool::BitEnumStringConverter::Entry s_UsageContextStrings[] =
{
	{	"uc_life_giving",			UC_LIFE_GIVING,								},
	{	"uc_life_getting",			UC_LIFE_GETTING,							},

	{	"uc_mana_giving",			UC_MANA_GIVING,								},
	{	"uc_mana_getting",			UC_MANA_GETTING,							},

	{	"uc_reanimating",			UC_REANIMATING,								},

	{	"uc_passive",				UC_PASSIVE,									},
	{	"uc_aggressive",			UC_AGGRESSIVE,								},

	{	"uc_offensive",				UC_OFFENSIVE,								},
	{	"uc_defensive",				UC_DEFENSIVE,								},
};

static stringtool::BitEnumStringConverter
	s_UsageContextConverter( s_UsageContextStrings, ELEMENT_COUNT( s_UsageContextStrings ), UC_LIFE_GIVING );

const char* ToString( eUsageContextFlags e )
{
	return ( s_UsageContextConverter.ToString( e ) );
}

gpstring ToFullString( eUsageContextFlags e )
{
	return ( s_UsageContextConverter.ToFullString( e ) );
}

bool FromString( const char* str, eUsageContextFlags& e )
{
	return ( s_UsageContextConverter.FromString( str, rcast <DWORD&> ( e ) ) );
}

bool FromFullString( const char* str, eUsageContextFlags & e )
{
	return ( s_UsageContextConverter.FromFullString( str, rcast <DWORD&> ( e ) ) );
}


//////////////////////////////////////////////////////////////////////////////
// enum eQueryTrait implementation

static const char* s_QueryTraitStrings[] =
{
	"qt_none",
	"qt_any",
	"qt_life",
	"qt_life_low",
	"qt_life_high",
	"qt_life_healing",
	"qt_life_damaging",
	"qt_mana",
	"qt_mana_low",
	"qt_mana_high",
	"qt_mana_healing",
	"qt_mana_damaging",
	"qt_reanimating",
	"qt_alive_conscious",
	"qt_alive_unconscious",
	"qt_alive",
	"qt_dead",
	"qt_ghost",
	"qt_weapon",
	"qt_melee_weapon",
	"qt_ranged_weapon",
	"qt_melee_weapon_selected",
	"qt_ranged_weapon_selected",
	"qt_one_shot_spell",
	"qt_multiple_shot_spell",
	"qt_command_cast_spell",
	"qt_auto_cast_spell",
	"qt_fighting",
	"qt_actor",
	"qt_item",
	"qt_potion",
	"qt_spell",
	"qt_armor",
	"qt_armor_wearable",
	"qt_shield",
	"qt_castable",
	"qt_attackable",
	"qt_selected",
	"qt_visible",
	"qt_invisible",
	"qt_has_los",
	"qt_pack",
	"qt_nonpack",
	"qt_busy",
	"qt_idle",
	"qt_good",
	"qt_neutral",
	"qt_evil",
	"qt_friend",
	"qt_enemy",
	"qt_survival_factor",
	"qt_offense_factor",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_QueryTraitStrings ) == QT_COUNT );

static stringtool::EnumStringConverter s_QueryTraitConverter( s_QueryTraitStrings, QT_BEGIN, QT_END );

const char* ToString( eQueryTrait e )
{
	return ( s_QueryTraitConverter.ToString( e ) );
}

bool FromString( const char* str, eQueryTrait& e )
{
	return ( s_QueryTraitConverter.FromString( str, rcast <int&> ( e ) ) );
}


///////////////////////////////////////////////////////////////////////
// eActionOrigin

static const char* s_AOStrings[] =
{
	"ao_invalid",
	"ao_human",
	"ao_reflex",
	"ao_party",
	"ao_command",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_AOStrings ) == AO_COUNT );

static stringtool::EnumStringConverter s_AOConverter( s_AOStrings, AO_BEGIN, AO_END, AO_INVALID );

bool FromString( const char* str, eActionOrigin & jo )
{
	return ( s_AOConverter.FromString( str, rcast <int&> ( jo ) ) );
}

const char* ToString( eActionOrigin jo )
{
	return ( s_AOConverter.ToString( jo ) );
}


///////////////////////////////////////////////////////////////////////
// eVoiceSound

static const char* s_VSStrings[] =
{
	"vs_invalid",
	"vs_enemy_spotted",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_VSStrings ) == VS_COUNT );

static stringtool::EnumStringConverter s_VSConverter( s_VSStrings, VS_BEGIN, VS_END );

bool FromString( const char* str, eVoiceSound & jo )
{
	return ( s_VSConverter.FromString( str, rcast <int&> ( jo ) ) );
}

const char* ToString( eVoiceSound jo )
{
	return ( s_VSConverter.ToString( jo ) );
}

const char* ToEventString( eVoiceSound e )
{
	return ( ToString( e ) + 3 );
}


//////////////////////////////////////////////////////////////////////////////
// equip requirements implementation

int EquipRequirementColl :: Load( const gpstring& str )
{
	int oldSize = size();
	bool success = true;

	stringtool::Extractor eachExtractor( ",", str );
	gpstring each;
	while ( eachExtractor.GetNextString( each ) )
	{
		const char* b = each, * iter = b;
		const char* e = stringtool::NextField( iter, ':' );
		if ( e != NULL )
		{
			EquipRequirement* entry = insert( end() );
			entry->m_Skill.assign( b, e );
			success = ::FromString( iter, entry->m_MinRequired );

#			if !GP_RETAIL
			if ( !gRules.GetBaseSkills().HasSkill( entry->m_Skill ) )
			{
				success = false;
			}
#			endif // !GP_RETAIL
		}
		else
		{
			success = false;
		}
	}

	if ( !success )
	{
		gperrorf(( "Illegally formatted/named equip requirements string '%s'\n" ));
	}

	return ( size() - oldSize );
}

void EquipRequirementColl :: Merge( const gpstring& str )
{
	EquipRequirementColl coll;
	if ( coll.Load( str ) )
	{
		for ( const_iterator i = coll.begin() ; i != coll.end() ; ++i )
		{
			for ( iterator j = begin() ; j != end() ; ++j )
			{
				// found? update it
				if ( j->m_Skill.same_no_case( i->m_Skill ) )
				{
					j->m_MinRequired += i->m_MinRequired;
					break;
				}
			}

			// not found? add it
			if ( j == end() )
			{
				push_back( *i );
			}
		}
	}
}

gpstring EquipRequirementColl :: ToString( void ) const
{
	gpstring str;

	for ( const_iterator i = begin() ; i != end() ; ++i )
	{
		str.appendf( str.empty() ? "%s:%f" : ",%s:%f", i->m_Skill.c_str(), i->m_MinRequired );
	}

	return ( str );
}

gpwstring EquipRequirementColl :: MakeToolTipString( Go* forActor ) const
{
	GoActor* owner = forActor ? forActor->QueryActor() : NULL;
	gpwstring entry( gpwtranslate( $MSG$ "Requires %s %.f\n" ) );
	gpwstring text;

	for ( const_iterator i = begin() ; i != end() ; ++i )
	{
		if ( i->m_MinRequired > 0 )
		{
			const Skill* skill = NULL;
			if ( gRules.GetBaseSkills().GetSkill( i->m_Skill, &skill ) )
			{
				if ( owner && (owner->GetSkillLevel( i->m_Skill ) < i->m_MinRequired) && forActor->HasInventory() && !forActor->GetInventory()->IsPackOnly() )
				{
					text += L"<required>";
				}
				text.appendf( entry, skill->GetScreenName().c_str(), i->m_MinRequired );
			}
		}
	}

	return ( text );
}

bool EquipRequirementColl :: CanEquip( Go* forActor ) const
{
	GoActor* actor = forActor ? forActor->QueryActor() : NULL;
	bool can = actor != NULL;

	if ( can )
	{
		for ( const_iterator i = begin() ; i != end() ; ++i )
		{
			if ( actor->GetSkillLevel( i->m_Skill ) < i->m_MinRequired )
			{
				can = false;
				break;
			}
		}
	}

	return ( can );
}

//////////////////////////////////////////////////////////////////////////////
// struct MpWorld implementation

bool MpWorld :: Load( FastFuelHandle fuel )
{
	m_Name = fuel.GetName();

	FuBi::FastFuelReader reader( fuel );
	FuBi::PersistContext persist( &reader, false );
	return ( Xfer( persist ) );
}

bool MpWorld :: Load( FuelHandle fuel )
{
	m_Name = fuel->GetName();

	FuBi::FuelReader reader( fuel );
	FuBi::PersistContext persist( &reader, false );
	return ( Xfer( persist ) );
}

bool MpWorld :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "screen_name",    m_ScreenName    );
	persist.Xfer( "description",    m_Description   );
	persist.Xfer( "required_level", m_RequiredLevel );

	if ( persist.IsRestoring() )
	{
		m_ScreenName  = ReportSys::TranslateW( m_ScreenName );
		m_Description = ReportSys::TranslateW( m_Description );

		// set screen name by default (it has to have a name!)
		if ( m_ScreenName.empty() )
		{
			m_ScreenName.assignf( L"[%S]", m_Name.c_str() );
		}
	}

	return ( true );
}

gpstring MpWorldColl :: GetDefaultMpWorldName( void ) const
{
	if ( empty() )
	{
		return ( gpstring::EMPTY );
	}

	// find (first) minimum required level
	const_iterator i = begin(), imin = i;
	for ( ++i ; i != end() ; ++i )
	{
		if ( i->m_RequiredLevel < imin->m_RequiredLevel )
		{
			imin = i;
		}
	}

	// take it and run
	return ( imin->m_Name );
}

bool MpWorldColl :: Load( FastFuelHandle fuel )
{
	FastFuelHandleColl worlds;
	fuel.ListChildren( worlds );

	FastFuelHandleColl::iterator i, ibegin = worlds.begin(), iend = worlds.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		MpWorld& mpWorld = *insert( end(), MpWorld() );
		mpWorld.Load( *i );
	}

	sort( MpWorld::LessByRequiredLevel() );

	return ( true );
}

bool MpWorldColl :: Load( FuelHandle fuel )
{
	FuelHandleList worlds = fuel->ListChildBlocks();

	FuelHandleList::iterator i, ibegin = worlds.begin(), iend = worlds.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		MpWorld& mpWorld = *insert( end(), MpWorld() );
		mpWorld.Load( *i );
	}

	sort( MpWorld::LessByRequiredLevel() );

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
