//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoShmo.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoShmo.h"

#include "ContentDb.h"
#include "FuBiSchema.h"
#include "GoAspect.h"
#include "GoData.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "NeMa_AspectMgr.h"
#include "Siege_Camera.h"

DECLARE_GO_COMPONENT( GoGold, "gold" );
DECLARE_GO_COMPONENT( GoPotion, "potion" );
DECLARE_GO_COMPONENT( GoFader, "fader" );

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_POINTERCLASS_TRAITS_EX( nema::HAspect, NemaAspect, nema::HAspect::Null );

//////////////////////////////////////////////////////////////////////////////
// class GoGold implementation

GoGold :: GoGold( Go* go )
	: Inherited( go )
{
	// Init the dropper of this Gold to an invalid player. 
	// This way we can know that it's ok to spit the gold in MP if sharing is on.
	m_DroppedBy = GOID_INVALID;
}

GoGold :: GoGold( const GoGold& source, Go* newGo )
	: Inherited( source, newGo ),
	  m_RangeColl( source.m_RangeColl )
{
	m_DroppedBy = source.m_DroppedBy;
}

GoGold :: ~GoGold( void )
{
	// this space intentionally left blank...
}

void GoGold :: SetGoldValueDirty( void )
{
	// don't do anything in edit mode
	if ( IsWorldEditMode() )
	{
		return;
	}

	if ( m_RangeColl && !m_RangeColl->empty() )
	{
		int gold = GetGo()->GetAspect()->GetGoldValue();

		// find a good aspect/texture combo to use
		GoGoldRangeColl::reverse_iterator i, begin = m_RangeColl->rbegin(), end = m_RangeColl->rend() - 1;
		for ( i = begin ; i != end ; ++i )
		{
			if ( gold >= i->m_MinGold )
			{
				break;
			}
		}

		// swap out the aspect
		GetGo()->GetAspect()->SetCurrentAspectSimple( i->m_Model.c_str(), i->m_Texture.c_str() );
	}
}

FuBiCookie GoGold :: RCDepositSelfIn( Go* where )
{
	FUBI_RPC_THIS_CALL_RETRY( RCDepositSelfIn, RPC_TO_ALL );

	if ( where->HasInventory() )
	{
		where->GetInventory()->RCDepositGold( GetGo() );
	}

	return ( RPC_SUCCESS );
}


GoComponent* GoGold :: Clone( Go* newGo )
{
	return ( new GoGold( *this, newGo ) );
}

bool GoGold :: Xfer( FuBi::PersistContext& persist )
{
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_DroppedBy", m_DroppedBy );
	}
	return ( true );
}

void GoGold :: Shutdown( void )
{
	// am i party gold? if so tell my player i'm gone!
	if ( GetGo()->GetPlayer()->GetTradeGold() == GetGoid() )
	{
		GetGo()->GetPlayer()->ClearTradeGold();
	}
}

bool GoGold :: CommitCreation( void )
{
	m_RangeColl = gContentDb.AddOrFindGoldRangeColl( GetData()->GetInternalField( "ranges" ) );
	SetGoldValueDirty();
	return ( true );
}

