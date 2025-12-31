//
//	Flamethrower_effect_base.cpp
//	Copyright 1999, Gas Powered Games
//
//
#include "precomp_flamethrower.h"
#include "gpcore.h"

#include <vector>

#include "gosupport.h"
#include "vector_3.h"
#include "matrix_3x3.h"
#include "space_3.h"
#include "fubitraitsimpl.h"

#include "RapiOwner.h"

#include "Flamethrower.h"
#include "WorldFx.h"
#include "Flamethrower_tracker.h"
#include "Flamethrower_effect_base.h"
#include "Flamethrower_FuBi_support.h"

#include "gpprofiler.h"

#include "namingkey.h"

inline bool FindObject( SFxEID id, SEMap &Coll, SEMap::iterator &iObj )
{
	iObj = Coll.find( id );
	return ( iObj != Coll.end() );
}

//
// Base class code for all special effects
//

SpecialEffect::SpecialEffect()
{
	m_ParentScriptID		=	SFxSID_INVALID;
	m_ParentEffectID		=	SFxEID_INVALID;
	m_ID					=	0;
	m_OwnerID				=	0;
	m_BoundingRadius		=	1.0f;

	m_Seed					=	0xDEADD00F;
	m_bInitialized			=	false;

	m_bFinished				=	false;
	m_bFinishing			=	false;

	m_Delay					=	0;
	m_bRotation				=	false;

	m_Scale					=	1.0f;
	m_ScaleFactor			=	1.0f;
	m_BoundingRadius		=	1.0f;
	m_NodeAlpha				=	1.0f;

	m_bRotation				=	false;
	m_bLoop					=	false;
	m_bIsChild				=	false;
	m_bWindAffected			=	false;

	m_OrientMode			=	Flamethrower_target::ORIENT_GO;
	
	m_bPauseImmune			=	false;
	m_bUseFog				=	false;
	m_bInitialized			=	false;
	m_bUseLoadedTexture		=	false;
	m_GlobalTextureID		=	0;

	m_TextureName			=	0;
	m_TextureWidth			=	0;
	m_TextureHeight			=	0;

	m_bMarkedForDraw		=	false;

	m_bAbsoluteCoords		=	false;

	m_TimeScalar			=	1.0f;
	m_Duration				=	0.0f;
	m_Delay					=	0.0f;

	m_Elapsed				=	0.0f;
	m_FrameSync				=	0.0f;
	m_bInterestOnly			=	false;

	m_bFast					=	true;
	m_bVisible				=	true;
	m_bMustUpdate			=	false;
	m_bHasMoved				=	true;
	m_bUpdated				=	false;

	m_MinFPS				=	1.0f;
}


SpecialEffect::~SpecialEffect()
{
	if( (m_TextureName != 0) && ( m_bUseLoadedTexture ) )
	{
		gWorldFx.GetRenderer().DestroyTexture( m_TextureName );
	}
}


