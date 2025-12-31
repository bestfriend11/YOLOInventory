//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoGizmo.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#if !GP_RETAIL

#include "FileSys.h"
#include "FileSysUtils.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "NamingKey.h"
#include "Nema_Aspect.h"
#include "Nema_AspectMgr.h"
#include "ContentDb.h"
#include "GoAspect.h"
#include "GoCore.h"
#include "GoEdit.h"
#include "GoGizmo.h"
#include "GoSupport.h"
#include "Services.h"
#include "Siege_Camera.h"
#include "Siege_Engine.h"
#include "Siege_Walker.h"
#include "WorldOptions.h"

DECLARE_DEV_GO_SERVER_ONLY_COMPONENT( GoGizmo, "gizmo" );

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_POINTERCLASS_TRAITS_EX( nema::HAspect, NemaAspect, nema::HAspect::Null );

//////////////////////////////////////////////////////////////////////////////
// class GoGizmo implementation

GoGizmo :: GoGizmo( Go* parent )
	: Inherited( parent )
	, m_DiffuseColor( vector_3::ONE )
{
	m_IsVisible       = true;
	m_Scale           = 1;
	m_Alpha           = 1;
	m_UseDiffuseColor = false;
}

GoGizmo :: GoGizmo( const GoGizmo& source, Go* parent )
	: Inherited( source, parent )
	, m_DiffuseColor( source.m_DiffuseColor )
{
	// copy misc
	m_IsVisible       = source.m_IsVisible;
	m_Scale           = source.m_Scale;
	m_Alpha           = source.m_Alpha;
	m_UseDiffuseColor = source.m_UseDiffuseColor;

	// take the aspect
	nema::HAspect sourceAspect = source.GetAspectHandle();
	m_Aspect = gAspectStorage.LoadAspect( gAspectStorage.GetAspectName( sourceAspect ),
										  sourceAspect->GetDebugName() );

	//$$$$ Ask Chad about this!!! -biddle
	if (Services::IsEditor())
	{
		m_Aspect->Reconstitute(0,true);
	}

	// copy textures over from clone source
	m_Aspect->CopyTextures( *sourceAspect );
	m_Aspect->ClearInstanceAttrFlags( 0xFFFFFFFF );
	m_Aspect->SetInstanceAttrFlags( sourceAspect->GetInstanceAttrFlags() );

	// reset nema traits
	UpdateNemaProperties();
}

GoGizmo :: ~GoGizmo( void )
{
	// free resources
	gAspectStorage.ReleaseAspect( GetAspectHandle(), true );
}

nema::HAspect GoGizmo :: GetAspectHandle( void ) const
{
	gpassert( m_Aspect );
	return ( m_Aspect );
}

siege::eCollectFlags GoGizmo :: BuildCollectFlags( void ) const
{
	siege::eCollectFlags flags = siege::CF_ACTOR | siege::CF_ITEM | siege::CF_GIZMO;

	if ( gWorldOptions.GetShowGizmos() )
	{
		flags |= siege::CF_SELECTABLE;
	}

	if ( GetGo()->IsMouseShadowed() )
	{
		flags |= siege::CF_HIGHLIGHTED;
	}

	if ( GetGo()->IsRenderDirty() )
	{
		flags |= siege::CF_POSORIENTCHANGED;
	}

	return ( flags );
}

