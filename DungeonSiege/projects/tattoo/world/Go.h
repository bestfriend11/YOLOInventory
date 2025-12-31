//////////////////////////////////////////////////////////////////////////////
//
// File     :  Go.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the primary Game Object classes.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GO_H
#define __GO_H

//////////////////////////////////////////////////////////////////////////////

#include "Flamethrower_types.h"
#include "Fuel.h"
#include "GpColl.h"
#include "Gps_types.h"
#include "GoBase.h"
#include "Rules.h"
#include "SkritDefs.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace stdx
{
	struct delete_by_ptr;
	struct delete_second_by_ptr;
}

class GoRpcMgr;

//////////////////////////////////////////////////////////////////////////////
// class GoComponent declaration

class GoComponent
{
public:
	SET_NO_INHERITED( GoComponent );

// Setup.

	// ctor/dtor
	GoComponent( Go* go );
	GoComponent( const GoComponent& source, Go* newGo );
	virtual ~GoComponent( void );

	// setup
	void Init( GoDataComponent* data );				// owned
	void Init( const GoDataComponent* data );		// referenced

	// special
#	if !GP_RETAIL
	void ForceOwned( void );
#	endif // !GP_RETAIL

// Attributes.

	// normal query
FEX const gpstring&        GetName( void ) const;
	Go*                    GetGo  ( void ) const				{  gpassert( m_Go != NULL );  return ( m_Go );  }
FEX Goid                   GetGoid( void ) const;
FEX Scid                   GetScid( void ) const;
	const GoDataComponent* GetData( void ) const				{  gpassert( m_DataComponent != NULL );  return ( m_DataComponent );  }

	// special fubi exports that don't give win98 export problems
FEX Go*                    FUBI_RENAME( GetGo   )( void ) const	{  return ( GetGo() );  }
FEX const GoDataComponent* FUBI_RENAME( GetData )( void ) const	{  return ( GetData() );  }

	// special
#	if !GP_RETAIL
	GoDataComponent* GetOwnedData( void ) const					{  gpassert( m_OwnedDataComponent != NULL );  return ( m_OwnedDataComponent );  }
#	endif // !GP_RETAIL

	// wants bits
	void SetWantsUpdates( bool set = true );
	void SetWantsDraws  ( bool set = true );
	bool GetWantsUpdates( void ) const							{  return ( m_WantsUpdates );  }
	bool GetWantsDraws  ( void ) const							{  return ( m_WantsDraws   );  }
	bool XferWantsBits  ( FuBi::PersistContext& persist );

// Abstract interface.

	// $ important - DO NOT call the inherited versions of these functions!

	// mostly required interfaces
	virtual int          GetCacheIndex( void ) = 0;
	virtual GoComponent* Clone        ( Go* newGo ) = 0;
	virtual bool         Xfer         ( FuBi::PersistContext& persist ) = 0;
	virtual bool         XferCharacter( FuBi::PersistContext& /*persist*/ )		{  return ( true );  }

	// event handlers
	virtual bool CommitCreation    ( void )							{  return ( true );  }
	virtual void CommitClone       ( const GoComponent* /*src*/ )	{  }
	virtual bool CommitLoad        ( void )							{  return ( true );  }
	virtual bool CommitImport      ( bool /*fullXfer*/ )			{  return ( true );  }
	virtual void Shutdown          ( void )							{  }
	virtual void Preload           ( void )							{  }
	virtual void HandleMessage     ( const WorldMessage& /*msg*/ )	{  }
	virtual void HandleCcMessage   ( const WorldMessage& /*msg*/ )	{  }
	virtual void Update            ( float /*deltaTime*/ )			{  ms_Overridden = false;  }
	virtual void UpdateSpecial     ( float /*deltaTime*/ )			{  }
	virtual void Draw              ( void )							{  ms_Overridden = false;  }
	virtual void ResetModifiers    ( void )							{  }
	virtual void RecalcModifiers   ( void )							{  }
	virtual void LinkParent        ( Go* /*parent*/ )				{  }
	virtual void LinkChild         ( Go* /*child*/ )				{  }
	virtual void UnlinkParent      ( Go* /*parent*/ )				{  }
	virtual void UnlinkChild       ( Go* /*child*/ )				{  }
	virtual void DetectWantsBits   ( void );
	virtual bool CanServerExistOnly( void );

#	if !GP_RETAIL
	virtual bool ShouldSaveInternals( void )													{  return ( false );  }
	virtual void SaveInternals      ( FuelHandle /*fuel*/ )										{  }
	virtual void DrawDebugHud       ( void )													{  }
	virtual void Dump               ( bool /*verbose*/, ReportSys::ContextRef /*ctx*/ = NULL )	{  }
#	endif // !GP_RETAIL

#	if !GP_RETAIL
	const BitVector* GetOverrideColl( void ) const;
#	endif // !GP_RETAIL

	GPDEBUG_ONLY( virtual bool AssertValid( void ) const; )

private:

	// spec
	Go*                    m_Go;					// who owns me?
	const GoDataComponent* m_DataComponent;			// points to template's component if not specialized in instance
	GoDataComponent*       m_OwnedDataComponent;	// specialized component

	// bool state
	bool m_WantsUpdates : 1;						// this go wants updates - make sure it is part of the update list when it is in an active world frustum
	bool m_WantsDraws   : 1;						// this go wants to be able to draw

	DECL_THREAD static bool ms_Overridden;

	SET_NO_COPYING( GoComponent );
};

//////////////////////////////////////////////////////////////////////////////
// class GoSkritComponent declaration

class GoSkritComponent : public GoComponent
{
public:
	SET_INHERITED( GoSkritComponent, GoComponent );

// Setup.

	// ctor/dtor
	GoSkritComponent( Go* go );
	GoSkritComponent( const GoSkritComponent& source, Go* newGo );
	virtual ~GoSkritComponent( void );

// Interface.

