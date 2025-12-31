#error	"Not using this in NEMA anymore"
#pragma once
#ifndef NEMA_KEYS_H
#define NEMA_KEYS_H

/*========================================================================

  Keys, part of the "NeMa" system

      
  author: Mike Biddlecombe
  date: 10/11/1999
  (c)opyright 1999, Gas Powered Games

  -----

  todo:


------------------------------------------------------------------------*/

#include <list>

#include "nema_types.h"
#include "vector_3.h"
#include "quat.h"

namespace nema {

	const float INFINITE_TIME = (RealMaximum*0.49f);	// Allows us to have INF+INF without overflow

	class tKey {

	public:

		inline tKey(
			float initTime,
			const vector_3& initPosition,
			const Quat& initRotation);

		inline	~tKey(void);

		float		m_StartTime;
		float		m_Interval;
		float		m_InverseInterval;

		vector_3	m_Position;
		Quat		m_Rotation;
		vector_3	m_Scale;
		tKey		*m_Left;
		tKey		*m_Right;

	};


	class tKeySequence {

	private:

		int		m_KeyCount;

		tKey*	m_FirstKey;
		tKey*	m_LastKey;

		tKey*	m_CurrentKey;
		float	m_CurrentTime;
		float	m_CurrentAlpha;

		float	m_Duration;
		bool	m_LoopAtEnd;

		bool	m_ValidIntervals;

	public:

		tKeySequence(bool looping)
			: m_KeyCount(0)
			, m_FirstKey(NULL)
			, m_LastKey(NULL)
			, m_CurrentKey(NULL)
			, m_CurrentTime(0)
			, m_CurrentAlpha(0.0f)
			, m_Duration(INFINITE_TIME)
			, m_LoopAtEnd(looping)
			, m_ValidIntervals(false)
		{};

		inline ~tKeySequence(void);

		inline void SetSequenceDuration(float newtime)	{ m_Duration = newtime;}
		inline float GetSequenceDuration(void)			{ return m_Duration;	}

		tKey*	GetFirstKey()		{ return m_FirstKey; }
		tKey*	GetLastKey()		{ return m_LastKey; }

		bool	GetCurrentBracket(float &alpha, nema::tKey* &lhs, nema::tKey* &rhs);

		bool	UpdateCurrentKey(float deltat, tKey* prevKey, float prevTime, bool &wrapped);

		tKey*	GetCurrKey(void)	{ return m_CurrentKey;  }
		float	GetCurrTime(void)	{ return m_CurrentTime; }
		float	GetCurrAlpha(void)	{ return m_CurrentAlpha; }

		void	InsertKey(tKey *newkey);
		void	BuildIntervals(void);



	};

}

//***********************************************************************************
inline nema::tKey::tKey(float initTime, const vector_3& initPosition, const Quat& initRotation)
	: m_StartTime(initTime)
	, m_Interval(INFINITE_TIME)
	, m_InverseInterval(0)
	, m_Position(initPosition)
	, m_Rotation(initRotation)
	, m_Scale(vector_3(1,1,1))
	, m_Right(NULL)
	, m_Left(NULL) 
{
}

//***********************************************************************************
inline nema::tKey::~tKey(void)
{

	if (m_Left) {

		m_Left->m_Right = m_Right;

		if (this->m_Interval == INFINITE_TIME) {
			m_Left->m_Interval = INFINITE_TIME;
			m_Left->m_InverseInterval = 0;
		} else {
			m_Left->m_Interval += this->m_Interval;
			m_Left->m_InverseInterval = 1.0f/m_Left->m_Interval;
		}

		if (m_Right) {

			m_Right->m_Left = m_Left;

		} else {

			m_Left = NULL;

		}


	} else {


	}

		
}

// ***************************************************
inline
nema::tKeySequence::
~tKeySequence(void) {

	nema::tKey *eraser = m_LastKey;

	while (eraser && eraser->m_Left) {
		eraser = eraser->m_Left;
		delete eraser->m_Right;
	}

	if (eraser) {
		delete eraser;
	}

	m_CurrentKey = NULL;
	m_CurrentTime = 0.0f;
	m_CurrentAlpha = 0.0f;

}


#endif


