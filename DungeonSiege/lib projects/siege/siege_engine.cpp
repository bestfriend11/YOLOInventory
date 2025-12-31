/*******************************************************************************************
**
**									SiegeEngine
**
**		See .h file for details
**
*******************************************************************************************/

#include "precomp_siege.h"

// Core includes
#include "gpcore.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "space_3.h"
#include "oriented_bounding_box_3.h"
#include "timemgr.h"
#include "gpprofiler.h"
#include "stringtool.h"
#include "stdhelp.h"

// Renderer includes
#include "RapiOwner.h"
#include "RapiPrimitive.h"
#include "RapiImage.h"

// Siege includes
#include "siege_viewfrustum.h"
#include "siege_camera.h"
#include "siege_engine.h"
#include "siege_frustum.h"
#include "siege_mouse_shadow.h"
#include "siege_compass.h"
#include "siege_hotpoint_database.h"
#include "siege_options.h"
#include "siege_pathfinder.h"
#include "siege_loader.h"
#include "siege_persist.h"


using namespace siege;

FUBI_REPLACE_NAME( "SiegeSiegeEngine", SiegeEngine );


// Axis hint conversions for Skrit support
static const char* s_AxisHintStrings[] =
{
	"ah_none"			,
	"ah_xaxis_positive"	,
	"ah_xaxis_negative"	,
	"ah_yaxis_positive"	,
	"ah_yaxis_negative"	,
	"ah_zaxis_positive"	,
	"ah_zaxis_negative"	,
};

// Connection structure for logical construction
struct LeafConnectionInfo
{
	vector_3		wMinBox;
	vector_3		wMaxBox;
	unsigned short	id;
};

FUBI_DECLARE_XFER_TRAITS( SINGLENODEFADE )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_fadeType",		obj.m_fadeType );
		persist.Xfer( "m_alpha",		obj.m_alpha );
		persist.Xfer( "m_startTime",	obj.m_startTime );

		return true;
	}
};

FUBI_DECLARE_XFER_TRAITS( NODEFLAGS )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_bOccludesCamera",	obj.m_bOccludesCamera );
		persist.Xfer( "m_bOccludesChanged", obj.m_bOccludesChanged );
		persist.Xfer( "m_bBoundsCamera",	obj.m_bBoundsCamera );
		persist.Xfer( "m_bBoundsChanged",	obj.m_bBoundsChanged );
		persist.Xfer( "m_bCameraFade",		obj.m_bCameraFade );
		persist.Xfer( "m_bCameraChanged",	obj.m_bCameraChanged );

		return true;
	}
};

FUBI_DECLARE_XFER_TRAITS( NODETEXSTATE )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_textureIndex",			obj.m_textureIndex );
		persist.Xfer( "m_bStateChanged",		obj.m_bStateChanged );
		persist.Xfer( "m_bState",				obj.m_bState );
		persist.Xfer( "m_bSpeedChanged",		obj.m_bSpeedChanged );
		persist.Xfer( "m_bSpeed",				obj.m_bSpeed );
		persist.Xfer( "m_bReplaced",			obj.m_bReplaced );
		persist.Xfer( "m_replacementTexture",	obj.m_replacementTexture );

		return true;
	}
};


COMPILER_ASSERT( ELEMENT_COUNT( s_AxisHintStrings ) == AH_COUNT );

static stringtool::EnumStringConverter s_AxisHintConverter( s_AxisHintStrings, AH_BEGIN, AH_END, AH_NONE );

const char* ToString( eAxisHint val )
	{  return ( s_AxisHintConverter.ToString( val ) );  }
bool FromString( const char* str, eAxisHint& val )
	{  return ( s_AxisHintConverter.FromString( str, rcast <int&> ( val ) ) );  }


SiegeEngine::SiegeEngine()
	: m_nodeDatabase()
	, m_meshDatabase()
	, m_pRenderer( 0 )
	, m_DeltaFrameTime( 0.0 )
	, m_AbsoluteDeltaFrameTime( 0.0f )
	, m_CheapShadowTex( 0 )
	, m_EnvironmentTex( 0 )
	, m_bMultithreaded( true )
	, m_renderFrustum( 0 )
	, m_bUpdatingFrustums( false )
	, m_NodeFadeTime( 0.4f )
	, m_NodeFadeAlpha( 32 )
	, m_NodeFadeBlack( 0 )
	, m_DiscoveryRadius( 20.0f )
	, m_DiscoverySampleLength( 5.0f )
	, m_FrustumUpdateDistance( 2.0f )
	, m_LevelOfDetail( 1.0f )
	, m_DragSelectThreshold( 4.0f )
{
	m_pOptions			= new SiegeOptions;

	m_pMouseShadow		= new SiegeMouseShadow;

	// Make the caches
	m_pNodeCache		= new siege::cache<siege::SiegeNode>( true );
	m_pMeshCache		= new siege::cache<siege::SiegeMesh>( false );

	// Create the walker
	m_pWalker			= new SiegeWalker;

	// Create the light database
	m_pLightDatabase	= new SiegeLightDatabase;

	// Create the camera
	m_pCamera			= new SiegeCamera;

	// Create the compass
	m_pCompass			= new SiegeCompass;

	// Create the hotpoint database
	m_pHotpointDatabase	= new SiegeHotpointDatabase;

	// Create the decal database
	m_pDecalDatabase	= new SiegeDecalDatabase;

	// Create a pathfinder
	m_pPathfinder		= new SiegePathfinder;

	// Create the transition
	m_pTransition		= new SiegeTransition;

	// Create the loader
	m_loadMgr			= new LoadMgr;

	// Get the primary thread ID
	m_primaryThreadId	= ::GetCurrentThreadId();
}

SiegeEngine::~SiegeEngine()
{
	m_loadMgr->CancelAll();
	m_loadMgr->StopLoadingThread( STOP_LOADER_AND_CANCEL );
	Delete( m_loadMgr );

	DestroyAllFrusta();

	Delete( m_pMouseShadow );
	Delete( m_pLightDatabase );
	Delete( m_pWalker );
	Delete( m_pNodeCache );
	Delete( m_pMeshCache );
	Delete( m_pCamera );
	Delete( m_pCompass );
	Delete( m_pHotpointDatabase );
	Delete( m_pDecalDatabase );
	Delete( m_pPathfinder ); 
	Delete( m_pTransition );

	if( m_CheapShadowTex )
	{
		m_pRenderer->DestroyTexture( m_CheapShadowTex );
		m_CheapShadowTex = 0;
	}
	if( m_EnvironmentTex )
	{
		m_pRenderer->DestroyTexture( m_EnvironmentTex );
		m_EnvironmentTex = 0;
	}

	Delete( m_pOptions );
}

FUBI_DECLARE_XFER_TRAITS( SiegeFrustum* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		bool success = true;

		// make sure it is clear on a restore
		if ( persist.IsRestoring() )
		{
			obj = NULL;
		}

		// see if it's there
		unsigned int id = obj ? obj->GetIdBitfield() : 0;
		persist.Xfer( "id", id );
		if ( id != 0 )
		{
			// build it if so
			if ( persist.IsRestoring() )
			{
				obj = gSiegeEngine.CreateNewFrustum( id );
			}

			// xfer bits
			success = obj->Xfer( persist );
		}

		return ( success );
	}
};

bool SiegeEngine::Xfer( FuBi::PersistContext& persist )
{
	// persist transition
	persist.EnterBlock( "m_pTransition" );
	m_pTransition->Xfer( persist );
	persist.LeaveBlock();

	// persist frusta
	persist.XferVector( "m_frustumColl", m_frustumColl );
	persist.Xfer( "m_renderFrustum", m_renderFrustum );

	// persist siege cam
	persist.EnterBlock( "m_pCamera" );
	m_pCamera->Xfer( persist );
	persist.LeaveBlock();

	// persist walker
	persist.EnterBlock( "m_pWalker" );
	m_pWalker->Xfer( persist );
	persist.LeaveBlock();

	// persist fades
	persist.XferMap( "m_singleNodeFadeMap", m_singleNodeFadeMap );
	persist.XferVector( "m_globalNodeFadeColl", m_globalNodeFadeColl );

	// persist node flags
	persist.XferMap( "m_nodeFlagMap", m_nodeFlagMap );

	// persist node texture changes
	persist.XferMap( "m_nodeTexMap", m_nodeTexMap );

	return ( true );
}

bool SiegeEngine::XferNodeTexMap( FuBi::PersistContext& persist )
{
	// persist node texture changes
	persist.XferMap( "m_nodeTexMap", m_nodeTexMap );

	return ( true );
}

bool SiegeEngine::XferNodeFlagMap( FuBi::PersistContext& persist )
{
	// persist node flag changes
	persist.XferMap( "m_nodeFlagMap", m_nodeFlagMap );

	return ( true );
}

void SiegeEngine::Shutdown( bool destroyAllFrusta )
{
	// Destroy all frustums
	if ( destroyAllFrusta )
	{
		DestroyAllFrusta();
	}

	// Clear the transition tables
	m_pTransition->Clear();

	// Clear the camera too
	m_pCamera->Clear();

	// Clear caches and databases
	m_pWalker->Clear();
	m_pNodeCache->Clear();
	m_pMeshCache->Clear();
	m_pLightDatabase->Clear();
	m_pHotpointDatabase->Clear();
	m_singleNodeFadeMap.clear();
	m_globalNodeFadeColl.clear();
	m_nodeFlagMap.clear();
	m_nodeTexMap.clear();
}

bool SiegeEngine::IsLoadingThreadRunning() const
{
	return ( m_loadMgr->IsLoadingThreadRunning() );
}

void SiegeEngine::StartLoadingThread()
{
	m_loadMgr->StartLoadingThread();
}

void SiegeEngine::StopLoadingThread( eStopLoaderType stopType )
{
	m_loadMgr->StopLoadingThread( stopType );
}

// Returns whether or not the function was called from the main thread
bool SiegeEngine::IsPrimaryThread()
{
	return( m_primaryThreadId == ::GetCurrentThreadId() );
}

void SiegeEngine::AttachRenderer( Rapi& pRapi )
{
	// Set the renderer
	m_pRenderer	= &pRapi;

	// Create the blobby blob for blobby blob shadow rendering
	RapiMemImage *image = new RapiMemImage( pRapi.GetShadowTexSize(), pRapi.GetShadowTexSize(), 0 );
	DWORD *pPixels	= image->GetBits();

	int radius	= (pRapi.GetShadowTexSize() / 4);
	for( int r = 0; r < radius; r++ )
	{
		float brightness = (float)(1.0f - (float)r/(float)radius);
		for( float a = 0; a<2*RealPi; a+= 2*RealPi/360 )
		{
			float anglesin, anglecos;
			SINCOSF( a, anglesin, anglecos );
			int x = FTOL( r*anglecos )+(pRapi.GetShadowTexSize()/2);
			int y = FTOL( r*anglesin )+(pRapi.GetShadowTexSize()/2);

			pPixels[ (y*pRapi.GetShadowTexSize())+x ] = FTOL( 255.0f * brightness ) << 24;
		}
	}
	m_CheapShadowTex	= pRapi.CreateAlgorithmicTexture( "siege blobby shadow", image, true, 0, TEX_LOCKED, true, true );

	// Create the env map
	m_EnvironmentTex	= pRapi.CreateTexture( "art\\bitmaps\\envmaps\\b_em_sphere.%img%", true, 1, TEX_LOCKED );
	pRapi.SetEnvironmentTexture( m_EnvironmentTex );
}

bool SiegeEngine::BeginRender()
{
	// Begin rendering
	m_pRenderer->SetGameElapsedTime( m_DeltaFrameTime );
	if ( !m_pRenderer->Begin3D() )
	{
		return false;
	}

	// Set wireframe mode
	if( m_pOptions->IsWireframeEnabled() )
	{
		m_pRenderer->SetWireframe( true );
	}
	else
	{
		m_pRenderer->SetWireframe( false );
	}

	m_NemaCostRendered				= 0;
	
	m_TotalTrisRendered				= 0;
	m_TotalVertsRendered			= 0;

	m_SiegeTrisRendered				= 0;
	m_NemaTrisRendered				= 0;
	m_DecalTrisRendered				= 0;

	m_SiegeVertsRendered			= 0;
	m_NemaVertsRendered				= 0;

	m_SelectedSiegeTrisRendered		= 0;
	m_SelectedNemaTrisRendered		= 0;

	m_SelectedSiegeVertsRendered	= 0;
	m_SelectedNemaVertsRendered		= 0;

	m_CulledNodeCount				= 0;
	m_RenderedNodeCount				= 0;

	m_SelectedSiegeNodeName.clear();

	return true;
}

bool SiegeEngine::EndRender( bool bCopyToPrimary, bool bCopyImmediately )
{
	// End the scene
	return m_pRenderer->End3D( bCopyToPrimary, bCopyImmediately );
}

