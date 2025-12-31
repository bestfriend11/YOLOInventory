//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDb.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the database that manages Game Objects.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GODB_H
#define __GODB_H

//////////////////////////////////////////////////////////////////////////////

#include "BucketVector.h"
#include "Flamethrower_Types.h"
#include "GoBase.h"
#include "KernelTool.h"

#include <map>
#include <set>

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace siege
{
	typedef stdx::fast_vector <ASPECTINFO> WorldObjectColl;
}

struct GoDbShouldPersistHelper;
struct MachineAddrQuery;
class Flamethrower_params;

struct GoDbDeleteXfer
{
	Goid m_Goid;
	bool m_Retire;
	bool m_FadeOut;
	bool m_MpDelayed;

	GoDbDeleteXfer( void )
	{
		::ZeroObject( *this );
	}

	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};

struct GoDbMondoDeleteXfer
{
	GoidColl m_Goids;

	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};

//////////////////////////////////////////////////////////////////////////////
// class GoDb declaration

class GoDb : public Singleton <GoDb>
{
public:
	SET_NO_INHERITED( GoDb );

// Structs

	struct EffectEntry
	{
		SFxSID                  m_ScriptId;
		eWorldEvent             m_CreationEvent;
		bool                    m_Active;
		bool                    m_XferOnSync;
		Goid                    m_LinkedTarget;
		Goid                    m_LinkedSource;
		my Flamethrower_params* m_OriginalParams;

		EffectEntry( void )  {  m_OriginalParams = NULL;  }

		EffectEntry( SFxSID scriptId, eWorldEvent creationEvent, bool active, bool xferOnSync )
		{
			m_ScriptId       = scriptId;
			m_CreationEvent  = creationEvent;
			m_Active         = active;
			m_XferOnSync     = xferOnSync;
			m_LinkedTarget   = GOID_INVALID;
			m_LinkedSource   = GOID_INVALID;
			m_OriginalParams = NULL;
		}

	   ~EffectEntry( void );

		bool Xfer        ( FuBi::PersistContext& persist );
		void WriteForSync( FuBi::BitPacker& packer, Goid owner = GOID_INVALID ) const;
	};

#	if !GP_RETAIL
	struct HudInfo
	{
		int m_RpcCtorStackCount;
		int m_RpcFrameStackCount;

		HudInfo( void )
		{
			m_RpcCtorStackCount = 0;
			m_RpcFrameStackCount = 0;
		}
	};
#	endif // !GP_RETAIL

// Types.

	typedef std::multimap <Go*, Go*> WatchDb;
	typedef std::multimap <siege::database_guid, Go*> SiegeGoDb;
	typedef CBFunctor0 SelectionChangedCb;
	typedef CBFunctor1wRet <Go*, bool> ForEachGoCb;
	typedef std::multimap <Go*, EffectEntry> EffectDb;			// go -> siegefx attached to it
#	if !GP_RETAIL
	typedef std::map <Go*, HudInfo> HudInfoDb;
#	endif // !GP_RETAIL

// Setup.

	// ctor/dtor
	GoDb( void );
   ~GoDb( void );

	// configuration
#	if GP_RETAIL
FEX	bool  IsEditMode           ( void ) const						{  return ( false );  }
#	else // GP_RETAIL
	void  SetEditMode          ( bool set = true )					{  m_EditMode = set;  }
FEX	bool  IsEditMode           ( void ) const						{  return ( m_EditMode );  }
	void  SetOverrideDetectOnly( bool set = true )					{  m_OverrideDetectOnly = set;  }
	bool  GetOverrideDetectOnly( void ) const						{  return ( m_OverrideDetectOnly );  }
#	endif // GP_RETAIL

	// free all resources
	void Shutdown         ( bool clean = true );
	bool IsShuttingDown   ( void ) const							{  return ( m_IsShuttingDown );  }
	bool IsShuttingDownBad( void ) const							{  return ( m_IsShuttingDownBad );  }

	// persistence
	void SetLockedForSaveLoad( bool set = true );
	bool IsLockedForSaveLoad ( void ) const							{  return ( m_LockedForSaveLoad );  }
	bool Xfer                ( FuBi::PersistContext& persist );
	void XferFxForSync       ( FuBi::BitPacker& packer, Go* go );
	void XferQuestBits       ( FuBi::PersistContext& persist, Goid goid );
	bool PostLoad            ( void );
	bool IsPostLoading		 ( void )								{ return ( m_PostLoading ); }

	// importing characters/stash
	void ImportCharacter( PlayerId playerId, int slot, const gpstring& spec );
	void ImportStash( PlayerId playerId, const gpstring& spec );

	// special update root access - don't mess with this unless you know what you're doing!
	Go* GetUpdateRoot( void ) const									{  return ( m_GoUpdateRoot );  }

	// callbacks
	void UpdateLevelOfDetail( void );
	void UpdateShadows( void );

	// spec setup
	static FuelHandle FindImportCharacterFuel       ( PlayerId playerId = PLAYERID_INVALID, int slot = 0, bool autoCreate = false );
	static gpstring   MakeImportCharacterFuelAddress( PlayerId playerId = PLAYERID_INVALID, int slot = 0 );
	static FuelHandle FindImportStashFuel			( PlayerId playerId = PLAYERID_INVALID, bool autoCreate = false );
	static gpstring   MakeImportStashFuelAddress	( PlayerId playerId = PLAYERID_INVALID );

// Operations.

	// broadcast events
	void SysUpdate( float deltaTime );
	void Update( float deltaTime, float actualDeltaTime );
	void UpdateMouseShadow( void );

	// saving go's
#	if !GP_RETAIL
	void BuildNodeContentIndex( FuelHandle fuel ) const;
	void SaveGo               ( Goid goid, FuelHandle fuel ) const;	// save an individual go (fuel handle points to xxx.gas in 'objects' folder) - must have an edit component
	void SaveAllGos           ( FuelHandle fuel ) const;			// save all global go's (fuel handle points to 'objects' folder) - they must all have edit components
#	endif // !GP_RETAIL

#	if !GP_RETAIL
FEX void DebugBreakOn( Goid breakGoid )								{  m_DebugUpdateBreakGoid = breakGoid;  }
#	endif // !GP_RETAIL

// Database management.

