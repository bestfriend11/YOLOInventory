//////////////////////////////////////////////////////////////////////////////
//
// File     :  Go.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "Go.h"

#include "AppModule.h"
#include "Flamethrower_interpreter.h"
#include "FuBi.h"
#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiPersistImpl.h"
#include "FuBiSchema.h"
#include "Gps_manager.h"
#include "MessageDispatch.h"
#include "NeMa_Aspect.h"
#include "Nema_Blender.h"
#include "ContentDb.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoCore.h"
#include "GoDb.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoEdit.h"
#include "GoFollower.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoPhysics.h"
#include "GoSupport.h"
#include "MCP.h"
#include "PContentDb.h"
#include "ReportSys.h"
#include "Rules.h"
#include "Server.h"
#include "Siege_Engine.h"
#include "Siege_Frustum.h"
#include "SkritObject.h"
#include "SkritSupport.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "World.h"
#include "WorldFx.h"
#include "WorldMap.h"
#include "WorldMessage.h"
#include "WorldOptions.h"
#include "WorldState.h"
#include "WorldSound.h"

//////////////////////////////////////////////////////////////////////////////
// priority sorting for Go components

struct GocSort
{
	const char* m_Name;
	int m_Priority;
};

static const int DEFAULT_PRIORITY = 100;

static GocSort s_GocSort[] =
{
	// $ lower numbers will get sorted earlier (updated first)

	{  "actor",      0,  },		// actor has to come before inventory so skills xfer before items on import
	{  "aspect",     1,  },
	{  "attack",     3,  },		// attack has to come after aspect so aspect is reconstituted when we create ammo, and after inventory so weapons available to parent to
	{  "body",       4,  },		// $ mike sez body gotta come after everything else
	{  "common",     0,  },
	{  "placement",  0,  },
	{  "gold",       5,  },		// do gold last so aspect is ready and we can update it
	{  "gui",        0,  },
	{  "defend",     0,  },
	{  "edit",       0,  },
	{  "follower",   0,  },
	{  "gizmo",      0,  },
	{  "inventory",  2,  },
	{  "magic",      0,  },
	{  "party",      0,  },
	{  "physics",    0,  },
};

struct LessByName
{
	bool operator () ( const GocSort& l, const GocSort& r ) const
		{  return ( compare_no_case( l.m_Name, r.m_Name ) < 0 );  }
	bool operator () ( const char* l, const GocSort& r ) const
		{  return ( compare_no_case( l, r.m_Name ) < 0 );  }
	bool operator () ( const GocSort& l, const char* r ) const
		{  return ( compare_no_case( l.m_Name, r ) < 0 );  }
};

static inline int GetPriority( const char* gocName )
{
	int priority = DEFAULT_PRIORITY;

	static bool s_Sorted = false;
	if ( !s_Sorted )
	{
		s_Sorted = true;
		std::sort( s_GocSort, ARRAY_END( s_GocSort ), LessByName() );
	}

	const GocSort* found = stdx::binary_search( s_GocSort, ARRAY_END( s_GocSort ), gocName, LessByName() );
	if ( found != ARRAY_END( s_GocSort ) )
	{
		priority = found->m_Priority;
	}

	return ( priority );
}

bool GocSortByPriority( GoComponent* l, GoComponent* r )
{
	return ( GetPriority( l->GetName() ) < GetPriority( r->GetName() ) );
};

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_XFER_TRAITS( const GoDataTemplate* )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		gpstring name;
		if ( persist.IsSaving() && (obj != NULL) )
		{
			name = obj->GetName();
		}

		bool rc = persist.Xfer( xfer, key, name );

		if ( persist.IsRestoring() )
		{
			obj = gContentDb.AcquireTemplateByName( name );
			if ( obj == NULL )
			{
				gpreportf( persist.GetReportContext(), ( "Unable to load/find template '%s'\n", name.c_str() ));
				rc = false;
			}
		}

		return ( rc );
	}
};

FUBI_DECLARE_XFER_TRAITS( Player* )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		PlayerId id = PLAYERID_INVALID;
		if ( persist.IsSaving() && (obj != NULL) )
		{
			id = obj->GetId();
		}

		bool rc = persist.Xfer( xfer, key, id );

		if ( persist.IsRestoring() )
		{
			obj = gServer.GetPlayer( id );

			// if no player, we may be loading MP game. just assign to screen player.
			if ( obj == NULL )
			{
				obj = gServer.GetScreenPlayer();
			}
		}

		return ( rc );
	}
};

//////////////////////////////////////////////////////////////////////////////
// class GoComponent implementation

bool GoComponent::ms_Overridden = false;

GoComponent :: GoComponent( Go* go )
{
	gpassert( go != NULL );

	m_Go                 = go;
	m_DataComponent      = NULL;
	m_OwnedDataComponent = NULL;
	m_WantsUpdates       = false;
	m_WantsDraws         = false;
}

GoComponent :: GoComponent( const GoComponent& source, Go* newGo )
{
	gpassert( newGo != NULL );

	m_Go           = newGo;
	m_WantsUpdates = false;
	m_WantsDraws   = false;

	// transfer or copy depending on ownership
	if ( source.m_OwnedDataComponent != NULL )
	{
		m_OwnedDataComponent = new GoDataComponent( *source.m_OwnedDataComponent );
		m_DataComponent = m_OwnedDataComponent;
	}
	else
	{
		m_OwnedDataComponent = NULL;
		m_DataComponent = source.m_DataComponent;
	}
}

GoComponent :: ~GoComponent( void )
{
	delete ( m_OwnedDataComponent );
}

void GoComponent :: Init( GoDataComponent* data )
{
	// don't init twice
	gpassert( (m_DataComponent == NULL) && (m_OwnedDataComponent == NULL) );

	m_DataComponent = data;
	m_OwnedDataComponent = data;
}

void GoComponent :: Init( const GoDataComponent* data )
{
	// don't init twice
	gpassert( (m_DataComponent == NULL) && (m_OwnedDataComponent == NULL) );

	m_DataComponent = data;
}

#if !GP_RETAIL

void GoComponent :: ForceOwned( void )
{
	if ( m_OwnedDataComponent == NULL )
	{
		gpassert( m_DataComponent != NULL );
		m_OwnedDataComponent = new GoDataComponent( *m_DataComponent );
		m_DataComponent = m_OwnedDataComponent;
	}
	m_OwnedDataComponent->CopyForWrite();
	gpassert( m_DataComponent == m_OwnedDataComponent );
}

#endif // !GP_RETAIL

const gpstring& GoComponent :: GetName( void ) const
{
	gpassert( m_DataComponent != NULL );
	return ( m_DataComponent->GetName() );
}

Goid GoComponent :: GetGoid( void ) const
{
	return ( GetGo()->GetGoid() );
}

Scid GoComponent :: GetScid( void ) const
{
	return ( GetGo()->GetScid() );
}

void GoComponent :: SetWantsUpdates( bool set )
{
	if ( m_WantsUpdates != set )
	{
		m_WantsUpdates = set;
		m_Go->UpdateWantsUpdates( set );
	}
}

void GoComponent :: SetWantsDraws( bool set )
{
	if ( m_WantsDraws != set )
	{
		m_WantsDraws = set;
		m_Go->UpdateWantsDraws( set );
	}
}

bool GoComponent :: XferWantsBits( FuBi::PersistContext& persist )
{
	FUBI_XFER_BITFIELD_BOOL( persist, "m_WantsUpdates", m_WantsUpdates );
	FUBI_XFER_BITFIELD_BOOL( persist, "m_WantsDraws"  , m_WantsDraws   );

	return ( true );
}

void GoComponent :: DetectWantsBits( void )
{
	// only redetect if not set already
	if ( !m_WantsUpdates )
	{
		ms_Overridden = true;
		Update( 0 );
		if ( ms_Overridden )
		{
			SetWantsUpdates();
		}
	}

	// only redetect if not set already
	if ( !m_WantsDraws )
	{
		ms_Overridden = true;
		Draw();
		if ( ms_Overridden )
		{
			SetWantsDraws();
		}
	}

	gpassert( AssertValid() );
}

bool GoComponent :: CanServerExistOnly( void )
{
	return ( GetData()->CanServerExistOnly() );
}

#if !GP_RETAIL

const BitVector* GoComponent :: GetOverrideColl( void ) const
{
	return ( m_OwnedDataComponent ? &m_OwnedDataComponent->GetOverrideColl() : NULL );
}

#endif // !GP_RETAIL

#if GP_DEBUG

bool GoComponent :: AssertValid( void ) const
{
	gpassert( m_Go != NULL );
	gpassert( m_DataComponent != NULL );
	return ( true );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// class GoSkritComponent implementation

FuBi::eVarType GoSkritComponent::ms_FuBiType = FuBi::VAR_UNKNOWN;

GoSkritComponent :: GoSkritComponent( Go* go )
	: Inherited( go )
{
	m_SkritPtr = NULL;
}

GoSkritComponent :: GoSkritComponent( const GoSkritComponent& source, Go* newGo )
	: Inherited( source, newGo )
{
	m_SkritPtr = NULL;
	Clone( source.m_Skrit );
}

GoSkritComponent :: ~GoSkritComponent( void )
{
	// this space intentionally left blank...
}

const FuBi::Record* GoSkritComponent :: GetSkritRecord( void ) const
{
	return ( m_SkritPtr->GetRecord() );
}

FuBi::Record* GoSkritComponent :: GetSkritRecord( void )
{
	return ( m_SkritPtr->GetRecord() );
}

FuBi::eVarType GoSkritComponent :: GetFuBiType( void )
{
	if ( ms_FuBiType == FuBi::VAR_UNKNOWN )
	{
		ms_FuBiType = gFuBiSysExports.FindType( "GoSkritComponent" );
		gpassert( ms_FuBiType != FuBi::VAR_UNKNOWN );
	}
	return ( ms_FuBiType );
}

int GoSkritComponent :: GetCacheIndex( void )
{
	// skrits are never cached components
	return ( -1 );
}

GoComponent* GoSkritComponent :: Clone( Go* newGo )
{
	return ( new GoSkritComponent( *this, newGo ) );
}

bool GoSkritComponent :: Xfer( FuBi::PersistContext& persist )
{
	return ( SpecialXfer( persist, true ) );
}

bool GoSkritComponent :: SpecialXfer( FuBi::PersistContext& persist, bool shouldPersist )
{
	bool success = true;

	if ( persist.IsFullXfer() )
	{
		gpassert( shouldPersist );

		Skrit::HObject skrit = m_Skrit.GetHandle();
		persist.Xfer( "m_Skrit", skrit );
		if ( persist.IsRestoring() )
		{
			m_Skrit = skrit;
			m_SkritPtr = m_Skrit.Get();
			if ( m_Skrit )
			{
				m_Skrit->SetOwner( this );
			}
			else
			{
				success = false;
			}
		}
	}
	else
	{
		if ( persist.IsRestoring() && !GetGo()->HasValidGoid() )
		{
			Clone( GetData()->GetSkritObject(), shouldPersist );
		}

		FuBi::Record* record = m_Skrit->GetRecord();
		if ( record != NULL )
		{
			persist.Xfer( NULL, *record );
		}
	}

	return ( success );
}

DECLARE_EVENT
{
	SKRIT_IMPORT bool OnGoCommitCreation( Skrit::Object* skrit, Skrit::eResult& handled )
		{  SKRIT_RESULT_EVENT( bool, OnGoCommitCreation, skrit, handled );  }
	SKRIT_IMPORT bool OnGoCommitImport( Skrit::Object* skrit, bool /*fullXfer*/ )
		{  SKRIT_EVENT( bool, OnGoCommitImport, skrit );  }
	SKRIT_IMPORT void OnGoShutdown( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoShutdown, skrit );  }
	SKRIT_IMPORT void OnGoPreload( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoPreload, skrit );  }
	SKRIT_IMPORT void OnGoHandleMessage( Skrit::Object* skrit, eWorldEvent /*event*/, const WorldMessage& /*msg*/ )
		{  SKRIT_EVENTV( OnGoHandleMessage, skrit );  }
	SKRIT_IMPORT void OnGoHandleCcMessage( Skrit::Object* skrit, eWorldEvent /*event*/, const WorldMessage& /*msg*/ )
		{  SKRIT_EVENTV( OnGoHandleCcMessage, skrit );  }
	SKRIT_IMPORT void OnGoUpdate( Skrit::Object* skrit, float /*deltaTime*/ )
		{  SKRIT_EVENTV( OnGoUpdate, skrit );  }
	SKRIT_IMPORT void OnGoUpdateSpecial( Skrit::Object* skrit, float /*deltaTime*/ )
		{  SKRIT_EVENTV( OnGoUpdateSpecial, skrit );  }
	SKRIT_IMPORT void OnGoDraw( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoDraw, skrit );  }
	SKRIT_IMPORT void OnGoResetModifiers( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoResetModifiers, skrit );  }
	SKRIT_IMPORT void OnGoRecalcModifiers( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoRecalcModifiers, skrit );  }
	SKRIT_IMPORT void OnGoLinkParent( Skrit::Object* skrit, Go* )
		{  SKRIT_EVENTV( OnGoLinkParent, skrit );  }
	SKRIT_IMPORT void OnGoLinkChild( Skrit::Object* skrit, Go* )
		{  SKRIT_EVENTV( OnGoLinkChild, skrit );  }
	SKRIT_IMPORT void OnGoUnlinkParent( Skrit::Object* skrit, Go* )
		{  SKRIT_EVENTV( OnGoUnlinkParent, skrit );  }
	SKRIT_IMPORT void OnGoUnlinkChild( Skrit::Object* skrit, Go* )
		{  SKRIT_EVENTV( OnGoUnlinkChild, skrit );  }
}

bool GoSkritComponent :: CommitCreation( void )
{
	// first validate public scids
#	if ( !GP_RETAIL )
	{
		using namespace FuBi;

		eVarType scidType = gFuBiSysExports.FindType( "Scid" );
		gpassert( scidType != VAR_UNKNOWN );

		const Record* record = GetSkritRecord();
		if ( record != NULL )
		{
			const HeaderSpec* spec = record->GetHeaderSpec();
			HeaderSpec::ConstIter i, ibegin = spec->GetColumnBegin(), iend = spec->GetColumnEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_Type.m_Type == scidType )
				{
					Scid scid = SCID_INVALID;
					gpverify( record->Get( i - ibegin, scid ) );
					if ( (scid != 0) && (scid != SCID_INVALID) && !gWorldMap.ContainsScid( scid ) )
					{
						gperrorf(( "Error: invalid linked scid detected!\n"
								   "\tSource Go Scid = 0x%08X (%s)\n"
								   "\tComponent = %s\n"
								   "\tField name = %s\n"
								   "\tDestination Scid = 0x%08X (can't find it in map!)\n",
								   GetScid(), GetGo()->GetTemplateName(),
								   GetName().c_str(),
								   i->GetName().c_str(),
								   scid ));
					}
				}
			}
		}
	}
#	endif // !GP_RETAIL

	// commit the skrit
	if ( !m_SkritPtr->CommitCreation() )
	{
		return ( false );
	}

	// pass along the event
	Skrit::eResult handled;
	bool rc = FuBiEvent::OnGoCommitCreation( m_SkritPtr, handled );
	if ( handled == Skrit::RESULT_NOEVENT )
	{
		return ( true );
	}
	else if ( Skrit::IsError( handled ) )
	{
		return ( false );
	}
	else
	{
		return ( rc );
	}
}

bool GoSkritComponent :: CommitImport( bool fullXfer )
{
	return ( FuBiEvent::OnGoCommitImport( m_SkritPtr, fullXfer ) );
}

void GoSkritComponent :: Shutdown( void )
{
	FuBiEvent::OnGoShutdown( m_SkritPtr );
	m_Skrit.ReleaseSkrit();
	m_SkritPtr = NULL;
}

void GoSkritComponent :: Preload( void )
{
	FuBiEvent::OnGoPreload( m_SkritPtr );
}

void GoSkritComponent :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );
	FuBiEvent::OnGoHandleMessage( m_SkritPtr, msg.GetEvent(), msg );
}

void GoSkritComponent :: HandleCcMessage( const WorldMessage& msg )
{
	gpassert( msg.IsCC() );
	FuBiEvent::OnGoHandleCcMessage( m_SkritPtr, msg.GetEvent(), msg );
}

void GoSkritComponent :: Update( float deltaTime )
{
	FuBiEvent::OnGoUpdate( m_SkritPtr, deltaTime );
	m_SkritPtr->Poll( deltaTime, true );
}

void GoSkritComponent :: UpdateSpecial( float deltaTime )
{
	FuBiEvent::OnGoUpdateSpecial( m_SkritPtr, deltaTime );
}

void GoSkritComponent :: Draw( void )
{
	FuBiEvent::OnGoDraw( m_SkritPtr );
}

void GoSkritComponent :: ResetModifiers( void )
{
	FuBiEvent::OnGoResetModifiers( m_SkritPtr );
}

void GoSkritComponent :: RecalcModifiers( void )
{
	FuBiEvent::OnGoRecalcModifiers( m_SkritPtr );
}

void GoSkritComponent :: LinkParent( Go* parent )
{
	FuBiEvent::OnGoLinkParent( m_SkritPtr, parent );
}

void GoSkritComponent :: LinkChild( Go* child )
{
	FuBiEvent::OnGoLinkChild( m_SkritPtr, child );
}

