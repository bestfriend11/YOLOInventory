// Dialog_Object_Next.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Object_Next.h"
#include "EditorObjects.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Next dialog


Dialog_Object_Next::Dialog_Object_Next(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Next::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Next)
	//}}AFX_DATA_INIT
}


void Dialog_Object_Next::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Next)
	DDX_Control(pDX, IDC_EDIT_MAXIMUM_MULTIPLIER, m_edit_max_scale);
	DDX_Control(pDX, IDC_EDIT_MINIMUM_MULTIPLIER, m_edit_min_scale);
	DDX_Control(pDX, IDC_CHECK_YAW, m_yaw);
	DDX_Control(pDX, IDC_CHECK_ROLL, m_roll);
	DDX_Control(pDX, IDC_CHECK_RANDOM_SCALE, m_randomScale);
	DDX_Control(pDX, IDC_CHECK_PITCH, m_pitch);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Next, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Next)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULTS, OnButtonDefaults)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Next message handlers

void Dialog_Object_Next::OnOK() 
{
	gEditorObjects.GetNextObjectSettings().bRandPitch = m_pitch.GetCheck() ? true : false;
	gEditorObjects.GetNextObjectSettings().bRandYaw = m_yaw.GetCheck() ? true : false;
	gEditorObjects.GetNextObjectSettings().bRandRoll = m_roll.GetCheck() ? true : false;
	gEditorObjects.GetNextObjectSettings().bRandomScale = m_randomScale.GetCheck() ? true : false;

	CString rTemp;
	m_edit_min_scale.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), gEditorObjects.GetNextObjectSettings().minMultiplier );

	m_edit_max_scale.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), gEditorObjects.GetNextObjectSettings().maxMultiplier );
	
	CDialog::OnOK();
}

void Dialog_Object_Next::OnButtonDefaults() 
{
	m_pitch.SetCheck( FALSE );
	gEditorObjects.GetNextObjectSettings().bRandPitch = false;

	m_yaw.SetCheck( FALSE );
	gEditorObjects.GetNextObjectSettings().bRandYaw = false;

	m_roll.SetCheck( FALSE );
	gEditorObjects.GetNextObjectSettings().bRandRoll = false;

	m_randomScale.SetCheck( FALSE );
	gEditorObjects.GetNextObjectSettings().bRandomScale = false;

	m_edit_min_scale.SetWindowText( "0.85" );
	gEditorObjects.GetNextObjectSettings().minMultiplier = 0.65f;
	
	m_edit_max_scale.SetWindowText( "1.5" );		
	gEditorObjects.GetNextObjectSettings().maxMultiplier = 1.2f;
}

BOOL Dialog_Object_Next::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pitch.SetCheck( gEditorObjects.GetNextObjectSettings().bRandPitch );
	m_yaw.SetCheck( gEditorObjects.GetNextObjectSettings().bRandYaw );
	m_roll.SetCheck( gEditorObjects.GetNextObjectSettings().bRandRoll );
	m_randomScale.SetCheck( gEditorObjects.GetNextObjectSettings().bRandomScale );

	gpstring sTemp;
	sTemp.assignf( "%f", gEditorObjects.GetNextObjectSettings().minMultiplier );
	m_edit_min_scale.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gEditorObjects.GetNextObjectSettings().maxMultiplier );
	m_edit_max_scale.SetWindowText( sTemp.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Object_Next::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
