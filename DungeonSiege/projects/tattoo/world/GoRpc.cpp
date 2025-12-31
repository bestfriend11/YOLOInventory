//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoRpc.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoRpc.h"

#include "World.h"
#include "WorldTime.h"
#include "FuBi.h"
#include "NetFuBi.h"
#include "NetLog.h"
#include "NetPipe.h"

//////////////////////////////////////////////////////////////////////////////
// struct GoCheck declaration and implementation

// $ this class's purpose is to manage data for retargetting of broadcast
//   rpc's based on which machine knows about which go.

struct GoCheck
{
	typedef FuBi::ClassSpec::InstanceToNetProc Proc;

	const char*    m_TypeName;
	FuBi::eVarType m_FuBiType;
	bool           m_IsPointer;
	Proc           m_Proc;

	GoCheck( const char* typeName )
	{
		m_TypeName  = typeName;
		m_FuBiType  = gFuBiSysExports.FindType( m_TypeName );
		m_Proc      = NULL;

		gpassert( m_FuBiType != FuBi::VAR_UNKNOWN );
		const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( m_FuBiType );
		gpassert( spec != NULL );
		if ( !(spec->m_Flags & FuBi::ClassSpec::FLAG_POINTERCLASS) )
		{
			m_Proc = spec->m_InstanceToNetProc;
			m_IsPointer = true;
		}
		else
		{
			m_IsPointer = false;
			gpassert( spec->m_Name.same_no_case( "Goid" ) );
		}
	}

	bool operator == ( FuBi::eVarType type ) const
	{
		return ( m_FuBiType == type );
	}

	static const GoCheck* Find( FuBi::eVarType type );
};

const GoCheck* GoCheck :: Find( FuBi::eVarType type )
{
	static GoCheck s_GoCheckTypes[] =
	{
		// main
		GoCheck( "Goid" ),
		GoCheck( "Go" ),

		// components
		GoCheck( "GoActor" ),
		GoCheck( "GoAspect" ),
		GoCheck( "GoAttack" ),
		GoCheck( "GoBody" ),
		GoCheck( "GoCommon" ),
		GoCheck( "GoConversation" ),
		GoCheck( "GoDefend" ),
#		if !GP_RETAIL
		GoCheck( "GoEdit" ),
#		endif // !GP_RETAIL
		GoCheck( "GoFollower" ),
#		if !GP_RETAIL
		GoCheck( "GoGizmo" ),
#		endif // !GP_RETAIL
		GoCheck( "GoGold" ),
		GoCheck( "GoGui" ),
		GoCheck( "GoInventory" ),
		GoCheck( "GoMagic" ),
		GoCheck( "GoMind" ),
		GoCheck( "GoParty" ),
		GoCheck( "GoPhysics" ),
		GoCheck( "GoPlacement" ),
		GoCheck( "GoPotion" ),
		GoCheck( "GoStash" ),
		GoCheck( "GoStore" ),
	};

	// $ lsearch fastest here
	const GoCheck* found = std::find( s_GoCheckTypes, ARRAY_END( s_GoCheckTypes ), type );
	if ( found == ARRAY_END( s_GoCheckTypes ) )
	{
		found = NULL;
	}
	return ( found );
}

//////////////////////////////////////////////////////////////////////////////
// debug helper functions

static void AddSerialIds( const char* name, stdx::fast_vector <UINT> & serials )
{
	gpassert( name != NULL );

	const FuBi::FunctionIndex* found = gFuBiSysExports.FindFunctionsByQualifiedName( name );
	if ( found != NULL )
	{
		FuBi::FunctionIndex::const_iterator i, ibegin = found->begin(), iend = found->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			serials.push_back( (*i)->m_SerialID );
		}
	}
	else
	{
		gperrorf(( "Error: function spec '%s' not found in system.\n\n"
				   "This is a CODE BUG (but safe to ignore) or maybe you need to fetch code.\n", name ));
	}
}

#if !GP_RETAIL

