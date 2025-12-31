#include "precomp_nema.h"

#include "filter_1.h"
#include "fubipersist.h"
#include "fubitraits.h"
#include "nema_blender.h"
#include "nema_kevents.h"
#include "nema_persist.h"
#include "stringtool.h"

#include <algorithm>

FUBI_EXPORT_ENUM( eAnimEvent, ANIMEVENT_BEGIN, ANIMEVENT_COUNT );
FUBI_REPLACE_NAME("nemaBlendGroup",BlendGroup);

using namespace nema;

static const char* s_AnimEventStrings[] =
{
	"ANIMEVENT_START",
	"ANIMEVENT_FINISH",

	"ANIMEVENT_GROUP_START",
	"ANIMEVENT_GROUP_FINISH",

	"ANIMEVENT_BEGIN_LOOP",
	"ANIMEVENT_END_LOOP",

	"ANIMEVENT_LEFT_FOOT_DOWN",
	"ANIMEVENT_RIGHT_FOOT_DOWN",

	"ANIMEVENT_ATTACH_AMMO",
	"ANIMEVENT_FIRE_WEAPON",

	"ANIMEVENT_BEGIN_SWING",
	"ANIMEVENT_END_SWING",

	"ANIMEVENT_SFX_1",
	"ANIMEVENT_SFX_2",
	"ANIMEVENT_SFX_3",
	"ANIMEVENT_SFX_4",

	"ANIMEVENT_DIE",

	"ANIMEVENT_TRANSITION_BEGIN",
	"ANIMEVENT_TRANSITION_END",

	"ANIMEVENT_HIDE_MESH",
	"ANIMEVENT_SHOW_MESH",

	"ANIMEVENT_ERROR"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_AnimEventStrings ) == ANIMEVENT_COUNT );

static const DWORD s_AnimEventFourCC[] =
{
	REVERSE_FOURCC('STRT'),
	REVERSE_FOURCC('FINI'),

	REVERSE_FOURCC('GSRT'),
	REVERSE_FOURCC('GFIN'),

	REVERSE_FOURCC('BEGL'),
	REVERSE_FOURCC('ENDL'),

	REVERSE_FOURCC('LFDN'),
	REVERSE_FOURCC('RFDN'),

	REVERSE_FOURCC('ATTA'),
	REVERSE_FOURCC('FIRE'),

	REVERSE_FOURCC('BSWG'),
	REVERSE_FOURCC('ESWG'),

	REVERSE_FOURCC('SFX1'),
	REVERSE_FOURCC('SFX2'),
	REVERSE_FOURCC('SFX3'),
	REVERSE_FOURCC('SFX4'),

	REVERSE_FOURCC('DEAD'),

	REVERSE_FOURCC('TBEG'),
	REVERSE_FOURCC('TEND'),

	REVERSE_FOURCC('HIDE'),
	REVERSE_FOURCC('SHOW'),

	REVERSE_FOURCC('AERR')
};

COMPILER_ASSERT( ELEMENT_COUNT( s_AnimEventFourCC ) == ANIMEVENT_COUNT );

static stringtool::EnumStringConverter s_AnimEventConverter( s_AnimEventStrings, ANIMEVENT_BEGIN, ANIMEVENT_END, ANIMEVENT_START);

const char* ToString( eAnimEvent ch )
	{  return ( s_AnimEventConverter.ToString( ch ) );  }
bool FromString( const char* str, eAnimEvent& ch )
	{  return ( s_AnimEventConverter.FromString( str, rcast <int&> ( ch ) ) );  }

eAnimEvent AnimEventFromString( const char* str )
{
	eAnimEvent ev = ANIMEVENT_START;
	FromString( str, ev );
	return ( ev );
}

DWORD AnimEventMaskFromFourCC( DWORD fourCC )
{
	DWORD mask = 1;
	for ( DWORD ev= 0; ev < ELEMENT_COUNT( s_AnimEventFourCC ); ev++,mask<<=1) {
		if (s_AnimEventFourCC[ev] == fourCC) return mask;
	}
	return 0;
}

FUBI_EXPORT bool AnimEventBitTest( DWORD dw, eAnimEvent ev ) {
	return BitTest(dw,ev);
}

gpstring AnimEventMaskToString(DWORD evm) {

	gpstring out;

	if (evm == 0) {
		out = "UNKN";
	} else {
		for (DWORD tagi = 0; tagi < 32; tagi++) {
			if (BitTest(evm,tagi)) {
				if (out.length() > 0 ) {
					out.append(" | ");
				}
				out.appendf(stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str());
			}
		}
	}
	return out;

}

//************************************************************
bool
KEventMap::AddEvent(float t, DWORD ev)	{


	gpassertf(t>=0.0f && t <=1.0f, ("Invalid event time %f",t))

	t = (t<0.0f) ? 0.0f : t    ;
	t = (t<1.0f) ? t    : 1.0f ;

	int kt = (int)(t*KEVENT_RESOLUTION);

	KEventMap::Iter ne = m_Container.find(kt);

	if (ne == m_Container.end()) {


		// This is the first event at this time
		KEventTags newkp = AnimEventMaskFromFourCC(ev);
		m_Container.insert(std::make_pair(kt,newkp));
		return true;

	} else {

		// Add this new FourCC to the eventtag
		(*ne).second |= AnimEventMaskFromFourCC(ev);

	}

	return false;

}

//************************************************************
bool
KEventMap::MakeFirstMatchLast(void)  {

	if ( ((*m_Container.begin()).second != (*m_Container.rbegin()).second) ||
		 ((*m_Container.rbegin()).first != KEVENT_RESOLUTION)) {

		m_Container.insert(std::make_pair( (int)KEVENT_RESOLUTION , (*m_Container.begin()).second) );

		return true;

	}
	return false;
}

//************************************************************
void
KEventMap::DebugDump(void) const {

	for (ConstIter kp = m_Container.begin(); kp!= m_Container.end(); ++kp) {

		gpgenericf(("Time %f: ",(*kp).first));

		for (DWORD tagi = 0; tagi < 32; tagi++) {
			if ((*kp).second & (1<<tagi)) {
				gpgenericf(("%s ",stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str()));
			}
		}

		gpgeneric("\n");
	}

}

FUBI_DECLARE_SELF_TRAITS( WarpInterval );

FUBI_DECLARE_XFER_TRAITS( BlendGroup* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new BlendGroup;
		}

		return ( obj->Xfer( persist ) );
	}
};

//************************************************************
bool
TimeWarp::Xfer( FuBi::PersistContext& persist ) {

	persist.Xfer( "m_TotalTime", m_TotalTime );
	persist.Xfer( "m_IntervalTime", m_IntervalTime );

	persist.XferList( "m_Intervals", m_Intervals );
	persist.XferList( "m_Groups",    m_Groups    );

	size_t distance = std::distance( m_Intervals.begin(), m_CurrentInterval );
	persist.Xfer( "m_CurrentInterval", distance );
	if ( persist.IsRestoring() )
	{
		m_CurrentInterval = m_Intervals.begin();
		std::advance( m_CurrentInterval, distance );
	}

	XferCollPtr( persist, "m_ConstructionGroup", m_Groups, m_ConstructionGroup );

	persist.EnterColl( "m_Intervals" );
	WarpIntervalList::iterator i, ibegin = m_Intervals.begin(), iend = m_Intervals.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		persist.AdvanceCollIter();

		persist.EnterColl( "m_Segments" );
		{
			WarpSegmentList::iterator j, jbegin = i->m_Segments.begin(), jend = i->m_Segments.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				persist.AdvanceCollIter();
				XferCollPtr( persist, "m_pBlendGroup", m_Groups, j->m_pBlendGroup );
			}
		}
		persist.LeaveColl();

		persist.EnterColl( "m_Blends" );
		{
			WarpBlendList::iterator j, jbegin = i->m_Blends.begin(), jend = i->m_Blends.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				persist.AdvanceCollIter();
				XferCollPtr( persist, "m_pBlendGroup", m_Groups, j->m_pBlendGroup );
			}
		}
		persist.LeaveColl();
	}
	persist.LeaveColl();

	return ( true );
}

//************************************************************
bool
TimeWarp::AttachConstructionGroup(Blender *b, bool merge) {

	if ((m_Intervals.size() == 0) || merge) {
		CreateFromConstructionGroup(b);
	} else {
		//AttachFromConstructionGroup();
		AppendFromConstructionGroup(b);
	}

	return true;


}

