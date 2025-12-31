#pragma error, "no longer in NEMA"
// TODO: wrap up a filesize() and loadbuffer()
// or use the generic game library -- biddle
#include "precomp_nema.h"
#include "filesys.h"
#include "stringtool.h"

using namespace nema;


//************************************************************
//************************************************************
//************************************************************
//
//  Base Controller functionality
//
//************************************************************
//************************************************************
//************************************************************

tControllerArray::
tControllerArray(gpstring initName, int initNum) 
	: m_Name(initName)
	, m_NumControllers(initNum)
{

	m_Controllers = new tController*[initNum];
	memset(m_Controllers,0,sizeof(tController*)*initNum);

}

//************************************************************
tControllerArray::
~tControllerArray(void)
{

	for (int i = 0; i < m_NumControllers ; i++) {
		delete m_Controllers[i];
	}
	delete [] m_Controllers;

}

//************************************************************
tController* 
tControllerArray::GetController(int i) const {
	gpassert(i >= 0);
	gpassert(i < m_NumControllers);
	return m_Controllers[i];
}

//************************************************************
void
tControllerArray::SetController(int i, tController* c) {
	gpassert(i >= 0);
	gpassert(i < m_NumControllers);
	m_Controllers[i]=c;
}

//************************************************************
tControllerStorage::~tControllerStorage(void) {

	for (iter i = m_ArraysList.begin(); i != m_ArraysList.end(); ++i) {
		delete (*i);
	}

	for (singleiter s = m_SingletonList.begin(); s != m_SingletonList.end(); ++s) {
		delete (*s);
	}

}

//************************************************************
//************************************************************
//************************************************************
tControllerArray*
tControllerStorage::FindControllerArray(const char* arrayname) {


	for (iter it = m_ArraysList.begin(); it!= m_ArraysList.end(); ++it) {
		if ((*it)->GetName() == arrayname) {
			return (*it);
		}
	}

	return NULL;
}


//************************************************************
tControllerArray*
tControllerStorage::LoadControllerArray(const char* arrayname, const char* fname, bool loopatend) {

	tControllerArray* newarray=NULL;

	//------------

	FileSys::AutoFileHandle file;
	FileSys::AutoMemHandle mem;
	if ( !file.Open( fname ) || !mem.Map( file ) )
	{
		return ( NULL );
	}

	//------------

	if ( mem.GetSize() != 0 ) {

		// Remove any existing controller array with this name on it

		for (iter it = m_ArraysList.begin(); it!= m_ArraysList.end(); ++it) {
			if ((*it)->GetName() == arrayname) {
				delete (*it);
				it = m_ArraysList.erase(it);
			}

		}

		const char *runner = (const char*)mem.GetData();

		// Parse the anim
		const sNeMaAnim_Chunk *header = (const sNeMaAnim_Chunk *)runner;
		runner += sizeof(sNeMaAnim_Chunk);

		gpassert(header->ChunkName == IDAnimHeader);
		
		// **************** Make room for the controller we are about to load
		newarray = new tControllerArray(arrayname, header->NumberOfKeyLists);
		gpassert(newarray);

		// **************** Read in the velocity data
		newarray->SetVelocity(vector_3(header->DeltaX,header->DeltaY, header->DeltaZ) * (1/header->Duration));

		// **************** Read in the STRING data
		const char* tmpstringtable = runner;
		runner += header->StringTableSize;

		// **************** Read in the KEY LISTS

		for (int i = 0;  i < header->NumberOfKeyLists; i++) {

			const sAnimKeyList_Chunk *tempkeylist = (const sAnimKeyList_Chunk*)runner;
			runner += sizeof(sAnimKeyList_Chunk);

			const char* keyname = tmpstringtable + tempkeylist->BoneNameOffset;

			gpassert(tempkeylist->ChunkName == IDAnimKeyList);

			tPRSController* curr_controller = new tPRSController(loopatend);

			newarray->SetController(i,curr_controller);

			gpassertm(curr_controller != NULL,"Can't find a match for the key name");

			tKeySequence* curr_key_seq = curr_controller->GetKeySequence();

			curr_key_seq->SetSequenceDuration(header->Duration);

			for (int k = 0; k < tempkeylist->NumberOfKeys; k++) {
				const sAnimKey_Chunk *tempkey = (const sAnimKey_Chunk*)runner;
				runner += sizeof(sAnimKey_Chunk);

				// Add the new keys to the controller

				tKey *newkey;
				if (i == 0) { 

					// This is the ROOT, we need to make sure to account for Y is UP

					newkey = new tKey(tempkey->Time,
								vector_3(-tempkey->PosX,tempkey->PosZ,tempkey->PosY),
								PreRotateNEMAtoSEIGE *
									Quat(tempkey->RotX,tempkey->RotY,tempkey->RotZ,tempkey->RotW)
								);

				} else {

					// This is NOT the ROOT, handle it properly
					newkey = new tKey(tempkey->Time,
								vector_3(tempkey->PosX,tempkey->PosY,tempkey->PosZ),
								Quat(tempkey->RotX,tempkey->RotY,tempkey->RotZ,tempkey->RotW)
								);

				}

				curr_key_seq->InsertKey(newkey);
			}

			// Now that we have all the keys, build up the internal intervals
			// between keys
			curr_key_seq->BuildIntervals();

		}

		// **************** Read in the End-Of-Anim

		const DWORD *tmpEndMark = (const DWORD*)runner;
		runner += sizeof(DWORD);

		gpassert(*tmpEndMark == IDAnimEndMarker);

		m_ArraysList.push_back(newarray);

	}

	return newarray;

}

