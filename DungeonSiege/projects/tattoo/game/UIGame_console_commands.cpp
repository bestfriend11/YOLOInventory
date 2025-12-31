#include "Precomp_Game.h"

#if !GP_RETAIL

#include "UiGame_Console_Commands.h"

#include "AIQuery.h"
#include "AppModule.h"
#include "CameraPosition.h"
#include "enchantment.h"
#include "FileSysXfer.h"
#include "Flamethrower_Tracker.h"
#include "Flamethrower_types.h"
#include "FuBiPersistImpl.h"
#include "FuBiSchema.h"
#include "GamePersist.h"
#include "Go.h"
#include "GoActor.h"
#include "GoBody.h"
#include "GoDb.h"
#include "GoFollower.h"
#include "GoMind.h"
#include "GoCore.h"
#include "GoAspect.h"
#include "GoPhysics.h"
#include "GpConsole.h"
#include "GpStats.h"
#include "UICommands.h"
#include "gps_manager.h"
#include "MCP.h"
#include "Mood.h"
#include "NetLog.h"
#include "PContentDb.h"
#include "Player.h"
#include "Quantify.h"
#include "RapiMouse.h"
#include "RapiOwner.h"
#include "ReportSys.h"
#include "Rules.h"
#include "Server.h"
#include "Services.h"
#include "Sim.h"
#include "Siege_Camera.h"
#include "Siege_Console.h"
#include "siege_compass.h"
#include "siege_frustum.h"
#include "Siege_Mesh.h"
#include "siege_mesh_door.h"
#include "Siege_Mouse_shadow.h"
#include "siege_engine.h"
#include "siege_loader.h"
#include "siege_hotpoint_database.h"
#include "siege_options.h"
#include "SkritEngine.h"
#include "System_Report.h"
#include "TankMgr.h"
#include "TattooGame.h"
#include "Timemgr.h"
#include "Ui.h"
#include "UiCamera.h"
#include "UIFrontEnd.h"
#include "UIGame.h"
#include "UIPartyManager.h"
#include "UICharacterSelect.h"
#include "Victory.h"
#include "Weather.h"
#include "WinX.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldSound.h"
#include "WorldTerrain.h"
#include "WorldOptions.h"
#include "WorldMessage.h"
#include "WorldState.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <list>

using namespace siege;


//
//----- game object management related console commands
//
// bool aiqueryboxes										// volume information for ai queries
// bool aiquerycache										// internal caching of ai query results
// bool alwaysgib											// everything will gib when killed
// bool atmosphere											// mood (music, fog) and weather (rain, wind, snow) control
// bool boxes												// engine boxes, such as object/mouse collision
// bool collisionboxes										// physics collision boxes
// bool culleffects											// special effects culling (fire, spell effects)
// bool defeat												// player cannot be defeated
// bool dlights												// dynamic light control (moving lights)
// bool ignorevertexcolors									// full bright render mode, ignores lighting
// bool drawdoors											// draw node connection doors
// bool drawfadednodes										// draw nodes that are currently faded out
// bool drawnormals											// draw vertex normals used for lighting calcs
// bool drawlogicalnodes									// draw logical nodes used in pathing calcs
// bool dshading											// dynamic shading control (lighting on objects from dynamic lights)
// bool dshadows											// dynamic shadow control (shadows cast from objects off of dynamic lights)
// bool floor												// highlight the polygons that are marked as floor in the nodes
// bool fps													// fps/debug render information console
// bool gore												// gore and blood control
// bool gpwarning											// disable warnings in the report system
// bool gperror												// disable errors in the report system
// bool gui													// hide/show the gui
// bool gui_cursor											// hide/show the cursor
// bool lightrays											// draw rays indicating light directions relative to objects/nodes
// bool monsterai											// monster intelligence control
// bool mood												// control of mood (music, fog)
// bool objcost												// color nodes based on cost of objects in them
// bool polystats											// information about polygons in active scene
// bool cpustats											// detailed information about current subsystem cpu usage
// bool tablestats											// display table component of cpustats
// bool graphstats											// display graph component of cpustats
// bool historystats										// display history component of cpustats
// bool polyalert											// enables onscreen warning about high poly counts
// bool profiler											// turn on and off function timing profiler
// bool showall												// show all ingame objects, including hidden ones (like the party object)
// bool shownone											// show no ingame objects
// bool showmpusage											// multiplayer usage statistics
// bool slights												// static light control (placed lights, non moving)
// bool spacetree											// draw connected graph of current inherited space tree
// bool sshading											// static shading control (lighting on objects from staic lights)
// bool sshadows											// static shadow control (shadows cast from objects off of static lights)
// bool weather												// control of weather (rain, snow, wind)
// bool wireframe											// draw the world in wireframe mode
// bool allowselectall										// allow the user to select any object, even those not marked as selectable
// bool showbones											// draw the bone structure of each go
// bool expensiveshadows									// toggle expensive shadow drawing
// bool water												// highlight the polygons of a node marked as water
// bool mcp													// misc. debug information about the mcp (path follower)
// bool tooltips											// control of user education tooltips
// bool fixedworldfrustum									// keep the world frustums from moving with their owner
// bool templates											// display gui specific information on rollover
// bool tuning												// extra info output for game balancing and pcontent
// bool noscids												// disable game object loading
// bool notextures											// disable texture loading
// bool gizmos												// draw gizmos (like camera orders)
// bool compass												// disable the compass
// bool allowselectnone										// do not allow the user to select anything
// bool friendlyfire										// allow attacks to hit allies
// bool discoveryfog										// control of discovery fog in the megamap
// bool decals												// decal drawing control
// bool camverttrack										// control of vertical movement of camera based on cursor position
// bool camhoriztrack										// control of horizontal movement of camera based on cursor position
// bool shadowspheres										// draw the bounding sphere of an object used for shadow calcs
// bool logskritcalls										// control of skrit call logging for debugging purposes
// bool save_as_text										// save games written as plain text
// bool save_as_tank										// save games written as compressed tank files
// bool simplerender										// simple render mode, which disables multitex and complex blend modes
// bool eliminatewaypoints									// linear waypoint elimination in paths
// bool dslog												// control of the internal net log
// exit														// exit the game
// sfx attach < parent_id > < child_id >					// adds effect to another effect
// sfx create < name >										// creates effect
// sfx rat < effect_id >									// removes all targets
// sfx destroy < effect_id|all >							// destroy effect
// sfx effects												// lists available effects
// sfx scripts												// lists available scripts
// sfx list [scripts]										// displays active effects or scripts
// sfx start < effect_id|all >								// runs effect
// sfx stop < effect_id|all|scripts|script < script_id > >	// stops effect(s) or script(s)
// sfx target < effect_id > < target_id >					// adds effect as a target
// sfx reload												// reloads all effect scripts
// sfx run < script name >									// runs an effect script locally
// sfx srun < script name >									// runs an effect script globally
// sfx erun < script name >									// runs an effect script on active equipped item locally
// sfx esrun < script name >								// runs an effect script on active equipped item globally
// sfx unload < script id >									// force unloads a script
// sfx load < script id >									// force loads a previously unloaded script
// sfx lcf < scale factor >									// amount to scale all lightsources culling radius by
// kill														// kills selected actors
// teleport													// list available bookmarks
// teleport <bookmark|map|node|position>					// teleport the selected go to the given location
// sound enable												// enables the sound system
// sound disable											// disables the sound system
// sound lplay <filename>									// loops the playback of a sample
// sound play <filename>									// plays a sample one time
// sound lstream <filename>									// loops the playback of a stream
// sound stream <filename>									// plays a sample as a stream
// sound stopsample <SOUNDID>								// stops a sample from playing
// sound stopstream <SOUNDID>								// stops a stream from playing
// sound history											// dumps history of sound usage to the console
// sim info													// information about current simulations
// sim start												// starts simulation of selected object(s)
// sim stop													// stops simulating all currently simulated objects
// sim x													// offset x position
// sim y													// offset y position
// sim z													// offset z position
// sim explode												// explode selected game object(s) with specified magnitude
// sim burn													// causes a go to catch on fire
// sim sdvolume <start_rad><end_rad><damage><dur>			// creates a damage volume using two selected gos static position
// sim ddvolume <start_rad><end_rad><damage><dur><length>	// creates a damage volume using two selected gos dynamic position
// sim debug												// enable debug help
// siegever													// print out game version
// stats uber												// display/change uber level
// stats int												// display/change intelligence
// stats str												// display/change strength
// stats dex												// display/change dexterity
// stats melee												// display/change melee
// stats ranged												// display/change ranged
// stats nmagic												// display/change nature magic
// stats cmagic												// display/change combat magic
// stats xp_uber											// display/change uber level
// stats xp_int												// display/change intelligence
// stats xp_str												// display/change strength
// stats xp_dex												// display/change dexterity
// stats xp_melee											// display/change melee
// stats xp_ranged											// display/change ranged
// stats xp_nmagic											// display/change nature magic
// stats xp_cmagic											// display/change combat magic
// stats xp <skill name>									// display/change set xp value by xp name
// stats life												// display/change life
// stats mana												// display/change mana
// stats lru												// display/change life recovery unit
// stats lrp												// display/change life recovery period
// stats mru												// display/change mana recovery unit
// stats mrp												// display/change mana recovery period
// stats sb													// display/change anim speed bias
// stats sr													// display/change sight range
// stats sa													// display/change sight fov
// stats min												// minimum stats
// stats med												// medium stats
// stats max												// maximum stats
// stats inv												// toggle invincibility on/off
// stats live												// change lifestate to alive
// stats levels												// displays all attained levels
// stats easy_diff											// sets the difficulty level to easy
// stats medium_diff										// sets the difficulty level to medium
// stats hard_diff											// sets the difficulty level to hard
// stats pdc												// enables/disables particle damage counter display
// stats rpdc												// resets the particle damage counter
// stats tpdc <seconds>										// sets the particle damage counters reset period time
// weather fog info											// display current fog information
// weather fog on											// turn the fog on
// weather fog off											// turn the fog off
// weather fog color										// set the fog color
// weather fog neardist										// set the near distance for the fog table
// weather fog fardist										// set the far distance for the fog table
// weather rain info										// display current rain information
// weather rain on											// turn the rain on
// weather rain off											// turn the rain off
// weather rain density										// set the rain density (number of drops)
// weather wind info										// display current wind info
// weather wind on											// turn the wind on
// weather wind off											// turn the wind off
// weather wind velocity									// set the wind velocity (meters per second)
// weather snow info										// display current snow info
// weather snow on											// turn the snow on
// weather snow off											// turn the snow off
// weather snow density										// set the snow density (number of flakes)
// sysinfo													// show information about the machine and OS
// camera nearclip											// change the near clipping plane distance
// camera farclip											// change the far clipping plane distance
// camera followmode										// activate camera follow mode
// camera springlook										// activate camera spring look mode
// gamma													// control the current screen gamma (brightness)
// sysgamma													// control the system gamma correction
// nodeinfo													// output information about the node currently under the cursor
// meshcheck												// numerical bounds checking to detect imprecise dooring
// frustuminfo												// display information about the currently active frustum
// infobox													// change the information displayed in the information box
// rapi info												// display rendering system information
// rapi cliptofog											// control clipping planes (snap to fog distance)
// mcp path													// display pathing information
// mcp optimize												// control of path optimization (straightening)
// mcp lag <val>											// set the current multiplayer lag time
// mcp label												// display informational labels
// mcp hud													// display mcp debugging huds
// mcp goal													// draw pathing goals
// mcp block												// draw path blocking
// mcp debug												// activate full debugging
// mcp test													// activate testing
// mcp crowding												// crowd control
// report													// display available report streams
// report clearignores										// turn off stream ignoring
// report enable <stream>									// enable a report stream
// report disable <stream>									// disable a report stream
// report filelog <stream> [<filename>]						// dump a report stream to the given file
// report nofilelog <stream>								// turn off file dumping for the given stream
// report <stream>											// toggle enabled state of the given stream
// exec														// print out all available execution commands and block names
// exec add <blockname>										// add exec entries from the block, overwriting existing entries
// exec clear												// clear out all existing blocks
// exec <commandname>										// execute the command by name
// quantify <number>										// count of frames to sample for and then snapshot
// quantify console <number>								// count of console commands to sample for and then snapshot
// head <new_head_name>										// change the head of the currently selected character
// party													// list all available parties for import
// party load<name>											// replace current party with saved party
// party save<name>											// save party with given name
// party add<name>											// add saved party to current party
// godb expire_now											// force all currently expiring objects to expire immediately
// triggersys display										// toggles display of trigger system debugging info
// triggersys reports										// toggles reporting of trigger system debugging info
// triggersys hitmarks										// toggles display of trigger system boundary hits
// 

//================================================================
//
// gpc_bool
//
// This is a GpConsole_command for flipping all the little
// booleans in the game.  It seemed too tedious to have a class
// for each little boolean.
//
//----------------------------------------------------------------

class gpc_bool : public GpConsole_command
{
	bool		m_bAtmosphere;

public:
	gpc_bool();
	~gpc_bool(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_bool::gpc_bool()
	: m_bAtmosphere( true )
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help += "bool <profiler|wireframe|boxes|collisionboxes|dlights|slights|\n";
	help += "      fps|polystats|floor|dshadows|sshadows|dshading|sshading|\n";
	help += "      ignorevertexcolors|drawdoors|culleffects|mood|\n";
	help += "      drawnormals|drawlogicalnodes|lightrays|weather|atmosphere|\n";
	help += "      showall|shownone|showmpusage|allowselectall|showbones|\n";
	help += "      expensiveshadows|water|fixedworldfrustum|monsterai|drawfadednodes|\n";
	help += "      templates|tuning|noscids|spacetree|discoveryfog|alwaysgib|decals|\n";
	help += "      camverttrack|camhoriztrack|shadowspheres|aiqueryboxes|aiquerycache|\n";
	help += "      notextures|logskritcalls|save_as_text|save_as_tank|simplerender|>\n";
	help += "      eliminatewaypoints|dslog|objcost>\n";
	help += " - toggles options\n";
	usage.push_back( "bool aiqueryboxes" );
	usage.push_back( "bool aiquerycache" );
	usage.push_back( "bool alwaysgib" );
	usage.push_back( "bool atmosphere" );
	usage.push_back( "bool boxes" );
	usage.push_back( "bool collisionboxes" );
	usage.push_back( "bool culleffects" );
	usage.push_back( "bool defeat" );
	usage.push_back( "bool dlights" );
	usage.push_back( "bool ignorevertexcolors" );
	usage.push_back( "bool drawdoors" );
	usage.push_back( "bool drawfadednodes" );
	usage.push_back( "bool drawnormals" );
	usage.push_back( "bool drawlogicalnodes" );
	usage.push_back( "bool dshading" );
	usage.push_back( "bool dshadows" );
	usage.push_back( "bool floor" );
	usage.push_back( "bool fps" );
	usage.push_back( "bool gore" );
	usage.push_back( "bool gpwarning" );
	usage.push_back( "bool gperror" );
	usage.push_back( "bool gui" );
	usage.push_back( "bool gui_cursor" );
	usage.push_back( "bool lightrays" );
	usage.push_back( "bool monsterai" );
	usage.push_back( "bool mood" );
	usage.push_back( "bool objcost" );
	usage.push_back( "bool polystats" );
	usage.push_back( "bool cpustats" );
	usage.push_back( "bool tablestats" );
	usage.push_back( "bool graphstats" );
	usage.push_back( "bool historystats" );
	usage.push_back( "bool polyalert" );
	usage.push_back( "bool profiler" );
	usage.push_back( "bool showall" );
	usage.push_back( "bool shownone" );
	usage.push_back( "bool showmpusage" );
	usage.push_back( "bool slights" );
	usage.push_back( "bool spacetree" );
	usage.push_back( "bool sshading" );
	usage.push_back( "bool sshadows" );
	usage.push_back( "bool weather" );
	usage.push_back( "bool wireframe" );
	usage.push_back( "bool allowselectall" );
	usage.push_back( "bool showbones" );
	usage.push_back( "bool expensiveshadows" );
	usage.push_back( "bool water" );
	usage.push_back( "bool mcp" );
	usage.push_back( "bool tooltips" );

	usage.push_back( "bool fixedworldfrustum" );
	usage.push_back( "bool templates" );
	usage.push_back( "bool tuning" );
	usage.push_back( "bool noscids" );
	usage.push_back( "bool notextures" );

	usage.push_back( "bool gizmos"			);
	usage.push_back( "bool compass"					);

	usage.push_back( "bool allowselectnone" );

	usage.push_back( "bool friendlyfire" );

	usage.push_back( "bool discoveryfog" );
	usage.push_back( "bool decals" );

	usage.push_back( "bool camverttrack" );
	usage.push_back( "bool camhoriztrack" );
	usage.push_back( "bool shadowspheres" );
	usage.push_back( "bool logskritcalls" );
	usage.push_back( "bool save_as_text" );
	usage.push_back( "bool save_as_tank" );
	usage.push_back( "bool simplerender" );
	usage.push_back( "bool eliminatewaypoints" );
	usage.push_back( "bool dslog" );

	gGpConsole.RegisterCommand( *this, "bool", help, usage );
};


bool gpc_bool::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	gpstring boolname = args.GetAsString( 1 );
	bool processed = false;
	if( boolname.same_no_case( "profiler" ) ) {
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_PROFILER );
		gProfiler.SetIsActive( gTattooGame.TestOptions( TattooGame::GOPTION_TRACE_PROFILER ) );
		processed = true;
	}
	if( boolname.same_no_case( "atmosphere" ) ) {
		if( m_bAtmosphere ) {
			m_bAtmosphere = false;
			gMood.SetEnabled( m_bAtmosphere );
			gWeather.SetEnabled( m_bAtmosphere );
			output = "Moods and Weather DISABLED\n";
		} else {
			m_bAtmosphere = true;
			gMood.SetEnabled( m_bAtmosphere );
			gWeather.SetEnabled( m_bAtmosphere );
			output = "Moods and Weather ENABLED\n";
		}
	}
	if( boolname.same_no_case( "mood" ) ) {
		if( gMood.GetEnabled() ) {
			gMood.SetEnabled( false );
			output = " Mood update DISABLED\n";
		} else {
			gMood.SetEnabled( true );
			output = " Mood update ACTIVE\n";
		}
	}
	if( boolname.same_no_case( "weather" ) ) {
		if( gWeather.GetEnabled() ) {
			gWeather.SetEnabled( false );
			output = " Weather update DISABLED\n";
		} else {
			gWeather.SetEnabled( true );
			output = " Weather update ACTIVE\n";
		}
	}

	if( boolname.same_no_case( "boxes" ) ){
		gSiegeEngine.GetOptions().ToggleNodeBoxes();
	}

	if( boolname.same_no_case( "collisionboxes" ) )  {
		gSiegeEngine.GetOptions().ToggleCollisionBoxes();
	}

	if( boolname.same_no_case( "wireframe" ) ) {
		gSiegeEngine.GetOptions().ToggleWireframe();
    }

	if( boolname.same_no_case( "fps" ) ) {
		gSiegeEngine.GetOptions().ToggleFPS();
    }

	if( boolname.same_no_case( "polystats" ) ) {
		gSiegeEngine.GetOptions().TogglePolyStats();
    }

	if( boolname.same_no_case( "cpustats" ) )
	{
#		if GP_ENABLE_STATS
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_CPUSTATS );
		bool enabled = gTattooGame.TestOptions( TattooGame::GOPTION_TRACE_CPUSTATS );
		gGpStatsMgr.Enable( enabled );
		gGpStatsMgr.SetLogSampleHistory( enabled, 500 );
