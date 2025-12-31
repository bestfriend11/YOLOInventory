/**************************************************************************

Filename		: ui_textureman.h

Description		: This is the user inteface texture management class.
				  It records and tracks all textures used in order to
				  prevent duplicate creation.

Creation Date	: 9/19/99

**************************************************************************/


#pragma once
#ifndef _UI_TEXTUREMAN_H_
#define _UI_TEXTUREMAN_H_



// Structures
struct UI_TEXTURE
{
	gpstring			name;
	unsigned int		index;
	int					ref_count;
	gpstring			interface_name;
	int					numFrames;
	int					framesPerSec;
};

struct UI_FONT
{
	gpstring			name;
	RapiFont			*pFont;
	int					ref_count;
};


struct FontType
{
	gpstring	sTypeface;
	int			height;
};

typedef std::map< gpstring, FontType, istring_less > LocFontMap;
typedef std::pair< gpstring, FontType > LocFontPair;


// Defines
#define ui_texture_vector std::vector< UI_TEXTURE >


// Class Definition
class UITextureManager : public Singleton <UITextureManager>
{
public:

	// Public methods

	// Constructor
	UITextureManager( Rapi *renderer );

	// Destructor 
	~UITextureManager();

	// Load a texture in and register it with the renderer
	// If the texture already exists, do not load another copy; 
	// instead just return the appropriate index
	const unsigned int LoadTexture( gpstring texture_name, gpstring interface_name = gpstring::EMPTY, bool bCursor = false );	
	std::vector< unsigned int > LoadAnimatedTextures(	gpstring texture_name, gpstring interface_name,
														unsigned int &framepersec, unsigned int &numFrames, bool bCursor = false );

	// Destroy a texture
	void DestroyAllTextures();
	void DestroyFont( RapiFont * pFont );

	// Set the invalid texture
	void SetInvalidTexture( gpstring texture_name );
	const unsigned int GetInvalidTexture() { return m_invalid.index; }

	// Load in a font
	RapiFont * LoadFont( gpstring sFont, bool bFailIfNotFound = false );
	RapiFont * LoadFont( gpstring sFont, int height, bool bAntiAlias = false );
	RapiFont * LoadFont( gpstring sFont, int height, DWORD dwBgColor );

	RapiFont * GetDefaultFont(); 

	RapiFont * ChangeFontAttributes( RapiFont * pFont, unsigned int height, bool bBold );

private:

	// Private Member Variables
	std::vector< UI_TEXTURE >	m_textures;				// Texture List 
	Rapi						*m_renderer;			// Renderer	
	UI_TEXTURE					m_invalid;				// Holds the index for the invalid texture
	std::vector< UI_FONT	>	m_fonts;				// Font List
	RapiFont *					m_pDefaultFont;
	LocFontMap					m_locFontMap;

};

#define gUITextureManager UITextureManager::GetSingleton()


#endif
