#include "precomp_nema.h"

#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "StdHelp.h"
#include "nema_blender.h"
#include "nema_kevents.h"
#include "nema_persist.h"

#include "WorldOptions.h"		// Needed to get game speed

using namespace nema;

FUBI_DECLARE_XFER_TRAITS( ActiveChore* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new ActiveChore;
		}
		return ( obj->Xfer( persist ) );
	}
};

//************************************************************
Blender::Blender(Aspect* own)
{
	m_Owner = own;
	m_ActiveChoreArray.resize( CHORE_COUNT, NULL );
	m_BonePositions.resize(own->GetNumBones());
	m_BoneRotations.resize(own->GetNumBones());
	m_IsDirtyFlag = true;
	m_RecentEvents = 0;

}

//************************************************************
Blender::~Blender(void)
{
	stdx::for_each( m_ActiveChoreArray, stdx::delete_by_ptr() );
}

FUBI_DECLARE_SELF_TRAITS( TimeWarp );

//************************************************************
struct NemaShouldPersistActiveChore
{
	// Persistence predicate
	bool operator () ( ActiveChore* ch) const
	{
		return (
			ch 
			&& ch->m_Skrit.IsPersistent()
			);
	}
};

//************************************************************
bool
Blender::Xfer( FuBi::PersistContext& persist ) {

	// Owner is now set when owning aspect is restored --biddle
	
	persist.Xfer        ( "m_RootVelocity",  m_RootVelocity );
	persist.Xfer		( "m_RecentEvents",  m_RecentEvents );

	persist.XferSparseArray( "m_ActiveChoreArray",  
							m_ActiveChoreArray.begin(), 
							m_ActiveChoreArray.end(),
							NemaShouldPersistActiveChore() );

	if (persist.IsRestoring() || m_TimeWarp.m_Intervals.size())
	{
		if (persist.EnterBlock( "timewarp" ))
		{
			persist.Xfer   ( "m_TimeWarp",      m_TimeWarp     );
			persist.Xfer   ( "m_TracePoint0",   m_TracePoint0  );
			persist.Xfer   ( "m_TracePoint1",   m_TracePoint1  );
		}
		else if (persist.IsRestoring())
		{
			m_TimeWarp.Clear();
		}
		persist.LeaveBlock();
	}


	return ( true );
}

//************************************************************
bool
Blender::XferPost( const ChoreDictionary& chores )
{

	ChoreDictionary::const_iterator i, begin = chores.begin(), end = chores.end();
	for ( i = begin ; i != end ; ++i )
	{
		ActiveChore*& ch = m_ActiveChoreArray[(*i).second.m_Verb];
		if (ch)
		{
			ch->Rebuild( m_Owner, &i->second );
		}
		else
		{
			ch = new ActiveChore;
			ch->Init( m_Owner, &i->second, false );
		}
	}
	SetDirtyFlag(true);

	// Code that was here is now located in nema_aspect ( fix for bug #10809)
	m_Owner->ResetBlender();

	return true;
}


void
Blender::CommitCreation()
{
	ActiveChoreArray::iterator i, begin = m_ActiveChoreArray.begin(), end = m_ActiveChoreArray.end();
	for ( i = begin ; i != end ; ++i )
	{
		ActiveChore * ch = *i;
		if( ch )
		{
			ch->CommitCreation();
		}
	}
}

//************************************************************
void 
Blender::AddChore(const Chore* nc, bool shouldPersist)
{
	delete m_ActiveChoreArray[nc->m_Verb];
	m_ActiveChoreArray[nc->m_Verb] = CreateActiveChore(m_Owner,nc,shouldPersist);
}

// ***************************************************
ActiveChore*
Blender::CreateActiveChore(Aspect* owner, const Chore* chore, bool shouldPersist) {

	// Routine converts a choreoter from the chore dictionary into an ActiveChore and returns a pointer
	// to the new active chore.

	// Create a new current chore
	ActiveChore* NewActiveChore = new ActiveChore;

	// Attempt to initialize
	if ( !NewActiveChore->Init( owner, chore, shouldPersist ) )
	{
		// Failed - delete and NULL pointer
		Delete( NewActiveChore );
	}

	return NewActiveChore;
}

//***********************************************************************************
void
Blender::SetOwner(Aspect* own)
{
	m_Owner = own;
	for (DWORD c=0; c<m_ActiveChoreArray.size(); c++)
	{
		if (m_ActiveChoreArray[c])
		{
			m_ActiveChoreArray[c]->SetOwner(own);
		}
	}
}

