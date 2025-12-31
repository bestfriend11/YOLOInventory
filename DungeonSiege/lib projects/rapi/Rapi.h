#pragma once
#ifndef _RAPI_
#define _RAPI_

/*************************************************************************************
**
**									Rapi
**
**		Defines the Rapi class.
**	
**		Rapi is a second level protected class responsible for managing a
**		subset of 3D calls in the game.  Each Rapi object can be thought of
**		as a display device or monitor.
**
**		Date:	05/26/99
**		Author: James Loe
**
*************************************************************************************/

#include "KernelTool.h"
#include "RapiOwner.h"

class Rapi
{
	friend class RapiOwner;
	friend class RapiFont;
	friend class RapiLight;
	friend class RapiTexture;
	friend class RapiSurface;
	friend class RapiStaticObject;
	friend class RapiMouse;

	enum eFogType
	{
		RAPI_FOGTYPE_NONE	= 0,
		RAPI_FOGTYPE_ZFOG	= 1,
		RAPI_FOGTYPE_WFOG	= 2,
		RAPI_FOGTYPE_VERTEX	= 3
	};

public:

	// Anyone can destroy me
	~Rapi();

	// Get a pointer to my D3D device
	// Never store a pointer to this object, as it may change
	LPDIRECT3DDEVICE8	GetDevice()							{ return m_pD3DDevice; }
	const D3DCAPS8&		GetDeviceDesc()						{ return m_d3dDevDesc; }

	// Get the primary surface
	LPDIRECT3DSURFACE8	GetPrimarySurface()					{ return m_pBackBuffer; }
	LPDIRECT3DSURFACE8	GetBackBufferSurface()				{ return m_pBackBuffer; }

	// Formats
	ePixelFormat		GetPrimaryPixelFormat()				{ return m_primaryPixelFormat; }
	D3DFORMAT			GetPrimaryD3DFormat()				{ return m_primaryD3DFormat; }

	D3DFORMAT			GetDepthD3DFormat()					{ return m_depthD3DFormat; }

	ePixelFormat		GetOpaqueTexPixelFormat()			{ return m_opaqueTexPixelFormat; }
	D3DFORMAT			GetOpaqueTexD3DFormat()				{ return m_opaqueTexD3DFormat; }

	ePixelFormat		GetAlphaTexPixelFormat()			{ return m_alphaTexPixelFormat; }
	D3DFORMAT			GetAlphaTexD3DFormat()				{ return m_alphaTexD3DFormat; }

	// Get a pointer to the D3D object
	// Never store a pointer to this object
	LPDIRECT3D8			GetObject();

	// Begin a frame
	bool				Begin3D( bool bClear = true );

	// End a frame
	bool				End3D( bool bCopyToPrimary = true, bool bCopyImmediately = false );

	// Blt/Flip the current frame onto the primary
	bool				CopySceneToPrimary();

	// Clear the backbuffer
	void				ClearBackBuffer( RECT *pRect = NULL, DWORD color = 0x00000000 );

	// Clear front and all back buffers to black
	void				ClearAllBuffers();

	// Tell Rapi to set the fast FPU mode
	void				SetFastFPU( bool bFastFPU )			{ m_bFastFPU = bFastFPU; }

	// Get the current frame count
	unsigned int		GetFrameCount()						{ return m_FrameCount; }

	// Set the elapsed game time
	double				GetFrameStartTime( )				{ return m_startTime; }
	void				SetGameElapsedTime( double time );

	// Get the width or height of the current display
	int					GetWidth()							{ return m_Width; }
	int					GetHeight()							{ return m_Height; }

	// Get the width or height of the current viewport
	int					GetViewWidth()						{ return m_viewWidth; }
	int					GetViewHeight()						{ return m_viewHeight; }

	// Get the current estimated frame rate
	float				GetFrameRate()						{ return m_frameRate; }

	// Get bits per pixel
	int					GetBitsPerPixel()					{ return m_BPP; }

	// Get texture information
	unsigned int		GetTextureSwitches()				{ return m_TextureSwitches; }
	unsigned int		GetNumActiveTex()					{ return m_NumActiveTextures; }
	unsigned int		GetTextureMemory()					{ return m_TextureMemory; }
	unsigned int		GetTexSystemMemory()				{ return m_TexSystemMemory; }
	unsigned int		GetTexManagerSystemMemory()			{ return m_TexManagerSystemMemory; }

	// Get available video memory
	unsigned int		GetSystemVideoMemory()				{ return m_vidMem; }
	unsigned int		GetAvailableTextureMemory()			{ return m_availTexMem; }

