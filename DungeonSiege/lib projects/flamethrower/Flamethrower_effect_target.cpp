//
//	Flamethrower_effect_target.cpp
//	Copyright 1999, Gas Powered Games
//
#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "vector_3.h"

#include "Flamethrower_effect_target.h"
#include "Flamethrower.h"
#include "Flamethrower_tracker.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"



Flamethrower_effect_target::Flamethrower_effect_target( SFxEID id )
	: Flamethrower_target( Flamethrower_target::EFFECT )
{
	SetID( (UINT32)id );
}


bool
Flamethrower_effect_target::OnDuplicate( Flamethrower_target **pTarget )
{
	*pTarget = new Flamethrower_effect_target( (SFxEID)m_ID );
	return true;
}

bool
Flamethrower_effect_target::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode, const gpstring &sBoneName )
{
	UNREFERENCED_PARAMETER( sBoneName );
	UNREFERENCED_PARAMETER( orient_mode );

	bool bSuccess = false;

	if( pPos )
	{
		if( gWorldFx.GetPosition( (SFxEID)m_ID, *pPos ) )
		{
			if( pOrient )
			{
				if( gWorldFx.GetOrientation( (SFxEID)m_ID, *pOrient ) )
				{
					(*pPos).pos += *pOrient * m_Offset;
					return true;
				}
			}
			else
			{
				if( gWorldFx.GetOrientation( (SFxEID)m_ID, m_Orientation ) )
				{
					(*pPos).pos += m_Orientation * m_Offset;
					bSuccess = true;
				}
			}
		}
	}

	if( pOrient )
	{
		bSuccess |= gWorldFx.GetOrientation( (SFxEID)m_ID, *pOrient );
	}

	return bSuccess;
}


bool
Flamethrower_effect_target::GetDirection( vector_3 &direction )
{
	return gWorldFx.GetDirection( (SFxEID)m_ID, direction );
}


bool
Flamethrower_effect_target::GetBoundingRadius( float &radius )
{
	return gWorldFx.GetBoundingRadius( (SFxEID)m_ID, radius );
}


bool GetBaseTarget( UINT32 id, Flamethrower_target **pTarget )
{
	TattooTracker *pTracker;

	// get the targets for the effect from the effect id
	if( gWorldFx.GetTargets( SFxEID(id), &pTracker ) )
	{
		// does the first target in this tracker contain yet another effect target?
		if( pTracker->GetType() == Flamethrower_target::EFFECT )
		{
			// yes, so recurse further up the targetting tree

			Flamethrower_target *pTempTarget = NULL;
			pTracker->GetTarget( TattooTracker::TARGET, &pTempTarget );

			return GetBaseTarget( pTempTarget->GetID(), pTarget );
		}
		else
		{
			// no, base go target is found
			return pTracker->GetTarget( TattooTracker::TARGET, pTarget );
		}
	}

	return false;
}


bool
Flamethrower_effect_target::GetIsInsideInventory( void )
{
	Flamethrower_target *pTarget = NULL;

	if( GetBaseTarget( m_ID, &pTarget ) && (pTarget->GetType() != Flamethrower_target::INVALID) )
	{
		return pTarget->GetIsInsideInventory();
	}

	return false;
}
