//
//
//	Flamethrower_effect_factory.cpp
//	Copyright 1999, Gas Powered Games
//
//
#include "precomp_flamethrower.h"
#include "gpcore.h"

#include <list>
#include <map>

#include "Flamethrower.h"
#include "Flamethrower_effect_factory.h"
#include "WorldFx.h"
#include "Flamethrower_tracker.h"
#include "Flamethrower_effect_base.h"

#include "Flamethrower_effects.h"
#include "Flamethrower_effects2.h"
#include "Flamethrower_effects3.h"




Flamethrower_effect_factory::def_map	*pMap;
Flamethrower_effect_factory				*pThat;

template <class type> void
RegisterEffect( const gpstring &sName, type T ) 
{
	Flamethrower_effect_factory::def_functor new_funct = makeFunctor( *pThat, T );

	pMap->insert( std::make_pair( sName, new_funct ));

	SpecialEffect *new_effect = new_funct();

	EffectParams &effect_params = gWorldFx.GetDefaultEffectParams( sName );
	effect_params.SetEffectName( sName );

	new_effect->RegisterDefaultParams( effect_params );

	delete new_effect;
}


#define MAKE_EFFECT_FUNCTION( EFFECT_TEXT, EFFECT_CLASS, MAKE_FUNCTION ) \
SpecialEffect* \
Flamethrower_effect_factory::MAKE_FUNCTION( void ) \
{ \
	SpecialEffect* pEffect = new EFFECT_CLASS; \
	pEffect->SetName( EFFECT_TEXT ); \
	return pEffect; \
}




Flamethrower_effect_factory::
Flamethrower_effect_factory( SEMap &effectList )
	: m_List( effectList ) 
{
	pMap = &m_ParentEffects;
	pThat = this;

	// Register available effects
	RegisterEffect ( "fire",				&Flamethrower_effect_factory::Make_Fire );
	RegisterEffect ( "trackball",			&Flamethrower_effect_factory::Make_Trackball );
	RegisterEffect ( "lightning",			&Flamethrower_effect_factory::Make_Lightning );
	RegisterEffect ( "steam",				&Flamethrower_effect_factory::Make_Steam );
	RegisterEffect ( "explosion",			&Flamethrower_effect_factory::Make_Explosion );
	RegisterEffect ( "spe",					&Flamethrower_effect_factory::Make_SPE );
	RegisterEffect ( "orbiter",				&Flamethrower_effect_factory::Make_Orbiter );
	RegisterEffect ( "polygonalexplosion",	&Flamethrower_effect_factory::Make_PolygonalExplosion );
	RegisterEffect ( "lightsource",			&Flamethrower_effect_factory::Make_Lightsource );
	RegisterEffect ( "sphere",				&Flamethrower_effect_factory::Make_Sphere );
	RegisterEffect ( "flurry",				&Flamethrower_effect_factory::Make_Flurry );
	RegisterEffect ( "charge",				&Flamethrower_effect_factory::Make_Charge );
	RegisterEffect ( "sparkles",			&Flamethrower_effect_factory::Make_Sparkles );
	RegisterEffect ( "discpile",			&Flamethrower_effect_factory::Make_DiscPile );
	RegisterEffect ( "cylinder",			&Flamethrower_effect_factory::Make_Cylinder );
	RegisterEffect ( "pointtracer",			&Flamethrower_effect_factory::Make_PointTracer );
	RegisterEffect ( "linetracer",			&Flamethrower_effect_factory::Make_LineTracer );
	RegisterEffect ( "fireb",				&Flamethrower_effect_factory::Make_FireB );
	RegisterEffect ( "curve",				&Flamethrower_effect_factory::Make_Curve );
	RegisterEffect ( "spawn",				&Flamethrower_effect_factory::Make_Spawn );
	RegisterEffect ( "sray",				&Flamethrower_effect_factory::Make_SRay );
	RegisterEffect ( "decal",				&Flamethrower_effect_factory::Make_Decal );
}

	MAKE_EFFECT_FUNCTION ( "fire",					Fire_Effect,				Make_Fire )
	MAKE_EFFECT_FUNCTION ( "trackball",				Trackball_Effect,			Make_Trackball )
	MAKE_EFFECT_FUNCTION ( "lightning",				Lightning_Effect,			Make_Lightning )
	MAKE_EFFECT_FUNCTION ( "steam",					Steam_Effect,				Make_Steam )
	MAKE_EFFECT_FUNCTION ( "explosion",				Explosion_Effect,			Make_Explosion )
	MAKE_EFFECT_FUNCTION ( "spe",					SPE_Effect,					Make_SPE )
	MAKE_EFFECT_FUNCTION ( "orbiter",				Orbiter_Effect,				Make_Orbiter )
	MAKE_EFFECT_FUNCTION ( "polygonalexplosion",	PolygonalExplosion_Effect,	Make_PolygonalExplosion )
	MAKE_EFFECT_FUNCTION ( "lightsource",			Lightsource_Effect,			Make_Lightsource )
	MAKE_EFFECT_FUNCTION ( "sphere",				Sphere_Effect,				Make_Sphere )
	MAKE_EFFECT_FUNCTION ( "flurry",				Flurry_Effect,				Make_Flurry )
	MAKE_EFFECT_FUNCTION ( "charge",				Charge_Effect,				Make_Charge )
	MAKE_EFFECT_FUNCTION ( "sparkles",				Sparkles_Effect,			Make_Sparkles )
	MAKE_EFFECT_FUNCTION ( "discpile",				DiscPile_Effect,			Make_DiscPile )
	MAKE_EFFECT_FUNCTION ( "cylinder",				Cylinder_Effect,			Make_Cylinder )
	MAKE_EFFECT_FUNCTION ( "pointtracer",			PointTracer_Effect,			Make_PointTracer )
	MAKE_EFFECT_FUNCTION ( "linetracer",			LineTracer_Effect,			Make_LineTracer )
	MAKE_EFFECT_FUNCTION ( "fireb",					FireB_Effect,				Make_FireB )
	MAKE_EFFECT_FUNCTION ( "curve",					Curve_Effect,				Make_Curve )
	MAKE_EFFECT_FUNCTION ( "spawn",					Spawn_Effect,				Make_Spawn )
	MAKE_EFFECT_FUNCTION ( "sray",					SRay_Effect,				Make_SRay )
	MAKE_EFFECT_FUNCTION ( "decal",					Decal_Effect,				Make_Decal )



