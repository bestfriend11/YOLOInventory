// Dialog_Hotpoint_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Hotpoint_Properties.h"
#include "EditorTerrain.h"
#include "EditorHotpoints.h"
#include "siege_hotpoint_database.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Properties dialog


Dialog_Hotpoint_Properties::Dialog_Hotpoint_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Hotpoint_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Hotpoint_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Hotpoint_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Hotpoint_Properties)
	DDX_Control(pDX, IDC_COMBO_EXISTING_HOTPOINT, m_existing_hotpoint);
	DDX_Control(pDX, IDC_EDIT_HOTPOINT_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_HOTPOINT_ID, m_id);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Hotpoint_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Hotpoint_Properties)
	ON_CBN_SELCHANGE(IDC_COMBO_EXISTING_HOTPOINT, OnSelchangeComboExistingHotpoint)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Properties message handlers

void Dialog_Hotpoint_Properties::OnOK() 
{
	gpstring sId;
	CString rId;
	m_id.GetWindowText( rId );
	sId = rId;

	int id = 0;
	stringtool::Get( sId, id );
	
	CString rName;
	m_name.GetWindowText( rName );

	gEditorHotpoints.SetSelectedID( id );
	gEditorHotpoints.SetSelectedName( rName.GetBuffer( 0 ) );	

	CDialog::OnOK();
}

BOOL Dialog_Hotpoint_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sHotpoint;
	siege::database_guid guid( gEditorTerrain.GenerateRandomGUID().c_str() );	
	sHotpoint.assignf( "0x%08X", guid.GetValue() );
	m_id.SetWindowText( sHotpoint.c_str() );
	m_name.SetWindowText( sHotpoint.c_str() );

	siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
	siege::SiegeHotpointList::iterator i;
	for ( i = hotpoints.begin(); i != hotpoints.end(); ++i )
	{
		if ( gGizmoManager.GetGizmo( (*i).m_Id ) == 0 )
		{
			int index = m_existing_hotpoint.AddString( ToAnsi( (*i).m_Name.c_str() ) );
			m_existing_hotpoint.SetItemData( index, (*i).m_Id );
		}
	}
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Hotpoint_Properties::OnSelchangeComboExistingHotpoint() 
{
	int sel = m_existing_hotpoint.GetCurSel();
	int data = m_existing_hotpoint.GetItemData( sel );
	CString rName;
	m_existing_hotpoint.GetLBText( m_existing_hotpoint.GetCurSel(), rName );

	gpstring sNum;
	sNum.assignf( "%d", data );

	m_name.SetWindowText( rName );
	m_id.SetWindowText( sNum.c_str() );
}
