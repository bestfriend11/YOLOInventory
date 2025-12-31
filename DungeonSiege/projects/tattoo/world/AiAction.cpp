#include "precomp_world.h"

#include "aiaction.h"
#include "aiquery.h"
#include "FuBiBitPacker.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoActor.h"
#include "GoMind.h"
#include "Go.h"
#include "GoAspect.h"
#include "MessageDispatch.h"
#include "SkritObject.h"
#include "GuiHelper.h"
#include "ui_shell.h"


///////////////////////////////////////////////////////////////////////
//	JobReq


JobReq::JobReq( void )
{
	Init();
}


void JobReq::Init()
{
	ZeroObject( *this );

	m_Jat             = JAT_INVALID;
	m_Q               = JQ_ACTION;
	m_GoalObject      = GOID_INVALID;
	m_GoalModifier    = GOID_INVALID;
	m_GoalPosition    = SiegePos::INVALID;
	m_GoalOrientation = Quat::INVALID;
	m_QPlace          = QP_INVALID;
	m_Origin		  = AO_INVALID;
	m_Slot            = ES_NONE;
	m_Notify          = GOID_INVALID;
}


void JobReq::Xfer( FuBi::BitPacker& packer )
{
	static JobReq s_Defaults;

	// xfer always - never can be invalid
	packer.XferRaw( m_Jat, FUBI_MAX_ENUM_BITS( JAT_ ) );

	packer.XferIf   ( m_Q,               s_Defaults.m_Q,          FUBI_MAX_ENUM_BITS( JQ_ ) );
	packer.XferIf   ( m_QPlace,          s_Defaults.m_QPlace,     FUBI_MAX_ENUM_BITS( QP_ ) );
	packer.XferIf   ( m_Origin,          s_Defaults.m_Origin,     FUBI_MAX_ENUM_BITS( AO_ ) );
	packer.XferCount( m_PlaceBefore,     s_Defaults.m_PlaceBefore );
	packer.XferCount( m_PlaceAfter,      s_Defaults.m_PlaceAfter  );
	packer.XferIf   ( m_Notify,          s_Defaults.m_Notify      );

	packer.XferIf( m_GoalObject,      s_Defaults.m_GoalObject      );
	packer.XferIf( m_GoalModifier,    s_Defaults.m_GoalModifier    );
	packer.XferIf( m_GoalPosition,    s_Defaults.m_GoalPosition    );
	packer.XferIf( m_GoalOrientation, s_Defaults.m_GoalOrientation );
	packer.XferIf( m_Slot,            s_Defaults.m_Slot,           FUBI_MAX_ENUM_BITS( ES_ ) );

	packer.XferIf   ( m_Int1,          s_Defaults.m_Int1          );
	packer.XferIf   ( m_Int2,          s_Defaults.m_Int2          );
	packer.XferIf   ( m_SECommandScid, s_Defaults.m_SECommandScid );
	packer.XferFloat( m_Float1 );
}


JobReq::JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier, SiegePos const & pos )
{
	Init();

	m_Jat          = type;
	m_Q            = q;
	m_GoalObject   = goal;
	m_GoalModifier = modifier;
	m_GoalPosition = pos;
	m_QPlace       = place;
	m_Origin	   = origin;
}


JobReq::JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier )
{
	Init();

	m_Jat          = type;
	m_Q            = q;
	m_GoalObject   = goal;
	m_GoalModifier = modifier;
	m_QPlace       = place;
	m_Origin       = origin;
}


JobReq::JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, SiegePos const & pos )
{
	Init();

	m_Jat          = type;
	m_Q            = q;
	m_GoalPosition = pos;
	m_QPlace       = place;
	m_Origin       = origin;
}


JobReq::JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal )
{
	Init();

	m_Jat          = type;
	m_Q            = q;
	m_GoalObject   = goal;
	m_QPlace       = place;
	m_Origin       = origin;
}


JobReq::JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin )
{
	Init();

	m_Jat		   = type;
	m_Q            = q;
	m_QPlace       = place;
	m_Origin       = origin;
}


