//////////////////////////////////////////////////////////////////////////////
//
// File     :  GamePersist.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains save/restore-the-game functionality for Tattoo.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GAMEPERSIST_H
#define __GAMEPERSIST_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"
#include <map>

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace Tank
{
	class Builder;
}

namespace FuBi
{
	struct PersistWriter;
	struct PersistReader;
	struct Manifest;
}

class RapiMemImage;
struct TankManifest;

//////////////////////////////////////////////////////////////////////////////
// struct SaveInfo declaration

// save game infos are returned here
class SaveInfo
{
public:
	SET_NO_INHERITED( SaveInfo );

// Data.

	// given by ui
	gpwstring          m_FileName;				// filename to use when requesting load
	bool               m_IsQuickSave;			// true if quicksaved
	bool               m_IsAutoSave;			// true if autosaved
	bool               m_IsRetailContent;		// true if we're using retail content

	// calculated for screen
	gpwstring          m_ScreenName;			// name of save game to show on screen
	gpwstring          m_SaveTimeText;			// formatted translated time of savegame converted to local time
	gpwstring          m_ElapsedTimeText;		// formatted translated elapsed time in game

	// read from system - per session
	gpversion          m_ProductVersion;		// version of product at save time (queried directly from exe)
	gpstring           m_ProductCompileMode;	// retail, release, etc.
	FILETIME           m_SaveTime;				// UTC time that this game was saved (use for sorting only)
	bool               m_IsSinglePlayer;		// true if saved from single player game
	gpstring           m_TeleportText;			// teleport text for hero
	gpstring           m_RegionText;			// what region hero is in
	double             m_ElapsedTime;			// elapsed time playing the game
	double             m_SaveDuration;			// amount of time it took to save the game

	// read from system - global
	gpwstring          m_PlayerName;			// name of this player
	gpwstring          m_HeroName;				// name of hero for this player
	gpwstring          m_MapScreenName;			// screen name of map playing in
	gpstring           m_MapName;				// name of map playing in

	// special
	my FuBi::Manifest* m_FuBiManifest;			// manifest of the EXE imports via FuBi
	my TankManifest*   m_TankManifest;			// manifest of all loaded tanks at time of save
	my RapiMemImage*   m_Thumbnail;				// small screenshot

// Methods.

	SaveInfo( void );
	SaveInfo( const SaveInfo& other );
	SaveInfo( const gpwstring& fileName, bool isQuickSave, bool isAutoSave );
   ~SaveInfo( void );

	void  SaveToFuel			( FuelHandle fuel, bool forHistory = false ) const;
	bool  LoadFromFuel			( FuelHandle fuel, bool getThumbnail, bool wantSp, bool wantMp, bool forHistory = false );
	void  LoadFromSystem		( const gpwstring& fileName, bool isQuickSave, bool isAutoSave );
	void  ConvertForScreen		( void );
	DWORD CreateThumbnailTexture( void );

// Predicates.

	struct GreaterByFileTime
	{
		bool operator () ( const SaveInfo* l, const SaveInfo* r ) const
		{
			return ( ::CompareFileTime( &l->m_SaveTime, &r->m_SaveTime ) > 0 );
		}
	};

	struct LessByScreenName
	{
		bool operator () ( const SaveInfo* l, const SaveInfo* r ) const
		{
			return ( compare_no_case( l->m_ScreenName, r->m_ScreenName ) < 0 );
		}
	};

// Other.

	SaveInfo& operator = ( const SaveInfo& other );

private:
	void init( void );
};

//////////////////////////////////////////////////////////////////////////////
// struct SaveInfoColl declaration

struct SaveInfoColl : stdx::fast_vector <SaveInfo*>
{
	SET_INHERITED( SaveInfoColl, stdx::fast_vector <SaveInfo*> );

	SaveInfoColl( void )  {  }
   ~SaveInfoColl( void )  {  clear();  }

	void clear( void )
	{
		stdx::for_each( *this, stdx::delete_by_ptr() );
		Inherited::clear();
	}

	SET_NO_COPYING( SaveInfoColl );
};

//////////////////////////////////////////////////////////////////////////////
// struct CharacterInfo declaration

struct CharacterInfo
{
	struct SkillInfo
	{
		gpwstring m_ScreenName;
		float     m_Level;
	};

	typedef std::map <gpstring, SkillInfo, istring_less> SkillDb;

// Data.

	gpwstring        m_ScreenName;				// screen name of this character (possibly chosen by user previously)
	gpwstring        m_ActorClass;				// class of actor (fighter, ranger, etc.)
	SkillDb          m_Skills;					// skills for this actor
	gpwstring        m_FileName;				// filename of saved game this is stored in (use for restoring)
	FILETIME         m_SaveTime;				// time this was saved out
	int              m_Index;					// index of character in collection (must be used to import)
	my RapiMemImage* m_Portrait;				// portrait saved out with this character

// Methods.