static void CheckGoMembership( FrustumId goFrustum, DWORD address, Go* go, const FuBi::RpcVerifySpec& spec, bool isRpcPacked )
{
	gpassert( goFrustum != FRUSTUMID_INVALID );

	// get all players for the target address
	Server::PlayerColl players;
	gServer.GetPlayersOnMachine( players, address );

	// for each player
	Server::PlayerColl::iterator i, ibegin = players.begin(), iend = players.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i)->IsRemote() )
		{
			// see which frustums that player knows about
			FrustumId playerFrustum = gGoDb.GetPlayerFrustumMask( (*i)->GetId() );

			// now make sure the go is in one of those
			if ( !BitFlagsContainAny( goFrustum, playerFrustum ) )
			{
				gperrorf(( "RPC validation error: attempting to pass a "
						   "non-all-clients Go* or Goid over the network to a "
						   "client machine that does not know about it!\n"
						   "\n"
						   "\tGoid                = 0x%08X (class = %s)\n"
						   "\tScid                = 0x%08X\n"
						   "\tTemplate            = '%s'\n"
						   "\tTeleport            = %s\n"
						   "\tClient              = %d ('%S')\n"
						   "\tClient frustum mask = 0x%08X\n"
						   "\tGo frustum mask     = 0x%08X\n"
						   "\n"
						   "RPC: %s\n"
						   "\n"
						   "Report this error in RAID along with the call stack and full error text.\n",
						   go->GetGoid(), GoidClassToString( GetGoidClass( go->GetGoid() ) ),
						   go->GetScid(),
						   go->GetTemplateName(),
						   go->MakeTeleportLocation().c_str(),
						   MakeIndex( (*i)->GetId() ), (*i)->GetName().c_str(),
						   playerFrustum,
						   goFrustum,
						   spec.BuildQualifiedNameAndParams( isRpcPacked ).c_str() ));
			}
		}
	}
}

bool GoRpcMgr :: CheckSafeGoRpcParam( Goid goid, const FuBi::RpcVerifySpec* spec, bool isRpcPacked )
{
	// verify that we are not passing bad stuff over the network
	const char* badType = NULL;
	GoHandle go( goid );
	int goidClass = GetGoidClass( goid );

	// globals or constants only
	if ( (goidClass != GO_CLASS_GLOBAL) && (goidClass != GO_CLASS_CONSTANT) )
	{
		badType = "non-global/non-constant";
	}
	else if ( !go && (goid != GOID_INVALID) && (goidClass != GO_CLASS_CONSTANT) )
	{
		badType = "non-existent";
	}
	else if ( go && go->IsMarkedForDeletion() && !go->IsDelayedMpDeletion() )
	{
		badType = "deleted";
	}
	else if ( go && go->IsServerOnly() )
	{
		badType = "server-only";
	}

	// report it
	if ( badType != NULL )
	{
		ReportSys::AutoReport autoReport( &gErrorContext );
		gperrorf(( "RPC validation error: attempting to pass %s Go* or Goid "
				   "over the network!\n"
				   "\n"
				   "\tGoid     = 0x%08X (class = %s)\n",
				   badType, goid, GoidClassToString( goidClass ) ));
		if ( go )
		{
			gperrorf(( "\tScid     = 0x%08X\n"
					   "\tTemplate = '%s'\n"
					   "\tTeleport = %s\n",
					   go->GetScid(),
					   go->GetTemplateName(),
					   go->MakeTeleportLocation().c_str() ));
		}
		else
		{
			gperrorf(( "SERIOUS ERROR: this Go does not exist!\n" ));
		}

		if ( spec != NULL )
		{
			gperrorf(( "\n"
					   "RPC: %s\n"
					   "\n"
					   "Report this error in RAID along with the call stack and full error text.",
					   spec->BuildQualifiedNameAndParams( isRpcPacked ).c_str() ));
		}

		// bad!
		return ( false );
	}

	// must be ok!
	return ( true );
}

