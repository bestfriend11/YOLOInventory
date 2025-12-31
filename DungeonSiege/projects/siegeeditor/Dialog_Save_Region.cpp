// Dialog_Save_Region.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Save_Region.h"
#include "EditorRegion.h"
#include "Preferences.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Region dialog


Dialog_Save_Region::Dialog_Save_Region(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Save_Region::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Save_Region)
	//}}AFX_DATA_INIT
}


void Dialog_Save_Region::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Save_Region)
	DDX_Control(pDX, IDC_CHECK_STARTING_POS, m_startingpos);
	DDX_Control(pDX, IDC_CHECK_STITCHDATA, m_stitch);
	DDX_Control(pDX, IDC_CHECK_NODES, m_nodes);
	DDX_Control(pDX, IDC_CHECK_LIGHTS, m_lights);
	DDX_Control(pDX, IDC_CHECK_GAMEOBJECTS, m_gameobjects);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Save_Region, CDialog)
	//{{AFX_MSG_MAP(Dialog_Save_Region)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Region message handlers

void Dialog_Save_Region::OnOK() 
{	
	bool bStitch	= m_stitch.GetCheck() ? true : false;
	bool bNodes		= m_nodes.GetCheck() ? true : false;
	bool bLights	= m_lights.GetCheck() ? true : false;	
	bool bGameObjects = m_gameobjects.GetCheck() ? true : false;
	bool bStartingPos = m_startingpos.GetCheck() ? true : false;

	gPreferences.SetSaveStitch( bStitch );
	gPreferences.SetSaveNodes( bNodes );
	gPreferences.SetSaveLights( bLights );	
	gPreferences.SetSaveGameObjects( bGameObjects );
	gPreferences.SetSaveStartingPos( bStartingPos );
	
	gEditorRegion.SaveRegion( bGameObjects, bLights, bNodes, bStitch, bStartingPos );
	CDialog::OnOK();
}

BOOL Dialog_Save_Region::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_stitch.SetCheck( (int)gPreferences.GetSaveStitch() );
	m_nodes.SetCheck( (int)gPreferences.GetSaveNodes() );
	m_lights.SetCheck( (int)gPreferences.GetSaveLights() );
	m_gameobjects.SetCheck( (int)gPreferences.GetSaveGameObjects() );
	m_startingpos.SetCheck( (int)gPreferences.GetSaveStartingPos() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Save_Region::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
