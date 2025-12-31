/**************************************************************************

Filename		: ui_textureman.cpp

Description		: This is the user inteface texture management class.
				  It records and tracks all textures used in order to
				  prevent duplicate creation.

Creation Date	: 9/19/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "FileSysDefs.h"
#include "fuel.h"
#include "rapiowner.h"
#include "rapimouse.h"
#include "rapiimage.h"
#include "ui_textureman.h"
#include "ui_shell.h"
#include "LocHelp.h"
#include "namingkey.h"


inline gpstring GetFullTextureName( const gpstring& contentname, const gpstring& nameext ) {

	gpstring fullfilename;

	if ( contentname.size() > 2 && toupper(contentname[0]) == 'B' && contentname[1] == '_') {

		if (gNamingKey.BuildContentLocation(contentname.c_str(),fullfilename)) {
			fullfilename += nameext;
		} else {
			gperrorf(("Failed to locate %s. Is the CDIMAGE Naming Key up to date?",contentname.c_str()));
			fullfilename.clear();	// Not a valid location
		}

	} else {

		// This is a legacy texture, we might want to print a warning
		fullfilename = contentname;

	}

	return fullfilename;
}

/**************************************************************************

Function		: UITextureManager()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UITextureManager::UITextureManager( Rapi *renderer )
: m_renderer( renderer )
, m_pDefaultFont( 0 )
{
	m_invalid.ref_count = 0;
	m_invalid.name.assign( "" );
	m_invalid.index = 0;
	m_pDefaultFont = renderer->CreateFont();

	FastFuelHandle hSettings( "config:global_settings:gui_font_map" );
	if ( hSettings.IsValid() )
	{
		FastFuelFindHandle hFind( hSettings );
		if ( hFind.FindFirstKeyAndValue() )
		{
			gpstring sKey;
			gpstring sValue;
			while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
			{
				gpstring sFont;
				int height = 0;
				stringtool::GetDelimitedValue( sValue, ',', 0, sFont, false );
				stringtool::GetDelimitedValue( sValue, ',', 1, height );
				FontType ft;
				ft.height = height;
				ft.sTypeface = sFont;
				m_locFontMap.insert( LocFontPair( sKey, ft ) );
			}
		}		
	}
}


/**************************************************************************

Function		: ~UITextureManager()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UITextureManager::~UITextureManager()
{
	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) 
	{
		if ( (*i).pFont && (*i).pFont->GetHFont() )
		{
			DeleteObject( (*i).pFont->GetHFont() );
		}		
	}

	DestroyAllTextures();
	delete m_pDefaultFont;
	m_pDefaultFont = 0;	
}


/**************************************************************************

Function		: LoadTexture()

Purpose			: Load a texture in if it does not exist and return its
				  index; otherwise if it is already loaded, just return
				  its index

Returns			: const unsigned int

**************************************************************************/
const unsigned int UITextureManager::LoadTexture( gpstring texture_name, gpstring interface_name, bool bCursor )
{
	// Try to locate a 'new' texture name using the NamingKey
	gpstring filename = GetFullTextureName( texture_name, ".%img%" );

	if ( filename.empty() ) 
	{
		m_renderer->AddTextureReference( m_invalid.index );
		return m_invalid.index;
	}
	
	int index = 0;
	
	if ( !bCursor )
	{
		index = m_renderer->CreateTexture( filename, false, 0, TEX_STATIC );
	}
	else
	{
		index = gRapiMouse.CreateCursorImage( filename );
	}

	return index;
}


/**************************************************************************

Function		: LoadAnimatedTextures()

Purpose			: Destroys a texture within its internal list and the
				  renderer

Returns			: void

**************************************************************************/
std::vector< unsigned int > UITextureManager::LoadAnimatedTextures( gpstring texture_name, gpstring interface_name,
																	unsigned int &framepersec, unsigned int &numFrames, bool bCursor )
{
	std::vector< unsigned int > textures;

	// Try to locate a 'new' texture name using the NamingKey
	gpstring filename = GetFullTextureName( texture_name, ".FLM" );

	if ( filename.empty() ) 
	{		
		return textures;
	}

	std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( filename, false, false ) );
	if ( reader.get() != NULL )
	{
		RapiImageReader::ImageColl images;
		reader->GetAllSurfaces( images, false );

		RapiImageReader::ImageColl::const_iterator i, ibegin = images.begin(), iend = images.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			int index = 0;
			
			if ( !bCursor )
			{
				index = m_renderer->CreateAlgorithmicTexture( texture_name, *i, false, 0, TEX_STATIC, true, true );
			}
			else
			{
				index = gRapiMouse.CreateCursorImage( *i );
			}

			textures.push_back( index );
		}

		framepersec = reader->GetFps();
		numFrames	= images.size();
	}

	return textures;
}