//************************************************************
bool
Blender::GetSubAnimsAreLoaded( eAnimChore chore, eAnimStance stance ) const  
{
	if ( (stance < MAXIMUM_STANCES) && m_ActiveChoreArray[chore] )
	{
		if ( m_ActiveChoreArray[chore]->m_vlAnimFourCC.empty() || !m_ActiveChoreArray[chore]->m_AnimHandles[stance].empty() )
		{
			return true;
		}
	}
	return false;
}

//************************************************************
int
Blender::GetNumSubAnims( eAnimChore chore, eAnimStance stance ) const  
{
	if (stance >= MAXIMUM_STANCES) return 0;
	return (m_ActiveChoreArray[chore]) ? (m_ActiveChoreArray[chore]->m_AnimHandles[stance].size()) : 0;
}

//***********************************************************************************
int
Blender::GetSubAnimIndex( eAnimChore chore, DWORD fourCC ) const
{

	if (m_ActiveChoreArray[chore])
	{
		for (int subanim =0 ; subanim < (int)m_ActiveChoreArray[chore]->m_vlAnimFourCC.size(); ++subanim )
		{
			if (m_ActiveChoreArray[chore]->m_vlAnimFourCC[subanim] == fourCC)
			{
				return subanim;
			}
		}
	}

	return -1;
}

//***********************************************************************************
float
Blender::GetScalarVelocity( eAnimChore chore, eAnimStance stance, DWORD subanim) const
{
	float v = 0;
	if (m_ActiveChoreArray[chore]) {
		m_ActiveChoreArray[chore]->GetScalarVelocityForStance((DWORD)stance,subanim,v);
	}
	return v;
}

//***********************************************************************************
float 
Blender::GetDuration( eAnimChore chore, eAnimStance stance, DWORD subanim) const
{
	float dur = 0;
	if (m_ActiveChoreArray[chore]) {
		dur = m_ActiveChoreArray[chore]->SubAnimDurationForStance((DWORD)stance,subanim);
	}
	return dur;
}

//***********************************************************************************
float 
Blender::GetBaseDuration( eAnimChore chore, eAnimStance stance) const
{
	float dur = 0;
	if (m_ActiveChoreArray[chore]) {
		dur = m_ActiveChoreArray[chore]->BaseAnimDurationForStance((DWORD)stance);
	}
	return dur;
}
	
//***********************************************************************************
int
Blender::GetMinDurationAnim( eAnimChore chore, eAnimStance stance) const
{
	int numAnims = GetNumSubAnims(chore, stance);
	gpassert( numAnims > 0 );

	float minAnimDuration = FLOAT_MAX;
	float currentAnimDuration = FLOAT_MAX;
	int minLengthAnimation = 0;					
	
	for (int i = 0; i < numAnims; i++)
	{
		currentAnimDuration = GetDuration(chore, stance, i);
		if (currentAnimDuration < minAnimDuration)
		{
			minAnimDuration = currentAnimDuration;
			minLengthAnimation = i;
		}
	}
	gpassert( minAnimDuration < FLOAT_MAX && currentAnimDuration < FLOAT_MAX );
	return minLengthAnimation;
}

//***********************************************************************************
void
Blender::ProcessHideShowEventsForSkippedChore( eAnimChore chore, eAnimStance stance,DWORD subanim ) const
{

	PRSKeys* kp = GetKeys(chore,stance,subanim);

	if (kp)
	{
		KEventMap& kevents = kp->GetCriticalEvents();
		KEventMap::Iter it = kevents.Begin();
		KEventMap::Iter end = kevents.End();

		for ( ; it!=end ; ++it )
		{
			if ( AnimEventBitTest( (*it).second, ANIMEVENT_HIDE_MESH ) )
			{
				m_Owner->SetHideMeshFlag(true);
			}
			if ( AnimEventBitTest( (*it).second, ANIMEVENT_SHOW_MESH ) )
			{
				m_Owner->SetHideMeshFlag(false);
			}
		}
	}	

}

//***********************************************************************************
PRSKeys*
Blender::GetKeys( eAnimChore chore, eAnimStance stance, DWORD subanim) const
{
	PRSKeys* kp = 0;
	if (m_ActiveChoreArray[chore])
	{
		 kp = m_ActiveChoreArray[chore]->GetKeysForStance((DWORD)stance,subanim);
	}
	return kp;
}

