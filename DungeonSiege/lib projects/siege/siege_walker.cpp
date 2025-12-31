/*******************************************************************************************
**
**									SiegeWalker
**
**		See .h file for details
**
*******************************************************************************************/

#include "precomp_siege.h"
#include "gpcore.h"

// Renderer includes
#include "RapiOwner.h"
#include "RapiPrimitive.h"

// Siege includes
#include "siege_cache.h"
#include "siege_cache_handle.h"
#include "siege_camera.h"
#include "siege_database_guid.h"
#include "siege_engine.h"
#include "siege_frustum.h"
#include "siege_hotpoint_database.h"
#include "siege_logical_mesh.h"
#include "siege_logical_node.h"
#include "siege_mesh.h"
#include "siege_mesh_door.h"
#include "siege_mouse_shadow.h"
#include "siege_options.h"
#include "siege_persist.h"
#include "siege_pos.h"
#include "siege_viewfrustum.h"
#include "siege_walker.h"

// Nema includes
#include "nema_aspect.h"

// Core includes
#include "space_3.h"
#include "filter_1.h"
#include "oriented_bounding_box_3.h"
#include "gpprofiler.h"
#include "fubipersist.h"
#include "fubitraitsimpl.h"

#include <malloc.h>

const float MIN_BOUND_SELECT_TEST = 0.5f;

using namespace siege;

#if !GP_RETAIL

void DebugDrawBox( const vector_3& c, const vector_3& d, const vector_3& colorv, Rapi& renderer )
{
	vector_3 p0 = c + vector_3( d.x, d.y,-d.z);
	vector_3 p1 = c + vector_3(-d.x, d.y,-d.z);
	vector_3 p2 = c + vector_3(-d.x,-d.y,-d.z);
	vector_3 p3 = c + vector_3( d.x,-d.y,-d.z);
	vector_3 p4 = c + vector_3( d.x, d.y, d.z);
	vector_3 p5 = c + vector_3(-d.x, d.y, d.z);
	vector_3 p6 = c + vector_3(-d.x,-d.y, d.z);
	vector_3 p7 = c + vector_3( d.x,-d.y, d.z);

	renderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= MAKEDWORDCOLOR( colorv );
	verts[1].color	= verts[0].color;

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p1, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p1, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	memcpy( &verts[1], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p3, sizeof( float ) * 3 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
}

#endif

#if GP_DEBUG

bool ASPECTINFO::AssertValid( void ) const
{

	gpassertf( Aspect ,					("ASPECTINFO has invalid nema aspect"));
	gpassertf( Object != 0,				("ASPECTINFO of [%s] has invalid Object",Aspect->GetDebugName()));
	gpassertf( pNode != NULL,			("ASPECTINFO of [%s] has invalid pNode",Aspect->GetDebugName()));
	gpassertf( ObjectScale > 0,			("ASPECTINFO of [%s] has invalid ObjectScale",Aspect->GetDebugName()));
	gpassertf( BoundingBodyScale > 0,	("ASPECTINFO of [%s] has invalid BoundingBodyScale",Aspect->GetDebugName()));

	return ( true );
}

#endif // GP_DEBUG

SiegeWalker::SiegeWalker(void)
	: m_targetNodeGUID(UNDEFINED_GUID)
	, m_LastLightVisit( 0 )
	, m_CurrentLightVisit( 0 )
	, m_MiniMeters( 0.125f )
	, m_MiniMaxMeters( 80.0f )
	, m_MiniMinMeters( 20.0f )
	, m_bMiniZoomIn( false )
	, m_bMiniZoomOut( false )
	, m_MiniMetersZoomPerSecond( 0.25f )
	, m_MiniStepZoomMeters( 2.5f )
	, m_OrientArrowTex( 0 )
	, m_SelectionRingTex( 0 )
	, m_DiscoveryTex( 0 )
	, m_SelectionTriTex( 0 )
	, m_TombstoneTex( 0 )
	, m_ObjectFadeTime( 0.4f )
	, m_innerFrustumScale( 1.0f )
	, m_bShadowGenerate( false )
{
}

SiegeWalker::~SiegeWalker(void)
{
	Clear();

	// Destroy the minimap textures
	if( m_OrientArrowTex )
	{
		gDefaultRapi.DestroyTexture( m_OrientArrowTex );
	}
	if( m_SelectionRingTex )
	{
		gDefaultRapi.DestroyTexture( m_SelectionRingTex );
	}
	if( m_DiscoveryTex )
	{
		gDefaultRapi.DestroyTexture( m_DiscoveryTex );
	}
	if( m_SelectionTriTex )
	{
		gDefaultRapi.DestroyTexture( m_SelectionTriTex );
	}
	if( m_TombstoneTex )
	{
		gDefaultRapi.DestroyTexture( m_TombstoneTex );
	}
}

bool SiegeWalker::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_targetNodeGUID", m_targetNodeGUID );
	persist.XferMap( "m_discoveryPosMap", m_discoveryPosMap );

	return ( true );
}

// Step the minimap zoom level in
void SiegeWalker::StepMiniMapZoomIn()
{
	m_MiniMeters	-= m_MiniStepZoomMeters;
	clamp_min_max( m_MiniMeters, m_MiniMinMeters, m_MiniMaxMeters );
}

// Step the minimap zoom level out
void SiegeWalker::StepMiniMapZoomOut()
{
	m_MiniMeters	+= m_MiniStepZoomMeters;
	clamp_min_max( m_MiniMeters, m_MiniMinMeters, m_MiniMaxMeters );
}

