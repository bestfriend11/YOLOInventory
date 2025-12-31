//
//	Flamethrower_effects.h
//	Copyright 1999, Gas Powered Games
//
//
#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "vector_3.h"
#include "space_3.h"
#include "filter_1.h"

#include "Flamethrower_effect_base.h"
#include "Flamethrower_effects.h"
#include "WorldFx.h"
#include "Flamethrower_tracker.h"

#include "gpprofiler.h"

#include "RapiOwner.h"

#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"

#include "namingkey.h"


// Fire ---
Fire_Effect::Fire_Effect( void )
{

	m_bDarkBlending	= false;
	m_bIgnite		= false;
	m_bDamage		= false;
	m_bCheap		= false;
	m_bInstantStart = false;
	m_bLineMode		= false;
	m_bAreaMode		= false;
	m_bYOrientOnly	= false;

	m_DisplaceMin	= 0.0f;
	m_DisplaceMax	= 0.0f;
	m_FlameSize		= 0.0f;
	m_AlphaFade		= 0.0f;
	m_MinFlames		= 0.0f;
	m_MaxFlames		= 0.0f;

	m_Radius		= 0.0f;
	m_Radius_min	= 0.0f;
	m_Radius_max	= 0.0f;
	m_Radius_inc_min= 0.0f;
	m_Radius_inc_max= 0.0f;
	m_Radius_rng_min= 0.0f;
	m_Radius_rng_max= 0.0f;

	m_TimeScalar	= 0.0f;

	m_FlameCount	= 0.0f;

	m_SinSpeed		= 0.0f;
	m_SinPos		= 0.0f;

	m_LineSpeed		= 0.0f;
	m_LinePos		= 0.0f;
}


Fire_Effect::~Fire_Effect( void )
{
}


void
Fire_Effect::RegisterDerivedParams( EffectParams &params )
{
	// Register default parameters
	params.SetVector3	( "color0",			0.8f, 0.5f, 0.0f	);	// PARAM(1)
	params.SetVector3	( "color1",			0.15f, 0.0875f, 0.0f);	// PARAM(2)
	params.SetVector3	( "velocity",		0.0f, 8.0f, 0.0f	);	// PARAM(3)
	params.SetVector3	( "accel",			0.0f, 14.0f, 0.0f	);	// PARAM(4)
	params.SetVector3	( "fdamage",		3.0f, 7.0f, 0.2f	);	// PARAM(5)
	params.SetVector3	( "fctrl",			0.25, 1.25, 0.0f	);	// PARAM(6)

	params.SetFloat		( "flamesize",		1.75f				);	// PARAM(7)
	params.SetFloat		( "alphafade",		0.85f				);	// PARAM(8)
	params.SetFloat		( "count",			30.0f				);	// PARAM(9)
	params.SetFloat		( "min_count",		30.0f				);	// PARAM(10)
																
	params.SetFloat		( "radius",			0.0f				);	// PARAM(11)
	params.SetFloat		( "min_radius",		0.0f				);	// PARAM(12)
	params.SetFloat		( "max_radius",		4.0f				);	// PARAM(13)
																
	params.SetFloat		( "min_iradius",	0.0f				);	// PARAM(14)	todo: doc this
	params.SetFloat		( "max_iradius",	0.0f				);	// PARAM(15)	todo: doc this
																
	params.SetFloat		( "radius_rmin",	0.0f				);	// PARAM(16)	todo: doc this
	params.SetFloat		( "radius_rmax",	4.0f				);	// PARAM(17)	todo: doc this
																
	params.SetFloat		( "min_displace",	0.0f				);	// PARAM(18)
	params.SetFloat		( "max_displace",	1.25f				);	// PARAM(19)
																
	params.SetBool		( "ignite",			false				);	// PARAM(20)
	params.SetBool		( "damage",			false				);	// PARAM(21)
	params.SetBool		( "dark",			false				);	// PARAM(22)
	params.SetBool		( "cheap",			false				);	// PARAM(23)
	params.SetBool		( "instant",		false				);	// PARAM(24)
	params.SetBool		( "line",			false				);	// PARAM(25)
	params.SetBool		( "area",			false				);	// PARAM(26)
	params.SetBool		( "yorient",		false				);	// PARAM(27)
	params.SetBool		( "grey_tex",		false				);	// PARAM(28)
																
	params.SetFloat		( "sinspeed",		0.0f				);	// PARAM(29)	todo: doc this
	params.SetFloat		( "sinpos",			0.0f				);	// PARAM(30)	todo: doc this
																
	params.SetFloat		( "linespeed",		0.0f				);	// PARAM(31)	todo: doc this
	params.SetFloat		( "linepos",		0.0f				);	// PARAM(32)	todo: doc this
}


void
Fire_Effect::InitializeDerived( void )
{
	// Parameters
	m_Params.GetVector3	( PARAM(1),			m_Base_color		);
	m_Params.GetVector3	( PARAM(2),			m_Color_variance	);
	m_Params.GetVector3	( PARAM(3),			m_Velocity			);
	m_Params.GetVector3	( PARAM(4),			m_Acceleration		);
	m_Params.GetVector3	( PARAM(5),			m_FDamage			);

	vector_3 temp(DoNotInitialize);
	m_Params.GetVector3	( PARAM(6),			temp				);
	m_FlameSize_min		= temp.x;
	m_FlameSize_max		= temp.y;
	m_FlameSize_inc		= temp.z;
	ValidateOrder( m_FlameSize_min, m_FlameSize_max );

	m_Params.GetFloat	( PARAM(7),			m_FlameSize			);
	m_Params.GetFloat	( PARAM(8),			m_AlphaFade			);
	m_Params.GetFloat	( PARAM(9),			m_MaxFlames			);

	if( !m_Params.GetFloat( PARAM(10),		m_MinFlames			) )
	{
		m_MinFlames = m_MaxFlames;
	}

	m_Params.GetFloat	( PARAM(11),		m_Radius			);
	m_Params.GetFloat	( PARAM(12),		m_Radius_min		);
	m_Params.GetFloat	( PARAM(13),		m_Radius_max		);
	m_Params.GetFloat	( PARAM(14),		m_Radius_inc_min	);
	m_Params.GetFloat	( PARAM(15),		m_Radius_inc_max	);

	m_Params.GetFloat	( PARAM(16),		m_Radius_rng_min	);
	m_Params.GetFloat	( PARAM(17),		m_Radius_rng_max	);

	m_Params.GetFloat	( PARAM(18),		m_DisplaceMin		);
	m_Params.GetFloat	( PARAM(19),		m_DisplaceMax		);
	m_Params.GetBool	( PARAM(20),		m_bIgnite			);
	m_Params.GetBool	( PARAM(21),		m_bDamage			);
	m_Params.GetBool	( PARAM(22),		m_bDarkBlending		);
	m_Params.GetBool	( PARAM(23),		m_bCheap			);
	m_Params.GetBool	( PARAM(24),		m_bInstantStart		);

	m_Params.GetBool	( PARAM(25),		m_bLineMode			);
	m_Params.GetBool	( PARAM(26),		m_bAreaMode			);

	m_Params.GetBool	( PARAM(27),		m_bYOrientOnly		);
	m_Params.GetBool	( PARAM(28),		m_bGreyTexture		);

	m_Params.GetFloat	( PARAM(29),		m_SinSpeed			);
	m_Params.GetFloat	( PARAM(30),		m_SinPos			);
	
	m_Params.GetFloat	( PARAM(31),		m_LineSpeed			);
	m_Params.GetFloat	( PARAM(32),		m_LinePos			);
	
	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	// Figure out what the max number of particles should be depending on the detail level
	ValidateOrder( m_MinFlames, m_MaxFlames );
	const float count = m_MinFlames + (gWorldFx.GetDetailLevel() * (m_MaxFlames - m_MinFlames));
	m_MaxFlames		= (count > MAX_FLAMES) ? MAX_FLAMES : count;

	// Fix-up any min/max problems
	ValidateOrder( m_DisplaceMin,	m_DisplaceMax	);
	ValidateOrder( m_Radius_min,	m_Radius_max	);

	m_TimeScalar *= 3.5f;		// using default burn rate of 3.5f
	//

	m_FlameCount = 0.0f;

	if( !m_bInstantStart )
	{
		Flame_list	initial_list;
		initial_list.guid = m_Pos.node;
		m_Flames.push_back( initial_list );
	}

	m_bMustUpdate |= (m_bDamage | m_bIgnite);
}


bool
Fire_Effect::ReInitialize( void )
{
	m_DrawVerts.resize( 4 * (int)m_MaxFlames + 4 );

	if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		return false;
	}

	return true;
}


bool
Fire_Effect::UnInitialize( void )
{
	m_bInitialized = false;
	m_FlameCount = 0;

	m_Flames.clear();
	m_Insert.clear();

	m_DrawVerts.clear();

	return true;
}


UINT32
Fire_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		if( m_bDarkBlending )
		{
			if( m_bGreyTexture )
			{
				m_TextureName = gWorldFx.GetGlobalTexture( 5 );
			}
			else
			{
				m_TextureName = gWorldFx.GetGlobalTexture( 7 );
			}
		}
		else
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 1 );
		}
	}
	return m_TextureName;
}


FUBI_DECLARE_SELF_TRAITS( Fire_Effect::Flame );

bool
Fire_Effect::Flame::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "position",		position	);
	persist.Xfer( "velocity",		velocity	);
	persist.Xfer( "acceleration",	acceleration);
	persist.Xfer( "color",			color		);
	persist.Xfer( "alpha",			alpha		);
	persist.Xfer( "s",				s			);

	return true;
}


FUBI_DECLARE_SELF_TRAITS( Fire_Effect::Flame_list );

bool
Fire_Effect::Flame_list::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "guid",	guid	);
	persist.XferList( "flames",	flames	);

	return true;
}


bool
Fire_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_Base_color",					m_Base_color				);
	persist.Xfer	( "m_Color_variance",				m_Color_variance			);
	persist.Xfer	( "m_Acceleration",					m_Acceleration				);
	persist.Xfer	( "m_Velocity",						m_Velocity					);
	persist.Xfer	( "m_bCheap",						m_bCheap					);
	persist.Xfer	( "m_AlphaFade",					m_AlphaFade					);
	persist.Xfer	( "m_FlameSize",					m_FlameSize					);
	persist.Xfer	( "m_MinFlames",					m_MinFlames					);
	persist.Xfer	( "m_MaxFlames",					m_MaxFlames					);
	persist.Xfer	( "m_DisplaceMin",					m_DisplaceMin				);
	persist.Xfer	( "m_DisplaceMax",					m_DisplaceMax				);

	persist.Xfer	( "m_origin_dir",					m_origin_dir				);

	persist.Xfer	( "m_MinimumCorner",				m_MinimumCorner				);
	persist.Xfer	( "m_MaximumCorner",				m_MaximumCorner				);

	persist.XferList( "m_Flames",						m_Flames					);
	persist.XferList( "m_Insert",						m_Insert					);

	persist.Xfer	( "m_bInstantStart",				m_bInstantStart				);
	persist.Xfer	( "m_bLineMode",					m_bLineMode					);
	persist.Xfer	( "m_bAreaMode",					m_bAreaMode					);
	persist.Xfer	( "m_bIgnite",						m_bIgnite					);
	persist.Xfer	( "m_bDamage",						m_bDamage					);

	persist.Xfer	( "m_bDarkBlending",				m_bDarkBlending				);
	persist.Xfer	( "m_bYOrientOnly",					m_bYOrientOnly				);

	persist.Xfer	( "m_bGreyTexture",					m_bGreyTexture				);

	persist.Xfer	( "m_FlameCount",					m_FlameCount				);

	persist.Xfer	( "m_Radius",						m_Radius					);
	persist.Xfer	( "m_Radius_min",					m_Radius_min				);
	persist.Xfer	( "m_Radius_max",					m_Radius_max				);
	persist.Xfer	( "m_Radius_inc_min",				m_Radius_inc_min			);
	persist.Xfer	( "m_Radius_inc_max",				m_Radius_inc_max			);
	persist.Xfer	( "m_Radius_rng_min",				m_Radius_rng_min			);
	persist.Xfer	( "m_Radius_rng_max",				m_Radius_rng_max			);

	persist.Xfer	( "m_SinPos",						m_SinPos					);
	persist.Xfer	( "m_SinSpeed",						m_SinSpeed					);

	persist.Xfer	( "m_LinePos",						m_LinePos					);
	persist.Xfer	( "m_LineSpeed",					m_LineSpeed					);

	persist.Xfer	( "m_FDamage",						m_FDamage					);

	persist.Xfer	( "m_FlameSize_min",				m_FlameSize_min				);
	persist.Xfer	( "m_FlameSize_max",				m_FlameSize_max				);
	persist.Xfer	( "m_FlameSize_inc",				m_FlameSize_inc				);

	return true;
}


void
Fire_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = ReInitialize();
		if( !m_bInitialized )
		{
			return;
		}

		if( !m_Targets->GetBoundingRadius( m_BoundingRadius ) )
		{
			m_BoundingRadius = 1.5f;
		}

		if( m_bAbsoluteCoords )
		{
			m_Scale = m_ScaleFactor = 1.0f;
		}
		else
		{
			m_Scale = (m_BoundingRadius/11.091f) * m_ScaleFactor;
		}
	}

	// Declare storage for the first list of flames that will be used
	Flame_list	initial_list;
	// The flames will start in the current node
	initial_list.guid = m_Pos.node;

	// Create an entire flame volume if specified right from the start
	// If the instant() param was specified for this effect then spawn max_flames
	// in the appropriate positions using a 20hz timestep
	if( m_bInstantStart )
	{
		// We don't ever want to do this again so flag it
		m_bInstantStart = false;

		// Before we get into the main loop set our flame count and initialize the
		// pseudo frame sync value to zero - this will be the delta t value
		m_FlameCount = m_MaxFlames;
		float PseudoSync = 0;

		// Starting at zero create each flame through the call CreateNewFlame
		for( int i = 0; i < m_MaxFlames; ++i )
		{
			// Instantiate a flame
			Flame	flame;
			if( CreateNewFlame( flame, PseudoSync ) )
			{
				// Simulate forward by 20hz
				PseudoSync += 0.05f;

				if( flame.position.node == initial_list.guid )
				{
					initial_list.flames.push_back( flame );
				}
				else
				{
					bool inserted = false;

					// Check through the list of insert lists to see if one
					// exists already for the node this flame belongs in
					FlameLColl::iterator insert = m_Insert.begin(), insert_end = m_Insert.end();
					for(; insert != insert_end; ++insert )
					{
						if( (*insert).guid == flame.position.node )
						{
							// Found a match so insert into this one
							(*insert).flames.push_back( flame );
							inserted = true;
							break;
						}
					}
					if( inserted == false )
					{
						// Flame was never inserted into any existing flame lists so
						// create a new flame list with the new flame and put it there
						Flame_list	new_list;
						new_list.guid = flame.position.node;
						new_list.flames.push_back( flame );
						m_Insert.push_back( new_list );
					}
				}
			}
		}

		// Push the initial list
		m_Flames.push_back( initial_list );

		// Now go through the list of insert lists and push them
		FlameLColl::iterator iList = m_Insert.begin();
		for(; iList != m_Insert.end(); ++iList )
		{
			m_Flames.push_back( *iList );
		}
		m_Insert.clear();
	}



	// Normal flame creation/destruction

	// Determine the number of flames that we should be processing depending on
	// whether or not we are starting or ending a simulation
	INT32	flames_to_process = (INT32)m_FlameCount;
	bool spawn_new_flames = false;

	if( m_bFinishing )
	{
		m_FlameCount -= 12.5f * m_FrameSync;
		if( m_FlameCount < 0.0f )
		{
			m_FlameCount = 0.0f;
			EndThisEffect();
			return;
		}
	}
	else
	{
		m_FlameCount += 12.5f * m_FrameSync;
		if( m_FlameCount >= m_MaxFlames )
		{
			m_FlameCount = m_MaxFlames;
		}
		spawn_new_flames = true;
	}

	flames_to_process = (INT32)m_FlameCount - flames_to_process;

	if( spawn_new_flames )
	{
		// Depending on the flame count - create the appropriate number of flames
		for( int flame_count = flames_to_process; flame_count > 0; --flame_count )
		{
			// Create a flame
			Flame flame;
			if( CreateNewFlame( flame, m_FrameSync ) )
			{
				bool bInserted = false;
				// Try and find a match with the existing list of nodes
				FlameLColl::iterator existing = m_Flames.begin(), exist_end = m_Flames.end();
				for(; existing != exist_end; ++existing )
				{
					if( flame.position.node == (*existing).guid )
					{
						(*existing).flames.push_back( flame );
						bInserted = true;
						break;
					}
				}
				// If the flame was never inserted then create a new list with
				// that flames node and push it back.
				if( !bInserted )
				{
					Flame_list	new_list;
					new_list.guid = flame.position.node;
					new_list.flames.push_back( flame );
					m_Flames.push_back( new_list );
				}
			}
		}
	}
	else
	{
		// We aren't spawning any new flames so just go through the current flame
		// lists and erase those flames that aren't that visible anymore
		FlameLColl::iterator kill = m_Flames.begin();
		for(; kill != m_Flames.end(); ++kill )
		{
			FlameColl::iterator iFlame = (*kill).flames.begin(), flame_end = (*kill).flames.end();
			while( iFlame != flame_end )
			{
				// If there is no alpha then delete it
				if( (*iFlame).alpha <= 0 )
				{
					// Erase the flame and continue
					iFlame = (*kill).flames.erase( iFlame );
					flame_end = (*kill).flames.end();

					--flames_to_process;
					if( flames_to_process == 0 )
					{
						goto $1; // Goto is good for skipping closing braces
					}
				}
				else
				{
					++iFlame;
				}
			}
		}
	}


