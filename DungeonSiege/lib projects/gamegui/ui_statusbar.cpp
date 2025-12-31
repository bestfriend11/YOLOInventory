/**************************************************************************

Filename		: ui_statusbar.cpp

Description		: Control for the standard click button

Creation Date	: 1/10/99

**************************************************************************/



#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_statusbar.h"
#include "RapiPrimitive.h"



/**************************************************************************

Function		: UIStatusBar()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIStatusBar::UIStatusBar( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
	: UIWindow( fhWindow, parent_interface, parent )
	, m_fStatus( 0 )
	, m_edge( EDGE_TOP ) 
	, m_max_size( 0 )
	, m_bUVMap( false )	
	, m_bDrawOutline( false )
	, m_baseMax( 0.0f )
	, m_bDrawAdvance( false )
	, m_dwAdvanceColor( 0xff00ff00 )
	, m_advanceDuration( 1.0f )
	, m_elapsedAdvanceDuration( 0.0f )
{	
	SetType( UI_TYPE_STATUSBAR );
	SetInputType( TYPE_INPUT_ALL );	

	SetFloatRect( (float)GetRect().left, (float)GetRect().right, (float)GetRect().top, (float)GetRect().bottom );
	gpstring sEdge;
	if ( fhWindow.Get( "dynamic_edge", sEdge ) ) 
	{		
		if ( sEdge.same_no_case( "top") ) 
		{
			m_edge = EDGE_TOP;
			m_max_size = GetFloatRect().bottom - GetFloatRect().top;
		}
		else if ( sEdge.same_no_case( "bottom" ) ) 
		{
			m_edge = EDGE_BOTTOM;
			m_max_size = GetFloatRect().bottom - GetFloatRect().top;
		}
		else if ( sEdge.same_no_case( "left") ) 
		{
			m_edge = EDGE_LEFT;
			m_max_size = GetFloatRect().right - GetFloatRect().left;
		}
		else if ( sEdge.same_no_case( "right") ) 
		{
			m_edge = EDGE_RIGHT;
			m_max_size = GetFloatRect().right - GetFloatRect().left;	
		}
	}	
	else {
		gpassertm( 0, "No dynamic edge specified for status bar control" );
	}
	fhWindow.Get( "uv_map", m_bUVMap );	
	fhWindow.Get( "draw_outline", m_bDrawOutline );

	fhWindow.Get( "draw_advance", m_bDrawAdvance );
	fhWindow.Get( "advance_color", m_dwAdvanceColor );
	fhWindow.Get( "advance_duration", m_advanceDuration );
}
UIStatusBar::UIStatusBar()
	: UIWindow()
	, m_fStatus( 0 )
	, m_edge( EDGE_TOP ) 
	, m_max_size( 0 )
	, m_bUVMap( false )	
	, m_bDrawOutline( false )
	, m_baseMax( 0.0f )
	, m_bDrawAdvance( false )
	, m_dwAdvanceColor( 0xff00ff00 )
	, m_advanceDuration( 1.0f )
	, m_elapsedAdvanceDuration( 0.0f )
{	
	SetType( UI_TYPE_STATUSBAR );
	SetInputType( TYPE_INPUT_ALL );	
}



/**************************************************************************

Function		: ~UIStatusBar()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIStatusBar::~UIStatusBar()
{
}


void UIStatusBar::Update( double seconds )
{
	if ( m_bDrawAdvance && m_elapsedAdvanceDuration != 0.0f )
	{
		m_elapsedAdvanceDuration -= (float)seconds;
		if ( m_elapsedAdvanceDuration < 0.0f )
		{
			m_elapsedAdvanceDuration = 0.0f;
		}
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIStatusBar::ProcessAction( UI_ACTION action, gpstring parameter )
{	
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}	
	else if ( action == ACTION_SETSTATUS ) {
		SetStatusPercentage( atoi( parameter.c_str() ) );
	}
	return false;
}



/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
bool UIStatusBar::ProcessMessage( UIMessage & msg )
{
	UNREFERENCED_PARAMETER( msg );
	return true;
}



/**************************************************************************

Function		: SetStatus()

Purpose			: Sets the current status information of the control 

Returns			: void

**************************************************************************/
void UIStatusBar::SetStatus( float fStatus )
{
	if ( fStatus < 0 ) {
		fStatus = 0;
	}
	if ( fStatus > 1.0f ) {
		fStatus = 1.0f;
	}

	if ( ::IsEqual( fStatus, 1.0f ) )
	{
		fStatus = 1.0f;
	}

	if		( m_edge == EDGE_TOP ) {
		SetFloatRect(	GetFloatRect().left, 
						GetFloatRect().right, 
						(GetFloatRect().bottom - (fStatus*m_max_size)),
						GetFloatRect().bottom	);		

		if ( m_bUVMap ) {			
			SetUVRect(	GetUVRect().left,
						GetUVRect().right,
						(double)(GetFloatRect().bottom-GetFloatRect().top)/(double)m_max_size,
						GetFloatRect().bottom );			
		}
	}
	else if ( m_edge == EDGE_BOTTOM ) {
		SetFloatRect(	GetFloatRect().left, 
						GetFloatRect().right, 
						GetFloatRect().top,
						(GetFloatRect().top + (fStatus*m_max_size)) );

		if ( m_bUVMap ) {			
			SetUVRect(	GetUVRect().left,
						GetUVRect().right,
						GetUVRect().top,
						(double)(GetFloatRect().bottom-GetFloatRect().top)/(double)m_max_size );			
		}
	}
	else if ( m_edge == EDGE_LEFT ) {
		SetFloatRect(	GetFloatRect().right - (fStatus*m_max_size), 
						GetFloatRect().right, 
						GetFloatRect().top,
						GetFloatRect().bottom );		

		if ( m_bUVMap ) {			
			SetUVRect(	(double)(GetFloatRect().right-GetFloatRect().left)/(double)m_max_size,
						GetUVRect().right,
						GetUVRect().top,
						GetUVRect().bottom );			
		}
	}
	else if ( m_edge == EDGE_RIGHT ) 
	{	
		SetFloatRect(	GetFloatRect().left, 
						GetFloatRect().left + (fStatus*m_max_size), 
						GetFloatRect().top,
						GetFloatRect().bottom );		

		if ( m_bUVMap ) {			
			SetUVRect(	GetUVRect().left,
						(double)(GetFloatRect().right-GetFloatRect().left)/(double)m_max_size,
						GetUVRect().top,
						GetUVRect().bottom );			
		}
	}


	m_fStatus = fStatus;
}



