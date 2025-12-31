//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoParty.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "Precomp_World.h"
#include "GoParty.h"

#include "AppModule.h"
#include "CameraAgent.h"
#include "FileSysUtils.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "Job.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoInventory.h"
#include "GoMind.h"
#include "GoSupport.h"
#include "world.h"
#include "aiquery.h"
#include "player.h"
#include "stdhelp.h"
#include "server.h"
#include "Siege_Engine.h"
#include "siege_light_database.h"
#include "siege_hotpoint_database.h"
#include "siege_decal.h"
#include "siege_decal_database.h"
#include "siege_node.h"
#include "siege_camera.h"
#include "trigger_sys.h"
#include "WorldTime.h"

#include <algorithm>


DECLARE_GO_COMPONENT( GoParty, "party" );


const int FORMATION_HALF_WIDTH	= 4;
const int FORMATION_HALF_LENGTH	= 4;


FUBI_DECLARE_XFER_TRAITS( GoParty* )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		Goid goid = GOID_INVALID;
		if ( persist.IsSaving() && (obj != NULL) )
		{
			goid = obj->GetGoid();
		}

		bool rc = persist.Xfer( xfer, key, goid );

		if ( persist.IsRestoring() )
		{
			GoHandle go( goid );
			obj = go ? go->QueryParty() : NULL;
		}

		return ( rc );
	}
};

/*
FUBI_DECLARE_XFER_TRAITS( FormSpotColl )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		return ( obj.Xfer( persist, key ) );
	}
};
*/


//////////////////////////////////////////////////////////////////////////////
// class GoParty implementation

GoParty :: GoParty( Go* go )
	: Inherited( go )
{
	gpassert( go );
	m_HotRadius                   = 0.0f;                 
	m_InnerLightRadius            = 0.0f;                 
	m_OuterLightRadius            = 0.0f;                 
	m_LightVerticalOffset         = 0.0f;                 
	m_LightColor                  = 0;                    
	m_lightId                     = siege::UNDEFINED_GUID;
	m_NextUpdateTime              = 0.0;                
	m_MaxEquppedRangedWeaponRange = 0;                    
	m_NumRangedFighters           = 0;                    
	m_CurrentFocus				  = GOID_INVALID;
	m_bUpdateShadows			  = false;

	//----- formation
	m_pFormation = new Formation( this );
	gpassert( m_pFormation );

	GetGo()->GetPlayer()->AddParty( GetGo() );
}


GoParty :: GoParty( const GoParty & source, Go* parent )
	: Inherited( source, parent )
{
	m_HotRadius                   = source.m_HotRadius;                  
	m_InnerLightRadius            = source.m_InnerLightRadius;           
	m_OuterLightRadius            = source.m_OuterLightRadius;           
	m_LightVerticalOffset         = source.m_LightVerticalOffset;        
	m_LightColor                  = source.m_LightColor;                 
	m_lightId                     = siege::UNDEFINED_GUID;               
	m_NextUpdateTime              = source.m_NextUpdateTime;             
	m_MaxEquppedRangedWeaponRange = source.m_MaxEquppedRangedWeaponRange;
	m_NumRangedFighters           = source.m_NumRangedFighters;          
	m_CurrentFocus				  = source.m_CurrentFocus;
	m_bUpdateShadows			  = false;

	m_pFormation 		  = new Formation( this );

	GetGo()->GetPlayer()->AddParty( GetGo() );
}


GoParty :: ~GoParty( void )
{
	Delete( m_pFormation );
	GetGo()->GetPlayer()->RemoveParty( GetGo() );
}


void GoParty :: TakeFrustum( FrustumId frustumId )
{
	gGoDb.SAssignFrustumToGo( frustumId, GetGoid() );
}


void GoParty :: AddMemberDeferred( Scid UniqueScid )
{
	m_WaitingToJoin.push_back( UniqueScid );
}


void GoParty :: GetMembersAccordingToRank( GopColl & out )
{
	std::map< DWORD, Go * > temp;
	GopColl packers;
	out.clear();

	for( GopColl::iterator i = GetGo()->GetChildBegin(); i != GetGo()->GetChildEnd(); ++i )
	{
		if( (*i)->IsInActiveWorldFrustum() && (*i)->HasMind() )
		{
			if( (*i)->GetMind()->GetRank() == 0xffffffff )		// $$ I don't much like this as a constant -bk
			{
				continue;
			}
			else
			{
				if( (*i)->GetInventory()->IsPackOnly() )
				{
					packers.push_back( *i );
				}
				else
				{
					temp.insert( std::make_pair( (*i)->GetMind()->GetRank(), (*i) ) );
				}
			}
		}
	}

	for( std::map< DWORD, Go * >::iterator j = temp.begin(); j != temp.end(); ++j )
	{
		out.push_back( (*j).second );
	}

	for ( i = packers.begin(); i != packers.end(); ++i )
	{
		out.push_back( *i );
	}
}


void GoParty :: RSAddMemberNow( Go* member )
{
	FUBI_RPC_THIS_CALL( RSAddMemberNow, RPC_TO_SERVER );

	RCAddMemberNow( member );

	if (member->IsAnyHumanPartyMember())
	{
		gTriggerSys.UpdateTriggersWhenGoJoinsParty( member );
	}
}


void GoParty :: RCAddMemberNow( Go* member )
{
	FUBI_RPC_THIS_CALL( RCAddMemberNow, RPC_TO_ALL );

	AddMemberNow( member );
}


void GoParty :: AddMemberNow( Go* member )
{
	CheckMemberToAdd( member );
	GetGo()->AddChild( member );
	if( GetGo() == gServer.GetScreenParty() )
	{
		m_bUpdateShadows	= true;
	}
}


void GoParty :: RSRemoveMemberNow( Go* member )
{
	FUBI_RPC_THIS_CALL( RSRemoveMemberNow, RPC_TO_SERVER );

	member->GetInventory()->SSetGold( 0 );

	if (member->IsAnyHumanPartyMember())
	{
		gTriggerSys.UpdateTriggersWhenGoLeavesParty( member );
	}

	RCRemoveMemberNow( member );
}


void GoParty :: RCRemoveMemberNow( Go* member )
{
	FUBI_RPC_THIS_CALL( RCRemoveMemberNow, RPC_TO_ALL );

	RemoveMemberNow( member );
}


void GoParty :: RemoveMemberNow( Go* member )
{
	GetGo()->RemoveChild( member );
	member->SetPlayer( gServer.GetComputerPlayer() );
	
	if( GetGo() == gServer.GetScreenParty() )
	{
		m_bUpdateShadows	= true;
	}
}


GoComponent* GoParty :: Clone( Go* newGo )
{
	return ( new GoParty( *this, newGo ) );
}


bool GoParty :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "hot_radius", m_HotRadius );
	persist.Xfer( "light_iradius", m_InnerLightRadius );
	persist.Xfer( "light_oradius", m_OuterLightRadius );
	persist.Xfer( "light_vertoffset", m_LightVerticalOffset );
	persist.XferHex( "light_color", m_LightColor );

	if ( persist.IsFullXfer() )
	{
		persist.EnterBlock( "m_pFormation" );
		m_pFormation->Xfer( persist );
		persist.LeaveBlock();

		persist.XferVector( "m_WaitingToJoin", m_WaitingToJoin );
		persist.Xfer( "m_NextUpdateTime", m_NextUpdateTime );
	}
	else
	{
		if( persist.IsRestoring() )
		{
			FastFuelHandle members = GetData()->GetInternalField( "add_members" );

			if( members )
			{
				FastFuelFindHandle fh( members );
				fh.FindFirstKeyAndValue();

				gpstring sIgnoredKey;
				gpstring sMemberScid;

				while( fh.GetNextKeyAndValue( sIgnoredKey, sMemberScid ) )
				{
					unsigned int scidInt;
					stringtool::Get( sMemberScid, scidInt );
					AddMemberDeferred( MakeScid( scidInt ) );
				}
			}
		}
	}
	return ( true );
}