//************************************************************
DWORD
Blender::UpdateActiveAnims(float deltat) {

	KEventTags triggered_events = m_TimeWarp.UpdateInterval(this,deltat);

	if (BitTest(triggered_events,ANIMEVENT_START))
	{
		// Reset the recent events at the beginning of each loop
		m_RecentEvents = 0;
	}
	else
	{
		m_RecentEvents |= (DWORD)triggered_events;
	}

	if (m_TimeWarp.m_CurrentInterval == m_TimeWarp.m_Intervals.end())
	{
		for (PRSKeyArrayIter k = m_ActiveAnims.begin();k != m_ActiveAnims.end(); ++k)
		{
			if ((*k) != NULL)
			{
				m_IsDirtyFlag |= (*k)->UpdateAbsolute(1,0);
			}
		}
		return 0;
	}

	float alpha;
	if (IsZero((*m_TimeWarp.m_CurrentInterval).m_WeightedDuration))
	{
		alpha = 0.0;
	} 
	else
	{
		alpha = m_TimeWarp.m_IntervalTime;
	}

	const WarpSegmentList& currsegs = (*m_TimeWarp.m_CurrentInterval).m_Segments;

	float adjusted_tenthousandth = 0.0001f * (WorldOptions::DoesSingletonExist() ? gWorldOptions.GetGameSpeed() : 1.0f );
	// normalize 1/1000th of a second
	float normalized_tolerance = 10.0f*adjusted_tenthousandth/(m_TimeWarp.m_TotalTime);

	m_ActiveAnims.clear();

	for (WarpSegmentList::const_iterator seg = currsegs.begin(); seg != currsegs.end() ; ++seg) 
	{

		const BlendGroup* pbg = (*seg).m_pBlendGroup;
		const DWORD bgi = (*seg).m_BlendGroupIndex;

		PRSKeys* keys = pbg->m_KeyInfo[bgi].GetKeys(this);

		if (!keys)
		{
			gperrorf(("DATA ERROR: Nema::UpdateActiveAnims() Some PRS keys appear to be missing. Are you loading a corrupt savegame?"));
			continue;
		}

		const float seginterv = (*seg).m_Interval;

		const float lhs_time = (*seg).m_Time;

		const float warped_keytime = lhs_time + (seginterv * alpha);
		const float warped_weight =  pbg->GetWeight(bgi,alpha);

		if (warped_weight != keys->GetBlendWeight())
		{
			m_IsDirtyFlag |= true;
		}
		if (IsZero(warped_weight))
		{
			keys->SetBlendWeight(0.0f);
		}
		else
		{
			keys->SetBlendWeight(warped_weight);
			m_IsDirtyFlag |= keys->UpdateAbsolute(warped_keytime, normalized_tolerance);
			m_ActiveAnims.push_back(keys);
		}

	}

	return triggered_events;
}

