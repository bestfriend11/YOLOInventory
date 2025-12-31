#pragma once
//////////////////////////////////////////////////////////////////////////////
//
// File     :  NEMA_KEVENTS.H
// Author(s):  Mike Biddlecombe
//
// Summary  : Definition of Critical Events used to define animation sections
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#ifndef NEMA_KEVENTS
#define NEMA_KEVENTS

#include <map>
#include <list>
#include <deque>

enum eAnimEvent
{
	SET_BEGIN_ENUM( ANIMEVENT_, 0 ),

	ANIMEVENT_START,
	ANIMEVENT_FINISH,

	ANIMEVENT_GROUP_START,
	ANIMEVENT_GROUP_FINISH,

	ANIMEVENT_BEGIN_LOOP,
	ANIMEVENT_END_LOOP,

	ANIMEVENT_LEFT_FOOT_DOWN,
	ANIMEVENT_RIGHT_FOOT_DOWN,

	ANIMEVENT_ATTACH_AMMO,
	ANIMEVENT_FIRE_WEAPON,

	ANIMEVENT_BEGIN_SWING,
	ANIMEVENT_END_SWING,

	ANIMEVENT_SFX_1,
	ANIMEVENT_SFX_2,
	ANIMEVENT_SFX_3,
	ANIMEVENT_SFX_4,

	ANIMEVENT_DIE,

	ANIMEVENT_TRANSITION_BEGIN,
	ANIMEVENT_TRANSITION_END,

	ANIMEVENT_HIDE_MESH,
	ANIMEVENT_SHOW_MESH,

	ANIMEVENT_ERROR,

	SET_END_ENUM( ANIMEVENT_ ),


};

const char* ToString( eAnimEvent ch );
bool FromString( const char* str, eAnimEvent& ch );

eAnimEvent AnimEventFromString( const char* str );
DWORD AnimEventMaskFromFourCC( DWORD fourCC );
gpstring AnimEventMaskToString(DWORD ev);

FUBI_EXPORT bool AnimEventBitTest( DWORD dw, eAnimEvent ev );

namespace nema {

	//fwd decl
	class Blender;

	typedef DWORD KEventTags;

	//-------------------------------------------------------

	const float KEVENT_RESOLUTION = 10000.0f;		// This will allow 10k events different events in anim

	class KEventMap
	{

	public :

		typedef stdx::linear_map<int, KEventTags>	Container;
		typedef Container::iterator					Iter;
		typedef Container::const_iterator			ConstIter;
		typedef Container::reverse_iterator			RevIter;
		typedef std::pair <ConstIter, bool>			Ret;

		DWORD NumEvents(void)					{ return m_Container.size(); }
		bool AddEvent(float t, DWORD ev);
		bool MakeFirstMatchLast(void);

		void DebugDump(void) const;
		void StatusDump(gpstring& out) const;

		const Iter Begin()				{ return m_Container.begin();		}
		const Iter End() 				{ return m_Container.end();			}
		int	Size() const				{ return m_Container.size();		}
		const Iter Find(float t)		{ return m_Container.find((int)(t*KEVENT_RESOLUTION));	}

		float Time(ConstIter i)	const			{ return (*i).first/KEVENT_RESOLUTION;	}
		KEventTags Tags(ConstIter i) const		{ return (*i).second;					}

	private :

		Iter		m_itCurrent;

		Container	m_Container;

	};

	//-------------------------------------------------------

	class BlendGroup {

		struct KeyInfo
		{
				
			KeyInfo(void) 
				: m_Chore(0)
				, m_Stance(0)
				, m_Index(0)
				, m_RelativeWeight(0)
			{};

			KeyInfo(DWORD c ,DWORD s, DWORD i) 
				: m_Chore(c)
				, m_Stance(s)
				, m_Index(i)
				, m_RelativeWeight(0)
			{};

			bool Xfer( FuBi::PersistContext& persist );

			PRSKeys* GetKeys(Blender* b) const;

			DWORD	m_Chore;
			DWORD	m_Stance;
			DWORD	m_Index;

			float	m_RelativeWeight;

		};
	
	public:

		// Need to have some sort of function to allow the
		// warp group weight to vary over time
		float						m_GroupWeight;
		float						m_NormalizedWeight;
		float						m_CurrentLHSWeight;
		float						m_CurrentRHSWeight;
		bool						m_IsCyclic;
		bool						m_IsStale;
		stdx::fast_vector<KeyInfo>	m_KeyInfo;

		~BlendGroup() {
			m_KeyInfo.clear();
		}

