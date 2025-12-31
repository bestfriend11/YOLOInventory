//
//	Flamethrower_interpreter.cpp
//
//

#include "precomp_flamethrower.h"
#include "gpcore.h"
#include "WorldTime.h"

#include <string>
#include <map>

#include "WorldFx.h"
#include "flamethrower_tracker.h"
#include "flamethrower_interpreter.h"
#include "flamethrower_interpreter_macros.h"
#include "flamethrower_interpreter_commands.h"
#include "flamethrower_FuBi_support.h"

#include "BlindCallback.h"

#include "GPProfiler.h"
#include "fuel.h"

#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"
#include "GoDb.h"
#include "GoRpc.h"
#include "GoSupport.h"
#include "StdHelp.h"
#include "SkritDefs.h"



FUBI_REPLACE_NAME( "Flamethrower_interpreter", SiegeFxInterpreter );


//
// --- RunScriptXfer
//

void RunScriptXfer::Xfer( FuBi::BitPacker& packer, bool fullXfer )
{
	packer.XferCount ( rcast <DWORD&> ( m_ScriptId ) );
	packer.XferString( m_Name, m_NameBuffer );
	packer.XferRaw   ( m_Type, FUBI_MAX_ENUM_BITS( WE_ ) );
	packer.XferIf    ( m_ScriptOwner, GOID_INVALID );

	if ( fullXfer )
	{
		packer.XferRaw( m_Seed );
		packer.XferBit( m_Xfer );
	}

	m_Params.Xfer( packer );

	if ( packer.IsRestoring() )
	{
		m_Targets = WorldFx::MakeTracker();
	}
	m_Targets->Xfer( packer );
}

#if !GP_RETAIL

void RunScriptXfer::RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf(
			"scriptId = 0x%08X, name = '%s', scriptOwner = 0x%08X, "
			"seed = 0x%08X, type = %s, params = '%s', targets = '%s'",
			m_ScriptId, m_Name ? m_Name : "<NULL>", m_ScriptOwner, m_Seed,
			::ToString( m_Type ), m_Params.ToString().c_str(),
			m_Targets->ToString().c_str() );

	if ( m_Xfer )
	{
		appendHere += " +xfer";
	}
}

#endif // !GP_RETAIL


//
// --- Flamethrower interpreter
//

Flamethrower_interpreter::Flamethrower_interpreter( const char *sScriptFuelAddress )
	: m_sScriptAddress( sScriptFuelAddress )
{
	m_GlobalScriptID			= 0;
	m_LocalScriptID				= 0;
	m_SecondsElapsed			= 0;
	m_UnPausedSecondsElapsed	= 0;
	m_NextMacroTokenIndex		= 0;
	m_NextCommandTokenIndex		= 0;

	m_TokenWaitfor			= 0;
	m_TokenSet				= 0;

	m_ScriptErrors	= 0;
	m_TotalScripts	= 0;
	m_TotalLines	= 0;

	BuildMacroVocabulary();
	BuildCommandVocabulary();
	BuildScriptVocabulary();

	gWorldFx.GetRNG().SetSeed( RandomDword() );

	for( UINT32 i = (gWorldFx.GetRNG().RandomDword() & 0xFF); i > 0; --i )
	{
		gWorldFx.GetRNG().RandomDword();
	}

	m_bIsBloodRed		= gWorldOptions.IsBloodRed();
	m_bIsBloodEnabled	= gWorldOptions.IsBloodEnabled();

}


Flamethrower_interpreter::~Flamethrower_interpreter( void )
{
	Shutdown( true );
}


void
Flamethrower_interpreter::Shutdown( bool bFullShutdown )
{
	StopAllScripts( true );
	FlushQueues();

	if( bFullShutdown )
	{
		def_script_name_map::iterator it = m_ParentScripts.begin(), it_end = m_ParentScripts.end();
		for(; it != it_end; ++it )
		{
			delete (*it).second;
		}

		def_command_list::iterator command = m_CommandList.begin(), command_end = m_CommandList.end();
		for( ; command != command_end; ++command )
		{
			delete *command;
		}

		def_macro_list::iterator macro = m_MacroList.begin(), macro_end = m_MacroList.end();
		for( ; macro != macro_end; ++macro )
		{
			delete *macro;
		}
	}
}


bool
Flamethrower_interpreter::IsEmpty( void ) const
{
	return (   m_DisabledScripts	.empty()
			&& m_RunningScripts		.empty()
			&& m_RunQueue			.empty()
			&& m_StopQueue			.empty()
			&& m_UnloadQueue		.empty()
			&& m_LoadQueue			.empty()
			&& m_FinishQueue		.empty()
			&& m_Signals			.empty()
			&& m_SignalInsQueue		.empty()
			);
}


void
Flamethrower_interpreter::PrepareForSave( void )
{
	// check the run queue
	{
		def_script_ptr_list::iterator i = m_RunQueue.begin(), i_end = m_RunQueue.end();

		for(; i != i_end; ++i )
		{
			(*i)->PrepareForSave();
		}
	}

	// check all the running scripts
	{
		def_script_id_map::iterator i = m_RunningScripts.begin(), i_end = m_RunningScripts.end();

		for(; i != i_end; ++i )
		{
			(*i).second->PrepareForSave();
		}
	}

	// check all the disabled scripts
	{
		def_script_id_map::iterator i = m_DisabledScripts.begin(), i_end = m_DisabledScripts.end();

		for(; i != i_end; ++i )
		{
			(*i).second->PrepareForSave();
		}
	}
}


bool
Flamethrower_interpreter::Xfer( FuBi::PersistContext& persist )
{
	persist.XferIfMap	( "m_DisabledScripts",			m_DisabledScripts		, FxShouldPersistSecond() );
	persist.XferIfMap	( "m_RunningScripts",			m_RunningScripts		, FxShouldPersistSecond() );
	persist.XferIfVector( "m_RunQueue",					m_RunQueue				, FxShouldPersistHelper() );

	persist.XferIfVector( "m_StopQueue",				m_StopQueue				, FxShouldPersistHelper() );
	persist.XferIfVector( "m_UnloadQueue",				m_UnloadQueue			, FxShouldPersistHelper() );
	persist.XferIfVector( "m_LoadQueue",				m_LoadQueue				, FxShouldPersistHelper() );
	persist.XferIfVector( "m_FinishQueue",				m_FinishQueue			, FxShouldPersistHelper() );

	persist.XferIfVector( "m_Signals",					m_Signals				, FxShouldPersistHelper() );
	persist.XferIfVector( "m_SignalInsQueue",	 		m_SignalInsQueue		, FxShouldPersistHelper() );

	persist.Xfer		( "m_SecondsElapsed",			m_SecondsElapsed		);
	persist.Xfer		( "m_UnPausedSecondsElapsed",	m_UnPausedSecondsElapsed);

	persist.Xfer		( "m_LocalScriptID",			m_LocalScriptID			);
	persist.Xfer		( "m_GlobalScriptID",			m_GlobalScriptID		);

	return true;
}


bool
Flamethrower_interpreter::AddScript( const char *sName,	const char *sScript, gpstring & sErrors )
{
	def_script_name_map::iterator i = m_ParentScripts.find( sName );

	if( i != m_ParentScripts.end() )
	{
		gpwarningf(( "SiegeFx: error - script \"%s\" rejected - duplicate script name\n", sName ));
		return false;
	}

	// prepare an empty script to store into
	Flamethrower_script *pNewScript = new Flamethrower_script;

	pNewScript->Reset();
	pNewScript->m_sName = sName;

	// parse the script
	if( ScanScript( sScript, sErrors, *pNewScript ) )
	{
		// add the new script to the parent list
		m_ParentScripts.insert( std::make_pair( sName, pNewScript ) );

		return true;
	}

	// delete the invalid script
	delete pNewScript;

	return false;
}


bool
Flamethrower_interpreter::DestroyScript( const gpstring &sScriptName )
{
	def_script_name_map::iterator it = m_ParentScripts.find( sScriptName );

	if( it != m_ParentScripts.end() )
	{
		delete (*it).second;
		m_ParentScripts.erase( it );

		return true;
	}
	return false;
}


static bool FilterFrustumByGo( FrustumId& frustumMask, Goid goid )
{
	GoHandle go( goid );
	if ( go )
	{
		// check it
		GPDEV_ONLY( GoRpcMgr::CheckSafeGoRpcParam( goid ) );

		// mask it
		frustumMask = MakeFrustumId( MakeInt( frustumMask ) & MakeInt( go->CalcWorldFrustumMembership() ) );
	}
	else
	{
		// no exist? well nobody gets it then!
		frustumMask = FRUSTUMID_INVALID;
	}

	return ( frustumMask != FRUSTUMID_INVALID );
}


SFxSID
Flamethrower_interpreter::RunScript( const char *sName, def_tracker targets, const char *sParams, Goid ScriptOwner, eWorldEvent type, bool server, bool xfer )
{
	// pre-convert script params
	RunScriptXfer scriptXfer;
	gpstring errors;
	if ( !TextToParams( sParams, errors, scriptXfer.m_Params ) )
	{
		gpwarningf(( "SiegeFx: error - cannot run script \"%s\", script params invalid \"%s\"\n", sName, sParams ));
		return ( false );
	}

	// get some vars
	DWORD seed = RandomDword();
	SFxSID script_id = GetNextScriptID( server, UsesSignals( sName ) );

	// only do this net stuff if we need to
	GoHandle owner( ScriptOwner );
	if ( server && owner && owner->IsGlobalGo() && ::IsServerLocal() && ::IsMultiPlayer() )
	{
		// ok we have to target this thing correctly. in order for us to run the
		// script, every single go involved must be created or pending on the client
		// machine.

		// start out assuming everyone gets it
		FrustumId frustumMask = (FrustumId)0xFFFFFFFF;

		// check owner first
		if ( FilterFrustumByGo( frustumMask, ScriptOwner ) )
		{
			// get the union of all our targets' frustum memberships
			TattooTracker::TargetColl::const_iterator i,
					ibegin = targets->GetTargetsBegin(), iend = targets->GetTargetsEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				const Flamethrower_target* target = *i;

				// if it has no goid, don't mess with mask
				UINT32 targetId = target->GetID();
				if ( targetId == 0 )
				{
					continue;
				}

				// reuse should-persist to get at whether or not it's got a goid
				bool hasGoid = false;
				target->ShouldPersist( hasGoid );
				if ( !hasGoid )
				{
					continue;
				}

				// ok now filter it and abort if it's completely masked off
				if ( !FilterFrustumByGo( frustumMask, MakeGoid( targetId ) ) )
				{
					break;
				}
			}

			// only bother with net if we at least had a few hits
			if ( frustumMask != FRUSTUMID_INVALID )
			{
				// this returns false if we can broadcast instead
				FuBi::AddressColl addresses;
				if ( !gGoDb.FillAddressesForFrustum( frustumMask, addresses ) && !addresses.empty() )
				{
					addresses.clear();
					addresses.push_back( RPC_TO_OTHERS );
				}

				// send 'em
				if ( !addresses.empty() )
				{
					// build xfer for net transport
					scriptXfer.m_ScriptId    = script_id;
					scriptXfer.m_Name        = sName;
					scriptXfer.m_ScriptOwner = ScriptOwner;
					scriptXfer.m_Seed        = seed;
					scriptXfer.m_Type        = type;
					scriptXfer.m_Xfer        = xfer;
					scriptXfer.m_Targets     = targets;

					// build data for xfer
					FuBi::BitPacker packer;
					scriptXfer.Xfer( packer );

					// now send through the net
					FuBi::AddressColl::const_iterator j, jbegin = addresses.begin(), jend = addresses.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						FUBI_SET_RPC_PEEKADDRESS( *j );
						RCRunScriptPacker( packer.GetSavedBits() );
					}

					// give targets back
					targets = scriptXfer.m_Targets;
				}
			}
		}
	}

	// now do it locally too and return the id
	return ( RunScript( sName, targets, scriptXfer.m_Params, 0, ScriptOwner, seed, type, script_id, xfer ) );
}