DWORD GoGold :: GoGoldToNet( GoGold* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoGold* GoGold :: NetToGoGold( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetGold() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
// class GoPotion implementation

GoPotion :: GoPotion( Go* go )
	: Inherited( go )
{
	// this space intentionally left blank...
}

GoPotion :: GoPotion( const GoPotion& source, Go* newGo )
	: Inherited( source, newGo ),
	  m_RangeColl( source.m_RangeColl )
{
	// this space intentionally left blank...
}

GoPotion :: ~GoPotion( void )
{
	// this space intentionally left blank...
}

const gpstring& GoPotion :: GetInventoryIcon2( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "inventory_icon_2" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoPotion :: SetFullRatioDirty( void )
{
	if ( m_RangeColl && !m_RangeColl->empty() )
	{
		float ratio = GetGo()->GetMagic()->GetPotionFullRatio();

		// find a good aspect/texture combo to use
		GoPotionRangeColl::const_iterator i, begin = m_RangeColl->begin(), end = m_RangeColl->end() - 1;
		for ( i = begin ; i != end ; ++i )
		{
			if ( ratio < i->m_MaxRatio )
			{
				break;
			}
		}

		// swap out the aspect
		GetGo()->GetAspect()->SetCurrentAspectTexture( 0, i->m_Texture.c_str() );
	}
}

GoComponent* GoPotion :: Clone( Go* newGo )
{
	return ( new GoPotion( *this, newGo ) );
}

bool GoPotion :: Xfer( FuBi::PersistContext& /*persist*/ )
{
	return ( true );
}

bool GoPotion :: CommitCreation( void )
{
	m_RangeColl = gContentDb.AddOrFindPotionRangeColl( GetData()->GetInternalField( "ranges" ) );
	return ( true );
}

DWORD GoPotion :: GoPotionToNet( GoPotion* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoPotion* GoPotion :: NetToGoPotion( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetPotion() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
// class GoFader implementation

GoFader :: GoFader( Go* go )
	: Inherited( go )
{
	m_ObjectScale  = 0;
	m_OldAspect    = GOID_INVALID;
}

GoFader :: GoFader( const GoFader& source, Go* newGo )
	: Inherited( source, newGo )
{
	m_ObjectScale  = source.m_ObjectScale;
	m_OldAspect    = source.m_OldAspect;
}

GoFader :: ~GoFader( void )
{
	// free resources
	if ( m_Aspect )
	{
		gAspectStorage.ReleaseAspect( m_Aspect );
	}
}

nema::HAspect GoFader :: GetAspectHandle( void ) const
{
	gpassert( m_Aspect );
	return ( m_Aspect );
}

siege::eCollectFlags GoFader :: BuildCollectFlags( void ) const
{
	return ( GetGo()->IsFading() ? siege::CF_FADEOUT : siege::CF_NONE );
}

bool GoFader :: BuildRenderInfo( siege::ASPECTINFO& info, siege::eRequestFlags rFlags )
{
	// early bailout if nothing to do
	if ( !m_Aspect )
	{
		return ( false );
	}

	// check purge
	if ( m_Aspect->IsPurged( 1 ) )
	{
		m_Aspect->Reconstitute( 0, true );

		// Restore the old primary bone position and orientation
		if( m_Aspect->IsPrimaryBoneLocked() )
		{
			m_Aspect->LockPrimaryBone( m_PrimaryBonePos, m_PrimaryBoneOrient );
		}
	}

// Perform render visibility tests.

	// figure out position
	SiegePos pos = GetGo()->GetPlacement()->GetPosition();
	const Quat& quat = GetGo()->GetPlacement()->GetOrientation();

	// am i culled from the view frustum?
	if( (rFlags & siege::RF_BOUNDS) || (rFlags & siege::RF_FRUSTUMCLIP) )
	{
		GoAspect::GetWorldSpaceOrientedBoundingVolume( &info.BoundOrient, &info.BoundCenter, &info.BoundHalfDiag, pos, quat, GetAspectHandle().Get() );
	}

	if( rFlags & siege::RF_FRUSTUMCLIP )
	{
		if( gSiegeEngine.GetCamera().GetFrustum().CullObject( info.BoundOrient, info.BoundCenter, info.BoundHalfDiag ) )
		{
			return ( false );
		}
	}

// Fill out the info struct.

	// fill out the info struct
	info.Object				= MakeInt( GetGoid() );
	info.Aspect				= GetAspectHandle().Get();
	info.MegamapIconTex		= 0;

	if( rFlags & siege::RF_POSITION )
	{
		info.Origin				= pos.pos;
	}
	if( rFlags & siege::RF_ORIENT )
	{
		quat.BuildMatrix( info.Orientation );
	}
	if( rFlags & siege::RF_SCALE )
	{
		info.ObjectScale		= m_ObjectScale;
		info.BoundingBodyScale	= m_ObjectScale;
	}
	if( rFlags & siege::RF_RENDERINDEX )
	{
		GoPlacement* placement = GetGo()->GetPlacement();
		info.RenderIndex1		= placement->GetRenderIndexCache1();
		info.RenderIndex2		= placement->GetRenderIndexCache2();
		info.RenderIndex3		= placement->GetRenderIndexCache3();
	}
	if( rFlags & siege::RF_ALPHA )
	{
		info.Alpha = info.Aspect->GetAlpha();

		if ( GetGo()->IsFading() )
		{
			if ( GetGo()->CheckFadingFirstTime() )
			{
				info.Alpha = 255;
			}
			else if ( info.Alpha == 0 )
			{
				// stop fading and self-destruct when siege has made us fully opaque
				GetGo()->StopFading();
				gGoDb.SMarkForDeletion( GetGoid() );
			}
		}
	}

	// all ok
	return ( true );
}

void GoFader :: TakeAspect( GoAspect* aspect )
{
	gpassert( aspect );

	// remember who it was
	m_OldAspect = aspect->GetGoid();

	// take aspect
	m_Aspect = aspect->GetAspectHandle();
	gAspectStorage.AddRefAspect( m_Aspect );

	// copy its params
	m_ObjectScale = aspect->GetRenderScale();

	// detach aspect from parent if any so it can render on its own and fade out
	nema::HAspect parent = m_Aspect->GetParent();
	if ( parent )
	{
		// Get true Go for nema parent to get proper scaling and orientation
		vector_3 parentScale	= parent->GetCurrentScale();
		GoHandle hParent( parent->GetGoid() );
		if ( hParent && hParent->HasAspect() )
		{
			parentScale		*= hParent->GetAspect()->GetRenderScale();
		}

		vector_3 childPos  ( DoNotInitialize );
		Quat     childRot  ( DoNotInitialize );
		m_Aspect->GetPrimaryBoneOrientation( childPos, childRot );

		parent->DetachChild( m_Aspect );

		// Need to remove any parent scale that was applied to the object
		// We compensates for the scale of the arrow so that it stays the
		// same size when it get stuck in to different scaled characters
		// This code should be checked against regular krug_grunts & scaled up
		// krug_commanders. After reference counting, object scaling is just
		// about my favorite thing to stay up all night staring at...

		childPos			*= parentScale;

		vector_3 kid_scale	= m_Aspect->GetCurrentScale();
		kid_scale			*= parentScale;
		m_Aspect->SetCurrentScale( kid_scale );

		m_Aspect->Reconstitute( 0, true );

		m_Aspect->LockPrimaryBone( childPos, childRot );
	}

	// Save the old primary bone position and orientation
	if( m_Aspect->IsPrimaryBoneLocked() )
	{
		m_Aspect->GetPrimaryBoneOrientation( m_PrimaryBonePos, m_PrimaryBoneOrient );
	}

	// say we're fading now
	GetGo()->StartFading();

	// self-destruct a little past when siege would normally kill us
	float timeout = gSiegeEngine.NodeWalker().GetObjectFadeTime() + 2.0f;
	WorldMessage( WE_REQ_DELETE, GetGoid(), GetGoid() ).SendDelayed( timeout );
}

bool GoFader :: FadeAspect( Goid goid )
{
	bool created = false;

	// see if we can use it
	GoHandle go( goid );
	if ( go && go->HasAspect() && go->GetAspect()->GetIsVisible() &&
		(go->IsInAnyWorldFrustum() || go->IsPendingEntry()) &&
		gSiegeEngine.IsNodeValid( go->GetPlacement()->GetPosition().node ) )
	{
		// create proxy
		GoCloneReq cloneReq( gContentDb.GetDefaultFaderProxyTemplate() );
		cloneReq.SetStartingPos( go->GetPlacement()->GetPosition() );
		cloneReq.SetStartingOrient( go->GetPlacement()->GetOrientation() );
		GoHandle fader( gGoDb.CloneLocalGo( cloneReq ) );
		if ( fader )
		{
			// hand off the aspect
			fader->GetFader()->TakeAspect( go->GetAspect() );
			created = true;
		}
		else
		{
			// hide the goaspect that we're fading
			go->GetAspect()->SetIsVisible( false );
		}
	}

	// done
	return ( created );
}

GoComponent* GoFader :: Clone( Go* newGo )
{
	return ( new GoFader( *this, newGo ) );
}

bool GoFader :: Xfer( FuBi::PersistContext& persist )
{
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_ObjectScale",			m_ObjectScale		);

		if ( persist.IsSaving() && m_Aspect && !m_Aspect->ShouldPersist() )
		{
			// $ special: if this aspect is not going to be persisted because
			//   its owning Go is lodfi or whatever, then we shouldn't try to
			//   fade it when we come back because it won't exist! so only save
			//   out zero'd stuff.

			nema::HAspect aspect;
			Goid oldAspect = GOID_INVALID;

			persist.Xfer( "m_Aspect",			aspect				);
			persist.Xfer( "m_OldAspect",		oldAspect			);
		}
		else
		{
			persist.Xfer( "m_Aspect",			m_Aspect			);
			persist.Xfer( "m_OldAspect",		m_OldAspect			);
		}

		persist.Xfer( "m_PrimaryBonePos",		m_PrimaryBonePos	);
		persist.Xfer( "m_PrimaryBoneOrient",	m_PrimaryBoneOrient );

#		if GP_DEBUG
		GoHandle oldAspect( m_OldAspect );
		if ( oldAspect && m_Aspect )
		{
			gpassert( oldAspect->ShouldPersist() == m_Aspect->ShouldPersist() );
		}
#		endif // GP_DEBUG
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
