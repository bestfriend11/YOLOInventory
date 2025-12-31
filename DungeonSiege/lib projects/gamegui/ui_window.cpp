/**************************************************************************

Filename		: ui_window.cpp

Description		: Contains the standard functions of the windows
				  used in the GameGUI

Creation Date	: 9/18/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "fuel.h"
#include "gps_manager.h"
#include "rapiowner.h"
#include "rapimouse.h"
#include "stdhelp.h"
#include "stringtool.h"
#include "appmodule.h"
#include "ui_animation.h"
#include "ui_messenger.h"
#include "ui_textureman.h"
#include "ui_window.h"
#include "ui_shell.h"
#include "ui_textbox.h"
#include "LocHelp.h"
#include "SkritEngine.h"


UIWindow::UIActionCb UIWindow::ms_uiActionCb;


/**************************************************************************

Function		: UIWindow()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIWindow::UIWindow( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: m_type			( UI_TYPE_WINDOW )
, m_secondary_type	( TYPE_INPUT_ALL )
, m_draw_order		( 0 )
, m_pullout_offset	( 0 )
, m_bTexture		( false )
, m_bTiled			( false )
, m_bDrag			( false )
, m_drag_time		( 0.0f	)
, m_drag_grid_x		( 0 )
, m_drag_grid_y		( 0 )
, m_drag_time_elapsed( 0.0f )
, m_mouseX			( 0 )
, m_mouseY			( 0 )
, m_bDragAllowed	( false )
, m_marquee_color	( vector_3( 0.0f, 1.0f, 0.0f ) )
, m_bNormalized		( false )
, m_bRollover		( false )
, m_bHasBorder		( false )
, m_bHasHelpText	( false )
, m_bHasToolTip		( false )
, m_bHelpTextVisible( false )
, m_bToolTipVisible ( false )
, m_ToolTipDelay	( 0.5f )
, m_MouseHoldTime	( 0.0 )
, m_LastRealTime	( 0.0 )
, m_ToolTipX		( -1 )
, m_ToolTipY		( -1 )
, m_scale			( 1.0f )
, m_bScaled			( false )
, m_index			( 0 )
, m_texture_index	( 0 )
, m_tag				( 0 )
, m_bMarkedForDeletion( false )
, m_parentWindow	( 0 )
, m_bCommon			( false )
, m_bCanHitDetect	( true )
, m_bConsumable		( false )
, m_bEnabled		( true )
, m_BorderColor		( 0x00FFFFFF )
, m_BgColor			( 0x00FFFFFF )
, m_bBgFill			( false )
, m_bPassThrough	( false )
, m_pHelpBox		( 0 )
, m_bStretchX		( false )
, m_bStretchY		( false )
, m_bCenterX		( false )
, m_bCenterY		( false )
, m_bNormalizeResize( false )
, m_alpha			( 1.0f  )
, m_bTopmost		( false )
, m_borderPadding	( 0		)
, m_bHidden			( false )
, m_bHasFocus		( false )
{
	// Set standard UI window information
	SetName( fhWindow.GetName() );

	::FromGasString( fhWindow.GetType(), m_type );

	SetInterfaceParent( parent_interface );
	SetParentWindow( parent );
	if ( parent )
	{
		parent->AddChild( this );
	}
	SetAlpha( 1.0 );
	SetVisible( true );
	SetUVRect( 0, 1, 0, 1 );
	SetNormalizedRect( 0.0f, 0.0f, 0.0f, 0.0f );

	// Construct all the messages
	FastFuelHandle fhMessages = fhWindow.GetChildNamed( "messages" );
	if ( fhMessages ) 
	{
		FastFuelFindHandle hFind( fhMessages );
		if ( hFind.FindFirstKeyAndValue() ) 
		{
			gpstring message_name;
			gpstring actions;
			while ( hFind.GetNextKeyAndValue( message_name, actions ) ) 
			{
				UI_EVENT ue;
				ue.msg_code = ConvertStringToMessageCode( message_name );
				unsigned int numStrings = stringtool::GetNumDelimitedStrings( actions, '&' );
				for ( unsigned int j = 0; j < numStrings; ++j ) 
				{
					gpstring action;
					stringtool::GetDelimitedValue( actions, '&', j, action );
					EVENT_ACTION ea = ConvertStringToAction( action );
					if ( ea.action == ACTION_LOADTEXTURE ) 
					{
						LoadTexture( ea.parameter );
					}
					else if ( ea.action == ACTION_CALL )
					{
						// move params to the front (if any)
						size_t found = ea.parameter.find( '?' );
						if ( found != ea.parameter.npos )
						{
							gpstring subparams = ea.parameter.substr( found );
							ea.parameter.erase( found );
							ea.parameter = subparams + "," + ea.parameter;
						}
						else
						{
							ea.parameter.insert( 0, "," );
						}

						// insert full gas path
						ea.parameter.insert( 0, fhWindow.GetParent().GetAddress() + ":skrit" );
					}
					ue.event_actions.push_back( ea );
				}
				m_events.push_back( ue );
			}
		}
	}

	// Set specific UI window information
	gpstring sRect;
	gpstring sUVCoords;
	gpstring sTexture;
	gpstring sWrapMode;
	float fAlpha = 0;
	if ( fhWindow.Get( "rect", sRect ) ) 
	{
		if ( fhWindow.Get( "normalized", m_bNormalized ) )
		{
			if ( m_bNormalized ) 
			{
				ProcessAction( ACTION_SETNORMALIZEDRECT, sRect );
				SetPulloutOffset( m_rect.left );
			}
			else 
			{
				ProcessAction( ACTION_SETRECT, sRect );
				SetPulloutOffset( m_rect.left );
			}
		}
		else 
		{
			ProcessAction( ACTION_SETRECT, sRect );
			SetPulloutOffset( m_rect.left );
		}
	}
	if ( fhWindow.Get( "uvcoords", sUVCoords ) ) 
	{
		ProcessAction( ACTION_SETUVCOORDS, sUVCoords );
	}
	if ( fhWindow.Get( "alpha", fAlpha ) )
	{
		SetAlpha( fAlpha );
	}
	if ( fhWindow.Get( "texture", sTexture ) ) 
	{
		if ( !sTexture.same_no_case( "none" ) && !sTexture.empty() ) 
		{
			LoadTexture( sTexture );
			SetHasTexture( true );
		}
	}
	if ( fhWindow.Get( "wrap_mode", sWrapMode ) ) 
	{
		if ( sWrapMode.same_no_case( "tiled" )) 
		{
			m_bTiled = true;
		}
		else if ( sWrapMode.same_no_case( "clamp" ) ) 
		{
			m_bTiled = false;
		}
	}

	DWORD dwBgColor = 0;
	if ( fhWindow.Get( "background_color", dwBgColor ) )
	{
		m_BgColor = dwBgColor;
	}

	fhWindow.Get( "background_fill", m_bBgFill );

	fhWindow.Get( "visible", m_bVisible );
	fhWindow.Get( "enabled", m_bEnabled );
	fhWindow.Get( "has_border", m_bHasBorder );
	fhWindow.Get( "border_color", m_BorderColor );
	fhWindow.Get( "border_padding", m_borderPadding );

	if( fhWindow.Get( "help_text", m_sHelpText ) )
	{
		if ( LocMgr::DoesSingletonExist() )
		{
			gLocMgr.TranslateUiToolTip( m_sHelpText, GetInterfaceParent().c_str(), GetName().c_str() );
		}
		fhWindow.Get( "help_text_box", m_sHelpTextBox );
		m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );

		if ( (m_sHelpText.size() > 0) && (m_sHelpTextBox.size() > 0) )
		{
			fhWindow.Get( "help_tool_tip", m_bHasToolTip );
			m_bHasHelpText = !m_bHasToolTip;
		}
	}

	fhWindow.Get( "rollover_help", m_sRolloverKey );	

	// Get any anchor information
	m_anchor_data.bBottomAnchor	= false;
	m_anchor_data.bottom_anchor = 0;
	m_anchor_data.bRightAnchor	= false;
	m_anchor_data.right_anchor	= 0;
	m_anchor_data.bLeftAnchor	= false;
	m_anchor_data.left_anchor	= 0;
	m_anchor_data.bTopAnchor	= false;
	m_anchor_data.top_anchor	= 0;


	fhWindow.Get( "is_bottom_anchor", m_anchor_data.bBottomAnchor );
	if ( m_anchor_data.bBottomAnchor ) 
	{
		fhWindow.Get( "bottom_anchor", m_anchor_data.bottom_anchor );
	}
	fhWindow.Get( "is_right_anchor", m_anchor_data.bRightAnchor );
	if ( m_anchor_data.bRightAnchor ) 
	{
		fhWindow.Get( "right_anchor", m_anchor_data.right_anchor );
	}
	fhWindow.Get( "is_left_anchor", m_anchor_data.bLeftAnchor );
	if ( m_anchor_data.bLeftAnchor )
	{
		fhWindow.Get( "left_anchor", m_anchor_data.left_anchor );
	}
	fhWindow.Get( "is_top_anchor", m_anchor_data.bTopAnchor );
	if ( m_anchor_data.bTopAnchor )
	{
		fhWindow.Get( "top_anchor", m_anchor_data.top_anchor );
	}

	fhWindow.Get( "stretch_x", m_bStretchX );
	fhWindow.Get( "stretch_y", m_bStretchY );
	fhWindow.Get( "center_x", m_bCenterX );
	fhWindow.Get( "center_y", m_bCenterY );
	fhWindow.Get( "normalize_resize", m_bNormalizeResize );

	// Assign the group name, if any	
	fhWindow.Get( "group", m_group );	
	fhWindow.Get( "dock_group", m_dock_group );

	// Get the Draw Order
	fhWindow.Get( "draw_order", m_draw_order );

	// Get the index of the window, if any
	fhWindow.Get( "index", m_index );

	fhWindow.Get( "tag", m_tag );

	fhWindow.Get( "hit_detect", m_bCanHitDetect );

	fhWindow.Get( "consumable", m_bConsumable );

	fhWindow.Get( "pass_through", m_bPassThrough );

	fhWindow.Get( "topmost", m_bTopmost );

	// Get the drag information
	if ( fhWindow.Get( "draggable", m_bDrag ) ) 
	{
		fhWindow.Get( "drag_x", m_drag_grid_x );
		fhWindow.Get( "drag_y", m_drag_grid_y );
		fhWindow.Get( "drag_delay", m_drag_time );

		gpstring sColor;
		if ( fhWindow.Get( "drag_marquee_color", sColor ) ) 
		{
			int value_int0 = 0;
			int value_int1 = 0;
			int value_int2 = 0;
			stringtool::GetDelimitedValue( sColor, ',', 0, value_int0	);
			stringtool::GetDelimitedValue( sColor, ',', 1, value_int1	);
			stringtool::GetDelimitedValue( sColor, ',', 2, value_int2	);

			 vector_3 color( (float)(value_int0/255.0f), (float)(value_int1/255.0f), (float)(value_int2/255.0f) );
			 m_marquee_color = color;
		}
	}
}
UIWindow::UIWindow()
: m_type			( UI_TYPE_WINDOW )
, m_secondary_type	( TYPE_INPUT_ALL )
, m_draw_order		( 0 )
, m_pullout_offset	( 0 )
, m_bTexture		( false )
, m_bTiled			( false )
, m_bDrag			( false )
, m_drag_time		( 0.0f	)
, m_drag_grid_x		( 0 )
, m_drag_grid_y		( 0 )
, m_drag_time_elapsed( 0.0f )
, m_mouseX			( 0 )
, m_mouseY			( 0 )
, m_bDragAllowed	( false )
, m_marquee_color	( vector_3( 0.0f, 1.0f, 0.0f ) )
, m_bNormalized		( false )
, m_bRollover		( false )
, m_bHasBorder		( false )
, m_bHasHelpText	( false )
, m_bHasToolTip		( false )
, m_bHelpTextVisible( false )
, m_bToolTipVisible ( false )
, m_ToolTipDelay	( 0.5f )
, m_MouseHoldTime	( 0.0 )
, m_LastRealTime	( 0.0 )
, m_ToolTipX		( -1 )
, m_ToolTipY		( -1 )
, m_scale			( 1.0f )
, m_bScaled			( false )
, m_index			( 0 )
, m_texture_index	( 0 )
, m_tag				( 0 )
, m_bMarkedForDeletion( false )
, m_parentWindow	( 0 )
, m_bCommon			( false )
, m_bCanHitDetect	( true )
, m_bConsumable		( false )
, m_bEnabled		( true )
, m_BorderColor		( 0x00FFFFFF )
, m_BgColor			( 0x00FFFFFF )
, m_bBgFill			( false )
, m_bPassThrough	( false )
, m_pHelpBox		( 0 )
, m_bNormalizeResize( false )
, m_bStretchX		( false )
, m_bStretchY		( false )
, m_bCenterX		( false )
, m_bCenterY		( false )
, m_alpha			( 1.0f  )
, m_bTopmost		( false )
, m_borderPadding	( 0		)
, m_bHasFocus		( false )
{
	// Set standard UI window information
	SetAlpha( 1.0 );
	SetVisible( true );
	SetUVRect( 0, 1, 0, 1 );
	SetNormalizedRect( 0.0f, 0.0f, 0.0f, 0.0f );

	// Get any anchor information
	m_anchor_data.bBottomAnchor	= false;
	m_anchor_data.bottom_anchor = 0;
	m_anchor_data.bRightAnchor	= false;
	m_anchor_data.right_anchor	= 0;
	m_anchor_data.bLeftAnchor	= false;
	m_anchor_data.left_anchor	= 0;
	m_anchor_data.bTopAnchor	= false;
	m_anchor_data.top_anchor	= 0;	
}

/**************************************************************************

Function		: ~UIWindow()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIWindow::~UIWindow()
{
	if ( GetTextureIndex() != 0 )
	{
		if ( GetType() == UI_TYPE_CURSOR )
		{
			gRapiMouse.DestroyCursorImage( GetTextureIndex() );
		}
		else
		{
			gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
		}
	}	
}


/**************************************************************************

Function		: Resize()

Purpose			: Converts and sets the normalized rectangle to the
				  current coordinates.

Returns			: void

**************************************************************************/
void UIWindow::Resize( int width, int height )
{
	m_screenWidth			= width;
	m_screenHeight			= height;

	if ( m_bNormalized ) 
	{
		m_rect.left = (int)(m_rectNormalized.left * width);
		m_rect.right = (int)(m_rectNormalized.right * width);
		m_rect.top = (int)(m_rectNormalized.top * height);
		m_rect.bottom = (int)(m_rectNormalized.bottom * height);
	}

	int window_width	= m_rect.right - m_rect.left;
	int window_height	= m_rect.bottom - m_rect.top;

	if ( m_anchor_data.bBottomAnchor ) 
	{
		m_rect.top = height - m_anchor_data.bottom_anchor;
		m_rect.bottom = m_rect.top + window_height;
	}
	if ( m_anchor_data.bRightAnchor ) 
	{
		m_rect.left = width - m_anchor_data.right_anchor;
		m_rect.right = m_rect.left + window_width;
	}
	if ( m_anchor_data.bTopAnchor )
	{
		m_rect.top = m_anchor_data.top_anchor;
		m_rect.bottom = m_rect.top + window_height;
	}
	if ( m_anchor_data.bLeftAnchor ) 
	{
		m_rect.left = m_anchor_data.left_anchor;
		m_rect.right = m_rect.left + window_width;
	}

	SetRect( m_rect.left, m_rect.right, m_rect.top, m_rect.bottom );
}