void
SpecialEffect::operator=( SpecialEffect &dup )
{
	dup.m_ParentScriptID		= m_ParentScriptID;
	dup.m_ParentEffectID		= m_ParentEffectID;
	dup.m_ID					= 0;
	dup.m_OwnerID				= m_OwnerID;
	dup.m_BoundingRadius		= m_BoundingRadius;

	dup.m_Scale					= m_Scale;
	dup.m_ScaleFactor			= m_ScaleFactor;

	dup.m_bRotation				= m_bRotation;
	dup.m_RotationAngles		= m_RotationAngles;
	dup.m_RotationMatrix		= m_RotationMatrix;

	dup.m_bFinished				= false;
	dup.m_bFinishing			= false;
	dup.m_bLoop					= m_bLoop;
	dup.m_bIsChild				= m_bIsChild;

	dup.m_bUseLoadedTexture		= m_bUseLoadedTexture;
	dup.m_sTextureName			= m_sTextureName;
	dup.m_GlobalTextureID		= m_GlobalTextureID;
	dup.m_TextureName			= m_TextureName;
	if( m_TextureName )
	{
		gWorldFx.GetRenderer().AddTextureReference( m_TextureName );
	}

	dup.m_TextureWidth			= m_TextureWidth;
	dup.m_TextureHeight			= m_TextureHeight;

	dup.m_Cam					= m_Cam;
	dup.m_Vx					= m_Vx;
	dup.m_Vy					= m_Vy;
	dup.m_Vz					= m_Vz;

	dup.m_NodeOrient			= m_NodeOrient;
	dup.m_NodeCentroid			= m_NodeCentroid;

	dup.m_bMarkedForDraw		= m_bMarkedForDraw;

	dup.m_Dir					= m_Dir;
	dup.m_Orient				= m_Orient;

	dup.m_Pos					= m_Pos;
	dup.m_Offset				= m_Offset;
	dup.m_TimeScalar			= m_TimeScalar;
	dup.m_Duration				= m_Duration;
	dup.m_Delay					= 0.0f;
	dup.m_bUseFog				= m_bUseFog;
	dup.m_bPauseImmune			= m_bPauseImmune;

	dup.m_OrientMode			= m_OrientMode;

	dup.m_Elapsed				= 0.0f;
	dup.m_FrameSync				= 0.0f;
	dup.m_bInterestOnly			= m_bInterestOnly;

	dup.m_bFast					= m_bFast;
	dup.m_bVisible				= m_bVisible;
	dup.m_bMustUpdate			= m_bMustUpdate;
	dup.m_bHasMoved				= m_bHasMoved;
	dup.m_bUpdated				= m_bUpdated;

	dup.m_MinFPS				= m_MinFPS;
}


void
SpecialEffect::PrepareForSave( void )
{
	m_Targets->PrepareForSave();

	GoidColl::iterator i, ibegin = m_Friendlys.begin(), iend = m_Friendlys.end();
	for ( i = ibegin ; i != iend ; )
	{
		if ( !IsValid( *i ) )
		{
			i = m_Friendlys.erase( i );
			iend = m_Friendlys.end();
		}
		else
		{
			++i;
		}
	}
}

void
SpecialEffect::PrepareForSaveChildren( void )
{
	SFxEIDColl childIDs;
	GetChildren( childIDs );

	SpecialEffect* kid;
	SFxEIDColl::iterator itID;
	for( itID = childIDs.begin(); itID != childIDs.end(); )
	{
		kid = gFlamethrower.FindEffect( *itID );
		if( kid == NULL )
		{
			gpassertm(0,("I was unable to reproduce this problem, please keep the call stack up. thanks! jess"));
			itID = childIDs.erase( itID );
		}
		else
		{
			++itID;
		}
	}

	// is my owner gone?
	if ( !IsValid( m_OwnerID ) )
	{
		m_OwnerID = GOID_INVALID;
	}
}

