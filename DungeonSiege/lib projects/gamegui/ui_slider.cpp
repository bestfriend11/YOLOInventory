/**************************************************************************

Filename		: ui_slider.cpp

Description		: Control for the slider

Creation Date	: 10/5/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_slider.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_listbox.h"
#include "ui_textureman.h"
#include "ui_listreport.h"
#include "ui_animation.h"
#include "ui_chatbox.h"
#include "ui_scroll_window.h"
#include "ui_textbox.h"
#include "ui_text.h"


/**************************************************************************

Function		: UISlider()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UISlider::UISlider( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_buttondown( false )
, m_button_alpha( 1.0f )
, m_button_spacer( 0 )
, m_button_size( 0 )
, m_fButtonSpacer( 0.0f )
, m_button_dynamic( true )
, m_bVertical ( true )
, m_bButtonVisible( true )
, m_button_mouse_offset( 0 )
, m_bListSlider( false )
, m_bTrackSlider( false )
, m_bRepeaterButtons( true )
, m_minvalue( 0 )
, m_maxvalue( 0 )
, m_stepvalue( 1 )
, m_value( 0 )
, m_bDrawTrackMark( false )
, m_trackMarkValue( 0 )
, m_pScrollDown( NULL )
, m_pScrollUp( NULL )
, m_bShowPopupValue( false )
, m_pPopupValue( NULL )
, m_button_texture_index_top( 0 )
, m_button_texture_index_mid( 0 )
, m_button_texture_index_bottom( 0 )
, m_track_left_texture_index( 0 )
, m_track_right_texture_index( 0 )
, m_track_center_texture_index( 0 )
, m_track_button_texture_index( 0 )
, m_track_mark_texture_index( 0 )
, m_dwDisableColor( 0x55ffffff )
{	
	SetType		( UI_TYPE_SLIDER	);
	SetInputType( TYPE_INPUT_ALL	);	

	fhWindow.Get( "dynamic_button", m_button_dynamic );
	
	gpstring sAxis;
	fhWindow.Get( "slide_axis", sAxis );	
	if ( sAxis.same_no_case( "horizontal") )
	{
		m_bVertical = false;
	}

	SetRange( fhWindow.GetInt( "max_value", 0 ), fhWindow.GetInt( "min_value", 0 ) );

	int value = 0;
	if( fhWindow.Get( "step_value", value ) )
	{
		m_stepvalue = value;
	}

	if( fhWindow.Get( "default_value", value ) )
	{
		SetValue( value );
	}
	
	fhWindow.Get( "repeater_buttons", m_bRepeaterButtons );

	if( fhWindow.GetBool( "has_scroll_buttons", false ) )
	{
		CreateSliderButtons();
	}

	FastFuelHandle hButton = fhWindow.GetChildNamed( "slider_button" );
	if ( hButton )
	{
		gpstring sTexture;
		if ( hButton.Get( "texture_top", sTexture ) )
		{
			m_button_texture_index_top = gUITextureManager.LoadTexture( sTexture, GetInterfaceParent(), false );
		}
		if ( hButton.Get( "texture_mid", sTexture ) )
		{
			m_button_texture_index_mid = gUITextureManager.LoadTexture( sTexture, GetInterfaceParent(), false );
		}
		if ( hButton.Get( "texture_bot", sTexture ) )
		{
			m_button_texture_index_bottom = gUITextureManager.LoadTexture( sTexture, GetInterfaceParent(), false );
		}		
	}

	bool bCommon = false;
	if( fhWindow.Get( "common_control", bCommon ) )
	{
		if( bCommon ) 
		{
			gpstring sTemplate;
			fhWindow.Get( "common_template", sTemplate );
			if( sTemplate.size() == 0 )
			{
				sTemplate = "slider";
			}
			CreateCommonCtrl( GetName(), sTemplate, GetParentWindow(), m_bVertical );		
		}
	}

	fhWindow.Get( "disable_color", m_dwDisableColor );

	fhWindow.Get( "show_popup_value", m_bShowPopupValue );
	if ( m_bShowPopupValue )
	{
		gpstring sFont;
		fhWindow.Get( "popup_value_font", sFont );

		m_pPopupValue = new UIText;
		m_pPopupValue->SetName( GetName() + "_popuptext" );
		m_pPopupValue->SetDrawOrder( GetDrawOrder() + 1 );
		m_pPopupValue->SetTopmost( true );
		m_pPopupValue->SetFont( sFont );
		m_pPopupValue->SetBackgroundFill( true );
		m_pPopupValue->SetBorder( true );
		m_pPopupValue->SetBackgroundColor( 0xFF000000 );
		m_pPopupValue->SetBorderColor( 0xFF808080 );
		m_pPopupValue->SetAlpha( 0.6f );
		m_pPopupValue->SetAutoSize( true );
		gUIShell.AddWindowToInterface( m_pPopupValue, parent_interface );
		m_pPopupValue->SetVisible( false );
	}
}
UISlider::UISlider()
: UIWindow()
, m_buttondown( false )
, m_button_alpha( 1.0f )
, m_button_spacer( 0 )
, m_button_size( 0 )
, m_fButtonSpacer( 0.0f )
, m_button_dynamic( true )
, m_bVertical ( true )
, m_bButtonVisible( true )
, m_button_mouse_offset( 0 )
, m_bListSlider( false )
, m_bTrackSlider( false )
, m_bRepeaterButtons( true )
, m_minvalue( 0 )
, m_maxvalue( 0 )
, m_stepvalue( 1 )
, m_value( 0 )
, m_bDrawTrackMark( false )
, m_trackMarkValue( 0 )
, m_pScrollDown( NULL )
, m_pScrollUp( NULL )
, m_bShowPopupValue( false )
, m_pPopupValue( NULL )
, m_button_texture_index_top( 0 )
, m_button_texture_index_mid( 0 )
, m_button_texture_index_bottom( 0 )
, m_track_left_texture_index( 0 )
, m_track_right_texture_index( 0 )
, m_track_center_texture_index( 0 )
, m_track_button_texture_index( 0 )
, m_track_mark_texture_index( 0 )
, m_dwDisableColor( 0x55ffffff )
{
	SetType		( UI_TYPE_SLIDER	);
	SetInputType( TYPE_INPUT_ALL	);	
}


/**************************************************************************

Function		: ~UISlider()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UISlider::~UISlider()
{
	if ( m_button_texture_index_top != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_texture_index_top );
	}
	if ( m_button_texture_index_mid != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_texture_index_mid );
	}
	if ( m_button_texture_index_bottom != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_texture_index_bottom );
	}
	if ( m_track_left_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_track_left_texture_index );
	}
	if ( m_track_right_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_track_right_texture_index );
	}
	if ( m_track_center_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_track_center_texture_index );
	}
	if ( m_track_button_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_track_button_texture_index );
	}
	if ( m_track_mark_texture_index != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_track_mark_texture_index );
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UISlider::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UISlider::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );

	if ( !IsEnabled() )
	{
		return false;
	}


	switch ( msg.GetCode() ) 
	{
		case MSG_LBUTTONDOWN:
		{
			UIWindow * pScroll = GetParentWindow();
			if ( pScroll )
			{
				pScroll->SetScrolling( true );
			}

			if( (GetMouseX() <= m_button_rect.right) &&
				(GetMouseX() >= m_button_rect.left) &&
				(GetMouseY() <= m_button_rect.bottom) &&
				(GetMouseY() >= m_button_rect.top) )
			{
				m_buttondown = true;

				if( m_bVertical )
				{
					m_button_mouse_offset = GetMouseY() - m_button_rect.top;
				}
				else
				{
					m_button_mouse_offset = GetMouseX() - m_button_rect.left;
				}
			}
			else
			{
				if( m_bVertical )
				{
					if( m_button_dynamic )
					{
						IncrementElement( ((GetMouseY() < m_button_rect.top) ? -1 : 1) * (m_button_rect.bottom - m_button_rect.top) );
					}
					else
					{
						IncrementElement( GetMouseY() - (m_button_rect.top + (m_button_rect.Height() / 2)) );
						m_button_mouse_offset = GetMouseY() - m_button_rect.top;
						m_buttondown = true;
					}
				}
				else
				{
					if( m_button_dynamic )
					{
						IncrementElement( ((GetMouseX() < m_button_rect.left) ? -1 : 1) * (m_button_rect.right - m_button_rect.left) );
					}
					else
					{
						IncrementElement( GetMouseX() - (m_button_rect.left + (m_button_rect.Width() / 2)) );
						m_button_mouse_offset = GetMouseX() - m_button_rect.left;
						m_buttondown = true;
					}
				}
			}

			if ( m_bShowPopupValue )
			{
				PositionPopupValueText();
				m_pPopupValue->SetVisible( true );
			}

			break;
		}

		case MSG_LBUTTONUP:
		{
			if( m_buttondown )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_BUTTONPRESS ), this );			
				CalculateSlider();
			}

			m_buttondown = false;

			if ( m_bShowPopupValue )
			{
				m_pPopupValue->SetVisible( false );
			}

			break;
		}

		case MSG_GLOBALLBUTTONUP:
		{
			if( m_buttondown )
			{
				CalculateSlider();
			}

			m_buttondown = false;

			if ( m_bShowPopupValue )
			{
				m_pPopupValue->SetVisible( false );
			}

			break;
		}

		case MSG_ROLLOVER:
		{
			if ( m_buttondown == true ) {
				gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
			}
			break;
		}

		case MSG_CREATED:
		{
			// Now, notify any listboxes that they have a child slider, if applicable
			UIWindow * pWindow = GetParentWindow();
			if ( pWindow && pWindow->GetType() == UI_TYPE_LISTBOX )
			{
				((UIListbox *)pWindow)->SetChildSlider( this );
			}
			else if ( pWindow && pWindow->GetType() == UI_TYPE_TEXTBOX && !pWindow->GetIsCommonControl() )
			{
				((UITextBox *)pWindow)->SetChildSlider( this );
			}
			break;
		}

		case MSG_SLIDERCALCULATE:
		{
			CalculateSlider();
			
			break;
		}

		case MSG_SLIDERINCREMENT:
		{
			UIWindow * pScroll = GetParentWindow();
			if ( pScroll )
			{
				pScroll->SetScrolling( true );
			}

			SetValue( m_value + 1 );
			CalculateSlider();

			break;
		}

		case MSG_SLIDERDECREMENT:
		{
			UIWindow * pScroll = GetParentWindow();
			if ( pScroll )
			{
				pScroll->SetScrolling( true );
			}

			SetValue( m_value - 1 );
			CalculateSlider();

			break;
		}
	}
	
	return true;
}


void UISlider::CalculateSlider()
{
	m_button_rect.left		= GetRect().left;
	m_button_rect.right		= GetRect().right;
	m_button_rect.top		= GetRect().top;
	m_button_rect.bottom	= GetRect().bottom;

	m_bButtonVisible = true;
	m_button_size = SLIDER_BUTTON_DEFAULT_SIZE;

	UIWindow * pScroll = GetParentWindow();
	if ( pScroll )
	{
		pScroll->SetLeadElement( m_value );
	}

	if ( m_button_dynamic )
	{
		if ( m_bListSlider )
		{
			if ( pScroll->GetNumElements() > 0 )
			{
				if ( m_bVertical )
				{
					m_button_size = (int)(((float)pScroll->GetMaxActiveElements() / pScroll->GetNumElements()) * (GetRect().bottom - GetRect().top));
				}
				else
				{
					m_button_size = (int)(((float)pScroll->GetMaxActiveElements() / pScroll->GetNumElements()) * (GetRect().right - GetRect().left));
				}
			}

			if ( m_button_size < SLIDER_BUTTON_MIN_SIZE )
			{
				m_button_size = SLIDER_BUTTON_MIN_SIZE;
			}
		}
		else
		{
			if ( m_maxvalue > m_minvalue )
			{
				if ( m_bVertical )
				{
					m_button_size = (GetRect().bottom - GetRect().top) / (m_maxvalue - m_minvalue + 1);
				}
				else
				{
					m_button_size = (GetRect().right - GetRect().left) / (m_maxvalue - m_minvalue + 1);
				}
			}

			if( m_button_size < SLIDER_BUTTON_MIN_SIZE )
			{
				m_button_size = SLIDER_BUTTON_MIN_SIZE;
			}
		}
	}

	if ( m_bVertical )
	{
		int range = m_maxvalue - m_minvalue;
		int value = m_value - m_minvalue;

		if ( range == 0 )
		{
			m_bButtonVisible = false;
			m_button_size = m_button_rect.bottom - m_button_rect.top;
			return;
		}

		m_button_rect.top = GetRect().top + (long)(((float)value / range) * (GetRect().bottom - GetRect().top - m_button_size));
		m_button_rect.bottom = m_button_rect.top + (m_button_size);
	}
	else
	{
		int range = m_maxvalue - m_minvalue;
		int value = m_value - m_minvalue;

		if ( range == 0 )
		{
			m_bButtonVisible = false;
			m_button_size = m_button_rect.right - m_button_rect.left;
			return;
		}

		int limitLeft = GetRect().left;
		int limitRight = GetRect().right;
		if ( m_bTrackSlider )
		{
			limitLeft += gUIShell.GetRenderer().GetTextureWidth( m_track_left_texture_index );
			limitRight -= gUIShell.GetRenderer().GetTextureWidth( m_track_right_texture_index );
		}

		m_button_rect.left = limitLeft + (long)(((float)value / range) * (limitRight - limitLeft - m_button_size));
		m_button_rect.right = m_button_rect.left + m_button_size;
	}
}


void UISlider::DrawTrackSlider()
{
	if ( GetRect().Width() < MIN_TRACK_SLIDER_WIDTH )
	{
		return;
	}

	gUIShell.GetRenderer().SetTextureStageState(	0,
													m_track_left_texture_index != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													D3DTADDRESS_WRAP,	// WRAP
													D3DTADDRESS_WRAP,
													D3DTEXF_POINT,		// LINEAR
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

	tVertex Verts[4];
	memset( Verts, 0, sizeof( tVertex ) * 4 );

	vector_3 default_color( 1.0f, 1.0f, 1.0f );
	DWORD color	= MAKEDWORDCOLOR( default_color );
	color		&= (0x00FFFFFF | ((DWORD)(255.0f * GetAlpha()) << 24));

	// left piece
	unsigned int leftTextureWidth = gUIShell.GetRenderer().GetTextureWidth( m_track_left_texture_index );
	Verts[0].x			= GetRect().left - 0.5f;
	Verts[0].y			= GetRect().top - 0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 1.0f;
	Verts[0].color		= color;

	Verts[1].x			= GetRect().left - 0.5f;
	Verts[1].y			= GetRect().bottom - 0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 0.0f;
	Verts[1].color		= color;

	Verts[2].x			= GetRect().left + leftTextureWidth - 0.5f;
	Verts[2].y			= GetRect().top - 0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= 1.0f;
	Verts[2].uv.v		= 1.0f;
	Verts[2].color		= color;

	Verts[3].x			= GetRect().left + leftTextureWidth - 0.5f;
	Verts[3].y			= GetRect().bottom - 0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= 1.0f;
	Verts[3].uv.v		= 0.0f;
	Verts[3].color		= color;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_track_left_texture_index, 1 );

	// right piece
	unsigned int rightTextureWidth = gUIShell.GetRenderer().GetTextureWidth( m_track_right_texture_index );
	Verts[0].x			= GetRect().right - rightTextureWidth - 0.5f;
	Verts[0].y			= GetRect().top - 0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 1.0f;
	Verts[0].color		= color;

	Verts[1].x			= GetRect().right - rightTextureWidth - 0.5f;
	Verts[1].y			= GetRect().bottom - 0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 0.0f;
	Verts[1].color		= color;

	Verts[2].x			= GetRect().right - 0.5f;
	Verts[2].y			= GetRect().top - 0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= 1.0f;
	Verts[2].uv.v		= 1.0f;
	Verts[2].color		= color;

	Verts[3].x			= GetRect().right - 0.5f;
	Verts[3].y			= GetRect().bottom - 0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= 1.0f;
	Verts[3].uv.v		= 0.0f;
	Verts[3].color		= color;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_track_right_texture_index, 1 );

	// center piece
	unsigned int textureWidth = gUIShell.GetRenderer().GetTextureWidth( m_track_center_texture_index );
	Verts[0].x			= GetRect().left + leftTextureWidth - 0.5f;
	Verts[0].y			= GetRect().top - 0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 1.0f;
	Verts[0].color		= color;

	Verts[1].x			= GetRect().left + leftTextureWidth - 0.5f;
	Verts[1].y			= GetRect().bottom - 0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 0.0f;
	Verts[1].color		= color;

	Verts[2].x			= GetRect().right - rightTextureWidth - 0.5f;
	Verts[2].y			= GetRect().top - 0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= (float)(GetRect().Width() - leftTextureWidth - rightTextureWidth) / textureWidth;
	Verts[2].uv.v		= 1.0f;
	Verts[2].color		= color;

	Verts[3].x			= GetRect().right - rightTextureWidth - 0.5f;
	Verts[3].y			= GetRect().bottom - 0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= Verts[2].uv.u;
	Verts[3].uv.v		= 0.0f;
	Verts[3].color		= color;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_track_center_texture_index, 1 );

	// marker
	if ( m_bDrawTrackMark )
	{
		int trackWidth = GetRect().Width() - leftTextureWidth - rightTextureWidth;
		int trackTextureWidth = gUIShell.GetRenderer().GetTextureWidth( m_track_mark_texture_index ) / 2;
		int markPos = (int)( ((float)(m_trackMarkValue - m_minvalue) / (m_maxvalue - m_minvalue)) * trackWidth );
		if ( (markPos - trackTextureWidth) < 0 )
		{
			markPos = trackTextureWidth;
		}
		else if ( (markPos + trackTextureWidth) > trackWidth )
		{
			markPos = trackWidth - trackTextureWidth;
		}
		markPos += GetRect().left + leftTextureWidth;

		Verts[0].x			= (markPos - trackTextureWidth) - 0.5f;
		Verts[0].y			= GetRect().top - 0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= 0.0f;
		Verts[0].uv.v		= 1.0f;
		Verts[0].color		= color;

		Verts[1].x			= (markPos - trackTextureWidth) - 0.5f;
		Verts[1].y			= GetRect().bottom - 0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= 0.0f;
		Verts[1].uv.v		= 0.0f;
		Verts[1].color		= color;

		Verts[2].x			= (markPos + trackTextureWidth) - 0.5f;
		Verts[2].y			= GetRect().top - 0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= 1.0f;
		Verts[2].uv.v		= 1.0f;
		Verts[2].color		= color;

		Verts[3].x			= (markPos + trackTextureWidth) - 0.5f;
		Verts[3].y			= GetRect().bottom - 0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= 1.0f;
		Verts[3].uv.v		= 0.0f;
		Verts[3].color		= color;

		gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_track_mark_texture_index, 1 );
	}

	// button
	Verts[0].x			= m_button_rect.left - 0.5f;
	Verts[0].y			= m_button_rect.top - 0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 1.0f;
	Verts[0].color		= color;

	Verts[1].x			= m_button_rect.left - 0.5f;
	Verts[1].y			= m_button_rect.bottom - 0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 0.0f;
	Verts[1].color		= color;

	Verts[2].x			= m_button_rect.right - 0.5f;
	Verts[2].y			= m_button_rect.top - 0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= 1.0f;
	Verts[2].uv.v		= 1.0f;
	Verts[2].color		= color;

	Verts[3].x			= m_button_rect.right - 0.5f;
	Verts[3].y			= m_button_rect.bottom - 0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= 1.0f;
	Verts[3].uv.v		= 0.0f;
	Verts[3].color		= color;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_track_button_texture_index, 1 );
}


/**************************************************************************

Function		: DrawSelectionTexture()

Purpose			: Draw a texture for the slider button

Returns			: void

**************************************************************************/
void UISlider::DrawSliderButton()
{
	// Draw multi-part button
	if ( GetVisible() && m_bButtonVisible )
	{
		gUIShell.GetRenderer().SetTextureStageState(	0,
														m_button_texture_index_top != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_WRAP,	// WRAP
														D3DTADDRESS_WRAP,
														D3DTEXF_POINT,		// LINEAR
														D3DTEXF_POINT,
														D3DTEXF_POINT,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														true );

		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );

		vector_3 default_color( 1.0f, 1.0f, 1.0f );
		DWORD color	= MAKEDWORDCOLOR( default_color );
		DWORD alpha = (DWORD)(255.0f * m_button_alpha * (IsEnabled() ? 1.0f : 0.6f));
		color		&= 0x00FFFFFF | (alpha << 24);

		if ( m_bVertical )
		{
			float PartSize = (float)gUIShell.GetRenderer().GetTextureHeight( m_button_texture_index_top ); //(float)SLIDER_BUTTON_MIN_SIZE * 0.5f;

			// top piece
			Verts[0].x			= m_button_rect.left - 0.5f;
			Verts[0].y			= m_button_rect.top - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 1.0f;
			Verts[0].color		= color;

			Verts[1].x			= m_button_rect.left - 0.5f;
			Verts[1].y			= m_button_rect.top + PartSize - 0.5f;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 0.0f;
			Verts[1].uv.v		= 0.0f;
			Verts[1].color		= color;

			Verts[2].x			= m_button_rect.right - 0.5f;
			Verts[2].y			= m_button_rect.top - 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 1.0f;
			Verts[2].uv.v		= 1.0f;
			Verts[2].color		= color;

			Verts[3].x			= m_button_rect.right - 0.5f;
			Verts[3].y			= m_button_rect.top + PartSize - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 0.0f;
			Verts[3].color		= color;

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_top, 1 );

			// middle piece
			if( m_button_rect.Height() > (PartSize * 2) )
			{
				float vBottom = ((float)m_button_rect.Height() - PartSize * 2) / gUIShell.GetRenderer().GetTextureHeight( m_button_texture_index_mid );

				Verts[0].x			= m_button_rect.left - 0.5f;
				Verts[0].y			= m_button_rect.top + PartSize - 0.5f;
				Verts[0].z			= 0.0f;
				Verts[0].rhw		= 1.0f;
				Verts[0].uv.u		= 0.0f;
				Verts[0].uv.v		= vBottom;
				Verts[0].color		= color;

				Verts[1].x			= m_button_rect.left - 0.5f;
				Verts[1].y			= m_button_rect.bottom - PartSize - 0.5f;
				Verts[1].z			= 0.0f;
				Verts[1].rhw		= 1.0f;
				Verts[1].uv.u		= 0.0f;
				Verts[1].uv.v		= 0.0f;
				Verts[1].color		= color;

				Verts[2].x			= m_button_rect.right - 0.5f;
				Verts[2].y			= m_button_rect.top + PartSize - 0.5f;
				Verts[2].z			= 0.0f;
				Verts[2].rhw		= 1.0f;
				Verts[2].uv.u		= 1.0f;
				Verts[2].uv.v		= vBottom;
				Verts[2].color		= color;

				Verts[3].x			= m_button_rect.right - 0.5f;
				Verts[3].y			= m_button_rect.bottom - PartSize - 0.5f;
				Verts[3].z			= 0.0f;
				Verts[3].rhw		= 1.0f;
				Verts[3].uv.u		= 1.0f;
				Verts[3].uv.v		= 0.0f;
				Verts[3].color		= color;

				gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_mid, 1 );
			}

			// bottom piece
			Verts[0].x			= m_button_rect.left - 0.5f;
			Verts[0].y			= m_button_rect.bottom - PartSize - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 1.0f;
			Verts[0].color		= color;

			Verts[1].x			= m_button_rect.left - 0.5f;
			Verts[1].y			= m_button_rect.bottom - 0.5f;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 0.0f;
			Verts[1].uv.v		= 0.0f;
			Verts[1].color		= color;

			Verts[2].x			= m_button_rect.right - 0.5f;
			Verts[2].y			= m_button_rect.bottom - PartSize - 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 1.0f;
			Verts[2].uv.v		= 1.0f;
			Verts[2].color		= color;

			Verts[3].x			= m_button_rect.right - 0.5f;
			Verts[3].y			= m_button_rect.bottom - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 0.0f;
			Verts[3].color		= color;

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_bottom, 1 );
		}
		else
		{
			float PartSize = (float)gUIShell.GetRenderer().GetTextureWidth( m_button_texture_index_top ); //(float)SLIDER_BUTTON_MIN_SIZE * 0.5f;

			// top piece
			Verts[0].x			= m_button_rect.left - 0.5f;
			Verts[0].y			= m_button_rect.top - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 1.0f;
			Verts[0].color		= color;

			Verts[1].x			= m_button_rect.left - 0.5f;
			Verts[1].y			= m_button_rect.bottom - 0.5f;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 1.0f;
			Verts[1].uv.v		= 1.0f;
			Verts[1].color		= color;

			Verts[2].x			= m_button_rect.left + PartSize - 0.5f;
			Verts[2].y			= m_button_rect.top - 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 0.0f;
			Verts[2].uv.v		= 0.0f;
			Verts[2].color		= color;

			Verts[3].x			= m_button_rect.left + PartSize - 0.5f;
			Verts[3].y			= m_button_rect.bottom - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 0.0f;
			Verts[3].color		= color;

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_top, 1 );

			// middle piece
			if( m_button_rect.right - m_button_rect.left > SLIDER_BUTTON_MIN_SIZE )
			{
				Verts[0].x			= m_button_rect.left + PartSize - 0.5f;
				Verts[0].y			= m_button_rect.top - 0.5f;
				Verts[0].z			= 0.0f;
				Verts[0].rhw		= 1.0f;
				Verts[0].uv.u		= 0.0f;
				Verts[0].uv.v		= 1.0f;
				Verts[0].color		= color;

				Verts[1].x			= m_button_rect.left + PartSize - 0.5f;
				Verts[1].y			= m_button_rect.bottom - 0.5f;
				Verts[1].z			= 0.0f;
				Verts[1].rhw		= 1.0f;
				Verts[1].uv.u		= 1.0f;
				Verts[1].uv.v		= 1.0f;
				Verts[1].color		= color;

				Verts[2].x			= m_button_rect.right - PartSize - 0.5f;
				Verts[2].y			= m_button_rect.top - 0.5f;
				Verts[2].z			= 0.0f;
				Verts[2].rhw		= 1.0f;
				Verts[2].uv.u		= 0.0f;
				Verts[2].uv.v		= 0.0f;
				Verts[2].color		= color;

				Verts[3].x			= m_button_rect.right - PartSize - 0.5f;
				Verts[3].y			= m_button_rect.bottom - 0.5f;
				Verts[3].z			= 0.0f;
				Verts[3].rhw		= 1.0f;
				Verts[3].uv.u		= 1.0f;
				Verts[3].uv.v		= 0.0f;
				Verts[3].color		= color;

				gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_mid, 1 );
			}

			// bottom piece
			Verts[0].x			= m_button_rect.right - PartSize - 0.5f;
			Verts[0].y			= m_button_rect.top - 0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= 0.0f;
			Verts[0].uv.v		= 1.0f;
			Verts[0].color		= color;

			Verts[1].x			= m_button_rect.right - PartSize - 0.5f;
			Verts[1].y			= m_button_rect.bottom - 0.5f;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= 1.0f;
			Verts[1].uv.v		= 1.0f;
			Verts[1].color		= color;

			Verts[2].x			= m_button_rect.right - 0.5f;
			Verts[2].y			= m_button_rect.top- 0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= 0.0f;
			Verts[2].uv.v		= 0.0f;
			Verts[2].color		= color;

			Verts[3].x			= m_button_rect.right - 0.5f;
			Verts[3].y			= m_button_rect.bottom - 0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= 1.0f;
			Verts[3].uv.v		= 0.0f;
			Verts[3].color		= color;

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_button_texture_index_bottom, 1 );
		}
	}
}


