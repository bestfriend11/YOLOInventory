//////////////////////////////////////////////////////////////////////////////
//
// File     :  nema_chore.cpp
// Author(s):  Mike Biddlecombe
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "precomp_nema.h"
#include "nema_chore.h"

#include "Choreographer.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "NamingKey.h"
#include "NeMa_KeyMgr.h"
#include "NeMa_Persist.h"
#include "SkritObject.h"
#include "SkritSupport.h"
#include "WorldState.h"
#include "fuel.h"
#include "GoDb.h"


using namespace nema;


//************************************************************
ActiveChore::ActiveChore()
	: m_lStanceMask(0)
{
	for (DWORD j = 0; j < MAXIMUM_STANCES; j++)
	{
		m_AnimHandles[j].clear();
	}
}

//************************************************************
ActiveChore::~ActiveChore() {
	for (DWORD j = 0; j < MAXIMUM_STANCES; j++) {
		for (HPRSKeyArrayIter k = m_AnimHandles[j].begin(); k != m_AnimHandles[j].end(); ++k) {
			if ((*k)) {
				gPRSKeysStorage.ReleasePRSKeys(*k);
			}
		}
		m_AnimHandles[j].clear();
	}
}

//************************************************************
bool ActiveChore::Init( Aspect* aspect, const Chore* chore, bool shouldPersist )
{
	// This is a new chore, we need to be sure that ALL the anims are loaded

	bool ret = InitHelper(aspect,chore);

	if (ret)
	{
		// clone the chore's skrit for local use
		Skrit::CloneReq cloneReq( chore->m_AnimSkritObject );
		cloneReq.m_CloneGlobals = true;
		cloneReq.m_DeferConstruction = ( WorldState::DoesSingletonExist() && (gWorldState.GetCurrentState() == WS_LOADING_SAVE_GAME) ) &&
									   ( GoDb::DoesSingletonExist() && !gGoDb.IsPostLoading() );
		if ( shouldPersist )
		{
			cloneReq.SetPersistDetect();
		}
		else
		{
			cloneReq.SetPersist( false );
		}
		ret = m_Skrit.CloneSkrit( cloneReq );
	}
	
	// tell the skrit who owns it
	if (ret)
	{
		SetOwner( aspect );
	}

	return ret;
}

//************************************************************
void ActiveChore::CommitCreation()
{
	m_Skrit->CommitCreation();
}

//************************************************************
bool ActiveChore::Rebuild( Aspect* aspect, const Chore* chore )
{

	bool ret = InitHelper(aspect,chore);

	// Check to see if the skrit was persisted by a previous save/restore

	if (ret && !m_Skrit)
	{
		// The skrit was NOT persisted, so we need to create another instance
		// clone the chore's skrit for local use
		Skrit::CloneReq cloneReq( chore->m_AnimSkritObject );
		cloneReq.m_CloneGlobals = true;
		cloneReq.m_DeferConstruction = (gWorldState.GetCurrentState() == WS_LOADING_SAVE_GAME && gGoDb.IsPostLoading() == false);
		cloneReq.SetPersist( false );
		ret = m_Skrit.CloneSkrit( cloneReq );

	}

	// tell the skrit who owns it
	if (ret)
	{
		m_Skrit->SetOwner( aspect );
	}

	return ret;
}



