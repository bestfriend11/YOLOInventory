#pragma once
#ifndef _SIEGE_MOUSE_SHADOW_
#define _SIEGE_MOUSE_SHADOW_

//**********************************************************************
//
// Mouse Shadow
//
//**********************************************************************

#include "siege_defs.h"
#include "siege_database_guid.h"
#include "vector_3.h"


namespace siege
{
	class SiegeMouseShadow
	{
	public:

		SiegeMouseShadow(void);
		~SiegeMouseShadow(void);

		void Init();
		void Shutdown();

		typedef std::list<DWORD> HitColl;

		// Update the mouse
		void Update( const float norm_cursor_x, const float norm_cursor_y );

		// Camera
		void					SetSelectionBox( bool flag )			{ m_bSelectionBox = flag; }
		bool					SelectionBox() const					{ return m_bSelectionBox; }
		vector_3				CameraSpacePosition() const				{ return m_CameraSpacePosition; }
		vector_3				ScreenPosition() const					{ return m_ScreenPos; }

		vector_3				CameraSpaceBeginPosition() const		{ return m_SelectBoxBegin; }
		vector_3				ScreenBeginPosition() const				{ return m_SelectBoxScreenBegin; }
		void					SetBeginPosition( const float norm_x, const float norm_y );

		// Frustum
		const SiegeViewFrustum&	SelectFrustum() const					{ return *m_pSelectFrustum; }
		SiegeViewFrustum&		SelectFrustum()							{ return *m_pSelectFrustum; }
		bool					UseSelectFrustum()						{ return m_bSelectFrustum; }

		// Terrain
		bool					IsTerrainHit() const					{ return (m_FloorMinDist < RealMaximum); }
		real					GetTerrainHitDistance() const			{ return m_FloorMinDist; }

		SiegePos const &		GetFloorHitPos()						{ return m_FloorHitPos; }
		void					SetFloorHitPos( SiegePos const & pos )	{ m_FloorHitPos = pos; }

		void					SetFloorHitDist( float t )				{ m_FloorMinDist = t; }

		// hit list
		bool					IsHit() const							{ return (m_MinDist < RealMaximum); }
		DWORD					GetHit() const							{ return m_Hit; }
		real					GetHitDistance() const					{ return m_MinDist; }
		const vector_3&			GetHitLocation() const					{ return m_HitLocation; }

		const HitColl&			GetHitColl() const						{ return m_HitColl; }
		const HitColl&			GetGlobalHitColl() const				{ return m_GlobalHitColl; }

		void					PushGeneralHit( DWORD g, const vector_3& pos, const float dist );
		void					PushAccurateHit( DWORD g, const vector_3& pos, const float dist );

	
	private:

		void					InitVariables();

		// Selection information
		vector_3				m_SelectBoxBegin;
		vector_3				m_SelectBoxScreenBegin;
		bool					m_bSelectionBox;
		bool					m_bMultiSelectEnabled;
		bool					m_bSelectFrustum;
		
		// Current camera space position of the mouse
		vector_3				m_CameraSpacePosition;
		vector_3				m_ScreenPos;

		// Object hits
		DWORD					m_Hit;
		bool					m_bAccurate;
		real					m_MinDist;
		HitColl					m_HitColl;
		HitColl					m_GlobalHitColl;
		vector_3				m_HitLocation;

		// Terrain hits
		SiegePos				m_FloorHitPos;
		real					m_FloorMinDist;

		// Select frustum
		SiegeViewFrustum*		m_pSelectFrustum;
	};
}


#endif