	// Get device identification
	DWORD				GetVendorId()						{ return m_deviceId.VendorId; }
	DWORD				GetDeviceId()						{ return m_deviceId.DeviceId; }
	LONGLONG			GetDriverVersion()					{ return m_deviceId.DriverVersion.QuadPart; }
	const char*			GetDeviceName()						{ return m_deviceId.Description; }

	// Get the system detail identifier
	int					GetSystemDetailIdentifier()			{ return m_sysDetail; }
	void				SetSystemDetailList( const SystemDetailList& sysDetails );

	// Display mode information
	const DisplayList	GetDisplayModeList();
	void				GetClosestDisplayMode( const int mWidth, const int mHeight, const int mBPP,
											   int& rWidth, int& rHeight, int& rBPP );
	bool				BitDepthAvailable( const int bpp );

	// Is this device currently running fullscreen?
	bool				IsFullscreen()						{ return m_bFullScreen; }

	// Are we waiting for vertical sync between frames?
	bool				IsVSync()							{ return m_bVSync; }

	// Is flipping allowed
	bool				IsFlipAllowed()						{ return m_bFlip; }
	void				SetFlipAllowed( bool bFlip );

	// Triple buffering
	bool				IsTripleBuffering()					{ return m_bTripleBuffering; }
	void				SetTripleBuffering( bool bBuffer )	{ m_bTripleBuffering = bBuffer; }

	// 16-bit only
	bool				Is16bitOnly()						{ return m_b16bitOnly; }
	void				Set16bitOnly( bool b16bit )			{ m_b16bitOnly = b16bit; }

	// Force complete texture rebuild whenever surfaces are restored
	bool				ForceTextureRestore()				{ return m_bForceTexRestore; }
	void				SetForceTextureRestore( bool bRest ){ m_bForceTexRestore = bRest; }

	// Trilinear filtering
	void				SetMipFilter( TEX_MPF filter );
	TEX_MPF				GetMipFilter()						{ return m_mipFilter; }
	bool				GetTrilinearCapable()				{ return m_bTrilinearCapable; }

	// Set the wireframe
	void				SetWireframe( bool bWire );
	bool				IsWireframe()						{ return m_bWireframe; }

	// Set what color to clear the screen with (r,g,b)
	void				SetClearColor( DWORD color )		{ m_ClearColor = color; }
	DWORD				GetClearColor()						{ return m_ClearColor; }

	// Set the gamma
	// Range: 0.0f(full dark) - 2.0f(full bright)
	void				SetGamma( const vector_3& gamma );
	void				SetGamma( const float gamma );
	vector_3			GetGamma()							{ return m_Gamma; }

	// Set the monitor gamma
	void				SetSystemGamma( const float gamma );
	float				GetSystemGamma()					{ return m_SystemGamma; }

	// Set the current fade
	void				SetFade( float fade )				{ m_Fade	= fade; }
	float				GetFade()							{ return m_Fade; }
	bool				IsFading() const					{ return m_Fade != 0.0f; }

	// Set the maximum/minimum Z
	void				SetMaximumZ( float z );
	void				SetMinimumZ( float z );

	// Set the texture factor based off of the given vector
	void				SetActiveLightFactor( const vector_3& light );

	// Enable/Disable alpha blending
	bool				EnableAlphaBlending( bool bBlend );

	// Enable/Disable "no textures" option (debug)
	void				SetNoTextures( bool bNo )			{ m_bNoTextures = bNo; }
	bool				GetNoTextures()						{ return m_bNoTextures; }

	// Enable/Disable rendering
	void				SetNoRender( bool bNo )				{ m_bEdit ? m_bNoRender = false : m_bNoRender = bNo; }
	bool				GetNoRender()						{ return m_bNoRender; }

	// Enable/Disable async cursor
	void				SetAsyncCursor( bool bCursor );
	bool				GetAsyncCursor()					{ return m_bAsyncCursor; }

	// Enable/Disable edit mode
	void				SetEditMode( bool bEdit )			{ m_bEdit = bEdit; }
	bool				GetEditMode()						{ return m_bEdit; }

	// Surface expiration time
	void				SetSurfaceExpirationTime( float t ) { m_SurfaceExpirationTime = t; }

	// Creates a texture from a file for use with this device with error control
	const unsigned int	CreateTexture(	gpstring const &	texture_file,
										bool				bMipmapping,
										int					rStage,		
										TEX_OPT				rOptimize,
										bool				bFullLoad = true,
										gpstring*			pErrorSink = NULL );

	// Restore (or upgrade) a texture that had no (or lost) surface information
	bool				LoadTexture( const unsigned int tex );

	// Release (or downgrade) a texture that has full surface information
	bool				UnloadTexture( const unsigned int tex );

	// Creates a texture from the passed image for use with this device
	const unsigned int	CreateAlgorithmicTexture(   gpstring const &	texture_name,
													RapiMemImage		*pTakeImage,
													bool				bMipmapping,
													int					rStage,		
													TEX_OPT				rOptimize,
													bool				bHasAlpha,
													bool				bDither );

