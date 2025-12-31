/**************************************************************************

Filename		: FontExporter.cpp

Description		: Export True-Type Fonts into bitmaps

Creation Date	: 10/20/99

Author			: Chad Queen

**************************************************************************/


// Include Files
#include "IgnoredWarnings.h"
#include "FontExporter.h"
#include <assert.h>
#include "math.h"


// Character size consts
const int char_start = 33;
const int char_end	 = 126;


/**************************************************************************

Function		: FontExporter()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
FontExporter::FontExporter()
	: m_size			( 10 )
	, m_bAllowDraw		( false )
	, m_x				( 0 )
	, m_y				( 0 )
	, m_screen_width	( 256 )
	, m_screen_height	( 256 )
	, m_spacing			( 0 )
	, m_bSmooth			( false )
{	
	strcpy( m_characters, "" );	
	m_hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
}


/**************************************************************************

Function		: ~FontExporter()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
FontExporter::~FontExporter()
{
	ReleaseDC( m_hWnd, m_hDC );
}


/**************************************************************************

Function		: GetAvailableFonts()

Purpose			: Retrieves a list of available fonts for use

Returns			: std::vector< std::string > 

**************************************************************************/
std::vector< std::string > FontExporter::GetAvailableFonts()
{
	m_hDC = GetDC( m_hWnd );
	SetFontColor( 0x00FFFFFF );
	SetBkColor( m_hDC, 0x00000000 );
	m_background_color = 0x00000000;
	
	std::vector< std::string > font_list;
	char szFontName[1024] = "";
	TCHAR szFaceName[32] = "";
	LOGFONT logfont;
	logfont.lfCharSet			= DEFAULT_CHARSET;	
	lstrcpy( logfont.lfFaceName, szFaceName );
	logfont.lfPitchAndFamily	= 0;
	while (( EnumFontFamiliesEx( m_hDC, &logfont, (FONTENUMPROC)EnumFontExProc, (LPARAM)this, 0 )) != 0 ) {
	}
	
	std::vector< Font_Information >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		font_list.push_back( (*i).fullname );
	}
	return font_list;
}


/**************************************************************************

Function		: AddFont()

Purpose			: Add a font to our font structure list

Returns			: bool

**************************************************************************/
bool FontExporter::AddFont( Font_Information fi )
{
	std::vector< Font_Information >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( fi.fullname == (*i).fullname ) {
			return false;
		}
	}
	
	m_fonts.push_back( fi );

	return true;
}


/**************************************************************************

Function		: ExportFont()

Purpose			: Export a font to an image file

Returns			: void

**************************************************************************/
void FontExporter::ExportFont( const char *szTypeface, bool bBold, bool bItalic, bool bUnderline )
{	
	SetMapMode( m_hDC, MM_TEXT );

	std::vector< Font_Information >::iterator i;
	for ( i = m_fonts.begin(); i != m_fonts.end(); ++i ) {
		if ( !strcmp( szTypeface, (*i).fullname.c_str() ) ) {
			break;
		}
	}

	int weight = 0;
	if ( bBold == true ) {
		weight = FW_BOLD;
	}

	HFONT hFont = CreateFont(	GetFontSize(), 
								0, 
								0, 
								0, 								
								weight, 
								bItalic, 
								bUnderline, 
								false, 
								ANSI_CHARSET, 
								OUT_TT_ONLY_PRECIS, 
								CLIP_DEFAULT_PRECIS,
								ANTIALIASED_QUALITY, 
								(*i).pitch_and_family, 
								(*i).typefacename.c_str() );

	
	/////////
	/*
	static int  iEnd[] = { PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE, PS_ENDCAP_FLAT };
	static int  iJoin[]= { PS_JOIN_ROUND,   PS_JOIN_BEVEL,    PS_JOIN_MITER };     
	LOGBRUSH	lb;
	lb.lbStyle = BS_SOLID;
	lb.lbColor = RGB (128, 128, 128);
	lb.lbHatch = 0;
          
	SelectObject( m_hDC, ExtCreatePen (PS_SOLID | PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_BEVEL, 1, &lb, 0, NULL)) ;
	*/
	////////      


	SelectObject( m_hDC, hFont );	

	GetCharWidth32( m_hDC, m_ascii_low, m_ascii_high, (int *)m_char_width );

	/*
	GetCharABCWidths( m_hDC, m_ascii_low, m_ascii_high, m_abc_width );
	for ( int k = 0; k < (m_ascii_high-m_ascii_low); ++k ) {
		m_char_width[k] = m_abc_width[k].abcB + m_abc_width[k].abcA + m_abc_width[k].abcC;
	}
	*/	

	int index = 0;
	for ( int j = m_ascii_low; j <= m_ascii_high; ++j ) {
		m_characters[index++] = (char)j;		
	}
	m_characters[index] = '\0';
	m_bAllowDraw = true;
		
	RECT rect;
	GetClientRect( m_hWnd, &rect );
	InvalidateRect( m_hWnd, &rect, true );
}


