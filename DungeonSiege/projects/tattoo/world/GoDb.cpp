//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDb.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoDb.h"

#include "AIQuery.h"
#include "CameraAgent.h"
#include "ContentDb.h"
#include "DebugProbe.h"
#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "Flamethrower_Interpreter.h"
#include "Flamethrower_FuBi_Support.h"
#include "FuBiPersistImpl.h"
#include "FuBiBitPacker.h"
#include "FuelDb.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoData.h"
#include "GoEdit.h"
#include "GoGizmo.h"
#include "GoMind.h"
#include "GoParty.h"
#include "GoRpc.h"
#include "GoShmo.h"
#include "GoSupport.h"
#include "MCP.h"
#include "Nema_AspectMgr.h"
#include "NetLog.h"
#include "NetPipe.h"
#include "PContentDb.h"
#include "RatioStack.h"
#include "ReportSys.h"
#include "Server.h"
#include "Siege_Camera.h"
#include "Siege_Engine.h"
#include "Siege_Frustum.h"
#include "Siege_Loader.h"
#include "Siege_Mouse_Shadow.h"
#include "Siege_Node.h"
#include "Siege_Options.h"
#include "Sim.h"
#include "SkritEngine.h"
#include "StdHelp.h"
#include "Trigger_Sys.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldMessage.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"

#if !GP_RETAIL
#include "nema_aspect.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// documentation

// $ note that this may be really out of date...use for reference only -sb

/*	GAME OBJECT LIFETIME
	---- ------ --------

	Load from static content (via instanced Scid):

		Cause:

		1. World streamer brings in new node on server.

		2. World streamer loops through node's content and sends Scids off to
		   GoDb::SLoadGo().

		Effect:

		1.  GoDb::SLoadGo() executes...

		2.  Check retired collection for Scid, abort if exists.

		3.  Check GoDb for a Go with that Scid, abort if exists.

		4.  GoDb::SCreateGo() executes...

		5.  Reserve Goid for new Go.

		6.  Get random seed to use for new Go.

		7.  Tell clients to create Go's with these parameters.

		8.  Use random seed.

		9.  Create new Go.

		10. Look up template and instantiate template components.

			a. Optionally specialize data from the template.

			b. Xfer data into the component.

			c. Ask component to issue preload requests.

		11. Optionally apply an initial position to go's placement (but do
			not mess with node occupancy yet).

		12. Add new Go to the creation collection.

		13. Assign the Goid to the new Go.

		14. Commit creation on all components (this may in turn cause creation
			of child Go's).

		15. Debug only: tell Go that it has entered the GoDb.

		16. Add Go to the Scid index.

		17. Move go into the on-deck database (see "Commit on-deck Go").

	Commit on-deck Go's:

		Cause:

		1.  First called at the top of GoDb::Update().

		2.  Then called at the bottom of GoDb::Update().

		Effect:

		1.  Switch to other on-deck collection buffer.

		2.  Move each Go pointer into the actual database (currently that slot
			will be null).

		3.  Send WE_CONSTRUCTED to each on-deck Go.

			a.  GoPlacement: add the Go to its node occupancy list. The GoDb
				will be notified.

		4.  Clear the on-deck db.

	IT'S THE I LOVE THE GODB SONG!!!!!!

		Ohhhh I love the GoDb
		its numerous lines of code
		they are so fine indeed
		they'll make you blow your <censored>

		I love I love the GoDb
		db's and criticals galore
		i <censored> love the way my head spins round
		GoDb can <censored> <censored> my <censored> <censored>

		Dear GoDb will you marry me
		you <censored> piece of <censored> <censored>
		it's your <censored> fault my <censored> <censored> is <censored>
		and <censored> <censored> your <censored> <censored>
		so you can <censored> <censored> my <censored> <censored> <censored>
		<censored> and then <censored> <censored> !$*(#@ <censored>
		($*(@**( <censored> RRAAAAAAARRRRRRRRR <censored> <censored> <censored>
		
		<BLAM!>
		<splat>
		<thud>
		<drip...drip...drip...>

		-sb
*/

//////////////////////////////////////////////////////////////////////////////
// struct GoDbDeleteXfer implementation

void GoDbDeleteXfer :: Xfer( FuBi::BitPacker& packer )
{
	packer.XferRaw( m_Goid      );
	packer.XferBit( m_Retire    );
	packer.XferBit( m_FadeOut   );
	packer.XferBit( m_MpDelayed );
}

#if !GP_RETAIL

void GoDbDeleteXfer :: RpcToString( gpstring& appendHere ) const
{
	appendHere.appendf( "goid = 0x%08X", m_Goid );
	if ( m_Retire )
	{
		appendHere += " +retire";
	}
	if ( m_FadeOut )
	{
		appendHere += " +fadeOut";
	}
	if ( m_MpDelayed )
	{
		appendHere += " +mpDelayed";
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct GoDbMondoDeleteXfer implementation

void GoDbMondoDeleteXfer :: Xfer( FuBi::BitPacker& packer )
{
	// $ late-night RLE compression algorithm courtesy XTZ. i've never written
	//   one of these before so don't laugh!!

	enum
	{
		// $ note: 30-bit goids are ok because the top 2 always 0 for globals

		CODE_STOP,				// no more to read
		CODE_SINGLE_LITERAL,	// 30-bit Goid
		CODE_LITERAL_RUN,		// 4-bit count-1, 30-bit Goid, 30-bit Goid...
		CODE_SEQUENCE,			// 8-bit minor index, 4-bit count-1 (uses most recent Goid added for base)
	};

	if ( packer.IsSaving() )
	{
		// these buckets should already be sorted!
#		if GP_DEBUG
		GoidColl localTest( m_Goids );
		localTest.sort();
		gpassert( ::memcmp( &*m_Goids.begin(), &*localTest.begin(), m_Goids.size_bytes() ) == 0 );
#		endif // GP_DEBUG

		// start with this state
		bool seekLiterals = true;

		// pack em in there
		GoidColl::const_iterator i, ibegin = m_Goids.begin(), iend = m_Goids.end();
		for ( i = ibegin ; i != iend ; )
		{
			// check state
			if ( seekLiterals )
			{
				// seek to end of literals
				GoidColl::const_iterator basei = i;
				GlobalGoidBits base( *basei );
				for ( ++i ; i != iend ; ++i )
				{
					// sequence match? change state
					if ( GlobalGoidBits( *i ).m_MajorIndex == GlobalGoidBits( *(i - 1) ).m_MajorIndex )
					{
						seekLiterals = false;
						break;
					}

					// no more than 16 at a time, flush
					if ( (i - basei) == 16 )
					{
						break;
					}
				}

				// flush literals
				int count = i - basei;
				gpassert( count > 0 );
				if ( count == 1 )
				{
					DWORD code = CODE_SINGLE_LITERAL;
					packer.WriteBits( &code, 2 );
					packer.WriteBits( &*basei, 30 );
				}
				else
				{
					DWORD code = CODE_LITERAL_RUN;
					packer.WriteBits( &code, 2 );
					--count;
					packer.WriteBits( &count, 4 );

					for ( ++count ; count > 0 ; --count )
					{
						packer.WriteBits( &*basei, 30 );
						++basei;
					}
				}
			}
			else
			{
				// write code
				DWORD code = CODE_SEQUENCE;
				packer.WriteBits( &code, 2 );

				// get vars
				GoidColl::const_iterator basei = i;
				GlobalGoidBits base( *basei ), root( *(basei - 1) );
				gpassert( base.m_MajorIndex == root.m_MajorIndex );
				gpassert( base.m_Magic == root.m_Magic );

				// write base index
				int minorIndex = base.m_MinorIndex;
				packer.WriteBits( &minorIndex, 8 );

				// seek to end of sequence
				for ( ++i ; i != iend ; ++i )
				{
					GlobalGoidBits cbits( *i );
					GlobalGoidBits pbits( *(i - 1) );
					gpassert( pbits.m_MajorIndex == base.m_MajorIndex );
					gpassert( pbits.m_Magic == base.m_Magic );

					// new literal?
					if ( cbits.m_MajorIndex != base.m_MajorIndex )
					{
						seekLiterals = true;
						break;
					}

					gpassert( cbits.m_Magic == base.m_Magic );

					// out of sequence? go again
					if ( cbits.m_MinorIndex != (pbits.m_MinorIndex + 1) )
					{
						break;
					}

					// no more than 16 at a time, flush
					if ( (i - basei) == 16 )
					{
						break;
					}
				}

				// write count
				int count = i - basei;
				gpassert( count > 0 );
				--count;
				packer.WriteBits( &count, 4 );
			}
		}

		// done
		DWORD code = CODE_STOP;
		packer.WriteBits( &code, 2 );
	}
	else
	{
		DWORD code = CODE_STOP;
		do
		{
			packer.ReadBits( &code, 2 );
			switch ( code )
			{
				case ( CODE_SINGLE_LITERAL ):
				{
					Goid& goid = *m_Goids.push_back( GOID_INVALID );
					packer.ReadBits( &goid, 30 );
				}
				break;

				case ( CODE_LITERAL_RUN ):
				{
					int count = 0;
					packer.ReadBits( &count, 4 );

					for ( ++count ; count > 0 ; --count )
					{
						Goid& goid = *m_Goids.push_back( GOID_INVALID );
						packer.ReadBits( &goid, 30 );
					}
				}
				break;

				case ( CODE_SEQUENCE ):
				{
					gpassert( !m_Goids.empty() );

					int minorIndex = 0;
					packer.ReadBits( &minorIndex, 8 );

					int count = 0;
					packer.ReadBits( &count, 4 );

					GlobalGoidBits base( m_Goids.back() );
					for ( ++count ; count > 0 ; --count )
					{
						GlobalGoidBits bits;
						bits.m_MajorIndex = base.m_MajorIndex;
						bits.m_MinorIndex = minorIndex++;
						bits.m_Magic = base.m_Magic;
						m_Goids.push_back( bits );
					}
				}
				break;
			}
		}
		while ( code != CODE_STOP );
	}
}

#if !GP_RETAIL

void GoDbMondoDeleteXfer :: RpcToString( gpstring& appendHere ) const
{
	GoidColl::const_iterator i, ibegin = m_Goids.begin(), iend = m_Goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i != ibegin )
		{
			appendHere += ", ";
		}
		appendHere.appendf( "0x%08X", *i );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct MachineAddrQuery declaration and implementation

struct MachineAddrQuery
{
	bool          m_ForLocal;
	const Player* m_RemotePlayers[ Server::MAX_PLAYERS + 1 ];

	MachineAddrQuery( DWORD machineId )
	{
		if ( ::IsSendLocalOnly( machineId ) )
		{
			m_ForLocal = true;
			m_RemotePlayers[ 0 ] = NULL;
		}
		else
		{
			m_ForLocal = machineId == RPC_TO_ALL;

			if ( m_ForLocal || (machineId == RPC_TO_OTHERS) )
			{
				// add all remotes
				const Player** out = m_RemotePlayers;
				Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( IsValid(*i) && (*i)->IsRemote() && !(*i)->IsComputerPlayer() )
					{
						*out++ = *i;
						gpassert( out <= ARRAY_END( m_RemotePlayers ) );
					}
				}
				*out = NULL;
			}
			else
			{
				// look up this specific one
				m_RemotePlayers[ 0 ] = gServer.GetHumanPlayerOnMachine( machineId );
				m_RemotePlayers[ 1 ] = NULL;
				gpassert( m_RemotePlayers[ 0 ] != NULL );
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////
// class GoPreloader declaration and implementation

class GoPreloader : public siege::Loader
{
public:
	SET_INHERITED( GoPreloader, siege::Loader );

	GoPreloader( void )									{  }
	virtual ~GoPreloader( void )						{  }

	virtual void* OnCreateLoadOrder( FOURCC /*type*/, const gpstring& name, DWORD /*id*/, void* /*extra*/ )
	{
		return ( new gpstring( name ) );
	}

	virtual void OnDestroyLoadOrder( void* data )
	{
		delete ( (gpstring*)data );
	}

	virtual siege::eCommitCode OnCommitLoadOrder( void* data )
	{
		gGoDb.FindCloneSource( *(gpstring*)data );
		return ( siege::COMMIT_SUCCESS );
	}

#	if !GP_RETAIL
	virtual gpstring OnStringizeLoadOrder( const void* data )
	{
		return ( *(const gpstring*)data );
	}
#	endif // !GP_RETAIL

private:
	SET_NO_COPYING( GoPreloader );
};

//////////////////////////////////////////////////////////////////////////////
// persistence support

struct GoDbShouldPersistHelper
{
	bool operator () ( const Go* go ) const
	{
		if_gpassert( go != NULL )
		{
			return ( go->ShouldPersist() );
		}
		return ( false );
	}

	bool operator () ( const GoDb::ExpireEntry entry ) const
	{
		return ( (*this)( entry.m_Go ) );
	}
};

struct GoDbShouldPersistFirst
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( GoDbShouldPersistHelper()( obj.first ) );
	}
};

struct GoDbShouldPersistSecond
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( GoDbShouldPersistHelper()( obj.second ) );
	}
};

GoDb::EffectEntry :: ~EffectEntry( void )
{
	delete ( m_OriginalParams );
}

bool GoDb::EffectEntry :: Xfer( FuBi::PersistContext& persist )
{
	persist.XferHex( "m_ScriptId",      m_ScriptId      );
	persist.Xfer   ( "m_CreationEvent", m_CreationEvent );
	persist.Xfer   ( "m_Active",        m_Active        );
	persist.Xfer   ( "m_XferOnSync",    m_XferOnSync    );

	return ( true );
}

void GoDb::EffectEntry :: WriteForSync( FuBi::BitPacker& packer, Goid owner ) const
{
	if ( m_XferOnSync )
	{
		Flamethrower_script* script = NULL;
		if (   gFlamethrower_interpreter.ShouldScriptXfer( m_ScriptId )
			&& gWorldFx.FindScript( m_ScriptId, &script ) )
		{
			packer.WriteBit( true );

			RunScriptXfer xfer;
			xfer.m_ScriptId    = m_ScriptId;
			xfer.m_Name        = script->GetName();
			xfer.m_ScriptOwner = owner;
			xfer.m_Type        = m_CreationEvent;
			xfer.m_Targets     = def_tracker( script->GetDefaultTargets()->Duplicate() );

			if ( m_OriginalParams != NULL )
			{
				xfer.m_Params = *m_OriginalParams;
			}

			xfer.Xfer( packer, false );
		}
	}
}

FUBI_DECLARE_SELF_TRAITS( GoDb::EffectEntry );

bool GoDb::ExpireEntry :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Go",     m_Go     );
	persist.Xfer( "m_Forced", m_Forced );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( GoDb::ExpireEntry );

bool GoDb::ScidBits :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Bits",    m_Bits    );
	persist.Xfer( "m_MustRpc", m_MustRpc );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( GoDb::ScidBits );

bool GoDb::GoFrustum :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_Go", m_Go );
	persist.Xfer( "m_FrustumId", m_FrustumId );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( GoDb::GoFrustum );

bool GoDb::PlayerFrustum :: Xfer( FuBi::PersistContext& persist )
{
	persist.XferList( "m_Coll", m_Coll );
	persist.Xfer( "m_OwnedMask", m_OwnedMask );

	if ( persist.IsSaving() )
	{
		Go* go = NULL;
		if ( m_Focus != m_Coll.end() )
		{
			go = m_Focus->m_Go;
		}
		persist.Xfer( "m_Focus", go );
	}
	else
	{
		Go* go = NULL;
		persist.Xfer( "m_Focus", go );
		if ( go != NULL )
		{
			Coll::iterator i, ibegin = m_Coll.begin(), iend = m_Coll.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_Go == go )
				{
					m_Focus = i;
					break;
				}
			}
			gpassert( i != iend );
		}
	}

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( GoDb::PlayerFrustum );

FUBI_DECLARE_XFER_TRAITS( EnchantmentStorage* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new EnchantmentStorage;
		}

		return ( obj->Xfer( persist ) );
	}
};

FUBI_DECLARE_XFER_TRAITS( GopColl )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferVector( key, obj ) );
	}
};

FUBI_DECLARE_XFER_TRAITS( GoDb::StringDb )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		return ( persist.XferMap( xfer, key, obj ) );
	}
};

FUBI_DECLARE_XFER_TRAITS( GoDb::StringStringDb )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		return ( persist.XferMap( xfer, key, obj ) );
	}
};

//////////////////////////////////////////////////////////////////////////////
// misc helpers

static bool LessByCloneSourceDepth( Go* l, Go* r )
{
	return ( l->GetRootDepth() < r->GetRootDepth() );
}

static bool LessByRootDepth( Go* l, Go* r )
{
	return ( l->GetCloneSourceDepth() < r->GetCloneSourceDepth() );
}

static bool IsVisitClear( Go* go )
{
	return ( !go->IsVisited() );
}

static void SortForCreation( GopColl& gos )
{
	// should not be calling this unless we're the server in an MP game
	gpassert( ::IsMultiPlayer() && ::IsServerLocal() );

	// get some vars
	Go** ib = &*gos.begin(), ** ie = &*gos.end();
	int goCount = ie - ib;

	// don't bother if nothing to do
	if ( goCount < 2 )
	{
		return;
	}

// Setup.

	// $ goal here is simply to make sure that parents appear in the list
	//   before children, and clone sources appear before things that require
	//   them. unfortunately implementation not quite so simple...

	for ( ; ; )
	{
		// clear every Go and related Go's so we can detect which are in the list
		bool sortRequired = false;
		for ( Go** i = ib ; i != ie ; ++i )
		{
			Go* csrc = (*i)->GetCloneSource();
			if ( csrc != NULL )
			{
				csrc->ClearVisit();
				sortRequired = true;
			}

			Go* parent = (*i)->GetParent();
			if ( parent != NULL )
			{
				parent->ClearVisit();
				sortRequired = true;
			}
		}

		// can bail out here, this means that we have zero dependencies
		if ( !sortRequired )
		{
			break;
		}

		// mark everything that's in the list as 'internal'
		for ( i = ib ; i != ie ; ++i )
		{
			(*i)->SetVisit();
		}

		// set flags for what's internally dependent. note that if none are
		// dependent then we can skip the sort. but if all are dependent then we
		// have a moebius strip or something.
		sortRequired = false;
		bool sortPossible = false;
		bool* flags = (bool*)::alloca( goCount ), * ibits;
		for ( i = ib, ibits = flags ; i != ie ; ++i, ++ibits )
		{
			Go* csrc   = (*i)->GetCloneSource();
			Go* parent = (*i)->GetParent     ();

			*ibits =    ((csrc   != NULL) && csrc  ->IsVisited())
					 || ((parent != NULL) && parent->IsVisited());

			if ( *ibits )
			{
				sortRequired = true;
			}
			else
			{
				sortPossible = true;
			}
		}

		// can bail out if everything is externally dependent. the list order
		// does not matter if nothing inside it refers to anything else in it.
		if ( !sortRequired )
		{
			break;
		}

		// if everything is internally dependent we have to do our best and bail...
		if ( !sortPossible )
		{
			// fall back to old sort method
			gpwarning( "SortForCreation() failed on server! Falling back to old method...\n" );
			std::sort( ib, ie, LessByCloneSourceDepth );
			std::stable_sort( ib, ie, LessByRootDepth );
			break;
		}

		// now that the comparisons are done, update visit bits with dependence
		// flags so we can sort them
		for ( i = ib, ibits = flags ; i != ie ; ++i, ++ibits )
		{
			(*i)->SetVisit( *ibits );
		}

		// partition by visit bit (set begin to mid and continue running)
		ib = std::partition( ib, ie, IsVisitClear );
	}
}

static bool FetchGlobalSyncGos( GopColl& gos, std::pair <GoDb::SiegeGoDb::iterator, GoDb::SiegeGoDb::iterator>* range )
{
	// fetch all go's
	gos.clear();
	for ( GoDb::SiegeGoDb::iterator i = range->first ; i != range->second ; ++i )
	{
		Go* go = i->second;
		if ( go->IsGlobalGo() )
		{
			gos.push_back( go );
		}
	}

	// sort and sync
	if ( !gos.empty() )
	{
		SortForCreation( gos );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb implementation

RECURSE_DECLARE( SiegeGoDb );
RECURSE_DECLARE( ExpireDb );

#if GP_RETAIL
#define CHECK_DB_LOCKED
#else // GP_RETAIL
#define CHECK_DB_LOCKED											\
	if ( gGoDb.IsLockedForSaveLoad() )							\
	{															\
		gperror( "Very bad! Someone is attempting to "			\
				 "modify the GoDb during a load/save!!" );		\
	}
#endif // GP_RETAIL

// 200 seems like a good number to distance new Goid's from old ones. it's
// actually a lot higher depending on avg bucket depth but whatever. originally
// this was 20 but goids are getting recycled wayy too quickly.
const int GO_GLOBAL_DISTANCE = 200;

GoDb :: GoDb( void )
	: m_GlobalDb  ( GlobalGoidBits::MAX_MAGIC, GO_GLOBAL_DISTANCE ),
	  m_LocalDb   ( LocalGoidBits ::MAX_MAGIC ),
	  m_CloneSrcDb( LocalGoidBits ::MAX_MAGIC )
{
	// just make sure they're constructed...
#	if !GP_RETAIL
	gGoLifeContext;
	gGoMsgContext;
	gGoUberContext;
#	endif // !GP_RETAIL

	m_ActiveFrustumMask         = FRUSTUMID_INVALID;
	m_GlobalDbCount             = 0;
	m_CloneSrcUnloadedCount     = 0;
	m_MostRecentNewGoidMain     = GOID_INVALID;
	m_MostRecentNewGoidLoader   = GOID_INVALID;
	m_MostRecentFixedSeedMain   = 0;
	m_MostRecentFixedSeedLoader = 0;
	m_ConstructingStandaloneGo  = false;
	m_ImportingStandaloneGo     = false;
	m_IsShuttingDown            = false;
	m_IsShuttingDownBad         = false;
	m_GoUpdateRoot              = new Go;
	m_PrimaryThreadId           = ::GetCurrentThreadId();
	m_CloneSrcLruTimeout        = 60.0f;
	m_CloneSrcLruCheck          = 5.0f;
	m_CloneSrcCheckTimeout      = 0.0f;
	m_DeletionRetireTime        = 30.0f;
	m_LockedForSaveLoad         = false;
	m_PostLoading				= false;

#	if !GP_RETAIL
	m_LastUpdateCount         = 0;
	m_LastRenderCount         = 0;
	m_LastViewCount           = 0;
	m_EditMode                = false;
	m_OverrideDetectOnly      = false;
	m_DebugUpdateBreakGoid    = GOID_INVALID;
	m_GlobalDbDirtyCount      = 0;
	m_GlobalDbServerOnlyCount = 0;
#	endif // !GP_RETAIL

	// reset timeout from fuel
	FastFuelHandle fuel( "config:global_settings:timeouts" );
	if ( fuel )
	{
		fuel.Get( "clonesrc_expiration_time",    m_CloneSrcLruTimeout   );
		fuel.Get( "clonesrc_expire_check_time",  m_CloneSrcCheckTimeout );
		fuel.Get( "recent_deletion_retire_time", m_DeletionRetireTime   );
	}

	// init some stuff
	RebuildCreationCaches();
	m_GoUpdateRoot->SetUpdateRoot();

	// tell siege about our fun
	gSiegeEngine.RegisterCollectObjectsCb( makeFunctor( *this, &GoDb::CollectObjects ) );
	gSiegeEngine.RegisterNodeSpaceChangedCb( makeFunctor( *this, &GoDb::OnSpaceChanged ) );
	gSiegeEngine.RegisterNodeWorldMembershipChangedCb( makeFunctor( *this, &GoDb::NodeChangeMembership ) );
	gSiegeLoadMgr.InstallLoader( LOADER_GO_PRELOAD_TYPE, new GoPreloader );

	// create import db
	FuelDB* importDb = gFuelSys.AddTextDb( "import" );
	importDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ | FuelDB::OPTION_MEM_ONLY, "", "" );

	// create import stash db
	FuelDB* importStashDb = gFuelSys.AddTextDb( "import_stash" );
	importStashDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ | FuelDB::OPTION_MEM_ONLY, "", "" );
}

GoDb :: ~GoDb( void )
{
	// remove siege callbacks
	gSiegeEngine.RegisterCollectObjectsCb( NULL );
	gSiegeEngine.RegisterNodeSpaceChangedCb( NULL );
	gSiegeEngine.RegisterNodeWorldMembershipChangedCb( NULL );
	gSiegeLoadMgr.UninstallLoader( LOADER_GO_PRELOAD_TYPE );

	// shut down in case we haven't already
	Shutdown( false );

	// delete our root go
	delete ( m_GoUpdateRoot );

	// remove import db
	gFuelSys.Remove( FuelHandle( "::import:root" )->GetDB() );

	// remove import stash db
	gFuelSys.Remove( FuelHandle( "::import_stash:root" )->GetDB() );
}

void GoDb :: Shutdown( bool clean )
{
	gpgeneric( "GoDb :: Shutdown\n" );

	gpassert( !IsConstructingGo() );
	gpassert( !m_IsShuttingDown );
	gpassert( !m_IsShuttingDownBad );

	if ( clean )
	{
		clean = !ReportSys::IsFatalOccurring();
	}

	m_IsShuttingDown = true;
	m_IsShuttingDownBad = !clean;

// Delete all go's.

	// commit everything currently pending
	CommitAllRequests( false );

	// delete all locals
	LocalDb::iterator j, jbegin = m_LocalDb.begin(), jend = m_LocalDb.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		MarkForDeletion( *j, false );
	}

	// delete all player go's
	DeletePlayerGos( PLAYERID_INVALID, false );

	// delete all clone sources
	{
		CloneSrcDb::iterator i, ibegin = m_CloneSrcDb.begin(), iend = m_CloneSrcDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->m_Go != NULL )
			{
				MarkForDeletion( i->m_Go, false );
			}
		}
	}

	// force deletions
	CommitAllRequests( false );

	// nuke the clone source db directly, it still has crud in it (unloaded go's)
	m_CloneNameDb.clear();
	m_CloneSrcDb.clear();
	m_CloneSrcUnloadedCount = 0;

	// clean out remaining db's
	ClearScidSpecifics();
	RebuildCreationCaches();
	m_QuestBitsDb.clear();

// Verify that everything is gone.

	// no more go's
	gpassert( GetTotalGoCount() == 0 );
	gpassert( GetGlobalGoDirtyCount() == 0 );
	gpassert( GetGlobalGoServerOnlyCount() == 0 );

	// indexes and other db's cleaned out
	gpassert( m_SelectionColl       .empty() );
	gpassert( m_HotColl             .empty() );
	gpassert( m_WatchingDb          .empty() );
	gpassert( m_WatchedDb           .empty() );
	gpassert( m_EffectDb            .empty() );
	gpassert( m_EffectLinkDb        .empty() );
	gpassert( m_EnchantmentDb       .empty() );
	gpassert( m_CloneSrcLockDb      .empty() );
	gpassert( m_CloneNameDb         .empty() );
	gpassert( m_SiegeGoDb           .empty() );
	gpassert( m_PendingGoDb         .empty() );
	gpassert( m_ExpireDb            .empty() );
	gpassert( m_ExpireReverseDb     .empty() );
	gpassert( m_OnDeckDb            .empty() );
	gpassert( m_TempOnDeckDb        .empty() );
	gpassert( m_GoCreateColl        .empty() );
	gpassert( m_DeleteReqColl       .empty() );
	gpassert( m_DeleteReqDelayedColl.empty() );
	gpassert( m_ScidIndex           .empty() );

#	if ( GP_DEBUG )
	{
		PlayerFrustumColl::iterator i, ibegin = m_PlayerFrustumColl.begin(), iend = m_PlayerFrustumColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpassert( i->m_Focus == i->m_Coll.end() );
			gpassert( i->m_Coll.empty() );
		}
	}
#	endif // GP_DEBUG

	m_PlayerFrustumColl.clear();

	gpassert( m_ActiveFrustumMask == FRUSTUMID_INVALID );

// Reset misc.

	// free memory
	m_GlobalDb  .Shutdown();
	m_LocalDb   .Shutdown();
	m_CloneSrcDb.Shutdown();

	// unneeded stat db's
	m_RecentDeletionColl.clear();
	m_RecentDeletionRevDb.clear();

	// reset vars
	m_MostRecentNewGoidMain     = GOID_INVALID;
	m_MostRecentNewGoidLoader   = GOID_INVALID;
	m_MostRecentFixedSeedMain   = 0;
	m_MostRecentFixedSeedLoader = 0;
	m_ConstructingStandaloneGo  = false;
	m_ImportingStandaloneGo     = false;

#	if !GP_RETAIL

	// reset vars
	m_LastUpdateCount         = 0;
	m_LastRenderCount         = 0;
	m_LastViewCount           = 0;
	m_DebugUpdateBreakGoid    = GOID_INVALID;
	m_GlobalDbDirtyCount      = 0;
	m_GlobalDbServerOnlyCount = 0;

#	endif // !GP_RETAIL

	m_IsShuttingDown = false;
	m_IsShuttingDownBad = false;
}

void GoDb :: SetLockedForSaveLoad( bool set )
{
	gpassert( m_LockedForSaveLoad != set );
	m_LockedForSaveLoad = set;

	// do skrit engine too, godb is closely tied to it
	gSkritEngine.SetLockedForSaveLoad( set );
}

bool GoDb :: Xfer( FuBi::PersistContext& persist )
{
	RatioSample ratioSample( "xfer_godb" );

	// must not be in the middle of doing anything!
	gpassert( m_OnDeckDb.empty() );
	gpassert( m_TempOnDeckDb.empty() );
	gpassert( m_MostRecentNewGoidMain   == GOID_INVALID );
	gpassert( m_MostRecentNewGoidLoader == GOID_INVALID );
	gpassert( m_MostRecentFixedSeedMain   == 0 );
	gpassert( m_MostRecentFixedSeedLoader == 0 );
	gpassert( !m_ConstructingStandaloneGo );
	gpassert( !m_ImportingStandaloneGo );
	gpassert( !m_Adder.IsActive() );
	gpassert( m_GoCreateColl.empty() );
	gpassert( m_DeleteReqColl.empty() );
	gpassert( m_DeleteReqDelayedColl.empty() );

	// $ very important to do these in proper order - all goids must be valid
	//   and pointing to alloc'd go's in order to properly resolve goids to
	//   go*'s. do the clone sources first, then the locals, then the globals.

// Xfer external systems that are Go-related.

	// must xfer the member map!!
	Membership::StaticXfer( persist );

// Preprocess databases.

	// first do all magic numbers
	UINT magic = m_GlobalDb.GetCurrentMagic();
	persist.Xfer( "m_GlobalDbMagic", magic );
	m_GlobalDb.SetCurrentMagic( magic );
	magic = m_CloneSrcDb.GetCurrentMagic();
	persist.Xfer( "m_CloneSrcDbMagic", magic );
	m_CloneSrcDb.SetCurrentMagic( magic );
	magic = m_LocalDb.GetCurrentMagic();
	persist.Xfer( "m_LocalDbMagic", magic );
	m_LocalDb.SetCurrentMagic( magic );

	// now prealloc our collections so goid's are valid
	if ( persist.IsRestoring() )
	{
		// sanity check
		gpassert( m_GlobalDb  .empty() );
		gpassert( m_CloneSrcDb.empty() );
		gpassert( m_LocalDb   .empty() );

		// do global db
		persist.EnterColl( "m_GlobalDb" );
		while ( persist.AdvanceCollIter() )
		{
			// get goid
			Goid goid;
			persist.Xfer( "m_Goid", goid );

			// alloc bucket
			GoBucket* bucket;
			GlobalGoidBits bits( goid );
			if ( m_GlobalDb.AllocSpecific( bucket, bits.m_MajorIndex, bits.m_Magic ) )
			{
				bucket->m_First = NULL;
				bucket->m_Count = 0;
			}

			// get at the actual go slot
			Go** newGo = NULL;
			if ( bits.m_MinorIndex == 0 )
			{
				newGo = &bucket->m_First;
			}
			else
			{
				// $ note that minor index is 1 greater than 'remaining' index

				if ( bucket->m_Remaining.size() < bits.m_MinorIndex )
				{
					bucket->m_Remaining.resize( bits.m_MinorIndex, NULL );
				}
				newGo = &bucket->m_Remaining[ bits.m_MinorIndex - 1 ];
			}

			// build new empty go at that slot
			gpassert( *newGo == NULL );
			*newGo = new Go;

			// update counts
			++bucket->m_Count;
			++m_GlobalDbCount;
		}
		persist.LeaveColl();

		gpstring templateName;

		// do clone source db
		persist.EnterColl( "m_CloneSrcDb" );
		while ( persist.AdvanceCollIter() )
		{
			templateName.clear();
			Goid goid;
			int lockCount;

			// get params
			persist.Xfer( "_goid", goid );
			gpassert( goid != GOID_INVALID );
			persist.Xfer( "_type", templateName );
			persist.Xfer( "_lock", lockCount );

			// alloc spot
			CloneBucket* newClone;
			LocalGoidBits bits( goid );
			gpverify( m_CloneSrcDb.AllocSpecific( newClone, bits.m_Index, bits.m_Magic ) );

			// check that the template exists
			if ( gContentDb.FindTemplateByName( templateName ) != NULL )
			{
				// take template name and add to index
				newClone->m_TemplateName = templateName;
				newClone->m_LockCount = lockCount;
				m_CloneNameDb[ templateName ] = goid;
				gpassert( m_CloneNameDb.size() == m_CloneSrcDb.size() );
			}
			else
			{
				// failed!
				gpreportf( persist.GetReportContext(),
					( "Unable to load/find template '%s' for clone source goid 0x%08X\n",
					  templateName.c_str(), goid ));
				m_CloneSrcDb.Free( bits.m_Index );
			}
		}
		persist.LeaveColl();

		// do local db
		persist.EnterColl( "m_LocalDb" );
		while ( persist.AdvanceCollIter() )
		{
			// get goid
			Goid goid;
			persist.Xfer( "m_Goid", goid );

			// alloc spot
			Go** newGo;
			LocalGoidBits bits( goid );
			gpverify( m_LocalDb.AllocSpecific( newGo, bits.m_Index, bits.m_Magic ) );

			// build new empty go at that slot
			*newGo = new Go;
		}
		persist.LeaveColl();
	}

// Xfer go's.

	{
		RatioSample ratioSample( "godb_xfer_go_components", GetGlobalGoCount() + GetLocalGoCount(), 0.13 );

		persist.EnterColl( "m_GlobalDb" );
		{
			int failedLoads = 0;

			GlobalIter i, begin = GlobalBegin(), end = GlobalEnd();
			for ( i = begin ; i != end ; ++i )
			{
				persist.AdvanceCollIter();
				bool rc = i->Xfer( persist );
				GPDEBUG_ONLY( i->m_DbgInGoDb = true );

				ratioSample.Advance( rc ? i->GetTemplateName() : "<ERROR>" );

				if ( persist.IsRestoring() )
				{
					if ( rc )
					{
						Scid scid = i->GetScid();
						gpassert( scid != SCID_INVALID );
						if ( scid != SCID_SPAWNED )
						{
							gpassert( !HasGoWithScid( scid ) );
							m_ScidIndex[ scid ] = &*i;
						}
					}
					else
					{
						++failedLoads;
					}
				}
				else
				{
					gpassert( rc );
				}
			}

			// we have to fatal at this point, sorry too many dependencies
			if ( failedLoads > 0 )
			{
				persist.ReportPersistErrors();
				gpfatalf(( "This game cannot be loaded due to a content/save game "
						   "data mismatch (%d game objects failed to persist), and "
						   "the system is now unstable. Sorry but there is no "
						   "choice but to fatal...\n", failedLoads ));
			}
		}
		persist.LeaveColl();

		persist.Xfer( "m_GlobalDbCount",         m_GlobalDbCount         );

		persist.EnterColl( "m_LocalDb" );
		{
			LocalDb::iterator i, begin = m_LocalDb.begin(), end = m_LocalDb.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( persist.IsRestoring() || (*i)->ShouldPersist() )
				{
					persist.AdvanceCollIter();
					(*i)->Xfer( persist );
					GPDEBUG_ONLY( (*i)->m_DbgInGoDb = true );

					ratioSample.Advance( (*i)->GetTemplateName() );

					if ( persist.IsRestoring() )
					{
						Scid scid = (*i)->GetScid();
						gpassert( scid != SCID_INVALID );
						if ( scid != SCID_SPAWNED )
						{
							gpassert( !HasGoWithScid( scid ) );
							m_ScidIndex[ scid ] = *i;
						}
					}
				}
			}
		}
		persist.LeaveColl();
	}

	// $ note: clone sources only need to save template name. they may not be
	//         modified in-game so no need to save them out exactly. also no
	//         need to restore them, that will be done on-demand. if this causes
	//         fps hitches immediately after loading the game, then this can be
	//         fixed by saving out whether or not the go existed at save time,
	//         and derefing all those post-restore game.

	if ( persist.IsSaving() )
	{
		persist.EnterColl( "m_CloneSrcDb" );
		{
			for ( CloneSrcDb::iterator i = m_CloneSrcDb.begin() ; i != m_CloneSrcDb.end() ; ++i )
			{
				persist.AdvanceCollIter();

				Goid goid = MakeCloneSrcGoid( i.GetIndex(), i.GetMagic() );
				persist.Xfer( "_goid", goid );
				persist.Xfer( "_type", i->m_TemplateName );
				persist.Xfer( "_lock", i->m_LockCount );
			}
		}
		persist.LeaveColl();
	}
	else
	{
		// start thinking everything has been unloaded
		m_CloneSrcUnloadedCount = m_CloneSrcDb.size();

		// grab all locked clone sources
		GoidColl goids;
		CloneSrcDb::const_iterator i, begin = m_CloneSrcDb.begin(), end = m_CloneSrcDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( i->m_LockCount > 0 )
			{
				goids.push_back( MakeCloneSrcGoid( i.GetIndex(), i.GetMagic() ) );
			}
		}

		// create them all
		GoidColl::iterator j, jbegin = goids.begin(), jend = goids.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			LocalGoidBits bits( *j );
			CloneBucket* bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );
			gpassert( (bucket != NULL) && (bucket->m_Go == NULL) );

			if ( bucket->m_LockCount > 0 )
			{
				int lockCount = bucket->m_LockCount;
				bucket->m_LockCount = 0;
				CreateCloneSrcGo( *j, bucket, NULL );
				bucket->m_LockCount = lockCount;
			}
		}
	}

