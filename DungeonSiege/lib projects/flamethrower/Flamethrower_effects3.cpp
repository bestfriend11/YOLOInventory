//
//	Flamethrower_effects3.cpp
//	Copyright 1999, Gas Powered Games
//
//

#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "vector_3.h"
#include "space_3.h"
#include "filter_1.h"

#include "Flamethrower_effect_base.h"
#include "Flamethrower_effects3.h"
#include "WorldFx.h"
#include "Flamethrower_tracker.h"
#include "Flamethrower_FuBi_support.h"

#include "RapiOwner.h"

#include "Physcalc.h"

#include "SkritSupport.h"
#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"



const vector_3 gravity( 0, -9.80665f, 0 );


// PolygonalExplosion Effect ---

PolygonalExplosion_Effect::PolygonalExplosion_Effect( void )
{
	m_RelativeGroundHeight		= 0;

	m_Fade_min					= 0;
	m_Fade_max					= 0;
	m_PolySides					= 0;
	m_PolyCount					= 0;
	m_Radius					= 0;

	m_bDarkBlending				= false;
	m_bOmniDir					= false;
	m_Magnitude					= 0;
}


void
PolygonalExplosion_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.2f, 0.2f, 0.85f		); // PARAM(1)
	params.SetVector3	( "color1",		0.0f, 0.0f, 0.0f		); // PARAM(2)
	params.SetVector3	( "fade_range",	0.5f, 1.0f, 0.0f		); // PARAM(3)
	params.SetVector3	( "rotrange",	200.0f, 200.0f, 200.0f	); // PARAM(4)
	params.SetVector3	( "displace",	0.0f, 0.0f, 0.0f		); // PARAM(5)

	params.SetUINT32	( "poly_sides",	9						); // PARAM(6)
	params.SetUINT32	( "count",		MAX_PARTICLES			); // PARAM(7)

	params.SetFloat		( "radius",		0.75f					); // PARAM(8)
	params.SetFloat		( "mag",		1.0f					); // PARAM(9)

	params.SetBool		( "dark",		false					); // PARAM(10)
	params.SetBool		( "omnidir",	false					); // PARAM(11)
}


void
PolygonalExplosion_Effect::InitializeDerived( void )
{
	vector_3	temp(DoNotInitialize);
	m_Params.GetVector3	( PARAM(1),		m_Base_color		);
	m_Params.GetVector3	( PARAM(2),		m_Color_variance	);
	m_Params.GetVector3	( PARAM(3),		temp	);
	m_Fade_min = temp.x;
	m_Fade_max = temp.y;
	m_Params.GetVector3	( PARAM(4),		m_RotationRange		);
	m_Params.GetVector3	( PARAM(5),		m_Displace			);

	m_Params.GetUINT32	( PARAM(6),		m_PolySides			);
	m_PolySides	= (m_PolySides > MAX_POLYSIDES-3)? MAX_POLYSIDES-3 : m_PolySides;
	m_Params.GetUINT32	( PARAM(7),		m_PolyCount			);
	m_PolyCount	= (m_PolyCount > MAX_PARTICLES )? MAX_PARTICLES : m_PolyCount;

	m_Params.GetFloat	( PARAM(8),		m_Radius			);
	m_Params.GetFloat	( PARAM(9),		m_Magnitude			);

	m_Params.GetBool	( PARAM(10),	m_bDarkBlending		);
	m_Params.GetBool	( PARAM(11),	m_bOmniDir			);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_Particles.resize( m_PolyCount );
	m_DrawVerts.resize( m_PolySides );

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}
}


FUBI_DECLARE_SELF_TRAITS( PolygonalExplosion_Effect::ExplosionParticle );

bool
PolygonalExplosion_Effect::ExplosionParticle::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer			( "orientation",	orientation		);
	persist.Xfer			( "position",		position		);
	persist.Xfer			( "velocity",		velocity		);

	persist.Xfer			( "rotation",		rotation		);
	persist.Xfer			( "rotation_delta",	rotation_delta	);

	persist.XferVector		( "verticies",		verticies		);
	persist.Xfer			( "vertex_count",	vertex_count	);

	persist.Xfer			( "color",			color			);

	persist.Xfer			( "alpha",			alpha			);
	persist.Xfer			( "alpha_fade",		alpha_fade		);

	return true;
}


bool
PolygonalExplosion_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer			( "m_RelativeGroundHeight",	m_RelativeGroundHeight	);

	persist.Xfer			( "m_Base_color",			m_Base_color			);
	persist.Xfer			( "m_Fade_min",				m_Fade_min				);
	persist.Xfer			( "m_Fade_max",				m_Fade_max				);
	persist.Xfer			( "m_PolySides",			m_PolySides				);
	persist.Xfer			( "m_PolyCount",			m_PolyCount				);
	persist.Xfer			( "m_Radius",				m_Radius				);

	persist.Xfer			( "m_bDarkBlending",		m_bDarkBlending			);
	persist.Xfer			( "m_bOmniDir",				m_bOmniDir				);
	persist.Xfer			( "m_Magnitude",			m_Magnitude				);
	persist.Xfer			( "m_RotationRange",		m_RotationRange			);
	persist.Xfer			( "m_Color_variance",		m_Color_variance		);
	persist.Xfer			( "m_Displace",				m_Displace				);

	persist.XferVector		( "m_Particles",			m_Particles				 );

	if( persist.IsRestoring() )
	{
		m_DrawVerts.resize( m_PolySides );
	}

	return true;
}


void
PolygonalExplosion_Effect::UpdateDerived( void )
{
	// constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		float range;

		if( !m_Targets->GetBoundingRadius( range ) )
		{
			range = 2.0f;
		}

		m_Targets->GetPlacement( &m_Pos, NULL, m_OrientMode );

		vector_3	v2( m_Dir );

		if( v2.IsZero() )
		{
			v2 = vector_3::UP;
		}

		matrix_3x3 bv = DextralColumns( v2 );

		vector_3	v1(bv.GetColumn_1());
		vector_3	v3(bv.GetColumn_2());

		for( UINT32 n = 0; n < m_PolyCount; ++n )
		{
			ExplosionParticle &particle = m_Particles[n];

			particle.alpha = 1.0f;
			particle.alpha_fade = gWorldFx.GetRNG().Random( m_Fade_min, m_Fade_max );

			float thetasin, thetacos;
			SINCOSF( gWorldFx.GetRNG().Random( PI2 ), thetasin, thetacos );
			float phisin, phicos;
			SINCOSF( gWorldFx.GetRNG().Random( PI2 ), phisin, phicos );

			const float radius = gWorldFx.GetRNG().Random( m_Radius );

			vector_3 p(	radius * thetasin * phicos,
						radius * thetasin * phisin,
						radius * thetacos );


			particle.position = p + m_Pos.pos + m_Offset;
			particle.position.x += gWorldFx.GetRNG().Random( -m_Displace.x, m_Displace.x );
			particle.position.y += gWorldFx.GetRNG().Random( -m_Displace.y, m_Displace.y );
			particle.position.z += gWorldFx.GetRNG().Random( -m_Displace.z, m_Displace.z );

			if( m_bOmniDir )
			{
				particle.velocity = m_Magnitude * p;
			}
			else
			{
				vector_3 vd((0.25f - gWorldFx.GetRNG().Random( 0.5f )),
							(0.25f - gWorldFx.GetRNG().Random( 0.5f )),
							(0.25f - gWorldFx.GetRNG().Random( 0.5f )) );

				particle.velocity = (v2+vd)*gWorldFx.GetRNG().Random( 3.0f, 6.5f );
			}

			particle.rotation_delta.x = ( DEGREES_TO_RADIANS * gWorldFx.GetRNG().Random( -m_RotationRange.x, m_RotationRange.x ) );
			particle.rotation_delta.y = ( DEGREES_TO_RADIANS * gWorldFx.GetRNG().Random( -m_RotationRange.y, m_RotationRange.y ) );
			particle.rotation_delta.z = ( DEGREES_TO_RADIANS * gWorldFx.GetRNG().Random( -m_RotationRange.z, m_RotationRange.z ) );
			particle.rotation += particle.rotation_delta * gWorldFx.GetRNG().Random(1.0f, 2.0f);

			const UINT32 vertex_count = (UINT32)gWorldFx.GetRNG().Random( 3, (int)max_t(3, (int)m_PolySides) );
			const float sd = 2.0f*RealPi/(float)vertex_count;

			particle.verticies.resize( vertex_count );

			particle.vertex_count = vertex_count;

			float sector = 0;

			particle.verticies[0] = vector_3::ZERO;

			for( UINT32 v = 1; v < vertex_count; ++v, sector += sd )
			{
				float radius = gWorldFx.GetRNG().Random( 0.05f, 0.75f );

				float phisin, phicos;
				SINCOSF( gWorldFx.GetRNG().Random( sector, sector + sd ), phisin, phicos );

				float	xp = m_ScaleFactor * radius * phicos;
				float	yp = m_ScaleFactor * radius * phisin;

				particle.verticies[v] = ( xp*v1 + yp*v3 );
			}

			const vector_3 color_variance(	gWorldFx.GetRNG().Random( -m_Color_variance.x, m_Color_variance.x ),
											gWorldFx.GetRNG().Random( -m_Color_variance.y, m_Color_variance.y ),
											gWorldFx.GetRNG().Random( -m_Color_variance.z, m_Color_variance.z ) );

			particle.color = MAKEDWORDCOLOR( m_Base_color ) + MAKEDWORDCOLOR( color_variance );
		}

		SiegePos position;
		position.pos	= m_Particles[0].position;
		position.node	= m_Pos.node;

		m_RelativeGroundHeight = gWorldFx.GetGroundHeight( position );
	}

	// move/rotate/fade each polys
	bool	bDone = true;

	for( UINT32 i = 0; i < (UINT32)m_PolyCount; ++i )
	{
		ExplosionParticle &particle = m_Particles[ i ];

		particle.position	+= 0.5f * (m_FrameSync*m_FrameSync) * gravity + particle.velocity * m_FrameSync;
		particle.velocity	+= gravity * m_FrameSync;
		particle.rotation	+= particle.rotation_delta * m_FrameSync;

		m_Particles[ i ].orientation = (  XRotationColumns( particle.rotation.x ) * YRotationColumns( particle.rotation.y )) 
										* ZRotationColumns( particle.rotation.z );

		if( m_RelativeGroundHeight >= particle.position.y )
		{
			particle.position.y		  = m_RelativeGroundHeight; 
			particle.velocity		  = vector_3::ZERO;
			particle.rotation_delta	  = vector_3::ZERO;
		}

		particle.alpha	-= particle.alpha_fade * m_FrameSync;
		particle.alpha	 = FilterClamp( 0.0f, 1.0f, particle.alpha );

		if( particle.alpha > 0 )
		{
			bDone = false;
		}
	}

	if( bDone )
	{
		EndThisEffect();
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
PolygonalExplosion_Effect::DrawSelf( void )
{
	Rapi &renderer = gWorldFx.GetRenderer();

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
	else
	{
		renderer.SetTextureStageState(0,
									D3DTOP_DISABLE,
									D3DTOP_DISABLE,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_ONE,
									false );
	}

	renderer.ScaleWorldMatrix( vector_3( m_Scale, m_Scale, m_Scale ) );

	for( UINT32 n = 0; n < m_PolyCount; ++n )
	{
		if( m_Particles[n].alpha > 0 )
		{
			renderer.SetWorldMatrix( m_Particles[ n ].orientation + m_NodeOrient, m_NodeOrient * m_Particles[ n ].position + m_NodeCentroid );

			DWORD color	= ( (m_Particles[n].color&0x00FFFFFF) | (( BYTE( m_Particles[n].alpha * m_NodeAlpha * 255.0f )) << 24) );

			for( UINT32 i = 0; i < m_Particles[n].vertex_count; ++i )
			{
				m_DrawVerts[i].x	= m_Particles[n].verticies[i].x;
				m_DrawVerts[i].y	= m_Particles[n].verticies[i].y;
				m_DrawVerts[i].z	= m_Particles[n].verticies[i].z;
				m_DrawVerts[i].color= color;
			}
			
			if( m_Particles[n].vertex_count > 2 )
			{
				renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, &*m_DrawVerts.begin(), m_Particles[n].vertex_count, SVERTEX, 0, 0 );
			}
		}
	}

	gWorldFx.SaveRenderingState();
}


float
PolygonalExplosion_Effect::GetBoundingRadius( void )
{
	return 2.0f * m_Scale;
}




// Lightsource Effect ---

Lightsource_Effect::Lightsource_Effect( void )
{
	m_In_dur				=	0;
	m_Out_dur;
	m_InnerFalloff_radius	=	0;
	m_OuterFalloff_radius	=	0;

	m_Color0Weight			=	0;
	m_Color1Weight			=	0;

	m_Flicker				=	0;
	m_FlickerRate			=	0;

	m_Visible				=	true;

	m_bCastShadow			=	true;
}