#		endif // GP_ENABLE_STATS
    }

	if( boolname.same_no_case( "tablestats" ) )
	{
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_CPUSTATS_TABLE );
    }

	if( boolname.same_no_case( "graphstats" ) )
	{
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_CPUSTATS_GRAPH );
    }

	if( boolname.same_no_case( "historystats" ) )
	{
		gTattooGame.ToggleOptions( TattooGame::GOPTION_TRACE_CPUSTATS_HISTORY );
    }

	if( boolname.same_no_case( "polyalert" ) ) {
		gUIGame.SetPolyAlert( !gUIGame.GetPolyAlert() );
	}

	if( boolname.same_no_case( "slights" ) ) {
		gSiegeEngine.GetOptions().ToggleStaticLighting();
    }

	if( boolname.same_no_case( "dlights" ) ) {
		gSiegeEngine.GetOptions().ToggleDynamicLighting();
    }

	if( boolname.same_no_case( "culleffects" ) ) {
		gSiegeEngine.GetOptions().ToggleEffectsCulling();
    }

	if( boolname.same_no_case( "dshadows" ) ){
		gSiegeEngine.GetOptions().ToggleDynamicShadows();
	}

	if( boolname.same_no_case( "sshadows" ) ){
		gSiegeEngine.GetOptions().ToggleStaticShadows();
	}

	if( boolname.same_no_case( "dshading" ) ){
		gSiegeEngine.GetOptions().ToggleDynamicShading();
	}

	if( boolname.same_no_case( "sshading" ) ){
		gSiegeEngine.GetOptions().ToggleStaticShading();
	}

	if( boolname.same_no_case( "ignorevertexcolors" ) ){
		gDefaultRapi.IgnoreVertexColor( !gDefaultRapi.IsIgnoringVertexColor() );
	}

	if( boolname.same_no_case( "drawdoors" ) ){
		gSiegeEngine.GetOptions().ToggleDoorDrawing();
	}

	if( boolname.same_no_case( "spacetree" ) ){
		gSiegeEngine.GetOptions().ToggleSpaceTree();
	}

	if( boolname.same_no_case( "floor" ) ){
		gSiegeEngine.GetOptions().ToggleFloors();
	}

	if( boolname.same_no_case( "friendlyfire" ) ){
		gUIGame.GetUICommands()->SetFriendlyFire( !gUIGame.GetUICommands()->GetFriendlyFire() );
	}

	if( boolname.same_no_case( "water" ) ){
		gSiegeEngine.GetOptions().ToggleWater();
	}

	if( boolname.same_no_case( "drawnormals" ) ){
		gSiegeEngine.GetOptions().ToggleNormalDrawing();
	}

	if( boolname.same_no_case( "drawfadednodes" ) ){
		gSiegeEngine.GetOptions().ToggleFadedNodeDrawing();
	}

	if( boolname.same_no_case( "lightrays" ) ){
		gSiegeEngine.GetOptions().ToggleLightRaysDrawing();
	}

	if( boolname.same_no_case( "gore" ) ){
		const bool bGore = gWorldOptions.GetAllowDismemberment();

		gWorldOptions.SetAllowDismemberment( !bGore );
		if( !bGore )
		{
			output = " Gore On!\n";
		}
		else
		{
			output = " Gore OFF!\n";
		}
	}

	if ( boolname.same_no_case( "gui" ) ) {
		gUI.GetGameGUI().SetVisible( !gUI.GetGameGUI().GetVisible() );
		gUIGame.GetUIPartyManager()->CloseAllCharacterMenus();
	}

	if ( boolname.same_no_case( "gui_cursor" ) ) {
		bool enable = !gUI.GetGameGUI().GetDrawCursor();
		gUI.GetGameGUI().SetDrawCursor( enable );
		if ( enable )
		{
			gRapiMouse.Enable();
		}
		else
		{
			gRapiMouse.Disable();
		}
	}

	if( boolname.same_no_case( "drawlogicalnodes" ) ){
		gSiegeEngine.GetOptions().ToggleDrawLogicalNodes();
	}

	if( boolname.same_no_case( "gpwarning" ) ) {
		ReportSys::ToggleWarningContexts();
	}

	if( boolname.same_no_case( "gperror" ) ) {
		ReportSys::ToggleErrorContexts();
	}

	if( boolname.same_no_case( "showall" ) ) {
		gWorldOptions.SetShowAll( !gWorldOptions.GetShowAll() );
	}

	if( boolname.same_no_case( "shownone" ) ) {
		gWorldOptions.SetShowObjects( !gWorldOptions.GetShowObjects() );
	}

	if( boolname.same_no_case( "showmpusage" ) ) {
		gWorldOptions.SetShowMpUsage( !gWorldOptions.GetShowMpUsage() );
	}

	if( boolname.same_no_case( "allowselectall" ) ) {
		gSiegeEngine.GetOptions().ToggleSelectAllObjects();
	}

	if( boolname.same_no_case( "allowselectnone" ) ) {
		gUIPartyManager.SetMustHaveSelection( !gUIPartyManager.GetMustHaveSelection() );
	}

	if( boolname.same_no_case( "showbones" ) ) {
		gSiegeEngine.GetOptions().ToggleShowBones();
	}

	if( boolname.same_no_case( "expensiveshadows" ) ) {
		gSiegeEngine.GetOptions().ToggleExpensiveShadows();
	}

	if( boolname.same_no_case( "compass" ) ) {
		gSiegeEngine.GetCompass().SetActive( !gSiegeEngine.GetCompass().GetActive() );
		if ( gSiegeEngine.GetCompass().GetActive() )
		{
			gUIShell.ShowInterface( "compass_hotpoints" );
		}
		else
		{
			gUIShell.HideInterface( "compass_hotpoints" );
		}
	}

	if( boolname.same_no_case( "gizmos" ) ){
		gWorldOptions.SetShowGizmos( !gWorldOptions.GetShowGizmos() );
	}

	if( boolname.same_no_case( "mcp" ) ){
		gWorldOptions.SetShowMCP( !gWorldOptions.GetShowMCP() );
	}

	if( boolname.same_no_case( "tooltips" ) ){
		gUIShell.SetToolTipsVisible( !gUIShell.GetToolTipsVisible() );
	}

	if( boolname.same_no_case( "fixedworldfrustum" ) ){
		gWorldOptions.SetFixedWorldFrustum( !gWorldOptions.GetFixedWorldFrustum() );
	}

	if ( boolname.same_no_case( "templates" ) )
	{
		gWorldOptions.SetDebugToolTips( !gWorldOptions.GetDebugToolTips() );
	}

	if ( boolname.same_no_case( "tuning" ) )
	{
		GetPContentErrorContext().Toggle();
		gpgenericf(( "Now %s the pcontent tuning errors...\n", GetPContentErrorContext().IsEnabled() ? "enabling" : "disabling" ));
	}

	if( boolname.same_no_case( "noscids" ) )
	{
		gWorldOptions.SetSkipContent( !gWorldOptions.GetSkipContent() );
	}

	if( boolname.same_no_case( "notextures" ) )
	{
		gDefaultRapi.SetNoTextures( !gDefaultRapi.GetNoTextures() );
	}

	if( boolname.same_no_case( "monsterai" ) )
	{
		GoMind::ToggleOptions( GoMind::OPTION_UPDATE_COMPUTER );
	}

	if ( boolname.same_no_case( "defeat" ) )
	{
		gVictory.ToggleAllowDefeat();
	}

	if( boolname.same_no_case( "discoveryfog" ) )
	{
		gSiegeEngine.GetOptions().ToggleDiscoveryFog();
	}

	if ( boolname.same_no_case( "alwaysgib" ) )
	{
		gWorldOptions.SetAlwaysGib( !gWorldOptions.GetAlwaysGib() );
	}

	if ( boolname.same_no_case( "objcost" ) )
	{
		gWorldOptions.SetShowObjectCost( !gWorldOptions.GetShowObjectCost() );
	}

	if ( boolname.same_no_case( "aiqueryboxes" ) )
	{
		gWorldOptions.SetAiQueryBoxes( !gWorldOptions.GetAiQueryBoxes() );
	}

	if ( boolname.same_no_case( "aiquerycache" ) )
	{
		gAIQuery.SetQueryCacheEnabled( !gAIQuery.GetQueryCacheEnabled() );
	}

	if( boolname.same_no_case( "decals" ) )
	{
		gSiegeEngine.GetOptions().ToggleDecals();
	}

	if( boolname.same_no_case( "camhoriztrack" ) )
	{
		gUIGame.SetCameraScreenHorizTrack( !gUIGame.IsCameraScreenHorizTrack() );
	}

	if( boolname.same_no_case( "camverttrack" ) )
	{
		gUIGame.SetCameraScreenVertTrack( !gUIGame.IsCameraScreenVertTrack() );
	}

	if( boolname.same_no_case( "shadowspheres" ) )
	{
		gSiegeEngine.GetOptions().ToggleShadowSpheres();
	}

	if( boolname.same_no_case( "save_as_text" ) )
	{
		gWorldOptions.SetSaveAsText( !gWorldOptions.GetSaveAsText() );
		gpgenericf(( "%s additionally saving out save game as text\n", gWorldOptions.GetSaveAsText() ? "Now" : "No longer" ));
	}

	if( boolname.same_no_case( "save_as_tank" ) )
	{
		gWorldOptions.SetSaveAsTank( !gWorldOptions.GetSaveAsTank() );
		gpgenericf(( "%s saving into tank files\n", gWorldOptions.GetSaveAsTank() ? "Now" : "No longer" ));
	}

	if( boolname.same_no_case( "simplerender" ) )
	{
		if( gDefaultRapiPtr != NULL )
		{
			gDefaultRapi.SetMultiTexBlend( !gDefaultRapi.IsMultiTexBlend() );
		}
	}

	if( boolname.same_no_case( "eliminatewaypoints" ) )
	{
		gWorldOptions.SetEliminateWaypoints( !gWorldOptions.GetEliminateWaypoints() );
	}

	if( boolname.same_no_case( "dslog" ) )
	{
		if ( gNetLog.IsLogging() )
		{
			gNetLog.StopLogging();
		}
		else
		{
			gNetLog.StartLogging( false );
		}
	}


	return( processed );
}




//================================================================
//
// gpc_exit
//
// This command sends a WM_CLOSE message to the main window to end
// the application
//
//----------------------------------------------------------------

class gpc_exit : public GpConsole_command
{
public:
	gpc_exit();
	~gpc_exit(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_exit::gpc_exit()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "exit - exits Dungeon Siege";
	usage.push_back( "exit" );

	gGpConsole.RegisterCommand( *this, "exit", help, usage );
};


bool gpc_exit::Execute( GpConsole_command_arguments & /*args*/, gpstring & /*output*/ )
{
	gTattooGame.ExitGame();
	return true;
}


//================================================================
//
// gpc_blowtorch
//
// This is a GpConsole_command for communicating with Flamethrower
//
//----------------------------------------------------------------

class gpc_blowtorch : public GpConsole_command
{
public:
	gpc_blowtorch();

	UINT32			GetFlags()				{ return 0; }
	gpstring		GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int	GetMinArguments()		{ return 1; }
	unsigned int	GetMaxArguments()		{ return 10;}

