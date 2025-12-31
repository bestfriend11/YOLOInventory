//////////////////////////////////////////////////////////////////////////////
//
// File     :  TattooGame.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the primary game loop.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TATTOOGAME_H
#define __TATTOOGAME_H

//////////////////////////////////////////////////////////////////////////////

#include "RapiAppModule.h"
#include "FuBiDefs.h"
#include "GoDefs.h"
#include "GpColl.h"
#include "StdHelp.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class FuelHandle;
class GameState;
class GameMovie;
class GpConsole;
class gpprofiler;
class Services;
class UI;
class World;

struct CameraPosition;
struct SaveInfoColl;
struct CharacterInfoColl;

// $$$ MARKED FOR DEATH
#if GP_ENABLE_STATS
class gpc_gpstats;
#endif // GP_ENABLE_STATS
#if !GP_RETAIL
class gpc_mman;
class gpc_file;
class gpc_log;
class gpc_reload;
class gpc_streamer;
class gpc_fps;
#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class TattooGame declaration

class TattooGame : public RapiAppModule, public Singleton <TattooGame>
{
public:
	SET_INHERITED( TattooGame, RapiAppModule );

	static ThisType& GetSingleton( void )  {  return ( Singleton <ThisType>::GetSingleton() );  }

// Types and constants.

	// 'g' options are game-specific options (to avoid colliding with AppModule's)
	enum eGOptions
	{
		GOPTION_TRACE_MEMORY           = 1 << 0,		// trace memory usage
		GOPTION_TRACE_PROFILER         = 1 << 1,		// trace profiler output
		GOPTION_TRACE_CPUSTATS         = 1 << 2,		// trace general cpu stats output
		GOPTION_TRACE_CPUSTATS_TABLE   = 1 << 3,		// add the table for stats
		GOPTION_TRACE_CPUSTATS_GRAPH   = 1 << 4,		// add the pie graph
		GOPTION_TRACE_CPUSTATS_HISTORY = 1 << 5,		// add the historical graph
		GOPTION_TRACE_STATS            = 1 << 6,		// trace stats output
		GOPTION_SCREENSHOT_STAMP       = 1 << 7,		// stamp screen shots
		GOPTION_MARKETING_DEMO		   = 1 << 8,		// game is in marketing demo mode
		GOPTION_CLEAN_RENDER		   = 1 << 9,		// do a "clean" render - no cursor, debug consoles, selection rings, etc.
		// ...

		GOPTION_NONE = 0,							// disable all options
		GOPTION_ALL  = 0xFFFFFFFF,					// enable all options
	};

	// don't hide parent's functions
	using Inherited::SetOptions;
	using Inherited::ClearOptions;
	using Inherited::ToggleOptions;
	using Inherited::TestOptions;

// Setup.

	// ctor/dtor
	TattooGame( void );
	virtual ~TattooGame( void );

	// persistence
	virtual bool Xfer( FuBi::PersistContext& persist );

	// configuration
	void SetOptions   ( eGOptions options, bool set = true )	{  m_GOptions = (eGOptions)(set ? (m_GOptions | options) : (m_GOptions & ~options));  OnOptionsChanged();  }
	void ClearOptions ( eGOptions options )						{  SetOptions( options, false );  OnOptionsChanged();  }
	void ToggleOptions( eGOptions options )						{  m_GOptions = (eGOptions)(m_GOptions ^ options);  OnOptionsChanged();  }
	bool TestOptions  ( eGOptions options ) const				{  return ( (m_GOptions & options) != 0 );  }

	// local event
	void OnOptionsChanged( void );

	// skip the next x frames of rendering (a "good" hack)
    void SetRenderSkip( int skipCount )							{  maximize( m_RenderSkipCount, skipCount );  }

	// skip time by fast-forwarding time without rendering
	bool SkipTime      ( float seconds );
	bool IsSkippingTime( void ) const							{  return ( m_IsSkippingTime );  }