/**************************************************************************

Function		: UpdateAnchorData()

Purpose			: Sets the windows Anchor data to the passed value
					used to update a child's anchor data.

Returns			: void

**************************************************************************/

void UIWindow::UpdateAnchorData( ANCHOR_DATA anchorData )
{
	m_anchor_data.bBottomAnchor	= anchorData.bBottomAnchor;
	m_anchor_data.bottom_anchor = anchorData.bottom_anchor;
	m_anchor_data.bRightAnchor	= anchorData.bRightAnchor;
	m_anchor_data.right_anchor	= anchorData.right_anchor;
	m_anchor_data.bLeftAnchor	= anchorData.bLeftAnchor;
	m_anchor_data.left_anchor	= anchorData.left_anchor;
	m_anchor_data.bTopAnchor	= anchorData.bTopAnchor;
	m_anchor_data.top_anchor	= anchorData.top_anchor;
}

/**************************************************************************

Function		: SetRect()

Purpose			: Sets the UIWindow's rect in screen coordinates

Returns			: void

**************************************************************************/
void UIWindow::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{
	m_rect.left		= left;
	m_rect.right	= right;
	m_rect.top		= top;
	m_rect.bottom	= bottom;

	if ( !bAnimation ) {
		m_default_rect = m_rect;
	}

	/*
	UIWindowVec children = GetChildren();
	UIWindowVec::iterator i;
	for ( i = children.begin(); i != children.end(); ++i )
	{
		(*i)->AdjustChildren();
	}
	*/
}


