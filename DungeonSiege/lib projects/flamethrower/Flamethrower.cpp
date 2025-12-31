//
//
//	Flamethrower.cpp
//	Copyright 1999, Gas Powered Games
//
//

#include "precomp_flamethrower.h"

#include "vector_3.h"

#include "RapiImage.h"

#include "WorldFx.h"
#include "WorldTime.h"

#include "flamethrower_interpreter.h"
#include "flamethrower_effect_factory.h"
#include "flamethrower_effect_target.h"
#include "flamethrower_tracker.h"
#include "flamethrower_effect_base.h"
#include "flamethrower_effects.h"
#include "flamethrower_fubi_support.h"
#include "flamethrower_types.h"
#include "flamethrower_tracker.h"
#include "flamethrower.h"

#include "gpprofiler.h"
#include "gosupport.h"
#include "Siege_Camera.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"

//


inline bool FindObject( SFxEID id, SEMap &Coll, SEMap::iterator &iObj )
{
	iObj = Coll.find( id );
	return ( iObj != Coll.end() );
}

#define ProcessIDQueue( COLL, FUNC )	\
{ SFxEIDColl::iterator id = COLL.begin(), id_end = COLL.end(); if( !COLL.empty() ) \
{ for( ; id != id_end; ++id ) {	FUNC( *id ); } COLL.clear();}} \


//
// --- Flamethrower
//

