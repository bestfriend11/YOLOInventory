#include "precomp_world.h"

#include "trigger_sys.h"
#include "trigger_actions.h"
#include "trigger_conditions.h"

#include "gps_manager.h"
#include "mood.h"
#include "victory.h"
#include "worldfx.h"
#include "worldmap.h"
#include "worldterrain.h"
#include "GoDefs.h"
#include "GoInventory.h"
#include "WorldTime.h"

using namespace trigger;

//
// Actions
//

//
// set_interest_radius( "Radius" )
//

set_interest_radius::set_interest_radius()
{
	Parameter::format p0;
	p0.sName			= "Radius";
	p0.isFloat			= true;
	p0.isRequired		= true;
	p0.i_default		= -1;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Use parameters defined by condition";
	p1.isString			= true;
	p1.isRequired		= true;
	p1.sValues			= gTriggerSys.GetConditionNames();
	m_Parameters.Add( p1 );
}

void set_interest_radius::Execute( Trigger & trigger, const Params &params )
{
	gpdevreportf(&gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf(&gTriggerSysContext,("Executing [set_interest_radius] [%s]\n", params.Doc.c_str()));

	if( params.floats.empty() )
	{
		// Problem.  Not enough good shit.
		gpwarningf(("trigger action set_interest_radius was not supplied with enough parameters from %s",trigger.GetOwner()->DevGetFuelAddress().c_str()));
	}
	else
	{
		GoOccupantMap* occs; 
		if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
		{
			GoOccupantMapIter it = occs->begin();

			for (;it!=occs->end();++it)
			{
				Goid occugoid = (*it).first;
				if( !GetGo( occugoid ) )
				{
					gpwarningf((
						"TRIGGERSYS: trigger at scid 0x%08x, action 'set_interest_radius', couldn't resolve Goid 0x%08x.."
						"A GO has appears to have left the world while still inside a trigger boundary."
						"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
						MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid )
						));
				}
				else
				{
					// Get the frustum id from the GoDb
					FrustumId fId	= gGoDb.GetGoFrustum( occugoid );
					if( fId != FRUSTUMID_INVALID )
					{
						siege::SiegeFrustum* pFrustum = gSiegeEngine.GetFrustum( MakeInt( fId ) );
						gpassert( pFrustum );
						if (pFrustum)
						{
							pFrustum->SetInterestRadius( params.floats[0] );
							gpdevreportf( &gTriggerSysContext,("\t\t\t\tSET INTEREST RADIUS for [%s g:0x%08x s:0x%08x] to %f\n",
																GetGo( occugoid )->GetTemplateName(),
																occugoid,
																GetGo( occugoid )->GetScid(),
																params.floats[0]
																));
						}
					}
					else
					{
						gpdevreportf( &gTriggerSysContext,("\t\t\t\tINVALID FRUSTUM: CANNOT SET INTEREST RADIUS for [%s g:0x%08x s:0x%08x]\n",
															GetGo( occugoid )->GetTemplateName(),
															occugoid,
															GetGo( occugoid )->GetScid()
															));
					}
				}
			}
		}
		else
		{
			gpdevreportf( &gTriggerSysContext,("\t\t\t\tTHERE ARE NO OCCUPANTS TO SET THE RADIUS ON!!!\n"));
		}
	}
}


//
// fade_nodes( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
fade_nodes::fade_nodes()
{
	Parameter::format p0;
	p0.sName			= "Region ID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.i_default		= -1;
	p0.bSaveAsHex		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Section";
	p1.isInt			= true;
	p1.isRequired		= false;
	p1.i_default		= -1;
	m_Parameters.Add( p1 );

	Parameter::format p2;
	p2.sName			= "Level";
	p2.isInt			= true;
	p2.isRequired		= false;
	p2.i_default		= -1;
	m_Parameters.Add( p2 );

	Parameter::format p3;
	p3.sName			= "Object";
	p3.isInt			= true;
	p3.isRequired		= false;
	p3.i_default		= -1;
	m_Parameters.Add( p3 );

	Parameter::format p4;
	p4.sName			= "Fade Type";
	p4.isString			= true;
	p4.isRequired		= true;
	p4.sValues.push_back( "out:alpha" );
	p4.sValues.push_back( "out:black" );
	p4.sValues.push_back( "out:instant" );
	p4.sValues.push_back( "in" );
	p4.sValues.push_back( "in:instant" );
	m_Parameters.Add( p4 );

}

void fade_nodes::Execute( Trigger & trigger, const Params &params)
{
#if !GP_RETAIL
	if (!trigger.IsClientTrigger())
	{
		gperrorf( ("Error: You seem to have a 'fade_nodes' action attached to a server-only trigger. Owner Scid 0x%08x\n",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1) );
	}
#endif

	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	if( (params.strings.empty()) || params.ints.empty() )
	{
		// Problem.  Not enough good shit.
		gpwarningf(("trigger action fade_nodes was not supplied with enough parameters from %s",trigger.GetOwner()->DevGetFuelAddress().c_str()));
	}
	else
	{
		
		if ( !gWorldMap.ContainsRegion(MakeRegionId(params.ints[0])) )
		{
			gperrorf((
				"BAD CONTENT: Please tell Dave T that trigger [%s:0x%08x:%d] has a FADE_NODES action with an invalid REGION ID [0x%08x]\n",
				trigger.GetOwner()->GetTemplateName(),
				trigger.GetOwner()->GetScid(),
				params.SubGroup,
				params.ints[0]
				));
		}
		else
		{
			gpdevreportf( &gTriggerSysContext,("Executing [fade_nodes:%s] "
											   "(%s) [0x%08x,%d,%d,%d] %s\n",
				params.strings[0].c_str(),										// Fade Type
				gWorldMap.GetRegionName(MakeRegionId(params.ints[0])).c_str(),	// Region Name String
				params.ints[0],													// Region ID
				params.ints[1],													// Section
				params.ints[2],													// Level
				params.ints[3],													// Object
				params.Doc.c_str()												// Docs
				));

			// GO GO GO!
			FADETYPE ft	= FT_NONE;
			if( params.strings[0].same_no_case( "out:alpha" ) )
			{
				ft	= FT_ALPHA;
			}
			else if( params.strings[0].same_no_case( "out:black" ) )
			{
				ft	= FT_BLACK;
			}
			else if( params.strings[0].same_no_case( "out:instant" ) )
			{
				ft	= FT_INSTANT;
			}
			else if( params.strings[0].same_no_case( "in" ) )
			{
				ft	= FT_IN;
			}
			else if( params.strings[0].same_no_case( "in:instant" ) )
			{
				ft	= FT_IN_INSTANT;
			}
			else
			{
				gpwarningf(("Could not associate correct fade type with fade_nodes trigger"));
			}

			// $$$$$ This will execute multiple times, once for each occupant
			GoOccupantMap* occs; 
			if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs) )
			{

				GoOccupantMapIter it = occs->begin();

				GoidColl members;

				for( ;it!=occs->end();++it )
				{
					Goid occugoid = (*it).first;
					if( !GetGo( occugoid ) )
					{
						gpwarningf((
							"TRIGGERSYS: trigger at scid 0x%08x, action 'fade_nodes', couldn't resolve Goid 0x%08x."
							"A GO has appears to have left the world while still inside a trigger boundary."
							"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
							MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid ) 
							));
					}
					else
					{
						members.push_back(occugoid);
						gpdevreportf( &gTriggerSysContext,("\t\t\t\tSET FADE for [%s g:0x%08x s:0x%08x]\n",
															GetGo( occugoid )->GetTemplateName(),
															occugoid,
															GetGo( occugoid )->GetScid()
															));
					}
				}

				if (!members.empty())
				{
					double fadetime = PreciseAdd(gWorldTime.GetTime(), (trigger.GetDelay() + params.Delay));
					ActionInfo * actionInfo = new ActionInfo(
											fadetime,
											ActionInfo::CT_NODE_FADE,
											params.ints[0],
											params.ints[1],
											params.ints[2],
											params.ints[3],
											ft,
											members
											);
		
					gTriggerSys.AddActionInfo( actionInfo->m_Time, actionInfo );
				}
			}
			else
			{
				gpwarningf(("WARNING: [fade_nodes] failed to locate any satisfying Gos. No FrustumID! No Fade!"));
			}
		}
	}
}

