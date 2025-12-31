//////////////////////////////////////////////////////////////////////////////
//
// File     :  Textureman.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "textureman.h"
#include "rapiowner.h"
#include "img.h"
#include "imgimage.h"
#include "tank.h"
#include "fuel.h"


TextureManager::TextureManager( Rapi *renderer, IMG *img )
: m_renderer( renderer )
, m_img( img )
{
	m_invalid.ref_count = 0;
	m_invalid.name.assign( "" );
	m_invalid.index = 0;
}


TextureManager::~TextureManager()
{
	DestroyAllTextures();
}


const unsigned int TextureManager::LoadTexture( gpstring texture_name )
{
	GENERIC_TEXTURE search_texture = FindTexture( texture_name );
	if ( search_texture.index != m_invalid.index ) {
		search_texture.ref_count++;
		return search_texture.index;
	}

	GENERIC_TEXTURE texture;
	texture.index			= m_renderer->CreateTexture( texture_name, false, 0, TEX_STATIC );
	texture.name			= texture_name;
	texture.ref_count++;
	m_textures.push_back( texture );

	return texture.index;
}
const unsigned int TextureManager::LoadTexture( gpstring texture_name, vector_3 color )
{
	UNREFERENCED_PARAMETER( color );
	GENERIC_TEXTURE search_texture = FindTexture( texture_name );
	if ( search_texture.index != m_invalid.index ) {
		search_texture.ref_count++;
		return search_texture.index;
	}

	GENERIC_TEXTURE texture;
	texture.index		= m_renderer->CreateTexture( texture_name, false, 0, TEX_STATIC );
	texture.name		= texture_name;
	texture.ref_count++;
	m_textures.push_back( texture );

	return texture.index;
}


void TextureManager::DestroyTexture( const unsigned int texture_index )
{
	texture_vector::iterator i;
	for ( i = m_textures.begin(); i != m_textures.end(); ++i ) {
		if ( (*i).index == texture_index ) {
			(*i).ref_count--;
			if ( (*i).ref_count <= 0 ) {
				m_renderer->DestroyTexture( texture_index );
				m_textures.erase( i );
			}			
		}
	}	
}
void TextureManager::DestroyTexture( gpstring texture_name )
{
	texture_vector::iterator i;
	for ( i = m_textures.begin(); i != m_textures.end(); ++i ) {
		if ( (*i).name.same_no_case( texture_name ) ) {
			(*i).ref_count--;
			if ( (*i).ref_count <= 0 ) {
				m_renderer->DestroyTexture( (*i).index );
				m_textures.erase( i );
			}			
		}
	}
}


void TextureManager::DestroyAllTextures()
{
	texture_vector::iterator i = m_textures.begin();
	while ( i != m_textures.end() ) {
		m_renderer->DestroyTexture( (*i).index );
		i = m_textures.erase( i );			
	}
	
	if ( m_invalid.ref_count != 0 ) {
		m_renderer->DestroyTexture( m_invalid.index );
	}	
}


GENERIC_TEXTURE TextureManager::FindTexture( gpstring texture_name )
{
	texture_vector::iterator i;
	for ( i = m_textures.begin(); i != m_textures.end(); ++i ) {
		if ( (*i).name.same_no_case( texture_name ) ) {
			return (*i);
		}
	}
	return m_invalid;
}


void TextureManager::SetInvalidTexture( gpstring texture_name )
{	
	m_invalid.index = m_renderer->CreateTexture( texture_name, false, 0, TEX_STATIC );
	m_invalid.name	= texture_name;
	m_invalid.ref_count++;
}


/**************************************************************************

Function		: LoadFont()

Purpose			: Load a font into the texture manager

Returns			: RapiFont*

**************************************************************************/
RapiFont * TextureManager::LoadFont( gpstring sFont )
{
	std::vector< GENERIC_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( (*i).name.same_no_case( sFont ) ) {
			return (*i).pFont;
		}
	}
	
	char szFont[1024] = "";
	sprintf( szFont, "ui:fonts:%s", sFont.c_str() );
	FuelHandle fhFont( szFont );
	
	int height		= 0;
	int startRange	= 0;
	int endRange	= 0;
	gpstring texture;

	fhFont->Get( "height", height );
	fhFont->Get( "startrange", startRange );
	fhFont->Get( "endrange", endRange );
	fhFont->Get( "texture", texture );

	//Tank::GRelativeToAbsolutePath( texture );
	IMGImage *image = GetIMG()->ReadImage( texture );
	
	GENERIC_FONT font;
	font.name = sFont;
	font.pFont = m_renderer->CreateFont( image, sFont, height, startRange, endRange );

	m_fonts.push_back( font );

	return font.pFont;
}