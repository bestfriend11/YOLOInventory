//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_listreport.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UI_LISTREPORT_H
#define __UI_LISTREPORT_H

#include "gpstring.h"

class UIWindow;
class UIMessage;
class RapiFont;
class UISlider;
class UIButton;


enum eColumnSort
{
	COLUMN_SORT_NONE,
	COLUMN_SORT_ASCENDING,
	COLUMN_SORT_DESCENDING,
};

// Elements are the data in the list control
struct UI_LISTREPORT_ELEMENT
{
	gpwstring	sText;
	int			ID;
	int			tag;
	bool		bSelected;
	DWORD		dwTextColor;
	int			icon;
};

typedef std::vector< UI_LISTREPORT_ELEMENT > ReportListElementVec;
typedef std::map< int, UI_LISTREPORT_ELEMENT > ReportListElementMap;
typedef std::pair< int, UI_LISTREPORT_ELEMENT > ElementPair;

// Columns are how the data is split up on the line
struct UI_LISTREPORT_COLUMN
{
	gpwstring				sName;
	int						width;
	eColumnSort				sort_format;
	ReportListElementMap	elements;
	UIButton				*pButton;
	bool					allowSort;
	bool					fixedWidth;
	DWORD					dwTextColor;
};
typedef std::vector< UI_LISTREPORT_COLUMN > ReportColumnVec;


class UIListReport : public UIWindow
{
public:

	enum
	{
		MIN_COLUMN_WIDTH		= 48,
		ELEMENT_HEIGHT_OFFSET	= 6,
		SEPERATOR_PIXEL_WIDTH	= 1,
		X_OFFSET				= 5,
		NO_COLUMN_SELECTED		= -1,
		TAG_NONE				= -1,
		INVALID_ROW				= -1,
		NO_ICON					= -1,
	};

	// Constructor
	UIListReport( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIListReport();

	virtual void Update( double seconds );

	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );

	// Destructor
	virtual ~UIListReport();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Column Manipulation
	void InsertColumn( gpwstring sName, int width, UIButton *pButton = NULL );

	int	GetColumnCount()						{  return m_report_columns.size();  }

	// Element Manipulation
	void AddElement( gpwstring sText, DWORD textColor, int tag );
	void AddElement( gpwstring sText, int tag = TAG_NONE )			{  AddElement( sText, GetTextColor(), tag );  }
	void SetElementText( gpwstring sText, int column, int row );
	void SetElementColor( DWORD color, int column, int row );
	void RemoveElement( int id );
	void RemoveAllElements();
	void RemoveAllColumns();

	// Query
	int						GetSelectedElementID()	{ return m_selected_element; }
	int						GetSelectedElementTag();
	gpwstring				GetSelectedElementText( int column = 0 );
	gpwstring				GetElementText( int column, int row );	
	UI_LISTREPORT_ELEMENT * GetElement( int id );
	UI_LISTREPORT_ELEMENT * GetElementByTag( int tag );
	UI_LISTREPORT_ELEMENT * GetElementByText( gpwstring sText );
	int						GetElementRowByID( int id );
	int						GetElementRowByTag( int tag );
	int						GetElementRowByText( gpwstring sText );

	void					SetSelectedElement( int rowId )		{  m_selected_element = rowId;  }

	// Deselect All Elements
	void	DeselectElements();
	
	// Render the control
	virtual void Draw();
	
	virtual void SetVisible( bool bVisible );

	// Set the current mouse click position
	virtual void SetMousePos( unsigned int x, unsigned int y );
	
	// Get the number of elements in the listbox
	virtual unsigned int GetNumElements();
	virtual unsigned int GetMaxActiveElements();	
		
	// Get/Set the "Lead" element index; the element that appears "first" ( the top ) in the window 
	virtual int		GetLeadElement()				{ return m_lead_element; }
	virtual void	SetLeadElement( int lead )		{ m_lead_element = lead; }

	// Text Color
	void			SetTextColor( unsigned int color )	{ m_dwColor = color;	};
	unsigned int	GetTextColor()						{ return m_dwColor;		};
		
	// Column sizes in pixels ( used for slider controls )
	int		GetColumnSize();
	int		GetTotalColumnSize();
	