/*****************************************************************/
//
//	RenderVisibleNodes()
//
//  The heart of the Siege engine, this is where most of the
//  work that goes into rendering a scene happens.
//
/*****************************************************************/
void SiegeWalker::RenderVisibleNodes()
{
	GPPROFILERSAMPLE( "SiegeWalker::RenderVisibleNodes()", SP_DRAW );

	// Get the currently active render frustum
	SiegeFrustum* pRenderFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pRenderFrustum )
	{
		// Get the render list from this frustum
		FrustumNodeColl& nodeColl		= pRenderFrustum->GetFrustumNodeColl();

		Rapi& renderer	= gSiegeEngine.Renderer();

		// Clear our node collections
		m_NodeColl.clear();
		m_AlphaNodeColl.clear();

		// Clear our object collections
		m_ObjectColl.clear();
		m_ObjectList.clear();
		m_ObjectAlphaList.clear();
		m_EffectColl.clear();

		// Update visit counts
		m_LastLightVisit = m_CurrentLightVisit;
		m_CurrentLightVisit++;

		// Calculate offset
		int fadeAmount	= FTOL( (255.0f - (float)gSiegeEngine.m_NodeFadeBlack) *
								max_t( 0.0f, min_t( 1.0f, (float)gSiegeEngine.GetAbsoluteDeltaFrameTime() / gSiegeEngine.m_NodeFadeTime ) ) );

		// Render all of the SiegeNodes and setup the object list
		for( FrustumNodeColl::iterator i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			// Light and render the current node
			if( LightSiegeNode( *(*i) ) )
			{
				if( (*i)->GetCameraFade() )
				{
					// Calculate direction of the camera
					vector_3 ray_origin		= (*i)->WorldToNodeSpace( gSiegeEngine.GetCamera().GetCameraPosition() );
					vector_3 ray_direction	= (*i)->GetTransposeOrientation() * gSiegeEngine.GetCamera().GetVectorOrientation();

					float ray_min			= FLOAT_MIN;
					vector_3 face_normal( DoNotInitialize );

					// Check for intersection
					if( (*i)->HitTest( ray_origin, ray_direction, ray_min, face_normal, LF_ALL ) &&
						( (ray_min > 0.0f) && (ray_min <= 1.0f) ) )
					{
						// Calculate the passed time interval and set the camera alpha
						(*i)->SetCameraNodeAlpha( (DWORD)max_t( (int)gSiegeEngine.m_NodeFadeBlack, (int)(*i)->m_CameraAlpha -  fadeAmount) );
					}
					else
					{
						(*i)->SetCameraNodeAlpha( (DWORD)min_t( (int)255, (int)(*i)->m_CameraAlpha + fadeAmount ) );
					}

					if( !(*i)->GetNodeAlpha() )
					{
						continue;
					}
				}

				// Put this node on our lists for sorting
				if( (*i)->GetNodeAlpha() == 0xFF )
				{
					m_NodeColl.push_back( (*i) );

					// Go through occupants of this node and set them up for rendering
					gSiegeEngine.CollectObjects( m_ObjectColl, *(*i), CF_ACTOR | CF_ITEM, RF_POSITION | RF_ORIENT | RF_RENDERINDEX | RF_BOUNDS | RF_SCALE | RF_FRUSTUMCLIP | RF_ALPHA | RF_CAMERA_DISTANCE | RF_PROJ_CENTER );
					gSiegeEngine.CollectEffects( m_EffectColl, *(*i) );
				}

				if( (*i)->HasAlphaStages() || ((*i)->GetNodeAlpha() != 0xFF) )
				{
					m_AlphaNodeColl.push_back( (*i) );
				}

				// Set the node distance from camera
				(*i)->SetCameraDistance( Length( gSiegeEngine.GetCamera().GetCameraPosition() - (*i)->GetCurrentCenter() ) );
			}
		}

		// Setup the render states to support shadow drawing
		bool bFog			= renderer.SetFogActivatedState( false );
		bool bAlphaBlend	= renderer.EnableAlphaBlending( false );
		renderer.GetDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );

		// Render the objects
		if( gSiegeEngine.GetOptions().DrawObjects() )
		{
			// Calculate the amount of the alpha modification this frame
			int alphaMod	= min( max( 1, FTOL( 255.0f * ((float)gSiegeEngine.GetAbsoluteDeltaFrameTime() / m_ObjectFadeTime) ) ), 255 );

			for( WorldObjectColl::iterator o = m_ObjectColl.begin(); o != m_ObjectColl.end(); ++o )
			{
				if( ((*o).Flags & CF_INTERESTONLY) && !(*o).pNode->IsInterestByFrustum( gSiegeEngine.GetRenderFrustum() ) )
				{
					continue;
				}

				if( gSiegeEngine.GetOptions().IsObjectFading() )
				{
					bool bAlphaModified	= false;
					if( (*o).Flags & CF_AVOID_CAMERA )
					{
						int alphaCamMod	= min( max( 1, FTOL( (float)(255 - (*o).CameraFadeAlpha) * ((float)gSiegeEngine.GetAbsoluteDeltaFrameTime() / m_ObjectFadeTime) ) ), 255 );

						// Run ray collision with camera
						oriented_bounding_box_3 tempBox( (*o).BoundCenter, (*o).BoundHalfDiag, (*o).BoundOrient );
						vector_3 intersectCoord( DoNotInitialize );
						if( tempBox.RayIntersectsOrientedBox( gSiegeEngine.GetCamera().GetCameraPosition(),
															  gSiegeEngine.GetCamera().GetVectorOrientation(),
															  &intersectCoord ) &&
							(Length( intersectCoord - gSiegeEngine.GetCamera().GetCameraPosition() ) <=
							(gSiegeEngine.GetCamera().GetVectorOrientation().Length() + 3.0f)) )
						{
							// Reduce alpha level
							(*o).Alpha	= (DWORD)max_t( (int)(*o).Alpha - alphaCamMod, (int)(*o).CameraFadeAlpha );
						}
						else
						{
							(*o).Alpha	= (DWORD)min_t( (int)(*o).Alpha + alphaCamMod, (int)255 );
						}
						bAlphaModified = true;
					}

					if( (*o).Flags & CF_FADEOUT )
					{
						// Reduce alpha level
						(*o).Alpha	= (DWORD)max_t( (int)(*o).Alpha - alphaMod, (int)0 );
						bAlphaModified = true;
					}

					if( (*o).Flags & CF_FADEIN )
					{
						// Increase alpha level
						(*o).Alpha	= (DWORD)min_t( (int)(*o).Alpha + alphaMod, (int)255 );
						bAlphaModified = true;
					}

					// If the alpha for this object is not supposed to be modified, set it to maximum
					if( !bAlphaModified && !(((*o).Flags & CF_PARTYMEMBER) || ((*o).Flags & CF_GIZMO) || ((*o).Flags & CF_GHOST)) )
					{
						(*o).Alpha = 0xFF;
					}
				}

				// Get the aspect
				nema::Aspect* aspect = (*o).Aspect;

				if( (*o).Alpha < 0xFF )
				{
					(*o).Flags |= CF_RENDERALPHAOBJECT;
					m_ObjectAlphaList.push_back( &(*o) );
				}
				else
				{
					if( aspect->GetSharedAttrFlags() & nema::VERTEX_ALPHA )
					{
						(*o).Flags |= CF_RENDERALPHAOBJECT;
						m_ObjectAlphaList.push_back( &(*o) );
					}
					else
					{
						for( unsigned int t = 0, count = aspect->GetNumSubTextures(); t < count; ++t )
						{
							if( gDefaultRapi.IsTextureAlpha( aspect->GetTexture( t ) ) )
							{
								(*o).Flags |= CF_RENDERALPHAOBJECT;
								m_ObjectAlphaList.push_back( &(*o) );
								break;
							}
						}

						if( t == aspect->GetNumSubTextures() )
						{
							if( (*o).Aspect->GetInstanceAttrFlags() & (nema::RENDER_TRACERS|nema::RENDER_FROZEN) )
							{
								m_ObjectAlphaList.push_back( &(*o) );
							}
							m_ObjectList.push_back( &(*o) );
						}
					}
				}

				// Light, deform, and generate shadow if needed
				LightAndDeformObject( (*o) );
			}
		}

		renderer.GetDevice()->SetRenderState( D3DRS_ZENABLE, TRUE );
		renderer.SetFogActivatedState( bFog );

		// Clear the backbuffer now
		if( gSiegeEngine.GetOptions().UseExpensiveShadows() ||
			gSiegeEngine.GetOptions().UseExpensivePartyShadows() )
		{
			if( renderer.IsShadowRenderTarget() )
			{
				// Set the render target back to primary
				renderer.ResetRenderTarget();
			}
			else
			{
				// Clear the back buffer
				RECT bltRect = { 0, 0, renderer.GetShadowTexSize(), renderer.GetShadowTexSize() };
				renderer.ClearBackBuffer( &bltRect, renderer.GetClearColor() );
			}

			// Restore the observer projection
			renderer.SetObserverProjectionActive();
		}

		// Force fog state reset
		renderer.SetFogActivatedState( !renderer.GetFogActivatedState() );
		renderer.SetFogActivatedState( !renderer.GetFogActivatedState() );

		if( gSiegeEngine.GetOptions().NodeBoxes() )
		{
			renderer.PushWorldMatrix();
			renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
			renderer.TranslateWorldMatrix( gSiegeEngine.GetCamera().GetTargetPosition() );

			RP_DrawSphere( renderer, 0.2f, 30, 0xFFFF0000 );

			renderer.PopWorldMatrix();
		}

		// Sort our object and node lists
		std::sort( m_NodeColl.begin(), m_NodeColl.end(), node_less() );
		std::sort( m_ObjectList.begin(), m_ObjectList.end(), obj_less() );

		std::sort( m_AlphaNodeColl.begin(), m_AlphaNodeColl.end(), node_greater() );
		std::sort( m_ObjectAlphaList.begin(), m_ObjectAlphaList.end(), obj_greater() );
		std::sort( m_EffectColl.begin(), m_EffectColl.end(), effect_greater() );

		for( SiegeNodeColl::iterator n = m_NodeColl.begin(); n != m_NodeColl.end(); ++n )
		{
			// Render the node
			RenderSiegeNode( *(*n), false );
		}

		// Iterate over all remaining opaque objects
		for( WorldObjectList::iterator o = m_ObjectList.begin(); o != m_ObjectList.end(); ++o )
		{
			// Validate render request
			gpassert( (*o)->AssertValid() );

			// Get the SiegeNode
			const SiegeNode& node = *(*o)->pNode;

			renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
			renderer.SetEnvironmentTexture( node.GetEnvironmentMap() );

			// Render this object
			RenderObject( *(*o) );
		}

		// Notify any callbacks of our post opaque render state
		gSiegeEngine.NotifyPostOpaqueRender();

		renderer.EnableAlphaBlending( true );

		for( n = m_AlphaNodeColl.begin(); n != m_AlphaNodeColl.end(); ++n )
		{
			if( (*n)->HasNoAlphaSortStages() )
			{
				if( (*n)->GetNodeAlpha() < 255 )
				{
					RenderSiegeNode( *(*n), false );
				}

				RenderSiegeNode( *(*n), true, true );
			}
		}

		// Go through alpha object list and look for ghosts
		for( o = m_ObjectAlphaList.begin(); o != m_ObjectAlphaList.end(); )
		{
			if( (*o)->Flags & CF_GHOST )
			{
				// Validate render request
				gpassert( (*o)->AssertValid() );

				// Get the SiegeNode
				const SiegeNode& node = *(*o)->pNode;

				renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
				renderer.SetEnvironmentTexture( node.GetEnvironmentMap() );

				// Render this object
				if( (*o)->Flags & CF_RENDERALPHAOBJECT )
				{
					RenderObject( *(*o) );
				}

				DWORD inst_attr = (*o)->Aspect->GetInstanceAttrFlags();
				if( inst_attr & (nema::RENDER_TRACERS | nema::RENDER_FROZEN) )
				{
					if( !((*o)->Flags & CF_RENDERALPHAOBJECT) )
					{
						// Setup the matrix
						renderer.TranslateWorldMatrix( (*o)->Origin );
						renderer.RotateWorldMatrix( (*o)->Orientation );
						renderer.ScaleWorldMatrix( vector_3( (*o)->ObjectScale ) );
					}

					if ( inst_attr & nema::RENDER_TRACERS)
					{
						(*o)->Aspect->RenderTracers( &renderer, 0 );
					}
					if ( inst_attr & nema::RENDER_FROZEN)
					{
						(*o)->Aspect->RenderFrozen( &renderer, 0 );
					}
				}

				o = m_ObjectAlphaList.erase( o );
			}
			else
			{
				++o;
			}
		}

		WorldEffectColl::iterator e = m_EffectColl.begin();
		o = m_ObjectAlphaList.begin();
		for( n = m_AlphaNodeColl.begin(); n != m_AlphaNodeColl.end(); ++n )
		{
			// Render sorted effects and objects
			for( ; ; )
			{
				float testDistance	= (*n)->GetCameraDistance();
				if( (o != m_ObjectAlphaList.end()) && ((*o)->CameraDistance > testDistance) )
				{
					testDistance	= (*o)->CameraDistance;
				}

				if( (e != m_EffectColl.end()) && ((*e).CameraDistance > testDistance) )
				{
					GPPROFILERSAMPLE( "SiegeFx::RenderCallback()", SP_DRAW | SP_EFFECTS );

					// Render effect
					(*e).RenderCallback();

					// Increment to next effect
					++e;
				}
				else if( testDistance > (*n)->GetCameraDistance() )
				{
					// Validate render request
					gpassert( (*o)->AssertValid() );

					// Get the SiegeNode
					const SiegeNode& node = *(*o)->pNode;

					renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
					renderer.SetEnvironmentTexture( node.GetEnvironmentMap() );

					// Render this object
					if( (*o)->Flags & CF_RENDERALPHAOBJECT )
					{
						RenderObject( *(*o) );
					}

					DWORD inst_attr = (*o)->Aspect->GetInstanceAttrFlags();
					if( inst_attr & (nema::RENDER_TRACERS | nema::RENDER_FROZEN) )
					{
						if( !((*o)->Flags & CF_RENDERALPHAOBJECT) )
						{
							// Setup the matrix
							renderer.TranslateWorldMatrix( (*o)->Origin );
							renderer.RotateWorldMatrix( (*o)->Orientation );
							renderer.ScaleWorldMatrix( vector_3( (*o)->ObjectScale ) );
						}

						if ( inst_attr & nema::RENDER_TRACERS)
						{
							(*o)->Aspect->RenderTracers( &renderer, 0 );
						}
						if ( inst_attr & nema::RENDER_FROZEN)
						{
							(*o)->Aspect->RenderFrozen( &renderer, 0 );
						}
					}

					// Increment to next object
					++o;
				}
				else
				{
					break;
				}
			}

			if( (*n)->GetNodeAlpha() < 255 && !(*n)->HasNoAlphaSortStages() )
			{
				RenderSiegeNode( *(*n), false );
			}

			RenderSiegeNode( *(*n), true );
		}

		// Render any remaining effects and objects
		for( ; ; )
		{
			float testDistance	= 0.0f;
			if( o != m_ObjectAlphaList.end() )
			{
				testDistance	= (*o)->CameraDistance;
			}
			if( (e != m_EffectColl.end()) && ((*e).CameraDistance > testDistance) )
			{
				GPPROFILERSAMPLE( "SiegeFx::RenderCallback()", SP_DRAW | SP_EFFECTS );

				// Render effect
				(*e).RenderCallback();

				// Increment to next effect
				++e;
			}
			else if( o != m_ObjectAlphaList.end() )
			{
				// Validate render request
				gpassert( (*o)->AssertValid() );

				// Get the SiegeNode
				const SiegeNode& node = *(*o)->pNode;

				renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
				renderer.SetEnvironmentTexture( node.GetEnvironmentMap() );

				// Render this object
				if( (*o)->Flags & CF_RENDERALPHAOBJECT )
				{
					RenderObject( *(*o) );
				}

				DWORD inst_attr = (*o)->Aspect->GetInstanceAttrFlags();
				if( inst_attr & (nema::RENDER_TRACERS | nema::RENDER_FROZEN) )
				{
					if( !((*o)->Flags & CF_RENDERALPHAOBJECT) )
					{
						// Setup the matrix
						renderer.TranslateWorldMatrix( (*o)->Origin );
						renderer.RotateWorldMatrix( (*o)->Orientation );
						renderer.ScaleWorldMatrix( vector_3( (*o)->ObjectScale ) );
					}

					if ( inst_attr & nema::RENDER_TRACERS)
					{
						(*o)->Aspect->RenderTracers( &renderer, 0 );
					}
					if ( inst_attr & nema::RENDER_FROZEN)
					{
						(*o)->Aspect->RenderFrozen( &renderer, 0 );
					}
				}

				// Increment to next object
				++o;
			}
			else
			{
				break;
			}
		}

		renderer.EnableAlphaBlending( bAlphaBlend );

		// If we were force generating shadows, clear the state
		m_bShadowGenerate	= false;

		// Reset world matrix to the identity
		renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
	}
}

