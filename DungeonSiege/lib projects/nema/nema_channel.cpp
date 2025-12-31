#pragma error, "no longer in NEMA"
#include "precomp_nema.h"


/*========================================================================

  Channel, part of the "NeMa" system

      
  author: Mike Biddlecombe
  date: 9/13/1999
  (c)opyright 1999, Gas Powered Games

  
	TODO: 
		inline as much as possible

------------------------------------------------------------------------*/

using namespace nema;

// ***************************************************
// fwd decls
// ***************************************************
void DummyCallback(unsigned int param);

// ***************************************************
tChannel::tChannel(nema::tController* initctrl )
	: m_Controller(initctrl)
	, m_ActiveController(initctrl)
	, m_OffsetPosition(vector_3::ZERO)
	, m_OffsetRotation(Quat::IDENTITY)
	, m_Parent(NULL)
	, m_ActiveParent(NULL)
//	, m_StoredData0(NULL)		// TODO::	req'd for PRS controllers
//	, m_StoredData1(NULL)		//			can I get away from storing these?
	, m_CompletionCallback(makeFunctor(DummyCallback))

{
};

tChannel::~tChannel(void) {
};

/*
// These all should be inlined ***********************
//----
const vector_3&	
tChannel::GetPosition(void) const {
	return m_Position;
};
	
void
tChannel::SetPosition(const vector_3& newval) {
	m_Position = newval;
};

//----
const Quat&
tChannel::GetRotation(void) const {
	return m_Rotation;
};

void
tChannel::SetRotation(const Quat& newval) {
	m_Rotation = newval;
};

//----
const vector_3&
tChannel::GetScale(void) const {
	return m_Scale;
};

void
tChannel::SetScale(const vector_3& newval) {
	m_Scale = newval;
};
*/

//----
tChannel*
tChannel::GetParent(void) const {
	return m_Parent;
};

void
tChannel::SetParent(tChannel* newval) {
	m_Parent = newval;
	m_ActiveParent = newval;
};

tChannel*
tChannel::GetActiveParent(void) const {
	return m_ActiveParent;
};

void
tChannel::SetActiveParent(tChannel* newval) {
	m_ActiveParent = newval;
};

void
tChannel::ResetActiveParent(void) {
	m_ActiveParent = m_Parent;
};

//----
tController*
tChannel::GetController(void) const {
	return m_Controller;
};

void
tChannel::SetController(tController* newval) {
	m_Controller = newval;
	m_ActiveController = newval;
};

tController*
tChannel::GetActiveController(void) const {
	return m_ActiveController;
};

void
tChannel::SetActiveController(tController* newval) {
	m_ActiveController = newval;
};

void
tChannel::ResetActiveController(void) {
	m_ActiveController = m_Controller;
};

// ***************************************************

bool 
tChannel::Update(float deltat, sBone &bone) {

	// Pass the tChannel through to its active controller 

	if (m_ActiveController) {
		return m_ActiveController->UpdateChannel(this,deltat,bone);
	} else {
		return false;
	}
	
};

// ***************************************************
//
// Channel callback support
//
// ***************************************************
void 
tChannel::ResetCompletionCallback(void) {
	m_CompletionCallback = makeFunctor(DummyCallback);
}

void 
tChannel::CallCompletionCallback(void) {
	m_CompletionCallback(0);
}

void DummyCallback(unsigned int param) {
}