void GoParty :: HandleMessage( const WorldMessage& msg )
{
	// handle message
	if ( msg.HasEvent( WE_PLAYER_CHANGED ) )
	{
		// remove from old
		((Player*)msg.GetData1())->RemoveParty( GetGo() );

		// add to new
		GetGo()->GetPlayer()->AddParty( GetGo() );
	}
	else if ( msg.HasEvent( WE_DESTRUCTED ) )
	{
		if ( GetGo()->HasFrustum() )
		{
			// release my frustum but don't delete it
			gGoDb.ReleaseFrustumFromGo( GetGoid(), false );
		}
	}
	else if ( msg.HasEvent( WE_LEFT_WORLD ) && (gServer.GetScreenParty() == GetGo()) )
	{
		PrivateCheckInWorld();
	}
}


void GoParty :: HandleCcMessage( const WorldMessage& msg )
{
	if ( msg.HasEvent( WE_FOCUSED ) && (gServer.GetScreenParty() == GetGo()) )
	{
		PrivateUpdate( 0.0f, false );
	}
}


void GoParty :: UpdateSpecial( float /*deltaTime*/ )
{
	if ( !GetGo()->IsInAnyWorldFrustum() )
	{
		PrivateCheckInWorld();
	}
}


void GoParty :: LinkChild( Go* child )
{
	// only do frustum stuff if we're the server and a human player
	if ( IsServerLocal() && GetGo()->IsAnyHumanParty() )
	{
		gpassert( GetGo()->GetPlayer()->GetController() == PC_HUMAN );

		// first one?
		if ( GetGo()->GetChildren().size() == 1 )
		{
			// give my frustum to this child
			FrustumId frustumId = gGoDb.GetGoFrustum( GetGoid() );
			gpassert( frustumId != FRUSTUMID_INVALID );
			gGoDb.SReleaseFrustumFromGo( GetGoid(), false );
			gGoDb.SAssignFrustumToGo( frustumId, child->GetGoid() );

			// this must be the hero
			gpassert( GetGo()->GetPlayer()->GetHero() == GOID_INVALID );
			GetGo()->GetPlayer()->SetHero( child->GetGoid() );
		}
		else
		{
			// create a new frustum for this child
			gGoDb.SCreateFrustumForGo( child->GetGoid() );
		}
	}

	// Shadow update
	if( GetGo() == gServer.GetScreenParty() )
	{
		m_bUpdateShadows	= true;
	}
}


void GoParty :: UnlinkChild( Go* child )
{
	// only do frustum stuff if we're the server and a human player or we're 
	// a client and the screen player
	if ( (IsServerLocal() && GetGo()->IsAnyHumanParty()) || GetGo()->IsScreenParty() )
	{
		gpassert( GetGo()->GetPlayer()->GetController() == PC_HUMAN );

		// last one and not being destroyed?
		if ( GetGo()->GetChildren().empty() )
		{
			// take this child's frustum for my own
			FrustumId frustumId = gGoDb.GetGoFrustum( child->GetGoid() );
			if ( frustumId != FRUSTUMID_INVALID )
			{
				gGoDb.ReleaseFrustumFromGo( child->GetGoid(), false );
				gGoDb.AssignFrustumToGo( frustumId, GetGoid() );
			}
		}
		else
		{
			// destroy this child's frustum
			gGoDb.ReleaseFrustumFromGo( child->GetGoid(), true );
		}

		// deselect this dude
		child->Deselect();
	}

	// Shadow update
	if( GetGo() == gServer.GetScreenParty() )
	{
		m_bUpdateShadows	= true;
	}
}


#if !GP_RETAIL

void GoParty :: DrawDebugHud()
{
	SiegePos pos = GetGo()->GetPlacement()->GetPosition();

	gWorld.DrawDebugSphere( pos, m_HotRadius, COLOR_DARK_GREEN, "" );

	pos.pos.y += 2.0;
	gWorld.DrawDebugSphere( pos, 0.25, COLOR_WHITE, GetGo()->GetTemplateName() );

}


bool GoParty :: ShouldSaveInternals( void )
{
	return ( !m_WaitingToJoin.empty() || !GetGo()->GetChildren().empty() );
}


void GoParty :: SaveInternals( FuelHandle fuel )
{
	FuelHandle members = fuel->CreateChildBlock( "add_members" );
	
	// save guys that haven't joined yet
	{
		ScidColl::iterator i;
		for( i = m_WaitingToJoin.begin(); i != m_WaitingToJoin.end(); ++i )
		{
			members->Add( "*", MakeInt(*i), FVP_HEX_INT );
		}
	}

	// save children alreay joined
	{
		GopColl::iterator i;
		for( i = GetGo()->GetChildBegin(); i != GetGo()->GetChildEnd(); ++i )
		{
			Scid childScid = (*i)->GetScid();
			if( childScid != SCID_INVALID )
			{
				members->Add( "*", MakeInt(childScid), FVP_HEX_INT );
			}
		}
	}
}
#endif // !GP_RETAIL


float GoParty :: GetMaxEquppedRangedWeaponRange()
{
	if( m_MaxEquppedRangedWeaponRange == 0.0 )
	{
		GopColl::iterator i, ibegin = GetGo()->GetChildBegin(), iend = GetGo()->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Go * rangedWeapon = (*i)->GetInventory()->GetSelectedRangedWeapon();
			if( rangedWeapon )
			{
				m_MaxEquppedRangedWeaponRange = max( m_MaxEquppedRangedWeaponRange, rangedWeapon->GetAttack()->GetAttackRange() );
			}
		}
	}

	return m_MaxEquppedRangedWeaponRange;
}


#if GP_DEBUG

bool GoParty :: AssertValid( void ) const
{
	gpassert( m_pFormation != NULL );
	return ( true );
}

#endif // GP_DEBUG


void GoParty :: PrivateCheckInWorld( void )
{
	// i've left the world! chances are my focus go popped around
	GoHandle focusGo( gGoDb.GetFocusGo( GetGo()->GetPlayerId() ) );
	if ( focusGo && (focusGo.Get() != GetGo()) )
	{
		PrivateUpdate( 0.0f, false );
	}
}