	const FuBi::Record* GetSkritRecord( void ) const;
	FuBi::Record*       GetSkritRecord( void );

	static FuBi::eVarType GetFuBiType( void );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// special version that knows if it's supposed to persist
	bool SpecialXfer( FuBi::PersistContext& persist, bool shouldPersist );

	// event handlers
	virtual bool CommitCreation ( void );
	virtual bool CommitImport   ( bool fullXfer );
	virtual void Shutdown       ( void );
	virtual void Preload        ( void );
	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void HandleCcMessage( const WorldMessage& msg );
	virtual void Update         ( float deltaTime );
	virtual void UpdateSpecial  ( float deltaTime );
	virtual void Draw           ( void );
	virtual void ResetModifiers ( void );
	virtual void RecalcModifiers( void );
	virtual void LinkParent     ( Go* parent );
	virtual void LinkChild      ( Go* child );
	virtual void UnlinkParent   ( Go* parent );
	virtual void UnlinkChild    ( Go* child );
	virtual void DetectWantsBits( void );

#	if !GP_RETAIL
	virtual void DrawDebugHud( void );
	virtual void Dump        ( bool verbose, ReportSys::ContextRef ctx = NULL );
#	endif // !GP_RETAIL

	GPDEBUG_ONLY( virtual bool AssertValid( void ) const; )

private:
	void Clone( Skrit::HObject cloneSource, bool shouldPersist = true );

	Skrit::HAutoObject m_Skrit;
	Skrit::Object*     m_SkritPtr;

	static FuBi::eVarType ms_FuBiType;

	FUBI_CLASS_INHERIT( ThisType, Inherited );
	SET_NO_COPYING( GoSkritComponent );
};

//////////////////////////////////////////////////////////////////////////////
// class Go declaration

// $ a Go is a component-based game object. it shall have no derivatives.

// note that we are tracking the random seed used to generate the go. this is
// used to synchronize multiple systems and guarantee that they all have the
// same randomly generated content. we're keeping the seed around in the go
// after creation for later replication when frustum memberships change and a
// load must be executed on clients.

class Go
{
public:
	SET_NO_INHERITED( Go );

// Types.

	// collections
	typedef stdx::fast_vector <my GoComponent*> ComponentColl;

	// type used to determine what's included in a sync packet
	enum eSyncType
	{
		SET_BEGIN_ENUM( SYNC_, 0 ),

		SYNC_MINIMAL,
		SYNC_FULL,
		SYNC_FOR_ALL_CLIENTS,

		SET_END_ENUM( SYNC_ ),

		SYNC_DEFAULT,
	};

// Attributes.

	// trait query
FEX Goid     GetGoid       ( void ) const					{  gpassert( IsValid( m_Goid, false ) );  return ( m_Goid );  }
FEX Scid     GetScid       ( void ) const					{  gpassert( IsValid( m_Scid, false ) );  return ( m_Scid );  }
FEX Go*      GetParent     ( void ) const					{  return ( m_Parent );  }
	bool     GetParent     ( Go*& parent ) const			{  parent = m_Parent;  return ( parent != NULL );  }
FEX Go*      GetRoot       ( void );
	int      GetRootDepth  ( void ) const;
FEX Go*      GetOwningParty( void ) const;
	bool     IsInSameParty ( const Go* other ) const;
	bool     IsRelativeOf  ( const Go* other ) const;
FEX Player*  GetPlayer     ( void ) const					{  return ( m_Player );  }
FEX PlayerId GetPlayerId   ( void ) const;
FEX DWORD    GetMachineId  ( void ) const;
FEX	void     SSetPlayer    ( PlayerId id );
FEX void	 RCSetPlayer   ( PlayerId id );
	void     SetPlayer     ( Player* player );

	// class query
FEX	bool IsGlobalGo     ( void ) const						{  return ( GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL    );  }
FEX	bool IsLocalGo      ( void ) const						{  return ( GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL     );  }
	bool IsCloneSourceGo( void ) const						{  return ( GetGoidClass( GetGoid() ) == GO_CLASS_CLONE_SRC );  }
	bool IsRpcCapableGo ( void ) const						{  return ( IsGlobalGo() && !IsMarkedForDeletion() && !IsServerOnly() );  }

	// template query
	const GoDataTemplate* GetDataTemplate( void ) const		{  return ( m_DataTemplate );  }

	// other query
FEX const char* GetTemplateName( void ) const;
#	if !GP_RETAIL
	gpstring MakeTeleportLocation( void ) const;
	double   GetCreateTime       ( void ) const				{  return ( m_CreateTime );  }
	UINT32   AddCRC32            ( UINT32 seed = 0 ) const;
#	endif // !GP_RETAIL

	// special
	DWORD    GetRandomSeed      ( void ) const				{  return ( m_RandomSeed );  }
	int      GetMpPlayerCount   ( void ) const;
	Go*      GetCloneSource     ( void );
	Goid     GetCloneSourceGoid ( void ) const				{  return ( m_CloneSource );  }
	int      GetCloneSourceDepth( void );
	RegionId GetRegionSource    ( void ) const				{  return ( m_RegionSource );  }