void GoSkritComponent :: UnlinkParent( Go* parent )
{
	FuBiEvent::OnGoUnlinkParent( m_SkritPtr, parent );
}

void GoSkritComponent :: UnlinkChild( Go* child )
{
	FuBiEvent::OnGoUnlinkChild( m_SkritPtr, child );
}

void GoSkritComponent :: DetectWantsBits( void )
{
	SetWantsUpdates( GetData()->SkritWantsUpdates() );
	SetWantsDraws( GetData()->SkritWantsDraws() );
}

DECLARE_EVENT
{
	// $ special: these events must be exported even though they are never
	//   used, because the skrits will be defining handlers for them. so just
	//   let them do their handlers and ignore them.

	SKRIT_IMPORT void OnGoDrawDebugHud( Skrit::Object* skrit )
		{  SKRIT_EVENTV( OnGoDrawDebugHud, skrit );  }
	SKRIT_IMPORT void OnGoDump( Skrit::Object* skrit, bool /*verbose*/, ReportSys::Context* /*ctx*/ )
		{  SKRIT_EVENTV( OnGoDump, skrit );  }
}

#if !GP_RETAIL

void GoSkritComponent :: DrawDebugHud( void )
{
	FuBiEvent::OnGoDrawDebugHud( m_SkritPtr );
}

void GoSkritComponent :: Dump( bool verbose, ReportSys::ContextRef ctx )
{
	FuBiEvent::OnGoDump( m_SkritPtr, verbose, ctx );
}

#endif // !GP_RETAIL

#if GP_DEBUG

bool GoSkritComponent :: AssertValid( void ) const
{
	Inherited::AssertValid();
	gpassert( m_Skrit );
	gpassert( m_SkritPtr != NULL );
	return ( true );
}

#endif // GP_DEBUG

void GoSkritComponent :: Clone( Skrit::HObject cloneSource, bool shouldPersist )
{
	gpassert( !m_Skrit );
	gpassert( m_SkritPtr == NULL );

	Skrit::CloneReq cloneReq( cloneSource );
	cloneReq.SetOwner( GetFuBiType(), this );
	cloneReq.m_DeferConstruction = true;
	cloneReq.m_CloneGlobals = true;
	cloneReq.SetPersist( shouldPersist );
	m_Skrit.CloneSkrit( cloneReq );
	m_SkritPtr = m_Skrit.Get();

	gpassert( m_Skrit );
	gpassert( m_SkritPtr != NULL );
}

//////////////////////////////////////////////////////////////////////////////
// Go support

#if !GP_RETAIL

struct AutoGoError
{
	Go*  m_Go;
	int* m_Scope;
	int  m_ErrorsBefore;
	int  m_WarningsBefore;

	AutoGoError( Go* go, int* scope )
		: m_Go( go ), m_Scope( scope )
	{
		if ( (*m_Scope)++ == 0 )
		{
			m_ErrorsBefore   = ReportSys::GetErrorCount();
			m_WarningsBefore = ReportSys::GetWarningCount();
		}
	}

   ~AutoGoError( void )
	{
		if ( --(*m_Scope) == 0 )
		{
			m_Go->IncrementErrorCount  ( ReportSys::GetErrorCount  () - m_ErrorsBefore   );
			m_Go->IncrementWarningCount( ReportSys::GetWarningCount() - m_WarningsBefore );
		}
	}
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class Go implementation

RECURSE_DECLARE( ComponentColl );

Go* Go :: GetRoot( void )
{
	Go* go = this;
	while ( go->m_Parent != NULL )
	{
		go = go->m_Parent;
	}

	return ( go );
}

int Go :: GetRootDepth( void ) const
{
	int depth = 0;

	const Go* go = this;
	while ( go->m_Parent != NULL )
	{
		go = go->m_Parent;
		++depth;
	}

	return ( depth );
}

Go* Go :: GetOwningParty( void ) const
{
	Go* go = m_Parent;
	if ( go != NULL )
	{
		while ( (go->m_Parent != NULL) && !go->HasParty() )
		{
			go = go->m_Parent;
		}
		if ( !go->HasParty() )
		{
			go = NULL;
		}
	}

	return ( go );
}

bool Go :: IsInSameParty( const Go* other ) const
{
	const Go* p0 = GetOwningParty();
	const Go* p1 = other ? other->GetOwningParty() : NULL;
	return ( (p0 != NULL) && (p0 == p1) );
}

bool Go :: IsRelativeOf( const Go* other ) const
{
	// if it's the same as me, easy oot
	if ( other == this )
	{
		return ( true );
	}

	// see if i'm its parent somewhere
	for ( const Go* go = other ; go->m_Parent != NULL ; go = go->m_Parent )
	{
		if ( go->m_Parent == this )
		{
			return ( true );
		}
	}

	// see if it's my parent somewhere
	for ( go = this ; go->m_Parent != NULL ; go = go->m_Parent )
	{
		if ( go->m_Parent == other )
		{
			return ( true );
		}
	}

	// guess not
	return ( false );
}

PlayerId Go :: GetPlayerId( void ) const
{
	return ( m_Player->GetId() );
}

DWORD Go :: GetMachineId( void ) const
{
	return ( m_Player->GetMachineId() );
}

void Go :: SSetPlayer( PlayerId id )
{
	CHECK_SERVER_ONLY;
	RCSetPlayer( id );
}

void Go :: RCSetPlayer( PlayerId id )
{
	FUBI_RPC_THIS_CALL( RCSetPlayer, RPC_TO_ALL );

	Player * player = gServer.GetPlayer( id );
	if ( player )
	{
		SetPlayer( player );
	}
	else
	{
		gperror( "Trying to set Player to NULL" );
	}
}

void Go :: SetPlayer( Player* player )
{
	if ( m_Player != player )
	{
		Player* oldPlayer = m_Player;
		m_Player = player;
		WorldMessage( WE_PLAYER_CHANGED, GetGoid(), GetGoid(), (DWORD)oldPlayer ).Send();

		for ( GopColl::iterator i = m_ChildColl.begin() ; i != m_ChildColl.end() ; ++i )
		{
			(*i)->SetPlayer( player );
		}
	}

	// validate
	gpassert( AssertValid() );
}

const char* Go :: GetTemplateName( void ) const
{
	gpassert( m_DataTemplate != NULL );
	return ( m_DataTemplate ? m_DataTemplate->GetName() : "<NULL>" );
}

#if !GP_RETAIL

gpstring Go :: MakeTeleportLocation( void ) const
{
	if ( IsOmni() )
	{
		return ( "<OMNI>" );
	}
	else
	{
		return ( gpstringf(
				"%s:n:%0.2f,%0.2f,%0.2f,%s",
				gWorldMap.GetMapName().c_str(),
				GetPlacement()->GetPosition().pos.x,
				GetPlacement()->GetPosition().pos.y,
				GetPlacement()->GetPosition().pos.z,
				GetPlacement()->GetPosition().node.ToString().c_str() ) );
	}
}

UINT32 Go :: AddCRC32( UINT32 seed ) const
{
	// $ include things that are relevant to pcontent calcs

	// template name
	::AddCRC32( seed, GetTemplateName(), ::strlen( GetTemplateName() ) );

	// variation name
	if ( HasGui() )
	{
		::AddCRC32( seed, GetGui()->GetVariation().c_str(), GetGui()->GetVariation().length() );
	}

	// modifiers
	if ( HasMagic() )
	{
		::AddCRC32( seed, GetMagic()->GetPrefixModifierName().c_str(), GetMagic()->GetPrefixModifierName().length() );
		::AddCRC32( seed, GetMagic()->GetSuffixModifierName().c_str(), GetMagic()->GetSuffixModifierName().length() );
	}

	// done
	return ( seed );
}

#endif // !GP_RETAIL

int Go :: GetMpPlayerCount( void ) const
{
	if ( IsCloneSourceGo() || (m_MpPlayerCount == 0) )
	{
		return ( Player::GetHumanPlayerCount() );
	}
	else
	{
		return ( m_MpPlayerCount );
	}
}

Go* Go :: GetCloneSource( void )
{
	Go* cloneSource = NULL;

	// see if it's still around and is the same type
	GoHandle go( m_CloneSource );
	if ( go && ::same_no_case( GetTemplateName(), go->GetTemplateName() ) )
	{
		cloneSource = go;
	}
	else
	{
		// must have gone bye bye
		m_CloneSource = GOID_INVALID;
	}

	return ( cloneSource );
}

int Go :: GetCloneSourceDepth( void )
{
	int depth = 0;

	for ( Go* go = this ; (go = go->GetCloneSource()) != NULL ; ++depth )
	{
		// just iterate
	}

	return ( depth );
}

bool Go :: IsInAnyScreenWorldFrustum( void ) const
{
#	if !GP_RETAIL
	if ( gGoDb.IsEditMode() )
	{
		return ( true );
	}
#	endif // !GP_RETAIL

	return ( BitFlagsContainAny( m_FrustumMembership, gGoDb.GetScreenPlayerFrustumMask() ) );
}

bool Go :: IsInActiveWorldFrustum( void ) const
{
#	if !GP_RETAIL
	if ( gGoDb.IsEditMode() )
	{
		return ( true );
	}
#	endif // !GP_RETAIL

	return ( BitFlagsContainAny( m_FrustumMembership, gGoDb.GetActiveFrustumMask() ) );
}

bool Go :: IsInActiveScreenWorldFrustum( void ) const
{
#	if !GP_RETAIL
	if ( gGoDb.IsEditMode() )
	{
		return ( true );
	}
#	endif // !GP_RETAIL

	return (   BitFlagsContainAny( m_FrustumMembership, gGoDb.GetActiveFrustumMask() )
			&& BitFlagsContainAny( m_FrustumMembership, gGoDb.GetScreenPlayerFrustumMask() ) );
}

FrustumId Go :: CalcWorldFrustumMembership( bool useNode ) const
{
	// have to ask the placement where we gonna be
	if ( IsPendingEntry() && HasPlacement() && useNode )
	{
		return ( MakeFrustumId( gSiegeEngine.GetNodeFrustumOwnedBitfield( GetPlacement()->GetPosition().node ) ) );
	}

	// if we're omni, we're in all frusta!
	if ( IsOmni() )
	{
		return ( MakeFrustumId( 0xFFFFFFFF ) );
	}

	// ok then default
	return ( GetWorldFrustumMembership() );
}

Go* Go :: GetFrustumOwnerInChain( bool inheritedPlacementOnly )
{
	if ( HasFrustum() )
	{
		return ( this );
	}

	if ( !HasParent() )
	{
		return ( NULL );
	}

	if ( inheritedPlacementOnly )
	{
		if ( HasPlacement() && GetPlacement()->DoesInheritPlacement() )
		{
			return ( GetParent()->GetFrustumOwnerInChain( true ) );
		}
		else
		{
			return ( NULL );
		}
	}
	else
	{
		return ( GetParent()->GetFrustumOwnerInChain() );
	}
}

bool Go :: IsCommittingCreation( void ) const
{
	if ( m_IsCommittingCreation )
	{
		return ( true );
	}
	if ( HasParent() )
	{
		return ( GetParent()->IsCommittingCreation() );
	}
	return ( false );
}

bool Go :: CalcBucketDirty( void ) const
{
	// see if we're dirty ourselves
	if ( IsBucketDirty() )
	{
		return ( true );
	}

	// get root go
	GlobalGoidBits root( GetGoid() );
	root.m_MinorIndex = 0;
	if ( GetGoid() == root )
	{
		// hey that's me!
		return ( false );
	}

	// see if root go is dirty
	GoHandle rootGo( root );
	return ( !rootGo || rootGo->IsBucketDirty() );
}

bool Go :: IsHero( void ) const
{
	return ( m_Player && (m_Player->GetHero() == GetGoid()) );
}

bool Go :: Select( bool select )
{
	// the godb does all the work... note that if we're selecting, we always
	// tell the godb to do it too so the go can be put at the front of the list
	if ( select || IsSelected() )
	{
		return ( gGoDb.Select( GetGoid(), select ) );
	}
	else
	{
		return ( select );
	}
}

void Go :: ClearAllChildrenVisit( void )
{
	GopColl::iterator i, ibegin = m_ChildColl.begin(), iend = m_ChildColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)->ClearVisit();
	}
}

bool Go :: IsTransferring( void ) const
{
	return ( m_Transferring );
}

bool Go :: SetTransferring( bool set )
{
	CHECK_PRIMARY_THREAD_ONLY;

	bool old = m_Transferring;
	m_Transferring = set;
	return ( old );
}

bool Go :: ClearTransferring( void )
{
	return ( SetTransferring( false ) );
}

#if !GP_RETAIL

bool Go :: IsPlacementChanging( void ) const
{
	if ( m_PlacementChanging > 0 )
	{
		return ( true );
	}
	if ( !HasParent() )
	{
		return ( false );
	}
	return ( GetParent()->IsPlacementChanging() );
}

void Go :: SetPlacementChanging( bool set )
{
	m_PlacementChanging += set ? 1 : -1;
	gpassert( m_PlacementChanging >= 0 );
}

void Go :: ClearPlacementChanging( void )
{
	SetPlacementChanging( false );
}

#endif // !GP_RETAIL

void Go :: UpdateWantsUpdates( bool newValue )
{
	// see if it's worth checking
	if ( m_WantsUpdates != newValue )
	{
		// turning updates off, have to get all the other components to agree
		if ( !newValue )
		{
			// skip server-only components if we're not the server
			ComponentColl::const_iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (*i)->GetWantsUpdates() )
				{
					newValue = true;
					break;
				}
			}
		}

		// update bit
		m_WantsUpdates = newValue;

		// maybe insert, maybe remove, who knows?
		PrivateCheckUpdates();
	}
}

void Go :: UpdateWantsDraws( bool newValue )
{
	if ( m_WantsDraws != newValue )
	{
		// turning draws off, have to get all other components to agree
		if ( !newValue )
		{
			RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

			ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (*i)->GetWantsDraws() )
				{
					newValue = true;
					break;
				}
			}
		}

		// change it
		m_WantsDraws = newValue;
	}
}

void Go :: UpdateWantsBits( void )
{
	m_WantsUpdates = false;
	m_WantsDraws   = false;

	RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

	ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// check for wants updates
		if ( (*i)->GetWantsUpdates() )
		{
			m_WantsUpdates = true;
		}

		// check for wants draws
		if ( (*i)->GetWantsDraws() )
		{
			m_WantsDraws = true;
		}

		// seen enough
		if ( m_WantsUpdates && m_WantsDraws )
		{
			break;
		}
	}
}

void Go :: PrepareToDrawNow( bool includeTextures )
{
	if ( HasAspect() )
	{
		GetAspect()->PrepareToDrawNow( includeTextures );
	}
}

bool Go :: IsInViewFrustum( void ) const
{
	return ( ( m_LastSimViewOccupant == gWorldTime.GetSimCount() ) || ( m_LastSimViewOccupant + 1 == gWorldTime.GetSimCount() ) );
}

bool Go :: WasInViewFrustumRecently( void ) const
{
	// $$$ SPECIAL MP HACK BEGIN
	//     so the server player's visibility doesn't determine this... we need
	//     to find a fix for this!!!
	if ( ::IsMultiPlayer() )
	{
		return ( IsInActiveWorldFrustum() );
	}
	// $$$ SPECIAL MP HACK END

	// last ten frames seems good enough for "recent"
	return ( (gWorldTime.GetSimCount() - m_LastSimViewOccupant) < 10 );
}

#if !GP_RETAIL

bool Go :: IsProbed( void ) const
{
	return ( GetGoid() == gWorldOptions.GetDebugProbeObject() );
}

bool Go :: IsHuded( void ) const
{
	bool is = false;

	if ( IsInViewFrustum() && (!HasAspect() || (GetAspect()->HasAspectHandle() && GetAspect()->IsNemaRoot() && (GetAspect()->GetIsVisible() || gWorldOptions.GetShowAll()))) )
	{
		if ( GetGoid() == gWorldOptions.GetDebugHUDObject() )
		{
			is = true;
		}
		else if ( gWorldOptions.GetDebugHudGroup() == DHG_ALL )
		{
			is = true;
		}
		else if ( IsActor() )
		{
			is = gWorldOptions.GetDebugHudGroup() == DHG_ACTORS;
		}
		else if ( IsItem() )
		{
			is = gWorldOptions.GetDebugHudGroup() == DHG_ITEMS;
		}
		else if ( IsCommand() )
		{
			is = gWorldOptions.GetDebugHudGroup() == DHG_COMMANDS;
		}
		else
		{
			is = gWorldOptions.GetDebugHudGroup() == DHG_MISC;
		}
	}

	return ( is );
}

#endif // !GP_RETAIL

eLifeState Go :: GetLifeState( void ) const
{
	return ( HasAspect() ? GetAspect()->GetLifeState() : LS_IGNORE );
}

const char* Go :: GetMaterial( void ) const
{
	// default material
	const char* material = "generic";

	// determine material type from aspect
	const GoAspect* aspect = QueryAspect();
	if ( aspect != NULL )
	{
		material = aspect->GetMaterial();
	}

	// done
	return ( material );
}

Go* Go :: GetBestArmor( void )
{
	return ( HasInventory() ? GetInventory()->GetBestArmor() : this );
}

bool Go :: IsActor( void ) const
{
	return ( HasActor() );
}

