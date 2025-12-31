//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritObject.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the object code for a Skrit.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITOBJECT_H
#define __SKRITOBJECT_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "GpColl.h"
#include "SkritDefs.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace Skrit
{
	namespace Raw
	{
		class DebugDb;
	}
}

namespace FileSys
{
	class Reader;
}

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// class ObjectImpl declaration

class ObjectImpl
{
public:
	SET_NO_INHERITED( ObjectImpl );

// Setup.

	// ctor/dtor
	ObjectImpl( const char* name = NULL );
   ~ObjectImpl( void );

	// persistence
	bool Xfer( FuBi::PersistContext& persist, const gpstring* name = NULL );
	bool CommitLoad( void );

// References.

	int AddRef     ( void )  {  gpassert( (m_RefCount >= 0) && (m_RefCount < 1000) );  return ( ++m_RefCount );  }		// sanity checks
	int Release    ( void )  {  gpassert( (m_RefCount >  0) && (m_RefCount < 1000) );  return ( --m_RefCount );  }		// sanity checks
	int GetRefCount( void )  {  return ( m_RefCount );  }

// Object code.

	void Reset( void );
	bool Copy ( const Raw::Header* header );
	bool Swap ( Raw::Pack& pack, eVarType ownerType );

// Query.

	typedef CBFunctor1wRet <const char*, bool> PropertyFilterCb;

	struct StateEntry;

	// simple
	const gpstring&              GetName                ( void ) const		{  return ( m_Name );  }
	bool                         IsValid                ( void ) const		{  return ( m_IsValid );  }
	int                          GetFunctionCount       ( void ) const		{  return ( scast <int> ( m_FunctionIndex.size() ) );  }
	const Raw::Pack*             GetPack                ( void ) const		{  return ( m_Pack );  }
	UINT32                       AddCrc32               ( UINT32 seed = 0 ) const;
	Store*                       GetGlobalStore         ( void )			{  return ( m_GlobalStore );  }
	bool                         HasGlobalStore         ( void ) const      {  return ( m_GlobalStore != NULL );  }
	const StateEntry*            GetStateEntry          ( int state ) const	{  return ( ccast <ObjectImpl*> ( this )->GetStateEntry( state ) );  }
	int                          GetStateCount          ( void ) const		{  return ( m_States.empty() ? 0 : scast <int> ( m_States.size() - 1 ) );  }
	int                          HasStates              ( void ) const		{  return ( !m_States.empty() );  }
	const FuBi::StoreHeaderSpec* GetStoreHeaderSpec     ( void ) const		{  return ( m_StoreHeaderSpec );  }
	FuBi::StoreHeaderSpec*       GetStoreHeaderSpec     ( void )			{  return ( m_StoreHeaderSpec );  }
	eVarType                     GetOwnerType           ( void ) const		{  return ( m_OwnerType );  }

	// complex
	int                          FindFunction           ( const char* funcName );
	int                          FindEvent              ( int state, UINT eventSerial ) const;
	const Raw::ExportEntry*      GetFunctionSpec        ( int funcIndex ) const;
	int                          GetOpCodeCount         ( int funcIndex ) const;
	bool                         CheckFunctionMatch     ( int funcIndex, const FuBi::FunctionSpec* spec );
	void                         CreateAndInitStore     ( FuBi::StoreHeaderSpec** spec, Store** store, Raw::eStore storeType, bool propertiesOnly = false, PropertyFilterCb cb = NULL );
	void                         BuildPropertiesAndStore( FuBi::StoreHeaderSpec& spec, FuBi::Store& store, PropertyFilterCb cb = NULL );
	bool                         XferStore              ( FuBi::PersistContext& persist, Store& store, Raw::eStore storeType );
	bool                         PostLoadStore          ( Store& store, Raw::eStore storeType );
	bool                         DoesWantPolls          ( void ) const;
	bool                         DoesUseEvents          ( UINT* eventSerial, int eventCount ) const;
	bool                         DoesUseEvent           ( UINT eventSerial ) const		{  return ( DoesUseEvents( &eventSerial, 1 ) );  }
#	if !GP_RETAIL
	const Raw::DebugDb*          GetDebugDb             ( void ) const;
#	endif // !GP_RETAIL

// State types.

