//////////////////////////////////////////////////////////////////////////////
//
// File     :  siege_console.h
// Author(s):  Casey Muratori, Scott Bilas (recoded)
//
// Summary  :  A text i/o console for use with Siege.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SIEGE_CONSOLE_H
#define __SIEGE_CONSOLE_H

//////////////////////////////////////////////////////////////////////////////

#include <string>
#include "Circular_Buffer.h"
#include "FuBiDefs.h"
#include "Vector_3.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class Rapi;
class RapiFont;

namespace siege  {  // begin of namespace siege

//////////////////////////////////////////////////////////////////////////////
// class Console declaration

class Console
{
public:
	SET_NO_INHERITED( Console );

	// ctor/dtor
	Console( RapiFont& font );
   ~Console( void );

	// back buffer
	UINT				Print( const gpstring& line );						// store a line of text into the backbuffer
	UINT				PrintAppend( const gpstring& line, UINT index );
	UINT				GetBackBufferLineCount( void )				{  return ( m_BackBuffer.size() );  }
	void				ScrollUp( UINT lines = 1 );								// scroll the backbuffer up
	void				ScrollDown( UINT lines = 1 );							// scroll the backbuffer down
	void				Clear(void);											// clear the backbuffer

	// input line
	const gpstring&		GetInputLine( void ) const					{  return ( m_InputLine );  }
	void				SetInputLine( const gpstring& line )		{  m_InputLine = line;  }
	gpstring&			GetLine( UINT line );

	// options
	bool				IsVisible( void ) const						{  return( m_Visible );  }
	void				SetVisibility( bool set = true )			{  m_Visible = set;  }
	void				SetInputLineVisibility( bool set = true )	{  m_InputLineVisible = set;  }

	// geometry
	FUBI_EXPORT void	SetSize( float width, float height )		{  m_Width = width; m_Height = height;  }
	FUBI_EXPORT void	SetPosition( float x, float y )				{  m_Position = vector_3( x, y, 0 );  }

	// rendering
	void				Render( Rapi& renderer ) const;							// render the console

	// registration
	static void			SetPrimaryConsole( Console* console )		{  ms_PrimaryConsole = console;  }
	static void			ClearPrimaryConsole( void )					{  SetPrimaryConsole( NULL );  }
	static Console*		GetPrimaryConsole( void )					{  return ( ms_PrimaryConsole );  }
	static Console&		GetValidPrimaryConsole( void )				{  gpassert( ms_PrimaryConsole != NULL );  return ( *ms_PrimaryConsole );  }

	FUBI_VARIABLE( bool,			m_, BottomUp,				"True if the console fills bottom up." );
	FUBI_VARIABLE( bool,			m_, RenderOutline,			"Whether or not to render the frame outline." );
	FUBI_VARIABLE( bool,			m_, RenderFilled,			"Whether or not to render the frame as filled." );
	FUBI_VARIABLE( DWORD,			m_, OutlineColor,			"The MakeColor() color of the outline." );
	FUBI_VARIABLE( DWORD,			m_, FillColor,				"The MakeColor() color of the filled background." );

private:

	// reference to the RapiFont
	RapiFont&						m_Font;

	// options
	bool							m_Visible;
	bool							m_InputLineVisible;

	// text storage
	gpstring						m_Prompt;
	gpstring						m_InputLine;
	circular_buffer <gpstring>		m_BackBuffer;

	// positional information
	int								m_ScrollPosition;
	float							m_Width;
	float							m_Height;
	vector_3						m_Position;

	// singleton
	static Console*					ms_PrimaryConsole;

	SET_NO_COPYING( Console );
};

#define gOutputConsole siege::Console::GetValidPrimaryConsole()

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace siege

#endif  // __SIEGE_CONSOLE_H

//////////////////////////////////////////////////////////////////////////////