bool Go :: IsAmmo( void ) const
{
	return ( HasAttack() ? ::IsAmmo( GetAttack()->GetAttackClass() ) : false );
}

bool Go :: IsAnyHumanParty( void ) const
{
	return ( HasParty() && (GetPlayer()->GetController() == PC_HUMAN) );
}

bool Go :: IsAnyHumanPartyMember( void ) const
{
	return ( HasParent() && GetParent()->IsAnyHumanParty() );
}

bool Go :: IsArmor( void ) const
{
	return ( IsItem() && HasDefend() );
}

bool Go :: IsBreakable( void ) const
{
	return ( HasPhysics() && GetPhysics()->GetIsBreakable() );
}

bool Go :: IsChad( void ) const
{
	if ( same_no_case( GetTemplateName(), "chad" ) )
	{
		gpgeneric( "Hey guess what, it looks like I'm Chad!\n" );
		return ( true );
	}
	else
	{
		gpwarning( "Holy crap, how come I'm not Chad?\n" );
		return ( false );
	}
}

bool Go :: IsCommand( void ) const
{
	// $ note that the ccast is ok - this is just updating cache info, still
	//   want to treat IsCommand() as const. compiler doesn't support 'mutable'
	//   on bitfields either apparently.

	if ( !m_IsCommandCached )
	{
		ccast <ThisType*> ( this )->m_IsCommandCached = true;
		ccast <ThisType*> ( this )->m_IsCommand = same_no_case( GetTemplateName(), "cmd_", 4 );
	}
	return ( m_IsCommand );
}

bool Go :: IsContainer( void ) const
{
	return ( (IsUsable() || IsBreakable()) && HasInventory() && !IsActor() );
}

bool Go :: IsEquipped( void ) const
{
	eEquipSlot slot;
	return ( IsInsideInventory( &slot ) );
}

bool Go :: IsActiveSpell( void ) const
{
	if ( IsInsideInventory() )
	{
		Go* go = GetParent();
		if ( go != NULL )
		{
			go = go->GetParent();
			if ( go != NULL )
			{
				GoInventory* inv = go->QueryInventory();
				if ( inv != NULL )
				{
					if ( inv->IsActiveSpell( this ) )
					{
						return ( true );
					}
				}
			}
		}
	}

	return ( false );
}

bool Go :: IsFireImmune( void ) const
{
	return (   !HasAspect () || GetAspect ()->GetIsInvincible()
			|| !HasPhysics() || GetPhysics()->GetIsFireImmune() );
}

bool Go :: IsGold( void ) const
{
	return ( HasGold() );
}

bool Go :: IsInsideInventory( eEquipSlot* slot ) const
{
	bool is = false;

	if ( slot != NULL )
	{
		*slot = ES_NONE;
	}

	Go* parent = GetParent();
	if ( parent != NULL )
	{
		GoInventory* inventory = parent->QueryInventory();
		if ( inventory != NULL )
		{
			if ( slot != NULL )
			{
				*slot = inventory->GetEquippedSlot( this );
				is = *slot != ES_NONE;
			}
			else
			{
				is = inventory->Contains( this );
			}
		}
	}

	return ( is );
}

bool Go :: IsItem( void ) const
{
	return ( !HasActor() && !HasParty() && HasAspect() );
}

bool Go :: IsJames( void ) const
{
	if ( same_no_case( GetTemplateName(), "james" ) )
	{
		gpgeneric( "James is in the house, wash up\n" );
		return ( true );
	}
	else
	{
		gpwarning( "Somebody kill me, I'm not James\n" );
		return ( false );
	}
}

bool Go :: IsMeleeWeapon( void ) const
{
	return ( IsWeapon() && GetAttack()->GetIsMelee() );
}

bool Go :: IsPotion( void ) const
{
	return ( HasPotion() );
}

bool Go :: IsRangedWeapon( void ) const
{
	return ( IsWeapon() && GetAttack()->GetIsProjectile() );
}

bool Go :: IsScreenParty( void ) const
{
	return ( this == gServer.GetScreenParty() );
}

bool Go :: IsScreenPartyMember( void ) const
{
	return ( HasParent() && (GetParent() == gServer.GetScreenParty() ) );
}

bool Go :: IsScreenPlayerOwned( void ) const
{
	return ( GetPlayer() == gServer.GetScreenPlayer() );
}

bool Go :: IsSelectable( void ) const
{
	const GoAspect* aspect = QueryAspect();
	return ( (aspect != NULL) ? aspect->GetIsSelectable() : true );
}

bool Go :: IsShield( void ) const
{
	return ( HasDefend() && GetDefend()->GetIsShield() );
}

bool Go :: IsSpawned( void ) const
{
	return ( m_Scid == SCID_SPAWNED );
}

bool Go :: IsSpell( void ) const
{
	return ( HasMagic() && GetMagic()->IsSpell() );
}

bool Go :: IsSpellBook( void ) const
{
	return ( HasGui() && GetGui()->IsSpellBook() );
}

bool Go :: IsUsable( void ) const
{
	return ( HasAspect() && GetAspect()->GetIsUsable() );
}

bool Go :: IsGhostUsable( void ) const
{
	return ( HasAspect() && GetAspect()->GetIsGhostUsable() );
}

bool Go :: IsWeapon( void ) const
{
	return ( !IsActor() && HasAttack() && GetAttack()->GetIsWeapon() );
}

bool Go :: IsGhost( void ) const
{
	return ( IsActor() && HasAspect() && GetAspect()->GetLifeState() == LS_GHOST );
}

bool Go :: IsTeamMember( Goid object ) const
{
	GoHandle hObject( object );
	if ( hObject.IsValid() && GetPlayer() && hObject->GetPlayer() )
	{
		Team * pTeam = gServer.GetTeam( GetPlayer()->GetId() );
		Team * pOtherTeam = gServer.GetTeam( hObject->GetPlayer()->GetId() );
		if ( pTeam && (pTeam == pOtherTeam) )
		{
			return true;
		}
	}

	return false;
}

void Go :: HandleMessage( const WorldMessage& msg )
{
	GPDEV_ONLY( AutoGoError autoGoError( this, &m_DevErrorCountScope ); )

	gpassert( (GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL) || (GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL) );
	gpassert( m_DbgInGoDb );

	// $ early bailout on cc'd messages to me if i'm not in the world and not omni
	if ( msg.IsCC() && !IsInAnyWorldFrustum() && !IsOmni() )
	{
		return;
	}

#	if ( !GP_RETAIL )

	if( TestWorldEventTraits( msg.GetEvent(), WMT_GO_IN_FRUSTUM ) )
	{
		if (   IsInGame( gWorldState.GetCurrentState() )
			&& (gWorldState.GetCurrentState() != WS_RELOADING)
			&& !IsInAnyWorldFrustum()
			&& !IsOmni()
			&& ::IsServerLocal() )
		{
			GoHandle sentFrom( msg.GetSendFrom() );
			gperrorf(( "Go %s, scid 0x%08x, player '%S' not allowed to receive world event %s while outside all valid world frusti.\nMessage sent from '%s', scid:0x%08x\n",
						GetTemplateName(),
						GetScid(),
						GetPlayer()->GetName().c_str(),
						::ToString( msg.GetEvent() ),
						GoidToDebugString( msg.GetSendFrom() ),
						sentFrom ? sentFrom->GetScid() : 0
						));
		}
	}

	if( !gServer.IsLocal() )
	{
		if( !TestWorldEventTraits( msg.GetEvent(), WMT_IS_CLIENT_OK ) && (GetGoidClass( GetGoid() ) != GO_CLASS_LOCAL) )
		{
				gperrorf(( "Go %s, scid 0x%08x, player '%S' not allowed to receive server-only world event '%s'.\n",
							GetTemplateName(),
							GetScid(),
							GetPlayer()->GetName().c_str(),
							::ToString( msg.GetEvent() ) ));
		}
	}

	if ( IsHuded() )
	{
		gpstring other;
		GoHandle from( msg.GetSendFrom() );
		if ( from )
		{
			other = from->GetTemplateName();
		}
		else
		{
			other.assignf( "0x%08X <invalid>", msg.GetSendFrom() );
		}

		gpgenericf(( "[%2.4f, 0x%08x] '%s' rcvd msg %s from '%s', msg sent %2.4f\n",
						gWorldTime.GetTime(),
						gWorldTime.GetSimCount(),
						GetTemplateName(),
						::ToString( msg.GetEvent() ),
						other.c_str(),
						msg.GetDebugSendTime()	));
	}

	if ( IsSelected() )
	{
		gpgomsgf(( "[r:%2.4f, s:%2.4f] '%s' rcvd msg %-20s from '%s'\n",
				gWorldTime.GetTime(),
				msg.GetDebugSendTime(),
				GetTemplateName(),
				::ToString( msg.GetEvent() ),
				GetGo(msg.GetSendFrom()) ? GetGo(msg.GetSendFrom())->GetTemplateName() : "<unknown>"
				));
	}

#	endif // !GP_RETAIL

	// go handle message
	if ( msg.GetEvent() == WE_REQ_DELETE )
	{
		// special handling for deletion request
		if ( !msg.IsCC() )
		{
			gGoDb.SMarkForDeletion( GetGoid() );
		}
	}
	else
	{
		// custom processing
		if ( !msg.IsCC() )
		{
			switch ( msg.GetEvent() )
			{
				case ( WE_CONSTRUCTED ):
				{
					// special: on construction, update the "wants" flags
					gpassert(   (GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL)
							 || (GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL) )

					ComponentColl::iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
					for ( i = ibegin ; i != iend ; ++i )
					{
						(*i)->DetectWantsBits();
					}
				}
				break;

				case ( WE_SELECTED   ):		gpassert( !m_IsSelected );		m_IsSelected = true;		break;
				case ( WE_DESELECTED ):		gpassert(  m_IsSelected );		m_IsSelected = false;		break;

				case ( WE_FOCUSED   ):		gpassert( !m_IsFocused );		m_IsFocused = true;			break;
				case ( WE_UNFOCUSED ):		gpassert(  m_IsFocused );		m_IsFocused = false;		break;

				case ( WE_HOTGROUP   ):		gpassert( !m_IsInHotGroup );	m_IsInHotGroup = true;		break;
				case ( WE_UNHOTGROUP ):		gpassert(  m_IsInHotGroup );	m_IsInHotGroup = false;		break;

				case ( WE_MOUSEHOVER   ):	gpassert( !m_IsMouseShadowed );	m_IsMouseShadowed = true;	break;
				case ( WE_UNMOUSEHOVER ):	gpassert(  m_IsMouseShadowed );	m_IsMouseShadowed = false;	break;

				case ( WE_DESTRUCTED ):
				{
					gMCP.RemoveGo( GetGoid() );
				}
				break;

				case ( WE_LEFT_WORLD ):
				{
					gMCP.RemoveGo( GetGoid() );
					Deselect();
				}
				break;

				case ( WE_KILLED ):
				{
					RCRemoveAllEnchantmentsOnSelf();
				}
				break;

				case ( WE_POST_RESTORE_GAME ):
				{
					// our frustum may have been ripped out from under us if
					// we're loading an MP game.
					if ( HasFrustum() && (gGoDb.GetGoFrustum( GetGoid() ) == FRUSTUMID_INVALID) )
					{
						m_HasFrustum = false;
					}

					// make sure we're up to date for min equip requirements
					if ( IsAnyHumanPartyMember() )
					{
						SetModifiersDirty();
						CheckModifierRecalc();
					}
				}
				break;
			}
		}

		// send to components
		ComponentColl::iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpassert( IsValidClientEvent( msg.GetEvent() ) || (GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL) );
			if ( msg.IsCC() )
			{
				(*i)->HandleCcMessage( msg );
			}
			else
			{
				(*i)->HandleMessage( msg );
			}
		}

		// omni go's that want updates get them starting now
		if ( msg.HasEvent( WE_CONSTRUCTED ) && !msg.IsCC() && IsOmni() && m_WantsUpdates )
		{
			PrivateStartUpdates();
		}
	}

	// siegefx handle messages if we're in the world
	if ( IsInActiveWorldFrustum() && !msg.IsCC() )
	{
		gGoDb.MessageEffectScripts( this, msg );
	}
}

void Go :: Update( float deltaTime )
{
	GPDEV_ONLY( AutoGoError autoGoError( this, &m_DevErrorCountScope ); )
	GPPROFILERSAMPLE( "Go::Update", SP_AI );

	gpassert( (GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL) || (GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL) );
	gpassert( IsInActiveWorldFrustum() || IsOmni() );
	gpassert( !IsMarkedForDeletion() );
	gpassert( m_WantsUpdates );
	gpassert( m_DbgInGoDb );
	gpassert( (m_RpcTotalBytes == 0) || IsGlobalGo() );

	// if we're the client the server may have told us to enter the world but
	// the node is not valid yet. so don't update just yet.
	if ( IsServerRemote() && HasPlacement() && !IsOmni() )
	{
		// $$$ check - is it pending entry???

		if ( !gSiegeEngine.IsNodeInAnyFrustum( GetPlacement()->GetPosition().node ) )
		{
			// $ early bailout - don't update until we are ready for it
			return;
		}
	}

	// update components
	ComponentColl::iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i)->GetWantsUpdates() )
		{
			(*i)->Update( deltaTime );

			if ( !IsOmni() && !IsInActiveWorldFrustum() )
			{
				break;
			}
		}
	}

	// if we're enchanted, we're always dirty. that way we don't have to worry
	// about always dirtying things properly to keep the UI up to date.
	if ( m_IsEnchanted )
	{
		SetModifiersDirty();
	}

	// check for recalc
	CheckModifierRecalc();

	// one of those updates may have pulled us out of the world
	if ( IsInActiveWorldFrustum() && HasFrustum() )
	{
		// force update of pos for siege frustum positioning
		GetPlacement()->ResetPosition();
	}
}

void Go :: UpdateSpecial( float deltaTime )
{
	GPDEV_ONLY( AutoGoError autoGoError( this, &m_DevErrorCountScope ); )
	GPPROFILERSAMPLE( "Go::UpdateSpecial", SP_AI );

	gpassert( IsGlobalGo() );
	gpassert( m_DbgInGoDb );

	// update components
	ComponentColl::iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)->UpdateSpecial( deltaTime );
	}

	// only if we're out of the world frustum so no call these twice...
	if ( !IsUpdating() && !IsInActiveWorldFrustum() )
	{
		// check for all-clients go's recalc
		CheckModifierRecalc();
	}
}

void Go :: Draw( void )
{
	GPDEV_ONLY( AutoGoError autoGoError( this, &m_DevErrorCountScope ); )

	gpassert( (GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL) || (GetGoidClass( GetGoid() ) == GO_CLASS_LOCAL) );
	gpassert( m_DbgInGoDb );

	// no draw twice!
	gpassert( m_LastSimViewOccupant != gWorldTime.GetSimCount() );

	// remember last draw
	m_LastSimViewOccupant = gWorldTime.GetSimCount();

	// draw find-items highlight
	if (   gWorldOptions.GetIndicateItems()
		&& !HasParent()
		&& HasGui()
		&& IsItem()
		&& IsSelectable()
		&& ( HasAspect() && GetAspect()->GetIsScreenPlayerVisible() ) )
	{
		gGoDb.GetVisibleItems().insert( this );
	}

	// anybody wanna draw?
	if ( m_WantsDraws )
	{
		// draw components
		ComponentColl::iterator i, ibegin = GetClientComponentBegin(), iend = GetComponentEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->GetWantsDraws() )
			{
				(*i)->Draw();
			}
		}
	}
}

void Go :: SetModifiersDirty( bool set )
{
	// $ note that updating children is not necessary. if their equip states
	//   change they notify their parent, not the other way around.

	m_ModifiersDirty = set;

	if ( set && HasParent() )
	{
		GetParent()->SetModifiersDirty();
	}
}

bool Go :: CheckModifierRecalc( void )
{
	// if not dirty, ignore completely
	if ( !m_ModifiersDirty )
	{
		return ( false );
	}

	// ok go for it
	PrivateRecalcModifiers();
	return ( true );
}

void Go :: SetBucketDirty( bool set )
{
	if ( set && !m_IsBucketDirty )
	{
		// skip spurious "dirty" checks when we're constructing a go, because the
		// server and client will be constructing exactly the same in parallel.
		if ( !gGoDb.IsCurrentThreadConstructingGo() )
		{
			// check for invalid goid, which happens when shutting down
			if ( (m_Goid != GOID_INVALID) && IsGlobalGo() )
			{
				// godb must know about this
				GPDEV_ONLY( gGoDb.AddDirtyGoBucket() );

				// set self dirty
				m_IsBucketDirty = true;
			}

			// tell all parents
			if ( m_Parent != NULL )
			{
				m_Parent->SetBucketDirty();
			}
		}
	}
	else if ( !set && m_IsBucketDirty )
	{
		// want to clear it eh?
		m_IsBucketDirty = false;

		// check for invalid goid, which happens when shutting down
#		if !GP_RETAIL
		if ( (m_Goid != GOID_INVALID) && IsGlobalGo() )
		{
			// godb must know about this
			gGoDb.AddDirtyGoBucket( false );
		}
#		endif // !GP_RETAIL
	}
}

void Go :: SetServerOnly( bool set )
{
	if ( m_IsServerOnly != set )
	{
		m_IsServerOnly = set;
		GPDEV_ONLY( gGoDb.AddServerOnlyCount( set ) );
	}
}

