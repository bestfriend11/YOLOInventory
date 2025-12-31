//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_console.cpp
// Author(s):  Casey Muratori, Scott Bilas (recoded)
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Siege.h"
#include "siege_console.h"

#include "RapiOwner.h"

namespace siege  {  // begin of namespace siege

//////////////////////////////////////////////////////////////////////////////
// class Console implementation

Console* Console::ms_PrimaryConsole = NULL;

Console :: Console( RapiFont& font )
	: m_Font( font ),
	  m_BackBuffer( 500 ),
	  m_Prompt( "> " ),
	  m_Position( 0, 0, 0 )
{
	m_Visible          = true;
	m_InputLineVisible = true;
	m_BottomUp         = true;
	m_RenderOutline    = true;
	m_RenderFilled     = false;
	m_OutlineColor     = 0xFFFFFFFF;
	m_FillColor        = 0xFF000000;
	m_ScrollPosition   = 0;
	m_Width            = 0;
	m_Height           = 0;

	// auto-register
	if ( ms_PrimaryConsole == NULL )
	{
		SetPrimaryConsole( this );
	}
}

Console :: ~Console( void )
{
	// this space intentionally left blank...
}

UINT Console :: Print( const gpstring& line )
{
	UINT index = line.find_first_of( '\n' );
	if( index == gpstring::npos )
	{
		m_BackBuffer.push_back( line );
	}
	else
	{
		// split the line in two $$ this is stupidly inefficient
		gpstring firstHalf( line, 0, index );
		gpstring secondHalf( line, index + 1, line.size() - (index + 1) );
		Print( firstHalf );
		if ( !secondHalf.empty() )
		{
			Print( secondHalf );
		}
	}
	m_ScrollPosition = 0;
	return ( m_BackBuffer.size() - 1 );
}

UINT Console :: PrintAppend( const gpstring& line, UINT index )
{
	if ( m_BackBuffer.empty() )
	{
		return ( Print( line ) );
	}
	else
	{
		m_BackBuffer[ index ] += line;
		return ( m_BackBuffer.size() - 1 );
	}
}

gpstring& Console :: GetLine( UINT line )
{
	if ( line < m_BackBuffer.size() )
	{
		return ( m_BackBuffer[ line ] );
	}
	else
	{
		gpassertm( 0, "You did not get the line you wanted." );
		return ( m_BackBuffer[ 0 ] );
	}
}

void Console :: ScrollUp( UINT lines )
{
	m_ScrollPosition += lines;
	int size = m_BackBuffer.size();
	if ( m_ScrollPosition >= size - 2 )
	{
		m_ScrollPosition = size - 3;
	}

	if ( m_ScrollPosition < 0 )
	{
		m_ScrollPosition = 0;
	}
}

void Console :: ScrollDown( UINT lines )
{
	m_ScrollPosition -= lines;
	if ( m_ScrollPosition < 0 )
	{
		m_ScrollPosition = 0;
	}
}

void Console :: Clear( void )
{
	m_BackBuffer.clear();
	m_ScrollPosition = 0;
}

void Console :: Render( Rapi& renderer ) const
{
	// early bailout if not visible
	if ( !m_Visible )
	{
		return;
	}

	// get some geometry helpers (all in screen space)
	UINT screenWidth = renderer.GetWidth(), screenHeight = renderer.GetHeight();
	float l = m_Position.x * screenWidth;
	float t = m_Position.y * screenHeight;
	float r = l + (m_Width * screenWidth);
	float b = t + (m_Height * screenHeight);

// Render the frame.

	const DWORD OUTLINE_COLOR = m_OutlineColor;
	const DWORD FILL_COLOR    = m_FillColor;

	// set for color
	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	bool bPrevFogState = renderer.SetFogActivatedState( false );

	// init coords
	tVertex frame[5];
	ZeroObject( frame );
	frame[ 0 ].rhw = 1;
	frame[ 1 ].rhw = 1;
	frame[ 2 ].rhw = 1;
	frame[ 3 ].rhw = 1;
	frame[ 4 ].rhw = 1;

	// render fill
	if ( m_RenderFilled )
	{
		// build coords
		frame[ 0 ].x = l;  frame[ 0 ].y = t;  frame[ 0 ].color = FILL_COLOR / 2;
		frame[ 1 ].x = l;  frame[ 1 ].y = b;  frame[ 1 ].color = FILL_COLOR;
		frame[ 2 ].x = r;  frame[ 2 ].y = t;  frame[ 2 ].color = FILL_COLOR / 2;
		frame[ 3 ].x = r;  frame[ 3 ].y = b;  frame[ 3 ].color = FILL_COLOR;

		// render
		renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, frame, 4, TVERTEX, NULL, 0 );
	}

