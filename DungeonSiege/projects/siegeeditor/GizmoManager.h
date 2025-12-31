//////////////////////////////////////////////////////////////////////////////
//
// File     :  GizmoManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __GIZMOMANAGER_H
#define __GIZMOMANAGER_H


// Include Files
#include "gpcore.h"
#include <map>
#include <vector>
#include <list>
#include "gpstring.h"
#include "siege_pos.h"
#include "nema_aspect.h"
#include "SiegeEditorTypes.h"
#include "GoDefs.h"


#define GIZMO_INVALID -1

enum eGizmoType
{
	GIZMO_POINTLIGHT,
	GIZMO_SPOTLIGHT,
	GIZMO_DIRLIGHT,
	GIZMO_OBJECTGIZMO,
	GIZMO_OBJECT,
	GIZMO_HOTPOINT,
	GIZMO_DIRHOTPOINT,	
	GIZMO_DECAL,
	GIZMO_STARTINGPOSITION,	
	GIZMO_TUNINGPOINT,
	GIZMO_METERPOINT,
};


struct Gizmo
{
	eGizmoType			type;
	SiegePos			spos;
	SiegePos			sdirection;
	DWORD				id;
	nema::HAspect		hAspect;
	bool				bSelected;
	bool				bVisible;
	bool				bNodeOverride;
	int					index;
};


struct GizmoModelInfo
{
	gpstring			sModel;
	gpstring			sTexture;
};


typedef std::vector< Gizmo >					GizmoVec;
typedef std::vector< Gizmo * >					GizmoPtrVec;
typedef std::map< eGizmoType, GizmoModelInfo >	GizmoModelInfoMap;
typedef std::pair< eGizmoType, GizmoModelInfo >	GizmoModelInfoPair;
typedef std::vector< DWORD >					GizmoIdVec;
typedef std::set< Gizmo * >						GizmoSet;
typedef std::map< gpstring, GizmoIdVec, istring_less >	GizmoGroupMap;



class GizmoManager : public Singleton <GizmoManager>
{
public:

	GizmoManager();
	~GizmoManager();

	void Deinit();

	// Hit Detection / Selection
	bool GizmoHitDetect( Gizmo & gizmo, bool bSelect = true );
	bool GizmoDragSelect( GizmoVec & gizmos, bool bSelect = true );
	void BeginDragSelect();	
	void EndDragSelect();
	void DeselectAll();
	int	 GetNumGizmosSelected();
	int	 GetNumObjectsSelected();
	void GetSelectedGizmoIDs			( std::vector< DWORD > & gizmo_ids );
	int  GetSelectedGizmoID				();
	void GetSelectedObjectIDs			( std::vector< DWORD > & object_ids );
	void GetDirectionalLightIDs			( std::vector< DWORD > & light_ids );
	void GetSelectedDirectionalLightIDs	( std::vector< DWORD > & light_ids );
	void GetSelectedLightIDs			( std::vector< DWORD > & light_ids );
	void GetGizmosOfType				( eGizmoType type, std::vector< DWORD > & gizmo_ids );
	void GetSelectedGizmosOfType		( eGizmoType type, std::vector< DWORD > & gizmo_ids );
	void ShowGizmosOfType				( eGizmoType type, bool bShow );
	bool IsCustomType					( eGizmoType type );
	bool IsRotationType					( eGizmoType type );

	void RemoveGizmosFromNode			( siege::database_guid nodeGuid );

	bool IsObjectSelected();
	bool IsLightSelected();
	bool DoesSelectionHaveInventory();
	bool DoesSelectionHaveEquipment();
	bool DoesSelectionHaveAspect();
	
	void SelectGizmo( DWORD object );
	void ForceSelectGizmo( DWORD object );
	void DeselectGizmo( DWORD object );

	void CursorRollover();
	void HitDetectLeadGizmo();		
	void SetLeadGizmo( Gizmo * pGizmo ) { m_pLeadGizmo = pGizmo;	}
	Gizmo * GetLeadGizmo()				{ return m_pLeadGizmo;		}
	Gizmo * GetGizmo( DWORD id );

	void SetRolloverGizmo( Gizmo * pGizmo ) { m_pRolloverGizmo = pGizmo;	}
	Gizmo * GetRolloverGizmo()				{ return m_pRolloverGizmo;		}

	void RotateSelection();

