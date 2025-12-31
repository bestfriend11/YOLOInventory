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

class GameProgress;



class GameState : public WorldState, public Singleton <GameState>
{

public:
	SET_INHERITED( GameState, WorldState );

	GameState();

	virtual ~GameState();

	virtual void Update();

	virtual void OnTransitionTo( eWorldState from, eWorldState to );

	void HandleBroadcast( WorldMessage & msg );

	void StartProgress  ( bool forLoad );
	void StopProgress   ( void );
	bool IsProgressBarUp( void ) const		{  return ( m_Progress != NULL );  }

private:

	eWorldState			m_MPGameType;
	my GameProgress*	m_Progress;

	double				m_LastTimeJipPlayerPresent;
};

#define gGameState Singleton <GameState>::GetSingleton()



#endif  // __GAMESTATE_H