Lightsource_Effect::~Lightsource_Effect( void )
{
	UnInitialize();
}


void
Lightsource_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.5f, 0.5f, 0.5f);	// PARAM(1)
	params.SetVector3	( "color1",		0.5f, 0.5f, 0.5f);	// PARAM(2)

	params.SetFloat		( "iradius",	0.0f			);	// PARAM(3)
	params.SetFloat		( "radius",		5.0f			);	// PARAM(4)
	params.SetFloat		( "tin",		1.0f			);	// PARAM(5)
	params.SetFloat		( "tout",		1.0f			);	// PARAM(6)
	params.SetFloat		( "frequency",	0.025f			);	// PARAM(7)

	params.SetBool		( "draw_shadow",false			);	// PARAM(8)
}


void
Lightsource_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Color0 );
	if( !m_Params.GetVector3( PARAM(2),	m_Color1 ) )
	{
		m_Color1 = m_Color0;
	}

	m_Params.GetFloat	( PARAM(3),	m_InnerFalloff_radius	);
	m_Params.GetFloat	( PARAM(4),	m_OuterFalloff_radius	);
	m_Params.GetFloat	( PARAM(5),	m_In_dur				);
	m_Params.GetFloat	( PARAM(6),	m_Out_dur				);
	m_Params.GetFloat	( PARAM(7),	m_FlickerRate			);

	m_Params.GetBool	( PARAM(8),	m_bCastShadow			);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();
}


bool
Lightsource_Effect::OnXfer( FuBi::PersistContext &persist )
{
	// m_Light_id is not xferred because it is destroyed and recreated

	persist.Xfer	( "m_In_dur",				m_In_dur				);
	persist.Xfer	( "m_Out_dur",				m_Out_dur				);
	persist.Xfer	( "m_InnerFalloff_radius",	m_InnerFalloff_radius	);
	persist.Xfer	( "m_OuterFalloff_radius",	m_OuterFalloff_radius	);

	persist.Xfer	( "m_Color0Weight",			m_Color0Weight			);
	persist.Xfer	( "m_Color1Weight",			m_Color1Weight			);

	persist.Xfer	( "m_Flicker",				m_Flicker				);
	persist.Xfer	( "m_FlickerRate",			m_FlickerRate			);

	persist.Xfer	( "m_Color0",				m_Color0				);
	persist.Xfer	( "m_Color1",				m_Color1				);

	persist.Xfer	( "m_Visible",				m_Visible				);
	persist.Xfer	( "m_bCastShadow",			m_bCastShadow			);


	return true;
}


bool
Lightsource_Effect::ReInitialize( void )
{
	if( !gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		return false;
	}

	return gWorldFx.CreateLightSource( m_Light_id, m_Pos, vector_3::ZERO, m_InnerFalloff_radius, m_OuterFalloff_radius, m_bCastShadow );
}


bool
Lightsource_Effect::UnInitialize( void )
{
	m_bInitialized = false;
	gWorldFx.DestroyLightSource( m_Light_id );
	return true;
}


void
Lightsource_Effect::UpdateDerived( void )
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

	if( m_bFinishing )
	{
		EndThisEffect();
		return;
	}

	float Intensity = 1.0f;

	if( m_Elapsed < m_In_dur )
	{
		Intensity = m_Elapsed / m_In_dur;
	}
	else if( (m_Duration != -1.0f) && (m_Elapsed >= (m_Duration - m_Out_dur)) )
	{
		Intensity = (m_Duration - m_Elapsed) / m_Out_dur;
	}

	m_Flicker -= m_FrameSync;

	if( m_Flicker <= 0 )
	{
		m_Flicker		= m_FlickerRate;
		m_Color0Weight	= RandomFloat();
		m_Color1Weight	= 1.0f - m_Color0Weight;
	}

	const unsigned long diffuseColor = MAKEDWORDCOLOR( Intensity * (m_Color0 * m_Color0Weight + m_Color1 * m_Color1Weight) );

	// Update the position of the light
	if( !gWorldFx.PositionLightSource( m_Light_id, m_Pos, diffuseColor ) )
	{
		// For some reason the light_id is not valid anymore so no harm done in destroying it
		gWorldFx.DestroyLightSource( m_Light_id );

		// Create a new lightsource and over write the old id
		gWorldFx.CreateLightSource( m_Light_id, m_Pos, GETDWORDCOLORV( diffuseColor ), m_InnerFalloff_radius, m_OuterFalloff_radius, m_bCastShadow );
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}





// Sphere effect ---

Sphere_Effect::Sphere_Effect( void )
	: m_Alpha(0)
{
	m_bDrawLines		= false;
	m_bCull				= false;

	m_InDur				= 0;
	m_OutDur			= 0;
	m_Alpha				= 0;
	m_Sides				= 0;

	m_CurrentScaleFactor= 0;
	m_ScaleIndex		= 0;
	m_Subdivision		= 0;


	memset( m_DrawVerts, 0, sizeof( sVertex ) * 4 );

	vector_3	v( 0.5257311121191f, 0, 0.8506508083520f );

	// Verticies
	m_Verts[ 0] = vector_3( -v.x,  v.y,  v.z );
	m_Verts[ 1] = vector_3(  v.x,  v.y,  v.z );
	m_Verts[ 2] = vector_3( -v.x,  v.y, -v.z );
	m_Verts[ 3] = vector_3(  v.x,  v.y, -v.z );

	m_Verts[ 4] = vector_3(  v.y,  v.z,  v.x );
	m_Verts[ 5] = vector_3(  v.y,  v.z, -v.x );
	m_Verts[ 6] = vector_3(  v.y, -v.z,  v.x );
	m_Verts[ 7] = vector_3(  v.y, -v.z, -v.x );

	m_Verts[ 8] = vector_3(  v.z,  v.x,  v.y );
	m_Verts[ 9] = vector_3( -v.z,  v.x,  v.y );
	m_Verts[10] = vector_3(  v.z, -v.x,  v.y );
	m_Verts[11] = vector_3( -v.z, -v.x,  v.y );

	// Connect points
	m_ConPts[ 0][0] = 0;  m_ConPts[ 0][1] = 4;  m_ConPts[ 0][2] = 1;
	m_ConPts[ 1][0] = 0;  m_ConPts[ 1][1] = 9;  m_ConPts[ 1][2] = 4;
	m_ConPts[ 2][0] = 9;  m_ConPts[ 2][1] = 5;  m_ConPts[ 2][2] = 4;
	m_ConPts[ 3][0] = 4;  m_ConPts[ 3][1] = 5;  m_ConPts[ 3][2] = 8;
	m_ConPts[ 4][0] = 4;  m_ConPts[ 4][1] = 8;  m_ConPts[ 4][2] = 1;

	m_ConPts[ 5][0] = 6;  m_ConPts[ 5][1] = 1;  m_ConPts[ 5][2] = 10;
	m_ConPts[ 6][0] = 9;  m_ConPts[ 6][1] = 0;  m_ConPts[ 6][2] = 11;
	m_ConPts[ 7][0] = 9;  m_ConPts[ 7][1] = 11; m_ConPts[ 7][2] = 2;
	m_ConPts[ 8][0] = 9;  m_ConPts[ 8][1] = 2;  m_ConPts[ 8][2] = 5;
	m_ConPts[ 9][0] = 7;  m_ConPts[ 9][1] = 2;  m_ConPts[ 9][2] = 11;


	m_ConPts[10][0] = 7;  m_ConPts[10][1] = 10; m_ConPts[10][2] = 3;
	m_ConPts[11][0] = 7;  m_ConPts[11][1] = 6;  m_ConPts[11][2] = 10;
	m_ConPts[12][0] = 7;  m_ConPts[12][1] = 11; m_ConPts[12][2] = 6;
	m_ConPts[13][0] = 11; m_ConPts[13][1] = 0;  m_ConPts[13][2] = 6;
	m_ConPts[14][0] = 0;  m_ConPts[14][1] = 1;  m_ConPts[14][2] = 6;


	m_ConPts[15][0] = 8;  m_ConPts[15][1] = 10; m_ConPts[15][2] = 1;
	m_ConPts[16][0] = 8;  m_ConPts[16][1] = 3;  m_ConPts[16][2] = 10;
	m_ConPts[17][0] = 5;  m_ConPts[17][1] = 3;  m_ConPts[17][2] = 8;
	m_ConPts[18][0] = 5;  m_ConPts[18][1] = 2;  m_ConPts[18][2] = 3;
	m_ConPts[19][0] = 2;  m_ConPts[19][1] = 7;  m_ConPts[19][2] = 3;
}


void
Sphere_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		 0.25f, 0.0f, 0.50f	);	// PARAM(1)
	params.SetVector3	( "color1",		 0.05f, 0.05f, 0.05f);	// PARAM(2)
	params.SetVector3	( "grow_params", 1.0f, 1.0f, 1.0f	);	// PARAM(3)
	params.SetVector3	( "irotate",	 0.0f, 0.0f, 0.0f	);	// PARAM(4)

	params.SetUINT32	( "subd",		 1					);	// PARAM(5)

	params.SetFloat		( "tin",		 1.0f				);	// PARAM(6)
	params.SetFloat		( "tout",		 1.0f				);	// PARAM(7)
	params.SetFloat		( "sides",		 BASE_SIDES			);	// PARAM(8)

	params.SetBool		( "lines",		 false	 			);	// PARAM(9)
	params.SetBool		( "cull",		 false			 	);	// PARAM(10)
}


void
Sphere_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);
	m_Params.GetVector3	( PARAM(2),		m_Line_color	);
	m_Params.GetVector3	( PARAM(3),		m_ScaleFactors	);
	m_bRotation = m_Params.GetVector3	( PARAM(4),	 m_RotationIncrements );

	m_Params.GetUINT32	( PARAM(5),		m_Subdivision	);

	m_Params.GetFloat	( PARAM(6),		m_InDur			);
	m_Params.GetFloat	( PARAM(7),		m_OutDur		);
	m_Params.GetFloat	( PARAM(8),		m_Sides			);

	m_Params.GetBool	( PARAM(9),		m_bDrawLines 	);
	m_Params.GetBool	( PARAM(10),	m_bCull		 	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_ScaleIndex = 0;
	m_CurrentScaleFactor = m_ScaleFactors.x;
}


bool
Sphere_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_bDrawLines",			m_bDrawLines			);
	persist.Xfer( "m_bCull",				m_bCull					);

	persist.Xfer( "m_InDur",				m_InDur					);
	persist.Xfer( "m_OutDur",				m_OutDur				);
	persist.Xfer( "m_Alpha",				m_Alpha					);
	persist.Xfer( "m_Sides",				m_Sides					);

	persist.Xfer( "m_CamDir",				m_CamDir				);

	persist.Xfer( "m_RotationIncrements",	m_RotationIncrements	);

	persist.Xfer( "m_ScaleFactors",			m_ScaleFactors			);
	persist.Xfer( "m_CurrentScaleFactor",	m_CurrentScaleFactor	);
	persist.Xfer( "m_ScaleIndex",			m_ScaleIndex			);

	persist.Xfer( "m_Subdivision",			m_Subdivision			);

	persist.Xfer( "m_Base_color",			m_Base_color			);
	persist.Xfer( "m_Line_color",			m_Line_color			);

	return true;
}


void
Sphere_Effect::UpdateDerived( void )
{
	vector_3	z_sight( 0, 0, 1 );
	m_CamDir = m_Cam * z_sight;

	if( m_bRotation )
	{
		m_RotationAngles += (m_RotationIncrements * m_FrameSync);

		m_RotationMatrix = (XRotationColumns( m_RotationAngles.x ) *
							YRotationColumns( m_RotationAngles.y ) *
							ZRotationColumns( m_RotationAngles.z ) );

		gWorldFx.GetRenderer().RotateWorldMatrix( m_Orient * m_RotationMatrix );
	}


	float	k = m_Elapsed / m_Duration;

	if( (k >= 0.5f) && !m_ScaleIndex )
	{
		++m_ScaleIndex;
	}

	float	start = m_ScaleFactors.GetElementsPointer()[ m_ScaleIndex ];
	float	stop = m_ScaleFactors.GetElementsPointer()[ m_ScaleIndex+1 ];

	if( !m_ScaleIndex )
	{
		m_CurrentScaleFactor = m_Scale * (start + 2*k*( stop - start ));
	}
	else
	{
		m_CurrentScaleFactor = m_Scale * (start + 2*(k-0.5f)*( stop - start ));
	}

	if( m_Elapsed < m_InDur )
	{
		m_Alpha = m_Elapsed / m_InDur;
	}
	else if((m_Duration != -1.0f) && (m_Elapsed >= (m_Duration - m_OutDur)))
	{
		m_Alpha = (m_Duration - m_Elapsed) / m_OutDur;
	}

	m_Alpha = FilterClamp( 0.0f, 1.0f, m_Alpha );

	if( m_bFinishing )
	{
		EndThisEffect();
	}
}


