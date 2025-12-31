//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_dialogbox.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UI_DIALOGBOX_H
#define __UI_DIALOGBOX_H


class UIDialogBox : public UIWindow
{
public:

	// Constructor
	UIDialogBox( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIDialogBox();

	virtual void CreateCommonCtrl( gpstring sTemplate = gpstring::EMPTY );

	// Destructor
	virtual ~UIDialogBox();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Draw the box
	virtual void Draw();

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

private:

	unsigned int box_bottom_left_corner;	
	unsigned int box_bottom_right_corner;
	unsigned int box_bottom_side;
	unsigned int box_left_side;
	unsigned int box_right_side;
	unsigned int box_top_left_corner;	
	unsigned int box_top_right_corner;	
	unsigned int box_top_side;
	unsigned int box_fill;

};


#endif