void
Flamethrower_interpreter::RCRunScriptPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_PEEKADDRESS( RCRunScriptPacker );

	RunScriptXfer scriptXfer;

	FuBi::BitPacker packer( packola );
	scriptXfer.Xfer( packer );

	// we may have gotten this twice due to xfer + rpc crossing paths so make
	// sure it's not already there
	Flamethrower_script *pScript = NULL;
	if( !FindScript( scriptXfer.m_ScriptId, &pScript ) )
	{
		RunScript( scriptXfer.m_Name, scriptXfer.m_Targets, scriptXfer.m_Params, 0, scriptXfer.m_ScriptOwner, scriptXfer.m_Seed, scriptXfer.m_Type, scriptXfer.m_ScriptId, scriptXfer.m_Xfer );
	}
}


SFxSID
Flamethrower_interpreter::RunScript( const char *sName, def_tracker targets, const Flamethrower_params & params, UINT32 start_param, Goid ScriptOwner, UINT32 seed, eWorldEvent type, bool xfer )
{
	SFxSID script_id = GetNextScriptID( false, UsesSignals( sName ) );
	return ( RunScript( sName, targets, params, start_param, ScriptOwner, seed, type, script_id, xfer ) );
}


SFxSID
Flamethrower_interpreter::RunScript( const char *sName, def_tracker targets, const Flamethrower_params & params, UINT32 start_param, Goid ScriptOwner, UINT32 seed, eWorldEvent type, SFxSID script_id, bool xfer )
{
	Flamethrower_script	*pScript = NULL;

	if( CreateNewScript( sName, params, start_param, targets, &pScript ) )
	{
		m_RunQueue.push_back( pScript );

		pScript->m_ID				= script_id;

		pScript->SetOwner			( ScriptOwner );
		pScript->SetSeed			( seed );

		gWorldFx.RegisterEffectScript( ScriptOwner, script_id, type, xfer, &params );
		pScript->SetRegistrationEvent( type );

		return script_id;
	}

	gperrorf(( "SiegeFx: error - script \"%s\" does not exist\n", sName ));

	return SFxSID_INVALID;
}


void
Flamethrower_interpreter::SStopScript( SFxSID id )
{
	if( IsGlobalSFxSID( id ) )
	{
		CHECK_SERVER_ONLY;  // moved here for #10796

		RCStopScript( id );
	}
	else
	{
		StopScript( id );
	}
}


void
Flamethrower_interpreter::SStopScript( Goid id, const char *sName )
{
	def_script_id_map::iterator iScript = m_RunningScripts.begin(), script_end = m_RunningScripts.end();

	for( ; iScript != script_end; ++iScript )
	{
		if( ((*iScript).second->GetOwner() == id) && ((*iScript).second->GetName().same_no_case( sName )) )
		{
			SStopScript( (*iScript).second->GetID() );
			return;
		}
	}
}


void
Flamethrower_interpreter::RCStopScript( SFxSID id )
{
	FUBI_RPC_CALL( RCStopScript, RPC_TO_ALL );

	StopScript( id );
}


void
Flamethrower_interpreter::StopScript( SFxSID id )
{
	m_StopQueue.push_back( id );
}


void
Flamethrower_interpreter::StopScript( Goid id, const char *sName )
{
	def_script_id_map::iterator iScript = m_RunningScripts.begin(), script_end = m_RunningScripts.end();

	for( ; iScript != script_end; ++iScript )
	{
		if( ((*iScript).second->GetOwner() == id) && ((*iScript).second->GetName().same_no_case( sName )) )
		{
			StopScript( (*iScript).second->GetID() );
			return;
		}
	}
}


void
Flamethrower_interpreter::StopAllScripts( bool bImmediate )
{
	def_script_id_map::iterator script = m_RunningScripts.begin(), script_end = m_RunningScripts.end();
	for(; script != script_end; ++script )
	{
		StopScript( (*script).second->GetID() );
	}

	for( script = m_DisabledScripts.begin(), script_end = m_DisabledScripts.end(); script != script_end; ++script )
	{
		StopScript( (*script).second->GetID() );
	}

	if( bImmediate )
	{
		def_script_id_list::iterator script_id = m_StopQueue.begin(), script_id_end = m_StopQueue.end();
		for(; script_id != script_id_end; ++script_id )
		{
			StopScriptNow( *script_id );
		}
		m_StopQueue.clear();
	}
}


void
Flamethrower_interpreter::FinishScript( SFxSID id )
{
	m_FinishQueue.push_back( id );
}


bool
Flamethrower_interpreter::FindScript( SFxSID id, Flamethrower_script **pScript )
{
	if( FindScriptObj( id, pScript, m_RunningScripts ) )
	{
		return ( true );
	}

	if ( FindScriptObj( id, pScript, m_DisabledScripts ) )
	{
		return ( true );
	}

	def_script_ptr_list::const_iterator i, ibegin = m_RunQueue.begin(), iend = m_RunQueue.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i)->GetID() == id )
		{
			*pScript = *i;
			return ( true );
		}
	}

	return ( false );
}


bool
Flamethrower_interpreter::IsScriptRunning( SFxSID id )
{
	Flamethrower_script *pScript = NULL;

	if( FindScriptObj( id, &pScript, m_RunningScripts ) )
	{
		return ( id == pScript->GetID() );
	}

	return false;
}


bool
Flamethrower_interpreter::ShouldScriptXfer( SFxSID id )
{
	// only bother if it's not stopping
	if ( stdx::has_any( m_StopQueue, id ) )
	{
		return ( false );
	}
	if ( stdx::has_any( m_FinishQueue, id ) )
	{
		return ( false );
	}

	return ( true );
}


Goid
Flamethrower_interpreter::GetRegistrationGoid( SFxSID id )
{
	// Check the run queue first
	Flamethrower_script *pScript = NULL;

	if( !FindScriptObj( id, &pScript, m_RunQueue ) && !FindScriptObj( id, &pScript, m_RunningScripts ) )
	{
		return GOID_INVALID;
	}

	Flamethrower_target *pTarget = NULL;

	if( pScript->GetDefaultTargets()->GetTarget( TattooTracker::TARGET, &pTarget ) )
	{
		return MakeGoid( pTarget->GetID() );
	}

	return GOID_INVALID;
}


void
Flamethrower_interpreter::UnloadScript( SFxSID id )
{
	m_UnloadQueue.push_back( id );
}


void
Flamethrower_interpreter::LoadScript( SFxSID id )
{
	m_LoadQueue.push_back( id );
}


void
Flamethrower_interpreter::RebuildScriptVocabulary()
{
	def_script_name_map::iterator it = m_ParentScripts.begin(), it_end = m_ParentScripts.end();

	for( ; it != it_end; ++it )
	{
		delete (*it).second;
	}
	m_ParentScripts.clear();

	BuildScriptVocabulary();
}

void
Flamethrower_interpreter::PostSignal( SFxSID id, const gpstring &sSignal, float timeout )
{
	if( m_SignalInsQueue.size() > 400 )
	{
#if !GP_RETAIL
		gpstring sError( "Flamethrower_interpreter::PostSignal() insertion queue is > 400 messages within this frame!\n\nDump:\n" );

		SignalColl::iterator it = m_SignalInsQueue.begin(), it_end = m_SignalInsQueue.end();
		for( ; it != it_end; ++it )
		{
			Flamethrower_script *pScript = NULL;

			sError.appendf( "msg: %s - script id: %d", (*it).m_sTag.c_str(), (UINT32)(*it).m_id );

			if( FindScript( (*it).m_id, &pScript ) )
			{
				GoHandle hOwner( pScript->GetOwner() );
				if( hOwner )
				{
					sError.appendf( " - name: %s - owner: %s - owner goid: %x", 
									pScript->GetName().c_str(),
									hOwner->GetTemplateName(), 
									(UINT32)hOwner->GetGoid()
									);
				}
			}
			sError.append( "\n" );
		}

		gperror( sError );
#endif
		return;
	}

	Signal new_signal( id, sSignal, timeout );
	m_SignalInsQueue.push_back( new_signal );
}


bool
Flamethrower_interpreter::CheckForSignal( SFxSID id, const gpstring &sSignal, bool bRemove )
{
	SignalColl::iterator iSig = m_Signals.begin(), sig_end = m_Signals.end();
	for( ; iSig != sig_end; ++iSig )
	{
		const SFxSID test_id  = (*iSig).m_id;

		if( ((test_id == 0) || (test_id == id)) && (*iSig).m_sTag.same_no_case( sSignal ) )
		{
			if( bRemove )
			{
				(*iSig).m_duration = 0;
			}
			return true;
		}
	}
	return false;
}


bool
Flamethrower_interpreter::SetScriptCallback( SFxSID id, const CBFunctor1< SFxSID > callback )
{
	// First try go through the list of scripts that are about ready to run
	def_script_ptr_list::iterator script = m_RunQueue.begin(), script_end = m_RunQueue.end();

	for( ; script != script_end; ++script )
	{
		if( id == (*script)->GetID() )
		{
			(*script)->m_Callback = callback;
			return true;
		}
	}

	// If it wasn't found in the run queue then check the currently running scripts
	def_script_id_map::iterator iscript = m_RunningScripts.find( id );

	if( iscript == m_RunningScripts.end() )
	{
		return false;
	}

	// Found the script so set the callback
	(*iscript).second->m_Callback = callback;

	return true;
}


void
Flamethrower_interpreter::Update( float SecondsElapsed, float UnPausedSecondsElapsed )
{
	UNREFERENCED_PARAMETER( UnPausedSecondsElapsed );

	m_SecondsElapsed = SecondsElapsed;
	m_UnPausedSecondsElapsed = UnPausedSecondsElapsed;

	// Process unloadable scripts
	if( !m_UnloadQueue.empty() )
	{
		def_script_id_list::iterator script = m_UnloadQueue.begin(), script_end = m_UnloadQueue.end();
		for( ; script != script_end; ++script )
		{
			UnloadScriptNow( *script );
		}
		m_UnloadQueue.clear();
	}

	// Process loadable scripts
	if( !m_LoadQueue.empty() )
	{
		def_script_id_list::iterator script = m_LoadQueue.begin(), script_end = m_LoadQueue.end();
		for( ; script != script_end; ++script )
		{
			LoadScriptNow( *script );
		}
		m_LoadQueue.clear();
	}

	// Process runnable scripts
	if( !m_RunQueue.empty() )
	{
		def_script_ptr_list runQueue;
		m_RunQueue.swap( runQueue );

		def_script_ptr_list::iterator script = runQueue.begin(), script_end = runQueue.end();
		for( ; script != script_end; ++script )
		{
			bool shouldDelete;
			if( (*script)->GetDefaultTargets()->IsValid( &shouldDelete ) )
			{
				m_RunningScripts.insert( std::make_pair( (*script)->GetID(), *script ) );
			}
			else if ( shouldDelete )
			{
				delete (*script);
			}
			else
			{
				UnloadScriptNow( *script );
			}
		}
	}

	// Process finishable scripts
	if( !m_FinishQueue.empty() )
	{
		def_script_id_list::iterator script_id = m_FinishQueue.begin(), script_id_end = m_FinishQueue.end();
		for(; script_id != script_id_end; ++script_id )
		{
			FinishScriptNow( *script_id );
		}
		m_FinishQueue.clear();
	}

	// Process stoppable scripts
	if( !m_StopQueue.empty() )
	{
		def_script_id_list::iterator script_id = m_StopQueue.begin(), script_id_end = m_StopQueue.end();
		for(; script_id != script_id_end; ++script_id )
		{
			StopScriptNow( *script_id );
		}
		m_StopQueue.clear();
	}

	// Process signal insertion queue
	if( !m_SignalInsQueue.empty() )
	{
		SignalColl::iterator iSig = m_SignalInsQueue.begin(), i_end = m_SignalInsQueue.end();
		for(; iSig != i_end; ++iSig )
		{
			m_Signals.push_back( *iSig );
		}
		m_SignalInsQueue.clear();
	}

	// Update scripts
	def_script_id_map::iterator script = m_RunningScripts.begin(), script_end = m_RunningScripts.end();
	for( ; script != script_end; ++script )
	{
		UpdateScript( (*script).second );
	}


	// Remove any expired signals
	SignalColl::iterator iSig = m_Signals.begin();

	while( iSig != m_Signals.end() )
	{
		(*iSig).m_duration -= SecondsElapsed;

		if( !(*iSig).m_sTag.empty() )
		{
			if( (*iSig).m_sTag[0] != '@' )
			{
				(*iSig).m_duration = 0;
			}
		}

		if( (*iSig).m_duration <= 0 )
		{
			iSig = m_Signals.erase( iSig );
		}
		else
		{
			++iSig;
		}
	}
}