// Xfer go components.

	{
		int failedLoads = 0;

		RatioSample ratioSample( "godb_xfer_go_components", GetGlobalGoCount() + GetLocalGoCount(), 0.63 );

		persist.EnterColl( "m_GlobalDb" );
		{
			GlobalIter i, begin = GlobalBegin(), end = GlobalEnd();
			for ( i = begin ; i != end ; ++i )
			{
				persist.AdvanceCollIter();
				if ( !i->XferComponents( persist ) )
				{
					++failedLoads;
				}

				ratioSample.Advance( i->GetTemplateName() );
			}
		}
		persist.LeaveColl();

		persist.EnterColl( "m_LocalDb" );
		{
			LocalDb::iterator i, begin = m_LocalDb.begin(), end = m_LocalDb.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( persist.IsRestoring() || (*i)->ShouldPersist() )
				{
					persist.AdvanceCollIter();
					if ( !(*i)->XferComponents( persist ) )
					{
						++failedLoads;
					}

					ratioSample.Advance( (*i)->GetTemplateName() );
				}
			}
		}
		persist.LeaveColl();

		// we have to fatal at this point, sorry too many dependencies
		if ( failedLoads > 0 )
		{
			persist.ReportPersistErrors();
			gpfatalf(( "This game cannot be loaded due to a content/save game "
					   "data mismatch (%d game objects failed to persist), and "
					   "the system is now unstable. Sorry but there is no "
					   "choice but to fatal...\n", failedLoads ));
		}
	}

// Xfer simple stuff.

	// safety check
#	if !GP_RETAIL
	if ( persist.IsSaving() )
	{
		EffectDb::iterator i, ibegin = m_EffectDb.begin(), iend = m_EffectDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Flamethrower_script* script = NULL;
			if ( !gFlamethrower.FindScript( i->second.m_ScriptId, &script ) )
			{
				gperrorf((
						"Bad! Someone deleted an FX script without telling the "
						"GoDb! Well anyway here's the LEAKED registration "
						"info...\n"
						"\n"
						"goid          : 0x%08X\n"
						"scid          : 0x%08X\n"
						"template      : %s\n"
						"script id     : 0x%08X (does not exist)\n"
						"creation event: %s\n"
						"active        : %s\n"
						"xferOnSync    : %s\n",
						i->first->GetGoid(),
						i->first->GetScid(),
						i->first->GetTemplateName(),
						i->second.m_ScriptId,
						::ToString( i->second.m_CreationEvent ),
						i->second.m_Active ? "yes" : "no",
						i->second.m_XferOnSync ? "yes" : "no" ));
			}
			else if ( script->GetOwner() != i->first->GetGoid() )
			{
				GoHandle badOwner( script->GetOwner() );
				gperrorf((
						"Bad! I found an FX script whose owner no longer "
						"matches who it was originally registered with! "
						"Here's the registration info...\n"
						"\n"
						"goid          : 0x%08X\n"
						"scid          : 0x%08X\n"
						"template      : %s\n"
						"script id     : 0x%08X (%s)\n"
						"script owner  : 0x%08X (%s)\n"
						"creation event: %s\n"
						"active        : %s\n",
						"xferOnSync    : %s\n",
						i->first->GetGoid(),
						i->first->GetScid(),
						i->first->GetTemplateName(),
						script->GetID(), script->GetName().c_str(),
						script->GetOwner(), badOwner ? badOwner->GetTemplateName() : "<NONEXISTENT>",
						::ToString( i->second.m_CreationEvent ),
						i->second.m_Active ? "yes" : "no",
						i->second.m_XferOnSync ? "yes" : "no" ));
			}
		}
	}
#	endif // !GP_RETAIL

	// this seems to happen sometimes, check for it until we find that bug
	{
		SiegeGoDb::iterator i, ibegin = m_SiegeGoDb.begin(), iend = m_SiegeGoDb.end();
		for ( i = ibegin ; i != iend ; )
		{
			Go* go = i->second;
			if ( go != NULL )
			{
#				if !GP_RETAIL
				GoHandle hgo( go->GetGoid() );
				if ( hgo != go )
				{
					gperrorf(( "Big problem! Somehow a Go is stored in the SiegeGoDb at node %s yet when resolving its Goid 0x%08X, it does not point back to itself!\n",
							   i->first.ToString().c_str(), go->GetGoid() ));
				}
#				endif // !GP_RETAIL
			}
			else
			{
				gperrorf(( "Big problem! Somehow a NULL Go is stored in the SiegeGoDb at node %s!!\n",
						   i->first.ToString().c_str() ));

				// ok attempt to "repair" this... there will probably be other
				// problems, but try to limp along...jeeez....
				i = m_SiegeGoDb.erase( i );
				continue;
			}

			++i;
		}
	}

	// xfer db's and such
	persist.XferVector( "m_SelectionColl",			m_SelectionColl     );
	persist.XferVector( "m_HotColl",				m_HotColl           );
	persist.XferVector( "m_PlayerFrustumColl",		m_PlayerFrustumColl );
	persist.Xfer      ( "m_ActiveFrustumMask",		m_ActiveFrustumMask );
	persist.XferMap   ( "m_WatchingDb",				m_WatchingDb        );
	persist.XferMap   ( "m_WatchedDb",				m_WatchedDb         );
	persist.XferIfMap ( "m_EffectDb",				m_EffectDb          , GoDbShouldPersistFirst() );
	persist.XferMap   ( "m_EnchantmentDb",			m_EnchantmentDb     );
	persist.XferVector( "m_EnchantmentRemoveQueue",	m_EnchantmentRemoveQueue	);
	persist.XferIfMap ( "m_SiegeGoDb",				m_SiegeGoDb         , GoDbShouldPersistSecond() );
	persist.XferMap   ( "m_CloneSrcLockDb",			m_CloneSrcLockDb    );
	persist.XferSet   ( "m_RetiredScidColl",		m_RetiredScidColl   );
	persist.XferMap   ( "m_ScidBitsColl",			m_ScidBitsColl      );
	persist.XferMap   ( "m_QuestBitsDb",			m_QuestBitsDb       );
	persist.XferIfMap ( "m_ExpireDb",				m_ExpireDb          , GoDbShouldPersistSecond() );

	// update cache bits
	if ( persist.IsRestoring() )
	{
		EnchantmentDb::iterator i, ibegin = m_EnchantmentDb.begin(), iend = m_EnchantmentDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			i->first->m_IsEnchanted = true;
		}
	}

	// verify that our lock db matches what we're expecting
#	if !GP_RETAIL
	{
		typedef std::map <Goid, int> GoidIntDb;
		GoidIntDb goidIntDb;

		GoGoidDb::iterator i, ibegin = m_CloneSrcLockDb.begin(), iend = m_CloneSrcLockDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i->first->IsInAnyWorldFrustum() )
			{
				++goidIntDb.insert( std::make_pair( i->second, 0 ) ).first->second;
			}
		}

		GoidIntDb::iterator j, jbegin = goidIntDb.begin(), jend = goidIntDb.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			Goid goid = j->first;
			LocalGoidBits bits( goid );
			CloneBucket* bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );
			if ( bucket != NULL )
			{
				if ( bucket->m_LockCount != j->second )
				{
					gperrorf(( "Clone source lock database entry count mismatch for Go 0x%08X (%s), found: %d, expected: %d\n",
							   goid, bucket->m_TemplateName.c_str(), bucket->m_LockCount, j->second ));
					bucket->m_LockCount = j->second;
				}
			}
			else
			{
				gperrorf(( "Clone source lock database entry found for nonexistent Go 0x%08X!\n", goid ));
			}
		}
	}
#	endif // !GP_RETAIL

	// xfer the mouse shadow - note that we have to do this special because
	// there may be lodfi go's in there (signs etc).
	if ( persist.IsSaving() )
	{
		GopColl mouseShadowColl( m_MouseShadowColl );
		for ( GopColl::iterator i = mouseShadowColl.begin() ; i != mouseShadowColl.end() ; )
		{
			if ( (*i)->IsLodfi() )
			{
				i = mouseShadowColl.erase( i );
			}
			else
			{
				++i;
			}
		}
		persist.XferVector( "m_MouseShadowColl", mouseShadowColl );
	}
	else
	{
		persist.XferVector( "m_MouseShadowColl", m_MouseShadowColl );
	}

	// xfer the pending-go mapping
	if ( persist.IsSaving() )
	{
		persist.EnterColl( "m_PendingGoDb" );

		SiegeGopCollDb::iterator i, ibegin = m_PendingGoDb.begin(), iend = m_PendingGoDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// first see if any should persist
			GopColl::iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( (*j != NULL) && (*j)->ShouldPersist() )
				{
					break;
				}
			}

			// persist if necessary
			if ( j != jend )
			{
				persist.AdvanceCollIter();
				persist.Xfer( "_key", ccast <siege::database_guid&> ( i->first ) );

				persist.EnterColl( FuBi::PersistContext::VALUE_STR );

				GopColl::iterator j, jbegin = i->second.begin(), jend = i->second.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( (*j != NULL) && (*j)->ShouldPersist() )
					{
						persist.AdvanceCollIter();
						persist.Xfer( FuBi::PersistContext::VALUE_STR, *j );
					}
				}

				persist.LeaveColl();
			}
		}

		persist.LeaveColl();
	}
	else
	{
		persist.XferMap( "m_PendingGoDb", m_PendingGoDb );
	}

	// restore reverse index
	if ( persist.IsRestoring() )
	{
		ExpireDb::iterator i, ibegin = m_ExpireDb.begin(), iend = m_ExpireDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			m_ExpireReverseDb[ i->second.m_Go ] = i;
		}
	}

#	if !GP_RETAIL
	persist.Xfer( "m_LastUpdateCount",         m_LastUpdateCount         );
	persist.Xfer( "m_LastRenderCount",         m_LastRenderCount         );
	persist.Xfer( "m_LastViewCount",           m_LastViewCount           );
	persist.Xfer( "m_EditMode",                m_EditMode                );
	persist.Xfer( "m_OverrideDetectOnly",      m_OverrideDetectOnly      );
	persist.Xfer( "m_DebugUpdateBreakGoid",    m_DebugUpdateBreakGoid    );
	persist.Xfer( "m_GlobalDbDirtyCount",      m_GlobalDbDirtyCount      );
	persist.Xfer( "m_GlobalDbServerOnlyCount", m_GlobalDbServerOnlyCount );
#	endif // !GP_RETAIL

// Finalize all Go's.

	// only on a restore
	if ( persist.IsRestoring() )
	{
		RatioSample ratioSample( "godb_xfer_commit_load", GetGlobalGoCount() + GetLocalGoCount() );

		// commit global go's
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			i->CommitLoad();

			ratioSample.Advance( i->GetTemplateName() );
		}

		// commit local go's
		LocalDb::iterator j, jbegin = m_LocalDb.begin(), jend = m_LocalDb.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			(*j)->CommitLoad();

			ratioSample.Advance( (*j)->GetTemplateName() );
		}

		// commit clone sources
		CloneSrcDb::iterator k, kbegin = m_CloneSrcDb.begin(), kend = m_CloneSrcDb.end();
		for ( k = kbegin ; k != kend ; ++k )
		{
			if ( k->m_Go != NULL )
			{
				k->m_Go->CommitLoad();
			}
		}
	}
#	if GP_DEBUG
	else
	{
		// check validity of global go's
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			i->AssertValid();
		}

		// check validity of local go's
		LocalDb::iterator j, jbegin = m_LocalDb.begin(), jend = m_LocalDb.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			(*j)->AssertValid();
		}

		// check validity of clone sources
		CloneSrcDb::iterator k, kbegin = m_CloneSrcDb.begin(), kend = m_CloneSrcDb.end();
		for ( k = kbegin ; k != kend ; ++k )
		{
			if ( k->m_Go != NULL )
			{
				k->m_Go->AssertValid();
			}
			gpassert( gContentDb.FindTemplateByName( k->m_TemplateName ) != NULL );
		}
	}
#	endif // GP_DEBUG

	if ( persist.IsRestoring() )
	{
		gGoDb.SetLockedForSaveLoad( false );
	}

	// Now that we have a valid GoDb, double check all the NemaAspect to ensure that all the
	// parent-child links are valid. Passing in 'true' will break any links that are invalid.
	if ( persist.IsRestoring() )
	{
		gAspectStorage.ValidateParentLinks( !GP_DEBUG );
	}

	// done
	return ( true );
}

void GoDb :: XferFxForSync( FuBi::BitPacker& packer, Go* go )
{
	if ( packer.IsSaving() )
	{
		// xfer each effect on this go directly
		std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go );
		for ( ; effects.first != effects.second ; ++effects.first )
		{
			effects.first->second.WriteForSync( packer );
		}

		// find linked effects and xfer those too
		std::pair <GoGoDb::const_iterator, GoGoDb::const_iterator> targets = m_EffectLinkDb.equal_range( go );
		for ( ; targets.first != targets.second ; ++targets.first )
		{
			effects = m_EffectDb.equal_range( targets.first->second );
			for ( ; effects.first != effects.second ; ++effects.first )
			{
				effects.first->second.WriteForSync( packer, go->GetGoid() );
			}
		}

		// terminate
		packer.WriteBit( false );
	}
	else
	{
		while ( packer.ReadBit() )
		{
			RunScriptXfer xfer;
			xfer.Xfer( packer, false );

			// we may have gotten this due to xfer + rpc crossing paths so make
			// sure it's not already there
			Flamethrower_script* script = NULL;
			if( !gFlamethrower_interpreter.FindScript( xfer.m_ScriptId, &script ) )
			{
				gFlamethrower_interpreter.RunScript(
						xfer.m_Name,
						xfer.m_Targets,
						xfer.m_Params,
						0,
						(xfer.m_ScriptOwner == GOID_INVALID) ? go->GetGoid() : xfer.m_ScriptOwner,
						::RandomDword(),
						xfer.m_Type,
						xfer.m_ScriptId,
						false );
			}
		}
	}
}

void GoDb :: XferQuestBits( FuBi::PersistContext& persist, Goid goid )
{
	if ( persist.IsSaving() )
	{
		QuestBitsDb::iterator found = m_QuestBitsDb.find( goid );
		if ( found != m_QuestBitsDb.end() )
		{
			persist.XferMap( FuBi::XFER_QUOTED, "questbits", found->second );
		}
	}
	else
	{
		StringStringDb& db = m_QuestBitsDb[ goid ];
		db.clear();
		persist.XferMap( FuBi::XFER_QUOTED, "questbits", db );
	}
}

bool GoDb :: PostLoad( void )
{
	// here to signify we have already called commit create
	gpassert( m_PostLoading == false );
	m_PostLoading = true;

	// add up how many nodes we gonna load
	Player* screenPlayer = gServer.GetScreenPlayer();
	if ( screenPlayer != NULL )
	{
		int nodeCount = 0;

		FrustumId screenFrusta = GetPlayerFrustumMask( screenPlayer->GetId() );
		for ( FrustumId i = FindFirstEnum( screenFrusta ) ; i ; i = FindNextEnum( i, screenFrusta ) )
		{
			siege::SiegeFrustum* screenFrustum = gSiegeEngine.GetFrustum( (DWORD)i );
			gpassert( screenFrustum != NULL );

			nodeCount += screenFrustum->GetFrustumNodeColl().size()
					  +  screenFrustum->GetLoadedNodeColl ().size();
		}

		RatioSample ratioSample( "godb_xfer_load_lodfi", nodeCount, 0.80 );

		// tell the world to load in all lodfi-based go's
		for ( i = FindFirstEnum( screenFrusta ) ; i ; i = FindNextEnum( i, screenFrusta ) )
		{
			siege::SiegeFrustum* screenFrustum = gSiegeEngine.GetFrustum( (DWORD)i );
			gpassert( screenFrustum != NULL );

			siege::FrustumNodeColl::const_iterator j,
					jbegin = screenFrustum->GetFrustumNodeColl().begin(),
					jend   = screenFrustum->GetFrustumNodeColl().end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				gWorldMap.LoadNodeContent( (*j)->GetGUID(), true, true );
				ratioSample.Advance( (*j)->GetGUID().ToString() );
			}

			jbegin = screenFrustum->GetLoadedNodeColl().begin();
			jend   = screenFrustum->GetLoadedNodeColl().end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				gWorldMap.LoadNodeContent( (*j)->GetGUID(), true, true );
				ratioSample.Advance( (*j)->GetGUID().ToString() );
			}
		}
	}

	// do the final commit to bring it all in
	{
		RatioSample ratioSample( "godb_xfer_commit", 0, 0.10 );
		CommitAllRequests();
	}

	// any outstanding textures?
	{
		RatioSample ratioSample( "" );
		gSiegeLoadMgr.CommitAll();
	}

	// here to signify we have already called commit create
	m_PostLoading = false;

	return ( true );
}

void GoDb :: ImportCharacter( PlayerId playerId, int slot, const gpstring& spec )
{
	// get at it
	FuelHandle charac = FindImportCharacterFuel( playerId, slot, true );

	// kill whatever is already there
	FuelHandle base = charac->GetParent();
	charac.Delete();

	// load our spec in
	base->AddChildren( spec, spec.length() );
}

void GoDb :: ImportStash( PlayerId playerId, const gpstring& spec )
{
	FuelHandle hPlayerStash = FindImportStashFuel( playerId, true );

	// kill whatever is already there
	FuelHandle hBase = hPlayerStash->GetParent();
	hPlayerStash.Delete();

	// load our spec in
	hBase->AddChildren( spec, spec.length() );					
}

static void UpdateLevelOfDetail( siege::FrustumNodeColl& coll, DWORD uniqueId )
{
	siege::FrustumNodeColl::iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		siege::SiegeNode* node = *i;
		if ( node->GetLastVisited() != uniqueId )
		{
			node->SetLastVisited( uniqueId );
			gWorldMap.SyncNodeLodfiContent( node->GetGUID() );
		}
	}
}

void GoDb :: UpdateLevelOfDetail( void )
{
	// ignore if nothing there
	if ( !gWorldMap.IsInitialized() )
	{
		return;
	}

	// commit any pending node loading
	gWorldTerrain.ForceWorldLoad();
	DWORD uniqueId = ::RandomDword();

	// adjust lodfi in all frusta
	FrustumId frustumIds = GetPlayerFrustumMask( gServer.GetScreenPlayer()->GetId() );
	for ( FrustumId i = ::FindFirstEnum( frustumIds ) ; i != FRUSTUMID_INVALID ; i = ::FindNextEnum( i, frustumIds ) )
	{
		siege::SiegeFrustum* frustum = gSiegeEngine.GetFrustum( (DWORD)i );
		::UpdateLevelOfDetail( frustum->GetFrustumNodeColl(), uniqueId );
		::UpdateLevelOfDetail( frustum->GetLoadedNodeColl(), uniqueId );
	}

	// avoid white texture syndrome!
	gSiegeLoadMgr.CommitAll();
}

void GoDb :: UpdateShadows( void )
{
	// Get all goes
	GoidColl allGos;
	GetAllGlobalGoids( allGos );
	GetAllLocalGoids( allGos, true );

	for( GoidColl::iterator i = allGos.begin(); i != allGos.end(); ++i )
	{
		GoHandle go( (*i) );
		if( go->HasAspect() && !go->IsCloneSourceGo() )
		{
			// Update the shadow texture for this go
			go->GetAspect()->CreateComplexShadowTexture();
		}
	}
}

FuelHandle GoDb :: FindImportCharacterFuel( PlayerId playerId, int slot, bool autoCreate )
{
	// $ format: ::import:<player_index>:<slot>:* (currently only slot 0 supported)

	if ( playerId == PLAYERID_INVALID )
	{
		Player* player = gServer.GetScreenPlayer();
		if ( player != NULL )
		{
			playerId = player->GetId();
		}
	}

	FuelHandle charac;

	if ( playerId != PLAYERID_INVALID )
	{
		gpstring blockName = ToString( MakeIndex( playerId ) );

		FuelHandle import( "::import:root" );
		FuelHandle player = import->GetChildBlock( blockName );
		if ( !player && autoCreate )
		{
			player = import->CreateChildBlock( blockName, blockName );
		}

		if ( player )
		{
			charac = player->GetChildBlock( ToString( slot ), autoCreate );
			if ( charac )
			{
				charac = charac->GetChildBlock( "member", autoCreate );
			}
		}
	}

	return ( charac );
}

gpstring GoDb :: MakeImportCharacterFuelAddress( PlayerId playerId, int slot )
{
	if ( playerId == PLAYERID_INVALID )
	{
		Player* player = gServer.GetScreenPlayer();
		if ( player != NULL )
		{
			playerId = player->GetId();
		}
	}

	gpstring address;

	if ( playerId != PLAYERID_INVALID )
	{
		address = "::import:";
		address += ToString( MakeIndex( playerId ) );
		address += ':';
		address += ToString( slot );
		address += ":member";
	}

	return ( address );
}

FuelHandle GoDb :: FindImportStashFuel( PlayerId playerId, bool autoCreate )
{
	if ( playerId == PLAYERID_INVALID )
	{
		Player* player = gServer.GetScreenPlayer();
		if ( player != NULL )
		{
			playerId = player->GetId();
		}
	}

	FuelHandle hPlayerStash;
	if ( playerId != PLAYERID_INVALID )
	{
		gpstring blockName = ToString( MakeIndex( playerId ) );

		FuelHandle hImport( "::import_stash:root" );
		FuelHandle hPlayer = hImport->GetChildBlock( blockName );
		if ( !hPlayer )
		{
			hPlayer = hImport->CreateChildBlock( blockName, blockName );
		}

		if ( hPlayer )
		{
			hPlayerStash = hPlayer->GetChildBlock( "stash", autoCreate );			
		}
	}

	return ( hPlayerStash );
}

gpstring GoDb :: MakeImportStashFuelAddress( PlayerId playerId )
{
	if ( playerId == PLAYERID_INVALID )
	{
		Player* player = gServer.GetScreenPlayer();
		if ( player != NULL )
		{
			playerId = player->GetId();
		}
	}

	gpstring address;

	if ( playerId != PLAYERID_INVALID )
	{
		address = "::import_stash:";
		address += ToString( MakeIndex( playerId ) );
		address += ":stash";
	}

	return ( address );
}

void GoDb :: SysUpdate( float /*deltaTime*/ )
{
#	if !GP_RETAIL
	m_LastUpdateCount = 0;
	m_LastRenderCount = 0;
	m_LastViewCount = 0;
#	endif // !GP_RETAIL
}

void GoDb :: Update( float deltaTime, float actualDeltaTime )
{
	GPPROFILERSAMPLE( "GoDb :: Update", SP_GODB );
	CHECK_DB_LOCKED;

	// only bother checking if we aren't doing a forced update
	if ( (deltaTime != 0) || (actualDeltaTime != 0) )
	{
		SINGLETON_ONCE_PER_SIM;
	}

	gpassert( m_ScidIndex.find( SCID_SPAWNED ) == m_ScidIndex.end() );
	gpassert( m_ScidIndex.find( SCID_INVALID ) == m_ScidIndex.end() );
	gpassert( m_RetiredScidColl.find( SCID_SPAWNED ) == m_RetiredScidColl.end() );
	gpassert( m_RetiredScidColl.find( SCID_INVALID ) == m_RetiredScidColl.end() );
	gpassert( m_ScidBitsColl.find( SCID_SPAWNED ) == m_ScidBitsColl.end() );
	gpassert( m_ScidBitsColl.find( SCID_INVALID ) == m_ScidBitsColl.end() );
	GPDEV_ONLY( m_HudInfoDb.clear() );

// Startup.

	double worldtime = gWorldTime.GetTime();

	CommitAllRequests( false );

	// pre-dirty the modifiers that enchantments may be modifying
	{
		RwCritical::ReadLock readLock( m_EnchantmentCritical );

		EnchantmentDb::iterator i, begin = m_EnchantmentDb.begin(), end = m_EnchantmentDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			i->first->SetModifiersDirty();
		}
	}

	// clear visible items set
	m_VisibleItemsDb.clear();

// Update Go's.

	// go through and update all objects in the scene
	{
		GPPROFILERSAMPLE( "GoDb :: Update() - update all objects in scene", SP_GODB );

		for ( Go* i = m_GoUpdateRoot->m_NextUpdate ; i != m_GoUpdateRoot ; )
		{
			// this go may be pending removal
			if ( i->IsPendingUpdateRemoval() )
			{
				// do it
				i = i->PrivateStopUpdates();
				gpassert( i != NULL );
			}
			else
			{
				if ( i->IsInActiveWorldFrustum() || i->IsOmni() )
				{
					// update debug flags
#					if ( !GP_RETAIL )
					{
						if ( i->GetGoid() == m_DebugUpdateBreakGoid )
						{
							m_DebugUpdateBreakGoid = GOID_INVALID;
							BREAKPOINT();
						}
					}
					++m_LastUpdateCount;
#					endif // !GP_RETAIL

					// update it if it wasn't marked for deletion by a previous
					// go's update in this loop.
					if ( !i->IsMarkedForDeletion() )
					{
						i->Update( deltaTime );
					}
				}
				else if ( i->IsSelected() )
				{
					i->Deselect();
				}

				// advance
				if_gpassert ( i->m_NextUpdate != NULL )
				{
					i = i->m_NextUpdate;
				}
				else
				{
					i = m_GoUpdateRoot;
				}
			}
		}
	}

	// now do the "special" update that happens on all party members
	{
		const Server::PlayerColl& playerColl = gServer.GetPlayers();
		Server::PlayerColl::const_iterator i, ibegin = playerColl.begin(), iend = playerColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Player* player = *i;

			if ( IsValid(player) && !player->IsComputerPlayer() )
			{
				const GopColl& parties = player->GetPartyColl();
				GopColl::const_iterator j, jbegin = parties.begin(), jend = parties.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					Go* party = *j;

					const GopColl& members = party->GetChildren();
					GopColl::const_iterator k, kbegin = members.begin(), kend = members.end();
					for ( k = kbegin ; k != kend ; ++k )
					{
						Go* member = *k;
						member->UpdateSpecial( deltaTime );
					}

					// do it on the party too, it needs it
					party->UpdateSpecial( deltaTime );
				}
			}
		}
	}

// Process triggers.

	gTriggerSys.ProcessSignals( worldtime );
	gTriggerSys.Update();

// Process enchantments.

	{
		GPPROFILERSAMPLE( "GoDb :: Update() - process enchantments", SP_GODB );

		// send 'em
		if ( m_EnchantmentDb.empty() )
		{
			RwCritical::WriteLock writeLock( m_EnchantmentCritical );

			EnchantmentDb::iterator i, begin = m_EnchantmentDb.begin(), end = m_EnchantmentDb.end();
			for ( i = begin ; i != end ; )
			{
				// see if we should keep it
				if ( i->second->HasEnchantments() )
				{
					++i;
				}
				else
				{
					delete ( i->second );
					i = m_EnchantmentDb.erase( i );
				}
			}
		}

		// process any pending requests
		if ( !m_EnchantmentRemoveQueue.empty() )
		{
			GoidColl removeQueue;
			{
				RwCritical::ReadLock writeLock( m_EnchantmentCritical );
				m_EnchantmentRemoveQueue.swap( removeQueue );
			}

			// process queued enchantment removal requests
			GoidColl::iterator i = removeQueue.begin(), end = removeQueue.end();
			for ( ; i != end ; i += 2 )
			{
				RemoveEnchantments( *i, *(i + 1), true );
			}
		}
	}

// Process expirations.

	// $ note that local go's can expire (used by sim and fx)

	{
		GPPROFILERSAMPLE( "GoDb :: Update() - process expirations", SP_GODB );

		for ( ; ; )
		{
			ExpireDb::iterator i = m_ExpireDb.begin();
			if ( i == m_ExpireDb.end() )
			{
				break;
			}
			if ( i->first > worldtime )
			{
				break;
			}

			// copy and erase the entry - further actions may alter the db
			ExpireEntry entry = i->second;
			Goid goid = entry.m_Go->GetGoid();
			{
				RECURSE_LOCK_FOR_WRITE( ExpireDb );

				// erase main entry
				m_ExpireDb.erase( i );

				// kill reverse index entry
				ExpireReverseDb::iterator index = m_ExpireReverseDb.find( entry.m_Go );
				if_gpassert( index != m_ExpireReverseDb.end() )
				{
					m_ExpireReverseDb.erase( index );
				}
			}

			// only do this stuff if not a clone source because "expiration"
			// means "lru cache unloading" to the clone source system
			if ( !entry.m_Go->IsCloneSourceGo() )
			{
				// well it's not
				entry.m_Go->m_IsExpiring = false;

				// notify of impending doom
				WorldMessage( entry.m_Forced ? WE_EXPIRED_FORCED : WE_EXPIRED_AUTO, goid ).Send( MD_CC_CHILDREN );
			}

			// ok delete 'em. don't delete w/ children if we're a party tho!
			if ( entry.m_Go->HasParty() )
			{
				SMarkForDeletion( entry.m_Go, entry.m_Forced, entry.m_Forced );
			}
			else
			{
				SMarkGoAndChildrenForDeletion( entry.m_Go, entry.m_Forced, entry.m_Forced );
			}
		}
	}

// Process LRU clone sources.

	m_CloneSrcLruCheck -= deltaTime;
	if ( m_CloneSrcLruCheck < 0 )
	{
		GPPROFILERSAMPLE( "GoDb :: Update() - process LRU clone sources", SP_GODB );

		m_CloneSrcLruCheck = m_CloneSrcCheckTimeout;

		RwCritical::ReadLock locker( m_DbCritical );

		CloneSrcDb::iterator i, ibegin = m_CloneSrcDb.begin(), iend = m_CloneSrcDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// expire this fucker if it's there and not locked.
			Go* go = i->m_Go;
			if ( (go != NULL) && !go->IsExpiring() && (i->m_LockCount == 0) )
			{
				StartExpiration( go );
			}
		}
	}

	// update worldfx now before we commit deletions, so any dependent fx that
	// are waiting to start up can target them properly before the go's get
	// nuked.
	gWorldFx.Update( deltaTime, actualDeltaTime );

// Finish.

	// commit again to clean out any pending requests - also flush any pending rpc's
	CommitAllRequests( true );
}

void GoDb :: UpdateMouseShadow( void )
{
	// early-out if we're not supposed to highlight at all. can't have any
	// messages going around!
	if ( gWorldOptions.GetDisableHighlight() )
	{
		return;
	}

	using siege::SiegeMouseShadow::HitColl;

	// clear mouse shadow bits
	GopColl::const_iterator i, ibegin = m_MouseShadowColl.begin(), iend = m_MouseShadowColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)->ClearVisit();
	}

	// get hits - ignore them all if we're in an nis or highlights are otherwise
	// disabled...
	if ( gWorldState.GetCurrentState() != WS_SP_NIS )
	{
		const HitColl& hits = gSiegeEngine.GetMouseShadow().GetHitColl();
		HitColl::const_iterator j, jbegin = hits.begin(), jend = hits.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			GoHandle go( MakeGoid( *j ) );
			if ( go )
			{
				go->SetVisit();

				if ( !go->IsMouseShadowed() )
				{
					m_MouseShadowColl.push_back( go );
					WorldMessage( WE_MOUSEHOVER, go->GetGoid() ).Send();
				}
			}
		}
	}

	// remove from mouse shadow group unvisited go's
	GopColl::iterator k = m_MouseShadowColl.begin();
	while ( k != m_MouseShadowColl.end() )
	{
		Go* go = *k;
		if ( !go->IsVisited() )
		{
			k = m_MouseShadowColl.erase( k );
			WorldMessage( WE_UNMOUSEHOVER, go->GetGoid() ).Send();
		}
		else
		{
			++k;
		}
	}
}

#if !GP_RETAIL

void GoDb :: BuildNodeContentIndex( FuelHandle fuel ) const
{
	GlobalConstIter i, begin = GlobalBegin(), end = GlobalEnd();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->GetScid() != SCID_SPAWNED )
		{
			gpassert( IsInstance( i->GetScid() ) );
			fuel->Add( i->GetPlacement()->GetPosition().node.ToString(), MakeInt( i->GetScid() ), FVP_HEX_INT );
		}
	}
}

void GoDb :: SaveGo( Goid goid, FuelHandle fuel ) const
{
	gpassert( IsEditMode() );

	// get go, must be valid etc etc
	GoHandle go( goid );
	gpassert( go && IsInstance( go->GetScid() ) );

	// now tell the edit component to save the go out
	go->GetEdit()->SaveGo( fuel );

	// update fuel address in contentdb's index - it should already be there btw
	gpverify( !gWorldMap.SetScidIndex( go->GetScid(), fuel->GetAddress().c_str() ) );
}

void GoDb :: SaveAllGos( FuelHandle fuel ) const
{
	gpassert( IsEditMode() );

	// save all the global go's
	GlobalConstIter i, begin = GlobalBegin(), end = GlobalEnd();
	for ( i = begin ; i != end ; ++i )
	{
		// only save out non-spawned go's
		if ( i->GetScid() != SCID_SPAWNED )
		{
			SaveGo( i->GetGoid(), fuel );
		}
	}
}

#endif // !GP_RETAIL

Goid GoDb :: FindCloneSource( const char* templateName )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	// scope
	Goid goid = GOID_INVALID;
	{
		RwCritical::WriteLock writeLock( m_CloneNameCritical );

		// find by name
		CloneNameDb::iterator found = m_CloneNameDb.find( templateName );
		if ( found == m_CloneNameDb.end() )
		{
			// ok go for a create lock now, then reclaim the clone critical
			m_CloneNameCritical.LeaveWrite();
			Critical::Lock serializer( m_CreateLock );
			m_CloneNameCritical.EnterWrite();

			// someone else may have been creating this exact clone - try again
			found = m_CloneNameDb.find( templateName );
			if ( found == m_CloneNameDb.end() )
			{
				// only create a real go if we're not persisting
				if ( !IsLockedForSaveLoad() )
				{
					goid = CreateCloneSrcGo( GOID_INVALID, NULL, templateName );
				}
				else
				{
					// we're loading/saving, so just create a slot for it (it will
					// be filled in later)
					CloneBucket* bucket;
					UINT index, magic;
					m_CloneSrcDb.Alloc( bucket, index, magic );

					// get the goid
					goid = MakeCloneSrcGoid( index, magic );

					// prep this for on-demand fill
					bucket->m_Go = NULL;
					bucket->m_TemplateName = templateName;
					bucket->m_LockCount = 0;
					++m_CloneSrcUnloadedCount;

					// insert in index
					m_CloneNameDb[ bucket->m_TemplateName ] = goid;
					gpassert( m_CloneNameDb.size() == m_CloneSrcDb.GetUsedBucketCount() );
				}
			}
			else
			{
				goid = found->second;
			}
		}
		else
		{
			goid = found->second;
		}
	}

	// touch the clone source now that somebody wants it (but after the critical
	// has been released)
	if ( !IsLockedForSaveLoad() )
	{
		GoHandle go( goid );
	}

	// done
	return ( goid );
}

