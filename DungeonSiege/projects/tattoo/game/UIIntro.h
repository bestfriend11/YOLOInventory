/**************************************************************************

Filename		: UIIntro.h

Class			: UIIntro

Author			: Chad Queen

Description		: Contains the class which handles the initial intro
				  sequence

Creation Date	: 4/3/2001

**************************************************************************/


#pragma once
#ifndef __UIINTRO_H
#define __UIINTRO_H


// Forward Declarations
class InputBinder;
class UIShell;
class UIWindow;


// Include Files
#include "ui.h"

typedef std::map< gpstring, float, istring_less > SceneToBeatMap;
typedef std::pair< gpstring, float > SceneToBeatPair;

class FadeData
{
public:

	FadeData() { m_duration = 0.0f; m_beginAlpha = 0.0f, m_endAlpha = 0.0f; }
	~FadeData() { }

	float m_duration;
	float m_beginAlpha;
	float m_endAlpha;
};

typedef std::map< gpstring, FadeData, istring_less > SceneFadeMap;
typedef std::pair< gpstring, FadeData > SceneFadePair;

// Main Frontend Class
class UIIntro : public UILayer, public Singleton < UIIntro >
{
public:
	
	// Constructor
	UIIntro();

	// Destructor
	~UIIntro();
	
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

	void FadeOutLoadingScreen();

	void FadeInMicrosoft();
	void FadeOutMicrosoft();

	void FadeInGasPowered();
	void FadeOutGasPowered();
	
	void ShowMonologue();
	void HideMonologue();
	
	void ActivateBeat( gpstring sScene );
	void BeatComplete();	
		
private:

	InputBinder *			m_pInputBinder;	
	bool					m_bIsVisible;
	float					m_currentBeat;
	gpstring				m_sCurrentScene;
	DWORD					m_currentSample;
	bool					m_bExiting;
	bool					m_bMonologPlaying;

	SceneToBeatMap			m_beatMap;
	SceneFadeMap			m_fadeMap;

};

#define gUIIntro UIIntro::GetSingleton()


#endif