#if !GP_RETAIL
bool fade_nodes::Validate( const Trigger & trigger, Params& params, gpstring& out_errmsg ) const
{
	if ( !gGoDb.IsEditMode() && !gWorldMap.ContainsRegion(MakeRegionId(params.ints[0])) )
	{
		gperrorf((
			"BAD CONTENT: Please tell Dave T that trigger [%s:0x%08x:%d] has a FADE_NODES action with an invalid REGION ID [0x%08x]\n",
			trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "<OWNER-NOT-ASSIGNED-YET>",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
			params.SubGroup,
			params.ints[0]										// Region ID
			));
		return false;
	}
	if (!trigger.IsClientTrigger())
	{
		out_errmsg.appendf( "Error: You seem to have a 'fade_nodes' action attached to a server-only trigger. Owner Scid 0x%08x\n",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1 );
		return false;
	}
	return true;
}
#endif

//
// fade_nodes_outer_local_party( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
fade_nodes_outer_local_party::fade_nodes_outer_local_party()
{
	Parameter::format p0;
	p0.sName			= "Region ID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.i_default		= -1;
	p0.bSaveAsHex		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Section";
	p1.isInt			= true;
	p1.isRequired		= false;
	p1.i_default		= -1;
	m_Parameters.Add( p1 );

	Parameter::format p2;
	p2.sName			= "Level";
	p2.isInt			= true;
	p2.isRequired		= false;
	p2.i_default		= -1;
	m_Parameters.Add( p2 );

	Parameter::format p3;
	p3.sName			= "Object";
	p3.isInt			= true;
	p3.isRequired		= false;
	p3.i_default		= -1;
	m_Parameters.Add( p3 );

	Parameter::format p4;
	p4.sName			= "Fade Type";
	p4.isString			= true;
	p4.isRequired		= true;
	p4.sValues.push_back( "out:alpha" );
	p4.sValues.push_back( "out:black" );
	p4.sValues.push_back( "out:instant" );
	p4.sValues.push_back( "in" );
	p4.sValues.push_back( "in:instant" );
	m_Parameters.Add( p4 );

}

void fade_nodes_outer_local_party::Execute( Trigger & trigger, const Params &params)
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	if( (params.strings.empty()) || params.ints.empty() )
	{
		// Problem.  Not enough good shit.
		gpwarningf(("trigger action fade_nodes_outer_local_party was not supplied with enough parameters from %s",trigger.GetOwner()->DevGetFuelAddress().c_str()));
	}
	else
	{
		if ( !gWorldMap.ContainsRegion(MakeRegionId(params.ints[0])) )
		{
			gperrorf((
				"BAD CONTENT: Please tell Dave T that trigger [%s:0x%08x:%d] has a FADE_NODES_OUTER_LOCAL_PARTY action with an invalid REGION ID [0x%08x]\n",
				trigger.GetOwner()->GetTemplateName(),
				trigger.GetOwner()->GetScid(),
				params.SubGroup,
				params.ints[0]
				));
		}
		else
		{
			gpdevreportf( &gTriggerSysContext,("Executing [fade_nodes_outer_local_party:%s] %s\n", params.strings[0].c_str(),params.Doc.c_str()));

			// GO GO GO!
			FADETYPE ft	= FT_NONE;
			if( params.strings[0].same_no_case( "out:alpha" ) )
			{
				ft	= FT_ALPHA;
			}
			else if( params.strings[0].same_no_case( "out:black" ) )
			{
				ft	= FT_BLACK;
			}
			else if( params.strings[0].same_no_case( "out:instant" ) )
			{
				ft	= FT_INSTANT;
			}
			else if( params.strings[0].same_no_case( "in" ) )
			{
				ft	= FT_IN;
			}
			else if( params.strings[0].same_no_case( "in:instant" ) )
			{
				ft	= FT_IN_INSTANT;
			}
			else
			{
				gpwarningf(("Could not associate correct fade type with fade_nodes_outer_local_party trigger"));
			}

			Go * pParty = gServer.GetScreenParty();
			if ( pParty )
			{
				GopColl partyColl = pParty->GetChildren();

				GoOccupantMap* occs = NULL; 
				trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs);

				GoidColl members;

				for ( GopColl::iterator i = partyColl.begin(); i != partyColl.end(); ++i )
				{
					Goid membergoid = (*i)->GetGoid();

					// Is this party member NOT a trigger occupant?

					bool found = false;

					if (occs)
					{
						found = occs->find(membergoid) != occs->end();
					}

					if (!found)
					{
						members.push_back(membergoid);

						gpdevreportf( &gTriggerSysContext,("\t\t\t\tSET FADE for [%s g:0x%08x s:0x%08x]\n",
															GetGo( membergoid )->GetTemplateName(),
															membergoid,
															GetGo( membergoid )->GetScid()
															));
					}
				}

				if (!members.empty())
				{
					double fadetime = PreciseAdd(gWorldTime.GetTime(), (trigger.GetDelay() + params.Delay));
					ActionInfo * actionInfo = new ActionInfo(
											fadetime,
											ActionInfo::CT_NODE_FADE,
											params.ints[0],
											params.ints[1],
											params.ints[2],
											params.ints[3],
											ft,
											members
											);
		
					gTriggerSys.AddActionInfo( actionInfo->m_Time, actionInfo );
				}

			}
		}
	}
}


//
// fade_nodes_global( "Region ID", "Section", "Level", "Object", "Fade Type", "Fade In/Out" )
//
fade_nodes_global::fade_nodes_global()
{
	Parameter::format p0;
	p0.sName			= "Region ID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.i_default		= -1;
	p0.bSaveAsHex		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Section";
	p1.isInt			= true;
	p1.isRequired		= false;
	p1.i_default		= -1;
	m_Parameters.Add( p1 );

	Parameter::format p2;
	p2.sName			= "Level";
	p2.isInt			= true;
	p2.isRequired		= false;
	p2.i_default		= -1;
	m_Parameters.Add( p2 );

	Parameter::format p3;
	p3.sName			= "Object";
	p3.isInt			= true;
	p3.isRequired		= false;
	p3.i_default		= -1;
	m_Parameters.Add( p3 );

	Parameter::format p4;
	p4.sName			= "Fade Type";
	p4.isString			= true;
	p4.isRequired		= true;
	p4.sValues.push_back( "out:alpha" );
	p4.sValues.push_back( "out:black" );
	p4.sValues.push_back( "out:instant" );
	p4.sValues.push_back( "in" );
	p4.sValues.push_back( "in:instant" );
	m_Parameters.Add( p4 );
}

