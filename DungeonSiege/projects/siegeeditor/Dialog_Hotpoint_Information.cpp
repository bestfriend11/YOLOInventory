// Dialog_Hotpoint_Information.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Hotpoint_Information.h"
#include "siege_hotpoint_database.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Information dialog


Dialog_Hotpoint_Information::Dialog_Hotpoint_Information(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Hotpoint_Information::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Hotpoint_Information)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Hotpoint_Information::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Hotpoint_Information)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_ID, m_id);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Hotpoint_Information, CDialog)
	//{{AFX_MSG_MAP(Dialog_Hotpoint_Information)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Information message handlers

BOOL Dialog_Hotpoint_Information::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int id = gGizmoManager.GetLeadGizmo()->id;

	siege::SiegeHotpointList hotpoints = gSiegeHotpointDatabase.GetHotpointList();
	siege::SiegeHotpointList::iterator i;
	for ( i = hotpoints.begin(); i != hotpoints.end(); ++i )
	{		
		if ( id == (*i).m_Id ) 
		{
			gpstring sTemp;
			sTemp.assignf( "0x%08X", id );
			m_id.SetWindowText( sTemp.c_str() );
			m_name.SetWindowText( ToAnsi( (*i).m_Name.c_str() ) );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
