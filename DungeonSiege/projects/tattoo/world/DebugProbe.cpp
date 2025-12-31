/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: DebugProbe.cpp

	Author(s)	: Bartosz Kijanka

	Purpose		:

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/


#include "Precomp_World.h"

#if !GP_RETAIL

#include "DebugProbe.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GameAuditor.h"
#include "GoMind.h"
#include "Job.h"
#include "nema_aspect.h"
#include "ReportSys.h"
#include "siege_console.h"
#include "world.h"
#include "NetFubi.h"
#include "ReportSys.h"
#include "server.h"
#include "Services.h"
#include "WorldTime.h"
#include "WorldState.h"
#include "MCP.h"




DebugProbe :: DebugProbe()
{
	m_Mode		= DP_NONE;
	m_ProbeGoid	= GOID_INVALID;
	m_ProbeScid = SCID_INVALID;
	m_bVerbose	= true;
	m_fReset    = false;
}


void DebugProbe :: SetMode( eDebugProbe mode )
{
	if( mode == DP_NONE )
	{
		gOutputConsole.SetRenderFilled( false );
		gOutputConsole.SetVisibility( false );
		gOutputConsole.Clear();

		if ( m_Mode == DP_NET )
		{
			//gNetTextContext.Disable();
		}
	}
	else
	{
		gOutputConsole.SetSize( 1.0f, 1.0f );
		gOutputConsole.SetPosition( 0, 0 );
		gOutputConsole.SetVisibility( true );
		gOutputConsole.SetRenderFilled( true );
		gOutputConsole.SetFillColor( 0xD0221F70 );
		gOutputConsole.Clear();

		if ( mode == DP_NET )
		{
			//gNetTextContext.Enable();
		}
	}

	if( mode == DP_MIND && m_Mode == DP_MIND )
	{
		mode = DP_MIND2;
	}

	m_Mode = mode;

	//gServices.GetOutputConsole().SetBottomUp( m_Mode == DP_NONE );
}


void DebugProbe :: ToggleMode( eDebugProbe mode )
{
	SetMode( (m_Mode == DP_NONE) ? mode : DP_NONE );
	if (m_Mode == DP_NONE)
	    m_fReset = true;
}


void DebugProbe :: SetProbeObject( Goid object )
{
	m_ProbeGoid = object;
	m_ProbeScid = SCID_INVALID;

	if( m_ProbeGoid == GOID_INVALID )
	{
		m_Mode = DP_NONE;
	}
}


void DebugProbe :: SetProbeObject( Scid object )
{
	m_ProbeScid = object;
	m_ProbeGoid = GOID_INVALID;

	if( m_ProbeScid == SCID_INVALID )
	{
		m_Mode = DP_NONE;
	}
}


bool DebugProbe :: IsProbing()
{
	if( m_Mode == DP_NONE )
	{
		return false;
	}
	else
	{
		return true;
	}
}


void DebugProbe :: GetProbeOutput( gpstring & out )
{
	switch( m_Mode )
	{
		case DP_MIND:
			GetMind( out );
			break;

		case DP_MIND2:
			GetMind2( out );
			break;

		case DP_BODY:
			GetBody( out );
			break;

		case DP_ASPECT:
			GetAspect( out );
			break;

		case DP_NET:
		{
			static gpstring cachedProbe;
			static double lastNetProbeTime;
			if( GetGlobalSeconds() - lastNetProbeTime  > 1.0f )
			{
				cachedProbe = "";
				GetNet( cachedProbe );
				lastNetProbeTime = GetGlobalSeconds();
			}
			out = cachedProbe;
		}
		break;

		case DP_COMBAT:
			GetCombat( out );
			break;
	};

	// $$$ for some reason the last line gets dropped by the console when printing top-to-bottom
	out.append( "\n*** PROBE END\n" );
}


Go * DebugProbe :: GetProbedGo()
{
	if( m_ProbeGoid != GOID_INVALID )
	{
		GoHandle go( m_ProbeGoid );
		return go;
	}
	else if( m_ProbeScid != SCID_INVALID )
	{
		Goid probeGoid = gGoDb.FindGoidByScid( m_ProbeScid );
		GoHandle go( probeGoid );
		return go;
	}
	else
	{
		return NULL;
	}
}


void DebugProbe :: GetBanner( gpstring const & name, gpstring & out )
{
	out.appendf( "\n=-=-=-=-==-=-=-=-=-=-=-=-=-=-= %s PROBE =-=-=-=-==-=-=-=-=-=-=-=-=-=-=\n", name.c_str() );
}