//************************************************************
bool
TimeWarp::CreateFromConstructionGroup(Blender *b) {

	gpassertf(m_ConstructionGroup != NULL, ("CreateFromConstructionGroup() : There is no construction group to  create with"));
	if (m_ConstructionGroup == NULL) {
		return false;
	}

	for (DWORD k = 0; k < m_ConstructionGroup->m_KeyInfo.size(); ++k) {

		WarpIntervalList::iterator currinterv = m_Intervals.begin();

		PRSKeys* newkeys = m_ConstructionGroup->m_KeyInfo[k].GetKeys(b);

		if (newkeys->GetCriticalEvents().Size() == 0) {

			// This is a REALLY OLD anim

			if (currinterv == m_Intervals.end()) {

				// Need to add a new timewarp interval
				WarpInterval newinterv;

				newinterv.m_LHSTags = 0;
				newinterv.m_RHSTags = 0;
				newinterv.m_WeightedDuration = 0.0;
				newinterv.m_WeightedTime = 0.0;
				newinterv.m_WeightedTimeScale = 0.0;

				currinterv = m_Intervals.insert(currinterv,newinterv);

			}

			WarpSegment newseg;

			newseg.m_pBlendGroup = m_ConstructionGroup;
			newseg.m_BlendGroupIndex = k;
			newseg.m_LHSTag = 0;
			newseg.m_RHSTag = 0;
			newseg.m_Time = 0;
			newseg.m_Interval = 1;

			(*currinterv).m_Segments.push_back(newseg);

			if (k == 0) {
				float w = 1.0f/((*currinterv).m_Blends.size()+1.0f);
				(*currinterv).AddGroupBlend(m_ConstructionGroup,w,w);
				++currinterv;
			}

		} else {


			KEventMap::Iter currev = newkeys->GetCriticalEvents().Begin();
			KEventMap::Iter nextev = currev;

			++nextev;

			KEventMap& events = newkeys->GetCriticalEvents();

			while (nextev != events.End()) {

				if (currinterv == m_Intervals.end()) {

					// Need to add a new timewarp interval
					WarpInterval newinterv;

					newinterv.m_LHSTags = 0;
					newinterv.m_RHSTags = 0;
					newinterv.m_WeightedDuration = 0.0;
					newinterv.m_WeightedTime = 0.0;
					newinterv.m_WeightedTimeScale = 0.0;

					currinterv = m_Intervals.insert(currinterv,newinterv);

				}

				WarpSegment newseg;

				newseg.m_pBlendGroup = m_ConstructionGroup;
				newseg.m_BlendGroupIndex = k;
				newseg.m_LHSTag   = events.Tags(currev);
				newseg.m_RHSTag   = events.Tags(nextev);
				newseg.m_Time	  = events.Time(currev);
				newseg.m_Interval = events.Time(nextev)-newseg.m_Time;

				(*currinterv).m_LHSTags |= newseg.m_LHSTag;
				(*currinterv).m_RHSTags |= newseg.m_RHSTag;
				(*currinterv).m_Segments.push_back(newseg);

				if (k == 0) {
					float w = 1.0f/((*currinterv).m_Blends.size()+1.0f);
					(*currinterv).AddGroupBlend(m_ConstructionGroup,w,w);
				}

				++currinterv;
				currev = nextev;
				++nextev;
			}
		}


	}

	m_TotalTime = 0.0f;
	m_CurrentInterval = m_Intervals.begin();
	m_IntervalTime = 0.0f;

	Recalc(b);

	m_Groups.push_front(m_ConstructionGroup);
	m_ConstructionGroup = NULL;


	return true;

}

//************************************************************
bool
TimeWarp::AttachFromConstructionGroup(Blender *b) {

	gpassertf(m_ConstructionGroup != NULL, ("AttachFromConstructionGroup() : There is no construction group to  create with"));
	if (m_ConstructionGroup == NULL) {
		return false;
	}

	bool first_blend = true;

	for (DWORD k = 0; k < m_ConstructionGroup->m_KeyInfo.size(); ++k) {

		WarpIntervalList::iterator matchinterv = m_CurrentInterval;

		// Always start to attach at the NEXT interval not the current
		++matchinterv;
		if (matchinterv == m_Intervals.end()) {

			// Need to add a new timewarp interval to the end
			// that is a copy of the interval at the start
			WarpInterval newinterv;

			newinterv.m_LHSTags = 0;
			newinterv.m_RHSTags = 0;
			newinterv.m_WeightedDuration = 0.0;
			newinterv.m_WeightedTime = 0.0;
			newinterv.m_WeightedTimeScale = 0.0;

			for (WarpSegmentList::iterator seg = (*m_Intervals.begin()).m_Segments.begin();
				seg != (*m_Intervals.begin()).m_Segments.end();
				++seg) {

				if ((*seg).m_pBlendGroup->m_IsCyclic) {
					WarpSegment newseg;
					newseg.m_pBlendGroup		= (*seg).m_pBlendGroup;
					newseg.m_BlendGroupIndex	= (*seg).m_BlendGroupIndex;
					newseg.m_LHSTag				= (*seg).m_LHSTag;
					newseg.m_RHSTag				= (*seg).m_RHSTag;
					newseg.m_Time				= (*seg).m_Time;
					newseg.m_Interval			= (*seg).m_Interval;

					newinterv.m_Segments.push_back(newseg);

					newinterv.m_LHSTags			|= (*seg).m_LHSTag;
					newinterv.m_RHSTags			|= (*seg).m_RHSTag;

				}

			}

			for (WarpBlendList::iterator blend = (*m_Intervals.begin()).m_Blends.begin();
												blend != (*m_Intervals.begin()).m_Blends.end();
												++blend) {
				if ((*blend).m_pBlendGroup->m_IsCyclic) {
					newinterv.AddGroupBlend((*blend).m_pBlendGroup,1.0f,1.0f);
				}
			}

			if (newinterv.m_Segments.size() > 0 && newinterv.m_Blends.size() > 0) {
				matchinterv = m_Intervals.insert(matchinterv,newinterv);
			}

		}

		gpassertf(matchinterv != m_Intervals.end(), ("AttachFromConstructionGroup() : Ran out of intervals to match with"));

		PRSKeys* newkeys = m_ConstructionGroup->m_KeyInfo[k].GetKeys(b);

		gpassertf((newkeys), ("AttachFromConstructionGroup() : Trying to attach an invalid set of keys"));

		KEventMap::ConstIter mtchev;
		KEventMap::ConstIter nextev;

		bool matched = false;

		KEventMap& events = newkeys->GetCriticalEvents();
		mtchev = events.Begin();
		nextev = mtchev;
		++nextev;

		while (matchinterv != m_Intervals.end()) {

			if (m_ConstructionGroup->m_IsCyclic) {

				while (nextev != events.End()) {

					const DWORD lhsev = events.Tags(mtchev) & ((1<<ANIMEVENT_LEFT_FOOT_DOWN)|(1<<ANIMEVENT_RIGHT_FOOT_DOWN));
					const DWORD rhsev = events.Tags(nextev) & ((1<<ANIMEVENT_LEFT_FOOT_DOWN)|(1<<ANIMEVENT_RIGHT_FOOT_DOWN));

					matched = ((lhsev & (*matchinterv).m_LHSTags) == lhsev) &&
							  ((rhsev & (*matchinterv).m_RHSTags) == rhsev);

					if (matched) {
						break;
					}

					mtchev = nextev;
					++nextev;
				}

			} else {

				const DWORD lhsev = events.Tags(mtchev) & ((1<<ANIMEVENT_LEFT_FOOT_DOWN)|(1<<ANIMEVENT_RIGHT_FOOT_DOWN));
				const DWORD rhsev = events.Tags(nextev) & ((1<<ANIMEVENT_LEFT_FOOT_DOWN)|(1<<ANIMEVENT_RIGHT_FOOT_DOWN));

				matched = ((lhsev & (*matchinterv).m_LHSTags) == lhsev) &&
						  ((rhsev & (*matchinterv).m_RHSTags) == rhsev);

			}

			if (matched) {
				break;
			}

			++matchinterv;

			mtchev = events.Begin();
			nextev = mtchev;
			++nextev;

		}

		KEventMap::ConstIter currev;
		currev = mtchev;

		do {

			if (matchinterv == m_Intervals.end()) {

				// Need to add a new timewarp interval
				WarpInterval newinterv;

				newinterv.m_LHSTags = 0;
				newinterv.m_RHSTags = 0;
				newinterv.m_WeightedDuration = 0.0;
				newinterv.m_WeightedTime = 0.0;
				newinterv.m_WeightedTimeScale = 0.0;

				matchinterv = m_Intervals.insert(matchinterv,newinterv);

			}

			WarpSegment newseg;

			newseg.m_pBlendGroup = m_ConstructionGroup;
			newseg.m_BlendGroupIndex = k;
			newseg.m_LHSTag   = events.Tags(currev);
			newseg.m_RHSTag   = events.Tags(nextev);
			newseg.m_Time     = events.Time(currev);
			newseg.m_Interval = events.Time(nextev)-newseg.m_Time;


			(*matchinterv).m_LHSTags |= newseg.m_LHSTag;
			(*matchinterv).m_RHSTags |= newseg.m_RHSTag;
			(*matchinterv).m_Segments.push_back(newseg);

			if (k == 0) {
				(*matchinterv).AddGroupBlend(m_ConstructionGroup,first_blend ? 0.0f:1.0f, 1.0f);
				first_blend = false;
			}

			++matchinterv;

			currev = nextev;
			++nextev;

			if (nextev == newkeys->GetCriticalEvents().End()) {
				currev = newkeys->GetCriticalEvents().Begin();
				nextev = currev;
				++nextev;
			}

		} while (currev != mtchev);


	}

	Recalc(b);

	m_Groups.push_front(m_ConstructionGroup);
	m_ConstructionGroup = NULL;

	return true;
}

