#include "precomp_world.h"

#include "job.h"
#include "aiquery.h"
#include "contentdb.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoMind.h"
#include "GoBody.h"
#include "rapiowner.h"
#include "skritobject.h"
#include "skritsupport.h"
#include "stringtool.h"
#include "world.h"
#include "worldtime.h"
#include "GoSupport.h"
#include "ReportSys.h"

#include "mcp.h"
#include "mcp_request.h"

using namespace siege;
using namespace MCP;

FUBI_EXPORT_ENUM( eJobResult, JR_BEGIN, JR_COUNT );

//////////////////////////////////////////////////////////////////////
// eJobResult

static const char * s_JRStrings[] =
{
	"jr_invalid",
	"jr_unknown",
	"jr_ok",
	"jr_failed",
	"jr_failed_no_path",
	"jr_failed_bad_params",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_JRStrings ) == JR_COUNT );

static stringtool::EnumStringConverter s_JRConverter( s_JRStrings, JR_BEGIN, JR_END );

bool FromString( const char* str, eJobResult & value )
{
	return ( s_JRConverter.FromString( str, rcast <int&> ( value ) ) );
}

const char* ToString( eJobResult value )
{
	return ( s_JRConverter.ToString( value ) );
}

FUBI_DECLARE_ENUM_TRAITS( eJobResult, JR_INVALID );


//////////////////////////////////////////////////////////////////////////////
//	skrit-side event declarations

DECLARE_EVENT
{
	FUBI_EXPORT void OnWorldMessage( Skrit::Object* skrit, eWorldEvent, WorldMessage const * )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnWorldMessage, skrit );  }

	FUBI_EXPORT void OnCCWorldMessage( Skrit::Object* skrit, eWorldEvent, WorldMessage const * )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnCCWorldMessage, skrit );  }

	FUBI_EXPORT void OnJobInit( Skrit::Object* skrit, Job * )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnJobInit, skrit );  }

	FUBI_EXPORT void OnJobInitPointers( Skrit::Object* skrit, Job * )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnJobInitPointers, skrit );  }
}


//////////////////////////////////////////////////////////////////////////////
// class Job implementation

DWORD Job :: m_GlobalInstanceCount = 1;


////////////////////////////////////////////////////////////////////////////////
//	
Job :: Job()
{
	InitData();
}


