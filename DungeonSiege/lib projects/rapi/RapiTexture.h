#pragma once
#ifndef _RAPI_TEXTURE_
#define _RAPI_TEXTURE_
/*************************************************************************************
**
**									RapiTexture
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	05/27/99
**		Author: James Loe
**
*************************************************************************************/


/*
**  Defines the RapiTexture class
**
**	Provides a central location for loading and managing textures.  Any texture that
**  is in use by the game needs to come through here, which is pretty much a given
**  considering that all the drawing calls only take references to RapiTexture
**  objects.  Functionality is provided so that the client can attach textures to
**  text strings and retrieve the texture elsewhere using that string.  This manager
**  will NOT automatically delete textures, and does NOT provide any type of reference
**  counting mechanism.  If you are done with a texture and know that you will not need
**  it again (or at least not for a while), make sure you explicitly destroy it through
**  the DestroyTexture() function in the owner object.  What this class does provide
**  is intelligent texture management, swapping, switching, and all that funky crap
**  that needs to go on behind the scenes for 3D applications.
*/

// Number of multitex layers allowed per surface
#define MAX_TEXTURE_LAYERS		4
#define MAX_TEXTURE_FRAMES		16

const unsigned int ENVIRONMENT	= 0xFFFFFFFF;

class TextureSurfaceInfo
{
public:

	TextureSurfaceInfo()
	: m_numFrames( 1 )
	, m_secondsPerFrame( 0.0f )
	, m_ushiftPerSecond( 0.0f )
	, m_vshiftPerSecond( 0.0f )
	, m_radrotPerSecond( 0.0f )
	, m_uRotCenter( 0.0f )
	, m_vRotCenter( 0.0f )
	, m_uWrap( -1 )
	, m_vWrap( -1 )
	, m_colorOp( -1 )
	, m_colorArg1( -1 )
	, m_colorArg2( -1 )
	, m_alphaOp( -1 )
	, m_alphaArg1( -1 )
	, m_alphaArg2( -1 )
	, m_currentFrameTime( 0.0 )
	, m_currentFrame( 0 )
	, m_currentUshift( 0.0 )
	, m_currentVshift( 0.0 )
	, m_currentRotation( 0.0 )
	{}

	// Number of frames of animation per layer of this texture
	int							m_numFrames;

	// Filepath's to the individual textures
	std::vector< gpstring >		m_textureNames;
	std::vector< unsigned int >	m_textureSurface;

	// Number of seconds to spend on each frame per layer
	float						m_secondsPerFrame;

	// Amount of UV shift and rotation to apply each second
	float						m_ushiftPerSecond;
	float						m_vshiftPerSecond;
	float						m_radrotPerSecond;

	// Center of rotation
	float						m_uRotCenter;
	float						m_vRotCenter;

	/**************************************************************/
	/*		OPTIONAL PARAMETERS									  */
	/**************************************************************/

	// Wrap or Clamp the UV's?
	int							m_uWrap;
	int							m_vWrap;

	// Texture operations and arguments
	int							m_colorOp;
	int							m_colorArg1;
	int							m_colorArg2;

	int							m_alphaOp;
	int							m_alphaArg1;
	int							m_alphaArg2;

	/**************************************************************/
	/*		TIMING PARAMETERS									  */
	/**************************************************************/

	double						m_currentFrameTime;
	int							m_currentFrame;
	double						m_currentUshift;
	double						m_currentVshift;
	double						m_currentRotation;
};

class TextureSurfaceDescription
{
public:

	TextureSurfaceDescription()
	: m_numLayers( 1 )
	, m_syncTimer( true )
	, m_hasAlpha( false )
	, m_alphaFormat( false )
	, m_alphaTest( false )
	, m_noAlphaSort( false )
	, m_dither( true )
	, m_bAnimationState( true )
	, m_animationSpeed( 1.0f )
	, m_texDetail( 0 )
	, m_multiInstance( false )
	, m_srcBlend( -1 )
	, m_destBlend( -1 )
	{}

	// Number of layers (multitex) in this texture
	int			m_numLayers;

	// Is this surface synched to the main timer?
	bool		m_syncTimer;

	// Does this surface have alpha?
	bool		m_hasAlpha;

	// Should this surface be created with an alpha format?
	bool		m_alphaFormat;

	// Should this surface use alpha test?
	bool		m_alphaTest;

	// Should this surface be fully alpha sorted?
	bool		m_noAlphaSort;

	// Should this surface dither down when converting to 16 bits?
	bool		m_dither;

	// Current state of the animation
	bool		m_bAnimationState;

	// Current speed of the animation
	float		m_animationSpeed;

	// Detail identifier
	int			m_texDetail;

	// Should this texture be treated as a multi instance surface?
	bool		m_multiInstance;

	// Blend modes
	int			m_srcBlend;
	int			m_destBlend;

