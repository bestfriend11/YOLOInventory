//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoBase.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoBase.h"

#include "ContentDb.h"
#include "FuBiBitPacker.h"
#include "GoAttack.h"
#include "WorldMap.h"

#include "GoFollower.h"

//////////////////////////////////////////////////////////////////////////////
// class GoString implementation

GoString& GoString :: Assign( const gpstring& str )
{
	m_String = &gContentDb.AddString( str );
	return ( *this );
}

GoString& GoString :: Assign( const char* str )
{
	if ( (m_String == NULL) || (m_String->c_str() != str) )
	{
		if ( str == NULL )
		{
			m_String = &gpstring::EMPTY;
		}
		else
		{
			m_String = &gContentDb.AddString( str );
		}
	}
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// class GowString implementation

GowString& GowString :: Assign( const gpwstring& str )
{
	m_String = &gContentDb.AddString( str );
	return ( *this );
}

GowString& GowString :: Assign( const wchar_t* str )
{
	if ( (m_String == NULL) || (m_String->c_str() != str) )
	{
		if ( str == NULL )
		{
			m_String = &gpwstring::EMPTY;
		}
		else
		{
			m_String = &gContentDb.AddString( str );
		}
	}
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// class GocExporter implementation

GocExporter :: GocExporter( const char* name, const char* internalName, GocFactoryCb cb, bool serverOnly, bool canServerExistOnly, bool devOnly )
{
	m_Name               = name;
	m_InternalName       = internalName;
	m_FactoryCb          = cb;
	m_ServerOnly         = serverOnly;
	m_CanServerExistOnly = canServerExistOnly;
	m_DevOnly            = devOnly;
	m_CacheIndex         = Go::FindCacheIndex( name );
}

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

template <typename T>
void ApplyCalcPosition( T& req )
{
	if ( req.m_UseStartingPos )
	{
		// fix up the siege pos
		if ( req.m_SnapToTerrain )
		{
			// snap the position to the terrain - note that this automatically
			// adjusts the node if necessary (same as UpdateNodePosition).
			gSiegeEngine.AdjustPointToTerrain( req.m_StartingPos );
			req.m_SnapToTerrain = false;
		}
		else if ( !req.m_ForcePosition )
		{
			// see if offset from node results in placement in a new node
			SiegePos correct( req.m_StartingPos );
			gSiegeEngine.AdjustPointToTerrain( correct );

			req.m_StartingPos.pos = req.m_StartingPos.WorldPos();
			req.m_StartingPos.node = correct.node;
			req.m_StartingPos.FromWorldPos( req.m_StartingPos.pos, req.m_StartingPos.node );
		}
	}
}

template <typename T>
void ApplyFinish( const T& req, Go* go )
{
	gpassert( !req.m_SnapToTerrain );

	// placement will have already been calc'd
	if ( req.m_UseStartingPos && go->HasPlacement() )
	{
		GPDEV_ONLY( go->SetPlacementChanging() );
		go->GetPlacement()->ForceSetPosition( req.m_StartingPos );
		GPDEV_ONLY( go->ClearPlacementChanging() );
	}

	// update orientation
	if ( req.m_UseStartingOrient && go->HasPlacement() )
	{
		go->GetPlacement()->SetOrientation( req.m_StartingOrient );
	}

	if ( go->HasFollower() )
	{
		go->GetFollower()->ForceUpdatePositionOrientation( );
	}

	// do fadein if requested
	if ( req.m_FadeIn && go->HasAspect() )
	{
		go->StartFading();
	}

	// mark as no-fx if requested
	if ( req.m_NoStartupFx )
	{
		go->SetNoStartupFx();
	}
}

//////////////////////////////////////////////////////////////////////////////
// class GoCreateReq implementation

GoCreateReq :: GoCreateReq( Scid scid, RegionId regionId, PlayerId playerId )
{
	Init();

	m_Scid     = scid;
	m_RegionId = regionId;
	m_PlayerId = playerId;

	gpassert( AssertValid() );
}

void GoCreateReq :: Init( void )
{
	m_Scid              = SCID_INVALID;
	m_RegionId          = REGIONID_INVALID;
	m_PlayerId          = PLAYERID_INVALID;
	m_MpPlayerCount     = 0;
	m_UseStartingPos    = false;
	m_UseStartingOrient = false;
	m_SnapToTerrain     = false;
	m_LocalGo           = false;
	m_LodfiGo           = false;
	m_FadeIn            = false;
	m_ForcePosition     = false;
	m_PrepareToDrawNow  = false;
	m_NoStartupFx       = false;
}

void GoCreateReq :: CalcPosition( void )
{
	ApplyCalcPosition( *this );
}

int GoCreateReq :: GetMpPlayerCount( void ) const
{
	return ( (m_MpPlayerCount == 0) ? Player::GetHumanPlayerCount() : m_MpPlayerCount );
}

void GoCreateReq :: Finish( Go* go ) const
{
	ApplyFinish( *this, go );
	go->m_IsLodfi = m_LodfiGo;
}

void GoCreateReq :: SetStartingPos( const SiegePos& siegePos )
{
	m_StartingPos = siegePos;

	if ( m_StartingPos.node.IsValid() )
	{
		m_UseStartingPos = true;
	}
}

void GoCreateReq :: Xfer( FuBi::BitPacker& packer, GoCreateReq& def )
{
	static GoCreateReq s_Defaults;

	packer.XferRaw( m_Scid );
	packer.XferIf( m_RegionId, def.m_RegionId );

	// $ playerid maxes out at bit index 8 so treat as word
	packer.XferAsBitIf( m_PlayerId, def.m_PlayerId, 2 );

	packer.XferCount( m_MpPlayerCount, 1 );

	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_UseStartingPos    );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_UseStartingOrient );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_SnapToTerrain     );

	// $ note that we do not do lodfis over net
	gpassert( !m_LocalGo );
	gpassert( !m_LodfiGo );

	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_FadeIn            );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_PrepareToDrawNow  );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_NoStartupFx       );

	if ( m_UseStartingPos )
	{
		FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_ForcePosition );
		packer.XferIf2( m_StartingPos, def.m_StartingPos, s_Defaults.m_StartingPos );
	}
	if ( m_UseStartingOrient )
	{
		packer.XferIf2( m_StartingOrient, def.m_StartingOrient, s_Defaults.m_StartingOrient );
	}

	// now that we've used the default, set it to self for next time
	def = *this;
}