	void RemoveGizmo( DWORD id );
	void InsertGizmo( eGizmoType type, SiegePos spos, DWORD id, nema::HAspect * phAspect = 0 );	
	void InsertGizmo( Go * pGo );
	bool DeleteGizmo( DWORD id, bool bForce = false );
	void DeleteSelectedGizmos();
	void Clear();

	void Move( bool bShift = false, bool bControl = false );
	void PrecisionMove( PRECISION_DIRECTION direction );
	void MoveUpdate();
	bool AccurateAdjustPositionToTerrain( SiegePos & spos, bool bAdjustToAny = false );
	void CalculateMoveOffset( bool bShift = false );
	
	void DrawGizmos();
	void DrawGizmoBox( Gizmo & gizmo, vector_3 color );
	void Init();
	void Update();

	void Cut();
	void Copy();
	void Paste();
	void DragCopy();

	float	GetPrecisionAmount()							{ return m_precision_movement; }
	void	SetPrecisionAmount( float precision_movement )	{ m_precision_movement = precision_movement; }

	void	GetWorldSpaceOrientedBoundingVolume( Gizmo & gizmo, vector_3 & center, vector_3 & half_diag, matrix_3x3 & orient );
	
	bool	IsTypeVisible( eGizmoType type );
	bool	IsTypeEditable( eGizmoType type );

	void		SetOrientation( matrix_3x3 orientation );
	void		SetOrientation( Quat orientation );
	matrix_3x3	GetOrientation();
	Quat		GetQuatOrientation();

	void		SetOrientation( DWORD id, Quat orient );
	Quat		GetOrientation( DWORD id );

	void		SetPosition( float x, float y, float z ); 
	void		SetPosition( SiegePos spos );
	void		SetPosition( int id, SiegePos spos );
	vector_3	GetPosition();
	void		GetPosition( SiegePos & spos );

	void		SetBodyScale( float scale );
	float		GetBodyScale();

	// Retrieves an object with a specified inventory
	Goid GetFirstObjectWithInventory();	
	
	void LoadLights();
	void LoadDecals();

	int  GetLastSelectedGizmoID()			{ return m_last_selected; }
	void SetLastSelectedGizmoID( int id	)	{ m_last_selected = id; }
	bool IsLastSelectedHit();

	void ShowAllObjects();
	void HideSelectedObjects();
	void ShowSelectedObjects();

	void ShowGizmo( DWORD id, bool bShow );

	// Grouping related functions
	void GetGizmoGroupNames( StringVec & groups );
	void SelectGroupings( StringVec groups );
	void DeselectGroupings( StringVec groups );
	void ShowGroupings( StringVec groups );
	void HideGroupings( StringVec groups );
	void RandomRotateSelection();
	void RandomScaleSelection( float min, float max );
	bool NewGroup( gpstring sGroup );
	void RemoveGroups( StringVec groups );
	void AddSelectionToGroup( StringVec groups );
	void RotateSelected( int mouseDeltaX );
	void SetMouseRotatePosition( SiegePos spos ) { m_mouseRotatePos = spos; }
	SiegePos GetMouseRotatePosition() { return m_mouseRotatePos; }

	void LoadGroups();
	void SaveGroups();

	void SetShowObjects( bool bShow );

	void SetStartingPosition( int id, SiegePos spos );

private:

	// Private Functions
	void DrawGizmo( Gizmo & gizmo );	
	void DrawObjectBox( vector_3 c, vector_3 d, vector_3 colorv, Rapi & renderer );

	// Private Member Variables
	GizmoVec			m_gizmos;
	GizmoPtrVec			m_selection_frustrum_gizmos;	// This is not a list of the selected gizmos, just a list of gizmos in the current
														// selection frustrum
	GizmoModelInfoMap	m_gizmo_model_info;
	unsigned int		m_godb_size;
	Gizmo *				m_pLeadGizmo;
	Gizmo *				m_pRolloverGizmo;
	float				m_precision_movement;	
	int					m_last_selected;
	float				m_xMoveOffset;
	float				m_yMoveOffset;
	float				m_zMoveOffset;
	GizmoSet			m_hitSet;
	int					m_selectionIndex;
	GizmoGroupMap		m_gizmoGroups;
	SiegePos			m_mouseRotatePos;
	
	int					m_startPosId;
	SiegePos			m_startPos;
	bool				m_bSnapStartPos;

};


#define gGizmoManager GizmoManager::GetSingleton()


#endif