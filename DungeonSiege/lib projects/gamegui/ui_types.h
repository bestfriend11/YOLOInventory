/**************************************************************************

Filename		: ui_types.h

Description		: Contains all pertinent structures and data types
				  concerning UI-based windows and their messages/properties.

				  note: a "gas" string is the name that will appear in gas,
				  as opposed to skrit.

Creation Date	: 9/11/99

**************************************************************************/


#pragma once
#ifndef _UI_TYPES_H_
#define _UI_TYPES_H_

#include "WinGeometry.h"

#include <list>
#include <vector>
#include <map>
#include <set>


// Constants
const int DRAW_ORDER_TOP = 65535;


// Enumerated Data Types
enum UI_CONTROL_TYPE
{
	SET_BEGIN_ENUM( UI_TYPE_, 0 ),

	UI_TYPE_WINDOW,
	UI_TYPE_BUTTON,
	UI_TYPE_CHECKBOX,
	UI_TYPE_SLIDER,
	UI_TYPE_LISTBOX,
	UI_TYPE_RADIOBUTTON,
	UI_TYPE_MULTISTAGEBUTTON,
	UI_TYPE_TEXT,
	UI_TYPE_CURSOR,
	UI_TYPE_DOCKBAR,
	UI_TYPE_GRIDBOX,
	UI_TYPE_POPUPMENU,
	UI_TYPE_ITEM,
	UI_TYPE_ITEMSLOT,
	UI_TYPE_INFOSLOT,
	UI_TYPE_STATUSBAR,
	UI_TYPE_EDITBOX,
	UI_TYPE_COMBOBOX,
	UI_TYPE_MOUSE_LISTENER,
	UI_TYPE_LISTREPORT,
	UI_TYPE_CHATBOX,
	UI_TYPE_TEXTBOX,
	UI_TYPE_DIALOGBOX,
	UI_TYPE_TAB,
	UI_TYPE_LINE,


	SET_END_ENUM( UI_TYPE_ ),
};

FEX const char* ToString     ( UI_CONTROL_TYPE e );
    const char* ToGasString  ( UI_CONTROL_TYPE e );
    bool        FromString   ( const char* str, UI_CONTROL_TYPE& e );
    bool        FromGasString( const char* str, UI_CONTROL_TYPE& e );


// Secondary Types - This type is used to determine the range of input
// the window can listen for.
enum UI_INPUT_TYPE
{
	SET_BEGIN_ENUM( TYPE_INPUT_, 0 ),

	TYPE_INPUT_NONE,	// Don't pay attention to any input
	TYPE_INPUT_ALL,		// Catch-all input solution
	TYPE_INPUT_MOUSE,	// Only listen and receive all mouse input messages
	TYPE_INPUT_KEY,		// Only listen and receive all keyboard input messages

	SET_END_ENUM( TYPE_INPUT_ ),
};

FEX const char* ToString     ( UI_INPUT_TYPE e );
    bool        FromString   ( const char* str, UI_INPUT_TYPE& e );


// Valid UI Actions
enum UI_ACTION
{
	SET_BEGIN_ENUM( ACTION_, 0 ),

	ACTION_SETRECT,
	ACTION_SETNORMALIZEDRECT,
	ACTION_SETVISIBLE,
	ACTION_SHOWGROUP,
	ACTION_LOADTEXTURE,
	ACTION_SETUVCOORDS,
	ACTION_MESSAGE,
	ACTION_PARENTMESSAGE,
	ACTION_SETALPHA,
	ACTION_ADD_ELEMENT,
	ACTION_REMOVE_ELEMENT,
	ACTION_NOTIFY,
	ACTION_SETHOTSPOT,
	ACTION_SETTEXT,
	ACTION_SETGROUP,
	ACTION_SETELEMENTHEIGHT,
	ACTION_SETNUMSTATES,
	ACTION_SETSTATE,
	ACTION_SHOWMENU,
	ACTION_ACTIVATEMENU,
	ACTION_ACTIVATEITEM,
	ACTION_SETCOLOR,
	ACTION_PLAYSOUND,
	ACTION_KEYLISTEN,
	ACTION_KEYPRESS,
	ACTION_RECTANIMATION,
	ACTION_ALPHAANIMATION,
	ACTION_FLASHANIMATION,
	ACTION_CLOCKANIMATION,
	ACTION_SETSTATUS,
	ACTION_LOADANIMATEDTEXTURE,
	ACTION_CONSUMEINPUT,
	ACTION_EDITSELECT,
	ACTION_SHIFT_X,
	ACTION_SHIFT_Y,
	ACTION_SETROLLOVERHELP,
	ACTION_VERTEXCOLOR,
	ACTION_COMMAND,
	ACTION_CALL,
	// this is the default/unknown/none action (doesn't do anything)
	ACTION_NONE,

