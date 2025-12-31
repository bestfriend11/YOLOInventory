/**************************************************************************

Filename		: ui_shell.cpp

Description		: This is the main shell for the user interface.  The
				  shell is what talks to the game and the game talks to
			      the shell.  All window information is stored and maintained
				  by the shell.

Creation Date	: 9/11/99

**************************************************************************/


// Include Files
#include "precomp_gamegui.h"
#include "fuel.h"
#include "imeui.h"
#include "inputbinder.h"
#include "rapiowner.h"
#include "appmodule.h"
#include "LocHelp.h"
#include "ui_animation.h"
#include "ui_button.h"
#include "ui_checkbox.h"
#include "ui_combobox.h"
#include "ui_cursor.h"
#include "ui_dockbar.h"
#include "ui_editbox.h"
#include "ui_emotes.h"
#include "ui_gridbox.h"
#include "ui_infoslot.h"
#include "ui_item.h"
#include "ui_itemslot.h"
#include "ui_line.h"
#include "ui_listbox.h"
#include "ui_mouse_listener.h"
#include "ui_messenger.h"
#include "ui_multistagebutton.h"
#include "ui_popupmenu.h"
#include "ui_radiobutton.h"
#include "ui_shell.h"
#include "ui_slider.h"
#include "ui_statusbar.h"
#include "ui_text.h"
#include "ui_textureman.h"
#include "ui_window.h"
#include "ui_listreport.h"
#include "ui_chatbox.h"
#include "ui_textbox.h"
#include "ui_dialogbox.h"
#include "ui_tab.h"
#include "gpprofiler.h"


// Input Method Editor (IME) Callbacks
void	GameGuiCallback_SetFont(IMEUI_FONT hfont);
void	GameGuiCallback_SetFontAttribute(UINT uHeight, bool bBold);
void	GameGuiCallback_SetTextColor(DWORD color);
void	GameGuiCallback_SetTextPosition(int x, int y);
void	GameGuiCallback_DrawText(const char* pszText );
void	GameGuiCallback_GetStringDimension(const char* szText, DWORD* puWidth, DWORD* puHeight);
void	GameGuiCallback_DrawRect( int x1, int y1, int x2, int y2, DWORD color );
void	GameGuiCallback_DrawFans(IMEUI_VERTEX* paVertex, UINT uNum);


/**************************************************************************

Function		: UIShell()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIShell::UIShell( Rapi& rapi, int screenWidth, int screenHeight )
: m_rapi( rapi )
, m_screenWidth( screenWidth )
, m_screenHeight( screenHeight )
, m_mouseX( 0 )
, m_mouseY( 0 )
, m_bVisible( true )
, m_bRolloverConsumed( false )
, m_lbuttondown( false )
, m_rbuttondown( false )
, m_rolloverItemID( 0 )
, m_deltaX( 0 )
, m_deltaY( 0 )
, m_bProcessDeletion( false )
, m_bGridPlace( true )
, m_bShowToolTips( true )
, m_bItemActive( false )
, m_bDrawCursors( false )
, m_bImeToggle( true )
, m_pActiveCursor( NULL ) 
, m_bShuttingDown( false )
, m_dragTolerance( 0 )
, m_mouseSettled( 0 )
, m_bDisabled( false )
, m_bIgnoreItemTexture( false )
, m_RolloverDelay( 0.25f )
{
	m_pTextureman	= new UITextureManager( &m_rapi );
	m_pMessenger	= new Messenger();
	m_pAnimation	= new UIAnimation();
	m_pEmotes		= new UIEmotes();
	m_pMessenger->SetUIWindows( &m_UIInterfaces );
	m_pAnimation->SetUIWindows( &m_UIInterfaces );

#if _USE_INPUT_BINDER_
	m_pInputBinder	= new InputBinder( "gui_library" );
#else
	m_pInputBinder	= NULL;
#endif

	// Publish information to the Input binder
	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();

	GetTextureman()->SetInvalidTexture( "b_gui_s_notfound" );

	if ( screenWidth == 0 || screenHeight == 0 )
	{
		screenWidth = gAppModule.GetGameWidth();
		screenHeight = gAppModule.GetGameHeight();
	}
	SetScreenDimensions( screenWidth, screenHeight );
	LoadCommonArt();

	FastFuelHandle hTooltips( "ui:tooltips" );
	if ( hTooltips )
	{
		hTooltips.Get( "default_tooltip_gui", m_sDefaultTooltipName );
	}

	FastFuelHandle hLineSpacers( "config:global_settings:gui_linespacer_map" );
	if ( hLineSpacers )
	{
		FastFuelFindHandle hFind( hLineSpacers );	
		if ( hFind.FindFirstKeyAndValue() )
		{
			gpstring sKey, sValue;
			while ( hFind.GetNextKeyAndValue( sKey, sValue ) )
			{
				int spacer = 0;
				stringtool::Get( sValue, spacer );
				m_lineSpacers.insert( std::make_pair( sKey, spacer ) );
			}
		}		
	}

	FastFuelHandle hGuiMisc( "config:global_settings:gui_misc" );
	if ( hGuiMisc )
	{
		hGuiMisc.Get( "item_drag_tolerance", m_dragTolerance );
		hGuiMisc.Get( "item_rollover_delay", m_RolloverDelay );
	}

//	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		InitImeUi();
	}
}


/**************************************************************************

Function		: ~UIShell()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIShell::~UIShell()
{
	SetIsShuttingDown( true );

	if ( m_pMessenger ) 
	{
		delete m_pMessenger;
	}

	for ( GUInterfaceVec::iterator j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		for ( UIWindowMap::iterator i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
		{
			delete (*i).second;
		}
		(*j)->ui_windows.clear();
		delete (*j);
	}
	m_UIInterfaces.clear();
	UnloadCommonArt();

	m_activeItems.clear();

	if ( m_pTextureman	) 
	{
		delete m_pTextureman;
	}

#if _USE_INPUT_BINDER_
	if ( m_pInputBinder ) {
		delete m_pInputBinder;
	}
#endif

	if ( m_pAnimation ) 
	{
		delete m_pAnimation;
	}

	if ( m_pEmotes )
	{
		delete m_pEmotes;
	}
}


void UIShell::SetScreenDimensions( int screenWidth, int screenHeight )
{
	m_screenWidth	= screenWidth;
	m_screenHeight	= screenHeight;
	m_drawItems.clear();

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		ImeUi_SetScreenDimension( (unsigned int)screenWidth, (unsigned int)screenHeight );
	}
}


/**************************************************************************

Function		: ActivateInterface()

Purpose			: Loads a UI block from Fuel and instantiates the interface

Returns			: void

**************************************************************************/
void UIShell::ActivateInterface( FastFuelHandle fhInterface )
{
	// If interface is already loaded, don't load it again
	GUInterfaceVec::iterator j;

	bool bReactivate = false;
	GUInterface * ui_interface = NULL;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		if ( fhInterface.IsValid() && (*j)->name.same_no_case( fhInterface.GetName() ) )
		{
			if ( (*j)->bInactive )
			{
				ui_interface = *j;
				bReactivate = true;
				break;
			}
			else
			{				
				ShowInterface( (*j)->name );
				return;
			}
		}
	}

	if ( fhInterface.IsDirectory() )
	{
		fhInterface = fhInterface.GetChildNamed( fhInterface.GetName() );
		gpassertf( fhInterface.IsValid(), ( "Attempting to activate an interface which is named differently than its parent directory: %s", fhInterface.GetName() ) );
	}

	if ( !fhInterface.IsValid() )
	{
		return;
	}
	
	if ( !bReactivate )
	{
		ui_interface = new GUInterface;
	}
	
	ui_interface->bVisible		= true;
	ui_interface->bDeactivate	= false;
	ui_interface->height		= 0;
	ui_interface->width			= 0;
	ui_interface->xoffset		= 0;
	ui_interface->yoffset		= 0;
	ui_interface->resWidth		= m_screenWidth;
	ui_interface->resHeight		= m_screenHeight;
	ui_interface->bHasTooltips	= false;	
	ui_interface->name			= fhInterface.GetName();
	ui_interface->bInactive		= false;
	ui_interface->bPassiveModal	= false;
	
	fhInterface.Get( "modal", ui_interface->bModal );
	fhInterface.Get( "passive_modal", ui_interface->bPassiveModal );
	if ( !fhInterface.Get( "centered", ui_interface->sCenter ) )
	{
		if ( fhInterface.Get( "centered_x", ui_interface->sCenter ) )
		{
			ui_interface->bCenteredX = true;
		}
		else if ( fhInterface.Get( "centered_y", ui_interface->sCenter ) )
		{
			ui_interface->bCenteredY = true;
		}
	}
	fhInterface.Get( "centered_y", ui_interface->sCenter );
	fhInterface.Get( "interface_type", ui_interface->interface_type );
	fhInterface.Get( "intended_resolution_width", ui_interface->resWidth );
	fhInterface.Get( "intended_resolution_height", ui_interface->resHeight );	

	if ( !bReactivate )
	{
		m_UIInterfaces.push_back( ui_interface );
	}

	// check for tooltips in this interface
	FastFuelHandle hTooltips( gpstring( "ui:tooltips:" ).append( fhInterface.GetName() ) );
	if ( hTooltips.IsValid() )
	{
		ui_interface->bHasTooltips = true;
	}

	// List the main windows
	FastFuelHandleColl fhlChildren;
	fhInterface.ListChildren( fhlChildren );
	FastFuelHandleColl::iterator i;
	for ( i = fhlChildren.begin(); i != fhlChildren.end(); ++i ) 
	{
		Instantiate( *i, ui_interface );
	}

	InitializeInterfaceType( ui_interface, ui_interface->interface_type, fhInterface );

	if (( ui_interface->resHeight != 0 ) && ( ui_interface->resWidth != 0 ))
	{
		NormalizeInterfaceToResolution( ui_interface->name );
	}

	if ( !ui_interface->sCenter.empty() )
	{
		CenterInterface( *ui_interface );
	}

	// Send the initialization message to all the windows
	m_pMessenger->SendUIMessageToInterface( UIMessage( MSG_CREATED ), ui_interface->name );
}


void UIShell::NormalizeInterfaceToResolution( gpstring sInterface )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		if ( (*j)->name.same_no_case( sInterface ) )
		{
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) 
			{
				(*i).second->ResizeToCurrentResolution( (*j)->resWidth, (*j)->resHeight );
			}
		}
	}
}


void UIShell::CenterInterface( GUInterface & gui )
{
	UIWindowMap::iterator i;
	UIWindow * pWindow = FindUIWindow( gui.sCenter, gui.name );
	if ( pWindow )
	{
		int center_x = m_screenWidth / 2;
		int center_y = m_screenHeight / 2;
		int width_center = (pWindow->GetRect().right - pWindow->GetRect().left)/2;
		int height_center = (pWindow->GetRect().bottom - pWindow->GetRect().top)/2;

		UIRect oldRect = pWindow->GetRect();

		if ( gui.bCenteredX )
		{
			pWindow->SetRect(	center_x - width_center, center_x + width_center,
								pWindow->GetRect().top, pWindow->GetRect().bottom );
		}
		else if ( gui.bCenteredY )
		{
			pWindow->SetRect(	pWindow->GetRect().left, pWindow->GetRect().right,
								center_y - height_center, center_y + height_center );
		}
		else
		{
			pWindow->SetRect(	center_x - width_center, center_x + width_center,
								center_y - height_center, center_y + height_center );
		}

		int x_diff = pWindow->GetRect().left - oldRect.left;
		int y_diff = pWindow->GetRect().top - oldRect.top;

		for ( i = gui.ui_windows.begin(); i != gui.ui_windows.end(); ++i ) 
		{
			if ( (*i).second == pWindow )
			{
				continue;
			}

			if ( !(*i).second->GetNormalizeResize() )
			{
				(*i).second->SetRect(	(*i).second->GetRect().left + x_diff,
										(*i).second->GetRect().right + x_diff,
										(*i).second->GetRect().top + y_diff,
										(*i).second->GetRect().bottom + y_diff );

				(*i).second->Resize( GetScreenWidth(), GetScreenHeight() );				
			}

			if ( (*i).second->GetType() == UI_TYPE_SLIDER )
			{
				((UISlider *)(*i).second)->CalculateSlider();
			}
		}
	}
}


