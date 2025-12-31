//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoSupport.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoSupport.h"
#include "siege_decal.h"

#include "FuBiPersist.h"

FUBI_REPLACE_NAME( "siege", Siege );

FUBI_REPLACE_NAME( "LightId_", LightId );
FUBI_EXPORT_POINTER_CLASS( LightId_ );

FUBI_REPLACE_NAME( "DecalId_", DecalId );
FUBI_EXPORT_POINTER_CLASS( DecalId_ );

FUBI_REPLACE_NAME( "SiegeId_", SiegeId );
FUBI_EXPORT_POINTER_CLASS( SiegeId_ );

FUBI_REPLACE_NAME( "SiegeeLogicalNodeFlags", eLogicalNodeFlags );
FUBI_EXPORT_BITFIELD_ENUM( eLogicalNodeFlags );

//////////////////////////////////////////////////////////////////////////////
// special effects implementations

bool StartGenericFx( const char* prefix, const char* name, const char* params )
{
	// create the go
	GoCloneReq cloneReq( name );
	cloneReq.SetStartingPos( SiegePos( vector_3::ZERO, gSiegeEngine.NodeWalker().TargetNodeGUID() ) );
	cloneReq.SetSnapToTerrain();
	Goid goid = gGoDb.CloneLocalGo( cloneReq );

	// set it up
	GoHandle go( goid );
	if ( go )
	{
		gpstring component;

		Go::ComponentColl::iterator i, ibegin = go->GetComponentBegin(), iend = go->GetComponentEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// find ours
			if ( (*i)->GetName().same_no_case( prefix, ::strlen( prefix ) ) )
			{
				// got it
				component = (*i)->GetName();
				break;
			}
		}

		// update item
		if ( !component.empty() )
		{
			// get record
			FuBi::Record* record = go->GetComponentRecord( component );
			gpassert( record != NULL );

			// parse params if any
			if ( params != NULL )
			{
				record->LoadFromString( params );
			}

			// must self destruct
			record->Set( "self_destruct", true );

			// start 'er up next frame after it's been committed into the world
			WorldMessage( WE_REQ_ACTIVATE, goid ).SendDelayed();
		}
	}

	// done
	return ( goid != GOID_INVALID );
}

bool StartCameraFx( const char* name, const char* params )
{
	return ( StartGenericFx( "camera_", name, params ) );
}

bool StartCameraFx( const char* name )
{
	return ( StartCameraFx( name, NULL ) );
}

bool StartLightFx( LightId siegeLight, const char* name, const char* params )
{
	gpstring localParams( "siege_light=" );
	localParams += ::ToString( MakeInt( siegeLight ), FuBi::XFER_HEX );

	if ( params != NULL )
	{
		localParams += '&';
		localParams += params;
	}

	return ( StartGenericFx( "light_", name, localParams ) );
}

bool StartLightFx( LightId siegeLight, const char* name )
{
	return ( StartLightFx( siegeLight, name, NULL ) );
}

bool StartDecalFx( DecalId siegeDecal, const char* name, const char* params )
{
	gpstring localParams( "siege_decal=" );
	localParams += ::ToString( MakeInt( siegeDecal ), FuBi::XFER_HEX );

	if ( params != NULL )
	{
		localParams += '&';
		localParams += params;
	}

	return ( StartGenericFx( "decal_", name, localParams ) );
}

bool StartDecalFx( DecalId siegeDecal, const char* name )
{
	return ( StartDecalFx( siegeDecal, name, NULL ) );
}

//////////////////////////////////////////////////////////////////////////////
// type LightId implementation

const LightId LIGHTID_INVALID = MakeLightId( siege::database_guid().GetValue() );

bool IsValid( LightId l, bool testExists )
{
	if ( testExists )
	{
		return ( gSiegeEngine.LightDatabase().HasLightDescriptor( MakeSiegeGuid( l ) ) );
	}
	else
	{
		return ( l != LIGHTID_INVALID );
	}
}

inline siege::LightDescriptor GetLightDescriptor( LightId id )
{
	return ( gSiegeEngine.LightDatabase().GetLightDescriptor( MakeSiegeGuid( id ) ) );
}

