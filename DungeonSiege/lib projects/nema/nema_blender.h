#pragma once
/*========================================================================

	NeMa_Blender.h

	A Blender provides an interface between an Aspect and its ActiveChores

	It is the keeper of the TimeWarp, the set of ActiveAnims, and the deformed
	bone positions
	

  author: Mike Biddlecombe
  date: 6/13/2001
  (c)opyright 2001, Gas Powered Games

  -----

  todo:


------------------------------------------------------------------------*/
#ifndef NEMA_BLENDER_H
#define NEMA_BLENDER_H

#include "nema_chore.h"

namespace nema {

	// Fwd Decl
	class TracerKeys;
	class KEventMap;

	class Blender 
	{

	public:

		Blender(Aspect *own);
		~Blender();

		bool Xfer( FuBi::PersistContext& persist );
		bool XferPost( const ChoreDictionary& chores );
		void CommitCreation();

		void SetOwner(Aspect *own);

		void AddChore(const Chore* nc,bool shouldPersist);

		bool HasValidChore(eAnimChore c)				{ return m_ActiveChoreArray[c] != NULL; }
		bool HasValidSkrit(eAnimChore c)				{ return m_ActiveChoreArray[c] ? m_ActiveChoreArray[c]->m_Skrit.IsValid() : false; }
		Skrit::HAutoObject& GetSkrit(eAnimChore c)		{ return m_ActiveChoreArray[c]->m_Skrit; }

		DWORD GetStanceMask(eAnimChore c)				{ return m_ActiveChoreArray[c] ? m_ActiveChoreArray[c]->m_lStanceMask : 0; }

		FUBI_EXPORT DWORD NumActiveAnims(void) const	{ return m_ActiveAnims.size(); }

		FUBI_EXPORT float DurationOfCurrentTimeWarp(void) const;
		FUBI_EXPORT float TimeOfCurrentTimeWarp(void);

		FUBI_EXPORT DWORD OpenBlendGroup(void)						    { m_RecentEvents = 0; return (DWORD)m_TimeWarp.OpenConstructionGroup();}
		FUBI_EXPORT void CloseBlendGroup()							    { m_TimeWarp.CloseConstructionGroup(this,false);	}
		FUBI_EXPORT void CloseBlendGroup(bool merge)				    { m_TimeWarp.CloseConstructionGroup(this,merge);	}
		FUBI_EXPORT void CloseBlendGroupWithTransition(float t, bool a)	{ m_TimeWarp.CloseConstructionGroupWithTransition(this,t,a); }

		FUBI_EXPORT void AddAnimToBlendGroup(DWORD i,float relw, bool f);
		FUBI_EXPORT void AddAnimToBlendGroup(DWORD i,float relw)		{ AddAnimToBlendGroup(i,relw,false);	}

		FUBI_EXPORT void SetBlendGroupWeight(DWORD g,float gw)		{ m_TimeWarp.SetGroupWeight(this,g,gw);		}
		FUBI_EXPORT float GetBlendGroupWeight(DWORD g)				{ return m_TimeWarp.GetGroupWeight(g);	}

		FUBI_EXPORT void SetBlendGroupAnimWeight(DWORD g,DWORD i,float relw)	{ m_TimeWarp.SetGroupAnimWeight(g,i,relw);	 }
		FUBI_EXPORT float GetBlendGroupAnimWeight(DWORD g,DWORD i)				{ return m_TimeWarp.GetGroupAnimWeight(g,i); }

		float GetActiveAnimTime(DWORD i) const;
		float GetActiveAnimWeight(DWORD i) const;
		const KEventMap* GetActiveAnimEvents(DWORD i);
		DWORD GetUpcomingEvents();
FEX		DWORD GetRecentEvents() const;
FEX		DWORD GetNumberOfEvents(eAnimEvent ev) const;

		bool IsCurrentlyAnimating();

		FUBI_EXPORT void ResetTimeWarp() { 
			m_RecentEvents = 0;
			m_TimeWarp.Clear();
			m_ActiveAnims.clear();
		}

		FUBI_EXPORT bool UpdateTimeWarp();
		FUBI_EXPORT bool IsIdle()				{ return m_TimeWarp.IsIdle(); }