void DebugProbe :: GetMind( gpstring & out )
{
	if( GetProbedGo() )
	{
		if( GetProbedGo()->HasMind() )
		{
			ReportSys::LocalContext localCont;
			ReportSys::StringSink tempSink;
			localCont.AddSink( &tempSink, false );

			GetBanner( "MIND", out );

			GetProbedGo()->GetMind()->Dump( m_bVerbose, &localCont );

			out.append( tempSink.GetBuffer() );
		}
		else
		{
			GetBanner( "MIND", out );
			out.append( "*** no mind component present\n" );
		}
	}
}


void DebugProbe :: GetMind2( gpstring & out )
{
	if( GetProbedGo() )
	{
		if( GetProbedGo()->HasMind() )
		{
			ReportSys::LocalContext localCont;
			ReportSys::StringSink tempSink;
			localCont.AddSink( &tempSink, false );

			GetBanner( "MIND", out );

			GetProbedGo()->GetMind()->Dump2( m_bVerbose, &localCont );

			out.append( tempSink.GetBuffer() );
		}
		else
		{
			GetBanner( "MIND", out );
			out.append( "*** no mind component present\n" );
		}
	}
}


void DebugProbe :: GetBody( gpstring & out )
{
	if( GetProbedGo() )
	{
		GetBanner( "BODY", out );
	
		ReportSys::LocalContext localCont;
		ReportSys::StringSink tempSink;
		localCont.AddSink( &tempSink, false );

		if( GetProbedGo()->HasBody() )
		{
			GetProbedGo()->GetBody()->Dump( true, &localCont ) ;
		}
		else
		{
			localCont.Output( "*** no body component present\n" );
		}

		GetBanner( "INVENTORY", out );

		if( GetProbedGo()->HasInventory() )
		{
			GetProbedGo()->GetInventory()->Dump( true, &localCont );
		}
		else
		{
			localCont.Output( "*** no inventory component present\n" );
		}
		out.append( tempSink.GetBuffer() );
	}
}


void DebugProbe :: GetAspect( gpstring & out )
{
	if( GetProbedGo() )
	{
		GetBanner( "ASPECT", out );
		out.append( "\n" );

		ReportSys::LocalContext localCont;
		ReportSys::StringSink tempSink;
		localCont.AddSink( &tempSink, false );
		GetProbedGo()->GetAspect()->Dump( &localCont );
		out.append( tempSink.GetBuffer() );

		if( GetProbedGo()->HasAspect() && GetProbedGo()->GetAspect()->GetAspectHandle().IsValid() )
		{
			GetProbedGo()->GetAspect()->GetAspectHandle()->DebugGetInfo(0, out );

			GetBanner( "MCP", out );

			ReportSys::LocalContext localCont;
			ReportSys::StringSink tempSink;
			localCont.AddSink( &tempSink, false );
			gMCPManager.Dump( GetProbedGo()->GetGoid(), &localCont );
			out.append( tempSink.GetBuffer() );
		}
		else
		{
			GetBanner( "ASPECT", out );
			out.append( "*** no aspect component present\n" );
		}

		//GetBody( out );
	}
}


