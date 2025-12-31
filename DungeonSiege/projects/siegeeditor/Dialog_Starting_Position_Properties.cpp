// Dialog_Starting_Position_Properties.cpp : implementation file
//


#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Starting_Position_Properties.h"
#include "EditorGizmos.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Position_Properties dialog


Dialog_Starting_Position_Properties::Dialog_Starting_Position_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Position_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Position_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Position_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Position_Properties)
	DDX_Control(pDX, IDC_COMBO_STARTING_GROUP, m_startGroups);
	DDX_Control(pDX, IDC_EDIT_STARTING_POSITION, m_id);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Position_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Position_Properties)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Position_Properties message handlers

BOOL Dialog_Starting_Position_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	std::vector< DWORD > ids;
	gGizmoManager.GetSelectedGizmosOfType( GIZMO_STARTINGPOSITION, ids );
	if ( ids.size() == 1 )
	{
		int id = gEditorGizmos.GetStartingPositionFromGizmo( (*(ids.begin())) );
		gpstring sNum;
		sNum.assignf( "%d", id );
		m_id.SetWindowText( sNum.c_str() );

		StringVec groups = gEditorGizmos.GetStartingGroups();
		StringVec::iterator j;
		for ( j = groups.begin(); j != groups.end(); ++j )
		{
			m_startGroups.AddString( (*j).c_str() );
		}

		gpstring sGroup;
		gEditorGizmos.GetPositionStartGroup( (*(ids.begin())), sGroup );

		int sel = m_startGroups.FindString( 0, sGroup.c_str() );
		if ( sel != CB_ERR )
		{
			m_startGroups.SetCurSel( sel );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Starting_Position_Properties::OnOK() 
{
	CString rGroup;
	int sel = m_startGroups.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_startGroups.GetLBText( sel, rGroup );

		std::vector< DWORD > ids;
		gGizmoManager.GetSelectedGizmosOfType( GIZMO_STARTINGPOSITION, ids );
		if ( ids.size() == 1 )
		{	
			gEditorGizmos.SetPositionStartGroup( *(ids.begin()), rGroup.GetBuffer( 0 ) );
		}
	}			
	
	CDialog::OnOK();
}

void Dialog_Starting_Position_Properties::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
