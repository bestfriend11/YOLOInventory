/**************************************************************************

Filename		: ui_shell.h

Description		: This is the main shell for the user interface.  The 
				  shell is what talks to the game and the game talks to 
			      the shell.  All window information is stored and maintained
				  by the shell.  

Creation Date	: 9/11/99

**************************************************************************/


#pragma once
#ifndef _UI_SHELL_H_
#define _UI_SHELL_H_


// Include Files
#include "ui_types.h"
#include "ui_interface.h"
#include "BlindCallback.h"

#include <vector>
#include <string>


// Forward Declarations
class UIWindow;
class FastFuelHandle;
class Rapi;
class IMG;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIEmotes;
class InputBinder;
class KeyInput;
class UIItem;
class UIEditBox;
class UITextBox;

// Defines
#ifndef _USE_INPUT_BINDER_
#define _USE_INPUT_BINDER_ 1
#endif

typedef CBFunctor1< int > UIItemCreationCb;	

class UIShell : public Singleton <UIShell>
{
public:

	// Public Methods
	UIShell	( Rapi& renderer, int screenWidth = 0, int screenHeight = 0 );	// Constructor
	~UIShell();																			// Destructor

	// Content Loading
	void		ActivateInterface	( FastFuelHandle fhInterface	);					// Activates an interface
FEX	void		ActivateInterface	( gpstring const & sInterfacePath, bool bShow );

FEX	void		DeactivateInterface	( const gpstring& interface_name	);				// Deactivates an interface
FEX	void		DeactivateInterfaceWindows( const gpstring& sInterface );
FEX	bool		DeleteWindow		( UIWindow * pWindow );
FEX	void		MarkInterfaceForActivation( const gpstring& interface_name, bool bShow );
FEX	void		MarkInterfaceForDeactivation( const gpstring& interface_name );
FEX	void		UnMarkInterfaceForDeactivation( const gpstring& sInterface );
	UIWindow *	Instantiate			( FastFuelHandle fhBlock, GUInterface *parent_interface, UIWindow *pParent = 0, gpstring sName = gpstring::EMPTY );	// Instantiate a block
FEX	void		ShowInterface		( const gpstring& interface_name );
FEX	void		HideInterface		( const gpstring& interface_name );
FEX	void		ShowGroup			( const char * szGroup, bool bShow, bool bMessage = false, const char * szInterface = NULL );
FEX	void		AddBlankInterface	( const gpstring& sName );
FEX	void		AddWindowToInterface( UIWindow *pWindow, const gpstring& sInterface, bool bExistFail = true );
	UIWindow *	CreateDefaultWindowOfType( UI_CONTROL_TYPE type );
FEX	UIWindow *	CreateDefaultWindowOfType( const gpstring& sType );
	void		ProcessWindowDeletionList();
	bool		IsModalInterfaceActive( gpstring sParent );
	void		ClearItems();
			
	// Update/Rendering
	void Update				( double seconds, int cursorX, int cursorY	);			// Update the UI
	void Draw				();														// Render the UI
	void DrawInterface		( gpstring sInterface	);
	void DrawCursor			();														// Render just the cursor
	void DrawItems			();														// Render the items
	void DrawStatusBars		();
	void DrawUiIme			();

	// Resize the UI
	void Resize				( int width, int height		);

	// Find a window
FEX	UIWindow		* FindUIWindow	( const char* window_name, const char* sInterface = NULL );
	GUInterface		* FindInterface	( const char* interface_name	);
	UIItem			* FindItemByID	( int id						);
	void			  FindActiveItems( UIItemVec & item_vec );
FEX	void			  AddActiveItem( UIWindow * pItem );
FEX	void			  RemoveActiveItem( UIWindow * pItem );
FEX	void			  ClearActiveItems() { m_activeItems.clear(); }
FEX	int				  GetNumActiveItems( const gpstring& sInterface );

	// Messaging helper
FEX bool SendUIMessage( eUIMessage code, const char* window_name, const char* sInterface );
FEX bool SendUIMessage( eUIMessage code, const char* window_name )	{ return SendUIMessage( code, window_name, NULL ); }

	// Texture Manager Access
	UITextureManager		*GetTextureman()	{ return m_pTextureman; };

	// Messenger Manager Access
	Messenger				*GetMessenger()		{ return m_pMessenger;	};

	// Animation Manager Access
	UIAnimation				*GetAnimation()		{ return m_pAnimation;	};

	Rapi &					GetRenderer()		{ return m_rapi;		};

	// Input Handling Routines
	InputBinder	*	GetInputBinder		()	{ return m_pInputBinder; };
	void PublishInterfaceToInputBinder	();
	void BindInputToPublishedInterface	();	
	bool Input_OnMouseMove				( int x, int y );
	bool Input_OnMouseLeftButtonDown	();
	bool Input_OnMouseLeftButtonUp		();
	bool Input_OnMouseRightButtonDown	();
	bool Input_OnMouseRightButtonUp		();
	bool Input_OnMouseLeftDoubleClick	();
	bool Input_OnMouseRightDoubleClick	();
	bool Input_OnMouseWheelUp			();
	bool Input_OnMouseWheelDown			();
	bool Input_OnKeypress				( const KeyInput& key );
	bool Input_OnChar					( wchar_t c );
	bool Input_OnEnter					();
	bool Input_OnEscape					();
	
#if !_USE_INPUT_BINDER_	
	bool Input_OnKeypress				( int key );
#endif