	// state query
	bool      HasValidGoid                ( void ) const	{  return ( m_Goid != GOID_INVALID );  }
	bool      HasParent                   ( void ) const	{  return ( m_Parent != NULL );  }
	bool      IsUpdating                  ( void ) const	{  return ( (m_NextUpdate != NULL) && !IsPendingUpdateRemoval() );  }
	bool      IsPendingUpdateRemoval      ( void ) const	{  return ( m_PendingUpdateRemoval );  }
FEX	bool      IsInAnyWorldFrustum         ( void ) const	{  return ( m_FrustumMembership != 0 );  }
FEX	bool      IsInAnyScreenWorldFrustum   ( void ) const;
FEX	bool      IsInActiveWorldFrustum      ( void ) const;
FEX	bool      IsInActiveScreenWorldFrustum( void ) const;
FEX	FrustumId GetWorldFrustumMembership   ( void ) const	{  return ( m_FrustumMembership );  }
FEX	FrustumId CalcWorldFrustumMembership  ( bool useNode = true ) const;
FEX	bool      IsOmni                      ( void ) const	{  return ( m_IsOmni );  }
	bool      IsAllClients                ( void ) const	{  return ( m_IsAllClients );  }
	bool      CalcAllClients              ( void ) const	{  return ( !IsServerOnly() && IsGlobalGo() && (IsAllClients() || IsOmni()) );  }
	bool      IsPContentInventory         ( void ) const	{  return ( m_IsPContentInventory );  }
	bool      IsPContentGenerated         ( void ) const	{  return ( m_IsPContentGenerated );  }
	bool      IsLodfi                     ( void ) const	{  gpassert( !m_IsLodfi || IsLocalGo() );  return ( m_IsLodfi );  }
	bool      ShouldPersist               ( void ) const	{  return ( !IsLodfi() && !IsCloneSourceGo() );  }
	bool      IsServerOnly                ( void ) const	{  return ( m_IsServerOnly );  }
	bool      HasFrustum                  ( void ) const	{  return ( m_HasFrustum );  }
	Go*       GetFrustumOwnerInChain      ( bool inheritedPlacementOnly = true );
	bool      IsInFrustumOwnerChain       ( void )          {  return ( GetFrustumOwnerInChain() != NULL );  }
	bool      IsMarkedForDeletion         ( void ) const	{  return ( m_MarkedForDeletion );  }
	bool      IsDelayedMpDeletion         ( void ) const	{  return ( m_DelayedMpDeletion );  }
	bool      IsExpiring                  ( void ) const	{  return ( m_IsExpiring );  }
	bool      IsCommittingCreation        ( void ) const;
	bool      IsPendingEntry              ( void ) const	{  return ( m_IsPendingEntry );  }
	bool      IsSpaceDirty                ( void ) const	{  return ( m_IsSpaceDirty );  }
	bool      IsWobbDirty                 ( void ) const	{  return ( m_IsWobbDirty );  }
	bool      IsRenderDirty               ( void ) const	{  return ( m_IsRenderDirty );  }
	bool      IsBucketDirty               ( void ) const	{  return ( m_IsBucketDirty );  }
	bool      CalcBucketDirty             ( void ) const;
	bool      IsFading                    ( void ) const	{  return ( m_IsFading );  }
	bool      IsHero                      ( void ) const;
	bool      CheckFadingFirstTime        ( void )			{  bool old = m_IsFadingFirstView;  m_IsFadingFirstView = false;  return ( old );  }

	// ui state query
FEX bool IsFocused      ( void ) const						{  return ( m_IsFocused       ); }
FEX bool IsSelected     ( void ) const  					{  return ( m_IsSelected      ); }
FEX bool IsMouseShadowed( void ) const						{  return ( m_IsMouseShadowed ); }
FEX bool IsInHotGroup   ( void ) const						{  return ( m_IsInHotGroup    ); }

	// selection
FEX bool Select        ( bool select );
FEX bool Select        ( void )								{  return ( Select( true ) );  }
FEX bool Deselect      ( void )								{  return ( Select( false ) );  }
FEX bool ToggleSelected( void )								{  return ( Select( !IsSelected() ) );  }

	// visiting - temporary usage only! don't assume visit bit's value!
	bool IsVisited ( void ) const							{  CHECK_PRIMARY_THREAD_ONLY;  return ( m_Visited );  }
	bool SetVisit  ( bool set = true )						{  CHECK_PRIMARY_THREAD_ONLY;  bool old = m_Visited;  m_Visited = set;  return ( old );  }
	bool ClearVisit( void )									{  return ( SetVisit( false ) );  }

	// visit all children
	void ClearAllChildrenVisit( void );

	// transferring - special use only
	bool IsTransferring   ( void ) const;
	bool SetTransferring  ( bool set = true );
	bool ClearTransferring( void );

	// placement changing - special use only
#	if !GP_RETAIL
	bool IsPlacementChanging   ( void ) const;
	void SetPlacementChanging  ( bool set = true );
	void ClearPlacementChanging( void );
#	endif // !GP_RETAIL

	// fading
	void StartFading( void )								{  m_IsFading = true;  m_IsFadingFirstView = true;  }
	void StopFading ( void )								{  m_IsFading = false;  }

	// wants bits
	void UpdateWantsUpdates( bool newValue );
	void UpdateWantsDraws  ( bool newValue );
	void UpdateWantsBits   ( void );

	// rendering
FEX	void PrepareToDrawNow        ( bool includeTextures );
FEX	void PrepareToDrawNow        ( void )					{  PrepareToDrawNow( true );  }
FEX bool IsInViewFrustum         ( void ) const;
FEX bool WasInViewFrustumRecently( void ) const;

#	if !GP_RETAIL

	// dev query
FEX bool IsProbed( void ) const;
FEX bool IsHuded ( void ) const;

	// fetch counts
	int GetErrorCount  ( void )								{  return ( m_DevErrorCount );  }
	int GetWarningCount( void )								{  return ( m_DevWarningCount );  }

	// inc/clear counts
	void IncrementErrorCount  ( int count )					{  m_DevErrorCount += count;  }
	void IncrementWarningCount( int count )					{  m_DevWarningCount += count;  }
	void ClearErrorCount      ( void )						{  m_DevErrorCount = 0;  }
	void ClearWarningCount    ( void )						{  m_DevWarningCount = 0;  }

	// get our instance fuel address
	const gpstring& DevGetFuelAddress( void ) const			{  return ( m_DevInstanceFuel );  }

#	else // !GP_RETAIL

FEX bool IsProbed( void ) const								{  return ( false );  }
FEX bool IsHuded ( void ) const								{  return ( false );  }

#	endif // !GP_RETAIL

// Component query helpers.

