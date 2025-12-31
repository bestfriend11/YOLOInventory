//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_listreport.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_listreport.h"
#include "ui_window.h"
#include "fuel.h"
#include "rapiowner.h"
#include "rapifont.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_slider.h"
#include "ui_button.h"
#include "LocHelp.h"
#include "Keys.h"


UIListReport::UIListReport( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_pFont( 0 )
	, m_dwColor( 0 )	
	, m_font_size( 0 )
	, m_selected_element( -1 )
	, m_element_height( 0 )
	, m_id_counter( 0 ) 
	, m_max_active( 0 )
	, m_lead_element( 0 )
	, m_SelectionColor( 0x991647FC )
	, m_pVSlider( NULL )
	, m_buttondown( false )
	, m_drag_size_column( NO_COLUMN_SELECTED )
	, m_vertical_pos( 0 )		
	, m_activeColumn( 0 )
	, m_IconSize( 16 )
	, m_bCustomColumnSort( false )
	, m_bDualColumnSort( true )
	, m_bShowColumnButtons( true )	
	, m_KeyPressed( 0 )
	, m_tex_sort_icon_up( 0 )
	, m_tex_sort_icon_down( 0 )
	, m_tex_separator( 0 )
	, m_tex_header_bg( 0 )
	, m_tex_selection( 0 )
//	, m_bHSliderActive( false )
//	, m_horizontal_pos( 0 )
//	, m_pHSlider( NULL )
{	
	SetType( UI_TYPE_LISTREPORT );
	SetInputType( TYPE_INPUT_ALL );

	fhWindow.Get( "custom_column_sort", m_bCustomColumnSort );
	fhWindow.Get( "dual_column_sort", m_bDualColumnSort );

	SetTextColor( 0xFFFFFFFF );	
	
	gpstring sFont;
	fhWindow.Get( "font_color", m_dwColor );
	fhWindow.Get( "font_size", m_font_size );
	if ( fhWindow.Get( "font_type", sFont ) ) 
	{
		m_pFont = gUITextureManager.LoadFont( sFont, true );
		if ( !m_pFont ) 
		{
			DWORD dwBgColor = 0;
			if ( fhWindow.Get( "font_bg_color", dwBgColor ) )
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_size, dwBgColor );
			}
			else
			{
				m_pFont = gUITextureManager.LoadFont( sFont, m_font_size );
			}
		}
	}

	fhWindow.Get( "selection_color", m_SelectionColor );

	if ( !m_pFont ) 
	{
		gperrorf(( "Using default system font for GUI element: %s, change it!", GetName().c_str() ));
		m_pFont = gUITextureManager.GetDefaultFont();
	}

	bool bCommonCtrl = false;
	if ( fhWindow.Get( "common_control", bCommonCtrl ) ) 
	{
		if ( bCommonCtrl ) 
		{
			CreateCommonCtrl();		
		}
		return;
	}
}
UIListReport::UIListReport()
	: UIWindow()
	, m_pFont( 0 )
	, m_dwColor( 0 )	
	, m_font_size( 0 )
	, m_selected_element( -1 )
	, m_element_height( 0 )
	, m_id_counter( 0 ) 
	, m_max_active( 0 )
	, m_lead_element( 0 )
	, m_SelectionColor( 0x991647FC )
	, m_pVSlider( NULL )
	, m_buttondown( false )
	, m_drag_size_column( NO_COLUMN_SELECTED )
	, m_vertical_pos( 0 )		
	, m_activeColumn( 0 )
	, m_IconSize( 16 )
	, m_bCustomColumnSort( false )
	, m_bDualColumnSort( true )
	, m_bShowColumnButtons( true )	
	, m_KeyPressed( 0 )
	, m_tex_sort_icon_up( 0 )
	, m_tex_sort_icon_down( 0 )
	, m_tex_separator( 0 )
	, m_tex_header_bg( 0 )
	, m_tex_selection( 0 )
//	, m_bHSliderActive( false )
//	, m_horizontal_pos( 0 )
//	, m_pHSlider( NULL )
{	
	SetType( UI_TYPE_LISTREPORT );
	SetInputType( TYPE_INPUT_ALL );

	SetTextColor( 0xFFFFFFFF );	
	gpstring sFont;	
}



UIListReport::~UIListReport()
{
	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() && m_pFont )
	{
		ReportColumnVec::iterator i;
		for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
		{
			ReportListElementMap::iterator j;	
			for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
			{		
				m_pFont->UnregisterString( (*j).second.sText );						
			}	
		}
	}

	if ( m_tex_sort_icon_up != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_tex_sort_icon_up );
	}

	if ( m_tex_sort_icon_down != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_tex_sort_icon_down );
	}

	if ( m_tex_separator != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_tex_separator );
	}

	if ( m_tex_header_bg != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_tex_header_bg );
	}

	if ( m_tex_selection != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_tex_selection );
	}

	{for ( IconMap::iterator i = m_Icons.begin(); i != m_Icons.end(); ++i )
	{
		if ( i->second > 0 )
		{
			gUIShell.GetRenderer().DestroyTexture( i->second );
		}
	}}
}


void UIListReport::Update( double /* seconds */ )
{
	if ( GetVisible() )
	{
		if ( GetLeadElement() >= (int)GetNumElements() )
		{
			SetLeadElement( 0 );
			if ( GetVSlider() )
			{
				GetVSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
				GetVSlider()->CalculateSlider();	
			}
		}
	}
}