	// Visibility/Activity
FEX	void SetVisible( bool bVisible )	{ m_bVisible = bVisible;	};
FEX	bool GetVisible() const				{ return m_bVisible;		};

FEX	void SetGuiDisabled( bool bDisable )	{ m_bDisabled = bDisable;  }
FEX	bool GetGuiDisabled() const				{ return m_bDisabled; }

	// Specialized Interface Handling/Query
	void	InitializeInterfaceType( GUInterface *ui_interface, gpstring interface_type, FastFuelHandle fhInterface );
	void	InitializePulloutInterface( gpstring interface_name, gpstring reference_interface, bool bWidth = true );
	void	InitializePulloutInterfaceToWindow( gpstring interface_name, gpstring reference_window, PULLOUT_MODE mode = X_ALIGN_RIGHT, int overlap = 0 );
	void	AlignWindowToWindow( gpstring sDest, gpstring sSource, PULLOUT_MODE mode, int overlap = 0 );	
	void	SetGroupScaleToAlignWithWindow( gpstring sGroup, gpstring sWindow, float scale );
FEX	void	ShiftInterface( const gpstring& sInterface, int xShift, int yShift );
FEX	void	ShiftGroup( const gpstring& sInterface, const gpstring& sGroup, int xShift, int yShift );
FEX	bool	DoWindowsOverlap( UIWindow * pWindow1, UIWindow * pWindow2 );
FEX	bool	DoWindowsOverlap( const char * szWindow1, const char * szInterface1,
							  const char * szWindow2, const char * szInterface2 );
FEX	void	PlaceInterfaceOnTop( const char * interface_name );

	// Cursor Switching
FEX	UICursor *	LoadUICursor( const gpstring& name, const gpstring& sInterface );
FEX	UICursor *	GetActiveCursor()						{ return m_pActiveCursor; }
FEX	void		SetActiveCursor( UICursor * pCursor )	{ m_pActiveCursor = pCursor; }

	// Mouse movement rollover consumption
	bool GetRolloverConsumed() { return m_bRolloverConsumed; }
	
	// List windows of a certain type
	UIWindowVec	ListWindowsOfType		( UI_CONTROL_TYPE type );
	UIWindowVec ListWindowsOfGroup		( gpstring group );
	UIWindowVec ListWindowsOfDockGroup	( gpstring group );
	UIWindowVec ListWindowsOfRadioGroup ( gpstring group );

	// Retrieve Input information for messaging
FEX	bool GetLButtonDown() const			{ return m_lbuttondown; };
FEX	void SetLButtonDown( bool lbutton ) { m_lbuttondown = lbutton; };
FEX	bool GetRButtonDown() const			{ return m_rbuttondown; };
FEX	void SetRButtonDown( bool rbutton ) { m_rbuttondown = rbutton; };
FEX	int	 GetMouseX() const				{ return m_mouseX; }
FEX	void SetMouseX( int x )				{ m_mouseX = x; }
FEX	int  GetMouseY() const				{ return m_mouseY; }
FEX	void SetMouseY( int y )				{ m_mouseY = y; }

	// Returns whether or not a particular interface is visible
FEX	bool IsInterfaceVisible ( const gpstring& interface_name );	

	void LoadCommonArt();
	void UnloadCommonArt();
	const unsigned int GetCommonArt( gpstring sArt );
	bool IsCommonArt( unsigned int texture );
	bool IsCommonArt( gpstring & sTexture );

	void NormalizeInterfaceToResolution( gpstring sInterface );
	void CenterInterface( GUInterface & gui );

	GUInterfaceVec & GetInterfaces() { return m_UIInterfaces; }

	gpstring GetTextureNamingKeyName( gpstring sFilename );

FEX	int GetScreenWidth() const { return m_screenWidth; }
FEX	int GetScreenHeight() const { return m_screenHeight; }

	void SetScreenDimensions( int screenWidth, int screenHeight );

	int		GetRolloverItemslotID() { return m_rolloverItemID; }
	void	SetRolloverItemslotID( int id ) { m_rolloverItemID = id;	}
	float	GetRolloverDelay()				{ return m_RolloverDelay;	}
	void	SetRolloverDelay( float delay ) { m_RolloverDelay = delay;	}
	
	void	InsertDragBox( UIRect rect, DWORD dwColor );
	void	DrawDragBoxes();
	void	DrawColorBox( UIRect rect, DWORD dwColor, DWORD dwSecondColor );
	void	DrawColorBox( UIRect rect, DWORD dwColor );

	bool OnDeltaXCursor( INT32 deltaX ) { m_deltaX = deltaX; return false; }
	bool OnDeltaYCursor( INT32 deltaY ) { m_deltaY = deltaY; return false; }