void GoParty :: PrivateUpdate( float /*timeDelta*/, bool updateLight )
{
	////////////////////////////////////////
	//	clear temp vars

	if( !m_TempGopCollA.empty() )
	{
		m_TempGopCollA.clear();
	}
	if( !m_TempGopCollB.empty() )
	{
		m_TempGopCollB.clear();
	}
	if( !m_TempGopCollC.empty() )
	{
		m_TempGopCollC.clear();
	}
	if( !m_TempQtColl1.empty() )
	{
		m_TempQtColl1.clear();
	}

	///////////////////////////////////////////////
	//	spatial

	// update party position so it stays with the members

	if ( GetGo()->GetPlayer()->IsComputerPlayer() )
	{
		// use simple geometric mean
		GoidColl hotGroup;
		GopColl::iterator i, ibegin = GetGo()->GetChildBegin(), iend = GetGo()->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// in same frustum?
			if (  MakeInt( (*i)->GetWorldFrustumMembership() )
				& MakeInt( GetGo()->GetWorldFrustumMembership() ) )
			{
				// got placement?
				hotGroup.push_back( (*i)->GetGoid() );
			}
		}

		// do it
		if ( !hotGroup.empty() )
		{
			// Calculate geometric center in world space
			SiegePos siegePos;
			siegePos.pos	= gAIQuery.GetGeometricCenter( hotGroup );
			siegePos.node	= GoHandle( *hotGroup.begin() )->GetPlacement()->GetPosition().node;

			// Build our new position
			siegePos.FromWorldPos( siegePos.pos, siegePos.node );

			// Set the position to the calculated center
			GetGo()->GetPlacement()->ForceSetPosition( siegePos );
		}
	}
	else if ( GetGo()->GetPlayer()->IsLocalHumanPlayer() )
	{
		// use focus + hotgroup thing. note that focus go will only ever be on
		// a node that does not exist under very special circumstances (see
		// #12317 for more info).
		Goid newFocus	= gGoDb.GetFocusGo( GetGo()->GetPlayerId() );
		GoHandle focusGo( newFocus );
		GopColl hotGroup;
		if (   focusGo
			&& focusGo->HasPlacement()
			&& focusGo->IsInAnyWorldFrustum()
			&& gSiegeEngine.IsNodeValid( focusGo->GetPlacement()->GetPosition().node ) )
		{
			gAIQuery.GetOccupantsInSphere(
					focusGo->GetPlacement()->GetPosition(),
					m_HotRadius,
					focusGo,
					NULL,
					0,
					OF_PARTY_MEMBERS | OF_ALIVE,
					&hotGroup );
			hotGroup.push_back( focusGo );

			if ( gServer.GetScreenParty() == GetGo() )
			{
				gGoDb.ReplaceHotGroup( hotGroup );
			}

			GOVectorColl Offsets;

			// tack on an extra focus go so it's weighted a little more that way
			hotGroup.push_back( focusGo );

			vector_3 oldWorldPos	= GetGo()->GetPlacement()->GetWorldPosition();
			vector_3 newWorldPos	= gAIQuery.GetGeometricCenter( hotGroup, Offsets );
			vector_3 offset			= newWorldPos - oldWorldPos;
			float offsetLength		= offset.Length();

			Go* pCurrentFocus	= ::GetGo( m_CurrentFocus );
			if( pCurrentFocus && m_CurrentFocus != newFocus )
			{
				// Search for the old focus go in the new hotgroup
				for( GopColl::iterator h = hotGroup.begin(); h != hotGroup.end(); ++h )
				{
					if( (*h) == pCurrentFocus )
					{
						// Old focus found, reset current focus
						m_CurrentFocus	= newFocus;
						break;
					}
				}
			}

			if( m_CurrentFocus == newFocus )
			{
				if( !offset.IsZero() )
				{
					// Scale it, based on meters per second allowable speed difference.
					// note that we use sys delta time so that party updates the same
					// regardless of game speed (or pause state). party controls the
					// camera, which should update in real-time for user experience.
					offset *= min_t( 1.0f, (gAppModule.GetActualDeltaTime() * gWorldOptions.GetPartySpeed() ) / offsetLength );

					if( (newWorldPos - (oldWorldPos + offset)).Length() > gWorldOptions.GetPartyTether() )
					{
						offset	= (newWorldPos - oldWorldPos).Normalize_T() * (offsetLength - gWorldOptions.GetPartyTether() );
					}
				}
			}
			else
			{
				if((m_CurrentFocus == GOID_INVALID && gWorldState.GetCurrentState() != WS_SP_NIS) ||
				   (MakeInt( focusGo->GetWorldFrustumMembership() ) &
					MakeInt( GetGo()->GetWorldFrustumMembership() )) == 0 )
				{
					// Get current offset
					vector_3 camOffset	= gSiegeEngine.GetCamera().GetCameraPosition() - gSiegeEngine.GetCamera().GetTargetPosition();

					// Set the new camera position
					CameraPosition cameraPos;
					cameraPos.cameraPos.FromWorldPos( newWorldPos + camOffset, focusGo->GetPlacement()->GetPosition().node );
					cameraPos.targetPos.FromWorldPos( newWorldPos, focusGo->GetPlacement()->GetPosition().node );

					gCameraAgent.RequestTeleport( cameraPos );
				}
				else
				{
					// Panning but not teleporting.  Juice the history to make it smooooooth.
					gSiegeEngine.GetCamera().ClearHistory();
					gCameraAgent.ResetCameraAffectorModification();
					gCameraAgent.ResetTargetAffectorModification();
				}
			}

			// Make sure that our current focus is set properly
			m_CurrentFocus	= newFocus;

			// Calculate geometric center in world space
			SiegePos siegePos;
			siegePos.pos	= oldWorldPos + offset;
			siegePos.node	= focusGo->GetPlacement()->GetPosition().node;

			// Build our new position
			siegePos.FromWorldPos( siegePos.pos, siegePos.node );

			// Adjust position.  This is done in two steps to make sure that if there is valid floor, it will
			// always use that first, then fall back to any geometry if there is no floor to use.
			SiegePos adjustPos	= siegePos;
			if( gSiegeEngine.AdjustPointToTerrain( adjustPos, siege::LF_IS_ANY ) )
			{
				siegePos.FromWorldPos( oldWorldPos + offset, adjustPos.node );
			}

			// Set the position to the calculated center
			GetGo()->GetPlacement()->ForceSetPosition( siegePos );

			// Update the party light
			if ( updateLight )
			{
				siegePos.pos.y	+= m_LightVerticalOffset;
				if( m_lightId == siege::UNDEFINED_GUID ||
					!gSiegeEngine.LightDatabase().HasLightDescriptor( m_lightId ) )
				{
					// Create the appropriate light source
					m_lightId = gSiegeEngine.LightDatabase().InsertPointLightSource( siegePos, vector_3( 1.0f, 1.0f, 1.0f ), siege::UNDEFINED_GUID,
																					 false, true, m_InnerLightRadius, m_OuterLightRadius );
				}
				else
				{
					// Update the light position
					gSiegeEngine.LightDatabase().SetLightPosition( m_lightId, siegePos );
				}
			}
		}
	}
	else
	{
		// remote player's go - always set to exact location of focus go
		GoHandle focusGo( gGoDb.GetFocusGo( GetGo()->GetPlayerId() ) );
		if ( focusGo && focusGo->HasPlacement() )
		{
			// Set the position to the calculated center
			GetGo()->GetPlacement()->ForceSetPosition( focusGo->GetPlacement()->GetPosition() );
		}
	}

	// update hotpoint basis
	if( gServer.GetScreenParty() == GetGo() )
	{
		gSiegeHotpointDatabase.SetActiveHotpointNode( GetGo()->GetPlacement()->GetPosition().node );
	}

	////////////////////////////////////////
	//	formation

	m_pFormation->Update();

	////////////////////////////////////////
	//	try to suck more members into the party

	if( m_NextUpdateTime > gWorldTime.GetTime() )
	{
		return;
	}

	ScidColl::iterator i;

	for( i = m_WaitingToJoin.begin(); i != m_WaitingToJoin.end(); )
	{
		// Try to find the object that we are adding
		GoHandle pMember( gGoDb.FindGoidByScid( (*i) ) );
		if( pMember )
		{
			// Add them to the party
			CheckMemberToAdd( pMember );
			GetGo()->AddChild( pMember );
			i = m_WaitingToJoin.erase( i );
			m_bUpdateShadows	= true;
			continue;
		}

		// Go to the next party member
		++i;
	}

	if( m_bUpdateShadows && GetGo() == gServer.GetScreenParty() )
	{
		gGoDb.UpdateShadows();
		m_bUpdateShadows	= false;
	}

	if( !m_WaitingToJoin.empty() )
	{
		m_NextUpdateTime = PreciseAdd( gWorldTime.GetTime(), 2.0 );
	}

	////////////////////////////////////////
	//	stats

	{
		m_NumRangedFighters = 0;
		GopColl::iterator i, ibegin = GetGo()->GetChildBegin(), iend = GetGo()->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if( (*i)->GetInventory()->GetSelectedRangedWeapon() )
			{
				++m_NumRangedFighters;
			}
		}
	}

	m_MaxEquppedRangedWeaponRange = 0;
	GetMaxEquppedRangedWeaponRange(); // force recalc
}


