//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoAspect.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoAspect.h"

#include "ContentDb.h"
#include "FileSys.h"
#include "FileSysUtils.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoEdit.h"
#include "GoInventory.h"
#include "GoShmo.h"
#include "GoSupport.h"
#include "NamingKey.h"
#include "Nema_AspectMgr.h"
#include "Nema_Blender.h"
#include "RapiImage.h"
#include "ReportSys.h"
#include "Rules.h"
#include "Services.h"
#include "Siege_Camera.h"
#include "Siege_Engine.h"
#include "Siege_Mouse_Shadow.h"
#include "Siege_Options.h"
#include "WorldOptions.h"
#include "WorldTerrain.h"
#include "WorldTime.h"

DECLARE_GO_COMPONENT( GoAspect, "aspect" );

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_POINTERCLASS_TRAITS_EX( nema::HAspect, NemaAspect, nema::HAspect::Null );

//////////////////////////////////////////////////////////////////////////////
// class GoAspect implementation

GoAspect :: GoAspect( Go* parent )
	: Inherited( parent )
	, m_WobbOrientCache  ( DoNotInitialize )
	, m_WobbCenterCache  ( DoNotInitialize )
	, m_WobbHalfDiagCache( DoNotInitialize )
{
	// init spec
	m_SelectionIndicatorScale = 1;
	m_RenderScaleBase         = 1;
	m_RenderScaleMultiplier   = 1;
	m_BoundingVolumeScale     = 1;
	m_UseRange				  = 0;

	// init state
	m_Options              = OPTION_NONE;
	m_CurrentAspectPtr     = NULL;
	m_GoldValue            = 0;
	m_ExperienceValue	   = 0;
	m_ExperienceRemaining  = 0;
	m_ExperienceBenefactor = GOID_INVALID;
	m_LifeState            = LS_IGNORE;
	m_bIsRespawningAsGhost = false;
	m_MaxLife              = 0;
	m_Life                 = 0;
	m_LifeRecoveryUnit     = 1;
	m_LifeRecoveryPeriod   = 10;
	m_MegamapIconTex       = 0;
	m_ComplexShadowTex     = 0;
	m_Mana				   = 0;
	m_RegenSyncPeriod      = 0;
	m_NonGhostAlpha		   = 255;
	m_GhostTimeout		   = 0;
	m_VisibleThisFrame	   = false;

	// cache
	m_ScreenPlayerVisibleCount = 0;
}

GoAspect :: GoAspect( const GoAspect& source, Go* parent )
	: Inherited           ( source, parent              )
	, m_MaxMana           ( source.m_MaxMana            )
	, m_ManaRecoveryUnit  ( source.m_ManaRecoveryUnit   )
	, m_ManaRecoveryPeriod( source.m_ManaRecoveryPeriod )
	, m_WobbOrientCache   ( DoNotInitialize )
	, m_WobbCenterCache   ( DoNotInitialize )
	, m_WobbHalfDiagCache ( DoNotInitialize )
{
	std::copy( source.m_DynamicTextures, ARRAY_END( source.m_DynamicTextures ), m_DynamicTextures );

	// copy spec
	m_SelectionIndicatorScale	= source.m_SelectionIndicatorScale;
	m_RenderScaleBase			= source.m_RenderScaleBase;
	m_RenderScaleMultiplier		= source.m_RenderScaleMultiplier;
	m_BoundingVolumeScale		= source.m_BoundingVolumeScale;
	m_UseRange					= source.m_UseRange;

	// copy state
	m_Options					= source.m_Options;
	m_GoldValue					= source.m_GoldValue;
	m_ExperienceValue			= source.m_ExperienceValue;
	m_ExperienceRemaining		= source.m_ExperienceRemaining;
	m_ExperienceBenefactor		= source.m_ExperienceBenefactor;
	m_ExperienceBenefactorSkill	= source.m_ExperienceBenefactorSkill;
	m_LifeState					= source.m_LifeState;
	m_bIsRespawningAsGhost		= source.m_bIsRespawningAsGhost;
	m_MaxLife					= source.m_MaxLife;
	m_Life						= source.m_Life;
	m_LifeRecoveryUnit			= source.m_LifeRecoveryUnit;
	m_LifeRecoveryPeriod		= source.m_LifeRecoveryPeriod;
	m_Mana						= source.m_Mana;
	m_GhostTimeout				= source.m_GhostTimeout;
	m_VisibleThisFrame			= source.m_VisibleThisFrame;

	// copy megamap icon
	m_MegamapIconTex          = source.m_MegamapIconTex;
	if( (m_MegamapIconTex != 0) && (m_MegamapIconTex != 0xFFFFFFFF) )
	{
		gDefaultRapi.AddTextureReference( m_MegamapIconTex );
	}

	// Clear complex shadow
	m_ComplexShadowTex        = 0;

	// init cache
	m_ScreenPlayerVisibleCount = 0;
	m_RegenSyncPeriod		   = 0;

	// cloning aspects that are in a nema relationship not allowed
	gpassert( source.IsNemaRoot() );

	// take the current aspect
	nema::HAspect sourceAspect = source.GetAspectHandle();
	m_CurrentAspect = gAspectStorage.LoadAspect( gAspectStorage.GetAspectName( sourceAspect ),
												 sourceAspect->GetDebugName() );
	m_CurrentAspectPtr = m_CurrentAspect.Get();

	// copy textures over from clone source
	m_CurrentAspectPtr->CopyTextures( *sourceAspect );

	// take the native aspect if it's not the current
	sourceAspect = source.m_NativeAspect;
	if ( sourceAspect )
	{
		m_NativeAspect = gAspectStorage.LoadAspect( gAspectStorage.GetAspectName( sourceAspect ),
													sourceAspect->GetDebugName() );
		m_NativeAspect->SetGoid( GetGoid() );

		// copy textures over from clone source
		m_NativeAspect->CopyTextures( *sourceAspect );
	}
}

GoAspect :: ~GoAspect( void )
{

	// parent should have already been cleared when body unlinked
	gpassert( IsNemaRoot() );

	// free resources
	gAspectStorage.ReleaseAspect( GetAspectHandle(), true );
	if ( m_NativeAspect )
	{
		gAspectStorage.ReleaseAspect( m_NativeAspect, true );
	}

	// free megamap icon
	if( (m_MegamapIconTex != 0) && (m_MegamapIconTex != 0xFFFFFFFF) )
	{
		gDefaultRapi.DestroyTexture( m_MegamapIconTex );
	}

	// free complex shadow texture
	if( m_ComplexShadowTex )
	{
		gDefaultRapi.DestroyTexture( m_ComplexShadowTex );
	}
}

#if !GP_RETAIL

void GoAspect :: Dump( ReportSys::Context * ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	ctx->Indent();

	ctx->OutputF( "DoesBlockCamera	= %d	", 	GetDoesBlockCamera()  );	ctx->OutputF( "IsGhostUsable  	= %d\n", 	GetIsGhostUsable()    );
	ctx->OutputF( "DoesBlockPath 	= %d	", 	GetDoesBlockPath()    );	ctx->OutputF( "IsInvincible  	= %d\n", 	GetIsInvincible()     );
	ctx->OutputF( "DynamicallyLit	= %d	", 	GetDynamicallyLit()   );	ctx->OutputF( "IsSelectable   	= %d\n", 	GetIsSelectable()     );
	ctx->OutputF( "ForceNoRender 	= %d	", 	GetForceNoRender()    );	ctx->OutputF( "IsUsable       	= %d\n", 	GetIsUsable()         );
	ctx->OutputF( "HasBlockedPath	= %d	", 	GetHasBlockedPath()   );	ctx->OutputF( "IsVisible     	= %d\n", 	GetIsVisible()        );
	ctx->OutputF( "InterestOnly  	= %d	", 	GetInterestOnly()     );	ctx->OutputF( "MegamapOrient  	= %d\n", 	GetMegamapOrient()    );
	ctx->OutputF( "IsCollidable  	= %d	", 	GetIsCollidable()     );	ctx->OutputF( "MegamapOverride	= %d\n", 	GetMegamapOverride()  );
	ctx->OutputF( "IsGagged       	= %d	", 	GetIsGagged()         );	ctx->OutputF( "ShouldDrawShadow = %d\n", 	GetShouldDrawShadow() );

	ctx->Outdent();
}

#endif // !GP_RETAIL

void GoAspect :: SetDoesBlockPath( bool set )
{
	if ( GetDoesBlockPath() != set )
	{
		if( GetDoesBlockPath() && !set )
		{
			SetOptions( OPTION_HAS_BLOCKED_PATH, true );
		}

		SetOptions( OPTION_DOES_BLOCK_PATH, set );

		if ( GetGo()->IsInAnyWorldFrustum() )
		{
			if ( set )
			{
				gWorldTerrain.RequestObjectBlock( GetGoid() );
			}
			else
			{
				gWorldTerrain.RequestObjectUnblock( GetGoid() );
			}
		}
	}
}

bool GoAspect :: GetIsVisible( void ) const
{
	return ( TestOptions( OPTION_IS_VISIBLE ) );
}

bool GoAspect :: GetIsScreenPlayerVisible( void ) const
{
	if ( !GetIsVisible() )
	{
		return false;
	}
	return ( m_ScreenPlayerVisibleCount == gServer.GetScreenPlayerVisibleCount() );
}