//************************************************************
bool
TimeWarp::AppendFromConstructionGroup(Blender *b) {

	gpassertf(m_ConstructionGroup != NULL, ("AttachFromConstructionGroup() : There is no construction group to  create with"));
	if (m_ConstructionGroup == NULL) {
		return false;
	}

	DWORD k;

	WarpIntervalList::iterator appendinterv = m_Intervals.end();
	--appendinterv;


	// Need to add a transition timewarp interval to the end
	// that is a copy of the last interval, but with all old groups
	// set to blend out and go stale and the new group to blend in

	WarpInterval transitioninterv;
	transitioninterv.m_LHSTags = 0;
	transitioninterv.m_RHSTags = 0;
	transitioninterv.m_WeightedDuration = 0.0;
	transitioninterv.m_WeightedTime = 0.0;
	transitioninterv.m_WeightedTimeScale = 0.0;

	for (WarpSegmentList::iterator seg = (*appendinterv).m_Segments.begin();
									seg != (*appendinterv).m_Segments.end();
									++seg)
	{

		WarpSegment newseg;
		newseg.m_pBlendGroup		= (*seg).m_pBlendGroup;
		newseg.m_BlendGroupIndex	= (*seg).m_BlendGroupIndex;
		newseg.m_LHSTag				= (*seg).m_RHSTag;
		newseg.m_RHSTag				= 0;
		newseg.m_Time				= (*seg).m_Time + (*seg).m_Interval;
		newseg.m_Interval			= 0;

		transitioninterv.m_Segments.push_back(newseg);
		transitioninterv.m_LHSTags |= (*seg).m_RHSTag;

	}

	for (k = 0; k < m_ConstructionGroup->m_KeyInfo.size(); ++k)
	{
		PRSKeys* newkeys = m_ConstructionGroup->m_KeyInfo[k].GetKeys(b);

		gpassertf(newkeys, ("AttachFromConstructionGroup() : Trying to attach an invalid set of keys"));

		KEventMap& events = newkeys->GetCriticalEvents();

		KEventMap::Iter currev = events.Begin();

		WarpSegment newseg;
		newseg.m_pBlendGroup = m_ConstructionGroup;
		newseg.m_BlendGroupIndex = k;
		newseg.m_LHSTag   = 0;
		newseg.m_RHSTag   = events.Tags(currev);
		newseg.m_Time     = 0;
		newseg.m_Interval = 0;

		transitioninterv.m_Segments.push_back(newseg);
		transitioninterv.m_RHSTags |= newseg.m_LHSTag;
	}



	for (WarpBlendList::iterator blend = (*appendinterv).m_Blends.begin();
									blend != (*appendinterv).m_Blends.end();
									++blend)
	{
		// Old groups go to 0%
		transitioninterv.AddGroupBlend((*blend).m_pBlendGroup,1.0f,0.0f,false);
	}

	// New group goes to 100%
	transitioninterv.AddGroupBlend(m_ConstructionGroup,0.0f,1.0f);

	// Append the transition interval
	m_Intervals.insert(m_Intervals.end(),transitioninterv);

	for (k = 0; k < m_ConstructionGroup->m_KeyInfo.size(); ++k)
	{
		PRSKeys* newkeys = m_ConstructionGroup->m_KeyInfo[k].GetKeys(b);

		gpassertf(newkeys, ("AttachFromConstructionGroup() : Trying to attach an invalid set of keys"));

		KEventMap& events = newkeys->GetCriticalEvents();

		KEventMap::Iter currev = events.Begin();
		KEventMap::Iter nextev = currev;
		++nextev;

		for ( ; nextev != events.End(); ++nextev ) 
		{
			WarpInterval newinterv;

			newinterv.m_LHSTags = 0;
			newinterv.m_RHSTags = 0;
			newinterv.m_WeightedDuration = 0.0;
			newinterv.m_WeightedTime = 0.0;
			newinterv.m_WeightedTimeScale = 0.0;

			WarpIntervalList::iterator lastintervit = m_Intervals.insert(m_Intervals.end(),newinterv);

			WarpSegment newseg;

			newseg.m_pBlendGroup = m_ConstructionGroup;
			newseg.m_BlendGroupIndex = k;
			newseg.m_LHSTag   = events.Tags(currev);
			newseg.m_RHSTag   = events.Tags(nextev);
			newseg.m_Time     = events.Time(currev);
			newseg.m_Interval = events.Time(nextev)-newseg.m_Time;

			(*lastintervit).m_LHSTags |= newseg.m_LHSTag;
			(*lastintervit).m_RHSTags |= newseg.m_RHSTag;
			(*lastintervit).m_Segments.push_back(newseg);

			if (k == 0) {
				(*lastintervit).AddGroupBlend(m_ConstructionGroup,1.0f, 1.0f);
			}

			currev = nextev;

		}

	}

	Recalc(b);

	m_Groups.push_front(m_ConstructionGroup);
	m_ConstructionGroup = NULL;

	return true;
}

