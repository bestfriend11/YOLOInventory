//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIMenuManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UIMENUMANAGER_H
#define __UIMENUMANAGER_H

#include "GamePersist.h"
#include "UICamera.h"

// Forward Declarations 
class UIDockbar;


class UIMenuManager : public Singleton <UIMenuManager>
{
public:

	UIMenuManager();

	~UIMenuManager() {};

	//********************* UI Menus ******************************

	void Init();
	void DeInit();

	bool OptionsMenu();
	bool IsOptionsMenuActive() { return m_bOptionsMenuActive; }	

	bool IsLoadSaveActive()		{ return m_bLoadSaveActive; }
	void CloseLoadSave();

	void OptionsDialog();
	void ExitGameDialog();
	void ExitWindowsDialog();
	void ExitGameSave();
	void ExitGameCancel();
	
	bool LoadGameDialog();
	bool SaveGameDialog();
	void ConstructSaveList( bool forRead );

	void ExitToWindows();
	void ExitGame();
	void ResumeGame();
	void ReturnToMenu(); 

	void AlignCompass();

	void CheckLoadSaveEnable();
	void SelectSaveGame();
	void DeleteSavedGame();
	void DeleteSavedGameDialog();
	bool ProcessLoadSave( bool bOverwriteSaveGame = false );

	bool KeyCloseAllDialogs();
	bool CloseAllDialogs();

	void SetExitDialogActive( bool bSet )	{ m_bExitDialogActive = bSet; }
	bool GetExitDialogActive()				{ return m_bExitDialogActive; }

	void ShowCompass( bool bShow );

	void SetGoldTradeDialogActive( bool bSet );
	bool GetGoldTradeDialogActive() { return m_bGoldTradeDialogActive; }

	void SetGoldInvalidDialogActive( bool bSet );
	bool GetGoldInvalidDialogActive() { return m_bGoldInvalidDialogActive; }

	void SetMultiRespawnOption( bool bSet );
	bool GetMultiRespawnOption() { return m_bMultiRespawn; }
	void ShowMultiRespawnMenu();
	void HideMultiRespawnMenu();

	bool QuickSaveDialog();
	bool QuickLoadDialog();
	bool QuickSave();
	bool QuickLoad();
	void AutoSave();

	void SetExitAfterSave( bool bSet )	{ m_bExitAfterSave = bSet; }
	bool GetExitAfterSave()				{ return m_bExitAfterSave; }

	bool GetIsQuickSaving() { return m_bQuickSaving; }
		
	void DrawSave();	
	void UpdateSave();

	void SetScrollText( float set ) { m_scrollText = set; }
	void SetMaxText( int set )	{ m_maxText = set; }
	int	 GetMaxText()			{ return m_maxText; }
	float GetScrollText()		{ return m_scrollText; }
	void ApplyConsoleOptions();

	bool GetGameWasPaused() { return m_bGameWasPaused; }

	void DefeatCallback();	
	void MpVictoryCallback();

	// Multiplayer Load
	void StartMultiplayerLoad();
	void RSUpdateMultiplayerLoad( float progress, DWORD machineId );
	void RCUpdateMultiplayerLoad( float progress, DWORD machineId );

	void StatusBarDock( bool bTop, UIDockbar * pDockbar );
	void HandleMenuResize();

	bool GetStatusDockTop()		{ return m_bStatusDockTop; }

	// Tutorial Tips
FEX	void SActivateTip	( const gpstring & sTip, Goid target );
FEX	void RCActivateTip	( const gpstring & sTip, DWORD machineId );
	void ActivateTip	( const gpstring & sTip, bool bPause, bool bDatabase = false, bool bForce = false );
	bool ActivateTip	( int order, bool bUseMax = false, bool bPause = true );	
	void ShowNextTip	();
	void ShowPreviousTip();
	void CloseTips		( bool bUnpause = false );
	void SetEnableTips	( bool bSet, bool bExplicit = false );
	bool GetEnableTips	()										{ return m_bEnableTips; }
	void ToggleTips		();
	bool ViewTutorialTips();
	void DefeatHelp		();
	bool HasDefeatHelp	();
	void StopTipAnimations();

	// Interface fading
FEX	void SFadeInterface( const char * szInterface, float duration, Goid target );
FEX void RCFadeInterface( const char * szInterface, float duration, DWORD machineId );
	void FadeInterface( const char * szInterface, float duration );
	void FadeOutInterface( UIWindow * pFadeWindow );
	void RemoveFadedInterface( UIWindow * pFadeWindow );
	bool CloseActiveFadeInterface();

FEX void SPlayChapterSound( const char * szSample, Goid target );
FEX void RCPlayChapterSound( const char * szSample, DWORD machineId );
	FUBI_MEMBER_SET( RCPlayChapterSound, +SERVER )

	void PlayChapterSound( const char * szSample );

	// Misc
	void ConsumeCharacterInput( UIWindow * pWindow );

	void SetModalActive( bool bSet );
	bool GetModalActive()				{ return m_bModalActive; }

	void GuiPause( bool bPaused );

	bool GetIsDefeated()				{ return m_bDefeated; }
	void SetIsDefeated( bool bSet )		{ m_bDefeated = bSet; }

	void ActivateMegaMap();
	void DeactivateMegaMap();

	void ShowServerTrafficBacklogIcon( bool bShow = true );	
	bool IsServerTrafficBacklogIconVisible();

	void ShowServerTimeoutIcon( bool bShow = true );
	bool IsServerTimeoutIconVisible();

	bool GetRequestedClientExit()		{ return m_bClientRequestedExit; }

	int  GetTradeShift()			{ return m_TradeShift; }
	void SetTradeShift( int shift ) { m_TradeShift = shift; }  


private:	
	
	bool						m_bIsLoading;
	bool						m_bGameWasPaused;
	bool						m_bOptionsMenuActive;
	bool						m_bLoadSaveActive;
	bool						m_bExitDialogActive;	
	bool						m_bGoldTradeDialogActive;
	bool						m_bGoldInvalidDialogActive;
	bool						m_bMultiRespawn;
	float						m_cameraYTrack;
	bool						m_bQuickSaving;
	bool						m_bSaving;
	bool						m_bAutoSaving;
	bool						m_bPastUpdate;
	bool						m_bLastUpdate;
	gpwstring					m_sSaveFile;
	float						m_scrollText;
	int							m_maxText;
	DWORD						m_dwQuickColor;
	DWORD						m_dwAutoColor;
	SaveInfoColl				m_saveInfoColl;
	bool						m_bExitAfterSave;
	bool						m_bStatusDockTop;
	int							m_currentTip;
	bool						m_bEnableTips;
	UIWindowVec					m_AnimatingTipWindows;
	float						m_TempFadeDuration;
	bool						m_bModalActive;	
	gpstring					m_sFadedInterface;
	bool						m_bDefeated;
	bool						m_bIgnoreAzimuth;
	bool						m_bClientRequestedExit;
	int							m_TradeShift;

	FUBI_SINGLETON_CLASS( UIMenuManager, "This is the UIMenuManager singleton" );
};


#define gUIMenuManager UIMenuManager::GetSingleton()


#endif