	// Creates a texture from a region in the primary buffer
	const unsigned int	CreateSubsectionTexture(	const RECT			*pRect,
													bool				bMipmapping,
													int					rStage,		
													TEX_OPT				rOptimize );

	// Creates a blank texture filled with the given color
	const unsigned int	CreateBlankTexture(			int					width,
													int					height,
													DWORD				color,
													TEX_OPT				rOptimize,
													bool				bHasAlpha,
													bool				bMatchPrimary,
													bool				bRenderTarget );

	// Creates a texture for shadow use
	const unsigned int	CreateShadowTexture();
	void				SetShadowTexSize( UINT shadowSize )	{ m_shadowTexSize = shadowSize; }
	UINT				GetShadowTexSize()					{ return m_shadowTexSize; }
	
	// Creates an image from a region in the primary buffer - You must maintain and delete the pointer
	RapiMemImage *		CreateSubsectionImage(		const RECT 			*pRect = NULL );

	// Copies a region in the primary buffer and stores it in the passed image
	void				CopySubsectionToImage(		RapiMemImage		*pImage,
													const RECT 			*pRect = NULL );

	// Blits a region in the primary buffer to the given texture
	void				CopySubsectionToTexture(	const unsigned int	texture,
													const RECT			*pRect = NULL );

	// Get the primary surface of a texture
	LPDIRECT3DTEXTURE8	GetTexSurface( const unsigned int tex );

	// Verify that a texture exists in Rapi
	bool				TextureExists( const gpstring& texture_file );

	// Add a reference to the given texture
	bool				AddTextureReference( const unsigned int tex );

	// Get the current reference count
	unsigned int		GetTextureReferenceCount( const unsigned int tex );
	unsigned int		GetLoadTextureReferenceCount( const unsigned int tex );

	// Destroy indicated texture and remove it from the list
	bool				DestroyTexture( const unsigned int tex );

	// Set the active texture
	void				SetTexture( const unsigned int stage, const unsigned int tex );

	// See if given texture has alpha
	bool				IsTextureAlpha( const unsigned int tex );

	// See if given texture does not need alpha sorting
	bool				IsTextureNoAlphaSort( const unsigned int tex );

	// See if given texture has environment
	bool				IsTextureEnvironment( const unsigned int tex );

	// See if given texture was loaded from disk
	bool				IsTextureFromDisk( const unsigned int tex );

	// Get texture information
	unsigned int		GetTextureWidth( const unsigned int tex )	{ RapiTexture* texture = m_Textures[ tex ];  gpassert( texture );
																	  return( texture ? texture->GetWidth() : 0 ); }
	unsigned int		GetTextureHeight( const unsigned int tex )	{ RapiTexture* texture = m_Textures[ tex ];  gpassert( texture );
																	  return( texture ? texture->GetHeight() : 0 ); }

	// Get the pathname
	const char*			GetTexturePathname( const unsigned int tex ){ RapiTexture* texture = m_Textures[ tex ];  gpassert( texture );
																	  return( texture ? texture->GetPathname() : ""); }

	// Texture animation modifications
	void				SetTextureAnimationState( unsigned int tex, bool bState );
	void				SetTextureAnimationSpeed( unsigned int tex, float speed );
	bool				GetTextureAnimationState( unsigned int tex );
	float				GetTextureAnimationSpeed( unsigned int tex );

	// Current active environment texture (for spherical env mapping)
	void				SetEnvironmentTexture( const unsigned int envTex )		{ m_EnvironmentTexture = envTex; }
	unsigned int		GetEnvironmentTexture()									{ return m_EnvironmentTexture; }

	// Makes sure that all surfaces are properly restored and that gamma is properly set.
	void				RestoreSurfaces();

	// Reloads all loaded textures, only required when exclusive mode or desktop
	// display settings are changed.
	void				ReloadSurfaces();

	// Active texture logging
	void				SetActiveTextureLogging( bool bLogging );
	const TextureSet&	GetActiveTextureLog()						{ return m_ActiveTextureLog; }

	// Change the saturation level for a collection of textures
	void				ChangeSaturationForTextures( const TextureSet& texSet, float saturation );
	void				RevertSaturatedTextures();

	// Create a new light
	const unsigned int	CreateLight();

	// Destroy indicated light and remove it from the list
	void				DestroyLight( const unsigned int light );