/**************************************************************************

Function		: DeactivateInterface()

Purpose			: Deactivates a loaded interface

Returns			: void

**************************************************************************/
void UIShell::DeactivateInterface( const gpstring& interface_name )
{
	gpassertm( !m_pMessenger->IsProcessingMessage(), "DeactivateInterface() should not be called as the direct result of a processing message." );

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		if ( interface_name.same_no_case( (*j)->name ) )
		{		
			bool bModal = (*j)->bVisible && (*j)->bModal;

			m_pAnimation->RemoveInterfaceAnimations( (*j) );
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
			{
				if ( bModal )
				{
					m_pMessenger->Notify( "modal_inactive", (*i).second );
					bModal = false;
				}				
				m_pMessenger->SendUIMessage( UIMessage( MSG_DESTROYED ), (*i).second );
				delete (*i).second;
			}
			delete (*j);
			(*j) = 0;
			m_UIInterfaces.erase( j );
			break;
		}
	}

	ClearItems();
}


void UIShell::DeactivateInterfaceWindows( const gpstring& sInterface )
{
	gpassertm( !m_pMessenger->IsProcessingMessage(), "DeactivateInterfaceWindows() should not be called as the direct result of a processing message." );

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		if ( sInterface.same_no_case( (*j)->name ) )
		{		
			bool bModal = (*j)->bVisible && (*j)->bModal;

			m_pAnimation->RemoveInterfaceAnimations( (*j) );
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
			{
				if ( bModal )
				{
					m_pMessenger->Notify( "modal_inactive", (*i).second );
					bModal = false;
				}				
				m_pMessenger->SendUIMessage( UIMessage( MSG_DESTROYED ), (*i).second );
				delete (*i).second;
			}

			(*j)->ui_windows.clear();
			(*j)->ui_draw_windows.clear();		
			(*j)->groupMap.clear();
			(*j)->bModal			= false;
			(*j)->bVisible			= false;
			(*j)->bDeactivate		= false;
			(*j)->width				= 0;
			(*j)->height			= 0;
			(*j)->xoffset			= 0;
			(*j)->yoffset			= 0;
			(*j)->resWidth			= 0;
			(*j)->resHeight			= 0;
			(*j)->bHasTooltips		= false;			
			(*j)->bCenteredX		= false;
			(*j)->bCenteredY		= false;
			(*j)->bInactive			= true;

			break;
		}		
	}
}


void UIShell::MarkInterfaceForDeactivation( const gpstring& interface_name )
{
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		if ( interface_name.same_no_case( (*j)->name ) ) 
		{
			(*j)->bDeactivate = true;
		}
	}

	ActivateVec::iterator iInterface;
	for ( iInterface = m_ActivateInterfaces.begin(); iInterface != m_ActivateInterfaces.end(); ++iInterface )
	{
		unsigned int pos = (*iInterface).first.find_last_of( ":" );
		if ( pos != gpstring::npos )
		{
			gpstring sInterface = (*iInterface).first.substr( pos+1, (*iInterface).first.size()-pos+1 );
			if ( sInterface.same_no_case( interface_name ) )
			{
				m_ActivateInterfaces.erase( iInterface );
				return;
			}
		}
	}	
}


void UIShell::UnMarkInterfaceForDeactivation( const gpstring& sInterface )
{
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		if ( sInterface.same_no_case( (*j)->name ) ) 
		{
			(*j)->bDeactivate = false;
		}
	}
}

void UIShell::MarkInterfaceForActivation( const gpstring& interface_name, bool bShow )
{
	GUInterfaceVec::iterator j;
	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		unsigned int pos = interface_name.find_last_of( ":" );
		if ( pos != gpstring::npos )
		{
			gpstring sInterface = interface_name.substr( pos+1, interface_name.size()-pos+1 );
			if ( sInterface.same_no_case( (*j)->name ) ) 
			{
				(*j)->bDeactivate = false;
				return;
			}
		}
	}

	m_ActivateInterfaces.push_back( std::pair< gpstring, bool >( interface_name, bShow ) );	
}


/**************************************************************************

Function		: ShowInterface()

Purpose			: Show or hide an interface

Returns			: void

**************************************************************************/
void UIShell::ShowInterface( const gpstring& interface_name )
{
	GUInterfaceVec::iterator j;
	UIWindowMap::iterator i;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		if ( interface_name.same_no_case( (*j)->name ) ) 
		{
			bool bModal = (*j)->bModal;
			if ( bModal )
			{
				ImeUi_EnableIme( false );
			}

			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
			{
				m_pMessenger->SendUIMessage( UIMessage( MSG_SHOW ), (*i).second );

				if ( bModal && !(*j)->bPassiveModal )
				{
					m_pMessenger->Notify( "modal_active", (*i).second );
					bModal = false;
				}
			}

			(*j)->bVisible = true;
		}
	}
}




void UIShell::HideInterface( const gpstring& interface_name )
{
	GUInterfaceVec::iterator j;
	UIWindowMap::iterator i;

	int numModals = 0;
	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		bool bModal = (*j)->bVisible && (*j)->bModal;
		if ( bModal )
		{
			numModals++;
		}
	}

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{		
		if ( interface_name.same_no_case( (*j)->name ) )
		{
			bool bModal = (*j)->bVisible && (*j)->bModal;

			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
			{
				m_pMessenger->SendUIMessage( UIMessage( MSG_HIDE ), (*i).second );

				if ( bModal && numModals == 1 )
				{
					m_pMessenger->Notify( "modal_inactive", (*i).second );
					bModal = false;
				}
			}
			(*j)->bVisible = false;
		}
	}
}




/**************************************************************************

Function		: ShowGroup()

Purpose			: Show or hide a group

Returns			: void

**************************************************************************/
void UIShell::ShowGroup( const char * szGroup, bool bShow, bool bMessage, const char * szInterface  )
{
	UIWindowVec::iterator i;
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
	{
		bool bFound = true;
		if ( szInterface != NULL )
		{
			if ( !(*j)->name.same_no_case( szInterface ) )
			{
				bFound = false;
			}
		}

		if ( bFound )
		{
			UIGroupMap::iterator iFind = (*j)->groupMap.find( szGroup );
			if ( iFind != (*j)->groupMap.end() )
			{
				for ( i = (*iFind).second.begin(); i != (*iFind).second.end(); ++i ) 
				{
					if ( (*i)->GetGroup().same_no_case( szGroup ) ) 
					{
						if ( bMessage )
						{
							bShow ? gUIMessenger.SendUIMessage( UIMessage( MSG_VISIBLE ), (*i) ) :
									gUIMessenger.SendUIMessage( UIMessage( MSG_INVISIBLE ), (*i) );
						}

						(*i)->SetVisible( bShow );
					}
				}
			}

			if ( szInterface != NULL )
			{
				return;
			}
		}
	}		
}


