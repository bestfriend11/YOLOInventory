//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpStats.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpStats.h"

#if !GP_RETAIL || GP_ENABLE_STATS

#include "AppModule.h"
#include "FuBiSchema.h"
#include "GpMath.h"
#include "ReportSys.h"
#include "StringTool.h"

#include <cmath>

//////////////////////////////////////////////////////////////////////////////
// enum eGroup implementation

static const char* s_GroupScreenStrings[] =
{
	"Idle",
	"Misc",
	"Render UI",
	"Update UI",
	"Render Effects",
	"Update Effects",
	"Physics",
	"Render Terrain",
	"Light Terrain",
	"Render Models",
	"Light Models",
	"Deform Models",
	"Animate Models",
	"Render Misc",
	"AI Query",
	"AI Misc",
	"GODB",
	"Path Find",
	"Path Follow",
	"Load Gos",
	"Load Terrain",
	"Load Texture",
	"Networking",
	"Skrit",
	"Fuel",
	"Triggers",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_GroupScreenStrings ) == GROUP_COUNT );

static stringtool::EnumStringConverter s_GroupScreenConverter(
		s_GroupScreenStrings, GROUP_BEGIN, GROUP_END );

const char* ToScreenString( eGroup e )
{
	return ( s_GroupScreenConverter.ToString( e ) );
}

////////////////////////////////////////////////////////////////////////////////
// enum eSystemProperty implementation

static const stringtool::BitEnumStringConverter::Entry s_SystemPropertyData[] =
{
	// special properties
	{ "sp_all"               , (DWORD)SP_ALL			},
	{ "sp_none"              , SP_NONE					},
	{ "sp_global"            , (DWORD)SP_GLOBAL			},

	// ai
	{ "sp_ai_mind"           , SP_AI_MIND				},
	{ "sp_ai_query"          , SP_AI_QUERY				},
	{ "sp_ai"                , SP_AI					},

	// net
	{ "sp_net_send"          , SP_NET_SEND				},
	{ "sp_net_receive"       , SP_NET_RECEIVE			},
	{ "sp_net"               , SP_NET					},

	// logic
	{ "sp_handle_message"    , SP_HANDLE_MESSAGE		},
	{ "sp_physics"           , SP_PHYSICS				},

	// effects
	{ "sp_effects_weather"   , SP_EFFECTS_WEATHER		},
	{ "sp_effects_object"    , SP_EFFECTS_OBJECT		},
	{ "sp_effects"           , SP_EFFECTS				},

	// rendering
	{ "sp_draw_terrain"      , SP_DRAW_TERRAIN			},
	{ "sp_draw_go"           , SP_DRAW_GO				},
	{ "sp_draw_calc_terrain" , SP_DRAW_CALC_TERRAIN		},
	{ "sp_draw_calc_go"      , SP_DRAW_CALC_GO			},
	{ "sp_draw"              , SP_DRAW					},

	// loading
	{ "sp_load_terrain"      , SP_LOAD_TERRAIN			},
	{ "sp_load_go"           , SP_LOAD_GO				},
	{ "sp_load"              , SP_LOAD					},

	// other
	{ "sp_skrit"             , SP_SKRIT					},
	{ "sp_fuel"              , SP_FUEL					},
	{ "sp_ui"                , SP_UI					},
	{ "sp_static_content"    , SP_STATIC_CONTENT		},
	{ "sp_misc"              , (DWORD)SP_MISC			},
	{ "sp_misc_nogroup"		 , (DWORD)SP_MISC_NOGROUP	},

	// triggers
	{ "sp_trigger_eval"      , SP_TRIGGER_EVAL			},
	{ "sp_trigger_exec"      , SP_TRIGGER_EXEC			},
	{ "sp_trigger_misc"      , SP_TRIGGER_MISC			},

	{ "godb"				 , SP_GODB					},
};

static stringtool::BitEnumStringConverter
	s_SystemPropertyConverter( s_SystemPropertyData, ELEMENT_COUNT( s_SystemPropertyData ), SP_NONE );

const char* ToString( eSystemProperty e )
{
	return ( s_SystemPropertyConverter.ToString( e ) );
}

gpstring ToFullString( eSystemProperty e )
{
	return ( s_SystemPropertyConverter.ToFullString( e ) );
}

bool FromString( const char* str, eSystemProperty& e )
{
	return ( s_SystemPropertyConverter.FromString( str, rcast <DWORD&> ( e ) ) );
}