/**************************************************************************

Function		: Draw()

Purpose			: Render the listbox and its elements appropriately

Returns			: void

**************************************************************************/
void UISlider::Draw()
{
	if ( GetVisible() )
	{
		/*
		if ( !IsEnabled() && GetVisible() && HasTexture() )
		{
			UIWindow::DrawComponent( GetRect(), GetTextureIndex(), GetAlpha(), GetUVRect(), GetTiledTexture(), m_dwDisableColor );
		}
		else if ( !IsEnabled() && GetVisible() )
		{
			UIWindow::DrawColorBox( GetRect(), m_dwDisableColor, false );
		}
		else
		{
			UIWindow::Draw();
		}
		*/

		UIWindow::Draw();

		if ( m_bTrackSlider )
		{
			float oldAlpha = GetAlpha();
			if ( !IsEnabled() )
			{
				SetAlpha( 0.6f );
			}

			DrawTrackSlider();

			SetAlpha( oldAlpha );
		}
		else
		{
			DrawSliderButton();
		}
	}
}


void UISlider::SetVisible( bool bVisible )
{
	UIWindow::SetVisible( bVisible );
}


/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the slider
				  button rectangle accordingly

Returns			: void

**************************************************************************/
void UISlider::SetMousePos( unsigned int x, unsigned int y )
{	
	if ( !gUIShell.GetLButtonDown() )
	{
		m_buttondown = false;
	}

	if ( m_buttondown )
	{		
		if ( m_bVertical ) 
		{
			if ( y != (unsigned int)(m_button_rect.top + m_button_mouse_offset) )
			{
				IncrementElement( y - (m_button_rect.top + m_button_mouse_offset) );
			}
		}
		else
		{
			if ( x != (unsigned int)(m_button_rect.left + m_button_mouse_offset) ) 
			{
				IncrementElement( x - (m_button_rect.left + m_button_mouse_offset) );
			}
		}

		if ( m_bShowPopupValue )
		{
			PositionPopupValueText();
		}
	}

	UIWindow::SetMousePos( x, y );
}


