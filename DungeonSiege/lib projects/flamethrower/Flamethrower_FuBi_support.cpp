//
//
//	Flamethrower_FuBi_support.cpp
//	Copyright 1999, Gas Powered Games
//
//
#include "precomp_flamethrower.h"
#include "flamethrower_fubi_support.h"
#include "flamethrower_effect_base.h"

FUBI_REPLACE_NAME( "SFxSID_", SFxSID );
FUBI_EXPORT_POINTER_CLASS( SFxSID_ );

FUBI_REPLACE_NAME( "SFxEID_", SFxEID );
FUBI_EXPORT_POINTER_CLASS( SFxEID_ );

bool FuBi::Traits <def_tracker> :: XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
{
	persist.EnterBlock( key );

	bool created = obj.get() != NULL;
	persist.Xfer( "_created", created );

	if ( persist.IsRestoring() && created )
	{
		obj = WorldFx::MakeTracker();
	}
	if ( created )
	{
		persist.Xfer( FuBi::PersistContext::VALUE_STR, *obj );
	}

	persist.LeaveBlock();
	return ( true );
}


bool FuBi::Traits <sVertex> :: XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
{
	persist.Xfer   ( "x",     obj.x     );
	persist.Xfer   ( "y",     obj.y     );
	persist.Xfer   ( "z",     obj.z     );
	persist.XferHex( "color", obj.color );
	persist.Xfer   ( "u",     obj.uv.u  );
	persist.Xfer   ( "v",     obj.uv.v  );

	return ( true );
}


bool FxShouldPersistHelper :: operator () ( const SpecialEffect* effect ) const
{
	gpassert( effect != NULL );
	return ( effect->ShouldPersist() );
}


bool FxShouldPersistHelper :: operator () ( const Flamethrower_script* script ) const
{
	gpassert( script != NULL );
	return ( script->ShouldPersist() );
}


bool FxShouldPersistHelper :: operator () ( SFxSID id ) const
{
	Flamethrower_script* script = NULL;
	gpverify( gFlamethrower.FindScript( id, &script ) );
	return ( script && script->ShouldPersist() );
}


bool FxShouldPersistHelper :: operator () ( SFxEID id ) const
{
	SpecialEffect* effect = gFlamethrower.FindEffect( id );
	gpassert( effect != NULL );
	return ( effect && effect->ShouldPersist() );
}


bool FxShouldPersistHelper :: operator () ( const Flamethrower_interpreter::Signal& signal ) const
{
	return ( (*this)( signal.m_id ) );
}