//************************************************************
bool
Blender::BlendActiveAnims(DWORD firstbone)
{

	m_RootVelocity = vector_3::ZERO;

	if (m_ActiveAnims.size() < 1)
	{
		return false;
	}
	else if (m_ActiveAnims.size() == 1)
	{
		PRSKeys* blendkey = m_ActiveAnims.front();

		for (DWORD b = firstbone; b < m_BonePositions.size(); ++b)
		{
			DWORD freezestate = m_Owner->GetIndexedBoneFrozenState(b);

			if ((int)b < blendkey->GetNumTracks())
			{
				vector_3& keypos = m_BonePositions[b];
				Quat& keyrot = m_BoneRotations[b];

				if ( freezestate == BFREEZE_BOTH )
				{
					if (b == firstbone)
					{
						// Need to fetch the deltas that are accumulating
						// so that they are properly cleared
						m_RootVelocity = vector_3::ZERO;
					}
				}
				else if ( freezestate == BFREEZE_POSITION ) 
				{
					keyrot = blendkey->GetCurrRotation(b);
					if (b == firstbone)
					{
						m_RootVelocity = vector_3::ZERO;
					}
				}
				else if (freezestate == BFREEZE_ROTATION ) 
				{
					keypos = (blendkey->GetCurrPosition(b));
				}
				else	// Nothing is frozen
				{
					keypos = (blendkey->GetCurrPosition(b));
					keyrot = blendkey->GetCurrRotation(b);
					if (b == firstbone)
					{
						m_RootVelocity = blendkey->GetVelocity();
					}
				}
			}
		}
	}
	else
	{
		// Interpolate between all active anims setting the m_BonePositions & m_BoneRotations
		for (DWORD b = firstbone; b < m_BonePositions.size(); ++b) {

			DWORD freezestate = m_Owner->GetIndexedBoneFrozenState(b);

			float curr_weight,prev_weight;
			float total_weight = 0;

			vector_3& keypos = m_BonePositions[b];
			Quat& keyrot =m_BoneRotations[b];

			for (PRSKeyArrayIter k = m_ActiveAnims.begin();k != m_ActiveAnims.end(); ++k)
			{
				PRSKeys* blendkey = (*k);
				if ( blendkey == NULL )
				{
					continue;
				}

				curr_weight = (*k)->GetBlendWeight();

				if (IsZero(curr_weight))
				{
					continue;
				}

				if ((int)b >= (*k)->GetNumTracks())
				{
					continue;
	//				gpfatal("There are too few tracks!");
				}

				prev_weight = total_weight;
				total_weight += curr_weight;

				if ( freezestate == BFREEZE_BOTH )
				{
					if (b == firstbone)
					{
						m_RootVelocity = vector_3::ZERO;
					}
					continue;
				}
				else if ( freezestate == BFREEZE_POSITION ) 
				{
					if (IsZero(prev_weight))
					{
						keyrot = blendkey->GetCurrRotation(b);
						if (b == firstbone)
						{
							m_RootVelocity = vector_3::ZERO;
						}
						continue;
					}

					float blend_alpha = 1-(prev_weight/total_weight);

					if (IsEqual(blend_alpha,1))
					{
						keyrot = blendkey->GetCurrRotation(b);
						if (b == firstbone)
						{
							m_RootVelocity = vector_3::ZERO;
						}
					} 
					else
					{
						SlerpInPlace( keyrot , blendkey->GetCurrRotation(b), blend_alpha);
						if (b == firstbone)
						{
							m_RootVelocity = vector_3::ZERO;
						}
					}
				}
				else if (freezestate == BFREEZE_ROTATION ) 
				{
					if (IsZero(prev_weight))
					{
						keypos = (blendkey->GetCurrPosition(b));
						continue;
					}

					float blend_alpha = 1-(prev_weight/total_weight);

					if (IsEqual(blend_alpha,1))
					{
						keypos = blendkey->GetCurrPosition(b);
						if (b == firstbone)
						{
							m_RootVelocity = blendkey->GetVelocity();
						}
					}
					else
					{
						keypos = keypos + (blendkey->GetCurrPosition(b) - keypos)*blend_alpha;
						if (b == firstbone)
						{
							SlerpInPlace(m_RootVelocity,blendkey->GetVelocity(), blend_alpha);
						}
					}
				}
				else	// Nothing is frozen
				{
					if (IsZero(prev_weight))
					{
						keypos = (blendkey->GetCurrPosition(b));
						keyrot = blendkey->GetCurrRotation(b);
						if (b == firstbone)
						{
							m_RootVelocity = blendkey->GetVelocity();
						}
						continue;
					}

					float blend_alpha = 1-(prev_weight/total_weight);

					if (IsEqual(blend_alpha,1))
					{
						keypos = blendkey->GetCurrPosition(b);
						keyrot = blendkey->GetCurrRotation(b);
						if (b == firstbone)
						{
							m_RootVelocity = blendkey->GetVelocity();
						}
					} 
					else
					{
						keypos = keypos + (blendkey->GetCurrPosition(b) - keypos)*blend_alpha;
						SlerpInPlace( keyrot , blendkey->GetCurrRotation(b), blend_alpha);
						if (b == firstbone)
						{
							SlerpInPlace(m_RootVelocity,blendkey->GetVelocity(), blend_alpha);
						}
					}
				}

			}
		}
	}

	return true;
}

//************************************************************
const gpstring&
Blender::GetBlendedPRSDebugName(DWORD i)
{
	return (i < m_ActiveAnims.size()) ? m_ActiveAnims[i]->GetDebugNameString() : gpstring::EMPTY;
}

//************************************************************
bool
Blender::IsolateActiveAnim(DWORD i, DWORD firstbone)
{

	if (i < m_ActiveAnims.size())
	{

		PRSKeys*  blendkey = m_ActiveAnims[i];

		for (DWORD b = firstbone; b < m_BonePositions.size(); ++b)
		{
			m_BonePositions[b] = (blendkey->GetCurrPosition(b));
			m_BoneRotations[b] = (blendkey->GetCurrRotation(b));
		}
	}

	return true;
}

#if !GP_RETAIL
//************************************************************
void
Blender::StatusDump(gpstring& out)
{
	out.appendf("Active Animations:\n");
	for (DWORD k = 0; k < m_ActiveAnims.size(); ++k)
	{
		out.appendf(" %s [%d.%d] = w:%8.3f t:%8.3f\n",
			m_ActiveAnims[k]->GetDebugName(),
			m_ActiveAnims[k]->GetMajorVersion(),
			m_ActiveAnims[k]->GetMinorVersion(),
			m_ActiveAnims[k]->GetBlendWeight(),
			m_ActiveAnims[k]->GetCurrentNormalizedTime());
	}
	m_TimeWarp.StatusDump(this,out);
}
#endif