	bool			Execute( GpConsole_command_arguments &args, gpstring &output );
};

gpc_blowtorch::gpc_blowtorch()
{
	gpstring					help;
	std::vector< gpstring >	usage;

	help = "\nsfx < command > [ effect ] [ parameters ] - issue Flamethrower command\n\n";
	help += "commands:\n";
	help += "   attach < parent_id > < child_id > - adds effect to another effect\n";
	help += "   create < name > - creates effect\n";
	help += "   rat < effect_id > - Removes All Targets\n";
	help += "   destroy < effect_id|all > - destroy effect\n";
	help += "   effects - lists available effects\n";
	help += "   scripts - lists available scripts\n";
	help += "   list [scripts] - displays active effects or scripts\n";
	help += "   start < effect_id|all > - runs effect\n";
	help += "   stop < effect_id|all|scripts|script < script_id > > - stops effect(s) or script(s)\n";
	help += "   target < effect_id > < target_id > - adds effect as a target\n";
	help += "   reload - reloads all effect scripts";
	help += "   run < script name > - runs an effect script locally\n";
	help += "   srun < script name > - runs an effect script globally\n";
	help += "   erun < script name > - runs an effect script on active equipped item locally\n";
	help += "   esrun < script name > - runs an effect script on active equipped item globally\n";
	help += "   unload < script id > - force unloads a script\n";
	help += "   load < script id > - force loads a previously unloaded script\n";
	help += "   lcf < scale factor > - amount to scale all lightsources culling radius by";
	usage.push_back( "sfx attach" );
	usage.push_back( "sfx create" );
	usage.push_back( "sfx create fire" );
	usage.push_back( "sfx create trackball" );
	usage.push_back( "sfx create lightning" );
	usage.push_back( "sfx create steam" );
	usage.push_back( "sfx create explosion" );
	usage.push_back( "sfx create spe" );
	usage.push_back( "sfx create orbiter" );
	usage.push_back( "sfx create polygonalexplosion" );
	usage.push_back( "sfx create lightsource" );
	usage.push_back( "sfx create sray" );
	usage.push_back( "sfx create sphere" );
	usage.push_back( "sfx create flurry" );
	usage.push_back( "sfx create charge" );
	usage.push_back( "sfx create sparkles" );
	usage.push_back( "sfx create decal" );
	usage.push_back( "sfx create discpile" );
	usage.push_back( "sfx create cylinder" );
	usage.push_back( "sfx create pointtracer" );
	usage.push_back( "sfx create linetracer" );
	usage.push_back( "sfx create curve" );
	usage.push_back( "sfx create spawn" );
	usage.push_back( "sfx rat" );
	usage.push_back( "sfx destroy" );
	usage.push_back( "sfx destroy all" );
	usage.push_back( "sfx effects" );
	usage.push_back( "sfx finish" );
	usage.push_back( "sfx finish effect" );
	usage.push_back( "sfx finish script" );
	usage.push_back( "sfx scripts" );
	usage.push_back( "sfx list" );
	usage.push_back( "sfx list scripts" );
	usage.push_back( "sfx start" );
	usage.push_back( "sfx start all" );
	usage.push_back( "sfx stop" );
	usage.push_back( "sfx stop script" );
	usage.push_back( "sfx stop all" );
	usage.push_back( "sfx stop scripts" );
	usage.push_back( "sfx target" );
	usage.push_back( "sfx load" );
	usage.push_back( "sfx unload" );
	usage.push_back( "sfx reload" );
	usage.push_back( "sfx run" );
	usage.push_back( "sfx srun" );
	usage.push_back( "sfx erun" );
	usage.push_back( "sfx esrun" );
	usage.push_back( "sfx lcf" );

	gGpConsole.RegisterCommand( *this, "sfx", help, usage );
}


bool
gpc_blowtorch::Execute( GpConsole_command_arguments &args, gpstring &output )
{
	gpstring	strEffect, strParams;
	unsigned int id;

	if( args.Size() < 2 ) {
		return false;
	}

	if( args.GetAsString(1).same_no_case( "create" ) )
	{
		strEffect = args.GetAsString( 2 );
		for( unsigned int i = 3; i < args.Size(); i++ )
		{
			// re-delimit
			strParams += args.GetAsString( i ) + ",";
		}

		if( !strParams.empty() && strParams[strParams.size()-1] == ',' )
		{
			strParams.at_split(strParams.size()-1) = 0;
		}

		GopColl mySelected = gGoDb.GetSelection();

		for( i = gGoDb.GetSelection().size(); i >0; --i )
		{
			def_tracker pTracker( new TattooTracker() );

			for( GopColl::iterator id = mySelected.begin(); id != mySelected.end(); id++ )
			{
				GoTarget	*Target = new GoTarget( (*id)->GetGoid() );
				pTracker->AddTarget( Target );
			}

			mySelected.erase( mySelected.begin() );

			gWorldFx.CreateEffect( strEffect, strParams, pTracker, GOID_INVALID, SFxSID_INVALID, RandomDword() );
		}
		return true;
	}
	else if( args.GetAsString(1).same_no_case( "attach" ) )
	{
		if( args.Size() == 4 )
		{
			int parent_id = args.GetAsInt( 2 );
			int	child_id = args.GetAsInt( 3 );

			gWorldFx.AttachEffect( (SFxEID)parent_id, (SFxEID)child_id );
			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "destroy" ) )
	{
		if( args.Size() == 3 )
		{
			if( args.GetAsString( 2 ).same_no_case( "all" ) )
			{
				gWorldFx.DestroyAllEffects();
				gWorldFx.StopAllScripts();
				return true;
			}
			else
			{
				id = args.GetAsInt( 2 );
				gWorldFx.DestroyEffect( (SFxEID)id );
				return true;
			}
		}
	}
	else if( args.GetAsString(1).same_no_case( "start" ) )
	{
		if( args.Size() == 3 )
		{
			if( args.GetAsString( 2 ).same_no_case( "all" ) )
			{
				gWorldFx.StartAllEffects();
				return true;
			}
			else
			{
				id = args.GetAsInt( 2 );
				gWorldFx.StartEffect( (SFxEID)id );
				return true;
			}
		}
	}
	else if( args.GetAsString(1).same_no_case( "stop" ) )
	{
		if( args.Size() == 3 )
		{
			if( args.GetAsString( 2 ).same_no_case( "all" ) )
			{
				gWorldFx.StopAllEffects();
				return true;
			}
			else if( args.GetAsString( 2 ).same_no_case( "scripts" ) )
			{
				gWorldFx.StopAllScripts();
				return true;
			}
			else
			{
				id = args.GetAsInt( 2 );
				gWorldFx.StopEffect( (SFxEID)id );
				return true;
			}
		}
		else if( args.Size() == 4 )
		{
			if( args.GetAsString( 2 ).same_no_case( "script" ) )
			{
				gWorldFx.StopScript( (SFxSID)args.GetAsInt( 3 ) );
				return true;
			}
		}
	}
	else if( args.GetAsString(1).same_no_case( "finish" ) )
	{
		if( args.Size() == 4 )
		{
			if( args.GetAsString( 2 ).same_no_case( "effect" ) )
			{
				gWorldFx.SetFinishing( (SFxEID)args.GetAsInt( 3 ) );
				return true;
			}
			else if( args.GetAsString( 2 ).same_no_case( "script" ) )
			{
				gWorldFx.FinishScript( (SFxSID)args.GetAsInt( 3 ) );
				return true;
			}
		}
	}
	else if( args.GetAsString(1).same_no_case( "list" ) )
	{
		if( args.Size() == 3 )
		{
			if( args.GetAsString( 2 ).same_no_case( "scripts" ) )
			{
				output = gWorldFx.GetActiveScripts();
			}
		}
		else
		{
			output = gWorldFx.GetActiveEffects();
		}
		return true;
	}
	else if( args.GetAsString(1).same_no_case( "scripts" ) )
	{
		output = gWorldFx.GetAvailableScripts();
		return true;
	}
	else if( args.GetAsString(1).same_no_case( "effects" ) )
	{
		output = gWorldFx.GetAvailableEffects();
		return true;
	}
	else if( args.GetAsString(1).same_no_case( "rat" ) )
	{
		if( args.Size() == 3 )
		{
			id = args.GetAsInt( 2 );
			gWorldFx.RemoveAllTargets( (SFxEID)id );
			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "target" ) )
	{
		if( args.Size() == 4 )
		{
			int effect_id = args.GetAsInt( 2 );
			int target_id = args.GetAsInt( 3 );

			gWorldFx.AddEffectTarget( (SFxEID)effect_id, (SFxEID)target_id );
			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "run" ) )
	{
		if( args.Size() >= 3 )
		{
			def_tracker tracker = WorldFx::MakeTracker();
			GopColl::const_iterator id = gGoDb.GetSelection().begin();

			Goid selected_goid = GOID_INVALID;

			if( gGoDb.GetSelection().size() != 1 )
			{
				for( ; id != gGoDb.GetSelection().end(); ++id )
				{
					tracker->AddGoTarget( (*id)->GetGoid() );
					selected_goid = (*id)->GetGoid();
				}
			}
			else
			{
				tracker->AddGoTarget( (*id)->GetGoid() );
				tracker->AddGoTarget( (*id)->GetGoid() );
			}

			gWorldFx.RunScript( args.GetAsString(2), tracker, args.GetAsString(3), selected_goid, WE_INVALID );

			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "srun" ) )
	{
		if( args.Size() >= 3 )
		{
			def_tracker tracker = WorldFx::MakeTracker();
			GopColl::const_iterator id = gGoDb.GetSelection().begin();

			Goid selected_goid = GOID_INVALID;

			if( gGoDb.GetSelection().size() != 1 )
			{
				for( ; id != gGoDb.GetSelection().end(); ++id )
				{
					tracker->AddGoTarget( (*id)->GetGoid() );
					selected_goid = (*id)->GetGoid();
				}
			}
			else
			{
				tracker->AddGoTarget( (*id)->GetGoid() );
				tracker->AddGoTarget( (*id)->GetGoid() );
			}

			gWorldFx.SRunScript( args.GetAsString(2), tracker, args.GetAsString(3), selected_goid, WE_INVALID );

			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "erun" ) )
	{
		if( args.Size() >= 3 )
		{
			def_tracker tracker = WorldFx::MakeTracker();
			GopColl::const_iterator id = gGoDb.GetSelection().begin();

			Goid selected_goid = GOID_INVALID;

			if( gGoDb.GetSelection().size() != 1 )
			{
				for( ; id != gGoDb.GetSelection().end(); ++id )
				{
					if( (*id)->HasInventory() )
					{
						Go *pGo = (*id)->GetInventory()->GetEquipped( ES_WEAPON_HAND );

						tracker->AddGoTarget( pGo->GetGoid() );
						selected_goid = (*id)->GetGoid();
					}
				}
			}
			else
			{
				if( (*id)->HasInventory() )
				{
					Go *pGo = (*id)->GetInventory()->GetEquipped( ES_WEAPON_HAND );
					tracker->AddGoTarget( pGo->GetGoid() );
					tracker->AddGoTarget( pGo->GetGoid() );
				}
			}

			gWorldFx.RunScript( args.GetAsString(2), tracker, args.GetAsString(3), selected_goid, WE_INVALID );

			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "esrun" ) )
	{
		if( args.Size() >= 3 )
		{
			def_tracker tracker = WorldFx::MakeTracker();
			GopColl::const_iterator id = gGoDb.GetSelection().begin();

			Goid selected_goid = GOID_INVALID;

			if( gGoDb.GetSelection().size() != 1 )
			{
				for( ; id != gGoDb.GetSelection().end(); ++id )
				{
					if( (*id)->HasInventory() )
					{
						Go *pGo = (*id)->GetInventory()->GetEquipped( ES_WEAPON_HAND );

						tracker->AddGoTarget( pGo->GetGoid() );
						selected_goid = (*id)->GetGoid();
					}
				}
			}
			else
			{
				if( (*id)->HasInventory() )
				{
					Go *pGo = (*id)->GetInventory()->GetEquipped( ES_WEAPON_HAND );
					tracker->AddGoTarget( pGo->GetGoid() );
					tracker->AddGoTarget( pGo->GetGoid() );
				}
			}

			gWorldFx.SRunScript( args.GetAsString(2), tracker, args.GetAsString(3), selected_goid, WE_INVALID );

			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "load" ) )
	{
		if( args.Size() == 3 )
		{
			id = args.GetAsInt( 2 );
			gWorldFx.LoadScript( (SFxSID)id );
		}
	}
	else if( args.GetAsString(1).same_no_case( "unload" ) )
	{
		if( args.Size() == 3 )
		{
			id = args.GetAsInt( 2 );
			gWorldFx.UnloadScript( (SFxSID)id );
		}
	}
	else if( args.GetAsString(1).same_no_case( "lcf" ) )
	{
		if( args.Size() == 3 )
		{
			gWorldFx.SetLightRadiusScaleFactor( args.GetAsFloat( 2 ) );
			return true;
		}
		else
		{
			char szOutput[256];
			sprintf( szOutput, "Lightsource culling factor is %f\n",
				gWorldFx.GetLightRadiusScaleFactor() );
			output = szOutput;
			return true;
		}
	}
	else if( args.GetAsString(1).same_no_case( "reload" ) )
	{
		gWorldFx.RebuildScriptVocabulary();
		return true;
	}
	return false;
}




//================================================================
//
// gpc_kill
//
// This is a GpConsole_command for killing objects
//
//----------------------------------------------------------------

class gpc_kill : public GpConsole_command
{
public:
	gpc_kill();
	~gpc_kill(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_kill::gpc_kill()
{
	gpstring help;
	std::vector< gpstring > usage;
	help =  "kill - kills selected objects.\n";
	usage.push_back( "kill" );

	// register the command
	gGpConsole.RegisterCommand( *this, "kill", help, usage );
};




bool gpc_kill::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if( args.Size() > 1 )
	{	//ERROR - wrong number of arguments
		output = "ERROR: <kill> does not take arguments.\n";
		return( false );
	}
	else
	{
		GopColl selection( gGoDb.GetSelection() );
		if ( !selection.empty() )
		{
			GopColl::const_iterator iSelected;
			for( iSelected = selection.begin(); iSelected != selection.end(); ++iSelected )
			{
				gRules.ChangeLife( (*iSelected)->GetGoid(), (*iSelected)->GetAspect()->GetMaxLife() * -2.0f, RPC_TO_ALL );
			}
		}
		else
		{	//ERROR - wrong number of arguments
			output = "ERROR: <kill> needs objects to be selected.\n";
			return( false );
		}
		gGoDb.DeselectAll();
	}
	return( true );
}




//================================================================
//
// gpc_teleport
//
// This is a GpConsole_command for moving objects or characters
//
//----------------------------------------------------------------

class gpc_teleport : public GpConsole_command
{
	SiegePos					m_Pos;
	CameraPosition				m_CameraPos;
	gpstring					m_MapName;
	gpstring					m_Teleport;

public:
	gpc_teleport();
	~gpc_teleport(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );

private:
	void ExecuteFadeCallback();

	void IntraMapTeleport();
	void InterMapTeleport();
};

gpc_teleport::gpc_teleport()
{
	gpstring help;
	std::vector< gpstring > usage;
	help =  "teleport - can move selected object or character.\n";
	help += "Only select one character for this operation.";


	// register the command
	gGpConsole.RegisterCommand( *this, "teleport", help, usage );
};


bool gpc_teleport::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	m_Pos = SiegePos::INVALID;
	m_MapName.clear();
	m_Teleport.clear();
	gpstring where = args.GetAsString( 1 );

	if ( (args.Size() == 1) || stringtool::HasDosWildcards( where ) )
	{
		gWorldMap.DumpBookmarks( where );
	}
	else if ( args.Size() == 2 )
	{
		if (   (gWorldState.GetCurrentState() != WS_SP_INGAME)
			&& (gWorldState.GetCurrentState() != WS_MP_INGAME)
			&& (gWorldState.GetCurrentState() != WS_MEGA_MAP ) )
		{
			output = "Teleport only supported in normal gameplay";
		}
		else if ( gGoDb.GetSelection().empty() )
		{
			output = "You must have at least one object or character selected!";
		}
		else
		{
			bool teleport = false;

			// somebody wanna switch maps?
			size_t found = where.find( ':' );
			if ( (found != 1) && (found != gpstring::npos) )
			{
				// definitely can't do this in MP
				if ( !::IsSinglePlayer() )
				{
					output = "Inter-map teleport only supported in single player";
				}
				else
				{
					// inter-map teleport
					m_MapName = where.left( found );
					m_Teleport = where.mid( found + 1 );

					// check it
					if ( gTankMgr.HasMap( m_MapName ) )
					{
						teleport = true;
					}
					else
					{
						output = "Invalid map name!";
					}
				}
			}
			else if ( gWorldMap.LoadBookmark( where, m_Pos, m_CameraPos ) )
			{
				// intra-map teleport
				teleport = true;
			}

			// do fadeout
			if ( teleport )
			{
				gTimeMgr.AddRapiFader( false, 1.0f );
				gTimeMgr.GetSysTimecaster().AddExecuteCallback( makeFunctor( *this, &gpc_teleport::ExecuteFadeCallback ), 1.2f );
			}
		}
	}

	return true;
}


void
gpc_teleport::ExecuteFadeCallback()
{
	// switch to reloading state temporarily
	eWorldState oldState = gWorldState.GetCurrentState();
	gWorldStateRequest( WS_RELOADING );
	gWorldState.Update();

	// do the teleport
	if ( m_MapName.empty() )
	{
		IntraMapTeleport();
	}
	else
	{
		InterMapTeleport();
	}

	// commit any pending stuff
	gGoDb.CommitAllRequests();

	// switch back to old state
	gWorldStateRequest( oldState );
	gWorldState.Update();

	// fade back in
	gTimeMgr.AddRapiFader( true, 1.0f );
	gTattooGame.SetRenderSkip( 2 );
}


void
gpc_teleport::IntraMapTeleport()
{
	if( IsServerLocal() && IsSinglePlayer() )
	{
		gTattooGame.TeleportPlayer( gServer.GetScreenPlayer()->GetId(), m_Pos, m_CameraPos );
	}
}

struct CreateEntry
{
	gpstring m_TemplateName;		// name of template for this go
	gpstring m_SubblockName;		// name of subblock used to load in go's saved info
	bool     m_WasHero;				// was this the hero? needed during reset
};

void
gpc_teleport::InterMapTeleport()
{
	// get a basis for xfer
	FuelHandle base( "::import:root" );
	base = base->CreateChildBlock( "temp", "temp" );

	// get a coll for clone sources of other selections
	typedef stdx::fast_vector <CreateEntry> CreateColl;
	CreateColl createColl;

	// xfer all our selected go's into there
	{
		FuBi::FuelWriter writer( base );
		FuBi::PersistContext persist( &writer, false );

		GopColl::iterator i, ibegin = gGoDb.GetSelectionBegin(), iend = gGoDb.GetSelectionEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpstring name;

			if ( (*i)->IsScreenPartyMember() )
			{
				name = ToString( i - ibegin );
				persist.EnterBlock( name );
				(*i)->XferCharacter( persist );
				persist.LeaveBlock();
			}

			CreateEntry& entry = *createColl.push_back();
			entry.m_TemplateName = (*i)->GetTemplateName();
			entry.m_SubblockName = name;
			entry.m_WasHero      = (*i)->IsHero();
		}
	}

	// kill everything
	gGoDb.Shutdown();
	gServer.Shutdown( SHUTDOWN_FOR_NEW_MAP );
	gUIPartyManager.DeconstructUIParty();
	gWorldMap.Shutdown();
	gMCP.Reset();

	// set new map
	gTankMgr.SetMap( m_MapName );
	gWorldMap.SSet( m_MapName, NULL, RPC_TO_SERVER );

	// choose the starting position
	if ( !m_Teleport.empty() )
	{
		SiegePos pos;
		CameraPosition cameraPos;
		if ( gWorldMap.LoadBookmark( m_Teleport, pos, cameraPos ) )
		{
			gServer.GetScreenPlayer()->SSetStartingPosition( pos, cameraPos );
		}
	}

	// rebuild our initial frustum
	gServer.GetScreenPlayer()->SCreateInitialFrustum();

