//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoActor.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoActor.h"

#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "GoAspect.h"
#include "GoData.h"
#include "GoSupport.h"
#include "ReportSys.h"
#include "Rules.h"
#include "ui_shell.h"
#include "WorldTime.h"
#include "config.h"
#include "victory.h"
#include "worldMap.h"

DECLARE_GO_COMPONENT( GoActor, "actor" );

//////////////////////////////////////////////////////////////////////////////
// class GoActor::GenericState implementation

GoActor::GenericState :: GenericState( const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level )
{
	m_sDescription	= ReportSys::TranslateW( sDescription );
	m_caster		= caster;
	m_spell			= spell;
	m_spellLevel	= spell_level;
	m_absTimeout	= absoluteTimeout;
}

bool GoActor::GenericState :: Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_sDescription", m_sDescription );
	persist.Xfer( "m_caster",       m_caster       );
	persist.Xfer( "m_spell",        m_spell        );
	persist.Xfer( "m_spellLevel",   m_spellLevel   );
	persist.Xfer( "m_absTimeout",   m_absTimeout   );

	return true;
}

void GoActor::GenericState :: Xfer( FuBi::BitPacker &packer )
{
	packer.XferString( m_sDescription );
	packer.XferRaw   ( m_caster       );
	packer.XferRaw   ( m_spell        );
	packer.XferFloat ( m_spellLevel   );
	packer.XferRaw   ( m_absTimeout   );
}

FUBI_DECLARE_SELF_TRAITS( GoActor::GenericState );

//////////////////////////////////////////////////////////////////////////////
// class GoActor implementation

GoActor :: GoActor( Go* go )
	: Inherited( go )
{
	m_Alignment                 = AA_NEUTRAL;
	m_PortraitTex               = 0;
	m_ChanceToHitBonusStart     = 0;
	m_ChanceToHitBonusKillStep  = 0;
	m_ChanceToHitBonusIncrement = 0;
	m_ChanceToHitBonusMax       = 0;
	m_Options                   = OPTION_NONE;
	m_bCanBePickedUp			= false;
	m_bCanShowHealth			= false;
	m_bEnchantmentRemoved		= false;
	m_UnconsciousEndTime		= 0;
	m_bMPScaled					= false;
}

GoActor :: GoActor( const GoActor& source, Go* newGo )
	: Inherited  ( source, newGo      )
	, m_Skills   ( source.m_Skills    )
	, m_Class    ( source.m_Class     )
{
	m_Alignment                 = source.m_Alignment;
	m_PortraitTex               = 0;
	m_ChanceToHitBonusStart     = source.m_ChanceToHitBonusStart;
	m_ChanceToHitBonusKillStep  = source.m_ChanceToHitBonusKillStep;
	m_ChanceToHitBonusIncrement = source.m_ChanceToHitBonusIncrement;
	m_ChanceToHitBonusMax       = source.m_ChanceToHitBonusMax;
	m_Options                   = source.m_Options;
	m_bCanBePickedUp			= source.m_bCanBePickedUp;
	m_bCanShowHealth			= source.m_bCanShowHealth;
	m_bEnchantmentRemoved		= source.m_bEnchantmentRemoved;
	m_UnconsciousEndTime		= source.m_UnconsciousEndTime;
	m_bMPScaled					= false;

	// set their owners
	SkillColl::iterator i, begin = m_Skills.begin(), end = m_Skills.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->SetOwnerGo( GetGo() );
	}
}

GoActor :: ~GoActor( void )
{
	SetPortraitTexture( 0, false );
}

void GoActor :: SAddGenericState( const char* sName, const char* sDescription, float timeout, Goid caster, Goid spell, float spell_level )
{
	CHECK_SERVER_ONLY;

	// convert into absolute server time for transfer
	RCAddGenericState( sName, sDescription, PreciseAdd( timeout, gWorldTime.GetTime() ), caster, spell, spell_level );
}

void GoActor :: RCAddGenericState( const char* sName, const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level )
{
	FUBI_RPC_THIS_CALL( RCAddGenericState, RPC_TO_ALL );

	AddGenericState( sName, sDescription, absoluteTimeout, caster, spell, spell_level );
}

void GoActor :: AddGenericState( const char* sName, const char* sDescription, double absoluteTimeout, Goid caster, Goid spell, float spell_level )
{
	GetGo()->SetBucketDirty();

	m_GenericStateStorage.insert( std::make_pair( sName, GenericState( sDescription, absoluteTimeout, caster, spell, spell_level ) ) );
}