void fade_nodes_global::Execute( Trigger & trigger, const Params &params)
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));

	if( (params.strings.empty()) || params.ints.empty() )
	{
		// Problem.  Not enough good shit.
		gpwarningf(("trigger action fade_nodes_global was not supplied with enough parameters from %s",trigger.GetOwner()->DevGetFuelAddress().c_str()));
	}
	else
	{

		if ( !gWorldMap.ContainsRegion(MakeRegionId(params.ints[0])) )
		{
			gperrorf((
				"BAD CONTENT: Please tell Dave T that trigger [%s:0x%08x:%d] has a FADE_NODES_GLOBAL action with an invalid REGION ID [0x%08x]\n",
				trigger.GetOwner()->GetTemplateName(),
				trigger.GetOwner()->GetScid(),
				params.SubGroup,
				params.ints[0]
				));
		}
		else
		{
			gpdevreportf( &gTriggerSysContext,("Executing [fade_nodes_global:%s] "
											   "(%s) [0x%08x,%d,%d,%d] %s\n",
				params.strings[0].c_str(),										// Fade Type
				gWorldMap.GetRegionName(MakeRegionId(params.ints[0])).c_str(),	// Region Name String
				params.ints[0],													// Region ID
				params.ints[1],													// Section
				params.ints[2],													// Level
				params.ints[3],													// Object
				params.Doc.c_str()												// Docs
				));

			// GO GO GO!
			FADETYPE ft	= FT_NONE;
			if( params.strings[0].same_no_case( "out:alpha" ) )
			{
				ft	= FT_ALPHA;
			}
			else if( params.strings[0].same_no_case( "out:black" ) )
			{
				ft	= FT_BLACK;
			}
			else if( params.strings[0].same_no_case( "out:instant" ) )
			{
				ft	= FT_INSTANT;
			}
			else if( params.strings[0].same_no_case( "in" ) )
			{
				ft	= FT_IN;
			}
			else if( params.strings[0].same_no_case( "in:instant" ) )
			{
				ft	= FT_IN_INSTANT;
			}
			else
			{
				gpwarningf(("Could not associate correct fade type with fade_nodes trigger"));
			}

			double fadetime = PreciseAdd(gWorldTime.GetTime(), (trigger.GetDelay() + params.Delay));
			ActionInfo * actionInfo = new ActionInfo(
									fadetime,
									ActionInfo::CT_SGLOBAL_NODE_FADE,
									params.ints[0],
									params.ints[1],
									params.ints[2],
									params.ints[3],
									ft,
									GoidColl()
									);

			gTriggerSys.AddActionInfo( actionInfo->m_Time, actionInfo );
		}

/*
		gWorldTerrain.SGlobalNodeFade( params.ints[0],					// Region ID
									   params.ints[1],					// Section
									   params.ints[2],					// Level
									   params.ints[3],					// Object
									   ft );							// Fade type
*/
	}
}

//
// fade_node( "Node", "Fade Type" )
//
fade_node::fade_node()
{
	Parameter::format p0;
	p0.sName			= "Node GUID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.i_default		= -1;
	p0.bSaveAsHex		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Fade Type";
	p1.isString			= true;
	p1.isRequired		= true;
	p1.sValues.push_back( "out:alpha" );
	p1.sValues.push_back( "out:black" );
	p1.sValues.push_back( "out:instant" );
	p1.sValues.push_back( "in" );
	p1.sValues.push_back( "in:instant" );
	m_Parameters.Add( p1 );
}

void fade_node::Execute( Trigger & trigger, const Params &params)
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	if( params.strings.empty() || params.ints.empty() )
	{
		// Problem.  Not enough good shit.
		gpwarningf(("trigger action fade_node was not supplied with enough parameters from %s",trigger.GetOwner()->DevGetFuelAddress().c_str()));
	}
	else
	{
		gpdevreportf( &gTriggerSysContext,("Executing [fade_node:%s] [0x%08x] %s\n",
			params.strings[0].c_str()					,		// Fade Type
			params.ints[0],
			params.Doc.c_str()									// Docs
			));
		
		// GO GO GO!
		FADETYPE ft	= FT_NONE;
		if( params.strings[0].same_no_case( "out:alpha" ) )
		{
			ft	= FT_ALPHA;
		}
		else if( params.strings[0].same_no_case( "out:black" ) )
		{
			ft	= FT_BLACK;
		}
		else if( params.strings[0].same_no_case( "out:instant" ) )
		{
			ft	= FT_INSTANT;
		}
		else if( params.strings[0].same_no_case( "in" ) )
		{
			ft	= FT_IN;
		}
		else if( params.strings[0].same_no_case( "in:instant" ) )
		{
			ft	= FT_IN_INSTANT;
		}
		else
		{
			gpwarningf(("Could not associate correct fade type with fade_node trigger"));
		}
/*
		gWorldTerrain.SNodeFade( siege::database_guid( params.ints[0] ),	// Node GUID
								 ft );										// Fade type
*/
		double fadetime = PreciseAdd(gWorldTime.GetTime(), (trigger.GetDelay() + params.Delay));
		ActionInfo * actionInfo = new ActionInfo(
								fadetime,
								ActionInfo::CT_SNODE_FADE,
								params.ints[0],
								0,
								0,
								0,
								ft,
								GoidColl()
								);

		gTriggerSys.AddActionInfo( actionInfo->m_Time, actionInfo );
	}
}


//
// send_world_message( "event type" )
//
send_world_message::send_world_message()
{
	// Parameter 0
	Parameter::format p0;

	p0.sName			= "Message Type";
	p0.isString			= true;
	p0.isRequired		= true;
	p0.s_default		= ToString( WE_REQ_ACTIVATE );

	for( UINT32 i = 0; i != WE_COUNT; ++i )
	{
		eWorldEvent event = eWorldEvent( i );
		if ( !TestWorldEventTraits( event, WMT_SEND_ONLY | WMT_ENGINE_ONLY ) )
		{
			p0.sValues.push_back( ToString( event ) );
		}
	}
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Send to Object";
	p1.isGoid			= true;
	p1.isRequired		= true;
	p1.i_default		= MakeInt( SCID_INVALID );
	p1.bSaveAsHex		= true;

	m_Parameters.Add( p1 );

	Parameter::format p2;
	p2.sName			= "Send delay";
	p2.isFloat			= true;

	m_Parameters.Add( p2 );

	// Send context
	Parameter::format p3;
	p3.sName			= "Message broadcast";
	p3.isString			= true;
	p3.isRequired		= true;
	p3.s_default		= "default";
	p3.sValues.push_back( "default" );
	p3.sValues.push_back( "all_conditions" );

	m_Parameters.Add( p3 );

	Parameter::format p4;
	p4.sName			= "Send to";
	p4.isString			= true;
	p4.isRequired		= true;
	p4.s_default		= "every";
	p4.sValues.push_back("every");
	p4.sValues.push_back("single");

	m_Parameters.Add( p4 );

	Parameter::format p5;
	p5.sName			= "Message Qualifier";
	p5.isInt			= true;
	p5.isRequired		= false;
	p5.i_default		= 0;

	m_Parameters.Add( p5 );

}

