/***************************************************************************************
**
**								SiegeHotpointDatabase
**
**		See .h file for details
**
***************************************************************************************/

#include "precomp_siege.h"
#include "siege_hotpoint_database.h"

using namespace siege;


// Construction
SiegeHotpointDatabase::SiegeHotpointDatabase()
	: m_pActiveHotpoint( NULL )
{
}

// Destruction
SiegeHotpointDatabase::~SiegeHotpointDatabase()
{
}

// Clear the hotpoint database
void SiegeHotpointDatabase::Clear()
{
	// Clear the hotpoint list
	m_HotpointList.clear();

	// Clear active information
	m_pActiveHotpoint		= NULL;
	m_activeHotpointNode	= UNDEFINED_GUID;
}

// Insert a new hotpoint
void SiegeHotpointDatabase::InsertHotpoint( const gpwstring& name, const unsigned int id, bool bAvailable )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) || (*i).m_Id == id )
		{
			gperrorf( ("Hotpoint of name: %s and/or id: 0x%08X cannot be inserted because a hotpoint with that name and/or id already exists!", name.c_str(), id) );
			return;
		}
	}

	SiegeHotpoint hotpoint;
	hotpoint.m_Name			= name;
	hotpoint.m_Id			= id;
	hotpoint.m_bAvailable	= bAvailable;

	m_HotpointList.push_back( hotpoint );
}

// Remove a hotpoint
void SiegeHotpointDatabase::RemoveHotpoint( const unsigned int id )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Id == id )
		{
			m_HotpointList.erase( i );
			return;
		}
	}
}

// Set hotpoint as available (the user can see it from their drop down menu)
void SiegeHotpointDatabase::SetHotpointAvailable( const gpwstring& name, const bool bAvailable )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			(*i).m_bAvailable = bAvailable;
			return;
		}
	}
}

void SiegeHotpointDatabase::SetHotpointAvailable( const unsigned short id, const bool bAvailable )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Id == id )
		{
			(*i).m_bAvailable = bAvailable;
			return;
		}
	}
}

// Set hotpoint as active
void SiegeHotpointDatabase::SetHotpointActive( const gpwstring& name )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			m_pActiveHotpoint = &(*i);
			return;
		}
	}
}

void SiegeHotpointDatabase::SetHotpointActive( const unsigned short id )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Id == id )
		{
			m_pActiveHotpoint = &(*i);
			return;
		}
	}
}

// Does the currently active hotpoint node match the location of the hotpoint?
bool SiegeHotpointDatabase::ArrivedAtHotpoint()
{
	if( m_pActiveHotpoint )
	{
		SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( m_activeHotpointNode );
		SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

		return( IsZero( node.GetHotpointDirection( m_pActiveHotpoint->m_Id ) ) );
	}

	// If there is no active hotpoint, then you are considered to always have arrived
	return true;
}

// Get active hotpoint information
vector_3 SiegeHotpointDatabase::GetActiveHotpointDirection()
{
	if( m_pActiveHotpoint )
	{
		SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( m_activeHotpointNode );
		SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

		return( node.GetCurrentOrientation() * node.GetHotpointDirection( m_pActiveHotpoint->m_Id ) );
	}

	return vector_3::ZERO;
}

// Get a named hotpoint's information
vector_3 SiegeHotpointDatabase::GetHotpointDirection( const wchar_t* name, const database_guid& node )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( node );
			SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

			return( node.GetCurrentOrientation() * node.GetHotpointDirection( (*i).m_Id ) );
		}
	}

	return vector_3::ZERO;
}

vector_3 SiegeHotpointDatabase::GetNodalHotpointDirection( const wchar_t* name, const database_guid& node )
{
	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		if( (*i).m_Name.same_no_case( name ) )
		{
			SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( node );
			SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

			return( node.GetHotpointDirection( (*i).m_Id ) );
		}
	}

	return vector_3::ZERO;
}

// Set a named hotpoint's information
void SiegeHotpointDatabase::UpdateHotpointDirections( const database_guid& from_node, const database_guid& to_node )
{
	SiegeNodeHandle fhandle	= gSiegeEngine.NodeCache().UseObject( from_node );
	SiegeNode& fnode		= fhandle.RequestObject( gSiegeEngine.NodeCache() );

	SiegeNodeHandle thandle	= gSiegeEngine.NodeCache().UseObject( to_node );
	SiegeNode& tnode		= thandle.RequestObject( gSiegeEngine.NodeCache() );

	for( SiegeHotpointList::iterator i = m_HotpointList.begin(); i != m_HotpointList.end(); ++i )
	{
		tnode.SetHotpointDirection( (*i).m_Id,
									tnode.GetTransposeOrientation() *
									(fnode.GetCurrentOrientation() * fnode.GetHotpointDirection( (*i).m_Id )) );
	}
}