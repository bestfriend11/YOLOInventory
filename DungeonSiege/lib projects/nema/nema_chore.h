#pragma once
//////////////////////////////////////////////////////////////////////////////
//
// File     :  nema_chore.h
// Author(s):  Mike Biddlecombe
//
// Summary  :  Contains <Summary>
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#ifndef NEMA_CHORE_H
#define NEMA_CHORE_H

#include "FuBiDefs.h"
#include "nema_types.h"
#include "nema_keymgr.h"
#include "nema_kevents.h"
#include "SkritDefs.h"

//#include <set>


namespace nema {

	typedef stdx::fast_vector<HPRSKeys>		HPRSKeyArray;
	typedef HPRSKeyArray::iterator			HPRSKeyArrayIter;
	typedef HPRSKeyArray::const_iterator	HPRSKeyArrayConstIter;

	class ActiveChore
	{

	public:

		ActiveChore();
		~ActiveChore();

		bool Init( Aspect* aspect, const Chore* chore, bool shouldPersist );
		void CommitCreation();

		bool Rebuild( Aspect* aspect, const Chore* chore );

		void SetOwner(Aspect* aspect);

		bool Xfer( FuBi::PersistContext& persist );

		FUBI_EXPORT int NumSubAnimsForStance( DWORD stance ) const;

		FUBI_EXPORT float SubAnimDurationForStance( DWORD stance, DWORD index) const;

		FUBI_EXPORT float BaseAnimDurationForStance( DWORD stance) const;

		bool GetScalarVelocityForStance(DWORD stance, DWORD index , float& v) const;

		PRSKeys* GetKeysForStance(DWORD stance, DWORD index);

		bool StatusDump(gpstring& out) const;

		// Local copies of initial chore data
		DWORD						m_lStanceMask;
		stdx::fast_vector<DWORD>	m_vlAnimFourCC;
		stdx::fast_vector<float>	m_vfAnimDurations;

		HPRSKeyArray				m_AnimHandles[MAXIMUM_STANCES];
		Skrit::HAutoObject			m_Skrit;

	private:

		bool InitHelper( Aspect* aspect, const Chore* chore );

		Aspect*					m_Owner;
	};
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __NEMA_CHORE_H

//////////////////////////////////////////////////////////////////////////////