inline void SetLightDescriptor( LightId id, const siege::LightDescriptor& ld )
{
	gSiegeEngine.LightDatabase().SetLightDescriptor( MakeSiegeGuid( id ), ld );
}

DWORD LightId_ :: GetColor( void ) const
{
	return ( GetLightDescriptor( this ).m_Color );
}

bool LightId_ :: GetActive( void ) const
{
	return ( GetLightDescriptor( this ).m_bActive );
}

float LightId_ :: GetIntensity( void ) const
{
	return ( GetLightDescriptor( this ).m_Intensity );
}

void LightId_ :: GetPosition( SiegePos& pos ) const
{
	pos = gSiegeEngine.LightDatabase().GetLightNodeSpacePosition( MakeSiegeGuid( this ) );
}

void LightId_ :: SetColor( DWORD color )
{
	siege::LightDescriptor ld = GetLightDescriptor( this );
	ld.m_Color = color;
	SetLightDescriptor( this, ld );
}

void LightId_ :: SetActive( bool active )
{
	siege::LightDescriptor ld = GetLightDescriptor( this );
	ld.m_bActive = active;
	SetLightDescriptor( this, ld );
}

void LightId_ :: SetIntensity( float intensity )
{
	siege::LightDescriptor ld = GetLightDescriptor( this );
	ld.m_Intensity = intensity;
	ld.m_bActive = !::IsZero( intensity );
	SetLightDescriptor( this, ld );
}

//////////////////////////////////////////////////////////////////////////////
// type DecalId implementation

const DecalId DECALID_INVALID = MakeDecalId( siege::database_guid().GetValue() );

bool IsValid( DecalId d, bool testExists )
{
	if ( testExists )
	{
		return ( gSiegeEngine.DecalDatabase().GetDecalPointer( MakeSiegeGuid( d ) ) != NULL );
	}
	else
	{
		return ( d != DECALID_INVALID );
	}
}

inline siege::SiegeDecal* GetDecalPointer( DecalId d )
{
	return ( gSiegeEngine.DecalDatabase().GetDecalPointer( MakeSiegeGuid( d ) ) );
}

bool DecalId_ :: GetActive( void ) const
{
	return ( GetDecalPointer( this )->GetActive() );
}

BYTE DecalId_ :: GetAlpha( void ) const
{
	return ( GetDecalPointer( this )->GetAlpha() );
}

void DecalId_ :: SetActive( bool active )
{
	GetDecalPointer( this )->SetActive( active );
}

void DecalId_ :: SetAlpha( BYTE alpha )
{
	GetDecalPointer( this )->SetAlpha( alpha );
}

//////////////////////////////////////////////////////////////////////////////
// type SiegeId implementation

const SiegeId SIEGEID_INVALID = MakeSiegeId( siege::database_guid().GetValue() );

bool IsValid( SiegeId s, bool testExists )
{
	if ( testExists )
	{
		return ( MakeSiegeGuid( s ).IsValidSlow() );
	}
	else
	{
		return ( s != SIEGEID_INVALID );
	}
}

//////////////////////////////////////////////////////////////////////////////
// GoString traits implementation