void
Sphere_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	// Disable texturing
	gWorldFx.GetRenderer().SetTextureStageState(0,
								D3DTOP_DISABLE,
								D3DTOP_DISABLE,
								D3DTADDRESS_WRAP,
								D3DTADDRESS_WRAP,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DBLEND_SRCALPHA,
								D3DBLEND_ONE,
								false );

	for( int i = 0; i < m_Sides; i++ )
	{
		Subdivide( m_Verts[m_ConPts[i][0]], m_Verts[m_ConPts[i][1]], m_Verts[m_ConPts[i][2]],
					m_Subdivision, m_Base_color, m_Alpha );
	}

	// Enable texturing
	gWorldFx.SaveRenderingState();

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}


void
Sphere_Effect::Subdivide( vector_3 v1, vector_3 v2, vector_3 v3, const int depth,
						 const vector_3 color, const float alpha )
{
	const vector_3 diffuse_light( 0.8f, 0.8f, 0.8f );

	if( depth == 0 )
	{
		vector_3 nv = Normalize( CrossProduct( (v1-v2), (v2-v3) ) );

		vector_3	adj_CamDir( m_RotationMatrix * m_CamDir );

		float dot = InnerProduct( nv, adj_CamDir );

		vector_3 diffuse;

		if( dot > 0 )
		{
			diffuse = HadamardProduct( diffuse_light, m_Base_color );
			diffuse *= dot;
		}
		else
		{
			if( m_bCull )
			{
				return;
			}
			diffuse = HadamardProduct( 0.5*m_Base_color, m_Base_color );
			diffuse *= FABSF( dot );
		}

		DWORD vcolor	= MAKEDWORDCOLOR( diffuse );
		vcolor			= ((vcolor&0x00FFFFFF)|((BYTE( 255.0f*m_NodeAlpha*alpha ))<<24));


		m_DrawVerts[0].x		= v1.x * m_CurrentScaleFactor;
		m_DrawVerts[0].y		= v1.y * m_CurrentScaleFactor;
		m_DrawVerts[0].z		= v1.z * m_CurrentScaleFactor;
		m_DrawVerts[0].color	= vcolor;

		m_DrawVerts[1].x		= v2.x * m_CurrentScaleFactor;
		m_DrawVerts[1].y		= v2.y * m_CurrentScaleFactor;
		m_DrawVerts[1].z		= v2.z * m_CurrentScaleFactor;
		m_DrawVerts[1].color	= vcolor;

		m_DrawVerts[2].x		= v3.x * m_CurrentScaleFactor;
		m_DrawVerts[2].y		= v3.y * m_CurrentScaleFactor;
		m_DrawVerts[2].z		= v3.z * m_CurrentScaleFactor;
		m_DrawVerts[2].color	= vcolor;

		Rapi	&renderer = gWorldFx.GetRenderer();
		renderer.DrawPrimitive( D3DPT_TRIANGLELIST, m_DrawVerts, 3, SVERTEX, 0, 0 );

		if( m_bDrawLines )
		{
			vcolor	= MAKEDWORDCOLOR( m_Line_color );
			if( dot > 0 )
			{
				vcolor	= MAKEDWORDCOLOR( m_Line_color );
				vcolor	= ((vcolor&0x00FFFFFF)|((BYTE(alpha*m_NodeAlpha*255.0f))<<24));
			}
			else
			{
				vcolor	= MAKEDWORDCOLOR( 0.5*m_Line_color );
				vcolor	= ((vcolor&0x00FFFFFF)|((BYTE(alpha*m_NodeAlpha*255.0f))<<24));
			}

			m_DrawVerts[0].color	= vcolor;
			m_DrawVerts[1].color	= vcolor;
			m_DrawVerts[2].color	= vcolor;

			renderer.DrawPrimitive( D3DPT_LINESTRIP, m_DrawVerts, 3, SVERTEX, 0, 0 );
		}
		return;
	}

	vector_3	v12( Normalize( v1 + v2 ) );
	vector_3	v23( Normalize( v2 + v3 ) );
	vector_3	v31( Normalize( v3 + v1 ) );

	Subdivide( v1, v12, v31, depth - 1, color, alpha );
	Subdivide( v2, v23, v12, depth - 1, color, alpha );
	Subdivide( v3, v31, v23, depth - 1, color, alpha );
	Subdivide( v12, v23, v31, depth - 1, color, alpha );
}




// Flurry Effect ---

Flurry_Effect::Flurry_Effect( void )
{
	m_Radius				=	0;

	m_ParticleCount			=	0;
	m_Size					=	0;
	m_Alpha					=	0;
	m_In_dur				=	0;
	m_Out_dur				=	0;

	m_PhiSpeed				=	0;
	m_ThetaSpeed			=	0;
	m_SinAmp				=	0;
	m_SinSpeed				=	0;

	m_CurrentGrowthFactor	=	0;
	m_GrowthIndex			=	0;
}


void
Flurry_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",			0.0f, 0.0f, 1.0f);	// PARAM(1)
	params.SetVector3	( "grow_params",	1.0f, 1.0f, 1.0f);	// PARAM(2)

	params.SetUINT32	( "count",			50				);	// PARAM(3)

	params.SetFloat		( "radius",			1.0f			);	// PARAM(4)
	params.SetFloat		( "tin",			1.0f			);	// PARAM(5)
	params.SetFloat		( "tout",			1.0f			);	// PARAM(6)
	params.SetFloat		( "iphi",			1.0f			);	// PARAM(7)
	params.SetFloat		( "itheta",			1.0f			);	// PARAM(8)
	params.SetFloat		( "iamp",			1.0f			);	// PARAM(9)
	params.SetFloat		( "amplitude",		1.0f			);	// PARAM(10)
}


void
Flurry_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);
	m_Params.GetVector3	( PARAM(2),		m_GrowthFactors	);

	m_Params.GetUINT32	( PARAM(3),		m_ParticleCount	);
	m_ParticleCount	= min_t( (int)m_ParticleCount, MAX_PARTICLES );

	m_Params.GetFloat	( PARAM(4),		m_Radius		);
	m_Params.GetFloat	( PARAM(5),		m_In_dur		);
	m_Params.GetFloat	( PARAM(6),		m_Out_dur		);
	m_Params.GetFloat	( PARAM(7),		m_PhiSpeed		);
	m_Params.GetFloat	( PARAM(8),		m_ThetaSpeed	);
	m_Params.GetFloat	( PARAM(9),		m_SinSpeed		);
	m_Params.GetFloat	( PARAM(10),	m_SinAmp		);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_GrowthIndex = 0;
	m_CurrentGrowthFactor = m_GrowthFactors.x;

	m_Size = 0.125f * m_Scale;

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
	//

	m_Particles.resize( m_ParticleCount );

	// create a bunch of particles each with random directions and positions
	PColl::iterator it = m_Particles.begin(), it_end = m_Particles.end();
	for( ; it != it_end; ++it )
	{
		Particle &particle = *it;

		particle.alpha			= 1.0f;

		particle.sin_amplitude	= gWorldFx.GetRNG().Random( m_Radius * 0.10f ) * m_SinAmp;

		particle.theta			= gWorldFx.GetRNG().Random( PI2 );
		particle.phi			= gWorldFx.GetRNG().Random( PI2 );
		particle.sin			= gWorldFx.GetRNG().Random( PI2 );

		particle.itheta			= gWorldFx.GetRNG().Random( -2.0f, 2.0f ) * m_ThetaSpeed;
		particle.iphi			= gWorldFx.GetRNG().Random( -2.0f, 2.0f ) * m_PhiSpeed;
		particle.isin			= gWorldFx.GetRNG().Random( PI2 ) * m_SinSpeed;
	}
}


UINT32
Flurry_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 4 );
	}

	return m_TextureName;
}


FUBI_DECLARE_SELF_TRAITS( Flurry_Effect::Particle );

bool
Flurry_Effect::Particle::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "alpha",			alpha			);

	persist.Xfer( "sin_amplitude",	sin_amplitude	);

	persist.Xfer( "theta",			theta			);
	persist.Xfer( "phi",			phi				);
	persist.Xfer( "sin",			sin				);

	persist.Xfer( "itheta",			itheta			);
	persist.Xfer( "iphi",			iphi			);
	persist.Xfer( "isin",			isin			);

	persist.Xfer( "position",		position		);

	return true;
}


bool
Flurry_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferVector	( "m_Particles",			m_Particles				);
																			
	persist.Xfer		( "m_Base_color",			m_Base_color			);
	persist.Xfer		( "m_Radius",				m_Radius				);
																			
	persist.Xfer		( "m_ParticleCount",		m_ParticleCount			);
	persist.Xfer		( "m_Size",					m_Size					);
	persist.Xfer		( "m_Alpha",				m_Alpha					);
	persist.Xfer		( "m_In_dur",				m_In_dur				);
	persist.Xfer		( "m_Out_dur",				m_Out_dur				);
																			
	persist.Xfer		( "m_PhiSpeed",				m_PhiSpeed				);
	persist.Xfer		( "m_ThetaSpeed",			m_ThetaSpeed			);
	persist.Xfer		( "m_SinAmp",				m_SinAmp				);
	persist.Xfer		( "m_SinSpeed",				m_SinSpeed				);
						
	persist.Xfer		( "m_GrowthFactors",		m_GrowthFactors			);
	persist.Xfer		( "m_CurrentGrowthFactor",	m_CurrentGrowthFactor	);
	persist.Xfer		( "m_GrowthIndex",			m_GrowthIndex			);

	persist.Xfer		( "m_Color0",				m_Color0				);
	persist.Xfer		( "m_Color1",				m_Color1				);
	persist.Xfer		( "m_Color2",				m_Color2				);

	return true;
}