bool GoGizmo :: BuildRenderInfo( siege::ASPECTINFO& info ) const
{
	// can we see this?
	if ( (!GetIsVisible() || !gWorldOptions.GetShowGizmos()) && !gWorldOptions.GetShowAll() )
	{
		return ( false );
	}

#	if !GP_RETAIL
	if ( Services::IsEditor() && !GetGo()->GetEdit()->IsVisible() )
	{
		return ( false );
	}
#	endif // !GP_RETAIL

	// figure out position
	SiegePos pos = GetGo()->GetPlacement()->GetPosition();
	const Quat& quat = GetGo()->GetPlacement()->GetOrientation();

	// if we're also a goaspect, take the top of its box (plus a little more)
	GoAspect* aspect = GetGo()->QueryAspect();
	if ( aspect != NULL && aspect->GetIsVisible() && !aspect->GetForceNoRender() )
	{
		vector_3 center, halfDiag;
		aspect->GetNodeSpaceOrientedBoundingVolume( center, halfDiag );
		pos.pos.y = center.y + halfDiag.y + 0.2f;
	}

	// am i culled from the view frustum?
	GoAspect::GetWorldSpaceOrientedBoundingVolume( &info.BoundOrient, &info.BoundCenter, &info.BoundHalfDiag, pos, quat, GetAspectHandle().Get() );
	if( gSiegeEngine.GetCamera().GetFrustum().CullObject( info.BoundOrient, info.BoundCenter, info.BoundHalfDiag ) )
	{
		return ( false );
	}

	// fill out the info struct
	info.Object				= MakeInt( GetGoid() );
	info.Aspect				= GetAspectHandle().Get();
	info.MegamapIconTex     = 0;
	info.Origin				= pos.pos;
	info.ObjectScale		= m_Scale;
	info.BoundingBodyScale	= m_Scale;
	info.RenderIndex1		= 0;
	info.RenderIndex2		= 0;
	info.RenderIndex3		= 0;
	info.Alpha				= (BYTE)min_t( FTOL( m_Alpha * 255.0 ), 255 );

	// build orientation matrix for siege
	quat.BuildMatrix( info.Orientation );

	// all ok
	return ( true );
}

void GoGizmo :: SetIsVisible( bool set )
{  
	if ( m_IsVisible != set )
	{
		if (GetGo()->IsOmni() || GetGo()->IsInAnyWorldFrustum())
		{
			if ( !Services::IsEditor() )
			{
				if ( set )
				{
					// Reconstitute immediately
					GetAspectHandle()->Reconstitute(0,true);
				}
				else 
				{
					if (!GetGo()->IsFading())
					{
						GetAspectHandle()->Purge();
					}
				}
			}
		}	
	}

	m_IsVisible = set;  	
}

GoComponent* GoGizmo :: Clone( Go* newParent )
{
	return ( new GoGizmo( *this, newParent ) );
}