// Calculates a difference vector relative to the origin
vector_3 SiegeEngine::GetDifferenceVector( const SiegePos& pos_orig, const SiegePos& pos_dest )
{
	// If we're in the same node, just get the difference
	if( pos_orig.node == pos_dest.node )
	{
		return (pos_dest.pos - pos_orig.pos);
	}

	gpassert( IsNodeValid( pos_orig.node ) );
	gpassert( IsNodeValid( pos_dest.node ) );
	
	// Otherwise do the internode transform to get the new difference
	vector_3 relative_dest;

	if( pos_dest.node == NodeWalker().TargetNodeGUID() )
	{
		relative_dest	= pos_dest.pos;
	}
	else
	{
		const SiegeNodeHandle dhandle(NodeCache().UseObject(pos_dest.node));
		const SiegeNode& dnode = dhandle.RequestObject( NodeCache() );
		relative_dest	= dnode.NodeToWorldSpace( pos_dest.pos );
	}

	if( pos_orig.node == NodeWalker().TargetNodeGUID() )
	{
		return (relative_dest - pos_orig.pos);
	}
	else
	{
		const SiegeNodeHandle ohandle(NodeCache().UseObject(pos_orig.node));
		const SiegeNode& onode = ohandle.RequestObject( NodeCache() );
		return (onode.WorldToNodeSpace( relative_dest ) - pos_orig.pos);
	}
}

matrix_3x3 SiegeEngine::GetDifferenceOrientation( const matrix_3x3& orig_orientation, const database_guid& orig_node,
												  const matrix_3x3& dest_orientation, const database_guid& dest_node )
{
	// Make sure that both nodes are valid
	if( (orig_node == UNDEFINED_GUID) || (dest_node == UNDEFINED_GUID) )
	{
		return matrix_3x3::IDENTITY;
	}

	// If we're in the same node, just get the difference
	if( orig_node == dest_node )
	{
		return( dest_orientation * Transpose( orig_orientation ) );
	}

	// Otherwise do the internode transform to get the new difference
	const SiegeNodeHandle dhandle( NodeCache().UseObject( dest_node ) );
	const SiegeNode& dnode = dhandle.RequestObject( NodeCache() );

	const SiegeNodeHandle ohandle( NodeCache().UseObject( orig_node ) );
	const SiegeNode& onode = ohandle.RequestObject( NodeCache() );

	return( (onode.GetTransposeOrientation() * (dnode.GetCurrentOrientation() * dest_orientation)) * Transpose( orig_orientation ) );
}

bool SiegeEngine::GetDifferenceVectorOrientation( const database_guid& orig_node, const vector_3& orig_pos, const Quat& orig_rot,
												  const database_guid& dest_node, const vector_3& dest_pos,	const Quat& dest_rot,
													vector_3& diff_pos,	Quat& diff_rot )
{
	if( (orig_node == UNDEFINED_GUID) || (dest_node == UNDEFINED_GUID) )
	{
		diff_pos = vector_3::ZERO;
		diff_rot = Quat::IDENTITY;
		return false;
	}

	if( orig_node == dest_node )
	{

		Quat orotinv = orig_rot.Inverse();

		orotinv.RotateVector(diff_pos, dest_pos-orig_pos);
		diff_rot = orotinv*dest_rot;

	}
	else
	{
		// $ The 'not in the same node' branch has not been thoroughly tested
		// $ Need to set up some good test cases and make sure I have the math right
		// $ -- biddle

		const SiegeNodeHandle dhandle(NodeCache().UseObject(dest_node));
		const SiegeNode& dnode = dhandle.RequestObject( NodeCache() );
		const SiegeNodeHandle ohandle(NodeCache().UseObject(orig_node));
		const SiegeNode& onode = ohandle.RequestObject( NodeCache() );

		Quat orot(Quat(onode.GetCurrentOrientation()) * orig_rot);
		Quat drot(Quat(dnode.GetCurrentOrientation()) * dest_rot);

		vector_3 opos(onode.NodeToWorldSpace( orig_pos ) );
		vector_3 dpos(dnode.NodeToWorldSpace( dest_pos ) );

		Quat orotinv = orot.Inverse();

		orotinv.RotateVector( diff_pos, dpos - opos );
		diff_rot = orotinv*drot;
	}

	return true;
}

// Get this node's orientation
void SiegeEngine::GetNodeOrientation( const siege::database_guid &node, matrix_3x3 &orient ) const
{
	const SiegeNodeHandle handle	= m_pNodeCache->UseObject( node );
	const SiegeNode& locate_node	= handle.RequestObject( *m_pNodeCache );

	orient							= locate_node.GetCurrentOrientation();
}

// Updates the position and guid if the passed location is not currently in the given node
bool SiegeEngine::UpdateNodePosition( SiegePos& siegepos )
{
	if( siegepos.node == UNDEFINED_GUID )
	{
		gpassertm( 0, "Cannot UpdateNodePosition() of an invalid GUID!" );
		return false;
	}

	// If this position is still within the node, don't change anything
	SiegeNodeHandle handle(NodeCache().UseObject(siegepos.node));
	SiegeNode& node = handle.RequestObject( NodeCache() );

	// Calculate a difference
	float pos_diff		= FLOAT_MAX;
	if( node.IsPointInNode( siegepos.pos ) )
	{
		pos_diff = Length2( siegepos.pos - node.GetCurrentCenter() );
	}

	// Otherwise, we need to check out our neighbors
	vector_3 world_pos	= node.NodeToWorldSpace( siegepos.pos );

	const SiegeNodeDoorList& doors	= node.GetDoors();
	for( SiegeNodeDoorConstIter i = doors.begin(); i != doors.end(); ++i )
	{
		SiegeNode* pnNode	= IsNodeValid( (*i)->GetNeighbour() );
		if( !pnNode )
		{
			continue;
		}

		if( pnNode->HasValidSpace() )
		{
			vector_3 node_pos = pnNode->WorldToNodeSpace( world_pos );
			if( pnNode->IsPointInNode( node_pos ) )
			{
				if( Length2( node_pos - pnNode->GetCurrentCenter() ) < pos_diff )
				{
					siegepos.pos	= node_pos;
					siegepos.node	= pnNode->GetGUID();
					return true;
				}
			}
		}
	}

	return false;
}

// Get the height and logical flags of liquid at the specified location
bool SiegeEngine::GetLiquidInfo( const SiegePos& pos, float& liquidHeight, eLogicalNodeFlags& liquidFlags )
{
	gpassert( IsNodeValid( pos.node ) );

	const SiegeNodeHandle handle	= NodeCache().UseObject( pos.node );
	const SiegeNode& locate_node	= handle.RequestObject( NodeCache() );

	float ray_min					= FLOAT_MAX;
	vector_3 node_ray_origin		= vector_3( pos.pos.x, locate_node.GetMinimumBounds().y - 0.1f, pos.pos.z );
	liquidFlags						= LF_IS_WATER;

	bool bHit = locate_node.HitTestFlags( node_ray_origin,
										  vector_3::UP,
										  ray_min, liquidFlags );

	if( bHit )
	{
		liquidHeight	= (node_ray_origin + (vector_3::UP * ray_min)).y - pos.pos.y;
		return true;
	}
	else
	{
		liquidHeight	= 0.0f;
		liquidFlags		= LF_NONE;
		return false;
	}
}

bool SiegeEngine::AdjustPointToTerrain( SiegePos & pos, eLogicalNodeFlags flags, DWORD recurseLevel, vector_3* pNormal )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	// Get the start node handle and push it onto our list
	SiegeNode* pStartNode			= IsNodeValid( pos.node );
	if( !pStartNode )
	{
		gperrorf(( "Invalid start node for AdjustPointToTerrain! [%f,%f,%f,0x%08x]",
			pos.pos.x,
			pos.pos.y,
			pos.pos.z,
			pos.node.GetValue()
			));
		return false;
	}

	float ray_min					= FLOAT_MAX;
	vector_3 face_normal			( DoNotInitialize );
	vector_3 node_ray_origin		= vector_3( pos.pos.x, pStartNode->GetMinimumBounds().y - 0.1f, pos.pos.z );

	if( pStartNode->HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, flags ) )
	{
		// Should make sure the node GUID is set correctly
		pos.pos.y					= node_ray_origin.y + ray_min;
		return true;
	}

	gpassert( m_nodeStack.empty() );

	// Initialize variables needed for the stack run
	unsigned int currentRecurse		= 0;
	unsigned int visitCount			= RandomDword();
	unsigned int i					= 0;
	unsigned int size				= 0;
	vector_3 worldOrigin			= pStartNode->NodeToWorldSpace( pos.pos );
	float currentYDiff				= FLOAT_MAX;
	float originalY;
	SiegePos currentPos;

	pStartNode->SetAPVisited( visitCount );

	// Go through the neighbors and put them on the new list
	const SiegeNodeDoorList& doors	= pStartNode->GetDoors();
	for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
	{
		SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
		if( !pNode )
		{
			continue;
		}

		pNode->SetAPVisited( visitCount );

		// Push the node onto the list
		m_nodeStack.push_back( pNode );
	}

	do
	{
		// Get the current size of the stack
		size = m_nodeStack.size();

		// Go through the nodes in the current list
		for( ; i < size; ++i )
		{
			// Get the current node
			const siege::SiegeNode& node	= *m_nodeStack[i];

			node_ray_origin					= node.WorldToNodeSpace( worldOrigin );
			originalY						= node_ray_origin.y;
			node_ray_origin.y				= node.GetMinimumBounds().y - 0.1f;

			if( node.HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, flags ) )
			{
				float ydiff	= FABSF( originalY - (node_ray_origin.y + ray_min) );
				if( ydiff < currentYDiff )
				{
					currentPos.pos		= node_ray_origin;
					currentPos.pos.y	+= ray_min;
					currentPos.node		= node.GetGUID();
					currentYDiff		= ydiff;
					if( pNormal )
					{
						*pNormal		= face_normal;
					}
				}
			}

			// Only put on neighbors if we are below the recursion limit
			if( currentRecurse < recurseLevel )
			{
				// Go through the neighbors and put them on the new list
				const SiegeNodeDoorList& ndoors = node.GetDoors();
				for( d = ndoors.begin(); d != ndoors.end(); ++d )
				{
					SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
					if( !pNode || pNode->GetAPVisited() == visitCount )
					{
						continue;
					}

					pNode->SetAPVisited( visitCount );

					// Push the node onto the list
					m_nodeStack.push_back( pNode );
				}
			}
		}

		// Check for completion
		if( currentYDiff != FLOAT_MAX )
		{
			// Set the new position
			pos = currentPos;

			// Clear the stack
			m_nodeStack.clear();
			return true;
		}

		// Set our current list to the new list and increment our recursion counter
		currentRecurse++;
	}
	while( size < m_nodeStack.size() );

	// Clear the stack
	m_nodeStack.clear();

	return false;
}

bool SiegeEngine::AdjustPointToTerrainWithIndex( SiegePos& pos, short& index1, short& index2, short& index3, eLogicalNodeFlags flags, DWORD recurseLevel )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	// Get the start node handle and push it onto our list
	SiegeNode* pStartNode			= IsNodeValid( pos.node );
	if( !pStartNode )
	{
		gperrorf(( "Invalid start node for AdjustPointToTerrainWithIndex! [%f,%f,%f,0x%08x]",
			pos.pos.x,
			pos.pos.y,
			pos.pos.z,
			pos.node.GetValue()
			));
		return false;
	}

	float ray_min					= FLOAT_MAX;
	TriNorm triangle;
	vector_3 node_ray_origin		= vector_3( pos.pos.x, pStartNode->GetMinimumBounds().y - 0.1f, pos.pos.z );

	if( pStartNode->HitTestTri( node_ray_origin, vector_3::UP, ray_min, triangle, flags ) )
	{
		// Should make sure the node GUID is set correctly
		pos.pos.y					= node_ray_origin.y + ray_min;
		GetRenderGeometryIndicesFromTriangle( triangle, pos.node, index1, index2, index3 );
		return true;
	}

	gpassert( m_nodeStack.empty() );

	// Initialize variables needed for the stack run
	unsigned int currentRecurse		= 0;
	unsigned int visitCount			= RandomDword();
	unsigned int i					= 0;
	unsigned int size				= 0;
	vector_3 worldOrigin			= pStartNode->NodeToWorldSpace( pos.pos );
	float currentYDiff				= FLOAT_MAX;
	float originalY;
	TriNorm currentTri;
	SiegePos currentPos;

	pStartNode->SetAPVisited( visitCount );

	// Go through the neighbors and put them on the new list
	const SiegeNodeDoorList& doors	= pStartNode->GetDoors();
	for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
	{
		SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
		if( !pNode )
		{
			continue;
		}

		pNode->SetAPVisited( visitCount );

		// Push the node onto the list
		m_nodeStack.push_back( pNode );
	}

	do
	{
		// Get the current size of the stack
		size = m_nodeStack.size();

		// Go through the nodes in the current list
		for( ; i < size; ++i )
		{
			// Get the current node
			const siege::SiegeNode& node	= *m_nodeStack[i];

			node_ray_origin					= node.WorldToNodeSpace( worldOrigin );
			originalY						= node_ray_origin.y;
			node_ray_origin.y				= node.GetMinimumBounds().y - 0.1f;

			if( node.HitTestTri( node_ray_origin, vector_3::UP, ray_min, triangle, flags ) )
			{
				float ydiff	= FABSF( originalY - (node_ray_origin.y + ray_min) );
				if( ydiff < currentYDiff )
				{
					currentPos.pos		= node_ray_origin;
					currentPos.pos.y	+= ray_min;
					currentPos.node		= node.GetGUID();
					currentTri			= triangle;
					currentYDiff		= ydiff;
				}
			}

			// Only put on neighbors if we are below the recursion limit
			if( currentRecurse < recurseLevel )
			{
				// Go through the neighbors and put them on the new list
				const SiegeNodeDoorList& ndoors = node.GetDoors();
				for( d = ndoors.begin(); d != ndoors.end(); ++d )
				{
					SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
					if( !pNode || pNode->GetAPVisited() == visitCount )
					{
						continue;
					}

					pNode->SetAPVisited( visitCount );

					// Push the node onto the list
					m_nodeStack.push_back( pNode );
				}
			}
		}

		// Check for completion
		if( currentYDiff != FLOAT_MAX )
		{
			// Set the new position
			pos = currentPos;
			GetRenderGeometryIndicesFromTriangle( currentTri, pos.node, index1, index2, index3 );

			// Clear the stack
			m_nodeStack.clear();
			return true;
		}

		// Set our current list to the new list and increment our recursion counter
		currentRecurse++;
	}
	while( size < m_nodeStack.size() );

	// Clear the stack
	m_nodeStack.clear();

	return false;
}

