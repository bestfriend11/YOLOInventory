//
//	Flamethrower_effects2.cpp
//	Copyright 1999, Gas Powered Games
//
//

#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "vector_3.h"
#include "space_3.h"
#include "filter_1.h"

#include "Flamethrower_effect_base.h"
#include "Flamethrower_effects2.h"
#include "WorldFx.h"
#include "Flamethrower_tracker.h"

#include "RapiOwner.h"

#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"

#include "namingkey.h"

// LineTracer effect ---


LineTracer_Effect::LineTracer_Effect( void )
{
	m_Fade_rate		= 0;
	m_fade_in		= 0;
	m_fade_out		= 0;
	m_Alpha_scalar	= 0;

	m_color0		= 0;
	m_color1		= 0;
}


void
LineTracer_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		1.0f, 1.0f, 1.0f ); // PARAM(1)
	params.SetVector3	( "color1",		1.0f, 1.0f, 1.0f ); // PARAM(2) 

	params.SetFloat		( "fade_rate",	0.10f );			// PARAM(3)
	params.SetFloat		( "tin",		0.5f );				// PARAM(4)
	params.SetFloat		( "tout",		0.5f );				// PARAM(5)

	params.SetBool		( "lock",		false );			// PARAM(6)
}


void
LineTracer_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color );
	m_color0 = MAKEDWORDCOLOR( m_Base_color );

	vector_3 temp(DoNotInitialize);
	m_Params.GetVector3	( PARAM(2),		temp );
	m_color1 = MAKEDWORDCOLOR( temp );

	m_Params.GetFloat	( PARAM(3),		m_Fade_rate		);
	m_Params.GetFloat	( PARAM(4),		m_fade_in		);
	m_Params.GetFloat	( PARAM(5),		m_fade_out		);

	m_Params.GetBool	( PARAM(6),		m_bLockToTarget );

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_bMustUpdate = true;
}


FUBI_DECLARE_SELF_TRAITS( LineTracer_Effect::Trace_element );

bool
LineTracer_Effect::Trace_element::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "alpha",		alpha );

	persist.Xfer		( "pos_a",		pos_a );
	persist.Xfer		( "pos_b",		pos_b );

	return true;
}


bool
LineTracer_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_Base_color",		m_Base_color		);
	persist.Xfer		( "m_Fade_rate",		m_Fade_rate			);
	persist.Xfer		( "m_fade_in",			m_fade_in			);
	persist.Xfer		( "m_fade_out",			m_fade_out			);
	persist.Xfer		( "m_Alpha_scalar",		m_Alpha_scalar		);
	persist.Xfer		( "m_bLockToTarget",	m_bLockToTarget		);

	persist.Xfer		( "m_RefPos",			m_RefPos			);
	persist.Xfer		( "m_RefPosNext",		m_RefPosNext		);

	persist.Xfer		( "m_color0",			m_color0			);
	persist.Xfer		( "m_color1",			m_color1			);

	persist.XferVector	( "m_Segments",			m_Segments			);

	return true;
}


void
LineTracer_Effect::UpdateDerived( void )
{
	// get the new positions for this frame if the effect is not yet ready to finish
	SiegePos point_a, point_b;

	bool bValidTargets = (  m_Targets->GetPlacement( &point_a, NULL, m_OrientMode, TattooTracker::TARGET ) &&
							m_Targets->GetPlacement( &point_b, NULL, m_OrientMode, TattooTracker::SOURCE ) );

	if( !gWorldFx.IsNodeVisible( point_a.node ) && !gWorldFx.IsNodeVisible( point_b.node ) )
	{
		return;
	}

	if( !m_bFinishing )
	{
		if(	bValidTargets )
		{
			if( m_bLockToTarget )
			{
				// get what will be the next reference position
				if( !m_Targets->GetPlacement( &m_RefPosNext, NULL, m_OrientMode, TattooTracker::REMAINING ) ||
					!gWorldFx.IsNodeVisible( m_RefPosNext.node ) )
				{
					// if the reference position target is gone or the node has been unloaded end this effect
					EndThisEffect();
					return;
				}
				m_Pos = m_RefPosNext;
			}
			else
			{
				m_Pos = point_a;
			}
		}
		else
		{
			// either point a or b is gone so end this effect
			EndThisEffect();
			return;
		}
	}

	// depending on what mode this effect is in store the positions appropriately
	if( m_bLockToTarget )
	{
		// correct for when the reference point enters a new node
		if( m_RefPos.node != m_RefPosNext.node )
		{
			// adjust all the old positions to be offsets relative to the new ref position in the new node
			matrix_3x3 diff_orient;
			gWorldFx.GetDifferenceOrient( m_RefPos.node, m_RefPosNext.node, diff_orient );

			// apply difference orientation to all offsets in the tracer collection
			TraceColl::iterator iTrace = m_Segments.begin(), iTrace_end = m_Segments.end();
			for(; iTrace != iTrace_end; ++iTrace )
			{
				(*iTrace).pos_a.pos = diff_orient * (*iTrace).pos_a.pos;
				(*iTrace).pos_b.pos = diff_orient * (*iTrace).pos_b.pos;
			}
		}

		// make sure not to add any new positions if the effect is in the middle of finishing
		if( !m_bFinishing )
		{
			// get the new entry in the collection
			Trace_element *pTracer = &*m_Segments.push_back();

			// validate the points again and check for zero vector length
			if( !m_RefPosNext.IsExactlyEqual( point_a ) )
			{
				const vector_3 diff( m_RefPosNext.pos - point_a.pos );

				// same position within tolerance - return
				if( diff.Length() < FLOAT_TOLERANCE )
				{
					return;
				}
			}
			else
			{
				return;
			}

			// validate for further zero vector length
			if( !m_RefPosNext.IsExactlyEqual( point_b ) )
			{
				const vector_3 diff( m_RefPosNext.pos - point_b.pos );

				// same position within tolerance - return
				if( diff.Length() < FLOAT_TOLERANCE )
				{
					return;
				}
			}
			else
			{
				return;
			}

			// all offsets are within the same node so just add it to the segment collection
			pTracer->pos_a.pos = gSiegeEngine.GetDifferenceVector( m_RefPosNext, point_a );
			pTracer->pos_b.pos = gSiegeEngine.GetDifferenceVector( m_RefPosNext, point_b );

			// save current orientation and position for next time through
			m_RefPos	= m_RefPosNext;
		}
	}
	else if( !m_bFinishing )
	{
		// get the new entry in the collection
		Trace_element *pTracer = &*m_Segments.push_back();

		pTracer->pos_a = point_a;
		pTracer->pos_b = point_b;
	}


	// not enough segments? then early out
	if( (m_Segments.size() <= 1) && !m_bFinishing )
	{
		return;
	}

	// determine correct alpha value
	float alpha = 0.0f;

	if( m_Duration != -1.0f )
	{
		if( m_Elapsed >= ( m_Duration - m_fade_out ) )
		{
			alpha = (m_Duration - m_Elapsed) / m_fade_out;
			m_bFinishing = true;
		}
	}

	// decrease alpha in each segment
	TraceColl::iterator iTrace = m_Segments.begin(), iTrace_end = m_Segments.end();
	while( iTrace != iTrace_end )
	{
		Trace_element &trace = *iTrace;

		// check position validity and erase bad invalids
		const bool bInvalidNodes = ( !gWorldFx.IsNodeVisible( trace.pos_a.node ) || !gWorldFx.IsNodeVisible( trace.pos_b.node ) ); 
		if( (trace.alpha <= 0) || (bInvalidNodes && !m_bLockToTarget) )
		{
			iTrace = m_Segments.erase( iTrace );
			iTrace_end = m_Segments.end();
		}
		else
		{
			if( alpha > 0.0f )
			{
				trace.alpha -= (1.0f - alpha);
			}
			else
			{
				trace.alpha -= m_FrameSync * m_Fade_rate;
			}
			trace.alpha = FilterClamp( 0.0f, 1.0f, trace.alpha );
			++iTrace;
		}
	}

	// all segments faded out?
	if( m_bFinishing && m_Segments.empty() )
	{
		EndThisEffect();
		return;
	}

	// determine alpha value for fade in/out transitionary stuff
	m_Alpha_scalar = m_Elapsed / m_fade_in;
	m_Alpha_scalar = FilterClamp( 0.0f, 1.0f, m_Alpha_scalar );

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
LineTracer_Effect::DrawSelf( void )
{
	if( m_bFinished || (m_Segments.size() < 3) )
	{
		return;
	}

	Rapi &renderer = gWorldFx.GetRenderer();

	// Disable texturing
	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_ONE,
									false );

	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,	D3DSHADE_GOURAUD );

	if( !m_bLockToTarget )
	{
		renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
	}

	// Draw
	const UINT32 lock_size		= m_Segments.size() * 2;
	unsigned int buffer_offset	= 0;
	sVertex	*pVertex = NULL;

	if( renderer.LockBufferedVertices( lock_size, buffer_offset, pVertex ) )
	{
		TraceColl::iterator iTrace = m_Segments.begin(), seg_end = m_Segments.end();
		
		unsigned int i = 0;
		for(; iTrace != seg_end; ++iTrace, i+=2 )
		{
			sVertex &vert0 = pVertex[ i ];
			sVertex &vert1 = pVertex[ i+1 ];

			const vector_3 a( m_bLockToTarget ? (m_RefPos.pos + (*iTrace).pos_a.pos) : (*iTrace).pos_a.WorldPos() );
			const vector_3 b( m_bLockToTarget ? (m_RefPos.pos + (*iTrace).pos_b.pos) : (*iTrace).pos_b.WorldPos() );

			vert0.x	= a.x;	vert1.x	= b.x;
			vert0.y	= a.y;	vert1.y	= b.y;
			vert0.z	= a.z;	vert1.z	= b.z;

			const DWORD vert_alpha = ( (BYTE((*iTrace).alpha*m_NodeAlpha*255.0f*m_Alpha_scalar)) << 24 );

			vert0.color	= ((m_color0&0x00FFFFFF)| vert_alpha );
			vert1.color	= ((m_color1&0x00FFFFFF)| vert_alpha );
		}

		renderer.UnlockBufferedVertices();
		renderer.SetTexture( 0, m_TextureName );
		renderer.SetTexture( 1, NULL );
		renderer.DrawBufferedPrimitive( D3DPT_TRIANGLESTRIP, buffer_offset, lock_size );
	}

	gWorldFx.SaveRenderingState();
}


bool
LineTracer_Effect::GetCullPosition( SiegePos & CullPosition )
{
	if( m_Segments.size() < 2 )
	{
		return false;
	}

	if( !m_bLockToTarget )
	{
		TraceColl::iterator a = m_Segments.begin(), b = m_Segments.end();
		--b;

		if( gWorldFx.IsNodeVisible( a->pos_a.node ) && gWorldFx.IsNodeVisible( b->pos_a.node ) )
		{
			if( m_Targets->GetPlacement( &CullPosition, NULL, m_OrientMode, TattooTracker::TARGET ) )
			{
				const vector_3 assumed_center( gSiegeEngine.GetDifferenceVector( (*a).pos_a, (*b).pos_a ) );
				m_BoundingRadius = assumed_center.Length();

				return ( gWorldFx.AddPositions( CullPosition, (0.5f * assumed_center ), false, NULL ) );
			}
		}
	}
	else
	{
		SiegePos a, b;
		if( m_Targets->GetPlacement( &a, NULL, m_OrientMode, TattooTracker::TARGET ) || 
			m_Targets->GetPlacement( &b, NULL, m_OrientMode, TattooTracker::SOURCE ) )
		{
			return (gWorldFx.IsNodeVisible( a.node ) || gWorldFx.IsNodeVisible( b.node ) );
		}
	}

	return false;
}