SFxEID
Flamethrower_interpreter::GetNextEffectID( SFxSID script_id )
{
	if( SFxSID_INVALID == script_id )
	{
		return SFxEID_INVALID;
	}

	Flamethrower_script *pScript = NULL;

	if( FindScript( script_id, &pScript ) )
	{
		SFxEID id = pScript->GetNextEffectID();

		if( (DWORD)id > 0x000000FF )
		{
			id = 0;
		}

		pScript->SetNextEffectID( (SFxEID)((DWORD)id + 1) );

		return ( (SFxEID)((DWORD)script_id | (DWORD)id) );
	}
	return SFxEID_INVALID;
}


SFxSID
Flamethrower_interpreter::GetNextScriptID( bool global, bool bUsesSignals )
{
	if( global )
	{
		m_GlobalScriptID = (SFxSID)((DWORD)m_GlobalScriptID + 0x00000200);

		if( (DWORD)m_GlobalScriptID < 0x80000000 )
		{
			m_GlobalScriptID = (SFxSID)0x80000000;
		}

		if( bUsesSignals )
		{
			return (SFxSID)((DWORD)m_GlobalScriptID | 0x00000100);
		}

		return m_GlobalScriptID;
	}

	m_LocalScriptID = (SFxSID)((DWORD)m_LocalScriptID + 0x00000200);

	if( (DWORD)m_LocalScriptID > 0x7FFFFF00 )
	{
		m_LocalScriptID = (SFxSID)0;
	}

	if( bUsesSignals )
	{
		return (SFxSID)((DWORD)m_LocalScriptID | 0x00000100);
	}

	return m_LocalScriptID;
}


bool
Flamethrower_interpreter::GetMacroTokenIndex( const char *sMacro, UINT32 & token )
{
	def_macro_list::iterator it = m_MacroList.begin(), it_end = m_MacroList.end();

	for( ; it != it_end; ++it )
	{
		if( (*it)->Name().same_no_case( sMacro ) )
		{
			token = (*it)->GetTokenIndex();
			return true;
		}
	}
	return false;
}


bool
Flamethrower_interpreter::GetCommandTokenIndex( const char *sCommand, UINT32 & token )
{
	def_command_list::iterator it = m_CommandList.begin(), it_end = m_CommandList.end();

	for( ; it != it_end; ++it )
	{
		if( (*it)->Name().same_no_case( sCommand ) )
		{
			token = (*it)->GetTokenIndex();
			return true;
		}
	}
	return false;
}


bool
Flamethrower_interpreter::GetParamTokenIndex( const char *sCommand, const char *sParam,	UINT32 & token )
{
	def_command_list::iterator it = m_CommandList.begin(), it_end = m_CommandList.end();

	for( ; it != it_end; ++it )
	{
		if( (*it)->Name().same_no_case( sCommand ) )
		{
			return ( (*it)->GetParamToken( sParam, token ) );
		}
	}
	return false;
}




#if !GP_RETAIL


gpstring
Flamethrower_interpreter::GetAvailableScripts()
{
	gpstring sOutput;
	gpstring sHeader( "= Available Scripts =" );

	sOutput.append( 91, '-' ); sOutput.append( "\n" );
	sOutput.append( 45 - sHeader.size()/2, ' ' );
	sOutput.append( sHeader ); sOutput.append( "\n" );
	sOutput.append( 91, '-' ); sOutput.append( "\n" );

	// Determine max field length
	def_script_name_map::iterator it = m_ParentScripts.begin(), iend = m_ParentScripts.end();

	UINT32 field_size = 0;
	for(; it != iend; ++it )
	{
		field_size = max_t( (*it).second->GetName().size(), field_size );
	}

	const UINT32 names_per_line = (UINT32)floor(91/field_size);

	UINT32 name_count = 0;
	for( it = m_ParentScripts.begin(); it != iend; ++it )
	{
		sOutput.append( (*it).second->GetName() );
		sOutput.append( 91/names_per_line - (*it).second->GetName().size()+1, ' ' );
		if( ++name_count == names_per_line )
		{
			name_count = 0;
			sOutput.append( "\n" );
		}
	}

	if( name_count != 0 )
	{
		sOutput.append( "\n" );
	}
	sOutput.appendf( "Total scripts = %d\n", m_ParentScripts.size() );

	return sOutput;
}


gpstring
Flamethrower_interpreter::GetActiveScripts()
{
	gpstring sActiveScripts;

	if( !m_RunningScripts.empty() )
	{
		sActiveScripts += "Active Scripts:\n---------------\n";

		def_script_id_map::iterator script = m_RunningScripts.begin(), script_end = m_RunningScripts.end();

		char szNumber[64];
		memset( szNumber, 0, sizeof( szNumber ) );

		for( ; script != script_end; ++script )
		{
			if( IsGlobalSFxSID( (*script).second->GetID() ) )
			{
				sActiveScripts.appendf( "id: %u - [%s] *GLOBAL*\n", (*script).second->GetID(), (*script).second->GetName().c_str() );
			}
			else
			{
				sActiveScripts.appendf( "id: %u - [%s] \n", (*script).second->GetID(), (*script).second->GetName().c_str() );
			}
		}
		sActiveScripts.appendf( "Total active scripts = %d\n", m_RunningScripts.size() );
	}

	if( !m_DisabledScripts.empty() )
	{
		sActiveScripts.appendf( "Total unloaded scripts = %d\n", m_DisabledScripts.size() );
	}

	return sActiveScripts;
}


#endif // !GP_RETAIL


bool script_less( Flamethrower_script *pA, Flamethrower_script *pB )
{
	return ( pA->GetName().compare_no_case( pB->GetName() ) < 0 );
}


bool
Flamethrower_interpreter::FindScriptObj( SFxSID id, Flamethrower_script **pScript, const def_script_id_map &map )
{
	def_script_id_map::const_iterator it = map.find( id );

	if( it == map.end() )
	{
		return false;
	}

	*pScript = (*it).second;

	return true;
}


bool
Flamethrower_interpreter::FindScriptObj( SFxSID id, Flamethrower_script **pScript, const def_script_ptr_list &list )
{
	def_script_ptr_list::const_iterator it = list.begin(), it_end = list.end();

	for(; it != it_end; ++it )
	{
		if( (*it)->GetID() == id )
		{
			*pScript = *it;
			return true;
		}
	}
	return false;
}


bool
Flamethrower_interpreter::UsesSignals( const char *sName )
{
	def_script_name_map::iterator iScript = m_ParentScripts.find( sName );

	if( iScript != m_ParentScripts.end() )
	{
		return (*iScript).second->GetUsesSignals();
	}

	return false;
}


bool
Flamethrower_interpreter::AddMacro( Flamethrower_macro *pMacro )
{
	def_macro_list::iterator macro = m_MacroList.begin(), macro_end = m_MacroList.end();

	for( ; macro != macro_end; ++macro )
	{
		if( (*macro)->Name().same_no_case( pMacro->Name() ) )
		{
			return false;
		}
	}

	pMacro->SetTokenIndex( m_NextMacroTokenIndex );
	m_MacroList.push_back( pMacro );

	++m_NextMacroTokenIndex;

	return true;
}


bool
Flamethrower_interpreter::AddCommand( Flamethrower_command *pCommand )
{
	def_command_list::iterator command = m_CommandList.begin(), command_end = m_CommandList.end();

	for( ; command != command_end; ++command )
	{
		if( (*command)->Name().same_no_case( pCommand->Name() ) )
		{
			return false;
		}
	}

	pCommand->SetTokenIndex( m_NextCommandTokenIndex );
	m_CommandList.push_back( pCommand );

	++m_NextCommandTokenIndex;

	return true;
}


void
Flamethrower_interpreter::BuildMacroVocabulary( void )
{
	// FM_nop must be defined first
	AddMacro	( new FM_nop					( "NOP"						) );

	AddMacro	( new FM_pop					( "POP"						) );
	AddMacro	( new FM_peek					( "PEEK"					) );
	AddMacro	( new FM_true					( "TRUE"					) );
	AddMacro	( new FM_false					( "FALSE"					) );
	AddMacro	( new FM_null_target			( "NULL_TARGET"				) );
	AddMacro	( new FM_source					( "SOURCE"					) );
	AddMacro	( new FM_target_kb				( "TARGET_KB"				) ); 	// Todo: doc this
	AddMacro	( new FM_source_kb				( "SOURCE_KB"				) ); 	// Todo: doc this
	AddMacro	( new FM_target					( "TARGET"					) );
	AddMacro	( new FM_default_timeout		( "DEFAULT_TIMEOUT"			) );
	AddMacro	( new FM_no_collision			( "NO_COLLISION"			) );
	AddMacro	( new FM_object_collision		( "OBJECT_COLLISION"		) );
	AddMacro	( new FM_terrain_collision		( "TERRAIN_COLLISION"		) );
	AddMacro	( new FM_source_position		( "SOURCE_POSITION"			) );
	AddMacro	( new FM_target_position		( "TARGET_POSITION"			) );
	AddMacro	( new FM_camera_position		( "CAMERA_POSITION"			) );
	AddMacro	( new FM_target_goid			( "TARGET_GOID"				) );
	AddMacro	( new FM_source_goid			( "SOURCE_GOID"				) );
	AddMacro	( new FM_owner_goid				( "OWNER_GOID"				) );
	AddMacro	( new FM_invalid_goid			( "INVALID_GOID"			) );
	AddMacro	( new FM_weapon_hand_name		( "WEAPON_HAND_NAME"		) );
	AddMacro	( new FM_shield_hand_name		( "SHIELD_HAND_NAME"		) );
	AddMacro	( new FM_caster_name			( "CASTER_NAME"				) );
	AddMacro	( new FM_attack_class			( "ATTACK_CLASS"			) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_dagger		( "WEAPON_TYPE_DAGGER"		) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_axe		( "WEAPON_TYPE_AXE"			) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_blunt		( "WEAPON_TYPE_BLUNT"		) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_staff		( "WEAPON_TYPE_STAFF"		) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_sword		( "WEAPON_TYPE_SWORD"		) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_bow		( "WEAPON_TYPE_BOW"			) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_minigun	( "WEAPON_TYPE_MINIGUN"		) ); 	// Todo: doc this
	AddMacro	( new FM_weapon_type_projectile	( "WEAPON_TYPE_PROJECTILE"	) ); 	// Todo: doc this
	AddMacro	( new FM_lodfi					( "LODFI"					) ); 	// Todo: doc this
	AddMacro	( new FM_red_blood				( "RED_BLOOD"				) ); 	// Todo: doc this
	AddMacro	( new FM_no_blood				( "NO_BLOOD"				) ); 	// Todo: doc this
}


void
Flamethrower_interpreter::BuildCommandVocabulary( void )
{
	AddCommand	( new FC_Call					( "call"        ) );
	AddCommand	( new FC_Camerashake			( "camerashake" ) );
	AddCommand	( new FC_Exit					( "exit"        ) );
	AddCommand	( new FC_Frandrange				( "frandrange"  ) );
	AddCommand	( new FC_Get					( "get"         ) );
	AddCommand	( new FC_Mode					( "mode"        ) );
	AddCommand	( new FC_Pause					( "pause"       ) );
	AddCommand	( new FC_Randrange				( "randrange"   ) );
	AddCommand	( new FC_Sfx					( "sfx"         ) );
	AddCommand	( new FC_Signal					( "signal"		) );
	AddCommand	( new FC_Sound					( "sound"       ) );
	AddCommand	( new FC_Worldmsg				( "worldmsg"    ) );

	Flamethrower_command *pSet = new FC_Set( "set" );
	AddCommand	( pSet );
	m_TokenSet = pSet->GetTokenIndex();

	Flamethrower_command *pWaitfor = new FC_Waitfor( "waitfor" );
	AddCommand	( pWaitfor );
	m_TokenWaitfor = pWaitfor->GetTokenIndex();
}


