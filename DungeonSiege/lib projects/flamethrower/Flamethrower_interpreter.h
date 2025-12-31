#pragma once
//
//	Flamethrower_interpreter.h
//
//	Script object class
//

#ifndef _FLAMETHROWER_INTERPRETER_H_
#define	_FLAMETHROWER_INTERPRETER_H_


class	TattooTracker;
class	Flamethrower_script;


#include "BlindCallback.h"
#include "BucketVector.h"
#include "Flamethrower_script_helpers.h"
#include "Flamethrower_tracker.h"

#include <map>
#include <vector>



struct RunScriptXfer
{
	SFxSID              m_ScriptId;
	const char*         m_Name;
	gpstring            m_NameBuffer;
	Goid                m_ScriptOwner;		// full xfer only
	UINT32              m_Seed;				// full xfer only
	eWorldEvent         m_Type;
	bool                m_Xfer;				// full xfer only
	Flamethrower_params m_Params;
	def_tracker         m_Targets;

	void Xfer( FuBi::BitPacker& packer, bool fullXfer = true );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};



//
// --- Flamethrower Interpreter Interface
//

class Flamethrower_interpreter : public Singleton < Flamethrower_interpreter >
{
public:
	class Signal;


	// --- Existence

	Flamethrower_interpreter			( const char *sScripFuelAddress );
	~Flamethrower_interpreter			( void );

	void		Shutdown				( bool bFullShutdown = false );
	bool		IsEmpty					( void ) const;


	// --- Persistence

	void		PrepareForSave			( void );
	bool		Xfer					( FuBi::PersistContext& persist );



	// --- Script construction

	// Adds a new script to the list of available scripts for execution later - returns true if added, false if error(s)
	bool		AddScript				( const char *sName, const char* sScript, gpstring & sErrors );

	// Removes the specified script from the list of available scripts
	bool		DestroyScript			( const gpstring &sScriptName );



	// --- Script control

	// General
	SFxSID		RunScript				( const char *sName, def_tracker targets, const char *sParams, Goid ScriptOwner, eWorldEvent type, bool server, bool xfer );

	// Internal
FEX	void		RCRunScriptPacker		( const_mem_ptr packola );
	SFxSID		RunScript				( const char *sName, def_tracker targets, const Flamethrower_params & params, UINT32 start_param, Goid ScriptOwner, UINT32 seed, eWorldEvent type, SFxSID script_id, bool xfer );
	SFxSID		RunScript				( const char *sName, def_tracker targets, const Flamethrower_params & params, UINT32 start_param, Goid ScriptOwner, UINT32 seed, eWorldEvent type, bool xfer );

	// stops both globally and locally running scripts
	void		SStopScript				( SFxSID id );
	
	void		SStopScript				( Goid id, const char *sName );

	// standard rc version of stop script - called only by SSStopScript
FEX	void		RCStopScript			( SFxSID id );

	// This just stops all of the spawned effects of the given script.
	void		StopScript				( SFxSID id );

	// This will stop a script on specified go that matches specified name
	void		StopScript				( Goid id, const char *sName );

	// Stops all of the active scripts destroying any spawned effects from each active script
	void		StopAllScripts			( bool immediate = false );

	// Flushes all queues - only called from shutdown
	void		FlushQueues				( void );


	// Forces a script to finish non-abruptly
	void		FinishScript			( SFxSID id );


	// --- Utility

	// Finds a script based on the script id
	bool		FindScript				( SFxSID id, Flamethrower_script **pScript );

	// Checks to see if a script is active or not
	bool		IsScriptRunning			( SFxSID id );

	// Checks to see if we should bother xferring a script
	bool		ShouldScriptXfer		( SFxSID id );

	// Get the go that the script should be registered with
	Goid		GetRegistrationGoid		( SFxSID id );



	// --- Loading/Unloading

	// Unloads a script so that it can be restarted in the same state
	void		UnloadScript			( SFxSID id );

	// Loads a script and restarts it
	void		LoadScript				( SFxSID id );



	// --- Reloading

	// Removes all old scripts and rebuilds script list by reprocessing all scripts in gas
	void		RebuildScriptVocabulary	( void );



	// --- Script communication

	// Post a signal - worldmessages come through here
	void		PostSignal				( SFxSID id, const gpstring &sSignal, float timeout = DEFAULT_SIGNAL_TIMEOUT );

	// Checks for signals sent to the specified script id
	bool		CheckForSignal			( SFxSID id, const gpstring &sSignal, bool bRemove = true );



	// --- Callback control