void GoParty :: CheckMemberToAdd( Go* potentialMember )
{
	if ( !GetGo()->GetPlayer()->IsComputerPlayer() )
	{
		if ( !FileSys::IsValidFileName( potentialMember->GetCommon()->GetScreenName() ) )
		{
			gperrorf(( "Invalid party member name detected! '%S' contains characters "
					   "that the file system doesn't like. The template '%s' screen "
					   "name must be changed. This is required for multiplayer "
					   "character save to work.",
					   potentialMember->GetCommon()->GetScreenName().c_str(),
					   potentialMember->GetTemplateName() ));
		}
	}
}


DWORD GoParty :: GoPartyToNet( GoParty* x )
{
	return ( MakeInt( x->GetGoid() ) );
}


GoParty* GoParty :: NetToGoParty( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetParty() : NULL );
}


void FormSpot :: Draw()
{
	if( m_Dirty )
	{
		RemoveDecal();
		AddDecal();
		m_Dirty = false;
	}

#if !GP_RETAIL
	if( m_Formation->GetParty()->GetGo()->IsHuded() )
	{
		gpstring label;
		label.assignf( "%d", m_DecalId );
		gWorld.DrawDebugPoint( m_Position, 0.1f, COLOR_RED, label.c_str() );
	}
#endif
}


void FormSpot :: AddDecal()
{
	if( !m_DecalId )
	{
		SiegePos pos = m_Position;
		pos.pos.y += 0.6f;

#if !GP_RETAIL
		if( m_Formation->GetParty()->GetGo()->IsHuded() )
		{
			gWorld.DrawDebugPoint( pos, 0.1f, COLOR_WHITE, "" );
		}
#endif
		gpstring decalFilename;

		if( m_IsFree )
		{
			decalFilename = m_Formation->GetFreeSpotDecalFilename();
		}
		else if( m_IsReserved )
		{
			decalFilename = m_Formation->GetNormalSpotDecalFilename();
		}
		else
		{
			gpassert( 0 );
		}

		if( m_IsLeader )
		{
			decalFilename = m_Formation->GetLeaderSpotDecalFilename();
		}

		if( m_IsTerrainBlocked )
		{
			decalFilename = m_Formation->GetTerrainBlockedSpotDecalFilename();
		}

		gpassert( !decalFilename.empty() );

		m_DecalId = gSiegeDecalDatabase.CreateDecal( pos, m_Orientation, 1.5f, 1.5f, 0.1f, 1.0f, decalFilename, false, false, false );
		gSiegeDecalDatabase.GetDecalPointer( m_DecalId )->SetLevelOfDetail( 0 );
	}
}


void FormSpot :: RemoveDecal()
{
	if( m_DecalId )
	{
		gSiegeDecalDatabase.DestroyDecal( m_DecalId );
		m_DecalId = 0;
	}
}


/////////////////////////////////////////////////////////////
// Formation

DWORD Formation::MAX_FORMATION_MEMBERS = 8;

Formation :: Formation( GoParty * party )
	: m_pParty( party )
	, m_DrawAssignedSpots( false )
	, m_DrawFreeSpots( false )
	, m_Dirty( true )
	, m_LeaderW( 0 )
	, m_LeaderL( 0 )
	, m_SpotSpacing( 0 )
{
	m_Name = "wedge";
	Clear();

	SetDirection( vector_3::NORTH );
}


Formation :: ~Formation()
{
	Clear();
}


bool Formation :: Validate()
{
	FormSpotColl::iterator is;

	// now that all members have a spot, delete any left-over spots...
	for( is = m_Spots.begin(); is != m_Spots.end(); ++is )
	{
		if( !(*is)->GetPosition().node.IsValidSlow() )
		{
			return false;
		}
	}
	return true;
}


void Formation :: Clear()
{
	m_Width			= FORMATION_HALF_WIDTH;
	m_Length		= FORMATION_HALF_LENGTH;
	m_MaxWidth		= FORMATION_HALF_WIDTH;
	m_MaxLength		= FORMATION_HALF_LENGTH;

	m_SpotWidth		= 0;
	m_SpotLength	= 0;

	stdx::for_each( m_Spots, stdx::delete_by_ptr() );
	m_Spots.clear();

	SetDirty();
}


bool Formation :: Xfer( FuBi::PersistContext& persist )
{
	// xfer simple stuff
	persist.Xfer( "m_Name",				 m_Name              );
	persist.Xfer( "m_Position",          m_Position          );
	persist.Xfer( "m_Direction",         m_Direction         );
	persist.Xfer( "m_MemberDirection",	 m_MemberDirection   );
	persist.Xfer( "m_SpotWidth",		 m_SpotWidth		 );
	persist.Xfer( "m_SpotLength",		 m_SpotLength        );
	persist.Xfer( "m_SpotSpacing",       m_SpotSpacing       );
	persist.Xfer( "m_LastIntersectsW",   m_LastIntersectsW   );
	persist.Xfer( "m_LastIntersectsL",   m_LastIntersectsL   );
	persist.Xfer( "m_Width",             m_Width             );
	persist.Xfer( "m_Length",            m_Length            );
	persist.Xfer( "m_MaxWidth",          m_MaxWidth          );
	persist.Xfer( "m_MaxLength",         m_MaxLength         );
	persist.Xfer( "m_pParty",            m_pParty            );

	FUBI_XFER_BITFIELD_BOOL( persist, "m_DrawAssignedSpots", m_DrawAssignedSpots );
	FUBI_XFER_BITFIELD_BOOL( persist, "m_DrawFreeSpots",     m_DrawFreeSpots     );

	// xfer complex stuff
	//persist.Xfer( "m_Spots", m_Spots );

	//persist.XferMap( "m_SavedSpotsMap", m_SavedSpotsMap );
	return ( true );
}


void Formation :: SetDirection( vector_3 const & dir )
{
	m_Direction		= dir;
	m_Direction.y		= 0;	// I don't like doing this
	m_Direction.Normalize();
	m_MemberDirection = m_Direction;

	SetDirty();
}


void Formation :: SetMemberDirection( vector_3 const & dir )
{
	m_MemberDirection		= dir;
	m_MemberDirection.y	= 0;	// I don't like doing this
	m_MemberDirection.Normalize();

	SetDirty();
}


