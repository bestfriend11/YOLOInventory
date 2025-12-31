//////////////////////////////////////////////////////////////////////////////
//
// File     :  UITeamManager.h
// Author(s):  Chad Queen
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UITEAMMANAGER_H
#define __UITEAMMANAGER_H

#include "GoDefs.h"

#define MAX_TEAM_MEMBERS 7

// Enum listing possible team-related windows
enum eTeamWindow
{
	TW_HEALTH,
	TW_MANA,
	TW_PORTRAIT,
	TW_DEATH,
	TW_HEALTH_WARNING,
	TW_UNCONSCIOUS,
	TW_PORTRAIT_ROLLOVER,
	TW_GHOST_TIMER,
	TW_PORTRAIT_ITEM,
	TW_GUARDED,

	TW_SIZE,
};


// Individual Team Member class object
class UITeamMember
{
public:

	UITeamMember( Goid teamMember ) { m_Member = teamMember; }

	Goid GetGoid()					{ return m_Member; }
	void SetGoid( Goid member )		{ m_Member = member; }
	
// Window management
	UIWindow *	GetGuiWindow( eTeamWindow index )						{ return m_pWindow[index];		}
	void		SetGuiWindow( UIWindow * pWindow, eTeamWindow index )	{ m_pWindow[index] = pWindow;	}

	unsigned int GetTexture()						{ return m_Texture; }
	void		 SetTexture( unsigned int texture ) { m_Texture = texture; }

private:

	Goid		m_Member;
	UIWindow *	m_pWindow[TW_SIZE];	
	unsigned int m_Texture;	
};


// Team Manager class
class UITeamManager : public Singleton <UITeamManager>
{
public:

	UITeamManager();
	~UITeamManager();

// Initialization
	void Init();
	void DeInit();	

// Updating 
	void Update( float seconds );
	void UpdateTeamInterface( float seconds );

// Interface
	void ConstructTeam();
	void DeconstructTeam();
	void CastOnCharacterPortrait( int teamIndex );
	void GuardOnCharacterPortrait( int teamIndex );	
	void ActivateTrade( int teamIndex );

// Portraits
	bool GenerateTeamPortraits();
	bool GeneratePortrait( Go * pTeamMember );
	void OnScreenSize();
	
private:

	UITeamMember * m_UITeamMembers[MAX_TEAM_MEMBERS];

	int m_TeamSize;

};

#define gUITeamManager UITeamManager::GetSingleton()


#endif