	// teleport there
	CameraPosition cameraPos;
	cameraPos.cameraPos = gSiegeEngine.GetCamera().GetCameraSiegePos();
	cameraPos.targetPos = gSiegeEngine.GetCamera().GetTargetSiegePos();
	gUICamera.SetTeleportPosition( cameraPos );

	// load a new map
	gWorldTerrain.ForceWorldLoad();
	gGoDb.CommitAllRequests();

	// place party
	gServer.GetScreenPlayer()->SAddPlayerToWorld( false );
	
	// now rebuild our party
	GoidColl goids;
	{
		FuBi::FuelReader reader( base );
		FuBi::PersistContext persist( &reader, false );

		CreateColl::iterator i, ibegin = createColl.begin(), iend = createColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GoCloneReq cloneReq( i->m_TemplateName );

			// if party member, parent is the party
			if ( !i->m_SubblockName.empty() )
			{
				cloneReq.m_Parent = gServer.GetScreenParty()->GetGoid();
				cloneReq.m_AllClients = true;
			}

			// place in the starting position for now
			cloneReq.SetStartingPos( gServer.GetScreenPlayer()->GetStartingPosition() );

			// clone 'er
			Goid goid = gGoDb.SCloneGo( cloneReq );
			GoHandle go( goid );
			if ( go )
			{
				// keep it for placement adjustment
				goids.push_back( goid );

				// xfer in if necessary
				if ( !i->m_SubblockName.empty() )
				{
					persist.EnterBlock( i->m_SubblockName );
					go->XferCharacter( persist );
					persist.LeaveBlock();
				}

				// hero?
				if ( i->m_WasHero )
				{
					go->GetPlayer()->SetHero( go->GetGoid() );
				}
			}
			else
			{
				gperrorf(( "Unable to restore Go with template '%s', sorry\n", i->m_TemplateName.c_str() ));
			}
		}
	}

	// give them the gold
	gServer.GetScreenPlayer()->SCreateTradeGold();

	// place everything
	PContentDb::PlaceObjectsInArray( gServer.GetScreenPlayer()->GetStartingPosition(), goids, 1.5f );

	// tell ui we're ready
	gUIPartyManager.ConstructUIParty();

	// tell MCP we're ready
	GoidColl::const_iterator i, ibegin = goids.begin(), iend = goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		GoHandle go( *i );
		if (go->HasFollower())
		{
			go->GetPlacement()->SetOrientation( Quat::IDENTITY );
			go->GetFollower()->ForceUpdatePositionOrientation();
		}
	}
	gMCP.Update();

	// teleport camera there
	gUICamera.Teleport();
	gUICamera.SetCameraMode( CMODE_TRACK );

	// done with this fuel
	base.Delete();

	// commit any outstanding requests to prevent white texture syndrome
	gSiegeLoadMgr.CommitAll();
}



//================================================================
//
// gpc_sound
//
//----------------------------------------------------------------

class gpc_sound : public GpConsole_command
{
public:
	gpc_sound();
	~gpc_sound(){}
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 2 ); }
	unsigned int GetMaxArguments()		{ return( 3 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_sound::gpc_sound()
{
	gpstring help;
	std::vector< gpstring > usage;
	help  =  "Sound Help -\n=======================================================\n";
	help +=  "enable				- Enables the sound system\n";
	help +=  "disable				- Disables the sound system\n";
	help +=  "lplay <filename>		- Loops the playback of a sample\n";
	help +=  "play <filename>		- Plays a sample one time\n";
	help +=  "lstream <filename>	- Loops the playback of a stream\n";
	help +=  "stream <filename>		- Plays a sample as a stream\n";
	help +=  "stopsample <SOUNDID>  - Stops a sample from playing\n";
	help +=  "stopstream <SOUNDID>  - Stops a stream from playing\n";
	help +=  "history               - Dumps history of sound usage to the console\n";
	usage.push_back( "sound enable" );
	usage.push_back( "sound disable" );
	usage.push_back( "sound lplay" );
	usage.push_back( "sound play" );
	usage.push_back( "sound lstream" );
	usage.push_back( "sound stream" );
	usage.push_back( "sound stopsample" );
	usage.push_back( "sound stopstream" );
	usage.push_back( "sound history" );

	// register the command
	gGpConsole.RegisterCommand( *this, "sound", help, usage );
}

bool gpc_sound::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	gpstring sOption( args.GetAsString( 1 ) );

	if( sOption.same_no_case( "enable" ) )
	{
		if( !gSoundManager.IsEnabled() )
		{
			gWorldSound.Enable( true, false );
			gWorldOptions.SetSound( true );
		}
	}
	else if( sOption.same_no_case( "disable" ) )
	{
		if( gSoundManager.IsEnabled() )
		{
			gWorldSound.Enable( false, false );
			gWorldOptions.SetSound( false );
			gWorldSound.Clear();
		}
	}
	else if( sOption.same_no_case( "lplay" ) )
	{
		DWORD id = gSoundManager.PlaySample( args.GetAsString( 2 ), true );
		output.assignf( "Playing sample looped with id %d", id );
	}
	else if( sOption.same_no_case( "play" ) )
	{
		DWORD id = gSoundManager.PlaySample( args.GetAsString( 2 ), false );
		output.assignf( "Playing sample with id %d", id );
	}
	else if( sOption.same_no_case( "lstream" ) )
	{
		DWORD id = gSoundManager.PlayStream( args.GetAsString( 2 ), GPGSound::STRM_AMBIENT, true );
		output.assignf( "Playing stream looped with id %d", id );
	}
	else if( sOption.same_no_case( "stream" ) )
	{
		DWORD id = gSoundManager.PlayStream( args.GetAsString( 2 ), GPGSound::STRM_AMBIENT, false );
		output.assignf( "Playing stream with id %d", id );
	}
	else if( sOption.same_no_case( "stopsample" ) )
	{
		gSoundManager.StopSample( (DWORD)args.GetAsInt( 2 ) );
	}
	else if( sOption.same_no_case( "stopstream" ) )
	{
		gSoundManager.StopStream( (DWORD)args.GetAsInt( 2 ) );
	}
	else if( sOption.same_no_case( "history" ) )
	{
		const GPGSound::SampleFileNameMap& historyMap	= gSoundManager.GetSampleHistoryMap();
		for( GPGSound::SampleFileNameMap::const_iterator i = historyMap.begin(); i != historyMap.end(); ++i )
		{
			gpstring sampleName	= (*i).first;
			stringtool::PadBackToLength( sampleName, ' ', 50 );
			output.appendf( "%s - %d\n", sampleName.c_str(), (*i).second );
		}
	}
	else
	{
		return false;
	}

	return true;
}


//================================================================
//
// gpc_sim
//
//----------------------------------------------------------------

class gpc_sim : public GpConsole_command
{
public:
	gpc_sim();
	~gpc_sim(){}
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 7 ); }

	bool	bDepthWasSet;

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_sim::gpc_sim()
{
	gpstring help;
	std::vector< gpstring > usage;
	help =	"sim - debugging commands for physics stuff";
	help+=  "\ninfo    - information about current simulations";
	help+=  "\nstart   - starts simulation of selected object(s)";
	help+=	"\nstop    - stops simulating all currently simulated objects";
	help+=  "\nx       - Offset x position";
	help+=  "\ny       - Offset y position";
	help+=  "\nz       - Offset z position";
	help+=  "\nexplode - explode selected game object(s) with specified magnitude";
	help+=  "\nburn    - causes a go to catch on fire\n";
	help+=  "\nsdvolume <start_radius> <end_radius> <damage> <duration> - creates a damage volume using two selected gos static position\n";
	help+=  "\nddvolume <start_radius> <end_radius> <damage> <duration> <length> - creates a damage volume using two selected gos dynamic position\n";
	help+=  "\ndebug   - enable debug help";
	usage.push_back( "sim info" );
	usage.push_back( "sim start" );
	usage.push_back( "sim stop" );
	usage.push_back( "sim x" );
	usage.push_back( "sim y" );
	usage.push_back( "sim z" );
	usage.push_back( "sim explode" );
	usage.push_back( "sim burn" );
	usage.push_back( "sim sdvolume" );
	usage.push_back( "sim ddvolume" );
	usage.push_back( "sim debug" );

	bDepthWasSet = false;

	// register the command
	gGpConsole.RegisterCommand( *this, "sim", help, usage );
}

bool gpc_sim::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	gpstring sOption( args.GetAsString( 1 ) );

	if( args.Size() == 2 ) {
		if( sOption.same_no_case( "start" ) )
		{
			GopColl Selection = gGoDb.GetSelection();
			for( GopColl::iterator id = Selection.begin(); id != Selection.end(); ++id ) {
				WorldMessage	message( WE_START_SIMULATING, GOID_INVALID, (*id)->GetGoid(), MakeInt( (*id)->GetGoid() ) );
				message.Send();
			}
		}
		else if( sOption.same_no_case( "stop" ) ) {
			gSim.StopAllSimulations();
		}
		else if( sOption.same_no_case( "info" ) ) {
			output = gSim.GetSimInfo().c_str();
		}
		else if( sOption.same_no_case( "debug" ) )
		{
			if( gSim.GetDebugState() )
			{
				gWorldOptions.SetDebugHudOptions( DHO_DEPTH, bDepthWasSet );

				gSim.SetDebugState( false );
				output = "Sim debug help OFF";
			}
			else
			{
				bDepthWasSet = gWorldOptions.TestDebugHudOptions( DHO_DEPTH );
				gWorldOptions.SetDebugHudOptions( DHO_DEPTH, true );
				gSim.SetDebugState( true );
				output = "Sim debug help ON";
			}
		}
	}
	if( sOption.same_no_case( "explode" ) ) {

		GopColl mySelected = gGoDb.GetSelection();
		for( GopColl::iterator id = mySelected.begin(); id != mySelected.end(); id++ ) {
			SimID source_object = 0;
			gSim.TriggerDependents( (*id)->GetGoid(), source_object );
			gSim.ExplodeGoWithDamage( (*id)->GetGoid(), (*id)->GetGoid(), (*id)->GetGoid() );
			output = "boom!";
		}
	}
	else if( sOption.same_no_case( "burn" ) ) {

		GopColl mySelected = gGoDb.GetSelection();
		for( GopColl::iterator id = mySelected.begin(); id != mySelected.end(); id++ ) {
			GoPhysics *pPhysics = (*id)->GetPhysics();
			if( pPhysics != NULL ) {
				pPhysics->SetIsOnFire( true );
			}
		}
		output = "burning...";
	}
	else if( args.Size() == 3 )
	{
		float		Displace( args.GetAsFloat( 2 ) );
		GopColl Selection = gGoDb.GetSelection();

		if( sOption.same_no_case( "x" ) ) {
			for( GopColl::iterator id = Selection.begin(); id != Selection.end(); ++id ) {
				GoHandle hSelectedGO( (*id)->GetGoid() );
				if( hSelectedGO.IsValid() ) {
					SiegePos GOPos( hSelectedGO->GetPlacement()->GetPosition() );
					GOPos.pos.x += Displace;
					hSelectedGO->GetPlacement()->SetPosition( GOPos, false );
				}
			}
		}
		else if( sOption.same_no_case( "y" ) ) {
			for( GopColl::iterator id = Selection.begin(); id != Selection.end(); ++id ) {
				GoHandle hSelectedGO( (*id)->GetGoid() );
				if( hSelectedGO.IsValid() ) {
					SiegePos GOPos( hSelectedGO->GetPlacement()->GetPosition() );
					GOPos.pos.y += Displace;
					hSelectedGO->GetPlacement()->SetPosition( GOPos, false );
				}
			}
		}
		else if( sOption.same_no_case( "z" ) ) {
			for( GopColl::iterator id = Selection.begin(); id != Selection.end(); ++id ) {
				GoHandle hSelectedGO( (*id)->GetGoid() );
				if( hSelectedGO.IsValid() ) {
					SiegePos GOPos( hSelectedGO->GetPlacement()->GetPosition() );
					GOPos.pos.z += Displace;
					hSelectedGO->GetPlacement()->SetPosition( GOPos, false );
				}
			}
		}
	}
	else if( args.Size() >= 6 )
	{
		if( sOption.same_no_case( "sdvolume" ) )
		{
			const float start_radius	= args.GetAsFloat( 2 );
			const float end_radius		= args.GetAsFloat( 3 );
			const float damage_amount	= args.GetAsFloat( 4 );
			const float duration		= args.GetAsFloat( 5 );

			SiegePos start_pos, end_pos;

			GopColl mySelected = gGoDb.GetSelection();

			if( mySelected.size() >= 2 )
			{
				GopColl::iterator iGo = mySelected.begin();

				start_pos = (*iGo)->GetPlacement()->GetPosition();
				++iGo;
				end_pos = (*iGo)->GetPlacement()->GetPosition();
			}
			gSim.CreateDamageVolume( start_pos, end_pos, start_radius, end_radius, damage_amount, duration, GOID_INVALID, GOID_INVALID );
		}
		else if( sOption.same_no_case( "ddvolume" ) )
		{
			const float start_radius	= args.GetAsFloat( 2 );
			const float end_radius		= args.GetAsFloat( 3 );
			const float damage_amount	= args.GetAsFloat( 4 );
			const float duration		= args.GetAsFloat( 5 );

			float length = 0;
			if( args.Size() == 7 )
			{
				length = args.GetAsFloat( 6 );
			}

			Goid start_goid = GOID_INVALID;
			Goid end_goid	= GOID_INVALID;

			SiegePos start_pos, end_pos;

			GopColl mySelected = gGoDb.GetSelection();

			if( mySelected.size() >= 2 )
			{
				GopColl::iterator iGo = mySelected.begin();

				start_goid = (*iGo)->GetGoid();
				++iGo;
				end_goid = (*iGo)->GetGoid();
			}
			gSim.CreateDamageVolume( start_goid, end_goid, length, start_radius, end_radius, damage_amount, duration, GOID_INVALID, GOID_INVALID );
		}
	}

	return true;
}




//================================================================
//
// gpc_siegever
//
// This is a GpConsole_command for printing out version info.
//
//----------------------------------------------------------------

