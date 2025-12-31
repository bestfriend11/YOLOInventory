#pragma once
#ifndef _SIEGE_HOTPOINT_DATABASE_H_
#define _SIEGE_HOTPOINT_DATABASE_H_

/***************************************************************************************
**
**								SiegeHotpointDatabase
**
**		This class is responsible for maintaining a list of the game's hotpoints
**		and keeping track of state and locality information.
**
**		Author:  James Loe
**		Date:	 09/05/00
**
***************************************************************************************/

namespace siege
{
	struct SiegeHotpoint
	{
		// Identification
		gpwstring		m_Name;
		unsigned int	m_Id;

		// Status
		bool			m_bAvailable;
	};

	typedef std::list< SiegeHotpoint >	SiegeHotpointList;


	class SiegeHotpointDatabase : public Singleton< SiegeHotpointDatabase >
	{
	public:

		// Construction and destruction
		SiegeHotpointDatabase();
		~SiegeHotpointDatabase();

		// Clear the hotpoint database
		void Clear();

		// Insert a hotpoint into the database
		void InsertHotpoint( const gpwstring& name, const unsigned int id, bool bAvailable = false );

		// Remove a hotpoint from the database
		void RemoveHotpoint( const unsigned int id );

		// Set hotpoint as available (the user can see it from their drop down menu)
		void SetHotpointAvailable( const gpwstring& name, const bool bAvailable );
		void SetHotpointAvailable( const unsigned short id, const bool bAvailable );

		// Set hotpoint as active
		void SetHotpointActive( const gpwstring& name );
		void SetHotpointActive( const unsigned short id );

		// Is a hotpoint currently active?
		bool IsHotpointActive()												{ return( m_pActiveHotpoint != NULL ); }

		// Set the node that you would like to get hotpoint direction from
		void SetActiveHotpointNode( const database_guid& node )				{ m_activeHotpointNode = node; }

		// Does the currently active hotpoint node match the location of the hotpoint?
		bool ArrivedAtHotpoint();

		// Get active hotpoint information
		vector_3 GetActiveHotpointDirection();

		// Get a named hotpoint's information
		vector_3 GetHotpointDirection( const wchar_t* name, const database_guid& node );
		vector_3 GetNodalHotpointDirection( const wchar_t* name, const database_guid& node );

		// Set a named hotpoint's information
		void UpdateHotpointDirections( const database_guid& from_node, const database_guid& to_node );

		// Get the list of hotpoints
		const SiegeHotpointList& GetHotpointList()							{ return m_HotpointList; }

	private:

		// List of hotpoints
		SiegeHotpointList	m_HotpointList;

		// Currently active hotpoint
		SiegeHotpoint*		m_pActiveHotpoint;

		// Node to base the direction of the currently active hotpoint
		database_guid		m_activeHotpointNode;
	};
}

#define gSiegeHotpointDatabase siege::SiegeHotpointDatabase::GetSingleton()

#endif