//************************************************************
tController* 
tControllerStorage::CreateSpinController(const char* ctrlname, float rpm) {

	// TODO:: don't just ignore the name --biddle
	// TODO:: should accept a spin axis -- biddle

	tSpinController *newspinner = new tSpinController(rpm);
	gpassert(newspinner);

	m_SingletonList.push_back(newspinner);

	return newspinner;
}

//************************************************************
tController* 
tControllerStorage::CreateTranslationController(const char* ctrlname, const vector_3& pA,const vector_3& pB, float cps) {

	// TODO:: don't just ignore the name --biddle

	tTranslationController *newtranslate = new tTranslationController(pA,pB,cps);
	gpassert(newtranslate);

	m_SingletonList.push_back(newtranslate);

	return newtranslate;
}

//************************************************************
//************************************************************
//************************************************************
//
//  Derived controllers 
//
//************************************************************
//************************************************************
//************************************************************

// ===========================================================
// Position Rotation Scale Controller:
// Stores a list of keys containing PRS data
// ===========================================================

tPRSController::tPRSController(bool initLoopAtEnd) 
	: m_LoopAtEnd(initLoopAtEnd)
{
	// TODO:: I really don't like the duplication of the 'loop at end' in the Sequence -- biddle
	m_KeySequence = new tKeySequence(initLoopAtEnd);
}


// ===========================================================
tPRSController::~tPRSController(void) {
	delete m_KeySequence;
}


