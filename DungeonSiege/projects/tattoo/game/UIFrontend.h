/**************************************************************************

Filename		: UIFrontend.h

Class			: UIFrontend

Author			: Chad Queen

Description		: Contains the class which handles the initial menu
				  interface.

Creation Date	: 10/12/99

**************************************************************************/


#pragma once
#ifndef _UIFRONTEND_H_
#define _UIFRONTEND_H_


// Forward Declarations
class InputBinder;
class UIShell;
class UIWindow;
class UICharacterSelect;



// Include Files
#include "ui.h"
#include "CameraPosition.h"
#include "nema_types.h"
#include "GamePersist.h"
#include "UIPartyManager.h"


struct NotifyToLight
{
	nema::LightSource nls;
	bool bRamp;
	float startIntensity;
	float endIntensity;
	float rampDuration;
	float secondsElapsed;
	bool bTriggered;
};

typedef std::map< gpstring, NotifyToLight, istring_less > NotifyToLightMap;
typedef std::pair< gpstring, NotifyToLight > NotifyToLightPair;

class GuiLight
{
public:

	GuiLight( nema::LightSource nls ) { m_nls = nls; }

	void ReceiveNotification( gpstring sNotify );
	void Update( float secondsElapsed );

	nema::LightSource m_nls;
	NotifyToLightMap  m_notifyLights;
};

typedef std::vector< GuiLight > GuiLightColl;


struct NotifyAmbience
{	
	DWORD startAmbience;
	DWORD finalAmbience;
	float rampDuration;
	bool  bRamp;
	float secondsElapsed;
	bool bTriggered;
};

typedef std::map< gpstring, NotifyAmbience, istring_less > NotifyAmbienceMap;
typedef std::pair< gpstring, NotifyAmbience > NotifyAmbiencePair;

class GuiLightScene
{
public:

	void ReceiveNotification( gpstring sNotify );	
	void Update( float secondsElapsed );

	DWORD				m_ambience;
	NotifyAmbienceMap	m_notifyAmbience;
	GuiLightColl		m_guiLights;

};

typedef std::map< gpstring, GuiLightScene, istring_less > GuiToLightMap;
typedef std::pair< gpstring, GuiLightScene > GuiToLightPair;


// Main Frontend Class
class UIFrontend : public UILayer, public Singleton < UIFrontend >
{
public:
	
	// Constructor
	UIFrontend();

	// Destructor
	~UIFrontend();
	
	//********************* Updating/Rendering *********************
	
	void Update( double seconds_elapsed );

	void Draw( double seconds_elapsed );	
	void DrawEffects( Go * pObject );

	bool IsVisible() { return( m_bIsVisible ); }

	void SetIsVisible( bool flag );

	void MultiplayerPrep();

	//********************* End Updating/Rendering *****************


	//********************* UI States/Exiting **********************
	
	void DeactivateInterfaces();
	void HideInterfaces();

	//********************* End UI States/Exiting ******************


	//********************* Game Specific Settings *****************

	// Insert the Available Map Strings
	void SetMapStrings();

	gpwstring GetEnteredName();	
	bool IsNameValid( gpwstring & sName );

	void LoadGameSelect();
	void ConstructSaveList();

	bool ShowCDCheckDialog();
	void HideCDCheckDialog();
	void CDCheckGUICallback( gpstring const & message, UIWindow & ui_window );
	
	//********************* End Game Specific Settings *************


	//********************* Input Handling Routines ****************
	
	InputBinder & GetInputBinder();
	void PublishInterfaceToInputBinder();
	void BindInputToPublishedInterface();

	bool OnEscape();
	bool RestartMovie();
	bool PauseMovie();
	bool SkipMovie();	

	virtual void OnAppActivate( bool /*activate*/ )  {  }

	//********************* End Input Handling Routines ************



	//********************* GameGUI Callbacks **********************
	
	void GameGUICallback( gpstring const & message, UIWindow & ui_window );

	//********************* End GameGUI Callbacks ******************
	
	void InitGuiLights();
	void InitGui( bool bFromMulti = false );
	void InitTexSwapMap();
	
	void CalculateGuiLights( Go * pGo );
	void SetTemporaryView();

	void GuiFlyin();
	void GuiSinglePlayer();
	void GuiStartGame();
	void GuiBackToMain();
	void GuiBackToSinglePlayer();
	void GuiBackToHeroSelect();
	void GuiLoadMap();
	void GuiLoadGame();
	void GuiBackToSPFromLG();
	void GuiMultiplayer();
	void GuiBackToMainFromMP();
	void GuiProviderFromMP();
	void GuiLoadMapToDifficulty();
	void GuiDifficultyToLoadMap();
	void GuiHeroSelectToDifficulty();
	void GuiDifficultyToHeroSelect();	
	void GuiLogoIn();
	void GuiLogoOut();
	
