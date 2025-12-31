/**************************************************************************

Filename		: ui_dockbar.cpp

Description		: Control for the standard click button

Creation Date	: 9/19/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_dockbar.h"
#include "fuel.h"
#include "rapiowner.h"
#include "stringtool.h"
#include "ui_messenger.h"
#include "ui_animation.h"
#include "ui_statusbar.h"
#include "AppModule.h"


/**************************************************************************

Function		: UIDockbar()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIDockbar::UIDockbar( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
{	
	SetType( UI_TYPE_DOCKBAR );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;
	m_dockbar_type = DOCKBAR_SCREEN;
	gpstring sType;
	m_currentLocation = DL_DEFAULT;
	
	if ( fhWindow.Get( "dockbar_type", sType ) ) {
		if ( sType.same_no_case( "dockbar_screen_x") ) {
			m_dockbar_type = DOCKBAR_SCREEN_X;
		}
		if ( sType.same_no_case( "dockbar_screen_y") ) {
			m_dockbar_type = DOCKBAR_SCREEN_Y;
		}
		if ( sType.same_no_case( "dockbar_screen_x_switch_y" ) ) 
		{
			m_dockbar_type = DOCKBAR_SCREEN_X_SWITCH_Y;
		}
		if ( sType.same_no_case( "dockbar_window" ) ) {
			m_dockbar_type = DOCKBAR_WINDOW;
		}
	}	
}
UIDockbar::UIDockbar()
: UIWindow()
{	
	SetType( UI_TYPE_DOCKBAR );
	SetInputType( TYPE_INPUT_ALL );
	m_buttondown = false;
	m_dockbar_type = DOCKBAR_SCREEN;	
	m_currentLocation = DL_DEFAULT;
}


/**************************************************************************

Function		: ~UIDockbar()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIDockbar::~UIDockbar()
{
}


/**************************************************************************

Function		: ProcessAction()

Purpose			: Process an action on the window

Returns			: bool

**************************************************************************/
bool UIDockbar::ProcessAction( UI_ACTION action, gpstring parameter )
{
	if ( UIWindow::ProcessAction( action, parameter ) == true ) {
		return true;
	}		
	return false;
}


/**************************************************************************

Function		: ProcessMessage()

Purpose			: Process messages specific to the control

Returns			: void

**************************************************************************/
bool UIDockbar::ProcessMessage( UIMessage & msg )
{
	switch ( msg.GetCode() ) 
	{
	case MSG_LBUTTONDOWN:
		{
			m_buttondown = true;
			m_ptStart.x = gUIShell.GetMouseX();
			m_ptStart.y = gUIShell.GetMouseY();		
			m_startRect = GetRect();
		}
		break;
	case MSG_LBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	case MSG_GLOBALLBUTTONUP:
		{
			m_buttondown = false;
		}
		break;
	}	
	return true;
}