	// basic
FEX eLifeState  GetLifeState           ( void ) const;
FEX const char* GetMaterial            ( void ) const;
FEX Go*         GetBestArmor           ( void );
FEX bool        IsActor                ( void ) const;
FEX bool		IsAmmo				   ( void ) const;
FEX bool        IsAnyHumanParty        ( void ) const;
FEX bool        IsAnyHumanPartyMember  ( void ) const;
FEX bool        IsArmor                ( void ) const;
FEX bool        IsBreakable            ( void ) const;
FEX bool        IsChad                 ( void ) const;
FEX bool        IsCommand              ( void ) const;
FEX bool        IsContainer            ( void ) const;
FEX bool        IsEquipped             ( void ) const;
FEX bool        IsActiveSpell          ( void ) const;
FEX bool        IsFireImmune           ( void ) const;
FEX bool        IsGold                 ( void ) const;
FEX bool        IsInsideInventory      ( void ) const			{  return ( m_IsInsideInventory );  }
    bool        IsInsideInventory      ( eEquipSlot* slot ) const;
FEX bool        IsNoStartupFx          ( void ) const			{  return ( m_NoStartupFx );  }
FEX bool        IsAutoExpirationEnabled( void ) const			{  return ( m_EnableAutoExpiration );  }
FEX bool        IsItem                 ( void ) const;
FEX bool        IsJames                ( void ) const;
FEX bool        IsMeleeWeapon          ( void ) const;
FEX bool        IsPotion               ( void ) const;
FEX bool        IsRangedWeapon         ( void ) const;
FEX bool        IsScreenParty          ( void ) const;
FEX bool        IsScreenPartyMember    ( void ) const;
FEX bool        IsScreenPlayerOwned    ( void ) const;
FEX bool        IsSelectable           ( void ) const;
FEX bool        IsShield               ( void ) const;
FEX bool        IsSpawned              ( void ) const;
FEX bool        IsSpell                ( void ) const;
FEX bool        IsSpellBook            ( void ) const;
FEX bool        IsUsable               ( void ) const;
FEX bool		IsGhostUsable		   ( void ) const;
FEX bool        IsWeapon               ( void ) const;
FEX bool		IsGhost				   ( void ) const;
FEX bool		IsTeamMember		   ( Goid object ) const;

// Events.

	// $$$ OPTIONS for debug render level etc. - make sure to track local vs.
	//     global for overriding/don't-care settings. possibly use two
	//     bitfields? one mask and one options set?

	void HandleMessage        ( const WorldMessage& msg );
	void Update               ( float deltaTime );
	void UpdateSpecial        ( float deltaTime );
	void Draw                 ( void );
	void SetModifiersDirty    ( bool set = true );
	void SetNoStartupFx       ( bool set = true )		{  m_NoStartupFx = set;  }
	bool CheckModifierRecalc  ( void );					// returns true if modifiers were updated
	void SetSpaceDirty        ( bool set = true )		{  m_IsSpaceDirty = set;  if ( set )  {  SetWobbDirty();  }  }
	void SetWobbDirty         ( bool set = true )		{  m_IsWobbDirty = set;  if ( set )  {  SetRenderDirty();  }  }
	void SetRenderDirty       ( bool set = true )		{  m_IsRenderDirty = set;  }
	void SetBucketDirty       ( bool set = true );
	void SetServerOnly        ( bool set = true );
FEX void EnableAutoExpiration ( bool set );
FEX void EnableAutoExpiration ( void )					{  EnableAutoExpiration( true );  }
FEX void DisableAutoExpiration( void )					{  EnableAutoExpiration( false );  }

	// $$$ OPTIONS for dumping - have one for verbose/brief/debug_hud_label etc. and one for class or tuning (ai, combat, general, etc.)

#	if !GP_RETAIL
	void DrawDebugHud( void );
	void Dump        ( bool verbose, ReportSys::ContextRef ctx = NULL );
#	endif // !GP_RETAIL

	// special callback from within components
	void SetRendered( void );

// Commands.

	// local go-based sounds
FEX DWORD    PlayVoiceSound        ( const char* eventName, bool local3d = true );
FEX void     SPlayVoiceSound       ( const char* eventName, bool local3d = true );
FEX	void     RCPlayVoiceSound      ( DWORD machineId, const char* eventName, bool local3d = true );
	void     RCPlayVoiceSound      ( const char* eventName, bool local3d = true )		{  RCPlayVoiceSound( RPC_TO_ALL, eventName, local3d );  }
FEX void     RCPlayVoiceSoundPeek  ( const char* eventName, bool local3d = true );
	void     RCPlayVoiceSoundId    ( DWORD machineId, eVoiceSound event, bool local3d = true );
	void     RCPlayVoiceSoundId    ( eVoiceSound event, bool local3d = true )			{  RCPlayVoiceSoundId( RPC_TO_ALL, event, local3d );  }
FEX void     RCPlayVoiceSoundIdPeek( eVoiceSound event, bool local3d = true );
FEX DWORD    PlayVoiceSound        ( const char* eventName, float minDist, float maxDist, bool bLoop = false );

	// material-based sounds detected from dst
FEX void     SPlayMaterialSound    ( const char* eventName, Goid dstGoid = GOID_INVALID, bool local3d = true );
FEX void     RCPlayMaterialSound   ( const char* eventName, Goid dstGoid, bool local3d );
FEX DWORD    PlayMaterialSound     ( const char* eventName, Goid dstGoid = GOID_INVALID, bool local3d = true );

	// material-based sounds with explicit type
FEX void     SPlayMaterialSound    ( const char* eventName, const char* dstMaterial, bool local3d = true );
FEX void     RCPlayMaterialSound   ( const char* eventName, const char* dstMaterial, bool local3d );
FEX DWORD    PlayMaterialSound     ( const char* eventName, const char* dstMaterial, bool local3d = true );