// Render all nodes in minimap format
void SiegeWalker::RenderVisibleMiniNodes( const int left, const int right, const int top, const int bottom )
{
	// Get the currently active render frustum
	SiegeFrustum* pRenderFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pRenderFrustum )
	{
		// Zoom control
		if( m_bMiniZoomIn )
		{
			m_MiniMeters	-= m_MiniMetersZoomPerSecond * (float)gSiegeEngine.GetAbsoluteDeltaFrameTime();
		}
		if( m_bMiniZoomOut )
		{
			m_MiniMeters	+= m_MiniMetersZoomPerSecond * (float)gSiegeEngine.GetAbsoluteDeltaFrameTime();
		}
		clamp_min_max( m_MiniMeters, m_MiniMinMeters, m_MiniMaxMeters );

		Rapi& renderer	= gSiegeEngine.Renderer();

		// Build camera based orientation
		matrix_3x3 orient( DoNotInitialize );
		vector_3 cam_dir	= gSiegeEngine.GetCamera().GetVectorOrientation();
		cam_dir.y			= 0.0f;

		if( !cam_dir.IsZero() )
		{
			orient			= MatrixFromDirection( cam_dir );
		}
		else
		{
			orient			= MatrixFromDirection( -vector_3::UP );
		}

		// Generate offset for centering
		vector_3 offset			= gSiegeEngine.GetCamera().GetTargetPosition();

		// Generate a world space clipping volume that corresponds to the screen space clipping volume
		float metersPerPixel	= m_MiniMeters / (float)max( right - left, bottom - top );
		float xdist				= ((float)(right - left) * metersPerPixel) * 0.5f;
		float ydist				= ((float)(bottom - top) * metersPerPixel) * 0.5f;

		oriented_bounding_box_3 clipBox( vector_3::ZERO, vector_3( xdist, 1000.0f, ydist ), matrix_3x3::IDENTITY );

		// Disable fog
		bool bFog = renderer.SetFogActivatedState( false );

		// Setup orthographic view
		renderer.SetOrthoMatrix( metersPerPixel, 0.1f, 200.0f );

		float trackingDistance	= 100.0f;
		vector_3 megamapCamPos	= offset + vector_3( 0.0f, trackingDistance, 0.0f );
		renderer.SetViewMatrix( megamapCamPos, offset, orient.GetColumn2_T() );

		// Build the mouse position in world space
		vector_3 mouseWorldPos	= gSiegeEngine.GetMouseShadow().ScreenPosition();
		mouseWorldPos.x			-= gSiegeEngine.GetCamera().GetViewportWidth() * 0.5f;
		mouseWorldPos.y			-= gSiegeEngine.GetCamera().GetViewportHeight() * 0.5f;

		mouseWorldPos.x			= -mouseWorldPos.x * metersPerPixel;
		mouseWorldPos.z			= -mouseWorldPos.y * metersPerPixel;
		mouseWorldPos.y			= 50.0f;

		mouseWorldPos			= (orient * mouseWorldPos) + offset;

		// Frustum radius
		float innerSquaredDiscoveryRadius		= pRenderFrustum->GetWidth() * m_innerFrustumScale;
		innerSquaredDiscoveryRadius		*= innerSquaredDiscoveryRadius;

		// Get the render list from this frustum
		FrustumNodeColl& nodeColl		= pRenderFrustum->GetFrustumNodeColl();

		// Clear our node collections
		m_NodeColl.clear();
		m_AlphaNodeColl.clear();

		m_ObjectColl.clear();
		m_ObjectList.clear();
		m_ObjectAlphaList.clear();

		bool bAlphaBlend = renderer.EnableAlphaBlending( false );

		if( gSiegeEngine.GetOptions().DrawDiscoveryFog() )
		{
			// Clear ZBuffer to back and set screen to black
			renderer.GetDevice()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 0.0f, 0 );

			sVertex sVerts[4];
			memset( sVerts, 0, sizeof( sVertex ) * 4 );

			renderer.SetTextureStageState(	0,
											D3DTOP_SELECTARG1,
											D3DTOP_SELECTARG1,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_NONE,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											true );

			// Render all of the stencil cutouts associated with this group of nodes
			renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC,				D3DCMP_ALWAYS );
			renderer.GetDevice()->SetRenderState( D3DRS_ALPHAREF,			0x00000080 );

			sVerts[0].x		= -gSiegeEngine.m_DiscoveryRadius;
			sVerts[0].y		= 0.0f;
			sVerts[0].z		= gSiegeEngine.m_DiscoveryRadius;
			sVerts[0].uv.u	= 0.0f;
			sVerts[0].uv.v	= 1.0f;
			sVerts[0].color	= 0x00000000;

			sVerts[1].x		= gSiegeEngine.m_DiscoveryRadius;
			sVerts[1].y		= 0.0f;
			sVerts[1].z		= gSiegeEngine.m_DiscoveryRadius;
			sVerts[1].uv.u	= 1.0f;
			sVerts[1].uv.v	= 1.0f;
			sVerts[1].color	= 0x00000000;

			sVerts[2].x		= -gSiegeEngine.m_DiscoveryRadius;
			sVerts[2].y		= 0.0f;
			sVerts[2].z		= -gSiegeEngine.m_DiscoveryRadius;
			sVerts[2].uv.u	= 0.0f;
			sVerts[2].uv.v	= 0.0f;
			sVerts[2].color	= 0x00000000;

			sVerts[3].x		= gSiegeEngine.m_DiscoveryRadius;
			sVerts[3].y		= 0.0f;
			sVerts[3].z		= -gSiegeEngine.m_DiscoveryRadius;
			sVerts[3].uv.u	= 1.0f;
			sVerts[3].uv.v	= 0.0f;
			sVerts[3].color	= 0x00000000;

			DiscoveryPosColl& discoveryColl = pRenderFrustum->GetDiscoveryPosColl();
			for( DiscoveryPosColl::iterator d = discoveryColl.begin(); d != discoveryColl.end(); ++d )
			{
				SiegeNode* pNode = gSiegeEngine.IsNodeInAnyFrustum( (*d).node );
				if( (pNode && (pNode->GetNodeAlpha() == 0xFF)) && !gSiegeEngine.SpaceDiffers( (*d).node, pRenderFrustum->GetPosition().node ) )
				{
					// Set the proper position and orientation of this discovery position
					vector_3 north		= gSiegeHotpointDatabase.GetHotpointDirection( L"north", (*d).node );
					if( north.IsZero() )
					{
						north			= vector_3::NORTH;
					}

					// Get the world space position of this discovery pos
					vector_3 worldDiscoveryPos	= pNode->NodeToWorldSpace( (*d).pos );
					renderer.SetWorldMatrix( MatrixFromDirection( north ), worldDiscoveryPos );

					if( Length2( worldDiscoveryPos - offset ) < innerSquaredDiscoveryRadius )
					{
						renderer.SetMinimumZ( 1.0f );
						renderer.SetSingleStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
					}
					else
					{
						renderer.SetMaximumZ( 0.0f );
						renderer.SetSingleStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
					}

					renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, &sVerts, 4, SVERTEX, &m_DiscoveryTex, 1 );
				}
			}

			renderer.SetMinimumZ( 0.0f );
			renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC,		D3DCMP_LESSEQUAL );
			renderer.GetDevice()->SetRenderState( D3DRS_ALPHAREF,	0x00000000 );
		}
		else
		{
			// Clear ZBuffer to back and set screen to black
			renderer.GetDevice()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0 );
		}

		// Calculate offset
		int fadeAmount	= FTOL( (255.0f - (float)gSiegeEngine.m_NodeFadeBlack) *
								max_t( 0.0f, min_t( 1.0f, (float)gSiegeEngine.GetAbsoluteDeltaFrameTime() / gSiegeEngine.m_NodeFadeTime ) ) );

		// Render all of the SiegeNodes and setup the object list
		for( FrustumNodeColl::iterator i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			// Light and render the current node
			if( LightMiniSiegeNode( *(*i), clipBox, orient, offset ) )
			{
				if( (*i)->GetCameraFade() )
				{
					// Calculate direction of the camera
					vector_3 ray_origin		= (*i)->WorldToNodeSpace( megamapCamPos );
					float ray_min			= FLOAT_MIN;
					vector_3 face_normal( DoNotInitialize );

					// Check for intersection
					if( (*i)->HitTest( ray_origin, -vector_3::UP, ray_min, face_normal, LF_ALL ) &&
						((ray_min < trackingDistance) && (ray_min > 0.0f)) )
					{
						// Calculate the passed time interval and set the camera alpha
						(*i)->SetCameraNodeAlpha( (DWORD)max_t( (int)gSiegeEngine.m_NodeFadeBlack, (int)(*i)->m_CameraAlpha -  fadeAmount) );
					}
					else
					{
						(*i)->SetCameraNodeAlpha( (DWORD)min_t( (int)255, (int)(*i)->m_CameraAlpha + fadeAmount ) );
					}

					if( !(*i)->GetNodeAlpha() )
					{
						continue;
					}
				}

				// Put this node on our lists for sorting
				if( (*i)->GetNodeAlpha() == 0xFF )
				{
					m_NodeColl.push_back( (*i) );
				}

				m_AlphaNodeColl.push_back( (*i) );
			}
		}

		for( SiegeNodeColl::iterator n = m_NodeColl.begin(); n != m_NodeColl.end(); ++n )
		{
			// Render the node
			RenderMiniSiegeNode( *(*n), false, mouseWorldPos );
		}

		renderer.EnableAlphaBlending( true );

		for( n = m_AlphaNodeColl.begin(); n != m_AlphaNodeColl.end(); ++n )
		{
			// Render the node
			if( (*n)->GetNodeAlpha() < 255 )
			{
				RenderMiniSiegeNode( *(*n), false, mouseWorldPos );
			}

			RenderMiniSiegeNode( *(*n), true, mouseWorldPos );
		}

		// Render the objects
		if( gSiegeEngine.GetOptions().DrawObjects() )
		{
			RenderMiniWorldObjects( orient, metersPerPixel );
		}

		renderer.EnableAlphaBlending( bAlphaBlend );

		// Re-enable fogging
		renderer.SetFogActivatedState( bFog );

		// Reset world matrix to the identity
		renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
		renderer.SetMaximumZ( 1.0f );
	}
}

// Light and clip a SiegeNode
bool SiegeWalker::LightSiegeNode( SiegeNode& node )
{
	GPPROFILERSAMPLE( "SiegeWalker::LightSiegeNode()", SP_DRAW | SP_DRAW_CALC_TERRAIN );

	// Get the mesh
	SiegeMesh& bmesh = node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

	// Do fading control on the node
	if( !gSiegeEngine.Renderer().IsIgnoringVertexColor() )
	{
		gSiegeEngine.UpdateFadedNodeAlpha( node );
	}

	// Don't bother continuing if the node can't be visible
	if( (!node.GetNodeAlpha() && !node.GetCameraFade()) && !gSiegeEngine.GetOptions().DrawFadedNodes() )
	{
		node.SetVisibleThisFrame( false );
		gSiegeEngine.m_CulledNodeCount++;
		return false;
	}

	if( !node.GetVisible() && !gSiegeEngine.GetOptions().DrawFadedNodes() )
	{
		node.SetVisibleThisFrame( false );
		gSiegeEngine.m_CulledNodeCount++;
		return false;
	}

	// Do visibility check
	if( gSiegeEngine.GetCamera().GetFrustum().CullSphere( node.GetWorldSpaceSphereCenter(), node.GetSphereRadius() ) )
	{
		// This bmesh is not on screen
		node.SetVisibleThisFrame(false);
		gSiegeEngine.m_CulledNodeCount++;

		if( node.LoadedLastUpdate() )
		{
			node.SetLoadedLastUpdate( false );
		}

		// $$$ Go through this node's occupants and put flying objects into the draw list
		return false;
	}
	else
	{
		gSiegeEngine.m_RenderedNodeCount++;
		node.SetVisibleThisFrame(true);

		if( node.LoadedLastUpdate() )
		{
//			m_bForceLoadNeeded	= true;
			node.SetLoadedLastUpdate( false );
		}
	}

	// Light the SiegeNode
	if( node.IsLightAccumRequested() )
	{
		// Accumulate static
		node.AccumulateStaticLighting();
		node.RequestLightingAccum( false );

		// Compute lights
		node.ComputeLighting( bmesh );
		node.SetLitThisFrame( true );

		if( node.GetNumLitVertices() == bmesh.GetNumberOfVertices() )
		{
			// Update decal lighting
			gSiegeDecalDatabase.UpdateDecalLighting( node );
		}
	}
	else if( node.ComputeLighting( bmesh ) )
	{
		node.SetLitThisFrame( true );

		// Update decal lighting
		gSiegeDecalDatabase.UpdateDecalLighting( node );
	}
	else
	{
		node.SetLitThisFrame( false );
	}

	return true;
}

//*****************************

void SiegeWalker::RenderSiegeNode( SiegeNode& node, bool alpha, bool noalphasort )
{
	GPPROFILERSAMPLE( "SiegeWalker::RenderSiegeNode()", SP_DRAW | SP_DRAW_TERRAIN );

	// Are you ready to RENDER!?
	Rapi& renderer		= gSiegeEngine.Renderer();

	if( node.HasBoundaryMesh() )
	{
		bool ignoreState			= renderer.IsIgnoringVertexColor();

		// Get the mesh
		SiegeMesh& bmesh			= node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

		// Setup the matrix for this node
		renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );

		// If we are an alphastage, render our alpha components and return
		if( alpha && node.GetNodeAlpha() )
		{
			bmesh.RenderAlpha( node.GetTextureListing(), (DWORD*)node.m_pVertexColors, noalphasort );
			return;
		}

		// Get geometry info
		int numtris					= bmesh.GetNumberOfTriangles();
		int numverts				= bmesh.GetNumberOfVertices();

		// Trace the mouse position into this node
		vector_3 ray_origin		= node.WorldToNodeSpace( gSiegeEngine.GetCamera().GetCameraPosition() );
		vector_3 ray_direction	= node.GetTransposeOrientation() * ( gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );

		float ray_min			= FLOAT_MIN;
		vector_3 face_normal( DoNotInitialize );

		if( node.HitTest( ray_origin, ray_direction, ray_min, face_normal, LF_HUMAN_PLAYER ) )
		{
			if( ray_min > 0.0f )
			{
				float distToHit	= ray_direction.Length() * ray_min;

				if( distToHit < gSiegeEngine.GetMouseShadow().GetTerrainHitDistance() )
				{
					// Set the new hit distance
					gSiegeEngine.GetMouseShadow().SetFloorHitDist( distToHit );

					SiegePos hitPos;
					hitPos.pos	= ray_origin + ( ray_direction * ray_min );
					hitPos.node = node.GetGUID();
					gSiegeEngine.GetMouseShadow().SetFloorHitPos( hitPos );

					// Add in the selected statistics to the engine
					gSiegeEngine.m_SelectedSiegeTrisRendered	= numtris;
					gSiegeEngine.m_SelectedSiegeVertsRendered	= numverts;
					gSiegeEngine.m_SelectedSiegeNodeName		= node.GetGUID().ToString();

					// Draw a box if requested
					if( gSiegeEngine.GetOptions().NodeBoxes() )
					{
						RP_DrawBox( gSiegeEngine.Renderer(),
									gSiegeEngine.GetMouseShadow().GetFloorHitPos().pos - vector_3( 0.2f, 0.2f, 0.2f ),
									gSiegeEngine.GetMouseShadow().GetFloorHitPos().pos + vector_3( 0.2f, 0.2f, 0.2f ),
									0xFFFFFFFF );
					}
				}
			}
		}

		// Setup flags for rendering
		int flag  = 0;
#if !GP_RETAIL

		if( gSiegeEngine.GetOptions().DrawFloors() )
		{
			flag |= SiegeMesh::FLAG_DRAW_FLOORS;
		}
		if( gSiegeEngine.GetOptions().IsNormalDrawingEnabled() )
		{
			flag |= SiegeMesh::FLAG_DRAW_NORMALS;
		}
		if( gSiegeEngine.GetOptions().DrawWater() )
		{
			flag |= SiegeMesh::FLAG_DRAW_WATER;
		}

#endif

		// Render the mesh
		bmesh.Render( flag, node.GetTextureListing(), (DWORD*)node.m_pVertexColors,
					  gSiegeEngine.GetOptions().DrawFadedNodes() ? 0xFF : node.GetNodeAlpha() );

		// Render decals
		unsigned int decal_tris	= gSiegeDecalDatabase.RenderNodeDecals( node );

		if( gDefaultRapi.IsTextureStateReset() )
		{
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
			gDefaultRapi.SetTextureStageState(	0,
												D3DTOP_SELECTARG2,
												D3DTOP_SELECTARG2,
												D3DTADDRESS_CLAMP,
												D3DTADDRESS_CLAMP,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DBLEND_SRCALPHA,
												D3DBLEND_INVSRCALPHA,
												false );
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
		}

#if !GP_RETAIL

		if( gSiegeEngine.GetOptions().IsDrawLogicalNodes() )
		{
			bool bFogState		= renderer.SetFogActivatedState( false );
			bool bIgnoreState	= renderer.IsIgnoringVertexColor();
			renderer.IgnoreVertexColor( false );

			for( unsigned int ln = 0; ln < node.GetNumLogicalNodes(); ++ln )
			{
				RP_DrawBox( renderer, node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetMinimumBounds(), node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetMaximumBounds(), 0xFF00FF00 );

				for( unsigned int leaf = 0; leaf < node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetNumLeafConnections(); ++leaf )
				{
					if( node.GetLogicalNodes()[ ln ]->IsLeafBlocked( node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].id ) )
					{
						RP_DrawBox( renderer, node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].minBox, node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].maxBox, 0xFFFF0000 );
					}
					else
					{
						RP_DrawBox( renderer, node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].minBox, node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].maxBox, 0xFF0000FF );
						RP_DrawLine( renderer,
									 node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].center,
									 node.GetLogicalNodes()[ ln ]->GetLogicalMesh()->GetLeafConnectionInfo()[ leaf ].center + vector_3( 0.0f, 0.2f, 0.0f ),
									 0xFF00FFFF );
					}
				}

				for( unsigned int lnc = 0; lnc < node.GetLogicalNodes()[ ln ]->GetNumNodeConnections(); ++lnc )
				{
					SiegeNode* pFarNode = gSiegeEngine.IsNodeValid( node.GetLogicalNodes()[ ln ]->GetNodeConnectionInfo()[ lnc ].m_farSiegeNode );
					if( pFarNode )
					{
						RP_DrawRay( renderer, node.WorldToNodeSpace( pFarNode->NodeToWorldSpace( pFarNode->GetLogicalNodes()[ node.GetLogicalNodes()[ ln ]->GetNodeConnectionInfo()[ lnc ].m_pCollection->m_farid ]->GetLogicalMesh()->GetCenter() ) ), 0xFFFF0000 );
					}
				}
			}

			renderer.SetFogActivatedState( bFogState );
			renderer.IgnoreVertexColor( bIgnoreState );
		}

		// Add statistics into the engine
		gSiegeEngine.m_TotalTrisRendered	+= numtris;
		gSiegeEngine.m_SiegeTrisRendered	+= numtris;

		gSiegeEngine.m_TotalVertsRendered	+= numverts;
		gSiegeEngine.m_SiegeVertsRendered	+= numverts;

		gSiegeEngine.m_TotalTrisRendered	+= decal_tris;
		gSiegeEngine.m_DecalTrisRendered	+= decal_tris;

		// Draw doors for this node if requested
		if( gSiegeEngine.GetOptions().IsDrawDoorsEnabled() )
		{
			// Draw the doors for this node
			// We draw doors that have neighbors in green, and doors that don't
			// in red.
			bool fogState	= renderer.SetFogActivatedState( false );
			renderer.IgnoreVertexColor( false );

			renderer.SetMaximumZ( 0.0f );
			renderer.SetTextureStageState(	0,
											D3DTOP_SELECTARG2,
											D3DTOP_SELECTARG2,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );

			for( siege::SiegeMesh::SiegeMeshDoorConstIter bdi = bmesh.GetDoors().begin(); bdi != bmesh.GetDoors().end(); ++bdi )
			{
				unsigned int numIndices		= (*bdi)->GetVertexIndices().size();

				if( !numIndices )
				{
					continue;
				}

				unsigned int	startIndex;
				sVertex*		pVerts;
				if( renderer.LockBufferedVertices( numIndices, startIndex, pVerts ) )
				{
					DWORD color	= 0xFFFF0000;

					SiegeNodeDoor* pSpaceDoor	= node.GetDoorByID( (*bdi)->GetID() );
					if( pSpaceDoor )
					{
						if( pSpaceDoor->GetNeighbour() == UNDEFINED_GUID )
						{
							color	= 0xFFFF0000;
						}
						else if( pSpaceDoor->IsInaccurate() )
						{
							color	= 0xFFFFFF00;
						}
						else
						{
							color	= 0xFF00FF00;
						}
					}

					for( unsigned int j = 0; j < numIndices; ++j )
					{
						pVerts[ j ]			= bmesh.GetVertices()[ (*bdi)->GetVertexIndices()[ j ] ];
						pVerts[ j ].color	= color;
					}

					renderer.UnlockBufferedVertices();

					renderer.DrawBufferedPrimitive( D3DPT_LINESTRIP, startIndex, numIndices );
				}
			}

			renderer.IgnoreVertexColor( ignoreState );
			renderer.SetFogActivatedState( fogState );

			renderer.SetMaximumZ( 1.0f );
		}

		// Draw space tree if requested
		if( gSiegeEngine.GetOptions().IsSpaceTreeEnabled() )
		{
			if( node.m_SpatialParent.IsValid() )
			{
				SiegeNodeHandle parent_handle	= gSiegeEngine.NodeCache().UseObject( node.m_SpatialParent );
				SiegeNode& parent_node			= parent_handle.RequestObject( gSiegeEngine.NodeCache() );

				vector_3 node_linepos			= node.GetCurrentCenter() + vector_3::UP;
				vector_3 parent_linepos			= parent_node.GetCurrentCenter() + vector_3::UP;

				gSiegeEngine.GetWorldSpaceLines().DrawLine( node.GetCurrentCenter(), node_linepos );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( node_linepos, parent_linepos );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( parent_node.GetCurrentCenter(), parent_linepos );

				matrix_3x3 line_orient			= MatrixFromDirection( node_linepos - parent_linepos );
				vector_3 center_line			= parent_linepos + ( ( node_linepos - parent_linepos ) * 0.5f );
				vector_3 point_line				= center_line + ( line_orient.GetColumn2_T() * 0.25f );

				gSiegeEngine.GetWorldSpaceLines().DrawLine( point_line, center_line - (line_orient.GetColumn0_T() * 0.25f) );
				gSiegeEngine.GetWorldSpaceLines().DrawLine( point_line, center_line + (line_orient.GetColumn0_T() * 0.25f) );
			}
		}

