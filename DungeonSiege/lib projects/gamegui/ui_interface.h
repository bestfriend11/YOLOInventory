

#pragma once
#ifndef _UI_INTERFACE_H_
#define _UI_INTERFACE_H_

#include "ui_window.h"
#include "ui_button.h"
#include "ui_cursor.h"
#include "ui_checkbox.h"
#include "ui_text.h"
#include "ui_radiobutton.h"
#include "ui_listbox.h"
#include "ui_slider.h"
#include "ui_multistagebutton.h"
#include "ui_popupmenu.h"
#include "ui_gridbox.h"
#include "ui_item.h"


// Enumerated Data types
enum PULLOUT_MODE
{
	X_ALIGN_LEFT,
	X_ALIGN_RIGHT,
	Y_ALIGN_TOP,
	Y_ALIGN_BOTTOM,
	X_ALIGN_LEFT_OUTSIDE,
	X_ALIGN_RIGHT_OUTSIDE,
	Y_ALIGN_TOP_OUTSIDE,
	Y_ALIGN_BOTTOM_OUTSIDE,
};


// Structures
struct GUInterface
{
	GUInterface()	: bModal( false )
					, bPassiveModal( false )
					, bVisible( false )
					, bDeactivate( false )
					, width( 0 )
					, height( 0 )
					, xoffset( 0 )
					, yoffset( 0 )
					, resWidth( 0 )
					, resHeight( 0 )
					, bHasTooltips( false )						
					, bCenteredX( false )
					, bCenteredY( false )
					, bInactive( false )
					, bEnabled( true )
	{}
					

	bool						bModal;	
	bool						bPassiveModal;
	bool						bVisible;
	bool						bDeactivate;
	gpstring					name;
	gpstring					interface_type;
	int							width;			// Optional parameter; for pullout interfaces
	int							height;			// Optional parameter; for pulldown interfaces
	int							xoffset;
	int							yoffset;
	UIWindowMap					ui_windows;
	UIWindowDrawMap				ui_draw_windows;
	UIGroupMap					groupMap;
	UITabStopMap				tabStops;
	int							resWidth;
	int							resHeight;
	bool						bHasTooltips;
	gpstring					sCenter;	
	bool						bCenteredX;
	bool						bCenteredY;
	bool						bInactive;
	bool						bEnabled;
};

typedef std::vector< GUInterface * > GUInterfaceVec;


#endif