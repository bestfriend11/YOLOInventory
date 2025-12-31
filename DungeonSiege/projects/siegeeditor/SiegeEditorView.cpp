//////////////////////////////////////////////////////////////////////////////
//
//	File     		:  SiegeEditorView.cpp
//
// 	Programmed by	:  Chad Queen
//
//	Copyright (c) 2002 Gas Powered Games Corp. All rights reserved. Gas
//	Powered Games, Dungeon Siege and Siege Editor are the exclusive trademarks of 
//	Gas Powered Games Corp.
//
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "EditorCamera.h"
#include "services.h"
#include "rapiowner.h"
#include "SiegeEditor.h"
#include "EditorTerrain.h"
#include "Services.h"
#include "EditorRegion.h"
#include "Siege_Cache.h"
#include "Preferences.h"
#include "GizmoManager.h"
#include "EditorObjects.h"
#include "MenuCommands.h"
#include "EditorLights.h"
#include "EditorTriggers.h"
#include "CommandActions.h"
#include "EditorGizmos.h"
#include "EditorHotpoints.h"
#include "EditorTuning.h"
#include "Siege_Camera.h"
#include "Dialog_Portraits.h"
#include "SiegeEditorDoc.h"
#include "SiegeEditorView.h"
#include "LeftView.h"
#include "BottomView.h"
#include "MainFrm.h"
#include "Dialog_Object_Grouping.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorView

IMPLEMENT_DYNCREATE(CSiegeEditorView, CView)

BEGIN_MESSAGE_MAP(CSiegeEditorView, CView)
	//{{AFX_MSG_MAP(CSiegeEditorView)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_OBJECT_PROPERTIES, MenuFunctions::ObjProperties)
	ON_COMMAND(ID_ADDGOTOCUSTOMVIEW, MenuFunctions::AddGOToCustomView)	
	ON_COMMAND(ID_EDIT_CUT,MenuFunctions::Cut)
	ON_COMMAND(ID_EDIT_COPY,MenuFunctions::Copy)
	ON_COMMAND(ID_EDIT_PASTE,MenuFunctions::Paste)	
	ON_COMMAND(ID_OBJECT_HIDE,MenuFunctions::HideObject)
	ON_COMMAND(ID_OBJECT_TRIGGERS_EXECUTE,MenuFunctions::ExecuteTriggers)
	ON_COMMAND(ID_OBJECT_TRIGGERS_RESET,MenuFunctions::ResetTriggers)
	ON_COMMAND(ID_LINK_MODE,MenuFunctions::LinkMode)	
	ON_COMMAND(ID_NODE_PROPERTIES,MenuFunctions::NodeProperties)	
	ON_COMMAND(ID_STARTINGPOS_PROPERTIES,MenuFunctions::StartingPosProperties)
	ON_COMMAND(ID_STARTINGPOS_CAMERAPREVIEW,MenuFunctions::StartingCameraPreview)
	ON_COMMAND(ID_STARTINGPOS_CAMERASET,MenuFunctions::SetCameraToCurrentPosition)
	ON_COMMAND(ID_HOTPOINT_INFORMATION,MenuFunctions::HotpointInformation)	
	ON_COMMAND(ID_GRAB_SCID,MenuFunctions::GrabScid)
	ON_COMMAND(ID_GRAB_GUID,MenuFunctions::GrabGuid) 
	ON_COMMAND(ID_DECAL_PROPERTIES,MenuFunctions::DecalProperties)
	ON_COMMAND(ID_CAMERA_SETTOGO,MenuFunctions::SetCameraToGo)
	ON_COMMAND(ID_CAMERA_GOSET,MenuFunctions::SetGoToCamera)
	ON_COMMAND(ID_GRAB_LIGHT_GUID,MenuFunctions::GrabLightGuid)
	ON_COMMAND(ID_STARTINGPOS_PROPERTIES_INFO,MenuFunctions::StartingPositionInfo)
	ON_COMMAND(ID_NODE_HIDE,MenuFunctions::HideNodes)
	ON_COMMAND(ID_DECAL_GRAB_GUID,MenuFunctions::GrabDecalGuid)
	ON_COMMAND(ID_KEY_LEFT,MenuFunctions::KeyLeft)
	ON_COMMAND(ID_KEY_RIGHT,MenuFunctions::KeyRight)
	ON_COMMAND(ID_KEY_UP,MenuFunctions::KeyUp)
	ON_COMMAND(ID_KEY_DOWN,MenuFunctions::KeyDown)
	ON_COMMAND(ID_KEY_DELETE,MenuFunctions::KeyDelete)
	ON_COMMAND(ID_NODE_LOGICALNODEPROPERTIES,MenuFunctions::LogicalNodeProperties)
	ON_COMMAND(ID_TOOLS_CONVERSATIONEDITOR,MenuFunctions::ConversationEditor)