/**************************************************************************

Function		: Instantiate()

Purpose			: Recursively instantiates the content from the fuel file

Returns			: void

**************************************************************************/
UIWindow * UIShell::Instantiate( FastFuelHandle fhBlock, GUInterface *parent_interface, UIWindow *pParent, gpstring sName )
{
	// Create the window based on its type
	const char* szType = fhBlock.GetType();

	// Ignore types which are components of controls
	if( same_no_case( szType, "message" ) )
	{
		return 0;
	}

	// Instantiate the windows dependent on which type they are
	UIWindow *ui_window = 0;
	UI_CONTROL_TYPE uiType;
	if ( ::FromGasString( szType, uiType ) )
	{
		switch ( uiType )
		{
		case UI_TYPE_WINDOW:
			ui_window = new UIWindow( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_BUTTON:
			ui_window = new UIButton( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_CHECKBOX:
			ui_window = new UICheckbox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_SLIDER:
			ui_window = new UISlider( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_LISTBOX:
			ui_window = new UIListbox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_RADIOBUTTON:
			ui_window = new UIRadioButton( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_MULTISTAGEBUTTON:
			ui_window = new UIMultiStageButton(	fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_TEXT:
			ui_window = new UIText( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_CURSOR:
			ui_window = new UICursor( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_DOCKBAR:
			ui_window = new UIDockbar( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_GRIDBOX:
			ui_window = new UIGridbox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_POPUPMENU:
			ui_window = new UIPopupMenu( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_ITEM:
			ui_window = new UIItem( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_ITEMSLOT:
			ui_window = new UIItemSlot( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_INFOSLOT:
			ui_window = new UIInfoSlot( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_STATUSBAR:
			ui_window = new UIStatusBar( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_EDITBOX:
			ui_window = new UIEditBox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_COMBOBOX:
			ui_window = new UIComboBox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_MOUSE_LISTENER:
			ui_window = new UIMouseListener( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_LISTREPORT:
			ui_window = new UIListReport( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_CHATBOX:
			ui_window = new UIChatBox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_TEXTBOX:
			ui_window = new UITextBox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_DIALOGBOX:
			ui_window = new UIDialogBox( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_TAB:
			ui_window = new UITab( fhBlock, parent_interface->name, pParent );
			break;

		case UI_TYPE_LINE:
			ui_window = new UILine( fhBlock, parent_interface->name, pParent );
			break;

		default:
			gpassert( 0 );	// $ should never get here
		}
	}

	if ( ui_window != 0 )
	{
		if ( !sName.empty() )
		{
			ui_window->SetName( sName );
		}

		if ( FindUIWindow( ui_window->GetName(), parent_interface->name ) != 0 )
		{
			gperrorf(("Window with name: %s already exists in interface: %s", ui_window->GetName().c_str(), parent_interface->name.c_str() ));
		}

		if ( parent_interface->bHasTooltips )
		{
			FastFuelHandle hTooltips( gpstring( "ui:tooltips:" ).append( parent_interface->name ) );
			if ( hTooltips )
			{
				gpstring sTextBox;
				gpwstring sHelpText;

				FastFuelHandle hTips = hTooltips.GetChildNamed( "tips" );
				if ( hTips )
				{
					hTooltips.Get( "tip_textbox", sTextBox );
					if ( sTextBox.empty() )
					{
						sTextBox = GetDefaultTooltipName();
					}
					if ( hTips.Get( ui_window->GetName(), sHelpText ) && !sHelpText.empty() )
					{
						if ( sHelpText[ 0 ] == '@' )
						{
							FastFuelHandle hText = hTooltips.GetChildNamed( "text" );
							if ( hText )
							{
								hText.Get( ToAnsi( sHelpText.right( sHelpText.size() - 1 ) ).c_str(), sHelpText );
							}
						}
						ui_window->SetToolTip( sHelpText, sTextBox, true );
					}
				}
				else
				{
					FastFuelHandle hHelp = hTooltips.GetChildNamed( "help" );
					if ( hHelp )
					{
						hTooltips.Get( "help_textbox", sTextBox );
						gpassertf( !sTextBox.empty(), ( "Assigning help text with an invalid help_textbox. interface = %s, window = %s.", parent_interface->name.c_str(), ui_window->GetName().c_str() ));
						if ( hHelp.Get( ui_window->GetName(), sHelpText ) && !sHelpText.empty() )
						{
							if ( sHelpText[ 0 ] == '@' )
							{
								FastFuelHandle hText = hTooltips.GetChildNamed( "text" );
								if ( hText )
								{
									hText.Get( ToAnsi( sHelpText.right( sHelpText.size() - 1 ) ).c_str(), sHelpText );
								}
							}
							ui_window->SetToolTip( sHelpText, sTextBox, false );
						}
					}
				}
			}
		}

		if ( !ui_window->GetGroup().empty() )
		{
			UIGroupMap::iterator iFind = parent_interface->groupMap.find( ui_window->GetGroup() );
			if ( iFind == parent_interface->groupMap.end() )
			{
				UIWindowVec windows;
				windows.push_back( ui_window );
				parent_interface->groupMap.insert( std::make_pair( ui_window->GetGroup(), windows ) );
			}
			else
			{
				(*iFind).second.push_back( ui_window );
			}
		}

		if ( ui_window->GetType() == UI_TYPE_EDITBOX )
		{
			int tabStop = ((UIEditBox *)ui_window)->GetTabStop();
			if ( tabStop >= 0 )
			{
				parent_interface->tabStops.insert( UITabStopMap::value_type( tabStop, ui_window ) );
			}
		}

		parent_interface->ui_windows.insert( UIWindowPair( ui_window->GetName(), ui_window ));
		parent_interface->ui_draw_windows.insert( UIWindowDrawPair( ui_window->GetDrawOrder(), ui_window ));
		ui_window->Resize( m_screenWidth, m_screenHeight );
	}

	// Instantiate children, if any
	FastFuelHandleColl fhlChildren;
	fhBlock.ListChildren( fhlChildren );
	FastFuelHandleColl::iterator i;
	for ( i = fhlChildren.begin(); i != fhlChildren.end(); ++i ) 
	{
		Instantiate( *i, parent_interface, ui_window );
	}

	return ui_window;
}


void UIShell::AddWindowToInterface( UIWindow *pWindow, const gpstring& sInterface, bool bExistFail )
{
	if ( pWindow != 0 ) {

		GUInterfaceVec::iterator j;
		for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {

			if ( (*j)->name.same_no_case( sInterface ) ) {

				if ( bExistFail )
				{
					if ( FindUIWindow( pWindow->GetName(), sInterface ) != 0 ) {
						gperrorf(("Window with name: %s already exists in interface: %s", pWindow->GetName().c_str(), sInterface.c_str() ));
					}
				}

				(*j)->ui_windows.insert( UIWindowPair( pWindow->GetName(), pWindow ));
				(*j)->ui_draw_windows.insert( UIWindowDrawPair( pWindow->GetDrawOrder(), pWindow ));
				pWindow->Resize( m_screenWidth, m_screenHeight );
			}
		}
	}
}


UIWindow * UIShell::CreateDefaultWindowOfType( UI_CONTROL_TYPE type )
{
	UIWindow * pWindow = 0;
	switch ( type )
	{
	case UI_TYPE_WINDOW:
		{
			pWindow = (UIWindow *)( new UIWindow() );
		}
		break;
	case UI_TYPE_BUTTON:
		{
			pWindow = (UIWindow *)( new UIButton() );
		}
		break;
	case UI_TYPE_CHECKBOX:
		{
			pWindow = (UIWindow *)( new UICheckbox() );
		}
		break;
	case UI_TYPE_SLIDER:
		{
			pWindow = (UIWindow *)( new UISlider() );
		}
		break;
	case UI_TYPE_LISTBOX:
		{
			pWindow = (UIWindow *)( new UIListbox() );
		}
		break;
	case UI_TYPE_RADIOBUTTON:
		{
			pWindow = (UIWindow *)( new UIRadioButton() );
		}
		break;
	case UI_TYPE_MULTISTAGEBUTTON:
		{
			pWindow = (UIWindow *)( new UIMultiStageButton() );
		}
		break;
	case UI_TYPE_TEXT:
		{
			pWindow = (UIWindow *)( new UIText() );
		}
		break;
	case UI_TYPE_CURSOR:
		{
			pWindow = (UIWindow *)( new UICursor() );
		}
		break;
	case UI_TYPE_DOCKBAR:
		{
			pWindow = (UIWindow *)( new UIDockbar() );
		}
		break;
	case UI_TYPE_GRIDBOX:
		{
			pWindow = (UIWindow *)( new UIGridbox() );
		}
		break;
	case UI_TYPE_POPUPMENU:
		{
			pWindow = (UIWindow *)( new UIPopupMenu() );
		}
		break;
	case UI_TYPE_ITEM:
		{
			pWindow = (UIWindow *)( new UIItem() );
		}
		break;
	case UI_TYPE_ITEMSLOT:
		{
			pWindow = (UIWindow *)( new UIItemSlot() );
		}
		break;
	case UI_TYPE_INFOSLOT:
		{
			pWindow = (UIWindow *)( new UIInfoSlot() );
		}
		break;
	case UI_TYPE_STATUSBAR:
		{
			pWindow = (UIWindow *)( new UIStatusBar() );
		}
		break;
	case UI_TYPE_EDITBOX:
		{
			pWindow = (UIWindow *)( new UIEditBox() );
		}
		break;
	case UI_TYPE_COMBOBOX:
		{
			pWindow = (UIWindow *)( new UIComboBox() );
		}
		break;
	case UI_TYPE_MOUSE_LISTENER:
		{
			pWindow = (UIWindow *)( new UIMouseListener() );
		}
		break;
	case UI_TYPE_LISTREPORT:
		{
			pWindow = (UIWindow *)( new UIListReport() );
		}
		break;
	case UI_TYPE_CHATBOX:
		{
			pWindow = (UIWindow *)( new UIChatBox() );
		}
		break;
	case UI_TYPE_TEXTBOX:
		{
			pWindow = (UIWindow *)( new UITextBox()	);
		}
		break;
	case UI_TYPE_DIALOGBOX:
		{
			pWindow = (UIWindow *)( new UIDialogBox() );
		}
		break;
	case UI_TYPE_TAB:
		{
			pWindow = (UIWindow *)( new UITab() );
		}
		break;
	}
	return pWindow;
}
UIWindow * UIShell::CreateDefaultWindowOfType( const gpstring& sType )
{
	UIWindow * pWindow = 0;

	UI_CONTROL_TYPE uiType;
	if ( FromGasString( sType, uiType ) )
	{
		pWindow = CreateDefaultWindowOfType( uiType );
	}

	return pWindow;
}


/**************************************************************************

Function		: Update()

Purpose			: Updates the UI ( animations, etc. )

Returns			: void

**************************************************************************/
void UIShell::Update( double seconds, int cursorX, int cursorY )
{
	GPPROFILERSAMPLE( "UIShell :: Update", SP_UI );

	StringVec deactivateVec;

	// See if the cursor moved

	if ( m_deltaX != 0 || m_deltaY != 0 )
	{
		m_bRolloverConsumed = Input_OnMouseMove( cursorX, cursorY );
		m_deltaX = 0;
		m_deltaY = 0;
	}
	else
	{
		m_mouseSettled++;
	}

	// Update drag windows
	{
		UIWindowMap::iterator				i;
		GUInterfaceVec::reverse_iterator	j;
		GUInterfaceVec::reverse_iterator	k;
		bool bModal = false;
		for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); ++k )
		{
			if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ))
			{
				bModal = true;
				for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
				{
					(*i).second->SetDragTimeElapsed( (*i).second->GetDragTimeElapsed() + seconds );
					(*i).second->Update( seconds );
				}
			}

			if ( (*k)->bDeactivate )
			{
				deactivateVec.push_back( (*k)->name );
			}
		}

		if ( bModal == false )
		{
			for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j ) {

				if ( (*j)->bVisible == true )
				{
					for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
					{
						(*i).second->SetDragTimeElapsed( (*i).second->GetDragTimeElapsed() + seconds );
						(*i).second->Update( seconds );
					}
				}
			}
		}
	}

	// Activate requested interfaces
	if ( !m_ActivateInterfaces.empty() )
	{
		for ( ActivateVec::iterator i = m_ActivateInterfaces.begin(); i != m_ActivateInterfaces.end(); ++i )
		{
			ActivateInterface( i->first, i->second );
		}
		m_ActivateInterfaces.clear();
	}

	// Deactivate requested interfaces
	{ StringVec::iterator i;
	for ( i = deactivateVec.begin(); i != deactivateVec.end(); ++i )
	{
		DeactivateInterface( *i );
	}}

	// Place any interfaces on top
	{
		for ( StringVec::iterator i = m_OnTopInterfaces.begin(); i != m_OnTopInterfaces.end(); ++i )
		{
			GUInterfaceVec::iterator findInterface = m_UIInterfaces.begin();
			while ( findInterface != m_UIInterfaces.end() )
			{
				if ( (*findInterface)->name.same_no_case( *i ) )
				{
					GUInterface * onTop = (*findInterface);
					m_UIInterfaces.erase( findInterface );
					m_UIInterfaces.push_back( onTop );
					findInterface = m_UIInterfaces.begin();
					i = m_OnTopInterfaces.erase( i );

					if ( i == m_OnTopInterfaces.end() )
					{
						break;
					}
				}
				else
				{
					++findInterface;
				}			
			}

			if ( i == m_OnTopInterfaces.end() )
			{
				break;
			}			
		}
	}

	m_OnTopInterfaces.clear();

	// Update animations
	m_pAnimation->Update( seconds );

	if ( GetProcessDeletion() )
	{
		ProcessWindowDeletionList();
		SetProcessDeletion( false );
	}		
}


bool UIShell::IsModalInterfaceActive( gpstring sParent )
{
	GUInterfaceVec::reverse_iterator	j;
	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if (( (*j)->bVisible == true ) && ( (*j)->bModal ) && ( !sParent.same_no_case( (*j)->name ) ))
		{
			return true;
		}
	}

	return false;
}


/**************************************************************************

Function		: Draw()

Purpose			: Draws all activated interfaces

Returns			: void

**************************************************************************/
void UIShell::Draw()
{
	GPPROFILERSAMPLE( "UIShell :: Draw", SP_UI | SP_DRAW );

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	if ( GetVisible() == true ) 
	{
		GUInterfaceVec::iterator j;
		UIWindowDrawMap::iterator i;
		for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
		{
			if ( (*j)->bVisible == true ) 
			{
				for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) 
				{

					if ( (*i).second->GetType() == UI_TYPE_CURSOR  )
					{
						m_drawCursors.push_back( (*i).second );
					}
					else if (( (*i).second->GetType() == UI_TYPE_ITEM ) ||
						( (*i).second->GetType() == UI_TYPE_TEXTBOX ) || (*i).second->GetTopmost() )
					{
						m_drawItems.push_back( (*i).second );
						if ( (*i).second->GetType() == UI_TYPE_TEXTBOX )
						{
							// $$$ temp fix
							(*i).second->Draw();
						}
					}
					else if ( (*i).second->GetType() == UI_TYPE_ITEMSLOT )
					{
						UIItemSlot * pSlot = (UIItemSlot *)(*i).second;
						if ( pSlot->IsEquipSlot() )
						{
							m_drawItems.push_back( (*i).second );
						}
						else
						{
							(*i).second->Draw();
						}
					}

					else
					{
						(*i).second->Draw();
					}
				}
			}
		}

		gUIAnimation.Draw();

		DrawDragBoxes();
	}
	else if ( GetDrawCursor() )
	{
		GUInterfaceVec::iterator j;
		UIWindowDrawMap::iterator i;
		for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) 
		{
			if ( (*j)->bVisible == true ) 
			{
				for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) 
				{
					if ( (*i).second->GetType() == UI_TYPE_CURSOR )
					{
						m_drawCursors.push_back( (*i).second );
					}					
				}
			}
		}
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}

void UIShell::DrawUiIme()
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		ImeUi_RenderUI();
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}


void UIShell::DrawInterface( gpstring sInterface )
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	if ( GetVisible() == true )
	{
		GUInterfaceVec::iterator j;
		UIWindowDrawMap::iterator i;
		for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
		{
			if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( sInterface ) ) )
			{
				for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i )
				{
					if ( (*i).second->GetType() == UI_TYPE_CURSOR )
					{
						m_drawCursors.push_back( (*i).second );
					}
					else
					{
						(*i).second->Draw();
					}
				}
			}
		}
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}


// Draws only the active cursors
void UIShell::DrawCursor()
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	UIWindowVec::iterator i;
	for ( i = m_drawCursors.begin(); i != m_drawCursors.end(); ++i )
	{
		(*i)->Draw();
	}

	m_drawCursors.clear();

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}


// Draws only the items
void UIShell::DrawItems()
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	UIWindowVec::iterator i;
	for ( i = m_drawItems.begin(); i != m_drawItems.end(); ++i )
	{
		(*i)->Draw();
	}

	m_drawItems.clear();	

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );

	if ( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() )
	{
		gUIShell.DrawUiIme();
	}
}


void UIShell::ClearItems()
{
	m_drawItems.clear();
}


void UIShell::DrawStatusBars()
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	if ( GetVisible() == true ) {
		GUInterfaceVec::iterator j;
		UIWindowMap::iterator i;
		for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
			if ( (*j)->bVisible == true ) {
				for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) {
					if ( (*i).second->GetType() == UI_TYPE_STATUSBAR )
					{
						(*i).second->Draw();
					}
				}
			}
		}
	}

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}


