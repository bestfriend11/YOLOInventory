//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoAspect.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the GoAspect component
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOASPECT_H
#define __GOASPECT_H

//////////////////////////////////////////////////////////////////////////////

#include "Nema_Types.h"
#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoAspect declaration

class GoAspect : public GoComponent
{
public:
	SET_INHERITED( GoAspect, GoComponent );

// Types.

	enum eOptions
	{
		// $ note: options tagged as (local) are not part of the public schema

		OPTION_NONE						=      0,

		// rendering
		OPTION_IS_VISIBLE				= 1 << 0,		// is this object visible (does it render)?
		OPTION_DRAW_SHADOW				= 1 << 1,		// should this object draw a shadow when rendered?
		OPTION_INTEREST_ONLY			= 1 << 2,		// should this object draw only when it is in the sphere of interest?
		OPTION_DRAW_SELECTION_INDICATOR = 1 << 3,		// should this object draw the selection indicator when it is highlighted/selected?
		OPTION_FORCE_NO_RENDER			= 1 << 4,		// never render this (different from invisible, don't ask why)

		// physics
		OPTION_IS_COLLIDABLE			= 1 << 5,		// is this object collidable with other objects like projectiles?
		OPTION_DOES_BLOCK_PATH			= 1 << 6,		// does this object prevent another from walking through it?
		OPTION_HAS_BLOCKED_PATH			= 1 << 7,		// does this object prevent another from walking through it?
		OPTION_IS_INVINCIBLE    		= 1 << 8,		// is this object immune to damage?

		// gui
		OPTION_IS_SELECTABLE			= 1 << 9,		// is object selectable from the GUI?
		OPTION_DOES_BLOCK_CAMERA		= 1 << 10,		// should camera try to avoid this object?
		OPTION_IS_USABLE				= 1 << 11,		// can this be used? opened, closed, switched etc.
		OPTION_IS_GHOST_USABLE			= 1 << 12,		// can this be used by a ghost?
		OPTION_IS_VIRGIN_PICKUP			= 1 << 13,		// has this item been picked up before?


		// other
		OPTION_IS_GAGGED				= 1 << 14,		// is this object "gagged" - not allowed to make sounds?
		OPTION_MEGAMAP_OVERRIDE			= 1 << 15,		// should this object draw on the megamap no matter what?
		OPTION_MEGAMAP_ORIENT			= 1 << 16,		// should this object's megamap icon be oriented?
		OPTION_DYNAMIC_LIGHT			= 1 << 17,		// is this object affected by dynamic light?
	};

// Setup.

	// ctor/dtor
	GoAspect( Go* parent );
	GoAspect( const GoAspect& source, Go* newParent );
	virtual ~GoAspect( void );

#	if !GP_RETAIL
FEX	void Dump( ReportSys::Context * ctx );
#	endif


// Options.

	#define PUB public:
	#define PRV private:

