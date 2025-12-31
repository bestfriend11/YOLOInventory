// Object_Property_Sheet.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Equipment.h"
#include "PropPage_Properties_Inventory.h"
#include "PropPage_Properties_Lighting.h"
#include "PropPage_Properties_Rotation.h"
#include "PropPage_Properties_Template.h"
#include "PropPage_Properties_Triggers.h"
#include "Preferences.h"
#include "GoData.h"
#include "EditorRegion.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Object_Property_Sheet

IMPLEMENT_DYNAMIC(Object_Property_Sheet, CPropertySheet)

Object_Property_Sheet::Object_Property_Sheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CResizableSheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_bMinimized = false;
}

Object_Property_Sheet::Object_Property_Sheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CResizableSheet(pszCaption, pParentWnd, iSelectPage)
{
	m_bMinimized = false;
}

Object_Property_Sheet::~Object_Property_Sheet()
{
}


BEGIN_MESSAGE_MAP(Object_Property_Sheet, CResizableSheet)
	//{{AFX_MSG_MAP(Object_Property_Sheet)
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_OBJECT_OK_BUTTON, OnObjectOkay)
	ON_BN_CLICKED(ID_OBJECT_APPLY_BUTTON, OnObjectApply)
	ON_BN_CLICKED(ID_OBJECT_CANCEL_BUTTON, OnObjectCancel)
	ON_BN_CLICKED(ID_OBJECT_HELP_BUTTON, OnObjectHelp)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Object_Property_Sheet message handlers

int Object_Property_Sheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CResizableSheet::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rectWnd;
	GetWindowRect( rectWnd );

	CRect rectOk;
	rectOk.top		= rectWnd.bottom - 30;
	rectOk.bottom	= rectWnd.bottom - 5;
	rectOk.left		= rectWnd.right - 105;
	rectOk.right	= rectWnd.right - 200;
	m_wndOk.Create( "OK", WS_VISIBLE | WS_CHILD, rectOk, this, ID_OBJECT_OK_BUTTON );

	CRect rectApply;
	rectApply.top		= rectWnd.bottom - 30;
	rectApply.bottom	= rectWnd.bottom - 5;
	rectApply.left		= rectWnd.right - 5;
	rectApply.right		= rectWnd.right - 100;
	m_wndApply.Create( "Apply", WS_VISIBLE | WS_CHILD, rectApply, this, ID_OBJECT_APPLY_BUTTON );

	m_wndCancel.Create( "Cancel", WS_VISIBLE | WS_CHILD, rectApply, this, ID_OBJECT_CANCEL_BUTTON );

	m_wndHelp.Create( "Help", WS_VISIBLE | WS_CHILD, rectApply, this, ID_OBJECT_HELP_BUTTON );

	CFont font;
	font.CreateStockObject( DEFAULT_GUI_FONT );
	m_wndOk.SetFont( &font );
	m_wndApply.SetFont( &font );
	m_wndCancel.SetFont( &font );
	m_wndHelp.SetFont( &font );
	
	return 0;
}