#if !GP_RETAIL
void JobReq::RpcToString( gpstring& appendHere ) const
{
	JobReq def;

	appendHere.appendf(
			"jat = %s, queue = %s, placement = %s, actionOrigin = %s, "
			"placeBefore = %d, placeAfter = %d, notify = 0x%08X",
			::ToString( m_Jat ), ::ToString( m_Q ), ::ToString( m_QPlace ), ::ToString( m_Origin ),
			m_PlaceBefore, m_PlaceAfter, m_Notify );

	if ( GoDb::DoesSingletonExist() && (m_Notify != GOID_INVALID) )
	{
		GoHandle go( m_Notify );
		appendHere.appendf( " (%s)", go ? go->GetTemplateName() : "<invalid>" );
	}

	appendHere.appendf( ", goalObject = 0x%08X", m_GoalObject );
	if ( GoDb::DoesSingletonExist() && (m_GoalObject != GOID_INVALID) )
	{
		GoHandle go( m_GoalObject );
		appendHere.appendf( " (%s)", go ? go->GetTemplateName() : "<invalid>" );
	}

	appendHere.appendf( ", goalModifier = 0x%08X", m_GoalModifier );
	if ( GoDb::DoesSingletonExist() && (m_GoalModifier != GOID_INVALID) )
	{
		GoHandle go( m_GoalModifier );
		appendHere.appendf( " (%s)", go ? go->GetTemplateName() : "<invalid>" );
	}

	if ( m_GoalPosition != def.m_GoalPosition )
	{
		appendHere.appendf(
				", goalPosition = %g,%g,%g,%s", 
				m_GoalPosition.pos.x, m_GoalPosition.pos.y,
				m_GoalPosition.pos.z, m_GoalPosition.node.ToString().c_str() );
	}

	if ( m_GoalOrientation != def.m_GoalOrientation )
	{
		appendHere.appendf(
				", goalOrientation = %g,%g,%g,%g",
				m_GoalOrientation.m_x, m_GoalOrientation.m_y,
				m_GoalOrientation.m_z, m_GoalOrientation.m_w );
	}

	appendHere.appendf(
			", slot = %s, int1 = %d, int2 = %d, commandScid = 0x%08X",
			::ToString( m_Slot ), m_Int1, m_Int2, m_SECommandScid );

	if ( GoDb::DoesSingletonExist() )
	{
		Goid seGoid = gGoDb.FindGoidByScid( m_SECommandScid );
		if ( seGoid != GOID_INVALID )
		{
			GoHandle go( seGoid );
			appendHere.appendf( " (%s)", go ? go->GetTemplateName() : "<invalid>" );
		}
	}

	appendHere.appendf( ", float1 = %g", m_Float1 );
}
#endif // !GP_RETAIL


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier, SiegePos const & pos )
{
	static JobReq s_JobReq;
	s_JobReq = JobReq( type, q, place, origin, goal, modifier, pos );
	return s_JobReq;
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, SiegePos const & pos )
{
	return MakeJobReq( type, q, place, origin, goal, GOID_INVALID, pos );
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier )
{
	return( MakeJobReq( type, q, place, origin, goal, modifier, SiegePos::INVALID ) );
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, SiegePos const & pos )
{
	return( MakeJobReq( type, q, place, origin, GOID_INVALID, GOID_INVALID, pos ) );
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	SiegePos const & pos, Goid modifier )
{
	JobReq & req = MakeJobReq( type, q, place, origin, GOID_INVALID, GOID_INVALID, pos );
	req.m_GoalModifier = modifier;
	return req;
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, eEquipSlot slot )
{
	JobReq & req = MakeJobReq( type, q, place, origin, goal, GOID_INVALID, SiegePos::INVALID );
	req.m_Slot = slot;
	return req;
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, eEquipSlot slot )
{
	JobReq & req = MakeJobReq( type, q, place, origin, GOID_INVALID, GOID_INVALID, SiegePos::INVALID );
	req.m_Slot = slot;
	return req;
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal )
{
	return( MakeJobReq( type, q, place, origin , goal, GOID_INVALID, SiegePos::INVALID ) );
}


JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin )
{
	return( MakeJobReq( type, q, place, origin , GOID_INVALID, GOID_INVALID, SiegePos::INVALID ) );
}


void JobReqClientsXfer :: Xfer( FuBi::BitPacker& packer )
{
	m_JobReq.Xfer( packer );

	int size = m_Clients.size();
	packer.XferCount( size );
	m_Clients.resize( size );
	packer.XferBytes( &*m_Clients.begin(), m_Clients.size_bytes() );
}


#if !GP_RETAIL

void JobReqClientsXfer :: RpcToString( gpstring& appendHere ) const
{
	m_JobReq.RpcToString( appendHere );

	if ( !m_Clients.empty() )
	{
		appendHere += ", clients = ";

		GoidColl::const_iterator i, ibegin = m_Clients.begin(), iend = m_Clients.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( i != ibegin )
			{
				appendHere += ",";
			}
			appendHere.appendf( "0x%08X", *i );
		}
	}
}

#endif // !GP_RETAIL


////////////////////////////////////////
//	AIAction


AIAction::AIAction()
{
	gMessageDispatch.RegisterBroadcastCallback( "aiaction", makeFunctor( *this, &AIAction::HandleBroadcast ) );
}