void UISlider::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{
	UIWindow::SetRect( left, right, top, bottom, bAnimation );

	if ( !m_bTrackSlider && (GetTextureIndex() != 0) )
	{
		if ( m_bVertical )
		{
			SetUVRect( 0.0, 1.0, 0.0, (double)GetRect().Height() / gUIShell.GetRenderer().GetTextureHeight( GetTextureIndex() ) );
		}
		else
		{
			SetUVRect( 0.0, (double)GetRect().Width() / gUIShell.GetRenderer().GetTextureWidth( GetTextureIndex() ), 0.0, 1.0 );
		}
	}

	CalculateSlider();
}


/**************************************************************************

Function		: IncrementElement()

Purpose			: Moves the list down because it is moving deeper into 
				  the list.

Returns			: void

**************************************************************************/
void UISlider::IncrementElement( int scroll_amount )
{
	if ( m_bVertical )
	{
		if ( m_button_rect.top + scroll_amount > GetRect().bottom - m_button_size )
		{
			m_button_rect.top = GetRect().bottom - m_button_size;
			m_button_rect.bottom = GetRect().bottom;
		}
		else if ( m_button_rect.top + scroll_amount < GetRect().top )
		{
			m_button_rect.top = GetRect().top;
			m_button_rect.bottom = GetRect().top + m_button_size;
		}
		else
		{
			m_button_rect.top += scroll_amount;
			m_button_rect.bottom = m_button_rect.top + m_button_size;
		}

		int range = m_maxvalue - m_minvalue;
		int value = (int)(0.5f + (float)range * ((float)(m_button_rect.top - GetRect().top) / (float)(GetRect().bottom - GetRect().top - m_button_size)));

		SetValue( m_minvalue + value );
	}
	else
	{
		if ( m_button_rect.left + scroll_amount > GetRect().right - m_button_size )
		{
			m_button_rect.left = GetRect().right - m_button_size;
			m_button_rect.right = GetRect().right;
		}
		else if ( m_button_rect.left + scroll_amount < GetRect().left )
		{
			m_button_rect.left = GetRect().left;
			m_button_rect.right = GetRect().left + m_button_size;
		}
		else
		{
			m_button_rect.left += scroll_amount;
			m_button_rect.right = m_button_rect.left + m_button_size;
		}

		int limitLeft = GetRect().left;
		int limitRight = GetRect().right;
		if ( m_bTrackSlider )
		{
			limitLeft += gUIShell.GetRenderer().GetTextureWidth( m_track_left_texture_index );
			limitRight -= gUIShell.GetRenderer().GetTextureWidth( m_track_right_texture_index );
		}
		
		int range = m_maxvalue - m_minvalue;
		int value = (int)ceil(((float)range * ((float)(m_button_rect.left - limitLeft) / (float)(limitRight - limitLeft - m_button_size))));

		SetValue( m_minvalue + value );
	}

	UIWindow * pScroll = GetParentWindow();
	if ( pScroll )
	{
		pScroll->SetLeadElement( m_value );
	}
}