	// set sim frame rate
	void  SetSimFrameRate( float sps )							{  m_SimFrameRate = sps;  }
	float GetSimFrameRate( void ) const							{  return ( m_SimFrameRate );  }

	// render current frame again
	void RenderFrame( bool includeGui );

	// quit the game right now
FEX	void ExitGame( void );

	bool CheckCdInDrive( void );

// Commands.

	// $$$ move to Server object
	bool LoadCameraSettings( const char* camBlock );

	// screen shot using proper default naming
	virtual bool ScreenShot( bool frontBuffer, const wchar_t* fileName = NULL, gpwstring* outFileName = NULL );

	// special screen shots
	bool ScreenShotDefaultNoGui        ( void );
	bool CopyScreenShotToClipboardNoGui( void );

// Persistence.

	// save implementation
	bool SaveGame          ( const wchar_t* saveName );			// save specific game, automatically overwriting if it's there and not read-only
#if !GP_RETAIL
FEX bool SaveGame          ( const char* saveName )				{  return ( SaveGame( ::ToUnicode( saveName ) ) );  }
#endif // !GP_RETAIL
	bool QuickSaveGame     ( void );							// save in quicksave slot (user initiated)
FEX	bool AutoSaveGame      ( void );							// queue save in autosave slot (timer or event initiated)
FEX	bool AutoSaveGameNow   ( void );							// save in autosave slot (ui initiated)
	bool SaveParty         ( const wchar_t* saveName,			// save party out, automatically overwriting if it's there and not read-only
						     bool outputStatus = false );
	bool SavePartyMember   ( int slot );						// save a party member out, automatically overwriting if it's there and not read-only
FEX	bool AutoSaveParty     ( void );							// save the party using the current player's name
FEX	bool CheckAutoSaveParty( bool ignoreMinTime );				// check to see if we should autosave the screen player's party and do it if true (optionally ignore the minimum-time requirement)
FEX	bool CheckAutoSaveParty( void )								{  return ( CheckAutoSaveParty( false ) );  }

	// load implementation
	bool LoadGame       ( const wchar_t* loadName );			// load specific game
	bool LoadNewestGame ( void );								// load the newest save game we can find based on file modification date
	bool LoadRecentGame ( void );								// load the most recently saved game
	bool QuickLoadGame  ( void );								// load the quicksave game
	bool AutoLoadGame   ( void );								// load the autosave game
	bool LoadGamePartial( const wchar_t* loadName );			// salvage what we can from a previous save

	// querying save game existence
	bool HasRecentSaveGame( void );
	bool HasQuickSaveGame ( void ) const;
	bool HasAutoSaveGame  ( void ) const;

	// deleting a save game
	bool DeleteSaveGame( const wchar_t* saveName );

	// character importing
	bool ImportCharacterFromSave    ( const wchar_t* saveName, int characterIndex );
	bool ImportAllCharactersFromSave( const wchar_t* saveName )	{  return ( ImportCharacterFromSave( saveName, -1 ) );  }

	// teleporting
FEX	void STeleportPlayer ( PlayerId playerId, const char* where, bool teleportSelectedItems = true );
	void RSTeleportPlayer( PlayerId playerId, const SiegePos& pos, const CameraPosition& cpos, bool teleportSelectedItems = true );
	void TeleportPlayer	 ( PlayerId playerId, const SiegePos& pos, bool teleportSelectedItems = true );
	void TeleportPlayer  ( PlayerId playerId, const SiegePos& pos, const CameraPosition& cpos, bool teleportSelectedItems = true );
FEX	void RSPrepareTeleportPlayer( PlayerId playerId, const char* where );
FEX void RCPrepareTeleportPlayer( PlayerId playerId, const char* where );
FEX void PrepareTeleportPlayer  ( PlayerId playerId, const char* where );

