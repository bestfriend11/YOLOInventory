#pragma once
//
// Flamethrower_types.h
//
//

#ifndef	_FLAMETHROWER_TYPES_H_
#define	_FLAMETHROWER_TYPES_H_

class CollisionInfo;
class SpecialEffect;
class TattooTracker;
class EffectParams;

#include "siege_database_guid.h"

struct SFxEID_;
typedef const SFxEID_* SFxEID;		// SiegeFx Effect ID
const SFxEID SFxEID_INVALID = 0;

struct SFxSID_;
typedef const SFxSID_* SFxSID;		// SiegeFx Script ID
const SFxSID SFxSID_INVALID = 0;

inline bool IsGlobalSFxEID( SFxEID id )  { return ( ((DWORD)id & 0x80000000) != 0 ); }
inline bool IsGlobalSFxSID( SFxSID id )  { return ( ((DWORD)id & 0x80000000) != 0 ); }

typedef stdx::fast_vector< SFxEID > SFxEIDColl;
typedef stdx::fast_vector< SFxSID > SFxSIDColl;

struct EffectDrawEntry
{
	SpecialEffect* m_Effect;
	float          m_Y;

	EffectDrawEntry( SpecialEffect* effect = NULL, float y = 0 )
	{
		m_Effect = effect;
		m_Y = y;
	}

	inline bool operator < ( const EffectDrawEntry& r ) const
	{
		return ( m_Y < r.m_Y );
	}
};

typedef stdx::fast_vector	< CollisionInfo* >	CollColl;
typedef std::map	< SFxEID, SpecialEffect* >	SEMap;
typedef stdx::fast_vector	< SpecialEffect* >	SEVector;
typedef std::multiset		< EffectDrawEntry >	SEDrawColl;
typedef std::auto_ptr		< TattooTracker  >	def_tracker;

typedef std::map< gpstring, EffectParams, istring_less >	def_EParamMap;
typedef std::map< siege::database_guid, SEDrawColl >		def_EffectMap;


#endif//_FLAMETHROWER_TYPES_H_
