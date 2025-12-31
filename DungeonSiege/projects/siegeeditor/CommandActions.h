//////////////////////////////////////////////////////////////////////////////
//
// File     :  CommandActions.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __COMMAND_ACTIONS_H
#define __COMMAND_ACTIONS_H

#include "siege_pos.h"

// This type lets us know the type of action
enum eEditorActionType
{
	ACTION_MOVE,
	ACTION_ADDNODE,
	ACTION_DELETENODE,
	ACTION_ADDOBJECT,
	ACTION_DELETEOBJECT,
};


// This base class holds the most basic unit, which is a single action 
// execute carries out the action held within
class CommandAction
{
public:

	CommandAction( eEditorActionType type ) { m_type = type; }	
	~CommandAction() {}

	virtual void Execute() {}

private:

	eEditorActionType m_type;

};


// This is the move command, which is a command action which stores a position to move a game object
class CommandMove : public CommandAction
{
public:

	CommandMove( eEditorActionType type, int object, SiegePos spos, bool bLight );

	virtual void Execute();	
	
private:

	int					m_object;
	SiegePos			m_spos;
	bool				m_bLight;
};


// This is the node addition command
class CommandAddNode : public CommandAction
{
public:

	CommandAddNode( eEditorActionType type, siege::database_guid nodeGUID, siege::database_guid meshGUID,
					siege::database_guid destGUID, int sourceDoor, int destDoor );

	virtual void Execute();

private:

	siege::database_guid	m_nodeGUID;
	siege::database_guid	m_meshGUID;
	siege::database_guid	m_destGUID;
	int						m_sourceDoor;
	int						m_destDoor;
};


// Node deletion command
class CommandDeleteNode : public CommandAction
{
public:

	CommandDeleteNode( eEditorActionType type, siege::database_guid nodeGUID );

	virtual void Execute();

private:

	siege::database_guid	m_nodeGUID;
};


// Command actions is the storage base of all the commands for a single set of work.  For example, if someone
// had 5 objects selected and moved them at the same time, we can store all five move commands in a CommandActions
// because it is a single set of work

typedef std::vector< CommandAction * > CommandActionVec;

class CommandActions 
{
public:

	CommandActions() { m_refCount = 0; }
	~CommandActions();	
	void Insert( CommandAction * pCAction );
	void Undo();

	CommandActionVec m_commandActions;

	void IncRefCount() { ++m_refCount; }
	void DecRefCount() { --m_refCount; }
	int	 GetRefCount() { return m_refCount; }

private:

	int m_refCount;

};


// Type definitions
typedef std::list< CommandActions * > CommandActionsList;


// This is the big daddy of all the classes.  It stores all the CommandActions and it will be what we call to 
// undo the last CommandActions
class CommandManager : public Singleton <CommandManager>
{
public:

	CommandManager();
	~CommandManager();

	void BeginActionSet();
	void InsertAction( CommandAction * pCAction );
	bool DoesActionSetExist() { return ( m_pCurrentSet ? true : false ); }
	void EndActionSet();

	void Insert( CommandActions * pCActions );	
	void Undo();

private:

	CommandActionsList	m_commandSets;
	CommandActions		* m_pCurrentSet;

};



#define gCommandManager CommandManager::GetSingleton()


#endif 