bool GoGizmo :: Xfer( FuBi::PersistContext& persist )
{
	bool success = true;

	// spec
	persist.Xfer( "is_visible",        m_IsVisible       );
	persist.Xfer( "scale",             m_Scale           );
	persist.Xfer( "alpha",             m_Alpha           );
	persist.Xfer( "diffuse_color",     m_DiffuseColor    );
	persist.Xfer( "use_diffuse_color", m_UseDiffuseColor );

	// xfer full
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_Aspect", m_Aspect );
		gpassert( m_Aspect.IsNull() || m_Aspect->ShouldPersist() );
	}
	else
	{
		// preprocess for saving
		gpstring modelName;
		if ( persist.IsSaving() )
		{
			modelName = FileSys::GetFileNameOnly( gAspectStorage.GetAspectName( GetAspectHandle() ) );
		}

		// xfer
		persist.Xfer( "model", modelName );

		// process the aspect if restoring, and it's different from current
		if (   persist.IsRestoring()
			&& (!m_Aspect || !gAspectStorage.GetAspectName( m_Aspect ).same_no_case( modelName )) )
		{
			// try actual model name
			gpstring modelFile;
			if ( !modelName.empty() && gNamingKey.BuildASPLocation( modelName, modelFile ) )
			{
				m_Aspect = gAspectStorage.LoadAspect( modelFile, modelName );
			}

			// failed? try default
			if ( !m_Aspect )
			{
				gperrorf(( "Placeholder model substitution - illegal model '%s' for Go at '%s'\n",
						   modelName.c_str(), GetGo()->DevGetFuelAddress().c_str() ));

				modelName = gContentDb.GetDefaultAspectName();
				if ( gNamingKey.BuildASPLocation( modelName, modelFile ) )
				{
					m_Aspect = gAspectStorage.LoadAspect( modelFile, modelName );
				}
			}

			// test for final failure
			if ( !m_Aspect )
			{
				// still failed? nothing will work then.
				gperrorf(( "Unable to load model '%s' for Go at '%s'\n",
						   modelName.c_str(), GetGo()->DevGetFuelAddress().c_str() ));
				success = false;
			}

			// $$$ Ask Chad if this reconstitute is correct too! --biddle
			if (success && Services::IsEditor())
			{
				m_Aspect->Reconstitute(0,false);
			}

		}

		if ( persist.IsRestoring() && success )
		{
			// $$$ use "unused param report" and dump set of extra fuel entries

			// set initial texture if any
			gpstring texture;
			persist.Xfer( "texture", texture );
			if ( texture.empty() && !m_UseDiffuseColor )
			{
				// use default if not using diffuse color
				texture = m_Aspect->GetDefaultTextureString( 0 );
			}

			if ( !texture.empty() )
			{
				// $$$$ somebody stop this copy-paste madness! uber-resource mgr will fix this problem -sb

				// tell nema to use it
				m_Aspect->ClearInstanceAttrFlags( nema::RENDER_DISABLE_TEXTURE );

				// resolve to a file
				gpstring texFile;
				if (   !gNamingKey.BuildIMGLocation( texture, texFile )
					|| !FileSys::DoesResourceFileExist( texFile ) )
				{
#					if ( !GP_RETAIL )
					{
						gpwarningf(( "Placeholder texture substitution: illegal texture '%s' for aspect '%s' in Go at '%s'\n",
									 texture.c_str(), gAspectStorage.GetAspectName( m_Aspect ).c_str(),
									 GetGo()->DevGetFuelAddress().c_str() ));
					}
#					endif // !GP_RETAIL

					texture = gContentDb.GetDefaultTextureName();
					if (   !gNamingKey.BuildIMGLocation( texture, texFile )
						|| !FileSys::DoesResourceFileExist( texFile ) )
					{
						gperrorf(( "Can't find placeholder texture '%s'! This is serious!\n", texture.c_str() ));
						texFile.clear();
					}
				}

				// set texture
				m_Aspect->SetTextureFromFile( 0, texFile );
			}
			else
			{
				// tell nema to use it
				m_Aspect->SetInstanceAttrFlags( nema::RENDER_DISABLE_TEXTURE );
			}

			// update alpha, color, etc.
			UpdateNemaProperties();
		}
	}

	return ( true );
}

void GoGizmo :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	switch ( msg.GetEvent() )
	{
		case ( WE_CONSTRUCTED ):
		{
			if ( (GetGo()->IsOmni() && GetIsVisible()) || Services::IsEditor() )
			{
				GetAspectHandle()->Reconstitute();
			}
		}
		break;

		case ( WE_ENTERED_WORLD ):
		{
			if ( GetIsVisible() || Services::IsEditor() )
			{
				if (!GetAspectHandle()->GetParent() || !GetAspectHandle()->GetParent()->IsPurged(2))
				{
					GetAspectHandle()->Reconstitute();
				}
			}			
		}
		break;

		case ( WE_LEFT_WORLD ):
		{			
			if (!GetGo()->IsFading())
			{
				GetAspectHandle()->Purge();
			}
		}
		break;
	}
}

bool GoGizmo :: CommitCreation( void )
{
	m_Aspect->SetGoid( GetGoid() );
	return ( true );
}

void GoGizmo :: UpdateNemaProperties( void )
{
	// set color
	if ( m_UseDiffuseColor )
	{
		m_Aspect->SetDiffuseColor( MAKEDWORDCOLOR( m_DiffuseColor ) );
		m_Aspect->SetInstanceAttrFlags( nema::RENDER_USE_COLOR );
	}
	else
	{
		m_Aspect->ClearInstanceAttrFlags( nema::RENDER_USE_COLOR );
	}
}

//////////////////////////////////////////////////////////////////////////////

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