/**************************************************************************

Function		: SetStatusPercentage()

Purpose			: Sets the current status using a percentage for the control

Returns			: void

**************************************************************************/
void UIStatusBar::SetStatusPercentage( int iStatus )
{
	SetStatus( (float)iStatus/100.0f );
}


#if !GP_RETAIL
void UIStatusBar::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "status_bar" );

			switch ( m_edge )
			{
			case EDGE_TOP:
				hWindow->Set( "dynamic_edge", "top" );
				break;
			case EDGE_BOTTOM:
				hWindow->Set( "dynamic_edge", "bottom" );
				break;
			case EDGE_LEFT:
				hWindow->Set( "dynamic_edge", "left" );
				break;
			case EDGE_RIGHT:
				hWindow->Set( "dynamic_edge", "right" );
				break;
			}
			
		}
	}
}
#endif


void UIStatusBar::Draw()
{
	if ( GetVisible() == true ) {
		if ( HasTexture() == true ) 
		{

			if ( GetTiledTexture() == true ) {
				gUIShell.GetRenderer().SetTextureStageState(	0,
																GetTextureIndex() != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
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
			else {
				gUIShell.GetRenderer().SetTextureStageState(	0,
																GetTextureIndex() != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
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

			vector_3 default_color( 1.0f, 1.0f, 1.0f );
			DWORD color	= MAKEDWORDCOLOR( default_color );
			color		&= 0x00FFFFFF;

			Verts[0].x			= (float)int(GetFloatRect().left)-0.5f;
			Verts[0].y			= (float)int(GetFloatRect().top)-0.5f;
			Verts[0].z			= 0.0f;
			Verts[0].rhw		= 1.0f;
			Verts[0].uv.u		= (float)GetUVRect().left;
			Verts[0].uv.v		= (float)GetUVRect().bottom;
			Verts[0].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));

			Verts[1].x			= (float)int(GetFloatRect().left)-0.5f;
			Verts[1].y			= (float)int(GetFloatRect().bottom)-0.5f;
			Verts[1].z			= 0.0f;
			Verts[1].rhw		= 1.0f;
			Verts[1].uv.u		= (float)GetUVRect().left;
			Verts[1].uv.v		= (float)GetUVRect().top;
			Verts[1].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));

			Verts[2].x			= (float)int(GetFloatRect().right)-0.5f;
			Verts[2].y			= (float)int(GetFloatRect().top)-0.5f;
			Verts[2].z			= 0.0f;
			Verts[2].rhw		= 1.0f;
			Verts[2].uv.u		= (float)GetUVRect().right;
			Verts[2].uv.v		= (float)GetUVRect().bottom;
			Verts[2].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));

			Verts[3].x			= (float)int(GetFloatRect().right)-0.5f;
			Verts[3].y			= (float)int(GetFloatRect().bottom)-0.5f;
			Verts[3].z			= 0.0f;
			Verts[3].rhw		= 1.0f;
			Verts[3].uv.u		= (float)GetUVRect().right;
			Verts[3].uv.v		= (float)GetUVRect().top;
			Verts[3].color		= (color | (((BYTE)((GetAlpha())*255.0f))<<24));

			gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &GetTextureIndex(), 1 );

			if ( m_bDrawAdvance && m_elapsedAdvanceDuration != 0.0f )
			{
				float alpha = m_elapsedAdvanceDuration / (m_advanceDuration != 0.0f ? m_advanceDuration : 1.0f);
				RP_DrawFilledRect( gUIShell.GetRenderer(), m_advanceRect.left, m_advanceRect.top,
														   m_advanceRect.right, m_advanceRect.bottom, (m_dwAdvanceColor | (((BYTE)((alpha)*255.0f))<<24)), 0.0f ); 				
			}

			if ( m_bDrawOutline )
			{
				UIRect outlineRect;
				switch ( GetEdge() )
				{
				case EDGE_LEFT:
					outlineRect.left = GetRect().right - (int)GetMaxSize();
					outlineRect.right = GetRect().right;
					outlineRect.top = GetRect().top;
					outlineRect.bottom = GetRect().bottom;
					break;
				case EDGE_RIGHT:
					outlineRect.left = GetRect().left;
					outlineRect.right = GetRect().left + (int)GetMaxSize();
					outlineRect.top = GetRect().top;
					outlineRect.bottom = GetRect().bottom;

					if ( GetStatus() == 1.0f )
					{
						outlineRect.right = GetRect().right;
					}
					break;
				case EDGE_TOP:
					outlineRect.left = GetRect().left;
					outlineRect.right = GetRect().right;
					outlineRect.top = GetRect().bottom - (int)GetMaxSize();
					outlineRect.bottom = GetRect().bottom;
					break;
				case EDGE_BOTTOM:
					outlineRect.left = GetRect().left;
					outlineRect.right = GetRect().right;
					outlineRect.top = GetRect().top;
					outlineRect.bottom = GetRect().top + (int)GetMaxSize();
					break;
				}

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

				LineVerts[0].x			= outlineRect.left-0.5f;
				LineVerts[0].y			= outlineRect.top-0.5f;
				LineVerts[0].z			= 0.0f;
				LineVerts[0].rhw		= 1.0f;
				LineVerts[0].color		= GetBorderColor();

				LineVerts[1].x			= outlineRect.left-0.5f;
				LineVerts[1].y			= outlineRect.bottom-0.5f;
				LineVerts[1].z			= 0.0f;
				LineVerts[1].rhw		= 1.0f;
				LineVerts[1].color		= GetBorderColor();

				LineVerts[2].x			= outlineRect.right-0.5f;
				LineVerts[2].y			= outlineRect.bottom-0.5f;
				LineVerts[2].z			= 0.0f;
				LineVerts[2].rhw		= 1.0f;
				LineVerts[2].color		= GetBorderColor();

				LineVerts[3].x			= outlineRect.right-0.5f;
				LineVerts[3].y			= outlineRect.top-0.5f;
				LineVerts[3].z			= 0.0f;
				LineVerts[3].rhw		= 1.0f;
				LineVerts[3].color		= GetBorderColor();

				LineVerts[4].x			= outlineRect.left-0.5f;
				LineVerts[4].y			= outlineRect.top-0.5f;
				LineVerts[4].z			= 0.0f;
				LineVerts[4].rhw		= 1.0f;
				LineVerts[4].color		= GetBorderColor();

				gUIShell.GetRenderer().DrawPrimitive( D3DPT_LINESTRIP, LineVerts, 5, TVERTEX, 0, 0 );

			}
		}
	}
}


