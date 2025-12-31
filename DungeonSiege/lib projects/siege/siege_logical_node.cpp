/***************************************************************************************
**
**								SiegeLogicalNode
**
**		Siege logical nodes are responsible for maintaining all basic information
**		about Siege nodes, including a simple geometry set, extensive connection
**		information, occupants, etc.
**
**		Author:		James Loe
**		Date:		11/17/99
**
***************************************************************************************/

#include "precomp_siege.h"
#include "gpcore.h"
#include "siege_logical_mesh.h"
#include "siege_logical_node.h"
#include "stringtool.h"

using namespace siege;
using namespace std;




static const stringtool::BitEnumStringConverter::Entry s_LogicalFlagsData[] =
{
	{ "lf_none"			   , LF_NONE		    },
	{ "lf_human_player"    , LF_HUMAN_PLAYER    },
	{ "lf_computer_player" , LF_COMPUTER_PLAYER },
	{ "lf_dirt"            , LF_DIRT            },
	{ "lf_shallow_water"   , LF_SHALLOW_WATER   },
	{ "lf_deep_water"      , LF_DEEP_WATER      },
	{ "lf_ice"             , LF_ICE             },
	{ "lf_lava"            , LF_LAVA            },
	{ "lf_mist"			   , LF_MIST			},

	{ "lf_size1_mover"     , LF_SIZE1_MOVER     },
	{ "lf_size2_mover"     , LF_SIZE2_MOVER     },
	{ "lf_size3_mover"     , LF_SIZE3_MOVER     },
	{ "lf_size4_mover"     , LF_SIZE4_MOVER     },

	{ "lf_hover"           , LF_HOVER           },
	{ "lf_boss"            , LF_BOSS			},

	{ "lf_clear"		   , LF_CLEAR			},
};

static stringtool::BitEnumStringConverter 
	s_LogicalFlagsConverter( s_LogicalFlagsData, ELEMENT_COUNT( s_LogicalFlagsData ), LF_NONE );

bool FromString( const char* str, eLogicalNodeFlags & q )
{
	return ( s_LogicalFlagsConverter.FromString( str, rcast <DWORD&> ( q ) ) );
}

bool FromFullString( const char* str, eLogicalNodeFlags& q )
{
	return ( s_LogicalFlagsConverter.FromFullString( str, rcast <DWORD&> ( q ) ) );
}

const char* ToString( eLogicalNodeFlags q )
{
	return ( s_LogicalFlagsConverter.ToString( q ) );
}

gpstring ToFullString( eLogicalNodeFlags q )
{
	return ( s_LogicalFlagsConverter.ToFullString( q ) );
}

//FUBI_EXPORT_IRREGULAR_ENUM( eLogicalNodeFlags );




// Constructor
SiegeLogicalNode::SiegeLogicalNode()
	: m_pLogicalMesh( NULL )
	, m_pSiegeNode( NULL )
	, m_id( 0 )
	, m_flags( 0 )
	, m_numNodeConnections( 0 )
	, m_pNodeConnectionInfo( NULL )
	, m_pathSearchIndex( 0xFFFFFFFF )
	, m_bActivePath( false )
{
}

// Destructor
SiegeLogicalNode::~SiegeLogicalNode()
{
	// Destroy the node connection information
	if( m_pNodeConnectionInfo )
	{
		// Go through all of the leaf connection info
		for( unsigned int i = 0; i < m_numNodeConnections; ++i )
		{
			if( m_pNodeConnectionInfo[i].m_farSiegeNode != m_pSiegeNode->GetGUID() )
			{
				delete[]	m_pNodeConnectionInfo[i].m_pCollection->m_pNodalLeafConnections;
				delete		m_pNodeConnectionInfo[i].m_pCollection;
			}
		}

		delete[] m_pNodeConnectionInfo;
	}
}

// Load logical node information from a file
bool SiegeLogicalNode::Load( SiegeNode* pSiegeNode, const char* &pData )
{
	// Store off the owner SiegeNode guid
	m_pSiegeNode			= pSiegeNode;
	gpassert( m_pSiegeNode->GetGUID().IsValid() );

	// Load in information about this logical node
	m_id					= *((unsigned char *)pData);
	pData					+= sizeof( unsigned char );

	m_flags					= *((unsigned int *)pData);
	pData					+= sizeof( unsigned int );

	m_numNodeConnections	= *((unsigned short *)pData);
	pData					+= sizeof( unsigned short );

	// Build a list of our connections and fill it in
	m_pNodeConnectionInfo	= new LNODECONNECT[ m_numNodeConnections ];

	for( unsigned int i = 0; i < m_numNodeConnections; ++i )
	{
		// Get a pointer to our connection
		LNODECONNECT* pConnection	= &m_pNodeConnectionInfo[ i ];

		// Siege node that owns this object
		pConnection->m_farSiegeNode	= *((database_guid*)pData);
		pData						+= sizeof( database_guid );

		// Create new connection pointer
		LCCOLLECTION* pCollection = pConnection->m_pCollection = new LCCOLLECTION;

		// Unique identifier of node (corresponds to logical mesh id)
		pCollection->m_farid		= *((unsigned char*)pData);
		pData						+= sizeof( unsigned char );

		// List of leaf connections
		pCollection->m_numNodalLeafConnections	= *((unsigned int *)pData);
		pData						+= sizeof( unsigned int );

		pCollection->m_pNodalLeafConnections	= new NODALLEAFCONNECT[ pCollection->m_numNodalLeafConnections ];
		memcpy( pCollection->m_pNodalLeafConnections, pData, sizeof( NODALLEAFCONNECT ) * pCollection->m_numNodalLeafConnections );

		pData						+= sizeof( NODALLEAFCONNECT ) * pCollection->m_numNodalLeafConnections;
	}

	// Reset logical flags to reflect the true meaning of LF_ALL
	if( m_flags & 0x00000001 )
	{
		m_flags &= ~0x00000001;
		m_flags |= LF_HUMAN_PLAYER | LF_COMPUTER_PLAYER | LF_DIRT;
	}

	// Check for the clear flag
	if( m_flags & LF_CLEAR )
	{
		m_flags	= 0;
	}

#if !GP_RETAIL
	if( m_flags & LF_IS_WATER )
	{
		if( m_flags & LF_HUMAN_PLAYER )
		{
			gperrorf(( "Node %s has water polys marked as human player!", pSiegeNode->GetGUID().ToString().c_str() ));
		}
		if( m_flags & LF_DIRT )
		{
			gperrorf(( "Node %s has water polys marked as dirt!", pSiegeNode->GetGUID().ToString().c_str() ));
		}
	}
#endif

	return true;
}