	// force reload from where we are
#	if !GP_RETAIL
FEX bool ReloadWorld    ( bool fullRestart = false );
FEX bool ReloadObjects  ( void )								{  return ( PrivateReloadObjects( RELOAD_GOS ) );  }
FEX bool ReloadContentDb( void )								{  return ( PrivateReloadObjects( RELOAD_CONTENTDB ) );  }
#	endif // !GP_RETAIL

	// last save
	gpwstring GetLastSaveGame ( void );
	gpwstring GetLastPartySave( void );
	gpwstring TakeLastSaveGame( void )							{  gpwstring temp;  temp.swap( m_LastSaveGame );  return ( temp );  }

	// save query
	int  GetCampaignSaveGameInfos   ( SaveInfoColl& coll, bool forRead, bool getThumbs = true );
	bool HasCampaignSaveGameInfos   ( bool forRead );
	bool HasCampaignSaveGame        ( const wchar_t* saveName );
	int  GetMultiplayerSaveGameInfos( SaveInfoColl& coll, bool forRead, bool getThumbs = true );
	bool HasMultiplayerSaveGameInfos( bool forRead );
	bool HasMultiplayerSaveGame     ( const wchar_t* saveName );
	int  GetPartySaveInfos          ( SaveInfoColl& coll, bool forRead, bool includeSaveGames = false, bool getPortraits = true );
	bool HasPartySaveInfos          ( bool forRead, bool includeSaveGames = false );
	bool HasPartySave               ( const wchar_t* saveName, bool includeSaveGames = false );
	int  GetMpCharacterSaveInfos    ( CharacterInfoColl& coll, bool getPortraits = true );

	// character import query
	int GetSavedCharacterInfos( CharacterInfoColl& coll, const gpwstring& saveName, bool getPortraits = true );

	// save/party name checking
	static bool IsValidSaveScreenName( const wchar_t* saveName, gpwstring* errorMsg );

	// key bindings
	void       LoadKeyConfig   ( const wchar_t* configName = NULL );
	void       SaveKeyConfig   ( const wchar_t* configName = NULL, bool autoSave = true );
	void       DeleteKeyConfig ( const wchar_t* configName = NULL, bool autoSave = true );
	FuelHandle GetKeyConfigFuel( const wchar_t* configName, bool forSaving = false );

	// user preferences
	void       LoadPrefsConfig   ( void );
	void       SavePrefsConfig   ( bool autoSave = true );
	void       DeletePrefsConfig ( bool autoSave = true );
	FuelHandle GetPrefsConfigFuel( bool forSaving = false );

	void SPause( bool pause );
	void RSUserPause( bool pause );
FEX	void RSUserPause( bool pause, PlayerId requestingPlayer );

	////////////////////////////////////////
	//	MP related

	int  GetMPSleep( void ) const								{  return ( m_MPSleep );  }
FEX	void SetMPSleep( int sleep )								{  m_MPSleep = sleep;  }
FEX	void SetMPSendFrequency( int Hz );

#	if !GP_RETAIL	
	// statistic and profiling
	void AddPendingOrderItem( const gpstring& pendingOrderIN )	{	m_PendingOrderBuffer.push_back( pendingOrderIN ); }
#	endif

private:

	bool PrivateSaveGame( const wchar_t* saveName, bool quick, bool aut );
	bool PrivateLoadGame( const wchar_t* loadName, bool quick, bool aut );

	enum eReloadType
	{
		RELOAD_GOS,
		RELOAD_CONTENTDB,
	};

	// pipeline
	virtual bool OnInitCore    ( void );
	virtual bool OnInitServices( void );
	virtual bool OnShutdown    ( void );

	// execution
	virtual void OnAppActivate        ( bool activate );
	virtual void OnUserPause          ( bool pause );
	virtual bool OnRequestAppClose    ( void );
	virtual bool OnScreenChange       ( void );
	virtual bool OnCheckFocus         ( void );
	virtual bool OnPreTranslateMessage( MSG& msg );
	virtual bool OnMessage            ( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& returnValue );
	virtual bool OnInactiveUpdate     ( void );
	virtual bool OnFrame              ( float deltaTime, float actualDeltaTime );
	virtual bool OnSysFrame           ( float deltaTime, float actualDeltaTime );
	virtual bool OnBeginFrame         ( void );
	virtual bool OnEndFrame           ( void );