// FireB ---
FireB_Effect::FireB_Effect( void )
{
	m_AlphaFade					=	0;
	m_FlameSize					=	0;
	m_MinFlames					=	0;
	m_MaxFlames					=	0;
	m_bCheap					=	false;
	m_bDarkBlending				=	false;
	m_bConstantVelocity			=	false;
	m_Velocity_scalar			=	0;
	
	m_Lower_r0					=	0;
	m_Lower_r1					=	0;
	m_Upper_r0					=	0;
	m_Upper_r1					=	0;
	
	m_DisplaceMin				=	0;
	m_DisplaceMax				=	0;
	m_BurnStartRate				=	0;
	m_BurnEndRate				=	0;

	m_OBB_freq					=	0;
	m_OBB_time					=	0;

	m_FlameCount				=	0;
	m_bIgnite					=	false;
	m_bDamage					=	false;

	m_bLightSpawn				=	false;
	m_Light_particle_frequency	=	0;
	m_Light_particle_count		=	0;
	m_light_radius				=	0;

	m_FlameSize_min				=	0;
	m_FlameSize_max				=	0;
	m_FlameSize_inc				=	0;

	m_Light_particle_count		=	0;

	m_bOBBReady					=	false;
}



FireB_Effect::~FireB_Effect( void )
{
	UnInitialize();
}


void
FireB_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",			0.8f, 0.5f, 0.0f	);	// PARAM(1)
	params.SetVector3	( "velocity",		0.0f, 8.0f, 0.0f 	);	// PARAM(2)
	params.SetVector3	( "accel",			0.0f, 14.0f, 0.0f	);	// PARAM(3)
	params.SetVector3	( "fdamage",		3.0f, 7.0f, 0.15f	);	// PARAM(4)
	params.SetVector3	( "fctrl",			-1.25, 1.25, 0.035f	);	// PARAM(5)

	params.SetUINT32	( "light_freq",		50		);				// PARAM(6)
																	
	params.SetFloat		( "light_radius",	1.0f	);				// PARAM(7)
	params.SetFloat		( "flamesize",		1.75f	);				// PARAM(8)
	params.SetFloat		( "alphafade",		0.85f	);				// PARAM(9)
	params.SetFloat		( "count",			90		);				// PARAM(10)
	params.SetFloat		( "min_count",		90		);				// PARAM(11)
	params.SetFloat		( "velocity_s",		1.0f	);				// PARAM(12)
	params.SetFloat		( "lower_r0",		0.5f	);				// PARAM(13)
	params.SetFloat		( "lower_r1",		1.25f	);				// PARAM(14)
	params.SetFloat		( "upper_r0",		1.0f	);				// PARAM(15)
	params.SetFloat		( "upper_r1",		3.25f	);				// PARAM(16)
	params.SetFloat		( "min_displace",	0.0f	);				// PARAM(17)
	params.SetFloat		( "max_displace",	0.0f	);				// PARAM(18)
	params.SetFloat		( "start_rate",		27.5f	);				// PARAM(19)
	params.SetFloat		( "end_rate",		16.0f	);				// PARAM(20)
	params.SetFloat		( "obb_freq",		0.1f	);				// PARAM(21)
																	
	params.SetBool		( "cheap",			false	);				// PARAM(22)
	params.SetBool		( "dark",			false	);				// PARAM(23)
	params.SetBool		( "cvel",			false	);				// PARAM(24)
	params.SetBool		( "ignite",			false	);				// PARAM(25)
	params.SetBool		( "damage",			false	);				// PARAM(26)
	params.SetBool		( "light_spawn",	false	);				// PARAM(27)
}


void
FireB_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),			m_Base_color		);
	m_Params.GetVector3	( PARAM(2),			m_Velocity 			);
	m_Params.GetVector3	( PARAM(3),			m_Acceleration		);
	m_Params.GetVector3	( PARAM(4),			m_FDamage			);

	vector_3 temp(DoNotInitialize);
	m_Params.GetVector3	( PARAM(5),			temp				);
	m_FlameSize_min		= temp.x;
	m_FlameSize_max		= temp.y;
	m_FlameSize_inc		= temp.z;
	ValidateOrder( m_FlameSize_min, m_FlameSize_max );

	m_Params.GetUINT32	( PARAM(6),			m_Light_particle_frequency);
	m_Params.GetFloat	( PARAM(7),			m_light_radius	);

	m_Params.GetFloat	( PARAM(8),			m_FlameSize	);
	m_Params.GetFloat	( PARAM(9),			m_AlphaFade			);

	m_Params.GetFloat	( PARAM(10),		m_MaxFlames			);
	if( !m_Params.GetFloat( PARAM(11),		m_MinFlames			) )
	{
		m_MinFlames = m_MaxFlames;
	}

	ValidateOrder( m_MinFlames, m_MaxFlames );
	const float count = m_MinFlames + (gWorldFx.GetDetailLevel() * (m_MaxFlames - m_MinFlames));

	m_MaxFlames	= (count > MAX_FLAMES) ? MAX_FLAMES : count;

	m_Params.GetFloat	( PARAM(12),		m_Velocity_scalar	);
	m_Params.GetFloat	( PARAM(13),		m_Lower_r0			);
	m_Params.GetFloat	( PARAM(14),		m_Lower_r1			);
	m_Params.GetFloat	( PARAM(15),		m_Upper_r0			);
	m_Params.GetFloat	( PARAM(16),		m_Upper_r1			);

	ValidateOrder( m_Lower_r0, m_Lower_r1 );
	ValidateOrder( m_Upper_r0, m_Upper_r1 );

	m_Params.GetFloat	( PARAM(17),		m_DisplaceMin		);
	m_DisplaceMin *= m_Scale;
	m_Params.GetFloat	( PARAM(18),		m_DisplaceMax		);
	m_DisplaceMax *= m_Scale;
	ValidateOrder( m_DisplaceMin, m_DisplaceMax );

	m_Params.GetFloat	( PARAM(19),		m_BurnStartRate		);
	m_Params.GetFloat	( PARAM(20),		m_BurnEndRate		);

	m_Params.GetFloat	( PARAM(21),		m_OBB_freq			);
	m_OBB_time			= m_OBB_freq;

	m_Params.GetBool	( PARAM(22),		m_bCheap			);
	m_Params.GetBool	( PARAM(23),		m_bDarkBlending		);
	m_Params.GetBool	( PARAM(24),		m_bConstantVelocity	);
	m_Params.GetBool	( PARAM(25),		m_bIgnite			);
	m_Params.GetBool	( PARAM(26),		m_bDamage			);
	m_Params.GetBool	( PARAM(27),		m_bLightSpawn		);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_TimeScalar *= 3.5f;		// using default burn rate of 3.5f

	m_MinimumCorner = vector_3( RealMaximum, RealMaximum, RealMaximum );
	m_MaximumCorner = vector_3( -RealMaximum, -RealMaximum, -RealMaximum );
	
	m_FlameCount = 0.0f;

	m_bMustUpdate |= (m_bDamage | m_bIgnite);
}



UINT32
FireB_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		if( m_bDarkBlending )
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 5 );
		}
		else
		{
			m_TextureName = gWorldFx.GetGlobalTexture( 1 );
		}
	}
	return m_TextureName;
}


bool
FireB_Effect::ReInitialize( void )
{
	if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		return false;
	}

	return true;
}


bool
FireB_Effect::UnInitialize( void )
{
	// Make sure that we destroy any lightsources that could have been left ever before this effect was killed off
	if( m_bLightSpawn )
	{
		def_flame_list::iterator iFlameList = m_Flames.begin();
		for(;  iFlameList != m_Flames.end(); ++iFlameList )
		{
			def_flame::iterator iFlame = (*iFlameList).flames.begin();
			for(; iFlame != (*iFlameList).flames.end(); ++iFlame )
			{
				if( (*iFlame).light_particle )
				{
					gWorldFx.DestroyLightSource( (*iFlame).light_id );
				}
			}
		}
	}

	m_Flames.clear();
	m_Insert.clear();
	m_FlameCount = 0;

	return true;
}


FUBI_DECLARE_SELF_TRAITS( FireB_Effect::Flame );
FUBI_DECLARE_SELF_TRAITS( FireB_Effect::Flame_list );


bool
FireB_Effect::Flame::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "position",			position		);
	persist.Xfer	( "velocity",			velocity		);
	persist.Xfer	( "acceleration",		acceleration	);
	persist.Xfer	( "alpha",				alpha			);
	persist.Xfer	( "s",					s				);
	persist.Xfer	( "light_particle",		light_particle	);
	persist.XferHex	( "light_id",			light_id		);

	return true;
}


bool
FireB_Effect::Flame_list::Xfer( FuBi::PersistContext &persist )
{
	persist.XferHex	( "guid",				guid			);
	persist.XferList( "flames",				flames			);

	return true;
}


bool
FireB_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_Base_color",					m_Base_color				);
	persist.Xfer	( "m_Acceleration",					m_Acceleration				);
	persist.Xfer	( "m_Velocity",						m_Velocity					);
	persist.Xfer	( "m_AlphaFade",					m_AlphaFade					);
	persist.Xfer	( "m_FlameSize",					m_FlameSize					);
	persist.Xfer	( "m_MinFlames",					m_MinFlames					);
	persist.Xfer	( "m_MaxFlames",					m_MaxFlames					);
	persist.Xfer	( "m_bCheap",						m_bCheap					);
	persist.Xfer	( "m_bDarkBlending",				m_bDarkBlending				);
	persist.Xfer	( "m_bConstantVelocity",			m_bConstantVelocity			);
	persist.Xfer	( "m_Velocity_scalar",				m_Velocity_scalar			);
														
	persist.Xfer	( "m_Lower_r0",						m_Lower_r0					);
	persist.Xfer	( "m_Lower_r1",						m_Lower_r1					);
	persist.Xfer	( "m_Upper_r0",						m_Upper_r0					);
	persist.Xfer	( "m_Upper_r1",						m_Upper_r1					);
														
	persist.Xfer	( "m_DisplaceMin",					m_DisplaceMin				);
	persist.Xfer	( "m_DisplaceMax",					m_DisplaceMax				);
	persist.Xfer	( "m_BurnStartRate",				m_BurnStartRate				);
	persist.Xfer	( "m_BurnEndRate",					m_BurnEndRate				);

	persist.Xfer	( "m_FDamage",						m_FDamage					);

	persist.Xfer	( "m_FlameSize_min",				m_FlameSize_min				);
	persist.Xfer	( "m_FlameSize_max",				m_FlameSize_max				);
	persist.Xfer	( "m_FlameSize_inc",				m_FlameSize_inc				);
	
	persist.Xfer	( "m_OBB_freq",						m_OBB_freq					);
	persist.Xfer	( "m_OBB_time",						m_OBB_time					);

	persist.Xfer	( "m_OBB_half_diag",				m_OBB_half_diag				);
	persist.Xfer	( "m_OBB_center",					m_OBB_center				);

	persist.Xfer	( "m_LastSpawnTarget",				m_LastSpawnTarget			);

	persist.Xfer	( "m_MinimumCorner",				m_MinimumCorner				);
	persist.Xfer	( "m_MaximumCorner",				m_MaximumCorner				);
														
	persist.XferList( "m_Flames",						m_Flames					);
	persist.XferList( "m_Insert",						m_Insert					);

	persist.Xfer	( "m_FlameCount",					m_FlameCount				);
	persist.Xfer	( "m_bIgnite",						m_bIgnite					);
	persist.Xfer	( "m_bDamage",						m_bDamage					);

	persist.Xfer	( "m_bLightSpawn",					m_bLightSpawn				);
	persist.Xfer	( "m_Light_particle_frequency",		m_Light_particle_frequency	);
	persist.Xfer	( "m_Light_particle_count",			m_Light_particle_count		);
	persist.Xfer	( "m_light_radius",					m_light_radius				);
	persist.Xfer	( "m_bOBBReady",					m_bOBBReady					);

	return true;
}