void UISlider::CreateSliderButtons()
{
	if ( m_pScrollUp || m_pScrollDown )
	{
		return;
	}

	if ( m_bVertical )
	{
		m_pScrollUp = new UIButton;
		m_pScrollUp->CreateCommonCtrl( gpstring( GetName() ).append( "scroll_up" ), "button_up", this, SLIDER_DECREMENT );
		m_pScrollUp->SetDrawOrder( GetDrawOrder() + 1 );
		m_pScrollUp->SetConsumable( IsConsumable() );
		m_pScrollUp->SetIsRepeater( m_bRepeaterButtons );
		gUIShell.AddWindowToInterface( m_pScrollUp, GetInterfaceParent() );
		AddChild( m_pScrollUp );

		m_pScrollDown = new UIButton;
		m_pScrollDown->CreateCommonCtrl( gpstring( GetName() ).append( "scroll_down" ), "button_down", this, SLIDER_INCREMENT );
		m_pScrollDown->SetDrawOrder( GetDrawOrder() + 1 );
		m_pScrollDown->SetConsumable( IsConsumable() );
		m_pScrollDown->SetIsRepeater( m_bRepeaterButtons );
		gUIShell.AddWindowToInterface( m_pScrollDown, GetInterfaceParent() );
		AddChild( m_pScrollDown );
	}
	else
	{
		m_pScrollUp = new UIButton;
		m_pScrollUp->CreateCommonCtrl( gpstring( GetName() ).append( "scroll_left" ), "button_left", this, SLIDER_DECREMENT );
		m_pScrollUp->SetDrawOrder( GetDrawOrder() + 1 );
		m_pScrollUp->SetConsumable( IsConsumable() );
		m_pScrollUp->SetIsRepeater( m_bRepeaterButtons );
		gUIShell.AddWindowToInterface( m_pScrollUp, GetInterfaceParent() );
		AddChild( m_pScrollUp );

		m_pScrollDown = new UIButton;
		m_pScrollDown->CreateCommonCtrl( gpstring( GetName() ).append( "scroll_right" ), "button_right", this, SLIDER_INCREMENT );
		m_pScrollDown->SetDrawOrder( GetDrawOrder() + 1 );
		m_pScrollDown->SetConsumable( IsConsumable() );
		m_pScrollDown->SetIsRepeater( m_bRepeaterButtons );
		gUIShell.AddWindowToInterface( m_pScrollDown, GetInterfaceParent() );
		AddChild( m_pScrollDown );
	}

	ResizeSliderButtons();
}