static void CheckGoParam(
		Goid goid, const FuBi::RpcVerifySpec& spec,
		const FuBi::AddressColl* addresses, bool send, bool isRpcPacked, bool isParam )
{
	// get serial id's of "safe" go functions
	static stdx::fast_vector <UINT> s_SafeFunctions;
	if ( s_SafeFunctions.empty() )
	{
		// these are used in creation/destruction and may operate on go's that
		// are not fully constructed or part of the db yet
		AddSerialIds( "GoDb::RCMarkForDeletionPacker", s_SafeFunctions );
		AddSerialIds( "Go::RCForcedExpiredTransfer",   s_SafeFunctions );

		// these are special cases where we have verified that it's ok to pass
		// go*'s/goids to machines even if they don't know about them.
		AddSerialIds( "WorldTerrain::RCRequestNodeTransition", s_SafeFunctions );
	}

	// check to see if our function is an exception
	if ( !stdx::has_any( s_SafeFunctions, spec.m_Function->m_SerialID ) )
	{
		// verify that we are not passing bad stuff over the network
		GoRpcMgr::CheckSafeGoRpcParam( goid, &spec, isRpcPacked );

		// verify that the go we're talking about even exists on the machine
		GoHandle go( goid );
		int goidClass = GetGoidClass( goid );
		if ( go && send && IsServerLocal() && (goidClass != GO_CLASS_CONSTANT) && (!isParam || (goid != GOID_INVALID)) )
		{
			// only check addresses if we're the top of the stack
			if ( addresses != NULL )
			{
				// only bother if not an "all" go
				if ( !go->CalcAllClients() )
				{
					// see what frustums it is in
					FrustumId goFrustum = go->GetWorldFrustumMembership();
					if ( goFrustum != FRUSTUMID_INVALID )
					{
						// check addresses
						if ( !addresses->empty() )
						{
							FuBi::AddressColl::const_iterator i, ibegin = addresses->begin(), iend = addresses->end();
							for ( i = ibegin ; i != iend ; ++i )
							{
								CheckGoMembership( goFrustum, *i, go, spec, isRpcPacked );
							}
						}
						else
						{
							CheckGoMembership( goFrustum, spec.m_Address, go, spec, isRpcPacked );
						}
					}
				}
			}
		}
	}
}

static void CheckGoOrGoidParam(
		void* param, const FuBi::TypeSpec& type, const FuBi::RpcVerifySpec& spec,
		const FuBi::AddressColl* addresses, bool send, bool isRpcPacked, bool isParam )
{
	const GoCheck* found = GoCheck::Find( type.m_Type );
	if ( found != NULL )
	{
		if ( found->m_IsPointer )
		{
			if ( found->m_Proc != NULL )
			{
				// go or component
				gpassert( type.m_Flags & (FuBi::TypeSpec::FLAG_POINTER | FuBi::TypeSpec::FLAG_REFERENCE) );
				gpassert( !(type.m_Flags & FuBi::TypeSpec::FLAG_HANDLE) );

				// get the goid out
				Goid goid = GOID_INVALID;
				if ( param )
				{
					if ( isRpcPacked )
					{
						// already in goid form
						goid = MakeGoid( (DWORD)param );
					}
					else
					{
						goid = MakeGoid( (*found->m_Proc)( param ) );
					}
				}
				CheckGoParam( goid, spec, addresses, send, isRpcPacked, isParam );
			}
		}
		else
		{
			// a plain goid
			gpassert( type.m_Flags & FuBi::TypeSpec::FLAG_POINTER );
			gpassert( !(type.m_Flags & (FuBi::TypeSpec::FLAG_REFERENCE | FuBi::TypeSpec::FLAG_HANDLE)) );
			CheckGoParam( MakeGoid( (DWORD)param ), spec, addresses, send, isRpcPacked, isParam );
		}
	}
}