/**************************************************************************

Function		: DestroyAllTextures()

Purpose			: Destroys all existing textures

Returns			: void

**************************************************************************/
void UITextureManager::DestroyAllTextures()
{	
	if ( m_invalid.index != 0 )
	{
		m_renderer->DestroyTexture( m_invalid.index );	
	}	
}


/**************************************************************************

Function		: SetInvalidTexture()

Purpose			: Load a particular image in as the invalid texture

Returns			: const unsigned int

**************************************************************************/
void UITextureManager::SetInvalidTexture( gpstring texture_name )
{
	// Try to locate a 'new' texture name using the NamingKey
	gpstring filename = GetFullTextureName( texture_name, ".%img%" );

	// Previous code didn't bother to check if the resource was there either -- biddle

	m_invalid.index = m_renderer->CreateTexture( filename, false, 0, TEX_STATIC );
	m_invalid.name	= texture_name;
	m_invalid.ref_count++;
}


/**************************************************************************

Function		: LoadFont()

Purpose			: Load a font into the texture manager

Returns			: RapiFont*

**************************************************************************/
RapiFont * UITextureManager::LoadFont( gpstring sFont, bool bFailIfNotFound )
{
	if ( sFont.empty() )
	{
		return 0;
	}	

	if ( (LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi()) )
	{
		LocFontMap::iterator iFound = m_locFontMap.find( sFont );
		if ( iFound != m_locFontMap.end() )
		{
			return LoadFont( (*iFound).second.sTypeface, (*iFound).second.height );			
		}
		else
		{
			iFound = m_locFontMap.find( "default" );
			if ( iFound != m_locFontMap.end() )
			{
				return LoadFont( (*iFound).second.sTypeface, (*iFound).second.height );			
			}
		}
	}

	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( (*i).name.same_no_case( sFont ) ) {
			(*i).ref_count++;
			return (*i).pFont;
		}
	}
	
	char szFont[1024] = "";
	sprintf( szFont, "ui:fonts:%s", sFont.c_str() );
	FastFuelHandle fhFont( szFont );

	if ( !fhFont ) 
	{
		return 0;
	}
	
	int height		= 0;
	int startRange	= 0;
	int endRange	= 0;
	gpstring texture;

	fhFont.Get( "height", height );
	fhFont.Get( "startrange", startRange );
	fhFont.Get( "endrange", endRange );
	fhFont.Get( "texture", texture );

	UI_FONT ui_font;
	ui_font.name = sFont;
	ui_font.ref_count = 1;

	// Try to locate a 'new' texture name using the NamingKey
	gpstring filename = GetFullTextureName( texture, ".%img%" );

	if ( filename.empty() ) 
	{
		// use default
		if ( bFailIfNotFound == false ) {
			ui_font.pFont = GetDefaultFont();
			gperrorf(( "Error loading font '%s', using default instead ( which is bad. )\n", sFont.c_str() ));
		}
		else {
			return 0;
		}
	}
	
	int fontTex = m_renderer->CreateTexture( filename, false, 0, TEX_STATIC );
	if ( fontTex != NULL )
	{
		ui_font.pFont = m_renderer->CreateFont( fontTex, sFont, height, startRange, endRange );
	}
	else
	{
		if ( bFailIfNotFound == false ) {
			// use default
			ui_font.pFont = GetDefaultFont();
			gperrorf(( "Error loading font '%s', using default instead ( which is bad. )\n", sFont.c_str() ));
		}
		else {
			return 0;
		}
	}

	m_fonts.push_back( ui_font );

	return ui_font.pFont;
}



RapiFont * UITextureManager::LoadFont( gpstring sFont, int height, bool bAntiAlias )
{
	if ( sFont.empty() )
	{
		return 0;
	}

	gpstring sNewFont;
	sNewFont.assignf( "%s_%d", sFont.c_str(), height );	
	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( (*i).name.same_no_case( sNewFont ) ) {
			(*i).ref_count++;
			return (*i).pFont;
		}
	}

	UI_FONT ui_font;

	HFONT hFont = CreateFont(	height, 
								0, 
								0, 
								0, 								
								FW_NORMAL, 
								false, 
								false, 
								false, 
								LocMgr::DoesSingletonExist() ? gLocMgr.GetCharSet() : DEFAULT_CHARSET, 
								OUT_TT_ONLY_PRECIS, 
								CLIP_DEFAULT_PRECIS,
								bAntiAlias ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY, 
								DEFAULT_PITCH | FF_DONTCARE, 
								sFont.c_str() );
	ui_font.pFont = m_renderer->CreateFont( hFont );
	ui_font.name = sNewFont;
	ui_font.ref_count = 1;

	if ( !LocMgr::DoesSingletonExist() || ( LocMgr::DoesSingletonExist() && gLocMgr.IsAnsi() ) )
	{
		DeleteObject( hFont );
	}

	m_fonts.push_back( ui_font );
	return ui_font.pFont;
}