$1:	// Main update state

	// Reset where the flame bounds are at
	m_MinimumCorner = vector_3( RealMaximum, RealMaximum, RealMaximum );
	m_MaximumCorner = vector_3( -RealMaximum, -RealMaximum, -RealMaximum );

	bool bVolumeBuilt = false;

	// Go through the list of flame lists
	FlameLColl::iterator uList = m_Flames.begin();
	for(; uList != m_Flames.end(); ++uList )
	{
		// If not in a valid node skip this list
		if( !gWorldFx.IsNodeVisible( (*uList).guid ) )
		{
			continue;
		}

		// Go through the list of flames
		FlameColl::iterator uFlame = (*uList).flames.begin(), uFlame_end = (*uList).flames.end();
		while( uFlame != uFlame_end )
		{
			// The alpha value is what determines the life of the flame
			float newAlpha = (*uFlame).alpha - m_AlphaFade * m_FrameSync;

			// Insert new flame
			if( (newAlpha <= 0) && !m_bFinishing )
			{
				// Create a new flame
				Flame flame;
				if( CreateNewFlame( flame, m_FrameSync ) )
				{
					// If the newly create flame is in the same node as the old replace it
					if( flame.position.node == (*uList).guid )
					{
						(*uFlame) = flame;
					}
					else
					{
						// Search through the list of flame lists and try to
						// find a match
						bool bInserted = false;
						FlameLColl::iterator iInsert = m_Insert.begin();
						for(; iInsert != m_Insert.end(); ++iInsert )
						{
							if( flame.position.node == (*iInsert).guid )
							{
								// Match found - we are done
								(*iInsert).flames.push_back( flame );
								bInserted = true;
								break;
							}
						}
						// If the flame was never inserted then create a new
						// flame list for that guid and insert the flame
						if( !bInserted )
						{
							Flame_list	new_list;
							new_list.guid = flame.position.node;
							new_list.flames.push_back( flame );
							m_Insert.push_back( new_list );
						}

						// Remove current flame from current list
						uFlame = (*uList).flames.erase( uFlame );
						uFlame_end = (*uList).flames.end();
						continue;
					}
				}
				++uFlame;
			}
			else
			{	// Update old flames

				// Update particle size
				if( m_FlameSize_inc > 0 )
				{
					(*uFlame).s += (1.0f - (*uFlame).alpha) * m_FlameSize_inc * m_FrameSync;
				}

				// Animate spawn radii
				if( m_SinSpeed > 0 )
				{
					m_SinPos += m_SinSpeed * m_FrameSync;

					if( m_SinPos > PI )
					{
						m_SinPos -= PI;
					}

					m_Radius = m_Radius_max * SINF( m_SinPos );
				}
				else
				{
					if( m_Radius_inc_min != 0 )
					{
						m_Radius_min += m_Radius_inc_min * m_FrameSync;
					}
					if( m_Radius_inc_max != 0 )
					{
						m_Radius_max += m_Radius_inc_max * m_FrameSync;
					}
				}

				// Set the translucency
				(*uFlame).alpha = newAlpha;

				siege::database_guid old_node( (*uFlame).position.node );

				// Determine new position and velocity using standard kinematic equations of motion
				const vector_3 displacement = ( 0.5f * m_FrameSync * m_FrameSync * (*uFlame).acceleration )
											+ (*uFlame).velocity * m_FrameSync;

				bool bNodeChange = false;

				if( !gWorldFx.AddPositions( (*uFlame).position, displacement, m_bFast, &bNodeChange ) )
				{
					continue;
				}

				if( bNodeChange )
				{
					matrix_3x3 orient_diff;

					gWorldFx.GetDifferenceOrient( old_node, (*uFlame).position.node, orient_diff );

					(*uFlame).velocity = orient_diff * (*uFlame).velocity;
					(*uFlame).acceleration = orient_diff * (*uFlame).acceleration;
				}

				(*uFlame).velocity += (*uFlame).acceleration * m_FrameSync;

				// If the recently moved flame entered a new node then transfer it
				if( (*uFlame).position.node != (*uList).guid )
				{
					// Search through the list of flame lists and try to
					// find a match
					bool bInserted = false;
					FlameLColl::iterator iInsert = m_Insert.begin();
					for(; iInsert != m_Insert.end(); ++iInsert )
					{
						if( (*uFlame).position.node == (*iInsert).guid )
						{
							// Match found - we are done
							(*iInsert).flames.push_back( *uFlame );
							bInserted = true;
							break;
						}
					}

					// If the flame was never inserted then create a new
					// flame list for that guid and insert the flame
					if( !bInserted )
					{
						Flame_list	new_list;
						new_list.guid = (*uFlame).position.node;
						new_list.flames.push_back( *uFlame );
						m_Insert.push_back( new_list );
					}

					// First get rid of the flame from the current list
					uFlame = (*uList).flames.erase( uFlame );
					uFlame_end = (*uList).flames.end();					
					continue;
				}
				// Update the flame bounding volume
				bVolumeBuilt = AddFlameToBounds( *uFlame );

				++uFlame;
			}
		}
	}



	// Now that we know all the nodes - sort the flame list requests
	FlameLColl::iterator iList = m_Insert.begin(), insert_end = m_Insert.end();
	for(; iList != insert_end; ++iList )
	{
		// Compare the flame lists in the insertion list with the main
		// list of all flame lists and try to find a match
		bool bList_exists = false;
		FlameLColl::iterator iExisting = m_Flames.begin();
		for(; iExisting != m_Flames.end(); ++iExisting )
		{
			if( (*iExisting).guid == (*iList).guid )
			{
				FlameColl::iterator iNewFlame = (*iList).flames.begin();
				for(; iNewFlame != (*iList).flames.end(); ++iNewFlame )
				{
					// Found an existing list so insert the flame here
					(*iExisting).flames.push_back( *iNewFlame );
				}
				bList_exists = true;
			}
		}
		// There was no list so just push it on our master list
		if( !bList_exists )
		{
			m_Flames.push_back( *iList );
		}
	}
	m_Insert.clear();



	// Check to see if anything should be ignited on fire - only the server is allowed to light stuff on fire
	if( (m_bDamage || m_bIgnite ) && bVolumeBuilt && gWorldFx.IsServerLocal() )
	{
		SiegePos BoxPos( m_Pos );

		const vector_3 HalfDiag	(0.5f * (m_MaximumCorner - m_MinimumCorner) );
		const vector_3 Center	(0.5f * (m_MaximumCorner + m_MinimumCorner) );

		if( gWorldFx.AddPositions( BoxPos, Center, false, NULL ) )
		{
			gWorldFx.DamageWithinBox( BoxPos, matrix_3x3::IDENTITY, HalfDiag, m_FDamage.x, m_FDamage.y,
											m_bIgnite, (m_FrameSync / m_TimeScalar), m_bIgnite, GetFriendly(), m_OwnerID );
		}
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


bool
Fire_Effect::CreateNewFlame( Flame &flame, const float delta_t )
{
	// Using the orientation of the target the velocity and acceleration
	// must oriented to that space
	matrix_3x3	orientation( m_Orient );

	// Origin is at the target position
	SiegePos	origin_position( m_Pos );

	// If a line or area parameter was specified then two targets must be used to determine
	// where the flame is going to appear at
	if( m_bLineMode || m_bAreaMode )
	{
		SiegePos	point_a, point_b;
		if( (m_Targets->GetPlacement( &point_a, NULL, m_OrientMode, TattooTracker::TARGET )) &&
			(m_Targets->GetPlacement( &point_b, NULL, m_OrientMode, TattooTracker::SOURCE )) )
		{
			// check for equality between two new fresh points
			if( point_a.IsExactlyEqual( point_b ) )
			{
				m_origin_dir = vector_3::UP;
			}
			else
			{
				// Determine the direction this effect will be pointing to
				m_origin_dir = gSiegeEngine.GetDifferenceVector( point_a, point_b );
			}

			// Setup origin and orientation
			origin_position	= point_a;
			orientation		= MatrixFromDirection( m_origin_dir );
		}
	}

	// Determine random flame position based on spawn mode
	if( m_bLineMode )
	{
		// If line mode just use the direction as a 'slope' to draw on
		if( m_LineSpeed != 0 )
		{
			m_LinePos += m_LineSpeed * m_FrameSync;

			if( m_LinePos > 1.0f )
			{
				FilterClamp( 0.0f, 1.0f, m_LinePos );
				m_bFinishing = true;
			}
		}
		else
		{
			m_LinePos = gWorldFx.GetRNG().RandomFloat();
		}

		flame.position.pos = m_origin_dir * m_LinePos;
	}
	else if( m_bAreaMode )
	{
		// Within the rect defined by the two targets pick a random unit point in
		// that plane to originate from...
		flame.position.pos.x = gWorldFx.GetRNG().RandomFloat();
		flame.position.pos.y = gWorldFx.GetRNG().RandomFloat();
		flame.position.pos.z = gWorldFx.GetRNG().RandomFloat();

		// and scale it.
		flame.position.pos *= m_origin_dir;
	}
	else
	{
		// Get a random spawn angleq
		const float a = gWorldFx.GetRNG().Random( PI2 );

		// Neither mode was specified so use the default origination mode which is a point in a circle
		float r = gWorldFx.GetRNG().Random( m_Radius_min, m_Radius_max ) * m_Scale + m_Radius;

		// restrict the radii
		if( r > m_Radius_rng_max )
		{
			r = m_Radius_rng_max;
		}
		else if( r < m_Radius_rng_min )
		{
			r = m_Radius_rng_min;
		}
		
		if( r < m_Radius_rng_min )
		{
			r = m_Radius_rng_min;
		}
		else if( r > m_Radius_rng_max )
		{
			r = m_Radius_rng_max;
		}

		SINCOSF( a, flame.position.pos.z, flame.position.pos.x );
		flame.position.pos.x *= r;
		flame.position.pos.y = r * gWorldFx.GetRNG().Random( m_DisplaceMin, m_DisplaceMax );
		flame.position.pos.z *= r;
	}

	flame.position.pos += origin_position.pos;
	flame.position.node = origin_position.node;

	vector_3 acceleration	= orientation * m_Acceleration;
	vector_3 velocity		= orientation * m_Velocity;

	// If we don't want to pay attention to the wind then don't
	if( m_bWindAffected )
	{
		vector_3 wind_vector( DoNotInitialize );
		gWorldFx.GetNodeWindVector( m_Pos.node, wind_vector );

		velocity += 5.0f * wind_vector;
	}

	// Take scale into account so that we can use the same number of polygons
	// for any size of fire
	flame.acceleration	= acceleration * m_Scale;
	flame.velocity		= velocity * gWorldFx.GetRNG().RandomFloat() * m_Scale;

	// Determine the staring alpha value
	flame.alpha = 1.0f;

	// Give this flame some random size
	flame.s = gWorldFx.GetRNG().Random( m_FlameSize_min, m_FlameSize_max );

	// Set the individual flame color using some color variance
	flame.color = m_Base_color;

	flame.color.x += gWorldFx.GetRNG().Random( -m_Color_variance.x, m_Color_variance.x );
	flame.color.y += gWorldFx.GetRNG().Random( -m_Color_variance.y, m_Color_variance.y );
	flame.color.z += gWorldFx.GetRNG().Random( -m_Color_variance.z, m_Color_variance.z );

	flame.color.x = FilterClamp( 0.0f, 1.0f, flame.color.x );
	flame.color.y = FilterClamp( 0.0f, 1.0f, flame.color.y );
	flame.color.z = FilterClamp( 0.0f, 1.0f, flame.color.z );

	// Anti-poof tech - start within a random delta t unit
	const float td = delta_t * gWorldFx.GetRNG().RandomFloat();
	flame.alpha = 1.0f - ( m_AlphaFade * td );

	siege::database_guid old_node = flame.position.node;

	bool bNodeChange = false;

	if( !gWorldFx.AddPositions( flame.position, ( 0.5f * td * td * flame.acceleration ) + flame.velocity * td, m_bFast, &bNodeChange ) )
	{
		// failed to create new flame
		return false;
	}

	// Determine new position and velocity using standard kinematic equations of motion
	if( bNodeChange )
	{
		matrix_3x3 diff_orient;
		gWorldFx.GetDifferenceOrient( old_node, flame.position.node, diff_orient );
		
		flame.velocity = diff_orient * flame.velocity;
		flame.acceleration = diff_orient * flame.acceleration;
	}

	flame.velocity += flame.acceleration * td;

	return true;
}


void
Fire_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}


	if( m_bDarkBlending )
	{
		gWorldFx.GetRenderer().SetTextureStageState(	0,
										D3DTOP_ADD,
										D3DTOP_MODULATE,

										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );
	}


	vector_3 pre_x( m_Scale * m_FlameSize * m_Vx );
	vector_3 pre_y;

	if( m_bCheap && m_bYOrientOnly )
	{
		pre_y = m_Scale * m_FlameSize * vector_3::UP;
	}
	else
	{
		pre_y = m_Scale * m_FlameSize * m_Vy;
	}

	// Draw all of the flames in all of the lists
	FlameLColl::iterator iDraw = m_Flames.begin(), iEnd = m_Flames.end();
	for(; iDraw != iEnd; ++iDraw )
	{
		// Make sure this is in a valid node otherwise abort this effect
		if( !gWorldFx.IsNodeVisible( (*iDraw).guid ) )
		{
			continue;
		}

		// All of the flames have been sorted by the node guid so we
		// only need to do the setup work once per list of flames
		SetDrawMatrix( (*iDraw).guid );
		ZeroMemory( &*m_DrawVerts.begin(), sizeof( sVertex ) * m_DrawVerts.size() );
	

		int flame_counter = 0;

		vector_3 vp0, vp1, vp2, vp3, vpp0, vpp1, vpp2, vpp3;

		if( m_bCheap )
		{
			vpp0 = (-pre_x + pre_y );
			vpp1 = (-pre_x - pre_y );
			vpp2 = ( pre_x + pre_y );
			vpp3 = ( pre_x - pre_y );
		}

		bool	draw_flames = false;

		// Now go through the actual list of flames and draw
		FlameColl::iterator iFlame = (*iDraw).flames.begin();
		for(; iFlame != (*iDraw).flames.end(); ++iFlame )
		{
			float const alpha = (*iFlame).alpha*m_NodeAlpha;
			if( alpha <= 0 ) continue;

			draw_flames = true;
			vector_3 const &position = (*iFlame).position.pos;
			float const s = (*iFlame).s;

			if( m_bCheap )
			{
				vp0 = position + vpp0;
				vp1 = position + vpp1;
				vp2 = position + vpp2;
				vp3 = position + vpp3;
			}
			else
			{
				const float		wh = m_FlameSize + alpha + s;
				const vector_3	whVx( m_Scale * wh*m_Vx );
				vector_3 whVy;

				if( m_bYOrientOnly )
				{
					whVy = m_Scale * wh*vector_3::UP;
				}
				else
				{
					whVy = m_Scale * wh*m_Vy;
				}
				vp0 = position + (-whVx +whVy);
				vp1 = position + (-whVx -whVy);
				vp2 = position + ( whVx +whVy);
				vp3 = position + ( whVx -whVy);
			}

			DWORD color	= MAKEDWORDCOLOR( (*iFlame).color );
			color		= ((color&0x00FFFFFF)|((BYTE(alpha*255.0f))<<24));

			int j = flame_counter * 4;
			++flame_counter;

			sVertex* verts = &*m_DrawVerts.begin();

			verts[j+0].x		= vp0.x;	verts[j+1].x		= vp1.x;
			verts[j+0].y		= vp0.y;	verts[j+1].y		= vp1.y;
			verts[j+0].z		= vp0.z;	verts[j+1].z		= vp1.z;
			verts[j+0].color	= color;	verts[j+1].color	= color;
			verts[j+0].uv.u		= 0.0f;		verts[j+1].uv.u		= 0.0f;
			verts[j+0].uv.v		= 1.0f;		verts[j+1].uv.v		= 0.0f;

			verts[j+2].x		= vp2.x;	verts[j+3].x		= vp3.x;
			verts[j+2].y		= vp2.y;	verts[j+3].y		= vp3.y;
			verts[j+2].z		= vp2.z;	verts[j+3].z		= vp3.z;
			verts[j+2].color	= color;	verts[j+3].color	= color;
			verts[j+2].uv.u		= 1.0f;		verts[j+3].uv.u		= 1.0f;
			verts[j+2].uv.v		= 1.0f;		verts[j+3].uv.v		= 0.0f;
		}

		if( draw_flames )
		{
			gWorldFx.GetRenderer().DrawIndexedPrimitive( D3DPT_TRIANGLELIST, &*m_DrawVerts.begin(),
				flame_counter * 4,	SVERTEX, gWorldFx.GetGlobalDrawIndexArray(),
				flame_counter * 6, &m_TextureName, 1 );
		}
	}

	if( m_bDarkBlending )
	{
		gWorldFx.SaveRenderingState();
	}
}

void
Fire_Effect::Track( void )
{
	SiegePos	targetPos;
	bool bNodeChange = false;

	if( m_Targets->GetPlacement( &targetPos, &m_Orient, m_OrientMode ) )
	{
		if( !m_Offset.IsZero() && !gWorldFx.AddPositions( targetPos, (m_Orient * m_Offset), m_bFast, &bNodeChange ) )
		{
			EndThisEffect();
			return;
		}

		if( bNodeChange )
		{
			matrix_3x3 diff_orient;

			gWorldFx.GetDifferenceOrient( targetPos.node, m_Pos.node, diff_orient );

			m_Acceleration	= diff_orient * m_Acceleration;
			m_Velocity		= diff_orient * m_Velocity;
			m_Orient		= diff_orient * m_Orient;
		}

		m_bHasMoved = ( m_bDamage && ( m_Pos != targetPos ) );

		m_Pos = targetPos;

		m_Targets->GetBoundingRadius( m_BoundingRadius );
		m_Scale = (m_BoundingRadius/11.091f) * m_ScaleFactor;

	}
	else
	{
		EndThisEffect();
	}
}



bool
Fire_Effect::AddFlameToBounds( Flame const &flame )
{
	if( flame.alpha >= 0.15f )
	{
		vector_3 Point( gSiegeEngine.GetDifferenceVector( m_Pos, flame.position ) );

		m_MinimumCorner.x = min_t( Point.x, m_MinimumCorner.x );
		m_MinimumCorner.y = min_t( Point.y, m_MinimumCorner.y );
		m_MinimumCorner.z = min_t( Point.z, m_MinimumCorner.z );

		m_MaximumCorner.x = max_t( Point.x, m_MaximumCorner.x );
		m_MaximumCorner.y = max_t( Point.y, m_MaximumCorner.y );
		m_MaximumCorner.z = max_t( Point.z, m_MaximumCorner.z );

		return true;
	}
	return false;
}


float
Fire_Effect::GetBoundingRadius( void )
{
	return Length((m_MaximumCorner + m_MinimumCorner) * 0.5f);
}


bool
Fire_Effect::GetCullPosition( SiegePos & CullPosition )
{
	if( gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		CullPosition = m_Pos;

		return ( gWorldFx.AddPositions( CullPosition, (0.5f * (m_MaximumCorner + m_MinimumCorner)), false, NULL ) );
	}
	return false;
}





// Trackball ---
Trackball_Effect::Trackball_Effect( void )
{
	m_Max_velocity			= 0;
	m_Acceleration			= 0;
	m_RVel_min				= 0;
	m_RVel_max				= 0;
	m_Velocity				= 0;
	m_Theta					= 0;
	m_bInit					= false;
	m_bCollided				= false;
	m_bHomingEnabled		= false;
	m_bDampingEnabled		= false;
	m_DamperSetting			= 0;
	m_SpiralRadius			= 0;
	m_SpinRate				= 0;
	m_bInvisible			= false;
	m_AfterLife				= 0;
	m_bCalcOrient			= false;
	m_bNoCollision			= false;
	m_bNoCollisionObjects	= false;
	m_bHitTargetOnly		= false;
}


void
Trackball_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",				1.0f, 0.0f, 0.0f );	// PARAM+1

	params.SetFloat		( "radius",				0.0f	);	// PARAM+2
	params.SetFloat		( "kdamp",				0.45f	);	// PARAM+3
	params.SetFloat		( "spin",				0.0f	);	// PARAM+4
	params.SetFloat		( "theta",				0.0f	);	// PARAM+5
	params.SetFloat		( "accel",				2.5f	);	// PARAM+6
	params.SetFloat		( "velocity",			5.0f	);	// PARAM+7
	params.SetFloat		( "max_velocity",		80.0f	);	// PARAM+8
	params.SetFloat		( "rvel_min",			0.0f	);	// PARAM+9
	params.SetFloat		( "rvel_max",			0.0f	);	// PARAM+10
	params.SetFloat		( "afterlife",			0.0f	);	// PARAM+11

	params.SetBool		( "homing",				false	);	// PARAM+12
	params.SetBool		( "damped",				false	);	// PARAM+13
	params.SetBool		( "invisible",			false	);	// PARAM+14
	params.SetBool		( "calc_orient",		false	);	// PARAM+15

	params.SetBool		( "no_collide",			false	);	// PARAM+16
	params.SetBool		( "no_collide_objects",	false	);	// PARAM+17

	params.SetBool		( "hit_target_only",	false	);	// PARAM+18

	// Overrides
	params.SetFloat		( "scale",				1.0f	);	// 6
}


void
Trackball_Effect::InitializeDerived( void )
{
	// Parameters
	m_Params.GetVector3	( PARAM(1),			m_Base_color			);

	m_Params.GetFloat	( PARAM(2),			m_SpiralRadius			);
	m_Params.GetFloat	( PARAM(3),			m_DamperSetting			);
	m_Params.GetFloat	( PARAM(4),			m_SpinRate				);
	m_Params.GetFloat	( PARAM(5),			m_Theta					);
	m_Params.GetFloat	( PARAM(6),			m_Acceleration			);
	m_Params.GetFloat	( PARAM(7),			m_Velocity				);
	m_Params.GetFloat	( PARAM(8),			m_Max_velocity			);
	m_Params.GetFloat	( PARAM(9),			m_RVel_min				);
	m_Params.GetFloat	( PARAM(10),		m_RVel_max				);
	m_Params.GetFloat	( PARAM(11),		m_AfterLife				);

	m_Params.GetBool	( PARAM(12),		m_bHomingEnabled		);
	m_Params.GetBool	( PARAM(13),		m_bDampingEnabled		);
	m_Params.GetBool	( PARAM(14),		m_bInvisible			);
	m_Params.GetBool	( PARAM(15),		m_bCalcOrient			);

	m_Params.GetBool	( PARAM(16),		m_bNoCollision			);
	m_Params.GetBool	( PARAM(17),		m_bNoCollisionObjects	);

	m_Params.GetBool	( PARAM(18),		m_bHitTargetOnly		);

	m_Params.GetFloat	( 6,				m_Scale					);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_DamperSetting = 1.0f - FilterClamp( 0.0f, 1.0f, m_DamperSetting );
	m_Velocity += gWorldFx.GetRNG().Random( m_RVel_min, m_RVel_max );


	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	m_bMustUpdate = true;
}


UINT32
Trackball_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}
	return m_TextureName;
}


