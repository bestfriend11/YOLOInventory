/**************************************************************************

Filename		: ui_cursor.cpp

Description		: User interface cursor control

Creation Date	: 9/22/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_cursor.h"
#include "ui_messenger.h"
#include "fuel.h"
#include "rapiowner.h"
#include "rapimouse.h"
#include "stringtool.h"
#include "ui_animation.h"
#include "AppModule.h"


/**************************************************************************

Function		: UICursor()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UICursor::UICursor( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_hotspotX( 0 )
, m_hotspotY( 0 )
{	
	SetType( UI_TYPE_CURSOR );	
	SetInputType( TYPE_INPUT_NONE );
	SetDrawOrder( DRAW_ORDER_TOP );
}
UICursor::UICursor()
: UIWindow()
, m_hotspotX( 0 )
, m_hotspotY( 0 )
{	
	SetType( UI_TYPE_CURSOR );	
	SetInputType( TYPE_INPUT_NONE );
	SetDrawOrder( DRAW_ORDER_TOP );
}



/**************************************************************************

Function		: ~UICursor()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UICursor::~UICursor()
{	
	if ( m_animated_textures.size() != 0 )
	{
		std::vector< unsigned int >::iterator i;
		for ( i = m_animated_textures.begin(); i != m_animated_textures.end(); ++i )
		{
			if ( GetTextureIndex() != (*i) )
			{
				gRapiMouse.DestroyCursorImage( *i );
			}
		}
	}
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UICursor::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}
	else if ( action == ACTION_SETRECT ) {
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

		double left		= (double)GetRect().left	/ (double)GetScreenWidth();
		double right	= (double)GetRect().right	/ (double)GetScreenWidth();
		double top		= (double)GetRect().top		/ (double)GetScreenHeight();
		double bottom	= (double)GetRect().bottom	/ (double)GetScreenHeight();
		SetNormalizedRect( left, right, top, bottom );
		return true;
	}
	else if ( action == ACTION_SETHOTSPOT ) {
		unsigned int value_int0 = 0;
		unsigned int value_int1 = 0;
		stringtool::GetDelimitedValue( parameter, ',', 0, value_int0 );
		stringtool::GetDelimitedValue( parameter, ',', 1, value_int1 );
		SetHotspot( value_int0, value_int1 );		
		return true;
	}	
	else if ( action == ACTION_LOADANIMATEDTEXTURE ) {
		LoadAnimatedTexture( parameter );
		SetHasTexture( true );
	}
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: bool

**************************************************************************/
bool UICursor::ProcessMessage( UIMessage & msg )
{	
	UNREFERENCED_PARAMETER( msg );
	return true;
}



/**************************************************************************

Function		: SetHotSpot()

Purpose			: Determines where the click point is on the mouse image

Returns			: void

**************************************************************************/
void UICursor::SetHotspot( unsigned int x, unsigned int y )
{
	m_hotspotX = x;
	m_hotspotY = y;
}


/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the cursor
				  rectangle accordingly

Returns			: void

**************************************************************************/
void UICursor::SetMousePos( unsigned int x, unsigned int y )
{
	SetMouseX( x );
	SetMouseY( y );
	
	int left	= GetMouseX()-m_hotspotX;
	int right	= left	+ ( GetRect().right-GetRect().left );
	int top		= GetMouseY()-m_hotspotY;
	int bottom	= top	+ ( GetRect().bottom-GetRect().top );
	SetRect( left, right, top, bottom );
}


void UICursor::Draw()
{
	// Make sure it draws in the right location
	if ( AppModule::DoesSingletonExist() && ( GetMouseX() != gAppModule.GetCursorX() ||
											  GetMouseY() != gAppModule.GetCursorY() ) )
	{
		SetMousePos( gAppModule.GetCursorX(), gAppModule.GetCursorY() );
	}

	if( GetVisible() && HasTexture() )
	{
		gUIShell.SetActiveCursor( this );
	}
}


/**************************************************************************

Function		: LoadAnimatedTexture()

Purpose			: Load a texture into the window

Returns			: void

**************************************************************************/
void UICursor::LoadAnimatedTexture( gpstring texture, bool bResize )
{
	if ( m_animated_textures.size() != 0 )
	{
		std::vector< unsigned int >::iterator i;
		for ( i = m_animated_textures.begin(); i != m_animated_textures.end(); ++i )
		{
			gUIShell.GetRenderer().DestroyTexture( *i );
		}
	}

	unsigned int framepersec	= 0;
	unsigned int numFrames		= 0;
	m_animated_textures = gUITextureManager.LoadAnimatedTextures( texture, GetInterfaceParent(), framepersec, numFrames, true );
	SetTextureIndex( (m_animated_textures.front()));
	gUIAnimation.AddTextureAnimation( framepersec, numFrames, this );

	if ( bResize == true ) 
	{
		SetRect(	0, gUIShell.GetRenderer().GetTextureWidth( GetTextureIndex() ), 
					0, gUIShell.GetRenderer().GetTextureHeight( GetTextureIndex() ) );
	}	
}



/**************************************************************************

Function		: GetAnimationTextures()

Purpose			: Retrieve a list of the available cursors to animate

Returns			: void

**************************************************************************/
void UICursor::GetAnimationTextures( std::vector< unsigned int > & textures )
{
	textures = m_animated_textures;
}


// Set is visible and set the texture index to the front 
void UICursor::SetVisible( bool bVisible )
{
	UIWindow::SetVisible( bVisible );

	if (( m_animated_textures.size() > 0 ) && !bVisible) {
		SetTextureIndex( (m_animated_textures.front()));
	}
}


void UICursor::Activate()
{
	if ( HasTexture() == false ) 
	{
		ReceiveMessage( UIMessage( MSG_ACTIVATECURSOR ) );
		SetHasTexture( true );
	}
	
	SetVisible( true );
}