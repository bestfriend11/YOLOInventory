/**************************************************************************

Filename		: FontExporter.h

Description		: Export True-Type Fonts into bitmaps

Creation Date	: 10/20/99

**************************************************************************/


#pragma once
#ifndef _FONTEXPORTER_H_
#define _FONTEXPORTER_H_

// Include Files
#include <wtypes.h>
#include <wingdi.h>
#include <string>
#include <vector>


// Structures
struct Font_Information
{
	std::string fullname;
	std::string typefacename;
	int			height;
	int			width;
	int			escapement;
	int			orientation;
	DWORD		out_precision;
	DWORD		clip_precision;
	DWORD		quality;
	DWORD		pitch_and_family;	
};


// Enumerated Function Prototypes
int CALLBACK EnumFontExProc(	ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType,             
								LPARAM lParam );

void LittleEndianToBigEndian16( short *sValue );
void LittleEndianToBigEndian32( long *sValue );


class FontExporter
{
public:

	// Constructor
	FontExporter();

	// Destructor
	~FontExporter();

	// Retrieve a text list of available fonts
	std::vector< std::string > GetAvailableFonts();	

	// Export a font into an image
	void ExportFont( const char *szTypeface, bool bBold, bool bItalic, bool bUnderline );
	
	// Add a font to our font list
	bool AddFont( Font_Information fi );

	// Set/Get font size
	void	SetFontSize( int size ) { m_size = size; }
	int		GetFontSize()			{ return m_size; }

	// Set/Get Window Information
	void		SetHWnd( HWND hWnd )	{ m_hWnd = hWnd; }
	HWND		GetHWnd()				{ return m_hWnd; }
	void		SetHInstance( HINSTANCE hInstance ) { m_hInstance = hInstance;	}
	HINSTANCE	GetHInstance()						{ return m_hInstance;		}
	HDC			GetFontDC()				{ return m_hDC;	 }

	// Draw
	void		Draw();

	void		SmoothFont( unsigned char * buffer );

	// Save Functionality
	bool		SaveFont( char *szFilename );

	// Sets the ascii range 
	void SetASCIIRange( unsigned int low, unsigned int high );

	// Set the screen width and screen height
	void	SetScreenWidth( int width )		{ m_screen_width = width;	}
	int		GetScreenWidth()				{ return m_screen_width;	}
	void	SetScreenHeight( int height )	{ m_screen_height = height;	}
	int		GetScreenHeight()				{ return m_screen_height;	}

	// Setup font exporter colors	
	void			SetBackgroundColor( unsigned int color );
	unsigned int	GetBackgroundColor()						{ return m_background_color;	}
	void			SetFontColor( unsigned int color );
	unsigned int	GetFontColor()								{ return m_text_color;			}

	void	SetSpacing( int spacing ) { m_spacing = (unsigned int)spacing; }

	void	SetSmooth( bool bSmooth )	{ m_bSmooth = bSmooth; }
	bool	GetSmooth()					{ return m_bSmooth; }


private:

	// Private Member Variables
	HWND		m_hWnd;
	HINSTANCE	m_hInstance;
	HDC			m_hDC;
	int			m_size;

	// Characters
	char			m_characters[512];
	unsigned int	m_char_width[512];
	ABC				m_abc_width[512];

	// Do we have a valid font to draw with?
	bool		m_bAllowDraw;

	// List of our available fonts and typeface names
	std::vector< Font_Information > m_fonts;

	// Current Drawing coordinates
	int			m_x;
	int			m_y;

	// ASCII range values to draw
	unsigned int m_ascii_low;
	unsigned int m_ascii_high;

	int			m_screen_width;
	int			m_screen_height;

	unsigned int m_background_color;
	unsigned int m_text_color;

	HBRUSH		m_hBrush;

	unsigned int m_spacing;

	bool		m_bSmooth;

};


#endif