#endif
	}
}

// Light and deform a single object
void SiegeWalker::LightAndDeformObject( ASPECTINFO& info )
{
	GPPROFILERSAMPLE( "Nema object light and deform", SP_DRAW | SP_DRAW_CALC_GO );

	bool shadowAndLight	= false;

	// Update the aspect and prepare for any rendering
	nema::Aspect* aspect = info.Aspect;
	if( aspect->Deform(0,0) || (aspect->GetLightVisit() != m_LastLightVisit) )
	{
		shadowAndLight	= true;
	}
	else if( (info.pNode->GetLitThisFrame() || info.Flags & CF_POSORIENTCHANGED)					  ||
			 (info.Flags & CF_ITEM  && aspect->GetAmbience() != info.pNode->GetObjectAmbientColor()) ||
			 (info.Flags & CF_ACTOR && aspect->GetAmbience() != info.pNode->GetActorAmbientColor())  ||
			 (info.Flags & CF_GIZMO && aspect->GetAmbience() != 0xFFFFFFFF) ||
			 (info.Alpha != aspect->GetAlpha()) )
	{
		shadowAndLight	= true;
	}
	aspect->SetLightVisit( m_CurrentLightVisit );

	const SiegeNode& node = *info.pNode;

	// Get the mesh
	SiegeMesh& mesh		= node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

	// Light setup
	const matrix_3x3 inv_local_orientation	= Transpose( info.Orientation);
 	const vector_3 inv_local_origin			= -info.Origin;

	// Go through the static lights
	if( gSiegeEngine.GetOptions().IsStaticLightingEnabled() )
	{
		for( SiegeLightList::const_iterator s = node.GetStaticLights().begin(); s != node.GetStaticLights().end(); ++s )
		{
			if( !(*s).m_Descriptor.m_bAffectsActors && (info.Flags & CF_ACTOR) )
			{
				continue;
			}
			if( !(*s).m_Descriptor.m_bAffectsItems && (info.Flags & CF_ITEM) )
			{
				continue;
			}

			LIGHTINFO li;
			li.pLight				= &((*s).m_Descriptor);
			li.color				= li.pLight->m_Color;

			if( !li.pLight->m_bActive )
			{
				continue;
			}

			if( li.pLight->m_bOccludeGeometry && (*s).m_pLightEffect )
			{
				if( (info.RenderIndex1 != info.RenderIndex2) &&
					(info.RenderIndex2 != info.RenderIndex3) &&
					(info.RenderIndex1 != info.RenderIndex3) )
				{
					bool bIndex1IsZero = IsZero( (*s).m_pLightEffect[ info.RenderIndex1 ] ) ||
										 (*s).m_pLightEffect[ info.RenderIndex1 ] > (1.0f + FLOAT_TOLERANCE);
					bool bIndex2IsZero = IsZero( (*s).m_pLightEffect[ info.RenderIndex2 ] ) ||
										 (*s).m_pLightEffect[ info.RenderIndex2 ] > (1.0f + FLOAT_TOLERANCE);
					bool bIndex3IsZero = IsZero( (*s).m_pLightEffect[ info.RenderIndex3 ] ) ||
										 (*s).m_pLightEffect[ info.RenderIndex3 ] > (1.0f + FLOAT_TOLERANCE);

					if( bIndex1IsZero && bIndex2IsZero && bIndex3IsZero )
					{
						continue;
					}
					else if( bIndex1IsZero || bIndex2IsZero || bIndex3IsZero )
					{
						vector_3 segmentvA( DoNotInitialize );
						vector_3 segmentvB( DoNotInitialize );
						vector_3 segmentvC( DoNotInitialize );

						bool bDarkPrim		= false;

						if( bIndex1IsZero )
						{
							if( bIndex2IsZero )
							{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));

								bDarkPrim	= true;
							}
							else if( bIndex3IsZero )
							{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));

								bDarkPrim	= true;
							}
							else
							{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));

								bDarkPrim	= false;
							}
						}
						else if( bIndex2IsZero )
						{
							if( bIndex3IsZero )
							{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));

								bDarkPrim	= true;
							}
							else
							{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));

								bDarkPrim	= false;
							}
						}
						else if( bIndex3IsZero )
						{
								segmentvA	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex1 ]));
								segmentvB	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex2 ]));
								segmentvC	= *((vector_3*)(&mesh.GetVertices()[ info.RenderIndex3 ]));

								bDarkPrim	= false;
						}

						vector_3 intPoint	= ClosestPointOnLine( segmentvA, segmentvB - segmentvA, info.Origin );

						float distToInt		= Length2( info.Origin - intPoint );
						float distTovC		= Length2( info.Origin - segmentvC );
						float distTotal		= distToInt + distTovC;

						if( bDarkPrim )
						{
							li.color		= fScaleDWORDColor( li.color, (distToInt / distTotal) );
						}
						else
						{
							li.color		= fScaleDWORDColor( li.color, (distTovC / distTotal) );
						}
					}
				}
			}

			if( li.pLight->m_Type == LT_POINT )
			{
				li.nodalPosition	= (*s).m_Position;
				li.localPosition	= inv_local_orientation * ((*s).m_Position + inv_local_origin);
			}
			else if( li.pLight->m_Type == LT_DIRECTIONAL )
			{
				li.nodalPosition	= (*s).m_Direction * 10.0f;
				li.localPosition	= Normalize( (inv_local_orientation * (*s).m_Direction) );
			}
			else if ( li.pLight->m_Type == LT_SPOT )
			{
				if( PointInCone( (*s).m_Position, (*s).m_Direction, li.pLight->m_OuterRadius, info.Origin ) )
				{
					// Calculate a local position to this object
					li.nodalPosition	= (*s).m_Position;
					li.localPosition	= inv_local_orientation * ((*s).m_Position + inv_local_origin);
				}
				else
				{
					continue;
				}
			}
			else
			{
				gpwarning("Illegal static light type in node");
				continue;
			}

			m_StaticLightCache.push_back( li );
		}
	}

	// Go through the dynamic lights
	if( gSiegeEngine.GetOptions().IsDynamicLightingEnabled() && (info.Flags & CF_DYNAMICLIGHT) )
	{
		for( SiegeLightList::const_iterator d = node.GetDynamicLights().begin(); d != node.GetDynamicLights().end(); ++d )
		{
			if( !(*d).m_Descriptor.m_bAffectsActors && (info.Flags & CF_ACTOR) )
			{
				continue;
			}
			if( !(*d).m_Descriptor.m_bAffectsItems && (info.Flags & CF_ITEM) )
			{
				continue;
			}

			LIGHTINFO li;
			li.pLight				= &((*d).m_Descriptor);
			li.color				= li.pLight->m_Color;

			if( !li.pLight->m_bActive )
			{
				continue;
			}

			if( li.pLight->m_Type == LT_POINT )
			{
				li.nodalPosition	= (*d).m_Position;
				li.localPosition	= inv_local_orientation * ((*d).m_Position + inv_local_origin);
			}
			else if( li.pLight->m_Type == LT_DIRECTIONAL )
			{
				li.nodalPosition	= (*d).m_Direction * 10.0f;
				li.localPosition	= Normalize( (inv_local_orientation * (*d).m_Direction) );
			}
			else if ( li.pLight->m_Type == LT_SPOT )
			{
				if( PointInCone( (*d).m_Position, (*d).m_Direction, li.pLight->m_OuterRadius, info.Origin ) )
				{
					// Calculate a local position to this object
					li.nodalPosition	= (*d).m_Position;
					li.localPosition	= inv_local_orientation * ((*d).m_Position + inv_local_origin);
				}
				else
				{
					continue;
				}
			}
			else
			{
				gpwarning("Illegal dynamic light type in node");
				continue;
			}

			m_DynamicLightCache.push_back( li );
		}
	}

	// Shadow rendering
	if( info.Flags & CF_DRAW_SHADOW )
	{
		LIGHTINFO shadowLight;
		shadowLight.pLight	= NULL;
		bool bResetExpensiveShadows	= false;

		if( (gSiegeEngine.GetOptions().UseExpensivePartyShadows() && (info.Flags & CF_PARTYMEMBER)) &&
			!gSiegeEngine.GetOptions().UseExpensiveShadows() )
		{
			gSiegeEngine.GetOptions().ToggleExpensiveShadows();
			bResetExpensiveShadows	= true;
		}

		if( gSiegeEngine.GetOptions().UseExpensiveShadows() )
		{
			float squaredDist	= FLOAT_MAX;

			// Render the shadows for this aspect
			if( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() )
			{
				for( LightCache::iterator l = m_StaticLightCache.begin(); l != m_StaticLightCache.end(); ++l )
				{
					if( (*l).pLight->m_bDrawShadow )
					{
						float lightDist	= (*l).localPosition.Length2();
						if( lightDist < squaredDist )
						{
							if( shadowLight.pLight )
							{
								if( (*l).pLight->m_Type == LT_DIRECTIONAL || shadowLight.pLight->m_Type != LT_DIRECTIONAL )
								{
									shadowLight = (*l);
									squaredDist	= lightDist;
								}
							}
							else
							{
								shadowLight = (*l);
								squaredDist	= lightDist;
							}
						}
					}
				}
			}

			if( gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
			{
				for( LightCache::iterator l = m_DynamicLightCache.begin(); l != m_DynamicLightCache.end(); ++l )
				{
					if( (*l).pLight->m_bDrawShadow )
					{
						float lightDist	= (*l).localPosition.Length2();
						if( lightDist < squaredDist )
						{
							if( shadowLight.pLight )
							{
								if( (*l).pLight->m_Type == LT_DIRECTIONAL || shadowLight.pLight->m_Type != LT_DIRECTIONAL )
								{
									shadowLight = (*l);
									squaredDist	= lightDist;
								}
							}
							else
							{
								shadowLight = (*l);
								squaredDist	= lightDist;
							}
						}
					}
				}
			}
		}

		if( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() || gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
		{
			GenerateShadow( info, shadowLight, shadowAndLight || m_bShadowGenerate );
		}

		if( bResetExpensiveShadows )
		{
			gSiegeEngine.GetOptions().ToggleExpensiveShadows();
		}
	}

	// Shading
	if( shadowAndLight )
	{
		GPPROFILERSAMPLE( "Nema object shading", SP_DRAW | SP_DRAW_CALC_GO );

		// Initialize shading for this object
		DWORD objectAlpha;
		if( (info.Flags & CF_PARTYMEMBER) || (info.Flags & CF_GIZMO) )
		{
			objectAlpha	= info.Alpha;
		}
		else
		{
			objectAlpha	= min( node.GetNodeAlpha(), info.Alpha );
		}

		if( info.Flags & CF_GIZMO )
		{
			aspect->InitializeLighting( 0xFFFFFFFF, objectAlpha );
		}
		else if( info.Flags & CF_ACTOR )
		{
			aspect->InitializeLighting( node.GetActorAmbientColor(), objectAlpha );
		}
		else
		{
			aspect->InitializeLighting( node.GetObjectAmbientColor(), objectAlpha );
		}

		nema::LightSource nLight;

		// Shade the aspect
		if( gSiegeEngine.GetOptions().IsStaticShadingEnabled() )
		{
			for( LightCache::iterator l = m_StaticLightCache.begin(); l != m_StaticLightCache.end(); ++l )
			{
				// Build the Nema LightSource from the Siege Light Description
				nLight.m_Color				= (*l).color;
				nLight.m_Intensity			= (*l).pLight->m_Intensity;

				vector_3 lightdir(0,-1,0);
				if( !(*l).localPosition.IsZero() )
				{
					lightdir				= Normalize((*l).localPosition);
				}

				if( (*l).pLight->m_Type == LT_DIRECTIONAL )
				{
					nLight.m_Type			= nema::NLS_INFINITE_LIGHT;
					nLight.m_Direction		= lightdir;
				}
				else if( (*l).pLight->m_Type == LT_SPOT )
				{
					nLight.m_Type			= nema::NLS_SPOT_LIGHT;
					nLight.m_Position		= (*l).localPosition;
					nLight.m_Direction		= lightdir;
					nLight.m_InnerCone		= PI/5;
					nLight.m_OuterCone		= PI/4;
				}
				else if( (*l).pLight->m_Type == LT_POINT )
				{
					nLight.m_Type			= nema::NLS_POINT_LIGHT;
					nLight.m_Position		= (*l).localPosition;
					nLight.m_InnerRadius	= (*l).pLight->m_InnerRadius;
					nLight.m_OuterRadius	= (*l).pLight->m_OuterRadius;
				}

				aspect->CalculateShading( nLight,gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
			}
		}
		if( gSiegeEngine.GetOptions().IsDynamicShadingEnabled() )
		{
			for( LightCache::iterator l = m_DynamicLightCache.begin(); l != m_DynamicLightCache.end(); ++l )
			{
				// Build the Nema LightSource from the Siege Light Description
				nLight.m_Color				= (*l).color;
				nLight.m_Intensity			= (*l).pLight->m_Intensity;

				vector_3 lightdir(0,-1,0);
				if( !(*l).localPosition.IsZero() )
				{
					lightdir				= Normalize((*l).localPosition);
				}

				if( (*l).pLight->m_Type == LT_DIRECTIONAL )
				{
					nLight.m_Type			= nema::NLS_INFINITE_LIGHT;
					nLight.m_Direction		= lightdir;
				}
				else if( (*l).pLight->m_Type == LT_SPOT )
				{
					nLight.m_Type			= nema::NLS_SPOT_LIGHT;
					nLight.m_Position		= (*l).localPosition;
					nLight.m_Direction		= lightdir;
					nLight.m_InnerCone		= PI/5;
					nLight.m_OuterCone		= PI/4;
				}
				else if( (*l).pLight->m_Type == LT_POINT )
				{
					nLight.m_Type			= nema::NLS_POINT_LIGHT;
					nLight.m_Position		= (*l).localPosition;
					nLight.m_InnerRadius	= (*l).pLight->m_InnerRadius;
					nLight.m_OuterRadius	= (*l).pLight->m_OuterRadius;
				}

				aspect->CalculateShading( nLight,gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
			}
		}
	}

	// Clear light caches
	m_StaticLightCache.clear();
	m_DynamicLightCache.clear();
}

// Render a single object
void SiegeWalker::RenderObject( ASPECTINFO& info )
{
	// Get the renderer
	Rapi& renderer		= gSiegeEngine.Renderer();

	const SiegeNode& node = *info.pNode;
	nema::Aspect* aspect = info.Aspect;

	const matrix_3x3 inv_local_orientation	= Transpose( info.Orientation);
 	const vector_3 inv_local_origin			= -info.Origin;

	// Setup the matrix
	renderer.TranslateWorldMatrix( info.Origin );
	renderer.RotateWorldMatrix( info.Orientation );
	renderer.ScaleWorldMatrix( vector_3( info.ObjectScale ) );

	// Get aspect information
	bool bGeneralHit	= false;
	bool bAccurateHit	= false;

	{
		GPPROFILERSAMPLE( "Nema object statistics and selection", SP_DRAW | SP_DRAW_GO );

		// Do stat counting
		int nematris	= 0;
		int nemaverts	= 0;
	
		for( unsigned int nm = 0, count = aspect->GetNumMeshes(); nm < count; ++nm )
		{
			int meshtris, meshverts;
			aspect->GetStats( nm, meshtris, meshverts );

			nematris	+= meshtris;
			nemaverts	+= meshverts;
		}

#if	!GP_RETAIL	
		// get the cost of rendering this object.
		gSiegeEngine.m_NemaCostRendered		+= aspect->GetCost();
#endif  // !GP_RETAIL

		gSiegeEngine.m_TotalTrisRendered	+= nematris;
		gSiegeEngine.m_NemaTrisRendered		+= nematris;

		gSiegeEngine.m_TotalVertsRendered	+= nemaverts;
		gSiegeEngine.m_NemaVertsRendered	+= nemaverts;

		if( (info.Flags & CF_SELECTABLE) || gSiegeEngine.GetOptions().SelectAllObjects() )
		{
			if( gSiegeEngine.GetMouseShadow().UseSelectFrustum() )
			{
				if( !gSiegeEngine.GetMouseShadow().SelectFrustum().CullObject( info.BoundOrient, info.BoundCenter, info.BoundHalfDiag ) )
				{
					// Signify hit
					bGeneralHit	= true;

#if !GP_RETAIL
					if( gSiegeEngine.GetOptions().NodeBoxes() )
					{
						renderer.PushWorldMatrix();
						renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
						renderer.TranslateWorldMatrix( info.BoundCenter );
						renderer.RotateWorldMatrix( info.BoundOrient );
						DebugDrawBox( vector_3(), info.BoundHalfDiag, vector_3(0,1,0.25f), renderer );

						renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
						renderer.TranslateWorldMatrix( info.Origin );
						renderer.RotateWorldMatrix( info.Orientation );

						for (int k = 0, count = aspect->GetNumberOfChildren(); k < count; k++)
						{
							nema::HAspect kid = aspect->GetChild(k);

							if (kid.IsValid())
							{
								renderer.PushWorldMatrix();
/*wart
								// Using the radius of the child box rather than orienting all
								// eight corners of its bbox and checking for the max/min.
								// Results in a bsphere that is larger that it needs to be
								int pb = kid->m_shared->m_primarybone;
								if (pb >= 0)
								{
									vector_3 p;
									Quat r;
									vector_3 s;
									kid->GetIndexedBoneOrientation(pb,p,r,s);
									// This is a rigid object located relative to the primary bone
									renderer.TranslateWorldMatrix( p );
									renderer.RotateWorldMatrix( r.BuildMatrix() );
								}
*/
								vector_3 bbc,bbd;
								Quat     bbo;

								kid->GetOrientedBoundingBox(bbo,bbc,bbd);
								renderer.TranslateWorldMatrix( bbc );
								renderer.RotateWorldMatrix( bbo.BuildMatrix() );

								DebugDrawBox( vector_3::ZERO, bbd, vector_3(0,1,0.25f), renderer );
								renderer.PopWorldMatrix();
							}
						}

						renderer.PopWorldMatrix();

					}
#endif
				}
			}
			else
			{
				// Get the mouse trace ray in local node space
				vector_3 ray_origin		= node.WorldToNodeSpace( gSiegeEngine.GetCamera().GetCameraPosition() );
				vector_3 ray_direction	= node.GetTransposeOrientation() * (  gSiegeEngine.GetCamera().GetMatrixOrientation() * gSiegeEngine.GetMouseShadow().CameraSpacePosition() );

				// Continue by moving the ray into local object space
				ray_origin				= inv_local_orientation * ( ray_origin + inv_local_origin );
				ray_direction			= inv_local_orientation * ray_direction;

				// Get the oriented information from the aspect
				vector_3 aspect_center;
				Quat	 aspect_orient;
				vector_3 aspect_half_diag;

				aspect->GetOrientedBoundingBox( aspect_orient, aspect_center, aspect_half_diag );
				aspect_half_diag	*= info.BoundingBodyScale;
				aspect_center		*= info.BoundingBodyScale;

				// Make sure that our half diagonal meets minimum size requirements
				vector_3 general_test( DoNotInitialize );
				general_test.x = max( aspect_half_diag.x, MIN_BOUND_SELECT_TEST );
				general_test.y = max( aspect_half_diag.y, MIN_BOUND_SELECT_TEST );
				general_test.z = max( aspect_half_diag.z, MIN_BOUND_SELECT_TEST );

				// Do one final space conversion into local aspect space
				Quat inv_asp_orient	= aspect_orient.Inverse();
				vector_3 local_origin( DoNotInitialize );
				inv_asp_orient.RotateVector( local_origin, ray_origin - aspect_center );
				vector_3 local_dir( DoNotInitialize );
				inv_asp_orient.RotateVector( local_dir, ray_direction );

				vector_3 coord( DoNotInitialize );
				if( RayIntersectsBox( -general_test, general_test, local_origin, local_dir, coord ) )
				{
					// Signify hit
					bGeneralHit	= true;

					// If this object has 1 or less bones
					if( aspect->GetNumBones() <= 1 )
					{
						if( RayIntersectsBox( -aspect_half_diag, aspect_half_diag, local_origin, local_dir, coord ) )
						{
							// Signify hit
							bAccurateHit	= true;
						}
					}
					else
					{
						// If we hit the main outer box, do a more accurate test with the bone boxes
						for( unsigned int bindex = 0, count = aspect->GetNumBones(); bindex < count; ++bindex )
						{
							vector_3	bonecenter( DoNotInitialize );
							vector_3	bonehalf_diag( DoNotInitialize );
							vector_3	unscaled_bonecenter( DoNotInitialize );
							vector_3	unscaled_bonehalf_diag( DoNotInitialize );
							Quat		boneorient( DoNotInitialize );

							if( aspect->GetOrientedBoneBoundingBox( bindex, boneorient, unscaled_bonecenter, unscaled_bonehalf_diag ) )
							{
								bonehalf_diag		= unscaled_bonehalf_diag * info.BoundingBodyScale;
								bonecenter			= unscaled_bonecenter * info.BoundingBodyScale;

								Quat invboneorient	= boneorient.Inverse();
								invboneorient.RotateVector( local_origin, ray_origin - bonecenter );
								invboneorient.RotateVector( local_dir, ray_direction );

								if( RayIntersectsBox( -bonehalf_diag, bonehalf_diag, local_origin, local_dir, coord ) )
								{
									// Signify hit
									bAccurateHit	= true;

#if !GP_RETAIL
									if( gSiegeEngine.GetOptions().NodeBoxes() )
									{
										renderer.PushWorldMatrix();
										renderer.TranslateWorldMatrix( unscaled_bonecenter );
										renderer.RotateWorldMatrix( boneorient.BuildMatrix() );
										DebugDrawBox( vector_3(), unscaled_bonehalf_diag, vector_3(0,1,0.75f), renderer );
										renderer.PopWorldMatrix();
									}
#endif

									break;
								}
							}
						}
					}
#if !GP_RETAIL
					if( gSiegeEngine.GetOptions().NodeBoxes() )
					{
						renderer.PushWorldMatrix();
						renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
						renderer.TranslateWorldMatrix( info.BoundCenter );
						renderer.RotateWorldMatrix( info.BoundOrient );
						DebugDrawBox( vector_3(), info.BoundHalfDiag, vector_3(0,1,0.9f), renderer );

						renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
						renderer.TranslateWorldMatrix( info.Origin );
						renderer.RotateWorldMatrix( info.Orientation );

						for (int k = 0, count = aspect->GetNumberOfChildren(); k < count; k++)
						{
							nema::HAspect kid = aspect->GetChild(k);

							if (kid.IsValid())
							{
								renderer.PushWorldMatrix();
								// Using the radius of the child box rather than orienting all
								// eight corners of its bbox and checking for the max/min.
								// Results in a bsphere that is larger that it needs to be
/* wart
								int pb = kid->m_shared->m_primarybone;
								if (pb >= 0)
								{
									vector_3 p;
									Quat r;
									vector_3 s;
									kid->GetIndexedBoneOrientation(pb,p,r,s);
									// This is a rigid object located relative to the primary bone
									renderer.TranslateWorldMatrix( p );
									renderer.RotateWorldMatrix( r.BuildMatrix() );
								}
*/
								vector_3 bbc,bbd;
								Quat     bbo;

								kid->GetOrientedBoundingBox(bbo,bbc,bbd);

								bbc *= info.BoundingBodyScale;

								renderer.TranslateWorldMatrix( bbc );
								renderer.RotateWorldMatrix( bbo.BuildMatrix() );

								DebugDrawBox( vector_3::ZERO, bbd, vector_3(0,1,0.9f), renderer );
								renderer.PopWorldMatrix();
							}
						}

						renderer.PopWorldMatrix();
					}
#endif
				}
			}

			if( bGeneralHit )
			{
				// This object was hit by the mouse selection frustum
				float hitdist = Length( info.BoundCenter - gSiegeEngine.GetCamera().GetCameraPosition() );

				// Add this object to the general hit list
				gSiegeEngine.GetMouseShadow().PushGeneralHit( info.Object, info.BoundCenter, hitdist );

				if( bAccurateHit )
				{
					// Add this object to our accurate hit list
					gSiegeEngine.GetMouseShadow().PushAccurateHit( info.Object, info.BoundCenter, hitdist );
				}

				// Add the stats
				gSiegeEngine.m_SelectedNemaTrisRendered		+= nematris;
				gSiegeEngine.m_SelectedNemaVertsRendered	+= nemaverts;
			}
		}
	}

	// Draw the shadow if necessary
	if( info.Flags & CF_DRAW_SHADOW )
	{
		if( gSiegeEngine.GetOptions().IsStaticShadowsEnabled() || gSiegeEngine.GetOptions().IsDynamicShadowsEnabled() )
		{
			RenderObjectShadow( info );

#if !GP_RETAIL
			if( gSiegeEngine.GetOptions().ShadowSpheres() )
			{
				renderer.PushWorldMatrix();

				// Calculate the accurate bounding sphere of this object and its children
				float sphere_radius;
				vector_3 sphere_center( DoNotInitialize );
				aspect->GetBoundingSphereIncludingChildren( sphere_center, sphere_radius ,info.ObjectScale);

				// Setup the matrix and draw the sphere
				renderer.SetWorldMatrix( info.pNode->GetCurrentOrientation(), info.pNode->GetCurrentCenter() );
				renderer.TranslateWorldMatrix( info.Origin + (info.Orientation * sphere_center) );
				RP_DrawSphere( renderer, sphere_radius );

				renderer.PopWorldMatrix();
			}
#endif
		}
	}

	{
		GPPROFILERSAMPLE( "Nema object rendering", SP_DRAW | SP_DRAW_GO );

		// Setup render states
		renderer.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_WRAP,
										D3DTADDRESS_WRAP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );

		// Render the aspect
		DWORD nemaflags = 0;

#if !GP_RETAIL
		if( gSiegeEngine.GetOptions().IsNormalDrawingEnabled() )
		{
			nemaflags |= nema::RENDER_NORMALS;
		}
#endif

		bool bIgnoreState	= renderer.IsIgnoringVertexColor();

		// Check for highlight
		if( info.Flags & CF_HIGHLIGHTED )
		{
			renderer.IgnoreVertexColor( true );
		}

		if ( !(info.Flags & CF_NORENDER) )
		{
			aspect->Render( &renderer,nemaflags );
		}

		if( gDefaultRapi.IsTextureStateReset() )
		{
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
			gDefaultRapi.SetTextureStageState(	0,
												D3DTOP_SELECTARG2,
												D3DTOP_SELECTARG2,
												D3DTADDRESS_CLAMP,
												D3DTADDRESS_CLAMP,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DBLEND_SRCALPHA,
												D3DBLEND_INVSRCALPHA,
												false );
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
		}
		renderer.IgnoreVertexColor( bIgnoreState );

#if !GP_RETAIL
		if( bAccurateHit && gSiegeEngine.GetOptions().ShowBones() )
		{
			for( DWORD b = 0, count = aspect->GetNumBones(); b < count; b++ )
			{
				vector_3 bonecenter,bonehalf_diag;
				Quat boneorient;

				if( aspect->GetOrientedBoneBoundingBox( b, boneorient, bonecenter, bonehalf_diag ) )
				{
					renderer.PushWorldMatrix();
					renderer.TranslateWorldMatrix( bonecenter );
					renderer.RotateWorldMatrix( boneorient.BuildMatrix() );
					DebugDrawBox( vector_3(), bonehalf_diag, vector_3(0,1,0.75f), renderer );	// green with lot o yellow
					renderer.PopWorldMatrix();

					aspect->GetIndexedBoneOrientation(b,bonecenter,boneorient);
					renderer.PushWorldMatrix();
					renderer.TranslateWorldMatrix( bonecenter );
					renderer.RotateWorldMatrix( boneorient.BuildMatrix() );
					RP_DrawOrigin(renderer,Length(bonehalf_diag));
					renderer.PopWorldMatrix();
				}
			}
		}
#endif
	}
}

// Light a SiegeNode for minimap purposes
bool SiegeWalker::LightMiniSiegeNode( SiegeNode& node, oriented_bounding_box_3& clip_box,
									  const matrix_3x3& orient, const vector_3& offset )
{
	// Don't continue if you can't see the node
	if( !node.GetVisible() )
	{
		node.SetVisibleThisFrame( false );
		gSiegeEngine.m_CulledNodeCount++;
		return false;
	}

	// Get the mesh
	SiegeMesh& bmesh = node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

	// Build bounding volume
	matrix_3x3 inv_orient( DoNotInitialize );
	orient.Transpose( inv_orient );
	oriented_bounding_box_3 node_box( inv_orient * (node.GetCurrentCenter() - offset + ( node.GetCurrentOrientation() * bmesh.BBoxCenter() )),
									  bmesh.BBoxHalfDiag(), node.GetCurrentOrientation() * inv_orient );

	// Clip this node
	if( !clip_box.CheckForContact( node_box ) )
	{
		// This bmesh is not on screen
		node.SetVisibleThisFrame(false);
		gSiegeEngine.m_CulledNodeCount++;

		if( node.LoadedLastUpdate() )
		{
			node.SetLoadedLastUpdate( false );
		}
		return false;
	}
	else
	{
		gSiegeEngine.m_RenderedNodeCount++;
		node.SetVisibleThisFrame(true);

		if( node.LoadedLastUpdate() )
		{
//			m_bForceLoadNeeded	= true;
			node.SetLoadedLastUpdate( false );
		}
	}

	// Do fading control on the node
	if( !gSiegeEngine.Renderer().IsIgnoringVertexColor() )
	{
		gSiegeEngine.UpdateFadedNodeAlpha( node );
	}

	// Don't bother continuing if the node can't be visible
	if( (!node.GetNodeAlpha() && !node.GetCameraFade()) && !gSiegeEngine.GetOptions().DrawFadedNodes() )
	{
		node.SetVisibleThisFrame( false );
		gSiegeEngine.m_CulledNodeCount++;
		return false;
	}

	// Go through occupants of this node and set them up for rendering
	gSiegeEngine.CollectObjects( m_ObjectColl, node, CF_ACTOR | CF_ITEM, RF_POSITION | RF_ORIENT | RF_SCALE | RF_SCREEN_PLAYER_ALIGN | RF_ALPHA );

	// Light the SiegeNode
	if( node.IsLightAccumRequested() )
	{
		// Accumulate static
		node.AccumulateStaticLighting();
		node.RequestLightingAccum( false );

		// Compute lights
		node.ComputeLighting( bmesh );
		node.SetLitThisFrame( true );

		if( node.GetNumLitVertices() == bmesh.GetNumberOfVertices() )
		{
			// Update decal lighting
			gSiegeDecalDatabase.UpdateDecalLighting( node );
		}
	}
	else if( node.ComputeLighting( bmesh ) )
	{
		node.SetLitThisFrame( true );

		// Update decal lighting
		gSiegeDecalDatabase.UpdateDecalLighting( node );
	}
	else
	{
		node.SetLitThisFrame( false );
	}

	return true;
}

// Render a SiegeNode for minimap purposes
void SiegeWalker::RenderMiniSiegeNode( SiegeNode& node, bool alpha, const vector_3& mouse_pos )
{
	// Are you ready to RENDER!?
	Rapi& renderer		= gSiegeEngine.Renderer();

	if( node.HasBoundaryMesh() )
	{
		// When rendering mini SiegeNodes, we need them to confine their Z-testing to the back half of the buffer.
		renderer.SetMinimumZ( 0.5f );

		// Setup the matrix for this node
		renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );

		// Get the mesh
		SiegeMesh& bmesh			= node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

		// If we are an alphastage, render our alpha components and return
		if( alpha )
		{
			bmesh.RenderAlpha( node.GetTextureListing(), (DWORD*)node.m_pVertexColors, true );
			bmesh.RenderAlpha( node.GetTextureListing(), (DWORD*)node.m_pVertexColors, false );
			return;
		}

		// Trace the mouse position into this node
		vector_3 ray_origin		= node.WorldToNodeSpace( mouse_pos );
		vector_3 ray_direction	= node.GetTransposeOrientation() * -vector_3::UP;

		float ray_min			= FLOAT_MIN;
		vector_3 face_normal( DoNotInitialize );

		if( node.HitTest( ray_origin, ray_direction, ray_min, face_normal, LF_IS_FLOOR ) )
		{
			if( ray_min > 0.0f )
			{
				float distToHit	= ray_direction.Length() * ray_min;

				if( distToHit < gSiegeEngine.GetMouseShadow().GetTerrainHitDistance() )
				{
					// Set the new hit distance
					gSiegeEngine.GetMouseShadow().SetFloorHitDist( distToHit );

					SiegePos hitPos;
					hitPos.pos	= ray_origin + ( ray_direction * ray_min );
					hitPos.node = node.GetGUID();
					gSiegeEngine.GetMouseShadow().SetFloorHitPos( hitPos );

					// Add in the selected statistics to the engine
					gSiegeEngine.m_SelectedSiegeNodeName		= node.GetGUID().ToString();

#if !GP_RETAIL
					// Draw a box if requested
					if( gSiegeEngine.GetOptions().NodeBoxes() )
					{
						RP_DrawBox( gSiegeEngine.Renderer(),
									gSiegeEngine.GetMouseShadow().GetFloorHitPos().pos - vector_3( 0.2f, 0.2f, 0.2f ),
									gSiegeEngine.GetMouseShadow().GetFloorHitPos().pos + vector_3( 0.2f, 0.2f, 0.2f ),
									0xFFFFFFFF );
					}
#endif
				}
			}
		}

		// Render the mesh
		bmesh.Render( 0, node.GetTextureListing(), (DWORD*)node.m_pVertexColors, node.GetNodeAlpha() );

		// Render the decals for this node
		gSiegeDecalDatabase.RenderNodeDecals( node );

		if( gDefaultRapi.IsTextureStateReset() )
		{
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
			gDefaultRapi.SetTextureStageState(	0,
												D3DTOP_SELECTARG2,
												D3DTOP_SELECTARG2,
												D3DTADDRESS_CLAMP,
												D3DTADDRESS_CLAMP,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DTEXF_LINEAR,
												D3DBLEND_SRCALPHA,
												D3DBLEND_INVSRCALPHA,
												false );
			gDefaultRapi.SetFogActivatedState( !gDefaultRapi.GetFogActivatedState() );
		}
	}
}

// Render the objects for minimap purposes
void SiegeWalker::RenderMiniWorldObjects( const matrix_3x3& orient, const float metersPerPixel )
{
	// Get the renderer
	Rapi& renderer					= gSiegeEngine.Renderer();

	// Build a good mouse positional box
	vector_3 mouseMinBox( DoNotInitialize );
	mouseMinBox.x	= min_t( gSiegeEngine.GetMouseShadow().ScreenBeginPosition().x, gSiegeEngine.GetMouseShadow().ScreenPosition().x );
	mouseMinBox.y	= min_t( gSiegeEngine.GetMouseShadow().ScreenBeginPosition().y, gSiegeEngine.GetMouseShadow().ScreenPosition().y );
	mouseMinBox.z	= -1.0f;

	vector_3 mouseMaxBox( DoNotInitialize );
	mouseMaxBox.x	= max_t( gSiegeEngine.GetMouseShadow().ScreenBeginPosition().x, gSiegeEngine.GetMouseShadow().ScreenPosition().x );
	mouseMaxBox.y	= max_t( gSiegeEngine.GetMouseShadow().ScreenBeginPosition().y, gSiegeEngine.GetMouseShadow().ScreenPosition().y );
	mouseMaxBox.z	= 1.0f;

	// Calculate the current desired world size of the icon bitmaps
	float defaultIconSize	= (metersPerPixel * 16.0f) * ((((m_MiniMaxMeters - m_MiniMeters) / (m_MiniMaxMeters - m_MiniMinMeters)) / 2.0f) + 0.5f);
	float doubleIconSize	= defaultIconSize * 2.0f;

	// Make sure that all objects render into the front half of the z-buffer
	gDefaultRapi.SetMaximumZ( 0.5f );

	// Iterate over all of the objects
	for( WorldObjectColl::iterator i = m_ObjectColl.begin(); i != m_ObjectColl.end(); ++i )
	{
		// Get the next aspect
		ASPECTINFO& info	= (*i);

		// No way we can draw if the icon is not correct
		if( !info.MegamapIconTex )
		{
			continue;
		}
		unsigned int tex	= info.MegamapIconTex;

		// See if this texture is override
		if( !(info.Flags & CF_MM_OVERRIDE) )
		{
			if( ( (info.Flags & CF_ITEM) && !(info.Flags & CF_SPAWNED) && (tex != 0xFFFFFFFF) ) ||						// spawned item
				( (info.Flags & CF_ACTOR) && !(info.Flags & CF_SCREEN_PLAYER_VIS) && !(info.Flags & CF_PARTYMEMBER) ) )	// screen player visible actors
			{
				continue;
			}
		}

		// Setup the matrix for this object
		vector_3 worldObjectPos	= info.pNode->NodeToWorldSpace( info.Origin );
		renderer.SetWorldMatrix( matrix_3x3::IDENTITY, worldObjectPos );

		if( info.Flags & CF_SELECTABLE )
		{
			if( gSiegeEngine.GetMouseShadow().UseSelectFrustum() )
			{
				// Select object using object position in mouse box test
				if( PointInBox( mouseMinBox, mouseMaxBox, renderer.GetScreenPosition( worldObjectPos ) ) )
				{
					// Add this object to the general hit list
					gSiegeEngine.GetMouseShadow().PushGeneralHit( info.Object, worldObjectPos, 0.0f );
				}
			}
			else
			{
				// Select object using mouse position in object box test
				vector_3 screenMinBox	= renderer.GetScreenPosition( worldObjectPos - (orient * vector_3( doubleIconSize, 0.0f, doubleIconSize )) );
				vector_3 screenMaxBox	= renderer.GetScreenPosition( worldObjectPos + (orient * vector_3( doubleIconSize, 0.0f, doubleIconSize )) );

				vector_3 objectMinBox( DoNotInitialize );
				objectMinBox.x	= min_t( screenMinBox.x, screenMaxBox.x );
				objectMinBox.y	= min_t( screenMinBox.y, screenMaxBox.y );
				objectMinBox.z	= -1.0f;

				vector_3 objectMaxBox( DoNotInitialize );
				objectMaxBox.x	= max_t( screenMinBox.x, screenMaxBox.x );
				objectMaxBox.y	= max_t( screenMinBox.y, screenMaxBox.y );
				objectMaxBox.z	= 1.0f;

				if( PointInBox( objectMinBox, objectMaxBox, gSiegeEngine.GetMouseShadow().ScreenPosition() ) )
				{
					// Add this object to the general hit list
					gSiegeEngine.GetMouseShadow().PushGeneralHit( info.Object, worldObjectPos, 0.0f );
				}
			}
		}

		DWORD scolor		= 0xFF0000FF;
		if( info.Flags & CF_PARTYMEMBER )
		{
			scolor			= 0xFF00FF00;
			if( !(info.Flags & CF_ALIVE) && !(info.Flags & CF_GHOST) )
			{
				tex			= m_TombstoneTex;
			}
		}
		else if( info.ScreenPlayerAlign == AF_FRIEND )
		{
			scolor			= 0xFF00FF00;
		}
		else if( info.ScreenPlayerAlign == AF_ENEMY )
		{
			scolor			= 0xFFFF0000;
		}

		sVertex sVerts[4];
		memset( sVerts, 0, sizeof( sVertex ) * 4 );

		sVerts[0].x		= -defaultIconSize;
		sVerts[0].y		= 0.0f;
		sVerts[0].z		= defaultIconSize;
		sVerts[0].uv.u	= 0.0f;
		sVerts[0].uv.v	= 1.0f;
		sVerts[0].color	= 0xFFFFFFFF;

		sVerts[1].x		= defaultIconSize;
		sVerts[1].y		= 0.0f;
		sVerts[1].z		= defaultIconSize;
		sVerts[1].uv.u	= 1.0f;
		sVerts[1].uv.v	= 1.0f;
		sVerts[1].color	= 0xFFFFFFFF;

		sVerts[2].x		= -defaultIconSize;
		sVerts[2].y		= 0.0f;
		sVerts[2].z		= -defaultIconSize;
		sVerts[2].uv.u	= 0.0f;
		sVerts[2].uv.v	= 0.0f;
		sVerts[2].color	= 0xFFFFFFFF;

		sVerts[3].x		= defaultIconSize;
		sVerts[3].y		= 0.0f;
		sVerts[3].z		= -defaultIconSize;
		sVerts[3].uv.u	= 1.0f;
		sVerts[3].uv.v	= 0.0f;
		sVerts[3].color	= 0xFFFFFFFF;

		// Set the render states
		renderer.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										true );

		if( tex == 0xFFFFFFFF )
		{
			// Render the aspect
			renderer.SetWorldMatrixRotation( info.pNode->GetCurrentOrientation() * info.Orientation );
			renderer.ScaleWorldMatrix( vector_3( info.ObjectScale ) );

			// Update the aspect and prepare for any rendering
			nema::Aspect* aspect = info.Aspect;
			aspect->Deform(0,0);

			// Initialize shading for this object
			DWORD objectAlpha;
			if( (info.Flags & CF_PARTYMEMBER) || (info.Flags & CF_GIZMO) )
			{
				objectAlpha	= info.Alpha;
			}
			else
			{
				objectAlpha	= min( info.pNode->GetNodeAlpha(), info.Alpha );
			}

			if( info.Flags & CF_ACTOR )
			{
				aspect->InitializeLighting( InterpolateColor( info.pNode->GetActorAmbientColor(), 0xFFFFFFFF, 0.25f ), objectAlpha );
			}
			else
			{
				aspect->InitializeLighting( InterpolateColor( info.pNode->GetObjectAmbientColor(), 0xFFFFFFFF, 0.25f ), objectAlpha );
			}

			aspect->Render( &renderer, 0 );

			// Reset the render states
			renderer.SetTextureStageState(	0,
											D3DTOP_MODULATE,
											D3DTOP_SELECTARG1,
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
			// Draw object
			if( info.Flags & CF_MM_ORIENT )
			{
				renderer.SetWorldMatrixRotation( info.pNode->GetCurrentOrientation() * info.Orientation );
			}
			else
			{
				renderer.SetWorldMatrixRotation( orient );
			}
			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, sVerts, 4, SVERTEX, &tex, 1 );
		}

		unsigned int ringTex	= m_SelectionRingTex;
		if( info.Flags & CF_PARTYMEMBER )
		{
			// Setup to draw the ring texture
			renderer.SetWorldMatrixRotation( info.pNode->GetCurrentOrientation() * info.Orientation );

			if( info.Flags & CF_FOCUSED )
			{
				ringTex	= m_OrientArrowTex;
			}
			else
			{
				ringTex = m_SelectionTriTex;
			}
		}

		// Draw colored selection ring
		if( info.Flags & CF_HIGHLIGHTED )
		{
			sVerts[0].color = sVerts[1].color = sVerts[2].color = sVerts[3].color = scolor;
			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, sVerts, 4, SVERTEX, &ringTex, 1 );
		}
		else if( info.Flags & CF_SELECTED )
		{
			sVerts[0].color = sVerts[1].color = sVerts[2].color = sVerts[3].color = ScaleDWORDColor( scolor, 128 );
			renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, sVerts, 4, SVERTEX, &ringTex, 1 );
		}
	}

	// Reset zbuffer limits
	gDefaultRapi.SetMaximumZ( 1.0f );

	renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );
}

