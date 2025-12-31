/***********************************************************************
**
**							SiegeFrustum
**
**	See .h file for details
**
***********************************************************************/

#include "precomp_siege.h"

// Core includes
#include "gpcore.h"
#include "gpprofiler.h"
#include "reportsys.h"
#include "stdhelp.h"

// Siege includes
#include "siege_defs.h"
#include "siege_engine.h"
#include "siege_frustum.h"
#include "siege_loader.h"
#include "siege_persist.h"
#include "siege_walker.h"

// Xfer includes
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"

using namespace siege;

const float FrustumUpdateTime					= 0.5f;
unsigned int SiegeFrustum::ms_usedFrustumMask	= 0;

#if !GP_RETAIL

static ReportSys::Context* gSiegeFrustumContextPtr   = NULL;
static ReportSys::Context* gSiegeNodeContextPtr		 = NULL;

ReportSys::Context& GetSiegeFrustumContext( void )
{
	if ( gSiegeFrustumContextPtr == NULL )
	{
		static ReportSys::Context s_SiegeFrustumContext( &gGenericContext, "SiegeFrustum" );
		gSiegeFrustumContextPtr = &s_SiegeFrustumContext;

		s_SiegeFrustumContext.Enable( false );
	}
	return ( *gSiegeFrustumContextPtr );
}

ReportSys::Context& GetSiegeNodeContext( void )
{
	if ( gSiegeNodeContextPtr == NULL )
	{
		static ReportSys::Context s_SiegeNodeContext( &gGenericContext, "SiegeNode" );
		gSiegeNodeContextPtr = &s_SiegeNodeContext;

		s_SiegeNodeContext.Enable( false );
	}
	return ( *gSiegeNodeContextPtr );
}

#endif // !GP_RETAIL

#define gpsiegefrustum( msg ) DEV_MACRO_REPORT( &GetSiegeFrustumContext(), msg )
#define gpsiegefrustumf( msg ) DEV_MACRO_REPORTF( &GetSiegeFrustumContext(), msg )

#define gpsiegenode( msg ) DEV_MACRO_REPORT( &GetSiegeNodeContext(), msg )
#define gpsiegenodef( msg ) DEV_MACRO_REPORTF( &GetSiegeNodeContext(), msg )


// Construction
SiegeFrustum::SiegeFrustum( unsigned int idBitfield )
	: m_bUpdateSpace( true )
	, m_bUpdateFrustum( true )
	, m_bFullLoad( true )
	, m_bDiscoverySubmit( true )
	, m_frustumWidth( 0.0f )
	, m_frustumHeight( 0.0f )
	, m_frustumDepth( 0.0f )
	, m_interestRadius( 0.0f )
	, m_ownedFrustums( 0x00000000 )
	, m_currentTime( 0.0 )
{
	// choose next id if requested
	if ( idBitfield == 0 )
	{
		idBitfield = GetNextFreeIdBitfield();
	}

	// take id's
	m_ownerFrustum = m_idBitfield = idBitfield;

	// register them
	gpassert( IsPower2( m_idBitfield ) && !BitFlagsContainAny( ms_usedFrustumMask, m_idBitfield ) );
	BitFlagsSet( ms_usedFrustumMask, m_idBitfield );
}

// Destruction
SiegeFrustum::~SiegeFrustum()
{
	// Unmark nodes that are owned by this frustum
	for( FrustumNodeColl::iterator n = m_ownedNodeColl.begin(); n != m_ownedNodeColl.end(); ++n )
	{
		// Release ownership of this node
		(*n)->SetFrustumOwnership( m_idBitfield, false );

		// Indicate upgraded status
		bool bWasUpgraded	= (*n)->IsUpgradedByFrustum( m_idBitfield );
		(*n)->SetUpgradedOwnership( m_idBitfield, false );

		// Release this node from my space
		if( (*n)->GetSpaceFrustum() == m_idBitfield )
		{
			(*n)->SetSpaceFrustum( 0 );
		}

		// If no one owns it, unload it
		if( !(*n)->IsOwnedByAnyFrustum() && !(*n)->IsLoadedByAnyFrustum() )
		{
			// Unload node
			UnloadNode( (*n) );
		}
		else if( bWasUpgraded && !(*n)->IsUpgradedByAnyFrustum() )
		{
			// Downgrade the node's loaded status
			(*n)->DowngradeLoad();
		}
	}

	// Go through any nodes in our loaded list and unload them
	for( n = m_loadedNodeColl.begin(); n != m_loadedNodeColl.end(); ++n )
	{
		gpassert( !(*n)->IsOwnedByFrustum( m_idBitfield ) );
		gpassert( (*n)->IsLoadedByFrustum( m_idBitfield ) );

		// Indicate loaded status
		(*n)->SetLoadedOwnership( m_idBitfield, false );

		// Indicate upgraded status
		bool bWasUpgraded	= (*n)->IsUpgradedByFrustum( m_idBitfield );
		(*n)->SetUpgradedOwnership( m_idBitfield, false );

		// Cancel outstanding orders in the manager, which should never happen
		// because the frustum destruction code SHOULD commit all node load
		// orders before this.  This is basically just a catch.
		if( LoadMgr::DoesSingletonExist() && !(*n)->IsLoadedPhysical() )
		{
			if( gSiegeLoadMgr.CancelById( LOADER_SIEGE_NODE_TYPE, (*n)->GetGUID().GetValue() ) )
			{
				gperror( "Cancelling node load order when the order should have already been committed!!" );
			}
		}

		// Remove any nodes that have completely loaded
		if( (*n)->IsLoadedPhysical() && !(*n)->IsOwnedByAnyFrustum() && !(*n)->IsLoadedByAnyFrustum() )
		{
			UnloadNode( (*n) );
		}
		else if( bWasUpgraded && !(*n)->IsUpgradedByAnyFrustum() )
		{
			// Downgrade the node's loaded status
			(*n)->DowngradeLoad();
		}
	}

	// Release any owned nodes
	if( !IsFrustumOwned() )
	{
		ReleaseOwnedFrustumHierarchy( m_ownedFrustums );
	}
	else
	{
		// Get parent frustum
		SiegeFrustum* pParentFrustum		= gSiegeEngine.GetFrustum( m_ownerFrustum );
		if( pParentFrustum )
		{
			// Release ownership
			pParentFrustum->m_ownedFrustums	&= ~m_idBitfield;
		}
	}

	// Clear my slot
	gpassert( BitFlagsContainAny( ms_usedFrustumMask, m_idBitfield ) );
	BitFlagsClear( ms_usedFrustumMask, m_idBitfield );

	// Was I the render frustum?
	if ( gSiegeEngine.GetRenderFrustum() == m_idBitfield )
	{
		gSiegeEngine.ClearRenderFrustum();
	}

	// Clear collections
	m_loadedNodeColl.clear();
	m_ownedNodeColl.clear();
	m_discoveryPosColl.clear();
	m_prevOwnedNodes.clear();
}