void send_world_message::Execute( Trigger &trigger, const Params &params)
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [send_world_message:%s]\n",params.Doc.c_str()));
	if( (!params.strings.empty()) && (!params.ints.empty()))
	{

		const DWORD qualifier = (params.ints.size()<2) ? 0 : params.ints[1];

		const Goid from_goid	= trigger.GetOwner()->GetGoid();
		const Scid to_scid		= MakeScid( params.ints[0] );

		if ( to_scid != SCID_INVALID )
		{
			eWorldEvent we;
			FromString( params.strings[0], we );

			// Remember to include the both the "action delay" and the "message send delay"
			float actual_delay = trigger.GetDelay() + params.Delay + (params.floats.empty() ? 0 : (float)params.floats[0]);

			if (we==WE_INVALID)
			{
				gperrorf(("TRIGGER ERROR: [%s:0x%08x] attempted to send invalid message [%s] to [%s:0x%08x] delay(%f)\n",
					trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
					trigger.GetOwner() ? MakeInt( trigger.GetOwner()->GetScid() ) : -1,
					params.strings[0].c_str(),
					GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
					MakeInt(to_scid),
					actual_delay));

				gpdevreportf( &gTriggerSysContext,("\t...ATTEMPTED to send invalid message [%s] to [%s:0x%08x] delay(%f)\n",
					params.strings[0].c_str(),
					GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
					MakeInt(to_scid),
					actual_delay));
				return;
			}

			if ( !gServer.IsLocal() && !TestWorldEventTraits( we, WMT_IS_CLIENT_OK ) )
			{
				return;
			}

			WorldMessage msg( we, from_goid, to_scid, qualifier );

			bool single_occupant = false;
			if ((params.strings.size() > 2) && params.strings[2].same_no_case( "single" ))
			{
				single_occupant = true;
			}

			if ( (params.strings.size() < 2) || (params.strings[1].same_no_case( "default" )) )
			{
				gpdevreportf( &gTriggerSysContext,("\t...sending [%s] to [%s:0x%08x] delay(%f)\n",
					ToString(we),
					GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
					MakeInt(to_scid),
					actual_delay));
				msg.SendDelayed( actual_delay, MD_FROM_TRIGGER );
			}
			else if (params.strings[1].same_no_case( "all_conditions" ) )
			{
				// $$$ todo: need to collect all the goids in all the conditions NOT JUST THE FIRST
				GoOccupantMap* occs; 
				if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
				{
					GoOccupantMapIter it = occs->begin();
					for (;it!=occs->end();++it)
					{
						Goid occugoid = (*it).first;
						if( !GetGo( occugoid ) )
						{
							gpwarningf(("TRIGGER ERROR: [%s:0x%08x] attempted to send message [%s] to [%s:0x%08x] passing unresolved goid [0x%08x]\n",
								trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
								trigger.GetOwner() ? MakeInt( trigger.GetOwner()->GetScid() ) : -1,
								ToString(we),
								GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
								MakeInt(to_scid),
								occugoid));
						}
						else
						{
							if (single_occupant && occs->size() > 1)
							{
								// Try to avoid the packmule
								if (GetGo( occugoid )->GetInventory()->IsPackOnly())
								{
									continue;
								}
							}

							msg.SetData1( MakeInt( occugoid ) );

							gpdevreportf( &gTriggerSysContext,("\t...sending [%s] to [%s:0x%08x] occupant (%s:0x%08x) delay(%f)\n",
								ToString(we),
								GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
								MakeInt(to_scid),
								GetGo(occugoid) ? GetGo(occugoid)->GetTemplateName() : "UNKNOWN_OCCUPANT",
								occugoid,
								actual_delay));
			
							msg.SendDelayed( actual_delay, MD_FROM_TRIGGER );

							if (single_occupant)
							{
								break;
							}
						}

					}
				}
				else
				{
					gpdevreportf( &gTriggerSysContext,("\t...WHERE ARE THE OCCUPANTS!\n"));
				}
			}
			else 
			{
				WORD  condid;
				FromString(params.strings[1],condid);

				ConditionConstIter c = trigger.GetConditions().find(condid);
				
				if (c != trigger.GetConditions().end())
				{
					Condition* cond = (*c).second;
					if (cond->HasSatisfyingOccupants())
					{
						GoOccupantMap* occs = cond->GetSatisfyingOccupants();
						GoOccupantMapIter it = occs->begin();
						for (;it!=occs->end();++it)
						{
							Goid occugoid = (*it).first;
							if( !GetGo( occugoid ) )
							{
								gpwarningf(("TRIGGER ERROR: [%s:0x%08x] attempted to send message [%s] to [%s:0x%08x] passing unresolved goid [0x%08x]\n",
									trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
									trigger.GetOwner() ? MakeInt( trigger.GetOwner()->GetScid() ) : -1,
									ToString(we),
									GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
									MakeInt(to_scid),
									occugoid));
							}
							else
							{
								if (single_occupant && occs->size() > 1)
								{
									// Try to avoid the packmule
									if (GetGo( occugoid )->GetInventory()->IsPackOnly())
									{
										continue;
									}
								}

								msg.SetData1( MakeInt( occugoid ) );

								gpdevreportf( &gTriggerSysContext,("...sending [%s] to [%s:0x%08x] occupant (%s:0x%08x) delay(%f)\n",
									ToString(we),
									GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
									MakeInt(to_scid),
									GetGo(occugoid) ? GetGo(occugoid)->GetTemplateName() : "UNKNOWN_OCCUPANT",
									occugoid,
									actual_delay));

								msg.SendDelayed( actual_delay, MD_FROM_TRIGGER );

								if (single_occupant)
								{
									break;
								}
							}
						}
					}
					else
					{
						gpdevreportf( &gTriggerSysContext,("...WHERE ARE THE OCCUPANTS!\n"));
					}
				}
				else
				{
					gperrorf(( "TRIGGER ERROR: [%s:0x%08x] can't send_world_message [%s] to [%s:0x%08x]. Cannot be locate a condition with ID [0x%04x] ('%s')\n",
						trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
						trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
						ToString(we),
						GetGo(to_scid) ? GetGo(to_scid)->GetTemplateName() : "UNKNOWN_RECIPIENT",
						MakeInt(to_scid),
						condid,
						params.strings[0].c_str()
						));
				}
			}
		}
		else
		{
			gperrorf(( 
				"TRIGGER ERROR: [%s:0x%08x] can't send_world_message '%s' invalid 'send to' Scid 0x%08x "
				"Perhaps the default template value was never changed?", 
				trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
				trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
				params.strings[0].c_str(),
				params.ints[0] ));
		}
	}
}