Flamethrower::Flamethrower( void )
{
	m_LightRadiusScaleFactor= 0.45f;
	m_bInitialized = false;

	Rapi	&renderer = gWorldFx.GetRenderer();

	float anglesin, anglecos;

	// Constants for algo texture generation

	const int DEFAULT_TEX_WIDTH	 = 64;
	const int DEFAULT_TEX_HEIGHT = 64;
	const int DEFAULT_TEX_RADIUS = 32;

	//
	// Create some procedural textures
	//
	{	// Create generic global radial texture
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float brightness = 1.0f - (float)r/DEFAULT_TEX_RADIUS;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLOR( brightness, brightness, brightness ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx generic radial", image, false, 0, TEX_LOCKED, false, true ) );
	}

	{	// Create another very similar radial texture with different alpha (default for fire)
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float brightness = 1.0f - (float)r/DEFAULT_TEX_RADIUS;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA( brightness, brightness, brightness, brightness * 0.82f ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx fire", image, false, 0, TEX_LOCKED, true, true ) );
	}

	{	// Create another radial texture with some noise (default for steam)
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; r++ )
		{
			const float intensity = 1.0f - (float)r / DEFAULT_TEX_RADIUS;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA(
								 (0.75f - Random( 1.0f / 255.0f )) * intensity,
								 (0.75f - Random( 1.0f / 255.0f )) * intensity,
								 (0.75f - Random( 1.0f / 255.0f )) * intensity,
								 0.75f * intensity ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx steam", image, false, 0, TEX_LOCKED, true, true ) );
	}

	{	// Create a straight wrappable texture (default for lightning )
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );
		UINT32 t = 24;

		for( UINT32 x = 0; x < 64; ++x )
		{
			for( UINT32 y = 0; y < 64; ++y )
			{
				float c = 0.25f*SINF(((float)x/64)*RealPi);

				if(( x >t ) && (x < 64 -t ))
				{
					c += 0.75f *SINF( RealPi*( (float)(x-t) / (float)(64-2*t) ));
				}

				image->SetPixel( y, x, MAKEDWORDCOLOR( c, c, c ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx lightning", image, false, 0, TEX_LOCKED, false, true ) );
	}

	{	// Create another very similar radial texture with different alpha (default for fire)
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float val = 1.0f - (float)r/DEFAULT_TEX_RADIUS;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA( val, val, val, val ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx fire 2", image, false, 0, TEX_LOCKED, true, true ) );
	}

	{	// Dark texture for doing dark smoke
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float val = 0.27f * (1.0f - (float)r/DEFAULT_TEX_RADIUS) + 0.02f;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA( val, val, val, val ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx dark smoke", image, false, 0, TEX_LOCKED, true, true ) );
	}


	{	// Sort of noisy texture for explosion
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );
		const float Noise = 0.25f;
		const float NoiseConstant = 1.0f - Noise;

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float intensity = 1.0f - (float)r / DEFAULT_TEX_RADIUS;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_WIDTH*0.5f);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_WIDTH*0.5f);

				image->SetPixel( x, y, MAKEDWORDCOLOR(
								 FABSF( NoiseConstant * 0.75f - Random( Noise ) ) * intensity,
								 FABSF( NoiseConstant * 0.75f - Random( Noise ) ) * intensity,
								 FABSF( NoiseConstant * 0.75f - Random( Noise ) ) * intensity ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx explosion", image, false, 0, TEX_LOCKED, false, true ) );
	}


	{	// Dark red texture for blood
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float val = 0.27f * (1.0f - (float)r/DEFAULT_TEX_RADIUS) + 0.02f;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA( val, 0, 0, val ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx blood", image, false, 0, TEX_LOCKED, true, true ) );
	}

	{	// Create another texture for blood with near full alpha
		RapiMemImage* image = new RapiMemImage( DEFAULT_TEX_WIDTH, DEFAULT_TEX_HEIGHT, 0 );

		for( UINT32 r = 0; r < DEFAULT_TEX_RADIUS; ++r )
		{
			const float color = 0.475f * (1.0f - (float)r/DEFAULT_TEX_RADIUS) + 0.02f;
			const float alpha = 0.8f * (1.0f - (float)r/DEFAULT_TEX_RADIUS) + 0.02f;
			for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
			{
				SINCOSF( a, anglesin, anglecos );

				const UINT8 x = (UINT8)(r*anglecos+DEFAULT_TEX_RADIUS);
				const UINT8 y = (UINT8)(r*anglesin+DEFAULT_TEX_RADIUS);

				image->SetPixel( x, y, MAKEDWORDCOLORA( color, color, color, alpha ) );
			}
		}

		m_GlobalTextureNames.push_back( renderer.CreateAlgorithmicTexture( "sfx blood2", image, false, 0, TEX_LOCKED, true, true ) );
	}


	// Pre-calculate a global index table for 256 points

	const UINT16 array_end = (UINT16)(6*MAX_POINT_INDEX);

	for( UINT16 n = 0, j = 0; n < array_end; n+=6, j+=4 )
	{
		m_GlobalDrawIndexArray[n+0] = (UINT16)(0+j);
		m_GlobalDrawIndexArray[n+1] = (UINT16)(1+j);
		m_GlobalDrawIndexArray[n+2] = (UINT16)(3+j);
		m_GlobalDrawIndexArray[n+3] = (UINT16)(0+j);
		m_GlobalDrawIndexArray[n+4] = (UINT16)(3+j);
		m_GlobalDrawIndexArray[n+5] = (UINT16)(2+j);
	}
}


void
Flamethrower::Initialize( const char *sScriptFuelAddress )
{
	m_pIFactory	= new Flamethrower_effect_factory( m_InitializedEffects );
	m_pIInterpreter= new Flamethrower_interpreter( sScriptFuelAddress );

	m_bInitialized = true;
}


Flamethrower::~Flamethrower()
{
	Shutdown( true );

	// Destroy algorithmic textures used by Flamethrower
	Rapi &renderer = gWorldFx.GetRenderer();
	for( unsigned int i = 0; i < m_GlobalTextureNames.size(); ++i )
	{
		if( m_GlobalTextureNames[i] )
		{
			renderer.DestroyTexture( m_GlobalTextureNames[i] );
		}
	}
}


void
Flamethrower::Shutdown( bool bFullShutdown )
{
	if( m_bInitialized || bFullShutdown )
	{
		SEMap::iterator it = m_InitializedEffects.begin(), it_end = m_InitializedEffects.end();

		// destroy all the effects
		for( ; it != it_end; ++it )
		{
			delete (*it).second;
		}
		m_InitializedEffects.clear();

		// destroy all the unloaded effects
		for( it = m_DisabledEffects.begin(), it_end = m_DisabledEffects.end(); it != it_end; ++it )
		{
			delete (*it).second;
		}
		m_DisabledEffects.clear();

		// get rid of all the collsion information
		CollColl::iterator cit = m_CollisionRoster.begin(), cit_end = m_CollisionRoster.end();
		for( ; cit != cit_end; ++cit )
		{
			delete *cit;
		}
		m_CollisionRoster.clear();

		// for a full shutdown get rid of everything
		if( bFullShutdown )
		{
			delete m_pIFactory;
			delete m_pIInterpreter;

			m_bInitialized = false;
		}
		else
		{
			m_pIInterpreter->StopAllScripts( true );
			m_pIInterpreter->FlushQueues();
		}

		m_ActiveEffects.clear();
		m_EffectMap.clear();

		// flush all the request queues
		m_UnloadStack.clear();
		m_LoadStack.clear();
		m_StartStack.clear();
		m_StopStack.clear();
		m_DeathStack.clear();

	}
}


bool
Flamethrower::IsEmpty( void ) const
{
	return (   m_pIInterpreter         ->IsEmpty()
			&& m_DisabledEffects		.empty()
			&& m_InitializedEffects		.empty()
			&& m_ActiveEffects			.empty()
			&& m_EffectMap				.empty()
			&& m_StartStack				.empty()
			&& m_StopStack				.empty()
			&& m_UnloadStack			.empty()
			&& m_LoadStack				.empty()
			&& m_DeathStack				.empty()
			&& m_CollisionRoster		.empty()
			);
}


FUBI_DECLARE_SELF_TRAITS( Flamethrower_interpreter );

FUBI_DECLARE_XFER_TRAITS( CollisionInfo* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new CollisionInfo;
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_XFER_TRAITS( SpecialEffect* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		gpstring name;
		if ( persist.IsSaving() )
		{
			name = obj->GetName();
		}

		persist.Xfer( "_name", name );

		if ( persist.IsRestoring() )
		{
			obj = gWorldFx.CreateEffect( name );
		}

		return ( obj->Xfer( persist ) );
	}
};


void
Flamethrower::PrepareForSave( void )
{
	// go through and clean out all the script targets
	m_pIInterpreter->PrepareForSave();

	// clean unloaded effect targets and children
	{
		SEMap::iterator i = m_DisabledEffects.begin(), i_end = m_DisabledEffects.end();
		for(; i != i_end; ++i )
		{
			(*i).second->PrepareForSave();
			(*i).second->PrepareForSaveChildren();
		}
	}

	// clean initialized effect targets and children
	{
		SEMap::iterator i = m_InitializedEffects.begin(), i_end = m_InitializedEffects.end();
		for(; i != i_end; ++i )
		{
			(*i).second->PrepareForSave();
			(*i).second->PrepareForSaveChildren();
		}
	}

	// clean deleted collisions
	{
		for ( CollColl::iterator i = m_CollisionRoster.begin() ; i != m_CollisionRoster.end() ; )
		{
			if ( !::IsValid( (*i)->GetOwnerID() ) )
			{
				delete ( *i );
				i = m_CollisionRoster.erase( i );
			}
			else
			{
				++i;
			}
		}
	}
}


bool
Flamethrower::Xfer( FuBi::PersistContext& persist )
{
	persist.XferIfMap( "m_DisabledEffects",    m_DisabledEffects,    FxShouldPersistSecond() );
	persist.XferIfMap( "m_InitializedEffects", m_InitializedEffects, FxShouldPersistSecond() );

	SFxEIDColl	id_list;
	if( persist.IsRestoring() )
	{
		persist.XferVector( FuBi::XFER_HEX, "_active_ids", id_list );

		SFxEIDColl::iterator iID = id_list.begin(), i_end = id_list.end();
		SEMap::iterator iEffect;
		for(; iID != i_end; ++iID )
		{
			FindObject( *iID, m_InitializedEffects, iEffect );
			m_ActiveEffects.insert( *iEffect );
		}
	}
	else
	{
		SEMap::iterator iID = m_ActiveEffects.begin(), i_end = m_ActiveEffects.end();
		for(; iID != i_end; ++iID )
		{
			if ( (*iID).second->ShouldPersist() )
			{
				id_list.push_back( (*iID).second->GetID() );
			}
		}
		persist.XferVector( FuBi::XFER_HEX, "_active_ids", id_list );
	}

	persist.XferIfVector( "m_StartStack",             m_StartStack             , FxShouldPersistHelper() );
	persist.XferIfVector( "m_StopStack",              m_StopStack              , FxShouldPersistHelper() );
	persist.XferIfVector( "m_UnloadStack",            m_UnloadStack            , FxShouldPersistHelper() );
	persist.XferIfVector( "m_LoadStack",              m_LoadStack              , FxShouldPersistHelper() );
	persist.XferIfVector( "m_DeathStack",             m_DeathStack             , FxShouldPersistHelper() );

	persist.XferVector  ( "m_CollisionRoster",        m_CollisionRoster        );

	persist.Xfer        ( "m_LightRadiusScaleFactor", m_LightRadiusScaleFactor );

	persist.Xfer        ( "m_pIInterpreter",          *m_pIInterpreter         );

	return ( true );
}


bool
Flamethrower::DestroyScript( const char *sScriptName )
{
	return m_pIInterpreter->DestroyScript( sScriptName );
}


void
Flamethrower::RebuildScriptVocabulary()
{
	m_pIInterpreter->RebuildScriptVocabulary();
}


SFxSID
Flamethrower::RunScript( const char *sName, def_tracker targets, const char *sParams, Goid ScriptOwner, eWorldEvent type, bool server, bool xfer )
{
	return m_pIInterpreter->RunScript( sName, targets, sParams, ScriptOwner, type, server, xfer );
}

Goid
Flamethrower::GetRegistrationGoid( SFxSID id )
{
	return m_pIInterpreter->GetRegistrationGoid( id );
}


void
Flamethrower::FinishScript( SFxSID id )
{
	m_pIInterpreter->FinishScript( id );
}


void
Flamethrower::SStopScript( SFxSID id )
{
	m_pIInterpreter->SStopScript( id );
}

void
Flamethrower::SStopScript( Goid id, const char *sName )
{
	m_pIInterpreter->SStopScript( id, sName );
}

void
Flamethrower::StopScript( SFxSID id )
{
	m_pIInterpreter->StopScript( id );
}


void
Flamethrower::StopScript( Goid id, const char *sName )
{
	m_pIInterpreter->StopScript( id, sName );
}


void
Flamethrower::StopAllScripts()
{
	m_pIInterpreter->StopAllScripts();
}


void
Flamethrower::UnloadScript( SFxSID id )
{
	m_pIInterpreter->UnloadScript( id );
}


void
Flamethrower::LoadScript( SFxSID id )
{
	m_pIInterpreter->LoadScript( id );
}


bool
Flamethrower::SetScriptCallback( SFxSID id, const CBFunctor1< SFxSID > callback )
{
	return m_pIInterpreter->SetScriptCallback( id, callback );
}


void
Flamethrower::PostSignal( SFxSID id, const gpstring &sSignal )
{
	m_pIInterpreter->PostSignal( id, sSignal );
}


bool
Flamethrower::FindScript( SFxSID id, Flamethrower_script **pScript )
{
	return m_pIInterpreter->FindScript( id, pScript );
}


SFxEID
Flamethrower::CreateEffect( const char *sName, const char *sParam, def_tracker targets, Goid owner_id, SFxSID script_id, UINT32 seed )
{
	SpecialEffect *pNewEffect;

	if( m_pIFactory->MakeEffect( sName, &pNewEffect ) )
	{
		pNewEffect->SetParams( GetDefaultEffectParams( sName ) );
		if( pNewEffect->GetParams().ParseAndStore( sParam ) )
		{
			pNewEffect->SetOwnerID			( owner_id	 );
			pNewEffect->OwnTargets			( targets	 );
			pNewEffect->SetParentScriptID	( script_id  );
			pNewEffect->SetID				( m_pIInterpreter->GetNextEffectID( script_id ) );

			pNewEffect->Initialize			( seed );

			m_InitializedEffects.insert( std::make_pair( pNewEffect->GetID(), pNewEffect ) );

			return pNewEffect->GetID();
		}
		else
		{
			delete pNewEffect;
		}
	}
	return SFxEID_INVALID;
}


SpecialEffect*
Flamethrower::CreateEffect( const char *sName )
{
	SpecialEffect *pNewEffect = NULL;
	m_pIFactory->MakeEffect( sName, &pNewEffect );

	return pNewEffect;
}


void
Flamethrower::AddTarget( SFxEID id, Flamethrower_target *pTarget )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->AddTarget( pTarget );
		(*effect).second->SetInterestOnly( pTarget->GetInterestOnly() );
	}
}


void
Flamethrower::AddEffectTarget( SFxEID id, SFxEID target_id )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		Flamethrower_effect_target	*pTarget = new Flamethrower_effect_target( target_id );

		(*effect).second->AddTarget( pTarget );
	}
}


void
Flamethrower::RemoveAllTargets( SFxEID id )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->RemoveAllTargets();
	}
}


bool
Flamethrower::GetTargets( SFxEID id, TattooTracker **pTracker )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->GetTargets( pTracker );
		return true;
	}
	return false;
}