bool
SpecialEffect::Xfer( FuBi::PersistContext& persist )
{
	gpassert( persist.IsRestoring() || ShouldPersist() );

#	if !GP_RETAIL
	if ( persist.IsSaving() )
	{
		CheckValid();
	}
#	endif // !GP_RETAIL

	persist.Xfer      ( "m_sName",               m_sName               );
	persist.XferVector( "m_Children",            m_Children            );
	persist.XferVector( "m_Friendlys",           m_Friendlys           );

	persist.XferHex   ( "m_ParentEffectID",      m_ParentEffectID      );
	persist.XferHex   ( "m_ParentScriptID",      m_ParentScriptID      );

	persist.XferHex   ( "m_ID",                  m_ID                  );
	persist.XferHex   ( "m_OwnerID",             m_OwnerID             );

	persist.Xfer      ( "m_bAbsoluteCoords",     m_bAbsoluteCoords     );
	persist.Xfer      ( "m_Scale",               m_Scale               );
	persist.Xfer      ( "m_ScaleFactor",         m_ScaleFactor         );
	persist.Xfer      ( "m_BoundingRadius",      m_BoundingRadius      );

	persist.Xfer      ( "m_NodeAlpha",           m_NodeAlpha           );

	persist.XferHex   ( "m_Seed",				 m_Seed				   );

	persist.Xfer      ( "m_bFinished",           m_bFinished           );
	persist.Xfer      ( "m_bFinishing",          m_bFinishing          );
	persist.Xfer      ( "m_bLoop",               m_bLoop               );
	persist.Xfer      ( "m_bIsChild",            m_bIsChild            );

	persist.Xfer	  ( "m_OrientMode",			 m_OrientMode		   );

	persist.Xfer      ( "m_bWindAffected",       m_bWindAffected       );
	persist.Xfer      ( "m_bPauseImmune",        m_bPauseImmune        );
	persist.Xfer      ( "m_bUseFog",             m_bUseFog             );

	persist.Xfer      ( "m_bInitialized",        m_bInitialized        );

	persist.Xfer      ( "m_bUseLoadedTexture",   m_bUseLoadedTexture   );
	persist.Xfer      ( "m_GlobalTextureID",	 m_GlobalTextureID	   );

	persist.Xfer      ( "m_sTextureName",        m_sTextureName		   );

	persist.Xfer      ( "m_RotationMatrix",      m_RotationMatrix      );
	persist.Xfer      ( "m_bRotation",           m_bRotation           );
	persist.Xfer      ( "m_RotationAngles",      m_RotationAngles      );

	persist.Xfer      ( "m_Cam",                 m_Cam                 );
	persist.Xfer      ( "m_Vx",                  m_Vx                  );
	persist.Xfer      ( "m_Vy",                  m_Vy                  );
	persist.Xfer      ( "m_Vz",                  m_Vz                  );

	persist.Xfer      ( "m_NodeOrient",          m_NodeOrient          );
	persist.Xfer      ( "m_NodeCentroid",        m_NodeCentroid        );

	persist.Xfer	  ( "m_bMarkedForDraw",		 m_bMarkedForDraw	   );
	persist.Xfer      ( "m_Pos",                 m_Pos                 );
	persist.Xfer      ( "m_Orient",              m_Orient              );
	persist.Xfer      ( "m_Dir",                 m_Dir                 );

	persist.Xfer      ( "m_ReportedPos",         m_ReportedPos         );
	persist.Xfer      ( "m_ReportedOrient",      m_ReportedOrient      );

	persist.Xfer      ( "m_Offset",              m_Offset              );
	persist.Xfer      ( "m_TimeScalar",          m_TimeScalar          );
	persist.Xfer      ( "m_Delay",               m_Delay               );
	persist.Xfer      ( "m_Duration",            m_Duration            );
	persist.Xfer      ( "m_Elapsed",             m_Elapsed             );
	persist.Xfer      ( "m_FrameSync",           m_FrameSync           );
	persist.Xfer	  ( "m_bInterestOnly",		 m_bInterestOnly	   );

	persist.Xfer      ( "m_Targets",             m_Targets             );

	persist.Xfer      ( "m_bFast",               m_bFast               );
	persist.Xfer      ( "m_bVisible",            m_bVisible            );
	persist.Xfer      ( "m_bMustUpdate",         m_bMustUpdate         );
	persist.Xfer      ( "m_bHasMoved",			 m_bHasMoved		   );
	persist.Xfer	  ( "m_bUpdated",			 m_bUpdated			   );

	persist.Xfer	  ( "m_MinFPS",				 m_MinFPS			   );


	bool bXfer = OnXfer( persist );

	if( persist.IsRestoring() )
	{
		InitializeTextures();
		if( m_bInitialized )
		{
			ReInitialize();
		}
	}

	return bXfer;
}


bool
SpecialEffect::ShouldPersist( void ) const
{
	if ( m_OwnerID == GOID_INVALID )
	{
		return ( true );
	}

	GoHandle go( m_OwnerID );
	gpassert( go );
	return ( !go || go->ShouldPersist() );
}


void
SpecialEffect::RegisterDefaultParams( EffectParams &params )
{
	params.SetFloat		( "dur",			-1.0f			 );	// 0
	params.SetFloat		( "delay",			0				 );	// 1
	params.SetVector3	( "dir",			0.0f, 0.0f, 0.0f );	// 2
	params.SetVector3	( "offset",			0.0f, 0.0f, 0.0f );	// 3
	params.SetVector3	( "rotate",			0.0f, 0.0f, 0.0f );	// 4
	params.SetFloat		( "ts",				1.0f			 );	// 5
	params.SetFloat		( "scale",			1.0f			 );	// 6
	params.SetBool		( "abs",			false			 );	// 7
	params.SetBool		( "use_wind",		false			 );	// 8
	params.SetBool		( "bone_orient",	false			 );	// 9
	params.SetBool		( "world_orient",	false			 );	// 10
	params.SetBool		( "pause_immune",	false			 );	// 11
	params.SetBool		( "use_fog",		false			 );	// 12
	params.SetBool		( "slow",			false			 );	// 13
	params.SetUINT32	( "owner",			0				 );	// 14
	params.SetGPString	( "texture",		gpstring::EMPTY	 );	// 15
	params.SetBool		( "must_update",	false			 );	// 16
	params.SetFloat		( "fps",			1.0f			 ); // 17

	RegisterDerivedParams( params );
}


