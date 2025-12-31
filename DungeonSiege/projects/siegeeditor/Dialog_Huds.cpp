// Dialog_Huds.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Huds.h"
#include "WorldOptions.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Huds dialog


Dialog_Huds::Dialog_Huds(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Huds::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Huds)
	m_group = -1;
	//}}AFX_DATA_INIT
}


void Dialog_Huds::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Huds)
	DDX_Control(pDX, IDC_MESSAGES, m_messages);
	DDX_Control(pDX, IDC_LABELS, m_labels);
	DDX_Control(pDX, IDC_AI_SENSORS, m_ai_sensors);
	DDX_Control(pDX, IDC_AI_PATHFINDER, m_ai_pathfinder);
	DDX_Control(pDX, IDC_CHECK_SINGLE_OBJECT, m_singleObject);
	DDX_Radio(pDX, IDC_NONE, m_group);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Huds, CDialog)
	//{{AFX_MSG_MAP(Dialog_Huds)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Huds message handlers

void Dialog_Huds::OnOK()
{
	CDialog::OnOK();

	if ( (eDebugHudGroup)m_group == DHG_NONE )
	{
		if ( gWorldOptions.GetDebugHudGroup() != DHG_NONE )
		{
			gPreferences.SetLastDebugHudGroup( gWorldOptions.GetDebugHudGroup() );
		}
		gPreferences.SetDrawDebugHuds( false );
	}
	else
	{
		gPreferences.SetDrawDebugHuds( true );
	}

	gWorldOptions.SetDebugHudOptions( DHO_AI_SENSORS, m_ai_sensors.GetCheck() ? true : false );
	gWorldOptions.SetDebugHudOptions( DHO_AI_PATHFINDER, m_ai_pathfinder.GetCheck() ? true : false );
	gWorldOptions.SetDebugHudOptions( DHO_MESSAGES, m_messages.GetCheck() ? true : false );
	gWorldOptions.SetDebugHudOptions( DHO_LABELS, m_labels.GetCheck() ? true : false );
	gWorldOptions.SetDebugHudGroup  ( (eDebugHudGroup)m_group );	

	gPreferences.SetHudSelected( m_singleObject.GetCheck() ? true : false );
	if ( !m_singleObject.GetCheck() )
	{
		gWorldOptions.SetDebugHUDObject( GOID_INVALID );
	}
}

BOOL Dialog_Huds::OnInitDialog()
{
	m_group         = gWorldOptions.GetDebugHudGroup();

	CDialog::OnInitDialog();

	m_ai_sensors.SetCheck( gWorldOptions.TestDebugHudOptions( DHO_AI_SENSORS ) );
	m_ai_pathfinder.SetCheck( gWorldOptions.TestDebugHudOptions( DHO_AI_PATHFINDER ) );
	m_messages.SetCheck( gWorldOptions.TestDebugHudOptions( DHO_MESSAGES ) );
	m_labels.SetCheck( gWorldOptions.TestDebugHudOptions( DHO_LABELS ) );	

	if ( gPreferences.GetHudSelected() )
	{
		m_singleObject.SetCheck( TRUE );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Huds::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