bool
Trackball_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_RVel_min",				m_RVel_min				);
	persist.Xfer( "m_RVel_max",				m_RVel_max				);
	persist.Xfer( "m_Max_velocity",			m_Max_velocity			);
	persist.Xfer( "m_Acceleration",			m_Acceleration			);
	persist.Xfer( "m_Velocity",				m_Velocity				);
											
	persist.Xfer( "m_Theta",				m_Theta					);
											
	persist.Xfer( "m_DisplayPos",			m_DisplayPos			);
	persist.Xfer( "m_Heading",				m_Heading				);
											
	persist.Xfer( "m_Init",					m_bInit					);
	persist.Xfer( "m_Collided",				m_bCollided				);
	persist.Xfer( "m_bHomingEnabled",		m_bHomingEnabled		);
	persist.Xfer( "m_bDampingEnabled",		m_bDampingEnabled		);
	persist.Xfer( "m_DamperSetting",		m_DamperSetting			);
	persist.Xfer( "m_SpiralRadius",			m_SpiralRadius			);
	persist.Xfer( "m_SpinRate",				m_SpinRate				);
	persist.Xfer( "m_SpiralOffset",			m_SpiralOffset			);
	persist.Xfer( "m_Base_color",			m_Base_color			);
	persist.Xfer( "m_Diff_vector",			m_Diff_vector			);
	persist.Xfer( "m_bInvisible",			m_bInvisible			);
	persist.Xfer( "m_AfterLife",			m_AfterLife				);

	persist.Xfer( "m_AngleOffset",			m_AngleOffset			);

	persist.Xfer( "m_bNoCollision",			m_bNoCollision			);
	persist.Xfer( "m_bNoCollisionObjects",	m_bNoCollisionObjects	);

	persist.Xfer( "m_bCalcOrient",			m_bCalcOrient			);
	persist.Xfer( "m_bHitTargetOnly",		m_bHitTargetOnly		);

	persist.Xfer( "m_TargetPos",			m_TargetPos				);
	persist.Xfer( "m_LastPos",				m_LastPos				);
	
	return true;
}


void
Trackball_Effect::Put( void )
{
	Rapi&	renderer = gWorldFx.GetRenderer();
	renderer.TranslateWorldMatrix( m_DisplayPos.pos );
}


void
Trackball_Effect::Track( void )
{	
	SiegePos new_pos;

	// Determine whether or not this effect should still be active or not
	if( m_bFinishing || ( m_bCollided == true ) || ( !m_Targets->GetPlacement( &new_pos, NULL ) ) )
	{
		EndThisEffect();
		return;
	}

	m_bHasMoved = ( m_TargetPos != new_pos );

	m_TargetPos = new_pos;

	// Determine the initial direction for trackball
	if( !m_bInit )
	{	
		// Make sure m_Pos gets initialized )
		if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
		{
			m_Pos = m_TargetPos;
			m_Pos.pos.y += 0.05f;
		}

		if( !m_Dir.IsZero() )
		{
			m_Heading = Normalize( m_Dir );
		}
		else
		{
			// same position? then will get zero vector - return
			if( m_Pos.IsExactlyEqual( m_TargetPos ) )
			{
				return;
			}

			// same node? check if postions are same - then return
			if( m_Pos.node == m_TargetPos.node )
			{
				const vector_3 diff( m_Pos.pos - m_TargetPos.pos );
				// same position within tolerance - return
				if( diff.Length() < FLOAT_TOLERANCE )
				{
					return;
				}
			}

			// Woo!!! zero vector can STILL occur even if:
			// ( (m_Pos.node != m_TargetPos.node) && (m_Pos.pos != m_TargetPos.pos) )
			// and their offsets relative to their nodes lie outside and overlap at the same point

			m_Diff_vector = gSiegeEngine.GetDifferenceVector( m_Pos, m_TargetPos );
			m_Heading = Normalize( m_Diff_vector );
		}
		m_bInit = true;
	} 	// If homing is enabled then adjust the heading based on the target position
	else if( m_bHomingEnabled )
	{
		if( m_Pos.IsExactlyEqual( m_TargetPos ) )
		{
			return;
		}

		// same node? check if postions are same - then return
		if( m_Pos.node == m_TargetPos.node )
		{
			const vector_3 diff( m_Pos.pos - m_TargetPos.pos );
			// same position within tolerance - return
			if( diff.Length() < FLOAT_TOLERANCE )
			{
				return;
			}
		}

		// ok make sure they are visible nodes
		if( !gWorldFx.IsNodeVisible( m_Pos.node ) || !gWorldFx.IsNodeVisible( m_TargetPos.node ) )
		{
			return;
		}

		// Woo!!! zero vector can STILL occur even if:
		// ( (m_Pos.node != m_TargetPos.node) && (m_Pos.pos != m_TargetPos.pos) )
		// and their offsets relative to their nodes lie outside and overlap at the same point

		m_Diff_vector = gSiegeEngine.GetDifferenceVector( m_Pos, m_TargetPos );

		const vector_3	last_heading( m_Heading );
		const vector_3	new_heading( Normalize( m_Diff_vector ) );

		if( m_bDampingEnabled )
		{
			vector_3	damped_heading( new_heading - last_heading );
			m_Heading =	last_heading + damped_heading * m_DamperSetting * m_FrameSync;
		}
		else
		{
			m_Heading =	new_heading;
		}
	}

	// Compute position from velocity and acceleration
	m_LastPos = m_Pos;

	if( (m_Delay == 0) && (m_Velocity <= m_Max_velocity) )
	{
		m_Velocity += m_FrameSync * m_Acceleration;
	}

	const float td = (( 0.5f * m_FrameSync*m_FrameSync * m_Acceleration ) + m_FrameSync * m_Velocity );

	if( m_Delay == 0 )
	{
		bool bNodeChange = false;

		if( !gWorldFx.AddPositions( m_Pos, (td * m_Heading), false, &bNodeChange ) )
		{
			EndThisEffect();
			return;
		}

		if( bNodeChange )
		{
			gWorldFx.ReOrientVector( m_LastPos.node, m_Pos.node, m_Heading );
		}
	}

	m_DisplayPos = m_Pos;

	// If there was a spiral radius set then make the trackball spiral around
	if( m_SpiralRadius > 0  )
	{
		const float target_distance = Length( gSiegeEngine.GetDifferenceVector( m_Pos, m_TargetPos ) );
		const vector_3 v( CrossProduct( vector_3::UP, m_Heading ) );
		const vector_3 w( CrossProduct( m_Heading, v ) );
		const float rfactor = m_SpiralRadius * Length( gSiegeEngine.GetDifferenceVector( m_Pos, m_TargetPos ) ) / target_distance;

		float thetasin, thetacos;
		SINCOSF( m_Theta, thetasin, thetacos );
		m_SpiralOffset	= ( rfactor*thetacos*v + rfactor*thetasin*w );

		if( m_Delay == 0 )
		{
			m_Theta			+= m_SpinRate * td;
			m_DisplayPos.pos+= m_SpiralOffset;
		}
	}

	// Create the orientation for this trackball if specified for other effects that will target it
	if( m_bCalcOrient )
	{
		m_Orient = MatrixFromDirection( m_Heading );
	}

	// Check for a collision and handle one if it comes up
	SiegePos	ContactNormal;
	Goid  		ContactGUID;

	const float	size = 0.75f * m_Scale;


	if( (m_Delay == 0 ) && !m_bNoCollision )
	{
		if( gWorldFx.CheckForCollision( m_bNoCollisionObjects, m_LastPos, m_DisplayPos, size, size, GetFriendly(), ContactGUID, m_Pos, ContactNormal, m_OwnerID ) )
		{
			if( m_bHitTargetOnly )
			{
				if( ( ContactGUID == m_Targets->GetID() ) )
				{
					m_bCollided = true;
				}
			}
			else
			{
				m_bCollided = true;
			}

			if( m_bCollided )
			{
				PostCollision( ContactGUID, m_Pos, ContactNormal, m_Heading );
				m_DisplayPos	= m_Pos;
				if( m_AfterLife == 0 )
				{
					EndThisEffect();
				}
			}
		}
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}


void
Trackball_Effect::DrawSelf( void )
{
	if( m_bFinished || m_bFinishing || m_bInvisible )
	{
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();

	float w = 0.50f;
	float h = 0.50f;

	vector_3 vp0 = -w*m_Vx +h*m_Vy;
	vector_3 vp1 = -w*m_Vx -h*m_Vy;
	vector_3 vp2 =  w*m_Vx +h*m_Vy;
	vector_3 vp3 =  w*m_Vx -h*m_Vy;

	renderer.ScaleWorldMatrix( vector_3( m_Scale, m_Scale, m_Scale ) );
	sVertex drawVerts[4];
	memset( drawVerts, 0, sizeof( sVertex ) * 4 );

	DWORD color	= MAKEDWORDCOLOR( m_Base_color );

	int over_draw = 1;

	if( !m_bUseLoadedTexture )
	{
		over_draw = 2;
	}

	for( int i = 0; i < over_draw; ++i )
	{
		drawVerts[0].x		= vp0.x;	drawVerts[1].x		= vp2.x;
		drawVerts[0].y		= vp0.y;	drawVerts[1].y		= vp2.y;
		drawVerts[0].z		= vp0.z;	drawVerts[1].z		= vp2.z;
		drawVerts[0].color	= color;	drawVerts[1].color	= color;
		drawVerts[0].uv.u	= 0.0f;		drawVerts[1].uv.u	= 1.0f;
		drawVerts[0].uv.v	= 1.0f;		drawVerts[1].uv.v	= 1.0f;

		drawVerts[2].x		= vp1.x;	drawVerts[3].x		= vp3.x;
		drawVerts[2].y		= vp1.y;	drawVerts[3].y		= vp3.y;
		drawVerts[2].z		= vp1.z;	drawVerts[3].z		= vp3.z;
		drawVerts[2].color	= color;	drawVerts[3].color	= color;
		drawVerts[2].uv.u	= 0.0f;		drawVerts[3].uv.u	= 1.0f;
		drawVerts[2].uv.v	= 0.0f;		drawVerts[3].uv.v	= 0.0f;

		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );
	}

	if( !m_bUseLoadedTexture )
	{
		color	= 0xFFA0A0A0;

		drawVerts[0].x		= vp0.x;	drawVerts[1].x		= vp2.x;
		drawVerts[0].y		= vp0.y;	drawVerts[1].y		= vp2.y;
		drawVerts[0].z		= vp0.z;	drawVerts[1].z		= vp2.z;
		drawVerts[0].color	= color;	drawVerts[1].color	= color;
		drawVerts[0].uv.u	= 0.0f;		drawVerts[1].uv.u	= 1.0f;
		drawVerts[0].uv.v	= 1.0f;		drawVerts[1].uv.v	= 1.0f;

		drawVerts[2].x		= vp1.x;	drawVerts[3].x		= vp3.x;
		drawVerts[2].y		= vp1.y;	drawVerts[3].y		= vp3.y;
		drawVerts[2].z		= vp1.z;	drawVerts[3].z		= vp3.z;
		drawVerts[2].color	= color;	drawVerts[3].color	= color;
		drawVerts[2].uv.u	= 0.0f;		drawVerts[3].uv.u	= 1.0f;
		drawVerts[2].uv.v	= 0.0f;		drawVerts[3].uv.v	= 0.0f;

		renderer.ScaleWorldMatrix( vector_3( 0.5f, 0.5f, 0.5f ) );
		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );
		color	= 0xFFA0A0A0;

		drawVerts[0].x		= vp0.x;	drawVerts[1].x		= vp2.x;
		drawVerts[0].y		= vp0.y;	drawVerts[1].y		= vp2.y;
		drawVerts[0].z		= vp0.z;	drawVerts[1].z		= vp2.z;
		drawVerts[0].color	= color;	drawVerts[1].color	= color;
		drawVerts[0].uv.u	= 0.0f;		drawVerts[1].uv.u	= 1.0f;
		drawVerts[0].uv.v	= 1.0f;		drawVerts[1].uv.v	= 1.0f;

		drawVerts[2].x		= vp1.x;	drawVerts[3].x		= vp3.x;
		drawVerts[2].y		= vp1.y;	drawVerts[3].y		= vp3.y;
		drawVerts[2].z		= vp1.z;	drawVerts[3].z		= vp3.z;
		drawVerts[2].color	= color;	drawVerts[3].color	= color;
		drawVerts[2].uv.u	= 0.0f;		drawVerts[3].uv.u	= 1.0f;
		drawVerts[2].uv.v	= 0.0f;		drawVerts[3].uv.v	= 0.0f;

		renderer.ScaleWorldMatrix( vector_3( 0.375f, 0.375f, 0.375f ) );
		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );
	}
}


float
Trackball_Effect::GetBoundingRadius( void )
{
// $$ todo this is correct
//	return 0.375f * m_Scale;

// This is not but fixes a lot of scripts - will have to find a better way later
	return 0.375f;
}


const SiegePos&
Trackball_Effect::GetPosition( void )
{
	m_ReportedPos.pos	=  m_Pos.pos + m_SpiralOffset;
	m_ReportedPos.node	=  m_Pos.node;

	gpassert( m_ReportedPos.node.IsValid() );

	return m_ReportedPos;
}




// Lightning ---
Lightning_Effect::Lightning_Effect( void )
{
	m_bGroundCheck		= false;
	m_Alpha				= 0;
	m_BoltLife			= 0;
	m_Bolt_life			= 0;
	m_Subd_param		= 0;
	m_Extra_subdivision	= 0;
	m_Min_displace		= 0;
	m_Max_displace		= 0;

	m_bArc				= false;

	m_ArcRadius			= 0;
	m_ArcTestDelay		= 0;
	m_ArcTestElapsed	= 0;

	m_ArcPosition		= TattooTracker::SOURCE;
	m_ArcSpreadDelay	= 0;
	m_ArcSpreadElapsed	= 0;

	m_ArcBranchCount	= 0;

	m_bDamage			= false;
	m_DamageMin			= 0;
	m_DamageMax			= 0;

	m_bIgnite			= false;
}


Lightning_Effect::~Lightning_Effect( void )
{
	// Apply any damage if specified - right after the visual plays if it even plays at all
	if( m_bDamage )
	{
		TattooTracker::TargetColl::iterator iTargets = m_Targets->GetTargetsBegin(), targets_end = m_Targets->GetTargetsEnd();

		for(; iTargets != targets_end; ++iTargets )
		{
			Goid Victim = MakeGoid( (*iTargets)->GetID());
			GoHandle hVictim( Victim );
			if( hVictim )
			{
				gWorldFx.ApplyDamage( m_OwnerID, Victim, m_DamageMin, m_DamageMax, m_bIgnite, 1.0f, false );
			}
		}
	}
}


void
Lightning_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",			0.6f, 0.7f, 1.0f );	// PARAM(1)
	params.SetVector3	( "damage",			5.0f, 10.0f, 0.0f ); // PARAM(2)

	params.SetFloat		( "bolt_life",		0.4f	);	// PARAM(3)
	params.SetFloat		( "subd",			0.400f	);	// PARAM(4)
	params.SetFloat		( "minsubd",		2.0f	);	// PARAM(5)
	params.SetFloat		( "mindisplace",	0.100f	);	// PARAM(6)
	params.SetFloat		( "maxdisplace",	0.165f	);	// PARAM(7)
	params.SetFloat		( "arc_radius",		1.0f	);	// PARAM(8)
	params.SetFloat		( "arc_test",		0.1f	);	// PARAM(9)
	params.SetFloat		( "arc_speed",		1.0f	);	// PARAM(10)

	params.SetUINT32	( "arc_branches",	3		);	// PARAM(11)

	params.SetBool		( "ignite",			false	);	// PARAM(12)
	params.SetBool		( "check_ground",	false	);	// PARAM(13)
}


void
Lightning_Effect::InitializeDerived( void )
{
	vector_3	temp( DoNotInitialize );

	m_Params.GetVector3	( PARAM(1),			m_Base_color		);
	m_bDamage = m_Params.GetVector3( PARAM(2), temp	);
	m_DamageMin = temp.x;
	m_DamageMax = temp.y;

	m_Params.GetFloat	( PARAM(3),			m_Bolt_life			);
	m_Params.GetFloat	( PARAM(4),			m_Subd_param		);
	m_Params.GetFloat	( PARAM(5),			m_Extra_subdivision	);
	m_Params.GetFloat	( PARAM(6),			m_Min_displace		);
	m_Params.GetFloat	( PARAM(7),			m_Max_displace		);
	ValidateOrder		( m_Min_displace,	m_Max_displace		);

	m_bArc = false;
	m_bArc |= m_Params.GetFloat	( PARAM(8),	m_ArcRadius			);
	m_bArc |= m_Params.GetFloat	( PARAM(9),	m_ArcTestDelay		);
	m_bArc |= m_Params.GetFloat	( PARAM(10),m_ArcSpreadDelay	);
	m_bArc |= m_Params.GetUINT32( PARAM(11),m_ArcBranchCount	);

	m_Params.GetBool	( PARAM(12),		m_bIgnite			);
	m_Params.GetBool	( PARAM(13),		m_bGroundCheck		);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_ArcTestElapsed	= 0;
	m_ArcSpreadElapsed	= 0;

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_bMustUpdate |= (m_bDamage | m_bIgnite);
}


UINT32
Lightning_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 3 );
	}
	return m_TextureName;
}



FUBI_DECLARE_CAST_TRAITS( TattooTracker::TARGET_INDEX, int );

bool
Lightning_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferVector	( "m_Points",				m_Points			);

	persist.Xfer		( "m_DefaultPosition",		m_DefaultPosition	);

	persist.Xfer		( "m_bGroundCheck",			m_bGroundCheck		);
	persist.Xfer		( "m_Alpha",				m_Alpha				);
	persist.Xfer		( "m_BoltLife",				m_BoltLife			);

	persist.Xfer		( "m_Base_color",			m_Base_color		);
	persist.Xfer		( "m_Bolt_life",			m_Bolt_life			);
	persist.Xfer		( "m_Subd_param",			m_Subd_param		);
	persist.Xfer		( "m_Extra_subdivision",	m_Extra_subdivision );

	persist.Xfer		( "m_Min_displace",			m_Min_displace		);
	persist.Xfer		( "m_Max_displace",			m_Max_displace		);

	persist.Xfer		( "m_bArc",					m_bArc				);

	persist.Xfer		( "m_ArcRadius",			m_ArcRadius			);
	persist.Xfer		( "m_ArcTestDelay",			m_ArcTestDelay		);
	persist.Xfer		( "m_ArcTestElapsed",		m_ArcTestElapsed	);
	persist.Xfer		( "m_ArcTestPosition",		m_ArcTestPosition	);

	persist.Xfer		( "m_ArcPosition",			m_ArcPosition		);
	persist.Xfer		( "m_ArcSpreadDelay",		m_ArcSpreadDelay	);
	persist.Xfer		( "m_ArcSpreadElapsed",		m_ArcSpreadElapsed	);

	persist.Xfer		( "m_ArcBranchCount",		m_ArcBranchCount	);

	persist.Xfer		( "m_bDamage",				m_bDamage			);
	persist.Xfer		( "m_DamageMin",			m_DamageMin			);
	persist.Xfer		( "m_DamageMax",			m_DamageMax			);

	persist.Xfer		( "m_bIgnite",				m_bIgnite			);

	return true;
}


void
Lightning_Effect::Generate3DBolt( V3Coll &points, V3Coll &temp, int recursion )
{
	UINT32 totalpoints = (points.size()-1)*(int)pow(2, recursion )+1;
	temp.resize( totalpoints );

	// Set up the pointlist
	float step = ((float)totalpoints - 1) / ((float)points.size() - 1);

	for( UINT32 i  = 0; i < points.size(); ++i )
	{
		temp[(int)(i * step)] = points[i];
	}

	// Start the subdivider
	if(recursion > 0)
	{
		for( i  = 0; i < points.size()-1; ++i )
		{
			Subdivide3DBolt( temp, recursion, (int)(i * step), (int)((i+1) * step) );
		}
	}
}


void
Lightning_Effect::Subdivide3DBolt( V3Coll &pointlist, int recursion, int iFirst, int iSecond )
{
	int iMid = ( iFirst + iSecond )/2;

	// Dereference
	const vector_3 &first = pointlist[ iFirst ];
	const vector_3 &second = pointlist[ iSecond ];

	// Generate iMid
	const vector_3	mid = ( first + second )*(float)0.5f;

	const float hlength = gWorldFx.GetRNG().Random( m_Min_displace, m_Max_displace ) * Length((second - mid));

	const vector_3	normal_vector( gWorldFx.GetRNG().RandomFloat(), gWorldFx.GetRNG().RandomFloat(), gWorldFx.GetRNG().RandomFloat() );
	const vector_3	random_vector( normal_vector * hlength );
	const vector_3	displace( random_vector );

	pointlist[iMid] = mid + displace;

	--recursion;
	if( recursion > 0 )
	{
		// Do left
		Subdivide3DBolt( pointlist, recursion, iFirst, iMid );
		// Do right
		Subdivide3DBolt( pointlist, recursion, iMid, iSecond );
	}
}