Goid GoDb :: FindAndLockCloneSource( const char* templateName, Go* owner )
{
	Goid goid = FindCloneSource( templateName );
	if ( goid != GOID_INVALID )
	{
		LockCloneSource( goid, owner );
	}
	return ( goid );
}

void GoDb :: LockCloneSource( Goid cloneSource, Go* owner )
{
	// ignore if obviously not supposed to be one
	if ( cloneSource == GOID_INVALID )
	{
		return;
	}

	gpassert( owner != NULL );
	gpassert( IsValid( cloneSource ) );
	gpassert( GetGoidClass( cloneSource ) == GO_CLASS_CLONE_SRC );
	gpassert( !owner->HasValidGoid() || !owner->IsCloneSourceGo() );	// ok to not have a goid yet if we're currently cloning a real one

	// add owner to the db
	{
		RwCritical::WriteLock writeLock( m_CloneSrcLockCritical );
		m_CloneSrcLockDb.insert( std::make_pair( owner, cloneSource ) );
	}

	// lock it automatically if owner is in the world
	if ( owner->IsInAnyWorldFrustum() )
	{
		LockCloneSources( owner, true );
	}
}

bool GoDb :: IsConstructingGo( void ) const
{
	bool is = m_Adder.IsActive();

	// if someone is asking in general, then they better be the owner if it's owned!
	gpassert( !is || (m_Adder.GetOwnerThreadId() == ::GetCurrentThreadId()) );

	return ( is );
}

bool GoDb :: IsCurrentThreadConstructingGo( void ) const
{
	return ( m_Adder.IsActive() && (m_Adder.GetOwnerThreadId() == ::GetCurrentThreadId()) );
}

bool GoDb :: IsConstructingStandaloneGo( void ) const
{
	return ( IsConstructingGo() && m_ConstructingStandaloneGo );
}

bool GoDb :: IsImportingStandaloneGo( void ) const
{
	return ( m_ImportingStandaloneGo );
}

Goid GoDb :: SGetMostRecentNewGoid( void )
{
	CHECK_SERVER_ONLY;

	Goid goid = GOID_INVALID;
	std::swap( goid, ::IsPrimaryThread() ? m_MostRecentNewGoidMain : m_MostRecentNewGoidLoader );
	return ( goid );
}

DWORD GoDb :: GetMostRecentFixedSeed( void )
{
	DWORD seed = 0;
	std::swap( seed, ::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader );
	return ( seed );
}

void GoDb :: PreloadCloneSource( Go* /*client*/, const char* templateName )
{
	if ( ::IsPrimaryThread() && ::IsInGame( gWorldState.GetCurrentState() ) )
	{
		// primary thread just issues requests
		gSiegeLoadMgr.LoadByName( LOADER_GO_PRELOAD_TYPE, templateName );
	}
	else
	{
		// we're already second thread - do it now
		FindCloneSource( templateName );
	}
}

Goid GoDb :: SCreateGo( const GoCreateReq& createReq )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	CHECK_SERVER_ONLY;

	Goid goid = GOID_INVALID;

	// special check to early abort objects that have dev templates compiled out
	// completely. this will speed things up as well as make it so that we
	// aren't constructing temporary go's only to delete them, which isn't
	// exactly perfect in multiplayer.
	FastFuelHandle fuel( gWorldMap.MakeObjectFuel( createReq.m_Scid, createReq.m_RegionId, createReq.m_StartingPos.node, true ) );
	if ( fuel )
	{
		if ( gContentDb.FindTemplateByName( fuel.GetType() ) != NULL )
		{
			// begin go construction and get next goid
			GoAdder::ServerLock locker;
			Goid initialGoid = m_Adder.AssignNextGoid();
			gpassert( initialGoid != GOID_INVALID );

			// get a seed for random generator for use on clients. if this is a
			// base go, then use the global rng, otherwise use go creation so
			// all clients are in sync.
			DWORD randomSeed = (GlobalGoidBits( initialGoid ).m_MinorIndex == 0) ? ::RandomDword() : GetGoCreateRng().RandomDword();

			// calc snap and node pos
			GoCreateReq localCreateReq( createReq );
			localCreateReq.CalcPosition();

			// create it locally
			RCCreateGo( RPC_TO_SERVER, initialGoid, randomSeed, localCreateReq );
			goid = SGetMostRecentNewGoid();
		}
#		if !GP_RETAIL
		else if ( !gWorldOptions.GetForceRetailContent() )
		{
			gperrorf(( "Game object at '%s' (scid = 0x%08X) instantiates unregistered template '%s'\n",
					   fuel.GetAddress().c_str(), createReq.m_Scid, fuel.GetType() ));
		}
#		endif // !GP_RETAIL
	}

	// return what we got
	return ( goid );
}

FuBiCookie GoDb :: RCCreateGo(
		DWORD machineId, Goid startGoid, DWORD randomSeed,
		const GoCreateReq& createReq, bool standalone, const_mem_ptr data )
{
	MachineAddrQuery query( machineId );

	// tell players to create it
	if ( query.m_RemotePlayers[ 0 ] != NULL )
	{
		CreateGoXfer xfer;
		xfer.m_CreateReq  = createReq;
		xfer.m_StartGoid  = startGoid;
		xfer.m_RandomSeed = randomSeed;
		xfer.m_Standalone = standalone;
		xfer.m_Data       = data;

		gplog( OutputGoCreatePacker( xfer, true ) );

		const Player** i, ** ibegin = query.m_RemotePlayers, ** iend = ARRAY_END( query.m_RemotePlayers );
		for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
		{
			FuBi::BitPacker packer;
			xfer.Xfer( packer, m_CreateGoXferCache[ ::MakeIndex( (*i)->GetId() ) - 1 ] );

			FUBI_SET_RPC_PEEKADDRESS( (*i)->GetMachineId() );
			RCCreateGoPacker( packer.GetSavedBits() );
		}
	}

	// optionally create locally on server
	FuBiCookie rc = RPC_SUCCESS;
	if ( query.m_ForLocal && !RCCreateGoFinal( startGoid, randomSeed, createReq, standalone, data, 0, false ) )
	{
		rc = RPC_FAILURE_IGNORE;
	}

	// check it
#	if !GP_RETAIL
	if ( rc == RPC_SUCCESS )
	{
		RCVerifyGoBucket( query, startGoid, standalone );
	}
#	endif // !GP_RETAIL

	return ( rc );
}

FuBiCookie GoDb :: RCCreateGoPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_RETRY_PEEKADDRESS( RCCreateGoPacker );
	int packetSize = FUBI_TAKE_RPC_PACKET_SIZE;

	CHECK_PRIMARY_THREAD_ONLY;
	gpassert( !::IsServerLocal() );

	CreateGoXfer xfer;

	Player* player = gServer.GetHumanPlayerOnMachine( RPC_TO_LOCAL );
	if ( player == NULL )
	{
		// $ hack for #12654 until we figure out why local player no exist yet
		//   this function still being called...
		gperror( "You just repro'd #12654!! Tried to look up local player for "
				 "a create request and nobody there...\n" );
		return ( RPC_FAILURE_IGNORE );
	}

	PlayerId playerId = player->GetId();
	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer, m_CreateGoXferCache[ ::MakeIndex( playerId ) - 1 ] );

	gplog( OutputGoCreatePacker( xfer, false ) );

	return (  RCCreateGoFinal(
				xfer.m_StartGoid, xfer.m_RandomSeed, xfer.m_CreateReq,
				xfer.m_Standalone, xfer.m_Data, packetSize, FUBI_IN_RPC_DISPATCH )
			? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoDb :: RCCreateGoFinal(
		Goid startGoid, DWORD randomSeed, const GoCreateReq& createReq,
		bool standalone, const_mem_ptr data, int packetSize, bool fromRpc )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	gpgolifef(( "GoDb::RCCreateGoFinal( scid = '0x%08X' )\n", createReq.m_Scid ));

	// we may have been called from an rpc - commit it
	if ( fromRpc && ::IsPrimaryThread() )
	{
		CommitDeleteRequests();
	}

	// set up locker
	GoAdder::ClientLock locker( startGoid );

	// use the random seed
	GetGoCreateRng().SetSeed( randomSeed );

	// cannot be already under construction
	gpassert( m_GoCreateColl.empty() );

	// create the go
	Goid goid = CreateGo( createReq, standalone );

	// on failure, free up any allocated go's
	if ( goid == GOID_INVALID )
	{
		DeleteFailedGo( startGoid );
	}
	else if ( data.mem && data.size )
	{
		// sync
		GoHandle newGo( goid );
		if_gpassert( newGo )
		{
			newGo->RpcSyncData( data );
		}
	}

	// update stats
#	if !GP_RETAIL
	GoHandle newGo( goid );
	if ( newGo )
	{
		newGo->SetRpcCreateBytes( packetSize );
	}
#	endif // !GP_RETAIL

	// clear the construction list, not needed any more
	m_GoCreateColl.clear();

	// remember the original requested go
	(::IsPrimaryThread() ? m_MostRecentNewGoidMain : m_MostRecentNewGoidLoader) = goid;

	// success/failure
	return ( goid != GOID_INVALID );
}

Goid GoDb :: CreateGo( const GoCreateReq& createReq, bool standalone )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	LOADER_OR_RPC_ONLY;

	Goid goid = GOID_INVALID;

	if ( !IsConstructingStandaloneGo() )
	{
		(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = GetGoCreateRng().GetSeed();

		m_ConstructingStandaloneGo = standalone;
		goid = CreateOrLoadGo( &createReq, NULL );
		m_ConstructingStandaloneGo = false;
	}

	return ( goid );
}

Goid GoDb :: SLoadGo( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	CHECK_SERVER_ONLY;

	gpassert( gWorldMap.ContainsRegion( regionId ) );
	gpgolifef(( "GoDb::SLoadGo( scid = '0x%08X', region = '%s' )\n", scid, gWorldMap.GetRegionName( regionId ).c_str() ));

	// check to see if it's already been loaded
	bool wasRetired = false;
	Goid goid = FindGoidByScid( scid, &wasRetired );
	if ( (goid == GOID_INVALID) && !wasRetired )
	{
		// create it (reuse startingpos.node for error reporting)
		GoCreateReq createReq( scid, regionId, PLAYERID_COMPUTER );
		createReq.m_StartingPos.node = forNodeGuid;
		goid = SCreateGo( createReq );
	}

	return ( goid );
}

Goid GoDb :: LoadLocalGo( Scid scid, RegionId regionId, const SiegeGuid& forNodeGuid, bool lodfi, bool fadeIn )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	gpgolifef(( "GoDb::LoadLocalGo( scid = '0x%08X', region = '%s', node = %s )\n",
				scid, gWorldMap.GetRegionName( regionId ).c_str(), forNodeGuid.ToString().c_str() ));

	// acquire creation lock
	GoAdder::Lock locker( GO_CLASS_LOCAL );

	// check to see if it's already been loaded
	bool wasRetired = false;
	Goid goid = FindGoidByScid( scid, &wasRetired );
	if ( (goid == GOID_INVALID) && !wasRetired )
	{
		// these calls may not be nested
		gpassert( gWorldMap.ContainsRegion( regionId ) );

		// build create req (reuse startingpos.node for error reporting)
		GoCreateReq createReq( scid, regionId, PLAYERID_COMPUTER );
		createReq.m_StartingPos.node = forNodeGuid;
		createReq.m_LocalGo = true;
		createReq.m_LodfiGo = lodfi;
		createReq.m_FadeIn = fadeIn;

		// create new local go
		(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = GetGoCreateRng().GetSeed();
		goid = CreateOrLoadGo( &createReq, NULL );
		if ( goid == GOID_INVALID )
		{
			DeleteFailedGo( m_Adder.GetCurrentGoid() );
		}
	}

	// wait a second...local go's aren't supposed to get retired anyway!
	gpassert( !wasRetired );

	// all done
	return ( goid );
}

void GoDb :: SetAsAllClientsGo( Go* go, bool set )
{
	gpassert( go != NULL );
	if ( go->IsAllClients() != set )
	{
		if ( IsServerLocal() && set )
		{
			ClientSyncGos( go->GetWorldFrustumMembership(), (FrustumId)0xFFFFFFFF, go, NULL );
		}

		go->m_IsAllClients = set;

		if ( IsServerLocal() && !set )
		{
			ClientSyncGos( (FrustumId)0xFFFFFFFF, go->GetWorldFrustumMembership(), go, NULL );
		}
	}

	GopColl::iterator i, ibegin = go->GetChildBegin(), iend = go->GetChildEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		SetAsAllClientsGo( *i, set );
	}
}

void GoDb :: SwapScids( Go* a, Go* b )
{
	gpassert( IsPointerValid( a ) && IsPointerValid( b ) );

	if ( a->GetScid() != b->GetScid() )
	{
		RwCritical::WriteLock writeLock( m_ScidCritical );

		// replace it in the scid index
		if ( !a->IsSpawned() && !b->IsSpawned() )
		{
			// simple swap
			ScidIndex::iterator afound = m_ScidIndex.find( a->GetScid() );
			gpassert( (afound != m_ScidIndex.end()) && (afound->second == a) );
			ScidIndex::iterator bfound = m_ScidIndex.find( b->GetScid() );
			gpassert( (bfound != m_ScidIndex.end()) && (bfound->second == b) );
			std::swap( afound->second, bfound->second );
		}
		else
		{
			// make 'a' the instanced scid
			gpassert( a->IsSpawned() != b->IsSpawned() );
			if ( a->IsSpawned() )
			{
				std::swap( a, b );
			}

			// one man enters, one man leaves
			ScidIndex::iterator found = m_ScidIndex.find( a->GetScid() );
			gpassert( (found != m_ScidIndex.end()) && (found->second == a) );
			found->second = b;
		}

		// swap scids
		std::swap( a->m_Scid, b->m_Scid );
	}
}

FuBiCookie GoDb :: RSCloneGo( const GoCloneReq& cloneReq )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	const char* templateName = NULL;

	DWORD goidClass = GetGoidClass( cloneReq.m_CloneSource );
	if ( (goidClass == GO_CLASS_CLONE_SRC) || (goidClass == GO_CLASS_LOCAL) )
	{
		templateName = GoHandle( cloneReq.m_CloneSource )->GetTemplateName();
	}

	return ( RSCloneGo( cloneReq, templateName ) );
}

FuBiCookie GoDb :: RSCloneGo( const GoCloneReq& cloneReq, const char* templateName )
{
	FUBI_RPC_CALL_RETRY( RSCloneGo, RPC_TO_SERVER );

	GPSTATS_SYSTEM( SP_LOAD_GO );

	Goid goid = SCloneGo( cloneReq, templateName );
	return ( (goid == GOID_INVALID) ? RPC_FAILURE : RPC_SUCCESS );
}

Goid GoDb :: SCloneGo ( const GoCloneReq& cloneReq, const char* templateName, const DWORD* forceRandomSeed )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	CHECK_SERVER_ONLY;

	Goid goid = GOID_INVALID;

	{
		// begin go construction and get next goid
		GoAdder::ServerLock locker;
		Goid initialGoid = m_Adder.AssignNextGoid();
		gpassert( initialGoid != GOID_INVALID );

		// get a seed for random generator for use on clients. if this is a base
		// go, then use the global rng, otherwise use go creation so all clients
		// are in sync.
		DWORD randomSeed = forceRandomSeed ? *forceRandomSeed : 
						   ((GlobalGoidBits( initialGoid ).m_MinorIndex == 0) ? ::RandomDword() : GetGoCreateRng().RandomDword());

		// calc snap and node pos
		GoCloneReq localCloneReq( cloneReq );
		localCloneReq.CalcPosition();

		// match all-clients state if there's a parent
		if ( localCloneReq.m_Parent != GOID_INVALID )
		{
			localCloneReq.m_AllClients = GoHandle( localCloneReq.m_Parent )->IsAllClients();
		}

		// see if we should create this on all machines
		bool calcAllClients = (localCloneReq.m_AllClients || localCloneReq.m_OmniGo) && !localCloneReq.m_ForceServerOnly;

		// create it
		RCCloneGo( calcAllClients ? RPC_TO_ALL : RPC_TO_SERVER, initialGoid, randomSeed, localCloneReq, templateName );
		goid = SGetMostRecentNewGoid();
	}

	// post-xfer requested?
	if ( cloneReq.m_XferCharacterPost )
	{
		GoHandle go( goid );
		if ( go )
		{
			go->XferCharacterPost();
		}
	}

	// return what we got
	return ( goid );
}

Goid GoDb :: SCloneGo( const GoCloneReq& cloneReq, const char* templateName )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	CHECK_SERVER_ONLY;

	return ( SCloneGo( cloneReq, templateName, NULL ) );
}

Goid GoDb :: SCloneGo( const GoCloneReq& cloneReq )
{
	return ( SCloneGo( cloneReq, NULL, NULL ) );
}

Goid GoDb :: CloneGo( const GoCloneReq& cloneReq, const char* templateName, bool standalone )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	// $ early bailout on plain template name
	if ( templateName == NULL )
	{
		return ( CloneGo( cloneReq, standalone, false ) );
	}

	// $ early bailout if standalone
	if ( IsConstructingStandaloneGo() )
	{
		gpassert( !standalone );
		return ( GOID_INVALID );
	}

	gpgolifef(( "GoDb::CloneGo( template = '%s' )\n", templateName ));
	const char* originalQuery = templateName;

	enum eType
	{
		TYPE_NORMAL,
		TYPE_PCONTENT,
		TYPE_FUEL,
		TYPE_IMPORT_BINARY,
		TYPE_IMPORT_TEXT,
	};

	PContentReq pcontentReq;
	FastFuelHandle templateFastFuel;
	FuelHandle templateFuel;
	gpstring templateNameStr;
	eType type = TYPE_NORMAL;
	Goid goid = GOID_INVALID;
	DWORD randomSeed = GetGoCreateRng().GetSeed();

	Critical::Lock serializer( m_CreateLock );

	if ( *templateName == '#' )
	{
		// $ parameterized content creation - cloning a virtual go

		// right, now do the query
		const GoDataTemplate* found = gPContentDb.FindTemplateByPContent( pcontentReq, templateName + 1 );
		if ( found != NULL )
		{
			templateName = found->GetName();
			type = TYPE_PCONTENT;

			if ( pcontentReq.IsForcingRandomSeed() )
			{
				randomSeed = pcontentReq.GetForcedRandomSeed();
				GetGoCreateRng().SetSeed( randomSeed );
			}
		}
		else
		{
			gpwarningf(( "CloneGo: pcontent query failed with request '%s'\n", templateName ));
			templateName = NULL;
		}
	}
	else if ( *templateName == ':' )
	{
		// $ build go manually from a given fuel block

		// it's actually a fuel address
		templateFuel = FuelHandle( templateName );
		if ( templateFuel && templateFuel->Get( "template_name", templateNameStr ) && !templateNameStr.empty() )
		{
			// imported content creation - cloning a virtual go
			templateName = templateNameStr;
			type = TYPE_IMPORT_TEXT;
		}
		else if ( templateFastFuel = FastFuelHandle( templateName ), templateFastFuel )
		{
			// see which kind - if it has a type, then it's a standard object,
			// otherwise it's an import
			if ( templateFastFuel.HasType() )
			{
				// direct from fuel block creation
				type = TYPE_FUEL;
			}
			else if ( templateFastFuel.Get( "template_name", templateNameStr ) && !templateNameStr.empty() )
			{
				// imported content creation - cloning a virtual go
				templateName = templateNameStr;
				type = TYPE_IMPORT_BINARY;
			}
		}
		else
		{
			gperrorf(( "CloneGo: unable to resolve fuel address for Go creation, request is '%s'\n", templateName ));
			templateName = NULL;
		}
	}

	// restore original rng seed that may have been messed up by pcontent queries
	(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = randomSeed;

	// if fuel, do a creation
	if ( type == TYPE_FUEL )
	{
		goid = CreateOrLoadGo( NULL, &cloneReq, templateFastFuel );
	}
	else if ( templateName != NULL )
	{
		// ordinary cloning - copy the clone source
		GoCloneReq localCloneReq( cloneReq );
		localCloneReq.m_CloneSource = FindCloneSource( templateName );
		if ( localCloneReq.m_CloneSource != GOID_INVALID )
		{
			// standalone if we're importing
			if ( (type == TYPE_IMPORT_TEXT) || (type == TYPE_IMPORT_BINARY) )
			{
				gpassert( !standalone );
				standalone = true;

				gpassert( !m_ImportingStandaloneGo );
				m_ImportingStandaloneGo = true;
			}

			// clone it
			goid = CloneGo( localCloneReq, standalone, type == TYPE_PCONTENT, true );

			// done with this always
			m_ImportingStandaloneGo = false;

			// post mod for virtual templates
			GoHandle go( goid );
			if ( go )
			{
				if ( type == TYPE_PCONTENT )
				{
					gPContentDb.ApplyPContentRequest( go, pcontentReq );
					GPDEV_ONLY( go->m_DevInstanceFuel = originalQuery );
				}
				else if ( type == TYPE_IMPORT_TEXT )
				{
					FuBi::FuelReader reader( templateFuel );
					FuBi::PersistContext persist( &reader, false );
					go->XferCharacter( persist );
				}
				else if ( type == TYPE_IMPORT_BINARY )
				{
					FuBi::FastFuelReader reader( templateFastFuel );
					FuBi::PersistContext persist( &reader, false );
					go->XferCharacter( persist );
				}

				gplog( OutputGoCreation( go ) );
			}
		}
	}

	return ( goid );
}

Goid GoDb :: CloneGo( const GoCloneReq& cloneReq, bool standalone, bool isPContent, bool calledFromClone )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	LOADER_OR_RPC_ONLY;

	// $ early bailout if standalone
	if ( IsConstructingStandaloneGo() )
	{
		gpassert( !standalone );
		return ( GOID_INVALID );
	}

	// verify properly formed request
	gpassert( cloneReq.AssertValid() );
	gpgolifef(( "GoDb::CloneGo( cloneSource = '%s' )\n", GoHandle( cloneReq.m_CloneSource )->GetTemplateName() ));

	// get the clone source
	GoHandle go( cloneReq.m_CloneSource );
	if ( !go )
	{
		gperrorf(( "SYNC ERROR: attempted to clone a Go with a clone source (goid = 0x%08X) that does not exist!\n", cloneReq.m_CloneSource ));
		return ( GOID_INVALID );
	}

	// get player - defaults to parent's then clone source's player
	Player* player = cloneReq.GetPlayer( go );
	if_not_gpassert( player != NULL )
	{
		gperrorf(( "SYNC ERROR: attempted to clone a Go with a player (id = 0x%08X) that does not exist!\n", cloneReq.m_PlayerId ));
		return ( GOID_INVALID );
	}

	// acquire creation lock
	GoAdder::Lock locker( GO_CLASS_GLOBAL );

	// check to make sure that we're not in the middle of handling errors at a higher level
	if ( m_Adder.GetCurrentGoid() == GOID_INVALID )
	{
		return ( GOID_INVALID );
	}

	// take flag
	m_ConstructingStandaloneGo = standalone;

	// get the next goid
	Goid goid = m_Adder.AssignNextGoid();
	if ( goid == GOID_INVALID )
	{
		return ( GOID_INVALID );
	}

	// if pcontent, then it's already set up right
	if ( !isPContent )
	{
		(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = GetGoCreateRng().GetSeed();
	}

	// create cloned go
	std::auto_ptr <Go> cloneGo( new Go( *go, player, false ) );

	// optionally set omni flag
	if ( cloneReq.m_OmniGo )
	{
		cloneGo->SetIsOmni();
	}

	// maybe an all clients go too
	cloneGo->m_IsAllClients = cloneReq.m_AllClients;

	// add to the creation collection
	m_GoCreateColl.push_back( cloneGo.get() );

	// add to temp db
	AddToTempOnDeckDb( cloneGo.get(), goid );

	// commit the go with new goid
	bool success = cloneGo->CommitCreation( goid, cloneReq.GetMpPlayerCount() );
	GPDEBUG_ONLY( cloneGo->m_DbgInGoDb = success; )

	// finish the request
	cloneReq.Finish( cloneGo.get() );

	// set parent/child relationship
	if ( success && (cloneReq.m_Parent != GOID_INVALID) )
	{
		GoHandle parent( cloneReq.m_Parent );
		if ( parent )
		{
			parent->AddChild( cloneGo.get() );
		}
		else if ( !WasGoidRecentlyDeleted( cloneReq.m_Parent ) )
		{
			gperrorf(( "SYNC ERROR: attempted to add newly cloned Go with a parent that does not exist!\n"
					   "\n"
					   "\tCloned go: goid = 0x%08X, scid = 0x%08X, template = '%s'\n"
					   "\tParent go: goid = 0x%08X\n",
					   cloneGo->GetGoid(), cloneGo->GetScid(), cloneGo->GetTemplateName(),
					   cloneReq.m_Parent ));
		}
	}

	// update its gold value on success - use pcontent rules for this
	if ( success && !isPContent )
	{
		gPContentDb.CalcAndApplyPContentRules( cloneGo.get() );
	}

	// if it's supposed to draw right away, set the flag
	if ( cloneReq.m_PrepareToDrawNow )
	{
		cloneGo->PrepareToDrawNow();
	}

	// post-finish the request
	cloneReq.FinishPost( cloneGo.get() );

	// record
	if ( !calledFromClone )
	{
		gplog( OutputGoCreation( cloneGo.get() ) );
	}

	// add it to the on deck set and let it go (even if failed to commit)
	AddToOnDeckDb( cloneGo.release(), goid );

	// release flag
	m_ConstructingStandaloneGo = false;

	// all done - if failed, note that caller will destroy the bad go
	return ( success ? goid : GOID_INVALID );
}

bool GoDb :: CloneGo( Goid explicitGoid, const GoCloneReq& cloneReq, const char* templateName )
{
	return ( RCCloneGo( RPC_TO_LOCAL, explicitGoid, 0, cloneReq, templateName, true ) == RPC_SUCCESS );
}

FuBiCookie GoDb :: RCCloneGo(
		DWORD machineId, Goid startGoid, DWORD randomSeed,
		const GoCloneReq& cloneReq, const char* templateName,
		bool standalone, const_mem_ptr data )
{
	MachineAddrQuery query( machineId );

	// tell players to clone it
	if ( query.m_RemotePlayers[ 0 ] != NULL )
	{
		// local xfer for packing
		CloneGoXfer xfer;
		xfer.m_CloneReq     = cloneReq;
		xfer.m_StartGoid    = startGoid;
		xfer.m_RandomSeed   = randomSeed;
		xfer.m_TemplateName = templateName;
		xfer.m_Standalone   = standalone;
		xfer.m_Data         = data;

		// some vars we may need when deciding how to handle for each player
		PlayerId    newPlayerId     = PLAYERID_INVALID;
		const char* newTemplateName = NULL;
		FrustumId   parentFrustum   = FRUSTUMID_INVALID;
		FrustumId   cloneFrustum    = FRUSTUMID_INVALID;

		// check to see if we have to clear out the parent
		if ( xfer.m_CloneReq.m_Parent != GOID_INVALID )
		{
			GoHandle parent( xfer.m_CloneReq.m_Parent );
			if ( !parent->IsRpcCapableGo() )
			{
				// can't use this parent no matter what
				xfer.m_CloneReq.m_Parent = GOID_INVALID;
			}
			else
			{
				parentFrustum = parent->IsAllClients() ? MakeFrustumId( 0xFFFFFFFF ) : parent->CalcWorldFrustumMembership( false );
			}

			// get a valid parentid, because we can't rely on the
			// fallback/default mechanism on the other side now that parent is
			// cleared out
			GoHandle cloneSource( xfer.m_CloneReq.m_CloneSource );
			Player* clonePlayer = xfer.m_CloneReq.GetPlayer( cloneSource );
			newPlayerId = clonePlayer->GetId();
		}

		// check to see if we have to promote clone source to string right now
		if ( xfer.m_TemplateName == NULL )
		{
			GoHandle cloneSource( xfer.m_CloneReq.m_CloneSource );
			if ( !cloneSource->IsRpcCapableGo() )
			{
				// we have to use named no matter what
				xfer.m_TemplateName = cloneSource->GetTemplateName();
				xfer.m_CloneReq.m_CloneSource = GOID_INVALID;
			}
			else
			{
				cloneFrustum = cloneSource->IsAllClients() ? MakeFrustumId( 0xFFFFFFFF ) : cloneSource->CalcWorldFrustumMembership( false );
			}

			// always grab a fallback template name for below
			newTemplateName = cloneSource->GetTemplateName();
		}
		else
		{
			// if we're using a template name, then clear this to save net bw
			xfer.m_CloneReq.m_CloneSource = GOID_INVALID;
		}

		gplog( OutputGoClonePacker( xfer, true ) );

		// remember original values
		Goid        originalParent       = xfer.m_CloneReq.m_Parent;
		PlayerId    originalPlayerId     = xfer.m_CloneReq.m_PlayerId;
		const char* originalTemplateName = xfer.m_TemplateName;

		// distribute to players in our set
		const Player** i, ** ibegin = query.m_RemotePlayers, ** iend = ARRAY_END( query.m_RemotePlayers );
		for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
		{
			const Player* player = *i;
			FrustumId playerFrustum = GetPlayerFrustumMask( player->GetId() );

			// check frustum intersection for parent
			if ( originalParent != GOID_INVALID )
			{
				if ( MakeInt( playerFrustum ) & MakeInt( parentFrustum ) )
				{
					xfer.m_CloneReq.m_Parent   = originalParent;
					xfer.m_CloneReq.m_PlayerId = originalPlayerId;
				}
				else
				{
					xfer.m_CloneReq.m_Parent   = GOID_INVALID;
					xfer.m_CloneReq.m_PlayerId = newPlayerId;
				}
			}

			// check frustum intersection for clone source
			if ( originalTemplateName == NULL )
			{
				if ( MakeInt( playerFrustum ) & MakeInt( cloneFrustum ) )
				{
					xfer.m_TemplateName           = NULL;
					xfer.m_CloneReq.m_CloneSource = cloneReq.m_CloneSource;
				}
				else
				{
					xfer.m_TemplateName           = newTemplateName;
					xfer.m_CloneReq.m_CloneSource = GOID_INVALID;
				}
			}

			FuBi::BitPacker packer;
			xfer.Xfer( packer, m_CloneGoXferCache[ ::MakeIndex( (*i)->GetId() ) - 1 ] );

			FUBI_SET_RPC_PEEKADDRESS( (*i)->GetMachineId() );
			RCCloneGoPacker( packer.GetSavedBits() );
		}
	}

	// optionally create locally on server
	FuBiCookie rc = RPC_SUCCESS;
	if ( query.m_ForLocal && !RCCloneGoFinal( startGoid, randomSeed, cloneReq, templateName, standalone, data, 0, false ) )
	{
		rc = RPC_FAILURE_IGNORE;
	}

	// check it
#	if !GP_RETAIL
	if ( rc == RPC_SUCCESS )
	{
		RCVerifyGoBucket( query, startGoid, standalone );
	}
#	endif // !GP_RETAIL

	return ( rc );
}

FuBiCookie GoDb :: RCCloneGoPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_RETRY_PEEKADDRESS( RCCloneGoPacker );
	int packetSize = FUBI_TAKE_RPC_PACKET_SIZE;

	CHECK_PRIMARY_THREAD_ONLY;

	CloneGoXfer xfer;

	Player* player = gServer.GetHumanPlayerOnMachine( RPC_TO_LOCAL );
	if ( player == NULL )
	{
		// $ hack for #12654 until we figure out why local player no exist yet
		//   this function still being called...
		gperror( "You just repro'd #12654!! Tried to look up local player for "
				 "a create request and nobody there...\n" );
		return ( RPC_FAILURE_IGNORE );
	}

	PlayerId playerId = player->GetId();
	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer, m_CloneGoXferCache[ ::MakeIndex( playerId ) - 1 ] );

	gplog( OutputGoClonePacker( xfer, false ) );

	return (  RCCloneGoFinal(
				xfer.m_StartGoid, xfer.m_RandomSeed, xfer.m_CloneReq,
				xfer.m_TemplateName, xfer.m_Standalone, xfer.m_Data, packetSize,
				FUBI_IN_RPC_DISPATCH )
			? RPC_SUCCESS : RPC_FAILURE_IGNORE );
}

bool GoDb :: RCCloneGoFinal( Goid startGoid, DWORD randomSeed, const GoCloneReq& cloneReq, const char* templateName, bool standalone, const_mem_ptr data, int packetSize, bool fromRpc )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	// we may have been called from an rpc - commit it
	if ( fromRpc && ::IsPrimaryThread() )
	{
		CommitDeleteRequests();
	}

	// set up locker
	Critical::Lock serializer( m_CreateLock );

	// use the random seed
	GetGoCreateRng().SetSeed( randomSeed );

	// cannot be already under construction
	gpassert( m_GoCreateColl.empty() );

	// clone the go
	Goid goid = GOID_INVALID;
	{
		GoAdder::ClientLock locker( startGoid );
		goid = CloneGo( cloneReq, templateName, standalone );
	}

	// on failure, free up any allocated go's
	if ( goid == GOID_INVALID )
	{
		DeleteFailedGo( startGoid );
	}
	else if ( data.mem && data.size )
	{
		// sync
		GoHandle newGo( goid );
		if_gpassert( newGo )
		{
			newGo->RpcSyncData( data );
		}
	}

	// update stats
#	if !GP_RETAIL
	GoHandle newGo( goid );
	if ( newGo )
	{
		newGo->SetRpcCreateBytes( packetSize );
	}
#	endif // !GP_RETAIL

	// clear the construction list, not needed any more
	m_GoCreateColl.clear();

	// remember the original requested go
	(::IsPrimaryThread() ? m_MostRecentNewGoidMain : m_MostRecentNewGoidLoader) = goid;

	// post-xfer requested?
	if ( fromRpc && cloneReq.m_XferCharacterPost )
	{
		GoHandle go( goid );
		if ( go )
		{
			go->XferCharacterPost();
		}
	}
	else
	{
		// special: if it's an actor, it may have been summoned, in which case
		// we need to calc its modifiers right now in case eric applies any
		// buffing enchantments right away (like with the killer gremal).
		GoHandle go( goid );
		if ( go && go->IsActor() && !go->IsAllClients() )
		{
			go->PrivateRecalcModifiers();
		}
	}

	// success/failure
	return ( goid != GOID_INVALID );
}

void GoDb :: RCSyncGo( Player* forPlayer, Go* go, bool standalone )
{
	// generate sync data
	void* freeThisPtr = NULL;
	const_mem_ptr sync = go->CreateRpcSyncData( standalone ? Go::SYNC_FULL : Go::SYNC_MINIMAL, freeThisPtr );

	// if no effects are running on server go, then they should not run on
	// client upon entering the world/construction etc.
	bool noStartupFx
			=    !HasRegisteredEffectScripts( go )
			  && !go->CalcAllClients();

	// create new go - check the region because a migrated go who has had
	// his scid replaced by expiration will have no region id.
	if ( go->IsSpawned() || (go->GetRegionSource() == REGIONID_INVALID) )
	{
		// build our clone request
		GoCloneReq cloneReq;
		cloneReq.m_PlayerId           = go->GetPlayer()->GetId();
		cloneReq.m_MpPlayerCount      = go->GetMpPlayerCount();
		cloneReq.m_OmniGo             = go->IsOmni();
		cloneReq.m_AllClients         = go->IsAllClients();
		cloneReq.m_ForceServerOnly    = go->IsServerOnly();
		cloneReq.m_ForceClientAllowed = !go->IsServerOnly();
		cloneReq.m_PrepareAmmo        = go->m_IsPreparedAmmo;
		cloneReq.m_NoStartupFx        = noStartupFx;

		// copy placement if necessary
		if ( !go->IsOmni() )
		{
			GoPlacement* placement = go->QueryPlacement();
			if ( (placement != NULL) && !placement->DoesInheritPlacement() )
			{
				cloneReq.SetStartingPos( placement->GetPosition() );
				cloneReq.SetStartingOrient( placement->GetOrientation() );
			}
		}

		// optionally set parent
		if ( go->HasParent() )
		{
			cloneReq.m_Parent = go->GetParent()->GetGoid();
		}

		// optionally set clone source
		Go* cloneSource = go->GetCloneSource();
		const char* templateName = NULL;
		if ( cloneSource != NULL )
		{
			cloneReq.m_CloneSource = cloneSource->GetGoid();
		}
		else
		{
			templateName = go->GetTemplateName();
		}

		// create the gold
		RCCloneGo( forPlayer->GetMachineId(), go->GetGoid(), go->GetRandomSeed(), cloneReq, templateName, standalone, sync );
	}
	else
	{
		GoCreateReq createReq( go->GetScid(), go->GetRegionSource(), go->GetPlayer()->GetId() );
		createReq.m_MpPlayerCount = go->GetMpPlayerCount();
		createReq.m_NoStartupFx   = noStartupFx;

		GoPlacement* placement = go->QueryPlacement();
		if ( (placement != NULL) && placement->GetIsPlacementDirty() && !placement->DoesInheritPlacement() )
		{
			createReq.SetStartingPos( placement->GetPosition() );
			createReq.SetStartingOrient( placement->GetOrientation() );
		}

		RCCreateGo( forPlayer->GetMachineId(), go->GetGoid(), go->GetRandomSeed(), createReq, standalone, sync );
	}

	// done with it
	::free( freeThisPtr );
}