void
Flamethrower_interpreter::BuildScriptVocabulary( void )
{
	if( m_sScriptAddress.empty() )
	{
		return;
	}

	FastFuelHandle effects_parent( m_sScriptAddress );

	FastFuelHandleColl effects;
	effects_parent.ListChildrenNamed( effects, "effect_script*" );

	FastFuelHandleColl::iterator i = effects.begin(), i_end = effects.end();
	gpstring sName, sScript;

	UINT32	missing_names = 0;
	UINT32	missing_scripts = 0;
	UINT32	bad_scripts = 0;
	m_ScriptErrors = 0;

	m_TotalScripts = 0;
	m_TotalLines   = 0;

	for( ; i != i_end; ++i )
	{
		if( i->Get( "name", sName ) )
		{
			if( i->Get( "script", sScript ) )
			{
				gpstring sErrors;
				if( !AddScript( sName, sScript, sErrors ) )
				{
					++bad_scripts;
					if( !sErrors.empty() )
					{
						gpwarningf(("SiegeFx: errors for script \"%s\":\n%s\n", sName.c_str(), sErrors.c_str() ));
					}
				}
				else
				{
					++m_TotalScripts;
				}
			}
			else
			{
				++missing_scripts;
				continue;
			}
		}
		else
		{
			++missing_names;
			continue;
		}
	}

	if( !!bad_scripts || !!missing_scripts || !!missing_names )
	{
		gpwarning( "\n=======================\nSiegeFx: error report\n=======================\n" );
		gpwarningf(( "bad scripts ...... %u\ntotal errors ..... %u\n", bad_scripts, m_ScriptErrors  ));
		gpwarningf(( "line count ....... %u\ntotal scripts .... %u\n", m_TotalLines, m_TotalScripts ));
		gpwarningf(( "missing names .... %u\nmissing scripts .. %u\n", missing_names, missing_scripts ));
	}

	effects_parent.Unload();
}


bool
Flamethrower_interpreter::CreateNewScript( const char * sName, const Flamethrower_params & script_params, UINT32 start_param, def_tracker targets, Flamethrower_script **pScript )
{
	if( sName == NULL )
	{
		gperror( "Flamethrower_interpreter::CreateNewScript - was passed a NULL pointer for a script name!" );
		return false;
	}

	def_script_name_map::iterator script = m_ParentScripts.find( sName );

	if( script == m_ParentScripts.end() )
	{
		return false;
	}

	*pScript = new Flamethrower_script( *((*script).second) );
	(*pScript)->SetDefaultTargets( targets );

	// where errors will be dumped to
	gpstring errors;

	// the 1 on the end here means that it will skip the first param since this was called using calls params and param 0 is the script name
	return ReplaceScriptParams( *pScript, script_params, start_param );
}


bool
Flamethrower_interpreter::TextToParams( const char * sParams, gpstring & errors, Flamethrower_params & script_params )
{
	UINT32 bc = 0;

	// determine the type that is getting stored
	Flamethrower_params::PARAM_TYPE pt = Flamethrower_params::PT_INVALID;

	for( UINT32 scan = 0, start = 0; sParams[ scan ] != 0; ++scan )
	{
		if( (Flamethrower_params::PT_INVALID == pt) && (sParams[ scan + 1] != 0) && ( sParams[ scan + 1 ] == '<' ) )
		{
			switch( sParams[ scan ] )
			{
				case 'i':	// store as int
				{
					pt = Flamethrower_params::PT_INT;
				} break;

				case 'u':	// store as uint32
				{
					pt = Flamethrower_params::PT_UINT32;
				} break;

				case 'f':	// store as float
				{
					pt = Flamethrower_params::PT_FLOAT;
				} break;

				case 's':	// store as string
				{
					pt = Flamethrower_params::PT_STRING;
				} break;

				case 'v':	// store as vector
				{
					pt = Flamethrower_params::PT_VECTOR;
				} break;

				case 'p':	// store as position
				{
					pt = Flamethrower_params::PT_POSITION;
				} break;
			}
		}

		if( (sParams[ scan ] == '[') || (sParams[ scan ] == '<') )
		{
			bc++;
			start = scan + 1;
		}
		else if( (sParams[ scan ] == ']') || (sParams[ scan ] == '>') )
		{
			--bc;

			if( pt == Flamethrower_params::PT_INVALID )
			{
				gpstring param;
				param.assign( (sParams + start), (scan - start) );

				StoreParam( param, errors, script_params );
			}
			else
			{
				StoreParam( sParams + start, errors, (scan - start), pt, script_params );
				pt = Flamethrower_params::PT_INVALID;
			}
		}
	}

	return ( bc == 0 );
}


bool
Flamethrower_interpreter::ReplaceScriptParams( Flamethrower_script * pScript, const Flamethrower_params & script_params, UINT32 start_param )
{
	// Get some temp storage
	char *pBuf = (char *)_alloca( 512 );
	char *pTemp = (char *)_alloca( 64 );

	Flamethrower_script::def_script::iterator line = pScript->m_Script.begin(), l_end = pScript->m_Script.end();
	for(; line != l_end; ++line )
	{
		if( ((*line).m_Type == Flamethrower_line::LT_COMMAND) || ((*line).m_Type == Flamethrower_line::LT_IF) )
		{
			for( UINT32 end = (*line).m_Params.Count(), line_param = 0; line_param < end; ++line_param )
			{
				Flamethrower_params::PARAM_TYPE pt = (*line).m_Params.GetType( line_param );

				// no logic here just replace the old text with the new
				if( (pt == Flamethrower_params::PT_SFX_PARAMS) || (pt == Flamethrower_params::PT_STRING) )
				{
					const char *pText = (pt == Flamethrower_params::PT_STRING) ? (*line).m_Params._gpstring( line_param ).c_str() 
									  : (*line).m_Params._sfxparam( line_param ).c_str();
					UINT32 bc = 0;
					UINT32 write_pos = 0;
					for( UINT32 begin = 0, end = 0; pText[ begin ] != 0; ++begin )
					{
						if( pText[ begin ] == '[' )
						{
							++bc;
							for( end = begin + 1; pText[ end ] != 0; ++end )
							{
								if( pText[ end ] == ']' )
								{
									// decrement the brace count
									--bc;

									// copy the text for the script parameter number
									const UINT32 len = (end - begin) - 1;
									::memcpy( pTemp, pText + (begin + 1), len );

									// null terminate
									pTemp[ len ] = 0;

									// now convert from ascii to a uint
									UINT32 script_param_index = 0;
									FromString( pTemp, script_param_index );

									// make sure this is a valid index to look up with otherwise just skip substitution
									if( script_param_index < script_params.Count() )
									{
										// lookup the script parameter using the index extracted from the text
										int new_offset = 0;
										script_params.Output( script_param_index + start_param, pBuf + write_pos, new_offset );

										write_pos += new_offset;
										pBuf[ write_pos ] = 0;

									}
									// skip ahead to the next point in the string
									begin = end;
									break;
								}
							}
						}
						else
						{
							// copy to the temp buffer
							pBuf[ write_pos ] = pText[ begin ];
							++write_pos;
						}
					}

					// null terminate
					pBuf[ write_pos ] = 0;

					// replace with new text if everything was ok
					if( bc == 0 )
					{
						// replace it with the new text
						if( pt == Flamethrower_params::PT_STRING )
						{
							(*line).m_Params._gpstring( line_param ) = pBuf;
						}
						else
						{
							(*line).m_Params._sfxparam( line_param ) = pBuf;
						}
					}
					else
					{
						gpwarningf(("SiegeFx: error - cannot run script \"%s\", script params invalid \"%s\"\n", pScript->GetName().c_str(), pText));

						delete pScript;
						return false;
					}
				}
				else if( (*line).m_Params.GetType( line_param ) == Flamethrower_params::PT_SCRIPT_PARAM )
				{
					// convert the script param token into the actual param
					UINT32 script_param_index = (*line).m_Params._sparam( line_param );

					if( (script_param_index + start_param) >= script_params.Count() )
					{
						gpwarningf(("SiegeFx: error - script \"%s\", requires more than %u parameters\n", pScript->GetName().c_str(), script_params.Count()-start_param));

						delete pScript;
						return false;
					}
					else
					{
						// replace existing param with new param to be used later during execution
						(*line).m_Params.Replace( line_param, script_param_index + start_param, script_params );
					}
				}
			}
		}
	}
	return true;
}


bool
Flamethrower_interpreter::ScanScript( const char *pText, gpstring & errors , Flamethrower_script & script )
{
	// get the length
	const UINT32 max_offset = ::strlen( pText );

	bool bNoScanErrors = true;

	// setup the runners
	UINT32 begin = 0;
	UINT32 end = 0;

	while( (begin < max_offset) || (end < max_offset) )
	{
		// get a line to store into
		Flamethrower_line *pNewLine = &*script.m_Script.push_back();

		if( ScanLine( pText, errors, begin, end, script, *pNewLine ) )
		{
			if( pNewLine->m_Type == Flamethrower_line::LT_EOF )
			{
				break;
			}
			else if( pNewLine->m_Type == Flamethrower_line::LT_COMMENT )
			{
				script.m_Script.pop_back();
			}
			else
			{
				++m_TotalLines;
			}

			begin = end;
			pNewLine = NULL;
		}
		else
		{
			script.m_Script.pop_back();

			bNoScanErrors = false;
		}
	}

	// setup the block offsets
	if( bNoScanErrors )
	{
		stdx::fast_vector< UINT32* > block_offsets;

		Flamethrower_script::def_script::iterator i = script.m_Script.begin(), i_end = script.m_Script.end();

		for( UINT32 offset = 0; i != i_end; ++offset, ++i )
		{
			if( (*i).m_Type == Flamethrower_line::LT_BLOCK_BEGIN )
			{
				(*i).m_BlockSkipOffset = offset;
				block_offsets.push_back( &(*i).m_BlockSkipOffset );
			}
			else if ( (*i).m_Type == Flamethrower_line::LT_BLOCK_END )
			{
				stdx::fast_vector< UINT32* >::iterator iBlock = block_offsets.end();
				--iBlock;

				**iBlock = offset - **iBlock;
				block_offsets.pop_back();
			}
		}

		// tag the if/else blocks
		Flamethrower_script::def_script::iterator iBlockEnd = i_end, iNextLine = script.m_Script.begin();

		for( i = iNextLine; i != i_end; ++i )
		{
			if( (*i).m_Type == Flamethrower_line::LT_IF )
			{
				++i;

				if( (*i).m_Type == Flamethrower_line::LT_BLOCK_BEGIN )
				{
					iNextLine = i;
					i += (*i).m_BlockSkipOffset;

					if( (*i).m_Type == Flamethrower_line::LT_BLOCK_END )
					{
						iBlockEnd = i;
						++i;

						if( (*i).m_Type == Flamethrower_line::LT_ELSE )
						{
							(*iBlockEnd).m_Type = Flamethrower_line::LT_BLOCK_END_ELSE;
						}

						i = iNextLine;
						continue;
					}
				}
			}
			else if( ((*i).m_Type == Flamethrower_line::LT_ELSE) && ((*(i-1)).m_Type != Flamethrower_line::LT_BLOCK_END_ELSE ) )
			{
				errors.appendf( "error: unmatched else in script \"%s\"\n", script.GetName().c_str());
				bNoScanErrors = false;
			}
		}
	}

	return bNoScanErrors;
}