void Formation :: SetPosition( SiegePos const & pos )
{	
	SiegePos temp( pos );

	if ( pos != SiegePos::INVALID )
	{
		gSiegeEngine.AdjustPointToTerrain( temp, siege::LF_HUMAN_PLAYER );
	}

	m_Position = temp;

	SetDirty();
}


void Formation :: Update()
{
	if( m_Position == SiegePos::INVALID )
	{
		return;
	}

	if( !(m_DrawAssignedSpots ) )
	{
		return;
	}

	if( m_Dirty )
	{
		UpdateSpots();
		m_Dirty = false;
	}

	FormSpotColl::iterator i;
	for( i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		(*i)->Draw();
	}
}


bool Formation :: CalcSpotPos( const float w, const float l, SiegePos & pos )
{
	gpassert( !GetDirection().IsZero() );

	pos = GetPosition();
	float width	 = w - m_CenterW;
	float length = l - m_CenterL;

	vector_3 unit_w = m_Direction.RotateY_T( -PI/2 );
	unit_w.Normalize();
	unit_w *= ( m_SpotWidth + m_SpotSpacing );

	vector_3 unit_l = m_Direction;
	unit_l.Normalize();
	unit_l *= ( m_SpotLength + m_SpotSpacing );

	pos.pos += ( unit_w * width );
	pos.pos += ( unit_l * length );

	if( m_pParty->GetGo()->GetPlayer()->GetController() == PC_HUMAN )
	{
		return gSiegeEngine.AdjustPointToTerrain( pos, siege::LF_HUMAN_PLAYER );
	}
	else
	{
		return gSiegeEngine.AdjustPointToTerrain( pos, siege::LF_COMPUTER_PLAYER );
	}
}


bool Formation :: CalcSpotPoints( const float w, const float l, SiegePos & center, SiegePos & tl, SiegePos & tr, SiegePos & br, SiegePos & bl )
{
	bool goodPoints = CalcSpotPos( w, l, center );

	if( goodPoints )
	{
		// calc other positions
		goodPoints = goodPoints && CalcSpotPos( w-(m_SpotWidth*0.3f), l+(m_SpotLength*0.3f), tl );
		goodPoints = goodPoints && CalcSpotPos( w+(m_SpotWidth*0.3f), l+(m_SpotLength*0.3f), tr );
		goodPoints = goodPoints && CalcSpotPos( w+(m_SpotWidth*0.3f), l-(m_SpotLength*0.3f), br );
		goodPoints = goodPoints && CalcSpotPos( w-(m_SpotWidth*0.3f), l-(m_SpotLength*0.3f), bl );
	}

	return goodPoints;
}


bool Formation :: Move( eQPlace place, eActionOrigin origin, bool followMode, bool bPreserveGuarding )
{
	typedef std::map< float, Go * > distMap;
	distMap tempDistMap;

	gpassertm( GetPosition() != SiegePos::INVALID, "You crazy monkey!  You telling the formation to move without setting it's position!" );

	if( !gSiegeEngine.IsNodeInAnyFrustum( GetPosition().node ) )
	{
		SetPosition( m_pParty->GetGo()->GetPlacement()->GetPosition() );
		return false;
	}

	SetShape( GetShape() );
	UpdateSpots();

	////////////////////////////////////////
	//	find formation leader

	FormSpot * leaderSpot = 0;
	FindSpot( m_LeaderW, m_LeaderL, leaderSpot );
	if ( !leaderSpot )
	{
		return false;
	}

	GoHandle leader( leaderSpot->GetOccupant() );
	if( !leader )
	{
		gperror( "Unknown leader of formation!  Perhaps formation move called with empty selection?  Request will be ignored.\n" );
		return false;
	}

	////////////////////////////////////////
	//	sort all party members by distance from leader

	gpassert( leader->GetParent() && leader->GetParent()->HasParty() );
	Go * party = leader->GetParent();

	for( GopColl::iterator i = party->GetChildBegin(); i != party->GetChildEnd(); ++i )
	{
		if( ( MakeInt( leader->GetWorldFrustumMembership() ) & MakeInt( (*i)->GetWorldFrustumMembership() ) ) == 0 )
		{
			continue;
		}

		if( !(IsAlive( (*i)->GetLifeState() ) || (*i)->IsGhost()) )
		{
			continue;
		}
		
		// don't move members that don't share a frustum with leader
		if( m_pParty->GetGo()->IsAnyHumanParty() && !(*i)->GetMind()->IsCurrentActionHumanInterruptable() )
		{
			continue;
		}

		float dist = gAIQuery.GetDistance( leader, (*i) );
		tempDistMap.insert( std::make_pair( dist, (*i ) ) );
	}

	////////////////////////////////////////
	//	move selected non-packers

	GopColl moved;
	Go * lastGo = NULL;

	FormSpotColl::iterator is;
	for( is = m_Spots.begin(); is != m_Spots.end(); ++ is )
	{
		// $$$ mod terrain blocked to use is walkable
		if( (*is)->IsTerrainBlocked() )
		{
			continue;
		}

		// if the occupant is valid, go to next
		GoHandle occupant( (*is)->GetOccupant() );
		if( occupant )
		{

			if( lastGo == NULL || ( lastGo->GetMind()->GetRank() < occupant->GetMind()->GetRank() ) )
			{
				lastGo = occupant;
			}			

			occupant->GetMind()->RSMove( (*is)->GetPosition(), place, origin );
			moved.push_back( occupant );
		}
	}

	////////////////////////////////////////
	//	have remainding non-packers

	GopColl unmovedNormal;
	GopColl unmovedPackers;

	if( lastGo )
	{
		Go * leader = lastGo;

		for( distMap::iterator id = tempDistMap.begin(); id != tempDistMap.end(); ++id )
		{
			if( moved.linear_find( (*id).second ) != moved.end() )
			{
				continue;
			}
			else
			{
				if( (*id).second->GetInventory()->IsPackOnly() )
				{
					unmovedPackers.push_back( (*id).second );
				}
				else
				{
					unmovedNormal.push_back( (*id).second );
				}
			}
		}

		if( followMode )
		{
			// assign follow to remainder
			float followDistance = 3.0f;	// $$$ constant
			for( GopColl::iterator im = unmovedNormal.begin(); im != unmovedNormal.end(); ++im )
			{
				// Preserve the guard job when following if necessary ( the action queue is about to be cleared )
				bool bPushFront = false;
				if ( place == QP_CLEAR && bPreserveGuarding && (*im)->GetMind()->GetFrontActionJat() == JAT_GUARD )
				{
					bPushFront = true;
				}

				JobReq tempReq( MakeJobReq( JAT_FOLLOW, JQ_ACTION, bPushFront ? QP_FRONT : place, origin, leader->GetGoid() ) );
				tempReq.m_Float1 = followDistance;
				followDistance += gWorldOptions.GetDefaultFollowerSpacing();
				(*im)->GetMind()->SDoJob( tempReq );
				lastGo = (*im);
			}
		}
	}

	////////////////////////////////////////
	// regardless of mode, have any mules follow the last guy

	if( lastGo  )
	{
		float followDistance = 3.0f;  // $$$ constant
		for( GopColl::iterator ip = unmovedPackers.begin(); ip != unmovedPackers.end(); ++ip )
		{
			// if we're not in follow mode, only selected mules follow the leader
			if( m_pParty->GetGo()->IsAnyHumanParty() && ( !followMode && !(*ip)->IsSelected() ) )
			{
				if ( !followMode )
				{
					Job * pJob = (*ip)->GetMind()->GetFrontJob( JQ_ACTION );
					if( pJob && pJob->GetJobAbstractType() == JAT_FOLLOW )
					{
						(*ip)->GetMind()->RSStop( AO_REFLEX );
					}
				}
				continue;
			}

			if( (*ip)->GetMind()->CanOperateOn( lastGo, true ) )
			{
				JobReq & req = MakeJobReq( JAT_FOLLOW, JQ_ACTION, place, origin, lastGo->GetGoid() );
				req.m_Float1 = followDistance;
				followDistance += gWorldOptions.GetDefaultPackFollowerSpacing();
				(*ip)->GetMind()->SDoJob( req );
			}
		}
	}

	Clear();
	m_Position = SiegePos::INVALID; // $ not the best place for it
	return true; // $$$
}