	// query
	Goid  FindCloneSource              ( const char* templateName );			// construct clone source go based on a template (for ammo, waypoints, etc)
	Goid  FindAndLockCloneSource       ( const char* templateName, Go* owner );	// find a clone source then lock it in memory, tying it to the given owner
	void  LockCloneSource              ( Goid cloneSource, Go* owner );			// just lock it in memory, tying it to the given owner
	bool  IsConstructingGo             ( void ) const;							// true if we're in the middle of constructing a go (must lock go construction!!)
	bool  IsCurrentThreadConstructingGo( void ) const;							// see if current thread is constructing
FEX bool  IsConstructingStandaloneGo   ( void ) const;							// true if we're constructing a go and it's standalone (not allowed to construct children)
	bool  IsImportingStandaloneGo      ( void ) const;							// true if we're importing a go from fuel and it's standalone
	Goid  SGetMostRecentNewGoid        ( void );								// (server only) gets the most recently created/cloned Go ( destructive: clears the value when it gets it)
	DWORD GetMostRecentFixedSeed       ( void );								// gets the random seed used to create most recent go

	// statistics
	int   GetGlobalGoCount       ( void ) const						{  return ( m_GlobalDbCount );  }
	int   GetLocalGoCount        ( void ) const						{  return ( m_LocalDb.size() );  }
	int   GetCloneSrcGoCount     ( void ) const						{  return ( GetCloneSrcTotalGoCount() - m_CloneSrcUnloadedCount );  }
	int   GetCloneSrcTotalGoCount( void ) const						{  return ( m_CloneSrcDb.size() );  }
	int   GetTotalGoCount        ( void ) const						{  return ( GetGlobalGoCount() + GetLocalGoCount() + GetCloneSrcGoCount() );  }
	int   GetExpiringGoCount     ( void ) const						{  return ( m_ExpireDb.size() );  }
	DWORD GetPrimaryThreadId     ( void ) const						{  return ( m_PrimaryThreadId );  }

#	if !GP_RETAIL
	int   GetGlobalGoDirtyCount     ( void ) const					{  return ( m_GlobalDbDirtyCount );  }
	int   GetGlobalGoServerOnlyCount( void ) const					{  return ( m_GlobalDbServerOnlyCount );  }
	int   GetLastUpdateCount        ( void ) const					{  return ( m_LastUpdateCount );  }
	int   GetLastRenderCount        ( void ) const					{  return ( m_LastRenderCount );  }
	int   GetLastViewCount          ( void ) const					{  return ( m_LastViewCount   );  }
#	endif // !GP_RETAIL

	// preloading
FEX void PreloadCloneSource( Go* client, const char* templateName );	// issue request to preload a clone source

	// creation via scid
FEX Goid       SCreateGo     ( const GoCreateReq& createReq );
private:
	FuBiCookie RCCreateGo      ( DWORD machineId, Goid startGoid, DWORD randomSeed, const GoCreateReq& createReq, bool standalone = false, const_mem_ptr data = const_mem_ptr() );
FEX FuBiCookie RCCreateGoPacker( const_mem_ptr packola );
	bool       RCCreateGoFinal ( Goid startGoid, DWORD randomSeed, const GoCreateReq& createReq, bool standalone, const_mem_ptr data, int packetSize, bool fromRpc );
	Goid       CreateGo        ( const GoCreateReq& createReq, bool standalone = false );
public:

	// special world streamer loading
	Goid SLoadGo    ( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid );
	Goid LoadLocalGo( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid, bool lodfi, bool fadeIn );

	// special jit creation of go's being set as all/not-all clients go's
	void SetAsAllClientsGo( Go* go, bool set = true );

	// special scid-swapping when transferring guts
	void SwapScids( Go* a, Go* b );

	// creation via cloning
FEX FuBiCookie RSCloneGo( const GoCloneReq& cloneReq );
FEX FuBiCookie RSCloneGo( const GoCloneReq& cloneReq, const char* templateName );			   
FEX Goid       SCloneGo ( const GoCloneReq& cloneReq, const char* templateName, const DWORD* forceRandomSeed );
FEX Goid       SCloneGo ( const GoCloneReq& cloneReq, const char* templateName );
FEX Goid       SCloneGo ( const GoCloneReq& cloneReq );
	Goid       CloneGo  ( const GoCloneReq& cloneReq, const char* templateName, bool standalone = false );
	Goid       CloneGo  ( const GoCloneReq& cloneReq, bool standalone = false, bool isPContent = false, bool calledFromClone = false );
	bool       CloneGo  ( Goid explicitGoid, const GoCloneReq& cloneReq, const char* templateName );	
private:	
	FuBiCookie RCCloneGo      ( DWORD machineId, Goid startGoid, DWORD randomSeed, const GoCloneReq& cloneReq, const char* templateName, bool standalone = false, const_mem_ptr data = const_mem_ptr() );
FEX FuBiCookie RCCloneGoPacker( const_mem_ptr packola );
	bool       RCCloneGoFinal ( Goid startGoid, DWORD randomSeed, const GoCloneReq& cloneReq, const char* templateName, bool standalone, const_mem_ptr data, int packetSize, bool fromRpc );
	void       RCSyncGo       ( Player* forPlayer, Go* go, bool standalone );

public:

	// local cloning
FEX Goid CloneLocalGo( const GoCloneReq& cloneReq );

	// creation via cloning (for siege edit use only) using prealloc'd scid, possibly loading from the given fuel address
#	if !GP_RETAIL
	Goid CloneNonSpawnedGo( const GoCloneReq& cloneReq, Scid newScid, FastFuelHandle fuel = FastFuelHandle() );
#	endif // !GP_RETAIL

	// placement - returns true if a change happened
	bool TransferNodeOccupant( Go* occupant, const siege::database_guid& oldNode, const siege::database_guid& newNode );
	bool AddNodeOccupant     ( Go* occupant, const siege::database_guid& newNode );
	bool RemoveNodeOccupant  ( Go* occupant, const siege::database_guid& oldNode );
	bool IsNodeOccupant      ( Go* occupant, const siege::database_guid& node ) const;

	// node occupancy query
	int  GetNodeOccupants     ( const siege::database_guid& node, GoidColl& coll ) const;
	bool GetNodeOccupantsIters( const siege::database_guid& node, SiegeGoDb::iterator& begin, SiegeGoDb::iterator& end );

	// nodes changing their membership
	void NodeChangeMembership( const siege::SiegeNode& node, unsigned int oldMembership, bool nodeWasDeleted );