void UIStatusBar::SetRect( int left, int right, int top, int bottom, bool bAnimation )
{
	UIWindow::SetRect( left, right, top, bottom, bAnimation );
	SetFloatRect( (float)left, (float)right, (float)top, (float)bottom );
}


void UIStatusBar::SetFloatRect( float left, float right, float top, float bottom )
{
	if ( m_bDrawAdvance )
	{
		switch ( m_edge )
		{
		case EDGE_TOP:
			if ( m_advanceRect.top != (int)top )
			{
				m_advanceRect.left = GetRect().left;
				m_advanceRect.top  = GetRect().top;
				m_advanceRect.right = GetRect().right;		
				m_advanceRect.bottom = m_advanceRect.top;
			}
			break;
		case EDGE_BOTTOM:
			if ( m_advanceRect.bottom != (int)bottom )
			{
				m_advanceRect.left = GetRect().left;		
				m_advanceRect.right = GetRect().right;
				m_advanceRect.bottom = GetRect().bottom;
				m_advanceRect.top = m_advanceRect.bottom;
			}
			break;
		case EDGE_LEFT:
			if ( m_advanceRect.left != (int)left )
			{
				m_advanceRect.left = GetRect().left;
				m_advanceRect.top  = GetRect().top;
				m_advanceRect.bottom = GetRect().bottom;
				m_advanceRect.right = m_advanceRect.left;
			}
			break;	
		case EDGE_RIGHT:
			if ( m_advanceRect.right != (int)right )
			{				
				m_advanceRect.top  = GetRect().top;
				m_advanceRect.right = GetRect().right;
				m_advanceRect.bottom = GetRect().bottom;
				m_advanceRect.left = m_advanceRect.right;
			}
			break;
		}
	}

	UIWindow::SetRect( (int)left, (int)right, (int)top, (int)bottom );
	GetFloatRect().left = left;
	GetFloatRect().right = right;
	GetFloatRect().top = top;
	GetFloatRect().bottom = bottom;

	if ( m_bDrawAdvance )
	{
		switch ( m_edge )
		{
		case EDGE_TOP:
			if ( m_advanceRect.top != GetRect().top )
			{
				m_advanceRect.top = GetRect().top;
				m_elapsedAdvanceDuration = m_advanceDuration;
			}
			break;
		case EDGE_BOTTOM:			
			if ( m_advanceRect.bottom != GetRect().bottom )
			{
				m_advanceRect.bottom = GetRect().bottom;
				m_elapsedAdvanceDuration = m_advanceDuration;
			}
			break;
		case EDGE_LEFT:
			if ( m_advanceRect.left != GetRect().left )
			{
				m_advanceRect.left = GetRect().left;
				m_elapsedAdvanceDuration = m_advanceDuration;
			}
			break;	
		case EDGE_RIGHT:
			if ( m_advanceRect.right < GetRect().right )
			{
				m_advanceRect.right = GetRect().right;
				m_elapsedAdvanceDuration = m_advanceDuration;
			}
			break;
		}
	}
}