// Performs priority based adjustment
bool SiegeEngine::AdjustPointToTerrainPriority( SiegeNode* pStartNode, SiegePos& pos, eLogicalNodeFlags flags, eLogicalNodeFlags secondary_flags, DWORD recurseLevel, bool bEarlyOut )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( !pStartNode )
	{
		gperrorf(( "NULL node passed to AdjustPointToTerrainPriority! [%f,%f,%f,0x%08x]",
			pos.pos.x,
			pos.pos.y,
			pos.pos.z,
			pos.node.GetValue()
			));
		return false;
	}

	float ray_min					= FLOAT_MAX;
	vector_3 face_normal			( DoNotInitialize );
	vector_3 node_ray_origin		= vector_3( pos.pos.x, pStartNode->GetMinimumBounds().y - 0.1f, pos.pos.z );

	if( pStartNode->HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, flags ) )
	{
		// Should make sure the node GUID is set correctly
		pos.pos.y					= node_ray_origin.y + ray_min;
		return true;
	}

	gpassert( m_nodeStack.empty() );

	// Initialize variables needed for the stack run
	unsigned int currentRecurse		= 0;
	unsigned int visitCount			= RandomDword();
	unsigned int i					= 0;
	unsigned int size				= 0;
	vector_3 worldOrigin			= pStartNode->NodeToWorldSpace( pos.pos );
	float currentYDiff				= FLOAT_MAX;
	float originalY;
	SiegePos currentPos;

	// Secondary checks
	float secondaryYDiff			= FLOAT_MAX;
	SiegePos secondaryPos;
	if( pStartNode->HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, secondary_flags ) )
	{
		secondaryPos.pos	= node_ray_origin;
		secondaryPos.pos.y	+= ray_min;
		secondaryPos.node	= pos.node;
		secondaryYDiff		= FABSF( pos.pos.y - (node_ray_origin.y + ray_min) );
	}

	pStartNode->SetAPVisited( visitCount );

	// Go through the neighbors and put them on the new list
	const SiegeNodeDoorList& doors	= pStartNode->GetDoors();
	for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
	{
		SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
		if( !pNode )
		{
			continue;
		}

		pNode->SetAPVisited( visitCount );

		// Push the node onto the list
		m_nodeStack.push_back( pNode );
	}

	do
	{
		// Get the current size of the stack
		size = m_nodeStack.size();

		// Go through the nodes in the current list
		for( ; i < size; ++i )
		{
			// Get the current node
			const siege::SiegeNode& node	= *m_nodeStack[i];

			node_ray_origin					= node.WorldToNodeSpace( worldOrigin );
			originalY						= node_ray_origin.y;
			node_ray_origin.y				= node.GetMinimumBounds().y - 0.1f;

			if( node.HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, flags ) )
			{
				float ydiff	= FABSF( originalY - (node_ray_origin.y + ray_min) );
				if( ydiff < currentYDiff )
				{
					currentPos.pos		= node_ray_origin;
					currentPos.pos.y	+= ray_min;
					currentPos.node		= node.GetGUID();
					currentYDiff		= ydiff;
				}
			}
			else if( node.HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal, secondary_flags ) )
			{
				float ydiff	= FABSF( originalY - (node_ray_origin.y + ray_min) );
				if( ydiff < secondaryYDiff )
				{
					secondaryPos.pos	= node_ray_origin;
					secondaryPos.pos.y	+= ray_min;
					secondaryPos.node	= node.GetGUID();
					secondaryYDiff		= ydiff;
				}
			}

			// Only put on neighbors if we are below the recursion limit
			if( currentRecurse < recurseLevel )
			{
				// Go through the neighbors and put them on the new list
				const SiegeNodeDoorList& ndoors = node.GetDoors();
				for( d = ndoors.begin(); d != ndoors.end(); ++d )
				{
					SiegeNode* pNode	= IsNodeInAnyFrustum( (*d)->GetNeighbour() );
					if( !pNode || pNode->GetAPVisited() == visitCount )
					{
						continue;
					}

					pNode->SetAPVisited( visitCount );

					// Push the node onto the list
					m_nodeStack.push_back( pNode );
				}
			}
		}

		if( bEarlyOut )
		{
			// Check for completion
			if( currentYDiff != FLOAT_MAX )
			{
				// Set the new position
				pos = currentPos;

				// Clear the stack
				m_nodeStack.clear();
				return true;
			}
			if( secondaryYDiff != FLOAT_MAX )
			{
				// Set the new position
				pos = secondaryPos;

				// Clear the stack
				m_nodeStack.clear();
				return true;
			}
		}

		// Set our current list to the new list and increment our recursion counter
		currentRecurse++;
	}
	while( size < m_nodeStack.size() );

	if( !bEarlyOut )
	{
		// Check for completion
		if( currentYDiff != FLOAT_MAX )
		{
			// Set the new position
			pos = currentPos;

			// Clear the stack
			m_nodeStack.clear();
			return true;
		}
		if( secondaryYDiff != FLOAT_MAX )
		{
			// Set the new position
			pos = secondaryPos;

			// Clear the stack
			m_nodeStack.clear();
			return true;
		}
	}

	// Clear the stack
	m_nodeStack.clear();
	return false;
}

void SiegeEngine::GetRenderGeometryIndicesFromTriangle( TriNorm& tri, database_guid& node_guid, short& index1, short& index2, short& index3 )
{
	gpassert( IsNodeValid( node_guid ) );
	SiegeNodeHandle handle	= m_pNodeCache->UseObject( node_guid );
	SiegeNode& node			= handle.RequestObject( *m_pNodeCache );

	// Get the mesh
	SiegeMesh& mesh			= node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
	TexStageList& sList		= mesh.GetRenderObject()->GetStageList();
	sVertex* pVerts			= mesh.GetVertices();

	for( TexStageList::iterator i = sList.begin(); i != sList.end(); ++i )
	{
		for( unsigned int v = 0; v < (*i).numVIndices; v += 3 )
		{
			unsigned int vindex1	= (*i).pVIndices[v]   + (*i).startIndex;
			unsigned int vindex2	= (*i).pVIndices[v+1] + (*i).startIndex;
			unsigned int vindex3	= (*i).pVIndices[v+2] + (*i).startIndex;

			unsigned int retIndex1, retIndex2, retIndex3;
			if( GetTriangleIndexMatch( tri.m_Vertices[0], retIndex1, pVerts, vindex1, vindex2, vindex3 ) &&
				GetTriangleIndexMatch( tri.m_Vertices[1], retIndex2, pVerts, vindex1, vindex2, vindex3 ) &&
				GetTriangleIndexMatch( tri.m_Vertices[2], retIndex3, pVerts, vindex1, vindex2, vindex3 ) )
			{
				// After much pain and suffering, we have found a match
				index1	= (short)(retIndex1 == vindex1 ? vindex1 : (retIndex1 == vindex2 ? vindex2 : (retIndex1 == vindex3 ? vindex3 : 0 )));
				index2	= (short)(retIndex2 == vindex1 ? vindex1 : (retIndex2 == vindex2 ? vindex2 : (retIndex2 == vindex3 ? vindex3 : 0 )));
				index3	= (short)(retIndex3 == vindex1 ? vindex1 : (retIndex3 == vindex2 ? vindex2 : (retIndex3 == vindex3 ? vindex3 : 0 )));
				return;
			}
		}
	}
}

// See if the given vertex is among the three given
bool SiegeEngine::GetTriangleIndexMatch( const vector_3& tri, unsigned int& retIndex, sVertex* pVerts,
										 unsigned int index1, unsigned int index2, unsigned int index3 )
{
	if( tri.IsExactlyEqual( *((vector_3*)(&pVerts[index1])) ) )
	{
		retIndex	= index1;
		return true;
	}
	if( tri.IsExactlyEqual( *((vector_3*)(&pVerts[index2])) ) )
	{
		retIndex	= index2;
		return true;
	}
	if( tri.IsExactlyEqual( *((vector_3*)(&pVerts[index3])) ) )
	{
		retIndex	= index3;
		return true;
	}

	return false;
}