AIAction::~AIAction()
{
	Shutdown();

	if( MessageDispatch::DoesSingletonExist() )
	{
		gMessageDispatch.UnregisterBroadcastCallback( "aiaction" );
	}
}


void AIAction::Shutdown()
{
	for( JobCloneSourceMap::iterator i = m_JobCloneSourceMap.begin(); i != m_JobCloneSourceMap.end(); ++i )
	{
		delete (*i).second;
	}
	m_JobCloneSourceMap.clear();
}


void AIAction::HandleBroadcast( WorldMessage & msg )
{
	if( msg.GetEvent() == WE_POST_RESTORE_GAME )
	{
		for( JobCloneSourceMap::iterator i = m_JobCloneSourceMap.begin(); i != m_JobCloneSourceMap.end(); ++i )
		{
			(*i).second->m_hSkrit->CommitCreation();
		}
	}
}


////////////////////////////////////////
//	group command assignment

void AIAction :: RSDoJob( JobReq const & req, const GoidColl& Clients )
{
	if( !Clients.empty() )
	{
		JobReqClientsXfer xfer;
		xfer.m_JobReq = req;
		xfer.m_Clients = Clients;

		FuBi::BitPacker packer;
		xfer.Xfer( packer );
		RSDoJobPacker( packer.GetSavedBits() );
	}
}


FuBiCookie AIAction :: RSDoJobPacker( const_mem_ptr packola )
{
	FUBI_RPC_CALL_RETRY( RSDoJobPacker, RPC_TO_SERVER );

	JobReqClientsXfer xfer;
	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer );

	JobReq& req = xfer.m_JobReq;
	GoidColl& gos = xfer.m_Clients;

	if( req.m_Jat == JAT_MOVE || req.m_Jat == JAT_PATROL )
	{
		JobReq tempReq( req );

		SiegePos Pos = req.m_GoalPosition;
		GONodePosList PosList;
		GopColl ClientsColl;
		gos.Translate( ClientsColl );
		gAIQuery.GetPositionsAroundNewCenter( ClientsColl, Pos, PosList );

		GONodePosList::iterator i;

		for( i = PosList.begin(); i != PosList.end(); ++i )
		{
			Go * pGO = (*i).first;
			SiegePos NewPosition = (*i).second;
			tempReq.m_GoalPosition = Pos;
			pGO->GetMind()->RSDoJob( tempReq );
		}
	}
	else
	{
		GoidColl::iterator i;

		for( i = gos.begin(); i != gos.end(); ++i )
		{
			GoHandle go( *i );

			if( go.IsValid() )
			{
				go->GetMind()->RSDoJob( req );
			}
		}
	}
	return( RPC_SUCCESS );
}



////////////////////////////////////////
//	types and data


////////////////////////////////////////
//	
struct lootInfo
{
public:

	lootInfo( Go * go )
	{
		m_Loot		= go;
		m_Claimed	= false;
	}

	Go * m_Loot;
	bool m_Claimed;
};

typedef std::map< Go *, lootInfo * > lootMap;
typedef std::multimap< float, lootInfo * > distanceMap;

////////////////////////////////////////
//	
struct collectorInfo
{
public:
	collectorInfo( Go * go )
	{
		m_Collector			= go;
		m_JobsAssigned		= 0;	
		m_LastLootPos		= go->GetPlacement()->GetPosition();
		
		for ( int i = (int)ES_FEET; i <= (int)ES_RING_3; ++i )
		{
			if ( !go->GetInventory()->GetEquipped( (eEquipSlot)i ) )
			{
				m_FreeSlots.push_back( (eEquipSlot)i );
			}
		}

		m_bRangedWeapon	= go->GetInventory()->GetItem( IL_ACTIVE_RANGED_WEAPON ) ? true : false;
		m_bMeleeWeapon	= go->GetInventory()->GetItem( IL_ACTIVE_MELEE_WEAPON ) ? true : false;
		m_bShield		= go->GetInventory()->GetItem( IL_SHIELD ) ? true : false;
	}	

	bool CanGet( Go * go )
	{
		gUIShell.SetIgnoreItemTexture( true );
		bool bCanGet = ( m_Collector->GetInventory()->GetGridbox()->GetFullRatio() < 1.0f || go->IsGold() ) && (m_Collector->GetInventory()->IsPackOnly() || m_Collector->GetInventory()->TestGet( go->GetGoid(), true ));
		gUIShell.SetIgnoreItemTexture( false );
		return bCanGet;
	}