/**************************************************************************

Function		: SetNormalizedRect()

Purpose			: Sets the UIWindow's rect in normalized coordinates and
				  sets the screen coordinates to match

Returns			: void

**************************************************************************/
void UIWindow::SetNormalizedRect( double left, double right, double top, double bottom )
{
	m_rectNormalized.left		= left;
	m_rectNormalized.right		= right;
	m_rectNormalized.top		= top;
	m_rectNormalized.bottom		= bottom;
/*
	if ( GetTiledTexture() )
	{
		UINormalizedRect tempRect;

		float test			= (float)(m_screenWidth  * m_rectNormalized.left + 0.5f) - GetRect().left;
		float correction	= (float)(test/(float)(GetRect().right-GetRect().left))*(float)(GetUVRect().right-GetUVRect().left);
		tempRect.left		= GetUVRect().left + correction;		

		test				= (float)(m_screenWidth  * m_rectNormalized.right + 0.5f) - GetRect().right;
		correction			= (float)(test/(float)(GetRect().right-GetRect().left))*(float)(GetUVRect().right-GetUVRect().left);
		tempRect.right		= GetUVRect().right + correction;

		test				= (float)(m_screenHeight  * m_rectNormalized.top + 0.5f) - GetRect().top;
		correction			= (float)(test/(float)(GetRect().bottom-GetRect().top))*(float)(GetUVRect().bottom-GetUVRect().top);
		tempRect.top		= GetUVRect().top + correction;

		test				= (float)(m_screenHeight  * m_rectNormalized.bottom + 0.5f) - GetRect().bottom;
		correction			= (float)(test/(float)(GetRect().bottom-GetRect().top))*(float)(GetUVRect().bottom-GetUVRect().top);
		tempRect.bottom		= GetUVRect().bottom + correction;

		GetUVRect() = tempRect;
	}
*/
	m_rect.left				= (int)(m_screenWidth  * m_rectNormalized.left		+ 0.5f);
	m_rect.right			= (int)(m_screenWidth  * m_rectNormalized.right		+ 0.5f);
	m_rect.top				= (int)(m_screenHeight * m_rectNormalized.top		+ 0.5f);
	m_rect.bottom			= (int)(m_screenHeight * m_rectNormalized.bottom	+ 0.5f);

	SetRect( m_rect.left, m_rect.right, m_rect.top, m_rect.bottom );
}


void UIWindow::ResizeToCurrentResolution( int original_width, int original_height )
{
	if ( m_bNormalizeResize )
	{
		SetNormalizedRect(	(double)m_rect.left/(double)original_width,
							(double)m_rect.right/(double)original_width,
							(double)m_rect.top/(double)original_height,
							(double)m_rect.bottom/(double)original_height );
	}

	if ( m_bStretchX )
	{
		SetRect(	(int)((float)m_screenWidth * ((float)m_rect.left/(float)original_width)),
					(int)((float)m_screenWidth * ((float)m_rect.right/(float)original_width)),
					m_rect.top,
					m_rect.bottom );
	}
	if ( m_bStretchY )
	{
		SetRect(	m_rect.left,
					m_rect.right,
					(int)((float)m_screenHeight * ((float)m_rect.top/(float)original_height)),
					(int)((float)m_screenHeight * ((float)m_rect.bottom/(float)original_height)) );
	}

	if ( m_bCenterX )
	{
		int screenHalfWidth = m_screenWidth/2;
		int windowHalfWidth = (GetRect().right-GetRect().left)/2;
		SetRect( screenHalfWidth - windowHalfWidth, screenHalfWidth + windowHalfWidth,
				 GetRect().top, GetRect().bottom );
	}
	if ( m_bCenterY )
	{
		int screenHalfHeight = m_screenHeight/2;
		int windowHalfHeight = (GetRect().bottom-GetRect().top)/2;
		SetRect( GetRect().left, GetRect().right,
				 screenHalfHeight - windowHalfHeight, screenHalfHeight + windowHalfHeight );
	}

	SetPulloutOffset( m_rect.left );

}


/**************************************************************************

Function		: SetUVRect()

Purpose			: Sets the UIWindow's UV rect in normalized coordinates

Returns			: void

**************************************************************************/
void UIWindow::SetUVRect( double left, double right, double top, double bottom )
{
	m_uv_rect.left		= left;
	m_uv_rect.right		= right;
	m_uv_rect.top		= top;
	m_uv_rect.bottom	= bottom;
}


