// Dialog_Map_Hotpoints.cpp : implementation file
//
#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Map_Hotpoints.h"
#include "EditorRegion.h"
#include "EditorMap.h"
#include "Dialog_Map_New_Hotpoint.h"
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
// Dialog_Map_Hotpoints dialog


Dialog_Map_Hotpoints::Dialog_Map_Hotpoints(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Map_Hotpoints::IDD, pParent)
{
	m_highestHotpointID = 0;
	//{{AFX_DATA_INIT(Dialog_Map_Hotpoints)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Map_Hotpoints::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Map_Hotpoints)
	DDX_Control(pDX, IDC_LIST_HOTPOINTS, m_hotpoints);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Map_Hotpoints, CDialog)
	//{{AFX_MSG_MAP(Dialog_Map_Hotpoints)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Hotpoints message handlers

void Dialog_Map_Hotpoints::OnButtonRemove() 
{
	if ( m_hotpoints.GetCurSel() != LB_ERR )
	{
		gEditorHotpoints.Delete( m_hotpoints.GetItemData( m_hotpoints.GetCurSel() ), true );
	}
}

BOOL Dialog_Map_Hotpoints::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
	siege::SiegeHotpointList::iterator i;
	for ( i = hotpoints.begin(); i != hotpoints.end(); ++i )
	{
		int index = m_hotpoints.AddString( ToAnsi( (*i).m_Name.c_str() ) );		
		m_hotpoints.SetItemData( index, (*i).m_Id );
	}
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