static void VerifyGeneralRpc( const FuBi::RpcVerifySpec& spec, const FuBi::AddressColl* addresses, bool send, bool areParamsRpcPacked, bool isThisRpcPacked )
{
	// tell netfubi that we can't have a flush here otherwise it will recurse
	// and give us the plain assert dialog, which has no stack trace etc. this
	// is ok because if the verify fails then we have a serious code problem
	// that must be fixed (so the flush wouldn't really help here).
	gpassert( !Singleton <NetFuBiSend>::DoesSingletonExist() || gNetFuBiSend.IsForceUpdateEnabled() );
	gNetFuBiSend.DisableForceUpdate();

	// check params
	const BYTE* param = rcast <const BYTE*> ( spec.m_Params.mem );
	FuBi::FunctionSpec::ParamSpecs::const_iterator i,
			begin = spec.m_Function->m_ParamSpecs.begin(),
			end   = spec.m_Function->m_ParamSpecs.end();
	for ( i = begin ; i != end ; ++i )
	{
		// check
		CheckGoOrGoidParam( (void*)(*(DWORD*)param), i->m_Type, spec, addresses, send, areParamsRpcPacked, true );

		// advance to next param
		param += i->m_Type.GetSizeBytes();
	}

	// check object (if any)
	if ( spec.m_Function->m_Flags & FuBi::FunctionSpec::FLAG_CALL_THISCALL )
	{
		// check for go's
		gpassert( spec.m_Function->m_Parent != NULL );
		FuBi::TypeSpec type( spec.m_Function->m_Parent->m_Type, FuBi::TypeSpec::FLAG_POINTER );
		CheckGoOrGoidParam( spec.m_This, type, spec, addresses, send, isThisRpcPacked, false );
	}

	// re-enable
	gNetFuBiSend.EnableForceUpdate();
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class GoRpcMgr implementation

GoRpcMgr :: GoRpcMgr( void )
	: Inherited( false )
{
	// register self with fubi
	gFuBiSysExports.RegisterRebroadcaster( this );

	// register nested RPC exceptions
#	if GP_DEBUG

	// these are ok for nested calls, they are called indirectly by lots of
	// things and aren't worth worrying about for individual cases.
	gFuBiSysExports.RegisterNestedRpcException( "*", "GoDb::RCMarkForDeletionPacker" );
	gFuBiSysExports.RegisterNestedRpcException( "*", "GoDb::RCCloneGoPacker" );
	gFuBiSysExports.RegisterNestedRpcException( "GoDb::RCCloneGoPacker", "*" );

	// these call all kinds of crazy shit, let em do what they want cause its
	// staging area stuff only anyway.
	gFuBiSysExports.RegisterNestedRpcException( "Server::RCCreatePlayerOnMachine", "*" );
	gFuBiSysExports.RegisterNestedRpcException( "Server::RCMarkPlayerForDeletion", "*" );
	gFuBiSysExports.RegisterNestedRpcException( "UIMultiplayer::RCStartNetworkGame", "*" );
	gFuBiSysExports.RegisterNestedRpcException( "Player::RCSetWorldState", "*" );
	gFuBiSysExports.RegisterNestedRpcException( "WorldMap::RCSet", "*" );

	// posting a collision may result in any number of things due to triggers.
	// because it's a message it's difficult to collapse this behavior so it
	// does not nest. $$$ RECONSIDER THIS LATER - try to allow a trigger
	// being sent to reset the nesting level (triggers can cause anything) -sb
	gFuBiSysExports.RegisterNestedRpcException( "Go::RCExplodeGo", "*" );

	// this is fine, plenty rare and i don't want to fix
	gFuBiSysExports.RegisterNestedRpcException( "GoAspect::RCSetLifeState", "GoAspect::RCSetGhostTimeout" );
	gFuBiSysExports.RegisterNestedRpcException( "GoParty::RCRemoveMemberNow", "GoInventory::RCEquip" );
	gFuBiSysExports.RegisterNestedRpcException( "Server::RCMarkPlayerForDeletion", "WorldState::RCSetWorldStateOnMachine" );
	gFuBiSysExports.RegisterNestedRpcException( "Victory::RCSetGameType", "Player::RCSetStartingGroup" );
	gFuBiSysExports.RegisterNestedRpcException( "GoMagic::RCApplyEnchantments", "GoMagic::RCSetPotionAmount" );
	gFuBiSysExports.RegisterNestedRpcException( "GoAspect::RCSetLifeState", "SiegeFxInterpreter::RCStopScript" );

	// $$$$ these are temporary, reevaluate later!!!
	gFuBiSysExports.RegisterNestedRpcException( "GoInventory::RCRemove", "SiegeFxInterpreter::RCRunScriptPacker" );

#	endif // GP_DEBUG
}

GoRpcMgr :: ~GoRpcMgr( void )
{
	gpassert( m_NextPacket.m_RouterGoid == GOID_INVALID );
}

void GoRpcMgr :: CommitPending( void )
{
	PacketColl::iterator i, ibegin = m_PendingColl.begin(), iend = m_PendingColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// get go - may have been deleted before now, so it's ok to skip if
		// gone. also - if it's still pending entry, then screw it, nuke the
		// packets. chances are the rpc's don't really need to be passed along
		// anyway if the siege node hasn't loaded yet. if they do, well, then
		// i can go through the trouble of adding a dependency list of all the
		// goid's that must be non-pending before the packets can go out, along
		// with an expiration to cull old crap. yeah, i'll have time for that.
		// so anyway for now, we nuke 'em.
		GoHandle go( i->m_RouterGoid );
		if ( go && !go->IsPendingEntry() )
		{
			// if we're in here, must have valid frustum membership
			FrustumId goFrustum = go->GetWorldFrustumMembership();
			gpassert( goFrustum != FRUSTUMID_INVALID );

			// check all pending goids
			GoidColl::iterator j, jbegin = i->m_PendingGoids.begin(), jend = i->m_PendingGoids.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				GoHandle pending( *j );
				if ( !pending || pending->IsPendingEntry() )
				{
					break;
				}
			}

			// if none pending, go for it
			if ( j == jend )
			{
				// have godb fill it out
				FuBi::AddressColl addresses;
				if ( !gGoDb.FillAddressesForFrustum( goFrustum, addresses ) )
				{
					// must want all of them
					addresses.clear();
					addresses.push_back( RPC_TO_ALL );
				}

				// send rpc if something to do (if empty, no clients need to know
				// about this rpc)
				if ( !addresses.empty() )
				{
					// we couldn't verify when it was pending, so do it now
#					if GP_DEBUG
					i->m_VerifySpec.m_Params = const_mem_ptr( &*(i->m_Buffer.begin() + 2), i->m_Buffer.size() );
					VerifyGeneralRpc( i->m_VerifySpec, &addresses, true, true, false );
#					endif // GP_DEBUG

					// do rpc
					gFuBiNetSendTransport.BeginMultiRPC( &*addresses.begin(), addresses.size(), i->m_IsRetrying );
					gFuBiNetSendTransport.Store( &*i->m_Buffer.begin(), i->m_Buffer.size() );
					gFuBiNetSendTransport.EndRPC();
				}
			}
		}
	}

	m_PendingColl.clear();
}