// Trace a ray into this logical node
// The ray needs to be in the node space coordinates of it's owner siege node.
bool SiegeLogicalNode::HitTest( const vector_3& ray_orig, const vector_3& ray_dir,
								float& ray_t, vector_3& facenormal )
{
	// This function is merely a stub that is passed directly to the BSPTree
	if( m_pLogicalMesh )
	{
		return m_pLogicalMesh->HitTest( ray_orig, ray_dir, ray_t, facenormal );
	}

	return false;
}

bool SiegeLogicalNode::HitTestTri( const vector_3& ray_orig, const vector_3& ray_dir,
								   float& ray_t, TriNorm& triangle )
{
	// This function is merely a stub that is passed directly to the BSPTree
	if( m_pLogicalMesh )
	{
		return m_pLogicalMesh->HitTestTri( ray_orig, ray_dir, ray_t, triangle );
	}

	return false;
}

// Test the given flags against a set of rules used to determine permissions
bool SiegeLogicalNode::AreFlagsAllowable( unsigned int flags )
{
	// Get the combination of flags
	unsigned int testFlags	= flags & m_flags;

	// Check the player mask
	if( !(testFlags & LF_PLAYER_MASK) )
	{
		return false;
	}

	// Check the material mask
	if( !(testFlags & LF_MATERIAL_MASK) )
	{
		return false;
	}

	// Check the type flags if any are set
	if( (flags & LF_TYPE_MASK) != (testFlags & LF_TYPE_MASK) )
	{
		return false;
	}

	return true;
}

// Set this logical node's logical mesh and update pointers
void SiegeLogicalNode::SetLogicalMesh( SiegeLogicalMesh* plMesh )
{
	// First, save off the mesh pointer
	m_pLogicalMesh = plMesh;

	if( plMesh->GetNumNodalConnections() )
	{
		// Build up the new list
		unsigned int numNewNodeConnections		= m_numNodeConnections + plMesh->GetNumNodalConnections();
		LNODECONNECT* pNewNodeConnectionInfo	= new LNODECONNECT[ numNewNodeConnections ];

		memcpy( pNewNodeConnectionInfo, m_pNodeConnectionInfo, sizeof( LNODECONNECT ) * m_numNodeConnections );
		delete[] m_pNodeConnectionInfo;

		for( unsigned int i = 0; i < plMesh->GetNumNodalConnections(); ++i )
		{
			pNewNodeConnectionInfo[ i + m_numNodeConnections ].m_farSiegeNode	= m_pSiegeNode->GetGUID();
			pNewNodeConnectionInfo[ i + m_numNodeConnections ].m_pCollection	= &plMesh->GetNodalConnectionInfo()[ i ];
		}

		m_pNodeConnectionInfo	= pNewNodeConnectionInfo;
		m_numNodeConnections	= (unsigned short)numNewNodeConnections;
	}

	// Add in any missing flags

	// We shouldn't need this line anymore now that users can set the logical flag permissions.
	// m_flags |= plMesh->GetFlags();
}

// Mark leaves as blocked
void SiegeLogicalNode::MarkLeafAsBlocked( unsigned short id, bool bBlocked )
{
	BlockedLeafMap::iterator i = m_BlockedLeafMap.find( id );
	if( i != m_BlockedLeafMap.end() )
	{
		if( bBlocked )
		{
			(*i).second++;
		}
		else
		{
			(*i).second--;
			if( (*i).second <= 0 )
			{
				m_BlockedLeafMap.erase( i );
			}
		}
	}
	else
	{
		if( bBlocked )
		{
			m_BlockedLeafMap.insert( std::pair< unsigned short, unsigned int >( id, 1 ) );
		}
	}
}

// Is leaf with the given id marked as blocked?
bool SiegeLogicalNode::IsLeafBlocked( unsigned short id )
{
	return( m_BlockedLeafMap.find( id ) != m_BlockedLeafMap.end() );
}

// Is this node connected to me?
bool SiegeLogicalNode::IsConnectedTo( const SiegeLogicalNode* pNode )
{
	for( unsigned int i = 0; i < m_numNodeConnections; ++i )
	{
		if( (m_pNodeConnectionInfo[ i ].m_farSiegeNode			== pNode->GetSiegeNode()->GetGUID()) &&
			(m_pNodeConnectionInfo[ i ].m_pCollection->m_farid	== pNode->GetID()) )
		{
			return true;
		}
	}

	return false;
}
