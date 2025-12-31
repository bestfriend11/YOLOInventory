#pragma once
/*=======================================================================================

  Job
  
  Purpose:		This is a container class for all AI skrits.  Basic function of the container
				is for C++ side job parameter storage, and to make jobs more pallatable to
				the GoMind and the C++ Job queueing mechanism.

  Auhtor(s):	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __JOB_H
#define __JOB_H


#include "siege_pos.h"
#include "fubidefs.h"
#include "skritdefs.h"
#include "godefs.h"
#include "mcp_defs.h"




////////////////////////////////////////
//	eJobResult

enum eJobResult
{
	SET_BEGIN_ENUM( JR_, 0 ),

	JR_INVALID,
	JR_UNKNOWN,
	JR_OK,
	JR_FAILED,
	JR_FAILED_NO_PATH,
	JR_FAILED_BAD_PARAMS,

	SET_END_ENUM( JR_ ),
};

const char* ToString( eJobResult value );
bool FromString( const char* str, eJobResult & value );



/*===========================================================================

	Class		: Job

	Author(s)	: Bartosz Kijanka

	Purpose		: This is the basic interface for the atomic AI task.  This
				  interface also includes some generic variable storage that
				  derived behaviors may use.

---------------------------------------------------------------------------*/
class Job
{

public:

	Job();

	Job( GoMind * pMind, eJobAbstractType jat );

	~Job( void );

	void Init();
	void InitPointers();
	void Deinit( bool onAppShutdown = false );

	void Update();
	bool InUpdateScope()									   		{ return m_InUpdateScope; }
	bool IsInitialized()									   		{ return m_Initialized; }
	Scid GetStartedBySECommand()									{ return m_StartedBySECommand; }
FEX	void SetStartedBySECommand( Scid scid )							{ m_StartedBySECommand = scid; }

#	if !GP_RETAIL
	void DrawDebugHud();
#	endif // !GP_RETAIL

	void HandleMessage( WorldMessage const & msg );

	void SetMind( GoMind * mind )							   		{ gpassert(!m_pMind); m_pMind = mind; }

	bool Xfer( FuBi::PersistContext& persist );

	////////////////////////////////////////
	//	identification	

	gpstring		GetName( void );
FEX	DWORD			GetId() const							   		{ return( m_Id ); }
FEX	Go *			GetGo( void ) const;
FEX	Goid			GetGoid( void );

	inline void		SetJobAbstractType( eJobAbstractType Type )		{ m_Jat = Type; }
FEX	inline			eJobAbstractType GetJobAbstractType() const		{ return( m_Jat ); }

	eJobTraits		GetTraits() const						   		{ return m_Traits; }
	void			SetTraits( eJobTraits traits );
FEX	void			SetTraitInterruptable( bool flag );
FEX void			SetTraitUseSpatialSensors( bool flag );

	// some trait shortcuts
FEX	bool IsOffensive();
FEX	bool IsDefensive();

FEX	eActionOrigin GetOrigin();
FEX bool IsUserAssigned();

FEX bool InAtomicState();
FEX void EnterAtomicState( float duration );
FEX void LeaveAtomicState();

	bool IsRegisteredWithEngaged()									{ return m_RegisteredWithEngaged; }

FEX	inline GoMind * GetMind() const				  		   			{ return( m_pMind ); }

	////////////////////////////////////////
	//	job parameters

FEX	Goid GetGoalObject()						  		   			{ return( m_GoalObject ); }
	void SetGoalObject( Goid go );
	
FEX	Goid GetGoalModifier()						  		   			{ return( m_GoalModifier ); }
	void SetGoalModifier( Goid go );
	
FEX	SiegePos const & GetGoalPosition() const;
	void SetGoalPosition( SiegePos const & pos );

FEX	Quat const & GetGoalOrientation() const				   			{ return m_GoalOrientation; }
	void SetGoalOrientation( Quat const & quat )		   			{ m_GoalOrientation = quat; }

FEX	eEquipSlot GetGoalSlot() const				  		   			{ return m_GoalSlot; }
	void SetGoalSlot( eEquipSlot slot )					   			{ m_GoalSlot = slot; }

FEX	int GetInt1() const									   			{ return m_Int1; }
FEX	int GetInt2() const									   			{ return m_Int2; }
	void SetInt1( int x )								   			{ m_Int1 = x; }
	void SetInt2( int x )								   			{ m_Int2 = x; }

FEX	float GetFloat1() const									   		{ return m_Float1; }
	void SetFloat1( float x )								   		{ m_Float1 = x; }

FEX	SiegePos const & GetInitPosition()		  			   			{ return( m_InitPosition ); }
FEX float GetJobTravelDistance();