	// Set the properties of a light
	void				SetLight(	unsigned int	light,				// light id
									LTYPE			lType,				// Type of light
									LCOLOR			&lColor,			// Color of light
									vector_3		&vPos,				// World position of light
									vector_3		&vDir,				// Direction the light is pointing
									float			fRange,				// Range beyond which the light has no effect
									float			fLinearA	= 1.0f,	// Linear attenuation value
									float			fConstA		= 0.0f, // Constant attenuation value
									float			fQuadA		= 0.0f, // Quadratic attenuation value
									float			fInnerAngle	= 0.52f,// Inner cone angle (only used for spotlights)
									float			fOuterAngle = 0.79f // Outer cone angle (only used for spotlights)
								);

	// Enable or disable a light.
	void				EnableLight( const unsigned int light, bool bEnable );

	// Set the ambient light
	void				SetAmbientLight( DWORD color );

	// Tell Rapi to ignore vertex lighting information
	void				IgnoreVertexColor( bool bIgnore );
	bool				IsIgnoringVertexColor()								{ return m_bIgnoreVertexColor; }

	// Fogging controls available 
	bool				GetFogActivatedState()								{ return m_bFogIsActive; }
	bool				SetFogActivatedState( const bool bNewState );
	bool				IsFogEnabled();

	// no alpha for fog so only use lower 24 bits
	void				SetFogColor( const DWORD color );
	DWORD				GetFogColor()										{ return m_fogColor; }

	// What equation to use for pixel(table) or vertex fog
	// (D3DFOG_NONE, D3DFOG_EXP, D3DFOG_EXP2, D3DFOG_LINEAR)
	void				SetFogModeFormula( const D3DFOGMODE mode );
	DWORD				GetFogModeFormula();

	void				SetFogDensity( const float density );
	float				GetFogDensity()										{ return m_fogDensity; }

	// Fog distance in CAMERA SPACE (ie. metres from camera position)
	void				SetFogNearDist( const float &d );
	void				SetFogFarDist( const float &d );
	void				SetFogDistances( const float near_dist, const float far_dist );
	float				GetFogNearDist();
	float				GetFogFarDist();

	// Fog distance in CLIP SPACE (ie. [0-1] from camera position)
	float				GetClipSpaceFogNearDist();
	float				GetClipSpaceFogFarDist();

	// Set to true to use ranged based fog computations
	void				EnableRangeBasedFog( const bool bEnable );
	bool				IsRangeBasedFogEnabled();
	bool				IsZFogEnabled();
	bool				IsWFogEnabled();
	bool				IsVertFogEnabled();

	bool				IsRangeFogAvailable();
	bool				IsTableFogAvailable();

	// Viewport clip distances are stored in Rapi so that they may be
	// manipulated when the fog distances are changed
	// Clip distance is in CAMERA SPACE (ie. metres from camera position)
	void				SetClipNearDist( const float &d );
	void				SetClipFarDist( const float &d );

	float				GetClipNearDist()					{ return m_clipNearDist; }
	float				GetClipFarDist()					{ return m_clipFarDist; }

	// Return a far clip that takes into account whether of not there is
	// fog limiting the distance we can see
	float				GetClipFarDistAdjustedForFog();

	// FarClipDist = FarFogDist * ClipToFogRatio;
	// I made this ratio public so that we can experiment with different video cards
	// May be possible to set this ratio as a constant
	void				SetClipToFogRatio( const float &r ) { m_clipToFogRatio	= r;	}
	float				GetClipToFogRatio()					{ return m_clipToFogRatio; }

	// Add a new font to the system
	RapiFont*			CreateFont( HFONT font = (HFONT)GetStockObject(SYSTEM_FONT), bool forceBitmap = false );
	RapiFont*			CreateFont( unsigned int texture, gpstring sFont, UINT32 characterHeight,
									UINT32 startRange, UINT32 endRange );
	
	// Viewport modification
	bool				SetViewport( float norm_width, float norm_height );
	bool				GetViewport( float& norm_width, float& norm_height );
	void				DrawViewportBands( DWORD color = 0xFF000000 );

	// States
	bool				IsManualTextureProjection()				{ return m_bManualTextureProjection; }
	bool				IsManualFrustumClipping()				{ return m_bManualFrustumClipping; }
	bool				IsOnlySquareTextures()					{ return m_bOnlySquareTextures; }
	bool				IsColorControl()						{ return m_bColorControl; }
	bool				IsMultiTexBlend()						{ return m_bMultiTexBlend; }
	bool				IsTextureStateReset()					{ return m_bTextureStateReset; }
	bool				IsModulateOnly()						{ return m_bModulateOnly; }

	void				SetColorControl( bool colorCont );
	void				SetMultiTexBlend( bool texBlend )		{ m_bMultiTexBlend = texBlend; }
	void				SetTextureStateReset( bool stateReset ) { m_bTextureStateReset = stateReset; }
	void				SetModulateOnly( bool modOnly )			{ m_bModulateOnly = modOnly; }