// Does the ray between the origin and the destination intersect any active terrain
bool SiegeEngine::RayIntersectsRenderedTerrainAndObjects( const SiegePos& origin, const SiegePos& destination,
														  eCollectFlags obj_flags, bool bCameraBlock )
{
	// Generate the ray we will need in world space
	vector_3 world_origin		= origin.WorldPos();
	vector_3 world_direction	= destination.WorldPos() - world_origin;
	float world_len_squared		= world_direction.Length2();

	float ray_t					= FLOAT_MIN;
	vector_3 face_normal( DoNotInitialize );
	vector_3 intersect_coord( DoNotInitialize );

	// Get the active list
	SiegeFrustum* pActiveFrustum	= GetFrustum( GetRenderFrustum() );
	if( pActiveFrustum )
	{
		const FrustumNodeColl& nodeColl	= pActiveFrustum->GetFrustumNodeColl();
		for( FrustumNodeColl::const_iterator i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			const SiegeNode& node	= *(*i);

			if( node.GetVisibleThisFrame() && (bCameraBlock ? node.GetOccludesCamera() : true) )
			{
				vector_3 node_origin	= node.WorldToNodeSpace( world_origin );
				vector_3 node_direction	= node.GetTransposeOrientation() * world_direction;

				if( node.HitTest( node_origin, node_direction, ray_t, face_normal, LF_IS_ANY ) )
				{
					if( IsPositive( ray_t ) && ray_t <= 1.0f )
					{
						return true;
					}
				}

				WorldObjectColl oColl;
				CollectObjects( oColl, node, obj_flags, RF_BOUNDS );

				for( WorldObjectColl::const_iterator o = oColl.begin(); o != oColl.end(); ++o )
				{
					// Get the oriented box
					oriented_bounding_box_3 oBox( (*o).BoundCenter, (*o).BoundHalfDiag, (*o).BoundOrient );

					if( oBox.RayIntersectsOrientedBox( world_origin, world_direction, &intersect_coord ) )
					{
						if( Length2( intersect_coord - world_origin ) <= world_len_squared )
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool SiegeEngine::CheckForLocalTerrainObstruction( const SiegePos& start, const SiegePos& end )
{
	GPPROFILERSAMPLE( "SiegeEngine::CheckForLocalTerrainObstruction", SP_MISC_NOGROUP );

	// Make sure the end point is over some good ground
	SiegePos floorEnd		= end;

	// If we hit the floor and it was less than 1 meter from our given position
	if( AdjustPointToTerrain( floorEnd ) && GetDifferenceVector( end, floorEnd ).Length2() < 1.0f )
	{
		float ray_t				= FLOAT_MAX;
		vector_3 face_normal( DoNotInitialize );

		// Get the start node
		SiegeNodeHandle handle	= m_pNodeCache->UseObject( start.node );
		SiegeNode& node			= handle.RequestObject( *m_pNodeCache );

		// Get the difference
		vector_3 diff			= GetDifferenceVector( start, floorEnd );

		// If we hit a wall and we are less than 1 meter away from it
		if( node.HitTest( start.pos, diff, ray_t, face_normal, LF_IS_WALL ) )
		{
			if( (ray_t < 1.0f) && (Length2( diff * ray_t ) < 1.0f) )
			{
				return true;
			}
		}

		if( start.node == floorEnd.node )
		{
			return false;
		}
		else
		{
			// See if we hit a wall in either node
			SiegeNodeHandle ehandle	= m_pNodeCache->UseObject( floorEnd.node );
			SiegeNode& enode		= ehandle.RequestObject( *m_pNodeCache );

			// Get the difference
			diff					= GetDifferenceVector( floorEnd, start );

			// If we hit a wall and we are less than 1 meter away from it
			if( enode.HitTest( floorEnd.pos, diff, ray_t, face_normal, LF_IS_WALL ) )
			{
				if( (ray_t < 1.0f) && (Length2( diff * ray_t ) < 1.0f) )
				{
					return true;
				}
			}

			return false;
		}
	}

	return true;
}

bool SiegeEngine::CheckForGlobalTerrainObstruction( const SiegePos& start, const SiegePos& end )
{
	GPPROFILERSAMPLE( "SiegeEngine::CheckForGlobalTerrainObstruction", SP_MISC_NOGROUP );

	CHECK_SIEGE_PRIMARY_THREAD_ONLY;
	gpassert( m_nodeStack.empty() );

	// Get the start node
	SiegeNode* pSNode		= IsNodeValid( start.node );
	if( !pSNode )
	{
		gperror( "Bad start position passed to CheckForGlobalTerrainObstruction()" );
		return false;
	}
	vector_3 worldStart		= pSNode->NodeToWorldSpace( start.pos );

	// Get the end node
	SiegeNode* pENode		= IsNodeValid( end.node );
	if( !pENode )
	{
		gperror( "Bad end position passed to CheckForGlobalTerrainObstruction()" );
		return false;
	}
	vector_3 worldEnd		= pENode->NodeToWorldSpace( end.pos );

	// Calculate world direction
	vector_3 worldDir		= worldEnd - worldStart;
	if( worldDir.IsZero() )
	{
		return false;
	}

	// Build our testing sphere
	vector_3 worldCenter	= worldStart + (worldDir * 0.5f);
	float sphereRadius		= Length( worldCenter - worldStart ) + 1.0f;

	float ray_t				= FLOAT_MAX;
	vector_3 face_normal( DoNotInitialize );

	// Start building a node queue
	unsigned int visitCount	= RandomDword();

	pSNode->SetAPVisited( visitCount );
	pENode->SetAPVisited( visitCount );
	m_nodeStack.push_back( pSNode );
	m_nodeStack.push_back( pENode );

	while( !m_nodeStack.empty() )
	{
		// Get the current node
		siege::SiegeNode& node	= *m_nodeStack.pop_back_t();

		// Make sure this node is inside our sphere
		if( node.HasValidSpace() &&
			SphereIntersectsBox( worldCenter, sphereRadius, node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds() ) )
		{
			// Check for obstruction
			if( node.HitTest( node.WorldToNodeSpace( worldStart ), node.GetTransposeOrientation() * worldDir, ray_t, face_normal, LF_IS_ANY ) )
			{
				if( ray_t < 1.0f )
				{
					m_nodeStack.clear();
					return true;
				}
			}

			// Go through the neighbors and put them on the new list
			const SiegeNodeDoorList& doors	= node.GetDoors();
			for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
			{
				SiegeNode* pNewNode	= IsNodeValid( (*d)->GetNeighbour() );
				if( !pNewNode )
				{
					continue;
				}

				if( !pNewNode->IsOwnedByAnyFrustum() || (pNewNode->GetAPVisited() == visitCount) )
				{
					continue;
				}

				pNewNode->SetAPVisited( visitCount );

				// Push the node onto the list
				m_nodeStack.push_back( pNewNode );
			}
		}
	}

	return false;
}

bool SiegeEngine::HitTestGlobalTerrain( const SiegePos& start, const SiegePos& end,
										SiegePos& intersect_pos, vector_3& intersect_norm, 
										const eLogicalNodeFlags flags )
{
	GPPROFILERSAMPLE( "SiegeEngine::HitTestGlobalTerrain", SP_MISC_NOGROUP );

	CHECK_SIEGE_PRIMARY_THREAD_ONLY;
	gpassert( m_nodeStack.empty() );

	// Get the start node
	SiegeNode* pSNode		= IsNodeValid( start.node );
	if( !pSNode )
	{
		gperror( "Bad start position passed to HitTestGlobalTerrain()" );
		return false;
	}
	vector_3 worldStart		= pSNode->NodeToWorldSpace( start.pos );

	// Get the end node
	SiegeNode* pENode		= IsNodeValid( end.node );
	if( !pENode )
	{
		gperror( "Bad end position passed to HitTestGlobalTerrain()" );
		return false;
	}
	vector_3 worldEnd		= pENode->NodeToWorldSpace( end.pos );

	// Calculate world direction
	vector_3 worldDir		= worldEnd - worldStart;
	if( worldDir.IsZero() )
	{
		return false;
	}

	// Build our testing sphere
	vector_3 worldCenter	= worldStart + (worldDir * 0.5f);
	float sphereRadius		= Length( worldCenter - worldStart ) + 1.0f;

	float ray_t				= FLOAT_MAX;
	float intersect_dist	= FLOAT_MAX;
	vector_3 face_normal( DoNotInitialize );

	// Start building a node queue
	unsigned int visitCount	= RandomDword();

	pSNode->SetAPVisited( visitCount );
	pENode->SetAPVisited( visitCount );
	m_nodeStack.push_back( pSNode );
	m_nodeStack.push_back( pENode );

	while( !m_nodeStack.empty() )
	{
		// Get the current node
		siege::SiegeNode& node	= *m_nodeStack.pop_back_t();

		// Make sure this node is inside our sphere
		if( node.HasValidSpace() &&
			SphereIntersectsBox( worldCenter, sphereRadius, node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds() ) )
		{
			// Check for obstruction
			vector_3 node_space_start = node.WorldToNodeSpace( worldStart );
			vector_3 node_space_ray   = node.GetTransposeOrientation() * worldDir;

			if( node.HitTest( node_space_start, node_space_ray, ray_t, face_normal, flags ) )
			{
				if( !IsNegative( ray_t ) && !IsNegative( 1.0f - ray_t ) && (ray_t < intersect_dist) )
				{
					intersect_pos.pos	= node_space_start + (ray_t * node_space_ray);
					intersect_pos.node	= node.GetGUID();
					intersect_norm		= face_normal;
					intersect_dist		= ray_t;
				}
			}

			// Go through the neighbors and put them on the new list
			const SiegeNodeDoorList& doors	= node.GetDoors();
			for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
			{
				SiegeNode* pNewNode	= IsNodeValid( (*d)->GetNeighbour() );
				if( !pNewNode )
				{
					continue;
				}

				if( !pNewNode->IsOwnedByAnyFrustum() || (pNewNode->GetAPVisited() == visitCount) )
				{
					continue;
				}

				pNewNode->SetAPVisited( visitCount );

				// Push the node onto the list
				m_nodeStack.push_back( pNewNode );
			}
		}
	}

	return (intersect_dist != FLOAT_MAX);
}

// Get a SiegeLogicalNode based off of the given position
SiegeLogicalNode* SiegeEngine::GetLogicalNodeAtPosition( const SiegePos& pos, const eLogicalNodeFlags flags )
{
	gpassert( flags );	// must specify floor types to search

	// Get this SiegeNode
	SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( pos.node );
	SiegeNode& node			= handle.RequestObject( *m_pNodeCache );

	// Get the logical nodes from this SiegeNode
	SiegeLogicalNode** ppLogicalNodes	= node.GetLogicalNodes();
	SiegeLogicalNode* pCurrentMatch		= NULL;
	SiegeLogicalNode* pClosestMatch		= NULL;
	float currentYDiff					= FLOAT_MAX;

	vector_3 mod( 0.0f, 0.2f, 0.0f );

	// Go through them
	for( unsigned int i = 0; i < node.GetNumLogicalNodes(); ++i )
	{
		SiegeLogicalNode& lNode	= *ppLogicalNodes[ i ];

		if( !lNode.GetLogicalMesh() )
		{
			continue;
		}

		// See if this particular LogicalNode matches our search criteria
		float ray_min	= FLOAT_MAX;
		vector_3 face_normal( DoNotInitialize );

		if( lNode.GetFlags() & flags )
		{
			// See if a ray hits this logical node
			vector_3 node_ray_origin( pos.pos.x, lNode.GetLogicalMesh()->GetMinimumBounds().y - 0.1f, pos.pos.z );
			if( lNode.HitTest( node_ray_origin, vector_3::UP, ray_min, face_normal ) )
			{
				float ydiff	= FABSF( pos.pos.y - (node_ray_origin.y + ray_min) );
				if( ydiff < currentYDiff )
				{
					currentYDiff		= ydiff;
					pCurrentMatch		= ppLogicalNodes[ i ];
				}
			}
			// See if the given node space position is inside this LogicalNode
			else if( PointInBox( (lNode.GetLogicalMesh()->GetMinimumBounds()-mod), (lNode.GetLogicalMesh()->GetMaximumBounds()+mod), pos.pos ) )
			{
				pClosestMatch = ppLogicalNodes[ i ];
			}
		}
	}

	return( pCurrentMatch != NULL ? pCurrentMatch : pClosestMatch );
}

// See if this position is blocked
bool SiegeEngine::IsPositionBlocked( SiegeLogicalNode* pLogicalNode, const SiegePos& pos )
{
	// Find this leaf
	unsigned short leaf_id	= m_pPathfinder->GetLeafAtPosition( pLogicalNode, pos.pos );
	if( leaf_id != 0xFFFF )
	{
		// Check to see if it is blocked
		return pLogicalNode->IsLeafBlocked( leaf_id );
	}

	return true;
}

// See if position is allowed for given position and flags
bool SiegeEngine::IsPositionAllowed( SiegePos& pos, const eLogicalNodeFlags flags, const float maxHeightDelta )
{
	// Check to make sure this position is valid
	if( !IsNodeValid( pos.node ) )
	{
		return false;
	}

	// Adjust point to terrain and check difference values for validity
	SiegePos adjusted_pos	= pos;
	if( AdjustPointToTerrain( adjusted_pos, flags ) )
	{
		if( FABSF( GetDifferenceVector( adjusted_pos, pos ).y ) > maxHeightDelta )
		{
			return false;
		}

		SiegeLogicalNode* pLogicalNode = GetLogicalNodeAtPosition( adjusted_pos, flags );
		if( IsPositionBlocked( pLogicalNode, adjusted_pos ) )
		{
			return false;
		}

		pos	= adjusted_pos;
		return true;
	}

	return false;
}

// Helpful function that will recursively traverse the known nodal tree the requested
// number of levels and call the given callback function for each unique node.
void SiegeEngine::NodeTraversalCallback( const database_guid startGuid,
										 CBFunctor2wRet< const SiegeNode&, void*, bool > callBack,
										 unsigned int numRecurse,
										 void* pAppDefinedData )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	gpassert( m_nodeStack.empty() );

	unsigned int currentRecurse		= 0;
	unsigned int visitCount			= RandomDword();
	unsigned int i					= 0;
	unsigned int size				= 0;

	// Get the start node handle and push it onto our list
	SiegeNode* pStartNode			= IsNodeValid( startGuid );
	if( !pStartNode )
	{
		return;
	}

	pStartNode->SetAPVisited( visitCount );
	m_nodeStack.push_back( pStartNode );

	do
	{
		// Get the current size of the stack
		size = m_nodeStack.size();

		// Go through the nodes in the current list
		for( ; i < size; ++i )
		{
			// Get the current node
			const siege::SiegeNode& node	= *m_nodeStack[i];

			// Call the callback function
			if( callBack( node, pAppDefinedData ) &&
				currentRecurse < numRecurse )
			{
				// Go through the neighbors and put them on the new list
				const SiegeNodeDoorList& doors	= node.GetDoors();
				for( SiegeNodeDoorConstIter d = doors.begin(); d != doors.end(); ++d )
				{
					SiegeNode* pNode	= IsNodeValid( (*d)->GetNeighbour() );
					if( !pNode || pNode->GetAPVisited() == visitCount )
					{
						continue;
					}

					pNode->SetAPVisited( visitCount );

					// Push the node onto the list
					m_nodeStack.push_back( pNode );
				}
			}
		}

		// Set our current list to the new list and increment our recursion counter
		currentRecurse++;
	}
	while( size < m_nodeStack.size() );

	// Clear the stack
	m_nodeStack.clear();
}

// Transition a node to a new position.  Handles the mathematical interpolation, but not
// any state tracking.  Takes a blind callback which is notified of completion of the transition.
bool SiegeEngine::NodeTransition( const database_guid targetGuid, const DWORD targetDoor,
								  const database_guid connectGuid, const DWORD connectDoor,
								  const double current_time, const double start_time, const double end_time,
								  const eAxisHint ahint, const DWORD owner, bool bConnect, bool bForceComplete )
{
	// Start the transition
	return m_pTransition->StartTransition( targetGuid, targetDoor, connectGuid, connectDoor, current_time, start_time, end_time, ahint, owner, bConnect, bForceComplete );
}

// Add the specified door connection and connect the two nodes
bool SiegeEngine::NodeConnection( const database_guid targetGuid, const DWORD targetDoor,
								  const database_guid connectGuid, const DWORD connectDoor,
								  bool bConnect, bool bForceComplete )
{
	// Connect the nodes
	return m_pTransition->NodeConnection( targetGuid, targetDoor, connectGuid, connectDoor, bConnect, bForceComplete );
}

// Query to see if the pathing is connected between two nodes
bool SiegeEngine::IsNodalPathingConnected( const database_guid& near_node, const database_guid& far_node )
{
	// Always return true if the node is the same
	if( near_node == far_node )
	{
		return true;
	}

	gpassert( near_node.IsValid() );
	gpassert( far_node.IsValid() );

	SiegeNode* pNearNode	= m_pNodeCache->GetValidObjectInCache( near_node );
	SiegeNode* pFarNode		= m_pNodeCache->GetValidObjectInCache( far_node );

	if( pNearNode && pFarNode )
	{
		// Go through all of the logical nodes for the near node
		for( unsigned int i = 0; i < pNearNode->GetNumLogicalNodes(); ++i )
		{
			// Go through all of the logical nodes for the far node
			for( unsigned int o = 0; o < pFarNode->GetNumLogicalNodes(); ++o )
			{
				// Check for connection
				if( pNearNode->GetLogicalNodes()[ i ]->IsConnectedTo( pFarNode->GetLogicalNodes()[ o ] ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

// Connect two siege nodes logical information
bool SiegeEngine::ConnectSiegeNodes( const database_guid& near_node, const database_guid& far_node )
{
	if ( near_node == far_node )
	{
		gperrorf(( "Cannot stitch node %s to itself!", near_node.ToString().c_str() ));
		return false;
	}

	gpassert( near_node.IsValid() );
	gpassert( far_node.IsValid() );

	SiegeNode* pNearNode	= m_pNodeCache->GetValidObjectInCache( near_node );
	SiegeNode* pFarNode		= m_pNodeCache->GetValidObjectInCache( far_node );

	if( pNearNode && pFarNode )
	{
		// Take the space critical
		kerneltool::Critical::Lock autoLock( m_spaceCritical );

		if( !pNearNode->HasValidSpace() || !pFarNode->HasValidSpace() )
		{
			// Find the door
			SiegeNodeDoor* pNearDoor		= NULL;
			SiegeNodeDoor* pFarDoor			= NULL;

			for( SiegeNodeDoorList::const_iterator d = pFarNode->GetDoors().begin(); d != pFarNode->GetDoors().end(); ++d )
			{
				if( (*d)->GetNeighbour() == near_node )
				{
					pFarDoor	= (*d);
					pNearDoor	= pNearNode->GetDoorByID( pFarDoor->GetNeighbourID() );
					break;
				}
			}

			gpassert( pNearDoor && pFarDoor );

			// Get the render frustum
			SiegeFrustum* pRenderFrustum = gSiegeEngine.GetFrustum( gSiegeEngine.GetRenderFrustum() );
			if( !pRenderFrustum )
			{
				return false;
			}

			if( !pNearNode->HasValidSpace() && !pFarNode->HasValidSpace() )
			{
				// Set near node to identity
				pNearNode->SetCurrentCenter( vector_3::ZERO );
				pNearNode->SetCurrentOrientation( matrix_3x3::IDENTITY );

				pRenderFrustum->BuildNodeSpace( *pNearNode, pNearDoor, *pFarNode, false );
				pFarNode->SetSpaceFrustum( 0 );
			}
			else
			{
				// Make sure that we have valid space
				if( !pNearNode->HasValidSpace() )
				{
					// Build space for our near node
					pRenderFrustum->BuildNodeSpace( *pFarNode, pFarDoor, *pNearNode, false );
					pNearNode->SetSpaceFrustum( 0 );
				}
				if( !pFarNode->HasValidSpace() )
				{
					// Build space for our far node
					pRenderFrustum->BuildNodeSpace( *pNearNode, pNearDoor, *pFarNode, false );
					pFarNode->SetSpaceFrustum( 0 );
				}
			}
		}

		// Setup expansion vector
		vector_3 mod( 0.05f, 0.05f, 0.05f );

		// Go through all of the LogicalNodes for the near node
		for ( unsigned int i = 0; i < pNearNode->GetNumLogicalNodes(); ++i )
		{
			SiegeLogicalNode* pNearLogicalNode	= pNearNode->GetLogicalNodes()[ i ];
			if ( !pNearLogicalNode->GetLogicalMesh() )
			{
				continue;
			}

			// Get the world space bounding volume for this logical node
			vector_3 minNearWorld( DoNotInitialize );
			vector_3 maxNearWorld( DoNotInitialize );
			pNearNode->NodeToWorldBox( pNearLogicalNode->GetLogicalMesh()->GetMinimumBounds(),
									   pNearLogicalNode->GetLogicalMesh()->GetMaximumBounds(),
									   minNearWorld, maxNearWorld );

			// Go through all of the LogicalNodes for the far node
			for ( unsigned int o = 0; o < pFarNode->GetNumLogicalNodes(); ++o )
			{
				SiegeLogicalNode* pFarLogicalNode	= pFarNode->GetLogicalNodes()[ o ];
				if ( !pFarLogicalNode->GetLogicalMesh() )
				{
					continue;
				}

				// Now we need to check for incontinuity.  This is basically when one node points to the other, but the
				// other doesn't point back.  This can by caused by asynchronous loading on regional boundaries, so it
				// is imperative that we properly handle this case
				bool bNearToFar	= pNearLogicalNode->IsConnectedTo( pFarLogicalNode );
				bool bFarToNear	= pFarLogicalNode->IsConnectedTo( pNearLogicalNode );
				if ( bNearToFar && bFarToNear )
				{
					// Both nodes already contain this connection, so we are done
					continue;
				}

				// Get the world space bounding volume for this logical node
				vector_3 minFarWorld( DoNotInitialize );
				vector_3 maxFarWorld( DoNotInitialize );
				pFarNode->NodeToWorldBox( pFarLogicalNode->GetLogicalMesh()->GetMinimumBounds(),
										  pFarLogicalNode->GetLogicalMesh()->GetMaximumBounds(),
										  minFarWorld, maxFarWorld );
				// See if they intersect
				if ( BoxIntersectsBox( (minNearWorld-mod), (maxNearWorld+mod), (minFarWorld-mod), (maxFarWorld+mod) ) )
				{
					// These two logical nodes intersect
					ConnectLogicalNodes( pNearLogicalNode, bNearToFar, pFarLogicalNode, bFarToNear );
				}
				else
				{
					if ( bNearToFar != bFarToNear )
					{
						gperror( "During internodal connection, and inconsistency was found that could not be corrected." );
					}
				}
			}
		}

		return true;
	}

	return false;
}

// Disconnect two siege nodes logical information
bool SiegeEngine::DisconnectSiegeNodes( const database_guid& near_node, const database_guid& far_node )
{
	if ( near_node == UNDEFINED_GUID || far_node == UNDEFINED_GUID )
	{
		gperror( "Cannot disconnect nodes because one or both are undefined!" );
		return false;
	}
	if ( near_node == far_node )
	{
		gperror( "Cannot disconnect node from itself!" );
		return false;
	}

	// Synchro threads while we update the pointers
	kerneltool::Critical::Lock autoLock( m_logicalCritical );

	// Get the near node
	SiegeNode* pNearNode	= m_pNodeCache->GetValidObjectInCache( near_node );
	if( pNearNode )
	{
		// Go through the near node
		for ( unsigned int i = 0; i < pNearNode->GetNumLogicalNodes(); ++i )
		{
			SiegeLogicalNode* pLogicalNode	= pNearNode->GetLogicalNodes()[ i ];

			// Go through the logical connections
			for ( unsigned int o = 0; o < pLogicalNode->GetNumNodeConnections(); )
			{
				if ( pLogicalNode->GetNodeConnectionInfo()[ o ].m_farSiegeNode == far_node )
				{
					// Remove this connection
					delete[] pLogicalNode->GetNodeConnectionInfo()[ o ].m_pCollection->m_pNodalLeafConnections;
					delete   pLogicalNode->GetNodeConnectionInfo()[ o ].m_pCollection;

					LNODECONNECT* pNewConnections	= new LNODECONNECT[ pLogicalNode->GetNumNodeConnections() - 1 ];
					memcpy( pNewConnections, pLogicalNode->GetNodeConnectionInfo(), sizeof( LNODECONNECT ) * o );
					memcpy( &pNewConnections[o], &pLogicalNode->GetNodeConnectionInfo()[o+1], sizeof( LNODECONNECT ) * (pLogicalNode->GetNumNodeConnections() - o - 1) );

					delete[] pLogicalNode->GetNodeConnectionInfo();
					pLogicalNode->SetNodeConnectionInfo( pNewConnections );

					unsigned short compilerismyfriend	= pLogicalNode->GetNumNodeConnections();
					compilerismyfriend					-= 1;
					pLogicalNode->SetNumNodeConnections( compilerismyfriend );
				}
				else
				{
					++o;
				}
			}
		}
	}

	SiegeNode* pFarNode		= m_pNodeCache->GetValidObjectInCache( far_node );
	if( pFarNode )
	{
		// Go through the far node
		for ( unsigned int i = 0; i < pFarNode->GetNumLogicalNodes(); ++i )
		{
			SiegeLogicalNode* pLogicalNode	= pFarNode->GetLogicalNodes()[ i ];

			// Go through the logical connections
			for ( unsigned int o = 0; o < pLogicalNode->GetNumNodeConnections(); )
			{
				if ( pLogicalNode->GetNodeConnectionInfo()[ o ].m_farSiegeNode == near_node )
				{
					// Remove this connection
					delete[] pLogicalNode->GetNodeConnectionInfo()[ o ].m_pCollection->m_pNodalLeafConnections;
					delete   pLogicalNode->GetNodeConnectionInfo()[ o ].m_pCollection;

					LNODECONNECT* pNewConnections	= new LNODECONNECT[ pLogicalNode->GetNumNodeConnections() - 1 ];
					memcpy( pNewConnections, pLogicalNode->GetNodeConnectionInfo(), sizeof( LNODECONNECT ) * o );
					memcpy( &pNewConnections[o], &pLogicalNode->GetNodeConnectionInfo()[o+1], sizeof( LNODECONNECT ) * (pLogicalNode->GetNumNodeConnections() - o - 1) );

					delete[] pLogicalNode->GetNodeConnectionInfo();
					pLogicalNode->SetNodeConnectionInfo( pNewConnections );

					unsigned short compilerismyfriend	= pLogicalNode->GetNumNodeConnections();
					compilerismyfriend					-= 1;
					pLogicalNode->SetNumNodeConnections( compilerismyfriend );
				}
				else
				{
					++o;
				}
			}
		}
	}

	return true;
}

// Connect a single logical node pair
bool SiegeEngine::ConnectLogicalNodes( SiegeLogicalNode* pNearNode, bool bNearToFarConnection,
									   SiegeLogicalNode* pFarNode, bool bFarToNearConnection )
{
	// Connect the leaves of these two logical nodes together
	LMESHLEAFINFO* pNearLeaves		= pNearNode->GetLogicalMesh()->GetLeafConnectionInfo();
	LMESHLEAFINFO* pFarLeaves		= pFarNode->GetLogicalMesh()->GetLeafConnectionInfo();
	unsigned int numNearLeaves		= pNearNode->GetLogicalMesh()->GetNumLeafConnections();
	unsigned int numFarLeaves		= pFarNode->GetLogicalMesh()->GetNumLeafConnections();

	// Get the near node
	SiegeNode& nearnode = *pNearNode->GetSiegeNode();

	// Get the far node
	SiegeNode& farnode = *pFarNode->GetSiegeNode();

	// Setup expansion vector
	vector_3 mod( 0.05f, 0.05f, 0.05f );

	std::vector< NODALLEAFCONNECT >		leafConnectionColl;
	std::vector< LeafConnectionInfo >	farConnectionColl;
	farConnectionColl.reserve( numFarLeaves );

	// Find the connections
	for ( unsigned int o = 0; o < numFarLeaves; ++o )
	{
		// Get the far leaf
		LMESHLEAFINFO* pFarLeaf	= &pFarLeaves[ o ];

		// Setup a world space bounding volume
		LeafConnectionInfo lci;

		farnode.NodeToWorldBox( pFarLeaf->minBox, pFarLeaf->maxBox, lci.wMinBox, lci.wMaxBox );

		lci.wMinBox		-= mod;
		lci.wMaxBox		+= mod;
		lci.id			= pFarLeaf->id;

		farConnectionColl.push_back( lci );
	}

	for ( unsigned int i = 0; i < numNearLeaves; ++i )
	{
		// Get the near leaf
		LMESHLEAFINFO* pNearLeaf	= &pNearLeaves[ i ];

		// Setup a world space bounding volume
		vector_3 nearminBox( DoNotInitialize );
		vector_3 nearmaxBox( DoNotInitialize );
		nearnode.NodeToWorldBox( pNearLeaf->minBox, pNearLeaf->maxBox, nearminBox, nearmaxBox );

		nearminBox					-= mod;
		nearmaxBox					+= mod;

		// Find the connections
		for ( std::vector< LeafConnectionInfo >::iterator f = farConnectionColl.begin(); f != farConnectionColl.end(); ++f )
		{
			if ( BoxIntersectsBox( nearminBox, nearmaxBox, (*f).wMinBox, (*f).wMaxBox ) )
			{
				NODALLEAFCONNECT nConnection;

				nConnection.local_id	= pNearLeaf->id;
				nConnection.far_id		= (*f).id;

				leafConnectionColl.push_back( nConnection );
			}
		}
	}

	// Connect two logical nodes together
	LNODECONNECT* pNearConnections		= pNearNode->GetNodeConnectionInfo();
	LNODECONNECT* pFarConnections		= pFarNode->GetNodeConnectionInfo();
	unsigned short numNearConnections	= pNearNode->GetNumNodeConnections();
	unsigned short numFarConnections	= pFarNode->GetNumNodeConnections();

	LNODECONNECT* pNewNearConnections	= NULL;
	LNODECONNECT* pNewFarConnections	= NULL;

	// Synchro threads while we update the pointers
	kerneltool::Critical::Lock autoLock( m_logicalCritical );

	// Make the new lists and copy the old information into them
	if ( !bNearToFarConnection )
	{
		pNewNearConnections				= new LNODECONNECT[ numNearConnections + 1 ];
		memcpy( pNewNearConnections, pNearConnections, sizeof( LNODECONNECT ) * numNearConnections );

		// Add the new connections
		pNewNearConnections[ numNearConnections ].m_farSiegeNode			= pFarNode->GetSiegeNode()->GetGUID();
		pNewNearConnections[ numNearConnections ].m_pCollection				= new LCCOLLECTION;
		pNewNearConnections[ numNearConnections ].m_pCollection->m_farid	= pFarNode->GetID();

		// Set the new data in the Logical node
		pNearNode->SetNodeConnectionInfo( pNewNearConnections );
		pNearNode->SetNumNodeConnections( (unsigned short)(numNearConnections + 1) );
		delete[] pNearConnections;
	}

	if ( !bFarToNearConnection )
	{
		pNewFarConnections				= new LNODECONNECT[ numFarConnections + 1 ];
		memcpy( pNewFarConnections, pFarConnections, sizeof( LNODECONNECT ) * numFarConnections );

		// Add the new connections
		pNewFarConnections[ numFarConnections ].m_farSiegeNode				= pNearNode->GetSiegeNode()->GetGUID();
		pNewFarConnections[ numFarConnections ].m_pCollection				= new LCCOLLECTION;
		pNewFarConnections[ numFarConnections ].m_pCollection->m_farid		= pNearNode->GetID();

		// Set the new data in the Logical node
		pFarNode->SetNodeConnectionInfo( pNewFarConnections );
		pFarNode->SetNumNodeConnections( (unsigned short)(numFarConnections + 1) );
		delete[] pFarConnections;
	}

	// Build new lists
	unsigned int numConnections		= leafConnectionColl.size();
	LNODECONNECT* pNearConnection	= NULL;
	LNODECONNECT* pFarConnection	= NULL;

	if ( !bNearToFarConnection )
	{
		pNearConnection												= &pNewNearConnections[ numNearConnections ];
		pNearConnection->m_pCollection->m_numNodalLeafConnections	= numConnections;
		pNearConnection->m_pCollection->m_pNodalLeafConnections		= new NODALLEAFCONNECT[ numConnections ];
	}

	if ( !bFarToNearConnection )
	{
		pFarConnection												= &pNewFarConnections[ numFarConnections ];
		pFarConnection->m_pCollection->m_numNodalLeafConnections	= numConnections;
		pFarConnection->m_pCollection->m_pNodalLeafConnections		= new NODALLEAFCONNECT[ numConnections ];
	}

	unsigned int index	= 0;
	for ( std::vector< NODALLEAFCONNECT >::iterator n = leafConnectionColl.begin(); n != leafConnectionColl.end(); ++n, ++index )
	{
		if ( pNearConnection )
		{
			pNearConnection->m_pCollection->m_pNodalLeafConnections[ index ]			= (*n);
		}

		if ( pFarConnection )
		{
			pFarConnection->m_pCollection->m_pNodalLeafConnections[ index ].local_id	= (*n).far_id;
			pFarConnection->m_pCollection->m_pNodalLeafConnections[ index ].far_id		= (*n).local_id;
		}
	}

	return true;
}

// Rebuild the doors
bool SiegeEngine::RebuildDoors( const database_guid nodeGuid )
{
	return m_pTransition->RebuildDoors( nodeGuid );
}

// Process any disconnects for this node that need to be setup as the result of a transition
void SiegeEngine::ProcessDisconnects( const database_guid nodeGuid )
{
	m_pTransition->ProcessDisconnects( nodeGuid );
}

// Collect doors using the registered callback
void SiegeEngine::CollectDoors( const database_guid& nodeGuid, SiegeTransition::NodeDoorColl& doorColl )
{
	gpassert( m_collectDoorsCb );

	// Collect the doors using the passed callback
	if( m_collectDoorsCb )
	{
		m_collectDoorsCb( nodeGuid, doorColl );
	}
}

// Collect object using registered callback
void SiegeEngine::CollectObjects( WorldObjectColl& objects, const SiegeNode& node, eCollectFlags cFlags, eRequestFlags rFlags )
{
	gpassert( m_collectObjectsCb );

	// Collect the objects using the passed callback
	if( m_collectObjectsCb )
	{
		m_collectObjectsCb( objects, node, cFlags, rFlags );
	}
}

// Collect effects using registered callback
void SiegeEngine::CollectEffects( WorldEffectColl& effects, const SiegeNode& node )
{
	// Collect the objects using the passed callback
	if( m_collectEffectsCb )
	{
		m_collectEffectsCb( effects, node );
	}
}

// Notify callback of node transition completion
void SiegeEngine::TransitionComplete( database_guid targetNode, DWORD owner )
{
	gpassert( m_transitionCompleteCb );

	if( m_transitionCompleteCb )
	{
		m_transitionCompleteCb( targetNode, owner );
	}
}

// Node ownership has changed
void SiegeEngine::NotifyNodeWorldMembershipChanged( const SiegeNode& node, unsigned int oldMembership, bool bDelete )
{
	if ( m_nwmcCb )
	{
		m_nwmcCb( node, oldMembership, bDelete );
	}
}

// Node upgraded ownership changed
void SiegeEngine::NotifyNodeUpgradedMembershipChanged( const SiegeNode& node, unsigned int oldMembership )
{
	if ( m_numcCb )
	{
		m_numcCb( node, oldMembership );
	}
}

// Register new cb
void SiegeEngine::RegisterNotifyNodeLoadCb( NLCALLBACK cb )
{
	gpassert( cb );
	m_nlCbColl.push_back( cb );
}

// Unregister existing cb
void SiegeEngine::UnregisterNotifyNodeLoadCb( NLCALLBACK cb )
{
	NLCALLBACKCOLL::iterator found = stdx::find( m_nlCbColl, cb );
	if ( found != m_nlCbColl.end() )
	{
		m_nlCbColl.erase( found );
	}
}

// Node load notify
void SiegeEngine::NotifyNodeLoad( const database_guid nodeGuid )
{
	NLCALLBACKCOLL::iterator i, ibegin = m_nlCbColl.begin(), iend = m_nlCbColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)( nodeGuid );
	}
}

// Node space changed notify
void SiegeEngine::NotifyNodeSpaceChanged( const database_guid nodeGuid, bool bIsElevator )
{
	if( m_nscCb )
	{
		m_nscCb( nodeGuid, bIsElevator );
	}

	// Update decal projections now that space has changed
	if( !m_pRenderer->IsManualTextureProjection() )
	{
		gSiegeDecalDatabase.UpdateDecalProjection( nodeGuid );
	}

	// Update hotpoint information
	SiegeNode* pNode	= DoesNodeExist( nodeGuid );
	if( pNode && pNode->IsHotpointUpdateRequested() )
	{
		SiegeFrustum* pFrustum	= GetFrustum( pNode->GetSpaceFrustum() );
		if( pFrustum )
		{
			gSiegeHotpointDatabase.UpdateHotpointDirections( pFrustum->GetPosition().node, pNode->GetGUID() );
			pNode->RequestHotpointUpdate( false );
		}
	}
}

// Post opaque render notify
void SiegeEngine::NotifyPostOpaqueRender()
{
	if( m_porCb )
	{
		m_porCb();
	}
}

void SiegeEngine::PrintFrameRate( float gameFPS, float gamespeed, RapiFont& font )
{
	char tempstr[256];

#if !GP_RETAIL

	// Print FPS and game speed
	if (gamespeed < 0.02f)
	{
		sprintf( tempstr, "GameFPS:[%5.1f]  RenderFPS:[%5.1f]  Speed:[%5.3f]  Flip[ %s ]",
				 gameFPS, m_pRenderer->GetFrameRate(), gamespeed,
				 (m_pRenderer->IsFullscreen() && m_pRenderer->IsFlipAllowed()) ? "true" : "false" );
	}
	else if (gamespeed < 0.2f)
	{
		sprintf( tempstr, "GameFPS:[%5.1f]  RenderFPS:[%5.1f]  Speed:[%5.2f]  Flip[ %s ]",
				 gameFPS, m_pRenderer->GetFrameRate(), gamespeed,
				 (m_pRenderer->IsFullscreen() && m_pRenderer->IsFlipAllowed()) ? "true" : "false" );
	}
	else
	{
		sprintf( tempstr, "GameFPS:[%5.1f]  RenderFPS:[%5.1f]  Speed:[%5.1f]  Flip[ %s ]",
				 gameFPS, m_pRenderer->GetFrameRate(), gamespeed,
				 (m_pRenderer->IsFullscreen() && m_pRenderer->IsFlipAllowed()) ? "true" : "false" );
	}
	font.Print( 0, 0, tempstr );

	// More texture info
	sprintf( tempstr, "TexSwitch:[%d]  NumTex:[%d]  SystemDetailLevel:[%d]  BPP:[%d]",
			 m_pRenderer->GetTextureSwitches(), m_pRenderer->GetNumActiveTex(),
			 m_pRenderer->GetSystemDetailIdentifier(), m_pRenderer->GetBitsPerPixel() );
	font.Print( 0, 10, tempstr );

	// Output random engine information
	sprintf( tempstr, "TexMem:[%dK]  SysTexMem:[%dK]  AvailTexMem:[%dK]  TotalVideoMem:[%dK]",
			 m_pRenderer->GetTextureMemory() / 1024, (m_pRenderer->GetTexSystemMemory() / 1024) + (m_pRenderer->GetTexManagerSystemMemory() / 1024),
			 m_pRenderer->GetAvailableTextureMemory() / 1024, m_pRenderer->GetSystemVideoMemory() / 1024 );
	font.Print( 0, 20, tempstr );

#else

	sprintf( tempstr, "FPS: [%5.1f]", gameFPS );
	font.Print( 0, 0, tempstr );

#endif
}

void SiegeEngine::PrintPolyStats( RapiFont& font )
{
	char tempstr[80];

	sprintf( tempstr, " Cost. (%5f)", m_NemaCostRendered);
	font.Print( 0, 40, tempstr );

	sprintf( tempstr, " Tri   (%5d %5d %5d) %6d", m_SiegeTrisRendered, m_NemaTrisRendered, m_DecalTrisRendered, m_TotalTrisRendered);
	font.Print( 0, 50, tempstr );

	sprintf( tempstr, " Vert  (%5d %5d      ) %6d", m_SiegeVertsRendered, m_NemaVertsRendered, m_TotalVertsRendered);
	font.Print( 0, 60, tempstr );

	sprintf( tempstr, "STri   (%5d %5d      )", m_SelectedSiegeTrisRendered, m_SelectedNemaTrisRendered);
	font.Print( 0, 70, tempstr );

	sprintf( tempstr, "SVert  (%5d %5d      )", m_SelectedSiegeVertsRendered, m_SelectedNemaVertsRendered);
	font.Print( 0, 80, tempstr );

	if( !m_SelectedSiegeNodeName.empty() )
	{
		sprintf( tempstr, "(%s)", m_SelectedSiegeNodeName.c_str() );
		font.Print( 0, 90, tempstr );
	}
}

// Create new frustum using optional callback
SiegeFrustum* SiegeEngine::CreateNewFrustum( DWORD idBitfield )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	SiegeFrustum* frustum = NULL;
	if ( m_frustumFactoryCb )
	{
		frustum = m_frustumFactoryCb( idBitfield );
	}
	else
	{
		frustum = new SiegeFrustum( idBitfield );
	}
	return ( frustum );
}

// Create a frustum
DWORD SiegeEngine::CreateFrustum( DWORD idBitfield )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( !m_bUpdatingFrustums )
	{
		// Check it
		if ( GetFrustum( idBitfield ) != NULL )
		{
			gperrorf(( "Cannot create new frustum id 0x%08X if something already there!\n", idBitfield ));
		}

		// Make it
		SiegeFrustum* frustum = CreateNewFrustum( idBitfield );
		gpassert( frustum != NULL );
		idBitfield = frustum->GetIdBitfield();

		// Set it
		size_t index = GetShift( idBitfield );
		m_frustumColl.resize( max_t( index + 1, m_frustumColl.size() ), NULL );
		gpassert( m_frustumColl[ index ] == NULL );
		m_frustumColl[ index ] = frustum;

		// Return the final bitfield
		return ( idBitfield );
	}
	else
	{
		gperror( "Cannot create frustum during a frustum update!" );
	}

	return 0;
}

// Destroy a frustum
void SiegeEngine::DestroyFrustum( DWORD idBitfield, bool bUpdate )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( !m_bUpdatingFrustums )
	{
		gpassert( IsPower2( idBitfield ) );
		size_t index = GetShift( idBitfield );
		gpassert( index < m_frustumColl.size() );

		// Since we are deleting a frustum, it is imperative that we make sure
		// all node load orders are completed.
		if( LoadMgr::DoesSingletonExist() )
		{
			gSiegeLoadMgr.Commit( LOADER_SIEGE_NODE_TYPE );
		}

		{
			// Take the space critical
			kerneltool::Critical::Lock autoLock( m_spaceCritical );

			// Destroy frustum
			SiegeFrustum*& pFrustum = m_frustumColl[ index ];
			gpassert( pFrustum != NULL );
			gpassert( pFrustum->GetIdBitfield() == idBitfield );
			Delete( pFrustum );

			// Clear out unused trailing slots
			while ( !m_frustumColl.empty() && (*m_frustumColl.rbegin() == NULL) )
			{
				m_frustumColl.pop_back();
			}
		}

		// Update frustums to rebuild space trees that may have changed due to
		// this destruction.
		if( bUpdate )
		{
			UpdateFrustums( 0.0f );
		}
	}
	else
	{
		gperror( "Cannot destroy a frustum during a frustum update!" );
	}
}

// Nuke 'em all
void SiegeEngine::DestroyAllFrusta()
{
	while ( !m_frustumColl.empty() )
	{
		DestroyFrustum( (*m_frustumColl.rbegin())->GetIdBitfield(), false );
	}
}

// Frustum update
bool SiegeEngine::UpdateFrustums( float secondsElapsed, bool completeUpdate )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	// Set our updating state
	m_bUpdatingFrustums	= true;

	// Update fades
	UpdateFades();

	// Update transitions
	m_pTransition->Update( secondsElapsed );

	// Pre-request updates from all frustums (optional)
	if ( completeUpdate )
	{
		for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
		{
			if ( *f != NULL )
			{
				(*f)->RequestUpdate();
			}
		}
	}

	// Update frustums
	bool bCompletelyLoaded	= true;
	for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
	{
		if ( *f != NULL )
		{
			// Update this frustum
			if( completeUpdate )
			{
				for( ;; )
				{
					if( (*f)->m_bUpdateFrustum && (*f)->IsFrustumOwned() )
					{
						// Update parent
						GetFrustum( (*f)->GetFrustumOwned() )->Update( 0.0f );
					}

					if( !(*f)->Update( 0.0f ) )
					{
						break;
					}
					else
					{
						GetFrustum( (*f)->GetFrustumOwned() )->RequestUpdate();
					}
				}

				// Update discovery position information
				m_pWalker->SubmitDiscoveryPosition( (*f)->GetPosition(), (*f) );
			}
			else
			{
				if( (*f)->Update( secondsElapsed ) )
				{
					bCompletelyLoaded	= false;
					if( (*f)->IsFrustumOwned() )
					{
						// Update parent
						GetFrustum( (*f)->GetFrustumOwned() )->RequestUpdate();
					}
				}
			}

			// Update node fades for this frustum
			(*f)->UpdateNodeFades();
		}
	}

	// Get the render frustum
	if( m_renderFrustum )
	{
		SiegeFrustum* pRenderFrustum		= GetFrustum( m_renderFrustum );
		if( pRenderFrustum )
		{
			// Update space if the render frustum is still requesting it.  This basically
			// means that the render frustum was released from ownership after it's own
			// update, so we need to update it again.
			if( !pRenderFrustum->IsFrustumOwned() && pRenderFrustum->IsSpaceUpdate() )
			{
				// Update space
				if( pRenderFrustum->Update( 0.0f ) )
				{
					bCompletelyLoaded	= false;
				}
			}

			// Get controlling frustum
			SiegeFrustum* pControlFrustum	= pRenderFrustum;
			if( pRenderFrustum->IsFrustumOwned() )
			{
				pControlFrustum		= gSiegeEngine.GetFrustum( pRenderFrustum->GetFrustumOwned() );
				if( !pControlFrustum )
				{
					pControlFrustum	= pRenderFrustum;
				}
			}

			// Take our target node from the render frustum's target node
			m_pWalker->SetTargetNodeGUID( pControlFrustum->GetPosition().node );

			// Update the camera to correct for potential space changes
			m_pCamera->UpdateCamera();
		}
	}

	// Update transitions
	m_pTransition->UpdateTransitionPositions();

	// Re-request updates from all frustums (optional)
	if ( completeUpdate )
	{
		for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
		{
			if ( *f != NULL )
			{
				(*f)->RequestUpdate();
			}
		}

		// Call self again to make sure elevators will have proper membership
		// now that all the nodes have space.
		UpdateFrustums( 0, false );
	}

	// Reset our updating state
	m_bUpdatingFrustums	= false;

	return bCompletelyLoaded;
}

void SiegeEngine::UpdateFrustumOwnership()
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	// Go through frustums
	for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
	{
		if ( *f != NULL )
		{
			// Update ownership
			(*f)->UpdateFrustumOwnership();
		}
	}
}

// Frustum access
siege::SiegeFrustum* SiegeEngine::GetFrustum( const DWORD idBitfield )
{
	siege::SiegeFrustum* pFrustum = NULL;

	if ( IsPower2( idBitfield ) )
	{
		size_t index = GetShift( idBitfield );
		if ( index < m_frustumColl.size() )
		{
			pFrustum = m_frustumColl[ index ];
			gpassert( (pFrustum == NULL) || (pFrustum->GetIdBitfield() == idBitfield) );
		}
	}

	return ( pFrustum );
}

// Set the render frustum
void SiegeEngine::SetRenderFrustum( const DWORD idBitfield )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	m_renderFrustum = idBitfield;

	// Get the render frustum
	if( m_renderFrustum )
	{
		SiegeFrustum* pRenderFrustum		= GetFrustum( m_renderFrustum );
		if( pRenderFrustum )
		{
			// Get controlling frustum
			SiegeFrustum* pControlFrustum	= pRenderFrustum;
			if( pRenderFrustum->IsFrustumOwned() )
			{
				pControlFrustum		= gSiegeEngine.GetFrustum( pRenderFrustum->GetFrustumOwned() );
				if( !pControlFrustum )
				{
					pControlFrustum	= pRenderFrustum;
				}
			}

			// Take our target node from the render frustum's target node
			m_pWalker->SetTargetNodeGUID( pControlFrustum->GetPosition().node );
		}
	}
}

// Add a frustum node replacment (for transition support)
void SiegeEngine::AddFrustumNodeReplacement( const database_guid& nodeGuid, const database_guid& repGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	m_nodeReplacementMap.insert( std::pair< database_guid, database_guid >( nodeGuid, repGuid ) );

	// Update position to accomodate replacement
	for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
	{
		if ( ((*f) != NULL) && ((*f)->GetPosition().node == nodeGuid) )
		{
			(*f)->SetPosition( (*f)->GetPosition() );
		}
	}
}

// Remove a frustum node replacement
void SiegeEngine::RemoveFrustumNodeReplacement( const database_guid& nodeGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	m_nodeReplacementMap.erase( nodeGuid );

	// Update position to accomodate replacement
	for( SiegeFrustumColl::iterator f = m_frustumColl.begin(); f != m_frustumColl.end(); ++f )
	{
		if ( ((*f) != NULL) && ((*f)->GetRequestedPosition().node == nodeGuid) )
		{
			(*f)->SetPosition( (*f)->GetRequestedPosition() );
		}
	}
}

// See if the given node is in transition
bool SiegeEngine::IsNodeInTransition( const database_guid& nodeGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	return( m_nodeReplacementMap.find( nodeGuid ) != m_nodeReplacementMap.end() );
}

// Utility functions
unsigned int SiegeEngine::GetNodeFrustumOwnedBitfield( const database_guid& nodeGuid )
{
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		return pNode->GetFrustumOwnedBitfield();
	}

	return 0;
}

SiegeNode* SiegeEngine::IsNodeInAnyFrustum( const database_guid& nodeGuid )
{
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		return( (pNode->GetFrustumOwnedBitfield() != 0) ? pNode : NULL );
	}

	return NULL;
}

SiegeNode* SiegeEngine::IsNodeValid( const database_guid& nodeGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( nodeGuid.IsValid() )
	{
		// Attempt to get the node
		SiegeNode* pNode = m_pNodeCache->GetValidObjectInCache( nodeGuid );
		if( pNode )
		{
			return( pNode->IsLoadedPhysical() ? pNode : NULL );
		}
	}

	return NULL;
}

SiegeNode* SiegeEngine::DoesNodeExist( const database_guid& nodeGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( nodeGuid.IsValid() )
	{
		// Attempt to get the node
		return( m_pNodeCache->GetValidObjectInCache( nodeGuid ) );
	}

	return NULL;
}

bool SiegeEngine::SpaceDiffers( const database_guid& nearGuid, const database_guid& farGuid )
{
	CHECK_SIEGE_PRIMARY_THREAD_ONLY;

	if( nearGuid.IsValid() && farGuid.IsValid() )
	{
		// Attempt to get the node
		SiegeNode* pNearNode	= m_pNodeCache->GetValidObjectInCache( nearGuid );
		SiegeNode* pFarNode		= m_pNodeCache->GetValidObjectInCache( farGuid );
		if( pNearNode && pFarNode )
		{
			if( pNearNode->GetSpaceFrustum() == pFarNode->GetSpaceFrustum() )
			{
				return false;
			}
		}
	}

	return true;
}

void SiegeEngine::NodeFade( const database_guid& nodeGuid, FADETYPE ft )
{
	// Don't do anything in this case
	if( ft == FT_NONE )
	{
		gpwarning( "Cannot attach node fade with fade type FT_NONE" );
		return;
	}

	// See if a fade with this identifier already exists
	SingleNodeFadeMap::iterator i = m_singleNodeFadeMap.find( nodeGuid );
	if( i != m_singleNodeFadeMap.end() )
	{
		if( (*i).second.m_fadeType == ft )
		{
			return;
		}
		else
		{
			(*i).second.m_fadeType	= ft;
			(*i).second.m_startTime	= ::GetGlobalSeconds();
		}
	}
	else
	{
		// Build a node fade structure to hold the information we will need about this
		SINGLENODEFADE nf;
		nf.m_fadeType	= ft;
		nf.m_alpha		= 0xFF;
		nf.m_startTime	= ::GetGlobalSeconds();

		// Add it to our list
		m_singleNodeFadeMap.insert( std::pair< database_guid, SINGLENODEFADE >( nodeGuid, nf ) );
	}
}

// Setup a global node fade
void SiegeEngine::GlobalNodeFade( int regionID, int section, int level, int object, FADETYPE ft )
{
	// Don't do anything in this case
	if( ft == FT_NONE )
	{
		gpwarning( "Cannot attach node fade with fade type FT_NONE" );
		return;
	}

	// See if a fade with this identifier already exists
	NodeFadeColl::iterator i = stdx::binary_search( m_globalNodeFadeColl.begin(), m_globalNodeFadeColl.end(), regionID, fade_less() );
	for( ; i != m_globalNodeFadeColl.end(); ++i )
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
	m_globalNodeFadeColl.push_back( nf );
	std::sort( m_globalNodeFadeColl.begin(), m_globalNodeFadeColl.end(), fade_less() );
}

// Fade updating
void SiegeEngine::UpdateFades()
{
	// Get the current time
	double currentTime	= ::GetGlobalSeconds();

	// Go through the list of single node fades and update them
	for( SingleNodeFadeMap::iterator s = m_singleNodeFadeMap.begin(); s != m_singleNodeFadeMap.end(); )
	{
		SINGLENODEFADE&	snf	= (*s).second;

		// See if this fade is done and we can remove it
		if( (snf.m_fadeType == FT_IN) && (snf.m_alpha == 0xFF) )
		{
			s = m_singleNodeFadeMap.erase( s );
			continue;
		}
		else if( ((snf.m_fadeType == FT_BLACK) && (snf.m_alpha == m_NodeFadeBlack)) ||
				 ((snf.m_fadeType == FT_ALPHA) && (snf.m_alpha == m_NodeFadeAlpha)) )
		{
			++s;
			continue;
		}
		else if( snf.m_fadeType == FT_INSTANT )
		{
			snf.m_alpha	= 0;
			++s;
			continue;
		}
		else if( snf.m_fadeType == FT_IN_INSTANT )
		{
			s = m_singleNodeFadeMap.erase( s );
			continue;
		}

		// Update this fade
		float interp	= min_t( 1.0f, ((float)(PreciseSubtract(currentTime, snf.m_startTime)) / m_NodeFadeTime) );

		if( snf.m_fadeType == FT_IN )
		{
			snf.m_alpha = (BYTE)FTOL( interp * 255.0f );
		}
		else if( snf.m_fadeType == FT_ALPHA )
		{
			snf.m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - m_NodeFadeAlpha) ));
		}
		else if( snf.m_fadeType == FT_BLACK )
		{
			snf.m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - m_NodeFadeBlack) ));
		}

		++s;
	}

	// Go through the list of node fades and update them
	for( NodeFadeColl::iterator i = m_globalNodeFadeColl.begin(); i != m_globalNodeFadeColl.end(); )
	{
		// See if this fade is done and we can remove it
		if( ((*i).m_fadeType == FT_IN) && ((*i).m_alpha == 0xFF) )
		{
			i = m_globalNodeFadeColl.erase( i );
			continue;
		}
		else if( (((*i).m_fadeType == FT_BLACK) && ((*i).m_alpha == m_NodeFadeBlack)) ||
				 (((*i).m_fadeType == FT_ALPHA) && ((*i).m_alpha == m_NodeFadeAlpha)) )
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
			i = m_globalNodeFadeColl.erase( i );
			continue;
		}

		// Update this fade
		float interp	= min_t( 1.0f, ((float)(PreciseSubtract(currentTime, (*i).m_startTime)) / m_NodeFadeTime) );

		if( (*i).m_fadeType == FT_IN )
		{
			(*i).m_alpha = (BYTE)FTOL( interp * 255.0f );
		}
		else if( (*i).m_fadeType == FT_ALPHA )
		{
			(*i).m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - m_NodeFadeAlpha) ));
		}
		else if( (*i).m_fadeType == FT_BLACK )
		{
			(*i).m_alpha = (BYTE)(0xFF - FTOL( interp * (float)(0xFF - m_NodeFadeBlack) ));
		}

		++i;
	}
}