/**************************************************************************

Function		: Resize()

Purpose			: Resizes the interface windows

Returns			: void

**************************************************************************/
void UIShell::Resize( int width, int height )
{
	int originalWidth	= m_screenWidth;
	int originalHeight	= m_screenHeight;

	SetScreenDimensions( width, height );

	GUInterfaceVec::iterator j;
	UIWindowMap::iterator i;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i )
		{
			(*i).second->Resize( width, height );

			if ( (*j)->resWidth != 0 && (*j)->resHeight != 0 )
			{
				(*i).second->ResizeToCurrentResolution( originalWidth, originalHeight );
			}
		}		

		if ( !(*j)->sCenter.empty() )
		{
			CenterInterface( *(*j) );
		}

		(*j)->resWidth = m_screenWidth;
		(*j)->resHeight = m_screenHeight;
	}
}


/**************************************************************************

Function		: FindUIWindow()

Purpose			: Finds a window based on name

Returns			: UIWindow *

**************************************************************************/
UIWindow * UIShell::FindUIWindow( const char* name, const char* sInterface )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		if ( (sInterface == NULL) || (*sInterface == '\0') || (*k)->name.same_no_case( sInterface ) ) {
			i = (*k)->ui_windows.find( name );
			if ( i != (*k)->ui_windows.end() ) {
				return (*i).second;
			}
		}
	}

	return 0;
}



/**************************************************************************

Function		: FindInterface()

Purpose			: Finds a user interface

Returns			: GUInterface *

**************************************************************************/
GUInterface * UIShell::FindInterface( const char* interface_name )
{
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( interface_name ) ) {
			return *j;
		}
	}
	return 0;
}



// Find an item using an id
UIItem * UIShell::FindItemByID( int id )
{
	GUInterfaceVec::iterator j;
	UIWindowMap::iterator i;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
		for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) {
			if ( (*i).second->GetType() == UI_TYPE_ITEM ) {
				UIItem *pItem = (UIItem *)(*i).second;
				if ( pItem->GetItemID() == (unsigned int)id ) {
					return pItem;
				}
			}
		}
	}
	return 0;
}



// Find all active items
void UIShell::FindActiveItems( UIItemVec & item_vec )
{
	UIWindowSet::iterator i;
	for ( i = m_activeItems.begin(); i != m_activeItems.end(); ++i )
	{
		if ( (*i) )
		{
			item_vec.push_back( (UIItem *)(*i) );
		}
	}
}


void UIShell::AddActiveItem( UIWindow * pItem )
{
	if ( pItem )
	{
		m_activeItems.insert( pItem );
	}
}


void UIShell::RemoveActiveItem( UIWindow * pItem )
{
	UIWindowSet::iterator iFind = m_activeItems.find( pItem );
	if ( iFind != m_activeItems.end() )
	{
		m_activeItems.erase( iFind );
	}
}


int UIShell::GetNumActiveItems( const gpstring& /*sInterface*/ )
{
	return m_activeItems.size();
}


// Messaging helper
bool UIShell::SendUIMessage( eUIMessage code, const char* window_name, const char* sInterface )
{
	bool success = false;

	UIWindow* wnd = FindUIWindow( window_name, sInterface );
	if ( wnd != NULL )
	{
		success = gUIMessenger.SendUIMessage( code, wnd );
	}

	return ( success );
}


/**************************************************************************

Function		: PublishInterfaceToInputBinder()

Purpose			: Publish a input information to the input binder.

Returns			: void

**************************************************************************/
void UIShell::PublishInterfaceToInputBinder()
{
#	if _USE_INPUT_BINDER_

	// Mouse specific input callbacks
	GetInputBinder()->PublishVoidInterface( "left_button_down",		makeFunctor( *this, &UIShell::Input_OnMouseLeftButtonDown	) );
	GetInputBinder()->PublishVoidInterface( "left_button_up",		makeFunctor( *this, &UIShell::Input_OnMouseLeftButtonUp		) );
	GetInputBinder()->PublishVoidInterface( "right_button_down",	makeFunctor( *this, &UIShell::Input_OnMouseRightButtonDown	) );
	GetInputBinder()->PublishVoidInterface( "right_button_up",		makeFunctor( *this, &UIShell::Input_OnMouseRightButtonUp	) );
	GetInputBinder()->PublishVoidInterface( "left_double_click",	makeFunctor( *this, &UIShell::Input_OnMouseLeftDoubleClick	) );
	GetInputBinder()->PublishVoidInterface( "on_wheel_up",			makeFunctor( *this, &UIShell::Input_OnMouseWheelUp			) );
	GetInputBinder()->PublishVoidInterface( "on_wheel_down",		makeFunctor( *this, &UIShell::Input_OnMouseWheelDown		) );
	GetInputBinder()->PublishVoidInterface( "on_enter",				makeFunctor( *this, &UIShell::Input_OnEnter	) );
	GetInputBinder()->PublishVoidInterface( "on_escape",			makeFunctor( *this, &UIShell::Input_OnEscape	) );
	GetInputBinder()->PublishIntInterface ( "cursor_dx",			makeFunctor( *this, &UIShell::OnDeltaXCursor                ) );
	GetInputBinder()->PublishIntInterface ( "cursor_dy",			makeFunctor( *this, &UIShell::OnDeltaYCursor                ) );


	// Keyboard specific input callbacks
	GetInputBinder()->PublishAllKeysInterface ( makeFunctor( *this, &UIShell::Input_OnKeypress ) );
	GetInputBinder()->PublishAllCharsInterface( makeFunctor( *this, &UIShell::Input_OnChar	   ) );

#	endif
}


/**************************************************************************

Function		: BindInputToPublishedInterface()

Purpose			: Bind input information to our published interface.

Returns			: void

**************************************************************************/
void UIShell::BindInputToPublishedInterface()
{
#	if _USE_INPUT_BINDER_

	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "left_down",      Keys::Q_IGNORE_QUALIFIERS ), "left_button_down"  );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "left_up",        Keys::Q_IGNORE_QUALIFIERS ), "left_button_up"    );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "left_dbl_click", Keys::Q_IGNORE_QUALIFIERS ), "left_double_click" );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "right_down",     Keys::Q_IGNORE_QUALIFIERS ), "right_button_down" );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "right_up",       Keys::Q_IGNORE_QUALIFIERS ), "right_button_up"   );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "wheel_up",       Keys::Q_IGNORE_QUALIFIERS ), "on_wheel_up"   );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "wheel_down",     Keys::Q_IGNORE_QUALIFIERS ), "on_wheel_down"   );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "dx", Keys::Q_IGNORE_QUALIFIERS ), "cursor_dx" );
	GetInputBinder()->BindInputToPublishedInterface( MouseInput( "dy", Keys::Q_IGNORE_QUALIFIERS ), "cursor_dy" );

	GetInputBinder()->BindInputToPublishedInterface( KeyInput( Keys::KEY_RETURN, Keys::Q_KEY_DOWN ), "on_enter"   );
	GetInputBinder()->BindInputToPublishedInterface( KeyInput( Keys::KEY_ESCAPE, Keys::Q_KEY_DOWN ), "on_escape"   );

#	endif
}