	// generic sound playback
FEX DWORD    PlaySound             ( const char* soundName, bool local3d = true, GPGSound::eSampleType sType = GPGSound::SND_EFFECT_NORM );
FEX DWORD    PlaySound             ( const char* soundName, GPGSound::eSampleType sType,
							       	 float minDist, float maxDist, bool bLoop = false );

	// misc sound
FEX void					StopSound		       ( DWORD soundId );
	gpstring				GetVoiceFromEvent      ( const char * eventName, gpstring * pri = NULL );
	GPGSound::eSampleType	GetSampleTypeFromString( const gpstring& pri );

	// save/restore to a persist context
	bool Xfer             ( FuBi::PersistContext& persist );
	bool XferComponents   ( FuBi::PersistContext& persist );
	bool XferCharacter    ( FuBi::PersistContext& persist );
	bool XferCharacterPost( bool isFullXfer = false );
	bool XferForSync      ( FuBi::BitPacker& packer, eSyncType syncType = SYNC_DEFAULT );
	bool CommitLoad       ( void );

	// network rpc syncro fun
	const_mem_ptr CreateRpcSyncData( eSyncType syncType, void*& freeThisPtr );
	void          RCAutoRpcSyncData( DWORD machineId, eSyncType syncType );
	void          RCRpcSyncData    ( DWORD machineId, const_mem_ptr data );
FEX	void          RCRpcSyncDataPeek( const_mem_ptr data );
	void          RpcSyncData      ( const_mem_ptr data );

	// call this to add an edit component
#	if !GP_RETAIL
	void MakeEditable( void );
#	endif // !GP_RETAIL

	// stats
#	if !GP_RETAIL
	void AddRpcFrameBytes( int bytes )								{  m_RpcFrameBytes += bytes;  m_RpcTotalBytes += bytes;  }
	void SetRpcCreateBytes( int bytes )								{  AddRpcFrameBytes( bytes );  m_RpcCreateBytes = bytes;  }
#	endif // !GP_RETAIL

// Links.

	// management
FEX void AddChild         ( Go* child );
FEX void RemoveChild      ( Go* child );
FEX void RemoveAllChildren( void );
FEX void SSetParent       ( Go* parent );
FEX void RCSetParent      ( Go* parent );
FEX void SetParent        ( Go* parent );
FEX void ClearParent      ( void )									{  SetParent( NULL );  }

	// query
FEX bool HasChild( const Go* child ) const;
FEX bool HasChild( Goid child ) const;

	// direct collection access (const only!)
FEX const GopColl& GetChildren( void ) const						{  return ( m_ChildColl );  }

	// child access
	GopColl::const_iterator GetChildBegin( void ) const				{  return ( m_ChildColl.begin() );  }
	GopColl::const_iterator GetChildEnd  ( void ) const				{  return ( m_ChildColl.end  () );  }
	GopColl::iterator       GetChildBegin( void )					{  return ( m_ChildColl.begin() );  }
	GopColl::iterator       GetChildEnd  ( void )					{  return ( m_ChildColl.end  () );  }

	// component access
	ComponentColl::const_iterator GetComponentBegin( void ) const	{  return ( m_ComponentColl.begin() );  }
	ComponentColl::const_iterator GetComponentEnd  ( void ) const	{  return ( m_ComponentColl.end  () );  }
	ComponentColl::iterator       GetComponentBegin( void )			{  return ( m_ComponentColl.begin() );  }
	ComponentColl::iterator       GetComponentEnd  ( void )			{  return ( m_ComponentColl.end  () );  }

	// special iters that skip server components on clients
	ComponentColl::const_iterator GetClientComponentBegin( void ) const	{  return ( m_ComponentColl.begin() + (IsServerRemote() ? m_ClientBegin : 0) );  }
	ComponentColl::iterator       GetClientComponentBegin( void )		{  return ( m_ComponentColl.begin() + (IsServerRemote() ? m_ClientBegin : 0) );  }

// Loc helpers.

FEX const gpstring& GetMessage( const char* messageName );

// Skrit component property access.

	const FuBi::Record* GetComponentRecord( const char* componentName, const char* withPropertyName = NULL ) const;
	FuBi::Record*       GetComponentRecord( const char* componentName, const char* withPropertyName = NULL  );

	// $ you can use NULL or "" for the componentName to have it look for and
	//   use the first one it finds with the property you want.

FEX int             GetComponentInt   ( const char* componentName, const char* propertyName, int defInt );
FEX int             GetComponentInt   ( const char* componentName, const char* propertyName )
										{  return ( GetComponentInt( componentName, propertyName, 0 ) );  }
FEX bool            GetComponentBool  ( const char* componentName, const char* propertyName, bool defBool );
FEX bool            GetComponentBool  ( const char* componentName, const char* propertyName )
										{  return ( GetComponentBool( componentName, propertyName, false ) );  }
FEX float           GetComponentFloat ( const char* componentName, const char* propertyName, float defFloat );
FEX float           GetComponentFloat ( const char* componentName, const char* propertyName )
										{  return ( GetComponentFloat( componentName, propertyName, 0 ) );  }
FEX const gpstring& GetComponentString( const char* componentName, const char* propertyName, const gpstring& defString );
FEX const gpstring& GetComponentString( const char* componentName, const char* propertyName )
										{  return ( GetComponentString( componentName, propertyName, gpstring::EMPTY ) );  }
FEX Scid            GetComponentScid  ( const char* componentName, const char* propertyName, Scid defScid );
FEX Scid            GetComponentScid  ( const char* componentName, const char* propertyName )
										{  return ( GetComponentScid( componentName, propertyName, SCID_INVALID ) );  }

FEX bool SetComponentInt   ( const char* componentName, const char* propertyName, int valInt );
FEX bool SetComponentBool  ( const char* componentName, const char* propertyName, bool valBool );
FEX bool SetComponentFloat ( const char* componentName, const char* propertyName, float valFloat );
FEX bool SetComponentString( const char* componentName, const char* propertyName, const gpstring& valString );
FEX bool SetComponentScid  ( const char* componentName, const char* propertyName, Scid valScid );

// Special.

