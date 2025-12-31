//
// SimObj.cpp
//
// Contains transfer functions for save/restore game
//

#include "precomp_world.h"

#include "Sim.h"
#include "SimObj.h"

#include "ContentDb.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"
#include "gosupport.h"
#include "WorldFx.h"



bool
SimObj::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_ID",						m_ID						);
	persist.XferHex		( "m_OwnerID",					m_OwnerID					);
	persist.XferHex		( "m_AttachedID",				m_AttachedID				);
	persist.XferHex		( "m_ObjectID",					m_ObjectID					);
	persist.XferHex		( "m_IntendedTarget",			m_IntendedTarget			);

	persist.Xfer		( "m_Attached",					m_bAttached					);
	persist.Xfer		( "m_AttachLife",				m_AttachLife				);
	persist.Xfer		( "m_AttachOffset",				m_AttachOffset				);
	persist.Xfer		( "m_AttachOrientation",		m_AttachOrientation			);

	persist.Xfer		( "m_Position",					m_Position					);
	persist.Xfer		( "m_Velocity",					m_Velocity					);
	persist.Xfer		( "m_Acceleration",				m_Acceleration				);
	persist.Xfer		( "m_InitialOrientation",		m_InitialOrientation		);
	persist.Xfer		( "m_Orientation",				m_Orientation				);
	persist.Xfer		( "m_Direction",				m_Direction					);
	persist.Xfer		( "m_AngularPosition",			m_AngularPosition			);
	persist.Xfer		( "m_AngularVelocity",			m_AngularVelocity			);
	persist.Xfer		( "m_AngularAcceleration",		m_AngularAcceleration		);

	persist.Xfer		( "m_bUseTrajectoryToOrient",	m_bUseTrajectoryToOrient	);
	persist.Xfer		( "m_bCollideWithGOs",			m_bCollideWithGOs			);
	persist.Xfer		( "m_bExplodeIfHitGo",			m_bExplodeIfHitGo			);
	persist.Xfer		( "m_bExplodeIfHitTerrain",		m_bExplodeIfHitTerrain		);
	persist.Xfer		( "m_bExplodeOnExpiration",		m_bExplodeOnExpiration		);
	persist.Xfer		( "m_bPassThrough",				m_bPassThrough				);
	persist.Xfer		( "m_bShouldDestroy",			m_bShouldDestroy			);
	persist.Xfer		( "m_bObjectAtRest",			m_bObjectAtRest				);
	persist.Xfer		( "m_bInCollision",				m_bInCollision				);
	persist.Xfer		( "m_bInDeletion",				m_bInDeletion				);
	persist.Xfer		( "m_bCanDoDamage",				m_bCanDoDamage				);
	persist.XferVector	( "m_Ignorables",				m_Ignorables				);
	persist.Xfer		( "m_Gravity",					m_Gravity					);
	persist.Xfer		( "m_Mass",						m_Mass						);
	persist.Xfer		( "m_Restitution",				m_Restitution				);
	persist.Xfer		( "m_Friction",					m_Friction					);
	persist.Xfer		( "m_DeflectionAngle",			m_DeflectionAngle			);
	persist.Xfer		( "m_Duration",					m_Duration					);
	persist.Xfer		( "m_Elapsed",					m_Elapsed					);

	persist.Xfer		( "m_bIsDropSim",				m_bIsDropSim				);
	persist.Xfer		( "m_DropQuat",					m_DropQuat					);
	persist.Xfer		( "m_DropPos",					m_DropPos					);
	persist.Xfer		( "m_bPlayDropSound",			m_bPlayDropSound			);

	persist.Xfer		( "m_Attacker",					m_Attacker					);

	return true;
}


