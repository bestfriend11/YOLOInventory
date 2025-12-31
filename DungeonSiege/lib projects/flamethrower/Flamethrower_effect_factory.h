#pragma once
//
//
//	Flamethrower_effect_factory.h
//	Copyright 1999, Gas Powered Games
//
//
#ifndef _FLAMETHROWER_EFFECT_FACTORY_H_
#define _FLAMETHROWER_EFFECT_FACTORY_H_

class Flamethrower;
class SpecialEffect;

#include <string>
#include <vector>
#include <map>
#include "BlindCallback.h"
#include "Flamethrower_types.h"


class Flamethrower_effect_factory
{
public:
	typedef CBFunctor0wRet< SpecialEffect* >				def_functor;
	typedef std::map< gpstring, def_functor, istring_less >	def_map;
	typedef std::pair< gpstring, def_functor >				def_pair;

private:
	SEMap &			m_List;
	def_map			m_ParentEffects;


public:

	// Existence
	Flamethrower_effect_factory( SEMap & effectList );


	// Creates a new effect
	bool			MakeEffect( const gpstring &sName, SpecialEffect **pEffect );

	// Returns a list of all the available effects
#	if !GP_RETAIL
	gpstring		GetAvailableEffects();
#	endif // !GP_RETAIL

	// Products
	SpecialEffect*	Make_Fire				( void );
	SpecialEffect*	Make_Trackball			( void );
	SpecialEffect*	Make_Lightning			( void );
	SpecialEffect*	Make_Steam				( void );
	SpecialEffect*	Make_Explosion			( void );
	SpecialEffect*	Make_SPE				( void );
	SpecialEffect*	Make_Orbiter			( void );
	SpecialEffect*	Make_PolygonalExplosion	( void );
	SpecialEffect*	Make_Lightsource		( void );
	SpecialEffect*	Make_Sphere				( void );
	SpecialEffect*	Make_Flurry				( void );
	SpecialEffect*	Make_Charge				( void );
	SpecialEffect*	Make_Sparkles			( void );
	SpecialEffect*	Make_DiscPile			( void );
	SpecialEffect*	Make_Cylinder			( void );
	SpecialEffect*	Make_PointTracer		( void );
	SpecialEffect*	Make_LineTracer			( void );
	SpecialEffect*	Make_FireB				( void );
	SpecialEffect*	Make_Curve				( void );
	SpecialEffect*	Make_Spawn				( void );
	SpecialEffect*	Make_SRay				( void );
	SpecialEffect*	Make_Decal				( void );
};


#endif // _FLAMETHROWER_EFFECT_FACTORY_H_
