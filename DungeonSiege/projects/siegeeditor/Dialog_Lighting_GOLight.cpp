// Dialog_Lighting_GOLight.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Lighting_GOLight.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_GOLight dialog


Dialog_Lighting_GOLight::Dialog_Lighting_GOLight(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Lighting_GOLight::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Lighting_GOLight)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Lighting_GOLight::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Lighting_GOLight)
	DDX_Control(pDX, IDC_LIST_SCRIPTS, m_list_scripts);
	DDX_Control(pDX, IDC_EDIT_MAGNITUDE_Z, m_z);
	DDX_Control(pDX, IDC_EDIT_MAGNITUDE_Y, m_y);
	DDX_Control(pDX, IDC_EDIT_MAGNITUDE_X, m_x);
	DDX_Control(pDX, IDC_EDIT_GUID, m_guid);
	DDX_Control(pDX, IDC_EDIT_GOLIGHT_SCID, m_scid);
	DDX_Control(pDX, IDC_EDIT_CUSTOM_SCRIPT, m_custom_script);
	DDX_Control(pDX, IDC_COMBO_AVAILABLE_SCRIPTS, m_available_scripts);
	DDX_Control(pDX, IDC_CHECK_INITIALLY_ACTIVE, m_initial_active);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Lighting_GOLight, CDialog)
	//{{AFX_MSG_MAP(Dialog_Lighting_GOLight)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_GOLIGHT, OnButtonDeleteGolight)
	ON_LBN_SELCHANGE(IDC_LIST_SCRIPTS, OnSelchangeListScripts)
	ON_BN_CLICKED(IDC_BUTTON_ADD_SCRIPT, OnButtonAddScript)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_SCRIPT, OnButtonRemoveScript)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL_SCRIPTS, OnButtonRemoveAllScripts)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_GOLight message handlers

void Dialog_Lighting_GOLight::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void Dialog_Lighting_GOLight::OnButtonDeleteGolight() 
{
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_GOLight::OnSelchangeListScripts() 
{
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_GOLight::OnButtonAddScript() 
{
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_GOLight::OnButtonRemoveScript() 
{
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_GOLight::OnButtonRemoveAllScripts() 
{
	// TODO: Add your control notification handler code here
	
}

BOOL Dialog_Lighting_GOLight::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