	// Overrides the default callback that the script was set to callback upon creation to a unique script
	bool		SetScriptCallback		( SFxSID id, const CBFunctor1< SFxSID > callback );



	// --- Update all scripts

	void		Update					( float SecondsElapsed, float UnPausedSecondsElapsed );


	// --- Update Helper Stuff
	float		GetSecondsElapsed()		{ return m_SecondsElapsed; }


	// --- ID

	SFxEID		GetNextEffectID			( SFxSID script_id );
	SFxSID		GetNextScriptID			( bool global = false, bool bUsesSignals = false );


	// --- Locale

	bool		IsBloodEnabled			( void ) const						{ return m_bIsBloodEnabled; }
	void		SetIsBloodEnabled		( bool bBloodEnabled )				{ m_bIsBloodEnabled = bBloodEnabled; }

	bool		IsBloodRed				( void ) const						{ return m_bIsBloodRed; }
	void		SetIsBloodRed			( bool bBloodRed )					{ m_bIsBloodRed = bBloodRed; }



	// --- Token stuff

	bool		GetCommandTokenIndex	( const char *sCommand, UINT32 & token );
	bool		GetMacroTokenIndex		( const char *sMacro,	UINT32 & token );
	bool		GetParamTokenIndex		( const char *sCommand, const char *sParam,	UINT32 & token );



	// --- Debugging info

#	if !GP_RETAIL
	gpstring	GetAvailableScripts		( void );
	gpstring	GetActiveScripts		( void );

#	endif // !GP_RETAIL



	// --- Typedefs

	typedef stdx::fast_vector< Signal >										SignalColl;
	typedef stdx::fast_vector< SignalColl::iterator  >						SignalIColl;
	typedef stdx::fast_vector< Flamethrower_macro*   >						def_macro_list;
	typedef stdx::fast_vector< Flamethrower_command* >						def_command_list;
	typedef stdx::fast_vector< Flamethrower_script*  >						def_script_ptr_list;
	typedef stdx::fast_vector< SFxSID >										def_script_id_list;

	typedef std::map	< SFxSID, Flamethrower_script* >					def_script_id_map;
	typedef std::map	< gpstring, Flamethrower_script*, istring_less >	def_script_name_map;

	enum { DEFAULT_SIGNAL_TIMEOUT	= 1	 };



	// --- Signal helper class

	class Signal
	{
	public:
		Signal							( void ) {}
		Signal							( SFxSID id, const gpstring &sTag, float duration )
			: m_id( id )
			, m_sTag( sTag )
			, m_duration( duration )
		{}

		bool Xfer( FuBi::PersistContext &persist );

		SFxSID		m_id;
		gpstring	m_sTag;
		float		m_duration;
	};



private:

	// --- Utility

	bool		FindScriptObj			( SFxSID id, Flamethrower_script **pScript, const def_script_id_map &map );
	bool		FindScriptObj			( SFxSID id, Flamethrower_script **pScript, const def_script_ptr_list &list );

	bool		UsesSignals				( const char *sName );


	// --- Script construction

	bool		AddMacro				( Flamethrower_macro *pMacro );
	bool		AddCommand				( Flamethrower_command *pCommand );

	void		BuildMacroVocabulary	( void );
	void		BuildCommandVocabulary	( void );
	void		BuildScriptVocabulary	( void );

	bool		CreateNewScript			( const char *sName, const Flamethrower_params & script_params, UINT32 start_param, def_tracker targets, Flamethrower_script **pScript );

	bool		TextToParams			( const char * sParams, gpstring & errors, Flamethrower_params & script_params );
	bool		ReplaceScriptParams		( Flamethrower_script * pScript, const Flamethrower_params & script_params, UINT32 start_param );

	bool		ScanScript				( const char *pText, gpstring & errors, Flamethrower_script & script );
	bool		ScanLine				( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_script & script, Flamethrower_line & line );
	bool		ScanBlock				( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end );
	bool		ScanExpression			( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, char * sOut );
	bool		ScanCommand				( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_script & script, Flamethrower_line & line );
	bool		ScanParams				( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_line & line );
	bool		StoreParam				( const char *pText, gpstring & errors, UINT32 len, Flamethrower_params::PARAM_TYPE pt, Flamethrower_params & params );
	bool		StoreParam				( const char *pText, gpstring & errors, Flamethrower_params & params, Flamethrower_command * pCommand = NULL );

	bool		PrepareParam			( const char *pText, Flamethrower_params & params, Flamethrower_params::PARAM_TYPE pt, UINT32 len, char * pBuf );


	// --- Queue processing operations