	struct TransitionEntry
	{
		typedef stdx::fast_vector <const Raw::ConditionStatic *> StaticIndex;
		typedef stdx::fast_vector <const Raw::ConditionDynamic*> DynamicIndex;

		StaticIndex                 m_StaticIndex;
		DynamicIndex                m_DynamicIndex;
		const Raw::StateTransition* m_Transition;
	};

	struct TriggerEntry
	{
		int m_TransitionIndex;
		int m_StaticIndex;
	};

	struct StateEntry
	{
		typedef stdx::fast_vector <TransitionEntry>   TransitionIndex;
		typedef stdx::fast_vector <TriggerEntry>      TriggerColl;
		typedef stdx::linear_map  <UINT, TriggerColl> TriggerIndex;

		const Raw::StateConfig* m_Config;
		TransitionIndex         m_Transitions;
		TriggerIndex            m_StaticTriggers;
		bool                    m_NeedsPoll;

		StateEntry( void )  {  m_Config = NULL;  m_NeedsPoll = false;  }
	};

private:

// Private methods.

	bool Commit( void );

	StateEntry* GetStateEntry( int state )
	{
		if ( m_States.empty() )
		{
			return ( NULL );
		}
		else if ( state == STATE_INDEX_NONE )
		{
			return ( &m_States.back() );
		}
		else
		{
			gpassert( (state >= 0) && (state < scast <int> ( m_States.size() - 1 )) );
			return ( &*(m_States.begin() + state) );
		}
	}

// Private types.

	typedef stdx::fast_vector <const Raw::ExportEntry*>        FunctionIndex;
	typedef stdx::fast_vector <const FuBi::FunctionSpec*>      FunctionSpecIndex;
	typedef std::pair         <UINT, int>                      EventEntry;
	typedef stdx::linear_map  <EventEntry, int>                FunctionByEventIndex;
	typedef stdx::linear_map  <const char*, int, istring_less> FunctionByNameIndex;
	typedef stdx::fast_vector <StateEntry>                     StateIndex;

// Private data.

	int                    m_RefCount;				// count of objects using this impl - when 0 delete me
	gpstring               m_Name;					// name of this object
	bool                   m_IsValid;				// have we successfully committed a pack?
	Raw::Pack*             m_Pack;					// our pack of raw data
	Store*                 m_GlobalStore;			// the global variable store shared by all objects
	FuBi::StoreHeaderSpec* m_StoreHeaderSpec;		// the table type spec (if this skrit has exposed properties)
	FunctionIndex          m_FunctionIndex;			// index of all exported functions in file order - this is our base export index
	FunctionByEventIndex   m_EventIndex;			// index of all event handlers by event serial id - indexes m_FunctionIndex
	FunctionByNameIndex    m_FunctionByNameIndex;	// index of all exported functions by name - indexes m_FunctionIndex
	FunctionSpecIndex      m_SignatureCache;		// every successful match for an incoming call will get one of these to cache the results - indexes m_FunctionIndex
	StateIndex             m_States;				// index of all states and the possible transitions out of them
	eVarType               m_OwnerType;				// type of owner class
#	if !GP_RETAIL
	Raw::DebugDb*          m_DebugDb;				// optional construct-once debug db for error reporting
#	endif // !GP_RETAIL

	SET_NO_COPYING( ObjectImpl );
};

//////////////////////////////////////////////////////////////////////////////
// class Object declaration

class Object
{
public:
	SET_NO_INHERITED( Object );

	enum  {  STATE_INDEX_INVALID = -2  };			// special state for pending state change detection

// Setup.

	// ctor/dtor
	Object( ObjectImpl* impl, void* owner, bool deferConstruct = false );
   ~Object( void );

	// persistence
	bool Xfer( FuBi::PersistContext& persist );
	bool CommitLoad( void );