void GoActor :: SRemoveGenericState( const char* sName )
{
	CHECK_SERVER_ONLY;

	RCRemoveGenericState( sName );
}

void GoActor :: RCRemoveGenericState( const char* sName )
{
	FUBI_RPC_THIS_CALL( RCRemoveGenericState, RPC_TO_ALL );

	RemoveGenericState( sName );
}

void GoActor :: RemoveGenericState( const char* sName )
{
	GSColl::iterator iGS = m_GenericStateStorage.find( sName );

	if( iGS != m_GenericStateStorage.end() )
	{
		m_GenericStateStorage.erase( iGS );
	}
}

bool GoActor :: HasGenericState( const char* sName ) const
{
	GSColl::const_iterator iGS = m_GenericStateStorage.find( sName );

	if( iGS != m_GenericStateStorage.end() )
	{
		return true;
	}
	return false;
}

float GoActor :: GetGenericStateSpellLevel( const char* sName ) const
{
	GSColl::const_iterator iGS = m_GenericStateStorage.find( sName );

	if( iGS != m_GenericStateStorage.end() )
	{
		return (*iGS).second.m_spellLevel;
	}
	return 0;
}

Goid GoActor :: GetGenericStateSpellGoid( const char* sName ) const
{
	GSColl::const_iterator iGS = m_GenericStateStorage.find( sName );

	if( iGS != m_GenericStateStorage.end() )
	{
		return (*iGS).second.m_spell;
	}
	return GOID_INVALID;
}

int GoActor :: GetGenericStateInfo( gpwstring &sUIDescriptions, int numStates ) const
{
	double time = gWorldTime.GetTime();
	int index = 0;

	GSColl::const_iterator iGS = m_GenericStateStorage.begin(), gs_end = m_GenericStateStorage.end();
	for ( ; iGS != gs_end ; ++iGS )
	{
		if ( numStates == -1 || index <= numStates )
		{
			const GenericState& gs = iGS->second;
			float delta = (float) ( PreciseSubtract(gs.m_absTimeout, time) );

			if ( delta > 0 )
			{
				if ( iGS != m_GenericStateStorage.begin() )
				{
					sUIDescriptions.append( L", " );
				}

				int time_left = ::Round( delta );

				sUIDescriptions.appendf( L"%s - <c:0x%x>%d</c>",
						gs.m_sDescription.c_str(),		// note: this is already translated
						gUIShell.GetTipColor( "magic" ),
						time_left );
			}
		}
		index++;
	}

	return index;
}

void GoActor :: SetPortraitTexture( DWORD portrait, bool addRef )
{
	if ( m_PortraitTex != portrait )
	{
		if ( m_PortraitTex != 0 )
		{
			gDefaultRapi.DestroyTexture( m_PortraitTex );
		}

		m_PortraitTex = portrait;

		if ( addRef && m_PortraitTex )
		{
			gDefaultRapi.AddTextureReference( m_PortraitTex );
		}
	}
}

FuBiCookie GoActor :: RCUpdatePortraitTexture( void )
{
	FUBI_RPC_THIS_CALL_RETRY( RCUpdatePortraitTexture, GetGo()->GetPlayer()->GetMachineId() );

	SetPortraitTexture( GetGo()->GetPlayer()->GetHeroPortrait(), true );
	return ( RPC_SUCCESS );
}

