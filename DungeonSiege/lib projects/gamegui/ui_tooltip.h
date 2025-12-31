//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_tooltip.h
// Author(s):  Chad Queen, Adam Swensen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once 
#ifndef __UI_TOOLTIP_H
#define __UI_TOOLTIP_H


// Forward Declarations
class UIWindow;


class UIToolTip : public UIWindow
{
	public:
	
	// Public Types
	struct TextLine
	{
		DWORD		dwColor;
		gpstring	sText;
	};
	
	typedef std::vector< TextLine > TextLineVec;

	// Public Methods
	
	// Constructor
	UIToolTip( FuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIToolTip();

	// Destructor
	virtual ~UIToolTip();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage msg							);

	void SetText( gpstring sText );
	void SetLineText( int lineNum, gpstring sText, DWORD dwColor = 0xFFFFFFFF );

	virtual void Draw();

	virtual void SetVisible	( bool bVisible );		
	
	// Text Color
	void			SetTextColor( unsigned int color )	{ m_dwColor = color;	};
	DWORD			GetTextColor()						{ return m_dwColor;		};
	
	virtual void	Update( double seconds );

	bool			EnlargeRect();

	bool			DoesTextFit( gpstring sText );

	void			CenterWindowToMousePos();

	virtual void	SetMousePos( unsigned int x, unsigned int y );

	virtual unsigned int		GetNumElements();	
	virtual unsigned int		GetMaxActiveElements();

	virtual int		GetLeadElement()			{ return m_lead_element; }	
	virtual void	SetLeadElement( int lead )	{ m_lead_element = lead; }

	virtual int		GetElementHeight() { return m_font_height; }

	virtual void	SetScrolling( bool bScrolling );
	virtual bool	GetScrolling()					{ return m_bStopText; }

	void			SetScrollRate( float scrollRate )	{ m_scroll_rate = scrollRate; }
	float			GetScrollRate()						{ return m_scroll_rate; }

	RapiFont *		GetFont() { return m_pFont; }
	void			SetFont( RapiFont * pFont ) { m_pFont = pFont; }

	JUSTIFICATION	GetJustification() { return m_justification; }
	void			SetJustification( JUSTIFICATION justify ) { m_justification = justify; }

	virtual void	Save( FuelHandle hSave, bool bChild );

	void			SetFixedLocation( bool bSet ) { m_bFixedLocation = bSet; }
	bool			GetFixedLocation() { return m_bFixedLocation; }

	void			SetFontHeight( int height ) { m_font_height = height; }
	int				GetFontHeight()				{ return m_font_height; }


private:	

	// Private Member Variables
	bool			m_buttondown;
	bool			m_bFixedLocation;
	TextLineVec		m_lines;
	int				m_font_width;
	int				m_font_height;
	RapiFont		*m_pFont;
	DWORD			m_dwColor;
	int				m_font_size;
	JUSTIFICATION	m_justification;
	float			m_scroll_rate;
	float			m_scroll_offset;
	int				m_max_width;
	int				m_max_height;
	int				m_lead_element;
	bool			m_bStopText;

};

#endif