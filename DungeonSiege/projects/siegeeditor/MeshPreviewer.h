//////////////////////////////////////////////////////////////////////////////
//
// File     :  MeshPreviewer.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __MESHPREVIEWER_H
#define __MESHPREVIEWER_H


#include "gpcore.h"
#include "math.h"
#include "vector_3.h"
#include "nema_aspect.h"
#include "siege_light_database.h"
#include "GoDefs.h"


// Forward Declarations
class Rapi;
class RapiOwner;
class TextureManager;
class Window_Mesh_Previewer;


namespace siege {
	class SiegeEngine;
	class console;
	class tnk_file_system;
	struct SiegeNodeLight;
}	

class Camera
{
public:
	
	// Constructor
	Camera()	{};

	// Destructor
	~Camera()	{};

	void		SetTargetPos( vector_3 target )	{ m_targetpos = target;		}
	vector_3	GetTargetPos()					{ return m_targetpos;		}
	
	void		SetDistance( float distance )	{ m_distance = distance;	}
	float		GetDistance()					{ return m_distance;		}
				
	void		SetOrbit( float orbit )			{ m_orbit = orbit;			}
	float		GetOrbit()						{ return m_orbit;			}
				
	void		SetHeight( float height )		{ m_height = height;		}
	float		GetHeight()						{ return m_height;			}
				
	void		SetAzimuth( float azimuth )		{ m_azimuth = azimuth;		}
	float		GetAzimuth()					{ return m_azimuth;			}

private:

	float		m_distance;
	float		m_orbit;
	float		m_height;
	float		m_azimuth;
	vector_3	m_targetpos;
};




class MeshPreviewer : public Singleton <MeshPreviewer>
{
public:
	// Public Methods

	// Constructor
	MeshPreviewer();

	// Destructor
	~MeshPreviewer();

	// Initialization
	void Init();
	void Deinit();

	// Load a node in 
	void LoadPreviewNode( gpstring szGUID );
	void LoadPreviewObject( gpstring sTemplate );

	// Render the node
	void RenderPreviewer();
	void RenderPreviewObject();
	void RenderPreviewNode();

	// Zoom In/Out
	void ZoomIn();
	void ZoomOut();
	
	// Viewing Helper Functions
	void RotateY( double x_magnitude );
	void RotateZ( double y_magnitude );
	void Zoom( bool bZoomIn );
	void RotateCamera( int deltax, int deltay );
	void ZoomCamera( int deltay );
	void TranslateCamera( vector_3 vTranslate );
	void ResetCamera();

	// Object Naming
	void		SetObjectName( gpstring sName )	{ m_sObject = sName;	}
	gpstring	GetObjectName()						{ return m_sObject;		}

	void		SetObjectGoid( Goid object ) { m_object = object; }

	void					SetPreviewerWindow( Window_Mesh_Previewer *pWindow )	{ m_pPreviewWindow = pWindow;	}
	Window_Mesh_Previewer *	GetPreviewerWindow()									{ return m_pPreviewWindow;		}

	Rapi * GetRenderer() { return m_pRenderer; }

	void SetInitialized( bool bSet ) { m_bInitialized = bSet; }
	bool IsInitialized()			 { return m_bInitialized; }

	void AddToTextureList( unsigned int texture );
	void AddToTextureList( const StaticObjectTex& gen_tex );

	void RemoveFromTextureList( unsigned int texture );
	void ClearTextureList();

private:

	// Private Member Variables
	gpstring				m_PreviewGUID;
	gpstring				m_sObject;

	// Rapi Renderer Object
	Rapi *					m_pRenderer;	
	Camera *				m_pCamera;	
	Goid					m_object;
	siege::SiegeNodeLight	m_SunLight;
	unsigned int			m_texture;

	// View Matrix Settings
	double					m_rotateY;
	double					m_rotateZ;
	double					m_zoom;
	vector_3				m_translate;

	siege::tnk_file_system  *	m_pSiegeFileSystem;
	siege::SiegeEngine		*	m_pSiegeEngine;
	Window_Mesh_Previewer	*	m_pPreviewWindow;

	std::vector< unsigned int >		m_textures;
	D3DTexList						m_generic_texlist;

	bool					m_bInitialized;
};


#define gPreviewer MeshPreviewer::GetSingleton()


#endif