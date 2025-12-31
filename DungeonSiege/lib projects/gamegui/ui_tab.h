//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_tab.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef _UI_TAB_H_
#define _UI_TAB_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This radio button class contains all functionality for a simple radio button
class UITab : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UITab(	FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UITab();

	// Destructor
	virtual ~UITab();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// Render the control
	virtual void Draw();

	// See if the radio button is currently selected
	bool GetCheck() { return m_checked; };
	void SetCheck( bool bCheck );
	
	void SetForceCheck( bool bCheck );

	gpstring	GetRadioGroup()						{ return m_sRadioGroup; }
	void		SetRadioGroup( gpstring sGroup ) { m_sRadioGroup = sGroup; }

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

	virtual void CreateCommonCtrl( gpstring sTemplate );

	void OnButtonDown();
	void OnButtonUp();
	void OnButtonRollover();
	void OnButtonRolloff();
	void OnCheck();

	int  GetRow()				{ return m_row; }
	void SetRow( int row )		{ m_row = row;	}

	int  GetColumn()				{ return m_column; }
	void SetColumn( int column )	{ m_column = column; }
	
	DWORD GetDisableColor() { return m_dwDisable; }
	void SetDisableColor( DWORD dwColor ) { m_dwDisable = dwColor; }

private:

	void MoveGroupTextDown();
	void MoveGroupTextUp();

	// Private Member Variables
	bool				m_buttondown;
	bool				m_checked;
	gpstring			m_sRadioGroup;
	bool				m_bStretchCtrl;
	int					m_row;
	int					m_column;
	bool				m_bTextUp;
	DWORD				m_dwDisable;

	// For common textures
	unsigned int m_button_center_tex;
	unsigned int m_button_left_tex;
	unsigned int m_button_right_tex;

};


#endif