	// Element ( rows of information ) sizes in pixels
	int	GetTotalElementSize();
	int	GetElementsSize();		// returns how much space there is in the window to display elements

	// Scroll information
//	void	SetHorizontalPos( int pos ) { m_horizontal_pos = pos; }
	void	SetVerticalPos( int pos )	{ m_vertical_pos = pos; }
//	int		GetHorizontalPos()			{ return m_horizontal_pos; }
	int		GetVerticalPos()			{ return m_vertical_pos; }	

	UISlider		*GetChildSlider()						{ return m_pVSlider;		};

	// The rectangle that isn't obscured by sliders or column headers
	UIRect &		GetUsableRect() { return m_rect_usable; }
	
	virtual int		GetElementHeight() { return m_element_height; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	void	SetColumnHeaderText( int column, gpwstring text, unsigned int color = 0xffffffff );

	void	SetColumnWidthFixed( int column, bool fixed );

	void	SetAllowColumnSort( int column, bool allow );
	
	void	SetSortColumn( int column, eColumnSort sort = COLUMN_SORT_DESCENDING );
	int		GetSortColumn();

	void	SortColumn( int column );
	int		GetActiveColumn() { return m_activeColumn; }
	
	int		GetColumnSize( int column );

	void	AddIcon( gpstring texture, int icon );

	void	SetElementIcon( int tag, int icon );
	void	SetElementIcon( int column, int row, int icon );

	void	ResizeSlider();

	void	SetShowColumnButtons( bool show )		{ m_bShowColumnButtons = show; }

	int		GetLastKeyPressed()						{ return m_KeyPressed; }

private:

	typedef std::map< int, unsigned int > IconMap;

	// Load in the texture for the selection
	void LoadSelectionTexture( gpstring texture );

	// Drawing
	void DrawElementTexture( UIRect element_rect, double alpha, unsigned int index );
	void DrawColumns();
	void DrawColumn( gpwstring sColumnName, UIRect column_rect, UI_LISTREPORT_COLUMN & column );
	
	// Child window accessors
//	UISlider * GetHSlider() { return m_pHSlider; }
	UISlider * GetVSlider() { return m_pVSlider; }

	// Slider Activation
//	void SetHSliderActive( bool bActive );
//	bool GetHSliderActive()					{ return m_bHSliderActive; }

	// Set/Get Element height/width
	void	SetHeight( int height );
	int		GetHeight()				{ return m_element_height;		};		

	// Sort Ascending or Descending in one column	
	void	SortColumn( UI_LISTREPORT_COLUMN & column );
	
	// For column clicking/dragging handling
	void	BeginColumnDragSize();
	bool	IsXInRange( int x1, int x2 );
	void	SetDragSizeColumn( int column_index )	{ m_drag_size_column = column_index;	}
	int		GetDragSizeColumn()						{ return m_drag_size_column;			}	

	bool	HitDetectElement();
	void	DrawSelection();	

	virtual void ResizeToCurrentResolution( int original_width, int original_height );

	// Private Variables
	RapiFont *			m_pFont;
	unsigned int		m_dwColor;
	ReportColumnVec		m_report_columns;
	int					m_font_size;
	int					m_selected_element;
	int					m_element_height;
	int					m_id_counter;
	int					m_max_active;
	int					m_lead_element;
	UIRect				m_rect_usable;
	UIRect				m_rect_sort_icon;
	unsigned int		m_SelectionColor;
	UISlider *			m_pVSlider;
	bool				m_buttondown;
	int					m_drag_size_column;
	int					m_vertical_pos;
	int					m_activeColumn;
	IconMap				m_Icons;
	int					m_IconSize;
	bool				m_bCustomColumnSort;
	bool				m_bDualColumnSort;	
	bool				m_bShowColumnButtons;
	int					m_KeyPressed;

	unsigned int		m_tex_sort_icon_up;						
	unsigned int		m_tex_sort_icon_down;	
	unsigned int		m_tex_separator;
	unsigned int		m_tex_header_bg;
	unsigned int		m_tex_selection;

//	bool				m_bHSliderActive;
//	int					m_horizontal_pos;
//	UISlider *			m_pHSlider;

};

#endif