#if 0
bool Formation :: MoveAndFollow( eQPlace place, eActionOrigin origin, Go * leader, SiegePos const & pos, bool bUseSelected )
{	
	typedef std::map< float, Go * > distMap;

	distMap tempDistMap;

	////////////////////////////////////////
	//	sort all party members by distance

	gpassert( leader->GetParent() && leader->GetParent()->HasParty() );
	Go * party = leader->GetParent();

	for( GopColl::iterator i = party->GetChildBegin(); i != party->GetChildEnd(); ++i )
	{
		if( (*i) == leader || (!(*i)->IsSelected() && bUseSelected) )
		{
			continue;
		}

		if( ( MakeInt( leader->GetWorldFrustumMembership() ) & MakeInt( (*i)->GetWorldFrustumMembership() ) ) == 0 )
		{
			continue;
		}

		float dist = gAIQuery.GetDistance( leader, (*i) );
		tempDistMap.insert( std::make_pair( dist, (*i ) ) );
	}

	// assign move to leader...
	if ( leader->GetMind()->GetFrontJob( JQ_ACTION ) &&
		 leader->GetMind()->GetFrontJob( JQ_ACTION )->GetJobAbstractType() == JAT_DRINK )
	{
		return false;
	}
	
	leader->GetMind()->SDoJob( MakeJobReq( JAT_MOVE, JQ_ACTION, place, origin, pos ) );

	// assign follow to remainder

	float distance = 2.0f;
	for( distMap::iterator id = tempDistMap.begin(); id != tempDistMap.end(); ++id, distance += 1.5f )
	{
		JobReq tempReq( MakeJobReq( JAT_FOLLOW, JQ_ACTION, place, origin, leader->GetGoid() ) );
		tempReq.m_Float1 = distance;

		(*id).second->GetMind()->SDoJob( tempReq );
	}

	return true;
}
#endif


bool Formation :: HasOccupant( FormSpotColl const & spots, Goid occ )
{
	FormSpotColl::const_iterator is;

	for( is = spots.begin(); is != spots.end(); ++ is )
	{
		if( (*is)->GetOccupant() == occ )
		{
			return true;
		}
	}
	return false;
}


bool Formation :: FindSpot( int w, int l, FormSpot * & spot )
{
	FormSpotColl::const_iterator is;

	for( is = m_Spots.begin(); is != m_Spots.end(); ++ is )
	{
		if( ((*is)->GetW() == w) && ((*is)->GetL() == l) )
		{
			spot = (*is);
			return true;
		}
	}

	return false;
}


bool Formation :: FindSpot( Goid occupant, FormSpot * & spot, bool findAlternate )
{
	FormSpotColl::const_iterator is;

	for( is = m_Spots.begin(); is != m_Spots.end(); ++ is )
	{
		if( (*is)->GetOccupant() == occupant )
		{
			if( findAlternate == (*is)->IsAlternate() )
			{
				spot = (*is);
				return true;
			}
		}
	}
	return false;
}


void Formation :: SetSpotSpacing( float spacing )
{
	m_SpotSpacing = max( spacing, m_MinSpotSpacing );
	m_SpotSpacing = min( m_SpotSpacing, m_MaxSpotSpacing );

	SetDirty();
}


bool Formation :: Rotate( float rad )
{
	vector_3 dir = m_Direction;
	dir.RotateY( rad );
	dir.Normalize();
	SetDirection( dir );

	SetDirty();

	// $$$ test if new positions for spots are valid

	return true;
}


bool Formation :: RotateMembers( float rad )
{
	vector_3 dir = m_MemberDirection;
	dir.RotateY( rad );
	dir.Normalize();

	SetMemberDirection( dir );

	SetDirty();

	// $$$ test if new positions for spots are valid

	return true;
}


void Formation :: HideSpots()
{
	FormSpotColl::iterator i;
	for ( i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		(*i)->RemoveDecal();
	}

	SetDrawAssignedSpots( false );
	SetDrawFreeSpots( false );
}


void Formation :: UpdateSpots()
{
	UpdateSpotsUtil();
	if( !AllOccupiedSpotsBlocked() )
	{
		return;
	}

	const vector_3 oldDirection( GetDirection() );
	float testRotation			= 0.0f;
	const float rotationDelta	= PI/16.0f;

	while( testRotation <= PI/2.0f )
	{
		testRotation += rotationDelta;

		vector_3 testDirectionLeft( oldDirection );
		testDirectionLeft.RotateY( -testRotation );

		SetDirection( testDirectionLeft );
		UpdateSpotsUtil();
		
		if( !AllOccupiedSpotsBlocked() )
		{
			return;
		}
		
		vector_3 testDirectionRight( oldDirection );
		testDirectionRight.RotateY( testRotation );

		SetDirection( testDirectionRight );
		UpdateSpotsUtil();

		if( !AllOccupiedSpotsBlocked() )
		{
			return;
		}
	}

	SetDirection( oldDirection );
}


void Formation :: UpdateSpotsUtil()
{
	// clear spot status
	for( FormSpotColl::iterator i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		(*i)->SetIsTerrainBlocked( false );
	}

	int w = m_Width;
	int l = m_Length;

	siege::SiegeNodeHandle centerHandle = gSiegeEngine.NodeCache().UseObject( GetPosition().node );
	siege::SiegeNode const & centerNode = centerHandle.RequestObject( gSiegeEngine.NodeCache() );

	for( int iw = -w; iw != w+1; ++iw )
	{
		for( int il = -l; il != l+1; ++il )
		{
			FormSpot * spot = NULL;
			SiegePos center, tl, tr, br, bl;

			// set positions

			if( FindSpot( iw, il, spot ) )
			{
				bool centerGood = CalcSpotPos( float( iw ), float( il ), center );

				spot->SetPos( center );

				siege::SiegeNodeHandle spotHandle = gSiegeEngine.NodeCache().UseObject( center.node );
				siege::SiegeNode const & spotNode = spotHandle.RequestObject( gSiegeEngine.NodeCache() );

				// Calculate local direction of member direction
				vector_3 local_dir		= centerNode.GetCurrentOrientation() * m_MemberDirection;
				local_dir				= spotNode.GetTransposeOrientation() * local_dir;
				local_dir.Normalize();

				// Build orientation
				matrix_3x3 spotOrient;
				spotOrient.SetColumn0( CrossProduct( local_dir, -vector_3::UP ) );
				spotOrient.SetColumn1( local_dir );
				spotOrient.SetColumn2( -vector_3::UP );

				spot->SetOrientation( spotOrient );

				// set properties
				spot->SetIsLeader( ( iw == m_LeaderW ) && ( il == m_LeaderL ) );
				spot->SetIsFree( spot->GetOccupant() == GOID_INVALID );
				spot->SetIsReserved( spot->GetOccupant() != GOID_INVALID );

				GoHandle occupant( spot->GetOccupant() );

				if( centerGood && occupant )
				{
					bool blocked = !gAIQuery.IsAreaWalkable( occupant->GetBody()->GetTerrainMovementPermissions(), center, 6, 0.15f ); // $$ constant
					spot->SetIsTerrainBlocked( blocked );

					if( !blocked && !spot->IsAlternate() )
					{
						for( FormSpotColl::iterator i = m_Spots.begin(); i != m_Spots.end(); )
						{
							if( (*i)->IsAlternate() && ( (*i)->GetOccupant() == spot->GetOccupant() ) )
							{
								delete( *i );
								i = m_Spots.erase( i );
							}
							else
							{
								++i;
							}
						}
					}
				}
				else if( !centerGood )
				{
					spot->SetIsTerrainBlocked( true );
					spot->RemoveDecal();
				}
			}
		}
	}

	ValidateAdjustFormation();
}