void
FireB_Effect::UpdateDerived( void )
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

		m_LastSpawnTarget = m_Pos;
	}

	// If it is time to finish then start decreasing the number of 
	// flame particles that we actually are using
	if( m_bFinishing )
	{
		// Determine the number of particles to use
		INT32	flames_to_process = (INT32)m_FlameCount;
		m_FlameCount -= m_BurnEndRate * m_FrameSync;

		// Clamp and die
		if( m_FlameCount < 0.0f )
		{
			m_FlameCount = 0.0f;
			EndThisEffect();
			return;
		}
		flames_to_process = (INT32)m_FlameCount - flames_to_process;

		// Iterate through our master list of lists of flames
		// and get rid of any particles that are not really visible
		def_flame_list::iterator kill = m_Flames.begin();
		for(; kill != m_Flames.end(); ++kill )
		{
			// Now look at the flames themselves to find out who should die
			def_flame::iterator iFlame = (*kill).flames.begin(), iflame_end = (*kill).flames.end();
			while( iFlame != iflame_end )
			{
				if( (*iFlame).alpha <= 0.15f )
				{
					// If we are erasing a flame particle that had a light attached
					// to it then we need to destroy it now
					if( m_bLightSpawn && (*iFlame).light_particle )
					{
						gWorldFx.DestroyLightSource( (*iFlame).light_id );
					}

					iFlame = (*kill).flames.erase( iFlame );
					iflame_end = (*kill).flames.end();

					--flames_to_process;
					if( flames_to_process == 0 )
					{
						goto main_update;	// goto is good for nested terminations
					}
				}
				else
				{
					++iFlame;
				}
			}
		}
	}
	else
	{
		// We are either just starting out or we haven't yet spawned the specified
		// total particle count so deterimine how many we should spawn for this sim
		INT32	flames_to_process = (INT32)m_FlameCount;
		m_FlameCount += m_BurnStartRate * m_FrameSync;

		// Clamp and continue
		if( m_FlameCount >= m_MaxFlames )
		{
			m_FlameCount = m_MaxFlames;
		}

		flames_to_process = (INT32)m_FlameCount - flames_to_process;

		// Based on how many flames we need to create - call CreateNewFlame and 
		// copy our instance into the flame list
		for( int flame_count = flames_to_process; flame_count > 0; --flame_count )
		{
			// After the creation of each flame it will be sorted by node guid and
			// inserted into an existing list or create one if one doesn't exist

			Flame flame;
			if( CreateNewFlame( flame ) )
			{
				flame.alpha -= m_AlphaFade*m_FrameSync;
				flame.alpha = FilterClamp( 0.0f, 1.0f, flame.alpha );

				// Look through the existing flame lists and see if there is a list
				// that exists in the node that this flame particle needs to be in
				bool inserted = false;
				def_flame_list::iterator existing = m_Flames.begin();
				for(; existing != m_Flames.end(); ++existing )
				{
					// Insert into this list if it matches our guid
					if( (*existing).guid == flame.position.node )
					{
						(*existing).flames.push_back( flame );
						inserted = true;
						break;
					}
				}

				// If the flame particle wasn't inserted into a list then we need to 
				// create a new list to put this particle in
				if( !inserted )
				{
					// Create the new list with the appropriate node
					Flame_list	new_list;
					new_list.guid = flame.position.node;
					new_list.flames.push_back( flame );
					m_Flames.push_back( new_list );
				}
			}
		}
	}

main_update:

	m_MinimumCorner = vector_3(  FLOAT_INFINITE,  FLOAT_INFINITE,  FLOAT_INFINITE ); 
	m_MaximumCorner = vector_3( -FLOAT_INFINITE, -FLOAT_INFINITE, -FLOAT_INFINITE ); 

	
	// During our update we had to create some new flame lists so insert them into
	// the master list of flame lists here now that we are able to disturb it
	def_flame_list::iterator iList = m_Insert.begin();
	for(; iList != m_Insert.end(); ++iList )
	{
		// There is the possibilty that the list could have already been created 
		// so look to see if it's guid is unique
		bool list_exists = false;
		def_flame_list::iterator iExisting = m_Flames.begin();
		for(; iExisting != m_Flames.end();++iExisting )
		{
			// If we found a match tnen push all the flames on this
			// list onto the existing list and discard the old
			if( (*iExisting).guid == (*iList).guid )
			{
				def_flame::iterator iNewFlame = (*iList).flames.begin(); 
				for(; iNewFlame != (*iList).flames.end(); ++iNewFlame )
				{
					(*iExisting).flames.push_back( *iNewFlame );
				}
				list_exists = true;
			}
		}

		// If we didn't find a list then push it on the master list with it's single flame
		if( !list_exists )
		{
			m_Flames.push_back( *iList );
		}
	}
	m_Insert.clear();

	m_OBB_time += m_FrameSync;

	if( !m_bOBBReady || (m_OBB_time >= m_OBB_freq) )
	{
		m_OBB_time = 0;
		m_bOBBReady |= ComputeOBB( m_Orient, m_OBB_center, m_OBB_half_diag );
	}

	// Damage anything within the current bounding volume
	if( (m_bDamage || m_bIgnite) && gWorldFx.IsServerLocal() && m_bOBBReady )
	{
		gWorldFx.DamageWithinBox( m_OBB_center, m_Orient, m_OBB_half_diag, m_FDamage.x, m_FDamage.y,
									m_bIgnite, (m_FrameSync / m_TimeScalar), m_bIgnite, GetFriendly(), m_OwnerID );
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif


	// Iterate through the list of flame lists and process each individual flame
	def_flame_list::iterator uList = m_Flames.begin();
	for(; uList != m_Flames.end(); ++uList )
	{
		// Process a flame list
		def_flame::iterator uFlame = (*uList).flames.begin(), uFlame_end = (*uList).flames.end();
		while( uFlame != uFlame_end )
		{
			// Update particle visibility and life
			(*uFlame).alpha -= m_AlphaFade*m_FrameSync;
			(*uFlame).alpha = FilterClamp( 0.0f, 1.0f, (*uFlame).alpha );

			// If the alpha is below zero then we need to get rid of this flame particle
			// and spawn a replacement for it
			if( (*uFlame).alpha <= 0 )
			{
				// If we are erasing a flame particle that had a light attached
				// to it then we need to destroy it now before we overwrite the
				// old flame
				if( m_bLightSpawn && (*uFlame).light_particle )
				{
					gWorldFx.DestroyLightSource( (*uFlame).light_id );
				}

				Flame flame;
				if( CreateNewFlame( flame ) )
				{
					// If this new flame matches the guid of the list of flame lists that
					// we are currently in then just replace the current flame with our 
					// new one and continue on...
					if( flame.position.node == (*uList).guid )
					{
						(*uFlame).position			= flame.position;
						(*uFlame).velocity			= flame.velocity;
						(*uFlame).light_particle	= flame.light_particle;
						(*uFlame).light_id			= flame.light_id;
						(*uFlame).acceleration		= flame.acceleration;
						(*uFlame).alpha				= flame.alpha;
						(*uFlame).s					= flame.s;
					}
					else
					{
						// The flame doesn't belong in this current list of flames
						// so we need to remove it and create a new flame in a new
						// list to match the new guid

						// First check to see if there is an existing flame list that
						// has the same guid before we just create a new list
						bool inserted = false;
						def_flame_list::iterator iInsert = m_Insert.begin();
						for(; iInsert != m_Insert.end(); ++iInsert )
						{
							// Insert into this list if it matches our guid
							if( (*iInsert).guid == flame.position.node )
							{
								(*iInsert).flames.push_back( flame );
								inserted = true;
								break;
							}
						}
						// If the flame particle wasn't inserted into a list then we need to 
						// create a new list to put this particle in
						if( !inserted )
						{
							Flame_list	new_list;
							new_list.guid = flame.position.node;
							new_list.flames.push_back( flame );
							m_Insert.push_back( new_list );
						}

						// Erase it from the old list
						uFlame = (*uList).flames.erase( uFlame );
						uFlame_end = (*uList).flames.end();
						continue;
					}
				}
				++uFlame;
				continue;
			}

			// Update particle size
			(*uFlame).s += (1.0f - (*uFlame).alpha) * m_FlameSize_inc * m_FrameSync;

			// Update particle motion
			siege::database_guid old_node( (*uFlame).position.node );

			const vector_3 displacement( ( 0.5f * m_FrameSync * m_FrameSync * (*uFlame).acceleration ) + (*uFlame).velocity * m_FrameSync );

			(*uFlame).velocity += (*uFlame).acceleration * m_FrameSync;

			bool bNodeChange = false;

			if( !gWorldFx.AddPositions( (*uFlame).position, displacement, m_bFast, &bNodeChange ) )
			{
				continue;
			}

			if( bNodeChange )
			{
				matrix_3x3 diff_orient;

				gWorldFx.GetDifferenceOrient( old_node, (*uFlame).position.node, diff_orient );

				(*uFlame).velocity	   = diff_orient * (*uFlame).velocity;
				(*uFlame).acceleration = diff_orient * (*uFlame).acceleration;

				// Flame particle moved into a new node so it must be transferred to a new list
				bool inserted = false;
				def_flame_list::iterator iInsert = m_Insert.begin();
				for(; iInsert != m_Insert.end(); ++iInsert )
				{
					// Insert into this list if it matches our guid
					if( (*uFlame).position.node == (*iInsert).guid )
					{
						(*iInsert).flames.push_back( *uFlame );
						inserted = true;
						break;
					}
				}

				// If the flame particle wasn't inserted into a list then we need to 
				// create a new list to put this particle in
				if( !inserted )
				{
					Flame_list new_list;
					new_list.guid = (*uFlame).position.node;
					new_list.flames.push_back( *uFlame );
					m_Insert.push_back( new_list );
				}

				// Erase it from the old list
				uFlame = (*uList).flames.erase( uFlame );
				uFlame_end = (*uList).flames.end();
				continue;
			}

			// If this flame has a light source associated with it we need to move it
			if( m_bLightSpawn && (*uFlame).light_particle )
			{
				if( !gWorldFx.PositionLightSource( (*uFlame).light_id, (*uFlame).position, MAKEDWORDCOLOR(m_Base_color) ) )
				{
					gWorldFx.CreateLightSource( (*uFlame).light_id, (*uFlame).position, m_Base_color, 0, m_light_radius, false );
				}
			}
			

			// Expand the bounding volume for this effect
			AddFlameToBounds( *uFlame );
			++uFlame;
		}
	}

	//finished update, update Last spawned position.
	m_LastSpawnTarget = m_Pos;
}