// Persistence
bool SiegeFrustum::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_frustumPos",			m_frustumPos );
	persist.Xfer( "m_reqFrustumPos",		m_reqFrustumPos );
	persist.Xfer( "m_frustumWidth",			m_frustumWidth );
	persist.Xfer( "m_frustumHeight",		m_frustumHeight );
	persist.Xfer( "m_frustumDepth",			m_frustumDepth, m_frustumHeight );
	persist.Xfer( "m_interestRadius",		m_interestRadius );
	persist.Xfer( "m_minBox", 				m_minBox );
	persist.Xfer( "m_maxBox", 				m_maxBox );
	persist.XferVector( "m_nodeFadeColl",	m_nodeFadeColl );

	m_prevOwnedNodes.clear();
	if( persist.IsSaving() )
	{
		FrustumNodeColl::iterator i, ibegin = m_ownedNodeColl.begin(), iend = m_ownedNodeColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			m_prevOwnedNodes.push_back( (*i)->GetGUID() );
		}
	}
	persist.XferVector( "m_prevOwnedNodes",	m_prevOwnedNodes );


#	if !GP_RETAIL
	if ( persist.IsSaving() )
	{
		persist.EnterBlock( "DEBUG" );

		persist.Xfer( "m_ownerFrustum", m_ownerFrustum );
		persist.Xfer( "m_ownedFrustums", m_ownedFrustums );

		persist.EnterColl( "m_loadedNodeColl" );
		{
			FrustumNodeColl::iterator i, ibegin = m_loadedNodeColl.begin(), iend = m_loadedNodeColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				persist.AdvanceCollIter();
				database_guid guid = (*i)->GetGUID();
				persist.Xfer( "*", guid );
			}
		}
		persist.LeaveColl();

		persist.EnterColl( "m_ownedNodeColl" );
		{
			FrustumNodeColl::iterator i, ibegin = m_ownedNodeColl.begin(), iend = m_ownedNodeColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				persist.AdvanceCollIter();
				database_guid guid = (*i)->GetGUID();
				persist.Xfer( "*", guid );
			}
		}
		persist.LeaveColl();

		persist.LeaveBlock();
	}
#	endif // !GP_RETAIL

	if( persist.IsRestoring() )
	{
		// We need to update space
		m_bUpdateSpace		= true;
		m_bUpdateFrustum	= true;
	}

	return ( true );
}

// Retrieve next free id
const unsigned int SiegeFrustum::GetNextFreeIdBitfield()
{
	// note that we have to remove the reserved bit from here just in case it's
	// already been used. don't want that to mess up the results.
	unsigned int bits = ms_usedFrustumMask & ~(1 << 31);

	// too many? fail it.
	if ( bits & (1 << 30) )
	{
		return ( 0 );
	}

	// ok get the next one
	return ( 1 << GetZeroShift( ~bits ) );
}

// Retrieve the reserved ID used for local operations
const unsigned int SiegeFrustum::GetReservedIdBitfield()
{
	return ( 1U << 31 );
}