#if !GP_RETAIL

void GoCreateReq :: RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf( "scid = 0x%08X, regionId = 0x%08X", m_Scid, m_RegionId );

	if ( m_PlayerId != PLAYERID_INVALID )
	{
		appendHere.appendf( ", playerId = 0x%08X", m_PlayerId );
	}

	if ( m_MpPlayerCount == 0 )
	{
		appendHere.append( ", mpPlayerCount = <DETECT>" );
	}
	else
	{
		appendHere.appendf( ", mpPlayerCount = %d", m_MpPlayerCount );
	}

	if ( m_UseStartingPos )
	{
		appendHere.appendf( ", startingPos = %g,%g,%g,%s",
				m_StartingPos.pos.x, m_StartingPos.pos.y, m_StartingPos.pos.z,
				m_StartingPos.node.ToString().c_str() );
	}

	if ( m_UseStartingOrient )
	{
		appendHere.appendf( ", startingOrient = %g,%g,%g,%g",
				m_StartingOrient.m_x, m_StartingOrient.m_y, m_StartingOrient.m_z, m_StartingOrient.m_w );
	}

	if ( m_SnapToTerrain )
	{
		appendHere += ", +snapToTerrain";
	}
	if ( m_LocalGo )
	{
		appendHere += ", +localGo";
	}
	if ( m_LodfiGo )
	{
		appendHere += ", +lodfiGo";
	}
	if ( m_FadeIn )
	{
		appendHere += ", +fadeIn";
	}
	if ( m_ForcePosition )
	{
		appendHere += ", +forcePosition";
	}
	if ( m_PrepareToDrawNow )
	{
		appendHere += ", +prepDrawNow";
	}
	if ( m_NoStartupFx )
	{
		appendHere += ", +noStartupFx";
	}
}

gpstring GoCreateReq :: RpcToString( void ) const
{
	gpstring out;
	RpcToString( out );
	return ( out );
}