bool FromFullString( const char* str, eSystemProperty& e )
{
	return ( s_SystemPropertyConverter.FromFullString( str, rcast <DWORD&> ( e ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// enum eSystemStage implementation

static const char* s_SystemStageStrings[] =
{
	"stage_startup",
	"stage_shutdown",
	"stage_load_game",
	"stage_save_game",
	"stage_load_map",
	"stage_front_end",
	"stage_in_game",
	"stage_user_sample",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_SystemStageStrings ) == STAGE_COUNT );

static stringtool::EnumStringConverter s_SystemStageConverter(
		s_SystemStageStrings, STAGE_BEGIN, STAGE_END );

const char* ToString( eSystemStage e )
{
	return ( s_SystemStageConverter.ToString( e ) );
}

bool FromString( const char* str, eSystemStage& e )
{
	return ( s_SystemStageConverter.FromString( str, rcast <int&> ( e ) ) );
}

//////////////////////////////////////////////////////////////////////////////

#endif // !GP_RETAIL || GP_ENABLE_STATS

#if GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// thread-specific system

DECL_THREAD DWORD g_GroupStack[ 2000 ];		// stack of groups we are within (compiler no like enums btw)
DECL_THREAD int   g_GroupStackTop;			// top of stack
DECL_THREAD DWORD g_SystemStack[ 2000 ];	// stack of systems we are within
DECL_THREAD int   g_SystemStackTop;			// top of stack

//////////////////////////////////////////////////////////////////////////////
// GpStats API implementation

namespace gpstats  {  // begin of namespace gpstats

AutoGroup :: AutoGroup( eGroup group )
{
	m_Group = group;
	EnterGroup( m_Group );
}

AutoGroup :: ~AutoGroup( void )
{
	LeaveGroup( m_Group );
}

AutoSystem :: AutoSystem( eSystemProperty system )
{
	m_System = system;
	EnterSystem( m_System );
}

AutoSystem :: ~AutoSystem( void )
{
	LeaveSystem( m_System );
}

#define ADD_COUNTER( SIZE, COUNTER )           gGpStatsMgr.AddToCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, GetCurrentSystem() )
#define INC_COUNTER( COUNTER )                 ADD_COUNTER( 1, COUNTER )
#define SUB_COUNTER( SIZE, COUNTER )           gGpStatsMgr.SubFromCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, GetCurrentSystem() )
#define DEC_COUNTER( COUNTER )                 SUB_COUNTER( 1, COUNTER )
#define SET_COUNTER( SIZE, COUNTER )           gGpStatsMgr.SetCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, GetCurrentSystem() )
#define ADD_COUNTER_SYS( SYS, SIZE, COUNTER )  gGpStatsMgr.AddToCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, SYS )
#define INC_COUNTER_SYS( SYS, COUNTER )        ADD_COUNTER_SYS( 1, COUNTER, SYS )
#define SUB_COUNTER_SYS( SYS, SIZE, COUNTER )  gGpStatsMgr.SubFromCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, SYS )
#define DEC_COUNTER_SYS( SYS, COUNTER )        SUB_COUNTER_SYS( 1, COUNTER, SYS )
#define SET_COUNTER_SYS( SYS, SIZE, COUNTER )  gGpStatsMgr.SetCounter( SIZE, GpStatsMgr::ms_SampleStats.m_##COUNTER, SYS )

bool AreStatsEnabled( void )
{
	return ( GpStatsMgr::DoesSingletonExist() && gGpStatsMgr.IsEnabled() );
}

void SetCurrentStage( eSystemStage stage )
{
	gGpStatsMgr.SetCurrentStage( stage );
}

eSystemStage GetCurrentStage( void )
{
	return ( gGpStatsMgr.GetCurrentStage() );
}

void EnterGroup( eGroup group )
{
	g_GroupStack[ g_GroupStackTop++ ] = group;
	gpassert( g_GroupStackTop != ELEMENT_COUNT( g_GroupStack ) );

	gGpStatsMgr.SetCurrentGroup( group );
}

void LeaveGroup( eGroup group )
{
	gpassert( g_GroupStackTop > 0 );
	gpassert( GetCurrentGroup() == group );

	--g_GroupStackTop;
	gGpStatsMgr.SetCurrentGroup( GetCurrentGroup() );
}

eGroup GetCurrentGroup( void )
{
	if ( g_GroupStackTop == 0 )
	{
		return ( GROUP_MISC );
	}

	return ( (eGroup)g_GroupStack[ g_GroupStackTop - 1 ] );
}

static eGroup ConvertToGroup( eSystemProperty prop )
{
	if ( prop & SP_FUEL )
	{
		return ( GROUP_FUEL );
	}
	else if ( prop & (SP_EFFECTS | SP_EFFECTS_WEATHER | SP_EFFECTS_OBJECT) )
	{
		if ( prop & SP_DRAW )
		{
			return ( GROUP_RENDER_FX );
		}
		else
		{
			return ( GROUP_UPDATE_FX );
		}
	}
	else if ( prop & SP_UI )
	{
		if ( prop & SP_DRAW )
		{
			return ( GROUP_RENDER_UI );
		}
		else
		{
			return ( GROUP_UPDATE_UI );
		}
	}
	else if ( prop & SP_DRAW_TERRAIN )
	{
		return ( GROUP_RENDER_TERRAIN );
	}
	else if ( prop & SP_DRAW_CALC_TERRAIN )
	{
		return ( GROUP_LIGHT_TERRAIN );
	}
	else if ( prop & SP_AI_QUERY )
	{
		return ( GROUP_AI_QUERY );
	}
	else if ( prop & (SP_TRIGGER_EXEC | SP_TRIGGER_EVAL | SP_TRIGGER_MISC) )
	{
		return ( GROUP_TRIGGERS );
	}
	else if ( prop & (SP_AI | SP_AI_MIND) )
	{
		return ( GROUP_AI_MISC );
	}
	else if( prop & SP_GODB )
	{
		return( GROUP_GODB );
	}
	else if ( prop & SP_LOAD_TERRAIN )
	{
		return ( GROUP_LOAD_TERRAIN );
	}
	else if ( prop & SP_LOAD_GO )
	{
		return ( GROUP_LOAD_GOS );
	}
	else if ( prop & SP_SKRIT )
	{
		return ( GROUP_AI_MISC );
	}
	else if ( prop & (SP_NET | SP_NET_SEND | SP_NET_RECEIVE) )
	{
		return ( GROUP_NET );
	}
	else if ( prop & SP_SKRIT )
	{
		return ( GROUP_SKRIT );
	}
	
	return ( GROUP_IDLE );
}

void EnterSystem( eSystemProperty system )
{
	g_SystemStack[ g_SystemStackTop++ ] = system;
	gpassert( g_SystemStackTop != ELEMENT_COUNT( g_SystemStack ) );

	eGroup group = ConvertToGroup( system );
	if ( group != GROUP_IDLE )
	{
		EnterGroup( group );
	}
}

void LeaveSystem( eSystemProperty system )
{
	eGroup group = ConvertToGroup( system );
	if ( group != GROUP_IDLE )
	{
		LeaveGroup( group );
	}

	--g_SystemStackTop;
	gpassert( g_SystemStackTop >= 0 );
}

eSystemProperty GetCurrentSystem( void )
{
	if ( g_SystemStackTop == 0 )
	{
		return ( SP_GLOBAL );
	}

	if ( GpStatsMgr::DoesSingletonExist() && gGpStatsMgr.IsMemBusy() )
	{
		return ( SP_NONE );
	}

	return ( (eSystemProperty)g_SystemStack[ g_SystemStackTop - 1 ] );
}

void AddFrame( void )
{
	gGpStatsMgr.AddFrame();
}

void AddMemAlloc( eSystemProperty system, size_t size )
{
	if ( !gGpStatsMgr.IsMemBusy() )
	{
		ADD_COUNTER_SYS( system, size, MemCurrentAllocs );
		ADD_COUNTER_SYS( system, size, MemTotalAllocs   );
	}
}

void AddMemDealloc( eSystemProperty system, size_t size, size_t waste )
{
	if ( !gGpStatsMgr.IsMemBusy() )
	{
		SUB_COUNTER_SYS( system, size,  MemCurrentAllocs );
		ADD_COUNTER_SYS( system, size,  MemTotalDeallocs );
		ADD_COUNTER_SYS( system, waste, MemTotalWaste    );
	}
}

inline int CalcDxSize( int width, int height, int bytesPerPixel, bool hasMips )
{
	int size = 0;
	do
	{
		size += (width * height);
		width >>= 1;
		height >>= 1;
	}
	while ( (width > 1) && (height > 1) && hasMips );

	return ( size * bytesPerPixel );
}

void AddDxCreateSurface( eSystemProperty system, int width, int height, int bytesPerPixel, bool hasMips )
{
	int size = CalcDxSize( width, height, bytesPerPixel, hasMips );
	ADD_COUNTER_SYS( system, size, DxCurrentSurfaceAllocs );
	ADD_COUNTER_SYS( system, size, DxTotalSurfaceAllocs );
}

void AddDxReleaseSurface( eSystemProperty system, int width, int height, int bytesPerPixel, bool hasMips )
{
	SUB_COUNTER_SYS( system, CalcDxSize( width, height, bytesPerPixel, hasMips ), DxCurrentSurfaceAllocs );
}

void AddDiyReserve( eSystemProperty system, int size )
{
	ADD_COUNTER_SYS( system, size, FileSysDiyReserves );
}

void AddDiyFree( eSystemProperty system, int size )
{
	SUB_COUNTER_SYS( system, size, FileSysDiyReserves );
}

void AddDiyCommit( int size )
{
	ADD_COUNTER( size, FileSysDiyCommits );
}

void AddFileRead( int size )
{
	ADD_COUNTER( size, FileSysDiskReads );
}

void AddFileDecompress( int size )
{
	ADD_COUNTER( size, FileSysDecompression );
}

void AddGoGlobalAlloc( void )
{
	INC_COUNTER( GoGlobalAllocs );
}

void AddGoCloneSrcAlloc( void )
{
	INC_COUNTER( GoCloneSrcAllocs );
}

void AddGoLocalAlloc( void )
{
	INC_COUNTER( GoLocalAllocs );
}

void AddGoLodfiAlloc( void )
{
	INC_COUNTER( GoLodfiAllocs );
}

void AddGoDealloc( void )
{
	INC_COUNTER( GoDeallocs );
}

#undef ADD_COUNTER
#undef INC_COUNTER
#undef SUB_COUNTER
#undef DEC_COUNTER
#undef SET_COUNTER

}  // end of namespace gpstats

//////////////////////////////////////////////////////////////////////////////
// struct GpThreadStats implementation

GpThreadStats :: GpThreadStats( DWORD threadId )
{
	m_ThreadHandle   = INVALID_HANDLE_VALUE;
	m_ThreadId       = threadId;
	m_CurrentGroup   = GROUP_MISC;
	m_LastSystemTime = 0;
	m_LastThreadTime = 0;

	InitTimings();
}

void GpThreadStats :: InitTimings( void )
{
	::ZeroObject( m_TimeAccumulators );
}

void GpThreadStats :: SetCurrentGroup( eGroup group, bool force )
{
	if ( force || (m_CurrentGroup != group) )
	{
		// get timing info
		double oldTime = m_LastSystemTime;
		m_LastSystemTime = ::GetSystemSeconds();

		// update group info
		m_TimeAccumulators[ m_CurrentGroup ] += (float)(m_LastSystemTime - oldTime);
		m_CurrentGroup = group;
	}
}

void GpThreadStats :: UpdateCurrentGroup( void )
{
	SetCurrentGroup( m_CurrentGroup, true );
}

//////////////////////////////////////////////////////////////////////////////
// class GpStatsMgr implementation

enum eGpCounterClass
{
	GPCOUNTER_FLUCTUATING,			// i just wanted to use "fluctuating" somewhere in the code - this counter grows and shrinks
	GPCOUNTER_GROW_ONLY,			// this counter only grows, but is ok to clear each frame
	GPCOUNTER_SESSION_TOTAL,		// this counter only grows, but it tracks session totals so don't clear it
	GPCOUNTER_SET_ONLY,				// this counter is set-only (not add) so no need to clear each frame
};

struct CounterTraits
{
	eGpCounterType  m_Type;			// type of counter
	int             m_Offset;		// offset from base of GpStats
	int             m_Size;			// size of counter
	eGpCounterClass m_Class;		// classification of this counter
	const char*     m_ScreenName;	// name for reporting
};

static const CounterTraits s_CounterTraits[] =
{
#	define ADD_MEMBER( NAME, CLASS )											\
		{																		\
			(eGpCounterType)GpStatsMgr::ms_SampleStats.m_##NAME.COUNTER_TYPE,	\
			offsetof( GpStats, m_##NAME ),										\
			sizeof( GpStatsMgr::ms_SampleStats.m_##NAME ),						\
			GPCOUNTER_##CLASS,													\
			#NAME,																\
		}

	ADD_MEMBER( FrameDelayTime         , SET_ONLY      ),
	ADD_MEMBER( CriticalBlockTime      , SET_ONLY      ),
	ADD_MEMBER( MemCurrentAllocs       , FLUCTUATING   ),
	ADD_MEMBER( MemTotalAllocs         , SESSION_TOTAL ),
	ADD_MEMBER( MemTotalDeallocs       , SESSION_TOTAL ),
	ADD_MEMBER( MemTotalWaste          , SESSION_TOTAL ),
	ADD_MEMBER( DxCurrentSurfaceAllocs , FLUCTUATING   ),
	ADD_MEMBER( DxTotalSurfaceAllocs   , SESSION_TOTAL ),
	ADD_MEMBER( FileSysDiyReserves     , FLUCTUATING   ),
	ADD_MEMBER( FileSysDiyCommits      , GROW_ONLY     ),
	ADD_MEMBER( FileSysDiskReads       , GROW_ONLY     ),
	ADD_MEMBER( FileSysDecompression   , GROW_ONLY     ),
	ADD_MEMBER( GoGlobalAllocs         , FLUCTUATING   ),
	ADD_MEMBER( GoCloneSrcAllocs       , FLUCTUATING   ),
	ADD_MEMBER( GoLocalAllocs          , FLUCTUATING   ),
	ADD_MEMBER( GoLodfiAllocs          , FLUCTUATING   ),
	ADD_MEMBER( GoDeallocs             , GROW_ONLY     ),

#	undef ADD_MEMBER
};

const GpStats GpStatsMgr::ms_SampleStats;

GpStatsMgr :: GpStatsMgr( void )
{
	m_Options           = OPTION_NONE;
	m_LogHistoryFrames  = 0;
	m_SpikeHoldTimeMsec = 2000;
	m_FrameTick         = ::GetTickCount();
	m_FrameTime         = ::GetSystemSeconds();
	m_FrameIndex        = 0;
	m_CurrentStage      = STAGE_STARTUP;
	m_DumpContext       = NULL;

	// this will catch when you add a counter but don't update the traits
#   if GP_DEBUG
	int size = 0;
	const CounterTraits* i, * ibegin = s_CounterTraits, * iend = ARRAY_END( s_CounterTraits );
	for ( i = ibegin ; i != iend ; ++i )
	{
		size += i->m_Size;
	}
	gpassert( size == sizeof( GpStats ) );
#   endif // GP_DEBUG
}

GpStatsMgr :: ~GpStatsMgr( void )
{
	Disable();

	delete ( m_DumpContext );
}

void GpStatsMgr :: SetLogSampleHistory( bool enable, int frameCount )
{
	RwCritical::ReadLock busy( m_MemCritical );

	if ( frameCount == 0 )
	{
		enable = false;
	}

	if ( (TestOptions( OPTION_LOG_SAMPLE_HISTORY ) != enable) || (enable && (m_LogHistoryFrames != frameCount)) )
	{
		SetOptions( OPTION_LOG_SAMPLE_HISTORY, enable );
		m_LogHistoryFrames = frameCount;
		initLogSampleHistory();
	}
}

void GpStatsMgr :: Report( ReportSys::ContextRef ctx ) const
{
	ReportSys::AutoReport autoReport( ctx );

	const Stage& stage = m_StageColl[ m_CurrentStage ];

	Stage::const_iterator i, ibegin = stage.begin(), iend = stage.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_Touched )
		{
			eSystemProperty system = MakeSystemPropertyFromIndex( i - ibegin );

			ctx->OutputF( "%15s count = %5d, last = %5d, current = %5d, max = %5d\n",
					ToString( system ),
					i->m_Stats.m_MemCurrentAllocs.m_Count,
					i->m_Stats.m_MemCurrentAllocs.m_Last,
					i->m_Stats.m_MemCurrentAllocs.m_Current,
					i->m_Stats.m_MemCurrentAllocs.m_Max );
			ctx->OutputF( "%15s last_sp = %5d, cur_sp = %5d, max_sp = %5d\n",
					"",
					i->m_Stats.m_MemCurrentAllocs.m_LastSpike,
					i->m_Stats.m_MemCurrentAllocs.m_CurrentSpike,
					i->m_Stats.m_MemCurrentAllocs.m_MaxSpike );
			ctx->OutputF( "%15s reads = %5d, decom = %5d, commits = %5d\n",
					"",
					i->m_Stats.m_FileSysDiskReads.m_Count,
					i->m_Stats.m_FileSysDecompression.m_Count,
					i->m_Stats.m_FileSysDiyCommits.m_Count );
		}
	}
}

void GpStatsMgr :: SetCurrentStage( eSystemStage stage )
{
	if ( m_CurrentStage == stage )
	{
		return;
	}

	RwCritical::ReadLock busy( m_MemCritical );

	// report on old stage?
	if ( TestOptions( OPTION_DUMP_ON_STAGE_CHANGE ) )
	{
		// build context if none
		if ( m_DumpContext == NULL )
		{
			ReportSys::LogFileSink <> * sink = new ReportSys::LogFileSink <> ( "gpstats.log" );
			sink->DisablePrefix();
			m_DumpContext = new ReportSys::LocalContext( sink, true, "Statistics", "Log" );
			m_DumpContext->SetOptions( ReportSys::Context::OPTION_IGNORETABS );
		}
		ReportSys::ContextRef ctx( m_DumpContext );
		ReportSys::AutoReport autoReport( ctx );

		// output header
		ctx->OutputF( "Stage Change\n"
					  "------------\n"
					  "\n"
					  "Old: %s\n"
					  "New: %s\n"
					  "\n"
					  "System\tCounter\tCount\tValue\tMax\tSpike\tMaxSpike\n",
					  ToString( m_CurrentStage ),
					  ToString( stage ) );
		const Stage& stage = m_StageColl[ m_CurrentStage ];

		// output everything in a table
		Stage::const_iterator i, ibegin = stage.begin(), iend = stage.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			const GpStats& stats = i->m_Stats;
			eSystemProperty system = MakeSystemPropertyFromIndex( i - ibegin );
			const char* systemName = ToString( system );

			const CounterTraits* j, * jbegin = s_CounterTraits, * jend = ARRAY_END( s_CounterTraits );
			for ( j = jbegin ; j != jend ; ++j )
			{
				ctx->OutputF( "%s\t%s\t", systemName, j->m_ScreenName );

				void* counter = (BYTE*)&stats + j->m_Offset;
				switch ( j->m_Type )
				{
					case ( GPCOUNTER_INT   ):  ((GpIntCounter  *)counter)->ReportFlat( ctx );  break;
					case ( GPCOUNTER_INT64 ):  ((GpInt64Counter*)counter)->ReportFlat( ctx );  break;
					case ( GPCOUNTER_FLOAT ):  ((GpFloatCounter*)counter)->ReportFlat( ctx );  break;

					default:
					{
						gpassert( 0 );
					}
				}

				ctx->OutputEol();
			}
		}

		ctx->OutputEol();
	}

	// select new stage
	m_CurrentStage = stage;

	// update log stats
	initLogSampleHistory();
}

void GpStatsMgr :: SetCurrentGroup( eGroup group )
{
	if ( IsEnabled() )
	{
		RwCritical::WriteLock writeLock( m_CounterCritical );
		m_CpuStats.GetStats().SetCurrentGroup( group );
	}
}

void GpStatsMgr :: AddFrame( void )
{
	// update current times
	double prevTime = m_FrameTime;
	m_FrameTick = ::GetTickCount();
	m_FrameTime = ::GetSystemSeconds();

	// update cpu time for frame delay
	SetCounter( (float)(m_FrameTime - prevTime), ms_SampleStats.m_FrameDelayTime, gpstats::GetCurrentSystem() );

	// update and clear block times
#	if KERNEL_CRITICAL_TIME_LOCKS
	float criticalBlockTime
		= kerneltool::  Critical::GetAndClearTotalBlockTime() * 0.001f
		+ kerneltool::RwCritical::GetAndClearTotalBlockTime() * 0.001f;
	SetCounter( criticalBlockTime, ms_SampleStats.m_CriticalBlockTime , gpstats::GetCurrentSystem() );
#	endif // KERNEL_CRITICAL_TIME_LOCKS

	// save the frame sample and advance
	if ( TestOptions( OPTION_LOG_SAMPLE_HISTORY ) )
	{
		// advance the frame
		if ( ++m_FrameIndex == m_LogHistoryFrames )
		{
			m_FrameIndex = 0;
		}

		// group stats
		{
			RwCritical::WriteLock writeLock( m_CounterCritical );

			stdx::fast_vector <QWORD> threadTimes;
			threadTimes.reserve( m_CpuStats.m_Stats.size() );

			// first commit the timings and get kernel timing totals
			QWORD totalThreadTime = 0;
			ThreadStatColl::iterator i, ibegin = m_CpuStats.m_Stats.begin(), iend = m_CpuStats.m_Stats.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// commit timings
				i->UpdateCurrentGroup();

				// get thread delta
				QWORD oldTime = i->m_LastThreadTime;
				QWORD dummy, kernelTime, userTime;
				::GetThreadTimes( i->m_ThreadHandle, (FILETIME*)&dummy, (FILETIME*)&dummy, (FILETIME*)&kernelTime, (FILETIME*)&userTime );
				i->m_LastThreadTime = kernelTime + userTime;
				QWORD deltaTime = i->m_LastThreadTime - oldTime;

				// track it
				totalThreadTime += deltaTime;
				threadTimes.push_back( deltaTime );
			}

			// figure out which history entry we're after
			GpGroupStats& stats = m_CpuStats.GetCurrentHistory()[ m_FrameIndex ];
			stats.Init();

			// set it up
			int index = 0;
			stats.m_TotalTime = 0;
			for ( i = ibegin ; i != iend ; ++i, ++index )
			{
				// find multiplier for this thread vs. total
				double weight = totalThreadTime ? ((double)threadTimes[ index ] / totalThreadTime) : 0;

				for ( int j = GROUP_BEGIN ; j != GROUP_END ; ++j )
				{
					// skip idle
					if ( j == GROUP_IDLE )
					{
						continue;
					}

					// calc time here
					float groupTime = (float)(i->m_TimeAccumulators[ j ] * weight);
					stats.m_Time[ j ] += groupTime;
					stats.m_TotalTime += groupTime;
				}

				if ( i->m_ThreadId == ::GetCurrentThreadId() )
				{
					// now do ratio
					stats.m_PrimaryThreadRatio = (float)weight;
				}

				// clear out timings for next frame
				i->InitTimings();
			}
		}

		// system stats
		{
			// commit
			Stage& stage = m_StageColl[ m_CurrentStage ];
			Stage::iterator i, ibegin = stage.begin(), iend = stage.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				i->CommitFrame( m_FrameIndex );
			}
		}
	}

	// clear per-frame counters
	if ( !TestOptions( OPTION_NO_CLEAR_PER_FRAME ) )
	{
		Stage& stage = m_StageColl[ m_CurrentStage ];

		Stage::iterator i, ibegin = stage.begin(), iend = stage.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GpStats& stats = i->m_Stats;

			const CounterTraits* j, * jbegin = s_CounterTraits, * jend = ARRAY_END( s_CounterTraits );
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( j->m_Class == GPCOUNTER_GROW_ONLY )
				{
					void* counter = (BYTE*)&stats + j->m_Offset;
					switch ( j->m_Type )
					{
						case ( GPCOUNTER_INT   ):  ((GpIntCounter  *)counter)->ResetExtras();  break;
						case ( GPCOUNTER_INT64 ):  ((GpInt64Counter*)counter)->ResetExtras();  break;
						case ( GPCOUNTER_FLOAT ):  ((GpFloatCounter*)counter)->ResetExtras();  break;

						default:
						{
							gpassert( 0 );
						}
					}
				}
			}
		}
	}
}

