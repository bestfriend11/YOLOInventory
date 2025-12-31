//////////////////////////////////////////////////////////////////////////////
//
// File     :  NeMa_Types.h
// Author(s):  Mike Biddlecombe
//
// Summary  :  Contains basic types and defs for NeMa. Usually can just
//             #include this file and nothing else from NeMa if not calling
//             any functions.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __NEMA_TYPES_H
#define __NEMA_TYPES_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "Quat.h"
#include "ResHandle.h"
#include "Vector_3.h"
#include "Point2.h"

#include <set>

//////////////////////////////////////////////////////////////////////////////

class Rapi;
class Chore;
enum eAnimChore;
enum eAnimStance;
enum eAnimEvent;

namespace nema {

	class ActiveChore;
	class Blender;

	typedef CBFunctor1<unsigned int> EventCallback;

	const float TEST_UPDATE_INTERVAL = (1/60.0f);
	const float INFINITE_TIME = FLOAT_INFINITE;

	const DWORD MAXIMUM_STANCES = 9;

	typedef unsigned short	WORD;
	typedef unsigned short	USHORT;
	typedef unsigned char	UBYTE;

	// TODO convert everything in NEMA to SIEGE orientation and do away with this constant
	//const Quat PreRotateNEMAtoSEIGE(0.0f,SQRT_ONEHALF,SQRT_ONEHALF,0.0f);
	const Quat PreRotateNEMAtoSEIGE(SQRT_ONEHALF,0.0f,0.0f,-SQRT_ONEHALF);
	// FWD decl the important classes and typedefs 

	class Aspect;
	typedef ResHandle<Aspect>		HAspect;
	typedef ResHandleMgr<HAspect>	HAspectMgr;

	typedef std::set<HAspect>			AspectSet;
	typedef AspectSet::iterator			AspectSetIter;
	typedef AspectSet::const_iterator	AspectSetConstIter;

	class PRSKeys;
//	typedef ResHandle<PRSKeys>		HPRSKeys;

	typedef stdx::fast_vector<PRSKeys*>	PRSKeyArray;
	typedef PRSKeyArray::iterator		PRSKeyArrayIter;

	// The NeMa lighting model is a slimmed down subset of data that is
	// available/req'd in the SiegeEngine lighting model

	enum LightSourceType {
		NLS_INFINITE_LIGHT,
		NLS_POINT_LIGHT, 
		NLS_SPOT_LIGHT,
	};

	struct LightSource {

		LightSourceType m_Type;

		DWORD m_Color;
		float m_Intensity;

		// spot & infinite lights
		vector_3 m_Direction;

		// spot & point lights
		vector_3 m_Position;
		
		// spot light only
		float m_InnerCone;
		float m_OuterCone;		

		// point light only
		float m_InnerRadius;
		float m_OuterRadius;

	};

} // End of nema namespace

//////////////////////////////////////////////////////////////////////////////

#endif  // __NEMA_TYPES_H

//////////////////////////////////////////////////////////////////////////////