bool
Flamethrower_interpreter::ScanLine( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_script & script, Flamethrower_line & line )
{
	UINT32 start = begin;

	// skip whitespace
	for(; ((pText[ begin ] == ' ') || (pText[ begin ] == '\t') || (pText[ begin ] == '\r') || (pText[ begin ] == '\n')); ++begin );

	// skip blank line
	if( pText[ begin ] == ';' )
	{
		if( (pText[ begin + 1 ] == '\r' ) && pText[ begin + 2] == '\n' )
		{
			// skip return & linefeed
			begin += 3;
		}
		else
		{
			// scan to next position
			++begin;
		}
	}

	// check for the end of the file
	if( pText[ begin ] == 0 )
	{
		line.m_Type = Flamethrower_line::LT_EOF;
		return true;
	}

	// is this line the beginning of a block?
	if( pText[ begin ] == '{' )
	{
		UINT32 block_start = begin;

		if( ScanBlock( pText, errors, block_start, end ) )
		{
			line.m_Type = Flamethrower_line::LT_BLOCK_BEGIN;
			return true;
		}

		if( block_start == end )
		{
			begin = end;
		}

		return false;
	}

	// is this line the end of a block?
	if( pText[ begin ] == '}' )
	{
		end = begin+1;

		line.m_Type = Flamethrower_line::LT_BLOCK_END;
		return true;
	}

	// check to see if it is safe to scan ahead
	UINT32 sa = 4;
	for( UINT32 n = begin; (pText[ n ] != 0) && (sa != 0) ; ++n, --sa );
	if( sa != 0 )
	{
		errors.append( "error: premature end of line \"" );
		errors.append( (pText + start), (begin - start ) );
		errors.append( "\"\n" );
		++m_ScriptErrors;
		return false;
	}

	// is this line a // style comment?
	if( (pText[ begin + 0 ] == '/') && (pText[ begin + 1 ] == '/') )
	{
		begin += 2;
		for(; ((pText[ begin ] != '\r') && (pText[ begin ] != '\n')); ++begin );
		end = begin + 1;

		line.m_Type = Flamethrower_line::LT_COMMENT;
		return true;
	}

	// is this line a /* */ style comment?
	if( (pText[ begin ] == '/') && (pText[ begin + 1 ] == '*') )
	{
		begin += 2;
		for(; (pText[ begin ] != 0); ++begin )
		{
			if( (pText[ begin ] == '*') && (pText[ begin + 1 ] == '/') )
			{
				break;
			}
		}
		end = begin + 2;

		line.m_Type = Flamethrower_line::LT_COMMENT;
		return true;
	}

	// is this line an if?
	if( (pText[ begin + 0 ] == 'i') && (pText[ begin + 1 ] == 'f') )
	{
		UINT32 exp_start = begin;
		char *pExpression = (char *)_alloca( 256 );

		if( ScanExpression( pText, errors, exp_start, end, pExpression ) )
		{
			line.m_Params.Add( pExpression );
			line.m_Type = Flamethrower_line::LT_IF;
			return true;
		}

		// missing or invalid expression
		errors.append( "error: missing or invalid expression for if \"\n" );
		errors.append( (pText + start), (begin - start ) );
		errors.append( "\"\n" );
		++m_ScriptErrors;

		line.m_Type = Flamethrower_line::LT_INVALID;
		return false;
	}

	// is this line an else?
	if( ( (pText[ begin + 0 ] == 'e') && (pText[ begin + 1 ] == 'l') && (pText[ begin + 2 ] == 's') && (pText[ begin + 3 ] == 'e') ) )
	{
		end = (begin + 4);

		line.m_Type = Flamethrower_line::LT_ELSE;
		return true;
	}

	// is this line a command?
	if( ScanCommand( pText, errors, begin, end, script, line ) )
	{
		line.m_Type = Flamethrower_line::LT_COMMAND;
		return true;
	}

	line.m_Type = Flamethrower_line::LT_INVALID;
	return false;
}


bool
Flamethrower_interpreter::ScanBlock( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end )
{
	// scan for the beginning of the block
	for( ; (pText[ begin ] != 0) && (pText[ begin ] != '{'); ++begin );

	UINT32 bc = 0, scan = begin;
	UINT32 new_begin = begin;

	if( pText[ begin ] == '{' )
	{
		++new_begin;
		for( ; (pText[ scan ] != 0); ++scan )
		{
			if( pText[ scan ] == '{' )
			{
				++bc;
				continue;
			}
			else if ( pText[ scan ] == '}' )
			{
				--bc;
			}

			// skip over comments
			if( (pText[ scan ] == '/') && (pText[ scan + 1 ] != 0) && (pText[ scan + 1 ] == '/') )
			{
				for(; (pText[ scan ] != '\r') && (pText[ scan ] != '\n'); ++scan );
			}

			if( bc == 0 )
			{
				// ending brace reached from where scanning first started
				begin = new_begin - 1;
				end = new_begin;
				return true;
			}
		}
	}
	// give error about unmatching brackets
	errors.append( "error: missing closing brace {\n" );
	++m_ScriptErrors;

	end = scan;
	begin = end;
	return false;
}


bool
Flamethrower_interpreter::ScanExpression( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, char * sOut )
{
	UINT32 start = begin;

	for( ;(pText[ begin ] != '(') && (pText[ begin ] != ';') && (pText[ begin ] != 0); ++begin );

	UINT32 pc = 0, scan = begin;

	if( pText[ begin ] == '(' )
	{
		// find the end of the expression
		for( ; (pText[ scan ] != '{') || (pText[ scan ] != '}') || (pText[ scan ] != ';') || (pText[ scan ] != 0); ++scan )
		{
			// skip over comments
			if( (pText[ scan ] == '/') && (pText[ scan + 1 ] != 0) && (pText[ scan + 1 ] == '/') )
			{
				for(; (pText[ scan ] != '\r') && (pText[ scan ] != '\n'); ++scan );
			}

			// verify parens are paired
			if( pText[ scan ] == '(' )
			{
				++pc;
			}
			else if( pText[ scan ] == ')' )
			{
				--pc;
			}

			if( pText[ scan ] == '#' )
			{
				// embed macro tokens
				UINT32 i = scan + 1;
				for(; ((pText[ i ] != '<') && (pText[ i ] != '>') && (pText[ i ] != '(') && (pText[ i ] != ')') &&
					   (pText[ i ] != '+') && (pText[ i ] != '-') && (pText[ i ] != '*') && (pText[ i ] != '/') &&
					   (pText[ i ] != '|') && (pText[ i ] != '&') && (pText[ i ] != '^') && (pText[ i ] != '=') &&
					   (pText[ i ] != ' ') && (pText[ i ] != 0)); ++i );

				UINT32 macro_len = (i - scan);
				::memcpy( sOut, pText + scan, macro_len );
				sOut[ macro_len ] = 0;

				UINT32 token = 0;

				if( GetMacroTokenIndex( sOut+1, token ) )
				{
					sOut[ macro_len ] = ' ';
					++sOut;
					*sOut++ = (char)token;
					*sOut++ = ' ';

					scan += macro_len-1;
				}
				else
				{
					errors.appendf( "error: undefined macro #%s\n", sOut );
					++m_ScriptErrors;
				}
			}
			else
			{
				// no macro so just copy the character over
				*sOut++ = pText[ scan ];
			}

			if( pc == 0 )
			{
				// end expression reached - ignore and trailing closing parens, they should be detected as errors later
				end = scan + 1;

				// null terminate
				*sOut++ = 0;

				return true;
			}
		}

		// give error about badly formatted expression
		errors.append( "error: badly formatted expression \"" );
		errors.append( (pText + start), (scan - start) );
		errors.append( "\"\n" );
		++m_ScriptErrors;

		end = scan + 1;
		return false;
	}

	errors.append( "error: missing expression \"" );
	errors.append( (pText + start), (begin - start ) );
	errors.append( "\"\n" );
	++m_ScriptErrors;

	end = begin + 1;
	return false;
}


bool
Flamethrower_interpreter::ScanCommand( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_script & script, Flamethrower_line & line )
{
	UINT32 scan = begin;

	// scan to what should be the start of a command
	for( ;(pText[ scan ] == ' ') || (pText[ scan ] == '\t') || (pText[ scan ] == '\r') || (pText[ scan ] == '\n'); ++scan );

	end = scan;

	// scan to what should be the end of the command
	for( ;(pText[ end ] != ';') && (pText[ end ] != ' ') && (pText[ end ] != '\t') && (pText[ end ] != '\r') && (pText[ end ] != '\n') && (pText[ end ] != 0); ++end );

	if( pText[ end ] != 0 )
	{
		// copy the command as a string onto the stack and null terminate
		const size_t cmd_len = (end - scan);
		char* pCommand = (char *)_alloca( cmd_len + 1 );
		::memcpy( pCommand, (pText + scan), cmd_len );
		pCommand[ cmd_len ] = 0;

		if( GetCommandTokenIndex( pCommand, line.m_CommandIndex ) )
		{
			// is this a parameterless command?
			if( pText[ scan + cmd_len ] == ';' )
			{
				++end;
				return true;
			}

			// tag this script if it uses the special waitfor macro
			if( m_TokenWaitfor == line.m_CommandIndex )
			{
				script.SetUsesSignals( true );
			}

			scan += cmd_len;
			end = scan;

			for( ; (pText[ end ] != 0) && (pText[ end ] != ';') && (pText[ end ] != '}'); ++end )
			{
				if( ScanParams( pText, errors, scan, end, line ) )
				{
					if( pText[ end ] == ';' )
					{
						++end;
						break;
					}
					else
					{
						scan = end;
					}
				}
			}
			return true;
		}
	}

	// if it got here then command is invalid
	errors.append( "error: unknown command \"" );
	errors.append( (pText + scan), (end - scan ) );
	errors.append( "\"\n" );
	++m_ScriptErrors;

	begin = end + 1;

	return false;
}


bool
Flamethrower_interpreter::ScanParams( const char *pText, gpstring & errors, UINT32 & begin, UINT32 & end, Flamethrower_line & line )
{
	UINT32 scan = begin;

	// scan to the start of what should be a param
	for( ;(pText[ scan ] == ' ') || (pText[ scan ] == '\t') || (pText[ scan ] == '\r') || (pText[ scan ] == '\n'); ++scan );

	if( (pText[ scan ] == ';') || (pText[ scan ] == 0) )
	{
		end = ++scan;
		return true;
	}

	// prepare some pointers
	char * pTarget = (char*)_alloca( 512 );
	char * pBuf = (char *)_alloca( 128 );

	if( pText[ scan ] == '\"' )	// is it a string param?
	{
		char * pStart = pTarget;
		// scan to the end of the string param starting after first "
		for( end = scan; (pText[ end ] != '\"') && (pText[ end ] != ';') && (pText[ end ] != 0); ++end );

		// make sure we didn't go off the end of the string
		if( pText[ end ] == 0 )
		{
			errors.append( "error: premature end of line on string param \"\n" );
			errors.append( (pText + begin), (end - scan) );
			errors.append( "\"\n" );
			++m_ScriptErrors;

			return false;
		}

		*pTarget++ = '\"';
		const char * pSource = pText + scan;

		UINT32 i = 1;
		for(; pSource[ i ] != '\"'; ++i )
		{
			if( pSource[ i ] == '#' )	// check for a macro tag
			{	// embed macro tokens
				for( end = i + 1; ((pSource[ end ] != ' ') && (pSource[ end ] != ',') && (pSource[ end ] != ')') && (pSource[ end ] != '>') && (pSource[ end ] != 0)); ++end );

				UINT32 macro_len = (end - i) - 1;
				::memcpy( pBuf, pSource + (i + 1), macro_len );
				pBuf[ macro_len ] = 0;

				UINT32 token = 0;

				if( GetMacroTokenIndex( pBuf, token ) )
				{
					*pTarget++ = '#';
					*pTarget++ = (char)token;

					i += macro_len;
				}
				else
				{
					errors.appendf( "error: undefined macro #%s\n", pBuf );
					++m_ScriptErrors;
				}
			}
			else if( (pSource[ i ] != ' ') && (pSource[ i ] != '\t') && (pSource[ i ] != '\r') && (pSource[ i ] != '\n') )
			{// no macro so just copy excluding whitespace
				*pTarget++ = pSource[ i ];
			}
		}

		// null terminate
		*pTarget = 0;

		// skip past the string param and the quote one the end
		end = (scan + i) + 1;

		// restore the pointer
		pTarget = pStart;
	}
	else if( (pText[ scan ] == '<') || (pText[ scan + 1 ] == '<') )	// is it an untranslated script param?
	{
		char *pStart = pTarget;

		// scan to the end of param
		for( end = scan; (pText[ end ] != '>') && (pText[ end ] != ';') && (pText[ end ] != 0); ++end );

		if( pText[ end ] != '>' )
		{
			errors.append( "error: missing '>' for call param \"\n" );
			errors.append( (pText + begin), (end - scan) );
			errors.append( "\"\n" );
			++m_ScriptErrors;

			return false;
		}

		// embed the tokens
		const char * pSource = pText + scan;
		UINT32 i = 0;
		for(;  pSource[ i ] != '>'; ++i )
		{
			if( pSource[ i ] == '#' )	// check for a macro tag
			{	// embed macro tokens
				for( end = i + 1; ((pSource[ end ] != ' ') && (pSource[ end ] != ',') && (pSource[ end ] != ')') && (pSource[ end ] != '>') && (pSource[ end ] != 0)); ++end );

				UINT32 macro_len = (end - i) - 1;
				::memcpy( pBuf, pSource + (i + 1), macro_len );
				pBuf[ macro_len ] = 0;

				UINT32 token = 0;

				if( GetMacroTokenIndex( pBuf, token ) )
				{
					*pTarget++ = '#';
					*pTarget++ = (char)token;

					i += macro_len;
				}
				else
				{
					errors.appendf( "error: undefined macro #%s\n", pBuf );
					++m_ScriptErrors;
				}
			}
			else
			{
				*pTarget++ = pSource[ i ];
			}
		}
		*pTarget++ = pSource[ i ];
		*pTarget++ = 0;

		pTarget = pStart;
		end = (scan + i) + 1;
	}
	else
	{
		// scan to the end of param
		for( end = scan; (pText[ end ] != ' ') && (pText[ end ] != '\t') && (pText[ end ] != '\r') && (pText[ end ] != '\n') &&	(pText[ end ] != ';') && (pText[ end ] != 0); ++end );

		// make sure we didn't go off the end of the string
		if( pText[ end ] == 0 )
		{
			errors.append( "error: premature end of line \n" );
			++m_ScriptErrors;
			begin = end;

			return false;
		}

		// copy the param as a string onto the stack excluding spaces and null terminate
		UINT32 param_len = (end - scan);
		::memcpy( pTarget, (pText + scan), param_len );
		pTarget[ param_len ] = 0;
	}

	bool bSuccess = StoreParam( pTarget, errors, line.m_Params, m_CommandList[ line.m_CommandIndex ] );

	if( !bSuccess )
	{
		begin = end;
	}

	// store this param in the line
	return bSuccess;
}


