//////////////////////////////////////////////////////////////////////////////
//
// File     :  Nema_PRSKeys.cpp
// Author(s):  Mike Biddlecombe
//
// Summary  : Implementation of Position Scale Rotation keys
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "precomp_nema.h"
#include "nema_prskeys.h"

#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "nema_iostructs.h"
#include "nema_persist.h"
#include "stringtool.h"
#include "filter_1.h"

using namespace nema;

// Spoof the namespace
FUBI_REPLACE_NAME("nemaPRSKeys",PRSKeys);


//************************************************************
PRSKeysImpl::
PRSKeysImpl(const_mem_ptr data)
	: m_RefCount(0)
{

	const char *runner = (const char *)data.mem;

	// Parse the anim
	const sNeMaAnim_Chunk *header = (const sNeMaAnim_Chunk *)runner;

	m_MajorVersion = header->MajorVersion;
	m_MinorVersion = header->MinorVersion;

	if (m_MajorVersion < 2)
	{
		gpfatal("The PRS Keys version earlier then 2.0 are no longer supported");
	}
	else
	{
		runner += sizeof(sNeMaAnim_Chunk);
	}

	gpassert(header->ChunkName == IDAnimHeader);

	// **************** Read in the displacement & duration data

	m_Duration = header->Duration;
	m_Frequency = 1.0f/m_Duration;

	m_LinearVelocity =
		vector_3(header->LinearDisplacementX,
				 header->LinearDisplacementY,
				 header->LinearDisplacementZ) * (1/header->Duration);

	m_ScalarVelocity = Length(m_LinearVelocity);

	// **************** Skip the STRING data

	runner += header->StringTableSize;

	// **************** Read in the 2.2 and later version notes
	if ( (m_MajorVersion > 1) || ((m_MajorVersion == 1) && (m_MinorVersion > 1)) ) {

		const sAnimNoteList_Chunk *tempNoteList = (const sAnimNoteList_Chunk*)runner;
		runner += sizeof(sAnimNoteList_Chunk);

		m_LoopAtEnd = false;

		for (DWORD t = 0; t < tempNoteList->NumberOfNotes; t++) {

			const sAnimNote_Chunk *tempNote = (const sAnimNote_Chunk*)runner;
			runner += sizeof(sAnimNote_Chunk);

			float notetime = tempNote->Time;

			if (m_MajorVersion < 3)
			{
				// The notetrack isn't normalized
				notetime *= m_Frequency;
			}

			if (IsEqual(notetime,0) && (tempNote->FourCC == REVERSE_FOURCC('BEGL'))) {
				m_LoopAtEnd = true;
				continue;	// Don't store BEGL events
			}
			if (tempNote->FourCC == REVERSE_FOURCC('ENDL')) {
				continue;	// Don't store ENDL events
			}

			// Ignore events we don't recognize
			if (!AnimEventMaskFromFourCC( tempNote->FourCC ) ) {
				continue;
			}

			if (notetime<0) notetime+=1.0;
			m_CriticalEvents.AddEvent(notetime,tempNote->FourCC);

		}

		if (m_LoopAtEnd && (m_CriticalEvents.NumEvents() > 1)) {

			m_CriticalEvents.MakeFirstMatchLast();

		} else {

			m_CriticalEvents.AddEvent(0.0f,REVERSE_FOURCC('GSRT'));
			m_CriticalEvents.AddEvent(1.0f,REVERSE_FOURCC('GFIN'));
		}

	}

	m_pTracer = NULL;

	if (m_MajorVersion > 2 || ((m_MajorVersion == 2) && (m_MinorVersion >= 3)) ) 
	{
		// See if there are any tracers in the file
		const sAnimTracerList_Chunk *tempTracerList = (const sAnimTracerList_Chunk*)runner;
		if ( tempTracerList->ChunkName == REVERSE_FOURCC('TRCR') )
		{
			runner += sizeof(sAnimTracerList_Chunk);
			m_pTracer = 
				new TracerKeys(	tempTracerList->StartTime,
								tempTracerList->EndTime,
								tempTracerList->NumberOfTracers);

			m_pTracer->m_Rotations.uninitialized_resize(tempTracerList->NumberOfTracers);
			m_pTracer->m_Positions.uninitialized_resize(tempTracerList->NumberOfTracers);

			DWORD size = sizeof(matrix_3x3)*tempTracerList->NumberOfTracers;
			memcpy(&m_pTracer->m_Rotations[0],runner,size);
			runner += size;

			size = sizeof(vector_3)*tempTracerList->NumberOfTracers;
			memcpy(&m_pTracer->m_Positions[0],runner,size);
			runner += size;

		}
	}

	if (!(((m_MajorVersion>3) || ((m_MajorVersion==3) && (m_MinorVersion>0)))) && 
			  ((m_MajorVersion>1) ||  ((m_MajorVersion==1) && (m_MinorVersion>0))))
	{

		// **************** Read in the ROOT KEYS (and discard them!)

		const sAnimRootKeyList_Chunk *header = (const sAnimRootKeyList_Chunk*)runner;

		if (m_MajorVersion<3)
		{
			// $$$$$$$$
			// There are still some front end anims in old formats ARRRGH!
			// $$$$$$$$

			// The there are an equal number of position and rotation keys

			runner += sizeof(sAnimRootKeyList_Chunk_2_x);

			for ( DWORD i = 0; i < header->NumberOfRotKeys; ++i )
			{
				runner += sizeof(sAnimRotPosKey_Chunk);
			}
		}
		else
		{
			// The there are an equal number of position and rotation keys
			runner += sizeof(sAnimRootKeyList_Chunk);

			DWORD size = header->NumberOfRotKeys * (sizeof(float)+sizeof(Quat));

			runner += size;

			size = header->NumberOfPosKeys * (sizeof(float)+sizeof(vector_3));

			runner += size;

		}
	}

	// **************** Read in the KEY LISTS
	m_TrackData.resize(header->NumberOfKeyLists);

	for (DWORD i = 0;  i < header->NumberOfKeyLists; i++)
	{
		const sAnimKeyList_Chunk *tempkeylist = (const sAnimKeyList_Chunk*)runner;

		gpassert(tempkeylist->ChunkName == IDAnimKeyList);
		PRSKeyTrackData& trackData = m_TrackData[i];

		if ((m_MajorVersion<3))
		{
			// $$$$$$$$
			// There are still some front end anims in old formats ARRRGH!
			// $$$$$$$$

			runner += sizeof(sAnimKeyList_Chunk_2_x);

			trackData.m_RotData.uninitialized_resize(tempkeylist->NumberOfRotKeys);
			trackData.m_PosData.uninitialized_resize(tempkeylist->NumberOfRotKeys);

			RotMap::iterator irot = trackData.m_RotData.begin();
			PosMap::iterator ipos = trackData.m_PosData.begin();

			for ( DWORD k = 0; k < tempkeylist->NumberOfRotKeys; ++k, ++irot, ++ipos )
			{
				const sAnimRotPosKey_Chunk *tempkey = (const sAnimRotPosKey_Chunk*)runner;
				runner += sizeof(sAnimRotPosKey_Chunk);

				float normtime = (m_Frequency*tempkey->Time);

				irot->first = normtime;
				irot->second.Set( &tempkey->RotX );
				ipos->first = normtime;
				ipos->second.Set( &tempkey->PosX );
			}
		}
		else
		{
			runner += sizeof(sAnimKeyList_Chunk);
			DWORD size = tempkeylist->NumberOfRotKeys * (sizeof(float)+sizeof(Quat));

			trackData.m_RotData.uninitialized_resize(tempkeylist->NumberOfRotKeys);
			memcpy(&(*trackData.m_RotData.begin()),runner,size);
			runner += size;

			size = tempkeylist->NumberOfPosKeys * (sizeof(float)+sizeof(vector_3));

			trackData.m_PosData.uninitialized_resize(tempkeylist->NumberOfPosKeys);
			memcpy(&(*trackData.m_PosData.begin()),runner,size);
			runner += size;

		}
	}

	// **************** Read in the pre version 2.2 notes
	if (m_MajorVersion < 2 || ((m_MajorVersion == 2) && (m_MinorVersion < 2)) ) {

		const sAnimNoteList_Chunk *tempNoteList = (const sAnimNoteList_Chunk*)runner;
		runner += sizeof(sAnimNoteList_Chunk);

		m_LoopAtEnd = false;

		for (DWORD t = 0; t < tempNoteList->NumberOfNotes; t++) {

			const sAnimNote_Chunk *tempNote = (const sAnimNote_Chunk*)runner;
			runner += sizeof(sAnimNote_Chunk);

			float notetime = tempNote->Time;

			if (IsEqual(notetime,0) && (tempNote->FourCC == REVERSE_FOURCC('BEGL'))) {
				m_LoopAtEnd = true;
				continue;	// Don't store BEGL events
			}
			if (tempNote->FourCC == REVERSE_FOURCC('ENDL')) {
				continue;	// Don't store ENDL events
			}

			// Ignore events we don't recognize
			if (!AnimEventMaskFromFourCC( tempNote->FourCC ) ) {
				continue;
			}

			m_CriticalEvents.AddEvent(notetime,tempNote->FourCC);

		}

		if (m_CriticalEvents.NumEvents() > 1) {

			if (m_LoopAtEnd && (m_CriticalEvents.NumEvents() > 1)) {

				m_CriticalEvents.MakeFirstMatchLast();

			} else {

				m_CriticalEvents.AddEvent(0.0f,REVERSE_FOURCC('GSRT'));
				m_CriticalEvents.AddEvent(1.0f,REVERSE_FOURCC('GFIN'));

			}

		} else {

			m_CriticalEvents.AddEvent(0.0f,REVERSE_FOURCC('GSRT'));
			m_CriticalEvents.AddEvent(1.0f,REVERSE_FOURCC('GFIN'));

		}

	}


	// **************** Read in the End-Of-Anim

	const DWORD *tmpEndMark = (const DWORD*)runner;
	runner += sizeof(DWORD);

	gpassert(*tmpEndMark == IDAnimEndMarker);

}