Goid GoDb :: CloneLocalGo( const GoCloneReq& cloneReq )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	Critical::Lock serializer( m_CreateLock );

	// verify properly formed request
	gpassert( cloneReq.AssertValid() );
	gpassert( !cloneReq.m_AllClients );
	gpassert( !cloneReq.m_ForceClientAllowed );

	// get the clone source
	GoHandle go( cloneReq.m_CloneSource );
	if ( !go )
	{
		return ( GOID_INVALID );
	}

	// acquire creation lock
	GoAdder::Lock locker( GO_CLASS_LOCAL );

	// get player - defaults to parent's then screen player
	Player* player = cloneReq.GetPlayer( go );
	if_not_gpassert( player != NULL )
	{
		return ( GOID_INVALID );
	}

	// calc snap and node pos
	GoCloneReq localCloneReq( cloneReq );
	localCloneReq.CalcPosition();

	// create cloned go
	(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = GetGoCreateRng().GetSeed();
	std::auto_ptr <Go> cloneGo( new Go( *go, player, true ) );

	// optionally set omni flag
	if ( localCloneReq.m_OmniGo )
	{
		cloneGo->SetIsOmni();
	}

	// set parent/child relationship
	if ( localCloneReq.m_Parent != GOID_INVALID )
	{
		GoHandle( localCloneReq.m_Parent )->AddChild( cloneGo.get() );
	}

	// get the next goid
	Goid goid = m_Adder.AssignNextGoid();
	gpassert( goid != GOID_INVALID );

	// add to temp db
	AddToTempOnDeckDb( cloneGo.get(), goid );

	// commit the go with new goid
	bool success = cloneGo->CommitCreation( goid );
	if ( !success )
	{
		// kill it on failure
		cloneGo->m_IsCommittingCreation = false;
		DeleteGo( cloneGo.release() );
		return ( GOID_INVALID );
	}

	// it's ready
	GPDEBUG_ONLY( cloneGo->m_DbgInGoDb = true; )

	// finish the request
	localCloneReq.Finish( cloneGo.get() );

	// if it's supposed to draw right away, set the flag
	if ( cloneReq.m_PrepareToDrawNow )
	{
		cloneGo->PrepareToDrawNow();
	}

	// post-finish the request
	localCloneReq.FinishPost( cloneGo.get() );

	// record
	gplog( OutputGoCreation( cloneGo.get() ) );

	// add it to the on deck set and let it go
	AddToOnDeckDb( cloneGo.release(), goid );

	// all done
	return ( goid );
}

#if !GP_RETAIL

Goid GoDb :: CloneNonSpawnedGo( const GoCloneReq& cloneReq, Scid newScid, FastFuelHandle fuel )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	Critical::Lock serializer( m_CreateLock );

	// make sure it's not colliding with something already in our index
	gpassert( !HasGoWithScid( newScid ) );

	// clone it
	const char* templateName = NULL;
	gpstring localName;
	if ( fuel )
	{
		localName = fuel.GetAddress( true );
		templateName = localName;
	}
	Goid goid = SCloneGo( cloneReq, templateName );

	// now give it the scid
	GoHandle go( goid );
	if ( go )
	{
		// reassign
		gpassert( go->GetScid() == SCID_SPAWNED );
		go->m_Scid = newScid;

		// add to index
		{
			RwCritical::WriteLock writeLock( m_ScidCritical );
			m_ScidIndex[ newScid ] = go;
		}

		// tell contentdb about it - make sure it doesn't collide there either
		gpverify( gWorldMap.SetScidIndex( newScid, gpstring::EMPTY ) );
	}

	// done
	return ( goid );
}

#endif // !GP_RETAIL

bool GoDb :: TransferNodeOccupant( Go* occupant, const siege::database_guid& oldNode, const siege::database_guid& newNode )
{
	CHECK_PRIMARY_THREAD_ONLY;

	GPPROFILERSAMPLE("GoDb::TransferNodeOccupant", SP_AI );

	// $ early bailout if no change - make sure this test comes first so we
	//   don't incur any unnecessary thread-locking overhead
	if ( oldNode == newNode )
	{
		return ( false );
	}

	RECURSE_LOCK_FOR_WRITE( SiegeGoDb );

	// get go
	gpassert( !occupant->IsPendingEntry() );
	gpassert( !occupant->IsCommittingCreation() );
	gpassert( occupant->HasPlacement() );

	gpgolifef(( "GoDb::TransferNodeOccupant( occupant = 0x%08X (%s), old = %s, new = %s )\n",
				occupant->GetGoid(), occupant->GetTemplateName(), oldNode.ToString().c_str(), newNode.ToString().c_str() ));

	// these will decide if we're entering or leaving the world
	bool oldInWorld = false;
	bool newInWorld = false;

	// remove from the old node
	if ( oldNode != siege::UNDEFINED_GUID )
	{
		// get starting place
		SiegeGoDb::iterator found = m_SiegeGoDb.find( oldNode );
		if ( found != m_SiegeGoDb.end() )
		{
			// iter till we find our occupant
			while ( found->second != occupant )
			{
				++found;
				gpassert( found != m_SiegeGoDb.end() );
				gpassert( found->first == oldNode );
			}

			// erase it
			m_SiegeGoDb.erase( found );

			// was the old node in the world?
			if ( gSiegeEngine.IsNodeInAnyFrustum( oldNode ) )
			{
				oldInWorld = true;
			}
		}
	}

	// add to the new node
	if ( newNode != siege::UNDEFINED_GUID )
	{
		// add it
		m_SiegeGoDb.insert( std::make_pair( newNode, occupant ) );

		// is the new node in the world?
		if ( gSiegeEngine.IsNodeInAnyFrustum( newNode ) )
		{
			newInWorld = true;
		}
	}

	// if the game is super slow, we may get told by the follower to jump right
	// out of our own frustum before we have a chance to tell it to move! so
	// just disallow un-setting the bit of the frustum this guy owns. see #11231.
	// note that if this is the first time entering the world, we gotta do it.
	Go* frustumOwner = occupant->GetFrustumOwnerInChain();
	bool forceStayInWorld = (frustumOwner != NULL) && !(newInWorld && !occupant->IsInAnyWorldFrustum());

	// send the proper message if changed
	FrustumId newFrustumId = MakeFrustumId( gSiegeEngine.GetNodeFrustumOwnedBitfield( newNode ) );
	if ( forceStayInWorld )
	{
		oldInWorld = true;
		newInWorld = true;
		newFrustumId = MakeFrustumId( MakeInt( newFrustumId ) | MakeInt( GetGoFrustum( frustumOwner->GetGoid() ) ) );
	}
	if ( oldInWorld != newInWorld )
	{
		if ( newInWorld )
		{
			occupant->EnterWorld( newFrustumId );
		}
		else
		{
			occupant->LeaveWorld();
		}
	}

	// update frustum membership to match
	FrustumId oldMembership = occupant->GetWorldFrustumMembership();
	if ( newInWorld )
	{
		occupant->SetFrustumMembership( newFrustumId );
	}
	else
	{
		gpassert( !occupant->IsInFrustumOwnerChain() );
		occupant->SetFrustumMembership( FRUSTUMID_INVALID );
	}
	gpassert( occupant->IsInAnyWorldFrustum() == newInWorld );

	if ( occupant->IsGlobalGo() )
	{
		// reflect change to clients
		if ( IsServerLocal() )
		{
			ClientSyncGos( oldMembership, occupant->GetWorldFrustumMembership(), occupant, NULL );
		}

		// aiquery would like to know about this
		gAIQuery.OnOccupantChanged( occupant, oldNode, newNode );
	}

	// it changed
	return ( true );
}

bool GoDb :: AddNodeOccupant( Go* occupant, const siege::database_guid& newNode )
{
	return ( TransferNodeOccupant( occupant, siege::UNDEFINED_GUID, newNode ) );
}

bool GoDb :: RemoveNodeOccupant( Go* occupant, const siege::database_guid& oldNode )
{
	return ( TransferNodeOccupant( occupant, oldNode, siege::UNDEFINED_GUID ) );
}

bool GoDb :: IsNodeOccupant( Go* occupant, const siege::database_guid& node ) const
{
	gpassert( occupant );
	gpassert( node.IsValid() );

	RECURSE_LOCK_FOR_READ( SiegeGoDb );

	bool is = false;

	// get starting place
	SiegeGoDb::const_iterator found = m_SiegeGoDb.find( node );
	if ( found != m_SiegeGoDb.end() )
	{
		// iter till we find our occupant
		for ( ; (found != m_SiegeGoDb.end()) && (found->first == node) ; ++found )
		{
			if ( found->second == occupant )
			{
				is = true;
				break;
			}
		}
	}

	// return found flag
	return ( is );
}

int GoDb :: GetNodeOccupants( const siege::database_guid& node, GoidColl& coll ) const
{
	gpassert( node.IsValid() );

	RECURSE_LOCK_FOR_READ( SiegeGoDb );

	int oldSize = coll.size();

	std::pair <SiegeGoDb::const_iterator, SiegeGoDb::const_iterator> rc =
			m_SiegeGoDb.equal_range( node );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		coll.push_back( rc.first->second->GetGoid() );
	}

	return ( coll.size() - oldSize );
}

bool GoDb :: GetNodeOccupantsIters( const siege::database_guid& node, SiegeGoDb::iterator& begin, SiegeGoDb::iterator& end )
{
	gpassert( node.IsValid() );

	RECURSE_LOCK_FOR_READ( SiegeGoDb );

	std::pair <SiegeGoDb::iterator, SiegeGoDb::iterator> rc =
			m_SiegeGoDb.equal_range( node );
	begin = rc.first;
	end = rc.second;

	return ( begin != end );
}

void GoDb :: NodeChangeMembership( const siege::SiegeNode& node, unsigned int oldMembership, bool nodeWasDeleted )
{
	RECURSE_LOCK_FOR_WRITE( SiegeGoDb );

	unsigned int newMembership = node.GetFrustumOwnedBitfield();
	gpassert( nodeWasDeleted || (newMembership != oldMembership) );

	// detect node world entry/exit
	bool oldInWorld = !!oldMembership;
	bool newInWorld = !!newMembership;

	// find our range of go's rooted in this node to work with
	std::pair <SiegeGoDb::iterator, SiegeGoDb::iterator> rc = m_SiegeGoDb.equal_range( node.GetGUID() );
	if ( rc.first != rc.second )
	{
		// membership-based processing
		if ( oldMembership != newMembership )
		{
			// process go enter/exit
			for ( SiegeGoDb::iterator i = rc.first ; i != rc.second ; )
			{
				// pre-increment the iterator. this should not be necessary,
				// because we've got checks in place to watch for recursion
				// changing this while we're iterating over it, but i saw a
				// crash today i could not explain with a bad 'i' iterator.
				// so by preincrementing the situation should improve a little
				// in case this go happens to move itself.
				SiegeGoDb::iterator last = i++;

				Go* go = last->second;

				FrustumId newFrustumId = MakeFrustumId( newMembership );
				bool localOldInWorld = oldInWorld, localNewInWorld = newInWorld;

				Go* frustumOwner = go->GetFrustumOwnerInChain();
				bool forceStayInWorld = (frustumOwner != NULL) && !(newInWorld && !go->IsInAnyWorldFrustum());
				if ( forceStayInWorld )
				{
					localOldInWorld = true;
					localNewInWorld = true;
					newFrustumId = MakeFrustumId( MakeInt( newFrustumId ) | MakeInt( GetGoFrustum( frustumOwner->GetGoid() ) ) );
				}

				// on enter/leave world
				if ( !localOldInWorld && localNewInWorld )
				{
					go->EnterWorld( newFrustumId );
				}
				else if ( localOldInWorld && !localNewInWorld )
				{
					if_gpassert( go )
					{
						go->LeaveWorld();

						if ( go->IsGlobalGo() )
						{
							gAIQuery.OnOccupantChanged( go, go->GetPlacement()->GetPosition().node, siege::UNDEFINED_GUID );
						}
					}
					else
					{
						gperror( "This was a bug in Watson 12888, 12880 that we could not figure out. \n"
									"If you can repo this in the debugger, or can reproduce tell Jessica\n"
									"go in GoDb :: NodeChangeMembership " );
						// handle this case, get it out of the m_SiegeGoDb, because it shouldn't be here
						// this will avoid nasty crashes!  maybe :)
						m_SiegeGoDb.erase(last);
						continue;
					}
				}

				// update membership flags
				gpassert( forceStayInWorld || (go->GetWorldFrustumMembership() == MakeFrustumId( oldMembership )) );
				go->SetFrustumMembership( newFrustumId );

				// safety
				gpassert( go != NULL );
				gpassert( go == last->second );
			}

			// reflect change to clients
			if ( IsServerLocal() )
			{
				ClientSyncGos( MakeFrustumId( oldMembership ), MakeFrustumId( newMembership ), NULL, &rc );
			}
		}

		// if node was deleted, start auto expirations (don't force it)
		if ( nodeWasDeleted )
		{
			for ( SiegeGoDb::iterator i = rc.first ; i != rc.second ; ++i )
			{
				StartExpiration( i->second, false );
			}
		}
	}

	// notify aiq
	if ( !oldInWorld && newInWorld )
	{
		// load global scid content
		if ( !gWorldOptions.GetSkipContent() )
		{
			// $ this string is necessary to differentiate load orders for siege
			//   nodes from orders for content - both use the database guid as an
			//   id, so the string helps keep them separate.
			static gpstring s_Content( "content" );
			gSiegeLoadMgr.Load( LOADER_GO_TYPE, s_Content, node.GetGUID().GetValue() );
		}

		// notify aiq
		gAIQuery.OnNodeEnterWorld( node.GetGUID() );
	}
	else if ( oldInWorld && !newInWorld )
	{
		// notify aiq
		gAIQuery.OnNodeLeaveWorld( node.GetGUID() );
	}

	// load/erase any lodfi objects here (unless noscids=true)
	if ( !gWorldOptions.GetSkipContent() )
	{
		FrustumId mask = GetScreenPlayerFrustumMask();
		FrustumId oldInScreen = (FrustumId)((DWORD)oldMembership & (DWORD)mask);
		FrustumId newInScreen = (FrustumId)((DWORD)newMembership & (DWORD)mask);
		if ( !!oldInScreen != !!newInScreen )
		{
			gWorldMap.LoadLodfiContent( node.GetGUID(), !!newInScreen );
		}

		// do another pass to clean out local plain objects
		if ( oldMembership && !newMembership && ::IsMultiPlayer() && ::IsServerLocal() )
		{
			gWorldMap.UnloadLocalContent( node.GetGUID() );
		}
	}
}

void GoDb :: RSMarkForDeletion( Scid scid, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_CALL( RSMarkForDeletion, RPC_TO_SERVER );

	SMarkForDeletion( scid, retire, fadeOut, mpdelayed );
}

void GoDb :: RSMarkForDeletion( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_TAG();

	if ( GetGoidClass( goid ) == GO_CLASS_GLOBAL )
	{
		FUBI_RPC_CALL( RSMarkForDeletion, RPC_TO_SERVER );
	}

	SMarkForDeletion( goid, retire, fadeOut, mpdelayed );
}

void GoDb :: RSMarkForDeletion( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_TAG();

	gpassert( go != NULL );

	if ( go->IsGlobalGo() )
	{
		FUBI_RPC_CALL( RSMarkForDeletion, RPC_TO_SERVER );
	}

	SMarkForDeletion( go, retire, fadeOut, mpdelayed );
}

void GoDb :: SMarkForDeletion( Scid scid, bool retire, bool fadeOut, bool mpdelayed )
{
	GoHandle go( FindGoidByScid( scid ) );
	if ( go )
	{
		SMarkForDeletion( go, retire, fadeOut, mpdelayed );
	}
	else if ( retire && ::IsValid( scid ) )
	{
		RwCritical::WriteLock writeLock( m_ScidCritical );
		if ( m_RetiredScidColl.find( scid ) == m_RetiredScidColl.end() )
		{
			m_RetiredScidColl.insert( scid );
			gTriggerSys.RetireScid( scid );
		}
	}
}

void GoDb :: SMarkForDeletion( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	GoHandle go( goid );
	if ( go )
	{
		SMarkForDeletion( go, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: SMarkForDeletion( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	gpassert( go != NULL );

	// if not already marked
	if ( !go->IsMarkedForDeletion() )
	{
		if ( go->IsGlobalGo() )
		{
			CHECK_SERVER_ONLY;

			if ( go->CalcAllClients() )
			{
				// tell all clients to mark this for deletion next frame
				RCMarkForDeletion( RPC_TO_ALL, go, retire, fadeOut, mpdelayed );
			}
			else
			{
				// loop over all our players and tell those that know about this
				// to nuke it, but only if they DO know about it (i.e. not a
				// server-only object)
				if ( !go->IsServerOnly() )
				{
					PlayerFrustumColl::iterator i, ibegin = m_PlayerFrustumColl.begin(), iend = m_PlayerFrustumColl.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						Player* player = gServer.GetPlayer( MakePlayerIdFromIndex( i - ibegin ) );

						// only do remote players
						if ( (player != NULL) && player->IsRemote() )
						{
							// does this player know about this go?
							if ( MakeInt( go->GetWorldFrustumMembership() ) & MakeInt( i->m_OwnedMask ) )
							{
								// nuke it for them
								RCMarkForDeletion( player->GetMachineId(), go, retire, fadeOut, mpdelayed );
							}
						}
					}
				}

				// and delete it locally
				MarkForDeletion( go, retire, fadeOut, mpdelayed );
			}
		}
		else
		{
			// delete locally next frame
			MarkForDeletion( go, retire, fadeOut, mpdelayed );
		}
	}
}

void GoDb :: RCMarkForDeletion( DWORD machineId, Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
#	if GP_DEBUG
	if ( IsServerLocal() )
	{
		GoHandle go( goid );
		gpassert( go && !go->IsMarkedForDeletion() );
	}
#	endif // GP_DEBUG

	// optz: send local without pack/unpack necessary if possible
	if ( IsSendLocalOnly( machineId ) )
	{
		MarkForDeletion( goid, retire, fadeOut, mpdelayed );
		return;
	}

	GoDbDeleteXfer xfer;
	xfer.m_Goid      = goid;
	xfer.m_Retire    = retire;
	xfer.m_FadeOut   = fadeOut;
	xfer.m_MpDelayed = mpdelayed;

	FuBi::BitPacker packer;
	xfer.Xfer( packer );

	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCMarkForDeletionPacker( packer.GetSavedBits() );
}

void GoDb :: RCMarkForDeletion( DWORD machineId, Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
#	if GP_DEBUG
	if ( IsServerLocal() )
	{
		gpassert( go && !go->IsMarkedForDeletion() );
	}
#	endif // GP_DEBUG

	// optz: send local without pack/unpack necessary if possible
	if ( IsSendLocalOnly( machineId ) )
	{
		MarkForDeletion( go, retire, fadeOut, mpdelayed );
		return;
	}

	GoDbDeleteXfer xfer;
	xfer.m_Goid      = go->GetGoid();
	xfer.m_Retire    = retire;
	xfer.m_FadeOut   = fadeOut;
	xfer.m_MpDelayed = mpdelayed;

	FuBi::BitPacker packer;
	xfer.Xfer( packer );

	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCMarkForDeletionPacker( packer.GetSavedBits() );
}

void GoDb :: RCMarkForDeletionPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_PEEKADDRESS( RCMarkForDeletionPacker );

	CHECK_PRIMARY_THREAD_ONLY;

	GoDbDeleteXfer xfer;

	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer );

	// should only be deleting global go's through this function
	gpassert( GetGoidClass( xfer.m_Goid ) == GO_CLASS_GLOBAL );

	MarkForDeletion( xfer.m_Goid, xfer.m_Retire, xfer.m_FadeOut, xfer.m_MpDelayed );
}

void GoDb :: RCMarkForMondoDeletionPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_PEEKADDRESS( RCMarkForMondoDeletionPacker );

	CHECK_PRIMARY_THREAD_ONLY;

	GoDbMondoDeleteXfer xfer;

	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer );

	GoidColl::iterator i, ibegin = xfer.m_Goids.begin(), iend = xfer.m_Goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// should only be deleting global go's through this function
		gpassert( GetGoidClass( *i ) == GO_CLASS_GLOBAL );

		// do it simply
		MarkForDeletion( *i, false, false, false );
	}
}

bool GoDb :: MarkForDeletion( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	CHECK_PRIMARY_THREAD_ONLY;
	CHECK_DB_LOCKED;

	GoHandle go( goid );
	if ( go )
	{
		MarkForDeletion( go, retire, fadeOut, mpdelayed );
		return ( true );
	}
	else
	{
		// the go does not exist - add it to the recently deleted queue just
		// in case we have any outstanding retrying rpc's for this go. here's a
		// case this might happen: someone jip's when one of the characters has
		// an arrow stuck in it. the character will sync on the new client but
		// the arrow will not. any rc's made on that arrow are assumed to be ok
		// on the server due to it being all-clients, yet the client will just
		// sit there and retry on it constantly failing. putting an entry here
		// will make it think it was deleted and simply fail all of those.
		AddRecentDeletion( goid );

		// record it
		gplog( OutputGoDeletionDenied( goid ) );
	}

	return ( false );
}

void GoDb :: MarkForDeletion( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	CHECK_PRIMARY_THREAD_ONLY;
	CHECK_DB_LOCKED;

	// only mark for deletion if not set already
	if ( !go->IsMarkedForDeletion() )
	{
		// yo we should NOT be trying to delete something that's in the middle
		// of being committed...
#		if !GP_RETAIL
		{
			RwCritical::ReadLock readLock( m_OnDeckCritical );

			static bool s_IsDebugging = SysInfo::IsDebuggerPresent();
			if ( m_TempOnDeckDb.find( go->m_Goid ) != m_TempOnDeckDb.end() )
			{
				// $ special: we want to catch this thing RIGHT NOW to
				//   see what the other thread is doing that it's got
				//   the handle outstanding. so just force a
				//   breakpoint immediately if we're in the debugger.
				//   that will catch it before this thread loses its
				//   time slice...
				if ( s_IsDebugging )
				{
					BREAKPOINT();
				}
				gperrorf(( "TELL SCOTT: A game object 0x%08X scid 0x%08X template "
						   "'%s' is being marked for deletion before it's even "
						   "done committing yet!!\n\n",
						   go->m_Goid, go->m_Scid, go->GetTemplateName() ));
			}
			else if ( (go->m_Parent != NULL) && (m_TempOnDeckDb.find( go->m_Parent->m_Goid ) != m_TempOnDeckDb.end())  )
			{
				if ( s_IsDebugging )
				{
					BREAKPOINT();
				}
				gperrorf(( "TELL SCOTT: A game object 0x%08X scid 0x%08X template "
						   "'%s' is being marked for deletion before its parent "
						   "0x%08X scid 0x%08X template '%s' is even done "
						   "committing yet!!\n\n",
						   go->m_Goid, go->m_Scid, go->GetTemplateName(),
						   go->m_Parent->m_Goid, go->m_Parent->m_Scid, go->m_Parent->GetTemplateName() ));
			}

			// extra check just for fun - is that bit set right??
			if ( go->m_IsCommittingCreation )
			{
				if ( s_IsDebugging )
				{
					BREAKPOINT();
				}
				gperrorf(( "TELL SCOTT: A game object 0x%08X scid 0x%08X template "
						   "'%s' is being marked for deletion before it's even "
						   "done committing yet!!\n\n",
						   go->m_Goid, go->m_Scid, go->GetTemplateName() ));
			}
		}
#		endif // !GP_RETAIL

		// fade it out if requested, but don't bother if certain flags set
		if ( fadeOut && !go->HasFader() && !go->IsCloneSourceGo() && (go->WasInViewFrustumRecently() || go->IsPendingEntry()) )
		{
			GoFader::FadeAspect( go->GetGoid() );
		}

		// it's got to be dirty now
		go->SetBucketDirty();

		// add it as a deletion request
		go->m_MarkedForDeletion = true;
		go->m_DelayedMpDeletion = mpdelayed;

		// record it
		gplog( OutputGoDeletionMarked( go, retire, fadeOut, mpdelayed ) );

		// special case for clients if we've delayed deletion for multiplayer
		if ( mpdelayed && !::IsServerLocal() )
		{
			m_DeleteReqDelayedColl.push_back( go );
			gpassert( stdx::has_none( m_DeleteReqDelayedColl, go ) );
		}
		else
		{
			gpassert( stdx::has_none( m_DeleteReqColl, go ) );
			m_DeleteReqColl.push_back( go );
		}

		// it's got an instance
		Scid scid = go->GetScid();
		if ( IsInstance( scid, false ) )
		{
			{
				RwCritical::WriteLock writeLock( m_ScidCritical );

				// remove it from the scid index right now....should definitely
				// be there but check just in case it's not (still an error
				// condition tho, see #10009)
				ScidIndex::iterator found = m_ScidIndex.find( scid );
				if ( (found != m_ScidIndex.end()) && (found->second == go) )
				{
					m_ScidIndex.erase( found );
				}
				else
				{
#					if !GP_RETAIL
					if ( found == m_ScidIndex.end() )
					{
						gplog( OutputGoInfo( "RAID#10009 Not In ScidIndex Go", go ) );
						gperrorf(( "TELL SCOTT that you just repro'd #10009. We're "
								   "trying to remove a Go from the Scid index but "
								   "when I looked up the slot it's supposed to be "
								   "in, there was nothing there! Here is "
								   "relevant data for the Go being deleted to "
								   "append to the RAID report (and as always "
								   "please post your .dslog files!)\n"
								   "\n"
								   "goid     = 0x%08X (addr = 0x%08X)\n"
								   "scid     = 0x%08X\n"
								   "template = '%s'\n",
								   go->m_Goid, go, scid, go->GetTemplateName() ));
					}
					else
					{
						gplog( OutputGoInfo( "RAID#10009 True Owner For ScidIndex Go", go ) );
						gplog( OutputGoInfo( "RAID#10009 Imposter In ScidIndex Go", found->second ) );
						gperrorf(( "TELL SCOTT that you just repro'd #10009. We're "
								   "trying to remove a Go from the Scid index but "
								   "when I looked up the slot it's supposed to be "
								   "in, there was another Go there! Here is "
								   "relevant data for the Go being deleted to "
								   "append to the RAID report (and as always "
								   "please post your .dslog files!)\n"
								   "\n"
								   "scid = 0x%08X\n"
								   "\n"
								   "TRUE OWNER:\n"
								   "-----------\n"
								   "goid     = 0x%08X (addr = 0x%08X)\n"
								   "template = '%s'\n"
								   "\n"
								   "IMPOSTER:\n"
								   "---------\n"
								   "goid     = 0x%08X (addr = 0x%08X)\n"
								   "template = '%s'\n",
								   scid,
								   go->m_Goid, go, go->GetTemplateName(),
								   found->second->m_Goid, found->second, found->second->GetTemplateName() ));
					}
#					endif // !GP_RETAIL
				}
			}

			// retire as well if requested and global
			if ( retire && go->IsGlobalGo() )
			{
				m_RetiredScidColl.insert( scid );
			}
		}
	}
}

void GoDb :: RSMarkGoAndChildrenForDeletion( Scid scid, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_CALL( RSMarkGoAndChildrenForDeletion, RPC_TO_SERVER );

	SMarkGoAndChildrenForDeletion( scid, retire, fadeOut, mpdelayed );
}

void GoDb :: RSMarkGoAndChildrenForDeletion( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_TAG();

	if ( GetGoidClass( goid ) == GO_CLASS_GLOBAL )
	{
		FUBI_RPC_CALL( RSMarkGoAndChildrenForDeletion, RPC_TO_SERVER );
	}

	SMarkGoAndChildrenForDeletion( goid, retire, fadeOut, mpdelayed );
}

void GoDb :: RSMarkGoAndChildrenForDeletion( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	FUBI_RPC_TAG();

	if ( go && go->IsGlobalGo() )
	{
		FUBI_RPC_CALL( RSMarkGoAndChildrenForDeletion, RPC_TO_SERVER );
	}

	SMarkGoAndChildrenForDeletion( go, retire, fadeOut, mpdelayed );
}