// Update frustum
bool SiegeFrustum::Update( const float secondsElapsed )
{
	GPPROFILERSAMPLE( "SiegeFrustum::Update()", SP_DRAW_CALC_TERRAIN );

	if( !m_bUpdateFrustum )
	{
		if( gSiegeEngine.m_nodeReplacementMap.empty() )
		{
			return false;
		}
		else
		{
			m_currentTime		= PreciseAdd(m_currentTime, secondsElapsed);
			if( m_currentTime < FrustumUpdateTime )
			{
				return false;
			}
			else
			{
				m_currentTime	= 0.0;

				// Update discovery position
				if( gSiegeEngine.SpaceDiffers( m_frustumPos.node, m_lastDiscoveryPos.node ) ||
					gSiegeEngine.GetDifferenceVector( m_frustumPos, m_lastDiscoveryPos ).Length2() > Square( gSiegeEngine.m_DiscoverySampleLength ) )
				{
					// Submit a new discovery position
					gSiegeEngine.NodeWalker().SubmitDiscoveryPosition( m_reqFrustumPos, this );

					// Save the old discovery position
					m_lastDiscoveryPos	= m_reqFrustumPos;
				}

				// Check to see if we have moved far enough that we need to update the frustum.
				if( gSiegeEngine.SpaceDiffers( m_frustumPos.node, m_lastUpdatePos.node ) ||
					gSiegeEngine.GetDifferenceVector( m_frustumPos, m_lastUpdatePos ).Length2() > Square( gSiegeEngine.m_FrustumUpdateDistance ) )
				{
					// Save the last update position
					m_lastUpdatePos	= m_reqFrustumPos;
					
					// need to update
					m_bUpdateFrustum	= true;
				}
			}
		}
	}

	if( m_frustumPos.node == UNDEFINED_GUID )
	{
		return false;
	}

	SiegeNode* pTargetNode = gSiegeEngine.DoesNodeExist( m_frustumPos.node );
	if( !pTargetNode )
	{
		if( !LoadNode( m_frustumPos.node ) )
		{
			return true;
		}

		// Node should exist now
		pTargetNode = gSiegeEngine.DoesNodeExist( m_frustumPos.node );
		if( !pTargetNode )
		{
			gperror( "Could not load target!  Must be bad position." );
			return true;
		}
	}

	// Make sure (for sure) that the target node is loaded
	if( !pTargetNode->IsLoadedPhysical() )
	{
		LoadNode( pTargetNode->GetGUID() );
		return true;
	}

	// Setup visit count
	unsigned int visitCount		= RandomDword();

	// We need to keep track of which of our owned frustums we visit this walk
	DWORD ownedFrustumsVisited	= 0;

	// Set the target node as owned
	if( !pTargetNode->IsOwnedByFrustum( m_idBitfield ) )
	{
		// Set it's ownership bit
		pTargetNode->SetFrustumOwnership( m_idBitfield, true );

		// Remove node from the loaded collection
		for( FrustumNodeColl::iterator n = m_loadedNodeColl.begin(); n != m_loadedNodeColl.end(); ++n )
		{
			if( (*n)->GetGUID() == pTargetNode->GetGUID() )
			{
				// Indicate loaded status
				pTargetNode->SetLoadedOwnership( m_idBitfield, false );
			
				// Erase from loaded collection
				m_loadedNodeColl.erase( n );
				break;
			}
		}

		// Upgrade the node to fully loaded if this frustum needs it
		if( m_bFullLoad && !pTargetNode->IsUpgradedByFrustum( m_idBitfield ) )
		{
			// Upgrade the node if it requires it
			if( !pTargetNode->IsUpgradedByAnyFrustum() )
			{
				pTargetNode->UpgradeLoad();
			}

			// Set upgrade status for this frustum
			pTargetNode->SetUpgradedOwnership( m_idBitfield, true );
		}

		// Update node alpha when it enters owned list
		gSiegeEngine.UpdateFadedNodeAlpha( *pTargetNode );

		// Insert it into our owner list
		m_ownedNodeColl.push_back( pTargetNode );
	}
	pTargetNode->SetInterestOwnership( m_idBitfield, true );
	pTargetNode->SetLastVisited( visitCount );

	// Determine if space update is needed
	bool bUpdateFrustum	= false;
	bool bUpdateSpace	= false;
	if( m_bUpdateSpace && !IsFrustumOwned() )
	{
		bUpdateSpace				= true;

		// Enter the space critical
		gSiegeEngine.GetSpaceCritical().Enter();

		// Set the origin of the world
		pTargetNode->SetCurrentCenter( vector_3::ZERO );
		pTargetNode->SetCurrentOrientation( matrix_3x3::IDENTITY );
		pTargetNode->SetSpaceFrustum( m_idBitfield );

#if !GP_RETAIL
		pTargetNode->SetSpatialParent( UNDEFINED_GUID );
#endif

		// Callback notification
		gSiegeEngine.NotifyNodeSpaceChanged( pTargetNode->GetGUID() );
	}

	// Build frustum box
	vector_3 minBox( DoNotInitialize );
	vector_3 maxBox( DoNotInitialize );
	pTargetNode->NodeToWorldBox( m_minBox, m_maxBox, minBox, maxBox );

	// Build frustum world pos
	vector_3 frustumPos		= pTargetNode->NodeToWorldSpace( m_frustumPos.pos );

	// List of nodes that are entering/leaving frustum.  This is used to defer the
	// actual transfer of membership until after the main space update.
	FrustumNodeColl enterWorldList;
	FrustumNodeColl leaveWorldList;

	// Setup the processing list
	SiegeNodeColl& nodeStack	= gSiegeEngine.m_nodeStack;
	gpassert( nodeStack.empty() );

	nodeStack.push_back( pTargetNode );

	// Walk the tree and do setup
	while( !nodeStack.empty() )
	{
		// Get the top node
		SiegeNode& curr_node		= *nodeStack.back();
		nodeStack.pop_back();

		// Get node information to setup the walk
		const SiegeNodeDoorList& dl		= curr_node.GetDoors();

		for( SiegeNodeDoorConstIter d = dl.begin(); d != dl.end(); ++d )
		{
			gpassert( (*d)->GetNeighbour() != UNDEFINED_GUID );

			// Is the far neighbour a valid node?
			SiegeNode* pFarNode = gSiegeEngine.DoesNodeExist( (*d)->GetNeighbour() );
			if( !pFarNode )
			{
				// Continue frustum update
				bUpdateFrustum	= true;

				if( bUpdateSpace )
				{
					// Leave the space critical
					gSiegeEngine.GetSpaceCritical().Leave();
				}

				if( !LoadNode( (*d)->GetNeighbour() ) )
				{
					if( bUpdateSpace )
					{
						// Enter the space critical
						gSiegeEngine.GetSpaceCritical().Enter();
					}

					continue;
				}

				if( bUpdateSpace )
				{
					// Enter the space critical
					gSiegeEngine.GetSpaceCritical().Enter();
				}

				// Node should exist now
				pFarNode = gSiegeEngine.DoesNodeExist( (*d)->GetNeighbour() );
			}

			// If this node has already been visited, leave it alone
			if( pFarNode->GetLastVisited() == visitCount )
			{
				continue;
			}

			// Set the visited count so this node knows it has been dealt with
			pFarNode->SetLastVisited( visitCount );

			// Every node that a frustum visits that it doesn't already own becomes
			// a load dependency.
			if( !pFarNode->IsOwnedByFrustum( m_idBitfield ) &&
				!pFarNode->IsLoadedByFrustum( m_idBitfield ) )
			{
				// Indicate loaded status
				pFarNode->SetLoadedOwnership( m_idBitfield, true );

				// Put this node onto our loaded listing
				m_loadedNodeColl.push_back( pFarNode );
			}

			if( !pFarNode->IsLoadedPhysical() )
			{
				if( bUpdateSpace )
				{
					// Leave the space critical
					gSiegeEngine.GetSpaceCritical().Leave();
				}

				LoadNode( pFarNode->GetGUID() );

				if( bUpdateSpace )
				{
					// Enter the space critical
					gSiegeEngine.GetSpaceCritical().Enter();
				}
				
				// Continue frustum update
				bUpdateFrustum	= true;
				continue;
			}

			// If the node has no doors, leave it alone
			if( pFarNode->GetDoors().empty() )
			{
				continue;
			}

			// Save space frustum information
			DWORD spaceFrustum	= pFarNode->GetSpaceFrustum();

			// Update space information
			if( bUpdateSpace || !pFarNode->HasValidSpace() )
			{
				if( !BuildNodeSpace( curr_node, (*d), *pFarNode ) )
				{
					// Could not properly build space for this node
					continue;
				}

				// Continue frustum update
				bUpdateFrustum	= true;
			}

			// If the node has a space frustum that is not this frustum
			if( !IsFrustumOwned() && ( (spaceFrustum != 0) && (spaceFrustum != m_idBitfield) ) )
			{
				// ... and we don't already own the frustum
				DWORD ownedAndLoaded	= pFarNode->GetLoadedAndOwnedBitfield();
				if( !DoesFrustumOwn( spaceFrustum ) && (ownedAndLoaded & spaceFrustum) != 0 )
				{
					// Take ownership of this frustum
					DWORD oldOwnedFrustums	= m_ownedFrustums;
					TakeOwnershipOfFrustumHierarchy( ownedAndLoaded );

					// Mark our owned frustum visited bits
					ownedFrustumsVisited	|= (m_ownedFrustums & ~oldOwnedFrustums);

					// Start updating space to absorb owned frustum(s) into our own
					if( !bUpdateSpace )
					{
						// Enter the space critical
						gSiegeEngine.GetSpaceCritical().Enter();

						// Build space for this node
						BuildNodeSpace( curr_node, (*d), *pFarNode );

						// Start updating space for the rest of this update
						bUpdateSpace			= true;
					}

					// Continue frustum update
					bUpdateFrustum	= true;
				}
			}

			// If this node is in my space
			if( pFarNode->GetSpaceFrustum() == m_ownerFrustum )
			{
				if( NodeIntersectsFrustum( *pFarNode, minBox, maxBox ) )
				{
					// See if we already own this node
					if( !pFarNode->IsOwnedByFrustum( m_idBitfield ) )
					{
						// Mark this node as owned
						enterWorldList.push_back( pFarNode );

						// Remove node from the owned collection
						for( FrustumNodeColl::iterator n = m_loadedNodeColl.begin(); n != m_loadedNodeColl.end(); ++n )
						{
							if( (*n)->GetGUID() == pFarNode->GetGUID() )
							{
								// Indicate loaded status
								(*n)->SetLoadedOwnership( m_idBitfield, false );

								// Erase from loaded collection
								m_loadedNodeColl.erase( n );
								break;
							}
						}

						// Update node alpha when it enters owned list
						gSiegeEngine.UpdateFadedNodeAlpha( *pFarNode );

						// Insert node into our owned collection
						m_ownedNodeColl.push_back( pFarNode );

						// Continue frustum update
						bUpdateFrustum	= true;
					}

					// Check for inclusion in the interest sphere
					if( SphereIntersectsBox( frustumPos, m_interestRadius, pFarNode->GetWorldSpaceMinimumBounds(), pFarNode->GetWorldSpaceMaximumBounds() ) )
					{
						pFarNode->SetInterestOwnership( m_idBitfield, true );
					}
					else
					{
						pFarNode->SetInterestOwnership( m_idBitfield, false );
					}

					// Upgrade the node to fully loaded if this frustum needs it
					if( m_bFullLoad && !pFarNode->IsUpgradedByFrustum( m_idBitfield ) )
					{
						// Upgrade the node if it requires it
						if( !pFarNode->IsUpgradedByAnyFrustum() )
						{
							pFarNode->UpgradeLoad();
						}

						// Set upgrade status for this frustum
						pFarNode->SetUpgradedOwnership( m_idBitfield, true );
					}

					// Mark that we have visited any frustums that may own this node
					ownedFrustumsVisited	|= (pFarNode->GetLoadedAndOwnedBitfield() & ~m_idBitfield);

					// Setup for further processing
					nodeStack.push_back( pFarNode );
				}
				else
				{
					if( pFarNode->IsOwnedByFrustum( m_idBitfield ) &&
						gSiegeEngine.m_nodeReplacementMap.find( pFarNode->GetGUID() ) == gSiegeEngine.m_nodeReplacementMap.end() )
					{
						// Release ownership of this node
						leaveWorldList.push_back( pFarNode );

						// Remove node from the owned collection
						for( FrustumNodeColl::iterator n = m_ownedNodeColl.begin(); n != m_ownedNodeColl.end(); ++n )
						{
							if( (*n)->GetGUID() == pFarNode->GetGUID() )
							{
								m_ownedNodeColl.erase( n );
								break;
							}
						}

						// Indicate loaded status
						pFarNode->SetLoadedOwnership( m_idBitfield, true );

						// Add node to loaded collection
						m_loadedNodeColl.push_back( pFarNode );

						// Continue frustum update
						bUpdateFrustum	= true;
					}

					if( OwnedFrustumOwnsNode( pFarNode->GetGUID() ) )
					{
						// Mark that we have visited this owned frustum
						ownedFrustumsVisited	|= (pFarNode->GetLoadedAndOwnedBitfield() & ~m_idBitfield);
						nodeStack.push_back( pFarNode );
					}
				}
			}
			else if( IsFrustumOwned() )
			{
				// Continue frustum update
				bUpdateFrustum	= true;
			}
		}
	}

	if( bUpdateSpace )
	{
		// Leave the space critical
		gSiegeEngine.GetSpaceCritical().Leave();
	}

	// Process our leave world nodes first
	for( FrustumNodeColl::iterator n = leaveWorldList.begin(); n != leaveWorldList.end(); ++n )
	{
		(*n)->SetFrustumOwnership( m_idBitfield, false );
	}

	// Process our enter world nodes
	for( n = enterWorldList.begin(); n != enterWorldList.end(); ++n )
	{
		(*n)->SetFrustumOwnership( m_idBitfield, true );
	}

	// Release any frustums that we did not visit
	if( !IsFrustumOwned() )
	{
		ReleaseOwnedFrustumHierarchy( (m_ownedFrustums & ~ownedFrustumsVisited) );
	}

	// Go through nodes in the owned collection put any that were not visited this update
	// into the loaded list
	for( n = m_ownedNodeColl.begin(); n != m_ownedNodeColl.end(); )
	{
		if( (*n)->GetLastVisited() != visitCount )
		{
			// Release ownership of this node
			(*n)->SetFrustumOwnership( m_idBitfield, false );

			// Indicate loaded status
			(*n)->SetLoadedOwnership( m_idBitfield, true );

			// Put this node on the loaded collection
			m_loadedNodeColl.push_back( (*n) );

			// Erase it from the owned collection
			n = m_ownedNodeColl.erase( n );
		}
		else
		{
			++n;
		}
	}

	// Go through any nodes left in our loaded list that we did not visit and unload them
	for( n = m_loadedNodeColl.begin(); n != m_loadedNodeColl.end(); )
	{
		// Make sure the node is actually loaded
		if( (*n)->IsLoadedPhysical() )
		{
			if( (*n)->GetLastVisited() != visitCount )
			{
				gpassert( !(*n)->IsOwnedByFrustum( m_idBitfield ) );

				// Indicate loaded status
				(*n)->SetLoadedOwnership( m_idBitfield, false );

				// Indicate upgraded status
				bool bWasUpgraded	= (*n)->IsUpgradedByFrustum( m_idBitfield );
				(*n)->SetUpgradedOwnership( m_idBitfield, false );

				// Release this node from my space
				if( (*n)->GetSpaceFrustum() == m_idBitfield )
				{
					(*n)->SetSpaceFrustum( 0 );
				}

				// If no one owns it, unload it
				if( !(*n)->IsOwnedByAnyFrustum() && !(*n)->IsLoadedByAnyFrustum() )
				{
					// This node is not owned by anyone and was not visited by this one, so we
					// need to unload it
					UnloadNode( (*n) );
				}
				else if( bWasUpgraded && !(*n)->IsUpgradedByAnyFrustum() )
				{
					// Downgrade the node's loaded status
					(*n)->DowngradeLoad();
				}

				// Remove it from our loaded collection
				n = m_loadedNodeColl.erase( n );

				// Continue frustum update
				bUpdateFrustum	= true;
			}
			else
			{
				++n;
			}
		}
		else
		{
			++n;
		}
	}

	// Updates have occured, so we need to reset our states
	m_bUpdateFrustum	= bUpdateFrustum;
	m_bUpdateSpace		= false;

	// Return update state
	return( m_bUpdateFrustum );
}