void
Flurry_Effect::UpdateDerived( void )
{
	const float	k = m_Elapsed / m_Duration;
	if( (k >= 0.5f) && !m_GrowthIndex )
	{
		++m_GrowthIndex;
	}

	const float	start = m_GrowthFactors.GetElementsPointer()[ m_GrowthIndex ];
	const float	stop  = m_GrowthFactors.GetElementsPointer()[ m_GrowthIndex+1 ];

	if( !m_GrowthIndex )
	{
		m_CurrentGrowthFactor = m_Scale * (start + 2*k*( stop - start ));
	}
	else
	{
		m_CurrentGrowthFactor = m_Scale * (start + 2*(k-0.5f)*( stop - start ));
	}

	if( m_Duration != -1.0f )
	{
		if( m_bFinishing || (m_Elapsed >= (m_Duration-m_Out_dur)) )
		{
			m_Alpha = (m_Duration - m_Elapsed)/m_Out_dur;
			m_bFinishing = true;
		}
		else
		{
			m_Alpha = (m_Alpha<=1.0f)?((m_Elapsed / m_In_dur) + 0.00001f):1.0f;
		}
	}

	m_Alpha = FilterClamp( 0.0f, 1.0f, m_Alpha );

	// setup the vert iterators
	PColl::iterator it = m_Particles.begin(), it_end = m_Particles.end();
	
	// set the verts
	for(; it != it_end; ++it )
	{
		Particle &particle = *it;

		// advance the angles
		particle.theta	+= particle.itheta * m_FrameSync;
		particle.phi	+= particle.iphi * m_FrameSync;
		particle.sin	+= particle.isin * m_FrameSync;

		// determine the particle position
		const float	r = (m_Radius + particle.sin_amplitude * SINF( particle.sin ))*m_CurrentGrowthFactor;

		float thetasin, thetacos;
		SINCOSF( particle.theta, thetasin, thetacos );

		float phisin, phicos;
		SINCOSF( particle.phi, phisin, phicos );

		particle.position.x = r * phisin * thetacos;
		particle.position.y = r * phisin * thetasin;
		particle.position.z = r * phicos;
	}

	// determine the colors
	m_Color0 = MAKEDWORDCOLOR( m_Base_color );
	m_Color0 = (m_Color0 & 0x00FFFFFF) | ((BYTE(m_Alpha * m_NodeAlpha * 255.0f))<<24);

	if( !m_bUseLoadedTexture )
	{
		m_Color1 = (0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*160.0f))<<24);
		m_Color2 = (0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24);
	}

	m_bFinished = ( (m_bFinished) | (m_Alpha <= 0.0f) );

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
Flurry_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	// do a little setup calculations
	const vector_3 w_xaxis( 2.0f * m_Size * m_Vx );
	const vector_3 h_yaxis( 2.0f * m_Size * m_Vy );

	const float temp = 2.0f / 3.41421f;
	const float temp2= 2.82842712f;	// 2*root2

	const vector_3 vtemp0 ( -w_xaxis + h_yaxis );
	const vector_3 vtemp1 ( -w_xaxis - temp2 * h_yaxis );
	const vector_3 vtemp2 ( temp2 * w_xaxis + h_yaxis );

	const vector_3 v0 ( temp * vtemp0 );
	const vector_3 v1 ( temp * vtemp1 );
	const vector_3 v2 ( temp * vtemp2 );

	Rapi & renderer = gWorldFx.GetRenderer();

	unsigned int buffer_offset = 0;
	sVertex* pVertex = NULL;
	
	const UINT32 size = m_Particles.size() * 3;	// n particles * 3 points
	const UINT32 lock_size = !m_bUseLoadedTexture?3*size:size;

	if( renderer.LockBufferedVertices( lock_size, buffer_offset, pVertex ) )
	{
		renderer.SetTexture( 0, m_TextureName );
		renderer.SetTexture( 1, NULL );

		PColl::iterator it = m_Particles.begin(), it_end = m_Particles.end();

		for( UINT32 i = 0; it != it_end; ++it, i += 3 )
		{
			const vector_3 & pos = (*it).position;

			sVertex & vert0 = pVertex[ i + 0 ];
			vert0.x			= pos.x + v0.x;
			vert0.y			= pos.y + v0.y;
			vert0.z			= pos.z + v0.z;
			vert0.color		= m_Color0;
			vert0.uv.u		= 0.0f;
			vert0.uv.v		= 1.0f;

			sVertex & vert1 = pVertex[ i + 1 ];
			vert1.x			= pos.x + v1.x;
			vert1.y			= pos.y + v1.y;
			vert1.z			= pos.z + v1.z;
			vert1.color		= m_Color0;
			vert1.uv.u		= 0.0f;
			vert1.uv.v		= -1.0f;

			sVertex & vert2 = pVertex[ i + 2 ];
			vert2.x			= pos.x + v2.x;
			vert2.y			= pos.y + v2.y;
			vert2.z			= pos.z + v2.z;
			vert2.color		= m_Color0;
			vert2.uv.u		= 2.0f;
			vert2.uv.v		= 1.0f;
		}

		if( !m_bUseLoadedTexture )
		{
			const vector_3 v0a( 0.7f * temp * vtemp0 );
			const vector_3 v1a( 0.7f * temp * vtemp1 );
			const vector_3 v2a( 0.7f * temp * vtemp2 );

			const vector_3 v0b( 0.57143f * v0a );
			const vector_3 v1b( 0.57143f * v1a );
			const vector_3 v2b( 0.57143f * v2a );

			it = m_Particles.begin();

			for( UINT32 i = size, j = 2*size; it != it_end; ++it, i += 3, j += 3 )
			{
				const vector_3 & pos = (*it).position;

				sVertex & vert0a= pVertex[ i + 0 ];	sVertex & vert0b= pVertex[ j + 0 ];
				vert0a.x		= pos.x + v0a.x;	vert0b.x		= pos.x + v0b.x;
				vert0a.y		= pos.y + v0a.y;	vert0b.y		= pos.y + v0b.y;
				vert0a.z		= pos.z + v0a.z;	vert0b.z		= pos.z + v0b.z;
				vert0a.color	= m_Color1;			vert0b.color	= m_Color2;
				vert0a.uv.u		= 0.0f;				vert0b.uv.u		= 0.0f;
				vert0a.uv.v		= 1.0f;				vert0b.uv.v		= 1.0f;

				sVertex & vert1a= pVertex[ i + 1 ];	sVertex & vert1b= pVertex[ j + 1 ];
				vert1a.x		= pos.x + v1a.x;	vert1b.x		= pos.x + v1b.x;
				vert1a.y		= pos.y + v1a.y;	vert1b.y		= pos.y + v1b.y;
				vert1a.z		= pos.z + v1a.z;	vert1b.z		= pos.z + v1b.z;
				vert1a.color	= m_Color1;			vert1b.color	= m_Color2;
				vert1a.uv.u		= 0.0f;				vert1b.uv.u		= 0.0f;
				vert1a.uv.v		= -1.0f;			vert1b.uv.v		= -1.0f;

				sVertex & vert2a= pVertex[ i + 2 ];	sVertex & vert2b= pVertex[ j + 2 ];
				vert2a.x		= pos.x + v2a.x;	vert2b.x		= pos.x + v2b.x;
				vert2a.y		= pos.y + v2a.y;	vert2b.y		= pos.y + v2b.y;
				vert2a.z		= pos.z + v2a.z;	vert2b.z		= pos.z + v2b.z;
				vert2a.color	= m_Color1;			vert2b.color	= m_Color2;
				vert2a.uv.u		= 2.0f;				vert2b.uv.u		= 2.0f;
				vert2a.uv.v		= 1.0f;				vert2b.uv.v		= 1.0f;
			}
		}

		renderer.UnlockBufferedVertices();
		renderer.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, buffer_offset, lock_size );
	}
}


float
Flurry_Effect::GetBoundingRadius( void )
{
	return	m_Radius * m_ScaleFactor * m_Size;
}




// Charge Effect ---

Charge_Effect::Charge_Effect( void )
	: m_CenterIntensity( 0 )
	, m_Alpha( 1.0f )
	, m_OutElapsed( 0 )
	, m_ActiveCount( 0 )
{
}


void
Charge_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		0.0f, 0.0f, 1.0f); // PARAM(1)

	params.SetUINT32	( "count",		16				); // PARAM(2)

	params.SetFloat		( "radius",		0.5f			); // PARAM(3)
	params.SetFloat		( "tout",		1.75f			); // PARAM(4)
	params.SetFloat		( "speed0",		1.0f			); // PARAM(5)
	params.SetFloat		( "centersize",	0.75f			); // PARAM(6)
	params.SetFloat		( "ialpha",		4.0f			); // PARAM(7)
}


void
Charge_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);

	m_Params.GetUINT32	( PARAM(2),		m_ParticleCount	);
	m_ParticleCount	= min_t( (int)m_ParticleCount, MAX_PARTICLES );

	m_Params.GetFloat	( PARAM(3),		m_Radius		);
	m_Radius *= m_ScaleFactor;

	m_Params.GetFloat	( PARAM(4),		m_Out_dur		);
	m_Params.GetFloat	( PARAM(5),		m_Speed			);
	m_Params.GetFloat	( PARAM(6),		m_CenterSize	);
	m_Params.GetFloat	( PARAM(7),		m_IAlpha		);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_Particles.resize( m_ParticleCount );

	m_Size = 0.1f * m_Scale;

	for( UINT32 i = 0; i < m_ParticleCount; ++i )
	{
		if( m_Duration == -1 )
		{
			m_Particles[ i ].delay	=	2.0f * i/m_ParticleCount;
		}
		else
		{
			m_Particles[ i ].delay	=	m_Duration * i/m_ParticleCount;
		}

		m_Particles[ i ].active		=	true;
		m_Particles[ i ].alpha		=	0.0f;
		m_Particles[ i ].ialpha		=	m_IAlpha;

		float	Phi_cos, Phi_sin;
		SINCOSF( gWorldFx.GetRNG().Random( PI2 ), Phi_sin, Phi_cos );
		float	Theta_cos, Theta_sin;
		SINCOSF( gWorldFx.GetRNG().Random( PI2 ), Theta_sin, Theta_cos );

		vector_3	Direction(	Phi_sin * Theta_cos,
								Phi_sin * Theta_sin,
								Phi_cos );

		Direction = Normalize( Direction );

		m_Particles[ i ].position	=	Direction * m_Radius;
		m_Particles[ i ].velocity	=	vector_3();
		m_Particles[ i ].accel		=	Direction * gWorldFx.GetRNG().Random( -1.25f, -3.0f ) * m_Speed;
	}
}


UINT32
Charge_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	return m_TextureName;
}


FUBI_DECLARE_SELF_TRAITS( Charge_Effect::Particle );

bool
Charge_Effect::Particle::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "active",		active	);

	persist.Xfer( "delay",		delay	);

	persist.Xfer( "alpha",		alpha	);
	persist.Xfer( "ialpha",		ialpha	);

	persist.Xfer( "position",	position);
	persist.Xfer( "velocity",	velocity);
	persist.Xfer( "accel",		accel	);

	return true;
}


bool
Charge_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferVector	( "m_Particles",		m_Particles			);

	persist.Xfer		( "m_CenterIntensity",	m_CenterIntensity	);
	persist.Xfer		( "m_ActiveCount",		m_ActiveCount		);
						
	persist.Xfer		( "m_Base_color",		m_Base_color		);
	persist.Xfer		( "m_Radius",			m_Radius			);
	persist.Xfer		( "m_ParticleCount",	m_ParticleCount		);
	persist.Xfer		( "m_Size",				m_Size				);
	persist.Xfer		( "m_Speed",			m_Speed				);
	persist.Xfer		( "m_CenterSize",		m_CenterSize		);
	persist.Xfer		( "m_Alpha",			m_Alpha				);
	persist.Xfer		( "m_IAlpha",			m_IAlpha			);
	persist.Xfer		( "m_Out_dur",			m_Out_dur			);
	persist.Xfer		( "m_OutElapsed",		m_OutElapsed		);

	return true;
}


void
Charge_Effect::UpdateDerived( void )
{
	m_ActiveCount = 0;

	for( UINT32 i = 0; i < m_ParticleCount; ++i )
	{
		if( !m_Particles[ i ].active )
		{
			continue;
		}

		++m_ActiveCount;

		if( m_Particles[ i ].delay == 0 )
		{
			if( m_Particles[ i ].alpha == 1.0f )
			{
				m_Particles[ i ].velocity =	m_Particles[ i ].velocity +
											m_Particles[ i ].accel * m_FrameSync;
				m_Particles[ i ].position =	m_Particles[ i ].position +
											m_Particles[ i ].velocity * m_FrameSync +
											m_Particles[ i ].accel * m_FrameSync * m_FrameSync * 0.5f;

				float dist = Length( m_Particles[ i ].position );

				if( (dist > m_Radius) || (dist < m_Radius*0.10f) )
				{
					if( m_bFinishing )
					{
						m_Particles[ i ].active	=	false;
					}
					else
					{
						m_Particles[ i ].active	=	true;

						m_Particles[ i ].alpha	=	0.0f;
						m_Particles[ i ].ialpha	=	m_IAlpha;

						m_CenterIntensity = FilterClamp( 0.0f, 1.0f, m_Elapsed/m_Duration );

						float	Phi_cos, Phi_sin;
						SINCOSF( gWorldFx.GetRNG().Random( PI2 ), Phi_sin, Phi_cos );
						float	Theta_cos, Theta_sin;
						SINCOSF( gWorldFx.GetRNG().Random( PI2 ), Theta_sin, Theta_cos );

						vector_3	Direction(	Phi_sin * Theta_cos,
												Phi_sin * Theta_sin,
												Phi_cos );

						Direction = Normalize( Direction );

						m_Particles[ i ].position	=	Direction * m_Radius;
						m_Particles[ i ].velocity	=	vector_3();
						m_Particles[ i ].accel		=	Direction * gWorldFx.GetRNG().Random( -1.25f, -3.0f ) * m_Speed;
					}
				}
			}
			else
			{
				m_Particles[ i ].alpha += m_Particles[ i ].ialpha * m_FrameSync;
				if( m_Particles[ i ].alpha > 1.0f )
				{
					m_Particles[ i ].alpha = 1.0f;
				}
			}
		}
		else
		{
			m_Particles[ i ].delay -= m_FrameSync;
			if( m_Particles[ i ].delay < 0 )
			{
				m_Particles[ i ].delay = 0;
			}
		}
	}

	if( m_Duration != -1.0f )
	{
		if( m_ActiveCount == 0 )
		{
			m_OutElapsed += m_FrameSync;
			m_Alpha = FilterClamp( 0.0f, 1.0f, ( m_Out_dur - m_OutElapsed ) / m_Out_dur );

			if( m_Alpha <= 0.0f )
			{
				EndThisEffect();
			}
		}
	}
#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif

}