void
Lightning_Effect::UpdateDerived( void )
{
	if( m_FrameSync == 0 )
	{
		return;
	}

	m_BoltLife -= m_FrameSync;

	if( m_BoltLife < 0 )
	{
		// The points that make up the lightning bolt come in as nodespace points here
		PosColl	node_coords( m_Targets->GetAllPositions() );

		// If there aren't two points then we can't draw anything so abort
		if( node_coords.size() < 2 )
		{
			EndThisEffect();
			return;
		}

		// Start going through the list of points
		V3Coll target_coords;
		PosColl::iterator first = node_coords.begin(), iEndPos = node_coords.end();
		--iEndPos;

		// Determine a really rough bounding radius based on the first and last position in the collection

		const float new_radius = 0.5f * Length( gSiegeEngine.GetDifferenceVector( *first, *iEndPos ) );

		m_bHasMoved = ( new_radius != m_BoundingRadius );

		m_BoundingRadius = new_radius;
		
		// The first point in the list is what we will use for the reference point
		// every point there after will be an offset from the first point
		m_Pos = (*first);
		vector_3 offset;

		// Store the offsets
		for(; first != node_coords.end(); ++first )
		{
			offset = gSiegeEngine.GetDifferenceVector( m_Pos, *first );
			target_coords.push_back( offset );
		}
		// Make sure that the last point in the list of points will include the target
		target_coords.push_back( offset );

		UINT32 recurse = 2;

		if( target_coords.size() > 1 )
		{
			float length = Length((target_coords[0] - target_coords[1]));
			recurse = (UINT32)(m_Subd_param * length * m_ScaleFactor + m_Extra_subdivision);
			recurse = (UINT32)(recurse>5)?5:recurse;
		}

		Generate3DBolt( target_coords, m_Points, recurse );

		m_Alpha = 1.0f;
		m_BoltLife *= gWorldFx.GetRNG().RandomFloat();
	}
	else
	{
		m_Alpha -= 6.25f * m_FrameSync;
		m_Alpha = FilterClamp( 0.0f, 1.0f, m_Alpha );
	}

	const int total_points = m_Points.size()-1;

	if( m_bFinishing || (total_points <= 1) )
	{
		EndThisEffect();
		return;
	}

	// Go through the list of points and make sure that none of them are below the ground
	// if the parameter was specified to check for ground clearance.
	if( m_bGroundCheck )
	{
		//$ This code doesn't work... this code sucks... fix later if ever

		const float reference_y = gWorldFx.GetNodeCenter( m_Pos ).y;
		SiegePos last_point( m_Pos );

		for( int i = 0; i < total_points; ++i )
		{
			// Calculate the difference in m_Pos space
			const SiegePos A( m_Pos.pos + m_Points[ i ], m_Pos.node );
			const SiegePos B( m_Pos.pos + m_Points[ i+1 ], m_Pos.node );
			vector_3 diff( gSiegeEngine.GetDifferenceVector( A, B ) );

			// Get to the next point in the first points space so that UpdateNodePosition
			// will always just be one node away
			SiegePos cur_point( last_point.pos + diff, last_point.node );
			gWorldFx.UpdateNodePosition( cur_point );
			last_point = cur_point;

			// Adjust height relative to original reference point
			float node_y_diff = 0;

			if( gWorldFx.AdjustPointToTerrain( cur_point ) )
			{
				// Now adjust from Nodespace to the m_Pos relative offset space by using
				// the difference in the node centers for proper height adjustment

				node_y_diff = gWorldFx.GetNodeCenter( cur_point ).y - reference_y;
			}
			// Make final adjustment to the offset
			m_Points[ i+1 ].y = cur_point.pos.y + node_y_diff - m_Pos.pos.y;
		}
	}

	// Handle arcing to any nearby targets using specified arcing parameters
	if( m_bArc && (m_ArcBranchCount > 0) )
	{
		// Figure out where to do the test to spread from
		if( (m_ArcSpreadElapsed > m_ArcSpreadDelay) ) // && m_bCanSpread )
		{
			m_ArcSpreadElapsed = 0;

			switch( m_ArcPosition )
			{
				case TattooTracker::SOURCE:
				{
					m_ArcPosition = TattooTracker::TARGET;
				} break;

				case TattooTracker::TARGET:
				{
					m_ArcPosition = TattooTracker::REMAINING;
				} break;

				default:
				{
					if( (m_Targets->GetTargetCount()) < (UINT32)m_ArcPosition )
					{
						m_ArcPosition = TattooTracker::TARGET_INDEX(m_ArcPosition+1);
					}
				}
			}

			if( m_Targets->GetPlacement( &m_ArcTestPosition, NULL, m_OrientMode, m_ArcPosition ) )
			{
				if( !m_Targets->GetPlacement( &m_ArcTestPosition, NULL, m_OrientMode ) )
				{
					EndThisEffect();
					return;
				}
			}
		}
		m_ArcSpreadElapsed += m_FrameSync;


		// Gather targets from current position in targetlist
		if( m_ArcTestElapsed > m_ArcTestDelay )
		{
			m_ArcTestElapsed = 0;

			gWorldFx.AddTargetsWithinSphere( m_ArcTestPosition, m_ArcRadius, GetFriendly(), m_OwnerID, m_ArcBranchCount, m_Targets );
		}
		m_ArcTestElapsed += m_FrameSync;
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}


void
Lightning_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	const int total_points = m_Points.size()-1;
	if( total_points <= 1 )
	{
		EndThisEffect();
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();
	renderer.TranslateWorldMatrix( m_Pos.pos );

	DWORD color	= MAKEDWORDCOLOR( m_Base_color );
	color		= ((color&0x00FFFFFF)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

	// Initialize the forward difference for the beginning of the bolt
	vector_3 ForwardDifference	= m_Points[1] - m_Points[0];

	m_DrawVerts.resize( 2*total_points );

	for( int i = 0, j = 0; i < total_points; j += 2 )
	{
		vector_3 WideVector = CrossProduct( ForwardDifference, m_Vz );
		float WideDot = InnerProduct( ForwardDifference, m_Vz );

		if( WideDot < 0 )
		{
			WideVector = -WideVector;
		}

		float WideVectorLength = Length( WideVector );
		if( WideVectorLength > RealEpsilon )
		{
			WideVector /= WideVectorLength;
			WideVector *= 0.125f * m_ScaleFactor;
		}

		const vector_3 &Vertex = m_Points[i];

		vector_3 vp0( (Vertex + WideVector).GetElementsPointer());
		vector_3 vp1( (Vertex - WideVector).GetElementsPointer());

		m_DrawVerts[ j+0 ].x		= vp0.x;
		m_DrawVerts[ j+0 ].y		= vp0.y;
		m_DrawVerts[ j+0 ].z		= vp0.z;
		m_DrawVerts[ j+0 ].color	= color;
		m_DrawVerts[ j+0 ].uv.u		= 0.0f;
		m_DrawVerts[ j+0 ].uv.v		= 0.0f;

		m_DrawVerts[ j+1 ].x		= vp1.x;
		m_DrawVerts[ j+1 ].y		= vp1.y;
		m_DrawVerts[ j+1 ].z		= vp1.z;
		m_DrawVerts[ j+1 ].color	= color;
		m_DrawVerts[ j+1 ].uv.u		= 0.0f;
		m_DrawVerts[ j+1 ].uv.v		= 1.0f;

		if(++i < total_points)
		{
			ForwardDifference = m_Points[i + 1] - Vertex;
		}
	}

	if( total_points > 1 )
	{
		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, &*m_DrawVerts.begin(), m_DrawVerts.size(), SVERTEX, &m_TextureName, 1 );
	}
}


bool
Lightning_Effect::GetCullPosition( SiegePos & CullPosition )
{
	if( m_Targets->GetTargetCount() > 1 )
	{
		SiegePos start, end;
		TattooTracker::TargetColl::iterator iFirst = m_Targets->GetTargetsBegin(), iLast = m_Targets->GetTargetsEnd();
		--iLast;

		if( (*iFirst)->GetPlacement( &start, NULL ) &&	(*iLast)->GetPlacement( &end, NULL ) )
		{
			if( gWorldFx.IsNodeVisible( start.node ) && gWorldFx.IsNodeVisible( end.node ) )
			{
				const vector_3 diff( gSiegeEngine.GetDifferenceVector( start, end ) );

				if( gWorldFx.AddPositions( start, 0.5f * diff, false, NULL ) )
				{
					CullPosition	 = start;
					m_BoundingRadius = diff.Length();
					return true;
				}
			}
		}
	}
	return false;
}



// Steam ---
Steam_Effect::Steam_Effect( void )
{
	m_MinimumCorner = vector_3( RealMaximum,  RealMaximum,  RealMaximum );
	m_MaximumCorner = vector_3(-RealMaximum, -RealMaximum, -RealMaximum );

	m_AlphaFade		= 0;
	m_WispSize		= 0;
	m_MinWisps		= 0;
	m_MaxWisps		= 0;
	m_WispCount		= 0;
	m_AlphaStart	= 0;
	m_bDarkBlending = false;
	m_bCheap		= false;
	m_bInstantStart = false;
	m_bLineMode		= false;
	m_bRectMode		= false;
}


void
Steam_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.4f, 0.4f,	0.4f	); // PARAM(1)
	params.SetVector3	( "velocity",	0.0f, 5.75f,0.0f	); // PARAM(2)
	params.SetVector3	( "accel",		0.0f, 4.0f,	0.0f	); // PARAM(3)

	params.SetFloat		( "alpha_start",1.0f 	); // PARAM(4)
	params.SetFloat		( "alphafade",	0.55f	); // PARAM(5)
	params.SetFloat		( "wispsize",	2.25f	); // PARAM(6)
	params.SetFloat		( "count",		96.0f	); // PARAM(7)
	params.SetFloat		( "min_count",	96.0f	); // PARAM(8)

	params.SetBool		( "instant",	false	); // PARAM(9)
	params.SetBool		( "line",		false	); // PARAM(10)
	params.SetBool		( "rect",		false	); // PARAM(11)
	params.SetBool		( "cheap",		false	); // PARAM(12)
	params.SetBool		( "dark",		false	); // PARAM(13)
}


void
Steam_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);
	m_Params.GetVector3	( PARAM(2),		m_Velocity		);
	m_Params.GetVector3	( PARAM(3),		m_Acceleration	);

	m_Params.GetFloat	( PARAM(4),		m_AlphaStart	);
	m_Params.GetFloat	( PARAM(5),		m_AlphaFade		);
	m_Params.GetFloat	( PARAM(6),		m_WispSize		);
	m_Params.GetFloat	( PARAM(7),		m_MaxWisps		);
	if( m_Params.GetFloat( PARAM(8),	m_MinWisps		) )
	{
		m_MinWisps = m_MaxWisps;
	}

	ValidateOrder( m_MinWisps, m_MaxWisps );
	const float count = m_MinWisps + (gWorldFx.GetDetailLevel() * (m_MaxWisps - m_MinWisps ));

	m_MaxWisps		= (count > MAX_WISPS) ? MAX_WISPS : count;

	m_Params.GetBool	( PARAM(9),		m_bInstantStart	);
	m_Params.GetBool	( PARAM(10),		m_bLineMode		);
	m_Params.GetBool	( PARAM(11),		m_bRectMode		);
	m_Params.GetBool	( PARAM(12),		m_bCheap		);
	m_Params.GetBool	( PARAM(13),		m_bDarkBlending	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();
}


UINT32
Steam_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		if( m_bDarkBlending )
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 5 );
		}
		else
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 2 );
		}
	}
	return m_TextureName;
}


bool
Steam_Effect::ReInitialize( void )
{
	if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		return false;
	}

	return true;
}


bool
Steam_Effect::UnInitialize( void )
{
	m_Wisps.clear();
	m_Insert.clear();
	m_WispCount = 0;
	return true;
}


FUBI_DECLARE_SELF_TRAITS( Steam_Effect::Wisp );
FUBI_DECLARE_SELF_TRAITS( Steam_Effect::Wisp_list );


bool
Steam_Effect::Wisp::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "position",		position		);
	persist.Xfer	( "velocity",		velocity		);
	persist.Xfer	( "acceleration",	acceleration	);
	persist.Xfer	( "alpha",			alpha			);

	return true;
}


bool
Steam_Effect::Wisp_list::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "guid",			guid	);
	persist.XferList( "wisps",			wisps	);

	return true;
}


bool
Steam_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferList( "m_Wisps",			m_Wisps			);
	persist.XferList( "m_Insert",			m_Insert		);

	persist.Xfer	( "m_Base_color",		m_Base_color	);
	persist.Xfer	( "m_Acceleration",		m_Acceleration	);
	persist.Xfer	( "m_Velocity",			m_Velocity		);
	persist.Xfer	( "m_AlphaFade",		m_AlphaFade		);
	persist.Xfer	( "m_WispSize",			m_WispSize		);
	persist.Xfer	( "m_MinWisps",			m_MinWisps		);
	persist.Xfer	( "m_MaxWisps",			m_MaxWisps		);
	persist.Xfer	( "m_WispCount",		m_WispCount		);
	persist.Xfer	( "m_AlphaStart",		m_AlphaStart	);
	persist.Xfer	( "m_bDarkBlending",	m_bDarkBlending	);
	persist.Xfer	( "m_bCheap",			m_bCheap		);

	persist.Xfer	( "m_MinimumCorner",	m_MinimumCorner );
	persist.Xfer	( "m_MaximumCorner",	m_MaximumCorner );

	persist.Xfer	( "m_bInstantStart",	m_bInstantStart	);
	persist.Xfer	( "m_bLineMode",		m_bLineMode		);
	persist.Xfer	( "m_bRectMode",		m_bRectMode		);

	return true;
}


void
Steam_Effect::Put( void )
{
	SiegePos	targetPos;

	if( m_Targets->GetPlacement( &targetPos, NULL, m_OrientMode ) )
	{
		m_Targets->GetBoundingRadius( m_BoundingRadius );

		m_Scale = (m_BoundingRadius/11.091f) * m_ScaleFactor;

		SiegePos old_pos( m_Pos );

		m_Pos.pos = targetPos.pos + m_Offset;
		m_Pos.node = targetPos.node;

		m_bHasMoved = ( m_Pos != old_pos );
	}
	else
	{
		EndThisEffect();
	}
}