// Update a node's alpha fade level
void SiegeEngine::UpdateFadedNodeAlpha( SiegeNode& node )
{
	SingleNodeFadeMap::const_iterator s = m_singleNodeFadeMap.find( node.GetGUID() );
	if( s != m_singleNodeFadeMap.end() )
	{
		node.SetNodeAlpha( (*s).second.m_alpha );
	}
	else
	{
		// Go through the current fades and see if any match
		NodeFadeColl::iterator i = stdx::binary_search( m_globalNodeFadeColl.begin(), m_globalNodeFadeColl.end(), node.GetRegionID(), fade_less() );
		for( ; i != m_globalNodeFadeColl.end(); ++i )
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

		// Get the render frustum
		SiegeFrustum* pFrustum = GetFrustum( m_renderFrustum );
		if( pFrustum )
		{
			pFrustum->UpdateFadedNodeAlpha( node );
		}
	}
}

// Change managed node flags
void SiegeEngine::SetNodeOccludesCamera( const database_guid& nodeGuid, bool bOccludes )
{
	// Change the state of the node
	SiegeNode* pNode	= DoesNodeExist( nodeGuid );
	if( pNode )
	{
		pNode->SetOccludesCamera( bOccludes );
	}

	// Check our database
	NodeFlagMap::iterator i = m_nodeFlagMap.find( nodeGuid );
	if( i != m_nodeFlagMap.end() )
	{
		(*i).second.m_bOccludesCamera	= bOccludes;
		(*i).second.m_bOccludesChanged	= true;
	}
	else
	{
		NODEFLAGS flags;
		flags.m_bOccludesCamera			= bOccludes;
		flags.m_bOccludesChanged		= true;
		flags.m_bBoundsCamera			= false;
		flags.m_bBoundsChanged			= false;
		flags.m_bCameraFade				= false;
		flags.m_bCameraChanged			= false;

		m_nodeFlagMap.insert( std::pair< database_guid, NODEFLAGS >( nodeGuid, flags ) );
	}
}