template < class type > bool
ScanForNumber( char *pBuf, UINT32 & begin, type & val )
{
	UINT32 i = begin;

	if( !((pBuf[ i ] >= '0' ) && (pBuf[ i ] <= '9' )) || (pBuf[ i ] == '.') || (pBuf[ i ] == '-') )
	{
		// scan to the start
		for( ; (pBuf[ i ] != 0) && ((pBuf[ i ] == ' ') || (pBuf[ i ] == '\t') || (pBuf[ i ] == '\n') || (pBuf[ i ] == '\r')) ; ++i );
	}

	UINT32 start = i;

	// scan to the end
	UINT32 j = i;
	for( ; (pBuf[ j ] != 0) && ((pBuf[ j ] != ' ') && (pBuf[ j ] != '\t') && (pBuf[ j ] != '\r') && (pBuf[ j ] != '\n')); ++j );

	// terminate
	pBuf[ j ] = 0;

	// setup for the next scan if there is one
	begin = j + 1;

	// get the float
	return FromString( pBuf + start, val );
}

bool
Flamethrower_interpreter::StoreParam( const char *pText, gpstring & errors, UINT32 len, Flamethrower_params::PARAM_TYPE pt, Flamethrower_params & params )
{
	// get some temp memory
	char * pBuf = (char*)_alloca( 1024 );

	if( PrepareParam( pText, params, pt, len, pBuf ) )
	{
		return true;
	}

	::memcpy( pBuf, pText, len );
	pBuf[ len ] = 0;

 	switch( pt )
	{
		case Flamethrower_params::PT_INT:
		{
			int val = 0;
			if( FromString( pBuf, val ) )
			{
				params.Add( val );
				return true;
			}

			errors.appendf( "error: cannot store param \"%s\" as a int\n", pBuf );
			++m_ScriptErrors;
			return false;
		} break;

		case Flamethrower_params::PT_UINT32:
		{
			UINT32 val = 0;
			if( FromString( pBuf, val ) )
			{
				params.Add( val );
				return true;
			}

			errors.appendf( "error: cannot store param \"%s\" as a UINT32\n", pBuf );
			++m_ScriptErrors;
			return false;
		} break;

		case Flamethrower_params::PT_FLOAT:
		{
			float val = 0;
			if( FromString( pBuf, val ) )
			{
				params.Add( val );
				return true;
			}

			errors.appendf( "error: cannot store param \"%s\" as a float\n", pBuf );
			++m_ScriptErrors;
			return false;
		} break;

		case Flamethrower_params::PT_STRING:
		{
			params.Add( pBuf );
			return true;
		} break;

		case Flamethrower_params::PT_VECTOR:
		{
			vector_3 val;

			UINT32 scan = 0;

			if( !(ScanForNumber( pBuf, scan, val.x ) && ScanForNumber( pBuf, scan, val.y ) && ScanForNumber( pBuf, scan, val.z )) )
			{
				errors.appendf( "error: cannot store param \"%s\" as a float\n", pText );
				++m_ScriptErrors;

				return false;
			}

			params.Add( val );
			return true;

		} break;

		case Flamethrower_params::PT_POSITION:
		{
			vector_3 val;
			int node = 0;

			UINT32 scan = 0;

			if( ScanForNumber( pBuf, scan, val.x ) && ScanForNumber( pBuf, scan, val.y ) && ScanForNumber( pBuf, scan, val.z ) && ScanForNumber( pBuf, scan, node ) )
			{
				siege::database_guid guid( node );
				SiegePos position( val, guid );

				params.Add( position );
				return true;
			}
			errors.appendf( "error: cannot store param \"%s\" as a position\n", pText );
			++m_ScriptErrors;
		} break;
	}

	return false;
}


bool
Flamethrower_interpreter::StoreParam( const char *pText, gpstring & errors, Flamethrower_params & params, Flamethrower_command * pCommand )
{
	switch( pText[ 0 ] )
	{
		// store as a macro
		case '#':
		{
			UINT32 macro_index = 0;

			if( GetMacroTokenIndex( pText + 1, macro_index ) )
			{
				const UINT32 name = params.Add( macro_index );
				params.SetType( name, Flamethrower_params::PT_MACRO );

				return true;
			}

			// macro not found
			errors.appendf( "error: unknown macro %s\n", pText );
			++m_ScriptErrors;

		} break;


		// store as a sfx parameter string
		case '\"':
		{
			const UINT32 name = params.Add( pText + 1 );
			params.SetType( name, Flamethrower_params::PT_SFX_PARAMS );

			return true;

		} break;

		// store as a variable
		case '$':
		{
			const UINT32 name = params.Add( pText + 1 );
			params.SetType( name, Flamethrower_params::PT_VARIABLE );

			return true;

		} break;

		// store as a script param
		case '[':
		{
			UINT32 bc = 0;
			// script params are special since they can be right next to each other so parse them all
			for( UINT32 i = 0; pText[ i ] != 0; ++i )
			{
				if( pText[ i ] == '[' )
				{
					++bc;
					for( UINT32 j = i + 1; pText[ j ] != 0; ++j )
					{
						if( pText[ j ] == ']' )
						{
							--bc;

							char *pBuf = (char *)_alloca( 64 );
							::memcpy( pBuf, &pText[ i + 1 ], (j - i) );
							pBuf[ (j - i) - 1 ] = 0;

							const UINT32 param = atoi( pBuf );

							const UINT32 name = params.Add( param );
							params.SetType( name, Flamethrower_params::PT_SCRIPT_PARAM );

							// adjust for the next iteration
							i = j + 1;
						}
					}
				}
			}

			// make sure the brackets were all matched up otherwise error
			if( bc != 0 )
			{
				errors.appendf( "error: missing closing bracket \"%s\"\n", pText );
				++m_ScriptErrors;

				return false;
			}

			return true;

		} break;

		// store as a specific type
		case 'i':
		case 'u':
		case 'f':
		case 's':
		case 'v':
		case 'p':
		case '<':
		{
			if( pText[ 1 ] == '<' )	// store as a specific type
			{
				Flamethrower_params::PARAM_TYPE pt = Flamethrower_params::PT_INVALID;

				switch( pText[ 0 ] )
				{
					case 'i':	// store as int
					{
						pt = Flamethrower_params::PT_INT;
					} break;

					case 'u':	// store as uint32
					{
						pt = Flamethrower_params::PT_UINT32;
					} break;

					case 'f':	// store as float
					{
						pt = Flamethrower_params::PT_FLOAT;
					} break;

					case 's':	// store as string
					{
						pt = Flamethrower_params::PT_STRING;
					} break;

					case 'v':	// store as vector
					{
						pt = Flamethrower_params::PT_VECTOR;
					} break;

					case 'p':	// store as position
					{
						pt = Flamethrower_params::PT_POSITION;
					} break;
				}

				for( UINT32 scan = 0, start = 0, bc = 0; pText[ scan ] != 0; ++scan )
				{
					if( pText[ scan ] == '<' )
					{
						bc++;
						start = scan + 1;
					}
					else if( pText[ scan ] == '>' )
					{
						--bc;
						if( bc == 0 )
						{
							StoreParam( pText + start, errors, (scan - start), pt, params );
						}
						else
						{
							errors.appendf( "error: cannot store param \"%s\"\n", pText );
							return false;
						}
					}
				}
				return true;
			}
			else if( pText[ 0 ] == '<' )	// store as a default string type
			{
				char *pBuf = (char *)_alloca( 512 );

				// this call will automatically add the param if it is a deferred param
				if( PrepareParam( pText, params, Flamethrower_params::PT_STRING, ::strlen( pText ), pBuf ) )
				{
					return true;
				}

				const UINT32 name = params.Add( pText );
				params.SetType( name, Flamethrower_params::PT_STRING );

				return true;
			}
		}

		// The param could be either a command parameter or just a number so determine what it is
		default:
		{
			UINT32 token = 0;

			// see if it matches as a command parameter first and store it away if so
			if( pCommand && pCommand->GetParamToken( pText, token ) )
			{
				const UINT32 name = params.Add( token );
				params.SetType( name, Flamethrower_params::PT_COMMAND_PARAM );

				return true;
			}
			else
			{
				// see what kind of characters are stored in the text
				bool bNumber = false, bPeriod = false, bMinus = false, bOther = false;

				for( UINT32 scan = 0; pText[ scan ] != 0; ++scan )
				{
					char c = pText[ scan ];

					if((c >= '0') && (c <= '9'))
					{
						bNumber = true;
					}
					else if( c == '.' )
					{
						bPeriod = true;
					}
					else if(c == '-')
					{
						bMinus = true;
					}
					else
					{
						bOther = true;
						break;
					}
				}

				if( bNumber && !bOther )
				{
					if( bPeriod )
					{
						// Store as PT_FLOAT
						float val =  0;
						FromString( pText, val );

						params.Add( val );
					}
					else
					{
						if( bMinus )
						{
							// store as PT_INT
							int val = 0;
							FromString( pText, val );

							params.Add( val );
						}
						else
						{
							// store as a PT_UINT32
							UINT32 val = 0;
							FromString( pText, val );

							params.Add( val );
						}
					}
				}
				else
				{
					// store as PT_STRING
					params.Add( pText );
				}
			}
			return true;
		}
	}
	return false;
}


bool
Flamethrower_interpreter::PrepareParam( const char *pText, Flamethrower_params & params, Flamethrower_params::PARAM_TYPE pt, UINT32 len, char * pBuf )
{
	// determine if evaluation should be deferred
	bool bDeferredEvaluation = false;
	for( UINT32 i = 0; i != len; ++i )
	{
		const char c = pText[ i ];

		if( (c == '$') || (c == '#') )
		{
			bDeferredEvaluation = true;
			break;
		}
	}

	// prefix with the type this symbol will evaluate to if needed
	if( bDeferredEvaluation )
	{
		switch( pt )
		{
			case Flamethrower_params::PT_INT:
			{
				pBuf[0] = 'i';
			} break;

			case Flamethrower_params::PT_UINT32:
			{
				pBuf[0] = 'u';
			} break;

			case Flamethrower_params::PT_FLOAT:
			{
				pBuf[0] = 'f';
			} break;

			case Flamethrower_params::PT_STRING:
			{
				pBuf[0] = 's';
			} break;

			case Flamethrower_params::PT_VECTOR:
			{
				pBuf[0] = 'v';
			} break;

			case Flamethrower_params::PT_POSITION:
			{
				pBuf[0] = 'p';
			} break;
		}
		::memcpy( pBuf+1, pText, len );
		pBuf[ len+1 ] = 0;

		// add it to the params structure as a string
		UINT32 name = params.Add( pBuf );

		// tag it for evaluation later
		params.DeferEval( name ) = true;

		return true;
	}
	return false;
}