bool UIStatusBar::HitDetect( int x, int y )
{	
	if (( x < GetRect().right	) && ( x > GetRect().left ) &&
		( y > GetRect().top	) && ( y < GetRect().bottom	)) {
		return true;
	}
	return false;
}


void UIStatusBar::ResizeToCurrentResolution( int original_width, int original_height )
{	
	UIWindow::ResizeToCurrentResolution( original_width, original_height );	

	/*
	switch ( m_edge )
	{
	case EDGE_TOP:
		m_max_size = m_max_size * ( maxRect.bottom / ;
		break;
	case EDGE_BOTTOM:
		m_max_size = GetFloatRect().bottom - GetFloatRect().top;
		break;
	case EDGE_LEFT:
		m_max_size = GetFloatRect().right - GetFloatRect().left;
		break;	
	case EDGE_RIGHT:
		m_max_size = GetFloatRect().right - GetFloatRect().left;	
		break;
	}
	*/
}


void UIStatusBar::SetScale( float scale )
{
	if ( GetScale() != 1.0f )
	{
		float adjustScale = 1.0f / GetScale();

		SetFloatRect( GetFloatRect().left,
					  GetFloatRect().right * adjustScale,
					  GetFloatRect().top,
					  GetFloatRect().bottom * adjustScale );
			
		
		m_max_size *= adjustScale;
	}

	switch ( m_edge )
	{
	case EDGE_TOP:
	case EDGE_BOTTOM:
		{
			SetFloatRect( GetFloatRect().left,
						  GetFloatRect().right * scale,
						  GetFloatRect().top,
						  GetFloatRect().top + ((GetFloatRect().bottom-GetFloatRect().top) * scale) );
		}
		break;		
	case EDGE_LEFT:			
	case EDGE_RIGHT:
		{
			SetFloatRect( GetFloatRect().left,
						  GetFloatRect().left + ((GetFloatRect().right-GetFloatRect().left) * scale),
						  GetFloatRect().top,
						  GetFloatRect().bottom * scale );
		}
		break;
	} 

	m_max_size *= scale;

	UIWindow::SetScale( scale );
}