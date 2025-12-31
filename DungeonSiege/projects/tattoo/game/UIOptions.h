/**************************************************************************

	UIOptions
	
	Date		: Oct 23, 2000
	
	Author		: Adam Swensen

	Purpose		: Handles the options interfaces.

**************************************************************************/


#pragma once
#ifndef _UIOPTIONS_H_
#define _UIOPTIONS_H_

#include "InputBinder.h"


#define KEY_BINDINGS_ROWS_VISIBLE 10
#define KEY_HOLD_SET_TIME 1.5f

#define OPTION_MIN_GAME_SPEED 0.2f
#define OPTION_MAX_GAME_SPEED 1.2f

#define OPTION_MIN_GAMMA_VALUE 0.5f
#define OPTION_MAX_GAMMA_VALUE 1.5f

#define OPTION_MIN_TEXT_SCROLL 1
#define OPTION_MAX_TEXT_SCROLL 10

#define OPTION_MIN_CHAT_LINES 1
#define OPTION_MAX_CHAT_LINES 8


class UIOptions : public Singleton< UIOptions >
{
public:

	UIOptions();
	~UIOptions();

	void Activate();
	void Deactivate();
	bool IsActive()								{  return m_bIsActive;  }

	void Update( double secondsElapsed );

	void SetDefaults();

	void SetDefaultFiltering( const gpstring & sFiltering ) { m_sDefaultFiltering = sFiltering; }
	void SetDefaultShadows();

private:

	void HideInterfaces();

	void GetVideoOptions();
	void SetVideoOptions();

	void GetAudioOptions();
	void SetAudioOptions();

	void GetGameOptions();
	void SetGameOptions();

	void GetInputOptions();
	void SetInputOptions();

	void GameGUICallback( gpstring const & message, UIWindow & ui_window );

	void CreateBindingsList( int startAt = 0 );
	gpwstring CreateKeyScreenString( KeyInput & key );
	gpwstring CreateMouseScreenString( MouseInput & mouse );

	bool GetSelectedBindingEntry( gpstring const & textName );

	bool InputKeyCallback( const KeyInput & key );
	bool InputMouseCallback( const Input & input );

	void RestoreDefaultVideoOptions();
	void RestoreDefaultAudioOptions();
	void RestoreDefaultInputOptions();
	void RestoreDefaultGameOptions();
	void RestoreDefaultInputBindings();		

private:

	bool	m_bIsActive;

	// store values which are dynamically changed, so they can be restored if cancel is pressed
	bool	m_bGotCancelVideoOptions;
	bool	m_bGotCancelAudioOptions;
	bool	m_bGotCancelGameOptions;

	int		CancelScreenRes;
	float	CancelGammaValue;

	DWORD	CancelSFXVolume;
	DWORD	CancelMusicVolume;
	DWORD	CancelAmbientVolume;
	DWORD	CancelVoiceVolume;
	DWORD	CancelMasterVolume;

	bool	CancelShowFPS;
	bool	CancelShowToolTips;
	float	CancelTextScroll;
	int		CancelMaxText;
	bool	CancelTutorialTips;
	eDifficulty CancelDifficulty;
	bool	CancelRedBlood;
	bool	CancelBloodEnabled;
	bool	CancelDismemberment;
	bool	CancelShareExperience;
	bool	CancelPriorityBoost;
	bool	CancelLockCameraX;
	bool	CancelLockCameraY;
	int		CancelZMMessage;
	int		CancelDropInventory;

	bool	m_bTempMuteSound;
	bool	m_bTempEnableSound;	

	// screen res
	typedef std::vector< gpstring > ResColl;
	ResColl	m_AllowedRes;

	// input bindings
	InputBinder *m_pInputBinder;
	InputBinder::BindingGroupMap m_bindings;
	Input	*m_pSelectedBindingInput;
	Input	*m_pSisterBindingInput;

	UIText	*m_pSelectedBindingWindow;
	int		m_SelectedBindingColumn;

	int		m_CurrentBindingRowAt;

	bool	m_bControlDown;
	bool	m_bShiftDown;
	bool	m_bAltDown;

	double	m_KeySecondsDown;

	bool	m_bGameBinderWasActive;
	bool	m_bGameWasPaused;

	bool	m_bCanShowDismembermentOption;
	bool	m_bCanShowRedBloodOption;

	gpstring m_sDefaultFiltering;

	gpstring m_sLastTab;

	gpstring m_sSelectedBindingName;

	UIText	**m_pInputDescriptions;
	UIText	**m_pInputPrimary;
	UIText	**m_pInputSecondary;
	
	FUBI_SINGLETON_CLASS( UIOptions, "The Game Options UI." );

	SET_NO_COPYING( UIOptions );
};

#define gUIOptions UIOptions::GetSingleton()


#endif