	SET_END_ENUM( ACTION_ ),
};

FEX const char* ToString     ( UI_ACTION e );
    const char* ToGasString  ( UI_ACTION e );
    bool        FromString   ( const char* str, UI_ACTION& e );
    bool        FromGasString( const char* str, UI_ACTION& e );



// Messages
enum eUIMessage
{
	SET_BEGIN_ENUM( MSG_, 0 ),

	MSG_NONE,
	MSG_INVISIBLE,
	MSG_VISIBLE,
	MSG_LBUTTONDOWN,
	MSG_LBUTTONUP,
	MSG_RBUTTONDOWN,
	MSG_RBUTTONUP,
	MSG_MBUTTONDOWN,
	MSG_MBUTTONUP,
	MSG_BUTTONPRESS,
	MSG_RBUTTONPRESS,
	MSG_ROLLOVER,
	MSG_ROLLOFF,
	MSG_CHECK,
	MSG_UNCHECK,
	MSG_UNCHECKED,	// This means that the button WAS checked and now isn't
	MSG_FORCECHECK,
	MSG_FORCEUNCHECK,
	MSG_LBUTTONDOWNCHECKED,
	MSG_LBUTTONUPCHECKED,
	MSG_ROLLOVERCHECKED,
	MSG_ROLLOFFCHECKED,
	MSG_GLOBALLBUTTONUP,
	MSG_GLOBALLBUTTONUPOFF,
	MSG_GLOBALLBUTTONDOWN,
	MSG_GLOBALRBUTTONUP,
	MSG_GLOBALRBUTTONUPOFF,
	MSG_GLOBALRBUTTONDOWN,
	MSG_GLOBALMBUTTONUP,
	MSG_GLOBALMBUTTONDOWN,
	MSG_KEYDOWN,
	MSG_KEYUP,
	MSG_KEYPRESS,
	MSG_CHAR,
	MSG_ITEMACTIVATE,
	MSG_ITEMDEACTIVATE,
	MSG_ITEMDEACTIVATESWITCH,
	MSG_DEACTIVATEITEMS,
	MSG_ITEMPLACE,
	MSG_STATECHANGE,
	MSG_ACTIVATEMENU,
	MSG_MENUSELECT,
	MSG_SLIDERCALCULATE,
	MSG_SLIDERINCREMENT,
	MSG_SLIDERDECREMENT,
	MSG_LDOUBLECLICK,
	MSG_RDOUBLECLICK,
	MSG_MDOUBLECLICK,
	MSG_GLOBALLDOUBLECLICK,
	MSG_GLOBALRDOUBLECLICK,
	MSG_GLOBALMDOUBLECLICK,
	MSG_WHEELUP,
	MSG_WHEELDOWN,
	MSG_EDITSELECT,
	MSG_EDITESCAPE,
	MSG_EDITHASFOCUS,
	MSG_EDITLOSTFOCUS,
	MSG_CREATED,
	MSG_DESTROYED,
	MSG_SHOW,
	MSG_HIDE,
	MSG_FADEOUT,
	MSG_STARTANIMATION,
	MSG_ENDANIMATION,
	MSG_ACTIVATECURSOR,
	MSG_DRAG,
	MSG_REMOVEITEM,
	MSG_EXIT,
	MSG_COMBOEXPAND,
	MSG_SELECT,
	MSG_CONTEXT,
	MSG_ENTER,
	MSG_ESCAPE,
	MSG_CHANGE,
	MSG_CLICKDELAY,
	MSG_RESETCLICKDELAY,
	MSG_DISABLE,
	MSG_ENABLE,