#endif // !GP_RETAIL

#if GP_DEBUG

bool GoCreateReq :: AssertValid( void ) const
{
	gpassert( IsInstance( m_Scid, false ) );
	gpassert( gWorldMap.ContainsRegion( m_RegionId ) );
	gpassert( ::IsValid( m_PlayerId ) );
	gpassert( !m_LodfiGo || m_LocalGo );
	gpassert( (m_MpPlayerCount >= 0) && (m_MpPlayerCount <= Server::MAX_PLAYERS) );

	return ( true );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// class CreateGoXfer implementation

void CreateGoXfer :: Xfer( FuBi::BitPacker& packer, CreateGoXfer& def )
{
	packer.XferRaw( m_StartGoid );
	m_CreateReq.Xfer( packer, def.m_CreateReq );
	packer.XferBit( m_Standalone );

	if ( !m_Standalone )
	{
		packer.XferIf( m_RandomSeed    );
	}

	// $ never xfer more than 65K in one shot, limit to 2 bytes
	packer.XferBuffer( m_Data, m_DataBuffer, 2 );

	// now that we've used the default, set it to self for next time
	def.CopyForDefault( *this );
}

#if !GP_RETAIL

void CreateGoXfer :: RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf( "startGoid = 0x%08X, randomSeed = 0x%08X", m_StartGoid, m_RandomSeed );
	if ( m_Standalone )
	{
		appendHere += " +standalone";
	}
	appendHere += ", ";
	m_CreateReq.RpcToString( appendHere );

	if ( m_Data.size > 0 )
	{
		appendHere.appendf( ", syncSize = %d, syncData = [", m_Data.size );

		const BYTE* b, * bbegin = (const BYTE*)m_Data.mem, * bend = bbegin + m_Data.size;
		if ( bbegin != bend )
		{
			for ( b = bbegin ; b != bend ; ++b )
			{
				if ( b != bbegin )
				{
					appendHere += " ";
				}
				appendHere.appendf( "%02x", *b );
			}
		}

		appendHere += "]";
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class GoCloneReq implementation

GoCloneReq :: GoCloneReq( Goid parent, Goid cloneSource, PlayerId playerId )
{
	Init();

	m_Parent      = parent;
	m_CloneSource = cloneSource;
	m_PlayerId    = playerId;

	gpassert( AssertValid() );
}

GoCloneReq :: GoCloneReq( Goid parent, const char* templateName, PlayerId playerId )
{
	Init();

	m_Parent      = parent;
	m_CloneSource = templateName ? gGoDb.FindCloneSource( templateName ) : GOID_INVALID;
	m_PlayerId    = playerId;

	gpassert( AssertValid() );
}

GoCloneReq :: GoCloneReq( Goid cloneSource, PlayerId playerId )
{
	Init();

	m_CloneSource = cloneSource;
	m_PlayerId    = playerId;

	gpassert( AssertValid() );
}

GoCloneReq :: GoCloneReq( const char* templateName, PlayerId playerId )
{
	Init();

	m_CloneSource = templateName ? gGoDb.FindCloneSource( templateName ) : GOID_INVALID;
	m_PlayerId    = playerId;

	gpassert( AssertValid() );
}

void GoCloneReq :: Init( void )
{
	m_Parent             = GOID_INVALID;
	m_CloneSource        = GOID_INVALID;
	m_PlayerId           = PLAYERID_INVALID;
	m_MpPlayerCount      = 0;
	m_UseStartingPos     = false;
	m_UseStartingOrient  = false;
	m_SnapToTerrain      = false;
	m_OmniGo             = false;
	m_AllClients         = false;
	m_FadeIn             = false;
	m_ForcePosition      = false;
	m_ForceServerOnly    = false;
	m_ForceClientAllowed = false;
	m_PrepareToDrawNow   = false;
	m_NoStartupFx        = false;
	m_XferCharacterPost  = false;
	m_PrepareAmmo        = false;
}

void GoCloneReq :: CalcPosition( void )
{
	ApplyCalcPosition( *this );
}

int GoCloneReq :: GetMpPlayerCount( void ) const
{
	return ( (m_MpPlayerCount == 0) ? Player::GetHumanPlayerCount() : m_MpPlayerCount );
}

void GoCloneReq :: Finish( Go* go ) const
{
	// preprocess - copy placement from the parent if none was specified
	if ( InheritsStartingPosOrient() && go->HasPlacement() )
	{
		GoCloneReq localCloneReq( *this );
		GoHandle parent( m_Parent );

		gpassert( !localCloneReq.m_SnapToTerrain );
		localCloneReq.SetStartingPos   ( parent->GetPlacement()->GetPosition   () );
		localCloneReq.SetStartingOrient( parent->GetPlacement()->GetOrientation() );

		ApplyFinish( localCloneReq, go );
	}
	else
	{
		ApplyFinish( *this, go );
	}

	// update bits
	if ( m_ForceServerOnly )
	{
		go->SetServerOnly( true );
	}
	if ( m_ForceClientAllowed )
	{
		go->SetServerOnly( false );
	}
}

void GoCloneReq :: FinishPost( Go* go ) const
{
	// prepare ammo if requested
	if ( m_PrepareAmmo )
	{
		if ( go->HasAspect() && go->HasPhysics() && go->HasAttack() && go->HasParent() && go->GetParent()->HasAttack()  )
		{
			go->GetParent()->GetAttack()->PrepareAmmo( go );
			go->m_IsPreparedAmmo = true;
		}
		else if ( ::IsServerLocal() )
		{
			gperrorf((
					"Cannot construct ammo '%s' that does not have aspect, "
					"physics, and attack components, and a parent with an "
					"attack component. Aborting...\n", go->GetTemplateName() ));
			gGoDb.SMarkForDeletion( go->GetGoid() );
		}
	}
}

bool GoCloneReq :: InheritsStartingPosOrient( void ) const
{
	GoHandle parent( m_Parent );
	if (   parent						// there is a parent
		&& parent->HasPlacement()		// it's got placement
		&& !parent->IsOmni()			// but it's not omni (which is inherited)
		&& !m_OmniGo					// and we're not requesting omni either
		&& !m_UseStartingPos			// and we haven't spec'd a starting pos
		&& !m_UseStartingOrient )		// or a starting orientation
	{
		return ( true );				// then it can be inherited from parent!
	}
	return ( false );					// or not
}

void GoCloneReq :: SetStartingPos( const SiegePos& siegePos )
{
	m_StartingPos = siegePos;

	if ( m_StartingPos.node.IsValid() )
	{
		m_UseStartingPos = true;
	}
}

Player* GoCloneReq :: GetPlayer( Go* tryOtherGo ) const
{
	// get player - defaults to parent's then clone source's player
	Player* player = NULL;
	if ( m_PlayerId != PLAYERID_INVALID )
	{
		player = gServer.GetPlayer( m_PlayerId );
	}
	else
	{
		// try parent
		if ( m_Parent != GOID_INVALID )
		{
			GoHandle parent( m_Parent );
			if ( parent )
			{
				player = parent->GetPlayer();
			}
		}

		// parent no work? try other go...
		if ( (player == NULL) && (tryOtherGo != NULL) )
		{
			// try clone source if exists
			player = tryOtherGo->GetPlayer();
		}
		else
		{
			// otherwise fall back to computer
			player = gServer.GetComputerPlayer();
		}
	}

	return ( player );
}

void GoCloneReq :: Xfer( FuBi::BitPacker& packer, GoCloneReq& def )
{
	static GoCloneReq s_Defaults;

	packer.XferIf2( m_Parent, def.m_Parent, s_Defaults.m_Parent );
	packer.XferIf( m_CloneSource, s_Defaults.m_CloneSource );

	// $ playerid maxes out at bit index 8 so treat as word
	packer.XferAsBitIf( m_PlayerId, def.m_PlayerId, 2 );

	packer.XferCount( m_MpPlayerCount, 1 );

	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_OmniGo             );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_AllClients         );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_FadeIn             );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_ForceServerOnly    );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_ForceClientAllowed );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_PrepareToDrawNow   );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_NoStartupFx        );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_XferCharacterPost  );
	FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_PrepareAmmo        );

	if ( !m_OmniGo )
	{
		FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_UseStartingPos );
		if ( m_UseStartingPos )
		{
			FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_SnapToTerrain );
			FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_ForcePosition );
			packer.XferIf2( m_StartingPos, def.m_StartingPos, s_Defaults.m_StartingPos );
		}

		FUBI_XFER_PACKED_BITFIELD_BOOL( packer, m_UseStartingOrient  );
		if ( m_UseStartingOrient )
		{
			packer.XferIf2( m_StartingOrient, def.m_StartingOrient, s_Defaults.m_StartingOrient );
		}
	}

	// now that we've used the default, set it to self for next time
	def = *this;
}

