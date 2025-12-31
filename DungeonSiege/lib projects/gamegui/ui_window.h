/**************************************************************************

Filename		: ui_window.h

Description		: Contains the standard definitions of the windows 
				  used in the GameGUI

Creation Date	: 9/18/99

**************************************************************************/


#pragma once
#ifndef _UI_WINDOW_H_
#define _UI_WINDOW_H_

// Include Files
#include "BlindCallback.h"
#include "ui_types.h"
#include "ui_scroll_window.h"
#include "FuBiDefs.h"


// Forward Declarations
class FastFuelHandle;
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;
class UITextBox;

// Defines
typedef std::vector< UI_EVENT >			ui_event_vector;
typedef std::vector< EVENT_ACTION >		ui_action_vector;


// Structures
struct ANCHOR_DATA
{
	bool bBottomAnchor;
	bool bRightAnchor;
	bool bLeftAnchor;
	bool bTopAnchor;
	int	 bottom_anchor;
	int	 right_anchor;
	int	 left_anchor;
	int	 top_anchor;
};


// This class outlines the standard UIWindow functionality.  All other UI controls
// will be derived from this class.  
class UIWindow : public UIScrollWindow
{
public:

	// Public Methods
	
	// Constructor
	UIWindow	( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );	
	UIWindow	();	

	// Destructor
	virtual ~UIWindow	();		

	// Name information
FEX	void				SetName( const gpstring& name )		{ m_szName = name; };
FEX const gpstring &	GetName() const						{ return m_szName; };
	
	// Set/Get Group name
FEX	void				SetGroup( const gpstring& group )	{ m_group = group;	};
FEX const gpstring &	GetGroup() const					{ return m_group;	};

FEX	void				SetDockGroup( const gpstring& group ){ m_dock_group = group; }
FEX const gpstring &	GetDockGroup() const				{ return m_dock_group; }
	
	// Type information
FEX	void				SetType( UI_CONTROL_TYPE type )		{ m_type = type; };
FEX	UI_CONTROL_TYPE 	GetType() const						{ return m_type; };
FEX	void				SetInputType( UI_INPUT_TYPE type )	{ m_secondary_type = type; };
FEX	UI_INPUT_TYPE 		GetInputType() const				{ return m_secondary_type; };

	// Interface Origin
FEX	void				SetInterfaceParent( const gpstring& parent_name )	{ m_szInterface = parent_name; };
FEX const gpstring &	GetInterfaceParent() const					{ return m_szInterface; };

	// Window information
FEX	void				SetParentWindow	( UIWindow *parent )		{ m_parentWindow = parent;			};
FEX UIWindow *			GetParentWindow	() const					{ return m_parentWindow;			};
	// $$$ this function should be const - using GetRect() to modify the rect
	//     bypasses virtual SetRect(), which may have unexpected results
	//     depending on the control type. -sb
FEX UIRect &			GetRect			()							{ return m_rect;					};
FEX int					GetScreenWidth	() const					{ return m_screenWidth;				};
FEX int					GetScreenHeight	() const					{ return m_screenHeight;			};	

	// Window resizing
FEX	void				Resize				( int width, int height );
	virtual void		SetRect				( int left, int right, int top, int bottom, bool bAnimation = false );
FEX	void   FUBI_RENAME( SetRect )			( int left, int right, int top, int bottom, bool bAnimation )  {  SetRect( left, right, top, bottom, bAnimation );  }
	void				SetNormalizedRect	( double left, double right, double top, double bottom );
FEX	void   FUBI_RENAME( SetNormalizedRect )	( float left, float right, float top, float bottom )		{ SetNormalizedRect( left, right, top, bottom ); }
FEX	const UINormalizedRect & GetNormalizedRect() const													{ return m_rectNormalized; };
	void				SetUVRect			( double left, double right, double top, double bottom );
FEX	void   FUBI_RENAME( SetUVRect )			( float left, float right, float top, float bottom )		{ SetUVRect( left, right, top, bottom ); }
FEX	const UINormalizedRect & GetUVRect		() const													{ return m_uv_rect; };
FEX	void				SetPulloutOffset	( int offset ) { m_pullout_offset = offset; };
FEX	int 				GetPulloutOffset	() const													{ return m_pullout_offset; };
FEX int					GetWidth() const	{ return (m_rect.right-m_rect.left); }
FEX int					GetHeight() const	{ return (m_rect.bottom-m_rect.top); }
	virtual void		ResizeToCurrentResolution( int original_width, int original_height );
	virtual void		AdjustChildren()	{};
	void				UpdateAnchorData	( ANCHOR_DATA anchorData );