/**************************************************************************

Function		: Draw()

Purpose			: Draw the window

Returns			: void

**************************************************************************/
void UIWindow::Draw()
{
	if ( GetVisible() == true )
	{
		if ( m_bHasBorder )
		{
			gUIShell.GetRenderer().SetTextureStageState(	0,
															D3DTOP_DISABLE,
															D3DTOP_SELECTARG1,
															D3DTADDRESS_WRAP,
															D3DTADDRESS_WRAP,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DTEXF_LINEAR,
															D3DBLEND_SRCALPHA,
															D3DBLEND_INVSRCALPHA,
															false );

			tVertex LineVerts[5];
			memset( LineVerts, 0, sizeof( tVertex ) * 5 );
			DWORD borderColor = (m_BorderColor | 0xFF000000);

			LineVerts[0].x			= m_rect.left-0.5f - m_borderPadding;
			LineVerts[0].y			= m_rect.top-0.5f - m_borderPadding;
			LineVerts[0].z			= 0.0f;
			LineVerts[0].rhw		= 1.0f;
			LineVerts[0].color		= borderColor;

			LineVerts[1].x			= m_rect.left-0.5f - m_borderPadding;
			LineVerts[1].y			= m_rect.bottom-0.5f + m_borderPadding;
			LineVerts[1].z			= 0.0f;
			LineVerts[1].rhw		= 1.0f;
			LineVerts[1].color		= borderColor;

			LineVerts[2].x			= m_rect.right-0.5f + m_borderPadding;
			LineVerts[2].y			= m_rect.bottom-0.5f + m_borderPadding;
			LineVerts[2].z			= 0.0f;
			LineVerts[2].rhw		= 1.0f;
			LineVerts[2].color		= borderColor;

			LineVerts[3].x			= m_rect.right-0.5f + m_borderPadding;
			LineVerts[3].y			= m_rect.top-0.5f - m_borderPadding;
			LineVerts[3].z			= 0.0f;
			LineVerts[3].rhw		= 1.0f;
			LineVerts[3].color		= borderColor;

			LineVerts[4].x			= m_rect.left-0.5f - m_borderPadding;
			LineVerts[4].y			= m_rect.top-0.5f - m_borderPadding;
			LineVerts[4].z			= 0.0f;
			LineVerts[4].rhw		= 1.0f;
			LineVerts[4].color		= borderColor;

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 5, TVERTEX, 0, 0 );
		}

		if ( HasTexture() == true )
		{
			if ( GetTiledTexture() == true )
			{
				gUIShell.GetRenderer().SetTextureStageState(	0,
																m_texture_index != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
																D3DTOP_MODULATE,
																D3DTADDRESS_WRAP,
																D3DTADDRESS_WRAP,
																D3DTEXF_POINT,
																D3DTEXF_POINT,
																D3DTEXF_POINT,
																D3DBLEND_SRCALPHA,
																D3DBLEND_INVSRCALPHA,
																true );
			}
			else
			{
				gUIShell.GetRenderer().SetTextureStageState(	0,
																m_texture_index != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
																D3DTOP_MODULATE,
																D3DTADDRESS_CLAMP,	// WRAP
																D3DTADDRESS_CLAMP,
																D3DTEXF_POINT,		// LINEAR
																D3DTEXF_POINT,
																D3DTEXF_POINT,
																D3DBLEND_SRCALPHA,
																D3DBLEND_INVSRCALPHA,
																true );
			}

			tVertex Verts[4];
			memset( Verts, 0, sizeof( tVertex ) * 4 );

			//vector_3 default_color( 1.0f, 1.0f, 1.0f );
			//DWORD color	= MAKEDWORDCOLOR( default_color );
			DWORD color	= m_BgColor	& 0x00FFFFFF;

			Verts[0].x			= m_rect.left-0.5f - m_borderPadding;
			Verts[0].y			= m_rect.top-0.5f - m_borderPadding;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= (float)m_uv_rect.left;
			Verts[0].uv.v		= (float)m_uv_rect.bottom;
			Verts[0].color		= (color | (((BYTE)((m_alpha)*255.0f))<<24));

			Verts[1].x			= m_rect.left-0.5f - m_borderPadding;
			Verts[1].y			= m_rect.bottom-0.5f + m_borderPadding;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= (float)m_uv_rect.left;
			Verts[1].uv.v		= (float)m_uv_rect.top;
			Verts[1].color		= (color | (((BYTE)((m_alpha)*255.0f))<<24));

			Verts[2].x			= m_rect.right-0.5f + m_borderPadding;
			Verts[2].y			= m_rect.top-0.5f - m_borderPadding;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= (float)m_uv_rect.right;
			Verts[2].uv.v		= (float)m_uv_rect.bottom;
			Verts[2].color		= (color | (((BYTE)((m_alpha)*255.0f))<<24));

			Verts[3].x			= m_rect.right-0.5f + m_borderPadding;
			Verts[3].y			= m_rect.bottom-0.5f + m_borderPadding;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= (float)m_uv_rect.right;
			Verts[3].uv.v		= (float)m_uv_rect.top;
			Verts[3].color		= (color | (((BYTE)((m_alpha)*255.0f))<<24));


			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_texture_index, 1 );
		}
		else if ( m_bBgFill )
		{
			DWORD color = m_BgColor & 0x00FFFFFF;
			color |= (((BYTE)((m_alpha)*255.0f))<<24);
			gUIShell.DrawColorBox( GetRect(), color );
		}

		DrawDragBox();
	}
}


/**************************************************************************

Function		: LoadTexture()

Purpose			: Load a texture into the window

Returns			: void

**************************************************************************/
bool UIWindow::LoadTexture( gpstring texture, bool bResizeWindow )
{
	if ( !UITextureManager::DoesSingletonExist() ) {
		return false;
	}

	if ( m_texture_index != 0 )
	{
		if ( GetType() == UI_TYPE_CURSOR )
		{
			gRapiMouse.DestroyCursorImage( m_texture_index );
		}
		else
		{
			gUIShell.GetRenderer().DestroyTexture( m_texture_index );
		}
	}

	m_texture_index = gUITextureManager.LoadTexture( texture, GetInterfaceParent(), GetType() == UI_TYPE_CURSOR ? true : false );

	if ( bResizeWindow == true ) 
	{
		m_rect.left = 0;
		m_rect.right = gUIShell.GetRenderer().GetTextureWidth( GetTextureIndex() );
		m_rect.top = 0;
		m_rect.bottom = gUIShell.GetRenderer().GetTextureHeight( GetTextureIndex() );
	}

	if ( m_texture_index == gUITextureManager.GetInvalidTexture() ) 
	{
		return false;
	}
	return true;
}
void UIWindow::LoadTexture ( unsigned int texture )
{
	if ( GetTextureIndex() != 0 )
	{
		if ( GetType() == UI_TYPE_CURSOR )
		{
			gRapiMouse.DestroyCursorImage( m_texture_index );
		}
		else
		{
			gUIShell.GetRenderer().DestroyTexture( m_texture_index );
		}
	}
	SetTextureIndex( texture );
	gUIShell.GetRenderer().AddTextureReference( texture );
}


/**************************************************************************

Function		: ReceiveMessage()

Purpose			: Receive a message and dispatch it to be handled

Returns			: bool

**************************************************************************/
bool UIWindow::ReceiveMessage( const UIMessage& msg_ )
{
	UIMessage msg( msg_ );
	bool bHandled = false;

	if ( ProcessMessage( msg ) ) {
		ui_event_vector *events = GetEvents();
		ui_event_vector::iterator i;
		for ( i = events->begin(); i != events->end(); ++i ) {
			if ( (*i).msg_code == msg.GetCode() ) {
				ui_action_vector::iterator j;
				for ( j = (*i).event_actions.begin(); j != (*i).event_actions.end(); ++j ) {
					if ( ProcessAction( (*j).action, (*j).parameter ) == true ) {
						bHandled = true;
					}
				}
			}
		}
	}
	return bHandled;
}


/**************************************************************************

Function		: ConvertStringToAction()

Purpose			: Convert a standard string into a even action structure

Returns			: void

**************************************************************************/

EVENT_ACTION UIWindow::ConvertStringToAction( gpstring action )
{
	EVENT_ACTION ea;

	gpstring sAction;
	gpstring sParameter;

	if ( action.size() > 512 )
	{
		gperror( "WHooooaahhh!" );
	}

	unsigned int pos = action.find( "(" );
	if ( pos != gpstring::npos )
	{
		sAction = action.substr( 0, pos );
	}	

	unsigned int end_pos = action.rfind( ")" );
	if ( end_pos != gpstring::npos )
	{	
		sParameter = action.substr( pos+1, end_pos-pos-1 );
	}

	ea.parameter = sParameter;

	::FromGasString( sAction, ea.action );

	// return it
	return ( ea );
}


eUIMessage UIWindow::ConvertStringToMessageCode( gpstring message )
{
	eUIMessage msg;
	::FromGasString( message, msg );
	return ( msg );
}