void SiegeEngine::SetNodeBoundsCamera( const database_guid& nodeGuid, bool bBounds )
{
	// Change the state of the node
	SiegeNode* pNode	= DoesNodeExist( nodeGuid );
	if( pNode )
	{
		pNode->SetBoundsCamera( bBounds );
	}

	// Check our database
	NodeFlagMap::iterator i = m_nodeFlagMap.find( nodeGuid );
	if( i != m_nodeFlagMap.end() )
	{
		(*i).second.m_bBoundsCamera		= bBounds;
		(*i).second.m_bBoundsChanged	= true;
	}
	else
	{
		NODEFLAGS flags;
		flags.m_bOccludesCamera			= false;
		flags.m_bOccludesChanged		= false;
		flags.m_bBoundsCamera			= bBounds;
		flags.m_bBoundsChanged			= true;
		flags.m_bCameraFade				= false;
		flags.m_bCameraChanged			= false;

		m_nodeFlagMap.insert( std::pair< database_guid, NODEFLAGS >( nodeGuid, flags ) );
	}
}

void SiegeEngine::SetNodeCameraFade( const database_guid& nodeGuid, bool bFade )
{
	// Change the state of the node
	SiegeNode* pNode	= DoesNodeExist( nodeGuid );
	if( pNode )
	{
		pNode->SetCameraFade( bFade );
	}

	// Check our database
	NodeFlagMap::iterator i = m_nodeFlagMap.find( nodeGuid );
	if( i != m_nodeFlagMap.end() )
	{
		(*i).second.m_bCameraFade		= bFade;
		(*i).second.m_bCameraChanged	= true;
	}
	else
	{
		NODEFLAGS flags;
		flags.m_bOccludesCamera			= false;
		flags.m_bOccludesChanged		= false;
		flags.m_bBoundsCamera			= false;
		flags.m_bBoundsChanged			= false;
		flags.m_bCameraFade				= bFade;
		flags.m_bCameraChanged			= true;

		m_nodeFlagMap.insert( std::pair< database_guid, NODEFLAGS >( nodeGuid, flags ) );
	}
}

