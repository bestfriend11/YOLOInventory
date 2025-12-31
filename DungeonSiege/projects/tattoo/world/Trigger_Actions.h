#pragma once
#ifndef _TRIGGER_ACTION_H_
#define _TRIGGER_ACTION_H_

#include "trigger_sys.h"

#include "mood.h"
#include "victory.h"
#include "worldfx.h"
#include "worldmap.h"
#include "worldterrain.h"

////////////////////////////////////////////////////////////////////////////////////
//
// Actions
//
////////////////////////////////////////////////////////////////////////////////////

namespace trigger
{

// Base Class

class Action
{
	friend class TriggerSys;

protected:

	Parameter		m_Parameters;

	bool			m_bAllowClientExecution;

public:

	Action()			{}
	virtual ~Action()	{}

	virtual const char* GetName() = 0;

	virtual	float		EffectiveDelay(const Params& parms, float init_delay ) { return parms.Delay+init_delay; }

	virtual void		Execute( Trigger &trigger, const Params &params ) = 0;

	virtual void		Label(Trigger& /*trigger*/, Params& /*params*/) {};

#if !GP_RETAIL
	virtual bool		Validate( const Trigger & /*trigger*/, Params& /*params*/, gpstring& /*out_errmsg*/ ) const { return true; }
	virtual void		Draw( Trigger & /*trigger*/, const Params & /*params*/ ) {}
#endif

	Parameter* GetFormat()					{ return &m_Parameters; }

	void SetAllowClientExecution( bool b )	{ m_bAllowClientExecution = b; }
	bool GetAllowClientExecution()			{ return m_bAllowClientExecution; }

	SET_NO_COPYING( Action );
};


//
// set_interest_radius( "Radius" )
//
class set_interest_radius : public Action
{
public:

	set_interest_radius();

	virtual const char* GetName() {return "set_interest_radius";}

	virtual void Execute( Trigger & trigger, const Params &params );

};


//
// fade_nodes( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
class fade_nodes : public Action
{
public:

	fade_nodes();

#if !GP_RETAIL
	virtual bool Validate( const Trigger & trigger, Params& params, gpstring& out_errmsg ) const;
#endif

	virtual const char* GetName() {return "fade_nodes";}

	virtual	float EffectiveDelay(const Params& /*parms*/, float /*init_delay*/)	{ return 0.0f; }

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// fade_nodes_outer_local_party( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
class fade_nodes_outer_local_party : public Action
{
public:

	fade_nodes_outer_local_party();

	virtual const char* GetName() {return "fade_nodes_outer_local_party";}

	virtual	float EffectiveDelay(const Params& /*parms*/, float /*init_delay*/)	{ return 0.0f; }

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// fade_nodes_global( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
class fade_nodes_global : public Action
{
public:

	fade_nodes_global();

	virtual const char* GetName() {return "fade_nodes_global";}

	virtual	float EffectiveDelay(const Params& /*parms*/, float /*init_delay*/)	{ return 0.0f; }

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// fade_node( "Node", "Fade Type" )
//
class fade_node : public Action
{
public:

	fade_node();

	virtual const char* GetName() {return "fade_node";}

	virtual	float EffectiveDelay(const Params& /*parms*/, float /*init_delay*/)	{ return 0.0f; }

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// send_world_message( "event type" )
//
class send_world_message : public Action
{

public:

	send_world_message();

	virtual const char* GetName() {return "send_world_message";}

	// The effective delay on a send message is 0, but the message may be sent with a delay
	virtual	float EffectiveDelay(const Params& /*parms*/, float /*init_delay*/)	{ return 0.0f; }

	virtual void Execute( Trigger & trigger, const Params &params );

	virtual void Label(const Trigger& trigger, Params& params ) const;	
	
#if !GP_RETAIL
	virtual bool Validate( const Trigger & trigger, Params& params, gpstring& out_errmsg ) const;
	virtual void Draw( Trigger & trigger, const Params &params );
#endif
};




//
// call_sfx_script("script name", "*emitter"|"bone name", "script parameters")
//
class call_sfx_script : public Action
{
public:

	call_sfx_script();

	virtual const char* GetName() {return "call_sfx_script";}

	virtual void Execute( Trigger & trigger, const Params &params );

	void AddTarget( def_tracker& tracker, Goid target, const Params &params );
	
};




//
// play_sound( "name" )
//
class play_sound : public Action
{

public:

	play_sound();

	virtual const char* GetName()			{ return "";}

	virtual void Execute( Trigger & trigger, const Params &params );

};



//
// change_actor_life( value, "condition_parameter" )
//
class change_actor_life : public Action
{

public:

	change_actor_life();

	virtual const char* GetName()			{ return "change_actor_life";}

	virtual void Execute( Trigger & trigger, const Params &params );

};


//
// change_actor_mana( value, "condition_parameter" )
//
class change_actor_mana : public Action
{

public:

	change_actor_mana();

	virtual const char* GetName()			{ return "change_actor_mana";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// start_camera_fx( "Name", "Params" )
//
class start_camera_fx : public Action
{
public:

	start_camera_fx();

	virtual const char* GetName()			{ return "start_camera_fx";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// change_quest_state( "name", "state" )
//
class change_quest_state : public Action
{
public:

	change_quest_state();

	virtual const char* GetName()			{ return "change_quest_state";}

	virtual void Execute( Trigger & trigger, const Params &params );
};

//
// mood_change( "moodname" )
//
class mood_change : public Action
{
public:

	mood_change();

	virtual const char* GetName()			{ return "mood_change";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


//
// victory_condition_met( "victorycondition" )
//
class victory_condition_met : public Action
{
public:

	victory_condition_met();

	virtual const char* GetName()			{ return "victory_condition_met";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


class set_camera_fade_node : public Action
{
public:

	set_camera_fade_node();

	virtual const char* GetName()			{ return "set_camera_fade_node";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


class set_occludes_camera_node : public Action
{
public:

	set_occludes_camera_node();

	virtual const char* GetName()			{ return "set_occludes_camera_node";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


class set_bounds_camera_node : public Action
{
public:

	set_bounds_camera_node();

	virtual const char* GetName()			{ return "set_bounds_camera_node";}

	virtual void Execute( Trigger & trigger, const Params &params );
};


class set_player_world_location : public Action
{
public:

	set_player_world_location();

	virtual const char* GetName()			{ return "set_player_world_location";}

	virtual void Execute( Trigger & trigger, const Params &params );

};

}; // namespace trigger

#endif // _TRIGGER_ACTION_H_