void
Steam_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = ReInitialize();
		if( !m_bInitialized )
		{
			return;
		}

		SiegePos	targetPos;
		matrix_3x3	target_orientation;

		if( m_Targets->GetPlacement( &targetPos, &target_orientation, m_OrientMode ) )
		{
			m_Targets->GetBoundingRadius( m_BoundingRadius );
		}
		else
		{
			m_BoundingRadius = 1.5f;
		}
		
		if( m_bAbsoluteCoords )
		{
			m_Scale = m_ScaleFactor = 1.0f;
		}
		else
		{
			m_Scale = (m_BoundingRadius/11.091f) * m_ScaleFactor;
		}

		m_Pos.pos = targetPos.pos + ( target_orientation * m_Offset );
		m_Pos.node = targetPos.node;
	}

	SiegePos	point_a, point_b;
	vector_3	origin_edge;

	if( (m_Targets->GetPlacement( &point_a, NULL, m_OrientMode, TattooTracker::TARGET )) &&
		(m_Targets->GetPlacement( &point_b, NULL, m_OrientMode, TattooTracker::SOURCE )) )
	{
		if( !gWorldFx.IsNodeVisible( point_a.node ) || !gWorldFx.IsNodeVisible( point_b.node ) )
		{
			EndThisEffect();
			return;
		}

		origin_edge = gSiegeEngine.GetDifferenceVector( point_a, point_b );
	}

	if( m_bInstantStart )
	{
		m_bInstantStart = false;
		Wisp_list	initial_list;
		initial_list.guid =	m_Pos.node;

		m_WispCount = m_MaxWisps;
		float PseudoSync = 0;

		for( int i = 0; i < m_MaxWisps; ++i )
		{
			Wisp wisp;

			PseudoSync += 0.048f;
			float td = PseudoSync;
			float newAlpha = m_AlphaStart - m_AlphaFade*PseudoSync;
			newAlpha = FilterClamp( 0.0f, 1.0f, newAlpha );

			float const wispDuration = m_AlphaStart/m_AlphaFade;
			td = newAlpha / m_AlphaFade;
			td = (float)fmod( td, wispDuration );

			float	X, Y, Z;

			if( m_bLineMode )
			{
				float magnitude = gWorldFx.GetRNG().RandomFloat();

				X = magnitude * origin_edge.x;
				Y = magnitude * origin_edge.y;
				Z = magnitude * origin_edge.z;
			}
			else if( m_bRectMode )
			{
				X = gWorldFx.GetRNG().RandomFloat() * origin_edge.x;
				Y = gWorldFx.GetRNG().RandomFloat() * origin_edge.y;
				Z = gWorldFx.GetRNG().RandomFloat() * origin_edge.z;
			}
			else
			{
				// Originate wisp
				const float r = gWorldFx.GetRNG().Random( 0.0f, 5.0f) * m_Scale;
				const float a = gWorldFx.GetRNG().Random( 0.0f, PI2 );

				// Set initial position
				SINCOSF( a, Z, X );
				X *= r;
				Y = r * gWorldFx.GetRNG().Random( 0.0f, 0.45f );
				Z *= r;
			}

			wisp.position.pos.x = ( X + m_Pos.pos.x );
			wisp.position.pos.y = ( Y + m_Pos.pos.y );
			wisp.position.pos.z = ( Z + m_Pos.pos.z );

			// Orient off of target
			matrix_3x3	orientation;
			m_Targets->GetPlacement( NULL, &orientation, m_OrientMode );

			vector_3	acceleration = orientation * m_Acceleration;
			vector_3	velocity = orientation * m_Velocity;
			
			if( m_bWindAffected )
			{
				vector_3 wind_vector( DoNotInitialize );
				gWorldFx.GetNodeWindVector( m_Pos.node, wind_vector );

				velocity += 5.0f * wind_vector;
			}

			// Set speeds
			wisp.acceleration = acceleration * m_Scale;
			wisp.velocity = velocity * gWorldFx.GetRNG().RandomFloat() * m_Scale;

			wisp.alpha = m_AlphaStart;

			wisp.alpha -= m_AlphaFade*td;
			wisp.alpha = FilterClamp( 0.0f, 1.0f, wisp.alpha );

			wisp.position.pos += ( 0.5f * (float)Square( td ) * wisp.acceleration )
								+ wisp.velocity * td;
			wisp.velocity += wisp.acceleration * td;

			wisp.position.node = m_Pos.node;

			if( gWorldFx.UpdateNodePosition( wisp.position ) )
			{
				bool inserted = false;
				for( WispLColl::iterator insert = m_Insert.begin(); insert != m_Insert.end(); ++insert )
				{
					if( (*insert).guid == wisp.position.node )
					{
						(*insert).wisps.push_back( wisp );
						inserted = true;
						break;
					}
				}
				if( inserted == false )
				{
					Wisp_list	new_list;
					new_list.guid = wisp.position.node;
					new_list.wisps.push_back( wisp );
					m_Insert.push_back( new_list );
				}
			}
			else
			{
				initial_list.wisps.push_back( wisp );
			}
		}
		m_Wisps.push_back( initial_list );
		for( WispLColl::iterator iList = m_Insert.begin(); iList != m_Insert.end();
		++iList )
		{
			m_Wisps.push_back( *iList );
		}
		m_Insert.clear();
	}

	INT32 wisps_to_process = (INT32)m_WispCount;
	bool spawn_new_wisps = false;

	if( !m_bFinishing )
	{
		m_WispCount += 10.5f * m_FrameSync;
		if( m_WispCount >= m_MaxWisps )
		{
			m_WispCount = m_MaxWisps;
		}
		spawn_new_wisps = true;
	}
	else
	{
		m_WispCount -= 10.5f * m_FrameSync;
		if( m_WispCount < 0.0f )
		{
			m_WispCount = 0.0f;
			EndThisEffect();
			return;
		}
	}

	wisps_to_process = (INT32)m_WispCount - wisps_to_process;

	if( !spawn_new_wisps )
	{
		for( WispLColl::iterator kill = m_Wisps.begin(); kill != m_Wisps.end(); ++kill )
		{
			WispColl::iterator iWisp = (*kill).wisps.begin(), wisp_end = (*kill).wisps.end();
			while( iWisp != wisp_end )
			{
				if( (*iWisp).alpha <= 0.25f )
				{
					iWisp = (*kill).wisps.erase( iWisp );
					wisp_end = (*kill).wisps.end();

					--wisps_to_process;
					if( wisps_to_process == 0 )
					{
						break;
					}
				}
				else
				{
					++iWisp;
				}
			}

			if( wisps_to_process == 0 )
			{
				break;
			}
		}
	}
	else
	{
		for( int wisp_count = wisps_to_process; wisp_count > 0; --wisp_count )
		{
			float td = m_FrameSync;
			Wisp wisp;

			float const wispDuration = m_AlphaStart/m_AlphaFade;
			td = m_AlphaStart / m_AlphaFade;
			td = (float)fmod( td, wispDuration );

			float	X, Y, Z;

			if( m_bLineMode )
			{
				float magnitude = gWorldFx.GetRNG().RandomFloat();

				X = magnitude * origin_edge.x;
				Y = magnitude * origin_edge.y;
				Z = magnitude * origin_edge.z;
			}
			else if( m_bRectMode )
			{
				X = gWorldFx.GetRNG().RandomFloat() * origin_edge.x;
				Y = gWorldFx.GetRNG().RandomFloat() * origin_edge.y;
				Z = gWorldFx.GetRNG().RandomFloat() * origin_edge.z;
			}
			else
			{
				// Originate wisp
				const float r = gWorldFx.GetRNG().Random( 0.0f, 5.0f) * m_Scale;
				const float a = gWorldFx.GetRNG().Random( 0.0f, PI2 );

				// Set initial position
				SINCOSF( a, Z, X );
				X *= r;
				Y = r * gWorldFx.GetRNG().Random( 0.0f, 0.45f );
				Z *= r;
			}

			// Set initial position
			wisp.position.pos.x = ( X + m_Pos.pos.x );
			wisp.position.pos.y = ( Y + m_Pos.pos.y );
			wisp.position.pos.z = ( Z + m_Pos.pos.z );

			// Orient off of target
			matrix_3x3	orientation;
			m_Targets->GetPlacement( NULL, &orientation, m_OrientMode );

			vector_3	acceleration = orientation * m_Acceleration;
			vector_3	velocity = orientation * m_Velocity;

			if( m_bWindAffected )
			{
				vector_3 wind_vector( DoNotInitialize );
				gWorldFx.GetNodeWindVector( m_Pos.node, wind_vector );

				velocity += 5.0f * wind_vector;
			}

			// Set speeds
			wisp.acceleration = acceleration * m_Scale;
			wisp.velocity = velocity * gWorldFx.GetRNG().RandomFloat() * m_Scale;

			wisp.alpha = m_AlphaStart;
			wisp.position.pos += ( 0.5f * (float)Square( td ) * wisp.acceleration )
								+ wisp.velocity * td;
			wisp.velocity += wisp.acceleration * td;

			wisp.position.node = m_Pos.node;
			gWorldFx.UpdateNodePosition( wisp.position );

			bool inserted = false;
			for( WispLColl::iterator existing = m_Wisps.begin(); existing != m_Wisps.end(); ++existing )
			{
				if( (*existing).guid == wisp.position.node )
				{
					(*existing).wisps.push_back( wisp );
					inserted = true;
					break;
				}
			}
			if( inserted == false )
			{
				Wisp_list	new_list;
				new_list.guid = wisp.position.node;
				new_list.wisps.push_back( wisp );
				m_Wisps.push_back( new_list );
			}
			AddWispToBounds( wisp );
		}
	}

	WispLColl::iterator uList = m_Wisps.begin(), uList_end = m_Wisps.end();
	for( ; uList != uList_end; ++uList )
	{
		WispColl::iterator uWisp = (*uList).wisps.begin(), uList_end = (*uList).wisps.end();
		while( uWisp != uList_end )
		{
			float td = m_FrameSync;
			float newAlpha = (*uWisp).alpha - m_AlphaFade*m_FrameSync;

			if( newAlpha < 0 )
			{
				Wisp wisp;

				float const wispDuration = m_AlphaStart/m_AlphaFade;
				td = -newAlpha / m_AlphaFade;
				td = (float)fmod( td, wispDuration );

				float	X, Y, Z;

				if( m_bLineMode )
				{
					float magnitude = gWorldFx.GetRNG().RandomFloat();

					X = magnitude * origin_edge.x;
					Y = magnitude * origin_edge.y;
					Z = magnitude * origin_edge.z;
				}
				else if( m_bRectMode )
				{
					X = gWorldFx.GetRNG().RandomFloat() * origin_edge.x;
					Y = gWorldFx.GetRNG().RandomFloat() * origin_edge.y;
					Z = gWorldFx.GetRNG().RandomFloat() * origin_edge.z;
				}
				else
				{
					// Originate wisp
					const float r = gWorldFx.GetRNG().Random( 0.0f, 5.0f) * m_Scale;
					const float a = gWorldFx.GetRNG().Random( 0.0f, PI2 );

					// Set initial position
					SINCOSF( a, Z, X );
					X *= r;
					Y = r * gWorldFx.GetRNG().Random( 0.0f, 0.45f );
					Z *= r;
				}

				// Set initial position
				wisp.position.pos.x = ( X + m_Pos.pos.x );
				wisp.position.pos.y = ( Y + m_Pos.pos.y );
				wisp.position.pos.z = ( Z + m_Pos.pos.z );

				// Orient off of target
				matrix_3x3	orientation;
				m_Targets->GetPlacement( NULL, &orientation, m_OrientMode );

				vector_3	acceleration = orientation * m_Acceleration;
				vector_3	velocity = orientation * m_Velocity;
				if( m_bWindAffected )
				{
					vector_3 wind_vector( DoNotInitialize );
					gWorldFx.GetNodeWindVector( m_Pos.node, wind_vector );

					velocity += 5.0f * wind_vector;
				}

				// Set speeds
				wisp.acceleration = acceleration * m_Scale;
				wisp.velocity = velocity * gWorldFx.GetRNG().RandomFloat() * m_Scale;

				wisp.alpha = m_AlphaStart;

				wisp.position.node = m_Pos.node;

				gWorldFx.UpdateNodePosition( wisp.position );

				if( wisp.position.node == (*uList).guid )
				{
					(*uWisp).position.pos = wisp.position.pos;
					(*uWisp).position.node = wisp.position.node;
					(*uWisp).velocity = wisp.velocity;
					(*uWisp).acceleration = wisp.acceleration;
					(*uWisp).alpha = wisp.alpha;
				}
				else
				{
					uWisp = (*uList).wisps.erase( uWisp );
					uList_end = (*uList).wisps.end();

					bool inserted = false;
					WispLColl::iterator iInsert = m_Insert.begin(), insert_end = m_Insert.end();
					for( ; iInsert != insert_end; ++iInsert )
					{
						if( (*iInsert).guid == wisp.position.node )
						{
							(*iInsert).wisps.push_back( wisp );
							inserted = true;
							AddWispToBounds( wisp );
							break;
						}
					}
					if( inserted == false )
					{
						Wisp_list	new_list;
						new_list.guid = wisp.position.node;
						new_list.wisps.push_back( wisp );
						AddWispToBounds( wisp );
						m_Insert.push_back( new_list );
					}
					continue;
				}
			}
			(*uWisp).alpha -= m_AlphaFade*td;
			(*uWisp).alpha = FilterClamp( 0.0f, 1.0f, (*uWisp).alpha );

			(*uWisp).position.pos += ( 0.5f * (float)Square( td ) * (*uWisp).acceleration ) + (*uWisp).velocity * td;
			(*uWisp).velocity += (*uWisp).acceleration * td;
			++uWisp;
		}
	}


	for( WispLColl::iterator iList = m_Insert.begin(), ilist_end = m_Insert.end(); iList != ilist_end; ++iList )
	{
		bool list_exists = false;
		for( WispLColl::iterator iExisting = m_Wisps.begin(); iExisting != m_Wisps.end(); ++iExisting )
		{
			if( (*iExisting).guid == (*iList).guid )
			{
				for( WispColl::iterator iNewWisp = (*iList).wisps.begin(); iNewWisp != (*iList).wisps.end(); ++iNewWisp )
				{
					(*iExisting).wisps.push_back( *iNewWisp );
					AddWispToBounds( *iNewWisp );
				}
				list_exists = true;
			}
		}
		if( list_exists == false )
		{
			m_Wisps.push_back( *iList );
		}
	}
	m_Insert.clear();

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}



void
Steam_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();

	if( m_bDarkBlending )
	{
		renderer.SetTextureStageState(	0,
									D3DTOP_ADD,
									D3DTOP_MODULATE,

									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	}

	vector_3	vpp0, vpp1, vpp2, vpp3;

	if( m_bCheap )
	{
		float	wh = m_WispSize + 8.5f;

		vpp0 = m_Scale * ( wh*m_Vy -wh*m_Vx );
		vpp1 = m_Scale * (-wh*m_Vx -wh*m_Vy );
		vpp2 = m_Scale * ( wh*m_Vx +wh*m_Vy );
		vpp3 = m_Scale * ( wh*m_Vx -wh*m_Vy );
	}

	for( WispLColl::iterator iDraw = m_Wisps.begin(); iDraw != m_Wisps.end(); ++iDraw )
	{
		// Make sure this is in a valid node otherwise abort this effect
		if( !gWorldFx.IsNodeVisible( (*iDraw).guid ) )
		{
			continue;
		}

		SetDrawMatrix( (*iDraw).guid );
		for( WispColl::iterator iWisp = (*iDraw).wisps.begin(); iWisp != (*iDraw).wisps.end(); ++iWisp )
		{
			vector_3 position( (*iWisp).position.pos );

			vector_3 vp0( position );
			vector_3 vp1( position );
			vector_3 vp2( position );
			vector_3 vp3( position );

			if( !m_bCheap )
			{
				float	wh = m_WispSize + 8.5f * ( 1.0f - (*iWisp).alpha );

				vector_3 position( (*iWisp).position.pos );

				vp0 += m_Scale * ( wh*m_Vy -wh*m_Vx );
				vp1 += m_Scale * (-wh*m_Vx -wh*m_Vy );
				vp2 += m_Scale * ( wh*m_Vx +wh*m_Vy );
				vp3 += m_Scale * ( wh*m_Vx -wh*m_Vy );
			}
			else
			{
				vp0 += vpp0;
				vp1 += vpp1;
				vp2 += vpp2;
				vp3 += vpp3;
			}

			sVertex	drawVerts[4];
			memset( drawVerts, 0, sizeof( sVertex )*4 );
			DWORD color = MAKEDWORDCOLOR( m_Base_color );
			color		= ((color&0x00FFFFFF)|((BYTE((*iWisp).alpha*m_NodeAlpha*255.0f))<<24));

			drawVerts[0].x			= vp0.x;
			drawVerts[0].y			= vp0.y;
			drawVerts[0].z			= vp0.z;
			drawVerts[0].color		= color;
			drawVerts[0].uv.u		= 0.0f;
			drawVerts[0].uv.v		= 1.0f;

			drawVerts[1].x			= vp2.x;
			drawVerts[1].y			= vp2.y;
			drawVerts[1].z			= vp2.z;
			drawVerts[1].color		= color;
			drawVerts[1].uv.u		= 1.0f;
			drawVerts[1].uv.v		= 1.0f;

			drawVerts[2].x			= vp1.x;
			drawVerts[2].y			= vp1.y;
			drawVerts[2].z			= vp1.z;
			drawVerts[2].color		= color;
			drawVerts[2].uv.u		= 0.0f;
			drawVerts[2].uv.v		= 0.0f;

			drawVerts[3].x			= vp3.x;
			drawVerts[3].y			= vp3.y;
			drawVerts[3].z			= vp3.z;
			drawVerts[3].color		= color;
			drawVerts[3].uv.u		= 1.0f;
			drawVerts[3].uv.v		= 0.0f;

			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );
		}
	}
	if( m_bDarkBlending )
	{
		gWorldFx.SaveRenderingState();
	}
}



void
Steam_Effect::AddPointToBounds( const vector_3 &Point )
{
// Adjust min dimension
	if( m_MinimumCorner.x > Point.x)
	{
		m_MinimumCorner.x = ( Point.x );
	}
	if( m_MinimumCorner.y > Point.y)
	{
		m_MinimumCorner.y = ( Point.y );
	}
	if( m_MinimumCorner.z > Point.z)
	{
		m_MinimumCorner.z = ( Point.z );
	}

// Adjust max dimension
	if( m_MaximumCorner.x < Point.x)
	{
		m_MaximumCorner.x = ( Point.x );
	}
	if( m_MaximumCorner.y < Point.y)
	{
		m_MaximumCorner.y = ( Point.y );
	}
	if( m_MaximumCorner.z < Point.z)
	{
		m_MaximumCorner.z = ( Point.z );
	}
}


void
Steam_Effect::AddWispToBounds( const Wisp &wisp )
{
	float const t = m_AlphaStart / m_AlphaFade;
	AddPointToBounds( wisp.position.pos );
	AddPointToBounds( wisp.position.pos + 0.5f * wisp.acceleration * t * t + wisp.velocity * t );
}



float
Steam_Effect::GetBoundingRadius( void )
{
	return 5.0f * m_Scale;
}




// Explosion ---
Explosion_Effect::Explosion_Effect( void )
{
	m_bOmniDirectional	=	false;
	m_bCollide			=	false;
	m_bSplat			=	false;
	m_bDarkBlending		=	false;
	m_bRandomScale		=	false;
	m_bSmoothTexture	=	false;
	m_bLineMode			=	false;

	m_Restitution		=	0;
	m_Fade_min			=	0;
	m_Fade_max			=	0;

	m_SplatFade			=	0;
	m_SplatLife			=	0;

	m_Radius			=	0;
	m_MinVelocity		=	0;
	m_MaxVelocity		=	0;

	m_Scale_min			=	0;
	m_Scale_max			=	0;

	m_min_phi			=	0;
	m_min_theta			=	0;
	m_max_phi			=	0;
	m_max_theta			=	0;

	m_check_freq		=	0;
	m_check_freq_count	=	0;
	m_check_freq_reset	=	0;

	m_SpawnRate			=	0;
	m_SplatTexture		=	0;
	m_bUseSplatTexture	=	false;
	m_SplatScaleUp		=	0;
	m_SplatSkip			=	0;

	m_AlphaStart		=	0;
}


void
Explosion_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",			1.0f, 0.10f, 0.10f	);	// PARAM(1)
	params.SetVector3	( "color1",			0.0f, 0.0f, 0.0f	);	// PARAM(2)
	params.SetVector3	( "color2",			1.0f, 0.10f, 0.10f	);	// PARAM(3)
	params.SetVector3	( "fade_range",		0.5f, 1.0f, 0.0f	);	// PARAM(4)
	params.SetVector3	( "scale_range",	0.25f, 0.25f, 0		);	// PARAM(5)
	params.SetVector3	( "ivel",			0.0f, 0.0f, 0.0f	);	// PARAM(6)
	params.SetVector3	( "rvel",			0.25f, 0.25f, 0.25f	);	// PARAM(7)

	params.SetUINT32	( "count",			32		);				// PARAM(8)
																	
	params.SetFloat		( "rebound",		0.65f	);				// PARAM(9)
	params.SetFloat		( "radius",			0.5f	);				// PARAM(10)
	params.SetFloat		( "splat_fade",		0.25f	);				// PARAM(11)
	params.SetFloat		( "splat_life",		2.0f	);				// PARAM(12)
	params.SetFloat		( "cfreq",			1.0f	);				// PARAM(13)
	params.SetFloat		( "vmin",			3.0f	);				// PARAM(14)
	params.SetFloat		( "vmax",			6.5f	);				// PARAM(15)
	params.SetFloat		( "min_phi",		0.0f	);				// PARAM(16)
	params.SetFloat		( "min_theta",		0.0f	);				// PARAM(17)
	params.SetFloat		( "max_phi",		PI		);				// PARAM(18)
	params.SetFloat		( "max_theta",		PI2		);				// PARAM(19)
	params.SetFloat		( "srate",			0		);				// PARAM(20)
																	
	params.SetBool		( "omni_dir",		false	);				// PARAM(21)
	params.SetBool		( "no_noise",		false	);				// PARAM(22)
	params.SetBool		( "rand_scale",		false	);				// PARAM(23)
	params.SetBool		( "splat",			false	);				// PARAM(24)
	params.SetBool		( "collide",		false	);				// PARAM(25)
	params.SetBool		( "dark",			false	);				// PARAM(26)
	params.SetBool		( "line",			false	);				// PARAM(27)

	params.SetGPString	( "stexture",		gpstring::EMPTY		);	// PARAM(28)
	params.SetFloat		( "alphastart",		1.0f	);				// PARAM(29)
	params.SetFloat		( "splatscaleup",	2.275f	);				// PARAM(30)
	params.SetUINT32	( "splatskip",		4		);				// PARAM(31)
}


void
Explosion_Effect::InitializeDerived( void )
{
	vector_3 temp( DoNotInitialize );

	m_Params.GetVector3	( PARAM(1),		m_Base_color		);
	m_Params.GetVector3	( PARAM(2),		m_Color_variance	);
	m_Params.GetVector3	( PARAM(3),		m_SplatColor		);

	m_Params.GetVector3	( PARAM(4),		temp				);
	m_Fade_min = max_t( temp.x, 0.5f );
	m_Fade_max = temp.y;

	m_Params.GetVector3	( PARAM(5),		temp				);
	m_Scale_min	= temp.x;
	m_Scale_max = temp.y;

	m_Params.GetVector3	( PARAM(6),		m_iVel				);
	m_Params.GetVector3	( PARAM(7),		m_rVel				);

	m_Params.GetUINT32	( PARAM(8),		m_ParticleCount		);

	m_Params.GetFloat	( PARAM(9),		m_Restitution		);
	m_Params.GetFloat	( PARAM(10),	m_Radius			);
	m_Params.GetFloat	( PARAM(11),	m_SplatFade			);
	m_Params.GetFloat	( PARAM(12),	m_SplatLife			);
	m_Params.GetFloat	( PARAM(13),	m_check_freq		);
	m_Params.GetFloat	( PARAM(14),	m_MinVelocity		);
	m_Params.GetFloat	( PARAM(15),	m_MaxVelocity		);
	m_Params.GetFloat	( PARAM(16),	m_min_phi			);
	m_Params.GetFloat	( PARAM(17),	m_min_theta			);
	m_Params.GetFloat	( PARAM(18),	m_max_phi			);
	m_Params.GetFloat	( PARAM(19),	m_max_theta			);
	m_Params.GetFloat	( PARAM(20),	m_SpawnRate			);
	
	m_Params.GetBool	( PARAM(21),	m_bOmniDirectional	);
	m_Params.GetBool	( PARAM(22),	m_bSmoothTexture	);
	m_Params.GetBool	( PARAM(23),	m_bRandomScale		);

	if( m_Params.GetBool( PARAM(24),	m_bSplat	) )	
	{
		m_bCollide = true;
	}
	else
	{
		m_Params.GetBool( PARAM(25),	m_bCollide	);
	}

	m_Params.GetBool	( PARAM(26),	m_bDarkBlending		);
	m_Params.GetBool	( PARAM(27),	m_bLineMode			);

	m_bUseSplatTexture = m_Params.GetGPString( PARAM(28),	m_sSplatTexture		);

	m_Params.GetFloat	( PARAM(29),	m_AlphaStart		);
	m_Params.GetFloat	( PARAM(30),	m_SplatScaleUp		);
	m_Params.GetUINT32	( PARAM(31),	m_SplatSkip			);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_SplatSkipCount= 0;

	m_ParticleCount	= min_t( (int)m_ParticleCount, MAX_PARTICLES );
	m_Radius		= max_t( m_Radius, 2.0f*FLOAT_TOLERANCE );
	m_check_freq	= FilterClamp( 0.0f, 1.0f, m_check_freq );
	m_SpawnTime		= m_SpawnRate;
	m_SpawnRate		= (m_SpawnRate == 0)?0.0f: (1.0f / m_SpawnRate);

	ValidateOrder( m_Fade_min,		m_Fade_max		);
	ValidateOrder( m_Scale_min,		m_Scale_max		);
	ValidateOrder( m_min_phi,		m_max_phi		);
	ValidateOrder( m_min_theta,		m_max_theta		);
	ValidateOrder( m_MinVelocity,	m_MaxVelocity	);

	if( m_bCollide )
	{
		m_check_freq_reset = (INT32)(m_ParticleCount / (m_ParticleCount * m_check_freq ));
		m_check_freq_count = m_check_freq_reset;
	}

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_Gravity.y = -9.8067f;

	m_bMustUpdate = true;

	InitializeSplatTexture();

	// test all parameters for the explosion effect that were given by the effect
	// do these seperatly, so we get nice warnings.  i think that these should be the
	// upper bound, we might want to lower or raise some of these
	gpassert( m_iVel		.Length()	< 5000 );
	gpassert( m_rVel		.Length()	< 5000 );
	gpassert( fabs(m_ParticleCount)		< 500 );
	gpassert( fabs(m_Restitution)		< 500 );
	gpassert( fabs(m_Radius)			< 500 );
	gpassert( fabs(m_SplatFade)			< 500 );
	gpassert( fabs(m_SplatLife)			< 500 );
	gpassert( fabs(m_check_freq)		< 500 );
	gpassert( fabs(m_MinVelocity)		< 500 );
	gpassert( fabs(m_MaxVelocity)		< 500 );
	gpassert( fabs(m_min_phi)			< 500 );
	gpassert( fabs(m_min_theta)			< 500 );
	gpassert( fabs(m_max_phi)			< 500 );
	gpassert( fabs(m_max_theta)			< 500 );
	gpassert( fabs(m_SpawnRate)			< 100000 );
}