#if !GP_RETAIL

void GoCloneReq :: RpcToString( gpstring& appendHere ) const
{
	bool empty = true;
	if ( m_Parent != GOID_INVALID )
	{
		if ( GoDb::DoesSingletonExist() )
		{
			GoHandle parent( m_Parent );
			appendHere.appendf( "parent = 0x%08X (%s)", m_Parent, parent ? parent->GetTemplateName() : "<invalid>" );
		}
		else
		{
			appendHere.appendf( "parent = 0x%08X", m_Parent );
		}

		empty = false;
	}

	if ( m_CloneSource != GOID_INVALID )
	{
		if ( !empty )
		{
			appendHere += ", ";
		}

		if ( GoDb::DoesSingletonExist() )
		{
			GoHandle cloneSource( m_CloneSource );
			appendHere.appendf( "cloneSource = 0x%08X (%s)", m_CloneSource, cloneSource ? cloneSource->GetTemplateName() : "<invalid>" );
		}
		else
		{
			appendHere.appendf( "cloneSource = 0x%08X", m_CloneSource );
		}

		empty = false;
	}

	if ( !empty )
	{
		appendHere += ", ";
	}
	appendHere.appendf( "playerId = 0x%08X", m_PlayerId );

	if ( m_UseStartingPos )
	{
		appendHere.appendf( ", startingPos = %g,%g,%g,%s",
				m_StartingPos.pos.x, m_StartingPos.pos.y, m_StartingPos.pos.z,
				m_StartingPos.node.ToString().c_str() );
	}

	if ( m_MpPlayerCount == 0 )
	{
		appendHere.append( ", mpPlayerCount = <DETECT>" );
	}
	else
	{
		appendHere.appendf( ", mpPlayerCount = %d", m_MpPlayerCount );
	}

	if ( m_UseStartingOrient )
	{
		appendHere.appendf( ", startingOrient = %g,%g,%g,%g",
				m_StartingOrient.m_x, m_StartingOrient.m_y, m_StartingOrient.m_z, m_StartingOrient.m_w );
	}

	if ( m_SnapToTerrain )
	{
		appendHere += ", +snapToTerrain";
	}
	if ( m_OmniGo )
	{
		appendHere += ", +omniGo";
	}
	if ( m_AllClients )
	{
		appendHere += ", +allClients";
	}
	if ( m_FadeIn )
	{
		appendHere += ", +fadeIn";
	}
	if ( m_ForcePosition )
	{
		appendHere += ", +forcePosition";
	}
	if ( m_ForceServerOnly )
	{
		appendHere += ", +forceServerOnly";
	}
	if ( m_ForceClientAllowed )
	{
		appendHere += ", +forceClientAllowed";
	}
	if ( m_PrepareToDrawNow )
	{
		appendHere += ", +prepDrawNow";
	}
	if ( m_NoStartupFx )
	{
		appendHere += ", +noStartupFx";
	}
	if ( m_XferCharacterPost )
	{
		appendHere += ", +xferCharacterPost";
	}
	if ( m_PrepareAmmo )
	{
		appendHere += ", +prepareAmmo";
	}
}