// Generate the texture for a complex shadow
void SiegeWalker::GenerateShadow( ASPECTINFO& object, const LIGHTINFO& light, bool bGenerateShadow )
{
	GPPROFILERSAMPLE( "SiegeWalker::GenerateShadow()", SP_DRAW | SP_DRAW_CALC_GO );

	Rapi& renderer			= gSiegeEngine.Renderer();

	float foghalfdist		= (renderer.GetFogFarDist() - renderer.GetFogNearDist()) * 0.5f;
	float colorScale		= object.CameraDistance - renderer.GetFogNearDist();
	if( !IsZero( foghalfdist ) && IsPositive( colorScale ) )
	{
		object.ShadowTriColl.m_colorScale	= 1.0f - min_t( 1.0f, colorScale / foghalfdist );
	}
	else
	{
		object.ShadowTriColl.m_colorScale	= 1.0f;
	}

	if( IsZero( object.ShadowTriColl.m_colorScale ) )
	{
		return;
	}

	// Get the radius of the object's bounding sphere
	float object_radius		= 0.0f;
	nema::Aspect* aspect	= object.Aspect;

	// Build a world space light position to use in our calculations
	vector_3 worldSpaceLight( DoNotInitialize );
	vector_3 worldObjectPos( DoNotInitialize );

	if( gSiegeEngine.GetOptions().UseExpensiveShadows() )
	{
		// Calculate the accurate bounding sphere of this object and its children
		vector_3 sphere_center( DoNotInitialize );
		aspect->GetBoundingSphereIncludingChildren( sphere_center, object_radius, object.ObjectScale);

		worldObjectPos		= object.pNode->NodeToWorldSpace( object.Origin + (object.Orientation * sphere_center) );

		if( light.pLight )
		{
			if( light.pLight->m_Type == LT_DIRECTIONAL )
			{
				worldSpaceLight = worldObjectPos + ( object.pNode->GetCurrentOrientation() * light.nodalPosition );
			}
			else
			{
				worldSpaceLight = object.pNode->NodeToWorldSpace( light.nodalPosition );
			}

			// This is a very crude correctional measure to prevent shadows that stretch for miles.
			// Since I don't do radiosity correction on shadows, this is neccesary, but should probably
			// be done based off of some angle based correction instead of this pile I've got here.
			if( worldSpaceLight.y - worldObjectPos.y < 3.0f )
			{
				worldSpaceLight.y = worldObjectPos.y + 3.0f;
			}
		}
		else
		{
			worldSpaceLight		= worldObjectPos;
			worldSpaceLight.y	+= 5.0f;
		}
	}
	else
	{
		worldObjectPos		= object.pNode->NodeToWorldSpace( object.ProjCenter );
		object_radius		= object.BoundHalfDiag.Length();

		worldSpaceLight		= worldObjectPos;
		worldSpaceLight.y	+= 5.0f;
	}

	float angle, nearplane, farplane;
	if( !renderer.SetLightProjection( worldObjectPos, worldSpaceLight, object_radius,
									  angle, nearplane, farplane ) )
	{
		return;
	}

	TRICOLLINFO& collSearch		= object.ShadowTriColl;

	if( gSiegeEngine.GetOptions().UseExpensiveShadows() && object.ComplexShadowTex )
	{
		if( bGenerateShadow )
		{
			RECT bltRect = { 0, 0, renderer.GetShadowTexSize(), renderer.GetShadowTexSize() };

			// If we are in render target mode, set the current shadow texture as the render target
			bool bRenderShadow	= true;
			if( renderer.IsShadowRenderTarget() )
			{
				bRenderShadow	= renderer.SetTextureAsRenderTarget( object.ComplexShadowTex );
			}
			else
			{
				// Clear background to white
				renderer.ClearBackBuffer( &bltRect, 0xFFFFFFFF );
			}

			if( bRenderShadow )
			{
				// Setup the matrix
				renderer.SetWorldMatrix( object.pNode->GetCurrentOrientation(), object.pNode->GetCurrentCenter() );
				renderer.TranslateWorldMatrix( object.Origin );
				renderer.RotateWorldMatrix( object.Orientation );
				renderer.ScaleWorldMatrix( vector_3( object.ObjectScale ) );

				renderer.SetLightProjectionActive();

				vector_3 light_pos( DoNotInitialize );
				if( light.pLight )
				{
					light_pos	= light.localPosition;
				}
				else
				{
					light_pos	= vector_3( 0.0f, 5.0f, 0.0f );
				}

				aspect->RenderShadow( renderer, light_pos, object.ShadowTriColl.m_colorScale );
			}

			if( !renderer.IsShadowRenderTarget() )
			{
				// Blt to the shadow texture
				renderer.CopySubsectionToTexture( object.ComplexShadowTex, &bltRect );
			}
		}

		farplane				+= 3.0f;
		nearplane				+= object_radius;

		// Build up the frustum
		vector_3				FrustumPoint[8];
		vector_3 frustPos		= worldSpaceLight;
		matrix_3x3 frustOrient	= MatrixFromDirection( worldObjectPos - worldSpaceLight );

		float Zdist				= nearplane;
		float Xdist				= Zdist * tanf( angle * 0.5f );
		float Ydist				= Xdist;

		FrustumPoint[0]			= frustPos + (frustOrient * vector_3(-Xdist, Ydist,Zdist));
		FrustumPoint[1]			= frustPos + (frustOrient * vector_3( Xdist, Ydist,Zdist));
		FrustumPoint[2]			= frustPos + (frustOrient * vector_3( Xdist,-Ydist,Zdist));
		FrustumPoint[3]			= frustPos + (frustOrient * vector_3(-Xdist,-Ydist,Zdist));

		float fn				= farplane / nearplane;
		Xdist					= Xdist * fn;
		Ydist					= Ydist * fn;
		Zdist					= Zdist * fn;

		FrustumPoint[4]			= frustPos + (frustOrient * vector_3(-Xdist, Ydist,Zdist));
		FrustumPoint[5]			= frustPos + (frustOrient * vector_3( Xdist, Ydist,Zdist));
		FrustumPoint[6]			= frustPos + (frustOrient * vector_3( Xdist,-Ydist,Zdist));
		FrustumPoint[7]			= frustPos + (frustOrient * vector_3(-Xdist,-Ydist,Zdist));

		collSearch.m_wminBox = vector_3( FLOAT_MAX );
		collSearch.m_wmaxBox = vector_3( FLOAT_MIN );
		for( unsigned int y = 0; y < 8; ++y )
		{
			collSearch.m_wminBox.x	= min_t( collSearch.m_wminBox.x, FrustumPoint[y].x );
			collSearch.m_wminBox.y	= min_t( collSearch.m_wminBox.y, FrustumPoint[y].y );
			collSearch.m_wminBox.z	= min_t( collSearch.m_wminBox.z, FrustumPoint[y].z );

			collSearch.m_wmaxBox.x	= max_t( collSearch.m_wmaxBox.x, FrustumPoint[y].x );
			collSearch.m_wmaxBox.y	= max_t( collSearch.m_wmaxBox.y, FrustumPoint[y].y );
			collSearch.m_wmaxBox.z	= max_t( collSearch.m_wmaxBox.z, FrustumPoint[y].z );
		}

		// Calculate the plane equation for the near plane
		collSearch.m_nearPlaneNormal	= frustOrient.GetColumn2_T();
		collSearch.m_nearPlanePoint		= FrustumPoint[0];
	}
	else
	{
		vector_3 diag( 0.523599f * object_radius );

		collSearch.m_wminBox		= worldObjectPos - diag;
		collSearch.m_wmaxBox		= worldObjectPos + diag;
		collSearch.m_wminBox.y		-= 2.0f;

		collSearch.m_nearPlaneNormal	= vector_3( 0.0f, -1.0f, 0.0f );
		collSearch.m_nearPlanePoint		= worldSpaceLight;
		collSearch.m_nearPlanePoint.y	-= nearplane;
	}

	renderer.BuildTextureProjectionMatrix( object.ShadowProjMat );
}