void send_world_message::Label(const Trigger &trigger, Params &params) const
{
	// Replace the 'result of condition' parameter with the actual label
	if ( (params.strings.size() > 1) )
	{
		if (params.strings[1].same_no_case( "default" ))
		{
		}
		else if (params.strings[1].same_no_case( "all_conditions" ))
		{
		}
		else if (params.strings[1].same_no_case( "result_of_condition" ))
		{
			if (!trigger.GetConditions().empty())
			{
				ConditionConstIter c = trigger.GetConditions().begin();
				if (params.strings.size() > 2)
				{
					for (;c!=trigger.GetConditions().end();++c)
					{
						if (params.strings[2].same_no_case((*c).second->GetName()))
						{
							break;
						}
					}
				}
				if (c!=trigger.GetConditions().end())
				{
					params.strings[1].assignf("%d",(*c).first);
					params.Doc.appendf("[*labelled condition (%s)*]",(*c).second->GetName());
					params.strings[2] = "every";
				}
				else
				{
					gpstring badstr = (params.strings.size() > 2) ? params.strings[2] : "MISSING PARAM";
					gperrorf((
						"Error: A 'send_world_message action' attached to trigger [%s:0x%08x] refers to non-existant condition [%s]. "
						"Setting to [all_conditions] as a default action. Docs have been modified to refect this change",
						trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
						trigger.GetOwner() ? MakeInt(trigger.GetOwnerGoid()) : -1 ,
						badstr.c_str()));
					params.strings[1] = "all_conditions";
					params.Doc.appendf("[*NON-EXISTANT CONDITION NAME %s*]",badstr.c_str());
				}
			}
			else
			{
				params.strings[1] = "all_conditions";
			}
		}
	}
}

#if !GP_RETAIL

bool send_world_message::Validate( const Trigger & trigger, Params& params, gpstring& out_errmsg ) const
{
	
	eWorldEvent we;
	FromString( params.strings.empty() ? "" : params.strings[0], we );
	const Scid to_scid	 = MakeScid( params.ints.empty() ? -1 : params.ints[0] );
	const Scid from_scid = trigger.GetOwner() ?  trigger.GetOwner()->GetScid() : MakeScid(0);

	if (to_scid == from_scid)
	{
		 if (we != WE_REQ_USE && 
			 we != WE_REQ_ACTIVATE &&
			 we != WE_REQ_DEACTIVATE)
		 {
			out_errmsg.appendf(("CONTENT ERROR: Trigger [%s:0x%08x] cannot send [%s] to itself "
					  "You can only send WE_REQ_USE, WE_REQ_ACTIVATE and WE_REQ_DEACTIVATE recursively",
				trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "<OWNER-NOT-ASSIGNED-YET>",
				trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
				params.strings.empty() ? "MISSING_EVENT_PARAMETER" : params.strings[0]
				));
			return false;
		 }
	}

	if (we==WE_INVALID)
	{
		out_errmsg.appendf("CONTENT ERROR: Trigger [%s:0x%08x] cannot send invalid message [%s] to [0x%08x]\n",
			trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
			params.strings.empty() ? "MISSING_EVENT_PARAMETER" : params.strings[0].c_str(),
			MakeInt(to_scid)
			);
		return false;
	}

	if ( (to_scid != SCID_INVALID) && !gWorldMap.ContainsScid( to_scid ) )
	{
		out_errmsg.appendf("CONTENT ERROR: Trigger [%s:0x%08x] cannot send message [%s] to invalid SCID [0x%08x]\n",
			trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
			params.strings.empty() ? "MISSING_EVENT_PARAMETER" : params.strings[0].c_str(),
			MakeInt(to_scid)
			);
		return false;
	}
	
	if ( (params.strings.size() < 2) || (params.strings[1].same_no_case( "default" )) )
	{
	}
	else if (params.strings[1].same_no_case( "all_conditions" ) )
	{
	}
	else 
	{
		if (params.strings[1].same_no_case( "result_of_condition" ) )
		{
			Label(trigger,params);
		}
		
		WORD  condid;
		FromString(params.strings[1],condid);

		ConditionConstIter c = trigger.GetConditions().find(condid);
		
		if (c == trigger.GetConditions().end())
		{
			out_errmsg.appendf(
				"TRIGGER ERROR: [%s:0x%08x] can't send_world_message: Cannot be locate a condition with ID [0x%04x] '%s' for 'send_world_message' action on Go scid 0x%08x\n",
				trigger.GetOwner() ? trigger.GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
				trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1,
				condid,
				params.strings[0].c_str(),
				trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1 );
			return false;
		}
	}

	return true;
}

void send_world_message::Draw( Trigger & trigger, const Params &params )
{
	if( !params.strings.empty() )
	{
		GoHandle go( gGoDb.FindGoidByScid( MakeScid( params.ints[0] ) ) );
		if ( go )
		{
			gpstring label( params.strings[0] );

			float actual_delay = trigger.GetDelay() + params.Delay + (params.floats.empty() ? 0 : (float)params.floats[0]);
			if ( actual_delay != 0 )
			{
				label.appendf( " (%2.2f)", actual_delay );
			}

			DWORD color = COLOR_YELLOW;

			eWorldEvent msg;
			FromString( params.strings[0], msg );
			if ( msg == WE_REQ_ACTIVATE )
			{
				color = COLOR_GREEN;
			}
			else if ( msg == WE_REQ_DEACTIVATE )
			{
				color = COLOR_RED;
			}

			gWorld.DrawDebugArc( trigger.GetOwner()->GetPlacement()->GetPosition(), go->GetPlacement()->GetPosition(), color, label, true );
		}
	}
}
#endif


//
// call_sfx_script("script name", "*emitter"|"bone name", "script parameters")
//
call_sfx_script::call_sfx_script()
{
	// Todo: add script name drop down
	Parameter::format p0;
	p0.sName			= "SFX Script Name";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Target Info/Bone Name";
	p1.isString			= true;
	p1.isRequired		= false;
	p1.sValues.push_back( "@kill_bone" );
	m_Parameters.Add( p1 );

	Parameter::format p2;
	p2.sName			= "Script Parameters";
	p2.isString			= true;
	p2.isRequired		= false;
	m_Parameters.Add( p2 );

}


void call_sfx_script::AddTarget( def_tracker& tracker, Goid target, const Params &params )
{
	GoHandle hTarget( target );

	if( hTarget->HasAspect() )
	{
		const char* boneName = NULL;
		if( params.strings.size() >= 2 ) {
			boneName = params.strings[1];
		}

		tracker->AddGoTarget( target, boneName );
	}
	else if( same_no_case( hTarget->GetTemplateName(), "point" ) )
	{
		tracker->AddStaticTarget( hTarget->GetPlacement()->GetPosition(), vector_3::UP, 1.0 );
	}
	else
	{
		tracker->AddGoEmitter( target );
	}
}

