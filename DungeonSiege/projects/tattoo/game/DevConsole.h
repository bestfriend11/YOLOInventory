//////////////////////////////////////////////////////////////////////////////
//
// File     :  DevConsole.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes needed to maintain the development console.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __DEVCONSOLE_H
#define __DEVCONSOLE_H

//////////////////////////////////////////////////////////////////////////////

#include "GPCore.h"

//////////////////////////////////////////////////////////////////////////////
// class DevConsoleCommand declaration

class DevConsoleCommand
{
public:
	SET_NO_INHERITED( DevConsoleCommand );

	DevConsoleCommand( void );
   ~DevConsoleCommand( void );

private:
	SET_NO_COPYING( DevConsoleCommand );
};

//////////////////////////////////////////////////////////////////////////////
// class DevConsole declaration

class DevConsole
{
public:
	SET_NO_INHERITED( DevConsole );

	DevConsole( void );
   ~DevConsole( void );

private:
	SET_NO_COPYING( DevConsole );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __DEVCONSOLE_H

//////////////////////////////////////////////////////////////////////////////
