//////////////////////////////////////////////////////////////////////////////
//
// File     :  MeshPreviewer.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

// Include Files
#include "PrecompEditor.h"
#include "PrecompEditor.h"
#include "Commctrl.h"
#include "Namingkey.h"
#include "RapiOwner.h"
#include "Resource.h"
#include "Space_3.h"
#include "Services.h"
#include "Window_Mesh_Previewer.h"
#include "nema_aspectmgr.h"
#include "siege_light_database.h"
#include "siege_options.h"
#include "MeshPreviewer.h"
#include "Server.h"
#include "GoInventory.h"
#include "FuelDB.h"
#include "Player.h"
#include "Preferences.h"
#include "Config.h"

#include "EditorTerrain.h"	// Added to check the current node set texture abbr --biddle


// Namespaces
using namespace siege;

MeshPreviewer::MeshPreviewer()
	: m_rotateY( 0 )
	, m_rotateZ( 0 )
	, m_zoom( -40 )			
	, m_pRenderer( 0 )
	, m_pCamera( 0 )	
	, m_object( GOID_INVALID )
	, m_texture( 0 )	
	, m_bInitialized( false )
	, m_pPreviewWindow( 0 ) 
{
	m_PreviewGUID.assign( "" );
}



MeshPreviewer::~MeshPreviewer()
{		
	delete m_pCamera;
}



void MeshPreviewer::Init()
{	
	if ( gPreferences.GetPreviewWindowPlacement().length != 0 )
	{
		WINDOWPLACEMENT wp = gPreferences.GetPreviewWindowPlacement();
		m_pPreviewWindow->SetWindowPlacement( &wp );
	}

	RECT rect;
	GetClientRect( m_pPreviewWindow->GetSafeHwnd(), &rect );
	if ( !gRapiOwner.PrepareToDraw( m_pPreviewWindow->GetSafeHwnd(), 1, false, rect.right, rect.bottom, 16 ) )
	{
		gperror( "Prepare to draw failed." );
	}

	m_pRenderer = gRapiOwner.GetDevice( 1 );
	m_pRenderer->SetEditMode( true );

	// Initialize the camera
	if ( !m_pCamera ) 
	{
		m_pCamera = new Camera();
		m_pCamera->SetTargetPos( vector_3( 0.0f, 0.0f, 0.0f ) );
		m_pCamera->SetAzimuth( 15.0f * RealPi/180.0f );
		m_pCamera->SetDistance( 3.0 );
		m_pCamera->SetHeight( m_pCamera->GetTargetPos().GetY() );
		m_pCamera->SetOrbit( 0.0f );
	}

	// Initialize a light source
	m_SunLight.m_Position				= vector_3(0,4,4);
	m_SunLight.m_Direction				= vector_3(0,0.7071f,0.7071f);
	m_SunLight.m_Descriptor.m_Type		= LT_DIRECTIONAL;
	m_SunLight.m_Descriptor.m_bActive	= true;
	m_SunLight.m_Descriptor.m_Color		= 0xFFFFFFFF;
	m_SunLight.m_Descriptor.m_Intensity	= 1.0f;
	m_SunLight.m_Descriptor.m_Subtype	= LS_DYNAMIC;	
	
	CheckMenuItem( GetMenu( m_pPreviewWindow->GetSafeHwnd() ), ID_PREFERENCES_WIREFRAME, gPreferences.GetPreviewWireframe() ? MF_CHECKED : MF_UNCHECKED );
	m_pRenderer->SetWireframe( gPreferences.GetPreviewWireframe() );
	m_pRenderer->SetClearColor( gPreferences.GetPreviewColor() );

	SetInitialized( true );
}


