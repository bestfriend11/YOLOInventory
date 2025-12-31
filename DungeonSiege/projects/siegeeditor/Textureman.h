//////////////////////////////////////////////////////////////////////////////
//
// File     :  Textureman.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __TEXTUREMAN_H
#define __TEXTUREMAN_H

#include "gpcore.h"
#include <string>
#include <vector>
#include "vector_3.h"

class RapiFont;
class Rapi;
class IMG;


// Structures
struct GENERIC_TEXTURE
{
	gpstring			name;
	unsigned int		index;
	int					ref_count;	
};

struct GENERIC_FONT
{
	gpstring			name;
	RapiFont			*pFont;
};


// Defines
#define texture_vector std::vector< GENERIC_TEXTURE >


// Class Definition
class TextureManager
{
public:

	// Public methods

	// Constructor
	TextureManager( Rapi *renderer, IMG *img );

	// Destructor 
	~TextureManager();

	// Load a texture in and register it with the renderer
	// If the texture already exists, do not load another copy; 
	// instead just return the appropriate index
	const unsigned int LoadTexture( gpstring texture_name );
	const unsigned int LoadTexture( gpstring texture_name, vector_3 color );

	// Destroy a texture
	void DestroyTexture( const unsigned int texture_index	);
	void DestroyTexture( gpstring		texture_name	);
	void DestroyAllTextures();
	
	// Find a texture based on name
	GENERIC_TEXTURE FindTexture( gpstring texture_name );

	// Set the invalid texture
	void SetInvalidTexture( gpstring texture_name );
	
	// Load in a font
	RapiFont * LoadFont( gpstring sFont );

private:

	// Get Access to IMG
	IMG * GetIMG() { return m_img; };

	// Private Member Variables
	std::vector< GENERIC_TEXTURE >	m_textures;				// Texture List 
	Rapi							*m_renderer;			// Renderer
	IMG								*m_img;					// Graphics loader
	GENERIC_TEXTURE					m_invalid;				// Holds the index for the invalid texture
	std::vector< GENERIC_FONT	>	m_fonts;				// Font List

};


#endif