// Render a single object shadow using the given light source
void SiegeWalker::RenderObjectShadow( ASPECTINFO& info )
{
	GPPROFILERSAMPLE( "SiegeWalker::RenderObjectShadow()", SP_DRAW | SP_DRAW_CALC_GO );

	Rapi& renderer			= gSiegeEngine.Renderer();

	if( IsZero( info.ShadowTriColl.m_colorScale ) )
	{
		return;
	}

	bool bResetExpensiveShadows	= false;
	if( (gSiegeEngine.GetOptions().UseExpensivePartyShadows() && (info.Flags & CF_PARTYMEMBER)) &&
		!gSiegeEngine.GetOptions().UseExpensiveShadows() )
	{
		gSiegeEngine.GetOptions().ToggleExpensiveShadows();
		bResetExpensiveShadows	= true;
	}

	// Setup the projection matrix
	renderer.SetTextureProjectionMatrix( info.ShadowProjMat );

	if( gSiegeEngine.GetOptions().UseExpensiveShadows() && info.ComplexShadowTex )
	{
		renderer.SetTexture( 0, info.ComplexShadowTex );
		renderer.SetTextureStageState(	0,
										D3DTOP_SELECTARG1,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_ZERO,
										D3DBLEND_SRCCOLOR,
										false );
	}
	else
	{
		renderer.SetTexture( 0, gSiegeEngine.GetCheapShadowTexture() );
		renderer.SetTextureStageState(	0,
										D3DTOP_SELECTARG1,
										D3DTOP_MODULATE,
										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										true );
	}

	// Setup for texture projection
	if( !renderer.IsManualTextureProjection() )
	{
		renderer.SetSingleStageState( 0, D3DTSS_TEXCOORDINDEX,			D3DTSS_TCI_CAMERASPACEPOSITION );
		renderer.SetSingleStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS,	D3DTTFF_PROJECTED | D3DTTFF_COUNT3 );
	}
	else
	{
		renderer.SetSingleStageState( 0, D3DTSS_TEXCOORDINDEX,			0 );
		renderer.SetSingleStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS,	D3DTTFF_DISABLE );
	}



	bool bAlphaBlend	= renderer.EnableAlphaBlending( true );
	bool bFog			= renderer.SetFogActivatedState( false );

	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	FALSE );
	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_FLAT );

	// Activate the Zbias matrix
	renderer.SetObserverZbiasProjectionActive();

	gSiegeEngine.NodeTraversalCallback( info.pNode->GetGUID(),
										makeFunctor( *this, &SiegeWalker::ShadowDrawCallBack ),
										2, &info.ShadowTriColl );

	// Restore matrix
	renderer.ResetObserverZbiasProjection();

	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_GOURAUD );
	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );

	renderer.SetFogActivatedState( bFog );
	renderer.EnableAlphaBlending( bAlphaBlend );

	if( !renderer.IsManualTextureProjection() )
	{
		renderer.SetSingleStageState( 0, D3DTSS_TEXCOORDINDEX,			0 );
		renderer.SetSingleStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS,	D3DTTFF_DISABLE );
	}

	if( bResetExpensiveShadows )
	{
		gSiegeEngine.GetOptions().ToggleExpensiveShadows();
	}
}