void GoAspect :: SSetIsVisible( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetIsVisible( set );
}

void GoAspect :: RCSetIsVisible( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetIsVisible, RPC_TO_ALL );

	SetIsVisible( set );
}

void GoAspect :: SetIsVisible( bool set )
{
	if ( set != GetIsVisible() )
	{
		GetGo()->SetBucketDirty();
		SetOptions( OPTION_IS_VISIBLE, set );

		if ( GetGo()->IsOmni() || GetGo()->IsInAnyWorldFrustum() )
		{
			if ( !Services::IsEditor() )
			{
				if ( set )
				{
					// Reconstitute immediately
					GetAspectHandle()->Reconstitute( 0, true );

					if( GetDoesBlockPath() )
					{
						gWorldTerrain.RequestObjectBlock( GetGoid() );
					}
				}
				else
				{
					if( GetDoesBlockPath() )
					{
						SetDoesBlockPath( false );
					}
					else if( !GetGo()->IsFading() )
					{
						GetAspectHandle()->Purge();
					}
				}
			}
		}
	}
}

void GoAspect :: SSetIsSelectable( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetIsSelectable( set );
}

void GoAspect :: RCSetIsSelectable( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetIsSelectable, RPC_TO_ALL );

	SetIsSelectable( set );
}

void GoAspect :: SetIsSelectable( bool set )
{
	SetOptions( OPTION_IS_SELECTABLE, set );

	if ( !set )
	{
		GetGo()->Deselect();
	}
}

void GoAspect :: SSetIsUsable( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetIsUsable( set );
}

void GoAspect :: RCSetIsUsable( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetIsUsable, RPC_TO_ALL );

	SetIsUsable( set );
}

void GoAspect :: SetIsUsable( bool set )
{
	if ( set != GetIsUsable() )
	{
		GetGo()->SetBucketDirty();
		SetOptions( OPTION_IS_USABLE, set );
	}
}

void GoAspect :: SSetIsGhostUsable( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetIsGhostUsable( set );
}

void GoAspect :: RCSetIsGhostUsable( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetIsGhostUsable, RPC_TO_ALL );

	SetIsGhostUsable( set );
}

void GoAspect :: SetIsGhostUsable( bool set )
{
	if ( set != GetIsGhostUsable() )
	{
		GetGo()->SetBucketDirty();
		SetOptions( OPTION_IS_GHOST_USABLE, set );
	}
}

void GoAspect :: SSetFreezeMeshFlag( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetFreezeMeshFlag( set );
}

void GoAspect :: RCSetFreezeMeshFlag( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetFreezeMeshFlag, RPC_TO_ALL );

	SetFreezeMeshFlag( set );
}

void GoAspect :: SetFreezeMeshFlag( bool set )
{
	GetGo()->SetBucketDirty();
	GetAspectPtr()->SetFreezeMeshFlag( set );
}

void GoAspect :: SSetLockMeshFlag( bool set )
{
	CHECK_SERVER_ONLY;

	RCSetLockMeshFlag( set );
}

void GoAspect :: RCSetLockMeshFlag( bool set )
{
	FUBI_RPC_THIS_CALL( RCSetLockMeshFlag, RPC_TO_ALL );

	SetLockMeshFlag( set );
}

void GoAspect :: SetLockMeshFlag( bool set )
{
	GetGo()->SetBucketDirty();
	GetAspectPtr()->SetLockMeshFlag( set );
}

void GoAspect :: RCFadeOut( void )
{
	FUBI_RPC_THIS_CALL( RCFadeOut, RPC_TO_ALL );

	GoFader::FadeAspect( GetGoid() );
}