void
Flamethrower::SetTargets( SFxEID id, def_tracker target )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->OwnTargets( target );
	}
}


void
Flamethrower::AddFriendly( SFxEID effect_id, Goid goid_id )
{
	SEMap::iterator effect;
	if( FindObject( effect_id, m_InitializedEffects, effect ) )
	{
		(*effect).second->AddFriendly( goid_id );
	}
}


void
Flamethrower::StartEffect( SFxEID id )
{
	m_StartStack.push_back( id );
}


void
Flamethrower::StartAllEffects()
{
	SEMap::iterator it = m_InitializedEffects.begin(), it_end = m_InitializedEffects.end();
	for( ; it != it_end; ++it )
	{
		StartEffect( (*it).second->GetID() );
	}
}


void
Flamethrower::SetFinishing( SFxEID effect_id )
{
	SEMap::iterator effect;
	if( FindObject( effect_id, m_InitializedEffects, effect ) )
	{
		(*effect).second->SetFinishing();
	}

}


void
Flamethrower::StopEffect( SFxEID id )
{
	m_StopStack.push_back( id );
}


void
Flamethrower::StopAllEffects()
{
	SEMap::iterator it = m_ActiveEffects.begin(), it_end = m_ActiveEffects.end();
	for( ; it != it_end; ++it )
	{
		StopEffect( (*it).second->GetID() );
	}
}


void
Flamethrower::DestroyEffect( SFxEID id )
{
	m_DeathStack.push_back( id );
}


void
Flamethrower::DestroyAllEffects()
{
	SEMap::iterator it = m_InitializedEffects.begin(), it_end = m_InitializedEffects.end();
	for( ; it != it_end; ++it )
	{
		DestroyEffect( (*it).second->GetID() );
	}
}


void
Flamethrower::UnloadEffect( SFxEID id )
{
	m_UnloadStack.push_back( id );
}


void
Flamethrower::LoadEffect( SFxEID id )
{
	m_LoadStack.push_back( id );
}


SpecialEffect*
Flamethrower::FindEffect( SFxEID id )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		return (*effect).second;
	}
	else if( FindObject( id, m_DisabledEffects, effect ) )
	{
		return (*effect).second;
	}

	return NULL;
}


bool
Flamethrower::GetPosition( SFxEID id, SiegePos &pos )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		pos = (*effect).second->GetPosition();
		return true;
	}
	return false;
}


bool
Flamethrower::SetPosition( SFxEID id, const SiegePos &pos )
{
	gpassert( pos.node.IsValid() );

	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->SetPosition( pos );
		return true;
	}
	return false;
}


bool
Flamethrower::GetOrientation( SFxEID id, matrix_3x3 &orientation )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		orientation = (*effect).second->GetOrientation();
		return true;
	}
	return false;
}


bool
Flamethrower::SetOrientation( SFxEID id, const matrix_3x3 &orientation )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->SetOrientation( orientation );
		return true;
	}
	return false;
}


bool
Flamethrower::GetDirection( SFxEID id, vector_3 &dir )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		dir = (*effect).second->GetDirection();
		return true;
	}
	return false;
}


bool
Flamethrower::SetDirection( SFxEID id, const vector_3 &dir )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		(*effect).second->SetDirection( dir );
		return true;
	}
	return false;
}


bool
Flamethrower::OffsetPosition( SFxEID id, vector_3 &offset, matrix_3x3 &basis )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		vector_3 new_offset( basis * offset );

		SiegePos	new_pos( (*effect).second->GetPosition() );
		new_pos.pos += new_offset;

		(*effect).second->SetPosition( new_pos );
		return true;
	}
	return false;
}


bool
Flamethrower::OffsetTargetPosition( SFxEID id, const TattooTracker::TARGET_INDEX n,
											const vector_3 &offset )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		TattooTracker *pTracker;

		(*effect).second->GetTargets( &pTracker );

		pTracker->SetPositionOffset( offset, n );
		return true;
	}
	return false;
}


