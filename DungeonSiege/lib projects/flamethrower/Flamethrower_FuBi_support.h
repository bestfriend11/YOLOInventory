#pragma once
//
// Flamethrower_fubi_support.cpp
//
#ifndef	_FLAMETHROWER_FUBI_SUPPORT_H_
#define	_FLAMETHROWER_FUBI_SUPPORT_H_


#include "FuBiTraits.h"
#include "Flamethrower_Interpreter.h"
#include "Flamethrower_Tracker.h"


FUBI_DECLARE_SELF_TRAITS( TattooTracker );

FUBI_DECLARE_POINTERCLASS_TRAITS( SFxSID, SFxSID_INVALID );
FUBI_DECLARE_POINTERCLASS_TRAITS( SFxEID, SFxEID_INVALID );

FUBI_DECLARE_XFER_TRAITS( def_tracker )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
};


FUBI_DECLARE_XFER_TRAITS( sVertex )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
};


struct FxShouldPersistHelper
{
	bool operator () ( const SpecialEffect* effect ) const;
	bool operator () ( const Flamethrower_script* script ) const;
	bool operator () ( SFxSID id ) const;
	bool operator () ( SFxEID id ) const;
	bool operator () ( const Flamethrower_interpreter::Signal& signal ) const;
};

struct FxShouldPersistFirst
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( FxShouldPersistHelper()( obj.first ) );
	}
};

struct FxShouldPersistSecond
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( FxShouldPersistHelper()( obj.second ) );
	}
};



#endif	//_FLAMETHROWER_FUBI_SUPPORT_H_