	CharacterInfo( void )
	{
		m_Index    = 0;
		m_Portrait = NULL;

		::ZeroObject( m_SaveTime );
	}

   ~CharacterInfo( void );

// Predicates.

	struct GreaterByFileTime
	{
		bool operator () ( const CharacterInfo* l, const CharacterInfo* r ) const
		{
			return ( ::CompareFileTime( &l->m_SaveTime, &r->m_SaveTime ) > 0 );
		}
	};

	SET_NO_COPYING( CharacterInfo );
};

//////////////////////////////////////////////////////////////////////////////
// struct CharacterInfoColl declaration

struct CharacterInfoColl : stdx::fast_vector <CharacterInfo*>
{
	SET_INHERITED( CharacterInfoColl, stdx::fast_vector <CharacterInfo*> );

	~CharacterInfoColl( void )
	{
		clear();
	}

	void clear( void )
	{
		stdx::for_each( *this, stdx::delete_by_ptr() );
		Inherited::clear();
	}
};

//////////////////////////////////////////////////////////////////////////////
// class GameXfer declaration

class GameXfer
{
public:
	SET_NO_INHERITED( GameXfer );

// Types.

	enum eOptions
	{
		OPTION_NONE         =      0,

		OPTION_TEXT         = 1 << 0,		// save this in text mode
		OPTION_TANK         = 1 << 1,		// save files into a tank
		OPTION_PARTY_SAVE   = 1 << 2,		// this includes a party save (affects default extension/path)
		OPTION_WORLD_SAVE   = 1 << 3,		// this includes a world save (affects default extension/path)
		OPTION_SINGLEPLAYER = 1 << 4,		// single player save game (affects default extension/path)
		OPTION_MULTIPLAYER  = 1 << 5,		// multiplayer save game (affects default extension/path)
		OPTION_QUICK_SAVE   = 1 << 6,		// quick saved game (affects default extension/path)
		OPTION_AUTO_SAVE    = 1 << 7,		// auto saved game (affects default extension/path)
	};

	static gpwstring MakeSaveGameFileName     ( const wchar_t* name, eOptions options, bool wildcard = false );
	static gpwstring AddSaveGameExtension     ( const wchar_t* name, eOptions options, bool wildcard = false );
	static eOptions  MakeSaveOptionsAndName   ( const wchar_t*& name, bool quickSave, bool autoSave );
	static bool      DoesSaveGameExist        ( const wchar_t* name, eOptions options );
	static bool      HasValidSaveGameExtension( const wchar_t* name );

// Setup.

	// ctor/dtor
	GameXfer( eOptions options = OPTION_NONE );
	virtual ~GameXfer( void )									{  }

	// options
	void     SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	eOptions GetOptions   ( void ) const						{  return ( m_Options );  }
	void     ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void     ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool     TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	// helpers
	bool XferCommon( FuBi::PersistContext& persist );

protected:

	bool m_StoppedLoadingThread;			// state info

private:

	eOptions m_Options;						// current xfer options

	SET_NO_COPYING( GameXfer );
};

MAKE_ENUM_BIT_OPERATORS( GameXfer::eOptions );

//////////////////////////////////////////////////////////////////////////////
// class GameSaver declaration

// $$$ modify version info for tank builder to store save game-specific desc
//     or author info etc.

// $$$ before doing a save, be sure to verify that all systems are in sync
//     and all retry packets are ok to ignore.

class GameSaver : public GameXfer
{
public:
	SET_INHERITED( GameSaver, GameXfer );

// Setup.

	// ctor/dtor
	GameSaver( eOptions options = OPTION_NONE );
	virtual ~GameSaver( void );

// Save methods.

	// global save methods
	bool BeginSave    ( const wchar_t* saveName );
	bool FullSave     ( void );
	bool FullPartySave( int slot = -1 );
	bool CommitSave   ( void );
	bool AbortSave    ( void );
	bool IsSaving     ( void ) const			{  return ( !m_SaveFileName.empty() );  }

	// individual save methods
	bool SaveParty    ( int slot = -1 );
	bool SaveWorld    ( void );
	bool SavePortraits( int slot = -1 );
	bool SaveThumbnail( void );

	// query
	const gpwstring& GetSaveName( void ) const	{  return ( m_SaveFileName );  }

	// special delete method
	bool DeleteSave( const wchar_t* saveName, bool* saveFileExists = NULL, bool dirOnly = false );
	