//************************************************************
bool
TimeWarp::TransitionToConstructionGroup(Blender *b, float reqtranstime, bool append)
{

	// Prep the groups in the timewarp:
	//
	//  Split the current interval by inserting a TRANSITION_BEGIN
	//
	//  Collapse all remaining intervals into a single interval that ends with TRANSITION_END
	//  the interval must be 'transtime' in duration
	//
	//
	// Prep the construction group:
	//  
	//  Add a TRANSITION_BEGIN event at the start of the anim
	// 
	//  Insert a TRANSITION_END event and split any segments in the construction group
	//
	// 
	// Overlay the construction group with a smooth transition from TBEG to TEND

	bool ret = false;

	// Are we right at the end of the current timewarp?
	if ( m_CurrentInterval != m_Intervals.end() && IsEqual(m_IntervalTime,1.0f,0.001f) )
	{
		++m_CurrentInterval;
		m_IntervalTime = 0;
	}

	if ( m_CurrentInterval == m_Intervals.end() )
	{
		// Nothing to transition to!
		for (BlendGroupPtrList::iterator i = m_Groups.begin(); i!=m_Groups.end();++i) 
		{
			delete (*i);
		}
		m_Groups.clear();
		m_Intervals.clear();
		m_TotalTime = 0.0f;
		m_IntervalTime = 0.0f;
		m_CurrentInterval = m_Intervals.end();
		ret = CreateFromConstructionGroup(b);
	}
	else 
	{

		{
	
			WarpIntervalList::iterator spliceinterv;
			WarpBlendList::iterator blend;
			float splicetime;

			if (append)
			{
				// We need to locate the splice interval that we will split to create the transition begin interval
				spliceinterv = m_Intervals.end();
				--spliceinterv;
				float remainingtime;

				if ((spliceinterv == m_CurrentInterval) || (spliceinterv == m_Intervals.begin()))
				{
					// We can't back up any further, the current interval is already being displayed
					// Allow whatever time remains in the current interval to be used too
					remainingtime = (*spliceinterv).m_WeightedDuration * (1-m_IntervalTime);
				} 
				else 
				{
					remainingtime = (*spliceinterv).m_WeightedDuration;
					while ((reqtranstime > remainingtime))
					{
						--spliceinterv;
						if ((spliceinterv == m_CurrentInterval) || (spliceinterv == m_Intervals.begin()))
						{
							// We can't back up any further, the current interval is already being displayed
							// Allow whatever time remains in the current interval to be used too
							remainingtime += (*spliceinterv).m_WeightedDuration * (1-m_IntervalTime);
							break;
						} 
						remainingtime += (*spliceinterv).m_WeightedDuration;
					}
				}

				if (reqtranstime <= remainingtime)
				{
					splicetime = (remainingtime-reqtranstime)/(*spliceinterv).m_WeightedDuration;
				}
				else // spliceinterv == m_CurrentInterval
				{
					reqtranstime = remainingtime;
					splicetime = m_IntervalTime;
				}

			}
			else
			{
				spliceinterv = m_CurrentInterval;
				splicetime = m_IntervalTime;
				m_IntervalTime = 0;
			}

			gpassert(splicetime>=0);
			gpassert(splicetime<=1);

			// Do we need to split this interval into the section we have 
			// already run through, and the remainders
			if ( IsZero(splicetime) )
			{
				// Insert the half segments that define the time just prior to the transition begin event
				for (WarpSegmentList::iterator seg = (*spliceinterv).m_Segments.begin();
												seg != (*spliceinterv).m_Segments.end();
												++seg)
				{
					(*seg).m_LHSTag	|= 1<<ANIMEVENT_TRANSITION_BEGIN;
				}

				(*spliceinterv).m_LHSTags |= 1<<ANIMEVENT_TRANSITION_BEGIN;
			}
			else
			{

				WarpInterval newinterv;
				newinterv.m_LHSTags = 0;
				newinterv.m_RHSTags = 0;

				WarpIntervalList::iterator transbegininterv = m_Intervals.insert(spliceinterv,newinterv);

				if (append)
				{
					if (spliceinterv == m_CurrentInterval)
					{
						m_CurrentInterval = transbegininterv;
						m_IntervalTime *= splicetime;
					}
				}

				// Insert the half segments that define the time just prior to the transition begin event
				for (WarpSegmentList::iterator seg = (*spliceinterv).m_Segments.begin();
												seg != (*spliceinterv).m_Segments.end();
												++seg)
				{

					WarpSegment newseg;
					newseg.m_pBlendGroup		= (*seg).m_pBlendGroup;
					newseg.m_BlendGroupIndex	= (*seg).m_BlendGroupIndex;
					newseg.m_LHSTag				= (*seg).m_LHSTag;
					newseg.m_RHSTag				= 1<<ANIMEVENT_TRANSITION_BEGIN;
					newseg.m_Time				= (*seg).m_Time;
					newseg.m_Interval			= (*seg).m_Interval * splicetime;

					(*seg).m_LHSTag				= 1<<ANIMEVENT_TRANSITION_BEGIN;
					(*seg).m_Time			    += newseg.m_Interval;
					(*seg).m_Interval			-= newseg.m_Interval;

					(*transbegininterv).m_Segments.push_back(newseg);
					(*transbegininterv).m_LHSTags |= newseg.m_LHSTag;

				}

				(*transbegininterv).m_RHSTags = 1<<ANIMEVENT_TRANSITION_BEGIN;
				(*spliceinterv).m_LHSTags = 1<<ANIMEVENT_TRANSITION_BEGIN;

				// Interpolate the blending for the transition begin interval
				for (blend = (*spliceinterv).m_Blends.begin();
					 blend != (*spliceinterv).m_Blends.end();
					 ++blend)
				{
					WarpBlend newblend;
					newblend.m_pBlendGroup = (*blend).m_pBlendGroup;
					newblend.m_LHSWeight = (*blend).m_LHSWeight;
					newblend.m_RHSWeight = (*blend).m_LHSWeight + (splicetime)*((*blend).m_RHSWeight - (*blend).m_LHSWeight);
					(*blend).m_LHSWeight = newblend.m_RHSWeight;
					(*transbegininterv).m_Blends.push_back(newblend);
				}

				(*transbegininterv).m_WeightedTime = (*spliceinterv).m_WeightedTime * splicetime;
				(*transbegininterv).m_WeightedDuration = (*spliceinterv).m_WeightedDuration * splicetime;
				(*transbegininterv).m_WeightedTimeScale = (*spliceinterv).m_WeightedTimeScale;

				(*spliceinterv).m_WeightedTime -= (*transbegininterv).m_WeightedTime;
				(*spliceinterv).m_WeightedDuration -= (*transbegininterv).m_WeightedDuration;
			}

			// Run through the remaining intervals, looking for the correct spot to insert the transition end

			float choptime = reqtranstime;
			for (WarpIntervalList::iterator staleinterval = spliceinterv;
							(choptime > (*staleinterval).m_WeightedDuration) && (staleinterval != m_Intervals.end());
							++staleinterval)
			{
				choptime -= (*staleinterval).m_WeightedDuration;
			}

			if (staleinterval == m_Intervals.end())
			{
				--staleinterval;
			
				(*staleinterval).m_RHSTags |= 1<<ANIMEVENT_TRANSITION_END;

				// There wasn't enough time left in the time warp to account for the transtime!
				// Need to delete the choptime from the transtime
				reqtranstime -= choptime;
			}
			else
			{
				float chopratio = choptime/(*staleinterval).m_WeightedDuration;
				if (IsZero(chopratio))
				{
					(*staleinterval).m_LHSTags |= 1<<ANIMEVENT_TRANSITION_END;
					if (staleinterval != m_Intervals.begin())
					{
						--staleinterval;
						(*staleinterval).m_RHSTags |= 1<<ANIMEVENT_TRANSITION_END;
					}

				}
				else if (IsEqual(chopratio,1))
				{
					(*staleinterval).m_RHSTags |= 1<<ANIMEVENT_TRANSITION_END;
					//if (staleinterval != m_Intervals.end())
					//{
					//	(*staleinterval).m_LHSTags |= 1<<ANIMEVENT_TRANSITION_END;
					//}
				}
				else
				{

					// We need to split the stale interval and all the segments that are tied to it
					WarpInterval newinterv;

					WarpIntervalList::iterator transendinterv = m_Intervals.insert(staleinterval,newinterv);
					if (staleinterval == spliceinterv)
					{
						spliceinterv = transendinterv;
					}
					if (staleinterval == m_CurrentInterval)
					{
						m_CurrentInterval = transendinterv;
					}

					(*transendinterv).m_LHSTags = (*staleinterval).m_LHSTags;
					(*transendinterv).m_RHSTags = 1<<ANIMEVENT_TRANSITION_END;

					(*transendinterv).m_WeightedTime      = (*staleinterval).m_WeightedTime * chopratio;
					(*transendinterv).m_WeightedDuration  = (*staleinterval).m_WeightedDuration * chopratio;
					(*transendinterv).m_WeightedTimeScale = (*staleinterval).m_WeightedTimeScale;

					(*staleinterval).m_LHSTags  |= 1<<ANIMEVENT_TRANSITION_END;
					(*staleinterval).m_RHSTags  = 0;

					(*staleinterval).m_WeightedTime     -= (*transendinterv).m_WeightedTime;
					(*staleinterval).m_WeightedDuration -= (*transendinterv).m_WeightedDuration;

					// Insert the half segments we need to define the transition end event
					for (WarpSegmentList::iterator seg = (*staleinterval).m_Segments.begin();
													seg != (*staleinterval).m_Segments.end();
													++seg)
					{
						WarpSegment newseg;
						newseg.m_pBlendGroup	 = (*seg).m_pBlendGroup;
						newseg.m_BlendGroupIndex = (*seg).m_BlendGroupIndex;
						newseg.m_LHSTag			 = (*seg).m_LHSTag;
						newseg.m_RHSTag			 = 1<<ANIMEVENT_TRANSITION_END;
						newseg.m_Time			 = (*seg).m_Time;
						newseg.m_Interval		 = (*seg).m_Interval * chopratio;

						(*seg).m_LHSTag			 |= 1<<ANIMEVENT_TRANSITION_END;
						(*seg).m_Time			 += newseg.m_Interval;
						(*seg).m_Interval		 -= newseg.m_Interval;

						(*transendinterv).m_Segments.push_back(newseg);
					}

					// Interpolate the blending for the transition end interval
					for (WarpBlendList::iterator blend = (*staleinterval).m_Blends.begin();
													blend != (*staleinterval).m_Blends.end();
													++blend)
					{
						WarpBlend newblend;
						newblend.m_pBlendGroup = (*blend).m_pBlendGroup;
						newblend.m_LHSWeight = (*blend).m_LHSWeight;
						newblend.m_RHSWeight = (*blend).m_LHSWeight + chopratio * ((*blend).m_RHSWeight-(*blend).m_LHSWeight);
						(*blend).m_LHSWeight = newblend.m_RHSWeight;
						(*transendinterv).m_Blends.push_back(newblend);
					}

					// Need to clobber the events (except a group finish!) in all remaining intervals
					//for (staleinterval++; staleinterval != m_Intervals.end() ; ++staleinterval)
					//{
					//	(*staleinterval).m_LHSTags = 0;
					//	(*staleinterval).m_RHSTags = 0;
					//}
					
				}
			}

			// ### at this point we have split the existing timewarp
			// ### and the spliceinterv points to the start of 
			// ### the timewarp segment at the transition begin
		
			for ( DWORD k = 0; k < m_ConstructionGroup->m_KeyInfo.size(); ++k )
			{
				PRSKeys* newkeys = m_ConstructionGroup->m_KeyInfo[k].GetKeys(b);

				gpassertf(newkeys, ("AttachFromConstructionGroup() : Trying to attach an invalid set of keys"));

				DWORD tflags = (*spliceinterv).m_LHSTags;
				gpassert((tflags & (1<<ANIMEVENT_TRANSITION_BEGIN)) != 0);

				WarpIntervalList::iterator lastinterv;
				WarpIntervalList::iterator matchinterv = spliceinterv;

				KEventMap& events = newkeys->GetCriticalEvents();
				KEventMap::Iter currev = events.Begin();
				KEventMap::Iter nextev = currev;
				++nextev;

				gpassert(nextev != events.End());

				float CGtranstime = reqtranstime * newkeys->GetImpl()->m_Frequency;
				float CGcurrtime = events.Time(currev);
				float CGnormaldur = (events.Time(nextev)-CGcurrtime);
				float CGactualdur = CGnormaldur * newkeys->GetDuration();

				float TWactualdur = (*matchinterv).m_WeightedDuration;

				DWORD currLHSTag = events.Tags(currev);
				DWORD currRHSTag = events.Tags(nextev);

				float weightLHS = 0;
				float weightRHS = 0;
				bool initweight = true;

				while ( (nextev != events.End()) || (matchinterv != m_Intervals.end()) ) 
				{

					if (nextev == events.End())
					{
						// No more events in the construction group, we can move on 
						// to blending
						break;
					}
					else if (matchinterv == m_Intervals.end())
					{

						// Add a new segment to the last interval

						WarpSegment newseg;

						newseg.m_pBlendGroup	 = m_ConstructionGroup;
						newseg.m_BlendGroupIndex = k;
						newseg.m_Time     = CGcurrtime;
						newseg.m_Interval = CGnormaldur;
						newseg.m_LHSTag   = currLHSTag;
						newseg.m_RHSTag   = currRHSTag;

						if (k == 0) {

							weightLHS = weightRHS;
							weightRHS = FilterSmoothStep(0,CGtranstime,CGcurrtime+CGnormaldur);

							// Append a new last interval
							WarpInterval newinterv;
							newinterv.m_LHSTags = currLHSTag;
							newinterv.m_RHSTags = currRHSTag;
							newinterv.m_Segments.push_back(newseg);
							newinterv.AddGroupBlend(m_ConstructionGroup,weightLHS, weightRHS,initweight);
							initweight = false;
							lastinterv = m_Intervals.insert(m_Intervals.end(),newinterv);
							(*lastinterv).m_WeightedDuration = 0;
							(*lastinterv).m_WeightedTime = 0;
							(*lastinterv).m_WeightedTimeScale = 1;
						} 
						else 
						{
							// Update the last interval
							(*lastinterv).m_LHSTags |= currLHSTag;
							(*lastinterv).m_RHSTags |= currRHSTag;
							(*lastinterv).m_Segments.push_back(newseg);
						}


						currev = nextev;
						++nextev;

						if (nextev != events.End())
						{
							TWactualdur = 0;

							CGcurrtime = events.Time(currev);

							CGnormaldur = events.Time(nextev)-events.Time(currev);
							CGactualdur = CGnormaldur * newkeys->GetDuration();

							currLHSTag = events.Tags(currev);
							currRHSTag = events.Tags(nextev);


						}

					}
					else 
					{
						// Compare the C.G.'s currev/nextev pair with the matchinterval
						// to decide which will end first
/*
						if (IsEqual(TWactualdur,CGactualdur,0.01f))
						{
							if (TWactualdur != CGactualdur)
							{
								_asm
								{
									int 3
								}
							}
						}
*/
						if (((*matchinterv).m_RHSTags == currRHSTag) || IsEqual(TWactualdur,CGactualdur,0.001f))
						{
							// These are matching intervals

							WarpSegment newseg;
							newseg.m_pBlendGroup = m_ConstructionGroup;
							newseg.m_BlendGroupIndex = k;

							newseg.m_Time     = CGcurrtime;
							newseg.m_Interval = CGnormaldur;
							newseg.m_LHSTag   = currLHSTag;
							newseg.m_RHSTag   = currRHSTag;

							if (currev == events.Begin())
							{
								// LHS of first segments has a TBEG
								newseg.m_LHSTag |= (1 << ANIMEVENT_TRANSITION_BEGIN);
							}

							(*matchinterv).m_LHSTags |= newseg.m_LHSTag;
							(*matchinterv).m_RHSTags |= newseg.m_RHSTag;
							(*matchinterv).m_Segments.push_back(newseg);

							if (k == 0) {
								weightLHS = weightRHS;
								weightRHS = FilterSmoothStep(0,CGtranstime,CGcurrtime+CGnormaldur);
								(*matchinterv).AddGroupBlend(m_ConstructionGroup,weightLHS, weightRHS,initweight);
								initweight = false;
							}

							++matchinterv;

							currev = nextev;
							++nextev;

							if ( (nextev != events.End()) && (matchinterv != m_Intervals.end()) )
							{
								TWactualdur = (*matchinterv).m_WeightedDuration;

								CGcurrtime  = events.Time(currev);
								CGnormaldur = events.Time(nextev)-events.Time(currev);
								CGactualdur = CGnormaldur * newkeys->GetDuration();

								currLHSTag = events.Tags(currev);
								currRHSTag = events.Tags(nextev);

							}

						}
						else if (TWactualdur < CGactualdur)
						{

							// The existing timewarp interval will end before the construction
							// group interval. We have already made sure that the timewarp has
							// a transition end, so we no longer need to worry about it.

							WarpSegment newseg;
							newseg.m_pBlendGroup = m_ConstructionGroup;
							newseg.m_BlendGroupIndex = k;

							newseg.m_Time = CGcurrtime;
							newseg.m_Interval = (TWactualdur/CGactualdur)*CGnormaldur;
							newseg.m_LHSTag   = currLHSTag;
							newseg.m_RHSTag   = (*matchinterv).m_RHSTags & (1 << ANIMEVENT_TRANSITION_END);

							if (currev == events.Begin())
							{
								// LHS of first segment from must have a TBEG
								newseg.m_LHSTag |= (1 << ANIMEVENT_TRANSITION_BEGIN);
							}

							currLHSTag = 0;

							(*matchinterv).m_LHSTags |= newseg.m_LHSTag;
							(*matchinterv).m_RHSTags |= newseg.m_RHSTag;
							(*matchinterv).m_Segments.push_back(newseg);

							if (k == 0) {
								weightLHS = weightRHS;
								weightRHS = FilterSmoothStep(0,CGtranstime,CGcurrtime+newseg.m_Interval);
								(*matchinterv).AddGroupBlend(m_ConstructionGroup,weightLHS,weightRHS,initweight);
								initweight = false;
							}

							++matchinterv;

							CGcurrtime += newseg.m_Interval;
							CGnormaldur -= newseg.m_Interval;
							CGactualdur -= TWactualdur;

							if (matchinterv != m_Intervals.end())
							{
							    TWactualdur = (*matchinterv).m_WeightedDuration;
							}
							else
							{
								TWactualdur = 0.0f;
							}

						}
						else if (TWactualdur > CGactualdur)
						{
							// The construction group interval will end before the existing
							// timewarp interval, we need to insert an interval into the TW

							gpassertm(k==0,("TWintervdur > CGintervdur but k !=0 in TransitionTo"));

							float ratio = CGactualdur/TWactualdur;

							if (k == 0) {
								WarpInterval newinterv;
								newinterv.m_LHSTags = 0;
								newinterv.m_RHSTags = 0;
								lastinterv = m_Intervals.insert(matchinterv,newinterv);
								(*lastinterv).m_WeightedDuration = 0;
								(*lastinterv).m_WeightedTime = 0;
								(*lastinterv).m_WeightedTimeScale = 1;
							}

							if (matchinterv == spliceinterv)
							{
								spliceinterv = lastinterv;
							}

							// Insert the half segments that define the time just prior to construction group
							for (WarpSegmentList::iterator seg = (*matchinterv).m_Segments.begin();
															seg != (*matchinterv).m_Segments.end();
															++seg)
							{

								WarpSegment newseg;
								newseg.m_pBlendGroup		= (*seg).m_pBlendGroup;
								newseg.m_BlendGroupIndex	= (*seg).m_BlendGroupIndex;
								newseg.m_LHSTag				= (*seg).m_LHSTag;
								newseg.m_RHSTag				= 0;
								newseg.m_Time				= (*seg).m_Time;
								newseg.m_Interval			= (*seg).m_Interval*ratio;
								(*seg).m_LHSTag				= 0;
								(*seg).m_Time			    += newseg.m_Interval;
								(*seg).m_Interval			-= newseg.m_Interval;

								(*lastinterv).m_Segments.push_back(newseg);
								(*lastinterv).m_LHSTags		|= newseg.m_LHSTag;

							}

							(*matchinterv).m_LHSTags = 0;

							(*lastinterv).m_WeightedTime      = (*matchinterv).m_WeightedTime * ratio;
							(*lastinterv).m_WeightedDuration  = (*matchinterv).m_WeightedDuration * ratio;
							(*lastinterv).m_WeightedTimeScale = (*matchinterv).m_WeightedTimeScale;

							(*matchinterv).m_WeightedTime     -= (*lastinterv).m_WeightedTime;
							(*matchinterv).m_WeightedDuration -= (*lastinterv).m_WeightedDuration;

							WarpSegment newseg;
							newseg.m_pBlendGroup = m_ConstructionGroup;
							newseg.m_BlendGroupIndex = k;

							newseg.m_Time     = CGcurrtime;
							newseg.m_Interval = CGnormaldur;
							newseg.m_LHSTag   = currLHSTag;
							newseg.m_RHSTag   = currRHSTag;

							if (currev == events.Begin())
							{
								// LHS of first segment from must have a TBEG
								newseg.m_LHSTag |= (1 << ANIMEVENT_TRANSITION_BEGIN);
							}

							(*lastinterv).m_LHSTags |= newseg.m_LHSTag;
							(*lastinterv).m_RHSTags |= newseg.m_RHSTag;
							(*lastinterv).m_Segments.push_back(newseg);

							if (k == 0)
							{
								weightLHS = weightRHS;
								weightRHS = FilterSmoothStep(0,CGtranstime,CGcurrtime+CGnormaldur);

								// Interpolate the blending for the transition interval
								for (WarpBlendList::iterator blend = (*matchinterv).m_Blends.begin();
																blend != (*matchinterv).m_Blends.end();
																++blend)
								{
									WarpBlend newblend;
									newblend.m_pBlendGroup = (*blend).m_pBlendGroup;
									newblend.m_LHSWeight = (*blend).m_LHSWeight;
									newblend.m_RHSWeight = (*blend).m_LHSWeight + (ratio)*((*blend).m_RHSWeight - (*blend).m_LHSWeight);
									(*blend).m_LHSWeight = newblend.m_RHSWeight;
									(*lastinterv).m_Blends.push_back(newblend);
								}
								(*lastinterv).AddGroupBlend(m_ConstructionGroup,weightLHS, weightRHS,initweight);
								initweight = false;
							}

							currev = nextev;
							++nextev;

							if (nextev != events.End())
							{
								TWactualdur -= CGactualdur;

								CGcurrtime = events.Time(currev);
								CGnormaldur = events.Time(nextev)-events.Time(currev);
								CGactualdur = CGnormaldur * newkeys->GetDuration();

								currLHSTag = events.Tags(currev);
								currRHSTag = events.Tags(nextev);

							}
						}
						else
						{
							gperror(("Nema::TimeWarp::TransitionToConstructionGroup() Unable to resolve the internal intervals for a TransitionTo"));
							break;
						}

					}

				}

				// We need to get rid of all the stale intervals that are hanging around
				if ( (matchinterv != m_Intervals.end()) && (matchinterv == m_CurrentInterval) )
				{
					++matchinterv;
				}
				m_Intervals.erase(matchinterv,m_Intervals.end());

			}

			// Assume all the groups are stale and that all cyclics are to be removed
			for (BlendGroupPtrList::iterator grp = m_Groups.begin(); grp != m_Groups.end(); ++grp) 
			{
				(*grp)->m_IsStale = true;
				(*grp)->m_IsCyclic = false;
			}

			// Now we have to make sure that ownership of the construction
			// group gets transferred to the timewarp 
			m_Groups.push_front(m_ConstructionGroup);
			m_ConstructionGroup = NULL;

			// Make sure that all groups we expect to use are properly marked as not stale
			WarpIntervalList::iterator interv = m_CurrentInterval;
			WarpBlendList::iterator wbit;

			for ( wbit = (*interv).m_Blends.begin(); wbit != (*interv).m_Blends.end();	++wbit)
			{
				// A blend in the current interval can have a zero on the RHS
				if (!IsZero((*wbit).m_LHSWeight)||!IsZero((*wbit).m_RHSWeight))
				{
					(*wbit).m_pBlendGroup->m_IsStale = false;
				}
			}
			++interv;

			while (interv != m_Intervals.end())
			{
				// All other intervals must be non-zero on the RHS in order to avoid removal
				for (wbit = (*interv).m_Blends.begin(); wbit != (*interv).m_Blends.end();	++wbit)
				{
					if (!IsZero((*wbit).m_RHSWeight))
					{
						(*wbit).m_pBlendGroup->m_IsStale = false;
					}
				}
				++interv;
			}

			RemoveStale(b);

			// This is a little bit of cleanup code to ensure that the blends on the current
			// interval are correct. It's clumsy, but it gets the job done.
			for (blend = (*m_CurrentInterval).m_Blends.begin();
				 blend != (*m_CurrentInterval).m_Blends.end();
				 ++blend)
			{
				(*blend).m_pBlendGroup->m_CurrentLHSWeight = (*blend).m_LHSWeight;
				(*blend).m_pBlendGroup->m_CurrentRHSWeight = (*blend).m_RHSWeight;
			}

		}
	}


	return ret;
}