#if !GP_RETAIL

void
SpecialEffect::CheckValid( void ) const
{
	bool persist = ShouldPersist();

	SFxEIDColl::const_iterator i, ibegin = m_Children.begin(), iend = m_Children.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( FxShouldPersistHelper()( *i ) != persist )
		{
			gperror( "SpecialEffect: an effect I have has a different 'shouldPersist' setting than me!\n" );
		}
	}

	if ( !m_Friendlys.empty() && !persist )
	{
		// if there are friendlies then we should be persisting
		gperror( "SpecialEffect: I have friendlies but I am not being persisted!\n" );
	}

	GoidColl::const_iterator j, jbegin = m_Friendlys.begin(), jend = m_Friendlys.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		// only global go's should end up here
		GoHandle go( *j );
		if ( !go )
		{
			gperror( "SpecialEffect: I have a friendly that is nonexistent!\n" );
		}
		else if ( !go->IsGlobalGo() )
		{
			gperror( "SpecialEffect: I have a friendly that is not a global Go!\n" );
		}
	}

	if ( (m_Targets.get() != NULL) && (m_Targets->ShouldPersist() != persist) )
	{
		gperror( "SpecialEffect: I have targets, but one or more of them has a different 'shouldPersist' setting than me!\n" );
	}
}

#endif // !GP_RETAIL


void
SpecialEffect::Initialize( UINT32 seed )
{
	m_Params.GetFloat	( 0,	m_Duration				);
	m_Params.GetFloat	( 1,	m_Delay					);
	m_Params.GetVector3	( 2,	m_Dir					);
	if( !m_Dir.IsZero() )
	{
		m_Dir = Normalize( m_Dir );
	}

	m_Params.GetVector3	( 3,	m_Offset				);
	m_bRotation = m_Params.GetVector3( 4, m_RotationAngles	);
	m_RotationAngles *= (RealPi / 180.0f);

	m_Params.GetFloat	( 5,	m_TimeScalar			);

	m_Params.GetFloat	( 6,	m_ScaleFactor			);
	// Copy bounding radius from first target
	m_Targets->GetBoundingRadius( m_BoundingRadius );
	m_Scale = m_BoundingRadius * m_ScaleFactor;

	m_Params.GetBool	( 7,	m_bAbsoluteCoords		);
	m_Params.GetBool	( 8,	m_bWindAffected			);

	bool bBoneOrient = false, bWorldOrient = false;

	m_Params.GetBool	( 9,	bBoneOrient				);
	m_Params.GetBool	( 10,	bWorldOrient			);

	if( !bBoneOrient && !bWorldOrient )
	{
		m_OrientMode = Flamethrower_target::ORIENT_GO;
	}
	else if( bBoneOrient )
	{
		m_OrientMode = Flamethrower_target::ORIENT_BONE;
	}
	else
	{
		m_OrientMode = Flamethrower_target::ORIENT_WORLD;
	}

	m_Params.GetBool	( 11,	m_bPauseImmune			);
	m_Params.GetBool	( 12,	m_bUseFog				);

	bool bSlow = false;
	m_Params.GetBool	( 13,	bSlow					);
	m_bFast = !bSlow;

	UINT32	temp;
	if( m_Params.GetUINT32	( 14,		temp			))
	{
		SetOwnerID( MakeGoid( temp ) );
	}

	m_bUseLoadedTexture = m_Params.GetGPString ( 15,	m_sTextureName );

	m_Params.GetBool	( 16,	m_bMustUpdate			);
	m_Params.GetFloat	( 17,	m_MinFPS				);

	// limit the maximum fps for an effect to be at 60
	m_MinFPS = min_t( m_MinFPS, 60.0f );
	m_MinFPS = (m_MinFPS < 1.0f)? 1.0f : 1.0f/m_MinFPS;

	// Seed the random number generator to be based off of Flamethrowers own RNG if effect is global
	gWorldFx.GetRNG().SetSeed( seed );
	m_Seed = gWorldFx.GetRNG().RandomDword();

	InitializeDerived();
	InitializeTextures(); 
}