#endif // !GP_RETAIL

#if GP_DEBUG

bool GoCloneReq :: AssertValid( void ) const
{
	gpassert( (m_CloneSource == GOID_INVALID) || ::IsValid( m_CloneSource ) );
	gpassert( (m_PlayerId == PLAYERID_INVALID) || ::IsValid( m_PlayerId ) );
	gpassert( !m_ForceServerOnly || !m_ForceClientAllowed );
	gpassert( (m_MpPlayerCount >= 0) && (m_MpPlayerCount <= Server::MAX_PLAYERS) );

	if ( m_OmniGo )
	{
		gpassert( !m_UseStartingPos    );
		gpassert( !m_UseStartingOrient );
		gpassert( !m_SnapToTerrain     );
		gpassert( !m_ForcePosition     );
	}

	return ( true );
}

#endif // GP_DEBUG

GoCloneReq& MakeGoCloneReq( Goid parent, Goid cloneSource, PlayerId playerId )
{
	static GoCloneReq s_GoCloneReq;
	s_GoCloneReq = GoCloneReq( parent, cloneSource, playerId );
	return ( s_GoCloneReq );
}

GoCloneReq& MakeGoCloneReq( Goid parent, Goid cloneSource )
{
	return ( MakeGoCloneReq( parent, cloneSource, PLAYERID_INVALID ) );
}