//************************************************************
bool ActiveChore::InitHelper( Aspect* aspect, const Chore* chore )
{

	DWORD stancemask = chore->m_lStanceMask;

	for (DWORD stance = 0; stance<MAXIMUM_STANCES; stance++) {
		m_AnimHandles[stance].clear();
	}

	gpstring sAnimName;
	HPRSKeys hkeys;

	if (stancemask == 0xffffffff) {

		// This is a preview chore within the anim viewer

		sAnimName = chore->m_vsAnimFiles[0][0];
		hkeys = gPRSKeysStorage.LoadPRSKeys(sAnimName,sAnimName);

		if (hkeys.IsValid()) {

			hkeys->SetBlendWeight(0.0);

			m_AnimHandles[0].push_back(hkeys);

		} else {

			return false;
		}


	} else {

		for (DWORD stance = 0; stance<MAXIMUM_STANCES; stance++) {

			if (((1<<stance) & stancemask) == 0) {
				continue;
			}

			//PRSKeyArray& stanceanims = m_AnimHandles[stance];

			for (DWORD nameindex = 0; nameindex < chore->m_vsAnimFiles[stance].size(); nameindex++) {

				gpstring sAnimFilename;
				sAnimName = chore->m_vsAnimFiles[stance][nameindex];

				hkeys = HPRSKeys();

				if (gNamingKey.BuildPRSLocation(sAnimName.c_str(), sAnimFilename )) {
					hkeys = gPRSKeysStorage.LoadPRSKeys(sAnimFilename,sAnimName.c_str());
				}

				if (hkeys.IsValid()) {

					hkeys->SetBlendWeight(0.0);

					m_AnimHandles[stance].push_back(hkeys);

				} else {

					if (m_AnimHandles[stance].size() == 0)
					{
						gpfatalf(("Invalid PRS '%s' was encountered while building list of subanims for \"%s\","
								"\nThis was the first PRS name encountered so there is nothing to use as a substitute"
								"\n\nYou will need to either locate the correct file or you can choose to remove"
								" or comment out the entry from the parent GAS file",sAnimName.c_str(),aspect->GetDebugName()));
						return false;
					} 
					else
					{
						HPRSKeys extracopy = m_AnimHandles[stance][0];
						m_AnimHandles[stance].push_back(extracopy);
						gPRSKeysStorage.AddRefPRSKeys(m_AnimHandles[stance][0]);
					}

				}

			}
		}
	}

	// copy any relevant chore stuff local that we may need later
	m_lStanceMask = chore->m_lStanceMask;
	m_vlAnimFourCC = chore->m_vlAnimFourCC;
	m_vfAnimDurations = chore->m_vfAnimDurations;

	return true;
}

//************************************************************
bool
ActiveChore::Xfer( FuBi::PersistContext& persist ) {

	gpassert(persist.IsRestoring() || m_Skrit.IsPersistent());
	persist.Xfer( "m_Skrit", m_Skrit );
	return ( true );
}


//************************************************************
void
ActiveChore::SetOwner(Aspect* aspect) {
	m_Owner	= aspect;
	if (m_Skrit)
	{
		m_Skrit->SetOwner(aspect);
	}
}

//************************************************************
int
ActiveChore::NumSubAnimsForStance( DWORD stance ) const  {

	if (stance >= MAXIMUM_STANCES) return 0;

	return(m_AnimHandles[stance].size());

}

//************************************************************
float
ActiveChore::SubAnimDurationForStance(DWORD stance, DWORD index) const  {

	if ( (stance < MAXIMUM_STANCES) && (index < m_AnimHandles[stance].size()) )
	{
		const PRSKeys* prs = m_AnimHandles[stance][index].GetValid();
		if ( prs != NULL )
		{
			return prs->GetDuration();
		}
	}

	return 0.0;

}
//************************************************************
float
ActiveChore::BaseAnimDurationForStance(DWORD stance) const  {

	float dur = 0.0f;
	if (stance < MAXIMUM_STANCES)
	{
		dur = -1;
		if (m_vfAnimDurations.size() > stance)
		{
			dur = m_vfAnimDurations[stance];
		}
		if (IsNegative(dur))
		{
			if (m_AnimHandles[stance].size() && m_AnimHandles[stance][0].IsValid())
			{
				dur = m_AnimHandles[stance][0]->GetDuration();
			} 
			else
			{
				for (DWORD i = 0; i < MAXIMUM_STANCES; ++i)
				{
					if (m_AnimHandles[i].size() && m_AnimHandles[i][0].IsValid())
					{
						dur = m_AnimHandles[i][0]->GetDuration();
						break;
					} 
				}
			}
		}
	}
	return dur;

}

///************************************************************
bool
ActiveChore::GetScalarVelocityForStance(DWORD stance, DWORD index , float& v) const  {


	if (stance < MAXIMUM_STANCES &&
		index < m_AnimHandles[stance].size() &&
		m_AnimHandles[stance][index].IsValid()) {

		v =  m_AnimHandles[stance][index]->GetScalarVelocity();

		return true;
	}

	v = 0;

	return false;

}

///************************************************************
PRSKeys*
ActiveChore::GetKeysForStance(DWORD stance, DWORD index )
{
	if ( (stance < MAXIMUM_STANCES) && (index < m_AnimHandles[stance].size()) )
	{
		return m_AnimHandles[stance][index].GetValid();
	}

	return 0;
}
