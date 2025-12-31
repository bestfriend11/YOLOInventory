/**************************************************************************

Filename		: UIOutro.h

Class			: UIOutro

Author			: Chad Queen

Description		: Contains the class which handles the initial outro
				  sequence

Creation Date	: 7/7/2001

**************************************************************************/


#pragma once
#ifndef __UIOUTRO_H
#define __UIOUTRO_H


// Forward Declarations
class InputBinder;
class UIShell;
class UIWindow;


// Include Files
#include "ui.h"


// Main Frontend Class
class UIOutro : public UILayer, public Singleton < UIOutro >
{
public:
	
	// Constructor
	UIOutro();

	// Destructor
	~UIOutro();
	
	//********************* Updating/Rendering *********************
	
	void Update( double seconds_elapsed );

	void Draw( double seconds_elapsed );	

	bool IsVisible() { return( m_bIsVisible ); }

	void SetIsVisible( bool flag );

	void SetAsActive( bool flag );

	
	//********************* UI States/Exiting **********************
	
	void DeactivateInterfaces();

	//********************* End UI States/Exiting ******************


	//********************* Input Handling Routines ****************
	
	InputBinder & GetInputBinder();
	void PublishInterfaceToInputBinder();
	void BindInputToPublishedInterface();

	bool OnEscape();

	virtual void OnAppActivate( bool /*activate*/ )  {  }

	//********************* End Input Handling Routines ************


	//********************* GameGUI Callbacks **********************
	
	void GameGUICallback( gpstring const & message, UIWindow & ui_window );

	//********************* End GameGUI Callbacks ******************

	void StopSampleCB( UINT32 param );

	void FadeToBlack();
	void ShowMonologue();
	void HideMonologue();
	void RollCredits();
	void CreditsFinish();
	void FadeInGame();

	bool SkipMovie();
	bool RestartMovie();
	bool PauseMovie();

	bool IsMonologuePlaying() { return m_bMonologPlaying; }
	bool AreCreditsRolling() { return m_bCreditsRolling; }

	void SetOutroGameSize();
	void SetLastGameSize();
		
private:

	InputBinder *			m_pInputBinder;	
	bool					m_bIsVisible;		
	DWORD					m_currentSample;
	float					m_initialPause;
	float					m_faderPause;
	float					m_faderInPause;
	bool					m_bMonologPlaying;	
	bool					m_bFading;
	bool					m_bCreditsRolling;
	SiegePos				m_Pos;
	DWORD					m_lastResWidth;
	DWORD					m_lastResHeight;	
	gpstring				m_sCreditsSample;
	
};

#define gUIOutro UIOutro::GetSingleton()


#endif