////////////////////////////////////////////////////////////////////////////////
//	
Job :: Job( GoMind * pMind, eJobAbstractType jat )
{
	InitData();
	m_Jat = jat;

	GPPROFILERSAMPLE( "Job :: Job", SP_AI );
	CHECK_SERVER_ONLY;

	m_pMind = pMind;
	gpassert( m_pMind );

	//hSkrit.CreateSkrit( Skrit::CreateReq( sSkritPath ) );
	{
		GPPROFILERSAMPLE( "Job :: Job - clone job skrit", SP_AI );
		Skrit::CloneReq req( *pMind->GetJobSkritCloneSource( m_Jat ) );
		req.m_CloneGlobals = true;
		hSkrit.CloneSkrit( req );
		m_pSkrit = hSkrit.Get();
	}

	if( m_pSkrit == NULL )
	{
		gpwarningf(( "[%2.4f,0x%08x] Job :: Job - Could not instantiate Skrit %s\n", gWorldTime.GetTime(), gWorldTime.GetSimCount(), ::ToString( jat ) ));
	}

	m_InitPosition = SiegePos::INVALID;
	m_Id = m_GlobalInstanceCount;
	++m_GlobalInstanceCount;

	gpassert( !m_Initialized );
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: InitData()
{
	m_Id					= 0;
	m_pMind					= NULL;
	m_pSkrit				= NULL;
	m_CleaningUp			= false;
	m_MarkedForDeletion		= false;
	m_Initialized			= false;
	m_InUpdateScope			= false;
	m_WatchingGoalMessages	= false;
	m_EndRequested			= false;
	m_RegisteredWithEngaged	= false;
	m_ShuttingDown			= false;

	m_Jat					= JAT_INVALID;
	m_Traits				= JT_NONE;
	m_StartedBySECommand	= SCID_INVALID;
	m_QueueIconGuid			= GOID_INVALID;
	m_GoalPosition			= SiegePos::INVALID;
	m_GoalOrientation		= Quat::INVALID;
	m_GoalObject			= GOID_INVALID;
	m_GoalModifier			= GOID_INVALID;
	m_GoalSlot				= ES_NONE;
	m_Timer					= 0.0;
	m_Int1					= 0;
	m_Int2					= 0;
	m_Float1				= 0;

	m_JobResult				= JR_INVALID;
	m_Notify				= GOID_INVALID;
	m_TimeEngagedLastVisible= DBL_MAX;
	m_TimeToExitAtomicState	= 0;

#	if !GP_RETAIL
	m_DebugTimerNotAllowedToExpireCount = 0;
	m_LastUpdated						= 0;
#	endif
}


////////////////////////////////////////////////////////////////////////////////
//	
Job :: ~Job()
{
	if( m_RegisteredWithEngaged )
	{
		gperrorf(( "Job %s being destroyed without reg/unreg engaged mismatch.  Go = %s, scid = 0x%08x", ::ToString( m_Jat ), GetGo()->GetTemplateName(), GetGo()->GetScid() ));
	}

	if( m_Initialized )
	{
		gperrorf(( "Job %s being destroyed without being deinitialized.  Go = %s, scid = 0x%08x", ::ToString( m_Jat ), GetGo()->GetTemplateName(), GetGo()->GetScid() ));
	}
/*
#	if !GP_RETAIL
	if( GetGo()->IsHuded() )
	{
		gpgenericf(( "[%2.4f, 0x%08x] Job :: ~Job - actor '%s', job '%s', id %d\n", gWorldTime.GetTime(), gWorldTime.GetSimCount(), GetGo()->GetTemplateName(), ToString( m_Jat ), m_Id ));
	}
#	endif
*/
	// Kill my icon object (if there is one)
	SetQueueIconGoid( GOID_INVALID );
}


////////////////////////////////////////////////////////////////////////////////
//	
Go * Job :: GetGo( void ) const
{
	return( m_pMind->GetGo() );
}


////////////////////////////////////////////////////////////////////////////////
//	
Goid Job :: GetGoid( void )
{
	return( GetGo()->GetGoid() );
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: SetTraits( eJobTraits traits )
{
	gpassert( traits != JT_NONE );
	m_Traits = traits;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: Update()
{
	GPPROFILERSAMPLE( "Job :: Update", SP_AI );

	CHECK_SERVER_ONLY;

	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( !m_InUpdateScope );

	m_InUpdateScope = true;

	if( !IsInitialized() || IsMarkedForDeletion() )
	{
		m_InUpdateScope = false;
		return;
	}

#	if !GP_RETAIL
	if( !Validate() )
	{
		m_InUpdateScope = false;
		return;
	}
#	endif

	PrivateUpdate();

	m_InUpdateScope = false;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: PrivateUpdate()
{
	//----- update goals

#	if !GP_RETAIL
	m_LastUpdated = gWorldTime.GetSimCount();
#	endif // !GP_RETAIL

	// check for timer timeout
	if( !IsZero( float(m_Timer) ) )
	{
		if( m_Timer <= gWorldTime.GetTime() )
		{
			m_Timer = 0.0;
			WorldMessage SelfTimeoutMessage( WE_JOB_TIMER_DONE, GOID_INVALID, GetGoid() );
			PrivateHandleMessage( SelfTimeoutMessage );
		}
	}

	// poll for state change
	if ( m_pSkrit != NULL )
	{
		GPPROFILERSAMPLE( "Job :: PrivateUpdate - skrit update", SP_AI | SP_SKRIT );
		m_pSkrit->Poll( gWorldTime.GetSecondsElapsed(), true );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: SetTraitInterruptable( bool flag )
{
	if( flag )
	{
		SetTraits( m_Traits | JT_AUTO_INTERRUPTABLE );
	}
	else
	{
		SetTraits( m_Traits & NOT( JT_AUTO_INTERRUPTABLE ) );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: SetTraitUseSpatialSensors( bool flag )
{
	if( flag )
	{
		SetTraits( m_Traits | JT_USE_SPATIAL_SENSORS );
	}
	else
	{
		SetTraits( m_Traits & NOT( JT_USE_SPATIAL_SENSORS ) );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: MarkForDeletion( eJobResult jr )
{
	m_MarkedForDeletion	= true;
	m_JobResult			= jr;

	if( m_WatchingGoalMessages && ( GetTraits() & JT_WATCH_GOAL_MESSAGES ) )
	{
		GoHandle watched( m_GoalObject );
		if( watched )
		{
			gGoDb.StopWatching( GetGo()->GetGoid(), m_GoalObject );
		}
		m_WatchingGoalMessages = false;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: RequestEnd()
{
	m_EndRequested = true;
	WorldMessage msg( WE_REQ_JOB_END, GetGoid() );
	HandleMessage( msg );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool Job :: EndRequested()
{
	return m_EndRequested;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: MarkForDeletion()
{
	MarkForDeletion( JR_UNKNOWN );
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: Init()
{
	GPPROFILERSAMPLE( "Job :: Init", SP_AI );

	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( !IsMarkedForDeletion() );
	gpassert( !m_ShuttingDown );

	bool previousUpdateScope = m_InUpdateScope;
	m_InUpdateScope = true;

	if( !m_Initialized && (m_pSkrit != NULL) )
	{
		bool success = true;

#		if !GP_RETAIL
		if( GetMind()->GetGo()->IsHuded() )
		{
			gpgenericf(( "[%2.4f, 0x%08x] Job :: Init - Job '%s', id:0x%08x.\n", gWorldTime.GetTime(), gWorldTime.GetSimCount(), ::ToString( GetJobAbstractType() ), GetId() ));
		}
#		endif // !GP_RETAIL

		////////////////////////////////////////
		//	set and validate params

		GoHandle goalObject( m_GoalObject );

		if( ( GetTraits() & JT_REQUIRE_GOAL ) && (m_GoalObject != GOID_NONE) )
		{
			if( !goalObject )
			{
#				if !GP_RETAIL
				gpwarningf(( "[%2.4f, 0x%08x] Go '%s', job :: Init - Job '%s', id:0x%08x.  Failed to init because goal object no longer exists.\n", 
								gWorldTime.GetTime(), 
								gWorldTime.GetSimCount(), 
								GetGo()->GetTemplateName(),
								::ToString( GetJobAbstractType() ), 
								GetId() ));
#				endif // !GP_RETAIL
				success = false;
			}
			else if( !GetMind()->CanOperateOn( goalObject ) )
			{
#				if !GP_RETAIL
				gpwarningf((	"[%2.4f, 0x%08x] Job :: Init - Go '%s', job '%s', id:0x%08x.  Failed to init due !CanOperateOn( goalObject )\n",
								gWorldTime.GetTime(),
								gWorldTime.GetSimCount(),
								GetGo()->GetTemplateName(),
								::ToString( GetJobAbstractType() ), GetId() ));
#				endif // !GP_RETAIL
				success = false;
			}
		}

		if( ( GetTraits() & JT_REQUIRE_POSITION ) )
		{
			if( !gSiegeEngine.IsNodeInAnyFrustum( m_GoalPosition.node ) )
			{
#				if !GP_RETAIL
				gpwarningf(( "[%2.4f, 0x%08x] Job :: Init - Job '%s', id:0x%08x.  Failed to init due to goal position being outside any frustum\n", gWorldTime.GetTime(), gWorldTime.GetSimCount(), ::ToString( GetJobAbstractType() ), GetId() ));
#				endif // !GP_RETAIL
				success = false;
			}
		}

		////////////////////////////////////////
		//	init

		if( success )
		{
			if( !m_WatchingGoalMessages && (GetTraits() & JT_WATCH_GOAL_MESSAGES) )
			{
				if( goalObject )
				{
					gGoDb.StartWatching( GetGo()->GetGoid(), GetGoalObject() );
					m_WatchingGoalMessages = true;
				}
			}

			if( goalObject )
			{
				if( goalObject->HasActor() )
				{
					goalObject->GetMind()->RegisterEngagedMe( GetGo()->GetGoid() );
					m_RegisteredWithEngaged = true;
				}
			}

			SetInitPosition( GetGo()->GetPlacement()->GetPosition() );

			/////////////////////////////////////////
			//	create waypoint icon

			if( GetTraits() & JT_ALWAYS_CREATE_ICON )
			{
				GPPROFILERSAMPLE( "Job :: Init - create waypoint", SP_AI );

				GoCloneReq cloneReq( gContentDb.GetDefaultWaypointMovingTemplate() );
				cloneReq.SetStartingPos( GetGo()->GetPlacement()->GetPosition() );
				Goid icon = gGoDb.CloneLocalGo( cloneReq );
				SetQueueIconGoid( icon );
				GoHandle pIcon( icon );

#				if !GP_RETAIL
				gpstring iconName;
				iconName.assignf( "waypoint '%s'", ToString( GetJobAbstractType() ) );
				pIcon->GetCommon()->SetScreenName( ::ToUnicode( iconName ) );
#				endif // !GP_RETAIL
			}

			////////////////////////////////////////
			//	init skrit

			if( ( GetTraits() & JT_REQUIRE_POSITION ) != 0 )
			{
				GoHandle icon( m_QueueIconGuid );
				if( icon )
				{
					icon->GetPlacement()->SetPosition( m_GoalPosition );
				}
			}

			{
				GPPROFILERSAMPLE( "Job :: Init - init Skrit", SP_AI );
				FuBiEvent::OnJobInit( m_pSkrit, this );
				m_Initialized = true;
			}
		}
		else
		{
			MarkForDeletion( JR_FAILED_BAD_PARAMS );
			m_Initialized = false;
		}
	}

	m_InUpdateScope = previousUpdateScope;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: InitPointers()
{
	FuBiEvent::OnJobInitPointers( m_pSkrit, this );
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: Deinit( bool onAppShutdown )
{
	GPPROFILERSAMPLE( "Job :: Deinit", SP_AI );

	CHECK_PRIMARY_THREAD_ONLY;

	if( InUpdateScope() )
	{
		return;
	}

	if( m_Initialized )
	{
		gpassert( !m_ShuttingDown );
		m_ShuttingDown = onAppShutdown;

		if( !onAppShutdown )
		{
			GoHandle goalObject( m_GoalObject );

			if( m_WatchingGoalMessages && ( GetTraits() & JT_WATCH_GOAL_MESSAGES ) )
			{
				m_WatchingGoalMessages = false;
				if( goalObject )
				{
					gGoDb.StopWatching( GetGo()->GetGoid(), GetGoalObject() );
				}
			}

			if( m_RegisteredWithEngaged )
			{
				gpassert( ( GetTraits() & JT_REQUIRE_GOAL ) != 0 );

				if( goalObject )
				{
					if( goalObject->HasMind() )
					{
						goalObject->GetMind()->UnregisterEngagedMe( GetGo()->GetGoid() );
					}
				}
				m_RegisteredWithEngaged = false;
			}
		}
		m_Initialized	= false;
		m_InitPosition	= SiegePos::INVALID;
	}

	gpassert( !m_WatchingGoalMessages );
	gpassert( !m_RegisteredWithEngaged );
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: HandleMessage( WorldMessage const & Message )
{
	GPPROFILERSAMPLE( "Job :: HandleMessage", SP_AI | SP_HANDLE_MESSAGE );

	CHECK_SERVER_ONLY;

	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( !m_InUpdateScope );

	m_InUpdateScope = true;

	if( !IsInitialized() )
	{
		m_InUpdateScope = false;
		return;
	}

	////////////////////////////////////////
	//	sanity

#	if !GP_RETAIL
	if( !Validate() )
	{
		m_InUpdateScope = false;
		return;
	}
#	endif

	PrivateHandleMessage( Message );

	m_InUpdateScope = false;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: PrivateHandleMessage( WorldMessage const & Message )
{
	////////////////////////////////////////
	//	MCP messages and ANIMATION messages must only be processed by intended Job

	if( !Message.IsCC() && (IsMCPMessage( Message.GetEvent() ) ) )
	{
		if( GetJobAbstractType() == JAT_BRAIN )
		{
		// brain quietly ignores MCP commands
		// return;
		}
		else if( !gMCP.MessagesAreInSync( GetGo()->GetGoid(), (MCP::eRID)Message.GetData1(), Message.GetEvent() ) )
		{
#			if !GP_RETAIL

			gpreportf("aimove",("OUTOFSYNC: Job [%s:0x%08x] for Go [%s] received message [%s] @ [%2.4f, 0x%08x]\n",
				::ToString( GetJobAbstractType() ),
				GetId(),
				GetGo()->GetTemplateName(),
				::ToString( Message.GetEvent() ),
				gWorldTime.GetTime(),
				gWorldTime.GetSimCount()
				));

			if( GetGo()->IsHuded() )
			{
				gpwarningf(( "[%2.4f, 0x%08x] Job [%s:0x%08x] for Go [%s] received OUT-OF-SYNC message [%s]\n",
					gWorldTime.GetTime(),
					gWorldTime.GetSimCount(),
					::ToString( GetJobAbstractType() ),
					GetId(),
					GetGo()->GetTemplateName(),
					::ToString( Message.GetEvent() )
					));
			}
#			endif // !GP_RETAIL
			return;
		}
	}

	////////////////////////////////////////
	//	make sure job is inited before processing message
	//CheckInitialized();

	if( m_pSkrit != NULL )
	{
		GPPROFILERSAMPLE( "Job :: PrivateHandleMessage - skrit calls", SP_AI | SP_SKRIT );

		if( !Message.IsCC() )
		{
			FuBiEvent::OnWorldMessage( m_pSkrit, Message.GetEvent(), &Message );
		}
		else
		{
			FuBiEvent::OnCCWorldMessage( m_pSkrit, Message.GetEvent(), &Message );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
gpstring Job :: GetName( void )
{
	return gpstring( ::ToString( GetJobAbstractType() ));
}


////////////////////////////////////////////////////////////////////////////////
//	
bool Job :: IsOffensive()
{
	return ( m_Traits & JT_OFFENSIVE ) != 0;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool Job :: IsDefensive()
{
	return ( m_Traits & JT_DEFENSIVE ) != 0;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool Job :: Validate()
{
	bool success = true;

	if( GetTraits() & JT_REQUIRE_POSITION  )
	{
		if( !gSiegeEngine.IsNodeInAnyFrustum( GetGoalPosition().node ) )
		{
			gpwarningf(( 	"Job :: Validate() - '%s' for Go '%s', Scid 0x%08x - has goal position set to a node that isn't loaded.\n",
							GetName().c_str(),
							GetMind()->GetGo()->GetTemplateName(),
							GetMind()->GetGo()->GetScid() ));
			success = false;
		}
	}
	else if( GetTraits() & JT_REQUIRE_GOAL )
	{
		GoHandle goal( GetGoalObject() );
		if( goal )
		{
			if( !goal->IsInAnyWorldFrustum() )
			{
				gpwarningf(( 	"Job :: Validate() - '%s' for Go '%s', Scid 0x%08x - goal object = %s, goal object which is not in any frustum.\n",
								GetName().c_str(),
								GetMind()->GetGo()->GetTemplateName(),
								GetMind()->GetGo()->GetScid(),
								goal->GetTemplateName()
							));
				success = false;
			}
		}
	}

	if( !success )
	{
		gpwarningf(( 	"Job :: Validate() - '%s' for Go '%s', Scid 0x%08x - has lost face.  Commiting self-deletion.\n",
						GetName().c_str(),
						GetMind()->GetGo()->GetTemplateName(),
						GetMind()->GetGo()->GetScid() ));
		MarkForDeletion( JR_FAILED_BAD_PARAMS );
	}

	return success;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: SetGoalObject( Goid go )
{
	gpassert( m_GoalObject == GOID_INVALID );
	gpassert( go != GOID_INVALID );
	gpassert( (GetGoidClass( go ) == GO_CLASS_GLOBAL) || (GetGoidClass( go ) == GO_CLASS_LOCAL) || (go == GOID_NONE) );
	
	m_GoalObject = go;
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: SetGoalModifier( Goid go )
{
	gpassert( m_GoalModifier == GOID_INVALID );
	gpassert( go != GOID_INVALID );
	gpassert( (GetGoidClass( go ) == GO_CLASS_GLOBAL) || (GetGoidClass( go ) == GO_CLASS_LOCAL) || (go == GOID_NONE) );

	m_GoalModifier = go;
}


////////////////////////////////////////////////////////////////////////////////
//	
eActionOrigin Job :: GetOrigin()
{
	if( GetTraits() & JT_HUMAN_ASSIGNED )
	{
		return AO_USER;
	}
	else if( GetTraits() & JT_REFLEX_ASSIGNED )
	{
		return AO_REFLEX;
	}
	else if( GetTraits() & JT_COMMAND_ASSIGNED )
	{
		return AO_COMMAND;
	}
	else
	{
		return AO_INVALID;
	}
}


bool Job :: IsUserAssigned()
{
	return( (GetTraits() & JT_HUMAN_ASSIGNED) != 0 );
}


bool Job :: InAtomicState()
{
	return ( m_Traits & JT_IN_ATOMIC_STATE ) != 0;
}


void Job :: EnterAtomicState( float duration )											
{
	SetTraits( m_Traits | JT_IN_ATOMIC_STATE );
	m_TimeToExitAtomicState = PreciseAdd( gWorldTime.GetTime(), duration );
	m_pMind->OnJobEnterAtomicState( this, duration );
}


void Job :: LeaveAtomicState()											
{ 
	SetTraits( m_Traits & NOT( JT_IN_ATOMIC_STATE ) );

#	if !GP_RETAIL
	if( GetGo()->IsHuded() )
	{
		gpwarningf((	"[%2.4f, 0x%08x] Job [%s:0x%08x] for Go [%s] left atomic state with a %2.2f second error margin\n",
						gWorldTime.GetTime(),
						gWorldTime.GetSimCount(),
						::ToString( GetJobAbstractType() ),
						GetId(),
						GetGo()->GetTemplateName(),
						PreciseSubtract( m_TimeToExitAtomicState, gWorldTime.GetTime() ) ));
	}
#	endif
}


#if !GP_RETAIL


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: DrawDebugHud()
{
	//gpassert( !IsMarkedForDeletion() );
/*
	if( GetGo()->GetMind()->GetMovementOrders() == MO_LIMITED  )
	{
		gWorld.DrawDebugSphere( GetCreationPosition(), GetLeashRange(), MAKEDWORDCOLOR( vector_3(0.3f,0.4f,0.8f) ), "leash range" );
	}
*/

/*
	gpstring label;
	label.assignf( "to %s creation pos", ::ToString( GetJobAbstractType() ) );
	gWorld.DrawDebugLinkedTerrainPoints( GetGo()->GetPlacement()->GetPosition(), GetCreationPosition(), COLOR_LIGHT_BLUE, label );
*/
	if( ( GetJobAbstractType() != JAT_BRAIN ) &&  m_Timer > gWorldTime.GetTime() )
	{
		gWorld.DrawDebugTriangle( GetGo()->GetPlacement()->GetPosition(), 1.0, COLOR_LIGHT_YELLOW, "job timer" );
	}
}
#endif // !GP_RETAIL


////////////////////////////////////////////////////////////////////////////////
//	
bool Job :: GetFocusPosition( SiegePos & pos )
{
	SiegePos focusPos = SiegePos::INVALID;

	if( GetTraits() & JT_REQUIRE_POSITION )
	{
		focusPos = GetGoalPosition();
	}

	if( !gSiegeEngine.IsNodeInAnyFrustum( focusPos.node ) || ( GetTraits() & JT_REQUIRE_GOAL ) )
	{
		GoHandle target( GetGoalObject() );
		if( target )
		{
			focusPos = target->GetPlacement()->GetPosition();
		}
	}

	if( gSiegeEngine.IsNodeInAnyFrustum( focusPos.node ) )
	{
		pos = focusPos;
		return( true );
	}
	else
	{
		return( false );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: DrawWaypointIcon()
{
}


////////////////////////////////////////////////////////////////////////////////
//	
void Job :: HideWaypointIcon()
{
	GoHandle go( GetQueueIconGoid() );
	if( go && go->HasAspect())				// -- $$$$TODO ask bart about this !!!!
	{
		go->GetAspect()->SetIsVisible( false );
	}
}


SiegePos const & Job :: GetGoalPosition() const
{
	// todo: change; right now the position is the GoalObject position, if not, it's the GoalPosition -BK
	GoHandle target( m_GoalObject );

	if( target )
	{
		return( target->GetPlacement()->GetPosition() );
	}
	else {
		return( m_GoalPosition );
	}
}


void Job :: SetGoalPosition( SiegePos const & pos )
{
	m_GoalPosition = pos;
}


void Job :: SetQueueIconGoid( Goid icon_guid )
{
	// release old
	if ( m_QueueIconGuid != GOID_INVALID )
	{
		gGoDb.SMarkForDeletion( m_QueueIconGuid );
	}

	// take new
	m_QueueIconGuid = icon_guid;
}


void Job :: SetTimer( float Delay )
{
#	if !GP_RETAIL
	if( m_Timer != 0.0 ) {
		++m_DebugTimerNotAllowedToExpireCount;
	}
#	endif
	m_Timer = PreciseAdd( gWorldTime.GetTime(), double( Delay ));
}


void Job :: ClearTimer()
{
	m_Timer = 0.0;
#	if !GP_RETAIL
	m_DebugTimerNotAllowedToExpireCount = 0;
#	endif
}


#if !GP_RETAIL
void Job :: Dump( bool verbose, ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	///////////////////////////////////
	//	skrit container object debug info

	GoHandle hGoalObject( GetGoalObject() );
	GoHandle hGoalModifier( GetGoalModifier() );

	ctx->Indent();

	ctx->Output( "--------------------------------------------------------------------------------------\n" );

	gpstring jobName( ::ToString( GetJobAbstractType() ) );
	jobName.to_upper();

	ctx->OutputF( "[%d %s %s] 0x%08x Job '%s' - origin= %s\n",
					m_Id,
					IsInitialized() ? "I" : " ",
					IsMarkedForDeletion() ? "DEL" : "---",
					m_LastUpdated,
					jobName.c_str(),
					::ToString( GetOrigin() ) );

	ctx->OutputF( "goal = %s, gmod = %s, gpos = %s\n",
					hGoalObject.IsValid() ? hGoalObject->GetTemplateName() : "n/a",
					hGoalModifier.IsValid() ? hGoalModifier->GetTemplateName() : "n/a",
					::ToString( GetGoalPosition() ).c_str() );

	if( verbose )
	{
		ctx->OutputF(	"job timer = %2.1f, travel dist = %2.2f\n",
						( m_Timer > gWorldTime.GetTime() ) ? m_Timer - gWorldTime.GetTime() : 0,
						GetJobTravelDistance() );

		ctx->OutputF(	"traits = 0x%08x, inatomicstate = %d, jt_auto_interruptable = %d, jt_use_spatial_sensors = %d\n",
						GetTraits(),
						InAtomicState(),
						( GetTraits() & JT_AUTO_INTERRUPTABLE ) != 0 ,
						( GetTraits() & JT_USE_SPATIAL_SENSORS ) != 0 );

		///////////////////////////////////
		//	skrit debug info

		// now let the skrit up it's info:
		ctx->Output( "[] skrit object dump:\n" );

		gpstring sTemp;
		m_pSkrit->DumpStateMachine( sTemp );

		ctx->Indent();
		ctx->Output( sTemp );
		ctx->Outdent();
	}

	ctx->Outdent();
}
#endif


void Job :: TakesOver( Job * oldJob )
{
	SetNotify( oldJob->GetNotify() );
	oldJob->SetNotify( GOID_INVALID );

	SetTraitInterruptable( ( oldJob->GetTraits() & JT_AUTO_INTERRUPTABLE ) != 0 );
	m_Traits |= oldJob->GetTraits() & JT_HUMAN_ASSIGNED;
}


float Job :: GetJobTravelDistance()
{
	if( m_InitPosition != SiegePos::INVALID )
	{
		if( ( gSiegeEngine.GetNodeFrustumOwnedBitfield( m_InitPosition.node ) & MakeInt( GetGo()->GetWorldFrustumMembership() ) ) != 0 )
		{
			return( gAIQuery.GetDistance( GetGo()->GetPlacement()->GetPosition(), m_InitPosition ) );
		}
		else
		{
			return FLT_MAX;
		}
	}
	else
	{
		return( 0 );
	}
}


///////////////////////////////////////////////////////////////////////////////////
//	persistance

FUBI_DECLARE_SELF_TRAITS( Job );


FUBI_DECLARE_CAST_TRAITS( eJobTraits, DWORD );

FUBI_DECLARE_ENUM_TRAITS( eJobAbstractType, JAT_NONE );


bool Job :: Xfer( FuBi::PersistContext& persist )
{
	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_Jat",					m_Jat );
		persist.Xfer( "m_Traits",				m_Traits );
		persist.Xfer( "m_Id",					m_Id );
		persist.Xfer( "m_GlobalInstanceCount",	m_GlobalInstanceCount );

		FUBI_XFER_BITFIELD_BOOL( persist, "m_MarkedForDeletion",       m_MarkedForDeletion       );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_Initialized",             m_Initialized             );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_InUpdateScope",           m_InUpdateScope           );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_WatchingGoalMessages",    m_WatchingGoalMessages    );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_EndRequested",            m_EndRequested            );
		FUBI_XFER_BITFIELD_BOOL( persist, "m_RegisteredWithEngaged",   m_RegisteredWithEngaged	 );

		persist.Xfer( "m_QueueIconGuid",	m_QueueIconGuid );

		//////////////////////////////////////
		//	job parameters

		persist.Xfer( "m_GoalObject",      m_GoalObject      );
		persist.Xfer( "m_GoalModifier",    m_GoalModifier    );
		persist.Xfer( "m_GoalPosition",    m_GoalPosition    );
		persist.Xfer( "m_GoalOrientation", m_GoalOrientation );
		persist.Xfer( "m_GoalSlot",        m_GoalSlot        );
		persist.Xfer( "m_Notify",          m_Notify          );
		persist.Xfer( "m_InitPosition",    m_InitPosition    );

		persist.Xfer( "m_JobResult",       m_JobResult       );

		//////////////////////////////////////
		//	temp var storage

		persist.Xfer( "m_Int1",		m_Int1 );
		persist.Xfer( "m_Int2",		m_Int2 );
		persist.Xfer( "m_Float1",	m_Float1 );

		//////////////////////////////////////
		//	skrit

		Skrit::HObject skrit = hSkrit;
		persist.Xfer( "hSkrit", skrit );
		if ( persist.IsRestoring() )
		{
			hSkrit = skrit;
			m_pSkrit = hSkrit.Get();
		}

		//////////////////////////////////////
		//	other

		persist.Xfer( "m_Timer",								m_Timer );
		persist.Xfer( "m_TimeEngagedLastVisible",				m_TimeEngagedLastVisible	);
		persist.Xfer( "m_TimeToExitAtomicState",				m_TimeToExitAtomicState		);

#		if !GP_RETAIL
		persist.Xfer( "m_LastUpdated",                       m_LastUpdated                       );
		persist.Xfer( "m_DebugTimerNotAllowedToExpireCount", m_DebugTimerNotAllowedToExpireCount );
#		endif // !GP_RETAIL
	}

	return true;
}