GoCloneReq& MakeGoCloneReq( Goid cloneSource, PlayerId playerId )
{
	return ( MakeGoCloneReq( GOID_INVALID, cloneSource, playerId ) );
}

GoCloneReq& MakeGoCloneReq( Goid cloneSource )
{
	return ( MakeGoCloneReq( GOID_INVALID, cloneSource, PLAYERID_INVALID ) );
}

GoCloneReq& MakeGoCloneReq( Goid parent, const char* templateName, PlayerId playerId )
{
	static GoCloneReq s_GoCloneReq;
	s_GoCloneReq = GoCloneReq( parent, templateName, playerId );
	return ( s_GoCloneReq );
}

GoCloneReq& MakeGoCloneReq( Goid parent, const char* templateName )
{
	return ( MakeGoCloneReq( parent, templateName, PLAYERID_INVALID ) );
}

GoCloneReq& MakeGoCloneReq( const char* templateName, PlayerId playerId )
{
	return ( MakeGoCloneReq( NULL, templateName, playerId ) );
}

GoCloneReq& MakeGoCloneReq( const char* templateName )
{
	return ( MakeGoCloneReq( GOID_INVALID, templateName, PLAYERID_INVALID ) );
}

GoCloneReq& MakeGoCloneReq( void )
{
	return ( MakeGoCloneReq( GOID_INVALID, (const char*)NULL, PLAYERID_INVALID ) );
}

//////////////////////////////////////////////////////////////////////////////
// class CloneGoXfer implementation

void CloneGoXfer :: Xfer( FuBi::BitPacker& packer, CloneGoXfer& def )
{
	packer.XferRaw( m_StartGoid );
	m_CloneReq.Xfer( packer, def.m_CloneReq );
	packer.XferBit( m_Standalone );

	if ( !m_Standalone )
	{
		packer.XferIf( m_RandomSeed );
	}

	// $ template names never > 255 chars, so limit count size to 1 byte
	packer.XferStringIf( m_TemplateName, m_TemplateNameBuffer, def.m_TemplateName, 1 );

	// $ never xfer more than 65K in one shot, limit to 2 bytes
	packer.XferBuffer( m_Data, m_DataBuffer, 2 );

	// now that we've used the default, set it to self for next time
	def.CopyForDefault( *this );
}

#if !GP_RETAIL

void CloneGoXfer :: RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf(
			"startGoid = 0x%08X, randomSeed = 0x%08X, templateName = '%s'",
			m_StartGoid, m_RandomSeed, m_TemplateName ? m_TemplateName : "<null>" );
	if ( m_Standalone )
	{
		appendHere += " +standalone";
	}
	appendHere += ", ";
	m_CloneReq.RpcToString( appendHere );

	if ( m_Data.size > 0 )
	{
		appendHere.appendf( ", syncSize = %d, syncData = [", m_Data.size );

		const BYTE* b, * bbegin = (const BYTE*)m_Data.mem, * bend = bbegin + m_Data.size;
		if ( bbegin != bend )
		{
			for ( b = bbegin ; b != bend ; ++b )
			{
				if ( b != bbegin )
				{
					appendHere += " ";
				}
				appendHere.appendf( "%02x", *b );
			}
		}

		appendHere += "]";
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