END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorView construction/destruction

CSiegeEditorView::CSiegeEditorView()
	: m_bValidPlace( true )
	, m_bShiftDown( false )
	, m_bMoving( false )
	, m_bCameraScrolling( false )
	, m_bNodeJustSnapped( false )
{
}

CSiegeEditorView::~CSiegeEditorView()
{
}

BOOL CSiegeEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorView drawing

void CSiegeEditorView::OnDraw(CDC* pDC)
{
	CSiegeEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

void CSiegeEditorView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
}

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorView diagnostics

#ifdef _DEBUG
void CSiegeEditorView::AssertValid() const
{
	CView::AssertValid();
}

void CSiegeEditorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSiegeEditorDoc* CSiegeEditorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSiegeEditorDoc)));
	return (CSiegeEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorView message handlers

void CSiegeEditorView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}

BOOL CSiegeEditorView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	BOOL bSuccess = CView::Create( lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext );

	// get the game up
	if ( bSuccess )
	{
		gEditor.SetParentWnd( m_hWnd );
		gEditor.SetStartupRect( rect );
		if ( !gEditor.Init( AfxGetInstanceHandle() ) )
		{
			return ( FALSE );
		}

		CreatePopupMenus();
	}

	// reset our cursor class
	::SetClassLong( m_hWnd, GCL_HCURSOR, NULL );

	return ( bSuccess );
}

void CSiegeEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);

	if (( nFlags == MK_RBUTTON ) && 
		(( GetKeyState( VK_SPACE ) & 0x80 ) || gPreferences.GetRightClickScroll() ) && 
		m_bCameraScrolling )
	{
		CPoint new_point;
		new_point.x = gEditorCamera.GetCameraXScrollPos();
		new_point.y = gEditorCamera.GetCameraYScrollPos();
		this->ClientToScreen( &new_point );

		gEditorCamera.UpdateScrolling( point.x, point.y, false );

		POINT cur_pos;
		GetCursorPos( &cur_pos );
		if (( cur_pos.x != new_point.x ) || ( cur_pos.y != new_point.y )) 
		{
			SetCursorPos( new_point.x, new_point.y );
		}
	}
	else if (( nFlags == (MK_RBUTTON | MK_SHIFT) ) ||
			( nFlags == (MK_RBUTTON | MK_LBUTTON ))) 
	{
		CPoint new_point;
		new_point.x = gEditorCamera.GetCameraXScrollPos();
		new_point.y = gEditorCamera.GetCameraYScrollPos();
		this->ClientToScreen( &new_point );

		gEditorCamera.UpdateScrolling( point.x, point.y, true );

		POINT cur_pos;
		GetCursorPos( &cur_pos );
		if (( cur_pos.x != new_point.x ) || ( cur_pos.y != new_point.y )) 
		{
			SetCursorPos( new_point.x, new_point.y );
		}
	}
	else if ( nFlags == MK_MBUTTON ) 
	{
		CPoint new_point;
		new_point.x = gEditorCamera.GetCameraXRotatePos();
		new_point.y = gEditorCamera.GetCameraYRotatePos();
		this->ClientToScreen( &new_point );

		if ( gPreferences.GetModeNodePlacement() && !gPreferences.GetVisualRotateOrient() )
		{
			gEditorTerrain.RotateSelectedNodeMesh( point.x, point.y );
		}
		else
		{
			gEditorCamera.Rotate( point.x, point.y );
		}

		POINT cur_pos;
		GetCursorPos( &cur_pos );
		if (( cur_pos.x != new_point.x ) || ( cur_pos.y != new_point.y )) 
		{
			SetCursorPos( new_point.x, new_point.y );
		}
	}
	else if ( nFlags == MK_LBUTTON ) 
	{
		if ( Dialog_Object_Grouping::DoesSingletonExist() && gObjectGrouping.GetRotateMode() && IsMouseOnTerrain() )
		{			
			gGizmoManager.RotateSelected( point.x - m_lastPoint.x );
		}
		else if ( gPreferences.GetPaintSelectNodes() && IsMouseOnTerrain() && (gSiegeEngine.GetMouseShadow().GetFloorHitPos().node != gEditorTerrain.GetSelectedNode()) ) 
		{
			gEditorTerrain.PaintAddNodeAsSelected( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
		}
		else if ( gGizmoManager.GetLeadGizmo() ) 
		{			
			gGizmoManager.Move();
			m_bMoving = true;
		}
	}
	else if ( nFlags == (MK_LBUTTON | MK_SHIFT) ) 
	{
		if ( gGizmoManager.GetLeadGizmo() ) 
		{
			gGizmoManager.Move( true );
			m_bMoving = true;
		}
	}
	else if ( Dialog_Portraits::DoesSingletonExist() && gDialogPortraits.GetAllowDrag() )
	{
		gDialogPortraits.AdjustRectToMouse( point.x, point.y );
	}
	else if ( gPreferences.GetModeNodePlacement() )
	{
		CPoint last = m_lastPoint;		
		this->ClientToScreen( &last );		

		POINT cur_pos;
		GetCursorPos( &cur_pos );
		if (( cur_pos.x != last.x ) || ( cur_pos.y != last.y )) 
		{	
			if ( m_bNodeJustSnapped )
			{
				gEditorTerrain.SetSelectedSnapped( false );
				m_bNodeJustSnapped = false;
			}

			if ( gEditorTerrain.GetSelectedSnapped() )
			{
				m_bNodeJustSnapped = true;
			}	

			gEditorTerrain.ResetNodeCalcTimer();
			gEditorTerrain.CalculateNodePlacement();				
		}				
	}

	m_lastPoint = point;
}

void CSiegeEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{	
	gGizmoManager.MoveUpdate();

	gCommandManager.EndActionSet();

	bool bShift = false;
	if ( nFlags == (nFlags | MK_CONTROL) )
	{
		bShift = true;
	}

	if (( !bShift ) && (!m_bMoving) && gPreferences.CanSelect() ) {
		gGizmoManager.DeselectAll();
	}	

	Gizmo gizmo;
	if ( gGizmoManager.GizmoHitDetect( gizmo, false ) == true ) {

		if ( gizmo.id != GIZMO_INVALID ) {	
			if ( bShift && !gizmo.bSelected )
			{
				gGizmoManager.DeselectGizmo( gizmo.id );
			}
			else if ( gGizmoManager.IsLastSelectedHit() ) 
			{
				gGizmoManager.SelectGizmo( gGizmoManager.GetLastSelectedGizmoID() );				
			}
			else 
			{
				gGizmoManager.SelectGizmo( gizmo.id );
				gGizmoManager.SetLastSelectedGizmoID( gizmo.id );
			}
		}

		if ( gPreferences.GetModeNodes() )
		{
			gEditorTerrain.SetSelectedNode( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
		}		
	}
	else
	{
		if ( gPreferences.GetSelectionLock() && bShift && (!m_bMoving) && gPreferences.CanSelect() )
		{
			gGizmoManager.DeselectAll();
		}
	}

	gGizmoManager.EndDragSelect();
	m_bMoving = false;
	CView::OnLButtonUp(nFlags, point);
}

void CSiegeEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	::SetFocus( gEditor.GetMainWnd() );

	if ( Dialog_Object_Grouping::DoesSingletonExist() && gObjectGrouping.GetRotateMode() && IsMouseOnTerrain() )
	{
		gGizmoManager.SetMouseRotatePosition( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
		return;
	}

	gCommandManager.BeginActionSet();
	
	gGizmoManager.SetLeadGizmo( 0 );
	m_bValidPlace = true;
	Gizmo gizmo;

	gGizmoManager.HitDetectLeadGizmo();

	bool bDetect = gGizmoManager.GizmoHitDetect( gizmo, false );
	
	bool bControl = false;
	if ( nFlags == (nFlags | MK_CONTROL) )
	{
		bControl = true;
	}	

	if (( bControl == false  ) && !gizmo.bSelected && gPreferences.CanSelect() ) 
	{
		if ( gPreferences.GetLinkMode() == false && gEditorObjects.IsSequenceEditing() == false && gPreferences.GetPlaceMeterPoints() == false )
		{
			gGizmoManager.DeselectAll();	
		}
	}
	
	if ( bDetect ) 
	{	
		m_bValidPlace = false;
		
		if ( bControl && gizmo.bSelected )
		{
			gGizmoManager.DeselectGizmo( gizmo.id );
			gGizmoManager.HitDetectLeadGizmo();
		}		
		else
		{	
			if ( gPreferences.GetModeNodes() )
			{
				gEditorTerrain.SetSelectedNode( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
				gEditorTerrain.SetSourceDoors( gEditorTerrain.GetSelectedNode() );
				gLeftView.PublishSourceDoorList();
				gEditorTerrain.SetSelectedSourceDoorID( 0 );
			}

			gGizmoManager.SelectGizmo( gizmo.id );
			gGizmoManager.HitDetectLeadGizmo();
			gGizmoManager.SetLastSelectedGizmoID( gizmo.id );
			gGizmoManager.CalculateMoveOffset();
		}
	}
	else {
		bool bPlaced = false;
		if ( IsMouseOnTerrain() ) 
		{
			if ( gPreferences.GetModeNodePlacement() && gEditorTerrain.GetSecondaryNode() != UNDEFINED_GUID ) 
			{				
				gEditorTerrain.PlaceSelectedNode();
			}
			else
			{
				if ((( nFlags != (MK_LBUTTON|MK_CONTROL) ) || ( gEditorTerrain.GetSelectedNode() == UNDEFINED_GUID )) && gPreferences.GetModeNodes() && gEditorTerrain.GetSelectedNode() != gSiegeEngine.GetMouseShadow().GetFloorHitPos().node ) 
				{
					gEditorTerrain.SetSelectedNode( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
					gEditorTerrain.SetSourceDoors( gEditorTerrain.GetSelectedNode() );
					gLeftView.PublishSourceDoorList();
					gEditorTerrain.SetSelectedSourceDoorID( 0 );				
				}			
				else if ( gPreferences.GetModeNodes() ) 
				{				
					gEditorTerrain.AddNodeAsSelected( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
				}

				if ( gPreferences.GetModeLogicalNodes() && ( nFlags != (MK_LBUTTON|MK_CONTROL) ) )
				{
					gEditorTerrain.DeselectAllLogicalIds();
					gEditorTerrain.AddSelectedLogicalId( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node,
														 gEditorTerrain.GetTempHitLogicalNode() );
				}
				else if ( gPreferences.GetModeLogicalNodes() )
				{
					gEditorTerrain.AddSelectedLogicalId( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node,
														 gEditorTerrain.GetTempHitLogicalNode() );
				}
				
				if ( gPreferences.CanPlace() && (!gEditorObjects.GetSelectedTemplateName().empty() || gEditorObjects.IsSequenceEditing() || gEditorTuning.GetMakerEnabled() || gPreferences.GetPlaceMeterPoints() ) && m_bValidPlace )  
				{
					bPlaced = true;
					SiegePos spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

					if ( gEditorObjects.GetSelectedTemplateName().same_no_case( "Point Light" ) ) 
					{
						gGizmoManager.DeselectAll();
						gEditorLights.AddDefaultPointLight( spos );					
					}
					else if ( gEditorObjects.GetSelectedTemplateName().same_no_case( "Spotlight" ) ) 
					{
						gGizmoManager.DeselectAll();
						gEditorLights.AddDefaultSpotlight( spos );
					}			
					else if ( gEditorObjects.GetSelectedTemplateName().same_no_case( "Starting Position" ) )
					{
						gGizmoManager.DeselectAll();
						gEditorGizmos.AddStartingPosition( spos );										
					}
					else if ( FileSys::IFileMgr::IsAliasedFrom( gEditorObjects.GetSelectedTemplateName(), ".%img%" ) )
					{
						gGizmoManager.DeselectAll();
						gEditorGizmos.AddDecal( gEditorObjects.GetSelectedTemplateName(), spos );
					}
					else if ( gEditorTuning.GetMakerEnabled() )
					{
						gEditorTuning.AddPoint();
					}
					else 
					{
						if ( gEditorObjects.IsSequenceEditing() )
						{
							gEditorObjects.PlaceSequenceObject();
						}
						else if ( gPreferences.GetPlaceMeterPoints() )
						{
							gEditorObjects.PlaceMeterPoint();
						}
						else
						{
							gEditorObjects.AddSelectedObject();										
						}
					}
				}			
			}
		}
		else if ( gPreferences.GetModeNodePlacement() && gEditorTerrain.GetSecondaryNode() != UNDEFINED_GUID ) 
		{
			gEditorTerrain.PlaceSelectedNode();
		}

		if (!( ( gEditorTerrain.GetSelectedNode() != UNDEFINED_GUID ) && gPreferences.GetPaintSelectNodes() ) && !bPlaced ) 
		{
			gGizmoManager.BeginDragSelect();
		}
	}

	if ( Dialog_Portraits::DoesSingletonExist() && gDialogPortraits.GetAllowDrag() )
	{
		gDialogPortraits.SetAllowDrag( false );
	}

	CView::OnLButtonDown(nFlags, point);
}

BOOL CSiegeEditorView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if ( zDelta < 0 ) {
		gEditorCamera.UpdateZoom( N );
	}
	else {
		gEditorCamera.UpdateZoom( S );
	}
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CSiegeEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_bCameraScrolling = false;
	::SetFocus( gEditor.GetMainWnd() );	

	if ( gPreferences.GetModeNodePlacement() )
	{
		gEditorTerrain.RotateSelectedNode();
		return;
	}

	gGizmoManager.HitDetectLeadGizmo();
	if ( gGizmoManager.GetLeadGizmo() && gGizmoManager.GetLeadGizmo()->bSelected ) 
	{		
		CPoint pt = point;
		ClientToScreen( &pt );

		if ( gGizmoManager.GetLeadGizmo()->type == GIZMO_STARTINGPOSITION )
		{
			m_wndStartingPosProp.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
		}
		else if ( gGizmoManager.GetLeadGizmo()->type == GIZMO_HOTPOINT )			
		{
			m_wndHotpointProp.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
		}
		else if ( gGizmoManager.GetLeadGizmo()->type == GIZMO_DECAL )
		{
			m_wndDecalProp.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
		}
		else
		{
			m_wndProperties.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
		}
	}
	else 
	{
		if (( GetKeyState( VK_SPACE ) & 0x80 ) || gPreferences.GetRightClickScroll() )
		{
			RECT rect;
			this->GetClientRect( &rect );
			gEditorCamera.SetCameraXScrollPos( rect.right/2 );
			gEditorCamera.SetCameraYScrollPos( rect.bottom/2 );
			POINT new_point;
			new_point.x = gEditorCamera.GetCameraXScrollPos();
			new_point.y = gEditorCamera.GetCameraYScrollPos();
			this->ClientToScreen( &new_point );
			SetCursorPos( new_point.x, new_point.y );
			while ( ShowCursor( false ) >= 0 );
			SetCapture();
			CView::OnRButtonDown(nFlags, point);
			m_bCameraScrolling = true;
		}
		else
		{
			if ( IsMouseOnTerrain() ) 
			{
				if (( nFlags != (MK_RBUTTON|MK_CONTROL) ) || ( gEditorTerrain.GetSelectedNode() == UNDEFINED_GUID )) 
				{
					if ( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node != gEditorTerrain.GetSelectedNode() )
					{
						gEditorTerrain.SetSelectedNode( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
						gEditorTerrain.SetSourceDoors( gEditorTerrain.GetSelectedNode() );
						gLeftView.PublishSourceDoorList();
						gEditorTerrain.SetSelectedSourceDoorID( 0 );
					}
				}
				else 
				{
					gEditorTerrain.AddNodeAsSelected( gSiegeEngine.GetMouseShadow().GetFloorHitPos().node );
				}

				CPoint pt = point;
				ClientToScreen( &pt );
				m_wndNodeProperties.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
			}
		}
	}
}

void CSiegeEditorView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	while ( ShowCursor( true ) < 0 );			

	CView::OnRButtonUp(nFlags, point);
}

void CSiegeEditorView::OnMButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	while ( ShowCursor( true ) < 0 );
	CView::OnMButtonUp( nFlags, point );
}

void CSiegeEditorView::OnMButtonDown(UINT nFlags, CPoint point)
{
	::SetFocus( gEditor.GetMainWnd() );

	if ( gPreferences.GetModeNodePlacement() && !gPreferences.GetVisualRotateOrient() )
	{			
		POINT newPoint;
		newPoint = point;
		gEditorCamera.SetCameraXRotatePos( newPoint.x );
		gEditorCamera.SetCameraYRotatePos( newPoint.y );

		/*
		int x = 0, y = 0;
		gEditorTerrain.GetSelectedPosPoint( x, y );					
		newPoint.x = x;
		newPoint.y = y;
		*/

		this->ClientToScreen( &newPoint );
		SetCursorPos( newPoint.x, newPoint.y );
		
		while ( ShowCursor( false ) >= 0 );
		SetCapture();
		CView::OnMButtonDown( nFlags, point );		
	}
	else
	{
		RECT rect;
		this->GetClientRect( &rect );
		gEditorCamera.SetCameraXRotatePos( rect.right/2 );
		gEditorCamera.SetCameraYRotatePos( rect.bottom/2 );
		POINT new_point;
		new_point.x = gEditorCamera.GetCameraXRotatePos();
		new_point.y = gEditorCamera.GetCameraYRotatePos();
		this->ClientToScreen( &new_point );
		SetCursorPos( new_point.x, new_point.y );
		while ( ShowCursor( false ) >= 0 );
		SetCapture();
		CView::OnMButtonDown( nFlags, point );
	}
}



bool CSiegeEditorView::IsMouseOnTerrain()
{
	siege::SiegeEngine & engine = gSiegeEngine;

	if ( !gEditorRegion.GetRegionHandle().IsValid() ) {
		return false;
	}
	
	bool bHitTerrain = false;

	// Set the terrain hit distance to max
	gSiegeEngine.GetMouseShadow().SetFloorHitDist( FLOAT_MAX );

	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;
	for( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
	{		
		SiegeNode& node = *(*iNode);

		// Trace the mouse position into this node
		vector_3 ray_origin		= node.WorldToNodeSpace( engine.GetCamera().GetCameraPosition() );
		vector_3 ray_direction	= node.GetTransposeOrientation() * ( engine.GetCamera().GetMatrixOrientation() * engine.GetMouseShadow().CameraSpacePosition() );

		float ray_min			= FLOAT_MIN;
		vector_3 face_normal( DoNotInitialize );

		if( node.HitTest( ray_origin, ray_direction, ray_min, face_normal, LF_IS_ANY ) )
		{
			if( ray_min > 0.0f )
			{
				bHitTerrain		= true;				
			}
		}

		eLogicalNodeFlags lf = LF_IS_ANY;
		if ( gPreferences.GetHitFloorsOnly() )
		{
			lf = LF_IS_FLOOR;
		}

		if( node.GetVisible() && node.HitTest( ray_origin, ray_direction, ray_min, face_normal, lf ) )
		{
			if( ray_min > 0.0f )
			{				
				float distToHit	= ray_direction.Length() * ray_min;

				if( distToHit < engine.GetMouseShadow().GetTerrainHitDistance() )
				{
					// Set the new hit distance
					engine.GetMouseShadow().SetFloorHitDist( distToHit );
					
					SiegePos hitPos;
					hitPos.pos	= ray_origin + ( ray_direction * ray_min );
					hitPos.node = node.GetGUID();
					gSiegeEngine.GetMouseShadow().SetFloorHitPos( hitPos );
					
					SetHitFaceNormal( face_normal );
				}
			}
		
			if ( gPreferences.GetModeLogicalNodes() )
			{				
				for ( unsigned int ln = 0; ln != node.GetNumLogicalNodes(); ++ln )
				{
					if ( node.GetLogicalNodes()[ln]->HitTest( ray_origin, ray_direction, ray_min, face_normal ) )
					{
						if ( gPreferences.GetHitFloorsOnly() && 
							 node.GetLogicalNodes()[ln]->GetFlags() & LF_IS_FLOOR )
						{
							gEditorTerrain.SetTempHitLogicalNode( node.GetLogicalNodes()[ln]->GetID() );
							break;
						}
						else if ( gEditorTerrain.GetTempHitLogicalNode() != node.GetLogicalNodes()[ln]->GetID() )
						{
							gEditorTerrain.SetTempHitLogicalNode( node.GetLogicalNodes()[ln]->GetID() );
							break;
						}
					}
				}
			}
		}
	}

	return bHitTerrain;
}


void CSiegeEditorView::OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu )
{
	CString sString;
	sString.LoadString( nItemID );

	((CMainFrame *)((CSiegeEditorApp *)GetParent())->m_pMainWnd)->GetStatusBar().GetStatusBarCtrl().SetText( sString.GetBuffer( sString.GetLength() ), 0, SBT_NOBORDERS );
}

void CSiegeEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_SHIFT ) 
	{
		m_bShiftDown = true;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSiegeEditorView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_SHIFT ) 
	{
		m_bShiftDown = false;
	}

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CSiegeEditorView::CreatePopupMenus()
{
	m_wndProperties.CreatePopupMenu();
	m_wndProperties.AppendMenu( MF_ENABLED, ID_OBJECT_PROPERTIES, "Properties" );	
	m_wndProperties.AppendMenu( MF_ENABLED, ID_LINK_MODE, "Link Object" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_OBJECT_HIDE, "Hide Object" );
	m_wndProperties.AppendMenu( MF_SEPARATOR );	
	m_wndProperties.AppendMenu( MF_ENABLED, ID_OBJECT_TRIGGERS_EXECUTE, "Execute Triggers" );	
	m_wndProperties.AppendMenu( MF_ENABLED, ID_OBJECT_TRIGGERS_RESET, "Reset Triggers" );	
	m_wndProperties.AppendMenu( MF_SEPARATOR );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_EDIT_CUT, "Cut" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_EDIT_COPY, "Copy" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_EDIT_PASTE, "Paste" );
	m_wndProperties.AppendMenu( MF_SEPARATOR );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_ADDGOTOCUSTOMVIEW, "Add To Custom View" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_GRAB_SCID, "Copy Scid to Clipboard" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_GRAB_LIGHT_GUID, "Copy Light Guid to Clipboard" );
	m_wndProperties.AppendMenu( MF_SEPARATOR );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_CAMERA_SETTOGO, "Set Camera to Game Object" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_CAMERA_GOSET, "Set Game Object to Camera" );
	m_wndProperties.AppendMenu( MF_SEPARATOR );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_TOOLS_CONVERSATIONEDITOR, "Region Conversation Manager" );


	m_wndNodeProperties.CreatePopupMenu();
	m_wndNodeProperties.AppendMenu( MF_ENABLED, ID_NODE_PROPERTIES, "Node Properties" );
	m_wndNodeProperties.AppendMenu( MF_ENABLED, ID_GRAB_GUID, "Copy GUID to Clipboard" );
	m_wndNodeProperties.AppendMenu( MF_ENABLED, ID_NODE_HIDE, "Hide Selected Nodes" );
	m_wndNodeProperties.AppendMenu( MF_ENABLED, ID_NODE_LOGICALNODEPROPERTIES, "Logical Node Properties" );

	m_wndHotpointProp.CreatePopupMenu();
	m_wndHotpointProp.AppendMenu( MF_ENABLED, ID_HOTPOINT_INFORMATION, "Hotpoint Information" );
	
	m_wndStartingPosProp.CreatePopupMenu();	
	m_wndStartingPosProp.AppendMenu( MF_ENABLED, ID_STARTINGPOS_PROPERTIES_INFO, "Properties" );
	m_wndStartingPosProp.AppendMenu( MF_ENABLED, ID_STARTINGPOS_CAMERASET, "Set Current Camera to Position" );
	m_wndStartingPosProp.AppendMenu( MF_ENABLED, ID_STARTINGPOS_CAMERAPREVIEW, "Preview Camera" );

	m_wndDecalProp.CreatePopupMenu();
	m_wndDecalProp.AppendMenu( MF_ENABLED, ID_DECAL_PROPERTIES, "Decal Properties" );
	m_wndDecalProp.AppendMenu( MF_ENABLED, ID_DECAL_GRAB_GUID, "Copy Decal Guid to Clipboard" );
}


void CSiegeEditorView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if ( gEditor.HasMainWnd() )
	{
		::SetWindowPos( gEditor.GetMainWnd(), NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER );
	}
}

BOOL CSiegeEditorView::OnEraseBkgnd(CDC* /*pDC*/) 
{
	return TRUE;
}



void CSiegeEditorView::OnKillFocus(CWnd* pNewWnd) 
{
	CView::OnKillFocus(pNewWnd);
	
	gGizmoManager.EndDragSelect();	
}

void CSiegeEditorView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	gGizmoManager.EndDragSelect();
	
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}