void GoDb :: SMarkGoAndChildrenForDeletion( Scid scid, bool retire, bool fadeOut, bool mpdelayed )
{
	GoHandle go( FindGoidByScid( scid ) );
	if ( go )
	{
		SMarkGoAndChildrenForDeletion( go, retire, fadeOut, mpdelayed );
	}
	else
	{
		SMarkForDeletion( scid, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: SMarkGoAndChildrenForDeletion( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	gpassert( goid != GOID_INVALID );

	// add the deletion request if it's valid
	GoHandle go( goid );
	if ( go )
	{
		SMarkGoAndChildrenForDeletion( go, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: SMarkGoAndChildrenForDeletion( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	gpassert( go != NULL );

	// delete this go
	RSMarkForDeletion( go, retire, fadeOut, mpdelayed );

	// and do its children too
	GopColl::const_iterator i, begin = go->GetChildren().begin(), end = go->GetChildren().end();
	for ( i = begin ; i != end ; ++i )
	{
		SMarkGoAndChildrenForDeletion( *i, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: MarkForDeletionReassignChildren( Scid scid, bool retire, bool fadeOut, bool mpdelayed )
{
	GoHandle go( FindGoidByScid( scid ) );
	if ( go )
	{
		MarkForDeletionReassignChildren( go->GetGoid(), retire, fadeOut, mpdelayed );
	}
	else
	{
		SMarkForDeletion( scid, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: MarkForDeletionReassignChildren( Goid goid, bool retire, bool fadeOut, bool mpdelayed )
{
	gpassert( goid != GOID_INVALID );

	// add the deletion request if it's valid
	GoHandle go( goid );
	if ( go )
	{
		// reassign the children if there's a parent
		if ( go->HasParent() )
		{
			// reassign
			GopColl children = go->GetChildren();
			GopColl::iterator i, begin = children.begin(), end = children.end();
			for ( i = begin ; i != end ; ++i )
			{
				// first do normal parent
				(*i)->SetParent( go->GetParent() );
			}
		}

		// finally, delete this go
		SMarkForDeletion( go, retire, fadeOut, mpdelayed );
	}
}

void GoDb :: DeletePlayerGos( PlayerId playerId, bool retire )
{
	// $ slow function, but should be called rarely

	Critical::Lock serializer1( m_CreateLock );
	RwCritical::WriteLock serializer2( m_DbCritical );

	// get the player (may be null if we want all player go's deleted)
	Player* player = NULL;
	if ( playerId != PLAYERID_INVALID )
	{
		player = gServer.GetPlayer( playerId );
	}

	// commit any leftover stuff
	CommitAllRequests( false );

	// delete all pending
	{
		SiegeGopCollDb::iterator i, ibegin = m_PendingGoDb.begin(), iend = m_PendingGoDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GopColl::iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( (*j != NULL) && ((player == NULL) || ((*j)->GetPlayer() == player)) )
				{
					MarkForDeletion( *j, retire );
				}
			}
		}
	}

	// delete all globals minus people with frustums and screen party parents
	{
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if (   ((player == NULL) || (i->GetPlayer() == player))
				&& !i->HasFrustum()
				&& !i->IsAnyHumanParty() )
			{
				MarkForDeletion( &*i, retire, (playerId != PLAYERID_INVALID) && i->IsAllClients() );
			}
		}
	}

	// force deletions
	CommitAllRequests( false );

	// delete all frustum owners in inactive frustums minus their parents
	{
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if (   ((player == NULL) || (i->GetPlayer() == player))
				&& !i->IsAnyHumanParty()
				&& !i->IsInActiveWorldFrustum() )
			{
				MarkForDeletion( &*i, retire, (playerId != PLAYERID_INVALID) && i->IsAllClients() );
			}
		}
	}

	// force deletions
	CommitAllRequests( false );

	// delete all remaining frustum owners minus their parents
	{
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if (   ((player == NULL) || (i->GetPlayer() == player))
				&& !i->IsAnyHumanParty() )
			{
				MarkForDeletion( &*i, retire, (playerId != PLAYERID_INVALID) && i->IsAllClients() );
			}
		}
	}

	// force deletions
	CommitAllRequests( false );

	// delete all remaining go's - should only be parties
	{
		GlobalIter i, ibegin = GlobalBegin(), iend = GlobalEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (player == NULL) || (i->GetPlayer() == player) )
			{
				MarkForDeletion( &*i, retire );
			}
		}
	}

	// commit them all
	CommitAllRequests( true );

	// don't need these if we're destroying everyone
	if ( playerId == PLAYERID_INVALID )
	{
		RebuildCreationCaches();
	}
	else if ( playerId != PLAYERID_COMPUTER )
	{
		int playerIndex = ::MakeIndex( playerId ) - 1;
		m_CreateGoXferCache[ playerIndex ] = CreateGoXfer();
		m_CloneGoXferCache [ playerIndex ] = CloneGoXfer ();
	}
}

static bool CheckScidBitsScid( Scid scid )
{
	if ( scid == SCID_SPAWNED )
	{
		gperrorf(( "Someone is trying to mess with scidbits on a SPAWNED scid! "
				   "Did you just use the 'add' command on the console? If so, "
				   "then you have just added a Go that must be placed in SE. "
				   "Not supported currently.\n" ));
		return ( false );
	}
	else if ( !::IsInstance( scid, false ) )
	{
		gperrorf(( "Someone is trying to mess with scidbits on an invalid scid 0x%08X!\n", scid ));
		return ( false );
	}

	return ( true );
}

DWORD GoDb :: GetScidBits( Scid scid, DWORD defValue ) const
{
	RwCritical::ReadLock locker( m_ScidBitsCritical );

	GPDEV_ONLY( CheckScidBitsScid( scid ) );
	ScidBitsColl::const_iterator found = m_ScidBitsColl.find( scid );
	return ( (found != m_ScidBitsColl.end()) ? found->second.m_Bits : defValue );
}

bool GoDb :: GetScidBit( Scid scid, int index, bool defValue ) const
{
	RwCritical::ReadLock locker( m_ScidBitsCritical );

	GPDEV_ONLY( CheckScidBitsScid( scid ) );
	gpassert( (index >= 0) && (index < (sizeof( ScidBitsColl::referent_type ) * 8)) );

	ScidBitsColl::const_iterator found = m_ScidBitsColl.find( scid );
	return ( (found != m_ScidBitsColl.end()) ? !!(found->second.m_Bits & (1 << index)) : defValue );
}

DWORD GoDb :: SSetScidBits( Scid scid, DWORD value )
{
	CHECK_SERVER_ONLY;
	RCSetScidBits( scid, value );
	return ( SetScidBits( scid, value, true ) );
}

void GoDb :: RCSetScidBits( Scid scid, DWORD value )
{
	FUBI_RPC_CALL( RCSetScidBits, RPC_TO_OTHERS );
	SetScidBits( scid, value, true );
}

DWORD GoDb :: SetScidBits( Scid scid, DWORD value, bool forRpc )
{
#	if !GP_RETAIL
	if ( IsEditMode() )
	{
		return ( 0 );
	}
#	endif // !GP_RETAIL

	// early out on bad request
	if ( !CheckScidBitsScid( scid ) )
	{
		return ( 0 );
	}

	// make the change
	DWORD old = 0;
	{
		RwCritical::WriteLock locker( m_ScidBitsCritical );

		std::pair <ScidBitsColl::iterator, bool> rc
				= m_ScidBitsColl.insert( std::make_pair( scid, ScidBits( value, forRpc ) ) );
		if ( !rc.second )
		{
			ScidBits& bits = rc.first->second;
			old = bits.m_Bits;
			bits.m_Bits = value;

			// override rpc if necessary
			if ( forRpc )
			{
				bits.m_MustRpc = true;
			}
		}
	}

	// notify of change
	if ( old != value )
	{
		Goid goid = FindGoidByScid( scid );
		if ( ::IsValid( goid ) )
		{
			WorldMessage( WE_SCIDBITS_CHANGED, GOID_INVALID, goid, old ).Send();
		}
	}

	// done
	return ( old );
}

bool GoDb :: SSetScidBit( Scid scid, int index, bool value )
{
	CHECK_SERVER_ONLY;
	RCSetScidBit( scid, index, value );
	return ( SetScidBit( scid, index, value, true ) );
}

void GoDb :: RCSetScidBit( Scid scid, int index, bool value )
{
	FUBI_RPC_CALL( RCSetScidBit, RPC_TO_OTHERS );
	SetScidBit( scid, index, value, true );
}

bool GoDb :: SetScidBit( Scid scid, int index, bool value, bool forRpc )
{
#	if !GP_RETAIL
	if ( IsEditMode() )
	{
		return ( false );
	}
#	endif // !GP_RETAIL

	// early out on bad request
	if ( !CheckScidBitsScid( scid ) )
	{
		return ( 0 );
	}

	gpassert( (index >= 0) && (index < (sizeof( ScidBitsColl::referent_type ) * 8)) );

	// make the change
	bool oldBit = false;
	DWORD newBit = 1 << index;
	DWORD newBits = value ? newBit : 0;
	DWORD oldBits = 0;
	{
		RwCritical::WriteLock locker( m_ScidBitsCritical );

		std::pair <ScidBitsColl::iterator, bool> rc
				= m_ScidBitsColl.insert( std::make_pair( scid, ScidBits( newBits, forRpc ) ) );
		if ( !rc.second )
		{
			ScidBits& bits = rc.first->second;
			oldBits = bits.m_Bits;
			oldBit = !!(oldBits & newBit);

			if ( value )
			{
				bits.m_Bits |= newBit;
			}
			else
			{
				bits.m_Bits &= ~newBit;
			}

			newBits = bits.m_Bits;

			// override rpc if necessary
			if ( forRpc )
			{
				bits.m_MustRpc = true;
			}
		}
	}

	// notify of change
	if ( oldBits != newBits )
	{
		Goid goid = FindGoidByScid( scid );
		if ( ::IsValid( goid ) )
		{
			WorldMessage( WE_SCIDBITS_CHANGED, GOID_INVALID, goid, oldBits ).Send();
		}
	}

	// done
	return ( oldBit );
}

int GoDb :: GetQuestInt( Goid goid, const char* mapName, const char* keyName, int defInt ) const
{
	int valInt = defInt;
	::FromString( GetQuestString( goid, mapName, keyName ), valInt );
	return ( valInt );
}

int GoDb :: GetQuestInt( Goid goid, const char* mapName, const char* keyName ) const
{
	return ( GetQuestInt( goid, mapName, keyName, 0 ) );
}

bool GoDb :: GetQuestBool( Goid goid, const char* mapName, const char* keyName, bool defBool ) const
{
	bool valBool = defBool;
	::FromString( GetQuestString( goid, mapName, keyName ), valBool );
	return ( valBool );
}

bool GoDb :: GetQuestBool( Goid goid, const char* mapName, const char* keyName ) const
{
	return ( GetQuestBool( goid, mapName, keyName, false ) );
}

float GoDb :: GetQuestFloat( Goid goid, const char* mapName, const char* keyName, float defFloat ) const
{
	float valFloat = defFloat;
	::FromString( GetQuestString( goid, mapName, keyName ), valFloat );
	return ( valFloat );
}

float GoDb :: GetQuestFloat( Goid goid, const char* mapName, const char* keyName ) const
{
	return ( GetQuestFloat( goid, mapName, keyName, 0 ) );
}

static void CleanupQuestParams( const char*& mapName, const char*& keyName )
{
	if ( keyName == NULL )
	{
		keyName = "";
	}
	if ( (mapName == NULL) || (*mapName == '\0') )
	{
		if ( WorldMap::DoesSingletonExist() )
		{
			mapName = gWorldMap.GetMapName();
		}
		else
		{
			mapName = "";
		}
	}
}

const gpstring& GoDb :: GetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& defString ) const
{
	CleanupQuestParams( mapName, keyName );

	QuestBitsDb::const_iterator f0 = m_QuestBitsDb.find( goid );
	if ( f0 != m_QuestBitsDb.end() )
	{
		StringStringDb::const_iterator f1 = f0->second.find( mapName );
		if ( f1 != f0->second.end() )
		{
			StringDb::const_iterator f2 = f1->second.find( keyName );
			if ( f2 != f1->second.end() )
			{
				return ( f2->second );
			}
		}
	}

	return ( defString );
}

const gpstring& GoDb :: GetQuestString( Goid goid, const char* mapName, const char* keyName ) const
{
	return ( GetQuestString( goid, mapName, keyName, gpstring::EMPTY ) );
}

void GoDb :: SSetQuestInt( Goid goid, const char* mapName, const char* keyName, int valInt )
{
	SSetQuestString( goid, mapName, keyName, (valInt != 0) ? ::ToString( valInt ) : gpstring::EMPTY );
}

void GoDb :: RSSetQuestInt( Goid goid, const char* mapName, const char* keyName, int valInt )
{
	FUBI_RPC_CALL( RSSetQuestInt, RPC_TO_SERVER );

	SSetQuestInt( goid, mapName, keyName, valInt );
}

void GoDb :: SetQuestInt( Goid goid, const char* mapName, const char* keyName, int valInt )
{
	SetQuestString( goid, mapName, keyName, (valInt != 0) ? ::ToString( valInt ) : gpstring::EMPTY );
}

void GoDb :: SSetQuestBool( Goid goid, const char* mapName, const char* keyName, bool valBool )
{
	SSetQuestString( goid, mapName, keyName, valBool ? ::ToString( valBool ) : gpstring::EMPTY );
}

void GoDb :: RSSetQuestBool( Goid goid, const char* mapName, const char* keyName, bool valBool )
{
	FUBI_RPC_CALL( RSSetQuestBool, RPC_TO_SERVER );

	SSetQuestBool( goid, mapName, keyName, valBool );
}

void GoDb :: SetQuestBool( Goid goid, const char* mapName, const char* keyName, bool valBool )
{
	SetQuestString( goid, mapName, keyName, valBool ? ::ToString( valBool ) : gpstring::EMPTY );
}

void GoDb :: SSetQuestFloat( Goid goid, const char* mapName, const char* keyName, float valFloat )
{
	SSetQuestString( goid, mapName, keyName, (!::IsZero( valFloat )) ? ::ToString( valFloat ) : gpstring::EMPTY );
}

void GoDb :: RSSetQuestFloat ( Goid goid, const char* mapName, const char* keyName, float valFloat )
{
	FUBI_RPC_CALL( RSSetQuestFloat, RPC_TO_SERVER );

	SSetQuestFloat( goid, mapName, keyName, valFloat );
}

void GoDb :: SetQuestFloat( Goid goid, const char* mapName, const char* keyName, float valFloat )
{
	SetQuestString( goid, mapName, keyName, (!::IsZero( valFloat )) ? ::ToString( valFloat ) : gpstring::EMPTY );
}

void GoDb :: SSetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString )
{
	CHECK_SERVER_ONLY;

	RCSetQuestString( goid, mapName, keyName, valString );
}

void GoDb :: RSSetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString )
{
	FUBI_RPC_THIS_CALL( RSSetQuestString, RPC_TO_SERVER );

	SSetQuestString( goid, mapName, keyName, valString );
}

void GoDb :: RCSetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString )
{
	FUBI_RPC_CALL( RCSetQuestString, RPC_TO_ALL );
	SetQuestString( goid, mapName, keyName, valString );
}

void GoDb :: SetQuestString( Goid goid, const char* mapName, const char* keyName, const gpstring& valString )
{
	CleanupQuestParams( mapName, keyName );

	// special case for 'any', do it to all party members
	if ( goid == GOID_ANY )
	{
		for ( Server::PlayerColl::iterator i = gServer.GetPlayersBegin() ; i != gServer.GetPlayersEnd() ; ++i )
		{
			Player* player = *i;
			if ( IsValid( player ) && !player->IsComputerPlayer() )
			{
				Go* party = player->GetParty();
				if ( party != NULL )
				{
					for ( GopColl::iterator j = party->GetChildBegin() ; j != party->GetChildEnd() ; ++j )
					{
						SetQuestString( (*j)->GetGoid(), mapName, keyName, valString );
					}
				}
			}
		}
	}
	else if ( IsValid( goid ) )
	{
		// ok process
		if ( !valString.empty() )
		{
			// simple, just set it
			m_QuestBitsDb[ goid ][ mapName ][ keyName ] = valString;
		}
		else
		{
			// we want to clear it! maybe we can erase some space too. note that if
			// we don't find an existing entry, then there is no need to do
			// anything here.
			QuestBitsDb::iterator f0 = m_QuestBitsDb.find( goid );
			if ( f0 != m_QuestBitsDb.end() )
			{
				StringStringDb::iterator f1 = f0->second.find( mapName );
				if ( f1 != f0->second.end() )
				{
					StringDb::iterator f2 = f1->second.find( keyName );
					if ( f2 != f1->second.end() )
					{
						f1->second.erase( f2 );
						if ( f1->second.empty() )
						{
							f0->second.erase( f1 );
							if ( f0->second.empty() )
							{
								m_QuestBitsDb.erase( f0 );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		gperrorf(( "Invalid goid 0x%08X!\n", goid ));
		return;
	}
}

bool GoDb :: StartExpiration( Go* go, bool forced )
{
	CHECK_PRIMARY_THREAD_ONLY;

	// cancel old if any first
	if ( !CancelExpiration( go, forced ) )
	{
		return ( false );
	}

	// don't bother expiring if it's already on its way out
	if ( go->IsMarkedForDeletion() )
	{
		return ( false );
	}

	// don't bother expiring if we're auto and that type is disabled
	if ( !forced && !go->IsAutoExpirationEnabled() )
	{
		return ( false );
	}

	// access go
	gpassert( go != NULL );
	gpassert( !go->IsExpiring() );

	// ignore inventory items - the parent expires all the children too
	if ( go->IsInsideInventory() )
	{
		return ( false );
	}

	// only the server may expire global go's
	if ( go->IsGlobalGo() && !IsServerLocal() )
	{
		return ( false );
	}

	// only bother with these checks for non-lodfi go's
	float delay = 0;
	if ( !go->IsLodfi() )
	{
		if ( !go->IsCloneSourceGo() )
		{
			if ( go->HasCommon() && !go->GetCommon()->CanExpireTriggers() )
			{
				// We need to keep this go around, it's triggers can't be expired
				if ( go->HasAspect() )
				{
					gpdevreportf( &gTriggerSysContext,(
							"TRIGGER WARNING: Unable to expire a go ['%s', goid=0x%08x, scid=0x%08X] that has a GoAspect "
							"because it has triggers attached to it that cannot expire. This is a content error - visible "
							"objects should not have 'advanced' triggers attached to them.",
							go->GetTemplateName(),
							go->GetGoid(),
							go->GetScid() ));
				}
				return ( false );
			}

			// find expiration class infoz
			const gpstring& expireClass
					= forced
					? go->GetCommon()->GetForcedExpirationClass()
					: go->GetCommon()->GetAutoExpirationClass();

			const ContentDb::ExpireEntry* expire = gContentDb.FindExpirationClass( expireClass );
			gpassert( expire != NULL );
			if ( expire == NULL )
			{
				return ( true );
			}

			// never expires?
			if ( expire->m_DelaySeconds < 0 )
			{
				return ( true );
			}

			// optionally retire this now
			if ( forced && IsInstance( go->GetScid(), false ) )
			{
				m_RetiredScidColl.insert( go->GetScid() );
			}

			// figure out our timeout
			delay = expire->m_DelaySeconds;
			delay += Random( -expire->m_RangeSeconds * 0.5f, expire->m_RangeSeconds * 0.5f );
		}
		else
		{
			// clone sources all have the same expiration, but add a little
			// randomness to it so we don't have bunching problems causing
			// hitching.
			delay = m_CloneSrcLruTimeout + Random( -m_CloneSrcLruTimeout * 0.1f, m_CloneSrcLruTimeout * 0.1f );
		}
	}

	// figure out our timeout
	double time = gWorldTime.GetTime();
	double timeout = PreciseAdd( time, delay );

	// note that the go is expiring now
	gpassert( !go->IsExpiring() );
	go->m_IsExpiring = true;

	// check for immediate expiration
	if ( timeout <= time )
	{
		// notify of impending doom
		if ( !go->IsCloneSourceGo() )
		{
			WorldMessage( forced ? WE_EXPIRED_FORCED : WE_EXPIRED_AUTO, go->GetGoid() ).Send( MD_CC_CHILDREN );
		}

		// ok delete 'em
		SMarkGoAndChildrenForDeletion( go, forced, forced );
	}
	else
	{
		RECURSE_LOCK_FOR_WRITE( ExpireDb );

		// ok then! start expiring!
		ExpireEntry entry;
		entry.m_Go = go;
		entry.m_Forced = forced;
		ExpireDb::iterator newEntry = m_ExpireDb.insert( std::make_pair( timeout, entry ) );
		std::pair <ExpireReverseDb::iterator, bool> rc
				= m_ExpireReverseDb.insert( std::make_pair( go, newEntry ) );
		gpassert( rc.second );
	}

	// done
	return ( true );
}

bool GoDb :: CancelExpiration( Go* go, bool forced )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_LOCK_FOR_WRITE( ExpireDb );

	gpassert( go != NULL );

	// see if it is already expiring
	bool success = true;
	if ( go->IsExpiring() )
	{
		ExpireReverseDb::iterator oldEntry = m_ExpireReverseDb.find( go );
		if ( oldEntry != m_ExpireReverseDb.end() )
		{
			// only remove if we're forcing the cancel or if old was not forced
			if ( forced || !oldEntry->second->second.m_Forced )
			{
				// remove old entry
				m_ExpireDb.erase( oldEntry->second );
				m_ExpireReverseDb.erase( oldEntry );
				go->m_IsExpiring = false;

				// if it was tagged for retirement (and not being deleted), clear that too
				if ( IsInstance( go->GetScid(), false ) && !go->IsMarkedForDeletion() )
				{
					ScidColl::iterator found = m_RetiredScidColl.find( go->GetScid() );
					if ( found != m_RetiredScidColl.end() )
					{
						m_RetiredScidColl.erase( found );
					}
				}
			}
			else
			{
				success = false;
			}
		}
		else
		{
			// go's can only be expiring yet not be in the db if they were
			// marked for deletion. very rarely during teleport this might
			// happen.
			gpassert( go->IsMarkedForDeletion() );
			success = false;
		}
	}

	// done
	return ( success );
}

bool GoDb :: IsExpiring( Go* go, float* timeLeft ) const
{
	bool is = go && go->IsExpiring();
	if ( is && (timeLeft != NULL) )
	{
		ExpireReverseDb::const_iterator found = m_ExpireReverseDb.find( go );
		gpassert( found != m_ExpireReverseDb.end() );
		*timeLeft = scast <float> ( PreciseSubtract( found->second->first, gWorldTime.GetTime() ) );
	}

	return ( is );
}

#if !GP_RETAIL

void GoDb :: ExpireAllImmediately( void )
{
	ExpireDb::iterator i, ibegin = m_ExpireDb.begin(), iend = m_ExpireDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// ok the compiler doesn't like this 'cause keys are supposed to be
		// const, but we know it's ok, don't we. :)
		ccast <double&> ( i->first ) = 0;
	}
}

#endif // !GP_RETAIL

bool GoDb :: Select( Goid goid, bool select )
{
	gpassert( goid != GOID_INVALID );

	bool oldState = false;

	GoHandle go( goid );
	if ( go )
	{
		// verify some stuff
		gpassert( go->IsGlobalGo() );
		gpassert( !select || go->IsSelectable() || gSiegeEngine.GetOptions().SelectAllObjects() );

		// select it
		if ( select )
		{
			// try to insert
			GopColl::iterator found = stdx::find( m_SelectionColl, go );
			if ( found != m_SelectionColl.end() )
			{
				oldState = true;

				// selected again - let's move it to the front
				if ( found != m_SelectionColl.begin() )
				{
					m_SelectionColl.erase( found );
					m_SelectionColl.insert( m_SelectionColl.begin(), go );
				}
			}
			else
			{
				// add
				m_SelectionColl.push_back( go );
				WorldMessage( WE_SELECTED, goid ).Send();

				// notify of a change
				if ( m_SelectionChangedCb )
				{
					m_SelectionChangedCb();
				}
			}
		}
		else if ( go->IsSelected() != select )
		{
			oldState = true;

			GopColl::iterator found = stdx::find( m_SelectionColl, go );
			if_gpassert( found != m_SelectionColl.end() )
			{
				// remove
				m_SelectionColl.erase( found );
				WorldMessage( WE_DESELECTED, goid ).Send();
			}

			// notify of a change
			if ( m_SelectionChangedCb )
			{
				m_SelectionChangedCb();
			}
		}

		// check for change to focus
		CheckFocusChange();
	}

	return ( oldState );
}

void GoDb :: DeselectAll( void )
{
	// tell go's to clear their selection state
	GopColl::iterator i, begin = m_SelectionColl.begin(), end = m_SelectionColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		// message the object
		WorldMessage( WE_DESELECTED, (*i)->GetGoid() ).Send();
	}

	// clear the selection set
	m_SelectionColl.clear();

	// notify of a change
	if ( m_SelectionChangedCb )
	{
		m_SelectionChangedCb();
	}

	// check for change to focus
	CheckFocusChange();
}

bool GoDb :: IsSelected( Goid goid ) const
{
	gpassert( goid != GOID_INVALID );

	GoHandle go( goid );
	return ( go ? go->IsSelected() : false );
}

bool GoDb :: AddToHotGroup( Goid goid, bool add )
{
	gpassert( goid != GOID_INVALID );

	bool oldState = false;

	GoHandle go( goid );
	if ( go )
	{
		// verify some stuff
		gpassert( go->IsGlobalGo() );
		gpassert( go->GetPlayerId() == gServer.GetScreenPlayer()->GetId() );

		// $ early bailout
		if ( go->IsInHotGroup() == add )
		{
			return ( add );
		}
		gpassert( stdx::has_any( m_HotColl, go ) == !add );

		// add it
		oldState = add;
		if ( add )
		{
			m_HotColl.push_back( go );
			WorldMessage( WE_HOTGROUP, goid ).Send();
		}
		else
		{
			m_HotColl.erase( stdx::find( m_HotColl, go ) );
			WorldMessage( WE_UNHOTGROUP, goid ).Send();
		}
	}

	return ( oldState );
}

void GoDb :: ReplaceHotGroup( const GopColl& coll )
{
	// clear hot group bits
	GopColl::const_iterator i, ibegin = m_HotColl.begin(), iend = m_HotColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)->ClearVisit();
	}

	// add all new entries
	ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Go* go = *i;
		go->SetVisit();

		if ( !go->IsInHotGroup() )
		{
			gpassert( go->GetPlayerId() == gServer.GetScreenPlayer()->GetId() );
			gpassert( !stdx::has_any( m_HotColl, go ) );
			m_HotColl.push_back( go );
			WorldMessage( WE_HOTGROUP, go->GetGoid() ).Send();
		}
	}

	// remove from hot group unvisited go's
	GopColl::iterator j = m_HotColl.begin();
	while ( j != m_HotColl.end() )
	{
		Go* go = *j;
		if ( !go->IsVisited() )
		{
			j = m_HotColl.erase( j );
			WorldMessage( WE_UNHOTGROUP, go->GetGoid() ).Send();
		}
		else
		{
			++j;
		}
	}
}

void GoDb :: RemoveAllFromHotGroup( void )
{
	// tell go's to clear their hot state
	GopColl::iterator i, begin = m_HotColl.begin(), end = m_HotColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		// message the object
		WorldMessage( WE_UNHOTGROUP, (*i)->GetGoid() ).Send();
	}

	// clear the selection set
	m_HotColl.clear();
}

bool GoDb :: IsInHotGroup( Goid goid )
{
	gpassert( goid != GOID_INVALID );

	GoHandle go( goid );
	return ( go ? go->IsInHotGroup() : false );
}

bool GoDb :: IsMouseShadowed( Goid goid ) const
{
	gpassert( goid != GOID_INVALID );

	GoHandle go( goid );
	return ( go ? go->IsMouseShadowed() : false );
}

void GoDb :: SOnInitialFrustumRegistered( Player* player )
{
	// make sure the frustum entry exists
	int index = ::MakeIndex( player->GetId() );
	m_PlayerFrustumColl.resize( max_t( index + 1, (int)m_PlayerFrustumColl.size() ) );

	// take the frustum initially created for this player
	PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
	gpassert( frustum.m_OwnedMask == 0 );
	frustum.m_OwnedMask = player->GetInitialFrustum();
}

FuBiCookie GoDb :: RCAssignFrustumToGo( DWORD machineId, FrustumId frustumId, Goid goid )
{
	FUBI_RPC_CALL_RETRY( RCAssignFrustumToGo, machineId );

	AssignFrustumToGo( frustumId, goid );
	return ( RPC_SUCCESS );
}

FuBiCookie GoDb :: SAssignFrustumToGo( FrustumId frustumId, Goid goid )
{
	CHECK_SERVER_ONLY;

	// tell server to assign
	AssignFrustumToGo( frustumId, goid );

	// tell client to assign
	FuBiCookie cookie = RPC_SUCCESS;
	DWORD machineId = GoHandle( goid )->GetMachineId();
	if ( machineId != RPC_TO_LOCAL )
	{
		cookie = RCAssignFrustumToGo( machineId, frustumId, goid );
	}
	return ( cookie );
}

void GoDb :: AssignFrustumToGo( FrustumId frustumId, Goid goid )
{
	gpassert( IsValid( frustumId ) );

	// get at go
	GoHandle go( goid );
	gpassert( go );

	// make sure we don't have any go's already registered with this frustum
#   if GP_DEBUG
	gpassert( !go->HasFrustum() );
	PlayerFrustumColl::const_iterator i, ibegin = m_PlayerFrustumColl.begin(), iend = m_PlayerFrustumColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		PlayerFrustum::Coll::const_iterator j, jbegin = i->m_Coll.begin(), jend = i->m_Coll.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			gpassert( j->m_Go != go.Get() );
			gpassert( j->m_FrustumId != frustumId );
		}
	}
#   endif // GP_DEBUG

	// make sure we have a slot for this player
	int index = ::MakeIndex( go->GetPlayerId() );
	m_PlayerFrustumColl.resize( max_t( index + 1, (int)m_PlayerFrustumColl.size() ) );

	// add new one - note that this may be already assigned in the case of the
	// initial frustum
	PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
	frustum.m_Coll.push_back( GoFrustum( go, frustumId ) );
	BitFlagsSet( frustum.m_OwnedMask, frustumId );

	// tell go about it
	go->SetHasFrustum();

	CheckFocusChange();
}

FuBiCookie GoDb :: SCreateFrustumForGo( Goid goid )
{
	CHECK_SERVER_ONLY;

	GoHandle go( goid );
	gpassert( go );
	gpassert( go->IsGlobalGo() );

	// brand new, ok create new frustum
	FrustumId frustumId;
	FuBiCookie cookie = gWorldTerrain.SRegisterWorldFrustum( frustumId, go->GetPlayer() );

	// add new one
	if ( cookie != RPC_FAILURE )
	{
		gpassert( cookie == RPC_SUCCESS );
		cookie = SAssignFrustumToGo( frustumId, goid );
	}

	// done
	return ( cookie );
}

void GoDb :: RemoveFrustumFromPlayer( PlayerId playerId )
{
	CHECK_SERVER_ONLY;

	// get the player
	gpassert( playerId != PLAYERID_INVALID );
	Player* player = NULL;
	if ( playerId != PLAYERID_INVALID )
	{
		player = gServer.GetPlayer( playerId );
	}
	gpassert( player );

	int index = ::MakeIndex( playerId );
	assert( (UINT)index <= m_PlayerFrustumColl.size() );

	// take the frustum initially created for this player
	PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
	gpassert( frustum.m_OwnedMask == player->GetInitialFrustum() );

	// delete the player's frustrum
	BitFlagsClear( m_ActiveFrustumMask, player->GetInitialFrustum() );
	gWorldTerrain.DestroyWorldFrustum( player->GetInitialFrustum() );

	// clear out the frustum mask
	gpassert( BitFlagsContainAny( frustum.m_OwnedMask, player->GetInitialFrustum() ) );
	BitFlagsClear( frustum.m_OwnedMask, player->GetInitialFrustum() );
}

FuBiCookie GoDb :: RCReleaseFrustumFromGo( DWORD machineId, Goid goid, bool eraseFrustum )
{
	FUBI_RPC_CALL_RETRY( RCReleaseFrustumFromGo, machineId );

	ReleaseFrustumFromGo( goid, eraseFrustum );
	return ( RPC_SUCCESS );
}

FuBiCookie GoDb :: SReleaseFrustumFromGo( Goid goid, bool eraseFrustum )
{
	CHECK_SERVER_ONLY;

	// tell server to release
	ReleaseFrustumFromGo( goid, eraseFrustum );

	// tell client to release
	FuBiCookie cookie = RPC_SUCCESS;
	DWORD machineId = GoHandle( goid )->GetMachineId();
	if ( machineId != RPC_TO_LOCAL )
	{
		cookie = RCReleaseFrustumFromGo( machineId, goid, eraseFrustum );
	}
	return ( cookie );
}

void GoDb :: ReleaseFrustumFromGo( Goid goid, bool eraseFrustum )
{
	// get at go
	GoHandle go( goid );
	gpassert( go );

	// defocus
	if ( go->IsFocused() )
	{
		ClearFocusGo( go->GetPlayerId() );
	}

	// deselect now to prevent recursion (#10548)
	go->Deselect();

	// get at collection
	int index = ::MakeIndex( go->GetPlayerId() );
	PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];

	// find it
	PlayerFrustum::Coll::iterator i, ibegin = frustum.m_Coll.begin(), iend = frustum.m_Coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_Go == go )
		{
			break;
		}
	}
	gpassert( i != iend );

	// remove it
	FrustumId frustumId = i->m_FrustumId;
	frustum.m_Coll.erase( i );
	gpassert( BitFlagsContainAny( frustum.m_OwnedMask, frustumId ) );
	BitFlagsClear( frustum.m_OwnedMask, frustumId );

	// tell go about it
	go->SetHasFrustum( false );

	// update focus now before frustum goes away and party gets moved
	CheckFocusChange();

	// kill the frustum if asked
	if ( eraseFrustum )
	{
		BitFlagsClear( m_ActiveFrustumMask, frustumId );
		gWorldTerrain.DestroyWorldFrustum( frustumId );
	}
	else
	{
		// if this was the last one, tell the player to keep it
		if ( frustum.m_Coll.empty() )
		{
			go->GetPlayer()->SetInitialFrustum( frustumId );
		}
	}
}

FrustumId GoDb :: GetGoFrustum( Goid goid )
{
	gpassert( goid != GOID_INVALID );

	FrustumId frustumId = FRUSTUMID_INVALID;

	GoHandle go( goid );
	if ( go )
	{
		// get at collection
		int index = ::MakeIndex( go->GetPlayerId() );
		if ( index < (int)m_PlayerFrustumColl.size() )
		{
			const PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];

			// find it
			PlayerFrustum::Coll::const_iterator i, ibegin = frustum.m_Coll.begin(), iend = frustum.m_Coll.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_Go == go.Get() )
				{
					frustumId = i->m_FrustumId;
					break;
				}
			}
			gpassert( (frustumId == FRUSTUMID_INVALID) || BitFlagsContainAny( frustum.m_OwnedMask, frustumId ) );
		}
	}

	return ( frustumId );
}

FrustumId GoDb :: GetPlayerFrustumMask( PlayerId playerId )
{
	FrustumId mask = FRUSTUMID_INVALID;

	int index = ::MakeIndex( playerId );
	if ( index < (int)m_PlayerFrustumColl.size() )
	{
		mask = m_PlayerFrustumColl[ index ].m_OwnedMask;
	}

	// if it's not here yet we may not have it set up (such as during map load)
	if ( mask == FRUSTUMID_INVALID )
	{
		Player* player = gServer.GetPlayer( playerId );
		if ( player != NULL )
		{
			mask = player->GetInitialFrustum();
		}
	}

	return ( mask );
}

FrustumId GoDb :: GetScreenPlayerFrustumMask( void )
{
	FrustumId mask = FRUSTUMID_INVALID;

	Player* player = gServer.GetScreenPlayer();
	if ( player != NULL )
	{
		mask = GetPlayerFrustumMask( player->GetId() );

		// we may not have our party set up yet...
		if ( mask == FRUSTUMID_INVALID )
		{
			mask = player->GetInitialFrustum();
		}
	}

	return ( mask );
}

bool GoDb :: FillAddressesForFrustum( FrustumId goFrustum, FuBi::AddressColl& addresses ) const
{
	gpassert( goFrustum != FRUSTUMID_INVALID );
	int playerCount = 0, hitCount = 0;

	// loop over all our players
	PlayerFrustumColl::const_iterator i, ibegin = m_PlayerFrustumColl.begin(), iend = m_PlayerFrustumColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Player* player = gServer.GetPlayer( MakePlayerIdFromIndex( i - ibegin ) );

		// only do remote players
		if ( (player != NULL) && player->IsRemote() )
		{
			++playerCount;

			// mask membership bits for this player's frust(a)
			if ( MakeInt( goFrustum ) & MakeInt( i->m_OwnedMask ) )
			{
				addresses.push_back( player->GetMachineId() );
				++hitCount;
			}
		}
	}

	// reprocessed if differs
	return ( playerCount != hitCount );
}

FuBiCookie GoDb :: RSSetFocusGo( Goid goid )
{
	FUBI_RPC_TAG();

	// get valid go
	GoHandle go( goid );
	if ( !go )
	{
		return ( RPC_FAILURE_IGNORE );
	}
	gpassert( go->IsGlobalGo() );

	// see if our focus go is not changing - note that we only do this check
	// if we're a client because if there is a sync problem, the server may
	// think the client has the right focus yet client still trying to ask
	// server to set focus...
	if ( IsServerRemote() && (GetFocusGo( go->GetPlayerId() ) == goid) )
	{
		return ( RPC_SUCCESS );
	}

	// route call to server
	FUBI_RPC_CALL_RETRY( RSSetFocusGo, RPC_TO_SERVER );

	// ignore the request if this go has no frustum and it was from a client.
	// this is necessary to handle buffered frustum reassignments resulting in
	// sync difference between client and server. incoming focus requests will
	// need to only accept the valid frustum that is final. $ early bailout.
	FuBiCookie cookie = RPC_SUCCESS;
	if ( go->HasFrustum() || go->IsScreenPlayerOwned() )
	{
		// tell server to do it
		SetFocusGo( goid );

		// tell client to do it
		DWORD machineId = go->GetMachineId();
		if ( machineId != RPC_TO_LOCAL )
		{
			cookie = RCSetFocusGo( machineId, goid );
		}
	}
	return ( cookie );
}

FuBiCookie GoDb :: RCSetFocusGo( DWORD machineId, Goid goid )
{
	FUBI_RPC_CALL_RETRY( RCSetFocusGo, machineId );

	SetFocusGo( goid );
	return ( RPC_SUCCESS );
}

void GoDb :: SetFocusGo( Goid goid )
{
	// get player
	GoHandle go( goid );
	gpassert( go );
	gpassert( go->HasFrustum() );

	// clear the old one
	PlayerId playerId = go->GetPlayerId();
	ClearFocusGo( playerId );

	// get at player's frustum
	int index = ::MakeIndex( playerId );
	gpassert( index < (int)m_PlayerFrustumColl.size() );
	PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
	gpassert( frustum.m_Focus == frustum.m_Coll.end() );

	// find it
	PlayerFrustum::Coll::iterator i, ibegin = frustum.m_Coll.begin(), iend = frustum.m_Coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_Go == go.Get() )
		{
			frustum.m_Focus = i;
			break;
		}
	}
	gpassert( i != iend );

	// activate its world frustum
	BitFlagsSet( m_ActiveFrustumMask, frustum.m_Focus->m_FrustumId );

	// tell siege engine which is the screen active
	if ( go->GetPlayer()->IsScreenPlayer() )
	{
		gSiegeEngine.SetRenderFrustum( MakeInt( frustum.m_Focus->m_FrustumId ) );
	}

	// tell it
	WorldMessage( WE_FOCUSED, frustum.m_Focus->m_Go->GetGoid() ).Send();
}

FuBiCookie GoDb :: RSClearFocusGo( PlayerId playerId )
{
	// route call to server
	FUBI_RPC_CALL_RETRY( RSClearFocusGo, RPC_TO_SERVER );

	// get valid player
	Player* player = gServer.GetPlayer( playerId );
	gpassert( player != NULL );

	// execute it on the player's client
	return ( RCClearFocusGo( player->GetMachineId(), playerId ) );
}

FuBiCookie GoDb :: RCClearFocusGo( DWORD machineId, PlayerId playerId )
{
	FUBI_RPC_CALL_RETRY( RCClearFocusGo, machineId );

	ClearFocusGo( playerId );
	return ( RPC_SUCCESS );
}

void GoDb :: ClearFocusGo( PlayerId playerId )
{
	// check frustum index
	int index = ::MakeIndex( playerId );
	if ( index < (int)m_PlayerFrustumColl.size() )
	{
		// deselect old if any
		PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
		if ( frustum.m_Focus != frustum.m_Coll.end() )
		{
			gpassert( frustum.m_Focus->m_Go->IsFocused() );

			// deactivate its world frustum
			BitFlagsClear( m_ActiveFrustumMask, frustum.m_Focus->m_FrustumId );

			// tell it
			WorldMessage( WE_UNFOCUSED, frustum.m_Focus->m_Go->GetGoid() ).Send();

			// clear the focus
			frustum.m_Focus = frustum.m_Coll.end();
		}
	}
}

Goid GoDb :: GetFocusGo( PlayerId playerId ) const
{
	gpassert( IsValid( playerId ) );

	Goid goid = GOID_INVALID;

	int index = ::MakeIndex( playerId );
	if ( index < (int)m_PlayerFrustumColl.size() )
	{
		const PlayerFrustum& frustum = m_PlayerFrustumColl[ index ];
		if ( frustum.m_Coll.end() != frustum.m_Focus )
		{
			goid = frustum.m_Focus->m_Go->GetGoid();
			gpassert( frustum.m_Focus->m_Go->IsFocused() );
		}
	}

	return ( goid );
}

Goid GoDb :: GetFocusGo( void ) const
{
	return ( GetFocusGo( gServer.GetScreenPlayer()->GetId() ) );
}

void GoDb :: StartWatching( Goid watcher, Goid watched )
{
	GoHandle gowatcher( watcher );
	GoHandle gowatched( watched );

	if ( gowatcher && gowatched )
	{
		if (   (GetGoidClass( watcher ) == GO_CLASS_GLOBAL)
			&& (GetGoidClass( watched ) == GO_CLASS_GLOBAL) )
		{
			if ( watcher != watched )
			{
#				if !GP_RETAIL

				// do some db validation
				std::pair <WatchDb::iterator, WatchDb::iterator> rc = m_WatchingDb.equal_range( gowatcher.Get() );
				for ( ; rc.first != rc.second ; ++rc.first )
				{
					if ( rc.first->second == gowatched )
					{
						gperrorf(( "Serious error! Go is already in watch db!!\n"
								   "\n"
								   "watcher: goid = 0x%08X, scid = 0x%08X, template = '%s'\n"
								   "watched: goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
								   gowatcher->GetGoid(), gowatcher->GetScid(), gowatcher->GetTemplateName(),
								   gowatched->GetGoid(), gowatched->GetScid(), gowatched->GetTemplateName() ));
					}
				}

#				endif // !GP_RETAIL

				// add to db's
				m_WatchingDb.insert( WatchDb::value_type( gowatcher, gowatched ) );
				m_WatchedDb.insert( WatchDb::value_type( gowatched, gowatcher ) );
			}
			else
			{
				gperrorf(( "Serious error! Both 'watcher' and 'watched' (0x%08X) are the same!\n", watcher ));
			}
		}
		else
		{
			gperrorf(( "Serious error! A non-global Go is involved in a watch!\n"
					   "\n"
					   "watcher: goid = 0x%08X, scid = 0x%08X, template = '%s'\n"
					   "watched: goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
					   gowatcher->GetGoid(), gowatcher->GetScid(), gowatcher->GetTemplateName(),
					   gowatched->GetGoid(), gowatched->GetScid(), gowatched->GetTemplateName() ));
		}
	}
	else
	{
		gperrorf(( "Error: trying to have 0x%08X watch 0x%08X, but %s invalid\n",
				   watcher, watched, (!watcher && !watched) ? "both are" : (!watcher ? "'watcher' is" : "'watched' is") ));
	}
}