void UISlider::ResizeSliderButtons()
{
	int top		= GetRect().top;
	int bottom	= GetRect().bottom;
	int left	= GetRect().left;
	int right	= GetRect().right;

	if ( m_bVertical )
	{
		if ( m_pScrollUp )
		{
			m_pScrollUp->SetRect( left, right, top, top + m_pScrollUp->GetRect().Height() );
			top += m_pScrollUp->GetRect().Height();
		}

		if ( m_pScrollDown )
		{
			m_pScrollDown->SetRect( left, right, bottom - m_pScrollUp->GetRect().Height(), bottom );
			bottom -= m_pScrollDown->GetRect().Height();
		}
	}
	else
	{
		if ( m_pScrollUp )
		{
			m_pScrollUp->SetRect( left, left + m_pScrollUp->GetRect().Width(), top, bottom );
			left += m_pScrollUp->GetRect().Width();
		}

		if ( m_pScrollDown )
		{
			m_pScrollDown->SetRect( right - m_pScrollUp->GetRect().Width(), right, top, bottom );
			right -= m_pScrollDown->GetRect().Width();
		}
	}

	SetRect( left, right, top, bottom );	
}


void UISlider::CreateCommonCtrl( gpstring sName, gpstring sTemplate, UIWindow *pParent, bool bVertical )
{
	if ( pParent && ( pParent->GetType() == UI_TYPE_LISTBOX || pParent->GetType() == UI_TYPE_LISTREPORT || pParent->GetType() == UI_TYPE_TEXTBOX || pParent->GetType() == UI_TYPE_CHATBOX ) )
	{
		m_bListSlider = true;
	}

	if ( GetTextureIndex() != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
		SetTextureIndex( 0 );
	}

	if ( sTemplate.same_no_case( "track" ) )
	{
		m_bTrackSlider = true;
		m_bVertical = false;
		m_button_dynamic = false;

		m_track_left_texture_index = gUIShell.GetCommonArt( "slider_track_left" );
		m_track_right_texture_index = gUIShell.GetCommonArt( "slider_track_right" );
		m_track_center_texture_index = gUIShell.GetCommonArt( "slider_track_center" );
		m_track_button_texture_index = gUIShell.GetCommonArt( "slider_track_button" );
		m_track_mark_texture_index = gUIShell.GetCommonArt( "slider_track_mark" );
	}
	else
	{	
		m_bTrackSlider = false;
		m_bVertical = bVertical;

		m_button_texture_index_top = gUIShell.GetCommonArt( "slider_button_up_top" );
		m_button_texture_index_mid = gUIShell.GetCommonArt( "slider_button_up_mid" );
		m_button_texture_index_bottom = gUIShell.GetCommonArt( "slider_button_up_bottom" );

		if( bVertical )
		{
			SetTextureIndex( gUIShell.GetCommonArt( "slider_background" ) );		
		}
		else
		{
			SetTextureIndex( gUIShell.GetCommonArt( "slider_background_horizontal" ) );		
		}

		SetHasTexture( true );
		SetTiledTexture( true );
	}
	
	SetParentWindow( pParent );
	SetName( sName );
	SetRect( GetRect().left, GetRect().right, GetRect().top, GetRect().bottom );

	CalculateSlider();
}