/**************************************************************************

Function		: Input_OnMouseMove()

Purpose			: Performs all operations necessary when the mouse moves.
				  This includes recording the mouse position within the
				  object and sending rollover messages to any windows.

Returns			: void

**************************************************************************/
bool UIShell::Input_OnMouseMove( int x, int y )
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	if (( x == m_mouseX ) && ( y == m_mouseY )) 
	{
		m_mouseSettled++;
		return m_bRolloverConsumed;
	}

	m_mouseSettled = 0;

	m_mouseX = x;
	m_mouseY = y;

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;

	SetRolloverItemslotID( 0 );
	m_sRolloverName.clear();	

	bool bRolloverAllowed = (!gAppModule.GetLButton() && !gAppModule.GetRButton()) ||
							(gAppModule.GetLButton() && GetLButtonDown()) ||
						    (gAppModule.GetRButton() && GetRButtonDown());

	// Always handle cursor interfaces
	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->interface_type.same_no_case( "cursor" ) )
		{
			for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i )
			{
				(*i).second->SetMousePos( m_mouseX, m_mouseY );
			}
		}
	}

	bool bConsumableFound = false;
	bool bInputConsumed = false;
	bool bModalActive = false;
	bool bDragged = false;

	// If any interfaces are modal, handle them and nothing else
	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bModal && (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				while ( i != (*j)->ui_draw_windows.begin() )
				{
					--i;
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool hitDetect = (*i).second->HitDetect( m_mouseX, m_mouseY );

						// Handle input on any windows
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE )) )
						{
							if ( hitDetect && !bConsumableFound && ((*i).second->GetType() != UI_TYPE_ITEM) )
							{
								if ( (*i).second->GetType() == UI_TYPE_ITEMSLOT )
								{
									SetRolloverItemslotID( ((UIItemSlot *)(*i).second)->GetItemID() );
								}
								else if ( (*i).second->GetType() == UI_TYPE_INFOSLOT )
								{
									SetRolloverItemslotID( ((UIInfoSlot *)(*i).second)->GetItemID() );
								}

								bInputConsumed = true;
								if ( m_sRolloverName.empty() && !(*i).second->GetParentWindow() )
								{
									if ( bRolloverAllowed )
									{
										SetRolloverName( (*i).second->GetRolloverKey() );
									}
								}
							}
							if ( !bConsumableFound && hitDetect && !(*i).second->GetRollover() )
							{
								if ( bRolloverAllowed )
								{
									m_pMessenger->SendUIMessage( UIMessage( MSG_ROLLOVER ), (*i).second );
									(*i).second->SetRollover( true );								
								}								
							}
							else if (  ( bConsumableFound && hitDetect && (*i).second->GetRollover() )
									|| ( !hitDetect && (*i).second->GetRollover() ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_ROLLOFF ), (*i).second );								
								(*i).second->SetRollover( false );
							}
						}
						if ( hitDetect && (*i).second->IsConsumable() )
						{
							bConsumableFound = true;
						}
						if (( (*i).second->GetType() == UI_TYPE_CURSOR )		||
							( (*i).second->GetType() == UI_TYPE_SLIDER )		||
							( (*i).second->GetType() == UI_TYPE_POPUPMENU )		||
							( (*i).second->GetType() == UI_TYPE_ITEM )			||
							( (*i).second->GetType() == UI_TYPE_DOCKBAR ) 		||
							( (*i).second->GetType() == UI_TYPE_GRIDBOX ) 		||
							( (*i).second->GetType() == UI_TYPE_LISTREPORT )	||
							( (*i).second->GetType() == UI_TYPE_TEXTBOX )		||
							( (*i).second->GetType() == UI_TYPE_LISTBOX )		||
							( (*i).second->GetType() == UI_TYPE_MOUSE_LISTENER ))
						{
							(*i).second->SetMousePos( m_mouseX, m_mouseY );
						}
					}
				}
			}

			bModalActive = true;
			break;
		}
	}

	if ( bModalActive )
	{
		return bInputConsumed;
	}

	// Handle non-modal interfaces
	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				while ( i != (*j)->ui_draw_windows.begin() )
				{
					--i;
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool hitDetect = (*i).second->HitDetect( m_mouseX, m_mouseY );

						// Handle input on any windows
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE )))
						{
							if ( hitDetect && !bConsumableFound &&
								((*i).second->GetType() != UI_TYPE_CHATBOX) && ((*i).second->GetType() != UI_TYPE_ITEM) )
							{
								if ( (*i).second->GetType() == UI_TYPE_ITEMSLOT )
								{
									SetRolloverItemslotID( ((UIItemSlot *)(*i).second)->GetItemID() );
								}
								else if ( (*i).second->GetType() == UI_TYPE_INFOSLOT )
								{
									SetRolloverItemslotID( ((UIInfoSlot *)(*i).second)->GetItemID() );
								}

								if ( m_sRolloverName.empty() && !(*i).second->GetParentWindow() )
								{
									if ( bRolloverAllowed )
									{
										SetRolloverName( (*i).second->GetRolloverKey() );
									}
								}
					 			
								if ( (*i).second->IsPassThrough() == false )
								{
									bInputConsumed = true;
								}								
							}

							if ( !bConsumableFound && hitDetect && !(*i).second->GetRollover() )
							{
								if ( bRolloverAllowed )
								{
									m_pMessenger->SendUIMessage( UIMessage( MSG_ROLLOVER ), (*i).second );
									(*i).second->SetRollover( true );								
								}								
							}
							else if (  ( bConsumableFound && hitDetect && (*i).second->GetRollover() )
									|| ( !hitDetect && (*i).second->GetRollover() ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_ROLLOFF ), (*i).second );								
								(*i).second->SetRollover( false );								
							}

							if ( (*i).second->GetDraggable() && GetLButtonDown() && (*i).second->GetDragAllowed() )
							{
								if ( ((*i).second->GetDragTimeElapsed() >= (*i).second->GetDragTime()) && !bDragged )
								{
									int delta_x = m_mouseX - (*i).second->GetMouseX();
									int delta_y = m_mouseY - (*i).second->GetMouseY();
									(*i).second->DragWindow( delta_x, delta_y );
									bDragged = true;
								}
								else
								{
									int delta_x = m_mouseX - (*i).second->GetMouseX();
									int delta_y = m_mouseY - (*i).second->GetMouseY();
									if (( abs(delta_x) > 5 ) || ( abs(delta_y) > 5 ))
									{
										(*i).second->SetDragAllowed( false );
									}
									else
									{
										(*i).second->SetDragTimeElapsed( 0.0f );
									}
								}
							}
						}
						if ( hitDetect && (*i).second->IsConsumable() )
						{
							bConsumableFound = true;
						}
						if (( (*i).second->GetType() == UI_TYPE_CURSOR )		||
							( (*i).second->GetType() == UI_TYPE_SLIDER )		||
							( (*i).second->GetType() == UI_TYPE_POPUPMENU )		||
							( (*i).second->GetType() == UI_TYPE_ITEM )			||
							( (*i).second->GetType() == UI_TYPE_DOCKBAR )		||
							( (*i).second->GetType() == UI_TYPE_GRIDBOX ) 		||
							( (*i).second->GetType() == UI_TYPE_LISTREPORT )	||
							( (*i).second->GetType() == UI_TYPE_TEXTBOX )		||
							( (*i).second->GetType() == UI_TYPE_LISTBOX )		||
							( (*i).second->GetType() == UI_TYPE_MOUSE_LISTENER ))
						{
							(*i).second->SetMousePos( m_mouseX, m_mouseY );
						}
					}
				}
			}
		}
	}

	return bInputConsumed;
}


/**************************************************************************

Function		: Input_OnMouseLeftButtonDown()

Purpose			: Performs all operations necessary when the left mouse
				  button is depressed.

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseLeftButtonDown()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	SetLButtonDown( true );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if ( (*k)->bModal && (*k)->bVisible && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{

					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
								&& ((*i).second->GetVisible()) )
						{

							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLBUTTONDOWN ), (*i).second );

							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								(*i).second->SetDragAllowed( true );
								if ( (*i).second->GetType() != UI_TYPE_CHATBOX )
								{
									bInputConsumed = true;
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}

			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					bool bVisible = (*i).second->GetVisible();
					bool bEnabled = (*i).second->IsEnabled();

					if ( bVisible && bEnabled  )
					{
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE )))
						{
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLBUTTONDOWN ), (*i).second );

							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LBUTTONDOWN ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								(*i).second->SetDragAllowed( true );
								if ( (*i).second->GetType() != UI_TYPE_CHATBOX )
								{
									if ( (*i).second->IsPassThrough() == false )
									{
										bInputConsumed = true;
									}
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}

					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	if ( !bInputConsumed )
	{
		SetLButtonDown( false );
	}

	return bInputConsumed;
}


/**************************************************************************

Function		: Input_OnMouseLeftButtonUp()

Purpose			: Performs all operations necessary when the left mouse
				  button is let up.

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseLeftButtonUp()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	UIWindow * pItemHit = NULL;
	SetLButtonDown( false );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LBUTTONUP ), (*i).second );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUP ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LBUTTONUP ), (*i).second );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									if ( (*i).second->IsPassThrough() == false )
									{
										bInputConsumed = true;
									}
								}
								else
								{
									pItemHit = (*i).second;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUP ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	if ( !bInputConsumed && pItemHit )
	{
		gUIMessenger.Notify( "item_interaction", pItemHit );
	}

	return bInputConsumed;
}


/**************************************************************************

Function		: Input_OnMouseRightButtonDown()

Purpose			: Performs all operations necessary when the right mouse
				  button is let down.

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseRightButtonDown()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	SetRButtonDown( true );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
								&& ((*i).second->GetVisible()) )
						{
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RBUTTONDOWN ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								if ( (*i).second->GetType() != UI_TYPE_CHATBOX )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRBUTTONDOWN ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}

			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if ( (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
									&& ((*i).second->GetVisible()) )
						{
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RBUTTONDOWN ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								if ( (*i).second->GetType() != UI_TYPE_CHATBOX )
								{
									if ( (*i).second->IsPassThrough() == false )
									{
										bInputConsumed = true;
									}
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRBUTTONDOWN ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	
	if ( !bInputConsumed )
	{
		SetRButtonDown( false );
	}

	return bInputConsumed;
}


/**************************************************************************

Function		: Input_OnMouseRightButtonUp()

Purpose			: Performs all operations necessary when the right mouse
				  button is let up.

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseRightButtonUp()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	SetRButtonDown( false );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RBUTTONUP ), (*i).second );
								bInputConsumed = true;
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRBUTTONUP ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RBUTTONUP ), (*i).second );

								if ( (*i).second->IsPassThrough() == false )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRBUTTONUP ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return bInputConsumed;
}



/**************************************************************************

Function		: Input_OnMouseLeftDoubleClick()

Purpose			: Performs all operations necessary when the left mouse
				  button is double clicked'

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseLeftDoubleClick()
{	
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	SetLButtonDown( false );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LDOUBLECLICK ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLDOUBLECLICK ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}

			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_LDOUBLECLICK ), (*i).second );
								(*i).second->SetMousePos( m_mouseX, m_mouseY );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALLDOUBLECLICK ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}	

	bool bConsumed = bInputConsumed | Input_OnMouseLeftButtonDown() | Input_OnMouseLeftButtonUp();
	return ( bConsumed );
}


/**************************************************************************

Function		: Input_OnMouseRightDoubleClick()

Purpose			: Performs all operations necessary when the right mouse
				  button is double clicked'

Returns			: bool

**************************************************************************/
bool UIShell::Input_OnMouseRightDoubleClick()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}
	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;
	SetRButtonDown( false );

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RDOUBLECLICK ), (*i).second );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRDOUBLECLICK ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								m_pMessenger->SendUIMessage( UIMessage( MSG_RDOUBLECLICK ), (*i).second );
								if ( (*i).second->GetType() != UI_TYPE_ITEM )
								{
									bInputConsumed = true;
								}
							}
							m_pMessenger->SendUIMessage( UIMessage( MSG_GLOBALRDOUBLECLICK ), (*i).second );
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return bInputConsumed | Input_OnMouseRightButtonDown() | Input_OnMouseRightButtonUp();
}