	// Rendering
	virtual void		Draw		();
	void				DrawComponent( UIRect rect, unsigned int tex, float alpha = 1.0f, UINormalizedRect uv_rect = UINormalizedRect(), bool bWrap = false, DWORD dwVertexColor = 0x00FFFFFF  );	
	void				DrawColorBox( UIRect rect, DWORD dwColor, DWORD dwBorder );
	void				DrawDragBox();
	virtual bool		LoadTexture	( gpstring texture, bool bResizeWindow = false );
FEX	void   FUBI_RENAME( LoadTexture )( const gpstring& texture, bool bResizeWindow )  {  LoadTexture( texture, bResizeWindow );  }
FEX	void				LoadTexture ( unsigned int texture );
FEX	void				SetAlpha	( float alpha )				{ m_alpha = alpha;			};
FEX	float 				GetAlpha	() const					{ return m_alpha;			};
	virtual void		SetVisible	( bool bVisible );			
FEX void   FUBI_RENAME( SetVisible )( bool bVisible )			{ SetVisible( bVisible );	}
FEX bool 				GetVisible	() const					{ return m_bVisible;		};
FEX void				SetBorder	( bool bBorder )			{ m_bHasBorder = bBorder;	};
FEX bool 				GetBorder	() const					{ return m_bHasBorder;		}
FEX void				SetBorderColor( unsigned int color )	{ m_BorderColor = color;    }
FEX DWORD				GetBorderColor() const					{ return m_BorderColor;		}
FEX void				SetBackgroundColor( unsigned int color ){ m_BgColor = color;	}
FEX bool				GetBackgroundFill() const				{ return m_bBgFill; }
FEX void				SetBackgroundFill( bool fill )			{ m_bBgFill = fill; }
FEX void				SetBorderPadding( int padding )			{ m_borderPadding = padding; }
FEX int					GetBorderPadding() const				{ return m_borderPadding;	}

	// Updating
	virtual void		Update( double seconds );

	// Get/Set Draw Order
FEX void				SetDrawOrder( int order )			{ m_draw_order = order;		};
FEX	int					GetDrawOrder() const				{ return m_draw_order;		};

	// Get/Set Texture State
FEX	void				SetHasTexture( bool bTexture )		{ m_bTexture = bTexture;	};
	bool 				HasTexture	 () const				{ return m_bTexture;		};
FEX	bool   FUBI_RENAME( GetHasTexture ) () const			{ return HasTexture();		};

	// Retreives the texture index
FEX	void				SetTextureIndex( unsigned int id )	{ m_texture_index = id; };
FEX	unsigned int		GetTextureIndex() const				{ return m_texture_index;	};
	unsigned int & 		GetTextureIndex()					{ return m_texture_index;	};
															
	// Texture tile/clamp information						
FEX	void				SetTiledTexture( bool bTiled )		{ m_bTiled = bTiled;		};
FEX	bool				GetTiledTexture() const				{ return m_bTiled;			};

	// Tooltips
	void				UpdateToolTip( double seconds );
FEX	void				SetToolTip( const gpwstring& HelpText, const gpstring& HelpTextBox, bool bIsToolTip = false );
FEX	void				HideToolTip();
FEX	bool				IsToolTipVisible() const								{ return m_bToolTipVisible; }
FEX	UITextBox *			GetToolTipBox() const									{ return m_pHelpBox; }
FEX	void				ClearToolTip()											{ m_sHelpText.clear(); m_bHasToolTip = false; m_bHasHelpText = false; m_pHelpBox = NULL; }
FEX	bool				GetHasToolTip() const									{ return m_bHasToolTip; }
FEX	const gpstring &	GetHelpBoxName() const									{ return m_sHelpTextBox; }
FEX	void				SetHelpBoxName( const gpstring& helpTextBox ) 			{ m_sHelpTextBox.assign(helpTextBox); }
	void				PositionTextBoxOutside();
	void				UpdateHelpText( );
	void				UpdateHelp( );

	// Drag information
FEX	void				SetDraggable( bool bDraggable )		{ m_bDrag = bDraggable;			};
FEX	bool 				GetDraggable() const				{ return m_bDrag;				};
FEX	void				SetDragGridX( int x )				{ m_drag_grid_x = x;			};
FEX	int					GetDragGridX() const				{ return m_drag_grid_x;			};
FEX	void				SetDragGridY( int y )				{ m_drag_grid_y = y;			};
FEX	int					GetDragGridY() const				{ return m_drag_grid_y;			};
	void				SetDragTime( double time )			{ m_drag_time = time;			};
FEX	void   FUBI_RENAME( SetDragTime )( float time )			{ SetDragTime( time );			};
FEX	double				GetDragTime() const					{ return m_drag_time;			};
	void				SetDragTimeElapsed( double time )	{ m_drag_time_elapsed = time;	};
FEX	void   FUBI_RENAME( SetDragTimeElapsed )( float time )	{ SetDragTimeElapsed( time );	};
FEX	double				GetDragTimeElapsed() const			{ return m_drag_time_elapsed;	};
FEX	void				SetDragAllowed( bool bAllowed );		
FEX	bool 				GetDragAllowed() const				{ return m_bDragAllowed;		};
FEX	void				DragWindow( int delta_x, int delta_y );
															
