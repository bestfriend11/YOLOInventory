//////////////////////////////////////////////////////////////////////////////
//
// File     :	ui_scroll_window.h
// Author(s):	Chad Queen
//
// Notes	:   This base class can be inherited by a UI control class that 
//				wants to utilize a slider control.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once 
#ifndef __UI_SCROLL_WINDOW_H
#define __UI_SCROLL_WINDOW_H



class UIScrollWindow
{
public:
	UIScrollWindow() {};

	virtual ~UIScrollWindow() {};


	// Horizontal scrolling is done on a pixel level

		// Get the size ( width ) of available column space in pixels ( usually the width of the control minus
		// scroll bar space )
		virtual int		GetColumnSize()					{ return 0; }

		// Get the total amount of pixels the columns take up
		virtual int		GetTotalColumnSize()			{ return 0; }

		// Set the horizontal position ( in pixels )
		virtual void	SetHorizontalPos( int pos )		{ UNREFERENCED_PARAMETER( pos ); }


	// Vertical scrolling is done on an "element" level.  Usually consists of a line of text
	
		// Total number of elements 
		virtual unsigned int		GetNumElements()				{ return 0; }
		
		// Maximum number of elements that can fit on the window at the same time
		virtual unsigned int		GetMaxActiveElements()			{ return 0; }

		// Get/Set the "Lead" element index; the element that appears "first" ( the top ) in the window 
		virtual int		GetLeadElement()				{ return 0; }
		virtual void	SetLeadElement( int lead )		{ UNREFERENCED_PARAMETER( lead ); }

		virtual int		GetElementHeight()				{ return 0; }

		virtual bool	GetScrollDown()					{ return false; }
		virtual void	SetScrollDown( bool bDown )		{ UNREFERENCED_PARAMETER( bDown ); }

	// Misc.

		virtual void	SetScrolling( bool bScrolling )	{ UNREFERENCED_PARAMETER( bScrolling ); }
		virtual bool	GetScrolling()					{ return false; }
};


#endif 