//************************************************************
PRSKeysImpl::
~PRSKeysImpl(void)
{
	delete m_pTracer; m_pTracer = NULL;
}

//************************************************************
PRSKeys::PRSKeys(const_mem_ptr data)
{

	// Make sure we decent data loaded first
	// then we can create the PRSKeys

	m_pImpl = new PRSKeysImpl(data);
	m_pImpl->AddRef();

	m_Tracks.resize(m_pImpl->m_TrackData.size());

	for (DWORD t = 0; t < m_pImpl->m_TrackData.size() ; ++t) {

		m_Tracks[t].m_pData	= &m_pImpl->m_TrackData[t];

		m_Tracks[t].m_CurrPosition	= vector_3::ZERO;
		m_Tracks[t].m_CurrRotation	= Quat::IDENTITY;

	}

	m_CurrentTime		= -1.0f;
	m_CurrentBlendWeight = 0.0f;

}

//************************************************************
PRSKeys::PRSKeys(PRSKeysImpl* pimpl) {

	// Make sure we decent data loaded first
	// then we can create the PRSKeys

	m_pImpl = pimpl;

	gpassertf(m_pImpl,("Failed to allocate a PRSKeys implementation"));

	if (!m_pImpl) {
		return;
	}

	m_pImpl->AddRef();

	m_Tracks.resize(m_pImpl->m_TrackData.size());

	for (DWORD t = 0; t < m_pImpl->m_TrackData.size() ; ++t) {

		m_Tracks[t].m_pData	= &m_pImpl->m_TrackData[t];

		m_Tracks[t].m_CurrPosition	= vector_3::ZERO;
		m_Tracks[t].m_CurrRotation	= Quat::IDENTITY;

	}

	m_CurrentTime	= -1.0f;
	m_CurrentBlendWeight = 0.0f;

}

