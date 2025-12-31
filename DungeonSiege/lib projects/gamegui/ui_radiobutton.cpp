/**************************************************************************

Filename		: ui_radiobutton.cpp

Description		: Control for the radio buttons

Creation Date	: 10/4/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_radiobutton.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_textureman.h"
#include "ui_animation.h"
#include "AppModule.h"


/**************************************************************************

Function		: UIRadioButton()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIRadioButton::UIRadioButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_buttondown( false )
	, m_checked( false )
	, m_bStretchCtrl( false )
	, m_bPopup( false )
	, m_bAllowUserPress( true )
	, m_bRolloverHighlight( false )
	, m_highlightColor( 0xff00ff00 )
	, m_bStrictRollover( false )
	, m_clickDelay( 0.0 )
	, m_secondsElapsed( 0.0 )
	, m_bMidState( false )
	, m_bResetDown( false )
	, m_bIgnoreItems( true )
	, m_bSendUncheckToSelf( false )
	, m_bLeftRightButton( false )
	, m_selection_texture( 0 )
	, m_bSelectionTexture( false )	
	, m_selection_alpha( 1.0f )	
	, m_rollover_texture( 0 )
	, m_bRolloverTexture( false )
	, m_rollover_alpha( 1.0f )
	, m_dwInvalidColor( 0x55ff0000 )
	, m_bInvalid( false )
	, m_button_center_tex( 0 )
	, m_button_left_tex( 0 )
	, m_button_right_tex( 0 )	
{	
	SetType( UI_TYPE_RADIOBUTTON );
	SetInputType( TYPE_INPUT_ALL );	

	FastFuelHandle hSel = fhWindow.GetChildTyped( "selection_texture" );
	if ( hSel )
	{
		gpstring texture;
		m_selection_alpha = 1;
		hSel.Get( "texture", texture );
		LoadSelectionTexture( texture );
		hSel.Get( "alpha", m_selection_alpha );
		
		gpstring uvcoords;
		hSel.Get( "uvcoords", uvcoords );
		double	value_double0	= 0;
		double	value_double1	= 0;
		double	value_double2	= 0;
		double	value_double3	= 0;

		// Set the uv coordinate rectangle		
		stringtool::GetDelimitedValue( uvcoords, ',', 0, value_double0 );
		stringtool::GetDelimitedValue( uvcoords, ',', 1, value_double1 );
		stringtool::GetDelimitedValue( uvcoords, ',', 2, value_double2 );
		stringtool::GetDelimitedValue( uvcoords, ',', 3, value_double3 );

		m_selection_uv.left		= value_double0;
		m_selection_uv.top		= value_double1;
		m_selection_uv.right	= value_double2;
		m_selection_uv.bottom	= value_double3;
		
		m_bSelectionTexture = true;
	}
	
	FastFuelHandle hRollover = fhWindow.GetChildTyped( "rollover_texture" );
	if ( hRollover )
	{
		gpstring texture;
		m_rollover_alpha = 1.0f;
		hRollover.Get( "texture", texture );		
		m_rollover_texture = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );

		LoadSelectionTexture( texture );
		hRollover.Get( "alpha", m_rollover_alpha );
		
		gpstring uvcoords;
		hRollover.Get( "uvcoords", uvcoords );
		double	value_double0	= 0;
		double	value_double1	= 0;
		double	value_double2	= 0;
		double	value_double3	= 0;

		// Set the uv coordinate rectangle		
		stringtool::GetDelimitedValue( uvcoords, ',', 0, value_double0 );
		stringtool::GetDelimitedValue( uvcoords, ',', 1, value_double1 );
		stringtool::GetDelimitedValue( uvcoords, ',', 2, value_double2 );
		stringtool::GetDelimitedValue( uvcoords, ',', 3, value_double3 );

		m_rollover_uv.left		= value_double0;
		m_rollover_uv.top		= value_double1;
		m_rollover_uv.right		= value_double2;
		m_rollover_uv.bottom	= value_double3;
		
		m_bRolloverTexture = true;
	}

	if ( !fhWindow.Get( "radio_group", m_sRadioGroup ) ) 
	{
		gpassertm( 0, "Radio button does not have a radio_group!  That kind of defeats the purpose..." );
	}

	fhWindow.Get( "popup", m_bPopup );
	fhWindow.Get( "allow_user_press", m_bAllowUserPress );
	fhWindow.Get( "rollover_highlight", m_bRolloverHighlight );
	fhWindow.Get( "highlight_color", m_highlightColor );
	fhWindow.Get( "click_delay", m_clickDelay );

	fhWindow.Get( "invalid_color", m_dwInvalidColor );
	fhWindow.Get( "send_uncheck_to_self", m_bSendUncheckToSelf );
	fhWindow.Get( "leftright_button", m_bLeftRightButton );

	fhWindow.Get( "ignore_items", m_bIgnoreItems );

	bool bCommon = false;
	if ( fhWindow.Get( "common_control", bCommon ) ) 
	{
		if ( bCommon ) 
		{
			gpstring sTemplate;
			fhWindow.Get( "common_template", sTemplate );
			CreateCommonCtrl( sTemplate );		
		}
		return;
	}	

}
UIRadioButton::UIRadioButton()
	: UIWindow()
	, m_buttondown( false )
	, m_checked( false )
	, m_bStretchCtrl( false )
	, m_bPopup( false )
	, m_bAllowUserPress( true )
	, m_bRolloverHighlight( false )
	, m_highlightColor( 0xff00ff00 )
	, m_bStrictRollover( false )
	, m_clickDelay( 0.0 )
	, m_secondsElapsed( 0.0 )
	, m_bMidState( false )
	, m_bResetDown( false )
	, m_bIgnoreItems( true )
	, m_bSendUncheckToSelf( false )
	, m_bLeftRightButton( false )
	, m_selection_texture( 0 )
	, m_bSelectionTexture( false )	
	, m_selection_alpha( 1.0f )	
	, m_rollover_texture( 0 )
	, m_bRolloverTexture( false )
	, m_rollover_alpha( 1.0f )
	, m_dwInvalidColor( 0x55ff0000 )
	, m_bInvalid( false )
	, m_button_center_tex( 0 )
	, m_button_left_tex( 0 )
	, m_button_right_tex( 0 )
{	
	SetType( UI_TYPE_RADIOBUTTON );
	SetInputType( TYPE_INPUT_ALL );		
}


/**************************************************************************

Function		: ~UIRadioButton()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIRadioButton::~UIRadioButton()
{
	if ( m_button_center_tex != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
	}
	if ( m_button_left_tex != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
	}
	if ( m_button_right_tex != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
	}
	if ( m_selection_texture != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_selection_texture );
	}
	if ( m_rollover_texture != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_rollover_texture );
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIRadioButton::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) 
	{
		if ( m_bMidState )
		{
			if ( GetCommonTemplate().same_no_case( "crystal" ) )
			{		
				if ( GetTextureIndex() != 0 )
				{
					gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
				}
				SetTextureIndex( gUIShell.GetCommonArt( "radio_crystal_mid" ) );							
			}
		}
		return true;
	}
	else if ( action == ACTION_SETGROUP ) {
		SetGroup( parameter );
		return true;
	}	
	else if ( action == ACTION_SETSTATE ) {
		int state = 0;
		stringtool::Get( parameter, state );
		if ( state == 0 ) {
			m_checked = false;			
		}
		else if ( state == 1 ) {
			gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
			gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );
			m_checked = true;
		}
		return true;
	}
	return false;
}


void UIRadioButton::SetCheck( bool bCheck ) 
{
	m_checked = bCheck;
	if ( m_checked == true ) 
	{
		bool bMidState = m_bMidState;
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
		m_bMidState = bMidState;
		gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );		
	}
	else {
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
	}
}


void UIRadioButton::SetForceCheck( bool bCheck )
{
	m_checked = bCheck;
	if ( m_checked == true )
	{
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
		gUIMessenger.SendUIMessage( UIMessage( MSG_FORCECHECK ), this );
	}
	else 
	{
		gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup() );
	}
}

/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UIRadioButton::ProcessMessage( UIMessage & msg )
{
	UIWindow::ProcessMessage( msg );
	
	if ( !m_bIgnoreItems && gUIShell.GetItemActive() )
	{
		return false;
	}

	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			if ( GetAllowUserPress() )
			{
				m_buttondown = true;				
				OnButtonDown();
			}

			m_secondsElapsed = 0.0f;
			m_bResetDown = true;	
		}
		break;
	case MSG_RBUTTONDOWN:
		{
			if ( m_bLeftRightButton )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
				break;
			}
		}
		break;
	case MSG_LBUTTONUP:
		{
			if ( GetAllowUserPress() )
			{
				if ( m_clickDelay && m_buttondown )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_RESETCLICKDELAY ) );
				}

				if ( m_buttondown == true ) 
				{
					bool bChecked = m_checked;
					gUIMessenger.SendUIMessageToGroup( UIMessage( MSG_UNCHECK ), GetRadioGroup(), this );					
					
					if ( m_bPopup )
					{
						m_checked = !bChecked;
						if ( m_checked )
						{
							OnButtonDown();
							m_bMidState = false;
							gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );					
						}
						else
						{
							if ( m_bSendUncheckToSelf )
							{
								gUIMessenger.SendUIMessage( UIMessage( MSG_UNCHECK ), this );
							}
							OnButtonUp();							
						}
					}
					else
					{
						m_bMidState = false;
						m_checked = true;
						gUIMessenger.SendUIMessage( UIMessage( MSG_CHECK ), this );						
					}				
				}
				m_buttondown	= false;
				m_bResetDown	= false;
				OnButtonUp();
			}
			else
			{
				if ( m_clickDelay && m_bResetDown )
				{
					gUIMessenger.SendUIMessage( UIMessage( MSG_RESETCLICKDELAY ) );
				}
				m_bResetDown	= false;
			}
		}
		break;
	case MSG_RBUTTONUP:
		{
			if ( m_bLeftRightButton )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONUP ), this );
				break;
			}
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			if ( m_clickDelay && m_bResetDown )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_RESETCLICKDELAY ) );
			}
			m_buttondown = false;
			m_bResetDown = false;
		}
		break;
	case MSG_GLOBALRBUTTONUP:
		{
			if ( m_bLeftRightButton )
			{
				gUIMessenger.SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUP ), this );
				break;
			}
		}
		break;
	case MSG_ROLLOVER:
		{
			if ( GetAllowUserPress() )
			{
				OnButtonRollover();
				if ( m_buttondown == true ) {
					gUIMessenger.SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), this );
					return false;
				}
				if ( gAppModule.GetLButton() == true ) {
					return false;
				}

				m_bStrictRollover = true;
				// why was this here in the first place?
				/*
				if ( m_checked == true ) 
				{
					return false;
				}			
				*/
			}
			else 
			{
				SetRollover( false );
			}
		}
		break;
	case MSG_ROLLOFF:
		{
			if ( GetAllowUserPress() )
			{
				OnButtonRolloff();
				m_bStrictRollover = false;
				// why was this here in the first place?
				/*
				if ( m_checked == true ) 
				{
					return false;
				}		
				*/
			}
		}
		break;
	case MSG_UNCHECK:
		{			
			m_checked = false;
			m_bMidState = false;
			OnButtonUp();
		}
		break;
	case MSG_CHECK:
		{				
			OnButtonDown();	
			m_checked = true;			
		}
		break;	
	case MSG_FORCECHECK:
		{
			m_checked = true;
		}
		break;
	}

	// Preprocess message
	if ( GetCheck() )
	{
		switch ( msg.GetCode() )
		{
		case MSG_LBUTTONDOWN:
			msg.m_code = MSG_LBUTTONDOWNCHECKED;
			break;
		case MSG_LBUTTONUP:
			msg.m_code = MSG_LBUTTONUPCHECKED;
			break;
		case MSG_ROLLOVER:
			msg.m_code = MSG_ROLLOVERCHECKED;
			break;
		case MSG_ROLLOFF:
			msg.m_code = MSG_ROLLOFFCHECKED;
			break;
		}
	}

	return true;
}