FuBi::Cookie GoRpcMgr :: BeginRPC( DWORD toAddress, bool shouldRetry )
{
	return ( BeginMultiRPC( &toAddress, 1, shouldRetry ) );
}

FuBi::Cookie GoRpcMgr :: BeginMultiRPC( DWORD* /*toAddress*/, int /*addresses*/, bool shouldRetry )
{
	// $ note: we ignore addresses in here because the godb determines it

	// make sure accumulation buffer is clear
	if ( !m_Buffer.empty() )
	{
		gpassertm( 0, "WARNING! Buffer should be empty at this point. Contents will be purged." );
		m_Buffer.clear();
	}

	// begin new packet
	gpassert( m_NextPacket.m_RouterGoid != GOID_INVALID );
	m_NextPacket.m_IsRetrying = shouldRetry;
	m_PendingColl.push_back( m_NextPacket );

	// clear now that we have it
	m_NextPacket = Packet();

	// all ok
	return ( RPC_SUCCESS );
}

DWORD GoRpcMgr :: EndRPC( void )
{
	DWORD packetSize = m_Buffer.size();
	m_PendingColl.back().m_Buffer = m_Buffer;
	m_Buffer.clear();
	return ( packetSize );
}

bool GoRpcMgr :: OnVerifySendRpc( const FuBi::RpcVerifyColl& specs, const FuBi::AddressColl& addresses, bool isRpcPacked )
{
#	if ( GP_DEBUG )
	{
		FuBi::RpcVerifyColl::const_iterator i, ibegin = specs.begin(), iend = specs.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (i->m_Function != NULL) && !i->IsNestDetectCancel() )
			{
				VerifyGeneralRpc( *i, (i == (iend - 1)) ? &addresses : NULL, true, isRpcPacked, isRpcPacked );
			}
		}
	}