void
Charge_Effect::DrawSelf( void )
{
	if( m_bFinished || !gWorldFx.IsNodeVisible( m_Pos.node) )
	{
		return;
	}

	Rapi&	renderer = gWorldFx.GetRenderer();

	vector_3 vc0 = m_Size * (-m_Vx +m_Vy);
	vector_3 vc1 = m_Size * (-m_Vx -m_Vy);
	vector_3 vc2 = m_Size * ( m_Vx +m_Vy);
	vector_3 vc3 = m_Size * ( m_Vx -m_Vy);

	vector_3 vc00, vc10, vc20, vc30, vc01, vc11, vc21, vc31;

	if( !m_bUseLoadedTexture )
	{
		vc00 = 0.7f * vc0;	 vc01 = 0.45f * vc0;
		vc10 = 0.7f * vc1;	 vc11 = 0.45f * vc1;
		vc20 = 0.7f * vc2;	 vc21 = 0.45f * vc2;
		vc30 = 0.7f * vc3;	 vc31 = 0.45f * vc3;
	}

	for( UINT32 i = 0; i < m_ParticleCount; ++i )
	{
		if( !m_Particles[ i ].active )
		{
			continue;
		}

		vector_3 vp0 = vc0 + m_Particles[ i ].position;
		vector_3 vp1 = vc1 + m_Particles[ i ].position;
		vector_3 vp2 = vc2 + m_Particles[ i ].position;
		vector_3 vp3 = vc3 + m_Particles[ i ].position;

		float	Alpha = m_Particles[ i ].alpha;

		DWORD color	= MAKEDWORDCOLOR( m_Base_color );
		color		= ((color&0x00FFFFFF)|((BYTE(Alpha*m_NodeAlpha*255.0f))<<24));

		m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
		m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
		m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
		m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
		m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
		m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

		m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
		m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
		m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
		m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
		m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
		m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
								&m_TextureName, 1 );

		if( !m_bUseLoadedTexture )
		{
			color		= ((0x00A0A0A0)|((BYTE(Alpha*m_NodeAlpha*160.0f))<<24));
			vector_3 vp0 = vc00 + m_Particles[ i ].position;
			vector_3 vp1 = vc10 + m_Particles[ i ].position;
			vector_3 vp2 = vc20 + m_Particles[ i ].position;
			vector_3 vp3 = vc30 + m_Particles[ i ].position;

			m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
			m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
			m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
			m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
			m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
			m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

			m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
			m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
			m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
			m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
			m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
			m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
									&m_TextureName, 1 );
		}

		if( !m_bUseLoadedTexture )
		{
			color		= ((0x00A0A0A0)|((BYTE(Alpha*m_NodeAlpha*255.0f))<<24));
			vector_3 vp0 = vc01 + m_Particles[ i ].position;
			vector_3 vp1 = vc11 + m_Particles[ i ].position;
			vector_3 vp2 = vc21 + m_Particles[ i ].position;
			vector_3 vp3 = vc31 + m_Particles[ i ].position;

			m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
			m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
			m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
			m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
			m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
			m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

			m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
			m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
			m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
			m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
			m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
			m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
									&m_TextureName, 1 );
		}
	}

	//
	//	Now draw the center concentration if applicable
	//
	const float	CenterSize = m_Radius * m_CenterIntensity * m_CenterSize;

	vector_3 vp0 = (-m_Vx +m_Vy) * CenterSize;
	vector_3 vp1 = (-m_Vx -m_Vy) * CenterSize;
	vector_3 vp2 = ( m_Vx +m_Vy) * CenterSize;
	vector_3 vp3 = ( m_Vx -m_Vy) * CenterSize;

	DWORD color	= MAKEDWORDCOLOR( m_Base_color );
	color = ((color&0x00FFFFFF)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

	m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
	m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
	m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
	m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
	m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
	m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

	m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
	m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
	m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
	m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
	m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
	m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
							&m_TextureName, 1 );

	if( !m_bUseLoadedTexture )
	{
		vp0 *= 0.80f;	vp1 *= 0.80f;	vp2 *= 0.80f;	vp3 *= 0.80f;
		color = ((0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*160.0f))<<24));

		m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
		m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
		m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
		m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
		m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
		m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

		m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
		m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
		m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
		m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
		m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
		m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
								&m_TextureName, 1 );

		vp0 *= 0.57f;	vp1 *= 0.57f;	vp2 *= 0.57f;	vp3 *= 0.57f;
		color = ((0x00A0A0A0)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

		m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
		m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
		m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
		m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
		m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
		m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

		m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
		m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
		m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
		m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
		m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
		m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX,
								&m_TextureName, 1 );
	}
}


float
Charge_Effect::GetBoundingRadius( void )
{
	return	m_Radius * m_Scale;
}




// Sparkles Effect ---

Sparkles_Effect::Sparkles_Effect( void )
{
	m_Radius		=	0;
	m_Size			=	0;
	m_ParticleCount	=	0;

	m_PSize			=	0;
	m_YVelocity		=	0;

	m_bDone			=	false;
}


void
Sparkles_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",	0.0f, 1.0f, 0.0f);	// PARAM(1)

	params.SetUINT32	( "count",	30				); // PARAM(2)

	params.SetFloat		( "radius",	1.0f			); // PARAM(3)
	params.SetFloat		( "psize",	1.0f			); // PARAM(4)
	params.SetFloat		( "yvel",	0.0f			); // PARAM(5)
}


void
Sparkles_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),	m_Base_color );

	m_Params.GetUINT32	( PARAM(2),	m_ParticleCount );
	m_ParticleCount	= min_t( (int)m_ParticleCount, MAX_PARTICLES );

	m_Params.GetFloat	( PARAM(3),	m_Radius );
	m_Params.GetFloat	( PARAM(4),	m_PSize );
	m_Params.GetFloat	( PARAM(5),	m_YVelocity );

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_Size = 0.15f * m_ScaleFactor;
}


UINT32
Sparkles_Effect::InitializeTexture( void )
{
	if( !m_bUseLoadedTexture )
	{
		m_TextureName = gWorldFx.GetGlobalTexture( 0 );
	}

	return m_TextureName;
}



FUBI_DECLARE_SELF_TRAITS( Sparkles_Effect::Particle );

bool
Sparkles_Effect::Particle::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "position",	position );
	persist.Xfer( "active",		active );
	persist.Xfer( "fading_in",	fading_in );
	persist.Xfer( "alpha",		alpha );
	persist.Xfer( "ialpha",		ialpha );
	persist.Xfer( "delay",		delay );

	return true;
}


FUBI_DECLARE_SELF_TRAITS( Sparkles_Effect::Particle_list );

bool
Sparkles_Effect::Particle_list::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "guid",		guid );
	persist.XferList( "particles",	particles );

	return true;
}


bool
Sparkles_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferList( "m_Particles",		m_Particles );
	persist.XferList( "m_InsertList",		m_InsertList );

	persist.Xfer	( "m_Base_color",		m_Base_color );
	persist.Xfer	( "m_Radius",			m_Radius );
	persist.Xfer	( "m_Size",				m_Size );
	persist.Xfer	( "m_ParticleCount",	m_ParticleCount );

	persist.Xfer	( "m_PSize",			m_PSize );
	persist.Xfer	( "m_YVelocity",		m_YVelocity );

	persist.Xfer	( "m_bDone",			m_bDone );

	return true;
}


void
Sparkles_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		m_Targets->GetPlacement( &m_Pos, NULL, m_OrientMode );

		Particle_list	plist;
		plist.guid		= m_Pos.node;
		plist.particles.resize( m_ParticleCount );

		PColl::iterator it = plist.particles.begin();
		for(UINT32 i = 0; i < m_ParticleCount; ++i, ++it )
		{
			if( m_Duration == -1 )
			{
				CreateParticle( *it, 2.0f * i/m_ParticleCount );
			}
			else
			{
				CreateParticle( *it, m_Duration * i/m_ParticleCount );
			}
		}
		m_Particles.push_back( plist );
	}

	m_bDone = true;

	// Update the alpha of each particle in each particle list
	PLColl::iterator iList = m_Particles.begin(), iEnd = m_Particles.end();

	for(; iList != iEnd; ++iList )
	{
		PColl::iterator iParticle = (*iList).particles.begin(), iListEnd  = (*iList).particles.end();

		for(; iParticle != iListEnd; ++iParticle )
		{
			if( !(*iParticle).active )
			{
				continue;
			}

			m_bDone = false;

			(*iParticle).position.pos.y += m_YVelocity * m_FrameSync;


			// Check to see if we should start fading pre-created particle in
			if( (*iParticle).delay == 0 )
			{
				// Fade particle in
				if( (*iParticle).fading_in )
				{
					(*iParticle).alpha += (*iParticle).ialpha * m_FrameSync;
					(*iParticle).alpha = FilterClamp( 0.0f, 1.0f, (*iParticle).alpha );

					if( (*iParticle).alpha == 0 )
					{
						(*iParticle).active = !m_bFinishing;
						continue;
					}

					if( (*iParticle).alpha == 1.0f )
					{
						(*iParticle).fading_in = false;
					}
				}
				else
				{
					// Fade particle out
					(*iParticle).alpha -= (*iParticle).ialpha * m_FrameSync;
					(*iParticle).alpha = FilterClamp( 0.0f, 1.0f, (*iParticle).alpha );

					// Spawn
					if( ((*iParticle).alpha == 0) && (*iParticle).active )
					{
						if( m_bFinishing )
						{
							(*iParticle).active = false;
						}
						else
						{
							Particle particle;
							CreateParticle( particle );

							// Replace existing particle
							if( m_Pos.node == (*iList).guid )
							{
								*iParticle = particle;
							}
							else
							{
								bool inserted = false;

								// Transfer particle to another existing list
								PLColl::iterator it = m_Particles.begin(), it_end = m_Particles.end();
								while( it != it_end )
								{
									if( (*it).guid == particle.position.node )
									{
										iParticle = (*iList).particles.erase( iParticle );
										it_end = m_Particles.end();

										(*it).particles.push_back( particle );
										inserted = true;
										break;
									}
									else
									{
										++it;
									}
								}

								// Create new list and place particle in it
								if( inserted == false )
								{
									bool list_exists = false;
									for( PLColl::iterator iListTemp = m_InsertList.begin();
										iListTemp != m_InsertList.end(); ++iListTemp )
									{
										if( (*iListTemp).guid == particle.position.node )
										{
											iParticle = (*iList).particles.erase( iParticle );

											(*iListTemp).particles.push_back( particle );
											list_exists = true;
											break;
										}
									}

									if( list_exists == false )
									{
										iParticle = (*iList).particles.erase( iParticle );

										Particle_list new_list;
										new_list.guid = particle.position.node;
										new_list.particles.push_back( particle );
										m_InsertList.push_back( new_list );
									}
								}
							}
						}
					}
				}
			}
			else
			{
				(*iParticle).delay -= m_FrameSync;
				if( (*iParticle).delay < 0 )
				{
					(*iParticle).delay = 0;
				}
			}
		}
	}

	PLColl::iterator iInsertList = m_InsertList.begin(), iIEnd = m_InsertList.end();
	for(; iInsertList != iIEnd; ++iInsertList )
	{
		m_Particles.push_back( *iInsertList );
	}

	m_InsertList.clear();

	if( m_bDone )
	{
		EndThisEffect();
	}

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
Sparkles_Effect::DrawSelf( void )
{
	Rapi&	renderer = gWorldFx.GetRenderer();

	PLColl::iterator iList = m_Particles.begin(), iEnd = m_Particles.end();
	for( ; iList != iEnd; ++iList )
	{
		// Make sure this is in a valid node otherwise abort this effect
		if( !gWorldFx.IsNodeVisible( (*iList).guid ) )
		{
			continue;
		}

		SetDrawMatrix( (*iList).guid );

		const float size_scalar = m_Size * m_PSize;

		vector_3 vc0 = size_scalar * (-m_Vx +m_Vy);
		vector_3 vc1 = size_scalar * (-m_Vx -m_Vy);
		vector_3 vc2 = size_scalar * ( m_Vx +m_Vy);
		vector_3 vc3 = size_scalar * ( m_Vx -m_Vy);

		vector_3	vc00, vc10, vc20, vc30, vc01, vc11, vc21, vc31;

		if( !m_bUseLoadedTexture )
		{
			vc00 = 0.7f * vc0;	vc01 = 0.4f * vc0;
			vc10 = 0.7f * vc1;	vc11 = 0.4f * vc1;
			vc20 = 0.7f * vc2;	vc21 = 0.4f * vc2;
			vc30 = 0.7f * vc3;	vc31 = 0.4f * vc3;
		}

		PColl::iterator iParticle = (*iList).particles.begin(), ipEnd = (*iList).particles.end();
		for(; iParticle != ipEnd; ++iParticle )
		{
			vector_3	position_offset( (*iParticle).position.pos );

			vector_3 vp0 = vc0 + position_offset;
			vector_3 vp1 = vc1 + position_offset;
			vector_3 vp2 = vc2 + position_offset;
			vector_3 vp3 = vc3 + position_offset;

			float	Alpha = (*iParticle).alpha*m_NodeAlpha;

			DWORD color	= MAKEDWORDCOLOR( m_Base_color );
			color		= ((color&0x00FFFFFF)|((BYTE(Alpha*255.0f))<<24));

			m_DrawVerts[0].x		= vp0.x;		m_DrawVerts[1].x		= vp2.x;
			m_DrawVerts[0].y		= vp0.y;		m_DrawVerts[1].y		= vp2.y;
			m_DrawVerts[0].z		= vp0.z;		m_DrawVerts[1].z		= vp2.z;
			m_DrawVerts[0].color	= color;		m_DrawVerts[1].color	= color;
			m_DrawVerts[0].uv.u		= 0.0f;			m_DrawVerts[1].uv.u		= 1.0f;
			m_DrawVerts[0].uv.v		= 1.0f;			m_DrawVerts[1].uv.v		= 1.0f;

			m_DrawVerts[2].x		= vp1.x;		m_DrawVerts[3].x		= vp3.x;
			m_DrawVerts[2].y		= vp1.y;		m_DrawVerts[3].y		= vp3.y;
			m_DrawVerts[2].z		= vp1.z;		m_DrawVerts[3].z		= vp3.z;
			m_DrawVerts[2].color	= color;		m_DrawVerts[3].color	= color;
			m_DrawVerts[2].uv.u		= 0.0f;			m_DrawVerts[3].uv.u		= 1.0f;
			m_DrawVerts[2].uv.v		= 0.0f;			m_DrawVerts[3].uv.v		= 0.0f;

			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX, &m_TextureName, 1 );

			if( !m_bUseLoadedTexture )
			{
				color		= ((0x00A0A0A0)|((BYTE(Alpha*160.0f))<<24));
				vector_3 vp0 = vc00 + position_offset;
				vector_3 vp1 = vc10 + position_offset;
				vector_3 vp2 = vc20 + position_offset;
				vector_3 vp3 = vc30 + position_offset;

				m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
				m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
				m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
				m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
				m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
				m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

				m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
				m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
				m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
				m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
				m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
				m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

				renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX, &m_TextureName, 1 );
			}

			if( !m_bUseLoadedTexture )
			{
				color		= ((0x00A0A0A0)|((BYTE(Alpha*255.0f))<<24));
				vector_3 vp0 = vc01 + position_offset;
				vector_3 vp1 = vc11 + position_offset;
				vector_3 vp2 = vc21 + position_offset;
				vector_3 vp3 = vc31 + position_offset;

				m_DrawVerts[0].x		= vp0.x;	m_DrawVerts[1].x		= vp2.x;
				m_DrawVerts[0].y		= vp0.y;	m_DrawVerts[1].y		= vp2.y;
				m_DrawVerts[0].z		= vp0.z;	m_DrawVerts[1].z		= vp2.z;
				m_DrawVerts[0].color	= color;	m_DrawVerts[1].color	= color;
				m_DrawVerts[0].uv.u		= 0.0f;		m_DrawVerts[1].uv.u		= 1.0f;
				m_DrawVerts[0].uv.v		= 1.0f;		m_DrawVerts[1].uv.v		= 1.0f;

				m_DrawVerts[2].x		= vp1.x;	m_DrawVerts[3].x		= vp3.x;
				m_DrawVerts[2].y		= vp1.y;	m_DrawVerts[3].y		= vp3.y;
				m_DrawVerts[2].z		= vp1.z;	m_DrawVerts[3].z		= vp3.z;
				m_DrawVerts[2].color	= color;	m_DrawVerts[3].color	= color;
				m_DrawVerts[2].uv.u		= 0.0f;		m_DrawVerts[3].uv.u		= 1.0f;
				m_DrawVerts[2].uv.v		= 0.0f;		m_DrawVerts[3].uv.v		= 0.0f;

				renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_DrawVerts, 4, SVERTEX, &m_TextureName, 1 );
			}
		}
		gWorldFx.SaveRenderingState();
	}
}