void UIRadioButton::ResetButtonStatus()
{
	m_buttondown = false;
	m_bResetDown = false;
	gUIMessenger.SendUIMessage( UIMessage( MSG_RESETCLICKDELAY ) );	
}

void UIRadioButton::Update( double seconds )
{
	UIWindow::Update( seconds );
	if ( (m_buttondown || m_bResetDown) && m_clickDelay && GetRollover() )
	{
		m_secondsElapsed += (float)seconds;
		if ( m_secondsElapsed >= m_clickDelay )
		{
			gUIMessenger.SendUIMessage( UIMessage( MSG_CLICKDELAY ), this );				
			m_secondsElapsed = 0.0f;
		}
	}	
}

/**************************************************************************

Function		: LoadSelectionTexture()

Purpose			: Load in a texture for the selection

Returns			: void

**************************************************************************/
void UIRadioButton::LoadSelectionTexture( const gpstring& texture )
{
	if ( m_selection_texture != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( m_selection_texture );
	}
	m_selection_texture = gUITextureManager.LoadTexture( texture, GetInterfaceParent() );
}


/**************************************************************************

Function		: DrawSelectionTexture()

Purpose			: Draw a texture for the selection

Returns			: void

**************************************************************************/
void UIRadioButton::DrawSelectionTexture()
{
	if ( GetVisible() == true && GetAlpha() != 0.0f ) 
	{
		gUIShell.GetRenderer().SetTextureStageState(	0,
														m_selection_texture != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_CLAMP,	// WRAP
														D3DTADDRESS_CLAMP,
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
		color		&= 0x00FFFFFF;

		Verts[0].x			= GetRect().left-0.5f;
		Verts[0].y			= GetRect().top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= (float)m_selection_uv.left;
		Verts[0].uv.v		= (float)m_selection_uv.bottom;
		Verts[0].color		= (color | (((BYTE)((m_selection_alpha)*255.0f))<<24));

		Verts[1].x			= GetRect().left-0.5f;
		Verts[1].y			= GetRect().bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= (float)m_selection_uv.left;
		Verts[1].uv.v		= (float)m_selection_uv.top;
		Verts[1].color		= (color | (((BYTE)((m_selection_alpha)*255.0f))<<24));

		Verts[2].x			= GetRect().right-0.5f;
		Verts[2].y			= GetRect().top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= (float)m_selection_uv.right;
		Verts[2].uv.v		= (float)m_selection_uv.bottom;
		Verts[2].color		= (color | (((BYTE)((m_selection_alpha)*255.0f))<<24));

		Verts[3].x			= GetRect().right-0.5f;
		Verts[3].y			= GetRect().bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= (float)m_selection_uv.right;
		Verts[3].uv.v		= (float)m_selection_uv.top;
		Verts[3].color		= (color | (((BYTE)((m_selection_alpha)*255.0f))<<24));

		gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_selection_texture, 1 );
	}
}