/**************************************************************************

Function		: Draw()

Purpose			: This call will render our current text to the screen/DC

Returns			: void

**************************************************************************/
void FontExporter::Draw()
{
	if ( (GetFontSize() >= 7) || (GetFontSize() <= 14 ))
	{
		m_spacing += 2;
	}

	// Fill in the background color
	RECT rect;
	GetClientRect( m_hWnd, &rect );	
	FillRect( m_hDC, &rect, m_hBrush );
	
	// Draw the text
	m_x = 0;
	m_y = 0;
	int total_width = 0;
	TEXTMETRIC tm;
	GetTextMetrics( m_hDC, &tm );
	int height = tm.tmHeight;
	DWORD dwColor = 0x00FF00FF;

	//////
	//BeginPath (m_hDC);    
	//////

	if ( m_bAllowDraw == true ) {		
		for ( int i = 0; i < strlen( m_characters ); ++i ) {
			total_width += m_char_width[i] + m_spacing;					
			
			if ( total_width > m_screen_width-2 ) {				
				TEXTMETRIC tm;
				GetTextMetrics( m_hDC, &tm );
				m_y += tm.tmHeight+1;				
				total_width = m_char_width[i];				
				
			}			
			
			char szText[1] = "";
			sprintf( szText, "%c", m_characters[i] );
			TextOut( m_hDC, m_x+total_width-m_char_width[i], m_y, szText, 1 );					
		}

		m_x = 0;
		m_y = 0;
		total_width = 0;

		for ( i = 0; i < strlen( m_characters ); ++i ) {
			total_width += m_char_width[i] + m_spacing;			
		
			if ( total_width > m_screen_width-2 ) {
				SetPixel( m_hDC, total_width, m_y, dwColor );
				TEXTMETRIC tm;
				GetTextMetrics( m_hDC, &tm );
				m_y += tm.tmHeight+1;								
				total_width = m_char_width[i];
				SetPixel( m_hDC, total_width, m_y, dwColor );
			}						
			SetPixel( m_hDC, total_width, m_y, dwColor );			
		}
		SetPixel( m_hDC, total_width, m_y, dwColor );		
	}

	if ( (GetFontSize() >= 7) || (GetFontSize() <= 14 ))
	{
		m_spacing -= 2;
	}	

}