// Fill in the managed flags for this node
void SiegeEngine::GetNodeFlags( SiegeNode* pNode )
{
	// Check our database
	NodeFlagMap::iterator i = m_nodeFlagMap.find( pNode->GetGUID() );
	if( i != m_nodeFlagMap.end() )
	{
		// Fill in the flags for this node
		if( (*i).second.m_bOccludesChanged )
		{
			pNode->SetOccludesCamera( (*i).second.m_bOccludesCamera );
		}
		if( (*i).second.m_bBoundsChanged )
		{
			pNode->SetBoundsCamera( (*i).second.m_bBoundsCamera );
		}
		if( (*i).second.m_bCameraChanged )
		{
			pNode->SetCameraFade( (*i).second.m_bCameraFade );
		}
	}
}

// Change nodal texture animation state
void SiegeEngine::SetNodalTextureAnimationState( const database_guid& nodeGuid, int texIndex, bool bState )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	// Get the node
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		// Update the state of this texture
		gDefaultRapi.SetTextureAnimationState( pNode->GetTextureListing()[ texIndex ].textureId, bState );
	}

	std::pair< NodeTexMap::iterator, NodeTexMap::iterator > tm = m_nodeTexMap.equal_range( nodeGuid );

	// Go through each discovery position in this node
	for( ; tm.first != tm.second ; ++tm.first )
	{
		if( (*tm.first).second.m_textureIndex == texIndex )
		{
			(*tm.first).second.m_bStateChanged	= true;
			(*tm.first).second.m_bState			= bState;
			return;
		}
	}

	// Not in collection, need to add it
	NODETEXSTATE nodeTex;
	nodeTex.m_textureIndex	= texIndex;
	nodeTex.m_bStateChanged	= true;
	nodeTex.m_bState		= bState;
	nodeTex.m_bSpeedChanged	= false;
	nodeTex.m_bSpeed		= 0.0f;
	nodeTex.m_bReplaced		= false;

	m_nodeTexMap.insert( std::pair< database_guid, NODETEXSTATE >( nodeGuid, nodeTex ) );
}