//************************************************************
PRSKeys::PRSKeys(PRSKeys* original) {

	// Make sure we decent data loaded first
	// then we can create the PRSKeys

	m_pImpl = original->m_pImpl;

	gpassertf(m_pImpl,("Failed to allocate a PRSKeys implementation"));

	if (!m_pImpl) {
		return;
	}

	m_pImpl->AddRef();

	m_Tracks			= original->m_Tracks;

	m_CurrentTime		 = -1.0f;
	m_CurrentBlendWeight = 0.0f;

}

//************************************************************
PRSKeys::
~PRSKeys(void) {

	if (m_pImpl->Release() == 0) {
		delete m_pImpl;
		m_pImpl = 0;
	}

}

//************************************************************
bool PRSKeys::
RotUpdateHelper(float target_time,RotMap& keys,Quat& outrot, bool loopatend)
{

	if (keys.empty()) return false;

	if (keys.size() == 1)
	{
		outrot = (*keys.begin()).second;
	} 
	else
	{

		RotMapIter rhs = keys.lower_bound(target_time);

		if (rhs == keys.begin()) {

			outrot = KEY_VAL(rhs);

		} else {

			RotMapIter lhs = rhs;

			--lhs;

			float lhs_time,rhs_time;

			lhs_time = TIME_VAL(lhs);

			if (IsZero(lhs_time - target_time)) {

				outrot = KEY_VAL(lhs);

			} else {

				if (rhs == keys.end())
				{
					if (loopatend)
					{
						rhs = keys.begin();
						rhs_time = TIME_VAL(rhs)+1.0f;

						// $$$ should I store the inverse interval between keys? probably, though it will
						// increase the key storage

						float alpha = (target_time - lhs_time)/(rhs_time - lhs_time);
						Slerp(outrot,KEY_VAL(lhs),KEY_VAL(rhs),alpha);
					}
					else
					{
						outrot = KEY_VAL(lhs);
					}
				} else {

					rhs_time = TIME_VAL(rhs);

					if (IsZero(rhs_time - target_time)) {

						outrot = KEY_VAL(rhs);

					} else {

						// $$$ should I store the inverse interval between keys? probably, though it will
						// increase the key storage

						float alpha = (target_time - lhs_time)/(rhs_time - lhs_time);
						Slerp(outrot, KEY_VAL(lhs),KEY_VAL(rhs),alpha);
					}
				}
			}
		}
	}

	return true;
}