	Goid GetNotify()									   			{ return m_Notify; }
	void SetNotify( Goid goid )							   			{ m_Notify = goid; }

FEX	void TakesOver( Job * oldJob );

	////////////////////////////////////////
	//	util
														   
FEX	Goid GetQueueIconGoid()									   		{ return m_QueueIconGuid;		}
	void SetQueueIconGoid( Goid icon_guid );

	// sets a count-down timer, in seconds.  The behavior will receive a WE_JOB_TIMER_DONE when timer has expired
FEX	void SetTimer( float Delay );

	double GetTimeEngagedLastVisible()							   	{ return( m_TimeEngagedLastVisible ); }
	void SetTimeEngagedLastVisible( double time ) 				   	{ m_TimeEngagedLastVisible = time; }

	// clears the timer if any
FEX	void ClearTimer();

FEX void MarkCleaningUp()											{ m_CleaningUp = true; }
FEX	void MarkForDeletion( eJobResult jr );
FEX	void MarkForDeletion();


FEX	void RequestEnd();
FEX bool EndRequested();

FEX	bool IsMarkedForDeletion()									   	{ return m_MarkedForDeletion; }
FEX bool IsCleaningUp()												{ return m_CleaningUp; }
FEX bool IsShuttingDown()											{ return m_ShuttingDown; }

FEX	eJobResult GetJobResult()										{ return m_JobResult; }
FEX	bool GetJobSuccessful()										   	{ return m_JobResult == JR_OK; }
	void UnmarkForDeletion()									   	{ m_MarkedForDeletion = false; }
	static DWORD GetInstanceCount()								   	{ return m_GlobalInstanceCount; }

	////////////////////////////////////////
	//	UI related

	bool GetFocusPosition( SiegePos & pos );
	void DrawWaypointIcon();
	void HideWaypointIcon();

	
	////////////////////////////////////////
	//	debug

	bool Validate();

#	if !GP_RETAIL
	void Dump( bool verbose, ReportSys::ContextRef ctx = NULL );
#	endif


private:

	void InitData();

	void SetInitPosition( SiegePos const & pos )
	{
		m_InitPosition = pos;
	}
	void PrivateHandleMessage( WorldMessage const & msg );
	void PrivateUpdate();

	//////////////////////////////////////
	//	identification

	eJobAbstractType 		m_Jat;
	eJobTraits       		m_Traits; 
	DWORD					m_Id;
	static DWORD			m_GlobalInstanceCount;
	Scid					m_StartedBySECommand;

	bool					m_CleaningUp				: 1;
	bool			 		m_MarkedForDeletion			: 1;
	bool					m_Initialized				: 1;
	bool					m_InUpdateScope				: 1;
	bool					m_WatchingGoalMessages		: 1;
	bool					m_EndRequested				: 1;
	bool					m_RegisteredWithEngaged		: 1;
	bool					m_ShuttingDown				: 1;

	Goid 		 			m_QueueIconGuid;

	eJobResult				m_JobResult;

	//////////////////////////////////////
	//	job initial parameters

	Goid					m_GoalObject;
	Goid					m_GoalModifier;
	SiegePos				m_GoalPosition;
	Quat					m_GoalOrientation;
	eEquipSlot				m_GoalSlot;
	Goid					m_Notify;
	SiegePos				m_InitPosition;

	////////////////////////////////////////////////////////////////////////////////
	//	runtime variables

	//////////////////////////////////////
	//	temp var storage
	int						m_Int1;
	int						m_Int2;
	float					m_Float1;

	//////////////////////////////////////
	//	cached pointers
	GoMind					* m_pMind;

	//////////////////////////////////////
	//	other

	double			   		m_Timer;
	double					m_TimeEngagedLastVisible;
	double					m_TimeToExitAtomicState;

	Skrit::HAutoObject 		hSkrit;
	Skrit::Object			* m_pSkrit;

	////////////////////////////////////////
	//	debug
#	if !GP_RETAIL
	unsigned int	   		m_DebugTimerNotAllowedToExpireCount;
	DWORD					m_LastUpdated;
#	endif

	SET_NO_INHERITED( Job );
	SET_NO_COPYING( Job );
};




#endif //  __JOB_H