RapiFont * UITextureManager::LoadFont( gpstring sFont, int height, DWORD dwBgColor )
{
	if ( sFont.empty() )
	{
		return 0;
	}

	gpstring sNewFont;	
	sNewFont.assignf( "%s_%d_antialias_0x%x", sFont.c_str(), height, dwBgColor );	
	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( (*i).name.same_no_case( sNewFont ) ) {
			(*i).ref_count++;
			return (*i).pFont;
		}
	}

	UI_FONT ui_font;
	HFONT hFont = CreateFont(	height, 
								0, 
								0, 
								0, 								
								FW_NORMAL, 
								false, 
								false, 
								false, 
								LocMgr::DoesSingletonExist() ? gLocMgr.GetCharSet() : DEFAULT_CHARSET, 
								OUT_TT_ONLY_PRECIS, 
								CLIP_DEFAULT_PRECIS,
								ANTIALIASED_QUALITY, 
								DEFAULT_PITCH | FF_DONTCARE, 
								sFont.c_str() );		
	
	ui_font.pFont = m_renderer->CreateFont( hFont );
	ui_font.name = sNewFont;
	ui_font.ref_count = 1;
	m_fonts.push_back( ui_font );
	ui_font.pFont->SetBackgroundColor( dwBgColor );

	if ( !LocMgr::DoesSingletonExist() || ( LocMgr::DoesSingletonExist() && gLocMgr.IsAnsi() ) )
	{
		DeleteObject( hFont );
	}

	return ui_font.pFont;	
}


void UITextureManager::DestroyFont( RapiFont * pFont )
{
	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) 
	{
		if ( (*i).pFont == pFont )
		{
			(*i).ref_count--;
			if ( (*i).ref_count == 0 )
			{
				if ( pFont->GetHFont() )
				{
					DeleteObject( pFont->GetHFont() );
				}
				Delete( pFont );
				m_fonts.erase( i );
			}
			return;
		}
	}
}


RapiFont * UITextureManager::ChangeFontAttributes( RapiFont * pFont, unsigned int height, bool bBold )
{	
	std::vector< UI_FONT >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) 
	{
		if ( (*i).pFont == pFont )
		{
			gpstring sFont;
			int pos = (*i).name.find_last_of( "_" );
			sFont = (*i).name.substr( 0, pos );

			gpstring sNewFont;	
			if ( bBold )
			{
				sNewFont.assignf( "%s_%dbold", sFont.c_str(), height );	
			}
			else
			{
				sNewFont.assignf( "%s_%d", sFont.c_str(), height );	
			}

			std::vector< UI_FONT >::iterator j;
			for ( j = m_fonts.begin(); j != m_fonts.end(); ++j ) {
				if ( (*j).name.same_no_case( sNewFont ) ) 
				{
					return (*j).pFont;
				}
			}

			HFONT hFont = CreateFont(	height, 
										0, 
										0, 
										0, 								
										bBold ? FW_BOLD : FW_NORMAL, 
										false, 
										false, 
										false, 
										LocMgr::DoesSingletonExist() ? gLocMgr.GetCharSet() : DEFAULT_CHARSET, 
										OUT_TT_ONLY_PRECIS, 
										CLIP_DEFAULT_PRECIS,
										NONANTIALIASED_QUALITY, 
										DEFAULT_PITCH | FF_DONTCARE, 
										sFont.c_str() );

			if ( bBold )
			{
				sFont.appendf( "_%dbold", height );
			}
			else
			{
				sFont.appendf( "_%d", height );
			}
			

			UI_FONT ui_font;
			ui_font.pFont = m_renderer->CreateFont( hFont );
			ui_font.name = sFont;

			m_fonts.push_back( ui_font );
			return ui_font.pFont;			
		}
	}	
	
	return pFont;
}


RapiFont * UITextureManager::GetDefaultFont()
{ 
	return m_pDefaultFont; 
}