BOOL Object_Property_Sheet::OnInitDialog() 
{	
	BOOL bResult = CResizableSheet::OnInitDialog();	

	CRect rectWnd;
	GetWindowRect( rectWnd );	
	MoveWindow( rectWnd.left, rectWnd.top, rectWnd.Width(), rectWnd.Height() + 35 );
	
	CRect rectOk;
	CRect rectApply;
	CRect rectCancel;
	CRect rectHelp;
	GetClientRect( rectWnd );
	
	rectCancel.top		= rectWnd.bottom - 30;
	rectCancel.bottom	= rectWnd.bottom - 5;
	rectCancel.right	= rectWnd.right - 10;
	rectCancel.left		= rectWnd.right - 90;	

	rectApply.top		= rectWnd.bottom - 30;
	rectApply.bottom	= rectWnd.bottom - 5;
	rectApply.right		= rectWnd.right - 95;
	rectApply.left		= rectWnd.right - 175;

	rectOk.top			= rectWnd.bottom - 30;
	rectOk.bottom		= rectWnd.bottom - 5;
	rectOk.right		= rectWnd.right - 180;
	rectOk.left			= rectWnd.right - 260;

	rectHelp.top		= rectWnd.bottom - 30;
	rectHelp.bottom		= rectWnd.bottom - 5;
	rectHelp.left		= rectWnd.left + 5;
	rectHelp.right		= rectWnd.left + 85;
	
	m_wndOk.MoveWindow( rectOk, TRUE );	
	m_wndApply.MoveWindow( rectApply, TRUE );
	m_wndCancel.MoveWindow( rectCancel, TRUE );
	m_wndHelp.MoveWindow( rectHelp, TRUE );

	m_resize.Create( this, FALSE );
  
	CResizeInfo rInfo[] =
	{
	//  id											l		t		w		h
	{ ID_OBJECT_OK_BUTTON,							100,	100,	0,		0 },
	{ ID_OBJECT_APPLY_BUTTON,						100,	100,	0,		0 },
	{ ID_OBJECT_CANCEL_BUTTON,						100,	100,	0,		0 },
	{ ID_OBJECT_HELP_BUTTON,						0,		100,	0,		0 },
	{ this->GetTabControl()->GetDlgCtrlID(),		0,		0,		100,	100 },

	{ 0 },
	};

	m_resize.Add( rInfo );
	m_resize.SetGripEnabled( TRUE );
	m_resize.SetEnabled( TRUE );
	m_resize.SetMinimumTrackingSize();
	ShowSizeGrip( FALSE );

	((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( FALSE );

	if ( GetPage( TEMPLATE_PAGE )->GetSafeHwnd() )
	{
		GetPage( TEMPLATE_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( TRIGGERS_PAGE )->GetSafeHwnd() ) 
	{
		GetPage( TRIGGERS_PAGE )->EnableWindow( FALSE );		
	}

	if ( GetPage( ROTATION_PAGE )->GetSafeHwnd() )
	{
		GetPage( ROTATION_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( EQUIPMENT_PAGE )->GetSafeHwnd() )
	{
		GetPage( EQUIPMENT_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( INVENTORY_PAGE )->GetSafeHwnd() )
	{
		GetPage( INVENTORY_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( LIGHTING_PAGE )->GetSafeHwnd() )
	{
		GetPage( LIGHTING_PAGE )->EnableWindow( FALSE );
	}

	if ( gGizmoManager.IsObjectSelected() )
	{
		if ( GetPage( TEMPLATE_PAGE )->GetSafeHwnd() )
		{
			GetPage( TEMPLATE_PAGE )->EnableWindow( TRUE );
		}

		if ( GetPage( TRIGGERS_PAGE )->GetSafeHwnd() )
		{
			GetPage( TRIGGERS_PAGE )->EnableWindow( TRUE );		
		}
	}
	if ( gGizmoManager.IsLightSelected() )
	{
		if ( GetPage( LIGHTING_PAGE )->GetSafeHwnd() )
		{
			GetPage( LIGHTING_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveAspect() )
	{
		if ( GetPage( ROTATION_PAGE )->GetSafeHwnd() )
		{
			GetPage( ROTATION_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveInventory() )
	{
		if ( GetPage( INVENTORY_PAGE )->GetSafeHwnd() )
		{
			GetPage( INVENTORY_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveEquipment() )
	{
		if( GetPage( EQUIPMENT_PAGE )->GetSafeHwnd() )
		{
			GetPage( EQUIPMENT_PAGE )->EnableWindow( FALSE );
		}
	}

	// enable save/restore, with active page
	EnableSaveRestore(_T("Properties"),_T("Window"), TRUE);	
	SetActiveWindow();
	Invalidate();

	std::vector< DWORD > gizmo_ids;
	gGizmoManager.GetSelectedGizmoIDs( gizmo_ids );
	std::vector< DWORD >::iterator i;
	for ( i = gizmo_ids.begin(); i != gizmo_ids.end(); ++i ) 
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		if ( pGizmo )
		{
			switch ( pGizmo->type )
			{
			case GIZMO_OBJECTGIZMO:
				{
					GoHandle hObject( MakeGoid(pGizmo->id) );
					gpstring sTrigger = hObject->GetDataTemplate()->GetGroupName();
					if ( sTrigger.same_no_case( "trigger" ) )
					{
						SetActivePage( TRIGGERS_PAGE );
					}
				}
				break;
			case GIZMO_POINTLIGHT:
			case GIZMO_SPOTLIGHT:
				SetActivePage( LIGHTING_PAGE );
				break;
			}
		}
		break;
	}
		
	return bResult;
}



void Object_Property_Sheet::OnObjectOkay()
{
	OnObjectApply();
	EndDialog( IDOK );
}

void Object_Property_Sheet::Apply()
{
	OnObjectApply();
}

void Object_Property_Sheet::OnObjectApply()
{
	bool bTemplateApply		= ((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->CanApply();	
	bool bTriggersApply		= ((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->CanApply();
	bool bRotationApply		= ((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->CanApply();
	bool bEquipmentApply	= ((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->CanApply();
	bool bInventoryApply	= ((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->CanApply();
	bool bLightingApply		= ((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->CanApply();	

	if ( !bTemplateApply && !bTriggersApply && !bRotationApply && !bRotationApply && !bEquipmentApply &&
		 !bInventoryApply && !bLightingApply )
	{
		((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( FALSE );
	}
	else
	{
		((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( TRUE );
	}

	if ( bTemplateApply )
	{
		((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->Apply();
	}

	if ( bTriggersApply )
	{
		((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->Apply();
	}

	// Always apply rotation
	((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->Apply();

	if ( bEquipmentApply )
	{
		((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->Apply();
	}

	if ( bInventoryApply )
	{
		((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->Apply();
	}

	if ( bLightingApply )
	{
		((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->Apply();
	}

	((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( FALSE );

	gEditorRegion.SetRegionDirty();
}

void Object_Property_Sheet::OnObjectCancel()
{
	/*
	((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->Cancel();
	((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->Cancel();
	((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->Cancel();
	((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->Cancel();
	((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->Cancel();
	((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->Cancel();
	*/
	
	EndDialog( IDCANCEL );
}


void Object_Property_Sheet::OnSize(UINT nType, int cx, int cy) 
{
	CPropertySheet::OnSize(nType, cx, cy);	
	
	Invalidate( TRUE );
	UpdateWindow();	
	if( GetActivePage() )
	{
		if ( !m_bMinimized )
		{
			int original = this->GetActiveIndex();
  			if ( this->GetActiveIndex() == LIGHTING_PAGE )
  			{
  				SetActivePage( 0 );
  			}
  			else
  			{
  				SetActivePage( GetActiveIndex() + 1 );
  			}
  
	  		SetActivePage( original );
		}

		m_bMinimized = false;
		if ( nType == SIZE_MINIMIZED )
		{
			m_bMinimized = true;
		}

		GetActivePage()->Invalidate( TRUE );
		GetActivePage()->UpdateWindow();
		((CResizablePage *)GetActivePage())->ArrangeLayout();
	}
}


void Object_Property_Sheet::Reset()
{		
	if ( !GetSafeHwnd() )
	{
		return;
	}
	
	if ( GetPage( TEMPLATE_PAGE )->GetSafeHwnd() )
	{
		GetPage( TEMPLATE_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( TRIGGERS_PAGE )->GetSafeHwnd() ) 
	{
		GetPage( TRIGGERS_PAGE )->EnableWindow( FALSE );		
	}

	if ( GetPage( ROTATION_PAGE )->GetSafeHwnd() )
	{
		GetPage( ROTATION_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( EQUIPMENT_PAGE )->GetSafeHwnd() )
	{
		GetPage( EQUIPMENT_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( INVENTORY_PAGE )->GetSafeHwnd() )
	{
		GetPage( INVENTORY_PAGE )->EnableWindow( FALSE );
	}

	if ( GetPage( LIGHTING_PAGE )->GetSafeHwnd() )
	{
		GetPage( LIGHTING_PAGE )->EnableWindow( FALSE );
	}

	if ( gGizmoManager.IsObjectSelected() )
	{
		if ( GetPage( TEMPLATE_PAGE )->GetSafeHwnd() )
		{
			GetPage( TEMPLATE_PAGE )->EnableWindow( TRUE );
		}

		if ( GetPage( TRIGGERS_PAGE )->GetSafeHwnd() )
		{
			GetPage( TRIGGERS_PAGE )->EnableWindow( TRUE );		
		}
	}
	if ( gGizmoManager.IsLightSelected() )
	{
		if ( GetPage( LIGHTING_PAGE )->GetSafeHwnd() )
		{
			GetPage( LIGHTING_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveAspect() )
	{
		if ( GetPage( ROTATION_PAGE )->GetSafeHwnd() )
		{
			GetPage( ROTATION_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveInventory() )
	{
		if ( GetPage( INVENTORY_PAGE )->GetSafeHwnd() )
		{
			GetPage( INVENTORY_PAGE )->EnableWindow( TRUE );
		}
	}
	if ( gGizmoManager.DoesSelectionHaveEquipment() )
	{
		if( GetPage( EQUIPMENT_PAGE )->GetSafeHwnd() )
		{
			GetPage( EQUIPMENT_PAGE )->EnableWindow( FALSE );
		}
	}

	((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->Reset();
	((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->Reset();
	((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->Reset();
	((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->Reset();
	((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->Reset();
	((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->Reset();

	((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( FALSE );	

	SetActivePage( TEMPLATE_PAGE );
	
	std::vector< DWORD > gizmo_ids;
	gGizmoManager.GetSelectedGizmoIDs( gizmo_ids );
	std::vector< DWORD >::iterator i;
	for ( i = gizmo_ids.begin(); i != gizmo_ids.end(); ++i ) 
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		if ( pGizmo )
		{
			switch ( pGizmo->type )
			{
			case GIZMO_OBJECTGIZMO:
				{
					GoHandle hObject( MakeGoid(pGizmo->id) );
					gpstring sTrigger = hObject->GetDataTemplate()->GetGroupName();
					if ( sTrigger.same_no_case( "trigger" ) )
					{
						SetActivePage( TRIGGERS_PAGE );
					}
				}
				break;
			case GIZMO_POINTLIGHT:
			case GIZMO_SPOTLIGHT:
				SetActivePage( LIGHTING_PAGE );
				break;
			}
		}
		break;
	}
}


bool Object_Property_Sheet::Evaluate()
{
	bool bTemplateApply		= ((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->CanApply();	
	bool bTriggersApply		= ((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->CanApply();
	bool bRotationApply		= ((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->CanApply();
	bool bEquipmentApply	= ((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->CanApply();
	bool bInventoryApply	= ((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->CanApply();
	bool bLightingApply		= ((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->CanApply();	

	if ( !bTemplateApply && !bTriggersApply && !bRotationApply && !bRotationApply && !bEquipmentApply &&
		 !bInventoryApply && !bLightingApply )
	{
		((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( FALSE );
		return false;
	}
	else
	{
		((CButton *)GetDlgItem(ID_OBJECT_APPLY_BUTTON))->EnableWindow( TRUE );
		return true;
	}

	return false;
}

void Object_Property_Sheet::OnDestroy() 
{
	/*
	WINDOWPLACEMENT pos;
	GetWindowPlacement( &pos );
	gPreferences.SetPropertiesPos( pos );
	*/

	CResizableSheet::OnDestroy();
	
	bool bTemplateApply		= ((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->CanApply();	
	bool bTriggersApply		= ((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->CanApply();
	bool bRotationApply		= ((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->CanApply();
	bool bEquipmentApply	= ((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->CanApply();
	bool bInventoryApply	= ((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->CanApply();
	bool bLightingApply		= ((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->CanApply();	

	((PropPage_Properties_Template *)GetPage( TEMPLATE_PAGE ))->Cancel();
	if ( bTemplateApply && bTriggersApply && bRotationApply && bRotationApply && bEquipmentApply &&
		 bInventoryApply && bLightingApply )
	{		
		((PropPage_Properties_Triggers *)GetPage( TRIGGERS_PAGE ))->Cancel();
		((PropPage_Properties_Rotation *)GetPage( ROTATION_PAGE ))->Cancel();
		((PropPage_Properties_Equipment *)GetPage( EQUIPMENT_PAGE ))->Cancel();
		((PropPage_Properties_Inventory *)GetPage( INVENTORY_PAGE ))->Cancel();
		((PropPage_Properties_Lighting *)GetPage( LIGHTING_PAGE ))->Cancel();		
	}	
}

void Object_Property_Sheet::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CResizableSheet::OnShowWindow(bShow, nStatus);
}


void Object_Property_Sheet::OnObjectHelp()
{
	HELPINFO lhelpInfo;
	CResizableSheet::OnHelpInfo(&lhelpInfo);			
}