	// deletion
FEX void RSMarkForDeletion( Scid scid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );		// request deletion of an individual go next sim
FEX void RSMarkForDeletion( Goid goid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );		// request deletion of an individual go next sim
FEX void RSMarkForDeletion( Go*  go,   bool retire = true, bool fadeOut = false, bool mpdelayed = false );		// request deletion of an individual go next sim
FEX void SMarkForDeletion ( Scid scid, bool retire, bool fadeOut, bool mpdelayed );
FEX void SMarkForDeletion ( Goid goid, bool retire, bool fadeOut, bool mpdelayed );
FEX void SMarkForDeletion ( Go*  go,   bool retire, bool fadeOut, bool mpdelayed );
FEX void SMarkForDeletion ( Scid scid, bool retire, bool fadeOut )						{  SMarkForDeletion( scid, retire, fadeOut, false );  }
FEX void SMarkForDeletion ( Goid goid, bool retire, bool fadeOut )						{  SMarkForDeletion( goid, retire, fadeOut, false );  }
FEX void SMarkForDeletion ( Go*  go,   bool retire, bool fadeOut )						{  SMarkForDeletion( go, retire, fadeOut, false );  }
FEX void SMarkForDeletion ( Scid scid, bool retire )									{  SMarkForDeletion( scid, retire, false );  }
FEX void SMarkForDeletion ( Goid goid, bool retire )									{  SMarkForDeletion( goid, retire, false );  }
FEX void SMarkForDeletion ( Go*  go,   bool retire )									{  SMarkForDeletion( go, retire, false );  }
FEX void SMarkForDeletion ( Scid scid )													{  SMarkForDeletion( scid, true );  }
FEX void SMarkForDeletion ( Goid goid )													{  SMarkForDeletion( goid, true );  }
FEX void SMarkForDeletion ( Go*  go )													{  SMarkForDeletion( go, true );  }
private:
	void RCMarkForDeletion           ( DWORD machineId, Goid goid, bool retire, bool fadeOut, bool mpdelayed );
	void RCMarkForDeletion           ( DWORD machineId, Go* go, bool retire, bool fadeOut, bool mpdelayed );
FEX void RCMarkForDeletionPacker     ( const_mem_ptr packola );
FEX void RCMarkForMondoDeletionPacker( const_mem_ptr packola );
	bool MarkForDeletion             ( Goid goid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );
	void MarkForDeletion             ( Go* go, bool retire = true, bool fadeOut = false, bool mpdelayed = false );
public:

