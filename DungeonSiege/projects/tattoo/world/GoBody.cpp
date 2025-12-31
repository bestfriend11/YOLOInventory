//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoBody.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoBody.h"

#include "Choreographer.h"
#include "ContentDb.h"
#include "FuBiPersist.h"
#include "FuBiSchema.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoFollower.h"
#include "GoSupport.h"
#include "Job.h"
#include "MCP.h"
#include "MCP_Request.h"
#include "NamingKey.h"
#include "Nema_Aspect.h"
#include "Nema_AspectMgr.h"
#include "Nema_Blender.h"
#include "RapiOwner.h"
#include "ReportSys.h"
#include "Rules.h"
#include "Siege_Engine.h"
#include "Siege_Node.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "WorldMessage.h"

using namespace siege;

DECLARE_GO_COMPONENT( GoBody, "body" );

//////////////////////////////////////////////////////////////////////////////
AnimReq& MakeAnimReq( eAnimChore chore )
{
	static AnimReq s_AnimReq;
	s_AnimReq = AnimReq( chore, 0, AS_DEFAULT, 0, 0 );
	return ( s_AnimReq );
}

//////////////////////////////////////////////////////////////////////////////
// class AnimReq implementation

AnimReq :: AnimReq( void )
{
	Init();

	m_ReqChore     = CHORE_NONE;
	m_ReqBlock     = 0;
	m_ReqStance    = AS_DEFAULT;
	m_ReqSubAnim   = 0;
	m_ReqFlags     = 0;
}

AnimReq :: AnimReq( eAnimChore chore, DWORD rblock, eAnimStance stance, int subanim , DWORD flags )
{
	m_ReqChore     = chore;
	m_ReqBlock     = rblock;
	m_ReqStance    = stance;
	m_ReqSubAnim   = subanim;
	m_ReqFlags     = flags;
}


void AnimReq :: Init( void )
{
	ZeroObject( *this );
}



//////////////////////////////////////////////////////////////////////////////
// class GoBody implementation

GoBody :: GoBody( Go* parent )
	: Inherited( parent )
{
	m_AngularTurningVelocity     = 360.0f;
	m_CanTurnAndMove             = true;

	m_MinMoveVelocity            = 1.0f;
	m_AvgMoveVelocity            = 1.0f;
	m_MaxMoveVelocity            = 1.0f;

	m_TerrainMovementPermissions = siege::LF_NONE;

}

GoBody :: GoBody( const GoBody& source, Go* parent )
	: Inherited( source, parent )
{
	m_AnimChoreDictionary        = source.m_AnimChoreDictionary;
	m_AngularTurningVelocity     = source.m_AngularTurningVelocity;
	m_CanTurnAndMove             = source.m_CanTurnAndMove;

	m_BoneTranslator             = source.m_BoneTranslator;

	m_MinMoveVelocity            = source.m_MinMoveVelocity;
	m_AvgMoveVelocity            = source.m_AvgMoveVelocity;
	m_MaxMoveVelocity            = source.m_MaxMoveVelocity;

	m_TerrainMovementPermissions = source.m_TerrainMovementPermissions;
}

GoBody :: ~GoBody( void )
{
	// this space intentionally left blank...
}

const gpstring& GoBody :: GetArmorVersion( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "armor_version" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

//////////////////////////////////////////////////////////////////////////////
// persistence support

bool GoBody :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "angular_turning_velocity",		m_AngularTurningVelocity		);
	persist.Xfer( "can_turn_and_move",				m_CanTurnAndMove				);
	persist.Xfer( "min_move_velocity",				m_MinMoveVelocity				);
	persist.Xfer( "avg_move_velocity",				m_AvgMoveVelocity				);
	persist.Xfer( "max_move_velocity",				m_MaxMoveVelocity				);

	if( persist.IsSaving() && !persist.IsFullXfer() )
	{
		enum siege::eLogicalNodeFlags tempPermissions = m_TerrainMovementPermissions & NOT( LF_PLAYER_MASK );
		persist.Xfer( "terrain_movement_permissions", tempPermissions );
	}
	else
	{
		persist.Xfer( "terrain_movement_permissions", m_TerrainMovementPermissions );
	}

	return ( true );
}

bool GoBody :: XferCharacter( FuBi::PersistContext& persist )
{
	UNREFERENCED_PARAMETER(persist);
	return ( true );
}

void GoBody :: RCAnimate( const AnimReq& animReq )
{
	FUBI_RPC_THIS_CALL( RCAnimate, RPC_TO_ALL );

	// issue animate call
	Animate( animReq );
}