// The purpose of this function is to iterate through all of the pixels
// in the bitmap and then, dependent on how ma
void FontExporter::SmoothFont( unsigned char * buffer )
{
	// First, let's pre-calculate the split byte colors for later use
	BYTE alpha	= (BYTE)( (m_text_color	& 0xFF000000) >> 24 );
	BYTE blue	= (BYTE)( (m_text_color	& 0x00FF0000) >> 16 );
	BYTE green	= (BYTE)( (m_text_color	& 0x0000FF00) >> 8 );
	BYTE red	= (BYTE)( m_text_color	& 0x000000FF );	

	int size = m_screen_width*m_screen_height*4;
	int pixel_index = 0;
	for ( int i = 0; i < size; i += 4 ) 
	{
		// Retrieve current pixel color
		BYTE alpha	= 0;
		BYTE blue	= (BYTE)( buffer[i]	);
		BYTE green	= (BYTE)( buffer[i+1]	);
		BYTE red	= (BYTE)( buffer[i+2]	);
		DWORD color = (alpha << 24) | (blue << 16) | (green << 8) | red;		

		// Make sure this is the SOLID color of the text we need to use		

		if ( color != 0x00000000 )
		{
			int f = 0;
		}

		if ( color == m_text_color )
		{
			// Now, let's find the row and column location of the pixel
			int row = floor( (double)pixel_index / (double)m_screen_width );
			int col = pixel_index - ( row * m_screen_width );

			// Find out which sides also have pixels of the text color
			bool bNorth = false;
			bool bSouth = false;
			bool bEast	= false;
			bool bWest	= false;

			int north_index = pixel_index - m_screen_width;
			int south_index = pixel_index + m_screen_width;
			int west_index	= pixel_index - 1;
			int east_index	= pixel_index + 1;
			
			int sides = 0;

			// Test the sides
			if ( north_index >= 0 )
			{
				int color_index = north_index*4;
				BYTE alpha	= 0;
				BYTE blue	= (BYTE)( buffer[color_index] );
				BYTE green	= (BYTE)( buffer[color_index+1]	);
				BYTE red	= (BYTE)( buffer[color_index+2]	);
				DWORD pixel_color = (alpha << 24) | (blue << 16) | (green << 8) | red;
				if ( pixel_color == m_background_color )
				{
					bNorth = true;					
				}				
				else 
				{
					if ( pixel_color == m_text_color )
					{
						sides++;
					}
				}
			}

			if ( south_index < (size/4) )
			{
				int color_index = south_index*4;
				BYTE alpha	= 0;
				BYTE blue	= (BYTE)( buffer[color_index] );
				BYTE green	= (BYTE)( buffer[color_index+1]	);
				BYTE red	= (BYTE)( buffer[color_index+2]	);
				DWORD pixel_color = (alpha << 24) | (blue << 16) | (green << 8) | red;
				if ( pixel_color == m_background_color )
				{
					bSouth = true;					
				}				
				else 
				{
					if ( pixel_color == m_text_color )
					{
						sides++;
					}
				}
			}

			int test_mod1 = west_index % m_screen_width;
			if (( west_index >= 0 ) && ( (west_index % m_screen_width ) != (m_screen_width-1) ))
			{
				int color_index = west_index*4;
				BYTE alpha	= 0;
				BYTE blue	= (BYTE)( buffer[color_index] );
				BYTE green	= (BYTE)( buffer[color_index+1]	);
				BYTE red	= (BYTE)( buffer[color_index+2]	);
				DWORD pixel_color = (alpha << 24) | (blue << 16) | (green << 8) | red;
				if ( pixel_color == m_background_color )
				{
					bWest = true;				
				}				
				else 
				{
					if ( pixel_color == m_text_color )
					{
						sides++;
					}
				}
			}

			int test_mod2 = east_index % m_screen_width;
			if (( east_index < (size/4)  ) && ( ( east_index % m_screen_width ) != 0 ))
			{
				int color_index = east_index*4;
				BYTE alpha	= 0;
				BYTE blue	= (BYTE)( buffer[color_index] );
				BYTE green	= (BYTE)( buffer[color_index+1]	);
				BYTE red	= (BYTE)( buffer[color_index+2]	);
				DWORD pixel_color = (alpha << 24) | (blue << 16) | (green << 8) | red;
				if ( pixel_color == m_background_color )
				{
					bEast = true;				
				}				
				else 
				{
					if ( pixel_color == m_text_color )
					{
						sides++;
					}
				}
			}	
			
			/*
			float factor = (float)sides / (float)1.5;
			if ( sides == 0 )
			{	
				factor = (float)1 / (float)1.5;
			}
			buffer[i]	= red	* factor;
			buffer[i+1]	= green * factor;
			buffer[i+2]	= blue	* factor;
			buffer[i+3]	= alpha * factor;			
			*/

			
			float factor = (float)sides / (float)8;
			if ( factor != 1.0f )
			{
				if ( bNorth )
				{
					if ( row % (m_size+2) != 0 )
					{
						int color_index = north_index*4;				
						buffer[color_index]		= red	* factor;
						buffer[color_index+1]	= green * factor;
						buffer[color_index+2]	= blue	* factor;
						buffer[color_index+3]	= alpha * factor;
					}
					else 
					{
						int fuck = 0;
					}
				}
				if ( bSouth )
				{
					int color_index = south_index*4;				
					buffer[color_index]		= red	* factor;
					buffer[color_index+1]	= green * factor;
					buffer[color_index+2]	= blue	* factor;
					buffer[color_index+3]	= alpha * factor;
				}

				/*
				if ( bWest )
				{
					int color_index = west_index*4;				
					buffer[color_index]		= red	* factor;
					buffer[color_index+1]	= green * factor;
					buffer[color_index+2]	= blue	* factor;
					buffer[color_index+3]	= alpha * factor;
				}
				*/

				/*
				if ( bEast )
				{
					int color_index = east_index*4;				
					buffer[color_index]		= red	* factor;
					buffer[color_index+1]	= green * factor;
					buffer[color_index+2]	= blue	* factor;
					buffer[color_index+3]	= alpha * factor;
				}
				*/
			}
		}

		++pixel_index;
	}
}