void MeshPreviewer::Deinit()
{
	
	SiegeEngine & engine = gSiegeEngine;
	database_guid oldGUID( UNDEFINED_GUID );
	if ( !m_PreviewGUID.empty() )
	{
		oldGUID = database_guid( m_PreviewGUID.c_str() );
		if( oldGUID != UNDEFINED_GUID )
		{
			/*
			// Put these lines in their own block so they get destructed
			{
				SiegeNodeHandle ohandle(engine.NodeCache().UseObject(oldGUID));
				SiegeNode& old_node	= ohandle.RequestObject(engine.NodeCache());
				const D3DTexList &oldTextures	= old_node.GetTextureListing();

				for( D3DTexList::const_iterator o = oldTextures.begin(); o != oldTextures.end(); ++o )
				{
					m_pRenderer->DestroyTexture( (*o).textureId );				
				}
			}
			*/

			// Remove the old node from our cache
			engine.NodeCache().RemoveSlotFromCache( oldGUID );
		}
	}

	/*
	GoHandle hOld( m_object );
	if ( hOld.IsValid() && hOld->HasAspect() )
	{
		int numTex = hOld->GetAspect()->GetAspectHandle()->GetNumSubTextures();
		for ( int i = 0; i != numTex; ++i )
		{
			DWORD tex = hOld->GetAspect()->GetAspectHandle()->GetTexture( i );
			m_pRenderer->DestroyTexture( tex );
		}
	}
	*/

	ClearTextureList();

//	gRapiOwner.StopDrawing( 1 );
//	m_pRenderer = NULL;
	m_object = GOID_INVALID;
	m_PreviewGUID.clear();
	
	SetInitialized( false);
}



void MeshPreviewer::LoadPreviewNode( gpstring szGUID )
{
	// We have to put the textures needed by this node into our renderer
	SiegeEngine & engine = gSiegeEngine;
	database_guid inGUID = database_guid( szGUID.c_str() );

	if ( szGUID.same_no_case( m_PreviewGUID ) ) 
	{
		return;
	}

	if ( inGUID == UNDEFINED_GUID ) 
	{
		return;
	}
	
	if ( !m_pRenderer ) 
	{
		return;
	}

	if ( !m_pPreviewWindow )
	{
		return;
	}

	// Save off the old guid
	database_guid oldGUID( UNDEFINED_GUID );
	if( !m_PreviewGUID.empty()  ) {
		oldGUID = database_guid( m_PreviewGUID.c_str() );
	}

	// Assign the GUID after we know it's valid
	m_PreviewGUID	= szGUID;
		
	if( oldGUID != UNDEFINED_GUID )
	{
		// Put these lines in their own block so they get destructed
		/*
		{
			SiegeNodeHandle ohandle(engine.NodeCache().UseObject(oldGUID));
			SiegeNode& old_node	= ohandle.RequestObject(engine.NodeCache());
			const D3DTexList &oldTextures	= old_node.GetTextureListing();

			for( D3DTexList::const_iterator o = oldTextures.begin(); o != oldTextures.end(); ++o )
			{			
				m_pRenderer->DestroyTexture( (*o).textureId );								
			}
		}
		*/
		ClearTextureList();

		// Remove the old node from our cache
		engine.NodeCache().RemoveSlotFromCache( oldGUID );
	}

	SiegeNodeHandle handle(engine.NodeCache().UseObject(inGUID));
	SiegeNode& preview_node = handle.RequestObject(engine.NodeCache());						
	preview_node.SetMeshGUID( inGUID );

/* Old method copied textures directly -- biddle

	const D3DTexList& textures		= preview_node.GetTextureListing();

	// Go through each texture and put it in our preview renderer
	for( D3DTexList::const_iterator i = textures.begin(); i != textures.end(); ++i )
	{
		m_pRenderer->CopyTextureFromRenderer( engine.Renderer(), (*i).textureId );
		AddToTextureList( (*i).textureId );
	}
*/
	
	// Go through each texture on the mesh and make sure that it is in the correct node set
	SiegeMesh& bmesh = preview_node.GetMeshHandle().RequestObject(engine.MeshCache());
	std::vector< gpstring >::const_iterator i = bmesh.GetTextureStrings().begin();
	for( ; i != bmesh.GetTextureStrings().end(); ++i )
	{
		gpstring filename;
		gpstring localized(*i);
		localized.to_lower();

		unsigned int pos = localized.find("xxx");
		if( pos != localized.npos )
		{
			// This is a generic texture, we need to localize it to the correct texture set and version
			localized.replace( pos, 3, gEditorTerrain.GetNodeSetCharacters() );
			gNamingKey.BuildIMGLocation( localized.c_str(), filename );
		}
		else
		{
			// Try to load the texture using the naming key to locate it
			if( !gNamingKey.BuildIMGLocation( localized.c_str(), filename ) )
			{
				gperrorf(( "Could not build naming key location for %s", localized.c_str() ));
			}
		}

		int newtex = m_pRenderer->CreateTexture( filename, true, 0, TEX_STATIC, true );
		
		if (!m_pRenderer->GetTexSurface(newtex))
		{
			gpgenericf(( "Previewer: texture [%s] could not be loaded\n", localized.c_str() ));
			// The localized texture doesn't exist (yet)
			m_pRenderer->DestroyTexture( newtex );
			gNamingKey.BuildIMGLocation( "b_i_glb_placeholder", filename );
			newtex = m_pRenderer->CreateTexture( filename, true, 0, TEX_STATIC, true );
		}
		
		StaticObjectTex objtexture;
		objtexture.textureId = newtex;
		objtexture.alpha		= m_pRenderer->IsTextureAlpha( newtex );
		objtexture.noalphasort	= m_pRenderer->IsTextureNoAlphaSort( newtex );
		AddToTextureList( objtexture );
	}


	m_rotateY	= 0;
	m_rotateZ	= 0;	

	// Clear out the aspect so only our selected node is seen	
	m_object = GOID_INVALID;		
}