bool UIShell::Input_OnMouseWheelUp()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool bHitDetected = false;
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );													
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								bHitDetected = true;
								if ( m_pMessenger->SendUIMessage( UIMessage( MSG_WHEELUP ), (*i).second ) )
								{
									if ( (*i).second->GetType() != UI_TYPE_ITEM )
									{
										if ( (*i).second->IsPassThrough() == false )
										{
											bInputConsumed = true;
										}
									}
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							bInputConsumed = bHitDetected;
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool bHitDetected = false;
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								bHitDetected = true;
								if ( m_pMessenger->SendUIMessage( UIMessage( MSG_WHEELUP ), (*i).second ) )
								{
									if ( (*i).second->GetType() != UI_TYPE_ITEM )
									{
										if ( (*i).second->IsPassThrough() == false )
										{
											bInputConsumed = true;
										}
									}
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							bInputConsumed = bHitDetected;
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return bInputConsumed;
}

bool UIShell::Input_OnMouseWheelDown()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	// This boolean flag returns its value in case other UI layers want to try and handle the input.
	bool bInputConsumed = false;

	UIWindowDrawMap::iterator			i;
	GUInterfaceVec::reverse_iterator	j;
	GUInterfaceVec::reverse_iterator	k;

	for ( k = m_UIInterfaces.rbegin(); k != m_UIInterfaces.rend(); k++ )
	{
		if (( (*k)->bModal == true ) && ( (*k)->bVisible == true ) && (*k)->bEnabled )
		{
			if ( (*k)->ui_draw_windows.size() != 0 )
			{
				i = (*k)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool bHitDetected = false;
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								bHitDetected = true;
								if ( m_pMessenger->SendUIMessage( UIMessage( MSG_WHEELDOWN ), (*i).second ) )
								{
									if ( (*i).second->GetType() != UI_TYPE_ITEM )
									{
										if ( (*i).second->IsPassThrough() == false )
										{
											bInputConsumed = true;
										}
									}
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							bInputConsumed = bHitDetected;
							return bInputConsumed;
						}
					}
					if ( i != (*k)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
			return bInputConsumed;
		}
	}

	for ( j = m_UIInterfaces.rbegin(); j != m_UIInterfaces.rend(); ++j )
	{
		if ( (*j)->bVisible && (*j)->bEnabled )
		{
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				i = (*j)->ui_draw_windows.end();
				i--;
				for (;;)
				{
					if ( (*i).second->GetVisible() && (*i).second->IsEnabled() )
					{
						bool bHitDetected = false;
						if (((*i).second->GetInputType() == TYPE_INPUT_ALL ) || ((*i).second->GetInputType() == TYPE_INPUT_MOUSE ))
						{
							(*i).second->SetDragAllowed( false );
							if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) )
							{
								bHitDetected = true;
								if ( m_pMessenger->SendUIMessage( UIMessage( MSG_WHEELDOWN ), (*i).second ) )
								{									
									if ( (*i).second->GetType() != UI_TYPE_ITEM )
									{
										if ( (*i).second->IsPassThrough() == false )
										{
											bInputConsumed = true;
										}
									}
								}
							}
						}
						if ( (*i).second->HitDetect( m_mouseX, m_mouseY ) && (*i).second->IsConsumable() )
						{
							bInputConsumed = bHitDetected;
							return bInputConsumed;
						}
					}
					if ( i != (*j)->ui_draw_windows.begin() )
					{
						--i;
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return bInputConsumed;
}


/**************************************************************************

Function		: Input_OnKeypress()

Purpose			: Reads all keypresses from the input binder and consumes
				  any input it is listening for.

Returns			: bool

**************************************************************************/

bool UIShell::Input_OnKeypress( const KeyInput& key )
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	UIMessage msg( MSG_KEYPRESS );
	msg.m_key = (wchar_t)key.GetKey();
	if ( key.GetQualifierFlags() & Keys::Q_KEY_DOWN )
	{
		if ( m_pMessenger->SendUIMessage( msg ) ) {
			return true;
		}
	}
	return false;
}

bool UIShell::Input_OnChar( wchar_t c )
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	UIMessage msg( MSG_CHAR );
	msg.m_key = c;
	if ( m_pMessenger->SendUIMessage( msg ) ) {
		return true;
	}
	return false;
}

bool UIShell::Input_OnEnter()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	UIMessage msg( MSG_ENTER );
	if ( m_pMessenger->SendUIMessage( msg ) ) {
		return true;
	}

	return false;
}

bool UIShell::Input_OnEscape()
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	UIMessage msg( MSG_ESCAPE );
	if ( m_pMessenger->SendUIMessage( msg ) ) {
		return true;
	}

	return false;
}

#if !_USE_INPUT_BINDER_

bool UIShell::Input_OnKeypress( int key )
{
	if ( GetGuiDisabled() )
	{
		return false;
	}

	char char_key = Keys::KeyCodeToString( (Keys::KEY)key.GetKey() );

	UIMessage msg( MSG_KEYPRESS );
	msg.key = char_key;
	if ( m_pMessenger->SendUIMessage( msg ) ) {
		return true;
	}
	return false;
}

#endif


/**************************************************************************

Function		: InitializeInterfaceType()

Purpose			: Handles special-case (non-standard) interface types

Returns			: void

**************************************************************************/
void UIShell::InitializeInterfaceType( GUInterface *ui_interface, gpstring interface_type, FastFuelHandle fhInterface )
{
	if ( interface_type.empty() ) 
	{
		return;
	}
	else if ( interface_type.same_no_case( "pullout" ) ) 
	{
		int xoffset = 0;
		int yoffset = 0;
		int width	= 0;
		if ( fhInterface.IsValid() ) 
		{
			fhInterface.Get( "xoffset", xoffset );
			fhInterface.Get( "yoffset", yoffset );
			fhInterface.Get( "width", width );
			ui_interface->width = width;
			ui_interface->xoffset = xoffset;
			ui_interface->yoffset = yoffset;
			UIWindowMap::iterator i;
			for ( i = ui_interface->ui_windows.begin(); i != ui_interface->ui_windows.end(); ++i ) 
			{
				(*i).second->SetRect(	(*i).second->GetRect().left + xoffset,
										(*i).second->GetRect().right + xoffset,
										(*i).second->GetRect().top + yoffset,
										(*i).second->GetRect().bottom + yoffset );
			}
		}
	}
}


/**************************************************************************

Function		: InitializePulloutInterface()

Purpose			: Initializes the pullout interface

Returns			: void

**************************************************************************/
void UIShell::InitializePulloutInterface( gpstring interface_name, gpstring reference_interface, bool bWidth  )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator j;

	int width = 0;
	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( reference_interface ) ) {
			if ( bWidth ) {
				width = (*j)->width;
			}
			else {
				width = (*j)->xoffset;
			}
		}
	}

	// For modal interfaces
	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( interface_name ) ) {
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) {
				(*i).second->SetRect(	(*i).second->GetPulloutOffset()+width,
										(*i).second->GetRect().right-(*i).second->GetRect().left+(*i).second->GetPulloutOffset()+width,
										(*i).second->GetRect().top,
										(*i).second->GetRect().bottom );
			}
		}
	}
}



/**************************************************************************

Function		: InitializePulloutInterfaceToWindow()

Purpose			: Initializes the pullout interface

Returns			: void

**************************************************************************/
void UIShell::InitializePulloutInterfaceToWindow( gpstring interface_name, gpstring reference_window, PULLOUT_MODE mode, int overlap )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator j;

	UIWindow *pWindow = FindUIWindow( reference_window );
	if ( !pWindow ) {
		return;
	}

	int xoffset = 0;
	int yoffset = 0;
	switch ( mode ) {
	case X_ALIGN_LEFT:
		{
			xoffset = pWindow->GetRect().left + overlap;
		}
		break;
	case X_ALIGN_RIGHT:
		{
			xoffset = pWindow->GetRect().right - overlap;
		}
		break;
	case Y_ALIGN_TOP:
		{
			yoffset = pWindow->GetRect().top + overlap;
		}
		break;
	case Y_ALIGN_BOTTOM:
		{
			yoffset = pWindow->GetRect().bottom - overlap;
		}
		break;
	default:
		{
			return;
		}
		break;
	}


	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( interface_name ) ) {
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) {
				int width	= (*i).second->GetRect().right-(*i).second->GetRect().left;
				int height	= (*i).second->GetRect().bottom-(*i).second->GetRect().top;

				if ( (mode == X_ALIGN_LEFT) || (mode == X_ALIGN_RIGHT) ) {
					(*i).second->SetRect(	(*i).second->GetPulloutOffset()+xoffset,
											(*i).second->GetPulloutOffset()+xoffset+width,
											(*i).second->GetRect().top,
											(*i).second->GetRect().bottom );
				}
				else if ( (mode == Y_ALIGN_TOP) || (mode == Y_ALIGN_BOTTOM) ) {
					(*i).second->SetRect(	(*i).second->GetRect().left,
											(*i).second->GetRect().right,
											(*i).second->GetPulloutOffset()+yoffset,
											(*i).second->GetPulloutOffset()+yoffset+height );
				}
			}
		}
	}
}


void UIShell::AlignWindowToWindow( gpstring sDest, gpstring sSource, PULLOUT_MODE mode, int overlap )
{
	UIWindow *pSourceWindow = FindUIWindow( sSource );
	UIWindow *pDestWindow	= FindUIWindow( sDest );

	if (( !pSourceWindow ) || ( !pDestWindow )) {
		return;
	}

	int xoffset = 0;
	int yoffset = 0;
	switch ( mode ) {
	case X_ALIGN_LEFT:
	case X_ALIGN_LEFT_OUTSIDE:
		{
			xoffset = pSourceWindow->GetRect().left + overlap;
		}
		break;
	case X_ALIGN_RIGHT:
	case X_ALIGN_RIGHT_OUTSIDE:
		{
			xoffset = pSourceWindow->GetRect().right - overlap;
		}
		break;
	case Y_ALIGN_TOP:
	case Y_ALIGN_TOP_OUTSIDE:
		{
			yoffset = pSourceWindow->GetRect().top + overlap;
		}
		break;
	case Y_ALIGN_BOTTOM:
	case Y_ALIGN_BOTTOM_OUTSIDE:
		{
			yoffset = pSourceWindow->GetRect().bottom - overlap;
		}
		break;
	default:
		{
			return;
		}
		break;
	}

	UIRect old_rect = pDestWindow->GetRect();

	int width	= pDestWindow->GetRect().right - pDestWindow->GetRect().left;
	int height	= pDestWindow->GetRect().bottom - pDestWindow->GetRect().top;

	if ( (mode == X_ALIGN_LEFT) || (mode == X_ALIGN_RIGHT) ) {
		pDestWindow->SetRect(	xoffset,
								xoffset+width,
								pDestWindow->GetRect().top,
								pDestWindow->GetRect().bottom );
	}
	else if ( (mode == Y_ALIGN_TOP) || (mode == Y_ALIGN_BOTTOM) ) {
		pDestWindow->SetRect(	pDestWindow->GetRect().left,
								pDestWindow->GetRect().right,
								yoffset,
								yoffset+height );
	}
	else if ( (mode == X_ALIGN_LEFT_OUTSIDE) || (mode == X_ALIGN_RIGHT_OUTSIDE) ) {
		pDestWindow->SetRect(	xoffset-width,
								xoffset,
								pDestWindow->GetRect().top,
								pDestWindow->GetRect().bottom );
	}
	else if ( (mode == Y_ALIGN_TOP_OUTSIDE) || (mode == Y_ALIGN_BOTTOM_OUTSIDE) ) {
		pDestWindow->SetRect(	pDestWindow->GetRect().left,
								pDestWindow->GetRect().right,
								yoffset-height,
								yoffset );
	}

	UIWindowVec group	= ListWindowsOfGroup( pDestWindow->GetGroup() );
	UIWindowVec::iterator i;
	for ( i = group.begin(); i != group.end(); ++i ) {
		int width	= (*i)->GetRect().right - (*i)->GetRect().left;
		int height	= (*i)->GetRect().bottom - (*i)->GetRect().top;
		xoffset = abs( old_rect.left - (*i)->GetRect().left );
		yoffset = abs( old_rect.top - (*i)->GetRect().top );

		if ( (*i) == pDestWindow ) {
			continue;
		}

		if ( (mode == X_ALIGN_LEFT) || (mode == X_ALIGN_RIGHT) ) {
			(*i)->SetRect(			pDestWindow->GetRect().left+xoffset,
									pDestWindow->GetRect().left+xoffset+width,
									(*i)->GetRect().top,
									(*i)->GetRect().bottom );
		}
		else if ( (mode == Y_ALIGN_TOP) || (mode == Y_ALIGN_BOTTOM) ) {
			(*i)->SetRect(			(*i)->GetRect().left,
									(*i)->GetRect().right,
									pDestWindow->GetRect().top+yoffset,
									pDestWindow->GetRect().top+yoffset+height );
		}
		else if ( (mode == X_ALIGN_LEFT_OUTSIDE) || (mode == X_ALIGN_RIGHT_OUTSIDE) ) {
			(*i)->SetRect(			pDestWindow->GetRect().left+xoffset,
									pDestWindow->GetRect().left+xoffset+width,
									(*i)->GetRect().top,
									(*i)->GetRect().bottom );
		}
		else if ( (mode == Y_ALIGN_TOP_OUTSIDE) || (mode == Y_ALIGN_BOTTOM_OUTSIDE) ) {
			int new_yoffset = pDestWindow->GetRect().top - old_rect.top;
			(*i)->SetRect(			(*i)->GetRect().left,
									(*i)->GetRect().right,
									(*i)->GetRect().top+new_yoffset,
									(*i)->GetRect().top+new_yoffset+height );

			/*
			(*i)->SetRect(			(*i)->GetRect().left,
									(*i)->GetRect().right,
									pDestWindow->GetRect().top+yoffset,
									pDestWindow->GetRect().top+yoffset+height );
			*/
		}
	}
}