void UIListReport::CreateCommonCtrl( gpstring /*sTemplate*/ )
{	
	SetIsCommonControl( true );

	if ( GetTextureIndex() != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
	}

	// Load textures
	m_tex_sort_icon_up		= gUIShell.GetCommonArt( "header_sep_up" ); 
	m_tex_sort_icon_down	= gUIShell.GetCommonArt( "header_sep_down" );
	m_tex_separator			= gUIShell.GetCommonArt( "header_sep" ); 
	m_tex_header_bg			= gUIShell.GetCommonArt( "header_background" );  
	m_tex_selection			= gUIShell.GetCommonArt( "listreport_selection" );

	// Set element height
	int font_width = 0;
	int font_height = 0;
	m_pFont->CalculateStringSize( "test", font_width, font_height );
	m_element_height = font_height + ELEMENT_HEIGHT_OFFSET;
	SetHeight( m_element_height );

	// Init draw rects
	m_rect_usable.left		= GetRect().left;
	m_rect_usable.right		= GetRect().right;
	m_rect_usable.top		= GetRect().top + m_element_height;
	m_rect_usable.bottom	= GetRect().bottom;

	m_rect_sort_icon.top	= 0;
	m_rect_sort_icon.left	= 0;
	m_rect_sort_icon.right	= gUIShell.GetRenderer().GetTextureWidth( m_tex_sort_icon_up );
	m_rect_sort_icon.bottom = gUIShell.GetRenderer().GetTextureHeight( m_tex_sort_icon_up );

	// Create vertical slider
	m_pVSlider = (UISlider *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_SLIDER );
	m_pVSlider->CreateCommonCtrl( GetName() + "_slider_vertical", "slider", this, true );
	m_pVSlider->SetRect( GetUsableRect().right - 16, GetUsableRect().right, GetUsableRect().top, GetUsableRect().bottom );
	m_pVSlider->SetDrawOrder( GetDrawOrder() + 1 );
	m_pVSlider->SetConsumable( true );
	m_pVSlider->SetInterfaceParent( GetInterfaceParent() );
	m_pVSlider->CreateSliderButtons();
	gUIShell.AddWindowToInterface( (UIWindow *)m_pVSlider, GetInterfaceParent() );
	AddChild( m_pVSlider );

	ResizeSlider();

	/*
	UIButton * pButtonLeft	= 0;
	UIButton * pButtonRight	= 0;
	UISlider * pSliderH		= 0;

	pButtonLeft = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
	gpstring sName = GetName();
	sName += "_button_left";
	pButtonLeft->CreateCommonCtrl( sName, "button_left", this, SLIDER_DECREMENT );
	pButtonLeft->SetRect(	GetRect().left, GetRect().left+pButtonLeft->GetRect().right,
							GetRect().bottom-pButtonLeft->GetRect().bottom, GetRect().bottom );		
	pButtonLeft->SetDrawOrder( GetDrawOrder() + 1 );
	gUIShell.AddWindowToInterface( (UIWindow *)pButtonLeft, GetInterfaceParent() );

	pButtonRight = (UIButton *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_BUTTON );
	sName = GetName();
	sName += "_button_right";
	pButtonRight->CreateCommonCtrl( sName, "button_right", this, SLIDER_INCREMENT );
	pButtonRight->SetRect(	GetRect().right-(pButtonRight->GetRect().right*2), GetRect().right-pButtonRight->GetRect().right,
							GetRect().bottom-pButtonRight->GetRect().bottom, GetRect().bottom );		
	pButtonRight->SetDrawOrder( GetDrawOrder() + 1 );
	gUIShell.AddWindowToInterface( (UIWindow *)pButtonRight, GetInterfaceParent() );		
	pSliderH = (UISlider *)gUIShell.CreateDefaultWindowOfType( UI_TYPE_SLIDER );
	sName = GetName();
	sName += "_slider_horizontal";
	pSliderH->CreateCommonCtrl( sName, "slider", this, false );
	pSliderH->SetRect(  pButtonLeft->GetRect().right, pButtonRight->GetRect().left,
						pButtonLeft->GetRect().top, GetRect().bottom );
	pSliderH->SetDrawOrder( GetDrawOrder() + 1 );
	gUIShell.AddWindowToInterface( (UIWindow *)pSliderH, GetInterfaceParent() );

	pButtonLeft->SetParentWindow( pSliderH );
	pButtonRight->SetParentWindow( pSliderH );
	pSliderH->AddChild( pButtonLeft );
	pSliderH->AddChild( pButtonRight );
	AddChild( pSliderH );
	SetHSliderActive( false );
	m_pHSlider = pSliderH;
	*/
}


bool UIListReport::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_SETELEMENTHEIGHT ) {
		SetHeight( atoi( parameter.c_str() ) );
		return true;
	}
	else if ( action == ACTION_ADD_ELEMENT ) {
		int strings = stringtool::GetNumDelimitedStrings( parameter, ',' );
		if ( strings == 1 ) {			
			AddElement( ToUnicode( parameter ) );
		}
		else {
			gpstring text;
			int tag;
			stringtool::GetDelimitedValue( parameter, ',', 0, text		);
			stringtool::GetDelimitedValue( parameter, ',', 1, tag		);			
			AddElement( ToUnicode( text ), tag );
		}
		return true;
	}	

	return false;
}


bool UIListReport::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;	
			BeginColumnDragSize();
			HitDetectElement();
		}
		break;
	case MSG_LBUTTONUP:
	case MSG_GLOBALLBUTTONUP:
		{			
			m_buttondown = false;
		}
		break;
	case MSG_LDOUBLECLICK:
		{
			if ( HitDetectElement() )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_SELECT ), this );
			}
		}
		break;
	case MSG_WHEELUP:
		{
			if ( GetChildSlider() )
			{
				GetChildSlider()->SetValue( GetChildSlider()->GetValue() - 2 );
			}
		}
		break;
	case MSG_WHEELDOWN:
		{
			if ( GetChildSlider() )
			{
				GetChildSlider()->SetValue( GetChildSlider()->GetValue() + 2 );
			}
		}
		break;
	case MSG_ROLLOVER:
		{				
		}
		break;
	case MSG_ROLLOFF:
		{
		}
		break;
	case MSG_KEYPRESS:
		{
			if ( GetVisible() && HasFocus() )
			{
				m_KeyPressed = msg.GetKey();

				switch ( msg.GetKey() )
				{
					case Keys::KEY_INT_UP:
					{
						ReportColumnVec::iterator column = m_report_columns.begin();
						ReportListElementMap::iterator prevElement = column->elements.begin();
						for ( ReportListElementMap::iterator i = column->elements.begin(); i != column->elements.end(); ++i )
						{
							if ( m_selected_element == i->second.ID )
							{
								m_selected_element = prevElement->second.ID;
								break;
							}
							prevElement = i;
						}
						break;
					}

					case Keys::KEY_INT_DOWN:
					{
						ReportColumnVec::iterator column = m_report_columns.begin();
						ReportListElementMap::iterator nextElement = column->elements.begin();
						++nextElement;
						for ( ReportListElementMap::iterator i = column->elements.begin(); nextElement != column->elements.end(); ++i, ++nextElement )
						{
							if ( m_selected_element == i->second.ID )
							{
								m_selected_element = nextElement->second.ID;
								break;
							}
						}
						break;
					}
				}
			}
		}
		break;
	case MSG_ENTER:
		{
			if ( HasFocus() && HitDetectElement() )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_SELECT ), this );
			}
		}
		break;
	}
	return true;
}