void
Flamethrower_interpreter::StopScriptNow( SFxSID id )
{
	if( DoScriptStoppage( id, m_RunningScripts ) )
	{
		DoScriptStoppage( id, m_DisabledScripts );
	}
}


void
Flamethrower_interpreter::StopAllSounds( Flamethrower_script* pScript )
{
	// stop all non-looping sounds
	Flamethrower_script::def_id_time_stack &sounds = pScript->GetSoundLog();
	Flamethrower_script::def_id_time_stack::iterator sound = sounds.begin(), sounds_end = sounds.end();

	for( ; sound != sounds_end; ++sound )
	{
		gWorldFx.StopSample( (*sound).second );
	}

	// stop all looping sounds
	Flamethrower_script::def_idstack &looping_sounds = pScript->GetLoopingSoundLog();
	Flamethrower_script::def_idstack::iterator looping_sound = looping_sounds.begin(), looping_sounds_end = looping_sounds.end();

	for( ; looping_sound != looping_sounds_end; ++looping_sound )
	{
		gWorldFx.StopSample( *looping_sound );
	}
}


bool
Flamethrower_interpreter::DoScriptStoppage( SFxSID id, def_script_id_map &script_map )
{
	def_script_id_map::iterator script = script_map.find( id );

	if( script == script_map.end() )
	{
		return true;
	}

	Flamethrower_script *pScript = (*script).second;

	Flamethrower_script::def_eidstack &effects = pScript->GetEffectLog();
	Flamethrower_script::def_eidstack::iterator effect = effects.begin(), effects_end = effects.end();

	for( ; effect != effects_end; ++effect )
	{
		gWorldFx.DestroyEffect( *effect );
	}

	// stop all currently playing sounds now
	StopAllSounds( pScript );

	if( pScript->m_Callback )
	{
		pScript->m_Callback( pScript->GetID() );
	}

	delete pScript;
	script_map.erase( script );
	return false;
}


void
Flamethrower_interpreter::FinishScriptNow( SFxSID id )
{
	def_script_id_map::iterator script = m_RunningScripts.find( id );

	if( script == m_RunningScripts.end() )
	{
		return;
	}

	Flamethrower_script *pScript = (*script).second;

	if( pScript->GetElapsed() == 0.0f )
	{
		StopScriptNow( id );
		return;
	}

	Flamethrower_script::def_eidstack &effects = pScript->GetEffectLog();
	Flamethrower_script::def_eidstack::iterator effect = effects.begin(), effects_end = effects.end();

	for( ; effect != effects_end; ++effect )
	{
		gWorldFx.SetFinishing( *effect );
	}

	// stop all currently playing sounds now
	StopAllSounds( pScript );

	if( pScript->m_Callback )
	{
		pScript->m_Callback( pScript->GetID() );
	}
}


void
Flamethrower_interpreter::UnloadScriptNow( SFxSID id )
{
	def_script_id_map::iterator script = m_RunningScripts.find( id );

	if( script == m_RunningScripts.end() )
	{
		return;
	}

	UnloadScriptNow( (*script).second );
	m_RunningScripts.erase( script );
}


void
Flamethrower_interpreter::UnloadScriptNow( Flamethrower_script *pScript )
{
	// Unload the individual effects
	Flamethrower_script::def_eidstack &effects = pScript->GetEffectLog();
	Flamethrower_script::def_eidstack::iterator effect = effects.begin(), effects_end = effects.end();

	for( ; effect != effects_end; ++effect )
	{
		gWorldFx.UnloadEffect( *effect );
	}

	// stop all currently playing sounds now
	StopAllSounds( pScript );

	m_DisabledScripts.insert( std::make_pair( pScript->GetID(), pScript ) );
}


void
Flamethrower_interpreter::LoadScriptNow( SFxSID id )
{
	def_script_id_map::iterator iScript = m_DisabledScripts.find( id );

	if( iScript == m_DisabledScripts.end() )
	{
		return;
	}

	// Load the individual effects
	Flamethrower_script::def_eidstack &effects = (*iScript).second->GetEffectLog();
	Flamethrower_script::def_eidstack::iterator effect = effects.begin(), effects_end = effects.end();

	for( ; effect != effects_end; ++effect )
	{
		gWorldFx.LoadEffect( *effect );
	}

	m_RunningScripts.insert( std::make_pair( (*iScript).second->GetID(), (*iScript).second ) );
	m_DisabledScripts.erase( iScript );
}


void
Flamethrower_interpreter::UpdateScript( Flamethrower_script *pScript )
{
	// our default targets may have gotten deleted - check them
	pScript->GetDefaultTargets()->ClearInvalidTargets();

	if( pScript->GetPauseImmune() )
	{
		pScript->m_Elapsed += m_UnPausedSecondsElapsed;
		pScript->m_FrameSync = m_UnPausedSecondsElapsed;
	}
	else
	{
		pScript->m_Elapsed += m_SecondsElapsed;
		pScript->m_FrameSync = m_SecondsElapsed;
	}

	// Update our non-looping sound list for this script
	if( !pScript->m_SoundLog.empty() )
	{
		Flamethrower_script::def_id_time_stack::iterator sound = pScript->m_SoundLog.begin();
		for( ; sound != pScript->m_SoundLog.end(); )
		{
			if( (*sound).first < gWorldTime.GetTime() )
			{
				sound = pScript->m_SoundLog.erase( sound );
			}
			else
			{
				break;
			}
		}
	}

	UINT32 & cur_line = pScript->GetLineNumber();
	UINT32 last_line = pScript->GetLastLineNumber();

	while( cur_line < last_line )
	{
		Flamethrower_line & line = pScript->GetLine( cur_line );

		switch( line.m_Type )
		{
			case Flamethrower_line::LT_BLOCK_BEGIN:
			{
				if( Flamethrower_script::BT_IF_TRUE == pScript->PeekBlock() )
				{
					pScript->PushBlock( Flamethrower_script::BT_BEGIN );
					++cur_line;
				}
				else if( Flamethrower_script::BT_IF_FALSE == pScript->PeekBlock() )
				{
					pScript->PushBlock( Flamethrower_script::BT_BEGIN );
					cur_line += line.m_BlockSkipOffset;
				}
				else
				{
					pScript->PushBlock( Flamethrower_script::BT_BEGIN );
					++cur_line;
				}
			} break;

			case Flamethrower_line::LT_BLOCK_END_ELSE:
			{
				pScript->PopBlock();
				++cur_line;
			} break;

			case Flamethrower_line::LT_BLOCK_END:
			{
				pScript->PopBlock();
				pScript->PopBlock();
				++cur_line;
			} break;

			case Flamethrower_line::LT_IF:
			{
				gpstring sSymbolicExpression = line.m_Params._gpstring( 0 );
				gpstring sNumericExpression;

				if( EvaluateMacrosAndLocals( pScript, sSymbolicExpression, sNumericExpression ) )
				{
					bool bTrue = false;
					if( Skrit::EvalBoolExpression( sNumericExpression, bTrue ) )
					{
						if( bTrue )
						{
							pScript->PushBlock( Flamethrower_script::BT_IF_TRUE );
						}
						else
						{
							pScript->PushBlock( Flamethrower_script::BT_IF_FALSE );
						}
						++cur_line;
						break;
					}
					gpwarningf(("SiegeFx: error - script \"%s\" - invalid expression evaluation %s\n", pScript->GetName().c_str(), sNumericExpression ));
				}
				else
				{
					gpwarningf(("SiegeFx: error - script \"%s\" - invalid expression substitution %s\n", pScript->GetName().c_str(), sSymbolicExpression ));
				}

				pScript->Stop();
			} break;

			case Flamethrower_line::LT_ELSE:
			{
				Flamethrower_script::BLOCK_TYPE bt = pScript->PopBlock();

				if( Flamethrower_script::BT_IF_TRUE == bt )
				{
					pScript->PushBlock( Flamethrower_script::BT_IF_FALSE );
				}
				else if( Flamethrower_script::BT_IF_FALSE == bt )
				{
					pScript->PushBlock( Flamethrower_script::BT_IF_TRUE );
				}

				++cur_line;
			} break;

			case Flamethrower_line::LT_COMMAND:
			{
				Flamethrower_command *pCommand = m_CommandList[ line.m_CommandIndex ];

				if( EvaluateMacrosAndLocals( pScript, line.m_Params, (pCommand->GetTokenIndex() == m_TokenSet ) ) )
				{
					if( pCommand->Execute( pScript, line.m_Params ) )
					{
						++cur_line;
					}
					else
					{
						return;
					}
				}
				else
				{
					gpwarningf(("SiegeFx: error - unable to evaluate paramters for script \"%s\"\n", pScript->GetName().c_str()));
					pScript->Stop();
				}
			} break;

			case Flamethrower_line::LT_EOF:
			{
				pScript->Stop();
				return;
			}

			default:
			{
				gpassert(!"SiegeFx: parse error - unknown line type");
			}
		}
	}
	pScript->Stop();
}


bool
Flamethrower_interpreter::EvaluateMacrosAndLocals( Flamethrower_script *pScript, Flamethrower_params & params, bool bSet )
{
	for( UINT32 i = 0, end = params.Count(); i != end; ++i )
	{
		switch( params.GetType( i ) )
		{
			// execute the macro and replace the symbolic value with a real value
			case Flamethrower_params::PT_MACRO:
			{
				if( m_MacroList[ params._macro( i ) ]->Execute( pScript ) )
				{
					params.Replace( i, 0, pScript->GetReturnValue() );
				}
			} break;

			// replace the symbolic variable with a real value
			case Flamethrower_params::PT_VARIABLE:
			{
				if( !(bSet && (i == 0)) )
				{
					Flamethrower_script::def_symbol_map::iterator it = pScript->m_Symbols.find( params._variable( i ) );

					if( it != pScript->m_Symbols.end() )
					{
						params.Replace( i, 0, (*it).second );
					}
					else
					{
						gpwarningf(( "SiegeFx: error - undefined symbol $%s in script \"%s\"\n", params._variable( i ).c_str(), pScript->GetName().c_str()));
						return false;
					}
				}
			} break;

			// go through the string and replace the symbolic references to macros and locals with real values
			case Flamethrower_params::PT_SFX_PARAMS:
			{
				return EvaluateMacrosAndLocals( pScript, params._sfxparam( i ), params._sfxparam( i ) );
			} break;

			// check for deferred evaluation
			case Flamethrower_params::PT_STRING:
			{
				bool & bNotEvaluated = params.DeferEval( i );

				if( bNotEvaluated )
				{
					bNotEvaluated = false;
					char symbol_type = params._gpstring( i )[0];

					if( EvaluateMacrosAndLocals( pScript, params._gpstring( i )+1, params._gpstring( i ) ) )
					{
						Flamethrower_params::PARAM_TYPE pt = Flamethrower_params::PT_INVALID;

						switch( symbol_type )
						{
							case 'i':	// store as int
							{
								pt = Flamethrower_params::PT_INT;
							} break;

							case 'u':	// store as uint32
							{
								pt = Flamethrower_params::PT_UINT32;
							} break;

							case 'f':	// store as float
							{
								pt = Flamethrower_params::PT_FLOAT;
							} break;

							case 's':	// store as string
							{
								pt = Flamethrower_params::PT_STRING;
							} break;

							case 'v':	// store as vector
							{
								pt = Flamethrower_params::PT_VECTOR;
							} break;

							case 'p':	// store as position
							{
								pt = Flamethrower_params::PT_POSITION;
							} break;
						}
						gpstring str( params._gpstring( i ) );
						params.Replace( i, pt, str );
					}
				}
			} break;
		}
	}
	return true;
}