void Go :: EnableAutoExpiration( bool set )
{
	gpassert( !IsCloneSourceGo() );
	if ( m_EnableAutoExpiration != set )
	{
		m_EnableAutoExpiration = set;
		if ( m_EnableAutoExpiration )
		{
			// if we're out of the world and reenabling expiration, start auto expire again
			if ( !IsInAnyWorldFrustum() && IsPendingEntry() )
			{
				gGoDb.StartExpiration( this, false );
			}
		}
		else
		{
			// cancel any auto expiration we were doing
			gGoDb.CancelExpiration( this, false );
		}
	}
}

#if !GP_RETAIL

Go* GetDebugDrawParent( Go* go )
{
	// no aspect? it's top level
	gpassert( go != NULL );
	if ( !go->HasAspect() )
	{
		return ( go );
	}

	// not nema root? find root
	while ( !go->GetAspect()->IsNemaRoot() )
	{
		go = GoHandle( go->GetAspect()->GetAspectPtr()->GetParent()->GetGoid() );
	}

	// only go up more if we're inside inventory
	while ( go->IsInsideInventory() )
	{
		go = go->GetParent();
	}

	// must be it!
	return ( go );
}

int GetDebugDrawChildCount( Go* go )
{
	int count = 0;

	// only care about inventories
	if ( go->HasInventory() )
	{
		GopColl::const_iterator i, ibegin = go->GetChildBegin(), iend = go->GetChildEnd();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->IsInsideInventory() )
			{
				++count;
				count += GetDebugDrawChildCount( *i );
			}
		}
	}

	// done
	return ( count );
}

bool HasDebugDrawParent( Go* go )
{
	return ( GetDebugDrawParent( go ) != NULL );
}

void GetDebugDrawTopCenter( Go* go, SiegePos& pos )
{
	pos = go->GetPlacement()->GetPosition();

	if ( go->HasAspect() )
	{
		vector_3 halfDiag( DoNotInitialize );
		go->GetAspect()->GetNodeSpaceOrientedBoundingVolume( pos.pos, halfDiag );
		pos.pos.y += halfDiag.y;
	}
	else
	{
		pos.pos.y += 1.0f;
	}
}

void GetDebugDrawParentTopCenter( Go* go, SiegePos& pos )
{
	GetDebugDrawTopCenter( GetDebugDrawParent( go ), pos );
}

void Go :: DrawDebugHud( void )
{
	// draw debug huds
	if ( IsHuded() )
	{
		// draw generic Go HUD
		if ( HasPlacement() )
		{
			if ( (GetErrorCount() > 0) || (GetWarningCount() > 0 ) )
			{
				gpstring errors;
				errors.assignf( "%d", GetErrorCount() );

				gpstring warnings;
				warnings.assignf( "%d", GetWarningCount() );

				gWorld.DrawDebugPoint( GetPlacement()->GetPosition(), 0.0f, 0, 3.2f, errors  , ( GetErrorCount() > 0 )   ? COLOR_RED : COLOR_GREEN );
				gWorld.DrawDebugPoint( GetPlacement()->GetPosition(), 0.0f, 0, 2.8f, warnings, ( GetWarningCount() > 0 ) ? COLOR_YELLOW : COLOR_GREEN );
			}

			gpstring sLabel;

			if ( !GetCommon()->GetDevInstanceText().empty() )
			{
				sLabel.appendf( "%s\n", GetCommon()->GetDevInstanceText().c_str() );
			}

			sLabel.appendf( "%s\nScid[0x%08x] Goid[0x%08x]\n", GetTemplateName(), GetScid(), GetGoid() );

			float timeLeft = 0;
			if ( gGoDb.IsExpiring( this, &timeLeft ) )
			{
				sLabel.appendf( "expiring in %.2f seconds\n", timeLeft );
			}

			if ( HasParent() && !GetParent()->GetCommon()->GetScreenName().empty() )
			{
				sLabel.appendf( "parent screen_name = %S\n", GetParent()->GetCommon()->GetScreenName().c_str() );
			}

			// screen name
			if ( !GetCommon()->GetScreenName().empty() )
			{
				sLabel.appendf( "screen_name = %S\n", GetCommon()->GetScreenName().c_str() );
			}

			gWorld.DrawDebugPoint( GetPlacement()->GetPosition(), 0.1f, MAKEDWORDCOLOR( vector_3( 0, 0.5 ,1.0 ) ), sLabel );

			// Draw any use points

			GoPlacement::UsePointColl coll;
			DWORD num = GetPlacement()->GetUsePoints(coll);
			for (DWORD i = 0 ; i < num; ++i)
			{
				gpstring plabel;
				plabel.assignf("use point # %d",i);
				gWorld.DrawDebugDirectedLine(GetPlacement()->GetPosition(),
											 coll[i],
											 0xFFFFCC66, plabel );
			}

		}

		{
			RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

			// draw components
			ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
			for ( i = begin ; i != end ; ++i )
			{
				(*i)->DrawDebugHud();
			}
		}
	}
	else
	{
		// Show the trigger info, even if we aren't showing the huds when they are turned on
		// at the console
		if (gWorldOptions.GetShowTriggerSys())
		{
			if (HasCommon() && (GetCommon()->HasInstanceTriggers() || GetCommon()->HasTemplateTriggers()))
			{
				GetCommon()->DrawDebugTriggers();
			}
		}
	}

	if (gWorldOptions.GetShowPathsMCP() && HasFollower())
	{
		GetFollower()->DebugDraw();
		if (!gAppModule.IsUserPaused())
		{
			// Disable this until further notice -- biddle
			// gMCP.Dump(GetGoid(),&gMCPSequencerContext);
		}
	}

	if ( gWorldOptions.GetShowMpUsage() && IsGlobalGo() && IsInViewFrustum() )
	{
		// who is responsible for my hud?
		Go* hudGo = GetDebugDrawParent( this );

		// get info
		GoDb::HudInfo& info = gGoDb.GetHudInfo( hudGo );

		// figure out where to draw
		SiegePos topCenter;
		GetDebugDrawTopCenter( hudGo, topCenter );
		topCenter.pos.y += 0.1f;

		// ok let's do the overall construction cost of this thing
		SiegePos costCenter( topCenter );
		costCenter.pos.y += 0.1f;
		costCenter.pos.y += info.m_RpcCtorStackCount * 0.1f;
		++info.m_RpcCtorStackCount;
		DWORD color = IsServerOnly() ? 0x8000CC00 : CalcBucketDirty() ? 0x80FF0000 : 0x80FFFF00;
		gWorld.DrawDebugWedge(
				costCenter,
				0.2f,
				vector_3( 1, 0, 0 ),
				0,
				color,
				true );
		if ( m_RpcCreateBytes > 0 )
		{
			gWorld.DrawDebugScreenLabel(
					costCenter,
					gpstringf( "%d", m_RpcCreateBytes ) );
		}

		// now cost per frame
		if ( m_RpcFrameBytes > 0 )
		{
			// smaller circle as a % of 100 bytes
			gWorld.DrawDebugWedge(
					costCenter,
					0.15f,
					vector_3( 1, 0, 0 ),
					(PI2 * m_RpcFrameBytes) / 100.0f,
					0xA0000000,
					true );

			SiegePos frameCenter( topCenter );
			frameCenter.pos.y += 0.1f;
			frameCenter.pos.y += (GetDebugDrawChildCount( hudGo ) + info.m_RpcFrameStackCount) * 0.1f;
			++info.m_RpcFrameStackCount;

			gWorld.DrawDebugWorldLabelScroll(
					frameCenter,
					gpstringf( "%s: %d", GetTemplateName(), m_RpcFrameBytes ),
					(m_RpcFrameBytes > 100) ? COLOR_LIGHT_RED : COLOR_WHITE,
					3.0f,
					true );

			m_RpcFrameBytes = 0;
		}
	}

	if ( gWorldOptions.GetShowObjectCost() && IsInViewFrustum() && HasAspect() )
	{
		// figure out where to draw
		SiegePos drawpos( GetPlacement()->GetPosition() );
		drawpos.pos.y += ( 1.25f + ( 0.66f * GetAspect()->GetDisplayCost() ) );

		DWORD color = IsLodfi() ? 0x40808000 : 0x40C04000 ;
		
		gWorld.DrawDebugWedge(
					drawpos,
					GetAspect()->GetDisplayCost(),
					vector_3( 1, 0, 0 ),
					(float) PI2,
					color,
					true );
	}
}

void Go :: Dump( bool verbose, ReportSys::ContextRef ctx )
{
	RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

	// dump all components
	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		(*i)->Dump( verbose, ctx );
	}
}

#endif // !GP_RETAIL

DWORD Go :: PlayVoiceSound( const char* eventName, bool local3d )
{
	// $ check for direct play
	gpassert( eventName != NULL );
	if ( *eventName == '#' )
	{
		return ( PlaySound( eventName + 1, local3d ) );
	}

	gpstring pri;
	gpstring sound = GetVoiceFromEvent( eventName, &pri );
	return ( PlaySound( sound, local3d, GetSampleTypeFromString( pri ) ) );
}

void Go :: SPlayVoiceSound( const char* eventName, bool local3d )
{
	if ( !IsGlobalGo() )
	{
		PlayVoiceSound( eventName, local3d );
	}
	else
	{
		RCPlayVoiceSound( eventName, local3d );
	}
}

void Go :: RCPlayVoiceSound( DWORD machineId, const char* eventName, bool local3d )
{
	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCPlayVoiceSoundPeek( eventName, local3d );
}

void Go :: RCPlayVoiceSoundPeek( const char* eventName, bool local3d )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCPlayVoiceSoundPeek );
	PlayVoiceSound( eventName, local3d );
}

void Go :: RCPlayVoiceSoundId( DWORD machineId, eVoiceSound event, bool local3d )
{
	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCPlayVoiceSoundIdPeek( event, local3d );
}

void Go :: RCPlayVoiceSoundIdPeek( eVoiceSound event, bool local3d )
{
	FUBI_RPC_TAG();

	if ( GetVoiceFromEvent( ToEventString( event ) ).empty() )
	{
		return;
	}

	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCPlayVoiceSoundIdPeek );

	PlayVoiceSound( ToEventString( event ), local3d );
}

DWORD Go :: PlayVoiceSound( const char* eventName, float minDist, float maxDist, bool bLoop )
{
	// $ check for direct play
	gpassert( eventName != NULL );
	if ( *eventName == '#' )
	{
		return ( PlaySound( eventName + 1, GPGSound::SND_EFFECT_NORM, minDist, maxDist, bLoop ) );
	}

	gpstring pri;
	gpstring sound = GetVoiceFromEvent( eventName, &pri );
	return ( PlaySound( sound, GetSampleTypeFromString( pri ), minDist, maxDist, bLoop ) );
}

void Go :: SPlayMaterialSound( const char* eventName, Goid dstGoid, bool local3d )
{
	if ( !IsGlobalGo() )
	{
		PlayMaterialSound( eventName, dstGoid, local3d );
	}
	else
	{
		RCPlayMaterialSound( eventName, dstGoid, local3d );
	}
}

void Go :: RCPlayMaterialSound( const char* eventName, Goid dstGoid, bool local3d )
{
	FUBI_RPC_THIS_CALL( RCPlayMaterialSound, RPC_TO_ALL );

	PlayMaterialSound( eventName, dstGoid, local3d );
}

DWORD Go :: PlayMaterialSound( const char* eventName, Goid dstGoid, bool local3d )
{
	GoHandle dstGo( dstGoid );
	return ( PlayMaterialSound( eventName, dstGo ? dstGo->GetMaterial() : NULL, local3d ) );
}

void Go :: SPlayMaterialSound( const char* eventName, const char* dstMaterial, bool local3d )
{
	if ( !IsGlobalGo() )
	{
		PlayMaterialSound( eventName, dstMaterial, local3d );
	}
	else
	{
		RCPlayMaterialSound( eventName, dstMaterial, local3d );
	}
}

void Go :: RCPlayMaterialSound( const char* eventName, const char* dstMaterial, bool local3d )
{
	FUBI_RPC_THIS_CALL( RCPlayMaterialSound, RPC_TO_ALL );

	PlayMaterialSound( eventName, dstMaterial, local3d );
}

DWORD Go :: PlayMaterialSound( const char* eventName, const char* dstMaterial, bool local3d )
{
	// $ check for direct play
	gpassert( eventName != NULL );
	if ( *eventName == '#' )
	{
		return ( PlaySound( eventName + 1, local3d ) );
	}

	gpstring pri;
	gpstring sound = gContentDb.FindMaterialSound( eventName, pri, GetMaterial(), dstMaterial );
	return ( PlaySound( sound, local3d, GetSampleTypeFromString( pri ) ) );
}

DWORD Go :: PlaySound( const char* soundName, bool local3d, GPGSound::eSampleType sType )
{
	// check for invalid name
	if ( (soundName == NULL) || (*soundName == '\0') )
	{
		return ( GPGSound::INVALID_SOUND_ID );
	}

	// check for gag order
	if ( HasAspect() && GetAspect()->GetIsGagged() )
	{
		return ( GPGSound::INVALID_SOUND_ID );
	}

	// check for placement
	if ( local3d && HasPlacement() )
	{
		// play the sample
		return ( gWorldSound.PlaySample( soundName, GetPlacement()->GetPosition(), false, gWorldSound.GetDefaultMinDist(), gWorldSound.GetDefaultMaxDist(), sType ) );
	}
	else
	{
		// play the sample
		return ( gSoundManager.PlaySample( soundName, false, sType ) );
	}
}

DWORD Go :: PlaySound( const char* soundName, GPGSound::eSampleType sType,
					   float minDist, float maxDist, bool bLoop )
{
	// check for invalid name
	if ( (soundName == NULL) || (*soundName == '\0') )
	{
		return ( GPGSound::INVALID_SOUND_ID );
	}

	// check for gag order
	if ( HasAspect() && GetAspect()->GetIsGagged() )
	{
		return ( GPGSound::INVALID_SOUND_ID );
	}

	// check for placement
	if ( HasPlacement() )
	{
		// play the sample
		return ( gWorldSound.PlaySample( soundName, GetPlacement()->GetPosition(), bLoop, minDist, maxDist, sType ) );
	}

	return GPGSound::INVALID_SOUND_ID;
}

void Go :: StopSound( DWORD soundId )
{
	// Stop the sample
	gSoundManager.StopSample( soundId );
}

gpstring Go :: GetVoiceFromEvent( const char * eventName, gpstring * priority )
{
	gpstring voice, pri;

	// 1. try my voice block
	if ( HasAspect() )
	{
		voice = GetAspect()->FindEventVoice( eventName, pri );
	}

	// 2. try material type of "me", dest of "generic" (if i'm not generic)
	if ( voice.empty() && !same_no_case( GetMaterial(), "generic" ) )
	{
		voice = gContentDb.FindMaterialSound( eventName, pri, GetMaterial() );
	}

	// 3. try global_voice in contentdb
	if ( voice.empty() )
	{
		voice = gContentDb.FindGlobalVoiceSound( eventName, pri );
	}

	if( priority )
	{
		*priority = pri;
	}
	return voice;
}

GPGSound::eSampleType Go :: GetSampleTypeFromString( const gpstring& pri )
{
	// Compare the string with known priorities
	if( pri.same_no_case( "low" ) )
	{
		return GPGSound::SND_EFFECT_LOW;
	}
	else if( pri.same_no_case( "norm" ) )
	{
		return GPGSound::SND_EFFECT_NORM;
	}
	else if( pri.same_no_case( "high" ) )
	{
		return GPGSound::SND_EFFECT_HIGH;
	}
	else if( pri.same_no_case( "always" ) )
	{
		return GPGSound::SND_EFFECT_AMBIENT;
	}

	return GPGSound::SND_EFFECT_NORM;
}