void UIListReport::SetVisible( bool bVisible )
{
	UIWindow::SetVisible( bVisible );

	if ( bVisible && GetChildSlider() && (GetChildSlider()->GetMax() == GetChildSlider()->GetMin()) )
	{
		GetChildSlider()->SetVisible( false );
	}
}


void UIListReport::SetMousePos( unsigned int x, unsigned int y )
{
	UIWindow::SetMousePos( x, y );	
	if ( (m_buttondown && gUIShell.GetLButtonDown()) && ( GetDragSizeColumn() != NO_COLUMN_SELECTED ))
	{
		int index		= 0;
		int left_edge	= GetRect().left;
		int right_edge	= GetRect().left;
		for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
		{
			right_edge += (*i).width + SEPERATOR_PIXEL_WIDTH;
			if ( index == GetDragSizeColumn() )
			{
				int widthShift = x - right_edge;

				if ( widthShift < 0 )
				{
					int buttonWidth = i->pButton ? i->pButton->GetRect().Width() + 10 : 0;
					int minColWidth = MIN_COLUMN_WIDTH + buttonWidth;
					if ( i->width + widthShift > minColWidth )
					{
						i->width += widthShift;
					}
					else
					{
						i->width = minColWidth;
					}
				}
				else if ( widthShift > 0 )
				{
					int totalRightWidth = 0;
					for ( ReportColumnVec::iterator j = (i + 1); j != m_report_columns.end(); ++j )
					{
						totalRightWidth += j->width + SEPERATOR_PIXEL_WIDTH;
					}

					int rightOverlap = (right_edge + widthShift + totalRightWidth) - GetUsableRect().right;
					if ( rightOverlap < 0 )
					{
						i->width += widthShift;
					}
					else if ( rightOverlap > 0 )
					{
						ReportColumnVec::reverse_iterator shrink = m_report_columns.rbegin();
						int shrinkIndex = m_report_columns.size() - 1;
						while ( shrinkIndex > index )
						{
							int buttonWidth = shrink->pButton ? shrink->pButton->GetRect().Width() + 10 : 0;
							int minColWidth = MIN_COLUMN_WIDTH + buttonWidth;
							if ( shrink->width - rightOverlap > minColWidth )
							{
								shrink->width -= rightOverlap;
								rightOverlap = 0;
								break;
							}
							else
							{
								rightOverlap -= (shrink->width - minColWidth);
								shrink->width = minColWidth;
							}

							--shrinkIndex;
							++shrink;
						}
						i->width += (widthShift - rightOverlap);
					}
				}
				break;
			}
			++index;
		}
		left_edge += (*i).width + SEPERATOR_PIXEL_WIDTH;
	}

	if ( m_buttondown && !gUIShell.GetLButtonDown() )
	{
		m_buttondown = false;
	}
}


void UIListReport::BeginColumnDragSize()
{
	SetDragSizeColumn( NO_COLUMN_SELECTED );
	if ( m_buttondown ) 
	{		
		int y1 = GetRect().top;
		int x1 = GetRect().left;
		ReportColumnVec::iterator i;		
		int index = 0;
		bool bSortSet = false;
		for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
		{
			int colWidth = i->width;

			if ( !i->fixedWidth && (i == (m_report_columns.end() - 1)) )
			{
				colWidth = GetUsableRect().right - x1;
				x1 = GetUsableRect().right;
			}
			else
			{
				x1 += (*i).width;			
			}

			int x2 = x1 + 1;
			int y2 = m_rect_usable.top;
			if ( IsXInRange( x1, x2 ) )
			{
				if (( GetMouseY() <= y2 ) && ( GetMouseY() >= y1 ))
				{
					SetDragSizeColumn( index );
					return;
				}
			}
			else if ( !bSortSet && i->allowSort )
			{
				if (( GetMouseY() <= y2 ) && ( GetMouseY() >= y1 ))
				{
					if (( (x1-colWidth) <= (GetMouseX()) ) && ( x1 >= (GetMouseX()) )) 
					{
						if ( !m_bDualColumnSort || ( (*i).sort_format == COLUMN_SORT_NONE ) || ( (*i).sort_format == COLUMN_SORT_ASCENDING ))
						{
							(*i).sort_format = COLUMN_SORT_DESCENDING;
						}
						else
						{
							(*i).sort_format = COLUMN_SORT_ASCENDING;
						}
						
						m_activeColumn = index;

						// Can only sort one column at a time
						ReportColumnVec::iterator j;
						int sort_index = 0;
						for ( j = m_report_columns.begin(); j != m_report_columns.end(); ++j ) 
						{
							if ( sort_index != index ) 
							{
								(*j).sort_format = COLUMN_SORT_NONE;
							}
							sort_index++;
						}

						if ( m_bCustomColumnSort )
						{
							gUIMessenger.SendUIMessage( UIMessage( MSG_CHANGE ), this );
						}
						else
						{
							SortColumn( *i );
						}

						bSortSet = true;						
					}
				}
			}
			index++;
		}
	}
}


bool UIListReport::IsXInRange( int x1, int x2 )
{
	x1 -= X_OFFSET;
	x2 += X_OFFSET;
	if (( GetMouseX() >= x1 ) && ( GetMouseX() <= x2 ))
	{
		return true;
	}
	return false;
}