void
Sparkles_Effect::CreateParticle( Sparkles_Effect::Particle &p, float delay )
{
	const float r		=	m_Radius * RandomFloat();
	float phiSin, phiCos, thetaSin, thetaCos;
	SINCOSF( gWorldFx.GetRNG().Random( PI2 ), phiSin, phiCos );
	SINCOSF( gWorldFx.GetRNG().Random( PI2 ), thetaSin, thetaCos );

	p.position			=	m_Pos;
	p.position.pos.x	+=	r * phiSin * thetaCos;
	p.position.pos.y	+=	r * phiSin * thetaSin;
	p.position.pos.z	+=	r * phiCos;
	p.active			=	!m_bFinishing;
	p.fading_in			=	true;
	p.alpha				=	0.0f;
	p.ialpha			=	gWorldFx.GetRNG().Random( 2.0f, 6.0f );
	p.delay				=	delay;
}


float
Sparkles_Effect::GetBoundingRadius( void )
{
	return m_Radius * m_Scale;
}







// DiscPile ---

DiscPile_Effect::DiscPile_Effect( void )
{
	m_bHeightIndexed		=	false;
	m_RelativeGroundHeight	=	0;
	m_Radius				=	0;
	m_DiscRadius			=	0;
	m_ThicknessFactor		=	0;
	m_Size					=	0;
	m_Sides					=	0;
	m_DiscCount				=	0;
}


void
DiscPile_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		1.0f, 1.0f, 0.15f	); // PARAM(1)
	params.SetVector3	( "minvel",		-0.75f, 1.0f, -0.75f); // PARAM(2)
	params.SetVector3	( "maxvel",		0.75f, 5.0f, 0.75f	); // PARAM(3)

	params.SetUINT32	( "sides",		10					); // PARAM(4)
	params.SetUINT32	( "count",		100					); // PARAM(5)

	params.SetFloat		( "radius",		0.45f				); // PARAM(6)
	params.SetFloat		( "discradius",	0.025f				); // PARAM(7)
	params.SetFloat		( "thickness",	0.1125f				); // PARAM(8)
}


void
DiscPile_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color		);
	m_Params.GetVector3	( PARAM(2),		m_MinVel			);
	m_Params.GetVector3	( PARAM(3),		m_MaxVel			);

	m_Params.GetUINT32	( PARAM(4),		m_Sides				);
	m_Sides = min_t( m_Sides, (UINT32)MAX_SIDES );
	m_Params.GetUINT32	( PARAM(5),		m_DiscCount			);
	m_DiscCount	= min_t( m_DiscCount, (UINT32)MAX_DISCS );

	m_Params.GetFloat	( PARAM(6),		m_Radius			);
	m_Radius *= m_ScaleFactor;

	m_Params.GetFloat	( PARAM(7),		m_DiscRadius		);
	m_Params.GetFloat	( PARAM(8),		m_ThicknessFactor	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	ValidateOrder( m_MinVel.x, m_MaxVel.x );
	ValidateOrder( m_MinVel.y, m_MaxVel.y );
	ValidateOrder( m_MinVel.z, m_MaxVel.z );

	// ---
	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

	m_Size = 0.15f * m_ScaleFactor;

	memset( m_SideVerts, 0, sizeof( sVertex )*(2*m_Sides) );
	memset( m_TopVerts, 0, sizeof( sVertex )*(1+(2*m_Sides)) );
	memset( m_BottomVerts, 0, sizeof( sVertex )*(1+(2*m_Sides)) );

	m_bHeightIndexed = false;

	DWORD color	= MAKEDWORDCOLOR( m_Base_color );

	float theta		= 0;
	float thetasin, thetacos;
	float iTheta	= 2*RealPi/m_Sides;
	float r = m_DiscRadius * m_ScaleFactor;
	float h = m_ThicknessFactor * r;


	// Generate top & bottom
	m_TopVerts[ 0 ].x			=	0.0f;
	m_TopVerts[ 0 ].y			=	h;
	m_TopVerts[ 0 ].z			=	0.0f;
	m_TopVerts[ 0 ].color		=	color;

	m_BottomVerts[ 0 ].x		=	0.0f;
	m_BottomVerts[ 0 ].y		=	-h;
	m_BottomVerts[ 0 ].z		=	0.0f;
	m_BottomVerts[ 0 ].color	=	color;

	for( UINT32 i = 1; i <= (m_Sides+1); ++i, theta += iTheta )
	{
		SINCOSF( theta, thetasin, thetacos );

		m_TopVerts[ i ].x		=	r * thetacos;
		m_TopVerts[ i ].y		=	h;
		m_TopVerts[ i ].z		=	r * thetasin;
		m_TopVerts[ i ].color	=	color;

		m_TopNorms[ i ] = Normalize( vector_3( m_TopVerts[ i ].x, 1.0f, m_TopVerts[ i ].z )) ;

		m_BottomVerts[ i ].x	=	r * thetacos;
		m_BottomVerts[ i ].y	=	-h;
		m_BottomVerts[ i ].z	=	r * thetasin;
		m_BottomVerts[ i ].color=	color;

		m_BottomNorms[ i ] = Normalize( vector_3( m_BottomVerts[ i ].x, -1.0f, m_BottomVerts[ i ].z ));

	}

	// Generate sides
	theta = 0;
	for( i = 0; i <= (m_Sides+1)*2; i+=2, theta += iTheta )
	{
		SINCOSF( theta, thetasin, thetacos );

		m_SideVerts[ 0+i ].x		=	r * thetacos;
		m_SideVerts[ 0+i ].y		=	h;
		m_SideVerts[ 0+i ].z		=	r * thetasin;
		m_SideVerts[ 0+i ].color	=	color;

		m_SideVerts[ 1+i ].x		=	r * thetacos;
		m_SideVerts[ 1+i ].y		=	-h;
		m_SideVerts[ 1+i ].z		=	r * thetasin;
		m_SideVerts[ 1+i ].color	=	color;
	}

	theta = 0;
	for( i = 0; i <= (m_Sides+1)*2; ++i, theta += iTheta )
	{
		vector_3	v1( m_SideVerts[ 0+i ].x, m_SideVerts[ 0+i ].y, m_SideVerts[ 0+i ].z );
		vector_3	v2( m_SideVerts[ 1+i ].x, m_SideVerts[ 1+i ].y, m_SideVerts[ 1+i ].z );
		vector_3	v3( m_SideVerts[ 2+i ].x, m_SideVerts[ 2+i ].y, m_SideVerts[ 2+i ].z );

		m_SideNorms[ i ]  = Normalize( CrossProduct( (v1-v2), (v2-v3) ) );
	}
}


FUBI_DECLARE_SELF_TRAITS( DiscPile_Effect::Disc );

bool
DiscPile_Effect::Disc::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "position",		position );
	persist.Xfer( "velocity",		velocity );
	persist.Xfer( "rotation",		rotation );
	persist.Xfer( "irotation",		irotation );

	return true;
}


bool
DiscPile_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferExistingColl( "m_Discs",       m_Discs,       m_Discs       + m_DiscCount     );

	persist.XferExistingColl( "m_SideVerts",   m_SideVerts,   m_SideVerts   + ((m_Sides+1)*2) );
	persist.XferExistingColl( "m_TopVerts",    m_TopVerts,    m_TopVerts    + (m_Sides+1)     );
	persist.XferExistingColl( "m_BottomVerts", m_BottomVerts, m_BottomVerts + (m_Sides+1)     );

	persist.XferExistingColl( "m_SideNorms",   m_SideNorms,   m_SideNorms   + ((m_Sides+1)*2) );
	persist.XferExistingColl( "m_TopNorms",    m_TopNorms,    m_TopNorms    + (m_Sides+1)     );
	persist.XferExistingColl( "m_BottomNorms", m_BottomNorms, m_BottomNorms + (m_Sides+1)     );

	persist.Xfer	( "m_bHeightIndexed",		m_bHeightIndexed		);
	persist.Xfer	( "m_RelativeGroundHeight",	m_RelativeGroundHeight	);
	persist.Xfer	( "m_Base_color",			m_Base_color			);
	persist.Xfer	( "m_Radius",				m_Radius				);
	persist.Xfer	( "m_DiscRadius",			m_DiscRadius			);
	persist.Xfer	( "m_ThicknessFactor",		m_ThicknessFactor		);
	persist.Xfer	( "m_Size",					m_Size					);

	persist.Xfer	( "m_Sides",				m_Sides					);
	persist.Xfer	( "m_DiscCount",			m_DiscCount				);

	persist.Xfer	( "m_MinVel",				m_MinVel				);
	persist.Xfer	( "m_MaxVel",				m_MaxVel				);

	return true;
}