//************************************************************
bool PRSKeys::
PosUpdateHelper(float target_time, PosMap& keys, vector_3& outpos, bool loopatend)
{

	if ( keys.empty() ) return false;

	if (keys.size() == 1)
	{
		outpos = (*keys.begin()).second;
	} 
	else
	{

		PosMapIter rhs = keys.lower_bound(target_time);

		if (rhs == keys.begin()) {

			outpos = KEY_VAL(rhs);

		} else {

			PosMapIter lhs = rhs;

			--lhs;

			float lhs_time,rhs_time;

			lhs_time = TIME_VAL(lhs);

			if (IsZero(lhs_time - target_time)) {

				outpos = KEY_VAL(lhs);

			} else {

				if (rhs == keys.end()) {

					if (loopatend)
					{
						rhs = keys.begin();
						rhs_time = TIME_VAL(rhs)+1.0f;

						// $$$ should I store the inverse interval between keys? probably, though it will
						// increase the key storage

						float alpha = (target_time - lhs_time)/(rhs_time - lhs_time);
						outpos  = KEY_VAL(lhs) + ((KEY_VAL(rhs)- KEY_VAL(lhs)) * alpha);
					}
					else
					{
						outpos = KEY_VAL(lhs);
					}

				} else {

					rhs_time = TIME_VAL(rhs);

					if (IsZero(rhs_time - target_time)) {

						outpos = KEY_VAL(rhs);
					} else {

						// $$$ should I store the inverse interval between keys? probably, though it will
						// increase the key storage

						float alpha = (target_time - lhs_time)/(rhs_time - lhs_time);
						outpos  = KEY_VAL(lhs) + ((KEY_VAL(rhs) - KEY_VAL(lhs)) * alpha);
					}
				}
			}
		}
	}


	return true;
}