bool FuBi::Traits <GoString> :: XferAsString( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const char* defValue )
{
	gpstring str( obj );
	bool rc = persist.Xfer( xfer, key, str, defValue );
	if ( persist.IsRestoring() )
	{
		obj = str;
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// GowString traits implementation

bool FuBi::Traits <GowString> :: XferAsString( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const wchar_t* defValue )
{
	gpwstring str( obj );
	bool rc = persist.Xfer( xfer, key, str, defValue );
	if ( persist.IsRestoring() )
	{
		obj = str;
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// Go* traits implementation

bool FuBi::Traits <Go*> :: XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
{
#	if !GP_RETAIL
	if ( persist.IsSaving() && (obj != NULL) && obj->IsLocalGo() )
	{
		// this makes sure that we don't persist any lodfi local go's.
		// those should be effectively INVISIBLE to the system, and
		// nobody should be keeping pointers to them.
		if ( obj->IsLodfi() )
		{
			gperrorf(( "Big problem! Someone is persisting a lodfi Go*!\n" ));
		}
	}
#	endif // !GP_RETAIL

	Goid goid = GOID_INVALID;
	if ( persist.IsSaving() && (obj != NULL) )
	{
		goid = obj->GetGoid();

#		if !GP_RETAIL
		GoHandle go( goid );
		if ( go != obj )
		{
			gperrorf(( "Big problem! Somehow a Go is being persisted yet when resolving its Goid 0x%08X, it does not point back to itself!\n", goid ));
		}
#		endif // !GP_RETAIL
	}

	bool rc = persist.Xfer( xfer, key, goid );

	if ( persist.IsRestoring() )
	{
		obj = GoHandle( goid );

#		if !GP_RETAIL
		if ( (goid != GOID_INVALID) && (obj == NULL) )
		{
			gperrorf(( "Big problem! A Go* that was persisted ok as 0x%08X was unable to reload!\n", goid ));
		}
#		endif // !GP_RETAIL
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// namespace GoMath implementation

float GoMath :: RandomFloat( float minFloat, float maxFloat )
{
	return ( GetGoCreateRng().Random( minFloat, maxFloat ) );
}

float GoMath :: RandomFloat( float maxFloat )
{
	return ( GetGoCreateRng().Random( maxFloat ) );
}

int GoMath :: RandomInt( int minInt, int maxInt )
{
	return ( GetGoCreateRng().Random( minInt, maxInt ) );
}

int GoMath :: RandomInt( int maxInt )
{
	return ( GetGoCreateRng().Random( maxInt ) );
}

float GoMath :: OrbitAngleToTarget( Goid go, Goid target )
{
	if( go != GOID_INVALID && target != GOID_INVALID )
	{
		Go* pGo		= GetGo( go );
		Go* pTarget	= GetGo( target );

		if( pGo && pGo->HasPlacement() && pTarget && pTarget->HasPlacement() )
		{

			const SiegePos& gpos = pGo->GetPlacement()->GetPosition();
			const SiegePos& tpos = pTarget->GetPlacement()->GetPosition();

			if ( gSiegeEngine.IsNodeInAnyFrustum(gpos.node) && 
				 gSiegeEngine.IsNodeInAnyFrustum(tpos.node) &&
				!gSiegeEngine.SpaceDiffers(gpos.node,tpos.node)
				)
			{
				// Get the current vector facing direction
				vector_3 current_orient( DoNotInitialize );
				pGo->GetPlacement()->GetOrientation().RotateZAxis( current_orient );
				current_orient.y	= 0.0f;

				// Get the difference vector to target
				vector_3 totarget	= -gSiegeEngine.GetDifferenceVector( gpos, tpos );
				totarget.y			= 0.0f;

				// Return the angle difference
				float short_angle	= AngleBetween( current_orient, totarget );
				return( CrossProduct( current_orient, totarget ).y > 0.0f ? short_angle : -short_angle );
			}

		}
	}

	return 0.0f;
}

float GoMath :: AzimuthAngleToTarget( Goid go, Goid target )
{
	if( go != GOID_INVALID && target != GOID_INVALID )
	{
		Go* pGo		= GetGo( go );
		Go* pTarget	= GetGo( target );

		if( pGo && pGo->HasPlacement() && pTarget && pTarget->HasPlacement() )
		{
			const SiegePos& gpos = pGo->GetPlacement()->GetPosition();
			const SiegePos& tpos = pTarget->GetPlacement()->GetPosition();

			if ( gSiegeEngine.IsNodeInAnyFrustum(gpos.node) && 
				 gSiegeEngine.IsNodeInAnyFrustum(tpos.node) &&
				!gSiegeEngine.SpaceDiffers(gpos.node,tpos.node)
				)
			{
				// Get the current vector facing direction
				vector_3 current_orient( DoNotInitialize );
				pGo->GetPlacement()->GetOrientation().RotateZAxis( current_orient );
				float angle_to_top	= AngleBetween( current_orient, vector_3::UP );

				vector_3 totarget	= -gSiegeEngine.GetDifferenceVector( gpos, tpos );

				// Return the angle difference
				return (AngleBetween( totarget, vector_3::UP ) - angle_to_top);
			}
		}
	}

	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////////