void UIListReport::AddElement( gpwstring sText, DWORD textColor, int tag )
{
	ReportColumnVec::iterator iColumn;
	if ( m_report_columns.size() == 0 )
	{
		return;
	}
	else
	{
		iColumn = m_report_columns.begin();
	}

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_pFont->RegisterString( sText );						
	}

	UI_LISTREPORT_ELEMENT *pElement = 0;
	if ( tag != TAG_NONE ) 
	{
		pElement = GetElementByTag( tag );
	}

	if ( pElement == NULL ) 
	{
		ReportColumnVec::iterator j;
		for ( j = m_report_columns.begin()+1; j != m_report_columns.end(); ++j )
		{
			UI_LISTREPORT_ELEMENT element;
			element.sText.clear();
			element.dwTextColor		= textColor;
			element.ID				= 0;
			element.bSelected		= false;		
			element.tag				= TAG_NONE;
			element.icon			= NO_ICON;
			(*j).elements.insert( ElementPair( (int)(*iColumn).elements.size(), element ) );		
		}

		UI_LISTREPORT_ELEMENT element;
		element.sText			= sText;
		element.dwTextColor		= textColor;
		element.ID				= m_id_counter++;
		element.bSelected		= false;		
		element.tag				= tag;
		element.icon			= NO_ICON;
		(*iColumn).elements.insert( ElementPair( (int)(*iColumn).elements.size(), element ) );			
	}
	else
	{
		pElement->sText			= sText;		
	}		

	ResizeSlider();
}


void UIListReport::SetElementText( gpwstring sText, int column, int row )
{
	ReportColumnVec::iterator i;
	ReportListElementMap::iterator j;	
	int col_index = 0;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( column == col_index )
		{
			int row_index = 0;			

			if ( (int)(*i).elements.size() < (row+1) )
			{
				for ( int o = 0; o != (row+1); ++o )
				{
					UI_LISTREPORT_ELEMENT element;
					element.sText.clear();
					element.dwTextColor		= GetTextColor();
					element.ID				= 0;
					element.bSelected		= false;		
					element.tag				= 0;
					(*i).elements.insert( ElementPair( o, element ) );
				}
			}			

			for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
			{
				if ( row_index == row )
				{
					if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
					{
						m_pFont->UnregisterString( (*j).second.sText );						
						m_pFont->RegisterString( sText );
					}
					(*j).second.sText = sText;
					return;
				}

				row_index++;
			}
		}

		col_index++;
	}
}


void UIListReport::SetElementColor( DWORD color, int column, int row )
{
	ReportColumnVec::iterator i;
	ReportListElementMap::iterator j;	
	int col_index = 0;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( column == col_index )
		{
			int row_index = 0;			

			for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
			{
				if ( row_index == row )
				{
					(*j).second.dwTextColor = color;
					return;
				}

				row_index++;
			}
		}

		col_index++;
	}
}


void UIListReport::SetHeight( int height )
{
	m_element_height = height;
	int box_height = GetRect().bottom - GetRect().top;
	
	if ( height != 0 ) 
	{
		m_max_active = (int)(box_height/height);
	}
	else 
	{
		m_max_active = 0;
	}
}


unsigned int UIListReport::GetMaxActiveElements()
{	
	int box_height = GetUsableRect().bottom - GetUsableRect().top;
	
	if ( m_element_height != 0 ) 
	{
		m_max_active = (int)(floor((double)box_height/(double)m_element_height));
	}
	else 
	{
		m_max_active = 0;
	}

	return m_max_active;
}


void UIListReport::InsertColumn( gpwstring sName, int width, UIButton *pButton )
{
	UI_LISTREPORT_COLUMN column;
	column.sName		= sName;
	column.width		= width;
	column.sort_format	= COLUMN_SORT_NONE;
	column.pButton		= pButton;
	column.allowSort	= true;
	column.fixedWidth	= false;
	column.dwTextColor	= GetTextColor();

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		m_pFont->RegisterString( sName );						
	}

	if ( m_report_columns.size() != 0 ) 
	{
		ReportColumnVec::iterator iColumn;
		iColumn = m_report_columns.begin();	
		for ( int i = 0; i != (int)(*iColumn).elements.size(); ++i )
		{
			UI_LISTREPORT_ELEMENT element;
			element.sText.clear();
			element.ID				= 0;
			element.bSelected		= false;		
			element.tag				= 0;
			column.elements.insert( ElementPair( i, element ) );		
		}
	}

	m_report_columns.push_back( column );
}


void UIListReport::RemoveAllElements()
{	
	m_id_counter	= 0;
	ReportColumnVec::iterator i;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		ReportListElementMap::iterator j;	
		for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
		{
			if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
			{
				m_pFont->UnregisterString( (*j).second.sText );						
			}		
		}

		(*i).elements.clear();
	}

	m_selected_element = -1;

	ResizeSlider();
}


void UIListReport::RemoveAllColumns()
{
	ReportColumnVec::iterator i;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
		{
			m_pFont->UnregisterString( (*i).sName );						
		}
	}
	m_report_columns.clear();
}


void UIListReport::RemoveElement( int id )
{
	ReportColumnVec::iterator i;
	ReportListElementMap::iterator j;	
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j ) 
		{
			if ( (*j).second.ID == id ) 
			{
				if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
				{
					m_pFont->UnregisterString( (*j).second.sText );						
				}
				(*i).elements.erase( j );
				break;
			}
		}
	}
	ResizeSlider();
}

int UIListReport::GetSelectedElementTag()
{
//$$$	int element = 0;

	ReportColumnVec::iterator iColumn = m_report_columns.begin();
	for ( ReportListElementMap::iterator i = (*iColumn).elements.begin(); i != (*iColumn).elements.end(); ++i )
	{
		if ( m_selected_element == i->second.ID )
		{
			return i->second.tag;
		}
//		++element;
	}
	
	return TAG_NONE;
}


UI_LISTREPORT_ELEMENT * UIListReport::GetElement( int id )
{
	ReportColumnVec::iterator i;
	ReportListElementMap::iterator j;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
		{
			if ( (*j).second.ID == id )
			{
				return (&(*j).second);
			}
		}
	}

	return NULL;
}
	

