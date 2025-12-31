/**************************************************************************

Filename		: UICommands.h

Class			: UICommands

Description		: Handles the command interface for the user 

Author			: Chad Queen


  (C)opyright Gas Powered Games 1999

**************************************************************************/



#pragma once
#ifndef _UICOMMAND_H_
#define _UICOMMAND_H_

#include "UIGame.h"


// Forward Declarations
class UICursor;

namespace siege {
	SiegePos;
};


enum eGuiContextCursor
{
	CURSOR_GUI_STANDARD,
	CURSOR_GUI_GRAB,
	CURSOR_GUI_SMASH,
	CURSOR_GUI_CAST,
	CURSOR_GUI_ATTACK,
	CURSOR_GUI_TALK,
	CURSOR_GUI_INITIATE,
	CURSOR_GUI_GUARD,
	CURSOR_GUI_CANT,
	CURSOR_GUI_ATTACK_CAST,
	CURSOR_GUI_WAIT,

	CURSOR_GUI_SIZE,
};

class UICommands : public Singleton <UICommands>
{
public:

	// Constructor
	UICommands();

	// Destructor
	~UICommands();

	// Cursor Initialization
	void InitCursors();

	// Commands
	void CommandAttack			( Goid target, bool noqueueclear = false, bool breakObject = false );
	void CommandAttack			( SiegePos spos, bool noqueueclear = false );
	void CommandAttack			( Go * pMember, Go * pObject, bool noqueueclear = false );

	void CommandCast			( Goid target, bool noqueueclear = false );
	void CommandCast			( SiegePos spos, bool noqueueclear = false );
	void CommandCast			( Go * pMember, Go * pObject, bool noqueueclear = false );

	void CommandGet				( Goid object, bool noqueueclear = false, ActionSource action_source = SOURCE_ACTOR, bool bForceGet = false );
	void CommandDrop			( Goid object, const SiegePos & siegepos, bool noqueueclear = false, bool bDeactivateItems = true, Go * pDropper = 0 );	
	void CommandEquip			( Goid object, const eEquipSlot slot, bool noqueueclear = false, bool bUseControl = true );
	void CommandEquip			( Goid object, const eEquipSlot slot, Goid actor, bool noqueueclear = false, bool bUseControl = true );
	void CommandUnequip			( const eEquipSlot slot, Goid actor = GOID_INVALID );	
	bool CommandGive			( Goid object, Goid target, bool noqueueclear = false );
	void CommandGuard			( Goid target, bool noqueueclear = false );
	void CommandMove			( const SiegePos & siegepos, bool patrol = false, bool noqueueclear = false );
	void CommandMove			();
	void InsertWaypoint			( const SiegePos & spos );
	void CommandPatrol			( const SiegePos & siegepos, bool noqueueclear = false );		
	void CommandStop			();
	void CommandUse				( Goid object, bool noqueueclear = false );
	void CommandUse				( Goid object, Goid actor, bool noqueueclear = false );	
	void CommandTalk			( Go * pMember, Go * pObject );
	void CommandTalk			( Go * pObject, bool noqueueclear = false );

FEX	void RSTalk					( Go * pObject, Go * pMember, bool noqueueclear );
FEX	void RCTalkBusy				( Go * pObject, DWORD machineId );
	
	bool Selection_SelectionCommandOnCursor	( const gpstring command_name, Goid object, bool noqueueclear = false );

	//bool SelectionCommandFollow ( bool noqueueclear = false );
	//bool InputCommandFollow	();

	bool SelectionCommandMove	( bool noqueueclear = false );
	bool InputCommandMove	();

	bool SelectionCommandPatrol	( bool noqueueclear = false );
	bool InputCommandPatrol	();

	bool SelectionCommandAttack	( bool noqueueclear = false );
	bool InputCommandAttack	();
	
	bool SelectionCommandCast	( bool noqueueclear = false );
	bool InputCommandCast	();

	bool SelectionCommandGuard	( bool noqueueclear = false );
	bool InputCommandGuard	();

	bool SelectionCommandGet	( bool noqueueclear = false );
	bool InputCommandGet	();

	bool SelectionCommandGive	( Goid object, bool noqueueclear = false );
	
	bool SelectionCommandUse	( bool noqueueclear = false );
	bool InputCommandUse	();

	bool SelectionCommandStop();

	void ContextActionOnTerrain			( const SiegePos & siegepos, bool noqueueclear = false );
	bool ContextCastOnTerrain			( const SiegePos & siegepos, bool noqueueclear = false );
	bool ContextActionOnGameObject		( Goid object, bool noqueueclear = false, ActionSource action_source = SOURCE_ACTOR );
	bool ContextActionOnGameObject		( Go * pMember, Go * pObject, bool noqueueclear );

	void ContextCursorAction			( bool bUseShadow = true );	
	bool ContextCursorAction			( Go * pMember, Go * pObject, eGuiContextCursor & active );

	void SetCursor( eGuiContextCursor cursor );

	void SetFriendlyFire( bool bSet )	{ m_bFriendlyFire = bSet; }
	bool GetFriendlyFire()				{ return m_bFriendlyFire; }

	eGuiContextCursor	GetActiveContextCursor() { return m_ActiveCursor; }
	void				SetActiveContextCursor( eGuiContextCursor cursor ) { m_ActiveCursor = cursor; }

private:

	UICursor * m_pCursors[CURSOR_GUI_SIZE];

	bool m_bFriendlyFire;
	eGuiContextCursor m_ActiveCursor;
	bool m_bAttackSoundPlayed;

	FUBI_SINGLETON_CLASS( UICommands, "This is the UICommands singleton" );
};

#define gUICommands UICommands::GetSingleton()


#endif