void
DiscPile_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		SiegePos	targetPos;
		matrix_3x3	target_orientation;

		if( m_Targets->GetPlacement( &targetPos, &target_orientation, m_OrientMode ) )
		{
			m_Pos.pos = targetPos.pos + m_Offset;
		}

		if( m_Dir.IsZero() )
		{
			m_Dir = vector_3::UP;
		}

		vector_3	oriented_velocity = target_orientation * m_Dir;

		for( UINT32 c = 0; c < m_DiscCount; ++c )
		{
			float anglesin, anglecos;
			SINCOSF( gWorldFx.GetRNG().Random( PI2 ), anglesin, anglecos );
			m_Discs[ c ].position = m_Pos.pos + vector_3( m_Radius*anglecos, gWorldFx.GetRNG().Random( m_Radius ), m_Radius*anglesin );

			vector_3 random_vector( RandomFloat(), RandomFloat(), RandomFloat() );

			m_Discs[ c ].rotation = random_vector * m_Radius;

			m_Discs[ c ].irotation.x = gWorldFx.GetRNG().Random( -10.0f, 10.0f );
			m_Discs[ c ].irotation.y = gWorldFx.GetRNG().Random( -10.0f, 10.0f );
			m_Discs[ c ].irotation.z = gWorldFx.GetRNG().Random( -10.0f, 10.0f );

			m_Discs[ c ].velocity.x = oriented_velocity.x + gWorldFx.GetRNG().Random( m_MinVel.x, m_MaxVel.x );
			m_Discs[ c ].velocity.y = oriented_velocity.y + gWorldFx.GetRNG().Random( m_MinVel.y, m_MaxVel.y );
			m_Discs[ c ].velocity.z = oriented_velocity.z + gWorldFx.GetRNG().Random( m_MinVel.z, m_MaxVel.z );
		}
	}

	if( m_bFinishing == true )
	{
		EndThisEffect();
		return;
	}

	const vector_3 diffuse_light( 1.0f, 1.0f, 1.0f );

	float iTheta = 2*RealPi/m_Sides;
	float theta = 0;

	// Calculate side diffuse lighting
	for( UINT32 i = 0; i <= (m_Sides+1)*2; ++i, theta += iTheta )
	{
		float dot = InnerProduct( m_SideNorms[i], gWorldFx.GetCameraDirection() );

		vector_3 diffuse;

		if( dot > 0 )
		{
			diffuse = HadamardProduct( diffuse_light, m_Base_color );
			diffuse *= dot;
		}
		else
		{
			diffuse = HadamardProduct( 0.5*m_Base_color, m_Base_color );
			diffuse *= FABSF( dot );
		}

		DWORD color	= MAKEDWORDCOLOR( diffuse );
		m_SideVerts[ i ].color = color;
	}

	// Calculate top and bottom diffuse lighting
	for(i = 1; i <= (m_Sides+1); ++i, theta += iTheta )
	{
		float dotTop = InnerProduct( m_TopNorms[ i ], gWorldFx.GetCameraDirection() );
		float dotBottom = InnerProduct( m_BottomNorms[ i ] , gWorldFx.GetCameraDirection() );

		vector_3 diffuseTop, diffuseBottom;

		if( dotTop > 0 )
		{
			diffuseTop = HadamardProduct( diffuse_light, m_Base_color );
			diffuseTop *= dotTop;
		}
		else
		{
			diffuseTop = HadamardProduct( 0.5*m_Base_color, m_Base_color );
			diffuseTop *= FABSF( dotTop );
		}

		if( dotBottom > 0 )
		{
			diffuseBottom = HadamardProduct( diffuse_light, m_Base_color );
			diffuseBottom *= dotBottom;
		}
		else
		{
			diffuseBottom = HadamardProduct( 0.5*m_Base_color, m_Base_color );
			diffuseBottom *= FABSF( dotBottom );
		}

		DWORD colorTop	= MAKEDWORDCOLOR( diffuseTop );
		DWORD colorBottom	= MAKEDWORDCOLOR( diffuseBottom );
		m_TopVerts[ i ].color = colorTop;
		m_BottomVerts[ i ].color = colorBottom;
	}

	if( !m_bHeightIndexed )
	{
		m_bHeightIndexed = true;

		SiegePos	position;
		position.pos = m_Discs[0].position;
		position.node = m_Pos.node;

		m_RelativeGroundHeight = gWorldFx.GetGroundHeight( position );
	}

	for( UINT32 c = 0; c < m_DiscCount; ++c )
	{
		if( m_Discs[c].position.y > m_RelativeGroundHeight )
		{
			m_Discs[c].rotation += m_Discs[c].irotation * m_FrameSync;

			m_Discs[c].position += 0.5f * (m_FrameSync*m_FrameSync) * gravity
										+ m_Discs[c].velocity * m_FrameSync;
			m_Discs[c].velocity += gravity * m_FrameSync;
		}
		else
		{
			m_Discs[c].velocity = vector_3();
			m_Discs[c].position.y = ( m_RelativeGroundHeight );
		}
	}
#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
DiscPile_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();

	renderer.SetTextureStageState(0,
								D3DTOP_DISABLE,
								D3DTOP_DISABLE,
								D3DTADDRESS_WRAP,
								D3DTADDRESS_WRAP,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DBLEND_ONE,
								D3DBLEND_ZERO,
								false );

	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );

	for( UINT32 c = 0; c < m_DiscCount; ++c )
	{
		renderer.SetWorldMatrix( m_NodeOrient, m_NodeCentroid + m_NodeOrient * m_Discs[ c ].position );
		renderer.RotateWorldMatrix( XRotationColumns( m_Discs[c].rotation.x ) *
									YRotationColumns( m_Discs[c].rotation.y ) *
									ZRotationColumns( m_Discs[c].rotation.z ) );
		// Draw disc
		renderer.DrawPrimitive( D3DPT_TRIANGLEFAN,	 m_TopVerts,	m_Sides+2,		SVERTEX, 0, 0 );
		renderer.DrawPrimitive( D3DPT_TRIANGLEFAN,	 m_BottomVerts,	m_Sides+2,		SVERTEX, 0, 0 );
		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, m_SideVerts,	2*(m_Sides+1),	SVERTEX, 0, 0 );
	}

	gWorldFx.SaveRenderingState();
}


float
DiscPile_Effect::GetBoundingRadius( void )
{
	return m_Radius * m_ScaleFactor;
}



Cylinder_Effect::Cylinder_Effect( void )
{
	m_Segments			=	0;

	m_Upper_radius		=	0;
	m_Upper_radius_limit=	0;
	m_Upper_radius_inc	=	0;

	m_Lower_radius		=	0;
	m_Lower_radius_limit=	0;
	m_Lower_radius_inc	=	0;

	m_Upper_height		=	0;
	m_Upper_height_limit=	0;
	m_Upper_height_inc	=	0;

	m_Lower_height		=	0;
	m_Lower_height_limit=	0;
	m_Lower_height_inc	=	0;

	m_SpinRate			=	0;
	m_Alpha				=	0;
	m_MaxAlpha			=	0;
	m_AlphaFade			=	0;

	m_In_dur			=	0;
	m_Out_dur			=	0;

	m_Theta				=	0;
	m_ThetaI			=	0;
	m_SpinPos			=	0;

	m_bDarkBlending		=	false;
}


void
Cylinder_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		1.0f, 1.0f, 1.0f);	// PARAM(1)
	params.SetVector3	( "rp0",		0.5f, 1.0f, 0.0f);	// PARAM(2)
	params.SetVector3	( "rp1",		0.5f, 1.0f, 0.0f);	// PARAM(3)
	params.SetVector3	( "hp0",		2.5f, 2.5f, 0.0f);	// PARAM(4)
	params.SetVector3	( "hp1",		0.0f, 0.0f, 0.0f);	// PARAM(5)
	params.SetVector3	( "irotate",	0.0f, 0.0f, 0.0f);	// PARAM(6)

	params.SetUINT32	( "segments",	16				);	// PARAM(7)

	params.SetFloat		( "tin",		0.5f			);	// PARAM(8)
	params.SetFloat		( "tout",		0.5f			);	// PARAM(9)
	params.SetFloat		( "alpha",		0.5f			);	// PARAM(10)
	params.SetFloat		( "spin",		0.0f			);	// PARAM(11)

	params.SetBool		( "dark",		false			);	// PARAM(12)
}


void
Cylinder_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);
	vector_3 temp( DoNotInitialize );
	m_Params.GetVector3	( PARAM(2),		temp );
	m_Upper_radius		= temp.x * m_ScaleFactor;
	m_Upper_radius_limit= temp.y * m_ScaleFactor;
	m_Upper_radius_inc	= temp.z * m_ScaleFactor;

	m_Params.GetVector3	( PARAM(3),		temp );
	m_Lower_radius		= temp.x * m_ScaleFactor;
	m_Lower_radius_limit= temp.y * m_ScaleFactor;
	m_Lower_radius_inc	= temp.z * m_ScaleFactor;

	m_Params.GetVector3	( PARAM(4),		temp );
	m_Upper_height		= temp.x * m_ScaleFactor;
	m_Upper_height_limit= temp.y * m_ScaleFactor;
	m_Upper_height_inc	= temp.z * m_ScaleFactor;

	m_Params.GetVector3	( PARAM(5),		temp );
	m_Lower_height		= temp.x * m_ScaleFactor;
	m_Lower_height_limit= temp.y * m_ScaleFactor;
	m_Lower_height_inc	= temp.z * m_ScaleFactor;

	vector_3	default_irotation;
	m_Params.GetVector3	( PARAM(6),		default_irotation );
	m_Params.GetUINT32	( PARAM(7),		m_Segments		);
	m_Segments			= min_t( (int)m_Segments, MAX_SEGMENTS );

	m_Params.GetFloat	( PARAM(8),		m_In_dur		);
	m_Params.GetFloat	( PARAM(9),		m_Out_dur		);
	m_Params.GetFloat	( PARAM(10),	m_MaxAlpha		);
	m_Params.GetFloat	( PARAM(11),	m_SpinRate		);

	m_Params.GetBool	( PARAM(12),	m_bDarkBlending	);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();

	m_DrawVerts.resize( 2*m_Segments+2 );

	if( m_bAbsoluteCoords )
	{
		m_Scale = m_ScaleFactor = 1.0f;
	}

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

	m_AlphaFade = m_MaxAlpha / m_Duration;

	m_Theta = 0;
	m_ThetaI = 2*RealPi/m_Segments;
	m_SpinPos = 0;

	if( m_bRotation || !default_irotation.IsZero() )
	{
		m_Rotation =	(XRotationColumns( m_RotationAngles.x ) *
						YRotationColumns( m_RotationAngles.y )) *
						ZRotationColumns( m_RotationAngles.z );

		// Convert rotation increments to radians;
		m_iRotation = (default_irotation * (RealPi/180.0f ));
	}
}


bool
Cylinder_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer		( "m_Base_color",			m_Base_color );
	persist.Xfer		( "m_Segments",				m_Segments );

	persist.Xfer		( "m_Upper_radius",			m_Upper_radius );
	persist.Xfer		( "m_Upper_radius_limit",	m_Upper_radius_limit );
	persist.Xfer		( "m_Upper_radius_inc",		m_Upper_radius_inc );
						
	persist.Xfer		( "m_Lower_radius",			m_Lower_radius );
	persist.Xfer		( "m_Lower_radius_limit",	m_Lower_radius_limit );
	persist.Xfer		( "m_Lower_radius_inc",		m_Lower_radius_inc );
						
	persist.Xfer		( "m_Upper_height",			m_Upper_height );
	persist.Xfer		( "m_Upper_height_limit",	m_Upper_height_limit );
	persist.Xfer		( "m_Upper_height_inc",		m_Upper_height_inc );
						
	persist.Xfer		( "m_Lower_height",			m_Lower_height );
	persist.Xfer		( "m_Lower_height_limit",	m_Lower_height_limit );
	persist.Xfer		( "m_Lower_height_inc",		m_Lower_height_inc );
						
	persist.Xfer		( "m_SpinRate",				m_SpinRate );
	persist.Xfer		( "m_Alpha",				m_Alpha );
	persist.Xfer		( "m_MaxAlpha",				m_MaxAlpha );
	persist.Xfer		( "m_AlphaFade",			m_AlphaFade );
						
	persist.Xfer		( "m_In_dur",				m_In_dur );
	persist.Xfer		( "m_Out_dur",				m_Out_dur );
						
	persist.Xfer		( "m_Theta",				m_Theta );
	persist.Xfer		( "m_ThetaI",				m_ThetaI );
	persist.Xfer		( "m_SpinPos",				m_SpinPos );
						
	persist.Xfer		( "m_bDarkBlending",		m_bDarkBlending );
						
	persist.Xfer		( "m_Rotation",				m_Rotation );
	persist.Xfer		( "m_iRotation",			m_iRotation );
	persist.Xfer		( "m_sRotation",			m_sRotation );

	persist.Xfer		( "m_RenderRotation",		m_RenderRotation );

	if( m_DrawVerts.empty() )
	{
		m_DrawVerts.resize( 2*m_Segments+2 );
	}
	return true;
}