/**************************************************************************

Function		: HitDetect()

Purpose			: Sees if the mouse is currently over the window

Returns			: bool

**************************************************************************/
bool UIWindow::HitDetect( int x, int y )
{
	if ( m_bCanHitDetect )
	{
		if (( x < m_rect.right	) && ( x >= m_rect.left		) &&
			( y >= m_rect.top	) && ( y < m_rect.bottom	)) {
			return true;
		}
	}
	return false;
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIWindow::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( ms_uiActionCb )
	{
		ms_uiActionCb( action, parameter, this );
	}

	switch ( action )
	{
	case ACTION_SETNORMALIZEDRECT :
		{
			double	value_double0	= 0;
			double	value_double1	= 0;
			double	value_double2	= 0;
			double	value_double3	= 0;

			// Set the window rectangle
			stringtool::GetDelimitedValue( parameter, ',', 0, value_double0 );
			stringtool::GetDelimitedValue( parameter, ',', 1, value_double1 );
			stringtool::GetDelimitedValue( parameter, ',', 2, value_double2 );
			stringtool::GetDelimitedValue( parameter, ',', 3, value_double3 );
			SetNormalizedRect( value_double0, value_double2, value_double1, value_double3 );

			SetRect(	(int)(value_double0*GetScreenWidth()), (int)(value_double2*GetScreenHeight()),
						(int)(value_double1*GetScreenWidth()), (int)(value_double3*GetScreenHeight()) );
		}
		return true;
	case ACTION_SETRECT:
		{
			int value_int0 = 0;
			int value_int1 = 0;
			int value_int2 = 0;
			int value_int3 = 0;

			// Set the window rectangle
			stringtool::GetDelimitedValue( parameter, ',', 0, value_int0 );
			stringtool::GetDelimitedValue( parameter, ',', 1, value_int1 );
			stringtool::GetDelimitedValue( parameter, ',', 2, value_int2 );
			stringtool::GetDelimitedValue( parameter, ',', 3, value_int3 );
			SetRect( value_int0, value_int2, value_int1, value_int3 );

			/*
			double left		= (double)GetRect().left	/ (double)GetScreenWidth();
			double right	= (double)GetRect().right	/ (double)GetScreenWidth();
			double top		= (double)GetRect().top		/ (double)GetScreenHeight();
			double bottom	= (double)GetRect().bottom	/ (double)GetScreenHeight();
			*/
			//SetNormalizedRect( left, right, top, bottom );
		}
		return true;
	case ACTION_LOADTEXTURE:
		{
			int strings = stringtool::GetNumDelimitedStrings( parameter, ',' );
			if ( strings == 1 ) 
			{
				if ( GetIsCommonControl() && gUIShell.IsCommonArt( parameter ) )
				{
					if ( GetTextureIndex() != 0 )
					{
						if ( GetType() == UI_TYPE_CURSOR )
						{
							gRapiMouse.DestroyCursorImage( GetTextureIndex() );
						}
						else
						{
							gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
						}
					}
					SetTextureIndex( gUIShell.GetCommonArt( parameter ) );
				}
				else
				{
					LoadTexture( parameter );
				}				
			}
			SetHasTexture( true );
		}
		return true;
	case ACTION_NOTIFY:
		{
			gUIMessenger.Notify( parameter, this );
		}
		return true;
	case ACTION_SETALPHA:
		{
			float alpha = 0.0;
			stringtool::Get( parameter, alpha );
			SetAlpha( alpha );
		}
		return true;
	case ACTION_MESSAGE:
		{
			gUIMessenger.SendUIMessage( UIMessage( ConvertStringToMessageCode( parameter ) ) );
		}
		return true;
	case ACTION_SETUVCOORDS:
		{
			double	value_double0	= 0;
			double	value_double1	= 0;
			double	value_double2	= 0;
			double	value_double3	= 0;

			// Set the uv coordinate rectangle
			stringtool::GetDelimitedValue( parameter, ',', 0, value_double0 );
			stringtool::GetDelimitedValue( parameter, ',', 1, value_double1 );
			stringtool::GetDelimitedValue( parameter, ',', 2, value_double2 );
			stringtool::GetDelimitedValue( parameter, ',', 3, value_double3 );

			SetUVRect( value_double0, value_double2, value_double1, value_double3 );
		}
		return true;
	case ACTION_PLAYSOUND:
		{
			gSoundManager.PlaySample( parameter );
		}
		return true;
	case ACTION_RECTANIMATION:
		{
			int left	= 0;
			int right	= 0;
			int top		= 0;
			int bottom	= 0;
			double duration= 0;

			int num = stringtool::GetNumDelimitedStrings( parameter, ',' );

			UIRect ani_rect;
			if ( num == 5 ) {
				// Set the uv coordinate rectangle
				stringtool::GetDelimitedValue( parameter, ',', 0, left );
				stringtool::GetDelimitedValue( parameter, ',', 1, top );
				stringtool::GetDelimitedValue( parameter, ',', 2, right );
				stringtool::GetDelimitedValue( parameter, ',', 3, bottom );
				stringtool::GetDelimitedValue( parameter, ',', 4, duration );
				ani_rect.left = left;
				ani_rect.right = right;
				ani_rect.top = top;
				ani_rect.bottom = bottom;
			}
			else {
				gpstring type;
				stringtool::GetDelimitedValue( parameter, ',', 0, type );
				stringtool::GetDelimitedValue( parameter, ',', 1, duration );
				ani_rect = m_default_rect;
			}

			if ( UIAnimation::DoesSingletonExist() ) {
				gUIAnimation.AddRectAnimation( duration, ani_rect, this );
			}
		}
		return true;
	case ACTION_ALPHAANIMATION:
		{
			double start_alpha	= 0;
			double end_alpha	= 0;
			double duration		= 0;
			bool	bLoop		= false;
			stringtool::GetDelimitedValue( parameter, ',', 0, duration );
			stringtool::GetDelimitedValue( parameter, ',', 1, start_alpha );
			stringtool::GetDelimitedValue( parameter, ',', 2, end_alpha );
			if ( stringtool::GetNumDelimitedStrings( parameter, ',' ) == 4 ) {
				stringtool::GetDelimitedValue( parameter, ',', 3, bLoop );
			}
			if ( UIAnimation::DoesSingletonExist() ) {
				gUIAnimation.AddAlphaAnimation( duration, start_alpha, end_alpha, this, bLoop );
			}
		}
		return true;
	case ACTION_FLASHANIMATION:
		{
			double	duration	= 0;
			bool	bLoop		= false;
			float	maxAlpha	= 1.0f;
			int		numLoops	= -1;
			stringtool::GetDelimitedValue( parameter, ',', 0, duration );
			stringtool::GetDelimitedValue( parameter, ',', 1, bLoop );
			if ( stringtool::GetNumDelimitedStrings( parameter, ',' ) >= 3 ) 
			{
				stringtool::GetDelimitedValue( parameter, ',', 2, maxAlpha );
			}
			if ( stringtool::GetNumDelimitedStrings( parameter, ',' ) == 4 ) 
			{
				stringtool::GetDelimitedValue( parameter, ',', 3, numLoops );
			}			

			if ( UIAnimation::DoesSingletonExist() ) 
			{
				gUIAnimation.AddFlashAnimation( duration, this, bLoop, maxAlpha, numLoops );
			}
		}
		break;
	case ACTION_CLOCKANIMATION:
		{
			double	duration	= 0;
			DWORD	color		= 0;
			stringtool::GetDelimitedValue( parameter, ',', 0, duration );
			stringtool::GetDelimitedValue( parameter, ',', 1, color );
			
			if ( UIAnimation::DoesSingletonExist() ) 
			{
				gUIAnimation.AddClockAnimation( duration, this, color );
			}

		}
		break;
	case ACTION_SETVISIBLE:
		{
			if ( parameter.same_no_case( "true" ) ) {
				SetVisible( true );
			}
			else if ( parameter.same_no_case( "false" )) {
				SetVisible( false );
			}
		}
		break;
	case ACTION_PARENTMESSAGE:
		{
			if ( GetParentWindow() != 0 ) {
				gUIMessenger.SendUIMessage( UIMessage( ConvertStringToMessageCode( parameter ) ), GetParentWindow() );
			}
		}
		return true;
	case ACTION_ACTIVATEMENU:
		{
			UIWindow *menu = gUIShell.FindUIWindow( parameter );
			if ( menu != 0 ) {
				gUIMessenger.SendUIMessage( UIMessage( MSG_ACTIVATEMENU ), menu );
			}
		}
		return true;
	case ACTION_SHOWGROUP:
		{
			if ( parameter.same_no_case( "true" ) )
			{
				gUIShell.ShowGroup( GetGroup(), true );
			}
			else if ( parameter.same_no_case( "false" ) )
			{
				gUIShell.ShowGroup( GetGroup(), false );
			}
		}
		return true;
	case ACTION_SHIFT_X:
		{
			SetRect( GetRect().left	+ atoi( parameter.c_str() ),
				 	 GetRect().right + atoi( parameter.c_str() ),
					 GetRect().top,
					 GetRect().bottom );
		}
		return true;
	case ACTION_SHIFT_Y:
		{
			SetRect( GetRect().left, GetRect().right,
					 GetRect().top	+ atoi( parameter.c_str() ),
					 GetRect().bottom + atoi( parameter.c_str() ) );

			UIWindowVec children = GetChildren();
			UIWindowVec::iterator k;
			for ( k = children.begin(); k != children.end(); ++k )
			{
				(*k)->AdjustChildren();
			}
		}
		return true;
	case ACTION_SETROLLOVERHELP:
		{
			SetRolloverKey( parameter );
			if ( HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
			{
				gUIShell.SetRolloverName( parameter );
			}
		}
		return true;
	case ACTION_VERTEXCOLOR:
		{
			stringtool::Get( parameter, m_BgColor );	
		}
		return true;
	case ACTION_COMMAND:
		{
			if ( gSkritEngine.ExecuteCommand( "uiwindow", parameter ).IsSuccess() )
			{
				return ( true );
			}
			gperrorf(( "An error occurred while executing '%s' in window '%s' interface '%s'\n",
					   parameter.c_str(), GetName().c_str(), GetInterfaceParent().c_str() ));
		}
		break;
	case ACTION_CALL:
		{
			// break down into path/params and funcname
			gpstring path, func;
			stringtool::GetDelimitedValue( parameter, ',', 0, path );
			stringtool::GetDelimitedValue( parameter, ',', 1, func );

			// make a skrit
			Skrit::CreateReq createReq( path );
			createReq.m_OwnerType = FuBi::GetTypeByName( "UIWindow" );
			createReq.m_Owner = this;
			createReq.m_PersistType = Skrit::PERSIST_NO;
			Skrit::HAutoObject skrit = gSkritEngine.CreateObject( createReq );
			if ( skrit )
			{
				Skrit::Result rc = skrit->Call( func );
				if ( rc.IsSuccess() )
				{
					return ( rc.m_Bool );
				}
				else
				{
					gperrorf(( "An error '%s' occurred while calling function '%s' from skrit '%s' in window '%s' interface '%s'\n",
							   Skrit::ToString( rc.m_Code ), func.c_str(),
							   path.c_str(), GetName().c_str(), GetInterfaceParent().c_str() ));
				}
			}
			else
			{
				gperrorf(( "An error occurred while attempting to compile skrit '%s' in window '%s' interface '%s'\n",
						   path.c_str(), GetName().c_str(), GetInterfaceParent().c_str() ));
			}
		}
		break;
	}

	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIWindow::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() )
	{
	case MSG_INVISIBLE:
		{
			SetVisible( false );
			return false;
		}
		break;
	case MSG_VISIBLE:
		{
			SetVisible( true );
		}
		break;
	case MSG_ROLLOVER:
		{
			m_bRollover = true;

			if( m_bHasHelpText )
			{
				m_bHelpTextVisible = true;
			}
		}
		break;
	case MSG_ROLLOFF:
		{
			// clear out the help text if we have rolled off something!
			m_bRollover = false;

			if( m_bHasHelpText )
			{
				m_bHelpTextVisible = false;

				if ( !m_pHelpBox )
				{
					m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
				}
				if( m_pHelpBox )
				{
					m_pHelpBox->SetText( gpwstring()  );
				}
			}
		}
		break;
	case MSG_DESTROYED:
	case MSG_SHOW:
		{
			m_bHidden = false;
		}
		break;
	case MSG_HIDE:
		{
			m_bHidden = true;
			if ( m_bToolTipVisible )
			{
				m_bRollover = false;
				m_bToolTipVisible = false;
				m_ToolTipX = -1;
				m_ToolTipY = -1;
				m_MouseHoldTime = 0;

				if ( !m_pHelpBox )
				{
					m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
				}
				if( m_pHelpBox )
				{
					m_pHelpBox->SetVisible( false );
				}
			}
			else if ( m_bHasHelpText && m_bRollover )
			{
				if ( !m_pHelpBox )
				{
					m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
				}
				if( m_pHelpBox )
				{
					m_pHelpBox->SetText( gpwstring() );
				}
			}

			m_bRollover = false;
			m_bHasFocus = false;
		}
		break;

	case MSG_LBUTTONDOWN:
		{
			m_bHasFocus = true;
		}
		break;
	case MSG_GLOBALLBUTTONDOWN:
		{
			if ( m_bHasFocus )
			{
				if ( !HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() ) )
				{
					m_bHasFocus = false;
				}
			}
		}
		break;
	}

	return true;
}