UI_LISTREPORT_ELEMENT * UIListReport::GetElementByTag( int tag )
{
	ReportColumnVec::iterator i;
	ReportListElementMap::iterator j;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
		{
			if ( (*j).second.tag == tag )
			{
				return (&(*j).second);
			}
		}
	}

	return NULL;
}


UI_LISTREPORT_ELEMENT * UIListReport::GetElementByText( gpwstring sText )
{
	for ( ReportColumnVec::iterator iColumn = m_report_columns.begin(); iColumn != m_report_columns.end(); ++iColumn )
	{
		for ( ReportListElementMap::iterator i = iColumn->elements.begin(); i != iColumn->elements.end(); ++i )
		{
			if ( i->second.sText.same_no_case( sText ) )
			{
				return &i->second;
			}
		}
	}

	return NULL;
}


gpwstring UIListReport::GetSelectedElementText( int column )
{
//$$$	int element = 0;

	for ( ReportColumnVec::iterator iColumn = m_report_columns.begin(); iColumn != m_report_columns.end(); ++iColumn )
	{
		if ( column == 0 )
		{
			for ( ReportListElementMap::iterator i = (*iColumn).elements.begin(); i != (*iColumn).elements.end(); ++i )
			{
				if ( i->second.ID == m_selected_element )
				{
					return i->second.sText;
				}
				//++element;
			}
		}
		--column;
	}
	
	return gpwstring::EMPTY;
}


gpwstring UIListReport::GetElementText( int column, int row )
{
	int iColumn = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i, ++iColumn )
	{
		if ( iColumn == column )
		{
			int iRow = 0;
			for ( ReportListElementMap::iterator j = (*i).elements.begin(); j != (*i).elements.end(); ++j, ++iRow )
			{
				if ( iRow == row )
				{
					return ( j->second.sText );
				}
			}
		}
	}
	return ( gpwstring() );
}


int UIListReport::GetElementRowByID( int id )
{
	if ( m_report_columns.size() == 0 )
	{
		return INVALID_ROW;
	}

	ReportColumnVec::iterator i = m_report_columns.begin();
	ReportListElementMap::iterator j;
	int row = 0;
	for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
	{
		if ( (*j).second.ID == id )
		{
			return row;
		}
		++row;
	}
	return INVALID_ROW;
}


int UIListReport::GetElementRowByTag( int tag )
{
	if ( m_report_columns.size() == 0 )
	{
		return INVALID_ROW;
	}

	ReportColumnVec::iterator i = m_report_columns.begin();
	ReportListElementMap::iterator j;
	int row = 0;
	for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
	{
		if ( (*j).second.tag == tag )
		{
			return row;
		}
		++row;
	}
	return INVALID_ROW;
}


int UIListReport::GetElementRowByText( gpwstring sText )
{
	if ( m_report_columns.size() == 0 )
	{
		return INVALID_ROW;
	}

	ReportColumnVec::iterator i = m_report_columns.begin();
	ReportListElementMap::iterator j;
	int row = 0;
	for ( j = (*i).elements.begin(); j != (*i).elements.end(); ++j )
	{
		if ( (*j).second.sText.same_no_case( sText ) )
		{
			return row;
		}
		++row;
	}
	return INVALID_ROW;
}

/*
void UIListReport::SetHSliderActive( bool bActive )
{
	UIWindowVec::iterator i;
	for ( i = GetChildren().begin(); i != GetChildren().end(); ++i )
	{
		if (( (*i)->GetType() == UI_TYPE_SLIDER ) && (!((UISlider *)(*i))->IsVertical()) ) 
		{
			(*i)->SetVisible( bActive );
		}			
	}

	m_bHSliderActive = bActive;
	if ( bActive ) 
	{
		GetHSlider()->CalculateSlider();
		m_rect_usable.bottom = m_pHSlider->GetRect().top;
	}
	else
	{
		m_rect_usable.bottom = GetRect().bottom;
	}
}
*/


int UIListReport::GetColumnSize()
{
	return( GetRect().right - GetRect().left );
}


int UIListReport::GetTotalColumnSize()
{
	int total_width = 0;
	ReportColumnVec::iterator i;		
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		total_width += (*i).width + SEPERATOR_PIXEL_WIDTH;
	}		
	return total_width;
}


void UIListReport::Draw()
{
	if (( GetVisible() == true ) ){ //&& ( HasTexture() )) {
		UIWindow::Draw();
		DrawColumns();			
	}	
}


void UIListReport::DrawColumns()
{
	D3DRECT rect;
	rect.x1 = GetRect().left;
	rect.x2 = GetRect().right;
	rect.y1 = GetRect().top;
	rect.y2 = GetRect().bottom;

	int total_width = 0;	

	UIRect column_rect;
	column_rect.top		= GetRect().top;
	column_rect.left	= GetRect().left;
	column_rect.bottom	= GetRect().top + m_element_height;
	
	// If the vertical slider isn't active, let's draw column across the area
	column_rect.right	= GetUsableRect().right;

	// Draw row backgrounds
	if ( !m_report_columns.empty() )
	{
		UIRect row_rect;

		row_rect.left = GetRect().left;
		row_rect.right = GetUsableRect().right;
		row_rect.top = column_rect.bottom;
		row_rect.bottom = row_rect.top + m_element_height;

		int element_num = 0;			
		ReportListElementMap::iterator i; 
		for ( i = m_report_columns.begin()->elements.begin(); i != m_report_columns.begin()->elements.end(); ++i, ++element_num )
		{
			if ( element_num < m_lead_element )
			{
				continue;
			}

			if ( element_num % 2 )
			{
				gUIShell.DrawColorBox( row_rect, 0x55808080 );
			}
			else
			{
				gUIShell.DrawColorBox( row_rect, 0x35808080 );
			}

			row_rect.top += m_element_height;
			row_rect.bottom += m_element_height;
			if ( row_rect.bottom > GetRect().bottom )
			{
				break;
			}
		}
	}

	DrawSelection();

	// Draw the main column header
	UINormalizedRect uv_header_rect;
	uv_header_rect.right = (float)column_rect.Width() / gUIShell.GetRenderer().GetTextureWidth( m_tex_header_bg );
	DrawComponent( column_rect, m_tex_header_bg, 1.0f, uv_header_rect, true );

	ReportColumnVec::iterator i;
	for ( i = m_report_columns.begin(); i != m_report_columns.end(); ++i )
	{
		UIRect newRect;
		newRect.top = GetRect().top;
		newRect.left = GetRect().left + total_width;
		newRect.right = newRect.left + (*i).width;
		newRect.bottom = column_rect.bottom;

		if ( !i->fixedWidth && (i == (m_report_columns.end() - 1)) )
		{
			newRect.right = GetUsableRect().right;
		}

		if ( 0 < newRect.right )
		{
			DrawColumn( (*i).sName, newRect, (*i) );
		}
				
		total_width += (*i).width + SEPERATOR_PIXEL_WIDTH;		
	}	
}