/**************************************************************************

Function		: Draw()

Purpose			: Render the radio button and its selection texture, if
				  applicable

Returns			: void

**************************************************************************/
void UIRadioButton::Draw() 
{ 
	if ( GetVisible() == true ) {
		UIWindow::Draw();

		if ( m_bStretchCtrl ) 
		{
			UIRect component_rect;
			int width	= gUIShell.GetRenderer().GetTextureWidth( m_button_left_tex );

			component_rect.left		= GetRect().left;
			component_rect.right	= GetRect().left + width;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;
			DrawComponent( component_rect, m_button_left_tex, GetAlpha() );

			component_rect.left		= GetRect().right - width;
			component_rect.right	= GetRect().right;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;
			DrawComponent( component_rect, m_button_right_tex, GetAlpha() );

			component_rect.left		= GetRect().left + width;
			component_rect.right	= GetRect().right - width;
			component_rect.top		= GetRect().top;
			component_rect.bottom	= GetRect().bottom;

			int sideWidth	= gUIShell.GetRenderer().GetTextureWidth( m_button_center_tex );	
			UINormalizedRect uv_component_rect;
			uv_component_rect.top	= 0.0f;
			uv_component_rect.left	= 0.0f;
			uv_component_rect.right	= (float)( component_rect.right-component_rect.left ) / (float)sideWidth;
			uv_component_rect.bottom= 1.0f;

			DrawComponent( component_rect, m_button_center_tex, GetAlpha(), uv_component_rect, true );		
		}

		if ( GetRollover() && m_bStrictRollover && m_bRolloverHighlight )
		{
			gUIShell.DrawColorBox( GetRect(), m_highlightColor );
		}

		if (( m_bRolloverTexture == true ) && GetRollover() && m_bStrictRollover )
		{
			DrawComponent( GetRect(), m_rollover_texture, m_rollover_alpha, m_rollover_uv );
		}

		if (( m_bSelectionTexture == true ) && ( m_checked == true ) && !m_bMidState ) 
		{
			DrawSelectionTexture();
		}		

		if ( m_bInvalid )
		{
			gUIShell.DrawColorBox( GetRect(), m_dwInvalidColor );
		}
	}
}


