/**************************************************************************

Filename		: ui_multistagebutton.h

Description		: Control for a multi-stage button

Creation Date	: 10/5/99

**************************************************************************/


#pragma once
#ifndef _UI_MULTISTAGEBUTTON_H_
#define _UI_MULTISTAGEBUTTON_H_


// Forward Declarations
class FuelHandle;
class Rapi;
class UITextureManager;
class Messenger;
class UIAnimation;
class UIMessage;


// This button class contains all functionality for a simple button
class UIMultiStageButton : public UIWindow
{
public:
	// Public Methods
	
	// Constructor
	UIMultiStageButton( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent );
	UIMultiStageButton();

	// Destructor
	virtual ~UIMultiStageButton();

	// Process actions for the button
	virtual bool ProcessAction	( UI_ACTION action, gpstring parameter	);
	virtual bool ProcessMessage	( UIMessage & msg						);

	// State information
	void	SetState( int state )	{ m_state = state;	};
	int		GetState()				{ return m_state;	};	
	void	SetNumStates( int num_states )	{ m_num_states = num_states;	};
	int		GetNumStates()					{ return m_num_states;			};

#if !GP_RETAIL
	virtual void Save( FuelHandle hSave, bool bChild );
#endif

private:

	// Private Member Variables
	bool	m_buttondown;
	int		m_state;
	int		m_num_states;

};


#endif