void
Explosion_Effect::InitializeSplatTexture( void )
{
	if( m_bUseSplatTexture )
	{
		// Destroy existing texture
		if( m_SplatTexture )
		{
			gWorldFx.GetRenderer().DestroyTexture( m_SplatTexture );
		}

		gpstring sFilename;

		if( gNamingKey.BuildIMGLocation( m_sSplatTexture, sFilename ) )
		{
			m_SplatTexture	= gWorldFx.GetRenderer().CreateTexture	( sFilename, false, 0, TEX_LOCKED );
		}
		else
		{
			gperrorf(("[%s] does not exist\n",m_sSplatTexture.c_str()));
			m_SplatTexture = gWorldFx.GetGlobalTexture( 8 );
		}
	}
	else
	{
		m_SplatTexture = gWorldFx.GetGlobalTexture( 8 );
	}
}


Explosion_Effect::~Explosion_Effect( void )
{
	if( (m_SplatTexture != 0) && ( m_bUseSplatTexture  ) )
	{
		gWorldFx.GetRenderer().DestroyTexture( m_SplatTexture );
	}
}


bool
Explosion_Effect::ReInitialize( void )
{
	m_DrawVerts.resize( (3 * m_ParticleCount) + 3 );

	// Clear array and setup UVs once since they don't change
	ZeroMemory( &*m_DrawVerts.begin(), sizeof( sVertex ) * m_DrawVerts.size() );

	for( UINT32 i = 0, vi = 0; i < m_ParticleCount; ++i, vi += 3 )
	{
		sVertex* verts = &*m_DrawVerts.begin();

		verts[vi+0].uv.u	= 0.0f;
		verts[vi+0].uv.v	= 0.0f;

		verts[vi+1].uv.u	= 2.0f;
		verts[vi+1].uv.v	= 0.0f;

		verts[vi+2].uv.u	= 0.0f;
		verts[vi+2].uv.v	= 2.0f;
	}
	return true;
}


UINT32
Explosion_Effect::InitializeTexture( void )
{
	InitializeSplatTexture();

	if( !m_bUseLoadedTexture )
	{
		if( m_bDarkBlending )
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 7 );
		}
		else
		{
			if( m_bSmoothTexture )
			{
				m_TextureName = gWorldFx.GetGlobalTexture( 2 );
			}
			else
			{
				m_TextureName = gWorldFx.GetGlobalTexture( 6 );
			}
		}
	}
	return m_TextureName;
}


FUBI_DECLARE_SELF_TRAITS( Explosion_Effect::Particle );


bool
Explosion_Effect::Particle::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "elapsed",		elapsed			);
	persist.Xfer	( "position",		position		);
	persist.Xfer	( "velocity",		velocity		);
	persist.Xfer	( "w",				w				);
	persist.Xfer	( "h",				h				);
	persist.Xfer	( "alpha",			alpha			);
	persist.Xfer	( "alpha_fade",		alpha_fade		);
	persist.Xfer	( "color",			color			);
	persist.Xfer	( "splat_verts0",	splat_verts[0]	);
	persist.Xfer	( "splat_verts1",	splat_verts[1]	);
	persist.Xfer	( "splat_verts2",	splat_verts[2]	);
	persist.Xfer	( "splat",			splat			);
	persist.Xfer	( "should_collide",	should_collide	);
	persist.Xfer	( "splat_life",		splat_life		);

	return true;
}


FUBI_DECLARE_XFER_TRAITS( Explosion_Effect::ParticleList )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferList( key, obj ) );
	}
};



bool
Explosion_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferMap	( "m_Particles",			m_Particles			);
	persist.XferMap	( "m_SplatParticles",		m_SplatParticles	);

	persist.Xfer	( "m_SpawnOrigin",			m_SpawnOrigin		);
	persist.Xfer	( "m_LastSpawnTarget",		m_LastSpawnTarget	);
	persist.Xfer	( "m_LastSpawnSource",		m_LastSpawnSource	);

	persist.Xfer	( "m_SpawnDirection",		m_SpawnDirection	);
	persist.Xfer	( "m_SpawnRate",			m_SpawnRate			);
	persist.Xfer	( "m_SpawnTime",			m_SpawnTime			);

	persist.Xfer	( "m_Gravity",				m_Gravity			);

	persist.Xfer	( "m_bOmniDirectional",		m_bOmniDirectional	);
	persist.Xfer	( "m_bCollide",				m_bCollide			);
	persist.Xfer	( "m_bSplat",				m_bSplat			);
	persist.Xfer	( "m_bDarkBlending",		m_bDarkBlending		);
	persist.Xfer	( "m_bRandomScale",			m_bRandomScale		);
	persist.Xfer	( "m_bSmoothTexture",		m_bSmoothTexture	);
	persist.Xfer	( "m_bLineMode",			m_bLineMode			);

	persist.Xfer	( "m_origin_dir",			m_origin_dir		);

	persist.Xfer	( "m_ParticleCount",		m_ParticleCount		);

	persist.Xfer	( "m_Restitution",			m_Restitution		);
	persist.Xfer	( "m_Base_color",			m_Base_color		);
	persist.Xfer	( "m_Color_variance",		m_Color_variance	);
	persist.Xfer	( "m_SplatColor",			m_SplatColor		);
	persist.Xfer	( "m_Fade_min",				m_Fade_min			);
	persist.Xfer	( "m_Fade_max",				m_Fade_max			);

	persist.Xfer	( "m_SplatFade",			m_SplatFade			);
	persist.Xfer	( "m_SplatLife",			m_SplatLife			);

	persist.Xfer	( "m_Radius",				m_Radius			);
	persist.Xfer	( "m_MinVelocity",			m_MinVelocity		);
	persist.Xfer	( "m_MaxVelocity",			m_MaxVelocity		);

	persist.Xfer	( "m_Scale_min",			m_Scale_min			);
	persist.Xfer	( "m_Scale_max",			m_Scale_max			);

	persist.Xfer	( "m_iVel",					m_iVel				);
	persist.Xfer	( "m_rVel",					m_rVel				);

	persist.Xfer	( "m_min_phi",				m_min_phi			);
	persist.Xfer	( "m_min_theta",			m_min_theta			);
	persist.Xfer	( "m_max_phi",				m_max_phi			);
	persist.Xfer	( "m_max_theta",			m_max_theta			);

	persist.Xfer	( "m_check_freq",			m_check_freq		);
	persist.Xfer	( "m_check_freq_count",		m_check_freq_count	);
	persist.Xfer	( "m_check_freq_reset",		m_check_freq_reset	);

	persist.Xfer	( "m_bUseSplatTexture",		m_bUseSplatTexture	);
	persist.Xfer	( "m_sSplatTexture",		m_sSplatTexture		);
	persist.Xfer	( "m_SplatScaleUp",			m_SplatScaleUp		);
	persist.Xfer	( "m_SplatSkip",			m_SplatSkip			);
	persist.Xfer	( "m_SplatSkipCount",		m_SplatSkipCount	);

	persist.Xfer	( "m_AlphaStart",			m_AlphaStart		);

	return true;
}


void
Explosion_Effect::UpdateDerived( void )
{
	// Setup the point of origin only when we need to
	if( !m_bInitialized || (m_SpawnRate > 0) )
	{
		// Do initial point of emission and directional setups
		m_Targets->GetPlacement( &m_SpawnOrigin, &m_Orient, m_OrientMode );
		m_Pos = m_SpawnOrigin;

		// only set these the first time through.
		if( !m_bInitialized )
		{
			m_LastSpawnTarget = m_SpawnOrigin;
			m_LastSpawnSource = m_SpawnOrigin;
		}

		// Correct position to the orientation of the target
		vector_3 offset( m_Orient * m_Offset );

		siege::database_guid old_node = m_SpawnOrigin.node;

		bool bNodeChange = false;

		if( !gWorldFx.AddPositions( m_SpawnOrigin, offset, !m_bCollide, &bNodeChange ) )
		{
			EndThisEffect();
			return;
		}

		matrix_3x3	orient_difference;

		if( bNodeChange )
		{
			gWorldFx.GetDifferenceOrient( old_node, m_SpawnOrigin.node, orient_difference );

			m_Orient = orient_difference * m_Orient;
		}

		// Explosion direction is specified either by the targets orientation or
		// by an external vector as m_Dir or it is an omnidirectional explosion
		if( !m_bOmniDirectional )
		{
			if( m_Dir.IsZero() )
			{
				m_Dir = vector_3::UP;
			}
			// orient to models space and determine actual firing direction
			m_SpawnDirection = m_Orient * MatrixFromDirection( m_Dir );
		}

		// Spawn all the particles if there is no spawn rate set
		if( m_SpawnRate == 0 )
		{
			Particle new_particle;

			// Prepare all explosion particles
			for( UINT32 n = 0; n < m_ParticleCount; ++n )
			{
				if( CreateParticle( new_particle ) )
				{
					InsertParticle( new_particle, m_Particles );
				}
			}
		}
	}

	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		ReInitialize();
	}


	// Update active particles
	bool bActiveParticle = false;

	ParticleMap::iterator iList = m_Particles.begin(), iListEnd = m_Particles.end();
	for(; iList != iListEnd; ++iList )
	{
		ParticleList::iterator iParticle = (*iList).second.begin(), iParticleEnd = (*iList).second.end();
		for(; iParticle != iParticleEnd; ++iParticle )
		{
			if( UpdateParticle( *iParticle ) || ((*iParticle).alpha <= 0 ) || ((*iParticle).splat) )
			{
				m_TransferList.push_back( iParticle );
			}

			bActiveParticle = true;
		}
	}

	iList = m_SplatParticles.begin(), iListEnd = m_SplatParticles.end();
	for(; iList != iListEnd; ++iList )
	{
		ParticleList::iterator iParticle = (*iList).second.begin(), iParticleEnd = (*iList).second.end();
		for(; iParticle != iParticleEnd; ++iParticle )
		{
			UpdateParticle( *iParticle );
			bActiveParticle = true;
		}
	}

	// Process transferred particles
	iParticleList::iterator i = m_TransferList.begin(), i_end = m_TransferList.end();
	for(; i != i_end; ++i )
	{
		if( (*(*i)).splat )
		{
			InsertParticle( **i, m_SplatParticles );
		}
		else if( (*(*i)).alpha > 0 )
		{	// Reinsert this particle into the correct list
			InsertParticle( **i, m_Particles );
		}
		// Now remove it from it's former list
		ParticleMap::iterator iList = m_Particles.find( (*(*i)).position.node );
		(*iList).second.erase( *i );
	}
	m_TransferList.clear();


	// Spawn new particles if specified to do so
	if( (m_SpawnRate > 0) && !m_bFinishing )
	{
		m_SpawnTime -= m_FrameSync;
		if( m_SpawnTime < 0 )
		{
			const UINT32 max_spawn = 1 + UINT32(floorf( FABSF( m_SpawnTime ) * m_SpawnRate ));

			// Determine how many particles should be spawned
			m_SpawnTime += max_spawn * ( 1.0f / m_SpawnRate );

			// Determine how many particles can actually be spawned because of room
			UINT32 total_particles = 0;
			ParticleMap::iterator iList = m_Particles.begin(), iListEnd = m_Particles.end();
			for(; iList != iListEnd; ++iList )
			{
				total_particles += (*iList).second.size();
			}

			if( total_particles < m_ParticleCount )
			{
				const UINT32 spawn_count = min_t( (m_ParticleCount - total_particles), max_spawn );

				for( UINT32 i = spawn_count; i > 0; --i )
				{
					// Create a new particle to insert
					Particle new_particle;
					if( CreateParticle( new_particle ) )
					{
						InsertParticle( new_particle, m_Particles );
					}
				}
			}
		}
	
		
		SiegePos target, source;

		// Set the Last spawn positions. We are going to use these to smoothly generate particles between frames.
		if( m_Targets->GetPlacement( &target, NULL, false, TattooTracker::TARGET ))
		{
			m_LastSpawnTarget = target;
		}
			
		if( m_Targets->GetPlacement( &source, NULL, false, TattooTracker::SOURCE ))
		{
			m_LastSpawnSource = source;
		}
	}

	// Check to see if this effect should finish
	if( (m_Duration == -1.0f) && (m_SpawnRate == 0 ) )
	{
		// Check if we are done with everything
		m_bFinished = !bActiveParticle ;
	}
	else if( m_bFinishing && !bActiveParticle )
	{
		EndThisEffect();
	}
}


bool
Explosion_Effect::CreateParticle( Particle &particle )
{
	if( m_bCollide )
	{
		// Now check for collisions based on frequency
		--m_check_freq_count;

		// Initialize particle collision check frequency
		if( m_check_freq_count < 1 )
		{
			m_check_freq_count = m_check_freq_reset;
			particle.should_collide = true;
		}
		else
		{
			particle.should_collide = false;
		}
	}

	// Initialize common particle attributes
	particle.alpha		= 1.0f;
	particle.alpha_fade	= gWorldFx.GetRNG().Random( m_Fade_min, m_Fade_max );
	particle.w			= gWorldFx.GetRNG().Random( m_Scale_min, m_Scale_max );
	particle.h			= particle.w;

	vector_3 p( DoNotInitialize );

	float	startTime = 0.0f;

	if( m_bLineMode )
	{
		SiegePos point_a, point_b;

		if( (m_Targets->GetPlacement( &point_a, NULL, false, TattooTracker::TARGET )) &&
			(m_Targets->GetPlacement( &point_b, NULL, false, TattooTracker::SOURCE )) )
		{
			if( point_a != point_b )
			{
				m_origin_dir = gSiegeEngine.GetDifferenceVector( point_a, point_b );
			}
			else
			{
				m_origin_dir = vector_3::UP;
			}
		}

		// check if positions are valid.
		if( gSiegeEngine.IsNodeValid( m_LastSpawnTarget.node ) && gSiegeEngine.IsNodeValid( m_LastSpawnSource.node ) )
		{
			vector_3 oldTarget( DoNotInitialize );
			vector_3 oldSource( DoNotInitialize );

			oldTarget = gSiegeEngine.GetDifferenceVector( point_a, m_LastSpawnTarget );
			oldSource = gSiegeEngine.GetDifferenceVector( point_a, m_LastSpawnSource );

			float linePos = gWorldFx.GetRNG().RandomFloat();
			float spawnPos = gWorldFx.GetRNG().RandomFloat();

			p = ( ( 1.0f - spawnPos ) * m_origin_dir * linePos ) + 
				( spawnPos * ( ( ( 1.0f - linePos ) * oldTarget ) + ( linePos * oldSource) ) );
			
			// store how far back in time this particle was created so we can update it acordingly.
			startTime = spawnPos * m_FrameSync; 	
		}
		else
		{
			p = m_origin_dir * gWorldFx.GetRNG().RandomFloat();
		}

		m_Pos = point_a;
		particle.position = point_a;	
		m_Orient = MatrixFromDirection( m_origin_dir );

	}
	else
	{
		particle.position = m_SpawnOrigin;

		// Prep particles for an omni-directional explosion using points
		// originating from the surface of a fixed radius i.e. sphere surface
		// without exploding inward
		if( m_bOmniDirectional )
		{
			// Determine some random angles withing a range
			float thetasin, thetacos, phisin, phicos;
			SINCOSF( gWorldFx.GetRNG().Random( m_min_theta, m_max_theta ), thetasin, thetacos );
			SINCOSF( gWorldFx.GetRNG().Random( m_min_phi, m_max_phi ), phisin, phicos );

			p.x = m_Radius * thetasin * phicos;
			p.y = m_Radius * thetasin * phisin;
			p.z = m_Radius * thetacos;

			// Determine random explosion magnitude
			const float	explosive_magnitude = gWorldFx.GetRNG().Random( m_MinVelocity, m_MaxVelocity );

			if( m_bRandomScale )
			{
				// Determine velocity based on size of particle - biggest = 10% max speed
				particle.velocity = (explosive_magnitude * (1.10f - (particle.w / m_Scale_max))) * Normalize( p );
			}
			else
			{
				particle.velocity = explosive_magnitude * Normalize( p );
			}
		}
		else// Prep particle velocity for a directional explosion
		{
			// Originate particles in a radius defined in the XY plane
			float	thetasin, thetacos;
			SINCOSF( gWorldFx.GetRNG().Random( m_max_theta ), thetasin, thetacos );
			float	r = gWorldFx.GetRNG().Random( m_Radius );

			// Forward movement is always along +Z
			p = (r * thetacos * m_SpawnDirection.GetColumn0_T()) + (r * thetasin * m_SpawnDirection.GetColumn1_T());

			// Set particle velocity along Z in the space of the direction
			particle.velocity = gWorldFx.GetRNG().Random( m_MinVelocity, m_MaxVelocity ) * m_SpawnDirection.GetColumn2_T();

			// Perturb the velocity vector based on the vector from the origin point to get a cone shape
			particle.velocity += p;
		}
		
		// adjust for a moving target.
		if( gSiegeEngine.IsNodeValid( m_LastSpawnTarget.node ) )
		{
			SiegePos point_a;
			if( m_Targets->GetPlacement( &point_a, NULL, false, TattooTracker::TARGET ) )
			{
				vector_3 oldTarget( DoNotInitialize );
			
				oldTarget = gSiegeEngine.GetDifferenceVector( point_a, m_LastSpawnTarget );
			
				
				float spawnPos = gWorldFx.GetRNG().RandomFloat();
				p += spawnPos * oldTarget;

				// store how far back in time this particle was created so we can update it acordingly.
				startTime = spawnPos * m_FrameSync; 
			}
		}
	}

	bool bNodeChange = false;
	siege::database_guid old_node = particle.position.node;

	if( !gWorldFx.AddPositions( particle.position, p, !m_bCollide, &bNodeChange ) )
	{
		return false;
	}

	// Create the random velocity vector
	const vector_3	random_velocity( gWorldFx.GetRNG().Random( -m_rVel.x, m_rVel.x ),
									 gWorldFx.GetRNG().Random( -m_rVel.y, m_rVel.y ),
									 gWorldFx.GetRNG().Random( -m_rVel.z, m_rVel.z ) );

	// Add random velocity and initial velocity to main velocity in the space of the target
	particle.velocity += m_Orient * ( m_iVel + random_velocity );

	if( bNodeChange )
	{
		gWorldFx.ReOrientVector( old_node, particle.position.node, particle.velocity );
	}

	// Add wind influence if specified
	if( m_bWindAffected )
	{
		vector_3 wind_vector( DoNotInitialize );
		gWorldFx.GetNodeWindVector( particle.position.node, wind_vector );
		
		particle.velocity += wind_vector;
	}

	// Set color and add variation
	particle.color = m_Base_color;

	if( !m_Color_variance.IsZero() )
	{
		particle.color.x += gWorldFx.GetRNG().Random( -m_Color_variance.x, m_Color_variance.x );
		particle.color.y += gWorldFx.GetRNG().Random( -m_Color_variance.y, m_Color_variance.y );
		particle.color.z += gWorldFx.GetRNG().Random( -m_Color_variance.z, m_Color_variance.z );

		particle.color.x = FilterClamp( 0.0f, 1.0f, particle.color.x );
		particle.color.y = FilterClamp( 0.0f, 1.0f, particle.color.y );
		particle.color.z = FilterClamp( 0.0f, 1.0f, particle.color.z );
	}

	// Set splate life counter
	particle.splat_life = m_SplatLife;

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

	if( startTime > 0.0f )
	{
		UpdateParticle( particle, startTime );
	}
	return true;
}


void
Explosion_Effect::InsertParticle( Particle &particle, ParticleMap &map )
{
	// Find an existing particle list that matches the guid
	ParticleMap::iterator iList = map.find( particle.position.node );

	if( iList == map.end() )
	{
		// None found so create a new Particle list
		map.insert( std::pair< siege::database_guid, ParticleList > ( particle.position.node, ParticleList() ) );

		// Since we know that the list exists since we just created it geth
		iList = map.find( particle.position.node );
	}

	// Insert the particle into the correct list
	(*iList).second.push_back( particle );
}