// Callback function to assist in the collection of bounded triangle information and shadow drawing
bool SiegeWalker::ShadowDrawCallBack( const SiegeNode& node, void* pData )
{
	GPPROFILERSAMPLE( "SiegeWalker::ShadowDrawCallBack()", SP_DRAW | SP_DRAW_GO );

	if( !node.IsOwnedByAnyFrustum() )
	{
		return false;
	}

	// Get the triangle collection search structure
	TRICOLLINFO* pCollSearch	= (TRICOLLINFO*)pData;
	TriangleColl newTriColl;

	if( !BoxIntersectsBox( pCollSearch->m_wminBox, pCollSearch->m_wmaxBox,
						   node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds() ) )
	{
		return false;
	}

	// Make the box nodespace for collision
	vector_3 minBox( DoNotInitialize );
	vector_3 maxBox( DoNotInitialize );
	node.WorldToNodeBox( pCollSearch->m_wminBox, pCollSearch->m_wmaxBox, minBox, maxBox );

	// Get the new triangle collection
	node.GetBoundedTriCollection( minBox, maxBox, newTriColl );
	if( newTriColl.empty() )
	{
		return false;
	}

	// Build a nodespace near clipping plane
	vector_3 nodeSpaceNormal	= node.GetTransposeOrientation() * pCollSearch->m_nearPlaneNormal;
	plane_3 nearplane( nodeSpaceNormal, -nodeSpaceNormal.DotProduct( node.WorldToNodeSpace( pCollSearch->m_nearPlanePoint ) ) );

	// Clip our list of triangles with the near plane to eliminate negative Z artifacting
	ClipTriangles( nearplane, newTriColl );
	if( newTriColl.empty() )
	{
		return false;
	}

	// Get the renderer and setup the matrices for drawing
	Rapi& renderer	= gSiegeEngine.Renderer();
	renderer.PushWorldMatrix();

	DWORD color;
	if( gSiegeEngine.GetOptions().UseExpensiveShadows() )
	{
		color	= 0xFFFFFFFF;
	}
	else
	{
		color	= SetDWORDAlpha( 0xFFFFFFFF, FTOL( pCollSearch->m_colorScale * 255.0f ) );
	}

	// See which version of projection we should use
	unsigned int iv	= 0;
	if( renderer.IsManualTextureProjection() )
	{
		// Texture transform matrix
		D3DMATRIX mat;

		if( gSiegeEngine.GetOptions().UseExpensiveShadows() && renderer.IsManualFrustumClipping() )
		{
			// Set the world matrix to identity
			renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );

			// Convert triangles to world space
			for( TriangleColl::iterator v = newTriColl.begin(); v != newTriColl.end(); ++v )
			{
				*((vector_3*)&(*v)) = node.NodeToWorldSpace( *((vector_3*)&(*v)) );
			}

			// If we're manually projecting the shadow geometry, we need to clip it with the view frustum
			gSiegeEngine.GetCamera().GetFrustum().ClipTrianglesToFrustum( newTriColl );
			if( newTriColl.empty() )
			{
				renderer.PopWorldMatrix();
				return false;
			}

			// Get the basic texture matrix
			renderer.GetTextureMatrix( mat );
		}
		else
		{
			// Clip the triangles just to the near plane to prevent negative z problems
			gSiegeEngine.GetCamera().GetFrustum().ClipTrianglesToNearPlane( newTriColl, node );
			if( newTriColl.empty() )
			{
				renderer.PopWorldMatrix();
				return false;
			}

			// Set the world matrix to node
			renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );

			// Get the current texture transform premultiplied with the current world matrix
			renderer.GetWorldTextureMatrix( mat );
		}

		unsigned int numActualVertices	= newTriColl.size();

		// Build the texture index coordinates
		sVertex* pVerts	= rcast <sVertex*> ( _alloca( numActualVertices * sizeof( sVertex ) ) );

		for( TriangleColl::iterator v = newTriColl.begin(); v != newTriColl.end(); ++v, ++iv )
		{
			*((vector_3*)&pVerts[iv])	= (*v);
			vector_3 vSrc( (*v) );

			// Calculate w and store it in our z component for future use
			(*v).z				= (vSrc.x*mat._14) + (vSrc.y*mat._24) + (vSrc.z*mat._34) + mat._44;
			float invw			= INVERSEF( (*v).z );

			// Calculate the 2D texture coords
			pVerts[iv].uv.u		= ((vSrc.x*mat._11) + (vSrc.y*mat._21) + (vSrc.z*mat._31) + mat._41) * invw;
			pVerts[iv].uv.v		= ((vSrc.x*mat._12) + (vSrc.y*mat._22) + (vSrc.z*mat._32) + mat._42) * invw;
			pVerts[iv].color	= color;
		}

		// Get a pointer to the main vertex buffer
		unsigned int	startIndex;
		if( renderer.CopyBufferedVertices( numActualVertices, startIndex, pVerts ) )
		{
			// Correct the 1/w for the projection by multiplying by wl.  This essentially makes our
			// divisor for perspective correction wl/w, which is correct.
			if( gSiegeEngine.GetOptions().UseExpensiveShadows() )
			{
				tVertex* ptVerts;

				// Transform the vertices
				unsigned int tIndex = renderer.TransformAndLockVertices( startIndex, numActualVertices, ptVerts );
				if( tIndex != 0xFFFFFFFF )
				{
					for( iv = 0; iv < numActualVertices; ++iv )
					{
						ptVerts[iv].rhw		*= newTriColl[iv].z;
					}

					renderer.UnlockBufferedTVertices();

					renderer.DrawBufferedTPrimitive( D3DPT_TRIANGLELIST, tIndex, numActualVertices );
				}
			}
			else
			{
				// Draw
				renderer.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, startIndex, numActualVertices );
			}
		}
	}
	else
	{
		renderer.SetWorldMatrix( node.GetCurrentOrientation(), node.GetCurrentCenter() );
		unsigned int numActualVertices	= newTriColl.size();

		// Get a pointer to the main vertex buffer
		unsigned int	startIndex;
		sVertex*		psVerts;
		if( renderer.LockBufferedVertices( numActualVertices, startIndex, psVerts ) )
		{
			// Fill in the main vertex buffer
			for( TriangleColl::iterator v = newTriColl.begin(); v != newTriColl.end(); ++v, ++iv )
			{
				*((vector_3*)&psVerts[iv])	= (*v);
				psVerts[iv].color			= color;
			}

			renderer.UnlockBufferedVertices();

			// Draw
			renderer.DrawBufferedPrimitive( D3DPT_TRIANGLELIST, startIndex, numActualVertices );
		}
	}

	// Pop the transform
	renderer.PopWorldMatrix();

	return true;
}

