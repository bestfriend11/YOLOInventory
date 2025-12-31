//////////////////////////////////////////////////////////////////////////////
//
// File     :  Nema_KeyMgr.h
// Author(s):  Mike Biddlecombe
//
// Summary  : Managers for different kinds of animation key data
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef NEMA_KEYMGR
#define NEMA_KEYMGR

#include "vector_3.h"
#include "quat.h"

#include <list>
#include <map>

namespace nema {

	//////////////////////////////////////////////////////////////////////////////
	//
	// Management of Position Rotation Scale (PRS) keys
	//
	////////////////////////////////////////////////////////////////////////////

	class PRSKeys;		// fwd decl
	class PRSKeysImpl;

	typedef ResHandle<PRSKeys>		HPRSKeys;
	typedef ResHandleMgr<PRSKeys>	HPRSKeysMgr;


	////////////////////////////////////////////////////////////////////////////

	class PRSKeysStorage : public Singleton <PRSKeysStorage>
	{
	public:
		SET_NO_INHERITED( PRSKeysStorage );

	// Ctor/dtor.

		PRSKeysStorage( void );
	   ~PRSKeysStorage( void );

		HPRSKeys LoadPRSKeys(char const * const fname, char const * const dbgname);

		int AddRefPRSKeys ( HPRSKeys hKeys );
		int ReleasePRSKeys( HPRSKeys hKeys );


	private:
		struct IndexEntry;
		struct PRSKeysEntry;

		typedef std::map<gpstring, PRSKeysEntry, istring_less> StringToPRSKeysMap;
		typedef ResHandleExtraMgr <HPRSKeys, IndexEntry> PRSKeysMgr;
		typedef std::list <HPRSKeys> HPRSKeysList;

		struct PRSKeysEntry
		{
			PRSKeysImpl*	m_Impl;
			HPRSKeysList	m_Inst;

			PRSKeysEntry( void )  {  m_Impl = NULL;  }
		};

		struct IndexEntry
		{
			StringToPRSKeysMap::iterator	m_NameEntry;
			HPRSKeysList::iterator	        m_ListEntry;
		};

		PRSKeysMgr							m_PRSKeysManager;
		StringToPRSKeysMap					m_PRSKeysByNameIndex;

		kerneltool::Critical				m_CreateLock;

		SET_NO_COPYING( PRSKeysStorage );
	};

	#define gPRSKeysStorage nema::PRSKeysStorage::GetSingleton()


};

// Now we all the different type of keys we can handle (they were fwd declared at the top of the file)
#include "nema_prskeys.h"


#endif