void UISlider::CalculateRelativeSliderPos()
{
	UIWindow * pScroll = (UIWindow *)GetParentWindow();
	if ( pScroll )
	{
		if ( !m_bVertical )
		{
			int slider_space	= (GetRect().right - GetRect().left ) - (m_button_rect.right - m_button_rect.left);
			int list_space		= pScroll->GetTotalColumnSize() - (pScroll->GetColumnSize() - pScroll->GetRect().left);			
			int movement = (int)ceil(float(list_space)/float(slider_space));
			pScroll->SetHorizontalPos( (m_button_rect.left-GetRect().left) * movement );
		}
	}
}


void UISlider::PositionPopupValueText()
{
	if ( m_bShowPopupValue )
	{
		if ( !m_bVertical )
		{
			int leftPos = (m_button_rect.left + m_button_rect.Width() / 2) - m_pPopupValue->GetRect().Width() / 2;
			m_pPopupValue->SetRect( leftPos, leftPos + m_pPopupValue->GetRect().Width(),
									m_button_rect.top - m_pPopupValue->GetRect().Height() - 4, m_button_rect.top - 4 );
		}
	}
}


#if !GP_RETAIL
void UISlider::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "slider" );
		}
	}
}
#endif


void UISlider::SetMin( int min )
{
	m_minvalue = min;

	if( m_minvalue > m_maxvalue )
	{
		m_maxvalue = m_minvalue;
	}

	if( m_value < m_minvalue )
	{
		m_value = m_minvalue;
	}
	else if( m_value > m_maxvalue )
	{
		m_value = m_maxvalue;
	}
}