// Positional information
void SiegeFrustum::SetPosition( const SiegePos& pos )
{
	// Update discovery position
	if( gSiegeEngine.SpaceDiffers( pos.node, m_lastDiscoveryPos.node ) ||
		gSiegeEngine.GetDifferenceVector( pos, m_lastDiscoveryPos ).Length2() > Square( gSiegeEngine.m_DiscoverySampleLength ) )
	{
		// Submit a new discovery position
		gSiegeEngine.NodeWalker().SubmitDiscoveryPosition( pos, this );

		// Save the old discovery position
		m_lastDiscoveryPos	= pos;
	}

	// Check to see if we have moved far enough that we need to update the frustum.
	if( gSiegeEngine.SpaceDiffers( pos.node, m_lastUpdatePos.node ) ||
		gSiegeEngine.GetDifferenceVector( pos, m_lastUpdatePos ).Length2() > Square( gSiegeEngine.m_FrustumUpdateDistance ) )
	{
		// Save the last update position
		m_lastUpdatePos	= pos;
		
		// need to update
		m_bUpdateFrustum	= true;
	}

	// Save this position in case we need to change it
	SiegePos nPos			= pos;

	// See if this is a replacement node
	FrustumNodeReplacementMap::iterator i = gSiegeEngine.m_nodeReplacementMap.find( pos.node );
	for( ; i != gSiegeEngine.m_nodeReplacementMap.end(); )
	{
		FrustumNodeReplacementMap::iterator o = gSiegeEngine.m_nodeReplacementMap.find( (*i).second );
		if( o != gSiegeEngine.m_nodeReplacementMap.end() )
		{
			i = o;
		}
		else
		{
			if( gSiegeEngine.IsNodeValid( (*i).second ) )
			{
				nPos.FromWorldPos( pos.WorldPos(), (*i).second );
			}
			break;
		}
	}

	// See if the node has changed
	if( nPos.node != m_frustumPos.node )
	{
		// We need to update space
		m_bUpdateSpace		= true;
		m_bUpdateFrustum	= true;

		gpsiegefrustumf(( "SiegeFrustum %d SPACE CHANGED\n", m_idBitfield ));
	}

	
	// Save the position
	m_reqFrustumPos			= pos;
	m_frustumPos			= nPos;

	// Build box
	m_minBox				= m_frustumPos.pos - vector_3( m_frustumWidth, m_frustumDepth,  m_frustumWidth );
	m_maxBox				= m_frustumPos.pos + vector_3( m_frustumWidth, m_frustumHeight, m_frustumWidth );
}