	void				SetShadowRenderTarget( bool target )	{ m_bShadowRenderTarget = target; }
	bool				IsShadowRenderTarget()					{ return m_bShadowRenderTarget; }

	void				SetBuffersFrames( bool buffers )		{ m_bBuffersFrames = buffers; }
	bool				IsBuffersFrames()						{ return m_bBuffersFrames; }

	void				SetSceneFade( bool fade )				{ m_bSceneFade = fade; }
	bool				IsSceneFade()							{ return m_bSceneFade; }

	// Render target
	bool				SetTextureAsRenderTarget( unsigned int tex );
	void				ResetRenderTarget();

	// Change the currently active projection
	void				SetLightProjectionActive();
	void				SetObserverProjectionActive();

	void				SetObserverZbiasProjectionActive();
	void				ResetObserverZbiasProjection();

	// Manual transform functions
	unsigned int		TransformAndLockVertices( const unsigned int vBufferStart, const unsigned int numVerts, tVertex*& pVerts );
	vector_3			GetScreenPosition( const vector_3& worldPos );

	// Matrix info
	void				SetViewMatrix(	const vector_3& vFrom, 
										const vector_3& vAt, 
										const vector_3& vWorldUp );

	void				SetPerspectiveMatrix(	const float fFOV		= 1.570795f,
												const float fNearPlane	= 1.0f,
												const float fFarPlane	= 1000.0f );

	void				SetOrthoMatrix(	const float fMetersPerPixel	= 1.0f,
										const float fNearPlane		= 1.0f,
										const float fFarPlane		= 200.0f );

	// Texture projection matrix
	void				SetTextureProjectionMatrix();
	void				SetTextureProjectionMatrix( D3DMATRIX& mat );
	void				BuildTextureProjectionMatrix( D3DMATRIX& mat, bool bYFlip = true );

	bool				SetLightProjection( const vector_3& object_position, const vector_3& eye_position, const float object_radius,
											float& angle, float& nearplane, float& farplane );
	void				SetLightProjection( const vector_3& origin, const matrix_3x3& orientation, const float angle,
										    const float aspect, const float nearplane, const float farplane );

	// World matrix functions
	void				SetWorldMatrix(	const matrix_3x3& mMat, const vector_3& vTrans ); 
	void				SetWorldMatrixTranslation( const vector_3& vTrans ); 
	void				SetWorldMatrixRotation(	const matrix_3x3& mMat ); 
	void				TranslateWorldMatrix( const vector_3& vTrans );
	void				RotateWorldMatrix( const matrix_3x3& mMat );
	void				ScaleWorldMatrix( const vector_3& vScale );

	// Assistance with manual transformation
	void				GetWorldTextureMatrix( D3DMATRIX& mat );
	void				GetWorldViewMatrix( D3DMATRIX& mat );
	void				GetWorldMatrix( D3DMATRIX& mat );
	void				GetTextureMatrix( D3DMATRIX& mat );
	D3DMATRIX&			GetCombinedMatrix()										{ return m_ObserverComb; }

	void				BuildCombinedTransformationMatrix( bool bZbias = false );

	// Matrix functions for stack manipulation
	void				PushWorldMatrix( const matrix_3x3& mMat, const vector_3& vTrans );
	void				PushWorldMatrix();
	void				PopWorldMatrix();

	// Shadow related function.  Enables the stencil buffer, if there is one,
	// and sets up all drawing code to write into the stencil planes.  Note
	// that the Enable call will return whether or not stencil is available.
	// When using these functions, the Enable call should be done right before
	// you draw your geometry, and the Disable call should be called when you
	// are finished.  This is very important, as we do not want to leave the
	// stencil buffer active unless we are using it.
	bool				EnableStencilBuffer( unsigned int reference );
	void				DisableStencilBuffer();

	// Helper to determine the number of primitives from the number of verts for any given type
	DWORD				GetNumPrimitives( DP_TYPE pType, DWORD numVertices );

	// Primitive drawing routines
	// vertType is one of the block of defines that tells me what type of vertex you are using.
	void				DrawPrimitive(	DP_TYPE			pType,				// Type of primitive to draw
										void*			pVertices,			// Vertices to draw
										unsigned int	numVertices,		// Number of vertices in the list
										unsigned int	vertType,			// Type of vertices
										unsigned int*	pTex,				// Textures to draw with.  Set to NULL if no texture is needed.
										unsigned int	numTex );				

	void				DrawIndexedPrimitive(	DP_TYPE			pType,		// Type of primitive to draw
												void*			pVertices,	// Vertices to use
												unsigned int	numVertices,// Number of vertices in the list
												unsigned int	vertType,	// Type of vertices
												unsigned short*	pIndices,	// Indices to draw with
												unsigned int	numIndices,	// Number of indices
												unsigned int*	pTex,		// Textures to use
												unsigned int	numTex );

