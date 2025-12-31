#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	GameState.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		:	Master game state synchronizer.  See WorldState.h

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __GAMESTATE_H
#define __GAMESTATE_H




#include "WorldState.h"



class GameState : public WorldState
{

public:
	SET_INHERITED( GameState, WorldState );

	GameState();

	virtual ~GameState(){};

	virtual void Update();

	virtual void OnTransitionTo( eWorldState from, eWorldState to );	
};




#endif  // __GAMESTATE_H