bool
Explosion_Effect::UpdateParticle( Particle &particle, float initialTime )
{
	float updateTime = ( initialTime > 0.0f ) ? initialTime :  m_FrameSync;
	particle.elapsed += updateTime;

	// Don't bother updating a particle that is dead
	if( particle.alpha <= 0 )
	{
		return false;
	}

	// If the particle has already splat then just update the alpha value for it
	if( particle.splat )
	{
		// Decrease splat alpha
		particle.splat_life -= updateTime;
		if( particle.splat_life < 0 )
		{
			particle.alpha -= m_SplatFade * updateTime;
			particle.alpha = FilterClamp( 0.0f, 1.0f, particle.alpha );
		}
		return false;
	}

	// Don't fade particle until it splats
	if( !m_bSplat )
	{
		particle.alpha -= particle.alpha_fade * updateTime;
		particle.alpha = FilterClamp( 0.0f, 1.0f, particle.alpha );
	}

	// make sure that there the particles that were started below the ground get killed off since there
	// is the possibility that they will never hit ground - give them 8 seconds
	if( m_bSplat && (particle.elapsed > 8.0f) )
	{
		return false;
	}

	// make sure that our velocity is bounded.  this will get 
	// very very large at times, and is caused by differenct sources
	gpassert( fabs(particle.velocity.Length()) < 5000 );

	const vector_3 displacement( (0.5f * (updateTime * updateTime) * m_Gravity) + particle.velocity * updateTime );

	// Save the old unmodified position
	SiegePos old_pos( particle.position );

	bool bNodeChange = false;

	if( !gWorldFx.AddPositions( particle.position, displacement, !m_bCollide, &bNodeChange ) )
	{
		return false;
	}

	particle.velocity += m_Gravity * updateTime;

	if( bNodeChange )
	{
		gWorldFx.ReOrientVector( old_pos.node, particle.position.node, particle.velocity );
	}

	// If we need to collide then handle the case for colliding
	if( m_bCollide )
	{
		// If the particle should collide then do some expensive calculations
		if( particle.should_collide )
		{
			vector_3	contactNormal( DoNotInitialize );
			SiegePos	contactPoint;

			if( gWorldFx.RayCast( old_pos, particle.position, contactNormal, contactPoint ) )
			{
				// If particle is supposed to splat then orient and stick
				if( m_bSplat )
				{
					if( m_SplatSkipCount > m_SplatSkip )
					{
						m_SplatSkipCount	= 0;
						particle.alpha		= m_AlphaStart;
						particle.color		= m_SplatColor;

						particle.w			*= m_SplatScaleUp;
						particle.h			*= m_SplatScaleUp;

						particle.splat		= true;
						particle.velocity	= vector_3::ZERO;

						particle.position.pos	= contactPoint.pos + ( 0.001f * contactNormal );
						particle.position.node	= contactPoint.node;

						// orient the splat to the face normal of the polygon
						matrix_3x3 terrain( MatrixFromDirection( contactNormal ) );

						// rotate in z by some random amount so the splats don't look totally uniform
						terrain = terrain * ZRotationColumns( gWorldFx.GetRNG().Random( PI2 ) );

						const vector_3 w_xaxis( particle.w * terrain.GetColumn0_T() );
						const vector_3 h_yaxis( particle.h * terrain.GetColumn1_T() );

						// Center the middle of the splat at 0,0
						particle.splat_verts[0] = m_Scale * (2.0f/3.41421f) * ( -w_xaxis + h_yaxis );
						particle.splat_verts[1] = m_Scale * (2.0f/3.41421f) * ( -w_xaxis - (2.41421f*h_yaxis) );
						particle.splat_verts[2] = m_Scale * (2.0f/3.41421f) * ( (2.41421f*w_xaxis) + h_yaxis );
					}
					else
					{
						++m_SplatSkipCount;

						particle.alpha = 0;
					}
				}
				else
				{
					// Check to see if the velocity should be reoriented again
					if( particle.position.node != contactPoint.node )
					{
						gWorldFx.ReOrientVector( particle.position.node, contactPoint.node, particle.velocity );
						bNodeChange = true;
					}

					particle.position = contactPoint;

					// Calculate reflection and rebound
					vector_3 i( DoNotInitialize );
					if( !particle.velocity.IsZero() )
					{
						i = Normalize( particle.velocity );
					}

					const float s = InnerProduct( contactNormal, i );

					if( s < 0 )
					{
						vector_3 p = i-contactNormal*s;
						vector_3 q = contactNormal*s;
						vector_3 restitution = m_Restitution*(p - q);
						particle.velocity = restitution*(Length(particle.velocity));
						particle.velocity += m_Gravity * updateTime;
					}
				}
			}
		}
	}
	return bNodeChange;
}


void
Explosion_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();

	// Setup the draw mode
	if( m_bDarkBlending )
	{
		renderer.SetTextureStageState(	0,
										D3DTOP_ADD,
										D3DTOP_MODULATE,

										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );
	}
	else
	{
		renderer.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										D3DTOP_MODULATE,

										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_ONE,
										false );
	}


	// Setup some storage for points
	vector_3	p0( DoNotInitialize );
	vector_3	p1( DoNotInitialize );
	vector_3	p2( DoNotInitialize );

	ParticleMap::iterator iList = m_Particles.begin(), iListEnd = m_Particles.end();

	for(; iList != iListEnd; ++iList )
	{
		// Make sure this is in a valid node otherwise abort this effect
		if( !gWorldFx.IsNodeVisible( (*iList).first ) )
		{
			continue;
		}

		// Setup the world matrix for the current node
		gWorldFx.GetNodeOrientAndCenter( (*iList).first, m_NodeOrient, m_NodeCentroid );
		renderer.SetWorldMatrix( m_NodeOrient, m_NodeCentroid );

		// Get camera orientation so we can orient to the camera for the current node
		m_Cam = Transpose( m_NodeOrient ) * gWorldFx.GetCameraOrientation();

		m_Vx = m_Cam.GetColumn_0();
		m_Vy = m_Cam.GetColumn_1();

		// Draw moving particles with the current settings
		ParticleList::iterator iParticle = (*iList).second.begin(), iParticleEnd = (*iList).second.end();
		UINT32 i = 0;
		sVertex* verts = &*m_DrawVerts.begin();

		for(; iParticle != iParticleEnd; ++iParticle, i += 3 )
		{
			// Get the vertex color
			DWORD color = MAKEDWORDCOLOR( (*iParticle).color );
			color		= ((color&0x00FFFFFF)|((BYTE((*iParticle).alpha*m_NodeAlpha*255.0f))<<24));

			// Calculate the verticies for the triangle
			const vector_3 wVx( (*iParticle).w * m_Vx );
			const vector_3 hVy( (*iParticle).h * m_Vy );
			const vector_3 pos( (*iParticle).position.pos );

			// construct the 3 points of the particle. This needs to be offset slightly up in the x direction
			// and down in the y direction to keep it centered.
			p0 = pos + m_Scale * ( (-0.5f * wVx) + (0.5f * hVy) );
			p1 = pos + m_Scale * ( (-0.5f * wVx) - (1.5f * hVy) );
			p2 = pos + m_Scale * ( ( 1.5f * wVx) + (0.5f * hVy) );

			// Set the actual points to draw
			verts[i+0].x		= p0.x;		verts[i+1].x		= p2.x;
			verts[i+0].y		= p0.y;		verts[i+1].y		= p2.y;
			verts[i+0].z		= p0.z;		verts[i+1].z		= p2.z;
			verts[i+0].color	= color;	verts[i+1].color	= color;

			verts[i+2].x		= p1.x;
			verts[i+2].y		= p1.y;
			verts[i+2].z		= p1.z;
			verts[i+2].color	= color;
		}

		// Draw moving particles
		if( i >= 3 )
		{
			renderer.DrawPrimitive( D3DPT_TRIANGLELIST, &*m_DrawVerts.begin(), i, SVERTEX, &m_TextureName, 1 );
		}
	}


	// Draw the splats
	if( m_bSplat && !m_SplatParticles.empty() )
	{
		m_DrawVerts.clear();

		iList = m_SplatParticles.begin(), iListEnd = m_SplatParticles.end();

		for(; iList != iListEnd; ++iList )
		{
			// Make sure this is in a valid node otherwise abort this effect
			if( !gWorldFx.IsNodeVisible( (*iList).first ) )
			{
				EndThisEffect();
				return;
			}

			// Setup the world matrix for the current node
			gWorldFx.GetNodeOrientAndCenter( (*iList).first, m_NodeOrient, m_NodeCentroid );
			renderer.SetWorldMatrix( m_NodeOrient, m_NodeCentroid );

			// Get camera orientation so we can orient to the camera for the current node
			m_Cam = Transpose( m_NodeOrient ) * gWorldFx.GetCameraOrientation();

			m_Vx = m_Cam.GetColumn_0();
			m_Vy = m_Cam.GetColumn_1();


			// Draw moving particles with the current settings
			ParticleList::iterator iParticle = (*iList).second.begin(), iParticleEnd = (*iList).second.end();
			UINT32 i = 0;
			sVertex* verts = &*m_DrawVerts.begin();

			for(; iParticle != iParticleEnd; ++iParticle, i += 3 )
			{
				// Get the vertex color
				DWORD color = MAKEDWORDCOLOR( (*iParticle).color );
				color		= ((color&0x00FFFFFF)|((BYTE((*iParticle).alpha*m_NodeAlpha*255.0f))<<24));


				// Setup particle position
				const vector_3 splat_pos( (*iParticle).position.pos );

				p0 = ( splat_pos + (*iParticle).splat_verts[0] );
				p1 = ( splat_pos + (*iParticle).splat_verts[1] );
				p2 = ( splat_pos + (*iParticle).splat_verts[2] );

		
				// Set the actual points to draw
				verts[i+0].x		= p0.x;		verts[i+1].x		= p2.x;
				verts[i+0].y		= p0.y;		verts[i+1].y		= p2.y;
				verts[i+0].z		= p0.z;		verts[i+1].z		= p2.z;
				verts[i+0].color	= color;	verts[i+1].color	= color;

				verts[i+2].x		= p1.x;
				verts[i+2].y		= p1.y;
				verts[i+2].z		= p1.z;
				verts[i+2].color	= color;
			}

			// Draw splats
			if( i >= 3 )
			{
				renderer.DrawPrimitive( D3DPT_TRIANGLELIST, &*m_DrawVerts.begin(), i, SVERTEX, &m_SplatTexture, 1 );
			}
		}
	}

	gWorldFx.SaveRenderingState();
}


float
Explosion_Effect::GetBoundingRadius( void )
{
	return 0.25f + (2.0f * m_Radius * m_Scale);
}





// Spatiotemporal Pattern Effect ---
SPE_Effect::SPE_Effect( void )
{
	m_Max_sparkles	=	0;

	m_Alpha_fade	=	0;
	m_In_dur		=	0;
	m_Out_dur		=	0;

	m_wh			=	0;
	m_Alpha			=	0;
	m_Radius		=	0;

	m_XIndex[ 0 ]	=	0;
	m_XIndex[ 1 ]	=	0;
	m_YIndex[ 0 ]	=	0;
	m_YIndex[ 1 ]	=	0;
	m_ZIndex[ 0 ]	=	0;
	m_ZIndex[ 1 ]	=	0;

	m_XSpeed[ 0 ]	=	0;
	m_XSpeed[ 1 ]	=	0;
	m_YSpeed[ 0 ]	=	0;
	m_YSpeed[ 1 ]	=	0;
	m_ZSpeed[ 0 ]	=	0;
	m_ZSpeed[ 1 ]	=	0;

	m_XSpace[ 0 ]	=	0;
	m_XSpace[ 1 ]	=	0;
	m_YSpace[ 0 ]	=	0;
	m_YSpace[ 1 ]	=	0;
	m_ZSpace[ 0 ]	=	0;
	m_ZSpace[ 1 ]	=	0;

	m_IndexCount	=	0;
}


void
SPE_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.1f, 0.3f, 0.9f);			// PARAM(1)
	params.SetVector3	( "index0",		0.000f, 0.000f, 1.570f	);	// PARAM(2)
	params.SetVector3	( "index1",		0.000f, 0.000f, 1.570f	);	// PARAM(3)
	params.SetVector3	( "speed0",		0.000f, 1.000f, 0.000f	);	// PARAM(4)
	params.SetVector3	( "speed1",		0.000f, 1.000f, 0.000f	);	// PARAM(5)
	params.SetVector3	( "space0",		0.098f, 0.000f, 0.098f	);	// PARAM(6)
	params.SetVector3	( "space1",		0.098f, 0.000f, 0.098f	);	// PARAM(7)

	params.SetUINT32	( "count",		64				);			// PARAM(8)
	
	params.SetFloat		( "alpha_fade",	0.16f			);			// PARAM(9)
	params.SetFloat		( "radius",		0.6f			);			// PARAM(10)
	params.SetFloat		( "tin",		1.0f			);			// PARAM(11)
	params.SetFloat		( "tout",		1.0f			);			// PARAM(12)

	// Overrides
	params.SetFloat		( "scale",		0.12f			);			// 6
}


void
SPE_Effect::InitializeDerived( void )
{
	vector_3 temp0(DoNotInitialize);
	vector_3 temp1(DoNotInitialize);
	m_Params.GetVector3	( PARAM(1),		m_Base_color );
	m_Params.GetVector3	( PARAM(2),		temp0	);
	m_Params.GetVector3	( PARAM(3),		temp1	);
	m_XIndex[0] = temp0.x;
	m_XIndex[1] = temp1.x;
	m_YIndex[0] = temp0.y;
	m_YIndex[1] = temp1.y;
	m_ZIndex[0] = temp0.z;
	m_ZIndex[1] = temp1.z;
	m_Params.GetVector3	( PARAM(4),		temp0	);
	m_Params.GetVector3	( PARAM(5),		temp1	);
	m_XSpeed[0] = temp0.x;
	m_XSpeed[1] = temp1.x;
	m_YSpeed[0] = temp0.y;
	m_YSpeed[1] = temp1.y;
	m_ZSpeed[0] = temp0.z;
	m_ZSpeed[1] = temp1.z;
	m_Params.GetVector3	( PARAM(6),		temp0	);
	m_Params.GetVector3	( PARAM(7),		temp1	);
	m_XSpace[0] = temp0.x;
	m_XSpace[1] = temp1.x;
	m_YSpace[0] = temp0.y;
	m_YSpace[1] = temp1.y;
	m_ZSpace[0] = temp0.z;
	m_ZSpace[1] = temp1.z;

	m_Params.GetUINT32	( PARAM(8),		m_Max_sparkles	); 
	m_Max_sparkles = min_t( m_Max_sparkles, (UINT32)Flamethrower::MAX_POINT_INDEX );
	
	m_Params.GetFloat	( PARAM(9),		m_Alpha_fade	);
	m_Params.GetFloat	( PARAM(10),	m_Radius		);
	m_Params.GetFloat	( PARAM(11),	m_In_dur		);
	m_Params.GetFloat	( PARAM(12),	m_Out_dur		);

	m_Params.GetFloat	( 6,			m_wh			);

	m_Verts.resize( m_Max_sparkles * 4 );
	m_IndexCount = 6 * (m_Verts.size() >> 2);

	VertColl::iterator it = m_Verts.begin(), it_end = m_Verts.end();

	while( it != it_end )
	{
		(*it).uv.u	= 0.0f;
		(*it).uv.v	= 1.0f;
		++it;

		(*it).uv.u	= 0.0f;
		(*it).uv.v	= 0.0f;
		++it;

		(*it).uv.u	= 1.0f;
		(*it).uv.v	= 1.0f;
		++it;

		(*it).uv.u	= 1.0f;
		(*it).uv.v	= 0.0f;
		++it;
	}


	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if(( m_Alpha_fade == 0 )||( m_Duration == -1.0f ))
	{
		m_Alpha = 1.0f;
	}
	if( m_In_dur == 0 )
	{
		m_In_dur = 0.001f;
	}
	if( m_Out_dur == 0 )
	{
		m_Out_dur = 0.001f;
	}
}


UINT32
SPE_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	return m_TextureName;
}


bool
SPE_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_Max_sparkles",	m_Max_sparkles );

	persist.Xfer( "m_Base_color",	m_Base_color	);
	persist.Xfer( "m_Alpha_fade",	m_Alpha_fade	);
	persist.Xfer( "m_In_dur",		m_In_dur		);
	persist.Xfer( "m_Out_dur",		m_Out_dur		);

	persist.Xfer( "m_wh",			m_wh			);
	persist.Xfer( "m_Alpha",		m_Alpha			);
	persist.Xfer( "m_Radius",		m_Radius		);

	persist.Xfer( "m_XIndex0",		m_XIndex[0]		);
	persist.Xfer( "m_XIndex1",		m_XIndex[1]		);
	persist.Xfer( "m_YIndex0",		m_YIndex[0]		);
	persist.Xfer( "m_YIndex1",		m_YIndex[1]		);
	persist.Xfer( "m_ZIndex0",		m_ZIndex[0]		);
	persist.Xfer( "m_ZIndex1",		m_ZIndex[1]		);

	persist.Xfer( "m_XSpeed0",		m_XSpeed[0]		);
	persist.Xfer( "m_XSpeed1",		m_XSpeed[1]		);
	persist.Xfer( "m_YSpeed0",		m_YSpeed[0]		);
	persist.Xfer( "m_YSpeed1",		m_YSpeed[1]		);
	persist.Xfer( "m_ZSpeed0",		m_ZSpeed[0]		);
	persist.Xfer( "m_ZSpeed1",		m_ZSpeed[1]		);

	persist.Xfer( "m_XSpace0",		m_XSpace[0]		);
	persist.Xfer( "m_XSpace1",		m_XSpace[1]		);
	persist.Xfer( "m_YSpace0",		m_YSpace[0]		);
	persist.Xfer( "m_YSpace1",		m_YSpace[1]		);
	persist.Xfer( "m_ZSpace0",		m_ZSpace[0]		);
	persist.Xfer( "m_ZSpace1",		m_ZSpace[1]		);

	persist.Xfer( "m_OldXIndex0",	m_OldXIndex[0]	);
	persist.Xfer( "m_OldXIndex1",	m_OldXIndex[1]	);
	persist.Xfer( "m_OldYIndex0",	m_OldYIndex[0]	);
	persist.Xfer( "m_OldYIndex1",	m_OldYIndex[1]	);
	persist.Xfer( "m_OldZIndex0",	m_OldZIndex[0]	);
	persist.Xfer( "m_OldZIndex1",	m_OldZIndex[1]	);

	persist.Xfer( "m_IndexCount",	m_IndexCount	);

	if( persist.IsRestoring() )
	{
		gpassert( m_Verts.empty() );
		m_Verts.resize( m_Max_sparkles * 4 );
		gpassert( m_IndexCount == (6 * (m_Verts.size() >> 2)) );

		VertColl::iterator it = m_Verts.begin(), it_end = m_Verts.end();
		while( it != it_end )
		{
			(*it).uv.u	= 0.0f;
			(*it).uv.v	= 1.0f;
			++it;

			(*it).uv.u	= 0.0f;
			(*it).uv.v	= 0.0f;
			++it;

			(*it).uv.u	= 1.0f;
			(*it).uv.v	= 1.0f;
			++it;

			(*it).uv.u	= 1.0f;
			(*it).uv.v	= 0.0f;
			++it;
		}
	}

	return true;
}