	bool CanEquip( Go * go )
	{
		if ( m_Collector->GetInventory()->TestEquipPassive( go->GetGoid() ) )
		{
			if ( go->IsRangedWeapon() )
			{
				if ( m_bRangedWeapon )
				{
					return false;
				}

				m_bRangedWeapon = true;
				return true;
			}
			else if ( go->IsMeleeWeapon() )
			{
				if ( m_bMeleeWeapon )
				{
					return false;
				}

				m_bMeleeWeapon = true;
				return true;
			}
			if ( go->IsShield() )
			{
				if ( m_bShield )
				{
					return false;
				}

				m_bShield = true;
				return true;
			}

			std::vector< eEquipSlot >::iterator i;
			for ( i = m_FreeSlots.begin(); i != m_FreeSlots.end(); ++i )
			{
				if ( go->GetGui()->GetEquipSlot() == (*i) )
				{
					m_FreeSlots.erase( i );
					return true;
				}
				else if ( go->GetGui()->GetEquipSlot() == ES_RING )
				{
					if ( (*i) == ES_RING_0 || (*i) == ES_RING_1 || (*i) == ES_RING_2 || (*i) == ES_RING_3 )
					{
						m_FreeSlots.erase( i );
						return true;
					}
				}
			}
		}
		return false;
	}

	Go *		m_Collector;
	GoidColl	m_MaybeCollected;
	GoidColl	m_NearbyPackers;

	DWORD		m_JobsAssigned;	
	SiegePos	m_LastLootPos;

	bool		m_bRangedWeapon;
	bool		m_bMeleeWeapon;
	bool		m_bShield;

	std::vector< eEquipSlot > m_FreeSlots;
};

typedef std::map< Go *, collectorInfo * > collectorMap;


void AIAction :: RSCollectLoot( const GoidColl& gos )
{
	if( !gos.empty() )
	{
		if ( ::IsSinglePlayer() )
		{
			const_mem_ptr ptr( (const void*)&gos.front(), gos.size() * sizeof( GoidColl::value_type ) );
			RSCollectLoot( ptr );
		}
		else if ( ::IsMultiPlayer() )
		{
			PackageAndSendLootMp( *(gos.begin()) );
		}
	}
}


FuBiCookie AIAction :: RSCollectLoot( const_mem_ptr ptr )
{
	FUBI_RPC_CALL_RETRY( RSCollectLoot, RPC_TO_SERVER );

	GoidColl gos;
	Goid * pgoid = (Goid*)ptr.mem;
	for( DWORD j = 0; j != ( ptr.size / sizeof( Goid ) ); ++j )
	{
		gos.push_back( *pgoid );
		++pgoid;
	}		
	
	SCollectLoot( gos );

	return( RPC_SUCCESS );
}



FuBiCookie AIAction::RSCollectLootMp( Goid member, const_mem_ptr ptr )
{
	FUBI_RPC_CALL_RETRY( RSCollectLootMp, RPC_TO_SERVER );

	GoHandle hMember( member );
	if ( hMember && hMember->GetPlayer()->GetMachineId() != gServer.GetLocalHumanPlayer()->GetMachineId() )
	{	
		UIGridbox * pGridbox = GetGridboxFromGO( hMember, 1 ); 
		if ( pGridbox )
		{
			FuBi::BitPacker packer( ptr );
			pGridbox->Xfer( packer );
			hMember->GetInventory()->SetGridbox( pGridbox );
			pGridbox->SetVisible( false );
		}
	}

	GoidColl members;
	members.push_back( member );
	SCollectLoot( members );

	if ( hMember->GetInventory()->HasGridbox() && hMember->GetPlayer()->GetMachineId() != gServer.GetLocalHumanPlayer()->GetMachineId() )
	{
		hMember->GetInventory()->GetGridbox()->ResetGrid();
		hMember->GetInventory()->SetGridbox( NULL );
	}

	return( RPC_SUCCESS );
}

void AIAction::PackageAndSendLootMp( Goid member )
{
	GoHandle hMember( member );	
	if ( hMember )
	{	
		FuBi::BitPacker packer;
		hMember->GetInventory()->GetGridbox()->Xfer( packer );				
		RSCollectLootMp( member, packer.GetSavedBits() );						
	}
}