	// render outline
	if ( m_RenderOutline )
	{
		// build coords
		frame[ 0 ].x = l + 0.5f;  frame[ 0 ].y = t + 0.5f;  frame[ 0 ].color = OUTLINE_COLOR;
		frame[ 1 ].x = l + 0.5f;  frame[ 1 ].y = b - 0.5f;  frame[ 1 ].color = OUTLINE_COLOR;
		frame[ 2 ].x = r - 0.5f;  frame[ 2 ].y = b - 0.5f;  frame[ 2 ].color = OUTLINE_COLOR;
		frame[ 3 ].x = r - 0.5f;  frame[ 3 ].y = t + 0.5f;  frame[ 3 ].color = OUTLINE_COLOR;
		frame[ 4 ] = frame[ 0 ];

		// render
		renderer.DrawPrimitive( D3DPT_LINESTRIP, frame, 5, TVERTEX, NULL, 0 );
	}

// Render the text.

	const float BORDER = 3.0f;

	// add some borders
	l += BORDER;  t += BORDER;  r -= BORDER;  b -= BORDER;

	// get some more geometry helpers
	int textWidth, textHeight;
	m_Font.CalculateStringSize( m_Prompt, textWidth, textHeight );
	float iterY = b - textHeight;

	// render the input line
	if ( m_InputLineVisible )
	{
		// render the prompt
		m_Font.Print( FTOL( l ), FTOL( iterY ), m_Prompt );

		UINT pos = 0;

		if ( !m_InputLine.empty() && (*m_InputLine.rbegin() == '.') )
		{
			// $$ this is *really* inefficient...cmu code...good thing it doesn't matter

			for( UINT len = m_InputLine.length() ; pos < len ; ++pos )
			{
				int width, height;
				m_Font.CalculateStringSize( m_InputLine.c_str() + pos, width, height );
				if ( (width + textWidth) < (r - l) )
				{
					break;
				}
			}
		}

		// render the line
		m_Font.Print( FTOL( l + textWidth ), FTOL( iterY ), m_InputLine.c_str() + pos );

		// work backwards
		iterY -= textHeight;
	}

	// render the back buffer
	if ( m_BottomUp )
	{
		for (  UINT line = m_BackBuffer.size() - m_ScrollPosition
			 ; (line > 0) && ((iterY - textHeight) > t)
			 ; --line, iterY -= textHeight )
		{
			const gpstring& currentLine = m_BackBuffer[ line - 1 ];
			m_Font.CalculateStringSize( currentLine, textWidth, textHeight );
			m_Font.Print( FTOL( l ), FTOL( iterY ), currentLine );
		}
	}
	else
	{
		iterY = t;

		if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
		{
			// skip a few lines
			iterY += textHeight * 5;
		}

		for (  UINT line = m_ScrollPosition
			 ; (line < m_BackBuffer.size()) && (iterY < (b - textHeight))
			 ; ++line, iterY += textHeight )
		{
			const gpstring& currentLine = m_BackBuffer[ line ];
			m_Font.CalculateStringSize( currentLine, textWidth, textHeight );
			m_Font.Print( FTOL( l ), FTOL( iterY ), currentLine );
		}
	}

	renderer.SetFogActivatedState( bPrevFogState );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace siege

//////////////////////////////////////////////////////////////////////////////