bool
Flamethrower::MakeTargetsStatic( SFxEID id, bool snap_to_ground )
{
	// Find the effect from the specified ID in the initialized effects
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) && (*effect).second->HasTargets() )
	{
		// Own the effects tracker and targets
		def_tracker old_targets = (*effect).second->ReleaseTargets();

		// Prepare a new tracke to put the static targets into
		def_tracker	new_targets = WorldFx::MakeTracker();

		// Create storage for a reference
		Goid object_to_reg_with = GOID_INVALID;

		// Iterate through each target and make a static version of it
		TattooTracker::TargetColl::iterator i, ibegin = old_targets->GetTargetsBegin(), iend = old_targets->GetTargetsEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Flamethrower_target	*pOldTarget	= *i;
			Flamethrower_target *pNewTarget = NULL;

			// Convert the target
			if( ConvertTargetToStatic( pOldTarget, pNewTarget, snap_to_ground ) )
			{
				// Add the target to the new tracker
				new_targets->SetTarget( pNewTarget, (TattooTracker::TARGET_INDEX)i.GetIndex() );

				// Record what go this target was registered to
				object_to_reg_with = MakeGoid( pNewTarget->GetID() );
			}
			else
			{
				return false;
			}
		}

		// Now that the tracker is prepared transfer ownership to the effect
		(*effect).second->OwnTargets( new_targets );

		return true;
	}
	return false;
}


bool
Flamethrower::ConvertTargetToStatic( Flamethrower_target *pOldTarget, Flamethrower_target *&pNewTarget, bool snap_to_ground )
{
	// Setup storage for the attributes of the target that is being converted
	matrix_3x3	orientation;
	vector_3	offset;
	SiegePos	position;
	vector_3	direction;
	float		radius = 0;

	if( pOldTarget->GetPlacement( &position, &orientation ) &&
		pOldTarget->GetDirection( direction ) && pOldTarget->GetPositionOffset( offset ) &&
		pOldTarget->GetBoundingRadius( radius ) )
	{
		// Adjust position to the ground if specified to do so
		if( snap_to_ground )
		{
			gWorldFx.AdjustPointToTerrain( position );
		}

		// Create the new static target and set initial position
		pNewTarget = Flamethrower_target::CreateByType( Flamethrower_target::STATIC, position );

		// Set the other remaining properties
		pNewTarget->SetDirection( direction );
		pNewTarget->SetPositionOffset( offset );
		pNewTarget->SetBoundingRadius( radius );

		return true;
	}
	return false;
}


def_tracker
Flamethrower::MakeTrackerTargetsStatic( def_tracker old_targets, bool snap_to_ground )
{
	// Prepare a new tracke to put the static targets into
	def_tracker	new_targets = WorldFx::MakeTracker();

	// Iterate through each target and make a static verion of it
	TattooTracker::TargetColl::iterator i, ibegin = old_targets->GetTargetsBegin(), iend = old_targets->GetTargetsEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Flamethrower_target	*pOldTarget	= *i;
		Flamethrower_target *pNewTarget = NULL;

		// Convert the target
		ConvertTargetToStatic( pOldTarget, pNewTarget, snap_to_ground );

		// Add the target to the new tracker
		new_targets->SetTarget( pNewTarget, (TattooTracker::TARGET_INDEX)i.GetIndex() );
	}

	return new_targets;
}


bool
Flamethrower::GetBoundingRadius( SFxEID id, float &radius )
{
	SEMap::iterator effect;
	if( FindObject( id, m_InitializedEffects, effect ) )
	{
		radius = (*effect).second->GetBoundingRadius();
		return true;
	}
	return false;
}


bool
Flamethrower::AttachEffect( SFxEID parent_id, SFxEID child_id )
{
	if( (!parent_id) || (!child_id) || ( parent_id == child_id ) )
	{
		return false;
	}

	SEMap::iterator it_parent;
	if( FindObject( parent_id, m_InitializedEffects, it_parent ) )
	{
		SEMap::iterator it_child;
		if( FindObject( child_id, m_InitializedEffects, it_child ) )
		{
			(*it_parent).second->AttachEffect( child_id );
			(*it_child).second->SetParentEffectID( parent_id );
			return true;
		}
	}
	return false;
}