void UIListReport::DrawColumn( gpwstring sColumnName, UIRect column_rect, UI_LISTREPORT_COLUMN & column )
{
	int draw_right = column_rect.right;
	bool bDrawSeperator = true;
	if ( column_rect.right > GetUsableRect().right )
	{
		draw_right = GetRect().right;
		bDrawSeperator = false;
	}

	int draw_left = column_rect.left;
	if ( column_rect.left < GetRect().left )
	{
		draw_left = GetRect().left;
	}
	if ( column_rect.right < GetRect().left )
	{
		bDrawSeperator = false;
	}

	UIRect draw_rect = column_rect;
	draw_rect.left = draw_left;
	draw_rect.right = draw_right;

	UINormalizedRect uv_header_rect;
	uv_header_rect.right = (float)draw_rect.Width() / gUIShell.GetRenderer().GetTextureWidth( m_tex_header_bg );
	DrawComponent( draw_rect, m_tex_header_bg, 1.0f, uv_header_rect, true );

	if ( !column.allowSort )
	{
//		gUIShell.DrawColorBox( draw_rect, 0x40000000 );
	}

	UIRect sep_rect;
	sep_rect.left	= column_rect.left;
	sep_rect.right	= column_rect.left + gUIShell.GetRenderer().GetTextureWidth( m_tex_separator );
	sep_rect.top	= column_rect.top;
	sep_rect.bottom = column_rect.bottom;
	
	if ( sep_rect.right < column_rect.right )
	{
		if ( column.sort_format == COLUMN_SORT_ASCENDING )
		{
			DrawComponent( sep_rect, m_tex_sort_icon_up );
		}
		else if ( column.sort_format == COLUMN_SORT_DESCENDING )
		{
			DrawComponent( sep_rect, m_tex_sort_icon_down );
		}
		else
		{
			DrawComponent( sep_rect, m_tex_separator );		
		}
	}
	
	if ( m_pFont ) 
	{
		// Draw the header

		int left = column_rect.left + sep_rect.Width() + 2;

		{
			int freeSpace = column_rect.Width() - sep_rect.Width();

			if ( column.pButton )
			{
				if ( m_bShowColumnButtons && 
					((column_rect.right - column.pButton->GetRect().Width() - 4) > (column_rect.left + sep_rect.Width())) &&
					(column_rect.right < GetRect().right) )
				{
					column.pButton->GetRect().left = column_rect.right - column.pButton->GetRect().Width() - 4;
					column.pButton->GetRect().right = column_rect.right - 4;
					int buttonHeight = column.pButton->GetRect().Height();
					column.pButton->GetRect().top = column_rect.top + (column_rect.Height() / 2) - (buttonHeight / 2);
					column.pButton->GetRect().bottom = column.pButton->GetRect().top + buttonHeight;
					column.pButton->SetVisible( true );

					freeSpace -= column.pButton->GetRect().Width();
				}
				else
				{
					column.pButton->SetVisible( false );
				}
			}

			int min_width	= 0;
			int min_height	= 0;
			m_pFont->CalculateStringSize( L"..", min_width, min_height );

			int width	= 0;
			int height	= 0;
			m_pFont->CalculateStringSize( sColumnName.c_str(), width, height ); 
			while ( width > freeSpace )
			{
				m_pFont->CalculateStringSize( sColumnName.c_str(), width, height ); 
				
				if ( freeSpace <= min_width ) 
				{
					sColumnName.clear();			
					break;
				}

				if ( sColumnName.find( L".." ) == gpwstring::npos ) 
				{
					sColumnName.erase( sColumnName.size()-1 );
					sColumnName.append( L".." );
				}
				else 
				{				
					if ( sColumnName.size() != 2 ) 
					{
						gpwstring sNew;
						sNew = sColumnName.substr( 0, sColumnName.size()-3 );
						sNew.append( sColumnName.substr( sColumnName.size()-2, sColumnName.size() ) );
						sColumnName = sNew;						
					}
				}			
			}		
			
			int top = column_rect.top + (column_rect.Height() / 2) - (height / 2);

			while ( (left + width) > GetRect().right )
			{
				if ( sColumnName.size() != 0 ) 
				{
					sColumnName.erase( sColumnName.size() - 1 );
					m_pFont->CalculateStringSize( sColumnName.c_str(), width, height ); 
				}
				else 
				{
					width = 0;
					break;
				}
			}
			
			m_pFont->Print( left, top, sColumnName.c_str(), column.dwTextColor, true );
		}

		// Draw the elements
		{
			ReportListElementMap::iterator i; 
			int element_num = 0;			
			for ( i = column.elements.begin(); i != column.elements.end(); ++i )
			{
				if ( element_num < m_lead_element )
				{
					element_num++;
					continue;
				}

				gpwstring sText = (*i).second.sText;

				int shiftLeft = 0;

				if ( i->second.icon != NO_ICON )
				{
					//shiftLeft += m_IconSize + 4;
				}

				int freeSpace = column_rect.Width() - shiftLeft;

				if ( column.pButton && column.pButton->GetVisible() )
				{
					freeSpace -= column.pButton->GetRect().Width();
				}

				/*
				if ( column.sort_format != COLUMN_SORT_NONE ) 
				{
					freeSpace -= (m_rect_sort_icon.right-m_rect_sort_icon.left);
				}
				*/

				int min_width	= 0;
				int min_height	= 0;
				m_pFont->CalculateStringSize( L"..", min_width, min_height );

				int width	= 0;
				int height	= 0;
				m_pFont->CalculateStringSize( sText.c_str(), width, height ); 
				while ( width > freeSpace )
				{			
					m_pFont->CalculateStringSize( sText.c_str(), width, height ); 
					
					if ( freeSpace <= min_width ) 
					{
						sText.clear();			
						break;
					}

					if ( sText.find( L".." ) == gpstring::npos ) 
					{
						sText.erase( sText.size()-1 );
						sText.append( L".." );
					}
					else 
					{				
						if ( sText.size() != 2 ) 
						{
							gpwstring sNew;
							sNew = sText.substr( 0, sText.size()-3 );
							sNew.append( sText.substr( sText.size()-2, sText.size() ) );
							sText = sNew;						
						}
					}			
				}
				
				int x = 0;
				int y = 0;
				
				do
				{
					if ( (x+width) > GetUsableRect().right ) 
					{
						if ( sText.size() != 0 ) 
						{
							sText.erase( sText.size()-1 );
							m_pFont->CalculateStringSize( sText.c_str(), width, height ); 
						}
						else 
						{
							break;
						}
					}
					x = left + shiftLeft;					
				
				} while ( (x+width) > GetUsableRect().right );

				y = column_rect.bottom + (m_element_height * element_num) - (m_element_height*(GetLeadElement()));								

				if ( y < GetUsableRect().top )
				{
					element_num++;
					continue;
				}

				if ( i->second.icon != NO_ICON )
				{
					IconMap::iterator findIcon = m_Icons.find( i->second.icon );
					if ( findIcon != m_Icons.end() )
					{
						UIRect icon_rect;
						icon_rect.left = column_rect.left;
						icon_rect.right = column_rect.left + m_IconSize;
						icon_rect.top = (y + (m_element_height / 2)) - (m_IconSize / 2);
						icon_rect.bottom = icon_rect.top + m_IconSize;
						DrawComponent( icon_rect, findIcon->second, 1.0f );
					}
				}

				if (( x >= GetRect().left ) && ( y <= (GetUsableRect().bottom-m_element_height) ))
				{
					m_pFont->Print( x, (y + (m_element_height / 2)) - (height / 2), sText.c_str(), (*i).second.dwTextColor, true );						
				}				
				
				element_num++;
			}						
		}		
	}
}


