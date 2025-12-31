#pragma once
#ifndef _SIEGE_MESH_DOOR_
#define _SIEGE_MESH_DOOR_

/* ========================================================================
   Header File: siege_mesh_door.h
   Declares: 
   ======================================================================== */

#include "matrix_3x3.h"
#include "vector_3.h"

#include <list>
#include <vector>

namespace siege
{
	class SiegeMeshDoor
	{
	private:

		// Don't let anyone else copy
		SiegeMeshDoor(SiegeMeshDoor const &);
		SiegeMeshDoor &operator=(SiegeMeshDoor const &);
		
	protected:

		// Identifier
		DWORD				m_Id;

		vector_3			m_Center;
		matrix_3x3			m_Orientation;

#if !GP_RETAIL

		// Verts (indices to owner mesh)
		std::vector<int>	m_Verts;

#endif

	public:

#if !GP_RETAIL
		
		SiegeMeshDoor(const UINT32& init_id, 
					  const vector_3& init_center,
					  const matrix_3x3& init_orient,
					  const std::vector<int>& init_verts
					 ) 
			: m_Id(init_id)
			, m_Center(init_center)
			, m_Orientation(init_orient)
			, m_Verts(init_verts)
		{};

#else

		SiegeMeshDoor(const UINT32& init_id, 
					  const vector_3& init_center,
					  const matrix_3x3& init_orient
					 ) 
			: m_Id(init_id)
			, m_Center(init_center)
			, m_Orientation(init_orient)
		{};

#endif

		SiegeMeshDoor();

		// Accessors
		DWORD				GetID()					{ return m_Id; }

		const vector_3&		GetCenter()				{ return m_Center; }
		const matrix_3x3&	GetOrientation()		{ return m_Orientation; }

#if !GP_RETAIL

		std::vector<int>&	GetVertexIndices()	{ return m_Verts; }

#endif
	};
}

#endif