float GpStatsMgr :: FillCpuStats( GpGroupStats* avgStatsPtr, ReportSys::Context* ctx )
{
	ReportSys::TextTableSink tableSink;
	ReportSys::LocalContext tableCtx( &tableSink, false );
	if ( ctx != NULL )
	{
		tableCtx.SetSchema( new ReportSys::Schema( 6, "group", "unit", "inst", "avg", "min", "max" ), true );
	}

	GroupStatColl& history = m_CpuStats.GetCurrentHistory();
	GpGroupStats& stats = history[ m_FrameIndex ];

	// note: min and max m_Time's are ratios
	GpGroupStats avgStats, minStats = stats, maxStats = stats;

	// first collect stats
	const int HISTORY_WINDOW = min_t( 25, (int)history.size() );
	float weight = 1.0f, weightSum = 0.0f;
	for ( int frame = 0 ; frame != HISTORY_WINDOW ; ++frame )
	{
		// work backwards
		int index = m_FrameIndex - frame;
		if ( index < 0 )
		{
			index = m_LogHistoryFrames - 1;
		}
		GpGroupStats& sample = history[ index ];

		// update group timings
		for ( int i = GROUP_BEGIN ; i != GROUP_END ; ++i )
		{
			if ( i == GROUP_IDLE )
			{
				continue;
			}

			avgStats.m_Time[ i ] += sample.m_Time[ i ] * weight;

			if ( ctx != NULL )
			{
				float ratio = sample.m_TotalTime ? (sample.m_Time[ i ] / sample.m_TotalTime) : 0;
				if ( frame == 0 )
				{
					minStats.m_Time[ i ] = ratio;
					maxStats.m_Time[ i ] = ratio;
				}
				else
				{
					minimize( minStats.m_Time[ i ], ratio );
					maximize( maxStats.m_Time[ i ], ratio );
				}
			}
		}

		// update stats timings
		avgStats.m_TotalTime += sample.m_TotalTime * weight;
		avgStats.m_PrimaryThreadRatio += sample.m_PrimaryThreadRatio * weight;
		if ( ctx != NULL )
		{
			minimize( minStats.m_TotalTime, sample.m_TotalTime );
			maximize( maxStats.m_TotalTime, sample.m_TotalTime );
			minimize( minStats.m_PrimaryThreadRatio, sample.m_PrimaryThreadRatio );
			maximize( maxStats.m_PrimaryThreadRatio, sample.m_PrimaryThreadRatio );
			
		}

		// reduce weighting next time around
		weightSum += weight;
		weight *= (1.0f - (1.0f / (HISTORY_WINDOW * 0.5f)));
	}

	// output group items report
	if ( ctx != NULL )
	{
		for ( int i = GROUP_BEGIN ; i != GROUP_END ; ++i )
		{
			if ( i == GROUP_IDLE )
			{
				continue;
			}

			int inst = (int)ceil( 100.0 * stats.m_Time[ i ] / stats.m_TotalTime );
			int savg = (int)ceil( 100.0 * avgStats.m_Time[ i ] / avgStats.m_TotalTime );
			int smin = (int)ceil( 100.0 * minStats.m_Time[ i ] );
			int smax = (int)ceil( 100.0 * maxStats.m_Time[ i ] );

			if ( (inst <= 1) && (savg <= 1) && (smin <= 1) && (smax <= 1) )
			{
				continue;
			}

			ReportSys::AutoReport autoReport( tableCtx );

			tableCtx.OutputField( ::ToScreenString( (eGroup)i ) );
			tableCtx.OutputField( "%cpu" );
			tableCtx.OutputFieldObj( inst );
			tableCtx.OutputFieldObj( savg );
			tableCtx.OutputFieldObj( smin );
			tableCtx.OutputFieldObj( smax );
		}

		// add a summary
		{
			ReportSys::AutoReport autoReport( tableCtx );

			tableCtx.OutputField( "Frame Time" );
			tableCtx.OutputField( "msec" );
			tableCtx.OutputFieldObj( (int)ceil( 1000.0 * stats.m_TotalTime ) );
			tableCtx.OutputFieldObj( (int)ceil( 1000.0 * avgStats.m_TotalTime / weightSum ) );
			tableCtx.OutputFieldObj( (int)ceil( 1000.0 * minStats.m_TotalTime ) );
			tableCtx.OutputFieldObj( (int)ceil( 1000.0 * maxStats.m_TotalTime ) );
		}
		{
			ReportSys::AutoReport autoReport( tableCtx );

			tableCtx.OutputField( "Primary Thread" );
			tableCtx.OutputField( "%cpu" );
			tableCtx.OutputFieldObj( (int)ceil( 100.0 * stats.m_PrimaryThreadRatio ) );
			tableCtx.OutputFieldObj( (int)ceil( 100.0 * avgStats.m_PrimaryThreadRatio / weightSum ) );
			tableCtx.OutputFieldObj( (int)ceil( 100.0 * minStats.m_PrimaryThreadRatio ) );
			tableCtx.OutputFieldObj( (int)ceil( 100.0 * maxStats.m_PrimaryThreadRatio ) );
		}
	}

	// done
	if ( avgStatsPtr != NULL )
	{
		*avgStatsPtr = avgStats;
	}
	if ( ctx != NULL )
	{
		tableSink.OutputReport( ctx );
	}

	// return the average time for a frame
	return ( avgStats.m_TotalTime / weightSum );
}