/**************************************************************************

Function		: SaveFont()

Purpose			: This call will save our current DC to a file

Returns			: bool

**************************************************************************/
bool FontExporter::SaveFont( char *szFilename )
{	

	HANDLE hFile = CreateFile(	szFilename, 
								GENERIC_WRITE, 
								0,
								0,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								0 );
		
	/*
	// Parse through PSD header information
	DWORD bytesWritten = 0;
	char signature[5] = "8BPS";
	if ( WriteFile( hFile, signature, 4, &bytesWritten, 0 ) == 0 ) {
		MessageBox( m_hWnd, "WriteFile Failed.", "Error", MB_OK );
	}

	short version	= 1;
	LittleEndianToBigEndian16( &version );
	WriteFile( hFile, &version, sizeof( short ),  &bytesWritten, 0 );

	int reserved	= 0;	
	WriteFile( hFile, &reserved, 6, &bytesWritten, 0 );

	short channels	= 3;
	LittleEndianToBigEndian16( &channels );
	WriteFile( hFile, &channels, sizeof( short ), &bytesWritten, 0 );

	long rows		= 256;
	long columns	= 256;
	LittleEndianToBigEndian32( &rows	);
	LittleEndianToBigEndian32( &columns );
	WriteFile( hFile, &rows, sizeof( unsigned int ), &bytesWritten, 0 );
	WriteFile( hFile, &rows, sizeof( unsigned int ), &bytesWritten, 0 );

	short depth = 8;
	LittleEndianToBigEndian16( &depth );
	WriteFile( hFile, &depth, sizeof( short ), &bytesWritten, 0 );
	
	short mode = 0;
	LittleEndianToBigEndian16( &mode );
	WriteFile( hFile, &mode, sizeof( short ), &bytesWritten, 0 );


	// Parse through Color Mode Data
	DWORD length = 0;
	WriteFile( hFile, &length, sizeof( DWORD ), &bytesWritten, 0 );

	// Parse through Image Resources
	WriteFile( hFile, &length, sizeof( DWORD ), &bytesWritten, 0 );

	// Parse through layer and mask information
	WriteFile( hFile, &length, sizeof( DWORD ), &bytesWritten, 0 );

	// Write out the actual image bits
	short compression = 0;		// Raw image data ( non-RLE compressed )
	WriteFile( hFile, &compression, sizeof( DWORD ), &bytesWritten, 0 );

	*/
	
	DWORD bytesWritten = 0;

	int size = m_screen_width*m_screen_height*4;
	unsigned char *buffer	= (unsigned char*)malloc( size );
	unsigned char *buffer2	= (unsigned char*)malloc( m_screen_width*m_screen_height*3 );
	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biSize			= sizeof( BITMAPINFOHEADER );
	bmpInfo.bmiHeader.biWidth			= m_screen_width;
	bmpInfo.bmiHeader.biHeight			= m_screen_height;
	bmpInfo.bmiHeader.biPlanes			= 1;
	bmpInfo.bmiHeader.biCompression		= BI_RGB;
	bmpInfo.bmiHeader.biSizeImage		= size;
	bmpInfo.bmiHeader.biXPelsPerMeter	= 0;
	bmpInfo.bmiHeader.biYPelsPerMeter	= 0;
	bmpInfo.bmiHeader.biClrUsed			= 0;
	bmpInfo.bmiHeader.biBitCount		= 32;
	bmpInfo.bmiHeader.biClrImportant	= 0;
	HBITMAP hBitmap = CreateDIBSection( m_hDC, &bmpInfo, DIB_RGB_COLORS, (void **)&buffer, 0, 0 );	
	
	HDC bDC = CreateCompatibleDC( m_hDC );
	SelectObject( bDC, hBitmap );
	BitBlt( bDC, 0, 0, m_screen_width-0, m_screen_height-0, m_hDC, 0, 0, SRCCOPY );

	int scanlines = GetDIBits( bDC, hBitmap, 0, 0, &buffer, &bmpInfo, DIB_RGB_COLORS );
	bmpInfo.bmiColors[0].rgbBlue		= 0;
	bmpInfo.bmiColors[0].rgbGreen		= 0;
	bmpInfo.bmiColors[0].rgbRed			= 0;
	bmpInfo.bmiColors[0].rgbReserved	= 0;

	if ( m_bSmooth )
	{
		SmoothFont( buffer );
	}

	int j = 0;
	for ( int i = 0; i < size; i += 4 ) {
		unsigned char temp = 0;		
		buffer2[j+2] = buffer[i+2];
		buffer2[j+1] = buffer[i+1];
		buffer2[j] = buffer[i];		
		j += 3;
	}	

	bmpInfo.bmiHeader.biBitCount = 24;
	bmpInfo.bmiHeader.biSizeImage = m_screen_width*m_screen_height*3;

	BITMAPFILEHEADER bfh;
	bfh.bfType = 'MB';
	bfh.bfSize =	sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)*2 +
					bmpInfo.bmiHeader.biSizeImage;

	// Note the +1 wackiness I need for the 32 to 24 bit conversion to work right...
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 1;
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;

	WriteFile( hFile, &bfh, sizeof( BITMAPFILEHEADER ), &bytesWritten, 0 );
	WriteFile( hFile, &bmpInfo, sizeof( BITMAPINFO ), &bytesWritten, 0 );	
	// WriteFile( hFile, buffer, size, &bytesWritten, 0 );
	WriteFile( hFile, buffer2, bmpInfo.bmiHeader.biSizeImage, &bytesWritten, 0 );
	
	CloseHandle( hFile );


	// Make a text file that stores out all the font values
	m_x = 0;
	m_y = 0;
	int total_width = 0;
	TEXTMETRIC tm;
	GetTextMetrics( m_hDC, &tm );
	int height = tm.tmHeight;

	// HANDLE hTextFile = CreateFile( "characters.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );

	FILE *f = fopen( "characters.txt", "wt" );
	if ( f ) {
		for ( int i = 0; i < strlen( m_characters ); ++i ) {
			total_width += m_char_width[i] + 2 + m_spacing;					
			
			if ( total_width > m_screen_width-2 ) {				
				TEXTMETRIC tm;
				GetTextMetrics( m_hDC, &tm );
				m_y += tm.tmHeight+1;				
				total_width = m_char_width[i];								

				DWORD textWritten = 0;
				// WriteFile( hTextFile, "\n", 1, &textWritten, 0 );
				fprintf( f, "\n" );
			}			
			
			char szText[1] = "";
			sprintf( szText, "%c", m_characters[i] );
			DWORD textWritten = 0;
			// WriteFile( hTextFile, szText, strlen( szText ), &textWritten, 0 );			
			if ( (m_characters[i] != '\n') && (m_characters[i] != '\r') ) {
				fprintf( f, "%c", m_characters[i] );
			}
		}
	}

	fclose( f );
	// CloseHandle( hTextFile );

	return false;
}