	// render help
	bool UpdateFrame( float deltaTime, float actualDeltaTimes, bool copyToPrimary = true, bool doUpdate = true );
	void PostOpaqueRender( void );

	void RSPauseAllClients( bool pause );
FEX	void RSPauseAllClients( bool pause, PlayerId requestingPlayer );
FEX	void RCUserPause( bool pause, double time, PlayerId requestingPlayer );

	// utility
	void DrawDebugConsoles( void );
	bool FirstRunEula( void );
	bool CheckDiskSpace( void );
	bool CheckTaskAssigned( void );
	void CheckSoundFading( void );

#	if !GP_RETAIL
	void UpdateFooter( void );
FEX void DumpPerformanceSummary( bool footerMode, ReportSys::Context * ctx );
	bool PrivateReloadObjects( eReloadType type );
#	endif // !GP_RETAIL

// Settings.

	// options
	eGOptions m_GOptions;				// configuration
	gpwstring m_LastSaveGame;			// name of last save game
	bool      m_InPauseRequestScope;	// used for multiplayer pause recursion safety
	int       m_RenderSkipCount;		// skip this many frames of rendering
	bool      m_IsSkippingTime;			// are we currently skipping time?
	bool      m_IsSoundFaded;			// is sound faded out?
	bool	  m_CopyFullMovieFrame;		// should we copy a full movie frame?

	// fixed sim frame rate
	float m_SimFrameRate;				// frame rate to force sim to
	float m_SimFramePending;			// how much time in this sim we've built up so far

#	if !GP_RETAIL
	gpstring  m_LastProfileOutput;		// misc storage
#	endif // !GP_RETAIL

// Owned objects.

	// $ these are in order of construction

	// core
#	if !GP_RETAIL
	my gpprofiler*  m_GpProfiler;
#	endif // !GP_RETAIL

	// services
	my GpConsole*		m_GpConsole;
	my Services*		m_Services;
	my GameState*		m_GameState;
	my GameMovie*		m_GameMovie;
	my World*			m_World;
	my UI*				m_UI;
	my ReportSys::Sink* m_GameConsoleSink;

	// stats gathering
#	if GP_ENABLE_STATS
	unsigned int    m_CpuStatsGraphTex;
	unsigned int    m_CpuStatsPHistoryTex;
	unsigned int    m_CpuStatsUHistoryTex;
#	endif // GP_ENABLE_STATS
	
#	if !GP_RETAIL
	typedef stdx::fast_vector <gpstring>		StringBuffer;
	StringBuffer m_PendingOrderBuffer;		// previous pending orders this is for perfmon
	gpstring	m_PendingOrderMarker;		// marker used in the pending order output log
#	endif

	// MP
	int m_MPSleep;

	// $$$ MARKED FOR DEATH
#	if !GP_RETAIL
#	if GP_ENABLE_STATS
	my gpc_gpstats*  m_GpcGpStats;
#	endif // GP_ENABLE_STATS
	my gpc_mman*     m_GpcMman;
	my gpc_file*     m_GpcFile;
	my gpc_log*      m_GpcLog;
	my gpc_reload*   m_GpcReload;
	my gpc_streamer* m_GpcStreamer;
	my gpc_fps*      m_GpcFps;
#	endif // !GP_RETAIL

	FUBI_CLASS_INHERIT( TattooGame, Inherited );
	FUBI_SINGLETON_CLASS( TattooGame, "The Dungeon Siege game engine." );
	SET_NO_COPYING( TattooGame );
};

MAKE_ENUM_BIT_OPERATORS( TattooGame::eGOptions );

#define gTattooGame TattooGame::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __TATTOOGAME_H

//////////////////////////////////////////////////////////////////////////////