bool Formation :: AllOccupiedSpotsBlocked()
{
	int occupied = 0;
	int occupiedAndBlocked = 0;

	for( FormSpotColl::iterator i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		if( (*i)->GetOccupant() != GOID_INVALID )
		{
			++occupied;
			if( (*i)->IsTerrainBlocked() )
			{
				++occupiedAndBlocked;
			}
		}
	}

	return occupied == occupiedAndBlocked;
}


void Formation :: ValidateAdjustFormation()
{
	SiegePos leaderPos;

	// find leader pos
	for( FormSpotColl::iterator i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		if( (*i)->IsLeader() )
		{
			leaderPos = (*i)->GetPosition();
			break;
		}
	}

	// mark spots too far up/down from leader as bad
	for( i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		if( (*i)->IsLeader() || (*i)->IsTerrainBlocked() )
		{
			continue;
		}

		vector_3 diff = gAIQuery.GetDifferenceVector( leaderPos, (*i)->GetPosition() );

		if( AbsoluteValue( diff.y ) > gWorldOptions.GetFormationSpotVerticalConstraint() )
		{
			(*i)->SetIsTerrainBlocked( true );
		}
	}

	// now take all occupied spots that are blocked and try to place behind leader...
	bool allBad = false;

	FormSpotColl blocked;
	FormSpot * pLeader = NULL;

	for( i = m_Spots.begin(); i != m_Spots.end(); ++i )
	{
		GoHandle occupant( (*i)->GetOccupant() );
		if( !occupant )
		{
			continue;
		}

		if( (*i)->IsLeader() )
		{
			pLeader = *i;
			if( (*i)->IsTerrainBlocked() )
			{
				allBad = true;
				break;
			}
		}

		if( (*i)->IsTerrainBlocked() )
		{
			if( (*i)->IsAlternate() )
			{
				allBad = true;
				break;
			}
			FormSpot * spot = NULL;
			if( !FindSpot( (*i)->GetOccupant(), spot, true ) )
			{
				blocked.push_back( *i );
			}
		}
	}

	if( !pLeader )
	{
		gpwarning( "no formation leader.  probably nobody was selected.\n" );
		return;
	}

	if( !allBad )
	{
		siege::SiegeNodeHandle centerHandle = gSiegeEngine.NodeCache().UseObject( GetPosition().node );
		siege::SiegeNode const & centerNode = centerHandle.RequestObject( gSiegeEngine.NodeCache() );

		for( FormSpotColl::iterator j = blocked.begin(); j != blocked.end(); ++j )
		{
			bool FoundBetterSpot = false;

			for( int k = pLeader->GetL(); k != -m_MaxLength; --k )
			{
				if( CanOccupySpot( pLeader->GetW(), k ) )
				{
					SiegePos center, tl, tr, br, bl;
					if( CalcSpotPoints( float( pLeader->GetW() ), float( k ), center, tl, tr, br, bl ) )
					{
						FormSpot * betterSpot = SetSpotOccupant( (*j)->GetOccupant(), pLeader->GetW(), k );
						betterSpot->SetPos( center );

						siege::SiegeNodeHandle spotHandle = gSiegeEngine.NodeCache().UseObject( center.node );
						siege::SiegeNode const & spotNode = spotHandle.RequestObject( gSiegeEngine.NodeCache() );

						// Calculate local direction of member direction
						vector_3 local_dir		= centerNode.GetCurrentOrientation() * m_MemberDirection;
						local_dir				= spotNode.GetTransposeOrientation() * local_dir;
						local_dir.Normalize();

						// Build orientation
						matrix_3x3 spotOrient;
						spotOrient.SetColumn0( CrossProduct( local_dir, -vector_3::UP ) );
						spotOrient.SetColumn1( local_dir );
						spotOrient.SetColumn2( -vector_3::UP );

						betterSpot->SetOrientation( spotOrient );

						betterSpot->SetIsAlternate( true );
						betterSpot->SetIsReserved( true );
						betterSpot->SetIsFree( false );
						FoundBetterSpot = true;
						break;
					}
				}
			}

			if( !FoundBetterSpot )
			{
				allBad = true;
				break;
			}
		}
	}

	if( allBad )
	{
		for( i = m_Spots.begin(); i != m_Spots.end(); ++i )
		{
			(*i)->SetIsTerrainBlocked( true );
		}
	}
}


bool Formation :: CanOccupySpot( int w, int l )
{
	FormSpot * spot;

	if( FindSpot( w, l, spot ) )
	{
		return spot->GetOccupant() == GOID_INVALID;
	}
	else
	{
		return true;
	}
}


FormSpot * Formation :: SetSpotOccupant( Goid occupant, int w, int l )
{
	//gpgenericf(( "Formation :: OccupySpot - %s, at x,y = %d,%d\n", occupant->GetTemplateName(), w, l ));

	if( !CanOccupySpot( w, l ) )
	{
		gpassertm( 0, "This spot can't be occupied." );
		return NULL;
	}

	FormSpot * spot = NULL;
	FindSpot( w, l, spot );

	if( !spot )
	{
		spot = new FormSpot( this );
		m_Spots.push_back( spot );
		spot->SetW( w );
		spot->SetL( l );
	}

	spot->SetOccupant( occupant );

	SetDirty();
	return spot;
}


bool Formation :: FindNextFreeSpot( int & w, int & l )
{
	for( int il = 0; il != -m_Length-1; --il )
	{
		for( int iw = 0; iw != m_Width+1; ++iw )
		{
			FormSpot * spot;

			if( !FindSpot( iw, il, spot ) )
			{
				w = iw;
				l = il;
				return true;
			}
			else if( !FindSpot( -iw, il, spot ) )
			{
				w = -iw;
				l = il;
				return true;
			}
		}
	}
	return false;
}


void Formation :: SetShapeSingleSpot( Go * member )
{
	Clear();

	m_SpotWidth		= 1.0;
	m_SpotLength	= 1.0;

	m_CenterW		= 0;
	m_CenterL		= 0;

	m_LeaderW		= 0;
	m_LeaderL		= 0;
	
	SetSpotOccupant( member->GetGoid(), 0, 0 );

	SetDirty();
}