void
FireB_Effect::Track( void )
{
	SiegePos targetPos;

	if( m_Targets->GetPlacement( &targetPos, &m_Orient, m_OrientMode ) )
	{
		if( !m_Offset.IsZero() && !gWorldFx.AddPositions( targetPos, (m_Orient * m_Offset), m_bFast, NULL ) )
		{
			EndThisEffect();
			return;
		}

		m_bHasMoved = ( m_bDamage && ( m_Pos != targetPos ));
		m_Pos = targetPos;
	}
	else
	{
		EndThisEffect();
	}
}


bool
FireB_Effect::CreateNewFlame( Flame &flame )
{
	// Deterimine a random size/scale for the particle
	flame.s = gWorldFx.GetRNG().Random( m_FlameSize_min, m_FlameSize_max );

	// ipos is the position on the inner radius
	// epos is the position on the outer radius
	vector_3	ipos, epos;

	// Pick a random radius for the path we are following on
	// the cone
	float r0 = m_Scale * gWorldFx.GetRNG().Random( m_Lower_r0, m_Lower_r1 );
	float r1 = m_Scale * gWorldFx.GetRNG().Random( m_Upper_r0, m_Upper_r1 );

	// pick a random point on the radius of the cone to start
	// both of the random radii by
	float thetasin, thetacos;
	SINCOSF( gWorldFx.GetRNG().Random( PI2 ), thetasin, thetacos );

	// Position on the XY plane for the inner radius 
	// starting at the base of the cone
	ipos.x = r0 * thetacos;
	ipos.y = 0;
	ipos.z = r0 * thetasin;

	// Position along the XY plane for the outer radius with
	// some random Z displacement to give the conical volume
	epos.x = r1 * thetacos;
	epos.y = gWorldFx.GetRNG().Random( m_DisplaceMin, m_DisplaceMax );
	epos.z = r1 * thetasin;

	// Orient to the space of the emission point
	ipos = m_Orient * ipos;
	epos = m_Orient * epos;

	matrix_3x3 new_orient( m_Orient );

	flame.position.pos  = m_Pos.pos + ipos;

	// adjust for a moving target.
	float spawnTime = gWorldFx.GetRNG().RandomFloat();

	if( gSiegeEngine.IsNodeValid( m_LastSpawnTarget.node ) )
	{
		vector_3 startoffset( DoNotInitialize );
		
		startoffset = gSiegeEngine.GetDifferenceVector( m_Pos, m_LastSpawnTarget );
		
		flame.position.pos += spawnTime * startoffset;
	}

	flame.position.node = m_Pos.node;

	// Calculate the difference and use that as the slope to
	// trave along the conical section from the emission point
	const vector_3 perturb( new_orient * (epos - ipos) );

	// Correct our acceleration and velocity vectors to the 
	// emission points space

	vector_3 acceleration	( perturb + (new_orient * m_Acceleration) );
	vector_3 velocity		( perturb + (new_orient * m_Velocity) );

	// Check to see if we should apply the currently available wind
	// vector to our velocity
	if( m_bWindAffected )
	{
		vector_3 wind_vector( DoNotInitialize );
		gWorldFx.GetNodeWindVector( flame.position.node, wind_vector );

		velocity += 5.0f * wind_vector;
	}

	// Why scale acceleration and velocity? Because this allows me to use
	// the exact same number of particles for any size of flame and in order
	// for it to work correctly the acceleration and speed must scale as well
	flame.acceleration = acceleration * m_Scale;

	// If parameters specify not to throw in a bit of randomness to the initial
	// velocity then use a constant starting velocity
	if( m_bConstantVelocity )
	{
		flame.velocity = velocity * m_Velocity_scalar * m_Scale;
	}
	else
	{
		flame.velocity = velocity * gWorldFx.GetRNG().Random( m_Velocity_scalar ) * m_Scale;
	}

	// ost = Offset Start Time
	const float ost = m_FrameSync * spawnTime; 

	// Alpha is the life of the particle - when it is zero the particle is 
	// dead and is replaced by a new one until the effect is done
	flame.alpha = max( 0.0f, (1.0f - (m_AlphaFade * ost) ) );

	siege::database_guid old_node( flame.position.node );

	const vector_3 displacement( ( 0.5f * ost * ost * flame.acceleration ) + flame.velocity * ( ost ) );

	bool bNodeChange = false;

	if( !gWorldFx.AddPositions( flame.position, displacement, m_bFast, &bNodeChange ) )
	{
		return false;
	}

	if( bNodeChange )
	{
		matrix_3x3 diff_orient;
		gWorldFx.GetDifferenceOrient( old_node, flame.position.node, diff_orient );

		flame.velocity	   = diff_orient * flame.velocity;
		flame.acceleration = diff_orient * flame.acceleration;
	}

	flame.velocity += flame.acceleration * ( ost );

	// If we are spawning lightsources every so often with a then see if
	// this particle is the one that should have it attached to
	if( m_bLightSpawn )
	{
		++m_Light_particle_count;
		
		flame.light_particle = false;

		if( m_Light_particle_count >= m_Light_particle_frequency )
		{
			m_Light_particle_count = 0;

			flame.light_particle = true;
			gWorldFx.CreateLightSource( flame.light_id, flame.position, m_Base_color, 0, m_light_radius, false );
		}
	}

	return true;
}



void
FireB_Effect::DrawSelf( void )
{
	Rapi&	renderer = gWorldFx.GetRenderer();

	// If specififed as a parameter - use an alternate blending mode to get
	// a darker look instead of making things get increasingly brighter
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

	// Save off the last node guid that we used so we can reference it later
	// in order to minimize the number of times that we setup our transformations
	siege::database_guid current_guid = siege::UNDEFINED_GUID;

	// Go through the list of flame lists, setup for the node all of those flame
	// particles are in and draw the list of flames
	def_flame_list::iterator iDraw = m_Flames.begin();
	for(;  iDraw != m_Flames.end(); ++iDraw )
	{
		// Check to see if we have already setup to draw to this node by comparing
		// the old draw guid with the specified guid that we want to draw to
		if( !(current_guid == (*iDraw).guid ))
		{
			// Make sure this is in a valid node otherwise abort this effect
			if( !gWorldFx.IsNodeVisible( (*iDraw).guid ) )
			{
				continue;
			}

			SetDrawMatrix( (*iDraw).guid );
			current_guid = (*iDraw).guid;
		}

		// This variable will hold the number of flames that we actually drew
		int flame_counter = 0;

		vector_3 vpp0, vpp1, vpp2, vpp3;

		// If cheap is specified then will minimize our multiplication and use a
		// less cool looking version of the firebreathing by doing as many calcs
		// as we can up front by orienting every flame the same
		if( m_bCheap )
		{
			vpp0 =  m_Scale * ( m_FlameSize * m_Vy - m_FlameSize * m_Vx);
			vpp1 =  m_Scale * (-m_FlameSize * m_Vx - m_FlameSize * m_Vy);
			vpp2 =  m_Scale * ( m_FlameSize * m_Vx + m_FlameSize * m_Vy);
			vpp3 =  m_Scale * ( m_FlameSize * m_Vx - m_FlameSize * m_Vy);
		}

		// Initialize some work variables
		bool	have_flames_to_draw = false;
		int		num_flames = 0;
		def_flame::iterator iFlame = (*iDraw).flames.begin();

		m_DrawVerts.resize( (*iDraw).flames.size() * 4 );

		// Loop through the lists of flames to draw
		for(; iFlame != (*iDraw).flames.end(); ++iFlame )
		{
			float const alpha = (*iFlame).alpha;

			// Skip drawing flames that we cannot see
			if( alpha <= 0 )
			{ 
				continue;
			}

			have_flames_to_draw = true;
			num_flames++;

			// Dereference some frequently used variables
			const vector_3  &position = (*iFlame).position.pos;
			const float  s = (*iFlame).s;

			// Setup some vectors before orienting
			vector_3 vp0( position );
			vector_3 vp1( position );
			vector_3 vp2( position );
			vector_3 vp3( position );

			// If we want speed then just use the cheaper calculations
			if( m_bCheap )
			{
				vp0 += vpp0;
				vp1 += vpp1;
				vp2 += vpp2;
				vp3 += vpp3;
			}
			else
			{
				// Use better more random calcs by orienting each flame 
				// individually
				float	wh = m_FlameSize + alpha + s;

				vp0 += m_Scale * ( wh*m_Vy -wh*m_Vx );
				vp1 += m_Scale * (-wh*m_Vx -wh*m_Vy );
				vp2 += m_Scale * ( wh*m_Vx +wh*m_Vy );
				vp3 += m_Scale * ( wh*m_Vx -wh*m_Vy );
			}

			// Convert our alpha modified color to a DWORD color for Rapi
			DWORD color	= MAKEDWORDCOLOR( m_Base_color );
			color		= ((color&0x00FFFFFF)|((BYTE(alpha*m_NodeAlpha*255.0f))<<24));

			// Setup the Rapi native vertex format
			int j = flame_counter * 4;
			++flame_counter;

			m_DrawVerts[j+0].x		= vp0.x;	m_DrawVerts[j+1].x		= vp1.x;
			m_DrawVerts[j+0].y		= vp0.y;	m_DrawVerts[j+1].y		= vp1.y;
			m_DrawVerts[j+0].z		= vp0.z;	m_DrawVerts[j+1].z		= vp1.z;
			m_DrawVerts[j+0].color	= color;	m_DrawVerts[j+1].color	= color;
			m_DrawVerts[j+0].uv.u	= 0.0f;		m_DrawVerts[j+1].uv.u	= 0.0f;
			m_DrawVerts[j+0].uv.v	= 1.0f;		m_DrawVerts[j+1].uv.v	= 0.0f;

			m_DrawVerts[j+2].x		= vp2.x;	m_DrawVerts[j+3].x		= vp3.x;
			m_DrawVerts[j+2].y		= vp2.y;	m_DrawVerts[j+3].y		= vp3.y;
			m_DrawVerts[j+2].z		= vp2.z;	m_DrawVerts[j+3].z		= vp3.z;
			m_DrawVerts[j+2].color	= color;	m_DrawVerts[j+3].color	= color;
			m_DrawVerts[j+2].uv.u	= 1.0f;		m_DrawVerts[j+3].uv.u	= 1.0f;
			m_DrawVerts[j+2].uv.v	= 1.0f;		m_DrawVerts[j+3].uv.v	= 0.0f;
		}
		if( have_flames_to_draw )
		{
			renderer.DrawIndexedPrimitive( D3DPT_TRIANGLELIST, &*m_DrawVerts.begin(), 
										num_flames * 4,	SVERTEX, gWorldFx.GetGlobalDrawIndexArray(), 
										num_flames * 6, &m_TextureName, 1 );
		}
	}

	if( m_bDarkBlending )
	{
		gWorldFx.SaveRenderingState();
	}
}