	SET_END_ENUM( MSG_ ),
};

FEX const char* ToString     ( eUIMessage e );
    const char* ToGasString  ( eUIMessage e );
    bool        FromString   ( const char* str, eUIMessage& e );
    bool        FromGasString( const char* str, eUIMessage& e );



// Element Types
enum UI_ELEMENT_TYPE
{
	ELEMENT_WINDOW,
	ELEMENT_TEXT,
};


// Status Bar Edges
enum EDGE_TYPE
{
	EDGE_TOP,
	EDGE_BOTTOM,
	EDGE_LEFT,
	EDGE_RIGHT,
};


// Structures

// Stores the normalized coordinates of a uiwindow rectangle
class UINormalizedRect
{
public:
	UINormalizedRect() { top = 0.0f; left = 0.0f; bottom = 1.0f; right = 1.0f; }
	double top;
	double bottom;
	double left;
	double right;

FEX double GetTop() const		{ return top;		}
FEX double GetBottom() const	{ return bottom;	}
FEX double GetLeft() const		{ return left;		}
FEX double GetRight() const		{ return right;		}
};

// Stores the standard pixel coordinates of a uiwindow rectangle
typedef GRect UIRect;


// Event Actions
struct EVENT_ACTION
{
	UI_ACTION	action;
	gpstring parameter;
};


// UI Event Packaging
struct UI_EVENT
{
	eUIMessage					msg_code;
	std::vector< EVENT_ACTION > event_actions;
};


struct DRAG_BOX
{
	DWORD dwColor;
	UIRect rect;
};


enum eCommonButtons
{
	SLIDER_INCREMENT,
	SLIDER_DECREMENT,
	SLIDER_NONE,		// Specifies that this is NOT a slider related button
	X_EXIT,
};


// Special skrit happy
class UIWindow;
struct UIWindowVec : std::vector< UIWindow * >
{
private:
FEX void		Add  ( UIWindow* wnd )				{  push_back( wnd ); }
FEX int			Size ( void ) const					{  return ( scast <int> ( size() ) );  }
FEX bool		Empty( void ) const					{  return ( empty() );  }
FEX UIWindow*	Get  ( int index ) const			{  gpassert( (index >= 0) && (index < Size()) );  return ( (*this)[ index ] );  }
FEX void		Set  ( int index, UIWindow* wnd )	{  gpassert( (index >= 0) && (index < Size()) );  (*this)[ index ] = wnd;  }
FEX void		Clear( void )						{  clear(); }
};


// Defines / Typedef
typedef std::list< UIWindow * >								ui_window_list;
typedef std::set< UIWindow * >								UIWindowSet;
typedef std::map< gpstring, UIWindow *, istring_less >		UIWindowMap;
typedef std::pair< gpstring, UIWindow * >					UIWindowPair;
typedef std::multimap< int, UIWindow * >					UIWindowDrawMap;
typedef std::pair< int, UIWindow * >						UIWindowDrawPair;
typedef std::vector< gpstring >								StringVec;
typedef std::map< gpstring, unsigned int, istring_less >	UICommonArtMap;
typedef std::vector< DRAG_BOX >								DragBoxColl;
typedef std::map< gpstring, DWORD, istring_less >			TipColorMap;
typedef std::pair< gpstring, DWORD >						TipColorPair;
typedef std::map< int, gpstring >							ResFontMap;
typedef std::pair< int, gpstring >							ResFontPair;
typedef std::map< gpstring, UIWindowVec, istring_less >		UIGroupMap;
typedef std::multimap< int, UIWindow * >					UITabStopMap;

#endif