void UIDockbar::Dock( bool bAutoDock, int x, int y )
{
	if ( !bAutoDock )
	{
		if ( GetDraggable() && (!GetDragAllowed() || !( GetDragTimeElapsed() >= GetDragTime() )) ) 
		{
			return;
		}
	}
	else
	{
		m_startRect.left = x;
		m_startRect.right = x + (GetRect().right-GetRect().left);
		m_startRect.top = y;
		m_startRect.bottom = y + (GetRect().bottom-GetRect().top);
	}

	int mouseX = GetMouseX();
	int mouseY = GetMouseY();
	if ( bAutoDock )
	{
		m_ptStart.x = x;
		m_ptStart.y = y;

		mouseX = x;
		mouseY = y;
	}

	UIWindowVec group;
	if ( !GetDockGroup().empty() ) {
		group = gUIShell.ListWindowsOfDockGroup( GetDockGroup() );
	}
	UIWindowVec::iterator i;

	switch ( m_dockbar_type ) {
	case DOCKBAR_SCREEN_Y:
		{
			if ( mouseY <= GetScreenHeight()/2 ) {
				if ( GetRect().bottom >= GetScreenHeight()/2 ) {
					for ( i = group.begin(); i != group.end(); ++i ) {
						int height = (*i)->GetRect().bottom - (*i)->GetRect().top;
						(*i)->SetRect(	(*i)->GetRect().left,
										(*i)->GetRect().right,
										GetScreenHeight() - (*i)->GetRect().bottom,
										GetScreenHeight() - (*i)->GetRect().bottom + height );	
												
						if ( (*i)->GetAnchorData().bBottomAnchor )
						{
							(*i)->GetAnchorData().top_anchor = (*i)->GetAnchorData().bottom_anchor - height;
							(*i)->GetAnchorData().bBottomAnchor = false;
							(*i)->GetAnchorData().bTopAnchor = true;
						}
					}

					gUIMessenger.Notify( "y_top_dock_sucessful", this );
					SetDockLocation( DL_TOP );
				}
			}		
			else if ( mouseY >= GetScreenHeight()/2 ) {
				if ( GetRect().top <= GetScreenHeight()/2 ) {
					for ( i = group.begin(); i != group.end(); ++i ) {
						int height = (*i)->GetRect().bottom - (*i)->GetRect().top;
						(*i)->SetRect(	(*i)->GetRect().left,
										(*i)->GetRect().right,
										GetScreenHeight()-height-(*i)->GetRect().top,
										GetScreenHeight()-(*i)->GetRect().top);					

						if ( (*i)->GetAnchorData().bTopAnchor )
						{
							(*i)->GetAnchorData().bottom_anchor = (*i)->GetAnchorData().top_anchor + height;
							(*i)->GetAnchorData().bBottomAnchor = true;
							(*i)->GetAnchorData().bTopAnchor = false;
						}
					}

					gUIMessenger.Notify( "y_bottom_dock_sucessful", this );
					SetDockLocation( DL_BOTTOM ); 
				}
			}			
		}	
		break;
	case DOCKBAR_SCREEN_X_SWITCH_Y:
		{
			UIRect dockRect;
			int xShift = m_startRect.left - m_ptStart.x;
			int yShift = m_startRect.top - m_ptStart.y;
			dockRect.left		= mouseX + xShift;
			dockRect.top		= mouseY + yShift;
			dockRect.right		= dockRect.left + ( m_startRect.right-m_startRect.left );
			dockRect.bottom		= dockRect.top + ( m_startRect.bottom-m_startRect.top );

			bool bContinue = true;
			int index = 1;
			while ( bContinue )
			{
				gpstring sName = GetName();
				sName.erase( sName.size() - 1 );
				sName.appendf( "%d", index );
				if ( !sName.same_no_case( GetName() ) )
				{
					UIWindow * pWindow = gUIShell.FindUIWindow( sName );
					if ( pWindow )
					{
						bool bTop = true;
						if ( dockRect.top < GetRect().top )
						{
							bTop = false;
						}						
						
						int upperBottom = pWindow->GetRect().bottom - (pWindow->GetRect().Height()/2);
						int upperTop	= pWindow->GetRect().top	- (pWindow->GetRect().Height()/2);

						int lowerBottom = pWindow->GetRect().bottom + (pWindow->GetRect().Height()/2);
						int lowerTop	= pWindow->GetRect().top	+ (pWindow->GetRect().Height()/2);

						if (  (( dockRect.top < upperBottom ) && ( dockRect.top > upperTop )) ||
							  (( dockRect.bottom > lowerTop ) && ( dockRect.bottom < lowerBottom )) )
						{
							if (( !bTop && lowerTop > dockRect.top ) ||
								( bTop && upperTop < dockRect.top ))
							{
								UIWindowVec groupSwitch;
								UIWindowVec::iterator iSwitch;
								if ( !pWindow->GetDockGroup().empty() ) 
								{									
									int yDiff = pWindow->GetRect().top - GetRect().top;								

									groupSwitch = gUIShell.ListWindowsOfDockGroup( pWindow->GetDockGroup() );								
									for ( iSwitch = groupSwitch.begin(); iSwitch != groupSwitch.end(); ++iSwitch )
									{							
										(*iSwitch)->SetRect( (*iSwitch)->GetRect().left,
															 (*iSwitch)->GetRect().right,
															 (*iSwitch)->GetRect().top - yDiff,
															 (*iSwitch)->GetRect().top - yDiff + ( (*iSwitch)->GetRect().bottom-(*iSwitch)->GetRect().top) );
									}

									for ( i = group.begin(); i != group.end(); ++i ) 
									{									
										(*i)->SetRect(	(*i)->GetRect().left,
														(*i)->GetRect().right,
														(*i)->GetRect().top + yDiff,
														(*i)->GetRect().top + yDiff + ( (*i)->GetRect().bottom-(*i)->GetRect().top ) );					
									}

									gUIMessenger.Notify( "x_switch_y_dock_sucessful", this );
								}							
							}
						}
					}
					else
					{
						bContinue = false;
					}
				}				
				index++;
			}
		}
		break;
	case DOCKBAR_SCREEN_X:
		{
			if ( mouseX <= GetScreenWidth()/2 ) {
				if ( GetRect().right >= GetScreenWidth()/2 ) {
					for ( i = group.begin(); i != group.end(); ++i ) {						
						
						UIStatusBar * pStatus = (UIStatusBar *)*i;	
						if ( (*i)->GetType() == UI_TYPE_STATUSBAR && pStatus->GetEdge() == EDGE_RIGHT )
						{
							
							int width = (*i)->GetRect().left+(int)pStatus->GetMaxSize() - (*i)->GetRect().left;							
							pStatus->SetRect(	GetScreenWidth()-width-(*i)->GetRect().left,
												GetScreenWidth()-(*i)->GetRect().left,
												(*i)->GetRect().top,
												(*i)->GetRect().bottom );
							pStatus->SetStatus( pStatus->GetStatus() );
						}
						else
						{
							int width = (*i)->GetRect().right - (*i)->GetRect().left;
							(*i)->SetRect(	GetScreenWidth() - (*i)->GetRect().right,
											GetScreenWidth() - (*i)->GetRect().right + width,
											(*i)->GetRect().top,
											(*i)->GetRect().bottom );
						}						
					}

					gUIMessenger.Notify( "x_left_dock_sucessful", this );
					SetDockLocation( DL_LEFT );
				}
			}		
			else if ( mouseX >= GetScreenWidth()/2 ) {
				if ( GetRect().left <= GetScreenWidth()/2 ) {
					for ( i = group.begin(); i != group.end(); ++i ) {
						
						UIStatusBar * pStatus = (UIStatusBar *)*i;
						if ( (*i)->GetType() == UI_TYPE_STATUSBAR && pStatus->GetEdge() == EDGE_RIGHT )
						{							
							int width = (*i)->GetRect().left+(int)pStatus->GetMaxSize() - (*i)->GetRect().left;							
							pStatus->SetRect(	GetScreenWidth()-width-(*i)->GetRect().left,
												GetScreenWidth()-(*i)->GetRect().left,
												(*i)->GetRect().top,
												(*i)->GetRect().bottom );	
							pStatus->SetStatus( pStatus->GetStatus() );
						}
						else
						{
							int width = (*i)->GetRect().right - (*i)->GetRect().left;
							(*i)->SetRect(	GetScreenWidth()-width-(*i)->GetRect().left,
											GetScreenWidth()-(*i)->GetRect().left,
											(*i)->GetRect().top,
											(*i)->GetRect().bottom );															
						}
					}

					gUIMessenger.Notify( "x_right_dock_sucessful", this );
					SetDockLocation( DL_RIGHT );
				}
			}	
		}
		break;
	}
}