bool Go :: Xfer( FuBi::PersistContext& persist )
{
	gpassert( persist.IsSaving() == m_DbgInGoDb );
	gpassert( persist.IsFullXfer() );
	gpassert( !IsMarkedForDeletion() );
	gpassert( !m_IsLodfi );
	gpassert( (m_RefCount == 0) || (::GlobalGetUnfilteredHandledExceptionCount() != 0) );
	gpassert( !m_IsCommittingCreation );
	gpassert( !m_Transferring );
	gpassert( m_PlacementChanging == 0 );

// Xfer simple members.

	// $ special: goid only xfer'd after everything else is done

	persist.Xfer   ( "m_Goid",          m_Goid          );
	persist.Xfer   ( "m_Scid",          m_Scid          );
	persist.Xfer   ( "m_DataTemplate",  m_DataTemplate  );
	persist.XferHex( "m_RandomSeed",    m_RandomSeed    );
	persist.Xfer   ( "m_MpPlayerCount", m_MpPlayerCount );
	persist.Xfer   ( "m_CloneSource",   m_CloneSource   );
	persist.Xfer   ( "m_RegionSource",  m_RegionSource  );

	// $ early bailout if the template was bad
	if ( m_DataTemplate == NULL )
	{
		gpassert( persist.IsRestoring() );
		return ( false );
	}

#	if !GP_RETAIL
	persist.XferQuoted( "m_DevInstanceFuel",    m_DevInstanceFuel    );
	persist.Xfer      ( "m_DevWarningCount",    m_DevWarningCount    );
	persist.Xfer      ( "m_DevErrorCount",      m_DevErrorCount      );
	persist.Xfer      ( "m_DevErrorCountScope", m_DevErrorCountScope );
#	endif

	bool isUpdating = m_NextUpdate != NULL;
	persist.Xfer( "isUpdating", isUpdating );
	if ( persist.IsRestoring() && isUpdating )
	{
		PrivateStartUpdates();
	}

	persist.Xfer      ( "m_FrustumMembership",   m_FrustumMembership   );
	persist.Xfer      ( "m_Parent",              m_Parent              );
	persist.Xfer      ( "m_Player",              m_Player              );
	persist.XferVector( "m_ChildColl",           m_ChildColl           );
	persist.XferHex   ( "m_LastSimViewOccupant", m_LastSimViewOccupant );

#	if !GP_RETAIL
	persist.XferHex ( "m_CreateTime", m_CreateTime );
#	endif // !GP_RETAIL

	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsOmni",               m_IsOmni                     );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsAllClients",         m_IsAllClients               );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_WantsUpdates",         m_WantsUpdates               );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_WantsDraws",           m_WantsDraws                 );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsCommand",            m_IsCommand                  );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsCommandCached",      m_IsCommandCached            );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsPContentInventory",  m_IsPContentInventory        );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsPContentGenerated",  m_IsPContentGenerated        );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsServerOnly",         m_IsServerOnly               );
	FUBI_XFER_BITFIELD_BOOL_DEF( persist, "m_EnableAutoExpiration", m_EnableAutoExpiration, true );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_HasFrustum",           m_HasFrustum                 );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsFocused",            m_IsFocused                  );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsSelected",           m_IsSelected                 );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsMouseShadowed",      m_IsMouseShadowed            );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsInHotGroup",         m_IsInHotGroup               );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_PendingUpdateRemoval", m_PendingUpdateRemoval       );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsExpiring",           m_IsExpiring                 );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsPendingEntry",       m_IsPendingEntry             );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsBucketDirty",        m_IsBucketDirty              );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsFading",             m_IsFading                   );
	FUBI_XFER_BITFIELD_BOOL    ( persist, "m_IsInsideInventory",    m_IsInsideInventory          );

	if ( persist.IsRestoring() )
	{
		m_IsFadingFirstView = true;
	}

// Xfer components.

	gpassert( persist.IsSaving() || m_ComponentColl.empty() );
	if ( !persist.IsFullXfer() && persist.IsRestoring() )
	{
		gpassert( m_ComponentColl.empty() );

		// look up fuel
		FastFuelHandle fuel = gWorldMap.MakeObjectFuel( m_Scid, m_RegionSource );

		// optionally force addition of edit component if we're in edit mode
#		if !GP_RETAIL
		{
			if ( gGoDb.IsEditMode() || gWorldOptions.GetTuningLoad() )
			{
				MakeEditable();
			}
		}
#		endif // !GP_RETAIL

		// instantiate template components
		GoDataTemplate::ComponentIter i,
				begin = m_DataTemplate->GetComponentBegin(),
				end   = m_DataTemplate->GetComponentEnd();
		for ( i = begin ; i != end ; ++i )
		{
			// create component
			bool serverComponent = false;
			GoComponent* newComponent = gContentDb.CreateGoComponent( this, *i, fuel, serverComponent, false );

			// add it to the collection
			AddNewComponent( newComponent, serverComponent );
		}
	}

// Finish.

	if ( persist.IsRestoring() )
	{
		m_ModifiersDirty = true;
		m_IsSpaceDirty   = true;
		m_IsWobbDirty    = true;
		m_IsRenderDirty  = true;

		if ( !persist.IsFullXfer() )
		{
			ResetCache();
		}
	}

	return ( true );
}

bool Go :: XferComponents( FuBi::PersistContext& persist )
{

// Create components.

	typedef stdx::fast_vector <int> IndexColl;

	gpstring name;
	IndexColl skipColl;

	if ( persist.IsRestoring() )
	{
		persist.EnterColl( "m_ComponentColl" );

		// look up fuel for me
		FastFuelHandle fuel;
		if ( m_RegionSource != REGIONID_INVALID )
		{
			fuel = gWorldMap.MakeObjectFuel( m_Scid, m_RegionSource );
		}

		// build and xfer components
		for ( int index = 0 ; persist.AdvanceCollIter() ; ++index )
		{
			// figure out the name
			persist.Xfer( "_name", name );

			// get some vars
			bool serverComponent = false;
			GoComponent* newGoc = NULL;

			// look it up in my template
			const GoDataComponent* godc = m_DataTemplate->FindComponentByName( name );
			if ( godc != NULL )
			{
				// construct the standard component
				newGoc = gContentDb.CreateGoComponent( this, *godc, fuel, serverComponent, false );
			}
			else
			{
				// hey it was a custom added component, create it
				newGoc = gContentDb.CreateRawGoComponent( this, name, serverComponent );

				// init it
				if ( newGoc != NULL )
				{
					newGoc->Init( gContentDb.FindComponentByName( name ) );
				}
			}

			// bad?
			if ( newGoc != NULL )
			{
				// add it to the collection
				AddNewComponent( newGoc, serverComponent );
			}
			else
			{
				// skip this one, trouble constructing a component means that
				// it probably doesn't exist any more
				skipColl.push_back( index );
				gpreport( persist.GetReportContext(),
					( "Unable to create/initialize component named '%s' for goid 0x%08X (%s)\n",
					  name.c_str(), m_Goid, GetTemplateName() ) );
			}
		}

		persist.LeaveColl();
	}

// Update caches.

#	if !GP_RETAIL
	{
		// special: tell edit component about it
		if ( HasEdit() )
		{
			GetEdit()->CommitCreation();
		}
	}
#	endif // !GP_RETAIL

	if ( persist.IsRestoring() )
	{
		ResetCache();
	}

// Xfer data.

	bool success = true;
	{
		persist.EnterColl( "m_ComponentColl" );

		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		IndexColl::iterator skipIter = skipColl.begin();
		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		int index = 0;
		for ( i = begin ; i != end ; ++i, ++index )
		{
			// advance state
			persist.AdvanceCollIter();

			// skip as needed
			while ( (skipIter != skipColl.end()) && (*skipIter == index) )
			{
				persist.AdvanceCollIter();
				++index;
			}

			// persist the type
			if ( persist.IsSaving() )
			{
				name = (*i)->GetName();
			}
			persist.Xfer( "_name", name );
			if ( persist.IsRestoring() )
			{
				if ( !(*i)->GetName().same_no_case( name ) )
				{
					gpreportf( persist.GetReportContext(),
						( "Content mismatch! The ordering of components has "
						  "changed since goid 0x%08X scid 0x%08X ('%s') was "
						  "saved in this file. Unable to persist component "
						  "'%s' (data in that slot is for component '%s')...\n",
						  m_Goid, m_Scid, GetTemplateName(),
						  (*i)->GetName().c_str(), name.c_str() ));
					success = false;
					continue;
				}
			}

			// xfer contents
			(*i)->XferWantsBits( persist );
			(*i)->Xfer( persist );
		}

		persist.LeaveColl();
	}

	// finish
	if ( persist.IsRestoring() )
	{
		UpdateWantsBits();
	}

	return ( success );
}

bool Go :: XferCharacter( FuBi::PersistContext& persist )
{
	bool success = true;

	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		// xfer components
		ComponentColl::iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( !(*i)->XferCharacter( persist ) )
			{
				success = false;
			}
		}

		// if restoring, commit components
		if ( persist.IsRestoring() )
		{
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( !(*i)->CommitImport( persist.IsFullXfer() ) )
				{
					success = false;
				}
			}
		}
	}	

	// update quest bits
	gGoDb.XferQuestBits( persist, GetGoid() );

	// make sure we recalc everything, unless we're in the middle of a go
	// creation (#9802). in that case, defer and have a higher level function
	// do the work (i.e. the original requestor of the SCloneGo via
	// XferCharacterPos).
	if ( persist.IsRestoring() && !gGoDb.IsConstructingGo() )
	{
		XferCharacterPost( persist.IsFullXfer() );
	}

	// done
	return ( success );
}

bool Go :: XferCharacterPost( bool isFullXfer )
{
	// recalc everything
	PrivateRecalcModifiers();

	// start out fully charged if we're not importing a full xfer. this post-
	// xfer step is required in case you've got stats-buffing equipment on.
	// need to crank your life/mana to the max in that case.
	if ( !isFullXfer && HasActor() && GetActor()->GetCanLevelUp() )
	{
		GetAspect()->SetCurrentMana( GetAspect()->GetMaxMana() );
		GetAspect()->SetCurrentLife( GetAspect()->GetMaxLife() );
	}

	return ( true );
}

bool Go :: XferForSync( FuBi::BitPacker& packer, eSyncType syncType )
{
	CHECK_PRIMARY_THREAD_ONLY;

	// $ this is a general jit sync function - add as necessary

// First sync the type.

	gpassert( (syncType == SYNC_DEFAULT) == packer.IsRestoring() );
	packer.XferRaw( syncType, FUBI_MAX_ENUM_BITS( SYNC_ ) );

	// cache component pointers
	GoCommon   * common    = QueryCommon  ();
	GoGui      * gui       = QueryGui     ();
	GoMagic    * magic     = QueryMagic   ();
	GoAspect   * aspect    = QueryAspect  ();
	GoActor    * actor     = QueryActor   ();
	GoFollower * follower  = QueryFollower();

// Sync [follower].

	// always sync followers
	if ( follower != NULL )
	{
		if ( packer.IsSaving() )
		{
			MCP::Plan* syncPlan = gMCP.FindPlan(GetGoid(),false);
			if ( (syncPlan != NULL) && syncPlan->NeedsRefresh() )
			{
				packer.WriteBit( 1 );
				// Fetch the refresh from the MCP plan for this Go
				syncPlan->BuildPackedRefresh( packer );
			}
			else
			{
				packer.WriteBit( 0 );
			}
		}
		else if ( packer.ReadBit() )
		{
			// Give the refresh to the GoFollower
			follower->UnpackRefresh( packer );
		}
	}

// $ EARLY BAILOUT.

	// early out maybe
	if ( syncType == SYNC_FOR_ALL_CLIENTS )
	{
		// effect scripts
		gGoDb.XferFxForSync( packer, this );

		// done!
		return ( true );
	}

// Sync pcontent.

	bool isPcontent = IsPContentGenerated();
	packer.XferBit( isPcontent );

	// variation, modifiers - anything that can affect gui (tooltips etc)
	if ( isPcontent )
	{
		// variation
		gpstring variation = gui ? gui->GetVariation() : "";
		packer.XferString( variation, 1 );

		// prefix modifier
		gpstring prefix = magic ? magic->GetPrefixModifierName() : "";
		packer.XferString( prefix, 1 );

		// suffix modifier
		gpstring suffix = magic ? magic->GetSuffixModifierName() : "";
		packer.XferString( suffix, 1 );

		// apply
		if ( packer.IsRestoring() )
		{
			PContentReq pcontentReq;
			pcontentReq.SetFinish( variation );
			pcontentReq.SetModifier( 0, gPContentDb.FindModifier( prefix ) );
			pcontentReq.SetModifier( 1, gPContentDb.FindModifier( suffix ) );
			gPContentDb.ApplyPContentRequest( this, pcontentReq );
		}
	}

	// now xfer the other stuff that's calc'd in pcontent requests too
	if ( (isPcontent || (syncType == SYNC_FULL)) && gui && aspect )
	{
		// gold value
		int goldValue = aspect->GetGoldValue();
		packer.XferCount( goldValue );

		// item power
		float itemPower = gui->GetItemPower();
		packer.XferFloat( itemPower );

		// set them separately so as not to accidentally cause dirtying
		if ( packer.IsRestoring() )
		{
			aspect->SetGoldValue( goldValue );
			gui->SetItemPower( itemPower );
		}
	}

	// need to xfer some life stuff for summoned creatures
	if ( actor != NULL )
	{
		// max life value
		float maxLife = aspect->GetMaxLife();
		packer.XferFloat( maxLife );
		if ( packer.IsRestoring() )
		{
			// we're the client, and enchantments don't xfer over, so just set
			// both of these to be the same, 'cause that's what shows up in
			// the mouseover.
			aspect->SetNaturalMaxLife( maxLife );
			aspect->SetMaxLife( maxLife );
		}

		// current life value, assume already set to max life
		float currentLife = aspect->GetCurrentLife();
		packer.XferFloatIf( currentLife, maxLife );
		if ( packer.IsRestoring() )
		{
			aspect->SetCurrentLife( currentLife );
		}
	}

// $ EARLY BAILOUT.

	// the only thing we sync on non-standalone creation is pcontent stuff. why?
	// well template name is not enough for things that are created by pcontent.
	// also need variation and modifiers.
	if ( syncType != SYNC_FULL )
	{
		return ( isPcontent || (actor != NULL) );
	}

// Sync [common].

	if ( common != NULL )
	{
		// membership
		bool diffMembership = !common->IsMembershipDefault();
		packer.XferBit( diffMembership );
		if ( diffMembership )
		{
			Membership membership = common->GetMembership();
			membership.Xfer( packer );
			if ( packer.IsRestoring() )
			{
				common->CopyMembership( membership );
			}
		}

		// triggers
		bool has_trigger_data;
		if (packer.IsSaving())
		{
			has_trigger_data = common->HasInstanceTriggers() || common->HasTemplateTriggers();
		}
		packer.XferBit(has_trigger_data);

		if ( has_trigger_data )
		{
			if ( !common->HasInstanceTriggers() &&  !common->HasTemplateTriggers() )
			{
				gperrorf((
					"You have received trigger sync data for a GO [%s:g:%08x:s:%08x] that has no triggers!!",
					GetTemplateName(),
					GetGoid(),
					GetScid()
					));
			}
			common->XferForSync(packer);
		}
	}

// Sync [aspect].

	if ( aspect != NULL )
	{

	// Do simple trait bits.

		bool isVisible       = aspect->GetIsVisible();
		bool isSelectable    = aspect->GetIsSelectable();
		bool isUsable        = aspect->GetIsUsable();
		bool isGhostUsable   = aspect->GetIsGhostUsable();

		packer.XferBit( isVisible     );
		packer.XferBit( isSelectable  );
		packer.XferBit( isUsable      );
		packer.XferBit( isGhostUsable );

		if ( packer.IsRestoring() )
		{
			aspect->SetIsVisible    ( isVisible     );
			aspect->SetIsSelectable ( isSelectable  );
			aspect->SetIsUsable     ( isUsable      );
			aspect->SetIsGhostUsable( isGhostUsable );
		}

	// Do simple trait vars.

		// life state
		eLifeState lifeState = aspect->GetLifeState();
		packer.XferRaw( lifeState, FUBI_MAX_ENUM_BITS( LS_ ) );
		aspect->SetLifeState( lifeState );

		// $ actors were sync'd above
		if ( actor == NULL )
		{
			// current life value
			float currentLife = aspect->GetCurrentLife();
			packer.XferFloat( currentLife );
			aspect->SetCurrentLife( currentLife );
		}

	// Do inventory.

		// inside inventory
		bool isInInventory = IsInsideInventory();
		packer.XferBit( isInInventory );

		// equip slot
		eEquipSlot equipSlot = ES_NONE;
		IsInsideInventory( &equipSlot );
		packer.XferRaw( equipSlot, FUBI_MAX_ENUM_BITS( ES_ ) );

		eInventoryLocation invSlot = IL_MAIN;
		if ( isInInventory )
		{
			Go* parent = GetParent();
			if ( (parent != NULL) && parent->HasInventory() )
			{				
				if ( packer.IsSaving() )
				{
					invSlot = parent->GetInventory()->GetLocation( this );
				}
				
				packer.XferRaw( invSlot, FUBI_MAX_ENUM_BITS( IL_ ) );
			}
		}

		// setup on restoring
		if ( packer.IsRestoring() && isInInventory )
		{
			Go* parent = GetParent();
			if ( (parent != NULL) && parent->HasInventory() )
			{
				// minor hack - temporarily remove its parenthood that was set
				// up during creation so the Add() function can re-add it
				ClearParent();

				// now add
				parent->GetInventory()->Add( this, invSlot, AO_REFLEX );

				// optionally equip too
				if ( equipSlot != ES_NONE )
				{
					parent->GetInventory()->Equip( equipSlot, GetGoid(), AO_REFLEX );
				}
			}
		}

	// Do animation.

		nema::Aspect* naspect = aspect->GetAspectPtr();
		gpassert( naspect != NULL );

		bool syncAnim = false;
		if ( packer.IsSaving() )
		{
			// Long winded expression to determine if we need to synch the animation state
			if ( naspect->HasBlender() &&
				 (
				  // It is dead, and animated...
				  (!::IsConscious( lifeState ) &&
				   (aspect->GetLifeState() != LS_IGNORE) &&
				   (naspect->GetBlender()->GetNumSubAnims( CHORE_DIE, naspect->GetCurrentStance()) > 0)
				  )
				  ||
				  // Does it open or close?
				  ((naspect->GetBlender()->GetNumSubAnims( CHORE_OPEN,  AS_PLAIN) > 0) ||
				   (naspect->GetBlender()->GetNumSubAnims( CHORE_CLOSE, AS_PLAIN) > 0)
				  )
				 )
			   )
			{
				syncAnim = true;
			}
		}

		packer.XferBit( syncAnim );

		if ( syncAnim )
		{
			// are we dead?
			if ( !::IsConscious( lifeState ) && (lifeState != LS_IGNORE) )
			{
				// $ dead on ground

				eAnimStance stance = naspect->GetCurrentStance();
				int subAnim = naspect->GetCurrentSubAnim();

				packer.XferRaw( stance );
				packer.XferCount( subAnim );

				if ( packer.IsRestoring() )
				{
					naspect->SetNextChore( CHORE_DIE, stance, subAnim, nema::ANIMFLAG_STARTATEND );
				}
			}
			else
			{
				// $ we must be something that we can open or close

				enum eOpenCloseChore
				{
					SET_BEGIN_ENUM( OCC_, 0 ),

					OCC_NONE,
					OCC_OPEN,
					OCC_CLOSE,

					SET_END_ENUM( OCC_, ),
				};

				eOpenCloseChore occ = OCC_NONE;
				if ( packer.IsSaving() )
				{
					eAnimChore nextChore = naspect->GetNextChore();

					if ( (nextChore == CHORE_OPEN) || (nextChore == CHORE_CLOSE) )
					{
						occ = (nextChore == CHORE_OPEN) ? OCC_OPEN : OCC_CLOSE;
					}
				}

				packer.XferRaw( *(DWORD*)&occ, FUBI_MAX_ENUM_BITS( OCC_ ) );

				if ( packer.IsRestoring() )
				{
					if ( occ == OCC_OPEN )
					{
						naspect->SetNextChore( CHORE_OPEN, AS_PLAIN, 0, nema::ANIMFLAG_STARTATEND );
					}
					else if ( occ == OCC_CLOSE )
					{
						naspect->SetNextChore( CHORE_CLOSE, AS_PLAIN, 0, nema::ANIMFLAG_STARTATEND );
					}
				}
			}
		}

	// Do nema trait bits.

		bool hideMesh   = naspect->GetHideMeshFlag();
		bool freezeMesh = naspect->GetFreezeMeshFlag();

		packer.XferBit( hideMesh   );
		packer.XferBit( freezeMesh );

		if ( packer.IsRestoring() )
		{
			naspect->SetHideMeshFlag  ( hideMesh   );
			aspect ->SetFreezeMeshFlag( freezeMesh );
		}
	}

// Sync [actor].

	if ( actor != NULL )
	{
		// actor does its own sync
		actor->XferForSync( packer );

		// max life value
		float maxLife = aspect->GetMaxLife();
		packer.XferFloat( maxLife );
		if ( packer.IsRestoring() )
		{
			// we're the client, and enchantments don't xfer over, so just set
			// both of these to be the same, 'cause that's what shows up in
			// the mouseover.
			aspect->SetNaturalMaxLife( maxLife );
			aspect->SetMaxLife( maxLife );
		}
	}

// Sync [magic].

	if ( magic != NULL )
	{
		if ( magic->IsPotion() )
		{
			float potionAmount = magic->GetPotionAmount();
			packer.XferFloat( potionAmount );
			magic->SetPotionAmount( potionAmount );
		}
	}

// Sync other fun stuff.

	// scidbits
	if ( !IsSpawned() )
	{
		DWORD scidBits = gGoDb.GetScidBits( GetScid() );
		packer.XferCount( scidBits );
		if ( scidBits != 0 )
		{
			gGoDb.SetScidBits( GetScid(), scidBits );
		}
	}

	// effect scripts
	gGoDb.XferFxForSync( packer, this );

	return ( true );
}