	// special deletion commands
FEX	void RSMarkGoAndChildrenForDeletion ( Scid scid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );	// request deletion of go and all children next sim
FEX	void RSMarkGoAndChildrenForDeletion ( Goid goid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );	// request deletion of go and all children next sim
FEX	void RSMarkGoAndChildrenForDeletion ( Go*  go,   bool retire = true, bool fadeOut = false, bool mpdelayed = false );	// request deletion of go and all children next sim
FEX	void SMarkGoAndChildrenForDeletion  ( Scid scid, bool retire, bool fadeOut, bool mpdelayed );
FEX	void SMarkGoAndChildrenForDeletion  ( Goid goid, bool retire, bool fadeOut, bool mpdelayed );
FEX	void SMarkGoAndChildrenForDeletion  ( Go*  go,   bool retire, bool fadeOut, bool mpdelayed );
FEX	void SMarkGoAndChildrenForDeletion  ( Scid scid, bool retire, bool fadeOut )		{  SMarkGoAndChildrenForDeletion( scid, retire, fadeOut, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Goid goid, bool retire, bool fadeOut )		{  SMarkGoAndChildrenForDeletion( goid, retire, fadeOut, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Go*  go,   bool retire, bool fadeOut )		{  SMarkGoAndChildrenForDeletion( go, retire, fadeOut, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Scid scid, bool retire )						{  SMarkGoAndChildrenForDeletion( scid, retire, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Goid goid, bool retire )						{  SMarkGoAndChildrenForDeletion( goid, retire, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Go*  go,   bool retire )						{  SMarkGoAndChildrenForDeletion( go, retire, false );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Scid scid )									{  SMarkGoAndChildrenForDeletion( scid, true );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Goid goid )									{  SMarkGoAndChildrenForDeletion( goid, true );  }
FEX	void SMarkGoAndChildrenForDeletion  ( Go*  go )										{  SMarkGoAndChildrenForDeletion( go, true );  }
	void MarkForDeletionReassignChildren( Scid scid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );	// request deletion of go and reassign its children to the parent of this go
	void MarkForDeletionReassignChildren( Goid goid, bool retire = true, bool fadeOut = false, bool mpdelayed = false );	// request deletion of go and reassign its children to the parent of this go

	// blast deletion
	void DeletePlayerGos( PlayerId playerId, bool retire = true );	// request deletion of a player's entire set of go's (called as part of a larger DeletePlayer() process initiated by the server)

	// scid-specific data manipulation
FEX	DWORD GetScidBits  ( Scid scid, DWORD defValue ) const;
FEX	DWORD GetScidBits  ( Scid scid ) const										{  return ( GetScidBits( scid, 0 ) );  }
FEX	bool  GetScidBit   ( Scid scid, int index, bool defValue ) const;
FEX	bool  GetScidBit   ( Scid scid, int index ) const							{  return ( GetScidBit( scid, index, false ) );  }
FEX	DWORD SSetScidBits ( Scid scid, DWORD value );
FEX	void  RCSetScidBits( Scid scid, DWORD value );
	DWORD SetScidBits  ( Scid scid, DWORD value, bool forRpc );
FEX	DWORD SetScidBits  ( Scid scid, DWORD value )								{  return ( SetScidBits( scid, value, false ) );  }
FEX	bool  SSetScidBit  ( Scid scid, int index, bool value );
FEX	void  RCSetScidBit ( Scid scid, int index, bool value );
	bool  SetScidBit   ( Scid scid, int index, bool value, bool forRpc );
FEX	bool  SetScidBit   ( Scid scid, int index, bool value )						{  return ( SetScidBit( scid, index, value, false ) );  }

	// map/character-specific data manipulation
FEX	int             GetQuestInt     ( Goid goid, const char* mapName, const char* keyName, int defInt ) const;
FEX	int             GetQuestInt     ( Goid goid, const char* mapName, const char* keyName ) const;
FEX	bool            GetQuestBool    ( Goid goid, const char* mapName, const char* keyName, bool defBool ) const;
FEX	bool            GetQuestBool    ( Goid goid, const char* mapName, const char* keyName ) const;
FEX	float           GetQuestFloat   ( Goid goid, const char* mapName, const char* keyName, float defFloat ) const;
FEX	float           GetQuestFloat   ( Goid goid, const char* mapName, const char* keyName ) const;
FEX	const gpstring& GetQuestString  ( Goid goid, const char* mapName, const char* keyName, const gpstring& defString ) const;
FEX	const gpstring& GetQuestString  ( Goid goid, const char* mapName, const char* keyName ) const;
FEX	void            SSetQuestInt    ( Goid goid, const char* mapName, const char* keyName, int valInt );
FEX	void            RSSetQuestInt   ( Goid goid, const char* mapName, const char* keyName, int valInt );
FEX	void            SetQuestInt     ( Goid goid, const char* mapName, const char* keyName, int valInt );
FEX	void            SSetQuestBool   ( Goid goid, const char* mapName, const char* keyName, bool valBool );
FEX	void            RSSetQuestBool  ( Goid goid, const char* mapName, const char* keyName, bool valBool );
FEX	void            SetQuestBool    ( Goid goid, const char* mapName, const char* keyName, bool valBool );
FEX	void            SSetQuestFloat  ( Goid goid, const char* mapName, const char* keyName, float valFloat );
FEX	void            RSSetQuestFloat ( Goid goid, const char* mapName, const char* keyName, float valFloat );
FEX	void            SetQuestFloat   ( Goid goid, const char* mapName, const char* keyName, float valFloat );
FEX	void            SSetQuestString ( Goid goid, const char* mapName, const char* keyName, const gpstring& valString );
FEX void			RSSetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString );
FEX	void            RCSetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString );
FEX	void            SetQuestString  ( Goid goid, const char* mapName, const char* keyName, const gpstring& valString );

	// expiration
FEX	bool StartExpiration ( Go* go, bool forced = true );			// tell this go to start expiring now - returns false if start failed due to lower priority of new request
FEX	bool CancelExpiration( Go* go, bool forced = true );			// tell this go to cancel its expiration (if it's expiring) now - returns false if cancel failed due to lower priority of new request
	bool IsExpiring      ( Go* go, float* timeLeft ) const;			// is this go expiring?
FEX	bool IsExpiring      ( Go* go ) const							{  return ( IsExpiring( go, NULL ) );  }

	// force everything that is expiring to expire right now (FOR TEST ONLY)
#	if !GP_RETAIL
	void ExpireAllImmediately( void );
#	endif // !GP_RETAIL

	// fading
	void StartFadeIn( Go* go );

// Selection.

	// selections
FEX bool Select     ( Goid goid, bool select );						// returns previous selection state of go
FEX bool Select     ( Goid goid )									{  return ( Select( goid, true  ) );  }
FEX bool Deselect   ( Goid goid )									{  return ( Select( goid, false ) );  }
FEX void DeselectAll( void );										// deselects all currently selected go's
FEX bool IsSelected ( Goid goid ) const;							// returns true if go is selected

	// direct access (const only!)
	const GopColl& GetSelection( void ) const						{  return ( m_SelectionColl );  }

	// selection access
	GopColl::const_iterator GetSelectionBegin( void ) const			{  return ( m_SelectionColl.begin() );  }
	GopColl::const_iterator GetSelectionEnd  ( void ) const			{  return ( m_SelectionColl.end  () );  }
	GopColl::iterator       GetSelectionBegin( void )				{  return ( m_SelectionColl.begin() );  }
	GopColl::iterator       GetSelectionEnd  ( void )				{  return ( m_SelectionColl.end  () );  }

	// selection callback
	void SetSelectionChangedCb( SelectionChangedCb cb )				{  m_SelectionChangedCb = cb;  }

// Misc UI groups.

	// hot go's (close proximity to the focus go)
FEX bool AddToHotGroup        ( Goid goid, bool add );
FEX bool AddToHotGroup        ( Goid goid )							{  return ( AddToHotGroup( goid, true  ) );  }
FEX void ReplaceHotGroup      ( const GopColl& coll );
FEX bool RemoveFromHotGroup   ( Goid goid )							{  return ( AddToHotGroup( goid, false ) );  }
FEX void RemoveAllFromHotGroup( void );
FEX bool IsInHotGroup         ( Goid goid );

	// direct access (const only!)
	const GopColl& GetHotGroup( void ) const						{  return ( m_HotColl );  }

	// visible items
	GopSet& GetVisibleItems( void )									{  return ( m_VisibleItemsDb );  }

// Mouse shadowing.

	// mouse shadow
FEX bool IsMouseShadowed( Goid goid ) const;						// returns true if go is shadowed by mouse

	// direct access (const only!)
	const GopColl& GetMouseShadowedGos( void ) const				{  return ( m_MouseShadowColl );  }

	// selection access
	GopColl::const_iterator GetMouseShadowedGosBegin( void ) const	{  return ( m_MouseShadowColl.begin() );  }
	GopColl::const_iterator GetMouseShadowedGosEnd  ( void ) const	{  return ( m_MouseShadowColl.end  () );  }
	GopColl::iterator       GetMouseShadowedGosBegin( void )		{  return ( m_MouseShadowColl.begin() );  }
	GopColl::iterator       GetMouseShadowedGosEnd  ( void )		{  return ( m_MouseShadowColl.end  () );  }

// Frustum & focus.

	// frustum work
	void       SOnInitialFrustumRegistered( Player * player );
FEX FuBiCookie RCAssignFrustumToGo        ( DWORD machineId, FrustumId frustumId, Goid goid );	// assign an existing frustum to a go
FEX FuBiCookie SAssignFrustumToGo         ( FrustumId frustumId, Goid goid );
	void       AssignFrustumToGo          ( FrustumId frustumId, Goid goid );
FEX FuBiCookie SCreateFrustumForGo        ( Goid goid );										// create a frustum and assign the go to it
FEX FuBiCookie RCReleaseFrustumFromGo     ( DWORD machineId, Goid goid, bool eraseFrustum );	// release a frustum from the go, optionally erasing it
FEX FuBiCookie SReleaseFrustumFromGo      ( Goid goid, bool eraseFrustum = true );
	void       ReleaseFrustumFromGo       ( Goid goid, bool eraseFrustum );
	void	   RemoveFrustumFromPlayer	  ( PlayerId playerId );
    FrustumId  GetGoFrustum               ( Goid goid );
	FrustumId  GetActiveFrustumMask       ( void )					{  return ( m_ActiveFrustumMask );  }
	FrustumId  GetPlayerFrustumMask       ( PlayerId playerId );
	FrustumId  GetScreenPlayerFrustumMask ( void );
	bool       FillAddressesForFrustum    ( FrustumId goFrustum, FuBi::AddressColl& addresses ) const;

	// focus go
FEX FuBiCookie RSSetFocusGo  ( Goid goid );
FEX FuBiCookie RCSetFocusGo  ( DWORD machineId, Goid goid );
	void       SetFocusGo    ( Goid goid );
FEX FuBiCookie RSClearFocusGo( PlayerId playerId );
FEX FuBiCookie RCClearFocusGo( DWORD machineId, PlayerId playerId );
	void       ClearFocusGo  ( PlayerId playerId );
FEX Goid       GetFocusGo    ( PlayerId playerId ) const;
FEX Goid       GetFocusGo    ( void ) const;

// Observers and watchers.

	// $ note: only global go's may participate in these lists

	// go-to-go observation
FEX void StartWatching    ( Goid watcher, Goid watched );
FEX void StopWatching     ( Goid watcher, Goid watched );
FEX bool IsWatching       ( Goid watcher ) const;
FEX bool IsBeingWatched   ( Goid watched ) const;
	int  GetWatching      ( Goid watcher, GoidColl& coll ) const;
	bool GetWatchingIters ( Goid watcher, WatchDb::iterator& begin, WatchDb::iterator& end );
	int  GetWatchedBy     ( Go* watched, GoidColl& coll ) const;
	bool GetWatchedByIters( Go* watched, WatchDb::iterator& begin, WatchDb::iterator& end );

// Effects and enchantments.

	// siege fx
	void RegisterEffectScript      ( Goid goid, SFxSID scriptId, eWorldEvent type, bool xferOnSync, const Flamethrower_params* originalParams );
	void UnregisterEffectScript    ( Goid goid, SFxSID scriptId );
	int  StopEffectScripts         ( Go* go, eWorldEvent forType, eWorldEvent exceptType );
	int  StopEffectScripts         ( Go* go, eWorldEvent forType );
	int  StopEffectScriptsExcept   ( Go* go, eWorldEvent exceptType );
	int  StopEffectScripts         ( Go* go );
	void MessageEffectScripts      ( Go* go, const WorldMessage& msg );
	int  PauseEffectScripts        ( Go* go );
	int  ResumeEffectScripts       ( Go* go );
	bool GetEffectScriptIters      ( Go* go, EffectDb::const_iterator& begin, EffectDb::const_iterator& end );
	bool HasRegisteredEffectScripts( Go* go );

	// enchantments
	void  RegisterEnchantment         ( Goid goid, const Enchantment& enchantment );
FEX	void  RemoveEnchantments          ( Goid goid, Goid item, bool immediate = false );
	bool  GetEnchantment              ( Goid goid, const gpstring &sEnchantmentDescription, Enchantment **enchantment );
	float GetEnchantmentTotal		  ( Goid goid, const gpstring &sEnchantmentName );
FEX bool  HasEnchantment              ( Goid goid, const gpstring &sEnchantmentDescription );
FEX bool  HasEnchantments             ( Goid goid );
FEX bool  HasEnchantmentNamed         ( Goid goid, const gpstring &sEnchantmentName );
	bool  HasSingleInstanceEnchantment( Goid goid, const EnchantmentStorage& enchantments );
FEX float GetLongestAlteration        ( Goid goid, Goid target, Goid source, Goid item );
FEX void  SetAbortScriptID            ( Goid goid, const gpstring &sEnchantmentName, SFxSID id );
FEX void  SetEnchantmentDoneMessage   ( Goid goid, const gpstring &sEnchantmentName, Goid send_to, eWorldEvent event );
	bool  ApplyEnchantments           ( Go* go, bool forActor );

// Database query.

	// $ note: only global go's may have an instance scid

	// find a go by its scid instance
	Goid FindGoidByScid( Scid scid, bool* isRetired ) const;
FEX Goid FindGoidByScid( Scid scid ) const							{  return ( FindGoidByScid( scid, NULL ) );  }
FEX bool HasGoWithScid ( Scid scid ) const							{  return ( FindGoidByScid( scid ) != GOID_INVALID );  }

	// query traits on a go
	bool IsOnDeck( Goid goid ) const;
	bool IsMarkedForDeletion( Goid goid ) const;

	// query traits on an ex-go
	bool WasGoidRecentlyDeleted( Goid goid ) const;

	// exec a callback on each global go
	void ForEachGlobalGo( ForEachGoCb cb );
	void ForEachLocalGo ( ForEachGoCb cb, bool includeLodfi = false );

// Special.

	// $ these are special functions really only meant to be called from the
	//   Siege Editor.

	// $ get all active goid's - very slow, do not use except in special cases
	int GetAllGlobalGoids  ( GoidColl& coll ) const;
	int GetAllLocalGoids   ( GoidColl& coll, bool includeLodfi = false ) const;
	int GetAllCloneSrcGoids( GoidColl& coll ) const;

	// $ commit all pending operations (deletions, creations)
	bool CommitAllRequests( bool flushRpcs = true, bool bOnDeckOnly = false );

	// $ clear out our retired/scidbit/etc. scid set (generally only used for dev purposes)
	void ClearScidSpecifics( void );

	// $ special for call only by Go's
	void LockCloneSources( Go* go, bool lock );
#	if !GP_RETAIL
	void AddDirtyGoBucket  ( bool add = true );
	void AddServerOnlyCount( bool add = true );
#	endif // !GP_RETAIL

	// $ special for call only by Server
	void SSyncOnMachine ( DWORD machineId );
FEX	void RCSyncOnMachine( DWORD machineId, const_mem_ptr ptr );
	void XferForSync    ( FuBi::BitPacker& packer );

	// $ special for call only by Player
	void       ClientSyncAllClientsGos  ( Player* player );
FEX	FuBiCookie RCClientSyncAllClientsGos( DWORD machineId, const gpstring& spec );

// Debugging support.

	// access HUD infoz
#	if !GP_RETAIL
	HudInfo& GetHudInfo( Go* go )				{  return ( m_HudInfoDb[ go ] );  }
#	endif // !GP_RETAIL

	// special check to make sure we got what we asked for in MP
#	if !GP_RETAIL
	void RCVerifyGoBucket( const MachineAddrQuery& query, Goid startGoid, bool standalone );
FEX void RCVerifyGoBucket( DWORD verifyGoid, DWORD verifyCrc );
	void AppendBucketContents( Goid goid, gpstring& appendHere );
#	endif // !GP_RETAIL

private:
	struct CloneBucket;
	class GlobalIter;
	class GlobalConstIter;

	// utility
	Go** DerefGoPtr           ( Goid goid );						// access the Go* that actually stores the Go
	Go*  DerefGo              ( Goid goid );						// convert a Goid to a Go*
	void DeleteGo             ( Go* go, bool destructMsg = true,	// delete a Go from the various db's - note that 'failedGo' is if it failed to construct all the way
							    bool failedGo = false );
	void DeleteFailedGo       ( Goid startGoid );					// delete failed go (root) and all children
	void FreeGlobalGoid       ( Goid goid, Go* go = NULL );			// free a global goid (the go ptr is optional - used for verification only)
	void FreeCloneSrcGoid     ( Goid goid );						// free a clone source goid
	bool CommitOnDeckDb       ( void );								// commit any Go's in the on-deck set to their respective collections
	bool CommitDeleteRequests ( bool doCloneSourcesNow = false );	// commit Go deletion requests
	bool CommitPendingSiegeDb ( void );								// commit Go's awaiting entry into the world
	void CheckFocusChange     ( void );								// check for a change to focus based on new selection set
	void NewGoEnterWorld      ( Go* go );
	void ClientSyncGos        ( FrustumId oldMembership, FrustumId newMembership, Go* go, std::pair <SiegeGoDb::iterator, SiegeGoDb::iterator>* range );
	bool ClientSyncGo         ( Go* go, Player* player, bool enter );
	Goid CreateOrLoadGo       ( const GoCreateReq* createReq, const GoCloneReq* cloneReq, FastFuelHandle fuel = FastFuelHandle() );
	Goid CreateCloneSrcGo     ( Goid goid, CloneBucket* bucket, const char* templateName );
	void FinishGoCreation     ( Goid goid );
	void AddToTempOnDeckDb    ( Go* go, Goid goid );
	void AddToOnDeckDb        ( Go* go, Goid goid );
	void AddRecentDeletion    ( Goid goid );
	void RebuildCreationCaches( void );
	void EraseEffectEntry     ( EffectDb::iterator& i );
	void EraseLinkedEffects   ( Go* target, Go* owner );

	// siege callback
	void CollectObjects( siege::WorldObjectColl& objects, const siege::SiegeNode& node, siege::eCollectFlags cflags, siege::eRequestFlags rflags ) const;
	void OnSpaceChanged( siege::database_guid guid, bool bIsElevator );

	// iterator access
	GlobalConstIter GlobalBegin( void ) const;
	GlobalConstIter GlobalEnd  ( void ) const;
	GlobalIter      GlobalBegin( void );
	GlobalIter      GlobalEnd  ( void );

// Types.

	// helpers
	typedef kerneltool::Critical Critical;
	typedef kerneltool::RwCritical RwCritical;

	// an element of the global database
	struct GoBucket
	{
		// $ note that GoBucket does not handle destruction of its Go's, the
		//   GoDb does it instead.

		Go*     m_First;
		GopColl m_Remaining;
		int     m_Count;

		GoBucket( void )
		{
			m_First = (Go*)0xFAABD00D;	// $ special "root" node that will stop GlobalConstIter::Next() when it loops to the end
			m_Count = 0;
		}

#		if !GP_RETAIL
		UINT32 AddCRC32( UINT32 seed = 0 ) const;
		UINT32 AddCRC32FromHandles( Goid baseGoid, UINT32 seed = 0 ) const;
#		endif // !GP_RETAIL
	};

	// an element of the clone source db
	struct CloneBucket
	{
		// $ note that CloneBucket does not handle destruction of its Go, the
		//   GoDb does it instead.

		Go*      m_Go;					// the Go (may be null if expired)
		gpstring m_TemplateName;		// the name of the template for this go
		int      m_LockCount;			// ref count of locks made on this clone source to keep it from unloading

		CloneBucket( void )
		{
			m_Go = NULL;
			m_LockCount = 0;
		}
	};

	// helper class that controls assignment of go's
	class GoAdder
	{
	public:
		struct LockBase
		{
			LockBase( void );

			kerneltool::Critical::Lock m_Lock;
		};

		struct ServerLock : LockBase
		{
			ServerLock( void );
		   ~ServerLock( void );
		};

		struct ClientLock : LockBase
		{
			ClientLock( Goid initialGoid );
		   ~ClientLock( void );
		};

		struct Lock : LockBase
		{
			Lock( DWORD goidClass );
		   ~Lock( void );
		};

		GoAdder( void );
	   ~GoAdder( void )							{  gpassert( !IsActive() );  }

		bool IsActive( void ) const				{  return ( m_RefCount > 0 );  }

		void EnterServer( void );
		void EnterClient( Goid initialGoid );
		void Enter      ( DWORD goidClass );
		void Leave      ( void );

		Goid GetCurrentGoid( void ) const		{  return ( m_Goid );  }
		Goid AssignNextGoid( void );

		DWORD GetOwnerThreadId( void ) const	{  return ( m_OwnerThreadId );  }

	private:
		void AddRef( void )
		{
			if ( m_RefCount == 0 )
			{
				m_OwnerThreadId = ::GetCurrentThreadId();
			}
			else
			{
				gpassert( m_OwnerThreadId == ::GetCurrentThreadId() );
			}

			gpassert( m_RefCount >= 0 );
			++m_RefCount;
		}

		// state
		Goid   m_Goid;
		int    m_RefCount;
		bool   m_ClientInitial;
		DWORD  m_OwnerThreadId;

		SET_NO_COPYING( GoAdder );
	};

	struct GoFrustum
	{
		Go*       m_Go;			// the go
		FrustumId m_FrustumId;	// frustum that it owns

		GoFrustum( Go* go = NULL, FrustumId frustumId = FRUSTUMID_INVALID )
			: m_Go( go ), m_FrustumId( frustumId )  {  }

		bool Xfer( FuBi::PersistContext& persist );
	};

	struct PlayerFrustum
	{
		typedef std::list <GoFrustum> Coll;

		Coll::iterator m_Focus;						// focus go - may be null
		Coll           m_Coll;						// simple collection of go's and the frusta they own
		FrustumId      m_OwnedMask;					// cache mask of owned frusta

		PlayerFrustum( void );
		PlayerFrustum( const PlayerFrustum& other );

		bool Xfer( FuBi::PersistContext& persist );
	};

	struct ExpireEntry
	{
		Go*  m_Go;					// go that is expiring
		bool m_Forced;				// if forced, then (a) reentering the world will not cancel the expiration and (b) the scid will retire

		bool Xfer( FuBi::PersistContext& persist );
	};

	struct ScidBits
	{
		DWORD m_Bits;
		bool  m_MustRpc;

		ScidBits( DWORD bits = 0, bool mustRpc = false )
		{
			m_Bits    = bits;
			m_MustRpc = mustRpc;
		}

		bool Xfer( FuBi::PersistContext& persist );
	};

	// friends
	friend GoHandle;				// $ for goid deref'ing
	friend GoAdder;
	friend GoAdder::LockBase;
	friend GoAdder::ServerLock;
	friend GoAdder::ClientLock;
	friend GoAdder::Lock;
	friend GoDbShouldPersistHelper;

	// collections
	typedef stdx::fast_vector <PlayerFrustum>                    PlayerFrustumColl;		// collection of player frusta, playerid's bit position is index
	typedef std::map          <Go*, my EnchantmentStorage*>      EnchantmentDb;			// go -> enchantment acting on it
	typedef BucketVector      <GoBucket>                         GlobalDb;
	typedef BucketVector      <my Go*>                           LocalDb;
	typedef BucketVector      <CloneBucket>                      CloneSrcDb;
	typedef std::map          <gpstring, Goid, istring_less>     CloneNameDb;			// name -> clone source goid
	typedef std::map          <Goid, my Go*>                     OnDeckDb;				// goid -> go ready to enter into the game (goid is what it will take when active)
	typedef std::multimap     <Go*, Goid>                        GoGoidDb;				// go* -> goid for mapping purposes
	typedef std::multimap     <Go*, Go*>                         GoGoDb;				// go* -> go* for mapping purposes
	typedef std::list         <std::pair <double, Goid> >        TimeGoidColl;			// time deleted, goid
	typedef std::map          <Goid, TimeGoidColl::iterator>     TimeGoidReverseDb;		// goid -> time deleted/goid iter in TimeGoidColl (reverse index)
	typedef std::map          <Scid, Go*>                        ScidIndex;				// scid -> go
	typedef std::set          <Scid>                             ScidColl;
	typedef std::map          <Scid, ScidBits>                   ScidBitsColl;
	typedef std::map          <gpstring, gpstring, istring_less> StringDb;
	typedef std::map          <gpstring, StringDb, istring_less> StringStringDb;
	typedef std::map          <Goid, StringStringDb>             QuestBitsDb;			// goid -> map -> key -> value
	typedef std::multimap     <double, ExpireEntry>              ExpireDb;				// absolute expiration time -> info about expiring go
	typedef std::map          <Go*, ExpireDb::iterator>          ExpireReverseDb;		// go -> expiration iter in ExpireDb (reverse index)
	typedef std::map          <siege::database_guid, GopColl>    SiegeGopCollDb;		// siege node -> gop coll
	typedef stdx::fast_vector <CreateGoXfer>                     CreateGoXferColl;
	typedef stdx::fast_vector <CloneGoXfer>                      CloneGoXferColl;

// Iterator types.

	// special iterators for global db - note that if performance is more
	// important than programming convenience, don't use these classes. hand-
	// code the loops instead.

	// GlobalConstIter class
	class GlobalConstIter : public std::_Bidit <Go, ptrdiff_t>
	{
	public:
		typedef GlobalDb::iterator BaseIter;

		GlobalConstIter( void )									{  }
		GlobalConstIter( const BaseIter& iter, int index = 0 )	: m_BaseIter( iter ), m_SubIndex( index )  {  Init();  }
		GlobalConstIter( const GlobalIter& obj )				: m_BaseIter( obj.m_BaseIter ), m_SubIndex( obj.m_SubIndex )  {  }

		Go*       Get     ( void )								{  Go* go = GetObject();  gpassert( go != NULL );  return ( go );  }
		const Go* GetConst( void ) const						{  Go* go = GetObject();  gpassert( go != NULL );  return ( go );  }

		const Go& operator *  ( void ) const					{  return ( *GetConst() );  }
		const Go* operator -> ( void ) const					{  return (  GetConst() );  }

		GlobalConstIter& operator ++ ( void )					{  Next();  return ( *this );  }
		GlobalConstIter  operator ++ ( int  )					{  GlobalConstIter tmp = *this;  Next();  return ( tmp );  }

		GlobalConstIter& operator -- ( void )					{  Prev();  return ( *this );  }
		GlobalConstIter  operator -- ( int  )					{  GlobalConstIter tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const GlobalConstIter& obj ) const	{  return ( (m_BaseIter == obj.m_BaseIter) && (m_SubIndex == obj.m_SubIndex) );  }
		bool operator != ( const GlobalConstIter& obj ) const	{  return ( !(*this == obj) );  }

	protected:
		Go* GetObject( void ) const								{  return ( (m_SubIndex == 0) ? m_BaseIter->m_First : m_BaseIter->m_Remaining[ m_SubIndex - 1 ] );  }

		void Init( void )
		{
			if ( GetObject() == NULL )
			{
				Next();
			}
		}

		void Next( void )
		{
			do
			{
				if ( ++m_SubIndex == ((int)m_BaseIter->m_Remaining.size() + 1) )
				{
					++m_BaseIter;
					m_SubIndex = 0;
				}
			}
			while ( GetObject() == NULL );
		}

		void Prev( void )
		{
			do
			{
				if ( m_SubIndex-- == 0 )
				{
					--m_BaseIter;
					m_SubIndex = m_BaseIter->m_Remaining.size();
				}
			}
			while ( GetObject() == NULL );
		}

		BaseIter m_BaseIter;
		int      m_SubIndex;
	};

	// GlobalIter class
	class GlobalIter : public GlobalConstIter
	{
	public:
		GlobalIter( void )										{  }
		GlobalIter( const BaseIter& iter, int index = 0 )		: GlobalConstIter( iter, index )  {  }

		Go& operator *  ( void ) const							{  return ( *GetObject() );  }
		Go* operator -> ( void ) const							{  return (  GetObject() );  }

		GlobalIter& operator ++ ( void )						{  Next();  return ( *this );  }
		GlobalIter  operator ++ ( int  )						{  GlobalIter tmp = *this;  Next();  return ( tmp );  }

		GlobalIter& operator -- ( void )						{  Prev();  return ( *this );  }
		GlobalIter  operator -- ( int  )						{  GlobalIter tmp = *this;  Prev();  return ( tmp );  }

		bool operator == ( const GlobalIter& obj ) const		{  return ( (m_BaseIter == obj.m_BaseIter) && (m_SubIndex == obj.m_SubIndex) );  }
		bool operator != ( const GlobalIter& obj ) const		{  return ( !(*this == obj) );  }
	};

// Data.

	// $ note: data marked *SYNC* must be synchronized for multithreaded
	//   access. most of the time this is done via critical locks but once per
	//   frame it may be done implicitly via a siege checkpoint callback. this
	//   is typically where deferred deletion requests are committed. data not
	//   marked as *SYNC* is only altered/queried from the primary thread.

	// observer linkage
	GopColl           m_SelectionColl;			// all selected go's - note that order matters so this is a vector
	GopColl           m_HotColl;				// hot go's - go's that are close to the party
	GopSet            m_VisibleItemsDb;			// currently visible items on ground
	GopColl           m_MouseShadowColl;		// go's currently shadowed by the mouse
	PlayerFrustumColl m_PlayerFrustumColl;		// vector of all the focus go's, where each player is one slot
	FrustumId         m_ActiveFrustumMask;		// bitfield marking active frusta
	WatchDb           m_WatchingDb;				// first is watching second
	WatchDb           m_WatchedDb;				// first is being watched by second (reverse index)
	EffectDb          m_EffectDb;				// all currently active SiegeFx
	EnchantmentDb     m_EnchantmentDb;			// which enchantments are currently in effect for all gos
	GoidColl		  m_EnchantmentRemoveQueue;	// queue for delayed removal of enchantments since an alteration can cause self deletion
	RwCritical        m_EnchantmentCritical;	// critical locker for enchantment related registration

	// go databases
	RwCritical  m_DbCritical;					// critical locker for any db (global, local, clonesrc) access
	GlobalDb    m_GlobalDb;				/*SYNC*/// db of all global go's
	int         m_GlobalDbCount;				// count of go's in global db (can't use size() on globaldb - that will only count buckets)
	LocalDb     m_LocalDb;				/*SYNC*/// db of local go's
	CloneSrcDb  m_CloneSrcDb;			/*SYNC*/// db of clone source go's
	int         m_CloneSrcUnloadedCount;/*SYNC*/// count of unloaded clone sources in global db
	GoGoidDb    m_CloneSrcLockDb;		/*SYNC*/// db of go's that have locked clone sources into memory
	RwCritical  m_CloneSrcLockCritical;	/*SYNC*/// locker for accessing m_CloneSrcLockDb
	CloneNameDb m_CloneNameDb;			/*SYNC*/// index of clone objects by name
	RwCritical  m_CloneNameCritical;	/*SYNC*/// critical serializer for the clone name db

	// world data
	SiegeGoDb       m_SiegeGoDb;				// db of go occupants in siege nodes (which may or may not be in the active world frustum)
	SiegeGopCollDb  m_PendingGoDb;				// db of freshly created go's awaiting occupancy in siege nodes
	ScidColl        m_RetiredScidColl;	/*SYNC*/// the set of all retired scids in the map
	ScidBitsColl    m_ScidBitsColl;		/*SYNC*/// data attached to scids for the map
	QuestBitsDb     m_QuestBitsDb;				// party-specific data (stored per character)
	RwCritical      m_ScidBitsCritical;			// critical to lock scid bits manipulation
	ExpireDb        m_ExpireDb;					// go's pending expiration mapped by absolute expiration time
	ExpireReverseDb m_ExpireReverseDb;			// reverse lookup for speed

	// go construction
	OnDeckDb   m_OnDeckDb;				/*SYNC*/// go's "on deck" - in limbo awaiting addition to primary db
	OnDeckDb   m_TempOnDeckDb;			/*SYNC*/// special additional go's for lookup while committing creation
	RwCritical m_OnDeckCritical;		/*SYNC*/// rwcritical just for serializing access to the on-deck db
	Goid       m_MostRecentNewGoidMain;			// most recently top-level created/cloned Go (main thread)
	Goid       m_MostRecentNewGoidLoader;		// most recently top-level created/cloned Go (loader thread)
	DWORD      m_MostRecentFixedSeedMain;		// random seed used to create most recent Go (main thread)
	DWORD      m_MostRecentFixedSeedLoader;		// random seed used to create most recent Go (loader thread)
	bool       m_ConstructingStandaloneGo;		// set if CREATE_STANDALONE style go under construction
	bool       m_ImportingStandaloneGo;			// set if importing something from fuel
	GoAdder    m_Adder;					/*SYNC*/// adder state info
	Critical   m_CreateLock;			/*SYNC*/// control access to go creation and Goid assignment
	GopColl    m_GoCreateColl;			/*SYNC*/// go's currently under construction (all must be committed at once)

	// misc
	bool               m_IsShuttingDown;		// the godb is currently shutting down at all
	bool               m_IsShuttingDownBad;		// the godb is currently shutting down (badly, uncleanly)
	my Go*             m_GoUpdateRoot;			// root of the go linked list update ring
	GopList            m_DeleteReqColl;			// deletion requests go here
	GopList            m_DeleteReqDelayedColl;	// delayed client-side mp deletion requests go here
	TimeGoidColl       m_RecentDeletionColl;	// deleted goids go here
	TimeGoidReverseDb  m_RecentDeletionRevDb;	// reverse lookup for speed
	ScidIndex          m_ScidIndex;		/*SYNC*/// for fast lookup of go's
	RwCritical         m_ScidCritical;	/*SYNC*/// rwcritical just for serializing access to the scid index
	SelectionChangedCb m_SelectionChangedCb;	// callback to invoke when selection set changes at all
	DWORD              m_PrimaryThreadId;		// what's the primary thread id?
	float              m_CloneSrcLruTimeout;	// this is the timeout to be used before lru-unloading clone sources
	float              m_CloneSrcLruCheck;		// seconds remaining until next lru check
	float              m_CloneSrcCheckTimeout;	// this is the timeout to be used before checking for lru-unloading clone sources
	float              m_DeletionRetireTime;	// time elapsed before retiring a recent deletion
	bool               m_LockedForSaveLoad;		// special: godb is locked for save/load - no new requests allowed
	bool			   m_PostLoading;			// special flag when we are in the PostLoading function -- this is so we know we can commit creationss

	// multiplayer-related data (no need to xfer for SP save game)
	GoGoDb             m_EffectLinkDb;			// db of links from source/target of an effect back to its owner if omni (for spells and xfer)
	CreateGoXferColl   m_CreateGoXferCache;		// cache of most recently used requests (for rpc's only)
	CloneGoXferColl    m_CloneGoXferCache;		// cache of most recently used requests (for rpc's only)

#	if !GP_RETAIL
	mutable int m_LastUpdateCount;				// number of go's updated last update call
	mutable int m_LastRenderCount;				// number of go's rendered last update call
	mutable int m_LastViewCount;				// number of go's in the view frustum last update call
	bool        m_EditMode;						// special: edit mode
	bool        m_OverrideDetectOnly;			// special: only detect overrides by differences, don't allow forcing
	Goid        m_DebugUpdateBreakGoid;			// break on this goid's update
	int         m_GlobalDbDirtyCount;			// count of dirty go's in global db
	int         m_GlobalDbServerOnlyCount;		// count of server-only go's in global db
	HudInfoDb   m_HudInfoDb;					// special db for tracking HUD infoz
#	endif // !GP_RETAIL

	// $$$ when implement unloading of clone sources, be sure to keep those
	//     Goid's allocated but keep a ref to them in the clone name db so we
	//     know what they were named. things like charred goid and ammo goid
	//     will depend on those clone src goid's never going away, so just
	//     delete and null out the clone source slot but don't dealloc it. on a
	//     deref, if get null, fill it in using the "retired clone src" db of
	//     goid-to-name.

	// $$$ may need to add statistics to see how sparse the buckets are to calc
	//     memory efficiency. also add plain stats to count #go's of each type,
	//     number deletions and allocs so far, total memory used by collections,
	//     etc...

	FUBI_SINGLETON_CLASS( GoDb, "Database of all runtime game objects (Go's)." );
	SET_NO_COPYING( GoDb );
};

#define gGoDb GoDb::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __GODB_H

//////////////////////////////////////////////////////////////////////////////
