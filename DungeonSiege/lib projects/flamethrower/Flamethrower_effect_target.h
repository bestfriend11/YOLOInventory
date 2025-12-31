#pragma once
#ifndef _FLAMETHROWER_EFFECT_TARGET_H_
#define _FLAMETHROWER_EFFECT_TARGET_H_

class	Flamethrower_target;
class	Flamethrower;

#include "vector_3.h"

#include "siege_pos.h"


class Flamethrower_effect_target : public Flamethrower_target
{
public:
	Flamethrower_effect_target	( void ) {}
	Flamethrower_effect_target	( SFxEID id );

	bool	OnDuplicate			( Flamethrower_target **pTarget );

	bool	GetPlacement		( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = Flamethrower_target::ORIENT_GO, 
								const gpstring &sBoneName = gpstring::EMPTY );
	
	bool	GetDirection		( vector_3 &direction );
	bool	GetBoundingRadius	( float &radius );

	bool	GetIsInsideInventory( void );

};


#endif // _FLAMETHROWER_EFFECT_TARGET_H_