void call_sfx_script::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [call_sfx_script:%s]", params.Doc.c_str()));

	Goid owner = trigger.GetOwnerGoid();
	GoHandle hOwner( owner );
	gpassert( hOwner );

	bool message_targets_found = false;

	// Run through messages, trying to determine some targets

	eWorldEvent type = WE_UNKNOWN;

	MessageList* messagelist;
	if (trigger.GetSubGroupSatisfyingMessages(params.SubGroup,messagelist))
	{
		for (MessageList::iterator mit = messagelist->begin(); mit != messagelist->end(); )
		{

			MessageHeader message = *mit;
			++mit;

			bool serverOnly = true;
			if ( ::TestWorldEventTraits( message.m_Event, WMT_IS_CLIENT_OK ) )
			{
				serverOnly = false;
			}

			if ( (serverOnly && !gServer.IsLocal()) || (params.strings.empty()) )
			{
				return;
			}

			// Resolve who the owner, source, and target are depending on the context from the worldmessage
			Goid source = GOID_INVALID;
			Goid target = GOID_INVALID;

			// Determine what association to assign to running this script based on whether or not there is a worldmessage
			type = message.m_Event;
			switch( type )
			{
				case WE_REQ_CAST_CHARGE:
				{
					source	= message.m_SendFrom;
					target	= message.m_SendFrom;
				} break;

				case WE_REQ_CAST:
				{
					source	= owner;
					target	= MakeGoid(message.m_Data1);

					GoHandle hSource( source );

					if( hSource && hSource->IsSpell() )
					{
						Go* pParent = hSource->GetParent();
						if ( pParent != NULL )
						{
							source = pParent->GetGoid();
						}
						else
						{
							gpdevreportf( &gTriggerSysContext,("...action aborted due to parent of spell %s (goid = 0x%08X, scid = 0x%08X) having been deleted\n",
									hSource->GetTemplateName(),
									hSource->GetGoid(),
									hSource->GetScid() ));
							return;
						}
					}
				} break;

				case WE_EQUIPPED:
				{
					Go *pParent = NULL;
					if( GoHandle( owner )->GetParent( pParent ) )
					{
						owner	= pParent->GetGoid();
						source	= message.m_SendFrom;
						target	= message.m_SendTo;
					}
				} break;

				case WE_CONSTRUCTED:
				{
					gperror( "Trigger convention violation: 'call_sfx_script' triggered by WE_CONSTRUCTED - use WE_ENTERED_WORLD intead.\n" );
					return;
				} break;


				default:
				{
					GoOccupantMap* occs; 
					if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
					{
						Goid occugoid = (*occs->begin()).first;
						source	= trigger.IsOneShot() ? occugoid : owner;
						owner	= occugoid;
						target	= occugoid;
					}
					else
					{
						source	= MakeGoid(message.m_Data1);
						target	= owner;
					}
				} break;
			}

			// Make sure there is always a source or a target even when it hasn't been specified
			if( !GoHandle( target ) || !GoHandle( source ) )
			{
				if( GoHandle( target ) && !GoHandle( source ) ) {
					source = target;
				}
				else
				{
					if( GoHandle( source ) && !GoHandle( target ) ) {
						target = source;
					}
					else
					{
						gperrorf(( "Trigger action:call_sfx_script - both target and source are invalid for '%s'\n", trigger.GetOwner()->GetTemplateName() ));
						return;
					}
				}
			}

			// Both target and source should be valid now so build the tracker for the special effect script
			def_tracker tracker = WorldFx::MakeTracker();
			AddTarget( tracker, target, params );
			AddTarget( tracker, source, params );

			// Start the special effect script
			gpdevreportf( &gTriggerSysContext,("...%s [%s:%s] own:%s:0x%08x src:%s:0x%08x tgt:%s:0x%08x",
							params.strings[0].c_str(),
							ToString(type),
							(params.strings.size() > 2) ? params.strings[2].c_str() : "",
							GetGo(owner)  ? (GetGo(owner)->GetTemplateName())  : "INVALID",
							GetGo(owner)  ? (GetGo(owner)->GetScid())          : MakeScid(0),
							GetGo(source) ? (GetGo(source)->GetTemplateName()) : "INVALID",
							GetGo(source) ? (GetGo(source)->GetScid())         : MakeScid(0),
							GetGo(target) ? (GetGo(target)->GetTemplateName()) : "INVALID",
							GetGo(target) ? (GetGo(target)->GetScid())         : MakeScid(0)
							));

			// don't run script if we don't want startup-based fx to run
			if ( !hOwner->IsNoStartupFx() || ((type != WE_CONSTRUCTED) && (type != WE_ENTERED_WORLD)) )
			{
				gWorldFx.SRunScript(
						params.strings[0],
						tracker,
						(params.strings.size() > 2) ? params.strings[2] : "",
						owner,
						type,
						hOwner->IsGlobalGo() && serverOnly );
			}
			
			message_targets_found = true;
		}
	
	}

	if (!message_targets_found) 
	{
		// We were unable to determine what to do from the messages received, trigger on occupants

		// $$$$$ This will execute multiple times, once for each occupant of the trigger

		Goid source = owner;
		Goid target = GOID_INVALID;
	
		GoOccupantMap* occs; 
		if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
		{
			gpdevreportf( &gTriggerSysContext,("\n\t...%s [X%d]", params.strings[0].c_str(), occs->size()));

			GoOccupantMapIter it = occs->begin();
			for (;it!=occs->end();++it)
			{
				Goid occugoid = (*it).first;
				if( !GetGo( occugoid ) )
				{
					gpwarningf((
						"TRIGGERSYS: trigger at scid 0x%08x, action 'call_sfx_script', couldn't resolve Goid 0x%08x."
						"A GO has appears to have left the world while still inside a trigger boundary."
						"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
						MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid ) 
						));
				}
				else
				{
					target = occugoid;

					// Make sure there is always a source or a target even when it hasn't been specified
					if( !GoHandle( target ) || !GoHandle( source ) )
					{
						if( GoHandle( target ) && !GoHandle( source ) ) {
							source = target;
						}
						else
						{
							if( GoHandle( source ) && !GoHandle( target ) ) {
								target = source;
							}
							else
							{
								gperrorf(( "Trigger action:call_sfx_script - both target and source are invalid for '%s'\n", trigger.GetOwner()->GetTemplateName() ));
								continue;
							}
						}
					}

					// Both target and source should be valid now so build the tracker for the special effect script
					def_tracker tracker = WorldFx::MakeTracker();
					AddTarget( tracker, target, params );
					AddTarget( tracker, source, params );

					// Start the special effect script
					gpdevreportf( &gTriggerSysContext,("...%s [%s:%s] own:%s:0x%08x src:%s:0x%08x tgt:%s:0x%08x",
									params.strings[0].c_str(),
									ToString(type),
									(params.strings.size() > 2) ? params.strings[2].c_str() : "",
									GetGo(owner)  ? (GetGo(owner)->GetTemplateName())  : "INVALID",
									GetGo(owner)  ? (GetGo(owner)->GetScid())          : MakeScid(0),
									GetGo(source) ? (GetGo(source)->GetTemplateName()) : "INVALID",
									GetGo(source) ? (GetGo(source)->GetScid())         : MakeScid(0),
									GetGo(target) ? (GetGo(target)->GetTemplateName()) : "INVALID",
									GetGo(target) ? (GetGo(target)->GetScid())         : MakeScid(0)
									));

					gWorldFx.SRunScript(
							params.strings[0],
							tracker,
							(params.strings.size() > 2) ? params.strings[2] : "",
							owner,
							type,
							hOwner->IsGlobalGo() );
				}
			}
		}
	}
	gpdevreportf( &gTriggerSysContext,("\n"));

}

