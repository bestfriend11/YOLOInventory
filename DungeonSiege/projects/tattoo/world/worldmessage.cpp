//////////////////////////////////////////////////////////////////////////////
//
// File     :  WorldMessage.cpp
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "WorldMessage.h"

#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "MessageDispatch.h"
#include "Go.h"
#include "GoSupport.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "WorldTime.h"
#include "WorldState.h"

FUBI_EXPORT_ENUM( eWorldEvent, WE_INVALID, WE_COUNT );

//////////////////////////////////////////////////////////////////////////////
// enum eWorldEvent implementation

// $ note: the event names must appear in the same order in the .h file
//         enum table as they appear here in the string table.
//
//         ALSO: KEEP THE TRAITS BITS UP TO DATE!!

struct WorldEventEntry
{
	const char* m_Name;
	bool        m_Traits[ 11 ];
};

static WorldEventEntry s_WorldEventTraits[] =
{
	{  "we_invalid"								,	{	0,	0,	0,	0, 	0,	0,	1,	0,	0,	0,	0	},	},
	{  "we_unknown"								,	{	0,	0,	0,	0, 	0,	0,	1,	0,	0,	0,	0	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// system

	{  "we_constructed"							,	{	1,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_destructed"							,	{	1,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_job_destructed"						,	{	0,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	1	},	},
	{  "we_entered_world"						,	{	1,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_left_world"							,	{	1,	0,	0,	1, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_frustum_membership_changed"			,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	1	},	},
	{  "we_frustum_active_state_changed"		,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	1	},	},
	{  "we_upgraded"							,	{	1,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_downgraded"							,	{	1,	0,	0,	1, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_expired_auto"						,	{	0,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_expired_forced"						,	{	0,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	1	},	},
	{  "we_pre_save_game"						,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	1	},	},
	{  "we_post_save_game"						,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	1	},	},
	{  "we_post_restore_game"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	1	},	},
	{  "we_terrain_transition_done"				,	{	0,	0,	0,	0, 	1,	0,	1,	1,	0,	0,	1	},	},
	{  "we_world_state_transition_done"			,	{	1,	0,	0,	0, 	0,	0,	1,	0,	1,	0,	0	},	},
	{  "we_camera_command_done"					,	{	0,	0,	0,	0, 	1,	0,	1,	1,	0,	0,	0	},	},
	{  "we_player_changed"						,	{	1,	0,	0,	0, 	0,	1,	1,	1,	0,	0,	0	},	},
	{  "we_player_data_changed"					,	{	1,	0,	0,	0, 	0,	0,	1,	0,	1,	0,	0	},	},
	{  "we_scidbits_changed"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_machine_connected"				,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_machine_disconnected"				,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_failed_connect"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_session_changed"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_session_terminated"				,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_session_added"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_session_connected"				,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_created"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_destroyed"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_ready"						,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_not_ready"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_world_state_changed"		,	{	1,	0,	0,	0, 	0,	0,	1,	0,	1,	0,	0	},	},
	{  "we_mp_player_set_character_level"		,	{	1,	0,	0,	0, 	0,	0,	1,	0,	1,	0,	0	},	},
	{  "we_mp_set_map"							,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_set_map_world"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_set_game_type"					,	{	1,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},
	{  "we_mp_terrain_fully_loaded"				,	{	0,	0,	0,	0, 	0,	0,	1,	0,	1,	0,	0	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// ui

	{  "we_selected"							,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_deselected"							,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_focused"								,	{	1,	0,	0,	1, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_unfocused"							,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_hotgroup"							,	{	1,	0,	0,	1, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_unhotgroup"							,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_mousehover"							,	{	1,	0,	0,	1, 	0,	1,	1,	0,	0,	0,	0	},	},
	{  "we_unmousehover"						,	{	1,	0,	0,	0, 	0,	1,	1,	0,	0,	0,	0	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// basic gameplay events

	{  "we_collided"							,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_glanced"								,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_damaged"								,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_dropped"								,	{	1,	0,	0,	0, 	0,	0,	1,	1,	0,	0,	1	},	},
	{  "we_equipped"							,	{	1,	0,	0,	0, 	0,	0,	1,	1,	0,	0,	1	},	},
	{  "we_exploded"							,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_go_timer_done"						,	{	0,	0,	0,	0, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_got"									,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_healed"								,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_killed"								,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_leveled_up"							,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_lost_consciousness"					,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_picked_up"							,	{	1,	0,	0,	0, 	0,	0,	1,	0,	0,	0,	1	},	},
	{  "we_req_activate"						,	{	1,	0,	0,	1, 	1,	0,	0,	0,	0,	1,	1	},	},
	{  "we_req_cast"							,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_req_cast_charge"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_req_deactivate"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	1,	1	},	},
	{  "we_req_delete"							,	{	0,	0,	0,	0, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_req_die"								,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_req_talk_begin"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_req_talk_end"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_req_use"								,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_resurrected"							,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_spell_collision"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_spell_expiration_timer_reset"		,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_spell_sync_begin"					,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_spell_sync_end"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_spell_sync_mid"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_start_burning"						,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_start_simulating"					,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_stop_burning"						,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_trigger_activate"					,	{	1,	0,	0,	1,	1,	0,	0,	0,	0,	1,	1	},	},
	{  "we_trigger_deactivate"					,	{	1,	0,	0,	1,	1,	0,	0,	0,	0,	1,	1	},	},
	{  "we_unequipped"							,	{	1,	0,	0,	0, 	0,	0,	1,	1,	0,	0,	1	},	},
	{  "we_weapon_launched"						,	{	1,	0,	0,	0, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_weapon_swung"						,	{	1,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// MCP and anim

	{  "we_mcp_all_sections_sent"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_chore_changing"					,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_facing_lockedon"					,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_facing_unlocked"					,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_goal_changed"					,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_goal_reached"					,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_invalidated"						,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_section_complete_warning"		,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_section_completed"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_node_blocked"					,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_dependancy_created"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_dependancy_removed"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_dependancy_broken"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mcp_mutual_dependancy"				,	{	0,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_looped"							,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_done"							,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_attach_ammo"					,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_weapon_fire"					,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_weapon_swung"					,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_sfx"							,	{	1,	1,	0,	1, 	0,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_die"							,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_anim_other"							,	{	1,	1,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// AI sensors

	{  "we_ai_activate"							,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_ai_deactivate"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_alert_enemy_spotted"					,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_alert_projectile_near_missed"		,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_attacked_melee"						,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_attacked_ranged"						,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_enemy_entered_inner_comfort_zone"	,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_enemy_entered_outer_comfort_zone"	,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_enemy_spotted"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_engaged_fled"						,	{	0,	0,	1,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_engaged_hit_killed"					,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_hit_lived"					,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_invalid"						,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_killed"						,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_lost"						,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_lost_consciousness"			,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_engaged_missed"						,	{	0,	0,	1,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_friend_entered_inner_comfort_zone"	,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_friend_entered_outer_comfort_zone"	,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_friend_spotted"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_gained_consciousness"				,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_job_current_action_base_jat_changed"	,	{	0,	0,	0,	1,	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_job_failed"							,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_job_finished"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_job_reached_travel_distance"			,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_job_timer_done"						,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_life_ratio_reached_high"				,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_life_ratio_reached_low"				,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_mana_ratio_reached_high"				,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_mana_ratio_reached_low"				,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_mind_action_q_emptied"				,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mind_action_q_idled"					,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_mind_processing_new_job"				,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},
	{  "we_req_job_end"							,	{	0,	0,	0,	1, 	1,	0,	0,	1,	0,	0,	1	},	},
	{  "we_req_sensor_flush"					,	{	0,	0,	0,	1, 	1,	0,	0,	0,	0,	0,	1	},	},

												//			A	E	I				F	S 	R 	A
												//		C	N	N	N	I		E	L	K 	P 	I
												//		L	I	G	F	N		N	A	R 	C 	 
												//		I	/	A	R	G	S	G	M	T		W
												//		E	M	G	S	A	E	I	E	B		N
												//		N	C	E	T	M	N	N	T	O		T
	/////////////////////////////////////////////		T	P	D	M	E	D	E	H	T		S
	// patch 1.1

	{  "we_mp_session_created"					,	{	0,	0,	0,	0, 	0,	1,	1,	0,	1,	0,	0	},	},

};

COMPILER_ASSERT( ELEMENT_COUNT( s_WorldEventTraits ) == WE_COUNT );

stringtool::EnumStringConverter& GetWorldEventConverter( void )
{
	static const char* s_Strings[ WE_COUNT ];

	static bool s_StringsBuilt = false;
	if ( !s_StringsBuilt )
	{
		s_StringsBuilt = true;

		const WorldEventEntry* i, * ibegin = s_WorldEventTraits, * iend = ARRAY_END( s_WorldEventTraits );
		const char** out = s_Strings;
		for ( i = ibegin ; i != iend ; ++i, ++out )
		{
			*out = i->m_Name;
		}
	}

	static stringtool::EnumStringConverter s_Converter( s_Strings, WE_BEGIN, WE_END, WE_INVALID );
	return ( s_Converter );
}

const char* ToString( eWorldEvent val )
{
	return ( GetWorldEventConverter().ToString( val ) );
}

bool FromString( const char* str, eWorldEvent& val )
{
	return ( GetWorldEventConverter().FromString( str, rcast <int&> ( val ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eWorldEventTraits implementation

eWorldEventTraits GetWorldEventTraits( eWorldEvent e )
{
	static eWorldEventTraits s_Traits[ WE_COUNT ];

	static bool s_TraitsBuilt = false;
	if ( !s_TraitsBuilt )
	{
		s_TraitsBuilt = true;

		const WorldEventEntry* i, * ibegin = s_WorldEventTraits, * iend = ARRAY_END( s_WorldEventTraits );
		eWorldEventTraits* out = s_Traits;
		for ( i = ibegin ; i != iend ; ++i, ++out )
		{
			*out = WMT_NONE;

			const bool* j, * jbegin = i->m_Traits, * jend = ARRAY_END( i->m_Traits );
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( *j )
				{
					*out |= (eWorldEventTraits)(1 << (j - jbegin));
				}
			}
		}
	}

	return ( s_Traits[ e ] );
}

bool TestWorldEventTraits( eWorldEvent e, eWorldEventTraits t )
{
	return ( (GetWorldEventTraits( e ) & t) != 0 );
}

bool TestWorldEventTraitsEq( eWorldEvent e, eWorldEventTraits t )
{
	return ( (GetWorldEventTraits( e ) & t) == t );
}

//////////////////////////////////////////////////////////////////////////////
// class WorldMessage implementation

#if !GP_RETAIL

inline void CheckValidWorldMessageFrom( Goid goid )
{
	gpassert( !::IsMemoryBadFood( (DWORD)goid ) );

	if (   (GetGoidClass( goid ) != GO_CLASS_GLOBAL)
		&& (GetGoidClass( goid ) != GO_CLASS_LOCAL)
		&& (goid != GOID_ANY) )
	{
		gperrorf(( "Invalid goid 0x%08X used in FROM parameter of a World Message!\n", goid ));
	}
}

inline void CheckValidWorldMessageTo( Goid goid )
{
	gpassert( !::IsMemoryBadFood( (DWORD)goid ) );

	if ( goid == GOID_INVALID )
	{
		gperror( "You can't send to an invalid Go." );
	}
	else if (   (GetGoidClass( goid ) != GO_CLASS_GLOBAL)
			 && (GetGoidClass( goid ) != GO_CLASS_LOCAL)
			 && (goid != GOID_ANY) )
	{
		gperrorf(( "Invalid goid 0x%08X used in TO parameter of a World Message!\n", goid ));
	}
}

inline void CheckValidWorldMessageFromTo( Goid from, Goid to )
{
	CheckValidWorldMessageFrom( from );
	CheckValidWorldMessageFrom( to   );

	if ( GoDb::DoesSingletonExist() )
	{
		GoHandle f( from ), t( to );
		if ( f && t )
		{
			if ( f->IsLodfi() != t->IsLodfi() )
			{
				gperrorf((
						"Sender/receiver of a world message must both be of the same lodfi/non-lodfi type.\n"
						"\n"
						"From: goid = 0x%08X, scid = 0x%08X, template = '%s'\n"
						"To  : goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
						from, f->GetScid(), f->GetTemplateName(),
						to  , t->GetScid(), t->GetTemplateName() ));
			}
		}
	}
}

inline void CheckValidWorldMessageFromTo( Goid from, Scid to )
{
	CheckValidWorldMessageFrom( from );

	if ( GoDb::DoesSingletonExist() )
	{
		if ( !::IsInstance( to ) )
		{
			gperrorf(( "Attempting to send to an invalid scid 0x%08X!\n", to ));
		}

		GoHandle f( from );
		if ( f && f->IsLodfi() )
		{
			gperror( "Can't send a world message to a scid from a lodfi go" );
		}
	}
}

#endif // !GP_RETAIL

unsigned int WorldMessage::ms_ConstructionCount = 0;

WorldMessage :: WorldMessage()
{
	Initialize();
}

WorldMessage :: WorldMessage( eWorldEvent Event )
{
	Initialize();

	m_Event = Event;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Goid To )
{
	Initialize();

	GPDEV_ONLY( CheckValidWorldMessageTo( To ) );

	m_Event				= Event;
	m_SendTo			= To;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Goid From, Goid To )
{
	Initialize();

	GPDEV_ONLY( CheckValidWorldMessageFromTo( From, To ) );

	m_Event				= Event;
	m_SendFrom			= From;
	m_SendTo			= To;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Goid From, Goid To, DWORD data )
{
	Initialize();

	GPDEV_ONLY( CheckValidWorldMessageFromTo( From, To ) );

	m_Event				= Event;
	m_SendFrom			= From;
	m_SendTo			= To;
	m_Data1				= data;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Scid To )
{
	Initialize();

	m_Event				= Event;
	m_SendToScid		= To;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Goid From, Scid To )
{
	Initialize();

	GPDEV_ONLY( CheckValidWorldMessageFromTo( From, To ) );

	m_Event				= Event;
	m_SendFrom			= From;
	m_SendToScid		= To;
}

WorldMessage :: WorldMessage( eWorldEvent Event, Goid From, Scid To, DWORD data )
{
	Initialize();

	GPDEV_ONLY( CheckValidWorldMessageFromTo( From, To ) );

	m_Event				= Event;
	m_SendFrom			= From;
	m_SendToScid		= To;
	m_Data1				= data;
}

bool WorldMessage :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Event",        m_Event        );
	persist.Xfer( "m_SendFrom",     m_SendFrom     );
	persist.Xfer( "m_SendTo",       m_SendTo       );
	persist.Xfer( "m_SendToScid",   m_SendToScid   );
	persist.Xfer( "m_MaturityTime", m_MaturityTime );

	FUBI_XFER_BITFIELD_BOOL( persist, "m_bBeingSent", m_bBeingSent );
	FUBI_XFER_BITFIELD_BOOL( persist, "m_bCC", m_bCC );
	FUBI_XFER_BITFIELD_BOOL( persist, "m_bBroadcast", m_bBroadcast );

	persist.Xfer( "m_Data1",        m_Data1        );

	GPDEV_ONLY( persist.Xfer( "m_DebugSendTime", m_DebugSendTime ) );

	return ( true );
}

void WorldMessage :: Initialize()
{
	m_Event			= WE_INVALID;
	m_SendFrom		= (Goid)GOID_INVALID;
	m_SendTo		= GOID_INVALID;
	m_SendToScid	= SCID_INVALID;
	m_MaturityTime	= 0.0;
	m_Data1			= 0;
	m_bCC					= false;
	m_bBroadcast			= false;
	m_bBeingSent			= false;

#if !GP_RETAIL
	m_DebugSendTime	= gWorldTime.GetTime();
#endif

	++ms_ConstructionCount;
}

#if GP_RETAIL
#define SET_INSIDE_SKRIT()
#else // GP_RETAIL
static DECL_THREAD int s_IsSkritFunction = 0;
struct AutoInc
{
	int& m_I;

	AutoInc( int& i ) : m_I( i )  {  gpassert( m_I >= 0 );  ++m_I;  }
   ~AutoInc( void )  {  --m_I;  gpassert( m_I >= 0 );  }
};
#define SET_INSIDE_SKRIT() AutoInc autoInc( s_IsSkritFunction )
#endif // GP_RETAIL

#if GP_DEBUG

bool CheckSkritVsEngine( eWorldEvent event )
{
	// not in skrit? we're ok
	if ( !s_IsSkritFunction )
	{
		return ( true );
	}

	// not an engine-only event? we're ok
	if ( !IsEngineOnlyEvent( event ) )
	{
		return ( true );
	}

	// special exceptions are made for nis mode where anything goes because it's a highly controlled sequence
	if ( WorldState::DoesSingletonExist() && (gWorldState.GetCurrentState() == WS_SP_NIS) )
	{
		return ( true );
	}

	// guess it's not ok
	return ( false );
}

void AssertSkritVsEngine( eWorldEvent event )
{
	gpassertf( CheckSkritVsEngine( event ),
			 ( "Bad! Message %s may only be called by engine functions!", ToString( event ) ));
}

#endif // GP_DEBUG

void WorldMessage :: Send( MESSAGE_DISPATCH_FLAG Flags ) const
{
	gMessageDispatch.Send( const_cast<WorldMessage&>(*this), Flags );
}

void WorldMessage :: SendDelayed( float delay, MESSAGE_DISPATCH_FLAG Flags ) const
{
	delay = max( 0.0f, delay );	// filter out the odd negative delay - possible in MP
	const_cast<WorldMessage&>(*this).SetDelayTime( delay );
	gMessageDispatch.SendDelayed( const_cast<WorldMessage&>(*this), Flags );
}

void WorldMessage :: SSend( MESSAGE_DISPATCH_FLAG Flags ) const
{
	GPDEBUG_ONLY( AssertSkritVsEngine( GetEvent() ) );
	gMessageDispatch.SSend( const_cast<WorldMessage&>(*this), Flags );
}

void WorldMessage :: SSendDelayed( float delay, MESSAGE_DISPATCH_FLAG Flags ) const
{
	delay = max( 0.0f, delay );	// filter out the odd negative delay - possible in MP
	const_cast<WorldMessage&>(*this).SetDelayTime( delay );
	gMessageDispatch.SSendDelayed( const_cast<WorldMessage&>(*this), Flags );
}

void WorldMessage :: SetDelayTime( float Delay )
{
	m_MaturityTime = PreciseAdd( gWorldTime.GetTime(), Delay );
}

//////////////////////////////////////////////////////////////////////////////
// class WorldMessage (Goid helpers) implementation

void SendWorldMessage( eWorldEvent event, Goid From, Goid To )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To ).Send();
}

void SendWorldMessage( eWorldEvent event, Goid From, Goid To, DWORD data )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To, data ).Send();
}

void PostWorldMessage( eWorldEvent event, Goid from, Goid to, DWORD data, float delay )
{
	SET_INSIDE_SKRIT();

	WorldMessage message( event, from, to, data );
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	message.SendDelayed( delay );
}

void PostWorldMessage( eWorldEvent event, Goid from, Goid to, float delay )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage message( event, from, to );
	message.SendDelayed( delay );
}


//////////////////////////////////////////////////////////////////////////////
// class WorldMessage (Scid helpers) implementation

void SendWorldMessage( eWorldEvent event, Goid From, Scid To )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To ).Send();
}

void SendWorldMessage( eWorldEvent event, Goid From, Scid To, DWORD data )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To, data ).Send();
}

void SSendWorldMessage( eWorldEvent event, Goid From, Scid To )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To ).SSend();
}

void SSendWorldMessage( eWorldEvent event, Goid From, Scid To, DWORD data )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage( event, From, To, data ).SSend();
}

void PostWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data, float delay )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage message( event, from, to, data );
	message.SendDelayed( delay );
}

void PostWorldMessage( eWorldEvent event, Goid from, Scid to, float delay )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage message( event, from, to );
	message.SendDelayed( delay );
}

void SPostWorldMessage( eWorldEvent event, Goid from, Scid to, DWORD data, float delay )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage message( event, from, to, data );
	message.SSendDelayed( delay );
}

void SPostWorldMessage( eWorldEvent event, Goid from, Scid to, float delay )
{
	SET_INSIDE_SKRIT();
	GPDEBUG_ONLY( AssertSkritVsEngine( event ) );

	WorldMessage message( event, from, to );
	message.SSendDelayed( delay );
}


//////////////////////////////////////////////////////////////////////////////
