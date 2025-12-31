//////////////////////////////////////////////////////////////////////////////
//
// File     :  GuiHelper.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef __GUIHELPER_H
#define __GUIHELPER_H

// Forward Declaration
class UIItem;
class UIGridbox;


gpstring GetTextureStringFromIcon( Go * pGO, gpstring sIcon, bool & bSuccess );

gpstring GetSlotName( Go * pGO );

UIItem * GetItemFromGO( Go * pGO, bool bActive = true, bool bPortrait = false );

// Get a gridbox from a go's settings
UIGridbox * GetGridboxFromGO( Go *pGO, int index );

bool ExtractFirstTagFromString( gpwstring & sString, gpwstring & sTag );

// For rounding floats to ints
inline int fix_precision( float val ) { return (int)((floorf(1024*val+0.5f))/1024.0f); }


#endif