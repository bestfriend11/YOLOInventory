#error, "no longer in NEMA"
#pragma once
#ifndef NEMA_CONTROLLER_H
#define NEMA_CONTROLLER_H

/*========================================================================

  Controller, part of the "NeMa" system

      
  author: Mike Biddlecombe
  date: 9/13/1999
  (c)opyright 1999, Gas Powered Games

  -----

  todo:


------------------------------------------------------------------------*/


#include "nema_types.h"
#include "vector_3.h"
#include "quat.h"


#include <list>

namespace nema {

	enum ControllerChunkIDs {

		IDAnimHeader			= 0x4D494E41,	// ANIM
		IDAnimKeyList			= 0x54534C4B,	// KLST
		IDAnimEndMarker			= 0x444E4541	// AEND

	};

	struct sNeMaAnim_Chunk {

		DWORD ChunkName;					// "ANIM"
		DWORD ChunkVersion;			
		DWORD StringTableSize;
		DWORD NumberOfKeyLists;
		float Duration;
		float DeltaX;
		float DeltaY;
		float DeltaZ;
		DWORD LoopAtEnd;
	};

	//** KEY LISTS ***********************************************

	struct sAnimKeyList_Chunk {
		DWORD	ChunkName;					// "KLST"
		DWORD	ChunkVersion;			
		DWORD	BoneIndex;		
		DWORD	BoneNameOffset;
		DWORD	NumberOfKeys;
	};

	//** KEY DATA (***********************************************

	struct sAnimKey_Chunk {
		// DWORD	ParentKeyIndex;		// TODO: add support for an animated parent relationship
		float	Time;
		float	RotX;
		float	RotY;
		float	RotZ;
		float	RotW;
		float	PosX;
		float	PosY;
		float	PosZ;
	};


	//************************************************************
	//************************************************************
	//************************************************************

	class tController {

	public:

//		virtual	tController() {};
		virtual ~tController() {};
		virtual bool UpdateChannel(tChannel* chan,float deltat, sBone& bone) = 0;
		virtual bool AttachToChannel(tChannel* chan, sBone& bone) = 0;

	};

	//---------------------------------

	class tControllerArray {

	public:

		tControllerArray(gpstring name, int num);

		~tControllerArray(void);

		tController* GetController(int i) const;
		void SetController(int i, tController* c);

		gpstring GetName(void) { return m_Name; }

		int GetNumControllers(void) { return m_NumControllers; }

		const vector_3& GetVelocity(void)		{ return m_velocity;	}
		void SetVelocity(const vector_3& v)		{ m_velocity = v;		}

		const vector_3& GetDuration(void)		{ return m_duration;	}
		void SetDuration(const vector_3& d)		{ m_duration = d;		}


	private:
		gpstring m_Name;
		int m_NumControllers;
		tController** m_Controllers;

		// TODO:: is this the best way to track the velocity?
		// Should every controller have a velocity/duration (probably...)
		// How do single controllers support velocity?
		// Should the spin controllers have 'angular velocity' rather than RPM?

		vector_3	m_velocity;
		vector_3	m_duration;

	};

	//---------------------------------

	class tControllerStorage : Singleton <tControllerStorage> {

		// The ArraysList is the original controller storage class
		// it is used to store arrays of PRS controllers. 
		typedef std::list<tControllerArray*>::iterator iter;
		std::list<tControllerArray*> m_ArraysList;

		// The singleton list is used to store single controllers,
		// these are typically unique special purpose controllers
		typedef std::list<tController*>::iterator singleiter;
		std::list<tController*> m_SingletonList;

		// Blend controllers are created on the fy to support blending of
		// other types of controllers. They are in their own list as I expect 
		// create and destroy them on the fly. Also might want to
		// differentiate a Blender from a Controller
		std::list<tControllerArray*> m_BlenderList;

	public:

		 ~tControllerStorage(void);
		tControllerArray* LoadControllerArray(const char* arrayname, const char* fname, bool loopatend=true);