bool
FireB_Effect::ComputeOBB( const matrix_3x3 &EigenVec, SiegePos &center, vector_3 &half_diag )
{
	vector_3 max( -FLOAT_INFINITE, -FLOAT_INFINITE, -FLOAT_INFINITE ); 
	vector_3 min(  FLOAT_INFINITE,  FLOAT_INFINITE,  FLOAT_INFINITE ); 

	bool bValidVolume = false;

	def_flame_list::iterator iDraw = m_Flames.begin(), i_draw_end = m_Flames.end();
	for(; iDraw != i_draw_end; ++iDraw )
	{
		if( !gWorldFx.IsNodeVisible( (*iDraw).guid ) )
		{
			continue;
		}

		def_flame::iterator iFlame = (*iDraw).flames.begin(), i_flames_end = (*iDraw).flames.end();
		for(; iFlame != i_flames_end; ++iFlame )
		{
			if( (*iFlame).alpha <= m_FDamage.z )
			{
				continue;
			}

			const vector_3 &point = (*iFlame).position.WorldPos();

			float range = 0;
			range = point.DotProduct( EigenVec.GetColumn0_T() );

			if( range > max.x )
			{
				max.x = range;
			}
			if( range < min.x )
			{
				min.x = range;
			}

			range = point.DotProduct( EigenVec.GetColumn1_T() );

			if( range > max.y )
			{
				max.y = range;
			}
			if( range < min.y )
			{
				min.y = range;
			}

			range = point.DotProduct( EigenVec.GetColumn2_T() );

			if( range > max.z )
			{
				max.z = range;
			}
			if( range < min.z )
			{
				min.z = range;
			}

			bValidVolume = true;
		}
	}

	if( bValidVolume )
	{
		SiegePos new_center;
		new_center.FromWorldPos( EigenVec * ( (max+min) * 0.5f), m_Pos.node );

		center	  = new_center;
		half_diag = (max-min) * 0.5f;

		return true;
	}
	return false;
}


bool
FireB_Effect::AddFlameToBounds( Flame const &flame )
{
	if( flame.alpha >= m_FDamage.z && gWorldFx.IsInWorldFrustum( flame.position.node ) )
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
FireB_Effect::GetBoundingRadius( void )
{
	return (0.5f * Length(m_MaximumCorner - m_MinimumCorner) );
}


bool
FireB_Effect::GetCullPosition( SiegePos & CullPosition )
{
	CullPosition = m_Pos;

	return gWorldFx.AddPositions( CullPosition, (0.5f * (m_MaximumCorner + m_MinimumCorner)), false, NULL );
}



// Curve effect ---

Curve_Effect::Curve_Effect( void )
{
	m_color_0			= 0;
	m_Size				= 0;
	m_Step				= 0;
	m_Curvature			= 0;
	m_Model				= 0;
	m_TrailLength		= 0;
	m_Velocity			= 0;
	m_CurvePos			= 0;
	m_Spacing			= 0;
	
	m_bPointsChosen		= false;
}


void
Curve_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.235f, 0.24f, 1.0f ); // PARAM(1)

	params.SetUINT32	( "model",		0		); // PARAM(2)

	params.SetFloat		( "size",		0.125f	); // PARAM(3)
	params.SetFloat		( "step",		0.25f	); // PARAM(4)
	params.SetFloat		( "curvature",	1.2f	); // PARAM(5)
	params.SetFloat		( "tlength",	2.0f	); // PARAM(6)
	params.SetFloat		( "velocity",	30.0f	); // PARAM(7)
	params.SetFloat		( "spacing",	3.0f	); // PARAM(8)
}


void
Curve_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),	m_Base_color );
	m_color_0		= MAKEDWORDCOLOR( m_Base_color );
	m_Params.GetUINT32	( PARAM(2),	m_Model		);

	m_Params.GetFloat	( PARAM(3),	m_Size	);
	m_Size *= m_ScaleFactor;
	m_Params.GetFloat	( PARAM(4),	m_Step	);
	m_Params.GetFloat	( PARAM(5),	m_Curvature	);
	m_Params.GetFloat	( PARAM(6),	m_TrailLength	);
	m_Params.GetFloat	( PARAM(7),	m_Velocity	);
	m_Params.GetFloat	( PARAM(8),	m_Spacing	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_bPointsChosen	= false;
	m_CurvePos		= 0;
}


UINT32
Curve_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	return m_TextureName;
}


bool
Curve_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_Base_color",		m_Base_color	);
	persist.Xfer		( "m_color_0",			m_color_0		);
	persist.Xfer		( "m_Size",				m_Size			);
	persist.Xfer		( "m_Step",				m_Step			);
	persist.Xfer		( "m_Curvature",		m_Curvature		);
	persist.Xfer		( "m_Model",			m_Model			);
	persist.Xfer		( "m_TrailLength",		m_TrailLength	);
	persist.Xfer		( "m_Velocity",			m_Velocity		);
	persist.Xfer		( "m_CurvePos",			m_CurvePos		);
	persist.Xfer		( "m_Spacing",			m_Spacing		);
						
	persist.Xfer		( "m_bPointsChosen",	m_bPointsChosen );
												
	persist.Xfer		( "m_RefPos",			m_RefPos		);
	persist.Xfer		( "m_P0",				m_P0			);
	persist.Xfer		( "m_P1",				m_P1			);
	persist.Xfer		( "m_P2",				m_P2			);
	persist.Xfer		( "m_P3",				m_P3			);
		
	persist.XferVector	( "m_Points",			m_Points		);

	return true;
}


void
Curve_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		// m_Pos will serve as the reference point from which all other points will be offset
		// from so that there is only one node position that has to be maintained
		m_Targets->GetPlacement( &m_RefPos, NULL, m_OrientMode );
		m_Pos = m_RefPos;
	}

	SiegePos source_position, target_position;

	if( m_Targets->GetPlacement( &target_position, NULL, m_OrientMode ) &&
		m_Targets->GetPlacement( &source_position, NULL, m_OrientMode, TattooTracker::SOURCE  ) )
	{
		m_bHasMoved = ( (target_position != m_LastStartPos) || (source_position != m_EndPos) );

		// Make sure this is in a valid node otherwise abort this effect
		if( !gWorldFx.IsNodeVisible( m_RefPos.node ) )
		{
			EndThisEffect();
			return;
		}
		
		// Setup the start and end points
		m_P0 = gSiegeEngine.GetDifferenceVector( m_RefPos, target_position );
		m_P3 = gSiegeEngine.GetDifferenceVector( m_RefPos, source_position );

		// Determinet a rough bounding radius for this effect
		m_BoundingRadius = Length( m_P0 - m_P3 );

		// If this effect was just created then the middle two control points need
		// to be chosen 
		if( !m_bPointsChosen )
		{
			m_bPointsChosen = true;

			switch( m_Model )
			{
				case 0:
				{	// Generate a more subtle curve
					vector_3 segment = gSiegeEngine.GetDifferenceVector( target_position, source_position );

					// Select spaced out starting points for the middle two control points
					m_P1 = 0.25f * segment;
					m_P2 = 0.75f * segment;

					float perturb_range = m_Curvature * Length( m_P1 );
					const float min_y = 0.75f * (m_P0.y>m_P3.y)?m_P3.y:m_P0.y;

					// now offset from quarter position
					m_P1.x += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
					m_P1.y += gWorldFx.GetRNG().Random( min_y, perturb_range );
					m_P1.z += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );

					m_P2.x += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
					m_P2.y += gWorldFx.GetRNG().Random( min_y, perturb_range );
					m_P2.z += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
				} break;

				case 1:
				{	// Generate crazier curve
					vector_3 segment = gSiegeEngine.GetDifferenceVector( target_position, source_position );

					// Select spaced out starting points for the middle two control points
					m_P2 = 0.25f * segment + m_P0;
					m_P1 = 0.75f * segment + m_P0;

					float perturb_range = m_Curvature * Length( m_P1 );
					const float min_y = 0.75f * (m_P0.y>m_P3.y)?m_P3.y:m_P0.y;

					// now offset from quarter position
					m_P1.x += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
					m_P1.y += gWorldFx.GetRNG().Random( min_y, perturb_range );
					m_P1.z += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );

					m_P2.x += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
					m_P2.y += gWorldFx.GetRNG().Random( min_y, perturb_range );
					m_P2.z += gWorldFx.GetRNG().Random( -perturb_range, perturb_range );
				} break;

				case 2:
				{	// Curve for when the origin is also the target

					// Select spaced out starting points for the middle two control points
					m_P1 = m_P0;
					m_P2 = m_P0;
					m_P3 = m_P0;

					const float min_y = 0.75f * m_P0.y;

					// now offset from quarter position
					m_P1.x += gWorldFx.GetRNG().Random( -m_Curvature, m_Curvature );
					m_P1.y += gWorldFx.GetRNG().Random( min_y, m_Curvature );
					m_P1.z += gWorldFx.GetRNG().Random( -m_Curvature, m_Curvature );

					m_P2.x += gWorldFx.GetRNG().Random( -m_Curvature, m_Curvature );
					m_P2.y += gWorldFx.GetRNG().Random( min_y, m_Curvature );
					m_P2.z += gWorldFx.GetRNG().Random( -m_Curvature, m_Curvature );
				} break;
			}
		}

		// Get straight line distance as an approximation to base curve steps and velocity by in order
		// to make curve section appear to animate at the same speed regardless of how large of a distance
		// needs to be traveled.
		float dist_v = Length( gSiegeEngine.GetDifferenceVector( target_position, source_position ) );

		if( dist_v == 0 )
		{
			dist_v = 0.01f;
		}

		m_Points.clear();

		const float k = (m_Spacing * m_Size) / dist_v;

		// Proportionalize distance sensitive curve generation parameters
		const float step		= m_Step * k;
		const float velocity	= m_Velocity * k;
		const float trail_len	= m_TrailLength * k;

		// Determine where in the curve where are 
		m_CurvePos += velocity * m_FrameSync;
		float curve_end = m_CurvePos - trail_len;
		curve_end = FilterClamp( 0.0f, 1.0f, curve_end );
		float curve_pos = m_CurvePos;
		curve_pos = FilterClamp( 0.0f, 1.0f,  curve_pos );

		// If the end of the curve ever equals the current position then the trail has caught
		// up with the head and this effect is over with
		if( (curve_end == curve_pos) && (m_FrameSync > 0) )
		{
			EndThisEffect();
			return;
		}

		// Compute the curve using bezier generation
		for( float u = curve_end; u < curve_pos; u += step )
		{
			vector_3 point =	m_RefPos.pos + 
								m_P0 * ((1-u)*(1-u)*(1-u)) + 
								m_P1 * 3*u*((1-u)*(1-u)) + 
								m_P2 * 3*(u*u)*(1-u) + 
								m_P3 * (u*u*u);

			m_Points.push_back( point );
		}

		// $$$ The position is generated by using an initial reference point and then each point
		// is recorded as an offset from that original position. So every point on the curve
		// actually never leaves the original node that it starts in it just gets drawn beyond
		// the source node boundry. This makes the lightsource effect not work.
	}
	else
	{ 
		EndThisEffect();
		return;
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
Curve_Effect::DrawSelf( void )
{
	// Get relevant camera info
	const float		s = m_Size;
	const vector_3	wVx = s * m_Vx;
	const vector_3	hVy = s * m_Vy;

	unsigned int i = 0;
	float n = (float)m_Points.size();

	stdx::fast_vector< vector_3 >::iterator iPoint = m_Points.begin(), iEnd = m_Points.end();
	stdx::fast_vector< vector_3 >::iterator iBegin = iPoint;

	// Iterate through the vector of points and draw the curve
	for( ; iPoint != iEnd; ++iPoint, ++i )
	{
		DWORD color = ((m_color_0&0x00FFFFFF)|((BYTE(m_NodeAlpha*255.0f*((float)i/n)))<<24));

		const vector_3 vp0 = - wVx + hVy + *iPoint;
		const vector_3 vp1 = - wVx - hVy + *iPoint;
		const vector_3 vp2 =   wVx + hVy + *iPoint;
		const vector_3 vp3 =   wVx - hVy + *iPoint;

		m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp1.x;
		m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp1.y;
		m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp1.z;
		m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
		m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 0.0f;
		m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 0.0f;

		m_DrawVerts[2].x		= vp2.x;	m_DrawVerts[3].x		= vp3.x;
		m_DrawVerts[2].y		= vp2.y;	m_DrawVerts[3].y		= vp3.y;
		m_DrawVerts[2].z		= vp2.z;	m_DrawVerts[3].z		= vp3.z;
		m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
		m_DrawVerts[2].uv.u		= 1.0f;		m_DrawVerts[3].uv.u		= 1.0f;
		m_DrawVerts[2].uv.v		= 1.0f;		m_DrawVerts[3].uv.v		= 0.0f;

		gWorldFx.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX, &m_TextureName, 1 );
	}

	// Set position for external attachment so that other effects can attach to the head of
	// this curve
	if( iPoint != iBegin )
	{
		--iPoint;
		m_Pos.pos = *iPoint;
	}
}