void
SpecialEffect::Update( const float SecondsElapsed, const float UnPausedSecondsElapsed )
{
	// If the elapsed time is twice the duration end this effect no matter what
	// UNLESS it has an infinite (-1) duration...
	if( ( m_Duration != -1 ) && ( m_Elapsed > ( m_Duration + m_Duration )) )
	{
		EndThisEffect();
	}

	// Default update code
	if( m_bPauseImmune )
	{
		m_FrameSync = UnPausedSecondsElapsed * m_TimeScalar;
		m_Elapsed += UnPausedSecondsElapsed;
	}
	else
	{
		m_FrameSync = SecondsElapsed * m_TimeScalar;
		m_Elapsed += SecondsElapsed;
	}

	// decrease the delay counter if it was set to something
	if( m_Delay > 0 )
	{
		m_Delay -= SecondsElapsed;

		if( m_Delay <= 0 )
		{
			m_Elapsed = m_Delay = 0;
		}
	}

	if( (m_Delay == 0) || UpdatesDuringDelay() )
	{
		if(( m_Elapsed >= m_Duration ) && ( m_Duration != -1.0f ) )
		{
			m_bFinishing = true;
		}

		// execute effect tracking code
		Track();

		if( m_bMustUpdate || m_bHasMoved || m_bVisible )
		{
			// is the owner of this effect visible or inside inventory?
			if( !gWorldFx.IsScriptOwnerVisible( m_ParentScriptID ) )
			{
				m_bUpdated = false;
				return;
			}

			// set the rng seed
			gWorldFx.GetRNG().SetSeed( m_Seed );
			m_Seed = gWorldFx.GetRNG().RandomDword();


			if( m_bFinished )
			{
				m_bUpdated = false;
				return;
			}

			// execute effect specific update code
			UpdateDerived();

			m_bUpdated = true;
			m_bVisible = false;
		}
	}

#	if !GP_RETAIL
	gWorldFx.DoUpdateSanityCheck();
#	endif
}


bool
SpecialEffect::Draw( void )
{
	if( m_NodeAlpha == 0 )
	{
		return false;
	}

	Rapi& renderer = gWorldFx.GetRenderer();

	DWORD old_fog_color = 0;

	gWorldFx.SaveRenderingState();


	if( !m_bUseFog )
	{
		old_fog_color = renderer.GetFogColor();
		renderer.SetFogColor( 0 );
	}

	gWorldFx.GetNodeOrientAndCenter( m_Pos.node, m_NodeOrient, m_NodeCentroid );
	renderer.SetWorldMatrix( m_NodeOrient, m_NodeCentroid );

	// Get the camera orientation
	m_Cam = Transpose( m_NodeOrient ) * gWorldFx.GetCameraOrientation();
	m_Vx = m_Cam.GetColumn_0();
	m_Vy = m_Cam.GetColumn_1();
	m_Vz = m_Cam.GetColumn_2();

	Put();
	DrawSelf();

	if( !m_bUseFog )
	{
		renderer.SetFogColor( old_fog_color );
	}

	gWorldFx.RestoreRenderingState();

#	if !GP_RETAIL
	gWorldFx.DoDrawSanityCheck();
#	endif

	m_bMarkedForDraw = false;

	return true;
}


bool
SpecialEffect::ShouldDraw( void ) const
{
	return ( m_Delay <= 0 );
}


void
SpecialEffect::AttachEffect( SFxEID id )
{
	m_Children.push_back( id );
}


void
SpecialEffect::RemoveEffect( SFxEID id )
{
	for( SFxEIDColl::iterator it = m_Children.begin(), it_end = m_Children.end(); it != it_end; ++it )
	{
		if( *it == id )
		{
			m_Children.erase( it );
			return;
		}
	}
}