/**************************************************************************

Function		: SetDragAllowed()

Purpose			: Set whether or not to allow the window to be dragged

Returns			: void

**************************************************************************/
void UIWindow::SetDragAllowed( bool bAllowed )
{
	m_bDragAllowed = bAllowed;
	SetDragTimeElapsed( 0.0f );
}



/**************************************************************************

Function		: DragWindow()

Purpose			: Drag window around the screen

Returns			: void

**************************************************************************/
void UIWindow::DragWindow( int delta_x, int delta_y )
{

	UIWindowVec group;
	if ( !GetGroup().empty() ) {
		group = gUIShell.ListWindowsOfGroup( GetGroup() );
	}
	UIWindowVec::iterator i;

	if ( GetDragGridX() != 0 )
	{
		if ( abs(delta_x) >= GetDragGridX() ) {

			if ( GetDragGridX() != 0 ) {
				delta_x = (delta_x/GetDragGridX()) * GetDragGridX();
			}

			int new_left	= GetRect().left  + delta_x;
			int new_right	= GetRect().right + delta_x;
			int width	= GetRect().right - GetRect().left;

			if ( new_left <= 0 ) {
				int distance = GetRect().left;
				SetRect(	0,
							width,
							GetRect().top,
							GetRect().bottom );
				SetMousePos( m_mouseX+delta_x, m_mouseY );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left - distance,
										(*i)->GetRect().right - distance,
										(*i)->GetRect().top,
										(*i)->GetRect().bottom );
					}
				}
			}
			else if ( new_right >= GetScreenWidth() ) {
				int distance = GetScreenWidth() - GetRect().right;
				SetRect(	GetScreenWidth()-width,
							GetScreenWidth(),
							GetRect().top,
							GetRect().bottom );
				SetMousePos( m_mouseX+delta_x, m_mouseY );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left + distance,
										(*i)->GetRect().right + distance,
										(*i)->GetRect().top,
										(*i)->GetRect().bottom );
					}
				}
			}
			else
			{

				SetRect(	GetRect().left + delta_x,
							GetRect().right + delta_x,
							GetRect().top,
							GetRect().bottom );
				SetMousePos( m_mouseX+delta_x, m_mouseY );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left + delta_x,
										(*i)->GetRect().right + delta_x,
										(*i)->GetRect().top,
										(*i)->GetRect().bottom );
					}
				}
			}
		}
	}
	if ( GetDragGridY() )
	{
		if ( abs(delta_y) >= GetDragGridY() ) {

			if ( GetDragGridY() != 0 ) {
				delta_y = (delta_y/GetDragGridY()) * GetDragGridY();
			}

			int new_top		= GetRect().top + delta_y;
			int new_bottom	= GetRect().bottom + delta_y;
			int height	= GetRect().bottom - GetRect().top;

			if ( new_top <= 0 ) {
				int distance = GetRect().top;
				SetRect(	GetRect().left,
							GetRect().right,
							0,
							height );
				SetMousePos( m_mouseX, m_mouseY+delta_y );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left,
										(*i)->GetRect().right,
										(*i)->GetRect().top - distance,
										(*i)->GetRect().bottom - distance );
					}
				}
			}
			else if ( new_bottom >= GetScreenHeight() ) {
				int distance = GetScreenHeight() - GetRect().bottom;
				SetRect(	GetRect().left,
							GetRect().right,
							GetScreenHeight()-height,
							GetScreenHeight() );
				SetMousePos( m_mouseX, m_mouseY+delta_y );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left,
										(*i)->GetRect().right,
										(*i)->GetRect().top + distance,
										(*i)->GetRect().bottom + distance );
					}
				}
			}
			else {

				SetRect(	GetRect().left,
							GetRect().right,
							GetRect().top + delta_y,
							GetRect().bottom + delta_y );
				SetMousePos( m_mouseX, m_mouseY+delta_y );

				for ( i = group.begin(); i != group.end(); ++i ) {
					if ( (*i) != this ) {
						(*i)->SetRect(	(*i)->GetRect().left,
										(*i)->GetRect().right,
										(*i)->GetRect().top + delta_y,
										(*i)->GetRect().bottom + delta_y );
					}
				}
			}
		}
	}
	if ( (abs(delta_y) >= GetDragGridY()) || (abs(delta_x) >= GetDragGridX()) ) {
		ReceiveMessage( UIMessage( MSG_DRAG ) );
	}
}