	// construct the object
	bool CommitCreation( void );
	bool IsConstructed( void ) const							{  return ( m_Constructed );  }

	// owner
	void SetOwner( void* owner )								{  m_Owner = owner;  }
	void* GetOwner( void ) const								{  return ( m_Owner );  }

	// implementation access
	const ObjectImpl* GetImpl       ( void ) const				{  return ( m_Impl );  }
	      ObjectImpl* GetImpl       ( void )					{  return ( m_Impl );  }
	Store*            GetGlobalStore( void )					{  return ( m_GlobalStore );  }
	const Store*      GetGlobalStore( void ) const				{  return ( m_GlobalStore );  }
	bool			  HasGlobalStore( void ) const				{  return ( m_GlobalStore != NULL ); }

// Query.

	// database access
	bool HasFunctions( void ) const								{  return ( m_Impl->GetFunctionCount() > 0 );  }
	int  FindFunction( const char* funcName )					{  return ( m_Impl->FindFunction( funcName ) );  }
	int  FindEvent   ( UINT eventSerial ) const					{  return ( m_Impl->FindEvent( m_CurrentState, eventSerial ) );  }

	// attributes
FEX	const gpstring& GetName( void ) const						{  return ( m_Impl->GetName() );  }

	// schema
	const FuBi::Record* GetRecord   ( void ) const				{  return ( ccast <ThisType*> ( this ) ->GetRecord() ); /*const ok*/  }
	FuBi::Record*       GetRecord   ( void );
	bool                HasRecord   ( void ) const				{  return ( m_Record != NULL );  }
	FuBi::Record*       CreateRecord( void );
	bool                ReadRecord  ( const char* params );		// must be key1=value1&key2=value2 format

	// results
	Result GetLastResult( void ) const							{  return ( m_LastResult );  }

// Events.

	Result Event    ( UINT eventSerial, const void* params = NULL );
	bool   ForcePoll( void );
	bool   Poll     ( float deltaTime, bool advanceFrame = false );

// Timers.

	// creation
FEX int  CreateTimer     ( float timeFromNow );					// new timer in seconds from now, auto-assigned id
FEX void CreateTimer     ( int id, float timeFromNow );			// new timer in seconds from now, use id given, overwrite existing timer if any
FEX int  CreateFrameTimer( int framesFromNow );					// new timer in frames from now, auto-assigned id
FEX void CreateFrameTimer( int id, int framesFromNow );			// new timer in frames from now, use id given, overwrite existing timer if any

	// destruction
FEX void DestroyTimer( int id );								// destroy timer by id

	// modification of existing
FEX void  SetTimerGlobal     ( int id, bool global );			// set global/local flag - if local, it will get deleted if the state changes (regardless of repeat count)
FEX void  SetTimerRepeatCount( int id, int count );				// set the repeat count - set to -1 for infinite
FEX float AddTimerSeconds    ( int id, float extraTime );		// add time to an existing timer, returns new time remaining
FEX void  ResetTimerSeconds  ( int id, float timeFromBase );	// reset timeout based on when the timer was originally created
FEX void  SetNewTimerSeconds ( int id, float timeFromNow );		// give it a new time that is based on current time (as if you destroyed and recreated it)

// State machine.

	// states
	void SetState            ( int newState );
FEX	bool IsStateChangePending( void ) const						{  return ( m_PendingState != STATE_INDEX_INVALID );  }
FEX	int  GetCurrentState     ( void ) const						{  return ( m_CurrentState );  }
FEX	int  GetPendingState     ( void ) const						{  return ( m_PendingState );  }

	// polling
FEX void  SetPollPeriod( float period )							{  m_PollPeriod = period;  }
FEX float GetPollPeriod( void ) const							{  return ( m_PollPeriod );  }

// Execution.

	// safe functions that do type checking (caching the results)
	Result Call( int funcIndex, const FuBi::FunctionSpec* spec, const_mem_ptr params = const_mem_ptr() );
	Result Call( const char* funcName, const FuBi::FunctionSpec* spec, const_mem_ptr params = const_mem_ptr() );