	// Request vertices from the main buffer of a given size
	bool				LockBufferedVertices( const unsigned int numVerts, unsigned int& startIndex, sVertex*& pVerts );
	void				UnlockBufferedVertices();

	// Request vertices from the main buffer of a given size and fill them in using the source information.  This routine
	// assumes that the buffer you pass in (pSrcVerts) is aligned on a 64-byte boundary (or at least can handle having
	// memory copied from it on such a boundary).  It is not necessary (and should not be done) to unlock the buffer after
	// this call, as all copying, locking, and unlocking is handled internally.
	bool				CopyBufferedVertices( const unsigned int numVerts, unsigned int& startIndex, sVertex* pSrcVerts );

	void				DrawBufferedPrimitive(	DP_TYPE			pType,
												unsigned int	startIndex,
												unsigned int	numVerts );

	void				DrawBufferedIndexedPrimitive(	DP_TYPE			pType,
														unsigned int	startIndex,
														unsigned int	numVerts,
														unsigned short*	pIndices,
														unsigned int	numIndices );

	// Lighting buffered verts
	bool				LockBufferedLVertices( const unsigned int numVerts, unsigned int& startIndex, lVertex*& pVerts );
	void				UnlockBufferedLVertices();

	void				DrawBufferedLPrimitive(	DP_TYPE			pType,
												unsigned int	startIndex,
												unsigned int	numVerts );

	void				DrawBufferedIndexedLPrimitive( DP_TYPE			pType,
													   unsigned int		startIndex,
													   unsigned int		numVerts,
													   unsigned short*	pIndices,
													   unsigned int		numIndices );

	// Transformed buffer verts
	bool				LockBufferedTVertices( const unsigned int numVerts, unsigned int& startIndex, tVertex*& pVerts );
	void				UnlockBufferedTVertices();

	void				DrawBufferedTPrimitive(	DP_TYPE			pType,
												unsigned int	startIndex,
												unsigned int	numVerts );

	void				DrawBufferedIndexedTPrimitive( DP_TYPE			pType,
													   unsigned int		startIndex,
													   unsigned int		numVerts,
													   unsigned short*	pIndices,
													   unsigned int		numIndices );

	// Index buffer
	bool				LockIndexBuffer( const unsigned int numIndices, unsigned int& startIndex, WORD*& pIndices );
	void				UnlockIndexBuffer();

	// Function for setting up the state of a texture stage
	void				SetTextureStageState(	int		stage,		// What stage to setup
												TEX_OP	colorop,	// Texturing operation
												TEX_OP  alphaop,	// Alpha operation
												TEX_AD	uaddress,	// Address mode in U direction
												TEX_AD	vaddress,	// Address mode in V direction
												TEX_MGF	magfilt,	// Expansion filter
												TEX_MNF minfilt,	// Shrinking filter
												TEX_MPF mipfilt,	// Mipmap filter
												TEX_BLD srcblend,	// Source blend
												TEX_BLD dstblend,	// Destination blend
												bool	alphatest );// Alphatesting
	void				SetSingleStageState( int stage, D3DTEXTURESTAGESTATETYPE type, DWORD value );

	// Holy craziness Batman!  This is one nasty function, needed to get textures working
	// between multiple Rapi objects.  I don't like it, but I don't have too much of a
	// choice.
	void				CopyTextureFromRenderer( Rapi& renderer, unsigned int tIndex );

	// Paint the back buffer(s) to the front buffer(s)
	void				OnWmPaint( const RECT& dstRect, const RECT& srcRect );

private:

	// Protect my constructor so no one can try to make me
	Rapi( );

	// Destruction of DirectX object
	void				DestroyComponents();
	
	// Initializes needed members
	HRESULT				Init( AdapterDesc &ddobj, HWND hwnd );

	// Enables this device for drawing
	bool				Enable( bool bFullScreen, int fsScreenWidth = 640, int fsScreenHeight = 480, int fsBPP = 16, bool bVsync = true );

	// Disables this device
	void				Disable();

	// Resets the device with the set parameters
	bool				ResetDevice();

	// Pauses/Unpauses drawing for this device
	void				Pause( bool bPause );

	// Invalidates all textures that are currently created.  Used when windowed apps resize.
	void				InvalidateSurfaces();

	// Restore all of the textures that were available before the InvalidateTextures call
	void				RestoreSurfacesAndLights();

	// Remove this font from our internal listing
	void				RemoveFromFontList( RapiFont* pFont );

	// Remove this light from our internal listing
	void				RemoveFromLightList( RapiLight* pLight );

	// Find an empty slot inside of our light list
	inline DWORD		FindEmptyLightSlot();