	// simple options
	void SetOptions   ( eOptions options, bool set = true )		{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{  return ( (m_Options & options) != 0 );  }

	// query helpers
FEX	bool GetIsVisible             ( void ) const;
FEX bool GetIsVisibleThisFrame	  ( void ) const				{  return ( m_VisibleThisFrame ); }
FEX	bool GetShouldDrawShadow      ( void ) const				{  return ( TestOptions( OPTION_DRAW_SHADOW              ) );  }
FEX	bool GetInterestOnly          ( void ) const				{  return ( TestOptions( OPTION_INTEREST_ONLY            ) );  }
FEX	bool GetDrawSelectionIndicator( void ) const				{  return ( TestOptions( OPTION_DRAW_SELECTION_INDICATOR ) );  }
FEX	bool GetForceNoRender         ( void ) const				{  return ( TestOptions( OPTION_FORCE_NO_RENDER          ) );  }
FEX	bool GetIsCollidable          ( void ) const				{  return ( TestOptions( OPTION_IS_COLLIDABLE            ) );  }
FEX	bool GetDoesBlockPath         ( void ) const				{  return ( TestOptions( OPTION_DOES_BLOCK_PATH          ) );  }
FEX	bool GetHasBlockedPath        ( void ) const				{  return ( TestOptions( OPTION_HAS_BLOCKED_PATH         ) );  }
FEX	bool GetIsInvincible          ( void ) const				{  return ( TestOptions( OPTION_IS_INVINCIBLE            ) );  }
FEX	bool GetIsSelectable          ( void ) const				{  return ( TestOptions( OPTION_IS_SELECTABLE            ) );  }
FEX	bool GetDoesBlockCamera       ( void ) const				{  return ( TestOptions( OPTION_DOES_BLOCK_CAMERA        ) );  }
FEX	bool GetIsUsable              ( void ) const				{  return ( TestOptions( OPTION_IS_USABLE                ) );  }
FEX bool GetIsGhostUsable		  ( void ) const				{  return ( TestOptions( OPTION_IS_GHOST_USABLE			 ) );  }
FEX	bool GetIsGagged              ( void ) const				{  return ( TestOptions( OPTION_IS_GAGGED                ) );  }
FEX bool GetMegamapOverride       ( void ) const				{  return ( TestOptions( OPTION_MEGAMAP_OVERRIDE         ) );  }
FEX bool GetMegamapOrient         ( void ) const				{  return ( TestOptions( OPTION_MEGAMAP_ORIENT           ) );  }
FEX bool GetDynamicallyLit        ( void ) const				{  return ( TestOptions( OPTION_DYNAMIC_LIGHT			 ) );  }
FEX bool GetIsVirginPickup        ( void ) const				{  return ( TestOptions( OPTION_IS_VIRGIN_PICKUP		 ) );  }

	// set helpers	
	void SetShouldDrawShadow		  ( bool set = true )		{  SetOptions( OPTION_DRAW_SHADOW             , set );  }
	void SetInterestOnly              ( bool set = true )		{  SetOptions( OPTION_INTEREST_ONLY           , set );  }
FEX	void SetDrawSelectionIndicator    ( bool set = true )		{  SetOptions( OPTION_DRAW_SELECTION_INDICATOR, set );  }
FEX	void SetIsCollidable    		  ( bool set = true )		{  SetOptions( OPTION_IS_COLLIDABLE           , set );  }
FEX	void SetDoesBlockPath   		  ( bool set = true );
FEX	void SetIsInvincible    		  ( bool set = true )		{  SetOptions( OPTION_IS_INVINCIBLE           , set );  }
	void SetIsGagged	    		  ( bool set = true )		{  SetOptions( OPTION_IS_GAGGED               , set );  }
	void SetMegamapOverride           ( bool set = true )       {  SetOptions( OPTION_MEGAMAP_OVERRIDE        , set );  }
	void SetMegamapOrient             ( bool set = true )       {  SetOptions( OPTION_MEGAMAP_ORIENT          , set );  }
	void SetDynamicallyLit			  ( bool set = true )		{  SetOptions( OPTION_DYNAMIC_LIGHT           , set );  }
	void SetVirginPickup			  ( bool set = true )		{  SetOptions( OPTION_IS_VIRGIN_PICKUP        , set );  }

	void SetIsScreenPlayerVisibleCount( DWORD count )			{  m_ScreenPlayerVisibleCount = count; }

	// other query
	bool GetIsScreenPlayerVisible( void ) const;

	// use range
	void  SetUseRange( float range )							{  m_UseRange = range; }
FEX	float GetUseRange( void ) const								{  return ( m_UseRange ); }

	// rpc helpers
PUB FEX void SSetIsVisible		( bool set = true );
PRV FEX void RCSetIsVisible		( bool set = true );
PUB FEX void SetIsVisible		( bool set = true );
PUB FEX void SSetIsSelectable	( bool set = true );
PRV FEX void RCSetIsSelectable	( bool set = true );
PUB     void SetIsSelectable	( bool set = true );
PUB FEX void SSetIsUsable		( bool set = true );
PRV FEX void RCSetIsUsable		( bool set = true );
PUB     void SetIsUsable		( bool set = true );
PUB FEX void SSetIsGhostUsable	( bool set = true );
PRV FEX void RCSetIsGhostUsable	( bool set = true );
PUB     void SetIsGhostUsable	( bool set = true );
PUB FEX void SSetFreezeMeshFlag ( bool set = true );
PRV FEX void RCSetFreezeMeshFlag( bool set = true );
PUB     void SetFreezeMeshFlag  ( bool set = true );
PUB FEX void SSetLockMeshFlag   ( bool set = true );
PRV FEX void RCSetLockMeshFlag  ( bool set = true );
PUB     void SetLockMeshFlag    ( bool set = true );

	// fading helpers
PUB FEX void RCFadeOut( void );

public:

// Interface.

	// const query

#if	!GP_RETAIL
	float           GetDisplayCost					( void ) const;
#endif  // !GP_RETAIL

	float           GetLowerLodfi					( void ) const;
	float           GetUpperLodfi					( void ) const;
	const gpstring& GetMaterial						( void ) const;
	DWORD			GetGhostAlpha					( void ) const;
	DWORD			GetCamFadeAlpha					( void ) const;
	bool			GetRolloverHighlight			( void ) const;
	bool			GetTemplateIsCollidable			( void ) const;	// Is the template set to collidable in the template/instance?
																	// If so, we can set the collidable flag to this when the aspect is resurrected.

	// experience tracking stuff
	float			GetExperienceValue				( void ) const		{  return ( m_ExperienceValue ); }
	void			SetExperienceValue				( float val )		{  m_ExperienceValue = val; }

	float           GetExperienceRemaining			( void ) const		{  return ( m_ExperienceRemaining ); }
	void			SetExperienceRemaining			( float value )		{  m_ExperienceRemaining = value; }

	Goid			GetExperienceBenefactor			( void ) const		{  return ( m_ExperienceBenefactor ); }
FEX	void			SetExperienceBenefactor			( Goid go )			{  m_ExperienceBenefactor = go; }

	const gpstring& GetExperienceBenefactorSkill	( void ) const		{  return m_ExperienceBenefactorSkill; }
FEX	void			SetExperienceBenefactorSkill	( const char *s )	{  m_ExperienceBenefactorSkill = s; }

FEX	void			ClearExperienceBenefactor		( void );

	const gpstring& GetExpiredTemplateName	 ( void ) const;
	const gpstring& GetMegamapIcon			 ( void ) const;

	// scale query
	float GetSelectionIndicatorScale( void ) const				{  return ( m_SelectionIndicatorScale );  }
FEX	float GetRenderScale            ( void ) const				{  return ( m_RenderScaleMultiplier * m_RenderScaleBase );  }
	float GetRenderScaleMultiplier  ( void ) const				{  return ( m_RenderScaleMultiplier );  }

	// scale modifications
	void SetRenderScaleMultiplier( float scale )				{  gpassert( scale > 0 );  m_RenderScaleMultiplier = scale;  GetGo()->SetWobbDirty();  }

	// state query
FEX	nema::HAspect GetAspectHandle( void ) const;
FEX	nema::Aspect* GetAspectPtr   ( void ) const;
	bool          HasAspectHandle( void ) const;
	bool          IsNemaRoot     ( void ) const;
	void          ClearNemaParent( void ) const;

	// life state
PUB	FEX	eLifeState GetLifeState          ( void ) const			{  return ( m_LifeState );  }
PUB	FEX	void       SSetLifeState         ( eLifeState state, bool forceUpdate );
PUB	FEX	void       SSetLifeState         ( eLifeState state )	{  SSetLifeState( state, true );  }
PRV	FEX	void       RCSetLifeState        ( eLifeState state );
PUB FEX void	   SetLifeState			 ( eLifeState state );
FEX		bool	   GetIsRespawningAsGhost( void ) const			{  CHECK_SERVER_ONLY; return m_bIsRespawningAsGhost;  }
		void	   SetIsRespawningAsGhost( bool respawning )	{  CHECK_SERVER_ONLY; m_bIsRespawningAsGhost = respawning;  }
		void	   HandleKilled          ( void );

	// misc
PUB		float	   GetGhostTimeout		 ( void ) const;
PUB	FEX void	   SResetGhostTimeout	 ( void );
PUB FEX void	   RCSetGhostTimeout	 ( double timeout );

	// gold
	#if !GP_RETAIL
PUB	FEX FuBiCookie RSSetGoldValue( int value );
	#endif
PUB	FEX void       SSetGoldValue ( int value, bool bForceRC );
PRV	FEX FuBiCookie RCSetGoldValue( int value );
PUB	FEX void       SetGoldValue  ( int value );
PUB	FEX	int        GetGoldValue  ( void ) const						{  return ( m_GoldValue );  }
PUB		void       AddGoldValue  ( int value )						{  SetGoldValue( GetGoldValue() + value );  }

public:

	// aspect changing
	void        SetCurrentAspect         ( nema::HAspect newAspect, bool transferInternals = false );
	void        SetCurrentAspectSimple   ( nema::HAspect newAspect );
	void        SetCurrentAspectSimple   ( const char* aspectName, const char* textureName = NULL );
	void        SetNativeAspect          ( nema::HAspect newAspect );
	void        RestoreNativeAspect      ( bool transferInternals = false );
	bool        IsCurrentAspectNative    ( void ) const		{  return ( !m_NativeAspect );  }

	// aspect texture changing
PUB FEX void		RSSetCurrentAspectTexture( int texIndex, const char* textureName = NULL );
PUB FEX void		SSetCurrentAspectTexture ( int texIndex, const char* textureName = NULL );
PRV FEX	void        RCSetCurrentAspectTexture( int texIndex, const char* textureName = NULL );
PUB FEX	const char* GetCurrentAspectTexture  ( int texIndex );
PUB 	void        SetCurrentAspectTexture  ( int texIndex, const char* textureName = NULL );

	// texture animation speed
PUB FEX void		SSetAspectTextureSpeed   ( int texIndex, const float texSpeed );
PRV FEX void		RCSetAspectTextureSpeed  ( int texIndex, const float texSpeed );
PUB FEX float		GetAspectTextureSpeed    ( int texIndex );
PRV 	void		SetAspectTextureSpeed    ( int texIndex, const float texSpeed );

public:

	// life query
FEX	float GetMaxLife           ( void ) const					{  return ( m_MaxLife );  }
	float GetNaturalMaxLife	   ( void ) const					{  return ( m_MaxLife.GetNatural() ); }
FEX	float GetCurrentLife       ( void ) const					{  return ( m_Life );  }
FEX	float GetLifeRatio         ( void ) const					{  return ( (m_MaxLife == 0) ? 0 : (m_Life / m_MaxLife) );  }
FEX	float GetLifeRecoveryUnit  ( void ) const					{  return ( m_LifeRecoveryUnit );  }
FEX	float GetLifeRecoveryPeriod( void ) const					{  return ( m_LifeRecoveryPeriod );  }

	// life modifications
PUB	FEX	void SSetCurrentLife    ( float life );
PRV	FEX	void RCSetCurrentLife   ( float life );					FUBI_MEMBER_SET( RCSetCurrentLife, +SERVER );
PUB		void SetCurrentLife     ( float life );
PUB	FEX	void SSetMaxLife        ( float life );
PRV	FEX	void RCSetMaxLife       ( float maxLife );				FUBI_MEMBER_SET( RCSetMaxLife, +SERVER );
PUB		void SetMaxLife         ( float maxLife )				{  m_MaxLife = maxLife;  }
PUB	FEX	void SSetNaturalMaxLife ( float maxLife );
PRV	FEX	void RCSetNaturalMaxLife( float maxLife );				FUBI_MEMBER_SET( RCSetNaturalMaxLife, +SERVER );
PUB		void SetNaturalMaxLife  ( float maxLife )				{  m_MaxLife.SetNatural( maxLife );  GetGo()->SetModifiersDirty();  }

public:

	// life changing
	void SetLifeRecoveryUnit         ( float val )				{  m_LifeRecoveryUnit = val;  }
	void SetLifeRecoveryPeriod       ( float val );
	void SetNaturalLifeRecoveryUnit  ( float val )				{  m_LifeRecoveryUnit.SetNatural( val );  GetGo()->SetModifiersDirty();  }
	void SetNaturalLifeRecoveryPeriod( float val );

	// mana query
FEX	float GetMaxMana           ( void ) const					{  return ( m_MaxMana );  }
	float GetNaturalMaxMana	   ( void ) const					{  return ( m_MaxMana.GetNatural() ); }
FEX	float GetCurrentMana       ( void ) const					{  return ( m_Mana );  }
FEX	float GetManaRatio         ( void ) const					{  return ( (m_MaxMana == 0) ? 0 : (m_Mana / m_MaxMana) );  }
FEX	float GetManaRecoveryUnit  ( void ) const					{  return ( m_ManaRecoveryUnit );  }
FEX	float GetManaRecoveryPeriod( void ) const					{  return ( m_ManaRecoveryPeriod );  }

	// mana modifications
PUB	FEX	void SSetCurrentMana    ( float mana );
PRV	FEX	void RCSetCurrentMana   ( float mana );					FUBI_MEMBER_SET( RCSetCurrentMana, +SERVER );
PUB		void SetCurrentMana     ( float mana );
PUB	FEX	void SSetMaxMana        ( float maxMana );
PRV	FEX	void RCSetMaxMana       ( float maxMana );				FUBI_MEMBER_SET( RCSetMaxMana, +SERVER );
PUB		void SetMaxMana         ( float maxMana )				{  m_MaxMana = maxMana;  }
PUB	FEX	void SSetNaturalMaxMana ( float maxMana );
PRV	FEX	void RCSetNaturalMaxMana( float maxMana );				FUBI_MEMBER_SET( RCSetNaturalMaxMana, +SERVER );
PUB		void SetNaturalMaxMana  ( float maxMana )				{  m_MaxMana.SetNatural( maxMana );  GetGo()->SetModifiersDirty();  }

	// helpers for rpc efficiency
PRV FEX void RCSyncState( float life, float mana );

public:

	// mana modifications
	void SetManaRecoveryUnit         ( float val )				{  m_ManaRecoveryUnit = val;  }
	void SetManaRecoveryPeriod       ( float val );
	void SetNaturalManaRecoveryUnit  ( float val )				{  m_ManaRecoveryUnit.SetNatural( val );  GetGo()->SetModifiersDirty();  }
	void SetNaturalManaRecoveryPeriod( float val );

	// configurable texture setting
FEX	void RCSetDynamicTexture( ePlayerSkin skin, const char* name );
FEX	void SSetDynamicTexture ( ePlayerSkin skin, const char* name );
	void SetDynamicTexture  ( ePlayerSkin skin, const char* name, bool force = false );

	// voice query
	gpstring FindEventVoice( const char* event, gpstring& pri ) const;

	// configurable texture query
	const gpstring& GetDynamicTexture( ePlayerSkin skin ) const	{  return ( m_DynamicTextures[ skin ] );  }

	// complex shadow
	void CreateComplexShadowTexture();

	// megamap texture
	void CreateMegamapTexture();

	// force reconstitution
	void PrepareToDrawNow( bool includeTextures = true );

// Utility.

	// basic utility
	siege::eCollectFlags BuildCollectFlags( void ) const;
	bool                 BuildRenderInfo  ( siege::ASPECTINFO& info, siege::eRequestFlags rFlags ) const;

	// collision info
	void GetWorldSpaceOrientedBoundingVolume( matrix_3x3& orient, vector_3& center, vector_3& halfDiag ) const
											  {  GetWorldSpaceOrientedBoundingVolume( &orient, &center, &halfDiag, 0xFFFFFFFF );  }
	void GetWorldSpaceBoundingVolume        ( vector_3& center, vector_3& halfDiag ) const
											  {  GetWorldSpaceOrientedBoundingVolume( NULL, &center, &halfDiag, 0xFFFFFFFF );  }

	void GetWorldSpaceOrientedBoneBoundingVolume( matrix_3x3& orient, vector_3& center, vector_3& halfDiag, DWORD bone_index ) const
											  {  GetWorldSpaceOrientedBoundingVolume( &orient, &center, &halfDiag, bone_index );  }

	void GetWorldSpaceCenter                ( vector_3& center ) const
											  {  GetWorldSpaceOrientedBoundingVolume( NULL, &center, NULL, 0xFFFFFFFF );  }
	void GetHalfDiagonal                    ( vector_3& halfDiag ) const
											  {  GetWorldSpaceOrientedBoundingVolume( NULL, NULL, &halfDiag, 0xFFFFFFFF );  }
	void GetNodeSpaceOrientedBoundingVolume ( matrix_3x3& orient, vector_3& center, vector_3& halfDiag ) const
											  {  GetNodeSpaceOrientedBoundingVolume( &orient, &center, &halfDiag, 0xFFFFFFFF );  }
	void GetNodeSpaceOrientedBoundingVolume ( vector_3& center, vector_3& halfDiag ) const
											  {  GetNodeSpaceOrientedBoundingVolume( NULL, &center, &halfDiag, 0xFFFFFFFF );  }
	void GetNodeSpaceCenter                 ( vector_3& center ) const
											  {  GetNodeSpaceOrientedBoundingVolume( NULL, &center, NULL, 0xFFFFFFFF );  }

FEX	float GetBoundingSphereRadius( void ) const;

	// static helpers
	static void GetWorldSpaceOrientedBoundingVolume(
			matrix_3x3* orient, vector_3* center, vector_3* halfDiag,
			const SiegePos& pos, const Quat& quat, nema::Aspect* aspect,
			float boundScale = 1, float renderScale = 1, DWORD bone_index = 0xFFFFFFFF );
	static void GetNodeSpaceOrientedBoundingVolume (
			matrix_3x3* orient, vector_3* center, vector_3* halfDiag,
			const SiegePos& pos, const Quat& quat, nema::Aspect* aspect,
			float boundScale = 1, float renderScale = 1, DWORD bone_index = 0xFFFFFFFF );

	// texturing
	void ResetTextures( void );				// reset all textures to originally instanced defaults

	#undef PUB
	#undef PRV

public:

// Special.

	// $ do not call these unless you know what you're doing!
	void UpdateChildrenPositions( void );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual bool         XferCharacter( FuBi::PersistContext& persist );

	// event handlers
	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void ResetModifiers ( void );
	virtual void UnlinkParent   ( Go* parent );
	virtual bool CommitCreation ( void );
	virtual bool CommitLoad		( void );
	virtual bool CommitImport	( bool /*fullXfer*/ );
	virtual void Update         ( float deltaTime );
	virtual void UpdateSpecial  ( float deltaTime );
	virtual void DetectWantsBits( void );

private:
	void GetWorldSpaceOrientedBoundingVolume( matrix_3x3* orient, vector_3* center, vector_3* halfDiag, DWORD bone_index ) const;
	void GetNodeSpaceOrientedBoundingVolume ( matrix_3x3* orient, vector_3* center, vector_3* halfDiag, DWORD bone_index ) const;
	void UpdateDynamicTextures              ( void );
	void UpdateRegeneration                 ( float deltaTime );

	// spec
	float m_SelectionIndicatorScale;				// scalar value for adjusting the size of the selection indicator //$$$ this should be scaled by scale/scale_base to take multipliers, enchantments and other modifiers etc. into account
	float m_RenderScaleBase;						// scale to set the basic visual size of the aspect
	float m_RenderScaleMultiplier;					// multiplier on top of base scale for fine-tuning
	float m_BoundingVolumeScale;					// for shrinking or expanding the bounding volumes of objects
	float m_UseRange;

	// state
	GoString      m_DynamicTextures[ PS_COUNT ];
	eOptions      m_Options;						// simple options for this component
	nema::HAspect m_CurrentAspect;					// nema aspect (representation)
	nema::Aspect* m_CurrentAspectPtr;				// nema aspect (cached ptr)
	nema::HAspect m_NativeAspect;					// the "native" aspect to pop back to when the current aspect is restored
	int           m_GoldValue;						// value of this aspect in gold units
	float		  m_ExperienceValue;				// the experience worth of killing this monster
	float         m_ExperienceRemaining;			// how many experience points this aspect is worth for killing
	Goid		  m_ExperienceBenefactor;			// what go gets the experience from the damage done by this go
	gpstring	  m_ExperienceBenefactorSkill;		// what skill the experience should go to from the benefactor
	unsigned int  m_MegamapIconTex;					// texture index for this aspect's megamap icon
	unsigned int  m_ComplexShadowTex;				// texture index for this aspect's complex shadow

	// life
	eLifeState    m_LifeState;						// what's the object doing?
	bool		  m_bIsRespawningAsGhost;			// are we in the process of returning as a ghost?
	GobFloat      m_MaxLife;						// maximum life (hit points) the object has or the amount of damage it can take before breaking apart
	float         m_Life;							// initial hit points (defaults to max_life)
	GobFloat      m_LifeRecoveryUnit;				// life recovery units added per period (calculated continuously)
	GobFloat      m_LifeRecoveryPeriod;				// life recovery period in seconds

	// mana
	GobFloat      m_MaxMana;
	float         m_Mana;
	GobFloat      m_ManaRecoveryUnit;
	GobFloat      m_ManaRecoveryPeriod;

	// other
	float		  m_RegenSyncPeriod;				// amount of time before left before syncronizing with the server
	mutable DWORD m_NonGhostAlpha;
	double		  m_GhostTimeout;
	mutable bool  m_VisibleThisFrame;

	// cache
	DWORD              m_ScreenPlayerVisibleCount;	// is this aspect visible to any actors belonging to the screen player?
	mutable matrix_3x3 m_WobbOrientCache;			// world space oriented bounding box cache - orientation
	mutable vector_3   m_WobbCenterCache;			// world space oriented bounding box cache - center
	mutable vector_3   m_WobbHalfDiagCache;			// world space oriented bounding box cache - half diagonal

	static DWORD     GoAspectToNet( GoAspect* x );
	static GoAspect* NetToGoAspect( DWORD d, FuBiCookie* cookie );

	SET_NO_COPYING( GoAspect );
	FUBI_RPC_CLASS( GoAspect, GoAspectToNet, NetToGoAspect, "" );
};

MAKE_ENUM_BIT_OPERATORS( GoAspect::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOASPECT_H

//////////////////////////////////////////////////////////////////////////////