/**************************************************************************

Function		: LoadUICursor()

Purpose			: Loads a cursor in the UI, safeguards against having
				  multiple cursors and other possible confusion in cursor
				  manipulation

Returns			: void

**************************************************************************/
UICursor * UIShell::LoadUICursor( const gpstring& name, const gpstring& sInterface )
{
	UIWindowMap::iterator i;

	UICursor *pCursor = (UICursor *)FindUIWindow( name, sInterface );
	if ( !pCursor )
	{
		return 0;
	}

	return pCursor;
}


/**************************************************************************

Function		: ListWindowsOfType()

Purpose			: Finds a window based on name

Returns			: UIWindow *

**************************************************************************/
UIWindowVec UIShell::ListWindowsOfType( UI_CONTROL_TYPE type )
{
	UIWindowVec window_types;

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k ) {
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i ) {
			if ( (*i).second->GetType() == type ) {
				window_types.push_back( (*i).second );
			}
		}
	}
	return window_types;
}


/**************************************************************************

Function		: ListWindowsOfGroup()

Purpose			: Finds a window based on group

Returns			: UIWindow *

**************************************************************************/
UIWindowVec UIShell::ListWindowsOfGroup( gpstring group )
{
	UIWindowVec windows;

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
		{
			if ( (*i).second->GetGroup().same_no_case( group ))
			{
				windows.push_back( (*i).second );
			}
			if ( (*i).second->GetType() == UI_TYPE_RADIOBUTTON )
			{
				if ( ((UIRadioButton *)(*i).second)->GetRadioGroup().same_no_case( group ) )
				{
					windows.push_back( (*i).second );
				}
			}
		}
	}
	return windows;
}


UIWindowVec UIShell::ListWindowsOfDockGroup( gpstring group )
{
	UIWindowVec windows;

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
		{
			if ( (*i).second->GetDockGroup().same_no_case( group ) )
			{
				windows.push_back( (*i).second );
			}
		}
	}
	return windows;
}


UIWindowVec UIShell::ListWindowsOfRadioGroup( gpstring group )
{
	UIWindowVec windows;

	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
		{
			if ( (*i).second->GetType() == UI_TYPE_RADIOBUTTON )
			{
				UIRadioButton * pButton = (UIRadioButton *)(*i).second;
				if ( pButton->GetRadioGroup().same_no_case( group ) )
				{
					windows.push_back( (*i).second );
				}
			}
			else if ( (*i).second->GetType() == UI_TYPE_TAB )
			{
				UITab * pTab = (UITab *)(*i).second;
				if ( pTab->GetRadioGroup().same_no_case( group ) )
				{
					windows.push_back( (*i).second );
				}
			}
		}
	}
	return windows;
}


/**************************************************************************

Function		: IsInterfaceVisible()

Purpose			: Is an interface visible

Returns			: bool

**************************************************************************/
bool UIShell::IsInterfaceVisible( const gpstring& interface_name )
{
	GUInterfaceVec::iterator j;

	for ( j = m_UIInterfaces.begin(); j != m_UIInterfaces.end(); ++j )
	{
		if ( interface_name.same_no_case( (*j)->name ) )
		{
			return (*j)->bVisible;
		}
	}
	return false;
}


void UIShell::SetGroupScaleToAlignWithWindow( gpstring sGroup, gpstring sWindow, float scale )
{
	UIWindow *pWindow = FindUIWindow( sWindow );

	UIRect old_rect = pWindow->GetRect();
	int rect_width = (int)((pWindow->GetRect().right - pWindow->GetRect().left) * scale);
	int rect_height = (int)((pWindow->GetRect().bottom - pWindow->GetRect().top) * scale);
	pWindow->SetScale( scale );

	pWindow->SetRect(	pWindow->GetRect().left, pWindow->GetRect().left+rect_width,
						pWindow->GetRect().top, pWindow->GetRect().top+rect_height );

	UIWindowVec group	= ListWindowsOfGroup( pWindow->GetGroup() );
	UIWindowVec::iterator i;
	for ( i = group.begin(); i != group.end(); ++i ) {

		if ( (*i) == pWindow ) {
			continue;
		}

		int x_offset = (int)(abs( old_rect.left - (*i)->GetRect().left ) * pWindow->GetScale());
		int y_offset = (int)(abs( old_rect.top - (*i)->GetRect().top ) * pWindow->GetScale());

		int group_window_width	= (int)(((*i)->GetRect().right - (*i)->GetRect().left) * pWindow->GetScale());
		int group_window_height	= (int)(((*i)->GetRect().bottom - (*i)->GetRect().top) * pWindow->GetScale());

		int x1 = pWindow->GetRect().left + x_offset;
		int y1 = pWindow->GetRect().top + y_offset;
		int x2 = x1 + group_window_width;
		int y2 = y1 + group_window_height;


		(*i)->SetRect( x1, x2, y1, y2 );
	}
}


void UIShell::LoadCommonArt()
{
	m_common_art.clear();
	FastFuelHandle hCommon( "UI:Interfaces:Common:common_control_art" );
	if ( hCommon.IsValid() == false )
	{
		return;
	}

	FastFuelFindHandle hFind( hCommon );
	if ( hFind.IsValid() && hFind.FindFirstKeyAndValue() )
	{
		gpstring sKey;
		gpstring sValue;

		while ( hFind.GetNextKeyAndValue( sKey, sValue ) ) 
		{
			unsigned int texture = (unsigned int)gUITextureManager.LoadTexture( sValue );
			m_common_art.insert( std::pair< gpstring, unsigned int >(sKey, texture) );
		}
	}
}


void UIShell::UnloadCommonArt()
{
	UICommonArtMap::iterator i;
	for ( i = m_common_art.begin(); i != m_common_art.end(); ++i )
	{
		GetRenderer().DestroyTexture( (*i).second );
	}
	m_common_art.clear();
}


const unsigned int UIShell::GetCommonArt( gpstring sArt )
{
	UICommonArtMap::iterator i = m_common_art.find( sArt );
	if ( i != m_common_art.end() )
	{
		GetRenderer().AddTextureReference( (*i).second );
		return (*i).second;
	}

	return 0;
}


bool UIShell::IsCommonArt( unsigned int texture )
{
	UICommonArtMap::iterator i;
	for ( i = m_common_art.begin(); i != m_common_art.end(); ++i )
	{
		if ( (*i).second == texture )
		{
			return true;
		}
	}

	return false;
}


bool UIShell::IsCommonArt( gpstring & sTexture )
{
	UICommonArtMap::iterator i = m_common_art.find( sTexture );
	if ( i != m_common_art.end() )
	{
		return true;
	}

	return false;
}


void UIShell::ActivateInterface( gpstring const & sInterfacePath, bool bShow )
{
	FastFuelHandle hInterface( sInterfacePath );

	if( !hInterface.IsValid() )
	{
		gperrorf(( "Invalid Interface: %s!  BOOM!!!!  The data does not seem to exist.  Try doing a get on UI data.", sInterfacePath.c_str() ));
		return;
	}

	ActivateInterface( hInterface );

	gpstring sInterfaceName;

	stringtool::GetDelimitedValue( sInterfacePath, ':', stringtool::GetNumDelimitedStrings( sInterfacePath, ':' ) - 1, sInterfaceName );

	if( bShow )
	{
		ShowInterface( sInterfaceName );
	}
	else
	{
		HideInterface( sInterfaceName );
	}
}


void UIShell::AddBlankInterface( const gpstring& sName )
{
	GUInterface *ui_interface = new GUInterface;
	ui_interface->bDeactivate		= false;
	ui_interface->height			= 0;
	ui_interface->width				= 0;
	ui_interface->xoffset			= 0;
	ui_interface->yoffset			= 0;
	ui_interface->resHeight			= 0;
	ui_interface->resWidth			= 0;
	ui_interface->name				= sName;
	ui_interface->bModal			= false;
	ui_interface->interface_type	= "";
	ui_interface->bVisible			= true;	
	m_UIInterfaces.push_back( ui_interface );
}


void UIShell::ProcessWindowDeletionList()
{
	UIWindowMap::iterator i;
	UIWindowDrawMap::iterator j;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{

		j = (*k)->ui_draw_windows.begin();
		while ( j != (*k)->ui_draw_windows.end() )
		{
			if ( (*j).second->GetMarkedForDeletion() )
			{
				j = (*k)->ui_draw_windows.erase( j );
				if ( j == (*k)->ui_draw_windows.end() )
				{
					break;
				}
			}
			else
			{
				++j;
			}
		}

		i = (*k)->ui_windows.begin();
		while ( i != (*k)->ui_windows.end() )
		{
			if ( !(*i).second )
			{
				i = (*k)->ui_windows.erase( i );
			}
			else if ( (*i).second->GetMarkedForDeletion() )
			{				
				if ( (*i).second->GetType() == UI_TYPE_ITEM )
				{
					UIItem * pItem = (UIItem *)(*i).second;
					if ( pItem->GetParentWindow() != 0 )
					{						
						pItem->SetMarkForDeletion( false );
						return;
					}
				}

				Delete( (*i).second );				
				i = (*k)->ui_windows.erase( i );
				if ( i == (*k)->ui_windows.end() )
				{
					break;
				}
			}
			else
			{
				++i;
			}
		}
	}
}


gpstring UIShell::GetTextureNamingKeyName( gpstring sFilename )
{
	gpstring sNew = sFilename;
	unsigned int pos = sFilename.find_last_of( "\\", sFilename.size() );
	if ( pos != gpstring::npos )
	{
		sNew = sFilename.substr( pos+1, sFilename.size() );
		unsigned int pos2 = sNew.find_last_of( ".", sNew.size() );
		if ( pos2 != gpstring::npos )
		{
			sNew = sNew.substr( 0, pos2 );
		}
	}
	return sNew;
}


bool UIShell::DeleteWindow( UIWindow * pWindow )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
		{
			if ( (*i).second == pWindow )
			{
				(*k)->ui_windows.erase( i );

				UIWindowDrawMap::iterator j;
				for ( j = (*k)->ui_draw_windows.begin(); j != (*k)->ui_draw_windows.end(); ++j )
				{
					if ( (*j).second == pWindow )
					{
						(*k)->ui_draw_windows.erase( j );

						delete pWindow;

						return true;
					}
				}
			}
		}
	}
	return false;
}


void UIShell::InsertDragBox( UIRect rect, DWORD dwColor )
{
	DRAG_BOX dragBox;
	dragBox.rect = rect;
	dragBox.dwColor = dwColor;
	m_dragBoxes.push_back( dragBox );
}


void UIShell::DrawDragBoxes()
{
	DragBoxColl::iterator i;
	for ( i = m_dragBoxes.begin(); i != m_dragBoxes.end(); ++i )
	{
		DrawColorBox( (*i).rect, (*i).dwColor );
	}
	m_dragBoxes.clear();
}


