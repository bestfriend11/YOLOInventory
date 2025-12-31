/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	GameState.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		:	This file handles game-state-change events.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/

#include "PrecompEditor.h"
#include "GameState.h"


const WorldState::TransitionEntry s_Transitions[] =
{	
	{ WS_INIT,					WS_INIT			},
	{ WS_INIT,					WS_SP_INGAME	},
	{ WS_INIT,					WS_SP_NIS		},
	{ WS_SP_INGAME,				WS_INIT			},
	{ WS_SP_INGAME,				WS_SP_NIS		},
	{ WS_SP_NIS,				WS_INIT			},
	{ WS_SP_NIS,				WS_SP_INGAME	},	
	{ WS_SP_INGAME,				WS_SP_INGAME	},	
};

GameState::GameState()
			: Inherited( s_Transitions, ELEMENT_COUNT( s_Transitions ) )
{	
}

void GameState::OnTransitionTo( eWorldState from, eWorldState to )
{
	if ( to == WS_SP_NIS )
	{
		gWorldStateRequest( from );
	}

	return;
}


void GameState::Update()
{
	WorldState::Update();
}