bool Go :: CommitLoad( void )
{
	bool success = true;

	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !(*i)->CommitLoad() )
			{
				success = false;
			}
		}
	}

	// validate
	gpassert( AssertValid() );

	return ( success );
}


const_mem_ptr Go :: CreateRpcSyncData( eSyncType syncType, void*& freeThisPtr )
{
	FuBi::BitPacker packer;

	const_mem_ptr mem;
	if ( XferForSync( packer, syncType ) )
	{
		mem = packer.ReleaseSavedBits( freeThisPtr );
	}

	return ( mem );
}

void Go :: RCAutoRpcSyncData( DWORD machineId, eSyncType syncType )
{
	void* freeThisPtr = NULL;
	const_mem_ptr sync = CreateRpcSyncData( syncType, freeThisPtr );
	if ( sync )
	{
		RCRpcSyncData( machineId, sync );
	}

	::free( freeThisPtr );
}

void Go :: RCRpcSyncData( DWORD machineId, const_mem_ptr data )
{
	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCRpcSyncDataPeek( data );
}

void Go :: RCRpcSyncDataPeek( const_mem_ptr data )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCRpcSyncDataPeek );
	RpcSyncData( data );
}

void Go :: RpcSyncData( const_mem_ptr data )
{
	FuBi::BitPacker packer( data );
	XferForSync( packer );
}

#if !GP_RETAIL

void Go :: MakeEditable( void )
{
	if ( !HasEdit() )
	{
		// build the edit component
		GoEdit* edit = new GoEdit( this );
		edit->Init( gContentDb.FindComponentByName( "edit" ) );

		// insert at end of server components, update index
		{
			RECURSE_LOCK_INSTANCE_FOR_WRITE( this, ComponentColl );
			m_ComponentColl.insert( m_ComponentColl.begin() + m_ClientBegin, edit );
			++m_ClientBegin;
		}

		// update cache to point to new component
		ResetCache();
	}
}

#endif // !GP_RETAIL

void Go :: AddChild( Go* child )
{
	gpassert( child != this );
	gpassert( child->AssertValid() );
	gpassert( !HasChild( child ) );
	gpassert( !HasValidGoid() || !IsCloneSourceGo() );

	// must be the same class
#	if GP_DEBUG
	if ( HasValidGoid() && child->HasValidGoid() )
	{
		// special case - local store gos		
		if ( !(HasStore() && GetGoidClass( child->GetGoid() ) == GO_CLASS_LOCAL
						  && GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL ) )
		{
			gpassert( GetGoidClass( GetGoid() ) == GetGoidClass( child->GetGoid() ) );
		}
	}
#	endif // GP_DEBUG

	// auto-release old parent
	if ( child->m_Parent != NULL )
	{
		// we're adding it to ourselves so no reason to bother with all-clients on removal
		bool oldTransfer = child->IsTransferring();
		child->SetTransferring( true );
		child->m_Parent->RemoveChild( child );
		child->SetTransferring( oldTransfer );
	}

	// make sure all clients flag is the same
	if ( !IsTransferring() && !child->IsTransferring() )
	{
		gGoDb.SetAsAllClientsGo( child, IsAllClients() );
	}

	// grab child
	child->m_Parent = this;
	m_ChildColl.push_back( child );

	// now owned by my player too
	child->SetPlayer( m_Player );

	// mess with components
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		// tell child's components to link the parent
		ComponentColl::iterator i, begin = child->m_ComponentColl.begin(), end = child->m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->LinkParent( this );
		}

		// tell my components to link this child
		begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->LinkChild( child );
		}
	}

	// promote to omni status if necessary (but never demote)
	if ( IsOmni() )
	{
		child->SetIsOmni();
	}

	// make sure we're ok now
	gpassert( AssertValid() );
}

void Go :: RemoveChild( Go* child )
{
	// preconditions
	gpassert( AssertValid() );
	gpassert( HasChild( child ) );

	// seek and destroy
	m_ChildColl.erase( stdx::find( m_ChildColl, child ) );

	// unlink
	UnlinkChild( child );

	// make sure all clients flag not set
	if ( !IsTransferring() && !child->IsTransferring() )
	{
		gGoDb.SetAsAllClientsGo( child, false );
	}	

	// postconditions
	gpassert( AssertValid() );
}

void Go :: RemoveAllChildren()
{
	GopColl children;
	children.swap( m_ChildColl );

	// un-set parent from children
	GopColl::iterator i, begin = children.begin(), end = children.end();
	for ( i = begin ; i != end ; ++i )
	{
		UnlinkChild( *i );
	}
}

void Go :: SSetParent( Go* parent )
{
	CHECK_SERVER_ONLY;

	SetParent( parent );		// first for me (so can do all-clients sync)
	RCSetParent( parent );		// then for everybody else
}

void Go :: RCSetParent( Go* parent )
{
	FUBI_RPC_THIS_CALL( RCSetParent, RPC_TO_ALL );

	SetParent( parent );
}

void Go :: SetParent( Go* parent )
{
	if ( m_Parent != parent )
	{
		if ( m_Parent != NULL )
		{
			m_Parent->RemoveChild( this );
		}
		if ( parent != NULL )
		{
			parent->AddChild( this );
		}
	}
}

bool Go :: HasChild( const Go* child ) const
{
	return ( stdx::has_any( m_ChildColl, child ) );
}

bool Go :: HasChild( Goid child ) const
{
	GopColl::const_iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (*i)->GetGoid() == child )
		{
			return ( true );
		}
	}
	return ( false );
}

void Go :: UpdateChildrenPositions( bool forceUpdate )
{
	if ( forceUpdate )
	{
		ClearAllChildrenVisit();

		// do inventory
		GoInventory* inv = QueryInventory();
		if ( inv != NULL )
		{
			inv->UpdateChildrenPositions();
		}
	}

	// always do aspect
	GoAspect* asp = QueryAspect();
	if ( asp != NULL )
	{
		asp->UpdateChildrenPositions();
	}
}

void Go :: RCExplodeGo( float magnitude, const vector_3 &initial_velocity, DWORD seed )
{
	FUBI_RPC_THIS_CALL( RCExplodeGo, RPC_TO_ALL );

	gSim.ExplodeGo( GetGoid(), magnitude, initial_velocity, seed );
}

void Go :: RCPlayCombatSound( Rules::eHitType hitType, Goid Weapon, Goid Victim, bool bFatalBlow )
{
	// optz: send local without pack/unpack necessary if possible
	if ( ::IsSendLocalOnly( RPC_TO_ALL ) )
	{
		gRules.PlayCombatSound( hitType, Weapon, Victim, bFatalBlow );
		return;
	}

	CombatSoundXfer xfer;
	xfer.m_HitType    = hitType;
	xfer.m_Weapon     = Weapon;
	xfer.m_bFatalBlow = bFatalBlow;

	FuBi::BitPacker packer;
	xfer.Xfer( packer );

	RCPlayCombatSoundPacker( packer.GetSavedBits() );
}

void Go :: RCPlayCombatSoundPacker( const_mem_ptr packola )
{
	FUBI_RPC_THIS_CALL( RCPlayCombatSoundPacker, RPC_TO_ALL );

	CombatSoundXfer xfer;

	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer );

	gRules.PlayCombatSound( xfer.m_HitType, xfer.m_Weapon, GetGoid(), xfer.m_bFatalBlow );
}

void Go :: RCSetCollision( Goid contact_object, bool bGlance )
{
	FUBI_RPC_THIS_CALL( RCSetCollision, RPC_TO_ALL );

	gSim.SetCollision( GetGoid(), contact_object, bGlance );
}

void Go :: RCRemoveAllEnchantmentsOnSelf( void )
{
	FUBI_RPC_TAG();

	// don't tell clients if nothing there
	if ( !gGoDb.HasEnchantments( GetGoid() ) )
	{
		return;
	}

	FUBI_RPC_THIS_CALL( RCRemoveAllEnchantmentsOnSelf, RPC_TO_ALL );

	gGoDb.RemoveEnchantments( GetGoid(), GOID_INVALID );
}

void Go :: RCSend( WorldMessage& message, MESSAGE_DISPATCH_FLAG flags )
{
	// do "to others" because SSend() does it local
	FUBI_RPC_THIS_CALL( RCSend, RPC_TO_OTHERS );

	if ( flags & MD_DELAYED )
	{
		gMessageDispatch.SendDelayed( message, flags );
	}
	else
	{
		gMessageDispatch.Send( message, flags );
	}
}

void Go :: RCDeactivateTrigger( WORD trignum )
{
	FUBI_RPC_THIS_CALL( RCDeactivateTrigger, RPC_TO_ALL );

	gTriggerSys.DeactivateTrigger( GetScid(), GetGoid(), trignum );
}

void Go :: SForcedExpiredTransfer( Go* src )
{
	CHECK_SERVER_ONLY;

	RCForcedExpiredTransfer( src );
}

FuBiCookie Go :: RCForcedExpiredTransfer( Go* src )
{
	FUBI_RPC_THIS_CALL_RETRY( RCForcedExpiredTransfer, RPC_TO_ALL );

	GPSTATS_SYSTEM( SP_LOAD_GO );

	// xfer children
	GopColl children = src->GetChildren();
	GopColl::iterator i, ibegin = children.begin(), iend = children.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Go* child = *i;

		// transfer equipped items
		if ( child->IsInsideInventory() && HasInventory() )
		{
			eEquipSlot equip = src->GetInventory()->GetEquippedSlot( child );
			eInventoryLocation loc = src->GetInventory()->GetLocation( child );

			if ( equip != ES_NONE )
			{
				child->ClearParent();
				GetInventory()->Equip( equip, child->GetGoid(), AO_REFLEX );
			}
			else
			{
				gpassert( loc != IL_INVALID );
				src->GetInventory()->Transfer( child, this, loc, AO_REFLEX );
			}
		}
		else
		{
			child->SetParent( this );
		}
	}

	// xfer scid
	gGoDb.SwapScids( src, this );

	// try to match poses if it is not currently animating. we
	// need to worry about it animating because if the go exits
	// the world in the middle of its die chore then the new
	// aspect will match the pose of the half-dying aspect,
	// which looks silly.
	if ( HasAspect() && src->HasAspect() && src->GetAspect()->IsNemaRoot() )
	{
		nema::Aspect* oldAspect = src->GetAspect()->GetAspectPtr();
		nema::Aspect* newAspect = GetAspect()->GetAspectPtr();

		if ( (newAspect->m_shared == oldAspect->m_shared) && oldAspect->HasBlender() )
		{
			// finish whatever was going on
			oldAspect->UpdateAnimationStateMachine( FLOAT_INFINITE );

			// copy pose
			newAspect->MatchPurgeStatus(oldAspect,IsInAnyScreenWorldFrustum());
			newAspect->CopyBonesPRS( oldAspect->GetHandle() );
		}
	}
}

const gpstring& Go :: GetMessage( const char* messageName )
{
	static gpstring s_String;
	s_String.clear();

	FastFuelHandle messages = GetDataTemplate()->GetFuelHandle().GetChildNamed( "messages" );
	if ( messages )
	{
		FastFuelHandle message = messages.GetChildNamed( messageName );
		if ( message )
		{
			message.Get( "screen_text", s_String );
		}
	}

	return ( s_String );
}

const FuBi::Record* Go :: GetComponentRecord( const char* componentName, const char* withPropertyName ) const
{
	// $ ccast ok, we aren't changing anything

	return ( ccast <ThisType*> ( this )->GetComponentRecord( componentName, withPropertyName ) );
}

FuBi::Record* Go :: GetComponentRecord( const char* componentName, const char* withPropertyName )
{
	FuBi::Record* record = NULL;

	if ( (componentName != NULL) && (*componentName != '\0') )
	{
		GoComponent* component = QueryComponent( componentName );
		if ( (component != NULL) && component->GetData()->IsSkrit() )
		{
			FuBi::Record* localRecord = scast <GoSkritComponent*> ( component )->GetSkritRecord();
			if ( (localRecord != NULL) && (withPropertyName != NULL) && (*withPropertyName != '\0') )
			{
				if ( localRecord->HasColumn( withPropertyName ) )
				{
					record = localRecord;
				}
			}
			else
			{
				record = localRecord;
			}
		}
	}
	else if ( (withPropertyName != NULL) && (*withPropertyName != '\0') )
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( (*i)->GetData()->IsSkrit() )
			{
				FuBi::Record* localRecord = scast <GoSkritComponent*> ( *i )->GetSkritRecord();
				if ( (localRecord != NULL) && localRecord->HasColumn( withPropertyName ) )
				{
					record = localRecord;
					break;
				}
			}
		}
	}

	return ( record );
}

template <typename T>
T GetComponentProperty( Go* go, const char* componentName, const char* propertyName, const T& defValue )
{
	gpassert( propertyName != NULL );

	T value = defValue;

	const FuBi::Record* record = go->GetComponentRecord( componentName, propertyName );
	if ( record != NULL )
	{
		record->Get( propertyName, value );
	}
	else if ( (componentName != NULL) && (*componentName != '\0') )
	{
		gperrorf(( "Component '%s' record or property '%s' unavailable - not a Skrit? Bad spelling?\n", componentName, propertyName ));
	}
	else
	{
		gperrorf(( "Property '%s' unavailable in any Skrit component of this Go\n", propertyName ));
	}

	return ( value );
}

int Go :: GetComponentInt( const char* componentName, const char* propertyName, int defInt )
{
	return ( GetComponentProperty( this, componentName, propertyName, defInt ) );
}

bool Go :: GetComponentBool( const char* componentName, const char* propertyName, bool defBool )
{
	return ( GetComponentProperty( this, componentName, propertyName, defBool ) );
}

float Go :: GetComponentFloat( const char* componentName, const char* propertyName, float defFloat )
{
	return ( GetComponentProperty( this, componentName, propertyName, defFloat ) );
}

const gpstring& Go :: GetComponentString( const char* componentName, const char* propertyName, const gpstring& defString )
{
	static gpstring s_String;
	s_String = GetComponentProperty( this, componentName, propertyName, defString );
	return ( s_String );
}