// Size information
void SiegeFrustum::SetDimensions( const float width, const float depth, const float height )
{
	// See if dimensions really changed
	if( m_frustumWidth  != width   ||
		m_frustumHeight != height ||
		m_frustumDepth  != depth )
	{
		m_bUpdateFrustum	= true;
	}

	// Save dimensions
	m_frustumWidth			= width;
	m_frustumHeight			= height;
	m_frustumDepth			= depth;

	// Build box
	m_minBox				= m_frustumPos.pos - vector_3( width, depth,  width );
	m_maxBox				= m_frustumPos.pos + vector_3( width, height, width );

	gpsiegefrustumf(( "SiegeFrustum %d DIMENSIONS CHANGED\n", m_idBitfield ));
}

// Set frustum ownership
void SiegeFrustum::SetFrustumOwned( const unsigned int idBitfield, bool bOwned )
{
	if( bOwned )
	{
		m_ownerFrustum		= idBitfield;

		gpsiegefrustumf(( "SiegeFrustum %d TOOK OWNERSHIP of SiegeFrustum %d\n", idBitfield, m_idBitfield ));
	}
	else if( IsFrustumOwned() )
	{
		m_ownerFrustum		= m_idBitfield;

		// Update the frustum
		m_bUpdateSpace		= true;
		m_bUpdateFrustum	= true;

		gpsiegefrustumf(( "SiegeFrustum %d RELEASED OWNERSHIP of SiegeFrustum %d\n", idBitfield, m_idBitfield ));
	}
}