//************************************************************
bool PRSKeys::
UpdateAbsolute(float normalized_target_time, float normalized_min_time) {

	// We need to be sure that the duration of the animation is not zero
	// in order to avoid going into an infinite loop while updating

	if (normalized_target_time > 1.0) return false;
	if (normalized_target_time < 0.0) return false;

	if (IsEqual(m_CurrentTime,normalized_target_time, normalized_min_time)) 
	{
		m_CurrentTime = normalized_target_time;
		return false;		
	} 

	PRSKeyTrackArray::iterator i, ibegin = m_Tracks.begin(), iend = m_Tracks.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		RotUpdateHelper(normalized_target_time,i->m_pData->m_RotData,i->m_CurrRotation,m_pImpl->m_LoopAtEnd);
		PosUpdateHelper(normalized_target_time,i->m_pData->m_PosData,i->m_CurrPosition,m_pImpl->m_LoopAtEnd);
	}

	m_CurrentTime = normalized_target_time;

	return true;

}

//************************************************
bool TracerKeys::BuildTracerTriStrip( float current_time,
									  const vector_3& in_p0,
									  const vector_3& in_p1,
									  stdx::fast_vector<sVertex>& out_points) const
{

	const float TRACER_DURATION = 0.125f;
	const DWORD TRACER_COLOR0 = MAKEDWORDCOLOR(0.2750f,0.2750f,0.2750f) & 0x00ffffff;
	const DWORD TRACER_COLOR1 = MAKEDWORDCOLOR(0.1500f,0.1500f,0.1500f) & 0x00ffffff;

	out_points.clear();

	float turn_on  = current_time - TRACER_DURATION;
	float turn_off = current_time;

	if (turn_off < m_StartKeyTime)
	{
		return false;
	}

	if (turn_on > m_FinishKeyTime)
	{
		return false;
	}

	if (IsEqual(m_StartKeyTime,m_FinishKeyTime))
	{
		return false;
	}

	int count = m_Rotations.size();

	float inv_duration = 1/(m_FinishKeyTime - m_StartKeyTime);
	float frame_rate = count * inv_duration;

	float norm_turn_on;
	float norm_turn_off;
	DWORD first_key;
	DWORD last_key;

	if (turn_on < m_StartKeyTime)
	{
		norm_turn_on = (m_StartKeyTime-turn_on) * inv_duration;
		first_key = 0;
	}
	else
	{
		norm_turn_on = 0.0f;
		first_key =  (int)floorf((frame_rate * (turn_on-m_StartKeyTime))+0.5f);
	}

	if (turn_off > m_FinishKeyTime)
	{
		norm_turn_off = (m_FinishKeyTime-turn_on) * inv_duration;
		last_key = count;
	}
	else
	{
		norm_turn_off = 1.0f;
		last_key =  (int)floorf((frame_rate * (turn_off-m_StartKeyTime))+0.5f);
	}

	DWORD num_keys = last_key - first_key;

	if ( num_keys < 2 )
	{
		return false;
	}

	float u = (float)first_key/count;
	float u_delta = 1.0f/count;

	out_points.uninitialized_resize((num_keys)*2);

	DWORD i,j;

	for (i = first_key, j = 0 ; i < last_key ; i++, j+=2)
	{
		vector_3& p0 = *(vector_3*)&out_points[j].x;
		m_Rotations[i].Transform(p0,in_p0);
		p0 += m_Positions[i];

		out_points[j].uv.u = u;
		out_points[j].uv.v = 0.0;

		DWORD alpha = (int)floorf(FilterSmoothStep((float)first_key,(float)last_key,(float)i) * 255.0f);

		out_points[j].color = ( alpha << 24 ) | TRACER_COLOR0;

		vector_3& p1 = *(vector_3*)&out_points[j+1].x;
		m_Rotations[i].Transform(p1,in_p1);
		p1 += m_Positions[i];

		out_points[j+1].uv.u = u;
		out_points[j+1].uv.v = 1.0;

		out_points[j+1].color = ( alpha << 24 ) | TRACER_COLOR1;

		u += u_delta;
	}

	return true;
}
//************************************************
bool TracerKeys::BuildTracerTriMesh( float current_time,
									  const vector_3& in_p0,
									  const vector_3& in_p1,
									  stdx::fast_vector<sVertex>& out_points) const
{

	const float TRACER_DURATION = 0.125f;
	const DWORD TRACER_COLOR0 = MAKEDWORDCOLOR(0.2750f,0.2750f,0.2750f) & 0x00ffffff;
	const DWORD TRACER_COLOR1 = MAKEDWORDCOLOR(0.2125f,0.2125f,0.2125f) & 0x00ffffff;
	const DWORD TRACER_COLOR2 = MAKEDWORDCOLOR(0.1500f,0.1500f,0.1500f) & 0x00ffffff;

	out_points.clear();

	float turn_on  = current_time - TRACER_DURATION;
	float turn_off = current_time;

	if (turn_off < m_StartKeyTime)
	{
		return false;
	}

	if (turn_on > m_FinishKeyTime)
	{
		return false;
	}

	if (IsEqual(m_StartKeyTime,m_FinishKeyTime))
	{
		return false;
	}

	int count = m_Rotations.size();

	float inv_duration = 1/(m_FinishKeyTime - m_StartKeyTime);
	float frame_rate = count * inv_duration;

	float norm_turn_on;
	float norm_turn_off;
	DWORD first_key;
	DWORD last_key;

	if (turn_on < m_StartKeyTime)
	{
		norm_turn_on = (m_StartKeyTime-turn_on) * inv_duration;
		first_key = 0;
	}
	else
	{
		norm_turn_on = 0.0f;
		first_key =  (int)floorf((frame_rate * (turn_on-m_StartKeyTime))+0.5f);
	}

	if (turn_off > m_FinishKeyTime)
	{
		norm_turn_off = (m_FinishKeyTime-turn_on) * inv_duration;
		last_key = count;
	}
	else
	{
		norm_turn_off = 1.0f;
		last_key =  (int)floorf((frame_rate * (turn_off-m_StartKeyTime))+0.5f);
	}

	DWORD num_keys = last_key - first_key;

	if ( num_keys < 2 )
	{
		return false;
	}

	float u = (float)first_key/count;
	float u_delta = 1.0f/count;

	out_points.uninitialized_resize((num_keys)*5);

	stdx::fast_vector<sVertex>::iterator j = out_points.begin();

	for (DWORD i = first_key ; i < last_key ; i++)
	{
		//	p0 = transformed tracer 0
		//	p1 = transformed tracer 1
		//	p2 = (p0+p1) * 0.5;
		//	p3 = (p0+p2) * 0.5;
		//	p4 = (p2+p1) * 0.5;
		// 
		// p0 -- p3 -- p2 -- p4 -- p1

		// Point 0
		vector_3& p0 = *(vector_3*)&((*j).x);
		m_Rotations[i].Transform(p0,in_p0);
		p0 += m_Positions[i];

		(*j).uv.u = u;
		(*j).uv.v = 0.0;

		DWORD alpha = (int)floorf(FilterSmoothStep((float)first_key,(float)last_key,(float)i) * 255.0f);

		(*j).color = ( alpha << 24 ) | TRACER_COLOR0;
		++j;

		// Point 1
		vector_3& p1 = *(vector_3*)&((*j).x);
		m_Rotations[i].Transform(p1,in_p1);
		p1 += m_Positions[i];

		(*j).uv.u = u;
		(*j).uv.v = 1.0;

		(*j).color = ( alpha << 24 ) | TRACER_COLOR0;
		++j;

		// Point 2
		vector_3& p2 = *(vector_3*)&((*j).x);
		p2 = ( p0 + p1 ) * 0.5;

		(*j).uv.u = u;
		(*j).uv.v = 0.5;

		(*j).color = ( alpha << 24 ) | TRACER_COLOR1;
		++j;

		// Point 3
		vector_3& p3 = *(vector_3*)&((*j).x);
		p3 = ( p0 + p2 ) * 0.5;

		(*j).uv.u = u;
		(*j).uv.v = 0.5;

		(*j).color = ( alpha << 24 ) | TRACER_COLOR2;
		++j;

		// Point 4
		vector_3& p4 = *(vector_3*)&((*j).x);
		p4 = ( p2 + p1 ) * 0.5;

		(*j).uv.u = u;
		(*j).uv.v = 0.5;

		(*j).color = ( alpha << 24 ) | TRACER_COLOR2;
		++j;

		u += u_delta;
	}

	return true;
}