// Set the current scale of the window
void UIWindow::SetScale( float scale )
{
	m_scale = scale;
}


void UIWindow::AddChild( UIWindow * pWindow )
{
	m_children.push_back( pWindow );
}


void UIWindow::RemoveChild( UIWindow * pWindow )
{
	UIWindowVec::iterator i;
	for ( i = m_children.begin(); i != m_children.end(); ++i ) {
		if ( (*i) == pWindow ) {
			m_children.erase( i );
			return;
		}
	}
}


void UIWindow::SetRolloverKey( const gpstring& sKey )
{ 
	m_sRolloverKey = sKey; 	
}


void UIWindow::SetVisible( bool bVisible )
{
	UIWindowVec::iterator i;
	for ( i = m_children.begin(); i != m_children.end(); ++i )
	{
		(*i)->SetVisible( bVisible );
	}

	if ( !bVisible && m_bVisible )
	{
		if ( m_bToolTipVisible )
		{
			m_bRollover = false;
			m_bToolTipVisible = false;
			m_ToolTipX = -1;
			m_ToolTipY = -1;
			m_MouseHoldTime = 0;

			if ( !m_pHelpBox )
			{
				m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
			}
			if( m_pHelpBox )
			{
				m_pHelpBox->SetVisible( false );
			}
		}
		else if ( m_bHasHelpText && m_bRollover )
		{
			if ( !m_pHelpBox )
			{
				m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
			}
			if( m_pHelpBox )
			{
				m_pHelpBox->SetText( gpwstring() );
			}
		}

		m_bRollover = false;
	}

	m_bVisible = bVisible;
}


void UIWindow::SetConsumable( bool bConsumable )
{
	m_bConsumable = bConsumable;
	UIWindowVec::iterator i;
	for ( i = m_children.begin(); i != m_children.end(); ++i )
	{
		(*i)->SetConsumable( bConsumable );
	}
}

void UIWindow::SetEnabled( bool bEnabled )
{
	m_bEnabled = bEnabled;
	UIWindowVec::iterator i;
	for ( i = m_children.begin(); i != m_children.end(); ++i )
	{
		(*i)->SetEnabled( bEnabled );
	}
}


void UIWindow::DrawComponent( UIRect rect, unsigned int tex, float alpha, UINormalizedRect uv_rect, bool bWrap, DWORD dwVertexColor )
{
	gUIShell.GetRenderer().SetTextureStageState(	0,
													tex != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													bWrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
													bWrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
													D3DTEXF_POINT,		// LINEAR
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

	tVertex Verts[4];
	memset( Verts, 0, sizeof( tVertex ) * 4 );

	DWORD color = (dwVertexColor | (((BYTE)((alpha)*255.0f))<<24));

	Verts[0].x			= rect.left-0.5f;
	Verts[0].y			= rect.top-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= (float)uv_rect.left;
	Verts[0].uv.v		= (float)uv_rect.bottom;
	Verts[0].color		= color;

	Verts[1].x			= rect.left-0.5f;
	Verts[1].y			= rect.bottom-0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= (float)uv_rect.left;
	Verts[1].uv.v		= (float)uv_rect.top;
	Verts[1].color		= color;

	Verts[2].x			= rect.right-0.5f;
	Verts[2].y			= rect.top-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= (float)uv_rect.right;
	Verts[2].uv.v		= (float)uv_rect.bottom;
	Verts[2].color		= color;

	Verts[3].x			= rect.right-0.5f;
	Verts[3].y			= rect.bottom-0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= (float)uv_rect.right;
	Verts[3].uv.v		= (float)uv_rect.top;
	Verts[3].color		= color;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &tex, 1 );
}


#if !GP_RETAIL
void UIWindow::Save( FuelHandle hSave, bool bChild )
{
	if ( GetParentWindow() && ( bChild == false ))
	{
		return;
	}

	if ( hSave.IsValid() )
	{
		FuelHandle hWindow = hSave->GetChildBlock( GetName(), true );
		if ( hWindow.IsValid() )
		{

			hWindow->Set( "common_control", GetIsCommonControl() );
			hWindow->Set( "common_template", GetCommonTemplate() );

			hWindow->SetType( "window" );
			hWindow->Set( "draw_order", GetDrawOrder() );

			if ( GetTextureIndex() )
			{
				gpstring sPath = gUIShell.GetRenderer().GetTexturePathname( GetTextureIndex() );
				gpstring sName = gUIShell.GetTextureNamingKeyName( sPath );
				hWindow->Set( "texture", sName );
			}

			hWindow->Set( "wrap_mode", m_bTiled ? "tiled" : "clamp" );

			if ( m_bDrag )
			{
				hWindow->Set( "draggable", m_bDrag );
				hWindow->Set( "drag_x", m_drag_grid_x );
				hWindow->Set( "drag_y", m_drag_grid_y );
				hWindow->Set( "drag_delay", m_drag_time );
			}

			gpstring sRect;
			sRect.assignf( "%d,%d,%d,%d", GetRect().left, GetRect().top, GetRect().right, GetRect().bottom );
			hWindow->Set( "rect", sRect );

			gpstring sUVRect;
			sUVRect.assignf( "%f,%f,%f,%f", GetUVRect().left, GetUVRect().top, GetUVRect().right, GetUVRect().bottom );
			hWindow->Set( "uvcoords", sUVRect );

			hWindow->Set( "alpha", GetAlpha() );
		}

		UIWindowVec children = GetChildren();
		UIWindowVec::iterator i;
		for ( i = children.begin(); i != children.end(); ++i )
		{
			if (( !GetIsCommonControl() ) || ( (*i)->GetType() == UI_TYPE_TEXT ))
			{
				(*i)->Save( hWindow, true );
			}
		}
	}
}
#endif


void UIWindow::DrawColorBox( UIRect rect, DWORD dwColor, DWORD dwBorder )
{
	gUIShell.DrawColorBox( rect, dwColor );

	gUIShell.GetRenderer().SetTextureStageState(	0,
														D3DTOP_DISABLE,
														D3DTOP_SELECTARG1,
														D3DTADDRESS_WRAP,
														D3DTADDRESS_WRAP,
														D3DTEXF_LINEAR,
														D3DTEXF_LINEAR,
														D3DTEXF_LINEAR,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														false );

	tVertex LineVerts[5];
	memset( LineVerts, 0, sizeof( tVertex ) * 5 );	

	LineVerts[0].x			= rect.left-0.5f;
	LineVerts[0].y			= rect.top-0.5f-1.0f;
	LineVerts[0].z			= 0.0f;
	LineVerts[0].rhw		= 1.0f;
	LineVerts[0].color		= dwBorder;

	LineVerts[1].x			= rect.left-0.5f;
	LineVerts[1].y			= rect.bottom-0.5f+1.0f;
	LineVerts[1].z			= 0.0f;
	LineVerts[1].rhw		= 1.0f;
	LineVerts[1].color		= dwBorder;

	LineVerts[2].x			= rect.right-0.5f+1.0f;
	LineVerts[2].y			= rect.bottom-0.5f+1.0f;
	LineVerts[2].z			= 0.0f;
	LineVerts[2].rhw		= 1.0f;
	LineVerts[2].color		= dwBorder;

	LineVerts[3].x			= rect.right-0.5f+1.0f;
	LineVerts[3].y			= rect.top-0.5f-1.0f;
	LineVerts[3].z			= 0.0f;
	LineVerts[3].rhw		= 1.0f;
	LineVerts[3].color		= dwBorder;

	LineVerts[4].x			= rect.left-0.5f;
	LineVerts[4].y			= rect.top-0.5f-1.0f;
	LineVerts[4].z			= 0.0f;
	LineVerts[4].rhw		= 1.0f;
	LineVerts[4].color		= dwBorder;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 5, TVERTEX, 0, 0 );
}