bool SiegeEngine::GetNodalTextureAnimationState( const database_guid& nodeGuid, int texIndex )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	std::pair< NodeTexMap::const_iterator, NodeTexMap::const_iterator > tm = m_nodeTexMap.equal_range( nodeGuid );

	// Go through each discovery position in this node
	for( ; tm.first != tm.second ; ++tm.first )
	{
		if( (*tm.first).second.m_textureIndex == texIndex &&
			(*tm.first).second.m_bStateChanged )
		{
			return( (*tm.first).second.m_bState );
		}
	}

	// Get the node
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		// Update the state of this texture
		return( gDefaultRapi.GetTextureAnimationState( pNode->GetTextureListing()[ texIndex ].textureId ) );
	}

	// Not in list and also not loaded
	gperrorf(( "Node texture animation state was requested for invalid node 0x%08x index %d!", nodeGuid.GetValue(), texIndex ));
	return false;
}

// Change nodal texture animation speed
void SiegeEngine::SetNodalTextureAnimationSpeed( const database_guid& nodeGuid, int texIndex, float speed )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	// Get the node
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		// Update the speed of this texture
		gDefaultRapi.SetTextureAnimationSpeed( pNode->GetTextureListing()[ texIndex ].textureId, speed );
	}

	std::pair< NodeTexMap::iterator, NodeTexMap::iterator > tm = m_nodeTexMap.equal_range( nodeGuid );

	// Go through each texture
	for( ; tm.first != tm.second ; ++tm.first )
	{
		if( (*tm.first).second.m_textureIndex == texIndex )
		{
			(*tm.first).second.m_bSpeedChanged	= true;
			(*tm.first).second.m_bSpeed			= speed;
			return;
		}
	}

	// Not in collection, need to add it
	NODETEXSTATE nodeTex;
	nodeTex.m_textureIndex	= texIndex;
	nodeTex.m_bStateChanged	= false;
	nodeTex.m_bState		= false;
	nodeTex.m_bSpeedChanged	= true;
	nodeTex.m_bSpeed		= speed;
	nodeTex.m_bReplaced		= false;

	m_nodeTexMap.insert( std::pair< database_guid, NODETEXSTATE >( nodeGuid, nodeTex ) );
}

float SiegeEngine::GetNodalTextureAnimationSpeed( const database_guid& nodeGuid, int texIndex )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	std::pair< NodeTexMap::const_iterator, NodeTexMap::const_iterator > tm = m_nodeTexMap.equal_range( nodeGuid );

	// Go through each texture
	for( ; tm.first != tm.second ; ++tm.first )
	{
		if( (*tm.first).second.m_textureIndex == texIndex &&
			(*tm.first).second.m_bSpeedChanged )
		{
			return( (*tm.first).second.m_bSpeed );
		}
	}

	// Get the node
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		// Update the state of this texture
		return( gDefaultRapi.GetTextureAnimationSpeed( pNode->GetTextureListing()[ texIndex ].textureId ) );
	}

	// Not in list and also not loaded
	gperrorf(( "Node texture animation speed was requested for invalid node 0x%08x index %d!", nodeGuid.GetValue(), texIndex ));
	return 0.0f;
}

// Change nodal texture
void SiegeEngine::ReplaceNodalTexture( const database_guid& nodeGuid, int texIndex, const gpstring& newTexName )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	// Get the node
	SiegeNode* pNode	= IsNodeValid( nodeGuid );
	if( pNode )
	{
		// Replace the texture
		pNode->ReplaceTexture( texIndex, newTexName, true );
	}

	std::pair< NodeTexMap::iterator, NodeTexMap::iterator > tm = m_nodeTexMap.equal_range( nodeGuid );

	// Go through each texture
	for( ; tm.first != tm.second ; ++tm.first )
	{
		if( (*tm.first).second.m_textureIndex == texIndex )
		{
			(*tm.first).second.m_bReplaced			= true;
			(*tm.first).second.m_replacementTexture	= newTexName;
			return;
		}
	}

	// Not in collection, need to add it
	NODETEXSTATE nodeTex;
	nodeTex.m_textureIndex			=	 texIndex;
	nodeTex.m_bStateChanged			= false;
	nodeTex.m_bState				= false;
	nodeTex.m_bSpeedChanged			= false;
	nodeTex.m_bSpeed				= 0.0f;
	nodeTex.m_bReplaced				= true;
	nodeTex.m_replacementTexture	= newTexName;

	m_nodeTexMap.insert( std::pair< database_guid, NODETEXSTATE >( nodeGuid, nodeTex ) );
}

// Process the texture changes for this node
void SiegeEngine::ProcessNodalTextureStates( SiegeNode& node, bool bFullLoad )
{
	kerneltool::Critical::Lock autoLock( m_nodeTexCritical );

	std::pair< NodeTexMap::const_iterator, NodeTexMap::const_iterator > tm = m_nodeTexMap.equal_range( node.GetGUID() );

	// Go through each discovery position in this node
	for( ; tm.first != tm.second ; ++tm.first )
	{
		const NODETEXSTATE& texState	= (*tm.first).second;

		// Process replacement
		if( texState.m_bReplaced )
		{
			// Replace the texture
			node.ReplaceTexture( texState.m_textureIndex, texState.m_replacementTexture, bFullLoad );
		}

		// Process state
		if( texState.m_bStateChanged )
		{
			// Update the state of this texture
			gDefaultRapi.SetTextureAnimationState( node.GetTextureListing()[ texState.m_textureIndex ].textureId, texState.m_bState );
		}

		// Process speed
		if( texState.m_bSpeedChanged )
		{
			// Update the speed of this texture
			gDefaultRapi.SetTextureAnimationSpeed( node.GetTextureListing()[ texState.m_textureIndex ].textureId, texState.m_bSpeed );
		}
	}
}

// Convert a SiegePos to world space
vector_3 SiegePos::WorldPos() const
{
	const siege::SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( node );
	const siege::SiegeNode& snode		= handle.RequestObject( gSiegeEngine.NodeCache() );

	return snode.NodeToWorldSpace( pos );
}

// Make a SiegePos from world space
void SiegePos::FromWorldPos( const vector_3& worldpos, const siege::database_guid& nnode )
{
	node	= nnode;

	const siege::SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( node );
	const siege::SiegeNode& snode		= handle.RequestObject( gSiegeEngine.NodeCache() );

	pos		= snode.WorldToNodeSpace( worldpos );
}

// Convert a SiegePos to screen space
vector_3 SiegePos::ScreenPos() const
{
	// Get the screen position of this world pos from Rapi
	return gDefaultRapi.GetScreenPosition( WorldPos() );
}

const SiegePos SiegePos::INVALID( vector_3::ZERO, UNDEFINED_GUID );
const SiegeRot SiegeRot::INVALID( Quat::ZERO, UNDEFINED_GUID );
