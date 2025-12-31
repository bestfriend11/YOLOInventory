/**************************************************************************

Filename		: ui_types.cpp

Creation Date	: 4/9/2001

**************************************************************************/

#include "precomp_gamegui.h"
#include "FuBiTraits.h"
#include "ui_types.h"


FUBI_EXPORT_ENUM( UI_CONTROL_TYPE, UI_TYPE_BEGIN, UI_TYPE_COUNT );
FUBI_EXPORT_ENUM( UI_INPUT_TYPE, TYPE_INPUT_BEGIN, TYPE_INPUT_COUNT );
FUBI_EXPORT_ENUM( UI_ACTION, ACTION_BEGIN, ACTION_COUNT );
FUBI_EXPORT_ENUM( eUIMessage, MSG_BEGIN, MSG_COUNT );


// provide access to derived types via skrit
#define MAKE_QUERY( cls, enm )											\
	class UI##cls;														\
	FEX UI##cls* QueryDerived##cls( UIWindow* base )					\
	{																	\
		if ( (base != NULL) && (base->GetType() == UI_TYPE_##enm) )		\
		{																\
			return ( (UI##cls*)base );									\
		}																\
		return ( NULL );												\
	}

MAKE_QUERY( Button,      BUTTON      );
MAKE_QUERY( Checkbox,    CHECKBOX    );
MAKE_QUERY( ComboBox,    COMBOBOX    );
MAKE_QUERY( EditBox,     EDITBOX     );
MAKE_QUERY( Gridbox,     GRIDBOX     );
MAKE_QUERY( Listbox,     LISTBOX     );
MAKE_QUERY( RadioButton, RADIOBUTTON );
MAKE_QUERY( Text,        TEXT        );
MAKE_QUERY( TextBox,     TEXTBOX     );
// $ add more as needed

#undef MAKE_QUERY


// $ note that these strings do not match the enum, they match the GAS
//   file naming format instead. skrit must be compatible with the data
//   conventions.

static const char* s_UI_CONTROL_TYPE_Strings[] =
{
	"ui_type_window",
	"ui_type_button",
	"ui_type_checkbox",
	"ui_type_slider",
	"ui_type_listbox",
	"ui_type_radio_button",
	"ui_type_button_multistage",
	"ui_type_text",
	"ui_type_cursor",
	"ui_type_dockbar",
	"ui_type_gridbox",
	"ui_type_popupmenu",
	"ui_type_item",
	"ui_type_itemslot",
	"ui_type_infoslot",
	"ui_type_status_bar",
	"ui_type_edit_box",
	"ui_type_combo_box",
	"ui_type_listener",
	"ui_type_listreport",
	"ui_type_chat_box",
	"ui_type_text_box",
	"ui_type_dialog_box",
	"ui_type_tab",
	"ui_type_line",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_UI_CONTROL_TYPE_Strings ) == UI_TYPE_COUNT );

static stringtool::EnumStringConverter s_UI_CONTROL_TYPE_Converter( s_UI_CONTROL_TYPE_Strings, UI_TYPE_BEGIN, UI_TYPE_END );

const char* ToString( UI_CONTROL_TYPE e )
	{  return ( s_UI_CONTROL_TYPE_Converter.ToString( e ) );  }
const char* ToGasString( UI_CONTROL_TYPE e )
	{  return ( ToString( e ) + 8 );  }
bool FromString( const char* str, UI_CONTROL_TYPE& e )
	{  return ( s_UI_CONTROL_TYPE_Converter.FromString( str, rcast <int&> ( e ) ) );  }
bool FromGasString( const char* str, UI_CONTROL_TYPE& e )
	{  return ( FromString( gpstring( "ui_type_" ) + str, e ) );  }


static const char* s_UI_INPUT_TYPE_Strings[] =
{
	"type_input_none",
	"type_input_all",
	"type_input_mouse",
	"type_input_key",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_UI_INPUT_TYPE_Strings ) == TYPE_INPUT_COUNT );

static stringtool::EnumStringConverter s_UI_INPUT_TYPE_Converter( s_UI_INPUT_TYPE_Strings, TYPE_INPUT_BEGIN, TYPE_INPUT_END );

const char* ToString( UI_INPUT_TYPE e )
	{  return ( s_UI_INPUT_TYPE_Converter.ToString( e ) );  }
bool FromString( const char* str, UI_INPUT_TYPE& e )
	{  return ( s_UI_INPUT_TYPE_Converter.FromString( str, rcast <int&> ( e ) ) );  }


static const char* s_UI_ACTION_Strings[] =
{
	"action_setrect",
	"action_setnormalizedrect",
	"action_setvisible",
	"action_showgroup",
	"action_loadtexture",
	"action_setuvcoords",
	"action_sendmessage",
	"action_parentmessage",
	"action_setalpha",
	"action_addelement",
	"action_removeelement",
	"action_notify",
	"action_sethotspot",
	"action_settext",
	"action_setgroup",
	"action_setelementheight",
	"action_setnumstates",
	"action_setstate",
	"action_showmenu",
	"action_activatemenu",
	"action_activateitem",
	"action_setcolor",
	"action_playsound",
	"action_keylisten",
	"action_keypress",
	"action_rectanimation",
	"action_alphaanimation",
	"action_flashanimation",
	"action_clockanimation",
	"action_setstatus",
	"action_loadanimatedtexture",
	"action_consumeinput",
	"action_editselect",
	"action_shift_x",
	"action_shift_y",
	"action_setrolloverhelp",
	"action_vertexcolor",
	"action_command",
	"action_call",
	"action_none",	
};

COMPILER_ASSERT( ELEMENT_COUNT( s_UI_ACTION_Strings ) == ACTION_COUNT );

static stringtool::EnumStringConverter s_UI_ACTION_Converter( s_UI_ACTION_Strings, ACTION_BEGIN, ACTION_END, ACTION_NONE );

const char* ToString( UI_ACTION e )
	{  return ( s_UI_ACTION_Converter.ToString( e ) );  }
const char* ToGasString( UI_ACTION e )
	{  return ( ToString( e ) + 7 );  }
bool FromString( const char* str, UI_ACTION& e )
	{  return ( s_UI_ACTION_Converter.FromString( str, rcast <int&> ( e ) ) );  }
bool FromGasString( const char* str, UI_ACTION& e )
	{  return ( FromString( gpstring( "action_" ) + str, e ) );  }


static const char* s_eUIMessage_Strings[] =
{
	"msg_none",
	"msg_oninvisible",
	"msg_onvisible",
	"msg_onlbuttondown",
	"msg_onlbuttonup",
	"msg_onrbuttondown",
	"msg_onrbuttonup",
	"msg_onmbuttondown",
	"msg_onmbuttonup",
	"msg_onbuttonpress",
	"msg_onrbuttonpress",
	"msg_onrollover",
	"msg_onrolloff",
	"msg_oncheck",
	"msg_onuncheck",
	"msg_onunchecked",
	"msg_onforcecheck",
	"msg_onforceuncheck",
	"msg_onlbuttondownchecked",
	"msg_onlbuttonupchecked",
	"msg_onrolloverchecked",
	"msg_onrolloffchecked",
	"msg_ongloballbuttonup",
	"msg_ongloballbuttonupoff",
	"msg_ongloballbuttondown",
	"msg_onglobalrbuttonup",
	"msg_onglobalrbuttonupoff",
	"msg_onglobalrbuttondown",
	"msg_onglobalmbuttonup",
	"msg_onglobalmbuttondown",
	"msg_onkeydown",
	"msg_onkeyup",
	"msg_onkeypress",
	"msg_char",
	"msg_onitemactivate",
	"msg_onitemdeactivate",
	"msg_deactivate_switch",
	"msg_ondeactivateitems",
	"msg_onitemplace",
	"msg_onstatechange",
	"msg_onactivatemenu",
	"msg_onmenuselect",
	"msg_calculateslidersize",
	"msg_increment_slider",
	"msg_decrement_slider",
	"msg_onldoubleclick",
	"msg_onrdoubleclick",
	"msg_onmdoubleclick",
	"msg_globalldoubleclick",
	"msg_globalrdoubleclick",
	"msg_globalmdoubleclick",
	"msg_onwheelup",
	"msg_onwheeldown",
	"msg_oneditselect",
	"msg_oneditescape",
	"msg_onedithasfocus",
	"msg_oneditlostfocus",
	"msg_oncreated",
	"msg_ondestroyed",
	"msg_onshow",
	"msg_onhidden",
	"msg_onfadeout",
	"msg_onstartanim",
	"msg_onanimcomplete",
	"msg_activatecursor",
	"msg_ondrag",
	"msg_onremoveitem",
	"msg_onexit",
	"msg_oncomboexpand",
	"msg_onselect",
	"msg_oncontext",
	"msg_onenter",
	"msg_onescape",
	"msg_onchange",
	"msg_onclickdelay",
	"msg_onresetclickdelay",
	"msg_disable",
	"msg_enable",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_eUIMessage_Strings ) == MSG_COUNT );

static stringtool::EnumStringConverter s_eUIMessage_Converter( s_eUIMessage_Strings, MSG_BEGIN, MSG_END, MSG_NONE );

const char* ToString( eUIMessage e )
	{  return ( s_eUIMessage_Converter.ToString( e ) );  }
const char* ToGasString( eUIMessage e )
	{  return ( ToString( e ) + 4 );  }
bool FromString( const char* str, eUIMessage& e )
	{  return ( s_eUIMessage_Converter.FromString( str, rcast <int&> ( e ) ) );  }
bool FromGasString( const char* str, eUIMessage& e )
	{  return ( FromString( gpstring( "msg_" ) + str, e ) );  }