void
Cylinder_Effect::UpdateDerived( void )
{
	m_SpinPos += m_SpinRate * m_FrameSync;

	if( m_SpinPos >= 2*RealPi )
	{
		m_SpinPos -= 2*RealPi;
	}

	m_Theta = m_SpinPos;

	if( m_Upper_radius_inc != 0 )
	{
		m_Upper_radius += m_Upper_radius_inc * m_FrameSync;

		if( m_Upper_radius_inc > 0 )
		{
			m_Upper_radius = ( m_Upper_radius > m_Upper_radius_limit )? m_Upper_radius_limit :
							m_Upper_radius;
		}
		else
		{
			m_Upper_radius = ( m_Upper_radius < m_Upper_radius_limit )? m_Upper_radius_limit :
							m_Upper_radius;
		}
	}

	if( m_Lower_radius_inc != 0 )
	{
		m_Lower_radius += m_Lower_radius_inc * m_FrameSync;

		if( m_Lower_radius_inc > 0 )
		{
			m_Lower_radius = ( m_Lower_radius > m_Lower_radius_limit )? m_Lower_radius_limit :
							m_Lower_radius;
		}
		else
		{
			m_Lower_radius = ( m_Lower_radius < m_Lower_radius_limit )? m_Lower_radius_limit :
							m_Lower_radius;
		}
	}

	if( m_Upper_height_inc != 0 )
	{
		m_Upper_height += m_Upper_height_inc * m_FrameSync;

		if( m_Upper_height_inc > 0 )
		{
			m_Upper_height = ( m_Upper_height > m_Upper_height_limit )? m_Upper_height_limit :
							m_Upper_height;
		}
		else
		{
			m_Upper_height = ( m_Upper_height < m_Upper_height_limit )? m_Upper_height_limit :
							m_Upper_height;
		}
	}

	if( m_Lower_height_inc != 0 )
	{
		m_Lower_height += m_Lower_height_inc * m_FrameSync;

		if( m_Lower_height_inc > 0 )
		{
			m_Lower_height = ( m_Lower_height > m_Lower_height_limit )? m_Lower_height_limit :
							m_Lower_height;
		}
		else
		{
			m_Lower_height = ( m_Lower_height < m_Lower_height_limit )? m_Lower_height_limit :
							m_Lower_height;
		}
	}

	if( m_Duration != -1.0f )
	{
		if( m_bFinishing || (m_Elapsed >= (m_Duration - m_Out_dur)) )
		{
			m_Alpha = (m_Duration - m_Elapsed) / m_Out_dur;
			m_bFinishing = true;
		}
		else
		{
			m_Alpha = (m_Alpha<=1.0f)?( (m_Elapsed / m_In_dur) + 0.00001f):1.0f;
		}
	}

	m_Alpha = (m_Alpha>m_MaxAlpha)?m_MaxAlpha:m_Alpha;

	
	if( m_OrientMode == Flamethrower_target::ORIENT_BONE )
	{
		matrix_3x3	target_orient;
		m_Targets->GetPlacement( NULL, &target_orient, m_OrientMode );

		bool bDoRotation = !m_iRotation.IsZero();

		if( m_bRotation || bDoRotation )
		{
			if( bDoRotation )
			{
				matrix_3x3 rotation((XRotationColumns( m_sRotation.x ) *
									YRotationColumns( m_sRotation.y )) *
									ZRotationColumns( m_sRotation.z ) );

				matrix_3x3 temp( target_orient * m_Rotation );
				temp = temp * rotation;

				m_sRotation += m_iRotation * m_FrameSync;
				m_RenderRotation = temp;
			}
			else
			{
				m_RenderRotation = ( target_orient * m_Rotation  );
			}
		}
		else
		{
			m_RenderRotation = target_orient;
		}
	}

	DWORD color	= MAKEDWORDCOLOR( m_Base_color );
	color		= ((color&0x00FFFFFF)|((BYTE(m_Alpha*m_NodeAlpha*255.0f))<<24));

	float thetasin, thetacos;

	for( UINT32 i = 0, j = 0; i < m_Segments+1; ++i, j+=2, m_Theta += m_ThetaI )
	{
		SINCOSF( m_Theta, thetasin, thetacos );

		m_DrawVerts[0+j].x		= m_Upper_radius * thetacos;
		m_DrawVerts[0+j].y		= m_Upper_height;
		m_DrawVerts[0+j].z		= m_Upper_radius * thetasin;
		m_DrawVerts[0+j].color	= color;
		m_DrawVerts[0+j].uv.u	= 0.0f + float(i/( (float)m_Segments+1) );
		m_DrawVerts[0+j].uv.v	= 0.98f;

		m_DrawVerts[1+j].x		= m_Lower_radius * thetacos;
		m_DrawVerts[1+j].y		= m_Lower_height;
		m_DrawVerts[1+j].z		= m_Lower_radius * thetasin;
		m_DrawVerts[1+j].color	= color;
		m_DrawVerts[1+j].uv.u	= 0.0f + float(i/( (float)m_Segments+1) );
		m_DrawVerts[1+j].uv.v	= 0.0f;
	}

	m_bFinished = ( (m_bFinished) | (m_Alpha <= 0.0f) );

#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
Cylinder_Effect::DrawSelf( void )
{
	if( m_bFinished || (m_Segments < 3) )
	{
		return;
	}

	Rapi &renderer = gWorldFx.GetRenderer();

	if( m_bDarkBlending )
	{
		renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG1,
									D3DTOP_SELECTARG1,

									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
		renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );
	}
	else
	{
		renderer.SetTextureStageState(0,
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
	}

	renderer.RotateWorldMatrix( m_RenderRotation );

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, &*m_DrawVerts.begin(), m_DrawVerts.size(), SVERTEX, &m_TextureName, 1 );

	gWorldFx.SaveRenderingState();
}


float
Cylinder_Effect::GetBoundingRadius( void )
{
	return (m_Upper_radius>m_Lower_radius)?m_Upper_radius:m_Lower_radius;
}




PointTracer_Effect::PointTracer_Effect( void )
	: m_Last_index( 0 )
{
	memset( m_DrawVerts, 0, sizeof( sVertex )*4 );

	m_Fade_rate			= 0;
	m_Decay_rate		= 0;
	m_Velocity			= 0;
	m_Frequency			= 0;
	m_Period			= 0;
	m_Impact_t			= 0;
	m_Tracer_limit		= 0;
	m_Tracer_count		= 0;
	m_Tracer_index		= 0;
	m_Last_index		= 0;
}


void
PointTracer_Effect::RegisterDerivedParams( EffectParams &params )
{
	params.SetVector3	( "color0",		1.0f, 1.0f, 1.0f);	// PARAM(1)

	params.SetFloat		( "fade_rate",	2.0f			);	// PARAM(2)
	params.SetFloat		( "decay_rate",	0.08f			);	// PARAM(3)
	params.SetFloat		( "velocity",	17.0f			);	// PARAM(4)
	params.SetFloat		( "frequency",	0.01f			);	// PARAM(5)
	params.SetFloat		( "period",		3.0f			);	// PARAM(6)
}


void
PointTracer_Effect::InitializeDerived( void )
{
	m_Params.GetVector3	( PARAM(1),		m_Base_color	);

	m_Params.GetFloat	( PARAM(2),		m_Fade_rate		);
	m_Params.GetFloat	( PARAM(3),		m_Decay_rate	);
	m_Params.GetFloat	( PARAM(4),		m_Velocity		);
	m_Params.GetFloat	( PARAM(5),		m_Frequency		);
	m_Params.GetFloat	( PARAM(6),		m_Period		);

	// Done with the params so clear the collection to save memory
	m_Params.Clear();
}


FUBI_DECLARE_SELF_TRAITS( PointTracer_Effect::Tracer );

bool
PointTracer_Effect::Tracer::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "time_index",		time_index	);
	persist.Xfer( "position",		position	);
	persist.Xfer( "alpha",			alpha		);

	return true;
}


bool
PointTracer_Effect::OnXfer( FuBi::PersistContext &persist )
{
	persist.XferVector	( "m_Tracers",			m_Tracers			);

	persist.Xfer		( "m_Base_color",		m_Base_color		);
	persist.Xfer		( "m_Fade_rate",		m_Fade_rate			);
	persist.Xfer		( "m_Decay_rate",		m_Decay_rate		);
	persist.Xfer		( "m_Velocity",			m_Velocity			);
	persist.Xfer		( "m_Frequency",		m_Frequency			);
	persist.Xfer		( "m_Period",			m_Period			);
	persist.Xfer		( "m_Impact_t",			m_Impact_t			);
						
	persist.Xfer		( "m_Target_position",	m_Target_position	);
	persist.Xfer		( "m_Spawn_position",	m_Spawn_position	);
						
	persist.Xfer		( "m_Tracer_limit",		m_Tracer_limit		);
	persist.Xfer		( "m_Tracer_count",		m_Tracer_count		);
	persist.Xfer		( "m_Tracer_index",		m_Tracer_index		);
	persist.Xfer		( "m_Last_index",		m_Last_index		);

	return true;
}


void
PointTracer_Effect::UpdateDerived( void )
{
	// Constructor deferred initialization
	if( !m_bInitialized )
	{
		m_bInitialized = true;

		m_Targets->GetPlacement( &m_Target_position, NULL, m_OrientMode );
		m_Spawn_position = m_Pos;

		vector_3 velocity_vector;
		float firing_angle;

		if( !Physcalc::CalculateFiringAngle( gSiegeEngine.GetDifferenceVector( m_Spawn_position, m_Target_position ),
										m_Velocity, true, firing_angle ) )
		{
			EndThisEffect();
			return;
		}

		m_Impact_t = Physcalc::CalculateTimeToImpact( gSiegeEngine.GetDifferenceVector( m_Spawn_position, m_Target_position ),
										m_Velocity, firing_angle, 9.80665f );
		Physcalc::CalculateFiringVector( gSiegeEngine.GetDifferenceVector( m_Spawn_position, m_Target_position ),
										m_Velocity, true, 0, 0, velocity_vector );

		int i = 0;
	
		m_Tracers.resize( UINT32(m_Period/m_Frequency) );

		for( float ti = 0; ti < m_Period; ti += m_Frequency, ++i )
		{
			vector_3	new_position( (gravity * ti*ti) * 0.5f + velocity_vector * ti +	m_Spawn_position.pos );
			m_Tracers[i].time_index = ti;
			m_Tracers[i].position.pos = new_position;
			m_Tracers[i].position.node = m_Spawn_position.node;
			m_Tracers[i].alpha = 0.0f;

			if( i == MAX_TRACERS )
			{
				break;
			}
		}

		m_Tracer_count = i;
		m_Tracer_index = 0;
	}

	if( m_Elapsed >= m_Impact_t )
	{
		bool fade_done = true;
		for( int ifade = m_Last_index; ifade > 0; --ifade )
		{
			if( m_Tracers[ifade].alpha != 0 )
			{
				m_Tracers[ifade].alpha -= m_Fade_rate * m_FrameSync;
				m_Tracers[ifade].alpha = FilterClamp( 0.0f, 1.0f, m_Tracers[ifade].alpha );
				fade_done = false;
			}
		}

		if( fade_done )
		{
			EndThisEffect();
			return;
		}
	}
	else
	{
		for( UINT32 j = 0; j < m_Tracer_count-1; ++j )
		{
			if( m_Tracers[j+1].time_index < m_Elapsed )
			{
				continue;
			}

			float alpha_set = 1.0f;

			for( int k = j; k > 0; --k ) 
			{
				alpha_set -= m_Decay_rate;
				m_Tracers[k].alpha = alpha_set;

				if( alpha_set <= 0.0f )	
				{
					m_Tracers[k].alpha = 0;
					break;
				}
			}

			for( int l = k; l > 0; --l )
			{
				m_Tracers[l].alpha = 0;
			}
			break;
		}
		m_Last_index = j;
	}
#if !GP_RETAIL
	gFlamethrower.IncUpdatedEffects();
#endif
}


void
PointTracer_Effect::DrawSelf( void )
{
	if( m_bFinished )
	{
		return;
	}

	Rapi	&renderer = gWorldFx.GetRenderer();

	renderer.SetTextureStageState(0,
								D3DTOP_DISABLE,
								D3DTOP_MODULATE,
								D3DTADDRESS_WRAP,
								D3DTADDRESS_WRAP,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DTEXF_LINEAR,
								D3DBLEND_SRCALPHA,
								D3DBLEND_ONE,
								false );

	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );

	SetDrawMatrix( m_Spawn_position.node );

	for( int i = 0; i < m_Last_index; ++i )
	{
		vector_3 vp0	= m_Tracers[ i ].position.pos;
		vector_3 vp2	= m_Tracers[ i+1 ].position.pos;

		DWORD color1	= MAKEDWORDCOLOR( m_Base_color );
		DWORD color2	= color1;
		color1			= ((color1&0x00FFFFFF)|((BYTE(m_Tracers[i].alpha*m_NodeAlpha*255.0f))<<24));
		color2			= ((color2&0x00FFFFFF)|((BYTE(m_Tracers[i+1].alpha*m_NodeAlpha*255.0f))<<24));

		m_DrawVerts[0].x			= vp0.x;	m_DrawVerts[1].x			= vp2.x;
		m_DrawVerts[0].y			= vp0.y;	m_DrawVerts[1].y			= vp2.y;
		m_DrawVerts[0].z			= vp0.z;	m_DrawVerts[1].z			= vp2.z;
		m_DrawVerts[0].color		= color1;	m_DrawVerts[1].color		= color2;

		renderer.DrawPrimitive( D3DPT_LINELIST, m_DrawVerts, 2, SVERTEX, 0, 1 );
	}

	gWorldFx.SaveRenderingState();
}


float
PointTracer_Effect::GetBoundingRadius( void )
{
	return 1.0f;
}