void GoBody :: SAnimate( const AnimReq& animReq )
{
	if ( !GetGo()->IsGlobalGo() || ::IsServerRemote() )
	{
		Animate( animReq );
	}
	else
	{
		// tell clients to start animating $$$ what about m_LocalOnly?
		RCAnimate( animReq );
	}

	if( ::IsServerLocal() )
	{
		// Update the MCP when we have a change in the gobody
		gMCP.UpdateGo(GetGoid());
	}

}

void GoBody :: Animate( const AnimReq& animReq )
{

	// remember notification state

	// set the stance if default
	eAnimStance stance = animReq.m_ReqStance;
	if ( stance == AS_DEFAULT )
	{
		// current equipment tells us which stance to use
		GoInventory* inv = GetGo()->QueryInventory();
		if ( inv != NULL )
		{
			stance = inv->GetAnimStance();
		}

		// may still be default
		if ( stance == AS_DEFAULT )
		{
			stance = AS_PLAIN;
		}
	}

	// get aspect
	nema::HAspect aspect = GetGo()->GetAspect()->GetAspectHandle();

	aspect->SetCurrReqBlock(animReq.m_ReqBlock);

	// attempt to set the chore
	if ( aspect->SetNextChore( animReq.m_ReqChore,
							   stance,
							   animReq.m_ReqSubAnim,
							   animReq.m_ReqFlags
							   ) )
	{
	}
	else
	{
		// $ may want to call notify and send it a failure event...
		gperrorf(( "GoBody::Animate: failed to make an anim controller for Go '%s', chore = '%s'\n",
				   GetGo()->GetTemplateName(), ::ToString( animReq.m_ReqChore ) ));

	}
}

bool GoBody :: IsPositionWalkable( SiegePos& position )
{
	eLogicalNodeFlags perms = GetTerrainMovementPermissions();
 	if( gSiegeEngine.AdjustPointToTerrain(position,perms) )
 	{
 		return gSiegeEngine.IsPositionAllowed( position, perms, 1.0f );
 	}
 	return false;
}