	void SetProcessDeletion( bool bProcess )	{ m_bProcessDeletion = bProcess; }
	bool GetProcessDeletion()					{ return m_bProcessDeletion; }

	void SetGridPlace( bool bPlace )	{ m_bGridPlace = bPlace; }
	bool GetGridPlace()					{ return m_bGridPlace; }

	void SetToolTipsVisible( bool bVisible )	{ m_bShowToolTips = bVisible; }
	bool GetToolTipsVisible()					{ return m_bShowToolTips; }

	void InitImeUi();
	UIEditBox * GetFocusedEditBox();
	void ResetImeEditBuffer();

	void		AddTipColor( gpstring sName, DWORD dwColor );
	DWORD		GetTipColor( const char* sName, DWORD dwDefaultColor = 0xffffffff );
	gpstring	GetTooltipColorName( DWORD dwColor );

	void SetItemActive( bool bActive )	{ m_bItemActive = bActive;	}
	bool GetItemActive()				{ return m_bItemActive;		}
	
	void SetDrawCursor( bool bDraw )	{ m_bDrawCursors = bDraw;	}
	bool GetDrawCursor()				{ return m_bDrawCursors;	}

	void SetImeToggle( bool bToggle )	{ m_bImeToggle = bToggle;	}
	bool GetImeToggle()					{ return m_bImeToggle;		}

	void SetRolloverName( gpstring sName )	{ m_sRolloverName = sName; }
	const gpstring & GetRolloverName()				{ return m_sRolloverName; }

	const gpstring & GetDefaultTooltipName()		{ return m_sDefaultTooltipName; }

	int GetLineSpacer( gpstring sWindow );		

	bool GetIsShuttingDown()			{ return m_bShuttingDown; }
	void SetIsShuttingDown( bool bSet ) { m_bShuttingDown = bSet; }

	int  GetDragTolerance()				{ return m_dragTolerance; }

	void OnAppActivate( bool bActivate );

	bool IsMouseSettled( int numFrames );

	int  AddToItemMap( UIItem * pItem );
	void RemoveFromItemMap( UIItem * pItem );
	UIItem * GetItem( int item );

	void RegisterItemCreationCb( UIItemCreationCb cb ) { m_ItemCreationCb = cb; }
	
	void SetIgnoreItemTexture( bool bSet )	{ m_bIgnoreItemTexture = bSet; }
	bool GetIgnoreItemTexture()				{ return m_bIgnoreItemTexture; }

private:
	
	typedef std::vector< std::pair< gpstring, bool > > ActivateVec;
	typedef std::map< gpstring, int, istring_less > GuiLineSpacerMap;
	typedef std::map< int, UIItem * > ItemMap;	

	// Private Member Variables	
	GUInterfaceVec					m_UIInterfaces;		// List of all available interfaces	
	UIWindowVec						m_drawCursors;
	UIWindowVec						m_drawItems;
	UIWindowSet						m_activeItems;
	ActivateVec						m_ActivateInterfaces;
	StringVec						m_OnTopInterfaces;
	int								m_screenWidth;		// Holds the current screen width
	int								m_screenHeight;		// Holds the current screen height
	Rapi&							m_rapi;				// Reference to the rapi object	
	UITextureManager				*m_pTextureman;		// Texture management object
	Messenger						*m_pMessenger;		// Messenger object
	UIAnimation						*m_pAnimation;		// Window Animation object
	UIEmotes						*m_pEmotes;			// A class that manages all our emotes in text windows!
	int								m_mouseX;			// Mouse y position
	int								m_mouseY;			// Mouse x position
	bool							m_lbuttondown;
	bool							m_rbuttondown;	
	InputBinder						*m_pInputBinder;	// For input management
	bool							m_bVisible;			// Is the interface visible/active?
	double							m_seconds;			// Update seconds 
	bool							m_bRolloverConsumed;// Is the rollover consumed by the GUI?
	bool							m_bShowToolTips;
	UICommonArtMap					m_common_art;
	int								m_rolloverItemID;
	DragBoxColl						m_dragBoxes;
	int								m_deltaX;
	int								m_deltaY;
	bool							m_bProcessDeletion;
	bool							m_bGridPlace;
	TipColorMap						m_tipColors;
	bool							m_bItemActive;
	bool							m_bDrawCursors;
	bool							m_bImeToggle;
	gpstring						m_sRolloverName;
	gpstring						m_sDefaultTooltipName;
	GuiLineSpacerMap				m_lineSpacers;
	UICursor *						m_pActiveCursor;	
	bool							m_bShuttingDown;
	int								m_dragTolerance;
	int								m_mouseSettled;
	bool							m_bDisabled;
	ItemMap							m_itemMap;
	UIItemCreationCb				m_ItemCreationCb;
	bool							m_bIgnoreItemTexture;
	float							m_RolloverDelay;
	float							m_RolloverTimer;


	FUBI_SINGLETON_CLASS( UIShell, "GameGUI main shell." );
};

#define gUIShell UIShell::GetSingleton()


#endif
