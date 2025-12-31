#pragma once
//
//	Flamethrower_interpreter_macros.h
//
//	Herin contains all of the macros used in the Flamethrower
//	interpreter
//

#ifndef _FLAMETHROWER_INTERPRETER_MACROS_H_
#define	_FLAMETHROWER_INTERPRETER_MACROS_H_


#define DECLARE_FLAMETHROWER_MACRO( macro )				\
	class FM_##macro : public Flamethrower_macro		\
	{													\
	public:												\
		FM_##macro( const gpstring &sName )				\
			: Flamethrower_macro( sName ){}				\
														\
		bool	Execute( Flamethrower_script *pScript );\
	}

DECLARE_FLAMETHROWER_MACRO( nop );
DECLARE_FLAMETHROWER_MACRO( pop );
DECLARE_FLAMETHROWER_MACRO( peek );
DECLARE_FLAMETHROWER_MACRO( true );
DECLARE_FLAMETHROWER_MACRO( false );
DECLARE_FLAMETHROWER_MACRO( null_target );
DECLARE_FLAMETHROWER_MACRO( source );
DECLARE_FLAMETHROWER_MACRO( target );

DECLARE_FLAMETHROWER_MACRO( target_kb );
DECLARE_FLAMETHROWER_MACRO( source_kb );

DECLARE_FLAMETHROWER_MACRO( default_timeout );
DECLARE_FLAMETHROWER_MACRO( no_collision );
DECLARE_FLAMETHROWER_MACRO( object_collision );
DECLARE_FLAMETHROWER_MACRO( terrain_collision );
DECLARE_FLAMETHROWER_MACRO( source_position );
DECLARE_FLAMETHROWER_MACRO( target_position );
DECLARE_FLAMETHROWER_MACRO( camera_position );
DECLARE_FLAMETHROWER_MACRO( target_goid );
DECLARE_FLAMETHROWER_MACRO( source_goid );
DECLARE_FLAMETHROWER_MACRO( owner_goid );
DECLARE_FLAMETHROWER_MACRO( invalid_goid );
DECLARE_FLAMETHROWER_MACRO( weapon_hand_name );
DECLARE_FLAMETHROWER_MACRO( shield_hand_name );
DECLARE_FLAMETHROWER_MACRO( caster_name );
DECLARE_FLAMETHROWER_MACRO( attack_class );
DECLARE_FLAMETHROWER_MACRO( weapon_type_dagger );
DECLARE_FLAMETHROWER_MACRO( weapon_type_axe );
DECLARE_FLAMETHROWER_MACRO( weapon_type_blunt );
DECLARE_FLAMETHROWER_MACRO( weapon_type_staff );
DECLARE_FLAMETHROWER_MACRO( weapon_type_sword );
DECLARE_FLAMETHROWER_MACRO( weapon_type_bow );
DECLARE_FLAMETHROWER_MACRO( weapon_type_minigun );
DECLARE_FLAMETHROWER_MACRO( weapon_type_projectile );
DECLARE_FLAMETHROWER_MACRO( lodfi );

DECLARE_FLAMETHROWER_MACRO( red_blood );
DECLARE_FLAMETHROWER_MACRO( no_blood );


#undef DECLARE_FLAMETHROWER_MACRO


#endif //_FLAMETHROWER_INTERPRETER_MACROS_H_