// Clear the walker out
void SiegeWalker::Clear()
{
	m_NodeColl.clear();
	m_AlphaNodeColl.clear();

	m_ObjectColl.clear();
	m_ObjectList.clear();
	m_ObjectAlphaList.clear();

	m_discoveryPosMap.clear();
}

// Submit new discover positions
void SiegeWalker::SubmitDiscoveryPosition( const SiegePos& pos, SiegeFrustum* pFrustum )
{
	if( pFrustum->IsDiscoverySubmission() )
	{
		// Go through the active list for this frustum and see if there is already a position close to this one
		float squaredDiscoverySampleLength	= Square( gSiegeEngine.m_DiscoverySampleLength );
		DiscoveryPosColl& discoveryPosColl	= pFrustum->GetDiscoveryPosColl();

		// Clear our discovery collection
		discoveryPosColl.clear();

		// Collect all current discover positions
		for( FrustumNodeColl::iterator n = pFrustum->GetFrustumNodeColl().begin(); n != pFrustum->GetFrustumNodeColl().end(); ++n )
		{
			std::pair< DiscoveryPosMap::const_iterator, DiscoveryPosMap::const_iterator > dc = m_discoveryPosMap.equal_range( (*n)->GetGUID() );

			// Go through each discovery position in this node
			for( ; dc.first != dc.second ; ++dc.first )
			{
				discoveryPosColl.push_back( SiegePos( (*dc.first).second, (*dc.first).first ) );
			}
		}

		for( DiscoveryPosColl::iterator d = discoveryPosColl.begin(); d != discoveryPosColl.end(); ++d )
		{
			if( !gSiegeEngine.SpaceDiffers( pos.node, (*d).node ) &&
				gSiegeEngine.GetDifferenceVector( pos, (*d) ).Length2() < squaredDiscoverySampleLength )
			{
				break;
			}
		}

		if( d == discoveryPosColl.end() )
		{
			// Put it in our mapping
			m_discoveryPosMap.insert( std::pair< database_guid, vector_3 >( pos.node, pos.pos ) );

			// Put in on the local frustum collection
			discoveryPosColl.push_back( pos );
		}
	}
}