		bool Xfer( FuBi::PersistContext& persist );

		FUBI_EXPORT bool AddToGroup(Blender* b,DWORD c, DWORD s, DWORD i, float w, bool fc);
		void ReNormalize();

#if !GP_RETAIL
		void DebugDump(Blender* b);
		void StatusDump(Blender* b, gpstring& out);
#endif

		float GetWeight(DWORD index, float t) const;

		float GetRelativeWeight(DWORD index) const;
		float SetRelativeWeight(DWORD index, float w);

		void	SetGroupWeight(float w)			{ m_GroupWeight = w;	}
		float	GetGroupWeight(void) const		{ return m_GroupWeight;	}
	};

	typedef std::list<BlendGroup*>	BlendGroupPtrList;

	//-------------------------------------------------------

	struct WarpBlend {

		BlendGroup*	m_pBlendGroup;
		float		m_LHSWeight;
		float		m_RHSWeight;

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::list<WarpBlend>	WarpBlendList;

	//-------------------------------------------------------

	struct WarpSegment {

		BlendGroup*	m_pBlendGroup;
		DWORD		m_BlendGroupIndex;
		float		m_Time;
		float		m_Interval;
		KEventTags	m_LHSTag;
		KEventTags	m_RHSTag;

		bool Xfer( FuBi::PersistContext& persist );
	};

	typedef std::list<WarpSegment>	WarpSegmentList;

	//-------------------------------------------------------

	class WarpInterval {

	public:

		~WarpInterval() {
			m_Segments.clear();
			};

		bool Xfer( FuBi::PersistContext& persist );

		KEventTags		m_LHSTags;
		KEventTags		m_RHSTags;
		float			m_WeightedTime;
		float			m_WeightedDuration;
		float			m_WeightedTimeScale;

		WarpSegmentList	m_Segments;
		WarpBlendList	m_Blends;

#if !GP_RETAIL
		void DebugDump(Blender* b) const;
		void StatusDump(Blender* b,gpstring& out) const;
#endif

		void AddGroupBlend(BlendGroup* pbg,float lhs,float rhs, bool init_current = true);
		void UpdateGroupBlends();

	};

	typedef std::list<WarpInterval>	WarpIntervalList;

	//-------------------------------------------------------

	class TimeWarp
	{

	public:

		float						m_TotalTime;
		float						m_IntervalTime;
		WarpIntervalList::iterator	m_CurrentInterval;

		WarpIntervalList			m_Intervals;
		BlendGroupPtrList			m_Groups;
		BlendGroup*					m_ConstructionGroup;
		bool						m_IsIdle;

		TimeWarp() {
			Clear();
		}

		~TimeWarp() {
			Clear();
		}

		bool Xfer( FuBi::PersistContext& persist );

		void Clear() {
			for (BlendGroupPtrList::iterator i = m_Groups.begin(); i!=m_Groups.end();++i) 
			{
				delete (*i);
			}
			m_Groups.clear();
			m_Intervals.clear();
			m_TotalTime = 0.0f;
			m_IntervalTime = 0.0f;
			m_CurrentInterval = m_Intervals.end();
			m_ConstructionGroup = NULL;
			m_IsIdle = true;
		}

		BlendGroup* OpenConstructionGroup();
		void CloseConstructionGroup(Blender *b, bool merge);
		void CloseConstructionGroupWithTransition(Blender *b, float transition_time, bool append);

		bool AttachConstructionGroup(Blender *b, bool merge);
		bool TransitionToConstructionGroup(Blender *b, float transtime,bool append);

		void SetGroupWeight(Blender*b, DWORD g, float w);
		float GetGroupWeight(DWORD g) const;

		void SetGroupAnimWeight(DWORD g, DWORD i, float w);
		float GetGroupAnimWeight(DWORD g, DWORD i) const;

		bool Recalc(Blender *b);

		KEventTags UpdateInterval(Blender *b, float deltat);

		void RemoveStale(Blender *b);

		DWORD CountEvents(eAnimEvent ev) const;

		bool IsIdle(void)		{ return m_IsIdle; }

#if !GP_RETAIL
		void DebugDump(Blender *b);
		void StatusDump(Blender *b, gpstring& out);
#endif

	private:

		bool CreateFromConstructionGroup(Blender *b);
		bool AttachFromConstructionGroup(Blender *b);
		bool AppendFromConstructionGroup(Blender *b);

	};

	inline bool BitTest(const DWORD m, const DWORD b) {return (m & (1<<b))!=0;}

};

#endif