// See if a frustum is owned by this frustum
bool SiegeFrustum::DoesFrustumOwn( const unsigned int idBitfield )
{
	return( (m_ownedFrustums & idBitfield) != 0 );
}

// Node helper functions
bool SiegeFrustum::OwnsNode( const database_guid& nodeGuid )
{
	// Get the node
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( nodeGuid );
	SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

	// See if the node has this frustum's bit set in it's owner field
	return( (node.GetLoadedAndOwnedBitfield() & m_idBitfield) != 0 );
}

bool SiegeFrustum::OwnedFrustumOwnsNode( const database_guid& nodeGuid )
{
	// Get the node
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( nodeGuid );
	SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

	// See if the node has this frustum's bit set in it's owner field
	return( (node.GetLoadedAndOwnedBitfield() & m_ownedFrustums) != 0 );
}

// Setup a node fade, usually as a result of a triggered action
void SiegeFrustum::NodeFade( int regionID, int section, int level, int object, FADETYPE ft )
{
	// Don't do anything in this case
	if( ft == FT_NONE )
	{
		gpwarning( "Cannot attach node fade with fade type FT_NONE" );
		return;
	}

	// See if a fade with this identifier already exists
	NodeFadeColl::iterator i = stdx::binary_search( m_nodeFadeColl.begin(), m_nodeFadeColl.end(), regionID, fade_less() );
	for( ; i != m_nodeFadeColl.end(); ++i )
	{
		if( (*i).m_regionID != regionID )
		{
			break;
		}

		if( (*i).m_section	== section	&&
			(*i).m_level	== level	&&
			(*i).m_object	== object )
		{
			if( (*i).m_fadeType != ft )
			{
				(*i).m_fadeType		= ft;
				(*i).m_startTime	= ::GetGlobalSeconds();
			}
			return;
		}
	}

	// Build a node fade structure to hold the information we will need about this
	NODEFADE nf;
	nf.m_regionID	= regionID;
	nf.m_section	= section;
	nf.m_level		= level;
	nf.m_object		= object;
	nf.m_fadeType	= ft;
	nf.m_alpha		= 0xFF;
	nf.m_startTime	= ::GetGlobalSeconds();

	// Add it to our list
	m_nodeFadeColl.push_back( nf );
	std::sort( m_nodeFadeColl.begin(), m_nodeFadeColl.end(), fade_less() );
}

// Update a node's alpha fade level
void SiegeFrustum::UpdateFadedNodeAlpha( SiegeNode& node )
{
	// Go through the current fades and see if any match
	NodeFadeColl::iterator i = stdx::binary_search( m_nodeFadeColl.begin(), m_nodeFadeColl.end(), node.GetRegionID(), fade_less() );
	for( ; i != m_nodeFadeColl.end(); ++i )
	{
		if( (*i).m_regionID != node.GetRegionID() )
		{
			break;
		}

		// See if we need to fade this node
		if( ((node.GetSection() == (*i).m_section)	|| ((*i).m_section	== -1)) &&
			((node.GetLevel()	== (*i).m_level)	|| ((*i).m_level	== -1)) &&
			((node.GetObject()	== (*i).m_object)	|| ((*i).m_object	== -1)) )
		{
			node.SetNodeAlpha( (*i).m_alpha );
			return;
		}
	}

	node.SetNodeAlpha( (BYTE)0xFF );
}

// See if a node intersects the frustum
bool SiegeFrustum::NodeIntersectsFrustum( const SiegeNode& node, vector_3& minBox, vector_3& maxBox )
{
	return( BoxIntersectsBox( node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds(), minBox, maxBox ) );	
}