void GpStatsMgr :: initLogSampleHistory( void )
{
	int count = TestOptions( OPTION_LOG_SAMPLE_HISTORY ) ? m_LogHistoryFrames : 0;

	m_CpuStats.InitLogSampleHistory( count );

	Stage& stage = m_StageColl[ m_CurrentStage ];
	Stage::iterator i, ibegin = stage.begin(), iend = stage.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->InitLogSampleHistory( count );
	}

	m_FrameIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////
// class GpStatsMgr::CpuStats implementation

GpStatsMgr::CpuStats :: ~CpuStats( void )
{
	ThreadStatColl::iterator i, ibegin = m_Stats.begin(), iend = m_Stats.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_ThreadHandle != INVALID_HANDLE_VALUE )
		{
			::CloseHandle( i->m_ThreadHandle );
		}
	}
}

void GpStatsMgr::CpuStats :: InitLogSampleHistory( int count )
{
	if ( count > 0 )
	{
		m_PausedHistory.resize( count );
		m_UnpausedHistory.resize( count );
	}
	else
	{
		m_PausedHistory.destroy();
		m_UnpausedHistory.destroy();
	}
}

GpThreadStats& GpStatsMgr::CpuStats :: GetStats( DWORD threadId )
{
	ThreadStatColl::iterator i, ibegin = m_Stats.begin(), iend = m_Stats.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_ThreadId == threadId )
		{
			return ( *i );
		}
	}

	m_Stats.push_back( threadId );
	GpThreadStats& stats = m_Stats.back();

	if ( threadId == ::GetCurrentThreadId() )
	{
		::DuplicateHandle( ::GetCurrentProcess(),
						   ::GetCurrentThread(),
						   ::GetCurrentProcess(),
						   &stats.m_ThreadHandle,
						   0,
						   FALSE,
						   DUPLICATE_SAME_ACCESS );
	}

	return ( stats );
}

GpStatsMgr::GroupStatColl& GpStatsMgr::CpuStats :: GetCurrentHistory( void )
{
	bool isPaused = AppModule::DoesSingletonExist() && (gAppModule.IsPaused() || gAppModule.IsUserPaused());
	return ( isPaused ? m_PausedHistory : m_UnpausedHistory );
}

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