	// Event/Message Accessors								
	std::vector< UI_EVENT > * GetEvents()					{ return &m_events;			};	
	
	// These are generic methods use as a way to set an index or id for a certain window.  
	// It is especially useful because it can be used to cut down the number of messages you
	// pass around to the game.
FEX	int					GetIndex() const		{ return m_index;	}
FEX	void				SetIndex( int index )	{ m_index = index;	}
	virtual void		GetAnimationTextures( std::vector< unsigned int > & /*textures*/ ) {}

	// Receive messages
FEX	bool				ReceiveMessage( const UIMessage& msg );

	// For message processing - MUST be filled in by specific control
	virtual bool 		ProcessAction	( UI_ACTION action, gpstring parameter );
FEX	bool   FUBI_RENAME(	ProcessAction )	( UI_ACTION action, const gpstring& parameter ) { return ( ProcessAction( action, parameter ) );  }
	virtual bool 		ProcessMessage	( UIMessage & msg );
FEX	bool   FUBI_RENAME(	ProcessMessage )( UIMessage & msg ) { return ( ProcessMessage( msg ) );  }

	// Convert a standard string into an event action 
	EVENT_ACTION ConvertStringToAction( gpstring action );
	eUIMessage	 ConvertStringToMessageCode( gpstring message );

	// Hit test the visible windows using the current mouse position
	virtual bool HitDetect( int x, int y );

	// Set the current mouse position - you must define this function in classes that need it
	virtual void SetMousePos( unsigned int x, unsigned int y ) { m_mouseX = x; m_mouseY = y; };
FEX	void	SetMouseX( int x )		{ m_mouseX = x;		}
FEX	int		GetMouseX() const		{ return m_mouseX;	}
FEX	void	SetMouseY( int y )		{ m_mouseY = y;		}
FEX	int		GetMouseY() const		{ return m_mouseY;	}

FEX	void	SetRollover( bool bRollover ) { m_bRollover = bRollover; }
FEX	bool	GetRollover() const			{ return m_bRollover; }
	
	// Window Message Receive States
FEX	bool	IsConsumable() const	{ return m_bConsumable; }
FEX	void	SetConsumable( bool bConsumable );
FEX	bool	IsPassThrough() const	{ return m_bPassThrough; }
FEX	void	SetPassThrough( bool bPass ) { m_bPassThrough = bPass; }

FEX bool	IsEnabled() const		{ return m_bEnabled; }
FEX	void	SetEnabled( bool bEnabled );

	// Scaling 
	virtual void		SetScale	( float scale );
FEX void   FUBI_RENAME( SetScale )	( float scale )	{ SetScale( scale ); }
FEX	float				GetScale	() const		{ return m_scale; }

FEX	void			AddChild( UIWindow * pWindow );
FEX	void			RemoveChild( UIWindow * pWindow );
FEX	UIWindowVec &	GetChildren() { return m_children; }

FEX	void	SetTag( long tag )	{ m_tag = tag;  }
FEX long	GetTag() const		{ return m_tag; }

FEX	bool	GetNormalizeResize() const { return m_bNormalizeResize; }
FEX	void	SetNormalizeResize( bool bSet ) { m_bNormalizeResize = bSet; }

	ANCHOR_DATA & GetAnchorData() { return m_anchor_data; }

FEX	void	SetMarkForDeletion( bool bDelete )	{ m_bMarkedForDeletion = bDelete; }
FEX	bool	GetMarkedForDeletion() const		{ return m_bMarkedForDeletion; }

	// Common Control Functions
FEX	bool				GetIsCommonControl	() const { return m_bCommon; }
FEX	void				SetIsCommonControl	( bool bSet ) { m_bCommon = bSet; }
	virtual void		CreateCommonCtrl	( gpstring sTemplate ) {}
FEX	void   FUBI_RENAME( CreateCommonCtrl )	( const gpstring& sTemplate )  {  CreateCommonCtrl( sTemplate );  }
FEX	const gpstring &	GetCommonTemplate	() const { return m_sCommonTemplate; }
FEX	void				SetCommonTemplate	( const gpstring& sTemplate = gpstring::EMPTY ) { m_sCommonTemplate = sTemplate; }

FEX	void SetTopmost( bool bSet )	{ m_bTopmost = bSet; }
FEX	bool GetTopmost() const			{ return m_bTopmost; }

FEX	const gpstring & GetRolloverKey() const { return m_sRolloverKey; }
FEX	void SetRolloverKey( const gpstring& sKey );

#if !GP_RETAIL
	// Persistence
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	// Callbacks
	typedef CBFunctor3< UI_ACTION, const gpstring&, UIWindow* > UIActionCb;
	static void SetUIActionCb( UIActionCb cb )	{ ms_uiActionCb = cb; }

FEX	bool	GetHidden() const		{ return m_bHidden; }

FEX	bool	HasFocus() const		{ return m_bHasFocus; }

private:

	// Private Member Variables
	UI_CONTROL_TYPE			m_type;					// The type of the control
	UI_INPUT_TYPE			m_secondary_type;		// The secondary ( input range ) type	
	ANCHOR_DATA				m_anchor_data;			// Holds the anchor information
	UINormalizedRect		m_rectNormalized;		// Holds the normalized coordinates of a window
	UINormalizedRect		m_uv_rect;				// Holds the UV coordinates of the texture
	UIRect					m_rect;					// Holds the pixel coordinates of a window
	UIRect					m_default_rect;			// This stores the windows default position
	gpstring				m_szInterface;			// The name of the interface this window came from
	UIWindow				*m_parentWindow;		// Name of parent window
	UIWindowVec				m_children;				// Windows that are have this window as a parent
	gpstring				m_szName;				// Name of window
	gpstring				m_group;				// Name of the group this window belongs to
	gpstring				m_dock_group;
	gpwstring				m_sHelpText;			// Rollover help text for this window.
	UITextBox *				m_pHelpBox;				// Rollover help box for this window.
	gpstring				m_sHelpTextBox;			// Rollover help box window name.
	bool					m_bHelpTextVisible;		// Visiblity flag for our help text.
	gpstring				m_sRolloverKey;			// Rollover key lookup
	bool					m_bHasHelpText;			// this window has rollover help text
	bool					m_bHasToolTip;			// this window has a tooltip
	bool					m_bToolTipVisible;		// is the tooltip currently visible
	double					m_ToolTipDelay;			// delay showing tooltip while holding the mouse over the window
	double					m_MouseHoldTime;		// time the cursor is held over the window
	double					m_LastRealTime;
	int						m_ToolTipX;
	int						m_ToolTipY;
	int						m_screenWidth;			// Holds the current screen width
	int						m_screenHeight;			// Holds the current screen height	
	unsigned int			m_texture_index;		// Index of the window texture
	float					m_alpha;				// Alpha value of window
	bool					m_bVisible;				// Is window visible
	std::vector< UI_EVENT > m_events;				// What messages/events does this window handle	
	int						m_draw_order;			// Specifies the draw order
	bool					m_bTexture;				// Specifies if the window has a texture
	bool					m_bTiled;				// Is the texture tiled
	int						m_pullout_offset;		// Offset if the window derived from a pullout interface
	int						m_index;				// Generic/Optional index parameter - can be used for anything
	unsigned int			m_mouseX;				// Mouse X position
	unsigned int			m_mouseY;				// Mouse Y position
	bool					m_bNormalized;			// Are original coordinates normalized?
	bool					m_bRollover;			// Is the cursor over the window?
	bool					m_bMarkedForDeletion;
	bool					m_bCanHitDetect;
	bool					m_bConsumable;			// only this window receives input in the draw order
	bool					m_bEnabled;				// this window can receive messages
	bool					m_bHasBorder;			// draw a border around the window
	unsigned int			m_BorderColor;			// color of window border
	int						m_borderPadding;		
	unsigned int			m_BgColor;				// window background color
	bool					m_bBgFill;				// fill solid background
	bool					m_bPassThrough;			// If this is set, then the game gui will report that the input
													// is not handled by the GUI, and must be passed to the game.
	long					m_tag;
	bool					m_bStretchX;
	bool					m_bStretchY;
	bool					m_bCenterX;
	bool					m_bCenterY;
	bool					m_bNormalizeResize;
	bool					m_bTopmost;
	bool					m_bHidden;

	bool					m_bHasFocus;

	// Window Dragging Variables
	bool					m_bDrag;
	double					m_drag_time;
	double					m_drag_time_elapsed;
	int						m_drag_grid_x;
	int						m_drag_grid_y;
	bool					m_bDragAllowed;
	vector_3				m_marquee_color;

	// For Window Scaling
	bool					m_bScaled;
	float					m_scale;
	
	// For Common Control Information Queries
	bool					m_bCommon;
	gpstring				m_sCommonTemplate;

	// For Callbacks
	static UIActionCb ms_uiActionCb;
};
#endif