	// Surface information
	std::vector< TextureSurfaceInfo > m_SurfaceLayers;
};


class RapiTexture
{
	friend class Rapi;

public:

	// Implicit destruction of texture
	~RapiTexture();

	// Get dimensions.  Since there may be multiple surfaces per texture, this call merely returns
	// the dimensions of the first surface.
	unsigned int				GetWidth();
	unsigned int				GetHeight();

	// Get the pathname of this texture
	inline const gpstring&		GetPathname() const					{ return m_Pathname; }

	// Alpha
	inline bool					HasAlpha()							{ return m_pSurfaceDescription[0].m_hasAlpha; }
	inline bool					IsAlphaFormat()						{ return m_pSurfaceDescription[0].m_alphaFormat; }
	inline bool					IsNoAlphaSort()						{ return m_pSurfaceDescription[0].m_noAlphaSort; }

	// Environment
	inline bool					HasEnvironment()					{ return m_bEnvironment; }

	// Traits
	bool						IsFromDisk();

	// Multi instance
	inline bool					IsMultiInstance()					{ return m_pSurfaceDescription[0].m_multiInstance; }

	// Multi pass
	inline bool					IsMultiPass()						{ return (m_numPasses > 1); }

	// Read in the surface description information
	void						InitSurfaceDescription( gpstring& errorSink );

	// Restore this texture by going through its owned surfaces and restoring them
	bool						Load();

	// Release this texture by going through its owned surfaces and releasing them
	bool						Unload();

	// Reference counting
	unsigned int				GetReferenceCount()					{ return m_refCount; }
	void						IncrementReferenceCount();
	void						DecrementReferenceCount();

	unsigned int				GetLoadReferenceCount()				{ return m_loadRefCount; }
	void						IncrementLoadReferenceCount();
	void						DecrementLoadReferenceCount();

	// Setup this surface as the currently active surface
	void						SetAsActiveSurface( double timeElapsed = 0.0f, double secondTime = 0.0f, unsigned int stage = 0, unsigned int pass = 0 );
	bool						SetAsRenderTarget();

	// Set the wrapping mode, ops and arguments
	void						SetSurfaceInformation( int layer, int pass = 0 );

	// Set the state of the texture animation
	void						SetAnimationState( bool bState );
	bool						GetAnimationState()					{ return m_pSurfaceDescription[0].m_bAnimationState; }

	// Set the speed of the texture animation
	void						SetAnimationSpeed( float speed );
	float						GetAnimationSpeed()					{ return m_pSurfaceDescription[0].m_animationSpeed; }

	// Active status this frame
	void						SetActive( bool active );
	bool						GetActive()							{ return m_pbActive[0]; }

	// Saturation
	void						SetSaturation( float saturation );
	void						RevertSaturation();
	bool						IsSaturated()						{ return m_bSaturated; }

private:

	// Construction of file based texture
	// filename is assumed to be either GAS relative or fully qualified
	RapiTexture(	Rapi*				pOwner,
					const gpstring&		filename,
					bool				bMipmapping,
					int					rStage,
					TEX_OPT				rOptimize,
					bool				bFullLoad,
					gpstring&			errorSink );

	// Construction of algorithmic texture
	RapiTexture(	Rapi*				pOwner,
					const gpstring&		texture_name,
					RapiMemImage*		takeImage,
					bool				bMipmapping,
					int					rStage,
					TEX_OPT				rOptimize,
					bool				bHasAlpha,
					bool				bDither,
					gpstring&			errorSink );

	// Construction of empty texture
	RapiTexture(	Rapi*				pOwner,
					int					width,
					int					height,
					DWORD				color,
					TEX_OPT				rOptimize,
					bool				bHasAlpha,
					bool				bDither,
					bool				bMatchPrimary,
					bool				bRenderTarget,
					gpstring&			errorSink );

	// Returns the enumerated op value of the D3D argument passed in string form.
	int							GetEnumeratedOpValueFromString( gpstring& value );

	// Returns the enumerated arg value of the D3D argument passed in string form.
	int							GetEnumeratedArgValueFromString( gpstring& value );

	// Returns the enumerated blend value of the D3D argument pass in string form.
	int							GetEnumeratedBlendValueFromString( gpstring& value );

	// Cleanup all information for this texture
	void						Destroy();

	// My owner and device
	Rapi*						m_pDevice;

	// GAS relative pathname
	gpstring					m_Pathname;

	// Reference counter to control destruction
	unsigned int				m_refCount;
	unsigned int				m_loadRefCount;

	// Is this texture active in the scene for this frame?
	bool*						m_pbActive;

	// Does this texture have an environment map?
	bool						m_bEnvironment;

	// Is this texture saturated?
	bool						m_bSaturated;

	// Number of rendering passes
	unsigned int				m_numPasses;

	// Surface description
	TextureSurfaceDescription*	m_pSurfaceDescription;
};

#endif