void AIAction::SCollectLoot( GoidColl & members )
{
	CHECK_SERVER_ONLY;
	
	////////////////////////////////////////////////////////////////////////////////
	//	build initial data structures

	lootMap lootmap;
	collectorMap collectormap;
	collectorMap packermap;
	int goldCount = 0;

	GopColl everyone;
	members.Translate( everyone );

	GopColl collectors;
	GopColl packers;	

	DWORD machineId = 0xffffffff;
	for( GopColl::iterator it = everyone.begin(); it != everyone.end(); ++it )
	{
		gpassert( (*it)->HasInventory() && (*it)->HasActor() && (*it)->HasMind() );
		if ( ::IsAlive( (*it)->GetAspect()->GetLifeState() ) )
		{			
			if( (*it)->GetInventory()->IsPackOnly() )
			{
				packers.push_back( *it );
				machineId = (*it)->GetPlayer()->GetMachineId();
			}
			else
			{
				collectors.push_back( *it );
				machineId = (*it)->GetPlayer()->GetMachineId();
			}
		}
	}

	////////////////////////////////////////
	//	if there are no collectors, and just packers
	//	then the packers become the collectors

	if( collectors.empty() && !packers.empty() )
	{
		packers.swap( collectors );
	}

	//	prep collectors
	{
		for( GopColl::iterator i = collectors.begin(); i != collectors.end(); ++i )
		{
			gpassert( (*i)->HasActor() && (*i)->HasMind() && (*i)->HasInventory() );
			gpverify( collectormap.insert( std::make_pair( (*i), new collectorInfo(*i) ) ).second );
		}
	}

	// put all potential loot that can be seen into the loot db
	{
		for( GopColl::iterator i = everyone.begin(); i != everyone.end(); ++i )
		{		
			gpassert( (*i)->HasActor() && (*i)->HasMind() && (*i)->HasInventory() );
			GoidColl::const_iterator ivbegin = (*i)->GetMind()->GetItemsVisible().begin();
			GoidColl::const_iterator ivend   = (*i)->GetMind()->GetItemsVisible().end();

			for( GoidColl::const_iterator iv = ivbegin; iv != ivend; ++iv )
			{
				GoHandle item( (*iv) );
				// make sure to only pick up items that are around...  if we have already
				// deleted them, then don't pick it up. 
				if( !item || item->IsMarkedForDeletion() )
				{
					continue;
				}

				if ( (*i)->GetPlayer() && !(*i)->GetPlayer()->IsAllowedLootPickup( item->GetGoid() ) )
				{
					continue;
				}
				// make sure item only appears once in map
				lootMap::iterator il = lootmap.find( item.Get() );

				if( ( il == lootmap.end() ) &&
					( item->HasGui() && item->IsSelectable() ) &&
					( !item->HasActor() || ( item->HasActor() && item->GetActor()->GetCanBePickedUp() ) ) )
				{
					if ( item->HasGold() )
					{
						goldCount++;
					}
					gpverify( lootmap.insert( std::make_pair( item.Get(), new lootInfo( item.Get() ) ) ).second );
				}
			}		
		}
	}

	int lootCount = lootmap.size();

	////////////////////////////////////////
	//	assign work to collectors until we run out of loot or space

	// let's build a remove list so we can remove temporary items out of our gridbox
	std::multimap< Go *, int > memberToLootRemoveMap;

	DWORD noRoomCount = 0;	
	while( !((noRoomCount >= collectormap.size()) || lootmap.empty())  )
	{
		noRoomCount = 0;
		for( collectorMap::iterator ic = collectormap.begin(); ic != collectormap.end(); ++ic )
		{
			////////////////////////////////////////
			//	build loot distance map

			distanceMap distancemap;

			{
				for( lootMap::iterator il = lootmap.begin(); il != lootmap.end(); ++il )
				{
					if( !(*il).second->m_Claimed )
					{
						distancemap.insert( std::make_pair( gAIQuery.GetDistance( (*ic).second->m_LastLootPos, (*il).first->GetPlacement()->GetPosition() ), (*il).second ) );
					}
				}
			}

			////////////////////////////////////////
			//	assign get for closest item

			bool bNoRoom = true;
			for( distanceMap::iterator id = distancemap.begin(); id != distancemap.end(); ++id )
			{
				bool bEquipPassive = (*ic).second->CanEquip( (*id).second->m_Loot );
				if( (*ic).second->CanGet( (*id).second->m_Loot ) || bEquipPassive )
				{
					if ( !bEquipPassive && !(*id).second->m_Loot->HasGold() )
					{
						if ( !(*ic).first->GetInventory()->GetGridbox()->ContainsID( MakeInt( (*id).second->m_Loot->GetGoid() ) ) )
						{
							// We don't want to load any texture we don't need 
							if ( machineId != gServer.GetLocalHumanPlayer()->GetMachineId() )
							{
								gUIShell.SetIgnoreItemTexture( true );
							}
							(*ic).first->GetInventory()->GetGridbox()->SetAutoPlaceNotify( false );
							if( !(*ic).first->GetInventory()->GetGridbox()->AutoItemPlacement( (*ic).first->GetInventory()->BuildUiName( (*id).second->m_Loot ), true, MakeInt( (*id).second->m_Loot->GetGoid() ) ) )
							{
								(*ic).first->GetInventory()->GetGridbox()->SetAutoPlaceNotify( true );
								gUIShell.SetIgnoreItemTexture( false );
								continue;
							}
							(*ic).first->GetInventory()->GetGridbox()->SetAutoPlaceNotify( true );
							gUIShell.SetIgnoreItemTexture( false );
						}
						memberToLootRemoveMap.insert( std::make_pair( (*ic).first, MakeInt( (*id).second->m_Loot->GetGoid() ) ) );
					}

					// assign job
					Goid lootGoid = (*id).second->m_Loot->GetGoid();

					(*ic).first->GetMind()->RSDoJob( MakeJobReq( JAT_GET, JQ_ACTION, (*ic).second->m_JobsAssigned ? QP_BACK : QP_CLEAR, AO_REFLEX, lootGoid ) );
					(*ic).second->m_MaybeCollected.push_back( lootGoid );
					++(*ic).second->m_JobsAssigned;
					lootMap::iterator il = lootmap.find( (*id).second->m_Loot );
					gpassert( il != lootmap.end() );

					// record changes
					(*ic).second->m_LastLootPos			=  (*id).second->m_Loot->GetPlacement()->GetPosition();

					(*id).second->m_Claimed				= true;

					if ( (*id).second->m_Loot->HasGold() )
					{
						goldCount--;
					}					

					// delete loot record
					delete (*il).second;
					lootmap.erase( il );

					bNoRoom = false;					

					break;
				}					
			}

			if ( bNoRoom )
			{
				noRoomCount++;
			}			
		}
	}

	std::multimap< Go *, int >::iterator iRemoveLoot;
	for ( iRemoveLoot = memberToLootRemoveMap.begin(); iRemoveLoot != memberToLootRemoveMap.end(); ++iRemoveLoot )
	{
		GoHandle hItem( MakeGoid( (*iRemoveLoot).second ) );
		if ( hItem.IsValid() && (*iRemoveLoot).first->GetInventory()->Contains( hItem ) && (*iRemoveLoot).first->GetInventory()->GetLocation( hItem ) == IL_MAIN && !hItem->IsEquipped() )
		{
			continue;
		}
		(*iRemoveLoot).first->GetInventory()->GetGridbox()->RemoveID( (*iRemoveLoot).second );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	if we have packers AND collectors, have the collectors give their loot to packers
	//	now that they have collected the loot

	if( !collectors.empty() && !packers.empty() )
	{
		////////////////////////////////////////////////////////////////////////////////
		//	init packer maps so we can predict how full their inventories are getting

		{
			for( GopColl::iterator ic = packers.begin(); ic != packers.end(); ++ic )
			{
				gpverify( packermap.insert( std::make_pair( (*ic), new collectorInfo(*ic) ) ).second );
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		//	make sure each collector has a distance-sorted list of nearby packers

		for( collectorMap::iterator ic = collectormap.begin(); ic != collectormap.end(); ++ic )
		{
			GoHandle collector( (*ic).second->m_Collector );
			gpassert( collector );

			GopColl tempPackers;

			gAIQuery.SortByDistance( collector.Get(), packers, tempPackers );
			tempPackers.Translate( (*ic).second->m_NearbyPackers );
		}

		////////////////////////////////////////////////////////////////////////////////
		//	have each collector give the loot to packer(s)

		/*
			for each collector
			{
				for each looted item
				{
					eliminate full packers from collector info packer list
					if item really was picked up
					{
						give to closest packer that has room
						if no packer has toom, remove loot from consideration
					}
					else
					{
						remove loot from consideration
					}
				}
			}
		*/

	
		for( ic = collectormap.begin(); ic != collectormap.end(); ++ic )
		{
			////////////////////////////////////////
			//	for each collector, remove full packers from consideration and give orders to offload loot

			GoHandle collector( (*ic).second->m_Collector );
			gpassert( collector );

			bool gavePackers = true;

			for( GoidColl::iterator il = (*ic).second->m_MaybeCollected.begin(); il != (*ic).second->m_MaybeCollected.end() && gavePackers; )
			{
				GoHandle loot( *il );
				gpassert( loot );

				////////////////////////////////////////
				// eliminate full packers from collector's consideration

				for( GoidColl::iterator ip = (*ic).second->m_NearbyPackers.begin(); ip != (*ic).second->m_NearbyPackers.end(); )
				{
					bool removePacker = false;

					GoHandle packer( *ip );
					if( packer )
					{
						collectorMap::iterator j = packermap.find( packer.Get() );
						if( j != packermap.end() )
						{
							if( packer->GetInventory()->GetGridbox()->GetFullRatio() == 1.0f )
							{
								removePacker = true;
							}
						}
						else
						{
							removePacker = true;
						}
					}
					else
					{
						removePacker = true;
					}

					if( removePacker )
					{
						ip = (*ic).second->m_NearbyPackers.erase( ip );
					}
					else
					{
						++ip;
					}
				}

				////////////////////////////////////////////////////////////////////////////////
				//	find closest packer and give him the loot

				int gavePackerCount = 0;
				{
					std::multimap< Go *, int > packerToLootRemoveMap;

					for( GoidColl::iterator ip = (*ic).second->m_NearbyPackers.begin(); ip != (*ic).second->m_NearbyPackers.end(); ++ip )
					{
						//Go * debugCollector = (*ic).first;

						GoHandle packer( *ip );
						gpassert( packer );
						gpassert( packer->GetInventory()->IsPackOnly() );

						collectorMap::iterator j = packermap.find( packer.Get() );
						gpassert( j != packermap.end() );						

						if( (*j).second->CanGet( loot ) )
						{
							(*ic).first->GetMind()->SDoJob( MakeJobReq( JAT_GIVE, JQ_ACTION, QP_BACK, AO_REFLEX, *ip, *il ) );							

							if ( !packer->GetInventory()->GetGridbox()->ContainsID( MakeInt( loot->GetGoid() ) ) && !loot->HasGold() )
							{
								if ( machineId != gServer.GetLocalHumanPlayer()->GetMachineId() )
								{
									gUIShell.SetIgnoreItemTexture( true );
								}
								packer->GetInventory()->GetGridbox()->SetAutoPlaceNotify( false );
								if ( !packer->GetInventory()->GetGridbox()->AutoItemPlacement( (*ic).first->GetInventory()->BuildUiName( loot ), true, MakeInt( loot->GetGoid() ) ) )
								{
									packer->GetInventory()->GetGridbox()->SetAutoPlaceNotify( true );
									gUIShell.SetIgnoreItemTexture( false );
									continue;
								}
								packer->GetInventory()->GetGridbox()->SetAutoPlaceNotify( true );								
								gUIShell.SetIgnoreItemTexture( false );
							}
							packerToLootRemoveMap.insert( std::make_pair( packer.Get(), MakeInt( loot->GetGoid() ) ) );

							il = (*ic).second->m_MaybeCollected.erase( il );
							++gavePackerCount;
							break;
						}
					}

					std::multimap< Go *, int >::iterator iRemoveLoot;
					for ( iRemoveLoot = packerToLootRemoveMap.begin(); iRemoveLoot != packerToLootRemoveMap.end(); ++iRemoveLoot )
					{
						GoHandle hItem( MakeGoid( (*iRemoveLoot).second ) );
						if ( hItem.IsValid() && (*iRemoveLoot).first->GetInventory()->Contains( hItem ) )
						{
							continue;
						}
						(*iRemoveLoot).first->GetInventory()->GetGridbox()->RemoveID( (*iRemoveLoot).second );
					}
				}
				gavePackers = gavePackerCount != 0;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	finish up

	////////////////////////////////////////
	//	cleanup temp db	

	int uncollectedCount = lootmap.size();
	for( lootMap::iterator il = lootmap.begin(); il != lootmap.end(); ++il )
	{
		delete (*il).second;
	}
	lootmap.clear();
	
	for( collectorMap::iterator ic = collectormap.begin(); ic != collectormap.end(); ++ic )
	{
		delete (*ic).second;
	}
	collectors.clear();

	for( collectorMap::iterator ip = packermap.begin(); ip != packermap.end(); ++ip )
	{
		delete (*ip).second;
	}
	packermap.clear();

	if ( noRoomCount != 0 && machineId != 0xffffffff )
	{
		int numLeftover = -1;		
		if ( uncollectedCount == lootCount )
		{
			numLeftover = 0;
		}
		else if ( uncollectedCount == 1 )
		{
			numLeftover = 1;			
		}
		else if ( (lootCount-uncollectedCount) < lootCount )
		{
			numLeftover = uncollectedCount;			
		}		

		RCCollectLoot( numLeftover, machineId );		
	}		
}


void AIAction::RCCollectLoot( int numLeftover, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCCollectLoot, machineId );

	if ( numLeftover == 1 )
	{
		gpscreen( $MSG$ "One item will not fit into inventory.\n" );
	}
	else if ( numLeftover == 0 )
	{
		gpscreen( $MSG$ "No more items will fit into your inventory.\n" );
	}
	else if ( numLeftover != -1 )
	{
		gpscreen( $MSG$ "Some items will not fit into your inventory.\n" );
	}	
}


void AIAction::RSDrinkLifeHealingPotions( GoidColl & gos, eActionOrigin jo )
{
	if( !gos.empty() )
	{
		const_mem_ptr ptr( (void*)&gos.front(),  gos.size() * sizeof( GoidColl::value_type ) );
		RSDrinkLifeHealingPotions( ptr, jo );
	}
}


void AIAction::RSDrinkManaHealingPotions( GoidColl & gos, eActionOrigin jo )
{
	if( !gos.empty() )
	{
		const_mem_ptr ptr( (void*)&gos.front(),  gos.size() * sizeof( GoidColl::value_type ) );
		RSDrinkManaHealingPotions( ptr, jo );
	}
}


void AIAction::RSCastLifeHealingSpells( GoidColl & /*gos*/, eActionOrigin /*jo*/ )
{
	// $$$ implement
}


FuBiCookie AIAction::RSDrinkLifeHealingPotions( const_mem_ptr ptr, eActionOrigin jo )
{
	FUBI_RPC_CALL_RETRY( RSDrinkLifeHealingPotions, RPC_TO_SERVER );

	GoidColl gos;
	Goid * pgoid = (Goid*)ptr.mem;
	for( DWORD j = 0; j != ( ptr.size / sizeof( Goid ) ); ++j )
	{
		gos.push_back( *pgoid );
		++pgoid;
	}

	GoidColl::const_iterator i;

	// for each client
	for( i = gos.begin(); i != gos.end(); ++i )
	{
		GoHandle actor( *i );
		if( actor && actor->HasMind() && ( !actor->GetMind()->AmBusy() || (actor->GetAspect()->GetLifeRatio() < 0.5)) )
		{
			actor->GetMind()->RSDrinkLifeHealingPotion( jo );
		}
	}
	return( RPC_SUCCESS );
}


FuBiCookie AIAction::RSDrinkManaHealingPotions( const_mem_ptr ptr, eActionOrigin jo )
{
	FUBI_RPC_CALL_RETRY( RSDrinkManaHealingPotions, RPC_TO_SERVER );

	GoidColl gos;
	Goid * pgoid = (Goid*)ptr.mem;
	for( DWORD j = 0; j != ( ptr.size / sizeof( Goid ) ); ++j )
	{
		gos.push_back( *pgoid );
		++pgoid;
	}

	GoidColl::const_iterator i;

	// for each member
	for( i = gos.begin(); i != gos.end(); ++i )
	{
		GoHandle actor( *i );
		if( actor && actor->HasMind() && ( !actor->GetMind()->AmBusy() || (actor->GetAspect()->GetManaRatio() < 0.5)) )
		{
			actor->GetMind()->RSDrinkManaHealingPotion( jo );
		}
	}
	return( RPC_SUCCESS );
}


FuBiCookie AIAction::RSCastLifeHealingSpells(   const_mem_ptr /*ptr*/, eActionOrigin /*jo*/ )
{
	FUBI_RPC_CALL_RETRY( RSCastLifeHealingSpells, RPC_TO_SERVER );
	// $$$ implement
	return( RPC_SUCCESS );
}


JobCloneSourceMap::iterator AIAction::RegisterJobCloneSource( gpstring const & str, bool delayJobInit )
{
	JobCloneSourceMap::iterator i = m_JobCloneSourceMap.find( str );

	if( i != m_JobCloneSourceMap.end() )
	{
		RegisterReference( i );
	}
	else
	{
		JobCacheSlot * pSlot = new JobCacheSlot;

		Skrit::CreateReq createReq( str );
		createReq.SetPersist( false );
		createReq.m_DeferConstruction = delayJobInit;
		pSlot->m_hSkrit.CreateSkrit( createReq );
		gpassert( !pSlot->m_hSkrit.IsNull() );

		pSlot->m_RefCount	= 1;
		pSlot->m_SkritPath	= str;

		i = m_JobCloneSourceMap.insert( m_JobCloneSourceMap.begin(), std::make_pair( str, pSlot ) );
		gpassert( (*i).second );
	}

	return i;
}


void AIAction::RegisterReference( JobCloneSourceMap::iterator i )
{
	gpassert( i != m_JobCloneSourceMap.end() );
	++(*i).second->m_RefCount;
}


void AIAction::UnregisterReference( JobCloneSourceMap::iterator i )
{
	gpassert( i != m_JobCloneSourceMap.end() );
	--(*i).second->m_RefCount;
	gpassert( (*i).second->m_RefCount >= 0 );
	if( (*i).second->m_RefCount == 0 )
	{
		delete (*i).second;
		m_JobCloneSourceMap.erase( i );
	}
}