//************************************************************
void
Blender::AddAnimToBlendGroup(DWORD i,float relw, bool forcecyclic)
{
	DWORD chore = m_Owner->GetCurrentChore();
	DWORD stance = m_Owner->GetCurrentStance();

	gpassertf(i < m_ActiveChoreArray[chore]->m_AnimHandles[stance].size(),("AddAnimToBlendGroup() Index [%d] out of range [%s]",i,m_Owner->GetDebugName()));
	gpassertf(m_ActiveChoreArray[chore]->m_AnimHandles[stance][i],("AddAnimToBlendGroup() Invalid PRS at index [%d] [%s]",i,m_Owner->GetDebugName()));

	if (stance >= MAXIMUM_STANCES ||
		!m_ActiveChoreArray[chore] ||
		i >= m_ActiveChoreArray[chore]->m_AnimHandles[stance].size() ||
		!m_ActiveChoreArray[chore]->m_AnimHandles[stance][i])
	{
		return;
	}

	gpassertf(m_TimeWarp.m_ConstructionGroup != NULL,("AddAnimToBlendGroup() Construction Group isn't open/valid [%s]",m_Owner->GetDebugName()));

	if (m_TimeWarp.m_ConstructionGroup != NULL)
	{
		m_TimeWarp.m_ConstructionGroup->AddToGroup(this,chore,stance,i,relw,forcecyclic);
	}
}


//************************************************************
bool
Blender::UpdateTimeWarp()
{
	m_TimeWarp.Recalc(this);
	return true;
}

//************************************************************
float
Blender::DurationOfCurrentTimeWarp(void) const
{
	return m_TimeWarp.m_TotalTime;
}

//************************************************************
float
Blender::TimeOfCurrentTimeWarp(void)
{

	if (!m_TimeWarp.m_Intervals.empty())
	{
		if ( m_TimeWarp.m_CurrentInterval !=  m_TimeWarp.m_Intervals.end() )
		{
			const float win = (*m_TimeWarp.m_CurrentInterval).m_WeightedDuration;
			const float itm = m_TimeWarp.m_IntervalTime;
			const float ctm = (*m_TimeWarp.m_CurrentInterval).m_WeightedTime;

			return ctm+(win*itm);
		}
	}
	return 0.0f;
}

//************************************************************
float
Blender::GetActiveAnimTime(DWORD i) const
{
	return (i < m_ActiveAnims.size()) ? m_ActiveAnims[i]->GetCurrentNormalizedTime() : 0;
}

//************************************************************
float
Blender::GetActiveAnimWeight(DWORD i) const
{
	return (i < m_ActiveAnims.size()) ? m_ActiveAnims[i]->GetBlendWeight() : 0;
}

//************************************************************
const KEventMap*
Blender::GetActiveAnimEvents(DWORD i)
{

	return (i < m_ActiveAnims.size()) ? &m_ActiveAnims[i]->GetCriticalEvents() : NULL;

}

//************************************************************
DWORD
Blender::GetUpcomingEvents()
{
	if (!m_TimeWarp.m_Intervals.empty())
	{
		if ( m_TimeWarp.m_CurrentInterval !=  m_TimeWarp.m_Intervals.end() )
		{
			return (*m_TimeWarp.m_CurrentInterval).m_RHSTags;
		}
	}

	return 0;

}

//************************************************************
DWORD
Blender::GetRecentEvents() const
{
	return m_RecentEvents;
}

//************************************************************
DWORD
Blender::GetNumberOfEvents(eAnimEvent ev) const
{
	return m_TimeWarp.CountEvents(ev);
}

//************************************************************
bool
Blender::IsCurrentlyAnimating()
{
	return ( !::IsEqual( TimeOfCurrentTimeWarp(), DurationOfCurrentTimeWarp() ) );
}

//************************************************************
bool
Blender::RemoveAllActiveAnims(void)
{
	m_ActiveAnims.clear();
	m_TimeWarp.Clear();
	return true;
}

//************************************************************
const TracerKeys* Blender::GetTracerKeys(void) const
{
	// $$$$ Fast hack to get at the tracer key data
	return m_ActiveAnims.empty() ? NULL : (*m_ActiveAnims.begin())->GetTracerKeys()  ;
}

