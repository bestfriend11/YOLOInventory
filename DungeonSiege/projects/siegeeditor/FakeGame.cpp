//////////////////////////////////////////////////////////////////////////////
//
// File     :  FakeGame.cpp
// Author(s):  Scott Bilas, Chad Queen
//
// Summary  :  This is a BIG HACK. It's purely meant to implement symbols
//             that are required by _World->GameGUI->Game. This should go
//             away when a new layer is added in the game so we can use
//			   virtual functions.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "PrecompEditor.h"
#include "gpcore.h"
#include "TattooGame.h"

//////////////////////////////////////////////////////////////////////////////
// FAKE GAME HA HA HA
void TattooGame::TeleportPlayer( PlayerId playerId, const SiegePos& pos, bool teleportSelectedItems ) { };
bool TattooGame::SaveGame( const wchar_t* saveName )  { return false; }
bool TattooGame::CheckAutoSaveParty( bool ignoreMinTime ) { return false; }
bool TattooGame::PrivateReloadObjects( eReloadType type ) { return false; }