		DWORD UpdateActiveAnims(float time);	// Interpolate keys in each active anim

		bool BlendActiveAnims(DWORD firstbone);	// Interpolate between all active anims
												// setting the m_BonePositions & m_BoneRotations
		bool RemoveAllActiveAnims(void);

		const vector_3& BlendedRootVelocity() const				{ return m_RootVelocity;	}

		const vector_3& BlendedBonePosition(DWORD b) const	{ return m_BonePositions[b]; }
		const Quat& BlendedBoneRotation(DWORD b) const		{ return m_BoneRotations[b]; }

FEX		const gpstring& GetBlendedPRSDebugName(DWORD i);

FEX		int   GetNumSubAnims   ( eAnimChore c, eAnimStance s) const;
FEX		int   GetSubAnimIndex  ( eAnimChore c, DWORD fourCC ) const;
FEX		float GetScalarVelocity( eAnimChore c, eAnimStance s, DWORD i) const;
FEX		float GetDuration      ( eAnimChore c, eAnimStance s, DWORD i) const;
FEX		float GetBaseDuration  ( eAnimChore c, eAnimStance s) const;
FEX		int	  GetMinDurationAnim( eAnimChore c, eAnimStance s) const;

		void ProcessHideShowEventsForSkippedChore( eAnimChore chore, eAnimStance stance,DWORD subanim ) const;

        PRSKeys* GetKeys       ( eAnimChore c, eAnimStance s, DWORD i) const;

		// Wrapper functions that assume we want info on the current chore & stance
		FUBI_EXPORT int   GetNumSubAnims	( void ) const				{ return GetNumSubAnims(m_Owner->GetCurrentChore(),m_Owner->GetCurrentStance());			}
		FUBI_EXPORT int   GetSubAnimIndex	( DWORD fourCC ) const		{ return GetSubAnimIndex(m_Owner->GetCurrentChore(),fourCC);								}
		FUBI_EXPORT float GetScalarVelocity	( DWORD i) const			{ return GetScalarVelocity(m_Owner->GetCurrentChore(),m_Owner->GetCurrentStance(),i);		}
		FUBI_EXPORT float GetDuration       ( DWORD i) const			{ return GetDuration(m_Owner->GetCurrentChore(),m_Owner->GetCurrentStance(),i);				}
		FUBI_EXPORT float GetBaseDuration	( void ) const				{ return GetBaseDuration(m_Owner->GetCurrentChore(),m_Owner->GetCurrentStance());			}

		FUBI_EXPORT bool GetSubAnimsAreLoaded( eAnimChore chore, eAnimStance stance ) const;

		bool IsolateActiveAnim(DWORD i, DWORD firstbone);

#if !GP_RETAIL
		void StatusDump( gpstring& s );
#endif

		const TracerKeys*  GetTracerKeys() const;

		void SetTracePoint0(const vector_3& p)		{ m_TracePoint0 = p; }
		void SetTracePoint1(const vector_3& p)		{ m_TracePoint1 = p; }

		const vector_3& GetTracePoint0(void) const	{ return m_TracePoint0; }
		const vector_3& GetTracePoint1(void) const	{ return m_TracePoint1; }

		bool IsDirty(void) const					{ return m_IsDirtyFlag; }
		void SetDirtyFlag(bool f)					{ m_IsDirtyFlag = f;	}

	private:

		typedef stdx::fast_vector <my ActiveChore*> ActiveChoreArray;
		ActiveChore* CreateActiveChore(Aspect *own,const Chore* nc, bool shouldPersist);

		Aspect*								m_Owner;

		vector_3							m_RootVelocity;
		DWORD								m_RecentEvents;	// Events that have taken place since
																// Timewarp was reset

		stdx::fast_vector<vector_3>			m_BonePositions;
		stdx::fast_vector<Quat>				m_BoneRotations;

		PRSKeyArray							m_ActiveAnims;
		ActiveChoreArray					m_ActiveChoreArray;
		TimeWarp							m_TimeWarp;

		vector_3							m_TracePoint0;
		vector_3							m_TracePoint1;
										
		bool								m_IsDirtyFlag;	// For caching blender updates
															// Is set to true if an active animation changes
	};
};

#endif