class gpc_siegever : public GpConsole_command
{
public:
	gpc_siegever();
	~gpc_siegever(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_siegever::gpc_siegever()
{
	gpstring help;
	std::vector< gpstring > usage;
	help = "siegever - print out game version info.\n";

	// register the command
	gGpConsole.RegisterCommand( *this, "siegever", help, usage );
}

bool gpc_siegever::Execute( GpConsole_command_arguments & /*args*/, gpstring & output )
{
	output += SysInfo::MakeExeSessionInfo();
	return ( true );
}



//================================================================
//
// gpc_stats
//
// This is a GpConsole_command for viewing and changing stats
//
//----------------------------------------------------------------

class gpc_stats : public GpConsole_command
{
public:
	gpc_stats();
	~gpc_stats(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 4 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_stats::gpc_stats()
{
	gpstring help;
	std::vector< gpstring > usage;
	help = "stats - view or change stats for the selected character\n";
	help += "(Optional: use name of stat followed by number to change it)\n";
	help += "uber            - uber level\n";
	help += "int             - intelligence\n";
	help += "str             - strength\n";
	help += "dex             - dexterity\n";
	help += "melee           - melee\n";
	help += "ranged          - ranged\n";
	help += "nmagic          - nature magic\n";
	help += "cmagic          - combat magic\n";
	help += "xp_uber         - uber level\n";
	help += "xp_int          - intelligence\n";
	help += "xp_str	         - strength\n";
	help += "xp_dex	         - dexterity\n";
	help += "xp_melee        - melee\n";
	help += "xp_ranged       - ranged\n";
	help += "xp_nmagic       - nature magic\n";
	help += "xp_cmagic       - combat magic\n";
	help += "xp <skill name> - set xp value by xp name\n";
	help += "life            - life\n";
	help += "mana            - mana\n";
	help += "lru             - life recovery unit\n";
	help += "lrp             - life recovery period\n";
	help += "mru             - mana recovery unit\n";
	help += "mrp             - mana recovery period\n";
	help += "sb              - anim speed bias\n";
	help += "sr              - sight range\n";
	help += "sa              - sight fov\n";
	help += "min             - min stats\n";
	help += "max             - max stats\n";
	help += "inv             - toggle invincibility ON/OFF\n";
	help += "live            - change lifestate to alive\n";
	help += "levels          - displays all attained levels\n";
	help += "easy_diff       - sets the difficulty level to easy\n";
	help += "medium_diff     - sets the difficulty level to medium\n";
	help += "hard_diff       - sets the difficulty level to hard\n";
	help += "pdc             - enables/disables particle damage counter display\n";
	help += "rpdc            - resets the particle damage counter\n";
	help += "tpdc <seconds>  - sets the particle damage counters reset period time\n";

	usage.push_back( "stats uber" );
	usage.push_back( "stats xp_uber" );
	usage.push_back( "stats int" );
	usage.push_back( "stats xp_int" );
	usage.push_back( "stats str" );
	usage.push_back( "stats xp_str" );
	usage.push_back( "stats dex" );
	usage.push_back( "stats xp_dex" );
	usage.push_back( "stats melee" );
	usage.push_back( "stats xp_melee" );
	usage.push_back( "stats ranged" );
	usage.push_back( "stats xp_ranged" );
	usage.push_back( "stats nmagic" );
	usage.push_back( "stats xp_nmagic" );
	usage.push_back( "stats cmagic" );
	usage.push_back( "stats xp_cmagic" );
	usage.push_back( "stats xp" );
	usage.push_back( "stats life" );
	usage.push_back( "stats mana" );
	usage.push_back( "stats lru" );
	usage.push_back( "stats lrp" );
	usage.push_back( "stats mru" );
	usage.push_back( "stats mrp" );
	usage.push_back( "stats sb" );
	usage.push_back( "stats sr" );
	usage.push_back( "stats sa" );
	usage.push_back( "stats min" );
	usage.push_back( "stats med" );
	usage.push_back( "stats max" );
	usage.push_back( "stats inv" );
	usage.push_back( "stats live" );
	usage.push_back( "stats levels" );
	usage.push_back( "stats easy_diff" );
	usage.push_back( "stats medium_diff" );
	usage.push_back( "stats hard_diff" );

	usage.push_back( "stats pdc" );
	usage.push_back( "stats rpdc" );
	usage.push_back( "stats tpdc" );


	// register the command
	gGpConsole.RegisterCommand( *this, "stats", help, usage );
}



bool gpc_stats::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if( gGoDb.GetSelection().size() != 1 )
	{
		output += "You need to select a single character.\n";
		return true;
	}

	const Goid selected_id = (*(gGoDb.GetSelection().begin()))->GetGoid();
	GoHandle	hSelected( selected_id );

	if( args.Size() == 1 )
	{
		ReportSys::LocalContext localCont;
		ReportSys::StringSink tempSink;
		localCont.AddSink( &tempSink, false );

		gAIQuery.DumpActorStats( hSelected, &localCont );

		output.append( tempSink.GetBuffer() );
	}
	else if( args.Size() == 2 )
	{
		if( args.GetAsString( 1 ).same_no_case( "pdc" ) )
		{
			if( gRules.GetPDCEnable() )
			{
				gRules.SetPDCEnable( false );
				output += "Particle Damage Counter DISABLED\n";
			}
			else
			{
				gRules.SetPDCEnable( true );
				output += "Particle Damage Counter ENABLED\n";
			}
			return true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "rpdc" ) )
		{
			gRules.PDCReset();
			output += "Particle Damage Counter RESET\n";
			return true;
		}
		else if( args.GetAsString( 1 ).same_no_case( "min" ) )
		{
			GoActor *pActor = hSelected->GetActor();
			GoAspect *pAspect = hSelected->GetAspect();

			if( pActor != NULL ) {
				Skill *pSkill = NULL;

				if( pActor->GetSkill( "uber", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "strength", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "intelligence", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "dexterity", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "melee", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "ranged", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "nature magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				if( pActor->GetSkill( "combat magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0 );
				}
				pAspect->SetNaturalManaRecoveryUnit( 1 );
				pAspect->SetNaturalManaRecoveryPeriod( 10 );

				hSelected->GetAspect()->SetNaturalLifeRecoveryUnit( 1 );
				hSelected->GetAspect()->SetNaturalLifeRecoveryPeriod( 10 );

				hSelected->SetModifiersDirty();
			}
		}
		else if( args.GetAsString( 1 ).same_no_case( "med" ) )
		{
			GoActor *pActor = hSelected->GetActor();
			GoAspect *pAspect = hSelected->GetAspect();

			if( pActor != NULL ) {
				Skill *pSkill = NULL;
		
				const float max_level = 0.5f * gRules.GetMaxLevel();

				if( pActor->GetSkill( "uber", &pSkill ) ) {
					pSkill->SetNaturalLevel( 0.5f * gRules.GetMaxLevelUber() );
				}
				if( pActor->GetSkill( "strength", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "intelligence", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "dexterity", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "melee", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "ranged", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "nature magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "combat magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				pAspect->SetNaturalManaRecoveryUnit( max_level );
				pAspect->SetNaturalManaRecoveryPeriod( 1 );

				hSelected->GetAspect()->SetNaturalLifeRecoveryUnit( max_level );
				hSelected->GetAspect()->SetNaturalLifeRecoveryPeriod( 1 );

				hSelected->SetModifiersDirty();
			}
		}
		else if( args.GetAsString( 1 ).same_no_case( "max" ) )
		{
			GoActor *pActor = hSelected->GetActor();
			GoAspect *pAspect = hSelected->GetAspect();

			if( pActor != NULL ) {
				Skill *pSkill = NULL;
		
				const float max_level = gRules.GetMaxLevel();

				if( pActor->GetSkill( "uber", &pSkill ) ) {
					pSkill->SetNaturalLevel( gRules.GetMaxLevelUber() );
				}
				if( pActor->GetSkill( "strength", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "intelligence", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "dexterity", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level - pSkill->GetLevelBias() );
				}
				if( pActor->GetSkill( "melee", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "ranged", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "nature magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				if( pActor->GetSkill( "combat magic", &pSkill ) ) {
					pSkill->SetNaturalLevel( max_level );
				}
				pAspect->SetNaturalManaRecoveryUnit( max_level );
				pAspect->SetNaturalManaRecoveryPeriod( 1 );

				hSelected->GetAspect()->SetNaturalLifeRecoveryUnit( max_level );
				hSelected->GetAspect()->SetNaturalLifeRecoveryPeriod( 1 );

				hSelected->SetModifiersDirty();
			}
		}
		else if( args.GetAsString( 1 ).same_no_case( "inv" ) )
		{
			GoAspect *pAspect = hSelected->GetAspect();
			if( pAspect->GetIsInvincible() )
			{
				output = "Invincibility OFF\n";
				pAspect->SetIsInvincible( false );
			}
			else
			{
				output = "Invincibility ON\n";
				pAspect->SetIsInvincible( true );
			}
		}
		else if( args.GetAsString( 1 ).same_no_case( "live" ) )
		{
			GoAspect *pAspect = hSelected->QueryAspect();
			if ( pAspect != NULL )
			{
				pAspect->SSetCurrentLife( pAspect->GetMaxLife() );
				pAspect->SSetLifeState( LS_ALIVE_CONSCIOUS );
			}

			GoActor *pActor = hSelected->QueryActor();
			if ( pActor != NULL )
			{
				pActor->RSResetUnconsciousDuration( 0 );
			}
		}
		else if( args.GetAsString( 1 ).same_no_case( "levels" ) )
		{
			const SkillColl &skills = hSelected->GetActor()->GetSkills();

			output.append( 91, '-' ); output.append( "\n" );
			gpstring sSkillLevels( "= Skill Levels =" );
			output.append( 45 - sSkillLevels.size()/2, ' ' );
			output.append( sSkillLevels ); output.append( "\n" );
			output.append( 91, '-' ); output.append( "\n" );
			SkillColl::const_iterator iSkill = skills.begin(), iEnd = skills.end();
			for(; iSkill != iEnd; ++iSkill ) {

				gpstring sInfo( (*iSkill).GetName() );

				sInfo.append( max_t(0,int(24-sInfo.size())), ' ' );
				sInfo.append( "Level: " );
				stringtool::Append( (*iSkill).GetLevel(), sInfo );

				sInfo.append( max_t(0,int(40-sInfo.size())), ' ' );
				sInfo.append( "XP Level: " );
				stringtool::Append( (*iSkill).GetLevel() - (*iSkill).GetLevelBias(), sInfo );

				sInfo.append( max_t(0,int(59-sInfo.size())), ' ' );
				sInfo.append( "XP: " );
				stringtool::Append( (*iSkill).GetExperience(), sInfo );

				sInfo.append( max_t(0,int(76-sInfo.size())), ' ' );
				sInfo.append( "Base Level: " );
				stringtool::Append( (*iSkill).GetLevelBias(), sInfo );
				sInfo.append( "\n" );

				output.append( sInfo );
			}
			output.append( 91, '-' ); output.append( "\n" );
		}
		else if( args.GetAsString( 1 ).same_no_case( "easy_diff" ) )
		{
			gWorldOptions.SetDifficulty( DIFFICULTY_EASY );
		}
		else if( args.GetAsString( 1 ).same_no_case( "medium_diff" ) )
		{
			gWorldOptions.SetDifficulty( DIFFICULTY_MEDIUM );
		}
		else if( args.GetAsString( 1 ).same_no_case( "hard_diff" ) )
		{
			gWorldOptions.SetDifficulty( DIFFICULTY_HARD );
		}
	}
	else if( (args.Size() == 3) && args.GetAsString( 1 ).same_no_case( "tpdc" ) )
	{
		const float reset_period = args.GetAsFloat( 2 );
		gRules.SetPDCResetPeriod( reset_period );
		output.appendf( "PDC will reset every %g seconds.\n", reset_period );
		return true;
	}
	else if( (args.Size() == 3) && hSelected->HasActor() )
	{
		gpstring sAttribute	= args.GetAsString( 1 );
		const float value	= args.GetAsFloat( 2 );

		GoActor *pActor  = hSelected->GetActor();
		GoAspect*pAspect = hSelected->GetAspect();
		Skill		 *pSkill  = NULL;

		// Check for level setting
		if( sAttribute.same_no_case( "uber" ) ) {
			if( pActor->GetSkill( "uber", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "int" ) ) {
			if( pActor->GetSkill( "intelligence", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "str" ) ) {
			if( pActor->GetSkill( "strength", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "dex" ) ) {
			if( pActor->GetSkill( "dexterity", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "melee" ) ) {
			if( pActor->GetSkill( "melee", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "ranged" ) ) {
			if( pActor->GetSkill( "ranged", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "nmagic" ) ) {
			if( pActor->GetSkill( "nature magic", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "cmagic" ) ) {
			if( pActor->GetSkill( "combat magic", &pSkill ) ) {
				pSkill->SetNaturalLevel( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		// Check for experience addding
		else if( sAttribute.same_no_case( "xp_uber" ) ) {
			const char szSkill[]= "uber";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_int" ) ) {
			const char szSkill[]= "intelligence";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_str" ) ) {
			const char szSkill[]= "strength";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_dex" ) ) {
			const char szSkill[]= "dexterity";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_melee" ) ) {
			const char szSkill[]= "melee";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_ranged" ) ) {
			const char szSkill[]= "ranged";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_nmagic" ) ) {
			const char szSkill[]= "nature magic";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "xp_cmagic" ) ) {
			const char szSkill[]= "combat magic";
			if( pActor->GetSkill( szSkill, &pSkill ) ) {
				pSkill->AwardExperience( value );
			} else {
				gpwarning("The actor does not have that skill\n" );
			}
		}
		else if( sAttribute.same_no_case( "life" ) ) {
			pAspect->SSetCurrentLife( value );
		}
		else if( sAttribute.same_no_case( "mana" ) ) {
			pAspect->SSetCurrentMana( value );
		}
		else if( sAttribute.same_no_case( "lru" ) ) {
			pAspect->SetNaturalLifeRecoveryUnit( value );
		}
		else if( sAttribute.same_no_case( "lrp" ) ) {
			pAspect->SetNaturalLifeRecoveryPeriod( value );
		}
		else if( sAttribute.same_no_case( "mru" ) ) {
			pAspect->SetNaturalManaRecoveryUnit( value );
		}
		else if( sAttribute.same_no_case( "mrp" ) ) {
			pAspect->SetNaturalManaRecoveryPeriod( value );
		}
		else if( sAttribute.same_no_case( "sb" ) ) {
			// $$ find out what should be set here - Rick
		}
		else if( sAttribute.same_no_case( "sr" ) ) {
			GoMind *pMind = hSelected->GetMind();
			if( pMind != NULL )
			{
				gpassertm( 0, "Not supported" );
				//pMind->SetSightRange( value );
			}
		}
		else if( sAttribute.same_no_case( "sa" ) )
		{
			GoMind *pMind = hSelected->GetMind();
			if( pMind != NULL )
			{
				gpassertm( 0, "Not supported" );
				//pMind->SetSightFov( value );
			}
		}
		else
		{
			output = "Argument '";
			output += sAttribute;
			output += "' is not valid\n";
			return ( true );
		}

		output = "Attribute was changed to new value.\n";
	}
	else if( (args.Size() == 4) && hSelected->HasActor() )
	{
		gpstring sOption	= args.GetAsString( 1 );
		gpstring sSkillName	= args.GetAsString( 2 );
		const float value	= args.GetAsFloat( 3 );

		GoActor *pActor  = hSelected->GetActor();
		Skill		 *pSkill  = NULL;

		if( pActor->GetSkill( sSkillName, &pSkill ) )
		{
			pSkill->AwardExperience( value );
		}
		else
		{
			output += "Unknown skill\n";
		}
	}
	else
	{
		output += "Too many args!\n";
		output += "(Optional: use name of sAttribute followed by number to change value)\n";
	}

	hSelected->CheckModifierRecalc();

	return (true);
}




//================================================================
//
// gpc_weather
//
// This is a GpConsole_command for altering weather
//
//----------------------------------------------------------------

class gpc_weather : public GpConsole_command
{
public:
	gpc_weather();
	~gpc_weather(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 9 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_weather::gpc_weather()
{
	gpstring help;
	std::vector< gpstring > usage;
	help  = "weather <fog|rain|snow|wind> <effect options> [values]\n\n";
	help += " fog options:  info, on, off, color, neardist, fardist\n";
	help += " rain options: info, on, off, density\n";
	help += " snow options: info, on, off, density\n";
	help += " wind options: info, on, off, velocity\n";
	help += "  Descriptions:\n";
	help += "    rain, snow, fog, wind:      [on|off] Turns weather effect on or off\n";
	help += "    rain, snow, fog, wind:      [info]   Displays current settings\n";
	help += "    fog:             color      [r g b]  Set fog color components 0.0-1.0\n";
	help += "                     neardist   [f]      Fog near distance\n";
	help += "                     fardist    [f]      Fog far distance\n";
	help += "    rain, snow:      density    [f]      Set effect density\n";
	help += "    wind:            velocity   [f]      Set wind strength maximums\n";
	help += "                     direction  [f]      Set the wind direction angle\n";

	usage.push_back( "weather fog" );
	usage.push_back( "weather fog info" );
	usage.push_back( "weather fog on" );
	usage.push_back( "weather fog off" );
	usage.push_back( "weather fog color" );
	usage.push_back( "weather fog neardist" );
	usage.push_back( "weather fog fardist" );
	usage.push_back( "weather rain" );
	usage.push_back( "weather rain info" );
	usage.push_back( "weather rain on" );
	usage.push_back( "weather rain off" );
	usage.push_back( "weather rain density" );
	usage.push_back( "weather wind" );
	usage.push_back( "weather wind info" );
	usage.push_back( "weather wind on" );
	usage.push_back( "weather wind off" );
	usage.push_back( "weather wind velocity" );
	usage.push_back( "weather snow" );
	usage.push_back( "weather snow on" );
	usage.push_back( "weather snow info" );
	usage.push_back( "weather snow off" );
	usage.push_back( "weather snow density" );

	// register the command
	gGpConsole.RegisterCommand( *this, "weather", help, usage );
}


bool gpc_weather::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if( args.Size() < 2 ) {
		return false;
	}

	gpstring sType = args.GetAsString( 1 );
	gpstring sCmd = args.GetAsString( 2 );
	char szSettings[512];

	ZeroMemory( szSettings, sizeof(szSettings) );

	if( sType.same_no_case( "fog" ) )
	{
		if( sCmd.same_no_case( "info" ) )
		{
			if( gDefaultRapi.GetFogActivatedState() )
			{
				output = "Fog is ON:\n";
			}
			else
			{
				output = "Fog is OFF:\n";
			}

			output += "color = ";

			sprintf( szSettings, "%8x\n", gDefaultRapi.GetFogColor() );
			output += szSettings;
			sprintf( szSettings, "neardist = %g\nfardist = %g\n",
								gDefaultRapi.GetFogNearDist(),
								gDefaultRapi.GetFogFarDist() );

			output += szSettings;
		}
		else if( sCmd.same_no_case( "on" ) )
		{
			gDefaultRapi.SetFogActivatedState( true );
		}
		else if( sCmd.same_no_case( "off" ) )
		{
			gDefaultRapi.SetFogActivatedState( false );
		}
		else if( (sCmd.same_no_case( "color" )  && (args.Size() == 6) ) )
		{
			DWORD color	= MAKEDWORDCOLOR( vector_3(args.GetAsFloat(3),args.GetAsFloat(4),args.GetAsFloat(5)) );
			gDefaultRapi.SetFogColor( color );
			gDefaultRapi.SetClearColor( color );
		}
		else if( (sCmd.same_no_case( "neardist" )  && (args.Size() == 4) ) )
		{
			gDefaultRapi.SetFogNearDist( args.GetAsFloat( 3 ) );
		}
		else if( (sCmd.same_no_case( "fardist" )  && (args.Size() == 4) ) )
		{
			gDefaultRapi.SetFogFarDist( args.GetAsFloat( 3 ) );
		}
	}
	else if( sType.same_no_case( "rain" ) )
	{
		if( sCmd.same_no_case( "info" ) )
		{
			if( gWeather.GetRain()->GetRaining() )
			{
				output = "Rain is ON:\n";
			}
			else
			{
				output = "Rain is OFF:\n";
			}
			sprintf( szSettings, "density = %g", gWeather.GetRain()->GetDensity() );
			output += szSettings;
		}
		else if( sCmd.same_no_case( "on" ) )
		{
			gWeather.GetRain()->SetRaining( true );
		}
		else if( sCmd.same_no_case( "off" ) )
		{
			gWeather.GetRain()->SetRaining( false );
		}
		else if( (sCmd.same_no_case( "density" )  && (args.Size() == 4) ) )
		{
			gWeather.GetRain()->SetDensity( args.GetAsFloat( 3 ));
		}
	}
	else if( sType.same_no_case( "wind" ) )
	{
		if( sCmd.same_no_case( "info" ) )
		{
			if( gWeather.GetWind()->GetBlowing() )
			{
				output = "Wind is ON:\n";
			}
			else
			{
				output = "Wind is OFF:\n";
			}

			sprintf( szSettings, "velocity = %f\ndirection = %f\n",
								gWeather.GetWind()->GetVelocity(),
								gWeather.GetWind()->GetAngularDirection() );
			output += szSettings;
		}
		else if( sCmd.same_no_case( "on" ) )
		{
			gWeather.GetWind()->SetBlowing( true );
		}
		else if( sCmd.same_no_case( "off" ) )
		{
			gWeather.GetWind()->SetBlowing( false );
		}
		else if( (sCmd.same_no_case( "velocity" )  && (args.Size() == 4) ) )
		{
			gWeather.GetWind()->SetVelocity( args.GetAsFloat( 3 ) );
		}
		else if( (sCmd.same_no_case( "direction" )  && (args.Size() == 4) ) )
		{
			gWeather.GetWind()->SetAngularDirection( args.GetAsFloat( 3 ) );
		}
	}
	else if( sType.same_no_case( "snow" ) )
	{
		if( sCmd.same_no_case( "info" ) )
		{
			if( gWeather.GetSnow()->GetSnowing() )
			{
				output = "Snow is ON:\n";
			}
			else
			{
				output = "Snow is OFF:\n";
			}
			sprintf( szSettings, "density = %g\n", gWeather.GetSnow()->GetDensity() );
			output += szSettings;
		}
		else if( sCmd.same_no_case( "on" ) )
		{
			gWeather.GetSnow()->SetSnowing( true );
		}
		else if( sCmd.same_no_case( "off" ) )
		{
			gWeather.GetSnow()->SetSnowing( false );
		}
		else if( (sCmd.same_no_case( "density" )  && (args.Size() == 4) ) )
		{
			gWeather.GetSnow()->SetDensity( args.GetAsFloat( 3 ) );
		}
	}
	return true;
}



//================================================================
//
// gpc_sysinfo
//
//----------------------------------------------------------------

class gpc_sysinfo : public GpConsole_command
{
public:
	gpc_sysinfo();
	~gpc_sysinfo(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_sysinfo::gpc_sysinfo()
{
	// register the command
	gpstring help;

	std::vector< gpstring > usage;

	help = "sysinfo - show information about the machine and OS";

	usage.push_back( "sysinfo" );

	gGpConsole.RegisterCommand( *this, "sysinfo", help, usage );
};




bool gpc_sysinfo::Execute( GpConsole_command_arguments & /*args*/, gpstring & output )
{

	system_report mSysInfo;
	mSysInfo.Get( output );
	return( true );
}




//================================================================
//
// gpc_camera
//
//----------------------------------------------------------------

class gpc_camera : public GpConsole_command
{
public:
	gpc_camera();
	~gpc_camera(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 4 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_camera::gpc_camera()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "camera [nearclip farclip followmode springlook] - camera control\n";
	usage.push_back( "camera nearclip" );
	usage.push_back( "camera farclip" );
	usage.push_back( "camera followmode" );
	usage.push_back( "camera springlook" );
	gGpConsole.RegisterCommand( *this, "camera", help, usage );
};


bool gpc_camera::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	switch( args.Size() )
	{
	case 1:
	{
		break;
	}

	case 2:
	{
		gpstring command	= args.GetAsString( 1 );

		if( command.same_no_case( "followmode" ) )
		{
			gUICamera.SetFollowCam( !gUICamera.GetFollowCam() );
		}
		else if( command.same_no_case( "springlook" ) )
		{
			gUICamera.SetSpringLook( !gUICamera.GetSpringLook() );
		}

		break;
	}

	case 3:
	{
		gpstring command	= args.GetAsString( 1 );

		if( command.same_no_case( "nearclip" ) )
		{
			float dist = args.GetAsFloat( 2 );
			if( dist <= 0.0f )
			{
				output += "Near clip must be greater than 0\n";
			}
			else if( dist >= gSiegeEngine.Renderer().GetClipFarDist() )
			{
				output += "Near clip must be less than Far clip\n";
			}
			else
			{
				gSiegeEngine.Renderer().SetClipNearDist( dist );
			}

			break;
		}
		else if( command.same_no_case( "farclip" ) )
		{
			float dist = args.GetAsFloat( 2 );
			if( dist <= gSiegeEngine.Renderer().GetClipNearDist() )
			{
				output += "Far clip must be greater than NearClip\n";
			}
			else
			{
				gSiegeEngine.Renderer().SetClipFarDist( dist );
			}

			break;
		}

		break;
	}
	}

	return( true );
}


//================================================================
//
// gpc_gamma
//
//----------------------------------------------------------------

class gpc_gamma : public GpConsole_command
{
public:
	gpc_gamma();
	~gpc_gamma(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_gamma::gpc_gamma()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "\ngamma <amount> - gamma control\n";
	help +="0.0f < amount < 2.0f\n";
	help +="ie. 'gamma 0.1f'    - set gamma to dark\n";
	help +="    'gamma 1.0f'    - set gamma to normal\n";
	help +="    'gamma 1.9f'    - set gamma to bright\n";
	usage.push_back( "gamma" );
	gGpConsole.RegisterCommand( *this, "gamma", help, usage );
};


bool
gpc_gamma::Execute( GpConsole_command_arguments &args, gpstring &/*output*/ )
{
	if( args.Size() != 2 )
	{
		return false;
	}

	float gamma	= args.GetAsFloat( 1 );
	if( gamma <= 2.0f && gamma >= 0.0f )
	{
		// Set the gamma
		gDefaultRapi.SetGamma( gamma );
		return true;
	}
	return false;
}


//================================================================
//
// gpc_sysgamma
//
//----------------------------------------------------------------

class gpc_sysgamma : public GpConsole_command
{
public:
	gpc_sysgamma();
	~gpc_sysgamma(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_sysgamma::gpc_sysgamma()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "\nsysgamma <amount> - system level gamma control\n";
	help +="0.0f < amount < 3.5f\n";
	usage.push_back( "sysgamma" );
	gGpConsole.RegisterCommand( *this, "sysgamma", help, usage );
};


bool
gpc_sysgamma::Execute( GpConsole_command_arguments &args, gpstring &/*output*/ )
{
	if( args.Size() != 2 )
	{
		return false;
	}

	float gamma	= args.GetAsFloat( 1 );
	if( gamma <= 3.5f && gamma >= 0.0f )
	{
		// Set the gamma
		gDefaultRapi.SetSystemGamma( gamma );
		return true;
	}
	return false;
}


//================================================================
//
// gpc_nodeinfo
//
//----------------------------------------------------------------

class gpc_nodeinfo : public GpConsole_command
{
public:
	gpc_nodeinfo();
	~gpc_nodeinfo(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_nodeinfo::gpc_nodeinfo()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "nodeinfo - show information about currently selected node";
	usage.push_back( "nodeinfo" );
	gGpConsole.RegisterCommand( *this, "nodeinfo", help, usage );
};


bool gpc_nodeinfo::Execute( GpConsole_command_arguments & /*args*/, gpstring & /*output*/ )
{
	if( gSiegeEngine.GetMouseShadow().IsTerrainHit())
	{
		siege::database_guid hitGUID = gSiegeEngine.GetMouseShadow().GetFloorHitPos().node;
		const SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hitGUID ) );
		const SiegeNode& locate_node = handle.RequestObject( gSiegeEngine.NodeCache() );

		gpgenericf(( "\nNODE_GUID = %s\n", hitGUID.ToString().c_str() ));
		gpgenericf(( "MESH_GUID = %s\n", locate_node.GetMeshGUID().ToString().c_str() ));

		const SiegeMesh& bmesh = locate_node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
		const std::list<gpstring> &infolist = bmesh.GetInfo();
		if ( !infolist.empty() )
		{
			gpgenericf(( "\nINFOLIST:\n\n" ));
			ReportSys::AutoIndent autoIndent( &gGenericContext );

			for (std::list<gpstring>::const_iterator i = infolist.begin() ; i!=infolist.end(); ++i)
			{
				gpgenericf(( "%s\n", i->c_str() ));
			}
		}

		const siege::NodeTexColl& textures = locate_node.GetTextureListing();
		if ( !textures.empty() )
		{
			gpgenericf(( "\nTEXTURES:\n\n" ));
			ReportSys::AutoIndent autoIndent( &gGenericContext );

			siege::NodeTexColl::const_iterator i, ibegin = textures.begin(), iend = textures.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				gpgenericf(( "%s\n", gDefaultRapi.GetTexturePathname( (*i).textureId ) ));
			}
		}
	}
	else
	{
		gpgeneric( "No node selected\n" );
	}
	return( true );
}


//================================================================
//
// gpc_meshcheck
//
//----------------------------------------------------------------

class gpc_meshcheck : public GpConsole_command
{
public:
	gpc_meshcheck();
	~gpc_meshcheck(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_meshcheck::gpc_meshcheck()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "meshcheck - analyze mesh information in the current scene for bad doors";
	usage.push_back( "meshcheck" );
	gGpConsole.RegisterCommand( *this, "meshcheck", help, usage );
};


bool gpc_meshcheck::Execute( GpConsole_command_arguments & /*args*/, gpstring & output )
{
	siege::SiegeFrustum* pFrustum	= gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pFrustum )
	{
		std::set< siege::database_guid >	meshesChecked;

		// Go through all loaded nodes
		for( siege::FrustumNodeColl::iterator n = pFrustum->GetFrustumNodeColl().begin(); n != pFrustum->GetFrustumNodeColl().end(); ++n )
		{
			if( meshesChecked.find( (*n)->GetMeshGUID() ) != meshesChecked.end() )
			{
				continue;
			}

			meshesChecked.insert( (*n)->GetMeshGUID() );

			// Get the mesh
			siege::SiegeMesh& bmesh		= (*n)->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );

			// Go through each door in the mesh
			for( siege::SiegeMesh::SiegeMeshDoorConstIter md = bmesh.GetDoors().begin(); md != bmesh.GetDoors().end(); ++md )
			{
				for( std::vector<int>::iterator dv = (*md)->GetVertexIndices().begin(); dv != (*md)->GetVertexIndices().end(); ++dv )
				{
					sVertex doorVert	= bmesh.GetVertices()[ (*dv) ];

					doorVert.x			= FABSF( doorVert.x * 20.0f );
					doorVert.y			= FABSF( doorVert.y * 20.0f );
					doorVert.z			= FABSF( doorVert.z * 20.0f );

					doorVert.x			= IsEqual( doorVert.x, ceilf( doorVert.x ), 0.001f ) ? ceilf( doorVert.x ) :
										  IsEqual( doorVert.x, floorf( doorVert.x ), 0.001f ) ? floorf( doorVert.x ) : doorVert.x;
					doorVert.y			= IsEqual( doorVert.y, ceilf( doorVert.y ), 0.001f ) ? ceilf( doorVert.y ) :
										  IsEqual( doorVert.y, floorf( doorVert.y ), 0.001f ) ? floorf( doorVert.y ) : doorVert.y;
					doorVert.z			= IsEqual( doorVert.z, ceilf( doorVert.z ), 0.001f ) ? ceilf( doorVert.z ) :
										  IsEqual( doorVert.z, floorf( doorVert.z ), 0.001f ) ? floorf( doorVert.z ) : doorVert.z;

					if( !IsZero( doorVert.x - (float)FTOL( doorVert.x ) ) ||
						!IsZero( doorVert.y - (float)FTOL( doorVert.y ) ) ||
						!IsZero( doorVert.z - (float)FTOL( doorVert.z ) ) )
					{
						output.appendf( "%s   - door %d\n", (*n)->GetMeshGUID().ToString().c_str(), (*md)->GetID() );
						break;
					}
				}
			}
		}
	}
	
	return( true );
}


//================================================================
//
// gpc_frustuminfo
//
//----------------------------------------------------------------

class gpc_frustuminfo : public GpConsole_command
{
public:
	gpc_frustuminfo();
	~gpc_frustuminfo(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 1 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_frustuminfo::gpc_frustuminfo()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "frustuminfo - display information about the currently active frustum";
	usage.push_back( "frustuminfo" );
	gGpConsole.RegisterCommand( *this, "frustuminfo", help, usage );
};


bool gpc_frustuminfo::Execute( GpConsole_command_arguments & /*args*/, gpstring & output )
{
	siege::SiegeFrustum* pFrustum	= gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
	if( pFrustum )
	{
		output.appendf( "--------- General Frustum Info ---------\n" );
		output.appendf( "Number of frustums      = %d\n", gSiegeEngine.GetNumberOfFrustums() );
		output.appendf( "Active frustum ID       = %d\n", gSiegeEngine.GetRenderFrustum() );
		output.appendf( "--------- Active Frustum Info ----------\n" );
		output.appendf( "Current frustum width   = %f\n", pFrustum->GetWidth() );
		output.appendf( "Current frustum height  = %f\n", pFrustum->GetHeight() );
		output.appendf( "Current interest radius = %f\n", pFrustum->GetInterestRadius() );
		output.appendf( "Current active nodes    = %d\n", pFrustum->GetFrustumNodeColl().size() );
		output.appendf( "Current loaded nodes    = %d\n", pFrustum->GetFrustumNodeColl().size() + pFrustum->GetLoadedNodeColl().size() );
	}
	
	return( true );
}



//================================================================
//
// gpc_infobox
//
//----------------------------------------------------------------

class gpc_infobox : public GpConsole_command
{
public:
	gpc_infobox();
	~gpc_infobox(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_infobox::gpc_infobox()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "infobox - change the information displayed in the information box";
	usage.push_back( "infobox" );
	gGpConsole.RegisterCommand( *this, "infobox", help, usage );
};


bool gpc_infobox::Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	gUIGame.SetInfoMode( args.GetAsString( 1 ) );
	return( true );
}



//================================================================
//
// gpc_rapi
//
//----------------------------------------------------------------

class gpc_rapi : public GpConsole_command
{
public:
	gpc_rapi();
	~gpc_rapi(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 9 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_rapi::gpc_rapi()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help  = "rapi <info|cliptofog> [values]\n\n";
	help += " cliptofog options:  <float>";
	usage.push_back( "rapi info" );
	usage.push_back( "rapi cliptofog" );
	gGpConsole.RegisterCommand( *this, "rapi", help, usage );

};


bool
gpc_rapi::Execute( GpConsole_command_arguments &args, gpstring &output )
{
	gpstring sCmd;

	if( args.Size() < 1 )
	{
		sCmd = "info";
	} else {
		sCmd = args.GetAsString( 1 );
	}

	if( (sCmd.same_no_case( "cliptofog" )  && (args.Size() == 3) ) )
	{
		float fcliptofog = args.GetAsFloat( 2 );
		gDefaultRapi.SetClipToFogRatio(fcliptofog);
		return true;

	} else if( (sCmd.same_no_case( "info" ) ) ) {

		output.appendf("\n\nNear Clip Dist = %f",gDefaultRapi.GetClipNearDist());
		output.appendf("\nFar Clip Dist = %f",gDefaultRapi.GetClipFarDist());
		output.appendf("\nAdj Far Clip  = %f",gDefaultRapi.GetClipFarDistAdjustedForFog());
		output.appendf("\nClip to Fog   = %f",gDefaultRapi.GetClipToFogRatio());

		if (gDefaultRapi.IsFogEnabled()) {
			output.appendf("\n\nFog Enabled = true");
			output.appendf("\n  Z Fog       = %s",(gDefaultRapi.IsZFogEnabled()?"true":"false"));
			output.appendf("\n  W Fog       = %s",(gDefaultRapi.IsWFogEnabled()?"true":"false"));
			output.appendf("\n  Range Fog   = %s",(gDefaultRapi.IsRangeBasedFogEnabled()?"true":"false"));
		} else {
			output.appendf("\n\nFog Enabled   = false",gDefaultRapi.GetFogNearDist());
		}

		output.appendf("\nNear Fog Dist = %f",gDefaultRapi.GetFogNearDist());
		output.appendf("\nFar Fog Dist  = %f",gDefaultRapi.GetFogFarDist());

		output.appendf("\nClip Space Near Fog Dist = %f",gDefaultRapi.GetClipSpaceFogNearDist());
		output.appendf("\nClip Space Far Fog Dist  = %f",gDefaultRapi.GetClipSpaceFogFarDist());

		output+= "\n";
		return true;

	}

	return false;
}


//================================================================
//
// gpc_mcp
//
//----------------------------------------------------------------

class gpc_mcp : public GpConsole_command
{
public:
	gpc_mcp();
	~gpc_mcp(){};
	UINT32 GetFlags()				{ return( 0 ); }
	gpstring GetLocation()			{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()	{ return( 1 ); }
	unsigned int GetMaxArguments()	{ return( 3 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_mcp::gpc_mcp()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "mcp [path|optimize|lag|label|hud|goal|block|debug|test|crowding] - path tweaks\n";
	usage.push_back( "mcp path" );
	usage.push_back( "mcp optimize" );
	usage.push_back( "mcp lag <val>" );
	usage.push_back( "mcp label" );
	usage.push_back( "mcp hud" );
	usage.push_back( "mcp goal" );
	usage.push_back( "mcp block" );
	usage.push_back( "mcp debug" );
	usage.push_back( "mcp test" );
	usage.push_back( "mcp crowding" );
	gGpConsole.RegisterCommand( *this, "mcp", help, usage );
};


bool gpc_mcp::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	switch( args.Size() )
	{
	case 1:
		{
			output.appendf("Lag is %f\n",gWorldOptions.GetLagMCP());
			output.appendf("Optimize is %s\n",gWorldOptions.GetOptimizeMCP()?"on":"off");
			output.appendf("Paths are %s\n",gWorldOptions.GetShowPathsMCP()?"visible":"not visible");
			output.appendf("Labelling is %s\n",gWorldOptions.GetLabelMCP()?"on":"off");
			output.appendf("HUDs are %s\n",gWorldOptions.GetHudMCP()?"visible":"not visible");
			output.appendf("Goals are %s\n",gWorldOptions.GetGoalMCP()?"visible":"not visible");
			output.appendf("Blocking is %s\n",gWorldOptions.GetBlockMCP()?"visible":"not visible");
			output.appendf("Debugging is %s\n",gWorldOptions.GetDebugMCP()?"enabled":"disabled");
			output.appendf("Testing is %s\n",gWorldOptions.GetTestMCP()?"enabled":"disabled");
			output.appendf("Crowding is %s\n",gWorldOptions.GetCrowdingMCP()?"visible":"not visible");
			break;
		}
	case 2:
		{
			gpstring command	= args.GetAsString( 1 );
			if( command.same_no_case( "optimize" ) )
			{
				gWorldOptions.SetOptimizeMCP( !gWorldOptions.GetOptimizeMCP() );
				output.appendf("MCP optimize is %s\n",gWorldOptions.GetDebugMCP()?"ENABLED":"DISABLED");
			}
			else if( command.same_no_case( "path" ) )
			{
				gWorldOptions.SetShowPathsMCP( !gWorldOptions.GetShowPathsMCP() );
			}
			else if( command.same_no_case( "label" ) )
			{
				gWorldOptions.SetLabelMCP( !gWorldOptions.GetLabelMCP() );
			}
			else if( command.same_no_case( "hud" ) )
			{
				gWorldOptions.SetHudMCP( !gWorldOptions.GetHudMCP() );
			}
			else if( command.same_no_case( "goal" ) )
			{
				gWorldOptions.SetGoalMCP( !gWorldOptions.GetGoalMCP() );
			}
			else if( command.same_no_case( "block" ) )
			{
				gWorldOptions.SetBlockMCP( !gWorldOptions.GetBlockMCP() );
			}
			else if( command.same_no_case( "crowding" ) )
			{
				gWorldOptions.SetCrowdingMCP( !gWorldOptions.GetCrowdingMCP() );
			}
			else if( command.same_no_case( "debug" ) )
			{
				gWorldOptions.SetDebugMCP( !gWorldOptions.GetDebugMCP() );
				output.appendf("MCP debugging is %s\n",gWorldOptions.GetDebugMCP()?"ENABLED":"DISABLED");
				
			}
			else if( command.same_no_case( "test" ) )
			{
				gWorldOptions.SetTestMCP( !gWorldOptions.GetTestMCP() );
				output.appendf("MCP testing is %s\n",gWorldOptions.GetTestMCP()?"ENABLED":"DISABLED");
				
			}
			break;
		}

	case 3:
		{
			gpstring command	= args.GetAsString( 1 );

			if( command.same_no_case( "lag" ) )
			{
				float lag = args.GetAsFloat( 2 );

				if( lag < 0.0f )
				{
					output += "Lag cannot be negative";
				}
				else
				{
					gWorldOptions.SetLagMCP( lag );
					output.appendf("Lag is now %f\n",lag);
				}

			}

			break;
		}
	}

	return( true );
}


//================================================================
//
// gpc_report
//
//----------------------------------------------------------------

class gpc_report : public GpConsole_command
{
public:
	gpc_report();
	~gpc_report(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 4 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_report::gpc_report()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "report [<stream>] - mess with reportsys\n";
	usage.push_back( "report clearignores" );
	usage.push_back( "report enable <stream>" );
	usage.push_back( "report disable <stream>" );
	usage.push_back( "report filelog <stream> [<filename>]" );
	usage.push_back( "report nofilelog <stream>" );
	usage.push_back( "report <stream>" );
	gGpConsole.RegisterCommand( *this, "report", help, usage );
}


bool gpc_report::Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	if ( args.Size() == 1 )
	{
		gpgeneric( "\n\nReportSys Streams:\n" );
		ReportSys::Mgr::ContextColl contexts;

		if ( gReportSysMgr.QueryContextByName( contexts, "*" ) )
		{
			// build report objects
			ReportSys::TextTableSink sink;
			ReportSys::LocalContext ctx( &sink, false );
			ctx.SetSchema( new ReportSys::Schema( 5, "Name", "Enabled", "Count", "Options", "Traits" ), true );

			// out all contexts
			ReportSys::Mgr::ContextColl::const_iterator i, ibegin = contexts.begin(), iend = contexts.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				ReportSys::AutoReport autoReport( ctx );

				// logging to file?
				ReportSys::Sink* fileLog = (*i)->FindSinkByType( "RetryFile" );

				// name
				ctx.OutputFieldObj( (*i)->GetName() );

				// enabled
				ctx.OutputFieldObj( (*i)->IsEnabled() );

				// count
				ctx.OutputFieldObj( (*i)->GetReportCount() );

				// options
				gpstring options;
				options += (*i)->TestOptions( ReportSys::Context::OPTION_AUTOREPORT ) ? 'A' : ' ';
				options += (*i)->TestOptions( ReportSys::Context::OPTION_NOGLOBAL   ) ? 'N' : ' ';
				options += (*i)->TestOptions( ReportSys::Context::OPTION_LOCALIZE   ) ? 'L' : ' ';
				options += fileLog ? (fileLog->IsEnabled() ? 'F' : 'f') : ' ';
				ctx.OutputField( options );

				// traits
				gpstring traits;
				traits += (*i)->TestTraits( ReportSys::Context::TRAIT_DEBUG   ) ? 'D' : ' ';
				traits += (*i)->TestTraits( ReportSys::Context::TRAIT_DEV     ) ? 'V' : ' ';
				traits += (*i)->TestTraits( ReportSys::Context::TRAIT_WARNING ) ? 'W' : ' ';
				traits += (*i)->TestTraits( ReportSys::Context::TRAIT_ERROR   ) ? 'E' : ' ';
				ctx.OutputField( traits );
			}

			// out header
			gpgeneric( "\n" );
			gGenericContext.OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
			gpgeneric( "\n" );

			// out table
			sink.Sort();
			sink.OutputReport();

			// out legend
			gGenericContext.OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
			gpgeneric( "\nOptions: A = auto report, N = no global, L = localize, F = file log\n" );
			gpgeneric( "Traits: D = debug, V = dev, W = warning, E = error\n\n" );
		}
		else
		{
			gpgeneric( "** No streams found!\n\n" );
		}
	}
	else if ( (args.Size() == 2) || (args.Size() == 3) || (args.Size() == 4) )
	{

		bool messWithFileLog = false;
		bool hasEnableDisable = false;
		bool enable = false;
		if ( args.Size() == 3 )
		{
			gpstring subcmd = args.GetAsString( 1 );
			if ( subcmd.same_no_case( "filelog" ) )
			{
				messWithFileLog = true;
				enable = true;
			}
			else if ( subcmd.same_no_case( "nofilelog" ) )
			{
				messWithFileLog = true;
				enable = false;
			}
			else if ( subcmd.same_no_case( "enable" ) )
			{
				hasEnableDisable = true;
				enable = true;
			}
			else if ( subcmd.same_no_case( "disable" ) )
			{
				hasEnableDisable = true;
				enable = false;
			}
			else
			{
				gpgenericf(( "Invalid subcommand '%s'\n", subcmd.c_str() ));
				return ( false );
			}
		}
		else if ( args.Size() == 4 )
		{
			gpstring subcmd = args.GetAsString( 1 );
			if ( subcmd.same_no_case( "filelog" ) )
			{
				messWithFileLog = true;
				enable = true;
			}
		}

		gpstring cmd = args.GetAsString( (messWithFileLog || hasEnableDisable) ? 2 : 1 );

		if ( !messWithFileLog && !hasEnableDisable && cmd.same_no_case( "clearignores" ) )
		{
			for ( AssertInstance* i = AssertInstance::ms_Root ; i != NULL ; i = i->m_Next )
			{
				i->m_IgnoreCount = 0;
			}
			AssertData::ms_GlobalIgnoreCount = 0;

			gpgeneric( "Ignore-always fun has been canceled\n" );
		}
		else
		{
			ReportSys::Context* ctx = gReportSysMgr.FindContext( cmd );
			if ( ctx != NULL )
			{
				if ( messWithFileLog )
				{
					gpgenericf(( "%s file logging for context '%s'\n", enable ? "Enabling" : "Disabling", ctx->GetName().c_str() ));

					ReportSys::Sink* fileLog = ctx->FindSinkByType( "RetryFile" );
					if ( enable )
					{
						if ( fileLog == NULL )
						{
							gpstring fileName;

							if ( args.Size() == 4 )
							{
								fileName = args.GetAsString( 3 );
							}
							else
							{
								gpstring ctxName( ctx->GetName() );
								ctxName.to_lower();

								gpstring computerName( SysInfo::GetComputerName() );
								computerName.to_lower();
								fileName.assignf( "%s-%s.log", ctxName.c_str(), computerName.c_str() );
							}

							// Look for an existing RetryFileSink with the same filename

							ReportSys::Mgr::ContextColl contexts;
							if ( gReportSysMgr.QueryContextByName( contexts, "*" ) )
							{
								ReportSys::Mgr::ContextColl::const_iterator i, ibegin = contexts.begin(), iend = contexts.end();
								for ( i = ibegin ; i != iend && fileLog == NULL; ++i )
								{
									// logging to file?
									ReportSys::RetryFileSink* sink = (ReportSys::RetryFileSink*)(*i)->FindSinkByType( "RetryFile" );
									if (sink && sink->GetFileName().same_no_case(fileName) )
									{
										fileLog = sink;
									}
								}
							}

							if (fileLog == NULL)
							{
								// Create a new sink and 'own' it
								fileLog = new ReportSys::LogFileSink <> ( fileName, false, ctx->GetName() + "Log" );
								ctx->AddSink( fileLog, true );
							}
							else
							{
								// Add the sink, but don't own it
								ctx->AddSink( fileLog, false ); 
							}


						}
						fileLog->Enable();
					}
					else if ( fileLog != NULL )
					{
						fileLog->Disable();
					}
					ctx->Enable( enable );
				}
				else
				{
					if ( !hasEnableDisable )
					{
						enable = !ctx->IsEnabled();
					}
					gpgenericf(( "%s context '%s'\n", enable ? "Enabling" : "Disabling", ctx->GetName().c_str() ));
					ctx->Enable( enable );

					if ( enable && ctx->GetName().same_no_case( "pcontent" ) && !GetPContentErrorContext().IsEnabled() )
					{
						gpgeneric( "Also enabling 'bool tuning' for proper error feedback during pcontent logging...\n" );
						GetPContentErrorContext().Enable();
					}
				}
			}
			else
			{
				gpgenericf(( "Invalid context name '%s'\n", cmd.c_str() ));
			}
		}
	}

	return( true );
}


//================================================================
//
// gpc_exec
//
//----------------------------------------------------------------

class gpc_exec : public GpConsole_command
{
public:
	gpc_exec();
	~gpc_exec(){};
	UINT32 GetFlags()					{ return( 0 ); }
	gpstring GetLocation()				{ return( gpstring(__FILE__) ); }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 3 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );

private:
	bool AddBlock( const char* name, bool warnOnFail = true );

	struct Command
	{
		gpstring m_Command;
		gpstring m_Skrit;
		gpstring m_Doc;

		Command( void )
			{  }
	};

	typedef std::map <gpstring, Command, istring_less> CommandDb;

	CommandDb m_CommandDb;
};

gpc_exec::gpc_exec()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help = "exec - print out all available execution commands and block names\n";
	help = "exec add <blockname> - add exec entries from the block, overwriting existing entries\n";
	help = "exec clear - clear out all existing blocks\n";
	help = "exec <commandname> - execute the command by name\n";
	usage.push_back( "exec" );
	gGpConsole.RegisterCommand( *this, "exec", help, usage );

	AddBlock( "global", false );
}


bool gpc_exec::Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	if ( args.Size() == 1 )
	{
		// build report objects
		ReportSys::TextTableSink sink;
		ReportSys::LocalContext ctx( &sink, false );
		ctx.SetSchema( new ReportSys::Schema( 2, "Name", "Doc" ), true );

		// out all commands
		gpgeneric( "\n\nExecution commands:\n\n" );
		if ( !m_CommandDb.empty() )
		{
			CommandDb::const_iterator i, ibegin = m_CommandDb.begin(), iend = m_CommandDb.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				ReportSys::AutoReport autoReport( ctx );

				// name
				ctx.OutputFieldObj( i->first );

				// doc
				ctx.OutputFieldObj( i->second.m_Doc );
			}

			// out header
			gpgeneric( "\n" );
			gGenericContext.OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
			gpgeneric( "\n" );

			// out table
			sink.Sort();
			sink.OutputReport();
		}
		else
		{
			gpgeneric( "<None found!>\n" );
		}

		// out all blocks
		gpgeneric( "\nAvailable blocks:\n\n" );
		FuelHandle exec( "config:console:exec" );
		FuelHandleList blocks;
		if ( exec && exec->ListChildBlocks( 1, NULL, NULL, blocks ) )
		{
			FuelHandleList::const_iterator i, ibegin = blocks.begin(), iend = blocks.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				gpgenericf(( "\t%s\n", (*i)->GetName() ));
			}
		}
		else
		{
			gpgeneric( "<None found!>\n" );
		}
	}
	else if ( args.Size() == 2 )
	{
		gpstring exec = args.GetAsString( 1 );
		if ( exec.same_no_case( "clear" ) )
		{
			gpgeneric( "Commands cleared\n" );
			m_CommandDb.clear();
		}
		else
		{
			CommandDb::const_iterator found = m_CommandDb.find( exec );
			if ( found != m_CommandDb.end() )
			{
				const Command& cmd = found->second;

				// command
				if ( !cmd.m_Command.empty() )
				{
					FileSys::MemReader reader( const_mem_ptr( cmd.m_Command.begin(), cmd.m_Command.size() )) ;
					gpstring command, output;
					while ( reader.ReadLine( command ) )
					{
						stringtool::RemoveBorderingWhiteSpace( command );
						if ( !command.empty() )
						{
							gGpConsole.Process( command, output );
							gpgeneric( output );
							command.erase();
						}
					}
				}

				// skrit
				if ( !cmd.m_Skrit.empty() )
				{
					gSkritEngine.ExecuteCommand( "<console>", cmd.m_Skrit );
				}
			}
			else
			{
				gperrorf(( "Error: unrecognized exec command '%s', check yer spelling\n", exec.c_str() ));
			}
		}
	}
	else if ( args.Size() == 3 )
	{
		gpstring subCommand = args.GetAsString( 1 );
		if ( subCommand.same_no_case( "add" ) )
		{
			AddBlock( args.GetAsString( 2 ) );
		}
		else
		{
			gperrorf(( "Error: unrecognized subcommand '%s'\n", subCommand.c_str() ));
		}
	}

	return ( true );
}


bool gpc_exec::AddBlock( const char* name, bool warnOnFail )
{
	gpassert( name != NULL );

	// first unload all existing stuff (in case user has typed in new stuff)
	FuelHandle base( "config" );
	base.Unload();
	base = FuelHandle( "config:console:exec" );

	// get at subblock
	FuelHandle block;
	FuelHandleList blocks;
	if ( base && base->GetChildBlock( name, block ) && block->ListChildBlocks( 1, NULL, NULL, blocks ) )
	{
		// read them all in
		FuelHandleList::iterator i, ibegin = blocks.begin(), iend = blocks.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// add cmd
			Command cmd;
			cmd.m_Command = (*i)->GetString( "command" );
			cmd.m_Skrit = (*i)->GetString( "skrit" );
			cmd.m_Doc = (*i)->GetString( "doc" );
			m_CommandDb[ (*i)->GetName() ] = cmd;

			// check syntax for fun
			if ( !cmd.m_Skrit.empty() )
			{
				gSkritEngine.CheckCommandSyntax( cmd.m_Skrit );
			}
		}
	}
	else if ( warnOnFail )
	{
		gperrorf(( "Unable to find block '%s'\n", name ));
	}

	return ( true );
}


//================================================================
//
// gpc_quantify
//
//----------------------------------------------------------------

class gpc_quantify : public GpConsole_command
{
public:
	gpc_quantify();
	virtual ~gpc_quantify(){};
	UINT32       GetFlags       ( void )	{  return ( 0 );  }
	gpstring     GetLocation    ( void )	{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )	{  return ( 2 );  }
	unsigned int GetMaxArguments( void )	{  return ( 3 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& /*output*/ );
};

gpc_quantify :: gpc_quantify( void )
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help += "quantify <command> - console commands to control Rational Quantify\n";
	help += "\t<number> - count of frames to sample for and then snapshot\n";
	help += "\tconsole <number> - count of console commands to sample for and then snapshot\n";
	usage.push_back( "quantify 1" );
	usage.push_back( "quantify console 1" );

	gGpConsole.RegisterCommand( *this, "quantify", help, usage );
}

bool gpc_quantify :: Execute( GpConsole_command_arguments& args, gpstring& /*output*/ )
{
	if ( args.Size() < 2 )
	{
		return ( false );
	}

	bool console = args.GetAsString( 1 ).same_no_case( "console" );
	gpstring frames = args.GetAsString( console ? 2 : 1 );

	int count;
	if ( stringtool::Get( frames, count ) && (count > 0) )
	{
		if ( console )
		{
			// note to skip the first one because we're currently executing it
			gQuantifyApi.BeginProfile( -1 );
			gGpConsole.AddCompletionCb( makeFunctor( gQuantifyApi, &QuantifyApi::StopProfile ), count + 1 );
		}
		else
		{
			gQuantifyApi.BeginProfile( count );
		}
	}
	else
	{
		gperrorf(( "Invalid frame count '%s'\n", args.GetAsString( 2 ) ));
	}

	return ( true );
}

//================================================================
//
// gpc_heads
//
//----------------------------------------------------------------

class gpc_heads : public GpConsole_command
{
public:
	gpc_heads();
	~gpc_heads(){};
	UINT32       GetFlags   ( void )	{ return ( 0 );  }
	gpstring     GetLocation( void )	{ return ( __FILE__ );  }
	unsigned int GetMinArguments()		{ return( 1 ); }
	unsigned int GetMaxArguments()		{ return( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_heads::gpc_heads()
{
	// register the command
	gpstring help;
	std::vector< gpstring > usage;
	help  = "head <new_head_name>\n";
	usage.push_back( "head" );
	gGpConsole.RegisterCommand( *this, "head", help, usage );

};

bool gpc_heads::Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if( gGoDb.GetSelection().size() != 1 )
	{
		output += "You need to select a single character.\n";
		return true;
	}

	const Goid selected_id = (*(gGoDb.GetSelection().begin()))->GetGoid();
	GoHandle	hSelected( selected_id );

	if( args.Size() == 2 )
	{
		gpstring name = args.GetAsString( 1 );
		gpassert(hSelected->HasInventory());
		hSelected->GetInventory()->SetCustomHead(name.same_no_case("default") ? "" : name);
	}

	const gpstring& ch = hSelected->GetInventory()->GetCustomHead();
	output += "Custom Head = ";
	output += ch.empty() ? "<default>" : ch;
	output += "\n";

	return ( true );

}


//================================================================
//
// gpc_party
//
//----------------------------------------------------------------

class gpc_party : public GpConsole_command
{
public:
	gpc_party();

	UINT32       GetFlags   ( void )	{ return ( 0 );  }
	gpstring     GetLocation( void )	{ return ( __FILE__ );  }
	unsigned int GetMinArguments()		{ return ( 1 ); }
	unsigned int GetMaxArguments()		{ return ( 3 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & /*output*/ );
	void LoadParty( const gpstring& name, bool replace );
	void SaveParty( const gpstring& name );
};

gpc_party :: gpc_party()
{
	gpstring help;
	std::vector< gpstring > usage;
	help += "party             - list all available parties for import\n";
	help += "party load <name> - replace current party with saved party\n";
	help += "party save <name> - save party with given name\n";
	help += "party add  <name> - add saved party to current party (careful!)\n";

	usage.push_back( "party" );
	usage.push_back( "party load" );
	usage.push_back( "party save" );
	usage.push_back( "party add" );

	gGpConsole.RegisterCommand( *this, "party", help, usage );
}

bool gpc_party :: Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	if ( args.Size() == 1 )
	{
		SaveInfoColl infos;
		gTattooGame.GetPartySaveInfos( infos, true, true );
		if ( !infos.empty() )
		{
			gpgeneric( "Parties can be imported from:\n\n" );
			ReportSys::AutoReport autoReport( &gGenericContext );

			SaveInfoColl::iterator i, ibegin = infos.begin(), iend = infos.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				gpgenericf(( "%S\n", FileSys::GetFileNameOnly( (*i)->m_FileName ).c_str() ));
			}
		}
	}
	else
	{
		gpstring subcmd = args.GetAsString( 1 );
		if ( subcmd.same_no_case( "load" ) )
		{
			LoadParty( args.GetAsString( 2 ), true );
		}
		else if ( subcmd.same_no_case( "add" ) )
		{
			LoadParty( args.GetAsString( 2 ), false );
		}
		else if ( subcmd.same_no_case( "save" ) )
		{
			SaveParty( args.GetAsString( 2 ) );
		}
	}

	return ( true );
}

void gpc_party :: LoadParty( const gpstring& name, bool replace )
{
	// figure out where to place
	if ( !gSiegeEngine.GetMouseShadow().IsTerrainHit() )
	{
		gpwarning( "Mouse cursor is not over valid floor.\n" );
		return;
	}

	// figure out what to load
	gpwstring loadName;
	if ( name.empty() )
	{
		loadName = gTattooGame.GetLastPartySave();
		if ( loadName.empty() )
		{
			gpwarning( "No recent party saved.\n" );
			return;
		}
	}
	else
	{
		loadName = ::ToUnicode( name );
	}

	// see if it exists
	if ( !gTattooGame.HasPartySave( loadName, true ) )
	{
		gpwarning( "That save game does not exist.\n" );
		return;
	}

	// switch to reloading state temporarily
	eWorldState oldState = gWorldState.GetCurrentState();
	gWorldStateRequest( WS_RELOADING );
	gWorldState.Update();

	// load that party
	GameLoader loader( GameSaver::OPTION_PARTY_SAVE );
	bool rc = loader.BeginLoad( loadName );
	if ( !rc )
	{
		// try as a world save
		loader.SetOptions( GameSaver::OPTION_WORLD_SAVE );
		rc = loader.BeginLoad( loadName );
	}
	if ( rc )
	{
		// nuke current party?
		if ( replace )
		{
			Go* party = gServer.GetScreenParty();
			if ( party != NULL )
			{
				GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
				for ( i = ibegin ; i != iend ; ++i )
				{
					gGoDb.SMarkGoAndChildrenForDeletion( (*i)->GetGoid() );
				}
				gGoDb.CommitAllRequests();
			}
		}

		// kill it from ui
		gUIPartyManager.DeconstructUIParty();

		// load it
		rc = loader.LoadParty( gSiegeEngine.GetMouseShadow().GetFloorHitPos(), replace );

		// set the portrait
		if ( replace )
		{
			GoHandle hero( gServer.GetScreenPlayer()->GetHero() );
			if ( hero )
			{
				hero->GetActor()->RCUpdatePortraitTexture();
			}
		}

		// tell ui to rebuild the party
		gUIPartyManager.ConstructUIParty();
	}

	// clean out pending ui fun
	gUI.Update( 0 );

	// status
	if ( rc )
	{
		gpscreen( "Party Load completed\n" );
	}
	else
	{
		gpscreen( "Party Load failed\n" );
	}

	// switch back to old state
	gWorldStateRequest( oldState );
	gWorldState.Update();
}

void gpc_party :: SaveParty( const gpstring& name )
{
	if ( name.empty() )
	{
		gpwstring lastName = gTattooGame.GetLastPartySave();
		if ( !lastName.empty() )
		{
			gTattooGame.SaveParty( lastName, true );
		}
		else
		{
			gpwarningf(( "No recent party saved\n" ));
		}
	}
	else
	{
		gTattooGame.SaveParty( ::ToUnicode( name ), true );
	}
}


//================================================================
//
// gpc_godb
//
//----------------------------------------------------------------

class gpc_godb : public GpConsole_command
{
public:
	gpc_godb();

	UINT32       GetFlags   ( void )	{ return ( 0 );  }
	gpstring     GetLocation( void )	{ return ( __FILE__ );  }
	unsigned int GetMinArguments()		{ return ( 2 ); }
	unsigned int GetMaxArguments()		{ return ( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & /*output*/ );
};

gpc_godb :: gpc_godb()
{
	gpstring help;
	std::vector< gpstring > usage;
	help += "godb expire_now - force all currently expiring objects to expire immediately\n";

	usage.push_back( "godb expire_now" );

	gGpConsole.RegisterCommand( *this, "godb", help, usage );
}

bool gpc_godb :: Execute( GpConsole_command_arguments & args, gpstring & /*output*/ )
{
	if ( args.Size() == 2 )
	{
		gpstring subcmd = args.GetAsString( 1 );
		if ( subcmd.same_no_case( "expire_now" ) )
		{
			gGoDb.ExpireAllImmediately();
		}
	}

	return ( true );
}

//================================================================
//
// gpc_triggersys
//
//----------------------------------------------------------------

class gpc_triggersys : public GpConsole_command
{
public:
	gpc_triggersys();
	~gpc_triggersys(){};

	UINT32       GetFlags   ( void )	{ return ( 0 );  }
	gpstring     GetLocation( void )	{ return ( __FILE__ );  }
	unsigned int GetMinArguments()		{ return ( 1 ); }
	unsigned int GetMaxArguments()		{ return ( 2 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_triggersys :: gpc_triggersys()
{
	gpstring help;
	std::vector< gpstring > usage;
	help += "triggersys display - toggles display of trigger system debugging info\n";
	help += "triggersys reports - toggles reporting of trigger system debugging info\n";
	help += "triggersys hitmarks - toggles display of trigger system boundary hits\n";

	usage.push_back( "triggersys display" );
	usage.push_back( "triggersys reporting" );
	usage.push_back( "triggersys hitmarks" );

	gGpConsole.RegisterCommand( *this, "triggersys", help, usage );
}

bool gpc_triggersys :: Execute( GpConsole_command_arguments & args, gpstring & output )
{

	if ( args.Size() == 2 )
	{
		gpstring subcmd = args.GetAsString( 1 );
		if ( subcmd.same_no_case( "display" ) )
		{
			gWorldOptions.SetShowTriggerSys( !gWorldOptions.GetShowTriggerSys() );
		}
		if ( subcmd.same_no_case( "reporting" ) )
		{
			gReportSysMgr.EnableContext( "TriggerSys", !gTriggerSysContext.IsEnabled() );
			if (GetTriggerSysContext().IsEnabled())
			{
				gpdevreportf( &gTriggerSysContext,("Trigger reporting enabled\n"));
			}
		}
		if ( subcmd.same_no_case( "hitmarks" ) )
		{
			gWorldOptions.SetShowTriggerSysHits( !gWorldOptions.GetShowTriggerSysHits() );
		}
	}
	output.appendf("TriggerSys display is %s\n", gWorldOptions.GetShowTriggerSys() ? "enabled" : "disabled" );
	output.appendf("TriggerSys reporting is %s\n", GetTriggerSysContext().IsEnabled() ? "enabled" : "disabled" );
	output.appendf("TriggerSys hitmarks is %s\n", gWorldOptions.GetShowTriggerSysHits() ? "enabled" : "disabled" );

	return ( true );
}


//================================================================
//
// gpc_perfmon
//
//----------------------------------------------------------------

class gpc_perfmon : public GpConsole_command
{
public:
	gpc_perfmon();
	~gpc_perfmon(){};

	UINT32       GetFlags   ( void )	{ return ( 0 );  }
	gpstring     GetLocation( void )	{ return ( __FILE__ );  }
	unsigned int GetMinArguments()		{ return ( 2 ); }
	unsigned int GetMaxArguments()		{ return ( 3 ); }

	bool Execute( GpConsole_command_arguments & args, gpstring & output );
};

gpc_perfmon :: gpc_perfmon()
{
	gpstring help;
	std::vector< gpstring > usage;
	help += "perfmon log  - toggles logging of performance monitoring data\n";
	help += "perfmon marker <optional marker name>  - adds marker to the perfmon log\n";
	help += "More functions comming soon!  I promise! \n";
	
	usage.push_back( "perfmon log" );

	gGpConsole.RegisterCommand( *this, "perfmon", help, usage );
}

bool gpc_perfmon :: Execute( GpConsole_command_arguments & args, gpstring & output )
{
	if ( args.Size() > 1 )
	{
		gpstring subcmd = args.GetAsString( 1 );
		if ( subcmd.same_no_case( "log" ) )
		{
			if ( gAppModule.TestOptions( AppModule::OPTION_PERFMON_LOG ) )
			{
				gAppModule.ClearOptions( AppModule::OPTION_PERFMON_LOG );
			}
			else
			{
				gAppModule.SetOptions( AppModule::OPTION_PERFMON_LOG );
			}
		}
		else if( subcmd.same_no_case( "marker" ) )
		{
			if( args.Size() > 2 )
			{
				gTattooGame.AddPendingOrderItem( args.GetAsString( 2 ) );
				output.appendf("perfmon log is marked with %s\n", args.GetAsString( 2 ) );
			}
			else 
			{
				gTattooGame.AddPendingOrderItem( gpstring("marker") );
				output.append("perfmon log is marked with marker\n" );
			}
		}
		else
		{
			output.appendf("option for perfmon not found, type help perfmon for usage\n" );
		}
	}
	
	output.appendf("perfmon log is %s\n", gAppModule.TestOptions( AppModule::OPTION_PERFMON_LOG ) ? "enabled!" : "disabled" );

	return ( true );
}

UIGame_console_commands::UIGame_console_commands()
{
	stdx::make_auto_ptr( mgpc_blowtorch,        new gpc_blowtorch        );
	stdx::make_auto_ptr( mgpc_bool,             new gpc_bool             );
	stdx::make_auto_ptr( mgpc_camera,           new gpc_camera           );
	stdx::make_auto_ptr( mgpc_exit,             new gpc_exit             );
	stdx::make_auto_ptr( mgpc_gamma,            new gpc_gamma            );
	stdx::make_auto_ptr( mgpc_sysgamma,         new gpc_sysgamma         );
	stdx::make_auto_ptr( mgpc_infobox,          new gpc_infobox          );
	stdx::make_auto_ptr( mgpc_kill,             new gpc_kill             );
	stdx::make_auto_ptr( mgpc_nodeinfo,         new gpc_nodeinfo         );
	stdx::make_auto_ptr( mgpc_meshcheck,		new gpc_meshcheck		 );
	stdx::make_auto_ptr( mgpc_frustuminfo,		new gpc_frustuminfo		 );
	stdx::make_auto_ptr( mgpc_sound,            new gpc_sound            );
	stdx::make_auto_ptr( mgpc_sim,              new gpc_sim              );
	stdx::make_auto_ptr( mgpc_siegever,         new gpc_siegever         );
	stdx::make_auto_ptr( mgpc_stats,            new gpc_stats            );
	stdx::make_auto_ptr( mgpc_sysinfo,          new gpc_sysinfo          );
	stdx::make_auto_ptr( mgpc_teleport,         new gpc_teleport         );
	stdx::make_auto_ptr( mgpc_weather,          new gpc_weather          );
	stdx::make_auto_ptr( mgpc_rapi,             new gpc_rapi             );
	stdx::make_auto_ptr( mgpc_mcp,              new gpc_mcp              );
	stdx::make_auto_ptr( mgpc_report,           new gpc_report           );
	stdx::make_auto_ptr( mgpc_exec,             new gpc_exec             );
	stdx::make_auto_ptr( mgpc_quantify,         new gpc_quantify         );
	stdx::make_auto_ptr( mgpc_heads,            new gpc_heads            );
	stdx::make_auto_ptr( mgpc_party,            new gpc_party            );
	stdx::make_auto_ptr( mgpc_godb,             new gpc_godb             );
	stdx::make_auto_ptr( mgpc_triggersys,       new gpc_triggersys       );
	stdx::make_auto_ptr( mgpc_perfmon,			new gpc_perfmon			 );
}


UIGame_console_commands :: ~UIGame_console_commands()
{
	// this space intentionally left blank...
}

#endif // !GP_RETAIL