bool
SimExp::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_ID",				m_ID			);

	persist.Xfer		( "m_Elapsed",			m_Elapsed		);
	persist.Xfer		( "m_Duration",			m_Duration		);

	persist.Xfer		( "m_Radius",			m_Radius		);

	persist.Xfer		( "m_Position",			m_Position		);
	persist.Xfer		( "m_MaxRadius",		m_MaxRadius		);
	persist.Xfer		( "m_Magnitude",		m_Magnitude		);
	persist.Xfer		( "m_Damage",			m_Damage		);
	
	persist.Xfer		( "m_bDamageAll",		m_bDamageAll	);

	persist.XferVector	( "m_Ignorables",		m_Ignorables	);

	persist.Xfer		( "m_Owner",			m_Owner			);
	persist.Xfer		( "m_OwnerWeapon",		m_OwnerWeapon	);
	persist.Xfer		( "m_UnloadSignaler",	m_UnloadSignaler);

	return true;
}


bool
SimDmg::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_ID",					m_ID					);

	persist.Xfer		( "m_source",				m_source				);
	persist.Xfer		( "m_target",				m_target				);
	persist.Xfer		( "m_start_pos",			m_start_pos				);
	persist.Xfer		( "m_end_pos",				m_end_pos				);

	persist.Xfer		( "m_length",				m_length				);

	persist.Xfer		( "m_start_radius",			m_start_radius			);
	persist.Xfer		( "m_end_radius",			m_end_radius			);

	persist.Xfer		( "m_damage",				m_damage				);

	persist.Xfer		( "m_duration",				m_duration				);
	persist.Xfer		( "m_elapsed",				m_elapsed				);

	persist.Xfer		( "m_bStatic",				m_bStatic				);

	persist.XferVector	( "m_Ignorables",			m_Ignorables			);

	persist.Xfer		( "m_Owner",				m_Owner					);
	persist.Xfer		( "m_OwnerWeapon",			m_OwnerWeapon			);

	persist.Xfer		( "m_UnloadSignaler",		m_UnloadSignaler		);

	return true;
}


bool
SimDmg::GetStartPosition( SiegePos & pos )
{
	// if it's a static damage volume just return the start position
	if( m_bStatic )
	{
		if( !gSiegeEngine.IsNodeValid( m_start_pos.node ) )
		{
			// force expiration by over elapsing
			m_elapsed = m_duration + 1.0f;
			return false;
		}
	}
	else
	{
		// is a dynamic damage volume so copy position from the go
		GoHandle hStartGo( m_source );

		if( hStartGo && hStartGo->IsInAnyWorldFrustum() )
		{
			m_start_pos = hStartGo->GetPlacement()->GetPosition();
		}
		else
		{
			// start go is invalid so delete this damage volume
			m_elapsed = m_duration + 1.0f;
			return false;
		}
	}

	// copy the good position and return success
	pos = m_start_pos;
	return true;
}


bool
SimDmg::GetEndPosition( SiegePos & pos )
{
	if( m_bStatic )
	{
		if( !gSiegeEngine.IsNodeValid( m_end_pos.node ) )
		{
			// force expiration by over elapsing
			m_elapsed = m_duration + 1.0f;
			return false;
		}
	}
	else
	{
		GoHandle hEndGo( m_target );

		if( hEndGo && hEndGo->IsInAnyWorldFrustum() )
		{
			SiegePos new_pos = hEndGo->GetPlacement()->GetPosition();

			if( (m_length != 0) && (new_pos != m_end_pos) )
			{
				// Move the end position to end at the start position plus the length specified
				if( GetStartPosition( m_start_pos ) )
				{
					vector_3 direction = gSiegeEngine.GetDifferenceVector( m_start_pos, new_pos );
					direction = !IsZero(direction) ? Normalize( direction ) : vector_3( 0,-1,0 );
					
					m_end_pos = m_start_pos;

					if( !gWorldFx.AddPositions( m_end_pos, (m_length * direction), false, NULL ) )
					{
						m_elapsed = m_duration + 1.0f;
						return false;
					}
				}
				else
				{
					// start position was bad so get rid of this volume
					m_elapsed = m_duration + 1.0f;
					return false;
				}
			}
		}
		else
		{
			// start go is invalid so delete this damage volume
			m_elapsed = m_duration + 1.0f;
			return false;
		}
	}

	// copy the good position and return success
	pos = m_end_pos;
	return true;
}