Spawn_Effect::Spawn_Effect( void )
{
	m_SpawnInterval			= 0;
	m_SpawnCount			= 0;
	m_MaxObjects			= 0;
	
	m_RadiusStartMin		= 0;
	m_RadiusStartMax		= 0;
	m_RadiusEndMin			= 0;
	m_RadiusEndMax			= 0;

	m_SpawnElapsed			= 0;
	m_TotalSpawned			= 0;
}


void
Spawn_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetGPString	( "model",		"frag_glb_bone_06"	); // PARAM(1)

	params.SetVector3	( "lvel",		0.0f, 8.0f, 0.0f	); // PARAM(2)
	params.SetVector3	( "lvel_var",	0.0f, 2.0f, 0.0f	); // PARAM(3)
	params.SetVector3	( "lacc",		0.0f, 0.0f, 0.0f	); // PARAM(4)
	params.SetVector3	( "lacc_var",	0.0f, 0.0f, 0.0f	); // PARAM(5)
	params.SetVector3	( "avel",		0.0f, 0.0f, 4.0f	); // PARAM(6)
	params.SetVector3	( "avel_var",	0.0f, 0.0f, 5.0f	); // PARAM(7)
	params.SetVector3	( "aacc",		0.0f, 0.0f, 0.0f	); // PARAM(8)
	params.SetVector3	( "aacc_var",	0.0f, 0.0f, 0.0f	); // PARAM(9)

	params.SetFloat		( "spawn_int",	0.5f	); // PARAM(10)
	params.SetFloat		( "spawn_count",1.0f	); // PARAM(11)
	params.SetFloat		( "count",		16.0f	); // PARAM(12)
	params.SetFloat		( "sr_min",		0.5f	); // PARAM(13)
	params.SetFloat		( "sr_max",		1.0f	); // PARAM(14)
	params.SetFloat		( "er_min",		1.0f	); // PARAM(15)
	params.SetFloat		( "er_max",		1.5f	); // PARAM(16)
}	


void
Spawn_Effect::InitializeDerived( void )
{
	m_Params.GetGPString( PARAM(1),		m_sModelName			);

	m_Params.GetVector3	( PARAM(2),		m_LinearVelocity		);
	m_Params.GetVector3	( PARAM(3),		m_LinearVelocityVar		);
	m_Params.GetVector3	( PARAM(4),		m_LinearAcceleration	);
	m_Params.GetVector3	( PARAM(5),		m_LinearAccelerationVar );
	m_Params.GetVector3	( PARAM(6),		m_AngularVelocity		);
	m_Params.GetVector3	( PARAM(7),		m_AngularVelocityVar	);
	m_Params.GetVector3	( PARAM(8),		m_AngularAcceleration	);
	m_Params.GetVector3	( PARAM(9),		m_AngularAccelerationVar);

	m_Params.GetFloat	( PARAM(10),	m_SpawnInterval			);
	m_Params.GetFloat	( PARAM(11),	m_SpawnCount			);
	m_Params.GetFloat	( PARAM(12),	m_MaxObjects			);
	m_Params.GetFloat	( PARAM(13),	m_RadiusStartMin		);
	m_Params.GetFloat	( PARAM(14),	m_RadiusStartMax		);
	m_Params.GetFloat	( PARAM(15),	m_RadiusEndMin			);
	m_Params.GetFloat	( PARAM(16),	m_RadiusEndMax			);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	// Fix-up and min/max problems
	ValidateOrder( m_RadiusStartMin,	m_RadiusStartMax);
	ValidateOrder( m_RadiusEndMin,		m_RadiusEndMax	);

	m_bMustUpdate = true;
}


bool
Spawn_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_sModelName",				m_sModelName			);
	persist.Xfer( "m_SpawnInterval",			m_SpawnInterval			);
	persist.Xfer( "m_SpawnCount",				m_SpawnCount			);
	persist.Xfer( "m_MaxObjects",				m_MaxObjects			);
				
	persist.Xfer( "m_RadiusStartMin",			m_RadiusStartMin		);
	persist.Xfer( "m_RadiusStartMax",			m_RadiusStartMax		);
	persist.Xfer( "m_RadiusEndMin",				m_RadiusEndMin			);
	persist.Xfer( "m_RadiusEndMax",				m_RadiusEndMax			);
				
	persist.Xfer( "m_LinearVelocity",			m_LinearVelocity		);
	persist.Xfer( "m_LinearVelocityVar",		m_LinearVelocityVar		);
	persist.Xfer( "m_LinearAcceleration",		m_LinearAcceleration	);
	persist.Xfer( "m_LinearAccelerationVar",	m_LinearAccelerationVar );
	persist.Xfer( "m_AngularVelocity",			m_AngularVelocity		);
	persist.Xfer( "m_AngularVelocityVar",		m_AngularVelocityVar	);
	persist.Xfer( "m_AngularAcceleration",		m_AngularAcceleration	);
	persist.Xfer( "m_AngularAccelerationVar",	m_AngularAccelerationVar);

	persist.Xfer( "m_SpawnElapsed",				m_SpawnElapsed			);
	persist.Xfer( "m_TotalSpawned",				m_TotalSpawned			);

	return true;
}


void
Spawn_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		// Position initialization
		SiegePos	targetPos;
		matrix_3x3	target_orientation;

		m_Targets->GetPlacement( &targetPos, &target_orientation, m_OrientMode );
		
		m_Pos.pos = targetPos.pos + ( target_orientation * m_Offset );
		m_Pos.node = targetPos.node;
	}

	m_SpawnElapsed -= m_FrameSync;

	if( m_SpawnElapsed <= 0 )
	{
		m_SpawnElapsed = m_SpawnInterval;

		for( UINT32 i = (UINT32)m_SpawnCount; i > 0; --i )
		{
			m_TotalSpawned++;
			SpawnObject();

			if( m_TotalSpawned >= m_MaxObjects )
			{
				EndThisEffect();
				break;
			}
		}
	}

	if( m_bFinishing )
	{
		EndThisEffect();
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}


float
Spawn_Effect::GetBoundingRadius( void )
{
	return max_t( m_RadiusStartMax, m_RadiusEndMax );
}


void
Spawn_Effect::SpawnObject( void )
{
	// Pick a random angle to start both of the random radii by
	float theta = gWorldFx.GetRNG().Random( PI2 );

	// Get trig values
	float sin_theta, cos_theta;
	SINCOSF( theta, sin_theta, cos_theta );

	// Pick a random radius for the path we are following on
	// the cone
	const float r0 = m_Scale * gWorldFx.GetRNG().Random( m_RadiusStartMin,	m_RadiusStartMax );
	const float r1 = m_Scale * gWorldFx.GetRNG().Random( m_RadiusEndMin,	m_RadiusEndMax );

	// Position on the XY plane for the inner radius 
	// starting at the base of the cone
	vector_3 inner_pos( r0 * cos_theta, 0, r0 * sin_theta );

	// Position along the XY plane for the outer radius with
	// some random Z displacement to give the conical volume
	vector_3 outer_pos( r1 * cos_theta, 0, r1 * sin_theta );

	// Get the orientation of the emission point
	matrix_3x3	orientation;
	m_Targets->GetPlacement( NULL, &orientation, m_OrientMode );

	// Orient to the space of the emission point
	inner_pos = orientation * inner_pos;
	outer_pos = orientation * outer_pos;

	// Calculate the difference and use that as the slope to
	// trave along the conical section from the emission point
	vector_3 perturb( outer_pos - inner_pos );

	// Create some random vectors within specified ranges for accleration and velocity	
	const vector_3 rla(	gWorldFx.GetRNG().Random( -m_LinearAccelerationVar.x,	m_LinearAccelerationVar.x	),
						gWorldFx.GetRNG().Random( -m_LinearAccelerationVar.y,	m_LinearAccelerationVar.y	),
						gWorldFx.GetRNG().Random( -m_LinearAccelerationVar.z,	m_LinearAccelerationVar.z	) );
	const vector_3 rlv(	gWorldFx.GetRNG().Random( -m_LinearVelocityVar.x,		m_LinearVelocityVar.x		),
						gWorldFx.GetRNG().Random( -m_LinearVelocityVar.y,		m_LinearVelocityVar.y		),
						gWorldFx.GetRNG().Random( -m_LinearVelocityVar.z,		m_LinearVelocityVar.z		) );
	const vector_3 raa(	gWorldFx.GetRNG().Random( -m_AngularAccelerationVar.x,	m_AngularAccelerationVar.x	),
						gWorldFx.GetRNG().Random( -m_AngularAccelerationVar.y,	m_AngularAccelerationVar.y	),
						gWorldFx.GetRNG().Random( -m_AngularAccelerationVar.z,	m_AngularAccelerationVar.z	) );
	const vector_3 rav(	gWorldFx.GetRNG().Random( -m_AngularVelocityVar.x,		m_AngularVelocityVar.x		),
						gWorldFx.GetRNG().Random( -m_AngularVelocityVar.y,		m_AngularVelocityVar.y		),
						gWorldFx.GetRNG().Random( -m_AngularVelocityVar.z,		m_AngularVelocityVar.z		) );

	vector_3 linear_acceleration	( perturb + (orientation * (m_LinearAcceleration + rla) ));
	vector_3 linear_velocity		( perturb + (orientation * (m_LinearVelocity + rlv) ));
	vector_3 angular_acceleration	( m_AngularAcceleration + raa );
	vector_3 angular_velocity		( m_AngularVelocity + rav );

	// Check to see if we should apply the currently available wind
	// vector to our velocity
	if( m_bWindAffected )
	{
		vector_3 wind_vector( DoNotInitialize );
		gWorldFx.GetNodeWindVector( m_Pos.node, wind_vector );

		linear_velocity += 5.0f * wind_vector;
	}


	// Create the object position to spawn at
	SiegePos object_position( m_Pos );

	// We don't want any puffiness so pick a timestep to bypass lame and
	// undesirable side effects from weird periodicity
	float offset_start_time = m_FrameSync * RandomFloat();

	vector_3 offset_position( inner_pos + (( 0.5f * (float)Square( offset_start_time ) * linear_acceleration )
						+ linear_velocity * ( offset_start_time )) );

	linear_velocity += linear_acceleration * ( offset_start_time );

	bool bNodeChange = false;

	if( !gWorldFx.AddPositions( object_position, offset_position, false, &bNodeChange ) )
	{
		return;
	}

	if( bNodeChange )
	{
		gWorldFx.ReOrientVector( m_Pos.node, object_position.node, linear_velocity );
	}

	// Spawn the object into the world
	gWorldFx.SpawnLocalPhysicsObject( m_sModelName, object_position, linear_acceleration,
					 				 linear_velocity, angular_acceleration, angular_velocity, GetFriendly() );
}




