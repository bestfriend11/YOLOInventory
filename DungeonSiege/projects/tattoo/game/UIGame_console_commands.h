#pragma once
#ifndef __UIGAME_CONSOLE_COMMANDS
#define __UIGAME_CONSOLE_COMMANDS

#if !GP_RETAIL

class GpConsole_command;


/*=======================================================================================

  UIGame_console_commands

  purpose:	Strictly for oganizational purposes, most of the console commands were just
			grouped in this class.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class UIGame_console_commands
{
public:

	UIGame_console_commands();
	~UIGame_console_commands();

private:

	std::auto_ptr< GpConsole_command > mgpc_blowtorch;
	std::auto_ptr< GpConsole_command > mgpc_bool;
	std::auto_ptr< GpConsole_command > mgpc_camera;
	std::auto_ptr< GpConsole_command > mgpc_exit;
	std::auto_ptr< GpConsole_command > mgpc_gamma;
	std::auto_ptr< GpConsole_command > mgpc_sysgamma;
	std::auto_ptr< GpConsole_command > mgpc_infobox;
	std::auto_ptr< GpConsole_command > mgpc_kill;
	std::auto_ptr< GpConsole_command > mgpc_nodeinfo;
	std::auto_ptr< GpConsole_command > mgpc_meshcheck;
	std::auto_ptr< GpConsole_command > mgpc_frustuminfo;
	std::auto_ptr< GpConsole_command > mgpc_sound;
	std::auto_ptr< GpConsole_command > mgpc_sim;
	std::auto_ptr< GpConsole_command > mgpc_siegever;
	std::auto_ptr< GpConsole_command > mgpc_stats;
	std::auto_ptr< GpConsole_command > mgpc_sysinfo;
	std::auto_ptr< GpConsole_command > mgpc_teleport;
	std::auto_ptr< GpConsole_command > mgpc_weather;
	std::auto_ptr< GpConsole_command > mgpc_rapi;
	std::auto_ptr< GpConsole_command > mgpc_mcp;
	std::auto_ptr< GpConsole_command > mgpc_report;
	std::auto_ptr< GpConsole_command > mgpc_exec;
	std::auto_ptr< GpConsole_command > mgpc_quantify;
	std::auto_ptr< GpConsole_command > mgpc_heads;
	std::auto_ptr< GpConsole_command > mgpc_party;
	std::auto_ptr< GpConsole_command > mgpc_godb;
	std::auto_ptr< GpConsole_command > mgpc_triggersys;
	std::auto_ptr< GpConsole_command > mgpc_perfmon;
};


#endif // !GP_RETAIL

#endif