bool
Flamethrower_interpreter::EvaluateMacrosAndLocals( Flamethrower_script * pScript, const char * sIn, gpstring & sOut )
{
	char * pBuffer = (char *)_alloca( 1024 );
	char * pSymbol = (char *)_alloca( 128 );

#if !GP_RETAIL
	if( ::strlen( sIn ) > 640 )
	{
		gpwarningf(("SiegeFx: error - Script [%s] has a paramter list that is too long\nShorten : \"%s\"\n",pScript->GetName().c_str(), sIn));
		return false;
	}
#endif

	UINT32 begin = 0, end = 0, len = ::strlen( sIn );
	char * pStart = pBuffer;

	while( begin < len )
	{
		if( sIn[ begin ] == '$' )
		{
			for( end = begin+1; ((sIn[ end ] != ' ') && (sIn[ end ] != ',') && (sIn[ end ] != ')') && (sIn[ end ] != '>') && (sIn[ end ] != 0) ); ++end );
			char *pSym = pSymbol;
			char *pSymbolName = pSymbol;

			for( UINT32 scan = begin+1; ((sIn[ scan ] != 0) && (scan != end)); ++scan )
			{
				*pSym++ = sIn[ scan ];
			}
			*pSym = 0;

			Flamethrower_script::def_symbol_map::iterator it = pScript->m_Symbols.find( pSymbolName );

			if( it != pScript->m_Symbols.end() )
			{
				int written = 0;
				(*it).second.Output( 0, pBuffer, written );

				pBuffer += written;
			}
			else
			{
				gpwarningf(( "SiegeFx: error - undefined symbol $%s in script \"%s\"\n", pSymbolName, pScript->GetName().c_str()));
				return false;
			}
			begin = end - 1;
		}
		else if( sIn[ begin ] == '#' )
		{
			const UINT32 macro_index = sIn[ begin + 1 ];

			if( macro_index > m_MacroList.size() )
			{
				gpwarningf(( "SiegeFx: error - undefined macro token in script \"%s\"\n", pScript->GetName().c_str()));
				return false;
			}

			m_MacroList[ sIn[ begin + 1 ] ]->Execute( pScript );

			int written = 0;
			pScript->GetReturnValue().Output( 0, pBuffer, written );

			pBuffer += written;
			++begin;
		}
		else
		{
			// skip copy of < > for plain strings
			if( !(((begin == 0) && (sIn[0] == '<')) || ((begin == len-1) && (sIn[len-1] == '>'))) )
			{
				*pBuffer++ = sIn[ begin ];
			}
		}

		++begin;
	}

	*pBuffer++ = 0;

	sOut = pStart;

	return true;
}


void
Flamethrower_interpreter::FlushQueues()
{
	def_script_ptr_list::iterator script = m_RunQueue.begin(), script_end = m_RunQueue.end();

	for( ; script != script_end; ++script )
	{
		delete *script;
	}

	m_UnloadQueue.clear();
	m_LoadQueue.clear();
	m_RunQueue.clear();
	m_FinishQueue.clear();
	m_StopQueue.clear();
	m_SignalInsQueue.clear();
}


FUBI_DECLARE_SELF_TRAITS( Flamethrower_interpreter::Signal );

bool
Flamethrower_interpreter::Signal::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_id",		m_id );
	persist.Xfer( "m_sTag",		m_sTag );
	persist.Xfer( "m_duration",	m_duration );

	return true;
}




//
// --- Flamethrower script
//

Flamethrower_script::Flamethrower_script()
{
	m_ID				=	SFxSID_INVALID;
	m_Owner				=	GOID_INVALID;

	m_Elapsed			=	0;
	m_Seed				=	0;
	m_FrameSync			=	0;
	m_bPauseImmune		=	false;

	m_CurrentLine		=	0;
	m_NextEffectID		=	SFxEID_INVALID;

	m_bUsesSignals		=	false;
}


Flamethrower_script::Flamethrower_script( Flamethrower_script &copy )
{
	*this = copy;
}


Flamethrower_script::~Flamethrower_script( void )
{
	stdx::for_each( m_Trackers, stdx::delete_by_ptr() );

	gWorldFx.UnRegisterScriptsFromGO( *this );
}


void
Flamethrower_script::PrepareForSave( void )
{
	// get rid of invalid trackers
	for ( TrackerColl::iterator i = m_Trackers.begin() ; i != m_Trackers.end() ; )
	{
		if( !(*i)->IsValid() )
		{
			Delete( *i );
			UINT index = i.GetIndex();
			++i;
			m_Trackers.Free( index );
		}
		else
		{
			++i;
		}
	}

	// get default trackers too
	if ( m_Default_targets.get() != NULL )
	{
		m_Default_targets->PrepareForSave();
	}

	// is my owner gone?
	if ( !IsValid( m_Owner ) )
	{
		m_Owner = GOID_INVALID;
	}
}


FUBI_DECLARE_CAST_TRAITS( Flamethrower_script::BLOCK_TYPE, int );

FUBI_DECLARE_XFER_TRAITS( Flamethrower_script* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if( persist.IsRestoring() )
		{
			obj = new Flamethrower_script;
		}

		return obj->Xfer( persist );
	}
};

FUBI_DECLARE_XFER_TRAITS( TattooTracker* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = WorldFx::MakeTracker().release();
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_SELF_TRAITS( Flamethrower_line );
FUBI_DECLARE_SELF_TRAITS( Flamethrower_params );


bool
Flamethrower_script::Xfer( FuBi::PersistContext &persist )
{
	gpassert( persist.IsRestoring() || ShouldPersist() );

#	if !GP_RETAIL
	if ( persist.IsSaving() )
	{
		CheckValid();
	}
#	endif // !GP_RETAIL

	persist.Xfer		( "m_sName",			m_sName				);

	persist.XferHex		( "m_ID",				m_ID				);
	persist.Xfer		( "m_Owner",			m_Owner				);
	persist.Xfer		( "m_NextEffectID",		m_NextEffectID		);

	persist.XferHex		( "m_Seed",				m_Seed				);

	persist.Xfer		( "m_Elapsed",			m_Elapsed			);
	persist.Xfer		( "m_FrameSync",		m_FrameSync			);
	persist.Xfer		( "m_bPauseImmune",		m_bPauseImmune		);

	persist.Xfer		( "m_CurrentLine",		m_CurrentLine		);

	persist.XferVector	( "m_BlockStack",		m_BlockStack		);
	persist.XferMap		( "m_Symbols",			m_Symbols			);
	persist.XferVector	( "m_Stack",			m_Stack				);
	persist.XferVector	( "m_EffectLog",		m_EffectLog			);

	persist.XferVector	( FuBi::XFER_QUOTED, "m_Script", m_Script	);

	persist.Xfer		( "m_Default_targets",	m_Default_targets	);
	persist.XferUnsafeBucketVector( "m_Trackers", m_Trackers		);

	persist.Xfer		( "m_RegAssEvent",		m_RegAssEvent		);

	persist.Xfer		( "m_bUsesSignals",		m_bUsesSignals		);

	return true;
}


bool
Flamethrower_script::ShouldPersist( void ) const
{
	if ( m_Owner == GOID_INVALID )
	{
		return ( true );
	}

	GoHandle go( m_Owner );
	gpassert( go );
	return ( !go || go->ShouldPersist() );
}


void
Flamethrower_script::Reset()
{
	m_sName				=	"";
	m_ID				=	0;
	m_Owner				=	GOID_INVALID;
	m_NextEffectID		=	SFxEID_INVALID;
	m_Seed				=	0;

	m_Elapsed			=	0;
	m_FrameSync			=	0;

	m_CurrentLine		=	0;

	m_Symbols.clear();
	m_Stack.clear();

	m_BlockStack.clear();

	m_Script.clear();

	def_tracker p(0);

	stdx::for_each( m_Trackers, stdx::delete_by_ptr() );
	m_Trackers.clear();
}


void
Flamethrower_script::operator=( Flamethrower_script &dup )
{
	Reset();

	m_sName				=	dup.m_sName;
	m_Seed				=	dup.m_Seed;
	m_Script			=	dup.m_Script;
	m_bPauseImmune		=	dup.m_bPauseImmune;
	m_bUsesSignals		=	dup.m_bUsesSignals;
}


void
Flamethrower_script::Stop()
{
	GetLineNumber() = GetLastLineNumber();

	if( m_EffectLog.empty() && m_SoundLog.empty() )
	{
		gFlamethrower_interpreter.StopScript( m_ID );
	}
}


void
Flamethrower_script::RemoveCompletionTestID( SFxEID effectid )
{
	def_eidstack::iterator it = m_EffectLog.begin(), effect_end = m_EffectLog.end();

	for( ; it != effect_end; ++it )
	{
		if( *it == effectid )
		{
			m_EffectLog.erase( it );
			return;
		}
	}
}


void
Flamethrower_script::RemoveCompletionTestID( UINT32 soundid )
{
	// remove all non-looping sounds
	def_id_time_stack::iterator it = m_SoundLog.begin();

	for( ; it != m_SoundLog.end(); ++it )
	{
		if( (*it).second == soundid )
		{
			m_SoundLog.erase( it );
			return;
		}
	}

	// remove all looping sounds
	def_idstack::iterator it_looping = m_LoopingSoundLog.begin();

	for( ; it_looping != m_LoopingSoundLog.end(); ++it_looping )
	{
		if( (*it_looping) == soundid )
		{
			m_LoopingSoundLog.erase( it_looping );
			return;
		}
	}
}


TattooTracker*
Flamethrower_script::GetTracker( int i )
{
	gpassert( i != 0 );

	TattooTracker ** tracker = m_Trackers.SafeDereference( i );
	return ( (tracker != NULL) ? *tracker : NULL );
}


def_tracker
Flamethrower_script::ReleaseTracker( int i )
{
	gpassert( i != 0 );

	TattooTracker* tracker = GetTracker( i );
	if ( tracker != NULL )
	{
		m_Trackers.Free( i );
	}

	return ( def_tracker( tracker ) );
}


void
Flamethrower_script::OwnTracker( int i, def_tracker &tracker )
{
	gpassert( i != 0 );

	TattooTracker ** ptracker = m_Trackers.SafeDereference( i );
	if ( ptracker != NULL )
	{
		delete ( *ptracker );
		*ptracker = tracker.release();
	}
	else
	{
		gpverify( m_Trackers.AllocSpecific( tracker.release(), i ) );
	}
}


int
Flamethrower_script::AddTracker( def_tracker &tracker )
{
	return ( m_Trackers.Alloc( tracker.release() ) );
}


void
Flamethrower_script::PushBlock( BLOCK_TYPE bt )
{
	m_BlockStack.push_back( bt );
}


Flamethrower_script::BLOCK_TYPE
Flamethrower_script::PeekBlock( void )
{
	if( m_BlockStack.empty() )
	{
		return BT_INVALID;
	}

	def_block_stack::iterator it = m_BlockStack.end();

	--it;

	return *it;
}


Flamethrower_script::BLOCK_TYPE
Flamethrower_script::PopBlock( void )
{
	if( m_BlockStack.empty() )
	{
		return BT_INVALID;
	}

	def_block_stack::iterator it = m_BlockStack.end();

	--it;

	BLOCK_TYPE	bt = *it;

	m_BlockStack.erase( it );

	return bt;
}


void
Flamethrower_script::RemoveSoundID( DWORD id )
{
	// try to find the sound in non-looping sounds
	def_id_time_stack::iterator it = m_SoundLog.begin();

	for(; it != m_SoundLog.end(); ++it )
	{
		if( (*it).second == id )
		{
			m_SoundLog.erase( it );
			return;
		}
	}

	// try to find the sound in the looping sounds
	def_idstack::iterator it_looping = m_LoopingSoundLog.begin();

	for(; it_looping != m_LoopingSoundLog.end(); ++it_looping )
	{
		if( *it_looping == id )
		{
			m_LoopingSoundLog.erase( it_looping );
			return;
		}
	}
}


#if !GP_RETAIL

void
Flamethrower_script::CheckValid( void ) const
{
	bool persist = ShouldPersist();

	def_eidstack::const_iterator i, ibegin = m_EffectLog.begin(), iend = m_EffectLog.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( FxShouldPersistHelper()( *i ) != persist )
		{
			gperror( "FX Script: an effect I have has a different 'shouldPersist' setting than me!\n" );
		}
	}

	if ( (m_Default_targets.get() != NULL) && (m_Default_targets->ShouldPersist() != persist ) )
	{
		gperror( "FX Script: my default targets have a different 'shouldPersist' setting than me!\n" );
	}

	TrackerColl::const_iterator j, jbegin = m_Trackers.begin(), jend = m_Trackers.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		if ( (*j)->ShouldPersist() != persist )
		{
			gperror( "FX Script: a tracker I have has a different 'shouldPersist' setting than me!\n" );
		}
	}
}

#endif // !GP_RETAIL