// ===========================================================
bool 
tPRSController::UpdateChannel(tChannel* chan,
							  float deltat,
							  sBone& bone ) {

	// Fetch the relevant keys and interpolate
	
	tKey* prevkey  = (tKey*)bone.m_storedData0;
	float prevtime = *(float*)&bone.m_storedData1;

	bool wrapped;
	bool changed = m_KeySequence->UpdateCurrentKey(deltat,prevkey,prevtime,wrapped);

	if (wrapped) {
		// TODO:: Should I call the completion function now, or should I wait
		// until the object has been rendered? -- biddle
		chan->CallCompletionCallback();

		prevkey  = (tKey*)bone.m_storedData0;
		prevtime = *(float*)&bone.m_storedData1;

	}

	if (!changed) return false;


	float alpha;
	tKey *lhs=0;
	tKey *rhs=0;

	m_KeySequence->GetCurrentBracket(alpha,lhs,rhs);

	gpassert(lhs);

	vector_3 pos;
	Quat rot;

	if (rhs) {

		vector_3 kidpos = ((1-alpha)*lhs->m_Position + alpha*rhs->m_Position);
		Quat kidrot = Slerp(lhs->m_Rotation,rhs->m_Rotation,alpha);

		tChannel* pchan = chan->GetActiveParent();
		gpassertm(pchan!=NULL, "The active parent of this controller was never assigned")


		if (pchan && (pchan != chan)) {

			// We have a valid parent
			Quat	 parentrot = pchan->GetCurrentRotation();
			vector_3 parentpos = pchan->GetCurrentPosition();
			
			bone.m_rotation = parentrot * kidrot;
			bone.m_position = parentpos + parentrot.RotateVector(kidpos);

		} else {

			bone.m_rotation = kidrot;
			bone.m_position = kidpos;

		}

	} else {

		vector_3 kidpos = lhs->m_Position;
		Quat kidrot = lhs->m_Rotation;

		tChannel* pchan = chan->GetParent();
		gpassertm(pchan!=NULL, "The parent of this controller was never assigned")

		if (pchan && (pchan != chan)) {

			// We have a valid parent
			Quat	 parentrot = pchan->GetCurrentRotation();
			vector_3 parentpos = pchan->GetCurrentPosition();
			
			bone.m_rotation = parentrot * kidrot;
			bone.m_position = parentpos + parentrot.RotateVector(kidpos);

		} else {

			bone.m_rotation = kidrot;
			bone.m_position = kidpos;
		}


	}

	// Keep track of the current key/time in the channel so the controller 
	// is 'data driven'

	bone.m_storedData0 = (DWORD)lhs;
	float tmp = m_KeySequence->GetCurrTime();
	bone.m_storedData1 = *(DWORD *)&tmp;

	chan->SetCurrentRotation(bone.m_rotation);
	chan->SetCurrentPosition(bone.m_position);

	return true;
}

// ===========================================================
bool 
tPRSController::AttachToChannel(tChannel* chan, sBone& bone) {

	gpassertm(m_KeySequence,"There's no key sequence for the PRS controller to are tyring to attach with");

	if (m_KeySequence && m_KeySequence->GetFirstKey()) {
		chan->SetController(this);
		bone.m_position = m_KeySequence->GetFirstKey()->m_Position;
		bone.m_rotation = m_KeySequence->GetFirstKey()->m_Rotation;
		bone.m_storedData0 = (DWORD)(m_KeySequence->GetFirstKey());
		float tmp = 0.0f;
		bone.m_storedData1 = *(DWORD *)&tmp;
		return true;
	}

	chan->SetCurrentRotation(bone.m_rotation);
	chan->SetCurrentPosition(bone.m_position);

	return false;
}

// ===========================================================
// Rigid Link Controller:
// 
// TODO -- could store a relative offest from the parent
//  (in the parents' coord sys)
// 
// ===========================================================
//==== Rigid Link Controller =================================

bool 
tRigidLinkController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {

	tChannel* pchan = chan->GetActiveParent();
	gpassertm(pchan!=NULL, "The active parent of this controller was never assigned")

	gpassertm(pchan != chan, "This channel is its own parent")

	if (pchan && (pchan != chan)) {

		// We have a valid parent
		Quat	 parentrot = pchan->GetCurrentRotation();
		vector_3 parentpos = pchan->GetCurrentPosition();

		bone.m_rotation = parentrot * chan->GetOffsetRotation();
		bone.m_position = parentpos + parentrot.RotateVector(chan->GetOffsetPosition());

		chan->SetCurrentRotation(bone.m_rotation);
		chan->SetCurrentPosition(bone.m_position);

		return true;

	} else {

		return false;

	}

}