//************************************************************
bool
TimeWarp::Recalc(Blender* b) {

	if (m_CurrentInterval == m_Intervals.end()) {
		m_TotalTime = 0.0;
		return true;
	}

	m_TotalTime = 0.0;

	WarpIntervalList::iterator interv = interv = m_Intervals.begin();
	while (interv != m_Intervals.end()) {

		float total_weight = 0.0f;
		float blended_duration = 0.0f;
		float actual_duration = 0.0;

		for (WarpSegmentList::iterator seg = (*interv).m_Segments.begin();
			seg != (*interv).m_Segments.end();
			++seg) {

			const BlendGroup* pbg = (*seg).m_pBlendGroup;
			const DWORD bgi = (*seg).m_BlendGroupIndex;
			const float segdur = (*seg).m_Interval * pbg->m_KeyInfo[bgi].GetKeys(b)->GetDuration();

			const float weight =  pbg->GetWeight(bgi,0.5);

			total_weight += weight;
			blended_duration += weight * segdur;
			actual_duration += segdur;

		}

		if (IsZero(total_weight)) {
			(*interv).m_WeightedDuration = 0.0f;
		} else {
			(*interv).m_WeightedDuration = blended_duration/total_weight;
		}

		(*interv).m_WeightedTime = m_TotalTime;
		m_TotalTime += (*interv).m_WeightedDuration;

		++interv;

	}

	interv = m_Intervals.begin();
	while (interv != m_Intervals.end()) {
		float dur = (*interv).m_WeightedDuration;
		if (IsZero(dur)) {
			(*interv).m_WeightedTimeScale = 0.0f;
		} else {
			(*interv).m_WeightedTimeScale = 1.0f/dur;
		}
		++interv;
	}

	return true;

}