void
Flamethrower::Update( float SecondsElapsed, float UnpausedSecondsElapsed )
{
	GPPROFILERSAMPLE("Flamethrower Master Update", SP_EFFECTS );

	// make sure that we don't update our effects too much.  if we are
	// getting a really bad framerate, we should not be updating so much.
	if( SecondsElapsed > 1 )
	{
		SecondsElapsed = 1.0f;
	}

	if( UnpausedSecondsElapsed > 1 )
	{
		UnpausedSecondsElapsed = 1.0f;
	}

	// Update the interpreter at 30fps
	float ts = ( 0.0333333f > SecondsElapsed ) ? SecondsElapsed : 0.0333333f;

	// Update scripts
	if ( SecondsElapsed > 0 )
	{
		for( float tr = SecondsElapsed; tr > 0; tr -= ts )
		{
			const float dt = ( tr < ts ) ? tr : ts;
			m_pIInterpreter->Update( dt, UnpausedSecondsElapsed );
		}
	}
	else
	{
		// just run one iter to process script deletions etc.
		m_pIInterpreter->Update( 0, 0 );
	}

	ProcessIDQueue( m_UnloadStack,	UnloadEffectNow		);	// Process Unloadable effects
	ProcessIDQueue( m_LoadStack,	LoadEffectNow		);	// Process Loadable effects
	ProcessIDQueue( m_StartStack,	StartEffectNow		);	// Process startable effects
	ProcessIDQueue( m_StopStack,	StopEffectNow		);	// Process stoppable effects
	ProcessIDQueue( m_DeathStack,	DestroyEffectNow	);	// Process destroyable effects

	// Process Collision roster
	if( !m_CollisionRoster.empty() )
	{
		CollColl::iterator it = m_CollisionRoster.begin();

		while( it != m_CollisionRoster.end() )
		{
			if( (*it)->LapseTimer( SecondsElapsed ) < 0 )
			{
				delete *it;
				it = m_CollisionRoster.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

#if !GP_RETAIL
	m_UpdatedEffects = 0;
#endif

	// Clear out the old list of effects so the loop below can rebuild it
	m_EffectMap.clear();

	// Update individual effects
	SEMap::iterator it = m_ActiveEffects.begin(), it_end = m_ActiveEffects.end();
	for(; it != it_end; ++it )
	{
		const float min_fps = (*it).second->GetMinFPS();
		SpecialEffect* effect = (*it).second;
		GoHandle owner( effect->GetOwnerID() );

		if( ( owner == NULL ) || !owner->IsLodfi() || owner->WasInViewFrustumRecently() )
		{
			ts = ( min_fps > SecondsElapsed ) ? SecondsElapsed : min_fps;

			if( effect->GetParentEffectID() == SFxEID_INVALID )
			{
				// get child info
				SFxEIDColl &children = effect->GetChildren();

				for( float tr = SecondsElapsed; tr > 0; tr -= ts )
				{
					// adjust for really small slices
					const float dt = ( tr < min_fps ) ? tr : min_fps;

					// update the parent
					effect->Update( dt, UnpausedSecondsElapsed );

					// update the children
					SFxEIDColl::iterator ch = children.begin(), ch_end = children.end();
					for( ; ch != ch_end; ++ch )
					{
						SEMap::iterator child;
						if( FindObject( *ch, m_ActiveEffects, child ) )
						{
							(*child).second->Update( dt, UnpausedSecondsElapsed );
						}
					}
				}
			}

			if( effect->IsFinished() )
			{
				DestroyEffect( effect->GetID() );
			}
			else if ( effect->ShouldDraw() && effect->GetTargets()->IsValid() )
			{
				const SiegePos & TargetPos	= effect->GetTargetPosition();
				const SiegePos & SourcePos	= effect->GetTargetPosition( TattooTracker::SOURCE );
				SiegePos CullPos;

				if( effect->GetCullPosition( CullPos ) )
				{
					if( CullPos.node == TargetPos.node )
					{
						if( CullPos.node == SourcePos.node )
						{
							// all target nodes are the same
							// Insert ONCE - CullPos.node
							InsertForCollection( effect, CullPos );
						}
						else
						{
							// cull and source position are different
							// insert TWICE - CullPos.node, SourcePos.node
							InsertForCollection( effect, CullPos );
							InsertForCollection( effect, SourcePos );
						}
					}
					else if( CullPos.node != SourcePos.node )
					{
						// all target nodes are different
						// insert THRICE - CullPos.node, SourcePos.node, TargetPos.node
						InsertForCollection( effect, CullPos );
						InsertForCollection( effect, SourcePos );
						InsertForCollection( effect, TargetPos );
					}
				}
				else
				{
					// cull position was invalid so just insert either target, source, or both
					InsertForCollection( effect, TargetPos );

					if( TargetPos.node != SourcePos.node )
					{
						InsertForCollection( effect, SourcePos );
					}
				}
			}
		}
	}
}


void
Flamethrower::InsertForCollection( SpecialEffect *effect, const SiegePos & pos )
{
	def_EffectMap::iterator ieffect = m_EffectMap.find( pos.node );

	if( ieffect == m_EffectMap.end() )
	{
		ieffect = m_EffectMap.insert( std::make_pair( pos.node, SEDrawColl() ) ).first;
	}

	SEDrawColl& drawColl = (*ieffect).second;
	drawColl.insert( EffectDrawEntry( effect, pos.pos.y ) );
}


void
Flamethrower::CollectEffectsToDraw( siege::WorldEffectColl &collection, const siege::SiegeNode &node )
{
	// Based on the node that has been passed in find out which efffects are in that node and push
	// them onto the collection that is also passed in after resolving camera distance and creating
	// the blind callback

	def_EffectMap::iterator i = m_EffectMap.find( node.GetGUID() );

	if( i != m_EffectMap.end() )
	{
		for( SEDrawColl::iterator j = (*i).second.begin(), j_end = (*i).second.end(); j != j_end; ++j )
		{
			SpecialEffect* effect = j->m_Effect;

			if( !effect->GetMarkedForDraw() && gSiegeEngine.IsNodeValid( effect->GetPosition().node ) )
			{
				if( gWorldFx.IsEffectVisible( effect ) )
				{
					effect->SetMarkedForDraw();

					siege::EFFECTINFO* pEffect = &*collection.push_back();

					pEffect->CameraDistance = Length( gWorldFx.GetCameraPosition() - effect->GetPosition().WorldPos() );
					pEffect->RenderCallback = makeFunctor( *effect, &SpecialEffect::Draw );

					if( !effect->GetUpdated() )
					{
						effect->Update( 0, 0 );
					}
					effect->SetIsVisible( true );
					effect->SetNodeAlpha( ( (float)node.GetNodeAlpha() / 255.0f ) );
				}
			}
		}
	}
}


unsigned int
Flamethrower::GetGlobalTexture( UINT32 texture_id )
{
	return (texture_id >= m_GlobalTextureNames.size()) ? 0 : m_GlobalTextureNames[ texture_id ];
}


bool
Flamethrower::EffectHasCollided( SFxEID id, CollisionInfo &info )
{
	CollisionInfo *pCollTemp = NULL;
	if( FindCollision( id, &pCollTemp ) )
	{
		info = *pCollTemp;
		return true;
	}
	return false;
}


void
Flamethrower::PostCollision( CollisionInfo *pCollision )
{
	gWorldFx.PostCollision( pCollision );
}


bool
Flamethrower::GetCollisionPoint( SFxEID id, SiegePos &point )
{
	CollisionInfo *pCollTemp = NULL;
	if( FindCollision( id, &pCollTemp ) )
	{
		point = pCollTemp->GetCollisionPoint();
		return true;
	}
	return false;
}


bool
Flamethrower::GetCollisionNormal( SFxEID id, SiegePos &normal )
{
	CollisionInfo *pCollTemp = NULL;
	if( FindCollision( id, &pCollTemp ) )
	{
		normal = pCollTemp->GetCollisionNormal();
		return true;
	}
	return false;

}


bool
Flamethrower::GetCollisionDirection( SFxEID id, vector_3 &dir )
{
	CollisionInfo *pCollTemp = NULL;
	if( FindCollision( id, &pCollTemp ) )
	{
		dir = pCollTemp->GetCollisionDirection();
		return true;
	}
	return false;

}


bool
Flamethrower::GetCollisionTargetID( SFxEID id, Goid & target_id )
{
	CollisionInfo *pCollTemp = NULL;
	if( FindCollision( id, &pCollTemp ) )
	{
		target_id = pCollTemp->GetTargetID();
		return true;
	}
	return false;

}


EffectParams&
Flamethrower::GetDefaultEffectParams( const char *sName )
{
	def_EParamMap::iterator iParam = m_DefaultEffectParams.find( sName );

	if( iParam != m_DefaultEffectParams.end() )
	{

		return (*iParam).second;
	}

	std::pair< def_EParamMap::iterator, bool > new_param = m_DefaultEffectParams.insert( std::make_pair( sName, EffectParams() ) );

	return new_param.first->second;
}



#if !GP_RETAIL


gpstring
Flamethrower::GetActiveEffects() const
{
	if( m_InitializedEffects.empty() && m_ActiveEffects.empty() && m_DisabledEffects.empty() )
	{
		return "No Active effects\n";
	}

	gpstring sOutput;
	SEMap::const_iterator	iEffect = m_InitializedEffects.begin(), iEnd = m_InitializedEffects.end();

	if( !m_InitializedEffects.empty() )
	{
		sOutput += "--Initialized Effects:\n";
		for( ; iEffect != iEnd; ++iEffect )
		{
			sOutput.appendf( "id: %u - [%s]\n", (*iEffect).second->GetID(), (*iEffect).second->GetName().c_str() );
		}
		sOutput.appendf( "Initialized effects = %d\n", m_ActiveEffects.size() );
	}

	if( !m_ActiveEffects.empty() )
	{
		sOutput += "--Active Effects:\n";

		UINT32 draw_count = 0;

		for( iEffect = m_ActiveEffects.begin(), iEnd = m_ActiveEffects.end(); iEffect != iEnd; ++iEffect )
		{
			sOutput.appendf( "id: %u - [%s]\n", (*iEffect).second->GetID(), (*iEffect).second->GetName().c_str() );

			if( (*iEffect).second->GetIsVisible() )
			{
				++draw_count;
			}
		}
		sOutput.appendf( "Active effects  = %d\n", m_ActiveEffects.size() );
		sOutput.appendf( "Updated effects = %d\n", m_UpdatedEffects );
		sOutput.appendf( "Effects drawn   = %d\n", draw_count );
	}

	if( !m_DisabledEffects.empty() )
	{
		sOutput.appendf( "Unloaded effects = %d\n", m_DisabledEffects.size() );
	}

	return sOutput;
}


gpstring
Flamethrower::GetActiveScripts() const
{
	return m_pIInterpreter->GetActiveScripts();
}


gpstring
Flamethrower::GetAvailableEffects() const
{
	return m_pIFactory->GetAvailableEffects();
}


gpstring
Flamethrower::GetAvailableScripts() const
{
	return m_pIInterpreter->GetAvailableScripts();
}


#endif // !GP_RETAIL


//
// Privately declared
//
bool
Flamethrower::FindCollision( SFxEID id, CollisionInfo **pCollision )
{
	CollColl::iterator it = m_CollisionRoster.begin(), it_end = m_CollisionRoster.end();

	for( ; it != it_end; ++it )
	{
		if( (*it)->GetID() == id )
		{
			*pCollision = *it;
			return true;
		}
	}
	return false;
}


void
Flamethrower::StartEffectNow( SFxEID id )
{
	SEMap::iterator iParent;

	if( FindObject( id, m_ActiveEffects, iParent ) )
	{
		return;
	}

	if( FindObject( id, m_InitializedEffects, iParent ) && (*iParent).second->HasTargets() )
	{
		m_ActiveEffects.insert( *iParent );
		iParent->second->Update( 0, 0 );

		if( (*iParent).second->HasChildren() )
		{
			SFxEIDColl childIDs;
			(*iParent).second->GetChildren( childIDs );

			SEMap::iterator kid;
			SFxEIDColl::iterator childID, childID_end;
			for( childID = childIDs.begin(), childID_end = childIDs.end(); childID != childID_end; ++childID )
			{
				if( FindObject( *childID, m_InitializedEffects, kid ) )
				{
					m_ActiveEffects.insert( *kid );
					kid->second->Update( 0, 0 );
				}
			}
		}
	}
}


void
Flamethrower::StopEffectNow( SFxEID id )
{
	SEMap::iterator iParent;

	if( FindObject( id, m_ActiveEffects, iParent ) )
	{
		SFxEIDColl childIDs;
		(*iParent).second->GetChildren( childIDs );
		m_ActiveEffects.erase( iParent );

		if( !childIDs.empty() )
		{
			SEMap::iterator kid;
			SFxEIDColl::iterator childID, childID_end;
			for( childID = childIDs.begin(), childID_end = childIDs.end(); childID != childID_end; ++childID )
			{
				if( FindObject( *childID, m_ActiveEffects, kid ) )
				{
					m_ActiveEffects.erase( kid );
				}
			}
		}
	}
}


void
Flamethrower::DestroyEffectNow( SFxEID id )
{
	StopEffectNow( id );

	SEMap::iterator iParent;

	if( FindObject( id, m_InitializedEffects, iParent ) )
	{
		DoEffectDestruction( iParent, m_InitializedEffects );
	}
	else if( FindObject( id, m_DisabledEffects, iParent ) )
	{
		DoEffectDestruction( iParent, m_DisabledEffects );
	}
}


void
Flamethrower::DoEffectDestruction( SEMap::iterator iParent, SEMap &list )
{
	SFxEIDColl childIDs;
	(*iParent).second->GetChildren( childIDs );
	(*iParent).second->NotifyScript();

	SEMap::iterator iEffectParent;

	// If this effect is the child of another remove it from the parents list of children
	if( FindObject( (*iParent).second->GetParentEffectID(), m_InitializedEffects, iEffectParent ) )
	{
		(*iEffectParent).second->RemoveEffect( (*iParent).second->GetID() );
	}

	delete (*iParent).second;
	list.erase( iParent );

	if( !childIDs.empty() )
	{
		SEMap::iterator kid;
		SFxEIDColl::iterator childID, childID_end;
		for( childID = childIDs.begin(), childID_end = childIDs.end(); childID != childID_end; ++childID )
		{
			if( FindObject( *childID, list, kid ) )
			{
				(*kid).second->NotifyScript();
				delete (*kid).second;
				list.erase( kid );
			}
		}
	}
}


void
Flamethrower::UnloadEffectNow( SFxEID id )
{
	SEMap::iterator iParent;

	if( FindObject( id, m_ActiveEffects, iParent ) )
	{
		SFxEIDColl childIDs;
		(*iParent).second->GetChildren( childIDs );
		(*iParent).second->UnInitialize();

		m_DisabledEffects.insert( *iParent );
		m_ActiveEffects.erase( iParent );

		if( FindObject( id, m_InitializedEffects, iParent ) )
		{
			m_InitializedEffects.erase( iParent );
		}

		if( !childIDs.empty() )
		{
			SEMap::iterator kid;
			SFxEIDColl::iterator childID, childID_end;
			for( childID = childIDs.begin(), childID_end = childIDs.end(); childID != childID_end; ++childID )
			{
				if( FindObject( *childID, m_ActiveEffects, kid ) )
				{
					m_DisabledEffects.insert( *kid );
					m_ActiveEffects.erase( kid );
				}
				if( FindObject( *childID, m_InitializedEffects, kid ) )
				{
					m_InitializedEffects.erase( kid );
				}
			}
		}
	}
}


void
Flamethrower::LoadEffectNow( SFxEID id )
{
	SEMap::iterator iParent;

	if( FindObject( id, m_DisabledEffects, iParent ) )
	{
		SFxEIDColl childIDs;
		(*iParent).second->GetChildren( childIDs );

		m_InitializedEffects.insert( *iParent );
		m_ActiveEffects.insert( *iParent );
		iParent->second->Update( 0, 0 );

		m_DisabledEffects.erase( iParent );

		if( !childIDs.empty() )
		{
			SEMap::iterator kid;
			SFxEIDColl::iterator childID, childID_end;
			for( childID = childIDs.begin(), childID_end = childIDs.end(); childID != childID_end; ++childID )
			{
				if( FindObject( *childID, m_DisabledEffects, kid ) )
				{
					m_InitializedEffects.insert( *kid );
					m_ActiveEffects.insert( *kid );
					kid->second->Update( 0, 0 );

					m_DisabledEffects.erase( kid );
				}
			}
		}
	}
}



//
// --- EffectParams stuff
//


UINT32	EffectParams ::	SetBool	( const char* sParamName, bool value )
{
	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	bool bNewVariable = true;
	for(; it != it_end; ++it, ++index )
	{
		if( (*it).m_sParamName.same_no_case( sParamName ) )
		{
			gpassert( ((*it).m_Type == PT_BOOL) || ((*it).m_Type == PT_INVALID) );

			(*it).m_Type		= PT_BOOL;
			(*it).m_bool		= value;

			bNewVariable = false;
			break;
		}
	}

	if( bNewVariable )
	{
		m_Params.push_back( effect_param( sParamName, value ) );
	}

	return index;
}

UINT32	EffectParams ::	SetUINT32 ( const char* sParamName, UINT32 value )
{
	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	bool bNewVariable = true;
	for(; it != it_end; ++it, ++index )
	{
		if( (*it).m_sParamName.same_no_case( sParamName ) )
		{
			gpassert( ((*it).m_Type == PT_UINT32) || ((*it).m_Type == PT_INVALID) );

			(*it).m_Type		= PT_UINT32;
			(*it).m_UINT32		= value;

			bNewVariable = false;
			break;
		}
	}

	if( bNewVariable )
	{
		m_Params.push_back( effect_param( sParamName, value ) );
	}

	return index;
}

UINT32	EffectParams ::	SetFloat ( const char* sParamName, float value )
{
	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	bool bNewVariable = true;
	for(; it != it_end; ++it, ++index )
	{
		if( (*it).m_sParamName.same_no_case( sParamName ) )
		{
			gpassert( ((*it).m_Type == PT_FLOAT) || ((*it).m_Type == PT_INVALID) );

			(*it).m_Type		= PT_FLOAT;
			(*it).m_float		= value;

			bNewVariable = false;
			break;
		}
	}

	if( bNewVariable )
	{
		m_Params.push_back( effect_param( sParamName, value ) );
	}

	return index;
}

UINT32	EffectParams ::	SetVector3 ( const char* sParamName, float value_x, float value_y, float value_z )
{
	vector_3 new_value(value_x, value_y, value_z );

	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	bool bNewVariable = true;
	for(; it != it_end; ++it, ++index )
	{
		if( (*it).m_sParamName.same_no_case( sParamName ) )
		{
			gpassert( ((*it).m_Type == PT_VECTOR_3) || ((*it).m_Type == PT_INVALID) );

			(*it).m_Type		= PT_VECTOR_3;
			(*it).m_vector_3	= new_value;

			bNewVariable = false;
			break;
		}
	}

	if( bNewVariable )
	{
		m_Params.push_back( effect_param( sParamName, vector_3(value_x, value_y, value_z) ) );
	}

	return index;
}

UINT32	EffectParams ::	SetGPString ( const char* sParamName, const char* value )
{
	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	bool bNewVariable = true;
	for(; it != it_end; ++it, ++index )
	{
		if( (*it).m_sParamName.same_no_case( sParamName ) )
		{
			gpassert( ((*it).m_Type == PT_GPSTRING) || ((*it).m_Type == PT_INVALID) );

			(*it).m_Type		= PT_GPSTRING;
			(*it).m_gpstring	= value;

			bNewVariable = false;
			break;
		}
	}

	if( bNewVariable )
	{
		m_Params.push_back( effect_param( sParamName, value ) );
	}

	return index;
}


bool EffectParams :: ParseAndStore ( const char* sParams )
{
	// Get the length
	const size_t plen = ::strlen( sParams ) + 1;

	// Protect against having to large of a param
	if( plen > 1024 )
	{
		gperrorf(("Effect %s is specifying too many parameters\n", GetEffectName()));
		return false;
	}

	// Make a copy so it can be modified
	char* pParams = (char* )_alloca( plen );
	::memcpy( pParams, (void*)(sParams), plen );

	// skip the - if there is one
	for(;(*pParams == '-') ; ++pParams);

	// Init the runners
	char* pNext	 = pParams;

	// Parse the entire param string
	while( *pNext != 0  )
	{
		// Skip any leading whitespace before parameter name
		for(;( (*pNext == ' ') || (*pNext == '\t') || (*pNext == '\n') || (*pNext == '\r') || (*pNext == 0) ); ++pNext);

		// Start at the beginning
		char* pOpen  = pNext;	// Points to the start of the inner value in the parens
		char* pClose = pNext;	// Points to the end of the inner value in the parens
		char* pParam = pNext;	// Points to the param text

		// Scan to the end of the parameter name and terminate it there
		for(; ((*pOpen != '(') && (*pOpen != ' ') && (*pOpen != '\t') && (*pOpen != '\r') && (*pOpen != '\n') && (*pOpen != 0) ); ++pOpen );

		if( *pOpen != '(' )
		{
			// Null terminate the parameter name
			*pOpen = 0; ++pOpen;

			// skip any intermediate whitespace between param and open paren
			for(;( (*pOpen == ' ') || (*pOpen == '\t') || (*pOpen == '\n') || (*pOpen == '\r') ); ++pOpen);
			// Scan for beginning of paren param starting with '('
			for(; ((*pOpen != '(') && (*pOpen != ')') && (*pOpen != ',') && (*pOpen != 0) ); ++pOpen );

			// Make sure this is the open paren
			if( *pOpen != '(' )
			{
				gperrorf(( "Effect %s is missing opening parenthesis. Text: '%s'\n", GetEffectName(), pParam ));
				return false;
			}
		}

		// Terminate parameter name and advance
		*pOpen = 0;
		++pOpen;

		// scan for ')'
		for(pClose = pOpen; ((*pClose != ')') && (*pClose != 0)); ++pClose );

		// check for errors
		if( *pClose == 0 )
		{
			gperrorf(( "Effect %s is missing closing parenthesis. Text: '%s'\n", GetEffectName(), pParam ));
			return false;
		}

		// Find the default param
		UINT32 param_index = GetIndex( pParam );

		// Error out if param is unrecognized
		if( param_index == INVALID_PARAM )
		{
			gperrorf(( "Effect %s does not use parameter '%s'\n", GetEffectName(), pParam ));
			return false;
		}

		// Get the actual parameter
		effect_param *pEffectParam= NULL;
		GetParam( param_index, &pEffectParam );

		// Save where to start reading the next parameter from
		pNext = pClose+1;

		// Skip any leading whitespace
		for(;( (*pOpen == '\n') || (*pOpen == '\t') || (*pOpen == '\r') || (*pOpen == ' ') ); ++pOpen);
		// Skip any trailing whitespace
		for(;( (*pClose == '\n') || (*pClose == '\t') || (*pClose == '\r') || (*pClose == ' ') ); --pClose);

		*pClose = 0;

		// Parse what is between the parenthesis
		switch( pEffectParam->m_Type )
		{
			case PT_BOOL:
			{
				pEffectParam->m_bool = true;
			} break;

			case PT_UINT32:
			{
				pEffectParam->m_UINT32 = UINT32(::atoi( pOpen ));
			} break;

			case PT_FLOAT:
			{
				pEffectParam->m_float = float(::atof( pOpen ));
			} break;

			case PT_VECTOR_3:
			{
				char* pStart = pOpen;
				char* pEnd	 = pOpen;

				// Get x component
				for(;( (*pStart == '\n') || (*pStart == '\t') || (*pStart == '\r') || (*pStart == ' ') ); ++pStart);
				// Scan ahead to first ',' or ' ' delimiter
				for(pEnd = pStart; ((*pEnd != ',') && (*pEnd != ' ') && (*pEnd != 0)); ++pEnd );

				// Check to see if the end of the inner param was reached and error out if true
				if( *pEnd == 0 )
				{
					gperrorf(( "Effect %s has a badly formatted parameter %s - Missing comma delimiters.\n", GetEffectName(), pParam ));
					return false;
				}
				*pEnd = 0; ++pEnd;

				pEffectParam->m_vector_3.x = float(::atof( pStart ));


				// Get y component
				for(pStart = pEnd;( (*pStart == '\n') || (*pStart == '\t') || (*pStart == '\r') || (*pStart == ' ') ); ++pStart);
				// Scan ahead to first ',' or ' ' delimiter
				for(pEnd = pStart; ((*pEnd != ',') && (*pEnd != ' ') && (*pEnd != 0)); ++pEnd );

				// Check to see if the end of the inner param was reached and error out if true
				if( *pEnd == 0 )
				{
					gperrorf(( "Effect %s has a badly formatted parameter %s - Missing comma delimiters.\n", GetEffectName(), pParam ));
					return false;
				}
				*pEnd = 0; ++pEnd;

				pEffectParam->m_vector_3.y = float(::atof( pStart ));


				// Get z component
				for(pStart = pEnd;( (*pStart == '\n') || (*pStart == '\t') || (*pStart == '\r') || (*pStart == ' ') ); ++pStart);
				// Scan ahead to first ',' or ' ' delimiter
				for(pEnd = pStart; ((*pEnd != ' ') && (*pEnd != 0)); ++pEnd );
				*pEnd = 0;

				pEffectParam->m_vector_3.z = float(::atof( pStart ));
			} break;

			case PT_GPSTRING:
			{
				pEffectParam->m_gpstring = pOpen;
			} break;

			default:
			{
				gperrorf(("Unrecognized parameter type was somehow specified for effect %s, param %s\n", GetEffectName(), pParam ));
				return false;
			} break;
		}

		// Note that this was an override
		pEffectParam->m_bOverride = true;
	}
	return true;
}


UINT32	EffectParams ::	GetIndex ( const char* sParamName )
{
	paramColl::iterator it = m_Params.begin(), it_end = m_Params.end();
	UINT32 index = 0;
	for(; it != it_end; ++it, ++index )
	{
		if( it->m_sParamName.same_no_case( sParamName ) )
		{
			return index;
		}
	}
	return (UINT32)INVALID_PARAM;
}


bool
EffectParams::GetParam( UINT32 index, effect_param **ppParam )
{
	if( index < m_Params.size() )
	{
		*ppParam = &m_Params[ index ];
		return true;
	}
	return false;
}

bool EffectParams :: GetBool ( UINT32 index, bool &value )
{
	if( index < m_Params.size() )
	{
		gpassert( m_Params[ index ].m_Type == PT_BOOL );

		value = m_Params[ index ].m_bool;
		return	m_Params[ index ].m_bOverride;
	}
	else
	{
		gperrorf(("Specified invalid EffectParams bool index of %d\n", index));
	}
	return false;
}

bool EffectParams :: GetUINT32 ( UINT32 index, UINT32 &value )
{
	if( index < m_Params.size() )
	{
		gpassert( m_Params[ index ].m_Type == PT_UINT32 );

		value = m_Params[ index ].m_UINT32;
		return	m_Params[ index ].m_bOverride;
	}
	else
	{
		gperrorf(("Specified invalid EffectParams UINT32 index of %d\n", index));
	}
	return false;
}

bool EffectParams :: GetFloat ( UINT32 index, float &value )
{
	if( index < m_Params.size() )
	{
		gpassert( m_Params[ index ].m_Type == PT_FLOAT );

		value = m_Params[ index ].m_float;
		return	m_Params[ index ].m_bOverride;
	}
	else
	{
		gperrorf(("Specified invalid EffectParams Float index of %d\n", index));
	}
	return false;
}

bool EffectParams :: GetVector3 ( UINT32 index, vector_3 &value )
{
	if( index < m_Params.size() )
	{
		gpassert( m_Params[ index ].m_Type == PT_VECTOR_3 );

		value = m_Params[ index ].m_vector_3;
		return	m_Params[ index ].m_bOverride;
	}
	else
	{
		gperrorf(("Specified invalid EffectParams Vector3 index of %d\n", index));
	}
	return false;
}

bool EffectParams :: GetGPString ( UINT32 index, gpstring &value )
{
	if( index < m_Params.size() )
	{
		gpassert( m_Params[ index ].m_Type == PT_GPSTRING );

		value = m_Params[ index ].m_gpstring;
		return	m_Params[ index ].m_bOverride;
	}
	else
	{
		gperrorf(("Specified invalid EffectParams String index of %d\n", index));
	}
	return false;
}


void EffectParams :: Clear ( void )
{
	m_Params.clear();
}






//
// --- CollisionInfo stuff
//

CollisionInfo::CollisionInfo( void )
{
	m_Elapsed			= 0;
	m_Effect_id			= 0;
	m_Target_id			= 0;
	m_Owner_id			= 0;
	m_bTerrainCollision	= false;

	GPDEBUG_ONLY( CheckValid( false ) );
}


CollisionInfo::CollisionInfo( SFxEID effect_id, Goid target_id, Goid owner_id,	const SiegePos &pos, const SiegePos &norm, const vector_3 &dir )
 : m_Effect_id( effect_id )
 , m_Target_id( target_id )
 , m_Owner_id( owner_id )
 , m_Collision_point( pos )
 , m_Collision_normal( norm )
 , m_Collision_direction( dir )
{
	m_Elapsed			= 5.0f;
	m_bTerrainCollision	= ( m_Target_id == GOID_INVALID );

	GPDEBUG_ONLY( CheckValid() );
}


bool CollisionInfo::Xfer( FuBi::PersistContext& persist )
{
#	if !GP_RETAIL
	if ( persist.IsSaving() )
	{
		CheckValid();
	}
#	endif // !GP_RETAIL

	persist.Xfer( "m_Elapsed",             m_Elapsed             );
	persist.Xfer( "m_Effect_id",           m_Effect_id           );
	persist.Xfer( "m_Target_id",           m_Target_id           );
	persist.Xfer( "m_Owner_id",            m_Owner_id            );
	persist.Xfer( "m_bTerrainCollision",   m_bTerrainCollision   );
	persist.Xfer( "m_Collision_point",     m_Collision_point     );
	persist.Xfer( "m_Collision_normal",    m_Collision_normal    );
	persist.Xfer( "m_Collision_direction", m_Collision_direction );

	return ( true );
}


#if !GP_RETAIL

void CollisionInfo::CheckValid( bool fullCheck ) const
{
	GoHandle target( m_Target_id );
	if ( target && target->IsLodfi() )
	{
		gperrorf(( "CollisionInfo: my target (%d, %s) is lodfi! Very bad!\n",
				   m_Target_id, target->GetTemplateName() ));
	}

	if ( fullCheck )
	{
		GoHandle owner( m_Owner_id );
		if ( owner->IsLodfi() )
		{
			gperrorf(( "CollisionInfo: my owner (0x%08X, %s) is lodfi! Very bad!\n", m_Owner_id, owner->GetTemplateName() ));
		}
	}
}

#endif // !GP_RETAIL