// Take ownership of a frustum hierarchy
void SiegeFrustum::TakeOwnershipOfFrustumHierarchy( DWORD frustumBitfield )
{
	// Check all possible combinations for set frustum bit
	for( unsigned int i = 0; i < 32; ++i )
	{
		DWORD frustum	= frustumBitfield & (1 << i);
		if( frustum )
		{
			// Get the frustum
			SiegeFrustum* pFrustum	= gSiegeEngine.GetFrustum( frustum );
			if( pFrustum && (pFrustum->GetFrustumOwned() != m_idBitfield) )
			{
				DWORD ownedBy	= pFrustum->GetFrustumOwned();

				// Release ownership of the frustum
				pFrustum->SetFrustumOwned( m_idBitfield, true );

				// Set owned bit
				m_ownedFrustums		|= frustum;

				// If this frustum is already owned, recursively take ownership of its parent
				if( ownedBy != pFrustum->GetIdBitfield() )
				{
					TakeOwnershipOfFrustumHierarchy( ownedBy );
				}
				else if( pFrustum->GetOwnedFrustums() != 0 )
				{
					// Otherwise take ownership of any children
					TakeOwnershipOfFrustumHierarchy( pFrustum->GetOwnedFrustums() );
				}

				// Remove any frustum ownership
				pFrustum->ClearOwnedFrustums();
			}
		}
	}
}

// Release ownership of a frustum hierarchy
void SiegeFrustum::ReleaseOwnedFrustumHierarchy( DWORD frustumBitfield )
{
	// Check all possible combinations for set frustum bit
	for( unsigned int i = 0; i < 32; ++i )
	{
		DWORD frustum	= frustumBitfield & (1 << i);
		if( frustum )
		{
			// Get the frustum
			SiegeFrustum* pFrustum	= gSiegeEngine.GetFrustum( frustum );
			if( pFrustum )
			{
				// Release ownership of the frustum
				pFrustum->SetFrustumOwned( m_idBitfield, false );

				// Clear owned bit
				m_ownedFrustums		&= ~frustum;
			}
			else
			{
				gperror( "Attempted to release an owned frustum that does not exist!" );
			}
		}
	}
}

// Build node space for a node
bool SiegeFrustum::BuildNodeSpace( SiegeNode& curr_node, SiegeNodeDoor* near_door, SiegeNode& far_node, bool bNotify )
{
	// Get the positional info for the current node
	const vector_3& node_center		= curr_node.GetCurrentCenter();
	const matrix_3x3& node_orient	= curr_node.GetCurrentOrientation();

	// Get started with the door connection information
	SiegeNodeDoor* far_door		= far_node.GetDoorByID( near_door->GetNeighbourID() );

#if !GP_RETAIL

	// Make sure that the far door exists...
	if( !far_door )
	{
		gpassert(far_door);

		gpstring dbgstr;
		dbgstr.assignf( "Near:\t%s:%d\n Target:\t%s:%d\nFar:\t%s\n\nFar node is missing the target door",
					curr_node.GetGUID().ToString().c_str(),
					near_door->GetID(),
					near_door->GetNeighbour().ToString().c_str(),
					near_door->GetNeighbourID(),
					far_node.GetGUID().ToString().c_str()
				);

		gperrorbox( dbgstr );

		near_door->SetNeighbour(UNDEFINED_GUID);
		return false;
	}

	// ...and that it does indeed point back to us
	if( !(far_door->GetNeighbour() == curr_node.GetGUID()) )
	{
		gpstring dbgstr;
		dbgstr.assignf( "Near:\t%s:%d\n Target:\t%s:%d\nFar:\t%s:%d\n Target:\t%s:%d\n\nFar node does not point back to near node at all",
					curr_node.GetGUID().ToString().c_str(),
					near_door->GetID(),
					near_door->GetNeighbour().ToString().c_str(),
					near_door->GetNeighbourID(),
					far_node.GetGUID().ToString().c_str(),
					far_door->GetID(),
					far_door->GetNeighbour().ToString().c_str(),
					far_door->GetNeighbourID()
					);

		gperrorbox( dbgstr );

		near_door->SetNeighbour(UNDEFINED_GUID);
		return false;
	}

	// ...and that it points back to the right part of us
	if( far_door->GetNeighbourID() != near_door->GetID() )
	{
		gpstring dbgstr;
		dbgstr.assignf( "Near:\t%s:%d\n Target:\t%s:%d\nFar:\t%s:%d\n Target:\t%s:%d\n\nFar node door does not point back to near node door",
					curr_node.GetGUID().ToString().c_str(),
					near_door->GetID(),
					near_door->GetNeighbour().ToString().c_str(),
					near_door->GetNeighbourID(),
					far_node.GetGUID().ToString().c_str(),
					far_door->GetID(),
					far_door->GetNeighbour().ToString().c_str(),
					far_door->GetNeighbourID()
					);

		gperrorbox( dbgstr );

		near_door->SetNeighbour(UNDEFINED_GUID);
		return false;
	}

#else

	if( !far_door )
	{
		return false;
	}

#endif

	// Aren't we lucky?  Now we can actually do the connection math
	matrix_3x3 inv_far_orient	= far_door->GetOrientation().Transpose_T();
	matrix_3x3 orient			= near_door->GetOrientation();

	orient.SetColumn_0( -orient.GetColumn_0() );
	orient.SetColumn_2( -orient.GetColumn_2() );

	matrix_3x3 door_orient		= orient * inv_far_orient;
	vector_3 door_offset		= near_door->GetCenter() + (door_orient * -far_door->GetCenter());

	// Set the new nodal orientation
	far_node.SetCurrentCenter( node_center + (node_orient * door_offset) );
	far_node.SetCurrentOrientation( node_orient * door_orient );

#if !GP_RETAIL
	far_node.SetSpatialParent( curr_node.GetGUID() );
#endif

	// Tell the node that it now has valid space
	if( !far_node.HasValidSpace() )
	{
		// Mark it for the next render pass
		far_node.SetLoadedLastUpdate( true );
	}

	far_node.SetSpaceFrustum( m_ownerFrustum );

	if( bNotify )
	{
		// Callback notification
		gSiegeEngine.NotifyNodeSpaceChanged( far_node.GetGUID() );
	}

	// Success
	return true;
}