void Formation :: SetShape( gpstring const & name )
{
	Clear();
	m_Name = name;
	SetDirty();

	////////////////////////////////////////
	//	validate params

	if( name.same_no_case( "none" ) )
	{
		return;
	}

	//////////////////////////////////////////////////////////////////
	//	load default formation info

	gpstring formationAddress( "world:ai:formations:" );
	formationAddress.append( name );

	FastFuelHandle formation( formationAddress );

	if( !formation )
	{
		gpassertm( 0, "Can't find default formation .gas block." );
		return;
	}

	formation.Get( "reserved_decal",			m_ReservedSpotDecalFilename, false );
	formation.Get( "free_decal",				m_FreeSpotDecalFilename, false );
	formation.Get( "leader_decal",				m_LeaderSpotDecalFilename, false );
	formation.Get( "terrain_blocked_decal",		m_TerrainBlockedSpotDecalFilename, false );

	if( IsZero( m_SpotSpacing ) )
	{
		formation.Get( "initial_spacing",		m_SpotSpacing, false );
	}

	formation.Get( "min_spacing",				m_MinSpotSpacing, false );
	formation.Get( "max_spacing",				m_MaxSpotSpacing, false );

	formation.Get( "spot_w",					m_SpotWidth,  false );
	formation.Get( "spot_l",					m_SpotLength, false );
	formation.Get( "center_w",					m_CenterW,    false );
	formation.Get( "center_l",					m_CenterL,    false );

	////////////////////////////////////////
	//	figure out who will enter formation

	GopColl allMembers;
	m_pParty->GetMembersAccordingToRank( allMembers );

	////////////////////////////////////////
	//	get regular movers - ignore packers

	GopColl movingMembers;
	QtColl qtc;

	qtc.push_back( QT_ALIVE_CONSCIOUS );

	if( m_pParty->GetGo()->GetPlayer()->GetController() == PC_HUMAN )
	{
		qtc.push_back( QT_NONPACK  );
		qtc.push_back( QT_SELECTED );
	}

	gAIQuery.Get( NULL, qtc, allMembers, movingMembers );

	if( movingMembers.empty() && ( m_pParty->GetGo()->GetPlayer()->GetController() == PC_HUMAN ) )
	{
		qtc.clear();		
		qtc.push_back( QT_PACK  );
		qtc.push_back( QT_SELECTED );
		gAIQuery.Get( NULL, qtc, allMembers, movingMembers );		
	}

	if( movingMembers.empty() && ( m_pParty->GetGo()->GetPlayer()->GetController() == PC_HUMAN ) )
	{
		qtc.clear();		
		qtc.push_back( QT_GHOST );
		qtc.push_back( QT_NONPACK  );
		qtc.push_back( QT_SELECTED );
		gAIQuery.Get( NULL, qtc, allMembers, movingMembers );		
	}

	if( movingMembers.size() == 1 )
	{
		SetShapeSingleSpot( movingMembers.front() );
		return;
	}

	if( movingMembers.empty() )
	{
		gpwarningf(( "UI issue: formation was told to take shape, but there are no Gos selected.  Caller check usage." ));
		return;
	}

	////////////////////////////////////////
	//	validate params

	if( movingMembers.size() > MAX_FORMATION_MEMBERS )
	{
		gpscreen( $MSG$ "Your party is too large to move in formation.\n" );
		return;
	}

	PriorityToCoordinate priCordMap;

	gpstring key, value;

	for( int y = FORMATION_HALF_LENGTH; y != -FORMATION_HALF_LENGTH - 1; --y )
	{
		stringtool::Set( y, key );
		if( formation.Get( key, value ) )
		{
			for( int x = -FORMATION_HALF_WIDTH; x != FORMATION_HALF_WIDTH + 1; ++x )
			{
				gpstring sPri;
				int pri;

				stringtool::GetDelimitedValue( value, ',', x + FORMATION_HALF_WIDTH, sPri );
				stringtool::Get( sPri, pri );

				if( pri > 0 )
				{
					Coordinate entry( x, y );
					priCordMap.insert( PriorityToCoordinate::value_type( pri, entry ) );
				}
			}
		}
		else
		{
			gpassertm( 0, "Default formation data syntax error." );
		}
	}

	if( priCordMap.size() != MAX_FORMATION_MEMBERS )
	{
		gpwarningf(( "Formation '%s' defined %d positions.  Must define %d\n", name.c_str(), priCordMap.size(), MAX_FORMATION_MEMBERS ));
		return;
	}
	
	/*
	gpgeneric( "Default formation:\n" );
	PriorityToCoordinate::iterator ip;
	for( ip = priCordMap.begin(); ip != priCordMap.end(); ++ip )
	{
		gpgenericf(( "pri = %d, x,y = %d, %d\n", (*ip).first, (*ip).second.first, (*ip).second.second ));
	}
	*/

	//////////////////////////////////////////////////////////////////
	//	assign default formation spots
	//	first to melee, then ranged, then magic, then helpers/others

	int highestLeader = MAX_FORMATION_MEMBERS + 1;  // 1 + max party members

	gpassert( movingMembers.size() <= MAX_FORMATION_MEMBERS );

	typedef std::map< DWORD, Go * > IntToGoMap;
	IntToGoMap rankGoMap;
	for( GopColl::iterator im = movingMembers.begin(); im != movingMembers.end(); ++im )
	{
		DWORD rank = (*im)->GetMind()->GetRank();
		gpverify( rankGoMap.insert( std::make_pair( rank, *im ) ).second );
	}

	// set occupied spots

	PriorityToCoordinate::iterator iSpot = priCordMap.begin();
	for( IntToGoMap::iterator ir = rankGoMap.begin(); ir != rankGoMap.end(); ++ir )
	{
		gpassert( iSpot != priCordMap.end() );
		unsigned int rank = (*ir).second->GetMind()->GetRank();
		gpassert( rank < MAX_FORMATION_MEMBERS );

		int x, y;
		x = (*iSpot).second.first;
		y = (*iSpot).second.second;
		SetSpotOccupant( (*ir).second->GetGoid(), x, y );

		if( (*iSpot).first <= highestLeader )
		{
			m_LeaderW = x;
			m_LeaderL = y;						
			highestLeader = (*iSpot).first;
		}

		iSpot = priCordMap.erase( iSpot );
	}

	// set free spots

	for( PriorityToCoordinate::iterator imap = priCordMap.begin(); imap != priCordMap.end(); ++imap )
	{
		int x, y;
		x = (*imap).second.first;
		y = (*imap).second.second;

		SetSpotOccupant( GOID_INVALID, x, y );
	}
}


void Formation :: SetNextShape()
{
	FastFuelHandle order( "world:ai:formations:ui_order" );
	gpassert( order );

	FastFuelFindHandle fh( order );
	fh.FindFirstKeyAndValue();

	gpstring formationNum, formationName;

	while( fh.GetNextKeyAndValue( formationNum, formationName ) )
	{
		if( formationName.same_no_case( m_Name ) )
		{
			break;
		}
	}

	int num;
	gpstring newFormationNum;
	stringtool::Get( formationNum, num );
	stringtool::Set( ++num, newFormationNum );

	if( !order.Get( newFormationNum, formationName ) )
	{
		if( !order.Get( "0", formationName ) )

		{
			gpassert( 0 );
		}
	}

	SetShape( formationName );
}


//////////////////////////////////////////////////////////////////////////////

