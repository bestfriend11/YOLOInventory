#pragma once
#ifndef _RAPI_SURFACE_
#define _RAPI_SURFACE_

/*******************************************************************************************
**
**									RapiSurface
**
**		The RapiSurface class in responsible for managing a single surface and provides
**		methods for parsing their information.  Multiple RapiSurfaces may make up a
**		single RapiTexture, which is why they have been separated.
**
**	Author:		James Loe
**	Date:		04/12/00
**
*******************************************************************************************/

class RapiMemImage;
class RapiImageReader;

class RapiSurface
{
	friend class Rapi;

public:

	~RapiSurface();

	// Get the surface
	LPDIRECT3DTEXTURE8			GetSurface()						{ return m_bValid ? m_pSurface : NULL; }

	// See if the surface is valid
	bool						IsValid()							{ return m_bValid; }

	// Get the pathname of this texture
	inline gpstring&			GetPathname()						{ return m_Pathname; }

	// Does this surface have mipmapping enabled?
	inline bool					HasMipLevels()						{ return m_bMipmapping; }

	// Get the surface's bit information
	RapiMemImage*				GetTextureBits()					{ return m_Image; }

	// Get height and width information
	unsigned int				GetWidth()							{ return m_Width; }
	unsigned int				GetHeight()							{ return m_Height; }

	// Get the surface's stage
	inline int					GetStage()							{ return m_rStage; }

	// Get the surface's optimizations
	inline TEX_OPT				GetOptimization()					{ return m_texOpt; }

	// Get surface infor
	inline bool					GetMatchPrimary()					{ return m_bMatchPrimary; }
	inline bool					GetRenderTarget()					{ return m_bRenderTarget; }

	// Invalidate the surface
	void						InvalidateSurface();

	// Restore this surface
	bool						RestoreSurface( gpstring* pErrorSink = NULL );

	// Setup the surface
	bool						SetupSurface( RapiImageReader* reader = NULL );

	// Reference counting
	unsigned int				GetReferenceCount()					{ return m_refCount; }
	void						IncrementReferenceCount()			{ m_refCount++; }
	void						DecrementReferenceCount()			{ m_refCount--; }

	unsigned int				GetLoadReferenceCount()				{ return m_loadRefCount; }
	void						IncrementLoadReferenceCount()		{ ++m_loadRefCount; }
	void						DecrementLoadReferenceCount()		{ --m_loadRefCount; }

	// Active texture info
	bool						GetActive()							{ return m_bActive; }
	void						SetActive( bool bActive )			{ m_bActive = bActive; }

	// Saturation
	void						SetSaturation( float saturation );
	void						RevertSaturation( bool bReset );

private:

	// Construction of file based texture
	RapiSurface(	Rapi*				pOwner,
					const gpstring&	filename,
					bool				bMipmapping,
					int					rStage,
					TEX_OPT				rOptimize,
					bool				bAlpha,
					bool				bDither,
					int					texDetail,
					bool				bFullLoad,
					gpstring&			errorSink );

	// Construction of algorithmic texture
	RapiSurface(	Rapi*				pOwner,
					RapiMemImage*		takeImage,
					bool				bMipmapping,
					int					rStage,
					TEX_OPT				rOptimize,
					bool				bAlpha,
					bool				bDither,
					int					texDetail,
					gpstring&			errorSink );

	// Construction of blank texture
	RapiSurface(	Rapi*				pOwner,
					int					width,
					int					height,
					DWORD				color,
					TEX_OPT				rOptimize,
					bool				bAlpha,
					bool				bDither,
					bool				bMatchPrimary,
					bool				bRenderTarget,
					gpstring&			errorSink );

	// Cleanup all information for this surface
	void						Destroy();

	// My owner and device
	Rapi*						m_pDevice;

	// Valid surface
	bool						m_bValid;

	// Pointer to surface
	LPDIRECT3DTEXTURE8			m_pSurface;
	LPDIRECT3DTEXTURE8			m_pOriginalSurface;

	// GAS relative pathname
	gpstring					m_Pathname;

	// Handle to image
	my RapiMemImage*			m_Image;
	unsigned int				m_Width;
	unsigned int				m_Height;

	// Fill color for blanks
	DWORD						m_Color;

	// Does this surface contain alpha important information
	bool						m_bAlpha;

	// Should this surface match the primary surface
	bool						m_bMatchPrimary;

	// Is this surface a potential render target?
	bool						m_bRenderTarget;

	// Should this surface dither down when converting to 16 bits?
	bool						m_bDither;

	// Texture detail identifier for system specific texture changes
	int							m_texDetail;

	// Mipmapping
	bool						m_bMipmapping;

	// Hint as to what stage this surface will be used for
	int							m_rStage;

	// What surface optimizations do I need
	TEX_OPT						m_texOpt;

	// Reference counter to control destruction
	unsigned int				m_refCount;
	unsigned int				m_loadRefCount;

	// Whether or not this surface was active this frame
	bool						m_bActive;

	// Saturation
	float						m_saturation;

	// Owning system
#	if GP_ENABLE_STATS
	eSystemProperty				m_OwningSystem;
#	endif // GP_ENABLE_STATS
};

#endif