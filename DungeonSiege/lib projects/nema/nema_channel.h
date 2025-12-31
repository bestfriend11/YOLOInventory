#error, "no longer in NEMA"
#pragma once
#ifndef NEMA_CHANNEL_H
#define NEMA_CHANNEL_H

/*========================================================================

  Channel, part of the "NeMa" system

      
  author: Mike Biddlecombe
  date: 9/13/1999
  (c)opyright 1999, Gas Powered Games

  
	TODO: 
		inline as much as possible

------------------------------------------------------------------------*/

#include "nema_types.h"
#include "nema_aspect.h"
#include "vector_3.h"
#include "quat.h"

/*============================================================================

----------------------------------------------------------------------------*/

namespace nema {


	class tChannel {

		friend bool Aspect::temporaryTransferInternals(HAspect dest);

	public:

		tChannel(nema::tController* initctrl);
		tChannel(/*Handle to a skrit*/);

		~tChannel(void);

		const vector_3&	GetCurrentPosition(void) { return m_CurrPosition; }
		const Quat&	GetCurrentRotation(void) { return m_CurrRotation; }
		const vector_3&	GetCurrentScale(void)	 { return m_CurrScale;    }

		void SetCurrentPosition(const vector_3& newval)	{ m_CurrPosition = newval; }
		void SetCurrentRotation(const Quat& newval)	{ m_CurrRotation = newval; }
		void SetCurrentScale(const vector_3& newval)	{ m_CurrScale    = newval; }

		void SetOffsetPosition(const vector_3& newval)	{ m_OffsetPosition = newval;}
		void SetOffsetRotation(const Quat& newval)		{ m_OffsetRotation = newval;}

		const vector_3&		GetOffsetPosition(void) const {return m_OffsetPosition;}
		const Quat&		GetOffsetRotation(void) const {return m_OffsetRotation;}


		tChannel* GetParent(void) const;
		void SetParent(tChannel* const newval);

		tController* GetController(void) const;
		void SetController(tController* newval);

		// 'Active' parents & controllers can be manipulated on the fly
		//
		// TODO we have problem if a linked parent gets deleted
		//
		// TODO Would it be better to just implement a stack to handle
		//		'active' stuff?

		tChannel* GetActiveParent(void) const;
		void SetActiveParent(tChannel* const newval);
		void ResetActiveParent(void);

		tController* GetActiveController(void) const;
		void SetActiveController(tController* const newval);
		void ResetActiveController(void);

///2blend		void SetStoredControllerKey(tKey* newval)	{ m_StoredControllerKey = newval;  }
///2blend		tKey* GetStoredControllerKey(void)			{ return m_StoredControllerKey;    }

///2blend		void SetStoredControllerTime(float newval)	{ m_StoredControllerTime = newval; }
///2blend		float GetStoredControllerTime(void)			{ return m_StoredControllerTime;   }

/*
		void SetStoredData0(DWORD newval)	{ m_StoredData0 = newval;  }
		DWORD GetStoredData0(void)			{ return m_StoredData0;    }

		void SetStoredData1(DWORD newval)	{ m_StoredData1 = newval;  }
		DWORD GetStoredData1(void)			{ return m_StoredData1;    }
*/

		bool Update(float deltat, sBone&);

		// probably need to be able to 'lookahead' on the channel
		// by this i mean the ability to evaluate the channel at arbitrary time

		// TODO:: The inclusion of any callbacks in the tChannel class may turn out to be a 'bad thing'
		// I want to make sure that any info that is better located in the choreographer model 
		// gets stored there rather than here in the aspect -- biddle

		// TODO:: what other types of callbacks are there? PRS threshold? time threshold?

		void RegisterCompletionCallback(EventCallback const &cb) {
												m_CompletionCallback = cb;
										}
		
		void ResetCompletionCallback(void);	
		void CallCompletionCallback(void);	


	private:

		tController*	m_Controller;
		tController*	m_ActiveController;

		// Copies of the pos/rot/scale stored in the 
		vector_3		m_CurrPosition;
		Quat			m_CurrRotation;
		vector_3		m_CurrScale;

		// TODO:: add these offsets to get the arrow to work in MS 10 -- biddle
		vector_3		m_OffsetPosition;
		Quat			m_OffsetRotation;
/*
		const vector_3	m_RestPosition;
		const Quat		m_RestRotation;
		const vector_3	m_RestScale;

		const vector_3	m_InvRestPosition;
		const Quat		m_InvRestRotation;
		const vector_3	m_InvRestScale;
*/
		tChannel*		m_Parent;
		tChannel*		m_ActiveParent;
/*
		DWORD			m_StoredData0;	// Some channels may need to keep
		DWORD			m_StoredData1;	// extra info for their controller
*/
		// Callbacks available on a per channel basis..
		// TODO:: Do I need to support multiple callbacks on a given event/trigger? 
		EventCallback	m_CompletionCallback;

	};

};


#endif