void GoDb :: StopWatching( Goid watcher, Goid watched )
{
	GoHandle gowatcher( watcher );
	GoHandle gowatched( watched );

	WatchDb::iterator found;
	bool deleted;

	// remove forward link
	deleted = false;
	for (  found = m_WatchingDb.find( gowatcher.Get() )
		 ; (found != m_WatchingDb.end()) && (found->first == gowatcher.Get())
		 ; ++found )
	{
		if ( found->second == gowatched )
		{
			m_WatchingDb.erase( found );
			deleted = true;
			break;
		}
	}
	gpassertm( deleted, "Go is not in the watching db" );

	// remove reverse link
	deleted = false;
	for ( found = m_WatchedDb.find( gowatched.Get() )
		 ; (found != m_WatchedDb.end()) && (found->first == gowatched.Get())
		 ; ++found )
	{
		if ( found->second == gowatcher )
		{
			m_WatchedDb.erase( found );
			deleted = true;
			break;
		}
	}
	gpassertm( deleted, "Go is not in the watched db" );
}

bool GoDb :: IsWatching( Goid watcher ) const
{
	GoHandle go( watcher );
	if ( go )
	{
		return ( m_WatchingDb.find( go.Get() ) != m_WatchingDb.end() );
	}
	else
	{
		return ( false );
	}
}

bool GoDb :: IsBeingWatched( Goid watched ) const
{
	GoHandle go( watched );
	if ( go )
	{
		return ( m_WatchedDb.find( go.Get() ) != m_WatchedDb.end() );
	}
	else
	{
		return ( false );
	}
}

int GoDb :: GetWatching( Goid watcher, GoidColl& coll ) const
{
	gpassert( watcher != GOID_INVALID );

	int oldSize = coll.size();

	std::pair <WatchDb::const_iterator, WatchDb::const_iterator> rc
			= m_WatchingDb.equal_range( GoHandle( watcher ).GetUnsafe() );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		coll.push_back( rc.first->second->GetGoid() );
	}

	return ( coll.size() - oldSize );
}

bool GoDb :: GetWatchingIters( Goid watcher, WatchDb::iterator& begin, WatchDb::iterator& end )
{
	gpassert( watcher != GOID_INVALID );

	std::pair <WatchDb::iterator, WatchDb::iterator> rc
			= m_WatchingDb.equal_range( GoHandle( watcher ).GetUnsafe() );
	begin = rc.first;
	end = rc.second;

	return ( begin != end );
}

int GoDb :: GetWatchedBy( Go* watched, GoidColl& coll ) const
{
	gpassert( watched != NULL );

	int oldSize = coll.size();

	std::pair <WatchDb::const_iterator, WatchDb::const_iterator> rc = m_WatchedDb.equal_range( watched );
	for ( ; rc.first != rc.second ; ++rc.first )
	{
		coll.push_back( rc.first->second->GetGoid() );
	}

	return ( coll.size() - oldSize );
}

bool GoDb :: GetWatchedByIters( Go* watched, WatchDb::iterator& begin, WatchDb::iterator& end )
{
	gpassert( watched != NULL );

	std::pair <WatchDb::iterator, WatchDb::iterator> rc = m_WatchedDb.equal_range( watched );
	begin = rc.first;
	end = rc.second;

	return ( begin != end );
}

void GoDb :: RegisterEffectScript( Goid goid, SFxSID scriptId, eWorldEvent type, bool xferOnSync, const Flamethrower_params* originalParams )
{
	gpassert( type != WE_LEFT_WORLD );
	gpassert( type != WE_DESTRUCTED );

	// just ignore these - we aren't interested in tracking them (blood etc.)
	// because we won't be stopping them when the go is destroyed.
	if ( (type == WE_DAMAGED) || (type == WE_EXPLODED) )
	{
		return;
	}

	// get the owner
	GoHandle go( goid );
	gpassert( go );

	// verify does not exist already
#	if GP_DEBUG
	std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( GoHandle( goid ).Get() );
	for ( ; effects.first != effects.second ; ++effects.first )
	{
		gpassert( effects.first->second.m_ScriptId != scriptId );
	}
#	endif // GP_DEBUG

	// safety check
	if ( !::IsGlobalSFxSID( scriptId ) && xferOnSync )
	{
		gperror( "Somebody is registering a local script with the 'xfer' flag, that's not going to work...\n" );
		xferOnSync = false;
	}

	// only bother with xfer in MP
	if ( ::IsSinglePlayer() )
	{
		xferOnSync = false;
	}

	// insert it
	EffectEntry& entry = m_EffectDb.insert( std::make_pair( go.Get(), EffectEntry( scriptId, type, go->IsInAnyWorldFrustum(), xferOnSync ) ) )->second;

	// special handling for xfer-ness
	if ( xferOnSync )
	{
		// set params separately so dtor of temporary does not delete it
		if ( (originalParams != NULL) && (originalParams->Count() > 0) )
		{
			entry.m_OriginalParams = new Flamethrower_params( *originalParams );
		}

		// need to generate links?
		if ( go->IsOmni() )
		{
			// get some vars
			Flamethrower_script* script = NULL;
			Flamethrower_target* target = NULL, * source = NULL;

			// find the script
			gpverify( gFlamethrower_interpreter.FindScript( scriptId, &script ) );

			// find its targets
			TattooTracker* tracker = script->GetDefaultTargets();
			gpverify( tracker->GetTarget( TattooTracker::TARGET, &target ) );
			gpverify( tracker->GetTarget( TattooTracker::SOURCE, &source ) );

			// set and link
			if ( target->GetType() == Flamethrower_target::DEFAULT )
			{
				GoHandle targetGo( (Goid)target->GetID() );
				if ( targetGo && !targetGo->IsOmni() )
				{
					entry.m_LinkedTarget = targetGo->GetGoid();
					m_EffectLinkDb.insert( std::make_pair( targetGo.Get(), go.Get() ) );
					targetGo->SetBucketDirty();
				}
			}
			if ( source->GetType() == Flamethrower_target::DEFAULT )
			{
				GoHandle sourceGo( (Goid)source->GetID() );
				if ( sourceGo && !sourceGo->IsOmni() && (sourceGo->GetGoid() != entry.m_LinkedTarget) )
				{
					entry.m_LinkedSource = sourceGo->GetGoid();
					m_EffectLinkDb.insert( std::make_pair( sourceGo.Get(), go.Get() ) );
					sourceGo->SetBucketDirty();
				}
			}
		}
		else
		{
			// if this can get xfer'd, we need to dirty the go
			go->SetBucketDirty();
		}
	}
}

void GoDb :: UnregisterEffectScript( Goid goid, SFxSID scriptId )
{
	GoHandle go( goid );
	if ( go )
	{
		std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go.Get() );
		for ( ; effects.first != effects.second ; ++effects.first )
		{
			if ( effects.first->second.m_ScriptId == scriptId )
			{
				EraseEffectEntry( effects.first );
				break;
			}
		}
	}
}

int GoDb :: StopEffectScripts( Go* go, eWorldEvent forType, eWorldEvent exceptType )
{
	gpassert( go != NULL );

	int stopped = 0;

	// stop them
	std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go );
	while ( effects.first != effects.second )
	{
		eWorldEvent script_type = effects.first->second.m_CreationEvent;
		if (   (((forType    == WE_INVALID) || (script_type == forType   )))
			&& (((exceptType == WE_INVALID) || (script_type != exceptType))) )
		{
			gpassert( script_type != WE_DAMAGED );
			gpassert( script_type != WE_EXPLODED );

			SFxSID id = effects.first->second.m_ScriptId;

			// if we're global, only the server can do the stoppage. unless
			// we're being told to stop all effects because we're deleting this
			// game object! that's important!!
			if ( IsGlobalSFxSID( id ) && ((forType != WE_INVALID) || (exceptType != WE_INVALID) || ::IsServerLocal()) )
			{
				if ( ::IsServerLocal() )
				{
					gWorldFx.SStopScript( id );
				}
			}
			else
			{
				gWorldFx.StopScript( id );
			}
			++stopped;

			EraseEffectEntry( effects.first );
		}
		else
		{
			++effects.first;
		}
	}

	return ( stopped );
}

int GoDb :: StopEffectScripts( Go* go, eWorldEvent forType )
{
	return ( StopEffectScripts( go, forType, WE_INVALID ) );
}

int GoDb :: StopEffectScriptsExcept( Go* go, eWorldEvent exceptType )
{
	return ( StopEffectScripts( go, WE_INVALID, exceptType ) );
}

int GoDb :: StopEffectScripts( Go* go )
{
	return ( StopEffectScripts( go, WE_INVALID, WE_INVALID ) );
}

void GoDb :: MessageEffectScripts( Go* go, const WorldMessage& msg )
{
	gpassert( go != NULL );

	// any broadcast messages should be skipped, otherwise flamethrower gets
	// flooded (and it shouldn't be paying attention to those messages anyway)
	if ( msg.IsBroadcast() )
	{
		return;
	}

	std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go );
	if ( (effects.first != effects.second) && IsFlamethrowerEvent( msg.GetEvent() ) )
	{
		gpstring event = ::ToString( msg.GetEvent() );

		do
		{
			gpassert( effects.first->second.m_Active || (gWorldState.GetCurrentState() == WS_RELOADING ) );
			gWorldFx.PostSignal( effects.first->second.m_ScriptId, event );
		}
		while ( ++effects.first != effects.second );
	}
}

int GoDb :: PauseEffectScripts( Go* go )
{
	gpassert( go != NULL );

	int paused = 0;

	std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go );
	for ( ; effects.first != effects.second ; ++effects.first, ++paused )
	{
		if ( effects.first->second.m_Active )
		{
			effects.first->second.m_Active = false;
			gWorldFx.UnloadScript( effects.first->second.m_ScriptId );
		}
	}

	return ( paused );
}

int GoDb :: ResumeEffectScripts( Go* go )
{
	gpassert( go != NULL );

	int resumed = 0;

	std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( go );
	for ( ; effects.first != effects.second ; ++effects.first, ++resumed )
	{
		if ( !effects.first->second.m_Active )
		{
			effects.first->second.m_Active = true;
			gWorldFx.LoadScript( effects.first->second.m_ScriptId );
		}
	}

	return ( resumed );
}

bool GoDb :: GetEffectScriptIters( Go* go, EffectDb::const_iterator& begin, EffectDb::const_iterator& end )
{
	gpassert( go != NULL );

	std::pair <EffectDb::const_iterator, EffectDb::const_iterator> rc = m_EffectDb.equal_range( go );
	begin = rc.first;
	end = rc.second;

	return ( begin != end );
}

bool GoDb :: HasRegisteredEffectScripts( Go* go )
{
	return (   (m_EffectDb    .find( go ) != m_EffectDb    .end())
			|| (m_EffectLinkDb.find( go ) != m_EffectLinkDb.end()) );
}

void GoDb :: RegisterEnchantment( Goid goid, const Enchantment& enchantment )
{
	// must be valid goid
	GoHandle go( goid );
	gpassert( go.IsValid() );
	go->m_IsEnchanted = true;

	// must serialize all of this in case threads are crossing in who enchants what
	RwCritical::WriteLock writeLock( m_EnchantmentCritical );

	// make sure we've got an entry
	std::pair <EnchantmentDb::iterator, bool> rc = m_EnchantmentDb.insert( EnchantmentDb::value_type( go, NULL ) );
	if ( rc.second )
	{
		rc.first->second = new EnchantmentStorage;
	}

	// now add the enchantment to the active set
	rc.first->second->DuplicateEnchantment( enchantment, goid );

	// mark go as dirty now that enchantment has been applied
	go->SetModifiersDirty();
}

void GoDb :: RemoveEnchantments( Goid goid, Goid item, bool immediate )
{
	gpassert( IsValid( goid ) );
	gpassert( (item == GOID_INVALID) || IsValid( item ) );

	CHECK_PRIMARY_THREAD_ONLY;

	// find enchantments
	EnchantmentDb::iterator found;
	{
		RwCritical::ReadLock readLock( m_EnchantmentCritical );
		found = m_EnchantmentDb.find( GoHandle( goid ).Get() );
	}

	// remove them
	if ( found != m_EnchantmentDb.end() )
	{
		// must serialize all of this in case threads are crossing in who enchants what
		RwCritical::WriteLock writeLock( m_EnchantmentCritical );

		if ( immediate )
		{
			EnchantmentStorage* storage = found->second;
			storage->RemoveEnchantments( item );

			// all gone?
			if ( !storage->HasEnchantments() )
			{
				delete ( storage );
				m_EnchantmentDb.erase( found );
			}
		}
		else
		{
			// just add to the queue - note that we still want to do the find()
			// above in case there aren't any enchantments registered for 'goid'
			m_EnchantmentRemoveQueue.push_back( goid );
			m_EnchantmentRemoveQueue.push_back( item );
		}
	}
}

bool GoDb :: GetEnchantment( Goid goid, const gpstring &sEnchantmentDescription, Enchantment **enchantment )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->GetEnchantment( sEnchantmentDescription, enchantment ) );
	}
	return ( false );
}

float GoDb :: GetEnchantmentTotal( Goid goid, const gpstring &sEnchantmentName )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->GetEnchantmentTotal( sEnchantmentName ) );
	}

	return ( 0.0f );
}

bool GoDb :: HasEnchantment( Goid goid, const gpstring &sEnchantmentDescription )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->HasEnchantment( sEnchantmentDescription ) );
	}

	return ( false );
}

bool GoDb :: HasEnchantments( Goid goid )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	return ( found != m_EnchantmentDb.end() );
}

bool GoDb :: HasEnchantmentNamed( Goid goid, const gpstring &sEnchantmentName )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->HasEnchantmentNamed( sEnchantmentName ) );
	}

	return ( false );
}

bool GoDb :: HasSingleInstanceEnchantment( Goid goid, const EnchantmentStorage& enchantments )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->HasSingleInstanceEnchantment( enchantments ) );
	}

	return ( false );
}

float GoDb :: GetLongestAlteration( Goid goid, Goid target, Goid source, Goid item )
{
	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		return ( found->second->GetLongestDuration( target, source, item, false ) );
	}

	return 0;
}

void GoDb :: SetAbortScriptID( Goid goid, const gpstring &sEnchantmentName, SFxSID id )
{
	CHECK_PRIMARY_THREAD_ONLY;

	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		found->second->SetAbortScriptID( sEnchantmentName, id );
	}
}

void GoDb :: SetEnchantmentDoneMessage( Goid goid, const gpstring &sEnchantmentName, Goid send_to, eWorldEvent event )
{
	CHECK_PRIMARY_THREAD_ONLY;

	RwCritical::ReadLock readLock( m_EnchantmentCritical );

	EnchantmentDb::iterator found = m_EnchantmentDb.find( GoHandle( goid ).GetUnsafe() );
	if ( found != m_EnchantmentDb.end() )
	{
		found->second->SetEnchantmentDoneMessage( sEnchantmentName, send_to, event );
	}
}

bool GoDb :: ApplyEnchantments( Go* go, bool forActor )
{
	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( go != NULL );

	EnchantmentStorage* storage = NULL;
	{
		RwCritical::ReadLock readLock( m_EnchantmentCritical );

		EnchantmentDb::iterator found = m_EnchantmentDb.find( go );
		if ( found != m_EnchantmentDb.end() )
		{
			storage = found->second;
		}
	}

	if ( storage != NULL )
	{
		storage->Update( gWorldTime.GetSecondsElapsed(), forActor );
		return ( true );
	}
	else
	{
		return ( false );
	}
}

Goid GoDb :: FindGoidByScid( Scid scid, bool* isRetired ) const
{
	RwCritical::ReadLock readLock( m_ScidCritical );
	Goid goid = GOID_INVALID;

	// look up scid in index first
	ScidIndex::const_iterator found = m_ScidIndex.find( scid );
	if ( found != m_ScidIndex.end() )
	{
		gpassert( !found->second->IsMarkedForDeletion() );
		goid = found->second->GetGoid();
	}
	else if ( isRetired != NULL )
	{
		// maybe it was retired?
		ScidColl::const_iterator found = m_RetiredScidColl.find( scid );
		*isRetired = found != m_RetiredScidColl.end();
	}

	// done
	return ( goid );
}

bool GoDb :: IsOnDeck( Goid goid ) const
{
	RwCritical::ReadLock readLock( m_OnDeckCritical );
	return ( m_OnDeckDb.find( goid ) != m_OnDeckDb.end() );
}

bool GoDb :: IsMarkedForDeletion( Goid goid ) const
{
	GoHandle go( goid );
	return ( go ? go->IsMarkedForDeletion() : false );
}

bool GoDb :: WasGoidRecentlyDeleted( Goid goid ) const
{
	gpassert( ::IsMultiPlayer() );
	return ( m_RecentDeletionRevDb.find( goid ) != m_RecentDeletionRevDb.end() );
}

void GoDb :: ForEachGlobalGo( ForEachGoCb cb )
{
	GoidColl coll;
	GetAllGlobalGoids( coll );

	GoidColl::iterator i, begin = coll.begin(), end = coll.end();
	for ( i = begin ; i != end ; ++i )
	{
		GoHandle go( *i );
		if ( go )
		{
			if ( !cb( go ) )
			{
				break;
			}
		}
	}
}

void GoDb :: ForEachLocalGo( ForEachGoCb cb, bool includeLodfi )
{
	GoidColl coll;
	GetAllLocalGoids( coll, includeLodfi );

	GoidColl::iterator i, begin = coll.begin(), end = coll.end();
	for ( i = begin ; i != end ; ++i )
	{
		GoHandle go( *i );
		if ( go )
		{
			if ( !cb( go ) )
			{
				break;
			}
		}
	}
}

int GoDb :: GetAllGlobalGoids( GoidColl& coll ) const
{
	RwCritical::ReadLock readLock( m_DbCritical );

	int oldSize = coll.size();

	GlobalConstIter i, begin = GlobalBegin(), end = GlobalEnd();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i.Get() != NULL )
		{
			coll.push_back( i->GetGoid() );
		}
	}

	return ( coll.size() - oldSize );
}

int GoDb :: GetAllLocalGoids( GoidColl& coll, bool includeLodfi ) const
{
	RwCritical::ReadLock readLock( m_DbCritical );

	int oldSize = coll.size();

	LocalDb::const_iterator i, begin = m_LocalDb.begin(), end = m_LocalDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (*i != NULL) && (includeLodfi || !(*i)->IsLodfi()) )
		{
			coll.push_back( (*i)->GetGoid() );
		}
	}

	return ( coll.size() - oldSize );
}

int GoDb :: GetAllCloneSrcGoids( GoidColl& coll ) const
{
	RwCritical::ReadLock readLock( m_DbCritical );

	int oldSize = coll.size();

	CloneSrcDb::const_iterator i, begin = m_CloneSrcDb.begin(), end = m_CloneSrcDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		coll.push_back( MakeCloneSrcGoid( i.GetIndex(), i.GetMagic() ) );
	}

	return ( coll.size() - oldSize );
}

bool GoDb :: CommitAllRequests( bool flushRpcs, bool bOnDeckOnly )
{
	GPPROFILERSAMPLE( "GoDb :: CommitAllRequests", SP_GODB );

	bool rc = false;

	if( bOnDeckOnly )
	{
		while ( CommitOnDeckDb() )
		{
			// keep looping forever until all requests flushed
			rc = true;
		}
	}
	else
	{
		while ( CommitOnDeckDb() | CommitDeleteRequests() | CommitPendingSiegeDb() )
		{
			// keep looping forever until all requests flushed
			rc = true;
		}
	}

	if ( flushRpcs )
	{
		if ( ::IsServerLocal() && GoRpcMgr::DoesSingletonExist() )
		{
			gGoRpcMgr.CommitPending();
		}

		// move all the delayed deletion requests over now
		if ( !m_DeleteReqDelayedColl.empty() )
		{
			gpassert( ::IsServerRemote() );
			m_DeleteReqColl.splice( m_DeleteReqColl.end(), m_DeleteReqDelayedColl );
		}
	}

	return ( rc );
}

void GoDb :: ClearScidSpecifics( void )
{
	m_RetiredScidColl.clear();
	m_ScidBitsColl.clear();
	gTriggerSys.ClearScidSpecifics();
}

void GoDb :: LockCloneSources( Go* go, bool lock )
{
	CHECK_DB_LOCKED;
	CHECK_PRIMARY_THREAD_ONLY;

	GoidColl goids;

	// gather up what we want to lock (do this in advance to avoid deadlocks)
	{
		RwCritical::ReadLock readLock( m_CloneSrcLockCritical );

		std::pair <GoGoidDb::iterator, GoGoidDb::iterator> rc = m_CloneSrcLockDb.equal_range( go );
		for ( ; rc.first != rc.second ; ++rc.first )
		{
			goids.push_back( rc.first->second );
		}
	}

	GoidColl::const_iterator i, ibegin = goids.begin(), iend = goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Goid goid = *i;

		// make sure it's in memory if we're locking
		if ( !lock )
		{
			GoHandle cloneGo( goid );
		}

		// grab a lock to mess with the db
		RwCritical::ReadLock readLock( m_DbCritical );

		// add/release a lock on it
		LocalGoidBits bits( goid );
		CloneBucket* bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );

		// watson bug 12888, 12880
		if_gpassert( bucket )
		{
			bucket->m_LockCount += lock ? 1 : -1;
			gpassert( bucket->m_LockCount >= 0 );
		}
	}
}

#if !GP_RETAIL

void GoDb :: AddDirtyGoBucket( bool add )
{
	CHECK_DB_LOCKED;
	m_GlobalDbDirtyCount += add ? 1 : -1;
}

void GoDb :: AddServerOnlyCount( bool add )
{
	CHECK_DB_LOCKED;
	m_GlobalDbServerOnlyCount += add ? 1 : -1;
}

#endif // !GP_RETAIL

void GoDb :: SSyncOnMachine ( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	FuBi::BitPacker packer;
	XferForSync( packer );
	RCSyncOnMachine( machineId, packer.GetSavedBits() );
}

void GoDb :: RCSyncOnMachine( DWORD machineId, const_mem_ptr ptr )
{
	FUBI_RPC_CALL( RCSyncOnMachine, machineId );

	FuBi::BitPacker packer( ptr );
	XferForSync( packer );
}

void GoDb :: XferForSync( FuBi::BitPacker& packer )
{

// Xfer rpc-able scid bits.

	if ( packer.IsSaving() )
	{
		ScidBitsColl::const_iterator i, ibegin = m_ScidBitsColl.begin(), iend = m_ScidBitsColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// only bother with nonzero things that must rpc (defaults work
			// otherwise)
			if ( (i->second.m_MustRpc) && (i->second.m_Bits != 0) )
			{
				packer.WriteBit( 1 );

				Scid scid = i->first;
				packer.XferRaw( scid );
				DWORD bits = i->second.m_Bits;
				packer.XferCount( bits );
			}
		}

		// terminate
		packer.WriteBit( 0 );
	}
	else
	{
		m_ScidBitsColl.clear();

		while ( packer.ReadBit() )
		{
			Scid scid;
			packer.XferRaw( scid );
			DWORD bits;
			packer.XferCount( bits );

			m_ScidBitsColl[ scid ] = ScidBits( bits, true );
		}
	}
}

void GoDb :: ClientSyncAllClientsGos( Player* player )
{
	// now it's time to tell this player about all the other all-clients go's
	// in the world (other players' parties and characters and all their inv)
	GoidColl globals;
	GopColl omnis, parties, characters, golds, stashes;	
	GetAllGlobalGoids( globals );
	{
		GoidColl::const_iterator i, ibegin = globals.begin(), iend = globals.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GoHandle go( *i );

			// find all all-clients go's that don't already belong to thisy
			// player and are not the go's used in the character chooser
			if (   (go->GetPlayer() != player)
				&& !go->IsServerOnly()
				&& (!go->HasPlacement() || !go->GetPlacement()->IsGuiGo()) )
			{
				if ( go->IsOmni() && !(go->HasStash() || (go->HasParent() && go->GetParent()->HasStash())) )
				{
					omnis.push_back( go );
				}
				else if ( go->IsAllClients() )
				{
					if ( go->HasParty() )
					{
						parties.push_back( go );
					}
					else if ( go->HasActor() )
					{
						characters.push_back( go );
					}
					else if ( go->HasStash() )
					{
						stashes.push_back( go );
					}
				}
			}
		}
	}

	// parties are simple, just create
	{
		GopColl::const_iterator i, ibegin = parties.begin(), iend = parties.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Go* go = *i;
			gpassert( go != NULL );
			gpassert( !go->IsOmni() );
			gpassert( !go->IsMarkedForDeletion() );
			gpassert( !go->HasParent() );

			// build our clone request
			GoCloneReq cloneReq;
			cloneReq.m_PlayerId = go->GetPlayer()->GetId();
			cloneReq.m_AllClients = true;
			GoPlacement* placement = go->QueryPlacement();
			if ( placement != NULL )
			{
				cloneReq.SetStartingPos( placement->GetPosition() );
				cloneReq.SetStartingOrient( placement->GetOrientation() );
			}

			// do the clone
			RCCloneGo( player->GetMachineId(), go->GetGoid(), go->GetRandomSeed(), cloneReq, go->GetTemplateName(), true );
		}
	}

	// characters are more complex, have to export/import
	if ( !characters.empty() )
	{
		gpstring spec;
	
		// create a jip block
		FuelHandle fuel( "::import:root" );
		fuel = fuel->CreateChildBlock( "xfer", "xfer" );

		// create fuel
		{
			// create output streams
			FuBi::FuelWriter writer( fuel );
			FuBi::PersistContext persist( &writer );

			// let's create a master block to house all the data we want to xfer
			// so we only have to perform one write when creating the specification to send over to the client.
			persist.EnterBlock( "all_clients_objects" );
			{
				// store go's by goid
				persist.EnterColl( "characters" );
				GopColl::const_iterator i, ibegin = characters.begin(), iend = characters.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					persist.AdvanceCollIter();

					Go* go = *i;
					gpassert( go != NULL );
					gpassert( !go->IsOmni() );
					gpassert( !go->IsMarkedForDeletion() );
					gpassert( go->HasParent() && go->GetParent()->HasParty() );

					// save goids
					Goid goid = go->GetGoid(), parent = go->HasParent() ? go->GetParent()->GetGoid() : GOID_INVALID;
					persist.Xfer( "goid", goid );
					persist.Xfer( "parent_goid", parent );

					// save template
					gpstring templateName = go->GetTemplateName();
					persist.Xfer( "template_name", templateName );

					// save player
					PlayerId player = go->GetPlayer()->GetId();
					persist.Xfer( "player", player );

					// export
					go->XferCharacter( persist );
				}
				persist.LeaveColl();

				// now xfer the stashes
				if ( !stashes.empty() )
				{
					persist.EnterColl( "stashes" );
					GopColl::const_iterator iStash;
					for ( iStash = stashes.begin(); iStash != stashes.end(); ++iStash )
					{
						persist.AdvanceCollIter();

						Go *go = *iStash;
						Goid goid = go->GetGoid(), parent = go->HasParent() ? go->GetParent()->GetGoid() : GOID_INVALID;
						persist.Xfer( "goid", goid );
						persist.Xfer( "parent_goid", parent );

						// save template
						gpstring templateName = go->GetTemplateName();
						persist.Xfer( "template_name", templateName );

						// save player
						PlayerId player = go->GetPlayer()->GetId();
						persist.Xfer( "player", player );

						// export
						go->XferCharacter( persist );
					}
					persist.LeaveColl();
				}					
			}
			persist.LeaveBlock();
		}

		// build the spec from the fuel			
		{
			FileSys::StringWriter writer( spec );
			fuel->GetChildBlock( "all_clients_objects" )->Write( writer );				
		}

		// not needed any more, clean it out
		fuel.Delete();				

		// now import those into the new client
		RCClientSyncAllClientsGos( player->GetMachineId(), spec );		

		// trade gold is simple too, just create
		{
			Server::PlayerColl::iterator i, ibegin = gServer.GetPlayersBegin(), iend = gServer.GetPlayersEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( IsValid(*i) && (*i != player) && !(*i)->IsComputerPlayer() )
				{
					GoHandle go( (*i)->GetTradeGold() );
					if ( go )
					{
						golds.push_back( go );
						RCSyncGo( player, go, true );
					}
				}
			}
		}

		// do an rpc sync to get everything else up to date
		{
			GopColl::const_iterator i, ibegin = characters.begin(), iend = characters.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				(*i)->RCAutoRpcSyncData( player->GetMachineId(), Go::SYNC_FULL );
			}
		}
	}

	// create the omni's, they just have to exist and that's it
	{
		GopColl::const_iterator i, ibegin = omnis.begin(), iend = omnis.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Go* go = *i;

			if ( stdx::has_any( golds, go ) )
			{
				continue;
			}

			gpassert( go->IsOmni() );
			gpassert( !go->IsMarkedForDeletion() );

			// build our clone request
			GoCloneReq cloneReq;
			cloneReq.m_Parent = go->HasParent() ? go->GetParent()->GetGoid() : GOID_INVALID;
			cloneReq.m_PlayerId = go->GetPlayer()->GetId();
			cloneReq.m_OmniGo = true;
			cloneReq.m_ForceClientAllowed = true;

			// do the clone
			RCCloneGo( player->GetMachineId(), go->GetGoid(), go->GetRandomSeed(), cloneReq, go->GetTemplateName(), true );
		}
	}
}

FuBiCookie GoDb :: RCClientSyncAllClientsGos( DWORD machineId, const gpstring& spec )
{
	FUBI_RPC_CALL_RETRY( RCClientSyncAllClientsGos, machineId );

	// create a jip block
	FuelHandle fuel( "::import:root" );
	fuel = fuel->CreateChildBlock( "xfer", "xfer" );

	// read fuel into new block
	fuel->AddChildren( spec, spec.length() );

	// read fuel
	{
		// create input streams
		FuBi::FuelReader reader( fuel );
		FuBi::PersistContext persist( &reader );

		persist.EnterBlock( "all_clients_objects" );
		{
			// read go's by goid
			persist.EnterColl( "characters" );
			while ( persist.AdvanceCollIter() )
			{
				// get goids
				Goid goid, parent;
				persist.Xfer( "goid", goid );
				persist.Xfer( "parent_goid", parent );

				// get template
				gpstring templateName;
				persist.Xfer( "template_name", templateName );

				// get player
				PlayerId player;
				persist.Xfer( "player", player );

				// build go
				GoCloneReq cloneReq;
				cloneReq.m_Parent = parent;
				cloneReq.m_PlayerId = player;
				cloneReq.m_AllClients = true;
				CloneGo( goid, cloneReq, templateName );

				// import
				GoHandle go( goid );
				if ( go )
				{
					go->XferCharacter( persist );
				}
			}
			persist.LeaveColl();

			persist.EnterColl( "stashes" );
			while ( persist.AdvanceCollIter() )
			{
				// get goids
				Goid goid, parent;
				persist.Xfer( "goid", goid );
				persist.Xfer( "parent_goid", parent );

				// get template
				gpstring templateName;
				persist.Xfer( "template_name", templateName );

				// get player
				PlayerId player;
				persist.Xfer( "player", player );

				// build go
				GoCloneReq cloneReq;
				cloneReq.m_Parent = parent;
				cloneReq.m_PlayerId = player;
				cloneReq.m_AllClients = true;
				cloneReq.m_OmniGo = true;
				CloneGo( goid, cloneReq, templateName );

				// import
				GoHandle go( goid );
				if ( go )
				{
					go->XferCharacter( persist );
				}
			}
			persist.LeaveColl();
		}
		persist.LeaveBlock();
	}

	// not needed any more, clean it out
	fuel.Delete();

	return ( RPC_SUCCESS );
}

#if !GP_RETAIL

void GoDb :: RCVerifyGoBucket( const MachineAddrQuery& query, Goid startGoid, bool standalone )
{
	// additional verification to try to catch pcontent sync issues
	if ( ::IsMultiPlayer() && !standalone && (query.m_RemotePlayers[ 0 ] != NULL) )
	{
		DWORD verifyGoid = 0;
		DWORD verifySeed = 0;
		{
			RwCritical::ReadLock readLock( m_DbCritical );

			GlobalGoidBits bits( startGoid );
			GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
			if_gpassert( bucket != NULL )
			{
				bits.m_MinorIndex = bucket->m_Count;
				verifyGoid = bits.m_Handle;
				verifySeed = bucket->AddCRC32FromHandles( startGoid );
			}
		}

		if ( verifyGoid != 0 )
		{
			typedef const Player* CPlayer;

			const CPlayer* i, * ibegin = query.m_RemotePlayers, * iend = ARRAY_END( query.m_RemotePlayers );
			for ( i = ibegin ; (i != iend) && (*i != NULL) ; ++i )
			{
				FUBI_SET_RPC_PEEKADDRESS( (*i)->GetMachineId() );
				RCVerifyGoBucket( verifyGoid, verifySeed );
			}
		}
	}
}

static void AppendGoInfo( gpstring& out, int index, Go* go, bool isOnDeck )
{
	if ( index != -1 )
	{
		out.appendf( "%d = ", index );
	}

	if ( go != (Go*)0xFAABD00D )
	{
		if ( go != NULL )
		{
			const char* templateName = go->GetTemplateName();
			out.appendf(
					"goid = 0x%08X, scid = 0x%08X, created at %g, template = %s (%s)\n",
					go->GetGoid(), go->GetScid(), go->GetCreateTime(),
					templateName ? templateName : "<NULL??>",
					isOnDeck ? "ON-DECK" : "IN-BUCKET" );
		}
		else
		{
			out.append( "<NULL>\n" );
		}
	}
	else
	{
		out.appendf( "0xFAABD00D <DEFAULT GOBUCKET 'FIRST'>\n" );
	}
}

static void AppendGoInfo( gpstring& out, Go* go, bool isOnDeck )
{
	AppendGoInfo( out, -1, go, isOnDeck );
}

static void AppendGoInfo( gpstring& out, Go* go )
{
	AppendGoInfo( out, go, gGoDb.IsOnDeck( go->GetGoid() ) );
}

static gpstring AppendGoInfo( Go* go )
{
	gpstring out;
	AppendGoInfo( out, go );
	return ( out );
}

void GoDb :: RCVerifyGoBucket( DWORD verifyGoid, DWORD verifyCrc )
{
	// $ note: we're using DWORD for verifyGoid so the gorpc stuff does not
	//   pick it apart and complain ('cause it's always going to be invalid).

	FUBI_RPC_CALL_PEEKADDRESS( RCVerifyGoBucket );
	CHECK_PRIMARY_THREAD_ONLY;

	gpstring error;

	GlobalGoidBits bits( verifyGoid );
	gpassert( bits.m_Class == GO_CLASS_GLOBAL );

	{
		RwCritical::ReadLock readLock( m_DbCritical );

		// find bucket
		GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
		if ( bucket != NULL )
		{
			// minorindex of incoming bits holds the count of items that ought to be
			// in this bucket.
			if ( (int)bits.m_MinorIndex != bucket->m_Count )
			{
				error.appendf( "Count of Go's in the bucket does not match! Expecting %d, found %d\n",
							   (int)bits.m_MinorIndex, bucket->m_Count );
			}

			// check crc too
			DWORD localCrc = bucket->AddCRC32FromHandles( (Goid)verifyGoid );
			if ( verifyCrc != localCrc )
			{
				error.appendf( "CRC of Go's in the bucket does not match! Expecting 0x%08X, found 0x%08X\n",
							   verifyCrc, localCrc );
			}
		}
		else
		{
			UINT magic = 0;
			if ( m_GlobalDb.GetMagic( bits.m_MajorIndex, magic ) )
			{
				error.appendf( "Bucket is mismatched! Magic requested = 0x%08X, magic that exists = 0x%08X\n",
							   bits.m_Magic, magic );
			}
			else
			{
				error.append( "Bucket does not exist!\n" );
			}
		}
	}

	if ( !error.empty() )
	{
		gpstring contents;
		AppendBucketContents( bits, contents );
		gperrorf(( "TELL SCOTT that you just repro'd #11777. The host told a "
				   "client (me) to create or clone a Go, then sent a "
				   "verification packet which failed! Please report this "
				   "occurrance in RAID with dslog files. The error "
				   "follows:\n\n%s\n%s", error.c_str(), contents.c_str() ));
	}
}

