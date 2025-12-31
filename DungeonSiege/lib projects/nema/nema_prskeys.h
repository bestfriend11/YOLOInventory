//////////////////////////////////////////////////////////////////////////////
//
// File     :  Nema_PRSKeys.h
// Author(s):  Mike Biddlecombe
//
// Summary  : Definition of Position Rotation Scale keys
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef NEMA_PRSKEY
#define NEMA_PRSKEY

#include "vector_3.h"
#include "quat.h"
#include "Point2.h"
#include "Space_2.h"

// Include Rapi stuff so we can build weapon tracer mesh
#include "RapiOwner.h"


#include <map>

#include "nema_keymgr.h"
#include "nema_kevents.h"

namespace nema {

/*	struct PRS {
		Quat		rot;
		vector_3	pos;
		// There isn't a scale... Yet! -- biddle
	};
*/
	typedef stdx::linear_map<float, Quat>		RotMap;
	typedef RotMap::iterator					RotMapIter;
	typedef RotMap::reverse_iterator			RotMapRevIter;
	typedef std::pair<RotMapIter, bool>			RotMapRet;

	inline float		TIME_VAL(RotMapIter i)			{ return (*i).first;	}
	inline const Quat&	KEY_VAL(RotMapIter i)			{ return (*i).second;	}
	inline const Quat&	KEY_VAL(RotMapRevIter i)		{ return (*i).second;	}

	typedef stdx::linear_map<float, vector_3>	PosMap;
	typedef PosMap::iterator					PosMapIter;
	typedef PosMap::reverse_iterator			PosMapRevIter;
	typedef std::pair<PosMapIter, bool>			PosMapRet;

	inline float			TIME_VAL(PosMapIter i)		{ return (*i).first;	}
	inline const vector_3&	KEY_VAL(PosMapIter i)		{ return (*i).second;	}
	inline const vector_3&	KEY_VAL(PosMapRevIter i)	{ return (*i).second;	}

	struct PRSKeyTrackData {
		RotMap		m_RotData;
		PosMap		m_PosData;
	};

	typedef stdx::fast_vector<PRSKeyTrackData>	PRSKeyTrackDataArray;
	typedef PRSKeyTrackDataArray::iterator		PRSKeyTrackDataArrayIter;

	struct PRSKeyTrack {

		PRSKeyTrackData *m_pData;


		vector_3		m_CurrPosition;
		Quat			m_CurrRotation;

	};

	typedef stdx::fast_vector<PRSKeyTrack>	PRSKeyTrackArray;
	typedef PRSKeyTrackArray::iterator		PRSKeyTrackArrayIter;

	////////////////////////////////////////////////////////////////////////////

	class TracerKeys 
	{
	public:

		TracerKeys(float s, float f, DWORD n)
			: m_StartKeyTime(s)
			, m_FinishKeyTime(f)
			{
				m_Positions.reserve(n);
				m_Rotations.reserve(n);
			};

		bool BuildTracerTriStrip(
			float time,
			const vector_3& in_p0,
			const vector_3& in_p1,
			stdx::fast_vector<sVertex>& out_points
			) const;

		bool BuildTracerTriMesh(
			float time,
			const vector_3& in_p0,
			const vector_3& in_p1,
			stdx::fast_vector<sVertex>& out_points
			) const;

		float	m_StartKeyTime;
		float	m_FinishKeyTime;

		stdx::fast_vector<matrix_3x3> m_Rotations;
		stdx::fast_vector<vector_3>   m_Positions;
	};

	////////////////////////////////////////////////////////////////////////////

	void FirstEventCorrectionHelper(RotMap& r,PosMap& p);
	void FirstEventCorrectionHelper(RotMap& m);
	void FirstEventCorrectionHelper(PosMap& m);

	////////////////////////////////////////////////////////////////////////////
	class PRSKeysImpl {

	public:

		PRSKeysImpl( const_mem_ptr mem );
		~PRSKeysImpl(void);


		int AddRef ( void )  {  gpassert( (m_RefCount >= 0) && (m_RefCount < 1000) );  return ( ++m_RefCount );  }		// sanity checks
		int GetRef ( void )  {  gpassert( (m_RefCount >= 0) && (m_RefCount < 1000) );  return (   m_RefCount );  }		// sanity checks
		int Release( void )  {  gpassert( (m_RefCount >  0) && (m_RefCount < 1000) );  return ( --m_RefCount );  }		// sanity checks

		DWORD					m_MajorVersion;
		DWORD					m_MinorVersion;

		float					m_Duration;
		float					m_Frequency;
		vector_3				m_LinearVelocity;			// Displacement/Duration
		float					m_ScalarVelocity;			// LinearDisplacement/Duration

		PRSKeyTrackDataArray	m_TrackData;

		KEventMap				m_CriticalEvents;

		bool					m_LoopAtEnd;

		TracerKeys*				m_pTracer;

		OBB2					m_OBB;

	private:

		DWORD		m_RefCount;

	};

	////////////////////////////////////////////////////////////////////////////

	class PRSKeys {

	public:

		PRSKeys(const_mem_ptr mem);			// Create an original
		PRSKeys(PRSKeysImpl* pimpl);		// Create a copy, sharing pimpl
		PRSKeys(PRSKeys* original);			// Clone an entire existing (shared pimpl too)

		~PRSKeys(void);

		PRSKeysImpl* GetImpl()					{  return ( m_pImpl );  }

		FUBI_EXPORT const char* GetDebugName( void ) const				{  return ( m_dbgname.c_str() );  }
		FUBI_EXPORT const gpstring& GetDebugNameString( void ) const	{  return ( m_dbgname );  }

		void SetDebugName( const gpstring& n )		{   m_dbgname = n;  }

		FUBI_EXPORT bool UpdateAbsolute(float t, float min_t); 

		bool RotUpdateHelper(float target_time,	RotMap& rkeys,	Quat& outrot    , bool loopatend);
		bool PosUpdateHelper(float target_time,	PosMap& pkeys,	vector_3& outpos, bool loopatend);

		// FUBI Accessors
		FUBI_EXPORT int GetNumTracks(void) const  { return m_Tracks.size(); }

		FUBI_EXPORT const bool GetLoopAtEnd( void ) const	{  return ( m_pImpl->m_LoopAtEnd );  }

		const float& GetBlendWeight(void) const {
													return m_CurrentBlendWeight;
												}

		void SetBlendWeight(const float f)		{
													m_CurrentBlendWeight = f;
												}

		FUBI_EXPORT float GetCurrentNormalizedTime(void) const	{
			return m_CurrentTime;
		}

		FUBI_EXPORT float GetDuration(void) const  { return m_pImpl->m_Duration;	}

		FUBI_EXPORT const vector_3& GetVelocity(void) const { return m_pImpl->m_LinearVelocity; }

		FUBI_EXPORT float GetVelocityX(void) const { return m_pImpl->m_LinearVelocity.x; }
		FUBI_EXPORT float GetVelocityY(void) const { return m_pImpl->m_LinearVelocity.y; }
		FUBI_EXPORT float GetVelocityZ(void) const { return m_pImpl->m_LinearVelocity.z; }

		FUBI_EXPORT float GetScalarVelocity(void) const { return m_pImpl->m_ScalarVelocity; }

		const vector_3& GetCurrPosition(int t) const	{
			const vector_3& tpos = m_Tracks[t].m_CurrPosition;
			return tpos;
		}

		FUBI_EXPORT float GetCurrPositionX(int t) const	{ return m_Tracks[t].m_CurrPosition.x; }
		FUBI_EXPORT float GetCurrPositionY(int t) const	{ return m_Tracks[t].m_CurrPosition.y; }
		FUBI_EXPORT float GetCurrPositionZ(int t) const	{ return m_Tracks[t].m_CurrPosition.z; }

		const Quat& GetCurrRotation(int t) const		{ return m_Tracks[t].m_CurrRotation; }

		FUBI_EXPORT float GetCurrRotationW(int t) const	{ return m_Tracks[t].m_CurrRotation.W(); }
		FUBI_EXPORT float GetCurrRotationX(int t) const	{ return m_Tracks[t].m_CurrRotation.X(); }
		FUBI_EXPORT float GetCurrRotationY(int t) const	{ return m_Tracks[t].m_CurrRotation.Y(); }
		FUBI_EXPORT float GetCurrRotationZ(int t) const	{ return m_Tracks[t].m_CurrRotation.Z(); }

		FUBI_MEMBER_DOC	( GetNumTracks, "", "Returns the number of bones (channels) in the animation" )

		FUBI_MEMBER_DOC	( GetDuration, "", "Returns the duration of the animation in seconds" )
		FUBI_MEMBER_DOC	( GetVelocityX, "", "Returns the X velocity of the animation in m/s" )
		FUBI_MEMBER_DOC	( GetVelocityY, "", "Returns the Y velocity of the animation in m/s" )
		FUBI_MEMBER_DOC	( GetVelocityZ, "", "Returns the Z velocity of the animation in m/s" )

		// Let the loader access the internals directly
		friend HPRSKeys nema::PRSKeysStorage::LoadPRSKeys(const char* const fname,const char* const dbgname);

		KEventMap& GetCriticalEvents(void) const {return m_pImpl->m_CriticalEvents;}

		bool  HasTracerKeys() const                 { return m_pImpl->m_pTracer != NULL;	}
		const TracerKeys*  GetTracerKeys() const    { return m_pImpl->m_pTracer;			}

		DWORD GetMajorVersion() const				{ return m_pImpl->m_MajorVersion;		}
		DWORD GetMinorVersion() const				{ return m_pImpl->m_MinorVersion;		}

	private:

		PRSKeysImpl			*m_pImpl;

		PRSKeyTrackArray	m_Tracks;

		float				m_CurrentTime;
		float				m_CurrentBlendWeight;

		gpstring	m_dbgname;			// Name printed for debugging

		SET_NO_COPYING( PRSKeys );

		FUBI_MANAGED_CLASS( PRSKeys, "Nema PRSKeys class" );

	};

};

#endif // NEMA_PRSKEY