/**************************************************************************

Function		: SetASCIIRange()

Purpose			: Sets the character range of which to draw onscreen

Returns			: void

**************************************************************************/
void FontExporter::SetASCIIRange( unsigned int low, unsigned int high )
{
	m_ascii_low		= low;
	m_ascii_high	= high;
}



/**************************************************************************

Function		: SetBackgroundColor()

Purpose			: Sets the character range of which to draw onscreen

Returns			: void

**************************************************************************/
void FontExporter::SetBackgroundColor( unsigned int color )	
{ 
	m_background_color = color;
	SetBkColor( m_hDC, (COLORREF)color );	
	m_hBrush = CreateSolidBrush( color );	
}



/**************************************************************************

Function		: SetFontColor()

Purpose			: Sets the character range of which to draw onscreen

Returns			: void

**************************************************************************/
void FontExporter::SetFontColor( unsigned int color ) 
{
	m_text_color = color;
	SetTextColor( m_hDC, (COLORREF)color );	
}



/**************************************************************************

Function		: EnumFontExProc()

Purpose			: Callback for enumerating all available fonts

Returns			: int CALLBACK

**************************************************************************/
int CALLBACK EnumFontExProc( ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType,             
							 LPARAM lParam )
{
	char szFontFullname[1024]	= "";
	sprintf( szFontFullname, "%s %s %s", lpelfe->elfFullName, lpelfe->elfStyle, lpelfe->elfScript );
	FontExporter *pFE = (FontExporter *)lParam;
	char szTypeface[1024]		= "";
	sprintf( szTypeface, "%s", lpelfe->elfFullName );
	
	Font_Information fi;
	fi.fullname			= szFontFullname;
	fi.typefacename		= szTypeface;
	fi.height			= lpelfe->elfLogFont.lfHeight;
	fi.width			= lpelfe->elfLogFont.lfWidth;
	fi.orientation		= lpelfe->elfLogFont.lfOrientation;
	fi.escapement		= lpelfe->elfLogFont.lfEscapement;
	fi.quality			= lpelfe->elfLogFont.lfQuality;
	fi.out_precision	= lpelfe->elfLogFont.lfOutPrecision;
	fi.clip_precision	= lpelfe->elfLogFont.lfClipPrecision;
	fi.pitch_and_family	= lpelfe->elfLogFont.lfPitchAndFamily;

	if ( pFE->AddFont( fi ) == true ) {
		return 1;
	}
		
	return 0;
}



/**************************************************************************

Function		: LittleEndianToBigEndian16()

Purpose			: Converts a short value big endian value to a short
				  little endian value...Damn Apple bastards...

Returns			: void

**************************************************************************/
void LittleEndianToBigEndian16( short *sValue )
{
	BYTE one	= *sValue;
	BYTE two	= *sValue>>8;

	*sValue		= two | (one<<8);		
}


/**************************************************************************

Function		: LittleEndianToBigEndian32()

Purpose			: Converts a long value big endian value to a long
				  little endian value...Damn Apple bastards...

Returns			: void

**************************************************************************/
void LittleEndianToBigEndian32( long *sValue )
{
	BYTE one	= *sValue;
	BYTE two	= *sValue>>8;
	BYTE three	= *sValue>>16;
	BYTE four	= *sValue>>24;

	*sValue		= four | (three<<8) | (two<<16) | (one<<24);
}
 