void MeshPreviewer::LoadPreviewObject( gpstring sTemplate )
{
	if ( !m_pRenderer || sTemplate.empty() )
	{
		return;
	}

	if ( !m_pPreviewWindow )
	{
		return;
	}

	m_sObject = sTemplate;
	if ( m_texture )
	{
		m_pRenderer->DestroyTexture( m_texture );
	}
	
	SiegeEngine & engine = gSiegeEngine;
	m_PreviewGUID = "";

	/*
	GoHandle hOld( m_object );
	if ( hOld.IsValid() && hOld->HasAspect() )
	{
		int numTex = hOld->GetAspect()->GetAspectHandle()->GetNumSubTextures();
		for ( int i = 0; i != numTex; ++i )
		{
			DWORD tex = hOld->GetAspect()->GetAspectHandle()->GetTexture( i );
			m_pRenderer->DestroyTexture( tex );
		}
	}
	*/
	ClearTextureList();

	m_object = GOID_INVALID;
	if ( FileSys::IFileMgr::IsAliasedFrom( m_sObject, "%img%" ) )
	{
		return;
	}

	GoCloneReq createGo( sTemplate, gServer.GetPlayer( ToUnicode( "human" ) )->GetId() );	
	createGo.m_OmniGo = true;
	Goid item = gGoDb.SCloneGo( createGo );
	GoHandle hItem( item );
	if ( hItem.IsValid() && hItem->HasAspect() )
	{
		m_object = item;

		int numTex = hItem->GetAspect()->GetAspectHandle()->GetNumSubTextures();
		for ( int i = 0; i != numTex; ++i )
		{
			DWORD tex = hItem->GetAspect()->GetAspectHandle()->GetTexture( i );
			m_pRenderer->CopyTextureFromRenderer( engine.Renderer(), tex );
			AddToTextureList( tex );
		}			

		if ( hItem->HasInventory() )
		{
			hItem->GetInventory()->UnequipAll( AO_REFLEX );
		}
	}
}


