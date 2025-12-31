#pragma once
#ifndef _RAPI_FONT_
#define _RAPI_FONT_
/*************************************************************************************
**
**									RapiFont
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	06/14/99
**		Author: James Loe
**
*************************************************************************************/

// Forward Declarations
class RapiMemImage;

struct UVRect
{
	float left;
	float top;
	float right;
	float bottom;
};


struct TTFTextComponent
{
	unsigned int	texture;
	RECT			rect;
	UVRect			uvrect;	
};

typedef std::vector< TTFTextComponent > TextComponents;

struct RapiTrueTypeText
{	
	gpwstring				sText;	
	TextComponents			textComponents;	
	int						refCount;
};

typedef std::vector< RapiTrueTypeText > TextCache;


// This class was, for the most part, ripped off from Casey's win32_opengl_font
class RapiFont
{
	friend class Rapi;

public:

	~RapiFont(void);

	// Sizing
	void CalculateStringSize(	char const * const String,
								int &Width, 
								int &Height);
	void CalculateStringSize(	gpwstring sText, int & width, int & height );

	// Printing
	void Print(	int StartX,	int StartY,
				char const * const String,
				DWORD dwColor = 0xFFFFFFFF,
				bool bUseLinear = false );
	void Print( int StartX, int StartY,
				gpwstring sText, 
				DWORD dwColor = 0xFFFFFFFF,
				bool bUseLinear = false );	
	
	void RenderTrueType( RapiTrueTypeText & ttt, DWORD dwColor, int StartX, int StartY );

	void Print3D( vector_3& textPos,
				  char const * const String, const float textHeightInMeters,
				  DWORD dwColor = 0xFFFFFFFF );

	// Modification
	void BuildFromWin32Font(HDC hdc, HFONT font);
	void BuildFromTexture( unsigned int texture );

	unsigned int	GetTextureIndex()	{ return m_Texture;		}
	gpstring		GetTextureName()	{ return m_sTexture;	}

	HFONT GetHFont() { return m_hFont; }

	void RegisterString( gpwstring sText );
	void UnregisterString( gpwstring sText );

	void	SetBackgroundColor( DWORD dwColor ) { m_dwBgColor = dwColor; }
	DWORD	GetBackgroundColor()				{ return m_dwBgColor; }

	void ResetSurfaces();
	
private:

	// Existence
	RapiFont(	Rapi*			pDevice,
				HDC				hdc, 
				HFONT			font,
				bool			forceBitmap );
	RapiFont(	Rapi*			pDevice, 
				unsigned int	texture,
				UINT32  		characterHeight,
				UINT32  		startRange,
				UINT32  		endRange,
				gpstring		sTexture );

	enum {NumberOfCharacters = 256, DropShadowX = 1, DropShadowY = 1};

	bool BuildCharacterTables( unsigned int texture );
	bool BuildCharacterTables( HDC hdc );	
	bool BuildCharacterImages( HDC hdc, HFONT font );

	// Rapi pointer
	Rapi*						m_pDevice;

	// Font information	
	unsigned int				m_Texture;
	int							m_characterHeightInPixels;
	int							m_characterXOffsetInPixels[NumberOfCharacters];
	int							m_characterYOffsetInPixels[NumberOfCharacters];
	int							m_characterWidthInPixels[NumberOfCharacters];
	int							m_textureWidth, m_textureHeight;
	int							m_startRange, m_endRange;
	bool						m_bCoordinateFlip;
	gpstring					m_sTexture;
	HFONT						m_hFont;	
	TextCache					m_textCache;
	DWORD						m_dwBgColor;

	// for non-bitmapped fonts
	RapiMemImage *				m_pImage;

};

#endif