Scid Go :: GetComponentScid( const char* componentName, const char* propertyName, Scid defScid )
{
	return ( GetComponentProperty( this, componentName, propertyName, defScid ) );
}

template <typename T>
bool SetComponentProperty( Go* go, const char* componentName, const char* propertyName, T value )
{
	gpassert( propertyName != NULL );

	bool success = false;

	FuBi::Record* record = go->GetComponentRecord( componentName, propertyName );
	if ( record != NULL )
	{
		success = record->Set( propertyName, value );
	}
	else if ( (componentName != NULL) && (*componentName != '\0') )
	{
		gperrorf(( "Component '%s' record or property '%s' unavailable - not a Skrit? Bad spelling?\n", componentName, propertyName ));
	}
	else
	{
		gperrorf(( "Property '%s' unavailable in any Skrit component of this Go\n", propertyName ));
	}

	return ( success );
}

bool Go :: SetComponentInt( const char* componentName, const char* propertyName, int valInt )
{
	return ( SetComponentProperty( this, componentName, propertyName, valInt ) );
}

bool Go :: SetComponentBool( const char* componentName, const char* propertyName, bool valBool )
{
	return ( SetComponentProperty( this, componentName, propertyName, valBool ) );
}

bool Go :: SetComponentFloat( const char* componentName, const char* propertyName, float valFloat )
{
	return ( SetComponentProperty( this, componentName, propertyName, valFloat ) );
}

bool Go :: SetComponentString( const char* componentName, const char* propertyName, const gpstring& valString )
{
	return ( SetComponentProperty( this, componentName, propertyName, valString ) );
}

bool Go :: SetComponentScid( const char* componentName, const char* propertyName, Scid valScid )
{
	return ( SetComponentProperty( this, componentName, propertyName, valScid ) );
}

GoComponent* Go :: QueryComponent( const char* name, bool mustExist )
{
	gpassert( !IsMemoryBadFood( (DWORD)this ) );

	RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (*i)->GetName().same_no_case( name ) )
		{
			return ( *i );
		}
	}
	gpassertf( !mustExist, ( "QueryComponent( '%s' ) failed", name ) );
	return ( NULL );
}

GoComponent* Go :: QueryComponent( const gpstring& name, bool mustExist )
{
	gpassert( !IsMemoryBadFood( (DWORD)this ) );

	RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (*i)->GetName().same_no_case( name ) )
		{
			return ( *i );
		}
	}
	gpassertf( !mustExist, ( "QueryComponent( '%s' ) failed", name.c_str() ) );
	return ( NULL );
}

const Go::AutoCache& Go :: GetAutoCache( void )
{
	static AutoCache s_AutoCache;
	return ( s_AutoCache );
}

int Go :: FindCacheIndex( const char* name )
{
	const AutoCache& cache = GetAutoCache();

	AutoCache::const_iterator found = cache.linear_find( name, istring_equal() );
	return ( (found != cache.end()) ? (found - cache.begin()) : -1 );
}

bool Go :: AddNewComponent( const gpstring& type )
{
	// $ early bailout if we already have it
	if ( HasComponent( type ) )
	{
		return ( true );
	}

	// create the new component
	bool serverComponent = false;
	GoComponent* newGoc = gContentDb.CreateRawGoComponent( this, type, serverComponent );
	if ( newGoc != NULL )
	{
		// init it
		newGoc->Init( gContentDb.FindComponentByName( type ) );

		// insert it
		AddNewComponent( newGoc, serverComponent );

		// update in case we added a cached component - CommitCreation() may
		// assume (maybe indirectly) that this is set up correctly
		ResetCache();

		// commit it
		if ( !newGoc->CommitCreation() )
		{
			RECURSE_LOCK_INSTANCE_FOR_WRITE( this, ComponentColl );

			// damn, failed - remove from collection and nuke
			m_ComponentColl.erase( stdx::find( m_ComponentColl, newGoc ) );
			if ( serverComponent )
			{
				--m_ClientBegin;
			}
			Delete( newGoc );
			ResetCache();
		}
	}

	// success only if we didn't have to nuke it
	return ( newGoc != NULL );
}

void Go :: AddNewComponent( GoComponent* newGoc, bool serverComponent )
{
	RECURSE_LOCK_INSTANCE_FOR_WRITE( this, ComponentColl );

	// insert it
	if ( serverComponent )
	{
		// insert at end of server components, update index
		m_ComponentColl.insert( m_ComponentColl.begin() + m_ClientBegin, newGoc );
		++m_ClientBegin;
	}
	else
	{
		// just stick at the end
		m_ComponentColl.push_back( newGoc );
	}
}

#if GP_DEBUG

bool Go :: AssertValid() const
{
	gpassert( this != NULL );
	gpassert( !m_DbgInGoDb || (GoHandle( m_Goid ).Get() == this) );
	gpassert( IsValid( m_Scid, false ) );
	gpassert( m_DataTemplate != NULL );
	gpassert( (m_Parent == NULL) || m_Parent->HasChild( this ) );
	gpassert( (m_NextUpdate == NULL) || (m_NextUpdate->m_PrevUpdate == this) );
	gpassert( (m_PrevUpdate == NULL) || (m_PrevUpdate->m_NextUpdate == this) );
	gpassert( (m_NextUpdate == NULL) == (m_PrevUpdate == NULL) );
	gpassert( !IsUpdating() || (IsUpdating() == m_WantsUpdates) );
	gpassert( m_Player != NULL );
	gpassert( (m_Parent == NULL) || (m_Parent->m_Player == m_Player) );
	gpassert( (m_ClientBegin >= 0) && ((m_ClientBegin == 0) || (m_ClientBegin < scast <int> ( m_ComponentColl.size() ))) );
	gpassert( !m_IsOmni || !IsInAnyWorldFrustum() ); 
	gpassert( (m_NextUpdate != NULL) || !m_PendingUpdateRemoval );
	gpassert( !m_IsInsideInventory || IsInsideInventory( NULL ) );
	gpassert( !m_IsServerOnly || !m_IsAllClients );
	gpassert( !m_IsServerOnly || ::IsServerLocal() );
	gpassert( !HasActor() || ( HasActor() && HasInventory()) );

	// clone sources may not have children
	if ( HasValidGoid() && (GetGoidClass( GetGoid() ) == GO_CLASS_CLONE_SRC)  )
	{
		gpassert( m_ChildColl.empty() );
	}

	{
		GopColl::const_iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( *i != NULL )
			{
				gpassert( (*i)->m_Parent == this );
				gpassert( (*i)->GetPlayer() == GetPlayer() );
				gpassert( (*i)->AssertValid() );
				gpassert( IsTransferring() || (*i)->IsTransferring() || ((*i)->IsAllClients() == IsAllClients()) );

				// must be the same class
				if ( HasValidGoid() && (*i)->HasValidGoid() )
				{
					// special case - global stores that have local gos
					if ( !(HasStore() && GetGoidClass( (*i)->GetGoid() ) == GO_CLASS_LOCAL 
									  && GetGoidClass( GetGoid() ) == GO_CLASS_GLOBAL ) )
					{
						gpassert( GetGoidClass( GetGoid() ) == GetGoidClass( (*i)->GetGoid() ) );
					}
				}

				// parties can only parent actors
				if( HasParty() )
				{
					gpassert( (*i)->HasActor() );
				}
			}
		}
	}

	{
		ConstCacheIter i, begin = GetCacheBegin(), end = GetCacheEnd();
		for ( i = begin ; i != end ; ++i )
		{
			gpassert( *i == QueryComponent( GetAutoCache()[ i - begin ] ) );
		}
	}

	if ( m_DbgInGoDb )
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::const_iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			gpassert( (*i)->AssertValid() );
		}
	}

	return ( true );
}

#endif // GP_DEBUG

void Go :: Init( bool resetPlayer )
{
	m_Goid          = GOID_INVALID;
	m_Scid          = SCID_INVALID;
	m_DataTemplate  = NULL;
	m_RandomSeed    = gGoDb.GetMostRecentFixedSeed();
	m_MpPlayerCount = Player::GetHumanPlayerCount();
	m_CloneSource   = GOID_INVALID;
	m_RegionSource  = REGIONID_INVALID;

#	if !GP_RETAIL
	m_DevInstanceFuel    = FastFuelHandle();
	m_DevWarningCount    = 0;
	m_DevErrorCount      = 0;
	m_DevErrorCountScope = 0;
#	endif

	m_FrustumMembership   = 0;
	m_Parent              = NULL;
	m_NextUpdate          = NULL;
	m_PrevUpdate          = NULL;
	m_ClientBegin         = 0;
	m_LastSimViewOccupant = (unsigned int)-1;

	gpassert( m_ComponentColl.empty() );
	gpassert( m_ChildColl.empty() );

#	if !GP_RETAIL
	m_CreateTime        = ::GetGlobalSeconds();
	m_RefCount          = 0;
	m_PlacementChanging = 0;
	m_RpcCreateBytes    = 0;
	m_RpcFrameBytes     = 0;
	m_RpcTotalBytes     = 0;
#	endif

	m_IsOmni               = false;
	m_IsAllClients         = false;
	m_WantsUpdates         = false;
	m_WantsDraws           = false;
	m_IsCommand            = false;
	m_IsCommandCached      = false;
	m_IsPContentInventory  = false;
	m_IsPContentGenerated  = false;
	m_IsLodfi              = false;
	m_IsServerOnly         = false;
	m_EnableAutoExpiration = true;
	m_HasFrustum           = false;
	m_IsFocused            = false;
	m_IsSelected           = false;
	m_IsMouseShadowed      = false;
	m_IsInHotGroup         = false;
	m_ModifiersDirty       = true;
	m_PendingUpdateRemoval = false;
	m_MarkedForDeletion    = false;
	m_DelayedMpDeletion    = false;
	m_IsExpiring           = false;
	m_IsCommittingCreation = false;
	m_IsPendingEntry       = true;
	m_IsSpaceDirty         = true;
	m_IsWobbDirty          = true;
	m_IsRenderDirty        = true;
	m_IsBucketDirty        = false;
	m_IsFading             = false;
	m_IsFadingFirstView    = false;
	m_IsInsideInventory    = false;
	m_IsEnchanted          = false;
	m_IsPreparedAmmo       = false;
	m_NoStartupFx          = false;

	// clear these out
	m_Visited      = false;
	m_Transferring = false;

#	if GP_DEBUG
	m_DbgInGoDb = false;
#	endif // GP_DEBUG

	if ( resetPlayer )
	{
		m_Player = NULL;
	}

	::ZeroMemory( GetCacheBegin(), (int)GetCacheEnd() - (int)GetCacheBegin() );
}

void Go :: ResetCache( bool sort )
{
	RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

	// sort the components - server then client
	if ( sort )
	{
		std::sort( m_ComponentColl.begin(), m_ComponentColl.begin() + m_ClientBegin, GocSortByPriority );
		std::sort( m_ComponentColl.begin() + m_ClientBegin, m_ComponentColl.end(), GocSortByPriority );
	}

	// reset the cache
	ComponentColl::iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		int index = (*i)->GetCacheIndex();
		if ( index != -1 )
		{
			GetCacheBegin()[ index ] = *i;
		}
	}
}

bool Go :: PrivateLoad( const char* templateName, FastFuelHandle fuel, Scid newScid, bool shutdown, bool shouldPersist )
{
	// reset
	if ( shutdown )
	{
		Shutdown();
	}

	// set fuel address
#	if !GP_RETAIL
	if ( fuel )
	{
		m_DevInstanceFuel = fuel.GetAddress();
	}
	else
	{
		m_DevInstanceFuel.assignf( "<%s>", templateName );
	}
#	endif // !GP_RETAIL

	// set new scid
	m_Scid = newScid;

	// look up template
	m_DataTemplate = gContentDb.AcquireTemplateByName( templateName );
	if ( m_DataTemplate == NULL )
	{
		if ( fuel )
		{
			gperrorf(( "Game object at '%s' (scid = 0x%08X) instantiates unregistered template '%s'\n",
					   fuel.GetAddress().c_str(), m_Scid, templateName ));
		}
		else if ( *templateName == '#' )
		{
			gperrorf(( "Coding error: attempted to create an automatic clone source '%s' from pcontent\n", templateName ));
		}
		else if ( *templateName == '\0' )
		{
			gperrorf(( "Coding error: attempted to clone a Go from an empty template!\n" ));
		}
		else
		{
			gperrorf(( "Game object (scid = 0x%08X) instantiates unregistered template '%s'\n", m_Scid, templateName ));
		}
		return ( false );
	}

	// optionally force addition of edit component if we're in edit mode
#	if !GP_RETAIL
	{
		if ( gGoDb.IsEditMode() || gWorldOptions.GetTuningLoad() )
		{
			MakeEditable();
		}
	}
#	endif // !GP_RETAIL

	// instantiate template components
	bool success = true;
	{
		GoDataTemplate::ComponentIter i,
									  begin = m_DataTemplate->GetComponentBegin(),
									  end   = m_DataTemplate->GetComponentEnd();
		for ( i = begin ; i != end ; ++i )
		{
			// create component
			bool serverComponent = false;
			GoComponent* newComponent = gContentDb.CreateGoComponent( this, *i, fuel, serverComponent, true, shouldPersist );
			if ( newComponent != NULL )
			{
				// add it to the collection
				AddNewComponent( newComponent, serverComponent );
			}
			else
			{
				gperrorf(( "Failure to construct component '%s'\n", i->GetName().c_str() ));
				GPDEV_ONLY( success = false );
			}
		}
	}
	if ( !success )
	{
		if ( fuel )
		{
			gperrorf(( "Aborting game object construction at '%s'\n", fuel.GetAddress().c_str() ));
		}
		else
		{
			gperrorf(( "Aborting game object construction from template '%s'\n", templateName ));
		}
		return ( false );
	}

	// update cache
	ResetCache();

	// validate
	gpassert( AssertValid() );

	// done
	return ( true );
}

void Go :: SetUpdateRoot( void )
{
	m_NextUpdate = m_PrevUpdate = this;
}

void Go :: EnterWorld( FrustumId membership )
{
	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( AssertValid() );
	gpassert( !IsOmni() );

	if ( IsInAnyWorldFrustum() )
	{
		return;
	}

	gpgolifef(( "Go::EnterWorld() - 0x%08X (%s)\n", GetGoid(), GetTemplateName() ));

	// insert me into the list if we want updates
	if ( m_WantsUpdates )
	{
		PrivateStartUpdates();
	}

	// server-only stuff
	if ( IsServerLocal() )
	{
		// time to stop expiring me if i was auto expiring
		gGoDb.CancelExpiration( this, false );

		// resume any simulations that this go may have had associated with it
		gSim.ResumeSimulation( GetGoid() );
	}

	// tell message dispatcher to resubmit any frozen messages
	gMessageDispatch.OnGoEnterWorld( this );

	// we're entering, tell everybody...
	WorldMessage( WE_ENTERED_WORLD, GOID_INVALID, GetGoid(), MakeInt(membership) ).Send();

	// tell godb to lock any clone sources in memory we're subscribed to
	gGoDb.LockCloneSources( this, true );

	// tell godb to restart any sfx that were started before i left the world or
	// were started during construction before i was in the world. must do this
	// after the WE_ENTERED_WORLD message sent out in case any skrit components
	// trigger on that to start new effects!!
	gGoDb.ResumeEffectScripts( this );

	// clone sources should not get scaled (otherwise spawned instances get scaled twice!)
	if ( ::IsMultiPlayer() && IsServerLocal() && GetPlayer()->IsComputerPlayer() && !IsCloneSourceGo() )
	{
		// make sure this is an actor and the map is inited
		// we dont' need to scale something that is not in the world! (like the staging area)
		if( HasActor() && gWorldMap.IsInitialized() )
		{
			// set the mp scaling using only the players in 
			// the near area for the calcuations
			GetActor()->ScaleForMultiplayer( );
		}
	}
}

void Go :: LeaveWorld( bool deletingGo )
{
	gpassert( deletingGo || !HasFrustum() );

	if ( !IsInAnyWorldFrustum() )
	{
		return;
	}

	CHECK_PRIMARY_THREAD_ONLY;

	gpgolifef(( "Go::LeaveWorld() - 0x%08X (%s)\n", GetGoid(), GetTemplateName() ));

	// we're leaving
	GPDEV_ONLY( SetPlacementChanging() );		// #10335
	WorldMessage( WE_LEFT_WORLD, GOID_INVALID, GetGoid(), MakeInt( GetGoid() ) ).Send();	
	GPDEV_ONLY( ClearPlacementChanging() );		// #10335

	// remove from update list if there
	if ( m_NextUpdate != NULL )
	{
		m_PendingUpdateRemoval = true;
	}

	// tell godb to let go of any clone sources in memory we're subscribed to
	gGoDb.LockCloneSources( this, false );

	// only do this if we are not deleting the go
	if ( !deletingGo )
	{
		// tell godb to stop and save off my sfx
		gGoDb.PauseEffectScripts( this );

		// server-only stuff
		if ( IsServerLocal() )
		{
			// pause any simulations that this go may have associated with it
			gSim.PauseSimulation( GetGoid() );
		}

		// tell message dispatcher to freeze any pending messages for me
		gMessageDispatch.OnGoLeaveWorld( this );
	}
}