void GoDb :: AppendBucketContents( Goid goid, gpstring& appendHere )
{
	GlobalGoidBits bits( goid );

	appendHere.appendf( "Contents of existing bucket 0x%04X:\n\n", (int)bits.m_MajorIndex );

	GoBucket oldBucket;
	UINT magic = 0;
	bool success = true;

	{
		RwCritical::ReadLock readLock( m_DbCritical );
		success = m_GlobalDb.GetMagic( bits.m_MajorIndex, magic );
		if ( success )
		{
			oldBucket = m_GlobalDb.UnsafeDereference( bits.m_MajorIndex );
		}
		else
		{
			appendHere.append( "Index is not valid!!!\n" );
		}
	}

	if ( success )
	{
		RwCritical::ReadLock readLock( m_OnDeckCritical );

		bits.m_MinorIndex = 0;

		appendHere.appendf( "count = %d, magic = 0x%08X\n", oldBucket.m_Count, magic );
		OnDeckDb::const_iterator found = m_OnDeckDb.find( bits );
		::AppendGoInfo( appendHere, 0, (found != m_OnDeckDb.end()) ? found->second : oldBucket.m_First, found != m_OnDeckDb.end() );

		GopColl::const_iterator i, ibegin = oldBucket.m_Remaining.begin(), iend = oldBucket.m_Remaining.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			++bits.m_MinorIndex;
			OnDeckDb::const_iterator found = m_OnDeckDb.find( bits );
			::AppendGoInfo( appendHere, i - ibegin + 1, (found != m_OnDeckDb.end()) ? found->second : *i, found != m_OnDeckDb.end() );
		}
	}
}

#endif // !GP_RETAIL

Go** GoDb :: DerefGoPtr( Goid goid )
{
	// $ important: this function is NOT thread-safe for globals! it returns
	//   a pointer to a go pointer, which may get realloc'd if the caller does
	//   not lock properly.

	Go** gop = NULL;

	// look up slot
	switch ( GetGoidClass( goid ) )
	{
		case ( GO_CLASS_GLOBAL ):
		{
			GlobalGoidBits bits( goid );
			GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
			if ( bucket != NULL )
			{
				UINT minorIndex = bits.m_MinorIndex;
				if ( minorIndex == 0 )
				{
					gop = &bucket->m_First;
				}
				else
				{
					--minorIndex;
					if ( minorIndex < bucket->m_Remaining.size() )
					{
						gop = &bucket->m_Remaining[ minorIndex ];
					}
				}
			}
		}
		break;

		case ( GO_CLASS_LOCAL ):
		{
			LocalGoidBits bits( goid );
			gop = m_LocalDb.Dereference( bits.m_Index, bits.m_Magic );
		}
		break;

		case ( GO_CLASS_CLONE_SRC ):
		{
			LocalGoidBits bits( goid );
			CloneBucket* bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );

			// check for real goid
			if ( bucket != NULL )
			{
				bool reacquire = true;

				// do on-demand recreation
				if ( bucket->m_Go == NULL )
				{
					gpassert( !IsLockedForSaveLoad() );

					// temporarily release our old lock and grab a write lock
					// instead. also grab the create lock early to avoid a deadlock.
					m_DbCritical.LeaveRead();
					m_CreateLock.Enter();
					m_DbCritical.EnterWrite();

					// reacquire the bucket - it may have shifted in between the
					// prev two statements.
					bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );

					// is it still null? somebody could have deref'd the goid in
					// between those two statements.
					bool create = bucket->m_Go == NULL;
					if ( create )
					{
						// recreate it
						CreateCloneSrcGo( goid, bucket, NULL );
					}

					// restore locks
					m_DbCritical.LeaveWrite();
					m_CreateLock.Leave();
					m_DbCritical.EnterRead();

					// was it already there? just do it again
					if ( !create )
					{
						return ( DerefGoPtr( goid ) );
					}
				}
				else if ( bucket->m_Go->IsExpiring() && !bucket->m_Go->IsMarkedForDeletion() && IsPrimaryThread() )
				{
					// it's been touched, cancel it from expiring if it's going
					// but only if we're the primary thread!
					CancelExpiration( bucket->m_Go );
				}
				else
				{
					reacquire = false;
				}

				// re-acquire bucket in case it shifted during last operation
				if ( reacquire )
				{
					bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );
				}

				// get the pointer
				gop = &bucket->m_Go;
			}
		}
		break;
	}

	return ( gop );
}

Go* GoDb :: DerefGo( Goid goid )
{
	Go* go = NULL;

	// quick check on invalid
	if ( goid != GOID_INVALID )
	{
		// lock
		m_DbCritical.EnterRead();

		// deref the goid to get a slot
		Go** gop = DerefGoPtr( goid );
		bool hadSlot = gop != NULL;
		if ( hadSlot )
		{
			go = *gop;
		}

		// release
		m_DbCritical.LeaveRead();

		// valid goid if it had a slot
		if ( hadSlot )
		{
			// a null slot means the go is on deck
			if ( go == NULL )
			{
				RwCritical::ReadLock readLock( m_OnDeckCritical );

				// check on-deck - may not be there if committed just now
				OnDeckDb::const_iterator found = m_OnDeckDb.find( goid );
				if ( found != m_OnDeckDb.end() )
				{
					go = found->second;
				}
				else
				{
					// check the temp collection, maybe it's being committed
					found = m_TempOnDeckDb.find( goid );
					if ( found != m_TempOnDeckDb.end() )
					{
						go = found->second;
					}
				}
			}

			// validate it's the right one! note that if we're loading, it will be invalid
			gpassert( !go || (go->m_Goid == GOID_INVALID) || (go->GetGoid() == goid) );
		}
	}

	return ( go );
}

void GoDb :: DeleteGo( Go* go, bool destructMsg, bool failedGo )
{
	gpassert( go != NULL );
	gpassert( !IsOnDeck( go->GetGoid() ) );
	GPSTATS_SAMPLE( AddGoDealloc() );

	Goid goid = go->GetGoid();

	// record it
	gplog( OutputGoDeletion( go ) );

	// $ these are handled automatically:
	//
	// m_SelectionColl    : WE_DESTRUCTED in Go will have Go deselect itself
	// m_PlayerFrustumColl: upon deselecting, the Go will also automatically lose focus and upon deletion/party loss the frustum will get destroyed

	// remove from hud
#	if !GP_RETAIL
	HudInfoDb::iterator hudFound = m_HudInfoDb.find( go );
	if ( hudFound != m_HudInfoDb.end() )
	{
		m_HudInfoDb.erase( hudFound );
	}
#	endif // !GP_RETAIL

	// remove if found from these db's
	stdx::erase_if_found( m_VisibleItemsDb, go );

	// stop any effects running on this go
	StopEffectScripts( go );

	// remove anything in the linked db
	{
		std::pair <GoGoDb::iterator, GoGoDb::iterator> targets = m_EffectLinkDb.equal_range( go );
		for ( ; targets.first != targets.second ; ++targets.first )
		{
			std::pair <EffectDb::iterator, EffectDb::iterator> effects = m_EffectDb.equal_range( targets.first->second );
			for ( ; effects.first != effects.second ; ++effects.first )
			{
				EffectEntry& entry = effects.first->second;
				if ( entry.m_LinkedTarget == goid )
				{
					entry.m_LinkedTarget = GOID_INVALID;
				}
				if ( entry.m_LinkedSource == goid )
				{
					entry.m_LinkedSource = GOID_INVALID;
				}
			}
		}
	}

	// verify they all got removed
	gpassert( !HasRegisteredEffectScripts( go ) );

	// remove any inactive simulations that this go may have associated with it
	gSim.RemoveSimulation( goid );

	// remove any siegefx variables that this go may have been using
	gWorldFx.ClearVariables( goid );

	// remove any quest bits for this guy
	{
		QuestBitsDb::iterator found = m_QuestBitsDb.find( goid );
		if ( found != m_QuestBitsDb.end() )
		{
			m_QuestBitsDb.erase( found );
		}
	}

	// nuke enchantments
	{
		RwCritical::WriteLock writeLock( m_EnchantmentCritical );

		// remove enchantment that may be applied
		EnchantmentDb::iterator enchantments = m_EnchantmentDb.find( go );
		if ( enchantments != m_EnchantmentDb.end() )
		{
			delete ( enchantments->second );
			m_EnchantmentDb.erase( enchantments );
		}

		// remove any pending remove-all requests
		for (  GoidColl::iterator i = m_EnchantmentRemoveQueue.begin()
			 ; i != m_EnchantmentRemoveQueue.end() ; )
		{
			if ( *i == goid )
			{
				i = m_EnchantmentRemoveQueue.erase( i, i + 2 );
			}
			else if ( (*i + 1) == goid )
			{
				RemoveEnchantments( *i, *(i + 1), true );
				i = m_EnchantmentRemoveQueue.erase( i, i + 2 );
			}
			else
			{
				i += 2;
			}
		}
	}

	// kill all ui collections
	go->Deselect();					// $ also kills focus
	if ( go->IsMouseShadowed() )
	{
		gpassert( stdx::has_any( m_MouseShadowColl, go ) );
		m_MouseShadowColl.erase( stdx::find( m_MouseShadowColl, go ) );
	}
	if ( go->IsInHotGroup() )
	{
		m_HotColl.erase( stdx::find( m_HotColl, go ) );
	}

	// placement-related infoz
	if ( go->HasPlacement() )
	{
		const siege::database_guid& nodeGuid = go->GetPlacement()->GetPosition().node;

		// if pending entry, remove its entry in the db
		if ( go->IsPendingEntry() )
		{
			// remove entry from pending
			SiegeGopCollDb::iterator found = m_PendingGoDb.find( nodeGuid );
			if ( found != m_PendingGoDb.end() )
			{
				GopColl& coll = found->second;
				GopColl::iterator entry = stdx::find( coll, go );
				if ( entry != coll.end() )
				{
					coll.erase( entry );

					// if that killed the entry, then remove the coll
					GopColl::iterator i, ibegin = coll.begin(), iend = coll.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( *i != NULL )
						{
							break;
						}
					}
					if ( i == iend )
					{
						m_PendingGoDb.erase( found );
					}
				}
			}
		}
		else
		{
			// lock and change
			{
				RECURSE_LOCK_FOR_WRITE( SiegeGoDb );

				// kill its node occupancy
				SiegeGoDb::iterator found = m_SiegeGoDb.find( nodeGuid );
				if ( found != m_SiegeGoDb.end() )
				{
					// iter till we find our occupant or go past the end
					do
					{
						// remove it
						if ( found->second == go )
						{
							m_SiegeGoDb.erase( found );
							break;
						}

						// advance
						++found;
					}
					while( (found != m_SiegeGoDb.end()) && (found->first == nodeGuid) );
				}
			}

			if ( go->IsGlobalGo() )
			{
				// Inform the trigger system that this object is no longer in existence
				gTriggerSys.UpdateTriggersWhenGoIsDestroyed( go );
			}

			// remove it from the world
			go->LeaveWorld( true );
			go->SetFrustumMembership( FRUSTUMID_INVALID );
			if ( (nodeGuid != siege::UNDEFINED_GUID) && go->IsGlobalGo() )
			{
				gAIQuery.OnOccupantChanged( go, nodeGuid, siege::UNDEFINED_GUID, true );
			}
		}
	}

	gpassert( go->AssertValid() );

	// safety check
#	if AIQUERY_UBER_PARANOIA
	if ( gAIQuery.HasGoInCache( go ) )
	{
		gpbitchslap_aiq( "Go detected in AIQ paranoia cache after it's been deleted!!", go );
	}
#	endif // AIQUERY_UBER_PARANOIA

#	if GP_DEBUG
	if (   (GetGoidClass( goid ) == GO_CLASS_GLOBAL)
		|| (GetGoidClass( goid ) == GO_CLASS_LOCAL ) )
	{
		gpassert( !go->HasPlacement() || go->IsOmni() || !IsNodeOccupant( go, go->GetPlacement()->GetPosition().node ) );
	}
#	endif // GP_DEBUG

	// notify non-clones of impending doom
	if (   (GetGoidClass( goid ) == GO_CLASS_GLOBAL)
		|| (GetGoidClass( goid ) == GO_CLASS_LOCAL ) )
	{
		// send delete message to the go if it wasn't a failed attempt
		if ( !failedGo && destructMsg && !m_IsShuttingDownBad )
		{
			WorldMessage( WE_DESTRUCTED, goid, goid, MakeInt( goid ) ).Send();
		}
	}

	{
		// remove everybody that this go is watching
		std::pair <WatchDb::iterator, WatchDb::iterator> rc;
		for ( rc = m_WatchingDb.equal_range( go ) ; rc.first != rc.second ; )
		{
			Go* watched = rc.first->second;
			++rc.first;		// $ increment the iter now (StopWatching will erase it)
			StopWatching( go->GetGoid(), watched->GetGoid() );
		}

		// tell everybody that is watching this go to stop watching it
		for ( rc = m_WatchedDb.equal_range( go ) ; rc.first != rc.second ; )
		{
			Go* watcher = rc.first->second;
			++rc.first;		// $ increment the iter now (StopWatching will erase it)
			StopWatching( watcher->GetGoid(), go->GetGoid() );
		}
	}

	{
		// remove any entries from the lock db
		RwCritical::WriteLock writeLock( m_CloneSrcLockCritical );
		std::pair <GoGoidDb::iterator, GoGoidDb::iterator> rc = m_CloneSrcLockDb.equal_range( go );
		m_CloneSrcLockDb.erase( rc.first, rc.second );
	}

	// trigger bits don't need to hang around on expired go's (unlike scid bits)
	Scid scid = go->GetScid();
	if ( ::IsInstance( scid, false ) && (m_RetiredScidColl.find( scid ) != m_RetiredScidColl.end()) )
	{
		gTriggerSys.RetireScid( scid );
	}

	// should not be in scid index
#	if GP_DEBUG
	{
		RwCritical::ReadLock readLock( m_ScidCritical );
		ScidIndex::iterator found = m_ScidIndex.find( go->GetScid() );
		gpassert( (found == m_ScidIndex.end()) || (found->second != go) )
	}
#	endif

	// tell the Go to shut down quickly
	go->Shutdown( false );

	// update stats tracking
#	if !GP_RETAIL
	if ( go->IsBucketDirty() )
	{
		go->m_IsBucketDirty = false;
		if ( go->IsGlobalGo() )
		{
			--m_GlobalDbDirtyCount;
			gpassert( m_GlobalDbDirtyCount >= 0 );
		}
	}
	if ( go->IsServerOnly() )
	{
		--m_GlobalDbServerOnlyCount;
		gpassert( m_GlobalDbServerOnlyCount >= 0 );
	}
#	endif // !GP_RETAIL

	// process custom db
	{
		// custom db deletion
		switch ( GetGoidClass( goid ) )
		{
			case ( GO_CLASS_GLOBAL ):
			{
				FreeGlobalGoid( goid, failedGo ? NULL : go );
			}
			break;

			case ( GO_CLASS_LOCAL ):
			{
				RwCritical::WriteLock writeLock( m_DbCritical );

				// easy, just kill it
				LocalGoidBits bits( goid );
				m_LocalDb.Free( bits.m_Index, NULL );
			}
			break;

			case ( GO_CLASS_CLONE_SRC ):
			{
				// only free the goid if we're really deleting it and not just
				// expiring it for on-demand reloading.
				if ( !go->IsExpiring() )
				{
					FreeCloneSrcGoid( goid );
				}
				else
				{
					RwCritical::WriteLock dbLock( m_CloneNameCritical );
					RwCritical::WriteLock writeLock( m_DbCritical );

					// just null out the go pointer
					LocalGoidBits bits( goid );
					CloneBucket* bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );
					bucket->m_Go = NULL;
					++m_CloneSrcUnloadedCount;
				}
			}
			break;

			default:
			{
				gpassert( 0 );		// $ should never get here
			}
		}

		// goid slot has been freed, clear it out
		go->m_Goid = GOID_INVALID;
	}

	// add to recently deleted db in mp games
	AddRecentDeletion( goid );

	// finally, delete the go
	GPDEBUG_ONLY( go->m_DbgInGoDb = false; )
	delete ( go );
}

void GoDb :: DeleteFailedGo( Goid startGoid )
{
	// only bother to free if something was constructed
	bool freedInitial = !m_Adder.IsActive();

	// use the construction list to kill go's
	GopColl::const_iterator i, begin = m_GoCreateColl.begin(), end = m_GoCreateColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		// it should be sitting in the on-deck set, remove it from there
		{
			RwCritical::WriteLock writeLock( m_OnDeckCritical );

			OnDeckDb::iterator found = m_OnDeckDb.find( (*i)->GetGoid() );
			if_gpassert( found != m_OnDeckDb.end() )
			{
				m_OnDeckDb.erase( found );
			}
		}

		// check to see if this DeleteGo() call will free the initial goid
		if ( (*i)->GetGoid() == startGoid )
		{
			freedInitial = true;
		}

		// delete the go
		DeleteGo( *i, false, true );
	}

	// free the initial goid if it hasn't been already
	if ( !freedInitial )
	{
		FreeGlobalGoid( startGoid );
	}

	// now check to make sure that the entire bucket was freed
	gpassert( m_GlobalDb.Dereference( GlobalGoidBits( startGoid ).m_MajorIndex,
									  GlobalGoidBits( startGoid ).m_Magic ) == NULL );
}

void GoDb :: FreeGlobalGoid( Goid goid, Go* go )
{
	RwCritical::WriteLock writeLock( m_DbCritical );

	// find proper bucket
	GlobalGoidBits bits( goid );
	GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
	if_not_gpassert( bucket != NULL )
	{
		// $ have to just abort here to avoid crashing. it's still an error
		//   condition though!!
		return;
	}

	// remove from the bucket
	UINT minorIndex = bits.m_MinorIndex;
	if ( minorIndex == 0 )
	{
		gpassert( (go == NULL) || (bucket->m_First == go) );
		bucket->m_First = NULL;
	}
	else
	{
		--minorIndex;
		gpassert( minorIndex < bucket->m_Remaining.size() );
		gpassert( (go == NULL) || (bucket->m_Remaining[ minorIndex ] == go) );
		bucket->m_Remaining[ minorIndex ] = NULL;
	}

	// possibly remove bucket
	gpassert( bucket->m_Count > 0 );
	if ( --bucket->m_Count == 0 )
	{
		// clear it out
		bucket->m_Remaining.destroy();

		// notify db owner
		m_GlobalDb.Free( bits.m_MajorIndex );
	}

	// update count
	--m_GlobalDbCount;
}

void GoDb :: FreeCloneSrcGoid( Goid goid )
{
	RwCritical::WriteLock dbLock( m_CloneNameCritical );
	RwCritical::WriteLock writeLock( m_DbCritical );

	// remove from the clone name db first (slow but not frequent)
	CloneNameDb::iterator i, begin = m_CloneNameDb.begin(), end = m_CloneNameDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->second == goid )
		{
			m_CloneNameDb.erase( i );
			break;
		}
	}
	gpassert( i != end );

	// now kill it
	LocalGoidBits bits( goid );
	m_CloneSrcDb.Free( bits.m_Index );
}

bool GoDb :: CommitOnDeckDb( void )
{
	// $ early bailout to avoid construction of map and serial lock
	if ( m_OnDeckDb.empty() )
	{
		return ( false );
	}

	OnDeckDb active;
	bool didAnyCommits = false;

	// loop until no more to do
	for ( ; ; )
	{
		// swap the on-deck collection
		{
			RwCritical::WriteLock writeLock( m_OnDeckCritical );
			if ( m_OnDeckDb.empty() )
			{
				break;
			}

			// grab it
			active.clear();
			active.swap( m_OnDeckDb );

			// special: there is a rare chance that the subobjects (inventory,
			// usually) of a go are in the on-deck collection, but the root go
			// is still committing creation (i.e. it's still in the temp-on-deck
			// db). so keep those subobjects in the on-deck db until the root is
			// all done.
			OnDeckDb::const_iterator i, ibegin = m_TempOnDeckDb.begin(), iend = m_TempOnDeckDb.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				GlobalGoidBits root( i->first );
				if ( (root.m_Class == GO_CLASS_GLOBAL) && (root.m_MinorIndex == 0) )
				{
					// just lsearch, it won't be huge
					for ( OnDeckDb::iterator j = active.begin() ; j != active.end() ; )
					{
						if ( GlobalGoidBits( j->first ).m_MajorIndex == root.m_MajorIndex )
						{
							// found one - move it back
							m_OnDeckDb.insert( *j );
							j = active.erase( j );
						}
						else
						{
							++j;
						}
					}
				}
			}

			// hey guess what - if all we have is stuff that got stuffed back
			// into m_OnDeckDb then we're going to end up spinning here until
			// the whole damn thing is finished committing. this can easily
			// happen when streaming in a store with lots of stuff in it.
			if ( active.empty() )
			{
				break;
			}
		}

		// we're doing some stuff here
		didAnyCommits = true;

		// make sure it's going to be valid!
#		if !GP_RETAIL
		{
			OnDeckDb::iterator i, begin = active.begin(), end = active.end();
			for ( i = begin ; i != end ; ++i )
			{
				Go* go = i->second;
				if ( !go->IsOmni() && go->IsPendingEntry() && go->HasPlacement() )
				{
					if ( !go->GetPlacement()->GetPosition().node.IsValid() )
					{
						gperrorf(( "Convention violated! Go '%s' is entering the world but does not have a valid node set!\n", go->GetTemplateName() ));
					}
				}
			}
		}
#		endif // !GP_RETAIL

		// for each on-deck go
		OnDeckDb::iterator i, begin = active.begin(), end = active.end();
		if ( begin != end )
		{
			// lock all collections for committal
			RwCritical::ReadLock readLock( m_DbCritical );

			// commit all on-deck go's into the godb
			for ( i = begin ; i != end ; ++i )
			{
				// find the slot
				Go** gop = DerefGoPtr( i->first );
				if ( (gop != NULL) && (*gop == NULL) )
				{
					// assign it
					*gop = i->second;
				}
				else
				{
					gplog( OutputGoInfo( "RAID#9580 On-Deck Go", i->second ) );
					if ( gop != NULL )
					{
						gplog( OutputGoInfo( "RAID#9580 Existing Go", *gop ) );
					}
					gperrorf(( "TELL SCOTT that you just repro'd #9580. We're "
							   "trying to commit a Go into the GoDb but when I "
							   "looked up the slot to put it in, it was either "
							   "already in use or nonexistent! Here is "
							   "relevant data to append to the RAID report "
							   "(and as always please post your .dslog files!) "
							   "NOTE: this game is now UNSTABLE and will "
							   "probably die soon!!!\n"
							   "\n"
							   "goid     = 0x%08X (addr = 0x%08X)\n"
							   "scid     = 0x%08X\n"
							   "template = '%s'\n"
							   "existing = %s\n",
							   i->first, i->second,
							   i->second->m_Scid,
							   i->second->GetTemplateName(),
							   (gop != NULL)
							   		? gpstringf( "0x%08X (%s)", (*gop)->m_Goid, (*gop)->GetTemplateName() ).c_str()
							   		: "<null>" ));
				}
			}
		}

		// final setup
		if ( !m_IsShuttingDownBad )
		{
			GopColl enteringWorld;
			for ( i = begin ; i != end ; ++i )
			{
				Goid goid = i->first;
				Go* go = i->second;

				// now tell all the go's they've been constructed
				WorldMessage( WE_CONSTRUCTED, GOID_INVALID, goid, MakeInt( goid ) ).Send();

				// queue it for entry in the world
				if ( go->HasPlacement() && !go->IsOmni() )
				{
					// see if we can put it in the world now
					const siege::database_guid& nodeGuid = go->GetPlacement()->GetPosition().node;

#					if !GP_RETAIL
					static bool s_IsDebugging = SysInfo::IsDebuggerPresent();
					if ( !nodeGuid.IsValid() )
					{
						// $ special: we want to catch this thing RIGHT NOW to
						//   see what the other thread is doing that it's got
						//   the handle outstanding. so just force a
						//   breakpoint immediately if we're in the debugger.
						//   that will catch it before this thread loses its
						//   time slice...
						if ( s_IsDebugging )
						{
							BREAKPOINT();
						}
						gperrorf(( "TELL SCOTT: A game object 0x%08X scid 0x%08X template '%s' is being put in the world but there's no node yet!!\n\n",
								   goid, go->GetScid(), go->GetTemplateName() ));
					}
#					endif // !GP_RETAIL

					if ( gSiegeEngine.IsNodeInAnyFrustum( nodeGuid ) )
					{
						// yes!
						if ( IsGlobalGoid( goid ) && IsServerLocal() && IsMultiPlayer() )
						{
							enteringWorld.push_back( go );
						}
						else
						{
							NewGoEnterWorld( go );
						}
					}
					else if ( IsServerLocal() )
					{
						gpgolifef(( "GoDb::CommitOnDeckDb( go = 0x%08X (%s) ) - added to pending godb at node %s\n",
									go->GetGoid(), go->GetTemplateName(), nodeGuid.ToString().c_str() ));

						// deferred
						m_PendingGoDb[ nodeGuid ].push_back( go );
					}
					else
					{
						// we're the client, do what the server says
						NewGoEnterWorld( go );
					}
				}
			}

			// now sort and do the force-enters. these have to be sorted
			// for sync so we have parents enter before children etc.
			if ( !enteringWorld.empty() )
			{
				SortForCreation( enteringWorld );

				GopColl::const_iterator j, jbegin = enteringWorld.begin(), jend = enteringWorld.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					NewGoEnterWorld( *j );
				}
			}
		}
	}

	// done
	return ( didAnyCommits );
}

bool GoDb :: CommitDeleteRequests( bool doCloneSourcesNow )
{
	bool committed = false;
	GopList cloneSrcDeletions;

	while ( !m_DeleteReqColl.empty() )
	{
		// clear out any pending commits - we cannot have any global/local go's
		// sitting in the commit set that are about to be deleted.
		CommitOnDeckDb();

		// delete front to back
		Go* go = m_DeleteReqColl.front();
		if ( doCloneSourcesNow || !go->IsCloneSourceGo() )
		{
			// pull it from the list
			m_DeleteReqColl.pop_front();
			gpassert( go->IsMarkedForDeletion() );

			// delete it
			DeleteGo( go );
			committed = true;
		}
		else
		{
			// move clone source delete requests to the other list
			cloneSrcDeletions.splice( cloneSrcDeletions.end(), m_DeleteReqColl, m_DeleteReqColl.begin() );
		}
	}

	// throw back our clone sources into the delete req
	m_DeleteReqColl.splice( m_DeleteReqColl.end(), cloneSrcDeletions );

	// if nothing happening in the loader, flush any clone source deletions
	if (  (!gSiegeLoadMgr.AreOrdersPending() || !gSiegeLoadMgr.IsLoadingThreadRunning())
		&& !doCloneSourcesNow && !m_DeleteReqColl.empty() )
	{
		// ok grab a critical to prevent any more orders from being processed
		// while we kill our clone sources in peace. see, this is necessary
		// because second thread frequently derefs clone sources via inventory
		// creation and if we're whacking one while it tries to use it, all hell
		// breaks loose. note that we don't update 'committed' here because
		// we don't need to iterate until empty at a higher level, 'cause they
		// are just being cached out, no big deal.
		kerneltool::Critical::Lock locker( gSiegeLoadMgr.GetProcessCritical() );
		CommitDeleteRequests( true );
	}
	else if ( !m_RecentDeletionColl.empty() )
	{
		// here's a good place to flush out recent deletions
		double retireTime = PreciseSubtract( gWorldTime.GetTime(), m_DeletionRetireTime );
		while ( !m_RecentDeletionColl.empty() && m_RecentDeletionColl.front().first < retireTime )
		{
			TimeGoidReverseDb::iterator found = m_RecentDeletionRevDb.find( m_RecentDeletionColl.front().second );
			gpassert( found != m_RecentDeletionRevDb.end() );
			m_RecentDeletionRevDb.erase( found );
			m_RecentDeletionColl.pop_front();
		}
	}

	return ( committed );
}

bool GoDb :: CommitPendingSiegeDb( void )
{
	bool committed = false;
	GopColl enteringWorld;

	for ( SiegeGopCollDb::iterator i = m_PendingGoDb.begin() ; i != m_PendingGoDb.end() ; )
	{
		bool erase = false;

		// see if we can put it in the world now
		siege::SiegeNode* node = gSiegeEngine.IsNodeValid( i->first );
		if ( (node != NULL) && node->IsOwnedByAnyFrustum() )
		{
			erase = true;

			// yes!
			GopColl::iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				Go* go = *j;
				if ( go != NULL )
				{
					if ( !go->IsCommittingCreation() )
					{
						if ( go->IsGlobalGo() && IsServerLocal() && IsMultiPlayer() )
						{
							enteringWorld.push_back( go );
						}
						else
						{
							NewGoEnterWorld( go );
						}

						committed = true;
						*j = NULL;
					}
					else
					{
						// damn it's still being worked on...
						erase = false;
					}
				}
			}
		}
		else if ( IsServerLocal() )
		{
			erase = true;

			// node no longer exists, we probably moved away
			for ( GopColl::iterator j = i->second.begin() ; j != i->second.end() ; ++j )
			{
				Go* go = *j;
				if ( go != NULL )
				{
					if ( go->IsCommittingCreation() )
					{
						// have to skip it for a bit...
						erase = false;
					}
					else if ( go->IsExpiring() )
					{
						// this thing was expiring already. that means it has
						// entered the world via some other means, most likely it
						// was inserted there via a forced expiration death & decay
						// path. so just force it onto that node.
						if ( go->IsGlobalGo() && IsServerLocal() && IsMultiPlayer() )
						{
							enteringWorld.push_back( go );
						}
						else
						{
							NewGoEnterWorld( go );
						}

						committed = true;
						*j = NULL;
					}
					else
					{
						// just delete it. we never saw it, it's still untouched.
						SMarkForDeletion( go, false );
						committed = true;
						*j = NULL;
					}
				}
			}
		}

		// how to advance?
		if ( erase )
		{
			i = m_PendingGoDb.erase( i );
			committed = true;
		}
		else
		{
			++i;
		}
	}

	// now sort and do the force-enters. these have to be sorted for sync so we
	// have parents enter before children etc.
	if ( !enteringWorld.empty() )
	{
		SortForCreation( enteringWorld );

		GopColl::const_iterator j, jbegin = enteringWorld.begin(), jend = enteringWorld.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			NewGoEnterWorld( *j );
		}
	}

	return ( committed );
}

void GoDb :: CheckFocusChange( void )
{
	// default focus is the party
	Go* focus = NULL;

	// find what should focus
	GopColl::iterator i, ibegin = m_SelectionColl.begin(), iend = m_SelectionColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// use first available selection item that has a frustum
		if ( (*i)->HasFrustum() && (*i)->IsScreenPlayerOwned() )
		{
			focus = *i;
			break;
		}
	}

	// if null, get the first frustum available from children
	if ( focus == NULL )
	{
		Go* party = gServer.GetScreenParty();
		if ( party != NULL )
		{
			// use party member closest to party object
			float closestDist = FLT_MAX;
			GopColl::iterator i, ibegin = party->GetChildBegin(), iend = party->GetChildEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// does it share a frustum?
				Go* child = *i;
				if (   (child != NULL)
					&& child->HasFrustum()
					&& (MakeInt( child->GetWorldFrustumMembership() ) & MakeInt( party->GetWorldFrustumMembership() )) )
				{
					// is it closest?
					vector_3 dist = party->GetPlacement()->GetWorldPosition()
								  - child->GetPlacement()->GetWorldPosition();
					float newDist = dist.Length();
					if ( newDist < closestDist )
					{
						focus = child;
						closestDist = newDist;
					}
				}
			}

			// use first available party member that has a frustum, period
			if ( focus == NULL )
			{
				for ( i = ibegin ; i != iend ; ++i )
				{
					Go* child = *i;
					if ( (child != NULL) && (child)->HasFrustum() )
					{
						focus = child;
						break;
					}
				}
			}

			// default to party
			if ( focus == NULL )
			{
				focus = party;
			}
		}
	}

	FrustumId maskBefore = gGoDb.GetActiveFrustumMask();

	// was it focused?
	if ( (focus != NULL) && focus->HasFrustum() )
	{
		RSSetFocusGo( focus->GetGoid() );
	}
	else
	{
		RSClearFocusGo( focus ? focus->GetPlayerId() : gServer.GetScreenPlayer()->GetId() );
	}

	if( maskBefore != gGoDb.GetActiveFrustumMask() )
	{
		WorldMessage( WE_FRUSTUM_ACTIVE_STATE_CHANGED, GOID_INVALID, GOID_ANY, MakeInt( maskBefore ) ).Send();
	}
}

void GoDb :: NewGoEnterWorld( Go* go )
{
	gpassert( go != NULL );

	// no longer pending entry
	go->m_IsPendingEntry = false;

	// enter the world
	AddNodeOccupant( go, go->GetPlacement()->GetPosition().node );

	// after construction, if it's an item and not in someone's inventory,
	// pretend it was just dropped on the ground.
	if ( go->HasGui() && !go->HasParent() && (GetGoidClass( go->GetGoid() ) == GO_CLASS_GLOBAL) )
	{
		WorldMessage( WE_DROPPED, go->GetGoid() ).SendDelayed();
	}
}

void GoDb :: ClientSyncGos( FrustumId oldMembership, FrustumId newMembership, Go* go, std::pair <SiegeGoDb::iterator, SiegeGoDb::iterator>* range )
{
	// one or the other, but not both
	gpassert( (go != NULL) != (range != NULL) );

	// valid range or no range
	gpassert( (range == NULL) || (range->first != range->second) );

	// $ early bailout if not multiplayer
	if ( ::IsSinglePlayer() )
	{
		return;
	}

	// $ early bailout if no change (this is possible if force-expiring a go that it outside the world)
	if ( oldMembership == newMembership )
	{
		return;
	}

	// $ early bailout if syncing a local go
	if ( (go != NULL) && (GetGoidClass( go->GetGoid() ) != GO_CLASS_GLOBAL) )
	{
		return;
	}

	// machines that need deletions go here
	typedef std::set <DWORD> MachineColl;
	MachineColl deleteMachineColl;

	// get some vars
	GopColl gos;
	GoidSet deleteGos;

	// normal processing for when we're not loading a map
	if ( !IsLoading( gWorldState.GetCurrentState() ) )
	{
		// loop over all our players
		PlayerFrustumColl::iterator i, ibegin = m_PlayerFrustumColl.begin(), iend = m_PlayerFrustumColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Player* player = gServer.GetPlayer( MakePlayerIdFromIndex( i - ibegin ) );

			// only do remote players
			if ( IsValid( player ) && player->IsRemote() )
			{
				// mask membership bits for this player's frust(a)
				bool inOld = !!(MakeInt( oldMembership ) & MakeInt( i->m_OwnedMask ));
				bool inNew = !!(MakeInt( newMembership ) & MakeInt( i->m_OwnedMask ));

				// changed?
				if ( inOld != inNew )
				{
					bool anyDeleted = false;
					if ( go != NULL )
					{
						// single go
						if ( !ClientSyncGo( go, player, inNew ) )
						{
							// $ note: only bother to insert go if this is the
							//   first time through this loop (optz)
							if ( deleteMachineColl.empty() )
							{
								deleteGos.insert( go->GetGoid() );
							}
							anyDeleted = true;
						}
					}
					else if ( !inNew )
					{
						// they're going away, this is a faster loop
						for ( SiegeGoDb::iterator j = range->first ; j != range->second ; ++j )
						{
							if ( !ClientSyncGo( j->second, player, inNew ) )
							{
								if ( deleteMachineColl.empty() )
								{
									deleteGos.insert( j->second->GetGoid() );
								}
								anyDeleted = true;
							}
						}
					}
					else if ( FetchGlobalSyncGos( gos, range ) )
					{
						// multiple go's
						GopColl::iterator j, jbegin = gos.begin(), jend = gos.end();
						for ( j = jbegin ; j != jend ; ++j )
						{
							if ( !ClientSyncGo( *j, player, inNew ) )
							{
								if ( deleteMachineColl.empty() )
								{
									deleteGos.insert( (*j)->GetGoid() );
								}
								anyDeleted = true;
							}
						}
					}

					// add if needed
					if ( anyDeleted )
					{
						deleteMachineColl.insert( player->GetMachineId() );
					}
				}
			}
		}
	}
	else
	{
		// $ this special code must run when we are loading a map because we
		//   have not placed the heroes yet, so m_PlayerFrustumColl is not set
		//   up yet, and the above code will fail to do anything useful.

		// loop over all our players
		Server::PlayerColl::const_iterator i, ibegin = gServer.GetPlayers().begin(), iend = gServer.GetPlayers().end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// added to a remote player?
			if ( IsValid( *i ) && (*i)->IsRemote() )
			{
				// mask membership bits for this player's frust(a). need to do this in case
				// we're getting called while load due to a player getting his frustum destroyed
				// on dropped connection. see #12366.
				bool inOld = !!(MakeInt( oldMembership ) & MakeInt( (*i)->GetInitialFrustum() ));
				bool inNew = !!(MakeInt( newMembership ) & MakeInt( (*i)->GetInitialFrustum() ));

				if ( !inOld && inNew )
				{
					if ( go != NULL )
					{
						// single go
						gpverify( ClientSyncGo( go, *i, true ) );
					}
					else
					{
						// loop through all go's
						for ( SiegeGoDb::iterator j = range->first ; j != range->second ; ++j )
						{
							gpverify( ClientSyncGo( j->second, *i, true ) );
						}
					}
				}
			}
		}
	}

	// mondo deletion: this is good for whacking stores on clients
	if ( !deleteGos.empty() )
	{
		gpassert( !deleteMachineColl.empty() );

		GoDbMondoDeleteXfer xfer;
		std::copy( deleteGos.begin(), deleteGos.end(), std::back_inserter( xfer.m_Goids ) );

		FuBi::BitPacker packer;
		xfer.Xfer( packer );

		MachineColl::const_iterator i, ibegin = deleteMachineColl.begin(), iend = deleteMachineColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			FUBI_SET_RPC_PEEKADDRESS( *i );
			RCMarkForMondoDeletionPacker( packer.GetSavedBits() );
		}
	}
}