int UIListReport::GetTotalElementSize()
{
	if ( m_report_columns.size() == 0 )
	{
		return 0;
	}

	int total_height = m_rect_usable.top;
	ReportColumnVec::iterator iColumn = m_report_columns.begin();
	total_height += (*iColumn).elements.size() * m_element_height;	
	return total_height;
}


int	UIListReport::GetElementsSize()
{
	int max_bottom = 0;
	/*
	if ( GetHSliderActive() )
	{
		max_bottom = m_rect_usable.bottom;
	}
	else 
	*/
	{
		max_bottom = GetRect().bottom;
	}
	return max_bottom;
}


unsigned int UIListReport::GetNumElements()
{
	if ( m_report_columns.size() == 0 )
	{
		return 0;
	}
	
	ReportColumnVec::iterator iColumn = m_report_columns.begin();
	return (*iColumn).elements.size();	
}


bool UIListReport::HitDetectElement()
{
	if ( m_report_columns.size() == 0 )
	{
		return ( false );
	}

	int element_num = 0;			

	ReportColumnVec::iterator iColumn = m_report_columns.begin();
	for ( ReportListElementMap::iterator i = (*iColumn).elements.begin(); i != (*iColumn).elements.end(); ++i )
	{
		if ( element_num >= GetLeadElement() )
		{
			int y = GetUsableRect().top + (m_element_height * (element_num - GetLeadElement())) - GetVerticalPos();				

			if (( y >= GetUsableRect().top ) && ( GetMouseX() <= GetUsableRect().right ) && ( y <= GetUsableRect().bottom ))
			{
				if (( GetMouseY() >= y  ) && ( GetMouseY() <= (y+m_element_height))) 
				{
					m_selected_element = i->second.ID; //$$$ element_num;
					return ( true );
				}
			}
		}

		++element_num;
	}
	return ( false );
}


void UIListReport::DrawSelection()
{
	if ( m_report_columns.size() == 0 )
	{
		return;
	}
	int element_num = 0;			

	ReportColumnVec::iterator iColumn = m_report_columns.begin();
	for ( ReportListElementMap::iterator i = (*iColumn).elements.begin(); i != (*iColumn).elements.end(); ++i )
	{
		if ( element_num >= GetLeadElement() )
		{
			int y = GetUsableRect().top + (m_element_height * (element_num - GetLeadElement())) - GetVerticalPos();				

			if ( y >= GetUsableRect().top )
			{
				if ( i->second.ID == m_selected_element )
				{
					UIRect select_rect;
					select_rect.left = GetUsableRect().left;
					select_rect.right = GetUsableRect().right;
					select_rect.top = y;
					select_rect.bottom = y + m_element_height;
					
					if ( select_rect.bottom <= GetUsableRect().bottom )
					{
						gUIShell.DrawColorBox( select_rect, m_SelectionColor );
						//DrawComponent( select_rect, m_tex_selection, 0.6f );
					}
					return;
				}			
			}
		}

		++element_num;
	}		
}


void UIListReport::SetColumnHeaderText( int column, gpwstring text, unsigned int color )
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( column == index )
		{
			i->sName = text;
			i->dwTextColor = color;
			return;
		}

		++index;
	}
}

void UIListReport::SetAllowColumnSort( int column, bool allow )
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( column == index )
		{
			i->allowSort = allow;
			if ( !allow )
			{
				i->sort_format = COLUMN_SORT_NONE;
			}
			return;
		}

		++index;
	}
}


void UIListReport::SetSortColumn( int column, eColumnSort sort )
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( (column == index) && i->allowSort )
		{
			i->sort_format = sort;
			SortColumn( (*i) );
		}
		else
		{
			i->sort_format = COLUMN_SORT_NONE;
		}

		++index;
	}
}


