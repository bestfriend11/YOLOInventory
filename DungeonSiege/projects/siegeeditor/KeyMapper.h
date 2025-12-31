//////////////////////////////////////////////////////////////////////////////
//
// File     :  KeyMapper.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __KEYMAPPER_H
#define __KEYMAPPER_H


// Include Files
#include "gpcore.h"
#include <vector>


class KeyMapper : public Singleton <KeyMapper>
{
public:

	KeyMapper() {};
	~KeyMapper() {};

	void LoadKeyMappings();
	void SaveKeyMappings();

	ACCEL * ReloadCurrentKeyMappings();
	void InsertKeyAssignment( ACCEL key_assignment );	
	ACCEL GetKeyAssignment( WORD id );
	void RemoveKeyAssignment( WORD id );

	void ResetMappings();

private:

	std::vector< ACCEL > m_key_mappings;

};


#define gKeyMapper KeyMapper::GetSingleton()



#endif