//////////////////////////////////////////////////////////////////////////////
//
// File     :  CommandActions.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "CommandActions.h"
#include "EditorLights.h"
#include "EditorTerrain.h"

using namespace siege;


// Command Move
CommandMove::CommandMove( eEditorActionType type, int object, SiegePos spos, bool bLight )
	: CommandAction( type )
{
	m_object	= object;
	m_spos		= spos;
	m_bLight	= bLight;
}


void CommandMove::Execute()
{
	if ( !m_bLight )
	{
		gGizmoManager.SetPosition( m_object, m_spos ); 
	}
	else
	{
		gEditorLights.SetLightPosition( siege::database_guid( m_object ), m_spos );
	}
}


// Command Add Node
CommandAddNode::CommandAddNode( eEditorActionType type, siege::database_guid nodeGUID, siege::database_guid meshGUID,
								siege::database_guid destGUID, int sourceDoor, int destDoor )
	: CommandAction( type )
{
	m_nodeGUID = nodeGUID;
	m_meshGUID = meshGUID;
	m_destGUID = destGUID;
	m_sourceDoor	= sourceDoor;
	m_destDoor		= destDoor;
}


void CommandAddNode::Execute()
{
	/*
	const siege::SiegeNodeList& hitTestList	= gSiegeEngine.NodeWalker().GetRenderList();
	if( !hitTestList.empty() )
	{
		for( siege::SiegeNodeList::const_iterator i = hitTestList.begin(); i != hitTestList.end(); ++i )
		{
			const siege::SiegeNode& node	= (*i).RequestObject( gSiegeEngine.NodeCache() );
			if ( node.GetGUID() == m_nodeGUID )
			{
				gEditorTerrain.AttachSiegeNodes( m_nodeGUID, m_meshGUID, siege::database_guid(), m_sourceDoor, m_destDoor );
				return;
			}
		}
	}
	*/

	gEditorTerrain.AttachSiegeNodes( m_nodeGUID, m_meshGUID, siege::database_guid(), m_sourceDoor, m_destDoor );
}



// Command Actions
CommandActions::~CommandActions()
{
	CommandActionVec::iterator i;
	for ( i = m_commandActions.begin(); i != m_commandActions.end(); ++i )	
	{
		delete (*i);
	}
	m_commandActions.clear();
}


void CommandActions::Insert( CommandAction * pCAction )
{
	m_commandActions.push_back( pCAction );
}


void CommandActions::Undo()
{
	CommandActionVec::reverse_iterator i;
	for ( i = m_commandActions.rbegin(); i != m_commandActions.rend(); ++i )	
	{
		(*i)->Execute();
	}
}



// Command Manager
CommandManager::CommandManager()
{
	m_pCurrentSet = 0;
}


CommandManager::~CommandManager()
{
	CommandActionsList::iterator i;
	for ( i = m_commandSets.begin(); i != m_commandSets.end(); ++i )
	{
		delete (*i);
	}
	m_commandSets.clear();
}


void CommandManager::Insert( CommandActions * pCActions )	
{
	m_commandSets.push_front( pCActions );
}


void CommandManager::Undo()
{
	if ( m_commandSets.size() != 0 )
	{
		CommandActionsList::iterator i = m_commandSets.begin();
		
		if ( i != m_commandSets.end() )
		{
			(*i)->Undo();
			delete (*i);
			m_commandSets.pop_front();
		}
	}
}


void CommandManager::BeginActionSet()
{
	if ( m_pCurrentSet == 0 )
	{
		CommandActions * pActions = new CommandActions();
		Insert( pActions );
		m_pCurrentSet = pActions;
	}
	m_pCurrentSet->IncRefCount();
}


void CommandManager::InsertAction( CommandAction * pCAction )
{
	if ( m_pCurrentSet )
	{
		m_pCurrentSet->Insert( pCAction );
	}
}


void CommandManager::EndActionSet()
{
	if ( m_pCurrentSet )
	{
		m_pCurrentSet->DecRefCount();
	
		if (( m_pCurrentSet->m_commandActions.size() == 0 ) && ( m_pCurrentSet->GetRefCount() == 0 ))
		{
			CommandActionsList::iterator i;
			for ( i = m_commandSets.begin(); i != m_commandSets.end(); ++i )
			{
				if ( (*i) == m_pCurrentSet )
				{
					delete (*i);
					m_commandSets.pop_front();					
					m_pCurrentSet = 0;
					return;
				}
			}
		}

		if ( m_pCurrentSet->GetRefCount() == 0 )
		{
			m_pCurrentSet = 0;
		}
	}
}