	// clean out saved history - use this when starting new games
#	if GP_ENABLE_DEV_FEATURES
	static void LoadSaveHistory ( FuelHandle fuel );
	static void ClearSaveHistory( void )						{  ms_SaveHistory.clear();  }
	static void AddSaveHistory  ( const SaveInfo& saveInfo )	{  ms_SaveHistory.push_back( new SaveInfo( saveInfo ) );  }
#	endif // GP_ENABLE_DEV_FEATURES

private:
	void                   reset       ( void );
	void                   xfer        ( FuBi::PersistContext& persist );
	FileSys::StreamWriter* createWriter( const char* fileName );
	bool                   commitWriter( FileSys::StreamWriter* writer );

	// spec
	gpwstring m_SaveFileName;					// where to save game into

	// state
	double m_StartTime;							// when did we begin save?

	// tank saving state
	my Tank::Builder* m_TankBuilder;			// our tank builder

#	if GP_ENABLE_DEV_FEATURES
	static SaveInfoColl ms_SaveHistory;
#	endif // GP_ENABLE_DEV_FEATURES

	SET_NO_COPYING( GameSaver );
};

//////////////////////////////////////////////////////////////////////////////
// class GameLoader declaration

class GameLoader : public GameXfer
{
public:
	SET_INHERITED( GameLoader, GameXfer );

// Setup.

	// ctor/dtor
	GameLoader( eOptions options = OPTION_NONE );
	virtual ~GameLoader( void );

// Load methods.

	// global load methods
	bool BeginLoad( const wchar_t* loadName );
	void EndLoad  ( void );
	bool IsLoading( void ) const							{  return ( !m_LoadFileName.empty() );  }

	// individual load methods
	bool LoadWorld       ( void );
	bool LoadWorldPartial( void );
	bool LoadParty       ( const SiegePos& where, bool fullLoad );

	// attributes
	FILETIME GetTimeStamp( void ) const						{  gpassert( IsLoading() );  return ( m_TimeStamp );  }
	bool     FillInfo    ( SaveInfo& info, bool getThumbnail = true );

	// misc restored data
	eWorldState GetLoadedWorldState( void ) const			{  return ( m_LoadedWorldState );  }

	// query
	static int GetSaveGameInfos( CharacterInfoColl* ccoll, SaveInfoColl* scoll, eOptions options, const char* fileSpec, bool forRead, bool getPics = true );
	static int GetSaveGameInfos( CharacterInfoColl* ccoll, SaveInfoColl* scoll, eOptions options, bool forRead, bool getPics = true );
	static int GetSaveGameInfos( SaveInfoColl* scoll, eOptions options, const char* fileSpec, bool forRead, bool getPics = true );
	static int GetSaveGameInfos( SaveInfoColl* scoll, eOptions options, bool forRead, bool getPics = true );
	static int GetSaveGameInfos( CharacterInfoColl* ccoll, eOptions options, bool forRead, bool getPics = true );
	static int GetSaveGameInfos( eOptions options, bool forRead );

	// imports
	bool          GetCharacterInfos           ( CharacterInfoColl& coll, bool getPics );
	int           GetCharacterCount           ( void );
	FuelHandle    GetCharacterByIndex         ( int index );
	bool          ImportCharacterByIndex      ( int index, bool setAsPrimary = true );
	bool          ImportAllCharacters         ( void )		{  return ( ImportCharacterByIndex( -1 ) );  }
	DWORD         CreatePortraitTextureByIndex( int index );
	RapiMemImage* CreatePortraitImageByIndex  ( int index );

private:
	void reset( void );

	// spec
	gpwstring m_OriginalName;				// original gameloader request
	gpwstring m_LoadFileName;				// where to load game from

	// general state
	TextFuelDB* m_Db;						// fuel db for root
	SaveInfo    m_SaveInfo;					// save header from info.gas
	FILETIME    m_TimeStamp;				// timestamp of file (tank or path)
	gpstring    m_ErrorString;				// errors go here

	// tank loading state
	my FileSys::IFileMgr* m_FileMgr;		// filemgr where we load from, installed to the "load" volume in filesys

	// misc restored data
	eWorldState m_LoadedWorldState;			// world state for when we saved the game

	SET_NO_COPYING( GameLoader );
};


//////////////////////////////////////////////////////////////////////////////
// special global function for copying a save game and renaming the character's
// screen name.  
// Notes on usage:
// saveFilename is the .dsparty filename without the path
// newScreenName is the screen name that will be used for the copied save game 
bool CopyRenameCharacterFromSave( const wchar_t* saveFileName, const wchar_t* newScreenName );


//////////////////////////////////////////////////////////////////////////////

#endif  // __GAMEPERSIST_H

//////////////////////////////////////////////////////////////////////////////
