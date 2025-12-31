// prsinfo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	return 0;
}

/*
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

	if (m_MajorVersion < 2 && m_MinorVersion < 1) {
		runner += sizeof(sNeMaAnim_Chunk_1_0);
	} else {
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


	// **************** Skip the STRING data
	const char* tmpstringtable = runner;
	runner += header->StringTableSize;

	float FirstEventTimeCorrection = 0.0;

	// **************** Read in the 2.2 and later version notes
	if ( (m_MajorVersion > 1) &&  (m_MinorVersion > 1) ) {

		const sAnimNoteList_Chunk *tempNoteList = (const sAnimNoteList_Chunk*)runner;
		runner += sizeof(sAnimNoteList_Chunk);

		m_LoopAtEnd = false;

		bool FirstEventTimeHasBeenCorrected = false;

		for (DWORD t = 0; t < tempNoteList->NumberOfNotes; t++) {

			const sAnimNote_Chunk *tempNote = (const sAnimNote_Chunk*)runner;
			runner += sizeof(sAnimNote_Chunk);

			float notetime = m_Frequency*tempNote->Time;

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

			if (m_LoopAtEnd && !FirstEventTimeHasBeenCorrected) {
				FirstEventTimeHasBeenCorrected = true;
				FirstEventTimeCorrection = notetime;
			}

			notetime -= FirstEventTimeCorrection;
			if (notetime<0) notetime+=1.0;
			m_CriticalEvents.AddEvent(notetime,tempNote->FourCC);

		}

		if (m_LoopAtEnd && (m_CriticalEvents.NumEvents() > 1)) {

			m_CriticalEvents.MakeFirstMatchLast();

		} else {

			m_CriticalEvents.AddEvent(0.0f,REVERSE_FOURCC('GSRT'));
			m_CriticalEvents.AddEvent(1.0f,REVERSE_FOURCC('GFIN'));
		}

//		m_CriticalEvents.DebugDump();
	} 


	// **************** Read in the ROOT KEYS

	if ((m_MajorVersion>1) && (m_MinorVersion>0)) { 

		const sAnimRootKeyList_Chunk *header = (const sAnimRootKeyList_Chunk*)runner;
		runner += sizeof(sAnimRootKeyList_Chunk);

		PRSMapIter previnsert = m_RootTrack.m_Data.end();

		for (DWORD i = 0;  i < header->NumberOfKeys; i++) {

			const sAnimKey_Chunk *tempkey = (const sAnimKey_Chunk*)runner;
			runner += sizeof(sAnimKey_Chunk);

			PRS prs;
			prs.pos = vector_3(tempkey->PosX,tempkey->PosY,tempkey->PosZ);
			prs.rot = Quat(tempkey->RotX,tempkey->RotY,tempkey->RotZ,tempkey->RotW);

			float normtime = (m_Frequency*tempkey->Time)-FirstEventTimeCorrection;
			previnsert = m_RootTrack.m_Data.insert(previnsert,std::make_pair(normtime, prs ));

		}

		if (FirstEventTimeCorrection > 0) {

			// We have to move all the keys that occurred BEFORE the first event to the end of the anim
			FirstEventCorrectionHelper(m_RootTrack.m_Data);

		}

	}

	// **************** Read in the KEY LISTS
	m_TrackData.resize(header->NumberOfKeyLists);

	for (DWORD i = 0;  i < header->NumberOfKeyLists; i++) {

		const sAnimKeyList_Chunk *tempkeylist = (const sAnimKeyList_Chunk*)runner;
		runner += sizeof(sAnimKeyList_Chunk);

		gpassert(tempkeylist->ChunkName == IDAnimKeyList);

		m_TrackData[i].m_Name = tmpstringtable + tempkeylist->BoneNameOffset;

		PRSMapIter previnsert =  m_TrackData[i].m_Data.end();

		for (DWORD k = 0; k < tempkeylist->NumberOfKeys; k++) {

			const sAnimKey_Chunk *tempkey = (const sAnimKey_Chunk*)runner;
			runner += sizeof(sAnimKey_Chunk);

			PRS prs;

			if ((m_MajorVersion<2) && i == 0) { 

				// This is the ROOT, we need to make sure to account for Y is UP

				prs.pos = PreRotateNEMAtoSEIGE.RotateVector(vector_3(tempkey->PosX,tempkey->PosY,tempkey->PosZ));
				prs.rot = PreRotateNEMAtoSEIGE * Quat(tempkey->RotX,tempkey->RotY,tempkey->RotZ,tempkey->RotW);

			} else {

				// This is NOT the ROOT, handle it without modification

				prs.pos = vector_3(tempkey->PosX,tempkey->PosY,tempkey->PosZ);
				prs.rot = Quat(tempkey->RotX,tempkey->RotY,tempkey->RotZ,tempkey->RotW);

			}

			float normtime = (m_Frequency*tempkey->Time)-FirstEventTimeCorrection;
			previnsert = m_TrackData[i].m_Data.insert(previnsert,std::make_pair( normtime, prs ));

		}

		if (FirstEventTimeCorrection > 0) {

			// We have to move all the keys with negative times (due to FET correction) to the end of the anim
			PRSMapIter k = m_TrackData[i].m_Data.begin();

			// Throw out the first key, it's duplicated at the end of the animation
			k = m_TrackData[i].m_Data.erase(k);

			while ( k != m_TrackData[i].m_Data.end() && (*k).first < 0) {
				previnsert = m_TrackData[i].m_Data.insert(previnsert,std::make_pair((*k).first+1.0f, (*k).second ));
				k = m_TrackData[i].m_Data.erase(k);
			}

			// Duplicate the new first key at the end
			m_TrackData[i].m_Data.insert(previnsert,std::make_pair((*k).first+1.0f, (*k).second ));

		}

	}

	// **************** Read in the pre version 2.2 notes
	if (m_MajorVersion < 2 || ((m_MajorVersion == 2) && (m_MinorVersion < 2)) ) {

		const sAnimNoteList_Chunk *tempNoteList = (const sAnimNoteList_Chunk*)runner;
		runner += sizeof(sAnimNoteList_Chunk);

		m_LoopAtEnd = false;

		bool FirstEventTimeHasBeenCorrected = false;

		for (DWORD t = 0; t < tempNoteList->NumberOfNotes; t++) {

			const sAnimNote_Chunk *tempNote = (const sAnimNote_Chunk*)runner;
			runner += sizeof(sAnimNote_Chunk);

			float notetime = m_Frequency*tempNote->Time;

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

			if (m_LoopAtEnd && !FirstEventTimeHasBeenCorrected) {
				FirstEventTimeHasBeenCorrected = true;
				FirstEventTimeCorrection = notetime;
			}

			notetime -= FirstEventTimeCorrection;
			if (notetime<0) notetime+=1.0;

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

//		m_CriticalEvents.DebugDump();
	}


	// **************** Read in the End-Of-Anim

	const DWORD *tmpEndMark = (const DWORD*)runner;
	runner += sizeof(DWORD);

	gpassert(*tmpEndMark == IDAnimEndMarker);

}

//************************************************************
void nema::FirstEventCorrectionHelper(PRSMap& keys) {

	PRSMapIter k = keys.begin();

	vector_3 offsetpos	= -(*k).second.pos;
	Quat offsetrot		=  (*k).second.rot.Inverse();

	vector_3 deltapos	= offsetpos + (*keys.rbegin()).second.pos;
	Quat deltarot		= offsetrot * (*keys.rbegin()).second.rot;

	PRS prs;

	PRSMapIter previnsert = keys.end();

	// Throw out the first key, it is duplicated at the end of the animation;
	k = keys.erase(k);

	while (k != keys.end() && (*k).first < 0) {
		prs = ((*k).second);
		prs.pos += deltapos;
		prs.rot = deltarot*prs.rot;
		previnsert = keys.insert(previnsert,std::make_pair((*k).first+1.0f, (*k).second ));
		k = keys.erase(k);
	}

	gpassertf(k != keys.end(),("Something went horribly wrong with the Correction helper"));
	gpassertf((*k).first == 0,("There are NO keys after the Time Correction!!"));

	// Duplicate the first key at the end
	prs = ((*k).second);
	prs.pos += deltapos;
	prs.rot = deltarot*prs.rot;
	keys.insert(previnsert,std::make_pair((*k).first+1.0f, prs ));

	// Run through the keys making sure that the first 

	k = keys.begin();

	offsetpos = -(offsetpos + (*k).second.pos);
	offsetrot =  (offsetrot * (*k).second.rot).Inverse();

	while (k != keys.end()) {
		(*k).second.pos += offsetpos;
		(*k).second.rot = offsetrot*(*k).second.rot;
		++k;
	}
}


*/