bool GoBody :: InstantiateDeformableArmor( Go* armor, const gpstring& ChestSlotArmorType )
{
	// cache some vars
	eEquipSlot slot = armor->GetGui()->GetEquipSlot();
	const char* armorSlot = ::GetArmorString( slot );
	const char* armorType = armor->GetDefend()->GetArmorType();
	const char* armorStyle = armor->GetDefend()->GetArmorStyle();

	// make sure we have valid armor version
	const char* armorVersion = GetArmorVersion();
	if ( *armorVersion == '\0' )
	{
		// $$$ MORE INFO...
		gperror( "Armor version is required to instantiate deformable armor!\n" );
		return ( false );
	}

	// build the new content name
	gpstring contentName;

	if ( (slot == ES_CHEST) )
	{
		contentName.assignf( "m_c_%s_pos_%s", armorVersion, armorType );
	}
	else if (slot == ES_FEET)
	{

		gpstring armorSubType;

		if (ChestSlotArmorType.size() == 0)
		{
			armorSubType = gContentDb.GetBootSubTypeForArmorBodyType("a1");
		}
		else
		{
			armorSubType = gContentDb.GetBootSubTypeForArmorBodyType(ChestSlotArmorType);
		}

		if (armorSubType.size() == 0)
		{
			gperrorf( ("Invalid Boot SubType for [%s]!\n",ChestSlotArmorType.c_str()) );
			armorSubType = "a";
		}

		contentName.assignf( "m_c_%s_%s_%s_%s", armorVersion, armorSlot, armorType, armorSubType.c_str() );

	}
	else if (slot == ES_FOREARMS)
	{

		gpstring armorSubType;

		if (ChestSlotArmorType.size() == 0)
		{
			armorSubType = gContentDb.GetGauntletSubTypeForArmorBodyType("a1");
		}
		else
		{
			armorSubType = gContentDb.GetGauntletSubTypeForArmorBodyType(ChestSlotArmorType);
		}

		if (armorSubType.size() == 0)
		{
			gperrorf( ("Invalid Gauntlet SubType for [%s]!\n",ChestSlotArmorType.c_str()) );
			armorSubType = "a";
		}

		contentName.assignf( "m_c_%s_%s_%s_%s", armorVersion, armorSlot, armorType, armorSubType.c_str() );


	}
	else
	{
		contentName.assignf( "m_c_%s_%s_%s", armorVersion, armorSlot, armorType );
	}

// Get replacement armor aspect.

	// get some vars
	bool badArmorAspect = false;
	nema::HAspect newAspect;

	// find the aspect or use default
	gpstring contentFile;
	if ( gNamingKey.BuildASPLocation( contentName, contentFile ) )
	{
		newAspect = gAspectStorage.LoadAspect( contentFile, contentName );
		if ( !newAspect )
		{
			// damn, warn
			gperrorf(( "Armor aspect file missing/invalid: '%s' ('%s')\n",
					   contentName.c_str(), contentFile.c_str() ));
			badArmorAspect = true;

			// replace with the very basic a1 type armor
			if ( (slot == ES_CHEST) )
			{
				contentName.assignf( "m_c_%s_pos_a1" , GetArmorVersion().c_str() );
			}
			else if ((slot == ES_FEET) || (slot == ES_FOREARMS))
			{
				contentName.assignf( "m_c_%s_%s_type1_a", GetArmorVersion().c_str(), armorSlot );
			}
			else
			{
				contentName.assignf( "m_a_%s_%s_type1", GetArmorVersion().c_str(), armorSlot );
			}

			// try again
			if ( gNamingKey.BuildASPLocation( contentName, contentFile ) )
			{
				newAspect = gAspectStorage.LoadAspect( contentFile, contentName );
			}
		}
	}
	else
	{
		gperrorf(( "Cannot parse aspect name: '%s'\n", contentName.c_str() ));
	}

// Set its textures.

	if ( newAspect )
	{
		if ( newAspect->GetNumSubTextures() == 1 )
		{
			if ( !badArmorAspect )
			{
				if ( (slot == ES_CHEST) )
				{
					contentName.assignf( "b_c_pos_%s_%s", armorType, armorStyle );
				}
				else
				{
					contentName.assignf( "b_a_%s_%s", armorSlot, armorStyle );
				}
			}
			else
			{
				contentName = "b_a_err-badtype";
			}

			if ( gNamingKey.BuildIMGLocation( contentName, contentFile ) )
			{
				newAspect->SetTextureFromFile( 0, contentFile );
			}
			else
			{
				gperrorf(( "Cannot parse bitmap name: '%s'\n", contentName.c_str() ));
			}
		}
		else
		{
			for ( int i = 0, count = newAspect->GetNumSubTextures() ; i != count ; ++i )
			{
				if ( i == 0 && count > 1)
				{
					// first flesh texture of aspect that will wear this item
					newAspect->CopyTexture(0,*GetGo()->GetAspect()->GetAspectHandle(),0);
					continue;

				}
				else if ( i == 0 || i == 1)
				{
					if ( (slot == ES_CHEST) )
					{
						contentName.assignf( "b_c_pos_%s_%s", armorType, armorStyle );
					}
					else
					{
						contentName.assignf( "b_a_%s_%s", armorSlot, armorStyle );
					}
				}
				else
				{
					contentName = newAspect->GetDefaultTextureString( i );
				}

				if ( gNamingKey.BuildIMGLocation( contentName, contentFile ) )
				{
					if (FileSys::DoesResourceFileExist( contentFile ) )
					{
						newAspect->SetTextureFromFile( i, contentFile );
					}
					else if( gNamingKey.BuildIMGLocation( "b_a_err-badstyle", contentFile ) )
					{
						newAspect->SetTextureFromFile( i, contentFile );
					}
				}
				else
				{
					gperrorf(( "Cannot parse bitmap name: '%s'\n", contentName.c_str() ));
				}
			}
		}

		// take the recomposed armor
		armor->GetAspect()->SetCurrentAspect( newAspect );
	}

	return ( !newAspect.IsNull() );
}