// ===========================================================
bool 
tRigidLinkController::AttachToChannel(tChannel* chan, sBone& bone) {

	chan->SetController(this);

	bone.m_rotation = bone.m_sharedbone->m_restRot;
	bone.m_position = bone.m_sharedbone->m_restPos;
	chan->SetCurrentRotation(bone.m_sharedbone->m_restRot);
	chan->SetCurrentPosition(bone.m_sharedbone->m_restPos);

	return true;
}

// ===========================================================
// Relative Link Controller:
// 
// ===========================================================
//==== Relative Link Controller ==============================

bool 
tRelativeLinkController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {

	tChannel* pchan = chan->GetActiveParent();
	gpassertm(pchan!=NULL, "The active parent of this controller was never assigned")

	gpassertm(pchan != chan, "This channel is its own parent")

	if (pchan && (pchan != chan)) {

		// We have a valid parent
		Quat	 parentrot = pchan->GetCurrentRotation();
		vector_3 parentpos = pchan->GetCurrentPosition();
			
		bone.m_rotation = parentrot * bone.m_sharedbone->m_restRot;
		bone.m_position = parentpos + parentrot.RotateVector(bone.m_sharedbone->m_restPos);

		chan->SetCurrentRotation(bone.m_rotation);
		chan->SetCurrentPosition(bone.m_position);

		return true;

	} else {

		return false;

	}

}

// ===========================================================
bool 
tRelativeLinkController::AttachToChannel(tChannel* chan, sBone& bone) {

	chan->SetController(this);

	bone.m_rotation = bone.m_sharedbone->m_restRot;
	bone.m_position = bone.m_sharedbone->m_restPos;

	chan->SetCurrentRotation(bone.m_sharedbone->m_restRot);
	chan->SetCurrentPosition(bone.m_sharedbone->m_restPos);

	return true;

}

// ===========================================================
// Absolute Link Controller:
// 
// ===========================================================

bool 
tAbsoluteLinkController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {

	if (chan) {

		bone.m_rotation = bone.m_sharedbone->m_restRot;
		bone.m_position = bone.m_sharedbone->m_restPos;

		chan->SetCurrentRotation(bone.m_rotation);
		chan->SetCurrentPosition(bone.m_position);

		return true;
	}

	return false;
}

// ===========================================================
bool 
tAbsoluteLinkController::AttachToChannel(tChannel* chan, sBone& bone) {

	chan->SetController(this);

	chan->SetCurrentRotation(bone.m_rotation);
	chan->SetCurrentPosition(bone.m_position);

	return true;

}


// ===========================================================
//
// Locked Controller:
// 
// ===========================================================

bool 
tLockedController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {


	if (chan) {

		chan->SetCurrentRotation(bone.m_rotation);
		chan->SetCurrentPosition(bone.m_position);

		return true;

	}

	return false;
}

// ===========================================================

bool 
tLockedController::AttachToChannel(tChannel* chan, sBone& bone) {

	chan->SetController(this);

	bone.m_position = chan->GetCurrentPosition();
	bone.m_rotation = chan->GetCurrentRotation();

	return true;

}


// ===========================================================
//
// 	Spin Controller:
// 
// ===========================================================

bool 
tSpinController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {

	if (chan) {

		Quat newrot = bone.m_rotation;
		newrot.RotateX(deltat*m_spinrate*(RealPi/30.0f));
		bone.m_rotation = newrot;

	}

	return false;
}

// ===========================================================
bool 
tSpinController::AttachToChannel(tChannel* chan, sBone& bone) {


	chan->SetController(this);

	chan->SetCurrentPosition(bone.m_sharedbone->m_restPos);
	chan->SetCurrentRotation(bone.m_sharedbone->m_restRot);

	return true;

}

// ===========================================================
//
// 	Translation Controller:
// 
// ===========================================================

bool 
tTranslationController::UpdateChannel(tChannel* chan, float deltat, sBone& bone) {


	if (chan) {

		m_tval = m_tval+(m_direction*deltat);

		if (m_tval >= m_cycletime) {

			m_direction = BACKWARD;
			m_tval = m_cycletime;

		} else if (m_tval <= 0.0f) {

			m_direction = FORWARD;
			m_tval = 0.0f;

		}

		const vector_3& pos = m_pointA+(m_tval/m_cycletime)*(m_pointB-m_pointA);

		bone.m_position = pos;

	}

	return false;
}