void DebugProbe :: GetNet( gpstring & out )
{
	if( !Server::DoesSingletonExist() )
	{
		return;
	}

	gpstring title( "NET" );
	title.append( gServer.IsLocal() ? " - SERVER" : " - CLIENT" );
	GetBanner( title, out );

	if( gWorld.IsMultiPlayer() )
	{
		static double TimeToUpdate;
		static unsigned int sd, rd;

		if( TimeToUpdate < gWorldTime.GetTime() )
		{
			double TimeThen;
			sd = gNetPipe.GetBytesSentDelta( gWorldTime.GetTime(), TimeThen );
			sd = unsigned int( float(sd) / float( gWorldTime.GetTime() - TimeThen ) );

			rd = gNetPipe.GetBytesReceivedDelta( gWorldTime.GetTime(), TimeThen );
			rd = unsigned int( float(rd) / float( gWorldTime.GetTime() - TimeThen ) );

			TimeToUpdate = gWorldTime.GetTime() + 0.25;
		}

		out.appendf( "Local WorldState = %s\n", ToString( gWorldState.GetCurrentState() ) );
		out.append( "players:\n" );
	
		for( Server::PlayerColl::const_iterator i = gServer.GetPlayers().begin(); i != gServer.GetPlayers().end(); ++i )
		{
			if( !IsValid( *i ) )
			{
				continue;
			}
			out.appendf( "player '%S', avg lat = %2.3f, machineid= 0x%08x, JIP = %d, %s, %s, load = %d%%\n",	(*i)->GetName().c_str(), 
																												(*i)->GetAverageLatency(),
																												(*i)->GetMachineId(), 
																												(*i)->IsJIP(),
																												(*i)->IsLocal() ? "local" : "remote",
																												ToString( (*i)->GetWorldState() ),
																												DWORD( (*i)->GetLoadProgress() * 100.0 ) );
		}

		out.appendf( "\nsd = %d bytes/sec, rd = %d bytes/sec : st = %dK bytes, rt = %dK bytes\n\n",
						sd,
						rd,
						gNetPipe.GetTotalBytesSent()/1024,
						gNetPipe.GetTotalBytesReceived()/1024 );

		ReportSys::LocalContext localCont;
		ReportSys::StringSink tempSink;
		localCont.AddSink( &tempSink, false );

		gNetPipe.Dump( &localCont );

		gNetFuBiSend.GetDebugCallHistogram( &localCont, m_fReset );
		m_fReset = false;
		gNetFuBiSend.Dump( &localCont );
		gNetFuBiReceive.Dump( &localCont );

		out.append( tempSink.GetBuffer() );
	}
	else
	{
		out.append( "*** Not in multiplayer mode\n" );
	}
}


void DebugProbe :: GetNet2( gpstring & out )
{
	GetBanner( "NET2", out );
	out.append( "*** not implemented\n" );
}