bool GoBody :: InstantiateDeformableUnwornArmor( Go* armor )
{
	// cache some vars
	eEquipSlot slot = armor->GetGui()->GetEquipSlot();
	const char* armorSlot = ::GetArmorString( slot );
	const char* armorType = armor->GetDefend()->GetArmorType();
	const char* armorStyle = armor->GetDefend()->GetArmorStyle();

	// build the new content name
	gpstring contentName;
	contentName.assignf( "m_a_%s_%s", armorSlot, armorType );

// Get replacement armor aspect.

	// get some vars
	bool badArmorAspect = false;
	nema::HAspect newAspect;

	// find the aspect or use default
	gpstring contentFile;
	if ( gNamingKey.BuildASPLocation( contentName, contentFile ) )
	{
		newAspect = gAspectStorage.LoadAspect( contentFile, contentName );
		if ( !newAspect )
		{
			// damn, warn
			gperrorf(( "Armor aspect file missing/invalid: '%s' ('%s')\n",
					   contentName.c_str(), contentFile.c_str() ));
			badArmorAspect = true;

			// replace with the very basic a1 type armor
			contentName.assignf( "m_a_%s_type1", armorSlot );

			// try again
			if ( gNamingKey.BuildASPLocation( contentName, contentFile ) )
			{
				newAspect = gAspectStorage.LoadAspect( contentFile, contentName );
			}
		}
	}
	else
	{
		gperrorf(( "Cannot parse aspect name: '%s'\n", contentName.c_str() ));
	}

// Set its textures.

	if ( newAspect )
	{
		gpstring texName;
		for ( int i = 0, count = newAspect->GetNumSubTextures() ; i != count ; ++i )
		{
			// get the texture name
			if ( (slot == ES_CHEST) )
			{
				texName.assignf( "b_c_pos_%s_%s", armorType, armorStyle );
			}
			else
			{
				texName.assignf( "b_a_%s_%s", armorSlot, armorStyle );
			}

			// resolve to a file
			gpstring texFile;
			if (   !gNamingKey.BuildIMGLocation( texName, texFile )
				|| !FileSys::DoesResourceFileExist( texFile ) )
			{
#				if ( !GP_RETAIL )
				{
					gpwarningf(( "Placeholder texture substitution: illegal texture '%s' for aspect '%s' in Go at '%s'\n",
								 texName.c_str(), gAspectStorage.GetAspectName( newAspect ).c_str(),
								 armor->DevGetFuelAddress().c_str() ));
				}
#				endif // !GP_RETAIL

				texName = gContentDb.GetDefaultTextureName();
				if (   !gNamingKey.BuildIMGLocation( texName, texFile )
					|| !FileSys::DoesResourceFileExist( texFile ) )
				{
					gperrorf(( "Can't find placeholder texture '%s'! This is serious!\n", texName.c_str() ));
					texFile.clear();
				}
			}

			// set texture
			newAspect->SetTextureFromFile( i, texFile );
		}

		// take the recomposed armor
		armor->GetAspect()->SetCurrentAspectSimple( newAspect );
	}

	return ( !newAspect.IsNull() );
}

GoComponent* GoBody :: Clone( Go* newParent )
{
	return ( new GoBody( *this, newParent ) );
}

bool GoBody :: CommitCreation( void )
{
	return ( CommitLoadOrCreate( false ) );
}

bool GoBody :: CommitLoad( void )
{
	return ( CommitLoadOrCreate( true ) );
}

void GoBody :: Shutdown( void )
{
	if ( GetGo()->HasAspect() && GetGo()->GetAspect()->HasAspectHandle() )
	{
		// clear any animation callbacks we may have pending
		GetGo()->GetAspect()->GetAspectHandle()->ResetAnimationCompletionCallback();
	}
}

void GoBody :: ResetModifiers( void )
{
	m_AvgMoveVelocity.Reset();
}

void GoBody :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	switch ( msg.GetEvent() )
	{
		case ( WE_ENTERED_WORLD ):
		{

			// Are we already running some chore? If we are then don't bother trying to
			// start the initial chore

			GoAspect* aspect = GetGo()->GetAspect();
			nema::HAspect haspect = aspect->GetAspectHandle();

			eAnimChore initialChore = haspect->GetNextChore();

			if ( initialChore == CHORE_NONE )
			{

				GetData()->GetRecord()->Get( "initial_chore", initialChore ) ;

				if (initialChore != CHORE_NONE)
				{
					// At this point we don't know whats in the inventory so we
					// Need to determine the lowest Stance value that is actually allowed on this aspect
					// $ this will automatically use the current stance as determined by
					//   equipped items in the inventory.
					if (IsServerLocal() && GetGo()->HasFollower() && !GetGo()->IsOmni())
					{
						gMCP.MakeRequest(GetGoid(),MCP::PL_PLAYANIM, initialChore , 0 , 0);
					}
					else
					{
						// This is either a 'local' object that animates WITHOUT the MCP
						// or a Go on a client that is in the process of being loaded
						GoAspect* aspect = GetGo()->GetAspect();
						nema::HAspect haspect = aspect->GetAspectHandle();
						haspect->SetNextChore(initialChore,AS_DEFAULT);
						GetGo()->SetBucketDirty();
					}
				}
			}
		}
		break;

		case ( WE_CONSTRUCTED ):
		{
			// no chores? no updates necessary!
			bool wantsUpdates = m_AnimChoreDictionary != NULL;

#			if !GP_RETAIL
			// siege editor always wants updates because it can change the triggers
			// but it won't tell the go to start updating
			if ( ::IsWorldEditMode() )
			{
				wantsUpdates = true;
			}
#			endif // !GP_RETAIL

			SetWantsUpdates( wantsUpdates );
		}
		break;
	}
}