// SRay effect ---

SRay_Effect::SRay_Effect( void )
{
	m_Radius			=	0.0f;
	m_Count				=	0;

	m_RayLen_min		=	0.0f;
	m_RayLen_max		=	0.0f;

	m_RayW_start_min	=	0.0f;
	m_RayW_start_max	=	0.0f;
	m_RayW_end_min		=	0.0f;
	m_RayW_end_max		=	0.0f;

	m_bInstant			=	false;

	m_SpawnRate			=	0.0f;
	m_SpawnTime			=	0.0f;

	m_bNoTexture		=	false;

	m_pDrawVerts		=	NULL;
}


SRay_Effect::~SRay_Effect( void )
{
	delete [] m_pDrawVerts;
}


void
SRay_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.9f, 0.9f, 1.0f ); // PARAM(1)
	params.SetVector3	( "color1",		0.2f, 0.2f, 1.0f ); // PARAM(2)
	params.SetVector3	( "color2",		0.0f, 0.0f, 0.0f ); // PARAM(3)
	params.SetVector3	( "theta",		0.0f, 1.0f, 3.0f ); // PARAM(4)
	params.SetVector3	( "phi",		0.0f, 1.0f, -3.0f); // PARAM(5)
	params.SetVector3	( "alpha",		1.0f, 0.5f, 0.5f ); // PARAM(6)

	params.SetUINT32	( "count",		16		); // PARAM(7)

	params.SetFloat		( "radius",		0.0005f	); // PARAM(8)
	params.SetFloat		( "lmin",		10.0f	); // PARAM(9)
	params.SetFloat		( "lmax",		10.0f	); // PARAM(10)
	params.SetFloat		( "wsmin",		0.15f	); // PARAM(11)
	params.SetFloat		( "wsmax",		0.15f	); // PARAM(12)
	params.SetFloat		( "wemin",		0.15f	); // PARAM(13)
	params.SetFloat		( "wemax",		0.15f	); // PARAM(14)
	params.SetFloat		( "srate",		0.015f	); // PARAM(15)

	params.SetBool		( "instant",	false	); // PARAM(16)
	params.SetBool		( "no_tex",		false	); // PARAM(17)
}


void
SRay_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Start_color	);
	m_Params.GetVector3	( PARAM(2),		m_End_color		);
	m_Params.GetVector3	( PARAM(3),		m_Vari_color	);
	m_Params.GetVector3	( PARAM(4),		m_ThetaSettings );
	m_Params.GetVector3	( PARAM(5),		m_PhiSettings	);
	m_Params.GetVector3	( PARAM(6),		m_AlphaSettings	);

	m_Params.GetUINT32	( PARAM(7),		m_Count			);

	m_Params.GetFloat	( PARAM(8),		m_Radius		);
	m_Params.GetFloat	( PARAM(9),		m_RayLen_min	);
	m_Params.GetFloat	( PARAM(10),	m_RayLen_max	);
	m_Params.GetFloat	( PARAM(11),	m_RayW_start_min);
	m_Params.GetFloat	( PARAM(12),	m_RayW_start_max);
	m_Params.GetFloat	( PARAM(13),	m_RayW_end_min	);
	m_Params.GetFloat	( PARAM(14),	m_RayW_end_max	);
	m_Params.GetFloat	( PARAM(15),	m_SpawnRate	);

	m_Params.GetBool	( PARAM(16),	m_bInstant		);
	m_Params.GetBool	( PARAM(17),	m_bNoTexture	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	ValidateOrder		( m_ThetaSettings.y, m_ThetaSettings.z );
	ValidateOrder		( m_PhiSettings.y, m_PhiSettings.z );
	ValidateOrder		( m_AlphaSettings.y, m_AlphaSettings.z );
	ValidateOrder		( m_RayLen_min,	m_RayLen_max );
	ValidateAngle		( m_ThetaSettings.x );
	ValidateAngle		( m_PhiSettings.x );

	m_Count				= min_t( (int)m_Count, MAX_RAY_COUNT	);
	m_RayLen_min		*= m_ScaleFactor;
	m_RayLen_max		*= m_ScaleFactor;
	m_Radius			= max_t( m_ScaleFactor * m_Radius, 0.005f  );

	m_RayW_start_min	*= m_ScaleFactor;
	m_RayW_start_max	*= m_ScaleFactor;
	m_RayW_end_min		*= m_ScaleFactor;
	m_RayW_end_max		*= m_ScaleFactor;

	ValidateOrder		( m_RayW_start_min,	m_RayW_start_max );
	ValidateOrder		( m_RayW_end_min,	m_RayW_end_max );

	m_SpawnTime			= m_SpawnRate;
	m_SpawnRate			= (m_SpawnRate==0)?0:(1.0f / m_SpawnRate);
}


UINT32
SRay_Effect::InitializeTexture( void )
{
	if( !m_bNoTexture && !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 3 );
	}
	return m_TextureName;
}


bool
SRay_Effect::ReInitialize( void )
{
	// Prep the draw verts & uv coords once
	m_pDrawVerts		= new sVertex[ 4*m_Count ];

	for( UINT32 i = 0; i < 4*m_Count; i+=4 )
	{
		m_pDrawVerts[ i+0 ].uv.u	=	0.0f;
		m_pDrawVerts[ i+0 ].uv.v	=	0.0f;

		m_pDrawVerts[ i+1 ].uv.u	=	1.0f;
		m_pDrawVerts[ i+1 ].uv.v	=	0.0f;
	
		m_pDrawVerts[ i+2 ].uv.u	=	0.0f;
		m_pDrawVerts[ i+2 ].uv.v	=	1.0f;
		
		m_pDrawVerts[ i+3 ].uv.u	=	1.0f;
		m_pDrawVerts[ i+3 ].uv.v	=	1.0f;
	}
	return true;
}



FUBI_DECLARE_SELF_TRAITS( SRay_Effect::SRay );


bool
SRay_Effect::SRay::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "alpha",				alpha );
	persist.Xfer	( "alpha_fade",			alpha_fade );

	persist.Xfer	( "start_color",		start_color );
	persist.Xfer	( "end_color",			end_color );

	persist.Xfer	( "length",				length );

	persist.Xfer	( "width_start",		width_start );
	persist.Xfer	( "width_end",			width_end );

	persist.Xfer	( "spin_theta",			spin_theta );
	persist.Xfer	( "spin_phi",			spin_phi );

	persist.Xfer	( "spin_itheta",		spin_theta );
	persist.Xfer	( "spin_iphi",			spin_phi );

	persist.Xfer	( "start_pos",			start_pos );
	persist.Xfer	( "end_pos",			end_pos );

	return true;
}


bool
SRay_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_Start_color",		m_Start_color );
	persist.Xfer	( "m_End_color",		m_End_color );
	persist.Xfer	( "m_Vari_color",		m_Vari_color );

	persist.Xfer	( "m_Radius",			m_Radius );
	persist.Xfer	( "m_Count",			m_Count );
	persist.Xfer	( "m_RayLen_min",		m_RayLen_min );
	persist.Xfer	( "m_RayLen_max",		m_RayLen_max );

	persist.Xfer	( "m_RayW_start_min",	m_RayW_start_min );
	persist.Xfer	( "m_RayW_start_max",	m_RayW_start_max );
	persist.Xfer	( "m_RayW_end_min",		m_RayW_end_min );
	persist.Xfer	( "m_RayW_end_max",		m_RayW_end_max );

	persist.Xfer	( "m_ThetaSettings",	m_ThetaSettings );
	persist.Xfer	( "m_PhiSettings",		m_PhiSettings );
	persist.Xfer	( "m_AlphaSettings",	m_AlphaSettings );

	persist.Xfer	( "m_bInstant",			m_bInstant );

	persist.Xfer	( "m_SpawnRate",		m_SpawnRate );
	persist.Xfer	( "m_SpawnTime",		m_SpawnTime );

	persist.Xfer	( "m_bNoTexture",		m_bNoTexture );
	
	persist.XferList( "m_Rays",				m_Rays );

	return true;
}


