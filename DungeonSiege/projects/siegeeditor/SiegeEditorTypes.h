//////////////////////////////////////////////////////////////////////////////
//
// File     :  SiegeEditorTypes.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __SIEGEEDITORTYPES_H
#define __SIEGEEDITORTYPES_H



/* Include Files */
#include <vector>
#include <string>
#include "gpstring.h"




/* Type Definitions */

// Good ol' vector o' strings
typedef std::vector< gpstring > StringVec;




/* Enumerated Data Types */

// For keyboard control of GO's 
enum PRECISION_DIRECTION
{
	PRECISION_LEFT,
	PRECISION_RIGHT,
	PRECISION_FORWARD,
	PRECISION_BACKWARD,
	PRECISION_UP,
	PRECISION_DOWN,
};



#endif 