bool
Flamethrower_effect_factory::MakeEffect( const gpstring &sName, SpecialEffect **pEffect )
{
	def_map::iterator itEffect;

	itEffect = m_ParentEffects.find( sName );

	if( itEffect == m_ParentEffects.end() )
	{
		return false;
	}

	def_pair effectData = *itEffect;
	*pEffect = effectData.second();

	return true;
}


#if !GP_RETAIL


gpstring
Flamethrower_effect_factory::GetAvailableEffects()
{
	gpstring sOutput;
	gpstring sHeader( "= Available Effects =" );

	sOutput.append( 91, '-' ); sOutput.append( "\n" );
	sOutput.append( 45 - sHeader.size()/2, ' ' );
	sOutput.append( sHeader ); sOutput.append( "\n" );
	sOutput.append( 91, '-' ); sOutput.append( "\n" );

	// Determine max field length
	def_map::const_iterator it = m_ParentEffects.begin(), iend = m_ParentEffects.end();

	UINT32 field_size = 0;
	for(; it != iend; ++it )
	{
		field_size = max_t( (*it).first.size(), field_size );
	}

	const UINT32 names_per_line = (UINT32)floor(91/field_size);

	UINT32 name_count = 0;
	for( it = m_ParentEffects.begin(); it != iend; ++it )
	{
		sOutput.append( (*it).first );
		sOutput.append( 91/names_per_line - (*it).first.size()+1, ' ' );

		if( ++name_count == names_per_line )
		{
			name_count = 0;
			sOutput.append( "\n" );
		}
	}

	if( name_count != 0 )
	{
		sOutput.append( "\n" );
	}
	sOutput.appendf( "Total effects = %d\n", m_ParentEffects.size() );

	return sOutput;
}


#endif // !GP_RETAIL