void UIWindow::DrawDragBox()
{
	if ( GetDragAllowed() && GetDraggable() && ( GetDragTimeElapsed() >= GetDragTime() )  )
	{
		gUIShell.GetRenderer().SetTextureStageState(	0,
														D3DTOP_DISABLE,
														D3DTOP_SELECTARG1,
														D3DTADDRESS_WRAP,
														D3DTADDRESS_WRAP,
														D3DTEXF_LINEAR,
														D3DTEXF_LINEAR,
														D3DTEXF_LINEAR,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														false );

		tVertex LineVerts[5];
		memset( LineVerts, 0, sizeof( tVertex ) * 5 );
		DWORD linecolor = MAKEDWORDCOLOR( m_marquee_color );

		LineVerts[0].x			= m_rect.left-0.5f;
		LineVerts[0].y			= m_rect.top-0.5f-1.0f;
		LineVerts[0].z			= 0.0f;
		LineVerts[0].rhw		= 1.0f;
		LineVerts[0].color		= linecolor;

		LineVerts[1].x			= m_rect.left-0.5f;
		LineVerts[1].y			= m_rect.bottom-0.5f+1.0f;
		LineVerts[1].z			= 0.0f;
		LineVerts[1].rhw		= 1.0f;
		LineVerts[1].color		= linecolor;

		LineVerts[2].x			= m_rect.right-0.5f+1.0f;
		LineVerts[2].y			= m_rect.bottom-0.5f+1.0f;
		LineVerts[2].z			= 0.0f;
		LineVerts[2].rhw		= 1.0f;
		LineVerts[2].color		= linecolor;

		LineVerts[3].x			= m_rect.right-0.5f+1.0f;
		LineVerts[3].y			= m_rect.top-0.5f-1.0f;
		LineVerts[3].z			= 0.0f;
		LineVerts[3].rhw		= 1.0f;
		LineVerts[3].color		= linecolor;

		LineVerts[4].x			= m_rect.left-0.5f;
		LineVerts[4].y			= m_rect.top-0.5f-1.0f;
		LineVerts[4].z			= 0.0f;
		LineVerts[4].rhw		= 1.0f;
		LineVerts[4].color		= linecolor;

		gUIShell.GetRenderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 5, TVERTEX, 0, 0 );
	}
}


void UIWindow::SetToolTip( const gpwstring& HelpText, const gpstring& HelpTextBox, bool bIsToolTip )
{
	m_sHelpText = HelpText;
	if ( LocMgr::DoesSingletonExist() )
	{
		gLocMgr.TranslateUiToolTip( m_sHelpText, GetInterfaceParent().c_str(), GetName().c_str() );
	}

	m_sHelpTextBox = HelpTextBox;

	m_bHasToolTip = bIsToolTip;
	m_bHasHelpText = !bIsToolTip;
}

void UIWindow::HideToolTip()
{
	m_bToolTipVisible = false;
	m_ToolTipX = -1;
	m_ToolTipY = -1;
	m_MouseHoldTime = 0;

	if ( !m_pHelpBox )
	{
		m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
	}
	if( m_pHelpBox )
	{
		m_pHelpBox->SetVisible( false );
	}
}

void UIWindow::UpdateHelpText( )
{
	if( m_bHasHelpText )
	{
		if ( !m_pHelpBox )
		{
			m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
		}
		if( m_pHelpBox )
		{
			m_pHelpBox->SetText( m_sHelpText  );
		}
	}
}

void UIWindow::UpdateHelp(  )
{
	if( m_bHelpTextVisible )
	{
		UpdateHelpText();
	}	
}

void UIWindow::UpdateToolTip( double seconds )
{
	if ( !gUIShell.IsModalInterfaceActive( GetInterfaceParent() ) )
	{
		if ( (m_bRollover && !HitDetect( gUIShell.GetMouseX(), gUIShell.GetMouseY() )) || m_sHelpText.empty() )
		{
			return;  // catch special case where the rollover state hasn't yet been updated.
		}

		if ( m_bToolTipVisible &&
			( !m_bRollover || !m_bVisible || gAppModule.GetLButton() || gAppModule.GetRButton() || !gUIShell.GetToolTipsVisible() ) )
		{
			HideToolTip();
		}
		else if ( m_bRollover && !m_bToolTipVisible && gUIShell.GetToolTipsVisible() && !gAppModule.GetLButton() && !gAppModule.GetRButton() )
		{
			if ( (m_ToolTipX != gUIShell.GetMouseX()) || (m_ToolTipY != gUIShell.GetMouseY()) )
			{
				m_ToolTipX = gUIShell.GetMouseX();
				m_ToolTipY = gUIShell.GetMouseY();
				m_MouseHoldTime = 0;
			}
			else
			{
				m_MouseHoldTime += seconds;
			}

			if ( (m_MouseHoldTime > m_ToolTipDelay) )
			{
				if ( !m_pHelpBox )
				{
					m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
				}
				if( m_pHelpBox )
				{
					m_pHelpBox->SetText( m_sHelpText );

					int tipLeft = m_ToolTipX;
					int tipRight = tipLeft + m_pHelpBox->GetRect().Width();
					int tipTop = m_ToolTipY - m_pHelpBox->GetRect().Height();
					int tipBottom = tipTop + m_pHelpBox->GetRect().Height();

					if ( tipRight > GetScreenWidth() )
					{
						tipLeft = GetScreenWidth() - m_pHelpBox->GetRect().Width();
						tipRight = GetScreenWidth();
					}
					if ( tipTop < 0 )
					{
						tipTop = 0;
						tipBottom = m_pHelpBox->GetRect().Height();
					}

					m_pHelpBox->SetRect( tipLeft, tipRight, tipTop, tipBottom );

					if ( GetType() == UI_TYPE_RADIOBUTTON )
					{
						PositionTextBoxOutside();
					}

					m_pHelpBox->SetVisible( true );

					m_bToolTipVisible = true;
				}

			}
		}
	}
}


void UIWindow::PositionTextBoxOutside()
{
	if ( !m_pHelpBox )
	{
		m_pHelpBox = (UITextBox *)gUIShell.FindUIWindow( m_sHelpTextBox );
	}

	if ( m_pHelpBox )
	{		
		int twidth	= m_pHelpBox->GetRect().right - m_pHelpBox->GetRect().left + m_pHelpBox->GetBorderPadding();
		int theight	= m_pHelpBox->GetRect().bottom - m_pHelpBox->GetRect().top + m_pHelpBox->GetBorderPadding();

		int rollover_width	= GetRect().right - GetRect().left;		

		RECT rect = GetRect();		
		m_pHelpBox->GetRect().left = (rect.right - ( rollover_width/2 )) - (twidth/2);
		m_pHelpBox->GetRect().right = m_pHelpBox->GetRect().left + twidth;
		m_pHelpBox->GetRect().bottom = rect.top - m_pHelpBox->GetBorderPadding();
		m_pHelpBox->GetRect().top = m_pHelpBox->GetRect().bottom - theight;

		if ( m_pHelpBox->GetRect().left < 0 )
		{
			m_pHelpBox->GetRect().left = 0;
			m_pHelpBox->GetRect().right = twidth; 
		}

		if ( m_pHelpBox->GetRect().right > gUIShell.GetScreenWidth() )
		{
			m_pHelpBox->GetRect().right = gUIShell.GetScreenWidth();
			m_pHelpBox->GetRect().left = gUIShell.GetScreenWidth() - twidth;
		}

		if ( m_pHelpBox->GetRect().top < 0 )
		{
			m_pHelpBox->GetRect().top = GetRect().bottom + m_pHelpBox->GetBorderPadding();
			m_pHelpBox->GetRect().bottom = m_pHelpBox->GetRect().top + theight;
		}							   
	}
}


void UIWindow::Update( double seconds )
{
	if ( m_bRollover && !m_bVisible )
	{
		m_bRollover = false;
	}

	if ( m_bHasToolTip && AppModule::DoesSingletonExist() )
	{
		UpdateToolTip( seconds );
	}
	
	if ( m_bHasHelpText && AppModule::DoesSingletonExist() )
	{
		UpdateHelp( );
	}
}