// Load a node
bool SiegeFrustum::LoadNode( const database_guid& nodeGuid )
{
	if( !nodeGuid.IsValid() )
	{
		return false;
	}

	// Request a node load
	LoadOrderId orderId	= gSiegeLoadMgr.LoadById( LOADER_SIEGE_NODE_TYPE, nodeGuid.GetValue() );
	if( IsOrderInstance( orderId ) || (orderId == LOADORDERID_DONE) )
	{
		// Get the node
		SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( nodeGuid );
		SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

		gpassert( !node.IsLoadedByFrustum( m_idBitfield ) );

		// Notify whoever is listening
		gSiegeEngine.NotifyNodeLoad( nodeGuid );

		gpsiegenodef(( "SiegeFrustum %d LOADED SiegeNode %s\n", m_idBitfield, nodeGuid.ToString().c_str() ));
	}

	return true;
}

// Unload a node
void SiegeFrustum::UnloadNode( SiegeNode* pNode )
{
	// Hold the node critical for the duration
	kerneltool::Critical::Lock nodeLock( gSiegeEngine.GetNodeDestroyCritical() );

	gpsiegenodef(( "SiegeFrustum %d UNLOADED SiegeNode %s\n", m_idBitfield, pNode->GetGUID().ToString().c_str() ));

	// Remove node from cache
	gSiegeEngine.NodeCache().RemoveSlotFromCache( pNode->GetGUID() );
}

// Update the current node fades
void SiegeFrustum::UpdateNodeFades()
{
	// Get the current time
	double currentTime	= ::GetGlobalSeconds();

	// Go through the list of node fades and update them
	for( NodeFadeColl::iterator i = m_nodeFadeColl.begin(); i != m_nodeFadeColl.end(); )
	{
		// See if this fade is done and we can remove it
		if( ((*i).m_fadeType == FT_IN) && ((*i).m_alpha == 0xFF) )
		{
			i = m_nodeFadeColl.erase( i );
			continue;
		}
		else if( (((*i).m_fadeType == FT_BLACK) && ((*i).m_alpha == gSiegeEngine.m_NodeFadeBlack)) ||
				 (((*i).m_fadeType == FT_ALPHA) && ((*i).m_alpha == gSiegeEngine.m_NodeFadeAlpha)) )
		{
			++i;
			continue;
		}
		else if( (*i).m_fadeType == FT_INSTANT )
		{
			(*i).m_alpha	= 0;
			++i;
			continue;
		}
		else if( (*i).m_fadeType == FT_IN_INSTANT )
		{
			i = m_nodeFadeColl.erase( i );
			continue;
		}

		// Update this fade
		float interp	= min_t( 1.0f, ((float)(PreciseSubtract(currentTime, (*i).m_startTime)) / gSiegeEngine.m_NodeFadeTime) );

		if( (*i).m_fadeType == FT_IN )
		{
			(*i).m_alpha = (BYTE)FTOL( interp * 255.0f );
		}
		else if( (*i).m_fadeType == FT_ALPHA )
		{
			(*i).m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - gSiegeEngine.m_NodeFadeAlpha) ));
		}
		else if( (*i).m_fadeType == FT_BLACK )
		{
			(*i).m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - gSiegeEngine.m_NodeFadeBlack) ));
		}

		++i;
	}
}

// Update ownership bits based on differences between current set and previous set
void SiegeFrustum::UpdateFrustumOwnership()
{
	if( m_prevOwnedNodes.empty() )
	{
		return;
	}

	// Go through the previous collection
	for( NodeColl::iterator i = m_prevOwnedNodes.begin(); i != m_prevOwnedNodes.end(); ++i )
	{
		// Try to find node in current owned nodes
		for( FrustumNodeColl::iterator o = m_ownedNodeColl.begin(); o != m_ownedNodeColl.end(); ++o )
		{
			if( (*i) == (*o)->GetGUID() )
			{
				break;
			}
		}
		if( o == m_ownedNodeColl.end() )
		{
			// Get the node if possible
			SiegeNode* pNode	= gSiegeEngine.IsNodeValid( (*i) );

			if( pNode )
			{
				// Set ownership release
				gSiegeEngine.NotifyNodeWorldMembershipChanged( *pNode, pNode->GetFrustumOwnedBitfield() | m_idBitfield, false );
			}
			else
			{
				SiegeNode node;
				node.SetGUID( (*i) );

				// Set ownership release with delete
				gSiegeEngine.NotifyNodeWorldMembershipChanged( node, m_idBitfield, true );
			}
		}
	}

	// Go through owned collection
	for( FrustumNodeColl::iterator o = m_ownedNodeColl.begin(); o != m_ownedNodeColl.end(); ++o )
	{
		for( NodeColl::iterator i = m_prevOwnedNodes.begin(); i != m_prevOwnedNodes.end(); ++i )
		{
			if( (*i) == (*o)->GetGUID() )
			{
				break;
			}
		}
		if( i == m_prevOwnedNodes.end() )
		{
			// Notify about enter
			gSiegeEngine.NotifyNodeWorldMembershipChanged( *(*o), (*o)->GetFrustumOwnedBitfield() & ~m_idBitfield, false );
		}
	}
}