//************************************************************
KEventTags TimeWarp::UpdateInterval(Blender *b,float deltat) {


	if (m_Intervals.size() == 0 || IsZero(m_TotalTime)) {
		m_CurrentInterval = m_Intervals.end();
		m_IsIdle = true;
		return 0;
	}

	m_IsIdle = false;

	bool changed = false;
	bool wrapped = false;

	KEventTags events= 0;

	if (IsZero((*m_CurrentInterval).m_WeightedDuration)) {

		do {
			bool hold_at_end = true;
			changed = true;
			for (WarpBlendList::iterator blend = (*m_CurrentInterval).m_Blends.begin();
												blend != (*m_CurrentInterval).m_Blends.end();
												++blend) {

				if (!IsZero((*blend).m_LHSWeight) && IsZero((*blend).m_RHSWeight))
				{
					if (!(*blend).m_pBlendGroup->m_IsStale) {
						events |= 1<<ANIMEVENT_GROUP_FINISH;
						(*blend).m_pBlendGroup->m_IsStale = true;
					}
				}
				else
				{
					if ((*blend).m_pBlendGroup->m_IsCyclic)
					{
						hold_at_end = false;
					}
				}
			}
			++m_CurrentInterval;

			if (m_CurrentInterval == m_Intervals.end())
			{
				if (hold_at_end) {
					m_IntervalTime = 1.0f;
					--m_CurrentInterval;

					// We have arrived at the end of the timewarp
					// so we need to send a finish event
					events |= 1<<ANIMEVENT_FINISH;
					m_IsIdle = true;

					break;
				} else {
					m_CurrentInterval = m_Intervals.begin();
					if (wrapped)
					{
						// Only repeat ONCE
						break;
					}
					wrapped = true;
				}
			} else {
				hold_at_end = false;
			}

		} while (IsZero((*m_CurrentInterval).m_WeightedDuration));

	}

	deltat *= (*m_CurrentInterval).m_WeightedTimeScale;

	if (m_IntervalTime == 0.0 && IsPositive(deltat)) {
		// We are at the start of the warp, we need to send the LHS tags too
		events |= (*m_CurrentInterval).m_LHSTags;
	}

	m_IntervalTime += deltat;

	wrapped = false;
	while (m_IntervalTime>1) {

		// Determine the time remaining AFTER this interval completes

		m_IntervalTime -= 1.0f;

		if (IsPositive(m_IntervalTime,0.001f) && !IsZero((*m_CurrentInterval).m_WeightedTimeScale))
		{
			m_IntervalTime /= (*m_CurrentInterval).m_WeightedTimeScale;
		}
		else
		{
			m_IntervalTime = 0;
		}

		bool hold_at_end = true;

		do {
			changed = true;

			events |= (*m_CurrentInterval).m_RHSTags;

			for (WarpBlendList::iterator blend = (*m_CurrentInterval).m_Blends.begin();
												blend != (*m_CurrentInterval).m_Blends.end();
												++blend) {

				/// Stale if only the RHS is zero
				if (!IsZero((*blend).m_LHSWeight) && IsZero((*blend).m_RHSWeight))
				{
					if (!(*blend).m_pBlendGroup->m_IsStale) {
						events |= 1<<ANIMEVENT_GROUP_FINISH;
						(*blend).m_pBlendGroup->m_IsStale = true;
					}
				}
				else
				{
					if ((*blend).m_pBlendGroup->m_IsCyclic)
					{
						hold_at_end = false;
					}
				}
			}

			++m_CurrentInterval;

			if (m_CurrentInterval == m_Intervals.end())
			{
				if (hold_at_end) {

					m_IntervalTime = 1.0f;
					--m_CurrentInterval;

					// We have arrived at the end of the timewarp
					// so we need to send a finish event
					events |= 1<<ANIMEVENT_FINISH;
					m_IsIdle = true;

					break;
				} else {
					m_CurrentInterval = m_Intervals.begin();
					if (wrapped)
					{
						// Only repeat ONCE
						break;
					}
					wrapped = true;
				}
			} else {
				hold_at_end = false;
			}

		} while (IsZero((*m_CurrentInterval).m_WeightedDuration));

		if (IsZero(m_IntervalTime))
		{
			m_IntervalTime = 0.0f;
		}
		else if (!hold_at_end)
		{
			m_IntervalTime *= (*m_CurrentInterval).m_WeightedTimeScale;
		}

	}

	if (changed)
	{
		if (wrapped)
		{
			// Reset all the weights of the cyclic groups to the weight at the end of the timeline
			for (WarpIntervalList::iterator interv = m_Intervals.begin() ; interv != m_Intervals.end() ; ++interv)
			{
				for (WarpBlendList::iterator blend = (*interv).m_Blends.begin(); blend != (*interv).m_Blends.end(); ++blend)
				{
					if ((*blend).m_pBlendGroup->m_IsCyclic)
					{
						(*blend).m_LHSWeight = (*blend).m_RHSWeight = (*blend).m_pBlendGroup->m_CurrentRHSWeight;
					}
					else
					{
						(*blend).m_LHSWeight = (*blend).m_RHSWeight = 0;
					}
				}

				// Clearing the tags to makes it easier to inspect a debug dump
				for (WarpSegmentList::iterator seg = (*interv).m_Segments.begin();
					seg != (*interv).m_Segments.end();
					++seg) {
					(*seg).m_LHSTag &= ~((1<<ANIMEVENT_TRANSITION_BEGIN)|(1<<ANIMEVENT_TRANSITION_END));
					(*seg).m_RHSTag &= ~((1<<ANIMEVENT_TRANSITION_BEGIN)|(1<<ANIMEVENT_TRANSITION_END));
				}

				(*interv).m_LHSTags &= ~((1<<ANIMEVENT_TRANSITION_BEGIN)|(1<<ANIMEVENT_TRANSITION_END));
				(*interv).m_RHSTags &= ~((1<<ANIMEVENT_TRANSITION_BEGIN)|(1<<ANIMEVENT_TRANSITION_END));
			}	

			events |= 1<<ANIMEVENT_START;

		}

		(*m_CurrentInterval).UpdateGroupBlends();

		if (BitTest(events,ANIMEVENT_GROUP_FINISH))
		{
			RemoveStale(b);
		}
	}

	return events;

}

//************************************************************
BlendGroup*
TimeWarp::OpenConstructionGroup(void) {

	gpassertf(m_ConstructionGroup == NULL, ("Already have the Construction Group open"));

	if (m_ConstructionGroup == NULL) {
		m_ConstructionGroup = new BlendGroup;
		m_ConstructionGroup->m_GroupWeight = 0;
		m_ConstructionGroup->m_NormalizedWeight = 0;
		m_ConstructionGroup->m_CurrentLHSWeight = 0;
		m_ConstructionGroup->m_CurrentRHSWeight = 0;
		m_ConstructionGroup->m_IsCyclic = false;
		m_ConstructionGroup->m_IsStale = false;
	}

	return m_ConstructionGroup;

}

//************************************************************
void
TimeWarp::CloseConstructionGroup(Blender*b, bool merge)
{
	gpassertf(m_ConstructionGroup != NULL, ("No Construction Group to close... never opened?"));
	if (m_ConstructionGroup == NULL) {
		return;
	}

	m_ConstructionGroup->ReNormalize();

	// Put the newly constructed group into the timewarp

	RemoveStale(b);
	
	AttachConstructionGroup(b,merge);

}

//************************************************************
void
TimeWarp::CloseConstructionGroupWithTransition(Blender* b, float transition_time,bool append)
{
	gpassertf(m_ConstructionGroup != NULL, ("No Construction Group to close... never opened?"));
	if (m_ConstructionGroup == NULL) {
		return;
	}

	m_ConstructionGroup->ReNormalize();

	RemoveStale(b);

	// Transition whatever is in the timewarp to the newly constructed group

	if (IsZero(transition_time))
	{
		// Instant transition
		AttachConstructionGroup(b,false);
	}
	else
	{
		TransitionToConstructionGroup(b,transition_time,append);
	}

}

//************************************************************
float
TimeWarp::GetGroupWeight(DWORD g) const {

	for (BlendGroupPtrList::const_iterator grp = m_Groups.begin(); grp != m_Groups.end(); ++grp) {
		if ((DWORD)(*grp) == g) {
			return (*grp)->m_GroupWeight;
		}
	}

	return 0.0;

}
//************************************************************
void
TimeWarp::SetGroupWeight(Blender* b, DWORD g, float w) {

	float scaler = w;

	BlendGroupPtrList::iterator grp;

	for (grp = m_Groups.begin(); grp != m_Groups.end(); ++grp) {
		if ((DWORD)(*grp) == g) {
			(*grp)->m_GroupWeight = w ;
		} else {
			scaler += (*grp)->m_GroupWeight;
		}
	}

	if (IsZero(scaler)) {
		scaler = 0.0f;
	} else {
		scaler = 1.0f/scaler;
	}

	for (grp = m_Groups.begin(); grp != m_Groups.end(); ++grp) {
		(*grp)->m_NormalizedWeight = (*grp)->m_GroupWeight * scaler;
	}

	Recalc(b);

//	DebugDump();

}

//************************************************************
float
TimeWarp::GetGroupAnimWeight(DWORD g, DWORD i) const {

	for (BlendGroupPtrList::const_iterator grp = m_Groups.begin(); grp != m_Groups.end(); ++grp)
	{
		if ((DWORD)(*grp) == g) 
		{
			if (i < (*grp)->m_KeyInfo.size())
			{
				return (*grp)->m_KeyInfo[i].m_RelativeWeight;
			}
		}
	}

	return 0.0;

}