// ===========================================================
bool 
tTranslationController::AttachToChannel(tChannel* chan, sBone& bone) {

	chan->SetController(this);

	return true;

}

// ===========================================================

// TODO -- get rid of this global (and its EXTERN in the header file)
// Need to develop a way to handle single instance controllers properly
tRigidLinkController GenericRigidLinkController;
tRelativeLinkController GenericRelativeLinkController;
tAbsoluteLinkController GenericAbsoluteLinkController;
tLockedController GenericLockedController;



//************************************************************
//************************************************************
//************************************************************
//
//  Blending Controller functionality
//
//************************************************************
//************************************************************
//************************************************************

#if BLENDING_NEEDS_TO_BE_CONVERTED

tControllerArray*
tControllerStorage::CreateBlendingArray(const char *arrayname, tControllerArray* A, tControllerArray* B) {

	tControllerArray* newarray=NULL;

	// Remove any existing controller array with this name on it

	for (iter it = m_BlenderList.begin(); it!= m_BlenderList.end(); ++it) {
		if ((*it)->GetName() == arrayname) {
			delete (*it);
			it = m_BlenderList.erase(it);
		}

	}

	gpassertm(A->GetNumControllers() == B->GetNumControllers(),"Attempted to blend controller arrays of different lengths");

	if (A->GetNumControllers() != B->GetNumControllers()) {
		return NULL;
	}

	newarray = new tControllerArray(arrayname, A->GetNumControllers());
	gpassert(newarray);

	if (newarray == NULL) {
		return NULL;
	}

	for (int i = 0; i < A->GetNumControllers(); ++i) {
		tBlendController* ctrl = new tBlendController(A->GetController(i),B->GetController(i));
		newarray->SetController(i,ctrl);
	}
	return newarray;

}

	
// ===========================================================
//
// 	Blend Controller:
// 
// ===========================================================

tBlendController::tBlendController(tController* A, tController* B)
	: m_controllerA(A)
	, m_controllerB(B)
	, m_weight(0.5f)			// Initialize weighting at 100% controller A
{
}

// ===========================================================
bool 
tBlendController::AttachToChannel(tChannel* chan,
							  vector_3& chanpos,
							  Quat& chanrot,
							  nema::DWORD& chanparm0,
							  nema::DWORD& chanparm1) {



	gpassertm(m_controllerA && m_controllerB, "Don't have valid controllers to blend with");

	vector_3 posA,posB;
	Quat	 rotA,rotB;

	// Attach each controller one at a time so we can get the initial pos/rot and any internal data
	m_controllerA->AttachToChannel(chan,posA,rotA,m_dataA0,m_dataA1);
	m_controllerB->AttachToChannel(chan,posB,rotB,m_dataB0,m_dataB1);

	chanpos = posA * (1.0-m_weight) + posB * m_weight;
	chanrot = Slerp(rotA,rotB, m_weight);

	// Set the controller's channel here to 
	chan->SetController(this);

	return true;

}

bool 
tBlendController::UpdateChannel(tChannel* chan,
							  float deltat,
							  vector_3& chanpos,
							  Quat& chanrot,
							  nema::DWORD& chanparm0,
							  nema::DWORD& chanparm1) {
	if (chan) {

		vector_3 posA,posB;
		Quat rotA,rotB;

		m_controllerA->UpdateChannel(chan,deltat,posA,rotA,m_dataA0,m_dataA1);
		m_controllerB->UpdateChannel(chan,deltat,posB,rotB,m_dataB0,m_dataB1);

		chanpos = posA * (1.0-m_weight) + posB * m_weight;
		chanrot = Slerp(rotA,rotB, m_weight);

		return false;
	}

	return false;
}
#endif