#	endif // GP_DEBUG

	return ( true );
}

bool GoRpcMgr :: OnVerifyDispatchRpc( bool preCall, const FuBi::RpcVerifySpec& spec )
{
	// $ note that preCall is true if called before the RPC is dispatched, and
	//   false if called afterwards. also note that on a "post" RPC dispatch
	//   callback, the return value is ignored (because we've already
	//   dispatched it).

	// $$$ fill this in - verify that the proposed dispatch is legal, return
	//     false on security failure.

#	if ( GP_DEBUG )
	if ( preCall )
	{
		VerifyGeneralRpc( spec, NULL, false, false, false );
	}
#	endif // GP_DEBUG

	return ( true );
}

FuBi::eBroadcastType GoRpcMgr :: OnProcessBroadcastRpc( const FuBi::RpcVerifySpec& spec, FuBi::AddressColl& addresses )
{
	using namespace FuBi;

	// get serial id's of "noncritical" go functions
	static stdx::fast_vector <UINT> s_NonCriticalFunctions;
	if ( s_NonCriticalFunctions.empty() )
	{
		// $ these should only go through if it's all ok right now, ok to skip
		//   if all params are not on the client.

		// allow this because these are a-ok to skip
		AddSerialIds( "Go::RCPlayCombatSoundPacker",            s_NonCriticalFunctions );
		AddSerialIds( "GoMagic::RCApplyEnchantments",           s_NonCriticalFunctions );
		AddSerialIds( "GoMind::RCSetClientCachedEngagedObject", s_NonCriticalFunctions );

		// allow this because at the edge of player 1's frustum player 2
		// shooting outside at an object that player 1 does not have causes
		// problems. because the ammo will be deleted anyway after a short time,
		// just abort the call entirely for player 1.
		AddSerialIds( "GoAttack::RCLaunchAmmo", s_NonCriticalFunctions );
		AddSerialIds( "Go::RCSetCollision",     s_NonCriticalFunctions );
	}

	// this must reprocess RPC_TO_ALL and RPC_TO_OTHER to only send out messages
	// to clients that know about those go's.
	gpassert( (spec.m_Address == RPC_TO_ALL) || (spec.m_Address == RPC_TO_OTHERS) );

	// safety checks
	gpassert( m_NextPacket.m_RouterGoid == GOID_INVALID );

	// may be some leftovers
	m_NextPacket.m_PendingGoids.clear();

	// see if our "this" type is controlled
	eBroadcastType broadcastType = BROADCAST_NORMAL;
	if ( spec.m_Function->m_Flags & FunctionSpec::FLAG_CALL_THISCALL )
	{
		// check for go's
		gpassert( spec.m_Function->m_Parent != NULL );
		const GoCheck* found = GoCheck::Find( spec.m_Function->m_Parent->m_Type );
		if ( found != NULL )
		{
			// $ it's controlled, reprocess the broadcast

			// convert the pointer to a cookie (goid)
			gpassert( (found->m_Proc != NULL) && (spec.m_This != 0) );
			Goid goid = MakeGoid( (*found->m_Proc)( spec.m_This ) );

			// grab the go
			GoHandle go( goid );
			gpassert( go );

			// see if it's a noncritical function
			bool nonCritical = stdx::has_any( s_NonCriticalFunctions, spec.m_Function->m_SerialID );

			// only bother with this if it's not an "all" go or is noncritical
			if ( !go->CalcAllClients() || nonCritical )
			{
				// get its membership (must be in at least one!)
				FrustumId goFrustum = go->CalcWorldFrustumMembership( false );
				if ( goFrustum != FRUSTUMID_INVALID )
				{
					gpassert( !go->IsPendingEntry() );
					bool pendingEntry = false;

					// check params for pending/nonexistent go's
					const BYTE* iparam = rcast <const BYTE*> ( spec.m_Params.mem );
					FuBi::FunctionSpec::ParamSpecs::const_iterator i,
							begin = spec.m_Function->m_ParamSpecs.begin(),
							end   = spec.m_Function->m_ParamSpecs.end();
					for ( i = begin ; i != end ; ++i )
					{
						// check
						const GoCheck* found = GoCheck::Find( i->m_Type.m_Type );
						if ( found != NULL )
						{
							void* param = (void*)(*(DWORD*)iparam);
							if ( param )
							{
								// extract goid
								Goid paramGoid = GOID_INVALID;
								if ( !found->m_IsPointer )
								{
									paramGoid = MakeGoid( (DWORD)param );
								}
								else if ( found->m_Proc != NULL )
								{
									paramGoid = MakeGoid( (*found->m_Proc)( param ) );
								}

								// see if it's pending
								GoHandle paramGo( paramGoid );
								if ( paramGo )
								{
									if ( !paramGo->CalcAllClients() && paramGo->IsPendingEntry() )
									{
										m_NextPacket.m_PendingGoids.push_back( paramGoid );
										pendingEntry = true;

										// break now if noncritical, we don't care about the rest
										if ( nonCritical )
										{
											break;
										}
									}

									// add its frustum if we're interested in that.
									// the godb fill below will choose only players
									// that have frustums that contain all of the
									// params.
									if ( nonCritical )
									{
										goFrustum = MakeFrustumId( ( MakeInt( goFrustum ) & MakeInt( paramGo->GetWorldFrustumMembership() ) ) );
									}
								}
								else if ( ::GetGoidClass( paramGoid ) != GO_CLASS_CONSTANT )
								{
									gperrorf(( "Serious error: attempting to pass a completely invalid Go*/Goid 0x%08X across the network!!\n", paramGoid ));
								}
							}
						}

						// advance to next param
						iparam += i->m_Type.GetSizeBytes();
					}

					// adjust if necessary
					if ( pendingEntry )
					{
						// chuck if noncritical, otherwise defer it
						if ( nonCritical && (goFrustum == FRUSTUMID_INVALID) )
						{
							broadcastType = BROADCAST_ABORT;
						}
						else
						{
							broadcastType = BROADCAST_DEFER;
						}
					}
					else if ( goFrustum != FRUSTUMID_INVALID )
					{
						// have godb fill it out
						if ( gGoDb.FillAddressesForFrustum( goFrustum, addresses ) )
						{
							broadcastType = BROADCAST_REDIRECT;
						}
					}
				}
				else if ( go->IsPendingEntry() && !nonCritical )
				{
					// defer this one
					broadcastType = BROADCAST_DEFER;
				}
				else
				{
					// no frustum, not pending entry - weird, must be local only or somethin
					broadcastType = BROADCAST_ABORT;
				}

				// if deferred, take goid
				if ( IsDeferred( broadcastType ) )
				{
					m_NextPacket.m_RouterGoid = goid;
					gplog( OutputRpcOutDeferred( spec, m_NextPacket.m_RouterGoid, m_NextPacket.m_PendingGoids ) );
				}
			}
		}
	}

	// done
	return ( broadcastType );
}