void UIRadioButton::SetMidState( bool bSet )
{
	m_bMidState = bSet;	
	if ( m_bMidState )
	{
		if ( GetCommonTemplate().same_no_case( "crystal" ) )
		{		
			if ( GetTextureIndex() != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
			}
			SetTextureIndex( gUIShell.GetCommonArt( "radio_crystal_mid" ) );							
		}
	}
}


#if !GP_RETAIL
void UIRadioButton::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "radio_button" );
			hWindow->Set( "radio_group", m_sRadioGroup );
		}
	}
}
#endif


void UIRadioButton::CreateCommonCtrl( gpstring sTemplate )
{
	UNREFERENCED_PARAMETER( sTemplate );
	SetIsCommonControl( true );
	SetCommonTemplate( sTemplate );
	if ( sTemplate.empty() )
	{
		m_bStretchCtrl = true;
		SetCommonTemplate( "" );
		m_button_center_tex	= gUIShell.GetCommonArt( "button2_center_up" );
		m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );
		m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
	}	
	else 
	{
		if ( sTemplate.same_no_case( "soundon" ) )
		{
			CreateStaticCommonCtrl( "button_soundon" );
			m_selection_texture = gUIShell.GetCommonArt( "button_sound_slash" );
			m_bSelectionTexture = true;
		}
		else if ( sTemplate.same_no_case( "soundoff" ) )
		{
			CreateStaticCommonCtrl( "button_soundoff" );
		}
		else if ( sTemplate.same_no_case( "crystal" ) )
		{
			CreateStaticCommonCtrl( "radio_crystal_off" );
			m_selection_texture = gUIShell.GetCommonArt( "radio_crystal_on_up" );
			m_bSelectionTexture = true;
			m_selection_uv = GetUVRect();
		}
		else if ( sTemplate.same_no_case( "button_4" ) )
		{
			m_bStretchCtrl = true;
			SetCommonTemplate( "button_4" );
			m_button_center_tex	= gUIShell.GetCommonArt( "button4_center_up" );
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
		}
	}
}