void Go :: SetFrustumMembership( FrustumId membership )
{
	// only bother if we've got an aspect, and also check mp/sp status because
	// no point doing this if we're single player.
	if ( HasAspect() )
	{
		// only bother on change
		if ( m_FrustumMembership != membership )
		{
			// get old purge state
			bool oldShouldBeLoaded = IsInAnyScreenWorldFrustum();
			FrustumId oldMembership = m_FrustumMembership;
			m_FrustumMembership = membership;
			bool newShouldBeLoaded = IsInAnyScreenWorldFrustum();
			if ( oldShouldBeLoaded != newShouldBeLoaded )
			{
				nema::Aspect* pAsp = GetAspect()->GetAspectHandle().Get();
				if (pAsp)
				{
					// Be sure to check visibility before bothering.
					if ( (GetAspect()->GetIsVisible() || GetAspect()->GetForceNoRender()) )
					{
						// Skip children, they will be affected by their parent
						if ((!pAsp->GetParent() || !pAsp->GetParent()->IsPurged(1)))
						{
							if ( newShouldBeLoaded )
							{
								pAsp->Reconstitute(0);
								//** Fixes bug #10809 (dead bodies reconsituting upright)
								pAsp->ResetBlender();
								//**
							}
							else
							{
								pAsp->Purge(1);
							}
						}
					}
				}
			}
			WorldMessage( WE_FRUSTUM_MEMBERSHIP_CHANGED, GetGoid(), GetGoid(), MakeInt( oldMembership ) ).Send();
		}
	}
	else
	{
		// just set it, who cares
		m_FrustumMembership = membership;
	}
}

Go :: ~Go( void )
{
	Shutdown( false );
}

Go :: Go( Player* player )
{
	Init();
	m_Player = player;
}

Go :: Go( const Go& source, Player* newPlayer, bool local )
{
	Init();
	Copy( source, newPlayer, local, SCID_SPAWNED, false );
}

void Go :: Copy( const Go& source, Player* newPlayer, bool local, Scid newScid, bool shutdown )
{
	GPDEV_ONLY( AutoGoError autoGoError( this, &m_DevErrorCountScope ); )

	// parameter validation
	gpassert( newScid != SCID_INVALID );

	// out with the old
	if ( shutdown )
	{
		Shutdown();
	}

	// copy simple items
	m_Goid                 = GOID_INVALID;
	m_Scid                 = newScid;
	m_DataTemplate         = source.m_DataTemplate;
	m_MpPlayerCount        = source.m_MpPlayerCount;
	m_RegionSource         = source.m_RegionSource;
	m_Parent               = NULL;
	m_Player               = newPlayer;
	m_ClientBegin          = source.m_ClientBegin;
	m_IsOmni               = source.m_IsOmni;
	m_IsAllClients         = source.m_IsAllClients;
	m_EnableAutoExpiration = source.m_EnableAutoExpiration;

	// match server-only-ness
	SetServerOnly( source.m_IsServerOnly );

	// we have the new template
	gpassert( m_DataTemplate != NULL );
	gContentDb.AddRefTemplate( m_DataTemplate );

	// $ note: don't copy random seed, that's already been set

	// note: only copy the clone source over if the clone source is not a clone
	// source! that doesn't make much sense but the point of the m_CloneSource
	// is to remember the real global goid clone source so it can be passed over
	// the network.
	if ( ::GetGoidClass( source.GetGoid() ) == GO_CLASS_GLOBAL )
	{
		m_CloneSource = source.GetGoid();
	}

	GPDEV_ONLY( m_DevInstanceFuel = source.m_DevInstanceFuel );

	{
		// reserve space for the incoming components
		m_ComponentColl.reserve( source.m_ComponentColl.size() );

		RECURSE_LOCK_INSTANCE_FOR_READ( &source, ComponentColl );
		RECURSE_LOCK_INSTANCE_FOR_WRITE( this, ComponentColl );

		// copy components
		ComponentColl::const_iterator i,
									  ibegin = source.m_ComponentColl.begin(),
									  iend   = source.m_ComponentColl.end  ();
		for ( i = ibegin ; i != iend ; ++i )
		{
			GoComponent* goc = *i;
			if ( !local || !goc->GetData()->IsServerOnly() )
			{
				m_ComponentColl.push_back( goc->Clone( this ) );
			}
			else
			{
				--m_ClientBegin;
			}
		}
	}

	// update cache but no need to sort, we're already cool there
	ResetCache( false );

	// force omni if no placement node
	if ( !HasPlacement() )
	{
		SetIsOmni();
	}

	// commit them
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::const_iterator i = source.m_ComponentColl.begin();
		ComponentColl::iterator j, jbegin = m_ComponentColl.begin(), jend = m_ComponentColl.end();
		for ( j = jbegin ; j != jend ; ++i, ++j )
		{
			(*j)->CommitClone( *i );
		}
	}

	// validate
	gpassert( AssertValid() );
}

bool Go :: LoadFromInstance( FastFuelHandle fuel, Scid scid, bool shutdown, bool shouldPersist )
{
	// detect scid from fuel name
	if ( scid == SCID_INVALID )
	{
		if ( !::FromString( fuel.GetName(), scid ) || !IsInstance( scid ) )
		{
			gperrorf(( "Invalid Scid '%s' at '%s'\n", fuel.GetName(), fuel.GetAddress().c_str() ));
			return ( false );
		}
	}

	return ( PrivateLoad( fuel.GetType(), fuel, scid, shutdown, shouldPersist ) );
}

bool Go :: LoadFromTemplate( const char* templateName, bool shutdown, bool shouldPersist )
{
	return ( PrivateLoad( templateName, FastFuelHandle(), SCID_SPAWNED, shutdown, shouldPersist ) );
}

bool Go :: CommitCreation( Goid goid, int mpPlayerCount, bool preload )
{
	// take the goid
	gpassert( m_Goid == GOID_INVALID );
	m_Goid = goid;

	// take mp player count
	m_MpPlayerCount = mpPlayerCount;

	// local go's may not have server-only components (duh)
#	if !GP_RETAIL
	if ( (GetGoidClass( goid ) == GO_CLASS_LOCAL) && (m_ClientBegin != 0) && !gGoDb.IsEditMode() )
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ReportSys::AutoReport autoReport( &gErrorContext );
		gperrorf(( "Error! I am a LOCAL Go and yet I contain server-only components! "
				   "Somebody needs to adjust my LODFI settings.\n"
				   "\n"
				   "Template: '%s'\n"
				   "\n"
				   "Server-only components: ", GetTemplateName() ));
		ComponentColl::iterator i, ibegin = m_ComponentColl.begin(), iend = ibegin + m_ClientBegin;
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i != ibegin )
			{
				gperror( ", " );
			}
			gperror( (*i)->GetName() );
		}
	}
#	endif // !GP_RETAIL

	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		// reflect to my components
		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !(*i)->CommitCreation() )
			{
				return ( false );
			}
		}

		// preload now too if requested
		if ( preload )
		{
			for ( i = begin ; i != end ; ++i )
			{
				(*i)->Preload();
			}
		}
	}

	// if i'm a clone source, make sure nobody thinks we're waiting world entry
	if ( GetGoidClass( goid ) == GO_CLASS_CLONE_SRC )
	{
		m_IsPendingEntry = false;
	}

	// validate
	gpassert( AssertValid() );

	// let's do a server-only detect
	if ( IsGlobalGo() )
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		SetServerOnly( true );

		ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( !(*i)->CanServerExistOnly() )
			{
				// awww... won't work. ok bail out. (sigh)
				SetServerOnly( false );
				break;
			}
		}
	}

	// track
#	if GP_ENABLE_STATS
	if ( IsGlobalGo() )
	{
		GPSTATS_SAMPLE( AddGoGlobalAlloc() );
	}
	else if ( IsCloneSourceGo() )
	{
		GPSTATS_SAMPLE( AddGoCloneSrcAlloc() );
	}
	else if ( IsLodfi() )
	{
		GPSTATS_SAMPLE( AddGoLodfiAlloc() );
	}
	else
	{
		GPSTATS_SAMPLE( AddGoLocalAlloc() );
	}
#	endif // GP_ENABLE_STATS

	// success!
	return ( true );
}

void Go :: Shutdown( bool reinit )
{
	// do not delete while outstanding handles!
#	if !GP_RETAIL
	static bool s_IsDebugging = SysInfo::IsDebuggerPresent();
	if ( (m_RefCount > 0) && (::GlobalGetUnfilteredHandledExceptionCount() == 0) )
	{
		// $$$ special: we want to catch this thing RIGHT NOW to see what the
		//     other thread is doing that it's got the handle outstanding. so
		//     just force a breakpoint immediately if we're in the debugger.
		//     hopefully that will catch it before this thread loses its time
		//     slice...
		if ( s_IsDebugging )
		{
			BREAKPOINT();
		}
		gperrorf(( "TELL SCOTT: A game object was deleted while GoHandles are still outstanding! RUN!!!\n\n"
				   "\tGoid = 0x%08X (%s)\n"
				   "\tScid = 0x%08X\n",
				   m_Goid, m_DataTemplate ? GetTemplateName() : "<UNKNOWN>", m_Scid ));
	}
#	endif // !GP_RETAIL

	// we're dirty (and get parent too)
	SetBucketDirty();

	// clear spec
	GPDEV_ONLY( m_DevInstanceFuel.clear() );
	gpassert( !IsInAnyWorldFrustum() );

	// clean up and verify references
	if ( m_Goid != GOID_INVALID )
	{
		// make sure we're not in these db's
		gpassert( !gGoDb.IsWatching( m_Goid ) );
		gpassert( !gGoDb.IsBeingWatched( m_Goid ) );
		gpassert( !gGoDb.IsOnDeck( m_Goid ) );

		// cancel any pending expiration
		gGoDb.CancelExpiration( this, true );

		// make sure dispatcher knows it
		gMessageDispatch.OnGoDestroyed( this );
	}

	// un-set child from parent - since we're shutting down no need to do the
	// all-clients management
	m_Transferring = true;
	ClearParent();
	m_Transferring = false;

	// un-set parent from children
	RemoveAllChildren();

	// let go of frustum if any (but no delete)
	if ( HasFrustum() )
	{
		gGoDb.ReleaseFrustumFromGo( GetGoid(), true );
	}

	// free our components
	{
		RECURSE_LOCK_INSTANCE_FOR_WRITE( this, ComponentColl );

		// shutdown
		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->Shutdown();
		}

		// delete and zero out
		for ( i = begin ; i != end ; ++i )
		{
			// get ptr and zero it
			GoComponent* component = *i;
			*i = NULL;

			// find the cache entry and nuke it
			int index = component->GetCacheIndex();
			if ( index != -1 )
			{
				gpassert( GetCacheBegin()[ index ] == component );
				GetCacheBegin()[ index ] = NULL;
			}

			// delete the component
			delete ( component );
		}

		// clear
		m_ComponentColl.clear();
	}

	// let go of frustum if any (but no delete)
	if ( HasFrustum() )
	{
		gGoDb.ReleaseFrustumFromGo( GetGoid(), true );
	}

	// release updates
	if ( m_NextUpdate != NULL )
	{
		PrivateStopUpdates();
	}

	// release our template
	if ( m_DataTemplate != NULL )
	{
		gContentDb.ReleaseTemplate( m_DataTemplate );
		m_DataTemplate = NULL;
	}

	// if i was the hero, clear it
	if ( (m_Player != NULL) && (m_Player->GetHero() == m_Goid) )
	{
		m_Player->SetHero( GOID_INVALID );
	}

	// reset members
	if ( reinit )
	{
		Init( false );
	}
}

void Go :: UnlinkChild( Go* child )
{
	gpassert( child->m_Parent == this );

	// special pre-unlink to commit any caches
	if ( child->HasPlacement() && !IsOmni() )
	{
		child->GetPlacement()->ResetPlacement();
	}

	// reset and erase
	child->m_Parent = NULL;

	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		// tell child's components to unlink the parent
		ComponentColl::iterator i, begin = child->m_ComponentColl.begin(), end = child->m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->UnlinkParent( this );
		}

		// tell my components to unlink this child
		begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->UnlinkChild( child );
		}
	}

	// both are dirty
	SetBucketDirty();
	child->SetBucketDirty();
}

void Go :: SetHasFrustum( bool set )
{
	m_HasFrustum = set;
	if ( m_HasFrustum )
	{
		GetPlacement()->ResetPosition();
	}
}

void Go :: SetIsOmni( bool set )
{
	m_IsOmni = set;
	if ( m_IsOmni )
	{
		m_IsPendingEntry = false;
		
		if ( HasPlacement() )
		{
			SiegePos oldPos = GetPlacement()->GetPosition();
			if ( oldPos.node.IsValid() && gGoDb.IsNodeOccupant( this, oldPos.node ) )
			{
				gGoDb.RemoveNodeOccupant( this, oldPos.node );
			}		
		}
	}
	else
	{
		// release updates - will resume updates once the object is transferred to world.
		if ( m_NextUpdate != NULL )
		{
			PrivateStopUpdates();
		}
	}
}

void Go :: PrivateCheckUpdates( void )
{
	// only do something if we're not already doing what we want
	if ( (m_WantsUpdates != IsUpdating()) && !m_PendingUpdateRemoval && IsInAnyWorldFrustum() )
	{
		if ( m_WantsUpdates )
		{
			PrivateStartUpdates();
		}
		else
		{
			m_PendingUpdateRemoval = true;
		}
	}
}

void Go :: PrivateStartUpdates( void )
{
	CHECK_PRIMARY_THREAD_ONLY;

	// $ early bailout if we were asked to reenter the world and the godb didn't update yet
	if ( m_PendingUpdateRemoval )
	{
		m_PendingUpdateRemoval = false;
		gpassert( m_NextUpdate != NULL );
		gpassert( m_PrevUpdate != NULL );
		return;
	}

	Go* root = gGoDb.GetUpdateRoot();

	gpassert( root != NULL );
	gpassert( m_NextUpdate == NULL );
	gpassert( m_PrevUpdate == NULL );

	m_NextUpdate = root->m_NextUpdate;
	m_PrevUpdate = root;
	m_NextUpdate->m_PrevUpdate = this;
	m_PrevUpdate->m_NextUpdate = this;
}

Go* Go :: PrivateStopUpdates( void )
{
	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( m_NextUpdate != NULL );
	gpassert( m_PrevUpdate != NULL );

	Go* old = m_NextUpdate;

	m_NextUpdate->m_PrevUpdate = m_PrevUpdate;
	m_PrevUpdate->m_NextUpdate = m_NextUpdate;
	m_NextUpdate = NULL;
	m_PrevUpdate = NULL;

	m_PendingUpdateRemoval = false;

	return ( old );
}

void Go :: PrivateRecalcModifiers( void )
{

// Apply algorithm for natural values.

	// actor is special - modifies the aspect
	GoActor* actor = QueryActor();
	if ( actor != NULL )
	{
		actor->ResetModifiers();
	}

	// have the godb apply any actor-specific enchantments that might be running
	// on us (strength mods etc.)
	if ( m_IsEnchanted && !gGoDb.ApplyEnchantments( this, true ) )
	{
		m_IsEnchanted = false;
	}

	// now calc max mana/life etc
	if ( actor != NULL )
	{
		actor->RecalcModifiers();
	}

// Setup.

	// first reset our modifiers from natural values
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( *i != actor )
			{
				(*i)->ResetModifiers();
			}
		}
	}

	// now check all child dirty bits of the children who are in the world
	{
		GopColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			(*i)->CheckModifierRecalc();
		}
	}

// Apply algorithm for modified values.

	// have the godb apply any non-actor-specific enchantments that might be
	// running on us (strength mods etc.)
	if ( m_IsEnchanted && !gGoDb.ApplyEnchantments( this, false ) )
	{
		m_IsEnchanted = false;
	}

	// inventory goes first
	GoInventory* inventory = QueryInventory();
	if ( inventory != NULL )
	{
		inventory->RecalcModifiers();
	}

	// do the rest
	{
		RECURSE_LOCK_INSTANCE_FOR_READ( this, ComponentColl );

		ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( (*i != inventory) && (*i != actor) )
			{
				(*i)->RecalcModifiers();
			}
		}
	}

// Finish.

	// take the newly modified value and test if equipment can still be equipped
	if ( HasActor() && GetActor()->GetEnchantmentRemoved() )
	{
		gWorld.TestEquipCallback( GetGoid() );
		GetActor()->SetEnchantmentRemoved( false );
	}

	// done
	m_ModifiersDirty = false;
}

DWORD Go :: GoToNet( Go* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

Go* Go :: NetToGo( DWORD d, FuBiCookie* cookie )
{
	// get goid
	Goid goid = MakeGoid( d );

	// if it's supposed to be null, leave it
	if ( goid == GOID_INVALID )
	{
		return ( NULL );
	}

	// attempt to resolve
	GoHandle go( goid );

	// success?
	if ( go )
	{
		return ( go );
	}

	// if this go was recently deleted, don't bother retrying
	if ( ::IsMultiPlayer() && gGoDb.WasGoidRecentlyDeleted( goid ) )
	{
		*cookie = RPC_SUCCESS_IGNORE;
	}
	else
	{
		*cookie = RPC_FAILURE_IGNORE;
	}

	// no go
	return ( NULL );
}

//////////////////////////////////////////////////////////////////////////////
// class GoHandle implementation

GoHandle& GoHandle :: operator = ( Goid goid )
{
	return ( operator = ( gGoDb.DerefGo( goid ) ) );
}

//////////////////////////////////////////////////////////////////////////////