	// unsafe functions - they can't check types but are super efficient
	Result Call( int funcIndex, const_mem_ptr params = const_mem_ptr() );
	Result Call( const char* funcName, const_mem_ptr params = const_mem_ptr() );

// Debugging.

#	if !GP_RETAIL

	// documentation
FEX void GenerateDocs( ReportSys::Context* ctx ) const;
FEX void GenerateDocs( void ) const;

	// lookup
FEX const char* GetStateName       ( int state ) const;
FEX const char* GetCurrentStateName( void ) const;
FEX const char* GetPendingStateName( void ) const;

	// disassembling
	void Disassemble( ReportSys::Context* ctx, FileSys::Reader* base ) const;
FEX void Disassemble( ReportSys::Context* ctx ) const;
FEX void Disassemble( void ) const;

	// debug dumping
FEX void Dump            ( ReportSys::Context* ctx ) const;
FEX void Dump            ( void ) const;
FEX void DumpStateMachine( ReportSys::Context* ctx ) const;
FEX void DumpStateMachine( void ) const;
	void DumpStateMachine( gpstring& dump, int indent = 0 ) const;

#	endif // !GP_RETAIL

private:

	struct TimerEntry;
	struct TriggerEntry;
	typedef stdx::fast_vector <TimerEntry> TimerColl;

// Private methods.

	// potential state changing
	void EnterPotentialStateChange( void );	// enter a potentially state changing function
	void LeavePotentialStateChange( void );	// leave a potentially state changing function

	// misc
	void                PrivateResetTriggers( void );
	bool                PrivateCheckTriggers( const ObjectImpl::StateEntry* stateEntry, UINT eventSerial, const void* params );
	void                PrivateTransition   ( const ObjectImpl::TransitionEntry* transition );
	bool                PrivateCheckDynamic ( const ObjectImpl::TransitionEntry* transition );
	TimerColl::iterator FindTimer           ( int id, bool warnOnFail = true );
	TimerColl::iterator FindSecondsTimer    ( int id, bool warnOnFail = true );
	TimerColl::iterator FindFramesTimer     ( int id, bool warnOnFail = true );
	void                CreateTimer         ( int id, float* timeFromNow, int* framesFromNow );

#	if !GP_RETAIL
	void DumpTransitionEntry( ReportSys::ContextRef ctx, const Raw::DebugDb& debugDb, const ObjectImpl::TransitionEntry& spec, const TriggerEntry& trigger ) const;
#	endif // !GP_RETAIL

// Private types.

	struct TriggerEntry
	{
		typedef stdx::fast_vector <bool> TriggerColl;

		TriggerColl m_Triggers;				// current set of triggers to poke
		int         m_Remaining;			// how many are left before trigger is fired

		bool Xfer       ( FuBi::PersistContext& persist );
		bool CheckAndSet( int trigger, const Raw::ConditionStatic* check, const void* params );

		bool ShouldPersist( void ) const
		{
			return ( (int)m_Triggers.size() != m_Remaining );
		}

		void Reset( int size )
		{
			gpassert( size >= 0 );
			m_Triggers.resize( size, false );
			std::fill( m_Triggers.begin(), m_Triggers.end(), false );
			m_Remaining = size;
		}
	};

	struct AutoState;
	friend struct AutoState;

	struct AutoState
	{
		Object* m_Object;

		AutoState( Object* object )
			: m_Object( object )  {  m_Object->EnterPotentialStateChange();  }
	   ~AutoState( void )
			{  m_Object->LeavePotentialStateChange();  }
	};

	struct AutoMachine;
	friend struct AutoMachine;

	struct AutoMachine
	{
		AutoState m_AutoState;
		Machine*  m_Machine;
		int       m_MachineIndex;

		AutoMachine( Object* object );
	   ~AutoMachine( void );

		Machine* operator -> ( void ) const
			{  return ( m_Machine );  }
	};