#if	!GP_RETAIL
float GoAspect :: GetDisplayCost( void ) const
{
	static AutoConstQuery <float> s_Query( "display_cost" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}
#endif  // !GP_RETAIL

float GoAspect :: GetLowerLodfi( void ) const
{
	static AutoConstQuery <float> s_Query( "lodfi_lower" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

float GoAspect :: GetUpperLodfi( void ) const
{
	static AutoConstQuery <float> s_Query( "lodfi_upper" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoAspect :: GetMaterial( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "material" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

DWORD GoAspect :: GetGhostAlpha( void ) const
{
	static AutoConstQuery <DWORD> s_Query( "ghost_alpha" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

DWORD GoAspect :: GetCamFadeAlpha( void ) const
{
	static AutoConstQuery <DWORD> s_Query( "cam_fade_alpha" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoAspect :: GetRolloverHighlight( void ) const
{
	static AutoConstQuery <bool> s_Query( "rollover_highlight" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoAspect :: GetTemplateIsCollidable( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_collidable" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoAspect :: ClearExperienceBenefactor( void )
{
	m_ExperienceBenefactor		= GOID_INVALID;
	m_ExperienceBenefactorSkill = gpstring::EMPTY;
}

const gpstring& GoAspect :: GetExpiredTemplateName( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "expired_template_name" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoAspect :: GetMegamapIcon( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "megamap_icon" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

nema::HAspect GoAspect :: GetAspectHandle( void ) const
{
	gpassert( m_CurrentAspect );
	return ( m_CurrentAspect );
}

nema::Aspect* GoAspect :: GetAspectPtr( void ) const
{
	gpassert( m_CurrentAspectPtr != NULL );
	return ( m_CurrentAspectPtr );
}

bool GoAspect :: HasAspectHandle( void ) const
{
	return ( !m_CurrentAspect.IsNull() );
}

bool GoAspect :: IsNemaRoot( void ) const
{
	return ( GetAspectPtr()->GetParent().IsNull() );
}

void GoAspect :: ClearNemaParent( void ) const
{
	nema::HAspect parent = GetAspectPtr()->GetParent();
	if ( parent )
	{
		parent->DetachChild( GetAspectHandle() );
	}
}

void GoAspect :: SSetLifeState( eLifeState state, bool forceUpdate )
{
	CHECK_SERVER_ONLY;

	if ( forceUpdate || (state != GetLifeState()) )
	{
		RCSetLifeState( state );
	}
}

void GoAspect :: RCSetLifeState( eLifeState state )
{
	FUBI_RPC_THIS_CALL( RCSetLifeState, RPC_TO_ALL );

	SetLifeState( state );
}

void GoAspect :: SetLifeState( eLifeState state )
{
	if ( m_LifeState != state )
	{
		GetGo()->SetBucketDirty();
		eLifeState oldLifeState = m_LifeState;
		m_LifeState = state;

		if ( IsAlive( m_LifeState ) )
		{
			SetIsCollidable( GetTemplateIsCollidable() );

			if ( oldLifeState == LS_GHOST && GetGo()->HasInventory() )
			{
				GetGo()->GetInventory()->SetInventoryDirty();
			}	
		}
		else if ( m_LifeState == LS_GHOST )
		{
			// this is the same thing as getting killed
			if ( ::IsServerLocal() && ::IsAlive( oldLifeState ) )
			{
				WorldMessage( WE_KILLED, GOID_INVALID, GetGoid(), false ).Send();
			}

			// ghosts get full up on these
			SetCurrentLife( GetMaxLife() );
			SetCurrentMana( GetMaxMana() );

			// start our timer
			if ( ::IsServerLocal() )
			{
				SResetGhostTimeout();
			}
		}
		else if ( state != LS_IGNORE )
		{
			HandleKilled();
		}
	}
}

void GoAspect :: HandleKilled( void )
{
	// deselect automatically
	GetGo()->Deselect();

	// no longer interactive
	SetIsCollidable( false );
	SetDoesBlockPath( false );

	// stop all effects from running on me except ones spawned by this message!
	gGoDb.StopEffectScriptsExcept( GetGo(), WE_KILLED );
}

float GoAspect :: GetGhostTimeout( void ) const
{
	return (float)( max_t( 0.0, PreciseSubtract( m_GhostTimeout, gWorldTime.GetTime() ) ) );
}

void GoAspect :: SResetGhostTimeout()
{
	CHECK_SERVER_ONLY;

	m_GhostTimeout = PreciseAdd( gServer.GetGhostTimeout(), gWorldTime.GetTime() );

	RCSetGhostTimeout( m_GhostTimeout );
}

void GoAspect :: RCSetGhostTimeout( double timeout )
{
	FUBI_RPC_THIS_CALL( RCSetGhostTimeout, RPC_TO_ALL );

	m_GhostTimeout = timeout;
}

#if !GP_RETAIL
FuBiCookie GoAspect :: RSSetGoldValue( int value )
{
	FUBI_RPC_TAG();

	//set local value immediately
	SetGoldValue( value );

	FUBI_RPC_THIS_CALL_RETRY( RSSetGoldValue, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	// $$ security

	SSetGoldValue( value, false );
	return ( RPC_SUCCESS );
}
#endif

void GoAspect :: SSetGoldValue( int value, bool bForceRC )
{
	CHECK_SERVER_ONLY;

	// set local value
	SetGoldValue( value );

	// as long as this is trading gold its ok to set value
	// otherwise leave value alone because it is random item
	if ( GetGo()->HasGold() && GetGo()->GetParent() )
	{
		Player * player = GetGo()->GetParent()->GetPlayer();

		if ( player )
		{
			if ( player->GetTradeGold() == GetGo()->GetGoid() )
			{
				RCSetGoldValue( value );
				bForceRC = false;
			}
		}
	}

	if ( bForceRC )
	{
		RCSetGoldValue( value );
	}
}

FuBiCookie GoAspect :: RCSetGoldValue( int value )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetGoldValue, RPC_TO_ALL );

	// $$ security

	SetGoldValue( value );

	return ( RPC_SUCCESS );
}

void GoAspect :: SetGoldValue( int value )
{
	if ( m_GoldValue != value )
	{
		GetGo()->SetBucketDirty();
		m_GoldValue = value;
	}

	GoGold* gold = GetGo()->QueryGold();
	if ( gold != NULL )
	{
		gold->SetGoldValueDirty();
	}
}

void GoAspect :: SetCurrentAspect( nema::HAspect newAspect, bool transferInternals )
{
	gpassert( newAspect );

	if (m_CurrentAspectPtr)
	{
		newAspect->MatchPurgeStatus(m_CurrentAspectPtr,GetGo()->IsInAnyScreenWorldFrustum());
		if ( transferInternals )
		{
			m_CurrentAspectPtr->temporaryTransferInternals( newAspect );
		}
	}

	if ( m_NativeAspect )
	{
		gAspectStorage.ReleaseAspect( m_CurrentAspect, true );
		m_CurrentAspect = newAspect;
	}
	else
	{
		m_NativeAspect = m_CurrentAspect;
		m_CurrentAspect = newAspect;
		if (m_NativeAspect != m_CurrentAspect)
		{
			m_NativeAspect->Purge();
		}
	}

	m_CurrentAspect->SetGoid( GetGoid() );
	m_CurrentAspectPtr = m_CurrentAspect.Get();
}

void GoAspect :: SetCurrentAspectSimple( nema::HAspect newAspect )
{
	gpassert( newAspect );

	if ( m_CurrentAspect )
	{
		newAspect->MatchPurgeStatus(m_CurrentAspectPtr,GetGo()->IsInAnyScreenWorldFrustum());
		gpassert( IsNemaRoot() && IsCurrentAspectNative() );
		gAspectStorage.ReleaseAspect( m_CurrentAspect, true );
	}

	m_CurrentAspect = newAspect;
	m_CurrentAspect->SetGoid( GetGoid() );
	m_CurrentAspectPtr = m_CurrentAspect.Get();
}

void GoAspect :: SetCurrentAspectSimple( const char* aspectName, const char* textureName )
{
	gpassert( aspectName != NULL );

	// update the aspect if it's actually different
	if ( !m_CurrentAspect || !FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( m_CurrentAspect ) ).same_no_case( aspectName ) )
	{
		// set it
		gpstring aspectFile;
		if ( gNamingKey.BuildASPLocation( aspectName, aspectFile ) )
		{
			nema::HAspect newAspect = gAspectStorage.LoadAspect( aspectFile, aspectName );
			if ( newAspect )
			{
				if ( newAspect->IsDeformable() )
				{
					// special handling for bodies - must xfer internals!
					SetCurrentAspect( newAspect, true );
				}
				else
				{
					// just replace the aspect only
					SetCurrentAspectSimple( newAspect );
				}
			}
		}
	}

	// update the texture
	SetCurrentAspectTexture( 0, textureName );
}

void GoAspect :: SetNativeAspect( nema::HAspect newAspect )
{
	gpassert( newAspect );

	// don't do it on if linked!
	gpassert( IsNemaRoot() );

	if ( IsCurrentAspectNative() )
	{
		newAspect->MatchPurgeStatus(m_NativeAspect.Get(),GetGo()->IsInAnyScreenWorldFrustum());
		gAspectStorage.ReleaseAspect( m_NativeAspect, true );
		m_NativeAspect = newAspect;
		m_NativeAspect->SetGoid( GetGoid() );
	}
	else
	{
		gpassert( m_CurrentAspect );
		gAspectStorage.ReleaseAspect( m_CurrentAspect, true );
		m_CurrentAspect = newAspect;
		m_CurrentAspect->SetGoid( GetGoid() );
		m_CurrentAspectPtr = m_CurrentAspect.Get();
	}
}

void GoAspect :: RestoreNativeAspect( bool transferInternals )
{

	if ( m_NativeAspect )
	{

		m_NativeAspect->MatchPurgeStatus(m_CurrentAspectPtr,GetGo()->IsInAnyScreenWorldFrustum());

		if ( transferInternals )
		{
			m_CurrentAspect->temporaryTransferInternals( m_NativeAspect );
		}

		gAspectStorage.ReleaseAspect( m_CurrentAspect, true );
		m_CurrentAspect = m_NativeAspect;
		m_CurrentAspect->SetGoid( GetGoid() );
		m_CurrentAspectPtr = m_CurrentAspect.Get();
		m_NativeAspect = nema::HAspect();
	}
}

void GoAspect :: RSSetCurrentAspectTexture( int texIndex, const char* textureName )
{
	FUBI_RPC_THIS_CALL( RSSetCurrentAspectTexture, RPC_TO_SERVER );

	SSetCurrentAspectTexture( texIndex, textureName );
}

void GoAspect :: SSetCurrentAspectTexture( int texIndex, const char* textureName )
{
	CHECK_SERVER_ONLY;

	RCSetCurrentAspectTexture( texIndex, textureName );
}

void GoAspect :: RCSetCurrentAspectTexture( int texIndex, const char* textureName )
{
	FUBI_RPC_THIS_CALL( RCSetCurrentAspectTexture, RPC_TO_ALL );

	// Call the basic function
	SetCurrentAspectTexture( texIndex, textureName );
}

const char* GoAspect :: GetCurrentAspectTexture( int texIndex )
{
	const char* texName = "";

	DWORD tex = m_CurrentAspectPtr->GetTexture( texIndex );
	if ( tex != 0 )
	{
		texName = gDefaultRapi.GetTexturePathname( tex );
		if ( texName == NULL )
		{
			texName = "";
		}
	}

	return ( texName );
}

void GoAspect :: SetCurrentAspectTexture( int texIndex, const char* textureName )
{
	// $$$$ somebody stop this copy-paste madness! uber-resource mgr will fix this problem -sb

	// resolve to a file
	gpstring texName( (textureName && *textureName) ? textureName : m_CurrentAspectPtr->GetDefaultTextureString( 0 ) );
	gpstring texFile;
	if (   !gNamingKey.BuildIMGLocation( texName, texFile )
		|| !FileSys::DoesResourceFileExist( texFile ) )
	{
#				if ( !GP_RETAIL )
		{
			gpwarningf(( "Placeholder texture substitution: illegal texture '%s' for aspect '%s' in Go at '%s'\n",
						 texName.c_str(), gAspectStorage.GetAspectName( m_CurrentAspect ).c_str(),
						 GetGo()->DevGetFuelAddress().c_str() ));
		}
#				endif // !GP_RETAIL

		texName = gContentDb.GetDefaultTextureName();
		if (   !gNamingKey.BuildIMGLocation( texName, texFile )
			|| !FileSys::DoesResourceFileExist( texFile ) )
		{
			gperrorf(( "Can't find placeholder texture '%s'! This is serious!\n", texName.c_str() ));
			texFile.clear();
		}
	}

	// set texture
	m_CurrentAspectPtr->SetTextureFromFile( texIndex, texFile );
}

void GoAspect :: SSetAspectTextureSpeed( int texIndex, const float texSpeed )
{
	CHECK_SERVER_ONLY;

	RCSetAspectTextureSpeed( texIndex, texSpeed );
}

void GoAspect :: RCSetAspectTextureSpeed( int texIndex, const float texSpeed )
{
	FUBI_RPC_THIS_CALL( RCSetAspectTextureSpeed, RPC_TO_ALL );

	// Call the basic function
	SetAspectTextureSpeed( texIndex, texSpeed );
}

float GoAspect :: GetAspectTextureSpeed( int texIndex )
{
	DWORD tex = m_CurrentAspectPtr->GetTexture( texIndex );
	if ( tex != 0 )
	{
		// Return the speed of this texture
		return gDefaultRapi.GetTextureAnimationSpeed( tex );
	}

	return ( 0.0f );
}

void GoAspect :: SetAspectTextureSpeed( int texIndex, const float texSpeed )
{
	// Get the texture
	DWORD tex = m_CurrentAspectPtr->GetTexture( texIndex );
	if ( tex != 0 )
	{
		gDefaultRapi.SetTextureAnimationSpeed( tex, texSpeed );
	}
	else
	{
		gperror( "RCSetAspectTextureSpeed called on a texture that is not valid!" );
	}
}

void GoAspect :: SSetCurrentLife( float life )
{
	CHECK_SERVER_ONLY;
	RCSetCurrentLife( life );
}

void GoAspect :: RCSetCurrentLife( float life )
{
	FUBI_RPC_THIS_CALL( RCSetCurrentLife, RPC_TO_ALL );

	SetCurrentLife( life );
}

void GoAspect :: SetCurrentLife( float life )
{
	if ( !::IsEqual( m_Life, life ) )
	{
		GetGo()->SetBucketDirty();
		m_Life = life;
	}
}

void GoAspect :: SSetMaxLife( float life )
{
	CHECK_SERVER_ONLY;
	RCSetMaxLife( life );
}

void GoAspect :: RCSetMaxLife( float maxLife )
{
	FUBI_RPC_THIS_CALL( RCSetMaxLife, RPC_TO_ALL );
	SetMaxLife( maxLife );
}

void GoAspect :: SSetNaturalMaxLife( float maxLife )
{
	CHECK_SERVER_ONLY;
	RCSetNaturalMaxLife( maxLife );
}

void GoAspect :: RCSetNaturalMaxLife( float maxLife )
{
	FUBI_RPC_THIS_CALL( RCSetNaturalMaxLife, RPC_TO_ALL );
	SetNaturalMaxLife( maxLife );
}

void GoAspect :: RCSyncState( float life, float mana )
{
	FUBI_RPC_THIS_CALL( RCSyncState, RPC_TO_ALL );
	SetCurrentLife( life );
	SetCurrentMana( mana );
}

void GoAspect :: SetLifeRecoveryPeriod( float val )
{
	if ( m_LifeRecoveryPeriod != val )
	{
		m_LifeRecoveryPeriod = val;
		DetectWantsBits();
	}
}

void GoAspect :: SetNaturalLifeRecoveryPeriod( float val )
{
	if ( m_LifeRecoveryPeriod.GetNatural() != val )
	{
		m_LifeRecoveryPeriod.SetNatural( val );
		GetGo()->SetModifiersDirty();
		DetectWantsBits();
	}
}

void GoAspect :: SSetCurrentMana( float mana )
{
	CHECK_SERVER_ONLY;
	RCSetCurrentMana( mana );
}

void GoAspect :: RCSetCurrentMana( float mana )
{
	FUBI_RPC_THIS_CALL( RCSetCurrentMana, RPC_TO_ALL );

	SetCurrentMana( mana );
}

void GoAspect :: SetCurrentMana( float mana )
{
	if ( !::IsEqual( m_Mana, mana ) )
	{
		GetGo()->SetBucketDirty();
		m_Mana = mana;
	}
}

void GoAspect :: SSetMaxMana( float maxMana )
{
	CHECK_SERVER_ONLY;
	RCSetMaxMana( maxMana );
}

void GoAspect :: RCSetMaxMana( float maxMana )
{
	FUBI_RPC_THIS_CALL( RCSetMaxMana, RPC_TO_ALL );
	SetMaxMana( maxMana );
}

void GoAspect :: SSetNaturalMaxMana( float maxMana )
{
	CHECK_SERVER_ONLY;
	RCSetNaturalMaxMana( maxMana );
}

void GoAspect :: RCSetNaturalMaxMana( float maxMana )
{
	FUBI_RPC_THIS_CALL( RCSetNaturalMaxMana, RPC_TO_ALL );
	SetNaturalMaxMana( maxMana );
}

void GoAspect :: SetManaRecoveryPeriod( float val )
{
	if ( m_ManaRecoveryPeriod != val )
	{
		m_ManaRecoveryPeriod = val;
		DetectWantsBits();
	}
}

void GoAspect :: SetNaturalManaRecoveryPeriod( float val )
{
	if ( m_ManaRecoveryPeriod.GetNatural() != val )
	{
		m_ManaRecoveryPeriod.SetNatural( val );
		GetGo()->SetModifiersDirty();
		DetectWantsBits();
	}
}

void GoAspect :: RCSetDynamicTexture( ePlayerSkin skin, const char* name )
{
	FUBI_RPC_THIS_CALL( RCSetDynamicTexture, RPC_TO_ALL );

	SetDynamicTexture( skin, name );
}

void GoAspect :: SSetDynamicTexture( ePlayerSkin skin, const char* name )
{
	CHECK_SERVER_ONLY;

	RCSetDynamicTexture( skin, name );
}

void GoAspect :: SetDynamicTexture( ePlayerSkin skin, const char* name, bool force )
{
	gpassert( name != NULL );
	GoString& dynamicTexture = m_DynamicTextures[ skin ];

	// $ early bailout if no change
	if ( dynamicTexture.GetString().same_no_case( name ) && !force )
	{
		return;
	}

	// take it
	dynamicTexture = name;

	// ordinary texture?
	if ( dynamicTexture.GetString().find( ',' ) == gpstring::npos )
	{
		// easy, just use it
		gpstring textureFile;
		if ( gNamingKey.BuildIMGLocation( dynamicTexture.c_str(), textureFile ) )
		{
			if ( m_NativeAspect && ( m_NativeAspect != m_CurrentAspect ) )
			{
				m_NativeAspect->SetTextureFromFile( skin, textureFile );
				if (skin == PS_FLESH)
				{
					m_CurrentAspectPtr->SetTextureFromFile( skin, textureFile );
				}
			}
			else
			{
				m_CurrentAspectPtr->SetTextureFromFile( skin, textureFile );
			}
		}
	}
	else
	{
		RapiMemImage* dest = new RapiMemImage;
		int textureCount = 0;

		// $ have to construct a layered texture
		stringtool::Extractor extractor( ",", dynamicTexture.c_str() );
		gpstring subtexName;
		while ( extractor.GetNextString( subtexName ) )
		{
			gpstring textureFile;
			if ( gNamingKey.BuildIMGLocation( subtexName, textureFile ) )
			{
				if ( textureCount == 0 )
				{
					if ( !dest->Init( textureFile ) )
					{
						continue;
					}
				}
				else
				{
					// get this subtexture
					RapiMemImage image;
					if ( !image.Init( textureFile ) )
					{
						continue;
					}

					// blend textures
					if (   (image.GetWidth () == dest->GetWidth ())
						&& (image.GetHeight() == dest->GetHeight()) )
					{
						BYTE *pDest  = (BYTE *)dest->GetBits();
						BYTE *pImage = (BYTE *)image.GetBits();
						float R, G, B, A;
						float R1, G1, B1, A1;
						float R2, G2, B2, A2;

						for ( int y = 0; y < dest->GetHeight(); ++y )
						{
							for ( int x = 0; x < dest->GetWidth(); ++x )
							{
								A1 = ((float)pDest[3] / 255);
								R1 = ((float)pDest[2] / 255);
								G1 = ((float)pDest[1] / 255);
								B1 = ((float)pDest[0] / 255);

								A2 = ((float)pImage[3] / 255);
								R2 = ((float)pImage[2] / 255);
								G2 = ((float)pImage[1] / 255);
								B2 = ((float)pImage[0] / 255);

								A = 1.0f;
								R = (R2 * A2) + (R1 * (1.0f - A2));
								G = (G2 * A2) + (G1 * (1.0f - A2));
								B = (B2 * A2) + (B1 * (1.0f - A2));

								pDest[3] = (BYTE)(A * 255);
								pDest[2] = (BYTE)(R * 255);
								pDest[1] = (BYTE)(G * 255);
								pDest[0] = (BYTE)(B * 255);

								pDest += 4;
								pImage += 4;

								/*
								*pDest = InterpolateColor( *pDest, *pImage, (float)((BYTE*)pImage)[0] / 255 );
								((BYTE*)pImage)[0] = 255;
								++pDest;
								++pImage;
								*/
							}
						}
					}
				}

				++textureCount;
			}
		}

		// create algo texture
		unsigned int newTexture = gDefaultRapi.CreateAlgorithmicTexture(
				gpstringf( "'%s' skin %d", GetGo()->GetTemplateName(), skin ),
				dest, true, 0, TEX_LOCKED, false, true );
		if ( m_NativeAspect && ( m_NativeAspect != m_CurrentAspect ) )
		{
			m_NativeAspect->SetTextureCreatedByExternalSource( skin, newTexture );
			if (skin == PS_FLESH)
			{
				m_CurrentAspectPtr->SetTextureCreatedByExternalSource( skin, newTexture );
			}
		}
		else
		{
			m_CurrentAspectPtr->SetTextureCreatedByExternalSource( skin, newTexture );
		}

		gDefaultRapi.DestroyTexture( newTexture );

	}

	if (skin == PS_FLESH)
	{
		GetGo()->GetInventory()->UpdateCustomHeadTexture();
	}
}

gpstring GoAspect :: FindEventVoice( const char* event, gpstring& pri ) const
{
	if_not_gpassert( (event != NULL) && (*event != '\0') )
	{
		return ( gpstring::EMPTY );
	}

	FastFuelHandle voice = GetData()->GetInternalField( "voice" );
	if ( !voice )
	{
		return ( gpstring::EMPTY );
	}

	FastFuelHandle evt = voice.GetChildNamed( event );

	if ( !evt )
	{
		return ( gpstring::EMPTY );
	}

	int key_sub	= 1;
	FastFuelHandle fevent( evt );
	if( fevent.HasKey( "priority" ) )
	{
		pri		= fevent.GetString( "priority" );
		key_sub	= 2;
	}

	FastFuelFindHandle fh( evt );
	if( !fh.FindFirstValue( "*" ) )
	{
		return ( gpstring::EMPTY );
	}

	gpstring choice;
	int index = Random( fh.GetKeyAndValueCount() - key_sub );
	while ( index-- >= 0 )
	{
		fh.GetNextValue( choice );
	}

	return ( choice );
}

// complex shadow
void GoAspect :: CreateComplexShadowTexture()
{

	if( !m_ComplexShadowTex )
	{
		if( GetShouldDrawShadow() && 
		   (gSiegeEngine.GetOptions().IsExpensiveShadows() ||
		   (GetGo()->IsScreenPartyMember() &&
		    gSiegeEngine.GetOptions().IsExpensivePartyShadows())) )
		{
			m_ComplexShadowTex = gDefaultRapi.CreateShadowTexture();
		}
	}
	else
	{
		if( !gSiegeEngine.GetOptions().IsExpensiveShadows() )
		{
			if( !GetGo()->IsScreenPartyMember() ||
		        !gSiegeEngine.GetOptions().IsExpensivePartyShadows() )
			{
				// free complex shadow texture
				gDefaultRapi.DestroyTexture( m_ComplexShadowTex );
				m_ComplexShadowTex	= 0;
			}
		}
	}
}

// megamap texture
void GoAspect :: CreateMegamapTexture()
{
	gpassert( m_MegamapIconTex == 0 );

	// build megamap icon
	gpstring texName	= GetMegamapIcon();
	if( texName.same_no_case( "<none>" ) )
	{
		m_MegamapIconTex	= 0;
	}
	else if( texName.same_no_case( "<aspect_mesh>" ) )
	{
		m_MegamapIconTex	= 0xFFFFFFFF;
	}
	else
	{
		gpstring texFile;
		if( !gNamingKey.BuildIMGLocation( texName, texFile ) || !FileSys::DoesResourceFileExist( texFile ) )
		{
			gperrorf(( "Could not find megamap texture %s for aspect %s", texName.c_str(), gAspectStorage.GetAspectName( m_CurrentAspect ).c_str() ));
		}
		else
		{
			m_MegamapIconTex	= gDefaultRapi.CreateTexture( texFile, false, 0, TEX_LOCKED );
		}
	}
}

void GoAspect :: PrepareToDrawNow( bool includeTextures )
{
	if ( GetIsVisible() || gWorldOptions.GetShowAll() )
	{
		m_CurrentAspectPtr->Reconstitute( includeTextures ? 0 : 1, true );
	}
}

siege::eCollectFlags GoAspect :: BuildCollectFlags ( void ) const
{
	siege::eCollectFlags flags = siege::CF_NONE;

	flags |= GetGo()->HasActor() ? siege::CF_ACTOR : siege::CF_ITEM;

	if ( GetIsSelectable             () )  {  flags |= siege::CF_SELECTABLE;         }
	if ( GetShouldDrawShadow         () )  {  flags |= siege::CF_DRAW_SHADOW;        }
	if ( GetGo()->IsSelected         () )  {  flags |= siege::CF_SELECTED;           }
	if ( GetDoesBlockCamera          () )  {  flags |= siege::CF_AVOID_CAMERA;       }
	if ( GetIsScreenPlayerVisible    () )  {  flags |= siege::CF_SCREEN_PLAYER_VIS;  }
	if ( GetGo()->IsScreenPartyMember() )  {  flags |= siege::CF_PARTYMEMBER;        }
	if ( GetGo()->IsGhost			 () )  {  flags |= siege::CF_GHOST;				 }
	if ( GetGo()->IsSpawned          () )  {  flags |= siege::CF_SPAWNED;            }
	if ( GetInterestOnly			 () )  {  flags |= siege::CF_INTERESTONLY;		 }
	if ( GetMegamapOverride          () )  {  flags |= siege::CF_MM_OVERRIDE;        }
	if ( GetMegamapOrient            () )  {  flags |= siege::CF_MM_ORIENT;          }
	if ( GetDynamicallyLit			 () )  {  flags |= siege::CF_DYNAMICLIGHT;		 }
	if ( GetGo()->IsFading           () )  {  flags |= siege::CF_FADEIN;             }
	if ( GetForceNoRender			 () )  {  flags |= siege::CF_NORENDER;			 }
	if ( IsAlive( m_LifeState )			)  {  flags |= siege::CF_ALIVE;				 }
	if ( GetGo()->IsFocused			 () )  {  flags |= siege::CF_FOCUSED;			 }

	if ( GetGo()->HasAttack() && IsAmmo( GetGo()->GetAttack()->GetAttackClass() ) )
	{
		flags |= siege::CF_PROJECTILE;
	}

	if ( GetGo()->IsRenderDirty() )
	{
		flags |= siege::CF_POSORIENTCHANGED;
		GetGo()->SetRenderDirty( false );
	}

	if ( GetGo()->IsMouseShadowed() && GetRolloverHighlight() && !gWorldOptions.GetDisableHighlight() )
	{
		bool bHighlight = true;
		if ( (::IsDead( GetLifeState() ) || GetLifeState() == LS_GHOST) && !GetGo()->IsScreenPartyMember() )
		{
			gWorld.RolloverHighlightCallback( bHighlight );
		}
		else if ( !GetGo()->IsScreenPartyMember() && !(GetGo()->IsItem() && GetGo()->HasGui()) && gSiegeEngine.GetMouseShadow().SelectionBox() )
		{
			bHighlight = false;
		}

		if ( bHighlight )
		{
			flags |= siege::CF_HIGHLIGHTED;
		}
	}

	return ( flags );
}

bool GoAspect :: BuildRenderInfo( siege::ASPECTINFO& info, siege::eRequestFlags rFlags ) const
{
	m_VisibleThisFrame = false;
		 
	// get cache
	GoPlacement* placement = GetGo()->GetPlacement();

	// sanity check
#	if !GP_RETAIL
	if ( placement->GetPosition().node != info.pNode->GetGUID() )
	{
		gperrorf((
			"%s is in %s node's occupant list, but thinks he's in %s node",
			GetGo()->GetTemplateName(),
			info.pNode->GetGUID().ToString().c_str(),
			placement->GetPosition().node.ToString().c_str() ));
	}
#	endif // !GP_RETAIL

// Perform render visibility tests.

	// get at the aspect (cache it local)
	nema::Aspect* aspect = GetAspectPtr();

	// skip non-root components - the parent will render this
	if ( !aspect->GetParent().IsNull() )
	{
		return ( false );
	}

	// can we see this occupant?
	if ( !GetForceNoRender() )
	{
		if ( (!GetIsVisible() || !gWorldOptions.GetShowObjects() || GetGo()->GetPlayer()->IsJIP() ) && !gWorldOptions.GetShowAll() )
		{
			return ( false );
		}

#	if !GP_RETAIL
		if ( Services::IsEditor() && !GetGo()->GetEdit()->IsVisible() )
		{
			return ( false );
		}
#	endif // !GP_RETAIL
	}

	// force-reconstitute if we're showing
	if ( gWorldOptions.GetShowAll() )
	{
		ccast <GoAspect*> ( this )->PrepareToDrawNow();
	}

	// am i culled from the view frustum?
	if( (rFlags & siege::RF_BOUNDS) || (rFlags & siege::RF_FRUSTUMCLIP) )
	{
		GetWorldSpaceOrientedBoundingVolume( info.BoundOrient, info.BoundCenter, info.BoundHalfDiag );
	}

	if( rFlags & siege::RF_FRUSTUMCLIP )
	{
		if( gSiegeEngine.GetCamera().GetFrustum().CullObject( info.BoundOrient, info.BoundCenter, info.BoundHalfDiag ) )
		{
			return ( false );
		}
	}

// Fill out the info struct.

	info.Object					= MakeInt( GetGoid() );
	info.Aspect					= aspect;
	info.MegamapIconTex			= m_MegamapIconTex;
	info.ComplexShadowTex		= m_ComplexShadowTex;
	info.CameraFadeAlpha		= GetCamFadeAlpha();

	if( rFlags & siege::RF_POSITION )
	{
		info.Origin				= placement->GetPosition().pos;
	}
	if( rFlags & siege::RF_ORIENT )
	{
		placement->GetOrientation().BuildMatrix( info.Orientation );
	}
	if( rFlags & siege::RF_SCALE )
	{
		info.ObjectScale		= GetRenderScale();
		info.BoundingBodyScale	= m_BoundingVolumeScale * info.ObjectScale;
	}
	if( rFlags & siege::RF_RENDERINDEX )
	{
		info.RenderIndex1		= placement->GetRenderIndexCache1();
		info.RenderIndex2		= placement->GetRenderIndexCache2();
		info.RenderIndex3		= placement->GetRenderIndexCache3();
	}
	if( rFlags & siege::RF_ALPHA )
	{
		info.Alpha = info.Aspect->GetAlpha();

		if ( GetGo()->IsFading() )
		{
			if ( GetGo()->CheckFadingFirstTime() )
			{
				info.Alpha = 0;
			}
			else if ( info.Alpha == 255 )
			{
				// stop fading when siege has made us fully opaque
				GetGo()->StopFading();
			}
		}
	}
	if( rFlags & siege::RF_SCREEN_PLAYER_ALIGN )
	{
		Player * player = gServer.GetScreenPlayer();
		if( player != NULL )
		{
			if( player->GetIsFriend( GetGo() ) )
			{
				info.ScreenPlayerAlign	= siege::AF_FRIEND;
			}
			else if( player->GetIsEnemy( GetGo() ) )
			{
				info.ScreenPlayerAlign	= siege::AF_ENEMY;
			}
			else if( player->GetIsNeutral( GetGo() ) )
			{
				info.ScreenPlayerAlign	= siege::AF_NEUTRAL;
			}
			else
			{
				info.ScreenPlayerAlign	= siege::AF_NONE;
			}
		}
	}
	if( rFlags & siege::RF_CAMERA_DISTANCE )
	{
		info.CameraDistance		= Length( gSiegeEngine.GetCamera().GetCameraPosition() - placement->GetWorldPosition() );
	}
	if( rFlags & siege::RF_PROJ_CENTER )
	{
		if( GetGo()->HasBody() && GetGo()->HasActor() )
		{
			const BoneTranslator& bt = GetGo()->GetBody()->GetBoneTranslator();

			int nema_bone_index = 0;

			vector_3 v,vt;

			bt.Translate( BoneTranslator::BODY_ANTERIOR, nema_bone_index );
			aspect->GetIndexedBonePosition(nema_bone_index,vt);

			bt.Translate( BoneTranslator::BODY_MID, nema_bone_index );
			aspect->GetIndexedBonePosition(nema_bone_index,vt);
			v += vt;

			bt.Translate( BoneTranslator::BODY_POSTERIOR, nema_bone_index );
			aspect->GetIndexedBonePosition(nema_bone_index,vt);
			v += vt;

			v.x *= (1/3.0f);
			v.y = 0;
			v.z *= (1/3.0f);

			// $$$ Mike, put node space projection center here
			info.ProjCenter	= info.Origin + placement->GetOrientation().RotateVector_T(v);
		}
		else
		{
			info.ProjCenter	= info.Origin;
		}
	}

	if ( GetLifeState() == LS_GHOST && info.Alpha != GetGhostAlpha() )
	{
		m_NonGhostAlpha = info.Alpha;
		info.Alpha = GetGhostAlpha();
	}
	else if ( GetLifeState() != LS_GHOST && info.Alpha == GetGhostAlpha() )
	{
		info.Alpha = m_NonGhostAlpha;
	}

	// guess it's visible...
	m_VisibleThisFrame = true;
	return ( true );
}

float GoAspect :: GetBoundingSphereRadius( void ) const
{
	// Aspect now keep the sphere radius -biddle
	return GetAspectPtr()->GetBoundingSphereRadius() * m_BoundingVolumeScale * GetRenderScale();
}

void GoAspect :: GetWorldSpaceOrientedBoundingVolume(
		matrix_3x3* orient, vector_3* center, vector_3* halfDiag,
		const SiegePos& pos, const Quat& quat, nema::Aspect* aspect,
		float boundScale, float renderScale, DWORD bone_index )
{
	// get node space params
	GetNodeSpaceOrientedBoundingVolume( orient, center, halfDiag, pos, quat, aspect, boundScale, renderScale, bone_index );

	// get the node
	siege::SiegeNodeHandle handle = gSiegeEngine.NodeCache().UseObject( pos.node );
	const siege::SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

	// convert center to world space
	if ( center != NULL )
	{
		*center = node.NodeToWorldSpace( *center );
	}

	// adjust the orientation to world space
	if ( orient != NULL )
	{
		matrix_3x3 m( *orient );
		Multiply( *orient, node.GetCurrentOrientation(), m );
	}
}

void GoAspect :: GetNodeSpaceOrientedBoundingVolume(
		matrix_3x3* orient, vector_3* center, vector_3* halfDiag,
		const SiegePos& pos, const Quat& quat, nema::Aspect* aspect,
		float boundScale, float renderScale, DWORD bone_index )
{
	// get the OBB information from the aspect
	Quat aspectQuat( DoNotInitialize );
	vector_3 aspectCenter( DoNotInitialize ), aspectHalfDiag( DoNotInitialize );

	if (aspect->IsPurged(2))
	{
		aspectCenter = aspectHalfDiag = vector_3::ZERO;
		aspectQuat = Quat::IDENTITY;
	}
	else
	{
		if ( (orient == NULL) && (halfDiag == NULL) )
		{
			if( 0xFFFFFFFF == bone_index )
			{
				aspect->GetOrientedBoundingBoxCenter( aspectCenter );
			}
			else
			{
				aspect->GetOrientedBoneBoundingBoxCenter( bone_index, aspectCenter );
			}
		}
		else
		{
			if( 0xFFFFFFFF == bone_index )
			{
				aspect->GetOrientedBoundingBox( aspectQuat, aspectCenter, aspectHalfDiag );
			}
			else
			{
				aspect->GetOrientedBoneBoundingBox( bone_index, aspectQuat, aspectCenter, aspectHalfDiag );
			}
		}
	}

	// convert to node space based on go's placement
	if ( center != NULL )
	{
		*center = pos.pos;
		*center += quat.RotateVector_T( aspectCenter * boundScale * renderScale );
	}

	// set the half diagonal
	if ( halfDiag != NULL )
	{
		*halfDiag = aspectHalfDiag * boundScale * renderScale;
	}

	// optionally set the orientation of the bounding volume
	if ( orient != NULL )
	{
		(quat * aspectQuat).BuildMatrix( *orient );
	}
}

void GoAspect :: ResetTextures( void )
{
	// $ do not call this if current aspect is non-native
	gpassert( IsCurrentAspectNative() );

	// texture list is internal
	FastFuelHandle textures = GetData()->GetInternalField( "textures" );

	// need to load all of the textures for this aspect
	gpstring dummy, texName;
	for ( int i = 0, count = m_CurrentAspectPtr->GetNumSubTextures() ; i != count ; ++i )
	{
		// get the texture name
		gpstring texName;
		if ( !textures || !textures.Get( ::ToString( i ), texName ) )
		{
			texName = m_CurrentAspectPtr->GetDefaultTextureString( i );
		}

		// resolve to a file
		gpstring texFile;
		if (   !gNamingKey.BuildIMGLocation( texName, texFile )
			|| !FileSys::DoesResourceFileExist( texFile ) )
		{
#			if ( !GP_RETAIL )
			{
				gpwarningf(( "Placeholder texture substitution: illegal texture '%s' for aspect '%s' in Go at '%s'\n",
							 texName.c_str(), gAspectStorage.GetAspectName( m_CurrentAspect ).c_str(),
							 GetGo()->DevGetFuelAddress().c_str() ));
			}
#			endif // !GP_RETAIL

			texName = gContentDb.GetDefaultTextureName();
			if (   !gNamingKey.BuildIMGLocation( texName, texFile )
				|| !FileSys::DoesResourceFileExist( texFile ) )
			{
				gperrorf(( "Can't find placeholder texture '%s'! This is serious!\n", texName.c_str() ));
				texFile.clear();
			}
		}

		// set texture
		m_CurrentAspectPtr->SetTextureFromFile( i, texFile );
	}
}

struct GoLessByRank
{
	bool operator () ( const Go* l, const Go* r ) const
	{
		return ( l->GetRootDepth() < r->GetRootDepth() );
	}
};

void GoAspect :: UpdateChildrenPositions( void )
{
	// move all children to exactly where i am
	nema::ChildLinkIter i,
			begin = GetAspectPtr()->GetChildren().begin(), end = GetAspectPtr()->GetChildren().end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (*i).first.IsValid() )
		{
			GoHandle child( (*i).first->GetGoid() );
			if ( child && child->HasPlacement() && !child->GetPlacement()->DoesInheritPlacement() )
			{
				child->GetPlacement()->CopyPlacement( *GetGo()->GetPlacement() );
			}
		}
		else
		{
			gpwarningf((
				"GOASPECT: UpdateChildrenPositions [%s g:%08x s:%08x] can't move mangled child aspect!\n",
				GetGo()->GetTemplateName(),
				GetGo()->GetGoid(),
				GetGo()->GetScid()
				));
		}
	}
}

GoComponent* GoAspect :: Clone( Go* newParent )
{
	return ( new GoAspect( *this, newParent ) );
}

bool GoAspect :: Xfer( FuBi::PersistContext& persist )
{
	bool success = true;

// Xfer public options.

	persist.XferOption( "is_visible",               *this, OPTION_IS_VISIBLE               );
	persist.XferOption( "draw_shadow",              *this, OPTION_DRAW_SHADOW              );
	persist.XferOption( "interest_only",            *this, OPTION_INTEREST_ONLY            );
	persist.XferOption( "draw_selection_indicator", *this, OPTION_DRAW_SELECTION_INDICATOR );
	persist.XferOption( "force_no_render",          *this, OPTION_FORCE_NO_RENDER          );
	persist.XferOption( "is_collidable",            *this, OPTION_IS_COLLIDABLE            );
	persist.XferOption( "does_block_path",          *this, OPTION_DOES_BLOCK_PATH          );
	persist.XferOption( "is_invincible",            *this, OPTION_IS_INVINCIBLE            );
	persist.XferOption( "is_selectable",            *this, OPTION_IS_SELECTABLE            );
	persist.XferOption( "does_block_camera",        *this, OPTION_DOES_BLOCK_CAMERA        );
	persist.XferOption( "is_usable",                *this, OPTION_IS_USABLE                );
	persist.XferOption( "is_ghost_usable",			*this, OPTION_IS_GHOST_USABLE		   );
	persist.XferOption( "is_gagged",                *this, OPTION_IS_GAGGED                );
	persist.XferOption( "megamap_override",			*this, OPTION_MEGAMAP_OVERRIDE         );
	persist.XferOption( "megamap_orient",			*this, OPTION_MEGAMAP_ORIENT           );
	persist.XferOption( "dynamically_lit",			*this, OPTION_DYNAMIC_LIGHT			   );
	persist.XferOption( "virgin_pickup",			*this, OPTION_IS_VIRGIN_PICKUP		   );
	
// Xfer the rest.

	// spec
	persist.Xfer( "experience_value",          m_ExperienceValue         );
	persist.Xfer( "selection_indicator_scale", m_SelectionIndicatorScale );
	persist.Xfer( "scale_base",                m_RenderScaleBase         );
	persist.Xfer( "scale_multiplier",          m_RenderScaleMultiplier   );
	persist.Xfer( "bounding_volume_scale",     m_BoundingVolumeScale     );
	persist.Xfer( "use_range",                 m_UseRange                );

	// state
	persist.Xfer( "gold_value",						m_GoldValue           );
	persist.Xfer( "life_state",						m_LifeState           );
	persist.Xfer( "max_life",						m_MaxLife             );
	persist.Xfer( "life",							m_Life                );
	persist.Xfer( "life_recovery_unit",				m_LifeRecoveryUnit    );
	persist.Xfer( "life_recovery_period",			m_LifeRecoveryPeriod  );
	persist.Xfer( "non_ghost_alpha",				m_NonGhostAlpha		  );

	// mana
	persist.Xfer( "max_mana",             m_MaxMana            );
	persist.Xfer( "mana",                 m_Mana               );
	persist.Xfer( "mana_recovery_unit",   m_ManaRecoveryUnit   );
	persist.Xfer( "mana_recovery_period", m_ManaRecoveryPeriod );

// Xfer aspect.

	// full?
	if ( persist.IsFullXfer() )
	{
		// aspects
		persist.Xfer( "m_CurrentAspect", m_CurrentAspect );
		persist.Xfer( "m_NativeAspect", m_NativeAspect );
		gpassert( m_CurrentAspect.IsNull() || m_CurrentAspect->ShouldPersist() );
		gpassert( m_NativeAspect.IsNull() || m_NativeAspect->ShouldPersist() );

		// xp
		persist.Xfer( "m_ExperienceRemaining",			m_ExperienceRemaining	    );
		persist.Xfer( "m_ExperienceBenefactor",			m_ExperienceBenefactor		);
		persist.Xfer( "m_ExperienceBenefactorSkill",	m_ExperienceBenefactorSkill );

		// dynamic textures
		persist.XferSparseArray( "m_DynamicTextures", m_DynamicTextures, ARRAY_END( m_DynamicTextures ), FuBi::PersistContext::EmptyStringCheck() );
		if ( persist.IsRestoring() )
		{
			m_CurrentAspectPtr = m_CurrentAspect.Get();

			UpdateDynamicTextures();
			CreateMegamapTexture();
		}

		// misc
		persist.Xfer( "m_bIsRespawningAsGhost", m_bIsRespawningAsGhost );
		persist.Xfer( "m_VisibleThisFrame", m_VisibleThisFrame );

		// cache
		persist.XferHex( "m_ScreenPlayerVisibleCount", m_ScreenPlayerVisibleCount );
	}
	else
	{
		// preprocess for saving
		gpstring modelName;
		if ( persist.IsSaving() )
		{
			modelName = FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( GetAspectHandle() ) );

			// $ special for <none> models that cannot be modified from SE
			if ( persist.IsSaving() )
			{
				gpstring originalName;
				GetData()->GetRecord()->Get( "model", originalName );
				if ( originalName.same_no_case( "<none>" ) )
				{
					modelName = originalName;
				}
			}
		}
		else if ( m_CurrentAspect.IsValid() )
		{
			modelName = FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( GetAspectHandle() ) );
		}

		// xfer
		bool bXferModel = true;
		if ( GetGo()->HasInventory() )
		{
			Go * pArmor = GetGo()->GetInventory()->GetEquipped( ES_CHEST );
			if ( pArmor )
			{
				gpstring sArmorName = FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( pArmor->GetAspect()->GetAspectHandle() ) );
				if ( sArmorName.same_no_case( modelName ) )
				{
					bXferModel = false;
				}
			}
		}

		if ( bXferModel )
		{
			persist.Xfer( "model", modelName );
		}

		// process the aspect if restoring, and it's different from current
		bool processAspect =
			   persist.IsRestoring()
			&& (!m_CurrentAspect || !FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( m_CurrentAspect ) ).same_no_case( modelName ));

		// special handling for editor
#		if !GP_RETAIL
		if ( processAspect && gGoDb.IsEditMode() )
		{
			// ok if we've already got an aspect, and we're about to xfer in
			// <none>, then that would mean we'd require another construction
			// message or something. so bail out instead.
			if ( m_CurrentAspect && modelName.same_no_case( "<none>" ) )
			{
				processAspect = false;
			}
		}
#		endif // !GP_RETAIL

		if ( processAspect )
		{
			// restore aspect if it was non-native
			RestoreNativeAspect();

			// release old model if any
			if ( m_CurrentAspect )
			{
				gAspectStorage.ReleaseAspect( m_CurrentAspect, true );
				m_CurrentAspect = nema::HAspect();
				m_CurrentAspectPtr = NULL;
			}

			// create the megamap texture
			CreateMegamapTexture();

			// try actual model name
			if ( !modelName.same_no_case( "<none>" ) )
			{
				gpstring modelFile;
				if ( !modelName.empty() && gNamingKey.BuildASPLocation( modelName, modelFile ) )
				{
					m_CurrentAspect = gAspectStorage.LoadAspect( modelFile, modelName );
				}

				// failed? try default
				if ( !m_CurrentAspect )
				{
#					if ( !GP_RETAIL )
					{
						gperrorf(( "Placeholder model substitution - illegal model '%s' for Go at '%s'\n",
								   modelName.c_str(), GetGo()->DevGetFuelAddress().c_str() ));
					}
#					endif // !GP_RETAIL

					modelName = gContentDb.GetDefaultAspectName();
					if ( gNamingKey.BuildASPLocation( modelName, modelFile ) )
					{
						m_CurrentAspect = gAspectStorage.LoadAspect( modelFile, modelName );
						m_CurrentAspectPtr = m_CurrentAspect.Get();
					}
				}

				if ( m_CurrentAspect )
				{
					// $$$ use "unused param report" and dump set of extra fuel entries

					// setup the aspect's textures
					m_CurrentAspectPtr = m_CurrentAspect.Get();
					ResetTextures();

				}
				else
				{
					// still failed? nothing will work then.
					m_CurrentAspectPtr = NULL;
					gperrorf(( "Unable to load model '%s' for Go at '%s'\n",
							   modelName.c_str(), GetGo()->DevGetFuelAddress().c_str() ));
					success = false;
				}
			}
		}
	}

	return ( success );
}

bool GoAspect :: XferCharacter( FuBi::PersistContext& persist )
{
	bool shouldPersist = true;
	if ( persist.IsSaving() )
	{
		GoString* i, * ibegin = m_DynamicTextures, * iend = ARRAY_END( m_DynamicTextures );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( !i->empty() )
			{
				break;
			}
		}

		shouldPersist = i != iend;
	}

	if ( shouldPersist )
	{
		if ( persist.EnterBlock( "skins" ) )
		{
			GoString* i, * ibegin = m_DynamicTextures, * iend = ARRAY_END( m_DynamicTextures );
			for ( i = ibegin ; i != iend ; ++i )
			{
				persist.Xfer( ToString( i - ibegin ), *i );
			}

			if ( persist.IsRestoring() )
			{
				UpdateDynamicTextures();
			}
		}
		persist.LeaveBlock();
	}

	// anything else?
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_GhostTimeout", m_GhostTimeout );
	}

	// force reconstitute so ammo has something to attach to!
	if ( persist.IsRestoring() && GetGo()->IsActor() && GetGo()->IsAllClients() )
	{
		PrepareToDrawNow( false );
	}

	return ( true );
}

void GoAspect :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	switch ( msg.GetEvent() )
	{
		case ( WE_CONSTRUCTED ):
		{
			if ( (GetGo()->IsOmni() && TestOptions( OPTION_IS_VISIBLE )) || Services::IsEditor() )
			{
				GetAspectHandle()->Reconstitute();
			}
		}
		break;

		case ( WE_DAMAGED ):
		{
			/// $$$ This is a bad way to pass messages to the nema aspect. Want to mark messages as something that the animation skrit might
			/// want, then we could just pass them all that way instead of handling individual messages. -ET
			if( !gWorld.IsMultiPlayer() )
			{
				nema::HAspect hasp = GetAspectHandle();
				if (hasp)
				{
					// send a message to the animation skrit.
					hasp->HandleMessage( msg );
				}
			}
		}
		break;

		case ( WE_DESTRUCTED ):
		{
			nema::HAspect hasp = GetAspectHandle();
			if (hasp)
			{
				hasp->SetGoid(GOID_INVALID);
			}
		}
		break;

		case ( WE_POST_RESTORE_GAME ):
		{
			nema::Aspect * ptr = m_CurrentAspect.Get();
			if( ptr )
			{
				ptr->CommitCreation();
			}

			if ( GetGo()->IsInAnyWorldFrustum() && GetDoesBlockPath() )
			{
				gWorldTerrain.RequestObjectBlock( GetGoid() );
			}
		}
		break;

		case ( WE_KILLED ):
		{
			// do killed handling
			HandleKilled();

			// start expiration countdown
			gGoDb.StartExpiration( GetGo() );
		}
		break;

		case ( WE_ENTERED_WORLD ):
		{
			if ( TestOptions( OPTION_IS_VISIBLE ) || GetForceNoRender() )
			{
				if (GetAspectHandle()->GetParent())
				{
					if (!GetAspectHandle()->GetParent().IsValid())
					{
						gperrorf(("GOASPECT CORRUPTION: The nema parent of [%s:0x%08x] is set to [0x%08x] but that's not a valid go handle!",
							GetGo() ? GetGo()->GetTemplateName() : "<UNKNOWN_GO>",
							GetGo() ? MakeInt(GetGo()->GetGoid()) : -1,
							GetAspectHandle()->GetParent()
							));
					}
					else if (!GetAspectHandle()->GetParent()->IsPurged(2))
					{
						GetAspectHandle()->Reconstitute(1);
					}
				}
				else
				{
					GetAspectHandle()->Reconstitute(1);
				}
				if ( GetDoesBlockPath() )
				{
					gWorldTerrain.RequestObjectBlock( GetGoid() );
				}
			}

#			if !GP_RETAIL
			if ( GetUpperLodfi() > 0 )
			{
				if ( GetIsCollidable() )
				{
					gperrorf((
							"TELL GREG: we have an object of type '%s' (scid = "
							"0x%08X) which is set collidable but it has "
							"positive lodfi settings %g/%g!!\n",
							GetGo()->GetTemplateName(),
							GetScid(),
							GetLowerLodfi(),
							GetUpperLodfi() ));
				}
				if ( GetDoesBlockPath() )
				{
					gperrorf((
							"TELL GREG: we have an object of type '%s' (scid = "
							"0x%08X) which is set to block paths but it has "
							"positive lodfi settings %g/%g!!\n",
							GetGo()->GetTemplateName(),
							GetScid(),
							GetLowerLodfi(),
							GetUpperLodfi() ));
				}
			}
#			endif // !GP_RETAIL
		}
		break;

		case ( WE_LEFT_WORLD ):
		{
			if ( (TestOptions( OPTION_IS_VISIBLE ) || GetForceNoRender()) && GetDoesBlockPath() )
			{
				gWorldTerrain.RequestObjectUnblock( GetGoid() );
			}
			if (!GetGo()->IsFading())
			{
				GetAspectHandle()->Purge();
			}
		}
		break;

		case ( WE_RESURRECTED ):
		{
			// start expiration countdown
			gGoDb.CancelExpiration( GetGo() );
		}
		break;

		case ( WE_LOST_CONSCIOUSNESS ):
		{
			// must explicitly unselect the object
			GetGo()->Deselect();
		}
		break;

		case ( WE_UNEQUIPPED ):
		{
			// stop any effects that were running on me due to equipping (like
			// if i'm a freakin' flamin' sword)
			gGoDb.StopEffectScripts( GetGo(), WE_EQUIPPED );
		}
		break;

		case ( WE_PICKED_UP ):
		{
			// stop any effects that were running on me due to being dropped on
			// the ground (like bubbles coming out of a potion)
			gGoDb.StopEffectScripts( GetGo(), WE_DROPPED );
		}
		break;

		case ( WE_EXPIRED_FORCED ):
		{
			// right! time to spawn a new dude to replace me with! (maybe)
			const gpstring& newTemplate = GetExpiredTemplateName();
			if ( !newTemplate.empty() )
			{
				// spawn dude
				GoCloneReq cloneReq( newTemplate, GetGo()->GetPlayerId() );
				if ( GetGo()->HasPlacement() )
				{
					// copy the placement exactly, no snapping
					cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
					cloneReq.SetStartingOrient( GetGo()->GetPlacement()->GetOrientation() );
					cloneReq.SetForcePosition();
				}
				Goid newGoid = gGoDb.SCloneGo( cloneReq );

				// xfer guts to new dude
				GoHandle newGo( newGoid );
				if ( newGo )
				{
					// transfer guts over
					newGo->SForcedExpiredTransfer( GetGo() );

					// special: mark for deferred deletion for multiplayer
					// machines so they can perform the clone then the transfer
					// then the deletion.
					gGoDb.SMarkGoAndChildrenForDeletion( GetGo(), true, true, true );

					// start the new one expiring normally so it can continue
					// on its death & decay path
					gGoDb.StartExpiration( newGo );
				}
			}
		}
		break;
	}
}

void GoAspect :: ResetModifiers( void )
{
	m_MaxLife           .Reset();
	m_LifeRecoveryUnit  .Reset();
	m_LifeRecoveryPeriod.Reset();
	m_MaxMana           .Reset();
	m_ManaRecoveryUnit  .Reset();
	m_ManaRecoveryPeriod.Reset();
}

void GoAspect :: UnlinkParent( Go* /*parent*/ )
{
	ClearNemaParent();
}

bool GoAspect :: CommitCreation( void )
{
	if ( m_CurrentAspect )
	{
		m_CurrentAspect->SetGoid( GetGoid() );
		m_CurrentAspectPtr = m_CurrentAspect.Get();
	}
	else
	{
		m_CurrentAspectPtr = NULL;
	}

	if ( m_NativeAspect )
	{
		m_NativeAspect->SetGoid( GetGoid() );
	}

	m_ExperienceRemaining = GetExperienceValue();

	// build complex shadow texture if this is not a clone source
	if( !GetGo()->IsCloneSourceGo() )
	{
		CreateComplexShadowTexture();
	}

	return true;
}

bool GoAspect :: CommitLoad( void )
{
	// build complex shadow texture if this is not a clone source
	if( !GetGo()->IsCloneSourceGo() )
	{
		CreateComplexShadowTexture();
	}

	return true;
}

bool GoAspect :: CommitImport( bool /*fullXfer*/ )
{
	// build complex shadow texture if this is not a clone source
	if( !GetGo()->IsCloneSourceGo() )
	{
		CreateComplexShadowTexture();
	}

	return true;
}

void GoAspect :: Update( float deltaTime )
{
	UpdateRegeneration( deltaTime );
}

void GoAspect :: UpdateSpecial( float deltaTime )
{
	if ( GetLifeState() == LS_GHOST )
	{
		if ( m_GhostTimeout != 0.0f  )
		{
			if ( m_GhostTimeout <= (float)gWorldTime.GetTime() )
			{
				if ( IsServerLocal() )
				{
					SSetLifeState( LS_ALIVE_CONSCIOUS );
				}
				m_GhostTimeout = 0.0f;
			}
		}
	}

	// regen mana and life (only if we're out of the world frustum so no call these twice)
	if ( !GetGo()->IsUpdating() && !GetGo()->IsInActiveWorldFrustum() )
	{
		UpdateRegeneration( deltaTime );
	}
}

void GoAspect :: DetectWantsBits( void )
{
	bool wantsUpdates = false;

	// only global's might regenerate
	if ( GetGo()->IsGlobalGo() )
	{
		if ( GetManaRecoveryPeriod() > 0 )
		{
			wantsUpdates = true;
		}
		else if ( GetLifeRecoveryPeriod() > 0 )
		{
			wantsUpdates = true;
		}
	}

	SetWantsUpdates( wantsUpdates );
}

void GoAspect :: GetWorldSpaceOrientedBoundingVolume( matrix_3x3* orient, vector_3* center, vector_3* halfDiag, DWORD bone_index ) const
{
	if ( GetGo()->IsWobbDirty() || bone_index != 0xFFFFFFFF )
	{
		GetWorldSpaceOrientedBoundingVolume(
				&m_WobbOrientCache,
				&m_WobbCenterCache,
				&m_WobbHalfDiagCache,
				GetGo()->GetPlacement()->GetPosition(),
				GetGo()->GetPlacement()->GetOrientation(),
				GetAspectPtr(),
				m_BoundingVolumeScale,
				GetRenderScale(),
				bone_index );

		if( bone_index == 0xFFFFFFFF )
		{
			GetGo()->SetWobbDirty( false );
		}
	}

	if ( orient != NULL )
	{
		*orient = m_WobbOrientCache;
	}
	if ( center != NULL )
	{
		*center = m_WobbCenterCache;
	}
	if ( halfDiag != NULL )
	{
		*halfDiag = m_WobbHalfDiagCache;
	}
}

void GoAspect :: GetNodeSpaceOrientedBoundingVolume( matrix_3x3* orient, vector_3* center, vector_3* halfDiag, DWORD bone_index ) const
{
	GetNodeSpaceOrientedBoundingVolume(
			orient,
			center,
			halfDiag,
			GetGo()->GetPlacement()->GetPosition(),
			GetGo()->GetPlacement()->GetOrientation(),
			GetAspectPtr(),
			m_BoundingVolumeScale,
			GetRenderScale(),
			bone_index );
}

void GoAspect :: UpdateDynamicTextures( void )
{
	GoString* i, * ibegin = m_DynamicTextures, * iend = ARRAY_END( m_DynamicTextures );
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !i->GetString().empty() )
		{
			SetDynamicTexture( (ePlayerSkin)(i - ibegin), i->c_str(), true );
		}
	}
}

void GoAspect :: UpdateRegeneration( float deltaTime )
{
	// regen mana and life
	gRules.RegenerateMana( GetGo(), deltaTime );
	gRules.RegenerateLife( GetGo(), deltaTime );

	// force sync if necessary
	if ( ::IsServerLocal() )
	{
		m_RegenSyncPeriod -= deltaTime;
		if ( m_RegenSyncPeriod < 0 )
		{
			m_RegenSyncPeriod = gRules.CalcSyncPeriod();
			RCSyncState( GetCurrentLife(), GetCurrentMana() );
		}
	}
}

DWORD GoAspect :: GoAspectToNet( GoAspect* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoAspect* GoAspect :: NetToGoAspect( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetAspect() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