	void		StopScriptNow			( SFxSID id );
	void		StopAllSounds			( Flamethrower_script* pScript );

	bool		DoScriptStoppage		( SFxSID id, def_script_id_map &script_map );
	void		FinishScriptNow			( SFxSID id );
	void		UnloadScriptNow			( SFxSID id );
	void		UnloadScriptNow			( Flamethrower_script *pScript );
	void		LoadScriptNow			( SFxSID id );


	// --- Update related

	void		UpdateScript			( Flamethrower_script *pScript );

	bool		EvaluateMacrosAndLocals	( Flamethrower_script *pScript, Flamethrower_params & params, bool bSet );
	bool		EvaluateMacrosAndLocals	( Flamethrower_script *pScript, const char * sIn, gpstring & sOut );
	


	// --- Data members (xfered)

	def_script_id_map		m_DisabledScripts;	// Unloaded scripts
	def_script_id_map		m_RunningScripts;	// Active scripts
	def_script_ptr_list		m_RunQueue;			// Scripts waiting to run

	def_script_id_list		m_StopQueue;		// Scripts waiting to stop
	def_script_id_list		m_UnloadQueue;		// Scripts waiting to unload
	def_script_id_list		m_LoadQueue;		// Scripts waiting to load
	def_script_id_list		m_FinishQueue;		// Scripts waiting to finish

	SignalColl				m_Signals;			// Active signal storage
	SignalColl				m_SignalInsQueue;	// insertion queue

	float					m_SecondsElapsed;
	float					m_UnPausedSecondsElapsed;

	SFxSID					m_GlobalScriptID;
	SFxSID					m_LocalScriptID;


	// --- Data members (NOT xfered)

	def_macro_list			m_MacroList;		// Storage for macros		- no need to xfer
	def_command_list		m_CommandList;		// Storage for commands		- no need to xfer
	def_script_name_map		m_ParentScripts;	// Storage for scripts		- no need to xfer

	gpstring				m_sScriptAddress;	// - no need to xfer
	bool					m_bIsBloodRed;		// - no need to xfer
	bool					m_bIsBloodEnabled;	// - no need to xfer

	UINT32					m_NextMacroTokenIndex;	// - no need to xfer
	UINT32					m_NextCommandTokenIndex;// - no need to xfer	

	UINT32					m_TokenWaitfor;		// Special token to compare against for signal usage on a script- no need to xfer
	UINT32					m_TokenSet;			// Specail token to compare against for symbol evaluation		- no need to xfer

	UINT32					m_ScriptErrors;		// - no need to xfer
	UINT32					m_TotalScripts;		// - no need to xfer
	UINT32					m_TotalLines;		// - no need to xfer



	FUBI_SINGLETON_CLASS( Flamethrower_interpreter, "Internal interface to Flamethrower_interpreter" );
	SET_NO_COPYING( Flamethrower_interpreter );
};

#define gFlamethrower_interpreter Flamethrower_interpreter::GetSingleton()



//
// --- Flamethrower script
//

class Flamethrower_script
{
	friend class Flamethrower_interpreter;

public:

	enum BLOCK_TYPE
	{
		BT_INVALID,

		BT_NORMAL,
		BT_BEGIN,
		BT_IF_TRUE,
		BT_IF_FALSE,
	};
	
	enum COLLISION
	{
		NO_COLLISION,
		TARGET_COLLISION,
		TERRAIN_COLLISION
	};

	typedef stdx::fast_vector	< Flamethrower_line >					def_script;
	typedef stdx::fast_vector	< BLOCK_TYPE >							def_block_stack;
	typedef	stdx::fast_vector	< Flamethrower_params >					def_stack;
	typedef stdx::fast_vector	< UINT32 >								def_idstack;
	typedef std::multimap		< double, UINT32 >						def_id_time_stack;
	typedef stdx::fast_vector	< SFxEID >								def_eidstack;
	typedef stdx::fast_vector	< SFxSID >								def_sidstack;
	typedef std::map	< gpstring, Flamethrower_params, istring_less >	def_symbol_map;



	// --- Existence

	Flamethrower_script			( void );
	Flamethrower_script			( Flamethrower_script &copy );
	~Flamethrower_script		( void );

	void					PrepareForSave			( void );
	bool					Xfer					( FuBi::PersistContext &persist );
	bool					ShouldPersist			( void ) const;
	void					Reset					( void );

	void operator=( Flamethrower_script &dup );


	// --- Control

	void					Stop					( void );


	// --- Registration

	void					RemoveCompletionTestID	( SFxEID effectid );
	void					RemoveCompletionTestID	( UINT32 soundid );