void GoRpcMgr :: OnBeginRedirect( const FuBi::RpcVerifyColl& specs )
{
	PushSingleton();
	gpassert( !specs.empty() );
	m_NextPacket.m_VerifySpec = specs.back();
}

void GoRpcMgr :: OnEndRedirect( void )
{
	PopSingleton();
}

void GoRpcMgr :: OnSendRpc( const FuBi::FunctionSpec* spec, DWORD netCookie, DWORD packetSize )
{
#	if !GP_RETAIL

	// check for go's
	if ( netCookie != 0 )
	{
		gpassert( spec->m_Parent != NULL );
		const GoCheck* found = GoCheck::Find( spec->m_Parent->m_Type );
		if ( found != NULL )
		{
			GoHandle go( MakeGoid( netCookie ) );
			if ( go )
			{
				// give "credit" to the Go for this madness
				go->m_RpcFrameBytes += packetSize;
				go->m_RpcTotalBytes += packetSize;
			}
		}
	}

#	endif // !GP_RETAIL
}

void GoRpcMgr :: OnDispatchRpc( const FuBi::FunctionSpec* spec, DWORD netCookie, DWORD packetSize )
{
#	if !GP_RETAIL

	// same thing as above
	OnSendRpc( spec, netCookie, packetSize );

#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