void
SRay_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;
		ReInitialize();
		// If instant create all the rays all at once
		if( m_bInstant )
		{
			SRay ray;

			for( UINT32 i = 0; i < m_Count; ++i )
			{
				CreateRay( ray );
				m_Rays.push_back( ray );
			}
		}
	}

	// Spawn new rays according to spawn rate if they weren't supposed to all spawn at once
	if( !m_bInstant && ( m_Rays.size() < m_Count ) )
	{
		if( m_SpawnTime > 0 )
		{
			m_SpawnTime -= m_FrameSync;
		}
		else
		{
			m_SpawnTime = 1.0f / m_SpawnRate;
			// Determine how many rays to spawn
			const UINT32 spawn_count = 1 + UINT32(floorf( FABSF( m_SpawnTime ) * m_SpawnRate ));
			
			SRay ray;

			for( UINT32 i = spawn_count; i > 0; --i )
			{
				CreateRay( ray );
				m_Rays.push_back( ray );
			}
		}
	}

	// Update the rays
	bool	bNothing_active = true;
	RayColl::iterator iRay = m_Rays.begin(), iEnd = m_Rays.end();
	for(; iRay != iEnd; ++iRay )
	{
		// Get the ray reference
		SRay &ray = *iRay;

		if( ray.alpha > 0.0f )
		{
			// Decrease alpha
			ray.alpha -= ray.alpha_fade * m_FrameSync;
			FilterClamp( 0.0f, 1.0f, ray.alpha );

			// Update positions based on spin rates
			ray.spin_theta		+= ray.spin_itheta * m_FrameSync;
			ray.spin_phi		+= ray.spin_iphi * m_FrameSync;

			float theta_sin		= 0.0f;
			float theta_cos		= 0.0f;
			float phi_sin		= 0.0f;
			float phi_cos		= 0.0f;
			SINCOSF( ray.spin_theta, theta_sin, theta_cos );
			SINCOSF( ray.spin_phi, phi_sin, phi_cos );

			const float temp	= m_Radius * theta_sin;
			ray.start_pos.x		= temp * phi_cos;
			ray.start_pos.y		= temp * phi_sin;
			ray.start_pos.z		= m_Radius * theta_cos;

			ray.end_pos			= ray.start_pos + ray.length * Normalize( ray.start_pos );
			bNothing_active		= false;
		}
		else if( !m_bFinishing )
		{
			// Create a new ray to take the place of the old expired ray
			CreateRay( *iRay );
		}
	}

	if( bNothing_active && m_bFinishing )
	{
		EndThisEffect();
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
SRay_Effect::CreateRay( SRay &ray )
{
	// Set initial alpha
	ray.alpha			= m_AlphaSettings.x;

	// Set the alpha fade
	ray.alpha_fade		= gWorldFx.GetRNG().Random( m_AlphaSettings.y, m_AlphaSettings.z );

	// Set ray color with color variance and clamp it
	ray.start_color.x	= m_Start_color.x	+ gWorldFx.GetRNG().Random( -m_Vari_color.x, m_Vari_color.x );
	ray.start_color.y	= m_Start_color.y	+ gWorldFx.GetRNG().Random( -m_Vari_color.y, m_Vari_color.y );
	ray.start_color.z	= m_Start_color.z	+ gWorldFx.GetRNG().Random( -m_Vari_color.z, m_Vari_color.z );
	ray.end_color.x		= m_End_color.x		+ gWorldFx.GetRNG().Random( -m_Vari_color.x, m_Vari_color.x );
	ray.end_color.y		= m_End_color.y		+ gWorldFx.GetRNG().Random( -m_Vari_color.y, m_Vari_color.y );
	ray.end_color.z		= m_End_color.z		+ gWorldFx.GetRNG().Random( -m_Vari_color.z, m_Vari_color.z );
	FilterClamp( 0.0f, 1.0f, ray.start_color.x );
	FilterClamp( 0.0f, 1.0f, ray.start_color.y );
	FilterClamp( 0.0f, 1.0f, ray.start_color.z );
	FilterClamp( 0.0f, 1.0f, ray.end_color.x );
	FilterClamp( 0.0f, 1.0f, ray.end_color.y );
	FilterClamp( 0.0f, 1.0f, ray.end_color.z );

	// Set the length of the ray
	ray.length			= gWorldFx.GetRNG().Random( m_RayLen_min, m_RayLen_max );

	// Set the start and end widths of theray
	ray.width_start		= gWorldFx.GetRNG().Random( m_RayW_start_min, m_RayW_start_max );
	ray.width_end		= gWorldFx.GetRNG().Random( m_RayW_end_min, m_RayW_end_max );

	// Set the starting angles
	ray.spin_theta		= gWorldFx.GetRNG().Random( m_ThetaSettings.x, PI2 );
	ray.spin_phi		= gWorldFx.GetRNG().Random( m_PhiSettings.x, PI2 );

	// Set the spin rates
	ray.spin_itheta		= gWorldFx.GetRNG().Random( m_ThetaSettings.y, m_ThetaSettings.z );
	ray.spin_iphi		= gWorldFx.GetRNG().Random( m_PhiSettings.y, m_PhiSettings.z );

	// Set the intial position
	ray.spin_theta		+= ray.spin_itheta * m_FrameSync;
	ray.spin_phi		+= ray.spin_iphi * m_FrameSync;

	float theta_sin		= 0.0f;
	float theta_cos		= 0.0f;
	float phi_sin		= 0.0f;
	float phi_cos		= 0.0f;
	SINCOSF( ray.spin_theta, theta_sin, theta_cos );
	SINCOSF( ray.spin_phi, phi_sin, phi_cos );

	const float temp	= m_Radius * theta_sin;
	ray.start_pos.x		= temp * phi_cos;
	ray.start_pos.y		= temp * phi_sin;
	ray.start_pos.z		= m_Radius * theta_cos;

	ray.end_pos			= ray.start_pos + ray.length * Normalize( ray.start_pos );
}


void
SRay_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	Rapi &renderer = gWorldFx.GetRenderer();

	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,	D3DSHADE_GOURAUD );

	renderer.SetTextureStageState(	0,
									D3DTOP_MODULATE,
									D3DTOP_MODULATE,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_ONE,
									false );
	

	RayColl::iterator iRay = m_Rays.begin(), iEnd = m_Rays.end();
	for( UINT32 i = 0; iRay != iEnd; ++iRay, i+=4 )
	{
		SRay &ray = *iRay;

		if( ray.alpha <= 0 )
		{
			continue;
		}

		DWORD start_color = ((MAKEDWORDCOLOR( ray.start_color )&0x00FFFFFF)|((BYTE(ray.alpha*m_NodeAlpha*255.0f))<<24));
		DWORD end_color	  = ((MAKEDWORDCOLOR( ray.end_color )&0x00FFFFFF)|((BYTE(ray.alpha*m_NodeAlpha*255.0f))<<24));

		vector_3	cross( CrossProduct( ray.start_pos, m_Vz ));
		vector_3	width_vector;

		if( !cross.IsZero() )
		{
			width_vector = Normalize( cross );
		}

		vector_3	start_offset( ray.width_start * width_vector );
		vector_3	end_offset( ray.width_end * width_vector );
		vector_3	p0( ray.start_pos + start_offset );
		vector_3	p1( ray.start_pos - start_offset );
		vector_3	p2( ray.end_pos + end_offset );
		vector_3	p3( ray.end_pos - end_offset );

		m_pDrawVerts[ i+0 ].x		=	p0.x;
		m_pDrawVerts[ i+0 ].y		=	p0.y;
		m_pDrawVerts[ i+0 ].z		=	p0.z;
		m_pDrawVerts[ i+0 ].color	=	start_color;

		m_pDrawVerts[ i+1 ].x		=	p2.x;
		m_pDrawVerts[ i+1 ].y		=	p2.y;
		m_pDrawVerts[ i+1 ].z		=	p2.z;
		m_pDrawVerts[ i+1 ].color	=	end_color;

		m_pDrawVerts[ i+2 ].x		=	p1.x;
		m_pDrawVerts[ i+2 ].y		=	p1.y;
		m_pDrawVerts[ i+2 ].z		=	p1.z;
		m_pDrawVerts[ i+2 ].color	=	start_color;

		m_pDrawVerts[ i+3 ].x		=	p3.x;
		m_pDrawVerts[ i+3 ].y		=	p3.y;
		m_pDrawVerts[ i+3 ].z		=	p3.z;
		m_pDrawVerts[ i+3 ].color	=	end_color;

	}

	if( !m_Rays.empty() )
	{
		renderer.DrawIndexedPrimitive( D3DPT_TRIANGLELIST, &m_pDrawVerts[0], 4*m_Rays.size(), SVERTEX, gWorldFx.GetGlobalDrawIndexArray(),
										6*m_Rays.size(), &m_TextureName, 1 );
	}

	gWorldFx.SaveRenderingState();
}


float
SRay_Effect::GetBoundingRadius( void )
{
	return m_RayLen_max + m_Radius;
}




// Decal effect ---

Decal_Effect::Decal_Effect( void )
{
	m_In_dur			=	0.0f;
	m_Out_dur			=	0.0f;

	m_near				=	0.0f;
	m_far				=	0.0f;

	m_DecalID			=	0;

	m_MaxAlpha			=	0.0f;
	m_Alpha				=	0.0f;
	m_bRemoveTexture	=	false;
}


Decal_Effect::~Decal_Effect( void )
{
	gWorldFx.DestroyDecal( m_DecalID, m_bRemoveTexture );
}


void
Decal_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetGPString	( "name",	"b_d_blood.%img%"	); // PARAM(1)

	params.SetFloat		( "tin",	1.0f	); // PARAM(2)
	params.SetFloat		( "tout",	1.0f	); // PARAM(3)
	params.SetFloat		( "near",	0.1f	); // PARAM(4)
	params.SetFloat		( "far",	1.0f	); // PARAM(5)
	params.SetFloat		( "alpha",	1.0f	); // PARAM(6)

	params.SetBool		( "save_texture",	false ); // PARAM(7)
}


void
Decal_Effect::InitializeDerived() 
{
	m_Params.GetGPString( PARAM(1),	m_sTexName	);

	m_Params.GetFloat	( PARAM(2),	m_In_dur	);
	m_Params.GetFloat	( PARAM(3),	m_Out_dur	);
	m_Params.GetFloat	( PARAM(4),	m_near		);
	m_Params.GetFloat	( PARAM(5),	m_far		);
	m_Params.GetFloat	( PARAM(6),	m_MaxAlpha	);
	m_MaxAlpha	=	min_t( 1.0f,	m_MaxAlpha );

	m_Params.GetBool	( PARAM(7),	m_bRemoveTexture );

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if( m_Duration == -1.0f )
	{
		m_Alpha = m_MaxAlpha;
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
}


bool
Decal_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_sTexName",			m_sTexName		);
	persist.Xfer( "m_In_dur",			m_In_dur		);
	persist.Xfer( "m_Out_dur",			m_Out_dur		);
	persist.Xfer( "m_near",				m_near			);
	persist.Xfer( "m_far",				m_far			);
	persist.Xfer( "m_MaxAlpha",			m_MaxAlpha		);
	persist.Xfer( "m_Alpha",			m_Alpha			);
	persist.Xfer( "m_bRemoveTexture",	m_bRemoveTexture);

	// m_DecalID does not need to 

	return true;
}


bool
Decal_Effect::ReInitialize( void )
{
	if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		return false;
	}

	gpstring sFilename;
	gNamingKey.BuildIMGLocation( m_sTexName, sFilename );

	gWorldFx.AdjustPointToTerrain( m_Pos );

	SiegePos start_pos( m_Pos );
	SiegePos end_pos  ( m_Pos );

	start_pos.pos.y -= 0.10f;

	vector_3		norm;
	SiegePos		decal_origin;

	gWorldFx.RayCast( start_pos, end_pos, norm, decal_origin );

	decal_origin.pos += norm;
	gWorldFx.CreateDecal( m_DecalID, decal_origin, MatrixFromDirection(-norm), m_Scale, m_Scale, 
								m_near, m_far, sFilename );
	return true;
}


void
Decal_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = ReInitialize();
		if( !m_bInitialized )
		{
			return;
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
			m_Alpha = ( m_Alpha <= m_MaxAlpha ) ? ( (m_Elapsed / m_In_dur) + 0.00001f):1.0f;
		}
	}

	m_Alpha		=	FilterClamp( 0.0f, m_MaxAlpha, m_Alpha );

	gWorldFx.SetDecalAlpha( m_DecalID, m_Alpha );

	m_bFinished = ( (m_bFinished) | (m_Alpha <= 0.0f) );

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}