	void					SetRegistrationEvent	( eWorldEvent type )				{ m_RegAssEvent = type; }
	eWorldEvent				GetRegistrationEvent	( void ) const						{ return m_RegAssEvent; }

	void					SetUsesSignals			( bool b )							{ m_bUsesSignals = b; }
	bool					GetUsesSignals			( void ) const						{ return m_bUsesSignals; }


	// --- Accessors

	gpstring&				GetName					( void )		 					{ return m_sName; }
	SFxSID					GetID					( void ) const						{ return m_ID; }

	Goid					GetOwner				( void ) const						{ return m_Owner; }
	void					SetOwner				( Goid id )							{ gpassert( m_Owner == GOID_INVALID ); m_Owner = id; }

	SFxEID					GetNextEffectID			( void ) const						{ return m_NextEffectID; }
	void					SetNextEffectID			( SFxEID id )						{ m_NextEffectID = id; }

	UINT32					GetSeed					( void ) const						{ return m_Seed; }
	void					SetSeed					( UINT32 seed )						{ m_Seed = seed; }

	void					SetPauseImmune			( bool b )							{ m_bPauseImmune = b; }
	bool					GetPauseImmune			( void ) const						{ return m_bPauseImmune; }
	float					GetElapsed				( void ) const						{ return m_Elapsed; }


	// --- Targets

	TattooTracker*			GetTracker				( int i );
	def_tracker				ReleaseTracker			( int i );
	void					OwnTracker				( int i, def_tracker &tracker );
	int						AddTracker				( def_tracker &tracker );

	


	TattooTracker*			GetDefaultTargets		( void )							{ return m_Default_targets.get(); }
	def_tracker				ReleaseDefaultTargets	( void )							{ return m_Default_targets; }
	void					SetDefaultTargets		( def_tracker targets )				{ m_Default_targets = targets; }


	// --- Execution

	inline def_stack&			GetStack			( void )							{ return m_Stack; }
	inline def_id_time_stack&	GetSoundLog			( void )							{ return m_SoundLog; }
	inline def_idstack&			GetLoopingSoundLog	( void )							{ return m_LoopingSoundLog; }
	inline def_eidstack&		GetEffectLog		( void )							{ return m_EffectLog; }
	inline def_symbol_map&		GetSymbols			( void )							{ return m_Symbols; }
	inline Flamethrower_params&	GetReturnValue		( void )							{ return m_ReturnValue; }
	inline Flamethrower_line&	GetLine				( UINT32 n )						{ return m_Script[ n ]; }
	inline Flamethrower_line&	GetCurrentLine		( void )							{ return m_Script[ m_CurrentLine ]; }
	inline UINT32&				GetLineNumber		( void )							{ return m_CurrentLine; }
	inline UINT32				GetLastLineNumber	( void ) const						{ return m_Script.size(); }

	void					PushBlock				( BLOCK_TYPE type );
	BLOCK_TYPE				PeekBlock				( void );
	BLOCK_TYPE				PopBlock				( void );


	// --- Callbacks

	void					SetCallback				( CBFunctor1< SFxSID > callback )	{ m_Callback = callback; }
	void					RemoveSoundID			( DWORD id );

	GPDEV_ONLY( void CheckValid( void ) const; )

private:

	typedef UnsafeBucketVector <my TattooTracker*> TrackerColl;

	// --- Data members (XFER) 

	gpstring					m_sName;

	SFxSID						m_ID;
	Goid						m_Owner;
	SFxEID						m_NextEffectID;

	UINT32						m_Seed;

	float						m_Elapsed;
	float						m_FrameSync;
	bool						m_bPauseImmune;

	UINT32						m_CurrentLine;

	def_block_stack				m_BlockStack;
	def_symbol_map				m_Symbols;
	def_stack					m_Stack;
	def_eidstack				m_EffectLog;

	def_script					m_Script;

	def_tracker					m_Default_targets;			// Default target and source
	TrackerColl					m_Trackers;					// Storage for targets created during script execution

	eWorldEvent					m_RegAssEvent;				// Registration association

	bool						m_bUsesSignals;				// True if this script is one that will be listening for worldmessages 


	// --- Data members (NO XFER) 

	def_id_time_stack			m_SoundLog;					// $ don't persist, sound id's don't restart on load
	def_idstack					m_LoopingSoundLog;			// $ don't persist, sound id's don't restart on load
	Flamethrower_params			m_ReturnValue;				// Holds the last return value - don't need to xfer
	CBFunctor1< SFxSID >		m_Callback;
};



#endif // _FLAMETHROWER_INTERPRETER_H_