//
// play_sound( "name" )
//

play_sound::play_sound()
{
	// Parameter 0
	Parameter::format p0;

	p0.sName			= "Name of sample to play";
	p0.isString			= true;
	p0.isRequired		= true;

	m_Parameters.Add( p0 );
}

void play_sound::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [play_sound:%s]", params.Doc.c_str()));
	UNREFERENCED_PARAMETER( trigger );

	if ( !gServer.IsLocal() )
	{
		return;
	}

	if( !params.strings.empty() )
	{
		gpdevreportf( &gTriggerSysContext,("%s", params.strings[0].c_str()));
		gSoundManager.PlaySample( params.strings[0] );
	}
	gpdevreportf( &gTriggerSysContext,("\n", params.Doc.c_str()));
}


//
// change_actor_life( value, "condition_parameter" )
//

change_actor_life::change_actor_life()
{
	// Parameter 0
	Parameter::format p0;
	p0.sName			= "Amount of life to change";
	p0.isFloat			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );

	// Parameter 2
	Parameter::format p1;
	p1.sName			= "Actor scid to change (setting to 0 changes all condition occupants)";
	p1.isInt			= true;
	p1.isRequired		= false;
	p1.bSaveAsHex		= true;
	p1.i_default		= 0;
	m_Parameters.Add( p1 );

}

void change_actor_life::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [change_actor_life] [%s]\n", params.Doc.c_str()));

	if ( !gServer.IsLocal() )
	{
		return;
	}

	if( params.floats.size() == 1 )
	{
		float delta = params.floats[0];

		GoOccupantMap* occs; 

		if (!params.ints.empty() && params.ints[0] != 0)
		{
			Goid actorGoid = MakeGoid( params.ints[0] );

			if( !GetGo( actorGoid ) )
			{
				gpwarningf((
					"TRIGGERSYS: trigger at scid 0x%08x, action 'change_actor_life', couldn't resolve Goid 0x%08x."
					"A GO has appears to have left the world while still inside a trigger boundary."
					"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
					MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( actorGoid ) 
					));
			}
			else
			{
				if ( delta < 0 )
				{
					gRules.DamageGo( actorGoid, trigger.GetOwner()->GetGoid(), trigger.GetOwner()->GetGoid(), -delta, false );
				}
				else
				{
					gRules.ChangeLife( actorGoid, delta, RPC_TO_ALL );
				}			
			}

		}
		else
		{
			if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
			{
				GoOccupantMapIter it = occs->begin();

				for (;it!=occs->end();++it)
				{
					Goid occugoid = (*it).first;
					Goid triggoid = trigger.GetOwner()->GetGoid();

					if ( !GetGo( occugoid ))
					{
						gpwarningf((
							"TRIGGERSYS: trigger at scid 0x%08x, action 'change_actor_life', couldn't resolve Goid 0x%08x."
							"A GO has appears to have left the world while still inside a trigger boundary."
							"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
							MakeInt( trigger.GetOwner()->GetScid() ) , occugoid 
							));
					}
					else
					{
						if ( delta < 0 )
						{
							gRules.DamageGo( occugoid, triggoid ,triggoid ,-delta , false );
						}
						else
						{
							gRules.ChangeLife( occugoid, delta, RPC_TO_ALL );
						}
					}
				}
			}
		}
	}
}

//
// change_actor_mana( value, "condition_parameter" )
//

change_actor_mana::change_actor_mana()
{
	// Parameter 0
	Parameter::format p0;

	p0.sName			= "Amount of mana to change";
	p0.isFloat			= true;
	p0.isRequired		= true;

	m_Parameters.Add( p0 );

	// Parameter 1
	Parameter::format p1;
	p1.sName			= "Actor scid to change (setting to 0 changes all condition occupants)";
	p1.isInt			= true;
	p1.isRequired		= false;
	p1.bSaveAsHex		= true;
	p1.i_default		= 0;
	m_Parameters.Add( p1 );

}

void change_actor_mana::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [change_actor_mana] [%s]\n", params.Doc.c_str()));

	if ( !gServer.IsLocal() )
	{
		return;
	}

	if( (params.floats.size() == 1) )
	{
		float delta = params.floats[0];

		if (!params.ints.empty() && params.ints[0] != 0)
		{
			Goid actorGoid = MakeGoid( params.ints[0] );

			if( !GetGo( actorGoid ) )
			{
				gpwarningf((
					"TRIGGERSYS: trigger at scid 0x%08x, action 'change_actor_mana', couldn't resolve Goid 0x%08x."
					"A GO has appears to have left the world while still inside a trigger boundary."
					"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
					MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( actorGoid ) 
					));
			}
			else
			{
				gRules.ChangeMana( actorGoid, delta, RPC_TO_ALL );
			}
		}
		else
		{
			GoOccupantMap* occs; 
			if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
			{
				GoOccupantMapIter it = occs->begin();

				for (;it!=occs->end();++it)
				{
					Goid occugoid = (*it).first;
					//gpassertm( GetGo( (*iGoid) ) != NULL, "Using invalid Goid, where valid one is required." );
					if( !GetGo( occugoid ) )
					{
						gpwarningf((
							"TRIGGERSYS: trigger at scid 0x%08x, action 'change_actor_mana', couldn't resolve Goid 0x%08x."
							"A GO has appears to have left the world while still inside a trigger boundary."
							"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
							MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid ) 
							));
					}
					else
					{
						gRules.ChangeMana( occugoid, delta, RPC_TO_ALL );
					}
				}
			}
		}
	}
}


//
// start_camera_fx( "Name", "Params" )
//

start_camera_fx::start_camera_fx()
{
	Parameter::format p0;
	p0.sName			= "Camera Name";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Camera Params";
	p1.isString			= true;
	p1.isRequired		= false;
	m_Parameters.Add( p1 );
}

void start_camera_fx::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [start_camera_fx] [%s]\n", params.Doc.c_str()));

	if( params.strings.empty() )
	{
		gpwarningf(( "trigger action start_camera_fx was not supplied with enough parameters from %s", trigger.GetOwner()->DevGetFuelAddress().c_str() ));
	}
	else
	{
		if ( params.strings.size() == 1 )
		{
			StartCameraFx( params.strings[0] );
		}
		else
		{
			StartCameraFx( params.strings[0], params.strings[1] );
		}
	}
}

//
// change_quest_state( "name", "state" )
//

change_quest_state::change_quest_state()
{
	Parameter::format p0;
	p0.sName			= "Quest Name";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Quest Status";
	p1.isString			= true;
	p1.isRequired		= true;
	p1.s_default		= "active";
	p1.sValues.push_back( "active" );
	p1.sValues.push_back( "inactive" );
	p1.sValues.push_back( "completed" );
	m_Parameters.Add( p1 );
		
	Parameter::format p3;
	p3.sName			= "Quest Activation Step (ignored for completed/inactive quest states)";
	p3.isInt			= true;		
	p3.isRequired		= false;		
	m_Parameters.Add( p3 );

}