void GoBody :: Update( float deltaTime )
{
	GPSTATS_GROUP( GROUP_ANIMATE_MODELS );

	// there is no point wasting time animating objects that (a) can't affect
	// gameplay and (b) aren't being rendered anyway.
	if ( GetGo()->IsLodfi() && !GetGo()->WasInViewFrustumRecently() )
	{
		return;
	}

	// only update the root object
	GoAspect* aspect = GetGo()->GetAspect();
	if ( aspect->IsNemaRoot()  )
	{
		nema::Aspect* paspect = aspect->GetAspectPtr();

		// if we're currently fidgeting and are going to continue fidgeting...
		if ( (paspect->GetCurrentChore() == CHORE_FIDGET) && (paspect->GetNextChore() == CHORE_FIDGET) )
		{
			// if we're not even in a rendered node, skip
			if ( !GetGo()->WasInViewFrustumRecently() )
			{
				return;
			}

			// if we're not visible, skip too
			if ( !aspect->GetIsVisible() )
			{
				return;
			}
		}
		
		GPPROFILERSAMPLE( "GoBody::Update - Aspect::UpdateAnimationStateMachine", SP_DRAW | SP_DRAW_CALC_GO );

		// $$$ apply spell/anim speed biases!!!

		if ( paspect->UpdateAnimationStateMachine( deltaTime ) )
		{
			GetGo()->SetWobbDirty();
		}

	}

	// $$	make sure dynamic walk permissions are set.  I'd rather do this on the WE_PLAYER_CHANGED, but it won't
	//		work without some careful changes to GoComponent initialization orders -bk
	if( GetGo()->GetPlayer()->GetController() == PC_HUMAN )
	{
		m_TerrainMovementPermissions |= siege::LF_HUMAN_PLAYER;
		m_TerrainMovementPermissions &= NOT( siege::LF_COMPUTER_PLAYER );
	}
	else if( GetGo()->GetPlayer()->GetController() == PC_COMPUTER )
	{
		m_TerrainMovementPermissions |= siege::LF_COMPUTER_PLAYER;
		m_TerrainMovementPermissions &= NOT( siege::LF_HUMAN_PLAYER );
	}
}

bool GoBody :: CommitLoadOrCreate( bool load )
{
	bool success = true;

	// get var
	nema::HAspect aspect = GetGo()->GetAspect()->GetAspectHandle();

	// only set aspect up fully if we're not a clone source
	if ( !GetGo()->IsCloneSourceGo() )
	{
		// set chore dictionary - this is internal (but not required)
		FastFuelHandle choreFuel = GetData()->GetInternalField( "chore_dictionary" );
		if ( choreFuel )
		{
			m_AnimChoreDictionary = gContentDb.AddOrFindChoreDictionary( choreFuel );
			if ( !load )
			{
				// only add chores if we are not loading because they will have
				// already been persisted in the aspect
				if ( !m_AnimChoreDictionary.IsNull() )
				{
					ChoreDictionary::const_iterator i, begin = m_AnimChoreDictionary->begin(), end = m_AnimChoreDictionary->end();
					for ( i = begin ; i != end ; ++i )
					{
						aspect->AddChore( &i->second, GetGo()->ShouldPersist() );
					}
				}
				else
				{
					gperror( "Chore dictionary construction failed, cannot construct GoBody component.\n" );
					success = false;
				}
			}
			else
			{
				if ( !m_AnimChoreDictionary.IsNull() )
				{
					// Fix up the blender, now that we finally have the chore dictionary -- biddle
					aspect->GetBlender()->XferPost(*m_AnimChoreDictionary);
				}
				else
				{
					gperror( "Chore dictionary reload failed, cannot reload blender, cannot construct GoBody component.\n" );
					success = false;
				}
			}

		}
	}

	// set bone translator - this is internal (and required)
	m_BoneTranslator = gContentDb.AddOrFindBoneTranslator( GetData()->GetInternalField( "bone_translator" ), aspect );
	if ( !m_BoneTranslator )
	{
		// use the empty one
		m_BoneTranslator = gContentDb.AddOrFindNullBoneTranslator();
	}

	return ( success );
}

#if !GP_RETAIL

void GoBody :: Dump( bool /*verbose*/, ReportSys::Context * ctx )
{
	ReportSys::AutoReport autoReport( ctx );
}

#endif // !GP_RETAIL

DWORD GoBody :: GoBodyToNet( GoBody* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoBody* GoBody :: NetToGoBody( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetBody() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