void UISlider::SetMax( int max )
{
	m_maxvalue = max;
	
	if( m_maxvalue < m_minvalue )
	{
		m_minvalue = m_maxvalue;
	}

	if( m_value < m_minvalue )
	{
		m_value = m_minvalue;
	}
	else if( m_value > m_maxvalue )
	{
		m_value = m_maxvalue;
	}
}

void UISlider::SetRange( int max, int min )
{
	if( min > max )
	{
		max = min;
	}

	m_minvalue = min;
	m_maxvalue = max;

	if( m_value < m_minvalue )
	{
		SetValue( m_minvalue );
	}
	else if( m_value > m_maxvalue )
	{
		SetValue( m_maxvalue );
	}
}

void UISlider::SetValue( int value )
{
	int oldValue = m_value;
	m_value = value;

	if( m_value < m_minvalue )
	{
		m_value = m_minvalue;
	}
	else if( m_value > m_maxvalue )
	{
		m_value = m_maxvalue;
	}

	if ( oldValue != m_value )
	{
		if ( m_bShowPopupValue )
		{
			m_pPopupValue->SetText( gpwstringf( L"%d", m_value ) );
		}

		gUIMessenger.SendUIMessage( UIMessage( MSG_CHANGE ), this );
	}

	CalculateSlider();
}