void
SPE_Effect::UpdateDerived( void )
{
	matrix_3x3	newOrient;
	bool		bGotOrient;

	// try to get the orientation of the target, need to handle the case where we can't get one.
	bGotOrient = m_Targets->GetPlacement( NULL, &newOrient, m_OrientMode );

	m_OldXIndex[0] = m_XIndex[0];
	m_OldYIndex[0] = m_YIndex[0];
	m_OldZIndex[0] = m_ZIndex[0];
	m_OldXIndex[1] = m_XIndex[1];
	m_OldYIndex[1] = m_YIndex[1];
	m_OldZIndex[1] = m_ZIndex[1];

	DWORD color = MAKEDWORDCOLOR( m_Base_color );
	color		= ((color&0x00FFFFFF)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

	vector_3		temp_pos( DoNotInitialize );
	vector_3		pos( DoNotInitialize );

	// set up some vectors to construct the forward facing poly.
	const vector_3	x_axis( m_wh * m_Vx );
	const vector_3	y_axis( m_wh * m_Vy );

	const vector_3	f1(-x_axis +y_axis);
	const vector_3	f2(-x_axis -y_axis);
	const vector_3	f3( x_axis +y_axis);
	const vector_3	f4( x_axis -y_axis);

	const float rval = m_Radius * 0.5f;

	VertColl::iterator it = m_Verts.begin(), it_end = m_Verts.end();

	while( it != it_end )
	{
		// Calculate the Position
		temp_pos.x = rval * ( SINF(m_XIndex[0]) + SINF(m_XIndex[1]) );
		temp_pos.y = rval * ( SINF(m_YIndex[0]) + SINF(m_YIndex[1]) );
		temp_pos.z = rval * ( SINF(m_ZIndex[0]) + SINF(m_ZIndex[1]) );

		// Orient to the target, this also works with bone and world orient.
		pos = bGotOrient ? ( newOrient * temp_pos ) : temp_pos;

		// construct the particle
		const vector_3 vp0( pos + f1 );
		const vector_3 vp1( pos + f2 );
		const vector_3 vp2( pos + f3 );
		const vector_3 vp3( pos + f4 );

		(*it).x		= vp0.x;
		(*it).y		= vp0.y;
		(*it).z		= vp0.z;
		(*it).color = color;
		++it;

		(*it).x		= vp1.x;
		(*it).y		= vp1.y;
		(*it).z		= vp1.z;
		(*it).color = color;
		++it;

		(*it).x		= vp2.x;
		(*it).y		= vp2.y;
		(*it).z		= vp2.z;
		(*it).color = color;
		++it;

		(*it).x		= vp3.x;
		(*it).y		= vp3.y;
		(*it).z		= vp3.z;
		(*it).color = color;
		++it;

		m_XIndex[0] += m_XSpace[0];
		m_XIndex[1] += m_XSpace[1];
		m_YIndex[0] += m_YSpace[0];
		m_YIndex[1] += m_YSpace[1];
		m_ZIndex[0] += m_ZSpace[0];
		m_ZIndex[1] += m_ZSpace[1];
	}

	m_XIndex[0] = m_OldXIndex[0] + m_XSpeed[0] * m_FrameSync;
	m_YIndex[0] = m_OldYIndex[0] + m_YSpeed[0] * m_FrameSync;
	m_ZIndex[0] = m_OldZIndex[0] + m_ZSpeed[0] * m_FrameSync;
	m_XIndex[1] = m_OldXIndex[1] + m_XSpeed[1] * m_FrameSync;
	m_YIndex[1] = m_OldYIndex[1] + m_YSpeed[1] * m_FrameSync;
	m_ZIndex[1] = m_OldZIndex[1] + m_ZSpeed[1] * m_FrameSync;

	if( m_Duration != -1.0f )
	{
		if( m_bFinishing || (m_Elapsed >= (m_Duration-m_Out_dur)) )
		{
			m_Alpha = (m_Duration - m_Elapsed) / m_Out_dur;
			m_bFinishing = true;
		}
		else
		{
			m_Alpha = (m_Alpha <= 1.0f) ? ((m_Elapsed / m_In_dur) + 0.00001f) : 1.0f;
		}
	}

	m_Alpha = FilterClamp( 0.0f, 1.0f, m_Alpha );

	m_bFinished = ( (m_bFinished) | (m_Alpha <= 0.0f) );

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
SPE_Effect::DrawSelf( void )
{
	gWorldFx.GetRenderer().DrawIndexedPrimitive( D3DPT_TRIANGLELIST, &(*m_Verts.begin()),
												 m_Verts.size(), SVERTEX, gWorldFx.GetGlobalDrawIndexArray(),
												 m_IndexCount, &m_TextureName, 1 );
}


float
SPE_Effect::GetBoundingRadius( void )
{
	return m_Radius * m_Scale;
}




// Orbiter Effect---
Orbiter_Effect::Orbiter_Effect( void )
{
	m_Radius				= 0;
	m_iRadius				= 0;
	m_Theta					= 0;
	m_iTheta				= 0;
	m_Phi					= 0;
	m_iPhi					= 0;
	m_Vdisplace				= 0;

	m_ModelGoid				= GOID_INVALID;

	m_bSmooth				= false;
	m_Smooth_history		= 0;

	m_bFreeSpin				= false;
	m_bOrientRot			= false;

	m_Vpos					= 0;
	m_bInvisible			= false;

	m_wh					= 0;
	m_Alpha					= 0;
	m_In_dur				= 0;
	m_Out_dur				= 0;

	m_Model_scale_original	= 0;
	m_Model_scale			= 0;
	m_Model_smax			= 0;
	m_Model_sin				= 0;
	m_Model_sout			= 0;
}


Orbiter_Effect::~Orbiter_Effect( void )
{
	if( m_ModelGoid != GOID_INVALID )
	{
		gWorldFx.DestroyEffectObject( m_ModelGoid );
	}
}


void
Orbiter_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	(	"color0",		0.2f, 1.0f, 0.2f); // PARAM(1)
	params.SetVector3	(	"rot_inc",		0.0f, 0.0f, 0.0f); // PARAM(2)

	params.SetGPString	(	"model",		""				); // PARAM(3)

	params.SetFloat		(	"model_sin",	1.0f			); // PARAM(4)
	params.SetFloat		(	"model_sout",	1.0f			); // PARAM(5)
	params.SetFloat		(	"model_smax",	1.0f			); // PARAM(6)
	params.SetFloat		(	"radius",		1.0f			); // PARAM(7)
	params.SetFloat		(	"radiusi",		0.0f			); // PARAM(8)

	params.SetFloat		(	"theta",		0.0f			); // PARAM(9)
	params.SetFloat		(	"itheta",		0.0f			); // PARAM(10)
	params.SetFloat		(	"phi",			0.0f			); // PARAM(11)
	params.SetFloat		(	"iphi",			2.0f			); // PARAM(12)
	params.SetFloat		(	"tin",			1.0f			); // PARAM(13)
	params.SetFloat		(	"tout",			1.0f			); // PARAM(14)
	params.SetFloat		(	"vdisplace",	0.0f			); // PARAM(15)
	params.SetFloat		(	"shistory",		16.0f			); // PARAM(16)

	params.SetBool		(	"invisible",	false			); // PARAM(17)
	params.SetBool		(	"smooth",		false			); // PARAM(18)
	params.SetBool		(	"free",			false			); // PARAM(19)
	params.SetBool		(	"orientrot",	false			); // PARAM(20)

	// Overrides
	params.SetFloat		(	"scale",		1.0f			); // 6
}


void
Orbiter_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	(	PARAM(1),	m_Base_color	);
	m_Params.GetVector3	(	PARAM(2),	m_RotationInc	);

	m_Params.GetGPString(	PARAM(3),	m_sModelName	);

	m_Params.GetFloat	(	PARAM(4),	m_Model_sin		);
	m_Params.GetFloat	(	PARAM(5),	m_Model_sout	);
	m_Params.GetFloat	(	PARAM(6),	m_Model_smax	);
	m_Params.GetFloat	(	PARAM(7),	m_Radius		);
	m_Params.GetFloat	(	PARAM(8),	m_iRadius		);

	m_Params.GetFloat	(	PARAM(9),	m_Theta			);
	m_Params.GetFloat	(	PARAM(10),	m_iTheta		);
	m_Params.GetFloat	(	PARAM(11),	m_Phi			);
	m_Params.GetFloat	(	PARAM(12),	m_iPhi			);
	m_Params.GetFloat	(	PARAM(13),	m_In_dur		);
	m_Params.GetFloat	(	PARAM(14),	m_Out_dur		);
	m_Params.GetFloat	(	PARAM(15),	m_Vdisplace		);
	m_Params.GetFloat	(	PARAM(16),	m_Smooth_history);

	m_Params.GetBool	(	PARAM(17),	m_bInvisible	);
	m_Params.GetBool	(	PARAM(18),	m_bSmooth		);
	m_Params.GetBool	(	PARAM(19),	m_bFreeSpin		);
	m_Params.GetBool	(	PARAM(20),	m_bOrientRot	);

	m_Params.GetFloat	(	6,			m_Scale			);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_RotationInc *= RealPi / 180.0f;
	m_Radius *= m_Scale;


	m_Vpos = 0;
	m_wh = 0.5f;

	if( m_Duration == -1.0f )
	{
		m_Alpha = 1.0f;
	}
	else
	{
		m_Alpha = 0.0f;
	}

	if( m_In_dur == 0 )
	{
		m_In_dur = 0.001f;
	}

	if( m_Out_dur == 0 )
	{
		m_Out_dur = 0.001f;
	}
	m_Model_scale = 1.0f;
	m_Model_sin = max( FLOAT_TOLERANCE, m_Model_sin );
}


UINT32
Orbiter_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	return m_TextureName;
}


bool
Orbiter_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_Base_color",		m_Base_color		);
	persist.Xfer( "m_Radius",			m_Radius			);
	persist.Xfer( "m_iRadius",			m_iRadius			);
	persist.Xfer( "m_Theta",			m_Theta				);
	persist.Xfer( "m_iTheta",			m_iTheta			);
	persist.Xfer( "m_Phi",				m_Phi				);
	persist.Xfer( "m_iPhi",				m_iPhi				);
	persist.Xfer( "m_Vdisplace",		m_Vdisplace			);

	persist.Xfer( "m_sModelName",		m_sModelName		);
	persist.Xfer( "m_ModelGoid",		m_ModelGoid			);

	persist.Xfer( "m_bSmooth",			m_bSmooth			);
	persist.Xfer( "m_Smooth_history",	m_Smooth_history	);

	persist.Xfer( "m_bFreeSpin",		m_bFreeSpin			);
	persist.Xfer( "m_bOrientRot",		m_bOrientRot		);

	persist.Xfer( "m_Vpos",				m_Vpos				);
	persist.Xfer( "m_bInvisible",		m_bInvisible		);

	persist.Xfer( "m_wh",				m_wh				);
	persist.Xfer( "m_Alpha",			m_Alpha				);
	persist.Xfer( "m_In_dur",			m_In_dur			);
	persist.Xfer( "m_Out_dur",			m_Out_dur			);

	persist.Xfer( "m_RotationInc",		m_RotationInc		);
	persist.Xfer( "m_Model_scale_original",	m_Model_scale_original );
	persist.Xfer( "m_Model_scale",		m_Model_scale		 );
	persist.Xfer( "m_Model_sin",		m_Model_sin			);
	persist.Xfer( "m_Model_sout",		m_Model_sout		);
	persist.Xfer( "m_Model_smax",		m_Model_smax		);

	persist.Xfer( "m_ModelOrient",		m_ModelOrient		);

	persist.Xfer( "m_LastPos",			m_LastPos			);

	persist.XferList( "m_PosHistory",	m_PosHistory		);

	return true;
}


void
Orbiter_Effect::Track( void )
{
	if( (m_Elapsed >= m_Duration) && (m_Duration != -1.0f) )
	{
		EndThisEffect();
		return;
	}

	if( !m_bInitialized )
	{
		if( !m_Targets->GetPlacement( &m_Pos, NULL ) )
		{
			EndThisEffect();
			return;
		}
		m_LastPos = m_Pos;
	}

	SiegePos	targetPos( m_Pos );
	SiegePos	newPos;
	matrix_3x3	newOrient;

	if( m_Targets->GetPlacement( &newPos, &newOrient, m_OrientMode ) )
	{
		m_bHasMoved = ( targetPos != newPos );

		targetPos = newPos;

		if( m_bFreeSpin && (targetPos.node != m_LastPos.node) )
		{
			matrix_3x3	old_orient( gWorldFx.GetNodeOrientation( m_LastPos ) );
			matrix_3x3	new_orient( gWorldFx.GetNodeOrientation( targetPos ) );

			matrix_3x3	orient_difference( Transpose( new_orient * Transpose( old_orient ) ) );
			m_Orient	= orient_difference * m_Orient;

			m_LastPos	= targetPos;
		}

		// Calculate the radial offset
		m_Phi	+= m_FrameSync * m_iPhi;
		m_Theta	+= m_FrameSync * m_iTheta;
		m_Vpos	+= m_FrameSync * m_Vdisplace;
		m_Radius+= m_FrameSync * m_iRadius;

		float thetasin, thetacos;
		SINCOSF( m_Theta, thetasin, thetacos );
		float phisin, phicos;
		SINCOSF( m_Phi, phisin, phicos );

		vector_3	radPos( m_Radius * phisin * thetacos,
							m_Radius * phisin * thetasin + m_Vpos,
							m_Radius * phicos );

		// Get the orientation of the target if specified
		matrix_3x3 targetOrientation;

		if( m_bFreeSpin )
		{
			if( m_bOrientRot )
			{
				vector_3	heading( gSiegeEngine.GetDifferenceVector( m_LastPos, m_Pos ) );
				if( !heading.IsZero() )
				{
					m_ModelOrient = m_Orient * MatrixFromDirection( heading );
				}

				m_LastPos = m_Pos;
			}
			else
			{
				targetOrientation = m_Orient;
			}
		}
		else
		{
			targetOrientation = newOrient;
		}

		if( m_bSmooth )
		{
			SiegePos temp_pos( targetPos );

			if( gWorldFx.AddPositions( temp_pos, radPos, false, NULL ) )
			{
				m_PosHistory.push_front( temp_pos  );
				if( m_PosHistory.size() > m_Smooth_history )
				{
					m_PosHistory.pop_back();
				}

				// Apply smoothing
				vector_3 avg_pos;
				int num_positions	= 0;

				std::list< SiegePos >::iterator i = m_PosHistory.begin(), iend = m_PosHistory.end();

				for( ; i != iend; ++i )
				{
					if( gWorldFx.IsNodeVisible( (*i).node ) )
					{
						avg_pos += (*i).WorldPos();
						++num_positions;
					}
				}

				if( num_positions )
				{
					avg_pos /= (float)num_positions;
				}

				m_Pos.FromWorldPos( avg_pos + ( targetOrientation * m_Offset ), newPos.node );
			}
		}
		else
		{
			if( gWorldFx.AddPositions( targetPos, ( targetOrientation * (m_Offset + radPos) ), m_bFast, NULL ) )
			{
				m_Pos = targetPos;
			}
		}
	}
	else
	{
		EndThisEffect();
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}



void
Orbiter_Effect::SetFinishing( void )
{
	m_Duration	= m_Model_sin + 2.0f * m_Model_sout;
	m_Elapsed	= m_Model_sout;
}


void
Orbiter_Effect::UpdateDerived( void )
{
	if( m_bInvisible )
	{
		return;
	}

	if( !m_bInitialized )
	{
		m_bInitialized = true;

		SiegePos temp_pos;
		m_Targets->GetPlacement( &temp_pos, NULL );
		m_LastPos = m_Pos;

		if( m_sModelName.empty() )
		{
			m_ModelGoid = GOID_INVALID;
		}
		else
		{
			m_ModelGoid = gWorldFx.CreateEffectObject( m_OwnerID, m_sModelName, m_Pos );

			if( m_ModelGoid == GOID_INVALID )
			{
				EndThisEffect();
				return;
			}

			gWorldFx.RotateEffectObject( m_ModelGoid, m_RotationAngles );

			m_Model_scale_original = gWorldFx.GetEffectObjectScale( m_ModelGoid );
		}
	}

	if( m_Duration != -1.0f )
	{
		if( m_bFinishing || (m_Elapsed >= (m_Duration-m_Out_dur)) )
		{
			m_Alpha = (m_Duration - m_Elapsed) / m_Out_dur;
			m_bFinishing = true;
		}
		else
		{
			m_Alpha = ( m_Alpha <= 1.0f ) ? ( (m_Elapsed / m_In_dur) + 0.00001f):1.0f;
		}
	}

	m_Alpha = FilterClamp( 0.0f, 1.0f, m_Alpha );

	m_bFinished = ( (m_bFinished) | (m_Alpha <= 0.0f) );

	// If there is a model associated with this orbiter then make sure that the position is being updated
	if( m_ModelGoid != GOID_INVALID )
	{
		// Set placement information
		gWorldFx.SetEffectObjectPosition( m_ModelGoid, m_Pos );

		if( m_bOrientRot )
		{
			gWorldFx.SetEffectObjectOrientation( m_ModelGoid, m_ModelOrient );
			gWorldFx.RotateEffectObject( m_ModelGoid, m_RotationAngles );
		}
		else
		{
			gWorldFx.RotateEffectObject( m_ModelGoid, m_RotationInc*m_FrameSync );
		}

		// Set scaling information
		if( m_Duration != -1.0f )
		{
			if( m_bFinishing || (m_Elapsed >= (m_Duration - m_Model_sout)) )
			{
				// Scaling out
				m_Model_scale = (m_Duration - m_Elapsed) / m_Model_sout;
			}
			else if( m_Elapsed <= m_Model_sin )
			{
				// Scaling in
				m_Model_scale = (m_Elapsed / m_Model_sin);
			}
		}

		float new_scale = ( m_Model_scale * m_Model_scale_original ) + FLOAT_TOLERANCE;

		if( new_scale != 1.0f && ( new_scale != m_Model_scale ) )
		{
			m_Model_scale = new_scale;

			if( new_scale <= 0 )
			{
				new_scale = FLOAT_TOLERANCE;
			}
			gWorldFx.SetEffectObjectScale( m_ModelGoid, new_scale * m_Model_smax );
		}
	}
}


void
Orbiter_Effect::DrawSelf( void )
{
	if( m_bFinished || m_bInvisible )
	{
		return;
	}

	// If we aren't using a model then we have to draw something

	if( m_sModelName.empty() )
	{
		Rapi&	renderer = gWorldFx.GetRenderer();

		vector_3 vp0 = m_Scale * (-m_wh*m_Vx +m_wh*m_Vy);
		vector_3 vp1 = m_Scale * (-m_wh*m_Vx -m_wh*m_Vy);
		vector_3 vp2 = m_Scale * ( m_wh*m_Vx +m_wh*m_Vy);
		vector_3 vp3 = m_Scale * ( m_wh*m_Vx -m_wh*m_Vy);

		sVertex	drawVerts[4];
		memset( drawVerts, 0, sizeof( sVertex )*4 );


		DWORD color	= MAKEDWORDCOLOR( m_Base_color );
		color		= ((color&0x00FFFFFF)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

		drawVerts[0].x			= vp0.x;
		drawVerts[0].y			= vp0.y;
		drawVerts[0].z			= vp0.z;
		drawVerts[0].color		= color;
		drawVerts[0].uv.u		= 0.0f;
		drawVerts[0].uv.v		= 1.0f;

		drawVerts[1].x			= vp2.x;
		drawVerts[1].y			= vp2.y;
		drawVerts[1].z			= vp2.z;
		drawVerts[1].color		= color;
		drawVerts[1].uv.u		= 1.0f;
		drawVerts[1].uv.v		= 1.0f;

		drawVerts[2].x			= vp1.x;
		drawVerts[2].y			= vp1.y;
		drawVerts[2].z			= vp1.z;
		drawVerts[2].color		= color;
		drawVerts[2].uv.u		= 0.0f;
		drawVerts[2].uv.v		= 0.0f;

		drawVerts[3].x			= vp3.x;
		drawVerts[3].y			= vp3.y;
		drawVerts[3].z			= vp3.z;
		drawVerts[3].color		= color;
		drawVerts[3].uv.u		= 1.0f;
		drawVerts[3].uv.v		= 0.0f;

		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );

		if( !m_bUseLoadedTexture )
		{
			color		= ((0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*160.0f))<<24));

			drawVerts[0].x			= vp0.x;
			drawVerts[0].y			= vp0.y;
			drawVerts[0].z			= vp0.z;
			drawVerts[0].color		= color;
			drawVerts[0].uv.u		= 0.0f;
			drawVerts[0].uv.v		= 1.0f;

			drawVerts[1].x			= vp2.x;
			drawVerts[1].y			= vp2.y;
			drawVerts[1].z			= vp2.z;
			drawVerts[1].color		= color;
			drawVerts[1].uv.u		= 1.0f;
			drawVerts[1].uv.v		= 1.0f;

			drawVerts[2].x			= vp1.x;
			drawVerts[2].y			= vp1.y;
			drawVerts[2].z			= vp1.z;
			drawVerts[2].color		= color;
			drawVerts[2].uv.u		= 0.0f;
			drawVerts[2].uv.v		= 0.0f;

			drawVerts[3].x			= vp3.x;
			drawVerts[3].y			= vp3.y;
			drawVerts[3].z			= vp3.z;
			drawVerts[3].color		= color;
			drawVerts[3].uv.u		= 1.0f;
			drawVerts[3].uv.v		= 0.0f;

			renderer.ScaleWorldMatrix( vector_3( 0.7f, 0.7f, 0.7f ) );
			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );

			color		= ((0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

			drawVerts[0].x			= vp0.x;
			drawVerts[0].y			= vp0.y;
			drawVerts[0].z			= vp0.z;
			drawVerts[0].color		= color;
			drawVerts[0].uv.u		= 0.0f;
			drawVerts[0].uv.v		= 1.0f;

			drawVerts[1].x			= vp2.x;
			drawVerts[1].y			= vp2.y;
			drawVerts[1].z			= vp2.z;
			drawVerts[1].color		= color;
			drawVerts[1].uv.u		= 1.0f;
			drawVerts[1].uv.v		= 1.0f;

			drawVerts[2].x			= vp1.x;
			drawVerts[2].y			= vp1.y;
			drawVerts[2].z			= vp1.z;
			drawVerts[2].color		= color;
			drawVerts[2].uv.u		= 0.0f;
			drawVerts[2].uv.v		= 0.0f;

			drawVerts[3].x			= vp3.x;
			drawVerts[3].y			= vp3.y;
			drawVerts[3].z			= vp3.z;
			drawVerts[3].color		= color;
			drawVerts[3].uv.u		= 1.0f;
			drawVerts[3].uv.v		= 0.0f;

			renderer.ScaleWorldMatrix( vector_3( 0.40f, 0.40f, 0.40f ) );
			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, drawVerts, 4, SVERTEX, &m_TextureName, 1 );
		}
	}
	else
	{
		// Nothing to do here since we aren't drawing anything
	}
}


float
Orbiter_Effect::GetBoundingRadius( void )
{
	return	m_wh*m_ScaleFactor*2.0f;
}