void
SpecialEffect::AddTarget( Flamethrower_target *pTarget )
{
	m_Targets->AddTarget( pTarget );
}


void
SpecialEffect::NotifyScript( void )
{
	Flamethrower_script *pParentScript;

	if( gWorldFx.FindScript( m_ParentScriptID, &pParentScript ))
	{
		pParentScript->RemoveCompletionTestID( m_ID );
	}
}


bool
SpecialEffect::GetCullPosition( SiegePos & CullPosition )
{
	if( gWorldFx.IsNodeVisible( m_Pos.node ) )
	{
		CullPosition = m_Pos;
		return true;
	}

	return false;
}


const matrix_3x3&
SpecialEffect::GetTargetOrientation( const TattooTracker::TARGET_INDEX n )
{
	m_Targets->GetPlacement( NULL, &m_ReportedOrient, m_OrientMode, n );

	return m_ReportedOrient;
}


const SiegePos&
SpecialEffect::GetTargetPosition( TattooTracker::TARGET_INDEX n )
{
	m_Targets->GetPlacement( &m_ReportedPos, NULL, m_OrientMode, n );

	return m_ReportedPos;
}


void
SpecialEffect::OwnTargets( def_tracker &t )
{
	m_Targets = t;

	// our first time?
	if ( !m_Pos.node.IsValid() )
	{
		Flamethrower_target* target;
		if ( m_Targets->GetTarget( TattooTracker::TARGET, &target ) )
		{
			target->GetPlacement( &m_Pos, NULL, m_OrientMode );
		}
	}
}


void
SpecialEffect::PostCollision( Goid target_id, const SiegePos &pos, const SiegePos &norm, const vector_3 &dir )
{
	CollisionInfo *pCollision = new CollisionInfo( m_ID, target_id, m_OwnerID, pos, norm, dir );
	gWorldFx.PostCollision( pCollision );
}


void
SpecialEffect::SetDrawMatrix( const siege::database_guid &guid )
{
	matrix_3x3	orient;
	vector_3	trans;

	gWorldFx.GetNodeOrientAndCenter( SiegePos( vector_3(), guid ), orient, trans );

	gWorldFx.GetRenderer().SetWorldMatrix( orient, trans );

	m_Cam = Transpose( orient ) * gWorldFx.GetCameraOrientation();
	m_Vx = m_Cam.GetColumn_0();
	m_Vy = m_Cam.GetColumn_1();
	m_Vz = m_Cam.GetColumn_2();
}


void
SpecialEffect::InitializeTextures( void )
{
	if( m_bUseLoadedTexture )
	{
		// Destroy existing texture
		if( m_TextureName )
		{
			gWorldFx.GetRenderer().DestroyTexture( m_TextureName );
			m_TextureName	= 0;
			m_TextureWidth	= 0;
			m_TextureHeight	= 0;
		}

		gpstring sFilename;

		if( gNamingKey.BuildIMGLocation( m_sTextureName, sFilename ) )
		{
			m_TextureName	= gWorldFx.GetRenderer().CreateTexture	( sFilename, false, 0, TEX_LOCKED );
			m_TextureWidth	= gWorldFx.GetRenderer().GetTextureWidth	( m_TextureName );
			m_TextureHeight	= gWorldFx.GetRenderer().GetTextureHeight( m_TextureName );
		}
		else
		{
			gperrorf(("[%s] does not exist\n",m_sTextureName.c_str()));
			EndThisEffect();
			return;
		}
	}
	else
	{
		m_GlobalTextureID = InitializeTexture();
	}
}


void
SpecialEffect::Track( void )
{
	SiegePos targetPos;

	if( m_Targets->GetPlacement( &targetPos, &m_Orient, m_OrientMode ) )
	{
		if( !m_Offset.IsZero() && !gWorldFx.AddPositions( targetPos, (m_Orient * m_Offset), m_bFast, NULL ) )
		{
			m_bFinished = true;
			return;
		}

		m_bHasMoved = ( m_Pos != targetPos );
		m_Pos = targetPos;
	}
	else
	{
		m_bFinished = true;
	}
}


void
SpecialEffect::Put( void )
{
	Rapi& renderer = gWorldFx.GetRenderer();
	renderer.TranslateWorldMatrix( m_Pos.pos );
}