void MeshPreviewer::RenderPreviewObject()
{
	GoHandle hItem( m_object );

	if ( FileSys::IFileMgr::IsAliasedFrom( GetObjectName(), "%img%" ) )
	{
		gpstring sFilename;
		sFilename = "%out%/art/bitmaps/decals/";
		sFilename += GetObjectName();
		sFilename = gConfig.ResolvePathVars( sFilename );
		m_texture = m_pRenderer->CreateTexture( sFilename, false, 0, TEX_LOCKED );

		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );

		vector_3 default_color( 1.0f, 1.0f, 1.0f );
		DWORD color	= MAKEDWORDCOLOR( default_color );
		color		&= 0xFFFFFFFF;

		RECT rect;

		if ( m_pPreviewWindow->GetSafeHwnd() )
		{
			m_pPreviewWindow->GetClientRect( &rect );
		}

		Verts[0].x			= rect.left-0.5f;
		Verts[0].y			= rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= 0.0f;
		Verts[0].uv.v		= 1.0f;
		Verts[0].color		= color;

		Verts[1].x			= rect.left-0.5f;
		Verts[1].y			= rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= 0.0f;
		Verts[1].uv.v		= 0.0f;
		Verts[1].color		= color;

		Verts[2].x			= rect.right-0.5f;
		Verts[2].y			= rect.top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= 1.0f;
		Verts[2].uv.v		= 1.0f;
		Verts[2].color		= color;

		Verts[3].x			= rect.right-0.5f;
		Verts[3].y			= rect.bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= 1.0f;
		Verts[3].uv.v		= 0.0f;
		Verts[3].color		= color;

		m_pRenderer->DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_texture, 1 );		
	}
	else if ( hItem.IsValid() )
	{
		m_pRenderer->SetTextureStageState(	0,
									D3DTOP_MODULATE,
									D3DTOP_SELECTARG1,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTFG_LINEAR,
									D3DTFN_LINEAR,
									D3DTFP_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

		vector_3	bcenter( DoNotInitialize );
		matrix_3x3	borient( DoNotInitialize );
		vector_3	bhalf_diag( DoNotInitialize );
		hItem->GetAspect()->GetWorldSpaceOrientedBoundingVolume( borient, bcenter, bhalf_diag );

		m_pRenderer->TranslateWorldMatrix( -bcenter-m_translate );	

		m_pRenderer->GetDevice()->SetRenderState( D3DRENDERSTATE_LIGHTING, false );
		nema::LightSource nls;
		nls.m_Type = nema::NLS_INFINITE_LIGHT;
		nls.m_Color = m_SunLight.m_Descriptor.m_Color;
		nls.m_Intensity = m_SunLight.m_Descriptor.m_Intensity;
		nls.m_Direction = Normalize(m_SunLight.m_Direction);

		hItem->GetAspect()->GetAspectHandle()->InitializeLighting( 0.5f, 255 );
		hItem->GetAspect()->GetAspectHandle()->CalculateShading(  nls, gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
		
		hItem->GetAspect()->GetAspectHandle()->Render( m_pRenderer );
	}
}


void MeshPreviewer::RenderPreviewNode()
{	
	vector_3 targetpos( 0.0f, 0.0f, 0.0f );
	SiegeEngine & engine = gSiegeEngine;
	database_guid inGUID = database_guid(m_PreviewGUID.c_str());
	SiegeNodeHandle handle(engine.NodeCache().UseObject(inGUID));
	SiegeNode& preview_node = handle.RequestObject(engine.NodeCache());						
	SiegeMesh& bmesh = preview_node.GetMeshHandle().RequestObject(engine.MeshCache());
	targetpos	= bmesh.Centroid();
	m_pRenderer->TranslateWorldMatrix( -targetpos-m_translate );	

	// Copy all of the door verts into the boundary mesh
	RapiStaticObject& object = *bmesh.GetRenderObject();
	object.SetRenderer( *m_pRenderer );
	
	object.UpdateTexlist( m_generic_texlist );
	object.Render();
	object.RenderAlpha();
	object.SetRenderer( engine.Renderer() );		
}


void MeshPreviewer::RenderPreviewer()
{
	if ( m_pRenderer == 0 || !IsInitialized() ) 
	{
		return;
	}

	m_pRenderer->SetPerspectiveMatrix( (RealPi/3.0f), 0.1f, 100.0f );

	m_pRenderer->Begin3D();

	vector_3 vFrom		= vector_3( 0.0f, 2.0f, 51.0f+(float)(m_zoom) );
	vector_3 vAt		= vector_3( 0.0f, 0.0f, 0.0f );
	vector_3 vWorldUp	= vector_3( 0.0f, 1.0f, 0.0f );

	m_pRenderer->SetViewMatrix( vFrom, vAt, vWorldUp );
	m_pRenderer->SetWorldMatrix( matrix_3x3(), vector_3() );
	m_pRenderer->RotateWorldMatrix( XRotationColumns( m_pCamera->GetAzimuth() )*YRotationColumns( m_pCamera->GetOrbit() ) );	
			
	m_pRenderer->SetTextureStageState(	0,
										D3DTOP_SELECTARG1,
										D3DTOP_SELECTARG2,
										D3DTADDRESS_WRAP,
										D3DTADDRESS_WRAP,
										D3DTFG_LINEAR,
										D3DTFN_LINEAR,
										D3DTFP_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );
	m_pRenderer->IgnoreVertexColor( true );

	if ( !m_PreviewGUID.empty() ) 
	{
		RenderPreviewNode();
	}
	else
	{
		GoHandle hItem( m_object );
		if ( hItem.IsValid() && hItem->HasAspect() )
		{
			RenderPreviewObject();
		}
	}		
	
	m_pRenderer->End3D();	
}



void MeshPreviewer::ZoomIn()
{ 
	m_zoom -= 1; 	 
}


void MeshPreviewer::ZoomOut()
{
	m_zoom += 1;
}



void MeshPreviewer::RotateY( double x_magnitude )
{
	if ( x_magnitude > 0 ) 
	{
		m_rotateY += 0.1;
		if ( m_rotateY >= (2*RealPi) ) 
		{
			m_rotateY = 0;
		} 
	}
	else if ( x_magnitude < 0 ) 
	{
		m_rotateY -= 0.1;
		if ( m_rotateY >= (2*RealPi) ) 
		{
			m_rotateY = 0;
		}
	}
}



void MeshPreviewer::RotateZ( double y_magnitude )
{
	if ( y_magnitude > 0 ) 
	{
		m_rotateZ -= 0.06;
		if ( m_rotateZ >= (2*RealPi) ) 
		{
			m_rotateZ = 0;
		} 
	}
	else if ( y_magnitude < 0 ) {
		m_rotateZ += 0.06;
		if ( m_rotateZ >= (2*RealPi) ) 
		{
			m_rotateZ = 0;
		}
	}	
}


void MeshPreviewer::TranslateCamera( vector_3 vTranslate )
{
	m_translate += vTranslate;
}



void MeshPreviewer::RotateCamera( int deltax, int deltay )
{
	m_pCamera->SetOrbit( m_pCamera->GetOrbit() + (float)deltax/100 );
	m_pCamera->SetAzimuth( m_pCamera->GetAzimuth() + (float)deltay/100 );
}



void MeshPreviewer::ZoomCamera( int deltay )
{
	m_pCamera->SetDistance( m_pCamera->GetDistance() + (float)deltay/100 );
}


void MeshPreviewer::ResetCamera()
{
	m_pCamera->SetTargetPos( vector_3( 0.0f, 0.0f, 0.0f ) );
	m_pCamera->SetAzimuth( 15.0f * RealPi/180.0f );
	m_pCamera->SetDistance( 3.0 );
	m_pCamera->SetHeight( m_pCamera->GetTargetPos().GetY() );
	m_pCamera->SetOrbit( 0.0f );
	m_rotateY	= 0;
	m_rotateZ	= 0;
	m_zoom		= -40;
	m_translate = vector_3::ZERO;
}


void MeshPreviewer::AddToTextureList( unsigned int texture )
{
	m_textures.push_back( texture );
}

void MeshPreviewer::AddToTextureList( const StaticObjectTex& gen_tex )
{
	m_generic_texlist.push_back( gen_tex );
	m_textures.push_back( gen_tex.textureId );
}

void MeshPreviewer::RemoveFromTextureList( unsigned int texture )
{
	if ( m_pRenderer )
	{
		D3DTexList::iterator g;
		for ( g = m_generic_texlist.begin(); g != m_generic_texlist.end(); ++g )
		{
			if ( (*g).textureId == texture )
			{
				// Texture gets destroyed in the next loop
				m_generic_texlist.erase( g );
				break;
			}
		}
		std::vector< unsigned int >::iterator i;
		for ( i = m_textures.begin(); i != m_textures.end(); ++i )
		{
			if ( *i == texture )
			{
				m_pRenderer->DestroyTexture( texture );
				m_textures.erase( i );
				return;
			}
		}
	}
	else
	{
		gperror( "Renderer does not exist, cannot remove texture" );
	}
}


void MeshPreviewer::ClearTextureList()
{
	if ( m_pRenderer )
	{
		std::vector< unsigned int >::iterator i;
		for ( i = m_textures.begin(); i != m_textures.end(); ++i )
		{
			m_pRenderer->DestroyTexture( *i );
		}

		m_generic_texlist.clear();
		m_textures.clear();
	}
}