	// $ do not call these unless you know what you're doing!
	void UpdateChildrenPositions( bool forceUpdate );
	void SetIsPContentInventory( bool set = true )			{  m_IsPContentInventory = set;  }
	void SetIsPContentGenerated( bool set = true )			{  m_IsPContentGenerated = set;  }
	void SetIsInsideInventory  ( bool set = true )			{  m_IsInsideInventory	 = set;  }
	void SetIsOmni			   ( bool set = true );

	// special go rpc routers
FEX void RCExplodeGo                  ( float magnitude, const vector_3 &initial_velocity, DWORD seed );
	void RCPlayCombatSound            ( Rules::eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow );
FEX void RCPlayCombatSoundPacker      ( const_mem_ptr packola );
FEX void RCSetCollision               ( Goid contact_object, bool bGlance );
FEX void RCRemoveAllEnchantmentsOnSelf( void );
FEX void RCSend                       ( WorldMessage& message, MESSAGE_DISPATCH_FLAG flags );
FEX void RCDeactivateTrigger          ( WORD trignum );

	// special forced transfer that will convert one expired object into this one
	void       SForcedExpiredTransfer ( Go* src );
FEX	FuBiCookie RCForcedExpiredTransfer( Go* src );

// Components.

FEX GoComponent*       QueryComponent( const char* name, bool mustExist = false );
	const GoComponent* QueryComponent( const char* name, bool mustExist = false ) const
	{
		// $ just use the other QI function - no difference between the const and non-const versions.
		return ( ccast <ThisType*> ( this )->QueryComponent( name, mustExist ) );
	}

		  GoComponent* QueryComponent( const gpstring& name, bool mustExist = false );
	const GoComponent* QueryComponent( const gpstring& name, bool mustExist = false ) const
	{
		// $ just use the other QI function - no difference between the const and non-const versions.
		return ( ccast <ThisType*> ( this )->QueryComponent( name, mustExist ) );
	}

FEX GoComponent*       GetComponent( const char* name )				{  return ( QueryComponent( name, true ) );  }
	const GoComponent* GetComponent( const char* name ) const		{  return ( QueryComponent( name, true ) );  }
FEX bool               HasComponent( const char* name ) const		{  return ( QueryComponent( name ) != NULL );  }
	bool               HasComponent( const gpstring& name ) const	{  return ( QueryComponent( name ) != NULL );  }

	typedef void** CacheIter;
	typedef const void* const* ConstCacheIter;

	// these declare QueryXxx() and GetXxx() in const and non-const versions,
	// both of which just return a pointer to the component (which is probably
	// cached). the Query() takes a param to assert if the component does not
	// exist, and the Get() calls Query() with that parameter true.

	// $ make sure to update the string map and the first/last macros. note that
	// all the goofy code here is to avoid having to update five different areas
	// every time you add an component. just two instead.