bool GoDb :: ClientSyncGo( Go* go, Player* player, bool enter )
{
	CHECK_SERVER_ONLY;

	gpassert( (go != NULL) && (player != NULL) );
	gpassert( !(go->IsOmni() && (go->HasParent() ? !go->GetParent()->IsOmni() : true)) );

	// $ early bailout if not multiplayer
	if ( ::IsSinglePlayer() )
	{
		return ( true );
	}

	// $ early bailout if syncing a local go
	Goid goid( go->GetGoid() );
	GlobalGoidBits bits( goid );
	if ( bits.m_Class != GO_CLASS_GLOBAL )
	{
		return ( true );
	}

	// on enter/leave world
	bool shouldKeep = true;
	if ( enter )
	{
		if ( go->IsAllClients() )
		{
			// sync any xfer-able fx that are running on this guy for the client
			if ( HasRegisteredEffectScripts( go ) || go->HasFollower() )
			{
				go->RCAutoRpcSyncData( player->GetMachineId(), Go::SYNC_FOR_ALL_CLIENTS );
			}
		}
		else if ( !go->IsMarkedForDeletion() && !go->IsServerOnly() )
		{
			enum eCreateType
			{
				CREATE_NONE,				// don't create it, taken care of separately
				CREATE_NORMAL,				// create it normally (clone template, create scid)
				CREATE_STANDALONE,			// create it, but skip creation of subobjects
			};
			eCreateType createType = CREATE_NORMAL;

			RwCritical::ReadLock readLock( m_DbCritical );

			// find the base object
			GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
			bool baseDirty = false;
			if ( bucket->m_First == NULL )
			{
				// see if our current object is the base
				if ( bits.m_MinorIndex == 0 )
				{
					baseDirty = go->IsBucketDirty();
				}
				else if ( !m_Adder.IsActive() || (GlobalGoidBits( m_Adder.GetCurrentGoid() ).m_MajorIndex != bits.m_MajorIndex) )
				{
					// we're not in the middle of constructing a subobject, so
					// we really do have a null first slot in the bucket. that
					// means the object was deleted, so it's dirty.
					baseDirty = true;
				}
			}
			else
			{
				baseDirty = bucket->m_First->IsBucketDirty();
			}

			// detect creation type
			if ( bits.m_MinorIndex == 0 )
			{
				// if base is dirty, subobjects must be sync'd separately. note
				// that the bucket may be empty if we're still constructing the
				// object and it has not been committed yet.
				gpassert( (bucket->m_First == NULL) || (bucket->m_First == go) );
				if ( baseDirty )
				{
					createType = CREATE_STANDALONE;
				}
			}
			else if ( baseDirty )
			{
				// base is dirty, so this object is managed independently
				createType = CREATE_STANDALONE;
			}
			else
			{
				// base is not dirty, so this object will be created when base is
				createType = CREATE_NONE;
			}

			// do creation
			if ( createType != CREATE_NONE )
			{
				RCSyncGo( player, go, createType == CREATE_STANDALONE );
			}
		}
	}
	else if ( !go->IsAllClients() && !go->IsServerOnly() )
	{
		// simple - always delete it if not an all-clients go
		shouldKeep = false;
	}

	// done
	return ( shouldKeep );
}

Goid GoDb :: CreateOrLoadGo( const GoCreateReq* createReq, const GoCloneReq* cloneReq, FastFuelHandle fuel )
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	CHECK_DB_LOCKED;

	// verify properly formed request
	gpassert( (createReq == NULL) || createReq->AssertValid() );
	gpassert( (cloneReq  == NULL) || cloneReq ->AssertValid() );
	gpassert( (createReq != NULL) || (cloneReq != NULL) );

	// verify we don't already have this scid
	if ( (createReq != NULL) && HasGoWithScid( createReq->m_Scid ) )
	{
		GoHandle oldGo( FindGoidByScid( createReq->m_Scid ) );
		gperrorf(( "SERIOUS ERROR: I was told to create a new Go with a Scid "
				   "that already exists! This is bad news indeed, and to fix "
				   "this bug we're going to need dslogs for all machines "
				   "involved in this game, please.\n"
				   "\n"
				   "GoCreateReq contents:\n"
				   "\n"
				   "%s\n"
				   "\n"
				   "Existing Go info:\n"
				   "\n"
				   "%s\n",
				   createReq->RpcToString().c_str(),
				   oldGo ? AppendGoInfo( oldGo ).c_str() : "<SCID -> GO LOOKUP FAILED!!>" ));
		if ( oldGo )
		{
			gplog( OutputGoInfo( "RAID#11777 Duplicate entering ScidIndex Go", oldGo ) );
		}
	}

	// acquire creation lock
	GoAdder::Lock locker( (createReq && createReq->m_LocalGo) ? GO_CLASS_LOCAL : GO_CLASS_GLOBAL );

	// get the next goid
	Goid goid = m_Adder.AssignNextGoid();
	if ( goid == GOID_INVALID )
	{
		return ( GOID_INVALID );
	}

	// create new go
	std::auto_ptr <Go> newGo;
	if ( createReq != NULL )
	{
		stdx::make_auto_ptr( newGo, gContentDb.CreateGo( *createReq ) );
		if ( newGo.get() == NULL )
		{
			// failed - damn.
			return ( GOID_INVALID );
		}
	}
	else
	{
		// get the player
		Player* player = cloneReq->GetPlayer();
		if ( player == NULL )
		{
			gperrorf(( "Illegal player attempted to create a game object (id = 0x%08X)\n",
					   cloneReq->m_PlayerId ));
			return ( GOID_INVALID );
		}

		// create the Go
		stdx::make_auto_ptr( newGo, new Go( player ) );

		// load it up
		if ( !newGo->LoadFromInstance( fuel, SCID_SPAWNED, false ) )
		{
			gperrorf(( "Failure to load game object from fuel address '%s'\n", fuel.GetAddress().c_str() ));
			return ( GOID_INVALID );
		}
	}

	// add to the creation collection
	if ( !createReq || !createReq->m_LocalGo )
	{
		m_GoCreateColl.push_back( newGo.get() );
	}

	// need to set its lodfi state before it commits
	newGo->m_IsLodfi = (createReq != NULL) && createReq->m_LodfiGo;

	// add to temp db
	AddToTempOnDeckDb( newGo.get(), goid );

	// commit the go with new goid
	bool success = newGo->CommitCreation( goid, createReq ? createReq->GetMpPlayerCount() : cloneReq->GetMpPlayerCount() );
	GPDEBUG_ONLY( newGo->m_DbgInGoDb = success; )

	// finish if successful
	if ( success )
	{
		if ( createReq != NULL )
		{
			createReq->Finish( newGo.get() );
		}
		else if ( cloneReq != NULL )
		{
			cloneReq->Finish( newGo.get() );
		}
	}

	// add to scid index on success, reset scid on failure
	if ( createReq != NULL )
	{
		if ( success )
		{
			Scid scid = newGo->GetScid();

			// verify no duplicates
			gpassert( scid == createReq->m_Scid );
			gpassert( !HasGoWithScid( scid ) );

			// add it
			RwCritical::WriteLock writeLock( m_ScidCritical );
			m_ScidIndex[ scid ] = newGo.get();
			gpassert( m_RetiredScidColl.find( scid ) == m_RetiredScidColl.end() );
		}
		else
		{
			// just in case...
			newGo->m_Scid = SCID_INVALID;
		}
	}

	// update its gold value on success - use pcontent rules for this
	if ( success )
	{
		gPContentDb.CalcAndApplyPContentRules( newGo.get() );
	}

	// if it's supposed to draw right away, set the flag
	if ( (createReq && createReq->m_PrepareToDrawNow) || (cloneReq && cloneReq->m_PrepareToDrawNow) )
	{
		newGo->PrepareToDrawNow();
	}

	// finish the request
	if ( success && cloneReq != NULL )
	{
		cloneReq->FinishPost( newGo.get() );
	}

	// record it
	gplog( OutputGoCreation( newGo.get() ) );

	// add it to the on deck set and let it go (even if failed to commit)
	AddToOnDeckDb( newGo.get(), goid );

	// all done - if failed, note that caller will destroy the bad go
	newGo.release();
	return ( success ? goid : GOID_INVALID );
}

Goid GoDb :: CreateCloneSrcGo( Goid goid, CloneBucket* bucket, const char* templateName )
{
	// get some vars
	DWORD randomSeed = GetGoCreateRng().GetSeed();
	DWORD fixedSeed = (::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader);
	gpstring localTemplateName;

	// set up template name
	if ( templateName == NULL )
	{
		gpassert( bucket != NULL );
		gpassert( goid != GOID_INVALID );
		localTemplateName = bucket->m_TemplateName;
	}
	else
	{
		gpassert( bucket == NULL );
		gpassert( goid == GOID_INVALID );
		localTemplateName = templateName;
	}

	// create new one
	std::auto_ptr <Go> newGo( new Go( gServer.GetComputerPlayer() ) );
	if ( newGo->LoadFromTemplate( localTemplateName, false, false ) )
	{
		// verify it's clone-ready
		gpassert( !IsInstance( newGo->GetScid() ) );

		// add to clone src db
		if ( bucket == NULL )
		{
			RwCritical::WriteLock writeLock( m_DbCritical );

			UINT index, magic;
			m_CloneSrcDb.Alloc( bucket, index, magic );
			goid = MakeCloneSrcGoid( index, magic );
			bucket->m_Go = NULL;
			bucket->m_TemplateName = localTemplateName;
			bucket->m_LockCount = 0;
			++m_CloneSrcUnloadedCount;
		}

		// build goid for new entry
		LocalGoidBits bits( goid );
		if ( newGo->CommitCreation( goid ) )
		{
			// it's ready
			GPDEBUG_ONLY( newGo->m_DbgInGoDb = true; )

			// re-acquire bucket, as there may have been realloc
			RwCritical::ReadLock readLock( m_DbCritical );
			bucket = m_CloneSrcDb.Dereference( bits.m_Index, bits.m_Magic );

			// set it up
			bucket->m_Go = newGo.release();
			--m_CloneSrcUnloadedCount;
			gpassert( m_LockedForSaveLoad || (m_CloneSrcUnloadedCount >= 0) );

			// record it
			gplog( OutputGoCreation( newGo.get() ) );
		}
		else
		{
			RwCritical::WriteLock writeLock( m_DbCritical );
			m_CloneSrcDb.Free( bits.m_Index );
			goid = GOID_INVALID;
		}

		// add to name index
		if ( (goid != GOID_INVALID) && (templateName != NULL) )
		{
			m_CloneNameDb.insert( CloneNameDb::value_type( localTemplateName, goid ) );
		}
	}
	else
	{
		// fail
		gperrorf(( "Unable to create source game object from template '%s' for cloning\n", localTemplateName.c_str() ));
		goid = GOID_INVALID;
	}

	// restore seed that may have been messed up by clone
	GetGoCreateRng().SetSeed( randomSeed );
	(::IsPrimaryThread() ? m_MostRecentFixedSeedMain : m_MostRecentFixedSeedLoader) = fixedSeed;

	// done
	return ( goid );
}

void GoDb :: FinishGoCreation( Goid goid )
{
	// $ ok so the idea here is to say that now that we've finished creating
	//   this brand new go, it's "sealed", and any changes will invalidate the
	//   go and its parents. then, when syncing go's on the clients, if there
	//   are any dirty go's in the bucket, we can't do the full deterministic
	//   sync by creating a go by template/scid. instead, we must sync each
	//   subobject individually. i don't want to hear that this solution is
	//   fucked up. i know. i spent all last night trying to come up with
	//   something better and nothing is easy. impossible problem #3715 solved.

	// if this was a global go, clear all dirty bits on go's in its bucket
	GlobalGoidBits bits( goid );
	if ( bits.m_Class == GO_CLASS_GLOBAL )
	{
		RwCritical::ReadLock readLock( m_DbCritical );

		// get at bucket
		GoBucket* bucket = m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
		if ( bucket != NULL )
		{
			if ( bucket->m_First != NULL )
			{
				bucket->m_First->SetBucketDirty( false );
			}

			GopColl::iterator i, ibegin = bucket->m_Remaining.begin(), iend = bucket->m_Remaining.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( *i != NULL )
				{
					(*i)->SetBucketDirty( false );
				}
			}
		}
	}
}

void GoDb :: AddToTempOnDeckDb( Go* go, Goid goid )
{
	CHECK_DB_LOCKED;

	// $ the purpose of the "temp' on deck db is to allow GoHandle deref's to
	//   resolve by going to a separate db for the lookup. can't just put the
	//   go into the on-deck db before it commits because it might get picked
	//   up by the primary thread and put into the main db before it has a
	//   chance to commit. so it goes into temp instead, just for the duration
	//   of the commit. and the deref checks there as well.

	RwCritical::WriteLock writeLock( m_OnDeckCritical );

	// mark that it's committing immediately to ward off deletions
	go->m_IsCommittingCreation = true;

	// check that it's not already there, then add it
	gpassert( m_TempOnDeckDb.find( goid ) == m_TempOnDeckDb.end() );
	m_TempOnDeckDb[ goid ] = go;
}

void GoDb :: AddToOnDeckDb( Go* go, Goid goid )
{
	CHECK_DB_LOCKED;

	// first see if there is something in the slot in the bucket already
#	if GP_DEBUG
	{
		RwCritical::ReadLock readLock( m_DbCritical );

		GlobalGoidBits bits( goid );
		if ( bits.m_Class == GO_CLASS_GLOBAL )
		{
			if_gpassert( m_GlobalDb.IsValidIndex( bits.m_MajorIndex ) )
			{
				GoBucket& bucket = m_GlobalDb.UnsafeDereference( bits.m_MajorIndex );
				if ( bits.m_MinorIndex == 0 )
				{
					gpassert( bucket.m_First == NULL );
				}
				else
				{
					gpassert( bits.m_MinorIndex <= bucket.m_Remaining.size() );
					gpassert( bucket.m_Remaining[ bits.m_MinorIndex - 1 ] == NULL );
				}
			}
		}
	}
#	endif // GP_DEBUG

	RwCritical::WriteLock writeLock( m_OnDeckCritical );

	// check that it's not already in the collection
	gpassert( m_OnDeckDb.find( goid ) == m_OnDeckDb.end() );

	// add it
	m_OnDeckDb[ goid ] = go;

	// clear temporary now that it's on deck
	OnDeckDb::iterator found = m_TempOnDeckDb.find( goid );
	if_gpassert( found != m_TempOnDeckDb.end() )
	{
		// no idea why it wouldn't be here, but we have to ship this thing. #12645.
		m_TempOnDeckDb.erase( found );
	}

	// done committing now
	go->m_IsCommittingCreation = false;
}

void GoDb :: AddRecentDeletion( Goid goid )
{
	if ( ::IsServerRemote() && (GetGoidClass( goid ) == GO_CLASS_GLOBAL) )
	{
		TimeGoidColl::iterator newEntry = m_RecentDeletionColl.insert( m_RecentDeletionColl.end(), std::make_pair( gWorldTime.GetTime(), goid ) );
		std::pair <TimeGoidReverseDb::iterator, bool> rc
				= m_RecentDeletionRevDb.insert( std::make_pair( goid, newEntry ) );
		if ( !rc.second )
		{
			TimeGoidColl::iterator& oldEntry = rc.first->second;
			gpassert( oldEntry != newEntry );

			// duplicate - rare case, but ok. just update the time stamp and
			// delete the old one
			m_RecentDeletionColl.erase( oldEntry );
			oldEntry = newEntry;
		}
		gpassert( m_RecentDeletionColl.size() == m_RecentDeletionRevDb.size() );
	}
}

void GoDb :: RebuildCreationCaches( void )
{
	m_CreateGoXferCache.clear();
	m_CreateGoXferCache.resize( Server::MAX_PLAYERS );
	m_CloneGoXferCache.clear();
	m_CloneGoXferCache.resize( Server::MAX_PLAYERS );
}

void GoDb :: EraseEffectEntry( GoDb::EffectDb::iterator& i )
{
	Go* owner = i->first;
	EffectEntry& entry = i->second;

	if ( entry.m_LinkedTarget != GOID_INVALID )
	{
		EraseLinkedEffects( GoHandle( entry.m_LinkedTarget ), owner );
	}
	if ( entry.m_LinkedSource != GOID_INVALID )
	{
		EraseLinkedEffects( GoHandle( entry.m_LinkedSource ), owner );
	}

	i = m_EffectDb.erase( i );
}

void GoDb :: EraseLinkedEffects( Go* target, Go* owner )
{
	std::pair <GoGoDb::iterator, GoGoDb::iterator> targets = m_EffectLinkDb.equal_range( target );
	for ( ; targets.first != targets.second ; ++targets.first )
	{
		if ( targets.first->second == owner )
		{
			m_EffectLinkDb.erase( targets.first );
			break;
		}
	}
}

void GoDb :: CollectObjects(
		/*OUT*/ siege::WorldObjectColl& objects,		// found objects go here
		/*IN */ const siege::SiegeNode& node,			// collecting objects in this node
		/*IN */ siege::eCollectFlags    cflags,			// find objects of this type
		/*IN */ siege::eRequestFlags    rflags ) const	// calc and store this additional info
{
	GPPROFILERSAMPLE( "GoDb::CollectObjects()", SP_DRAW | SP_DRAW_CALC_GO );
	GPSTATS_GROUP( GROUP_RENDER_MODELS );

	RECURSE_LOCK_FOR_READ( SiegeGoDb );

	// find occupants of this node
	std::pair <SiegeGoDb::const_iterator, SiegeGoDb::const_iterator> rc =
			m_SiegeGoDb.equal_range( node.GetGUID() );
	if ( rc.first != rc.second )
	{
		// are we rendering? since we don't have a specific flag for this,
		// we can assume that if the client is worrying about alpha then
		// they must be rendering.
		bool isRendering = !!(rflags & siege::RF_ALPHA);

		// construct an initial aspectinfo to store data into
		objects.push_back();

		// for each occupant...
		for ( ; rc.first != rc.second ; ++rc.first )
		{
			gpassert( rc.first->first == node.GetGUID() );

			// get the go
			Go* go = rc.first->second;

#			if !GP_RETAIL
			// Must be in the world (along with ALL kids) if we're rendering it!!
			if ( !go->IsInAnyWorldFrustum() )
			{
				gperrorf(("ERROR: [NOT-IN-WORLD] GoDB::CollectObjects() [%s]",go->GetTemplateName() ));
			}
			if ( go->HasAspect() )
			{
				nema::Aspect* asp = go->GetAspect()->GetAspectHandle().Get();
				nema::ChildLinks kids = asp->GetChildren();

				for (nema::ChildLinkIter it = kids.begin(); it != kids.end(); ++it)
				{
					if ((*it).first.IsValid())
					{
						nema::Aspect* child = (*it).first.Get();
						Go* kidgo =  GetGo(child->GetGoid());
						if (kidgo)
						{
							if ( !kidgo->IsInAnyWorldFrustum() && !kidgo->IsPendingEntry() )
							{
								gperrorf(("ERROR: [NOT-IN-WORLD] GoDB::CollectObjects() child [%s] (of [%s])",kidgo->GetTemplateName(),go->GetTemplateName() ));
							}
						}
					}
				}
			}
#			endif

			// get an info obj ready
			siege::ASPECTINFO* info = &objects.back();
			info->pNode = &node;

			// render the go
			if ( isRendering )
			{
#				if ( !GP_RETAIL )
				++m_LastViewCount;
				{
					// tell go to render its debug hud if any. note that this
					// also updates its "last view" sim id.
					go->DrawDebugHud();

					// gizmo will render too
					GoGizmo* gizmo = go->QueryGizmo();
					if ( gizmo != NULL )
					{
						info->Flags = gizmo->BuildCollectFlags();
						if ( gizmo->BuildRenderInfo( *info ) )
						{
							// commit last one and add a new one
							info = &*objects.push_back();
							info->pNode = &node;
						}
					}
				}
#				endif

				// fader will render too
				GoFader* fader = go->QueryFader();
				if ( fader != NULL )
				{
					info->Flags = fader->BuildCollectFlags();
					if ( info->Flags && fader->BuildRenderInfo( *info, rflags ) )
					{
						// commit last one and add a new one
						info = &*objects.push_back();
						info->pNode = &node;
					}
				}

				// give go an opportunity to draw ui features
				go->Draw();
			}

			// get at the go's aspect component, skip if it doesn't have one
			GoAspect* aspect = go->QueryAspect();
			if ( aspect == NULL )
			{
				continue;
			}

			// see what the flags are and skip if we don't want this one
			info->Flags = aspect->BuildCollectFlags();
			if( !(info->Flags & cflags) )
			{
				continue;
			}

			// ask the aspect if it wants to be rendered and get its info
			if ( !aspect->BuildRenderInfo( *info, rflags ) )
			{
				continue;
			}

			// commit the last one, get a new one
			objects.push_back();

			// update count maybe
#			if ( !GP_RETAIL )
			{
				if ( isRendering )
				{
					++m_LastRenderCount;
				}
			}
#			endif // !GP_RETAIL

			// paranoid
			gpassert( go != NULL );
			gpassert( go == rc.first->second );
		}

		// the last one was unused, dump it
		objects.pop_back();
	}
}

void GoDb :: OnSpaceChanged( siege::database_guid guid, bool bIsElevator )
{
	// note: this could be called from the second thread in a special case -
	// if we are loading in nodes on the edge of the world on a region boundary.
	// in this case, rather than queuing up notifications to dirty them on the
	// next godb update, we can simply ignore the space changed notification
	// right here, because any go's loaded there will have proper space in the
	// first place. and if you cross nodes they will get their notification
	// properly anyway.
	UNREFERENCED_PARAMETER( bIsElevator );

	if ( IsPrimaryThread() )
	{
		// lock and change
		{
			RECURSE_LOCK_FOR_WRITE( SiegeGoDb );

			// find our range of go's rooted in this node to work with
			std::pair <SiegeGoDb::iterator, SiegeGoDb::iterator> rc = m_SiegeGoDb.equal_range( guid );
			if ( rc.first != rc.second )
			{
				// invalidate space caches
				for ( SiegeGoDb::iterator i = rc.first ; i != rc.second ; ++i )
				{
					i->second->SetSpaceDirty();
					if ( bIsElevator && i->second->HasMind() )
					{
						i->second->GetMind()->SetIsRidingElevator();
					}
				}
			}
		}

		// let aiq know
		gAIQuery.OnSpaceChanged( guid );
	}
}

GoDb::GlobalConstIter GoDb :: GlobalBegin( void ) const
{
	return ( GlobalConstIter( ccast <GlobalDb&> ( m_GlobalDb ).begin() ) );
}

GoDb::GlobalConstIter GoDb :: GlobalEnd( void ) const
{
	return ( GlobalConstIter( ccast <GlobalDb&> ( m_GlobalDb ).end() ) );
}

GoDb::GlobalIter GoDb :: GlobalBegin( void )
{
	return ( GlobalIter( m_GlobalDb.begin() ) );
}

GoDb::GlobalIter GoDb :: GlobalEnd( void )
{
	return ( GlobalIter( m_GlobalDb.end() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoBucket implementation

#if !GP_RETAIL

UINT32 GoDb::GoBucket :: AddCRC32( UINT32 seed ) const
{
	if ( m_First != NULL )
	{
		seed = m_First->AddCRC32( seed );
	}

	GopColl::const_iterator i, ibegin = m_Remaining.begin(), iend = m_Remaining.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( *i != NULL )
		{
			seed = (*i)->AddCRC32( seed );
		}
	}

	return ( seed );
}

UINT32 GoDb::GoBucket :: AddCRC32FromHandles( Goid baseGoid, UINT32 seed ) const
{
	GlobalGoidBits bits( baseGoid );
	bits.m_MinorIndex = 0;
	gpassert( bits.m_Class == GO_CLASS_GLOBAL );

	for ( int i = 0 ; i < m_Count ; ++i, ++bits.m_MinorIndex )
	{
		GoHandle go( bits );
		if_gpassert( go )
		{
			seed = go->AddCRC32( seed );
		}
	}

	return ( seed );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoAdder implementation

GoDb::GoAdder :: GoAdder( void )
{
	m_Goid          = GOID_INVALID;
	m_RefCount      = 0;
	m_ClientInitial = false;
	m_OwnerThreadId = 0;
}

void GoDb::GoAdder :: EnterServer( void )
{
	// this must be the first add
	gpassert( !IsActive() );
	gpassert( !m_ClientInitial );

	// set goid class to global (client/server only used to create globals)
	m_Goid = GO_CLASS_GLOBAL;

	// up ref count, set state
	AddRef();
}

void GoDb::GoAdder :: EnterClient( Goid initialGoid )
{
	// verify that this is either the first add or we came from the server
	// initial state. also must be global.
	gpassert( IsActive() || (m_Goid == GOID_INVALID) );
	gpassert( !IsActive() || (m_Goid == initialGoid) );
	gpassert( GetGoidClass( initialGoid ) == GO_CLASS_GLOBAL );
	gpassert( !m_ClientInitial );

	// take goid
	m_Goid = initialGoid;

	// up ref count
	AddRef();
	m_ClientInitial = true;
}

void GoDb::GoAdder :: Enter( DWORD goidClass )
{
	// verify that this is either the first add, or that we are adding new go's
	// of the same class as the original
	gpassert( !IsActive() || ((m_Goid != GOID_INVALID) && (goidClass == GetGoidClass( m_Goid ))) );

	// cannot be a clone source - those are only created in FindCloneSource()
	gpassert( goidClass != GO_CLASS_CLONE_SRC );

	// set goid class if first hit - just use global bits cls as a helper
	if ( !IsActive() )
	{
		GlobalGoidBits bits( (DWORD)0 );
		bits.m_Class = goidClass;
		m_Goid = bits;
	}

	// up ref count
	AddRef();
}

void GoDb::GoAdder :: Leave( void )
{
	// down ref count
	gpassert( m_RefCount > 0 );
	if ( --m_RefCount == 0 )
	{
		gGoDb.FinishGoCreation( m_Goid );

		// reset if all done
		m_Goid = GOID_INVALID;
		m_ClientInitial = false;
		m_OwnerThreadId = 0;
	}
}

Goid GoDb::GoAdder :: AssignNextGoid( void )
{
	RwCritical::WriteLock writeLock( gGoDb.m_DbCritical );

	// generate a new one
	Goid originalGoid = m_Goid;
	switch ( GetGoidClass( m_Goid ) )
	{
		case ( GO_CLASS_GLOBAL ):
		{
			Go* oldGo = NULL;

			// client uses predefined goid
			if ( m_ClientInitial )
			{
				GoBucket* bucket;
				GlobalGoidBits bits( m_Goid );

				// add the empty to db
				bool added = gGoDb.m_GlobalDb.AllocSpecific( bucket, bits.m_MajorIndex, bits.m_Magic );
				if ( added )
				{
					// just initialize it
					bucket->m_First = NULL;
					bucket->m_Count = 0;
				}

				// if standalone but not importing, force the slot to exist
				if ( gGoDb.IsConstructingStandaloneGo() && !gGoDb.IsImportingStandaloneGo() )
				{
					// root slot?
					if ( bits.m_MinorIndex == 0 )
					{
						oldGo = bucket->m_First;
					}
					else
					{
						// make sure we have room for it
						bucket->m_Remaining.resize( max_t( bucket->m_Remaining.size(), bits.m_MinorIndex ), NULL );
						oldGo = bucket->m_Remaining[ bits.m_MinorIndex - 1 ];
					}

					// got a new one
					++bucket->m_Count;
					++gGoDb.m_GlobalDbCount;
				}
				else
				{
					if ( added )
					{
						// added into slot 0 - register it
						++bucket->m_Count;
						++gGoDb.m_GlobalDbCount;
					}
					else
					{
						// something (likely a ServerLock) already got it
						oldGo = bucket->m_First;
						gpassert( bucket->m_Count == 1 );
					}
					gpassert( bucket->m_Remaining.empty() );
				}

				// normal state
				m_ClientInitial = false;
			}
			else if ( m_RefCount == 1 )
			{
				// add brand new bucket to db
				GoBucket* bucket;
				UINT majorIndex, magic;
				gGoDb.m_GlobalDb.Alloc( bucket, majorIndex, magic );

				// clear it
				bucket->m_First = NULL;
				bucket->m_Count = 1;
				++gGoDb.m_GlobalDbCount;
				gpassert( bucket->m_Remaining.empty() );

				// build goid for new entry
				m_Goid = MakeGlobalGoid( majorIndex, 0, magic );
			}
			else
			{
				// find existing bucket
				GlobalGoidBits bits( m_Goid );
				GoBucket* bucket = gGoDb.m_GlobalDb.Dereference( bits.m_MajorIndex, bits.m_Magic );
				if ( bucket != NULL )
				{
					// add it
					bucket->m_Remaining.push_back( NULL );
					++bucket->m_Count;
					++gGoDb.m_GlobalDbCount;

					// build goid for new entry
					int minorIndex = bucket->m_Remaining.size();
					int maxIndex = (1 << GlobalGoidBits::MAX_BITS_MINOR_INDEX);
					if ( minorIndex < maxIndex )
					{
						bits.m_MinorIndex = bucket->m_Remaining.size();
						m_Goid = bits;
					}
					else
					{
						Goid rootGoid( MakeGlobalGoid( bits.m_MajorIndex, 0, bits.m_Magic ) );
						GoHandle rootGo( rootGoid );

						gperrorf((
								"SERIOUS ERROR! Somebody is creating greater than "
								"%d sub-objects, which is too many, not enough "
								"bits!! The most likely causes of this are an "
								"insane skrit creating too much stuff during "
								"CommitCreation() or a store or other inventory-"
								"containing Go which just has too much stuff in "
								"it. Either reduce the number of objects being "
								"created or defer some of them to happen after "
								"WE_CONSTRUCTED is sent (so they get their own "
								"GoBuckets). Note that if you do the delayed "
								"creation option, be sure to preload their "
								"templates so they have minimal hit to the "
								"main thread!\n"
								"\n"
								"Root goid = 0x%08X, scid = 0x%08X, template = '%s'\n",
								maxIndex - 1, rootGoid,
								rootGo ? rootGo->GetScid() : 0,
								rootGo ? rootGo->GetTemplateName() : "<UNKNOWN>" ));

						// can't use this!!
						m_Goid = GOID_INVALID;
					}
				}
#				if !GP_RETAIL
				else
				{
					gpstring contents;
					gGoDb.AppendBucketContents( m_Goid, contents );
					gperrorf((
							"TELL SCOTT that you just repro'd #12218. We're "
							"trying to add a Go to an existing bucket, but "
							"when I did the lookup, it failed!!\n"
							"\n"
							"New goid = 0x%08X\n"
							"\n"
							"%s\n",
							m_Goid, contents.c_str() ));
				}
#				endif // !GP_RETAIL
			}

			if ( oldGo != NULL )
			{
#				if !GP_RETAIL
				gpstring contents;
				gGoDb.AppendBucketContents( m_Goid, contents );
				gperrorf(( "SERIOUS ERROR: I was told to create a new Go at a "
						   "specific slot, but there was something already "
						   "there! This is terrible news. I have to abort the "
						   "object creation, and who knows what's going to "
						   "happen next...\n"
						   "\n"
						   "Adder stats:\n"
						   "\n"
						   "m_Goid (pre)    = 0x%08X\n"
						   "m_Goid (post)   = 0x%08X\n"
						   "m_RefCount      = %d\n"
						   "m_ClientInitial = %s\n"
						   "m_OwnerThreadId = %d\n"
						   "\n"
						   "Bucket contents:\n"
						   "\n"
						   "%s\n",
						   originalGoid,
						   m_Goid,
						   m_RefCount,
						   m_ClientInitial,
						   m_OwnerThreadId,
						   contents.c_str() ));
#				endif // !GP_RETAIL

				// can't use this!!
				m_Goid = GOID_INVALID;
			}
		}
		break;

		case ( GO_CLASS_LOCAL ):
		{
			// add to local db
			UINT index, magic;
			gGoDb.m_LocalDb.Alloc( NULL, index, magic );

			// build goid for new entry
			m_Goid = MakeLocalGoid( index, magic );
		}
		break;

		default:
		{
			gpassert( 0 );		//$ should never get here
		}
	}

	// return new goid
	return ( m_Goid );
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoAdder::LockBase implementation

GoDb::GoAdder::LockBase :: LockBase( void )
	: m_Lock( gGoDb.m_CreateLock )
{
	// this space intentionally left blank...
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoAdder::ServerLock implementation

GoDb::GoAdder::ServerLock :: ServerLock( void )
{
	gGoDb.m_Adder.EnterServer();
}

GoDb::GoAdder::ServerLock :: ~ServerLock( void )
{
	gGoDb.m_Adder.Leave();
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoAdder::ClientLock implementation

GoDb::GoAdder::ClientLock :: ClientLock( Goid initialGoid )
{
	gGoDb.m_Adder.EnterClient( initialGoid );
}

GoDb::GoAdder::ClientLock :: ~ClientLock( void )
{
	gGoDb.m_Adder.Leave();
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::GoAdder::Lock implementation

GoDb::GoAdder::Lock :: Lock( DWORD goidClass )
{
	gGoDb.m_Adder.Enter( goidClass );
}

GoDb::GoAdder::Lock :: ~Lock( void )
{
	gGoDb.m_Adder.Leave();
}

//////////////////////////////////////////////////////////////////////////////
// class GoDb::PlayerFrustum implementation

GoDb::PlayerFrustum :: PlayerFrustum( void )
{
	m_Focus = m_Coll.end();
	m_OwnedMask = FRUSTUMID_INVALID;
}

GoDb::PlayerFrustum :: PlayerFrustum( const PlayerFrustum& other )
	: m_Coll( other.m_Coll )
{
	// copy iter
	m_Focus = m_Coll.begin();
	size_t dist = std::distance( other.m_Coll.begin(), Coll::const_iterator( other.m_Focus ) );
	std::advance( m_Focus, dist );

	// copy the rest
	m_OwnedMask = other.m_OwnedMask;
}

//////////////////////////////////////////////////////////////////////////////