	// Creation of surfaces.  Since only RapiTexture's can create explicit surfaces, these are protected
	const unsigned int	CreateSurface(	const gpstring		texture_file,
										bool				bMipmapping,
										int					rStage,		
										TEX_OPT				rOptimize,
										bool				bAlpha, 
										bool				bDither,
										int					texDetail,
										bool				bFullLoad,
										gpstring&			errorSink );

	const unsigned int	CreateSurface(	RapiMemImage		*pTakeImage,
										bool				bMipmapping,
										int					rStage,		
										TEX_OPT				rOptimize,
										bool				bAlpha,
										bool				bDither,
										int					texDetail,
										gpstring&			errorSink );

	const unsigned int	CreateSurface(	int					width,
										int					height,
										DWORD				color,	
										TEX_OPT				rOptimize,
										bool				bAlpha, 
										bool				bDither,
										bool				bMatchPrimary,
										bool				bRenderTarget,
										gpstring&			errorSink );

	bool				LoadSurface( const unsigned int surface );
	bool				UnloadSurface( const unsigned int surface );
										
	// Destruction of surfaces
	void				DestroySurface( unsigned int surface );

	// Restore main surfaces
	bool				RestoreMainSurfaces( bool bInfiniteWait = false );

	// Using the current system info, pick a good sys identifier
	void				SetSystemDetailIdentifier();

	// Build a view matrix.  Result is returned in mView.
	void				BuildViewMatrix(const vector_3& vFrom, 
										const vector_3& vAt, 
										const vector_3& vWorldUp,
										D3DMATRIX& mView );

	// Build a projection matrix.  Result is returned in mPersp.
	void				BuildPerspectiveMatrix(	const float fFOV,
												const float fAspect,
												const float fNearPlane,
												const float fFarPlane,
												D3DMATRIX& mPersp );

	// Build a orthographic matrix.  Result is returned in mOrtho.
	void				BuildOrthoMatrix( const float fWidth,
										  const float fHeight,
										  const float fNearPlane,
										  const float fFarPlane,
										  D3DMATRIX& mOrtho );

	// Construct a gamma table from the current gamma
	void				BuildGammaTable();

    // Window handle
    HWND					m_hwnd;

	// Use this critical for pausing to avoid flipping/blting while paused
	kerneltool::Critical	m_pauseCritical;

	// Adapter identification
	D3DADAPTER_IDENTIFIER8	m_deviceId;
	AdapterDesc				m_deviceDesc;

	// Object device description
	D3DPRESENT_PARAMETERS	m_deviceParams;

	// DirectX components
	LPDIRECT3D8				m_pD3D;
    LPDIRECT3DDEVICE8		m_pD3DDevice;

	// Surfaces
	LPDIRECT3DSURFACE8		m_pBackBuffer;
	LPDIRECT3DSURFACE8		m_pZBuffer;

	// Formats
	ePixelFormat			m_primaryPixelFormat;
	D3DFORMAT				m_primaryD3DFormat;

	D3DFORMAT				m_depthD3DFormat;
	unsigned int			m_stencil;

	ePixelFormat			m_opaqueTexPixelFormat;
	D3DFORMAT				m_opaqueTexD3DFormat;

	ePixelFormat			m_alphaTexPixelFormat;
	D3DFORMAT				m_alphaTexD3DFormat;

	// States
	bool					m_bOnGDIBuffer;
	bool					m_bSoftwareTransform;
	bool					m_bTripleBuffering;
	bool					m_bManualTextureProjection;
	bool					m_bManualFrustumClipping;
	bool					m_bOnlySquareTextures;
	bool					m_b16bitOnly;
	bool					m_bForceTexRestore;
	bool					m_bColorControl;
	bool					m_bMultiTexBlend;
	bool					m_bTextureStateReset;
	bool					m_bModulateOnly;
	bool					m_bNoAdd;
	bool					m_bShadowRenderTarget;
	bool					m_bBuffersFrames;
	bool					m_bSceneFade;

	// ZBuffer surface description
	D3DFORMAT				m_ZBufferFormat;
	bool					m_bUseWBuffer;

	// Round Robin list of vertex buffers
	LPDIRECT3DVERTEXBUFFER8 m_pVBuffer;
	unsigned int			m_VBufferPosition;
	bool					m_bVBufferLocked;

	LPDIRECT3DVERTEXBUFFER8 m_pLVBuffer;
	unsigned int			m_LVBufferPosition;
	bool					m_bLVBufferLocked;

	LPDIRECT3DVERTEXBUFFER8 m_pTVBuffer;
	unsigned int			m_TVBufferPosition;
	bool					m_bTVBufferLocked;