void UIShell::DrawColorBox( UIRect rect, DWORD dwColor, DWORD dwSecondColor )
{
	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

	gUIShell.GetRenderer().SetTextureStageState(	0,
													D3DTOP_DISABLE,
													D3DTOP_MODULATE,
													D3DTADDRESS_CLAMP,
													D3DTADDRESS_CLAMP,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DTEXF_POINT,
													D3DBLEND_SRCALPHA,
													D3DBLEND_INVSRCALPHA,
													true );

	tVertex Verts[4];
	memset( Verts, 0, sizeof( tVertex ) * 4 );

	Verts[0].x			= rect.left-0.5f;
	Verts[0].y			= rect.top-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].rhw		= 1.0f;
	Verts[0].uv.u		= 0.0f;
	Verts[0].uv.v		= 0.0f;
	Verts[0].color		= dwColor;

	Verts[1].x			= rect.left-0.5f;
	Verts[1].y			= rect.bottom-0.5f;
	Verts[1].z			= 1.0f;
	Verts[1].rhw		= 1.0f;
	Verts[1].uv.u		= 0.0f;
	Verts[1].uv.v		= 1.0f;
	Verts[1].color		= dwColor;

	Verts[2].x			= rect.right-0.5f;
	Verts[2].y			= rect.top-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].rhw		= 1.0f;
	Verts[2].uv.u		= 1.0f;
	Verts[2].uv.v		= 0.0f;
	Verts[2].color		= dwSecondColor;

	Verts[3].x			= rect.right-0.5f;
	Verts[3].y			= rect.bottom-0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].rhw		= 1.0f;
	Verts[3].uv.u		= 1.0f;
	Verts[3].uv.v		= 1.0f;
	Verts[3].color		= dwSecondColor;

	gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

	gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_LESSEQUAL );
}


void UIShell::DrawColorBox( UIRect rect, DWORD dwColor )
{
	DrawColorBox( rect, dwColor, dwColor );
}

void UIShell::ShiftInterface( const gpstring& sInterface, int xShift, int yShift )
{
	UIWindowMap::iterator i;
	GUInterfaceVec::iterator k;
	for ( k = m_UIInterfaces.begin(); k != m_UIInterfaces.end(); ++k )
	{
		if ( sInterface.same_no_case( (*k)->name ) )
		{
			UIWindowVec texts;
			UIWindowVec::iterator iText;
			for ( i = (*k)->ui_windows.begin(); i != (*k)->ui_windows.end(); ++i )
			{
				if ( (*i).second->GetType() == UI_TYPE_TEXT )
				{
					texts.push_back( (*i).second );
				}
				else
				{
					(*i).second->SetRect(	(*i).second->GetRect().left + xShift,
											(*i).second->GetRect().right + xShift,
											(*i).second->GetRect().top + yShift,
											(*i).second->GetRect().bottom + yShift );
				}
			}

			for ( iText = texts.begin(); iText != texts.end(); ++iText )
			{
				(*iText)->SetRect(	(*iText)->GetRect().left + xShift,
									(*iText)->GetRect().right + xShift,
									(*iText)->GetRect().top + yShift,
									(*iText)->GetRect().bottom + yShift );
			}

			return;
		}
	}
}


void UIShell::ShiftGroup( const gpstring& /*sInterface*/, const gpstring& sGroup, int xShift, int yShift )
{
	UIWindowVec::iterator i;
	UIWindowVec group = ListWindowsOfGroup( sGroup );
	for ( i = group.begin(); i != group.end(); ++i )
	{
		(*i)->SetRect(	(*i)->GetRect().left + xShift,
						(*i)->GetRect().right + xShift,
						(*i)->GetRect().top + yShift,
						(*i)->GetRect().bottom + yShift );
	}
}


bool UIShell::DoWindowsOverlap( UIWindow * pWindow1, UIWindow * pWindow2 )
{
	if ( pWindow1 && pWindow2 )
	{
		return pWindow1->GetRect().Intersects( pWindow2->GetRect() );
	}

	return false;
}


bool UIShell::DoWindowsOverlap( const char * szWindow1, const char * szInterface1,
								const char * szWindow2, const char * szInterface2 )
{
	return DoWindowsOverlap( FindUIWindow( szWindow1, szInterface1 ),
							 FindUIWindow( szWindow2, szInterface2 ) );
}


void UIShell::PlaceInterfaceOnTop( const char * interface_name )
{
	m_OnTopInterfaces.push_back( interface_name );
}


void UIShell::AddTipColor( gpstring sName, DWORD dwColor )
{
	if ( m_tipColors.find( sName ) == m_tipColors.end() )
	{
		m_tipColors.insert( TipColorPair( sName, dwColor ) );
	}
}


DWORD UIShell::GetTipColor( const char* sName, DWORD dwDefaultColor )
{
	TipColorMap::iterator i = m_tipColors.find( sName );
	if ( i != m_tipColors.end() )
	{
		return (*i).second;
	}

	return dwDefaultColor;
}


gpstring UIShell::GetTooltipColorName( DWORD dwColor )
{
	TipColorMap::iterator i;
	for ( i = m_tipColors.begin(); i != m_tipColors.end(); ++i )
	{
		if ( (*i).second == dwColor )
		{
			return (*i).first;
		}
	}	

	return "";
}


int UIShell::GetLineSpacer( gpstring sWindow )
{
	GuiLineSpacerMap::iterator iFind = m_lineSpacers.find( sWindow );
	if ( iFind != m_lineSpacers.end() )
	{
		return (*iFind).second;
	}

	return 0;
}


void UIShell::OnAppActivate( bool bActivate )
{
	if ( !bActivate )
	{
		SetLButtonDown( false );
		SetRButtonDown( false );

		gUIMessenger.SendUIMessage( UIMessage( MSG_GLOBALLBUTTONUP ) );
		gUIMessenger.SendUIMessage( UIMessage( MSG_GLOBALRBUTTONUP ) );
	}
}


void UIShell::InitImeUi()
{
	if ( !AppModule::DoesSingletonExist() )
	{
		return;
	}

	ImeUiCallback_SetFont				= GameGuiCallback_SetFont;
	ImeUiCallback_SetFontAttribute		= GameGuiCallback_SetFontAttribute;
	ImeUiCallback_SetTextColor			= GameGuiCallback_SetTextColor;
	ImeUiCallback_SetTextPosition		= GameGuiCallback_SetTextPosition;
	ImeUiCallback_DrawText				= GameGuiCallback_DrawText;
	ImeUiCallback_GetStringDimension	= GameGuiCallback_GetStringDimension;
	ImeUiCallback_DrawRect				= GameGuiCallback_DrawRect;
	ImeUiCallback_Malloc				= malloc;
	ImeUiCallback_Free					= free;
	ImeUiCallback_DrawFans				= GameGuiCallback_DrawFans;
		
	bool bDisable = !( LocMgr::DoesSingletonExist() && !gLocMgr.IsAnsi() 
		&& gLocMgr.GetLanguage() != MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED ) );
	
	ImeUi_Initialize( gAppModule.GetMainWnd(), bDisable );
	if ( bDisable )
	{
		return;
	}
	ImeUi_SetSupportLevel( 2 );

	FastFuelHandle hImeAppearance( "config:ime_appearance" );
	if ( hImeAppearance.IsValid() )
	{
		IMEUI_APPEARANCE ia;

		int temp = 0;
		hImeAppearance.Get( "candcolorbase", temp );
		ia.candColorBase = temp;

		hImeAppearance.Get( "candcolorborder", temp );
		ia.candColorBorder = temp;

		hImeAppearance.Get( "candcolortext", temp );
		ia.candColorText = temp;

		hImeAppearance.Get( "compcolorconverted", temp );
		ia.compColorConverted = temp;

		hImeAppearance.Get( "compcolorinput", temp );
		ia.compColorInput = temp;

		hImeAppearance.Get( "compcolorinputerr", temp );
		ia.compColorInputErr = temp;

		hImeAppearance.Get( "compcolortargetconv", temp );
		ia.compColorTargetConv = temp;

		hImeAppearance.Get( "compcolortargetnotconv", temp );
		ia.compColorTargetNotConv = temp;

		hImeAppearance.Get( "compcolortext", temp );
		ia.compColorText = temp;

		hImeAppearance.Get( "comptranslucence", temp );
		ia.compTranslucence = (BYTE)temp;

		hImeAppearance.Get( "symbolcolor", temp );
		ia.symbolColor = temp;

		hImeAppearance.Get( "symbolcolortext", temp );
		ia.symbolColorText = temp;

		hImeAppearance.Get( "symbolheight", temp );
		ia.symbolHeight = (BYTE)temp;

		hImeAppearance.Get( "symbolplacement", temp );
		ia.symbolPlacement = (BYTE)temp;

		hImeAppearance.Get( "symboltranslucence", temp );
		ia.symbolTranslucence = (BYTE)temp;

		float swirlTime = 0.0f;
		if ( hImeAppearance.Get( "swirl_time_increment", swirlTime ) )
		{
			ImeUi_SetSwirlTimeIncrement( (double)swirlTime );
		}

		ImeUi_SetAppearance( &ia );
	}

//	ImeUi_ToggleIme( true );
}


UIEditBox * UIShell::GetFocusedEditBox()
{
	UIWindowVec windows = ListWindowsOfType( UI_TYPE_EDITBOX );
	UIWindowVec::iterator i;

	for ( i = windows.begin(); i != windows.end(); ++i )
	{
		if ( ((UIEditBox *)(*i))->GetAllowInput() )
		{
			return (UIEditBox *)*i;
		}
	}

	return 0;
}


int UIShell::AddToItemMap( UIItem * pItem )
{
	ItemMap::iterator i = m_itemMap.find( pItem->GetItemID() );
	if ( i == m_itemMap.end() )
	{
		m_itemMap.insert( std::make_pair( pItem->GetItemID(), pItem ) );
	}
	
	return pItem->GetItemID();
}


void UIShell::RemoveFromItemMap( UIItem * pItem )
{
	ItemMap::iterator i = m_itemMap.find( pItem->GetItemID() );
	if ( i != m_itemMap.end() )
	{
		m_itemMap.erase( pItem->GetItemID() );
	}
}


UIItem * UIShell::GetItem( int item )
{
	ItemMap::iterator i = m_itemMap.find( item );
	if ( i != m_itemMap.end() )
	{
		return (*i).second;
	}
	else
	{
		if ( m_ItemCreationCb )
		{
			m_ItemCreationCb( item );
		}
		else
		{
			return NULL;
		}
		
		i = m_itemMap.find( item );
		if ( i == m_itemMap.end() )
		{
			return NULL;
		}
		else
		{
			return (*i).second;
		}
	}
}


void UIShell::ResetImeEditBuffer()
{
	if ( ImeUi_IsEnabled() )
	{
		ImeUI_CancelString();
	}
}


bool UIShell::IsMouseSettled( int numFrames )
{
	return ( numFrames <= m_mouseSettled );
}


void GameGuiCallback_SetFont(IMEUI_FONT hfont)
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->SetFont( (RapiFont *)hfont );
	}
}

void GameGuiCallback_SetFontAttribute( UINT uHeight, bool bBold )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->SetFontAttributes( uHeight, bBold );
	}
}

void GameGuiCallback_SetTextColor( DWORD color )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->SetImeColor( color );
	}
}

void GameGuiCallback_SetTextPosition( int x, int y )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->SetTextPosition( x, y );
	}
}

void GameGuiCallback_DrawText( const char* pszText )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->DrawText( ToUnicode( pszText ) );
	}
}

void GameGuiCallback_GetStringDimension( const char* szText, DWORD* puWidth, DWORD* puHeight )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		int width	= 0;
		int height	= 0;
		pEdit->GetFont()->CalculateStringSize( ToUnicode( szText ), width, height );
		*puWidth = width;
		*puHeight = height;
	}
}

void GameGuiCallback_DrawRect( int x1, int y1, int x2, int y2, DWORD color )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->DrawRect( UIRect( x1, y1, x2, y2 ), color );
	}
}

void GameGuiCallback_DrawFans( IMEUI_VERTEX* paVertex, UINT uNum )
{
	UIEditBox * pEdit = gUIShell.GetFocusedEditBox();
	if ( pEdit )
	{
		pEdit->DrawFans( paVertex, uNum );
	}
}