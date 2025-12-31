//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_textbox.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once 
#ifndef __UI_TEXTBOX_H
#define __UI_TEXTBOX_H


// Forward Declarations
class UISlider;
class UIWindow;

#include "ui_scroll_window.h"

struct ColorRange
{
	DWORD		dwColor;
	int			beginRange;
	int			endRange;
};

typedef std::vector< ColorRange > ColorRanges;

struct ColorSegment
{
	DWORD		dwColor;
	gpwstring	sText;
	int			xOffset;
};

typedef std::vector< ColorSegment > ColorSegments;

struct TextLine
{
	DWORD		dwColor;
	gpwstring	sText;
	ColorSegments colorSegments;
};
typedef std::vector< TextLine > TextLineVec;


class UITextBox : public UIWindow
{
	FUBI_CLASS_INHERIT( UITextBox, UIWindow );

public:
	// Public Methods
	
	// Constructor
	UITextBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UITextBox();

	// Destructor
	virtual ~UITextBox();

	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );
	
	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	void SetText( gpwstring sText );
	int SetLineText( int lineNum, gpwstring sText, DWORD dwColor = 0xFFFFFFFF );

	virtual void Draw();

	virtual void SetVisible	( bool bVisible );	
	
	// Text Color
FEX	void			SetTextColor( unsigned int color )	{ m_dwColor = color;	};
FEX	DWORD			GetTextColor() const				{ return m_dwColor;		};

	void			PackageColorSegments( int beginPos, TextLine & line, const ColorRanges & colorVec );
	void			ConvertColorRangeToSegments( ColorRanges & colorRanges, TextLine & line );	
	void			GetColorRangeVec( gpwstring & sText, ColorRanges & colorVec );
	bool			GetColorRange( int pos, ColorRanges & colorVec, ColorRange & colorRange );	
	
	virtual void	Update( double seconds );

	bool			EnlargeRect();

	bool			DoesTextFit( gpwstring sText );
	bool			DoesLineTextFit( gpwstring sText );

FEX	void			CenterWindowToMousePos();

	virtual void	SetMousePos( unsigned int x, unsigned int y );

	virtual unsigned int		GetNumElements();	
FEX unsigned int   FUBI_RENAME( GetNumElements )() { return ( GetNumElements() ); }
	virtual unsigned int		GetMaxActiveElements();
FEX	unsigned int   FUBI_RENAME( GetMaxActiveElements )() { return ( GetMaxActiveElements() ); }

	virtual int			GetLeadElement()					{ return m_lead_element; }	
FEX	int	   FUBI_RENAME( GetLeadElement() )					{ return ( GetLeadElement() ); }
	virtual void		SetLeadElement( int lead )			{ m_lead_element = lead; }
FEX	void   FUBI_RENAME( SetLeadElement )( int lead )		{ SetLeadElement( lead ); }

	virtual int			GetElementHeight()					{ return m_font_height; }
FEX	int	   FUBI_RENAME( GetElementHeight() )				{ return ( GetElementHeight() ); }

	virtual void		SetScrolling( bool bScrolling );
FEX void   FUBI_RENAME( SetScrolling )( bool bScrolling )	{ SetScrolling( bScrolling ); }
	virtual bool		GetScrolling()						{ return m_bStopText; }
FEX	bool   FUBI_RENAME( GetScrolling )()					{ return ( GetScrolling() ); }

FEX	void			SetScrollRate( float scrollRate )		{ m_scroll_rate = scrollRate; }
FEX	float			GetScrollRate() const					{ return m_scroll_rate; }

	RapiFont *		GetFont() { return m_pFont; }
	void			SetFont( RapiFont * pFont ) { m_pFont = pFont; }

FEX	JUSTIFICATION	GetJustification() const { return m_justification; }
FEX	void			SetJustification( JUSTIFICATION justify ) { m_justification = justify; }

#if !GP_RETAIL
	virtual void	Save( FuelHandle hSave, bool bChild );
#endif

FEX	void			SetFixedLocation( bool bSet ) { m_bFixedLocation = bSet; }
FEX	bool			GetFixedLocation() const { return m_bFixedLocation; }

FEX	void			SetFontHeight( int height ) { m_font_height = height; }
FEX	int				GetFontHeight() const		{ return m_font_height; }

	void			SetToolTipText( gpwstring sText );

FEX	void			SetScrollThrough( bool bSet )	{ m_bScrollThrough = bSet; }
FEX	bool			GetScrollThrough() const		{ return m_bScrollThrough; }

FEX	void			SetDisableUpdate( bool bSet )	{ m_bDisableUpdate = bSet; }
FEX	bool			GetDisableUpdate() const		{ return m_bDisableUpdate; }

FEX	void			SetLineSpacer( int set )		{ m_lineSpacer = set; }
FEX	int				GetLineSpacer() const			{ return m_lineSpacer; }

	int				ReformatTooltip( const gpwstring & sText, int lineNum, DWORD dwColor );

	void			RecalculateSlider();

	void			SetChildSlider( UISlider * pSlider );

private:	

	// $ these are private to prevent accidental usage from C++
FEX	void	SetText			( const gpstring& sText ) { SetText( ::ToUnicode( sText ) ); }
FEX	int		SetLineText		( int lineNum, const gpstring& sText, DWORD dwColor ) { return ( SetLineText( lineNum, ::ToUnicode( sText ), dwColor ) ); }
FEX	void	SetToolTipText	( const gpstring& sText ) { SetToolTipText( ::ToUnicode( sText ) ); }
FEX	int		ReformatTooltip	( const gpstring & sText, int lineNum, DWORD dwColor ) { return ( ReformatTooltip( ::ToUnicode( sText ), lineNum, dwColor ) ); }

	// Private Member Variables
	bool			m_buttondown;
	bool			m_bFixedLocation;
	TextLineVec		m_lines;
	int				m_font_width;
	int				m_font_height;
	RapiFont		*m_pFont;
	DWORD			m_dwColor;	
	JUSTIFICATION	m_justification;
	float			m_scroll_rate;
	float			m_scroll_offset;
	int				m_max_width;
	int				m_max_height;
	UISlider *		m_pVSlider;
	bool			m_bSlider;
	int				m_lead_element;
	bool			m_bStopText;
	int				m_borderPadding;
	bool			m_bScrollThrough;
	bool			m_bFinished;
	bool			m_bNoStop;
	bool			m_bShowFirst;
	bool			m_bDisableUpdate;
	bool			m_bCenterHeight;
	int				m_lineSpacer;	
};

#endif