	LPDIRECT3DINDEXBUFFER8	m_pIndexBuffer;
	unsigned int			m_IndexBufferPosition;
	bool					m_bIndexBufferLocked;

	DWORD					m_bufferFlags;
	D3DPOOL					m_bufferMem;

	// Size of the screen
    UINT					m_Width;
	UINT					m_Height;
	UINT					m_viewWidth;
	UINT					m_viewHeight;
	UINT					m_viewWidthOffset;
	UINT					m_viewHeightOffset;
	UINT					m_BPP;
	DWORD					m_vidMem;
	DWORD					m_availTexMem;

	// Shadow texture size
	UINT					m_shadowTexSize;

	// System detail identifier
	int						m_sysDetail;
	SystemDetailList		m_sysDetailList;
	ValidModeList			m_validModes;

	// Display mode listing
	DisplayList				m_DisplayList;

	// Mipmap filtering mode
	bool					m_bTrilinearCapable;
	TEX_MPF					m_mipFilter;

	// Specific information that the renderer need about states
	bool					m_bInScene;
	bool					m_bFlipBltOnBegin;
	bool					m_bPaused;
	bool					m_bFullScreen;
	bool					m_bWireframe;
	bool					m_bIgnoreVertexColor;
	bool					m_bVSync;
	bool					m_bFlip;
	bool					m_bFastFPU;
	bool					m_bAlphaBlendEnabled;
	bool					m_bNoTextures;
	bool					m_bNoRender;
	bool					m_bAsyncCursor;
	bool					m_bEdit;
	bool					m_bSimdSupport;

	// Current frame count
	unsigned int			m_FrameCount;

	// Clear color
	DWORD					m_ClearColor;

	// Gamma info
	vector_3				m_Gamma;
	float					m_SystemGamma;

	// Fade info
	float					m_Fade;

	// Ambient lighting
	DWORD					m_ambience;

	// Frame rate information
	float					m_frameRate;
	double                  m_startTime;
	double					m_elapsedTime;

	// One second timer.  Runs up to one second and then resets.  Used to
	// keep animated texture information synched across the system
	double					m_secondTime;

	// List of textures
	TextureBucketVector		m_Textures;
	RapiTexture*			m_ActiveTex[ NUM_TEXTURES ];
	kerneltool::Critical	m_TextureCritical;

	// State tracking
	TextureStageState		m_TextureStageStates[ NUM_TEXTURES ];
	bool					m_bAlphaTest;
	TEX_BLD					m_srcBlend;
	TEX_BLD					m_dstBlend;

	// List of surfaces
	SurfaceBucketVector		m_Surfaces;
	kerneltool::Critical	m_SurfaceCritical;
	kerneltool::Critical	m_RestoreCritical;

	// Texture mapping for database control
	TextureMap				m_TextureMap;
	SurfaceMap				m_SurfaceMap;
	SurfaceExpirationMap	m_SurfaceExpirationMap;
	float					m_SurfaceExpirationTime;

	// Active environment texture
	unsigned int			m_EnvironmentTexture;

	// Active texture logging
	bool					m_bActiveTextureLog;
	TextureSet				m_ActiveTextureLog;

	// Saturated set
	TextureSet				m_SaturatedTextureSet;
	float					m_Saturation;

	// List of lights
	RapiLight*				m_Lights[ LIGHT_CACHE ];

	// List of fonts
	FontList				m_Fonts;

	// Matrix stack
	MatrixStack				m_MatrixStack;

	// Helpful information
	unsigned int			m_TextureSwitches;
	unsigned int			m_NumActiveTextures;
	unsigned int			m_TrianglesDrawn;
	unsigned int			m_VerticesTransformed;
	unsigned int			m_TextureMemory;
	unsigned int			m_TexSystemMemory;
	unsigned int			m_TexManagerSystemMemory;

	// Matrices to define the light view
	D3DMATRIX				m_LightView;
	D3DMATRIX				m_LightProj;

	// Matrices to define the observer view
	float					m_ObserverFOV;
	D3DMATRIX				m_ObserverView;
	D3DMATRIX				m_ObserverProj;
	D3DMATRIX				m_ObserverZbiasProj;

	// Combined matrices for speed improvements
	D3DMATRIX				m_ObserverComb;

	// Hardware capabilities
	D3DCAPS8				m_d3dDevDesc;

	// Fog & Clip plane settings 
	eFogType				m_fogType;
	bool					m_bFogIsActive;
	
	DWORD					m_fogColor;

	float					m_fogNearDist;
	float					m_fogFarDist;

	float					m_fogDensity;

	float					m_clipNearDist;
	float					m_clipFarDist;

	float					m_clipToFogRatio;

	// Mouse
	RapiMouse*				m_pRapiMouse;

	// Primary thread ID
	DWORD					m_primaryThreadId;

};

#endif