void change_quest_state::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [change_quest_state:%s]", params.Doc.c_str()));

	if ( !gServer.IsLocal() )
	{
		return;
	}

	if ( params.strings.size() > 1 )
	{

		gpdevreportf( &gTriggerSysContext,(" [%s] becomes [%s] ",params.strings[0].c_str(),params.strings[1].c_str()));

		if ( params.strings[1].same_no_case( "active" ) )
		{
			int order = 0;
			if ( params.ints.size() == 1 )
			{
				order = params.ints[0];
			}

			gpdevreportf( &gTriggerSysContext,("\n"));
			gVictory.RSActivateQuest( params.strings[0], order );
		}
		else if ( params.strings[1].same_no_case( "inactive" ) )
		{
			gpdevreportf( &gTriggerSysContext,("\n"));
			gVictory.RSDeactivateQuest( params.strings[0] );
		}
		else if ( params.strings[1].same_no_case( "completed" ) )
		{
			Goid activator = GOID_INVALID;
			GoOccupantMap* occs; 
			if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
			{
				if (!occs->empty())
				{
					GoOccupantMapIter it = occs->begin();
					activator = (*it).first;
					gpdevreportf( &gTriggerSysContext,(" awarded to %s", GetGo(activator)->GetTemplateName()));
				}
			}
			gpdevreportf( &gTriggerSysContext,("\n"));
			gVictory.RSCompletedQuest( params.strings[0], activator );
		}
	}
	else
	{
		gpwarningf(( "trigger action change_quest_state was not supplied with enough parameters" ));
	}
}

//
// mood_change( "moodname" )
//

mood_change::mood_change()
{
	Parameter::format p0;
	p0.sName			= "Mood Name";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );

}

void mood_change::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [mood_change:%s] ", params.Doc.c_str()));

	if ( !params.strings.empty() )
	{
		gpdevreportf( &gTriggerSysContext,("%s\n", params.strings[0].c_str()));
		// Go through each go in the trigger field

		GoOccupantMap* occs; 
		if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
		{
			GoOccupantMapIter it = occs->begin();
			for (;it!=occs->end();++it)
			{
				Goid occugoid = (*it).first;
				if( !GetGo( occugoid ) )
				{
					gpwarningf((
						"TRIGGERSYS: trigger at scid 0x%08x, action 'mood_change', couldn't resolve Goid 0x%08x."
						"A GO has appears to have left the world while still inside a trigger boundary."
						"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
						MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid ) 
						));
				}
				else
				{
					gMood.RegisterMoodRequest( occugoid, params.strings[0] );
					gpdevreportf( &gTriggerSysContext,("\t\t\t\tCHANGE MOOD for [%s g:0x%08x s:0x%08x]\n",
														GetGo( occugoid )->GetTemplateName(),
														occugoid,
														GetGo( occugoid )->GetScid()
														));
				}
			}
		}
	}
	else
	{
		gpdevreportf( &gTriggerSysContext,("\n"));
		gpwarningf(( "trigger action mood_change was not supplied with enough parameters from %s", trigger.GetOwner()->DevGetFuelAddress().c_str() ));
	}
}


//
// victory_condition_met( "victorycondition" )
//

victory_condition_met::victory_condition_met()
{
	Parameter::format p0;
	p0.sName			= "Condition Name";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );
}

void victory_condition_met::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [victory_condition_met] [%s]\n", params.Doc.c_str()));
	UNREFERENCED_PARAMETER( trigger );
	gVictory.SSetVictoryConditionMet( params.strings[0].c_str(), true );
}


set_camera_fade_node::set_camera_fade_node()
{
	Parameter::format p0;
	p0.sName			= "Node GUID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.bSaveAsHex		= true;

	p0.i_default		= -1;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Camera Fade";
	p1.isInt			= true;
	p1.isBool			= true;
	p1.isRequired		= true;
	p1.i_default		= 0;
	m_Parameters.Add( p1 );
}

void set_camera_fade_node::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [set_camera_fade_node] [%s]\n", params.Doc.c_str()));

	UNREFERENCED_PARAMETER( trigger );

	gWorldTerrain.SSetNodeCameraFade( siege::database_guid( params.ints[0] ), params.ints[1] != 0 );
}


set_occludes_camera_node::set_occludes_camera_node()
{
	Parameter::format p0;
	p0.sName			= "Node GUID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.bSaveAsHex		= true;

	p0.i_default		= -1;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Occludes Camera";
	p1.isInt			= true;
	p1.isBool			= true;
	p1.isRequired		= true;
	p1.i_default		= 0;
	m_Parameters.Add( p1 );
}

void set_occludes_camera_node::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [set_occludes_camera_node] [%s]\n", params.Doc.c_str()));

	UNREFERENCED_PARAMETER( trigger );

	gWorldTerrain.SSetNodeOccludesCamera( siege::database_guid( params.ints[0] ), params.ints[1] != 0 );
}


set_bounds_camera_node::set_bounds_camera_node()
{
	Parameter::format p0;
	p0.sName			= "Node GUID";
	p0.isInt			= true;
	p0.isRequired		= true;
	p0.bSaveAsHex		= true;

	p0.i_default		= -1;
	m_Parameters.Add( p0 );

	Parameter::format p1;
	p1.sName			= "Bounds Camera";
	p1.isInt			= true;
	p1.isBool			= true;
	p1.isRequired		= true;
	p1.i_default		= 0;
	m_Parameters.Add( p1 );
}

void set_bounds_camera_node::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [set_bounds_camera_node] [%s]\n", params.Doc.c_str()));

	UNREFERENCED_PARAMETER( trigger );

	gWorldTerrain.SSetNodeBoundsCamera( siege::database_guid( params.ints[0] ), params.ints[1] != 0 );
}


set_player_world_location::set_player_world_location()
{
	Parameter::format p0;
	p0.sName			= "World Location";
	p0.isString			= true;
	p0.isRequired		= true;
	m_Parameters.Add( p0 );
}

void set_player_world_location::Execute( Trigger &trigger, const Params &params )
{
	gpdevreportf( &gTriggerSysContext,("Trigger [%s:0x%08x:%d] ",trigger.GetOwner()->GetTemplateName(),trigger.GetOwner()->GetScid(),params.SubGroup));
	gpdevreportf( &gTriggerSysContext,("Executing [set_player_world_location] [%s]\n", params.Doc.c_str()));

	if(!params.strings.empty())
	{
		// $$$ All Gos in the occupants list will be sent to the same position!!! -biddle

		GoOccupantMap* occs; 
		if (trigger.GetSubGroupSatisfyingOccupants(params.SubGroup,occs))
		{
			GoOccupantMapIter it = occs->begin();
			for (;it!=occs->end();++it)
			{
				Goid occugoid = (*it).first;
				if( !GetGo( occugoid ) )
				{
					gpwarningf((
						"TRIGGERSYS: trigger at scid 0x%08x, action 'set_player_world_location', couldn't resolve Goid 0x%08x."
						"A GO has appears to have left the world while still inside a trigger boundary."
						"This happens routinely on clients in a MP game, and it is handled cleanly in that situation",
						MakeInt( trigger.GetOwner()->GetScid() ), MakeInt( occugoid ) 
						));
				}
				else
				{
					Go * pGo = GetGo( occugoid );
					if ( !pGo->GetPlayer()->IsComputerPlayer() )
					{
						pGo->GetPlayer()->SSetWorldLocation( params.strings[0] );
					}
				}
			}
		}
	}
}