void UIRadioButton::CreateStaticCommonCtrl( gpstring sTexturePrefix )
{	
	gpstring sTextureName = sTexturePrefix;
	sTextureName =	sTexturePrefix;
	sTextureName += "_up";

	if ( GetTextureIndex() != 0 )
	{
		gUIShell.GetRenderer().DestroyTexture( GetTextureIndex() );
	}

	SetTextureIndex( gUIShell.GetCommonArt( sTextureName ) );
	SetHasTexture( true );

	{
		UI_EVENT ue;
		ue.msg_code = MSG_UNCHECK;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	/*
	{
		UI_EVENT ue;
		ue.msg_code = MSG_ROLLOFF;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	*/

	sTextureName = sTexturePrefix;
	sTextureName += "_down";

	{
		UI_EVENT ue;
		ue.msg_code = MSG_LBUTTONDOWN;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	{ 
		UI_EVENT ue;
		ue.msg_code = MSG_CHECK;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}

	sTextureName = sTexturePrefix;
	sTextureName += "_hov";

	/*
	{
		UI_EVENT ue;
		ue.msg_code = MSG_ROLLOVER;
		EVENT_ACTION ea;
		ea.action = ACTION_LOADTEXTURE;
		ea.parameter = sTextureName;
		ue.event_actions.push_back( ea );
		GetEvents()->push_back( ue );
	}
	*/
}


void UIRadioButton::OnButtonDown()
{
	if ( m_bStretchCtrl ) 
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_down" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_down" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_down" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_down" );
		}
	}
}


void UIRadioButton::OnButtonUp()
{
	if (( m_bStretchCtrl ) && !GetCheck() )
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
		}
	}
}


void UIRadioButton::OnButtonRollover()
{
	if (( m_bStretchCtrl ) && !GetCheck() )
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_hov" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_hov" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_hov" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_hov" );
		}
	}
}


void UIRadioButton::OnButtonRolloff()
{
	if (( m_bStretchCtrl ) && !GetCheck() )
	{
		if ( GetCommonTemplate().empty() )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button2_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button2_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button2_right_up" );
		}
		else if ( GetCommonTemplate().same_no_case( "button_4" ) )
		{
			if ( m_button_center_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_center_tex );
			}
			m_button_center_tex = gUIShell.GetCommonArt( "button4_center_up" );

			if ( m_button_left_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_left_tex );
			}
			m_button_left_tex	= gUIShell.GetCommonArt( "button4_left_up" );

			if ( m_button_right_tex != 0 )
			{
				gUIShell.GetRenderer().DestroyTexture( m_button_right_tex );
			}
			m_button_right_tex	= gUIShell.GetCommonArt( "button4_right_up" );
		}
	}
}