//************************************************************
void
TimeWarp::SetGroupAnimWeight(DWORD g, DWORD i, float w) {

	BlendGroupPtrList::iterator grp;

	for (grp = m_Groups.begin(); grp != m_Groups.end(); ++grp)
	{
		if ((DWORD)(*grp) == g)
		{
			DWORD sz = (*grp)->m_KeyInfo.size();

			if (i<sz)
			{

				if (sz == 1) 
				{
					(*grp)->m_KeyInfo[i].m_RelativeWeight = w;
					return;
				}

				float scaler;

				if (IsEqual((*grp)->m_KeyInfo[i].m_RelativeWeight,1))
				{
					scaler = 0;
					w = 1;
				}
				else
				{
					scaler = (1-w)/(1-(*grp)->m_KeyInfo[i].m_RelativeWeight);
					if (IsZero(scaler))
					{
						scaler = 0.0f;
					}
				}

				(*grp)->m_KeyInfo[i].m_RelativeWeight = w;

				for(DWORD j = 0; j < sz; ++j) {
					if (i == j) continue;
					(*grp)->m_KeyInfo[j].m_RelativeWeight *= scaler;
				}
			}
			return;

		}
	}

//	DebugDump();

}


//************************************************************
DWORD
TimeWarp::CountEvents(eAnimEvent ev) const
{
	
	DWORD count=0;

	if (ev == ANIMEVENT_FINISH)
	{
		// Finish events are generated, they do not appear in the list of intervals
		// We report that there is 1 ANIMEVENT_FINISH if we determine that the timewarp
		// does loop on the last interval

		count = 1; // Assume the animation finishes unless proven otherwise
		WarpIntervalList::const_iterator interv = m_Intervals.end();
		--interv;

		if ( interv != m_Intervals.end() )
		{
			for (WarpBlendList::const_iterator blend = (*interv).m_Blends.begin();
											 blend != (*interv).m_Blends.end();
										 	++blend) 
			{
				if ((*blend).m_pBlendGroup->m_IsCyclic)
				{
					// This animation loops, so it has no ANIMEVENT_FINISH
					count = 0;
					break;
				}
			}
			++interv;
		}
	}
	else
	{

		WarpIntervalList::const_iterator interv = interv = m_Intervals.begin();

		if (interv != m_Intervals.end()) 
		{
			DWORD mask = 1<<ev;
			count = ((*interv).m_LHSTags & mask) ? 1 : 0;

			while (interv != m_Intervals.end())
			{
				if ((*interv).m_RHSTags & mask)
				{
					++count;
				}
				++interv;
			}
		}
	}

	return count;

}

//************************************************************
void
TimeWarp::RemoveStale(Blender *b) {

	bool foundstale = false;

	float weightscaler = 0.0;

	for (BlendGroupPtrList::iterator grp = m_Groups.begin(); grp != m_Groups.end(); ++grp) {
		if ((*grp)->m_IsStale) {
			foundstale = true;
		} else {
			weightscaler += (*grp)->m_GroupWeight;
		}
	}

	if (foundstale) {


		if (IsZero(weightscaler)) {
			weightscaler = 0.0f;
		} else {
			weightscaler = 1.0f/weightscaler;
		}

		WarpIntervalList::iterator interv = m_Intervals.begin();

		while (interv != m_Intervals.end()) {

			for (WarpSegmentList::iterator seg = (*interv).m_Segments.begin(); seg != (*interv).m_Segments.end();) {
				if ((*seg).m_pBlendGroup->m_IsStale) {
					seg = (*interv).m_Segments.erase(seg);
				} else {
					++seg;
				}
			}

			for (WarpBlendList::iterator blend = (*interv).m_Blends.begin(); blend != (*interv).m_Blends.end();) {
				if ((*blend).m_pBlendGroup->m_IsStale) {
					blend = (*interv).m_Blends.erase(blend);
				} else {
					++blend;
				}
			}

			if ((*interv).m_Segments.size() == 0) {
				if (interv == m_CurrentInterval) {
					m_CurrentInterval = m_Intervals.end();
				}
				interv = m_Intervals.erase(interv);
			} else {
				++interv;
			}

		}

		for (BlendGroupPtrList::iterator grp = m_Groups.begin(); grp != m_Groups.end(); ) {
			if ((*grp)->m_IsStale) {
				delete (*grp);
				grp = m_Groups.erase(grp);
			} else {
				(*grp)->m_NormalizedWeight =  (*grp)->m_GroupWeight * weightscaler;
				++grp;
			}
		}

		Recalc(b);

	}



}

#if !GP_RETAIL
//************************************************************
void
TimeWarp::DebugDump(Blender *b) {

	gpgenericf(("\n\n-------------------------\n"));
	gpgenericf(("TotalTime:    %f\n",m_TotalTime));
	gpgenericf(("IntervalTime: %f\n",m_IntervalTime));

	int cnt = 0;

	BlendGroupPtrList::iterator bg = m_Groups.begin();

	while (bg != m_Groups.end()) {
		gpgenericf(("Group #%d (0x%08x)",cnt++,(*bg)));
		if ((*bg) == NULL) continue;
		(*bg)->DebugDump(b);
		++bg;
	}


	cnt = 0;

	gpgenericf(("\nIntervals:\n"));

	WarpIntervalList::iterator interv = m_Intervals.begin();

	while (interv != m_Intervals.end()) {

		gpgenericf(("\nInterval #%d",cnt++));

		if (m_CurrentInterval == interv) {
			gpgenericf(("  @@@ CurrentInterval @@@ -------------------------\n"));
		} else {
			gpgenericf(("\n"));
		}

		(*interv).DebugDump(b);

		++interv;
	}
}

//************************************************************
void
TimeWarp::StatusDump(Blender *b,gpstring& out) {

	out.appendf("TotalTime:    %f ",m_TotalTime);
	out.appendf("IntervalTime: %f\n",m_IntervalTime);

	int cnt = 0;

	BlendGroupPtrList::const_iterator bg = m_Groups.begin();

	while (bg != m_Groups.end()) {
		out.appendf("Group #%d (0x%08x)",cnt++,(*bg));
		if ((*bg) == NULL) continue;
		(*bg)->StatusDump(b,out);
		++bg;
	}


	cnt = 0;

	WarpIntervalList::const_iterator interv = m_Intervals.begin();

	while (interv != m_Intervals.end()) {

		out.appendf("Interval #%d",cnt++);

		if (interv == m_CurrentInterval) {
			out.appendf(("  @@@ CurrentInterval @@@ -------------------------\n"));
		} else {
			out.appendf("\n");
		}

		(*interv).StatusDump(b,out);

		++interv;
	}
}
#endif

FUBI_DECLARE_SELF_TRAITS( BlendGroup::KeyInfo );

//************************************************************
bool
BlendGroup::KeyInfo::Xfer(FuBi::PersistContext& persist)
{
	persist.Xfer       ( "m_Chore",				m_Chore				);
	persist.Xfer       ( "m_Stance",			m_Stance			);
	persist.Xfer       ( "m_Index",				m_Index				);
	persist.Xfer       ( "m_RelativeWeight",	m_RelativeWeight	);
	return true;
}


//************************************************************
PRSKeys* 
BlendGroup::KeyInfo::GetKeys(Blender *b) const
{
	return b->GetKeys((eAnimChore)m_Chore,(eAnimStance)m_Stance,m_Index);
}

//************************************************************
bool
BlendGroup::Xfer(FuBi::PersistContext& persist) {

	persist.Xfer       ( "m_GroupWeight",      m_GroupWeight      );
	persist.Xfer       ( "m_NormalizedWeight", m_NormalizedWeight );
	persist.Xfer       ( "m_CurrentLHSWeight", m_CurrentLHSWeight );
	persist.Xfer       ( "m_CurrentRHSWeight", m_CurrentRHSWeight );

	persist.XferVector ( "m_KeyInfo",          m_KeyInfo          );

	persist.Xfer       ( "m_IsCyclic",         m_IsCyclic         );
	persist.Xfer       ( "m_IsStale",          m_IsStale          );

	return ( true );
}

//************************************************************
bool
BlendGroup::AddToGroup(Blender* b, DWORD c, DWORD s, DWORD i, float w, bool forcecyclic) {

#if !GP_RETAIL
	if (IsMemoryBadFood((DWORD)this))
	{
		gperrorf(("NEMA_KEVENTS: Attempting to add keys to a corrupt/missing blending group! this_ptr = [0x%08x]",(DWORD)this));
		return false;
	}
#endif

	bool ret = true;

	KeyInfo ki(c,s,i);

	PRSKeys* k = ki.GetKeys(b);

	if (k) 
	{
		// Shoud verify that these new keys match the others in the group
		ki.m_RelativeWeight = w;
		m_KeyInfo.push_back(ki);

		m_IsCyclic |= (k->GetLoopAtEnd() | forcecyclic);

		k->UpdateAbsolute(0,0);
	}
	else
	{
		gperror("NEMA_KEVENTS: The animation keys that you are trying to blend with are not valid!");
		ret=false;
	}
	
	return ret;

}

//************************************************************
float
BlendGroup::GetWeight(DWORD index, float t) const {

	if (index < m_KeyInfo.size()) {
		return  m_KeyInfo[index].m_RelativeWeight  *
				m_NormalizedWeight *
				(m_CurrentLHSWeight + FilterSmoothStep(0,1,t) * (m_CurrentRHSWeight - m_CurrentLHSWeight));
	}

	return 0.0;
}

//************************************************************
void
BlendGroup::ReNormalize(void) {

	int i;
	float scaler=0.0f;

	int sz = (int)m_KeyInfo.size();

	for(i = 0; i < sz; ++i)
	{
		scaler += m_KeyInfo[i].m_RelativeWeight;
	}

	if (IsZero(scaler))
	{
		scaler = 0.0f;
	} 
	else
	{
		scaler = 1.0f/scaler;
	}

	for(i = 0; i < sz; ++i)
	{
		m_KeyInfo[i].m_RelativeWeight *= scaler;
	}

}