	struct TimerEntry
	{
		union FrameTime
		{
			int    m_Frame;					// frame to fire on
			double m_Time;					// time to fire on in seconds
		};

		FrameTime m_FrameTimeSpec;			// creation spec
		FrameTime m_FrameTimeState;			// current state
		bool      m_TimeInFrames;			// are we using time in frames or seconds?
		bool      m_Global;					// is timer global or local to this state? if local, delete it on a state change regardless of remaining shots
		int       m_Remaining;				// number of times left that this timer will fire
		int       m_Id;						// id of this timer

		TimerEntry( void )					{  ::ZeroObject( *this );  }

		bool Xfer( FuBi::PersistContext& persist );

		bool operator == ( int id ) const	{  return ( m_Id == id );  }
	};

	typedef stdx::fast_vector <TriggerEntry> TriggerIndex;
	typedef stdx::fast_vector <int> IntColl;
	typedef ObjectImpl::StateEntry StateEntry;

// Private data.

	// $ important persistence info: we're xfer'ing the states and triggers
	//   as integer indexes, not by name. this means that any changes to skrits
	//   will affect save game files if the conditions are changed, added,
	//   reordered or removed, or if states are removed or reordered.

	// spec
	void* m_Owner;							// arbitrary dword for who the owner is
	bool  m_Constructed;					// only set true after we've constructed the object

	// owned objects
	ObjectImpl*   m_Impl;					// ref-counted shared implementation
	int           m_StateChangeDepth;		// how many levels deep in potential state changing calls we are
#	if !GP_RETAIL
	int           m_DebugStateChangeDepth;	// special debug tracker that goes outside of state changes
#	endif // !GP_RETAIL
	Store*        m_GlobalStore;			// the global variable store for this Object instance
	Result        m_LastResult;				// last result we had
	FuBi::Record* m_Record;					// record representing properties, constructed on demand

	// state machine
	int               m_CurrentState;		// state we're currently in, STATE_INDEX_NONE for none
	int               m_PendingState;		// state we're going to, STATE_INDEX_INVALID for no change
	bool              m_IsPolling;			// are we polling for state transition checking?
	float             m_PollPeriod;			// polling period from one check to the next (set to 0.0 for polling every frame)
	float             m_CurrentPoll;		// current time (used for next poll)
	double            m_CurrentTime;		// current time (used for timed functions)
	int               m_CurrentFrame;		// current frame (used for timed functions)
	TriggerIndex      m_Triggers;			// bool arrays for each transition for the current state
	IntColl           m_AtFunctions;		// indexes of functions meant to trigger "at" a certain time
	TimerColl         m_Timers;				// outstanding timers
	int               m_NextTimerId;		// next timer id to use
	const StateEntry* m_StateEntry;			// current state entry in impl

	// debug tuning
public:
#	if !GP_RETAIL
	FUBI_VARIABLE( bool, m_, TraceOpCodes,      "trace all opcode execution" );
	FUBI_VARIABLE( bool, m_, TraceSysCalls,     "trace outgoing calls to system functions" );
	FUBI_VARIABLE( bool, m_, TraceSkritCalls,   "trace calls to other skrit functions within this object" );
	FUBI_VARIABLE( bool, m_, TraceStateChanges, "trace changes in state" );
	FUBI_VARIABLE( bool, m_, TraceEvents,       "trace incoming events" );
#	endif // !GP_RETAIL

	SET_NO_COPYING( Object );
};

//////////////////////////////////////////////////////////////////////////////
// class ObjectImpl inline implementation

inline const Raw::ExportEntry* ObjectImpl :: GetFunctionSpec( int funcIndex ) const
{
	gpassert( IsValid() );
	gpassert( funcIndex != INVALID_FUNCTION_INDEX );
	gpassert( (funcIndex >= 0) && (funcIndex < scast <int> ( m_FunctionIndex.size() )) );

	return ( m_FunctionIndex[ funcIndex ] );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

#endif  // __SKRITOBJECT_H

//////////////////////////////////////////////////////////////////////////////