void DebugProbe :: GetCombat( gpstring & out )
{
	static gpstring sLastOut;
	static gpstring sLastAttack;
	static Goid LastProbedGo = GOID_INVALID;
	static Goid LastDefendGo = GOID_INVALID;
	static int attackCount = 0;
	static int defendCount = 0;
	gpstring sLine;

	GetBanner( "COMBAT", out );

	AuditorDb * pCombatProbe = gGameAuditor.FindDb( "combatprobe" );
	if ( !pCombatProbe )
	{
		return;
	}


	bool bMeleeCombat = true;
	int victimId;

	if( !pCombatProbe->Get( MakeInt( GetProbedGo()->GetGoid() ), "DamageGoMelee", victimId ) )
	{
		if( pCombatProbe->Get( MakeInt( GetProbedGo()->GetGoid() ), "DamageGoMagic", victimId ) )
		{
			bMeleeCombat = false;
		}
		else
		{
			out = sLastOut;
			return;
		}
	}

	if ( LastProbedGo != GetProbedGo()->GetGoid() )
	{
		sLastAttack.clear();
		attackCount = 0;
		defendCount = 0;
	}
	else
	{
		out.append( sLastAttack );
	}

	GoHandle hVictim( MakeGoid( victimId ) );

	int attackedId;
	pCombatProbe->Get( victimId, "DamageGo", attackedId );
	GoHandle hAttacked;
	if ( MakeGoid( attackedId ) == GetProbedGo()->GetGoid() )
	{
		hAttacked = MakeGoid( attackedId );
	}

	if ( LastDefendGo != MakeGoid( victimId ) )
	{
		defendCount = 0;
		LastDefendGo = MakeGoid( victimId );
	}

	++attackCount;

	if ( hVictim )
	{
		float min, max, value;
		int probed_goid = MakeInt( GetProbedGo()->GetGoid() );
		int victim_goid = MakeInt( hVictim->GetGoid() );

		sLine.appendf( "%s ATTACKING %s (Hit #%d)", GetProbedGo()->GetTemplateName(), hVictim->GetTemplateName(), attackCount );
		if ( hAttacked )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );

			int attackNum = 0;
			pCombatProbe->Get( victim_goid, gpstringf( "attacking_go_%d", MakeInt( hAttacked->GetGoid() ) ), attackNum );
			defendCount += attackNum;

			sLine.appendf( "|  %s ATTACKING %s (Hit #%d)", hVictim->GetTemplateName(), hAttacked->GetTemplateName(), defendCount );
		}
		sLine.appendf( "\n" );

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Attack Stats
		sLine.appendf( "%s (Attacker):", GetProbedGo()->GetTemplateName() );
		if ( hAttacked )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|  %s (Attacker):", hVictim->GetTemplateName() );
		}
		sLine.appendf( "\n" );

		int killsVsVictim = 0;
		if ( pCombatProbe->Get( GE_PLAYER_KILL, MakeInt( GetProbedGo()->GetPlayerId() ), hVictim->GetTemplateName(), killsVsVictim ) )
		{
			sLine.appendf( " Kills Vs. %s = %i\n", hVictim->GetTemplateName(), killsVsVictim );
		}

		if ( pCombatProbe->Get( probed_goid, "attacker_rating", value ) )
		{
			sLine.appendf( "  Attacker Rating   : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( victim_goid, "attacker_rating", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Attacker Rating   : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( probed_goid, "victim_rating", value ) )
		{
			sLine.appendf( "  Victim Rating     : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( victim_goid, "victim_rating", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Victim Rating     : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( probed_goid, "chance_hit_before_bonus", value ) )
		{
			sLine.appendf( "  Chance To Hit     : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( victim_goid, "chance_hit_before_bonus", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Chance To Hit     : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( probed_goid, "chance_hit", value ) )
		{
			sLine.appendf( "         With Bonus : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( victim_goid, "chance_hit", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|           With Bonus : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if (pCombatProbe->Get( probed_goid, "damage_base_min", min ) && 
			pCombatProbe->Get( probed_goid, "damage_base_max", max ) )
		{
			sLine.appendf( "  Base Damage       : %2.2f to %2.2f", min, max );
		}
		if (hAttacked && pCombatProbe->Get( victim_goid, "damage_base_min", min ) && 
			pCombatProbe->Get( victim_goid, "damage_base_max", max ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Base Damage       : %2.2f to %2.2f", min, max );
		}
		sLine.appendf( "\n" );

		if( bMeleeCombat )
		{
			if (pCombatProbe->Get( probed_goid, "damage_weapon_min", min ) &&
				pCombatProbe->Get( probed_goid, "damage_weapon_max", max ) )
			{
				sLine.appendf( "  Weapon Damage     : %2.2f to %2.2f", min, max );
			}
			if (hAttacked &&pCombatProbe->Get( victim_goid, "damage_weapon_min", min ) &&
				pCombatProbe->Get( victim_goid, "damage_weapon_max", max ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|    Weapon Damage     : %2.2f to %2.2f", min, max );
			}
			sLine.appendf( "\n" );

			if (pCombatProbe->Get( probed_goid, "damage_strength_min", min ) &&
				pCombatProbe->Get( probed_goid, "damage_strength_max", max ) )
			{
				sLine.appendf( "  Strength Damage   : %2.2f to %2.2f", min, max );
			}
			if (hAttacked && pCombatProbe->Get( victim_goid, "damage_strength_min", min ) &&
				pCombatProbe->Get( victim_goid, "damage_strength_max", max ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|    Strength Damage   : %2.2f to %2.2f", min, max );
			}
			sLine.appendf( "\n" );

			if (pCombatProbe->Get( probed_goid, "damage_total_min", min ) && 
				pCombatProbe->Get( probed_goid, "damage_total_max", max ) )
			{
				sLine.appendf( "= Total Damage      : %2.2f to %2.2f", min, max );
			}
			if (hAttacked && pCombatProbe->Get( victim_goid, "damage_total_min", min ) && 
				pCombatProbe->Get( victim_goid, "damage_total_max", max ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|  = Total Damage      : %2.2f to %2.2f", min, max );
			}
			sLine.appendf( "\n" );

			if ( pCombatProbe->Get( probed_goid, "percent_blocked", value ) )
			{
				sLine.appendf( "  Percent Blocked   : %2.2f", value );
			}
			if ( hAttacked && pCombatProbe->Get( victim_goid, "percent_blocked", value ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|    Percent Blocked   : %2.2f", value );
			}
			sLine.appendf( "\n" );

			if ( pCombatProbe->Get( probed_goid, "damage_roll", value ) )
			{
				sLine.appendf( "  Damage Roll       : %2.2f", value );
			}
			if ( hAttacked && pCombatProbe->Get( victim_goid, "damage_roll", value ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|    Damage Roll       : %2.2f", value );
			}
			sLine.appendf( "\n" );

			if ( pCombatProbe->Get( probed_goid, "damage_given", value ) )
			{
				sLine.appendf( "= Damage Given      : %2.2f", value );
			}
			if ( hAttacked && pCombatProbe->Get( victim_goid, "damage_given", value ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|  = Damage Given      : %2.2f", value );
			}
			sLine.appendf( "\n" );
		}
		else
		{
			if (pCombatProbe->Get( probed_goid, "damage_spell_min", min ) &&
				pCombatProbe->Get( probed_goid, "damage_spell_max", max ) )
			{
				sLine.appendf( "  Spell Damage       : %2.2f to %2.2f", min, max );
			}
			if (hAttacked && pCombatProbe->Get( victim_goid, "damage_spell_min", min ) && 
				pCombatProbe->Get( victim_goid, "damage_spell_max", max ) )
			{
				sLine.append( max_t(0,int(40-sLine.size())), ' ' );
				sLine.appendf( "|    Spell Damage      : %2.2f to %2.2f", min, max );
			}
			sLine.appendf( "\n" );

			// From Rules::DamageGoMagic
			if (pCombatProbe->Get( probed_goid, "explosive_damage", min ) &&
				pCombatProbe->Get( probed_goid, "explosive_damage_radius", max ) )
			{
				sLine.appendf( "  Explosive Damage   : %2.2f\n", min );
				sLine.appendf( "  Radius             : %2.2f\n", max );
			}
			if ( pCombatProbe->Get( probed_goid, "victim_defense", min ) )
			{
				sLine.appendf( "  Victim defense     : %2.2f\n", min );
			}
			if ( pCombatProbe->Get( probed_goid, "damage_before_blocking", min ) )
			{
				sLine.appendf( "  Damage before block: %2.2f\n", min );
			}
			if ( pCombatProbe->Get( probed_goid, "awarded_experience", min ) )
			{
				sLine.appendf( "  Experience awarded : %2.2f\n", min );
			}
			if ( pCombatProbe->Get( probed_goid, "piercing_damage", min ) )
			{
				sLine.appendf( "  Piercing Damage    : %2.2f\n", min );
			}
			else if ( pCombatProbe->Get( probed_goid, "magic_damage", min ) )
			{
				sLine.appendf( "  Magic Damage       : %2.2f\n", min );
			}

			// From Rules::DamageGoParticle
			if ( pCombatProbe->Get( probed_goid, "particle_damage", min ) )
			{
				sLine.appendf( "  Particle damage    : %2.2f\n", min );
			}
			if ( pCombatProbe->Get( probed_goid, "particle_experience_awarded", min ) )
			{
				sLine.appendf( "  Particle XP awarded: %2.2f\n", min );
			}
		}

		if ( pCombatProbe->Get( probed_goid, "experience_gained", value ) )
		{
			sLine.appendf( "= Experience Gained  : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( victim_goid, "experience_gained", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|  = Experience Gained : %2.2f", value );
		}
		sLine.appendf( "\n" );

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Defense Stats
		sLine.appendf( "%s (Victim):", hVictim->GetTemplateName() );
		if ( hAttacked )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|  %s (Victim):", hAttacked->GetTemplateName() );
		}
		sLine.appendf( "\n%s\n", sLine.c_str() );

		if ( pCombatProbe->Get( victimId, "defense_base", value ) )
		{
			sLine.appendf( "  Base Defense      : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "defense_base", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Base Defense      : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "defense_dexterity", value ) )
		{
			sLine.appendf( "  Dexterity Defense : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "defense_dexterity", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Dexterity Defense : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "defense_equip", value ) )
		{
			sLine.appendf( "  Armor Defense     : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "defense_equip", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Armor Defense     : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "defense_total", value ) )
		{
			sLine.appendf( "= Total Defense     : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "defense_total", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|  = Total Defense     : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "life_before", value ) )
		{
			sLine.appendf( "  Life Before       : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "life_before", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Life Before       : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "life_after", value ) )
		{
			sLine.appendf( "  Life After        : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "life_after", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|    Life After        : %2.2f", value );
		}
		sLine.appendf( "\n" );

		if ( pCombatProbe->Get( victimId, "damage_taken", value ) )
		{
			sLine.appendf( "= Damage Taken      : %2.2f", value );
		}
		if ( hAttacked && pCombatProbe->Get( attackedId, "damage_taken", value ) )
		{
			sLine.append( max_t(0,int(40-sLine.size())), ' ' );
			sLine.appendf( "|  = Damage Taken      : %2.2f", value );
		}
		sLine.appendf( "\n" );
	}

	sLine.appendf( "---------------------------------------------------------------------------------\n" );
	out.append( sLine );

	sLastAttack = sLine;

	sLastOut = out;

	LastProbedGo = GetProbedGo()->GetGoid();

	pCombatProbe->Reset();
}


#endif // !GP_RETAIL