#if !GP_RETAIL
//************************************************************
void
BlendGroup::DebugDump(Blender* b) {

	gpgenericf(("\n\n%s\n",m_IsStale ? "STALE":"NOT STALE"));
	gpgenericf(("%s\n",m_IsCyclic ? "CYCLIC":"NOT CYCLIC"));
	gpgenericf(("GroupWeight: %0f\n",m_GroupWeight));
	gpgenericf(("CurrLHSWeight: %0f\n",m_CurrentLHSWeight));
	gpgenericf(("CurrRHSWeight: %0f\n",m_CurrentRHSWeight));

	int sz = (int)m_KeyInfo.size();

	for (int i = 0; i< sz ; i++)
	{
		KeyInfo& ki = m_KeyInfo[i];
		PRSKeys* pk = ki.GetKeys(b);
		gpgenericf(("k:%08x relw:%f dur:%f %s\n",pk,ki.m_RelativeWeight,pk->GetDuration(),pk->GetDebugName()));
	}

}

//************************************************************
void
BlendGroup::StatusDump(Blender* b,gpstring& out) {

	out.appendf("\n%s ",m_IsStale ? "STALE":"NOT STALE");
	out.appendf("%s ",m_IsCyclic ? "CYCLIC":"NOT CYCLIC");
	out.appendf("GroupWeight: %0f  ",m_GroupWeight);
	out.appendf("CurrLHSWeight: %0f  ",m_CurrentLHSWeight);
	out.appendf("CurrRHSWeight: %0f\n",m_CurrentRHSWeight);

	int sz = (int)m_KeyInfo.size();
	for (int i = 0; i< sz ; i++)
	{
		KeyInfo& ki = m_KeyInfo[i];
		PRSKeys* pk = ki.GetKeys(b);
		out.appendf("k:%08x relw:%f dur:%f %s\n",pk,ki.m_RelativeWeight,pk->GetDuration(),pk->GetDebugName());
	}

}

#endif

//************************************************************
bool
WarpBlend::Xfer( FuBi::PersistContext& persist ) {

	persist.Xfer( "m_LHSWeight", m_LHSWeight );
	persist.Xfer( "m_RHSWeight", m_RHSWeight );

	return ( true );
}

//************************************************************
bool
WarpSegment::Xfer( FuBi::PersistContext& persist ) {

	persist.Xfer   ( "m_BlendGroupIndex", m_BlendGroupIndex );
	persist.Xfer   ( "m_Time",            m_Time            );
	persist.Xfer   ( "m_Interval",        m_Interval        );
	persist.XferHex( "m_LHSTag",          m_LHSTag          );
	persist.XferHex( "m_RHSTag",          m_RHSTag          );

	return ( true );
}

FUBI_DECLARE_SELF_TRAITS( WarpSegment );
FUBI_DECLARE_SELF_TRAITS( WarpBlend );

//************************************************************
bool
WarpInterval::Xfer( FuBi::PersistContext& persist )
{
	persist.XferHex ( "m_LHSTags",           m_LHSTags           );
	persist.XferHex ( "m_RHSTags",           m_RHSTags           );
	persist.Xfer    ( "m_WeightedTime",      m_WeightedTime      );
	persist.Xfer    ( "m_WeightedDuration",  m_WeightedDuration  );
	persist.Xfer    ( "m_WeightedTimeScale", m_WeightedTimeScale );
	persist.XferList( "m_Segments",          m_Segments          );
	persist.XferList( "m_Blends",            m_Blends            );

	return ( true );
}

//************************************************************
void
WarpInterval::AddGroupBlend(BlendGroup* pbg,float lhs,float rhs, bool init_current) {


	WarpBlend newblend;
	newblend.m_pBlendGroup = pbg;
	newblend.m_LHSWeight = lhs;
	newblend.m_RHSWeight = rhs;

	if (init_current)
	{
		pbg->m_CurrentLHSWeight = lhs;
		pbg->m_CurrentRHSWeight = rhs;
	}

	const float lhs_scaler = 1.0f-lhs;
	const float rhs_scaler = 1.0f-rhs;

	for (WarpBlendList::iterator i = m_Blends.begin() ; i != m_Blends.end(); ++i) {
		(*i).m_LHSWeight *= lhs_scaler;
		(*i).m_RHSWeight *= rhs_scaler;
	}

	m_Blends.push_back(newblend);

}

//************************************************************
void
WarpInterval::UpdateGroupBlends() {

	for (WarpBlendList::iterator i = m_Blends.begin() ; i != m_Blends.end(); ++i) {
		(*i).m_pBlendGroup->m_CurrentLHSWeight =(*i).m_LHSWeight;
		(*i).m_pBlendGroup->m_CurrentRHSWeight =(*i).m_RHSWeight;
	}

}

#if !GP_RETAIL
//************************************************************
void
WarpInterval::DebugDump(Blender* b) const {

	gpgenericf(("Tags: %08x %08x\n",m_LHSTags,m_RHSTags));
	gpgenericf(("Weighted Time:       %f\n",m_WeightedTime));
	gpgenericf(("Weighted Duration:   %f\n",m_WeightedDuration));
	gpgenericf(("Weighted Time Scale: %f\n",m_WeightedTimeScale));

	DWORD tagi;

	gpgeneric("LHS tags:");

	for (tagi = 0; tagi < 32; tagi++) {
		if (BitTest(m_LHSTags,tagi)) {
			gpgenericf(("%s ",stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str()));
		}
	}
	gpgeneric("\nRHS tags:");

	for (tagi = 0; tagi < 32; tagi++) {
		if (BitTest(m_RHSTags,tagi)) {
			gpgenericf(("%s ",stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str()));
		}
	}

	gpgeneric("\n");

	for (WarpSegmentList::const_iterator seg = m_Segments.begin(); seg != m_Segments.end(); ++seg) {

		gpstring lhstags;

		for (tagi = 0; tagi < 32; tagi++) {
			if (BitTest((*seg).m_LHSTag, tagi)) {
				lhstags.append(stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true));
				lhstags.append(" ");
			}
		}

		gpstring rhstags;
		for (tagi = 0; tagi < 32; tagi++) {
			if (BitTest((*seg).m_RHSTag, tagi)) {
				rhstags.append(stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true));
				rhstags.append(" ");
			}
		}

		float LHSWeight = 0;
		float RHSWeight = 0;
		for (WarpBlendList::const_iterator bli = m_Blends.begin();bli != m_Blends.end(); ++bli)
		{
			if ((*bli).m_pBlendGroup == (*seg).m_pBlendGroup)
			{
				LHSWeight = (*bli).m_LHSWeight;
				RHSWeight = (*bli).m_RHSWeight;
				break;
			}
		}

		gpgenericf(("k:%08x bgp: %08x bgi: %3d t: %8.6f i: %8.6f LHS:%8.6f %08x %s RHS:%8.6f %08x %s\n",
			(int)(*seg).m_pBlendGroup->m_KeyInfo[(*seg).m_BlendGroupIndex].GetKeys(b),
			(*seg).m_pBlendGroup,
			(*seg).m_BlendGroupIndex,
			(*seg).m_Time,
			(*seg).m_Interval,
			LHSWeight,
			(*seg).m_LHSTag,
			lhstags.c_str(),
			RHSWeight,
			(*seg).m_RHSTag,
			rhstags.c_str()
			));
	}

}
//************************************************************
void
WarpInterval::StatusDump(Blender* b, gpstring& out) const {

	out.appendf("Weighted Time:       %f  ",m_WeightedTime);
	out.appendf("Weighted Duration:   %f  ",m_WeightedDuration);
	out.appendf("Weighted Time Scale: %f\n",m_WeightedTimeScale);

	DWORD tagi;

	out.appendf("LHS tags:");

	for (tagi = 0; tagi < 32; tagi++) {
		if (BitTest(m_LHSTags,tagi)) {
			out.appendf("%s ",stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str());
		}
	}
	out.appendf("RHS tags:");

	for (tagi = 0; tagi < 32; tagi++) {
		if (BitTest(m_RHSTags,tagi)) {
			out.appendf("%s ",stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true).c_str());
		}
	}

	out.appendf("\n");

	for (WarpSegmentList::const_iterator seg = m_Segments.begin(); seg != m_Segments.end(); ++seg) {

		gpstring lhstags;

		for (tagi = 0; tagi < 32; tagi++) {
			if (BitTest((*seg).m_LHSTag, tagi)) {
				lhstags.append(stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true));
				lhstags.append(" ");
			}
		}

		gpstring rhstags;
		for (tagi = 0; tagi < 32; tagi++) {
			if (BitTest((*seg).m_RHSTag, tagi)) {
				rhstags.append(stringtool::MakeFourCcString(s_AnimEventFourCC[tagi],true));
				rhstags.append(" ");
			}
		}

		float LHSWeight = 0;
		float RHSWeight = 0;
		for (WarpBlendList::const_iterator bli = m_Blends.begin();bli != m_Blends.end(); ++bli)
		{
			if ((*bli).m_pBlendGroup == (*seg).m_pBlendGroup)
			{
				LHSWeight = (*bli).m_LHSWeight;
				RHSWeight = (*bli).m_RHSWeight;
				break;
			}
		}

		out.appendf("k:%08x bgp: %08x bgi: %2d t: %5.3f i: %5.3f LHS:%5.3f %08x %s RHS:%5.3f %08x %s\n",
			(int)(*seg).m_pBlendGroup->m_KeyInfo[(*seg).m_BlendGroupIndex].GetKeys(b),
			(*seg).m_pBlendGroup,
			(*seg).m_BlendGroupIndex,
			(*seg).m_Time,
			(*seg).m_Interval,
			LHSWeight,
			(*seg).m_LHSTag,
			lhstags.c_str(),
			RHSWeight,
			(*seg).m_RHSTag,
			rhstags.c_str()
			);
	}

}

#endif