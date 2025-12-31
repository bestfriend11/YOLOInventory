#include "precomp_nema.h"

using namespace nema;

//***********************************************************************************
bool
tKeySequence::GetCurrentBracket(float& alpha, tKey* &lhs, tKey* &rhs) {

	if (m_CurrentKey == NULL) {

		return false;

	}

	alpha = m_CurrentAlpha;
	lhs = m_CurrentKey;
	rhs = m_CurrentKey->m_Right;
	if (!rhs) {
		rhs = m_FirstKey;
	}

	return true;

}

//***********************************************************************************
// Return true if the current key/time is modified
// Return wrapped == true if we reach the last key
bool
tKeySequence::UpdateCurrentKey(float deltat, tKey* prevKey, float prevTime, bool &wrapped)	{

	if (prevKey == NULL) {
		return false;
	}

	m_CurrentKey = prevKey;
	m_CurrentTime = prevTime;

	wrapped = false;

	if ( deltat >= 0 ) { 

		if (m_CurrentKey->m_Interval == INFINITE_TIME) {
			m_CurrentTime = 0;
			m_CurrentAlpha = 0;
			wrapped = true;
			return true;
		}

		deltat += m_CurrentTime;

		gpassert(m_CurrentKey->m_Interval);

		while (m_CurrentKey && (deltat > m_CurrentKey->m_Interval)) {

			deltat -= m_CurrentKey->m_Interval;

			if (m_CurrentKey == m_LastKey) {

				wrapped = true;

				if (m_LoopAtEnd) {
					m_CurrentKey = m_FirstKey;
				} else {
					m_CurrentTime = 0;
					m_CurrentAlpha = 0;
					wrapped = true;
					return true;
				}

			} else {

				m_CurrentKey = m_CurrentKey->m_Right;
			}
		}

		gpassert(m_CurrentKey);

		if (m_CurrentKey) {
			m_CurrentTime = deltat;
			m_CurrentAlpha = m_CurrentTime*m_CurrentKey->m_InverseInterval;
		} else {
			return false;
		}

	} else {
	
		// Here we are actually going in reverse
		// TODO:: verify I have reverse time updates working properly -- biddle
		gpassertm(0,"Untested code branch");

		if (m_CurrentKey->m_Interval == INFINITE_TIME) {
			m_CurrentKey = m_CurrentKey->m_Left;
		}

		while (m_CurrentKey && (deltat > m_CurrentTime)) {

			deltat -= m_CurrentTime;

			if (m_CurrentKey == m_FirstKey) {

				if (m_LoopAtEnd) {
					m_CurrentKey = m_LastKey;
				} else {
					m_CurrentTime = 0;
					m_CurrentAlpha = 0;
					return true;
				}

			} else {

				m_CurrentKey = m_CurrentKey->m_Left;

			}

			m_CurrentTime = m_CurrentKey->m_Interval;

		}

		m_CurrentTime -= deltat;
		m_CurrentAlpha = m_CurrentTime*m_CurrentKey->m_InverseInterval;

	}

	if ((prevTime == m_CurrentTime) && (prevKey == m_CurrentKey)) {
		return false;
	}

	return true;

}

// ***************************************************
void
tKeySequence::
InsertKey(tKey *newkey) {

	gpassert(newkey->m_StartTime >= 0.0f);
	gpassert(newkey->m_StartTime < INFINITE_TIME);

	tKey *lhs = m_FirstKey;

	tKey *rhs = (lhs)?lhs->m_Right:NULL;
		
	// This is a linear search through the keys to find the correct insertion slot
	// if, at some point in the future we start to do a lot if key
	// insertions we should revisit this routine.
	// --biddle

	while (rhs && (newkey->m_StartTime >= (rhs->m_StartTime))) {

		lhs = rhs;
		rhs = rhs->m_Right;

	}

	if (rhs) {

		rhs->m_Left = newkey;
		lhs->m_Right = newkey;

	} else if (lhs) {

		lhs->m_Right = newkey;
		m_LastKey = newkey;

	} else {

		m_FirstKey = newkey;
		m_LastKey = newkey;

	}

	newkey->m_Left = lhs;
	newkey->m_Right = rhs;

	m_KeyCount++;

	m_ValidIntervals = false;

}

// ***************************************************
void
tKeySequence::
BuildIntervals(void) {


	if (!m_FirstKey) return;

	tKey *lhs = m_FirstKey;
	tKey *rhs = (lhs) ? lhs->m_Right : NULL;
		
	// This is a linear search through the keys to find the correct insertion slot
	// if, at some point in the future we start to do a lot if key
	// insertions we should revisit this routine.
	// --biddle

	while (rhs) {

		lhs->m_Interval = rhs->m_StartTime - lhs->m_StartTime;

		lhs->m_InverseInterval = 1.0f/lhs->m_Interval;

		lhs = rhs;
		rhs = rhs->m_Right;

	}

	if (lhs && m_LoopAtEnd && (m_Duration != INFINITE_TIME)) {
		lhs->m_Interval = m_Duration - lhs->m_StartTime;
		lhs->m_InverseInterval = 1.0f/lhs->m_Interval;
	} else {
		lhs->m_Interval = INFINITE_TIME;
		lhs->m_InverseInterval = 0;
	}

	m_CurrentKey = m_FirstKey;
	m_CurrentTime = 0;
	m_CurrentAlpha = 0;

	m_ValidIntervals = true;


}
