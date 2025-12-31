//////////////////////////////////////////////////////////////////////////////
//
// File     :  ScidManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __SCIDMANAGER_H
#define __SCIDMANAGER_H

// Include Files
#include "gpcore.h"
#include "GoDefs.h"


class SCIDManager : public Singleton <SCIDManager>
{
public:

	SCIDManager();
	~SCIDManager()	{}
	
	// Read
	Scid GetScid();

	void CompileScidSet();

	void ClearScids();

	void SetScidRange( int scidRange );

private:
	
	bool IsScidUnique( Scid scid );
	Scid				m_nextScid;
	std::set< Scid >	m_usedScids;

};

#define gSCIDManager SCIDManager::GetSingleton()


#endif