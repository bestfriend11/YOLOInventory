#pragma once
//
//	Flamethrower_interpreter_commands
//
//	Herein contains recognized interpreter commands
//

#ifndef _FLAMETHROWER_INTERPRETER_COMMANDS_H_
#define _FLAMETHROWER_INTERPRETER_COMMANDS_H_


#define DECLARE_FLAMETHROWER_COMMAND_BEGIN( CMD )					\
	class FC_##CMD : public Flamethrower_command					\
	{	public:																				\

#define DECLARE_FLAMETHROWER_COMMAND_END( CMD )											\
		FC_##CMD( const gpstring &Name )												\
				: Flamethrower_command( Name ){}										\
		bool	Execute( Flamethrower_script *pScript, Flamethrower_params & params	);	\
	};



//
// call < script name > [ parameter list ]
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Call );
DECLARE_FLAMETHROWER_COMMAND_END( Call );


//
// camerashake < effect name > [ parameter list ]
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Camerashake );
DECLARE_FLAMETHROWER_COMMAND_END( Camerashake );


//
// exit immediate
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Exit );
DECLARE_FLAMETHROWER_COMMAND_END( Exit );


//
// frandrange < min > < max >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Frandrange );
DECLARE_FLAMETHROWER_COMMAND_END( Frandrange );


//
// get < collision > < point|direction >, < effect_id >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Get );

	enum eGetParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		GP_COLLISION,
		GP_TARGET_POSITION,
		GP_ID,
		GP_TARGET,
		GP_SOURCE,
		GP_POINT,
		GP_DIRECTION,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Get );


//
// mode < mode name >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Mode );

	enum eModeParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		MP_PAUSE_IMMUNE,
		MP_PAUSE_AFFECTED,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Mode );


//
// pause < seconds >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Pause );
DECLARE_FLAMETHROWER_COMMAND_END( Pause );


//
// randrange < min > < max >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Randrange );
DECLARE_FLAMETHROWER_COMMAND_END( Randrange );


//
// set < $variable name >, < value|#macro >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Set );
DECLARE_FLAMETHROWER_COMMAND_END( Set );


//
// sfx < command > < parameter1, parameter2... >, < target >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Sfx );

	enum eSfxParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		SP_ALL,
		SP_TARGET,
		SP_SOURCE,

		SP_CREATE,
		SP_DESTROY,

		SP_START,
		SP_FINISH,
		SP_STOP,

		SP_FREEZE_TARGETS,
		SP_SNAP_TO_GROUND,
		SP_RAT,
		SP_ATTACH,
		SP_ATTACH_POINT,
		SP_POSITION,
		SP_POSITION_AT,
		SP_OFFSET,
		SP_OFFSET_BONE,
		SP_DIRECTION,

		SP_FRIENDLY,
		SP_PARTY,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Sfx );


//
// signal < message text >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Signal );

	enum eSignalParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		SP_ALL_SCRIPTS,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Signal );


//
// sound < command > < parameters... >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Sound );

	enum eSoundParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		SP_PLAY,
		SP_LOOP,
		SP_AT,
		SP_DIST,
		SP_STOP,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Sound );


//
// waitfor < script|signal|collision > < id > < timeout >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Waitfor );

	enum eWaitforParam
	{
		SET_BEGIN_ENUM( PARAM_, 0 ),

		WP_TERMINATION,
		WP_SCRIPT,
		WP_SIG,
		WP_COLLISION,

		SET_END_ENUM( PARAM_ ),
	};

	bool	GetParamToken( const char *sParam, UINT32 & token );

DECLARE_FLAMETHROWER_COMMAND_END( Waitfor );


//
// worldmsg < spell > < success|failure >
//
DECLARE_FLAMETHROWER_COMMAND_BEGIN( Worldmsg );
DECLARE_FLAMETHROWER_COMMAND_END( Worldmsg );



#undef DECLARE_FLAMETHROWER_COMMAND_BEGIN
#undef DECLARE_FLAMETHROWER_COMMAND_END


#endif // _FLAMETRHOWER_INTERPRETER_COMMANDS_H_
