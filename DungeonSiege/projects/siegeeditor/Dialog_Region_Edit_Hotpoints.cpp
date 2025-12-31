// Dialog_Region_Edit_Hotpoints.cpp : implementation file
//
#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Region_Edit_Hotpoints.h"
#include "EditorRegion.h"
#include "EditorMap.h"
#include "EditorTerrain.h"
#include "EditorHotpoints.h"
#include "Dialog_Region_Hotpoint_Direction.h"
#include "Dialog_Region_New_Hotpoint.h"
#include "siege_hotpoint_database.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Edit_Hotpoints dialog


Dialog_Region_Edit_Hotpoints::Dialog_Region_Edit_Hotpoints(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Region_Edit_Hotpoints::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Region_Edit_Hotpoints)
	//}}AFX_DATA_INIT
}


void Dialog_Region_Edit_Hotpoints::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Region_Edit_Hotpoints)
	DDX_Control(pDX, IDC_LIST_HOTPOINTS, m_hotpoints);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Region_Edit_Hotpoints, CDialog)
	//{{AFX_MSG_MAP(Dialog_Region_Edit_Hotpoints)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_DIRECTIONAL, OnButtonEditDirectional)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_LBN_SELCHANGE(IDC_LIST_HOTPOINTS, OnSelchangeListHotpoints)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Edit_Hotpoints message handlers


BOOL Dialog_Region_Edit_Hotpoints::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	std::vector< DWORD > dirHotpoints;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DIRHOTPOINT, dirHotpoints );
	for ( i = dirHotpoints.begin(); i != dirHotpoints.end(); ++i )
	{
		siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
		siege::SiegeHotpointList::iterator j;
		for ( j = hotpoints.begin(); j != hotpoints.end(); ++j )
		{
			if ( (*j).m_Id == *i )
			{
				int index = m_hotpoints.AddString( ToAnsi( (*j).m_Name ) );
				m_hotpoints.SetItemData( index, (*j).m_Id );
				break;
			}
		}
	}

	gEditorHotpoints.SetCanDraw( true );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT Dialog_Region_Edit_Hotpoints::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{	
	return CDialog::WindowProc(message, wParam, lParam);
}

void Dialog_Region_Edit_Hotpoints::OnButtonEditDirectional() 
{
	int sel = m_hotpoints.GetCurSel();
	int id = -1;
	if ( sel != LB_ERR )
	{
		id = m_hotpoints.GetItemData( sel );
		
		if( gGizmoManager.GetGizmo( id ) )
		{
			gEditorHotpoints.SetCurrentDirection( gGizmoManager.GetGizmo( id )->sdirection.pos );
			Dialog_Region_Hotpoint_Direction dlg;
			dlg.DoModal();				
			gGizmoManager.GetGizmo( id )->sdirection.pos = gEditorHotpoints.GetCurrentDirection();
		}
	}
}

void Dialog_Region_Edit_Hotpoints::OnOK() 
{
	gEditorHotpoints.SetCanDraw( false );	
	CDialog::OnOK();
}

void Dialog_Region_Edit_Hotpoints::OnButtonNew() 
{
	Dialog_Region_New_Hotpoint newHp;
	if ( newHp.DoModal() != IDOK )
	{
		return;
	}
	gEditorHotpoints.AddDirectionalHotpoint( vector_3( 1.0f, 0.0f, 0.0f ), gEditorHotpoints.GetSelectedID() );
	
	m_hotpoints.ResetContent();
	std::vector< DWORD > dirHotpoints;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DIRHOTPOINT, dirHotpoints );
	for ( i = dirHotpoints.begin(); i != dirHotpoints.end(); ++i )
	{
		siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
		siege::SiegeHotpointList::iterator j;
		for ( j = hotpoints.begin(); j != hotpoints.end(); ++j )
		{
			if ( (*j).m_Id == *i )
			{
				int index = m_hotpoints.AddString( ToAnsi( (*j).m_Name ) );
				m_hotpoints.SetItemData( index, (*j).m_Id );
				break;
			}
		}
	}
}

void Dialog_Region_Edit_Hotpoints::OnButtonRemove() 
{
	if ( m_hotpoints.GetCurSel() != LB_ERR )
	{
		int id = m_hotpoints.GetItemData( m_hotpoints.GetCurSel() );
		gEditorHotpoints.Delete( id );
	}

	m_hotpoints.ResetContent();
	std::vector< DWORD > dirHotpoints;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DIRHOTPOINT, dirHotpoints );
	for ( i = dirHotpoints.begin(); i != dirHotpoints.end(); ++i )
	{
		siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
		siege::SiegeHotpointList::iterator j;
		for ( j = hotpoints.begin(); j != hotpoints.end(); ++j )
		{
			if ( (*j).m_Id == *i )
			{
				int index = m_hotpoints.AddString( ToAnsi( (*j).m_Name ) );
				m_hotpoints.SetItemData( index, (*j).m_Id );
				break;
			}
		}
	}
}

void Dialog_Region_Edit_Hotpoints::OnSelchangeListHotpoints() 
{
	int sel = m_hotpoints.GetCurSel();
	if ( sel != LB_ERR )
	{
		gEditorHotpoints.SetSelectedID( m_hotpoints.GetItemData( sel ) );
	}
}