	void GuiTextureSwap( gpstring sWindow, int windowIndex, gpstring sEvent );

	void MenuBarsFlyinCompletionCb( unsigned int param );
	void MenuBarsSinglePlayerCompletionCb( unsigned int param );
	void MenuBarsStartNewGameCompletionCb( unsigned int param );
	void LoadMapCompletionCb( unsigned int param );
	void LoadMapPreviousCompletionCb( unsigned int param );
	void LoadGameCompletionCb( unsigned int param );
	void MenuBarsMPToMainCompletionCb( unsigned int param );
	void MenuBarsMultiPlayerCompletionCb( unsigned int param );
	void MapLoadToDifficultyCompletionCb( unsigned int param );
	void DifficultyToMapLoadCompletionCb( unsigned int param );
	void DifficultyToHeroSelectCompletionCb( unsigned int param );
	void LogoOutCompletionCb( unsigned int param );
	void LogoInCompletionCb( unsigned int param );
	
	const vector_3 & GetCharacterPos()		{ return m_vCharacterPos; }
	const vector_3 & GetCharacterFacePos()	{ return m_vCharacterFacePos; }

	const CameraEulerPosition & GetCameraPosition() { return m_Camera; }	

	void UnloadData();

	void UpdateGuiLights( float secondsElapsed );

	void		SetDefaultName( gpwstring sName )	{ m_sDefaultName = sName; }
	gpwstring	GetDefaultName()					{ return m_sDefaultName; }

	void SelectNameDialog();

	gpstring			GetSelectedMapName()					{ return m_sMapName; }
	void				SetSelectedMapName( gpstring sMapName ) { m_sMapName = sMapName; }

	void				CheckLoadMap();

	void				BeginLoadMap();

	// The purpose of these functions is to get the screen to draw a frame before starting
	// the map load in order to give the user instant feeback that the map is preparing to load
	bool				GetBeginLoadMap()				{ return m_bBeginLoad; }
	void				SetBeginLoadMap( bool bSet )	{ m_bBeginLoad = bSet; }

	void PlayFrontendSound( gpstring const & sEvent );

	void ShowDialog( gpwstring const & message );

	void StartFrontendMusic();
	void StopFrontendMusic();

private:

	CameraEulerPosition		m_Camera;

	bool					m_bIsVisible;
	bool					m_bGuiInitialized;

	InputBinder *			m_pInputBinder;		

	UICharacterSelect *		m_pCharacterSelect;

	MapInfoColl				m_mapInfoColl;

	SaveInfoColl			m_saveInfoColl;

	Goid					m_guiBackButton;

	Goid					m_guiBackdrop;

	Goid					m_guiMainMenu;

	Goid					m_guiMenuBars;	

	Goid					m_guiHeroMenu;

	Goid					m_guiLeftSide;

	Goid					m_guiRightSide;	

	Goid					m_guiLoadMap;

	Goid					m_guiLogo;

	vector_3				m_vCharacterPos;

	vector_3				m_vCharacterFacePos;

	bool					m_bProviderInit;

	float					m_orthoMeterPerPixel;

	GuiToLightMap			m_guiLightMap;

	D3DRECT					m_char_rect;

	DWORD					m_dwQuickColor;

	DWORD					m_dwAutoColor;

	typedef std::vector< unsigned int > TextureColl;

	TextureColl				m_preloadTextures;

	typedef std::map< int, gpstring > GuiTexIndexMap;
	typedef std::pair< int, gpstring > GuiTexIndexPair;

	typedef std::map< gpstring, GuiTexIndexMap, istring_less > GuiTexEventMap;
	typedef std::pair< gpstring, GuiTexIndexMap > GuiTexEventPair;

	typedef std::map< gpstring, GuiTexEventMap, istring_less > GuiTexSwapMap;
	typedef std::pair< gpstring, GuiTexEventMap > GuiTexSwapPair;

	GuiTexSwapMap			m_guiTexSwapMap;

	gpwstring				m_sDefaultName;

	gpstring				m_sMapName;
	bool					m_bBeginLoad;

	bool					m_bDrawCharSelect;

	float					m_LogoTimeout;
	float					m_LogoCurrentTime;
	bool					m_bLogoOut;

	eWorldState				m_NextWorldState;

	DWORD					m_FrontendMusic;
	DWORD					m_logoSample;

	bool					m_bVersionSet;
};

#define gUIFrontend UIFrontend::GetSingleton()


#endif