	// $$ future: remove rarely used cache elements if necessary and expand the
	// macros to call the QueryComponent() in those cases.

#	define ADD_COMPONENT__( T ) 															\
			Go##T* Query##T( bool mustExist = false ) 										\
				{  gpassert( IsPointerValid( this ) );										\
				   gpassert( !mustExist || (m_##Go##T != NULL) );  return ( m_##Go##T );  }	\
			FEX Go##T* FUBI_RENAME( Query##T )( bool mustExist )							\
				{  return ( Query##T( mustExist ) );  }										\
			FEX Go##T* FUBI_RENAME( Query##T )( void )										\
				{  return ( Query##T() );  }												\
			const Go##T* Query##T( bool mustExist = false ) const							\
				{  gpassert( IsPointerValid( this ) );										\
				   gpassert( !mustExist || (m_##Go##T != NULL) );  return ( m_##Go##T );  }	\
			FEX Go##T* Get##T( void )		{  return ( Query##T( true ) );  }				\
			const Go##T* Get##T( void ) const		{  return ( Query##T( true ) );  }		\
			FEX bool Has##T( void ) const	{  return ( Query##T() != NULL );  }			\
		private:																			\
			Go##T* m_##Go##T;																\
		public:

#	define ADD_COMPONENT_0( T )																			\
		ADD_COMPONENT__( T )																			\
		CacheIter GetCacheBegin( void )				{  return ( (CacheIter)&m_##Go##T );  }				\
		ConstCacheIter GetCacheBegin( void ) const	{  return ( (ConstCacheIter)&m_##Go##T );  }

#	define ADD_COMPONENT_N( T )																			\
		ADD_COMPONENT__( T )																			\
		CacheIter GetCacheEnd( void )				{  return ( (CacheIter)&m_##Go##T + 1 );  }			\
		ConstCacheIter GetCacheEnd( void ) const	{  return ( (ConstCacheIter)&m_##Go##T + 1 );  }

	// keep these in sync with the autocache name list below

	ADD_COMPONENT_0( Actor        );
	ADD_COMPONENT__( Aspect       );
	ADD_COMPONENT__( Attack       );
	ADD_COMPONENT__( Body         );
	ADD_COMPONENT__( Common       );
	ADD_COMPONENT__( Conversation );
	ADD_COMPONENT__( Defend       );
#	if !GP_RETAIL
	ADD_COMPONENT__( Edit         );
#	endif
	ADD_COMPONENT__( Fader        );
	ADD_COMPONENT__( Follower     );
#	if !GP_RETAIL
	ADD_COMPONENT__( Gizmo        );
#	endif // !GP_RETAIL
	ADD_COMPONENT__( Gold         );
	ADD_COMPONENT__( Gui          );
	ADD_COMPONENT__( Inventory    );
	ADD_COMPONENT__( Magic        );
	ADD_COMPONENT__( Mind         );
	ADD_COMPONENT__( Party        );
	ADD_COMPONENT__( Physics      );
	ADD_COMPONENT__( Placement    );
	ADD_COMPONENT__( Potion       );
	ADD_COMPONENT__( Stash        );
	ADD_COMPONENT_N( Store        );	

#	undef ADD_COMPONENT

	struct AutoCache : stdx::fast_vector <const char*>
	{
		AutoCache( void )
		{
			push_back( "actor"        );
			push_back( "aspect"       );
			push_back( "attack"       );
			push_back( "body"         );
			push_back( "common"       );
			push_back( "conversation" );
			push_back( "defend"       );
#			if !GP_RETAIL
			push_back( "edit"         );
#			endif // !GP_RETAIL
			push_back( "fader"        );
			push_back( "follower"     );
#			if !GP_RETAIL
			push_back( "gizmo"        );
#			endif // !GP_RETAIL
			push_back( "gold"         );
			push_back( "gui"          );
			push_back( "inventory"    );
			push_back( "magic"        );
			push_back( "mind"         );
			push_back( "party"        );
			push_back( "physics"      );
			push_back( "placement"    );
			push_back( "potion"       );
			push_back( "stash"		  );
			push_back( "store"        );			
		}
	};

	// cache access
	static const AutoCache& GetAutoCache  ( void );
	static int              FindCacheIndex( const char* name );

	// $ special - do not call these unless you know what you're doing!
	bool AddNewComponent( const gpstring& type );
	void AddNewComponent( GoComponent* newGoc, bool serverComponent );

// Debug.

	GPDEBUG_ONLY( bool AssertValid( void ) const; )

private:
	void Init                ( bool resetPlayer = true );
	void ResetCache          ( bool sort = true );
	bool PrivateLoad         ( const char* templateName, FastFuelHandle fuel, Scid newScid, bool shutdown = true, bool shouldPersist = true );
	void SetUpdateRoot       ( void );
	void EnterWorld          ( FrustumId membership );
	void LeaveWorld          ( bool deletingGo = false );
	void SetFrustumMembership( FrustumId membership );

// Setup.

	// $ only fog's (friends of go's!) may perform these

	// ctor/dtor
	Go( Player* player = NULL );
	Go( const Go& source, Player* newPlayer, bool local );
   ~Go( void );

	// major operations
	void Copy            ( const Go& source, Player* newPlayer, bool local, Scid newScid = SCID_SPAWNED, bool shutdown = true );
	bool LoadFromInstance( FastFuelHandle fuel, Scid scid = SCID_INVALID, bool shutdown = true, bool shouldPersist = true );
	bool LoadFromTemplate( const char* templateName, bool shutdown = true, bool shouldPersist = true );
	bool CommitCreation  ( Goid goid, int mpPlayerCount = 0, bool preload = true );
	void Shutdown        ( bool reinit = true );
	void UnlinkChild     ( Go* child );
	void SetHasFrustum   ( bool set = true );	

	// utility
	void PrivateCheckUpdates   ( void );
	void PrivateStartUpdates   ( void );
	Go*  PrivateStopUpdates    ( void );
	void PrivateRecalcModifiers( void );

	// ref counting
#	if !GP_RETAIL
	void IncRefCount( void )				{  gpassert( m_RefCount >= 0 );  ::InterlockedIncrement( (LONG*)&m_RefCount );  gpassert( m_RefCount < 100 );  }
	void DecRefCount( void )				{  gpassert( m_RefCount > 0 );  ::InterlockedDecrement( (LONG*)&m_RefCount );  }
#	endif // !GP_RETAIL

	friend GoDb;
	friend GoRpcMgr;
	friend ContentDb;
	friend GoCreateReq;
	friend GoCloneReq;
	friend GoHandle;
	friend std::auto_ptr <Go>;
	friend stdx::delete_by_ptr;
	friend stdx::delete_second_by_ptr;

// Data.

	// creation spec
	Goid                  m_Goid;			// id of this go
	Scid                  m_Scid;			// scid of instance
	const GoDataTemplate* m_DataTemplate;	// template this was instanced from
	DWORD                 m_RandomSeed;		// random seed set before creation (for replicating pcontent)
	int                   m_MpPlayerCount;	// how many players were in this game when this go was created?
	Goid                  m_CloneSource;	// who were we originally cloned from? this is cleared out upon detect a mismatch
	RegionId              m_RegionSource;	// what region was i originally created in? used for scid sync across machiens

#	if !GP_RETAIL
	gpstring m_DevInstanceFuel;				// fuel address original instance came from
	int      m_DevWarningCount;				// how many warnings were generated in updating this particular go
	int      m_DevErrorCount;				// how many errors were generated in updating this particular go
	int      m_DevErrorCountScope;			// how deep in recursion we are for reporting errors?
#	endif

	// state
	FrustumId     m_FrustumMembership;		// which frustums am i a member of?
	Go*           m_Parent;					// who is my parent go?
	Go*           m_NextUpdate;				// who is next in the update list? (internal ring doubly-linked list)
	Go*           m_PrevUpdate;				// who comes before me in the update list?
	Player*       m_Player;					// which player owns me?
	ComponentColl m_ComponentColl;			// collection of owned components, server components first, clients second
	int           m_ClientBegin;			// index of first client component
	GopColl       m_ChildColl;				// collection of referenced children
	unsigned int  m_LastSimViewOccupant;	// id of last sim where this go was an occupant of a siege node in the view frustum

#	if !GP_RETAIL
	double m_CreateTime;					// time when we were created
	int    m_RefCount;						// ref count on this go from gohandles (for debug only)
	int    m_PlacementChanging;				// this is in-between placement changes (inside a fixup loop)
	int    m_RpcCreateBytes;				// cost in RPC bytes to create this
	int    m_RpcFrameBytes;					// cost this frame in RPC bytes
	int    m_RpcTotalBytes;					// total cost in RPC bytes so far
#	endif

	// bool spec
	bool m_IsOmni               : 1;		// i'm an omniscient/omnipotent go! fear my wrath! i don't live in this world so on your knees, fool!
	bool m_IsAllClients         : 1;		// this go exists on all clients (party members and the player parties only)
	bool m_WantsUpdates         : 1;		// this go wants updates - make sure it is part of the update list when it is in an active world frustum
	bool m_WantsDraws           : 1;		// this go wants to be able to draw
	bool m_IsCommand            : 1;		// does this Go host a cmd_* filename command skrit component?
	bool m_IsCommandCached      : 1;		// has the IsCommand query been completed and cached
	bool m_IsPContentInventory  : 1;		// was this Go generated for inventory via a pcontent query?
	bool m_IsPContentGenerated  : 1;		// was this Go generated and modified via a pcontent query?
	bool m_IsLodfi              : 1;		// can this Go be removed based on lodfi?
	bool m_IsServerOnly         : 1;		// is this a global go that only exists on the server?
	bool m_EnableAutoExpiration : 1;		// enable automatic expiration for this game object (on by default)

	// bool state
	bool m_HasFrustum           : 1;		// this go has its own frustum!
	bool m_IsFocused            : 1;		// this is a focused go, meaning it is first in the list of selected party go's
	bool m_IsSelected           : 1;		// it's selected
	bool m_IsMouseShadowed      : 1;		// this go is in the mouse shadow
	bool m_IsInHotGroup         : 1;		// this is a "hot" go, meaning it is a party member within x meters of the focus go
	bool m_ModifiersDirty       : 1;		// modifiers need to be recalc'ed next chance
	bool m_PendingUpdateRemoval : 1;		// this go is pending removal from the update list (must buffer these requests to avoid chaos in godb update loop)
	bool m_MarkedForDeletion    : 1;		// this go has been requested to be deleted
	bool m_DelayedMpDeletion    : 1;		// this go has been requested to be deleted with the mp delayed option set
	bool m_IsExpiring           : 1;		// this go is currently expiring
	bool m_IsCommittingCreation : 1;		// this go is currently being committed
	bool m_IsPendingEntry       : 1;		// pending entry into the world (sitting in godb's m_PendingGoDb)
	bool m_IsSpaceDirty         : 1;		// true if space is dirty for this object - worldpos must be recalc'd
	bool m_IsWobbDirty          : 1;		// true if the world-space oriented bounding box is dirty for this object - must be recalc'd
	bool m_IsRenderDirty        : 1;		// true if rot/pos/anim changed for this object - lighting must be recalc'd
	bool m_IsBucketDirty        : 1;		// true if this go or any other go's in its bucket have been dirtied by some critical event, which changes how this is reconstructed on clients
	bool m_IsFading             : 1;		// true if this go has an aspect and it is fading in/out
	bool m_IsFadingFirstView    : 1;		// true if we're on the first view for a fade
	bool m_IsInsideInventory    : 1;		// true if we're inside someone's inventory
	bool m_IsEnchanted          : 1;		// true if there are enchantments running on this go
	bool m_IsPreparedAmmo       : 1;		// true if being cloned as ammo
	bool m_NoStartupFx          : 1;		// true if no startup fx should be run on this

	// temporary states (not persisted, do not assume current value)
	bool m_Visited              : 1;		// this go has been visited
	bool m_Transferring         : 1;		// this go is being transferred

#	if GP_DEBUG
	bool m_DbgInGoDb            : 1;		// once we're definitely in the godb, this is set true
#	endif

public:
	static DWORD GoToNet( Go* x );
	static Go*   NetToGo( DWORD d, FuBiCookie* cookie );

private:
	SET_NO_COPYING( Go );
	FUBI_RPC_CLASS( Go, GoToNet, NetToGo, "" );
};

//////////////////////////////////////////////////////////////////////////////
// class GoHandle declaration

class GoHandle
{
public:
	SET_NO_INHERITED( GoHandle );

// Construction.

	// ctor/dtor
	GoHandle( void )						{  m_Go = NULL;  }
#	if !GP_RETAIL
   ~GoHandle( void )						{  operator = ( GOID_INVALID );  }
#	endif // !GP_RETAIL
	explicit GoHandle( Goid goid )			{  GPDEV_ONLY( m_Go = NULL );  operator = ( goid );  }
	explicit GoHandle( Go* go )				{  GPDEV_ONLY( m_Go = NULL );  operator = ( go );  }

// Query.

	bool IsValid( void ) const				{  return ( m_Go != NULL );  }

	Go*       Get     ( void )				{  gpassert( m_Go != NULL );  return ( m_Go );  }
	const Go* GetConst( void ) const		{  gpassert( m_Go != NULL );  return ( m_Go );  }

	Go*       GetUnsafe( void )				{  return ( m_Go );  }
	const Go* GetUnsafe( void ) const		{  return ( m_Go );  }

// Operators.

	Go*       operator -> ( void )			{  return ( Get() );  }
	const Go* operator -> ( void ) const	{  return ( GetConst() );  }

	Go&       operator *  ( void )			{  return ( *Get() );  }
	const Go& operator *  ( void ) const	{  return ( *GetConst() );  }

	operator Go*       ( void )				{  return ( m_Go );  }
	operator const Go* ( void ) const		{  return ( m_Go );  }

	GoHandle& operator = ( Goid goid );
	GoHandle& operator = ( Go* go );

private:
	Go* m_Go;

	SET_NO_COPYING( GoHandle );
};

inline GoHandle& GoHandle :: operator = ( Go* go )
{
#	if !GP_RETAIL
	if ( m_Go != NULL )
	{
		m_Go->DecRefCount();
	}
#	endif // !GP_RETAIL

	m_Go = go;

#	if !GP_RETAIL
	if ( m_Go != NULL )
	{
		m_Go->IncRefCount();
	}
#	endif // !GP_RETAIL

	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __GO_H

//////////////////////////////////////////////////////////////////////////////