int UIListReport::GetSortColumn()
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( i->sort_format != COLUMN_SORT_NONE )
		{
			return index;
		}

		++index;
	}

	return -1;
}


void UIListReport::SortColumn( int column )
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( (column == index) && i->allowSort )
		{
			SortColumn( (*i) );
			return;
		}

		++index;
	}
}

int UIListReport::GetColumnSize( int column )
{
	int index = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i ) 
	{
		if ( (column == index) && i->allowSort )
		{
			return i->width;
		}
		++index;
	}
	return 0;
}

void UIListReport::SortColumn( UI_LISTREPORT_COLUMN & column )
{
	switch ( column.sort_format )
	{
	case COLUMN_SORT_DESCENDING:
		{
			ReportListElementMap::iterator i;
			ReportListElementMap::iterator j;
			for ( i = column.elements.begin(); i != column.elements.end(); ++i )
			{
				for ( j = column.elements.begin(); j != column.elements.end(); ++j )
				{
					if ( (*i).second.sText.compare_no_case( (*j).second.sText ) > 0 )
					{
						UI_LISTREPORT_ELEMENT element;
						element		= (*i).second;
						(*i).second	= (*j).second;						
						(*j).second = element;

						ReportColumnVec::iterator o;
						for ( o = m_report_columns.begin(); o != m_report_columns.end(); ++o )
						{
							if ( (&(*o)) != (&column) )
							{
								ReportListElementMap::iterator iFound1 = (*o).elements.find( (*i).first );
								if ( iFound1 != (*o).elements.end() )
								{
									ReportListElementMap::iterator iFound2 = (*o).elements.find( (*j).first );
									{
										if ( iFound2 != (*o).elements.end() )
										{
											UI_LISTREPORT_ELEMENT found_element;
											found_element		= (*iFound1).second;
											(*iFound1).second	= (*iFound2).second;						
											(*iFound2).second = found_element;											
										}
									}									
								}
							}
						}
					}
				}
			}
		}
		break;
	case COLUMN_SORT_ASCENDING:
		{
			ReportListElementMap::iterator i;
			ReportListElementMap::iterator j;
			for ( i = column.elements.begin(); i != column.elements.end(); ++i )
			{
				for ( j = column.elements.begin(); j != column.elements.end(); ++j )
				{
					if ( (*i).second.sText.compare_no_case( (*j).second.sText ) < 0 )
					{
						UI_LISTREPORT_ELEMENT element;
						element		= (*i).second;
						(*i).second	= (*j).second;						
						(*j).second = element;

						ReportColumnVec::iterator o;
						for ( o = m_report_columns.begin(); o != m_report_columns.end(); ++o )
						{
							if ( (&(*o)) != (&column) )
							{
								ReportListElementMap::iterator iFound1 = (*o).elements.find( (*i).first );
								if ( iFound1 != (*o).elements.end() )
								{
									ReportListElementMap::iterator iFound2 = (*o).elements.find( (*j).first );
									{
										if ( iFound2 != (*o).elements.end() )
										{
											UI_LISTREPORT_ELEMENT found_element;
											found_element		= (*iFound1).second;
											(*iFound1).second	= (*iFound2).second;						
											(*iFound2).second = found_element;											
										}
									}									
								}
							}
						}
					}
				}
			}
		}
		break;
	}
}


void UIListReport::ResizeToCurrentResolution( int original_width, int original_height )
{
	UIWindow::ResizeToCurrentResolution( original_width, original_height );
	m_rect_usable.top		= GetRect().top + m_element_height;
	m_rect_usable.left		= GetRect().left;
	m_rect_usable.bottom	= GetRect().bottom;
	m_rect_usable.right		= GetRect().right;
}


#if !GP_RETAIL
void UIListReport::Save( FuelHandle hSave, bool bChild )
{
	UIWindow::Save( hSave, bChild );
	if ( GetParentWindow() && ( bChild == false ))
	{
		return;
	}	

	if ( hSave.IsValid() )
	{
		FuelHandle hWindow = hSave->GetChildBlock( GetName(), true );
		if ( hWindow.IsValid() )
		{
			hWindow->SetType( "listreport" );

			if ( m_pFont )
			{
				hWindow->Set( "font_type", m_pFont->GetTextureName() );
			}
			hWindow->Set( "font_size", m_font_size );
			hWindow->Set( "font_color", m_dwColor, FVP_HEX_INT );
		}
	}
}
#endif

void UIListReport::AddIcon( gpstring texture, int icon )
{
	IconMap::iterator findIcon = m_Icons.find( icon );
	if ( findIcon == m_Icons.end() )
	{
		unsigned int textureId = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
		m_Icons.insert( IconMap::value_type( icon, textureId ) );
	}
}

void UIListReport::SetElementIcon( int tag, int icon )
{
	for ( ReportColumnVec::iterator iCol = m_report_columns.begin(); iCol != m_report_columns.end(); ++iCol ) 
	{
		for ( ReportListElementMap::iterator i = (*iCol).elements.begin(); i != (*iCol).elements.end(); ++i )
		{
			if ( i->second.tag == tag )
			{
				i->second.icon = icon;
				return;
			}
		}
	}
}

void UIListReport::SetElementIcon( int column, int row, int icon )
{
	int iColumn = 0;
	for ( ReportColumnVec::iterator i = m_report_columns.begin(); i != m_report_columns.end(); ++i, ++iColumn )
	{
		if ( iColumn == column )
		{
			int iRow = 0;
			for ( ReportListElementMap::iterator j = (*i).elements.begin(); j != (*i).elements.end(); ++j, ++iRow )
			{
				if ( iRow == row )
				{
					j->second.icon = icon;
					break;
				}
			}
		}
	}
}

void UIListReport::ResizeSlider()
{
	GetVSlider()->SetRange( GetNumElements() - GetMaxActiveElements() );
	GetVSlider()->SetValue( GetLeadElement() );
	GetVSlider()->CalculateSlider();	
	GetVSlider()->SetVisible( GetVSlider()->GetMax() > 0 );
}