		tControllerArray* FindControllerArray(const char* arrayname);

		// Trying to come up with way to keep track of single instance controllers -- biddle
		tController* CreateSpinController(const char* ctrlname, float rpm);
		tController* CreateTranslationController(const char* ctrlname, const vector_3& pA,const vector_3& pB, float cps);

		tControllerArray* CreateBlendingArray(const char* arrayname, tControllerArray* A, tControllerArray* B);

	};

	#define gControllerStorage nema::tControllerStorage::GetSingleton()

	//---------------------------------
	//---------------------------------
	//---------------------------------

	class tPRSController : public tController {

	public:

		explicit tPRSController(bool LoopAtEnd = true);
		~tPRSController(void);
		
		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

		tKeySequence* GetKeySequence(void) { return m_KeySequence; }

		bool GetLoopAtEnd(void)			{ return m_LoopAtEnd; }
		void SetLoopAtEnd(bool flag)	{ m_LoopAtEnd = flag; }

	private:

		// Not all controllers will have key sequences
		tKeySequence*	m_KeySequence;

		bool m_LoopAtEnd;

		vector_3		m_CurrPosition;
		Quat			m_CurrRotation;
		vector_3		m_CurrScale;

	};

	// A rigid link just gets info from its parent
	class tRigidLinkController : public tController {

	public:

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

	};

	// A relative link applies a relative offset to the parent
	class tRelativeLinkController : public tController {

	public:

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

	};

	// An absolute controller ignores any parent and uses the RestRotation
	class tAbsoluteLinkController : public tController {

	public:

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

	};

	// "locked" is a fancy way of saying "do nothing"
	class tLockedController : public tController {

	public:

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

	};

	// TODO:: I am not happy with the spin and translate controllers
	// storing data in the controller is the wrong way to do it
	// Going to have to find way to dynamically add channels to an aspect
	// --biddle

	// A rotation controller
	class tSpinController : public tController {

	public:

		tSpinController(real rpm=0.0f) : m_spinrate(rpm) {};

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

		void SetSpinRate(float rpm) { m_spinrate = rpm;  }
		float GetSpinRate(void)		{ return m_spinrate; }

	private:
		float m_spinrate;
	};

	// A translation controller
	class tTranslationController : public tController {

	public:

		tTranslationController(const vector_3& pA,const vector_3& pB, float dur)
			: m_direction(FORWARD)
			, m_cycletime(dur)
			, m_pointA(pA)
			, m_pointB(pB)
			, m_tval(0.0f)
		  {};

		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

		void ReverseDirection(bool) { m_direction = (m_direction==BACKWARD) ? FORWARD:BACKWARD; }

	private:
		enum { FORWARD = 1, BACKWARD = -1}  m_direction;
		float m_cycletime;
		vector_3 m_pointA;
		vector_3 m_pointB;
		float m_tval;

	};

#if BLENDING_NEEDS_TO_BE_CONVERTED
	class tBlendController : public tController {

	public:

		explicit tBlendController(tController* A, tController* B);
		
		bool UpdateChannel(tChannel* chan, float deltat, sBone& bone);
		bool AttachToChannel(tChannel* chan, sBone& bone);

	private:

		tController* m_controllerA;
		tController* m_controllerB;

		float m_weight;		// normalized value: 0.0 = 100% CtrlA, 1.0 = 100% CtrlB

		DWORD m_dataA0;		// StoredData for CtrlA
		DWORD m_dataA1;

		DWORD m_dataB0;		// StoredData for CtrlB
		DWORD m_dataB1;

	};

#endif

};

// TODO -- move these generics into the singleton list
extern nema::tRigidLinkController GenericRigidLinkController;
extern nema::tRelativeLinkController GenericRelativeLinkController;
extern nema::tAbsoluteLinkController GenericAbsoluteLinkController;
extern nema::tLockedController GenericLockedController;


#endif