void UIDockbar::ForceDock( int x, int y )
{
	if ( m_dockbar_type == DOCKBAR_SCREEN_X_SWITCH_Y )
	{
		UNREFERENCED_PARAMETER( x );

		int yDiff = GetRect().top - y;

		UIWindowVec groupSwitch;
		UIWindowVec::iterator iSwitch;
		groupSwitch = gUIShell.ListWindowsOfDockGroup( GetDockGroup() );								
		for ( iSwitch = groupSwitch.begin(); iSwitch != groupSwitch.end(); ++iSwitch )
		{							
			(*iSwitch)->SetRect( (*iSwitch)->GetRect().left,
								 (*iSwitch)->GetRect().right,
								 (*iSwitch)->GetRect().top - yDiff,
								 (*iSwitch)->GetRect().top - yDiff + ( (*iSwitch)->GetRect().bottom-(*iSwitch)->GetRect().top) );
		}					
	}
}


/**************************************************************************

Function		: SetMousePos()

Purpose			: Sets the current mouse position and adjusts the cursor
				  rectangle accordingly

Returns			: void

**************************************************************************/
void UIDockbar::SetMousePos( unsigned int x, unsigned int y )
{
	UIWindow::SetMousePos( x, y );
	if ( m_buttondown ) 
	{
		if ( !gAppModule.GetLButton() )
		{
			SetDragAllowed( false );
			return;
		}

		Dock();
	}		
}


#if !GP_RETAIL
void UIDockbar::Save( FuelHandle hSave, bool bChild )
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
			hWindow->SetType( "dockbar" );

			switch ( m_dockbar_type )
			{
			case DOCKBAR_SCREEN_X:
				hWindow->Set( "dockbar_type", "dockbar_screen_x" ); 
				break;
			case DOCKBAR_SCREEN_Y:
				hWindow->Set( "dockbar_type", "dockbar_screen_y" ); 
				break;
			case DOCKBAR_WINDOW:
				hWindow->Set( "dockbar_type", "dockbar_window" ); 
				break;
			}
		}
	}
}
#endif


void UIDockbar::Draw()
{
	if ( GetVisible() )
	{
		if ( GetTextureIndex() != 0 )
		{
			UIWindow::DrawComponent( GetRect(), GetTextureIndex(), GetAlpha(), GetUVRect(), !GetTiledTexture() );
		}
		
		if ( m_buttondown )
		{
			UIRect colorRect;
			int xShift = m_startRect.left - m_ptStart.x;
			int yShift = m_startRect.top - m_ptStart.y;

			int mouseX = gUIShell.GetMouseX();
			int mouseY = gUIShell.GetMouseY();			

			colorRect.left		= mouseX + xShift;
			colorRect.top		= mouseY + yShift;
			colorRect.right		= colorRect.left + ( m_startRect.right-m_startRect.left );
			colorRect.bottom	= colorRect.top + ( m_startRect.bottom-m_startRect.top );

			if ( m_dockbar_type == DOCKBAR_SCREEN_X_SWITCH_Y )
			{
				colorRect.left = GetRect().left;
				colorRect.right = GetRect().right;
			}

			if ( GetDragAllowed() && GetDraggable() && ( GetDragTimeElapsed() >= GetDragTime() )  ) 
			{
				gUIShell.InsertDragBox( colorRect, 0x5500FF00 );
			}
		}

		UIWindow::DrawDragBox();
	}
}