const gpstring& GoActor :: GetPortraitIcon( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "portrait_icon" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoActor :: GetRace( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "race" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoActor :: GetIsMale( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_male" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoActor :: GetHighestSkillLevel( void ) const
{
	float highest_level = 0;

	SkillColl::const_iterator i = m_Skills.begin(), iend = m_Skills.end();

	for(; i != iend; ++i )
	{
		highest_level = max_t( (*i).GetLevel(), highest_level );
	}

	return highest_level;
}

void GoActor :: CreateSkill( const char* skill )
{
	if ( skill && *skill )
	{
		m_Skills.push_back( Skill( skill ) );
	}
	else
	{
		gperrorf(( "TELL RICK: attempted to create a skill with no name!\n" ));
	}
}

void GoActor :: RCSetSkillLevels( float strength, float intelligence, float dexterity )
{
	FUBI_RPC_THIS_CALL( RCSetSkillLevels, RPC_TO_ALL );

	Skill *pSkill = NULL;

	m_Skills.GetSkill( "strength", &pSkill );
	pSkill->SetNaturalLevel( strength );

	m_Skills.GetSkill( "intelligence", &pSkill );
	pSkill->SetNaturalLevel( intelligence );

	m_Skills.GetSkill( "dexterity", &pSkill );
	pSkill->SetNaturalLevel( dexterity );
}

void GoActor :: SSetAlignment( eActorAlignment alignment )
{
	CHECK_SERVER_ONLY;

	RCSetAlignment( alignment );
}

void GoActor :: RCSetAlignment( eActorAlignment alignment )
{
	FUBI_RPC_THIS_CALL( RCSetAlignment, RPC_TO_ALL );

	SetAlignment( alignment );
}

void GoActor :: SetAlignment( eActorAlignment alignment )
{
	if ( m_Alignment != alignment )
	{
		GetGo()->SetBucketDirty();
		m_Alignment = alignment;
	}
}

void GoActor :: SSetCanShowHealth( bool bSet )
{
	CHECK_SERVER_ONLY;

	RCSetCanShowHealth( bSet );
}

void GoActor :: RCSetCanShowHealth( bool bSet )
{
	FUBI_RPC_THIS_CALL( RCSetCanShowHealth, RPC_TO_ALL );

	SetCanShowHealth( bSet );
}

void GoActor :: SetCanShowHealth( bool bSet )
{
	if ( m_bCanShowHealth != bSet )
	{
		m_bCanShowHealth = bSet;
		GetGo()->SetBucketDirty();
	}
}

void GoActor :: RecalcModifiers( void )
{
	// possibly recalc other stats if we're a character actor
	if ( GetCanLevelUp() )
	{
		gRules.RecalculateMaxMana( GetGo() );
		gRules.RecalculateMaxLife( GetGo() );
	}
}

GoComponent* GoActor :: Clone( Go* newGo )
{
	return ( new GoActor( *this, newGo ) );
}

bool GoActor :: Xfer( FuBi::PersistContext& persist )
{

// Easy stuff.

	// traits
	persist.Xfer      ( "alignment",        m_Alignment      );
	persist.Xfer      ( "can_be_picked_up", m_bCanBePickedUp );
	persist.Xfer      ( "can_show_health",  m_bCanShowHealth );
	persist.XferQuoted( "screen_class",     m_Class          );
	persist.Xfer	  ( "m_bMPScaled",		m_bMPScaled		 );

	// options
	persist.XferOption( "can_level_up",    *this, OPTION_CAN_LEVEL_UP    );
	persist.XferOption( "is_hero",         *this, OPTION_IS_HERO         );
	persist.XferOption( "is_charmable",    *this, OPTION_IS_CHARMABLE    );
	persist.XferOption( "is_possessable",  *this, OPTION_IS_POSSESSABLE  );
	persist.XferOption( "drops_spellbook", *this, OPTION_DROPS_SPELLBOOK );

	persist.Xfer( "chance_to_hit_bonus_start",     m_ChanceToHitBonusStart     );
	persist.Xfer( "chance_to_hit_bonus_kill_step", m_ChanceToHitBonusKillStep  );
	persist.Xfer( "chance_to_hit_bonus_increment", m_ChanceToHitBonusIncrement );
	persist.Xfer( "chance_to_hit_bonus_max",       m_ChanceToHitBonusMax       );


// Complex stuff.

	if ( persist.IsFullXfer() )
	{
		// skills
		m_Skills.Xfer( persist, "m_Skills" );

		// Generic states are used by spells for simple state indication like charm/freeze/etc...
		persist.XferMap( "m_GenericStateStorage", m_GenericStateStorage );

		// other
		persist.Xfer( "m_UnconsciousEndTime", m_UnconsciousEndTime );
		persist.Xfer( "m_bEnchantmentRemoved", m_bEnchantmentRemoved );
	}
	else
	{
		// skills - str, dex, int, melee, ranged, magic
		if ( persist.IsRestoring() )
		{
			FastFuelHandle hSkills = GetData()->GetInternalField( "skills" );
			if ( hSkills )
			{
				// create the default base skills defined by data read in by Rules
				m_Skills = gRules.GetBaseSkills();

				// init them from the fuel
				m_Skills.LoadSkills( hSkills );

				// set their owners
				SkillColl::iterator i, begin = m_Skills.begin(), end = m_Skills.end();
				for ( i = begin ; i != end ; ++i )
				{
					i->SetOwnerGo( GetGo() );
				}
			}
		}
	}

// Finish.

	return ( true );
}

bool GoActor :: XferCharacter( FuBi::PersistContext& persist )
{
	m_Skills.XferCharacter( persist, "skills" );
	if( gServer.GetLocalHumanPlayer()->GetId() == GetGo()->GetPlayerId() )
	{
		gVictory.XferCharacter( persist, "victory" );
	}
	persist.Xfer( "actor_class", m_Class );

	return ( true );
}

void GoActor :: XferForSync( FuBi::BitPacker& packer )
{
	// alignment
	packer.XferRaw( m_Alignment, FUBI_MAX_ENUM_BITS( AA_ ) );

	// can show health? (used for summoned creatures)
	packer.XferBit( m_bCanShowHealth );

	// generic states sync
	if ( packer.IsSaving() )
	{
		GSColl::iterator i, ibegin = m_GenericStateStorage.begin(), iend = m_GenericStateStorage.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			packer.WriteBit( true );
			gpstring str( i->first );
			packer.XferString( str );
			i->second.Xfer( packer );
		}

		// terminate
		packer.WriteBit( false );
	}
	else
	{
		gpstring str;
		GenericState gs;

		while ( packer.ReadBit() )
		{
			packer.XferString( str );
			gs.Xfer( packer );
			m_GenericStateStorage.insert( std::make_pair( str, gs ) );
		}
	}
}

void GoActor :: Update( float /*deltaTime*/ )
{
	UpdateGenericStates();
}

void GoActor :: UpdateSpecial( float /*deltaTime*/ )
{
	// only if we're out of the world frustum so no call these twice...
	if ( !GetGo()->IsUpdating() && !GetGo()->IsInActiveWorldFrustum() )
	{
		UpdateGenericStates();
	}
}

void GoActor :: ResetModifiers( void )
{
	m_Skills.ResetModifiers();
}

float GoActor :: GetUnconsciousDuration( void ) const
{
	return (float)( PreciseSubtract( max_t( 0.0, m_UnconsciousEndTime ), gWorldTime.GetTime() ) );
}

// make the character start the unconsious timer.
void GoActor :: RSResetUnconsciousDuration( float duration )
{
	FUBI_RPC_THIS_CALL( RSResetUnconsciousDuration, RPC_TO_SERVER );

	m_UnconsciousEndTime = PreciseAdd( duration, gWorldTime.GetTime() );

	RCSetUnconsiousEndTime( m_UnconsciousEndTime );
}

void GoActor :: RCSetUnconsiousEndTime( double endTime )
{
	FUBI_RPC_THIS_CALL( RCSetUnconsiousEndTime, RPC_TO_ALL );

	m_UnconsciousEndTime = endTime;
}

//
// sets values for multiplayer scaling, this was originally in CommitCreateOrImport
// but needed to be moved to the primary tread after the go was placed in the 
// world, so we can get frustum information for the object.
//
void GoActor :: ScaleForMultiplayer( bool useAllPlayers )
{
	// this can only be called from the primary thread due to the frustum query!
	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( ::IsMultiPlayer() );
	gpassert( GetGo()->GetPlayer()->IsComputerPlayer() );
	gpassert( !GetGo()->IsCloneSourceGo() );

#if !GP_RETAIL
	if ( !gGoDb.IsEditMode() )
	{
#endif // !GP_RETAIL

		if( !m_bMPScaled )
		{
			// this is for monsters!
			// clone sources should not get scaled (otherwise spawned instances get scaled twice!)
			if ( ::IsMultiPlayer() && GetGo()->GetPlayer()->IsComputerPlayer() && !GetGo()->IsCloneSourceGo() )
			{
				m_bMPScaled = true;
				GoAspect *pAspect = GetGo()->QueryAspect();
				if( pAspect )
				{
					int player_count = 0;

					// $$$ const int player_count = GetGo()->GetMpPlayerCount();
					// $$$ temp just to see how we like it!!!!
					// $$$ jake is going to play around with this
					// $$$ to determine what he wants!
					if( useAllPlayers == false )
					{
						if( !gConfig.GetBool( "region_scaling" ) )
						{
							// do the frustum first.
							unsigned int scaledObjectFrustum = gSiegeEngine.GetNodeFrustumOwnedBitfield( GetGo()->GetPlacement()->GetPosition().node );
							unsigned int scaledObjectPlayerIntersection = 0;

							// first loop through all the player an see who has the scaledObject in their frustum.
							for( Server::PlayerColl::iterator playerIter = gServer.GetPlayersBegin(); playerIter < gServer.GetPlayersEnd(); ++playerIter )
							{
								if( (*playerIter)->AddedPlayerToWorld() )
								{
									GoHandle hPlayerGo( (*playerIter)->GetHero() );
									if( hPlayerGo && hPlayerGo.IsValid() )
									{
										if( (unsigned int) hPlayerGo->GetWorldFrustumMembership() & scaledObjectFrustum )
										{
											scaledObjectPlayerIntersection |= (unsigned int) hPlayerGo->GetWorldFrustumMembership();	
										}
									}
								}
							}
							// now we need to count the of players in the scaledObjectPlayerIntersection!
							for( int i = 0; scaledObjectPlayerIntersection != 0; i++ )
							{
								// if we have the bit set, then clear it and increment.
								if( scaledObjectPlayerIntersection & (1 << i) )
								{
									scaledObjectPlayerIntersection &= ~(1 << i);
									player_count++;
								}
							}
						}
						else
						{
							// now for the region calc.  only do this for things with a mind!
							// and if the world is all the way loaded!
							if( GetGo()->HasMind() && gWorldMap.IsInitialized() )
							{
								RegionId desiredRegion = gWorldMap.GetRegionByNode( GetGo()->GetPlacement()->GetPosition().node );
								for( Server::PlayerColl::iterator playerIter = gServer.GetPlayersBegin(); playerIter < gServer.GetPlayersEnd(); ++playerIter )
								{
									if( (*playerIter)->AddedPlayerToWorld() )
									{
										GoHandle hPlayer( (*playerIter)->GetHero() );
										if( hPlayer && hPlayer.IsValid() && gWorldMap.GetRegionByNode( hPlayer->GetPlacement()->GetPosition().node ) == desiredRegion )
										{
											player_count++;
										}
									}
								}
							}	
						}
					}

					// if for some reason we didnt' find anyone
					// set it to the default!
					if( player_count == 0 )
					{
						player_count = GetGo()->GetMpPlayerCount();
					}

					// Get difficulty scales
					const float new_life = pAspect->GetMaxLife() * gRules.GetDifficultyScaleLife( player_count );

					pAspect->SetNaturalMaxLife( new_life );
					pAspect->SetMaxLife( new_life );
					pAspect->SetCurrentLife( new_life );

					const float new_ep_val = pAspect->GetExperienceValue() * gRules.GetDifficultyScaleEP( player_count );

					pAspect->SetExperienceValue( new_ep_val );
					pAspect->SetExperienceRemaining( new_ep_val );
				}
			}
		}
#if !GP_RETAIL
	}
#endif // !GP_RETAIL
}

bool GoActor :: CommitCreateOrImport( bool importFullXfer )
{
#if !GP_RETAIL
	if ( !gGoDb.IsEditMode() )
	{
#endif // !GP_RETAIL
		// this is for players! -- note the monster mp scaling was moved to enterworld 
		// so the monster would be assigned a frustum, and we would be on the primary thread!
		if ( GetCanLevelUp() )
		{
			gpassert( GetGo()->HasAspect() );

			GoAspect *pAspect = GetGo()->GetAspect();

			// first time - actor must set initial stats
			pAspect->SetNaturalMaxMana( gRules.CalculateMaxMana( GetGo() ) );
			pAspect->SetNaturalMaxLife( gRules.CalculateMaxLife( GetGo() ) );

			// force updates so natural values are copied to modified values
			ResetModifiers();
			pAspect->ResetModifiers();

			// start out fully charged if we're not importing a full xfer
			if ( !importFullXfer )
			{
				pAspect->SetCurrentMana( pAspect->GetMaxMana() );
				pAspect->SetCurrentLife( pAspect->GetMaxLife() );
			}
		}	
#if !GP_RETAIL
	}
#endif // !GP_RETAIL

	return ( true );
}

void GoActor :: UpdateGenericStates( void )
{
	double time = gWorldTime.GetTime();

	// Decrement any Generic state timeouts
	for ( GSColl::iterator iGS = m_GenericStateStorage.begin() ; iGS != m_GenericStateStorage.end() ; )
	{
		GenericState& gs = iGS->second;

		if ( PreciseSubtract(gs.m_absTimeout, time) < -30.0 )
		{
			// Get rid of stale states
			iGS = m_GenericStateStorage.erase( iGS );
		}
		else
		{
			++iGS;
		}
	}
}

DWORD GoActor :: GoActorToNet( GoActor* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoActor* GoActor :: NetToGoActor( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetActor() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
